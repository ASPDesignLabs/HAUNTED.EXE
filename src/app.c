#include "app.h"
#include "snd.h"
#include "gfx.h"

#define FADE_SECONDS 1.4

static const Scene *const SCENES[SCENE_COUNT] = { &scene_haunt, &scene_crawl };

typedef enum { PH_FADEIN, PH_RUN, PH_FADEOUT } Phase;

static struct {
    AppConfig cfg;
    int audio_external;
    int cur;
    Phase phase;
    double phase_t, scene_t, total_t;
    uint32_t seed;
    Frame frame;
    Input *in;
    int control, quit;              /* a person has the scene; they chose to leave */
    double hold_t, hold_show, grace;   /* H held this long; the bar as drawn; a moment's calm after handing back */
} A;

void app_default_config(AppConfig *c) {
    memset(c, 0, sizeof *c);
    c->sound = 1;
    c->volume = 60;
    c->brightness = 85;
    c->scene_enabled[0] = c->scene_enabled[1] = 1;
    c->stay_seconds = 240;
    c->pixel_shift = 1;
    c->first_scene = 0;
    c->start_fade = 1;
}

static void enter_scene(int idx) {
    const Scene *sc = SCENES[idx];
    A.cur = idx;
    A.frame.w = sc->w; A.frame.h = sc->h;
    A.frame.fade = 1.0f;
    memset(A.frame.pix, 0, sizeof A.frame.pix);
    memset(A.frame.pal, 0, sizeof A.frame.pal);
    A.seed = A.seed * 1664525u + 1013904223u;
    sc->enter(A.seed);
    snd_start_program(sc->sound_program);
    snd_fade_master(1.0f, (float)FADE_SECONDS);
    A.scene_t = 0;
}

static int next_enabled(int from) {
    for (int i = 1; i <= SCENE_COUNT; i++) {
        int n = (from + i) % SCENE_COUNT;
        if (A.cfg.scene_enabled[n]) return n;
    }
    return from;
}

void app_init(const AppConfig *cfg, int audio_driven_externally) {
    A.cfg = *cfg;
    A.audio_external = audio_driven_externally;
    A.total_t = 0;
    A.seed = 0xC0FFEEu ^ (uint32_t)(cfg->first_scene * 7919);
    snd_init();
    snd_set_volume(cfg->sound ? cfg->volume / 100.0f : 0.0f);
    int first = cfg->first_scene;
    if (first < 0 || first >= SCENE_COUNT || !cfg->scene_enabled[first])
        first = cfg->scene_enabled[0] ? 0 : 1;
    enter_scene(first);
    A.phase = cfg->start_fade ? PH_FADEIN : PH_RUN;
    A.phase_t = 0;
}

static int playable_scene(void) {
    for (int i = 0; i < SCENE_COUNT; i++) if (SCENES[i]->control) return i;
    return -1;
}

/* H held long enough: the crypt is yours (from HAUNT.EXE too, cutting straight to it) */
static void take_control(void) {
    int p = playable_scene();
    if (p < 0) return;
    if (A.cur != p) {
        enter_scene(p);
        A.phase = PH_FADEIN; A.phase_t = FADE_SECONDS * 0.6;
    } else if (A.phase == PH_FADEOUT) {                    /* it was leaving: it stays */
        A.phase = PH_RUN; A.phase_t = 0;
        snd_fade_master(1.0f, 0.3f);
    }
    SCENES[p]->control(CTRL_TAKE);
    A.control = 1; A.hold_t = 0; A.scene_t = 0;
}

void app_update(double dt) {
    if (dt > 0.25) dt = 0.25;
    A.total_t += dt;
    A.phase_t += dt;
    A.scene_t += dt;
    if (A.grace > 0) A.grace -= dt;

    if (A.in && !A.control && !A.quit) {
        if (A.in->down[KEY_H] && playable_scene() >= 0) {
            A.hold_t += dt;
            if (A.cur == playable_scene() && SCENES[A.cur]->control) SCENES[A.cur]->control(CTRL_PEEK);   /* a look at where things stand */
            if (A.hold_t >= HOLD_SECONDS) take_control();
        } else A.hold_t = 0;
    }
    A.hold_show = A.hold_t > A.hold_show ? A.hold_t : (A.hold_show - dt * 4.0 > 0 ? A.hold_show - dt * 4.0 : 0);

    const Scene *sc = SCENES[A.cur];
    if (A.control && sc->input && A.in) sc->input(A.in, dt);
    sc->update(dt);
    if (!A.audio_external) snd_advance(dt);
    if (A.control && sc->status) {
        int st = sc->status();
        if (st == PLAY_AUTO) { A.control = 0; A.scene_t = 0; A.grace = 1.5; }   /* back to the autopilot */
        else if (st == PLAY_QUIT) A.quit = 1;
    }
    if (A.in) { memset(A.in->hit, 0, sizeof A.in->hit); A.in->mdx = A.in->mdy = 0.0f; A.in->wheel = 0; A.in->ntext = 0; }

    switch (A.phase) {
    case PH_FADEIN:
        if (A.phase_t >= FADE_SECONDS) { A.phase = PH_RUN; A.phase_t = 0; }
        break;
    case PH_RUN: {
        int others = next_enabled(A.cur) != A.cur;
        if (others && A.scene_t >= A.cfg.stay_seconds && !A.control) {
            A.phase = PH_FADEOUT; A.phase_t = 0;
            snd_fade_master(0.0f, (float)FADE_SECONDS);
        }
        break;
    }
    case PH_FADEOUT:
        if (A.phase_t >= FADE_SECONDS) {
            enter_scene(next_enabled(A.cur));
            A.phase = PH_FADEIN; A.phase_t = 0;
        }
        break;
    }
}

int app_mode(int *w, int *h) { *w = A.frame.w; *h = A.frame.h; return A.cur; }
const char *app_scene_name(void) { return SCENES[A.cur]->name; }

static float transition_fade(void) {
    switch (A.phase) {
    case PH_FADEIN:  return (float)smoothstepf(0.0f, (float)FADE_SECONDS, (float)A.phase_t);
    case PH_FADEOUT: return 1.0f - (float)smoothstepf(0.0f, (float)FADE_SECONDS, (float)A.phase_t);
    default:         return 1.0f;
    }
}

static void build_lut(uint32_t lut[256], int bgr) {
    float k = A.frame.fade * transition_fade() * (A.cfg.brightness / 100.0f);
    for (int i = 0; i < 256; i++) {
        int c[3];
        for (int j = 0; j < 3; j++) {
            int v6 = A.frame.pal[i][j] & 63;
            int v8 = (v6 << 2) | (v6 >> 4);          /* VGA DAC 6 -> 8 bits */
            c[j] = clampi((int)(v8 * k + 0.5f), 0, 255);
        }
        lut[i] = bgr ? (uint32_t)((c[0] << 16) | (c[1] << 8) | c[2])
                     : (uint32_t)(c[0] | (c[1] << 8) | (c[2] << 16) | (0xFFu << 24));
    }
}

/* "HOLD H" and a bar filling over three seconds, drawn over whichever scene is up */
static void hold_overlay(uint32_t *dst, int bgr) {
    if (A.hold_show <= 0.0 || A.control) return;
    int w = A.frame.w, h = A.frame.h, s = w > 400 ? 2 : 1;
    const char *msg = A.cur == playable_scene() ? "HOLD H TO TAKE CONTROL" : "HOLD H TO ENTER THE CRYPT";
    int tw = (int)strlen(msg) * 8 * s, bw = tw, bh = 4 * s;
    int x0 = (w - tw) / 2, y0 = h - 30 * s;
    float k = (float)(A.hold_show / HOLD_SECONDS); if (k > 1.0f) k = 1.0f;
    float br = A.cfg.brightness / 100.0f;
    #define RGBX(r, g, b) (bgr ? (uint32_t)(((int)((r) * br) << 16) | ((int)((g) * br) << 8) | (int)((b) * br)) \
                              : (uint32_t)((int)((r) * br) | ((int)((g) * br) << 8) | ((int)((b) * br) << 16) | 0xFF000000u))
    for (int y = y0 - 4 * s; y < y0 + 8 * s + 6 * s + bh; y++)       /* a dark plate behind it */
        for (int x = x0 - 6 * s; x < x0 + tw + 6 * s; x++)
            if (x >= 0 && y >= 0 && x < w && y < h) dst[y * w + x] = RGBX(8, 6, 12);
    for (size_t c = 0; msg[c]; c++)
        for (int gy = 0; gy < 8; gy++)
            for (int gx = 0; gx < 8; gx++)
                if (font_bios8[(unsigned char)msg[c]][gy] & (0x80 >> gx))
                    for (int yy = 0; yy < s; yy++) for (int xx = 0; xx < s; xx++)
                        dst[(y0 + gy * s + yy) * w + x0 + ((int)c * 8 + gx) * s + xx] = RGBX(230, 220, 200);
    int by = y0 + 12 * s;
    for (int y = by; y < by + bh; y++)
        for (int x = x0; x < x0 + bw; x++)
            dst[y * w + x] = x < x0 + (int)(bw * k) ? RGBX(232, 110, 24) : RGBX(48, 40, 56);
    #undef RGBX
}

void app_present(uint32_t *dst, int bgr) {
    uint32_t lut[256];
    SCENES[A.cur]->render(&A.frame);
    build_lut(lut, bgr);
    int n = A.frame.w * A.frame.h;
    const uint8_t *src = A.frame.pix;
    for (int i = 0; i < n; i++) dst[i] = lut[src[i]];
    hold_overlay(dst, bgr);
}

void app_set_input(Input *in) { A.in = in; }
int app_control_state(void) { return A.control ? APP_PLAYER : (A.hold_t > 0.0 || A.grace > 0.0 ? APP_HOLDING : APP_AUTO); }
void app_focus(int on) { if (!on && A.control && SCENES[A.cur]->control) SCENES[A.cur]->control(CTRL_PAUSE); }
int app_quit_requested(void) { return A.quit; }
int app_save(uint8_t *buf, int cap) { int p = playable_scene(); return p >= 0 && SCENES[p]->save ? SCENES[p]->save(buf, cap) : 0; }
void app_load(const uint8_t *buf, int len) { int p = playable_scene(); if (p >= 0 && SCENES[p]->load && buf && len > 0) SCENES[p]->load(buf, len); }

static struct { uint8_t data[SLOT_MAX]; int len, dirty; } SLOTS[SLOT_COUNT];
static void slot_put(int i, const uint8_t *buf, int len, int dirty) {
    if (i < 0 || i >= SLOT_COUNT || !buf || len <= 0 || len > SLOT_MAX) return;
    memcpy(SLOTS[i].data, buf, (size_t)len); SLOTS[i].len = len; SLOTS[i].dirty = dirty;
}
int slot_read(int i, uint8_t *buf, int cap) {
    if (i < 0 || i >= SLOT_COUNT || SLOTS[i].len <= 0 || SLOTS[i].len > cap) return 0;
    memcpy(buf, SLOTS[i].data, (size_t)SLOTS[i].len);
    return SLOTS[i].len;
}
void slot_write(int i, const uint8_t *buf, int len) { slot_put(i, buf, len, 1); }
void app_slot_load(int i, const uint8_t *buf, int len) { slot_put(i, buf, len, 0); }
int app_slot_take(int i, uint8_t *buf, int cap) {
    if (i < 0 || i >= SLOT_COUNT || !SLOTS[i].dirty) return 0;
    SLOTS[i].dirty = 0;
    return slot_read(i, buf, cap);
}

void app_present_rgb(uint8_t *rgb, int *w, int *h) {
    static uint32_t buf[FB_MAX_W * FB_MAX_H];
    app_present(buf, 0);
    *w = A.frame.w; *h = A.frame.h;
    for (int i = 0; i < A.frame.w * A.frame.h; i++) {
        rgb[i * 3 + 0] = (uint8_t)(buf[i] & 255);
        rgb[i * 3 + 1] = (uint8_t)((buf[i] >> 8) & 255);
        rgb[i * 3 + 2] = (uint8_t)((buf[i] >> 16) & 255);
    }
}

void app_pixel_shift(int *dx, int *dy) {
    if (!A.cfg.pixel_shift) { *dx = *dy = 0; return; }
    /* slow Lissajous drift: about one source pixel every 8 s at most */
    *dx = (int)floorf(6.0f * sinf((float)(A.total_t / 311.0) * TAU_F) + 0.5f);
    *dy = (int)floorf(5.0f * sinf((float)(A.total_t / 457.0) * TAU_F + 1.0f) + 0.5f);
}
