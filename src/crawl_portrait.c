/* The portraits of the slain (docs/PLAN-portraits.md): each kind's picture in a frame on her walls, its candle beside it,
 * a tapestry over it until one is slain. The game draws them into its walls' overlays (scene_crawl.c, por_frame), and
 * tools/portraitmock.c the mock-ups.
 *
 * Everything is placed in texels of a wall face (64 x 64) and drawn at s pixels a texel, so the same drawing makes the
 * walls' own overlay (s 1), a finer one (s 2) and the card up close. Shapes are worked out at each pixel's middle, so a
 * larger s gives a finer shape, not a blockier one. The creature is drawn at half its size at s 1, at its own at s 2. */
#include "crawl_portrait.h"
#include <math.h>
#include <string.h>

const char *const PL_NAME[NPLAYOUT] = { "A SCONCE TO ITS RIGHT", "A SCONCE TO ITS LEFT", "A PAIR OF SCONCES", "A LAMP HUNG OVER IT",
                                        "A LEDGE OF CANDLES", "A SKULL ON A BRACKET", "A STAND OF THREE" };

static uint8_t *P;
static int PW, PH, PST, POX, POY, S;
static float PT;                                        /* the clock */

static float clampf_(float v, float a, float b) { return v < a ? a : v > b ? b : v; }
static int clampi_(int v, int a, int b) { return v < a ? a : v > b ? b : v; }
static float smooth_(float a, float b, float v) { float t = clampf_((v - a) / (b - a), 0, 1); return t * t * (3 - 2 * t); }
static uint32_t hsh(int x, int y, uint32_t s) {
    uint32_t h = (uint32_t)x * 374761393u + (uint32_t)y * 668265263u + s * 2246822519u;
    h = (h ^ (h >> 13)) * 1274126177u;
    return h ^ (h >> 16);
}
static float h01(int x, int y, uint32_t s) { return (float)(hsh(x, y, s) & 0xFFFF) / 65535.0f; }
static uint8_t sh(int ramp, float v) {
    if (ramp == R_GOLD) return (uint8_t)(R_GOLD + clampi_((int)(v * (NGOLD - 1) + 0.5f), 0, NGOLD - 1));
    return (uint8_t)(ramp + clampi_((int)(v * 15.0f + 0.5f), 0, 15));
}
static float lit(float nx, float ny, float nz) {        /* the house's light: from the upper left, in front */
    float d = nx * -0.45f + ny * -0.65f + nz * 0.62f;
    return d < 0.0f ? 0.0f : d;
}
static const uint8_t BAY[4][4] = { { 0, 8, 2, 10 }, { 12, 4, 14, 6 }, { 3, 11, 1, 9 }, { 15, 7, 13, 5 } };
/* a colour at a light (1 its own), dithered as the crypt is */
static uint8_t at_light(uint8_t c, float l, int x, int y) {
    if (IS_FULLBRIGHT(c) || c == C_CLEAR) return c;
    int lv = (int)(l * LIGHT_UNIT + BAY[y & 3][x & 3] * (1.0f / 16.0f));
    return crawl_cmap[clampi_(lv, 0, NLIGHT - 1)][c];
}

/* ---- pixels, and shapes in texels ---- */
static void pp(int x, int y, uint8_t c) {               /* a pixel, in the face's own pixels */
    x += POX; y += POY;
    if ((unsigned)x < (unsigned)PW && (unsigned)y < (unsigned)PH) P[y * PST + x] = c;
}
static uint8_t pget(int x, int y) {
    x += POX; y += POY;
    return (unsigned)x < (unsigned)PW && (unsigned)y < (unsigned)PH ? P[y * PST + x] : (uint8_t)C_CLEAR;
}
#define TX(p) (((float)(p) + 0.5f) / (float)S)         /* a pixel's middle, in texels */
static int lo_px(float t) { return (int)floorf(t * S); }
static int hi_px(float t) { return (int)ceilf(t * S); }

static void ellipse(float cx, float cy, float rx, float ry, int ramp, float lo, float hi) {
    for (int y = lo_px(cy - ry); y <= hi_px(cy + ry); y++)
        for (int x = lo_px(cx - rx); x <= hi_px(cx + rx); x++) {
            float nx = (TX(x) - cx) / rx, ny = (TX(y) - cy) / ry, d = nx * nx + ny * ny;
            if (d <= 1.0f) pp(x, y, sh(ramp, lo + (hi - lo) * lit(nx, ny, sqrtf(1.0f - d))));
        }
}
static void flat_ellipse(float cx, float cy, float rx, float ry, uint8_t c) {
    for (int y = lo_px(cy - ry); y <= hi_px(cy + ry); y++)
        for (int x = lo_px(cx - rx); x <= hi_px(cx + rx); x++) {
            float nx = (TX(x) - cx) / rx, ny = (TX(y) - cy) / ry;
            if (nx * nx + ny * ny <= 1.0f) pp(x, y, c);
        }
}
static void capsule(float x0, float y0, float x1, float y1, float r, int ramp, float lo, float hi) {
    float vx = x1 - x0, vy = y1 - y0, len2 = vx * vx + vy * vy + 1e-6f;
    for (int y = lo_px(fminf(y0, y1) - r); y <= hi_px(fmaxf(y0, y1) + r); y++)
        for (int x = lo_px(fminf(x0, x1) - r); x <= hi_px(fmaxf(x0, x1) + r); x++) {
            float px = TX(x), py = TX(y), t = clampf_(((px - x0) * vx + (py - y0) * vy) / len2, 0.0f, 1.0f);
            float ox = (px - (x0 + vx * t)) / r, oy = (py - (y0 + vy * t)) / r, d = ox * ox + oy * oy;
            if (d <= 1.0f) pp(x, y, sh(ramp, lo + (hi - lo) * lit(ox, oy, sqrtf(1.0f - d))));
        }
}
static void box(float x0, float y0, float x1, float y1, uint8_t c) {
    for (int y = lo_px(y0); y < hi_px(y1); y++) for (int x = lo_px(x0); x < hi_px(x1); x++) pp(x, y, c);
}
/* a flame standing on (cx, by), h tall and w wide, leaning by sway at its tip; its heart white */
static void flame(float cx, float by, float h, float w, float sway) {
    for (int y = lo_px(by - h); y <= hi_px(by); y++)
        for (int x = lo_px(cx - w - 2); x <= hi_px(cx + w + 2); x++) {
            float u = (by - TX(y)) / h;
            if (u < 0.0f || u > 1.0f) continue;
            float c = cx + sway * u * u, ww = w * fmaxf(0.0f, sinf(3.14159f * (0.15f + 0.85f * (1.0f - u)))) * (1.0f - 0.3f * u);
            if (ww < 0.25f) continue;
            float d = fabsf(TX(x) - c) / (ww + 0.01f);
            if (d > 1.0f) continue;
            float v = 1.0f - d * 0.6f - u * 0.5f;
            pp(x, y, v > 0.93f && u < 0.45f ? (uint8_t)C_WHITE : (uint8_t)(R_FLAME + clampi_((int)(v * 15.0f), 2, 15)));
        }
}
/* a candle's flame as the clock moves it: k tells one from another */
typedef struct { float x, y, h, w, sway, power; } Flame;
static Flame flame_of(float x, float top, float h, int k) {
    float t = PT + (float)k * 1.37f;
    Flame f = { x, top - 0.6f, h * (1.0f + 0.10f * sinf(t * 9.1f) + 0.06f * sinf(t * 23.0f + (float)k)),
                h * 0.30f, 0.55f * sinf(t * 6.3f + (float)k) + 0.25f * sinf(t * 13.7f), 1.0f + 0.08f * sinf(t * 11.0f + (float)k * 2.0f) };
    return f;
}
/* a church candle on (x, by), h tall and r round: its wax lit from the left, a cup at its top, wax run down its front */
static void candle(float x, float by, float h, float r, int k, int lit_, Flame *out) {
    for (int y = lo_px(by - h); y < hi_px(by); y++)
        for (int xx = lo_px(x - r); xx <= hi_px(x + r); xx++) {
            float nx = (TX(xx) - x) / r;
            if (fabsf(nx) > 1.0f) continue;
            float top = TX(y) < by - h + 0.7f ? 0.08f : 0.0f;
            pp(xx, y, sh(R_BONE, 0.40f + 0.50f * lit(nx, 0.0f, sqrtf(1.0f - nx * nx)) + top));
        }
    for (int d = 0; d < 2; d++) {                                   /* runs of wax down its front */
        float dx = x - r * 0.45f + (float)d * r * 0.9f, l = 0.8f + h01(k, d, 701) * h * 0.5f;
        for (int y = lo_px(by - h); y < hi_px(by - h + l); y++)
            for (int xx = lo_px(dx - 0.3f); xx < hi_px(dx + 0.3f); xx++) pp(xx, y, sh(R_BONE, 0.96f));
    }
    box(x - 0.2f, by - h - 0.9f, x + 0.25f, by - h, sh(R_IRON, 0.12f));   /* the wick */
    if (lit_) {
        Flame f = flame_of(x, by - h - 0.6f, 2.6f + 0.9f * fminf(1.0f, r), k);
        flame(f.x, f.y, f.h, f.w, f.sway);
        if (out) *out = f;
    }
}
static void smoke(float x, float y, float h, uint32_t seed) {      /* a thread from a candle gone out */
    for (int k = 0; k < (int)(h * S); k++) {
        float t = (float)k / (h * S);
        int px = (int)((x + sinf(t * 6.0f + PT * 1.5f + (float)(seed & 7)) * (0.3f + 1.5f * t)) * S), py = (int)(y * S) - k;
        if (pget(px, py) != C_CLEAR || h01(px, py + (int)(PT * 8.0f), seed) < t * 0.8f) continue;
        pp(px, py, sh(R_IRON, 0.45f - 0.25f * t));
    }
}

/* ---- the layouts: where the frame hangs on the face, and its candles ---- */
static const int FRAME_AT[NPLAYOUT][4] = {             /* x0, y0, x1, y1: forty-two texels by forty-eight, the picture 32 by 38 */
    { 4, 8, 46, 56 }, { 18, 8, 60, 56 }, { 11, 8, 53, 56 }, { 11, 15, 53, 63 }, { 11, 4, 53, 52 }, { 18, 8, 60, 56 }, { 5, 8, 47, 56 } };
void portrait_frame_rect(int layout, int r[4]) { memcpy(r, FRAME_AT[clampi_(layout, 0, NPLAYOUT - 1)], sizeof FRAME_AT[0]); }
#define MOULD 5                                         /* the frame's width, in texels */

static Flame g_fl[4];
static int g_nfl;
static void keep_flame(Flame f) { if (g_nfl < 4) g_fl[g_nfl++] = f; }

/* an iron sconce on the wall: a backplate, its finial, the arm out to a drip pan, a candle in it */
static void sconce(float x, float y, float h, int k) {
    capsule(x, y - 3.0f, x, y + 4.5f, 1.25f, R_IRON, 0.10f, 0.55f);   /* the backplate */
    ellipse(x, y + 5.6f, 0.9f, 1.3f, R_IRON, 0.10f, 0.6f);            /* its finial */
    ellipse(x, y - 3.2f, 1.0f, 1.0f, R_IRON, 0.15f, 0.7f);
    capsule(x, y + 1.2f, x, y - 0.4f, 0.55f, R_IRON, 0.2f, 0.7f);     /* the arm, coming at you */
    for (int d = 0; d < 3; d++) {                                      /* wax hung off the pan */
        float dx = x - 1.6f + (float)d * 1.4f + h01(k, d, 41) * 0.5f, l = 0.6f + h01(k, d, 43) * 1.8f;
        box(dx, y - 0.6f, dx + 0.45f, y - 0.6f + l, sh(R_BONE, 0.82f - 0.1f * (float)d));
    }
    ellipse(x, y - 1.0f, 2.7f, 0.9f, R_IRON, 0.18f, 0.8f);            /* the drip pan */
    Flame f;
    candle(x, y - 1.2f, h, 1.0f, k, 1, &f);
    keep_flame(f);
}
static void skull_bracket(float x, float y, int k) {                 /* a skull set on an iron bracket, a candle on its crown */
    capsule(x - 3.5f, y + 2.0f, x + 3.5f, y + 2.0f, 0.6f, R_IRON, 0.15f, 0.6f);   /* the bracket's shelf */
    capsule(x, y + 2.0f, x, y + 6.5f, 0.8f, R_IRON, 0.12f, 0.5f);
    ellipse(x, y + 6.8f, 0.9f, 0.9f, R_IRON, 0.15f, 0.6f);
    ellipse(x, y - 1.8f, 3.3f, 3.0f, R_BONE, 0.30f, 0.95f);           /* the cranium */
    ellipse(x, y + 0.9f, 2.4f, 1.4f, R_BONE, 0.25f, 0.85f);           /* the jaw */
    for (int s = -1; s <= 1; s += 2) flat_ellipse(x + (float)s * 1.25f, y - 1.0f, 0.85f, 0.95f, sh(R_IRON, 0.02f));   /* its sockets */
    flat_ellipse(x, y + 0.4f, 0.4f, 0.6f, sh(R_IRON, 0.05f));
    for (int t = -2; t <= 2; t++) box(x + (float)t * 0.75f - 0.15f, y + 1.3f, x + (float)t * 0.75f + 0.15f, y + 2.1f, sh(R_BONE, 0.2f));   /* teeth */
    for (int d = 0; d < 4; d++) {                                      /* the candle's wax run over its brow */
        float dx = x - 1.5f + (float)d * 1.0f, l = 1.0f + h01(k, d, 47) * 2.6f + (d == 1 ? 1.2f : 0.0f);
        box(dx, y - 4.4f, dx + 0.5f, y - 4.4f + l, sh(R_BONE, 0.97f));
    }
    Flame f;
    candle(x + 0.3f, y - 4.2f, 5.5f, 1.0f, k, 1, &f);
    keep_flame(f);
}
static void hung_lamp(float x, float y0, float y1, int k) {         /* an iron lantern hung on a chain over it */
    for (float y = y0; y < y1 - 4.0f; y += 1.6f) {                     /* the chain's links, turned alternately */
        int alt = ((int)((y - y0) / 1.6f)) & 1;
        ellipse(x, y + 0.8f, alt ? 0.45f : 0.75f, 0.85f, R_IRON, 0.2f, 0.7f);
    }
    float cy = y1;                                                     /* the lantern's middle */
    ellipse(x, cy - 4.2f, 2.8f, 1.2f, R_IRON, 0.12f, 0.6f);           /* its cap */
    ellipse(x, cy - 5.2f, 0.9f, 0.8f, R_IRON, 0.2f, 0.7f);
    for (int y = lo_px(cy - 3.2f); y < hi_px(cy + 3.0f); y++)          /* the glass, its glow behind the bars */
        for (int xx = lo_px(x - 2.6f); xx < hi_px(x + 2.6f); xx++) {
            float u = (TX(xx) - x) / 2.6f, v = (TX(y) - cy) / 3.0f;
            pp(xx, y, (uint8_t)(R_FLAME + clampi_((int)(10.0f - 6.0f * (u * u + v * v)), 3, 12)));
        }
    Flame f = flame_of(x, cy + 1.6f, 3.0f, k);
    flame(f.x, f.y, f.h, f.w * 0.8f, f.sway * 0.4f);
    for (int b = -1; b <= 1; b++) box(x + (float)b * 1.7f - 0.3f, cy - 3.2f, x + (float)b * 1.7f + 0.3f, cy + 3.0f, sh(R_IRON, 0.22f));   /* its bars */
    capsule(x - 2.8f, cy + 3.2f, x + 2.8f, cy + 3.2f, 0.7f, R_IRON, 0.1f, 0.55f);   /* its foot */
    ellipse(x, cy + 4.4f, 0.8f, 0.8f, R_IRON, 0.15f, 0.6f);
    f.power = 1.15f * f.power; f.y = cy; keep_flame(f);
}
static void ledge(float x0, float x1, float y, int k) {              /* an iron shelf on two brackets, three candles on it, one out */
    for (int s = 0; s < 2; s++) {
        float bx = s ? x1 - 4.0f : x0 + 4.0f;
        capsule(bx, y + 0.5f, bx + (s ? 3.0f : -3.0f) * 0.0f, y + 6.0f, 0.55f, R_IRON, 0.12f, 0.5f);
        capsule(bx, y + 5.5f, bx + (s ? -3.5f : 3.5f), y + 0.8f, 0.45f, R_IRON, 0.12f, 0.5f);   /* its strut */
    }
    capsule(x0, y, x1, y, 0.85f, R_IRON, 0.12f, 0.75f);
    for (int i = 0; i < 6; i++) {                                      /* wax pooled and hung off it */
        float dx = x0 + 3.0f + h01(k, i, 53) * (x1 - x0 - 6.0f), l = 0.6f + h01(k, i, 59) * 2.4f;
        box(dx, y + 0.4f, dx + 0.5f, y + 0.4f + l, sh(R_BONE, 0.8f));
    }
    Flame f;
    candle(x0 + 5.5f, y - 0.6f, 6.5f, 1.15f, k, 1, &f); keep_flame(f);
    candle(x1 - 6.0f, y - 0.6f, 9.0f, 1.2f, k + 1, 1, &f); keep_flame(f);
    candle(x1 - 11.0f, y - 0.6f, 2.5f, 1.0f, k + 2, 0, NULL);
    smoke(x1 - 11.0f, y - 4.0f, 6.0f, 61);
}
static void stand(float x, float y_floor, float y_tray, int k) {     /* the candle stand on the floor: tripod, twisted shaft, tray */
    for (int s = -1; s <= 1; s++) capsule(x, y_floor - 3.0f, x + (float)s * 3.2f, y_floor - 0.4f, 0.6f, R_IRON, 0.12f, 0.6f);
    for (int y = lo_px(y_tray); y < hi_px(y_floor - 2.5f); y++)
        for (int xx = lo_px(x - 0.6f); xx < hi_px(x + 0.6f); xx++)
            pp(xx, y, sh(R_IRON, ((int)(TX(y) * 1.2f + (TX(xx) - x) * 2.0f) & 1) ? 0.62f : 0.22f));   /* twisted */
    ellipse(x, y_floor - 9.0f, 1.3f, 0.8f, R_IRON, 0.2f, 0.8f); ellipse(x, y_tray + 8.0f, 1.1f, 0.7f, R_IRON, 0.2f, 0.8f);   /* knops */
    for (int i = 0; i < 5; i++) {
        float dx = x - 3.5f + h01(k, i, 67) * 7.0f, l = 0.6f + h01(k, i, 71) * 2.2f;
        box(dx, y_tray + 0.5f, dx + 0.45f, y_tray + 0.5f + l, sh(R_BONE, 0.82f));
    }
    ellipse(x, y_tray, 4.6f, 1.1f, R_IRON, 0.12f, 0.75f);             /* the tray */
    Flame f;
    candle(x - 3.0f, y_tray - 0.4f, 4.0f, 0.85f, k, 1, &f); keep_flame(f);
    candle(x, y_tray - 0.4f, 6.5f, 0.95f, k + 1, 1, &f); keep_flame(f);
    candle(x + 3.0f, y_tray - 0.4f, 5.0f, 0.85f, k + 2, 1, &f); keep_flame(f);
}
static void layout_lights(int layout, int behind) {                 /* the candles: those that hang behind the frame first */
    switch (layout) {
    case PL_SCONCE_R: if (!behind) sconce(54.5f, 35.0f, 8.0f, 1); break;
    case PL_SCONCE_L: if (!behind) sconce(9.5f, 35.0f, 8.0f, 2); break;
    case PL_PAIR: if (!behind) { sconce(5.0f, 31.0f, 6.0f, 3); sconce(59.0f, 31.0f, 6.0f, 4); } break;
    case PL_LAMP: if (behind) hung_lamp(32.0f, -0.5f, 7.0f, 5); break;
    case PL_LEDGE: if (!behind) ledge(9.0f, 55.0f, 57.0f, 6); break;
    case PL_SKULL: if (!behind) skull_bracket(9.5f, 37.0f, 9); break;
    case PL_STAND: if (!behind) stand(56.0f, 64.0f, 30.0f, 10); break;
    }
}

/* ---- the frame: a moulding, its profile shaded by where the light comes from, its bead worked into pearls; a block at each
 * corner with a rosette, a cartouche standing over its head with a skull in it; gems on its foot, one a rank, lit in the rank's
 * colour once unlocked ---- */
static const int FRAME_RAMP[NRANK + 1] = { R_WOOD, R_BONE, R_MOSS, R_GHOST, R_BLOOD, R_IRON, R_GOLD };   /* bare, then RK_I.. */
static const float FRAME_LO[NRANK + 1] = { 0.10f, 0.22f, 0.12f, 0.12f, 0.10f, 0.00f, 0.06f };
static const float FRAME_HI[NRANK + 1] = { 0.58f, 0.95f, 0.90f, 0.92f, 0.88f, 0.46f, 1.00f };
static const int GEM_RAMP[NRANK] = { R_BONE, R_MOSS, R_GHOST, R_BLOOD, R_IRON, R_GOLD };
static float profile(float q) {                         /* the moulding's height across it, outer edge 0 to sight edge 1 */
    if (q < 0.14f) return 0.65f * smooth_(0.0f, 0.14f, q);              /* the outer lip */
    if (q < 0.32f) return 0.65f;                                        /* its flat */
    if (q < 0.55f) { float u = (q - 0.32f) / 0.23f; return 0.65f - 0.40f * sinf(u * 3.14159f) * 0.9f - 0.1f * u; }   /* the cove */
    if (q < 0.82f) { float u = (q - 0.55f) / 0.27f; return 0.55f + 0.35f * sinf(u * 3.14159f); }   /* the bead */
    return 0.55f - 0.45f * smooth_(0.82f, 1.0f, q);                     /* down to the picture */
}
static float g_lx, g_ly;                                /* the light's way across the frame, for its blocks */
static float shade_n(float nx, float ny) {              /* a surface leaning nx, ny (1 a full turn to the side) in the light */
    float nl = sqrtf(nx * nx + ny * ny + 1.0f);
    return clampf_((nx * g_lx * 0.75f + ny * g_ly * 0.75f + 0.62f) / nl, 0.0f, 1.0f);
}
static void frame(const int r[4], int rank, float lx, float ly, const PortraitView *v) {
    int fi = rank + 1, ramp = FRAME_RAMP[fi];
    float lo = FRAME_LO[fi], hi = FRAME_HI[fi], m = (float)MOULD;
    float ll = sqrtf(lx * lx + ly * ly) + 1e-6f; g_lx = lx / ll; g_ly = ly / ll;
    for (int y = r[1] * S; y < r[3] * S; y++)
        for (int x = r[0] * S; x < r[2] * S; x++) {
            float u = TX(x), w = TX(y);
            float d4[4] = { u - (float)r[0], (float)r[2] - u, w - (float)r[1], (float)r[3] - w };   /* left, right, top, bottom */
            int side = 0;
            for (int k = 1; k < 4; k++) if (d4[k] < d4[side]) side = k;
            float d = d4[side];
            if (d >= m) continue;                                       /* the picture */
            float val, b;
            if (fminf(d4[0], d4[1]) < m && fminf(d4[2], d4[3]) < m) {  /* a corner block: raised, its edges bevelled */
                float cu = fminf(d4[0], d4[1]), cv = fminf(d4[2], d4[3]), sx = d4[0] < d4[1] ? -1.0f : 1.0f, sy = d4[2] < d4[3] ? -1.0f : 1.0f;
                float ce = fminf(fminf(cu, m - cu), fminf(cv, m - cv));
                float nx = 0, ny = 0;
                if (ce < 0.9f) {
                    if (cu < 0.9f) nx = sx * 0.8f; else if (m - cu < 0.9f) nx = -sx * 0.8f;
                    if (cv < 0.9f) ny = sy * 0.8f; else if (m - cv < 0.9f) ny = -sy * 0.8f;
                }
                b = shade_n(nx, ny);
                val = lo + (hi - lo) * (0.1f + 0.95f * b * b);
                if (ce < 0.5f / (float)S) val *= 0.55f;
            } else {
                static const float NIN[4][2] = { { 1, 0 }, { -1, 0 }, { 0, 1 }, { 0, -1 } };
                float q = d / m, e = 0.5f / (float)S / m;
                float slope = (profile(fminf(1.0f, q + e)) - profile(fmaxf(0.0f, q - e))) / (2.0f * e) * 0.22f;
                b = shade_n(-slope * NIN[side][0], -slope * NIN[side][1]);
                val = lo + (hi - lo) * (0.12f + 0.98f * b * b);
                if (S >= 2 && q > 0.55f && q < 0.82f) {                  /* the bead, a string of pearls */
                    float a = side < 2 ? w : u, bm = sinf(a * 6.2832f / 1.5f);
                    val *= 0.78f + 0.22f * bm;
                    b *= 0.85f + 0.15f * bm;
                }
                if (q < 0.6f / m / (float)S * 2.0f || q > 1.0f - 0.6f / m) val *= 0.45f;   /* its edges, in shadow */
            }
            if (ramp == R_WOOD) val += 0.06f * (h01((int)(u * 3.0f), (int)(w * 0.6f), 81) - 0.5f) + (h01((int)u, (int)w, 83) < 0.05f ? -0.08f : 0.0f);   /* grain, worm */
            uint8_t c = sh(ramp, val);
            if (rank == RK_V && b > 0.9f) c = (uint8_t)(R_IRON + 12);    /* the black's lacquer catching the light */
            if (rank == RK_GILDED && b > 0.8f && h01(x, y, 85) < 0.025f) c = (uint8_t)C_WHITE;
            pp(x, y, c);
        }
    float cm = m * 0.5f;                                                /* the corners' rosettes */
    float CX[4] = { r[0] + cm, r[2] - cm, r[0] + cm, r[2] - cm }, CY[4] = { r[1] + cm, r[1] + cm, r[3] - cm, r[3] - cm };
    for (int k = 0; k < 4; k++) {
        if (S >= 2) {
            for (int p = 0; p < 4; p++) {
                float a = (float)p * 1.5708f + 0.785f;
                ellipse(CX[k] + cosf(a) * 1.05f, CY[k] + sinf(a) * 1.05f, 0.95f, 0.95f, ramp, lo, hi);
            }
            ellipse(CX[k], CY[k], 0.75f, 0.75f, ramp, lo, hi * 0.8f);
            pp(lo_px(CX[k]), lo_px(CY[k]), sh(ramp, lo));
        } else ellipse(CX[k], CY[k], 1.3f, 1.3f, ramp, lo, hi);
    }
    if (v->layout != PL_LAMP) {                                         /* the cartouche over its head, a skull in it */
        float sx = (r[0] + r[2]) * 0.5f, sy = (float)r[1] + 0.8f;
        ellipse(sx, sy, 3.6f, 3.3f, ramp, lo, hi);
        for (int p = -1; p <= 1; p += 2) ellipse(sx + (float)p * 3.6f, sy + 1.1f, 1.1f, 1.1f, ramp, lo, hi);   /* its scrolls */
        if (S >= 2) {
            static const char *const SK[7] = { ".#####.", "#######", "#..#..#", "#######", ".##.##.", ".#.#.#.", "......." };
            float ps = 0.62f;
            for (int yy = 0; yy < 7; yy++) for (int xx = 0; xx < 7; xx++) {
                if (SK[yy][xx] != '#') continue;
                float ux = sx + ((float)xx - 3.5f) * ps, uy = sy + ((float)yy - 3.6f) * ps;
                box(ux, uy, ux + ps, uy + ps, sh(ramp, lo + (hi - lo) * (yy < 2 ? 1.0f : yy < 4 ? 0.8f : 0.62f)));
            }
            for (int p = -1; p <= 1; p += 2) box(sx + (float)p * 1.25f - 0.3f, sy - 1.5f, sx + (float)p * 1.25f + 0.3f, sy - 0.9f, sh(ramp, lo));   /* its sockets */
        } else ellipse(sx, sy, 1.4f, 1.3f, ramp, lo + 0.3f, hi);
    }
    /* the gems on its foot: the kind's ranks, in order; an unlocked one in its rank's colour, one whose gold waits twinkling */
    float gy = r[3] - cm, step = 3.4f, gx0 = (r[0] + r[2]) * 0.5f - step * (float)(v->nranks - 1) * 0.5f;
    for (int i = 0; i < v->nranks; i++) {
        int rk = v->ranks[i], on = v->unlocked >> rk & 1, wait = v->unpaid >> rk & 1;
        float gx = gx0 + step * (float)i, rr = 1.35f;
        for (int y = lo_px(gy - rr - 0.4f); y <= hi_px(gy + rr + 0.4f); y++)   /* its setting */
            for (int x = lo_px(gx - rr - 0.4f); x <= hi_px(gx + rr + 0.4f); x++)
                if (fabsf(TX(x) - gx) + fabsf(TX(y) - gy) <= rr + 0.45f) pp(x, y, sh(ramp, lo * 0.5f));
        for (int y = lo_px(gy - rr); y <= hi_px(gy + rr); y++)
            for (int x = lo_px(gx - rr); x <= hi_px(gx + rr); x++) {
                float dx = TX(x) - gx, dy = TX(y) - gy;
                if (fabsf(dx) + fabsf(dy) > rr) continue;
                float b = 0.5f - 0.35f * (dx + dy) / rr;
                uint8_t c = on ? sh(GEM_RAMP[rk], rk == RK_V ? 0.05f + 0.3f * b : 0.35f + 0.6f * b) : sh(R_IRON, 0.04f + 0.08f * b);
                pp(x, y, c);
            }
        if (on) pp(lo_px(gx - 0.5f), lo_px(gy - 0.6f), rk == RK_V ? (uint8_t)(R_IRON + 12) : (uint8_t)C_WHITE);   /* its glint */
        if (wait) {                                                     /* its gold, waiting: a star now and then */
            float ph = fmodf(PT * 0.9f + (float)i * 0.37f, 1.6f);
            if (ph < 0.45f) {
                int a = (int)((0.45f - fabsf(ph - 0.225f) * 2.0f) * 2.2f * (float)S) + 1;
                int cx = lo_px(gx), cy = lo_px(gy);
                for (int k = -a; k <= a; k++) { pp(cx + k, cy, (uint8_t)(R_GOLD + 12)); pp(cx, cy + k, (uint8_t)(R_GOLD + 12)); }
                pp(cx, cy, (uint8_t)C_WHITE);
            }
        }
    }
}

/* ---- the picture: the run's stone behind it and its floor in front, in a pool of painted light, the creature walking on the
 * spot and turning about as in the bestiary, in its rank's colours and dressing, its eyes put back after ---- */
#define PIC_W 64                                        /* the picture in the sprite's own pixels: two a texel */
#define PIC_H 76
#define PIC_HZ 58                                       /* where the floor meets the wall */
#define PIC_FOOT 72                                     /* where it stands */
static int pick_sprite(const PortraitView *v, int *flip) {
    float a = v->t - v->walk0, ph = fmodf(a, 6.4f);
    int step = (int)(a * 4.0f) & 1, facing = (int)(ph / 1.6f);     /* towards you, its right side, towards you, its left */
    *flip = facing == 3;
    return facing & 1 ? v->spr[2 + step] : v->spr[step];
}
static uint8_t creature_px(const PortraitView *v, int spr, int flip, int qx, int qy, const uint8_t *lut, const uint8_t *ov) {
    int sx = qx, sy = qy - (PIC_FOOT - TS);
    if (sx < 0 || sy < 0 || sx >= TS || sy >= TS) return C_CLEAR;
    if (flip) sx = TS - 1 - sx;
    uint8_t o = ov ? ov[sx * TS + sy] : (uint8_t)C_CLEAR, c = crawl_spr[spr][sx * TS + sy];
    if (o != C_CLEAR) return o;
    if (c == C_CLEAR) return c;
    return lut ? lut[c] : c;
}
static float lum(uint8_t c) { return (float)(crawl_pal[c][0] * 3 + crawl_pal[c][1] * 4 + crawl_pal[c][2] * 2); }
static void picture(const int r[4], const PortraitView *v) {
    int ix0 = (r[0] + MOULD) * S, iy0 = (r[1] + MOULD) * S, ix1 = (r[2] - MOULD) * S, iy1 = (r[3] - MOULD) * S;
    int flip, spr = pick_sprite(v, &flip), sil = v->rank < 0;
    const uint8_t *lut = !sil && v->family >= 0 && v->rank != RK_I ? crawl_rank_lut[v->family][v->rank] : NULL;
    int dix = sil ? -1 : crawl_dress_ix[spr][clampi_(v->rank, 0, NRANK - 1)];
    const uint8_t *ov = dix >= 0 && v->rank != RK_I ? crawl_dress[dix] : NULL;
    for (int y = iy0; y < iy1; y++)
        for (int x = ix0; x < ix1; x++) {
            float qx = ((float)(x - ix0) + 0.5f) * 2.0f / (float)S, qy = ((float)(y - iy0) + 0.5f) * 2.0f / (float)S;
            uint8_t c = C_CLEAR;
            if (S >= 2) c = creature_px(v, spr, flip, (int)qx, (int)qy, lut, ov);
            else {                                                      /* half its size: the block's middling colour, keeping
                                                                           a lone bright bone and the eyes' dark */
                int bx = (x - ix0) * 2, by = (y - iy0) * 2, n = 0;
                uint8_t cs[4];
                for (int k = 0; k < 4; k++) { uint8_t q = creature_px(v, spr, flip, bx + (k & 1), by + (k >> 1), lut, ov); if (q != C_CLEAR) cs[n++] = q; }
                if (n >= 2) {
                    for (int i = 1; i < n; i++) { uint8_t q = cs[i]; int j = i - 1; while (j >= 0 && lum(cs[j]) > lum(q)) { cs[j + 1] = cs[j]; j--; } cs[j + 1] = q; }
                    c = cs[n / 2];
                } else if (n == 1 && lum(cs[0]) > 90.0f) c = cs[0];
            }
            float dx = qx - 32.0f, dy = qy - 42.0f;
            if (c != C_CLEAR) {                                         /* it, lit from your side */
                float l = sil ? 1.0f : 1.04f - 0.10f * dx / 32.0f;
                pp(x, y, sil ? (uint8_t)(R_IRON + 2) : at_light(c, l, x, y));
                continue;
            }
            float l = 1.02f - 0.60f * (dx * dx + dy * dy * 0.7f) / (44.0f * 44.0f) - 0.05f * (dx + dy) / 64.0f;
            float ex = fminf(qx, (float)PIC_W - qx), ey = fminf(qy, (float)PIC_H - qy);   /* in the frame's shadow at its edge */
            if (ex < 3.0f || ey < 3.0f) l *= 0.55f + 0.45f * fminf(ex, ey) / 3.0f;
            uint8_t b;
            if (qy < PIC_HZ) b = v->wall[(((int)(qx * 1.25f) + 9) & 63) * TS + (((int)(qy * 1.25f) + 6) & 63)];   /* its run's stone, a step behind it */
            else {                                                      /* its run's floor, going away */
                float dz = qy - (PIC_HZ - 2.0f), depth = 60.0f / dz;
                int fu = (int)(32.0f + (qx - 32.0f) * depth / 22.0f) & 63, fv = (int)(depth * 3.0f + 9.0f) & 63;
                b = v->floor[fu * TS + fv];
                l *= 0.55f + 0.45f * clampf_(dz / 18.0f, 0.0f, 1.0f);
                float sx = (qx - 32.0f) / 15.0f, sy = (qy - (float)PIC_FOOT + 0.5f) / 3.2f;   /* its shadow */
                if (!sil && sx * sx + sy * sy < 1.0f) l *= 0.5f;
            }
            if (sil) l *= 0.85f;
            pp(x, y, at_light(b, clampf_(l, 0.22f, 1.0f), x, y));
        }
    if (sil || !v->eye_core) return;
    const EyeAt *e = &crawl_eyes[spr];                                  /* its eyes, never lost in the light, as in the crypt */
    if (fmodf(v->t * 0.37f, 5.0f) < 0.08f) return;                       /* a blink */
    for (int i = 0; i < e->n; i++) {
        float ex = (float)(flip ? TS - 1 - e->x[i] : e->x[i]) + 0.5f, ey = (float)e->y[i] + (PIC_FOOT - TS) + 0.5f;
        int cs = S >= 2 ? S : 1, cx = ix0 + (int)(ex * (float)S / 2.0f) - cs / 2, cy = iy0 + (int)(ey * (float)S / 2.0f) - cs / 2;
        int out = e->n == 2 ? ((e->x[i] < e->x[1 - i]) != flip ? -1 : 1) : 1;
        for (int yy = 0; yy < cs; yy++) for (int xx = 0; xx < cs; xx++) pp(cx + xx, cy + yy, (uint8_t)v->eye_core);
        if (S < 2) continue;
        switch (v->rank) {                                              /* the rank's own form, as render_eyes has them */
        case RK_II: for (int k = 0; k < cs; k++) pp(out > 0 ? cx + cs : cx - 1, cy + k, (uint8_t)v->eye_glow); break;
        case RK_III: for (int k = 1; k <= cs; k++) { pp(cx + cs / 2, cy - k, (uint8_t)v->eye_glow); pp(cx + cs / 2, cy + cs - 1 + k, (uint8_t)v->eye_glow); } break;
        case RK_IV: for (int k = 1; k <= cs + 1; k++) pp((out > 0 ? cx + cs - 1 : cx) + out * k, cy - (k + 1) / 2, (uint8_t)v->eye_glow); break;
        case RK_V: for (int k = -1; k <= cs; k++) { pp(cx + k, cy - 1, (uint8_t)v->eye_glow); pp(cx + k, cy + cs, (uint8_t)v->eye_glow); } break;
        case RK_GILDED: pp(cx - 1, cy, (uint8_t)v->eye_glow); pp(cx + cs, cy, (uint8_t)v->eye_glow); pp(cx, cy - 1, (uint8_t)v->eye_glow); pp(cx, cy + cs, (uint8_t)v->eye_glow); break;
        default: break;
        }
    }
}

/* ---- the tapestry: oxblood, pleated, a border of gold thread and a skull worked in it, a fringe; hung from an iron rod over
 * the frame and its cartouche, drawn back to the side away from the candle and left there, gathered and tied (the owner's Q8) ---- */
static void tapestry(const int r[4], int side, float veil) {
    float X0 = (float)r[0] - 1.5f, X1 = (float)r[2] + 1.5f, Y0 = (float)r[1] - 3.2f, Y1 = (float)r[3] + 2.4f, Wc = X1 - X0;
    float Wv = Wc * (0.14f + 0.86f * veil), deep = 1.0f - veil;   /* gathered, it covers the frame's side and no more */
    static const char *const SK[7] = { ".#####.", "#######", "#..#..#", "#######", ".##.##.", ".#.#.#.", "......." };
    for (int x = lo_px(X0); x < hi_px(X1); x++) {                     /* a column at a time: its pleat and hem are the column's */
        float u = TX(x), p = side ? X1 - u : u - X0;
        if (p < 0.0f || p > Wv) continue;
        float c = p / Wv * Wc, hem = Y1 - deep * 3.5f * powf(p / Wv, 1.5f);
        float ph = c * 6.2832f / 4.2f, amp = 0.05f + 0.08f * deep, sph = sinf(ph);
        for (int y = lo_px(Y0); y < hi_px(Y1 + 2.4f); y++) {
            float w = TX(y), cy = w - Y0;
            if (w > hem) {                                              /* the fringe */
                int th = (int)(c * 2.0f);
                if ((th & 1) || w > hem + 1.2f + 1.0f * h01(th, 3, 91)) continue;
                pp(x, y, sh(R_GOLD, 0.20f + 0.12f * h01(th, 5, 93)));
                continue;
            }
            float val = 0.13f + amp * sph - 0.06f * cy / (Y1 - Y0) + 0.03f * (h01((int)(c * 3.0f), (int)(cy * 3.0f), 95) - 0.5f);
            uint8_t col = sh(R_BLOOD, val);
            float bc = fminf(c, Wc - c), bh = fminf(cy, hem - w);
            if ((bc > 1.3f && bc < 2.0f && bh > 1.3f) || (bh > 1.3f && bh < 2.0f && bc > 1.3f))   /* the border, gold thread */
                col = sh(R_GOLD, 0.26f + 0.18f * fmaxf(0.0f, sph) + 0.08f * h01((int)(c * 4.0f), (int)(cy * 4.0f), 97));
            float mx = (c - (Wc * 0.5f - 7.0f)) / 2.0f, my = (cy - ((Y1 - Y0) * 0.45f - 7.0f)) / 2.0f;   /* the skull worked in it */
            if (mx >= 0.0f && my >= 0.0f && mx < 7.0f && my < 7.0f && SK[(int)my][(int)mx] == '#')
                col = sh(R_GOLD, 0.18f + 0.14f * fmaxf(0.0f, sph) + 0.06f * (((x + y) & 1) ? 1.0f : 0.0f));
            if (Wv - p < 0.6f) col = sh(R_BLOOD, 0.04f);                /* its leading edge */
            if (veil < 0.6f && fabsf(cy - (Y1 - Y0) * 0.6f) < 0.65f)    /* the cord that holds it back */
                col = sh(R_GOLD, 0.45f + 0.4f * (cy < (Y1 - Y0) * 0.6f ? 1.0f : 0.0f));
            pp(x, y, col);
        }
    }
    if (veil < 0.6f) {                                                  /* the cord's tassel */
        float tx = side ? X1 - Wv : X0 + Wv, ty = Y0 + (Y1 - Y0) * 0.6f;
        capsule(tx, ty, tx + (side ? -0.6f : 0.6f), ty + 3.5f, 0.5f, R_GOLD, 0.3f, 0.9f);
        ellipse(tx + (side ? -0.6f : 0.6f), ty + 4.2f, 0.9f, 1.1f, R_GOLD, 0.25f, 0.85f);
    }
    capsule(X0 - 1.0f, Y0 - 0.3f, X1 + 1.0f, Y0 - 0.3f, 0.6f, R_IRON, 0.12f, 0.7f);   /* the rod */
    ellipse(X0 - 1.6f, Y0 - 0.3f, 1.1f, 1.1f, R_IRON, 0.15f, 0.8f); ellipse(X1 + 1.6f, Y0 - 0.3f, 1.1f, 1.1f, R_IRON, 0.15f, 0.8f);
    for (int k = 0; k < 5; k++) {                                       /* its rings */
        float rx = side ? X1 - Wv * ((float)k + 0.5f) / 5.0f : X0 + Wv * ((float)k + 0.5f) / 5.0f;
        ellipse(rx, Y0 - 0.2f, 0.7f, 0.9f, R_IRON, 0.2f, 0.7f);
    }
}

void portrait_face(uint8_t *pix, int stride, int w, int h, int ox, int oy, int s, const PortraitView *v) {
    P = pix; PST = stride; PW = w; PH = h; POX = ox; POY = oy; S = s < 1 ? 1 : s; PT = v->t;
    g_nfl = 0;
    int r[4];
    portrait_frame_rect(v->layout, r);
    layout_lights(v->layout, 1);
    if (v->veil < 1.0f) {                                               /* the tapestry, all the way down, hides both */
        picture(r, v);
        float fx = (float)(r[0] + r[2]) * 0.5f, fy = (float)(r[1] + r[3]) * 0.5f;
        static const float LX[NPLAYOUT] = { 54.5f, 9.5f, 32.0f, 32.0f, 32.0f, 9.5f, 55.5f }, LY[NPLAYOUT] = { 24, 24, 20, 5, 47, 26, 22 };
        frame(r, v->frame, LX[v->layout] - fx - 0.0f, LY[v->layout] - fy - 18.0f, v);
    }
    if (v->veil >= 0.0f) {                                              /* below 0: none at all */
        static const int SIDE[NPLAYOUT] = { 0, 1, 0, 0, 0, 1, 0 };     /* gathered away from its candle */
        tapestry(r, SIDE[v->layout], v->veil);
    }
    layout_lights(v->layout, 0);
}
int portrait_flames(const PortraitView *v, float out[4][3]) {
    for (int i = 0; i < g_nfl; i++) { out[i][0] = g_fl[i].x; out[i][1] = g_fl[i].y - g_fl[i].h * 0.4f; out[i][2] = g_fl[i].power; }
    return g_nfl;
}

/* ---- each run's stone and floor, kept: her hall is always drawn in level 1's art, so the others are made once and kept ---- */
static uint8_t RUN_WALL[4][TS * TS], RUN_FLOOR[4][TS * TS];
void portrait_keep_runs(void) {
    static int kept;
    if (kept) return;
    kept = 1;
    for (int r = 0; r < 4; r++) crawl_art_stone(5 + 10 * r, RUN_WALL[r], RUN_FLOOR[r]);   /* each run at its fifth level, a little worn */
}
const uint8_t *portrait_run_wall(int run) { return RUN_WALL[clampi_(run, 0, 3)]; }
const uint8_t *portrait_run_floor(int run) { return RUN_FLOOR[clampi_(run, 0, 3)]; }
