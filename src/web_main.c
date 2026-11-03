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
KEEP void web_config(int sound, int volume, int brightness, int haunt, int crawl, int stay, int shift, int pad) {
    app_default_config(&g_cfg);
    g_cfg.sound = sound; g_cfg.volume = clampi(volume, 0, 100); g_cfg.brightness = clampi(brightness, 30, 100);
    g_cfg.scene_enabled[0] = haunt; g_cfg.scene_enabled[1] = crawl;
    if (!haunt && !crawl) g_cfg.scene_enabled[0] = 1;
    g_cfg.stay_seconds = stay < 30 ? 30 : stay; g_cfg.pixel_shift = shift; g_cfg.pad = pad;
    g_cfg.first_scene = g_cfg.scene_enabled[0] ? 0 : 1;
}
/* a slot from storage, before web_start: the page puts its bytes in web_slot_buf first */
KEEP uint8_t *web_slot_buf(void) { return g_slot; }
KEEP void web_slot_load(int i, int len) { app_slot_load(i, g_slot, len); }
/* a slot the game wrote since last asked: its bytes in web_slot_buf, its length, or 0 */
KEEP int web_slot_take(int i) { return app_slot_take(i, g_slot, sizeof g_slot); }

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
KEEP void web_release(void) {
    int pad = g_in.pad;
    memset(&g_in, 0, sizeof g_in);
    g_in.pad = pad;
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
