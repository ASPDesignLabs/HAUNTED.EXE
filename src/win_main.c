/* Win32 screensaver shell.
 *
 *   HAUNTED.scr /s        run full screen
 *   HAUNTED.scr /c[:hwnd] settings dialog
 *   HAUNTED.scr /p hwnd   live preview inside the Display Settings dialog
 *   HAUNTED.scr /w        windowed test mode (Esc to quit, F11 for borderless full screen)
 *   HAUNTED.scr /f        capture mode: a borderless full-screen window that ignores the
 *                         mouse and keeps running, for OBS or Xbox Game Bar (Esc quits, F11 toggles)
 *
 * Renamed to HAUNTED.EXE, the same file opens straight into the test window when started with no
 * argument (a .scr opens its settings then, as Windows expects).
 *
 * In any of the running modes, holding H for three seconds hands CATACOMB to the keyboard and
 * mouse; the mouse is then read raw and kept inside the window. Holding any button of a game
 * controller (XInput, loaded if Windows has it) does the same, and the pad plays it. Choosing to leave saves the
 * progress in the registry, and the next run starts from it.
 *
 * Output is an indexed VGA frame scaled by an integer factor with nearest
 * neighbour filtering (OpenGL 1.1, GDI as a fallback).  Sound goes through
 * waveOut on its own thread. */
#define WIN32_LEAN_AND_MEAN
#define _WIN32_WINNT 0x0601
#include <windows.h>
#include <commctrl.h>
#include <mmsystem.h>
#include <GL/gl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "app.h"
#include "snd.h"
#include "build/version.h"

#define REG_KEY "Software\\HalloweenVGASaver"          /* the old name, so settings and progress carry over */
#define APP_TITLE "HAUNTED.EXE"

/* ------------------------------------------------------------------ */
/* Settings                                                            */
/* ------------------------------------------------------------------ */

typedef struct { AppConfig app; int largest_monitor; } Settings;

static DWORD reg_get(HKEY k, const char *name, DWORD def) {
    DWORD v, sz = sizeof v, type = 0;
    if (RegQueryValueExA(k, name, NULL, &type, (LPBYTE)&v, &sz) == ERROR_SUCCESS && type == REG_DWORD) return v;
    return def;
}
static void reg_set(HKEY k, const char *name, DWORD v) {
    RegSetValueExA(k, name, 0, REG_DWORD, (const BYTE *)&v, sizeof v);
}

static void settings_load(Settings *s) {
    app_default_config(&s->app);
    s->largest_monitor = 1;
    HKEY k;
    if (RegOpenKeyExA(HKEY_CURRENT_USER, REG_KEY, 0, KEY_READ, &k) != ERROR_SUCCESS) return;
    s->app.sound = (int)reg_get(k, "Sound", (DWORD)s->app.sound);
    s->app.volume = (int)reg_get(k, "Volume", (DWORD)s->app.volume);
    s->app.brightness = (int)reg_get(k, "Brightness", (DWORD)s->app.brightness);
    s->app.stay_seconds = (int)reg_get(k, "StaySeconds", (DWORD)s->app.stay_seconds);
    s->app.pixel_shift = (int)reg_get(k, "PixelShift", (DWORD)s->app.pixel_shift);
    s->app.scene_enabled[0] = (int)reg_get(k, "SceneHaunt", 1);
    s->app.scene_enabled[1] = (int)reg_get(k, "SceneDemo", 1);
    s->largest_monitor = (int)reg_get(k, "LargestMonitor", 1);
    s->app.pad = (int)reg_get(k, "Controller", 1);
    RegCloseKey(k);
    if (s->app.volume < 0) s->app.volume = 0;
    if (s->app.volume > 100) s->app.volume = 100;
    if (s->app.brightness < 30) s->app.brightness = 30;
    if (s->app.brightness > 100) s->app.brightness = 100;
    if (s->app.stay_seconds < 30) s->app.stay_seconds = 30;
    if (!s->app.scene_enabled[0] && !s->app.scene_enabled[1]) s->app.scene_enabled[0] = 1;
}

/* the merchant's ledger: its pages as values Slot1 to Slot3, and beside them Progress (the page to start
 * from) and Mark (the dead's), all loaded before the first scene and written whenever the game changes one */
static const char *slot_name(int i, char *buf, int n) {
    if (i == SLOT_PROGRESS) return "Progress";
    if (i == SLOT_MARK) return "Mark";
    snprintf(buf, (size_t)n, "Slot%d", i + 1);
    return buf;
}
static void slots_load(void) {
    HKEY k;
    if (RegOpenKeyExA(HKEY_CURRENT_USER, REG_KEY, 0, KEY_READ, &k) != ERROR_SUCCESS) return;
    for (int i = 0; i < SLOT_ALL; i++) {
        static uint8_t buf[SLOT_MAX];
        char name[8];
        DWORD sz = sizeof buf, type = 0;
        if (RegQueryValueExA(k, slot_name(i, name, sizeof name), NULL, &type, buf, &sz) == ERROR_SUCCESS && type == REG_BINARY) app_slot_load(i, buf, (int)sz);
    }
    RegCloseKey(k);
}
static void slots_flush(void) {
    for (int i = 0; i < SLOT_ALL; i++) {
        static uint8_t buf[SLOT_MAX];
        int n = app_slot_take(i, buf, sizeof buf);
        HKEY k;
        if (n <= 0 || RegCreateKeyExA(HKEY_CURRENT_USER, REG_KEY, 0, NULL, 0, KEY_WRITE, NULL, &k, NULL) != ERROR_SUCCESS) continue;
        char name[8];
        RegSetValueExA(k, slot_name(i, name, sizeof name), 0, REG_BINARY, buf, (DWORD)n);
        RegCloseKey(k);
    }
}

static void settings_save(const Settings *s) {
    HKEY k;
    if (RegCreateKeyExA(HKEY_CURRENT_USER, REG_KEY, 0, NULL, 0, KEY_WRITE, NULL, &k, NULL) != ERROR_SUCCESS) return;
    reg_set(k, "Sound", (DWORD)s->app.sound);
    reg_set(k, "Volume", (DWORD)s->app.volume);
    reg_set(k, "Brightness", (DWORD)s->app.brightness);
    reg_set(k, "StaySeconds", (DWORD)s->app.stay_seconds);
    reg_set(k, "PixelShift", (DWORD)s->app.pixel_shift);
    reg_set(k, "SceneHaunt", (DWORD)s->app.scene_enabled[0]);
    reg_set(k, "SceneDemo", (DWORD)s->app.scene_enabled[1]);
    reg_set(k, "LargestMonitor", (DWORD)s->largest_monitor);
    reg_set(k, "Controller", (DWORD)s->app.pad);
    RegCloseKey(k);
}

/* ------------------------------------------------------------------ */
/* Audio (waveOut, double-buffered ring on its own thread)             */
/* ------------------------------------------------------------------ */

#define AUDIO_FRAMES 1024
#define AUDIO_BUFS 4

static HWAVEOUT g_wo;
static WAVEHDR g_hdr[AUDIO_BUFS];
static int16_t g_abuf[AUDIO_BUFS][AUDIO_FRAMES * 2];
static HANDLE g_aevent, g_athread;
static volatile LONG g_arun;

static DWORD WINAPI audio_thread(LPVOID unused) {
    (void)unused;
    while (g_arun) {
        WaitForSingleObject(g_aevent, 40);
        for (int i = 0; i < AUDIO_BUFS && g_arun; i++) {
            if (g_hdr[i].dwFlags & WHDR_DONE) {
                snd_render(g_abuf[i], AUDIO_FRAMES);
                g_hdr[i].dwFlags &= ~WHDR_DONE;
                waveOutWrite(g_wo, &g_hdr[i], sizeof(WAVEHDR));
            }
        }
    }
    return 0;
}

static int audio_start(void) {
    WAVEFORMATEX wf;
    memset(&wf, 0, sizeof wf);
    wf.wFormatTag = WAVE_FORMAT_PCM; wf.nChannels = 2; wf.nSamplesPerSec = SND_RATE;
    wf.wBitsPerSample = 16; wf.nBlockAlign = 4; wf.nAvgBytesPerSec = SND_RATE * 4;
    g_aevent = CreateEventA(NULL, FALSE, FALSE, NULL);
    if (waveOutOpen(&g_wo, WAVE_MAPPER, &wf, (DWORD_PTR)g_aevent, 0, CALLBACK_EVENT) != MMSYSERR_NOERROR) {
        CloseHandle(g_aevent); g_aevent = NULL; g_wo = NULL;
        return 0;
    }
    snd_set_latency(AUDIO_FRAMES * (AUDIO_BUFS - 1));
    for (int i = 0; i < AUDIO_BUFS; i++) {
        memset(&g_hdr[i], 0, sizeof g_hdr[i]);
        g_hdr[i].lpData = (LPSTR)g_abuf[i];
        g_hdr[i].dwBufferLength = sizeof g_abuf[i];
        waveOutPrepareHeader(g_wo, &g_hdr[i], sizeof(WAVEHDR));
        snd_render(g_abuf[i], AUDIO_FRAMES);
        waveOutWrite(g_wo, &g_hdr[i], sizeof(WAVEHDR));
    }
    g_arun = 1;
    g_athread = CreateThread(NULL, 0, audio_thread, NULL, 0, NULL);
    if (g_athread) SetThreadPriority(g_athread, THREAD_PRIORITY_ABOVE_NORMAL);
    return 1;
}

static void audio_stop(void) {
    if (!g_wo) return;
    g_arun = 0;
    if (g_aevent) SetEvent(g_aevent);
    if (g_athread) { WaitForSingleObject(g_athread, 1000); CloseHandle(g_athread); g_athread = NULL; }
    waveOutReset(g_wo);
    for (int i = 0; i < AUDIO_BUFS; i++) waveOutUnprepareHeader(g_wo, &g_hdr[i], sizeof(WAVEHDR));
    waveOutClose(g_wo);
    g_wo = NULL;
    if (g_aevent) { CloseHandle(g_aevent); g_aevent = NULL; }
}

/* ------------------------------------------------------------------ */
/* Display                                                             */
/* ------------------------------------------------------------------ */

typedef struct {
    HWND hwnd;
    HDC hdc;
    HGLRC rc;
    int use_gl;
    GLuint tex;
    int win_w, win_h;
    int last_mw, last_mh;
} Display;

static uint32_t g_pixels[FB_MAX_W * FB_MAX_H];
static DWORD g_start_tick;
static const char *g_dump_path;
static int g_gltest;
static void request_quit(void);
static BITMAPINFO g_bmi;

typedef BOOL (WINAPI *PFN_SWAPINTERVAL)(int);

static int gl_init(Display *d) {
    PIXELFORMATDESCRIPTOR pfd;
    memset(&pfd, 0, sizeof pfd);
    pfd.nSize = sizeof pfd; pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA; pfd.cColorBits = 32;
    int pf = ChoosePixelFormat(d->hdc, &pfd);
    if (!pf || !SetPixelFormat(d->hdc, pf, &pfd)) return 0;
    d->rc = wglCreateContext(d->hdc);
    if (!d->rc || !wglMakeCurrent(d->hdc, d->rc)) return 0;
    PFN_SWAPINTERVAL swap = (PFN_SWAPINTERVAL)(void *)wglGetProcAddress("wglSwapIntervalEXT");
    if (swap) swap(1);
    glGenTextures(1, &d->tex);
    glBindTexture(GL_TEXTURE_2D, d->tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1024, 512, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glEnable(GL_TEXTURE_2D);
    return 1;
}

static void display_open(Display *d, HWND hwnd, int want_gl) {
    memset(d, 0, sizeof *d);
    d->hwnd = hwnd;
    d->hdc = GetDC(hwnd);
    d->use_gl = want_gl && gl_init(d);
    if (!d->use_gl) {
        if (d->rc) { wglMakeCurrent(NULL, NULL); wglDeleteContext(d->rc); d->rc = NULL; }
        memset(&g_bmi, 0, sizeof g_bmi);
        g_bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        g_bmi.bmiHeader.biPlanes = 1;
        g_bmi.bmiHeader.biBitCount = 32;
        g_bmi.bmiHeader.biCompression = BI_RGB;
    }
}

static void display_close(Display *d) {
    if (d->rc) { wglMakeCurrent(NULL, NULL); wglDeleteContext(d->rc); d->rc = NULL; }
    if (d->hdc) { ReleaseDC(d->hwnd, d->hdc); d->hdc = NULL; }
}

/* integer scale that fits; small preview windows fall back to a fit */
static void compute_dest(const Display *d, int mw, int mh, int exact, int *x, int *y, int *w, int *h) {
    int sx = d->win_w / mw, sy = d->win_h / mh;
    int s = sx < sy ? sx : sy;
    if (s >= 1) {
        *w = mw * s; *h = mh * s;
        int dx = 0, dy = 0;
        if (exact) {
            app_pixel_shift(&dx, &dy);
            dx *= s; dy *= s;
            int mx = (d->win_w - *w) / 2, my = (d->win_h - *h) / 2;
            if (dx > mx) dx = mx; if (dx < -mx) dx = -mx;
            if (dy > my) dy = my; if (dy < -my) dy = -my;
        }
        *x = (d->win_w - *w) / 2 + dx; *y = (d->win_h - *h) / 2 + dy;
    } else {                                           /* window smaller than the mode: fit, keep aspect */
        float fx = (float)d->win_w / mw, fy = (float)d->win_h / mh;
        float f = fx < fy ? fx : fy;
        *w = (int)(mw * f); *h = (int)(mh * f);
        *x = (d->win_w - *w) / 2; *y = (d->win_h - *h) / 2;
    }
}

static void display_frame(Display *d, int exact) {
    int mw, mh, dx, dy, dw, dh;
    app_mode(&mw, &mh);
    app_present(g_pixels, d->use_gl ? 0 : 1);
    compute_dest(d, mw, mh, exact, &dx, &dy, &dw, &dh);
    if (d->use_gl) {
        glViewport(0, 0, d->win_w, d->win_h);
        glMatrixMode(GL_PROJECTION); glLoadIdentity();
        glOrtho(0, d->win_w, d->win_h, 0, -1, 1);
        glMatrixMode(GL_MODELVIEW); glLoadIdentity();
        glClearColor(0, 0, g_gltest ? 0.35f : 0.0f, 1);
        glClear(GL_COLOR_BUFFER_BIT);
        if (g_gltest >= 2) glDisable(GL_TEXTURE_2D);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, mw, mh, GL_RGBA, GL_UNSIGNED_BYTE, g_pixels);
        float u = (float)mw / 1024.0f, v = (float)mh / 512.0f;
        glColor3f(1, 1, 1);
        glBegin(GL_QUADS);
        glTexCoord2f(0, 0); glVertex2i(dx, dy);
        glTexCoord2f(u, 0); glVertex2i(dx + dw, dy);
        glTexCoord2f(u, v); glVertex2i(dx + dw, dy + dh);
        glTexCoord2f(0, v); glVertex2i(dx, dy + dh);
        glEnd();
        if (g_dump_path && GetTickCount() - g_start_tick > 9000) {    /* debug: HVS_DUMP=file.ppm */
            unsigned char *px = (unsigned char *)malloc((size_t)d->win_w * d->win_h * 3);
            if (px) {
                glPixelStorei(GL_PACK_ALIGNMENT, 1);
                glReadPixels(0, 0, d->win_w, d->win_h, GL_RGB, GL_UNSIGNED_BYTE, px);
                FILE *f = fopen(g_dump_path, "wb");
                if (f) {
                    fprintf(f, "P6\n%d %d\n255\n", d->win_w, d->win_h);
                    for (int y = d->win_h - 1; y >= 0; y--) fwrite(px + (size_t)y * d->win_w * 3, 3, (size_t)d->win_w, f);
                    fclose(f);
                }
                free(px);
            }
            request_quit();
        }
        SwapBuffers(d->hdc);
    } else {
        if (d->last_mw != mw || d->last_mh != mh || exact) {          /* clear borders when the mode or shift changes */
            RECT r = { 0, 0, d->win_w, d->win_h };
            FillRect(d->hdc, &r, (HBRUSH)GetStockObject(BLACK_BRUSH));
            d->last_mw = mw; d->last_mh = mh;
        }
        g_bmi.bmiHeader.biWidth = mw;
        g_bmi.bmiHeader.biHeight = -mh;
        if (dw < mw) { SetStretchBltMode(d->hdc, HALFTONE); SetBrushOrgEx(d->hdc, 0, 0, NULL); }   /* preview thumbnail: smooth shrink */
        else SetStretchBltMode(d->hdc, COLORONCOLOR);                                              /* otherwise: hard pixels */
        StretchDIBits(d->hdc, dx, dy, dw, dh, 0, 0, mw, mh, g_pixels, &g_bmi, DIB_RGB_COLORS, SRCCOPY);
    }
}

/* ------------------------------------------------------------------ */
/* Windows                                                             */
/* ------------------------------------------------------------------ */

enum { MODE_SAVER, MODE_CONFIG, MODE_PREVIEW, MODE_WINDOWED, MODE_CAPTURE };

static int g_mode = MODE_SAVER;
static POINT g_mouse0;
static int g_mouse0_set;
static Display g_disp;
static HWND g_main;
static volatile int g_quit;
static Input g_in;                      /* the keys and mouse, for whoever holds H */
static int g_captured;                  /* the cursor is hidden and kept in the window */

static int vk_key(WPARAM vk) {
    switch (vk) {
    case VK_UP: return KEY_UP; case VK_DOWN: return KEY_DOWN; case VK_LEFT: return KEY_LEFT; case VK_RIGHT: return KEY_RIGHT;
    case 'W': return KEY_W; case 'A': return KEY_A; case 'S': return KEY_S; case 'D': return KEY_D;
    case 'E': return KEY_E; case 'Q': return KEY_Q; case 'H': return KEY_H; case 'Y': return KEY_Y; case 'N': return KEY_N;
    case 'B': return KEY_B; case 'L': return KEY_L; case 'C': return KEY_C; case 'M': return KEY_M;
    case VK_SPACE: return KEY_SPACE; case VK_CONTROL: return KEY_CTRL; case VK_SHIFT: return KEY_SHIFT;
    case VK_TAB: return KEY_TAB; case VK_ESCAPE: return KEY_ESC; case VK_RETURN: return KEY_ENTER;
    case VK_BACK: return KEY_BACK; case VK_PRIOR: return KEY_PGUP; case VK_NEXT: return KEY_PGDN;
    default: return vk >= '1' && vk <= '9' ? KEY_1 + (int)(vk - '1') : -1;
    }
}
/* the key left of 1 by where it is (scan code 0x29), not what it prints, as in Quake */
static int key_of(WPARAM vk, LPARAM lp) { return ((lp >> 16) & 0xFF) == 0x29 ? KEY_CONSOLE : vk_key(vk); }
static void key_event(int k, int down, LPARAM lp) {
    if (k < 0) return;
    if (down && !(lp & (1 << 30))) g_in.hit[k] = 1;              /* not the key repeating */
    g_in.down[k] = (uint8_t)down;
}

/* while a person plays: no cursor, and it can't wander off the window */
static void set_capture(int on) {
    if (on == g_captured) return;
    g_captured = on;
    if (on) {
        RECT r; GetClientRect(g_main, &r);
        MapWindowPoints(g_main, NULL, (POINT *)&r, 2);
        ClipCursor(&r);
        if (g_mode != MODE_SAVER) while (ShowCursor(FALSE) >= 0) {}
    } else {
        ClipCursor(NULL);
        if (g_mode != MODE_SAVER) while (ShowCursor(TRUE) < 0) {}
        memset(&g_in, 0, sizeof g_in);
    }
}

static void request_quit(void) { g_quit = 1; }

/* ------------------------------------------------------------------ */
/* A game controller (XInput), if Windows has it                       */
/* ------------------------------------------------------------------ */

/* XInput's own structures, declared here so it is only loaded, never linked */
typedef struct { WORD buttons; BYTE lt, rt; SHORT lx, ly, rx, ry; } XPad;
typedef struct { DWORD packet; XPad pad; } XState;
typedef struct { WORD low, high; } XVibration;
typedef DWORD (WINAPI *XGetStateFn)(DWORD, XState *);
typedef DWORD (WINAPI *XSetStateFn)(DWORD, XVibration *);
static XGetStateFn g_xget;
static XSetStateFn g_xset;
static int g_pad_on;                    /* the setting */
static int g_pad_in[4];                 /* which slots have a pad */
static DWORD g_pad_look[4];             /* when an empty slot was last asked: it is slow to answer */
static double g_pad_touch = -1e9;       /* when the pad was last touched */
static WORD g_mot_lo, g_mot_hi;         /* the motors as last set */

static double now_seconds(void);
static void pad_open(int on) {
    static const char *const DLL[3] = { "xinput1_4.dll", "xinput1_3.dll", "xinput9_1_0.dll" };
    g_pad_on = on;
    for (int i = 0; i < 3 && on && !g_xget; i++) {
        HMODULE m = LoadLibraryA(DLL[i]);
        if (!m) continue;
        g_xget = (XGetStateFn)(void *)GetProcAddress(m, "XInputGetState");
        g_xset = (XSetStateFn)(void *)GetProcAddress(m, "XInputSetState");
        if (!g_xget) FreeLibrary(m);
    }
    for (int i = 0; i < 4; i++) { g_pad_in[i] = 1; g_pad_look[i] = 0; }   /* ask every slot once */
}
/* a stick past its dead zone, round, the rest stretched back to 0..1 */
static void pad_stick(SHORT sx, SHORT sy, float dead, float *ox, float *oy) {
    float x = sx / 32767.0f, y = sy / 32767.0f, m = sqrtf(x * x + y * y), d = dead / 32767.0f;
    if (m <= d) { *ox = *oy = 0.0f; return; }
    float k = (fminf(m, 1.0f) - d) / (1.0f - d) / m;
    *ox = clampf(x * k, -1.0f, 1.0f); *oy = clampf(y * k, -1.0f, 1.0f);
}
static void pad_motors(WORD lo, WORD hi) {
    if (!g_xset || (lo == g_mot_lo && hi == g_mot_hi)) return;
    XVibration v = { lo, hi };
    for (int i = 0; i < 4; i++) if (g_pad_in[i]) g_xset((DWORD)i, &v);
    g_mot_lo = lo; g_mot_hi = hi;
}
/* a person playing with the pad, touched in the last two minutes: Windows doesn't count it as activity */
static int pad_playing(void) { return app_control_state() == APP_PLAYER && now_seconds() - g_pad_touch < 120.0; }
static void pad_poll(void) {
    static const WORD BIT[PAD_COUNT] = { 0x1000, 0x2000, 0x4000, 0x8000, 0x0100, 0x0200, 0x0020, 0x0010, 0x0040, 0x0080,
                                         0x0001, 0x0002, 0x0004, 0x0008, 0, 0 };
    uint8_t was[PAD_COUNT];
    memcpy(was, g_in.pad_down, sizeof was);
    memset(g_in.pad_down, 0, sizeof g_in.pad_down);
    g_in.lx = g_in.ly = g_in.rx = g_in.ry = g_in.lt = g_in.rt = 0.0f;
    g_in.pad = 0;
    int focus = g_mode == MODE_SAVER || GetForegroundWindow() == g_main;   /* the program reads it only while it is in front */
    if (!g_pad_on || !g_xget || !focus) { pad_motors(0, 0); return; }
    DWORD now = GetTickCount();
    for (int i = 0; i < 4; i++) {
        if (!g_pad_in[i] && now - g_pad_look[i] < 2000) continue;
        XState st;
        g_pad_look[i] = now;
        if (g_xget((DWORD)i, &st) != ERROR_SUCCESS) { g_pad_in[i] = 0; continue; }
        g_pad_in[i] = 1; g_in.pad = 1;
        for (int b = 0; b < PAD_COUNT; b++) if (BIT[b] && (st.pad.buttons & BIT[b])) g_in.pad_down[b] = 1;
        float lt = st.pad.lt / 255.0f, rt = st.pad.rt / 255.0f, lx, ly, rx, ry;
        g_in.lt = fmaxf(g_in.lt, lt > 0.1f ? lt : 0.0f); g_in.rt = fmaxf(g_in.rt, rt > 0.1f ? rt : 0.0f);
        if (lt > 0.33f) g_in.pad_down[PAD_LT] = 1;
        if (rt > 0.33f) g_in.pad_down[PAD_RT] = 1;
        pad_stick(st.pad.lx, st.pad.ly, 7849.0f, &lx, &ly);        /* XInput's own suggested dead zones */
        pad_stick(st.pad.rx, st.pad.ry, 8689.0f, &rx, &ry);
        if (fabsf(lx) + fabsf(ly) > fabsf(g_in.lx) + fabsf(g_in.ly)) { g_in.lx = lx; g_in.ly = ly; }
        if (fabsf(rx) + fabsf(ry) > fabsf(g_in.rx) + fabsf(g_in.ry)) { g_in.rx = rx; g_in.ry = ry; }
    }
    int touched = g_in.lx != 0.0f || g_in.ly != 0.0f || g_in.rx != 0.0f || g_in.ry != 0.0f;
    for (int b = 0; b < PAD_COUNT; b++) {
        if (g_in.pad_down[b] && !was[b]) g_in.pad_hit[b] = 1;
        touched |= g_in.pad_down[b];
    }
    if (touched) g_pad_touch = now_seconds();
    if (pad_playing()) {                                           /* keep the display awake, now and then */
        static double told;
        if (now_seconds() - told > 20.0) { told = now_seconds(); SetThreadExecutionState(ES_DISPLAY_REQUIRED | ES_SYSTEM_REQUIRED); }
    }
    float lo = 0.0f, hi = 0.0f;
    if (app_control_state() == APP_PLAYER) app_pad_motors(&lo, &hi);
    pad_motors((WORD)(lo * 65535.0f), (WORD)(hi * 65535.0f));
}

/* F11 in the test and capture windows: borderless over the whole monitor, or back to a normal
 * window.  The borderless window is one pixel taller than the monitor so Windows keeps composing
 * it (no exclusive full-screen switch), which is what screen recorders need to see it. */
static RECT g_win_rect = { 80, 60, 1536, 909 };
static int g_borderless;
static void set_borderless(int on) {
    MONITORINFO mi; mi.cbSize = sizeof mi;
    GetMonitorInfoA(MonitorFromWindow(g_main, MONITOR_DEFAULTTONEAREST), &mi);
    if (on) {
        if (!g_borderless) GetWindowRect(g_main, &g_win_rect);
        SetWindowLongA(g_main, GWL_STYLE, WS_POPUP | WS_VISIBLE);
        SetWindowPos(g_main, HWND_TOP, mi.rcMonitor.left, mi.rcMonitor.top, mi.rcMonitor.right - mi.rcMonitor.left,
                     mi.rcMonitor.bottom - mi.rcMonitor.top + 1, SWP_FRAMECHANGED | SWP_SHOWWINDOW);
    } else {
        SetWindowLongA(g_main, GWL_STYLE, WS_OVERLAPPEDWINDOW | WS_VISIBLE);
        SetWindowPos(g_main, HWND_TOP, g_win_rect.left, g_win_rect.top, g_win_rect.right - g_win_rect.left,
                     g_win_rect.bottom - g_win_rect.top, SWP_FRAMECHANGED | SWP_SHOWWINDOW);
    }
    g_borderless = on;
}

static LRESULT CALLBACK saver_proc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    int play = g_mode != MODE_PREVIEW ? app_control_state() : APP_AUTO;
    switch (msg) {
    case WM_SETCURSOR:
        if (g_mode == MODE_SAVER || play == APP_PLAYER || (g_borderless && LOWORD(lp) == HTCLIENT)) { SetCursor(NULL); return TRUE; }
        break;
    case WM_ERASEBKGND: return 1;
    case WM_PAINT: { PAINTSTRUCT ps; BeginPaint(hwnd, &ps); EndPaint(hwnd, &ps); return 0; }
    case WM_SIZE:
        g_disp.win_w = LOWORD(lp); g_disp.win_h = HIWORD(lp);
        if (g_captured) { set_capture(0); set_capture(1); }
        return 0;
    case WM_KEYDOWN: case WM_SYSKEYDOWN: {
        int k = key_of(wp, lp);
        if (wp == VK_F11 || (wp == VK_RETURN && (GetKeyState(VK_MENU) & 0x8000))) {
            if (g_mode == MODE_WINDOWED || g_mode == MODE_CAPTURE) set_borderless(!g_borderless);
            else if (play != APP_PLAYER) request_quit();
        } else if (play == APP_PLAYER || k == KEY_H) key_event(k, 1, lp);   /* a player's keys, or H being held */
        else if (g_mode == MODE_SAVER || wp == VK_ESCAPE) request_quit();
        if (msg == WM_SYSKEYDOWN && wp != VK_F4) return 0;             /* Alt alone mustn't open the system menu */
        break;
    }
    case WM_KEYUP: case WM_SYSKEYUP:
        key_event(key_of(wp, lp), 0, lp);
        return 0;
    case WM_CHAR:                                                  /* letters and digits, for the console */
        if (play == APP_PLAYER) {
            char c = (char)wp;
            if (c >= 'a' && c <= 'z') c = (char)(c - 32);
            if (((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9')) && g_in.ntext < (int)sizeof g_in.text) g_in.text[g_in.ntext++] = c;
        }
        return 0;
    case WM_LBUTTONDOWN: case WM_RBUTTONDOWN:
        if (play == APP_PLAYER) { key_event(msg == WM_LBUTTONDOWN ? KEY_MOUSE1 : KEY_MOUSE2, 1, 0); return 0; }
        if (g_mode == MODE_SAVER) request_quit();
        return 0;
    case WM_LBUTTONUP: case WM_RBUTTONUP:
        key_event(msg == WM_LBUTTONUP ? KEY_MOUSE1 : KEY_MOUSE2, 0, 0);
        return 0;
    case WM_MBUTTONDOWN: case WM_XBUTTONDOWN:
        if (g_mode == MODE_SAVER && play != APP_PLAYER) request_quit();
        return 0;
    case WM_INPUT: {                                               /* raw mouse: turning, and the wheel */
        if (play != APP_PLAYER) break;
        RAWINPUT ri; UINT sz = sizeof ri;
        if (GetRawInputData((HRAWINPUT)lp, RID_INPUT, &ri, &sz, sizeof(RAWINPUTHEADER)) != (UINT)-1 && ri.header.dwType == RIM_TYPEMOUSE) {
            if (!(ri.data.mouse.usFlags & MOUSE_MOVE_ABSOLUTE)) { g_in.mdx += (float)ri.data.mouse.lLastX; g_in.mdy += (float)ri.data.mouse.lLastY; }
            if (ri.data.mouse.usButtonFlags & RI_MOUSE_WHEEL) g_in.wheel += (SHORT)ri.data.mouse.usButtonData / WHEEL_DELTA;
        }
        break;
    }
    case WM_MOUSEMOVE:
        if (g_mode == MODE_SAVER) {
            POINT p; GetCursorPos(&p);
            if (!g_mouse0_set || play != APP_AUTO) { g_mouse0 = p; g_mouse0_set = 1; }   /* holding H or playing: the mouse may move */
            else if (GetTickCount() - g_start_tick > 800 &&
                     (abs(p.x - g_mouse0.x) + abs(p.y - g_mouse0.y)) > 8) request_quit();
        }
        return 0;
    case WM_ACTIVATEAPP:
        if (!wp) {
            if (play == APP_PLAYER) { app_focus(0); set_capture(0); }  /* away mid-game: it waits at the menu */
            else if (g_mode == MODE_SAVER && GetTickCount() - g_start_tick > 1500) request_quit();
            memset(&g_in, 0, sizeof g_in);
        }
        return 0;
    case WM_SYSCOMMAND:
        if (g_mode == MODE_SAVER && ((wp & 0xFFF0) == SC_SCREENSAVE || (wp & 0xFFF0) == SC_CLOSE)) return 0;
        if (g_mode != MODE_PREVIEW && pad_playing() && ((wp & 0xFFF0) == SC_SCREENSAVE || (wp & 0xFFF0) == SC_MONITORPOWER)) return 0;
        break;
    case WM_CLOSE: request_quit(); return 0;
    case WM_DESTROY:
        if (g_mode == MODE_PREVIEW) PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcA(hwnd, msg, wp, lp);
}

static LRESULT CALLBACK black_proc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_SETCURSOR: SetCursor(NULL); return TRUE;
    case WM_ERASEBKGND: {
        RECT r; GetClientRect(hwnd, &r);
        FillRect((HDC)wp, &r, (HBRUSH)GetStockObject(BLACK_BRUSH));
        return 1;
    }
    case WM_KEYDOWN: case WM_SYSKEYDOWN: case WM_LBUTTONDOWN: case WM_RBUTTONDOWN: case WM_MBUTTONDOWN:
        request_quit(); return 0;
    case WM_MOUSEMOVE: {
        POINT p; GetCursorPos(&p);
        if (GetTickCount() - g_start_tick > 800 && g_mouse0_set &&
            (abs(p.x - g_mouse0.x) + abs(p.y - g_mouse0.y)) > 8) request_quit();
        return 0;
    }
    case WM_SYSCOMMAND:
        if ((wp & 0xFFF0) == SC_SCREENSAVE || (wp & 0xFFF0) == SC_CLOSE) return 0;
        break;
    }
    return DefWindowProcA(hwnd, msg, wp, lp);
}

typedef struct { RECT rc; HMONITOR mon; int primary; } MonInfo;
static MonInfo g_mons[16];
static int g_nmons;

static BOOL CALLBACK mon_enum(HMONITOR m, HDC dc, LPRECT r, LPARAM lp) {
    (void)dc; (void)lp;
    if (g_nmons >= 16) return FALSE;
    MONITORINFO mi; mi.cbSize = sizeof mi;
    GetMonitorInfoA(m, &mi);
    g_mons[g_nmons].rc = mi.rcMonitor;
    g_mons[g_nmons].mon = m;
    g_mons[g_nmons].primary = (mi.dwFlags & MONITORINFOF_PRIMARY) != 0;
    g_nmons++;
    (void)r;
    return TRUE;
}

static void enable_dpi_awareness(void) {
    HMODULE u = GetModuleHandleA("user32.dll");
    typedef BOOL (WINAPI *Fn1)(HANDLE);
    Fn1 f = (Fn1)(void *)GetProcAddress(u, "SetProcessDpiAwarenessContext");
    if (f && f((HANDLE)-4)) return;                  /* per-monitor v2 */
    typedef BOOL (WINAPI *Fn2)(void);
    Fn2 g = (Fn2)(void *)GetProcAddress(u, "SetProcessDPIAware");
    if (g) g();
}

static void pump_messages(void) {
    MSG m;
    while (PeekMessageA(&m, NULL, 0, 0, PM_REMOVE)) {
        if (m.message == WM_QUIT) { g_quit = 1; return; }
        TranslateMessage(&m);
        DispatchMessageA(&m);
    }
}

static double now_seconds(void) {
    static LARGE_INTEGER freq;
    LARGE_INTEGER t;
    if (!freq.QuadPart) QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&t);
    return (double)t.QuadPart / (double)freq.QuadPart;
}

/* ------------------------------------------------------------------ */
/* Running the saver (/s, /w)                                          */
/* ------------------------------------------------------------------ */

static int run_saver(HINSTANCE hi, const Settings *set, int windowed) {
    WNDCLASSA wc;
    memset(&wc, 0, sizeof wc);
    wc.lpfnWndProc = saver_proc; wc.hInstance = hi; wc.lpszClassName = "HVSSaver";
    wc.hCursor = LoadCursorA(NULL, IDC_ARROW);
    wc.style = CS_OWNDC;
    RegisterClassA(&wc);
    WNDCLASSA bc = wc;
    bc.lpfnWndProc = black_proc; bc.lpszClassName = "HVSBlack"; bc.style = 0;
    RegisterClassA(&bc);

    g_nmons = 0;
    EnumDisplayMonitors(NULL, NULL, mon_enum, 0);
    if (g_nmons == 0) {
        g_mons[0].rc.left = 0; g_mons[0].rc.top = 0;
        g_mons[0].rc.right = GetSystemMetrics(SM_CXSCREEN); g_mons[0].rc.bottom = GetSystemMetrics(SM_CYSCREEN);
        g_mons[0].primary = 1; g_nmons = 1;
    }
    int pick = 0;
    for (int i = 0; i < g_nmons; i++) if (g_mons[i].primary) pick = i;
    if (set->largest_monitor) {
        long best = 0;
        for (int i = 0; i < g_nmons; i++) {
            long a = (long)(g_mons[i].rc.right - g_mons[i].rc.left) * (g_mons[i].rc.bottom - g_mons[i].rc.top);
            if (a > best) { best = a; pick = i; }
        }
    }

    g_start_tick = GetTickCount();
    GetCursorPos(&g_mouse0); g_mouse0_set = 1;
    RECT r = g_mons[pick].rc;
    if (windowed) {                                /* 1: test window, 2: capture window */
        RECT adj = { 0, 0, 1440, 810 };
        AdjustWindowRect(&adj, WS_OVERLAPPEDWINDOW, FALSE);
        int ww = adj.right - adj.left, wh = adj.bottom - adj.top;
        g_win_rect.left = r.left + 80; g_win_rect.top = r.top + 60; g_win_rect.right = g_win_rect.left + ww; g_win_rect.bottom = g_win_rect.top + wh;
        g_main = CreateWindowExA(0, "HVSSaver", windowed == 2 ? APP_TITLE " (capture - Esc quits, F11 window)" : APP_TITLE " (test window - Esc quits, F11 full screen)",
                                 WS_OVERLAPPEDWINDOW | WS_VISIBLE, g_win_rect.left, g_win_rect.top, ww, wh,
                                 NULL, NULL, hi, NULL);
        if (g_main && windowed == 2) set_borderless(1);
    } else {
        g_main = CreateWindowExA(WS_EX_TOPMOST, "HVSSaver", APP_TITLE, WS_POPUP | WS_VISIBLE,
                                 r.left, r.top, r.right - r.left, r.bottom - r.top, NULL, NULL, hi, NULL);
        for (int i = 0; i < g_nmons; i++) {
            if (i == pick) continue;
            RECT b = g_mons[i].rc;
            CreateWindowExA(WS_EX_TOPMOST, "HVSBlack", APP_TITLE, WS_POPUP | WS_VISIBLE,
                            b.left, b.top, b.right - b.left, b.bottom - b.top, NULL, NULL, hi, NULL);
        }
        ShowCursor(FALSE);
    }
    if (!g_main) return 1;
    RECT cr; GetClientRect(g_main, &cr);
    g_disp.win_w = cr.right; g_disp.win_h = cr.bottom;
    SetForegroundWindow(g_main);

    display_open(&g_disp, g_main, getenv("HVS_NOGL") == NULL);   /* HVS_NOGL=1 forces the GDI path */
    g_disp.win_w = cr.right; g_disp.win_h = cr.bottom;

    RAWINPUTDEVICE rid;                             /* the mouse, raw, for turning while someone plays */
    rid.usUsagePage = 0x01; rid.usUsage = 0x02; rid.dwFlags = 0; rid.hwndTarget = g_main;
    RegisterRawInputDevices(&rid, 1, sizeof rid);

    pad_open(set->app.pad);                        /* a controller, if one is plugged in */
    snd_init();                                    /* the audio thread needs the synth ready */
    int audio = set->app.sound ? audio_start() : 0;
    slots_load();
    app_set_input(&g_in);
    app_init(&set->app, audio);                    /* audio: the device owns the sound clock */

    double prev = now_seconds();
    while (!g_quit) {
        pump_messages();
        if (g_quit) break;
        double t = now_seconds();
        double dt = t - prev; prev = t;
        if (dt > 0.1) dt = 0.1;
        pad_poll();
        app_update(dt);
        slots_flush();                                 /* a page signed, a death, the mark: keep it */
        if (app_quit_requested()) break;               /* fled to the surface: the ledger has what is kept */
        set_capture(app_control_state() == APP_PLAYER && GetForegroundWindow() == g_main);
        display_frame(&g_disp, 1);
        if (!g_disp.use_gl) Sleep(8);                  /* no vsync available: pace ourselves */
    }
    slots_flush();
    set_capture(0);
    pad_motors(0, 0);                              /* never left rumbling */

    audio_stop();
    display_close(&g_disp);
    DestroyWindow(g_main);
    if (!windowed) ShowCursor(TRUE);
    return 0;
}

/* ------------------------------------------------------------------ */
/* Preview (/p)                                                        */
/* ------------------------------------------------------------------ */

static int run_preview(HINSTANCE hi, HWND parent) {
    WNDCLASSA wc;
    memset(&wc, 0, sizeof wc);
    wc.lpfnWndProc = saver_proc; wc.hInstance = hi; wc.lpszClassName = "HVSPreview";
    wc.hCursor = LoadCursorA(NULL, IDC_ARROW); wc.style = CS_OWNDC;
    RegisterClassA(&wc);
    RECT r; GetClientRect(parent, &r);
    g_mode = MODE_PREVIEW;
    g_main = CreateWindowExA(0, "HVSPreview", "", WS_CHILD | WS_VISIBLE, 0, 0, r.right, r.bottom, parent, NULL, hi, NULL);
    if (!g_main) return 1;
    g_disp.win_w = r.right; g_disp.win_h = r.bottom;
    display_open(&g_disp, g_main, 0);
    g_disp.win_w = r.right; g_disp.win_h = r.bottom;

    Settings s; settings_load(&s);
    s.app.sound = 0; s.app.pixel_shift = 0; s.app.start_fade = 0;
    s.app.first_scene = s.app.scene_enabled[1] ? 1 : 0;
    s.app.scene_enabled[0] = s.app.scene_enabled[1] = 0;
    s.app.scene_enabled[s.app.first_scene] = 1;
    app_init(&s.app, 0);

    double prev = now_seconds();
    MSG m;
    while (!g_quit) {
        while (PeekMessageA(&m, NULL, 0, 0, PM_REMOVE)) {
            if (m.message == WM_QUIT) { g_quit = 1; break; }
            TranslateMessage(&m); DispatchMessageA(&m);
        }
        if (g_quit || !IsWindow(parent)) break;
        double t = now_seconds(), dt = t - prev; prev = t;
        if (dt > 0.2) dt = 0.2;
        app_update(dt);
        display_frame(&g_disp, 0);
        Sleep(40);
    }
    display_close(&g_disp);
    return 0;
}

/* ------------------------------------------------------------------ */
/* Settings dialog (/c)                                                */
/* ------------------------------------------------------------------ */

enum { ID_SOUND = 100, ID_VOL, ID_VOLTXT, ID_BRIGHT, ID_BRIGHTTXT, ID_SC0, ID_SC1, ID_STAY, ID_STAYTXT,
       ID_SHIFT, ID_MON, ID_PAD };

static Settings g_cfg;
static HWND g_cfgwnd, g_cfgparent;
static HFONT g_font;

static HWND ctl(const char *cls, const char *text, DWORD style, int x, int y, int w, int h, int id) {
    HWND c = CreateWindowExA(0, cls, text, WS_CHILD | WS_VISIBLE | style, x, y, w, h, g_cfgwnd, (HMENU)(INT_PTR)id,
                             GetModuleHandleA(NULL), NULL);
    SendMessageA(c, WM_SETFONT, (WPARAM)g_font, TRUE);
    return c;
}

static void cfg_update_labels(void) {
    char b[64];
    int v = (int)SendDlgItemMessageA(g_cfgwnd, ID_VOL, TBM_GETPOS, 0, 0);
    snprintf(b, sizeof b, "%d%%", v); SetDlgItemTextA(g_cfgwnd, ID_VOLTXT, b);
    v = (int)SendDlgItemMessageA(g_cfgwnd, ID_BRIGHT, TBM_GETPOS, 0, 0);
    snprintf(b, sizeof b, "%d%%", v); SetDlgItemTextA(g_cfgwnd, ID_BRIGHTTXT, b);
    v = (int)SendDlgItemMessageA(g_cfgwnd, ID_STAY, TBM_GETPOS, 0, 0);
    snprintf(b, sizeof b, "%d min", v); SetDlgItemTextA(g_cfgwnd, ID_STAYTXT, b);
}

static void cfg_apply_and_close(void) {
    g_cfg.app.sound = IsDlgButtonChecked(g_cfgwnd, ID_SOUND) == BST_CHECKED;
    g_cfg.app.volume = (int)SendDlgItemMessageA(g_cfgwnd, ID_VOL, TBM_GETPOS, 0, 0);
    g_cfg.app.brightness = (int)SendDlgItemMessageA(g_cfgwnd, ID_BRIGHT, TBM_GETPOS, 0, 0);
    g_cfg.app.stay_seconds = 60 * (int)SendDlgItemMessageA(g_cfgwnd, ID_STAY, TBM_GETPOS, 0, 0);
    g_cfg.app.scene_enabled[0] = IsDlgButtonChecked(g_cfgwnd, ID_SC0) == BST_CHECKED;
    g_cfg.app.scene_enabled[1] = IsDlgButtonChecked(g_cfgwnd, ID_SC1) == BST_CHECKED;
    if (!g_cfg.app.scene_enabled[0] && !g_cfg.app.scene_enabled[1]) g_cfg.app.scene_enabled[0] = 1;
    g_cfg.app.pixel_shift = IsDlgButtonChecked(g_cfgwnd, ID_SHIFT) == BST_CHECKED;
    g_cfg.largest_monitor = (int)SendDlgItemMessageA(g_cfgwnd, ID_MON, CB_GETCURSEL, 0, 0) == 1;
    g_cfg.app.pad = IsDlgButtonChecked(g_cfgwnd, ID_PAD) == BST_CHECKED;
    settings_save(&g_cfg);
    DestroyWindow(g_cfgwnd);
}

static LRESULT CALLBACK cfg_proc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_HSCROLL: cfg_update_labels(); return 0;
    case WM_COMMAND:
        if (LOWORD(wp) == IDOK) { cfg_apply_and_close(); return 0; }
        if (LOWORD(wp) == IDCANCEL) { DestroyWindow(hwnd); return 0; }
        break;
    case WM_CLOSE: DestroyWindow(hwnd); return 0;
    case WM_DESTROY:
        if (g_cfgparent) EnableWindow(g_cfgparent, TRUE);
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcA(hwnd, msg, wp, lp);
}

static int run_config(HINSTANCE hi, HWND parent) {
    INITCOMMONCONTROLSEX icc = { sizeof icc, ICC_BAR_CLASSES };
    InitCommonControlsEx(&icc);
    settings_load(&g_cfg);
    g_font = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
    g_cfgparent = parent;

    WNDCLASSA wc;
    memset(&wc, 0, sizeof wc);
    wc.lpfnWndProc = cfg_proc; wc.hInstance = hi; wc.lpszClassName = "HVSConfig";
    wc.hCursor = LoadCursorA(NULL, IDC_ARROW); wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    RegisterClassA(&wc);

    const int W = 440, H = 402;
    RECT r = { 0, 0, W, H };
    AdjustWindowRect(&r, WS_CAPTION | WS_SYSMENU, FALSE);
    int ww = r.right - r.left, wh = r.bottom - r.top;
    int sx = (GetSystemMetrics(SM_CXSCREEN) - ww) / 2, sy = (GetSystemMetrics(SM_CYSCREEN) - wh) / 2;
    if (parent) {
        RECT pr; GetWindowRect(parent, &pr);
        sx = pr.left + ((pr.right - pr.left) - ww) / 2; sy = pr.top + ((pr.bottom - pr.top) - wh) / 2;
    }
    g_cfgwnd = CreateWindowExA(WS_EX_DLGMODALFRAME, "HVSConfig", APP_TITLE " Settings",
                               WS_CAPTION | WS_SYSMENU | WS_VISIBLE, sx, sy, ww, wh, parent, NULL, hi, NULL);
    if (!g_cfgwnd) return 1;
    if (parent) EnableWindow(parent, FALSE);

    int y = 14;
    ctl("BUTTON", "Play sound (FM synth music and effects)", BS_AUTOCHECKBOX | WS_TABSTOP, 16, y, 400, 20, ID_SOUND);
    CheckDlgButton(g_cfgwnd, ID_SOUND, g_cfg.app.sound ? BST_CHECKED : BST_UNCHECKED);
    y += 30;
    ctl("STATIC", "Volume", 0, 16, y + 4, 80, 18, 0);
    HWND t = ctl(TRACKBAR_CLASSA, "", TBS_AUTOTICKS | WS_TABSTOP, 100, y, 250, 30, ID_VOL);
    SendMessageA(t, TBM_SETRANGE, TRUE, MAKELONG(0, 100)); SendMessageA(t, TBM_SETTICFREQ, 10, 0);
    SendMessageA(t, TBM_SETPOS, TRUE, g_cfg.app.volume);
    ctl("STATIC", "", 0, 360, y + 4, 60, 18, ID_VOLTXT);
    y += 38;
    ctl("STATIC", "Brightness", 0, 16, y + 4, 80, 18, 0);
    t = ctl(TRACKBAR_CLASSA, "", TBS_AUTOTICKS | WS_TABSTOP, 100, y, 250, 30, ID_BRIGHT);
    SendMessageA(t, TBM_SETRANGE, TRUE, MAKELONG(30, 100)); SendMessageA(t, TBM_SETTICFREQ, 10, 0);
    SendMessageA(t, TBM_SETPOS, TRUE, g_cfg.app.brightness);
    ctl("STATIC", "", 0, 360, y + 4, 60, 18, ID_BRIGHTTXT);
    y += 42;
    ctl("BUTTON", "Scenes", BS_GROUPBOX, 16, y, 408, 78, 0);
    ctl("BUTTON", "HAUNT.EXE  -  a possessed PC (80x25 text mode)", BS_AUTOCHECKBOX | WS_TABSTOP, 30, y + 20, 380, 20, ID_SC0);
    ctl("BUTTON", "CATACOMB  -  a Catacomb 3-D style crypt crawl (mode 13h)", BS_AUTOCHECKBOX | WS_TABSTOP, 30, y + 46, 380, 20, ID_SC1);
    CheckDlgButton(g_cfgwnd, ID_SC0, g_cfg.app.scene_enabled[0] ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(g_cfgwnd, ID_SC1, g_cfg.app.scene_enabled[1] ? BST_CHECKED : BST_UNCHECKED);
    y += 90;
    ctl("STATIC", "Switch every", 0, 16, y + 4, 80, 18, 0);
    t = ctl(TRACKBAR_CLASSA, "", TBS_AUTOTICKS | WS_TABSTOP, 100, y, 250, 30, ID_STAY);
    SendMessageA(t, TBM_SETRANGE, TRUE, MAKELONG(1, 15)); SendMessageA(t, TBM_SETTICFREQ, 1, 0);
    SendMessageA(t, TBM_SETPOS, TRUE, g_cfg.app.stay_seconds / 60 < 1 ? 1 : g_cfg.app.stay_seconds / 60);
    ctl("STATIC", "", 0, 360, y + 4, 60, 18, ID_STAYTXT);
    y += 38;
    ctl("BUTTON", "Slowly drift the picture (helps prevent OLED burn-in)", BS_AUTOCHECKBOX | WS_TABSTOP, 16, y, 408, 20, ID_SHIFT);
    CheckDlgButton(g_cfgwnd, ID_SHIFT, g_cfg.app.pixel_shift ? BST_CHECKED : BST_UNCHECKED);
    y += 30;
    ctl("BUTTON", "Game controller (Xbox and compatible): hold any button to play", BS_AUTOCHECKBOX | WS_TABSTOP, 16, y, 408, 20, ID_PAD);
    CheckDlgButton(g_cfgwnd, ID_PAD, g_cfg.app.pad ? BST_CHECKED : BST_UNCHECKED);
    y += 30;
    ctl("STATIC", "Show on", 0, 16, y + 4, 60, 18, 0);
    HWND cb = ctl("COMBOBOX", "", CBS_DROPDOWNLIST | WS_TABSTOP | WS_VSCROLL, 100, y, 320, 100, ID_MON);
    SendMessageA(cb, CB_ADDSTRING, 0, (LPARAM)"The primary monitor");
    SendMessageA(cb, CB_ADDSTRING, 0, (LPARAM)"The largest monitor (other screens stay black)");
    SendMessageA(cb, CB_SETCURSEL, g_cfg.largest_monitor ? 1 : 0, 0);
    y += 38;
    ctl("BUTTON", "OK", BS_DEFPUSHBUTTON | WS_TABSTOP, 252, y, 82, 26, IDOK);
    ctl("BUTTON", "Cancel", BS_PUSHBUTTON | WS_TABSTOP, 342, y, 82, 26, IDCANCEL);
    ctl("STATIC", "HAUNTED.EXE " HVS_VERSION ". VGA font bitmaps: Px437 IBM fonts by VileR (CC BY-SA 4.0).", SS_LEFT, 16, y - 2, 230, 44, 0);
    cfg_update_labels();

    MSG m;
    while (GetMessageA(&m, NULL, 0, 0) > 0) {
        if (!IsDialogMessageA(g_cfgwnd, &m)) { TranslateMessage(&m); DispatchMessageA(&m); }
    }
    return 0;
}

/* ------------------------------------------------------------------ */

int WINAPI WinMain(HINSTANCE hi, HINSTANCE hprev, LPSTR cmd, int show) {
    (void)hprev; (void)show;
    enable_dpi_awareness();
    timeBeginPeriod(1);
    g_dump_path = getenv("HVS_DUMP");
    g_gltest = getenv("HVS_GLTEST") ? atoi(getenv("HVS_GLTEST")) : 0;

    /* argument: first token after the program name; accepts /s -s /c:123 /p 123 */
    const char *a = cmd ? cmd : "";
    while (*a == ' ') a++;
    char flag = 0;
    HWND hw = NULL;
    if ((*a == '/' || *a == '-') && a[1]) {
        flag = (char)((a[1] >= 'A' && a[1] <= 'Z') ? a[1] + 32 : a[1]);
        const char *p = a + 2;
        while (*p == ':' || *p == ' ') p++;
        if (*p) hw = (HWND)(UINT_PTR)_strtoui64(p, NULL, 10);
    }

    if (!flag) {                                   /* started as HAUNTED.EXE: just run it */
        char me[MAX_PATH];
        DWORD n = GetModuleFileNameA(NULL, me, sizeof me);
        if (n > 4 && n < sizeof me && _stricmp(me + n - 4, ".exe") == 0) flag = 'w';
    }

    int rc = 0;
    switch (flag) {
    case 's': {
        HANDLE mtx = CreateMutexA(NULL, TRUE, "HalloweenVGASaver_Instance");
        if (GetLastError() == ERROR_ALREADY_EXISTS) { timeEndPeriod(1); return 0; }
        Settings s; settings_load(&s);
        g_mode = MODE_SAVER;
        rc = run_saver(hi, &s, 0);
        if (mtx) CloseHandle(mtx);
        break;
    }
    case 'w': {
        Settings s; settings_load(&s);
        g_mode = MODE_WINDOWED;
        s.app.pixel_shift = 0;
        rc = run_saver(hi, &s, 1);
        break;
    }
    case 'f': {                                    /* capture: a steady picture, no drift */
        Settings s; settings_load(&s);
        g_mode = MODE_CAPTURE;
        s.app.pixel_shift = 0;
        rc = run_saver(hi, &s, 2);
        break;
    }
    case 'p':
        if (hw && IsWindow(hw)) rc = run_preview(hi, hw);
        break;
    case 'c':
    default:
        g_mode = MODE_CONFIG;
        rc = run_config(hi, (hw && IsWindow(hw)) ? hw : NULL);
        break;
    }
    timeEndPeriod(1);
    return rc;
}
