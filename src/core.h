/* Shared types for the Halloween VGA screensaver.
 *
 * Everything the scenes draw goes into an 8-bit indexed Frame with a 6-bit
 * VGA DAC palette, exactly like a real VGA card.  The platform layer turns
 * that into pixels and scales it by an integer factor. */
#ifndef CORE_H
#define CORE_H

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <math.h>

#define FB_MAX_W 720
#define FB_MAX_H 400
#define PI_F 3.14159265358979f
#define TAU_F 6.28318530717959f

typedef struct Frame {
    int w, h;                       /* active video mode */
    float fade;                     /* scene-level fade, 0..1 (palette scale) */
    uint8_t pix[FB_MAX_W * FB_MAX_H];
    uint8_t pal[256][3];            /* VGA DAC: 6 bits per channel, 0..63 */
} Frame;

/* Keys and the mouse, for a scene a person can take over (holding H). */
enum { KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT, KEY_W, KEY_A, KEY_S, KEY_D, KEY_E, KEY_Q, KEY_SPACE, KEY_CTRL,
       KEY_SHIFT, KEY_TAB, KEY_ESC, KEY_ENTER, KEY_H, KEY_Y, KEY_N,
       KEY_1, KEY_2, KEY_3, KEY_4, KEY_5, KEY_6, KEY_7, KEY_8, KEY_9, KEY_MOUSE1, KEY_MOUSE2, KEY_B, KEY_L,
       KEY_CONSOLE, KEY_BACK, KEY_PGUP, KEY_PGDN, KEY_C, KEY_M, KEY_COUNT };   /* KEY_CONSOLE: the key left of 1, wherever ~ is on it */
/* A game controller, by XInput's layout; the triggers count as buttons past a third of the way. */
enum { PAD_A, PAD_B, PAD_X, PAD_Y, PAD_LB, PAD_RB, PAD_VIEW, PAD_MENU, PAD_LS, PAD_RS,
       PAD_UP, PAD_DOWN, PAD_LEFT, PAD_RIGHT, PAD_LT, PAD_RT, PAD_COUNT };
typedef struct Input {
    uint8_t down[KEY_COUNT];        /* held now */
    uint8_t hit[KEY_COUNT];         /* went down since the last update */
    float mdx, mdy;                 /* mouse movement since the last update, in mouse counts */
    int wheel;                      /* wheel notches since the last update, + away from you */
    char text[24];                  /* letters and digits typed since the last update, in capitals */
    int ntext;
    int pad;                        /* a controller is plugged in */
    uint8_t pad_down[PAD_COUNT], pad_hit[PAD_COUNT];
    float lx, ly, rx, ry;           /* the sticks past their dead zones, -1..1, y up */
    float lt, rt;                   /* the triggers, 0..1 */
} Input;
void pad_rumble(float low, float high, float seconds);   /* the controller's motors, 0..1 each, for a while */
enum { CTRL_TAKE = 1, CTRL_PAUSE = 2, CTRL_PEEK = 3 };  /* Scene.control */
enum { PLAY_AUTO, PLAY_PLAYER, PLAY_QUIT };             /* Scene.status */

/* A scene owns one VGA mode and one sound program. */
typedef struct Scene {
    const char *name;
    int w, h;                       /* native mode: 320x200 or 720x400 */
    int sound_program;              /* SND_PROG_* */
    void (*enter)(uint32_t seed);   /* (re)start from the top */
    void (*update)(double dt);      /* advance the simulation */
    void (*render)(Frame *f);       /* draw the current state */
    /* playable scenes only; NULL elsewhere */
    void (*control)(int what);                    /* CTRL_TAKE: a person takes over; CTRL_PAUSE: the window lost focus; CTRL_PEEK: H held, not yet taken */
    int  (*status)(void);                         /* PLAY_AUTO once handed back, PLAY_QUIT when the player chose to leave */
    void (*input)(const Input *in, double dt);    /* before each update while a person has it */
} Scene;

/* Save slots: the merchant's ledger, three pages, and beside them the progress (which page to start
 * from) and the dead's mark. The app keeps their bytes, the platform stores them (the registry on
 * Windows, files in the harness), loading them all before the first scene and writing any that changed
 * after each step. A scene reads and writes them whenever it likes. */
#define SLOT_COUNT 3                   /* the ledger's pages */
enum { SLOT_PROGRESS = SLOT_COUNT, SLOT_MARK, SLOT_ALL };
#define SLOT_MAX 16384                 /* a run and all the merchant's hall keeps for it */
int  slot_read(int i, uint8_t *buf, int cap);        /* the slot's length, 0 when it is empty */
void slot_write(int i, const uint8_t *buf, int len);

extern const Scene scene_haunt;     /* HAUNT.EXE   - 80x25 text mode, 720x400 */
extern const Scene scene_crawl;     /* CATACOMB    - mode 13h, 320x200        */

/* VGA bitmap fonts (src/font_vga.c, generated). */
extern const unsigned char font_vga16[256][16];
extern const unsigned char font_bios8[256][8];

/* ---- small helpers ---------------------------------------------------- */

static inline int clampi(int v, int lo, int hi) { return v < lo ? lo : (v > hi ? hi : v); }
static inline float clampf(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }
static inline float lerpf(float a, float b, float t) { return a + (b - a) * t; }
static inline float smoothstepf(float a, float b, float x) {
    float t = clampf((x - a) / (b - a), 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

/* xorshift32 - tiny, deterministic, per-scene state */
static inline uint32_t rng_next(uint32_t *s) {
    uint32_t x = *s ? *s : 0x9E3779B9u;
    x ^= x << 13; x ^= x >> 17; x ^= x << 5;
    *s = x;
    return x;
}
static inline float rng_f(uint32_t *s) { return (rng_next(s) >> 8) * (1.0f / 16777216.0f); }
static inline int rng_range(uint32_t *s, int lo, int hi) { /* inclusive */
    return lo + (int)(rng_next(s) % (uint32_t)(hi - lo + 1));
}

#endif
