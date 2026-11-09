/* The title screen's art (docs/PLAN-title.md): drawn as the game draws, in mode 13h's terms (320x200, one palette, light
 * as a step in a ramp, lightning as the DAC rewritten), from the game's own palette, textures, sprites and faces. Three
 * scenes come round in turn: A, BENEATH (a graveyard in cross-section, a walker with a lantern in the crypt under it); B,
 * THE WALK (run I's hall in the first person through an arch, the name cut in its lintel); C, THE RISEN (one great stone,
 * the name cut in it, a hand out of the grave). Each has its menu after the key and its screen of graves. The game calls
 * title_draw with what it wants shown; tools/titlemock.c drives the same for the mock-ups. */
#include "crawl_title.h"
#include "crawl_ui.h"
#include "gfx.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define W 320
#define H 200
#define PI 3.14159265f

static Frame *TF;                                       /* the frame drawn into */
#define F (*TF)
/* the input in use: the prompt and the hints follow it (the owner, 9 November) */
static int PADMODE;
static const char *ver_word;                            /* the build, in A's corner, or 0 */
static const char *any_prompt(void) { return PADMODE ? "PRESS ANY BUTTON" : "PRESS ANY KEY"; }
static float LOOP;                                      /* the scene's loop: everything that moves comes round in it */
static uint8_t BASEPAL[256][3];                         /* the game's palette as it stands */

static float frac(float v) { return v - floorf(v); }
static float smooth(float a, float b, float v) { float t = clampf((v - a) / (b - a), 0, 1); return t * t * (3 - 2 * t); }
static uint32_t hsh(int x, int y, uint32_t s) {
    uint32_t h = (uint32_t)x * 374761393u + (uint32_t)y * 668265263u + s * 2246822519u;
    h = (h ^ (h >> 13)) * 1274126177u;
    return h ^ (h >> 16);
}
static float h01(int x, int y, uint32_t s) { return (float)(hsh(x, y, s) & 0xFFFF) / 65535.0f; }
static float vnoise(float x, float y, uint32_t s) {
    int x0 = (int)floorf(x), y0 = (int)floorf(y);
    float fx = x - x0, fy = y - y0;
    fx = fx * fx * (3 - 2 * fx); fy = fy * fy * (3 - 2 * fy);
    float a = h01(x0, y0, s), b = h01(x0 + 1, y0, s), c = h01(x0, y0 + 1, s), d = h01(x0 + 1, y0 + 1, s);
    return (a + (b - a) * fx) * (1 - fy) + (c + (d - c) * fx) * fy;
}
static float fbm(float x, float y, uint32_t s) {
    float v = 0, amp = 0.5f;
    for (int o = 0; o < 4; o++) { v += amp * vnoise(x, y, s + (uint32_t)o * 77u); x *= 2.03f; y *= 2.03f; amp *= 0.5f; }
    return v / 0.9375f;
}
/* periodic in x with period p (cells of noise), for things that drift and must come round with the loop */
static float vnoise_px(float x, float y, int p, uint32_t s) {
    int x0 = (int)floorf(x), y0 = (int)floorf(y);
    float fx = x - x0, fy = y - y0;
    fx = fx * fx * (3 - 2 * fx); fy = fy * fy * (3 - 2 * fy);
    int ax = ((x0 % p) + p) % p, bx = (ax + 1) % p;
    float a = h01(ax, y0, s), b = h01(bx, y0, s), c = h01(ax, y0 + 1, s), d = h01(bx, y0 + 1, s);
    return (a + (b - a) * fx) * (1 - fy) + (c + (d - c) * fx) * fy;
}
static float fbm_px(float x, float y, int p, uint32_t s) {
    float v = 0, amp = 0.5f;
    for (int o = 0; o < 4; o++) { v += amp * vnoise_px(x, y, p, s + (uint32_t)o * 77u); x *= 2; y *= 2; p *= 2; amp *= 0.5f; }
    return v / 0.9375f;
}
static const uint8_t BAYER[4][4] = { { 0, 8, 2, 10 }, { 12, 4, 14, 6 }, { 3, 11, 1, 9 }, { 15, 7, 13, 5 } };
static float bay(int x, int y) { return (BAYER[y & 3][x & 3] + 0.5f) / 16.0f; }

/* ---- the frame as materials and light (13h has no light: a colour is a step in a ramp, so the light picks the step) ---- */
enum { MF_FLAT = 1, MF_DIRECT = 2, MF_NOWARM = 4 };
static uint8_t MR[H][W], MF[H][W], MD[H][W];             /* the ramp; flags; a colour as it is (MF_DIRECT) */
static float MV[H][W], LC[H][W], LW[H][W];               /* the stuff's own value 0..1; cold light and warm, 1 as drawn */
static int warm_of(int ramp) { return ramp == R_STONE ? R_WARM : ramp == R_CEIL ? R_WARM : ramp == R_IRON ? R_WARM : ramp; }
static void matf(int x, int y, int ramp, float v, int flags) { if ((unsigned)x < W && (unsigned)y < H) { MR[y][x] = (uint8_t)ramp; MV[y][x] = v; MF[y][x] = (uint8_t)flags; } }
static void direct(int x, int y, uint8_t c) { if ((unsigned)x < W && (unsigned)y < H) { MF[y][x] = MF_DIRECT; MD[y][x] = c; } }
static void light_clear(float cold, float warm) { for (int y = 0; y < H; y++) for (int x = 0; x < W; x++) { LC[y][x] = cold; LW[y][x] = warm; } }
static void light_add(float cx, float cy, float r, float k, int warm) {   /* a soft round light */
    int x0 = clampi((int)(cx - r), 0, W - 1), x1 = clampi((int)(cx + r), 0, W - 1), y0 = clampi((int)(cy - r), 0, H - 1), y1 = clampi((int)(cy + r), 0, H - 1);
    for (int y = y0; y <= y1; y++) for (int x = x0; x <= x1; x++) {
        float d = sqrtf((x - cx) * (x - cx) + (y - cy) * (y - cy)) / r;
        if (d >= 1) continue;
        float a = (1 - d) * (1 - d) * k;
        if (warm) LW[y][x] += a; else LC[y][x] += a;
    }
}
static void resolve(void) {
    for (int y = 0; y < H; y++) for (int x = 0; x < W; x++) {
        int fl = MF[y][x];
        if (fl & MF_DIRECT) { F.pix[y * W + x] = MD[y][x]; continue; }
        int r = MR[y][x];
        if (r == 0) { F.pix[y * W + x] = 0; continue; }
        float lc = LC[y][x], lw = LW[y][x], l = lc + lw;
        float v = MV[y][x] * l * 15.0f;
        float d = fl & MF_FLAT ? 0.5f : bay(x, y);
        int ramp = r;
        if (!(fl & MF_NOWARM) && lw > 0.02f && warm_of(r) != r && lw / l > 0.25f + 0.5f * bay(x + 2, y + 1)) ramp = warm_of(r);
        int n = ramp == R_GOLD ? NGOLD : 16;
        int k = (int)(v * (n - 1) / 15.0f + d);
        if (k <= 0 && v < 0.35f) { F.pix[y * W + x] = 0; continue; }
        F.pix[y * W + x] = (uint8_t)(ramp + clampi(k, 0, n - 1));
    }
}

/* ---- the palette: the game's, at level 3, with ramps lent as a menu lends them ---- */
static void ramp3(uint8_t pal[256][3], int base, int n, const int a[3], const int m[3], const int b[3]) {
    for (int i = 0; i < n; i++) {
        float t = i / (float)(n - 1), u;
        const int *p, *q;
        if (t < 0.5f) { p = a; q = m; u = t * 2; } else { p = m; q = b; u = (t - 0.5f) * 2; }
        for (int c = 0; c < 3; c++) pal[base + i][c] = (uint8_t)clampi((int)(p[c] + (q[c] - p[c]) * u + 0.5f), 0, 63);
    }
}
static void pal_night(void) {                          /* R_CEIL the night sky, R_FLOOR the earth */
    memcpy(F.pal, BASEPAL, sizeof F.pal);
    static const int SA[3] = { 0, 0, 3 }, SM[3] = { 7, 8, 19 }, SB[3] = { 30, 33, 47 };
    static const int EA[3] = { 2, 1, 1 }, EM[3] = { 15, 10, 7 }, EB[3] = { 38, 29, 21 };
    ramp3(F.pal, R_CEIL, 16, SA, SM, SB);
    ramp3(F.pal, R_FLOOR, 16, EA, EM, EB);
}
static void pal_flash(float k) {                       /* lightning: the sky's ramp and the stone's raised in the DAC */
    if (k <= 0) return;
    for (int i = 0; i < 16; i++) for (int c = 0; c < 3; c++) {
        int v = F.pal[R_CEIL + i][c];
        F.pal[R_CEIL + i][c] = (uint8_t)clampi((int)(v + (54 - v) * k * (0.25f + 0.04f * i) + (c == 2 ? 4 * k : 0)), 0, 63);
        v = F.pal[R_STONE + i][c];
        F.pal[R_STONE + i][c] = (uint8_t)clampi((int)(v + (56 - v) * k * 0.18f), 0, 63);
    }
}

/* ======================================================================================================================
 * The name: HAUNTED.EXE in a broken gothic hand. Each letter is a few strokes of a broad nib held at 35 degrees (the
 * stems come out thick with their ends cut on the slant, what climbs to the right thin), with thorns and hairlines;
 * filled at four by four under each pixel and kept where half is covered.
 * ====================================================================================================================== */
typedef struct { float L; int n; float p[14]; } Stroke;          /* the nib's length (0: a polygon), points as x, y */
typedef struct { char ch; float w; Stroke s[9]; } Glyph;
static const Glyph GLYPHS[] = {
    { 'H', 29, { { 9, 2, { 6, 1, 6, 40 } }, { 9, 2, { 22, 5, 22, 40 } }, { 6, 2, { 6, 22, 22, 19 } },
                 { 2.2f, 3, { 6, 2, 2, 5, 0, 10 } }, { 0, 3, { 2.4f, 24, -1.8f, 27.5f, 2.4f, 30 } },
                 { 0, 3, { 25.6f, 12, 29.5f, 15, 25.6f, 18 } }, { 2.2f, 3, { 22, 6, 25, 2, 27, 0 } } } },
    { 'A', 29, { { 6, 2, { 2.5f, 39, 14, 2 } }, { 9, 3, { 13, 2, 22, 8, 22, 40 } }, { 5.5f, 2, { 8, 26, 22, 23 } },
                 { 2.2f, 2, { -1, 40, 7, 40 } }, { 0, 3, { 25.6f, 16, 29.5f, 19, 25.6f, 22 } },
                 { 2.2f, 2, { 22, 40, 28, 37 } } } },
    { 'U', 29, { { 9, 4, { 6, 3, 6, 32, 12, 40, 18, 37 } }, { 9, 2, { 22, 1, 22, 40 } },
                 { 2.2f, 3, { 6, 4, 2, 6, 0, 11 } }, { 0, 3, { 2.4f, 18, -1.8f, 21.5f, 2.4f, 24 } },
                 { 2.2f, 2, { 22, 40, 28, 37 } } } },
    { 'N', 29, { { 9, 2, { 6, 5, 6, 40 } }, { 5, 2, { 7, 3, 21, 36 } }, { 9, 2, { 22, 1, 22, 37 } },
                 { 2.2f, 3, { 6, 6, 2, 8, 0, 13 } }, { 0, 3, { 25.6f, 22, 29.5f, 25, 25.6f, 28 } },
                 { 2.2f, 2, { -1, 40, 6, 40 } } } },
    { 'T', 30, { { 7, 3, { 0.5f, 6, 14, 2, 29.5f, 5 } }, { 9, 2, { 15, 3, 15, 40 } },
                 { 2.2f, 2, { 0.5f, 6, -0.5f, 13 } }, { 2.2f, 2, { 29.5f, 5, 30.5f, 12 } },
                 { 0, 3, { 18.6f, 20, 22.5f, 23, 18.6f, 26 } }, { 0, 3, { 11.4f, 26, 7.5f, 29, 11.4f, 32 } } } },
    { 'E', 27, { { 9, 6, { 22, 4, 13, 1, 6, 8, 6, 33, 13, 40, 23, 36 } }, { 6, 2, { 6, 21, 18, 19 } },
                 { 2.2f, 2, { 22, 4, 24, 10 } }, { 0, 3, { 18, 19.5f, 22.5f, 18, 18, 23.5f } } } },
    { 'D', 29, { { 9, 2, { 6, 1, 6, 40 } }, { 7.5f, 6, { 6, 1, 15, 2, 22.5f, 9, 22.5f, 32, 15, 39, 6, 40 } },
                 { 2.2f, 3, { 6, 2, 2, 5, 0, 10 } }, { 0, 3, { 2.4f, 24, -1.8f, 27.5f, 2.4f, 30 } } } },
    { 'X', 28, { { 9, 2, { 4, 1, 23, 40 } }, { 4, 2, { 23, 1, 4, 40 } }, { 2.2f, 2, { -1, 40, 8, 40 } },
                 { 2.2f, 2, { 19, 1, 28, 1 } } } },
    { '.', 12, { { 0, 4, { 6, 29, 10.5f, 34, 6, 39, 1.5f, 34 } } } },
};
static const Glyph *glyph_of(char c) { for (size_t i = 0; i < sizeof GLYPHS / sizeof GLYPHS[0]; i++) if (GLYPHS[i].ch == c) return &GLYPHS[i]; return NULL; }

static int in_convex(const float *p, int n, float x, float y) {
    int pos = 0, neg = 0;
    for (int i = 0; i < n; i++) {
        float ax = p[2 * i], ay = p[2 * i + 1], bx = p[2 * ((i + 1) % n)], by = p[2 * ((i + 1) % n) + 1];
        float c = (bx - ax) * (y - ay) - (by - ay) * (x - ax);
        if (c > 1e-6f) pos = 1; else if (c < -1e-6f) neg = 1;
        if (pos && neg) return 0;
    }
    return 1;
}
#define LGW 320
#define LGH 96
typedef struct {
    int w, h;                                   /* the logo's box */
    uint8_t m[LGH][LGW];                        /* in it */
    uint8_t letter[LGH][LGW];                   /* which letter, 1 on */
    int8_t d[LGH][LGW];                         /* steps in from its edge */
    float nx[LGH][LGW], ny[LGH][LGW];           /* which way its surface leans */
    int ndrip;
    struct { int x, y, len, per, big; float ph; } drip[96];
} Logo;
static Logo LG;
static int lg_dripmax = 99;                                             /* the drips' longest, for a small name over words */
/* the name laid out: line 1 at scale s1, then line 2 (or on the same line when same is set) at s2 */
static void logo_build(const char *l1, float s1, const char *l2, float s2, int same, uint32_t seed) {
    memset(&LG, 0, sizeof LG);
    const int SS = 4;
    float gap = 2.5f;
    float w1 = 0, w2 = 0;
    for (const char *c = l1; *c; c++) w1 += (glyph_of(*c)->w + gap) * s1;
    for (const char *c = l2; *c; c++) w2 += (glyph_of(*c)->w + gap) * s2;
    w1 -= gap * s1; w2 -= gap * s2;
    float ox1, oy1 = 3, ox2, oy2;
    if (same) { LG.w = (int)(w1 + 4 * s1 + w2) + 8; ox1 = 4; ox2 = ox1 + w1 + 4 * s1; oy2 = oy1 + (40 - 40 * s2 / s1) * s1 * 0 + 40 * (s1 - s2); LG.h = (int)(40 * s1) + 8; }
    else { LG.w = (int)fmaxf(w1, w2) + 8; ox1 = 4 + (LG.w - 8 - w1) / 2; ox2 = LG.w - 4 - w2 - 6 * s1; oy2 = oy1 + 40 * s1 + 4; LG.h = (int)(oy2 + 40 * s2) + 5; }
    if (LG.w > LGW) LG.w = LGW;
    if (LG.h > LGH) LG.h = LGH;
    for (int line = 0; line < 2; line++) {
        const char *s = line ? l2 : l1;
        float sc = line ? s2 : s1, x = line ? ox2 : ox1, y = line ? oy2 : oy1;
        int li = line ? (int)strlen(l1) : 0;
        for (const char *c = s; *c; c++, li++) {
            const Glyph *g = glyph_of(*c);
            const float na = 35.0f * PI / 180.0f, nx = cosf(na), ny = -sinf(na);
            for (int k = 0; k < 9 && g->s[k].n; k++) {
                const Stroke *st = &g->s[k];
                float poly[16];
                int segs = st->L > 0 ? st->n - 1 : 1;
                for (int sgi = 0; sgi < segs; sgi++) {
                    int np;
                    if (st->L > 0) {
                        float ax = st->p[2 * sgi], ay = st->p[2 * sgi + 1], bx = st->p[2 * sgi + 2], by = st->p[2 * sgi + 3], hl = st->L * 0.5f;
                        float ex = nx * hl, ey = ny * hl;
                        if (st->L < 3) {                    /* a hairline: a pen of its own width across the way it goes */
                            float dx = bx - ax, dy = by - ay, l = sqrtf(dx * dx + dy * dy) + 1e-6f;
                            ex = -dy / l * hl; ey = dx / l * hl;
                        }
                        float q[8] = { ax - ex, ay - ey, ax + ex, ay + ey, bx + ex, by + ey, bx - ex, by - ey };
                        memcpy(poly, q, sizeof q); np = 4;
                    } else { memcpy(poly, st->p, sizeof(float) * 2 * st->n); np = st->n; }
                    float bx0 = 1e9f, by0 = 1e9f, bx1 = -1e9f, by1 = -1e9f;
                    for (int i = 0; i < np; i++) {
                        poly[2 * i] = x + poly[2 * i] * sc; poly[2 * i + 1] = y + poly[2 * i + 1] * sc;
                        bx0 = fminf(bx0, poly[2 * i]); bx1 = fmaxf(bx1, poly[2 * i]); by0 = fminf(by0, poly[2 * i + 1]); by1 = fmaxf(by1, poly[2 * i + 1]);
                    }
                    for (int py = (int)floorf(by0); py <= (int)ceilf(by1); py++)
                        for (int px = (int)floorf(bx0); px <= (int)ceilf(bx1); px++) {
                            if (px < 0 || py < 0 || px >= LG.w || py >= LG.h) continue;
                            int cov = 0;
                            for (int j = 0; j < SS; j++) for (int i = 0; i < SS; i++)
                                cov += in_convex(poly, np, px + (i + 0.5f) / SS, py + (j + 0.5f) / SS);
                            if (cov * 2 >= SS * SS) { LG.m[py][px] = 1; LG.letter[py][px] = (uint8_t)(li + 1); }
                        }
                }
            }
            x += (g->w + gap) * sc;
        }
    }
    /* steps in from the edge, and the lean of a surface rounded over two of them */
    for (int y = 0; y < LG.h; y++) for (int x = 0; x < LG.w; x++) LG.d[y][x] = LG.m[y][x] ? 9 : 0;
    for (int it = 0; it < 9; it++)
        for (int y = 0; y < LG.h; y++) for (int x = 0; x < LG.w; x++) {
            if (!LG.m[y][x]) continue;
            int best = 9;
            for (int j = -1; j <= 1; j++) for (int i = -1; i <= 1; i++) {
                int X = x + i, Y = y + j;
                int v = (X < 0 || Y < 0 || X >= LG.w || Y >= LG.h) ? 0 : LG.d[Y][X];
                if (v + 1 < best) best = v + 1;
            }
            LG.d[y][x] = (int8_t)best;
        }
    for (int y = 0; y < LG.h; y++) for (int x = 0; x < LG.w; x++) {
        if (!LG.m[y][x]) continue;
        float hx = 0, hy = 0;
        for (int j = -1; j <= 1; j++) for (int i = -1; i <= 1; i++) {
            int X = x + i, Y = y + j;
            float v = (X < 0 || Y < 0 || X >= LG.w || Y >= LG.h) ? 0 : fminf((float)LG.d[Y][X], 3.0f);
            hx += v * i; hy += v * j;
        }
        float l = sqrtf(hx * hx + hy * hy);
        LG.nx[y][x] = l > 0 ? -hx / l : 0; LG.ny[y][x] = l > 0 ? -hy / l : 0;
        if (LG.d[y][x] >= 3) LG.nx[y][x] = LG.ny[y][x] = 0;
    }
    /* drips: from the lowest edges, the feet first */
    LG.ndrip = 0;
    for (int x = 1; x < LG.w - 1 && LG.ndrip < 96; x++)
        for (int y = LG.h - 2; y > 0; y--) {
            if (!LG.m[y][x] || LG.m[y + 1][x]) continue;
            if (!LG.m[y][x - 1] || !LG.m[y][x + 1] || LG.m[y + 1][x - 1] || LG.m[y + 1][x + 1]) continue;
            float r = h01(x, y, seed);
            int foot = y > LG.h * 0.6f || (y > 30 && y < 50);
            if (r < (foot ? 0.42f : 0.12f)) {
                int k = LG.ndrip++;
                LG.drip[k].x = x; LG.drip[k].y = y + 1;
                LG.drip[k].big = r < (foot ? 0.16f : 0.03f);
                LG.drip[k].len = (LG.drip[k].big ? 9 : 3) + (int)(h01(x, y, seed + 1) * (LG.drip[k].big ? 24 : 9));
                if (LG.drip[k].len > lg_dripmax) LG.drip[k].len = lg_dripmax;
                LG.drip[k].per = 1 + (int)(h01(x, y, seed + 2) * 3);
                LG.drip[k].ph = h01(x, y, seed + 3);
                x += 3;
            }
            break;
        }
}

/* the name drawn: style 0 letters of blood; 1 cut into stone with blood in the cuts. lit: how much light falls on it;
 * fall: how far a drop may fall below it before it is lost (or lands); ground: the row it lands on, or -1 */
enum { LS_BLOOD, LS_CARVED };
static void logo_draw(int ox, int oy, int style, float lit, int fall_to, float t) {
    /* the shadow behind */
    if (style == LS_BLOOD)
        for (int y = 0; y < LG.h; y++) for (int x = 0; x < LG.w; x++) {
            if (!LG.m[y][x]) continue;
            for (int k = 1; k <= 3; k++) {
                int X = ox + x + k, Y = oy + y + k;
                if ((unsigned)X < W && (unsigned)Y < H && !(y + k < LG.h && x + k < LG.w && LG.m[y + k][x + k])) direct(X, Y, 0);
            }
        }
    for (int y = 0; y < LG.h; y++) for (int x = 0; x < LG.w; x++) {
        int X = ox + x, Y = oy + y;
        if (!LG.m[y][x]) {
            int edge = 0;
            for (int j = -1; j <= 1; j++) for (int i = -1; i <= 1; i++) {
                int a = x + i, b = y + j;
                if (a >= 0 && b >= 0 && a < LG.w && b < LG.h && LG.m[b][a]) edge = 1;
            }
            if (edge && style == LS_BLOOD) direct(X, Y, 0);
            if (style == LS_CARVED && edge) {                       /* the stone's lip: lit below and right of a cut */
                int lip = (y > 0 && LG.m[y - 1][x]) || (x > 0 && LG.m[y][x - 1]);
                if (lip && (unsigned)X < W && (unsigned)Y < H) MV[Y][X] = fminf(1.0f, MV[Y][X] + 0.22f);
            }
            continue;
        }
        float lx = -0.6f, ly = -0.8f;                                /* light from above, a little to the left */
        float dotl = LG.nx[y][x] * lx + LG.ny[y][x] * ly;
        int dd = LG.d[y][x];
        uint8_t c;
        if (style == LS_BLOOD) {
            float v = 0.42f + 0.30f * dotl - 0.05f * (float)(dd > 3);
            float sweep = frac(t / LOOP) * (LG.w + 160) - 80;        /* a gleam crossing the wet once a loop */
            float g = 1.0f - fabsf((x + y * 0.6f) - sweep) / 14.0f;
            if (g > 0) v += 0.28f * g * g;
            v *= lit;
            if (dd == 1 && dotl > 0.55f) c = (uint8_t)(R_SKIN + clampi((int)(9 + 6 * lit + (g > 0.6f ? 2 : 0)), 0, 15));   /* the wet catches it */
            else if (dd == 2 && dotl > 0.6f && h01(x, y, 5) < 0.25f) c = (uint8_t)(R_BLOOD + 15);
            else c = (uint8_t)(R_BLOOD + clampi((int)(v * 15 + bay(X, Y) * 0.8f), 1, 14));
            direct(X, Y, c);
        } else {
            /* the cut: its upper and left walls in shadow, the lower and right lit; blood lies in the bottom of it */
            float wallv = dotl > 0.15f ? 0.03f : dotl < -0.15f ? 0.2f : 0.1f;
            int blood = dd >= 2 || (dd == 1 && LG.d[y][x] && dotl < -0.5f && 0);
            if (blood) {
                float v = (0.5f + 0.2f * (-dotl) * 0.5f) * lit;
                float sweep = frac(t / LOOP) * (LG.w + 160) - 80;
                float g = 1.0f - fabsf((x + y * 0.6f) - sweep) / 12.0f;
                if (g > 0) v += 0.25f * g * g;
                if (dd == 2 && dotl > 0.4f) c = (uint8_t)(R_BLOOD + 4);
                else if (dd == 2 && dotl < -0.4f) c = (uint8_t)(R_BLOOD + clampi((int)(13 * lit), 3, 14));
                else c = (uint8_t)(R_BLOOD + clampi((int)(v * 15 + bay(X, Y) * 0.8f), 1, 13));
                direct(X, Y, c);
            } else if ((unsigned)X < W && (unsigned)Y < H) { MV[Y][X] = wallv; MF[Y][X] &= (uint8_t)~MF_DIRECT; }
        }
    }
    /* the drips: grow, swell, let go a drop, draw back a little, again; each comes round a whole number of times a loop */
    for (int k = 0; k < LG.ndrip; k++) {
        int dx = ox + LG.drip[k].x, dy = oy + LG.drip[k].y;
        float u = frac(t / LOOP * LG.drip[k].per + LG.drip[k].ph);
        float L = (float)LG.drip[k].len, len;
        if (u < 0.7f) len = L * (0.35f + 0.65f * smooth(0, 0.7f, u));
        else if (u < 0.78f) len = L;
        else len = L * (1.0f - 0.65f * smooth(0.78f, 1.0f, u));
        int il = (int)len, bulb = u > 0.55f && u < 0.79f ? 2 : 1;
        for (int j = 0; j < il; j++) {
            direct(dx, dy + j, (uint8_t)(R_BLOOD + clampi((int)(7 * lit) - (j > il - 2 ? 0 : 1), 2, 12)));
            if (LG.drip[k].big && j < il - 3) direct(dx + 1, dy + j, (uint8_t)(R_BLOOD + clampi((int)(5 * lit), 1, 10)));
            if (j == 0 || (j < il / 2 && LG.drip[k].big)) direct(dx - 1, dy + j, (uint8_t)(R_BLOOD + clampi((int)(9 * lit), 2, 13)));
        }
        if (il > 0) {                                               /* the bead at its end, and its gleam */
            int by = dy + il;
            for (int j = 0; j < bulb + 1; j++) for (int i = -bulb + 1; i <= bulb - (bulb > 1 ? 0 : 1); i++)
                direct(dx + i, by + j - 1, (uint8_t)(R_BLOOD + clampi((int)(8 * lit), 2, 12)));
            direct(dx - (bulb > 1), by - 1, (uint8_t)(R_SKIN + clampi((int)(10 * lit), 4, 15)));
            if (style == LS_BLOOD) direct(dx, by + bulb, 0);
        }
        if (u >= 0.78f && fall_to > 0) {                           /* the drop let go */
            float ft = (u - 0.78f) * LOOP / LG.drip[k].per;
            float fy = dy + L + 0.5f * 260.0f * ft * ft;
            if (fy < fall_to) {
                int fyi = (int)fy;
                direct(dx, fyi, (uint8_t)(R_BLOOD + clampi((int)(9 * lit), 3, 13)));
                direct(dx, fyi - 1, (uint8_t)(R_BLOOD + clampi((int)(6 * lit), 2, 10)));
                if (ft > 0.12f) direct(dx, fyi - 2, (uint8_t)(R_BLOOD + clampi((int)(4 * lit), 1, 8)));
            } else {                                                /* a splash where it lands, a moment */
                float land = sqrtf(2.0f * (fall_to - dy - L) / 260.0f);
                float st = ft - land;
                if (st < 0.25f) {
                    int r = 1 + (int)(st * 12);
                    direct(dx - r, fall_to - (r > 1), (uint8_t)(R_BLOOD + 6)); direct(dx + r, fall_to - (r > 1), (uint8_t)(R_BLOOD + 6));
                    direct(dx, fall_to, (uint8_t)(R_BLOOD + 4));
                }
            }
        }
    }
}

/* the prompt: inlaid words that breathe in and out slowly (no blinking), on a band of dark stone if asked */
static void prompt(int cx, int y, const char *s, int band, float t, int ramp) {
    int w = (int)strlen(s) * 8;
    if (band) ui_band(&F, cx - w / 2 - 8, y - 4, w + 16, 16);
    float b = 0.5f + 0.5f * sinf(2 * PI * t / (LOOP / roundf(LOOP / 3.0f)));
    int col = ramp + clampi((int)(8 + 6 * b), 0, 15);
    ui_text8(&F, cx - w / 2, y, s, FACE_INSC, col, TX_INLAID);
}

/* ---- the night sky, its moon and clouds ---- */
static float flash_k(float t, float at) {                /* lightning: a flash, a pause, a second, and its fading */
    float u = t - at;
    if (u < 0) u += LOOP;
    if (u < 0.08f) return 1.0f;
    if (u < 0.18f) return 0.15f;
    if (u < 0.30f) return 0.75f;
    if (u < 0.9f) return 0.75f * (1 - smooth(0.3f, 0.9f, u));
    return 0;
}
static void sky(int y0, int y1, float t, float mx, float my, float mr, float flash) {
    for (int y = y0; y < y1; y++) for (int x = 0; x < W; x++) {
        float v = 0.06f + 0.30f * (float)(y - y0) / (float)(y1 - y0);
        float d = sqrtf((x - mx) * (x - mx) + (y - my) * (y - my));
        if (d < mr * 4.5f) v += 0.32f * powf(1 - d / (mr * 4.5f), 2);   /* the moon's halo */
        matf(x, y, R_CEIL, v, MF_NOWARM);
    }
    for (int i = 0; i < 70; i++) {                                     /* stars, a few breathing */
        int sx = (int)(h01(i, 1, 77) * W), sy = y0 + (int)(h01(i, 2, 77) * (y1 - y0) * 0.8f);
        float tw = h01(i, 3, 77) < 0.4f ? 0.5f + 0.5f * sinf(2 * PI * (t / LOOP * (1 + i % 3) + h01(i, 4, 77))) : 1;
        float dm = sqrtf((sx - mx) * (sx - mx) + (sy - my) * (sy - my));
        if (dm < mr * 2.5f) continue;
        matf(sx, sy, R_CEIL, 0.55f + 0.45f * tw * h01(i, 5, 77), MF_NOWARM | MF_FLAT);
    }
    for (int y = (int)(my - mr - 1); y <= (int)(my + mr + 1); y++)      /* the moon: craters, lit from the right */
        for (int x = (int)(mx - mr - 1); x <= (int)(mx + mr + 1); x++) {
            float dx = (x - mx) / mr, dy = (y - my) / mr, r2 = dx * dx + dy * dy;
            if (r2 > 1 || y < y0) continue;
            float n = fbm(x * 0.18f, y * 0.18f, 41);
            float v = 0.72f + 0.25f * n + 0.18f * dx - 0.25f * r2 * r2;
            if (n > 0.62f) v -= 0.18f;
            matf(x, y, R_BONE, clampf(v, 0.3f, 1.0f), MF_NOWARM | MF_FLAT);
        }
    for (int layer = 0; layer < 2; layer++) {                          /* clouds drifting left, coming round with the loop */
        int per = layer ? 5 : 4;
        float sp = (float)(W) / LOOP * (layer ? 1 : 2) / 1.0f;
        for (int y = y0; y < y1; y++) for (int x = 0; x < W; x++) {
            /* drifting, and seamless round the loop: two drifts a loop apart, crossfaded */
            float fy = y * 0.045f + layer * 3, a1 = (x + t * sp * 0.25f) / (float)W * per, a0 = (x + (t - LOOP) * sp * 0.25f) / (float)W * per;
            float c = fbm_px(a1, fy, per, 91 + layer * 13) * (1 - t / LOOP) + fbm_px(a0, fy, per, 91 + layer * 13) * (t / LOOP);
            float band = 1 - fabsf((float)(y - (y0 + (y1 - y0) * (layer ? 0.55f : 0.3f))) / ((y1 - y0) * 0.25f));
            c = c * clampf(band, 0, 1) - 0.36f;
            if (c <= 0) continue;
            float a = clampf(c * 4, 0, 1);
            if (a < bay(x, y) * 0.9f) continue;
            float d = sqrtf((x - mx) * (x - mx) + (y - my) * (y - my));
            float rim = d < mr * 5 ? 0.25f * (1 - d / (mr * 5)) : 0;
            matf(x, y, R_CEIL, 0.14f + 0.16f * layer + rim + 0.1f * (1 - a), MF_NOWARM);
        }
    }
    (void)flash;
}

/* fog: dithered drifting bands of pale stone */
static void fog(int y0, int y1, float t, float dens, uint32_t seed) {
    for (int y = y0; y < y1; y++) for (int x = 0; x < W; x++) {
        float fy = y * 0.08f, f1 = fbm_px((x + t * 9) / (float)W * 3, fy, 3, seed), f0 = fbm_px((x + (t - LOOP) * 9) / (float)W * 3, fy, 3, seed);
        float f = (f1 * (1 - t / LOOP) + f0 * (t / LOOP)) * smooth((float)y0, (float)y0 + (y1 - y0) * 0.5f, (float)y) * (1 - smooth((float)y1 - 6, (float)y1, (float)y));
        f = f * dens - 0.5f;
        if (f > bay(x, y) * 0.5f && !(MF[y][x] & MF_DIRECT)) { MR[y][x] = R_STONE; MV[y][x] = fmaxf(MV[y][x], 0.3f + 0.4f * f); MF[y][x] = MF_NOWARM; }
    }
}

/* ---- shapes in the materials ---- */
static void mdisc(float cx, float cy, float r, int ramp, float v, int flags) {
    for (int y = (int)floorf(cy - r); y <= (int)ceilf(cy + r); y++) for (int x = (int)floorf(cx - r); x <= (int)ceilf(cx + r); x++)
        if ((x + 0.5f - cx) * (x + 0.5f - cx) + (y + 0.5f - cy) * (y + 0.5f - cy) <= r * r) matf(x, y, ramp, v, flags);
}
static void mline(float x0, float y0, float x1, float y1, float w, int ramp, float v, int flags) {
    float dx = x1 - x0, dy = y1 - y0, l = sqrtf(dx * dx + dy * dy);
    int n = (int)(l * 2) + 1;
    for (int i = 0; i <= n; i++) { float u = (float)i / n; mdisc(x0 + dx * u, y0 + dy * u, w * 0.5f, ramp, v, flags); }
}
static void mrect(int x, int y, int w, int h, int ramp, float v, int flags) { for (int j = 0; j < h; j++) for (int i = 0; i < w; i++) matf(x + i, y + j, ramp, v, flags); }


/* a dead tree: a trunk and its branches, forked again and again */
static void branch(float x, float y, float ang, float len, float w, int depth, uint32_t seed) {
    float x1 = x + cosf(ang) * len, y1 = y - sinf(ang) * len;
    mline(x, y, x1, y1, w, R_WOOD, 0.12f, 0);
    if (depth <= 0 || w < 0.9f) return;
    int n = 2 + (h01(depth, (int)len, seed) < 0.3f);
    for (int i = 0; i < n; i++) {
        float da = (h01(i, depth, seed) - 0.5f) * 1.3f + (i - (n - 1) * 0.5f) * 0.55f;
        branch(x1, y1, ang + da, len * (0.62f + 0.2f * h01(i + 7, depth, seed)), w * 0.62f, depth - 1, seed * 31u + (uint32_t)i);
    }
}
/* a root going down into the earth, forking */
static void root(float x, float y, float ang, float len, float w, int depth, uint32_t seed) {
    float px = x, py = y;
    int n = (int)(len / 3) + 1;
    for (int i = 1; i <= n; i++) {
        float u = (float)i / n;
        float a = ang + 0.5f * sinf(u * 5 + seed % 7);
        float nx = px + cosf(a) * 3, ny = py + sinf(a) * 3;
        mline(px, py, nx, ny, w * (1 - 0.6f * u), R_WOOD, 0.3f - 0.1f * u, 0);
        px = nx; py = ny;
    }
    if (depth > 0) for (int i = 0; i < 2; i++) root(px, py, ang + (i ? 0.6f : -0.6f) + (h01(i, depth, seed) - 0.5f), len * 0.6f, w * 0.6f, depth - 1, seed * 17u + (uint32_t)i);
}
/* a small skull in the earth, or a bone */
static void skull_small(int x, int y, float v) {
    static const char *const S[6] = { ".###.", "#####", "#.#.#", "#####", ".#.#.", ".###." };
    for (int j = 0; j < 6; j++) for (int i = 0; i < 5; i++) if (S[j][i] == '#') matf(x + i, y + j, R_BONE, v - 0.04f * j + (i == 0 ? -0.05f : 0), 0);
}
static void bone_small(int x, int y, int len, int dir, float v) {
    for (int i = 0; i < len; i++) matf(x + i, y + dir * i / 3, R_BONE, v, 0);
    matf(x - 1, y - 1, R_BONE, v, 0); matf(x - 1, y + 1, R_BONE, v, 0);
    matf(x + len, y + dir * len / 3 - 1, R_BONE, v, 0); matf(x + len, y + dir * len / 3 + 1, R_BONE, v, 0);
}
/* a coffin, from the side: a long six-sided box, its lid's edge */
static void coffin_side(int x, int y, int w, int h, float v, float open) {
    for (int j = 0; j < h; j++) for (int i = 0; i < w; i++) {
        float u = (float)i / (w - 1);
        int taper = (int)(h * 0.25f * (u < 0.3f ? (0.3f - u) / 0.3f : u > 0.85f ? (u - 0.85f) / 0.15f : 0));
        if (j < taper || j >= h - taper) continue;
        float vv = v + (j == 0 || j == 1 ? 0.08f : 0) - (j == h - 1 ? 0.08f : 0) + 0.05f * h01(i / 4, j, 3);
        int lid = j == 2;
        matf(x + i, y + j - (int)(open * (u > 0.4f ? (u - 0.4f) * 6 : 0)) * (j < 3), R_WOOD, lid ? vv - 0.1f : vv, 0);
    }
}
/* a bat across the sky: two wings flapping */
static void bat(float x, float y, float ph) {
    int up = sinf(ph) > 0;
    mrect((int)x - 1, (int)y, 3, 2, R_IRON, 0.05f, MF_FLAT);
    for (int i = 1; i <= 4; i++) {
        int dy = up ? -(i / 2) : (i / 2);
        matf((int)x - 1 - i, (int)y + dy, R_IRON, 0.05f, MF_FLAT);
        matf((int)x + 1 + i, (int)y + dy, R_IRON, 0.05f, MF_FLAT);
    }
}
/* a flame and its flicker: a few pixels at full brightness, and the light round it */
static float flick(float t, int seed) {
    return 0.82f + 0.1f * sinf(t * 17.0f + seed) + 0.06f * sinf(t * 29.0f + seed * 3) + 0.05f * sinf(t * 7.0f + seed * 5);
}
static void flame(int x, int y, float h, float t, int seed) {
    int hh = (int)(h * (0.85f + 0.2f * sinf(t * 13 + seed)));
    for (int j = 0; j < hh; j++) {
        float u = (float)j / hh;
        int w = u < 0.35f ? 1 : u < 0.75f ? 1 : 0;
        int sway = (int)roundf(sinf(t * 9 + seed + j * 0.7f) * u * 1.2f);
        for (int i = -w; i <= w; i++) direct(x + i + sway, y - j, (uint8_t)(R_FLAME + clampi((int)(15 - u * 9 - abs(i) * 4), 2, 15)));
    }
}

/* ======================================================================================================================
 * The sculpt kit (docs/PLAN-title.md, step 2): a canvas for things built of parts. Each part has a height (raised, the
 * face, sunk) and a stuff (stone, wood, iron, ivy, rope); shading gives bevels and cast shadows by height, seams where parts
 * meet, the walls of what is cut; then breaks (fresh and pale where they are new), cracks with a lit lip, chips along every
 * edge, weathering, and words cut in it that gild while chosen. It is drawn into the frame upright, leaning or turned.
 * ====================================================================================================================== */
extern unsigned char ui_face8[NFACE][128][8], ui_face16[128][16];
extern unsigned char ui_has8[NFACE][128], ui_has16[128];
#define SCW 320
#define SCH 200
enum { SM_STONE, SM_WOOD, SM_WOODV, SM_IRON, SM_IVY, SM_ROPE };
enum { SF_GILD = 1, SF_CALM = 2, SF_FRESH = 4, SF_CRACK = 8, SF_PALE = 16 };
static struct {
    int w, h;
    uint8_t id[SCH][SCW], m[SCH][SCW], f[SCH][SCW];
    int8_t ht[SCH][SCW];
    float dv[SCH][SCW];
    uint8_t r[SCH][SCW], d[SCH][SCW];
    float v[SCH][SCW];
    float gk[SCH][SCW];                                                 /* 0: the canvas's gilding; else 1 + this word's own */
} S;
static float sc_gk;                                                     /* set before sc_text: the words' own gilding, 1 + k */
static void sc_begin(int w, int h) { memset(&S, 0, sizeof S); S.w = w < SCW ? w : SCW; S.h = h < SCH ? h : SCH; }
static int sc_in(int i, int j) { return i >= 0 && j >= 0 && i < S.w && j < S.h; }
static int sc_solid(int i, int j) { return sc_in(i, j) && S.id[j][i]; }
static void sc_px(int i, int j, int id, int ht, int m, float dv) {
    if (!sc_in(i, j)) return;
    S.id[j][i] = (uint8_t)id; S.ht[j][i] = (int8_t)ht; S.m[j][i] = (uint8_t)m; S.dv[j][i] = dv; S.f[j][i] &= SF_CALM;
}
static void sc_rect(int x, int y, int w, int h, int id, int ht, int m, float dv) { for (int j = 0; j < h; j++) for (int i = 0; i < w; i++) sc_px(x + i, y + j, id, ht, m, dv); }
static void sc_poly(const float *p, int n, int id, int ht, int m, float dv) {          /* any polygon, by crossings */
    float y0 = 1e9f, y1 = -1e9f, x0 = 1e9f, x1 = -1e9f;
    for (int k = 0; k < n; k++) { x0 = fminf(x0, p[2 * k]); x1 = fmaxf(x1, p[2 * k]); y0 = fminf(y0, p[2 * k + 1]); y1 = fmaxf(y1, p[2 * k + 1]); }
    for (int j = (int)floorf(y0); j <= (int)ceilf(y1); j++) for (int i = (int)floorf(x0); i <= (int)ceilf(x1); i++) {
        float px = i + 0.5f, py = j + 0.5f;
        int in = 0;
        for (int a = 0, b = n - 1; a < n; b = a++) {
            float ax = p[2 * a], ay = p[2 * a + 1], bx = p[2 * b], by = p[2 * b + 1];
            if ((ay > py) != (by > py) && px < (bx - ax) * (py - ay) / (by - ay) + ax) in = !in;
        }
        if (in) sc_px(i, j, id, ht, m, dv);
    }
}
static void sc_disc(float cx, float cy, float r, int id, int ht, int m, float dv) {
    for (int j = (int)floorf(cy - r); j <= (int)ceilf(cy + r); j++) for (int i = (int)floorf(cx - r); i <= (int)ceilf(cx + r); i++)
        if ((i + 0.5f - cx) * (i + 0.5f - cx) + (j + 0.5f - cy) * (j + 0.5f - cy) <= r * r) sc_px(i, j, id, ht, m, dv);
}
static void sc_ring(float cx, float cy, float r0, float r1, int id, int ht, int m, float dv) {
    for (int j = (int)floorf(cy - r1); j <= (int)ceilf(cy + r1); j++) for (int i = (int)floorf(cx - r1); i <= (int)ceilf(cx + r1); i++) {
        float d2 = (i + 0.5f - cx) * (i + 0.5f - cx) + (j + 0.5f - cy) * (j + 0.5f - cy);
        if (d2 <= r1 * r1 && d2 >= r0 * r0) sc_px(i, j, id, ht, m, dv);
    }
}
static void sc_line(float x0, float y0, float x1, float y1, float w, int id, int ht, int m, float dv) {
    float dx = x1 - x0, dy = y1 - y0, l = sqrtf(dx * dx + dy * dy);
    int n = (int)(l * 2) + 1;
    for (int k = 0; k <= n; k++) { float u = (float)k / n; sc_disc(x0 + dx * u, y0 + dy * u, w * 0.5f, id, ht, m, dv); }
}
/* an arch over [x0, x1] springing at yb, rise high at its middle: round, or pointed (two arcs meeting) */
static void sc_arch(int x0, int x1, int yb, float rise, int pointed, int id, int ht, int m, float dv) {
    float hw = (x1 - x0) * 0.5f, xm = (x0 + x1) * 0.5f;
    float pr = pointed ? hw * 1.732f : hw, R = (pr * pr + hw * hw) / (2 * hw), sq = rise / pr;   /* pointed: equilateral, pressed down */
    for (int i = x0; i < x1; i++) {
        float x = i + 0.5f, top;
        if (!pointed) top = rise * sqrtf(fmaxf(0, 1 - (x - xm) * (x - xm) / (hw * hw)));
        else if (pointed == 2) top = rise * powf(fmaxf(0, 1 - fabsf(x - xm) / hw), 0.62f);   /* a gable with bowed sides, sharp at its point */
        else { float c = x <= xm ? x0 + R : x1 - R; top = sq * sqrtf(fmaxf(0, R * R - (x - c) * (x - c))); }
        for (int j = (int)floorf(yb - top); j < yb; j++) sc_px(i, j, id, ht, m, dv);
    }
}
/* how far in from the edge of the parts listed (a mask of ids), up to 8 */
static uint8_t SDIST[SCH][SCW];
static void sc_dist(uint32_t ids) {
    for (int j = 0; j < S.h; j++) for (int i = 0; i < S.w; i++) SDIST[j][i] = (S.id[j][i] && (ids >> S.id[j][i] & 1)) ? 8 : 0;
    for (int it = 0; it < 8; it++)
        for (int j = 0; j < S.h; j++) for (int i = 0; i < S.w; i++) {
            if (!SDIST[j][i]) continue;
            int b = SDIST[j][i];
            if (i == 0 || j == 0 || i == S.w - 1 || j == S.h - 1) b = 1;
            else { int n = SDIST[j][i - 1]; if (SDIST[j][i + 1] < n) n = SDIST[j][i + 1]; if (SDIST[j - 1][i] < n) n = SDIST[j - 1][i]; if (SDIST[j + 1][i] < n) n = SDIST[j + 1][i]; if (n + 1 < b) b = n + 1; }
            SDIST[j][i] = (uint8_t)b;
        }
}
static void sc_groove(uint32_t ids, int at) {                         /* a line cut at this far in from the outline */
    sc_dist(ids);
    for (int j = 0; j < S.h; j++) for (int i = 0; i < S.w; i++) if (SDIST[j][i] == at) S.ht[j][i] = -1;
}
/* a mark of the game's own ('#' and 'o'), scale times, at a height: raised out of a sunk field, or cut into the face */
static void sc_mark(int x, int y, const char *const *mark, int n, int sc, int ht, float dv_o) {
    for (int j = 0; j < n; j++) for (int i = 0; mark[j][i]; i++) {
        if (mark[j][i] == '.') continue;
        for (int b = 0; b < sc * sc; b++) {
            int X = x + i * sc + b % sc, Y = y + j * sc + b / sc;
            if (!sc_solid(X, Y)) continue;
            S.ht[Y][X] = (int8_t)ht;
            S.f[Y][X] |= SF_CALM;
            if (mark[j][i] == 'o') S.dv[Y][X] += dv_o;
        }
    }
}
/* words cut in: sunk, a few of their pixels worn away, the stone round them quieter; gildable ones fill with gold */
static const unsigned char *g8(uint8_t c) { return c < 128 && ui_has8[FACE_INSC][c] ? ui_face8[FACE_INSC][c] : font_bios8[c]; }
static const unsigned char *g16(uint8_t c) { return c < 128 && ui_has16[c] ? ui_face16[c] : font_vga16[c]; }
static int sc_text_w(const char *s, int adv) { return (int)strlen(s) * adv - (adv - 7); }
static void sc_text(int x, int y, const char *s, int big, int adv, int gild, uint32_t seed) {
    int rows = big ? 16 : 8, n = (int)strlen(s);
    for (int j = -2; j < rows + 1; j++) for (int i = -2; i < n * adv + 1; i++) if (sc_solid(x + i, y + j)) S.f[y + j][x + i] |= SF_CALM;
    for (int k = 0; k < n; k++) {
        const unsigned char *g = big ? g16((uint8_t)s[k]) : g8((uint8_t)s[k]);
        for (int r = 0; r < rows; r++) for (int c = 0; c < 8; c++) {
            if (!(g[r] >> (7 - c) & 1)) continue;
            int X = x + k * adv + c, Y = y + r;
            if (!sc_solid(X, Y) || h01(X, Y, seed) < 0.022f) continue;
            S.ht[Y][X] = -1;
            if (gild) { S.f[Y][X] |= SF_GILD; S.gk[Y][X] = sc_gk; }
        }
    }
}
static void sc_text_c(int cx, int y, const char *s, int big, int gild, uint32_t seed, int maxw) {
    int adv = sc_text_w(s, 8) > maxw ? 7 : 8;                        /* a long name closes up a little */
    sc_text(cx - sc_text_w(s, adv) / 2, y, s, big, adv, gild, seed);
}
/* a break: a jagged bite out of it; what is left round the bite is fresh stone */
static void sc_break(float cx, float cy, float r, uint32_t seed) {
    for (int j = (int)(cy - r * 1.6f); j <= cy + r * 1.6f; j++) for (int i = (int)(cx - r * 1.6f); i <= cx + r * 1.6f; i++) {
        if (!sc_solid(i, j)) continue;
        float a = atan2f(j - cy, i - cx), d = sqrtf((i - cx) * (i - cx) + (j - cy) * (j - cy));
        float rr = r * (0.75f + 0.45f * vnoise(a * 2.2f + 9, 0.5f, seed) + 0.18f * h01((int)(a * 9), 0, seed));
        if (d < rr) S.id[j][i] = 0;
        else if (d < rr + 2.5f) S.f[j][i] |= SF_FRESH;
    }
}
/* a crack: a wandering line from a point, forking now and then; it goes under words rather than through them */
static void sc_crack(float x, float y, float ang, int len, uint32_t seed, int forks) {
    for (int k = 0; k < len; k++) {
        ang += (h01(k, 1, seed) - 0.5f) * 0.9f;
        x += cosf(ang); y += sinf(ang);
        int i = (int)x, j = (int)y;
        if (!sc_solid(i, j)) { if (k > 3) return; continue; }
        if (!(S.f[j][i] & SF_CALM)) S.f[j][i] |= SF_CRACK;
        if (forks > 0 && h01(k, 2, seed) < 0.06f) sc_crack(x, y, ang + (h01(k, 3, seed) < 0.5f ? 0.8f : -0.8f), len / 3, seed * 7u + (uint32_t)k, forks - 1);
    }
}
/* chips: little bites out of every edge; more where it stands proud */
static void sc_chips(float amt, uint32_t seed) {
    static uint8_t cut[SCH][SCW];
    memset(cut, 0, sizeof cut);
    for (int j = 0; j < S.h; j++) for (int i = 0; i < S.w; i++) {
        if (!S.id[j][i] || S.m[j][i] == SM_IVY || S.m[j][i] == SM_ROPE) continue;
        int edge = !sc_solid(i - 1, j) || !sc_solid(i + 1, j) || !sc_solid(i, j - 1) || !sc_solid(i, j + 1);
        int corner = (!sc_solid(i - 1, j) || !sc_solid(i + 1, j)) && (!sc_solid(i, j - 1) || !sc_solid(i, j + 1));
        if (!edge) continue;
        float n = vnoise(i * 0.45f, j * 0.45f, seed);
        if (n > 0.86f - amt * 0.3f || (corner && h01(i, j, seed) < amt)) cut[j][i] = 1;
    }
    for (int j = 0; j < S.h; j++) for (int i = 0; i < S.w; i++) if (cut[j][i]) {
        S.id[j][i] = 0;
        if (vnoise(i * 0.45f, j * 0.45f, seed) > 0.93f - amt * 0.3f)        /* a deeper chip: the next pixel in too */
            for (int d = 0; d < 4; d++) { static const int DX[4] = { 1, -1, 0, 0 }, DY[4] = { 0, 0, 1, -1 }; int X = i + DX[d], Y = j + DY[d]; if (sc_solid(X, Y) && h01(X, Y, seed + 1) < 0.5f) { S.id[Y][X] = 0; } }
    }
}
/* ivy: a stem wandering up, leaves either side */
static void sc_ivy(float x, float y, int len, uint32_t seed) {
    float ang = -PI / 2;
    for (int k = 0; k < len; k++) {
        ang += (h01(k, 1, seed) - 0.5f) * 0.6f; ang = clampf(ang, -PI * 0.68f, -PI * 0.36f);
        x += cosf(ang); y += sinf(ang);
        if (!sc_in((int)x, (int)y)) return;
        sc_px((int)x, (int)y, 200, 2, SM_IVY, -0.1f);
        if (k % 3 == 0) {
            int side = (k / 3) % 2 ? 1 : -1;
            float lx = x + side * 2.2f, ly = y + 0.5f;
            sc_disc(lx, ly, 1.6f + h01(k, 2, seed) * 0.8f, 200, 2, SM_IVY, 0.12f * h01(k, 3, seed));
        }
    }
}
/* weathered stone: grain, pits, water run down from the top, damp risen at the foot, lichen yellow at its heart, moss */
static float weathered(int i, int j, float v, uint32_t seed, int calm, int *ramp) {
    *ramp = R_STONE;
    float k = calm ? 0.35f : 1.0f, fy = (float)j / S.h;
    v += 0.12f * k * (fbm(i * 0.11f, j * 0.11f, seed) - 0.5f);
    v += 0.07f * (h01(i, j, seed + 1) - 0.5f);
    if (!calm && h01(i, j, seed + 2) < 0.04f) v -= 0.15f;
    else if (h01(i, j, seed + 3) < 0.015f) v += 0.1f;
    float streak = fbm(i * 0.5f, j * 0.03f, seed + 4);
    v -= 0.2f * k * smooth(0.5f, 0.78f, streak) * (1.0f - 0.6f * fy);
    float damp = smooth(0.72f, 0.98f, fy + 0.15f * (fbm(i * 0.07f, 3, seed + 5) - 0.5f));
    v -= 0.14f * damp;
    if (!calm) {
        float l = fbm(i * 0.16f + 7, j * 0.16f, seed + 6);
        if (l > 0.7f && h01(i, j, seed + 7) > 0.3f) {
            int core = l > 0.77f;
            *ramp = core ? R_GOLD : R_MOSS;
            return core ? 0.26f + 0.25f * (l - 0.77f) / 0.23f : 0.24f + 0.1f * h01(i, j, seed + 8);
        }
        if (damp > 0.5f && h01(i, j, seed + 9) < 0.35f * damp) { *ramp = R_MOSS; return 0.17f + 0.1f * h01(i, j, seed + 10); }
    }
    return v;
}
/* the shading: every pixel's ramp and value (or a colour as it is, for gold); gild 0..1 how far the words are gilt, in
 * gramp (R_GOLD, or R_BONE for a pale inlay) */
static void sc_shade(float bv, uint32_t seed, float gild, int gramp) {
#define HT(a, b) (sc_solid(a, b) ? S.ht[b][a] : -9)
    float gild0 = gild;
    for (int j = 0; j < S.h; j++) for (int i = 0; i < S.w; i++) {
        S.d[j][i] = 0;
        gild = gild0;
        if (!S.id[j][i]) continue;
        int h = S.ht[j][i], m = S.m[j][i], fl = S.f[j][i];
        if (S.gk[j][i] > 0) gild = S.gk[j][i] - 1;
        float v = bv + S.dv[j][i];
        float lit = 0;
        if (HT(i - 1, j) < h) lit += 0.13f; else if (HT(i - 2, j) < h) lit += 0.05f;
        if (HT(i, j - 1) < h) lit += 0.15f; else if (HT(i, j - 2) < h) lit += 0.06f;
        if (HT(i + 1, j) < h) lit -= 0.13f; else if (HT(i + 2, j) < h) lit -= 0.05f;
        if (HT(i, j + 1) < h) lit -= 0.16f; else if (HT(i, j + 2) < h) lit -= 0.06f;
        if (h < 0) {                                                    /* what is cut: dark, its far wall catching light */
            if (fl & SF_GILD) {                                         /* a letter: cut deep, so it reads */
                S.r[j][i] = m == SM_WOOD || m == SM_WOODV ? R_WOOD : R_STONE;
                S.v[j][i] = (HT(i + 1, j) > h || HT(i, j + 1) > h) ? 0.14f : 0.04f;
                if (!(gild > 0.01f && gild * 1.2f > bay(i, j))) continue;
            }
            v -= 0.2f;
            if (HT(i - 1, j) > h || HT(i, j - 1) > h) v -= 0.08f;
            if (HT(i + 1, j) > h || HT(i, j + 1) > h) v += 0.1f;
        }
        if (h == 0 && m != SM_IVY && ((sc_solid(i - 1, j - 1) && (S.f[j - 1][i - 1] & SF_GILD)) || (sc_solid(i, j - 1) && (S.f[j - 1][i] & SF_GILD) && S.ht[j - 1][i] < 0))) v += 0.12f;   /* a letter's lit lip */
        if (h == 0 && (HT(i - 1, j - 1) > 0 || HT(i - 2, j - 2) > 0 || HT(i - 1, j - 2) > 0)) v -= 0.12f;   /* a raised part's shadow */
        if (sc_solid(i, j + 1) && S.id[j + 1][i] != S.id[j][i] && S.ht[j + 1][i] == h && S.m[j + 1][i] == m) v -= 0.1f;   /* a joint */
        if (sc_solid(i + 1, j) && S.id[j][i + 1] != S.id[j][i] && S.ht[j][i + 1] == h && S.m[j][i + 1] == m) v -= 0.06f;
        v += lit;
        int ramp = R_STONE;
        if (m == SM_STONE) {
            if (fl & SF_CRACK) { S.r[j][i] = R_STONE; S.v[j][i] = 0.05f; continue; }
            if (sc_solid(i - 1, j - 1) && (S.f[j - 1][i - 1] & SF_CRACK)) v += 0.08f;   /* the crack's lip */
            if (fl & SF_FRESH) { v += 0.12f + 0.05f * (h01(i, j, seed + 11) - 0.5f); ramp = R_STONE; }
            else v = weathered(i, j, v, seed, (fl & SF_CALM) != 0 || h != 0, &ramp);
        } else if (m == SM_WOOD || m == SM_WOODV) {                     /* grain along the plank, knots, the ends gone dark */
            float a = m == SM_WOOD ? (float)j : (float)i, b = m == SM_WOOD ? (float)i : (float)j;
            float grain = sinf(a * 1.9f + fbm(b * 0.06f, a * 0.3f, seed) * 7.0f);
            v += 0.07f * grain + 0.05f * (h01(i, j, seed + 12) - 0.5f);
            float kn = vnoise(i * 0.25f, j * 0.25f, seed + 13);
            if (kn > 0.82f) v -= 0.18f * (kn - 0.82f) / 0.18f;
            if (fl & SF_CRACK) v = 0.04f;
            ramp = R_WOOD;
            if (fbm(i * 0.2f, j * 0.2f, seed + 14) > 0.66f) { ramp = R_STONE; v *= 0.85f; }   /* weathered silver */
        } else if (m == SM_IRON) {
            ramp = h01(i, j, seed + 15) < 0.25f ? R_BRICK : R_IRON;
            v = v * 0.8f + 0.05f * (h01(i, j, seed + 16) - 0.5f);
        } else if (m == SM_IVY) {
            ramp = R_MOSS; v = 0.3f + S.dv[j][i] + lit * 0.8f;
        } else if (m == SM_ROPE) {
            ramp = R_WOOD; v = 0.5f + lit * 0.6f - 0.15f * (((i + j) % 3) == 0);
        }
        if ((fl & SF_GILD) && gild > 0.01f && gild * 1.2f > bay(i, j)) {   /* the words fill with gold, a little at a time */
            int n = gramp == R_GOLD ? NGOLD : 16;
            int hi = (HT(i, j - 1) > h) ? -2 : (HT(i, j + 1) > h ? 2 : 0);
            S.d[j][i] = (uint8_t)(gramp + clampi((int)((n - 1) * (0.42f + 0.38f * gild)) + hi + (h01(i, j, seed) < 0.1f), 0, n - 1));
            continue;
        }
        S.r[j][i] = (uint8_t)ramp; S.v[j][i] = v;
    }
#undef HT
}
/* into the frame: the canvas's foot at (cx, by), turned by deg (plus leaning right) */
static void sc_draw(int cx, int by, float deg) {
    float a = deg * PI / 180.0f, ca = cosf(a), sa = sinf(a);
    if (fabsf(deg) < 3.5f) {                                            /* a small lean: sheared row by row, so words keep their shape */
        for (int j = 0; j < S.h; j++) {
            int sh = (int)roundf((S.h - j) * tanf(a));
            for (int i = 0; i < S.w; i++) {
                if (!S.id[j][i]) continue;
                int X = cx - S.w / 2 + i + sh, Y = by - S.h + j;
                if (S.d[j][i]) direct(X, Y, S.d[j][i]); else matf(X, Y, S.r[j][i], S.v[j][i], MF_FLAT);
            }
        }
        return;
    }
    float ext = sqrtf((float)(S.w * S.w + S.h * S.h)) + 2;
    for (int Y = by - (int)ext; Y <= by + (int)(ext * 0.5f); Y++) for (int X = cx - (int)ext; X <= cx + (int)ext; X++) {
        if ((unsigned)X >= W || (unsigned)Y >= H) continue;
        float rx = X + 0.5f - cx, ry = Y + 0.5f - by;
        float sx = rx * ca + ry * sa, sy = -rx * sa + ry * ca;
        int i = (int)floorf(sx + S.w * 0.5f), j = (int)floorf(sy + S.h);
        if (!sc_solid(i, j)) continue;
        if (S.d[j][i]) direct(X, Y, S.d[j][i]); else matf(X, Y, S.r[j][i], S.v[j][i], MF_FLAT);
    }
}

static const char *const KEY_SKULL[11] = {
    "...#####...", ".#########.", "###########", "##..###..##", "#....#....#", "##..###..##", "#####.#####", ".####.####.",
    "..#######..", "..#.#.#.#..", "...#####..." };
/* ---- the graves, built ---- */
/* I: a gothic stele. A two-step plinth, pinnacles at its shoulders, a pointed head, a moulded border; the class's mark in
 * relief in a sunk quatrefoil. Its head's corner is broken off, cracks run from the break. 104 by 100. */
static void build_stele(const char *const *mark, const char *num, const char *const *lines, int nl, uint32_t seed) {
    sc_begin(104, 100);
    sc_rect(0, 90, 104, 10, 1, 0, SM_STONE, 0);                          /* the plinth */
    sc_rect(5, 81, 94, 9, 2, 0, SM_STONE, 0.03f);
    sc_rect(8, 38, 88, 43, 3, 0, SM_STONE, 0.04f);                       /* the shaft on square shoulders, */
    sc_arch(20, 84, 39, 31, 2, 3, 0, SM_STONE, 0.04f);                   /* its head a tall pointed gable */
    sc_groove(1u << 3, 4);                                               /* the moulding */
    for (int k = 1; k < 6; k++) for (int e = 0; e < 2; e++) {           /* crockets: knobs of leaf up the gable's edges */
        float u = k / 6.0f, x = e ? 84 - u * 30 : 20 + u * 30;
        int top = 0;
        while (top < 40 && !sc_solid((int)x, top)) top++;
        sc_disc(x + (e ? 1.5f : -1.5f), top + 0.5f, 1.8f, 6, 1, SM_STONE, 0.08f);
    }
    sc_rect(50, 0, 4, 9, 6, 1, SM_STONE, 0.08f);                         /* a cross for its finial */
    sc_rect(47, 2, 10, 3, 6, 1, SM_STONE, 0.08f);
    for (int k = 0; k < 2; k++) {                                        /* the pinnacles, standing proud */
        int x = k ? 92 : 1;
        sc_rect(x, 44, 11, 37, 4 + k, 1, SM_STONE, 0.02f);
        float p[6] = { (float)x - 1, 44, (float)x + 12, 44, x + 5.5f, 27 };
        sc_poly(p, 3, 4 + k, 1, SM_STONE, 0.02f);
        sc_disc(x + 5.5f, 25, 2.6f, 4 + k, 1, SM_STONE, 0.05f);
        sc_rect(x, 44, 11, 2, 4 + k, 1, SM_STONE, 0.1f);                 /* a string course */
        sc_rect(x + 4, 50, 3, 26, 4 + k, -1, SM_STONE, 0);               /* a blind lancet cut in each */
    }
    for (int k = 0; k < 5; k++) {                                        /* the quatrefoil, sunk */
        static const float QX[5] = { 0, -6.5f, 6.5f, 0, 0 }, QY[5] = { 0, 0, 0, -6.5f, 6.5f };
        float cx = 52 + QX[k], cy = 25 + QY[k];
        for (int j = (int)cy - 7; j <= cy + 7; j++) for (int i = (int)cx - 7; i <= cx + 7; i++)
            if (sc_solid(i, j) && S.id[j][i] == 3 && (i + 0.5f - cx) * (i + 0.5f - cx) + (j + 0.5f - cy) * (j + 0.5f - cy) <= 6.4f * 6.4f) S.ht[j][i] = -1;
    }
    if (mark) sc_mark(52 - 11, 25 - 11, mark, 11, 2, 0, 0.14f);          /* the mark, standing out of it */
    sc_text_c(52, 82, num, 0, 1, seed, 80);
    for (int k = 0; k < nl; k++) sc_text_c(52, 41 + k * 9, lines[k], 0, 1, seed + (uint32_t)k, 80);
    sc_break(98, 28, 8, seed + 3);                                       /* a shoulder and its pinnacle's top, gone */
    sc_crack(89, 37, 1.9f, 40, seed + 4, 2);
    sc_crack(70, 18, 1.9f, 16, seed + 5, 1);
    sc_crack(18, 90, -1.2f, 14, seed + 6, 1);
    sc_chips(0.35f, seed + 7);
}
/* II: a wheel-cross on a slab. The cross stands proud on the slab's gable, its ring broken away on the left; a crack across
 * the slab; ivy up its right side. 104 by 100. */
static void build_wheelcross(const char *const *mark, const char *num, const char *const *lines, int nl, uint32_t seed) {
    sc_begin(104, 100);
    sc_rect(2, 90, 100, 10, 1, 0, SM_STONE, 0);
    sc_rect(8, 82, 88, 8, 2, 0, SM_STONE, 0.03f);
    sc_rect(10, 40, 84, 42, 3, 0, SM_STONE, 0.02f);                      /* the slab, a low gable on it */
    float gab[6] = { 10, 40.5f, 94, 40.5f, 52, 30 };
    sc_poly(gab, 3, 3, 0, SM_STONE, 0.02f);
    sc_groove(1u << 3, 4);
    sc_rect(45, 0, 14, 44, 5, 1, SM_STONE, 0.06f);                       /* the cross */
    sc_rect(27, 11, 50, 11, 5, 1, SM_STONE, 0.06f);
    sc_ring(52, 16.5f, 12.5f, 17.5f, 5, 1, SM_STONE, 0.06f);
    sc_disc(52, 16.5f, 6, 5, 2, SM_STONE, 0.1f);                         /* its boss */
    for (int j = 2; j < 40; j++) if (j < 10 || j > 23) { sc_px(51, j, 5, 0, SM_STONE, 0.02f); sc_px(52, j, 5, 0, SM_STONE, 0.02f); }   /* a groove down each arm */
    for (int i = 29; i < 75; i++) if (i < 45 || i > 59) { sc_px(i, 16, 5, 0, SM_STONE, 0.02f); }
    if (mark) sc_mark(52 - 5, 16 - 5, mark, 11, 1, 1, 0.12f);
    sc_text_c(52, 82, num, 0, 1, seed, 80);
    for (int k = 0; k < nl; k++) sc_text_c(52, 45 + k * 9, lines[k], 0, 1, seed + (uint32_t)k, 74);
    sc_break(31, 12, 7, seed + 3);                                       /* the ring broken, the arm's end with it */
    sc_crack(93, 44, 2.3f, 50, seed + 4, 2);
    sc_crack(12, 60, 0.3f, 16, seed + 5, 1);
    sc_chips(0.3f, seed + 7);
    sc_ivy(92, 99, 80, seed + 8);
    sc_ivy(95, 97, 40, seed + 9);
}
/* the empty plot's marker: two rough timbers lashed with rope, the upright's top split; 60 by 92 */
static void build_timber_cross(uint32_t seed) {
    sc_begin(60, 92);
    sc_rect(25, 0, 10, 92, 1, 0, SM_WOODV, 0.02f);
    sc_rect(4, 19, 52, 9, 2, 1, SM_WOOD, 0.04f);
    for (int j = 34; j < 92; j++) if (h01(j / 5, 0, seed) < 0.5f) { sc_px(25, j, 1, 0, SM_WOODV, -0.16f); sc_px(26, j, 1, 0, SM_WOODV, -0.1f); }   /* bark left on it */
    for (int i = 6; i < 54; i++) if (h01(i / 6, 1, seed) < 0.4f) sc_px(i, 27, 2, 1, SM_WOOD, -0.14f);
    for (int k = 0; k < 4; k++) {                                        /* the lashing, crossed over the joint */
        sc_line(23, 16.0f + k * 1.5f, 37, 27.0f + k * 1.5f, 1.4f, 3, 2, SM_ROPE, 0);
        sc_line(37, 16.0f + k * 1.5f, 23, 27.0f + k * 1.5f, 1.4f, 3, 2, SM_ROPE, 0);
    }
    sc_line(30, 30, 27, 38, 1, 3, 2, SM_ROPE, 0);                        /* a loose end */
    sc_break(30, 0, 4.5f, seed);                                         /* the top split and broken */
    sc_crack(30, 3, 1.57f, 16, seed + 1, 0);
    sc_crack(5, 23, 0.0f, 9, seed + 2, 0);
    sc_crack(55, 22, PI, 10, seed + 3, 0);
    sc_chips(0.25f, seed + 4);
}
/* the board hung from it: two planks, nails rusting down them, the words scratched in; 84 by 40 */
static void build_plank(uint32_t seed, const char *num) {
    sc_begin(86, 42);
    sc_rect(1, 1, 84, 19, 1, 0, SM_WOOD, 0.05f);
    sc_rect(0, 21, 85, 20, 2, 0, SM_WOOD, 0.02f);
    for (int k = 0; k < 4; k++) {
        int x = k % 2 ? 79 : 4, y = k < 2 ? 5 : 26;
        sc_disc(x + 0.5f, y + 0.5f, 1.6f, 9, 1, SM_IRON, 0.1f);
        for (int j = 2; j < 6 + (int)(h01(k, 1, seed) * 6); j++) if (sc_solid(x, y + j)) { S.m[y + j][x] = SM_IRON; S.dv[y + j][x] = -0.1f; }   /* the rust run */
    }
    sc_break(86, 34, 6, seed + 1);                                        /* the lower plank's end split off */
    sc_crack(70, 10, PI, 22, seed + 2, 0);
    sc_text_c(43, 3, num, 0, 1, seed + 3, 80);
    sc_text_c(43, 13, "UNMARKED", 0, 1, seed + 4, 80);
    sc_text_c(43, 23, "NOTHING", 0, 1, seed + 5, 80);
    sc_text_c(43, 32, "WRITTEN", 0, 1, seed + 6, 80);
    sc_chips(0.3f, seed + 7);
}
/* a piece broken off, lying where it fell */
static void build_fragment(int w, int h, uint32_t seed) {
    sc_begin(w, h);
    float p[12];
    for (int k = 0; k < 6; k++) { float a = k / 6.0f * 2 * PI, r = 0.75f + 0.25f * h01(k, 0, seed); p[2 * k] = w * 0.5f + cosf(a) * r * w * 0.5f; p[2 * k + 1] = h * 0.55f + sinf(a) * r * h * 0.45f; }
    sc_poly(p, 6, 1, 0, SM_STONE, 0);
    for (int i = 0; i < w; i++) for (int j = 0; j < h; j++) if (sc_solid(i, j)) { S.f[j][i] |= SF_FRESH; if (sc_solid(i, j + 1)) S.f[j + 1][i] |= SF_FRESH; break; }
    sc_chips(0.3f, seed + 1);
}
/* a plot's kerb, seen from the front: a low stone strip between two posts, one finial gone; w by 18 */
static void build_kerb(int w, uint32_t seed) {
    sc_begin(w, 18);
    sc_rect(0, 9, w, 9, 1, 0, SM_STONE, 0);
    sc_rect(0, 4, 9, 14, 2, 1, SM_STONE, 0.04f); sc_rect(w - 9, 4, 9, 14, 3, 1, SM_STONE, 0.04f);
    sc_disc(4.5f, 3.2f, 3.2f, 2, 1, SM_STONE, 0.06f); sc_disc(w - 4.5f, 3.2f, 3.2f, 3, 1, SM_STONE, 0.06f);
    sc_break(w - 4.5f, 2, 4, seed);
    sc_crack(w * 0.4f, 9, 1.2f, 9, seed + 1, 0);
    sc_chips(0.35f, seed + 2);
}

/* ---- the weathered pieces of the screens (step 3) ---- */
/* a lych-gate's beam across the top, the title cut in it and gilt, iron bands, its tiles over it; for out of doors */
static void ui_beam(const char *title, float t) {
    sc_begin(W, 26);
    for (int i = 0; i < W; i++) for (int j = 0; j < 6; j++) {          /* the roof's edge: a row of slates */
        int sl = (i + (j > 2 ? 6 : 0)) / 12;
        sc_px(i, j, 10 + sl % 2, (j % 3 == 0) ? 0 : 1, SM_STONE, -0.18f + 0.06f * h01(sl, j / 3, 3));
    }
    sc_rect(0, 6, W, 18, 1, 0, SM_WOOD, 0.02f);
    sc_rect(0, 23, W, 3, 2, 1, SM_WOOD, 0.08f);
    for (int k = 0; k < 2; k++) { int x = k ? 248 : 52; sc_rect(x, 6, 7, 18, 3, 1, SM_IRON, 0.1f); for (int j = 9; j < 22; j += 6) sc_disc(x + 3.5f, j, 1.2f, 3, 2, SM_IRON, 0.2f); }
    sc_text(W / 2 - sc_text_w(title, 8) / 2, 7, title, 1, 8, 1, 41);
    sc_crack(20, 14, 0.05f, 30, 5, 1); sc_crack(300, 12, PI, 24, 6, 1);
    sc_break(W + 2, 12, 6, 7);
    sc_chips(0.2f, 8);
    sc_shade(0.42f, 44, 0.85f, R_GOLD);
    sc_draw(W / 2, 26, 0);
    (void)t;
}
/* a slab for a row: chipped, its words cut and gilt while chosen */
static void row_slab(int x, int y, int w, const char *s, float k, uint32_t seed) {
    sc_begin(w, 17);
    float p[12] = { 2, 0, (float)w - 3, 0, (float)w, 3, (float)w - 1, 17, 1, 17, 0, 3 };
    sc_poly(p, 6, 1, 0, SM_STONE, 0.04f);
    sc_groove(1u << 1, 2);
    sc_text_c(w / 2, 5, s, 0, 1, seed, w - 8);
    sc_crack((float)(w - 18), 0, 1.9f, 12, seed + 1, 0);
    sc_chips(0.3f, seed + 2);
    sc_shade(0.36f + 0.1f * k, seed + 3, k, R_GOLD);
    sc_draw(x + w / 2, y + 17, 0);
}
/* the hints, each key on a worn stone cap, its letter cut and filled pale; the words after it inlaid */
static void ui_keys_worn(int y, const char *const keys[], const char *const says[], int n) {
    int tw = 0;
    for (int k = 0; k < n; k++) tw += sc_text_w(keys[k], 8) + 8 + (int)strlen(says[k]) * 8 + (k < n - 1 ? 14 : 0);
    int x = (W - tw) / 2;
    for (int k = 0; k < n; k++) {
        int cw = sc_text_w(keys[k], 8) + 8;
        sc_begin(cw, 13);
        float p[12] = { 1, 0, (float)cw - 2, 0, (float)cw, 2, (float)cw, 13, 0, 13, 0, 2 };
        sc_poly(p, 6, 1, 1, SM_STONE, 0.1f);
        sc_text(4, 3, keys[k], 0, 8, 1, 60 + (uint32_t)k);
        sc_chips(0.25f, 70 + (uint32_t)k);
        int capr = !PADMODE ? R_BONE : !strcmp(keys[k], "A") ? R_MOSS : !strcmp(keys[k], "B") ? R_BLOOD : !strcmp(keys[k], "X") ? R_GLOW : !strcmp(keys[k], "Y") ? R_GOLD : R_BONE;
        sc_shade(0.42f, 80 + (uint32_t)k, 1.0f, capr);
        sc_draw(x + cw / 2, y + 11, 0);
        x += cw + 4;
        for (int j = 0; j < 13; j++) for (int i = 0; i < (int)strlen(says[k]) * 8 + 2; i++) LC[y - 2 + j][x - 1 + i] = fmaxf(LC[y - 2 + j][x - 1 + i], 0.5f);
        x += (int)strlen(says[k]) * 8 + 14;
    }
}
static void ui_keys_words(int y, const char *const keys[], const char *const says[], int n) {   /* the words, after the light is resolved */
    int tw = 0;
    for (int k = 0; k < n; k++) tw += sc_text_w(keys[k], 8) + 8 + (int)strlen(says[k]) * 8 + (k < n - 1 ? 14 : 0);
    int x = (W - tw) / 2;
    for (int k = 0; k < n; k++) {
        x += sc_text_w(keys[k], 8) + 8 + 4;
        ui_text8(&F, x, y + 1, says[k], FACE_INSC, R_BONE + 10, TX_INLAID);
        x += (int)strlen(says[k]) * 8 + 14;
    }
}
static void ui_light(int y0, int y1, float v) { for (int y = y0; y < y1 && y < H; y++) for (int x = 0; x < W; x++) LC[y][x] = fmaxf(LC[y][x], v); }
static const char *KEYS3[3] = { "A/D", "ENTER", "ESC" }, *SAYS3[3] = { "MOVES", "RISES", "BACK" };
static const char *PADK3[3] = { "D-PAD", "A", "B" };
#define GKEYS (PADMODE ? PADK3 : KEYS3)
/* a vignette: the light falling off towards the corners, to hold the eye */
static void vignette(float k) {
    for (int y = 0; y < H; y++) for (int x = 0; x < W; x++) {
        float dx = (x - W * 0.5f) / (W * 0.5f), dy = (y - H * 0.5f) / (H * 0.5f), r = sqrtf(dx * dx * 0.8f + dy * dy * 1.1f);
        float f = 1 - k * smooth(0.55f, 1.25f, r);
        LC[y][x] *= f; LW[y][x] *= f;
    }
}

static struct { int on; float appear, press, k[4]; } MN;
static const char *MKEYS[3] = { "W/S", "ENTER", "ESC" }, *MPADK[3] = { "D-PAD", "A", "B" }, *MSAYS[3] = { "MOVES", "SELECTS", "BACK" };
#define MENUKEYS (PADMODE ? MPADK : MKEYS)
static void title_foot_pre(void) { if (MN.on && MN.appear > 0.5f) { ui_keys_worn(185, MENUKEYS, MSAYS, 3); ui_light(182, H, 0.62f); } }
static void title_foot(int y, int band, float t) {
    if (MN.on && MN.appear > 0.5f) { ui_keys_words(185, MENUKEYS, MSAYS, 3); return; }
    if (MN.on && MN.appear > 0.02f) return;
    if (MN.on && MN.press > 0) {                                          /* the key taken: the words flare and are gone */
        int w = (int)strlen(any_prompt()) * 8;
        if (band) ui_band(&F, W / 2 - w / 2 - 8, y - 4, w + 16, 16);
        ui_text8(&F, W / 2 - w / 2, y, any_prompt(), FACE_INSC, R_BONE + 15, TX_INLAID);
        return;
    }
    prompt(W / 2, y, any_prompt(), band, t, R_BONE);
}
/* ---- the menu after the key (docs/PLAN-title.md, 4b): four choices on a stone in the middle, the scene going on ---- */
static const char *const MENU_ROWS[4] = { "CHOOSE A GRAVE", "BEGIN A NEW RUN", "THE CONTROLS", "HELP" };
static const char *menu_note(int i) {
    static const char *const N[4] = { "ONE OF HER PAGES", "FROM THE BEGINNING", "WHAT EACH KEY DOES", "NOT WRITTEN YET" };
    return i == 2 && PADMODE ? "WHAT EACH BUTTON DOES" : N[i];
}
static int menu_sel(void) { int b = 0; for (int k = 1; k < 4; k++) if (MN.k[k] > MN.k[b]) b = k; return b; }
/* the four, cut and gilding one by one, with a gilt notch either side of the chosen; then a rule and the note */
static void menu_rows(int xc, int y0, int dy, int maxw, int note_y, uint32_t seed) {
    for (int i = 0; i < 4; i++) {
        sc_gk = 1 + MN.k[i];
        sc_text_c(xc, y0 + i * dy, MENU_ROWS[i], 0, 1, seed + (uint32_t)i, maxw);
        int hw = sc_text_w(MENU_ROWS[i], 8) / 2 + 7;
        for (int e = 0; e < 2; e++) {                                   /* the notches, small lozenges */
            int nx = xc + (e ? hw : -hw - 1), ny = y0 + i * dy + 3;
            static const int DX[5] = { 0, -1, 1, 0, 0 }, DY[5] = { 0, 0, 0, -1, 1 };
            for (int q = 0; q < 5; q++) if (sc_solid(nx + DX[q], ny + DY[q])) { S.ht[ny + DY[q]][nx + DX[q]] = -1; S.f[ny + DY[q]][nx + DX[q]] |= SF_GILD; S.gk[ny + DY[q]][nx + DX[q]] = 1 + MN.k[i]; }
        }
    }
    sc_gk = 0;
    if (note_y >= 0) {
        for (int x = xc - 60; x <= xc + 60; x++) if (sc_solid(x, note_y - 4) && abs(x - xc) > 3) S.ht[note_y - 4][x] = -1;   /* a rule, broken at its middle */
        sc_text_c(xc, note_y, menu_note(menu_sel()), 0, 0, seed + 9, maxw);
    }
}
/* A's: a stone with a steep gable, a skull cut in it, rising out of the earth into the middle of the graveyard; 172 by 96 */
static void build_menu_stone(void) {
    sc_begin(172, 96);
    sc_rect(0, 20, 172, 76, 1, 0, SM_STONE, 0.04f);
    sc_arch(36, 136, 21, 20, 2, 1, 0, SM_STONE, 0.04f);
    sc_groove(1u << 1, 4);
    for (int k = 1; k < 6; k++) for (int e = 0; e < 2; e++) {
        float u = k / 6.0f, x = e ? 136 - u * 50 : 36 + u * 50;
        int top = 0;
        while (top < 30 && !sc_solid((int)x, top)) top++;
        sc_disc(x + (e ? 1.5f : -1.5f), top + 0.5f, 1.8f, 6, 1, SM_STONE, 0.08f);
    }
    sc_mark(86 - 5, 8, KEY_SKULL, 11, 1, -1, 0);
    menu_rows(86, 28, 13, 156, 82, 900);
    sc_break(170, 24, 7, 901);
    sc_crack(150, 20, 2.0f, 40, 902, 2);
    sc_crack(4, 60, 0.3f, 18, 903, 1);
    sc_chips(0.3f, 904);
    sc_ivy(166, 95, 40, 905); sc_ivy(5, 95, 14, 906);
}
/* B's: a tablet on two rings, lowered on chains from the lintel; 180 by 88 */
static void build_menu_tablet(void) {
    sc_begin(180, 88);
    float p[16] = { 5, 6, 175, 6, 180, 11, 180, 83, 175, 88, 5, 88, 0, 83, 0, 11 };
    sc_poly(p, 8, 1, 0, SM_STONE, 0.06f);
    sc_groove(1u << 1, 3); sc_groove(1u << 1, 6);
    for (int e = 0; e < 2; e++) { float rx = e ? 156 : 24; sc_ring(rx, 5, 2, 4.2f, 2, 1, SM_IRON, 0.1f); sc_rect((int)rx - 3, 8, 7, 4, 2, 1, SM_IRON, 0.06f); }
    sc_mark(90 - 5, 11, KEY_SKULL, 11, 1, -1, 0);
    menu_rows(90, 21, 13, 160, 74, 910);
    sc_break(2, 86, 7, 911);
    sc_crack(176, 30, 2.6f, 30, 912, 2);
    sc_crack(60, 88, -1.3f, 14, 913, 1);
    sc_chips(0.3f, 914);
}

/* ======================================================================================================================
 * A, BENEATH: the graveyard under the moon; under it the earth, its dead and its roots; under that the crypt, and in it
 * a walker with a lantern going by
 * ====================================================================================================================== */
#define A_GROUND 96
#define A_CRYPT0 140
#define A_FLOOR 184
static float a_lantern_x, a_lantern_y, a_flick;
static uint8_t WMASK[H][W];                                             /* the walker, for the shadow it throws */
static void wm(int x, int y, int ramp, float v) { matf(x, y, ramp, v, 0); if ((unsigned)x < W && (unsigned)y < H) WMASK[y][x] = 1; }
static void wline(float x0, float y0, float x1, float y1, float w, int ramp, float v) {
    float dx = x1 - x0, dy = y1 - y0, l = sqrtf(dx * dx + dy * dy);
    int n = (int)(l * 2) + 1;
    for (int k = 0; k <= n; k++) {
        float u = (float)k / n, cx = x0 + dx * u, cy = y0 + dy * u, r = w * 0.5f;
        for (int y = (int)floorf(cy - r); y <= (int)ceilf(cy + r); y++) for (int x = (int)floorf(cx - r); x <= (int)ceilf(cx + r); x++)
            if ((x + 0.5f - cx) * (x + 0.5f - cx) + (y + 0.5f - cy) * (y + 0.5f - cy) <= r * r) wm(x, y, ramp, v);
    }
}
/* the walker, side on, going right: a hooded cloak, ragged at the hem, a rope at the waist; a staff in the back hand, a
 * lantern held out in the front on a short chain */
static void walker(float x, int foot, float t, int lantern_out) {
    memset(WMASK, 0, sizeof WMASK);
    float step = t / 0.5f * PI;                                       /* a step each half second */
    float bob = fabsf(sinf(step)) * 1.2f;
    int top = foot - 30 + (int)roundf(-bob);
    int cx = (int)roundf(x);
    /* the staff, planted ahead of the back foot and swung forward again */
    float sp = frac(t / 1.0f), tipx = cx - 3 + (sp < 0.5f ? 8 - sp * 16 : -8 + (sp - 0.5f) * 32 * 0.5f);
    wline((float)cx - 3, (float)top + 9, tipx, (float)foot, 1.4f, R_WOOD, 0.14f);
    wm(cx - 3, top + 8, R_WOOD, 0.2f);
    /* the cloak: shoulders to a hem that swings, folds down it, ragged at its edge */
    float sw = sinf(step) * 2.0f;
    for (int y = top + 8; y < foot - 1; y++) {
        float u = (float)(y - top - 8) / (foot - 1 - top - 8);
        float back = 5 + 4 * u + (u > 0.6f ? -sw * (u - 0.6f) * 2 : 0), front = 4 + 3 * u + (u > 0.6f ? sw * (u - 0.6f) * 2 : 0);
        for (int i = -(int)back; i <= (int)front; i++) {
            if (y > foot - 4 && h01(i + cx, y, 17) < 0.35f) continue;   /* the hem in rags */
            float fold = fmodf(i * 1.0f + y * 0.15f + 40, 3.5f) < 1 ? -0.03f : 0.02f;
            float v = 0.07f + fold + (i >= (int)front - 1 ? 0.3f : i > 0 ? 0.06f : 0);
            if (y == top + 15 || y == top + 16) v = 0.24f + 0.1f * (i > 0);   /* the rope at the waist */
            wm(cx + i, y, y == top + 15 || y == top + 16 ? R_WOOD : R_IRON, v);
        }
    }
    wline((float)cx + 2, top + 16.0f, cx + 1.0f + sw * 0.3f, top + 22.0f, 1, R_WOOD, 0.3f);   /* its knot's end */
    int fa = (int)roundf(sinf(step) * 3), fb = -fa;                    /* the feet, in turn */
    for (int k = 0; k < 4; k++) { wm(cx + fa + k, foot - 2, R_IRON, 0.18f); wm(cx + fa + k, foot - 1, R_IRON, 0.14f); wm(cx + fb - 1 + k, foot - 2, R_IRON, 0.12f); wm(cx + fb - 1 + k, foot - 1, R_IRON, 0.1f); }
    /* the hood: round, a point behind, its rim lit at the front, the face in its shadow */
    for (int y = top; y < top + 11; y++) for (int i = -5; i <= 5; i++) {
        float dx = i - 0.5f, dy = y - (top + 5.5f);
        int in = dx * dx + dy * dy <= 26 || (i < -2 && y < top + 4 && y > top - 1 && i > -7 + (y - top));
        if (!in) continue;
        int face = i >= 1 && y >= top + 3 && y <= top + 8 && dx * dx + dy * dy < 14;
        int rim = !face && i >= 2 && dx * dx + dy * dy > 16;
        wm(cx + i, y, face ? R_SKIN : R_IRON, face ? (i >= 3 && y > top + 5 ? 0.24f : 0.04f) : rim ? 0.4f : 0.09f);
    }
    /* the arm out, the lantern hanging from the hand, swinging after the step */
    int hx = cx + 9, hy = top + 13;
    wline((float)cx + 2, (float)top + 10, (float)hx, (float)hy, 2.6f, R_IRON, 0.2f);
    wm(hx, hy, R_SKIN, 0.4f);
    float swing = sinf(step - 0.8f) * 0.18f;
    float lx = hx + sinf(swing) * 6, ly = hy + cosf(swing) * 6;
    mline((float)hx, (float)hy, lx, ly - 2, 1, R_IRON, 0.3f, 0);
    mrect((int)lx - 2, (int)ly - 3, 5, 1, R_IRON, 0.35f, 0); matf((int)lx, (int)ly - 4, R_IRON, 0.35f, 0);
    mrect((int)lx - 2, (int)ly + 5, 5, 1, R_IRON, 0.3f, 0);
    for (int j = -2; j <= 4; j++) { matf((int)lx - 2, (int)ly + j, R_IRON, 0.3f, 0); matf((int)lx + 2, (int)ly + j, R_IRON, 0.3f, 0); }
    for (int j = -1; j <= 4; j++) for (int i = -1; i <= 1; i++) direct((int)lx + i, (int)ly + j, (uint8_t)(R_FLAME + clampi(9 + (j > 1) * 3 - abs(i) * 2 + (int)(2 * sinf(t * 23 + j)), 0, 15)));
    if (lantern_out) { a_lantern_x = lx; a_lantern_y = ly + 2; }
}
/* the small stones over the graves, built: each leaning or broken its own way */
static void small_built(int kind, int cx, int base, float deg, uint32_t seed) {
    switch (kind) {
    case 0:                                                             /* a gothic stone, its gable pointed */
        sc_begin(14, 20); sc_rect(0, 17, 14, 3, 1, 0, SM_STONE, 0); sc_rect(1, 8, 12, 9, 2, 0, SM_STONE, 0.04f);
        sc_arch(1, 13, 9, 8, 2, 2, 0, SM_STONE, 0.04f); sc_groove(1u << 2, 2); sc_chips(0.3f, seed); break;
    case 1:                                                             /* a wheel-cross, an arm broken */
        sc_begin(15, 24); sc_rect(2, 20, 11, 4, 1, 0, SM_STONE, 0); sc_rect(6, 2, 4, 19, 2, 0, SM_STONE, 0.05f);
        sc_rect(1, 6, 13, 4, 2, 0, SM_STONE, 0.05f); sc_ring(8, 8, 3, 5, 2, 1, SM_STONE, 0.06f); sc_break(14, 8, 2.5f, seed); break;
    case 2:                                                             /* an angel on a plinth, her head bowed, a wing broken */
        sc_begin(20, 32); sc_rect(4, 24, 12, 8, 1, 0, SM_STONE, 0); sc_rect(3, 23, 14, 2, 1, 1, SM_STONE, 0.08f);
        { float robe[8] = { 8, 9, 13, 9, 15, 23, 5, 23 }; sc_poly(robe, 4, 2, 0, SM_STONE, 0.06f); }
        sc_disc(10, 6.5f, 2.4f, 2, 1, SM_STONE, 0.08f);
        { float lw[8] = { 8, 10, 1, 2, 2, 13, 7, 17 }; sc_poly(lw, 4, 3, -1, SM_STONE, 0.02f); }
        { float rw[8] = { 13, 10, 19, 2, 18, 13, 14, 17 }; sc_poly(rw, 4, 3, -1, SM_STONE, 0.02f); }
        for (int j = 12; j < 23; j += 3) sc_px(11 + (j % 2), j, 2, -1, SM_STONE, 0);   /* the robe's folds */
        sc_break(18, 3, 3, seed); sc_chips(0.25f, seed + 1); break;
    case 3:                                                             /* a round-headed stone, chipped */
        sc_begin(12, 16); sc_rect(0, 6, 12, 10, 1, 0, SM_STONE, 0.04f); sc_arch(0, 12, 6, 6, 0, 1, 0, SM_STONE, 0.04f);
        sc_groove(1u << 1, 2); sc_chips(0.4f, seed); break;
    case 4:                                                             /* an obelisk, snapped */
        sc_begin(10, 30); sc_rect(0, 26, 10, 4, 1, 0, SM_STONE, 0); sc_rect(1, 22, 8, 4, 2, 0, SM_STONE, 0.04f);
        { float ob[8] = { 2, 22, 8, 22, 7, 6, 3, 6 }; sc_poly(ob, 4, 3, 0, SM_STONE, 0.06f); }
        sc_break(5, 7, 3.5f, seed); sc_chips(0.25f, seed + 1); break;
    case 5:                                                             /* the obelisk's top, fallen */
        sc_begin(6, 12); { float tp[6] = { 0.5f, 12, 5.5f, 12, 3, 0 }; sc_poly(tp, 3, 1, 0, SM_STONE, 0.06f); }
        for (int i = 0; i < 6; i++) if (sc_solid(i, 11)) S.f[11][i] |= SF_FRESH; break;
    case 6:                                                             /* a slab gone over */
        sc_begin(12, 18); sc_rect(0, 0, 12, 18, 1, 0, SM_STONE, 0.04f); sc_groove(1u << 1, 2); sc_chips(0.4f, seed); break;
    default:                                                            /* the stump of one broken across */
        sc_begin(12, 11); sc_rect(0, 0, 12, 11, 1, 0, SM_STONE, 0.02f); sc_break(6, -2, 6, seed); sc_chips(0.4f, seed + 1); break;
    }
    sc_shade(0.42f, seed + 9, 0, R_GOLD);
    sc_draw(cx, base, deg);
}
static void a_world(float t, float flash, int graves) {
    float mx = graves ? 34 : 196, my = graves ? 22 : 70, mr = graves ? 11 : 14;
    pal_night();
    pal_flash(flash * 0.55f);
    light_clear(1.0f + 0.4f * flash, 0);
    sky(0, A_GROUND, t, mx, my, mr, flash);
    /* the far hills and the church, flat against the sky; one window lit */
    for (int x = 0; x < W; x++) {
        float hy = 80 + 6 * sinf(x * 0.021f + 1.3f) + 3 * sinf(x * 0.063f) + 2 * fbm(x * 0.05f, 0, 8);
        if (x > 44 && x < 70) hy = fminf(hy, 72);
        if (x > 52 && x < 62) hy = fminf(hy, 64);
        if (x >= 55 && x <= 58) hy = fminf(hy, 50 + (float)abs(x - 56) * 4);
        if (x == 56) hy = fminf(hy, 46);                                /* the spire's cross */
        for (int y = (int)hy; y < A_GROUND; y++) matf(x, y, R_CEIL, 0.1f + 0.02f * (y > 88), MF_NOWARM);
    }
    mrect(54, 48, 5, 1, R_CEIL, 0.1f, MF_NOWARM);
    for (int y = 77; y < 81; y++) for (int x = 47; x < 49; x++) if (!(y == 77 && x == 48)) direct(x, y, (uint8_t)(R_FLAME + clampi((int)(2 + 2 * flick(t, 21) - (y == 80)), 0, 15)));
    /* the tree, its crows (one shakes its wings now and then) */
    branch(28, A_GROUND, 1.5f, 30, 8, 6, 7);
    branch(28, A_GROUND - 18, 2.5f, 18, 4, 4, 9);
    branch(30, A_GROUND - 24, 0.7f, 16, 3.5f, 4, 13);
    mrect(40, 52, 3, 2, R_IRON, 0.04f, MF_FLAT); matf(43, 51, R_IRON, 0.04f, MF_FLAT); matf(39, 51, R_IRON, 0.04f, MF_FLAT);
    if (frac(t / 3.0f) < 0.12f) { mline(40, 52, 37, 48, 1, R_IRON, 0.04f, MF_FLAT); mline(42, 52, 45, 48, 1, R_IRON, 0.04f, MF_FLAT); }
    mrect(17, 47, 3, 2, R_IRON, 0.04f, MF_FLAT); matf(16, 46, R_IRON, 0.04f, MF_FLAT); matf(19, 49, R_IRON, 0.04f, MF_FLAT);
    /* the stones, built: leaning, snapped, gone over; an angel among them */
    small_built(0, 84, A_GROUND, 3.0f, 31);
    small_built(1, 106, A_GROUND, -7.0f, 32);
    small_built(2, 130, A_GROUND, 0, 33);
    small_built(3, 152, A_GROUND, 9.0f, 34);
    small_built(4, 172, A_GROUND, 0, 35);
    small_built(5, 182, A_GROUND, 78.0f, 36);
    small_built(6, 204, A_GROUND, -80.0f, 37);
    small_built(7, 222, A_GROUND, 2.0f, 38);
    small_built(0, 240, A_GROUND, -3.0f, 39);
    for (int x = 250; x < 318; x += 4) { if (x == 270 || x == 274) continue; mrect(x, A_GROUND - 13, 1, 13, R_IRON, 0.12f, 0); matf(x, A_GROUND - 14, R_IRON, 0.2f, 0); }
    mline(270, A_GROUND - 1, 276, A_GROUND - 8, 1, R_IRON, 0.12f, 0);   /* a railing bent over */
    mrect(250, A_GROUND - 11, 68, 1, R_IRON, 0.12f, 0); mrect(250, A_GROUND - 4, 68, 1, R_IRON, 0.12f, 0);
    {
        int mx0 = 262, mw = 42, base = A_GROUND;
        for (int y = base - 34; y < base; y++) for (int x = mx0; x < mx0 + mw; x++) {
            int ped = y < base - 24, roof = y < base - 30 + abs(x - (mx0 + mw / 2)) * 6 / (mw / 2);
            if (ped && roof) continue;
            if (ped && x > mx0 + mw - 9 && y < base - 27 + (x - mx0 - mw + 9) / 2) continue;   /* its pediment's corner broken */
            float v = 0.3f + 0.12f * fbm(x * 0.4f, y * 0.4f, 21) + 0.18f * (x > mx0 + mw - 4);
            int col = !ped && ((x - mx0) % 9 < 3) && y > base - 23 && y < base - 2;
            int door = !ped && x > mx0 + 16 && x < mx0 + 26 && y > base - 18;
            if (door) v = 0.03f + (y < base - 16 ? 0.05f : 0);
            else if (!ped && !col && y > base - 23 && y < base - 2) v -= 0.12f;
            if (y == base - 24 || y == base - 23) v += 0.15f;
            if (fabsf(fbm(x * 0.1f, y * 0.1f, 22) - 0.5f) < 0.02f) v -= 0.15f;
            matf(x, y, R_STONE, v, 0);
        }
    }
    /* the earth in its bands: topsoil and roots, clay, gravel, the rock over the vault */
    for (int y = A_GROUND; y < H; y++) for (int x = 0; x < W; x++) {
        float wv = 3 * (fbm(x * 0.03f, 0, 59) - 0.5f) * 2;
        int ramp = R_FLOOR;
        float n = fbm(x * 0.07f, y * 0.11f, 51), v;
        if (y < 106 + wv) v = 0.2f + 0.14f * n - 0.08f * (y < A_GROUND + 3);
        else if (y < 117 + wv) { ramp = h01(x / 3, y, 64) < 0.5f ? R_BRICK : R_FLOOR; v = 0.15f + 0.08f * n + 0.03f * sinf(y * 1.3f + n * 4); }
        else if (y < 128 + wv) { v = 0.22f + 0.1f * n; if (h01(x / 2, y / 2, 60) < 0.3f) { ramp = R_STONE; v = 0.22f + 0.14f * ((y & 1) == 0) + 0.06f * h01(x, y, 61); } }
        else { ramp = R_STONE; v = 0.16f + 0.08f * n; if (fabsf(fbm(x * 0.05f, y * 0.1f, 62) - 0.5f) < 0.02f) v = 0.06f; }
        if (h01(x, y, 53) < 0.02f) v += 0.12f;
        matf(x, y, ramp, v, 0);
    }
    root(28, A_GROUND + 1, 1.6f, 36, 3.5f, 3, 3); root(22, A_GROUND + 1, 2.3f, 28, 2.4f, 3, 4); root(35, A_GROUND + 1, 0.9f, 32, 2.4f, 3, 5);
    for (int x = 0; x < W; x += 3) if (h01(x, 1, 63) < 0.6f) mline((float)x, A_GROUND + 1.0f, x + (h01(x, 2, 63) - 0.5f) * 3, A_GROUND + 3 + h01(x, 3, 63) * 5, 1, R_WOOD, 0.24f, 0);   /* the grass's roots */
    /* the buried: coffins under some of the stones; an old one burst, its dead in it; bones loose */
    coffin_side(70, 104, 28, 8, 0.42f, 0); coffin_side(146, 108, 28, 8, 0.38f, 0);
    {
        int bx = 222, by = 110;                                        /* the burst coffin, its lid gone askew */
        for (int i = 0; i < 32; i++) { matf(bx + i, by + 8, R_WOOD, 0.34f, 0); matf(bx + i, by + 9, R_WOOD, 0.26f, 0); }
        for (int j = 0; j < 9; j++) { matf(bx, by + j, R_WOOD, 0.34f, 0); matf(bx + 31, by + j, R_WOOD, 0.3f, 0); }
        for (int i = 0; i < 18; i++) matf(bx + 14 + i, by - 1 - i / 6, R_WOOD, 0.36f, 0);
        for (int i = 1; i < 31; i++) for (int j = 0; j < 8; j++) matf(bx + i, by + j, R_FLOOR, 0.08f, 0);
        skull_small(bx + 2, by + 2, 0.52f);
        for (int i = 7; i < 26; i++) matf(bx + i, by + 5, R_BONE, 0.44f, 0);          /* the spine, ribs, a leg */
        for (int i = 9; i < 16; i += 2) { matf(bx + i, by + 3, R_BONE, 0.4f, 0); matf(bx + i, by + 4, R_BONE, 0.4f, 0); matf(bx + i, by + 6, R_BONE, 0.36f, 0); }
        mrect(bx + 17, by + 4, 3, 3, R_BONE, 0.4f, 0);
        for (int i = 20; i < 30; i++) matf(bx + i, by + 6 + (i > 25), R_BONE, 0.42f, 0);
    }
    skull_small(118, 112, 0.42f); skull_small(52, 120, 0.36f); skull_small(186, 124, 0.34f);
    bone_small(124, 116, 6, 1, 0.38f); bone_small(30, 116, 7, -1, 0.34f); bone_small(258, 106, 6, 1, 0.36f); bone_small(176, 118, 5, -1, 0.35f);
    for (int x = 0; x < W; x += 2) {                                    /* grass along the edge */
        int g = (int)(h01(x, 0, 71) * 3);
        for (int j = 1; j <= g; j++) matf(x, A_GROUND - j, R_MOSS, 0.18f + 0.06f * j, 0);
    }
    (void)graves;
}
/* the crypt: arches of voussoirs between piers with capitals and bases; its back wall of blocks */
static int a_open(int x, int y) {                                       /* in the passage's air (seen against its back wall) */
    if (y >= A_FLOOR || y < A_CRYPT0) return 0;
    int bay = ((x % 64) + 64) % 64;
    if (bay < 6) return y > A_CRYPT0 + 16;                              /* a pier */
    float c = (bay - 35) / 29.0f;
    float top = A_CRYPT0 + 4 + 12 * (1 - sqrtf(fmaxf(0, 1 - c * c)));
    return y >= top;
}
static void a_crypt(float t) {
    for (int y = A_CRYPT0 - 8; y < A_FLOOR + 3; y++) for (int x = 0; x < W; x++) {
        int bay = ((x % 64) + 64) % 64;
        if (a_open(x, y)) {                                             /* the back wall: courses of blocks */
            int row = (y - A_CRYPT0) / 6, off = row & 1 ? 8 : 0;
            int mort = (y - A_CRYPT0) % 6 == 0 || ((x + off) % 16) == 0;
            float v = mort ? 0.12f : 0.3f + 0.12f * fbm(x * 0.2f, y * 0.2f, 81) + 0.05f * h01((x + off) / 16, row, 82);
            if (!mort && h01(x, y, 83) < 0.03f) v -= 0.12f;
            matf(x, y, R_STONE, v, 0);
            continue;
        }
        if (y >= A_FLOOR) { matf(x, y, R_STONE, 0.36f - 0.1f * (y - A_FLOOR) + ((x % 12) == 0 ? -0.1f : 0), 0); continue; }
        int near = 0;                                                   /* the ring of voussoirs round each arch */
        for (int d = 1; d <= 5 && !near; d++) if (a_open(x, y + d) && bay >= 6) near = d;
        if (near) {
            float cxb = (x / 64) * 64 + 35.0f, ang = atan2f((float)(y - (A_CRYPT0 + 17)), x - cxb);
            float fb = (ang + PI) / (PI / 9);
            float v = 0.4f - near * 0.03f + (((int)fb) % 2 ? 0.05f : -0.02f);
            if (fabsf(frac(fb) - 0.5f) > 0.42f) v = 0.14f;              /* the joints, fanning out */
            if (near == 5) v = 0.16f;
            matf(x, y, R_STONE, v + 0.04f * (h01(x, y, 84) - 0.5f), 0);
        } else if (bay < 6 || bay > 61) {                               /* the piers, their capitals and bases */
            int cap = y >= A_CRYPT0 + 13 && y <= A_CRYPT0 + 17, base_ = y >= A_FLOOR - 4;
            int inpier = bay < 6 || ((cap || base_) && bay > 61);
            if (!inpier && !(cap || base_)) continue;
            if (!inpier) continue;
            float v = 0.38f + (bay == 5 ? 0.1f : bay == 0 ? -0.12f : 0) + 0.06f * ((y / 6) % 2);
            if (cap || base_) v = 0.48f - (y == A_CRYPT0 + 17 || y == A_FLOOR - 1 ? 0.2f : 0);
            matf(x, y, R_STONE, v, 0);
        } else if (y > A_CRYPT0 - 8) matf(x, y, R_STONE, 0.18f + 0.06f * fbm(x * 0.1f, y * 0.1f, 85), 0);
    }
    for (int b = 0; b <= 5; b++) for (int y = A_CRYPT0 + 13; y <= A_CRYPT0 + 17; y++) for (int dx = -2; dx < 8; dx++) {   /* capitals and bases wider than the piers */
        int x = b * 64 + dx;
        matf(x, y, R_STONE, y == A_CRYPT0 + 17 ? 0.26f : 0.5f, 0);
        matf(x, A_FLOOR - 4 + (y - A_CRYPT0 - 13), R_STONE, y == A_CRYPT0 + 13 ? 0.52f : 0.4f, 0);
    }
    /* niches with skulls in some bays, a sarcophagus in one, a ledge of the dead in another */
    for (int b = 0; b < 6; b++) {
        int bx = b * 64 + 20;
        if (b == 3) {                                                   /* the sarcophagus, its lid carved with its sleeper */
            mrect(bx - 4, A_FLOOR - 11, 36, 11, R_STONE, 0.34f, 0);
            for (int i = 0; i < 36; i += 6) mrect(bx - 2 + i, A_FLOOR - 8, 4, 6, R_STONE, 0.24f, 0);
            mrect(bx - 5, A_FLOOR - 13, 38, 2, R_STONE, 0.48f, 0);
            mrect(bx + 2, A_FLOOR - 16, 24, 3, R_STONE, 0.44f, 0);       /* the effigy: head, hands, feet */
            mdisc(bx + 2, A_FLOOR - 16, 2.5f, R_STONE, 0.48f, 0);
            mrect(bx + 12, A_FLOOR - 18, 3, 2, R_STONE, 0.46f, 0);
            mrect(bx + 25, A_FLOOR - 18, 2, 3, R_STONE, 0.42f, 0);
        } else if (b % 2 == 0) {
            for (int k = 0; k < 2; k++) {
                int nx = bx + k * 16, ny = A_FLOOR - 26;
                mrect(nx, ny, 12, 9, R_STONE, 0.04f, 0);
                skull_small(nx + 3, ny + 3, 0.5f);
                if (k) bone_small(nx + 1, ny + 7, 8, 0, 0.4f);
                mrect(nx, ny + 9, 12, 1, R_STONE, 0.45f, 0);
            }
        } else {
            mrect(bx - 2, A_FLOOR - 9, 32, 9, R_STONE, 0.32f, 0);
            mrect(bx - 2, A_FLOOR - 9, 32, 1, R_STONE, 0.46f, 0);
            for (int i = 0; i < 30; i += 6) mrect(bx + i, A_FLOOR - 6, 3, 4, R_STONE, 0.22f, 0);
        }
    }
    for (int b = 1; b <= 4; b += 3) {                                   /* chains from the arches' crowns, swaying */
        float cxb = b * 64 + 35, sway = sinf(t * 2 * PI / LOOP * 3 + b) * 1.2f;
        for (int k = 0; k < 16; k++) matf((int)(cxb + sway * k / 16), A_CRYPT0 + 5 + k, R_IRON, k % 2 ? 0.32f : 0.24f, 0);
        mdisc(cxb + sway, A_CRYPT0 + 22, 1.6f, R_IRON, 0.3f, 0);
    }
    for (int b = 0; b <= 5; b++) {                                      /* cobwebs in the corners of the arches */
        int px = b * 64 + 6;
        for (int r = 3; r <= 9; r += 3) for (int a = 0; a < 10; a++) {
            float an = a / 9.0f * PI * 0.5f;
            matf(px + (int)(cosf(an) * r), A_CRYPT0 + 17 + (int)(sinf(an) * r) - r, R_BONE, 0.3f, MF_FLAT);
        }
        for (int a = 0; a < 3; a++) mline((float)px, A_CRYPT0 + 8.0f, px + cosf(a * 0.7f) * 10, A_CRYPT0 + 8 + sinf(a * 0.7f) * 10, 1, R_BONE, 0.26f, MF_FLAT);
    }
    for (int i = 0; i < 9; i++) bone_small(10 + i * 34 + (int)(h01(i, 0, 86) * 14), A_FLOOR - 1, 4 + i % 3, 0, 0.36f);   /* bones on the floor */
    for (int i = 0; i < 14; i++) {                                      /* dust coming down out of the cracks */
        float x = h01(i, 1, 91) * W, ph = h01(i, 2, 91), sp = 1 + (i % 2);
        float u = frac(t / LOOP * sp + ph);
        float y = A_CRYPT0 + 18 + u * (A_FLOOR - A_CRYPT0 - 18);
        if (a_open((int)x, (int)y)) matf((int)x, (int)y, R_BONE, 0.65f, MF_FLAT);
    }
}
static void scene_a(float t) {
    float flash = flash_k(t, LOOP * 0.55f);
    a_world(t, flash, 0);
    a_crypt(t);
    float wx = -24 + (W + 48) * (t / LOOP);                             /* the walker crosses once a loop */
    walker(wx, A_FLOOR, t, 1);
    float ex = 2 * 64 + 22 + 16 + 3, ey = A_FLOOR - 22;                 /* eyes in the dark of a niche, shut while the light is near */
    int eyes = fabsf(a_lantern_x - ex) > 60 && frac(t / 3.1f) > 0.06f;
    /* light: the moon on the ground, little in the earth, nothing in the crypt but the lantern */
    for (int y = A_GROUND; y < H; y++) for (int x = 0; x < W; x++) {
        float k = 0.85f - 0.55f * smooth(A_GROUND, A_CRYPT0 - 4, (float)y) - 0.24f * smooth(A_CRYPT0 - 10, A_CRYPT0 + 2, (float)y);
        LC[y][x] = k * (1.0f + 0.35f * flash * (y < A_CRYPT0 - 6));
    }
    a_flick = flick(t, 3);
    light_add(a_lantern_x, a_lantern_y, 82 * a_flick, 1.6f, 1);
    light_add(a_lantern_x, a_lantern_y, 20, 0.9f, 1);
    light_add(48, 79, 7, 0.25f * flick(t, 21), 1);                      /* the church's window */
    /* the walker's shadow, thrown big on the back wall away from the lantern */
    for (int y = A_CRYPT0; y < A_FLOOR; y++) for (int x = 0; x < W; x++) {
        if (WMASK[y][x] || !a_open(x, y)) continue;
        float qx = a_lantern_x + (x - a_lantern_x) / 1.7f, qy = a_lantern_y + (y - a_lantern_y) / 1.7f;
        int ix = (int)qx, iy = (int)qy;
        if (ix >= 0 && iy >= 0 && ix < W && iy < H && WMASK[iy][ix]) LW[y][x] *= 0.22f;
    }
    for (int i = 0; i < 2; i++) {                                       /* the bats over the moon, twice a loop */
        float u = frac(t / LOOP * 2 + i * 0.37f);
        bat(W + 10 - u * (W + 30), 30 + 12 * sinf(u * 9 + i) + i * 10, t * 26 + i);
    }
    if (MN.on && MN.appear > 0) {                                       /* the menu's stone, rising out of the earth */
        float e = 1 - powf(1 - MN.appear, 3), off = (1 - e) * 150 + (MN.appear > 0.9f && MN.appear < 1 ? sinf(MN.appear * 60) : 0);
        build_menu_stone(); sc_shade(0.46f, 920, 0, R_GOLD); sc_draw(W / 2, 156 + (int)off, 0);
        for (int i = 0; i < 24 && MN.appear < 1; i++) {                  /* earth falling off it as it comes */
            float x = 76 + h01(i, 1, 921) * 168, fall = frac(h01(i, 2, 921) + MN.appear * 3) * 30;
            matf((int)x, 60 + (int)off + (int)fall, R_FLOOR, 0.4f, MF_FLAT);
        }
        for (int y = 56; y < 158; y++) for (int x = 72; x < 248; x++) LC[y][x] = fmaxf(LC[y][x], 0.72f * MN.appear);
        int sel = menu_sel();
        light_add(W / 2, 60 + off + 28 + sel * 13 + 3, 70, 0.5f * MN.appear, 1);
    }
    title_foot_pre();
    fog(A_GROUND - 22, A_GROUND + 2, t, 1.5f, 33);
    if (flash > 0.7f) {                                                 /* lightning on the hills */
        float bx = 118, by = 0;
        for (int j = 0; j < 14; j++) {
            float nx = bx + (h01(j, 1, 5) - 0.5f) * 10, ny = by + 6;
            mline(bx, by, nx, ny, 1, R_GLOW, 1.0f, MF_FLAT | MF_NOWARM);
            bx = nx; by = ny;
            if (by > 82) break;
        }
    }
    vignette(0.35f);
    logo_build("HAUNTED", 0.95f, ".EXE", 0.84f, 1, 11);
    logo_draw((W - LG.w) / 2, 10, LS_BLOOD, 1.0f + 0.25f * flash, A_GROUND - 1, t);
    resolve();
    if (eyes) { F.pix[(int)ey * W + (int)ex] = R_BLOOD + 15; F.pix[(int)ey * W + (int)ex + 2] = R_BLOOD + 15; }
    for (int y = 0; y < H; y++) for (int x = 0; x < W; x++) if (MR[y][x] == R_GLOW && (MF[y][x] & MF_FLAT) && y < A_GROUND) F.pix[y * W + x] = C_WHITE;
    title_foot(189, 0, t);
    if (ver_word && MN.appear < 0.02f) gfx_text8(&F, W - 2 - 8 * (int)strlen(ver_word), 191, ver_word, R_STONE + 4, 1);
}

/* ---- the graves to choose from: what the ledger's pages would say (made up for the mock-ups) ---- */
static TitlePage TP[3];                                  /* the three pages, as the view gives them */
static float GK[3];                                     /* how chosen each */
static const char *foot_word;                           /* a word in place of the hints, or 0 */
static const char *const *page_lines(int g) { static const char *L[3][4]; for (int k = 0; k < 4; k++) L[g][k] = TP[g].line[k]; return L[g]; }
static const char *const *page_mark(int g) { return TP[g].cls >= 1 && TP[g].cls <= 6 ? CLASS_MARK[TP[g].cls - 1] : NULL; }
static void candle(int x, int base, int lit, float t, int seed) {
    mrect(x - 1, base - 7, 3, 7, R_BONE, 0.62f, 0);
    matf(x - 1, base - 7, R_BONE, 0.75f, 0);
    if (lit) flame(x, base - 8, 6, t, seed);
    else matf(x, base - 8, R_IRON, 0.2f, MF_FLAT);
}
static void mist(int cx, int base, float k, float t, uint32_t seed) {    /* a breath of cold mist off a grave */
    if (k <= 0) return;
    for (int y = base - 40; y < base + 1; y++) for (int x = cx - 46; x < cx + 46; x++) {
        float f = fbm((x - cx) * 0.05f, (y + t * 9) * 0.07f, seed) * (1 - fabsf((float)(x - cx)) / 46) * smooth((float)base - 40, (float)base - 4, (float)y);
        if (f * k > 0.4f + bay(x, y) * 0.32f && !(MF[y][x] & MF_DIRECT)) { MR[y][x] = R_GHOST; MV[y][x] = fmaxf(MV[y][x] * 0.6f, 0.14f + 0.22f * f * k); MF[y][x] = MF_NOWARM; }
    }
}
static void dead_flowers(int cx, int base, uint32_t seed) {            /* a few stems left at a grave, their heads hanging */
    for (int k = 0; k < 6; k++) {
        float x = cx - 8 + k * 3 + h01(k, 0, seed) * 2, hgt = 6 + h01(k, 1, seed) * 6, bend = (h01(k, 2, seed) - 0.3f) * 4;
        mline(x, (float)base, x + bend * 0.5f, base - hgt, 1, R_MOSS, 0.16f, 0);
        mline(x + bend * 0.5f, base - hgt, x + bend, base - hgt + 3, 1, R_MOSS, 0.14f, 0);
        mdisc(x + bend, base - hgt + 3.5f, 1.3f, k % 2 ? R_BLOOD : R_PURPLE, 0.22f, 0);
    }
}
/* one grave of the three, built and drawn: the stone (or the cross and its board), its flowers or candles */
static void grave_drawn(int g, int cx, int base, float k, float t) {
    float lean = g == 0 ? 1.5f : -2.0f;
    if (TP[g].used) {
        if (g != 1) build_stele(page_mark(g), TP[g].num, page_lines(g), 4, 100 + (uint32_t)g);
        else build_wheelcross(page_mark(g), TP[g].num, page_lines(g), 4, 100 + (uint32_t)g);
        sc_shade(0.44f, 200 + (uint32_t)g, k, R_GOLD);
        sc_draw(cx, base + 1, lean);
        if (g != 1) {                                                   /* the broken corner, where it fell */
            build_fragment(17, 9, 300); sc_shade(0.42f, 301, 0, R_GOLD); sc_draw(cx + 60, base + 1, 18);
            build_fragment(8, 5, 302); sc_shade(0.42f, 303, 0, R_GOLD); sc_draw(cx + 72, base + 1, -10);
        }
        for (int c = 0; c < 2; c++) candle(cx + (c ? 49 : -50), base - 9, k > 0.5f, t, g * 2 + c);
        if (k <= 0.5f) dead_flowers(cx + (g ? 18 : -18), base - 10, 50 + (uint32_t)g);
    } else {
        float a = 5 * PI / 180;
        build_timber_cross(400); sc_shade(0.4f, 401, 0, R_GOLD); sc_draw(cx, base + 2, 5);
        /* the board hangs from the arms on two cords */
        float ax[2], ay[2];
        for (int e = 0; e < 2; e++) { float rx = e ? 22.0f : -22.0f, ry = -67.0f; ax[e] = cx + rx * cosf(a) - ry * sinf(a); ay[e] = base + 2 + rx * sinf(a) + ry * cosf(a); }
        int py = base - 52;
        mline(ax[0], ay[0], cx - 34.0f, (float)py + 2, 1, R_WOOD, 0.45f, 0);
        mline(ax[1], ay[1], cx + 34.0f, (float)py + 2, 1, R_WOOD, 0.45f, 0);
        build_plank(410, TP[g].num); sc_shade(0.42f, 411, k, R_BONE); sc_draw(cx, py + 42, 0);
        for (int c = 0; c < 2; c++) candle(cx + (c ? 30 : -30), base, k > 0.5f, t, g * 2 + c);
    }
}
static void a_graves(float t) {
    const int G = 124;
    pal_night();
    light_clear(0.75f, 0);
    sky(0, G, t, 228, 48, 8, 0);
    for (int x = 0; x < W; x++) {                                      /* the hills behind */
        float hy = 98 + 5 * sinf(x * 0.025f + 0.4f) + 3 * sinf(x * 0.07f) + 2 * fbm(x * 0.05f, 0, 8);
        for (int y = (int)hy; y < G; y++) matf(x, y, R_CEIL, 0.09f, MF_NOWARM);
    }
    for (int y = G; y < H; y++) for (int x = 0; x < W; x++) {          /* the earth in its bands */
        float n = fbm(x * 0.07f, y * 0.11f, 51), strata = 0.05f * sinf(y * 0.55f + 2 * fbm(x * 0.02f, y * 0.02f, 52));
        float v = 0.28f + 0.26f * (n - 0.5f) * 1.6f + strata - 0.16f * (float)(y - G) / (H - G);
        if (y < G + 3) v -= 0.1f;
        if (h01(x, y, 53) < 0.03f) v += 0.14f;
        int band = y > G + 30 && y < G + 36 + (int)(3 * sinf(x * 0.05f));
        matf(x, y, band ? R_BRICK : R_FLOOR, band ? v * 0.7f : v, 0);
    }
    static const int CX[3] = { 57, 160, 263 };
    for (int g = 0; g < 2; g++) {                                      /* the shafts, the coffins in them */
        for (int y = G; y < G + 28; y++) for (int x = CX[g] - 36; x < CX[g] + 36; x++) if (!(MF[y][x] & MF_DIRECT)) MV[y][x] += 0.05f * (h01(x, y, 57 + g) - 0.3f) + ((x == CX[g] - 36 || x == CX[g] + 35) ? -0.1f : 0);
        coffin_side(CX[g] - 34, G + 12, 68, 12, 0.36f, 0.25f * GK[g]);
    }
    skull_small(CX[2] + 14, G + 20, 0.36f); bone_small(CX[2] - 22, G + 24, 7, 1, 0.34f);
    for (int x = 0; x < W; x += 2) {                                    /* the turf */
        int gg = (int)(h01(x, 0, 71) * 3);
        for (int j = 1; j <= gg; j++) matf(x, G - j, R_MOSS, 0.2f + 0.06f * j, 0);
    }
    for (int g = 0; g < 3; g++) grave_drawn(g, CX[g], G, GK[g], t);
    for (int g = 0; g < 3; g++) mist(CX[g], G, GK[g], t, 300 + g);
    fog(G - 14, G + 1, t, 1.1f, 34);
    ui_beam("RISE FROM WHICH GRAVE?", t);
    if (!foot_word) ui_keys_worn(184, GKEYS, SAYS3, 3);
    /* light: the moon, a candle's warmth on the chosen, the coffin's lid lit from within */
    for (int y = G; y < H; y++) for (int x = 0; x < W; x++) LC[y][x] = y < 158 ? 0.55f - 0.25f * (y - G) / (H - G) : 0.62f;
    for (int g = 0; g < 3; g++) {
        float k = GK[g];
        if (k <= 0) continue;
        float fl = flick(t, g);
        light_add(CX[g], G - 14, 76 * fl, 1.05f * k, 1);
        light_add(CX[g], G - 50, 60, 0.3f * k, 0);
        if (TP[g].used) light_add(CX[g], G + 14, 40, 0.8f * k, 0);
    }
    vignette(0.45f);
    ui_light(160, H, 0.6f);
    resolve();
    for (int g = 0; g < 2; g++) {                                      /* the coffins' cracks of light while chosen */
        float k = GK[g];
        if (k < 0.3f) continue;
        for (int i = 4; i < 64; i++) if (h01(i, g, 7) < 0.7f) F.pix[(G + 12) * W + CX[g] - 34 + i] = (uint8_t)(R_GLOW + clampi((int)(6 + 8 * k), 0, 15));
    }
    if (foot_word) { int w = (int)strlen(foot_word) * 8; ui_band(&F, W / 2 - w / 2 - 6, 182, w + 12, 15); ui_text8(&F, W / 2 - w / 2, 186, foot_word, FACE_INSC, R_BLOOD + 13, TX_INLAID); }
    else ui_keys_words(184, GKEYS, SAYS3, 3);
}

/* ======================================================================================================================
 * B, THE WALK: run I's hall in the first person, walked slowly towards its dark, the game's own textures and light
 * table; seen through an arch whose lintel carries the name
 * ====================================================================================================================== */
#define B_PERIOD 4                                       /* the hall's furniture comes round every four cells */
#define B_PROJ 186.0f
#define B_EYE 0.5f
static float b_camx, b_hor;
static float b_torch(float t, int k) { return flick(t, k * 7 + 3); }
static float b_light(float wx, float wy, float wz, float dist, float t) {   /* torches each side, the walker's own glow, fog */
    float l = 0.1f;
    for (int c = (int)floorf(b_camx) - 2; c <= (int)floorf(b_camx) + 14; c++) {
        int k = ((c % B_PERIOD) + B_PERIOD) % B_PERIOD;
        if (k != 0 && k != 2) continue;
        float tx = c + 0.5f, ty = k == 0 ? 1.08f : 2.92f, tz = 0.62f;
        float d2 = (wx - tx) * (wx - tx) + (wy - ty) * (wy - ty) + (wz - tz) * (wz - tz);
        l += 1.5f * b_torch(t, c) / (1 + 3.2f * d2);
    }
    float d2c = (wx - b_camx) * (wx - b_camx) + (wy - 2) * (wy - 2);
    l += 0.35f / (1 + 2.5f * d2c);
    return l * expf(-dist / 5.5f);
}
static uint8_t b_lit(uint8_t texel, float l) {
    if (texel == C_CLEAR) return 0;
    return crawl_cmap[clampi((int)(l * LIGHT_UNIT), 0, NLIGHT - 1)][texel];
}
static int b_wall_tex(int cell, int left, int texx, int texy, float t) {   /* a wall's texel: its stone, what stands on it */
    int k = ((cell % B_PERIOD) + B_PERIOD) % B_PERIOD;
    uint8_t c = crawl_tex[T_MATV(M1_ASHLAR, (cell >> 1) & 1)][texx * TS + texy];
    int over = -1;
    if (left && k == 0) over = T_FEAT + F1_TORCH + ((int)(t * 9) + cell) % 3;
    if (!left && k == 2) over = T_FEAT + F1_TORCH + ((int)(t * 9) + cell + 1) % 3;
    if (left && k == 2) over = T_FEAT + F1_NICHE;
    if (!left && k == 0) over = T_FEAT + F1_SLAB;
    if (!left && k == 1 && (cell / B_PERIOD) % 2) over = T_FEAT + F1_BANNER;
    if (over >= 0) { uint8_t o = crawl_tex[over][texx * TS + texy]; if (o != C_CLEAR) c = o; }
    return c;
}
static float b_depth[W];
static float b_vig(int x, int y) { float dx = (x - W * 0.5f) / (W * 0.5f), dy = (y - 120.0f) / 100.0f; return 1 - 0.4f * smooth(0.5f, 1.2f, sqrtf(dx * dx * 0.8f + dy * dy)); }
/* the game's own props in the hall, as billboards against its depth: on the floor, or hung from the vault */
static void b_sprite(float wx, float wy, int spr, int hang, float sc, float t) {
    float dx = wx - b_camx, dy = wy - 2.0f;
    if (dx < 1.5f || dx > 14) return;                                    /* close by, it has gone past behind the piers */
    float size = B_PROJ / dx * sc, sx = (dy / (dx * 0.66f) + 1) * W * 0.5f;
    float top = hang ? b_hor - B_PROJ * (1 - B_EYE) / dx : b_hor + B_PROJ * B_EYE / dx - size;
    float l = b_light(wx, wy, hang ? 0.8f : 0.25f, dx, t);
    for (int c = (int)(sx - size / 2); c < (int)(sx + size / 2); c++) {
        if (c < 0 || c >= W || dx > b_depth[c] - 0.05f) continue;
        int u = clampi((int)((c - (sx - size / 2)) / size * TS), 0, TS - 1);
        for (int r = (int)top; r < (int)(top + size); r++) {
            if (r < 50 || r >= H) continue;
            int v = clampi((int)((r - top) / size * TS), 0, TS - 1);
            uint8_t tx = crawl_spr[spr][u * TS + v];
            if (tx == C_CLEAR) continue;
            direct(c, r, IS_FULLBRIGHT(tx) ? tx : b_lit(tx, l * b_vig(c, r)));
        }
    }
}
static void b_sprites(float t) {
    static const struct { float x, y; int spr, hang; float sc; } BP[] = {
        { 0.7f, 1.32f, SP_BONES2, 0, 0.7f }, { 1.6f, 1.25f, SP_CANDLES, 0, 0.6f }, { 2.45f, 2.72f, SP_SKULLPILE, 0, 0.6f },
        { 3.4f, 2.62f, SP_GIBBET_SKEL, 1, 0.78f }, { 1.25f, 2.55f, SP_CHAINS, 1, 0.8f }, { 2.9f, 1.3f, SP_ROOTS, 1, 0.8f },
        { 3.75f, 2.72f, SP_URN, 0, 0.55f }, { 0.25f, 2.7f, SP_COBWEB, 1, 0.7f } };
    int n = (int)(sizeof BP / sizeof BP[0]);
    for (int blk = (int)floorf(b_camx / B_PERIOD) + 3; blk >= (int)floorf(b_camx / B_PERIOD); blk--)   /* far first */
        for (int k = n - 1; k >= 0; k--) b_sprite(blk * B_PERIOD + BP[k].x, BP[k].y, BP[k].spr, BP[k].hang, BP[k].sc, t);
    /* a spectre crossing the hall at the edge of the fog, once a loop */
    float u = frac(t / LOOP + 0.1f);
    if (u > 0.25f && u < 0.75f) {
        float wy = 1.15f + (u - 0.25f) / 0.5f * 1.7f, wx = b_camx + 7.5f;
        float dx = wx - b_camx, size = B_PROJ / dx * 0.95f, sx = ((wy - 2) / (dx * 0.66f) + 1) * W * 0.5f;
        float top = b_hor + B_PROJ * B_EYE / dx - size - 3 * sinf(t * 3);
        int spr = (int)(t * 4) % 2 ? SP_GHOST_A : SP_GHOST_B;
        float a = smooth(0.25f, 0.33f, u) * (1 - smooth(0.67f, 0.75f, u));
        for (int c = (int)(sx - size / 2); c < (int)(sx + size / 2); c++) {
            if (c < 0 || c >= W) continue;
            int uu = clampi((int)((c - (sx - size / 2)) / size * TS), 0, TS - 1);
            for (int r = (int)top; r < (int)(top + size); r++) {
                if (r < 50 || r >= H || bay(c, r) > a * 0.6f) continue;
                uint8_t tx = crawl_spr[spr][uu * TS + clampi((int)((r - top) / size * TS), 0, TS - 1)];
                if (tx != C_CLEAR) direct(c, r, crawl_cmap[18][tx]);
            }
        }
    }
}
static void b_hall(float t, int y0, int y1) {
    float step = b_camx * 2 * PI;                                       /* two steps a cell */
    b_hor = 100 + sinf(step) * 1.2f;
    float camy = 2.0f + sinf(step * 0.5f) * 0.03f;
    for (int x = 0; x < W; x++) {
        float cxs = (2.0f * x / W - 1) * 0.66f, rx = 1, ry = cxs;
        float d = ry < 0 ? (1 - camy) / ry : ry > 0 ? (3 - camy) / ry : 1e9f;
        if (d > 60) d = 60;
        b_depth[x] = d;
        float wx = b_camx + d;
        int cell = (int)floorf(wx), left = ry < 0;
        int texx = (int)(frac(wx) * TS); if (!left) texx = TS - 1 - texx;
        float top = b_hor - B_PROJ * (1 - B_EYE) / d, bot = b_hor + B_PROJ * B_EYE / d;
        for (int y = y0; y < y1; y++) {
            uint8_t c;
            if (y >= top && y < bot) {                                    /* the wall */
                float v = (y - top) / (bot - top);
                int texy = clampi((int)(v * TS), 0, TS - 1);
                uint8_t tx = (uint8_t)b_wall_tex(cell, left, texx, texy, t);
                float wz = 1 - v;
                c = IS_FULLBRIGHT(tx) ? tx : b_lit(tx, b_light(wx, left ? 1.0f : 3.0f, wz, d, t) * b_vig(x, y));
            } else {
                int floor = y >= bot;
                float dist = floor ? B_PROJ * B_EYE / (y - b_hor + 0.5f) : B_PROJ * (1 - B_EYE) / (b_hor - y + 0.5f);
                float fx = b_camx + dist * rx, fy = camy + dist * ry;
                int tu = (int)(frac(fx) * TS) & 63, tv = (int)(frac(fy) * TS) & 63;
                uint8_t tx = crawl_tex[floor ? T_FLOORS + FL1_FLAGS : T_CEILS + CE1_RIB][tu * TS + tv];
                c = b_lit(tx, b_light(fx, fy, floor ? 0 : 1, dist, t) * (floor ? 1.0f : 0.8f) * b_vig(x, y));
            }
            direct(x, y, c);
        }
    }
    /* dust in the torches' light, drifting */
    for (int i = 0; i < 90; i++) {
        float px = (floorf(b_camx / B_PERIOD) + (i % 3)) * B_PERIOD + h01(i, 1, 13) * B_PERIOD;
        float py = 1.15f + h01(i, 2, 13) * 1.7f, pz = frac(h01(i, 3, 13) + t / LOOP * (1 + i % 2)) ;
        pz = 0.15f + 0.8f * (0.5f + 0.5f * sinf(2 * PI * (h01(i, 3, 13) + t / LOOP * (1 + i % 2))));
        float dx = px - b_camx, dy = py - camy;
        if (dx < 0.3f) continue;
        int sx = (int)((dy / (dx * 0.66f) + 1) * W / 2), sy = (int)(b_hor - (pz - B_EYE) * B_PROJ / dx);
        if (sx < 0 || sx >= W || sy < y0 || sy >= y1 || dx > b_depth[sx]) continue;
        float l = b_light(px, py, pz, dx, t);
        if (l > 0.55f) direct(sx, sy, (uint8_t)(R_BONE + clampi((int)(l * 8), 3, 14)));
    }
}
/* the arch it is seen through: two piers and a lintel of great blocks; the name cut in the lintel */
static int b_arch(int x, int y) {                                       /* 0 the opening, 1 a pier, 2 the lintel */
    if (y < 62) return 2;
    if (x < 24 || x >= W - 24) return 1;
    float c = (x - W * 0.5f) / (W * 0.5f - 24);
    float top = 62 + 10 * (1 - sqrtf(fmaxf(0, 1 - c * c)));               /* the opening's segmental head */
    return y < top ? 2 : 0;
}
/* the arch built: blocks each their own, chipped, cracked, stained and grown over; a keystone with a skull cut in it; a
 * piece gone from the opening's edge; the step worn hollow where feet have gone */
static void b_build_frame(void) {
    sc_begin(W, H);
    int id = 1;
    for (int row = 0; row < 2; row++) {                                 /* the lintel's two courses */
        int off = row ? 32 : 0;
        for (int bx = -off; bx < W; bx += 64) {
            for (int y = row * 31; y < row * 31 + 31; y++) for (int x = bx; x < bx + 64; x++)
                if (x >= 0 && x < W && b_arch(x, y) == 2) sc_px(x, y, id, 0, SM_STONE, 0.04f * (h01(id, 0, 3) - 0.5f));
            id++;
        }
    }
    for (int y = 62; y < 76; y++) for (int x = 24; x < W - 24; x++) if (b_arch(x, y) == 2) sc_px(x, y, id, 0, SM_STONE, -0.02f);   /* the head's voussoirs */
    for (int x = 24; x < W - 24; x += 18) for (int y = 62; y < 76; y++) if (sc_solid(x, y)) S.id[y][x] = (uint8_t)(id + 1);
    id += 2;
    for (int side = 0; side < 2; side++) for (int y = 62; y < 184; y += 23) {   /* the piers' blocks */
        int x0 = side ? W - 24 : 0;
        for (int yy = y; yy < y + 23 && yy < 184; yy++) for (int x = x0; x < x0 + 24; x++) sc_px(x, yy, id, 0, SM_STONE, 0.04f * (h01(id, 1, 3) - 0.5f));
        id++;
    }
    for (int y = 184; y < H; y++) for (int x = 0; x < W; x++) {         /* the step, worn hollow in the middle */
        float worn = 1 - fabsf(x - W * 0.5f) / 70.0f;
        sc_px(x, y, 240, 0, SM_STONE, -0.08f + 0.12f * fmaxf(0, worn) * (y > 186));
    }
    float kb[8] = { 141, 56, 179, 56, 172, 86, 148, 86 };               /* the keystone, a wedge standing proud */
    sc_poly(kb, 4, 241, 1, SM_STONE, 0.12f);
    for (int y = 60; y < 83; y++) for (int x = 146; x < 175; x++) if (sc_solid(x, y) && S.id[y][x] == 241) { float e = (x - 160) / (14.0f - (y - 60) * 0.12f); if (fabsf(e) < 0.82f) S.ht[y][x] = 0; }   /* its sunk field */
    sc_mark(160 - 11, 61, KEY_SKULL, 11, 2, 2, 0.1f);                   /* the skull, in relief */
    for (int y = 56; y < 87; y++) for (int x = 140; x < 180; x++) if (sc_solid(x, y) && S.id[y][x] == 241) {
        if (S.ht[y][x] == 2) S.dv[y][x] += 0.18f;
        else if (S.ht[y][x] == 0) { S.dv[y][x] -= 0.14f; S.f[y][x] |= SF_CALM; }
    }
    for (int y = 0; y < 76; y++) for (int x = 0; x < W; x++) if (sc_solid(x, y) && S.id[y][x] != 241 && (S.ht[y][x] == 0) && y >= 6 && y < 56 && x > 8 && x < W - 8) S.f[y][x] |= SF_CALM;   /* quiet under the name */
    sc_break(W - 24, 96, 7, 51);                                         /* a bite out of the opening's edge */
    sc_break(-2, 30, 9, 52);                                             /* the lintel's end crumbled */
    sc_crack(60, 0, 1.4f, 30, 53, 2); sc_crack(250, 61, -1.8f, 26, 54, 2); sc_crack(10, 120, 0.3f, 18, 55, 1);
    sc_crack(W - 6, 150, 2.6f, 16, 56, 1); sc_crack(110, 184, -0.3f, 30, 57, 1);
    sc_chips(0.3f, 58);
    sc_ivy(4, 183, 70, 59); sc_ivy(9, 183, 40, 60); sc_ivy(W - 4, 183, 46, 61);
}
static void b_frame(float t, float glow) {
    b_build_frame();
    sc_shade(0.46f, 62, 0, R_GOLD);
    sc_draw(W / 2, H, 0);
    light_clear(0.36f, 0);
    for (int y = 0; y < H; y++) for (int x = 0; x < W; x++) {          /* the hall's glow on the arch's inside */
        if (!b_arch(x, y)) continue;
        float dx = fminf((float)abs(x - 24), (float)abs(x - (W - 24))), dy = 72.0f - y;
        float dd = b_arch(x, y) == 2 ? fmaxf(0, dy) : dx;
        LW[y][x] = glow * 0.9f * expf(-dd / 22.0f);
    }
    light_add(160, 92, 40, 0.55f * glow, 1);                             /* the keystone, lit from the hall under it */
    (void)t;
}
static void scene_b(float t) {
    memcpy(F.pal, BASEPAL, sizeof F.pal);
    b_camx = 0.5f + t / LOOP * B_PERIOD * 2;
    for (int y = 0; y < H; y++) for (int x = 0; x < W; x++) { MF[y][x] = 0; MR[y][x] = 0; }
    b_hall(t, 50, H);
    b_sprites(t);
    float tab_y = 84;
    if (MN.on && MN.appear > 0) {                                       /* the menu's tablet, let down on its chains */
        float a = MN.appear, e = a < 0.8f ? 1 - powf(1 - a / 0.8f, 2) : 1 + 0.04f * sinf((a - 0.8f) / 0.2f * PI);
        tab_y = 84 - (1 - e) * 110;
        build_menu_tablet(); sc_shade(0.46f, 930, 0, R_GOLD); sc_draw(W / 2, (int)tab_y + 88, 0);
        for (int e2 = 0; e2 < 2; e2++) {                                 /* its chains, up to the lintel */
            int cx = W / 2 - 90 + (e2 ? 156 : 24);
            for (int y = 60; y < (int)tab_y + 2; y++) {
                int link = ((y - (int)tab_y) % 6 + 6) % 6;
                if (link < 4) { matf(cx - 1, y, R_IRON, 0.3f, MF_FLAT); matf(cx + 1, y, R_IRON, 0.24f, MF_FLAT); }
                else matf(cx, y, R_IRON, 0.34f, MF_FLAT);
            }
        }
    }
    b_frame(t, 0.85f * flick(t, 11));
    if (MN.on && MN.appear > 0) {
        for (int y = (int)tab_y; y < (int)tab_y + 90; y++) for (int x = 68; x < 252; x++) if (y >= 0 && y < H) LC[y][x] = fmaxf(LC[y][x], 0.7f * MN.appear);
        light_add(W / 2, tab_y + 26 + menu_sel() * 13 + 3, 70, 0.5f * MN.appear, 1);
    }
    title_foot_pre();
    /* roots down from the lintel into the opening, stirring in the draught; cobwebs in its corners */
    for (int k = 0; k < 6; k++) {
        float x = 40 + k * 47 + h01(k, 0, 90) * 16, len = 8 + h01(k, 1, 90) * 16, sw = sinf(2 * PI * t / LOOP * 2 + k) * 1.3f;
        float y0 = 62 + 10 * (1 - sqrtf(fmaxf(0, 1 - powf((x - W * 0.5f) / (W * 0.5f - 24), 2)))) - 1;
        mline(x, y0, x + sw * 0.5f, y0 + len * 0.6f, 1.6f, R_WOOD, 0.22f, 0);
        mline(x + sw * 0.5f, y0 + len * 0.6f, x + sw, y0 + len, 1, R_WOOD, 0.2f, 0);
        if (k % 2) mline(x + sw * 0.5f, y0 + len * 0.6f, x + sw + 3, y0 + len * 0.8f, 1, R_WOOD, 0.18f, 0);
    }
    for (int e = 0; e < 2; e++) {
        int cx = e ? W - 25 : 24, sx = e ? -1 : 1;
        for (int r = 4; r <= 16; r += 4) for (int a = 0; a <= 12; a++) { float an = a / 12.0f * PI * 0.5f; matf(cx + sx * (int)(cosf(an) * r), 66 + (int)(sinf(an) * r), R_BONE, 0.3f, MF_FLAT); }
        for (int a = 0; a < 4; a++) mline((float)cx, 66, cx + sx * cosf(a * 0.5f) * 18, 66 + sinf(a * 0.5f) * 18, 1, R_BONE, 0.26f, MF_FLAT);
    }
    /* candles on the step's ends */
    static const int CA[6] = { 7, 12, 17, 303, 308, 313 }, CH[6] = { 2, 5, 0, 1, 4, 0 };
    for (int i = 0; i < 6; i++) { mrect(CA[i] - 1, 186 - 6 - CH[i], 3, 6 + CH[i], R_BONE, 0.6f, 0); matf(CA[i] - 1, 186 - 6 - CH[i], R_BONE, 0.75f, 0); }
    light_add(14, 176, 60, 0.7f * flick(t, 31), 1); light_add(306, 176, 60, 0.7f * flick(t, 32), 1);
    for (int i = 0; i < 6; i++) flame(CA[i], 186 - 7 - CH[i], 5, t, i + 40);
    logo_build("HAUNTED", 0.95f, ".EXE", 0.84f, 1, 21);
    logo_draw((W - LG.w) / 2, 6, LS_CARVED, 1.05f + 0.1f * flick(t, 4), H - 2, t);
    resolve();
    title_foot(190, 0, t);
}

/* a loculus's slab: a moulded border, iron cramps top and foot, the class's mark cut at its head, the words; a corner
 * broken and cracks; 86 by 92 */
static void build_loculus_slab(const char *const *mark, const char *num, const char *const *lines, uint32_t seed) {
    sc_begin(86, 92);
    sc_rect(0, 0, 86, 92, 1, 0, SM_STONE, 0.06f);
    sc_groove(1u << 1, 3);
    sc_groove(1u << 1, 5);
    for (int k = 0; k < 2; k++) {                                        /* the cramps */
        int y = k ? 86 : 1;
        sc_rect(37, y, 12, 5, 2, 1, SM_IRON, 0.1f);
        sc_disc(40, y + 2.5f, 1, 2, 2, SM_IRON, 0.25f); sc_disc(46, y + 2.5f, 1, 2, 2, SM_IRON, 0.25f);
        for (int jj = 0; jj < 7; jj++) if (sc_solid(43, y + 5 + jj) && !k) { S.m[y + 5 + jj][43] = SM_IRON; S.dv[y + 5 + jj][43] = -0.12f; }   /* rust run down */
    }
    if (mark) sc_mark(43 - 5, 9, mark, 11, 1, -1, 0);
    sc_text_c(43, 23, num, 0, 1, seed, 78);
    for (int k = 0; k < 4; k++) sc_text_c(43, 37 + k * 10, lines[k], 0, 1, seed + 1 + (uint32_t)k, 78);
    sc_break(2, 89, 7, seed + 7);
    sc_crack(70, 0, 1.9f, 40, seed + 8, 2);
    sc_crack(0, 30, 0.2f, 14, seed + 9, 1);
    sc_chips(0.3f, seed + 10);
}
/* a tablet set in the wall, the title cut in it and gilt */
static void ui_wall_tablet(int cx, int y, int w, const char *title) {
    sc_begin(w, 24);
    float p[16] = { 4, 0, (float)w - 4, 0, (float)w, 4, (float)w, 20, (float)w - 4, 24, 4, 24, 0, 20, 0, 4 };
    sc_poly(p, 8, 1, 0, SM_STONE, 0.08f);
    sc_groove(1u << 1, 3);
    for (int k = 0; k < 2; k++) { int x = k ? w - 9 : 4; sc_rect(x, 9, 5, 6, 2, 1, SM_IRON, 0.1f); }
    sc_text(w / 2 - sc_text_w(title, 8) / 2, 4, title, 1, 8, 1, 43);
    sc_crack((float)w - 30, 0, 1.8f, 20, 9, 1);
    sc_chips(0.3f, 10);
    sc_shade(0.42f, 46, 0.9f, R_GOLD);
    sc_draw(cx, y + 24, 0);
}
static void scene_b_graves(float t) {
    memcpy(F.pal, BASEPAL, sizeof F.pal);
    static const int CX[3] = { 58, 160, 262 };
    const int NY0 = 44, NY1 = 148;
    float lights[5][3];
    int nl = 0;
    for (int g = 0; g < 3; g++) { float k = GK[g]; if (k > 0) { lights[nl][0] = CX[g]; lights[nl][1] = NY1 + 4; lights[nl][2] = 1.3f * k * flick(t, g); nl++; } }
    lights[nl][0] = 4; lights[nl][1] = 80; lights[nl][2] = 0.8f * flick(t, 9); nl++;
    lights[nl][0] = W - 4; lights[nl][1] = 80; lights[nl][2] = 0.8f * flick(t, 10); nl++;
    for (int y = 0; y < H; y++) for (int x = 0; x < W; x++) {          /* the wall: run I's stone, by the candles and two torches */
        float l = 0.14f;
        for (int i = 0; i < nl; i++) { float dx = x - lights[i][0], dy = (y - lights[i][1]) * 1.2f; l += lights[i][2] / (1 + (dx * dx + dy * dy) / 2600.0f); }
        float dxv = (x - W * 0.5f) / (W * 0.5f), dyv = (y - H * 0.5f) / (H * 0.5f);
        l *= 1 - 0.45f * smooth(0.55f, 1.25f, sqrtf(dxv * dxv * 0.8f + dyv * dyv * 1.1f));
        int tx = ((x % 64) + 64) % 64, ty = y % 64;
        uint8_t c = crawl_tex[T_MATV(M1_ASHLAR, ((x / 64) + (y / 64)) & 1)][tx * TS + ty];
        direct(x, y, b_lit(c, l));
        LC[y][x] = l * 0.45f; LW[y][x] = l * 0.55f;
    }
    for (int g = 0; g < 3; g++) {
        float k = GK[g];
        int x0 = CX[g] - 45, x1 = CX[g] + 45;
        /* the niche: an arch of voussoirs, a skull for its keystone, the dark inside */
        for (int y = NY0 - 10; y < NY1 + 2; y++) for (int x = x0 - 9; x < x1 + 9; x++) {
            float c = (x - CX[g]) / 54.0f, top = NY0 - 10 + 22 * (1 - sqrtf(fmaxf(0, 1 - c * c)));
            if (y < top) continue;
            float ci = (x - CX[g]) / 45.0f, itop = NY0 + 20 * (1 - sqrtf(fmaxf(0, 1 - ci * ci)));
            int inside = x >= x0 && x < x1 && y >= itop && y < NY1;
            if (inside) { matf(x, y, R_STONE, 0.04f + 0.03f * (y > NY1 - 10), 0); continue; }
            float ang = atan2f((float)(y - (NY0 + 20)), (float)(x - CX[g]));
            int block = (int)floorf((ang + PI) / (PI / 11)), ring = y < NY0 + 22;
            float v = 0.46f + 0.06f * (block % 2) + 0.08f * (fbm(x * 0.2f, y * 0.2f, 70 + g) - 0.5f);
            if (ring && fabsf(fmodf((ang + PI) / (PI / 11), 1.0f) - 0.5f) > 0.45f) v = 0.16f;   /* the joints between voussoirs */
            if (!ring && (x == x0 - 1 || x == x1)) v = 0.2f;
            if (!ring && (y - NY0) % 14 == 0) v = 0.2f;
            if (h01(x, y, 71 + g) < 0.03f) v -= 0.15f;
            matf(x, y, R_STONE, v, 0);
        }
        skull_small(CX[g] - 2, NY0 - 9, 0.62f);
        for (int y = NY1; y < NY1 + 7; y++) for (int x = x0 - 10; x < x1 + 10; x++) {   /* the sill, chipped */
            if ((x == x0 - 10 || x == x1 + 9) && h01(x, y, 5) < 0.5f) continue;
            if (y == NY1 && h01(x, 0, 6 + g) < 0.08f) continue;
            matf(x, y, R_STONE, (y == NY1 ? 0.6f : y > NY1 + 4 ? 0.26f : 0.42f) + 0.06f * (h01(x, y, 7) - 0.5f), 0);
        }
        if (TP[g].used) {
            int dxs = (int)(10 * k);
            for (int y = NY0 + 16; y < NY1 - 1; y++) for (int x = x0 + 2; x < x0 + 2 + dxs; x++)   /* the cold behind the slab drawn aside */
                if (x >= x0 + dxs - 1 || h01(x, y + (int)(t * 6), 3) < 0.04f * k) matf(x, y, R_GLOW, 0.12f + 0.2f * k, MF_NOWARM | MF_FLAT);
            build_loculus_slab(page_mark(g), TP[g].num, page_lines(g), 700 + (uint32_t)g);
            sc_shade(0.48f, 710 + (uint32_t)g, k, R_GOLD);
            sc_draw(CX[g] + dxs, NY1, 0);
        } else {                                                        /* empty: its slab fallen in two, a board hung where it was */
            for (int r = 4; r <= 16; r += 4) for (int a = 0; a <= 12; a++) { float an = a / 12.0f * PI * 0.5f; matf(x1 - 1 - (int)(cosf(an) * r), NY0 + 14 + (int)(sinf(an) * r) - 2, R_BONE, 0.4f, MF_FLAT); }
            build_plank(410, TP[g].num); sc_shade(0.44f, 411, k, R_BONE); sc_draw(CX[g], NY0 + 66, 0);
            mline((float)CX[g], NY0 + 18.0f, CX[g] - 30.0f, NY0 + 25.0f, 1, R_WOOD, 0.45f, 0);
            mline((float)CX[g], NY0 + 18.0f, CX[g] + 30.0f, NY0 + 25.0f, 1, R_WOOD, 0.45f, 0);
            matf(CX[g], NY0 + 17, R_IRON, 0.6f, 0);
            build_fragment(34, 22, 720); sc_shade(0.44f, 721, 0, R_GOLD); sc_draw(CX[g] - 20, NY1, -14);
            build_fragment(28, 18, 722); sc_shade(0.42f, 723, 0, R_GOLD); sc_draw(CX[g] + 22, NY1, 22);
            skull_small(CX[g] - 3, NY1 - 7, 0.45f);
        }
        candle(CX[g] + 36, NY1, k > 0.5f, t, g);
        candle(CX[g] + 40, NY1, k > 0.5f, t, g + 3);
    }
    for (int g = 0; g < 3; g++) { float k = GK[g]; if (k > 0 && TP[g].used) light_add(CX[g] - 36, (NY0 + NY1) * 0.5f, 40, 0.8f * k, 0); }
    ui_wall_tablet(W / 2, 6, 208, "RISE FROM WHICH GRAVE?");
    if (!foot_word) ui_keys_worn(184, GKEYS, SAYS3, 3);
    for (int y = 0; y < H; y++) for (int x = 0; x < W; x++) if (!(MF[y][x] & MF_DIRECT)) { float l = LC[y][x] + LW[y][x]; LC[y][x] = fmaxf(l * 0.5f, 0.24f); LW[y][x] = l * 0.5f; }
    ui_light(160, H, 0.6f);
    resolve();
    if (foot_word) { int w = (int)strlen(foot_word) * 8; ui_band(&F, W / 2 - w / 2 - 6, 182, w + 12, 15); ui_text8(&F, W / 2 - w / 2, 186, foot_word, FACE_INSC, R_BLOOD + 13, TX_INLAID); }
    else ui_keys_words(184, GKEYS, SAYS3, 3);
}

/* ======================================================================================================================
 * C, THE RISEN: one stone close, the name cut in it and bleeding; candles at its foot; out of the grave before it a hand
 * claws at the air, earth falling off it
 * ====================================================================================================================== */
/* a limb's piece: a capsule from a to b, r0 to r1 thick, shaded round its length, lit from above left; nothing under
 * the ground line gy */
static void capsule(float ax, float ay, float bx, float by, float r0, float r1, int ramp, float v, int gy, uint32_t seed) {
    float dx = bx - ax, dy = by - ay, l2 = dx * dx + dy * dy + 1e-6f;
    int x0 = (int)floorf(fminf(ax, bx) - fmaxf(r0, r1) - 1), x1 = (int)ceilf(fmaxf(ax, bx) + fmaxf(r0, r1) + 1);
    int y0 = (int)floorf(fminf(ay, by) - fmaxf(r0, r1) - 1), y1 = (int)ceilf(fmaxf(ay, by) + fmaxf(r0, r1) + 1);
    float l = sqrtf(l2), nx = -dy / l, ny = dx / l;
    if (nx * -0.6f + ny * -0.8f < 0) { nx = -nx; ny = -ny; }        /* the normal that faces the light */
    for (int y = y0; y <= y1; y++) for (int x = x0; x <= x1; x++) {
        if (y >= gy) continue;
        float px = x + 0.5f - ax, py = y + 0.5f - ay;
        float u = clampf((px * dx + py * dy) / l2, 0, 1);
        float r = r0 + (r1 - r0) * u;
        float qx = px - dx * u, qy = py - dy * u, d = sqrtf(qx * qx + qy * qy);
        if (d > r) continue;
        float side = (qx * nx + qy * ny) / (r + 1e-6f);                 /* +1 the lit edge, -1 the dark */
        float vv = v * (0.72f + 0.38f * side) + 0.05f * (h01(x, y, seed) - 0.5f);
        if (d > r - 0.8f && side < -0.2f) vv *= 0.55f;                  /* its outline in shadow */
        int rot = fbm(x * 0.35f, y * 0.35f, seed) > 0.64f;
        matf(x, y, rot ? R_MOSS : ramp, rot ? vv * 0.9f : vv, 0);
    }
}
/* the hand: out of the earth at (gx, gy), its wrist rise above it; fingers curled by curl (0 open, 1 a fist) */
static float hand_tips[5][2], hand_lift = 1.0f;
static void hand(float gx, int gy, float sc, float rise, float curl, float t, uint32_t seed) {
    if (rise <= 0) return;
    float lean = -0.18f + 0.06f * sinf(t * 1.3f);
    float wx = gx + sinf(lean) * 40 * sc * rise, wy = gy - cosf(lean) * 40 * sc * rise;
    capsule(gx + 2 * sc, gy + 6, wx, wy, 6.0f * sc, 5.2f * sc, R_WARM, 0.36f * hand_lift, gy, seed);   /* the forearm, rags at the wrist */
    {                                                                   /* a cuff of shroud at the wrist, torn, its rags hanging */
        float cx0 = gx + 2 * sc + (wx - gx - 2 * sc) * 0.62f, cy0 = gy + 6 + (wy - gy - 6) * 0.62f;
        for (int i = -7; i <= 7; i++) for (int j = -2; j <= 2; j++) {
            int X = (int)(cx0 + i * sc), Y = (int)(cy0 + j * sc + i * 0.2f);
            if (Y < gy && abs(i) <= 6) matf(X, Y, R_PURPLE, (0.14f + 0.06f * (j < 0) + 0.04f * (i < 0)) * hand_lift, 0);
        }
        for (int i = 0; i < 5; i++) {
            float x0 = cx0 + (i - 2) * 2.4f * sc, ln = (4 + h01(i, 0, seed) * 7) * sc, sw = sinf(t * 3 + i) * sc;
            mline(x0, cy0 + 2 * sc, x0 + sw, cy0 + 2 * sc + ln, 1.2f * sc, R_PURPLE, 0.12f * hand_lift, 0);
        }
    }
    float ux = sinf(lean), uy = -cosf(lean), rx = cosf(lean), ry = sinf(lean);   /* up the hand, and across it */
    float kx = wx + ux * 13 * sc, ky = wy + uy * 13 * sc;               /* the knuckles */
    /* the palm: a wedge from the wrist out to the knuckles */
    for (int k = 0; k <= 6; k++) {
        float u = k / 6.0f;
        float cx = wx + ux * 13 * sc * u, cy = wy + uy * 13 * sc * u, half = (4.6f + 2.4f * u) * sc;
        capsule(cx - rx * half * 0.6f, cy - ry * half * 0.6f, cx + rx * half * 0.6f, cy + ry * half * 0.6f, 2.4f * sc, 2.4f * sc, R_WARM, (0.4f + 0.04f * u) * hand_lift, gy, seed + 1);
    }
    static const float BASE[4] = { -5.2f, -1.8f, 1.6f, 4.8f }, LEN[4] = { 0.92f, 1.06f, 1.0f, 0.78f }, SPREAD[4] = { -0.34f, -0.11f, 0.1f, 0.3f };
    for (int f = 0; f < 4; f++) {
        float x = kx + rx * BASE[f] * sc, y = ky + ry * BASE[f] * sc;
        float a = lean + SPREAD[f] * (1 - 0.5f * curl), segs[3] = { 8, 6, 4.5f };
        float c = curl * (0.85f + 0.3f * h01(f, 3, seed)) + 0.08f * sinf(t * 7 + f * 1.7f);
        for (int j = 0; j < 3; j++) {
            a += c * (0.55f + 0.25f * j);                               /* the claw bends forward, to the right */
            float l = segs[j] * LEN[f] * sc;
            float nx = x + sinf(a) * l, ny = y - cosf(a) * l;
            float r = (2.0f - 0.35f * j) * sc;
            capsule(x, y, nx, ny, r, r * 0.88f, j == 2 ? R_BONE : R_WARM, (j == 2 ? 0.5f : 0.42f) * hand_lift, gy, seed + 3 + f);
            x = nx; y = ny;
        }
        matf((int)x, (int)y, R_BONE, 0.75f, 0);                         /* the nail gone, the bone's tip */
        { float bx0 = kx + rx * BASE[f] * sc, by0 = ky + ry * BASE[f] * sc; if (by0 < gy) { matf((int)bx0, (int)by0, R_BONE, 0.62f * hand_lift, 0); matf((int)bx0 + 1, (int)by0, R_BONE, 0.5f * hand_lift, 0); } }   /* its knuckle through the skin */
        hand_tips[f][0] = x; hand_tips[f][1] = y;
    }
    float tx = wx + ux * 5 * sc + rx * 6 * sc, ty = wy + uy * 5 * sc + ry * 6 * sc, ta = lean + 0.9f - 0.5f * curl;   /* the thumb */
    for (int j = 0; j < 2; j++) {
        float l = (j ? 5.5f : 7) * sc, nx = tx + sinf(ta) * l, ny = ty - cosf(ta) * l;
        capsule(tx, ty, nx, ny, 2.3f * sc, 2.0f * sc, j ? R_BONE : R_WARM, j ? 0.5f : 0.42f, gy, seed + 9);
        tx = nx; ty = ny; ta -= 0.5f * curl + 0.2f;
    }
    hand_tips[4][0] = tx; hand_tips[4][1] = ty;
}
/* earth off the hand, falling back: little clods dropping from its fingers and wrist, a whole number of times a loop */
static void clods(float gx, int gy, float t, float sc, int n) {
    for (int i = 0; i < n; i++) {
        float u = frac(t / LOOP * (2 + i % 3) + h01(i, 1, 5));
        int f = i % 5;
        float sx = hand_tips[f][0] + (h01(i, 2, 5) - 0.5f) * 6 * sc, sy = hand_tips[f][1] + 4 * sc;
        if (i % 4 == 3) { sx = gx + (h01(i, 3, 5) - 0.5f) * 12 * sc; sy = gy - 30 * sc; }
        float y = sy + 0.5f * 200 * (u * 0.6f) * (u * 0.6f) * 3;
        if (y >= gy) continue;
        matf((int)sx, (int)y, R_FLOOR, 0.4f, MF_FLAT);
        if (i % 2) matf((int)sx + 1, (int)y, R_FLOOR, 0.3f, MF_FLAT);
    }
}
static void leaf(float x, float y, float ph, int col) {               /* an autumn leaf turning over as it goes */
    int w = (int)(1.5f + 1.5f * fabsf(sinf(ph)));
    for (int i = -w; i <= w; i++) matf((int)x + i, (int)y + (i * (sinf(ph * 0.5f) > 0 ? 1 : -1)) / 2, col, 0.55f + 0.1f * (i == 0), MF_FLAT);
}
/* the great stone: a round head on a moulded slab, a winged death's head in it, volutes at its shoulders, a plinth with
 * MEMENTO MORI cut in it; its left shoulder broken off and cracks from the break; ivy up that side. 214 by 150 */
static const char *const WING[9] = { "........#####", ".....########", "...###########", ".############", "#############",
                                     "..###########", "....#########", ".......######", ".........####" };
static int C_FACE;                                                      /* 0 the name stacked; 1 the menu's face */
static void build_great_stone(void) {
    sc_begin(214, 150);
    sc_rect(0, 138, 214, 12, 1, 0, SM_STONE, 0);
    sc_rect(8, 127, 198, 11, 2, 0, SM_STONE, 0.03f);
    sc_rect(14, 46, 186, 81, 3, 0, SM_STONE, 0.04f);
    sc_arch(14, 200, 47, 36, 0, 3, 0, SM_STONE, 0.04f);
    sc_groove(1u << 3, 4); sc_groove(1u << 3, 7);
    for (int e = 0; e < 2; e++) {                                        /* the volutes at its shoulders */
        float vx = e ? 199.5f : 14.5f;
        sc_disc(vx, 50, 7, 4, 1, SM_STONE, 0.06f);
        sc_ring(vx, 50, 2.5f, 4.2f, 4, 0, SM_STONE, 0.0f);
    }
    for (int j = 0; j < 9; j++) for (int i = 0; WING[j][i]; i++) if (WING[j][i] == '#')   /* the wings, feathered */
        for (int b = 0; b < 4; b++) for (int e = 0; e < 2; e++) {
            int x = e ? 118 + i * 2 + (b & 1) : 96 - i * 2 - (b & 1), y = 17 + j * 2 + (b >> 1);
            if (sc_solid(x, y)) { S.ht[y][x] = 1; S.dv[y][x] = 0.16f; S.f[y][x] |= SF_CALM; if ((i + j / 2) % 3 == 0 && j > 1 && i > 2) { S.ht[y][x] = 0; S.dv[y][x] = -0.08f; } }
        }
    for (int y = 12; y < 42; y++) for (int x = 94; x < 120; x++) if (sc_solid(x, y) && (x - 107) * (x - 107) / 169.0f + (y - 26) * (y - 26) / 225.0f < 1) S.ht[y][x] = -1;   /* the skull's sunk field */
    sc_mark(107 - 11, 15, KEY_SKULL, 11, 2, 1, 0.1f);
    for (int y = 12; y < 42; y++) for (int x = 94; x < 120; x++) if (sc_solid(x, y) && S.ht[y][x] == 1 && x > 95 && x < 119) S.dv[y][x] += 0.14f;
    for (int y = 46; y < 124; y++) for (int x = 20; x < 194; x++) if (sc_solid(x, y) && S.ht[y][x] == 0) S.f[y][x] |= SF_CALM;   /* quiet where the name is cut */
    if (!C_FACE) sc_text_c(107, 129, "MEMENTO MORI", 0, 0, 77, 190);
    else { menu_rows(107, 80, 11, 170, -1, 940); sc_text_c(107, 129, menu_note(menu_sel()), 0, 0, 941, 190); }
    sc_break(10, 46, 13, 81);                                            /* the left shoulder, gone */
    sc_crack(22, 52, 0.5f, 60, 82, 2);
    sc_crack(30, 40, -0.4f, 30, 83, 1);
    sc_crack(190, 127, -2.0f, 30, 84, 1);
    sc_crack(150, 138, -1.4f, 12, 85, 0);
    sc_chips(0.3f, 86);
    sc_ivy(6, 149, 90, 87); sc_ivy(12, 149, 60, 88); sc_ivy(200, 149, 30, 89);
}
static void grass_fg(float t) {                                         /* dark blades in the foreground's corners, stirring */
    for (int k = 0; k < 60; k++) {
        int side = k % 2;
        float x = side ? W - 2 - h01(k, 0, 92) * 90 : 2 + h01(k, 0, 92) * 90;
        float hgt = 10 + h01(k, 1, 92) * 22 * (1 - fabsf((side ? W - x : x)) / 110.0f), sw = sinf(2 * PI * t / LOOP * 2 + k * 0.7f) * 2 + (side ? -2 : 2);
        if (hgt < 6) continue;
        mline(x, (float)H, x + sw * 0.4f, H - hgt * 0.6f, 1.6f, R_MOSS, 0.03f, MF_FLAT | MF_NOWARM);
        mline(x + sw * 0.4f, H - hgt * 0.6f, x + sw, H - hgt, 1, R_MOSS, 0.04f, MF_FLAT | MF_NOWARM);
    }
}
static void scene_c(float t) {
    float flash = flash_k(t, LOOP * 0.62f);
    const int G = 160;
    pal_night();
    pal_flash(flash * 0.5f);
    light_clear(0.62f + 0.4f * flash, 0);
    sky(0, G, t, 46, 34, 16, flash);
    for (int x = 0; x < W; x++) {                                      /* the far graveyard, flat against the hills */
        float hy = 128 + 5 * sinf(x * 0.03f + 2) + 2 * fbm(x * 0.06f, 0, 18);
        for (int y = (int)hy; y < G; y++) matf(x, y, R_CEIL, 0.1f, MF_NOWARM);
    }
    for (int i = 0; i < 16; i++) {
        int x = 8 + i * 20 + (int)(h01(i, 1, 8) * 8), h = 5 + (int)(h01(i, 2, 8) * 7), sh = (int)(h01(i, 3, 8) * 3);
        if (x > 50 && x < 270) continue;
        for (int y = G - 26 - h; y < G - 26; y++) for (int dx = -2; dx <= 2; dx++) {
            int in = sh == 1 ? (abs(dx) <= 0 || (y == G - 26 - h + 2 && abs(dx) <= 2)) : abs(dx) <= 2 - (y < G - 26 - h + 1 && abs(dx) == 2);
            if (in) matf(x + dx, y, R_CEIL, 0.04f, MF_NOWARM);
        }
    }
    for (int x = 272; x < 318; x++) for (int y = 108; y < G - 4; y++) {   /* a mausoleum, flat against the hills */
        int ped = y < 118 && y >= 118 - (23 - abs(x - 295)) / 2;
        if ((y >= 118 || ped) && !(y >= 128 && x > 290 && x < 300)) matf(x, y, R_CEIL, 0.05f + 0.02f * (y == 118), MF_NOWARM);
    }
    for (int x = 0; x < 64; x += 3) { mrect(x, G - 22, 1, 14, R_CEIL, 0.04f, MF_NOWARM); matf(x, G - 23, R_CEIL, 0.04f, MF_NOWARM); }   /* the railings */
    mrect(0, G - 19, 64, 1, R_CEIL, 0.04f, MF_NOWARM);
    branch(24, G, 1.45f, 34, 7, 6, 31);
    branch(24, G - 24, 0.6f, 18, 3.5f, 4, 37);
    for (int y = G - 6; y < H; y++) for (int x = 0; x < W; x++) {      /* the ground */
        float n = fbm(x * 0.04f, y * 0.08f, 51);
        float v = 0.16f + 0.1f * n + 0.04f * (y - G) / 40.0f + 0.04f * (h01(x, y, 52) - 0.5f) + 0.03f * (((x + y / 3) % 4) == 0);
        if (y < G) { if (h01(x, 0, 3) * 6 < G - y) continue; matf(x, y, R_MOSS, 0.12f + 0.04f * (G - y), 0); continue; }
        matf(x, y, n > 0.68f ? R_FLOOR : R_MOSS, n > 0.68f ? v + 0.04f : v, 0);
    }
    for (int i = 0; i < 26; i++) {                                     /* fallen leaves on the ground */
        float lx = h01(i, 1, 46) * W, ly = G + 4 + h01(i, 2, 46) * 34;
        static const int LCOL[3] = { R_BRICK, R_WOOD, R_GOLD };
        mrect((int)lx, (int)ly, 2 + (i % 2), 1, LCOL[i % 3], 0.3f, 0);
    }
    const int SB = G + 2;
    build_great_stone();
    sc_shade(0.36f, 101, 0, R_GOLD);
    sc_draw(W / 2, SB, 0);
    build_fragment(18, 10, 120); sc_shade(0.36f, 121, 0, R_GOLD); sc_draw(W / 2 - 112, SB + 1, -24);
    build_fragment(10, 6, 122); sc_shade(0.36f, 123, 0, R_GOLD); sc_draw(W / 2 - 96, SB + 3, 12);
    dead_flowers(W / 2 + 70, SB, 124);
    for (int x = 200; x < 320; x++) {                                   /* the mound before it, and the hand out of it */
        int mh = (int)(9 * sqrtf(fmaxf(0, 1 - powf((x - 284) / 60.0f, 2))));
        for (int y = G + 14 - mh; y < G + 16; y++) matf(x, y, R_FLOOR, 0.3f + 0.14f * fbm(x * 0.3f, y * 0.3f, 7) + 0.1f * (y == G + 14 - mh), 0);
    }
    float u = t / LOOP;
    float curl = 0.2f + 0.62f * (0.5f - 0.5f * cosf(2 * PI * u * 2)), rise = 0.92f + 0.08f * sinf(2 * PI * u);
    const float HX = 288;
    hand(HX, G + 12, 1.05f, rise, curl, t, 77);
    clods(HX, G + 12, t, 1.05f, 14);
    for (int i = 0; i < 9; i++) {                                       /* the earth cracked open round it */
        float a = i / 9.0f * PI;
        mline(HX - cosf(a) * 7, G + 12.0f - sinf(a) * 2, HX - cosf(a) * (14 + 6 * h01(i, 1, 2)), G + 12.0f - sinf(a) * 3, 1, R_FLOOR, 0.08f, 0);
    }
    static const int CA[6] = { 62, 70, 78, 242, 250, 258 }, CH[6] = { 0, 3, 1, 2, 0, 3 };   /* candles at the stone's foot */
    for (int i = 0; i < 6; i++) {
        mrect(CA[i] - 1, G + 4 - 9 - CH[i], 3, 9 + CH[i], R_BONE, 0.6f, 0); matf(CA[i] - 1, G + 4 - 9 - CH[i], R_BONE, 0.75f, 0);
        matf(CA[i] + 1, G + 4 - 6 - CH[i], R_BONE, 0.7f, 0); matf(CA[i] + 1, G + 4 - 5 - CH[i], R_BONE, 0.7f, 0);   /* wax run down */
    }
    for (int y = 0; y < H; y++) for (int x = 0; x < W; x++) if (y > G) LC[y][x] *= 0.75f;
    for (int i = 0; i < 6; i += 2) { float fl = flick(t, i); light_add(CA[i] + 8, G - 8, 110 * fl, 0.75f, 1); }
    for (int i = 3; i < 6; i += 2) { float fl = flick(t, i); light_add(CA[i] - 8, G - 8, 110 * fl, 0.75f, 1); }
    for (int i = 0; i < 7; i++) {                                       /* the leaves, the bats */
        float lu = frac(t / LOOP * (1 + i % 2) + h01(i, 1, 44));
        float lx = W + 8 - lu * (W + 16), ly = 20 + h01(i, 2, 44) * 120 + lu * 30 + 6 * sinf(lu * 12 + i);
        static const int LC_[3] = { R_BRICK, R_WOOD, R_GOLD };
        leaf(lx, ly, lu * 30 + i, LC_[i % 3]);
    }
    { float bu = frac(t / LOOP + 0.2f); bat(W + 10 - bu * (W + 30), 24 + 10 * sinf(bu * 8), t * 26); }
    fog(G - 16, G + 6, t, 1.2f, 35);
    grass_fg(t);
    vignette(0.35f);
    for (int i = 0; i < 6; i++) flame(CA[i], G + 4 - 10 - CH[i], 6, t, i);
    if (!C_FACE) { logo_build("HAUNTED", 0.8f, ".EXE", 0.76f, 0, 31); logo_draw((W - LG.w) / 2, SB - 150 + 50, LS_CARVED, 1.0f + 0.2f * flash, G + 1, t); }
    else {                                                              /* the name on one line over the four */
        lg_dripmax = 5;
        logo_build("HAUNTED", 0.56f, ".EXE", 0.5f, 1, 31);
        lg_dripmax = 99;
        logo_draw((W - LG.w) / 2, SB - 150 + 50, LS_CARVED, 1.0f + 0.2f * flash, G + 1, t);
        for (int y = SB - 150 + 46; y < SB - 26; y++) for (int x = 60; x < 260; x++) LC[y][x] = fmaxf(LC[y][x], 0.62f * MN.appear);
        light_add(W / 2, SB - 150 + 80 + menu_sel() * 11 + 3, 60, 0.45f, 1);
        if (MN.appear < 1) for (int i = 0; i < 40; i++) {                /* dust off the face as it is cut again */
            float x = 70 + h01(i, 1, 950) * 180, y = SB - 140 + h01(i, 2, 950) * 90 + MN.appear * 40 * (1 + h01(i, 3, 950));
            matf((int)x, (int)y, R_STONE, 0.7f, MF_FLAT);
        }
    }
    title_foot_pre();
    resolve();
    title_foot(186, 1, t);
}

/* the graves in C: three stones close, a mound before each; out of the chosen one's a hand comes up, its candles lit; the
 * empty plot an open pit, a spade in its earth */
static void scene_c_graves(float t) {
    const int G = 124, M = 152;                                         /* the stones' foot; the kerbs' */
    pal_night();
    light_clear(0.55f, 0);
    sky(0, G, t, 30, 50, 9, 0);
    for (int x = 0; x < W; x++) {
        float hy = 100 + 5 * sinf(x * 0.03f + 2) + 2 * fbm(x * 0.06f, 0, 18);
        for (int y = (int)hy; y < G; y++) matf(x, y, R_CEIL, 0.1f, MF_NOWARM);
    }
    for (int y = G - 6; y < H; y++) for (int x = 0; x < W; x++) {      /* the turf, in strokes of grass */
        float n = fbm(x * 0.04f, y * 0.08f, 51);
        float v = 0.15f + 0.08f * n + 0.06f * (h01(x, y / 2, 52) - 0.5f) + 0.05f * (((x + y / 3) % 4) == 0);
        if (y < G) { if (h01(x, 0, 3) * 6 < G - y) continue; matf(x, y, R_MOSS, 0.12f + 0.04f * (G - y), 0); continue; }
        matf(x, y, n > 0.7f ? R_FLOOR : R_MOSS, n > 0.7f ? v + 0.04f : v, 0);
    }
    branch(8, G - 2, 1.35f, 30, 6, 6, 31);                              /* the tree, behind the stones */
    static const int CX[3] = { 57, 160, 263 };
    for (int g = 0; g < 3; g++) {
        float k = GK[g];
        grave_drawn(g, CX[g], G, k, t);
        if (TP[g].used) {
            for (int x = CX[g] - 44; x <= CX[g] + 44; x++) {             /* the grave's earth, heaped between stone and kerb */
                float m = fmaxf(0, 1 - powf((x - CX[g]) / 44.0f, 2));
                int top = G + 4 + (int)(14 * (1 - sqrtf(m)));
                for (int y = top; y < M - 4; y++) {
                    float v = 0.24f + 0.12f * fbm(x * 0.3f, y * 0.3f, 7 + g) + 0.1f * (y == top) - 0.06f * (h01(x, y, 8) < 0.15f);
                    matf(x, y, h01(x, y, 9 + g) < 0.08f && y > top + 2 ? R_STONE : R_FLOOR, v, 0);
                }
                if (h01(x, 0, 10 + g) < 0.4f) for (int jj = 1; jj < 3; jj++) matf(x, top - jj, R_MOSS, 0.2f, 0);
            }
            build_kerb(96, 600 + (uint32_t)g); sc_shade(0.42f, 610 + (uint32_t)g, 0, R_GOLD); sc_draw(CX[g], M + 6, g ? -1.0f : 1.2f);
            float rise = smooth(0.1f, 0.9f, k);
            if (rise > 0) {                                             /* out of the chosen one's earth, a hand */
                float curl = 0.15f + 0.5f * (0.5f - 0.5f * cosf(2 * PI * t * 0.8f));
                hand_lift = 1.3f;
                hand(CX[g] + 28, M - 3, 0.86f, rise * 0.6f, curl, t, 70 + (uint32_t)g);
                hand_lift = 1.0f;
                clods(CX[g] + 28, M - 3, t, 0.72f, rise > 0.6f ? 10 : 0);
            }
        } else {                                                        /* the open pit, its earth thrown up, the spade */
            for (int x = CX[g] - 46; x < CX[g] + 46; x++) {
                int mh = (int)(10 * sqrtf(fmaxf(0, 1 - powf((x - CX[g]) / 46.0f, 2))));
                for (int y = G + 6 - mh; y < G + 8; y++) matf(x, y, R_FLOOR, 0.3f + 0.14f * fbm(x * 0.25f, y * 0.25f, 19) + 0.08f * (y == G + 6 - mh), 0);
            }
            for (int y = G + 8; y < M + 4; y++) for (int x = CX[g] - 36; x < CX[g] + 36; x++) {
                float e = fminf(fminf((float)(x - CX[g] + 36), (float)(CX[g] + 35 - x)), (float)(M + 3 - y));
                matf(x, y, R_FLOOR, y == G + 8 ? 0.45f : 0.02f + 0.03f * fminf(e, 3) + 0.04f * (h01(x, y, 4) < 0.1f), 0);
            }
            for (int y = G + 9; y < M + 3; y += 5) for (int x = CX[g] - 34; x < CX[g] + 34; x++) if (h01(x, y, 5) < 0.4f) matf(x, y, R_FLOOR, 0.1f, 0);   /* spade marks in its sides */
            mline(CX[g] + 44.0f, G + 6.0f, CX[g] + 52.0f, G - 30.0f, 2, R_WOOD, 0.4f, 0);
            mrect(CX[g] + 48, G - 34, 8, 2, R_WOOD, 0.4f, 0);
            for (int y = G + 2; y < G + 10; y++) for (int x = CX[g] + 40; x < CX[g] + 48; x++) if (y >= G + 6 + (x - CX[g] - 40) / 3 - 4) matf(x, y, R_IRON, 0.45f + 0.1f * (x == CX[g] + 41), 0);
        }
    }
    for (int g = 0; g < 3; g++) mist(CX[g], M - 4, GK[g], t, 300 + g);
    ui_beam("RISE FROM WHICH GRAVE?", t);
    if (!foot_word) ui_keys_worn(184, GKEYS, SAYS3, 3);
    for (int y = G; y < H; y++) for (int x = 0; x < W; x++) LC[y][x] = y < 160 ? 0.5f : 0.62f;
    for (int g = 0; g < 3; g++) {
        float k = GK[g];
        if (k <= 0) continue;
        float fl = flick(t, g);
        light_add(CX[g], G - 12, 80 * fl, 1.0f * k, 1);
        light_add(CX[g], G - 50, 60, 0.3f * k, 0);
    }
    vignette(0.45f);
    ui_light(160, H, 0.6f);
    resolve();
    if (foot_word) { int w = (int)strlen(foot_word) * 8; ui_band(&F, W / 2 - w / 2 - 6, 182, w + 12, 15); ui_text8(&F, W / 2 - w / 2, 186, foot_word, FACE_INSC, R_BLOOD + 13, TX_INLAID); }
    else ui_keys_words(184, GKEYS, SAYS3, 3);
}


/* ---- what the game calls ---- */
float title_loop(int scene) { static const float L[3] = { 12.0f, 8.0f, 8.0f }; return L[clampi(scene, 0, 2)]; }
void title_draw(Frame *f, float t, const TitleView *v) {
    static uint8_t was[H * W];
    TF = f; f->w = W; f->h = H; f->fade = 1.0f;
    memcpy(BASEPAL, crawl_pal, sizeof BASEPAL);
    ui_ramps(0);
    PADMODE = v->pad;
    MN.on = 1; MN.appear = clampf(v->menu, 0, 1); MN.press = clampf(v->press, 0, 1);
    for (int k = 0; k < 4; k++) MN.k[k] = clampf(v->mk[k], 0, 1);
    for (int g = 0; g < 3; g++) { GK[g] = clampf(v->gk[g], 0, 1); TP[g] = v->page[g]; }
    foot_word = v->foot; ver_word = v->version;
    int sc = clampi(v->scene, 0, 2);
    LOOP = title_loop(sc);
    float tm = fmodf(t < 0 ? 0 : t, LOOP);
    if (v->graves) { if (sc == 0) a_graves(tm); else if (sc == 1) scene_b_graves(tm); else scene_c_graves(tm); return; }
    if (sc == 0) scene_a(tm);
    else if (sc == 1) scene_b(tm);
    else if (MN.appear >= 0.999f) { C_FACE = 1; scene_c(tm); C_FACE = 0; }
    else if (MN.appear <= 0.001f) { C_FACE = 0; scene_c(tm); }
    else {                                                              /* the old face dissolving into the new, in a dither */
        C_FACE = 0; scene_c(tm); memcpy(was, F.pix, sizeof was);
        C_FACE = 1; scene_c(tm); C_FACE = 0;
        for (int y = 0; y < H; y++) for (int x = 0; x < W; x++) if (bay(x, y) * 0.98f + 0.01f > MN.appear) F.pix[y * W + x] = was[y * W + x];
    }
}
/* for the mock-ups' sheets: 0 the name in blood, 1 cut in stone (each on one line and stacked), 2 the built pieces */
void title_debug_frame(Frame *f, int which, float t) {
    TF = f; f->w = W; f->h = H;
    memcpy(BASEPAL, crawl_pal, sizeof BASEPAL);
    ui_ramps(0);
    LOOP = 6;
    pal_night();
    light_clear(1, 0);
    if (which < 2) {
        for (int y = 0; y < H; y++) for (int x = 0; x < W; x++) matf(x, y, which ? R_STONE : R_CEIL, which ? 0.35f + 0.15f * fbm(x * 0.1f, y * 0.1f, 3) : 0.15f, 0);
        logo_build("HAUNTED", 0.95f, ".EXE", 0.84f, 1, 11);
        logo_draw((W - LG.w) / 2, 20, which ? LS_CARVED : LS_BLOOD, 1, which ? 0 : 150, t);
        logo_build("HAUNTED", 1.3f, ".EXE", 0.95f, 0, 12);
        logo_draw((W - LG.w) / 2, 80, which ? LS_CARVED : LS_BLOOD, 1, which ? 0 : 196, t);
        resolve();
        return;
    }
    for (int y = 0; y < H; y++) for (int x = 0; x < W; x++) matf(x, y, R_CEIL, 0.12f, 0);
    build_stele(page_mark(0), TP[0].num, page_lines(0), 4, 100); sc_shade(0.44f, 200, 0, R_GOLD); sc_draw(56, 104, 0);
    build_wheelcross(page_mark(1), TP[1].num, page_lines(1), 4, 101); sc_shade(0.44f, 201, 1, R_GOLD); sc_draw(164, 104, 0);
    build_timber_cross(400); sc_shade(0.4f, 401, 0, R_GOLD); sc_draw(270, 104, 0);
    build_plank(410, "III"); sc_shade(0.42f, 411, 0, R_BONE); sc_draw(270, 150, 0);
    build_kerb(100, 5); sc_shade(0.44f, 6, 0, R_GOLD); sc_draw(56, 128, 0);
    build_fragment(17, 9, 300); sc_shade(0.42f, 301, 0, R_GOLD); sc_draw(130, 128, 0);
    MN.k[0] = 0; row_slab(8, 140, 148, "UNSIGNED: LEVEL 3", 0, 500);
    row_slab(8, 160, 148, "DIG A NEW GRAVE", 1, 501);
    ui_keys_worn(182, KEYS3, SAYS3, 3);
    resolve();
    ui_keys_words(182, KEYS3, SAYS3, 3);
}
/* the page's words for the view, from what the ledger knows */
void title_page_set(TitlePage *p, const char *num, int used, int cls, const char *who, const char *l1, const char *l2, const char *l3) {
    memset(p, 0, sizeof *p);
    snprintf(p->num, sizeof p->num, "%s", num);
    p->used = used; p->cls = cls;
    snprintf(p->line[0], sizeof p->line[0], "%s", who ? who : "");
    snprintf(p->line[1], sizeof p->line[1], "%s", l1 ? l1 : "");
    snprintf(p->line[2], sizeof p->line[2], "%s", l2 ? l2 : "");
    snprintf(p->line[3], sizeof p->line[3], "%s", l3 ? l3 : "");
}
