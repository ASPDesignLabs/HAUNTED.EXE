/* The browser's shell, the C side (docs/PLAN-web.md). web/haunted.js drives it: it fills the input, steps the
 * app each animation frame, takes the frame as RGBA bytes, keeps the sound worklet fed from snd_render, and
 * keeps the slots in localStorage. Everything here is a thin door onto app.h, as tools/host.c is. Built with
 * Emscripten and HVS_WEB (make web). */
#include <emscripten.h>
#include <string.h>
#include "app.h"
#include "snd.h"

#define KEEP EMSCRIPTEN_KEEPALIVE

static Input g_in;
static uint32_t g_pix[FB_MAX_W * FB_MAX_H];
static int16_t g_pcm[8192 * 2];
static float g_pcmf[8192 * 2];
static uint8_t g_slot[SLOT_MAX];
static AppConfig g_cfg;
static int g_started;

/* the keys' and buttons' names in core.h's order, space-separated, so the page finds their numbers by name */
static const char *const KEYNAME[] = { "UP", "DOWN", "LEFT", "RIGHT", "W", "A", "S", "D", "E", "Q", "SPACE", "CTRL",
    "SHIFT", "TAB", "ESC", "ENTER", "H", "Y", "N", "1", "2", "3", "4", "5", "6", "7", "8", "9", "MOUSE1", "MOUSE2", "B", "L",
    "CONSOLE", "BACK", "PGUP", "PGDN", "C", "M" };
static const char *const PADNAME[] = { "A", "B", "X", "Y", "LB", "RB", "VIEW", "MENU", "LS", "RS", "UP", "DOWN", "LEFT",
    "RIGHT", "LT", "RT" };
_Static_assert(sizeof KEYNAME / sizeof KEYNAME[0] == KEY_COUNT, "a key without a name");
_Static_assert(sizeof PADNAME / sizeof PADNAME[0] == PAD_COUNT, "a button without a name");
static char g_names[512];
static const char *names(const char *const *n, int c) {
    g_names[0] = 0;
    for (int i = 0; i < c; i++) { if (i) strcat(g_names, " "); strcat(g_names, n[i]); }
    return g_names;
}
KEEP const char *web_key_names(void) { return names(KEYNAME, KEY_COUNT); }
KEEP const char *web_pad_names(void) { return names(PADNAME, PAD_COUNT); }
KEEP int web_slots(void) { return SLOT_ALL; }
KEEP int web_slot_index(int which) { return which == 0 ? SLOT_PROGRESS : which == 1 ? SLOT_MARK : SLOT_PREFS; }

/* the settings, before web_start: the /c dialog's, by the registry's names */
KEEP void web_config(int sound, int volume, int brightness, int haunt, int crawl, int stay, int shift, int pad, int music, int effects) {
    app_default_config(&g_cfg);
    g_cfg.sound = sound; g_cfg.volume = clampi(volume, 0, 100); g_cfg.brightness = clampi(brightness, 30, 100);
    g_cfg.scene_enabled[0] = haunt; g_cfg.scene_enabled[1] = crawl;
    if (!haunt && !crawl) g_cfg.scene_enabled[0] = 1;
    g_cfg.stay_seconds = stay < 30 ? 30 : stay; g_cfg.pixel_shift = shift; g_cfg.pad = pad;
    g_cfg.music = clampi(music, 0, 100); g_cfg.effects = clampi(effects, 0, 100);
    g_cfg.first_scene = g_cfg.scene_enabled[0] ? 0 : 1;
}
/* a slot from storage, before web_start: the page puts its bytes in web_slot_buf first */
KEEP uint8_t *web_slot_buf(void) { return g_slot; }
KEEP void web_slot_load(int i, int len) { app_slot_load(i, g_slot, len); }
/* a slot the game wrote since last asked: its bytes in web_slot_buf, its length, or 0 */
KEEP int web_slot_take(int i) { return app_slot_take(i, g_slot, sizeof g_slot); }

/* a drawn level from the world editor's export, before web_start (its PLAY, docs/PLAN-mapedit.md): the page puts the text's
 * bytes in web_drawn_buf; then "" if it was read, else why not */
int crawl_drawn_text(const char *text, char *err, int nerr);
static char g_dtxt[262144], g_derr[256];
KEEP char *web_drawn_buf(void) { return g_dtxt; }
KEEP int web_drawn_cap(void) { return (int)sizeof g_dtxt; }
KEEP const char *web_drawn_load(int len) {
    g_dtxt[len < 0 ? 0 : len < (int)sizeof g_dtxt ? len : (int)sizeof g_dtxt - 1] = 0;
    return crawl_drawn_text(g_dtxt, g_derr, sizeof g_derr) ? g_derr : "";
}

/* a side passage from the world editor (docs/PLAN-side.md), before web_start: its text in web_side_buf, then web_side_load with its
 * number; "" if it was read, else why not. web_side_now: a new run begins on that level and goes straight into passage n */   /* side (PLAN-side) */
int crawl_side_text(int n, const char *text, char *err, int nerr);
void crawl_side_now(int n, int level);
static char g_stxt[65536], g_serr[256];
KEEP char *web_side_buf(void) { return g_stxt; }
KEEP int web_side_cap(void) { return (int)sizeof g_stxt; }
KEEP const char *web_side_load(int n, int len) {
    g_stxt[len < 0 ? 0 : len < (int)sizeof g_stxt ? len : (int)sizeof g_stxt - 1] = 0;
    return crawl_side_text(n, g_stxt, g_serr, sizeof g_serr) ? g_serr : "";
}
KEEP void web_side_now(int n, int level) { crawl_side_now(n, level); }
void crawl_side_tune(int k, float v);
KEEP void web_side_tune(int k, float v) { crawl_side_tune(k, v); }   /* the editor's sliders while it plays */

/* the generator's lights on the drawing loaded (docs/PLAN-lights.md): after web_drawn_load, with or without web_start (the editor
 * loads this module in its own page only to ask). A static array: how many, then style (DL_*), x, y for each, in the drawing's cells;
 * the count -1 if no drawing was read, or if there was no memory to keep the game's state while it is laid (about 3 MB; not seen) */
typedef struct { int style, x, y; } DrawnLight;                    /* as the game's */
int crawl_drawn_lights(DrawnLight *out, int max);
#define WEB_LIGHTS 256
KEEP int *web_drawn_lights(void) {
    static int a[1 + 3 * WEB_LIGHTS];
    static DrawnLight L[WEB_LIGHTS];
    int n = crawl_drawn_lights(L, WEB_LIGHTS);
    a[0] = n < 0 ? -1 : n;
    for (int i = 0; i < n; i++) { a[1 + 3 * i] = L[i].style; a[2 + 3 * i] = L[i].x; a[3 + 3 * i] = L[i].y; }
    return a;
}

KEEP void web_start(void) {
    if (g_started) return;
    g_started = 1;
    app_set_input(&g_in);
    app_init(&g_cfg, 1);                                           /* the sound card's clock is the music's, as on Windows */
}

/* input: a key (KEY_*), the mouse, the wheel, a letter or digit typed; all of it let go when the page loses focus */
KEEP void web_key(int k, int down) {
    if (k < 0 || k >= KEY_COUNT) return;
    if (down && !g_in.down[k]) g_in.hit[k] = 1;
    g_in.down[k] = (uint8_t)(down != 0);
}
KEEP void web_mouse(float dx, float dy) { g_in.mdx += dx; g_in.mdy += dy; }
KEEP void web_wheel(int n) { g_in.wheel += n; }
KEEP void web_text(int c) {
    if (c >= 'a' && c <= 'z') c -= 32;
    if (((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9')) && g_in.ntext < (int)sizeof g_in.text) g_in.text[g_in.ntext++] = (char)c;
}
KEEP void web_release(void) {                                     /* all let go; the pad, touch and the stage stay as they are */
    Input keep = g_in;
    memset(&g_in, 0, sizeof g_in);
    g_in.pad = keep.pad; g_in.touch = keep.touch;
    g_in.stage_w = keep.stage_w; g_in.stage_h = keep.stage_h; g_in.pic_x = keep.pic_x; g_in.pic_y = keep.pic_y;
    g_in.safe_l = keep.safe_l; g_in.safe_t = keep.safe_t; g_in.safe_r = keep.safe_r; g_in.safe_b = keep.safe_b;
}

/* touch (docs/PLAN-mobile.md): playing by touch or not; the stage, in the picture's pixels, and what the device keeps at its
 * edges; a finger by its id, where it is on the stage, and whether it landed (0), moved (1) or lifted (2) */
KEEP void web_touch_mode(int on) { g_in.touch = on; if (!on) { g_in.nfinger = 0; g_in.stage_w = g_in.stage_h = 0; } }
KEEP void web_stage(int sw, int sh, int l, int t, int r, int b) {
    g_in.stage_w = sw; g_in.stage_h = sh; g_in.safe_l = l; g_in.safe_t = t; g_in.safe_r = r; g_in.safe_b = b;
}
KEEP void web_touch(int id, float x, float y, int phase) {
    int i = 0;
    while (i < g_in.nfinger && g_in.finger[i].id != id) i++;
    if (i == g_in.nfinger) {
        if (phase != 0 || g_in.nfinger >= TOUCH_MAX) return;       /* only a landing begins a finger */
        g_in.finger[i] = (Finger){ id, x, y, 1, 0 };
        g_in.nfinger++;
        return;
    }
    g_in.finger[i].x = x; g_in.finger[i].y = y;
    if (phase == 2) g_in.finger[i].up = 1;
}
static uint32_t g_spix[FB_MAX_W * FB_MAX_H];
KEEP uint32_t *web_stage_frame(void) {                             /* the stage as RGBA bytes, web_stage's size; 0 if it couldn't */
    return app_present_stage(g_spix, g_in.stage_w, g_in.stage_h, 0) ? g_spix : 0;
}
/* the pad: buttons a bit each (PAD_*), the sticks past their dead zones, y up, the triggers 0..1; present 0 when none */
KEEP void web_pad(int present, int buttons, float lx, float ly, float rx, float ry, float lt, float rt) {
    g_in.pad = present;
    for (int b = 0; b < PAD_COUNT; b++) {
        int d = present && ((buttons >> b) & 1);
        if (d && !g_in.pad_down[b]) g_in.pad_hit[b] = 1;
        g_in.pad_down[b] = (uint8_t)d;
    }
    g_in.lx = lx; g_in.ly = ly; g_in.rx = rx; g_in.ry = ry; g_in.lt = lt; g_in.rt = rt;
}

/* a step; then 1 if the player chose to flee to the surface */
KEEP int web_step(double dt) {
    if (!g_started) return 0;
    if (dt > 0.1) dt = 0.1;
    app_update(dt);
    return app_quit_requested();
}
KEEP int web_state(void) { return app_control_state(); }           /* APP_AUTO, APP_HOLDING, APP_PLAYER */
KEEP void web_focus(int on) { app_focus(on); if (!on) web_release(); }

/* the frame: RGBA bytes, web_width x web_height */
KEEP uint32_t *web_frame(void) { app_present(g_pix, 0); return g_pix; }
KEEP int web_width(void) { int w, h; app_mode(&w, &h); return w; }
KEEP int web_height(void) { int w, h; app_mode(&w, &h); return h; }
KEEP int web_shift_x(void) { int x, y; app_pixel_shift(&x, &y); return x; }
KEEP int web_shift_y(void) { int x, y; app_pixel_shift(&x, &y); return y; }

/* sound: n frames of stereo at SND_RATE, as floats, interleaved */
KEEP float *web_audio(int n) {
    if (n > 8192) n = 8192;
    snd_render(g_pcm, n);
    for (int i = 0; i < n * 2; i++) g_pcmf[i] = g_pcm[i] * (1.0f / 32768.0f);
    return g_pcmf;
}
KEEP int web_rate(void) { return SND_RATE; }

/* the rumble asked for now, 0..1 each, a person's only */
KEEP float web_motor(int high) {
    float lo = 0.0f, hi = 0.0f;
    if (app_control_state() == APP_PLAYER) app_pad_motors(&lo, &hi);
    return high ? hi : lo;
}
