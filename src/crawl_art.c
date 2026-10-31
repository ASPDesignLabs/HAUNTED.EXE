/* Procedural art for the CATACOMB raycaster: palette, light colormap,
 * 64x64 wall textures, sprites and the caster's hand. */
#include "crawl.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

uint8_t crawl_pal[256][3];
uint8_t crawl_cmap[NLIGHT][256];
uint8_t crawl_tex[NTEX][TS * TS];
uint8_t crawl_spr[NSPR][TS * TS];
int8_t crawl_glint[NSPR][2];

/* ------------------------------------------------------------------ */
/* Palette and colormap                                                */
/* ------------------------------------------------------------------ */

static void ramp3(int base, const int a[3], const int m[3], const int b[3]) {
    for (int i = 0; i < 16; i++) {
        float t = i / 15.0f, u;
        const int *p, *q;
        if (t < 0.5f) { p = a; q = m; u = t * 2.0f; } else { p = m; q = b; u = (t - 0.5f) * 2.0f; }
        for (int c = 0; c < 3; c++)
            crawl_pal[base + i][c] = (uint8_t)clampi((int)(p[c] + (q[c] - p[c]) * u + 0.5f), 0, 63);
    }
}

#define RAMP(base, r0, g0, b0, r1, g1, b1, r2, g2, b2) do { \
    static const int A_[3] = { r0, g0, b0 }, M_[3] = { r1, g1, b1 }, B_[3] = { r2, g2, b2 }; \
    ramp3(base, A_, M_, B_); } while (0)

static void build_palette(void) {
    memset(crawl_pal, 0, sizeof crawl_pal);
    RAMP(R_STONE,  2, 2, 4,  20, 21, 26,  46, 47, 52);
    RAMP(R_WARM,   3, 2, 2,  22, 18, 15,  48, 42, 36);
    RAMP(R_BONE,   4, 3, 2,  34, 30, 22,  62, 58, 48);
    RAMP(R_BRICK,  3, 1, 1,  28, 12,  9,  52, 30, 24);
    RAMP(R_MOSS,   1, 3, 1,  12, 24, 10,  34, 48, 24);
    RAMP(R_WOOD,   3, 2, 1,  24, 14,  7,  46, 32, 18);
    RAMP(R_IRON,   2, 2, 3,  15, 14, 16,  42, 40, 44);
    RAMP(R_SKIN,   6, 3, 2,  40, 26, 18,  62, 48, 38);
    RAMP(R_GHOST,  3, 7, 9,  24, 42, 46,  54, 63, 63);
    RAMP(R_PURPLE, 4, 1, 7,  26,  9, 38,  50, 32, 62);
    RAMP(R_BLOOD,  6, 0, 0,  36,  4,  4,  60, 18, 12);
    RAMP(R_FLOOR,  2, 2, 2,  15, 13, 11,  34, 30, 26);
    RAMP(R_CEIL,   1, 1, 2,  10, 10, 13,  26, 26, 32);
    RAMP(R_FLAME, 26, 3, 0,  63, 26,  0,  63, 63, 44);
    RAMP(R_GLOW,   2, 8, 22, 14, 38, 63,  54, 63, 63);
    crawl_pal[C_WHITE][0] = crawl_pal[C_WHITE][1] = crawl_pal[C_WHITE][2] = 63;
    static const int GA[3] = { 5, 3, 0 }, GM[3] = { 40, 25, 3 }, GB[3] = { 63, 59, 32 };
    for (int i = 0; i < NGOLD; i++) {                                  /* gold, 13 steps */
        float t = i / (float)(NGOLD - 1), u = t < 0.5f ? t * 2.0f : (t - 0.5f) * 2.0f;
        const int *p = t < 0.5f ? GA : GM, *q = t < 0.5f ? GM : GB;
        for (int c = 0; c < 3; c++) crawl_pal[R_GOLD + i][c] = (uint8_t)clampi((int)(p[c] + (q[c] - p[c]) * u + 0.5f), 0, 63);
    }
}

/* Darkening stays inside a colour's own ramp, so shadows keep their hue. */
static void build_colormap(void) {
    for (int L = 0; L < NLIGHT; L++) {
        float f = L / LIGHT_UNIT;
        for (int c = 0; c < 256; c++) {
            if (c == 0 || c == C_CLEAR || IS_FULLBRIGHT(c)) { crawl_cmap[L][c] = (uint8_t)c; continue; }
            int base = c >= R_GOLD ? R_GOLD : ((c - 1) / 16) * 16 + 1, n = c >= R_GOLD ? NGOLD : 16;
            float tr = crawl_pal[c][0] * f, tg = crawl_pal[c][1] * f, tb = crawl_pal[c][2] * f;
            int best = 0;
            float bd = tr * tr * 3 + tg * tg * 4 + tb * tb * 2;
            for (int i = base; i < base + n; i++) {
                float dr = crawl_pal[i][0] - tr, dg = crawl_pal[i][1] - tg, db = crawl_pal[i][2] - tb;
                float d = dr * dr * 3 + dg * dg * 4 + db * db * 2;
                if (d < bd) { bd = d; best = i; }
            }
            crawl_cmap[L][c] = (uint8_t)best;
        }
    }
}

/* ------------------------------------------------------------------ */
/* Noise                                                               */
/* ------------------------------------------------------------------ */

static uint32_t hsh(int x, int y, uint32_t s) {
    uint32_t h = (uint32_t)x * 374761393u + (uint32_t)y * 668265263u + s * 2246822519u;
    h = (h ^ (h >> 13)) * 1274126177u;
    return h ^ (h >> 16);
}
static float h01(int x, int y, uint32_t s) { return (float)(hsh(x, y, s) & 0xFFFF) / 65535.0f; }

static int wrapi(int v, int p) { v %= p; return v < 0 ? v + p : v; }

static float vnoise(float x, float y, int per, uint32_t s) {          /* tileable value noise */
    int x0 = (int)floorf(x), y0 = (int)floorf(y);
    float fx = x - x0, fy = y - y0;
    fx = fx * fx * (3 - 2 * fx); fy = fy * fy * (3 - 2 * fy);
    int ax = wrapi(x0, per), bx = wrapi(x0 + 1, per), ay = wrapi(y0, per), by = wrapi(y0 + 1, per);
    float a = h01(ax, ay, s), b = h01(bx, ay, s), c = h01(ax, by, s), d = h01(bx, by, s);
    return lerpf(lerpf(a, b, fx), lerpf(c, d, fx), fy);
}

static float fbm(float x, float y, int per, uint32_t s) {
    float v = 0, amp = 0.5f;
    for (int o = 0; o < 4; o++) { v += amp * vnoise(x, y, per, s + (uint32_t)o * 77u); x *= 2; y *= 2; per *= 2; amp *= 0.5f; }
    return v;
}

static uint8_t sh(int ramp, float v) {
    if (ramp == R_GOLD) return (uint8_t)(R_GOLD + clampi((int)(v * (NGOLD - 1) + 0.5f), 0, NGOLD - 1));
    return (uint8_t)(ramp + clampi((int)(v * 15.0f + 0.5f), 0, 15));
}

/* ------------------------------------------------------------------ */
/* Wall textures                                                       */
/* ------------------------------------------------------------------ */

static void tput(int t, int x, int y, uint8_t c) { crawl_tex[t][(x & 63) * TS + (y & 63)] = c; }

/* big dressed stone blocks, two per row, four rows */
static float block_shade(int x, int y, uint32_t seed) {
    int r = y / 16, yy = y % 16;
    int off = (int)(h01(r, 0, seed) * 32);
    int xx = (x + off) & 63;
    int bi = xx / 32, lx = xx % 32, bw = 32;
    if (h01(r, bi, seed + 3) > 0.55f) { bw = 16; bi = bi * 2 + lx / 16; lx %= 16; }
    float base = 0.40f + 0.22f * h01(r * 7 + bi, 9, seed);
    float n = fbm(x / 8.0f, y / 8.0f, 8, seed) * 0.26f - 0.13f;
    if (lx == 0 || yy == 0) return 0.08f + 0.05f * h01(x, y, seed + 9);           /* mortar */
    float bev = 0.0f;
    if (yy == 1) bev += 0.14f; else if (lx == 1) bev += 0.07f;
    if (yy == 15) bev -= 0.12f; else if (lx == bw - 1) bev -= 0.06f;
    if (h01(x, y, seed + 5) > 0.975f) bev -= 0.15f;                                 /* pits */
    return base + n + bev;
}

static void gen_stone(int t, int ramp, uint32_t seed) {
    for (int y = 0; y < TS; y++)
        for (int x = 0; x < TS; x++) tput(t, x, y, sh(ramp, block_shade(x, y, seed)));
}

static void gen_rubble(int t, uint32_t seed) {
    float px[18], py[18], pb[18];
    for (int i = 0; i < 18; i++) { px[i] = h01(i, 1, seed) * 64; py[i] = h01(i, 2, seed) * 64; pb[i] = 0.32f + 0.3f * h01(i, 3, seed); }
    for (int y = 0; y < TS; y++)
        for (int x = 0; x < TS; x++) {
            float d1 = 1e9f, d2 = 1e9f; int n1 = 0;
            for (int i = 0; i < 18; i++) {
                float dx = fabsf(x + 0.5f - px[i]), dy = fabsf(y + 0.5f - py[i]);
                if (dx > 32) dx = 64 - dx;
                if (dy > 32) dy = 64 - dy;
                float d = sqrtf(dx * dx + dy * dy);
                if (d < d1) { d2 = d1; d1 = d; n1 = i; } else if (d < d2) d2 = d;
            }
            float v;
            if (d2 - d1 < 1.4f) v = 0.06f;
            else {
                float dx = x + 0.5f - px[n1], dy = y + 0.5f - py[n1];
                if (dx > 32) dx -= 64; else if (dx < -32) dx += 64;
                if (dy > 32) dy -= 64; else if (dy < -32) dy += 64;
                v = pb[n1] + 0.010f * (-dx - dy) + fbm(x / 6.0f, y / 6.0f, 8, seed + 1) * 0.2f - 0.1f;
                if (d2 - d1 < 2.6f) v -= 0.08f;
            }
            tput(t, x, y, sh(R_STONE, v));
        }
}

static void gen_brick(int t, uint32_t seed) {
    for (int y = 0; y < TS; y++)
        for (int x = 0; x < TS; x++) {
            int r = y / 8, yy = y % 8;
            int xx = (x + (r & 1) * 8) & 63, bi = xx / 16, lx = xx % 16;
            if (yy == 0 || lx == 0) { tput(t, x, y, sh(R_STONE, 0.18f + 0.06f * h01(x, y, seed))); continue; }
            float v = 0.38f + 0.2f * h01(r, bi, seed) + fbm(x / 6.0f, y / 6.0f, 8, seed) * 0.2f - 0.1f;
            if (yy == 1) v += 0.1f;
            if (yy == 7) v -= 0.1f;
            if (lx == 15) v -= 0.05f;
            tput(t, x, y, sh(R_BRICK, v));
        }
}

static void gen_moss(int t, uint32_t seed) {
    for (int y = 0; y < TS; y++)
        for (int x = 0; x < TS; x++) {
            float base = block_shade(x, y, seed);
            float m = fbm(x / 10.0f, y / 10.0f, 8, seed + 40) + 0.25f * (y / 64.0f) + (h01(x, 0, seed + 41) > 0.82f ? 0.25f * (1.0f - y / 64.0f) : 0);
            if (m > 0.62f) tput(t, x, y, sh(R_MOSS, 0.22f + 0.6f * (m - 0.62f) * 2.5f + 0.1f * (base - 0.4f)));
            else tput(t, x, y, sh(R_STONE, base * 0.9f));
        }
}

/* a 15x15 skull at (ox, oy) into texture t */
static void tex_skull(int t, int ox, int oy, float bright, int glow) {
    for (int y = 0; y < 15; y++)
        for (int x = 0; x < 15; x++) {
            float dx = (x - 7.0f) / 6.8f, dy = (y - 6.0f) / 6.2f;
            int cran = dx * dx + dy * dy <= 1.0f && y < 11;
            int jaw = y >= 10 && y <= 14 && x >= 3 && x <= 11 && (y < 13 || (x >= 4 && x <= 10));
            if (!cran && !jaw) continue;
            float v = bright - 0.45f * (dx * dx + dy * dy) - 0.05f * (dx + dy);
            if (jaw) v = bright - 0.25f - (y - 10) * 0.06f;
            tput(t, ox + x, oy + y, sh(R_BONE, v));
        }
    for (int e = 0; e < 2; e++) {
        float ex = e ? 9.6f : 4.4f;
        for (int y = 4; y < 10; y++)
            for (int x = 1; x < 14; x++) {
                float dx = (x - ex) / 2.2f, dy = (y - 7.0f) / 2.4f;
                if (dx * dx + dy * dy <= 1.0f) tput(t, ox + x, oy + y, (glow && y == 7 && fabsf(x - ex) < 0.6f) ? R_FLAME + 9 : R_BONE);
            }
    }
    tput(t, ox + 7, oy + 10, R_BONE); tput(t, ox + 6, oy + 11, R_BONE + 1); tput(t, ox + 8, oy + 11, R_BONE + 1);
    for (int x = 4; x <= 10; x += 2) { tput(t, ox + x, oy + 12, R_BONE + 1); tput(t, ox + x, oy + 13, R_BONE + 1); }
}

static void gen_skulls(int t, uint32_t seed) {
    for (int y = 0; y < TS; y++)
        for (int x = 0; x < TS; x++) tput(t, x, y, sh(R_BONE, 0.04f + 0.10f * fbm(x / 4.0f, y / 4.0f, 16, seed)));
    for (int row = 0; row < 4; row++) {
        int by = row * 16 + 15;                                   /* a femur across each gap */
        for (int x = 0; x < TS; x++) {
            float v = 0.55f + 0.1f * sinf(x * 0.4f + row);
            tput(t, x, by, sh(R_BONE, v));
            if ((x + row * 5) % 21 == 0) { tput(t, x, by - 1, sh(R_BONE, 0.6f)); tput(t, x + 1, by - 1, sh(R_BONE, 0.5f)); }
        }
        for (int col = 0; col < 4; col++) {
            float b = 0.80f + 0.18f * h01(row, col, seed) - 0.1f;
            tex_skull(t, col * 16 + (row & 1) * 8, row * 16, b, h01(col, row, seed + 2) > 0.94f);
        }
    }
}

static void gen_niche(int t, uint32_t seed) {
    gen_stone(t, R_STONE, seed);
    for (int ny = 0; ny < 2; ny++)
        for (int nx = 0; nx < 2; nx++) {
            int x0 = nx * 32 + 4, y0 = ny * 32 + 4, w = 24, hh = 24;
            float cx = x0 + w / 2.0f;
            for (int y = y0 - 1; y <= y0 + hh; y++)
                for (int x = x0 - 1; x <= x0 + w; x++) {
                    int inside;
                    if (y < y0 + 8) { float dx = (x + 0.5f - cx) / (w / 2.0f), dy = (y + 0.5f - (y0 + 8)) / 8.0f; inside = dx * dx + dy * dy <= 1.0f; }
                    else inside = x >= x0 && x < x0 + w && y < y0 + hh;
                    if (!inside) continue;
                    float depth = (y - y0) / (float)hh;
                    tput(t, x, y, sh(R_STONE, 0.02f + 0.06f * depth));
                }
            for (int x = x0; x < x0 + w; x++) tput(t, x, y0 + hh, sh(R_STONE, 0.72f));   /* sill */
            tex_skull(t, (int)cx - 7, y0 + hh - 15, 0.70f + 0.15f * h01(nx, ny, seed), 0);
        }
}

static void gen_banner(int t, uint32_t seed) {
    gen_stone(t, R_STONE, seed);
    for (int x = 10; x <= 53; x++) { tput(t, x, 3, sh(R_IRON, 0.7f)); tput(t, x, 4, sh(R_IRON, 0.35f)); }
    for (int x = 15; x <= 48; x++) {
        int bottom = 52 + (int)(4.0f * sinf(x * 0.7f) - 3.0f * fabsf(sinf(x * 1.9f)));
        for (int y = 5; y <= bottom; y++) {
            float v = 0.42f + 0.18f * sinf((x - 15) * 0.55f) - 0.003f * y + fbm(x / 5.0f, y / 5.0f, 16, seed) * 0.12f;
            uint8_t c = sh(R_BLOOD, v);
            if (x == 15 || x == 48 || y == 5 || y == 6) c = sh(R_BONE, 0.5f + 0.1f * sinf(y));
            tput(t, x, y, c);
        }
    }
    tex_skull(t, 25, 18, 0.85f, 0);
    for (int i = -7; i <= 7; i++) {                                  /* crossed bones under it */
        tput(t, 32 + i, 37 + i / 2, sh(R_BONE, 0.7f));
        tput(t, 32 + i, 37 - i / 2, sh(R_BONE, 0.6f));
    }
}

static void gen_torch(int t, int frame, uint32_t seed) {
    gen_stone(t, R_STONE, seed);
    for (int y = 38; y <= 47; y++)                                  /* bracket plate */
        for (int x = 27; x <= 36; x++) {
            float v = 0.42f + ((x == 27 || y == 38) ? 0.2f : 0.0f) - ((x == 36 || y == 47) ? 0.2f : 0.0f);
            if ((x == 29 || x == 34) && (y == 40 || y == 45)) v = 0.9f;
            tput(t, x, y, sh(R_IRON, v));
        }
    for (int y = 33; y < 38; y++) for (int x = 30; x <= 33; x++) tput(t, x, y, sh(R_IRON, 0.55f - (x - 30) * 0.08f));
    for (int y = 20; y <= 35; y++)                                  /* the torch */
        for (int x = 30; x <= 33; x++) tput(t, x, y, sh(R_WOOD, 0.5f - fabsf(x - 31.0f) * 0.12f - (y - 20) * 0.01f));
    static const float SWAY[3] = { 0.0f, 1.6f, -1.3f }, TALL[3] = { 1.0f, 0.9f, 1.1f };
    for (int y = 0; y <= 22; y++)
        for (int x = 20; x <= 44; x++) {
            float u = (22 - y) / (21.0f * TALL[frame]);              /* 0 at the base, 1 at the tip */
            if (u > 1.0f) continue;
            float cx = 31.5f + SWAY[frame] * u * u * 2.0f;
            float w = 5.6f * powf(fmaxf(0.0f, sinf(PI_F * (0.18f + 0.82f * (1.0f - u)))), 0.8f) * (1.0f - 0.35f * u);
            if (w < 0.3f) continue;
            float d = fabsf(x + 0.5f - cx) / (w + 0.01f);
            float n = fbm(x / 4.0f, y / 4.0f + frame * 3.0f, 16, seed + 90 + frame) * 0.5f;
            if (d > 1.0f - 0.25f * n) continue;
            float heat = 1.0f - d * 0.7f - u * 0.55f + n * 0.3f;
            tput(t, x, y, (uint8_t)(R_FLAME + clampi((int)(heat * 15.0f), 1, 15)));
        }
}

static void gen_door(int t, uint32_t seed) {
    for (int y = 0; y < TS; y++)
        for (int x = 0; x < TS; x++) {
            int p = x / 8, lx = x % 8;
            float v = 0.42f + 0.14f * h01(p, 0, seed) + 0.07f * sinf(y * 0.7f + p * 3.0f + x * 0.25f)
                    + fbm(x / 4.0f, y / 12.0f, 16, seed) * 0.12f;
            if (lx == 0) v = 0.05f;
            else if (lx == 1) v += 0.08f;
            else if (lx == 7) v -= 0.08f;
            tput(t, x, y, sh(R_WOOD, v));
        }
    for (int band = 0; band < 2; band++) {
        int y0 = band ? 51 : 9;
        for (int y = y0; y < y0 + 4; y++)
            for (int x = 0; x < TS; x++) {
                float v = 0.45f + (y == y0 ? 0.2f : 0.0f) - (y == y0 + 3 ? 0.2f : 0.0f);
                if (x % 8 == 4 && (y == y0 + 1 || y == y0 + 2)) v = 0.9f;
                tput(t, x, y, sh(R_IRON, v));
            }
    }
    for (int y = 15; y <= 26; y++)                                  /* barred window */
        for (int x = 24; x <= 39; x++) {
            int edge = x == 24 || x == 39 || y == 15 || y == 26;
            int bar = x == 28 || x == 32 || x == 36;
            tput(t, x, y, edge ? sh(R_IRON, 0.4f) : (bar ? sh(R_IRON, 0.65f) : 0));
        }
    for (int a = 0; a < 40; a++) {                                  /* ring handle */
        float ang = TAU_F * a / 40;
        int x = 47 + (int)lroundf(4.0f * cosf(ang)), y = 35 + (int)lroundf(4.5f * sinf(ang));
        tput(t, x, y, sh(R_IRON, 0.5f + 0.35f * -sinf(ang)));
    }
}

static void gen_jamb(int t, uint32_t seed) {
    for (int y = 0; y < TS; y++)
        for (int x = 0; x < TS; x++) {
            float v = 0.42f + fbm(x / 8.0f, y / 8.0f, 8, seed) * 0.2f - 0.1f;
            if (y % 16 == 0) v = 0.12f;
            if (x < 3) v += 0.15f;
            if (x > 60) v -= 0.15f;
            tput(t, x, y, sh(R_STONE, v));
        }
    for (int h = 0; h < 2; h++)
        for (int y = 10 + h * 40; y < 15 + h * 40; y++)
            for (int x = 0; x < 22; x++) tput(t, x, y, sh(R_IRON, 0.5f + (y == 10 + h * 40 ? 0.25f : 0.0f) + (x == 18 ? 0.3f : 0.0f)));
}

/* the locked door: dark planks, gold straps and a big lock plate */
static void gen_locked(int t, uint32_t seed, int metal) {
    for (int y = 0; y < TS; y++)
        for (int x = 0; x < TS; x++) {
            int p = x / 8, lx = x % 8;
            float v = 0.30f + 0.12f * h01(p, 0, seed) + 0.06f * sinf(y * 0.7f + p * 3.0f + x * 0.25f)
                    + fbm(x / 4.0f, y / 12.0f, 16, seed) * 0.12f;
            if (lx == 0) v = 0.04f; else if (lx == 1) v += 0.07f; else if (lx == 7) v -= 0.07f;
            tput(t, x, y, sh(R_WOOD, v));
        }
    for (int band = 0; band < 3; band++) {
        int y0 = band == 0 ? 6 : (band == 1 ? 30 : 54);
        for (int y = y0; y < y0 + 4; y++)
            for (int x = 0; x < TS; x++) {
                float v = 0.55f + (y == y0 ? 0.35f : 0.0f) - (y == y0 + 3 ? 0.35f : 0.0f);
                if (x % 10 == 5 && (y == y0 + 1 || y == y0 + 2)) v = 1.0f;
                tput(t, x, y, sh(metal, metal == R_GOLD ? v : 0.2f + v * 0.8f));
            }
    }
    for (int y = 20; y <= 46; y++)                                  /* lock plate with a keyhole */
        for (int x = 23; x <= 40; x++) {
            int edge = x == 23 || y == 20, shadow = x == 40 || y == 46;
            float v = 0.62f + (edge ? 0.3f : 0.0f) - (shadow ? 0.35f : 0.0f) + fbm(x / 3.0f, y / 3.0f, 16, seed + 7) * 0.15f;
            float kx = x + 0.5f - 31.5f, ky = y + 0.5f - 30.0f;
            if (kx * kx + ky * ky < 7.0f || (fabsf(kx) < 1.2f && ky > 0.0f && ky < 9.0f)) { tput(t, x, y, 0); continue; }
            if ((x == 25 || x == 38) && (y == 22 || y == 44)) v = 1.0f;
            tput(t, x, y, sh(metal, metal == R_GOLD ? v : 0.2f + v * 0.8f));
        }
}

/* a boss arena's gate: an arch of dressed stone over a portcullis, and the dark behind its bars */
static void gen_gate(int t, uint32_t seed) {
    gen_stone(t, R_STONE, seed);
    for (int y = 6; y < TS; y++)
        for (int x = 8; x < 56; x++) {
            float dx = (x + 0.5f - 32.0f) / 24.0f, dy = (y + 0.5f - 22.0f) / 16.0f;
            if (y < 22 && dx * dx + dy * dy > 1.0f) continue;
            float edge = y < 22 ? 1.0f - sqrtf(dx * dx + dy * dy) : fminf((x - 8) / 24.0f, (55 - x) / 24.0f);
            uint8_t c = sh(R_STONE, 0.03f + 0.04f * (1.0f - (float)y / TS));
            if (edge < 0.06f) c = sh(R_STONE, 0.75f);             /* the arch's lit edge */
            int bx = (x - 10) % 7, by = (y - 4) % 10;
            if (edge >= 0.06f && (bx == 0 || bx == 1)) c = sh(R_IRON, bx ? 0.42f : 0.62f + 0.1f * h01(x, y, seed));   /* the bars */
            if (edge >= 0.06f && (by == 0 || by == 1) && y > 8) c = sh(R_IRON, by ? 0.3f : 0.55f);
            if (y >= 58 && bx <= 1 && edge >= 0.06f) c = (y & 1) ? sh(R_IRON, 0.8f) : sh(R_IRON, 0.2f);   /* the spikes, a hand off the floor */
            tput(t, x, y, c);
        }
}

/* the Accursed Portal, in the grave plot's far wall: a stone arch, and in it a slow swirl of red and
 * violet turning in on itself, a few sparks caught in it; four frames, a quarter turn apart */
static void gen_portal(int t, int frame, uint32_t seed) {
    gen_stone(t, R_STONE, seed);
    for (int y = 2; y < TS; y++)
        for (int x = 4; x < 60; x++) {
            float ax = (x + 0.5f - 32.0f) / 28.0f, ay = (y + 0.5f - 24.0f) / 22.0f;
            if (y < 24 && ax * ax + ay * ay > 1.0f) continue;
            float edge = y < 24 ? 1.0f - sqrtf(ax * ax + ay * ay) : fminf((x - 4) / 28.0f, (59 - x) / 28.0f);
            if (edge < 0.1f) { tput(t, x, y, sh(R_STONE, edge < 0.04f ? 0.8f : 0.3f)); continue; }   /* the arch's carved rim */
            float dx = x + 0.5f - 32.0f, dy = y + 0.5f - 36.0f, r = sqrtf(dx * dx + dy * dy), a = atan2f(dy, dx);
            float v = 0.5f + 0.5f * sinf(2.0f * a + r * 0.32f - (float)frame * PI_F * 0.5f) * (0.7f + 0.3f * sinf(r * 0.11f - frame));
            uint8_t c;
            if (r < 3.5f) c = r < 1.6f ? C_WHITE : (uint8_t)(R_PURPLE + 15);   /* its heart */
            else if (v > 0.72f) c = (uint8_t)(R_PURPLE + clampi(11 + (int)((v - 0.72f) * 14.0f), 11, 15));
            else if (v > 0.38f) c = (uint8_t)(R_PURPLE + 6 + (int)((v - 0.38f) * 12.0f));
            else c = (uint8_t)(R_BLOOD + 3 + (int)(v * 10.0f));
            if (h01(x + frame * 7, y, seed) > 0.985f) c = (uint8_t)(R_GLOW + 13);   /* a spark caught in it */
            tput(t, x, y, c);
        }
}

/* a font set into the wall: an arched niche over a stone basin, with a draught of pale water in it (a
 * few full-bright glints so it can be found in the dark), or drunk dry and cracked */
static void gen_font(int t, uint32_t seed, int full) {
    gen_stone(t, R_STONE, seed);
    float cx = 32.0f;
    for (int y = 8; y < 48; y++)
        for (int x = 14; x < 50; x++) {
            float dx = (x + 0.5f - cx) / 18.0f, dy = (y + 0.5f - 22.0f) / 14.0f;
            if (y < 22 && dx * dx + dy * dy > 1.0f) continue;
            tput(t, x, y, sh(R_STONE, 0.03f + 0.08f * (y - 8) / 40.0f));
        }
    for (int y = 38; y < 56; y++)                                   /* the basin, a half bowl standing out of the wall */
        for (int x = 12; x < 52; x++) {
            float dx = (x + 0.5f - cx) / 20.0f, dy = (y + 0.5f - 38.0f) / 16.0f;
            if (dx * dx + dy * dy > 1.0f) continue;
            float v = 0.38f + 0.28f * (-dx * 0.5f) + 0.25f * (1.0f - dy) + fbm(x / 4.0f, y / 4.0f, 16, seed + 5) * 0.12f;
            if (y == 38 || y == 39) v = 0.8f;                       /* its lip */
            tput(t, x, y, sh(R_STONE, v));
        }
    for (int x = 15; x < 49; x++)                                   /* what is in it, seen over the lip */
        for (int y = 34; y < 38; y++) {
            float dx = (x + 0.5f - cx) / 17.0f, dy = (y + 0.5f - 37.5f) / 3.5f;
            if (dx * dx + dy * dy > 1.0f) continue;
            uint8_t c;
            if (full) {
                c = sh(R_GHOST, 0.45f + 0.3f * sinf(x * 0.6f + y * 1.3f) * 0.5f + 0.15f * (y - 34) / 4.0f);
                if (h01(x, y, seed + 9) > 0.86f) c = (uint8_t)(R_GLOW + 9 + (int)(h01(y, x, seed) * 5.0f));
            } else c = sh(R_STONE, 0.05f + 0.05f * h01(x, y, seed));
            tput(t, x, y, c);
        }
    if (!full)                                                      /* a crack down the dry bowl */
        for (int y = 40, x = 30; y < 54; y++) { x += (int)(h01(y, 3, seed) * 3.0f) - 1; tput(t, x, y, sh(R_STONE, 0.06f)); }
}

static void gen_sign(int t, uint32_t seed) {
    gen_stone(t, R_STONE, seed);
    for (int y = 14; y < 48; y++)
        for (int x = 8; x < 56; x++) {
            int edge = y == 14 || y == 47 || x == 8 || x == 55;
            float v = 0.35f + 0.08f * h01(0, (y - 14) / 6, seed) + fbm(x / 6.0f, y / 2.0f, 16, seed + 3) * 0.15f - ((y - 14) % 6 == 0 ? 0.12f : 0.0f);
            tput(t, x, y, edge ? sh(R_WOOD, 0.15f) : sh(R_WOOD, v));
        }
    for (int i = 0; i < 14; i++) { tput(t, 16, i, sh(R_IRON, (i & 1) ? 0.7f : 0.4f)); tput(t, 47, i, sh(R_IRON, (i & 1) ? 0.7f : 0.4f)); }
    for (int y = 18; y < 38; y++)                                  /* a cat's head, burned into the wood */
        for (int x = 18; x < 46; x++) {
            float dx = (x + 0.5f - 32) / 10.0f, dy = (y + 0.5f - 30) / 7.5f;
            int head = dx * dx + dy * dy < 1.0f;
            int ear = (y < 26 && y >= 18) && ((fabsf(x + 0.5f - 25) < (y - 18) * 0.55f) || (fabsf(x + 0.5f - 39) < (y - 18) * 0.55f));
            if (head || ear) tput(t, x, y, sh(R_WOOD, 0.08f));
        }
    tput(t, 28, 29, sh(R_GOLD, 0.9f)); tput(t, 29, 29, sh(R_GOLD, 0.7f)); tput(t, 35, 29, sh(R_GOLD, 0.9f)); tput(t, 36, 29, sh(R_GOLD, 0.7f));
    for (int c = 0; c < 3; c++)                                    /* and three coins */
        for (int y = 40; y < 45; y++)
            for (int x = 22 + c * 8; x < 28 + c * 8; x++)
                if (hypotf(x + 0.5f - (25 + c * 8), y + 0.5f - 42.5f) < 2.6f) tput(t, x, y, sh(R_GOLD, 0.5f + 0.4f * (y < 42)));
}

/* the final door of each level: an arch of old stone, iron doors, a skull for a keystone, and
 * runes that glow; opened, it is nothing but light */
/* A secret's doorway into its pockets (until each theme gives it its own look): an opening cut in the
 * stone, and inside it doorway after doorway going away into a pale light, yellow for the first two
 * sets and cold white for the last two; full-bright, so it shows in the dark of the room */
/* Looking through a doorway (x 18..45, y 10..63) into a room beyond, in one-point perspective towards
 * (32, 34): how far in (s, 1 at the doorway's edge, smaller further in) and which face is seen. */
enum { RV_WALL, RV_CEIL, RV_FLOOR };
static float room_view(int x, int y, int *face) {
    float px = x + 0.5f - 32.0f, py = y + 0.5f - 34.0f;
    float sx = fabsf(px) / 14.0f, st = -py / 24.0f, sb = py / 29.0f, s = sx;
    *face = RV_WALL;
    if (st > s) { s = st; *face = RV_CEIL; }
    if (sb > s) { s = sb; *face = RV_FLOOR; }
    return s;
}

/* A secret's doorway into its pockets, cut through the stone. Into the Backrooms (the first two sets):
 * a glimpse of a pale yellow room under a tube light; into Severance, a white hall and a pale door. */
static void gen_passage(int t, int warm) {
    for (int y = 0; y < TS; y++)
        for (int x = 0; x < TS; x++) {
            uint8_t c;
            if (x < 18 || x > 45 || y < 12) {
                c = sh(R_STONE, block_shade(x, y, 221) * 0.8f);
                if ((x == 17 || x == 46) && y >= 12) c = sh(R_STONE, 0.1f);   /* the cut's edge */
                if (y == 11 && x >= 17 && x <= 46) c = sh(R_STONE, 0.12f);
            } else if (warm) {                                     /* the Backrooms beyond, in bone's pale yellow */
                int face;
                float s = room_view(x, y, &face);
                if (s < 0.4f) c = sh(R_BONE, 0.74f + 0.04f * h01(x, y, 225));          /* the far wall */
                else if (face == RV_CEIL) c = (fabsf(x + 0.5f - 32.0f) < 3.5f * s && s > 0.5f && s < 0.68f) ? C_WHITE : sh(R_BONE, 0.82f - 0.2f * s);
                else if (face == RV_FLOOR) c = sh(R_BONE, 0.42f + 0.05f * h01(x, y, 226) - 0.08f * s);
                else c = sh(R_BONE, 0.66f - 0.14f * s + ((x & 3) == 0 ? 0.04f : 0.0f));
            } else {                                               /* Severance beyond: a white hall, green carpet, a pale door */
                int face;
                float s = room_view(x, y, &face);
                if (s < 0.4f) c = (x >= 30 && x <= 33 && y >= 30 && y <= 42) ? sh(R_MOSS, 0.78f)
                                : (x >= 29 && x <= 34 && y >= 29 && y <= 42) ? sh(R_GHOST, 0.55f) : sh(R_GHOST, 0.88f);   /* the far wall, a door in it */
                else if (face == RV_CEIL) c = (fabsf(x + 0.5f - 32.0f) < 5.0f * s && fmodf(s * 6.0f, 1.0f) < 0.5f) ? C_WHITE : sh(R_GHOST, 0.94f - 0.1f * s);
                else if (face == RV_FLOOR) c = sh(R_MOSS, 0.30f + 0.03f * h01(x, y, 227) - 0.04f * s);
                else c = sh(R_GHOST, 0.86f - 0.12f * s);
            }
            tput(t, x, y, c);
        }
}

/* A chain's doorway after the dweller failed its trial: the same cut in the stone, the frames inside gone
 * dark, and across the opening a dull red seal, cracked through, with a little of it run down the stone */
static void gen_sealed(int t) {
    for (int y = 0; y < TS; y++)
        for (int x = 0; x < TS; x++) {
            uint8_t c;
            if (x < 18 || x > 45 || y < 12) {
                c = sh(R_STONE, block_shade(x, y, 221) * 0.8f);
                if ((x == 17 || x == 46) && y >= 12) c = sh(R_STONE, 0.1f);
                if (y == 11 && x >= 17 && x <= 46) c = sh(R_STONE, 0.12f);
            } else {                                               /* doorway after doorway, unlit */
                int face;
                float s = room_view(x, y, &face), f = fmodf(s * 5.0f, 1.0f);
                c = sh(R_STONE, (f < 0.12f ? 0.13f : 0.04f) + 0.05f * s + (face == RV_FLOOR ? 0.02f : 0.0f));
            }
            float dx = x + 0.5f - 32.0f, dy = y + 0.5f - 33.0f, r = sqrtf(dx * dx + dy * dy);
            float crack = fabsf(dy + dx * 0.55f - 1.5f * sinf(dx * 0.7f));   /* a jagged break, lower left to upper right */
            if (r < 10.5f) {
                float wob = 10.0f + 0.6f * sinf(atan2f(dy, dx) * 7.0f);     /* pressed wax, its rim uneven */
                if (r < wob) {
                    float v = 0.34f + 0.14f * (-dx - dy) / 14.0f - (r > wob - 1.5f ? 0.08f : 0.0f);
                    if (r < 6.0f && r > 4.6f) v -= 0.1f;           /* the ring of the die */
                    if (fabsf(dx) < 0.8f && dy > -4.0f && dy < 4.0f) v -= 0.1f;   /* a mark struck in it */
                    if (fabsf(dy + 1.0f) < 0.8f && fabsf(dx) < 2.6f) v -= 0.1f;
                    c = sh(R_BLOOD, v);
                    if (crack < 0.9f) c = sh(R_STONE, 0.02f);
                    else if (crack < 1.6f) c = sh(R_BLOOD, v - 0.12f);
                }
            }
            if (x >= 27 && x <= 29 && y > 42 && y < 50 + 3 * (x == 28) && r > 9.0f) c = sh(R_BLOOD, 0.22f);   /* a run of it */
            tput(t, x, y, c);
        }
}

/* The Wretched Repository: the stone cut with three narrow arched niches, an iron door shut in each,
 * riveted, a lock plate and a ring, rust running from the rivets */
static void gen_repository(int t) {
    gen_stone(t, R_STONE, 233);
    for (int k = 0; k < 3; k++) {
        int x0 = 3 + k * 21, w = 16, y0 = 10, y1 = 56;
        float cx = x0 + w / 2.0f;
        for (int y = y0 - 1; y <= y1; y++)
            for (int x = x0 - 1; x <= x0 + w; x++) {
                int inside;
                if (y < y0 + 7) { float dx = (x + 0.5f - cx) / (w / 2.0f + 1.0f), dy = (y + 0.5f - (y0 + 7)) / 8.0f; inside = dx * dx + dy * dy <= 1.0f; }
                else inside = x >= x0 - 1 && x <= x0 + w;
                if (!inside) continue;
                uint8_t c = sh(R_STONE, 0.04f);                        /* the niche's dark rim */
                int door = x >= x0 + 1 && x <= x0 + w - 2 && y >= y0 + 2;
                if (door) {
                    float v = 0.30f + 0.10f * (x - x0) / (float)w + (fbm(x / 3.0f, y / 6.0f, 8, 239 + k) - 0.5f) * 0.12f;
                    if (x == x0 + 1 || y == y0 + 2) v += 0.12f;            /* its edge catching the light */
                    if ((y - y0) % 13 == 6) v -= 0.12f;                    /* the bands */
                    c = sh(R_IRON, v);
                    if (((y - y0) % 13 == 5 || (y - y0) % 13 == 7) && (x == x0 + 3 || x == x0 + w - 4)) c = sh(R_IRON, 0.7f);   /* rivets */
                    if (((y - y0) % 13 == 8 || (y - y0) % 13 == 9) && (x == x0 + 3 || x == x0 + w - 4) && h01(x, y, 241 + k) > 0.3f) c = sh(R_WARM, 0.32f);   /* rust run */
                }
                tput(t, x, y, c);
            }
        for (int y = 30; y <= 36; y++) for (int x = (int)cx - 2; x <= (int)cx + 2; x++) tput(t, x, y, sh(R_IRON, 0.62f));   /* the lock plate */
        tput(t, (int)cx, 32, sh(R_IRON, 0.02f)); tput(t, (int)cx, 33, sh(R_IRON, 0.02f));   /* its keyhole */
        for (int a = 0; a < 12; a++) { float an = TAU_F * a / 12.0f; tput(t, (int)(cx + 2.4f * cosf(an)), (int)(41 + 2.4f * sinf(an)), sh(R_IRON, 0.55f + 0.2f * -sinf(an))); }   /* a ring to pull */
        for (int x = x0 - 1; x <= x0 + w; x++) tput(t, x, y1 + 1, sh(R_STONE, 0.7f));   /* the sill */
    }
}

/* ---- the Backrooms (1.0.0b6) ----
 * Everything here is drawn in R_FLOOR (dark brown, mustard, pale yellow) and R_CEIL (off-white), which
 * only the floor, the ceiling and the fog use in the crypt; a Backrooms pocket recolours the two
 * (crawl_art_theme). The tube lights are C_WHITE, and R_FLAME's palest step for the diffuser. */
static float paper_shade(int x, int y, uint32_t seed) {
    int sx = x & 31;
    float v = 0.74f + (fbm(x / 16.0f, y / 16.0f, 4, seed) - 0.5f) * 0.10f;
    if (sx == 0) v -= 0.06f;                                       /* the seam between two strips */
    if (sx == 8 || sx == 24) v += 0.05f;                           /* pinstripes */
    int cy = (y + (x >> 5) * 8) & 15, dx = abs(sx - 16);
    if (dx <= 3 && (cy == dx + 4 || cy == dx + 5)) v -= 0.07f;      /* a small chevron, over and over */
    v -= 0.12f * smoothstepf(34.0f, 58.0f, (float)y) * fbm(x / 8.0f, y / 8.0f, 8, seed + 5);   /* damp, rising */
    if (y <= 1) v -= 0.12f;                                        /* the ceiling's shadow */
    if (y >= 59) v = y == 59 ? 0.64f : 0.44f + 0.04f * h01(x, y, seed + 7);   /* the skirting */
    return v;
}

static void gen_paper(int t, int kind, uint32_t seed) {
    for (int y = 0; y < TS; y++)
        for (int x = 0; x < TS; x++) {
            float v = paper_shade(x, y, seed);
            uint8_t c;
            if (kind == 1 && y < 59) {                             /* a leak from above: a stain, its tide line, runs */
                float dx = x - 40.0f, dy = (y - 2.0f) * 0.8f;
                float r = sqrtf(dx * dx + dy * dy) + (fbm(x / 8.0f, y / 8.0f, 8, seed + 9) - 0.5f) * 12.0f;
                if (r < 20.0f) v -= 0.08f + 0.08f * (r / 20.0f);
                if (r >= 18.5f && r < 21.0f) v -= 0.10f;
                for (int k = 0; k < 3; k++) {
                    float rx = 33.0f + k * 6.0f + 2.0f * sinf(y * 0.21f + k);
                    if (fabsf(x - rx) < 0.8f && y < 30 + k * 9) v -= 0.07f;
                }
            }
            c = sh(R_FLOOR, v);
            if (kind == 2 && x >= 29 && x <= 34 && y >= 48 && y <= 56) {   /* a socket, painted over */
                c = sh(R_CEIL, (x == 29 || y == 48) ? 0.86f : (x == 34 || y == 56) ? 0.52f : 0.74f);
                if ((x == 31 || x == 32) && (y == 50 || y == 51 || y == 53 || y == 54)) c = sh(R_CEIL, 0.18f);
            }
            tput(t, x, y, c);
        }
}

/* a doorway between pockets: off-white trim, and through it another yellow room under a tube */
static void gen_bdoor(int t, uint32_t seed) {
    for (int y = 0; y < TS; y++)
        for (int x = 0; x < TS; x++) {
            uint8_t c;
            int trim = (x >= 16 && x <= 47 && y >= 8) && !(x >= 18 && x <= 45 && y >= 10);
            if (trim) c = sh(R_CEIL, (x == 16 || y == 8) ? 0.86f : (x == 47 || x == 18 || y == 10) ? 0.56f : 0.74f);
            else if (!(x >= 18 && x <= 45 && y >= 10)) c = sh(R_FLOOR, paper_shade(x, y, seed));
            else {
                int face;
                float s = room_view(x, y, &face);
                if (s < 0.4f) c = sh(R_FLOOR, 0.66f + 0.03f * h01(x, y, seed));
                else if (face == RV_CEIL) c = (fabsf(x + 0.5f - 32.0f) < 3.5f * s && s > 0.5f && s < 0.68f) ? C_WHITE : sh(R_CEIL, 0.66f - 0.22f * s);
                else if (face == RV_FLOOR) c = sh(R_FLOOR, 0.40f + 0.06f * h01(x, y, seed + 1) - 0.10f * s);
                else c = sh(R_FLOOR, 0.62f - 0.18f * s);
            }
            tput(t, x, y, c);
        }
}

/* The chain's end: a doorway onto a run of doorways, each 0.7 the size of the one before, paler further
 * in until they burn white; four frames that drift in, so it never arrives. */
static void gen_visportal(int t, int frame, uint32_t seed) {
    const float r = 0.70f, lr = -logf(r), p = frame / 4.0f;
    for (int y = 0; y < TS; y++)
        for (int x = 0; x < TS; x++) {
            uint8_t c;
            int trim = (x >= 16 && x <= 47 && y >= 8) && !(x >= 18 && x <= 45 && y >= 10);
            if (trim) c = sh(R_CEIL, (x == 16 || y == 8) ? 0.86f : (x == 47 || x == 18 || y == 10) ? 0.56f : 0.74f);
            else if (!(x >= 18 && x <= 45 && y >= 10)) c = sh(R_FLOOR, paper_shade(x, y, seed));
            else {
                int face;
                float s = fmaxf(room_view(x, y, &face), 1e-4f);
                float d = -logf(s) / lr + p, f = d - floorf(d);    /* how many doorways in, and where between two */
                int k = (int)floorf(d);
                float heat = clampf((d - 3.0f) / 3.0f, 0.0f, 1.0f);   /* from the fourth doorway on, light itself */
                static const uint8_t BY[4][4] = { { 0, 8, 2, 10 }, { 12, 4, 14, 6 }, { 3, 11, 1, 9 }, { 15, 7, 13, 5 } };
                int lit = heat * 2.0f * 16.0f > BY[y & 3][x & 3] + 0.5f;   /* the light comes on through an ordered dither */
                if (s < 0.02f) c = C_WHITE;
                else if (f < 0.12f) c = heat > 0.4f ? C_WHITE : sh(R_CEIL, 0.86f);   /* each doorway's trim */
                else if (lit) c = (uint8_t)(R_FLAME + clampi(13 + (int)(heat * 2.99f) - (face == RV_FLOOR), 12, 15));
                else if (face == RV_CEIL) c = (fabsf(x + 0.5f - 32.0f) < 3.5f * s && f > 0.45f && f < 0.62f) ? C_WHITE : sh(R_CEIL, 0.60f + 0.05f * k);
                else if (face == RV_FLOOR) c = sh(R_FLOOR, 0.38f + 0.04f * k + 0.05f * h01(x, y, seed + 2));
                else c = sh(R_FLOOR, 0.60f + 0.05f * k);
            }
            tput(t, x, y, c);
        }
}

static void gen_carpet(int t, uint32_t seed) {
    for (int y = 0; y < TS; y++)
        for (int x = 0; x < TS; x++) {
            float v = 0.47f + (h01(x, y, seed) - 0.5f) * 0.10f + (fbm(x / 4.0f, y / 4.0f, 16, seed + 1) - 0.5f) * 0.10f;
            float damp = fbm(x / 16.0f, y / 16.0f, 4, seed + 2);
            if (damp > 0.60f) v -= (damp - 0.60f) * 0.5f;            /* dark where it is wet through */
            tput(t, x, y, sh(R_FLOOR, v));
        }
}

/* ceiling tiles: a grid of T-bars at x 16 and 48 and y 0 and 32, so a panel can sit in the middle of a
 * cell and run its length; kind 0 plain, 1 stained, 2 a tube panel lit, 3 the panel dead */
static void gen_ceiling(int t, int kind, uint32_t seed) {
    for (int y = 0; y < TS; y++)
        for (int x = 0; x < TS; x++) {
            int gx = (x + 16) & 31, gy = y & 31;
            uint8_t c;
            if (gx == 0 || gy == 0) c = sh(R_CEIL, 0.88f);         /* the T-bars */
            else {
                float v = 0.70f + (h01(x, y, seed) - 0.5f) * 0.06f - (gx == 31 || gy == 31 ? 0.08f : 0.0f);
                if (h01(x, y, seed + 3) > 0.94f) v -= 0.10f;         /* pinholes */
                c = sh(R_CEIL, v);
                if (kind == 1) {                                   /* a water stain on the middle tile */
                    float dx = x - 34.0f, dy = y - 15.0f, rr = sqrtf(dx * dx + dy * dy) + (fbm(x / 8.0f, y / 8.0f, 8, seed + 4) - 0.5f) * 8.0f;
                    if (gx > 1 && gx < 31 && gy > 1 && gy < 31 && x >= 17 && x <= 47 && y < 32 && rr < 11.0f)
                        c = sh(R_FLOOR, rr > 9.5f ? 0.46f : 0.66f + 0.04f * h01(x, y, seed + 5));
                }
            }
            if (kind >= 2 && x >= 17 && x <= 47 && y >= 1) {       /* the panel, the cell's length */
                int frame = x <= 18 || x >= 46 || y <= 2 || y >= 62;
                int tube = (x >= 23 && x <= 25) || (x >= 37 && x <= 39);
                if (kind == 2) c = frame ? sh(R_CEIL, 0.92f) : (tube || ((x + y) & 3) == 0) ? C_WHITE : (uint8_t)(R_FLAME + 15);
                else {
                    int end = y <= 8 || y >= 55;
                    c = frame ? sh(R_CEIL, 0.82f) : tube ? sh(R_CEIL, end ? 0.22f : 0.44f) : sh(R_CEIL, ((x + y) & 3) == 0 ? 0.52f : 0.58f);
                }
            }
            tput(t, x, y, c);
        }
}

/* ---- Severance (1.0.0b6) ----
 * Again in R_FLOOR and R_CEIL, which a Severance pocket makes a muted green (the carpet, the doors, the
 * partitions) and a cool white (walls, ceiling, frames, the water cooler). The lit panels are C_WHITE. */
static float white_shade(int x, int y, uint32_t seed) {
    float v = 0.86f + (h01(x, y, seed) - 0.5f) * 0.02f;
    if ((x & 31) == 0) v -= 0.05f;                                 /* where two panels of the wall meet */
    if (y <= 1) v -= 0.08f;
    if (y >= 58) v = y == 58 ? 0.80f : 0.60f + 0.02f * h01(x, y, seed + 1);   /* a grey skirting */
    return v;
}

static void gen_white(int t, int kind, uint32_t seed) {
    for (int y = 0; y < TS; y++)
        for (int x = 0; x < TS; x++) {
            uint8_t c = sh(R_CEIL, white_shade(x, y, seed));
            if (kind == 1 && x >= 22 && x <= 41 && y >= 18 && y <= 30) {   /* a placard: a line of print, an arrow */
                c = sh(R_CEIL, (x == 22 || y == 18) ? 0.92f : (x == 41 || y == 30) ? 0.5f : 0.74f);
                if (y == 21 && x >= 25 && x <= 38 && (x % 3) != 2) c = sh(R_CEIL, 0.3f);
                if ((y == 26 && x >= 25 && x <= 34) || (x >= 34 && x <= 37 && abs(y - 26) <= 37 - x)) c = sh(R_CEIL, 0.15f);
            }
            if (kind == 2 && x >= 16 && x <= 47 && y >= 11 && y <= 37) {   /* a picture, calmer than it needs to be */
                if (x <= 17 || x >= 46 || y <= 12 || y >= 36) c = sh(R_WOOD, (x == 16 || y == 11) ? 0.5f : 0.3f);
                else {
                    float hz = 27.0f + 1.5f * sinf(x * 0.21f) + 0.8f * sinf(x * 0.53f + 1.0f);
                    if (y < hz) c = sh(R_GHOST, 0.84f - (y - 13) * 0.012f);            /* a pale sky */
                    else c = sh(R_MOSS, 0.56f + 0.06f * h01(x, y, seed + 3) - (y - hz) * 0.006f);   /* a field */
                    float tx = x - 37.0f, ty = y - 21.0f;
                    if (tx * tx / 9.0f + ty * ty / 12.0f < 1.0f) c = sh(R_MOSS, 0.3f + 0.08f * h01(x, y, seed + 4));   /* one tree */
                    if (x == 37 && y >= 24 && y < hz + 1) c = sh(R_WOOD, 0.3f);
                }
            }
            tput(t, x, y, c);
        }
}

/* a doorway between pockets: a door more fit for a ward than an office, in a steel frame */
static void gen_sdoor(int t, uint32_t seed) {
    for (int y = 0; y < TS; y++)
        for (int x = 0; x < TS; x++) {
            uint8_t c = sh(R_CEIL, white_shade(x, y, seed));
            if (x >= 15 && x <= 48 && y >= 6) c = sh(R_CEIL, (x == 15 || y == 6) ? 0.84f : x == 48 ? 0.48f : 0.68f);   /* the frame */
            if (x >= 18 && x <= 45 && y >= 9) {                    /* the door, pale green steel */
                c = sh(R_FLOOR, 0.80f + 0.02f * h01(x, y, seed + 2) - (x == 45 || y == 9 ? 0.12f : 0.0f));
                if (x >= 25 && x <= 38 && y >= 16 && y <= 29) {    /* wired glass, dark behind */
                    int edge = x == 25 || x == 38 || y == 16 || y == 29;
                    c = edge ? sh(R_CEIL, 0.42f) : (((x + y) & 3) == 0 || ((x - y) & 3) == 0) ? sh(R_CEIL, 0.36f) : sh(R_FLOOR, 0.14f);
                    if (!edge && x - 26 < 4 && y - 17 < 4 - (x - 26)) c = sh(R_CEIL, 0.5f);   /* a glint on it */
                }
                if (y >= 11 && y <= 13 && x >= 27 && x <= 36) c = sh(R_CEIL, y == 12 && (x & 1) ? 0.2f : 0.55f);   /* a placard */
                if (y >= 35 && y <= 38 && x >= 20 && x <= 43) c = sh(R_IRON, y == 35 ? 0.82f : y == 38 ? 0.38f : 0.62f);   /* the push bar */
                if (y >= 53 && y <= 60 && x >= 19 && x <= 44) c = sh(R_IRON, 0.58f + 0.05f * sinf(x * 1.7f) + (y == 53 ? 0.15f : 0.0f));   /* kick plate */
                if (x == 19 && (abs(y - 15) <= 1 || abs(y - 34) <= 1 || abs(y - 52) <= 1)) c = sh(R_IRON, 0.4f);   /* hinges */
            }
            tput(t, x, y, c);
        }
}

static void gen_scarpet(int t, uint32_t seed) {                   /* close office carpet: a tight weave, nothing spilled */
    for (int y = 0; y < TS; y++)
        for (int x = 0; x < TS; x++) {
            float v = 0.50f + (h01(x, y, seed) - 0.5f) * 0.05f + ((((x >> 1) + (y >> 1)) & 1) ? 0.025f : 0.0f);
            tput(t, x, y, sh(R_FLOOR, v));
        }
}

/* ceiling panels, four to a cell; lit, every panel shines (C_WHITE, with a faint prism in R_GLOW) */
static void gen_sceiling(int t, int lit_up, uint32_t seed) {
    for (int y = 0; y < TS; y++)
        for (int x = 0; x < TS; x++) {
            int gx = x & 31, gy = y & 31;
            uint8_t c;
            if (gx == 0 || gy == 0) c = sh(R_CEIL, 0.70f);
            else if (gx == 31 || gy == 31) c = sh(R_CEIL, 0.80f);
            else if (lit_up && gx >= 3 && gx <= 28 && gy >= 3 && gy <= 28) c = ((x + 2 * y) % 5 == 0) ? (uint8_t)(R_GLOW + 15) : C_WHITE;
            else c = sh(R_CEIL, 0.90f + (h01(x, y, seed) - 0.5f) * 0.02f);
            tput(t, x, y, c);
        }
}

static void gen_exit(int t, int frame, int open) {
    for (int y = 0; y < TS; y++)
        for (int x = 0; x < TS; x++) {
            float dx = x + 0.5f - 32.0f, dy = y + 0.5f - 27.0f;
            int in_door = (y >= 27 && fabsf(dx) < 19.0f) || (y < 27 && dx * dx + dy * dy < 19.0f * 19.0f);
            int in_arch = (y >= 27 && fabsf(dx) < 24.0f) || (y < 27 && dx * dx + dy * dy < 24.0f * 24.0f);
            uint8_t c;
            if (!in_arch) {                                        /* the wall around it */
                c = sh(R_STONE, block_shade(x, y, 211) * 0.8f);
            } else if (!in_door) {                                 /* voussoirs of the arch */
                float a = atan2f(dy, dx), r = sqrtf(dx * dx + dy * dy);
                int seg = y >= 27 ? (int)(y / 9) + 40 : (int)((a + PI_F) / (PI_F / 9.0f));
                float v = 0.45f + 0.18f * h01(seg, 3, 213) + fbm(x / 5.0f, y / 5.0f, 8, 214) * 0.15f;
                if ((y < 27 && fmodf((a + PI_F) / (PI_F / 9.0f), 1.0f) < 0.12f) || (y >= 27 && y % 9 == 0)) v = 0.12f;
                if (r > 23.0f || (y >= 27 && fabsf(dx) > 23.0f)) v -= 0.12f;
                c = sh(R_STONE, v);
                int rune = (y < 27 ? (int)((a + PI_F) / (PI_F / 9.0f)) : y / 9 + 9) % 2 == 0;   /* runes cut into every other stone */
                float rx = y < 27 ? fmodf((a + PI_F) / (PI_F / 9.0f), 1.0f) : (fabsf(dx) - 19.0f) / 5.0f;
                float ry = y < 27 ? (r - 19.0f) / 5.0f : fmodf(y / 9.0f, 1.0f);
                if (rune && rx > 0.3f && rx < 0.7f && ry > 0.25f && ry < 0.75f && ((x * 3 + y * 5 + (int)(rx * 7)) % 3 == 0))
                    c = (uint8_t)(R_GLOW + (frame ? 15 : 11));
            } else if (open) {                                     /* thrown open: a deep blue swirl, a pale light at its heart */
                float d = sqrtf(dx * dx * 0.8f + (y < 27 ? dy * dy : 0.0f) + (y - 40.0f) * (y - 40.0f) * 0.15f);
                float sw = sinf(atan2f(y - 40.0f, dx) * 5.0f + d * 0.45f) * 0.5f + 0.5f;
                int lv = clampi((int)(4.5f - d * 0.25f + sw * 3.0f), 0, 7);
                c = (uint8_t)(R_GLOW + (d < 4.0f ? 9 : lv));
            } else {                                               /* two iron-bound doors */
                int seam = fabsf(dx) < 0.8f;
                float v = 0.22f + 0.1f * h01(x / 6, 1, 215) + fbm(x / 3.0f, y / 9.0f, 16, 216) * 0.1f;
                if ((y - 2) % 12 == 0) v += 0.18f;
                if ((y - 2) % 12 == 1) v -= 0.08f;
                c = seam ? (uint8_t)R_IRON : sh(R_IRON, v);
                if ((y - 2) % 12 == 0 && ((x + 2) % 6 == 0)) c = sh(R_IRON, 0.85f);   /* rivets */
                float rr = sqrtf(dx * dx + (y - 41.0f) * (y - 41.0f));           /* the glowing seal */
                if (fabsf(rr - 8.0f) < 0.9f) c = (uint8_t)(R_GLOW + (frame ? 14 : 12));
                if (rr < 8.0f && (fabsf(dx) < 0.7f || fabsf(y - 41.0f) < 0.7f)) c = (uint8_t)(R_GLOW + (frame ? 12 : 10));
            }
            tput(t, x, y, c);
        }
    for (int y = 0; y < 9; y++)                                    /* the skull keystone */
        for (int x = 26; x < 38; x++) {
            float nx = (x + 0.5f - 32.0f) / 5.5f, ny = (y + 0.5f - 4.0f) / 4.6f;
            if (nx * nx + ny * ny > 1.0f) continue;
            float v = 0.55f + 0.4f * (-nx * 0.4f - ny * 0.5f);
            if (((x == 29 || x == 30) || (x == 33 || x == 34)) && (y == 3 || y == 4)) { tput(t, x, y, open ? (uint8_t)(R_GLOW + 15) : 0); continue; }
            if (x == 31 && y == 6) v = 0.1f;
            tput(t, x, y, sh(R_BONE, v));
        }
}

/* ------------------------------------------------------------------ */
/* Sprite canvas                                                       */
/* ------------------------------------------------------------------ */

static uint8_t *cvp;
static int cvw, cvh;
static uint8_t cv_sprite[TS * TS];

static void cv_begin(uint8_t *buf, int w, int h) { cvp = buf; cvw = w; cvh = h; memset(buf, C_CLEAR, (size_t)w * h); }
static void cv_px(int x, int y, uint8_t c) { if ((unsigned)x < (unsigned)cvw && (unsigned)y < (unsigned)cvh) cvp[y * cvw + x] = c; }

static float lit(float nx, float ny, float nz) {
    float d = nx * -0.45f + ny * -0.65f + nz * 0.62f;
    return d < 0.0f ? 0.0f : d;
}

static void cv_ellipse(float cx, float cy, float rx, float ry, int ramp, float lo, float hi) {
    for (int y = (int)floorf(cy - ry); y <= (int)ceilf(cy + ry); y++)
        for (int x = (int)floorf(cx - rx); x <= (int)ceilf(cx + rx); x++) {
            float nx = (x + 0.5f - cx) / rx, ny = (y + 0.5f - cy) / ry, d = nx * nx + ny * ny;
            if (d > 1.0f) continue;
            cv_px(x, y, sh(ramp, lo + (hi - lo) * lit(nx, ny, sqrtf(1.0f - d))));
        }
}

static void cv_flat(float cx, float cy, float rx, float ry, uint8_t c) {
    for (int y = (int)floorf(cy - ry); y <= (int)ceilf(cy + ry); y++)
        for (int x = (int)floorf(cx - rx); x <= (int)ceilf(cx + rx); x++) {
            float nx = (x + 0.5f - cx) / rx, ny = (y + 0.5f - cy) / ry;
            if (nx * nx + ny * ny <= 1.0f) cv_px(x, y, c);
        }
}

static void cv_capsule(float x0, float y0, float x1, float y1, float r, int ramp, float lo, float hi) {
    float vx = x1 - x0, vy = y1 - y0, len2 = vx * vx + vy * vy + 1e-6f;
    int xa = (int)floorf(fminf(x0, x1) - r), xb = (int)ceilf(fmaxf(x0, x1) + r);
    int ya = (int)floorf(fminf(y0, y1) - r), yb = (int)ceilf(fmaxf(y0, y1) + r);
    for (int y = ya; y <= yb; y++)
        for (int x = xa; x <= xb; x++) {
            float px = x + 0.5f, py = y + 0.5f;
            float t = clampf(((px - x0) * vx + (py - y0) * vy) / len2, 0.0f, 1.0f);
            float ox = (px - (x0 + vx * t)) / r, oy = (py - (y0 + vy * t)) / r, d = ox * ox + oy * oy;
            if (d > 1.0f) continue;
            cv_px(x, y, sh(ramp, lo + (hi - lo) * lit(ox, oy, sqrtf(1.0f - d))));
        }
}

static void cv_line(float x0, float y0, float x1, float y1, uint8_t c) {
    int n = (int)(fmaxf(fabsf(x1 - x0), fabsf(y1 - y0))) + 1;
    for (int i = 0; i <= n; i++) cv_px((int)(x0 + (x1 - x0) * i / n), (int)(y0 + (y1 - y0) * i / n), c);
}

static void cv_poly(const float *p, int n, int ramp, float lo, float hi, float ystart, float yspan) {
    float ymin = 1e9f, ymax = -1e9f;
    for (int i = 0; i < n; i++) { ymin = fminf(ymin, p[2 * i + 1]); ymax = fmaxf(ymax, p[2 * i + 1]); }
    for (int y = (int)floorf(ymin); y <= (int)ceilf(ymax); y++) {
        float yc = y + 0.5f, xs[16]; int k = 0;
        for (int i = 0; i < n && k < 16; i++) {
            float ax = p[2 * i], ay = p[2 * i + 1], bx = p[2 * ((i + 1) % n)], by = p[2 * ((i + 1) % n) + 1];
            if ((ay <= yc && by > yc) || (by <= yc && ay > yc)) xs[k++] = ax + (bx - ax) * (yc - ay) / (by - ay);
        }
        for (int i = 1; i < k; i++) { float v = xs[i]; int j = i - 1; while (j >= 0 && xs[j] > v) { xs[j + 1] = xs[j]; j--; } xs[j + 1] = v; }
        for (int i = 0; i + 1 < k; i += 2)
            for (int x = (int)ceilf(xs[i] - 0.5f); x <= (int)floorf(xs[i + 1] - 0.5f); x++) {
                float v = lo + (hi - lo) * clampf(1.0f - (yc - ystart) / yspan, 0.0f, 1.0f);
                cv_px(x, y, sh(ramp, v));
            }
    }
}

static void cv_outline(uint8_t c) {                    /* dark rim around the silhouette */
    static uint8_t tmp[HAND_W * HAND_H];
    memcpy(tmp, cvp, (size_t)cvw * cvh);
    for (int y = 0; y < cvh; y++)
        for (int x = 0; x < cvw; x++) {
            if (tmp[y * cvw + x] != C_CLEAR) continue;
            int n = 0;
            if (x > 0 && tmp[y * cvw + x - 1] != C_CLEAR && !IS_FULLBRIGHT(tmp[y * cvw + x - 1])) n++;
            if (x < cvw - 1 && tmp[y * cvw + x + 1] != C_CLEAR && !IS_FULLBRIGHT(tmp[y * cvw + x + 1])) n++;
            if (y > 0 && tmp[(y - 1) * cvw + x] != C_CLEAR && !IS_FULLBRIGHT(tmp[(y - 1) * cvw + x])) n++;
            if (y < cvh - 1 && tmp[(y + 1) * cvw + x] != C_CLEAR && !IS_FULLBRIGHT(tmp[(y + 1) * cvw + x])) n++;
            if (n) cvp[y * cvw + x] = c;
        }
}

static void cv_store(int id) {
    for (int x = 0; x < TS; x++)
        for (int y = 0; y < TS; y++) crawl_spr[id][x * TS + y] = cv_sprite[y * TS + x];
}

static void cv_bone(float x0, float y0, float x1, float y1, float r, float lo, float hi) {
    float dx = x1 - x0, dy = y1 - y0, L = sqrtf(dx * dx + dy * dy) + 1e-3f, px = -dy / L, py = dx / L;
    cv_capsule(x0, y0, x1, y1, r, R_BONE, lo, hi);
    for (int e = 0; e < 2; e++) {
        float ex = e ? x1 : x0, ey = e ? y1 : y0;
        cv_ellipse(ex + px * r * 0.7f, ey + py * r * 0.7f, r * 1.05f, r * 1.05f, R_BONE, lo, hi);
        cv_ellipse(ex - px * r * 0.7f, ey - py * r * 0.7f, r * 1.05f, r * 1.05f, R_BONE, lo, hi);
    }
}

static void cv_skull(float cx, float cy, float s, float lo, float hi) {
    cv_ellipse(cx, cy, 6.2f * s, 5.8f * s, R_BONE, lo, hi);
    cv_ellipse(cx, cy + 4.2f * s, 4.4f * s, 2.9f * s, R_BONE, lo * 0.9f, hi * 0.85f);
    cv_flat(cx - 2.4f * s, cy + 0.9f * s, 1.9f * s, 2.1f * s, R_BONE);
    cv_flat(cx + 2.4f * s, cy + 0.9f * s, 1.9f * s, 2.1f * s, R_BONE);
    cv_flat(cx, cy + 3.1f * s, 0.8f * s, 1.1f * s, R_BONE);
    for (int i = -2; i <= 2; i++) cv_px((int)(cx + i * 1.3f * s), (int)(cy + 5.3f * s), R_BONE + 1);
}

/* ------------------------------------------------------------------ */
/* Sprites                                                             */
/* ------------------------------------------------------------------ */

static void spr_bones(int id, int heap, uint32_t seed) {
    cv_begin(cv_sprite, TS, TS);
    cv_flat(32, 61.5f, heap ? 26 : 20, heap ? 3.5f : 2.5f, R_WARM + 1);
    int n = heap ? 10 : 5;
    for (int i = 0; i < n; i++) {
        float cx = 32 + (h01(i, 1, seed) - 0.5f) * (heap ? 38 : 30);
        float cy = (heap ? 60 - 14 * h01(i, 2, seed) * (1.0f - fabsf(cx - 32) / 24) : 59 - 3 * h01(i, 2, seed));
        float a = (h01(i, 3, seed) - 0.5f) * (heap ? 2.4f : 0.9f);
        float l = 7 + 5 * h01(i, 4, seed);
        cv_bone(cx - cosf(a) * l, cy - sinf(a) * l, cx + cosf(a) * l, cy + sinf(a) * l, 1.4f, 0.25f, 0.92f);
    }
    if (heap) { cv_skull(23, 48, 1.0f, 0.3f, 0.95f); cv_skull(41, 50, 1.0f, 0.3f, 0.95f); cv_skull(32, 41, 0.9f, 0.3f, 1.0f); }
    else cv_skull(42, 54, 0.95f, 0.3f, 0.95f);
    cv_outline(0);
    cv_store(id);
}

static void spr_skullpile(void) {
    cv_begin(cv_sprite, TS, TS);
    cv_flat(32, 62, 26, 2.5f, R_WARM + 1);
    static const float ROWS[4][5] = { {56, 14, 26, 38, 50}, {47, 20, 32, 44, 0}, {38, 26, 38, 0, 0}, {29, 32, 0, 0, 0} };
    for (int r = 0; r < 4; r++)
        for (int i = 1; i < 5 && ROWS[r][i] > 0; i++) cv_skull(ROWS[r][i], ROWS[r][0], 0.98f, 0.28f, 0.95f - r * 0.02f);
    cv_outline(0);
    cv_store(SP_SKULLPILE);
}

static void spr_cobweb(void) {
    cv_begin(cv_sprite, TS, TS);
    uint8_t c = R_STONE + 12, c2 = R_STONE + 9;
    float ax[7], ay[7];
    for (int i = 0; i < 7; i++) {
        float a = PI_F * (0.08f + 0.84f * i / 6.0f);
        ax[i] = 32 + cosf(a) * 31; ay[i] = 1 + sinf(a) * 34;
        cv_line(32, 0, ax[i], ay[i], c);
    }
    for (int ring = 1; ring <= 5; ring++) {
        float f = ring / 5.5f;
        for (int i = 0; i < 6; i++) {
            float x0 = 32 + (ax[i] - 32) * f, y0 = (ay[i]) * f, x1 = 32 + (ax[i + 1] - 32) * f, y1 = ay[i + 1] * f;
            float mx = (x0 + x1) / 2, my = (y0 + y1) / 2 + 1.5f * f;
            cv_line(x0, y0, mx, my, c2); cv_line(mx, my, x1, y1, c2);
        }
    }
    cv_px(24, 14, R_IRON + 1); cv_px(25, 14, R_IRON + 1); cv_px(40, 20, R_IRON + 1);
    cv_store(SP_COBWEB);
}

static void spr_chains(void) {
    cv_begin(cv_sprite, TS, TS);
    for (int c = 0; c < 2; c++) {
        float x = c ? 39 : 25; int len = c ? 46 : 38;
        for (int y = 0; y < len; y += 4) {
            if ((y / 4) & 1) { cv_capsule(x, y, x, y + 4, 0.9f, R_IRON, 0.2f, 0.85f); }
            else for (int a = 0; a < 16; a++) {
                float ang = TAU_F * a / 16;
                cv_px((int)(x + 0.5f + 2.0f * cosf(ang)), (int)(y + 2.5f + 2.6f * sinf(ang)), sh(R_IRON, 0.45f - 0.3f * sinf(ang + 0.7f)));
            }
        }
        for (int a = 0; a < 24; a++) {                               /* shackle */
            float ang = PI_F * a / 23;
            cv_px((int)(x + 0.5f + 4.0f * cosf(ang)), (int)(len + 3 + 4.0f * sinf(ang)), sh(R_IRON, 0.7f - 0.4f * cosf(ang)));
        }
    }
    cv_store(SP_CHAINS);
}

static void spr_urn(void) {
    cv_begin(cv_sprite, TS, TS);
    cv_flat(32, 62, 13, 2, R_WARM + 1);
    cv_ellipse(32, 51, 11, 10, R_BRICK, 0.15f, 0.8f);
    for (int y = 37; y <= 42; y++) for (int x = 27; x <= 37; x++) cv_px(x, y, sh(R_BRICK, 0.45f - (x - 27) * 0.03f));
    cv_ellipse(32, 37, 7.5f, 2.2f, R_BRICK, 0.4f, 0.9f);
    cv_flat(32, 37, 5, 1.2f, R_BRICK + 1);
    for (int x = 22; x <= 42; x++) cv_px(x, 50, R_BRICK + 3);
    for (int s = -1; s <= 1; s += 2)
        for (int a = 0; a < 14; a++) {
            float ang = PI_F * 0.5f + PI_F * a / 13 * s;
            cv_px((int)(32 + s * 11 + 3.0f * cosf(ang) * -s + 0.5f), (int)(45 + 3.0f * sinf(ang)), sh(R_BRICK, 0.6f));
        }
    cv_outline(0);
    cv_store(SP_URN);
}

static void spr_pillar(void) {
    cv_begin(cv_sprite, TS, TS);
    for (int y = 0; y < TS; y++) {
        float r = (y < 6 || y > 57) ? 13.0f : 10.0f;
        for (int x = 0; x < TS; x++) {
            float nx = (x + 0.5f - 32) / r;
            if (fabsf(nx) > 1.0f) continue;
            float nz = sqrtf(1 - nx * nx);
            float v = 0.18f + 0.7f * lit(nx, 0.0f, nz);
            if (r < 11.0f && cosf(nx * PI_F * 4.0f) > 0.7f) v -= 0.08f;
            if (y == 6 || y == 57) v -= 0.15f;
            if (y == 0 || y == 63) v -= 0.1f;
            v += fbm(x / 6.0f, y / 6.0f, 16, 3) * 0.12f - 0.06f;
            cv_px(x, y, sh(R_STONE, v));
        }
    }
    cv_store(SP_PILLAR);
}

static void flame(float cx, float by, float hgt, float w, float sway) {
    for (int y = (int)(by - hgt); y <= (int)by; y++)
        for (int x = (int)(cx - w - 2); x <= (int)(cx + w + 2); x++) {
            float u = (by - y) / hgt;
            float c = cx + sway * u * u;
            float ww = w * fmaxf(0.0f, sinf(PI_F * (0.15f + 0.85f * (1.0f - u)))) * (1.0f - 0.3f * u);
            if (ww < 0.3f) continue;
            float d = fabsf(x + 0.5f - c) / (ww + 0.01f);
            if (d > 1.0f) continue;
            cv_px(x, y, (uint8_t)(R_FLAME + clampi((int)((1.0f - d * 0.6f - u * 0.5f) * 15.0f), 2, 15)));
        }
}

static void spr_candles(void) {
    cv_begin(cv_sprite, TS, TS);
    cv_ellipse(32, 61, 15, 2.5f, R_BONE, 0.3f, 0.6f);
    static const float C[3][2] = { {24, 15}, {33, 21}, {42, 11} };
    for (int i = 0; i < 3; i++) {
        float x = C[i][0], h = C[i][1];
        for (int y = (int)(61 - h); y <= 61; y++)
            for (int xx = (int)x - 2; xx <= (int)x + 2; xx++) {
                float nx = (xx + 0.5f - x) / 2.6f;
                cv_px(xx, y, sh(R_BONE, 0.35f + 0.55f * lit(nx, 0, sqrtf(fmaxf(0, 1 - nx * nx)))));
            }
        cv_px((int)x, (int)(60 - h), R_IRON + 1);
        flame(x + 0.5f, 59 - h, 7, 2.2f, 0.6f);
    }
    cv_store(SP_CANDLES);
}

/* the skeleton: a list of bones that can collapse into a heap */
typedef struct { int kind; float x0, y0, x1, y1, r; } Part;   /* kind 0 bone, 1 skull, 2 pelvis */

static float g_decay;                                   /* 0 on the first level, 1 eight levels down */
static void gem(float cx, float cy, float r, int ramp);
static void skel_draw(float walk, float k, uint32_t seed, int raise) {
    Part P[32]; int n = 0;
    #define BONE(a, b, c, d, r) (P[n++] = (Part){ 0, a, b, c, d, r })
    float w = walk;
    BONE(29, 42, 27 + w * 3, 52, 1.6f); BONE(27 + w * 3, 52, 27 + w * 4.5f, 61, 1.4f); BONE(27 + w * 4.5f, 62, 23 + w * 4.5f, 62.5f, 1.0f);
    BONE(35, 42, 37 - w * 3, 52, 1.6f); BONE(37 - w * 3, 52, 37 - w * 4.5f, 61, 1.4f); BONE(37 - w * 4.5f, 62, 41 - w * 4.5f, 62.5f, 1.0f);
    P[n++] = (Part){ 2, 32, 40, 0, 0, 0 };
    BONE(32, 18, 32, 38, 1.4f);
    for (int i = 0; i < 4; i++) {
        float y = 21 + i * 3.6f;
        BONE(31, y, 25 + i * 0.6f, y + 2, 0.9f); BONE(25 + i * 0.6f, y + 2, 27 + i * 0.5f, y + 4.2f, 0.8f);
        BONE(33, y, 39 - i * 0.6f, y + 2, 0.9f); BONE(39 - i * 0.6f, y + 2, 37 - i * 0.5f, y + 4.2f, 0.8f);
    }
    BONE(24, 19, 40, 19, 1.1f);
    BONE(24, 19, 21, 30, 1.3f); BONE(21, 30, 19 - w * 2, 40, 1.1f);
    if (raise == 1) { BONE(40, 19, 46, 11, 1.3f); BONE(46, 11, 44, 3, 1.1f); BONE(38, 4, 50, 1, 1.0f); }   /* bone held high */
    else if (raise == 2) { BONE(40, 19, 47, 12, 1.3f); BONE(47, 12, 48, 4, 1.1f); }   /* arm up, to strike */
    else { BONE(40, 19, 43, 29, 1.3f); BONE(43, 29, 48, 22, 1.1f); }
    BONE(32, 15, 32, 18, 1.2f);
    P[n++] = (Part){ 1, 32, 9, 0, 0, 1.0f };
    #undef BONE

    cv_begin(cv_sprite, TS, TS);
    if (k > 0.5f) cv_flat(32, 61.5f, 20, 2.5f, R_WARM + 1);
    for (int i = 0; i < n; i++) {
        Part p = P[i];
        if (p.kind == 0 && p.r < 1.3f && h01(i, 41, 77) < g_decay * 0.3f) continue;   /* deeper down, bones go missing */
        float cx = p.kind ? p.x0 : (p.x0 + p.x1) / 2, cy = p.kind ? p.y0 : (p.y0 + p.y1) / 2;
        float ty = 57.0f + 5.0f * h01(i, 1, seed), tx = cx + (h01(i, 2, seed) - 0.5f) * 26.0f;
        float kk = k * k * (3 - 2 * k);
        float ncx = cx + (tx - cx) * kk, ncy = cy + (ty - cy) * powf(kk, 0.8f);
        float rot = kk * (h01(i, 3, seed) - 0.5f) * 3.0f + kk * 1.2f;
        float cr = cosf(rot), sr = sinf(rot);
        if (p.kind == 0) {
            float ax = p.x0 - cx, ay = p.y0 - cy, bx = p.x1 - cx, by = p.y1 - cy;
            float x0 = ncx + ax * cr - ay * sr, y0 = ncy + ax * sr + ay * cr;
            float x1 = ncx + bx * cr - by * sr, y1 = ncy + bx * sr + by * cr;
            if (p.r >= 1.3f) cv_bone(x0, y0, x1, y1, p.r * 0.9f, 0.25f, 0.95f);
            else cv_capsule(x0, y0, x1, y1, p.r, R_BONE, 0.25f, 0.9f);
        } else if (p.kind == 1) {
            cv_skull(ncx, ncy - (kk > 0.9f ? 1.5f : 0.0f), 1.0f, 0.3f, 1.0f);
            if (g_decay > 0.3f) { cv_line(ncx + 1, ncy - 5, ncx + 3, ncy - 1, R_BONE + 2); cv_px((int)ncx + 4, (int)ncy - 2, R_BONE + 1); }   /* a cracked skull */
            uint8_t eye = g_decay > 0.55f ? (uint8_t)(R_GLOW + 13) : (uint8_t)(R_FLAME + 10);   /* the eyes go cold */
            if (k < 0.3f) { cv_px((int)ncx - 2, (int)ncy + 1, eye); cv_px((int)ncx + 2, (int)ncy + 1, eye); }
        } else {
            cv_ellipse(ncx, ncy, 7, 3.4f, R_BONE, 0.25f, 0.9f);
            cv_flat(ncx - 3, ncy, 1.2f, 1.2f, R_BONE); cv_flat(ncx + 3, ncy, 1.2f, 1.2f, R_BONE);
        }
    }
    if (g_decay > 0.0f)                                            /* grime, rot and moss on the bones */
        for (int y = 0; y < TS; y++)
            for (int x = 0; x < TS; x++) {
                uint8_t c = cv_sprite[y * TS + x];
                if (c < R_BONE || c >= R_BONE + 16) continue;
                float n = fbm(x / 5.0f, y / 5.0f, 16, seed + 31);
                if (n < 0.28f + 0.32f * g_decay) cv_sprite[y * TS + x] = (uint8_t)(c - 3 < R_BONE + 1 ? R_BONE + 1 : c - 3);
                if (g_decay > 0.35f && n > 0.78f - 0.12f * g_decay) cv_sprite[y * TS + x] = sh(R_MOSS, 0.3f + 0.3f * n);
            }
}

static void spr_skeleton(int id, float walk, float k, uint32_t seed, int raise) {
    skel_draw(walk, k, seed, raise);
    cv_outline(0);
    cv_store(id);
}

/* A skeleton with a round wooden shield on its arm and a sword: it closes in and strikes */
static void spr_shieldskel(int id, float walk, int swing, int shield) {
    skel_draw(walk, 0.0f, 13, swing ? 2 : 0);
    if (swing) {                                                   /* the blade raised over its head */
        cv_capsule(48, 4, 46, -20, 1.0f, R_IRON, 0.5f, 1.0f);
        cv_capsule(44, 5, 52, 3, 0.9f, R_WOOD, 0.3f, 0.6f);
    } else {                                                       /* held low and forward */
        cv_capsule(48, 22, 55, 7, 1.0f, R_IRON, 0.5f, 1.0f);
        cv_capsule(45, 24, 51, 20, 0.9f, R_WOOD, 0.3f, 0.6f);
    }
    if (shield) {                                                  /* the shield on the left arm, in front of the ribs */
        float cx = 22.5f, cy = 30.0f;
        for (int y = 15; y < 46; y++)
            for (int x = 7; x < 39; x++) {
                float nx = (x + 0.5f - cx) / 13.0f, ny = (y + 0.5f - cy) / 14.5f, d = nx * nx + ny * ny;
                if (d > 1.0f) continue;
                float v = 0.3f + 0.45f * lit(nx * 0.4f, ny * 0.4f, sqrtf(1.0f - d * 0.6f));
                uint8_t c;
                if (d > 0.8f) c = sh(R_IRON, v + 0.25f);                                 /* iron rim */
                else if (d < 0.05f) c = sh(R_IRON, v + 0.45f);                           /* the boss */
                else {
                    int plank = (int)((x - 7) / 5.5f);
                    c = sh(R_WOOD, v + 0.08f * h01(plank, 3, 51) - (((x - 7) % 6) == 0 ? 0.12f : 0.0f) - g_decay * 0.15f * h01(x / 3, y / 3, 52));
                    if (fabsf(y + 0.5f - cy) < 1.2f) c = sh(R_IRON, v + 0.1f);          /* a band across */
                }
                cv_px(x, y, c);
            }
        cv_px(18, 24, sh(R_IRON, 0.9f)); cv_px(27, 36, sh(R_IRON, 0.9f));
    } else {                                                       /* what is left of it: a splintered strap */
        cv_capsule(19, 26, 26, 33, 1.0f, R_WOOD, 0.2f, 0.45f);
        cv_line(24, 27, 21, 22, sh(R_WOOD, 0.4f)); cv_line(26, 31, 29, 27, sh(R_WOOD, 0.35f));
    }
    cv_outline(0);
    cv_store(id);
}

/* The wall skull: a great skull set into the stone. Asleep it is only bone; woken, fire gathers in
 * its eyes and mouth; then it blazes. */
static void spr_wskull(int id, int heat) {
    cv_begin(cv_sprite, TS, TS);
    if (heat == 3) {                                               /* broken, falling */
        cv_ellipse(24, 40, 12, 10, R_BONE, 0.25f, 0.9f); cv_ellipse(43, 46, 10, 8, R_BONE, 0.2f, 0.8f);
        cv_line(24, 31, 30, 46, R_BONE + 1); cv_line(40, 40, 46, 52, R_BONE + 1);
        cv_ellipse(22, 40, 3.5f, 4.0f, R_BONE, 0.0f, 0.05f);
        cv_outline(0); cv_store(id); return;
    }
    cv_ellipse(32, 27, 21, 20, R_BONE, 0.22f, 0.95f);              /* the dome */
    cv_ellipse(32, 44, 14, 9, R_BONE, 0.2f, 0.8f);                 /* the jaw */
    for (int y = 0; y < TS; y++)                                   /* cracks and grime */
        for (int x = 0; x < TS; x++) {
            uint8_t *p = &cv_sprite[y * TS + x];
            if (*p == C_CLEAR) continue;
            if (fbm(x / 6.0f, y / 6.0f, 16, 71) < 0.3f) *p = (uint8_t)(*p - 2 < R_BONE ? R_BONE : *p - 2);
        }
    cv_line(22, 9, 27, 20, R_BONE + 2); cv_line(27, 20, 25, 24, R_BONE + 2);
    for (int k = -1; k <= 1; k += 2) {                             /* sockets */
        float ex = 32 + k * 8.5f, ey = 29;
        cv_flat(ex, ey, 6.0f, 6.5f, 0);
        if (heat) {
            float r = heat == 1 ? 2.5f : 4.6f;
            for (int y = (int)(ey - 7); y <= (int)(ey + 7); y++)
                for (int x = (int)(ex - 7); x <= (int)(ex + 7); x++) {
                    float d = hypotf(x + 0.5f - ex, (y + 0.5f - ey) * 0.9f) / r;
                    if (d < 1.0f) cv_px(x, y, (uint8_t)(R_FLAME + clampi((int)((1.1f - d * 0.8f) * 15), 2, 15)));
                }
            if (heat == 2) for (int i = 0; i < 6; i++) cv_px((int)(ex + (h01(i, k + 2, 73) - 0.5f) * 8), (int)(ey - 6 - h01(i, k + 5, 73) * 6), (uint8_t)(R_FLAME + 9 + (i & 3)));
        } else cv_px((int)ex, (int)ey + 1, R_FLAME + 3);           /* a last ember */
    }
    float nose[6] = { 32, 34, 29, 40, 35, 40 };                    /* nose */
    cv_poly(nose, 3, R_BONE, 0.0f, 0.0f, 34, 6);
    for (int y = 34; y < 41; y++) for (int x = 29; x < 36; x++) if (cv_sprite[y * TS + x] == R_BONE) cv_sprite[y * TS + x] = 0;
    for (int i = -4; i <= 4; i++) {                                /* teeth over a dark mouth */
        int x = 32 + i * 2;
        cv_px(x, 45, 0); cv_px(x, 46, heat ? (uint8_t)(R_FLAME + 6 + heat * 3) : 0);
        cv_px(x - 1, 44, sh(R_BONE, 0.9f)); cv_px(x - 1, 47, sh(R_BONE, 0.7f));
    }
    cv_outline(0);
    cv_store(id);
}

/* A big black spider: the red eyes are all you see of it in its web until it drops */
static void spr_spider(int id, int frame) {
    cv_begin(cv_sprite, TS, TS);
    float cx = 32, cy = 36;
    if (frame == 3) cy = 52;                                       /* dead on its back */
    for (int s = -1; s <= 1; s += 2)
        for (int l = 0; l < 4; l++) {                              /* legs: up, out and down, in two joints */
            float a = (l - 1.5f) * 0.42f + (frame == 1 ? (l & 1 ? 0.18f : -0.18f) : 0.0f) + (frame == 2 ? -0.35f : 0.0f);
            float kx = cx + s * (9 + l * 1.5f), ky = cy - 4 + l * 3;
            float mx = cx + s * (17 + 3 * cosf(a)), my = cy - 12 + l * 4 + 6 * sinf(a);
            float fx = cx + s * (23 + l * 1.2f), fy = cy + 8 + l * 4 - (frame == 2 && l == 0 ? 14 : 0);
            if (frame == 3) { mx = cx + s * (8 + l * 2); my = cy - 10 - l * 2; fx = cx + s * (5 + l); fy = cy - 14 - l; }
            cv_capsule(kx, ky, mx, my, 1.2f, R_IRON, 0.05f, 0.35f);
            cv_capsule(mx, my, fx, fy, 0.9f, R_IRON, 0.05f, 0.3f);
        }
    cv_ellipse(cx, cy + 6, 9.5f, 9.0f, R_IRON, 0.04f, 0.38f);     /* abdomen, with a red mark */
    cv_flat(cx, cy + 7, 2.2f, 3.0f, (uint8_t)(R_BLOOD + 9)); cv_flat(cx, cy + 3, 1.4f, 1.4f, (uint8_t)(R_BLOOD + 7));
    cv_ellipse(cx, cy - 4, 6.0f, 5.0f, R_IRON, 0.05f, 0.42f);     /* head */
    if (frame != 3) {
        static const int EYE[8][2] = { {-3, -6}, {3, -6}, {-1, -7}, {1, -7}, {-4, -4}, {4, -4}, {-2, -5}, {2, -5} };
        for (int i = 0; i < 8; i++) cv_px((int)cx + EYE[i][0], (int)cy + EYE[i][1], i < 4 ? (uint8_t)(R_FLAME + 3) : (uint8_t)(R_FLAME + 1));
        cv_capsule(cx - 2, cy - 1, cx - 3, cy + (frame == 2 ? 4 : 2), 0.8f, R_BONE, 0.4f, 0.8f);   /* fangs */
        cv_capsule(cx + 2, cy - 1, cx + 3, cy + (frame == 2 ? 4 : 2), 0.8f, R_BONE, 0.4f, 0.8f);
    }
    cv_outline(0);
    cv_store(id);
}

/* projectiles and the floor burning */
static void spr_ball(int id, float r, int ramp, int dark, int frame, int skull) {
    cv_begin(cv_sprite, TS, TS);
    for (int y = 0; y < TS; y++)
        for (int x = 0; x < TS; x++) {
            float dx = x + 0.5f - 32, dy = y + 0.5f - 32, d = sqrtf(dx * dx + dy * dy);
            float n = fbm(x / 3.0f + frame * 5.0f, y / 3.0f, 32, 61 + ramp) * 0.6f, rr = r * (0.85f + n);
            if (d > rr) continue;
            float heat = 1.05f - d / rr * 0.75f;
            uint8_t c = (uint8_t)(ramp + clampi((int)(heat * 15), 3, 15));
            if (dark && n > 0.42f && d > rr * 0.4f) c = (uint8_t)(dark + 2 + (int)(n * 6));
            cv_px(x, y, c);
        }
    if (skull) {                                                   /* a skull face in the flames */
        cv_flat(28, 30, 2.4f, 2.6f, R_BLOOD + 2); cv_flat(36, 30, 2.4f, 2.6f, R_BLOOD + 2);
        cv_flat(32, 35, 1.0f, 1.4f, R_BLOOD + 2);
        for (int i = -2; i <= 2; i++) cv_px(32 + i * 2, 39, R_BLOOD + 3);
    }
    for (int i = 0; i < 10; i++) {
        float a = h01(i, frame, 7 + ramp) * TAU_F, l = r + 2 + h01(i, frame + 3, 7) * 5;
        cv_px((int)(32 + cosf(a) * l), (int)(32 + sinf(a) * l), (uint8_t)(ramp + 8 + (i & 3)));
    }
    cv_store(id);
}

static void spr_frost(void) {
    cv_begin(cv_sprite, TS, TS);
    for (int k = 0; k < 5; k++) {                                  /* a cluster of ice shards */
        float a = k * TAU_F / 5 + 0.3f, l = 6 + (k & 1) * 3;
        float tri[6] = { 32 + cosf(a) * l, 32 + sinf(a) * l, 32 + cosf(a + 0.5f) * 2, 32 + sinf(a + 0.5f) * 2, 32 + cosf(a - 0.5f) * 2, 32 + sinf(a - 0.5f) * 2 };
        cv_poly(tri, 3, R_GLOW, 1.0f, 0.55f, 32 - l, 2 * l);
    }
    cv_flat(32, 32, 2.5f, 2.5f, C_WHITE);
    cv_store(SP_FROST);
}

static void spr_spear(void) {
    cv_begin(cv_sprite, TS, TS);
    for (int y = 0; y < TS; y++)                                   /* a sharpened bone, seen point-first, with a pale trail */
        for (int x = 0; x < TS; x++) {
            float dx = (x + 0.5f - 32) / 4.5f, dy = (y + 0.5f - 32) / 4.5f, d = dx * dx + dy * dy;
            if (d < 1.0f) cv_px(x, y, sh(R_BONE, 0.45f + 0.5f * lit(dx, dy, sqrtf(1.0f - d))));
            else if (d < 2.6f && h01(x, y, 3) < 0.5f) cv_px(x, y, sh(R_BONE, 0.25f));
        }
    cv_flat(31, 31, 1.2f, 1.2f, sh(R_BONE, 1.0f));
    cv_store(SP_SPEAR);
}

static void spr_wisp(void) {
    cv_begin(cv_sprite, TS, TS);
    for (int y = 0; y < TS; y++)
        for (int x = 0; x < TS; x++) {
            float d = hypotf(x + 0.5f - 32, y + 0.5f - 32);
            if (d < 4.5f) cv_px(x, y, (uint8_t)(R_GLOW + clampi((int)(15 - d * 2.2f), 4, 15)));
        }
    for (int i = 0; i < 8; i++) cv_px(32 + (int)((h01(i, 1, 9) - 0.5f) * 14), 32 + (int)((h01(i, 2, 9) - 0.5f) * 14), (uint8_t)(R_GLOW + 9));
    cv_store(SP_WISP);
}

static void spr_spark(void) {
    cv_begin(cv_sprite, TS, TS);
    for (int k = -5; k <= 5; k++) { cv_px(32 + k, 32, (uint8_t)(R_FLAME + 15 - abs(k))); cv_px(32, 32 + k, (uint8_t)(R_FLAME + 15 - abs(k))); }
    for (int k = -3; k <= 3; k++) { cv_px(32 + k, 32 + k, (uint8_t)(R_FLAME + 12 - abs(k))); cv_px(32 + k, 32 - k, (uint8_t)(R_FLAME + 12 - abs(k))); }
    cv_store(SP_SPARK);
}

/* the Blight Flask (docs/PLAN-b11.md, section 10): a squat stoppered flask, green venom behind the glass, turning
 * as it flies */
static void spr_flask(void) {
    cv_begin(cv_sprite, TS, TS);
    for (int y = 0; y < TS; y++)
        for (int x = 0; x < TS; x++) {
            float dx = (x + 0.5f - 32) / 6.5f, dy = (y + 0.5f - 34) / 6.0f, d = dx * dx + dy * dy;
            if (d < 1.0f) {                                         /* the belly: glass over the venom */
                float l = lit(dx, dy, sqrtf(1.0f - d));
                uint8_t c = dy > -0.35f ? sh(R_MOSS, 0.35f + 0.55f * l) : sh(R_GHOST, 0.25f + 0.4f * l);
                if (dx < -0.3f && dy < -0.1f && d > 0.35f && d < 0.6f) c = (uint8_t)(R_GHOST + 14);   /* a gleam */
                if (dy > 0.1f && h01(x, y, 5) < 0.08f) c = (uint8_t)(R_MOSS + 14);                     /* a bubble */
                cv_px(x, y, c);
            } else if (d < 1.25f) cv_px(x, y, sh(R_GHOST, 0.2f));
        }
    for (int y = 23; y < 28; y++) for (int x = 30; x < 35; x++) cv_px(x, y, sh(R_GHOST, 0.3f + 0.1f * (x - 30)));   /* the neck */
    for (int y = 20; y < 24; y++) for (int x = 29; x < 36; x++) cv_px(x, y, sh(R_WOOD, 0.35f + 0.08f * (x - 29)));   /* the stopper */
    cv_store(SP_FLASK);
}

/* the ultimates (docs/PLAN-b11.md, sections 9-10) */
static void spr_orb(int id, int frame) {                           /* Amperage: a ball of lightning, threads crackling off it */
    cv_begin(cv_sprite, TS, TS);
    for (int y = 0; y < TS; y++)
        for (int x = 0; x < TS; x++) {
            float dx = x + 0.5f - 32, dy = y + 0.5f - 32, d = sqrtf(dx * dx + dy * dy);
            float n = fbm(x / 2.5f + frame * 7.0f, y / 2.5f, 32, 83) * 0.35f, rr = 9.0f * (0.9f + n);
            if (d > rr) continue;
            if (d < 3.2f) { cv_px(x, y, C_WHITE); continue; }
            float sw = sinf(atan2f(dy, dx) * 3.0f + d * 0.7f + frame * 2.0f + n * 9.0f);   /* swirling */
            cv_px(x, y, sh(R_GOLD, clampf(1.0f - d / rr * 0.55f + 0.2f * sw, 0.35f, 1.0f)));
        }
    for (int k = 0; k < 6; k++) {                                  /* the threads, jagged */
        float a = h01(k, frame, 41) * TAU_F, x = 32 + cosf(a) * 7.0f, y = 32 + sinf(a) * 7.0f, l = 9.0f + 10.0f * h01(k, frame, 43);
        for (int s = 0; s < 5; s++) {
            float b = a + (h01(k * 5 + s, frame, 47) - 0.5f) * 1.4f, nx = x + cosf(b) * l / 5.0f, ny = y + sinf(b) * l / 5.0f;
            cv_line(x, y, nx, ny, s < 2 ? C_WHITE : sh(R_GOLD, 0.9f - 0.1f * s));
            x = nx; y = ny;
        }
    }
    cv_store(id);
}
static void spr_lance(void) {                                      /* Winter's Lance: a long star of ice */
    cv_begin(cv_sprite, TS, TS);
    for (int k = 0; k < 6; k++) {
        float a = k * TAU_F / 6.0f + 0.26f, l = k & 1 ? 11.0f : 17.0f;
        float tri[6] = { 32 + cosf(a) * l, 32 + sinf(a) * l, 32 + cosf(a + 0.42f) * 3.5f, 32 + sinf(a + 0.42f) * 3.5f,
                         32 + cosf(a - 0.42f) * 3.5f, 32 + sinf(a - 0.42f) * 3.5f };
        cv_poly(tri, 3, R_GLOW, 0.45f, 1.0f, 32 - l, 2 * l);
    }
    cv_flat(32, 32, 4.0f, 4.0f, sh(R_GLOW, 0.95f));
    cv_flat(32, 32, 2.2f, 2.2f, C_WHITE);
    for (int i = 0; i < 14; i++) {                                 /* frost streaming off it */
        float a = h01(i, 1, 51) * TAU_F, l = 6.0f + 16.0f * h01(i, 2, 51);
        cv_px((int)(32 + cosf(a) * l), (int)(32 + sinf(a) * l), i & 1 ? C_WHITE : sh(R_GLOW, 0.7f));
    }
    cv_store(SP_LANCE);
}
static void spr_ballista(void) {                                   /* the Ballista's spear, point first, its head bound in iron */
    cv_begin(cv_sprite, TS, TS);
    for (int y = 0; y < TS; y++)
        for (int x = 0; x < TS; x++) {
            float dx = (x + 0.5f - 32) / 7.5f, dy = (y + 0.5f - 32) / 7.5f, d = dx * dx + dy * dy;
            if (d < 0.55f) cv_px(x, y, sh(R_BONE, 0.5f + 0.5f * lit(dx, dy, sqrtf(1.0f - d))));
            else if (d < 1.0f) cv_px(x, y, sh(R_IRON, 0.25f + 0.6f * lit(dx, dy, sqrtf(1.0f - d)) + 0.1f * ((x + y) & 1)));   /* the binding */
            else if (d < 2.4f && h01(x, y, 5) < 0.4f) cv_px(x, y, sh(R_BONE, 0.3f));   /* the wind of it */
        }
    for (int k = 0; k < 4; k++) cv_px(32 + (k & 1 ? 5 : -5), 32 + (k & 2 ? 5 : -5), sh(R_IRON, 0.9f));   /* rivets */
    cv_flat(31.5f, 31.5f, 1.5f, 1.5f, sh(R_BONE, 1.0f));
    cv_store(SP_BALLISTA);
}
static void spr_fspirit(int id, int frame) {                       /* Firewall's flame spirits */
    cv_begin(cv_sprite, TS, TS);
    for (int y = 16; y < 50; y++) {
        float u = (float)(y - 16) / 34.0f, w = 9.0f * sinf(PI_F * fminf(1.0f, u * 1.15f)) * (0.5f + 0.5f * u);
        float wob = sinf(u * 9.0f + frame * 2.6f) * 2.0f * (1.0f - u);
        for (int x = (int)(32 + wob - w); x <= (int)(32 + wob + w); x++) {
            float e = fabsf(x + 0.5f - 32 - wob) / (w + 0.01f), heat = (1.0f - e) * 0.7f + u * 0.35f;
            if (u < 0.3f && h01(x, y + frame * 13, 9) < 0.3f * (1.0f - u / 0.3f)) continue;   /* its tip, licking */
            cv_px(x, y, (uint8_t)(R_FLAME + clampi((int)(heat * 15.0f), 3, 15)));
        }
    }
    cv_flat(29.0f, 36.0f, 1.4f, 2.2f, (uint8_t)(R_BLOOD + 1)); cv_flat(35.0f, 36.0f, 1.4f, 2.2f, (uint8_t)(R_BLOOD + 1));   /* hollow eyes */
    cv_store(id);
}
static void spr_rime(void) {                                       /* the Lance's rime: frost feathering out on the floor */
    cv_begin(cv_sprite, TS, TS);
    for (int y = 54; y <= 63; y++)
        for (int x = 6; x < 58; x++) {
            float nx = (x + 0.5f - 32) / 25.0f, ny = (y + 0.5f - 59.0f) / 4.5f, d = sqrtf(nx * nx + ny * ny);
            if (d > 1.0f) continue;
            float f = fbm(x / 3.0f, y / 1.5f, 32, 97);
            if (f > 0.42f + 0.25f * d) cv_px(x, y, f > 0.62f ? C_WHITE : sh(R_GLOW, 0.35f + 0.5f * f));
            else if (h01(x, y, 99) < 0.12f * (1.0f - d)) cv_px(x, y, sh(R_GLOW, 0.8f));
        }
    cv_store(SP_RIME);
}

static void spr_mote(void) {                                       /* a soul where the dead fell (section 12) */
    cv_begin(cv_sprite, TS, TS);
    for (int y = 18; y < 44; y++) {
        float u = (float)(y - 18) / 26.0f, w = 6.0f * sinf(PI_F * fminf(1.0f, 0.15f + u * 0.95f)) * (0.35f + 0.65f * u);
        for (int x = (int)(32 - w); x <= (int)(32 + w); x++) {
            float e = fabsf(x + 0.5f - 32) / (w + 0.01f);
            if (u < 0.35f && h01(x, y, 61) < 0.25f) continue;      /* its tip, wavering */
            cv_px(x, y, (uint8_t)(R_GHOST + clampi((int)((1.0f - e * 0.7f) * 13.0f + u * 3.0f), 4, 15)));
        }
    }
    cv_px(30, 34, (uint8_t)(R_GHOST + 3)); cv_px(34, 34, (uint8_t)(R_GHOST + 3));   /* two dim eyes */
    cv_px(31, 37, (uint8_t)(R_GHOST + 5)); cv_px(32, 37, (uint8_t)(R_GHOST + 5)); cv_px(33, 37, (uint8_t)(R_GHOST + 5));   /* a mouth, open */
    for (int i = 0; i < 6; i++) cv_px((int)(32 + (h01(i, 3, 71) - 0.5f) * 18), (int)(26 + h01(i, 4, 71) * 20), (uint8_t)(R_GHOST + 12));
    cv_store(SP_MOTE);
}

static void spr_burn(int id, int frame) {
    cv_begin(cv_sprite, TS, TS);
    for (int i = 0; i < 9; i++) {
        float cx = 8 + i * 6 + (h01(i, frame, 3) - 0.5f) * 3, h = 8 + 10 * h01(i, frame + 7, 3);
        for (int y = (int)(64 - h); y < 64; y++)
            for (int x = (int)(cx - 3); x <= (int)(cx + 3); x++) {
                float u = (y - (64 - h)) / h, w = 3.0f * u;
                if (fabsf(x + 0.5f - cx) > w) continue;
                cv_px(x, y, (uint8_t)(R_FLAME + clampi((int)(4 + u * 11), 2, 15)));
            }
    }
    cv_store(id);
}

static void spr_soulurn(void) {
    cv_begin(cv_sprite, TS, TS);
    cv_flat(32, 62, 12, 2.0f, R_WARM + 1);
    for (int y = 34; y <= 61; y++)                                 /* a squat funerary urn */
        for (int x = 18; x <= 46; x++) {
            float u = (x + 0.5f - 32) / (9.0f + 4.0f * sinf((y - 34) / 27.0f * 3.1f)), v = (y - 34) / 27.0f;
            if (fabsf(u) > 1.0f) continue;
            float l = lit(u, v - 0.5f, sqrtf(fmaxf(0.0f, 1 - u * u)));
            uint8_t c = sh(R_WARM, 0.2f + 0.55f * l);
            if (y == 40 || y == 41) c = sh(R_GOLD, 0.3f + 0.6f * l);
            cv_px(x, y, c);
        }
    for (int y = 30; y < 35; y++) for (int x = 26; x <= 38; x++) cv_px(x, y, sh(R_WARM, 0.35f + (y == 30 ? 0.3f : 0.0f)));
    cv_outline(0);
    for (int i = 0; i < 12; i++)                                   /* a soul seeping out of the lid */
        cv_px(32 + (int)((h01(i, 1, 11) - 0.5f) * 10 + sinf(i * 0.9f) * 2), 29 - (int)(h01(i, 2, 11) * 14), (uint8_t)(R_GLOW + 8 + (i % 6)));
    cv_store(SP_SOULURN);
}

static void spr_ringitem(void) {
    cv_begin(cv_sprite, TS, TS);
    cv_flat(32, 62, 12, 2.0f, R_WARM + 1);
    cv_ellipse(32, 57, 13, 5.5f, R_PURPLE, 0.15f, 0.7f);           /* a velvet cushion */
    for (int a = 0; a < 64; a++) {                                 /* the ring, standing on it */
        float t = a / 64.0f * TAU_F;
        for (float w = -1.2f; w <= 1.2f; w += 0.6f) cv_px((int)(32 + cosf(t) * (6 + w)), (int)(46 + sinf(t) * (6 + w)), sh(R_GOLD, 0.5f + 0.45f * cosf(t + 2.3f)));
    }
    gem(32, 39, 3.0f, R_BLOOD);
    cv_outline(0);
    cv_px(30, 37, C_WHITE);
    cv_store(SP_RINGITEM);
}

/* the merchant's pedestals and wares; anything in R_PURPLE takes the colour of what is on offer */
static void spr_pedestal(void) {
    cv_begin(cv_sprite, TS, TS);
    for (int y = 30; y < 64; y++)
        for (int x = 14; x < 50; x++) {
            int cap = y < 36 || y >= 60;
            float hw = cap ? 17.5f : 13.0f - (y < 40 ? 0.0f : 0.0f);
            if (fabsf(x + 0.5f - 32) > hw) continue;
            float u = (x + 0.5f - 32) / hw;
            float v = 0.25f + 0.5f * lit(u, cap ? -0.6f : 0.0f, sqrtf(fmaxf(0.0f, 1 - u * u))) + (y == 30 ? 0.2f : 0.0f) - (y == 35 || y == 59 ? 0.12f : 0.0f);
            if (!cap && (x - 14) % 7 == 0) v -= 0.08f;
            cv_px(x, y, sh(R_STONE, v));
        }
    cv_outline(0);
    cv_store(SP_PEDESTAL);
}

static void spr_items(void) {
    /* a spellbook, open, pages glowing */
    cv_begin(cv_sprite, TS, TS);
    for (int s = -1; s <= 1; s += 2) {
        float cover[8] = { 32, 42, 32 + s * 22, 38, 32 + s * 22, 24, 32, 28 };
        cv_poly(cover, 4, R_PURPLE, 0.75f, 0.35f, 24, 18);
        float page[8] = { 32, 40, 32 + s * 19, 36, 32 + s * 19, 24, 32, 27 };
        cv_poly(page, 4, R_BONE, 0.95f, 0.7f, 24, 16);
        for (int l = 0; l < 4; l++) cv_line(32 + s * 4, 30 + l * 2.5f, 32 + s * 16, 28 + l * 2.5f, sh(R_PURPLE, 0.6f));
    }
    cv_line(32, 27, 32, 41, sh(R_PURPLE, 0.2f));
    cv_outline(0);
    for (int i = 0; i < 10; i++) cv_px(20 + (int)(h01(i, 1, 21) * 24), 10 + (int)(h01(i, 2, 21) * 14), sh(R_PURPLE, 1.0f));
    cv_store(SP_ITEM_TOME);
    /* a rune stone */
    cv_begin(cv_sprite, TS, TS);
    for (int y = 14; y < 46; y++)
        for (int x = 18; x < 46; x++) {
            float u = (x + 0.5f - 32) / 14.0f, v = (y + 0.5f - 30) / 16.0f;
            if (u * u * u * u + v * v * v * v > 1.0f) continue;
            cv_px(x, y, sh(R_STONE, 0.3f + 0.4f * lit(u, v, 0.6f) + fbm(x / 4.0f, y / 4.0f, 16, 23) * 0.15f));
        }
    static const char *RUNE[9] = { "..#..", ".###.", "#.#.#", "..#..", ".#.#.", "#...#", "..#..", "..#..", "..#.." };
    for (int y = 0; y < 9; y++) for (int x = 0; x < 5; x++) if (RUNE[y][x] == '#') { cv_px(27 + x * 2, 21 + y * 2, sh(R_PURPLE, 1.0f)); cv_px(28 + x * 2, 21 + y * 2, sh(R_PURPLE, 0.85f)); cv_px(27 + x * 2, 22 + y * 2, sh(R_PURPLE, 0.85f)); }
    cv_outline(0);
    cv_store(SP_ITEM_RUNE);
    /* a robe, folded */
    cv_begin(cv_sprite, TS, TS);
    for (int y = 26; y < 46; y++)
        for (int x = 12; x < 52; x++) {
            float u = (x + 0.5f - 32) / 20.0f, v = (y - 26) / 20.0f;
            if (fabsf(u) > 1.0f - v * 0.1f) continue;
            float f = 0.35f + 0.35f * (1.0f - v) + 0.12f * sinf(y * 1.2f) - (((y - 26) % 6) == 0 ? 0.15f : 0.0f);
            cv_px(x, y, sh(R_PURPLE, f));
        }
    for (int x = 13; x < 51; x++) cv_px(x, 33, sh(R_GOLD, 0.8f));
    cv_outline(0);
    cv_store(SP_ITEM_ROBE);
    /* a hood, standing up as if worn by no one */
    cv_begin(cv_sprite, TS, TS);
    for (int y = 10; y < 48; y++)
        for (int x = 12; x < 52; x++) {
            float u = (x + 0.5f - 32) / (10.0f + (y - 10) * 0.28f), v = (y - 10) / 38.0f;
            if (fabsf(u) > 1.0f || (y < 16 && fabsf(u) > (y - 9) / 7.0f)) continue;
            float hole = hypotf((x + 0.5f - 32) / 7.0f, (y + 0.5f - 28) / 10.0f);
            cv_px(x, y, hole < 1.0f ? 0 : sh(R_PURPLE, 0.2f + 0.5f * lit(u, v - 0.5f, sqrtf(fmaxf(0.0f, 1 - u * u)))));
        }
    cv_outline(0);
    cv_store(SP_ITEM_HOOD);
    /* a ring */
    cv_begin(cv_sprite, TS, TS);
    for (int a = 0; a < 90; a++) {
        float t = a / 90.0f * TAU_F;
        for (float w = -1.6f; w <= 1.6f; w += 0.5f) cv_px((int)(32 + cosf(t) * (9 + w)), (int)(34 + sinf(t) * (9 + w) * 0.8f), sh(R_GOLD, 0.45f + 0.5f * cosf(t + 2.3f)));
    }
    gem(32, 25, 4.5f, R_PURPLE);
    cv_outline(0);
    cv_px(30, 22, C_WHITE);
    cv_store(SP_ITEM_RING);
    /* a bracer, lying on its side: a cuff of cloth and leather, two straps, studs */
    cv_begin(cv_sprite, TS, TS);
    for (int y = 20; y < 46; y++)
        for (int x = 18; x < 47; x++) {
            float hh = 7.0f + 2.0f * (x - 18) / 28.0f, v = (y + 0.5f - 33) / hh;
            if (fabsf(v) > 1.0f) continue;
            float f = 0.2f + 0.65f * lit(0.0f, v, sqrtf(1.0f - v * v)) - ((x - 18) % 5 == 0 ? 0.06f : 0.0f);
            int strap = (x >= 24 && x <= 26) || (x >= 37 && x <= 39);
            cv_px(x, y, strap ? sh(R_WOOD, f * 0.8f) : sh(R_PURPLE, f));
        }
    for (int y = 25; y < 42; y++) cv_px(46, y, sh(R_IRON, 0.08f));   /* the open end */
    cv_px(25, 31, sh(R_GOLD, 0.95f)); cv_px(38, 30, sh(R_GOLD, 0.95f));
    cv_outline(0);
    cv_store(SP_ITEM_BRACER);
    /* boots, a pair, side by side */
    cv_begin(cv_sprite, TS, TS);
    for (int b = 0; b < 2; b++) {
        float dx = b ? 0.0f : -7.0f, dy = b ? 0.0f : -2.0f, lo = b ? 0.75f : 0.5f, hi = b ? 0.3f : 0.15f;
        float boot[14] = { 21 + dx, 16 + dy, 31 + dx, 16 + dy, 31 + dx, 36 + dy, 42 + dx, 38 + dy, 44 + dx, 42 + dy, 44 + dx, 46 + dy, 21 + dx, 46 + dy };
        cv_poly(boot, 7, R_PURPLE, lo, hi, 16 + dy, 30);
        cv_line(21 + dx, 46 + dy, 44 + dx, 46 + dy, sh(R_IRON, 0.25f));
        cv_line(21 + dx, 19 + dy, 31 + dx, 19 + dy, sh(R_PURPLE, b ? 0.95f : 0.7f));
        cv_line(24 + dx, 46 + dy, 24 + dx, 41 + dy, sh(R_IRON, 0.3f));
    }
    cv_outline(0);
    cv_store(SP_ITEM_BOOTS);
    /* an amulet: a chain, a gold rim, a stone in the colour of what it is */
    cv_begin(cv_sprite, TS, TS);
    for (int i = 0; i <= 12; i++) {
        float t = i / 12.0f;
        cv_px((int)(20 + 10 * t), (int)(12 + 16 * t * (2 - t)), sh(R_IRON, 0.7f - 0.3f * (i & 1)));
        cv_px((int)(44 - 10 * t), (int)(12 + 16 * t * (2 - t)), sh(R_IRON, 0.7f - 0.3f * (i & 1)));
    }
    cv_ellipse(32, 37, 8.5f, 9.5f, R_GOLD, 0.35f, 0.95f);
    gem(32, 37, 6.0f, R_PURPLE);
    cv_outline(0);
    cv_px(30, 33, C_WHITE);
    cv_store(SP_ITEM_AMULET);
    /* a bag, tied at the neck */
    cv_begin(cv_sprite, TS, TS);
    cv_ellipse(32, 38, 13.5f, 10.5f, R_PURPLE, 0.15f, 0.85f);
    float neck[8] = { 28, 29, 36, 29, 40, 20, 24, 20 };
    cv_poly(neck, 4, R_PURPLE, 0.75f, 0.35f, 20, 10);
    for (int x = 27; x <= 37; x++) { cv_px(x, 28, sh(R_WOOD, 0.6f)); cv_px(x, 29, sh(R_WOOD, 0.35f)); }
    cv_line(37, 29, 41, 34, sh(R_WOOD, 0.5f));
    for (int k = 0; k < 4; k++) cv_line(26 + k * 4, 20, 25 + k * 4.4f, 17, sh(R_PURPLE, 0.6f));   /* the gathered cloth */
    cv_outline(0);
    cv_store(SP_ITEM_BAG);
    /* a long bone on the floor, for the spear */
    cv_begin(cv_sprite, TS, TS);
    cv_bone(20, 58, 44, 54, 2.2f, 0.35f, 0.95f);
    cv_outline(0);
    cv_store(SP_ITEM_BONE);
}

/* The Black Altar: a squat block of black stone under a heavier slab, a rune cut in its face, an iron
 * bowl on top. Its fire is drawn apart (and at full brightness), violet or gold, two frames each. */
static void spr_black_altar(void) {
    cv_begin(cv_sprite, TS, TS);
    for (int y = 40; y < 64; y++)                                   /* the block */
        for (int x = 17; x < 47; x++) {
            float u = (x + 0.5f - 32) / 15.0f;
            float v = 0.12f + 0.22f * lit(u, -0.2f, sqrtf(fmaxf(0.0f, 1.0f - u * u))) + 0.05f * fbm(x / 3.0f, y / 3.0f, 8, 41);
            cv_px(x, y, sh(R_IRON, v));
        }
    for (int y = 36; y < 41; y++)                                   /* the slab over it */
        for (int x = 13; x < 51; x++) cv_px(x, y, sh(R_IRON, y == 36 ? 0.45f : 0.22f + 0.04f * fbm(x / 2.0f, y, 8, 43)));
    static const char *RUNE[9] = { "#...#", "##.##", "#.#.#", "..#..", "..#..", ".#.#.", "#...#", "#...#", "#...#" };
    for (int y = 0; y < 9; y++)                                     /* a rune cut in the face, faintly violet */
        for (int x = 0; x < 5; x++) if (RUNE[y][x] == '#') { cv_px(28 + x * 2, 44 + y * 2, sh(R_PURPLE, 0.35f)); cv_px(29 + x * 2, 44 + y * 2, sh(R_PURPLE, 0.25f)); }
    for (int x = 24; x < 41; x++) {                                 /* the iron bowl */
        float u = (x + 0.5f - 32) / 8.5f, d = 1.0f - u * u;
        if (d <= 0.0f) continue;
        for (int y = 31; y < 31 + (int)(5.0f * d) + 1; y++) cv_px(x, y, sh(R_IRON, 0.3f + 0.3f * (y == 31)));
    }
    cv_outline(0);
    cv_store(SP_BALTAR);
    for (int c = 0; c < 2; c++)                                     /* the fire: violet, then gold */
        for (int fr = 0; fr < 2; fr++) {
            cv_begin(cv_sprite, TS, TS);
            int ramp = c ? R_GOLD : R_PURPLE;
            for (int y = 13; y < 32; y++)
                for (int x = 24; x < 41; x++) {
                    float h = (32 - y) / 19.0f;                     /* 0 at the bowl, 1 at the tip */
                    float sway = 1.6f * sinf(h * 6.0f + fr * 2.6f) * h;
                    float w = (6.2f - 5.4f * h) * (0.8f + 0.2f * sinf(y * 1.7f + fr * 3.0f));
                    float dx = fabsf(x + 0.5f - 32 - sway);
                    if (dx > w || (dx > w * 0.55f && h01(x, y + fr * 7, 47 + c) < 0.3f)) continue;   /* ragged at the edges */
                    float core = 1.0f - dx / w;
                    if (core > 0.65f && h < 0.45f) { cv_px(x, y, C_WHITE); continue; }
                    cv_px(x, y, sh(ramp, fminf(1.0f, 0.3f + 0.65f * core * (1.0f - 0.4f * h))));
                }
            cv_store(c ? SP_BFLAME_G0 + fr : SP_BFLAME_V0 + fr);
        }
}

const char *const RUNE_GLYPH[9][7] = {                             /* by DF_*: ward, blink, mirror, mend, purify, nova, dread, slip, reap */
    { "#.#.#", "#.#.#", ".###.", "..#..", "..#..", "..#..", "..#.." },   /* algiz: the ward */
    { "##...", "#.#..", "#..#.", "#.#..", "##...", "#.#..", "#..#." },   /* raido: the journey */
    { "#...#", "##.##", "#.#.#", "#...#", "#...#", "#...#", "#...#" },   /* mannaz: the mirror */
    { "##...", "#.#..", "#.#..", "##...", "#.#..", "#.#..", "##..." },   /* berkano: healing */
    { "#....", "##...", "#.#..", "#....", "#....", "#....", "#...." },   /* laguz: water, cleansing */
    { "#.#.#", ".###.", "#####", ".###.", "#.#.#", "..#..", "..#.." },   /* frost, a star */
    { "#....", "##...", "#.#..", "#..#.", "#.#..", "##...", "#...." },   /* thurisaz: the thorn, dread */
    { "#...#", "##.##", "#.#.#", "#...#", "#.#.#", "##.##", "#...#" },   /* dagaz: the turning of time */
    { "###..", "#.#..", "#..#.", "#.#..", "###..", "#.#..", "#..#." },   /* perthro: lots, fate, souls */
};

/* The elements' glyphs (docs/PLAN-b11.md, section 2), 7 by 7, by EL_*: '#' in the element's colour, 'o' its
 * bright core. On the target's readout, the bestiary, the Alignment and the status bar. */
const char *const ELEM_GLYPH[6][7] = {
    { "...#...", "..##...", ".###.#.", ".##o##.", "##ooo##", "##ooo##", ".#####." },   /* a flame */
    { "...#...", "#..#..#", ".#.#.#.", "..#o#..", ".#.#.#.", "#..#..#", "...#..." },   /* a six-armed flake */
    { "....###", "...###.", "..###..", ".######", "...##..", "..#.#..", ".#...#." },   /* a forked bolt */
    { "...#...", "...#...", "..###..", ".##o##.", "##o.o##", "###o###", ".#####." },   /* a drop with a bubble in it */
    { "...#...", "...##..", "..###..", ".#####.", ".#o#o#.", ".#ooo#.", "..###.." },   /* a wisp's teardrop flame, looking out */
    { ".......", "#.....#", "##...##", ".#####.", "##...##", "#.....#", "......." },   /* a bone */
};
/* The runes' marks for their sockets (section 3), 11 by 11, by DF_*: the same two tones */
const char *const RUNE_MARK[9][11] = {
    { "##.......##", "###.....###", ".###...###.", "...##.##...", "....###....", ".....#.....",
      "....###....", "...##.##...", ".###...###.", "###.....###", "##.......##" },   /* crossed bones */
    { "...........", "...#####...", ".##.....##.", "#...ooo...#", ".##.ooo.##.", "...#####...",
      "...........", "........#..", ".#########.", "........#..", "..........." },   /* an eye, and a step away */
    { "..####.....", ".#oooo#....", "#o#oooo#...", "#oo#oooo#..", "#ooooooo#..", ".#ooooo#...",
      "..#####....", "......#....", ".......#...", "........##.", ".........##" },   /* a hand mirror */
    { "...........", ".###...###.", "#####.#####", "####ooo####", "#####.#####", ".###ooo###.",
      "..###.###..", "...#ooo#...", "....#.#....", ".....#.....", "..........." },   /* a stitched heart */
    { "...#####...", "..#.....#..", ".#...#...#.", "#...###...#", "#..#####..#", "#..##o##..#",
      "#..#####..#", "#...###...#", ".#.......#.", "..#.....#..", "...#####..." },   /* a drop in a ring */
    { ".....#.....", ".#...#...#.", "..#..#..#..", "...#.#.#...", "....ooo....", "#####o#####",
      "....ooo....", "...#.#.#...", "..#..#..#..", ".#...#...#.", ".....#....." },   /* a burst */
    { "...#####...", "..#######..", ".#########.", ".##..#..##.", ".##..#..##.", ".#########.",
      "..###.###..", "...#####...", "...#.#.#...", "...#####...", "..........." },   /* a skull */
    { "###########", ".#ooooooo#.", "..#ooooo#..", "...#ooo#...", "....#o#....", ".....#.....",
      "....#.#....", "...#...#...", "..#..o..#..", ".#.ooooo.#.", "###########" },   /* an hourglass */
    { "..######...", ".#.....###.", ".........##", "..........#", ".........#.", "........#..",
      ".......#...", "......#....", ".....#.....", "....#......", "...#......." },   /* a scythe */
};

/* The classes' marks (docs/PLAN-b11.md, section 11), 11 by 11, by the class's element: the same two tones */
const char *const CLASS_MARK[6][11] = {
    { ".....#.....", "....###....", ".....#.....", "..#######..", ".....#.....", ".....#.....",
      ".#...#...#.", ".##.o#o.##.", ".#oo###oo#.", "##ooo#ooo##", "###########" },   /* the Witchfinder: a stake, a pyre */
    { "..........#", ".........##", "........##.", ".......##..", "......##...", ".....##....",
      "....##.....", "...#o#.....", "..#oo#.....", ".#oo#......", ".###......." },   /* the Anatomist: a scalpel */
    { ".#.......#.", ".#.......#.", ".#.......#.", ".#...#...#.", ".#..##...#.", ".#.#oo#..#.",
      ".#..#oo#.#.", ".#...##..#.", ".#...#...#.", "###.....###", "###.....###" },   /* the Galvanist: two rods, a spark */
    { "...#####...", "...#####...", ".#########.", "...........", "..####.....", ".#o##o#....",
      ".#######...", "..#######..", "....######.", "......####.", "........##." },   /* the Plague Doctor: hat, lenses, the beak */
    { "....ooo....", "...o#o#o...", "....ooo....", "...........", "..#######..", ".##.#.#.##.",
      "###.#.#.###", "###########", "###########", ".#########.", "..........." },   /* the Sin-Eater: a coin on scored bread */
    { ".####......", "#####.#....", "######o#...", "#######o#..", "######o#...", "#####.#....",
      ".####.#....", "......#....", "......#....", "......#....", "......#...." },   /* the Headsman: the axe */
};

/* The Rune Shrine: a stele with an arched head on two steps, a disc cut in its face. Sanctified, the
 * stone is pale and clean, the disc gilded with a four-pointed star, a white cloth with a gold hem laid
 * over the step and two candles cold. Cursed, the same stone gone dark with soot and cracked, the disc
 * broken and the star turned over, the cloth burnt to rags, the candles guttered black; what glows
 * (the cut channels, a column of small marks down each side, the flames) is drawn apart at full
 * brightness, in R_PURPLE that takes the shrine's rank's colour. */
static int shrine_stele(int x, int y) {                             /* inside the stele's outline? */
    if (x < 21 || x > 42 || y > 50) return 0;
    if (y >= 22) return y >= 12;
    float dx = x + 0.5f - 32.0f, dy = y + 0.5f - 22.0f;
    return dx * dx + dy * dy <= 11.0f * 11.0f;
}
static void spr_shrine_body(int cursed) {
    cv_begin(cv_sprite, TS, TS);
    for (int y = 0; y < TS; y++)                                    /* the stele */
        for (int x = 0; x < TS; x++) {
            if (!shrine_stele(x, y)) continue;
            float u = (x + 0.5f - 32.0f) / 11.0f;
            float v = 0.38f + 0.3f * lit(u, -0.3f, sqrtf(fmaxf(0.0f, 1.0f - u * u))) + 0.06f * fbm(x / 4.0f, y / 4.0f, 8, 501);
            if (cursed) {
                v *= 0.55f;
                if (fbm(x / 3.0f, y / 6.0f, 8, 503) > 0.58f) v *= 0.6f;   /* soot running down */
            }
            cv_px(x, y, sh(R_STONE, v));
        }
    for (int y = 50; y < 64; y++)                                   /* two steps */
        for (int x = 8; x < 56; x++) {
            int top = y < 55;
            if (top && (x < 14 || x > 49)) continue;
            float v = (y == 50 || y == 55 ? 0.62f : 0.42f) + 0.05f * fbm(x / 3.0f, y / 2.0f, 8, 507);
            cv_px(x, y, sh(R_STONE, cursed ? v * 0.5f : v));
        }
    if (!cursed) {
        for (int y = 0; y < TS; y++)                                /* the disc, gilded, and its star */
            for (int x = 0; x < TS; x++) {
                float dx = x + 0.5f - 32.0f, dy = y + 0.5f - 27.0f, r = sqrtf(dx * dx + dy * dy);
                if (r > 6.2f && r < 7.6f) cv_px(x, y, sh(R_GOLD, 0.55f + 0.35f * (dy < 0)));
                float ax = fabsf(dx), ay = fabsf(dy);
                if ((ax < 1.0f && ay < 6.0f) || (ay < 1.0f && ax < 6.0f) || (ax + ay < 2.6f)) cv_px(x, y, sh(R_GOLD, 0.7f + 0.25f * (dy < 0)));
            }
        for (int x = 15; x <= 48; x++) {                            /* the cloth, falling over the step to a point */
            int drop = 6 - abs(x - 32) / 4;
            for (int y = 49; y <= 52 + drop; y++) cv_px(x, y, sh(R_BONE, y == 49 ? 0.95f : 0.8f - 0.05f * (y - 49) + 0.08f * ((x & 3) == 0)));
            cv_px(x, 53 + drop, sh(R_GOLD, 0.7f));                  /* its hem */
        }
        for (int s = -1; s <= 1; s += 2) {                          /* two candles, cold */
            int cx = 32 + s * 15;
            for (int y = 42; y < 50; y++) { cv_px(cx, y, sh(R_BONE, 0.85f)); cv_px(cx + 1, y, sh(R_BONE, 0.7f)); }
            cv_px(cx, 41, 0);
        }
    } else {
        for (int y = 0; y < TS; y++)                                /* the disc, broken: its cut channels dark */
            for (int x = 0; x < TS; x++) {
                float dx = x + 0.5f - 32.0f, dy = y + 0.5f - 27.0f, r = sqrtf(dx * dx + dy * dy);
                float a = atan2f(dy, dx);
                if (r > 6.2f && r < 7.6f && !(a > 0.9f && a < 1.5f)) cv_px(x, y, sh(R_STONE, 0.05f));
            }
        static const float CRACK[][4] = { { 26, 14, 23, 30 }, { 23, 30, 25, 44 }, { 38, 33, 41, 47 }, { 35, 15, 37, 21 } };
        for (int i = 0; i < 4; i++) cv_line(CRACK[i][0], CRACK[i][1], CRACK[i][2], CRACK[i][3], sh(R_STONE, 0.02f));
        for (int x = 16; x <= 47; x++) {                            /* what is left of the cloth, burnt */
            if (h01(x, 0, 509) < 0.35f) continue;
            int drop = (int)(4 * h01(x, 1, 509));
            for (int y = 49; y <= 50 + drop; y++) cv_px(x, y, sh(y == 50 + drop ? R_WARM : R_BONE, y == 50 + drop ? 0.08f : 0.35f));
        }
        for (int s = -1; s <= 1; s += 2) {                          /* the candles, burnt down black */
            int cx = 32 + s * 15;
            for (int y = 45; y < 50; y++) { cv_px(cx, y, sh(R_IRON, 0.2f)); cv_px(cx + 1, y, sh(R_IRON, 0.12f)); }
            cv_px(cx - 1, 49, sh(R_IRON, 0.15f)); cv_px(cx + 2, 49, sh(R_IRON, 0.1f));   /* the wax run down */
        }
        for (int y = 33; y < 49; y++) if (h01(31, y, 511) < 0.8f) cv_px(31 + (y > 40), y, sh(R_BLOOD, 0.3f));   /* a dark run from the disc */
    }
    cv_outline(0);
    cv_store(cursed ? SP_SHRINE_C : SP_SHRINE_S);
}
static void spr_shrine_glow(int fr) {
    cv_begin(cv_sprite, TS, TS);
    for (int y = 0; y < TS; y++)                                    /* the broken disc and the star turned over */
        for (int x = 0; x < TS; x++) {
            float dx = x + 0.5f - 32.0f, dy = y + 0.5f - 27.0f, r = sqrtf(dx * dx + dy * dy), a = atan2f(dy, dx);
            float ax = fabsf(dx);
            if (r > 6.4f && r < 7.4f && !(a > 0.9f && a < 1.5f)) cv_px(x, y, sh(R_PURPLE, 0.75f + 0.2f * fr));
            if ((ax < 0.9f && dy > -5.5f && dy < 5.5f) || (fabsf(dy - 2.5f) < 0.9f && ax < 3.6f))   /* the cross, its bar low */
                cv_px(x, y, sh(R_PURPLE, 0.95f));
        }
    for (int s = -1; s <= 1; s += 2)                                /* a column of small marks down each side */
        for (int k = 0; k < 5; k++) {
            int gx = 32 + s * 7 - 1, gy = 37 + k * 3 - 15 * (k > 4);
            if (gy > 48) break;
            cv_px(gx, gy, sh(R_PURPLE, 0.7f + 0.3f * ((k + fr) & 1))); cv_px(gx + (int)(h01(k, s + 2, 513) * 2.0f), gy + 1, sh(R_PURPLE, 0.6f));
        }
    for (int s = -1; s <= 1; s += 2) {                              /* the candle flames */
        int cx = 32 + s * 15;
        float sway = (fr ? 0.6f : -0.5f) * s;
        for (int y = 39; y < 45; y++) {
            float h = (45 - y) / 6.0f, w = 1.6f * (1.0f - h) + 0.3f;
            for (int x = cx - 2; x <= cx + 2; x++) {
                float d = fabsf(x + 0.5f - cx - 0.5f - sway * h);
                if (d > w) continue;
                cv_px(x, y, d < w * 0.45f && h < 0.6f ? C_WHITE : sh(R_FLAME, 0.55f + 0.4f * (1.0f - h)));
            }
        }
    }
    cv_store(fr ? SP_SHRINE_G1 : SP_SHRINE_G0);
}
/* a rune about the shrine: its mark, big in the frame (drawn small), in R_PURPLE with a soft rim;
 * k = NDEF for the plain mark it shows before it is attuned to any */
static void spr_srune(int k) {
    static const char *const PLAIN[7] = { "..#..", ".###.", "#.#.#", "..#..", "#.#.#", ".###.", "..#.." };
    const char *const *g = k < 9 ? RUNE_GLYPH[k] : PLAIN;
    cv_begin(cv_sprite, TS, TS);
    for (int pass = 0; pass < 2; pass++)
        for (int y = 0; y < 7; y++)
            for (int x = 0; x < 5; x++) {
                if (g[y][x] != '#') continue;
                int x0 = 17 + x * 6, y0 = 11 + y * 6;
                for (int yy = -1 + pass; yy < 7 - pass; yy++)
                    for (int xx = -1 + pass; xx < 7 - pass; xx++)
                        cv_px(x0 + xx, y0 + yy, pass ? sh(R_PURPLE, 1.0f - 0.25f * (float)(y0 + yy - 11) / 42.0f) : sh(R_PURPLE, 0.35f));
            }
    cv_store(k < 9 ? SP_SRUNE0 + k : SP_SRUNE_X);
}
/* a Cursed Coin, standing on its edge: old gold gone dark, a raised rim, green in the worn places, a
 * skull struck on its face with one red eye */
static void spr_ccoin(void) {
    cv_begin(cv_sprite, TS, TS);
    cv_flat(32, 61.5f, 9.5f, 1.6f, sh(R_WARM, 0.05f));
    for (int y = 0; y < TS; y++)
        for (int x = 0; x < TS; x++) {
            float dx = (x + 0.5f - 32.0f) / 10.0f, dy = (y + 0.5f - 50.0f) / 10.0f, d = dx * dx + dy * dy;
            if (d > 1.0f) continue;
            float v;
            if (d > 0.74f) v = 0.5f + 0.4f * lit(dx, dy, sqrtf(1.0f - d));   /* the rim, catching the light */
            else if (d > 0.6f) v = 0.08f;                          /* the groove inside it */
            else v = 0.22f + 0.12f * (-dx - dy);                    /* the dark field */
            uint8_t c = sh(R_GOLD, v);
            if (d <= 0.6f && fbm(x / 1.5f, y / 1.5f, 8, 515) > 0.66f) c = sh(R_MOSS, 0.25f);   /* verdigris */
            cv_px(x, y, c);
        }
    cv_ellipse(32, 48.5f, 3.6f, 3.3f, R_BONE, 0.4f, 0.8f);         /* the skull */
    cv_flat(32, 52.2f, 2.0f, 1.2f, sh(R_BONE, 0.5f));
    cv_px(30, 48, 0); cv_px(31, 48, 0); cv_px(33, 48, (uint8_t)(R_BLOOD + 14)); cv_px(34, 48, (uint8_t)(R_BLOOD + 12));   /* one eye dark, one red */
    cv_px(32, 50, 0);
    for (int k = 0; k < 3; k++) cv_px(31 + k, 53, 0);              /* teeth */
    cv_outline(0);
    cv_store(SP_CCOIN);
}

/* a drain low in the wall, barred, a dark stain down from it: where a cistern's bile comes out */
static void gen_grate(int t, uint32_t seed) {
    gen_stone(t, R_STONE, seed);
    for (int y = 38; y < 58; y++)
        for (int x = 14; x < 50; x++) {
            int edge = y == 38 || x == 14 || x == 49 || y == 57;
            uint8_t c = edge ? sh(R_STONE, 0.72f) : sh(R_STONE, 0.02f + 0.03f * h01(x, y, seed));
            if (!edge && (x - 16) % 6 < 2) c = sh(R_IRON, (x - 16) % 6 ? 0.35f : 0.6f);   /* the bars */
            if (!edge && y == 47) c = sh(R_IRON, 0.5f);
            tput(t, x, y, c);
        }
    for (int y = 58; y < TS; y++)                                   /* the stain, down to the floor */
        for (int x = 20; x < 44; x++)
            if (fbm(x / 3.0f, y / 6.0f, 16, seed + 3) > 0.42f) tput(t, x, y, sh(R_MOSS, 0.12f + 0.1f * h01(x, y, seed)));
}

/* ---- the runs' hazards (1.0.0b13, docs/PLAN-b13.md section 5): what stands in the wall, on plain stone ---- */

/* a burial slab set in the wall, its rings and its line of lettering; cracked, a seam of pale light round it */
static void gen_hz_slab(int t, uint32_t seed, int open) {
    gen_stone(t, R_STONE, seed);
    for (int y = 18; y < 46; y++)
        for (int x = 8; x < 56; x++) {
            int edge = y == 18 || y == 45 || x == 8 || x == 55;
            float v = 0.42f + 0.12f * fbm(x / 6.0f, y / 6.0f, 16, seed + 5) - (y > 40 ? 0.08f : 0.0f);
            uint8_t c = edge ? sh(R_STONE, 0.18f) : sh(R_BONE, v * 0.75f);
            if (!edge && y >= 28 && y <= 31 && x >= 16 && x < 48 && ((x / 3) & 1) && h01(x, y, seed) > 0.25f) c = sh(R_BONE, 0.12f);   /* the lettering */
            tput(t, x, y, c);
        }
    for (int r = 0; r < 2; r++) {                                      /* iron rings to draw it out by */
        int cx = r ? 46 : 18, cy = 38;
        for (int a = 0; a < 20; a++) tput(t, cx + (int)lroundf(2.5f * cosf(a * 0.314f)), cy + (int)lroundf(2.5f * sinf(a * 0.314f)), sh(R_IRON, 0.35f + 0.2f * (a < 10)));
    }
    if (!open) return;
    for (int i = 0; i < 2 * (48 + 28); i++) {                          /* the seam: pale, as if lit from inside */
        int x, y, k = i % (48 + 28);
        if (k < 48) { x = 8 + k; y = i < 48 + 28 ? 17 : 46; } else { y = 18 + (k - 48); x = i < 48 + 28 ? 7 : 56; }
        tput(t, x, y, h01(x, y, seed + 9) > 0.3f ? (uint8_t)(R_GLOW + 13) : (uint8_t)C_WHITE);
    }
    for (int x = 20; x < 30; x++) tput(t, x, 18 + (x - 20) / 3, R_GLOW + 10);   /* and a crack across a corner */
}

/* a crack in the wall that breathes cold: rime furred out round it, a little blue at its heart */
static void gen_hz_cold(int t, uint32_t seed) {
    gen_stone(t, R_STONE, seed);
    float x = 30.0f + 4.0f * h01(1, 1, seed);
    for (int y = 6; y < 60; y++) {
        x += (h01(y, 3, seed) - 0.5f) * 2.4f;
        int w = 1 + (y > 18 && y < 48);
        for (int dx = -w; dx <= w; dx++) tput(t, (int)x + dx, y, dx == 0 ? sh(R_GLOW, 0.15f) : sh(R_STONE, 0.02f));
        for (int k = 0; k < 7; k++) {                                  /* the rime, feathering out */
            float ax = x + (h01(y, k, seed + 2) - 0.5f) * 18.0f;
            if (h01(y, k + 20, seed) < 0.55f - fabsf(ax - x) / 22.0f) tput(t, (int)ax, y, sh(R_GHOST, 0.55f + 0.4f * h01(y, k, seed + 3)));
        }
    }
}

/* a sluice gate low in the wall: an iron plate in its guides, a wheel to wind it; open, the dark and the water coming */
static void gen_hz_sluice(int t, uint32_t seed, int open) {
    gen_stone(t, R_STONE, seed);
    for (int y = 30; y < 64; y++)
        for (int x = 14; x < 50; x++) {
            int guide = x < 17 || x > 46;
            uint8_t c;
            if (guide) c = sh(R_IRON, 0.3f + 0.15f * (x == 15 || x == 48));
            else if (!open) c = sh(R_IRON, 0.18f + 0.1f * ((y - 30) % 8 == 0) + 0.06f * h01(x, y, seed) + 0.1f * ((x * 7 + y) % 23 == 0));   /* riveted plate */
            else if (y < 40) c = sh(R_IRON, 0.22f + 0.08f * ((y - 30) % 8 == 0));   /* thrown up, its edge showing */
            else { float f = fbm(x / 4.0f, (y + 0.0f) / 3.0f, 16, seed + 7); c = f > 0.55f ? sh(R_GHOST, 0.6f + 0.35f * f) : sh(R_STONE, 0.02f + 0.06f * f); }   /* water foaming out of the dark */
            tput(t, x, y, c);
        }
    for (int a = 0; a < 28; a++) {                                     /* the wheel above it */
        float ang = a * 0.2244f;
        tput(t, 32 + (int)lroundf(7.0f * cosf(ang)), 20 + (int)lroundf(7.0f * sinf(ang)), sh(R_IRON, 0.45f));
    }
    for (int k = 0; k < 4; k++) for (int r = 1; r < 7; r++) tput(t, 32 + (int)lroundf(r * cosf(k * 0.785f)), 20 + (int)lroundf(r * sinf(k * 0.785f)), sh(R_IRON, 0.35f));
    for (int y = 27; y < 31; y++) tput(t, 32, y, sh(R_IRON, 0.4f));
}

/* the Weeping Saint: a robed figure in a round-headed alcove, her hands together, tear tracks down her face;
 * drinking, her eyes alight */
static void gen_hz_saint(int t, uint32_t seed, int lit) {
    gen_stone(t, R_STONE, seed);
    for (int y = 6; y < 62; y++)
        for (int x = 14; x < 50; x++) {
            float dx = x + 0.5f - 32.0f, top = 20.0f - sqrtf(fmaxf(0.0f, 18.0f * 18.0f - dx * dx));
            if (y < top + 4.0f) continue;
            tput(t, x, y, sh(R_STONE, 0.03f + 0.04f * h01(x, y, seed)));     /* the alcove's dark */
        }
    for (int y = 14; y < 62; y++)
        for (int x = 18; x < 46; x++) {
            float dx = x + 0.5f - 32.0f, head = (dx * dx) / 25.0f + (y + 0.5f - 19.0f) * (y + 0.5f - 19.0f) / 30.0f;
            float robe = y >= 24 ? fabsf(dx) - (5.0f + (y - 24) * 0.28f) : 99.0f;
            if (head <= 1.0f) tput(t, x, y, sh(R_BONE, 0.5f - 0.04f * dx));
            else if (robe <= 0.0f) tput(t, x, y, sh(R_GHOST, 0.18f + 0.14f * (dx < 0 ? 1.0f : 0.4f) + 0.05f * h01(x, y, seed) - 0.12f * ((int)(dx + 40) % 4 == 0)));
        }
    for (int y = 30; y < 36; y++) for (int x = 29; x < 35; x++) tput(t, x, y, sh(R_BONE, 0.42f));   /* the hands */
    for (int e = 0; e < 2; e++) {
        int ex = e ? 34 : 29;
        tput(t, ex, 18, lit ? (uint8_t)(R_GLOW + 15) : sh(R_STONE, 0.05f));
        tput(t, ex + 1, 18, lit ? (uint8_t)(R_GLOW + 12) : sh(R_STONE, 0.08f));
        for (int y = 19; y < 28; y++) tput(t, ex + (y > 24), y, sh(R_GHOST, 0.75f));   /* the tears */
    }
    for (int x = 18; x < 46; x++) tput(t, x, 61, sh(R_GHOST, 0.5f + 0.3f * h01(x, 61, seed)));   /* her pool's beginning */
}

/* the Lodestone Seam: a band of dark rock veined green, the veins crackling blue-white when it is about to arc */
static void gen_hz_seam(int t, uint32_t seed, int lit) {
    gen_stone(t, R_STONE, seed);
    for (int y = 0; y < TS; y++)
        for (int x = 0; x < TS; x++) {
            float band = fabsf(y - 32.0f - 8.0f * sinf(x * 0.09f + 1.0f)) - 12.0f;
            if (band > 0.0f) continue;
            float v = fbm(x / 5.0f, y / 5.0f, 16, seed + 3);
            uint8_t c = sh(R_IRON, 0.08f + 0.12f * v);
            float vein = fabsf(fbm(x / 9.0f, y / 4.0f, 8, seed + 11) - 0.5f);
            if (vein < 0.035f) c = lit ? (uint8_t)(h01(x, y, seed) > 0.5f ? R_GLOW + 14 : C_WHITE) : sh(R_MOSS, 0.55f + 0.4f * (0.035f - vein) / 0.035f);
            tput(t, x, y, c);
        }
}

/* Fissure Breath: a crack down black rock, glowing from deep inside; bright, it is about to breathe */
static void gen_hz_fissure(int t, uint32_t seed, int hot) {
    gen_stone(t, R_STONE, seed);
    for (int y = 0; y < TS; y++) for (int x = 0; x < TS; x++) tput(t, x, y, sh(R_IRON, 0.05f + 0.1f * fbm(x / 6.0f, y / 6.0f, 16, seed)));
    float x = 32.0f;
    for (int y = 2; y < 62; y++) {
        x += (h01(y, 5, seed) - 0.5f) * 2.0f;
        int w = 1 + (y > 12 && y < 52) + (hot && y > 20 && y < 44);
        for (int dx = -w - 2; dx <= w + 2; dx++) {
            int ad = abs(dx);
            uint8_t c = ad <= w ? (uint8_t)(R_FLAME + (hot ? 15 - ad * 2 : 9 - ad * 3)) : sh(R_BRICK, (hot ? 0.75f : 0.45f) - 0.15f * (ad - w));
            tput(t, (int)x + dx, y, c);
        }
    }
}

/* the Black Mirror: a sheet of obsidian in an iron frame, a cold streak across it; and, drinking, a shape in it */
static void gen_hz_mirror(int t, uint32_t seed, int seen) {
    gen_stone(t, R_STONE, seed);
    for (int y = 6; y < 60; y++)
        for (int x = 12; x < 52; x++) {
            int frame = x < 15 || x > 48 || y < 9 || y > 56;
            uint8_t c = frame ? sh(R_IRON, 0.3f + 0.15f * (x == 12 || y == 6)) : sh(R_IRON, 0.02f + 0.03f * h01(x, y, seed));
            if (!frame && fabsf((x - 15) - (y - 9) * 0.7f - 6.0f) < 1.2f) c = sh(R_GHOST, 0.3f);   /* the streak */
            tput(t, x, y, c);
        }
    if (!seen) return;
    for (int y = 16; y < 56; y++)                                      /* a hooded shape, standing where you stand */
        for (int x = 18; x < 46; x++) {
            float dx = x + 0.5f - 32.0f, hood = dx * dx / 30.0f + (y - 22.0f) * (y - 22.0f) / 40.0f, body = y > 26 ? fabsf(dx) - (7.0f + (y - 26) * 0.25f) : 99.0f;
            if ((hood <= 1.0f || body <= 0.0f) && ((x + y) & 1)) tput(t, x, y, sh(R_PURPLE, 0.25f + 0.15f * (hood <= 1.0f)));
        }
    tput(t, 30, 23, R_GLOW + 12); tput(t, 34, 23, R_GLOW + 12);
}

/* grit falling from the ceiling in a thin column, two frames of it */
static void spr_grit(int id, int frame) {
    cv_begin(cv_sprite, TS, TS);
    uint32_t s = 911u + (uint32_t)frame * 37u;
    for (int i = 0; i < 70; i++) {
        float y = h01(i, 1, s) * 52.0f, x = 32.0f + (h01(i, 2, s) - 0.5f) * (3.0f + y * 0.08f);
        cv_px((int)x, (int)y, sh(R_BONE, 0.35f + 0.4f * h01(i, 3, s)));
        if (h01(i, 4, s) > 0.7f) cv_px((int)x, (int)y + 1, sh(R_STONE, 0.5f));
    }
    cv_store(id);
}

/* the Swinging Censer: a great iron bowl on its chain, pierced, smoke curling out of it (hung from the top) */
static void spr_censer(void) {
    cv_begin(cv_sprite, TS, TS);
    for (int y = 0; y < 40; y++) cv_px(32, y, (y & 3) == 0 ? sh(R_IRON, 0.55f) : sh(R_IRON, 0.32f));   /* the chain */
    for (int k = 0; k < 3; k++) cv_line(32, 38, 22 + k * 10, 44, sh(R_IRON, 0.35f));
    cv_ellipse(32, 49, 11, 7, R_IRON, 0.12f, 0.6f);
    for (int x = 23; x <= 41; x += 4) cv_px(x, 48, R_FLAME + 9);       /* the holes, a coal's light in them */
    cv_flat(32, 43.5f, 10, 1.6f, sh(R_IRON, 0.5f));
    for (int i = 0; i < 26; i++) {                                    /* smoke, rising off it */
        float y = 42.0f - h01(i, 1, 41) * 30.0f, x = 32.0f + sinf(y * 0.3f + i) * (2.0f + (42 - y) * 0.15f);
        cv_px((int)x, (int)y, sh(R_STONE, 0.45f + 0.3f * h01(i, 2, 41)));
    }
    cv_outline(sh(R_IRON, 0.02f));
    cv_store(SP_CENSER);
}
/* a soft puff, dithered at its edge so it shows the crypt through it: smoke, the embalmers' breath, gas */
static void spr_puff(int id, int ramp, float lo, float hi, uint32_t seed) {
    cv_begin(cv_sprite, TS, TS);
    for (int y = 20; y < 64; y++)
        for (int x = 0; x < TS; x++) {
            float dx = (x + 0.5f - 32.0f) / 30.0f, dy = (y + 0.5f - 50.0f) / 14.0f, d = dx * dx + dy * dy;
            d += (fbm(x / 7.0f, y / 5.0f, 16, seed) - 0.5f) * 0.9f;
            if (d > 1.0f || (d > 0.55f && ((x + y) & 1)) || (d > 0.8f && (x & 1))) continue;
            cv_px(x, y, sh(ramp, lo + (hi - lo) * (1.0f - d) * (0.7f + 0.3f * fbm(x / 3.0f, y / 3.0f, 16, seed + 1))));
        }
    cv_store(id);
}
/* a votive rack gone over: an iron frame on its side, cups scattered, wax running from them alight */
static void spr_votive(void) {
    cv_begin(cv_sprite, TS, TS);
    cv_flat(32, 61, 24, 2.5f, R_WARM + 1);
    cv_line(12, 58, 50, 50, sh(R_IRON, 0.4f)); cv_line(12, 59, 50, 51, sh(R_IRON, 0.25f));
    cv_line(16, 57, 16, 48, sh(R_IRON, 0.35f)); cv_line(46, 51, 47, 42, sh(R_IRON, 0.35f));
    for (int i = 0; i < 7; i++) {
        float x = 10.0f + i * 7.0f + 3.0f * h01(i, 1, 51), y = 56.0f + 3.0f * h01(i, 2, 51);
        cv_ellipse(x, y, 2.4f, 1.8f, R_BLOOD, 0.3f, 0.7f);
        cv_line(x + 2, y + 1, x + 6, y + 2, sh(R_BONE, 0.8f));               /* the wax running */
        if (i & 1) { cv_px((int)x, (int)y - 3, R_FLAME + 12); cv_px((int)x, (int)y - 2, R_FLAME + 8); }
    }
    cv_store(SP_VOTIVE);
}
/* the Hungry Reliquary: a gilded casket on a stone plinth, its lid lifted on a violet dark when it feeds */
static void spr_hz_reliq(int id, int open) {
    cv_begin(cv_sprite, TS, TS);
    for (int y = 46; y < 64; y++) for (int x = 20; x < 44; x++) cv_px(x, y, sh(R_STONE, 0.35f + 0.15f * (x < 24) - 0.1f * (y > 60)));   /* the plinth */
    for (int y = 34; y < 46; y++) for (int x = 22; x < 42; x++) cv_px(x, y, sh(R_GOLD, 0.45f + 0.3f * (x < 30) + 0.2f * ((x + y) % 7 == 0)));   /* the casket */
    for (int x = 22; x < 42; x++) cv_px(x, 40, sh(R_GOLD, 0.25f));
    cv_flat(32, 40, 2.2f, 2.2f, sh(R_PURPLE, 0.6f));                  /* a stone in it */
    if (!open) {
        float lid[8] = { 21, 34, 43, 34, 39, 28, 25, 28 };
        cv_poly(lid, 4, R_GOLD, 0.85f, 0.5f, 28, 6);
    } else {
        for (int y = 30; y < 35; y++) for (int x = 24; x < 40; x++) cv_px(x, y, (x + y) & 1 ? (uint8_t)(R_PURPLE + 13) : (uint8_t)(R_GLOW + 6));
        cv_line(22, 34, 30, 18, sh(R_GOLD, 0.8f)); cv_line(42, 34, 36, 18, sh(R_GOLD, 0.6f)); cv_line(30, 18, 36, 18, sh(R_GOLD, 0.9f));   /* the lid thrown up */
    }
    cv_outline(sh(R_STONE, 0.02f));
    cv_store(id);
}
/* a web across a doorway, wall to wall and floor to beam: strands so fine they hardly show */
static void spr_snare(void) {
    cv_begin(cv_sprite, TS, TS);
    for (int k = 0; k < 10; k++) {
        float a = k * 0.628f;
        cv_line(32, 30, 32 + 34.0f * cosf(a), 30 + 34.0f * sinf(a), sh(R_GHOST, 0.35f + 0.1f * (k & 1)));
    }
    for (int r = 4; r < 30; r += 5)
        for (int k = 0; k < 10; k++) {
            float a0 = k * 0.628f, a1 = a0 + 0.628f;
            cv_line(32 + r * cosf(a0), 30 + r * sinf(a0), 32 + r * cosf(a1), 30 + r * sinf(a1), sh(R_GHOST, 0.3f));
        }
    cv_store(SP_SNARE);
}
/* Shatterglass: black glass spikes stood up from a star of cracks, their edges cold */
static void spr_spikes(void) {
    cv_begin(cv_sprite, TS, TS);
    for (int i = 0; i < 9; i++) {
        float x = 10.0f + i * 5.5f + 2.0f * h01(i, 1, 61), h = 14.0f + 22.0f * h01(i, 2, 61) * (1.0f - fabsf(i - 4.0f) / 6.0f);
        float lean = (h01(i, 3, 61) - 0.5f) * 8.0f;
        float tri[6] = { x - 3.0f, 63.0f, x + 3.0f, 63.0f, x + lean, 63.0f - h };
        cv_poly(tri, 3, R_IRON, 0.12f, 0.45f, 63.0f - h, h);
        cv_line(x + 3.0f, 63, x + lean, 63.0f - h, sh(R_GHOST, 0.7f));   /* the edge, catching the light */
        cv_line(x - 3.0f, 63, x + lean, 63.0f - h, sh(R_IRON, 0.05f));
    }
    cv_store(SP_SPIKES);
}
/* ash coming down: grey flakes, thicker below */
static void spr_ashfall(void) {
    cv_begin(cv_sprite, TS, TS);
    for (int i = 0; i < 160; i++) {
        float y = sqrtf(h01(i, 1, 71)) * 63.0f, x = 4.0f + h01(i, 2, 71) * 56.0f;
        cv_px((int)x, (int)y, sh(R_STONE, 0.45f + 0.4f * h01(i, 3, 71)));
        if (h01(i, 4, 71) > 0.5f) cv_px((int)x + 1, (int)y, sh(R_STONE, 0.35f));
    }
    cv_store(SP_ASHFALL);
}

/* ---- puzzles' things (1.0.0b13, docs/PLAN-b13.md sections 6 and 7) ---- */
const char *const PZ_MARKS[16][7] = {
    { "...#...", "..###..", ".#####.", ".##.##.", ".#####.", ".#####.", "#######" },   /* a mitre */
    { ".#####.", "#######", "#.....#", "#.###.#", "#.###.#", "#######", ".#...#." },   /* a helm */
    { "..###..", ".#...#.", "#.....#", "#.....#", ".#...#.", "..###..", "......." },   /* a tonsure */
    { "#..#..#", "#.###.#", "#######", "#######", ".#####.", "#######", "......." },   /* a crown */
    { ".##....", "#..#...", "#..####", ".##..#.", ".....#.", "....##.", "......." },   /* a key */
    { "..###..", ".#.#.#.", "#..#..#", "#######", "#..#..#", ".#.#.#.", "..###.." },   /* a wheel */
    { "#.....#", ".#...#.", "..#.#..", "...#...", "..#.#..", ".#...#.", "#.....#" },   /* arrows, crossed */
    { "#.#.#.#", "#######", ".#####.", ".##.##.", ".#####.", ".#####.", "#######" },   /* a tower */
    { "..##...", ".####..", "#######", ".#####.", ".#...#.", ".#...#.", "......." },   /* a lamb */
    { "...#...", "..#.#..", "...#...", ".#####.", "..###..", "...#...", "..###.." },   /* a lamp */
    { "..###..", ".#...#.", "#..#..#", "#.###.#", "#..#..#", ".#...#.", "..###.." },   /* an eye */
    { ".#.#.#.", ".#.#.#.", ".#####.", "#######", ".#####.", "..###..", "..###.." },   /* a hand */
    { "#.....#", ".#...#.", "..#.#..", "...#...", "..#.#..", ".##.##.", "##...##" },   /* picks */
    { "..###..", ".##....", "##.....", "##.....", "##.....", ".##....", "..###.." },   /* a moon */
    { "...#...", "...#...", "#######", ".#####.", "..###..", ".##.##.", "#.....#" },   /* a star */
    { ".#####.", "#######", "#.#.#.#", "#######", ".#####.", ".#.#.#.", "......." },   /* a skull */
};
static void spr_pz_mark(int id, int k) {                           /* a mark, carved dark and edged pale, centred at (32, 30) */
    cv_begin(cv_sprite, TS, TS);
    for (int y = 0; y < 7; y++)
        for (int x = 0; x < 7; x++)
            if (PZ_MARKS[k][y][x] == '#') { cv_px(29 + x, 27 + y, sh(R_BLOOD, 0.25f)); cv_px(30 + x, 28 + y, sh(R_BONE, 0.85f)); }
    cv_store(id);
}
static void spr_pz_pips(int id, int n) {                           /* one to nine pips, as on a die, centred at (32, 30) */
    static const char *const P[9] = { "....#....", "#.......#", "#...#...#", "#.#...#.#", "#.#.#.#.#", "#.##.##.#", "#.######.", "####.####", "#########" };
    cv_begin(cv_sprite, TS, TS);
    for (int k = 0; k < 9; k++)
        if (P[n - 1][k] == '#') { int x = 27 + (k % 3) * 4, y = 25 + (k / 3) * 4; cv_px(x, y, sh(R_GOLD, 0.9f)); cv_px(x + 1, y, sh(R_GOLD, 0.7f)); cv_px(x, y + 1, sh(R_GOLD, 0.6f)); cv_px(x + 1, y + 1, sh(R_GOLD, 0.4f)); }
    cv_store(id);
}
static void spr_pz_skull(void) {                                   /* a skull on the floor, its brow toward you (the mark goes there) */
    cv_begin(cv_sprite, TS, TS);
    cv_flat(32, 61, 9, 1.8f, R_WARM + 1);
    cv_skull(32, 52, 1.15f, 0.25f, 0.85f);
    cv_outline(sh(R_BONE, 0.05f));
    cv_store(SP_PZ_SKULL);
}
static void spr_pz_plinth(void) {                                  /* a low plinth: what it takes stands on its top */
    cv_begin(cv_sprite, TS, TS);
    for (int y = 46; y < 64; y++)
        for (int x = 20; x < 44; x++) cv_px(x, y, sh(R_STONE, 0.3f + 0.2f * (x < 23) + 0.15f * (y == 46) - 0.1f * (y > 60) + 0.06f * h01(x, y, 5)));
    for (int x = 18; x < 46; x++) { cv_px(x, 45, sh(R_STONE, 0.6f)); cv_px(x, 46, sh(R_STONE, 0.45f)); }
    cv_outline(sh(R_STONE, 0.02f));
    cv_store(SP_PZ_PLINTH);
}
static void spr_pz_dial(int id, int turn) {                        /* a bronze dial on a post, its pointer at one of eight */
    cv_begin(cv_sprite, TS, TS);
    for (int y = 40; y < 64; y++) for (int x = 30; x < 34; x++) cv_px(x, y, sh(R_IRON, 0.3f + 0.2f * (x == 30)));
    cv_ellipse(32, 30, 11, 11, R_GOLD, 0.3f, 0.75f);
    cv_flat(32, 30, 8, 8, sh(R_GOLD, 0.2f));
    for (int k = 0; k < 8; k++) cv_px(32 + (int)lroundf(9.5f * sinf(k * 0.7854f)), 30 - (int)lroundf(9.5f * cosf(k * 0.7854f)), sh(R_BONE, 0.9f));
    float a = turn * 0.7854f;
    cv_line(32, 30, 32 + 7.0f * sinf(a), 30 - 7.0f * cosf(a), sh(R_BLOOD, 0.7f));
    cv_px(32, 30, sh(R_GOLD, 0.95f));
    cv_outline(sh(R_IRON, 0.02f));
    cv_store(id);
}
static void spr_pz_lever(int id, int down) {                       /* an iron lever in a slot in its stone block */
    cv_begin(cv_sprite, TS, TS);
    for (int y = 48; y < 64; y++) for (int x = 24; x < 40; x++) cv_px(x, y, sh(R_STONE, 0.35f + 0.15f * (x < 27)));
    for (int x = 30; x < 34; x++) cv_px(x, 48, sh(R_STONE, 0.05f));
    float tx = down ? 42.0f : 22.0f, ty = down ? 44.0f : 30.0f;
    cv_capsule(32, 49, tx, ty, 1.4f, R_IRON, 0.25f, 0.65f);
    cv_ellipse(tx, ty, 2.6f, 2.6f, R_WOOD, 0.3f, 0.7f);
    cv_outline(sh(R_IRON, 0.02f));
    cv_store(id);
}
static void spr_pz_candle(int id, int lit) {                       /* a tall candle in an iron pricket */
    cv_begin(cv_sprite, TS, TS);
    cv_line(26, 63, 32, 52, sh(R_IRON, 0.35f)); cv_line(38, 63, 32, 52, sh(R_IRON, 0.35f));
    for (int y = 30; y < 52; y++) for (int x = 30; x < 34; x++) cv_px(x, y, sh(R_BONE, 0.6f + 0.25f * (x == 30)));
    for (int x = 28; x < 36; x++) cv_px(x, 52, sh(R_IRON, 0.5f));
    cv_px(32, 29, sh(R_IRON, 0.1f));
    if (lit) { cv_flat(32, 25, 2.2f, 4.0f, R_FLAME + 11); cv_flat(32, 26, 1.0f, 2.2f, R_FLAME + 15); }
    cv_store(id);
}
static void spr_pz_tablet(void) {                                  /* a carved tablet on a stand: something to read */
    cv_begin(cv_sprite, TS, TS);
    for (int y = 54; y < 64; y++) for (int x = 28; x < 36; x++) cv_px(x, y, sh(R_STONE, 0.3f));
    for (int y = 22; y < 54; y++)
        for (int x = 18; x < 46; x++) {
            float top = 26.0f - sqrtf(fmaxf(0.0f, 14.0f * 14.0f - (x - 32.0f) * (x - 32.0f))) * 0.3f;
            if (y < top) continue;
            uint8_t c = sh(R_STONE, 0.45f + 0.1f * h01(x, y, 9));
            if (y > 30 && y < 50 && x > 21 && x < 43 && (y % 4) == 0 && h01(x, y, 3) > 0.35f) c = sh(R_STONE, 0.15f);   /* its lines */
            cv_px(x, y, c);
        }
    cv_outline(sh(R_STONE, 0.02f));
    cv_store(SP_PZ_TABLET);
}

/* a niche cut in the wall: an arched recess with a sill, a mark cut over it (drawn as a mark by the game) */
static void spr_pz_niche(void) {
    cv_begin(cv_sprite, TS, TS);
    for (int y = 14; y < 52; y++)
        for (int x = 18; x < 46; x++) {
            float dx = x + 0.5f - 32.0f, top = 26.0f - sqrtf(fmaxf(0.0f, 14.0f * 14.0f - dx * dx)) * 0.6f;
            if (y < top - 3.0f) continue;
            int edge = y < top || x < 21 || x > 42;
            cv_px(x, y, edge ? sh(R_STONE, 0.5f + 0.1f * (x < 24)) : sh(R_STONE, 0.04f + 0.03f * h01(x, y, 3)));
        }
    for (int x = 16; x < 48; x++) { cv_px(x, 52, sh(R_STONE, 0.65f)); cv_px(x, 53, sh(R_STONE, 0.4f)); cv_px(x, 54, sh(R_STONE, 0.2f)); }   /* its sill */
    cv_store(SP_PZ_NICHE);
}
/* a knight on his tomb, his helmed head turned: away, to the right, toward you, to the left */
static void spr_pz_effigy(int id, int turn) {
    cv_begin(cv_sprite, TS, TS);
    for (int y = 44; y < 64; y++) for (int x = 6; x < 58; x++) cv_px(x, y, sh(R_STONE, 0.3f + 0.15f * (y < 47) + 0.1f * (x < 9) - 0.12f * (y > 60) + 0.05f * h01(x, y, 7)));   /* the tomb chest */
    for (int x = 10; x < 54; x += 9) for (int y = 50; y < 60; y++) cv_px(x, y, sh(R_STONE, 0.12f));   /* its panels */
    cv_capsule(16, 40, 44, 40, 4.5f, R_IRON, 0.2f, 0.6f);             /* the knight lying */
    cv_capsule(40, 38, 50, 38, 2.0f, R_IRON, 0.25f, 0.55f);           /* his feet, and a hound under them */
    cv_ellipse(51, 41, 3.0f, 2.2f, R_STONE, 0.3f, 0.6f);
    cv_line(24, 38, 30, 34, sh(R_GHOST, 0.5f));                       /* the sword on his breast */
    float hx = 13.0f, hy = 35.0f;                                     /* the helm on its pillow */
    cv_ellipse(hx, hy + 3.0f, 5.0f, 2.0f, R_BLOOD, 0.25f, 0.5f);
    cv_ellipse(hx, hy, 4.2f, 4.4f, R_IRON, 0.25f, 0.75f);
    if (turn == 2) { cv_line(hx - 3, hy, hx + 3, hy, sh(R_IRON, 0.02f)); cv_line(hx, hy - 1, hx, hy + 3, sh(R_IRON, 0.02f)); }   /* the visor, at you */
    else if (turn == 1) { cv_line(hx + 1, hy, hx + 4, hy, sh(R_IRON, 0.02f)); cv_px((int)hx + 4, (int)hy + 1, sh(R_IRON, 0.02f)); }
    else if (turn == 3) { cv_line(hx - 4, hy, hx - 1, hy, sh(R_IRON, 0.02f)); cv_px((int)hx - 4, (int)hy + 1, sh(R_IRON, 0.02f)); }
    else { cv_line(hx - 2, hy - 3, hx + 2, hy - 3, sh(R_IRON, 0.85f)); }   /* the back of the helm, its crest */
    cv_outline(sh(R_STONE, 0.02f));
    cv_store(id);
}
/* a pan of bronze on its stand; and the great scale, its beam tipped left, level, or right */
static void spr_pz_pan(void) {
    cv_begin(cv_sprite, TS, TS);
    for (int y = 46; y < 64; y++) for (int x = 30; x < 34; x++) cv_px(x, y, sh(R_IRON, 0.3f + 0.2f * (x == 30)));
    cv_flat(32, 63, 8, 1.5f, sh(R_IRON, 0.2f));
    cv_ellipse(32, 45, 13, 3.5f, R_GOLD, 0.3f, 0.75f);
    cv_flat(32, 44, 11, 2.0f, sh(R_GOLD, 0.25f));
    cv_outline(sh(R_IRON, 0.02f));
    cv_store(SP_PZ_PAN);
}
static void spr_pz_scale(int id, int tilt) {
    cv_begin(cv_sprite, TS, TS);
    for (int y = 14; y < 64; y++) for (int x = 30; x < 34; x++) cv_px(x, y, sh(R_IRON, 0.3f + 0.25f * (x == 30)));
    cv_flat(32, 63, 10, 1.6f, sh(R_IRON, 0.2f));
    float d = (float)(tilt - 1) * 5.0f;
    cv_line(10, 16 + d, 54, 16 - d, sh(R_GOLD, 0.8f)); cv_line(10, 17 + d, 54, 17 - d, sh(R_GOLD, 0.45f));
    for (int k = 0; k < 2; k++) {
        float ex = k ? 54.0f : 10.0f, ey = k ? 16.0f - d : 16.0f + d;
        cv_line(ex, ey, ex - 5, ey + 16, sh(R_GOLD, 0.4f)); cv_line(ex, ey, ex + 5, ey + 16, sh(R_GOLD, 0.4f));
        cv_ellipse(ex, ey + 17, 7.0f, 2.2f, R_GOLD, 0.3f, 0.7f);
    }
    cv_ellipse(32, 14, 3, 3, R_GOLD, 0.4f, 0.9f);
    cv_outline(sh(R_IRON, 0.02f));
    cv_store(id);
}
/* a small reliquary: a gilt casket, a purple stone in its front (a red one on the judged), beads on its lid */
static void spr_pz_casket(void) {
    cv_begin(cv_sprite, TS, TS);
    cv_flat(32, 62, 10, 1.6f, R_WARM + 1);
    for (int y = 48; y < 62; y++) for (int x = 23; x < 41; x++) cv_px(x, y, sh(R_GOLD, 0.4f + 0.3f * (x < 27) + 0.15f * ((x + y) % 6 == 0)));
    for (int x = 22; x < 42; x++) { cv_px(x, 47, sh(R_GOLD, 0.85f)); cv_px(x, 46, sh(R_GOLD, 0.6f)); }
    cv_flat(32, 54, 2.0f, 2.0f, sh(R_PURPLE, 0.6f));
    cv_outline(sh(R_GOLD, 0.05f));
    cv_store(SP_PZ_CASKET);
}
/* the Order's banner on a pole: a cross on black, its foot ragged */
static void spr_pz_banner(void) {
    cv_begin(cv_sprite, TS, TS);
    for (int y = 4; y < 64; y++) cv_px(31, y, sh(R_WOOD, 0.35f)), cv_px(32, y, sh(R_WOOD, 0.55f));
    cv_line(18, 8, 46, 8, sh(R_WOOD, 0.5f));
    for (int y = 9; y < 46; y++)
        for (int x = 19; x < 46; x++) {
            if (x == 31 || x == 32) continue;
            if (y > 40 && h01(x, y, 13) < (float)(y - 40) / 6.0f) continue;   /* ragged */
            int cross = (x >= 30 && x <= 33) || (y >= 20 && y <= 23);
            cv_px(x, y, cross ? sh(R_BONE, 0.65f) : sh(R_BLOOD, 0.12f + 0.08f * h01(x, y, 2)));
        }
    cv_store(SP_PZ_BANNER);
}

/* a cistern let into the wall: a stone tank, its flood lines cut, water to a height (0 dry to 8 full) */
static void spr_pz_cistern(int id, int lvl) {
    cv_begin(cv_sprite, TS, TS);
    for (int y = 18; y < 64; y++)
        for (int x = 14; x < 50; x++) {
            int wall = x < 17 || x > 46 || y > 60;
            float wl = 60.0f - (float)lvl * 5.0f;
            uint8_t c = wall ? sh(R_STONE, 0.4f + 0.15f * (x < 16)) : (y >= wl ? sh(R_GHOST, 0.25f + 0.1f * ((x + y) & 1) + (y == (int)wl ? 0.4f : 0.0f)) : sh(R_STONE, 0.05f));
            if (!wall && (y - 20) % 5 == 0 && x < 21) c = sh(R_BONE, 0.6f);   /* the flood lines */
            cv_px(x, y, c);
        }
    cv_outline(sh(R_STONE, 0.02f));
    cv_store(id);
}
static void spr_pz_saint(void) {                                   /* a saint in grey stone, her hands held out for her emblem */
    cv_begin(cv_sprite, TS, TS);
    for (int y = 56; y < 64; y++) for (int x = 20; x < 44; x++) cv_px(x, y, sh(R_STONE, 0.35f + 0.1f * (x < 23)));
    float robe[8] = { 24, 56, 40, 56, 37, 22, 27, 22 };
    cv_poly(robe, 4, R_STONE, 0.55f, 0.25f, 22, 34);
    cv_ellipse(32, 16, 4.5f, 5.0f, R_STONE, 0.3f, 0.7f);
    cv_capsule(28, 26, 22, 34, 1.6f, R_STONE, 0.3f, 0.6f); cv_capsule(36, 26, 42, 34, 1.6f, R_STONE, 0.3f, 0.6f);   /* arms out */
    cv_ellipse(32, 11, 6.5f, 2.0f, R_GOLD, 0.3f, 0.6f);               /* a halo, dull */
    cv_outline(sh(R_STONE, 0.02f));
    cv_store(SP_PZ_SAINT);
}
static void spr_pz_emblem(void) {                                  /* a small bronze emblem on the floor: its sign goes on its face */
    cv_begin(cv_sprite, TS, TS);
    cv_flat(32, 62, 8, 1.4f, R_WARM + 1);
    cv_ellipse(32, 55, 7.0f, 6.0f, R_GOLD, 0.25f, 0.65f);
    cv_outline(sh(R_GOLD, 0.05f));
    cv_store(SP_PZ_EMBLEM);
}
static void spr_pz_spout(void) {                                   /* a lion's-head spout, water from its mouth */
    cv_begin(cv_sprite, TS, TS);
    cv_ellipse(32, 36, 9, 9, R_STONE, 0.25f, 0.65f);
    cv_flat(32, 40, 3, 2, sh(R_STONE, 0.02f));
    for (int y = 42; y < 64; y++) cv_px(32 + (y & 1), y, sh(R_GHOST, 0.6f + 0.2f * (y & 1)));
    cv_outline(sh(R_STONE, 0.02f));
    cv_store(SP_PZ_SPOUT);
}
static void spr_pz_mill(int id, int turn) {                        /* a wooden mill wheel on its axle, paddles round it */
    cv_begin(cv_sprite, TS, TS);
    for (int a = 0; a < 40; a++) { float an = a * 0.157f; cv_px(32 + (int)lroundf(16.0f * cosf(an)), 40 + (int)lroundf(16.0f * sinf(an)), sh(R_WOOD, 0.4f)); }
    for (int k = 0; k < 8; k++) {
        float an = k * 0.785f + turn * 0.39f;
        cv_line(32, 40, 32 + 16.0f * cosf(an), 40 + 16.0f * sinf(an), sh(R_WOOD, 0.3f));
        cv_capsule(32 + 15.0f * cosf(an), 40 + 15.0f * sinf(an), 32 + 20.0f * cosf(an), 40 + 20.0f * sinf(an), 1.5f, R_WOOD, 0.3f, 0.6f);
    }
    cv_ellipse(32, 40, 2.5f, 2.5f, R_IRON, 0.3f, 0.6f);
    cv_outline(sh(R_WOOD, 0.02f));
    cv_store(id);
}
static void spr_pz_idol(void) {                                    /* a squat idol of black glass, gold in its eyes */
    cv_begin(cv_sprite, TS, TS);
    float body[8] = { 22, 63, 42, 63, 38, 30, 26, 30 };
    cv_poly(body, 4, R_IRON, 0.35f, 0.08f, 30, 33);
    cv_ellipse(32, 24, 8, 8, R_IRON, 0.1f, 0.45f);
    cv_px(29, 23, R_GOLD + 12); cv_px(35, 23, R_GOLD + 12); cv_px(29, 24, R_GOLD + 9); cv_px(35, 24, R_GOLD + 9);
    cv_outline(sh(R_IRON, 0.02f));
    cv_store(SP_PZ_IDOL);
}
static void spr_pz_lock(void) {                                    /* a great lock plate, a channel cut into its keyhole */
    cv_begin(cv_sprite, TS, TS);
    for (int y = 28; y < 52; y++) for (int x = 22; x < 42; x++) cv_px(x, y, sh(R_IRON, 0.3f + 0.25f * (x < 25 || y < 31)));
    cv_flat(32, 38, 2.5f, 2.5f, sh(R_IRON, 0.02f)); for (int y = 38; y < 46; y++) cv_px(32, y, sh(R_IRON, 0.02f));
    for (int x = 10; x < 22; x++) cv_px(x, 38, sh(R_GOLD, 0.35f));
    cv_outline(sh(R_IRON, 0.02f));
    cv_store(SP_PZ_LOCK);
}
static void spr_pz_wheel(void) {                                   /* an iron sluice wheel on the wall, spoked; a sign on its hub */
    cv_begin(cv_sprite, TS, TS);
    for (int a = 0; a < 48; a++) { float an = a * 0.131f; cv_px(32 + (int)lroundf(14.0f * cosf(an)), 30 + (int)lroundf(14.0f * sinf(an)), sh(R_IRON, 0.5f)); cv_px(32 + (int)lroundf(13.0f * cosf(an)), 30 + (int)lroundf(13.0f * sinf(an)), sh(R_IRON, 0.3f)); }
    for (int k = 0; k < 6; k++) { float an = k * 1.047f; cv_line(32 + 6.0f * cosf(an), 30 + 6.0f * sinf(an), 32 + 13.0f * cosf(an), 30 + 13.0f * sinf(an), sh(R_IRON, 0.35f)); }
    cv_flat(32, 30, 6, 6, sh(R_STONE, 0.5f));
    cv_outline(sh(R_IRON, 0.02f));
    cv_store(SP_PZ_WHEEL);
}

/* the Hollows' and the Deep's puzzle pieces */
static void spr_pz_board(void) {                                   /* a slate board on two legs, chalked */
    cv_begin(cv_sprite, TS, TS);
    cv_line(20, 63, 22, 40, sh(R_WOOD, 0.4f)); cv_line(44, 63, 42, 40, sh(R_WOOD, 0.4f));
    for (int y = 16; y < 42; y++) for (int x = 14; x < 50; x++) cv_px(x, y, x < 16 || x > 47 || y < 18 || y > 39 ? sh(R_WOOD, 0.45f) : sh(R_STONE, 0.08f + 0.03f * h01(x, y, 17)));
    for (int k = 0; k < 5; k++) for (int x = 19; x < 44; x++) if (h01(x, k, 19) > 0.35f) cv_px(x, 21 + k * 4, sh(R_BONE, 0.7f));   /* the chalk */
    cv_store(SP_PZ_BOARD);
}
static void spr_pz_box(void) {                                     /* a counterweight box of iron-bound timber, open-topped */
    cv_begin(cv_sprite, TS, TS);
    for (int y = 40; y < 64; y++) for (int x = 16; x < 48; x++) cv_px(x, y, (x - 16) % 8 == 0 || y == 40 || y == 63 ? sh(R_IRON, 0.4f) : sh(R_WOOD, 0.3f + 0.1f * h01(x, y, 3)));
    for (int y = 34; y < 40; y++) cv_line(18, y, 18, y, sh(R_IRON, 0.3f));
    cv_line(32, 0, 32, 40, sh(R_IRON, 0.35f));                        /* its chain up the shaft */
    cv_outline(sh(R_WOOD, 0.02f));
    cv_store(SP_PZ_BOX);
}
static void spr_pz_sack(void) {                                    /* a sack of ore, tied, tallied (the tally goes on it) */
    cv_begin(cv_sprite, TS, TS);
    cv_flat(32, 62, 11, 1.6f, R_WARM + 1);
    cv_ellipse(32, 53, 11, 9, R_WOOD, 0.3f, 0.65f);
    cv_ellipse(32, 44, 5, 3, R_WOOD, 0.35f, 0.6f);
    cv_line(28, 42, 36, 42, sh(R_WOOD, 0.1f));
    cv_outline(sh(R_WOOD, 0.02f));
    cv_store(SP_PZ_SACK);
}
static void spr_pz_hook(void) {                                    /* an iron hook on a strip of board */
    cv_begin(cv_sprite, TS, TS);
    for (int y = 14; y < 24; y++) for (int x = 20; x < 44; x++) cv_px(x, y, sh(R_WOOD, 0.3f + 0.1f * (y == 14)));
    cv_line(32, 24, 32, 30, sh(R_IRON, 0.5f)); cv_line(32, 30, 35, 28, sh(R_IRON, 0.5f));
    cv_store(SP_PZ_HOOK);
}
static void spr_pz_tag(void) {                                     /* a brass tag on a thong, a number stamped on it */
    cv_begin(cv_sprite, TS, TS);
    cv_flat(32, 62, 6, 1.2f, R_WARM + 1);
    cv_ellipse(32, 56, 5, 4, R_GOLD, 0.25f, 0.7f);
    cv_line(32, 52, 30, 46, sh(R_WOOD, 0.3f));
    cv_px(31, 56, sh(R_GOLD, 0.1f)); cv_px(33, 56, sh(R_GOLD, 0.1f));
    cv_store(SP_PZ_TAG);
}
static void spr_pz_socket(void) {                                  /* a hollow cut in the rock to take a bone */
    cv_begin(cv_sprite, TS, TS);
    cv_ellipse(32, 30, 12, 9, R_STONE, 0.02f, 0.12f);
    for (int a = 0; a < 40; a++) { float an = a * 0.157f; cv_px(32 + (int)lroundf(12.0f * cosf(an)), 30 + (int)lroundf(9.0f * sinf(an)), sh(R_STONE, 0.5f)); }
    cv_store(SP_PZ_SOCKET);
}
static void spr_pz_fossil(void) {                                  /* a great bone, stone-grey and old */
    cv_begin(cv_sprite, TS, TS);
    cv_flat(32, 62, 14, 1.8f, R_WARM + 1);
    cv_bone(18, 56, 46, 54, 2.6f, 0.25f, 0.6f);
    cv_outline(sh(R_STONE, 0.02f));
    cv_store(SP_PZ_FOSSIL);
}
static void spr_pz_stone(void) {                                   /* a standing stone, rough, a mark cut in its face */
    cv_begin(cv_sprite, TS, TS);
    float body[8] = { 22, 63, 42, 63, 39, 12, 25, 14 };
    cv_poly(body, 4, R_STONE, 0.5f, 0.25f, 12, 51);
    for (int y = 14; y < 63; y++) for (int x = 22; x < 43; x++) if (h01(x, y, 29) > 0.93f) cv_px(x, y, sh(R_MOSS, 0.4f));
    cv_outline(sh(R_STONE, 0.02f));
    cv_store(SP_PZ_STONE);
}
static void spr_pz_lamp(int id, int lit) {                         /* a miner's lamp on a nail, dark or burning */
    cv_begin(cv_sprite, TS, TS);
    cv_line(32, 8, 32, 18, sh(R_IRON, 0.4f));
    for (int y = 18; y < 34; y++) for (int x = 27; x < 38; x++) cv_px(x, y, (x - 27) % 3 == 0 || y == 18 || y == 33 ? sh(R_IRON, 0.45f) : (lit ? (uint8_t)(R_FLAME + 8 + (y > 24) * 4) : sh(R_IRON, 0.08f)));
    cv_store(id);
}
static void spr_pz_prism(int id, int turn) {                       /* a prism of clear crystal on a black plinth, its bright face turned */
    cv_begin(cv_sprite, TS, TS);
    for (int y = 50; y < 64; y++) for (int x = 24; x < 40; x++) cv_px(x, y, sh(R_IRON, 0.15f + 0.1f * (x < 27)));
    float tri[6] = { 22, 50, 42, 50, 32, 22 };
    cv_poly(tri, 3, R_GLOW, 0.25f, 0.55f, 22, 28);
    if (turn == 2) for (int y = 30; y < 50; y++) for (int x = 30 - (50 - y) / 6; x <= 34 + (50 - y) / 6; x++) cv_px(x, y, sh(R_GHOST, 0.85f));   /* its face, to you */
    else if (turn == 1) cv_line(36, 48, 40, 48, R_GLOW + 15), cv_line(34, 30, 41, 49, sh(R_GHOST, 0.9f));
    else if (turn == 3) cv_line(24, 48, 28, 48, R_GLOW + 15), cv_line(30, 30, 23, 49, sh(R_GHOST, 0.9f));
    else cv_line(32, 24, 32, 49, sh(R_GLOW, 0.2f));
    cv_outline(sh(R_IRON, 0.02f));
    cv_store(id);
}
static void spr_pz_geode(int id, int lit) {                        /* a split geode, dark inside, or full of light */
    cv_begin(cv_sprite, TS, TS);
    cv_ellipse(32, 50, 13, 12, R_STONE, 0.15f, 0.45f);
    for (int y = 42; y < 60; y++) for (int x = 24; x < 41; x++) if (hypotf(x - 32.0f, (y - 50.0f) * 1.2f) < 8.0f) cv_px(x, y, lit ? (uint8_t)(R_GLOW + 8 + ((x + y) & 3) * 2) : sh(R_PURPLE, 0.1f + 0.15f * ((x + y) & 1)));
    cv_outline(sh(R_STONE, 0.02f));
    cv_store(id);
}
static void spr_pz_throne(void) {                                  /* a throne of black glass, its back high, gold at its edges */
    cv_begin(cv_sprite, TS, TS);
    for (int y = 6; y < 64; y++) for (int x = 18; x < 46; x++) {
        int back = y < 40 && x > 21 && x < 43, seat = y >= 40 && y < 48, legs = y >= 48 && (x < 23 || x > 40);
        if (!back && !seat && !legs) continue;
        cv_px(x, y, (x == 18 || x == 45 || y == 6) ? sh(R_GOLD, 0.7f) : sh(R_IRON, 0.08f + 0.06f * (x < 25)));
    }
    cv_outline(sh(R_IRON, 0.02f));
    cv_store(SP_PZ_THRONE);
}
static void spr_pz_statue(void) {                                  /* a statue of the dead court in black glass: who it is goes on its breast */
    cv_begin(cv_sprite, TS, TS);
    for (int y = 56; y < 64; y++) for (int x = 24; x < 40; x++) cv_px(x, y, sh(R_IRON, 0.25f));
    float robe[8] = { 25, 56, 39, 56, 37, 22, 27, 22 };
    cv_poly(robe, 4, R_IRON, 0.35f, 0.1f, 22, 34);
    cv_ellipse(32, 17, 4.5f, 5.0f, R_IRON, 0.15f, 0.5f);
    cv_outline(sh(R_IRON, 0.01f));
    cv_store(SP_PZ_STATUE);
}
static void spr_pz_vent(int id, int open) {                        /* a vent in the rock, its iron shutter shut, or open with the cold breathing out */
    cv_begin(cv_sprite, TS, TS);
    for (int y = 34; y < 56; y++) for (int x = 20; x < 44; x++) cv_px(x, y, x < 22 || x > 41 || y < 36 || y > 53 ? sh(R_IRON, 0.4f) : (open ? sh(R_GHOST, 0.2f + 0.15f * ((x + y) & 1)) : sh(R_IRON, 0.2f + 0.08f * ((y - 36) % 4 == 0))));
    if (open) for (int i = 0; i < 20; i++) cv_px(24 + (int)(h01(i, 1, 31) * 16.0f), 20 + (int)(h01(i, 2, 31) * 14.0f), sh(R_GHOST, 0.6f));
    cv_store(id);
}
static void spr_pz_slab(void) {                                    /* a square of the grid: a low slab, flush with the floor */
    cv_begin(cv_sprite, TS, TS);
    for (int y = 58; y < 64; y++) for (int x = 14; x < 50; x++) cv_px(x, y, sh(R_STONE, 0.45f + 0.15f * (y == 58)));
    cv_store(SP_PZ_SLAB);
}
static void spr_pz_numstone(void) {                                /* a rounded stone with a number of pips cut on it */
    cv_begin(cv_sprite, TS, TS);
    cv_flat(32, 62, 9, 1.4f, R_WARM + 1);
    cv_ellipse(32, 54, 9, 8, R_STONE, 0.3f, 0.7f);
    cv_outline(sh(R_STONE, 0.02f));
    cv_store(SP_PZ_NUMSTONE);
}
static void spr_pz_letter(int id, int ch) {                        /* a letter in brass, from the BIOS font, centred at (32, 30) */
    cv_begin(cv_sprite, TS, TS);
    for (int r = 0; r < 8; r++)
        for (int c = 0; c < 8; c++)
            if (font_bios8['A' + ch][r] >> (7 - c) & 1) { cv_px(28 + c, 26 + r, sh(R_GOLD, 0.9f)); cv_px(29 + c, 27 + r, sh(R_GOLD, 0.25f)); }
    cv_store(id);
}

/* where the hidden message lies (1.0.0b13, docs/PLAN-b13.md section 6a), a run each: a niche's candle-box, a satchel
 * drowned in the silt, a miner's boot on its side, a geode broken open */
static void spr_hide(int id, int run) {
    cv_begin(cv_sprite, TS, TS);
    if (run == 0) {                                                /* oak, its lid lifted at one end, two stubs and their wax */
        cv_flat(32, 62, 13, 1.6f, R_WARM + 1);
        for (int y = 48; y < 62; y++)
            for (int x = 21; x < 43; x++)
                cv_px(x, y, sh(R_WOOD, 0.28f + 0.22f * (x < 24) - 0.1f * (y > 59) + 0.08f * h01(x, y / 3, 41) + 0.1f * ((y - 48) % 5 == 0)));
        for (int x = 20; x < 44; x++) {
            int top = 45 - (x > 31 ? (x - 31) / 4 : 0);
            for (int y = top; y < 48; y++) cv_px(x, y, y == top ? sh(R_WOOD, 0.68f) : (y < top + 2 ? sh(R_WOOD, 0.46f) : sh(R_WOOD, 0.04f)));   /* and the dark under it */
        }
        for (int y = 51; y < 55; y++) for (int x = 30; x < 34; x++) cv_px(x, y, sh(R_IRON, 0.25f + 0.3f * (x == 30)));   /* its hasp */
        for (int k = 0; k < 2; k++) {
            int cx = 25 + k * 7, h = 6 + 3 * k;
            for (int y = 45 - h; y < 45; y++) for (int x = cx - 1; x <= cx + 1; x++) cv_px(x, y, sh(R_BONE, 0.55f + 0.2f * (x == cx - 1)));
            cv_px(cx, 44 - h, sh(R_WOOD, 0.05f));
            for (int y = 48; y < 50 + 3 * k; y++) cv_px(cx + 1, y, sh(R_BONE, 0.5f));
        }
        cv_outline(sh(R_WOOD, 0.02f));
    } else if (run == 1) {                                         /* black with water, its strap trailing, half sunk */
        cv_flat(32, 61, 17, 2.6f, sh(R_STONE, 0.14f));
        cv_ellipse(32, 54, 12, 8, R_WOOD, 0.08f, 0.35f);
        float flap[8] = { 21, 49, 43, 49, 40, 57, 24, 57 };
        cv_poly(flap, 4, R_WOOD, 0.38f, 0.16f, 49, 8);
        for (int y = 54; y < 58; y++) for (int x = 31; x < 34; x++) cv_px(x, y, sh(R_IRON, 0.4f + 0.3f * (y == 54)));   /* the buckle */
        for (int t = 0; t < 2; t++) { cv_line(22, 51 + t, 17, 44 + t, sh(R_WOOD, 0.3f)); cv_line(17, 44 + t, 24, 39 + t, sh(R_WOOD, 0.3f)); cv_line(24, 39 + t, 30, 43 + t, sh(R_WOOD, 0.25f)); }
        for (int x = 24; x < 41; x += 3) cv_px(x, 49 + (x & 1), sh(R_GHOST, 0.55f));   /* wet */
        for (int y = 58; y < 63; y++)                              /* the silt over its foot */
            for (int x = 16; x < 49; x++) if (h01(x, y, 43) < 0.25f + 0.18f * (float)(y - 58)) cv_px(x, y, sh(R_STONE, 0.18f + 0.12f * h01(x, y, 44)));
        cv_outline(sh(R_WOOD, 0.01f));
    } else if (run == 2) {                                         /* hobnailed, its laces loose, the shaft's mouth open */
        cv_capsule(42, 52, 27, 52, 5.0f, R_WOOD, 0.18f, 0.5f);     /* the foot */
        cv_capsule(23, 50, 19, 38, 6.0f, R_WOOD, 0.15f, 0.48f);    /* the shaft */
        cv_ellipse(44, 52, 4.5f, 4.6f, R_WOOD, 0.25f, 0.62f);      /* the toe cap */
        cv_flat(19, 34, 5.0f, 2.2f, sh(R_WOOD, 0.02f));
        for (int x = 17; x < 48; x++) for (int y = 57; y < 60; y++) cv_px(x, y, sh(R_IRON, 0.08f + 0.04f * (y == 57)));   /* the sole */
        for (int x = 19; x < 47; x += 4) cv_px(x, 60, sh(R_IRON, 0.6f));   /* its nails */
        for (int k = 0; k < 4; k++) { cv_line(27 + k * 3, 47, 29 + k * 3, 50, sh(R_BONE, 0.35f)); cv_line(29 + k * 3, 47, 27 + k * 3, 50, sh(R_BONE, 0.3f)); }
        cv_line(28, 47, 25, 42, sh(R_BONE, 0.3f)); cv_line(25, 42, 27, 39, sh(R_BONE, 0.3f));   /* a lace undone */
        cv_outline(sh(R_WOOD, 0.01f));
    } else {                                                       /* two halves, the crystals inside it dull without light */
        cv_ellipse(28, 51, 11, 10, R_STONE, 0.15f, 0.45f);
        for (int y = 43; y < 60; y++)
            for (int x = 20; x < 37; x++) {
                float d = hypotf(x + 0.5f - 28.5f, (y + 0.5f - 51.0f) * 1.05f);
                if (d < 6.6f) cv_px(x, y, sh(R_PURPLE, 0.25f + 0.5f * h01(x / 2, y / 2, 47) + 0.15f * (d > 5.4f)));
                else if (d < 7.6f) cv_px(x, y, sh(R_BONE, 0.45f));
            }
        cv_ellipse(45, 58, 7, 5, R_STONE, 0.12f, 0.4f);
        for (int x = 40; x < 51; x++) cv_px(x, 53 + abs(x - 45) / 3, sh(R_PURPLE, 0.55f));
        for (int i = 0; i < 6; i++) cv_px(18 + (int)(h01(i, 1, 48) * 32.0f), 61 + (int)(h01(i, 2, 48) * 2.0f), sh(R_PURPLE, 0.5f));   /* shards */
        cv_outline(sh(R_STONE, 0.02f));
    }
    cv_store(id);
}
/* the Black Shrine: a block of black stone, a niche, a skull in it, a stub of candle before it; its eyes are drawn
 * over it at full brightness, faint, or flaring when it shows */
static void spr_bshrine(void) {
    cv_begin(cv_sprite, TS, TS);
    for (int y = 12; y < 64; y++)
        for (int x = 19; x < 45; x++) {
            float top = 19.0f - (12.0f - fabsf(x + 0.5f - 32.0f)) * 0.42f;
            if (y < top) continue;
            int edge = y < top + 1.5f || x == 19 || x == 44;
            cv_px(x, y, sh(R_IRON, (edge ? 0.2f : 0.03f + 0.05f * h01(x, y / 2, 51)) + 0.05f * (x < 22) + 0.03f * (y > 59)));
        }
    for (int y = 23; y < 48; y++)
        for (int x = 24; x < 40; x++) {
            float dx = x + 0.5f - 32.0f, top = 31.0f - sqrtf(fmaxf(0.0f, 64.0f - dx * dx));
            if (y < top) continue;
            cv_px(x, y, (y < top + 1.0f || x == 24 || x == 39) ? sh(R_IRON, 0.22f) : sh(R_PURPLE, 0.02f));
        }
    cv_skull(32, 37, 0.85f, 0.3f, 0.62f);
    for (int k = -1; k <= 1; k += 2) cv_flat(32 + k * 2.1f, 37.8f, 1.5f, 1.7f, sh(R_IRON, 0.0f));   /* the sockets, black */
    for (int x = 18; x < 46; x++) for (int y = 60; y < 64; y++) cv_px(x, y, sh(R_IRON, 0.12f + 0.08f * (y == 60)));   /* its step */
    cv_flat(32, 59.5f, 4.0f, 1.0f, sh(R_BONE, 0.45f));            /* the candle's wax pool, and the stub */
    for (int y = 54; y < 59; y++) for (int x = 31; x < 34; x++) cv_px(x, y, sh(R_BONE, 0.55f + 0.2f * (x == 31)));
    cv_px(32, 53, sh(R_WOOD, 0.05f));
    cv_px(32, 52, R_FLAME + 12); cv_px(32, 51, R_FLAME + 15);
    cv_outline(sh(R_IRON, 0.0f));
    cv_store(SP_BSHRINE);
}
static void spr_bshrine_eyes(int id, int flare) {
    cv_begin(cv_sprite, TS, TS);
    for (int k = -1; k <= 1; k += 2) {
        int ex = 32 + k * 2 - (k < 0), ey = 38;
        if (!flare) { cv_px(ex, ey, R_PURPLE + 13); cv_px(ex, ey - 1, R_PURPLE + 8); continue; }
        for (int r = 1; r <= 5; r++) {                             /* rays, thinning */
            uint8_t c = (uint8_t)(R_PURPLE + 15 - r * 2);
            cv_px(ex - r, ey, c); cv_px(ex + r, ey, c);
            if (r <= 3) { cv_px(ex, ey - r, c); cv_px(ex, ey + r, c); }
            if (r <= 2) { cv_px(ex - r, ey - r, c); cv_px(ex + r, ey - r, c); }
        }
        cv_px(ex, ey, C_WHITE); cv_px(ex + 1, ey, R_PURPLE + 15); cv_px(ex, ey - 1, R_PURPLE + 15);
    }
    cv_store(id);
}

/* ---- a document's page (1.0.0b13): a leaf of the Order's book of hours, a letter come in a bottle, a foreman's
 * scrap of sacking, a strip of beaten gold leaf ---- */
static uint8_t doc_shade(int run, float v) {
    if (run == 3) return sh(R_GOLD, v);
    return sh(run == 2 ? R_WOOD : R_BONE, v);
}
int crawl_doc_paper(int run, unsigned char c) {
    if (run == 3) return c >= R_GOLD && c < R_GOLD + NGOLD;
    if (run == 0 && c >= R_BLOOD && c < R_BLOOD + 16) return 1;   /* the vellum's red rules are page too */
    int base = run == 2 ? R_WOOD : R_BONE;
    return c >= base && c < base + 16;
}
unsigned char crawl_doc_page(unsigned char *pix, int stride, int x0, int y0, int w, int h, int run, unsigned seed) {
    int plain = run & 4;                                           /* (a slip: no square for a first letter) */
    run &= 3;
    for (int y = 0; y < h; y++)
        for (int x = 0; x < w; x++) {
            float e = 99.0f;                                       /* how far in from a ragged edge */
            float rag = run == 0 ? 2.5f : run == 1 ? 1.0f : run == 2 ? 2.0f : 1.6f;
            float dl = x - rag * (run == 0 ? 2.0f * fbm(y / 3.0f, 1.0f, 64, seed) + 1.0f : fbm(y / 4.0f, 2.0f, 64, seed));   /* torn from its binding */
            float dr = (w - 1 - x) - rag * fbm(y / 4.0f, 3.0f, 64, seed + 1);
            float dt = y - rag * fbm(x / 4.0f, 4.0f, 64, seed + 2), db = (h - 1 - y) - rag * fbm(x / 4.0f, 5.0f, 64, seed + 3);
            e = fminf(fminf(dl, dr), fminf(dt, db));
            if (e < 0.0f) continue;
            if (run == 2 && e < 3.0f && h01(x, y, seed + 9) < 0.35f) continue;   /* frayed */
            if (run == 3 && fbm(x / 16.0f, y / 12.0f, 64, seed + 5) > 0.7f) continue;   /* beaten through */
            float v;
            uint8_t c;
            if (run == 0) {
                v = 0.66f + 0.12f * (fbm(x / 13.0f, y / 11.0f, 64, seed + 6) - 0.5f) + 0.03f * (h01(x, y, seed) - 0.5f);
                if (e < 2.0f) v -= 0.18f;
                c = doc_shade(0, v);
                int ix = x - 8, iy = y - 6, rx = w - 7 - x, ry = h - 7 - y;
                if ((ix == 0 || ix == 2 || rx == 0 || rx == 2 || iy == 0 || iy == 2 || ry == 0 || ry == 2) && ix >= 0 && iy >= 0 && rx >= 0 && ry >= 0) c = sh(R_BLOOD, 0.5f);   /* ruled in red */
                if (x < 4 && (y % 22) == 11) c = sh(R_WOOD, 0.1f);  /* its sewing */
            } else if (run == 1) {
                v = 0.86f + 0.06f * (fbm(x / 17.0f, y / 13.0f, 64, seed + 6) - 0.5f);
                float tide = (0.14f + 0.12f * fbm(x / 40.0f, 9.0f, 64, seed + 7)) * (float)h, wet = (float)(h - y) - tide;   /* where the water came up to */
                if (wet < 0.0f) v -= 0.08f + 0.06f * fbm(x / 9.0f, y / 7.0f, 64, seed + 10);
                else if (wet < 1.2f) v -= 0.2f;
                else if (wet > 6.0f && wet < 7.0f && h01(x, 0, seed + 11) < 0.6f) v -= 0.08f;   /* and once higher, fainter */
                if (x == w / 2 || y == h / 3) v += 0.05f; else if (x == w / 2 + 1 || y == h / 3 + 1) v -= 0.07f;   /* its folds */
                c = doc_shade(1, v);
                float sx = (float)(x - (w - 22)), sy = (float)(y - (h - 20));
                float sd = hypotf(sx, sy) + 1.4f * (fbm(atan2f(sy, sx) * 1.5f + 3.0f, 1.0f, 64, seed + 8) - 0.5f);
                if (sd < 9.0f) c = sh(R_BLOOD, 0.35f + 0.25f * (sd < 6.5f && sd > 5.0f) + 0.15f * (sx < -1 && sy < -1));   /* the seal */
            } else if (run == 2) {
                int warp = (x & 1) ^ (y & 1), thread = (y % 3 == 0) || (x % 3 == 0);
                v = 0.52f + 0.1f * warp + 0.06f * thread + 0.1f * (fbm(x / 9.0f, y / 9.0f, 64, seed + 6) - 0.5f);
                if (hypotf((float)(x - w * 0.7f), (float)(y - h * 0.25f) * 1.3f) < 11.0f + 3.0f * fbm(x / 5.0f, y / 5.0f, 64, seed + 7)) v -= 0.12f;   /* grease */
                c = doc_shade(2, v);
            } else {
                float r = fbm(x / 5.0f, y / 3.0f, 64, seed + 6), ridge = 1.0f - fabsf(r - 0.5f) * 2.0f;   /* crinkled */
                v = 0.55f + 0.3f * ridge + 0.08f * (h01(x, y, seed) - 0.5f);
                if (e < 1.5f) v -= 0.2f;
                c = doc_shade(3, v);
            }
            pix[(y0 + y) * stride + x0 + x] = c;
        }
    if (run == 0 && !plain)                                        /* a gilt square for the first letter */
        for (int y = 10; y < 22 && y < h; y++) for (int x = 12; x < 24 && x < w; x++) pix[(y0 + y) * stride + x0 + x] = sh(R_GOLD, 0.55f + 0.25f * ((x + y) % 5 == 0));
    return run == 0 ? sh(R_WOOD, 0.03f) : run == 1 ? sh(R_IRON, 0.12f) : run == 2 ? sh(R_STONE, 0.08f) : sh(R_GOLD, 0.2f);
}
void crawl_doc_ink(unsigned char *pix, int stride, int x0, int y0, int w, int h, int run, unsigned char ink, unsigned seed) {
    if (run != 1 && run != 3) return;
    for (int y = h - 2; y >= 0; y--)
        for (int x = w - 2; x >= 0; x--) {
            uint8_t *p = &pix[(y0 + y) * stride + x0 + x];
            if (*p != ink) continue;
            if (run == 1) {                                        /* run a little, downward, where it was wet */
                uint8_t *q = p + stride;
                if (*q != ink && crawl_doc_paper(1, *q) && h01(x, y, seed) < 0.3f) *q = sh(R_IRON, 0.4f);
            } else {                                               /* scratched: a bright edge below and to the right */
                uint8_t *q = p + stride + 1;
                if (*q != ink && crawl_doc_paper(3, *q)) *q = sh(R_GOLD, 1.0f);
            }
        }
}

/* a rock, broken from the vault: drawn at the bottom of its square, so it can be dropped from above */
static void spr_rock(void) {
    cv_begin(cv_sprite, TS, TS);
    for (int y = 44; y < 64; y++)
        for (int x = 20; x < 44; x++) {
            float dx = (x + 0.5f - 32.0f) / 10.5f, dy = (y + 0.5f - 54.0f) / 8.5f, d = dx * dx + dy * dy;
            d += (fbm(x / 3.0f, y / 3.0f, 16, 77) - 0.5f) * 0.6f;
            if (d > 1.0f) continue;
            cv_px(x, y, sh(R_STONE, 0.25f + 0.45f * lit(dx, dy, sqrtf(fmaxf(0.0f, 1.0f - d))) + 0.1f * h01(x, y, 5)));
        }
    cv_outline(0);
    cv_store(SP_ROCK);
}
static void spr_rubble(void) {
    cv_begin(cv_sprite, TS, TS);
    cv_flat(32, 62, 13.0f, 1.6f, sh(R_STONE, 0.05f));
    for (int k = 0; k < 6; k++) {
        float cx = 22.0f + 20.0f * h01(k, 1, 31), cy = 59.0f + 3.0f * h01(k, 2, 31), r = 2.0f + 3.0f * h01(k, 3, 31);
        cv_ellipse(cx, cy, r * 1.2f, r, R_STONE, 0.2f, 0.65f);
    }
    for (int i = 0; i < 40; i++) cv_px(18 + (int)(h01(i, 5, 31) * 28.0f), 60 + (int)(h01(i, 6, 31) * 4.0f), sh(R_BONE, 0.3f));   /* dust */
    cv_outline(0);
    cv_store(SP_RUBBLE);
}

/* fire bursting from the floor's cracks: a narrow tongue the height of a man, flickering */
static void spr_erupt(int id, int frame) {
    cv_begin(cv_sprite, TS, TS);
    for (int y = 4; y < TS; y++)
        for (int x = 18; x < 46; x++) {
            float k = (float)(TS - y) / 60.0f;                      /* 0 at the floor, 1 at the top */
            float w = 11.0f * (1.0f - k * 0.75f) + 2.0f * sinf(y * 0.45f + frame * 2.1f);
            float dx = fabsf(x + 0.5f - 32.0f - 3.0f * sinf(y * 0.2f + frame * 1.7f) * k);
            if (dx > w) continue;
            float v = 1.0f - dx / w * 0.6f - k * 0.45f + 0.2f * (fbm(x / 4.0f, (y + frame * 9) / 4.0f, 16, 99) - 0.5f);
            if (h01(x, y + frame * 31, 5) > 0.9f - k * 0.2f && k > 0.5f) continue;   /* ragged at the top */
            cv_px(x, y, sh(R_FLAME, v));
        }
    cv_store(id);
}

/* the void's column: purple dark, thickest at the floor, with motes in it; the renderer draws it see-through */
static void spr_voidcol(void) {
    cv_begin(cv_sprite, TS, TS);
    for (int y = 0; y < TS; y++)
        for (int x = 14; x < 50; x++) {
            float k = (float)y / (TS - 1), dx = fabsf(x + 0.5f - 32.0f) / 18.0f;
            if (dx > 1.0f || h01(x, y, 61) > 0.35f + 0.6f * k * (1.0f - dx)) continue;
            cv_px(x, y, sh(R_PURPLE, 0.15f + 0.35f * k * (1.0f - dx)));
        }
    for (int i = 0; i < 26; i++) cv_px(18 + (int)(h01(i, 1, 63) * 28.0f), (int)(h01(i, 2, 63) * 60.0f), sh(R_PURPLE, 0.85f + 0.15f * h01(i, 3, 63)));
    cv_store(SP_VOIDCOL);
}

/* hands of the dark, rising: black, long-fingered, reaching */
static void spr_hands(void) {
    cv_begin(cv_sprite, TS, TS);
    for (int h = 0; h < 3; h++) {
        float bx = 20.0f + h * 12.0f, top = 34.0f + 6.0f * h01(h, 1, 71), lean = (h - 1) * 1.6f;
        cv_capsule(bx, 63, bx + lean, top + 8.0f, 2.6f, R_IRON, 0.02f, 0.2f);   /* the wrist */
        for (int f = 0; f < 4; f++) {
            float a = -0.9f + 0.6f * f + 0.15f * h, len = 7.0f + 2.0f * h01(h, f, 73);
            cv_capsule(bx + lean, top + 8.0f, bx + lean + sinf(a) * len, top + 8.0f - cosf(a) * len, 1.0f, R_IRON, 0.05f, 0.3f);
        }
    }
    cv_outline(0);
    cv_store(SP_HANDS);
}

/* a boss's Named Odious Scrip: a curl of old parchment on the floor, tied with a red cord under a
 * black seal pressed with a skull */
static void spr_scrip(void) {
    cv_begin(cv_sprite, TS, TS);
    cv_flat(32, 60.5f, 14.0f, 1.8f, sh(R_WARM, 0.05f));
    cv_capsule(19, 53, 45, 53, 5.0f, R_BONE, 0.35f, 0.85f);        /* the roll */
    for (int k = 0; k < 2; k++) {                                   /* its curled ends */
        float ex = k ? 45.5f : 18.5f;
        cv_ellipse(ex, 53, 2.2f, 5.2f, R_BONE, 0.2f, 0.7f);
        cv_flat(ex, 53, 0.9f, 2.2f, sh(R_WARM, 0.15f));
    }
    for (int y = 47; y <= 59; y++) { cv_px(30, y, sh(R_BLOOD, 0.55f)); cv_px(31, y, sh(R_BLOOD, 0.75f)); }   /* the cord */
    cv_ellipse(31, 53, 4.6f, 4.4f, R_IRON, 0.02f, 0.3f);           /* the seal, black wax */
    cv_flat(31, 52.6f, 2.0f, 1.8f, sh(R_IRON, 0.38f));             /* a skull pressed in it */
    cv_px(30, 52, sh(R_IRON, 0.05f)); cv_px(32, 52, sh(R_IRON, 0.05f));
    cv_px(31, 54, sh(R_IRON, 0.45f));
    cv_outline(0);
    cv_store(SP_SCRIP);
}

/* The Hexed Totem (1.0.0b7), in the merchant's back room: a black pole stacked with skulls whose eyes
 * burn cold, under an iron crossbar with four hooks. Each boss's scrip hangs from a hook once its ability
 * is unlocked; the game draws those on. */
static void spr_hextotem(void) {
    cv_begin(cv_sprite, TS, TS);
    float cx = 32;
    cv_flat(cx, 62, 11, 1.8f, R_WARM + 1);
    cv_flat(cx, 61.5f, 5, 1.6f, sh(R_IRON, 0.1f));                /* its foot, driven into the floor */
    cv_capsule(cx, 7, cx, 61, 2.5f, R_IRON, 0.04f, 0.38f);         /* the pole, black and old */
    cv_capsule(cx - 15, 17, cx + 15, 17, 1.3f, R_IRON, 0.08f, 0.5f);   /* the crossbar */
    for (int k = 0; k < 4; k++) {                                  /* its hooks */
        int hx = (int)(cx - 13 + k * 8.67f);
        for (int y = 18; y <= 22; y++) cv_px(hx, y, sh(R_IRON, 0.5f));
        cv_px(hx + 1, 23, sh(R_IRON, 0.45f)); cv_px(hx + 2, 22, sh(R_IRON, 0.4f));
    }
    for (int k = 0; k < 4; k++) cv_skull(cx, 55 - k * 9.0f, 0.78f - 0.04f * k, 0.18f, 0.72f);   /* stacked, smaller going up */
    cv_skull(cx, 8.5f, 0.62f, 0.22f, 0.8f);                       /* and one on top, over the bar */
    for (int k = 0; k < 5; k++) {                                  /* their eyes, burning cold */
        float y = k < 4 ? 55 - k * 9.0f + 0.7f : 9.1f, s = k < 4 ? 0.78f - 0.04f * k : 0.62f;
        uint8_t c = (uint8_t)(R_GLOW + 12 + (k & 1) * 2);
        cv_px((int)(cx - 2.4f * s), (int)y, c); cv_px((int)(cx + 2.4f * s), (int)y, c);
    }
    for (int y = 24; y < 60; y += 7) cv_px((int)cx - 3, y, sh(R_BLOOD, 0.4f));   /* old marks cut in the wood */
    cv_outline(0);
    cv_store(SP_HEXTOTEM);
}

/* The Corpus Compendium: a great tome open on a lectern of black oak, its pages close-written, a red
 * ribbon hanging from it, and a few motes of light over it from whatever it remembers */
/* The Lampstand (docs/PLAN-b11.md, section 8): six iron bowls on a black iron stand, fanned out from its stem on
 * curled arms, the elements' order left to right. Its flames are the game's to draw, each bowl's in its element's
 * colour once its line is open (LAMP_BOWL: where each sits) */
const signed char LAMP_BOWL[6][2] = { { 9, 30 }, { 16, 21 }, { 25, 14 }, { 39, 14 }, { 48, 21 }, { 55, 30 } };
static void spr_lampstand(void) {
    cv_begin(cv_sprite, TS, TS);
    cv_flat(32, 62, 13, 1.8f, R_WARM + 1);
    cv_capsule(32, 53, 20, 62, 1.6f, R_IRON, 0.04f, 0.26f);        /* the feet */
    cv_capsule(32, 53, 44, 62, 1.6f, R_IRON, 0.04f, 0.26f);
    cv_capsule(32, 54, 32, 62, 1.4f, R_IRON, 0.04f, 0.22f);
    cv_capsule(32, 13, 32, 56, 1.9f, R_IRON, 0.05f, 0.3f);         /* the stem */
    cv_capsule(28, 50, 36, 50, 1.3f, R_IRON, 0.06f, 0.32f);        /* a collar */
    static const float ARM[3][6] = { { 32, 45, 13, 41, 9, 33 }, { 32, 37, 18, 30, 16, 24 }, { 32, 27, 26, 22, 25, 17 } };
    for (int i = 0; i < 3; i++)
        for (int side = 0; side < 2; side++) {                     /* the arms, curling up to their bowls */
            float m = side ? -1.0f : 1.0f;
            const float *a = ARM[i];
            cv_capsule(32 + m * (a[0] - 32), a[1], 32 + m * (a[2] - 32), a[3], 1.1f, R_IRON, 0.05f, 0.28f);
            cv_capsule(32 + m * (a[2] - 32), a[3], 32 + m * (a[4] - 32), a[5], 1.0f, R_IRON, 0.05f, 0.28f);
        }
    for (int b = 0; b < 6; b++) {                                  /* the bowls, shallow, catching a little light on the rim */
        float bx = LAMP_BOWL[b][0], by = LAMP_BOWL[b][1] + 1;
        cv_flat(bx, by, 4.6f, 1.9f, sh(R_IRON, 0.12f));
        for (int x = (int)bx - 4; x <= (int)bx + 4; x++) cv_px(x, (int)by - 1, sh(R_IRON, 0.32f + 0.04f * (x - bx)));
        cv_px((int)bx, (int)by + 2, sh(R_IRON, 0.1f));
    }
    cv_capsule(32, 9, 32, 13, 0.9f, R_IRON, 0.1f, 0.38f);          /* a spike at the top */
    cv_outline(0);
    cv_store(SP_LAMPSTAND);
}

/* The Dead Bell (docs/PLAN-b11.md, section 12): a cracked bronze bell under a beam on two posts. Corrupted it goes
 * black, and the crack burns a sickly green */
static void spr_deadbell(int id, int corrupt) {
    cv_begin(cv_sprite, TS, TS);
    cv_flat(32, 62, 18, 1.8f, R_WARM + 1);
    cv_capsule(13, 12, 13, 62, 2.2f, R_WOOD, 0.08f, 0.42f);         /* the posts, */
    cv_capsule(51, 12, 51, 62, 2.2f, R_WOOD, 0.08f, 0.42f);
    cv_capsule(10, 11, 54, 11, 2.4f, R_WOOD, 0.1f, 0.5f);           /* the beam, */
    cv_capsule(16, 18, 13, 14, 1.0f, R_WOOD, 0.08f, 0.35f); cv_capsule(48, 18, 51, 14, 1.0f, R_WOOD, 0.08f, 0.35f);   /* braces */
    cv_capsule(32, 12, 32, 18, 1.0f, R_IRON, 0.06f, 0.3f);          /* the hanger */
    for (int y = 18; y <= 47; y++) {                                /* the bell: a crown, a waist, a flared lip */
        float u = (float)(y - 18) / 29.0f, w = 5.5f + 4.0f * u + 7.5f * u * u * u;
        for (int x = (int)(32 - w); x <= (int)(32 + w); x++) {
            float nx = (x + 0.5f - 32) / (w + 0.01f), l = lit(nx, -0.2f, sqrtf(fmaxf(0.0f, 1.0f - nx * nx)));
            uint8_t c = corrupt ? sh(R_STONE, 0.03f + 0.18f * l) : sh(R_GOLD, 0.22f + 0.6f * l);
            if (y >= 45 || (y > 20 && (y - 20) % 9 == 0)) c = corrupt ? sh(R_STONE, 0.08f + 0.2f * l) : sh(R_GOLD, 0.35f + 0.6f * l);   /* bands, the lip */
            cv_px(x, y, c);
        }
    }
    int cx = 27;                                                    /* the crack, down from the shoulder */
    for (int y = 22; y <= 46; y++) {
        cx += (int)(h01(y, 7, 13) * 3.0f) - 1;
        cv_px(cx, y, corrupt ? (uint8_t)(R_MOSS + 13 + (y & 1)) : sh(R_GOLD, 0.05f));
        if (corrupt && (y & 3) == 0) cv_px(cx + 1, y, (uint8_t)(R_MOSS + 10));
    }
    cv_flat(32, 50, 2.0f, 2.2f, corrupt ? sh(R_STONE, 0.1f) : sh(R_IRON, 0.3f));   /* the clapper */
    if (corrupt)
        for (int i = 0; i < 10; i++) cv_px((int)(20 + h01(i, 2, 17) * 24), (int)(48 + h01(i, 3, 17) * 10), (uint8_t)(R_MOSS + 9 + (i & 3)));   /* what drips from it */
    cv_outline(0);
    cv_store(id);
}
/* The Weeping Font: a stone basin on a short pillar, under a carved face on a slab. Dry, its tracks are dust; cursed, it
 * brims dark red and the face weeps red */
static void spr_font(int id, int cursed) {
    cv_begin(cv_sprite, TS, TS);
    cv_flat(32, 62, 18, 1.8f, R_WARM + 1);
    for (int y = 4; y <= 46; y++)                                   /* a broad slab, arched, a niche cut in it */
        for (int x = 8; x <= 56; x++) {
            float d = hypotf(x + 0.5f - 32, y + 0.5f - 26);
            if (y < 26 && d > 24.5f) continue;
            int niche = y >= 12 && x >= 17 && x <= 47 && (y >= 26 || hypotf(x + 0.5f - 32, y + 0.5f - 27) < 15.5f);
            float v = 0.22f + 0.22f * fbm(x / 4.0f, y / 4.0f, 32, 31) + (niche ? -0.1f : 0.0f) + (x < 11 || (y < 26 && d > 22.5f) ? 0.12f : 0.0f);
            cv_px(x, y, sh(R_STONE, v));
        }
    for (int a = 0; a < 40; a++) {                                  /* a band of carving round the arch */
        float an = PI_F + (float)a / 39.0f * PI_F;
        int x = (int)(32 + cosf(an) * 19.5f), y = (int)(26 + sinf(an) * 19.5f);
        cv_px(x, y, (a & 1) ? sh(R_STONE, 0.5f) : sh(R_STONE, 0.12f));
    }
    cv_flat(32, 26, 9.5f, 11.0f, sh(R_STONE, 0.3f));               /* the face in the niche: */
    cv_flat(28, 23, 2.4f, 1.5f, sh(R_STONE, 0.03f)); cv_flat(36, 23, 2.4f, 1.5f, sh(R_STONE, 0.03f));   /* hollow eyes, */
    cv_capsule(32, 24, 32, 29, 0.9f, R_STONE, 0.35f, 0.55f);       /* a nose, */
    cv_capsule(29, 32, 35, 32, 0.8f, R_STONE, 0.02f, 0.1f);        /* a mouth turned down */
    cv_px(28, 33, sh(R_STONE, 0.05f)); cv_px(36, 33, sh(R_STONE, 0.05f));
    for (int side = 0; side < 2; side++)                            /* the tears' tracks, down to the basin */
        for (int y = 25; y <= 41; y++) {
            int x = (side ? 36 : 28) + (int)(sinf((float)y * 0.5f + side) * 0.8f);
            cv_px(x, y, cursed ? (uint8_t)(R_BLOOD + 9 + (y & 1) * 2) : sh(R_STONE, 0.45f));
        }
    for (int y = 41; y <= 47; y++) {                                /* a shallow basin, wide */
        float u = (float)(y - 41) / 6.0f, w = 21.0f - 8.0f * u * u;
        for (int x = (int)(32 - w); x <= (int)(32 + w); x++) {
            float nx = (x + 0.5f - 32) / w;
            cv_px(x, y, sh(R_STONE, 0.2f + 0.4f * lit(nx, 0.3f, sqrtf(fmaxf(0.0f, 1.0f - nx * nx)))));
        }
    }
    for (int x = 11; x <= 53; x++) {                                /* its brim, and what is in it */
        cv_px(x, 41, sh(R_STONE, 0.62f));
        if (x > 12 && x < 52) cv_px(x, 42, cursed ? (uint8_t)(R_BLOOD + 4 + ((x * 7) % 5 == 0 ? 6 : 0)) : sh(R_STONE, 0.06f));
    }
    cv_capsule(32, 48, 32, 57, 5.5f, R_STONE, 0.12f, 0.42f);        /* a short thick stem, */
    cv_capsule(23, 59, 41, 59, 2.4f, R_STONE, 0.12f, 0.42f);        /* a broad foot */
    if (cursed) for (int i = 0; i < 5; i++) cv_px(14 + i * 9, 43 + (i & 1), (uint8_t)(R_BLOOD + 6));   /* spilling over the lip */
    cv_outline(0);
    cv_store(id);
}

/* a bowl's flame, in texels from the middle of the bowl's rim (dy up is negative): the step of its element's ramp
 * there, 0 none. Taller and fuller as the line ranks, swaying, the core brightest low down */
int crawl_lamp_flame(int dx, int dy, int rank, float t, int seed) {
    float h = 5.0f + 0.25f * (float)(rank < 20 ? rank : 20), u = (float)-dy / h;
    if (u < 0.0f || u > 1.0f) return 0;
    float sway = sinf(t * 9.0f + (float)seed * 1.7f) * 0.7f * u + sinf(t * 23.0f + (float)seed) * 0.3f * u;
    float w = (2.0f + 0.05f * (float)rank) * powf(sinf(3.14159f * (0.15f + 0.85f * u)), 0.7f) * (1.0f - 0.45f * u);
    float x = fabsf((float)dx - sway);
    if (x > w) return 0;
    float k = 1.0f - x / (w + 0.01f) * 0.6f - u * 0.5f;
    return 6 + (int)(k * 9.0f + 0.5f);
}

static void spr_lectern(void) {
    cv_begin(cv_sprite, TS, TS);
    float cx = 32;
    cv_flat(cx, 62, 11, 1.8f, R_WARM + 1);
    const float base[8] = { 22, 61, 42, 61, 39, 56, 25, 56 };
    cv_poly(base, 4, R_WOOD, 0.08f, 0.32f, 56, 6);                 /* its foot */
    cv_capsule(cx, 36, cx, 57, 2.6f, R_WOOD, 0.08f, 0.42f);        /* the stem */
    cv_capsule(cx - 6, 45, cx + 6, 45, 1.2f, R_WOOD, 0.1f, 0.4f);  /* a turned collar */
    const float board[8] = { 11, 35, 53, 35, 47, 27, 17, 27 };
    cv_poly(board, 4, R_WOOD, 0.14f, 0.42f, 27, 9);                /* the sloping top, nearer edge lower */
    for (int x = 11; x <= 53; x++) { cv_px(x, 36, sh(R_WOOD, 0.08f)); cv_px(x, 37, sh(R_WOOD, 0.05f)); }   /* its lip */
    const float cover[8] = { 14, 33, 50, 33, 46, 25, 18, 25 };
    cv_poly(cover, 4, R_BLOOD, 0.1f, 0.24f, 25, 8);                /* the binding, dark red leather */
    for (int side = 0; side < 2; side++) {                         /* the two pages, bowed up to the spine */
        for (int y = 24; y <= 32; y++) {
            float k = (y - 24) / 8.0f, xa = side ? 33.0f : 19.0f - 4.0f * k + 1.0f, xb = side ? 45.0f + 4.0f * k - 1.0f : 31.0f;
            for (int x = (int)xa; x <= (int)xb; x++) {
                float bow = side ? (x - 33) / 13.0f : (31 - x) / 13.0f;
                float v = 0.72f + 0.12f * (1.0f - bow) - (y == 24 ? 0.0f : 0.0f);
                if (((y - 25) % 2 == 0) && y < 32 && x > (int)xa + 1 && x < (int)xb - 1 && h01(x, y, 141 + side) > 0.3f) v -= 0.3f;   /* close-written */
                cv_px(x, y - (int)(1.5f * (1.0f - bow)), sh(R_BONE, v));
            }
        }
    }
    cv_line(32, 22, 32, 32, sh(R_BONE, 0.35f));                    /* the spine's fold */
    cv_line(36, 32, 37, 41, sh(R_BLOOD, 0.55f));                   /* the ribbon */
    cv_px(37, 42, sh(R_BLOOD, 0.4f)); cv_px(36, 42, sh(R_BLOOD, 0.4f));
    cv_outline(0);
    static const int MOTE[5][2] = { { 26, 17 }, { 35, 14 }, { 41, 19 }, { 30, 10 }, { 22, 21 } };
    for (int i = 0; i < 5; i++) cv_px(MOTE[i][0], MOTE[i][1], (uint8_t)(R_GLOW + 11 + (i & 1) * 3));   /* after the rim: they float free */
    cv_store(SP_LECTERN);
}

/* The merchant's map (1.0.0b9): a scroll half unrolled between its two rollers, a maze drawn on it in
 * brown ink, a red seal; it floats beside her, so it casts no shadow */
static void spr_mapware(void) {
    cv_begin(cv_sprite, TS, TS);
    for (int x = 17; x <= 47; x++) {                               /* the sheet, its edges a little torn */
        int top = 22 + (int)(1.6f * h01(x, 1, 151)) + (x % 9 == 4), bot = 44 - (int)(1.6f * h01(x, 2, 151)) - (x % 11 == 6);
        for (int y = top; y <= bot; y++) {
            float v = 0.74f + 0.12f * (fbm(x / 6.0f, y / 6.0f, 16, 153) - 0.5f) - 0.1f * ((y - top) < 1 || (bot - y) < 1);
            cv_px(x, y, sh(R_BONE, v));
        }
    }
    for (int i = 0; i < 9; i++) {                                  /* the maze on it */
        int x0 = 21 + (int)(h01(i, 3, 155) * 20.0f), y0 = 26 + (int)(h01(i, 4, 155) * 14.0f), len = 3 + (int)(h01(i, 5, 155) * 6.0f);
        if (i & 1) cv_line(x0, y0, fminf(x0 + len, 44), y0, sh(R_WOOD, 0.12f)); else cv_line(x0, y0, x0, fminf(y0 + len, 41), sh(R_WOOD, 0.12f));
    }
    cv_line(20, 25, 44, 25, sh(R_WOOD, 0.2f)); cv_line(20, 41, 44, 41, sh(R_WOOD, 0.2f));
    cv_line(20, 25, 20, 41, sh(R_WOOD, 0.2f)); cv_line(44, 25, 44, 41, sh(R_WOOD, 0.2f));
    cv_px(38, 37, sh(R_BLOOD, 0.75f)); cv_px(39, 37, sh(R_BLOOD, 0.75f)); cv_px(38, 36, sh(R_BLOOD, 0.6f));   /* you are here */
    for (int k = 0; k < 2; k++) {                                  /* the rollers, dark wood, their knobs */
        float rx = k ? 48.5f : 15.5f;
        cv_capsule(rx, 20, rx, 46, 2.6f, R_BONE, 0.38f, 0.7f);
        cv_ellipse(rx, 17.5f, 1.8f, 1.8f, R_WOOD, 0.1f, 0.45f);
        cv_ellipse(rx, 48.5f, 1.8f, 1.8f, R_WOOD, 0.1f, 0.45f);
    }
    cv_ellipse(32, 45.5f, 3.2f, 2.6f, R_BLOOD, 0.15f, 0.5f);       /* the seal hanging from it */
    cv_line(32, 43, 32, 44, sh(R_BLOOD, 0.3f));
    cv_outline(0);
    static const int MOTE[4][2] = { { 12, 30 }, { 52, 26 }, { 24, 14 }, { 42, 52 } };
    for (int i = 0; i < 4; i++) cv_px(MOTE[i][0], MOTE[i][1], (uint8_t)(R_GLOW + 10 + (i & 1) * 3));
    cv_store(SP_MAPWARE);
}

/* Severance: four desks pushed together behind low green partitions, an old terminal on each, numbers
 * on their dark screens. Drawn in R_FLOOR and R_CEIL, so only a Severance pocket shows it true. */
static void spr_desks(void) {
    cv_begin(cv_sprite, TS, TS);
    static const int MX[2][2] = { { 9, 27 }, { 36, 54 } };          /* two terminals showing over the partitions */
    for (int m = 0; m < 2; m++) {
        int x0 = MX[m][0], x1 = MX[m][1], y0 = m ? 24 : 22, y1 = 39;
        for (int y = y0; y <= y1; y++)
            for (int x = x0; x <= x1; x++) {
                float v = 0.80f - (x - x0) * 0.012f;
                if (x == x0 || y == y0) v = 0.92f;
                if (x == x1) v = 0.55f;
                uint8_t c = sh(R_CEIL, v);
                if (x >= x0 + 3 && x <= x1 - 3 && y >= y0 + 3 && y <= y1 - 4) {   /* the screen */
                    c = sh(R_IRON, 0.08f);
                    if ((y - y0) % 3 == 1 && h01(x, y, 611 + m) > 0.45f) c = sh(R_GHOST, 0.55f + 0.3f * h01(x, y, 612));
                }
                cv_px(x, y, c);
            }
    }
    for (int y = 36; y < 62; y++)                                   /* the partitions */
        for (int x = 3; x <= 60; x++) {
            uint8_t c = sh(R_FLOOR, 0.56f + 0.05f * h01(x, y, 613) - (y > 56 ? 0.08f : 0.0f));
            if (y <= 37) c = sh(R_CEIL, y == 36 ? 0.94f : 0.72f);     /* the white rail along the top */
            if (x <= 4 || x >= 59 || x == 31 || x == 32) c = sh(R_CEIL, x <= 4 || x == 31 ? 0.86f : 0.6f);   /* posts */
            cv_px(x, y, c);
        }
    for (int x = 3; x <= 60; x += 1) { cv_px(x, 62, sh(R_IRON, 0.3f)); cv_px(x, 63, sh(R_IRON, 0.18f)); }   /* feet */
    cv_outline(0);
    cv_store(SP_DESKS);
}

/* Severance's water cooler: a white cabinet, red and blue taps, cups at its side, and its bottle upside
 * down on top, full, or lower after the drink */
static void spr_cooler(int id, int low) {
    cv_begin(cv_sprite, TS, TS);
    for (int y = 36; y < 63; y++)                                   /* the cabinet */
        for (int x = 22; x <= 41; x++) {
            float v = 0.92f - (x - 22) * 0.016f;
            if (y == 36) v = 0.96f;
            uint8_t c = sh(R_CEIL, v);
            if (y >= 40 && y <= 49 && x >= 25 && x <= 38) c = sh(R_CEIL, y == 40 ? 0.3f : 0.45f);   /* the recess */
            if (y >= 48 && y <= 49 && x >= 26 && x <= 37) c = sh(R_IRON, 0.5f);   /* its drip tray */
            cv_px(x, y, c);
        }
    for (int k = 0; k < 2; k++) {                                   /* the taps, hot and cold */
        int tx = k ? 34 : 28;
        for (int y = 41; y <= 44; y++) for (int x = tx; x <= tx + 1; x++) cv_px(x, y, k ? sh(R_GHOST, 0.45f + 0.15f * (x == tx)) : sh(R_BLOOD, 0.55f + 0.2f * (x == tx)));
    }
    for (int y = 39; y <= 52; y++) for (int x = 43; x <= 45; x++) cv_px(x, y, sh(R_CEIL, x == 43 ? 0.95f : 0.7f));   /* the cups */
    cv_px(44, 53, sh(R_CEIL, 0.6f));
    for (int y = 30; y <= 35; y++) for (int x = 29; x <= 34; x++) cv_px(x, y, sh(R_GHOST, 0.6f + 0.15f * (x <= 30)));   /* its neck */
    float level = low ? 24.0f : 12.0f;                              /* the water's top, the bottle being upside down */
    for (int y = 8; y < 30; y++)
        for (int x = 21; x <= 42; x++) {
            float nx = (x + 0.5f - 31.5f) / 10.5f, ny = (y + 0.5f - 19.0f) / 11.0f;
            if (nx * nx * nx * nx + ny * ny * ny * ny > 1.0f) continue;   /* a rounded square, as such bottles are */
            float l = lit(nx, ny, sqrtf(fmaxf(0.0f, 1.0f - nx * nx)));
            float v = y >= level ? 0.42f + 0.25f * l : 0.88f + 0.1f * l;   /* water below, bare plastic above */
            if (y == 14 || y == 20 || y == 26) v += 0.06f;           /* its ribs */
            if ((int)level == y) v = 0.96f;                         /* the line of the water */
            cv_px(x, y, sh(R_GHOST, v));
        }
    cv_outline(0);
    cv_store(id);
}

/* An Exquisite Blink's double: the caster as the creatures see it, hooded and robed, one hand up,
 * all in violet light that thins towards the hem. Drawn at full brightness, and faded as it goes. */
static void spr_double(void) {
    cv_begin(cv_sprite, TS, TS);
    for (int y = 17; y < 63; y++) {                                 /* the robe */
        float t = (y - 17) / 45.0f, hw = 8.0f + 6.5f * t;
        for (int x = (int)(32 - hw) - 1; x <= (int)(32 + hw) + 1; x++) {
            float dx = x + 0.5f - 32;
            if (fabsf(dx) > hw) continue;
            if (y > 48 && h01(x, y, 351) < (y - 48) / 15.0f) continue;   /* thinning away below */
            float u = dx / hw, v = 0.35f + 0.45f * (1.0f - u * u) * (1.0f - 0.4f * t);
            if (sinf(dx * 1.2f + y * 0.04f) > 0.65f) v -= 0.1f;     /* folds */
            cv_px(x, y, sh(R_PURPLE, v));
        }
    }
    cv_capsule(24, 21, 21, 38, 3.2f, R_PURPLE, 0.3f, 0.7f);         /* the left arm, hanging */
    cv_ellipse(21, 39, 1.8f, 1.8f, R_PURPLE, 0.6f, 0.95f);
    cv_capsule(40, 21, 46, 12, 3.0f, R_PURPLE, 0.3f, 0.75f);        /* the right, raised as it cast */
    cv_ellipse(47, 10, 2.0f, 2.0f, R_PURPLE, 0.7f, 1.0f);
    for (int k = 0; k < 10; k++)                                    /* the spell's last sparks about the hand */
        cv_px((int)(47 + (h01(k, 1, 353) - 0.5f) * 9), (int)(9 - 7 * h01(k, 2, 353)), sh(R_PURPLE, 0.85f + 0.15f * h01(k, 3, 353)));
    cv_ellipse(32, 12, 7.5f, 8.0f, R_PURPLE, 0.35f, 0.8f);          /* the hood, and only shadow in it */
    cv_flat(32, 13.5f, 4.8f, 5.6f, 0);
    cv_outline(0);
    cv_store(SP_DOUBLE);
}

/* The wraith: a tall hooded shape, nothing inside the hood but two cold eyes, bone hands reaching
 * out of the sleeves, the hem of its robe dissolving into wisps. recoil: thrown back, shrieking. */
static void spr_wraith(int id, float phase, int recoil) {
    cv_begin(cv_sprite, TS, TS);
    float lift = recoil ? -3.0f : 0.0f;
    for (int y = 1; y < TS; y++)                                    /* the robe */
        for (int x = 0; x < TS; x++) {
            float dx = x + 0.5f - 32.0f, yy = y + 0.5f - lift;
            float hw;
            if (yy < 14.0f) {                                         /* a tall, pointed hood */
                float hy = (yy - 11.0f) / 9.5f; hw = 10.0f * sqrtf(fmaxf(0.0f, 1.0f - hy * hy));
                if (yy < 7.0f) hw *= (yy - 0.5f) / 6.5f;
                if (yy < 1.0f) continue;
            }
            else hw = 11.0f + (yy - 14.0f) * (recoil ? 0.42f : 0.36f);
            float hem = 54.0f + 4.0f * sinf(dx * 0.55f + phase) + 3.0f * sinf(dx * 1.3f - phase * 1.7f);
            if (fabsf(dx) > hw || yy > hem + 6.0f) continue;
            if (yy > hem && h01(x, y, 19 + (int)(phase * 3.0f)) > (hem + 6.0f - yy) / 6.0f) continue;   /* frays into wisps */
            float u = dx / hw;
            float v = 0.06f + 0.16f * (1.0f - u * u) - (yy - 14.0f) * 0.002f;
            if (fmodf(dx * 0.35f + 20.0f + sinf(yy * 0.1f + phase) * 0.6f, 2.0f) < 0.35f) v -= 0.05f;   /* folds */
            if (fabsf(u) > 0.88f) v += 0.3f;                        /* a cold rim of light */
            if (yy < 14.0f && yy < 11.0f - 9.5f * sqrtf(fmaxf(0.0f, 1.0f - (dx / 10.0f) * (dx / 10.0f))) + 2.5f) v += 0.3f;   /* the hood's crown */
            cv_px(x, y, sh(R_GHOST, v));
        }
    for (int y = 6; y < 24; y++)                                    /* the hood's dark mouth */
        for (int x = 23; x < 42; x++) {
            float nx = (x + 0.5f - 32.0f) / 6.2f, ny = (y + 0.5f - 15.5f - lift) / (recoil ? 8.0f : 7.0f);
            if (nx * nx + ny * ny <= 1.0f) cv_px(x, y, 0);
        }
    int ey = (int)(14.0f + lift);                                   /* eyes */
    for (int k = -1; k <= 1; k += 2) {
        int ex = 32 + k * 3 - (k < 0 ? 1 : 0);
        cv_px(ex, ey, C_WHITE); cv_px(ex + 1, ey, (uint8_t)(R_GLOW + 15));
        cv_px(ex, ey + 1, (uint8_t)(R_GLOW + 13)); cv_px(ex + 1, ey + 1, (uint8_t)(R_GLOW + 12));
    }
    if (recoil) for (int y = ey + 4; y < ey + 9; y++) for (int x = 30; x < 35; x++) cv_px(x, y, 0);   /* the shriek */
    for (int s = -1; s <= 1; s += 2) {                              /* sleeves and bone hands */
        float sx = 32.0f + s * 9.0f, sy = 20.0f + lift;
        float hx = recoil ? 32.0f + s * 25.0f : 32.0f + s * 19.0f, hy = recoil ? 6.0f + lift : 30.0f + 2.0f * sinf(phase + s);
        cv_capsule(sx, sy, hx - s * 2.0f, hy, 3.4f, R_GHOST, 0.15f, 0.55f);
        for (int f = -1; f <= 1; f++) {
            float fa = (recoil ? -1.6f : 0.1f) + f * 0.35f;
            float tx = hx + s * cosf(fa) * 5.0f, ty = hy + sinf(fa) * 5.0f + (recoil ? 0.0f : 2.0f);
            cv_line(hx, hy, tx, ty, sh(R_BONE, 0.75f));
            cv_px((int)tx, (int)ty, sh(R_BONE, 0.95f));
        }
        cv_ellipse(hx, hy, 1.8f, 1.6f, R_BONE, 0.4f, 0.9f);
    }
    cv_store(id);
}

static void spr_bat(int id, int frame) {
    cv_begin(cv_sprite, TS, TS);
    if (frame == 2) {                                           /* dead: folded, falling */
        cv_ellipse(32, 40, 3.5f, 4.5f, R_IRON, 0.05f, 0.3f);
        float wl[6] = { 29, 36, 24, 50, 30, 46 }, wr[6] = { 35, 36, 40, 50, 34, 46 };
        cv_poly(wl, 3, R_PURPLE, 0.2f, 0.05f, 36, 14); cv_poly(wr, 3, R_PURPLE, 0.2f, 0.05f, 36, 14);
        cv_store(id);
        return;
    }
    float up = frame == 0 ? 1.0f : -1.0f;
    for (int s = -1; s <= 1; s += 2) {
        float p[18] = { 32 + s * 3, 27, 32 + s * 9, 22 - 4 * up, 32 + s * 19, 19 - 6 * up, 32 + s * 24, 24 - 4 * up,
                        32 + s * 21, 28 - 2 * up, 32 + s * 17, 26 - 1 * up, 32 + s * 13, 31, 32 + s * 9, 28, 32 + s * 4, 33 };
        cv_poly(p, 9, R_PURPLE, 0.28f, 0.06f, 18, 16);
    }
    cv_ellipse(32, 30, 3.6f, 5.0f, R_IRON, 0.05f, 0.35f);
    cv_ellipse(32, 24, 2.8f, 2.6f, R_IRON, 0.08f, 0.4f);
    cv_px(30, 21, R_IRON + 3); cv_px(34, 21, R_IRON + 3); cv_px(30, 20, R_IRON + 2); cv_px(34, 20, R_IRON + 2);
    cv_px(31, 24, R_FLAME + 6); cv_px(33, 24, R_FLAME + 6);
    cv_store(id);
}

static void spr_fire(int id, int frame) {
    cv_begin(cv_sprite, TS, TS);
    float cx = 32, cy = 33, r = 7.5f;
    for (int y = 18; y < 48; y++)
        for (int x = 16; x < 48; x++) {
            float dx = x + 0.5f - cx, dy = y + 0.5f - cy, d = sqrtf(dx * dx + dy * dy);
            float n = fbm(x / 3.0f + frame * 5.0f, y / 3.0f, 32, 61) * 0.6f;
            float rr = r * (0.85f + n);
            if (d > rr) continue;
            float heat = 1.05f - d / rr * 0.75f;
            cv_px(x, y, (uint8_t)(R_FLAME + clampi((int)(heat * 15), 3, 15)));
        }
    for (int i = 0; i < 10; i++) {
        float a = h01(i, frame, 7) * TAU_F, l = r + 2 + h01(i, frame + 3, 7) * 5;
        cv_px((int)(cx + cosf(a) * l), (int)(cy + sinf(a) * l), (uint8_t)(R_FLAME + 8 + (i & 3)));
    }
    cv_store(id);
}

static void spr_boom(int id, int frame) {
    cv_begin(cv_sprite, TS, TS);
    float cx = 32, cy = 32, r = 9.0f + frame * 6.5f;
    for (int y = 0; y < TS; y++)
        for (int x = 0; x < TS; x++) {
            float dx = x + 0.5f - cx, dy = y + 0.5f - cy, d = sqrtf(dx * dx + dy * dy);
            float n = fbm(x / 3.0f, y / 3.0f + frame * 7.0f, 32, 99 + frame) * 0.7f;
            float edge = r * (0.75f + n * 0.5f);
            if (d > edge) continue;
            float heat = (1.0f - d / edge) * (1.2f - frame * 0.32f) + n * 0.3f - (frame == 2 ? 0.25f : 0.0f);
            if (frame == 2 && h01(x, y, 5) > 0.55f) continue;
            if (heat < 0.12f) { if (frame > 0) cv_px(x, y, sh(R_WARM, 0.15f + n * 0.3f)); continue; }
            cv_px(x, y, (uint8_t)(R_FLAME + clampi((int)(heat * 15), 1, 15)));
        }
    cv_store(id);
}

static void spr_bonefly(int id, float ang) {
    cv_begin(cv_sprite, TS, TS);
    float c = cosf(ang) * 8.0f, s = sinf(ang) * 8.0f;
    cv_bone(32 - c, 34 - s, 32 + c, 34 + s, 1.3f, 0.3f, 0.95f);
    cv_outline(0);
    cv_store(id);
}

/* ---- treasure ---- */

static void spr_coins(int id, int frame) {
    cv_begin(cv_sprite, TS, TS);
    cv_flat(32, 62, 17, 2.2f, R_WARM + 1);
    for (int i = 0; i < 34; i++) {                                  /* a mound of coins lying flat */
        float r = h01(i, 1, 40) * 13.0f, a = h01(i, 2, 40) * TAU_F;
        float x = 32 + cosf(a) * r, y = 60 - (13.0f - r) * 0.55f - h01(i, 3, 40) * 1.5f + sinf(a) * r * 0.18f;
        cv_ellipse(x, y, 3.2f, 1.3f, R_GOLD, 0.35f, 1.0f);
        cv_px((int)(x - 1), (int)y, sh(R_GOLD, 0.95f));
    }
    for (int i = 0; i < 3; i++) {                                   /* a few on edge */
        float x = 22 + i * 9.5f, y = 56 - (i == 1 ? 6 : 0);
        cv_ellipse(x, y, 1.4f, 3.0f, R_GOLD, 0.3f, 0.95f);
    }
    cv_outline(R_GOLD);
    static const int G[2][3][2] = { { {27, 52}, {38, 55}, {33, 49} }, { {30, 54}, {24, 57}, {40, 52} } };
    for (int i = 0; i < 3; i++) cv_px(G[frame][i][0], G[frame][i][1], C_WHITE);
    cv_store(id);
}

static void spr_chalice(void) {
    cv_begin(cv_sprite, TS, TS);
    cv_flat(32, 62, 10, 1.8f, R_WARM + 1);
    cv_ellipse(32, 60.5f, 7.0f, 1.8f, R_GOLD, 0.3f, 0.9f);
    cv_capsule(32, 59, 32, 49, 1.4f, R_GOLD, 0.3f, 1.0f);
    cv_ellipse(32, 53, 2.6f, 1.5f, R_GOLD, 0.35f, 1.0f);
    for (int y = 37; y <= 48; y++)                                  /* the cup */
        for (int x = 22; x <= 42; x++) {
            float u = (x + 0.5f - 32) / 9.0f, v = (y + 0.5f - 37) / 11.5f;
            if (u * u + v * v > 1.0f) continue;
            cv_px(x, y, sh(R_GOLD, 0.3f + 0.7f * lit(u, v * 0.5f, sqrtf(fmaxf(0.0f, 1 - u * u - v * v)))));
        }
    cv_ellipse(32, 37.5f, 9.0f, 2.0f, R_GOLD, 0.15f, 0.55f);
    cv_flat(32, 37.8f, 7.2f, 1.2f, R_BLOOD + 3);
    cv_flat(32, 43, 1.6f, 1.8f, R_BLOOD + 12); cv_px(31, 42, R_BLOOD + 15);
    cv_outline(0);
    cv_store(SP_CHALICE);
}

static void gem(float cx, float cy, float r, int ramp) {
    for (int y = (int)(cy - r); y <= (int)(cy + r); y++)
        for (int x = (int)(cx - r); x <= (int)(cx + r); x++) {
            float dx = (x + 0.5f - cx) / r, dy = (y + 0.5f - cy) / r;
            if (fabsf(dx) + fabsf(dy) * 0.8f > 1.0f) continue;
            float v = dx < 0 && dy < 0 ? 0.95f : (dx >= 0 && dy < 0 ? 0.7f : (dx < 0 ? 0.5f : 0.3f));
            if (fabsf(dx) < 0.3f && fabsf(dy) < 0.3f) v = 0.85f;
            cv_px(x, y, sh(ramp, v));
        }
}

static void spr_gems(void) {
    cv_begin(cv_sprite, TS, TS);
    cv_flat(32, 62, 14, 2.0f, R_WARM + 1);
    for (int i = 0; i < 12; i++) {
        float x = 22 + h01(i, 4, 41) * 20, y = 60 - h01(i, 5, 41) * 2.5f;
        cv_ellipse(x, y, 3.0f, 1.2f, R_GOLD, 0.35f, 0.95f);
    }
    gem(25, 53, 4.5f, R_BLOOD); gem(37, 52, 5.0f, R_GLOW); gem(31, 56, 4.0f, R_MOSS); gem(42, 57, 3.2f, R_PURPLE);
    cv_outline(0);
    cv_px(23, 51, C_WHITE); cv_px(35, 49, C_WHITE);
    cv_store(SP_GEMS);
}

static void spr_crown(void) {
    cv_begin(cv_sprite, TS, TS);
    cv_flat(32, 62, 14, 2.0f, R_WARM + 1);
    for (int y = 48; y <= 60; y++)                                  /* the band, seen as a cylinder */
        for (int x = 19; x <= 45; x++) {
            float u = (x + 0.5f - 32) / 13.5f;
            float v = 0.35f + 0.6f * lit(u, 0.0f, sqrtf(fmaxf(0.0f, 1 - u * u)));
            if (y == 48 || y == 60) v += 0.15f;
            if (y == 54) v -= 0.15f;
            cv_px(x, y, sh(R_GOLD, v));
        }
    for (int i = 0; i < 5; i++) {                                   /* points with pearls */
        float x = 21 + i * 5.5f, hgt = i == 2 ? 15 : (i == 1 || i == 3 ? 11 : 8);
        float tri[6] = { x - 3.2f, 48.5f, x + 3.2f, 48.5f, x, 48.5f - hgt };
        cv_poly(tri, 3, R_GOLD, 0.95f, 0.45f, 48.5f - hgt, hgt);
        cv_ellipse(x, 48.5f - hgt, 1.6f, 1.6f, R_BONE, 0.6f, 1.0f);
    }
    gem(32, 54, 2.8f, R_BLOOD); gem(24, 54, 2.2f, R_GLOW); gem(40, 54, 2.2f, R_MOSS);
    cv_outline(0);
    cv_store(SP_CROWN);
}

static void spr_chest(void) {
    cv_begin(cv_sprite, TS, TS);
    cv_flat(32, 62, 22, 2.2f, R_WARM + 1);
    float lid[8] = { 14, 47, 50, 47, 46, 31, 18, 31 };              /* the open lid, behind */
    cv_poly(lid, 4, R_WOOD, 0.35f, 0.15f, 31, 16);
    for (int x = 18; x <= 46; x++) cv_px(x, 31, sh(R_GOLD, 0.7f));
    for (int i = 0; i < 16; i++) {                                  /* the hoard, heaped inside */
        float x = 17 + h01(i, 6, 42) * 30, y = 46 - h01(i, 7, 42) * 4 * (1.0f - fabsf(x - 32) / 18.0f);
        cv_ellipse(x, y, 3.2f, 1.4f, R_GOLD, 0.4f, 1.0f);
    }
    for (int y = 47; y <= 61; y++)                                  /* the box */
        for (int x = 13; x <= 51; x++) {
            int plank = (y - 47) / 5;
            float v = 0.45f + 0.08f * h01(plank, 1, 43) + fbm(x / 6.0f, y / 2.0f, 16, 44) * 0.12f - (y == 47 + plank * 5 ? 0.15f : 0.0f);
            uint8_t c = sh(R_WOOD, v);
            if (x == 13 || x == 14 || x == 50 || x == 51 || y == 47 || y == 61) c = sh(R_IRON, 0.55f + (y == 47 ? 0.25f : 0.0f));
            if (x >= 29 && x <= 35 && y >= 49 && y <= 56) c = sh(R_GOLD, (x == 29 || y == 49) ? 1.0f : 0.65f);
            if (x == 32 && (y == 52 || y == 53)) c = 0;
            cv_px(x, y, c);
        }
    cv_outline(0);
    cv_px(24, 43, C_WHITE); cv_px(39, 44, C_WHITE);
    cv_store(SP_CHEST);
}

static void spr_key(int id, float scale, int metal) {
    cv_begin(cv_sprite, TS, TS);
    #define KX(x) (32.0f + ((x) - 32.0f) * scale)
    float ar = fmaxf(fabsf(scale), 0.18f);
    float lo = metal == R_GOLD ? 0.35f : 0.45f;
    cv_ellipse(KX(21), 38, 6.5f * ar, 6.5f, metal, lo, 1.0f);       /* the bow */
    cv_flat(KX(21), 38, 3.0f * ar, 3.0f, C_CLEAR);
    cv_capsule(KX(26), 38, KX(47), 38, 2.0f, metal, lo, 1.0f);       /* the shank */
    cv_capsule(KX(43), 38, KX(43), 45, 1.7f, metal, lo - 0.05f, 0.95f);   /* the bit */
    cv_capsule(KX(47), 38, KX(47), 44, 1.7f, metal, lo - 0.05f, 0.95f);
    cv_ellipse(KX(29), 38, 2.0f * ar, 3.0f, metal, 0.5f, 1.0f);      /* collar */
    #undef KX
    cv_outline(sh(metal, 0.05f));
    cv_store(id);
}

static void spr_potion(void) {
    cv_begin(cv_sprite, TS, TS);
    cv_flat(32, 62, 10, 1.8f, R_WARM + 1);
    for (int y = 42; y <= 61; y++)
        for (int x = 22; x <= 42; x++) {
            float u = (x + 0.5f - 32) / 9.5f, v = (y + 0.5f - 52.5f) / 9.0f;
            if (u * u + v * v > 1.0f) continue;
            float l = lit(u, v, sqrtf(fmaxf(0.0f, 1 - u * u - v * v)));
            cv_px(x, y, y >= 47 ? sh(R_BLOOD, 0.35f + 0.65f * l) : sh(R_STONE, 0.25f + 0.3f * l));
        }
    for (int y = 34; y <= 43; y++)
        for (int x = 29; x <= 35; x++) cv_px(x, y, sh(R_STONE, x == 29 ? 0.75f : (x == 35 ? 0.25f : 0.45f)));
    for (int y = 30; y <= 34; y++)
        for (int x = 29; x <= 35; x++) cv_px(x, y, sh(R_WOOD, 0.4f + (y == 30 ? 0.3f : 0.0f)));
    cv_outline(0);
    cv_px(27, 50, C_WHITE); cv_px(26, 51, C_WHITE); cv_px(28, 49, C_WHITE);
    cv_store(SP_POTION);
}

/* ------------------------------------------------------------------ */
/* Set dressing: things that hang from the ceiling, graves, fire        */
/* ------------------------------------------------------------------ */

/* a chain straight down from y0 to y1: links alternately edge-on and face-on */
static void chain_down(float x, int y0, int y1) {
    for (int y = y0; y < y1; y += 4) {
        if (((y - y0) / 4) & 1) cv_capsule(x, (float)y, x, (float)(y + 4), 0.9f, R_IRON, 0.2f, 0.85f);
        else for (int a = 0; a < 16; a++) {
            float ang = TAU_F * a / 16;
            cv_px((int)(x + 0.5f + 2.0f * cosf(ang)), (int)(y + 2.5f + 2.6f * sinf(ang)), sh(R_IRON, 0.45f - 0.3f * sinf(ang + 0.7f)));
        }
    }
}
/* a thin chain on a slant: beads of iron, light and dark */
static void chain_line(float x0, float y0, float x1, float y1) {
    int n = (int)(hypotf(x1 - x0, y1 - y0) / 1.6f) + 1;
    for (int i = 0; i <= n; i++) {
        float t = (float)i / n;
        cv_px((int)(x0 + (x1 - x0) * t), (int)(y0 + (y1 - y0) * t), sh(R_IRON, (i & 1) ? 0.7f : 0.3f));
    }
}
static void hook(float x, float y, float s) {                       /* a meat hook hanging at (x, y) */
    cv_capsule(x, y, x, y + 7 * s, 1.0f * s, R_IRON, 0.25f, 0.9f);
    for (int a = 0; a <= 14; a++) {
        float ang = PI_F * a / 14.0f, r = 4.0f * s;
        cv_px((int)(x - r + r * cosf(ang) + 0.5f), (int)(y + 7 * s + r * sinf(ang)), sh(R_IRON, 0.75f - 0.4f * a / 14.0f));
        cv_px((int)(x - r + r * cosf(ang) + 0.5f), (int)(y + 7 * s + r * sinf(ang) + 1), sh(R_IRON, 0.3f));
    }
    cv_px((int)(x - 2 * 4.0f * s), (int)(y + 7 * s - 1), sh(R_IRON, 0.9f));   /* the point */
}

static void spr_hooks(void) {
    cv_begin(cv_sprite, TS, TS);
    static const float X[3] = { 20, 33, 45 }, L[3] = { 22, 34, 16 };
    for (int i = 0; i < 3; i++) { chain_down(X[i], 0, (int)L[i]); hook(X[i] + 0.5f, L[i], 1.0f); }
    for (int y = 37; y < 47; y++)                                  /* a rag of something long dead on the middle one */
        for (int x = 25; x < 36; x++) {
            float u = (y - 37) / 10.0f, w = 4.5f - 2.0f * u + sinf(y * 1.3f) * 0.8f;
            if (fabsf(x + 0.5f - 30.5f) > w) continue;
            cv_px(x, y, sh(R_WARM, 0.18f + 0.22f * h01(x, y, 5)));
        }
    cv_store(SP_HOOKS);
}

static void spr_gibbet(int id, int prisoner) {
    cv_begin(cv_sprite, TS, TS);
    chain_down(32, 0, 12);
    float cx = 32, top = 15, bot = 47, rx = 11;
    if (prisoner) {                                                /* slumped against the bars, an arm through them */
        cv_bone(26, 44, 37, 45, 1.3f, 0.25f, 0.85f);
        cv_bone(29, 44, 33, 33, 1.1f, 0.25f, 0.85f);
        for (int r = 0; r < 4; r++) cv_capsule(28.5f, 34.0f + r * 2.2f, 35.5f, 35.0f + r * 2.2f, 0.8f, R_BONE, 0.25f, 0.8f);
        cv_skull(31, 28, 0.9f, 0.3f, 0.95f);
        cv_bone(36, 37, 46, 42, 1.0f, 0.25f, 0.8f);
        cv_bone(46, 42, 47, 50, 0.9f, 0.25f, 0.75f);
    }
    for (int a = 0; a < 40; a++) {                                 /* the hoops, top and bottom, and a band round the middle */
        float ang = TAU_F * a / 40.0f, sx = cx + rx * cosf(ang), sz = sinf(ang);
        if (sz < 0) continue;                                      /* the front half only: the back is behind the bars */
        cv_px((int)sx, (int)(top + 2.5f * sz), sh(R_IRON, 0.35f + 0.5f * sz));
        cv_px((int)sx, (int)(bot + 2.5f * sz), sh(R_IRON, 0.35f + 0.5f * sz));
        cv_px((int)sx, (int)(31 + 2.5f * sz), sh(R_IRON, 0.3f + 0.4f * sz));
    }
    cv_ellipse(cx, top - 1, 4, 3, R_IRON, 0.2f, 0.8f);             /* the dome where the chain takes it */
    for (int b = 0; b < 7; b++) {                                  /* the bars, brighter in front */
        float ang = PI_F * (b + 0.5f) / 7.0f, x = cx - rx * cosf(ang), lit = 0.3f + 0.55f * sinf(ang);
        for (int y = (int)(top + 2.5f * sinf(ang)); y <= (int)(bot + 2.5f * sinf(ang)); y++) cv_px((int)x, y, sh(R_IRON, lit));
    }
    for (int x = (int)(cx - rx); x <= (int)(cx + rx); x++) cv_px(x, (int)bot + 3, sh(R_IRON, 0.25f));
    cv_outline(0);
    cv_store(id);
}

static void spr_chandelier(int id, int frame) {
    cv_begin(cv_sprite, TS, TS);
    float cx = 32, ry = 24, rx = 21;
    chain_down(cx, 0, 7);
    for (int k = 0; k < 3; k++) {                                  /* three chains down to the ring */
        float ang = PI_F * (0.15f + 0.35f * k), x = cx + rx * cosf(ang) * 0.95f;
        chain_line(cx, 8, x, ry + 3.0f * sinf(ang));
    }
    for (int a = 0; a < 72; a++) {                                 /* the iron ring */
        float ang = TAU_F * a / 72.0f, sz = sinf(ang);
        uint8_t c = sh(R_IRON, sz > 0 ? 0.35f + 0.5f * sz : 0.18f);
        cv_px((int)(cx + rx * cosf(ang)), (int)(ry + 3.5f * sz), c);
        if (sz > 0) cv_px((int)(cx + rx * cosf(ang)), (int)(ry + 3.5f * sz + 1), sh(R_IRON, 0.2f));
    }
    for (int k = 0; k < 7; k++) {                                  /* candles, the ones in front drawn last */
        float ang = TAU_F * k / 7.0f + 0.25f, sz = sinf(ang);
        if (sz < -0.2f) continue;
        float x = cx + rx * cosf(ang), y = ry + 3.5f * sz, h = 6 + 3 * h01(k, 1, 77);
        for (int yy = (int)(y - h); yy <= (int)y; yy++) { cv_px((int)x, yy, sh(R_BONE, 0.55f + 0.25f * sz)); cv_px((int)x + 1, yy, sh(R_BONE, 0.35f + 0.25f * sz)); }
        cv_px((int)x, (int)(y - h + 2), sh(R_BONE, 0.95f));        /* a drip */
        flame(x + 1.0f, y - h - 1, 5.5f + (float)((frame + k) & 1), 1.7f, (((frame + k) % 3) - 1) * 0.8f);
    }
    cv_store(id);
}

static void spr_roots(void) {
    cv_begin(cv_sprite, TS, TS);
    for (int r = 0; r < 9; r++) {
        float x = 6 + 52 * h01(r, 1, 31), len = 10 + 30 * h01(r, 2, 31), w = 1.6f + 1.4f * h01(r, 3, 31), ph = h01(r, 4, 31) * 6.0f;
        for (int y = 0; y < (int)len; y++) {
            float u = y / len, xx = x + sinf(y * 0.25f + ph) * 2.0f * u, ww = w * (1.0f - u * 0.85f);
            for (int dx = (int)(-ww - 1); dx <= (int)(ww + 1); dx++) {
                float nx = (dx + 0.5f) / (ww + 0.01f);
                if (fabsf(nx) > 1.0f) continue;
                cv_px((int)(xx + dx), y, sh(R_WOOD, 0.12f + 0.3f * lit(nx, 0, sqrtf(1 - nx * nx)) - 0.08f * u));
            }
            if (h01(r, y, 33) > 0.93f) cv_line(xx, (float)y, xx + (h01(r, y, 34) - 0.5f) * 10, y + 4 + 4 * h01(r, y, 35), sh(R_WOOD, 0.15f));   /* a rootlet */
        }
    }
    for (int x = 0; x < TS; x++) for (int y = 0; y < 2 + (int)(3 * h01(x / 3, 0, 36)); y++) cv_px(x, y, sh(R_WARM, 0.1f + 0.15f * h01(x, y, 37)));   /* soil */
    cv_store(SP_ROOTS);
}

static void spr_websheet(void) {
    cv_begin(cv_sprite, TS, TS);
    uint8_t c = R_STONE + 12, c2 = R_STONE + 9;
    for (int s = 0; s < 9; s++) {                                  /* strands slung from the ceiling, overlapping */
        float x0 = -4 + 60 * h01(s, 1, 41), span = 14 + 22 * h01(s, 2, 41), sag = 5 + 22 * h01(s, 3, 41);
        int n = (int)span;
        for (int i = 0; i <= n; i++) {
            float t = (float)i / n, y = sag * 4.0f * t * (1 - t) + 0.5f;
            cv_px((int)(x0 + span * t), (int)y, (s & 1) ? c : c2);
        }
    }
    for (int s = 0; s < 7; s++) {                                  /* threads hanging down, tattered, a few with dust caught */
        float x = 4 + 56 * h01(s, 4, 41), drop = 14 + 26 * h01(s, 5, 41);
        for (int y = 0; y < (int)drop; y++) if (h01(s, y, 42) > 0.2f) cv_px((int)(x + sinf(y * 0.35f + s) * 0.9f), y, (y & 1) ? c : c2);
        cv_px((int)x, (int)drop, c); cv_px((int)x + 1, (int)drop, c2);
    }
    for (int i = 0; i < 14; i++) {                                 /* a few strands across */
        float x = 6 + 50 * h01(i, 3, 43), y = 4 + 18 * h01(i, 4, 43), a = (h01(i, 6, 43) - 0.5f) * 1.2f, l = 6 + 8 * h01(i, 5, 43);
        cv_line(x, y, x + cosf(a) * l, y + sinf(a) * l, c2);
    }
    cv_px(40, 12, R_IRON + 1); cv_px(41, 12, R_IRON + 1); cv_px(22, 7, R_IRON + 1);   /* flies */
    cv_store(SP_WEBSHEET);
}

/* a grave: a stone on its mound */
static void mound(float cx, float w) {
    cv_ellipse(cx, 61.5f, w, 3.0f, R_WARM, 0.08f, 0.35f);
    for (int i = 0; i < 6; i++) cv_px((int)(cx - w + 2 * w * h01(i, 1, 51)), 59 + (int)(2 * h01(i, 2, 51)), sh(R_MOSS, 0.3f + 0.3f * h01(i, 3, 51)));
}
static void stone_px(int x, int y, float nx, float shade_v, int seed) {
    float v = 0.2f + 0.55f * lit(nx, 0.0f, sqrtf(fmaxf(0.0f, 1.0f - nx * nx))) + shade_v + (fbm(x / 4.0f, y / 4.0f, 16, (uint32_t)seed) - 0.5f) * 0.22f;
    if (h01(x, y, seed + 5) > 0.97f) v -= 0.2f;                    /* pits */
    if (y > 50 && fbm(x / 3.0f, y / 3.0f, 32, (uint32_t)seed + 9) > 0.6f) { cv_px(x, y, sh(R_MOSS, 0.15f + 0.4f * v)); return; }   /* moss creeping up */
    cv_px(x, y, sh(R_STONE, v));
}
static void soil(float cx, float w, int ground, uint32_t seed);
static void spr_tomb(int id, int kind) {
    cv_begin(cv_sprite, TS, TS);
    switch (kind) {
    case 0: {                                                      /* round-topped headstone */
        float cx = 32, hw = 11, top = 26;
        mound(cx, 17);
        for (int y = (int)top; y <= 60; y++)
            for (int x = (int)(cx - hw); x <= (int)(cx + hw); x++) {
                float nx = (x + 0.5f - cx) / hw, dy = y + 0.5f - (top + hw);
                if (dy < 0 && (x + 0.5f - cx) * (x + 0.5f - cx) + dy * dy > hw * hw) continue;
                float e = 0.0f;
                if ((fabsf(x + 0.5f - cx) < 1.0f && y > 32 && y < 46) || (fabsf(y + 0.5f - 37) < 1.0f && fabsf(x + 0.5f - cx) < 5)) e = -0.25f;   /* a cross cut in it */
                if (y > 49 && y < 55 && (x & 3) == 0 && fabsf(x + 0.5f - cx) < 7) e = -0.18f;               /* worn letters */
                stone_px(x, y, nx, e, 61);
            }
        cv_line(cx + 6, top + 4, cx + 2, top + 13, sh(R_STONE, 0.05f));     /* a crack */
        break;
    }
    case 1: {                                                      /* a stone cross */
        float cx = 32;
        mound(cx, 14);
        for (int y = 18; y <= 60; y++)
            for (int x = 20; x <= 44; x++) {
                int upright = fabsf(x + 0.5f - cx) < 4.0f, arm = y >= 27 && y <= 33 && fabsf(x + 0.5f - cx) < 12.0f;
                if (!upright && !arm) continue;
                float nx = upright && !arm ? (x + 0.5f - cx) / 4.0f : (y + 0.5f - 30.0f) / 3.5f * 0.6f;
                stone_px(x, y, nx, (y == 18 || y == 27) ? 0.12f : 0.0f, 67);
            }
        for (int a = 0; a < 40; a++) {                             /* a ring at the crossing, celtic style */
            float ang = TAU_F * a / 40.0f;
            cv_px((int)(cx + 7.5f * cosf(ang)), (int)(30 + 7.5f * sinf(ang)), sh(R_STONE, 0.35f + 0.3f * -sinf(ang)));
        }
        break;
    }
    case 2: {                                                      /* an obelisk */
        float cx = 32;
        mound(cx, 13);
        for (int y = 8; y <= 60; y++) {
            float hw = y < 16 ? (y - 8) * 0.75f : 6.0f + (y - 16) * 0.05f;
            for (int x = (int)(cx - hw - 1); x <= (int)(cx + hw + 1); x++) {
                if (fabsf(x + 0.5f - cx) > hw) continue;
                float nx = (x + 0.5f - cx) / (hw + 0.01f);
                stone_px(x, y, nx > 0 ? 0.6f : -0.6f, (y < 16 ? 0.15f : 0.0f) + (y == 52 ? -0.2f : 0.0f), 71);   /* two faces, one lit */
            }
        }
        cv_skull(cx, 40, 0.55f, 0.3f, 0.9f);                       /* a carved death's head */
        break;
    }
    case 4: {                                                      /* the newest: clean-cut and pale, the earth before it still turned */
        float cx = 32, hw = 11, top = 25;
        for (int y = (int)top; y <= 59; y++)
            for (int x = (int)(cx - hw); x <= (int)(cx + hw); x++) {
                float nx = (x + 0.5f - cx) / hw, dy = y + 0.5f - (top + hw);
                if (dy < 0 && (x + 0.5f - cx) * (x + 0.5f - cx) + dy * dy > hw * hw) continue;
                float v = 0.42f + 0.45f * lit(nx, 0.0f, sqrtf(fmaxf(0.0f, 1.0f - nx * nx))) + (fbm(x / 5.0f, y / 5.0f, 12, 83) - 0.5f) * 0.08f;
                if (dy < 0 && (x + 0.5f - cx) * (x + 0.5f - cx) + dy * dy > (hw - 1.3f) * (hw - 1.3f)) v += 0.1f;   /* the dressed edge */
                if ((fabsf(x + 0.5f - cx) < 1.0f && y > 30 && y < 43) || (fabsf(y + 0.5f - 34) < 1.0f && fabsf(x + 0.5f - cx) < 5)) v -= 0.3f;   /* a cross cut sharp */
                if ((y == 47 || y == 50 || y == 53) && fabsf(x + 0.5f - cx) < 7 - (y == 53) * 3 && (x * 5 + y) % 4) v -= 0.28f;   /* fresh letters */
                cv_px(x, y, sh(R_STONE, v));
            }
        soil(cx, 17, 59, 89);
        break;
    }
    default: {                                                     /* broken: the stump, the top fallen beside it */
        float cx = 28, hw = 10;
        mound(32, 19);
        for (int y = 40; y <= 60; y++)
            for (int x = (int)(cx - hw); x <= (int)(cx + hw); x++) {
                float edge = 40 + (x - (cx - hw)) * 0.45f;               /* a slanting break */
                if (y < edge) continue;
                stone_px(x, y, (x + 0.5f - cx) / hw, y < edge + 1.5f ? 0.15f : 0.0f, 73);
            }
        for (int y = 52; y <= 60; y++)                             /* the fallen top, on its side */
            for (int x = 38; x <= 56; x++) {
                float dx = x + 0.5f - 47.0f, dy = y + 0.5f - 60.0f;
                if (dx * dx / 81.0f + dy * dy / 64.0f > 1.0f) continue;
                stone_px(x, y, dx / 9.0f, -0.05f, 79);
            }
        break;
    }
    }
    cv_outline(0);
    cv_store(id);
}

static void spr_firepit(int id, int frame) {
    cv_begin(cv_sprite, TS, TS);
    float cx = 32;
    for (int i = 0; i < 4; i++) {                                  /* the fire, tongues of it, licking up */
        float x = cx - 9 + i * 6 + (h01(i, frame, 81) - 0.5f) * 3, h = 18 + 14 * h01(i, frame + 5, 81);
        flame(x, 54, h, 3.6f, (h01(i, frame + 9, 81) - 0.5f) * 5.0f);
    }
    flame(cx + (frame - 1) * 1.2f, 55, 30, 6.0f, (frame - 1) * 2.5f);
    for (int i = 0; i < 6; i++) {                                  /* sparks */
        float x = cx + (h01(i, frame, 83) - 0.5f) * 26, y = 12 + 18 * h01(i, frame + 2, 83);
        cv_px((int)x, (int)y, (uint8_t)(R_FLAME + 9 + (i & 3)));
    }
    for (int i = 0; i < 11; i++) {                                 /* the ring of stones, the front ones over the flames */
        float ang = PI_F * (0.0f + i / 10.0f), x = cx - 17 * cosf(ang), y = 55 + 4.5f * sinf(ang);
        cv_ellipse(x, y, 4.2f, 3.3f, R_STONE, 0.15f + 0.1f * h01(i, 1, 85), 0.75f + 0.15f * sinf(ang));
    }
    cv_flat(cx, 62, 19, 1.6f, R_WARM + 1);
    cv_store(id);
}

static void spr_totem(int id, int frame) {
    cv_begin(cv_sprite, TS, TS);
    float cx = 32;
    cv_flat(cx, 62, 9, 1.6f, R_WARM + 1);
    cv_capsule(cx, 20, cx, 62, 2.2f, R_WOOD, 0.15f, 0.7f);         /* the pole */
    cv_capsule(cx - 13, 30, cx + 13, 30, 1.3f, R_WOOD, 0.15f, 0.6f);   /* the crossbar */
    for (int k = 0; k < 3; k++) cv_skull(cx, 52 - k * 10.5f, 0.85f, 0.28f, 0.92f);   /* stacked skulls */
    for (int k = 0; k < 3; k++) {                                  /* their eyes, lit from within */
        float y = 52 - k * 10.5f + 0.8f;
        int c = R_FLAME + 9 + ((frame + k) & 1) * 3;
        cv_px((int)(cx - 2.0f), (int)y, (uint8_t)c); cv_px((int)(cx + 2.0f), (int)y, (uint8_t)c);
    }
    for (int s = -1; s <= 1; s += 2) {                             /* candles on the crossbar ends, and one on top */
        float x = cx + s * 12;
        for (int y = 22; y <= 29; y++) { cv_px((int)x, y, sh(R_BONE, 0.6f)); cv_px((int)x + 1, y, sh(R_BONE, 0.4f)); }
        flame(x + 1, 21, 6.0f + (float)((frame + (s > 0)) & 1), 1.8f, (frame ? 0.7f : -0.7f) * s);
    }
    for (int y = 13; y <= 20; y++) { cv_px((int)cx, y, sh(R_BONE, 0.65f)); cv_px((int)cx + 1, y, sh(R_BONE, 0.45f)); }
    flame(cx + 1, 12, 7.0f + frame, 2.0f, frame ? 0.8f : -0.6f);
    cv_store(id);
}

/* ------------------------------------------------------------------ */
/* The bestiary (docs/PLAN-bestiary.md). A creature's body is drawn in  */
/* its family's ramp (crawl.h, PF_*) so that a rank can recolour it;    */
/* everything else on it keeps its colour.                              */
/* ------------------------------------------------------------------ */

/* move the drawing down by dy and cut it off at the ground, for things climbing out of it */
static void cv_sink(int dy, int ground) {
    for (int y = TS - 1; y >= 0; y--)
        for (int x = 0; x < TS; x++) {
            int sy = y - dy;
            cv_sprite[y * TS + x] = (y >= ground || sy < 0) ? C_CLEAR : cv_sprite[sy * TS + x];
        }
}

/* rot on dead skin: darker patches, green mould, and here and there a raw wound */
static void cv_rot(int ramp, float amount, uint32_t seed) {
    for (int y = 0; y < TS; y++)
        for (int x = 0; x < TS; x++) {
            uint8_t *p = &cv_sprite[y * TS + x];
            if (*p < ramp || *p >= ramp + 16) continue;
            float n = fbm(x / 5.0f, y / 5.0f, 16, seed), m = fbm(x / 3.0f, y / 3.0f, 32, seed + 7);
            if (n < 0.28f + 0.08f * amount) *p = (uint8_t)(*p - 2 < ramp + 1 ? ramp + 1 : *p - 2);
            if (m > 0.74f - 0.08f * amount) *p = sh(R_MOSS, 0.15f + 0.3f * (*p - ramp) / 15.0f);
            if (m < 0.2f && n > 0.55f) *p = sh(R_BLOOD, 0.2f + 0.15f * n);
        }
}

/* a dead hand: palm and four fingers hanging or clawing from (x, y), pointing along angle a */
static void dead_hand(float x, float y, float a, float spread, int ramp, float s) {
    cv_ellipse(x, y, 2.4f * s, 2.2f * s, ramp, 0.35f, 0.9f);
    for (int f = 0; f < 4; f++) {
        float fa = a + (f - 1.5f) * spread, l = (3.6f + (f == 1 || f == 2) * 0.8f) * s;
        float mx = x + cosf(fa) * l * 0.6f, my = y + sinf(fa) * l * 0.6f;
        cv_line(x + cosf(fa) * 1.5f * s, y + sinf(fa) * 1.5f * s, mx, my, sh(ramp, 0.7f));
        cv_line(mx, my, mx + cosf(fa + 0.5f) * l * 0.5f, my + sinf(fa + 0.5f) * l * 0.5f, sh(ramp, 0.5f));
    }
}

/* clods and a mound of turned earth in the grave's ramp-free brown */
static void soil(float cx, float w, int ground, uint32_t seed) {
    cv_ellipse(cx, ground + 2.0f, w, 5.0f, R_WOOD, 0.04f, 0.32f);
    for (int i = 0; i < 9; i++) {
        float x = cx + (h01(i, 1, seed) - 0.5f) * w * 2.0f, y = ground - 1.0f + 4.0f * h01(i, 2, seed);
        cv_ellipse(x, y, 1.6f + 1.6f * h01(i, 3, seed), 1.3f + 1.0f * h01(i, 4, seed), R_WOOD, 0.05f, 0.4f);
    }
    for (int i = 0; i < 5; i++) cv_px((int)(cx + (h01(i, 5, seed) - 0.5f) * w * 1.6f), ground - 3 - (int)(5 * h01(i, 6, seed)), sh(R_WOOD, 0.3f));   /* flung earth */
}

/* The shambler: a corpse in rags out of its grave, one arm reaching, the other hanging.
 * pose: 0 walking, 1 grabbing, 2-3 climbing out (head and hands, then to the waist), 4 sinking to
 * its knees, 5 down. Its skin is R_WARM (PF_FLESH). */
static void spr_shambler(int id, float w, int pose) {
    cv_begin(cv_sprite, TS, TS);
    int grab = pose == 1 || pose == 2 || pose == 3;
    if (pose == 5) {                                               /* down: lying along the floor, an arm flung out */
        cv_flat(32, 61.5f, 26, 2.5f, R_WARM + 1);
        cv_capsule(40, 58, 56, 60, 2.6f, R_WOOD, 0.08f, 0.35f);    /* legs */
        cv_capsule(40, 60, 55, 62, 2.4f, R_WOOD, 0.06f, 0.3f);
        cv_ellipse(30, 57.5f, 11, 4.2f, R_IRON, 0.08f, 0.4f);      /* the shroud-rag over the chest */
        cv_capsule(24, 55, 18, 47, 1.8f, R_WARM, 0.35f, 0.9f);      /* an arm up off the floor */
        dead_hand(17, 46, -1.9f, 0.35f, R_WARM, 1.0f);
        cv_ellipse(14, 58, 5.0f, 4.4f, R_WARM, 0.2f, 0.8f);        /* the head, on its side */
        cv_flat(12, 57, 1.2f, 1.2f, 0); cv_flat(15, 56, 1.2f, 1.2f, 0);
        cv_line(10, 60, 14, 61, sh(R_BLOOD, 0.2f));
        cv_rot(R_WARM, 1.0f, 211);
        cv_outline(0); cv_store(id); return;
    }
    if (pose <= 1) cv_flat(32, 62, 12, 1.6f, R_WARM + 1);
    float kneel = pose == 4 ? 1.0f : 0.0f, sway = w * 0.8f + kneel * 3.0f;
    float hx = 30 + sway + kneel * 4.0f, hy = 13 + (grab ? 1.0f : 0.0f) + kneel * 14.0f;   /* head, tilted over */
    float sy = 23 + kneel * 13.0f;                                  /* shoulders */
    if (!grab && pose != 4) {                                       /* the hanging arm, behind */
        cv_capsule(22 + sway, sy + 1, 20 + w * 1.5f, sy + 10, 1.9f, R_WARM, 0.3f, 0.85f);
        cv_capsule(20 + w * 1.5f, sy + 10, 21 + w * 2.5f, sy + 18, 1.7f, R_WARM, 0.3f, 0.8f);
        dead_hand(21 + w * 2.5f, sy + 19, 1.7f, 0.25f, R_WARM, 0.9f);
    }
    if (pose == 4) {                                                /* on its knees, shins under it */
        cv_capsule(27, 50, 22, 61, 2.6f, R_WOOD, 0.08f, 0.35f);
        cv_capsule(37, 50, 42, 61, 2.6f, R_WOOD, 0.08f, 0.35f);
        cv_capsule(22, 61, 14, 62, 2.0f, R_WARM, 0.3f, 0.75f);
        cv_capsule(42, 61, 50, 62, 2.0f, R_WARM, 0.3f, 0.75f);
    } else {
        for (int s = -1; s <= 1; s += 2) {                          /* legs: rags to the knee, bare shins, a dragging foot */
            float k = s * w, kx = 32 + s * 4.5f - k * 2.0f, fx = 32 + s * 5.0f - k * 3.5f;
            cv_capsule(32 + s * 3.5f, 42, kx, 52, 2.8f, R_WOOD, 0.06f, 0.33f);
            cv_capsule(kx, 51, fx, 60.5f, 2.0f, R_WARM, 0.3f, 0.85f);
            cv_ellipse(fx + s * 1.5f, 61.5f, 3.2f, 1.6f, R_WARM, 0.3f, 0.75f);
        }
    }
    /* the torso, grey skin under a rotten shirt torn open over the ribs */
    float bot = pose == 4 ? 52.0f : 44.0f, cx = 31.0f + sway * 0.7f;
    float tp[8] = { 22 + sway, sy, 40 + sway, sy, 37 + sway * 0.5f, bot, 25 + sway * 0.5f, bot };
    cv_poly(tp, 4, R_WARM, 0.85f, 0.4f, sy, 22);
    cv_ellipse(23 + sway, sy + 2, 3.4f, 3.0f, R_WARM, 0.35f, 0.9f); cv_ellipse(39 + sway, sy + 2, 3.4f, 3.0f, R_WARM, 0.35f, 0.9f);
    for (int y = (int)sy - 1; y <= (int)bot + 3; y++)               /* the shirt: faded, folded, ragged at the hem, torn open */
        for (int x = 18; x <= 45; x++) {
            uint8_t c = cv_sprite[y * TS + x];
            int skin = c >= R_WARM && c < R_WARM + 16;
            if (!skin && !(y > bot && y <= bot + 3 && x > tp[6] && x < tp[4] && h01(x, 9, 215) > 0.45f)) continue;
            if (x > cx - 6 && x < cx + 0.5f - (y - sy) * 0.15f && y > sy + 2 && y < sy + 16) continue;   /* the tear, down to the belly */
            if (y > bot - 1 && h01(x, 7, 215) < 0.3f) continue;     /* ragged hem */
            float fold = 0.5f + 0.5f * sinf((x - cx) * 0.9f + y * 0.15f);
            float v = 0.22f + 0.3f * (1.0f - fabsf(x + 0.5f - cx) / 12.0f) + 0.1f * fold;
            cv_px(x, y, sh(R_IRON, v));
        }
    for (int i = 0; i < 2; i++) {                                   /* ribs showing in the tear, and the rot inside */
        float y = sy + 6 + i * 3.5f;
        for (int k = 0; k <= 4; k++) cv_px((int)(cx - 5 + k), (int)(y + 0.25f * (k - 2) * (k - 2)), sh(R_BONE, 0.85f - 0.08f * k));
    }
    cv_flat(cx - 2.5f, sy + 13, 1.8f, 1.3f, sh(R_BLOOD, 0.2f));
    cv_capsule(hx, hy + 5, 30.5f + sway, sy + 1, 2.2f, R_WARM, 0.3f, 0.75f);   /* the neck */
    /* arms */
    if (grab) {                                                     /* both up and out at you, fingers hooked */
        cv_capsule(22 + sway, sy + 1, 16 + sway, sy - 1, 2.0f, R_WARM, 0.35f, 0.9f);
        cv_capsule(16 + sway, sy - 1, 11 + sway, sy - 5, 1.8f, R_WARM, 0.35f, 0.9f);
        dead_hand(10 + sway, sy - 6, -2.2f, 0.4f, R_WARM, 1.15f);
        cv_capsule(40 + sway, sy + 1, 46 + sway, sy - 1, 2.0f, R_WARM, 0.35f, 0.9f);
        cv_capsule(46 + sway, sy - 1, 51 + sway, sy - 5, 1.8f, R_WARM, 0.35f, 0.9f);
        dead_hand(52 + sway, sy - 6, -0.9f, 0.4f, R_WARM, 1.15f);
    } else if (pose == 4) {                                         /* hanging slack */
        cv_capsule(22 + sway, sy + 1, 22 + sway, sy + 14, 1.9f, R_WARM, 0.3f, 0.8f);
        dead_hand(22 + sway, sy + 15, 1.6f, 0.2f, R_WARM, 0.9f);
        cv_capsule(40 + sway, sy + 1, 42 + sway, sy + 14, 1.9f, R_WARM, 0.3f, 0.8f);
        dead_hand(42 + sway, sy + 15, 1.5f, 0.2f, R_WARM, 0.9f);
    } else {                                                        /* the reaching arm, out towards you */
        cv_capsule(40 + sway, sy + 1, 45 + sway, sy + 3, 2.0f, R_WARM, 0.35f, 0.9f);
        cv_capsule(45 + sway, sy + 3, 49 + sway - w * 0.5f, sy + 0, 1.9f, R_WARM, 0.4f, 0.95f);
        dead_hand(50 + sway - w * 0.5f, sy - 1, 0.9f, 0.3f, R_WARM, 1.1f);
    }
    /* the head: sunken eyes with a dull light in them, a slack jaw, a few strands of hair */
    cv_ellipse(hx, hy, 6.0f, 6.6f, R_WARM, 0.35f, 1.0f);
    cv_ellipse(hx + 0.3f, hy + 4.3f, 4.0f, 2.8f, R_WARM, 0.25f, 0.75f);
    for (int k = -1; k <= 1; k += 2) {
        cv_flat(hx + k * 2.3f, hy - 0.5f, 1.6f, 1.5f, 0);
        cv_px((int)(hx + k * 2.3f), (int)(hy - 0.5f), (uint8_t)(R_FLAME + (grab ? 7 : 4)));
    }
    float mo = grab ? 2.2f : 1.2f;                                  /* the mouth, wider when it lunges */
    cv_flat(hx + 0.4f, hy + 3.6f, 2.0f, mo, 0);
    for (int i = -1; i <= 1; i++) cv_px((int)(hx + 0.4f + i * 1.2f), (int)(hy + 3.6f - mo + 1), sh(R_BONE, 0.7f));
    for (int i = 0; i < 7; i++) {                                   /* lank hair over the crown and down the sides */
        float a = PI_F * (1.05f + 0.9f * i / 6.0f), x = hx + cosf(a) * 5.6f, y = hy + sinf(a) * 6.2f;
        float drop = (i == 0 || i == 6) ? 6.0f + 3.0f * h01(i, 2, 219) : 1.5f;
        cv_line(x, y, x + (x < hx ? -0.8f : 0.8f), y + drop, sh(R_IRON, 0.18f + 0.12f * h01(i, 1, 219)));
    }
    cv_rot(R_WARM, 0.6f, 211);
    if (pose == 2 || pose == 3) {                                   /* climbing out of the grave */
        cv_sink(pose == 2 ? 30 : 14, 58);
        soil(32, 17, 58, 221);
    }
    cv_outline(0);
    cv_store(id);
}

/* The Drowned: bloated, blue-grey, hung with weed and dripping. Its body is R_STONE (PF_STONE).
 * pose: 0-1 wading, 2 grabbing, 3 rising from its pool, 4 bursting, 5 what it leaves: a foul pool. */
static void drowned_pool(float cx, float w, int foul, uint32_t seed) {
    int ramp = foul ? R_MOSS : R_GHOST;
    cv_flat(cx, 59.5f, w, 4.2f, sh(ramp, 0.1f));
    for (int y = 55; y <= 63; y++)
        for (int x = (int)(cx - w); x <= (int)(cx + w); x++) {
            float nx = (x + 0.5f - cx) / w, ny = (y + 0.5f - 59.5f) / 4.2f, d = sqrtf(nx * nx + ny * ny);
            if (d > 1.0f) continue;
            float r = sinf(d * 11.0f + h01((int)seed, 0, 9) * 6.0f);
            if (r > 0.75f) cv_px(x, y, sh(ramp, 0.3f + 0.3f * r * (1.0f - d)));
            else if (ny < -0.3f && h01(x, y, seed) > 0.85f) cv_px(x, y, sh(ramp, 0.5f));   /* the light on the far side */
        }
    for (int i = 0; i < 5; i++) {                                   /* bubbles */
        float x = cx + (h01(i, 1, seed) - 0.5f) * w * 1.4f, y = 58 + 3 * h01(i, 2, seed);
        cv_px((int)x, (int)y, sh(ramp, 0.75f)); cv_px((int)x + 1, (int)y - 1, sh(ramp, 0.5f));
    }
}

static void spr_drowned(int id, float w, int pose) {
    cv_begin(cv_sprite, TS, TS);
    if (pose == 5) { drowned_pool(32, 20, 1, 231); cv_store(id); return; }
    int grab = pose == 2 || pose == 3;
    float sway = w * 1.0f;
    if (pose == 4) {                                                /* burst: rags of skin flung out round a cloud of gas */
        for (int i = 0; i < 160; i++) {
            float a = h01(i, 1, 233) * TAU_F, r = 4 + 20 * sqrtf(h01(i, 2, 233));
            float x = 32 + cosf(a) * r, y = 38 + sinf(a) * r * 0.8f;
            if ((i & 1) && h01(i, 3, 233) > 0.3f) cv_px((int)x, (int)y, sh(R_MOSS, 0.3f + 0.4f * h01(i, 4, 233)));
        }
        for (int i = 0; i < 9; i++) {
            float a = h01(i, 5, 233) * TAU_F, r = 8 + 14 * h01(i, 6, 233);
            cv_ellipse(32 + cosf(a) * r, 40 + sinf(a) * r * 0.7f, 2.0f + 2.0f * h01(i, 7, 233), 1.5f + 1.5f * h01(i, 8, 233), R_STONE, 0.2f, 0.7f);
        }
        cv_ellipse(32, 44, 8, 7, R_STONE, 0.1f, 0.45f);            /* what is left, slumping */
        cv_flat(32, 44, 4, 3, sh(R_BLOOD, 0.15f));
        drowned_pool(32, 16, 1, 235);
        cv_outline(0); cv_store(id); return;
    }
    if (pose != 3) cv_flat(32, 62, 14, 1.6f, R_WARM + 1);
    for (int s = -1; s <= 1; s += 2)                                /* short thick legs */
        cv_capsule(32 + s * 6.0f, 48, 32 + s * 7.0f - s * w * 1.5f, 60, 4.2f, R_STONE, 0.15f, 0.6f);
    cv_ellipse(32 + sway, 37, 14.5f, 15.0f, R_STONE, 0.2f, 0.9f);  /* the belly, swollen tight */
    cv_flat(32 + sway, 40, 1.4f, 1.8f, sh(R_STONE, 0.2f));        /* the navel, pushed out */
    cv_rot(R_STONE, 0.3f, 236);
    for (int i = 0; i < 10; i++)                                    /* barnacles */
        cv_px((int)(20 + 24 * h01(i, 3, 237) + sway), (int)(26 + 22 * h01(i, 4, 237)), sh(R_BONE, 0.55f + 0.3f * h01(i, 5, 237)));
    float sy = 25;
    if (grab) {                                                     /* arms up, heavy, swollen fingers */
        for (int s = -1; s <= 1; s += 2) {
            cv_capsule(32 + s * 13 + sway, sy, 32 + s * 19 + sway, sy - 6, 3.4f, R_STONE, 0.25f, 0.8f);
            cv_capsule(32 + s * 19 + sway, sy - 6, 32 + s * 22 + sway, sy - 13, 3.0f, R_STONE, 0.25f, 0.8f);
            dead_hand(32 + s * 22 + sway, sy - 15, s < 0 ? -2.2f : -0.9f, 0.45f, R_STONE, 1.3f);
        }
    } else {                                                        /* arms hanging heavy at its sides */
        for (int s = -1; s <= 1; s += 2) {
            cv_capsule(32 + s * 13 + sway, sy + 1, 32 + s * 17 + sway * 0.5f, sy + 14, 3.4f, R_STONE, 0.2f, 0.7f);
            cv_capsule(32 + s * 17 + sway * 0.5f, sy + 14, 32 + s * 16 - s * w, sy + 24, 3.0f, R_STONE, 0.2f, 0.65f);
            dead_hand(32 + s * 16 - s * w, sy + 26, 1.6f, 0.3f, R_STONE, 1.2f);
        }
    }
    float hx = 32 + sway, hy = 16;                                  /* the head, swollen, sunk into the shoulders */
    cv_ellipse(hx, hy + 5, 9.0f, 5.0f, R_STONE, 0.2f, 0.8f);       /* jowls and neck all one */
    cv_ellipse(hx, hy, 8.2f, 8.0f, R_STONE, 0.3f, 1.0f);
    for (int k = -1; k <= 1; k += 2) {                              /* milky eyes in puffed lids */
        cv_ellipse(hx + k * 3.2f, hy - 0.5f, 2.6f, 2.0f, R_STONE, 0.2f, 0.7f);
        cv_flat(hx + k * 3.2f, hy, 1.8f, 1.3f, sh(R_GHOST, 0.9f));
        cv_px((int)(hx + k * 3.2f), (int)hy, sh(R_GHOST, 0.6f));
    }
    cv_flat(hx, hy + 5.0f, grab ? 3.0f : 2.0f, grab ? 2.4f : 1.3f, 0);   /* the mouth, and water out of it */
    for (int y = (int)(hy + 5); y < (int)(hy + 14); y++) if (h01(y, 1, 239) > 0.3f) cv_px((int)hx + (y & 1), y, sh(R_GHOST, 0.5f + 0.3f * h01(y, 2, 239)));
    for (int i = 0; i < 8; i++) {                                   /* weed hung over the crown and down over the shoulders */
        float a = PI_F * (1.0f + i / 7.0f), x = hx + cosf(a) * 7.5f, y0 = hy + sinf(a) * 7.5f;
        float len = (i == 0 || i == 7 || i == 1 || i == 6) ? 9 + 7 * h01(i, 1, 241) : 3 + 3 * h01(i, 1, 241), ph = h01(i, 2, 241) * 6;
        for (int y = 0; y < (int)len; y++) {
            float xx = x + (x < hx ? -1.0f : 1.0f) * y * 0.25f + sinf(y * 0.5f + ph + w) * 1.0f;
            cv_px((int)xx, (int)(y0 + y), sh(R_MOSS, 0.1f + 0.25f * (1.0f - y / len)));
            cv_px((int)xx + 1, (int)(y0 + y), sh(R_MOSS, 0.06f + 0.15f * (1.0f - y / len)));
        }
    }
    for (int i = 0; i < 7; i++) {                                   /* drips */
        float x = 20 + 24 * h01(i, 3, 243) + sway, y = 40 + 18 * h01(i, (int)(w * 3 + 4), 243);
        cv_px((int)x, (int)y, sh(R_GHOST, 0.8f)); cv_px((int)x, (int)y + 1, sh(R_GHOST, 0.5f));
    }
    if (pose == 3) { cv_sink(16, 58); drowned_pool(32, 22, 0, 245); }
    cv_outline(0);
    cv_store(id);
}

/* The ghoul: low, on all fours, grey and starved, a ridge of spine over its back. Skin R_WARM.
 * pose: 0-1 loping, 2 leaping, 3 feeding on bones, 4 dead. */
static void spr_ghoul(int id, float w, int pose) {
    cv_begin(cv_sprite, TS, TS);
    if (pose == 4) {
        cv_flat(32, 61.5f, 24, 2.5f, R_WARM + 1);
        cv_ellipse(34, 58, 13, 4.0f, R_WARM, 0.12f, 0.55f);
        for (int i = 0; i < 5; i++) cv_px(26 + i * 4, 54, sh(R_BONE, 0.6f));
        cv_capsule(22, 58, 10, 61, 1.6f, R_WARM, 0.15f, 0.5f); cv_capsule(46, 58, 56, 61, 1.6f, R_WARM, 0.15f, 0.5f);
        cv_ellipse(18, 57, 4.4f, 3.6f, R_WARM, 0.28f, 0.8f);
        cv_flat(16, 56, 1.0f, 1.0f, 0);
        cv_rot(R_WARM, 0.4f, 251);
        cv_outline(0); cv_store(id); return;
    }
    float lift = pose == 2 ? -10.0f : 0.0f, feed = pose == 3 ? 1.0f : 0.0f;
    float hx = 32, hy = 40 + lift + feed * 10.0f;                   /* the head, thrust forward and low */
    if (pose != 2) cv_flat(32, 62, 18, 1.6f, R_WARM + 1);
    if (pose == 3)                                                  /* what it is feeding on */
        for (int i = 0; i < 5; i++) {
            float cx = 32 + (h01(i, 1, 255) - 0.5f) * 26, a = (h01(i, 2, 255) - 0.5f) * 1.2f, l = 5 + 3 * h01(i, 3, 255);
            cv_bone(cx - cosf(a) * l, 60 - sinf(a) * l, cx + cosf(a) * l, 60 + sinf(a) * l, 1.2f, 0.25f, 0.9f);
        }
    /* hind legs, behind, folded */
    for (int s = -1; s <= 1; s += 2) {
        float kx = 32 + s * 13, ky = 44 + lift * 0.6f;
        cv_capsule(32 + s * 6, 34 + lift, kx, ky, 2.6f, R_WARM, 0.2f, 0.6f);
        cv_capsule(kx, ky, 32 + s * 9 + (pose == 2 ? s * 6 : 0), 58 + lift * 0.3f, 2.0f, R_WARM, 0.2f, 0.55f);
    }
    /* the back, arched high, with the spine standing out */
    cv_ellipse(32, 33 + lift + feed * 4, 12, 8, R_WARM, 0.3f, 0.9f);
    for (int i = 0; i < 4; i++) cv_ellipse(32, 26.5f + lift + feed * 4 + i * 2.6f, 1.8f - i * 0.2f, 1.2f, R_WARM, 0.5f, 1.0f);   /* knuckles of spine */
    for (int s2 = -1; s2 <= 1; s2 += 2) cv_ellipse(32 + s2 * 6.5f, 29 + lift + feed * 4, 3.0f, 2.2f, R_WARM, 0.35f, 0.95f);   /* shoulder blades */
    for (int i = 0; i < 4; i++) {                                   /* ribs down the flanks */
        float y = 31 + lift + feed * 4 + i * 2.5f;
        cv_line(22, y, 26, y + 1.5f, sh(R_WARM, 0.2f)); cv_line(42, y, 38, y + 1.5f, sh(R_WARM, 0.2f));
    }
    /* front arms: long, bony, planted wide, or flung out in the leap */
    for (int s = -1; s <= 1; s += 2) {
        float ex = 32 + s * 15, ey = 38 + lift, fx, fy;
        if (pose == 2) { ex = 32 + s * 17; ey = 26 + lift; fx = 32 + s * 24; fy = 18 + lift; }
        else { float step = s * w * 3.0f; fx = 32 + s * 17 + step * 0.3f; fy = 60 - fmaxf(0.0f, step) * 1.5f; }
        cv_capsule(32 + s * 9, 32 + lift + feed * 4, ex, ey, 2.4f, R_WARM, 0.3f, 0.85f);
        cv_capsule(ex, ey, fx, fy, 2.0f, R_WARM, 0.28f, 0.8f);
        for (int c = -1; c <= 1; c++) {                             /* claws */
            float ca = (pose == 2 ? (s < 0 ? -2.3f : -0.8f) : 1.57f + s * 0.3f) + c * 0.35f;
            cv_line(fx, fy, fx + cosf(ca) * 4.0f, fy + sinf(ca) * 4.0f, sh(R_BONE, 0.65f));
        }
    }
    /* the head: long skull-like face, bat ears, a mouth of needles */
    for (int s = -1; s <= 1; s += 2) {                              /* the ears, standing out past the shoulders */
        float e[6] = { hx + s * 4.0f, hy - 4, hx + s * 16, hy - 14, hx + s * 6.5f, hy + 0.5f };
        cv_poly(e, 3, R_WARM, 0.45f, 0.95f, hy - 14, 14);
        float in[6] = { hx + s * 5.5f, hy - 3, hx + s * 13.5f, hy - 11, hx + s * 6.5f, hy - 0.5f };
        cv_poly(in, 3, R_BLOOD, 0.1f, 0.3f, hy - 11, 10);
    }
    cv_ellipse(hx, hy, 7.0f, 6.2f, R_WARM, 0.35f, 1.0f);
    cv_ellipse(hx, hy + 4.5f, 4.8f, 3.2f, R_WARM, 0.25f, 0.75f);
    for (int s = -1; s <= 1; s += 2) {
        cv_flat(hx + s * 2.8f, hy - 0.8f, 1.8f, 1.4f, 0);
        cv_px((int)(hx + s * 2.8f), (int)(hy - 0.8f), (uint8_t)(R_FLAME + 9));
    }
    float mo = pose == 2 ? 2.8f : (pose == 3 ? 1.0f : 1.6f);
    cv_flat(hx, hy + 5.0f, 3.4f, mo, 0);
    for (int i = -3; i <= 3; i++) { if (i & 1) continue; cv_px((int)hx + i, (int)(hy + 5.0f - mo + 0.5f), sh(R_BONE, 0.9f)); cv_px((int)hx + i + 1, (int)(hy + 5.0f + mo - 0.5f), sh(R_BONE, 0.75f)); }
    if (pose == 3) cv_line(hx - 1, hy + 6, hx + 2, hy + 12, sh(R_BLOOD, 0.3f));
    cv_rot(R_WARM, 0.3f, 253);
    cv_outline(0);
    cv_store(id);
}

/* grime and moss on bones, deeper down, as on the skeletons */
static void cv_bone_decay(uint32_t seed) {
    if (g_decay <= 0.0f) return;
    for (int y = 0; y < TS; y++)
        for (int x = 0; x < TS; x++) {
            uint8_t c = cv_sprite[y * TS + x];
            if (c < R_BONE || c >= R_BONE + 16) continue;
            float n = fbm(x / 5.0f, y / 5.0f, 16, seed);
            if (n < 0.28f + 0.32f * g_decay) cv_sprite[y * TS + x] = (uint8_t)(c - 3 < R_BONE + 1 ? R_BONE + 1 : c - 3);
            if (g_decay > 0.35f && n > 0.78f - 0.12f * g_decay) cv_sprite[y * TS + x] = sh(R_MOSS, 0.3f + 0.3f * n);
        }
}

/* The bone hound: a dog's skeleton in a spiked collar, coming at you a little from the side.
 * pose: 0-1 running, 2 biting, 3 a heap. All bone (PF_BONE). */
static void dog_skull(float x, float y, float jaw, int eye) {
    cv_ellipse(x + 3, y - 1, 6.0f, 5.0f, R_BONE, 0.3f, 1.0f);      /* the crown, behind */
    cv_capsule(x + 1, y + 1, x - 7, y + 6, 2.8f, R_BONE, 0.3f, 0.95f);   /* the long snout, towards you */
    cv_capsule(x + 2, y + 4, x - 6, y + 8 + jaw, 1.6f, R_BONE, 0.25f, 0.8f);   /* the jaw */
    for (int i = 0; i < 4; i++) {                                   /* teeth, top and bottom */
        cv_px((int)(x - 6 + i * 2.0f), (int)(y + 8.0f - (3 - i) * 0.6f), sh(R_BONE, 1.0f));
        cv_px((int)(x - 5 + i * 2.0f), (int)(y + 7.5f + jaw - (3 - i) * 0.6f), sh(R_BONE, 0.9f));
    }
    if (jaw > 1.0f) cv_flat(x - 2, y + 7 + jaw * 0.5f, 3.0f, jaw * 0.5f, 0);   /* the gape */
    cv_flat(x - 7, y + 5, 1.0f, 0.8f, 0);                          /* nostril */
    cv_flat(x + 2.5f, y - 0.5f, 2.0f, 1.8f, 0);                    /* the eye socket */
    if (eye) cv_px((int)(x + 2.5f), (int)(y - 0.5f), (uint8_t)(R_FLAME + 11));
}

static void spr_hound(int id, float w, int pose) {
    cv_begin(cv_sprite, TS, TS);
    if (pose == 3) {
        cv_flat(32, 61.5f, 24, 2.5f, R_WARM + 1);
        for (int i = 0; i < 8; i++) {
            float cx = 32 + (h01(i, 1, 301) - 0.5f) * 36, cy = 59 - 4 * h01(i, 2, 301), a = (h01(i, 3, 301) - 0.5f) * 2.0f, l = 4 + 5 * h01(i, 4, 301);
            cv_bone(cx - cosf(a) * l, cy - sinf(a) * l, cx + cosf(a) * l, cy + sinf(a) * l, 1.1f, 0.25f, 0.9f);
        }
        for (int i = 0; i < 5; i++) cv_line(30 + i * 3, 52, 28 + i * 3, 58, sh(R_BONE, 0.7f));   /* the ribs, fallen in */
        dog_skull(20, 52, 2.0f, 0);
        cv_bone_decay(303);
        cv_outline(0); cv_store(id); return;
    }
    float bite = pose == 2 ? 1.0f : 0.0f, bob = (pose == 0 ? 1.0f : 0.0f) * w;
    cv_flat(32, 62, 20, 2.0f, R_WARM + 1);
    /* the far legs, thinner and darker */
    cv_capsule(44, 32 + bob, 48 - w * 2, 44, 1.2f, R_BONE, 0.15f, 0.5f); cv_capsule(48 - w * 2, 44, 45 - w * 3, 58, 1.0f, R_BONE, 0.15f, 0.5f);
    cv_capsule(30, 37 + bob, 33 + w * 2, 48, 1.2f, R_BONE, 0.15f, 0.5f); cv_capsule(33 + w * 2, 48, 32 + w * 3, 60, 1.0f, R_BONE, 0.15f, 0.5f);
    /* the tail, up and wagging for no one */
    cv_capsule(46, 29 + bob, 52, 23 - w, 1.1f, R_BONE, 0.3f, 0.8f); cv_capsule(52, 23 - w, 57, 21 + w, 0.9f, R_BONE, 0.3f, 0.8f);
    /* the spine and the cage of ribs hanging from it */
    for (int i = 0; i < 9; i++) cv_ellipse(24 + i * 2.5f, 30 + bob - sinf(i / 8.0f * PI_F) * 2.0f, 1.5f, 1.3f, R_BONE, 0.35f, 0.95f);
    for (int i = 0; i < 5; i++) {
        float x = 27 + i * 3.0f, top = 31 + bob - sinf((x - 24) / 20.0f * PI_F) * 2.0f, drop = 10 - fabsf(i - 1.5f) * 1.5f;
        cv_capsule(x, top, x - 2.5f, top + drop * 0.6f, 0.8f, R_BONE, 0.3f, 0.85f);
        cv_capsule(x - 2.5f, top + drop * 0.6f, x - 1.0f, top + drop, 0.7f, R_BONE, 0.25f, 0.75f);
    }
    cv_ellipse(43, 33 + bob, 4.0f, 3.0f, R_BONE, 0.3f, 0.85f);      /* the pelvis */
    /* the near legs */
    cv_bone(42, 34 + bob, 39 + w * 2, 46, 1.2f, 0.3f, 0.9f); cv_capsule(39 + w * 2, 46, 42 + w, 52, 1.1f, R_BONE, 0.3f, 0.85f);
    cv_capsule(42 + w, 52, 40 + w * 3, 61, 1.0f, R_BONE, 0.3f, 0.85f); cv_ellipse(39 + w * 3, 61.5f, 2.2f, 1.2f, R_BONE, 0.3f, 0.8f);
    cv_bone(27, 37 + bob, 24 - w * 2, 48, 1.3f, 0.3f, 0.95f); cv_capsule(24 - w * 2, 48, 25 - w * 3, 60, 1.1f, R_BONE, 0.3f, 0.9f);
    cv_ellipse(24 - w * 3, 61.5f, 2.4f, 1.2f, R_BONE, 0.3f, 0.85f);
    /* the collar, spiked, and the head */
    float hx = 18, hy = 26 + bob + bite * 6.0f;
    for (int a = 0; a < 20; a++) {
        float ang = TAU_F * a / 20.0f, sz = sinf(ang);
        if (sz < -0.3f) continue;
        int x = (int)(23 + 4.5f * cosf(ang)), y = (int)(hy + 5 + 2.0f * sz);
        cv_px(x, y, sh(R_IRON, 0.3f + 0.4f * sz)); cv_px(x, y + 1, sh(R_IRON, 0.2f));
        if (a % 4 == 1 && sz > 0.2f) cv_px(x, y + 2, sh(R_IRON, 0.85f));
    }
    dog_skull(hx, hy, bite ? 6.0f : 1.0f + (w > 0 ? 0.6f : 0.0f), 1);
    cv_bone_decay(305);
    cv_outline(0);
    cv_store(id);
}

/* The skeleton knight: plate over bone, a two-handed sword with a steel blade (R_GHOST, so the
 * plate's rank leaves it alone). pose 0-1 walking, 2 the long wind-up, 3 the blow. state: 0 full
 * plate, 1 helm and half the plate hacked off, 2 down to a cracked breastplate. */
static void plate(float x0, float y0, float x1, float y1, float r, float dent) {
    cv_capsule(x0, y0, x1, y1, r, R_IRON, 0.25f - dent, 1.0f - dent);
}
static void sword(float gx, float gy, float a) {
    float dx = cosf(a), dy = sinf(a), px = -dy, py = dx;
    cv_ellipse(gx - dx * 6.0f, gy - dy * 6.0f, 1.8f, 1.8f, R_IRON, 0.4f, 1.0f);   /* pommel */
    cv_capsule(gx - dx * 5.0f, gy - dy * 5.0f, gx + dx * 3.0f, gy + dy * 3.0f, 1.2f, R_WOOD, 0.2f, 0.6f);   /* the grip */
    cv_capsule(gx + dx * 3.5f - px * 6.0f, gy + dy * 3.5f - py * 6.0f, gx + dx * 3.5f + px * 6.0f, gy + dy * 3.5f + py * 6.0f, 1.1f, R_IRON, 0.35f, 0.95f);
    for (int i = 4; i < 40; i++) {                                  /* the blade, a bright edge and a fuller down it */
        float t = (float)i, wdt = i > 34 ? (40 - i) * 0.35f : 1.9f;
        for (float o = -wdt; o <= wdt; o += 0.5f) {
            float v = 0.55f + 0.4f * (o / (wdt + 0.01f)) * -0.5f + (fabsf(o) < 0.4f ? -0.15f : 0.0f) + (o > wdt - 0.6f ? 0.35f : 0.0f);
            cv_px((int)(gx + dx * t + px * o), (int)(gy + dy * t + py * o), sh(R_GHOST, v));
        }
    }
}
static void spr_knight(int id, float w, int pose, int state) {
    cv_begin(cv_sprite, TS, TS);
    float gx, gy, ga;                                               /* where the hands grip, and where the blade points */
    if (pose == 2) { gx = 41; gy = 11; ga = -2.9f; }
    else if (pose == 3) { gx = 27; gy = 40; ga = 2.55f; }
    else if (pose == 4) { gx = 32; gy = 25; ga = 1.5708f; }        /* planted: both hands on the pommel, the point in the floor */
    else { gx = 37; gy = 34; ga = -1.05f; }
    cv_flat(32, 62, 16, 2.0f, R_WARM + 1);
    if (pose == 2) sword(gx, gy, ga);                               /* raised behind the head */
    /* legs: bone, with plate on the thighs and shins unless it has been hacked away */
    for (int s = -1; s <= 1; s += 2) {
        float k = s * w, hx = 32 + s * 4.0f, kx = 32 + s * 5.0f - k * 2.5f, fx = 32 + s * 5.5f - k * 4.0f;
        if (pose == 3 || pose == 4) { kx += s * 1.5f; fx += s * 3.0f; }   /* braced for the blow, or the shout */
        cv_bone(hx, 41, kx, 51, 1.6f, 0.25f, 0.95f);
        cv_bone(kx, 51, fx, 60, 1.4f, 0.25f, 0.9f);
        if (state < 1 || (state < 2 && s > 0)) plate(hx, 42, kx, 50, 3.0f, 0.0f);
        if (state < 2) { plate(kx, 52, fx, 59, 2.6f, 0.0f); cv_ellipse(kx, 51, 2.6f, 2.2f, R_IRON, 0.4f, 1.0f); }
        if (state < 2) cv_ellipse(fx - s * 1.0f, 61.5f, 3.6f, 1.6f, R_IRON, 0.3f, 0.9f);
        else cv_capsule(fx, 61.5f, fx - s * 3.5f, 62, 1.0f, R_BONE, 0.3f, 0.8f);
    }
    /* the spine and ribs under the plate, which show where it is gone */
    cv_capsule(32, 18, 32, 40, 1.4f, R_BONE, 0.3f, 0.9f);
    for (int i = 0; i < 4; i++) {
        float y = 21 + i * 3.6f;
        cv_capsule(31, y, 25 + i * 0.6f, y + 2, 0.9f, R_BONE, 0.25f, 0.9f); cv_capsule(33, y, 39 - i * 0.6f, y + 2, 0.9f, R_BONE, 0.25f, 0.9f);
    }
    cv_ellipse(32, 40, 7, 3.4f, R_BONE, 0.25f, 0.9f);
    /* the fauld: bands of plate over the hips */
    if (state < 2)
        for (int b = 0; b < 3; b++) {
            float y = 37 + b * 2.6f, hw = 9.5f + b * 0.8f;
            for (int x = (int)(32 - hw); x <= (int)(32 + hw); x++) {
                if (state == 1 && x < 28 && b > 0) continue;
                float nx = (x + 0.5f - 32) / hw;
                for (int yy = 0; yy < 3; yy++) cv_px(x, (int)y + yy, sh(R_IRON, 0.3f + 0.5f * lit(nx, -0.2f, sqrtf(fmaxf(0, 1 - nx * nx))) - yy * 0.08f));
            }
        }
    /* the breastplate, with a ridge down it; hacked and split as the knight is worn down */
    for (int y = 18; y <= 37; y++) {
        float hw = 9.0f + 1.5f * sinf((y - 18) / 19.0f * PI_F) - (y > 31 ? (y - 31) * 0.6f : 0.0f);
        for (int x = (int)(32 - hw); x <= (int)(32 + hw); x++) {
            float nx = (x + 0.5f - 32) / hw;
            if (state == 2 && (x < 30 - (y - 18) * 0.15f || y > 32)) continue;   /* the left half and the belly gone */
            float v = 0.25f + 0.6f * lit(nx, (y - 26) / 14.0f, sqrtf(fmaxf(0.0f, 1.0f - nx * nx)));
            if (fabsf(x + 0.5f - 32) < 0.8f) v += 0.25f;
            if (state >= 1 && h01(x / 2, y / 2, 311) > 0.86f) v -= 0.3f;    /* dents */
            cv_px(x, y, sh(R_IRON, v));
        }
    }
    if (state == 2) cv_line(30, 19, 33, 33, sh(R_IRON, 0.05f));
    /* arms: both hands on the grip */
    for (int s = -1; s <= 1; s += 2) {
        float sx = 32 + s * 9.0f, sy = 20;
        float ex = (sx + gx) * 0.5f + s * 4.0f, ey = (sy + gy) * 0.5f + 4.0f;
        if (pose == 2) { ex = (sx + gx) * 0.5f + s * 3.0f; ey = (sy + gy) * 0.5f + 3.0f; }
        cv_bone(sx, sy, ex, ey, 1.3f, 0.25f, 0.9f);
        cv_bone(ex, ey, gx + s * 1.0f, gy, 1.1f, 0.25f, 0.9f);
        int keep = state == 0 || (state == 1 && s > 0) || (state == 2 && s > 0);
        if (keep) cv_ellipse(sx, sy - 0.5f, 5.0f, 4.0f, R_IRON, 0.3f, 1.0f);   /* pauldron */
        if (state < 2) plate(ex, ey, gx + s * 1.0f, gy, 2.0f, 0.05f);          /* vambrace */
    }
    if (pose != 2) sword(gx, gy, ga);
    for (int s = -1; s <= 1; s += 2) cv_ellipse(gx + s * 1.2f, gy + 0.5f, 2.2f, 2.0f, state < 2 ? R_IRON : R_BONE, 0.3f, 0.9f);   /* gauntlets, or bare bone */
    /* the head: a great helm with a slit, and two embers in it; or the bare skull once the helm is off */
    cv_capsule(32, 15, 32, 18, 1.2f, R_BONE, 0.3f, 0.9f);
    if (state == 0) {
        for (int y = 2; y <= 16; y++)
            for (int x = 25; x <= 39; x++) {
                float nx = (x + 0.5f - 32) / 7.0f, top = 2 + 3.0f * nx * nx;
                if (fabsf(nx) > 1.0f || y < top) continue;
                float v = 0.25f + 0.6f * lit(nx, (y - 9) / 9.0f, sqrtf(fmaxf(0, 1 - nx * nx)));
                if (fabsf(x + 0.5f - 32) < 0.8f && y < 9) v += 0.25f;
                if (y == 9 || (y == 10 && fabsf(nx) < 0.85f)) v = 0.0f;   /* the eye slit */
                if (y > 11 && (x & 1) && fabsf(nx) < 0.5f && y < 15) v -= 0.3f;   /* breaths */
                cv_px(x, y, sh(R_IRON, v));
            }
        cv_px(29, 9, (uint8_t)(R_FLAME + 11)); cv_px(34, 9, (uint8_t)(R_FLAME + 11));
    } else {
        cv_skull(32, 9, 1.0f, 0.3f, 1.0f);
        cv_px(30, 10, (uint8_t)(R_FLAME + 10)); cv_px(34, 10, (uint8_t)(R_FLAME + 10));
        if (state == 1) for (int x = 26; x <= 38; x++) cv_px(x, 3 + (x - 32) * (x - 32) / 12, sh(R_IRON, 0.5f));   /* the helm's rim, all that is left */
    }
    cv_bone_decay(313);
    cv_outline(0);
    cv_store(id);
}

static void spr_knight_dead(int id) {
    cv_begin(cv_sprite, TS, TS);
    cv_flat(32, 61.5f, 26, 2.5f, R_WARM + 1);
    for (int i = 0; i < 6; i++) {
        float cx = 32 + (h01(i, 1, 321) - 0.5f) * 34, cy = 59 - 3 * h01(i, 2, 321), a = (h01(i, 3, 321) - 0.5f) * 1.6f, l = 5 + 4 * h01(i, 4, 321);
        cv_bone(cx - cosf(a) * l, cy - sinf(a) * l, cx + cosf(a) * l, cy + sinf(a) * l, 1.3f, 0.25f, 0.9f);
    }
    cv_ellipse(30, 56, 9, 5, R_IRON, 0.2f, 0.9f);                  /* the breastplate, on its back */
    cv_line(30, 52, 30, 60, sh(R_IRON, 0.9f));
    cv_ellipse(47, 57, 5, 4, R_IRON, 0.2f, 0.85f);                 /* a pauldron */
    for (int y = 49; y <= 61; y++)                                  /* the helm, rolled on its side */
        for (int x = 9; x <= 21; x++) {
            float nx = (x + 0.5f - 15) / 6.5f, ny = (y + 0.5f - 55) / 6.5f;
            if (nx * nx + ny * ny > 1.0f) continue;
            float v = 0.25f + 0.6f * lit(nx, ny, sqrtf(fmaxf(0, 1 - nx * nx - ny * ny)));
            if (fabsf(x + 0.5f - 13.0f) < 0.8f && ny > -0.6f) v = 0.0f;   /* the slit, now upright */
            cv_px(x, y, sh(R_IRON, v));
        }
    for (int i = 4; i < 34; i++) cv_px(20 + i, 62 - i / 8, sh(R_GHOST, 0.7f - (i & 1) * 0.15f));   /* the sword, flat on the floor */
    cv_bone_decay(323);
    cv_outline(0);
    cv_store(id);
}

/* The deacon: a skeleton priest in a hooded violet robe (PF_ROBE) with a red stole, a staff with a
 * skull on it. pose 0-1 walking, 2 raising the dead, 3 fallen. */
static void spr_deacon(int id, float w, int pose) {
    cv_begin(cv_sprite, TS, TS);
    if (pose == 3) {
        cv_flat(32, 61.5f, 24, 2.5f, R_WARM + 1);
        float m[12] = { 10, 62, 16, 52, 28, 48, 42, 50, 54, 57, 56, 62 };
        cv_poly(m, 6, R_PURPLE, 0.45f, 0.1f, 48, 14);
        cv_capsule(20, 60, 50, 55, 1.2f, R_BLOOD, 0.15f, 0.45f);
        cv_capsule(6, 61, 58, 59, 1.1f, R_WOOD, 0.2f, 0.6f);        /* the staff, across */
        cv_skull(30, 50, 0.95f, 0.3f, 1.0f);
        cv_bone_decay(331);
        cv_outline(0); cv_store(id); return;
    }
    int chant = pose == 2;
    cv_flat(32, 62, 15, 1.8f, R_WARM + 1);
    if (chant)                                                      /* the dead's light, rising round it */
        for (int i = 0; i < 26; i++) {
            float x = 32 + (h01(i, 1, 333) - 0.5f) * 44, y = 30 + 32 * h01(i, 2, 333);
            cv_px((int)x, (int)y, (uint8_t)(R_GLOW + 6 + (int)(8 * h01(i, 3, 333))));
        }
    /* the staff, in its right hand */
    float stx = chant ? 47.0f : 45.0f + w * 0.5f, sty = chant ? 0.0f : 6.0f;
    cv_capsule(stx, sty + 6, stx - 1, 62, 1.2f, R_WOOD, 0.2f, 0.65f);
    cv_skull(stx, sty + 3, 0.7f, 0.3f, 1.0f);
    cv_px((int)stx - 1, (int)sty + 4, (uint8_t)(chant ? R_GLOW + 14 : R_FLAME + 6)); cv_px((int)stx + 2, (int)sty + 4, (uint8_t)(chant ? R_GLOW + 14 : R_FLAME + 6));
    /* the robe, falling from the shoulders to a hem that drags */
    for (int y = 16; y < 63; y++) {
        float t = (y - 16) / 46.0f, hw = 8.5f + 7.0f * t, sway = w * 1.2f * t;
        for (int x = (int)(32 - hw + sway) - 1; x <= (int)(32 + hw + sway) + 1; x++) {
            float dx = x + 0.5f - 32 - sway;
            if (fabsf(dx) > hw) continue;
            if (y > 58 && h01(x, y, 335 + (w > 0)) < (y - 58) / 5.0f) continue;   /* ragged hem */
            float u = dx / hw, v = 0.15f + 0.4f * (1.0f - u * u) * (1.0f - 0.3f * t);
            if (sinf(dx * 1.1f + y * 0.05f) > 0.6f) v -= 0.08f;     /* folds */
            cv_px(x, y, sh(R_PURPLE, v));
        }
    }
    for (int s = -1; s <= 1; s += 2)                                /* the stole down the front */
        for (int y = 20; y < 52; y++) {
            int x = 32 + s * 3 + (int)(w * 1.0f * (y - 16) / 46.0f);
            cv_px(x, y, sh(R_BLOOD, 0.3f + 0.15f * (s > 0))); cv_px(x + s, y, sh(R_BLOOD, 0.22f));
            if (y == 49 || y == 50) { cv_px(x - s, y, sh(R_BONE, 0.7f)); }
        }
    /* arms: sleeves, and bone hands out of them */
    for (int s = -1; s <= 1; s += 2) {
        float sx = 32 + s * 8, sy = 20, hx, hy;
        if (chant) { hx = 32 + s * 17; hy = 6; }
        else if (s > 0) { hx = stx - 1; hy = 34; }
        else { hx = 21 - w; hy = 38; }
        float ex = (sx + hx) * 0.5f + s * 3.0f, ey = (sy + hy) * 0.5f + 2.0f;
        if (chant) { cv_bone(sx, sy, ex, ey, 1.2f, 0.25f, 0.9f); cv_bone(ex, ey, hx, hy + 2, 1.0f, 0.25f, 0.9f); }   /* the sleeves fallen back */
        cv_capsule(sx, sy, chant ? sx + s * 2.0f : ex, chant ? sy + 4.0f : ey, 3.4f, R_PURPLE, 0.15f, 0.55f);
        if (!chant) cv_capsule(ex, ey, hx, hy - 1, 3.0f, R_PURPLE, 0.15f, 0.5f);
        cv_ellipse(hx, hy, 1.9f, 1.8f, R_BONE, 0.3f, 0.95f);
        for (int f = 0; f < 3; f++) cv_line(hx, hy, hx + s * (1.0f + f) * (chant ? 0.6f : 0.4f), hy + (chant ? -3.0f : 3.0f) + f * (chant ? 0.0f : 0.5f), sh(R_BONE, 0.8f));
        if (chant) for (int k = 0; k < 6; k++) cv_px((int)(hx + (h01(k, s + 2, 337) - 0.5f) * 8), (int)(hy - 2 - 6 * h01(k, s + 5, 337)), (uint8_t)(R_GLOW + 10 + (k & 3)));
    }
    /* the hood, and the skull in its shadow */
    cv_ellipse(32, 12, 8.0f, 8.5f, R_PURPLE, 0.15f, 0.6f);
    cv_flat(32, 13.5f, 5.6f, 6.4f, 0);
    cv_skull(32, 13, 0.82f, 0.25f, 0.85f);
    cv_px(30, 14, (uint8_t)(chant ? R_GLOW + 14 : R_FLAME + 8)); cv_px(34, 14, (uint8_t)(chant ? R_GLOW + 14 : R_FLAME + 8));
    cv_bone_decay(339);
    cv_outline(0);
    cv_store(id);
}

/* the bestiary's bones age with the level, like the skeletons' */
static void spr_bestiary_bones(void) {
    spr_hound(SP_HOUND_A, 1.0f, 0); spr_hound(SP_HOUND_B, -1.0f, 1); spr_hound(SP_HOUND_BITE, 0.0f, 2); spr_hound(SP_HOUND_D, 0.0f, 3);
    for (int st = 0; st < 3; st++) {
        int base = SP_KNIGHT_A + st * 4;
        spr_knight(base, 1.0f, 0, st); spr_knight(base + 1, -1.0f, 1, st); spr_knight(base + 2, 0.0f, 2, st); spr_knight(base + 3, 0.0f, 3, st);
        spr_knight(SP_KNIGHT_P0 + st, 0.0f, 4, st);
    }
    spr_knight_dead(SP_KNIGHT_D);
    spr_deacon(SP_DEACON_A, 1.0f, 0); spr_deacon(SP_DEACON_B, -1.0f, 1); spr_deacon(SP_DEACON_CHANT, 0.0f, 2); spr_deacon(SP_DEACON_D, 0.0f, 3);
}

/* a spider's leg in two joints from the body at (kx, ky) out to the foot, banded if band >= 0 */
static void spider_leg(float kx, float ky, float mx, float my, float fx, float fy, float r, int ramp, int band) {
    cv_capsule(kx, ky, mx, my, r, ramp, 0.12f, 0.6f);
    cv_capsule(mx, my, fx, fy, r * 0.75f, ramp, 0.1f, 0.55f);
    if (band >= 0) {                                                /* dark rings round the joints */
        cv_ellipse(mx, my, r * 1.1f, r * 1.1f, band, 0.02f, 0.2f);
        cv_ellipse((mx + fx) * 0.5f, (my + fy) * 0.5f, r * 0.8f, r * 0.8f, band, 0.02f, 0.18f);
    }
}

/* The hunting spider: a big brown wolf spider, hairy, low and wide, its two great front eyes shining
 * back at you. Body R_WOOD (PF_HIDE), dark bands. pose 0-1 running, 2 leaping, 3 dead. */
static void spr_hunter(int id, int frame) {
    cv_begin(cv_sprite, TS, TS);
    float cx = 32, cy = frame == 3 ? 54 : 44;
    if (frame != 2 && frame != 3) cv_flat(32, 62, 24, 2.0f, R_WARM + 1);
    for (int s = -1; s <= 1; s += 2)
        for (int l = 0; l < 4; l++) {                               /* legs: up into a high knee, then out and down */
            float k = (frame == 1 ? (l & 1 ? 1.0f : -1.0f) : (frame == 0 ? (l & 1 ? -1.0f : 1.0f) : 0.0f)) * s;
            float kx = cx + s * (5 + l * 1.2f), ky = cy - 2 + l * 2.0f;
            float mx = cx + s * (14 + l * 3.0f), my = cy - 14 + l * 3.0f + k * 2.0f;
            float fx = cx + s * (20 + l * 4.0f) + k, fy = 61 - (l == 0 ? 2.0f : 0.0f);
            if (frame == 2) { mx = cx + s * (12 + l * 2); my = cy - 16 - (3 - l) * 2.0f; fx = cx + s * (16 + l * 4); fy = cy - 24 + l * 6.0f; }
            if (frame == 3) { mx = cx + s * (8 + l * 2); my = cy - 10 - l * 2; fx = cx + s * (4 + l); fy = cy - 16 - l * 1.5f; }
            spider_leg(kx, ky, mx, my, fx, fy, 1.5f, R_WOOD, R_IRON);
        }
    cv_ellipse(cx, cy - 3, 10.0f, 7.0f, R_WOOD, 0.1f, 0.55f);     /* the abdomen, behind and above */
    for (int i = 0; i < 4; i++) cv_line(cx - 6 + i * 4, cy - 8, cx - 5 + i * 3.4f, cy + 1, sh(R_IRON, 0.12f));   /* chevrons */
    cv_ellipse(cx, cy + 4, 8.0f, 6.0f, R_WOOD, 0.15f, 0.65f);     /* the head and fore-body */
    for (int y = (int)cy - 10; y < (int)cy + 10; y++)               /* hair: a fringe of lighter flecks round the edge */
        for (int x = (int)cx - 12; x < (int)cx + 12; x++) {
            uint8_t c = cv_sprite[y * TS + x];
            if (c >= R_WOOD && c < R_WOOD + 16 && h01(x, y, 401) > 0.82f) cv_px(x, y, (uint8_t)(c + 3 > R_WOOD + 15 ? R_WOOD + 15 : c + 3));
        }
    if (frame != 3) {
        for (int k = -1; k <= 1; k += 2) {                          /* the two great eyes, shining */
            cv_flat(cx + k * 2.6f, cy + 2.5f, 2.0f, 2.0f, 0);
            cv_px((int)(cx + k * 2.6f), (int)(cy + 2), (uint8_t)(R_FLAME + 14)); cv_px((int)(cx + k * 2.6f) + (k > 0 ? -1 : 1), (int)(cy + 3), (uint8_t)(R_FLAME + 9));
        }
        static const int EYE[6][2] = { {-5, 0}, {5, 0}, {-2, -1}, {2, -1}, {-4, 6}, {4, 6} };
        for (int i = 0; i < 6; i++) cv_px((int)cx + EYE[i][0], (int)cy + EYE[i][1], (uint8_t)(R_FLAME + 4));
        float open = frame == 2 ? 3.0f : 1.0f;
        for (int k = -1; k <= 1; k += 2) cv_capsule(cx + k * 2.0f, cy + 7, cx + k * (2.0f + open * 0.5f), cy + 10 + open, 0.9f, R_IRON, 0.1f, 0.4f);   /* fangs */
        for (int k = -1; k <= 1; k += 2) cv_capsule(cx + k * 6.0f, cy + 6, cx + k * 7.0f, cy + 11, 1.2f, R_WOOD, 0.15f, 0.5f);   /* palps */
    }
    cv_outline(0);
    cv_store(id);
}

/* The brood mother: a black spider the size of a dog, her abdomen swollen with eggs that show pale
 * through the skin. Body R_IRON (PF_CHITIN). pose 0-1 walking, 2 laying, 3 dead. */
static void spr_brood(int id, int frame) {
    cv_begin(cv_sprite, TS, TS);
    float cx = 32, cy = frame == 3 ? 52 : 42, ab = frame == 2 ? -6.0f : 0.0f;
    if (frame != 3) cv_flat(32, 62, 28, 2.2f, R_WARM + 1);
    cv_ellipse(cx, cy - 12 + ab, 17.0f, 13.0f, R_IRON, 0.05f, 0.42f);   /* the abdomen, raised behind her */
    for (int i = 0; i < 14; i++) {                                  /* eggs pressing pale against the skin */
        float a = h01(i, 1, 411) * TAU_F, r = 11.0f * sqrtf(h01(i, 2, 411));
        cv_ellipse(cx + cosf(a) * r * 1.2f, cy - 12 + ab + sinf(a) * r * 0.8f, 2.4f, 2.0f, R_BONE, 0.15f, 0.45f);
    }
    cv_flat(cx, cy - 6 + ab, 3.0f, 2.4f, (uint8_t)(R_BLOOD + 9));   /* her mark */
    if (frame == 2) {                                               /* a sac coming, on a thread */
        cv_line(cx, cy - 2 + ab, cx, cy + 12, sh(R_STONE, 0.8f));
        cv_ellipse(cx, cy + 16, 5.0f, 5.5f, R_BONE, 0.3f, 0.85f);
    }
    for (int s = -1; s <= 1; s += 2)
        for (int l = 0; l < 4; l++) {
            float k = (frame == 1 ? (l & 1 ? 1.0f : -1.0f) : (frame == 0 ? (l & 1 ? -1.0f : 1.0f) : 0.0f)) * s;
            float kx = cx + s * (7 + l * 1.5f), ky = cy + l * 2.0f;
            float mx = cx + s * (18 + l * 2.5f), my = cy - 14 + l * 3.5f + k * 2.0f;
            float fx = cx + s * (24 + l * 2.0f) + k, fy = 61;
            if (frame == 3) { mx = cx + s * (9 + l * 2); my = cy - 12 - l * 2; fx = cx + s * (5 + l); fy = cy - 18 - l; }
            spider_leg(kx, ky, mx, my, fx, fy, 2.0f, R_IRON, -1);
        }
    cv_ellipse(cx, cy + 3, 9.0f, 7.0f, R_IRON, 0.06f, 0.45f);     /* head */
    if (frame != 3) {
        static const int EYE[10][2] = { {-3, 0}, {3, 0}, {-1, -1}, {1, -1}, {-5, 1}, {5, 1}, {-2, 2}, {2, 2}, {-4, -2}, {4, -2} };
        for (int i = 0; i < 10; i++) cv_px((int)cx + EYE[i][0], (int)cy + EYE[i][1], (uint8_t)(R_FLAME + (i < 4 ? 5 : 2)));
        for (int k = -1; k <= 1; k += 2) cv_capsule(cx + k * 2.5f, cy + 7, cx + k * 3.5f, cy + 12, 1.1f, R_BONE, 0.3f, 0.75f);
    }
    cv_outline(0);
    cv_store(id);
}

/* an egg sac, stuck to the wall: still, stirring with shapes inside, then split and empty */
static void spr_sac(int id, int frame) {
    cv_begin(cv_sprite, TS, TS);
    float cx = 32, cy = 40, r = frame == 1 ? 13.0f : 12.0f;
    for (int i = 0; i < 9; i++) {                                   /* the silk that holds it, slung from above */
        float x = cx + (h01(i, 1, 421) - 0.5f) * 30;
        cv_line(x, 0, cx + (x - cx) * 0.4f, cy - r + 2, sh(R_STONE, 0.6f + 0.2f * (i & 1)));
    }
    if (frame == 2) {                                               /* split: torn lips of silk round a dark hollow */
        cv_ellipse(cx, cy, r, r * 1.1f, R_BONE, 0.2f, 0.7f);
        cv_flat(cx, cy + 1, r * 0.6f, r * 0.8f, 0);
        for (int i = 0; i < 10; i++) {
            float a = h01(i, 2, 423) * TAU_F;
            cv_line(cx + cosf(a) * r * 0.55f, cy + sinf(a) * r * 0.75f, cx + cosf(a) * r * 0.75f, cy + sinf(a) * r * 0.95f, sh(R_BONE, 0.9f));
        }
        cv_line(cx, cy + r, cx + 2, cy + r + 8, sh(R_BONE, 0.5f));  /* a strand trailing */
        cv_outline(0); cv_store(id); return;
    }
    cv_ellipse(cx, cy, r, r * 1.1f, R_BONE, 0.3f, 0.95f);
    for (int y = (int)(cy - r * 1.1f); y <= (int)(cy + r * 1.1f); y++)   /* wound silk, and the dark shapes inside */
        for (int x = (int)(cx - r); x <= (int)(cx + r); x++) {
            uint8_t *p = &cv_sprite[y * TS + x];
            if (*p < R_BONE || *p >= R_BONE + 16) continue;
            if (sinf((x - cx) * 0.8f + (y - cy) * 1.3f) > 0.85f) *p = (uint8_t)(*p + 2 > R_BONE + 15 ? R_BONE + 15 : *p + 2);
            float n = fbm(x / 3.0f + frame * 2.3f, y / 3.0f, 32, 425);
            if (n > 0.62f) *p = sh(R_IRON, 0.2f + 0.2f * (1.0f - n));
        }
    if (frame == 1) for (int i = 0; i < 4; i++) cv_px((int)(cx - 6 + i * 4), (int)(cy - 3 + (i & 1) * 5), (uint8_t)(R_FLAME + 3));   /* eyes, inside */
    cv_outline(0);
    cv_store(id);
}

/* spiderlings: drawn bold, to be shown at a fifth of the size */
static void spr_ling(int id, int frame) {
    cv_begin(cv_sprite, TS, TS);
    for (int s = -1; s <= 1; s += 2)
        for (int l = 0; l < 4; l++) {
            float k = (frame ? (l & 1 ? 1.0f : -1.0f) : (l & 1 ? -1.0f : 1.0f)) * s * 3.0f;
            cv_capsule(32 + s * 6, 40 + l * 3, 32 + s * (20 + l * 2), 30 + l * 6 + k, 2.4f, R_IRON, 0.05f, 0.4f);
            cv_capsule(32 + s * (20 + l * 2), 30 + l * 6 + k, 32 + s * (26 + l * 1.5f), 60, 2.0f, R_IRON, 0.05f, 0.35f);
        }
    cv_ellipse(32, 38, 12, 10, R_IRON, 0.06f, 0.45f);
    cv_ellipse(32, 48, 8, 7, R_IRON, 0.08f, 0.5f);
    for (int k = -1; k <= 1; k += 2) cv_flat(32 + k * 3.5f, 47, 2.2f, 2.2f, (uint8_t)(R_FLAME + 5));
    cv_outline(0);
    cv_store(id);
}

/* The Wraith: tall, solid and dark, a cold light along its edges and frost falling off its hem.
 * Drawn to be shown half as tall again as a man. Off the rank ladder. pose 0-1 gliding, 2 reaching
 * for you, 3 driven back, arms over its face. */
static void spr_wraith_big(int id, float ph, int pose) {
    cv_begin(cv_sprite, TS, TS);
    float flare = pose == 3 ? 1.0f : 0.0f;
    for (int y = 6; y < TS; y++)                                    /* the robe, from the shoulders to rags that trail */
        for (int x = 0; x < TS; x++) {
            float dx = x + 0.5f - 32.0f, t = (y - 6) / 57.0f;
            float hw = (y < 16 ? 9.0f + (y - 6) * 0.55f : 14.5f + (y - 16) * (0.12f + 0.12f * flare)) + sinf(y * 0.21f + ph) * 0.8f * t;
            if (fabsf(dx) > hw) continue;
            float hem = 52.0f + 5.0f * sinf(dx * 0.7f + ph) + 4.0f * sinf(dx * 1.9f - ph * 1.3f);
            if (y > hem && h01(x / 2, y, 501 + (int)(ph * 3)) > (63.0f - y) / (63.0f - hem + 0.01f) * 0.9f) continue;   /* rags */
            float u = dx / hw, v = 0.03f + 0.13f * (1.0f - u * u) - t * 0.05f;
            if (sinf(dx * 0.75f + sinf(y * 0.08f + ph) * 1.2f) > 0.75f) v -= 0.04f;   /* deep folds */
            if (fabsf(u) > 0.86f) { cv_px(x, y, sh(R_GHOST, 0.25f + 0.35f * (fabsf(u) - 0.86f) / 0.14f)); continue; }   /* the cold rim */
            cv_px(x, y, sh(R_IRON, v));
        }
    /* the hood: high and pointed, its mouth black, two eyes like stars in it */
    for (int y = 0; y < 20; y++)
        for (int x = 18; x < 46; x++) {
            float dx = x + 0.5f - 32.0f, top = 1.0f + dx * dx * 0.06f;
            float hw = y < 8 ? (y - 0.5f) * 1.0f : 8.0f + (y - 8) * 0.25f;
            if (y < top || fabsf(dx) > hw) continue;
            float u = dx / (hw + 0.01f), v = 0.05f + 0.14f * (1.0f - u * u);
            if (fabsf(u) > 0.8f) v = 0.3f;
            cv_px(x, y, fabsf(u) > 0.8f ? sh(R_GHOST, v) : sh(R_IRON, v));
        }
    cv_flat(32, 12.5f, 5.0f, 6.0f, 0);
    float ey = 11.5f, gap = pose == 2 ? 3.0f : 2.5f;
    for (int k = -1; k <= 1; k += 2) {
        int ex = (int)(32 + k * gap - (k < 0));
        if (pose == 3) { cv_px(ex, (int)ey, (uint8_t)(R_GLOW + 12)); cv_px(ex + k, (int)ey, (uint8_t)(R_GLOW + 9)); continue; }   /* narrowed */
        cv_px(ex, (int)ey, C_WHITE); cv_px(ex, (int)ey + 1, (uint8_t)(R_GLOW + 13));
        cv_px(ex + k, (int)ey, (uint8_t)(R_GLOW + 11)); cv_px(ex, (int)ey - 1, (uint8_t)(R_GLOW + 9));
        if (pose == 2) { cv_px(ex - k, (int)ey, (uint8_t)(R_GLOW + 11)); cv_px(ex, (int)ey + 2, (uint8_t)(R_GLOW + 8)); }
    }
    /* arms: long sleeves, and longer fingers */
    for (int s = -1; s <= 1; s += 2) {
        float sx = 32 + s * 12.0f, sy = 17, hx, hy;
        if (pose == 3) { hx = 32 + s * 4.0f; hy = 8; }             /* over its face */
        else if (pose == 2 && s > 0) { hx = 44; hy = 22; }          /* reaching out at you */
        else { hx = 32 + s * 17.0f + sinf(ph + s) * 1.0f; hy = 44 + cosf(ph * 0.7f + s) * 1.5f; }
        float ex = (sx + hx) * 0.5f + s * 3.0f, ey2 = (sy + hy) * 0.5f;
        cv_capsule(sx, sy, ex, ey2, 4.0f, R_IRON, 0.04f, 0.22f);
        cv_capsule(ex, ey2, hx, hy - 2, 3.4f, R_IRON, 0.04f, 0.2f);
        cv_line(ex + s * 3.5f, ey2, hx + s * 3.0f, hy - 2, sh(R_GHOST, 0.4f));   /* the rim catching the sleeve */
        float sc = pose == 2 && s > 0 ? 1.5f : 1.0f, base = pose == 3 ? -1.57f - s * 0.4f : (pose == 2 && s > 0 ? -0.3f : 1.57f + s * 0.15f);
        cv_ellipse(hx, hy, 2.0f * sc, 1.8f * sc, R_BONE, 0.15f, 0.5f);
        for (int f = 0; f < 4; f++) {                               /* five-inch fingers, frost at the tips */
            float a = base + (f - 1.5f) * 0.32f, l = (7.0f + (f == 1 || f == 2) * 2.0f) * sc;
            float mx = hx + cosf(a) * l * 0.55f, my = hy + sinf(a) * l * 0.55f, tx = hx + cosf(a + 0.25f * s) * l, ty = hy + sinf(a + 0.25f * s) * l;
            cv_line(hx, hy, mx, my, sh(R_BONE, 0.45f)); cv_line(mx, my, tx, ty, sh(R_BONE, 0.35f));
            cv_px((int)tx, (int)ty, sh(R_GHOST, 0.9f));
        }
    }
    for (int i = 0; i < 40; i++) {                                  /* frost falling from the hem */
        float x = 32 + (h01(i, 1, 503) - 0.5f) * 44, y = 50 + 13 * h01(i, (int)(ph * 2) + 2, 503);
        if (cv_sprite[(int)y * TS + (int)x] == C_CLEAR) cv_px((int)x, (int)y, sh(R_GHOST, 0.45f + 0.4f * h01(i, 3, 503)));
    }
    cv_store(id);
}

/* The shade: a shape of darkness, seen only in light: near black, a violet edge that smokes away,
 * two dim eyes. pose 0-1 drifting, 2 reaching, 3 coming apart. */
static void spr_shade(int id, float ph, int pose) {
    cv_begin(cv_sprite, TS, TS);
    float gone = pose == 3 ? 1.0f : 0.0f;
    for (int y = 4; y < TS; y++)
        for (int x = 0; x < TS; x++) {
            float dx = x + 0.5f - 32.0f;
            float hw;
            if (y < 16) { float hy = (y - 10.0f) / 6.0f; hw = 5.5f * sqrtf(fmaxf(0.0f, 1.0f - hy * hy)); }   /* the head */
            else if (y < 20) hw = 3.0f;                              /* the neck */
            else hw = 10.0f + (y < 30 ? (y - 20) * 0.2f : 2.0f - (y - 30) * 0.12f);   /* shoulders down to a waist of smoke */
            hw += sinf(y * 0.4f + ph) * 0.8f;
            float edge = hw - fabsf(dx);
            if (edge < 0.0f) continue;
            float fadey = y > 44 ? (y - 44) / 19.0f : 0.0f;          /* it has no feet: it thins to nothing */
            float n = h01(x, y, 511 + (int)(ph * 5));
            if (n < fadey * 1.1f || n < gone * 0.75f) continue;
            if (edge < 1.5f && ((x + y) & 1)) continue;             /* a smoking edge */
            cv_px(x, y, edge < 2.5f ? sh(R_PURPLE, 0.12f + 0.12f * (1.0f - fadey)) : sh(R_IRON, 0.02f + 0.04f * n));
        }
    for (int s = -1; s <= 1; s += 2) {                              /* arms of smoke */
        float hx = pose == 2 && s > 0 ? 50.0f : 32 + s * 13.0f, hy = pose == 2 && s > 0 ? 22.0f : 40.0f + sinf(ph + s) * 2.0f;
        for (int i = 0; i <= 14; i++) {
            float t = i / 14.0f, x = lerpf(32 + s * 9.0f, hx, t), y = lerpf(22, hy, t), r = 2.6f - t * 1.2f;
            for (int yy = (int)(y - r); yy <= (int)(y + r); yy++)
                for (int xx = (int)(x - r); xx <= (int)(x + r); xx++)
                    if (hypotf(xx + 0.5f - x, yy + 0.5f - y) <= r && h01(xx, yy, 513) > gone * 0.7f)
                        cv_px(xx, yy, (xx + yy) & 1 ? sh(R_PURPLE, 0.15f) : sh(R_IRON, 0.04f));
        }
    }
    if (pose != 3) {
        cv_px(30, 10, sh(R_PURPLE, 0.85f)); cv_px(34, 10, sh(R_PURPLE, 0.85f));
        cv_px(30, 11, sh(R_PURPLE, 0.5f)); cv_px(34, 11, sh(R_PURPLE, 0.5f));
    }
    cv_store(id);
}

/* The ember imp: squat, red clay skin cracked over fire inside, horns, a grin full of coals.
 * Skin R_BRICK (PF_CLAY); the cracks are full-bright and keep burning whatever its rank. Drawn to be
 * shown at half size. pose 0-1 hopping, 2 throwing, 3 crumbling. */
static void spr_imp(int id, int pose) {
    cv_begin(cv_sprite, TS, TS);
    if (pose == 3) {
        cv_flat(32, 61.5f, 20, 2.5f, R_WARM + 1);
        for (int i = 0; i < 30; i++) {
            float x = 32 + (h01(i, 1, 521) - 0.5f) * 34, y = 52 + 10 * h01(i, 2, 521);
            cv_ellipse(x, y, 1.0f + 2.0f * h01(i, 3, 521), 1.0f + 1.4f * h01(i, 4, 521), i % 3 ? R_IRON : R_BRICK, 0.1f, 0.45f);
        }
        for (int i = 0; i < 8; i++) cv_px((int)(20 + 24 * h01(i, 5, 521)), (int)(40 + 20 * h01(i, 6, 521)), (uint8_t)(R_FLAME + 8 + (i & 3)));
        cv_outline(0); cv_store(id); return;
    }
    float up = pose == 0 ? -3.0f : 0.0f;
    cv_flat(32, 62, 12, 1.6f, R_WARM + 1);
    cv_capsule(40, 50 + up, 50, 54 + up, 1.3f, R_BRICK, 0.15f, 0.5f);   /* the tail, an ember at its tip */
    cv_capsule(50, 54 + up, 55, 46 + up, 1.1f, R_BRICK, 0.15f, 0.5f);
    cv_flat(56, 45 + up, 1.8f, 1.8f, (uint8_t)(R_FLAME + 12));
    for (int s = -1; s <= 1; s += 2) {                              /* little wings, leathery */
        float wv[8] = { 32 + s * 7, 34 + up, 32 + s * 22, 24 + up, 32 + s * 20, 36 + up, 32 + s * 12, 40 + up };
        cv_poly(wv, 4, R_BRICK, 0.08f, 0.3f, 24 + up, 16);
    }
    for (int s = -1; s <= 1; s += 2) {                              /* legs, bent */
        float kx = 32 + s * 8, ky = 52 + up * 0.5f;
        cv_capsule(32 + s * 5, 46 + up, kx, ky, 2.6f, R_BRICK, 0.2f, 0.75f);
        cv_capsule(kx, ky, 32 + s * 6, 61, 2.0f, R_BRICK, 0.2f, 0.7f);
        cv_ellipse(32 + s * 7, 61.5f, 3.0f, 1.4f, R_BRICK, 0.15f, 0.6f);
    }
    cv_ellipse(32, 44 + up, 9.0f, 8.5f, R_BRICK, 0.25f, 0.9f);    /* pot belly */
    for (int s = -1; s <= 1; s += 2) {                              /* arms: one up with a coal, or both out */
        float hx = pose == 2 && s > 0 ? 46.0f : 32 + s * 13.0f, hy = pose == 2 && s > 0 ? 22.0f + up : 46.0f + up;
        cv_capsule(32 + s * 7, 38 + up, hx, hy, 2.0f, R_BRICK, 0.25f, 0.85f);
        cv_ellipse(hx, hy, 2.4f, 2.2f, R_BRICK, 0.25f, 0.85f);
        if (pose == 2 && s > 0) { cv_flat(hx + 1, hy - 3, 3.0f, 3.0f, (uint8_t)(R_FLAME + 10)); cv_flat(hx + 1, hy - 3, 1.6f, 1.6f, (uint8_t)(R_FLAME + 14)); }
    }
    float hy = 28 + up;                                             /* the head: big, horned, grinning */
    for (int s = -1; s <= 1; s += 2) {
        cv_capsule(32 + s * 6, hy - 6, 32 + s * 10, hy - 12, 1.8f, R_BONE, 0.15f, 0.55f);
        cv_capsule(32 + s * 10, hy - 12, 32 + s * 9, hy - 17, 1.1f, R_BONE, 0.15f, 0.5f);
        float e[6] = { 32 + s * 8, hy - 2, 32 + s * 15, hy - 5, 32 + s * 9, hy + 2 };   /* pointed ears */
        cv_poly(e, 3, R_BRICK, 0.2f, 0.7f, hy - 5, 7);
    }
    cv_ellipse(32, hy, 9.5f, 8.5f, R_BRICK, 0.3f, 1.0f);
    for (int s = -1; s <= 1; s += 2) {
        cv_flat(32 + s * 3.5f, hy - 1.5f, 2.0f, 1.5f, 0);
        cv_px(32 + s * 3 + (s < 0 ? -1 : 0), (int)(hy - 2), (uint8_t)(R_FLAME + 14)); cv_px(32 + s * 3 + (s < 0 ? 0 : 1), (int)(hy - 1), (uint8_t)(R_FLAME + 10));
    }
    for (int x = 26; x <= 38; x++) {                                /* the grin, wide and lit from inside */
        int y = (int)(hy + 3 + (x - 32) * (x - 32) * -0.04f + 1.5f);
        cv_px(x, y, (uint8_t)(R_FLAME + 9)); cv_px(x, y + 1, (uint8_t)(R_FLAME + ((x & 1) ? 13 : 6)));
        if (!(x & 1)) cv_px(x, y - 1, sh(R_BONE, 0.8f));
    }
    for (int k = 0; k < 7; k++) {                                   /* cracks with the fire showing through */
        float x = 24 + 16 * h01(k, 1, 523), y = 36 + 14 * h01(k, 2, 523) + up, a = h01(k, 3, 523) * TAU_F;
        for (int i = 0; i < 5; i++) {
            a += (h01(k, i + 4, 523) - 0.5f) * 1.4f; x += cosf(a); y += sinf(a);
            uint8_t c = cv_sprite[(int)y * TS + (int)x];
            if (c >= R_BRICK && c < R_BRICK + 16) cv_px((int)x, (int)y, (uint8_t)(R_FLAME + 6 + (i & 1) * 3));
        }
    }
    cv_outline(0);
    cv_store(id);
}

/* an ember in flight: a coal and its sparks */
static void spr_ember(int id, int frame) {
    cv_begin(cv_sprite, TS, TS);
    for (int i = 0; i < 10; i++) {
        float x = 32 + (h01(i, frame, 531) - 0.5f) * 18, y = 32 + (h01(i, frame + 3, 531) - 0.5f) * 18;
        cv_px((int)x, (int)y, (uint8_t)(R_FLAME + 6 + (i & 7)));
    }
    cv_flat(32, 32, 6.0f, 5.5f, (uint8_t)(R_FLAME + 7));
    cv_flat(32 + (frame ? 0.5f : -0.5f), 31, 4.0f, 3.8f, (uint8_t)(R_FLAME + 11));
    cv_flat(31, 30, 2.0f, 1.8f, (uint8_t)(R_FLAME + 15));
    for (int i = 0; i < 5; i++) cv_px(26 + i * 3, 33 + (int)(2 * h01(i, frame + 7, 531)), R_IRON + 2);   /* the char on it */
    cv_store(id);
}

/* The gargoyle: carved stone (R_STONE, PF_STONE: it is the same stone as the walls), winged and
 * horned. pose 0 perched on its plinth, asleep (the statue in an alcove looks just like it); 1-2
 * flying, wings up and down; 3 swooping, claws first; 4 broken. */
static void spr_gargoyle(int id, int pose) {
    cv_begin(cv_sprite, TS, TS);
    if (pose == 4) {
        cv_flat(32, 61.5f, 24, 2.5f, R_WARM + 1);
        for (int i = 0; i < 9; i++) {
            float x = 32 + (h01(i, 1, 541) - 0.5f) * 40, y = 56 + 6 * h01(i, 2, 541);
            cv_ellipse(x, y, 2.5f + 3.0f * h01(i, 3, 541), 2.0f + 2.0f * h01(i, 4, 541), R_STONE, 0.1f, 0.65f);
        }
        float wv[8] = { 34, 50, 52, 44, 50, 54, 38, 57 };          /* a broken wing */
        cv_poly(wv, 4, R_STONE, 0.15f, 0.55f, 44, 13);
        cv_ellipse(18, 53, 5.5f, 5.0f, R_STONE, 0.15f, 0.7f);      /* the head, face up */
        cv_capsule(15, 48, 13, 44, 1.2f, R_STONE, 0.2f, 0.6f);
        cv_outline(0); cv_store(id); return;
    }
    int perch = pose == 0, swoop = pose == 3;
    float by = perch ? 0.0f : -6.0f;                                /* it rides higher when it flies */
    if (perch) {                                                    /* the plinth */
        for (int y = 52; y < 64; y++)
            for (int x = 18; x <= 46; x++) {
                float nx = (x + 0.5f - 32) / 14.0f;
                stone_px(x, y, nx, y < 54 ? 0.15f : 0.0f, 543);
            }
    }
    /* wings: folded round it, or spread */
    for (int s = -1; s <= 1; s += 2) {
        if (perch) {
            float wv[8] = { 32 + s * 8, 26, 32 + s * 17, 18, 32 + s * 16, 46, 32 + s * 9, 50 };
            cv_poly(wv, 4, R_STONE, 0.1f, 0.5f, 18, 32);
        } else {
            float tip = pose == 1 ? -2.0f : (pose == 2 ? 26.0f : 6.0f), out = swoop ? 22.0f : 30.0f;
            float wv[12] = { 32 + s * 6, 30 + by, 32 + s * 14, 16 + by + tip * 0.4f, 32 + s * out, 6 + by + tip, 32 + s * (out - 3), 22 + by + tip * 0.6f,
                             32 + s * (out - 6), 30 + by + tip * 0.4f, 32 + s * 10, 40 + by };
            cv_poly(wv, 6, R_STONE, 0.1f, 0.6f, 6 + by + tip, 34);
            for (int r = 0; r < 3; r++)                             /* ribs of the wing */
                cv_line(32 + s * 8, 30 + by, 32 + s * (out - r * 4), 8 + by + tip + r * 6, sh(R_STONE, 0.15f));
        }
    }
    /* legs and claws */
    for (int s = -1; s <= 1; s += 2) {
        float kx = 32 + s * 8, ky = perch ? 46.0f : 46.0f + by, fx = 32 + s * 7, fy = perch ? 52.0f : (swoop ? 40.0f + by : 56.0f + by);
        if (swoop) { kx = 32 + s * 6; ky = 38 + by; fx = 32 + s * 9; fy = 48 + by; }
        cv_capsule(32 + s * 5, 40 + by * (perch ? 0 : 1), kx, ky, 3.0f, R_STONE, 0.2f, 0.75f);
        cv_capsule(kx, ky, fx, fy, 2.2f, R_STONE, 0.2f, 0.7f);
        for (int c = -1; c <= 1; c++) cv_line(fx, fy, fx + c * 2.0f, fy + 3.5f, sh(R_STONE, 0.35f));
    }
    cv_ellipse(32, 36 + (perch ? 0 : by), 9.0f, 10.0f, R_STONE, 0.2f, 0.85f);   /* the body, hunched */
    for (int s = -1; s <= 1; s += 2) {                              /* arms: on its knees, or reaching */
        float hx = swoop ? 32 + s * 12.0f : 32 + s * 6.0f, hy = swoop ? 30.0f + by : (perch ? 44.0f : 42.0f + by);
        cv_capsule(32 + s * 8, 30 + (perch ? 0 : by), hx, hy, 2.2f, R_STONE, 0.25f, 0.8f);
        for (int c = -1; c <= 1; c++) cv_line(hx, hy, hx + c * 1.5f + s * 1.0f, hy + (swoop ? -3.0f : 3.0f), sh(R_STONE, 0.35f));
    }
    float hx = 32, hy = (perch ? 22.0f : 20.0f + by) + (perch ? 2.0f : 0.0f);   /* the head: lowered asleep, up awake */
    for (int s = -1; s <= 1; s += 2) {                              /* horns, curling back */
        cv_capsule(hx + s * 4, hy - 4, hx + s * 8, hy - 9, 1.6f, R_STONE, 0.2f, 0.75f);
        cv_capsule(hx + s * 8, hy - 9, hx + s * 7, hy - 13, 1.1f, R_STONE, 0.2f, 0.7f);
    }
    cv_ellipse(hx, hy, 6.5f, 6.0f, R_STONE, 0.25f, 0.95f);
    cv_ellipse(hx, hy + 4, 4.5f, 2.8f, R_STONE, 0.2f, 0.8f);       /* muzzle */
    for (int s = -1; s <= 1; s += 2) {
        cv_flat(hx + s * 2.5f, hy - 1, 1.5f, 1.0f, perch ? sh(R_STONE, 0.1f) : 0);
        if (!perch) cv_px((int)(hx + s * 2.5f), (int)(hy - 1), (uint8_t)(R_FLAME + 12));
    }
    cv_flat(hx, hy + 5, swoop ? 3.0f : 2.0f, swoop ? 1.6f : 0.6f, 0);
    if (swoop) for (int i = -1; i <= 1; i += 2) cv_px((int)hx + i * 2, (int)hy + 4, sh(R_STONE, 0.9f));
    for (int y = 0; y < TS; y++)                                    /* weathering on the stone */
        for (int x = 0; x < TS; x++) {
            uint8_t *p = &cv_sprite[y * TS + x];
            if (*p < R_STONE || *p >= R_STONE + 16 || (perch && y >= 52)) continue;
            float n = fbm(x / 4.0f, y / 4.0f, 16, 545);
            if (n < 0.3f) *p = (uint8_t)(*p - 2 < R_STONE + 1 ? R_STONE + 1 : *p - 2);
            else if (h01(x, y, 547) > 0.96f) *p = (uint8_t)(*p - 4 < R_STONE + 1 ? R_STONE + 1 : *p - 4);
        }
    cv_outline(0);
    cv_store(id);
}

/* ------------------------------------------------------------------ */
/* The dressing that comes in as you go down (PLAN-bestiary, section 8) */
/* ------------------------------------------------------------------ */

/* a candle of bone-coloured wax, h tall, standing on (x, by), lit */
static void candle(float x, float by, float h, int frame, int k, int ramp) {
    for (int y = (int)(by - h); y <= (int)by; y++) { cv_px((int)x, y, sh(ramp, 0.6f)); cv_px((int)x + 1, y, sh(ramp, 0.4f)); }
    cv_px((int)x, (int)(by - h + 2), sh(ramp, 0.95f));
    flame(x + 1.0f, by - h - 1, 5.5f + (float)((frame + k) & 1), 1.7f, (((frame + k) % 3) - 1) * 0.7f);
}

/* an iron stand of three candles, waist high: the halls' light on the first levels */
static void spr_candlestand(int id, int frame) {
    cv_begin(cv_sprite, TS, TS);
    cv_flat(32, 62, 10, 1.6f, R_WARM + 1);
    cv_capsule(32, 30, 32, 58, 1.2f, R_IRON, 0.2f, 0.8f);
    for (int s = -1; s <= 1; s++) cv_capsule(32, 56, 32 + s * 7, 62, 1.0f, R_IRON, 0.2f, 0.7f);   /* three feet */
    cv_ellipse(32, 30, 9, 2.0f, R_IRON, 0.25f, 0.85f);             /* the drip tray */
    for (int i = 0; i < 4; i++) cv_px(25 + i * 4, 32 + (i & 1), sh(R_BONE, 0.7f));   /* wax run over its rim */
    candle(26, 29, 7, frame, 0, R_BONE); candle(31, 29, 11, frame, 1, R_BONE); candle(37, 29, 8, frame, 2, R_BONE);
    cv_store(id);
}

static void spr_broken_arms(void) {
    cv_begin(cv_sprite, TS, TS);
    cv_flat(32, 62, 26, 2.0f, R_WARM + 1);
    for (int y = 46; y < 63; y++)                                   /* half a round shield, on its back, split along a plank */
        for (int x = 8; x < 34; x++) {
            float nx = (x + 0.5f - 21) / 13.0f, ny = (y + 0.5f - 56) / 6.0f, d = nx * nx + ny * ny;
            if (d > 1.0f || x + 0.5f > 21 + 6 - (y - 46) * 0.3f) continue;
            uint8_t c = d > 0.78f ? sh(R_IRON, 0.35f + 0.4f * -ny) : sh(R_WOOD, 0.2f + 0.25f * -ny + 0.06f * h01(x / 5, 1, 601) - (((x - 8) % 5) == 0 ? 0.1f : 0.0f));
            cv_px(x, y, c);
        }
    cv_capsule(30, 61, 54, 55, 1.0f, R_IRON, 0.4f, 0.95f);         /* the sword, snapped short */
    cv_capsule(28, 58, 33, 63, 0.9f, R_IRON, 0.3f, 0.7f);          /* its guard */
    cv_capsule(25, 63, 30, 61, 1.1f, R_WOOD, 0.2f, 0.5f);          /* the grip */
    cv_capsule(47, 61, 58, 59, 0.9f, R_IRON, 0.35f, 0.85f);        /* and the end of the blade, apart */
    cv_line(54, 55, 55, 57, sh(R_IRON, 0.15f));
    cv_outline(0);
    cv_store(SP_BROKEN_ARMS);
}

/* a grave's earth turned over from below; and the grave open, the coffin's lid split */
static void spr_mound(int id, int open) {
    cv_begin(cv_sprite, TS, TS);
    if (!open) {
        cv_ellipse(32, 59, 20, 7.5f, R_WOOD, 0.08f, 0.45f);        /* heaved up from underneath */
        soil(32, 18, 56, 611);
        for (int i = 0; i < 4; i++) cv_line(20 + i * 8, 58, 23 + i * 7, 52, sh(R_WOOD, 0.02f));   /* cracks across it */
        cv_store(id); return;
    }
    soil(14, 10, 58, 613); soil(50, 10, 58, 615);
    cv_flat(32, 59.5f, 14, 3.6f, 0);                                /* the hole */
    for (int x = 19; x <= 45; x++) cv_px(x, 57, sh(R_WOOD, 0.08f));
    cv_capsule(22, 59, 30, 54, 1.4f, R_WOOD, 0.15f, 0.55f);        /* the lid, split, boards up */
    cv_capsule(34, 54, 43, 60, 1.4f, R_WOOD, 0.15f, 0.5f);
    cv_line(30, 54, 31, 51, sh(R_WOOD, 0.4f)); cv_line(34, 54, 33, 50, sh(R_WOOD, 0.35f));   /* splinters */
    cv_outline(0);
    cv_store(id);
}

/* standing water: still, a glint on it; or stirred, rings and bubbles (something under it) */
static void spr_puddle(int id, int stir) {
    cv_begin(cv_sprite, TS, TS);
    float cx = 32, cy = 59, w = 24, hh = 4.5f;
    for (int y = (int)(cy - hh); y <= (int)(cy + hh); y++)
        for (int x = (int)(cx - w); x <= (int)(cx + w); x++) {
            float nx = (x + 0.5f - cx) / w, ny = (y + 0.5f - cy) / hh, d = nx * nx + ny * ny + 0.15f * sinf(x * 0.4f);
            if (d > 1.0f) continue;
            float v = 0.06f + 0.1f * (1.0f + ny) * 0.5f;
            if (ny < -0.4f && fabsf(nx) < 0.5f && h01(x, y, 621) > 0.6f) v = 0.45f;   /* the far edge catches the light */
            if (stir) { float r = sinf(sqrtf(nx * nx * 3 + ny * ny) * 10.0f); if (r > 0.8f) v = 0.35f; }
            cv_px(x, y, sh(R_GHOST, v));
        }
    for (int x = (int)(cx - w * 0.6f); x < (int)(cx + w * 0.6f); x++) if (h01(x, 0, 623) > 0.55f) cv_px(x, (int)(cy - hh * 0.6f), sh(R_GHOST, 0.6f));   /* the glint */
    if (stir) for (int i = 0; i < 6; i++) {
        float x = cx + (h01(i, 1, 625) - 0.5f) * w, y = cy - 1 + 2 * h01(i, 2, 625);
        cv_px((int)x, (int)y - 1, sh(R_GHOST, 0.85f)); cv_px((int)x + 1, (int)y - 2, sh(R_GHOST, 0.6f)); cv_px((int)x, (int)y - 3, sh(R_GHOST, 0.4f));
    }
    cv_store(id);
}

/* flowstone: a stalagmite of wet rock skinned over with pale calcite, and the drip that built it */
static void spr_stalag(void) {
    cv_begin(cv_sprite, TS, TS);
    cv_flat(32, 62, 14, 2.0f, R_WARM + 1);
    for (int y = 14; y < 63; y++) {
        float t = (y - 14) / 49.0f, hw = 1.5f + 11.0f * powf(t, 1.4f) + sinf(y * 0.7f) * 0.6f * t;
        for (int x = (int)(32 - hw) - 1; x <= (int)(32 + hw) + 1; x++) {
            float nx = (x + 0.5f - 32) / (hw + 0.01f);
            if (fabsf(nx) > 1.0f) continue;
            float v = 0.15f + 0.6f * lit(nx, 0.0f, sqrtf(1.0f - nx * nx));
            float skin = fbm(x / 2.5f, y / 9.0f, 32, 619) + 0.25f * (1.0f - t);   /* calcite run down it in streaks, thickest at the tip */
            if (skin > 0.68f) cv_px(x, y, sh(R_BONE, v * 0.85f + 0.08f));
            else cv_px(x, y, sh(R_STONE, v - (skin < 0.35f ? 0.06f : 0.0f)));
        }
    }
    for (int y = 0; y < 10; y++) if (y % 4 != 3) cv_px(32, y, sh(R_GHOST, 0.6f));   /* the drip, falling */
    cv_px(32, 12, sh(R_GHOST, 0.9f));
    cv_outline(0);
    cv_store(SP_STALAG);
}

/* a lantern on a chain: an iron cage, its panes lit from within */
static void spr_lantern(int id, int frame) {
    cv_begin(cv_sprite, TS, TS);
    chain_down(32, 0, 16);
    cv_ellipse(32, 18, 4.0f, 2.5f, R_IRON, 0.25f, 0.85f);           /* the cap */
    for (int y = 20; y < 34; y++)
        for (int x = 26; x <= 38; x++) {
            float nx = (x + 0.5f - 32) / 6.5f;
            if (fabsf(nx) > 1.0f) continue;
            int bar = fabsf(nx) > 0.82f || fabsf(nx) < 0.12f || y == 20 || y == 33;
            if (bar) { cv_px(x, y, sh(R_IRON, 0.3f + 0.4f * lit(nx, 0, sqrtf(1 - nx * nx)))); continue; }
            float glow = 1.0f - fabsf(nx) * 0.6f - fabsf(y - 28.0f) / 14.0f + (frame ? 0.05f : -0.05f);
            cv_px(x, y, (uint8_t)(R_FLAME + clampi((int)(glow * 13.0f), 3, 13)));
        }
    flame(32.5f, 30, 6.0f + frame, 1.8f, frame ? 0.6f : -0.6f);
    cv_ellipse(32, 35, 5.0f, 1.8f, R_IRON, 0.2f, 0.7f);             /* the base */
    cv_px(32, 37, sh(R_IRON, 0.5f));
    cv_store(id);
}

/* a brazier: an iron bowl on three legs, coals and fire in it */
static void spr_brazier(int id, int frame) {
    cv_begin(cv_sprite, TS, TS);
    cv_flat(32, 62, 12, 1.6f, R_WARM + 1);
    for (int s = -1; s <= 1; s++) cv_capsule(32 + s * 6, 44, 32 + s * 10, 62, 1.1f, R_IRON, 0.2f, 0.75f);
    for (int i = 0; i < 3; i++) {
        float x = 32 + (i - 1) * 5 + (h01(i, frame, 631) - 0.5f) * 3, h = 12 + 10 * h01(i, frame + 4, 631);
        flame(x, 36, h, 3.6f, (h01(i, frame + 8, 631) - 0.5f) * 4.0f);
    }
    flame(32, 37, 20 + 2.0f * frame, 5.0f, (frame - 1) * 2.0f);
    for (int y = 36; y <= 45; y++)                                  /* the bowl, in front of the fire's foot */
        for (int x = 18; x <= 46; x++) {
            float nx = (x + 0.5f - 32) / (14.0f - (y - 36) * 0.6f);
            if (fabsf(nx) > 1.0f) continue;
            cv_px(x, y, sh(R_IRON, 0.25f + 0.45f * lit(nx, 0.3f, sqrtf(1 - nx * nx)) + (y == 36 ? 0.25f : 0.0f)));
        }
    for (int i = 0; i < 6; i++) cv_px((int)(22 + 20 * h01(i, frame, 633)), 35, (uint8_t)(R_FLAME + 8 + (i & 3)));   /* coals at the lip */
    for (int i = 0; i < 5; i++) cv_px((int)(32 + (h01(i, frame, 635) - 0.5f) * 20), (int)(6 + 10 * h01(i, frame + 2, 635)), (uint8_t)(R_FLAME + 10));   /* sparks */
    cv_store(id);
}

/* a chain from a ring in the floor to a spiked collar, open, and bones gnawed white */
static void spr_collar(void) {
    cv_begin(cv_sprite, TS, TS);
    cv_flat(32, 62, 24, 2.0f, R_WARM + 1);
    for (int i = 0; i < 4; i++) {
        float cx = 14 + i * 9 + 4 * h01(i, 1, 641), cy = 59 + 2 * h01(i, 2, 641), a = (h01(i, 3, 641) - 0.5f) * 1.4f, l = 4 + 3 * h01(i, 4, 641);
        cv_bone(cx - cosf(a) * l, cy - sinf(a) * l, cx + cosf(a) * l, cy + sinf(a) * l, 1.2f, 0.4f, 1.0f);
        cv_px((int)cx, (int)cy - 1, sh(R_BONE, 0.2f)); cv_px((int)cx + 1, (int)cy, sh(R_BONE, 0.25f));   /* tooth marks */
    }
    chain_line(8, 61, 30, 56);                                      /* from the ring to the collar */
    for (int a = 0; a < 18; a++) { float ang = TAU_F * a / 18; cv_px((int)(7 + 2.2f * cosf(ang)), (int)(61 + 1.4f * sinf(ang)), sh(R_IRON, 0.6f)); }
    for (int a = 0; a < 26; a++) {                                  /* the collar, open, its spikes out */
        float ang = PI_F * (0.1f + 1.6f * a / 25.0f);
        int x = (int)(38 + 7.0f * cosf(ang)), y = (int)(55 + 3.5f * sinf(ang));
        cv_px(x, y, sh(R_IRON, 0.4f + 0.3f * sinf(ang))); cv_px(x, y + 1, sh(R_IRON, 0.25f));
        if (a % 5 == 2) cv_line(x, y, x + cosf(ang) * 3.0f, y + sinf(ang) * 2.0f - 1, sh(R_IRON, 0.85f));
    }
    cv_outline(0);
    cv_store(SP_COLLAR);
}

/* icicles from the ceiling, where something cold has been */
static void spr_icicles(void) {
    cv_begin(cv_sprite, TS, TS);
    for (int i = 0; i < 11; i++) {
        float x = 6 + 52 * h01(i, 1, 651), len = 8 + 28 * h01(i, 2, 651) * h01(i, 3, 651) + 4, w = 1.2f + 1.6f * h01(i, 4, 651);
        for (int y = 0; y < (int)len; y++) {
            float hw = w * (1.0f - y / len);
            for (int dx = (int)(-hw - 1); dx <= (int)(hw + 1); dx++) {
                float nx = (dx + 0.5f) / (hw + 0.01f);
                if (fabsf(nx) > 1.0f) continue;
                cv_px((int)x + dx, y, sh(R_GHOST, 0.35f + 0.55f * lit(nx, 0, sqrtf(1 - nx * nx)) - 0.15f * y / len));
            }
        }
        cv_px((int)x, (int)len + 2, sh(R_GHOST, 0.8f));             /* a drop about to fall */
    }
    for (int x = 0; x < TS; x++) for (int y = 0; y < 2; y++) cv_px(x, y, sh(R_GHOST, 0.5f + 0.3f * h01(x, y, 653)));   /* rime along the stone */
    cv_store(SP_ICICLES);
}

/* a bell hung from a beam: green with age, its clapper still */
static void spr_bell(void) {
    cv_begin(cv_sprite, TS, TS);
    cv_capsule(6, 3, 58, 3, 2.6f, R_WOOD, 0.15f, 0.6f);            /* the beam */
    cv_capsule(32, 5, 32, 10, 1.4f, R_IRON, 0.3f, 0.8f);            /* the yoke */
    for (int y = 10; y < 38; y++) {
        float t = (y - 10) / 28.0f, hw = 6.0f + 4.0f * t + 7.0f * powf(t, 4.0f);
        for (int x = (int)(32 - hw); x <= (int)(32 + hw); x++) {
            float nx = (x + 0.5f - 32) / hw;
            if (fabsf(nx) > 1.0f) continue;
            float v = 0.2f + 0.7f * lit(nx, -0.2f, sqrtf(1 - nx * nx)) + (y == 33 || y == 34 ? 0.1f : 0.0f);
            uint8_t c = sh(R_BRICK, v);
            if (fbm(x / 3.0f, y / 5.0f, 32, 661) > 0.62f + t * 0.1f) c = sh(R_MOSS, 0.35f + 0.3f * v);   /* verdigris */
            cv_px(x, y, c);
        }
    }
    cv_flat(32, 37.5f, 14, 1.3f, sh(R_BRICK, 0.08f));               /* the mouth */
    cv_capsule(32, 34, 32, 40, 1.2f, R_IRON, 0.2f, 0.6f);           /* the clapper */
    cv_ellipse(32, 41, 2.0f, 2.0f, R_IRON, 0.2f, 0.7f);
    cv_outline(0);
    cv_store(SP_BELL);
}

/* a carcass picked nearly clean: a cage of ribs with dark meat still on it */
static void spr_carcass(void) {
    cv_begin(cv_sprite, TS, TS);
    cv_flat(32, 62, 26, 2.5f, R_WARM + 1);
    cv_ellipse(32, 58, 20, 4.0f, R_BLOOD, 0.05f, 0.2f);            /* what has soaked into the floor */
    cv_capsule(12, 54, 50, 52, 1.5f, R_BONE, 0.3f, 0.9f);           /* the spine */
    for (int i = 0; i < 7; i++) {                                   /* ribs arching up */
        float x = 18 + i * 4.5f, h = 10 - fabsf(i - 3.0f) * 1.5f;
        for (int k = 0; k <= 10; k++) {
            float t = k / 10.0f, xx = x + sinf(t * PI_F) * 3.0f, yy = 53 - sinf(t * PI_F * 0.95f) * h + t * 6.0f;
            cv_px((int)xx, (int)yy, sh(R_BONE, 0.85f - 0.3f * t));
            if (h01(i, k, 671) > 0.75f) cv_px((int)xx + 1, (int)yy, sh(R_BLOOD, 0.2f + 0.1f * h01(i, k, 673)));   /* shreds of meat */
        }
    }
    cv_ellipse(52, 52, 5, 4, R_BONE, 0.3f, 0.95f);                  /* the skull, a beast's */
    cv_capsule(52, 54, 59, 57, 1.8f, R_BONE, 0.25f, 0.85f);
    cv_flat(51, 51, 1.3f, 1.3f, 0);
    cv_capsule(16, 56, 6, 61, 1.3f, R_BONE, 0.25f, 0.8f); cv_capsule(48, 57, 40, 61, 1.2f, R_BONE, 0.25f, 0.8f);   /* legs, gnawed */
    cv_outline(0);
    cv_store(SP_CARCASS);
}

/* plate on a wooden stand, as the knights left it; and a great helm on the floor */
static void spr_armourstand(void) {
    cv_begin(cv_sprite, TS, TS);
    cv_flat(32, 62, 12, 1.6f, R_WARM + 1);
    cv_capsule(32, 10, 32, 61, 1.3f, R_WOOD, 0.15f, 0.6f);
    cv_capsule(24, 61, 40, 61, 1.3f, R_WOOD, 0.15f, 0.55f);
    cv_capsule(20, 21, 44, 21, 1.4f, R_WOOD, 0.15f, 0.6f);          /* the shoulders */
    for (int y = 20; y <= 40; y++) {                                /* the breastplate */
        float hw = 9.0f + 1.5f * sinf((y - 20) / 20.0f * PI_F) - (y > 34 ? (y - 34) * 0.7f : 0.0f);
        for (int x = (int)(32 - hw); x <= (int)(32 + hw); x++) {
            float nx = (x + 0.5f - 32) / hw;
            cv_px(x, y, sh(R_IRON, 0.25f + 0.6f * lit(nx, (y - 28) / 14.0f, sqrtf(fmaxf(0, 1 - nx * nx))) + (fabsf(x + 0.5f - 32) < 0.8f ? 0.2f : 0.0f) - 0.25f * (h01(x / 3, y / 3, 681) > 0.9f)));
        }
    }
    cv_ellipse(22, 21, 5, 4, R_IRON, 0.3f, 1.0f); cv_ellipse(42, 21, 5, 4, R_IRON, 0.3f, 1.0f);
    for (int y = 3; y <= 16; y++)                                   /* the helm on top */
        for (int x = 25; x <= 39; x++) {
            float nx = (x + 0.5f - 32) / 7.0f, top = 3 + 3.0f * nx * nx;
            if (fabsf(nx) > 1.0f || y < top) continue;
            float v = 0.25f + 0.6f * lit(nx, (y - 9) / 9.0f, sqrtf(fmaxf(0, 1 - nx * nx)));
            if (y == 9 || (y == 10 && fabsf(nx) < 0.85f)) v = 0.0f;
            cv_px(x, y, sh(R_IRON, v));
        }
    for (int i = 0; i < 6; i++) cv_line(26 + i * 2.4f, 1 + (i & 1), 27 + i * 2.4f, 4, sh(R_STONE, 0.75f));   /* cobweb strands from it */
    cv_outline(0);
    cv_store(SP_ARMOURSTAND);
}
static void spr_helm(void) {
    cv_begin(cv_sprite, TS, TS);
    cv_flat(32, 62, 12, 1.6f, R_WARM + 1);
    for (int y = 46; y <= 62; y++)
        for (int x = 23; x <= 41; x++) {
            float nx = (x + 0.5f - 32) / 8.5f, ny = (y + 0.5f - 54) / 8.0f;
            if (nx * nx + ny * ny > 1.0f) continue;
            float v = 0.25f + 0.65f * lit(nx, ny, sqrtf(fmaxf(0, 1 - nx * nx - ny * ny)));
            if (fabsf(x + 0.5f - 29.0f) < 0.9f && ny > -0.7f && ny < 0.5f) v = 0.0f;   /* the slit, on its side */
            if (h01(x / 2, y / 2, 683) > 0.88f) v -= 0.3f;           /* a dent */
            cv_px(x, y, sh(R_IRON, v));
        }
    cv_outline(0);
    cv_store(SP_HELM);
}

/* a mine prop: a post, a cap-beam across the whole hall, a brace, and the rock it holds up */
static void spr_timber(void) {
    cv_begin(cv_sprite, TS, TS);
    for (int x = 0; x < TS; x++) for (int y = 0; y < 3; y++) cv_px(x, y, sh(R_STONE, 0.15f + 0.25f * h01(x, y, 691)));
    cv_capsule(-2, 6, 66, 6, 3.2f, R_WOOD, 0.12f, 0.55f);          /* the cap */
    cv_capsule(32, 9, 32, 62, 3.0f, R_WOOD, 0.12f, 0.6f);           /* the post */
    cv_capsule(32, 22, 50, 8, 1.8f, R_WOOD, 0.12f, 0.5f);           /* a brace */
    for (int i = 0; i < 6; i++) cv_line(30, 14 + i * 8, 34, 15 + i * 8, sh(R_WOOD, 0.05f));   /* grain and splits */
    cv_ellipse(32, 6, 1.0f, 1.0f, R_IRON, 0.3f, 0.8f);              /* a spike in the joint */
    cv_outline(0);
    cv_store(SP_TIMBER);
}

/* a miner's lamp hung from a nail on a short post */
static void spr_minelamp(int id, int frame) {
    cv_begin(cv_sprite, TS, TS);
    cv_flat(32, 62, 8, 1.4f, R_WARM + 1);
    cv_capsule(32, 18, 32, 62, 2.2f, R_WOOD, 0.12f, 0.55f);
    cv_capsule(32, 22, 38, 22, 0.8f, R_IRON, 0.3f, 0.8f);           /* the nail */
    cv_line(38, 22, 38, 26, sh(R_IRON, 0.5f));                     /* the hook */
    cv_ellipse(38, 27, 3.0f, 1.4f, R_IRON, 0.25f, 0.8f);
    for (int y = 28; y < 36; y++)                                   /* the glass */
        for (int x = 35; x <= 41; x++) {
            float nx = (x + 0.5f - 38) / 3.5f;
            if (fabsf(nx) > 1.0f) continue;
            cv_px(x, y, fabsf(nx) > 0.75f ? sh(R_IRON, 0.4f) : (uint8_t)(R_FLAME + clampi((int)((1.0f - fabsf(nx) * 0.5f - fabsf(y - 32.0f) / 9.0f) * 13.0f) + frame, 3, 14)));
        }
    flame(38.5f, 33, 4.0f + frame, 1.2f, frame ? 0.4f : -0.4f);
    cv_ellipse(38, 36, 3.6f, 1.4f, R_IRON, 0.2f, 0.7f);
    cv_store(id);
}

/* a mine cart, heaped with ore: copper-green and iron-red in the broken rock */
static void spr_cart(void) {
    cv_begin(cv_sprite, TS, TS);
    cv_flat(32, 62, 26, 2.0f, R_WARM + 1);
    for (int i = 0; i < 16; i++) {                                  /* the ore, heaped over the rim */
        float x = 14 + 36 * h01(i, 1, 701), y = 34 - 6 * sinf((x - 14) / 36.0f * PI_F) + 4 * h01(i, 2, 701);
        int ramp = i % 5 == 0 ? R_MOSS : (i % 5 == 1 ? R_BRICK : R_STONE);
        cv_ellipse(x, y, 3.0f + 2.0f * h01(i, 3, 701), 2.4f + 1.4f * h01(i, 4, 701), ramp, 0.15f, ramp == R_STONE ? 0.7f : 0.8f);
    }
    for (int y = 34; y <= 52; y++)                                  /* the box: planks bound with iron */
        for (int x = 11; x <= 53; x++) {
            float lean = (y - 34) * 0.18f;
            if (x < 11 + lean || x > 53 - lean) continue;
            int band = x < 14 + lean || x > 50 - lean || y == 34 || y == 35 || y == 51 || y == 52 || (y > 41 && y < 44);
            float v = band ? 0.3f + 0.35f * (y < 36) : 0.12f + 0.2f * h01(x / 6, 1, 703) + 0.08f * (1.0f - (y - 34) / 18.0f);
            cv_px(x, y, band ? sh(R_IRON, v) : sh(R_WOOD, v - ((y - 34) % 6 == 0 ? 0.08f : 0.0f)));
        }
    for (int s = -1; s <= 1; s += 2) {                              /* wheels */
        cv_ellipse(32 + s * 13, 56, 6.0f, 6.0f, R_IRON, 0.15f, 0.7f);
        cv_ellipse(32 + s * 13, 56, 2.2f, 2.2f, R_IRON, 0.3f, 0.9f);
    }
    cv_outline(0);
    cv_store(SP_CART);
}

/* pale fungus in a cluster */
static void spr_fungus(void) {
    cv_begin(cv_sprite, TS, TS);
    cv_flat(32, 62, 18, 1.8f, R_WARM + 1);
    static const float M[7][4] = { {22, 9, 5.5f, 0.9f}, {30, 15, 7.0f, 1.0f}, {39, 10, 5.0f, 0.85f}, {45, 6, 3.5f, 0.8f}, {17, 5, 3.0f, 0.75f}, {34, 5, 3.0f, 0.8f}, {27, 7, 3.5f, 0.85f} };
    for (int i = 0; i < 7; i++) {
        float x = M[i][0], h = M[i][1], r = M[i][2];
        cv_capsule(x, 62, x + (i & 1 ? 0.8f : -0.8f), 62 - h, r * 0.22f + 0.6f, R_BONE, 0.3f, M[i][3]);
        cv_ellipse(x + (i & 1 ? 0.8f : -0.8f), 62 - h, r, r * 0.5f, R_BONE, 0.3f, M[i][3]);
        for (int k = -1; k <= 1; k++) cv_px((int)(x + k * r * 0.5f), (int)(62 - h + r * 0.35f), sh(R_WARM, 0.25f));   /* gills */
        for (int k = 0; k < 3; k++) cv_px((int)(x - r * 0.4f + k * r * 0.4f), (int)(62 - h - r * 0.2f), sh(R_WARM, 0.45f));   /* spots */
    }
    cv_outline(0);
    cv_store(SP_FUNGUS);
}

/* a cluster of crystals glowing by themselves, a little brighter on the second frame */
static void spr_crystal(int id, int frame) {
    cv_begin(cv_sprite, TS, TS);
    cv_flat(32, 62, 20, 2.0f, R_WARM + 1);
    for (int i = 0; i < 7; i++) {
        float bx = 18 + 28 * h01(i, 1, 711), a = (h01(i, 2, 711) - 0.5f) * 0.9f, len = 14 + 30 * h01(i, 3, 711) * (i == 3 ? 1.4f : 1.0f), w = 2.5f + 2.5f * h01(i, 4, 711);
        float dx = sinf(a), dy = -cosf(a);
        for (float t = 0; t < len; t += 0.5f) {
            float hw = t > len - w * 1.6f ? w * (len - t) / (w * 1.6f) : w;
            for (float o = -hw; o <= hw; o += 0.5f) {
                float u = o / (w + 0.01f);
                int glow = clampi((int)(4 + 8 * (1.0f - fabsf(u)) * (0.4f + 0.6f * t / len) + (u < -0.2f ? 2 : 0)) + frame, 2, 15);
                cv_px((int)(bx + dx * t - dy * o), (int)(62 + dy * t + dx * o), (uint8_t)(R_GLOW + glow));
            }
        }
    }
    for (int i = 0; i < 5; i++) cv_ellipse(16 + 32 * h01(i, 5, 713), 61, 3.0f, 2.0f, R_IRON, 0.1f, 0.4f);   /* the rock they grow out of */
    cv_store(id);
}

/* a vent of molten rock: a crust of black stone split over the fire, a pool of it bubbling on top */
static void spr_magma(int id, int frame) {
    cv_begin(cv_sprite, TS, TS);
    for (int i = 0; i < 3; i++) {
        float x = 26 + i * 6 + (h01(i, frame, 721) - 0.5f) * 3;
        flame(x, 42, 8 + 8 * h01(i, frame + 3, 721), 3.0f, (h01(i, frame + 6, 721) - 0.5f) * 3.0f);
    }
    for (int y = 40; y < 63; y++)                                   /* the cone of crust */
        for (int x = 6; x <= 58; x++) {
            float hw = 9.0f + (y - 40) * 1.1f, nx = (x + 0.5f - 32) / hw;
            if (fabsf(nx) > 1.0f) continue;
            float n = fbm(x / 4.0f, y / 4.0f, 16, 723);
            uint8_t c = sh(R_IRON, 0.08f + 0.3f * lit(nx, -0.3f, sqrtf(1 - nx * nx)) + 0.1f * n);
            float crack = fabsf(fbm(x / 6.0f, y / 3.0f, 16, 725 + frame) - 0.5f);
            if (crack < 0.035f + 0.01f * frame) c = (uint8_t)(R_FLAME + clampi((int)(12 - crack * 150), 4, 14));
            cv_px(x, y, c);
        }
    cv_flat(32, 41, 9, 2.2f, (uint8_t)(R_FLAME + 10));              /* the pool at the top */
    for (int i = 0; i < 5; i++) cv_px((int)(25 + 14 * h01(i, frame, 727)), 40 + (int)(2 * h01(i, frame + 1, 727)), (uint8_t)(R_FLAME + 15));
    cv_store(id);
}

/* shards of black glass, standing up from the floor, edges catching the light */
static void spr_obsidian(void) {
    cv_begin(cv_sprite, TS, TS);
    cv_flat(32, 62, 18, 2.0f, R_WARM + 1);
    for (int i = 0; i < 6; i++) {
        float bx = 20 + 24 * h01(i, 1, 731), top = 22 + 30 * h01(i, 2, 731), w = 3 + 4 * h01(i, 3, 731), lean = (h01(i, 4, 731) - 0.5f) * 10;
        float p[6] = { bx - w, 62, bx + w, 62, bx + lean, top };
        cv_poly(p, 3, R_IRON, 0.06f, 0.32f, top, 62 - top);
        cv_line(bx + w * 0.3f, 61, bx + lean, top, sh(R_GHOST, 0.7f));   /* the glassy edges */
        cv_line(bx - w * 0.7f, 61, bx + lean, top, sh(R_PURPLE, 0.35f));
        cv_px((int)(bx + lean), (int)top, sh(R_GHOST, 0.95f));
    }
    cv_outline(0);
    cv_store(SP_OBSIDIAN);
}

/* an altar: a block of stone under a dark cloth, a skull, black candles burning */
static void spr_altar(int id, int frame) {
    cv_begin(cv_sprite, TS, TS);
    cv_flat(32, 62, 24, 2.0f, R_WARM + 1);
    for (int y = 40; y < 63; y++)
        for (int x = 9; x <= 55; x++) stone_px(x, y, (x + 0.5f - 32) / 23.0f, y < 42 ? 0.15f : 0.0f, 741);
    for (int y = 39; y < 54; y++)                                   /* the cloth over it, hanging to a fringe */
        for (int x = 14; x <= 50; x++) {
            if (y > 50 && (x & 1)) continue;
            cv_px(x, y, sh(R_BLOOD, 0.08f + 0.12f * (y < 41) + 0.04f * sinf(x * 0.9f)));
        }
    cv_skull(32, 34, 0.8f, 0.3f, 0.95f);
    static const float C[4][2] = { {16, 10}, {22, 14}, {42, 12}, {48, 8} };
    for (int i = 0; i < 4; i++) candle(C[i][0], 39, C[i][1], frame, i, R_IRON);
    cv_outline(0);
    cv_store(id);
}

/* ------------------------------------------------------------------ */
/* Side views: each creature walking in profile, facing right. The     */
/* bestiary mirrors them for left.                                     */
/* ------------------------------------------------------------------ */

/* a curved line through three points, for ribs seen from the side */
static void cv_curve(float x0, float y0, float cx, float cy, float x1, float y1, uint8_t c) {
    for (int i = 0; i <= 16; i++) {
        float t = i / 16.0f, u = 1.0f - t;
        cv_px((int)(u * u * x0 + 2 * u * t * cx + t * t * x1), (int)(u * u * y0 + 2 * u * t * cy + t * t * y1), c);
    }
}

/* a skull in profile, looking right: crown, brow, the long face, a hinged jaw */
static void skull_side(float x, float y, float s, float jaw, uint8_t eye) {
    cv_ellipse(x - 1.0f * s, y, 6.0f * s, 5.6f * s, R_BONE, 0.3f, 1.0f);
    cv_capsule(x + 1.5f * s, y + 2.5f * s, x + 5.5f * s, y + 3.5f * s, 2.2f * s, R_BONE, 0.3f, 0.9f);   /* the upper jaw */
    cv_capsule(x - 1.0f * s, y + 5.0f * s, x + 5.0f * s, y + 5.5f * s + jaw, 1.4f * s, R_BONE, 0.25f, 0.8f);   /* the lower */
    cv_flat(x + 2.6f * s, y - 0.2f * s, 1.8f * s, 1.9f * s, 0);    /* the socket */
    if (eye) cv_px((int)(x + 2.8f * s), (int)y, eye);
    cv_flat(x + 5.6f * s, y + 1.4f * s, 0.7f * s, 0.9f * s, 0);    /* the nose */
    for (int i = 0; i < 4; i++) cv_px((int)(x + (2.5f + i * 1.0f) * s), (int)(y + 4.4f * s), sh(R_BONE, 0.95f));
    cv_flat(x - 3.0f * s, y + 2.5f * s, 1.0f * s, 0.8f * s, sh(R_BONE, 0.3f));   /* the ear hole */
}

/* a skeleton walking right. w: the stride, +1 or -1; arm: 0 swinging, 1 a sword up and a shield on */
static void skel_side(float w, int arm) {
    float bob = fabsf(w) * 0.6f;
    w *= 1.5f;                                                     /* a longer stride than the front view needs */
    cv_flat(32, 62, 14, 1.8f, R_WARM + 1);
    /* the far leg and arm first, darker */
    cv_capsule(31, 41, 31 - 4 * w + 1, 51, 1.4f, R_BONE, 0.12f, 0.55f); cv_capsule(31 - 4 * w + 1, 51, 31 - 5 * w, 60, 1.2f, R_BONE, 0.12f, 0.5f);
    cv_capsule(31 - 5 * w, 61, 31 - 5 * w + 5, 61.5f, 1.0f, R_BONE, 0.12f, 0.5f);
    if (!arm) { cv_capsule(31, 21 + bob, 31 + 4 * w, 29, 1.1f, R_BONE, 0.12f, 0.5f); cv_capsule(31 + 4 * w, 29, 32 + 6 * w, 36, 1.0f, R_BONE, 0.12f, 0.5f); }
    /* spine, ribs, sternum, pelvis */
    cv_capsule(31, 17 + bob, 29.5f, 27 + bob, 1.3f, R_BONE, 0.3f, 0.9f); cv_capsule(29.5f, 27 + bob, 31, 38, 1.3f, R_BONE, 0.3f, 0.9f);
    for (int i = 0; i < 5; i++) {
        float y0 = 20.5f + i * 3.3f + bob;
        cv_curve(30, y0, 37.5f - i * 0.3f, y0 - 1.0f, 36.5f - i * 0.6f, y0 + 4.0f, sh(R_BONE, 0.85f - i * 0.06f));
        cv_curve(30, y0 + 1, 37.0f - i * 0.3f, y0, 36.0f - i * 0.6f, y0 + 4.8f, sh(R_BONE, 0.45f));
    }
    cv_line(37.5f, 21 + bob, 36.0f, 33 + bob, sh(R_BONE, 0.95f));
    cv_ellipse(31, 40, 4.2f, 3.3f, R_BONE, 0.3f, 0.9f); cv_flat(32, 40.5f, 1.4f, 1.2f, 0);
    /* the near leg, striding */
    cv_bone(31, 41, 31 + 4 * w, 51, 1.5f, 0.3f, 0.95f); cv_bone(31 + 4 * w, 51, 31 + 5 * w - 1, 60, 1.3f, 0.3f, 0.9f);
    cv_capsule(31 + 5 * w - 1, 61, 31 + 5 * w + 4, 61.5f, 1.1f, R_BONE, 0.3f, 0.9f);
    if (!arm) { cv_bone(32, 21 + bob, 32 - 4 * w, 29, 1.2f, 0.3f, 0.95f); cv_capsule(32 - 4 * w, 29, 31 - 6 * w, 36, 1.1f, R_BONE, 0.3f, 0.9f); }
    else {                                                         /* sword arm up and forward */
        cv_bone(32, 21 + bob, 36, 28, 1.2f, 0.3f, 0.95f); cv_capsule(36, 28, 39, 22, 1.1f, R_BONE, 0.3f, 0.9f);
        cv_capsule(39, 22, 47, 5, 1.0f, R_IRON, 0.5f, 1.0f);       /* the blade */
        cv_capsule(36, 23, 42, 21, 0.9f, R_WOOD, 0.3f, 0.6f);      /* the guard */
    }
    cv_capsule(31.5f, 14 + bob, 31, 17 + bob, 1.1f, R_BONE, 0.3f, 0.9f);
    skull_side(33, 9 + bob, 1.0f, 0.0f, g_decay > 0.55f ? (uint8_t)(R_GLOW + 13) : (uint8_t)(R_FLAME + 10));
    if (arm) {                                                     /* the shield, held out in front, seen at a slant */
        for (int y = 17; y < 48; y++)
            for (int x = 36; x < 46; x++) {
                float nx = (x + 0.5f - 41.0f) / 4.5f, ny = (y + 0.5f - 32.5f) / 15.0f, d = nx * nx + ny * ny;
                if (d > 1.0f) continue;
                uint8_t c = d > 0.72f ? sh(R_IRON, 0.4f + 0.4f * -ny) : sh(R_WOOD, 0.2f + 0.3f * lit(nx, ny, sqrtf(1 - d)) - (((y - 17) % 6) == 0 ? 0.08f : 0.0f));
                if (fabsf(y + 0.5f - 32.5f) < 1.0f && d <= 0.72f) c = sh(R_IRON, 0.5f);
                cv_px(x, y, c);
            }
    }
    cv_bone_decay(901);
    cv_outline(0);
}
static void spr_skel_side(int id, float w, int arm) { cv_begin(cv_sprite, TS, TS); skel_side(w, arm); cv_store(id); }

/* a bat's wing from the shoulder: a membrane between three long fingers, scalloped at the edge */
static void bat_wing(float sx, float sy, float wy, float span, float lo, float hi) {
    float tips[3][2] = { { sx - 6 * span, sy + 16 * wy * span }, { sx - 14 * span, sy + 15 * wy * span }, { sx - 17 * span, sy + 6 * wy * span } };
    float elbow_x = sx - 4 * span, elbow_y = sy + 7 * wy * span;
    for (int f = 0; f < 3; f++) {
        float nx = f < 2 ? tips[f + 1][0] : sx - 9 * span, ny = f < 2 ? tips[f + 1][1] : sy + 1 * wy;
        float mx = (tips[f][0] + nx) * 0.5f + 2.0f * span, my = (tips[f][1] + ny) * 0.5f - 2.0f * wy * span;   /* the scallop between */
        float tri[8] = { elbow_x, elbow_y, tips[f][0], tips[f][1], mx, my, nx, ny };
        cv_poly(tri, 4, R_PURPLE, hi, lo, fminf(elbow_y, fminf(tips[f][1], ny)), 18);
    }
    cv_line(sx, sy, elbow_x, elbow_y, sh(R_IRON, 0.35f));
    for (int f = 0; f < 3; f++) cv_line(elbow_x, elbow_y, tips[f][0], tips[f][1], sh(R_IRON, 0.25f));
}

/* a bat from the side, wings up or down */
static void spr_bat_side(int id, int up) {
    cv_begin(cv_sprite, TS, TS);
    float wy = up ? -1.0f : 1.0f;
    bat_wing(31, 28, wy, 1.25f, 0.03f, 0.15f);                     /* the far wing */
    cv_ellipse(31, 30, 6.0f, 3.6f, R_IRON, 0.05f, 0.35f);         /* the body, level in flight */
    cv_capsule(25, 31, 21, 34, 0.8f, R_IRON, 0.05f, 0.3f);         /* feet trailing */
    cv_ellipse(38, 28.5f, 3.0f, 2.6f, R_IRON, 0.08f, 0.4f);        /* the head, and its snout and ear */
    cv_capsule(40, 29, 42, 29.5f, 1.0f, R_IRON, 0.08f, 0.35f);
    float ear[6] = { 37, 26.5f, 38.5f, 21, 39.5f, 26.5f };
    cv_poly(ear, 3, R_IRON, 0.3f, 0.1f, 21, 6);
    cv_px(39, 28, (uint8_t)(R_FLAME + 6));
    bat_wing(34, 29, wy, 1.5f, 0.06f, 0.3f);                       /* the near one */
    cv_store(id);
}

/* a spider from the side: the abdomen behind, the head in front, legs either side of it */
static void spr_spider_side(int id, int frame) {
    cv_begin(cv_sprite, TS, TS);
    cv_flat(32, 62, 24, 1.8f, R_WARM + 1);
    for (int pass = 0; pass < 2; pass++)                           /* the far legs, then the body, then the near */
        for (int l = 0; l < 4; l++) {
            if (pass == 1) break;
            float k = ((l + frame) & 1) ? 2.5f : -2.5f, ax = 34 + l * 1.5f, fx = 14 + l * 11.0f + k, kx = (ax + fx) * 0.5f + (l < 2 ? -3 : 3);
            cv_capsule(ax, 44, kx, 33 + l * 0.6f, 1.1f, R_IRON, 0.02f, 0.2f);
            cv_capsule(kx, 33 + l * 0.6f, fx, 61, 0.8f, R_IRON, 0.02f, 0.18f);
        }
    cv_ellipse(23, 42, 10.0f, 8.0f, R_IRON, 0.04f, 0.38f);         /* the abdomen, its red mark on top */
    cv_flat(22, 36, 2.6f, 1.6f, (uint8_t)(R_BLOOD + 9));
    cv_ellipse(37, 45, 6.5f, 4.6f, R_IRON, 0.06f, 0.42f);          /* the head */
    for (int i = 0; i < 3; i++) cv_px(41 + i, 43 - (i & 1), (uint8_t)(R_FLAME + (i ? 2 : 4)));
    cv_capsule(42, 47, 43, 51, 0.8f, R_BONE, 0.4f, 0.8f);          /* a fang */
    for (int l = 0; l < 4; l++) {
        float k = ((l + frame + 1) & 1) ? 2.5f : -2.5f, ax = 35 + l * 1.5f, fx = 12 + l * 12.0f + k, kx = (ax + fx) * 0.5f + (l < 2 ? -4 : 4);
        cv_capsule(ax, 46, kx, 31 + l * 0.8f, 1.3f, R_IRON, 0.06f, 0.38f);
        cv_capsule(kx, 31 + l * 0.8f, fx, 61.5f, 0.95f, R_IRON, 0.05f, 0.32f);
    }
    cv_outline(0);
    cv_store(id);
}

/* the spectre from the side: leaning on, its hem streaming back, a hand held out */
static void spr_ghost_side(int id, float ph) {
    cv_begin(cv_sprite, TS, TS);
    for (int y = 2; y < TS; y++)
        for (int x = 0; x < TS; x++) {
            float t = (y - 2) / 60.0f, lean = 8.0f * (1.0f - t);     /* the head forward of the hem */
            float front = 36.0f + lean + (y < 14 ? -(14 - y) * 0.6f : 0.0f), back = 26.0f + lean - t * 18.0f + sinf(y * 0.2f + ph) * 2.0f;
            if (y < 4 && x > front - 3) continue;
            if (x > front || x < back) continue;
            float hem = 52.0f + 4.0f * sinf(x * 0.5f + ph);
            if (y > hem && h01(x, y, 941 + (int)(ph * 3.0f)) > (hem + 8.0f - y) / 8.0f) continue;
            float u = (x - back) / (front - back + 0.01f), v = 0.08f + 0.18f * sinf(u * PI_F) - t * 0.05f;
            if (u > 0.9f) v += 0.25f;                              /* the cold rim along its front */
            cv_px(x, y, sh(R_GHOST, v));
        }
    cv_flat(40, 11, 3.0f, 4.0f, 0);                                 /* the hood's mouth, turned away a little */
    cv_px(41, 10, C_WHITE); cv_px(41, 11, (uint8_t)(R_GLOW + 13));
    cv_capsule(36, 20, 47, 25 + sinf(ph), 3.2f, R_GHOST, 0.15f, 0.5f);   /* the sleeve */
    for (int f = -1; f <= 1; f++) cv_line(48, 25 + sinf(ph), 52, 24 + f * 1.5f + sinf(ph), sh(R_BONE, 0.8f));
    cv_ellipse(48, 25 + sinf(ph), 1.6f, 1.5f, R_BONE, 0.4f, 0.9f);
    cv_store(id);
}

/* the wall skull turned to show its side: a great skull in profile, a coal in its eye */
static void spr_wskull_side(void) {
    cv_begin(cv_sprite, TS, TS);
    skull_side(30, 26, 3.2f, 1.0f, 0);
    for (int y = 0; y < TS; y++)
        for (int x = 0; x < TS; x++) {
            uint8_t *p = &cv_sprite[y * TS + x];
            if (*p >= R_BONE && *p < R_BONE + 16 && fbm(x / 6.0f, y / 6.0f, 16, 71) < 0.3f) *p = (uint8_t)(*p - 2 < R_BONE ? R_BONE : *p - 2);
        }
    cv_flat(38.5f, 26, 4.2f, 4.6f, (uint8_t)(R_FLAME + 8)); cv_flat(38.5f, 26, 2.2f, 2.4f, (uint8_t)(R_FLAME + 13));
    cv_outline(0);
    cv_store(SP_WSKULL_S);
}

/* the shambler from the side: bent forward, arms out ahead of it, one foot dragging */
static void spr_shambler_side(int id, float w) {
    cv_begin(cv_sprite, TS, TS);
    cv_flat(32, 62, 14, 1.8f, R_WARM + 1);
    cv_capsule(31, 42, 31 - 3 * w, 52, 2.6f, R_WOOD, 0.04f, 0.25f);   /* the far leg */
    cv_capsule(31 - 3 * w, 52, 30 - 6 * w, 60.5f, 1.8f, R_WARM, 0.15f, 0.5f);
    cv_capsule(30 - 6 * w, 61.5f, 34 - 6 * w, 61.5f, 1.4f, R_WARM, 0.15f, 0.5f);
    cv_capsule(35, 22, 48, 21, 1.8f, R_WARM, 0.15f, 0.55f);        /* the far arm, out ahead */
    dead_hand(50, 22, 0.5f, 0.3f, R_WARM, 0.9f);
    float tp[8] = { 31, 22, 38, 23, 35, 43, 27, 43 };              /* the body, bent at the hips */
    cv_poly(tp, 4, R_WARM, 0.85f, 0.4f, 22, 21);
    for (int y = 21; y <= 45; y++)                                  /* the shirt, ragged */
        for (int x = 24; x <= 40; x++) {
            uint8_t c = cv_sprite[y * TS + x];
            if (c < R_WARM || c >= R_WARM + 16 || (y > 41 && h01(x, 7, 961) < 0.35f)) continue;
            cv_px(x, y, sh(R_IRON, 0.22f + 0.25f * (x - 24) / 16.0f + 0.08f * sinf(y * 0.8f)));
        }
    cv_capsule(31, 42, 31 + 3 * w, 52, 2.8f, R_WOOD, 0.06f, 0.33f);   /* the near leg */
    cv_capsule(31 + 3 * w, 52, 32 + 5 * w, 60.5f, 2.0f, R_WARM, 0.3f, 0.8f);
    cv_capsule(32 + 5 * w, 61.5f, 37 + 5 * w, 61.5f, 1.5f, R_WARM, 0.3f, 0.8f);
    cv_capsule(37, 21, 39, 16, 2.2f, R_WARM, 0.3f, 0.75f);         /* the neck, craned */
    cv_ellipse(41, 13, 5.2f, 6.0f, R_WARM, 0.35f, 1.0f);            /* the head, hung forward */
    cv_ellipse(44, 17, 3.0f, 2.2f, R_WARM, 0.25f, 0.7f);           /* the slack jaw */
    cv_flat(44, 12, 1.4f, 1.3f, 0); cv_px(44, 12, (uint8_t)(R_FLAME + 4));
    cv_flat(46, 16.5f, 1.2f, 1.0f, 0);
    for (int i = 0; i < 5; i++) cv_line(37 + i, 8, 34 + i * 0.8f, 13 + i * 1.5f, sh(R_IRON, 0.2f));   /* hair, down the back */
    cv_capsule(37, 23, 52, 25, 2.0f, R_WARM, 0.35f, 0.9f);         /* the near arm, reaching */
    dead_hand(54, 26, 0.6f, 0.35f, R_WARM, 1.1f);
    cv_rot(R_WARM, 0.6f, 963);
    cv_outline(0);
    cv_store(id);
}

/* the Drowned from the side: the swollen belly out in front, the head sunk, weed down its back */
static void spr_drowned_side(int id, float w) {
    cv_begin(cv_sprite, TS, TS);
    cv_flat(32, 62, 15, 1.8f, R_WARM + 1);
    cv_capsule(28, 48, 28 - 3 * w, 60, 4.0f, R_STONE, 0.1f, 0.4f); /* the far leg */
    cv_capsule(30, 26, 38, 40, 3.0f, R_STONE, 0.1f, 0.4f);         /* the far arm, hanging */
    cv_ellipse(32, 37, 13.0f, 14.5f, R_STONE, 0.2f, 0.9f);         /* the belly, tight as a drum */
    cv_flat(44, 40, 1.4f, 1.8f, sh(R_STONE, 0.2f));
    cv_rot(R_STONE, 0.3f, 965);
    for (int i = 0; i < 10; i++) cv_px((int)(22 + 20 * h01(i, 3, 967)), (int)(26 + 22 * h01(i, 4, 967)), sh(R_BONE, 0.55f + 0.3f * h01(i, 5, 967)));
    cv_capsule(31, 48, 31 + 3 * w, 60, 4.2f, R_STONE, 0.15f, 0.6f); /* the near leg */
    cv_ellipse(35 + 3 * w, 61.5f, 4.0f, 1.6f, R_STONE, 0.15f, 0.55f);
    cv_ellipse(37, 17, 7.5f, 7.0f, R_STONE, 0.3f, 1.0f);           /* the head, sunk forward */
    cv_flat(41, 16, 1.8f, 1.4f, sh(R_GHOST, 0.9f));                 /* a milky eye */
    cv_flat(43, 21, 2.0f, 1.2f, 0);                                 /* the mouth, and the water out of it */
    for (int y = 22; y < 34; y++) if (h01(y, 1, 969) > 0.3f) cv_px(43 + (y & 1), y, sh(R_GHOST, 0.5f + 0.3f * h01(y, 2, 969)));
    for (int i = 0; i < 6; i++) {                                   /* weed, down the back of its head and shoulders */
        float x = 30 + i * 1.5f, len = 8 + 10 * h01(i, 1, 971);
        for (int y = 0; y < (int)len; y++) cv_px((int)(x - y * 0.3f + sinf(y * 0.5f + i + w) * 0.8f), 11 + y, sh(R_MOSS, 0.1f + 0.22f * (1.0f - y / len)));
    }
    cv_capsule(36, 26, 44, 38, 3.4f, R_STONE, 0.25f, 0.8f);        /* the near arm, heavy */
    dead_hand(45, 40, 1.3f, 0.3f, R_STONE, 1.2f);
    for (int i = 0; i < 6; i++) cv_px((int)(22 + 22 * h01(i, 3, 973)), (int)(42 + 16 * h01(i, (int)(w * 3 + 4), 973)), sh(R_GHOST, 0.8f));   /* drips */
    cv_outline(0);
    cv_store(id);
}

/* the bone hound from the side: running right, the long skull low, the tail up */
static void spr_hound_side(int id, float w) {
    cv_begin(cv_sprite, TS, TS);
    cv_flat(32, 62, 22, 1.8f, R_WARM + 1);
    float bob = w * 0.8f;
    /* far legs, darker */
    cv_capsule(42, 33 + bob, 40 - 4 * w, 46, 1.1f, R_BONE, 0.12f, 0.45f); cv_capsule(40 - 4 * w, 46, 42 - 6 * w, 60, 1.0f, R_BONE, 0.12f, 0.45f);
    cv_capsule(18, 32 - bob, 20 + 4 * w, 45, 1.2f, R_BONE, 0.12f, 0.45f); cv_capsule(20 + 4 * w, 45, 16 + 5 * w, 60, 1.0f, R_BONE, 0.12f, 0.45f);
    cv_capsule(16, 30, 9, 24, 1.0f, R_BONE, 0.3f, 0.8f); cv_capsule(9, 24, 6, 17 + w, 0.9f, R_BONE, 0.3f, 0.8f);   /* the tail */
    for (int i = 0; i < 12; i++) cv_ellipse(17 + i * 2.5f, 30 + bob * 0.5f - sinf(i / 11.0f * PI_F) * 2.0f, 1.5f, 1.3f, R_BONE, 0.35f, 0.95f);   /* the spine */
    for (int i = 0; i < 6; i++) {                                   /* ribs, hanging under it */
        float x = 25 + i * 3.0f, top = 31 + bob * 0.5f - sinf((x - 17) / 28.0f * PI_F) * 2.0f, drop = 11 - fabsf(i - 2.0f) * 1.4f;
        cv_curve(x, top, x + 3.5f, top + drop * 0.6f, x + 1.0f, top + drop, sh(R_BONE, 0.85f));
    }
    cv_ellipse(18, 33, 4.5f, 3.2f, R_BONE, 0.3f, 0.85f);            /* the pelvis */
    /* near legs */
    cv_bone(42, 34 + bob, 42 + 5 * w, 46, 1.2f, 0.3f, 0.9f); cv_capsule(42 + 5 * w, 46, 44 + 7 * w, 60, 1.1f, R_BONE, 0.3f, 0.9f);
    cv_capsule(44 + 7 * w, 61, 48 + 7 * w, 61.5f, 1.0f, R_BONE, 0.3f, 0.85f);
    cv_bone(19, 33 - bob, 17 - 4 * w, 45, 1.3f, 0.3f, 0.95f); cv_capsule(17 - 4 * w, 45, 22 - 5 * w, 52, 1.1f, R_BONE, 0.3f, 0.9f);
    cv_capsule(22 - 5 * w, 52, 19 - 5 * w, 60, 1.0f, R_BONE, 0.3f, 0.9f); cv_capsule(19 - 5 * w, 61, 23 - 5 * w, 61.5f, 1.0f, R_BONE, 0.3f, 0.85f);
    for (int y = 26; y < 35; y++) cv_px(46 + (y & 1), y + (int)bob, sh(R_IRON, 0.35f + 0.3f * (y & 1)));   /* the collar */
    cv_px(48, 27 + (int)bob, sh(R_IRON, 0.9f)); cv_px(48, 31 + (int)bob, sh(R_IRON, 0.9f));
    float hx = 51, hy = 28 + bob;                                   /* the skull, long in the snout */
    cv_ellipse(hx, hy, 5.0f, 4.4f, R_BONE, 0.3f, 1.0f);
    cv_capsule(hx + 2, hy + 1, hx + 10, hy + 3, 2.3f, R_BONE, 0.3f, 0.95f);
    cv_capsule(hx, hy + 4, hx + 9, hy + 6, 1.4f, R_BONE, 0.25f, 0.8f);
    for (int i = 0; i < 4; i++) cv_px((int)(hx + 4 + i * 1.6f), (int)(hy + 4.5f), sh(R_BONE, 1.0f));
    cv_flat(hx + 1.5f, hy - 1.0f, 1.8f, 1.6f, 0); cv_px((int)(hx + 1.5f), (int)(hy - 1), (uint8_t)(R_FLAME + 11));
    cv_bone_decay(975);
    cv_outline(0);
    cv_store(id);
}

/* the Wraith from the side: leaning on, its hood ahead of it, the robe streaming back, one long hand
 * held out low */
static void spr_wraith_side(int id, float ph) {
    cv_begin(cv_sprite, TS, TS);
    for (int y = 4; y < TS; y++)
        for (int x = 0; x < TS; x++) {
            float t = (y - 4) / 59.0f, lean = 7.0f * (1.0f - t);
            float front = 40.0f + lean - (y < 16 ? (16 - y) * 0.7f : 0.0f) + sinf(y * 0.25f + ph) * 0.6f;
            float back = 26.0f + lean - t * 20.0f + sinf(y * 0.18f + ph) * 2.0f;
            if (x > front || x < back) continue;
            float hem = 53.0f + 5.0f * sinf(x * 0.45f + ph) + 3.0f * sinf(x * 1.3f - ph);
            if (y > hem && h01(x / 2, y, 977 + (int)(ph * 3)) > (63.0f - y) / (63.0f - hem + 0.01f) * 0.9f) continue;
            float u = (x - back) / (front - back + 0.01f), v = 0.03f + 0.12f * sinf(u * PI_F) - t * 0.04f;
            if (u > 0.9f || u < 0.06f) { cv_px(x, y, sh(R_GHOST, 0.28f + 0.3f * (u > 0.9f))); continue; }   /* the cold rim */
            cv_px(x, y, sh(R_IRON, v));
        }
    for (int y = 0; y < 18; y++)                                    /* the hood, peaked forward */
        for (int x = 30; x < 50; x++) {
            float top = 3.0f + (x - 44) * (x - 44) * 0.08f;
            if (y < top || x > 47 - (y > 10 ? (y - 10) * 0.6f : 0) || x < 33 + y * 0.1f) continue;
            cv_px(x, y, x > 45 || y < top + 1.0f ? sh(R_GHOST, 0.35f) : sh(R_IRON, 0.08f + 0.06f * (x - 33) / 14.0f));
        }
    cv_flat(44, 11, 2.6f, 4.0f, 0);                                  /* the hood's mouth, and one star of an eye */
    cv_px(45, 10, C_WHITE); cv_px(45, 11, (uint8_t)(R_GLOW + 13)); cv_px(44, 10, (uint8_t)(R_GLOW + 10));
    cv_capsule(38, 18, 46, 32, 3.8f, R_IRON, 0.04f, 0.2f);          /* the sleeve */
    cv_line(41, 17, 49, 31, sh(R_GHOST, 0.4f));
    cv_ellipse(48, 35, 2.0f, 1.8f, R_BONE, 0.15f, 0.5f);            /* the hand, and fingers too long for it */
    for (int f = 0; f < 4; f++) {
        float a = 0.6f + f * 0.25f, l = 8.0f + (f == 1) * 2.0f;
        float mx = 48 + cosf(a) * l * 0.55f, my = 35 + sinf(a) * l * 0.55f;
        cv_line(48, 35, mx, my, sh(R_BONE, 0.45f)); cv_line(mx, my, 48 + cosf(a + 0.3f) * l, 35 + sinf(a + 0.3f) * l, sh(R_BONE, 0.35f));
    }
    for (int i = 0; i < 30; i++) {                                  /* frost falling off the hem */
        float x = 8 + 40 * h01(i, 1, 979), y = 50 + 13 * h01(i, (int)(ph * 2) + 2, 979);
        if (cv_sprite[(int)y * TS + (int)x] == C_CLEAR) cv_px((int)x, (int)y, sh(R_GHOST, 0.45f + 0.4f * h01(i, 3, 979)));
    }
    cv_store(id);
}

/* a spider from the side, in a body ramp: legs banded or not, the abdomen as big as it is, eggs in it */
static void spider_side(int frame, int ramp, int band, float ab, int eggs, int big_eyes) {
    cv_flat(32, 62, 24, 1.8f, R_WARM + 1);
    for (int l = 0; l < 4; l++) {                                   /* far legs */
        float k = ((l + frame) & 1) ? 2.5f : -2.5f, ax = 34 + l * 1.5f, fx = 14 + l * 11.0f + k, kx = (ax + fx) * 0.5f + (l < 2 ? -3 : 3);
        cv_capsule(ax, 44, kx, 33 + l * 0.6f, 1.1f, ramp, 0.02f, 0.2f);
        cv_capsule(kx, 33 + l * 0.6f, fx, 61, 0.8f, ramp, 0.02f, 0.18f);
    }
    cv_ellipse(25 - (ab - 1.0f) * 6, 42 - (ab - 1.0f) * 6, 10.0f * ab, 8.0f * ab, ramp, 0.04f, 0.42f);   /* the abdomen */
    if (eggs) for (int i = 0; i < 10; i++) {
        float a = h01(i, 1, 981) * TAU_F, r = 8.0f * ab * sqrtf(h01(i, 2, 981));
        cv_ellipse(25 - (ab - 1.0f) * 6 + cosf(a) * r, 42 - (ab - 1.0f) * 6 + sinf(a) * r * 0.75f, 2.0f, 1.7f, R_BONE, 0.15f, 0.45f);
    }
    if (band >= 0) for (int i = 0; i < 3; i++) cv_line(18 + i * 5, 35, 20 + i * 5, 49, sh(band, 0.12f));   /* chevrons */
    else cv_flat(22, 36, 2.6f, 1.6f, (uint8_t)(R_BLOOD + 9));
    cv_ellipse(37, 45, 6.5f, 4.6f, ramp, 0.06f, 0.45f);            /* the head */
    if (big_eyes) { cv_flat(42, 43, 1.6f, 1.6f, 0); cv_px(42, 43, (uint8_t)(R_FLAME + 14)); }
    else for (int i = 0; i < 3; i++) cv_px(41 + i, 43 - (i & 1), (uint8_t)(R_FLAME + (i ? 2 : 4)));
    cv_capsule(42, 47, 43, 51, 0.8f, eggs ? R_BONE : R_IRON, 0.3f, 0.7f);   /* a fang */
    for (int l = 0; l < 4; l++) {                                   /* near legs */
        float k = ((l + frame + 1) & 1) ? 2.5f : -2.5f, ax = 35 + l * 1.5f, fx = 12 + l * 12.0f + k, kx = (ax + fx) * 0.5f + (l < 2 ? -4 : 4);
        cv_capsule(ax, 46, kx, 31 + l * 0.8f, 1.3f, ramp, 0.06f, 0.42f);
        cv_capsule(kx, 31 + l * 0.8f, fx, 61.5f, 0.95f, ramp, 0.05f, 0.36f);
        if (band >= 0) cv_ellipse(kx, 31 + l * 0.8f, 1.5f, 1.5f, band, 0.02f, 0.2f);
    }
    cv_outline(0);
}
static void spr_hunter_side(int id, int frame) { cv_begin(cv_sprite, TS, TS); spider_side(frame, R_WOOD, R_IRON, 1.0f, 0, 1); cv_store(id); }
static void spr_brood_side(int id, int frame) { cv_begin(cv_sprite, TS, TS); spider_side(frame, R_IRON, -1, 1.5f, 1, 0); cv_store(id); }

/* the ghoul from the side: on all fours, the back arched high, the head low and forward */
static void spr_ghoul_side(int id, float w) {
    cv_begin(cv_sprite, TS, TS);
    cv_flat(32, 62, 22, 1.8f, R_WARM + 1);
    cv_capsule(20, 38, 22 - 3 * w, 50, 2.4f, R_WARM, 0.15f, 0.45f); cv_capsule(22 - 3 * w, 50, 18 - 3 * w, 61, 1.8f, R_WARM, 0.15f, 0.45f);   /* far hind leg */
    cv_capsule(40, 36, 44 - 3 * w, 48, 2.0f, R_WARM, 0.15f, 0.45f); cv_capsule(44 - 3 * w, 48, 45 - 4 * w, 61, 1.6f, R_WARM, 0.15f, 0.45f);   /* far arm */
    float back[10] = { 16, 40, 22, 28, 32, 25, 42, 30, 40, 42 };   /* the body, arched */
    cv_poly(back, 5, R_WARM, 0.85f, 0.35f, 25, 17);
    for (int i = 0; i < 5; i++) cv_ellipse(22 + i * 4.5f, 28 - sinf((i + 0.5f) / 5.0f * PI_F) * 3.0f, 1.6f, 1.3f, R_WARM, 0.5f, 1.0f);   /* spine */
    for (int i = 0; i < 4; i++) cv_line(26 + i * 4, 32, 25 + i * 4, 39, sh(R_WARM, 0.25f));   /* ribs */
    cv_capsule(20, 40, 24 + 3 * w, 50, 2.6f, R_WARM, 0.3f, 0.8f); cv_capsule(24 + 3 * w, 50, 20 + 3 * w, 61, 2.0f, R_WARM, 0.3f, 0.8f);   /* near hind leg */
    cv_capsule(41, 36, 47 + 3 * w, 47, 2.2f, R_WARM, 0.3f, 0.85f); cv_capsule(47 + 3 * w, 47, 48 + 5 * w, 61, 1.8f, R_WARM, 0.3f, 0.85f);   /* near arm */
    for (int c = -1; c <= 1; c++) cv_line(48 + 5 * w, 61, 51 + 5 * w + c, 62, sh(R_BONE, 0.7f));
    float ear[6] = { 45, 33, 39, 26, 43, 35 };                       /* an ear laid back */
    cv_poly(ear, 3, R_WARM, 0.4f, 0.9f, 26, 9);
    cv_ellipse(48, 37, 5.5f, 4.8f, R_WARM, 0.35f, 1.0f);            /* the head, low */
    cv_capsule(50, 39, 55, 41, 2.2f, R_WARM, 0.25f, 0.8f);         /* the muzzle */
    cv_flat(54, 42, 2.0f, 1.0f, 0);
    for (int i = 0; i < 3; i++) cv_px(51 + i * 2, 42, sh(R_BONE, 0.9f));
    cv_flat(50, 36, 1.4f, 1.2f, 0); cv_px(50, 36, (uint8_t)(R_FLAME + 9));
    cv_rot(R_WARM, 0.3f, 983);
    cv_outline(0);
    cv_store(id);
}

/* the knight from the side: skeleton in plate, the greatsword held upright before it */
static void spr_knight_side(int id, float w) {
    cv_begin(cv_sprite, TS, TS);
    skel_side(w, 0);
    float sw = w * 1.5f;
    cv_capsule(31 - 4 * sw + 1, 52, 31 - 5 * sw, 59, 2.4f, R_IRON, 0.12f, 0.5f);   /* far greave */
    cv_capsule(31, 42, 31 + 4 * sw, 50, 2.8f, R_IRON, 0.25f, 0.95f);   /* near cuisse and greave */
    cv_capsule(31 + 4 * sw, 52, 31 + 5 * sw - 1, 59, 2.4f, R_IRON, 0.25f, 0.95f);
    cv_ellipse(31 + 4 * sw, 51, 2.4f, 2.0f, R_IRON, 0.4f, 1.0f);
    cv_ellipse(31 + 5 * sw + 1, 61.5f, 3.6f, 1.5f, R_IRON, 0.3f, 0.9f);
    for (int y = 19; y <= 38; y++)                                  /* the breastplate, curved out */
        for (int x = 26; x <= 40; x++) {
            float front = 38.0f + 2.0f * sinf((y - 19) / 19.0f * PI_F) - (y > 32 ? (y - 32) * 0.5f : 0.0f), back = 27.0f + (y > 30 ? (y - 30) * 0.2f : 0.0f);
            if (x < back || x > front) continue;
            float u = (x - back) / (front - back);
            cv_px(x, y, sh(R_IRON, 0.25f + 0.6f * u - (x == (int)front ? 0.0f : 0.0f) + (fabsf(u - 0.85f) < 0.08f ? 0.2f : 0.0f)));
        }
    for (int b = 0; b < 3; b++) for (int x = 26; x <= 37; x++) cv_px(x, 39 + b * 2, sh(R_IRON, 0.35f + 0.3f * (x - 26) / 11.0f));   /* the fauld */
    cv_ellipse(31, 21, 5.0f, 4.0f, R_IRON, 0.3f, 1.0f);            /* pauldron */
    cv_capsule(32, 22, 37, 30, 1.4f, R_BONE, 0.3f, 0.9f);          /* arms to the grip */
    cv_capsule(37, 30, 41, 29, 1.2f, R_BONE, 0.3f, 0.9f);
    cv_ellipse(41.5f, 29, 2.2f, 2.0f, R_IRON, 0.3f, 0.9f);         /* gauntlets */
    cv_capsule(42, 33, 42, 27, 1.2f, R_WOOD, 0.2f, 0.6f);          /* the grip, the guard, the blade up */
    cv_capsule(38, 26, 46, 26, 1.0f, R_IRON, 0.35f, 0.95f);
    for (int y = 2; y < 26; y++) for (int o = -1; o <= 1; o++) cv_px(42 + o, y, sh(R_GHOST, 0.55f + 0.2f * o + (y < 4 ? -0.1f : 0.0f)));
    for (int y = 2; y <= 16; y++)                                   /* the great helm, in profile */
        for (int x = 26; x <= 39; x++) {
            float nx = (x + 0.5f - 32.5f) / 6.5f, top = 2.0f + 3.0f * nx * nx;
            if (fabsf(nx) > 1.0f || y < top) continue;
            float v = 0.25f + 0.6f * lit(nx, (y - 9) / 9.0f, sqrtf(fmaxf(0, 1 - nx * nx)));
            if (y == 9 && x > 34) v = 0.0f;                         /* the slit at the front */
            if (y > 11 && x > 35 && (y & 1)) v -= 0.3f;             /* breaths */
            cv_px(x, y, sh(R_IRON, v));
        }
    cv_px(37, 9, (uint8_t)(R_FLAME + 11));
    cv_outline(0);
    cv_store(id);
}

/* the shade from the side: narrower, leaning into its drift, one arm of smoke reaching ahead */
static void spr_shade_side(int id, float ph) {
    cv_begin(cv_sprite, TS, TS);
    for (int y = 4; y < TS; y++)
        for (int x = 0; x < TS; x++) {
            float dx = x + 0.5f - 31.0f + (y - 30) * 0.12f, hw;
            if (y < 16) { float hy = (y - 10.0f) / 6.0f; hw = 5.0f * sqrtf(fmaxf(0.0f, 1.0f - hy * hy)); dx -= 1.5f; }   /* the head, forward */
            else if (y < 20) { hw = 3.0f; dx -= 1.0f; }
            else hw = 8.5f + (y < 30 ? (y - 20) * 0.2f : 2.0f - (y - 30) * 0.1f);
            hw += sinf(y * 0.4f + ph) * 0.8f;
            float edge = hw - fabsf(dx);
            if (edge < 0.0f) continue;
            float fadey = y > 44 ? (y - 44) / 19.0f : 0.0f, n = h01(x, y, 515 + (int)(ph * 5));
            if (n < fadey * 1.1f) continue;
            if (edge < 1.5f && ((x + y) & 1)) continue;
            cv_px(x, y, edge < 2.5f ? sh(R_PURPLE, 0.12f + 0.12f * (1.0f - fadey)) : sh(R_IRON, 0.02f + 0.04f * n));
        }
    for (int i = 0; i <= 3; i++) cv_px(26 - i, 7 + i * 2, sh(R_PURPLE, 0.15f));   /* the hood's point, trailing */
    for (int i = 0; i <= 14; i++) {                                 /* an arm of smoke, reaching */
        float t = i / 14.0f, x = lerpf(35, 50, t), y = lerpf(23, 33 + sinf(ph) * 2.0f, t), r = 3.0f - t * 1.3f;
        for (int yy = (int)(y - r); yy <= (int)(y + r); yy++)
            for (int xx = (int)(x - r); xx <= (int)(x + r); xx++)
                if (hypotf(xx + 0.5f - x, yy + 0.5f - y) <= r) cv_px(xx, yy, (xx + yy) & 1 ? sh(R_PURPLE, 0.15f) : sh(R_IRON, 0.04f));
    }
    cv_px(35, 10, sh(R_PURPLE, 0.85f)); cv_px(35, 11, sh(R_PURPLE, 0.5f));
    cv_store(id);
}

/* the imp from the side: a hop, the tail and its ember behind, a wing up, the grin ahead */
static void spr_imp_side(int id, int pose) {
    cv_begin(cv_sprite, TS, TS);
    float up = pose == 0 ? -3.0f : 0.0f;
    cv_flat(32, 62, 11, 1.6f, R_WARM + 1);
    cv_capsule(26, 50 + up, 16, 54 + up, 1.3f, R_BRICK, 0.15f, 0.5f);   /* the tail */
    cv_capsule(16, 54 + up, 10, 46 + up, 1.1f, R_BRICK, 0.15f, 0.5f);
    cv_flat(9, 45 + up, 1.8f, 1.8f, (uint8_t)(R_FLAME + 12));
    float fw[8] = { 30, 33 + up, 15, 19 + up, 17, 31 + up, 24, 38 + up };   /* the far wing */
    cv_poly(fw, 4, R_BRICK, 0.04f, 0.18f, 19 + up, 19);
    cv_capsule(30, 46 + up, 35, 52 + up * 0.5f, 2.4f, R_BRICK, 0.1f, 0.45f);   /* the far leg */
    cv_capsule(35, 52 + up * 0.5f, 31, 61, 1.8f, R_BRICK, 0.1f, 0.45f);
    cv_ellipse(32, 44 + up, 8.0f, 8.5f, R_BRICK, 0.25f, 0.9f);     /* pot belly */
    cv_capsule(33, 46 + up, 38, 53 + up * 0.5f, 2.6f, R_BRICK, 0.2f, 0.75f);   /* the near leg */
    cv_capsule(38, 53 + up * 0.5f, 34, 61, 2.0f, R_BRICK, 0.2f, 0.7f);
    cv_ellipse(36, 61.5f, 3.4f, 1.4f, R_BRICK, 0.15f, 0.6f);
    float nw[8] = { 31, 34 + up, 18, 22 + up, 18, 31 + up, 24, 39 + up };   /* the near wing */
    cv_poly(nw, 4, R_BRICK, 0.08f, 0.32f, 22 + up, 17);
    cv_capsule(35, 38 + up, 42, 45 + up, 2.0f, R_BRICK, 0.25f, 0.85f);   /* an arm, out ahead */
    cv_ellipse(43, 46 + up, 2.3f, 2.1f, R_BRICK, 0.25f, 0.85f);
    float hy = 28 + up;
    cv_capsule(33, hy - 6, 28, hy - 11, 1.8f, R_BONE, 0.15f, 0.55f);   /* a horn, swept back */
    cv_capsule(28, hy - 11, 27, hy - 16, 1.1f, R_BONE, 0.15f, 0.5f);
    float e[6] = { 31, hy - 2, 23, hy - 5, 30, hy + 2 };            /* the ear, laid back */
    cv_poly(e, 3, R_BRICK, 0.2f, 0.7f, hy - 5, 7);
    cv_ellipse(36, hy, 8.5f, 8.0f, R_BRICK, 0.3f, 1.0f);
    cv_flat(40, hy - 1.5f, 1.8f, 1.5f, 0);
    cv_px(40, (int)(hy - 2), (uint8_t)(R_FLAME + 14)); cv_px(41, (int)(hy - 1), (uint8_t)(R_FLAME + 10));
    for (int x = 37; x <= 44; x++) {                                /* the grin, from the side */
        int y = (int)(hy + 4 - (x - 37) * 0.35f);
        cv_px(x, y, (uint8_t)(R_FLAME + 9)); cv_px(x, y + 1, (uint8_t)(R_FLAME + ((x & 1) ? 13 : 6)));
        if (!(x & 1)) cv_px(x, y - 1, sh(R_BONE, 0.8f));
    }
    for (int k = 0; k < 6; k++) {                                   /* cracks with the fire showing through */
        float x = 26 + 12 * h01(k, 1, 525), y = 37 + 13 * h01(k, 2, 525) + up, a = h01(k, 3, 525) * TAU_F;
        for (int i = 0; i < 5; i++) {
            a += (h01(k, i + 4, 525) - 0.5f) * 1.4f; x += cosf(a); y += sinf(a);
            uint8_t c = cv_sprite[(int)y * TS + (int)x];
            if (c >= R_BRICK && c < R_BRICK + 16) cv_px((int)x, (int)y, (uint8_t)(R_FLAME + 6 + (i & 1) * 3));
        }
    }
    cv_outline(0);
    cv_store(id);
}

/* the deacon from the side: the robe dragging behind, the staff carried before it, the skull in the hood */
static void spr_deacon_side(int id, float w) {
    cv_begin(cv_sprite, TS, TS);
    cv_flat(31, 62, 14, 1.8f, R_WARM + 1);
    float stx = 43.0f + w * 1.5f;                                   /* the staff, planted ahead */
    cv_capsule(stx, 9, stx - 1.0f, 62, 1.2f, R_WOOD, 0.2f, 0.65f);
    cv_skull(stx, 6, 0.7f, 0.3f, 1.0f);
    cv_px((int)stx - 1, 7, (uint8_t)(R_FLAME + 6)); cv_px((int)stx + 2, 7, (uint8_t)(R_FLAME + 6));
    for (int y = 16; y < 63; y++) {                                 /* the robe */
        float t = (y - 16) / 46.0f, front = 36.0f + 4.0f * t + w * 2.0f * t, back = 26.0f - 6.0f * t - (y > 50 ? (y - 50) * 0.5f : 0.0f) + w * 0.8f * t;
        for (int x = (int)back - 1; x <= (int)front + 1; x++) {
            if (x + 0.5f < back || x + 0.5f > front) continue;
            if (y > 58 && h01(x, y, 341 + (w > 0)) < (y - 58) / 5.0f) continue;
            float u = (x + 0.5f - back) / (front - back), v = 0.12f + 0.42f * sinf(u * PI_F) * (1.0f - 0.3f * t) + 0.08f * u;
            if (sinf(x * 0.9f + y * 0.12f) > 0.65f) v -= 0.08f;     /* folds */
            cv_px(x, y, sh(R_PURPLE, v));
        }
        if (y >= 20 && y < 52) { int x = (int)(front - 1.0f); cv_px(x, y, sh(R_BLOOD, 0.35f)); cv_px(x - 1, y, sh(R_BLOOD, 0.25f)); }   /* the stole */
    }
    cv_capsule(31, 21, 37, 30, 3.4f, R_PURPLE, 0.15f, 0.55f);      /* the sleeve, out to the staff */
    cv_capsule(37, 30, stx - 1.5f, 33, 3.0f, R_PURPLE, 0.15f, 0.5f);
    cv_ellipse(stx - 0.5f, 33, 1.9f, 1.8f, R_BONE, 0.3f, 0.95f);
    cv_ellipse(30, 12, 7.5f, 8.5f, R_PURPLE, 0.15f, 0.6f);         /* the hood, its point behind */
    float pt[6] = { 25, 6, 19, 12, 25, 15 };
    cv_poly(pt, 3, R_PURPLE, 0.1f, 0.35f, 6, 9);
    cv_flat(35, 13.5f, 3.4f, 5.6f, 0);                              /* the skull, in the shadow at the front */
    cv_ellipse(35.5f, 13.0f, 2.4f, 3.4f, R_BONE, 0.25f, 0.8f);
    cv_px(36, 12, (uint8_t)(R_FLAME + 8));
    for (int i = 0; i < 3; i++) cv_px(35 + i, 16, sh(R_BONE, i & 1 ? 0.4f : 0.85f));
    cv_bone_decay(343);
    cv_outline(0);
    cv_store(id);
}

/* the gargoyle from the side, in flight: the wings up and then down, the tail behind, claws hanging */
static void garg_wing(int down, float ox, float oy, float lo, float hi) {
    static const float UP[14] = { 31, 27, 27, 8, 20, 1, 12, 7, 7, 18, 19, 24, 26, 32 }, DN[14] = { 31, 28, 28, 45, 22, 57, 14, 53, 9, 44, 18, 37, 25, 33 };
    const float *w = down ? DN : UP;
    float p[14];
    for (int i = 0; i < 7; i++) { p[2 * i] = w[2 * i] + ox; p[2 * i + 1] = w[2 * i + 1] + oy; }
    cv_poly(p, 7, R_STONE, lo, hi, down ? 28 + oy : 1 + oy, 30);
    for (int i = 1; i <= 4; i++) cv_line(p[0], p[1], p[2 * i], p[2 * i + 1], sh(R_STONE, lo + 0.08f));   /* ribs */
}
static void spr_garg_side(int id, int down) {
    cv_begin(cv_sprite, TS, TS);
    garg_wing(down, 3.0f, -1.0f, 0.04f, 0.25f);                     /* the far wing */
    cv_capsule(23, 38, 13, 44, 1.8f, R_STONE, 0.15f, 0.6f);        /* the tail and its spade */
    cv_capsule(13, 44, 8, 40, 1.3f, R_STONE, 0.15f, 0.6f);
    float sp[6] = { 8, 36, 4, 41, 10, 42 };
    cv_poly(sp, 3, R_STONE, 0.15f, 0.6f, 36, 6);
    cv_capsule(29, 39, 31, 48, 2.6f, R_STONE, 0.12f, 0.5f);        /* the far leg, hanging */
    cv_capsule(31, 48, 28, 55, 2.0f, R_STONE, 0.12f, 0.5f);
    cv_ellipse(32, 34, 10.0f, 7.0f, R_STONE, 0.2f, 0.85f);         /* the body, hunched forward */
    cv_capsule(34, 40, 37, 49, 2.8f, R_STONE, 0.2f, 0.75f);        /* the near leg */
    cv_capsule(37, 49, 34, 56, 2.2f, R_STONE, 0.2f, 0.7f);
    for (int c = -1; c <= 1; c++) { cv_line(34, 56, 35 + c * 2.0f, 59.5f, sh(R_STONE, 0.35f)); cv_line(28, 55, 29 + c * 2.0f, 58.5f, sh(R_STONE, 0.25f)); }
    cv_capsule(38, 31, 45, 38, 2.2f, R_STONE, 0.25f, 0.8f);        /* an arm, claws ahead */
    for (int c = -1; c <= 1; c++) cv_line(45, 38, 47.5f, 40.0f + c * 1.5f, sh(R_STONE, 0.35f));
    garg_wing(down, 0.0f, 0.0f, 0.12f, 0.6f);                       /* the near wing */
    cv_capsule(41, 21, 37, 16, 1.6f, R_STONE, 0.2f, 0.75f);        /* a horn, curling back */
    cv_capsule(37, 16, 38, 12, 1.1f, R_STONE, 0.2f, 0.7f);
    cv_ellipse(43, 24, 5.5f, 5.0f, R_STONE, 0.25f, 0.95f);         /* the head, and its muzzle */
    cv_ellipse(47.5f, 26.5f, 3.2f, 2.3f, R_STONE, 0.2f, 0.8f);
    cv_flat(45, 23, 1.2f, 1.0f, 0); cv_px(45, 23, (uint8_t)(R_FLAME + 12));
    cv_flat(48.5f, 28, 2.0f, 0.6f, 0);
    for (int y = 0; y < TS; y++)                                    /* weathering on the stone */
        for (int x = 0; x < TS; x++) {
            uint8_t *p = &cv_sprite[y * TS + x];
            if (*p < R_STONE || *p >= R_STONE + 16) continue;
            float n = fbm(x / 4.0f, y / 4.0f, 16, 549);
            if (n < 0.3f) *p = (uint8_t)(*p - 2 < R_STONE + 1 ? R_STONE + 1 : *p - 2);
            else if (h01(x, y, 551) > 0.96f) *p = (uint8_t)(*p - 4 < R_STONE + 1 ? R_STONE + 1 : *p - 4);
        }
    cv_outline(0);
    cv_store(id);
}

/* the side views of bone age with the level, like the skeletons */
static uint8_t side_drawn[SP_GARG_SB - SP_SHAMB_SA + 1];          /* the new creatures' side views, as they are drawn */
static void spr_sides(void) {
    spr_skel_side(SP_SKEL_SA, 1.0f, 0); spr_skel_side(SP_SKEL_SB, -1.0f, 0);
    spr_skel_side(SP_SHIELD_SA, 1.0f, 1); spr_skel_side(SP_SHIELD_SB, -1.0f, 1);
    spr_bat_side(SP_BAT_SA, 1); spr_bat_side(SP_BAT_SB, 0);
    spr_spider_side(SP_SPIDER_SA, 0); spr_spider_side(SP_SPIDER_SB, 1);
    spr_ghost_side(SP_GHOST_SA, 0.0f); spr_ghost_side(SP_GHOST_SB, PI_F);
    spr_wskull_side();
    spr_shambler_side(SP_SHAMB_SA, 1.0f); spr_shambler_side(SP_SHAMB_SB, -1.0f);
    spr_drowned_side(SP_DROWN_SA, 1.0f); spr_drowned_side(SP_DROWN_SB, -1.0f);
    spr_hound_side(SP_HOUND_SA, 1.0f); spr_hound_side(SP_HOUND_SB, -1.0f);
    spr_wraith_side(SP_WRAITH_SA, 0.0f); spr_wraith_side(SP_WRAITH_SB, PI_F);
    spr_hunter_side(SP_HUNTER_SA, 0); spr_hunter_side(SP_HUNTER_SB, 1);
    spr_ghoul_side(SP_GHOUL_SA, 1.0f); spr_ghoul_side(SP_GHOUL_SB, -1.0f);
    spr_knight_side(SP_KNIGHT_SA, 1.0f); spr_knight_side(SP_KNIGHT_SB, -1.0f);
    spr_brood_side(SP_BROOD_SA, 0); spr_brood_side(SP_BROOD_SB, 1);
    spr_shade_side(SP_SHADE_SA, 0.0f); spr_shade_side(SP_SHADE_SB, 1.6f);
    spr_imp_side(SP_IMP_SA, 0); spr_imp_side(SP_IMP_SB, 1);
    spr_deacon_side(SP_DEACON_SA, 1.0f); spr_deacon_side(SP_DEACON_SB, -1.0f);
    spr_garg_side(SP_GARG_SA, 0); spr_garg_side(SP_GARG_SB, 1);
    for (int i = SP_SHAMB_SA; i <= SP_GARG_SB; i++) side_drawn[i - SP_SHAMB_SA] = 1;
    static const int FRONT[][3] = {                                /* not drawn from the side yet: the front view stands in */
        { SP_SHAMB_SA, SP_SHAMB_A, SP_SHAMB_B }, { SP_DROWN_SA, SP_DROWN_A, SP_DROWN_B }, { SP_HOUND_SA, SP_HOUND_A, SP_HOUND_B },
        { SP_WRAITH_SA, SP_WRAITH_A, SP_WRAITH_B }, { SP_HUNTER_SA, SP_HUNTER_A, SP_HUNTER_B }, { SP_GHOUL_SA, SP_GHOUL_A, SP_GHOUL_B },
        { SP_KNIGHT_SA, SP_KNIGHT_A, SP_KNIGHT_B }, { SP_BROOD_SA, SP_BROOD_A, SP_BROOD_B }, { SP_SHADE_SA, SP_SHADE_A, SP_SHADE_B },
        { SP_IMP_SA, SP_IMP_A, SP_IMP_B }, { SP_DEACON_SA, SP_DEACON_A, SP_DEACON_B }, { SP_GARG_SA, SP_GARG_A, SP_GARG_B },
    };
    for (int i = 0; i < (int)(sizeof FRONT / sizeof FRONT[0]); i++)
        if (!side_drawn[FRONT[i][0] - SP_SHAMB_SA]) {
            memcpy(crawl_spr[FRONT[i][0]], crawl_spr[FRONT[i][1]], TS * TS);
            memcpy(crawl_spr[FRONT[i][0] + 1], crawl_spr[FRONT[i][2]], TS * TS);
        }
}

static void find_glints(void) {
    for (int i = 0; i < NSPR; i++) {
        int bx = -1, by = -1, best = -1;
        for (int x = 0; x < TS; x++)
            for (int y = 0; y < TS; y++) {
                int c = crawl_spr[i][x * TS + y];
                if (c >= R_GOLD && c < R_GOLD + NGOLD && c > best) { best = c; bx = x; by = y; }
            }
        crawl_glint[i][0] = (int8_t)bx; crawl_glint[i][1] = (int8_t)by;
    }
}

static void spr_bestiary_bones(void);
static void spr_sides(void);

/* ---- ranks: the body ramp redrawn in the rank's own (see crawl.h) ---- */

uint8_t crawl_rank_lut[NPF][NRANK][256];

static void lut_ramp(uint8_t *lut, int from, int to) {             /* the gold ramp is short: spread over it */
    for (int k = 0; k < 16; k++) lut[from + k] = (uint8_t)(to == R_GOLD ? R_GOLD + (k * (NGOLD - 1) + 7) / 15 : to + k);
}

static void build_rank_luts(void) {
    static const int BODY[NPF] = { R_BONE, R_WARM, R_IRON, R_WOOD, R_PURPLE, R_BRICK, R_STONE, R_IRON, R_PURPLE };
    static const int LADDER[NRANK] = { -1, R_MOSS, R_GHOST, R_BLOOD, R_IRON, R_GOLD };
    for (int f = 0; f < NPF; f++)
        for (int r = 0; r < NRANK; r++) {
            uint8_t *lut = crawl_rank_lut[f][r];
            for (int i = 0; i < 256; i++) lut[i] = (uint8_t)i;
            int to = LADDER[r];
            if (to < 0) continue;
            if (f == PF_ARMOUR && r == RK_V) {                     /* plate blackened: the same iron, deeper */
                for (int k = 0; k < 16; k++) lut[R_IRON + k] = (uint8_t)(R_IRON + 1 + k * 9 / 15);
                continue;
            }
            if (to == BODY[f]) to = R_BONE;                        /* black already: the deep ones go pale instead */
            if (f == PF_WING && r == RK_V) to = R_BLOOD;           /* a black bat is lost in the dark: they stop at red */
            lut_ramp(lut, BODY[f], to);
            if (to == R_MOSS) lut_ramp(lut, R_MOSS, R_WARM);       /* moss on a green body would vanish: grime instead */
        }
}

/* ------------------------------------------------------------------ */

/* ---- each level down looks older: its own hue, darker stone, rot and ruin ---- */

static void ramp_tinted(int base, const int a[3], const int m[3], const int b[3], const float tint[3], float k) {
    int A[3], M[3], B[3];
    for (int c = 0; c < 3; c++) {
        A[c] = (int)(a[c] * tint[c] * k + 0.5f); M[c] = (int)(m[c] * tint[c] * k + 0.5f); B[c] = (int)(b[c] * tint[c] * k + 0.5f);
    }
    ramp3(base, A, M, B);
}

/* darken a texture pixel within its own ramp */
static uint8_t darker(uint8_t c, int by) {
    if (c == 0 || c >= C_WHITE || IS_FULLBRIGHT(c)) return c;
    int base = ((c - 1) / 16) * 16 + 1;
    return (uint8_t)(c - by < base + 1 ? base + 1 : c - by);
}

static void decay_tex(int t, float dk, uint32_t seed) {
    if (dk <= 0.0f) return;
    for (int y = 0; y < TS; y++)                                    /* grime and damp */
        for (int x = 0; x < TS; x++) {
            uint8_t c = crawl_tex[t][x * TS + y];
            float n = fbm(x / 9.0f, y / 9.0f, 8, seed + 5);
            if (n < 0.22f + 0.3f * dk) c = darker(c, 1 + (n < 0.2f + 0.15f * dk));
            if (dk > 0.25f && y > 40 && fbm(x / 6.0f, y / 4.0f, 16, seed + 9) > 0.8f - 0.25f * dk - (y - 40) * 0.006f && !IS_FULLBRIGHT(c))
                c = sh(R_MOSS, 0.2f + 0.35f * h01(x, y, seed));      /* moss creeping up from the floor */
            crawl_tex[t][x * TS + y] = c;
        }
    int drips = (int)(dk * 7.0f);                                    /* streaks running down */
    for (int k = 0; k < drips; k++) {
        int x = (int)(h01(k, 1, seed) * 63), len = 10 + (int)(h01(k, 2, seed) * 44.0f * dk);
        for (int y = 0; y < len; y++)
            if (!IS_FULLBRIGHT(crawl_tex[t][x * TS + y])) crawl_tex[t][x * TS + y] = darker(crawl_tex[t][x * TS + y], 2);
    }
    int cracks = (int)(dk * 6.0f + 0.5f);                            /* cracks wandering through the stone */
    for (int k = 0; k < cracks; k++) {
        float x = h01(k, 3, seed) * 64.0f, y = h01(k, 4, seed) * 64.0f, a = h01(k, 5, seed) * TAU_F;
        int len = 8 + (int)(h01(k, 6, seed) * 26.0f);
        for (int i = 0; i < len; i++) {
            a += (h01(k, i + 10, seed) - 0.5f) * 1.2f;
            x += cosf(a); y += sinf(a);
            int xi = ((int)x) & 63, yi = ((int)y) & 63;
            uint8_t c = crawl_tex[t][xi * TS + yi];
            if (!IS_FULLBRIGHT(c)) crawl_tex[t][xi * TS + yi] = darker(c, 6);
        }
    }
    int chips = (int)(dk * 5.0f);                                    /* broken-off corners */
    for (int k = 0; k < chips; k++) {
        int x0 = (int)(h01(k, 7, seed) * 60), y0 = (int)(h01(k, 8, seed) * 60), w = 2 + (int)(h01(k, 9, seed) * 4), h = 2 + (int)(h01(k, 11, seed) * 3);
        for (int y = y0; y < y0 + h; y++)
            for (int x = x0; x < x0 + w; x++) {
                uint8_t c = crawl_tex[t][(x & 63) * TS + (y & 63)];
                if (!IS_FULLBRIGHT(c)) crawl_tex[t][(x & 63) * TS + (y & 63)] = darker(c, y == y0 ? 1 : 4);
            }
    }
}

/* the level's palette: its set's tint, dimmer going down, bones yellowing with age */
static void level_palette(int level) {
    static const float TINT[4][2][3] = {                            /* each set's stone, from its run's first level to its last */
        { { 1.00f, 1.00f, 1.00f }, { 1.10f, 1.05f, 0.88f } },     /* the Crypts: blue-grey limestone, yellowing like bone */
        { { 0.88f, 1.06f, 0.90f }, { 0.84f, 0.95f, 1.20f } },     /* the Catacombs: green with damp, to cold wet slate */
        { { 1.14f, 1.00f, 0.76f }, { 1.16f, 0.86f, 0.82f } },     /* the Hollows: sandstone, to rust and old blood */
        { { 0.92f, 0.92f, 0.94f }, { 1.04f, 0.88f, 1.14f } },     /* the Deep: ash, to violet */
    };
    int set = crawl_set(level), step = crawl_step(level);
    float sk = (float)step / (float)(RUN_LEN - 1);
    float dk = crawl_decay(level), depth = crawl_depth(level), dim = 1.0f - 0.09f * set - 0.06f * sk;
    float tn[3], half[3];
    for (int c = 0; c < 3; c++) { tn[c] = lerpf(TINT[set][0][c], TINT[set][1][c], sk); half[c] = 1.0f + (tn[c] - 1.0f) * 0.5f; }
    build_palette();
    { static const int a[3] = { 2, 2, 4 }, m[3] = { 20, 21, 26 }, b[3] = { 46, 47, 52 }; ramp_tinted(R_STONE, a, m, b, tn, dim); }
    { static const int a[3] = { 2, 2, 2 }, m[3] = { 15, 13, 11 }, b[3] = { 34, 30, 26 }; ramp_tinted(R_FLOOR, a, m, b, half, dim); }
    { static const int a[3] = { 1, 1, 2 }, m[3] = { 10, 10, 13 }, b[3] = { 26, 26, 32 }; ramp_tinted(R_CEIL, a, m, b, half, dim); }
    {                                                                /* bones yellow and dull with age */
        float age = 0.4f * dk + 0.6f * depth, aged[3] = { 1.0f - 0.12f * age, 1.0f - 0.16f * age, 1.0f - 0.38f * age };
        static const int a[3] = { 4, 3, 2 }, m[3] = { 34, 30, 22 }, b[3] = { 62, 58, 48 };
        ramp_tinted(R_BONE, a, m, b, aged, 1.0f);
    }
}

/* Inside a pocket the floor and ceiling ramps are the theme's: for the Backrooms, R_FLOOR runs from
 * dark brown through mustard to pale yellow (carpet and wallpaper both) and R_CEIL is off-white; for
 * Severance, R_FLOOR is a muted green (carpet, doors, partitions) and R_CEIL a cool white. */
void crawl_art_theme(int theme, int level) {
    level_palette(level);
    if (theme == PT_BACKROOMS) {
        RAMP(R_FLOOR, 4, 3, 1,  33, 28, 12,  58, 55, 34);
        RAMP(R_CEIL,  3, 3, 2,  30, 29, 24,  60, 59, 53);
    } else if (theme == PT_SEVERANCE) {                             /* a muted green; a cool white */
        RAMP(R_FLOOR, 2, 4, 3,  16, 25, 19,  42, 53, 44);
        RAMP(R_CEIL,  3, 3, 4,  32, 34, 35,  62, 63, 63);
    }
    build_colormap();
}

static void specials_plain(void);
static void treasure_plain(void);
void crawl_art_level(int level) {
    float dk = crawl_decay(level), depth = crawl_depth(level);
    g_decay = fmaxf(dk, depth);                                     /* the dead themselves only get older going down */
    level_palette(level);
    build_colormap();
    gen_stone(T_STONE, R_STONE, 11 + level * 7);
    gen_rubble(T_RUBBLE, 23 + level * 7);
    gen_skulls(T_SKULLS, 31);
    gen_brick(T_BRICK, 47 + level * 7);
    gen_moss(T_MOSS, 53 + level * 7);
    gen_niche(T_NICHE, 11);
    gen_banner(T_BANNER, 11);
    for (int f = 0; f < 3; f++) gen_torch(T_TORCH0 + f, f, 11);
    gen_jamb(T_JAMB, 79);
    gen_passage(T_PASSAGE, crawl_set(level) < 2);
    static const int DECAYS[] = { T_STONE, T_RUBBLE, T_SKULLS, T_BRICK, T_MOSS, T_NICHE, T_BANNER, T_TORCH0, T_TORCH1, T_TORCH2, T_JAMB };
    for (int i = 0; i < (int)(sizeof DECAYS / sizeof DECAYS[0]); i++)
        decay_tex(DECAYS[i], dk, (uint32_t)(DECAYS[i] >= T_TORCH0 && DECAYS[i] <= T_TORCH2 ? 900 : 900 + DECAYS[i] * 13 + level * 31));
    spr_skeleton(SP_SKEL_A, 1.0f, 0.0f, 3, 0);
    spr_skeleton(SP_SKEL_B, -1.0f, 0.0f, 3, 0);
    spr_skeleton(SP_SKEL_D1, 0.0f, 0.35f, 3, 0);
    spr_skeleton(SP_SKEL_D2, 0.0f, 0.7f, 3, 0);
    spr_skeleton(SP_SKEL_D3, 0.0f, 1.0f, 3, 0);
    spr_skeleton(SP_SKEL_T, 0.3f, 0.0f, 3, 1);
    spr_shieldskel(SP_SHIELD_A, 1.0f, 0, 1); spr_shieldskel(SP_SHIELD_B, -1.0f, 0, 1); spr_shieldskel(SP_SHIELD_S, 0.2f, 1, 1);
    spr_shieldskel(SP_SHIELD_XA, 1.0f, 0, 0); spr_shieldskel(SP_SHIELD_XB, -1.0f, 0, 0); spr_shieldskel(SP_SHIELD_XS, 0.2f, 1, 0);
    spr_bestiary_bones();
    spr_sides();
    treasure_plain();
    if (crawl_drawn(level)) crawl_art_run(level);                  /* the run's own look (docs/PLAN-b10.md) */
    else {                                                          /* not drawn yet: the look as it was */
        static const float KS[4] = { 1.0f, 0.5f, 0.12f, -0.5f };
        for (int f = 0; f < 4; f++) { spr_key(SP_KEY0 + f, KS[f], R_GOLD); spr_key(SP_KEYS0 + f, KS[f], R_IRON); spr_key(SP_KEYC0 + f, KS[f], R_BLOOD); }
        specials_plain();
    }
}

/* ------------------------------------------------------------------ */
/* Each run's own look (docs/PLAN-b10.md)                              */
/* ------------------------------------------------------------------ */

/* A run's materials are laid on one grid of courses, 16 pixels high, so where two meet the courses run
 * on. g_mid keeps each material's blocks or stones by pixel (0 a joint), so a blend can let whole blocks
 * fall away. What stands on a wall is drawn apart, C_CLEAR where the wall shows through. Only the Crypts
 * are drawn so far; the other runs borrow them until theirs are approved. */
static uint16_t g_mid[NMAT][TS * TS];

const RunLook crawl_looks[4] = {
    { "THE ORDER'S CRYPTS",
      { "DRESSED ASHLAR", "COURSED RUBBLE", "LIME PLASTER", "CUT BEDROCK", "BONE WALL" },
      { "BURIAL NICHES", "THE ORDER'S BANNER", "A KNIGHT'S SLAB", "WALL SHRINE", "MEMORIAL TABLET", "TORCH", "TORCH", "TORCH" },
      { "FLAGSTONES", "CHAPTER FLOOR", "WORN SETTS", "BONE EARTH", "BROKEN FLAGS" },
      { "RIBBED VAULT", "STONE VAULT", "ROUGH ROCK" },
      { { M1_ASHLAR, M1_RUBBLE }, { M1_PLASTER, M1_ASHLAR }, { M1_PLASTER, M1_RUBBLE }, { M1_ASHLAR, M1_BEDROCK },
        { M1_RUBBLE, M1_BEDROCK }, { M1_BONES, M1_ASHLAR } } },
    { "THE DROWNED CATACOMBS",
      { "BEDDED SLATE", "SLATE LINING", "SILT AND RUBBLE", "FLOWSTONE", "BONE IN SILT" },
      { "FLOODED NICHES", "A SAINT'S ALCOVE", "A CULVERT", "HANDPRINTS", "A WEB CURTAIN", "LANTERN", "LANTERN", "LANTERN" },
      { "WET FLAGS", "SILT", "SLATE ON EDGE", "STANDING WATER", "PUDDLED FLAGS" },
      { "SLATE VAULT", "BEDDED ROCK", "DRIPPING ROCK" },
      { { M2_LINING, M2_SLATE }, { M2_LINING, M2_SILT }, { M2_SLATE, M2_SILT }, { M2_FLOW, M2_SLATE },
        { M2_FLOW, M2_LINING }, { M2_BONES, M2_SILT } } },
    { "THE CHARNEL MINE",
      { "ROUGH GRANITE", "PICK-CUT FACE", "TIMBER SHORING", "BASALT COLUMNS", "PACKED BACKFILL" },
      { "AN ORE VEIN", "FOSSIL RIBS", "A TIMBER SET", "AN ORE CHUTE", "THE CULT'S MARKS", "OIL LAMP", "OIL LAMP", "OIL LAMP" },
      { "GRAVEL", "RAILS", "TRODDEN EARTH", "CHARNEL FLOOR", "FALLEN SCREE" },
      { "TIMBER LAGGING", "ROUGH ROCK", "ROCK AND ROOTS" },
      { { M3_FACE, M3_GRANITE }, { M3_TIMBER, M3_FACE }, { M3_TIMBER, M3_GRANITE }, { M3_FACE, M3_FILL },
        { M3_BASALT, M3_GRANITE }, { M3_FILL, M3_GRANITE } } },
    { "THE BLACK GLASS",
      { "OBSIDIAN", "BASALT COLUMNS", "ASH-CRUSTED ROCK", "CRYSTAL CRUST", "BONE IN GLASS" },
      { "A GEODE", "A GEM SEAM", "GOLD VEINS", "FUSED BONES", "THE THRONE'S FRIEZE", "MAGMA FISSURE", "MAGMA FISSURE", "MAGMA FISSURE" },
      { "CRACKED GLASS", "THRONE FLAGS", "DRIFTED ASH", "CINDERS", "SHATTERED GLASS" },
      { "CRYSTAL", "GLINTING ROCK", "SOOTED ROCK" },
      { { M4_OBSIDIAN, M4_ASH }, { M4_BASALT, M4_OBSIDIAN }, { M4_BASALT, M4_ASH }, { M4_CRYSTAL, M4_OBSIDIAN },
        { M4_CRYSTAL, M4_ASH }, { M4_BONES, M4_OBSIDIAN } } },
};

static uint8_t lighter(uint8_t c, int by) {                        /* lighten a pixel within its own ramp */
    if (c == 0 || c >= C_WHITE || IS_FULLBRIGHT(c)) return c;
    int base = ((c - 1) / 16) * 16 + 1;
    return (uint8_t)(c + by > base + 15 ? base + 15 : c + by);
}
static void cv_to_tex(int t) {                                     /* the canvas (row-major) into a texture (column-major) */
    for (int x = 0; x < TS; x++)
        for (int y = 0; y < TS; y++) crawl_tex[t][x * TS + y] = cv_sprite[y * TS + x];
}
static float wrapd(float d) { d = fabsf(d); return d > 32.0f ? 64.0f - d : d; }

/* ---- the Crypts' materials ---- */

/* dressed ashlar: courses 16 high, blocks 32 long (some cut in two), every other course set over by 16 */
static int ashlar_at(int x, int y, uint32_t seed, int *lx, int *ly, int *bw) {
    int r = (y & 63) >> 4, off = (r & 1) * 16, xx = (x - off) & 63, slot = xx >> 5, bx = xx & 31, w = 32;
    if (h01(r, slot, seed) > 0.62f) { w = 16; slot = slot * 2 + (bx >> 4); bx &= 15; } else slot *= 2;
    *lx = bx; *ly = y & 15; *bw = w;
    return 1 + r * 4 + slot;
}
static const char *const MASON[4][5] = {                           /* masons' marks, cut small in a block's face */
    { "..#..", "..#..", "#####", "..#..", "..#.." },
    { "..#..", ".###.", "#.#.#", "..#..", "..#.." },
    { "..#..", ".#.#.", "#...#", "#####", "....." },
    { "####.", "...#.", "...#.", ".#.#.", "..#.." },
};
/* vseed 0 is the plain look; another gives the same courses with a mason's mark, a block put back
 * later, a chipped corner, so the two sit side by side without a seam */
static void gen_ashlar(int t, int m, uint32_t seed, uint32_t vseed) {
    int ra = (int)(hsh(1, 1, vseed) & 1);
    int idm = vseed ? 1 + ra * 8 + (int)(hsh(1, 2, vseed) & 1) * 2 : -1;          /* blocks in even courses never cross the edge */
    int idr = vseed ? 1 + (1 - ra) * 8 + (int)(hsh(1, 3, vseed) & 1) * 2 : -1;
    int idc = 1 + (int)(hsh(1, 4, vseed + 7) & 1) * 8 + 2 * (int)(hsh(1, 5, vseed + 7) & 1);
    const char *const *gl = MASON[hsh(1, 6, vseed) & 3];
    for (int y = 0; y < TS; y++)
        for (int x = 0; x < TS; x++) {
            int lx, ly, w, id = ashlar_at(x, y, seed, &lx, &ly, &w);
            if (lx == 0 || ly == 0) { g_mid[m][x * TS + y] = 0; tput(t, x, y, sh(R_STONE, 0.08f + 0.06f * h01(x, y, seed + 1))); continue; }
            g_mid[m][x * TS + y] = (uint16_t)id;
            float quiet = id == idr ? 0.5f : 1.0f;
            float v = 0.48f + 0.11f * (2.0f * h01(id, 3, seed) - 1.0f) - (id == idr ? 0.09f : 0.0f)
                    + quiet * (0.17f * (fbm(x / 8.0f, y / 8.0f, 8, seed + 2) - 0.5f) + 0.07f * (fbm(x / 2.0f, y / 2.0f, 32, seed + 3) - 0.5f));
            int dir = (hsh(id, 4, seed) & 1) ? 1 : -1;               /* the tooling, one way or the other */
            if ((((x * dir + y * 2) % 4) + 4) % 4 == 0) v -= 0.022f * quiet;
            if (ly == 1) v += 0.12f; else if (lx == 1) v += 0.06f;
            if (ly == 15) v -= 0.13f; else if (lx == w - 1) v -= 0.07f;
            if (h01(x, y, seed + 5) > 0.982f) v -= 0.14f;
            if (id == idm) {
                int gx = lx - (w / 2 - 2), gy = ly - 6;
                if (gx >= 0 && gx < 5 && gy >= 0 && gy < 5 && gl[gy][gx] == '#') v -= 0.13f;
                else if (gx >= 1 && gx <= 5 && gy >= 1 && gy <= 5 && gl[gy - 1][gx - 1] == '#') v += 0.05f;
            }
            if (id == idc && (w - lx) + (16 - ly) < 8 + (int)(h01(x, y, seed + 6) * 2.0f)) v -= 0.17f;
            tput(t, x, y, sh(R_STONE, v));
        }
}

/* coursed rubble in lime: rough stones laid in courses of seven to nine, two to every course of ashlar,
 * pointed with a pale mortar. In place of the brick: what an order would build with below ground */
typedef struct { int sx[8][24], sw[8][24], ns[8], y0[8], h[8]; } RubbleLay;
static RubbleLay g_rl;
static void rubble_lay(uint32_t seed) {
    for (int p = 0; p < 4; p++) {
        int split = 7 + (int)(h01(p, 1, seed) * 3.0f);
        g_rl.y0[2 * p] = p * 16; g_rl.h[2 * p] = split; g_rl.y0[2 * p + 1] = p * 16 + split; g_rl.h[2 * p + 1] = 16 - split;
    }
    for (int c = 0; c < 8; c++) {
        int x = (int)(h01(c, 2, seed) * 64.0f), left = 64, k = 0;
        while (left > 0 && k < 24) {
            int w = 10 + (int)(h01(c, 10 + k, seed) * 11.0f);
            if (left - w < 8) w = left;
            g_rl.sx[c][k] = x; g_rl.sw[c][k] = w; x += w; left -= w; k++;
        }
        g_rl.ns[c] = k;
    }
}
static int rubble_at(int x, int y, uint32_t seed, float *u, float *v) {   /* the stone there (0 mortar), and where in it */
    x &= 63; y &= 63;
    int c = 0;
    while (c < 7 && y >= g_rl.y0[c] + g_rl.h[c]) c++;
    for (int k = 0; k < g_rl.ns[c]; k++) {
        int px = (x - g_rl.sx[c][k]) & 63, w = g_rl.sw[c][k], h = g_rl.h[c], py = y - g_rl.y0[c];
        if (px >= w) continue;
        int id = 1 + c * 24 + k;
        float top = 0.3f + 0.9f * h01(id, 6, seed), bot = 0.3f + 0.9f * h01(id, 7, seed);   /* not every stone fills its course */
        float uu = (px + 0.5f - w * 0.5f) / (w * 0.5f - 0.5f), vv = py + 0.5f < h * 0.5f ? (py + 0.5f - h * 0.5f) / (h * 0.5f - top) : (py + 0.5f - h * 0.5f) / (h * 0.5f - bot);
        float r = powf(fabsf(uu), 4.0f) + powf(fabsf(vv), 3.2f);
        *u = uu; *v = vv;
        return r < 1.0f - 0.35f * h01(x, y, seed + 9) * (fabsf(uu) > 0.6f || fabsf(vv) > 0.6f) ? id : 0;
    }
    *u = *v = 0.0f;
    return 0;
}
static void gen_rubble_c(int t, int m, uint32_t seed, uint32_t vseed) {
    rubble_lay(seed);
    int lc = (int)(hsh(2, 1, vseed) % 8), lost = vseed ? 1 + lc * 24 + (int)(hsh(2, 2, vseed) % (uint32_t)g_rl.ns[lc]) : -1;
    for (int y = 0; y < TS; y++)
        for (int x = 0; x < TS; x++) {
            float u, v;
            int id = rubble_at(x, y, seed, &u, &v);
            g_mid[m][x * TS + y] = (uint16_t)id;
            if (!id || id == lost) {                                 /* the mortar, shadowed under each stone; or a stone gone */
                float u2, v2;
                int above = rubble_at(x, y - 1, seed, &u2, &v2), above2 = rubble_at(x, y - 2, seed, &u2, &v2);
                if (id == lost) { tput(t, x, y, sh(R_STONE, 0.03f + 0.04f * h01(x, y, seed + 13))); continue; }
                float mv = 0.66f + 0.1f * (h01(x, y, seed + 11) - 0.5f) + 0.06f * (fbm(x / 4.0f, y / 4.0f, 16, seed + 12) - 0.5f);
                if (above && above != lost) mv -= 0.28f; else if (above2 && above2 != lost) mv -= 0.1f;
                tput(t, x, y, sh(R_STONE, mv));
                continue;
            }
            int sand = h01(id, 4, seed) < 0.18f || (vseed && h01(id, 4, vseed) < 0.08f);
            float r = powf(fabsf(u), 4.0f) + powf(fabsf(v), 3.2f);
            float val = 0.30f + 0.2f * h01(id, 5, seed) - 0.08f * u - 0.16f * v - (r > 0.7f ? 0.06f : 0.0f)
                      + 0.12f * (fbm(x / 4.0f, y / 4.0f, 16, seed + 14) - 0.5f) + 0.05f * (fbm(x / 2.0f, y / 2.0f, 32, seed + 15) - 0.5f);
            tput(t, x, y, sand ? sh(R_FLOOR, val + 0.2f) : sh(R_STONE, val));
        }
}

/* lime plaster over the stone, trowelled, with a painted dado of red ochre and its crosses; vseed paints a
 * consecration cross in a ring */
static void gen_plaster(int t, int m, uint32_t seed, uint32_t vseed) {
    for (int y = 0; y < TS; y++)
        for (int x = 0; x < TS; x++) {
            float v = 0.62f + 0.10f * (fbm(x / 16.0f, y / 16.0f, 4, seed) - 0.5f) + 0.05f * (fbm(x / 4.0f, y / 4.0f, 16, seed + 1) - 0.5f);
            for (int k = 0; k < 6; k++) {                            /* the trowel's arcs */
                float cx = h01(k, 1, seed) * 64.0f, cy = h01(k, 2, seed) * 64.0f, rr = 6.0f + h01(k, 3, seed) * 10.0f;
                float d = hypotf(wrapd(x + 0.5f - cx), wrapd(y + 0.5f - cy));
                if (fabsf(d - rr) < 0.6f && h01(x, y, seed + 4 + k) > 0.45f) v -= 0.03f;
            }
            uint8_t c = sh(R_STONE, v);
            int worn = fbm(x / 4.0f, y / 4.0f, 16, seed + 20) < 0.33f;
            if (!worn && (y == 39 || y == 49)) c = sh(R_BRICK, 0.36f + 0.05f * h01(x, y, seed + 21));
            if (y >= 41 && y <= 47) {
                int lx = (x + 4) & 15, ly = y - 41;
                int cross = ((lx == 7 || lx == 8) && ly >= 1 && ly <= 5) || (ly == 3 && lx >= 5 && lx <= 10);
                if (!cross && !worn) c = sh(R_BRICK, 0.40f + 0.08f * (fbm(x / 2.0f, y / 2.0f, 32, seed + 22) - 0.5f));
            }
            if (vseed) {
                float dx = x + 0.5f - 32.0f, dy = y + 0.5f - 21.0f, d = hypotf(dx, dy);
                int ring = d > 8.0f && d < 9.6f;
                int arm = d < 7.6f && ((fabsf(dx) < 1.4f + 0.32f * fabsf(dy)) && fabsf(dy) > 1.0f ? 1 : (fabsf(dy) < 1.4f + 0.32f * fabsf(dx) && fabsf(dx) > 1.0f));
                if ((ring || arm) && fbm(x / 4.0f, y / 4.0f, 16, seed + 23) > 0.3f) c = sh(R_BRICK, 0.38f);
            }
            tput(t, x, y, c);
            g_mid[m][x * TS + y] = 0;
        }
}

/* the rock the crypts are cut from: a face dressed with the pick in patches of parallel strokes, the beds
 * showing faintly, a few old breaks; vseed has a fissure down it */
static void gen_bedrock(int t, int m, uint32_t seed, uint32_t vseed) {
    static float buf[TS * TS];
    float px[9], py[9], pb[9];
    for (int i = 0; i < 9; i++) { px[i] = h01(i, 1, seed) * 64; py[i] = h01(i, 2, seed) * 64; pb[i] = h01(i, 3, seed); }
    for (int y = 0; y < TS; y++)
        for (int x = 0; x < TS; x++) {
            float d1 = 1e9f, d2 = 1e9f; int n1 = 0;
            for (int i = 0; i < 9; i++) {
                float dx = wrapd(x + 0.5f - px[i]), dy = wrapd(y + 0.5f - py[i]), d = sqrtf(dx * dx + dy * dy);
                if (d < d1) { d2 = d1; d1 = d; n1 = i; } else if (d < d2) d2 = d;
            }
            float v = 0.38f + 0.07f * pb[n1] + 0.2f * (fbm(x / 16.0f, y / 16.0f, 4, seed + 1) - 0.5f) + 0.1f * (fbm(x / 4.0f, y / 4.0f, 16, seed + 4) - 0.5f)
                    + 0.04f * (fbm(x / 2.0f, y / 2.0f, 32, seed + 2) - 0.5f)
                    + 0.04f * sinf(y * TAU_F / 16.0f + 2.5f * fbm(x / 16.0f, y / 16.0f, 4, seed + 3));
            if (d2 - d1 < 1.0f) v -= 0.06f;
            buf[x * TS + y] = v;
            g_mid[m][x * TS + y] = 0;
        }
    for (int k = 0; k < 16; k++) {                                   /* the pick's work: patches of parallel grooves */
        float cx = h01(k, 4, seed) * 64, cy = h01(k, 5, seed) * 64, a = (k & 1 ? 0.7f : -0.75f) + 0.25f * (h01(k, 6, seed) - 0.5f);
        float rad = 6.0f + h01(k, 7, seed) * 5.0f, ca = cosf(a), sa = sinf(a);
        for (int yy = -12; yy <= 12; yy++)
            for (int xx = -12; xx <= 12; xx++) {
                float d = sqrtf((float)(xx * xx + yy * yy));
                if (d > rad) continue;
                float across = xx * -sa + yy * ca, f = 1.0f - d / rad;
                float g = fmodf(across + 30.0f, 3.0f);
                int xi = ((int)cx + xx) & 63, yi = ((int)cy + yy) & 63;
                if (g < 0.9f) buf[xi * TS + yi] -= 0.09f * f; else if (g < 1.6f) buf[xi * TS + yi] += 0.04f * f;
            }
    }
    if (vseed) {
        float x = 20.0f + h01(3, 1, vseed) * 24.0f;
        for (int y = 0; y < TS; y++) {
            x += (h01(y, 2, vseed) - 0.5f) * 1.4f;
            int xi = ((int)x) & 63;
            buf[xi * TS + y] -= 0.22f; buf[((xi + 1) & 63) * TS + y] -= 0.08f; buf[((xi - 1) & 63) * TS + y] += 0.04f;
        }
    }
    for (int y = 0; y < TS; y++) for (int x = 0; x < TS; x++) tput(t, x, y, sh(R_STONE, buf[x * TS + y]));
}

/* an ossuary's wall: rows of skulls between banks of long bones laid end out, the way the old charnel
 * houses stacked them; vseed crosses two long bones in a bank and leaves a skull broken */
static void bone_knob(int t, float cx, float cy, float bright) {
    for (int y = (int)cy - 4; y <= (int)cy + 4; y++)
        for (int x = (int)cx - 4; x <= (int)cx + 4; x++) {
            float dx = (x + 0.5f - cx) / 2.9f, dy = (y + 0.5f - cy) / 3.1f, d = dx * dx + dy * dy;
            if (d > 1.0f) continue;
            float v = bright - 0.35f * d - 0.12f * (dx + dy);
            if (fabsf(dx) < 0.18f && dy > 0.1f) v -= 0.25f;             /* the notch between the two knuckles */
            tput(t, x, y, sh(R_BONE, v));
        }
}
static void gen_bonewall(int t, int m, uint32_t seed, uint32_t vseed) {
    for (int y = 0; y < TS; y++)
        for (int x = 0; x < TS; x++) { tput(t, x, y, sh(R_BONE, 0.03f + 0.06f * fbm(x / 4.0f, y / 4.0f, 16, seed))); g_mid[m][x * TS + y] = 0; }
    int broke = vseed ? (int)(hsh(4, 1, vseed) % 8) : -1;
    for (int row = 0; row < 4; row++) {
        int y0 = row * 16;
        if (!(row & 1)) {
            for (int k = 0; k < 4; k++) {
                int n = (row / 2) * 4 + k;
                tex_skull(t, k * 16 + (row == 2 ? 8 : 0), y0, 0.70f + 0.18f * h01(n, row, seed) - (n == broke ? 0.22f : 0.0f), 0);
                if (n == broke)                                     /* a crack across the brow */
                    for (int i = 0; i < 8; i++) tput(t, k * 16 + (row == 2 ? 8 : 0) + 4 + i, y0 + 3 + (i & 1) + i / 3, sh(R_BONE, 0.08f));
            }
        } else {
            for (int sub = 0; sub < 2; sub++)
                for (int k = 0; k < 10; k++) {
                    float cx = k * 6.4f + (sub ? 3.2f : 0.0f) + (row == 3 ? 1.6f : 0.0f), cy = y0 + 4.0f + sub * 7.5f;
                    if (vseed && row == 1 && cx > 20.0f && cx < 44.0f) continue;
                    bone_knob(t, cx, cy, 0.62f + 0.22f * h01(k + sub * 10, row, seed));
                }
            if (vseed && row == 1)                                  /* two long bones crossed in the bank */
                for (int b = 0; b < 2; b++)
                    for (int i = 0; i <= 22; i++) {
                        float x = 21.0f + i, y = y0 + 2.0f + (b ? 22 - i : i) * 0.55f;
                        for (int w = -1; w <= 1; w++) tput(t, (int)x, (int)y + w, sh(R_BONE, 0.62f - 0.12f * w - (i == 0 || i == 22 ? 0.0f : 0.06f)));
                        if (i == 0 || i == 22) { bone_knob(t, x, y, 0.75f); }
                    }
        }
    }
}

/* ---- blends: one material giving way to the next across a cell, the first on its left and in front ---- */

/* Whole blocks or stones of the first go where the second shows; a skin (plaster) frays instead, its lip
 * catching the light. Where the first stands in front, the second takes its shadow. Both edges of the cell
 * match the plain materials, so a blend sits between them without a seam. */
static void gen_blend(int t, int a, int b, int skin, uint32_t seed) {
    const uint8_t *A = crawl_tex[T_MATV(a, 0)], *B = crawl_tex[T_MATV(b, 0)];
    const uint16_t *ida = g_mid[a];
    static uint8_t mask[TS * TS];
    static float sum[200][2]; static int cnt[200][2], mnx[200], mxx[200];
    for (int i = 0; i < 200; i++) { sum[i][0] = sum[i][1] = 0; cnt[i][0] = cnt[i][1] = 0; mnx[i] = 64; mxx[i] = -1; }
    for (int x = 0; x < TS; x++)
        for (int y = 0; y < TS; y++) { int id = ida[x * TS + y]; if (id && id < 200) { if (x < mnx[id]) mnx[id] = x; if (x > mxx[id]) mxx[id] = x; } }
    for (int x = 0; x < TS; x++)
        for (int y = 0; y < TS; y++) {
            int id = ida[x * TS + y];
            if (!id || id >= 200) continue;
            int half = mnx[id] < 8 && mxx[id] > 56 && x < 32;        /* a block across the cell's edge counts as two */
            sum[id][half] += x + 0.5f; cnt[id][half]++;
        }
    for (int x = 0; x < TS; x++)
        for (int y = 0; y < TS; y++) {
            float g = (x + 0.5f) / 64.0f, env = sinf(PI_F * g);
            int id = ida[x * TS + y], bm;
            if (!skin && id && id < 200) {
                int half = mnx[id] < 8 && mxx[id] > 56 && x < 32;
                float c = sum[id][half] / (64.0f * (float)cnt[id][half]);
                bm = c < 0.12f ? 0 : c > 0.88f ? 1 : c + (h01(id * 2 + half, 7, seed) - 0.5f) * 0.55f * sinf(PI_F * c) > 0.5f;
            } else {
                float n = fbm(x / 8.0f, y / 8.0f, 8, seed) - 0.5f + 0.4f * (fbm(x / 4.0f, y / 4.0f, 16, seed + 1) - 0.5f);
                bm = g + env * n * (skin ? 0.75f : 0.9f) > 0.5f;
            }
            mask[x * TS + y] = (uint8_t)bm;
        }
    if (!skin)                                                       /* a joint follows the blocks beside it */
        for (int x = 0; x < TS; x++)
            for (int y = 0; y < TS; y++) {
                if (ida[x * TS + y]) continue;
                int votes[2] = { 0, 0 };
                for (int d = 0; d < 4; d++) {
                    int nx = x + (d == 0) - (d == 1), ny = (y + (d == 2) - (d == 3)) & 63;
                    if (nx < 0 || nx > 63 || !ida[nx * TS + ny]) continue;
                    votes[mask[nx * TS + ny]]++;
                }
                if (votes[0] || votes[1]) mask[x * TS + y] = votes[1] > votes[0];
            }
    #define MB(xx, yy) ((xx) < 0 ? 0 : (xx) > 63 ? 1 : mask[(xx) * TS + ((yy) & 63)])
    for (int x = 0; x < TS; x++)
        for (int y = 0; y < TS; y++) {
            int bm = mask[x * TS + y];
            uint8_t c = bm ? B[x * TS + y] : A[x * TS + y];
            if (bm) {                                                /* in the shadow of what stands in front */
                if (!MB(x - 1, y) || !MB(x, y - 1) || !MB(x - 1, y - 1)) c = darker(c, 4);
                else if (!MB(x - 2, y) || !MB(x, y - 2) || !MB(x - 2, y - 1)) c = darker(c, 2);
                else if (!MB(x + 1, y) || !MB(x, y + 1)) c = darker(c, 1);
            } else if (MB(x + 1, y) || MB(x, y + 1) || MB(x - 1, y) || MB(x, y - 1)) {
                c = skin ? ((MB(x - 1, y) || MB(x, y - 1)) ? lighter(c, 2) : darker(c, 1)) : darker(c, 1);
            }
            crawl_tex[t][x * TS + y] = c;
        }
    #undef MB
}

/* the plaster falling away with the years, the rubble behind it showing */
static void plaster_flake(int tp, int tr, float dk, uint32_t seed) {
    if (dk <= 0.05f) return;
    static uint8_t lost[TS * TS];
    for (int x = 0; x < TS; x++)
        for (int y = 0; y < TS; y++) {
            float f = fbm(x / 8.0f, y / 8.0f, 8, seed) + 0.25f * (fbm(x / 4.0f, y / 4.0f, 16, seed + 1) - 0.5f) - (y > 48 ? 0.08f : 0.0f);
            lost[x * TS + y] = f < 0.14f + 0.36f * dk;
        }
    #define LO(xx, yy) lost[((xx) & 63) * TS + ((yy) & 63)]
    for (int x = 0; x < TS; x++)
        for (int y = 0; y < TS; y++) {
            uint8_t *p = &crawl_tex[tp][x * TS + y];
            if (LO(x, y)) {
                uint8_t c = crawl_tex[tr][x * TS + y];
                if (!LO(x - 1, y) || !LO(x, y - 1)) c = darker(c, 4); else if (!LO(x - 2, y) || !LO(x, y - 2)) c = darker(c, 2);
                *p = c;
            } else if (LO(x + 1, y) || LO(x, y + 1)) *p = lighter(*p, 2);
        }
    #undef LO
}

/* rot for the run's walls: the grime and moss follow one field shared by every texture of the run, so they
 * run on across a seam; the drips, cracks and chips are each texture's own */
static void decay_wall(int t, float dk, uint32_t field, uint32_t own) {
    if (dk <= 0.0f) return;
    uint8_t *p = crawl_tex[t];
    for (int y = 0; y < TS; y++)
        for (int x = 0; x < TS; x++) {
            uint8_t c = p[x * TS + y];
            if (c == C_CLEAR || IS_FULLBRIGHT(c)) continue;
            float n = fbm(x / 8.0f, y / 8.0f, 8, field + 5);
            if (n < 0.22f + 0.3f * dk) c = darker(c, 1 + (n < 0.2f + 0.15f * dk));
            if (dk > 0.25f && y > 40 && fbm(x / 8.0f, y / 4.0f, 8, field + 9) > 0.8f - 0.25f * dk - (y - 40) * 0.006f)
                c = sh(R_MOSS, 0.2f + 0.35f * h01(x, y, field));
            p[x * TS + y] = c;
        }
    #define DK(xx, yy, by) do { uint8_t *q_ = &p[((xx) & 63) * TS + ((yy) & 63)]; if (*q_ != C_CLEAR && !IS_FULLBRIGHT(*q_)) *q_ = darker(*q_, by); } while (0)
    int drips = (int)(dk * 6.0f);
    for (int k = 0; k < drips; k++) {
        int x = (int)(h01(k, 1, own) * 63), len = 10 + (int)(h01(k, 2, own) * 40.0f * dk);
        for (int y = 0; y < len; y++) DK(x, y, 2);
    }
    int cracks = (int)(dk * 5.0f + 0.5f);
    for (int k = 0; k < cracks; k++) {
        float x = h01(k, 3, own) * 64.0f, y = h01(k, 4, own) * 64.0f, a = h01(k, 5, own) * TAU_F;
        int len = 8 + (int)(h01(k, 6, own) * 24.0f);
        for (int i = 0; i < len; i++) { a += (h01(k, i + 10, own) - 0.5f) * 1.2f; x += cosf(a); y += sinf(a); DK((int)x, (int)y, 6); }
    }
    #undef DK
}

/* ---- what stands on the Crypts' walls (C_CLEAR elsewhere) ---- */

static int arch_in(float x, float y, float x0, float y0, float w, float h, float rise) {   /* an arched opening */
    if (x < x0 || x >= x0 + w || y >= y0 + h) return 0;
    if (y >= y0 + rise) return 1;
    float dx = (x - (x0 + w * 0.5f)) / (w * 0.5f), dy = (y - (y0 + rise)) / rise;
    return dx * dx + dy * dy <= 1.0f;
}
static void cut_arch_r(float x0, float y0, float w, float h, float rise, int sill, uint32_t seed, int ramp, float lift) {   /* an opening with dressed stones round it, in that ramp */
    for (int y = (int)y0 - 4; y < (int)(y0 + h) + 4; y++)
        for (int x = (int)x0 - 4; x < (int)(x0 + w) + 4; x++) {
            float px = x + 0.5f, py = y + 0.5f;
            if (arch_in(px, py, x0, y0, w, h, rise)) { cv_px(x, y, sh(R_STONE, 0.02f + 0.06f * (py - y0) / h)); continue; }
            int near = 0;
            for (int k = 0; k < 12 && !near; k++) { float a = k * TAU_F / 12.0f; near = arch_in(px + 2.6f * cosf(a), py + 2.6f * sinf(a), x0, y0, w, h, rise); }
            if (!near || py >= y0 + h) continue;
            float cx = x0 + w * 0.5f, ang = atan2f(py - (y0 + rise), px - cx);
            float v = 0.6f + 0.08f * (fbm(x / 4.0f, y / 4.0f, 16, seed) - 0.5f) - 0.06f * ((px - cx) / w) - 0.05f * ((py - y0) / h);
            if (py < y0 + rise && fmodf(ang + TAU_F, 0.45f) < 0.07f) v = 0.25f;      /* the joints between the arch's stones */
            if (py >= y0 + rise && ((int)(py - y0 - rise) % 8) == 0) v = 0.25f;
            cv_px(x, y, sh(ramp, v + lift));
        }
    if (sill)
        for (int x = (int)x0 - 4; x < (int)(x0 + w) + 4; x++) {
            cv_px(x, (int)(y0 + h), sh(ramp, 0.74f + lift)); cv_px(x, (int)(y0 + h) + 1, sh(ramp, 0.48f + lift)); cv_px(x, (int)(y0 + h) + 2, sh(ramp, 0.2f + lift));
        }
}
static void cut_arch(float x0, float y0, float w, float h, float rise, int sill, uint32_t seed) { cut_arch_r(x0, y0, w, h, rise, sill, seed, R_FLOOR, 0.12f); }
static void gen_feat_niche(int t, uint32_t seed) {                 /* four burial niches, a skull or a shroud in each */
    cv_begin(cv_sprite, TS, TS);
    for (int ny = 0; ny < 2; ny++)
        for (int nx = 0; nx < 2; nx++) {
            float x0 = nx * 32 + 7.0f, y0 = ny * 32 + 6.0f, w = 18.0f, h = 19.0f, cx = x0 + w * 0.5f;
            cut_arch(x0, y0, w, h, 7.0f, 1, seed);
            int k = (int)(hsh(nx, ny, seed) % 3);
            if (k == 0) cv_skull(cx, y0 + h - 5.5f, 0.8f, 0.3f, 0.82f);
            else if (k == 1) { cv_ellipse(cx, y0 + h - 4.0f, 7.5f, 3.8f, R_WARM, 0.12f, 0.5f); cv_skull(cx - 5.0f, y0 + h - 5.0f, 0.55f, 0.25f, 0.6f); }
            else { cv_bone(cx - 6, y0 + h - 1.5f, cx + 5, y0 + h - 2.5f, 1.0f, 0.25f, 0.7f); cv_skull(cx + 3.0f, y0 + h - 4.5f, 0.6f, 0.25f, 0.7f); }
        }
    cv_to_tex(t);
}
static void gen_feat_banner(int t, uint32_t seed, float tatter) {  /* the Order's banner: a cross on red, torn with the years */
    cv_begin(cv_sprite, TS, TS);
    for (int x = 10; x <= 53; x++) { cv_px(x, 3, sh(R_IRON, 0.75f)); cv_px(x, 4, sh(R_IRON, 0.35f)); }
    cv_ellipse(10, 3.5f, 1.8f, 1.8f, R_IRON, 0.3f, 0.9f); cv_ellipse(53, 3.5f, 1.8f, 1.8f, R_IRON, 0.3f, 0.9f);
    for (int x = 15; x <= 48; x++) {
        float hem = 52.0f + 3.0f * sinf(x * 0.7f) - 3.0f * fabsf(sinf(x * 1.9f)) - tatter * (5.0f + 10.0f * h01(x / 3, 1, seed));
        if (((x - 15) % 11) == 5) hem += 4.0f - tatter * 2.0f;                 /* the swallowtails */
        for (int y = 5; y <= (int)hem; y++) {
            float v = 0.40f + 0.16f * sinf((x - 15) * 0.55f) - 0.003f * y + fbm(x / 4.0f, y / 4.0f, 16, seed) * 0.12f;
            uint8_t c = sh(R_BLOOD, v);
            if (x <= 16 || x >= 47 || y <= 6) c = sh(R_GOLD, 0.55f + 0.2f * sinf(y * 0.9f));
            float dx = x + 0.5f - 32.0f, dy = y + 0.5f - 26.0f;
            int arm = (fabsf(dy) <= 11.0f && fabsf(dx) <= 1.6f + 0.42f * fabsf(dy)) || (fabsf(dx) <= 10.0f && fabsf(dy) <= 1.6f + 0.42f * fabsf(dx));
            if (arm) c = sh(R_BONE, 0.82f - 0.012f * (dx + dy) - (fabsf(dx) > 9.0f || fabsf(dy) > 10.0f ? 0.15f : 0.0f));
            if (tatter > 0.0f && fbm(x / 4.0f, y / 4.0f, 16, seed + 9) > 1.02f - 0.32f * tatter) continue;   /* holes, the wall behind */
            if (tatter > 0.55f && fabsf((x - 21) - (y - 14) * 0.45f) < 0.6f && y > 14 && y < 38) continue;  /* a slash */
            cv_px(x, y, c);
        }
    }
    cv_skull(32, 26.5f, 0.42f, 0.35f, 0.85f);
    cv_to_tex(t);
}
static void gen_feat_slab(int t, uint32_t seed) {                  /* a knight's slab set in the wall: shield on sword */
    cv_begin(cv_sprite, TS, TS);
    for (int y = 3; y <= 60; y++)
        for (int x = 13; x <= 50; x++) {
            float v = 0.52f + 0.1f * (fbm(x / 4.0f, y / 4.0f, 16, seed) - 0.5f);
            int bx = x - 13, by = y - 3, bw = 37, bh = 57;
            if (bx < 3 || by < 3 || bx > bw - 3 || by > bh - 3) v += (bx == 0 || by == 0) ? 0.18f : (bx == bw || by == bh) ? -0.2f : 0.06f;
            else if (bx == 3 || by == 3) v -= 0.16f; else if (bx == bw - 3 || by == bh - 3) v += 0.1f;
            float dx = x + 0.5f - 32.0f;
            if (fabsf(dx) <= 1.6f && y >= 6 && y <= 57) v = 0.7f - (dx > 0.8f ? 0.15f : 0.0f) - (y > 52 ? (y - 52) * 0.05f : 0.0f);   /* the sword */
            if (y >= 10 && y <= 11 && fabsf(dx) <= 10.5f) v = y == 10 ? 0.78f : 0.4f;   /* its guard */
            if (y >= 5 && y <= 7 && fabsf(dx) <= 2.2f) v = 0.72f;
            float sy = y + 0.5f, hw = sy < 26 ? 10.0f : 10.0f * sqrtf(fmaxf(0.0f, 1.0f - (sy - 26) * (sy - 26) / 196.0f));
            if (sy >= 14 && sy <= 40 && fabsf(dx) <= hw) {                            /* the shield over it, with its cross */
                v = 0.64f - 0.015f * dx - 0.005f * (sy - 14);
                if (fabsf(dx) > hw - 1.2f || sy < 15.2f) v = 0.42f;
                if ((fabsf(dx) < 1.5f && sy > 16 && sy < 37) || (fabsf(sy - 24.0f) < 1.5f && fabsf(dx) < hw - 2.5f)) v = 0.82f;
            }
            cv_px(x, y, sh(R_FLOOR, v + 0.12f));
        }
    cv_to_tex(t);
}
static void gen_feat_shrine(int t, uint32_t seed) {                /* a wall shrine: a candle burning by a skull */
    cv_begin(cv_sprite, TS, TS);
    cut_arch(22, 12, 20, 32, 10, 1, seed);
    for (int y = 12; y < 26; y++) for (int x = 24; x < 40; x++) if (cvp[y * TS + x] != C_CLEAR && arch_in(x + 0.5f, y + 0.5f, 22, 12, 20, 32, 10)) cv_px(x, y, sh(R_STONE, 0.01f));   /* soot */
    cv_skull(35, 39, 0.62f, 0.3f, 0.8f);
    for (int y = 33; y <= 43; y++) for (int x = 26; x <= 28; x++) cv_px(x, y, sh(R_BONE, 0.82f - (x - 26) * 0.18f));
    cv_px(25, 42, sh(R_BONE, 0.6f)); cv_px(29, 40, sh(R_BONE, 0.55f)); cv_px(29, 41, sh(R_BONE, 0.5f));
    for (int y = 27; y <= 32; y++) for (int x = 25; x <= 29; x++) {
        float u = (x + 0.5f - 27.5f) / (1.2f + (y - 27) * 0.25f);
        if (fabsf(u) <= 1.0f) cv_px(x, y, (uint8_t)(R_FLAME + clampi(15 - (int)(fabsf(u) * 6) - (32 - y), 6, 15)));
    }
    cv_to_tex(t);
}
static void gen_feat_tablet(int t, uint32_t seed) {                /* a memorial tablet, lettered, pinned with iron */
    cv_begin(cv_sprite, TS, TS);
    for (int y = 14; y <= 47; y++)
        for (int x = 15; x <= 48; x++) {
            int bx = x - 15, by = y - 14;
            float v = 0.68f + 0.08f * (fbm(x / 4.0f, y / 4.0f, 16, seed) - 0.5f);
            if (bx == 0 || by == 0) v += 0.15f; else if (bx == 33 || by == 33) v -= 0.25f; else if (bx == 2 || by == 2) v -= 0.12f;
            if ((y >= 18 && y <= 23 && x == 31) || (y == 20 && x >= 29 && x <= 33)) v = 0.32f;   /* a small cross */
            int row = (y - 27) % 4;
            if (y >= 27 && y <= 43 && row == 0 && x >= 19 && x <= 44) {                        /* the lettering */
                int g = (x - 19) / 3;
                if (h01(g, y, seed + 1) > 0.25f && ((x - 19) % 3) != 2) v = 0.36f;
            }
            if (y >= 28 && y <= 44 && row == 1 && x >= 19 && x <= 44 && ((x - 19) % 3) != 2 && h01((x - 19) / 3, y - 1, seed + 1) > 0.25f) v += 0.06f;
            cv_px(x, y, sh(R_FLOOR, v + 0.1f));
        }
    for (int k = 0; k < 4; k++) cv_ellipse(k & 1 ? 45.5f : 18.5f, k & 2 ? 44.5f : 17.5f, 1.3f, 1.3f, R_IRON, 0.25f, 0.85f);
    cv_to_tex(t);
}
static void gen_feat_torch(int t, int frame, uint32_t seed) {      /* an iron sconce and its torch, on whatever wall */
    cv_begin(cv_sprite, TS, TS);
    for (int y = 38; y <= 47; y++)
        for (int x = 27; x <= 36; x++) {
            float v = 0.42f + ((x == 27 || y == 38) ? 0.2f : 0.0f) - ((x == 36 || y == 47) ? 0.2f : 0.0f);
            if ((x == 29 || x == 34) && (y == 40 || y == 45)) v = 0.9f;
            cv_px(x, y, sh(R_IRON, v));
        }
    for (int y = 33; y < 38; y++) for (int x = 30; x <= 33; x++) cv_px(x, y, sh(R_IRON, 0.55f - (x - 30) * 0.08f));
    for (int x = 28; x <= 35; x++) { cv_px(x, 30, sh(R_IRON, 0.7f)); cv_px(x, 31, sh(R_IRON, 0.35f)); }   /* the ring that holds it */
    for (int y = 20; y <= 35; y++)
        for (int x = 30; x <= 33; x++) if (y < 30 || y > 31) cv_px(x, y, sh(R_WOOD, 0.5f - fabsf(x - 31.0f) * 0.12f - (y - 20) * 0.01f));
    static const float SWAY[3] = { 0.0f, 1.6f, -1.3f }, TALL[3] = { 1.0f, 0.9f, 1.1f };
    for (int y = 0; y <= 22; y++)
        for (int x = 20; x <= 44; x++) {
            float u = (22 - y) / (21.0f * TALL[frame]);
            if (u > 1.0f) continue;
            float cx = 31.5f + SWAY[frame] * u * u * 2.0f;
            float w = 5.6f * powf(fmaxf(0.0f, sinf(PI_F * (0.18f + 0.82f * (1.0f - u)))), 0.8f) * (1.0f - 0.35f * u);
            if (w < 0.3f) continue;
            float d = fabsf(x + 0.5f - cx) / (w + 0.01f), n = fbm(x / 4.0f, y / 4.0f + frame * 3.0f, 16, seed + 90 + frame) * 0.5f;
            if (d > 1.0f - 0.25f * n) continue;
            cv_px(x, y, (uint8_t)(R_FLAME + clampi((int)((1.0f - d * 0.7f - u * 0.55f + n * 0.3f) * 15.0f), 1, 15)));
        }
    cv_to_tex(t);
}

/* ---- the Crypts' floors (each cell its own, so nothing crosses a cell's edge) ---- */

static void gen_floor_flags(int t, uint32_t seed, int broken) {     /* flagstones, laid four to a side, some doubled */
    int id[4][4], ox[16], oy[16], fw[16], fh[16], n = 0;
    memset(id, -1, sizeof id);
    for (int uy = 0; uy < 4; uy++)
        for (int ux = 0; ux < 4; ux++) {
            if (id[uy][ux] >= 0) continue;
            float r = h01(ux, uy, seed);
            int w = 1, h = 1;
            if (r < 0.25f && ux < 3 && uy < 3 && id[uy][ux + 1] < 0 && id[uy + 1][ux] < 0 && id[uy + 1][ux + 1] < 0) w = h = 2;
            else if (r < 0.55f && ux < 3 && id[uy][ux + 1] < 0) w = 2;
            else if (r < 0.8f && uy < 3 && id[uy + 1][ux] < 0) h = 2;
            for (int j = 0; j < h; j++) for (int i = 0; i < w; i++) id[uy + j][ux + i] = n;
            ox[n] = ux * 16; oy[n] = uy * 16; fw[n] = w * 16; fh[n] = h * 16; n++;
        }
    int gone = broken ? (int)(hsh(5, 1, seed) % (uint32_t)n) : -1;
    for (int y = 0; y < TS; y++)
        for (int x = 0; x < TS; x++) {
            int f = id[y / 16][x / 16], px = x - ox[f], py = y - oy[f];
            uint8_t c;
            if (px == 0 || py == 0 || px == fw[f] - 1 || py == fh[f] - 1) c = sh(R_FLOOR, 0.1f + 0.06f * h01(x, y, seed + 1));
            else {
                float cx = (px - fw[f] * 0.5f) / (fw[f] * 0.5f), cy = (py - fh[f] * 0.5f) / (fh[f] * 0.5f);
                float v = 0.46f + 0.07f * (2.0f * h01(f, 3, seed) - 1.0f) + 0.14f * (fbm(x / 8.0f, y / 8.0f, 8, seed + 2) - 0.5f)
                        + 0.05f * (1.0f - fmaxf(fabsf(cx), fabsf(cy))) + (px == 1 || py == 1 ? 0.07f : 0.0f) - (px == fw[f] - 2 || py == fh[f] - 2 ? 0.07f : 0.0f);
                if (h01(x, y, seed + 4) > 0.985f) v -= 0.12f;
                c = sh(R_FLOOR, v + 0.12f);
                if (f == gone && fbm(x / 4.0f, y / 4.0f, 16, seed + 6) > 0.42f) c = sh(R_FLOOR, 0.14f + 0.1f * h01(x, y, seed + 7));   /* broken out: the earth below */
            }
            tput(t, x, y, c);
        }
    if (broken)                                                     /* cracks across the rest */
        for (int k = 0; k < 4; k++) {
            float x = h01(k, 8, seed) * 64.0f, y = h01(k, 9, seed) * 64.0f, a = h01(k, 10, seed) * TAU_F;
            for (int i = 0; i < 22; i++) {
                a += (h01(k, i + 11, seed) - 0.5f) * 1.0f; x += cosf(a); y += sinf(a);
                if (x < 1 || y < 1 || x > 62 || y > 62) break;
                crawl_tex[t][(int)x * TS + (int)y] = darker(crawl_tex[t][(int)x * TS + (int)y], 5);
            }
        }
}
static void gen_floor_chapter(int t, uint32_t seed) {              /* the chapter house: pale and dark stone set lozenge-wise */
    for (int y = 0; y < TS; y++)
        for (int x = 0; x < TS; x++) {
            int a = (x + y) & 31, b = (x - y + 64) & 31, dark = (((x + y) >> 5) + ((x - y + 64) >> 5)) & 1;
            float v = dark ? 0.3f : 0.58f;
            v += 0.12f * (fbm(x / 8.0f, y / 8.0f, 8, seed + dark) - 0.5f) + 0.04f * (fbm(x / 2.0f, y / 2.0f, 32, seed + 3) - 0.5f);
            uint8_t c = dark ? sh(R_STONE, v) : sh(R_FLOOR, v + 0.14f);
            if (a == 0 || b == 0) c = sh(R_FLOOR, 0.12f);
            tput(t, x, y, c);
        }
}
static void gen_floor_setts(int t, uint32_t seed) {                /* a hall's setts, worn round on top */
    for (int y = 0; y < TS; y++)
        for (int x = 0; x < TS; x++) {
            int r = y / 8, ly = y & 7, s = x / 8, lx = x & 7;
            int pair = h01(r, s / 2, seed) < 0.4f;                    /* some doubled, never across the edge */
            int w = pair ? 16 : 8, px = pair ? (x & 15) : lx;
            int sid = r * 8 + (pair ? (s & ~1) : s);
            uint8_t c;
            if (px == 0 || ly == 0) c = sh(R_FLOOR, 0.1f + 0.08f * h01(x, y, seed + 1));
            else {
                float u = (px + 0.5f - w * 0.5f) / (w * 0.5f), v = (ly + 0.5f - 4.0f) / 4.0f;
                float val = 0.44f + 0.06f * (2.0f * h01(sid, 3, seed) - 1.0f) + 0.09f * (1.0f - u * u - v * v) - 0.04f * (u + v)
                          + 0.07f * (fbm(x / 4.0f, y / 4.0f, 16, seed + 2) - 0.5f);
                c = sh(h01(sid, 4, seed) < 0.12f ? R_STONE : R_FLOOR, val + (h01(sid, 4, seed) < 0.12f ? 0.0f : 0.12f));
            }
            tput(t, x, y, c);
        }
}
static void gen_floor_earth(int t, uint32_t seed) {                /* an ossuary's floor: earth trodden hard, bone in it */
    for (int y = 0; y < TS; y++)
        for (int x = 0; x < TS; x++) {
            float v = 0.3f + 0.16f * (fbm(x / 8.0f, y / 8.0f, 8, seed) - 0.5f) + 0.08f * (fbm(x / 2.0f, y / 2.0f, 32, seed + 1) - 0.5f);
            uint8_t c = sh(R_FLOOR, v);
            if (h01(x, y, seed + 2) > 0.93f) c = sh(R_FLOOR, v + 0.2f);
            tput(t, x, y, c);
        }
    for (int k = 0; k < 14; k++) {                                   /* splinters of bone, a knuckle, a tooth */
        float x = 3 + h01(k, 3, seed) * 58, y = 3 + h01(k, 4, seed) * 58, a = h01(k, 5, seed) * PI_F;
        int len = 1 + (int)(h01(k, 6, seed) * 5.0f);
        for (int i = 0; i <= len; i++) tput(t, (int)(x + cosf(a) * i), (int)(y + sinf(a) * i), sh(R_BONE, 0.5f + 0.2f * h01(k, i, seed)));
        tput(t, (int)(x + sinf(a)), (int)(y - cosf(a)), sh(R_FLOOR, 0.12f));
    }
}

/* ---- the Crypts' ceilings ---- */

static float vault_stones(int x, int y, uint32_t seed, int cell) {  /* small stones in a vault's web */
    float d1 = 1e9f, d2 = 1e9f; int n1 = 0;
    for (int i = 0; i < 40; i++) {
        float px = h01(i, 1, seed) * 64, py = h01(i, 2, seed) * 64, dx = wrapd(x + 0.5f - px), dy = wrapd(y + 0.5f - py);
        float d = sqrtf(dx * dx * (cell ? 1.0f : 0.5f) + dy * dy);
        if (d < d1) { d2 = d1; d1 = d; n1 = i; } else if (d < d2) d2 = d;
    }
    if (d2 - d1 < 1.0f) return 0.1f;
    return 0.42f + 0.14f * (h01(n1, 3, seed) - 0.5f) + 0.1f * (fbm(x / 4.0f, y / 4.0f, 16, seed + 4) - 0.5f);
}
static void gen_ceil_rib(int t, uint32_t seed) {                   /* a rib vault over a room: ribs on the cell's edges and diagonals, a boss */
    for (int y = 0; y < TS; y++)
        for (int x = 0; x < TS; x++) {
            float v = vault_stones(x, y, seed, 1);
            int e = x < 2 || x > 61 || y < 2 || y > 61, e2 = x == 2 || x == 61 || y == 2 || y == 61;
            float dd = fminf(fabsf((float)x - y), fabsf((float)x + y - 63.0f));
            if (dd < 1.6f) v = 0.62f; else if (dd < 2.6f) v = 0.22f;
            if (e) v = 0.66f; else if (e2) v = 0.2f;
            float bx = x + 0.5f - 32.0f, by = y + 0.5f - 32.0f, br = sqrtf(bx * bx + by * by);
            if (br < 5.5f) { float a = atan2f(by, bx); v = 0.4f + 0.4f * (1.0f - br / 5.5f) + (fmodf(a + TAU_F, TAU_F / 6.0f) < 0.18f && br > 1.5f ? -0.25f : 0.0f); }
            else if (br < 6.5f) v = 0.18f;
            tput(t, x, y, sh(R_CEIL, v + 0.08f));
        }
}
static void gen_ceil_vault(int t, uint32_t seed) {                 /* a hall's barrel vault: rough voussoirs, soot from the torches */
    for (int y = 0; y < TS; y++)
        for (int x = 0; x < TS; x++) {
            float v = vault_stones(x, y, seed, 0) - 0.12f * fmaxf(0.0f, fbm(x / 8.0f, y / 8.0f, 8, seed + 5) - 0.55f) * 3.0f;
            tput(t, x, y, sh(R_CEIL, v));
        }
}
static void gen_ceil_rock(int t, uint32_t seed) {                  /* rough rock, left as it was cut */
    for (int y = 0; y < TS; y++)
        for (int x = 0; x < TS; x++) {
            float v = 0.46f + 0.22f * (fbm(x / 8.0f, y / 8.0f, 8, seed) - 0.5f) + 0.1f * (fbm(x / 2.0f, y / 2.0f, 32, seed + 1) - 0.5f);
            if (fabsf(fbm(x / 8.0f, y / 8.0f, 8, seed + 2) - 0.5f) < 0.02f) v -= 0.16f;
            tput(t, x, y, sh(R_CEIL, v));
        }
}
static void decay_flat(int t, float dk, uint32_t seed) {           /* rot on a floor or ceiling: grime, damp, cracks */
    if (dk <= 0.0f) return;
    for (int y = 0; y < TS; y++)
        for (int x = 0; x < TS; x++) {
            uint8_t c = crawl_tex[t][x * TS + y];
            float n = fbm(x / 8.0f, y / 8.0f, 8, seed + 5);
            if (n < 0.2f + 0.3f * dk) c = darker(c, 1 + (n < 0.18f + 0.15f * dk));
            if (dk > 0.4f && fbm(x / 4.0f, y / 4.0f, 16, seed + 6) > 0.86f - 0.18f * dk && (c - 1) % 16 < 4) c = sh(R_MOSS, 0.15f + 0.2f * h01(x, y, seed));
            crawl_tex[t][x * TS + y] = c;
        }
    int cracks = (int)(dk * 3.0f + 0.5f);
    for (int k = 0; k < cracks; k++) {
        float x = h01(k, 3, seed) * 64.0f, y = h01(k, 4, seed) * 64.0f, a = h01(k, 5, seed) * TAU_F;
        for (int i = 0; i < 16; i++) {
            a += (h01(k, i + 10, seed) - 0.5f) * 1.2f; x += cosf(a); y += sinf(a);
            if (x < 1 || y < 1 || x > 62 || y > 62) break;
            crawl_tex[t][(int)x * TS + (int)y] = darker(crawl_tex[t][(int)x * TS + (int)y], 4);
        }
    }
}

/* ---- the Crypts' doors and keys ---- */

static void gen_rdoor(int t, uint32_t seed, int metal) {           /* oak under iron; a vault's straps and plate in its key's metal */
    int vault = metal >= 0;
    for (int y = 0; y < TS; y++)
        for (int x = 0; x < TS; x++) {
            int p = x / 8, lx = x % 8;
            float v = (vault ? 0.32f : 0.42f) + 0.13f * h01(p, 0, seed) + 0.06f * sinf(y * 0.7f + p * 3.0f + x * 0.25f) + fbm(x / 4.0f, y / 16.0f, 16, seed) * 0.12f;
            if (lx == 0) v = 0.05f; else if (lx == 1) v += 0.08f; else if (lx == 7) v -= 0.08f;
            tput(t, x, y, sh(R_WOOD, v));
        }
    int strap = vault ? metal : R_IRON;
    float lo = strap == R_GOLD ? 0.0f : 0.2f, k = strap == R_GOLD ? 1.0f : 0.8f;
    for (int band = 0; band < (vault ? 3 : 2); band++) {
        int y0 = vault ? 6 + band * 24 : (band ? 50 : 10), x1 = vault ? 63 : 46;
        for (int y = y0; y < y0 + (vault ? 4 : 3); y++)
            for (int x = 0; x <= x1; x++) {
                float v = 0.5f + (y == y0 ? 0.3f : 0.0f) - (y == y0 + (vault ? 3 : 2) ? 0.3f : 0.0f);
                if (x % 7 == 3 && y == y0 + 1) v = 1.0f;
                tput(t, x, y, sh(strap, lo + v * k));
            }
        if (!vault)                                                  /* the strap ends in a split curl */
            for (int a = 0; a < 18; a++) {
                float ang = a / 18.0f * PI_F * 1.5f;
                tput(t, x1 + 1 + (int)(3.0f * sinf(ang)), y0 + 1 - (int)(3.0f * cosf(ang)), sh(R_IRON, 0.6f));
                tput(t, x1 + 1 + (int)(3.0f * sinf(ang)), y0 + 1 + (int)(3.0f * cosf(ang)), sh(R_IRON, 0.45f));
            }
    }
    if (!vault) {
        for (int r = 0; r < 3; r++) for (int p = 0; p < 8; p++) { int x = p * 8 + 4, y = 22 + r * 10; if (x > 22 && x < 42 && r == 0) continue; tput(t, x, y, sh(R_IRON, 0.9f)); tput(t, x + 1, y + 1, sh(R_IRON, 0.2f)); }
        for (int y = 15; y <= 26; y++)
            for (int x = 24; x <= 39; x++) {
                int edge = x == 24 || x == 39 || y == 15 || y == 26, bar = x == 28 || x == 32 || x == 36;
                tput(t, x, y, edge ? sh(R_IRON, 0.4f) : (bar ? sh(R_IRON, 0.65f) : 0));
            }
        for (int a = 0; a < 40; a++) {
            float ang = TAU_F * a / 40;
            tput(t, 47 + (int)lroundf(4.0f * cosf(ang)), 38 + (int)lroundf(4.5f * sinf(ang)), sh(R_IRON, 0.5f + 0.35f * -sinf(ang)));
        }
    } else
        for (int y = 16; y <= 48; y++)
            for (int x = 22; x <= 41; x++) {
                int edge = x == 22 || y == 16, shade = x == 41 || y == 48;
                float v = 0.62f + (edge ? 0.3f : 0.0f) - (shade ? 0.35f : 0.0f) + fbm(x / 4.0f, y / 4.0f, 16, seed + 7) * 0.15f;
                float kx = x + 0.5f - 31.5f, ky = y + 0.5f - 34.0f;
                if (kx * kx + ky * ky < 7.0f || (fabsf(kx) < 1.2f && ky > 0.0f && ky < 9.0f)) { tput(t, x, y, 0); continue; }
                if ((fabsf(kx) < 1.0f && y >= 19 && y <= 28) || (y >= 22 && y <= 23 && fabsf(kx) < 4.0f)) v -= 0.35f;   /* the Order's cross */
                tput(t, x, y, sh(metal, lo + v * k));
            }
}
static void gen_rjamb(int t, uint32_t seed) {                      /* the doorway's side: the run's ashlar, cut narrow, the hinge pins */
    for (int y = 0; y < TS; y++)
        for (int x = 0; x < TS; x++) {
            float v = 0.46f + fbm(x / 8.0f, y / 8.0f, 8, seed) * 0.18f - 0.09f + 0.08f * h01(y / 16, 1, seed);
            if ((y & 15) == 0 || ((y & 31) < 16 ? x == 32 : x == 16 || x == 48)) v = 0.1f;
            if ((y & 15) == 1) v += 0.1f;
            if (x < 3) v += 0.12f; if (x > 60) v -= 0.12f;
            tput(t, x, y, sh(R_STONE, v));
        }
    for (int h = 0; h < 2; h++)
        for (int y = 10 + h * 40; y < 15 + h * 40; y++)
            for (int x = 0; x < 22; x++) tput(t, x, y, sh(R_IRON, 0.5f + (y == 10 + h * 40 ? 0.25f : 0.0f) + (x == 18 ? 0.3f : 0.0f)));
}
static void spr_key_trefoil(int id, float scale, int metal) {      /* a church key: a trefoil bow, a warded bit */
    cv_begin(cv_sprite, TS, TS);
    #define KX(x) (32.0f + ((x) - 32.0f) * scale)
    float ar = fmaxf(fabsf(scale), 0.18f), lo = metal == R_GOLD ? 0.35f : 0.45f;
    for (int k = 0; k < 3; k++) {
        float a = -PI_F * 0.5f + k * TAU_F / 3.0f;
        cv_ellipse(KX(20.0f + 3.6f * cosf(a)), 38.0f + 3.6f * sinf(a), 3.4f * ar, 3.4f, metal, lo, 1.0f);
    }
    cv_flat(KX(20), 38, 1.7f * ar, 1.7f, C_CLEAR);
    cv_capsule(KX(25), 38, KX(47), 38, 1.9f, metal, lo, 1.0f);
    cv_ellipse(KX(27), 38, 1.6f * ar, 2.8f, metal, 0.5f, 1.0f);
    cv_ellipse(KX(30), 38, 1.3f * ar, 2.4f, metal, 0.5f, 1.0f);
    for (int y = 38; y <= 46; y++)
        for (int x = 41; x <= 47; x++) {
            if ((x == 43 || x == 45) && y >= 42) continue;             /* the wards */
            cv_px((int)KX(x + 0.5f), y, sh(metal, lo + (1.0f - lo) * (x == 41 || y == 38 ? 0.95f : 0.6f)));
        }
    #undef KX
    cv_outline(sh(metal, 0.05f));
    cv_store(id);
}

/* ---- the Catacombs' materials: slate cut and laid, the silt the floods left, the stone the water grew ---- */

/* a tex pixel's value, kept in floats until the texture is made, so the layers can add */
static float g_v[TS * TS];
#define GV(x, y) g_v[((x) & 63) * TS + ((y) & 63)]

/* bedded slate, as it was cut: beds of every thickness, the partings between them rough and waving a
 * little, pieces broken out of the face, the cleavage running across at a slant; vseed runs a quartz vein
 * through it and leaves a patch wet */
static void gen_slate(int t, int m, uint32_t seed, uint32_t vseed) {
    float bed_y0[24], bed_v[24];
    int nb = 0;
    for (int c = 0; c < 4; c++) {                                    /* beds within each course of 16 */
        int y = c * 16;
        while (y < c * 16 + 16 && nb < 23) {
            int th = 3 + (int)(h01(c, y, seed) * 6.0f);
            if (c * 16 + 16 - (y + th) < 3) th = c * 16 + 16 - y;
            bed_y0[nb] = (float)y; bed_v[nb] = 0.31f + 0.12f * (h01(nb, 1, seed) - 0.5f);
            y += th; nb++;
        }
    }
    bed_y0[nb] = 64.0f;
    float wph = h01(9, 9, seed) * TAU_F;
    for (int y = 0; y < TS; y++)
        for (int x = 0; x < TS; x++) {
            float wave = 1.2f * sinf(x * TAU_F / 64.0f + wph) + 0.8f * (fbm(x / 8.0f, 0.5f, 8, seed + 12) - 0.5f) * 2.0f;
            float yy = y - wave + 64.0f;
            float ym = fmodf(yy, 64.0f);
            int b = 0;
            while (b + 1 < nb && ym >= bed_y0[b + 1]) b++;
            float in = ym - bed_y0[b], th = bed_y0[b + 1] - bed_y0[b];
            int course = (int)(ym / 16.0f) & 3, major = fmodf(ym, 16.0f) < 1.0f;
            float v = bed_v[b] + 0.05f * (fbm(x / 8.0f, y / 4.0f, 8, seed + 5) - 0.5f) + 0.05f * (fbm(x / 2.0f, y / 2.0f, 32, seed + 6) - 0.5f);
            if (in < 1.0f) v = major ? 0.08f : v - 0.14f;             /* the partings */
            else if (in < 2.0f) v += 0.07f;                          /* each bed's lip, catching the light */
            else if (in > th - 1.0f) v -= 0.05f;
            float cl = fmodf(x * 0.5f + y + 256.0f + 6.0f * fbm(x / 16.0f, y / 16.0f, 4, seed + 8), 7.0f);   /* the cleavage */
            if (cl < 0.7f) v -= 0.03f; else if (cl < 1.4f) v += 0.02f;
            float br = fbm(x / 8.0f, y / 8.0f, 8, seed + 13);         /* pieces broken out of the face */
            int out = br > 0.68f && !major;
            if (out) v -= 0.1f + (br > 0.74f ? 0.05f : 0.0f);
            else if (br > 0.65f) v += 0.06f;
            if (h01(x, y, seed + 10) > 0.99f) v += 0.12f;             /* wet glints */
            int id = major || in < 1.0f ? 0 : 1 + course * 3 + (int)(((x + (int)(h01(course, b, seed) * 64.0f)) & 63) / 22);
            if (vseed) {
                float vx = 10.0f + y * 0.7f + 4.0f * sinf(y * 0.19f + h01(1, 1, vseed) * 6.0f), dv = wrapd(x + 0.5f - vx);
                if (dv < 0.9f) v = 0.6f + 0.08f * h01(x, y, vseed); else if (dv < 1.6f) v += 0.08f;
                if (hypotf(wrapd(x - 44.0f), wrapd(y - 20.0f)) < 9.0f + 3.0f * fbm(x / 8.0f, y / 8.0f, 8, vseed)) v -= 0.06f;
            }
            GV(x, y) = v;
            g_mid[m][x * TS + y] = (uint16_t)id;
        }
    for (int y = 0; y < TS; y++) for (int x = 0; x < TS; x++) tput(t, x, y, sh(R_STONE, GV(x, y)));
}

/* slate lining: dressed slabs a hand thick, laid dry in courses of 16, the joints deep; vseed cuts a
 * cross in one slab and leaves one gone */
static void gen_lining(int t, int m, uint32_t seed, uint32_t vseed) {
    static const int SPLIT[4][4] = { { 5, 5, 6, 0 }, { 4, 6, 6, 0 }, { 6, 5, 5, 0 }, { 4, 4, 4, 4 } };
    int sub[64], sub_k[64], sub_h[64], sub_y0[64], ns = 0;
    for (int c = 0; c < 4; c++) {
        const int *sp = SPLIT[hsh(c, 1, seed) & 3];
        int y = c * 16;
        for (int k = 0; k < 4 && sp[k]; k++) {
            for (int i = 0; i < sp[k]; i++) { sub[y + i] = ns; sub_k[y + i] = i; sub_h[y + i] = sp[k]; sub_y0[y + i] = y; }
            y += sp[k]; ns++;
        }
    }
    int carve = vseed ? 1 + (int)(hsh(8, 1, vseed) % 30) : -1, gone = vseed ? 1 + (int)(hsh(8, 2, vseed) % 30) : -1;
    for (int y = 0; y < TS; y++) {
        int r = sub[y], off = (int)(h01(r, 3, seed) * 64.0f), x = 0;
        int sx[12], sw[12], n = 0;
        while (x < 64 && n < 12) { int w = 11 + (int)(h01(r, 10 + n, seed) * 17.0f); if (64 - x - w < 8) w = 64 - x; sx[n] = x; sw[n] = w; x += w; n++; }
        for (int xx = 0; xx < TS; xx++) {
            int lx = (xx - off) & 63, k = 0;
            while (k + 1 < n && lx >= sx[k + 1]) k++;
            int px = lx - sx[k], id = 1 + (r * 7 + k) % 180;
            float v;
            if (px == 0 || sub_k[y] == 0) { v = 0.04f + 0.04f * h01(xx, y, seed + 1); id = 0; }   /* laid dry: the joints are deep */
            else {
                v = 0.36f + 0.1f * (h01(id, 4, seed) - 0.5f) + 0.05f * (fbm(xx / 8.0f, y / 8.0f, 8, seed + 2) - 0.5f) + 0.03f * (fbm(xx / 2.0f, y / 2.0f, 32, seed + 3) - 0.5f);
                if (sub_k[y] == 1) v += 0.1f; else if (sub_k[y] == sub_h[y] - 1) v -= 0.07f;
                if (px == 1) v += 0.04f; else if (px == sw[k] - 1) v -= 0.05f;
                if (h01(xx, y, seed + 4) > 0.985f) v += 0.1f;
                if ((sw[k] - px) + (sub_h[y] - sub_k[y]) < 4 && h01(id, 5, seed) < 0.4f) v -= 0.12f;   /* a corner knocked off */
                if (id == gone) { v = 0.05f + 0.03f * h01(xx, y, seed + 6); }
                else if (id == carve) {                              /* a cross cut in the slab */
                    int cx = px - sw[k] / 2, cy = sub_k[y] - sub_h[y] / 2;
                    if ((cx == 0 && abs(cy) <= 2) || (abs(cx) <= 2 && cy == -1)) v -= 0.16f;
                }
            }
            GV(xx, y) = v;
            g_mid[m][xx * TS + y] = (uint16_t)id;
        }
        (void)sub_y0;
    }
    for (int y = 0; y < TS; y++) for (int x = 0; x < TS; x++) tput(t, x, y, sh(R_STONE, GV(x, y)));
}

/* stones in the texture, wrapping at the edges: for the silt's cobbles and the bones in it */
static void tx_blob(int m, int id, float cx, float cy, float rx, float ry, float base, int ramp, uint8_t *out, float sink) {
    for (int y = (int)floorf(cy - ry) - 1; y <= (int)ceilf(cy + ry) + 1; y++)
        for (int x = (int)floorf(cx - rx) - 1; x <= (int)ceilf(cx + rx) + 1; x++) {
            float nx = (x + 0.5f - cx) / rx, ny = (y + 0.5f - cy) / ry, d = nx * nx + ny * ny;
            if (d > 1.0f || ny > sink) continue;
            GV(x, y) = base + 0.3f * lit(nx, ny, sqrtf(1.0f - d)) - 0.1f;
            out[((x) & 63) * TS + ((y) & 63)] = (uint8_t)ramp;
            if (m >= 0) g_mid[m][((x) & 63) * TS + ((y) & 63)] = (uint16_t)id;
        }
}

/* silt and rubble: cobbles the water rolled, set in the silt it left in bands; vseed has a shell in it and a
 * long bone */
static void gen_silt(int t, int m, uint32_t seed, uint32_t vseed) {
    static uint8_t ramp[TS * TS];
    for (int y = 0; y < TS; y++)
        for (int x = 0; x < TS; x++) {
            GV(x, y) = 0.36f + 0.06f * sinf(y * 1.1f + 5.0f * fbm(x / 16.0f, y / 16.0f, 4, seed)) + 0.06f * (fbm(x / 8.0f, y / 4.0f, 8, seed + 1) - 0.5f)
                     + 0.04f * (fbm(x / 2.0f, y / 2.0f, 32, seed + 2) - 0.5f);
            ramp[x * TS + y] = R_FLOOR; g_mid[m][x * TS + y] = 0;
        }
    int n = 0;
    float px[22], py[22], pr[22];
    for (int k = 0; k < 60 && n < 22; k++) {                         /* cobbles, not too close */
        float cx = h01(k, 3, seed) * 64.0f, cy = h01(k, 4, seed) * 64.0f, r = 3.5f + h01(k, 5, seed) * 5.0f;
        int ok = 1;
        for (int j = 0; j < n && ok; j++) if (hypotf(wrapd(cx - px[j]), wrapd(cy - py[j])) < r + pr[j] + 1.5f) ok = 0;
        if (!ok) continue;
        px[n] = cx; py[n] = cy; pr[n] = r * 1.2f;
        n++;
        float base = 0.3f + 0.24f * h01(k, 6, seed), q = h01(k, 8, seed);   /* river stones of several kinds, flattened as they lie */
        tx_blob(m, n, cx, cy, r * (1.1f + 0.6f * h01(k, 7, seed)), r * (0.65f + 0.3f * h01(k, 11, seed)), base, q < 0.45f ? R_FLOOR : q < 0.6f ? R_WARM : R_STONE, ramp, 0.75f);
    }
    for (int y = 0; y < TS; y++)                                     /* the silt shadowed under each stone */
        for (int x = 0; x < TS; x++)
            if (ramp[x * TS + y] == R_FLOOR && !g_mid[m][x * TS + y] && g_mid[m][x * TS + ((y - 1) & 63)]) GV(x, y) -= 0.16f;
    if (vseed) {                                                     /* a shell, ribbed; a long bone half out */
        float sx = 18.0f + h01(2, 1, vseed) * 28.0f, sy = 20.0f + h01(2, 2, vseed) * 24.0f;
        for (int y = (int)sy - 4; y <= (int)sy + 3; y++)
            for (int x = (int)sx - 5; x <= (int)sx + 5; x++) {
                float nx = (x + 0.5f - sx) / 5.0f, ny = (y + 0.5f - sy) / 4.0f;
                if (nx * nx + ny * ny > 1.0f || ny > 0.6f) continue;
                GV(x, y) = 0.6f + 0.15f * (1.0f - ny) - (fmodf(atan2f(ny + 1.2f, nx) * 6.0f + 20.0f, 1.0f) < 0.25f ? 0.2f : 0.0f);
                ramp[(x & 63) * TS + (y & 63)] = R_BONE;
            }
        float bx = 6.0f + h01(2, 3, vseed) * 20.0f, by = 46.0f + h01(2, 4, vseed) * 8.0f;
        for (int i = 0; i <= 18; i++)
            for (int w = -1; w <= 1; w++) { GV((int)bx + i, (int)(by - i * 0.25f) + w) = 0.66f - 0.12f * w - (i == 0 || i == 18 ? 0.0f : 0.05f); ramp[(((int)bx + i) & 63) * TS + (((int)(by - i * 0.25f) + w) & 63)] = R_BONE; }
    }
    for (int y = 0; y < TS; y++) for (int x = 0; x < TS; x++) tput(t, x, y, sh(ramp[x * TS + y], GV(x, y)));
}

/* flowstone: the calcite the water laid down over the wall, lobe over lobe like wax run down a candle, each
 * lit on its shoulder and dark under its rim, soft folds down it, stained where the water ran most; vseed
 * runs a thread of water down it */
static void gen_flow(int t, int m, uint32_t seed, uint32_t vseed) {
    float sx = 14.0f + h01(3, 1, vseed + 1) * 36.0f;
    for (int y = 0; y < TS; y++)
        for (int x = 0; x < TS; x++) {
            float fold = sinf(TAU_F * 3.0f * x / 64.0f + 2.2f * (fbm(x / 16.0f, y / 32.0f, 4, seed) - 0.5f) * 2.0f);
            float rims[6];                                            /* each lobe's rim, its own, dipping a little under the folds */
            for (int k = 0; k < 6; k++) rims[k] = 16.0f * (k - 1) + 8.0f + 10.0f * (fbm(x / 16.0f, (float)((k + 3) & 3) * 2.3f, 4, seed + 3) - 0.5f) + 1.2f * fold;
            int k = 1;
            while (k < 5 && y >= rims[k]) k++;
            float u = clampf((y - rims[k - 1]) / fmaxf(4.0f, rims[k] - rims[k - 1]), 0.0f, 1.0f);   /* 0 at a lobe's top, 1 at its rim */
            float v = 0.66f - 0.34f * u * u + 0.05f * fold + 0.05f * (fbm(x / 8.0f, y / 8.0f, 8, seed + 1) - 0.5f) + 0.03f * (fbm(x / 2.0f, y / 2.0f, 32, seed + 2) - 0.5f);
            if (u > 0.93f) v = 0.12f;                                  /* the shadow under the rim */
            else if (u < 0.08f) v += 0.06f;                           /* the shoulder below it */
            uint8_t c = sh(R_BONE, v);
            if (fbm(x / 4.0f, y / 16.0f, 16, seed + 5) > 0.6f && u > 0.25f && u < 0.93f) c = sh(R_WARM, 0.62f - 0.34f * u * u);   /* tan where the water ran most */
            if (u < 0.2f && h01(x, y, seed + 4) > 0.9f) c = sh(R_BONE, 0.9f);   /* wet on the shoulders */
            if (vseed && wrapd(x + 0.5f - (sx + 2.0f * sinf(y * 0.21f))) < 1.0f && u < 0.93f)
                c = h01(x, y, vseed) > 0.5f ? sh(R_GHOST, 0.62f) : sh(R_GHOST, 0.36f);
            tput(t, x, y, c);
            g_mid[m][x * TS + y] = 0;
        }
}

/* bone in silt: the dead the floods brought down, packed in the mud where they fell: long bones every way,
 * skulls, a jaw, the odd run of vertebrae, some of it under the mud again; vseed leaves a ribcage open */
static void wrap_draw(int kind, float x, float y, float a, float b, float c, float d) {   /* a shape on the canvas, again across each edge */
    for (int oy = -64; oy <= 64; oy += 64)
        for (int ox = -64; ox <= 64; ox += 64) {
            if (kind == 0) cv_bone(x + ox, y + oy, a + ox, b + oy, c, 0.35f, d);
            else if (kind == 1) cv_skull(x + ox, y + oy, a, 0.3f, b);
            else cv_ellipse(x + ox, y + oy, a, b, R_BONE, 0.3f, c);
        }
}
static void gen_bonesilt(int t, int m, uint32_t seed, uint32_t vseed) {
    cv_begin(cv_sprite, TS, TS);
    for (int k = 0; k < 12; k++) {                                   /* long bones */
        float x0 = h01(k, 1, seed) * 64.0f, y0 = h01(k, 2, seed) * 64.0f, a = h01(k, 3, seed) * PI_F, len = 11.0f + h01(k, 4, seed) * 9.0f;
        wrap_draw(0, x0, y0, x0 + cosf(a) * len, y0 + sinf(a) * len, 1.25f + 0.35f * h01(k, 5, seed), 0.7f + 0.2f * h01(k, 6, seed));
    }
    for (int k = 0; k < 3; k++) {                                    /* skulls */
        float cx = 10.0f + k * 21.0f + h01(k, 7, seed) * 8.0f, cy = 12.0f + h01(k, 8, seed) * 40.0f;
        wrap_draw(1, cx, cy, 0.85f + 0.2f * h01(k, 9, seed), 0.82f, 0, 0);
    }
    for (int k = 0; k < 2; k++) {                                    /* a run of vertebrae */
        float x = h01(k, 10, seed) * 64.0f, y = h01(k, 11, seed) * 64.0f, a = h01(k, 12, seed) * PI_F;
        for (int i = 0; i < 6; i++) wrap_draw(2, x + cosf(a) * i * 2.6f, y + sinf(a) * i * 2.6f, 1.3f, 1.1f, 0.72f, 0);
    }
    if (vseed) {                                                     /* a ribcage, open */
        float cx = 24.0f + h01(5, 1, vseed) * 16.0f, cy = 26.0f;
        for (int r = 0; r < 5; r++)
            for (int sd = -1; sd <= 1; sd += 2)
                for (int i = 1; i <= 9; i++) {
                    float x = cx + sd * i, y = cy + r * 3.4f + 0.06f * i * i;
                    cv_px((int)x, (int)y, sh(R_BONE, 0.72f - 0.02f * i)); cv_px((int)x, (int)y + 1, sh(R_BONE, 0.4f));
                }
        cv_capsule(cx, cy - 2, cx, cy + 17, 1.0f, R_BONE, 0.4f, 0.75f);
    }
    for (int y = 0; y < TS; y++)
        for (int x = 0; x < TS; x++) {
            float v = 0.27f + 0.05f * sinf(y * 0.9f + 4.0f * fbm(x / 16.0f, y / 16.0f, 4, seed)) + 0.07f * (fbm(x / 4.0f, y / 4.0f, 16, seed + 1) - 0.5f);
            uint8_t c = cv_sprite[y * TS + x];
            float under = fbm(x / 8.0f, y / 8.0f, 8, seed + 2);       /* some of it under the mud again */
            if (c != C_CLEAR && under > 0.64f) c = C_CLEAR;
            if (c == C_CLEAR) {
                uint8_t up = cv_sprite[((y - 1) & 63) * TS + x];
                if (up != C_CLEAR && fbm(x / 8.0f, ((y - 1) & 63) / 8.0f, 8, seed + 2) <= 0.64f) v -= 0.12f;   /* the mud shadowed under what sticks out */
                c = sh(R_FLOOR, v);
            } else if (under > 0.58f) c = darker(c, 3);                /* just coming out of it */
            tput(t, x, y, c);
            g_mid[m][x * TS + y] = 0;
        }
}

/* the Catacombs' rot: damp, then the floods. A flood line at one height on every wall of the run (from the
 * shared field, so it runs on across a seam), climbing with the run; under it the stain, the silt, older
 * lines; algae along it in patches. The webs come as things on the walls, not in the stone, so they don't
 * repeat cell after cell */
static float flood_y(float dk) { return 60.0f - 34.0f * dk; }
static void decay_catacomb(int t, float dk, uint32_t field, uint32_t own) {
    uint8_t *p = crawl_tex[t];
    float fy = flood_y(dk);
    for (int y = 0; y < TS; y++)
        for (int x = 0; x < TS; x++) {
            uint8_t c = p[x * TS + y];
            if (c == C_CLEAR || IS_FULLBRIGHT(c)) continue;
            float n = fbm(x / 8.0f, y / 8.0f, 8, field + 5);
            if (n < 0.2f + 0.25f * dk) c = darker(c, 1);              /* damp */
            float line = fy + 1.6f * (fbm(x / 16.0f, 0.5f, 4, field + 7) - 0.5f) * 2.0f, d = y - line;
            if (d >= 0.0f && d < 1.0f) c = lighter(c, 3);             /* the line's salt crust */
            else if (d >= 1.0f && d < 3.0f) c = darker(c, 3);
            else if (d >= 3.0f) {                                     /* under it: stained, silted, older lines */
                c = darker(c, 1 + (d > 16.0f));
                if (fmodf(d + 64.0f, 11.0f) < 0.8f && d > 8.0f) c = darker(c, 1);
                if (d > 8.0f && fbm(x / 8.0f, y / 4.0f, 8, field + 8) < 0.06f + 0.006f * d) c = sh(R_FLOOR, 0.24f + 0.1f * h01(x, y, field));
            }
            if (dk > 0.15f && d > 1.0f && d < 3.0f + 3.0f * dk && fbm(x / 8.0f, y / 4.0f, 8, field + 9) > 0.7f - 0.08f * dk && h01(x, y, field + 2) > 0.3f)
                c = sh(R_MOSS, 0.16f + 0.24f * h01(x, y, field + 1));  /* algae along the water's edge, in patches */
            p[x * TS + y] = c;
        }
    #define DK(xx, yy, by) do { uint8_t *q_ = &p[((xx) & 63) * TS + ((yy) & 63)]; if (*q_ != C_CLEAR && !IS_FULLBRIGHT(*q_)) *q_ = darker(*q_, by); } while (0)
    int drips = 2 + (int)(dk * 5.0f);                                /* the seep down from above */
    for (int k = 0; k < drips; k++) {
        int x = (int)(h01(k, 1, own) * 63), len = 8 + (int)(h01(k, 2, own) * 30.0f);
        for (int y = 0; y < len; y++) { DK(x, y, 2); if (y > 2 && h01(k, y, own) > 0.8f) { uint8_t *q_ = &p[(x & 63) * TS + y]; if (*q_ != C_CLEAR && !IS_FULLBRIGHT(*q_)) *q_ = lighter(*q_, 4); } }
    }
    #undef DK
}

/* ---- what stands on the Catacombs' walls ---- */

static void water_in(float x0, float x1, float y, float depth, uint32_t seed) {   /* standing water in an opening, its surface catching the light */
    for (int yy = (int)y; yy < (int)(y + depth); yy++)
        for (int x = (int)x0; x < (int)x1; x++) {
            if (cvp[yy * TS + x] == C_CLEAR) continue;
            float v = yy == (int)y ? 0.62f : 0.16f + 0.08f * (fbm(x / 4.0f, yy / 2.0f, 16, seed) - 0.5f) - 0.01f * (yy - y);
            if (yy == (int)y + 1 && h01(x, yy, seed) > 0.6f) v = 0.42f;
            cv_px(x, yy, sh(R_GHOST, v));
        }
}
static void gen_feat_fniche(int t, uint32_t seed) {                /* two niches, the flood standing in them: a skull afloat, the silt under */
    cv_begin(cv_sprite, TS, TS);
    for (int nx = 0; nx < 2; nx++) {
        float x0 = nx * 32 + 7.0f, y0 = 12.0f, w = 18.0f, h = 34.0f, cx = x0 + w * 0.5f;
        cut_arch_r(x0, y0, w, h, 8.0f, 1, seed, R_STONE, 0.04f);
        int k = (int)(hsh(nx, 1, seed) % 3);
        float wy = y0 + h - 9.0f - 3.0f * h01(nx, 2, seed);
        if (k == 0) cv_skull(cx, wy - 1.0f, 0.75f, 0.3f, 0.8f);
        else if (k == 1) { cv_bone(cx - 6, wy - 1.0f, cx + 5, wy - 2.0f, 1.0f, 0.25f, 0.7f); cv_skull(cx + 3.0f, wy - 2.5f, 0.55f, 0.25f, 0.7f); }
        else for (int y = (int)wy - 3; y < (int)wy; y++) for (int x = (int)x0 + 2; x < (int)(x0 + w) - 2; x++) cv_px(x, y, sh(R_WARM, 0.18f + 0.05f * h01(x, y, seed)));   /* a shroud gone to rags */
        water_in(x0, x0 + w, wy, 9.0f + 3.0f * h01(nx, 2, seed), seed + nx);
        for (int x = (int)x0 - 3; x < (int)(x0 + w) + 3; x++)        /* algae at the water's height on the stones */
            for (int y = (int)wy - 1; y <= (int)wy + 1; y++) if (cvp[y * TS + x] != C_CLEAR && !arch_in(x + 0.5f, y + 0.5f, x0, y0, w, h, 8.0f) && h01(x, y, seed + 3) > 0.35f) cv_px(x, y, sh(R_MOSS, 0.3f + 0.2f * h01(x, y, seed + 4)));
    }
    cv_to_tex(t);
}
static void gen_feat_saint(int t, uint32_t seed) {                 /* a saint in an alcove, in faded blue and gilt, candles at the feet */
    cv_begin(cv_sprite, TS, TS);
    cut_arch_r(18, 4, 28, 50, 13, 1, seed, R_STONE, 0.06f);
    for (int y = 5; y < 20; y++) for (int x = 20; x < 44; x++) if (cvp[y * TS + x] != C_CLEAR && arch_in(x + 0.5f, y + 0.5f, 18, 4, 28, 50, 13)) cv_px(x, y, sh(R_STONE, 0.12f));
    for (int a = 0; a < 64; a++) {                                    /* the halo, gilt on the plaster */
        float ang = TAU_F * a / 64.0f;
        for (float rr = 7.2f; rr < 8.6f; rr += 0.5f) cv_px((int)(32.0f + rr * cosf(ang)), (int)(18.0f + rr * sinf(ang)), sh(R_GOLD, 0.55f + 0.3f * -sinf(ang)));
    }
    for (int y = 22; y <= 51; y++)                                   /* the robe, a hood over the skull */
        for (int x = 20; x <= 44; x++) {
            float half = y < 30 ? 5.5f + (y - 22) * 0.4f : 8.5f + (y - 30) * 0.16f, dx = x + 0.5f - 32.0f;
            if (fabsf(dx) > half) continue;
            float fold = sinf(dx * 1.1f + y * 0.05f) * 0.08f, v = 0.32f + fold - 0.012f * dx + (y > 46 ? -0.08f : 0.0f) + 0.06f * (fbm(x / 4.0f, y / 4.0f, 16, seed) - 0.5f);
            cv_px(x, y, sh(R_GHOST, v));
            if (fabsf(dx) > half - 1.0f) cv_px(x, y, sh(R_GOLD, 0.45f));   /* its gilt hem */
        }
    for (int y = 12; y <= 25; y++) for (int x = 25; x <= 39; x++) {   /* the hood */
        float dx = (x + 0.5f - 32.0f) / 7.0f, dy = (y + 0.5f - 19.5f) / 7.0f;
        if (dx * dx + dy * dy <= 1.0f) cv_px(x, y, sh(R_GHOST, 0.26f - 0.1f * dy));
    }
    cv_skull(32, 19.5f, 0.6f, 0.3f, 0.78f);
    for (int i = -4; i <= 4; i++) { cv_px(32 + i, 36, sh(R_BONE, 0.7f)); cv_px(32 + i, 37, sh(R_BONE, 0.45f)); }   /* the hands, folded */
    cv_ellipse(28, 36.5f, 1.6f, 1.4f, R_BONE, 0.5f, 0.85f); cv_ellipse(36, 36.5f, 1.6f, 1.4f, R_BONE, 0.5f, 0.85f);
    for (int k = 0; k < 3; k++) {                                     /* candles on the sill */
        float cx = 23.0f + k * 9.0f + (k == 1 ? 0.0f : 0.0f), top = 49.0f - (k == 1 ? 4 : 1);
        for (int y = (int)top; y <= 53; y++) for (int x = (int)cx - 1; x <= (int)cx + 1; x++) cv_px(x, y, sh(R_BONE, 0.82f - (x - cx + 1) * 0.15f));
        cv_px((int)cx, (int)top - 1, (uint8_t)(R_FLAME + 15)); cv_px((int)cx, (int)top - 2, (uint8_t)(R_FLAME + 12)); cv_px((int)cx, (int)top - 3, (uint8_t)(R_FLAME + 8));
        cv_px((int)cx - 1, (int)top - 1, (uint8_t)(R_FLAME + 9)); cv_px((int)cx + 1, (int)top - 1, (uint8_t)(R_FLAME + 9));
        for (int d = 1; d <= 2; d++) cv_px((int)cx + (k - 1), 54 + d, sh(R_BONE, 0.7f));   /* wax run down */
    }
    cv_to_tex(t);
}
static void gen_feat_culvert(int t, uint32_t seed) {               /* a culvert low in the wall: a round arch, its bars, the water coming out */
    cv_begin(cv_sprite, TS, TS);
    cut_arch_r(19, 32, 26, 22, 13, 0, seed, R_STONE, 0.06f);
    for (int y = 32; y < 54; y++)
        for (int x = 19; x < 45; x++) {
            if (!arch_in(x + 0.5f, y + 0.5f, 19, 32, 26, 22, 13)) continue;
            float v = 0.03f + 0.04f * h01(x, y, seed);
            cv_px(x, y, sh(R_STONE, v));
            if ((x - 19) % 5 == 2) cv_px(x, y, sh(R_IRON, 0.32f + (x % 5 == 1 ? 0.0f : 0.15f) * ((y & 3) != 0)));   /* the bars */
            if ((x - 19) % 5 == 2 && h01(x, y / 3, seed + 2) > 0.7f) cv_px(x, y, sh(R_GHOST, 0.4f));   /* gone green */
        }
    for (int y = 54; y < 64; y++)                                     /* the outflow down the wall's foot */
        for (int x = 24; x < 40; x++) {
            float w = 6.0f + (y - 54) * 0.35f, dx = fabsf(x + 0.5f - 32.0f);
            if (dx > w) continue;
            float v = 0.22f + 0.2f * h01(x, y + (int)(dx), seed + 3) + (dx < 1.5f ? 0.2f : 0.0f);
            cv_px(x, y, sh(R_GHOST, v));
        }
    for (int x = 15; x < 49; x++) for (int y = 52; y < 56; y++) if (cvp[y * TS + x] != C_CLEAR && h01(x, y, seed + 4) > 0.4f && !(x >= 24 && x < 40 && y >= 54)) cv_px(x, y, sh(R_MOSS, 0.28f + 0.2f * h01(x, y, seed + 5)));
    cv_to_tex(t);
}
static void hand_at(float cx, float cy, float a, float s, uint32_t seed) {   /* a hand pressed in the mud, and its drag */
    float ca = cosf(a), sa = sinf(a);
    for (int y = (int)(cy - 9 * s); y <= (int)(cy + 9 * s); y++)
        for (int x = (int)(cx - 9 * s); x <= (int)(cx + 9 * s); x++) {
            float px = x + 0.5f - cx, py = y + 0.5f - cy, u = (px * ca + py * sa) / s, v = (-px * sa + py * ca) / s;
            int in = (u * u) / 10.0f + (v - 1.5f) * (v - 1.5f) / 12.0f < 1.0f;                      /* the palm */
            for (int f = 0; f < 5 && !in; f++) {
                float fx = -3.2f + f * 1.6f + (f == 0 ? -0.8f : 0.0f), fy = f == 0 ? -0.5f : -2.0f, len = f == 0 ? 3.2f : (f == 2 ? 5.2f : 4.4f);
                float ang = f == 0 ? -0.9f : -1.5708f + (f - 2) * 0.16f, ex = fx + cosf(ang) * len, ey = fy + sinf(ang) * len;
                float vx = ex - fx, vy = ey - fy, tt = clampf(((u - fx) * vx + (v - fy) * vy) / (vx * vx + vy * vy), 0.0f, 1.0f);
                float ox = u - (fx + vx * tt), oy = v - (fy + vy * tt);
                in = ox * ox + oy * oy < 0.42f;
            }
            if (in) cv_px(x, y, sh(R_FLOOR, 0.46f + 0.1f * h01(x, y, seed)));
        }
    int drag = (int)(h01(1, 1, seed) * 3.0f);                        /* fingers dragged down after */
    for (int k = 0; k < drag; k++)
        for (int y = (int)(cy + 5 * s); y < (int)(cy + (12 + 8 * h01(k, 2, seed)) * s); y++)
            if (h01(k, y, seed + 1) > 0.15f) cv_px((int)(cx - 2 + k * 1.6f * s + (y - cy) * sa * 0.3f), y, sh(R_FLOOR, 0.38f));
}
static void gen_feat_hands(int t, uint32_t seed) {                 /* handprints in the silt on the wall, where the water came up */
    cv_begin(cv_sprite, TS, TS);
    for (int k = 0; k < 4; k++) {
        float x = 12.0f + (k & 1) * 26.0f + h01(k, 1, seed) * 14.0f, y = 14.0f + (k >> 1) * 20.0f + h01(k, 2, seed) * 10.0f;
        hand_at(x, y, (h01(k, 3, seed) - 0.5f) * 0.8f, 1.45f + 0.3f * h01(k, 4, seed), seed + (uint32_t)k * 7u);
    }
    cv_to_tex(t);
}
static void gen_feat_web(int t, uint32_t seed) {                   /* a curtain of web across the stone: sagging strands, drops, two wheels */
    cv_begin(cv_sprite, TS, TS);
    #define WEBPX(xx, yy, vv) do { int x_ = (int)(xx), y_ = (int)(yy); if (x_ >= 1 && x_ <= 62 && y_ >= 0 && y_ <= 62 && h01(x_, y_, seed + 99) > 0.18f) cv_px(x_, y_, sh(R_STONE, (vv))); } while (0)
    for (int k = 0; k < 4; k++) {                                     /* strands sagging from the top */
        float sag = 10.0f + 22.0f * h01(k, 1, seed), y0 = 1.0f + 6.0f * h01(k, 2, seed);
        for (float x = 1.0f; x < 63.0f; x += 0.5f) { float u = (x - 32.0f) / 31.0f; WEBPX(x, y0 + sag * (1.0f - u * u), 0.78f + 0.12f * h01((int)x, k, seed)); }
    }
    for (int k = 0; k < 5; k++) {                                     /* threads hanging down */
        float x = 4.0f + h01(k, 3, seed) * 56.0f, len = 16.0f + 30.0f * h01(k, 4, seed);
        for (float y = 0.0f; y < len; y += 0.5f) WEBPX(x + sinf(y * 0.15f + k) * 1.2f, y, 0.72f + 0.15f * h01(k, (int)y, seed));
        WEBPX(x, len, 0.95f);
    }
    for (int w = 0; w < 1; w++) {                                     /* a wheel caught in it */
        float cx = 16.0f + w * 30.0f + 6.0f * h01(w, 5, seed), cy = 26.0f + 14.0f * h01(w, 6, seed), R = 7.0f + 4.0f * h01(w, 7, seed);
        for (int s = 0; s < 8; s++) { float a = s * TAU_F / 8.0f; for (float r = 0.0f; r < R; r += 0.5f) WEBPX(cx + cosf(a) * r, cy + sinf(a) * r, 0.84f); }
        for (float r = 2.0f; r < R; r += 2.2f) for (int s = 0; s < 8; s++) {
            float a0 = s * TAU_F / 8.0f, a1 = (s + 1) * TAU_F / 8.0f;
            for (float q = 0.0f; q <= 1.0f; q += 0.12f) WEBPX(cx + cosf(a0 + (a1 - a0) * q) * r * (1.0f - 0.08f * sinf(q * PI_F)), cy + sinf(a0 + (a1 - a0) * q) * r * (1.0f - 0.08f * sinf(q * PI_F)), 0.76f);
        }
    }
    #undef WEBPX
    cv_to_tex(t);
}
static void gen_feat_lantern(int t, int frame, uint32_t seed) {    /* a lantern on an iron hook: horn panes lit from within */
    cv_begin(cv_sprite, TS, TS);
    for (int x = 31; x <= 42; x++) { cv_px(x, 20, sh(R_IRON, 0.7f)); cv_px(x, 21, sh(R_IRON, 0.3f)); }   /* the arm out from the wall */
    for (int y = 16; y <= 26; y++) { cv_px(42, y, sh(R_IRON, 0.55f)); cv_px(43, y, sh(R_IRON, 0.3f)); }
    for (int x = 40; x <= 45; x++) for (int y = 14; y <= 17; y++) cv_px(x, y, sh(R_IRON, y == 14 ? 0.7f : 0.4f));   /* its plate */
    for (int y = 21; y <= 25; y++) cv_px(32, y, sh(R_IRON, 0.5f));    /* the hook */
    cv_px(33, 25, sh(R_IRON, 0.5f)); cv_px(34, 24, sh(R_IRON, 0.5f));
    for (int x = 28; x <= 37; x++) cv_px(x, 26, sh(R_IRON, 0.6f));    /* the cap */
    for (int x = 29; x <= 36; x++) cv_px(x, 27, sh(R_IRON, 0.35f));
    static const float GL[3] = { 0.0f, 0.08f, -0.06f };
    for (int y = 28; y <= 40; y++)
        for (int x = 27; x <= 37; x++) {
            float nx = (x + 0.5f - 32.0f) / 5.5f;
            int bar = fabsf(nx) > 0.82f || fabsf(nx) < 0.1f || y == 28 || y == 40 || y == 34;
            if (bar) { cv_px(x, y, sh(R_IRON, 0.25f + 0.35f * lit(nx, 0.0f, sqrtf(fmaxf(0.0f, 1.0f - nx * nx))))); continue; }
            float glow = 1.0f - fabsf(nx) * 0.55f - fabsf(y - 35.0f) / 13.0f + GL[frame];
            cv_px(x, y, (uint8_t)(R_FLAME + clampi((int)(glow * 12.0f), 2, 12)));
        }
    static const float FH[3] = { 4.0f, 5.0f, 3.5f }, FS[3] = { 0.0f, 0.5f, -0.5f };
    for (int y = 0; y < 7; y++) {
        float u = y / FH[frame];
        if (u > 1.0f) break;
        int x = (int)(32.0f + FS[frame] * u);
        cv_px(x, 37 - y, (uint8_t)(R_FLAME + 15 - y));
        if (u < 0.6f) { cv_px(x - 1, 37 - y, (uint8_t)(R_FLAME + 11)); cv_px(x + 1, 37 - y, (uint8_t)(R_FLAME + 11)); }
    }
    for (int x = 27; x <= 37; x++) cv_px(x, 41, sh(R_IRON, 0.5f));    /* the base, a drip of wax */
    cv_px(32, 42, sh(R_IRON, 0.4f));
    (void)seed;
    cv_to_tex(t);
}

/* ---- the Catacombs' floors and ceilings ---- */

static void gen_floor_wetflags(int t, uint32_t seed, int puddles) { /* slate flags, wet: water in the joints, sheen on the stone; puddled, it stands */
    int id[4][4], ox[16], oy[16], fw[16], fh[16], n = 0;
    memset(id, -1, sizeof id);
    for (int uy = 0; uy < 4; uy++)
        for (int ux = 0; ux < 4; ux++) {
            if (id[uy][ux] >= 0) continue;
            float r = h01(ux, uy, seed);
            int w = 1, h = 1;
            if (r < 0.3f && ux < 3 && uy < 3 && id[uy][ux + 1] < 0 && id[uy + 1][ux] < 0 && id[uy + 1][ux + 1] < 0) w = h = 2;
            else if (r < 0.6f && ux < 3 && id[uy][ux + 1] < 0) w = 2;
            else if (r < 0.8f && uy < 3 && id[uy + 1][ux] < 0) h = 2;
            for (int j = 0; j < h; j++) for (int i = 0; i < w; i++) id[uy + j][ux + i] = n;
            ox[n] = ux * 16; oy[n] = uy * 16; fw[n] = w * 16; fh[n] = h * 16; n++;
        }
    for (int y = 0; y < TS; y++)
        for (int x = 0; x < TS; x++) {
            int f = id[y / 16][x / 16], px = x - ox[f], py = y - oy[f];
            uint8_t c;
            if (px == 0 || py == 0) c = sh(R_GHOST, 0.08f + 0.06f * h01(x, y, seed + 1));
            else {
                float v = 0.32f + 0.07f * (2.0f * h01(f, 3, seed) - 1.0f) + 0.1f * (fbm(x / 8.0f, y / 8.0f, 8, seed + 2) - 0.5f) + 0.012f * sinf(px * 1.7f)
                        + (px == 1 || py == 1 ? 0.05f : 0.0f) - (px == fw[f] - 1 || py == fh[f] - 1 ? 0.06f : 0.0f);
                if (h01(x, y, seed + 4) > 0.97f) v += 0.16f;           /* the wet on it */
                c = sh(R_STONE, v);
            }
            float pd = fbm(x / 8.0f, y / 8.0f, 8, seed + 9) + 0.3f * (1.0f - fminf(1.0f, hypotf(x - 30.0f - 10.0f * h01(1, 1, seed), y - 34.0f) / 26.0f));
            if (puddles && pd > 0.72f) {                              /* standing water, its edge catching the light */
                c = pd > 0.745f ? sh(R_GHOST, 0.14f + 0.06f * (fbm(x / 2.0f, y / 2.0f, 32, seed + 10) - 0.5f) + (h01(x, y, seed + 11) > 0.97f ? 0.3f : 0.0f)) : sh(R_GHOST, 0.42f);
            }
            tput(t, x, y, c);
        }
}
static void gen_floor_silt(int t, uint32_t seed) {                 /* silt the water left, in ripples; laid as laid, so nothing in it repeats cell by cell */
    for (int y = 0; y < TS; y++)
        for (int x = 0; x < TS; x++) {
            float ph = y * TAU_F / 8.0f + 2.5f * fbm(x / 16.0f, y / 16.0f, 4, seed);
            float v = 0.34f + 0.07f * sinf(ph) + 0.05f * cosf(ph) * 0.5f + 0.06f * (fbm(x / 8.0f, y / 8.0f, 8, seed + 1) - 0.5f) + 0.04f * (fbm(x / 2.0f, y / 2.0f, 32, seed + 2) - 0.5f);
            tput(t, x, y, sh(R_FLOOR, v));
        }
}
static void gen_floor_slateedge(int t, uint32_t seed) {            /* a hall's floor: slate set on edge, rows of thin slips */
    for (int y = 0; y < TS; y++)
        for (int x = 0; x < TS; x++) {
            int r = y / 16, ly = y & 15, off = (int)(h01(r, 1, seed) * 8.0f), lx = (x + off) & 63, s = lx / 8, sx = lx & 7;
            uint8_t c;
            int gap = sx == 0 || ly == 0 || (h01(r, s, seed + 2) > 0.55f && ly == 8);
            if (gap) c = sh(R_FLOOR, 0.1f + 0.06f * h01(x, y, seed + 3));
            else {
                int sid = r * 16 + s * 2 + (h01(r, s, seed + 2) > 0.55f && ly > 8);
                float v = 0.34f + 0.16f * (h01(sid, 4, seed) - 0.5f) + (sx == 1 ? 0.06f : sx == 7 ? -0.05f : 0.0f) + (sx >= 3 && sx <= 4 ? 0.02f : 0.0f) + 0.05f * (fbm(x / 4.0f, y / 4.0f, 16, seed + 5) - 0.5f);
                c = sh(R_STONE, v);
            }
            tput(t, x, y, c);
        }
}
static void gen_floor_water(int t, uint32_t seed) {                /* standing water over flags: the stones dim beneath, a ring going out */
    gen_floor_wetflags(t, seed, 0);
    for (int y = 0; y < TS; y++)
        for (int x = 0; x < TS; x++) {
            uint8_t under = crawl_tex[t][x * TS + y];
            float lum = (float)((under - 1) % 16) / 15.0f;
            float rip = fbm(x / 8.0f, y / 8.0f, 8, seed + 20);
            float v = 0.1f + 0.14f * lum + 0.06f * (rip - 0.5f) + (fabsf(rip - 0.55f) < 0.012f ? 0.08f : 0.0f);
            if (h01(x, y, seed + 21) > 0.996f) v += 0.28f;
            tput(t, x, y, sh(R_GHOST, v));
        }
}
static void straw(int t, int x, int y) {                           /* a calcite straw hanging from the stone, seen from below */
    tput(t, x, y, sh(R_BONE, 0.55f)); tput(t, x + 1, y + 1, sh(R_CEIL, 0.1f));
}
static void gen_ceil_slatevault(int t, uint32_t seed) {            /* a room's vault laid in slate: thin voussoirs, damp, straws */
    for (int y = 0; y < TS; y++)
        for (int x = 0; x < TS; x++) {
            int r = y / 8, ly = y & 7, off = (int)(h01(r, 1, seed) * 64.0f), lx = (x + off) & 63;
            int sw = 16 + (int)(h01(r, 2, seed) * 2.0f) * 16, s = lx / sw;
            float v = ly == 0 || lx % sw == 0 ? 0.04f : 0.4f + 0.12f * (h01(r * 8 + s, 3, seed) - 0.5f) + (ly == 1 ? 0.1f : ly == 7 ? -0.08f : 0.0f)
                    + 0.05f * (fbm(x / 4.0f, y / 4.0f, 16, seed + 4) - 0.5f);
            tput(t, x, y, sh(R_CEIL, v + 0.16f));
        }
    for (int k = 0; k < 3; k++) straw(t, (int)(h01(k, 5, seed) * 62), (int)(h01(k, 6, seed) * 62));
}
static void gen_ceil_bedded(int t, uint32_t seed) {                /* a hall's roof: the slate's beds seen from below, a joint across */
    for (int y = 0; y < TS; y++)
        for (int x = 0; x < TS; x++) {
            float b = y + 3.0f * fbm(x / 16.0f, y / 16.0f, 4, seed);
            int bed = (int)(b / 5.0f);
            float v = 0.36f + 0.08f * (h01(bed, 1, seed) - 0.5f) + 0.05f * (fbm(x / 4.0f, y / 4.0f, 16, seed + 2) - 0.5f);
            if (fmodf(b + 64.0f, 5.0f) < 0.8f) v -= 0.2f; else if (fmodf(b + 64.0f, 5.0f) < 1.6f) v += 0.06f;
            if (fabsf(x - 22.0f - 6.0f * fbm(x / 16.0f, y / 8.0f, 4, seed + 3)) < 0.6f) v -= 0.14f;
            tput(t, x, y, sh(R_CEIL, v + 0.12f));
        }
    for (int k = 0; k < 2; k++) straw(t, (int)(h01(k, 5, seed) * 62), (int)(h01(k, 6, seed) * 62));
}
static void gen_ceil_drip(int t, uint32_t seed) {                  /* rough rock, wet, hung with straws */
    gen_ceil_rock(t, seed);
    for (int y = 0; y < TS; y++)
        for (int x = 0; x < TS; x++) if (h01(x, y, seed + 7) > 0.985f) tput(t, x, y, lighter(crawl_tex[t][x * TS + y], 3));   /* wet on it */
    for (int k = 0; k < 7; k++) straw(t, (int)(h01(k, 5, seed) * 62), (int)(h01(k, 6, seed) * 62));
}
static void decay_flat2(int t, float dk, uint32_t seed, int wet) { /* the Catacombs' rot on a floor or ceiling: damp, silt drifting, algae */
    if (dk <= 0.0f) return;
    for (int y = 0; y < TS; y++)
        for (int x = 0; x < TS; x++) {
            uint8_t c = crawl_tex[t][x * TS + y];
            float n = fbm(x / 8.0f, y / 8.0f, 8, seed + 5);
            if (n < 0.2f + 0.25f * dk) c = darker(c, 1);
            if (wet && fbm(x / 8.0f, y / 8.0f, 8, seed + 6) < 0.1f + 0.22f * dk && (c - 1) / 16 != (R_GHOST - 1) / 16) c = sh(R_FLOOR, 0.24f + 0.1f * h01(x, y, seed));   /* silt drifting in */
            if (dk > 0.3f && fbm(x / 4.0f, y / 4.0f, 16, seed + 7) > 0.84f - 0.18f * dk && (c - 1) % 16 < 5) c = sh(R_MOSS, 0.14f + 0.18f * h01(x, y, seed + 1));
            crawl_tex[t][x * TS + y] = c;
        }
}

/* ---- the Catacombs' doors and keys ---- */

static void gen_sluice(int t, uint32_t seed, int metal) {          /* a sluice door, riveted iron, rusted, green with slime where the water stood; a vault's bands and plate in its key's metal */
    int vault = metal >= 0;
    for (int y = 0; y < TS; y++)
        for (int x = 0; x < TS; x++) {
            int p = x / 21, lx = x % 21;
            float v = 0.34f + 0.06f * h01(p, 0, seed) + 0.07f * (fbm(x / 8.0f, y / 8.0f, 8, seed) - 0.5f) + 0.04f * (fbm(x / 2.0f, y / 2.0f, 32, seed + 1) - 0.5f);
            if (lx == 0) v = 0.12f; else if (lx == 1) v += 0.1f; else if (lx == 20) v -= 0.06f;
            uint8_t c = sh(R_IRON, v);
            if (fbm(x / 2.0f, y / 16.0f, 32, seed + 2) > 0.66f) c = sh(R_WOOD, 0.3f + 0.15f * h01(x, y, seed + 3));   /* rust run down it */
            float slime = 46.0f + 3.0f * (fbm(x / 16.0f, 0.5f, 4, seed + 8) - 0.5f) * 2.0f;   /* green below where the water stood */
            if (y > slime && fbm(x / 4.0f, y / 4.0f, 16, seed + 9) > 0.35f - 0.01f * (y - slime)) c = sh(R_MOSS, 0.14f + 0.16f * h01(x, y, seed + 4));
            else if (y > slime - 1.0f && y <= slime) c = darker(c, 3);
            tput(t, x, y, c);
            if ((lx == 3 || lx == 18) && y % 6 == 3) { tput(t, x, y, sh(R_IRON, 0.72f)); tput(t, x + 1, y + 1, sh(R_IRON, 0.15f)); }   /* rivets down the seams */
        }
    int band = vault ? metal : R_IRON;
    float lo = band == R_GOLD ? 0.0f : 0.25f, k = band == R_GOLD ? 1.0f : 0.75f;
    for (int b = 0; b < 3; b++) {                                     /* three bands across */
        int y0 = 7 + b * 22;
        for (int y = y0; y < y0 + 5; y++)
            for (int x = 0; x < TS; x++) {
                float v = 0.5f + (y == y0 ? 0.3f : 0.0f) - (y == y0 + 4 ? 0.3f : 0.0f) + 0.06f * (fbm(x / 4.0f, y / 4.0f, 16, seed + 5) - 0.5f);
                if (x % 8 == 4 && y == y0 + 2) v = 1.0f;
                uint8_t c = sh(band, lo + v * k);
                if (!vault && fbm(x / 4.0f, y / 4.0f, 16, seed + 6) > 0.68f) c = sh(R_WOOD, 0.34f);
                tput(t, x, y, c);
            }
    }
    if (!vault) {                                                    /* a grille to see the water coming, a wheel to wind it */
        for (int y = 14; y <= 25; y++)
            for (int x = 24; x <= 39; x++) {
                int edge = x == 24 || x == 39 || y == 14 || y == 25, bar = x == 28 || x == 32 || x == 36 || y == 20;
                tput(t, x, y, edge ? sh(R_IRON, 0.45f) : (bar ? sh(R_IRON, 0.62f) : 0));
            }
        for (int a = 0; a < 48; a++) {
            float ang = TAU_F * a / 48;
            tput(t, 46 + (int)lroundf(6.0f * cosf(ang)), 41 + (int)lroundf(6.0f * sinf(ang)), sh(R_IRON, 0.5f + 0.35f * -sinf(ang)));
        }
        for (int s = 0; s < 4; s++) for (int r = 1; r < 6; r++) { float ang = s * TAU_F / 4.0f + 0.4f; tput(t, 46 + (int)lroundf(r * cosf(ang)), 41 + (int)lroundf(r * sinf(ang)), sh(R_IRON, 0.62f)); }
        tput(t, 46, 41, sh(R_IRON, 0.9f));
    } else
        for (int y = 16; y <= 46; y++)
            for (int x = 22; x <= 41; x++) {                          /* the lock plate, a keyhole, the saint's cross */
                int edge = x == 22 || y == 16, shade = x == 41 || y == 46;
                float v = 0.6f + (edge ? 0.3f : 0.0f) - (shade ? 0.35f : 0.0f) + fbm(x / 4.0f, y / 4.0f, 16, seed + 7) * 0.15f;
                float kx = x + 0.5f - 31.5f, ky = y + 0.5f - 34.0f;
                if (kx * kx + ky * ky < 7.0f || (fabsf(kx) < 1.2f && ky > 0.0f && ky < 9.0f)) { tput(t, x, y, 0); continue; }
                if ((fabsf(kx) < 1.0f && y >= 19 && y <= 28) || (y >= 21 && y <= 22 && fabsf(kx) < 3.0f)) v -= 0.35f;
                tput(t, x, y, sh(metal, lo + v * k));
            }
}
static void gen_slatejamb(int t, uint32_t seed) {                  /* the doorway's side: slate lining cut narrow, the iron slot the door runs in */
    for (int y = 0; y < TS; y++)
        for (int x = 0; x < TS; x++) {
            int r = y / 5;
            float v = 0.36f + 0.08f * (h01(r, 1, seed) - 0.5f) + 0.05f * (fbm(x / 8.0f, y / 8.0f, 8, seed) - 0.5f);
            if (y % 5 == 0 || ((r & 1) ? x == 24 : x == 44)) v = 0.06f;
            if (y % 5 == 1) v += 0.08f;
            uint8_t c = sh(R_STONE, v);
            if (x < 5) c = sh(R_IRON, 0.3f + (x == 0 ? 0.25f : x == 4 ? -0.1f : 0.0f) + (y % 8 == 4 && x == 2 ? 0.4f : 0.0f));
            tput(t, x, y, c);
        }
}
static void spr_key_ring(int id, float scale, int metal) {         /* a key on a ring, pitted with the wet */
    cv_begin(cv_sprite, TS, TS);
    #define KX(x) (32.0f + ((x) - 32.0f) * scale)
    float ar = fmaxf(fabsf(scale), 0.18f), lo = metal == R_GOLD ? 0.35f : 0.45f;
    for (int a = 0; a < 64; a++) {                                    /* the ring */
        float ang = TAU_F * a / 64.0f;
        for (float w = -0.6f; w <= 0.6f; w += 0.6f) cv_px((int)KX(19.0f + (6.0f + w) * cosf(ang)), (int)(30.0f + (6.0f + w) * sinf(ang)), sh(metal, lo + (1.0f - lo) * (0.55f + 0.4f * -sinf(ang + 0.6f))));
    }
    cv_ellipse(KX(24), 36, 4.0f * ar, 4.0f, metal, lo, 1.0f);       /* the bow, through the ring */
    cv_flat(KX(24), 36, 1.8f * ar, 1.8f, C_CLEAR);
    cv_capsule(KX(27), 38.5f, KX(44), 49, 1.8f, metal, lo, 1.0f);   /* the shank, hanging */
    for (int y = 46; y <= 54; y++)
        for (int x = 42; x <= 48; x++) {
            float dx = x - 42.0f - (y - 46) * 0.35f;
            if (dx < 0.0f || dx > 4.0f || (y >= 51 && ((int)dx) == 2)) continue;
            cv_px((int)KX(x + 0.5f), y, sh(metal, lo + (1.0f - lo) * (dx < 1.0f ? 0.95f : 0.6f)));
        }
    for (int y = 0; y < TS; y++)                                     /* pitted */
        for (int x = 0; x < TS; x++) {
            uint8_t c = cvp[y * TS + x];
            if (c == C_CLEAR) continue;
            if (h01(x, y, 777) > 0.86f) cvp[y * TS + x] = sh(metal, lo * 0.5f);
            else if (metal != R_GOLD && h01(x, y, 778) > 0.93f) cvp[y * TS + x] = sh(R_GHOST, 0.32f);
        }
    #undef KX
    cv_outline(sh(metal, 0.05f));
    cv_store(id);
}

/* ---- the Catacombs' treasure: coin gone black in the silt, a saint's reliquary, pearls; the crown and the
 * chest as they came out of the water ---- */

static void spr_coins_silt(int id, int frame) {
    cv_begin(cv_sprite, TS, TS);
    cv_flat(32, 62, 18, 2.4f, R_WARM + 1);
    for (int i = 0; i < 34; i++) {
        float r = h01(i, 1, 40) * 13.0f, a = h01(i, 2, 40) * TAU_F;
        float x = 32 + cosf(a) * r, y = 60 - (13.0f - r) * 0.55f - h01(i, 3, 40) * 1.5f + sinf(a) * r * 0.18f;
        int bright = h01(i, 4, 41) < 0.45f;
        cv_ellipse(x, y, 3.2f, 1.3f, R_GOLD, bright ? 0.35f : 0.05f, bright ? 1.0f : 0.32f);
        if (bright) cv_px((int)(x - 1), (int)y, sh(R_GOLD, 0.95f));
    }
    for (int i = 0; i < 5; i++) {                                    /* silt caked over the heap */
        float x = 22 + h01(i, 5, 42) * 20, y = 55 + h01(i, 6, 42) * 6;
        cv_ellipse(x, y, 3.5f, 1.6f, R_FLOOR, 0.15f, 0.4f);
    }
    cv_outline(0);
    static const int G[2][3][2] = { { {27, 52}, {38, 55}, {33, 49} }, { {30, 54}, {24, 57}, {40, 52} } };
    for (int i = 0; i < 3; i++) if (cvp[G[frame][i][1] * TS + G[frame][i][0]] != C_CLEAR && (cvp[G[frame][i][1] * TS + G[frame][i][0]] >= R_GOLD)) cv_px(G[frame][i][0], G[frame][i][1], C_WHITE);
    cv_px(G[frame][0][0], G[frame][0][1], C_WHITE);
    cv_store(id);
}
static void spr_reliquary(void) {                                  /* a saint's reliquary: a little gilt house, blue enamel, a bone behind crystal */
    cv_begin(cv_sprite, TS, TS);
    cv_flat(32, 62, 15, 2.0f, R_WARM + 1);
    for (int y = 46; y <= 60; y++)
        for (int x = 19; x <= 45; x++) {
            int edge = x == 19 || x == 45 || y == 46 || y == 60 || y == 53;
            float v = 0.5f + 0.35f * (x < 24 ? 0.6f : x > 40 ? -0.4f : 0.0f) + (y == 46 ? 0.3f : 0.0f);
            uint8_t c = edge ? sh(R_GOLD, 0.85f) : sh(R_GHOST, 0.3f + 0.12f * ((x / 3 + y / 3) & 1));
            if (!edge && x >= 27 && x <= 37 && y >= 48 && y <= 58) c = (x == 27 || x == 37 || y == 48 || y == 58) ? sh(R_GOLD, 0.7f) : sh(R_GLOW, 0.12f + 0.05f * (y - 48) / 10.0f);
            (void)v;
            cv_px(x, y, c);
        }
    cv_bone(29, 55, 35, 51, 0.8f, 0.45f, 0.8f);                     /* the relic, behind the crystal */
    float roof[8] = { 17, 46, 47, 46, 41, 36, 23, 36 };
    cv_poly(roof, 4, R_GOLD, 1.0f, 0.4f, 36, 10);
    for (int x = 23; x <= 41; x += 3) cv_px(x, 41, sh(R_GOLD, 0.25f));
    for (int y = 27; y <= 35; y++) cv_px(32, y, sh(R_GOLD, 0.85f));  /* a cross on the ridge */
    for (int x = 29; x <= 35; x++) cv_px(x, 30, sh(R_GOLD, 0.85f));
    for (int k = 0; k < 4; k++) cv_ellipse(19 + k * 8.6f, 61, 1.4f, 1.2f, R_GOLD, 0.4f, 0.9f);   /* its feet */
    cv_outline(0);
    cv_px(24, 39, C_WHITE); cv_px(30, 50, C_WHITE);
    cv_store(SP_CHALICE);
}
static void spr_pearls(void) {                                     /* pearls out of a rotted purse, an oyster open beside */
    cv_begin(cv_sprite, TS, TS);
    cv_flat(32, 62, 15, 2.0f, R_WARM + 1);
    for (int y = 52; y <= 61; y++)                                   /* the shell */
        for (int x = 34; x <= 50; x++) {
            float nx = (x + 0.5f - 42.0f) / 8.0f, ny = (y + 0.5f - 58.0f) / 4.0f;
            if (nx * nx + ny * ny > 1.0f) continue;
            cv_px(x, y, nx * nx + ny * ny > 0.65f ? sh(R_STONE, 0.3f + 0.1f * h01(x, y, 43)) : sh(R_GHOST, 0.55f + 0.2f * h01(x / 2, y, 44)));
        }
    for (int y = 49; y <= 61; y++) for (int x = 16; x <= 30; x++) {  /* the purse, gone to rags */
        float nx = (x + 0.5f - 23.0f) / 7.0f, ny = (y + 0.5f - 56.0f) / 6.0f;
        if (nx * nx + ny * ny <= 1.0f && h01(x, y, 45) > 0.15f) cv_px(x, y, sh(R_WARM, 0.18f + 0.15f * (1.0f - ny)));
    }
    static const float P[11][3] = { {26, 56, 2.6f}, {30, 58, 2.4f}, {33, 55, 2.2f}, {28, 52, 2.0f}, {35, 59, 2.0f}, {22, 59, 2.3f}, {38, 56, 1.8f}, {41, 57, 2.8f}, {31, 61, 1.8f}, {25, 61, 1.6f}, {36, 52, 1.6f} };
    for (int i = 0; i < 11; i++) cv_ellipse(P[i][0], P[i][1], P[i][2], P[i][2], R_BONE, 0.45f, 1.0f);
    cv_outline(0);
    cv_px(40, 55, C_WHITE); cv_px(25, 55, C_WHITE); cv_px(32, 54, C_WHITE);
    cv_store(SP_GEMS);
}
static void spr_drown(int id, uint32_t seed) {                     /* the crown or the chest as the water gave it back: tarnished, silted, weed on it */
    uint8_t *s = crawl_spr[id];
    int top = TS, bot = 0;
    for (int x = 0; x < TS; x++) for (int y = 0; y < TS; y++) if (s[x * TS + y] != C_CLEAR && s[x * TS + y] != R_WARM + 1) { if (y < top) top = y; if (y > bot) bot = y; }
    for (int x = 0; x < TS; x++)
        for (int y = 0; y < TS; y++) {
            uint8_t c = s[x * TS + y];
            if (c == C_CLEAR || c == C_WHITE || c == R_WARM + 1) continue;
            if (c >= R_GOLD && h01(x, y, seed) > 0.35f) s[x * TS + y] = (uint8_t)(c - 3 >= R_GOLD ? c - 3 : R_GOLD);
            if (y > bot - 5 && fbm(x / 4.0f, y / 4.0f, 16, seed + 1) > 0.45f) s[x * TS + y] = sh(R_FLOOR, 0.2f + 0.15f * h01(x, y, seed + 2));
        }
    for (int k = 0; k < 3; k++) {                                     /* weed hanging off it */
        int x = 20 + (int)(h01(k, 3, seed) * 24.0f), y = top + (int)(h01(k, 4, seed) * 6.0f);
        for (int i = 0; i < 9; i++) { int xx = x + (int)(1.4f * sinf(i * 0.8f + k)), yy = y + i; if (yy < TS && s[xx * TS + yy] != C_CLEAR) s[xx * TS + yy] = sh(R_MOSS, 0.3f + 0.25f * h01(k, i, seed)); }
    }
}
/* ---- the Hollows' materials: the caves, the mine cut through them, the timber that held it up, the pit
 * where the dead went ---- */

/* rough granite: the cave as the water left it, in lumps and joints, crystals in it; vseed runs a band
 * of pale quartz across */
static int voro(int x, int y, const float *px, const float *py, int n, float *d1o, float *d2o) {   /* the nearest of n points, wrapping */
    float d1 = 1e9f, d2 = 1e9f; int k1 = 0;
    for (int i = 0; i < n; i++) {
        float dx = wrapd(x + 0.5f - px[i]), dy = wrapd(y + 0.5f - py[i]), d = sqrtf(dx * dx + dy * dy);
        if (d < d1) { d2 = d1; d1 = d; k1 = i; } else if (d < d2) d2 = d;
    }
    *d1o = d1; *d2o = d2;
    return k1;
}
static void gen_granite(int t, int m, uint32_t seed, uint32_t vseed) {
    float px[8], py[8];
    static float e[TS * TS];
    static int k1[TS * TS];
    for (int i = 0; i < 8; i++) { px[i] = h01(i, 1, seed) * 64.0f; py[i] = h01(i, 2, seed) * 64.0f; }
    for (int y = 0; y < TS; y++)
        for (int x = 0; x < TS; x++) {
            float d1, d2;
            k1[x * TS + y] = voro(x, y, px, py, 8, &d1, &d2);
            e[x * TS + y] = d2 - d1 + 3.0f * (fbm(x / 8.0f, y / 8.0f, 8, seed + 9) - 0.5f);   /* how far inside its lump, the edges worn */
        }
    #define E(xx, yy) e[((xx) & 63) * TS + ((yy) & 63)]
    for (int y = 0; y < TS; y++)
        for (int x = 0; x < TS; x++) {
            int k = k1[x * TS + y];
            float ee = E(x, y), gx = E(x + 1, y) - E(x - 1, y), gy = E(x, y + 1) - E(x, y - 1);   /* each lump's swell, lit from above and the left */
            float v = 0.46f + 0.08f * (h01(k, 3, seed) - 0.5f) + 0.06f * fminf(1.0f, ee / 8.0f) - 0.035f * gx - 0.05f * gy
                    + 0.1f * (fbm(x / 8.0f, y / 8.0f, 8, seed + 4) - 0.5f) + 0.05f * (fbm(x / 2.0f, y / 2.0f, 32, seed + 5) - 0.5f);
            int id = 1 + k;
            if (ee < 0.9f && fbm(x / 8.0f, y / 8.0f, 8, seed + 10) > 0.42f) { v = 0.12f + 0.06f * h01(x, y, seed + 6); id = 0; }   /* a crack along the joint, here and there */
            float c = h01(x, y, seed + 7);                            /* its crystals: pale feldspar, dark mica */
            if (c > 0.95f) v += 0.12f; else if (c < 0.03f) v -= 0.12f;
            if (vseed) {
                float vy = 30.0f + 10.0f * sinf(x * TAU_F / 64.0f + h01(1, 1, vseed) * 6.0f), dv = fabsf(y + 0.5f - vy);
                if (dv < 2.2f + fbm(x / 8.0f, 0.5f, 8, vseed) * 1.5f) v = 0.62f + 0.08f * h01(x, y, vseed) - 0.03f * gy;
            }
            GV(x, y) = v;
            g_mid[m][x * TS + y] = (uint16_t)id;
        }
    #undef E
    for (int y = 0; y < TS; y++) for (int x = 0; x < TS; x++) tput(t, x, y, sh(R_STONE, GV(x, y)));
}

/* a face cut with the pick: bands of strokes, one way then the other, drill holes down it; vseed has the
 * scar of a charge fired */
static void gen_mineface(int t, int m, uint32_t seed, uint32_t vseed) {
    for (int y = 0; y < TS; y++)
        for (int x = 0; x < TS; x++) {
            GV(x, y) = 0.48f + 0.14f * (fbm(x / 8.0f, y / 8.0f, 8, seed) - 0.5f) + 0.05f * (fbm(x / 2.0f, y / 2.0f, 32, seed + 1) - 0.5f);
            g_mid[m][x * TS + y] = 0;                                 /* rock: a blend frays it, it doesn't fall away in blocks */
        }
    for (int k = 0; k < 48; k++) {                                    /* the pick's work all over it: patches of short strokes */
        float cx = h01(k, 4, seed) * 64.0f, cy = h01(k, 5, seed) * 64.0f, a = (k & 1 ? 0.95f : 0.55f) + 0.3f * (h01(k, 6, seed) - 0.5f);
        float rad = 4.0f + h01(k, 7, seed) * 4.0f, ca = cosf(a), sa = sinf(a);
        for (int yy = -9; yy <= 9; yy++)
            for (int xx = -9; xx <= 9; xx++) {
                float d = sqrtf((float)(xx * xx + yy * yy));
                if (d > rad) continue;
                float across = xx * -sa + yy * ca, f = 1.0f - d / rad, g = fmodf(across + 30.0f, 2.6f);
                if (g < 0.8f) GV((int)cx + xx, (int)cy + yy) -= 0.2f * f; else if (g < 1.4f) GV((int)cx + xx, (int)cy + yy) += 0.1f * f;
            }
    }
    for (int k = 0; k < (vseed ? 2 : 1); k++) {                       /* drill holes, half of each left in the face */
        float dx = 14.0f + k * 30.0f + 8.0f * h01(k, 3, seed), y0 = 6.0f + 20.0f * h01(k, 4, seed), len = 18.0f + 14.0f * h01(k, 5, seed);
        for (int y = (int)y0; y < (int)(y0 + len); y++) {
            int x = (int)(dx + (y - y0) * 0.08f);
            GV(x, y) = 0.08f; GV(x - 1, y) -= 0.1f; GV(x + 1, y) += 0.1f; GV(x + 2, y) += 0.04f;
        }
    }
    if (vseed)                                                        /* a charge's scar: rock blown out, soot round it */
        for (int y = 0; y < TS; y++)
            for (int x = 0; x < TS; x++) {
                float d = hypotf(wrapd(x + 0.5f - 36.0f), wrapd(y + 0.5f - 26.0f)) + 3.0f * fbm(x / 8.0f, y / 8.0f, 8, vseed);
                if (d < 7.0f) GV(x, y) = 0.12f + 0.08f * (d / 7.0f); else if (d < 12.0f) GV(x, y) -= 0.12f * (1.0f - (d - 7.0f) / 5.0f);
            }
    for (int y = 0; y < TS; y++) for (int x = 0; x < TS; x++) tput(t, x, y, sh(R_STONE, GV(x, y)));
}

/* timber shoring: boards laid flat against the rock, two to a course, a post at each edge so that two
 * cells side by side make one post between them; with the rot the boards split and some come down. vseed
 * has a board gone */
static void gen_timber(int t, int m, uint32_t seed, uint32_t vseed, float dk) {
    int gone = vseed ? 1 + (int)(hsh(9, 1, vseed) % 8) : -1;
    for (int y = 0; y < TS; y++)
        for (int x = 0; x < TS; x++) {
            int b = y / 8, ly = y & 7, id = 1 + b;
            float v;
            uint8_t ramp = R_WOOD;
            int lost = id == gone || (dk > 0.3f && h01(b, 7, seed + 3) < 0.25f * dk && x > 8 && x < 56 && fbm(x / 16.0f, b * 1.3f, 4, seed + 4) > 0.45f);
            if (lost) { ramp = R_STONE; v = 0.18f + 0.1f * (fbm(x / 4.0f, y / 4.0f, 16, seed + 5) - 0.5f); id = 0; }   /* the rock behind */
            else {
                v = 0.45f + 0.12f * (h01(b, 1, seed) - 0.5f) + 0.06f * sinf(y * 1.7f + x * 0.05f + b) + 0.08f * (fbm(x / 16.0f, y / 2.0f, 4, seed + 2) - 0.5f);   /* the grain */
                if (ly == 0) v = 0.12f; else if (ly == 1) v += 0.08f; else if (ly == 7) v -= 0.1f;
                if (((x + b * 11) & 31) == 15 && (ly == 2 || ly == 5)) { ramp = R_IRON; v = 0.6f; }   /* the nails */
                if (dk > 0.15f && fabsf(fmodf(x * 0.3f + b * 7.0f, 13.0f) - 6.5f) < 0.4f && ly > 1 && ly < 7 && h01(b, (int)(x * 0.3f), seed + 6) < dk) v -= 0.2f;   /* split with the years */
            }
            if (x < 5 || x > 58) {                                    /* half a post at each edge */
                int px = x < 5 ? x : 63 - x;
                ramp = R_WOOD; id = 0;
                v = 0.3f + 0.1f * (px / 5.0f) + 0.05f * (fbm(x / 2.0f, y / 8.0f, 32, seed + 7) - 0.5f) + (x == 5 || x == 58 ? -0.1f : 0.0f);
                if (px == 4) v -= 0.14f;
            }
            GV(x, y) = v;
            tput(t, x, y, sh(ramp, v));
            g_mid[m][x * TS + y] = (uint16_t)id;
        }
}

/* basalt in columns, the joints between them, cracks across, in that ramp (the Hollows' iron-grey); vseed has a column broken */
static void gen_basalt(int t, int m, uint32_t seed, uint32_t vseed, int ramp) {
    int cx[8], cw[8], n = 0, x = 0;
    while (x < 64 && n < 8) { int w = 9 + (int)(h01(n, 1, seed) * 6.0f); if (64 - x - w < 8) w = 64 - x; cx[n] = x; cw[n] = w; x += w; n++; }
    int broke = vseed ? (int)(hsh(10, 1, vseed) % (uint32_t)n) : -1;
    for (int y = 0; y < TS; y++)
        for (int xx = 0; xx < TS; xx++) {
            int k = 0;
            while (k + 1 < n && xx >= cx[k + 1]) k++;
            int px = xx - cx[k], w = cw[k];
            float facet = px < w / 3 ? 0.08f : (px < 2 * w / 3 ? 0.0f : -0.08f);   /* three of its faces showing */
            float v = 0.45f + 0.08f * (h01(k, 2, seed) - 0.5f) + facet + 0.06f * (fbm(xx / 4.0f, y / 8.0f, 16, seed + 3) - 0.5f);
            int seg = (int)((y + 64.0f * h01(k, 4, seed)) / (14.0f + 8.0f * h01(k, 5, seed)));
            float cy = fmodf(y + 64.0f * h01(k, 4, seed) + (px - w / 2) * 0.3f, 14.0f + 8.0f * h01(k, 5, seed));
            int id = 1 + k * 6 + (seg % 6);
            if (px == 0) { v = 0.06f; id = 0; } else if (px == 1) v += 0.06f;
            if (cy < 0.9f) { v = 0.1f; id = 0; } else if (cy < 1.8f) v += 0.05f;
            if (k == broke && y > 24 && y < 40) v = 0.05f + 0.06f * h01(xx, y, vseed);   /* a piece of it gone */
            if (h01(xx, y, seed + 6) > 0.985f) v += 0.12f;
            tput(t, xx, y, sh(ramp, v));
            g_mid[m][xx * TS + y] = (uint16_t)id;
        }
}

/* packed backfill: earth and broken rock tipped in, and what was tipped with it; vseed shows a skull */
static void gen_backfill(int t, int m, uint32_t seed, uint32_t vseed) {
    static uint8_t ramp[TS * TS];
    for (int y = 0; y < TS; y++)
        for (int x = 0; x < TS; x++) {
            GV(x, y) = 0.36f + 0.08f * (fbm(x / 8.0f, y / 8.0f, 8, seed) - 0.5f) + 0.06f * (fbm(x / 2.0f, y / 2.0f, 32, seed + 1) - 0.5f);
            ramp[x * TS + y] = R_FLOOR; g_mid[m][x * TS + y] = 0;
        }
    int n = 0;
    float px[30], py[30], pr[30];
    for (int k = 0; k < 90 && n < 30; k++) {                         /* broken rock, every size */
        float cx = h01(k, 3, seed) * 64.0f, cy = h01(k, 4, seed) * 64.0f, r = 1.8f + h01(k, 5, seed) * 4.5f;
        int ok = 1;
        for (int j = 0; j < n && ok; j++) if (hypotf(wrapd(cx - px[j]), wrapd(cy - py[j])) < r + pr[j] + 0.8f) ok = 0;
        if (!ok) continue;
        px[n] = cx; py[n] = cy; pr[n] = r; n++;
        tx_blob(m, n, cx, cy, r * (1.0f + 0.5f * h01(k, 6, seed)), r, 0.34f + 0.22f * h01(k, 7, seed), h01(k, 8, seed) < 0.3f ? R_FLOOR : R_STONE, ramp, 1.0f);
    }
    cv_begin(cv_sprite, TS, TS);                                      /* bones in it */
    for (int k = 0; k < 6; k++) {
        float x0 = h01(k, 11, seed) * 64.0f, y0 = h01(k, 12, seed) * 64.0f, a = h01(k, 13, seed) * PI_F, len = 8.0f + h01(k, 14, seed) * 8.0f;
        wrap_draw(0, x0, y0, x0 + cosf(a) * len, y0 + sinf(a) * len, 1.1f, 0.7f);
    }
    if (vseed) wrap_draw(1, 20.0f + h01(6, 1, vseed) * 24.0f, 30.0f, 0.85f, 0.8f, 0, 0);
    for (int y = 0; y < TS; y++)
        for (int x = 0; x < TS; x++) {
            uint8_t c = cv_sprite[y * TS + x];
            if (c != C_CLEAR && fbm(x / 8.0f, y / 8.0f, 8, seed + 2) < 0.62f) { tput(t, x, y, c); continue; }
            float v = GV(x, y);
            if (ramp[x * TS + y] == R_FLOOR && !g_mid[m][x * TS + y] && g_mid[m][x * TS + ((y - 1) & 63)]) v -= 0.12f;
            tput(t, x, y, sh(ramp[x * TS + y], v));
        }
}

/* the Hollows' rot: worked, then left. Dust and grime; scree come down at the foot of the wall; claw
 * marks late, where something went at the stone */
static void decay_hollows(int t, float dk, uint32_t field, uint32_t own, int marked) {
    uint8_t *p = crawl_tex[t];
    for (int y = 0; y < TS; y++)
        for (int x = 0; x < TS; x++) {
            uint8_t c = p[x * TS + y];
            if (c == C_CLEAR || IS_FULLBRIGHT(c)) continue;
            float n = fbm(x / 8.0f, y / 8.0f, 8, field + 5);
            if (n < 0.16f + 0.2f * dk) c = darker(c, 1 + (n < 0.12f + 0.1f * dk));
            float sc = 62.0f - 12.0f * dk - 5.0f * fbm(x / 8.0f, 0.5f, 8, field + 6);   /* the scree banked against it */
            if (dk > 0.2f && y > sc && h01(x, y, field + 7) > 0.2f) c = sh(h01(x, y, field + 8) < 0.5f ? R_STONE : R_FLOOR, 0.25f + 0.3f * h01(x / 2, y / 2, field + 9));
            p[x * TS + y] = c;
        }
    #define DK(xx, yy, by) do { uint8_t *q_ = &p[((xx) & 63) * TS + ((yy) & 63)]; if (*q_ != C_CLEAR && !IS_FULLBRIGHT(*q_)) *q_ = darker(*q_, by); } while (0)
    int cracks = (int)(dk * 4.0f + 0.5f);
    for (int k = 0; k < cracks; k++) {
        float x = h01(k, 3, own) * 64.0f, y = h01(k, 4, own) * 64.0f, a = h01(k, 5, own) * TAU_F;
        int len = 8 + (int)(h01(k, 6, own) * 22.0f);
        for (int i = 0; i < len; i++) { a += (h01(k, i + 10, own) - 0.5f) * 1.2f; x += cosf(a); y += sinf(a); DK((int)x, (int)y, 5); }
    }
    if (marked && dk > 0.5f && h01(5, 5, own) < (dk - 0.5f) * 1.4f) {   /* something's claws, four lines dragged down the stone (on one look of each, so they don't repeat) */
        float x0 = 12.0f + 36.0f * h01(6, 6, own), y0 = 12.0f + 14.0f * h01(7, 7, own), a = 1.2f + 0.5f * h01(8, 8, own);
        for (int c = 0; c < 4; c++)
            for (int i = 0; i < 22; i++) {
                int x = (int)(x0 + c * 3.0f + cosf(a) * i), y = (int)(y0 + sinf(a) * i - 0.04f * (c - 1.5f) * i);
                DK(x, y, 6); if ((x & 1) == 0) DK(x + 1, y, 2);
            }
    }
    #undef DK
}

/* ---- what stands on the Hollows' walls ---- */

static void gen_feat_ore(int t, uint32_t seed) {                   /* a seam across the rock: gold in it, copper gone green, the picks' marks about it */
    cv_begin(cv_sprite, TS, TS);
    for (int x = 2; x < 62; x++)
        for (int y = 0; y < TS; y++) {
            float cy = 22.0f + x * 0.35f + 4.0f * sinf(x * 0.13f + h01(1, 1, seed) * 6.0f), w = 3.0f + 2.5f * fbm(x / 8.0f, 0.5f, 8, seed);
            float d = fabsf(y + 0.5f - cy);
            if (d > w) continue;
            float r = h01(x, y, seed + 2);
            uint8_t c = sh(R_STONE, 0.18f + 0.1f * h01(x, y, seed + 3));
            if (r > 0.8f) c = sh(R_GOLD, 0.45f + 0.5f * h01(x, y, seed + 4));
            else if (r > 0.62f) c = sh(R_MOSS, 0.4f + 0.3f * h01(x, y, seed + 5));
            if (d > w - 1.0f) c = sh(R_STONE, 0.5f);
            cv_px(x, y, c);
        }
    for (int k = 0; k < 9; k++) {                                     /* the picks' work about it */
        float x = 6.0f + h01(k, 6, seed) * 52.0f, y = 14.0f + h01(k, 7, seed) * 40.0f;
        for (int i = 0; i < 4; i++) cv_px((int)x + i, (int)y + i / 2, sh(R_STONE, 0.08f));
    }
    for (int k = 0; k < 4; k++) cv_px(8 + (int)(h01(k, 8, seed) * 48.0f), 26 + (int)(h01(k, 9, seed) * 18.0f), C_WHITE);
    cv_to_tex(t);
}
static void gen_feat_fossil(int t, uint32_t seed) {                /* the ribs and spine of something older than the dead, set in the stone */
    cv_begin(cv_sprite, TS, TS);
    for (int i = 0; i < 46; i++) {                                    /* the spine, curving */
        float x = 9.0f + i, y = 20.0f + 6.0f * sinf(i * 0.09f + 0.4f);
        cv_ellipse(x, y, 1.4f, 2.0f, R_BONE, 0.3f, 0.66f);
        if ((i % 3) == 0) cv_px((int)x, (int)y - 2, sh(R_BONE, 0.2f));
    }
    for (int r = 0; r < 8; r++) {                                     /* its ribs, sweeping down and back */
        float x0 = 13.0f + r * 5.2f, y0 = 20.0f + 6.0f * sinf((x0 - 9.0f) * 0.09f + 0.4f), len = 22.0f + 6.0f * sinf(r * 0.5f);
        for (int i = 0; i < (int)len; i++) {
            float u = i / len, x = x0 - 6.0f * u * u, y = y0 + i;
            cv_px((int)x, (int)y, sh(R_BONE, 0.62f - 0.25f * u)); cv_px((int)x + 1, (int)y, sh(R_BONE, 0.4f - 0.2f * u)); cv_px((int)x - 1, (int)y, sh(R_STONE, 0.12f));
        }
    }
    cv_ellipse(54, 17, 6.0f, 4.5f, R_BONE, 0.3f, 0.7f);              /* the skull, long and low */
    cv_flat(55.5f, 16.5f, 1.6f, 1.4f, sh(R_STONE, 0.05f));
    for (int i = 0; i < 5; i++) cv_px(50 + i * 2, 20, sh(R_BONE, 0.85f));
    (void)seed;
    cv_to_tex(t);
}
static void gen_feat_frame(int t, uint32_t seed) {                 /* a mine set: two posts and a cap across, wedged tight */
    cv_begin(cv_sprite, TS, TS);
    for (int s = 0; s < 2; s++)
        for (int y = 8; y < TS; y++)
            for (int x = 0; x < 7; x++) {
                int xx = s ? 56 + x : 1 + x;
                float v = 0.3f + 0.04f * x + 0.06f * (fbm(xx / 2.0f, y / 8.0f, 32, seed + s) - 0.5f) + (x == 0 ? -0.1f : x == 6 ? -0.12f : 0.0f);
                cv_px(xx, y, sh(R_WOOD, v));
            }
    for (int y = 3; y < 11; y++)
        for (int x = 0; x < TS; x++) {
            float v = 0.38f + 0.06f * sinf(x * 0.25f + y) + 0.05f * (fbm(x / 8.0f, y / 2.0f, 8, seed + 2) - 0.5f) + (y == 3 ? 0.1f : y == 10 ? -0.14f : 0.0f);
            cv_px(x, y, sh(R_WOOD, v));
        }
    for (int s = 0; s < 2; s++) for (int y = 11; y < 15; y++) for (int x = 0; x < 4; x++) cv_px(s ? 52 + x : 8 + x, y, sh(R_WOOD, 0.5f - 0.05f * (y - 11)));   /* the wedges */
    cv_to_tex(t);
}
static void gen_feat_chute(int t, uint32_t seed) {                 /* an ore chute out of the wall, its hatch, the gravel it spilled */
    cv_begin(cv_sprite, TS, TS);
    for (int y = 22; y < 44; y++)
        for (int x = 18; x < 46; x++) {
            float drop = (x - 18) * 0.2f;
            if (y < 22 + drop || y > 34 + drop) continue;
            int side = y > 31 + drop;
            float v = side ? 0.28f : 0.42f + 0.05f * sinf(x * 0.6f);
            if (y < 23 + drop) v = 0.55f;
            cv_px(x, y, sh(R_WOOD, v + 0.05f * (fbm(x / 4.0f, y / 4.0f, 16, seed) - 0.5f)));
        }
    for (int y = 23; y < 32; y++) for (int x = 18; x < 22; x++) cv_px(x, y, sh(R_IRON, 0.35f + (x == 18 ? 0.2f : 0.0f)));   /* the hatch */
    for (int k = 0; k < 60; k++) {                                     /* spilled gravel down the wall's foot */
        float x = 40.0f + 10.0f * (h01(k, 1, seed) - 0.5f) * (1.0f + h01(k, 2, seed)), y = 46.0f + h01(k, 3, seed) * 17.0f;
        if (fabsf(x - 42.0f) > 3.0f + (y - 46.0f) * 0.7f) continue;
        cv_px((int)x, (int)y, sh(h01(k, 4, seed) < 0.5f ? R_STONE : R_FLOOR, 0.3f + 0.35f * h01(k, 5, seed)));
    }
    cv_to_tex(t);
}
static void gen_feat_marks(int t, uint32_t seed) {                 /* the cult's marks: an eye in a ring in old blood, charcoal signs about it */
    cv_begin(cv_sprite, TS, TS);
    for (int a = 0; a < 120; a++) {
        float ang = TAU_F * a / 120.0f, r = 15.0f + 0.6f * sinf(ang * 7.0f);
        for (float w = 0.0f; w < 1.6f; w += 0.8f) cv_px((int)(32.0f + (r + w) * cosf(ang)), (int)(28.0f + (r + w) * sinf(ang)), sh(R_BLOOD, 0.3f + 0.15f * h01(a, 1, seed)));
    }
    for (int i = 0; i <= 24; i++) {                                   /* the triangle inside it */
        float u = i / 24.0f;
        cv_px((int)(32.0f - 11.0f + 11.0f * u), (int)(36.0f - 17.0f * u), sh(R_BLOOD, 0.35f));
        cv_px((int)(32.0f + 11.0f * u), (int)(19.0f + 17.0f * u), sh(R_BLOOD, 0.35f));
        cv_px((int)(21.0f + 22.0f * u), 36, sh(R_BLOOD, 0.35f));
    }
    for (int y = 25; y <= 31; y++) for (int x = 26; x <= 38; x++) {   /* the eye */
        float dx = (x + 0.5f - 32.0f) / 6.0f, dy = (y + 0.5f - 28.0f) / 3.0f;
        if (dx * dx + dy * dy <= 1.0f) cv_px(x, y, dx * dx + dy * dy < 0.25f ? sh(R_STONE, 0.05f) : sh(R_BONE, 0.55f));
    }
    for (int k = 0; k < 6; k++) {                                     /* the drips under the ring */
        int x = 20 + (int)(h01(k, 2, seed) * 24.0f), len = 4 + (int)(h01(k, 3, seed) * 12.0f);
        for (int y = 0; y < len; y++) cv_px(x, 43 + y, sh(R_BLOOD, 0.28f));
    }
    for (int k = 0; k < 5; k++) {                                     /* charcoal signs */
        int x = 4 + (int)(h01(k, 4, seed) * 54.0f), y = 48 + (int)(h01(k, 5, seed) * 10.0f);
        if (x > 18 && x < 46 && y < 52) continue;
        for (int i = 0; i < 5; i++) { cv_px(x + i, y, sh(R_STONE, 0.06f)); cv_px(x + 2, y - 2 + i, sh(R_STONE, 0.06f)); }
    }
    cv_to_tex(t);
}
static void gen_feat_lamp(int t, int frame, uint32_t seed) {       /* a miner's oil lamp hung on a spike driven in the rock */
    cv_begin(cv_sprite, TS, TS);
    for (int x = 30; x <= 36; x++) cv_px(x, 22, sh(R_IRON, 0.65f));    /* the spike */
    cv_px(36, 23, sh(R_IRON, 0.5f)); cv_px(36, 24, sh(R_IRON, 0.5f));
    for (int y = 23; y <= 27; y++) cv_px(33, y, sh(R_IRON, 0.45f));    /* the hanger */
    for (int y = 28; y <= 36; y++)                                     /* the lamp: a squat brass pot, its spout */
        for (int x = 27; x <= 39; x++) {
            float dx = (x + 0.5f - 33.0f) / 6.0f, dy = (y + 0.5f - 33.0f) / 4.5f;
            if (dx * dx + dy * dy > 1.0f) continue;
            cv_px(x, y, sh(R_GOLD, 0.22f + 0.45f * lit(dx, dy, sqrtf(fmaxf(0.0f, 1.0f - dx * dx - dy * dy)))));
        }
    for (int x = 38; x <= 42; x++) cv_px(x, 30 - (x - 38) / 2, sh(R_GOLD, 0.4f));
    static const float FH[3] = { 5.0f, 6.0f, 4.0f }, FS[3] = { 0.0f, 0.6f, -0.6f };
    for (int y = 0; y < 7; y++) {
        float u = y / FH[frame];
        if (u > 1.0f) break;
        int x = (int)(42.5f + FS[frame] * u);
        cv_px(x, 27 - y, (uint8_t)(R_FLAME + 15 - 2 * y));
        if (u < 0.5f) cv_px(x + 1, 27 - y, (uint8_t)(R_FLAME + 10));
    }
    (void)seed;
    cv_to_tex(t);
}

/* ---- the Hollows' floors and ceilings ---- */

static void gen_floor_gravel(int t, uint32_t seed, int scree) {    /* gravel trodden flat; fallen scree on it as the run goes */
    for (int y = 0; y < TS; y++)
        for (int x = 0; x < TS; x++) {
            float v = 0.38f + 0.08f * (fbm(x / 8.0f, y / 8.0f, 8, seed) - 0.5f);
            float r = h01(x, y, seed + 1);
            uint8_t c = sh(R_FLOOR, v + (r > 0.7f ? 0.1f : r < 0.25f ? -0.08f : 0.0f));
            if (r > 0.92f) c = sh(R_STONE, 0.45f + 0.2f * h01(x, y, seed + 2));
            tput(t, x, y, c);
        }
    if (scree) {                                                      /* rock come down from the roof */
        static uint8_t ramp[TS * TS];
        for (int i = 0; i < TS * TS; i++) { g_v[i] = 0.0f; ramp[i] = 0; }
        for (int k = 0; k < 9; k++) {
            float cx = 6.0f + h01(k, 3, seed) * 52.0f, cy = 6.0f + h01(k, 4, seed) * 52.0f, r = 2.5f + 4.0f * h01(k, 5, seed);
            tx_blob(-1, 0, cx, cy, r * (1.0f + 0.4f * h01(k, 6, seed)), r, 0.3f + 0.25f * h01(k, 7, seed), R_STONE, ramp, 1.0f);
        }
        for (int y = 0; y < TS; y++) for (int x = 0; x < TS; x++) if (ramp[x * TS + y]) tput(t, x, y, sh(R_STONE, GV(x, y)));
    }
}
static void gen_floor_rails(int t, uint32_t seed) {                /* a tramway's rails on their sleepers, laid as laid, along the room */
    gen_floor_gravel(t, seed, 0);
    for (int y = 0; y < TS; y++)
        for (int x = 0; x < TS; x++) {
            int sl = (y & 15) >= 4 && (y & 15) <= 9;
            if (sl && x >= 10 && x <= 53) tput(t, x, y, sh(R_WOOD, 0.3f + 0.06f * ((y & 15) == 4 ? 1.0f : 0.0f) + 0.05f * (fbm(x / 4.0f, y / 2.0f, 16, seed + 3) - 0.5f) - (x == 10 || x == 53 ? 0.08f : 0.0f)));
            if ((x >= 18 && x <= 20) || (x >= 43 && x <= 45)) tput(t, x, y, sh(R_IRON, x == 19 || x == 44 ? 0.72f : 0.32f));
        }
}
static void gen_floor_trodden(int t, uint32_t seed) {              /* the halls' floor: earth trodden hard, ruts worn in it */
    for (int y = 0; y < TS; y++)
        for (int x = 0; x < TS; x++) {
            float v = 0.4f + 0.12f * (fbm(x / 8.0f, y / 8.0f, 8, seed) - 0.5f) + 0.05f * (fbm(x / 2.0f, y / 2.0f, 32, seed + 1) - 0.5f);
            if (fbm(x / 4.0f, y / 4.0f, 16, seed + 3) > 0.62f) v -= 0.07f;   /* worn hollows */
            uint8_t c = sh(R_FLOOR, v + (h01(x, y, seed + 2) > 0.95f ? 0.12f : 0.0f));
            if (h01(x, y, seed + 4) > 0.985f) { c = sh(R_STONE, 0.5f); tput(t, x + 1, y + 1, sh(R_FLOOR, v - 0.12f)); }   /* a pebble */
            tput(t, x, y, c);
        }
}
static void gen_ceil_lagging(int t, uint32_t seed) {               /* boards across the roof, a beam every half cell */
    for (int y = 0; y < TS; y++)
        for (int x = 0; x < TS; x++) {
            int b = x / 8, lx = x & 7;
            float v = 0.42f + 0.1f * (h01(b, 1, seed) - 0.5f) + 0.06f * sinf(y * 0.5f + b) + 0.05f * (fbm(x / 2.0f, y / 8.0f, 32, seed + 2) - 0.5f);
            if (lx == 0) v = 0.1f; else if (lx == 1) v += 0.06f;
            uint8_t c = sh(R_WOOD, v);
            if ((y & 31) < 6) c = sh(R_WOOD, 0.3f + ((y & 31) == 0 ? 0.12f : (y & 31) == 5 ? -0.12f : 0.0f));   /* a beam */
            tput(t, x, y, c);
        }
}
static void gen_ceil_roots(int t, uint32_t seed) {                 /* rough rock, roots come through it */
    gen_ceil_rock(t, seed);
    for (int k = 0; k < 5; k++) {
        float x = h01(k, 1, seed) * 64.0f, y = h01(k, 2, seed) * 64.0f, a = h01(k, 3, seed) * TAU_F;
        for (int i = 0; i < 26; i++) {
            a += (h01(k, i + 5, seed) - 0.5f) * 0.9f; x += cosf(a); y += sinf(a);
            tput(t, (int)x, (int)y, sh(R_WOOD, 0.22f + 0.1f * h01(k, i, seed))); tput(t, (int)x + 1, (int)y, sh(R_WOOD, 0.12f));
            if (i == 12) { float b = a + 1.2f, bx = x, by = y; for (int j = 0; j < 9; j++) { bx += cosf(b); by += sinf(b); tput(t, (int)bx, (int)by, sh(R_WOOD, 0.18f)); } }
        }
    }
}

/* ---- the Hollows' doors, keys and treasure ---- */

static void gen_minedoor(int t, uint32_t seed, int metal) {        /* rough planks braced in a Z; a vault's chained across and locked in its key's metal */
    int vault = metal >= 0;
    for (int y = 0; y < TS; y++)
        for (int x = 0; x < TS; x++) {
            int p = x / 11, lx = x % 11;
            float v = 0.36f + 0.12f * (h01(p, 0, seed) - 0.5f) + 0.07f * sinf(y * 0.6f + p * 2.0f) + 0.08f * (fbm(x / 4.0f, y / 16.0f, 16, seed) - 0.5f);
            if (lx == 0) v = 0.06f; else if (lx == 1) v += 0.06f;
            float d = fabsf((y - 12.0f) - (52.0f - 12.0f) * (1.0f - x / 63.0f));   /* the brace, corner to corner */
            if ((y >= 8 && y <= 14) || (y >= 50 && y <= 56)) v = 0.48f + (y == 8 || y == 50 ? 0.1f : 0.0f) - (y == 14 || y == 56 ? 0.14f : 0.0f);
            else if (d < 3.2f) v = 0.46f - (d > 2.4f ? 0.12f : 0.0f);
            tput(t, x, y, sh(R_WOOD, v));
        }
    if (!vault)
        for (int y = 28; y <= 36; y++) for (int x = 48; x <= 54; x++) tput(t, x, y, sh(R_IRON, 0.35f + (x == 48 || y == 28 ? 0.25f : 0.0f)));   /* the hasp */
    else {
        float lo = metal == R_GOLD ? 0.1f : 0.25f;
        for (int c = 0; c < 2; c++)                                   /* chains across it, a padlock where they meet */
            for (int i = 0; i < 64; i += 4) {
                float y = 20.0f + c * 24.0f + 3.0f * sinf(i * 0.1f);
                for (int a = 0; a < 16; a++) { float ang = TAU_F * a / 16.0f; tput(t, i + 2 + (int)(2.2f * cosf(ang)), (int)y + (int)(1.4f * sinf(ang)), sh(metal, lo + 0.6f + 0.3f * -sinf(ang))); }
            }
        for (int y = 30; y <= 44; y++)
            for (int x = 25; x <= 38; x++) {
                float dx = (x + 0.5f - 31.5f) / 6.5f, dy = (y + 0.5f - 38.0f) / 6.0f;
                if (y < 34) { if (fabsf(fabsf(x + 0.5f - 31.5f) - 4.0f) < 1.1f) tput(t, x, y, sh(metal, lo + 0.5f)); continue; }
                if (dx * dx + dy * dy > 1.0f) continue;
                float v = lo + 0.55f * lit(dx, dy, sqrtf(1.0f - dx * dx - dy * dy)) + 0.2f;
                if (fabsf(x + 0.5f - 31.5f) < 1.0f && y > 37 && y < 42) v = 0.0f;
                tput(t, x, y, v <= 0.0f ? 0 : sh(metal, v));
            }
    }
}
static void gen_timberjamb(int t, uint32_t seed) {                 /* the doorway's side: the rock, and a post set against it (after the blends: it reuses the granite's blocks) */
    gen_granite(t, 0, seed + 5, 0);
    for (int y = 0; y < TS; y++)
        for (int x = 0; x < 14; x++) tput(t, x, y, sh(R_WOOD, 0.32f + 0.03f * x - (x == 13 ? 0.12f : 0.0f) + 0.05f * (fbm(x / 2.0f, y / 8.0f, 32, seed) - 0.5f)));
}
static void spr_key_tag(int id, float scale, int metal) {          /* a heavy key, a tally tag on its ring */
    cv_begin(cv_sprite, TS, TS);
    #define KX(x) (32.0f + ((x) - 32.0f) * scale)
    float ar = fmaxf(fabsf(scale), 0.18f), lo = metal == R_GOLD ? 0.35f : 0.45f;
    cv_ellipse(KX(20), 38, 6.0f * ar, 6.0f, metal, lo, 1.0f);       /* the bow, broad */
    cv_flat(KX(20), 38, 2.6f * ar, 2.6f, C_CLEAR);
    cv_capsule(KX(25), 38, KX(48), 38, 2.4f, metal, lo, 1.0f);       /* a thick shank */
    for (int y = 38; y <= 47; y++) for (int x = 42; x <= 49; x++) { if ((x == 44 || x == 47) && y >= 43) continue; cv_px((int)KX(x + 0.5f), y, sh(metal, lo + (1.0f - lo) * (x == 42 || y == 38 ? 0.95f : 0.62f))); }
    for (int a = 0; a < 20; a++) { float ang = TAU_F * a / 20.0f; cv_px((int)KX(14.0f + 2.5f * cosf(ang)), (int)(44.0f + 2.5f * sinf(ang)), sh(R_IRON, 0.6f)); }   /* the ring */
    cv_ellipse(KX(12.5f), 51.5f, 4.2f * ar, 4.2f, R_GOLD, 0.25f, 0.7f);   /* its brass tag */
    for (int i = 0; i < 3; i++) cv_px((int)KX(11.0f + i * 1.5f), 51, sh(R_GOLD, 0.05f));   /* the tally scratched on it */
    #undef KX
    cv_outline(sh(metal, 0.05f));
    cv_store(id);
}
static void spr_nuggets(int id, int frame) {                       /* gold as it came out of the rock */
    cv_begin(cv_sprite, TS, TS);
    cv_flat(32, 62, 16, 2.2f, R_WARM + 1);
    for (int i = 0; i < 16; i++) {
        float x = 20.0f + h01(i, 1, 46) * 24.0f, y = 60.0f - h01(i, 2, 46) * 6.0f * (1.0f - fabsf(x - 32.0f) / 14.0f), r = 1.6f + h01(i, 3, 46) * 2.2f;
        for (int yy = (int)(y - r); yy <= (int)(y + r); yy++)
            for (int xx = (int)(x - r - 1); xx <= (int)(x + r + 1); xx++) {
                float dx = (xx + 0.5f - x) / (r + 0.6f * h01(i, xx, 47)), dy = (yy + 0.5f - y) / r;
                if (dx * dx + dy * dy > 1.0f) continue;
                cv_px(xx, yy, sh(R_GOLD, 0.3f + 0.65f * lit(dx, dy, sqrtf(fmaxf(0.0f, 1.0f - dx * dx - dy * dy)))));
            }
    }
    cv_outline(0);
    static const int G[2][3][2] = { { {27, 55}, {37, 57}, {31, 52} }, { {29, 57}, {24, 58}, {39, 55} } };
    for (int i = 0; i < 3; i++) if (cvp[G[frame][i][1] * TS + G[frame][i][0]] != C_CLEAR) cv_px(G[frame][i][0], G[frame][i][1], C_WHITE);
    cv_store(id);
}
static void spr_rawgems(void) {                                    /* gems still in a lump of their rock */
    cv_begin(cv_sprite, TS, TS);
    cv_flat(32, 62, 15, 2.0f, R_WARM + 1);
    for (int y = 46; y <= 61; y++)
        for (int x = 18; x <= 46; x++) {
            float dx = (x + 0.5f - 32.0f) / 14.0f, dy = (y + 0.5f - 55.0f) / 8.0f;
            if (dx * dx + dy * dy * 1.3f > 1.0f) continue;
            cv_px(x, y, sh(R_STONE, 0.25f + 0.35f * lit(dx, dy, 0.6f) + 0.08f * h01(x, y, 48)));
        }
    static const float C[5][4] = { { 24, 52, 3.5f, R_BLOOD }, { 33, 49, 4.5f, R_GLOW }, { 40, 54, 3.0f, R_MOSS }, { 29, 57, 2.6f, R_PURPLE }, { 37, 58, 2.2f, R_BLOOD } };
    for (int i = 0; i < 5; i++) {                                     /* crystals standing out of it */
        float cx = C[i][0], cy = C[i][1], r = C[i][2];
        for (int y = (int)(cy - r * 1.6f); y <= (int)(cy + r * 0.4f); y++)
            for (int x = (int)(cx - r); x <= (int)(cx + r); x++) {
                float u = (x + 0.5f - cx) / r, w = 1.0f - (cy + r * 0.4f - y) / (r * 2.0f);
                if (fabsf(u) > w) continue;
                cv_px(x, y, sh((int)C[i][3], u < -0.2f ? 0.95f : u < 0.3f ? 0.7f : 0.45f));
            }
    }
    cv_outline(0);
    cv_px(32, 46, C_WHITE); cv_px(23, 50, C_WHITE);
    cv_store(SP_GEMS);
}
static void spr_strongbox(void) {                                  /* a mine's strongbox, iron, its lid sprung, the take inside */
    cv_begin(cv_sprite, TS, TS);
    cv_flat(32, 62, 20, 2.2f, R_WARM + 1);
    float lid[8] = { 15, 44, 49, 44, 46, 32, 18, 32 };
    cv_poly(lid, 4, R_IRON, 0.5f, 0.2f, 32, 12);
    for (int i = 0; i < 14; i++) { float x = 18 + h01(i, 6, 49) * 28, y = 45 - h01(i, 7, 49) * 3; cv_ellipse(x, y, 2.6f, 1.6f, R_GOLD, 0.4f, 1.0f); }
    for (int y = 45; y <= 61; y++)
        for (int x = 14; x <= 50; x++) {
            float v = 0.36f + 0.06f * (fbm(x / 4.0f, y / 4.0f, 16, 50) - 0.5f) + (y == 45 ? 0.2f : 0.0f) - (x > 47 ? 0.1f : 0.0f);
            if ((x - 14) % 9 == 0 || y == 52) v = 0.55f;               /* its bands */
            if ((x - 14) % 9 == 0 && (y & 3) == 0) v = 0.85f;          /* their rivets */
            uint8_t c = sh(R_IRON, v);
            if (x >= 29 && x <= 35 && y >= 48 && y <= 56) c = sh(R_GOLD, (x == 29 || y == 48) ? 0.8f : 0.45f);
            if (x == 32 && (y == 51 || y == 52)) c = 0;
            cv_px(x, y, c);
        }
    cv_outline(0);
    cv_px(25, 43, C_WHITE);
    cv_store(SP_CHEST);
}

/* ---- the Deep's materials ---- */

/* the Deep's rot runs its own way: glittering as the run begins, burning through its middle, molten late,
 * and dark at the end, the fire gone out of the rock */
static float deep_glitter(float dk) { return clampf(1.0f - dk * 2.5f, 0.0f, 1.0f); }
static float deep_heat(float dk) { return clampf(1.0f - fabsf(dk - 0.62f) / 0.3f, 0.0f, 1.0f); }
static float deep_dark(float dk) { return clampf((dk - 0.75f) / 0.25f, 0.0f, 1.0f); }

/* obsidian: black glass broken in shells, each facet ribbed about the point it flaked from, the light sliding
 * over its curve, a hard glint along its upper edge; vseed has a great flake struck off it */
static void gen_obsidian(int t, int m, uint32_t seed, uint32_t vseed) {
    float px[9], py[9];
    static int k1[TS * TS];
    static float e[TS * TS];
    for (int i = 0; i < 9; i++) { px[i] = h01(i, 1, seed) * 64.0f; py[i] = h01(i, 2, seed) * 64.0f; }
    for (int y = 0; y < TS; y++)
        for (int x = 0; x < TS; x++) {
            float d1, d2;
            k1[x * TS + y] = voro(x, y, px, py, 9, &d1, &d2);
            e[x * TS + y] = d2 - d1;
        }
    float fx = 40.0f + 8.0f * h01(4, 4, vseed), fy = 26.0f + 8.0f * h01(5, 5, vseed);
    for (int y = 0; y < TS; y++)
        for (int x = 0; x < TS; x++) {
            int k = k1[x * TS + y];
            float ox = px[k] + (h01(k, 3, seed) - 0.5f) * 24.0f, oy = py[k] + (h01(k, 4, seed) - 0.5f) * 24.0f;
            float r = hypotf(wrapd(x + 0.5f - ox), wrapd(y + 0.5f - oy)), ring = sinf(r * (0.8f + 0.4f * h01(k, 5, seed)));
            float ang = h01(k, 7, seed) * TAU_F, gx = wrapd(x + 0.5f - px[k]) * (x + 0.5f > px[k] ? 1.0f : -1.0f), gy = wrapd(y + 0.5f - py[k]) * (y + 0.5f > py[k] ? 1.0f : -1.0f);
            float across = (cosf(ang) * gx + sinf(ang) * gy) / 20.0f - (h01(k, 8, seed) - 0.5f);
            float sheen = expf(-across * across * 9.0f);
            float v = 0.12f + 0.07f * (h01(k, 6, seed) - 0.5f) + 0.04f * ring + 0.03f * (fbm(x / 8.0f, y / 8.0f, 8, seed + 8) - 0.5f) + 0.3f * sheen * (0.65f + 0.35f * ring);
            if (vseed) {                                              /* the flake's scar: ridges close together, bright */
                float fr = hypotf(wrapd(x + 0.5f - fx), wrapd(y + 0.5f - fy));
                if (fr < 13.0f) v = 0.1f + 0.22f * (0.5f + 0.5f * sinf(fr * 1.9f)) * (1.0f - fr / 16.0f) + (fr < 2.0f ? 0.2f : 0.0f);
            }
            uint8_t c = sh(R_STONE, v);
            int up = k1[((x - 1) & 63) * TS + ((y - 1) & 63)] != k, down = k1[((x + 1) & 63) * TS + ((y + 1) & 63)] != k;
            int keen = h01(k, 9, seed) < 0.35f;                       /* some facets' edges catch the light, most don't */
            if (e[x * TS + y] < 0.9f) c = sh(R_STONE, 0.03f);         /* the break between facets */
            else if (up && e[x * TS + y] < 2.0f && keen) c = sh(R_STONE, 0.34f + 0.3f * sheen);
            else if (down && e[x * TS + y] < 2.0f) c = sh(R_STONE, 0.06f);
            else if (sheen > 0.9f && h01(x, y, seed + 9) > 0.97f) c = sh(R_GHOST, 0.45f);
            tput(t, x, y, c);
            g_mid[m][x * TS + y] = (uint16_t)(e[x * TS + y] < 0.9f ? 0 : 1 + k);
        }
}

/* rock crusted with ash: grey stone full of the holes its gas left, ash settled on every ledge and run down
 * from it; vseed has a vent's mouth, sulphur yellow about it */
static void gen_ashrock(int t, int m, uint32_t seed, uint32_t vseed) {
    for (int y = 0; y < TS; y++)
        for (int x = 0; x < TS; x++) {
            float s = y + 9.0f * (fbm(x / 16.0f, y / 32.0f, 4, seed + 1) - 0.5f) * 2.0f + 64.0f;
            float u = fmodf(s, 12.0f);
            float v = 0.32f + 0.12f * (fbm(x / 8.0f, y / 8.0f, 8, seed) - 0.5f) + 0.05f * (fbm(x / 2.0f, y / 2.0f, 32, seed + 2) - 0.5f) + 0.012f * u;
            uint8_t c = sh(R_STONE, v);
            float lip = 2.2f + 1.6f * fbm(x / 4.0f, s / 12.0f, 16, seed + 3);
            if (u < lip) c = sh(R_IRON, 0.58f + 0.14f * (fbm(x / 2.0f, y / 2.0f, 32, seed + 4) - 0.5f) + (u < 0.8f ? 0.08f : 0.0f));   /* the ash on the ledge */
            else if (u < lip + 1.0f) c = sh(R_STONE, 0.08f);          /* the shadow under it */
            else if (u < lip + 9.0f && fbm(x / 2.0f, y / 16.0f, 32, seed + 5) > 0.68f - 0.02f * (u - lip)) c = sh(R_IRON, 0.42f + 0.1f * h01(x, y, seed + 6));   /* run down the rock */
            else if (h01(x, y, seed + 7) > 0.975f) c = sh(R_IRON, 0.62f);
            tput(t, x, y, c);
            g_mid[m][x * TS + y] = 0;
        }
    for (int k = 0; k < 26; k++) {                                    /* the holes the gas left */
        float cx = h01(k, 11, seed) * 64.0f, cy = h01(k, 12, seed) * 64.0f, r = 0.8f + 1.6f * h01(k, 13, seed);
        for (int y = (int)(cy - r) - 1; y <= (int)(cy + r) + 1; y++)
            for (int x = (int)(cx - r * 1.3f) - 1; x <= (int)(cx + r * 1.3f) + 1; x++) {
                float dx = (x + 0.5f - cx) / (r * 1.3f), dy = (y + 0.5f - cy) / r, d = dx * dx + dy * dy;
                if (d <= 1.0f) tput(t, x, y, sh(R_STONE, dy > 0.4f ? 0.44f : 0.04f));
            }
    }
    if (vseed) {                                                      /* the vent's mouth, and its sulphur */
        float cx = 30.0f + 10.0f * h01(6, 1, vseed), cy = 40.0f;
        for (int y = (int)cy - 12; y <= (int)cy + 8; y++)
            for (int x = (int)cx - 14; x <= (int)cx + 14; x++) {
                float dx = (x + 0.5f - cx) / 6.0f, dy = (y + 0.5f - cy) / 3.5f, d = sqrtf(dx * dx + dy * dy) + 0.6f * (fbm(x / 4.0f, y / 4.0f, 16, vseed) - 0.5f);
                if (d < 1.0f) tput(t, x, y, sh(R_STONE, 0.02f + (dy > 0.5f ? 0.06f : 0.0f)));
                else if (d < 2.2f && h01(x, y, vseed + 1) < 0.75f - 0.3f * (d - 1.0f)) tput(t, x, y, sh(R_GOLD, 0.3f + 0.25f * h01(x, y, vseed + 2) - 0.1f * (d - 1.0f)));
            }
    }
}

/* crystal crust: the wall grown over with amethyst, point after point, each a pyramid of six faces seen end
 * on; a light down in the deepest (glow, 0 to 1); vseed has one great crystal */
static void gen_crystal(int t, int m, uint32_t seed, uint32_t vseed, float glow) {
    for (int y = 0; y < TS; y++)
        for (int x = 0; x < TS; x++) {
            tput(t, x, y, sh(R_PURPLE, 0.06f + 0.06f * fbm(x / 4.0f, y / 4.0f, 16, seed)));
            g_mid[m][x * TS + y] = 0;
        }
    const float LA = 3.93f;                                           /* the light, from the upper left */
    for (int k = 0; k < 34; k++) {
        float cx = h01(k, 1, seed) * 64.0f, cy = h01(k, 2, seed) * 64.0f, r = 5.0f + 5.5f * h01(k, 3, seed), rot = h01(k, 4, seed) * 1.05f;
        if (vseed && k == 33) { cx = 34.0f + 6.0f * h01(1, 1, vseed); cy = 30.0f; r = 14.0f; }
        int lit_core = h01(k, 5, seed) < 0.06f + 0.12f * glow;
        for (int y = (int)(cy - r) - 1; y <= (int)(cy + r) + 1; y++)
            for (int x = (int)(cx - r) - 1; x <= (int)(cx + r) + 1; x++) {
                float dx = x + 0.5f - cx, dy = y + 0.5f - cy, d = sqrtf(dx * dx + dy * dy);
                float a = atan2f(dy, dx) - rot + TAU_F * 2.0f, sec = fmodf(a, PI_F / 3.0f);
                float rr = r * 0.866f / cosf(sec - PI_F / 6.0f);
                if (d > rr) continue;
                float th = rot + (floorf(a / (PI_F / 3.0f)) + 0.5f) * PI_F / 3.0f;
                float v = 0.42f + 0.3f * cosf(th - LA) + 0.12f * (1.0f - d / rr);
                uint8_t c = sh(R_PURPLE, v);
                if (fminf(sec, PI_F / 3.0f - sec) * d < 0.5f && d > 1.0f) c = sh(R_PURPLE, v - 0.16f);   /* the ridge between two faces */
                if (d > rr - 0.9f) c = sh(R_PURPLE, 0.05f);           /* where it meets the next */
                else if (d < 1.0f) c = sh(R_PURPLE, 0.95f);           /* the point */
                else if (lit_core && d < rr * 0.45f && glow > 0.05f) c = (uint8_t)(R_GLOW + clampi((int)(3.0f + 9.0f * glow * (1.0f - d / (rr * 0.45f)) + 2.0f * cosf(th - LA)), 1, 13));
                tput(t, x, y, c);
                g_mid[m][(x & 63) * TS + (y & 63)] = (uint16_t)(1 + k);
            }
    }
}

/* bone in glass: the dead the fire caught, fused into the black glass, its sheen running over them; vseed has a
 * skull pressed into it */
static void gen_boneglass(int t, int m, uint32_t seed, uint32_t vseed) {
    gen_obsidian(t, m, seed, 0);
    cv_begin(cv_sprite, TS, TS);
    for (int k = 0; k < 9; k++) {
        float x0 = h01(k, 1, seed + 3) * 64.0f, y0 = h01(k, 2, seed + 3) * 64.0f, a = h01(k, 3, seed + 3) * PI_F, len = 12.0f + h01(k, 4, seed + 3) * 9.0f;
        wrap_draw(0, x0, y0, x0 + cosf(a) * len, y0 + sinf(a) * len, 1.3f + 0.3f * h01(k, 5, seed + 3), 0.75f);
    }
    for (int k = 0; k < (vseed ? 3 : 2); k++)
        wrap_draw(1, 12.0f + k * 22.0f + 6.0f * h01(k, 7, seed + 3), 14.0f + 36.0f * h01(k, 8, seed + 3), k == 2 ? 1.2f : 0.85f, 0.8f, 0, 0);
    for (int y = 0; y < TS; y++)
        for (int x = 0; x < TS; x++) {
            uint8_t b = cv_sprite[y * TS + x], g = crawl_tex[t][x * TS + y];
            if (b == C_CLEAR) continue;
            int deep = fbm(x / 8.0f, y / 8.0f, 8, seed + 4) > 0.58f;   /* some of it further in, under more glass */
            uint8_t c = darker(b, deep ? 5 : 2);
            if (g >= R_GHOST && g < R_GHOST + 16) c = lighter(c, 3);   /* the glass's glint over it */
            else if (g >= R_STONE && g < R_STONE + 16 && g - R_STONE >= 7) c = lighter(c, 1);
            tput(t, x, y, c);
        }
    for (int i = 0; i < TS * TS; i++) g_mid[m][i] = 0;
}

/* the Deep's rot: soot and grime from the shared field; glitter early; cracks that heat through the middle of
 * the run and glow, run molten on one look of each late on, and go out dark at the end */
static void decay_deep(int t, float dk, uint32_t field, uint32_t own, int marked) {
    float glit = deep_glitter(dk), heat = deep_heat(dk), dark = deep_dark(dk);
    uint8_t *p = crawl_tex[t];
    for (int y = 0; y < TS; y++)
        for (int x = 0; x < TS; x++) {
            uint8_t c = p[x * TS + y];
            if (c == C_CLEAR || IS_FULLBRIGHT(c)) continue;
            float n = fbm(x / 8.0f, y / 8.0f, 8, field + 5);
            if (n < 0.12f + 0.2f * dk + 0.12f * dark) c = darker(c, 1 + (n < 0.1f + 0.12f * dk));   /* soot, and the dark coming */
            if (glit > 0.0f && h01(x, y, own + 3) > 1.0f - 0.002f * glit) c = sh(R_GHOST, 0.6f);   /* mica and glass dust glittering */
            p[x * TS + y] = c;
        }
    int cracks = dk > 0.25f ? 1 + (int)(dk * 4.0f) : 0;
    for (int k = 0; k < cracks; k++) {
        float x = h01(k, 3, own) * 64.0f, y = h01(k, 4, own) * 64.0f, a = h01(k, 5, own) * TAU_F;
        int len = 10 + (int)(h01(k, 6, own) * 24.0f);
        for (int i = 0; i < len; i++) {
            a += (h01(k, i + 10, own) - 0.5f) * 1.1f; x += cosf(a); y += sinf(a);
            uint8_t *q = &p[((int)x & 63) * TS + ((int)y & 63)];
            if (*q == C_CLEAR) continue;
            if (heat > 0.15f) {
                *q = (uint8_t)(R_FLAME + clampi((int)(2.0f + 10.0f * heat), 2, 12));
                uint8_t *q2 = &p[(((int)x + 1) & 63) * TS + ((int)y & 63)];
                if (*q2 != C_CLEAR && !IS_FULLBRIGHT(*q2)) *q2 = sh(R_BLOOD, 0.2f + 0.2f * heat);
            } else *q = dark > 0.0f ? sh(R_BLOOD, 0.1f) : sh(R_STONE, 0.02f);   /* not yet hot; or cooled again, red-black */
        }
    }
    if (marked && heat > 0.4f) {                                      /* the rock running molten down from a crack */
        int x0 = 10 + (int)(h01(7, 7, own) * 44.0f), y0 = 8 + (int)(h01(8, 8, own) * 18.0f), len = (int)(14.0f + 30.0f * heat);
        for (int i = 0; i < len && y0 + i < 64; i++) {
            int x = x0 + (int)(1.5f * sinf(i * 0.3f)), w = i < len - 4 ? 1 : 0;
            for (int o = -w; o <= w; o++) {
                uint8_t *q = &p[((x + o) & 63) * TS + (y0 + i)];
                if (*q != C_CLEAR) *q = (uint8_t)(R_FLAME + clampi(13 - i / 6 - abs(o) * 3, 3, 14));
            }
        }
    }
}

/* ---- what stands on the Deep's walls ---- */

static void gen_feat_geode(int t, uint32_t seed, float glow) {    /* a geode broken open in the rock: banded rind, crystals grown inward from it, alight */
    cv_begin(cv_sprite, TS, TS);
    for (int y = 4; y < 60; y++)
        for (int x = 4; x < 60; x++) {
            float dx = (x + 0.5f - 32.0f) / 17.0f, dy = (y + 0.5f - 31.0f) / 15.0f, a = atan2f(dy, dx);
            float d = sqrtf(dx * dx + dy * dy) + 0.22f * (fbm(a * 1.6f + 3.0f, 0.5f, 8, seed) - 0.5f) + 0.06f * (fbm(x / 4.0f, y / 4.0f, 16, seed + 1) - 0.5f);
            if (d > 1.28f) continue;
            if (d > 1.0f) {                                            /* the rind, banded like agate */
                int band = (int)((d - 1.0f) * 26.0f);
                static const int RB[4] = { R_STONE, R_GHOST, R_STONE, R_PURPLE };
                float v = (band & 1 ? 0.48f : 0.28f) + 0.12f * lit(-dx, -dy, 0.5f) - (d > 1.22f ? 0.16f : 0.0f);
                cv_px(x, y, sh(RB[band & 3], v));
                continue;
            }
            uint8_t c = sh(R_PURPLE, 0.04f);                          /* the hollow's dark */
            for (int ring = 0; ring < 2; ring++) {                     /* two courses of crystals, each pointing in from the rind */
                int nsec = ring ? 19 : 29;
                float sw = TAU_F / nsec, aa = a + PI_F + h01(ring, 1, seed) * sw + 0.4f * (fbm(d * 3.0f, 0.5f, 4, seed + 2 + ring) - 0.5f);
                int sec = (int)floorf(aa / sw);
                float off = aa / sw - sec - 0.5f;                       /* -0.5 .. 0.5 across the crystal */
                float len = (ring ? 0.88f : 0.5f) * (0.7f + 0.6f * h01(sec, 3 + ring, seed));
                float in = 1.0f - d;                                    /* how far in from the rind */
                if (in > len * (1.0f - 2.0f * fabsf(off))) continue;    /* past its point */
                float side = (off < 0.0f ? 1.0f : -1.0f) * cosf(sec * sw - 3.93f);
                float f = 0.45f + 0.25f * side + 0.3f * (in / len);    /* the point brightest */
                c = sh(R_PURPLE, 0.15f + 0.6f * f);                    /* amethyst, the light in its points */
                if (glow > 0.05f && ring && in > len * 0.55f) c = (uint8_t)(R_GLOW + clampi((int)(2.0f + 11.0f * glow * f), 1, 14));
                if (fabsf(off) > 0.44f) c = sh(R_PURPLE, 0.06f);
                break;
            }
            if (d > 0.96f) c = sh(R_STONE, 0.05f);                    /* the lip's shadow */
            cv_px(x, y, c);
        }
    cv_to_tex(t);
}
static void gen_feat_seam(int t, uint32_t seed, float glow) {     /* a seam across the rock, a few gems in it, some with a fire in them */
    cv_begin(cv_sprite, TS, TS);
    for (int x = 2; x < 62; x++)
        for (int y = 0; y < TS; y++) {
            float cy = 30.0f - x * 0.25f + 5.0f * sinf(x * 0.11f + h01(1, 1, seed) * 6.0f), w = 2.0f + 1.6f * fbm(x / 8.0f, 0.5f, 8, seed);
            float d = fabsf(y + 0.5f - cy);
            if (d > w) continue;
            uint8_t c = h01(x, y, seed + 2) > 0.75f ? sh(R_BONE, 0.38f + 0.15f * h01(x, y, seed + 3)) : sh(R_STONE, 0.1f + 0.05f * h01(x, y, seed + 4));
            if (d > w - 0.8f) c = sh(R_STONE, 0.03f);
            cv_px(x, y, c);
        }
    for (int k = 0; k < 7; k++) {                                     /* the gems in it, where they fell */
        float x = 6.0f + 52.0f * h01(k, 5, seed), cy = 30.0f - x * 0.25f + 5.0f * sinf(x * 0.11f + h01(1, 1, seed) * 6.0f) + 1.5f * (h01(k, 6, seed) - 0.5f);
        float q = h01(k, 7, seed), r = 1.6f + 2.4f * h01(k, 8, seed) * h01(k, 9, seed);
        int ramp = q < 0.4f ? R_BLOOD : q < 0.5f ? R_GOLD : -1, fire = ramp < 0 ? (q < 0.8f ? R_GLOW : R_FLAME) : 0;
        for (int y = (int)(cy - r) - 1; y <= (int)(cy + r) + 1; y++)
            for (int xx = (int)(x - r) - 1; xx <= (int)(x + r) + 1; xx++) {
                float dx = xx + 0.5f - x, dy = y + 0.5f - cy;
                if (fabsf(dx) + fabsf(dy) * 1.2f > r) continue;
                float f = (dx + dy < 0 ? 0.8f : 0.45f) - 0.15f * (fabsf(dx) + fabsf(dy)) / r;
                uint8_t c = fire ? (glow > 0.05f ? (uint8_t)(fire + clampi((int)(3.0f + 9.0f * glow * f), 1, 13)) : sh(R_PURPLE, f)) : sh(ramp, f);
                cv_px(xx, y, c);
            }
    }
    cv_to_tex(t);
}
static void gen_feat_gold(int t, uint32_t seed) {                 /* threads of gold through white quartz, forking */
    cv_begin(cv_sprite, TS, TS);
    static float bx[8], by[8], ba[8]; static int bl[8];
    int nb = 1;
    bx[0] = 4.0f; by[0] = 18.0f + 24.0f * h01(0, 2, seed); ba[0] = 0.15f; bl[0] = 56;
    for (int b = 0; b < nb; b++) {
        float x = bx[b], y = by[b], a = ba[b];
        for (int i = 0; i < bl[b]; i++) {
            a += (h01(b, i + 9, seed) - 0.5f) * 0.5f + (b ? 0.0f : -0.08f * (a - 0.15f));
            x += cosf(a); y += sinf(a);
            if (nb < 8 && b < 3 && i > 4 && h01(b, i + 60, seed) < 0.07f) {   /* a fork */
                bx[nb] = x; by[nb] = y; ba[nb] = a + (h01(b, i + 70, seed) < 0.5f ? 0.9f : -0.9f); bl[nb] = 8 + (int)(h01(nb, 4, seed) * 12.0f); nb++;
            }
            for (int oy = -1; oy <= 1; oy++)
                for (int ox = -1; ox <= 1; ox++)
                    if (cvp[(((int)y + oy) & 63) * TS + (((int)x + ox) & 63)] == C_CLEAR && h01((int)x + ox, (int)y + oy, seed + 1) < 0.8f)
                        cv_px(((int)x + ox) & 63, ((int)y + oy) & 63, sh(R_BONE, 0.42f + 0.16f * h01((int)x + ox, (int)y + oy, seed + 2)));
            cv_px((int)x & 63, (int)y & 63, sh(R_GOLD, 0.6f + 0.35f * h01(i, b, seed + 3)));
            if (!b && h01(i, 6, seed) < 0.05f) cv_ellipse(x, y, 1.8f, 1.5f, R_GOLD, 0.35f, 1.0f);   /* a nugget in it */
        }
    }
    cv_to_tex(t);
}
static void gen_feat_fused(int t, uint32_t seed) {                /* one of the dead fused into the rock, an arm up as if it clawed at it */
    cv_begin(cv_sprite, TS, TS);
    cv_bone(31, 20, 32, 44, 1.1f, 0.35f, 0.75f);                      /* the spine */
    for (int r = 0; r < 5; r++)
        for (int sd = -1; sd <= 1; sd += 2)
            for (int i = 1; i <= 8 - r / 2; i++) {
                float x = 31.5f + sd * i * 1.1f, y = 23.0f + r * 3.2f + 0.08f * i * i;
                cv_px((int)x, (int)y, sh(R_BONE, 0.7f - 0.03f * i)); cv_px((int)x, (int)y + 1, sh(R_BONE, 0.35f));
            }
    cv_bone(24, 21, 17, 36, 1.2f, 0.35f, 0.8f); cv_bone(17, 36, 21, 50, 1.0f, 0.35f, 0.75f);   /* an arm hanging */
    cv_bone(39, 21, 46, 10, 1.2f, 0.35f, 0.8f); cv_bone(46, 10, 49, 1, 1.0f, 0.35f, 0.75f);    /* the other, raised */
    for (int f = 0; f < 3; f++) cv_line(49.0f + (f - 1) * 1.5f, 2.0f, 49.0f + (f - 1) * 2.5f, -2.0f, sh(R_BONE, 0.6f));
    cv_ellipse(32, 47, 6.0f, 3.0f, R_BONE, 0.35f, 0.75f);             /* the pelvis */
    cv_bone(28, 49, 25, 63, 1.3f, 0.35f, 0.8f); cv_bone(36, 49, 40, 63, 1.3f, 0.35f, 0.8f);
    cv_skull(31.0f, 12.0f, 1.0f, 0.35f, 0.85f);
    static uint8_t keep[TS * TS];
    memcpy(keep, cv_sprite, sizeof keep);
    for (int y = 0; y < TS; y++)                                       /* the rock melted about it, and the glaze over it */
        for (int x = 0; x < TS; x++) {
            uint8_t b = keep[y * TS + x];
            if (b != C_CLEAR) { cv_px(x, y, fabsf((x - y) * 0.7f - 4.0f) < 2.0f ? lighter(b, 2) : darker(b, 2)); continue; }
            int near = 0;
            for (int k = 0; k < 8 && !near; k++) { int xx = x + (int)(3.0f * cosf(k * 0.785f)), yy = y + (int)(3.0f * sinf(k * 0.785f)); near = xx >= 0 && xx < 64 && yy >= 0 && yy < 64 && keep[yy * TS + xx] != C_CLEAR; }
            if (near) cv_px(x, y, sh(R_STONE, 0.04f + 0.06f * fbm(x / 4.0f, y / 2.0f, 16, seed)));
        }
    cv_to_tex(t);
}
static void gen_feat_frieze(int t, uint32_t seed) {               /* the throne's frieze: a crowned dead king carved in the glass, inlaid with gold */
    cv_begin(cv_sprite, TS, TS);
    for (int y = 6; y <= 58; y++)                                     /* the gold border */
        for (int x = 8; x <= 56; x++) {
            int edge = x == 8 || x == 56 || y == 6 || y == 58, in = x == 10 || x == 54 || y == 8 || y == 56;
            if (edge) cv_px(x, y, sh(R_GOLD, 0.5f + (x == 8 || y == 6 ? 0.3f : 0.0f)));
            else if (in && x >= 10 && x <= 54 && y >= 8 && y <= 56) cv_px(x, y, sh(R_GOLD, 0.35f));
        }
    #define CUT(x0, y0, x1, y1) do { cv_line(x0, y0, x1, y1, sh(R_GOLD, 0.45f)); cv_line((x0) + 1, (y0) + 1, (x1) + 1, (y1) + 1, sh(R_STONE, 0.02f)); } while (0)
    CUT(20, 18, 20, 50); CUT(44, 18, 44, 50); CUT(20, 18, 44, 18); CUT(16, 50, 48, 50);   /* the throne */
    CUT(24, 38, 40, 38); CUT(24, 38, 22, 50); CUT(40, 38, 42, 50);                        /* the seat, the knees */
    CUT(27, 30, 24, 38); CUT(37, 30, 40, 38); CUT(27, 30, 37, 30);                        /* the robe */
    cv_skull(32.0f, 24.5f, 0.62f, 0.3f, 0.7f);
    for (int i = 0; i < 5; i++) cv_line(28.0f + i * 2.0f, 20.0f, 28.0f + i * 2.0f, 17.0f - (i & 1) * 2.0f, sh(R_GOLD, 0.75f));   /* its crown */
    cv_line(28, 20, 36, 20, sh(R_GOLD, 0.6f));
    for (int i = 0; i < 9; i++) cv_px(14 + i * 4 + (i > 4 ? 2 : 0), 54, sh(R_GOLD, 0.6f));   /* the marks below it */
    #undef CUT
    (void)seed;
    cv_to_tex(t);
}
static void gen_feat_fissure(int t, int frame, uint32_t seed, float glow) {   /* a fissure in the wall, the molten rock in it, its light (three frames) */
    cv_begin(cv_sprite, TS, TS);
    float ph = h01(1, 1, seed) * 6.0f;
    for (int y = 4; y < 60; y++) {
        float cx = 32.0f + 5.0f * sinf(y * 0.13f + ph) + 3.0f * (fbm(y / 8.0f, 0.5f, 8, seed) - 0.5f) * 2.0f;
        float w = (1.2f + 2.6f * sinf(PI_F * (y - 4) / 56.0f)) * (0.75f + 0.5f * fbm(y / 4.0f, 1.5f, 16, seed + 1));
        for (int x = (int)(cx - w - 6.0f); x <= (int)(cx + w + 6.0f); x++) {
            float d = fabsf(x + 0.5f - cx);
            int pulse = sinf(y * 0.37f + frame * 2.1f + ph) > -0.2f;
            if (d < w * 0.45f) cv_px(x, y, (uint8_t)(R_FLAME + clampi((int)((pulse ? 15.0f : 13.0f) * (0.45f + 0.55f * glow)), 4, 15)));
            else if (d < w) cv_px(x, y, (uint8_t)(R_FLAME + clampi((int)((9.0f + 3.0f * pulse) * (0.45f + 0.55f * glow)), 3, 12)));
            else if (d < w + 1.2f) cv_px(x, y, (uint8_t)(R_FLAME + clampi((int)(4.0f * glow), 1, 4)));
            else if (h01(x, y, seed + 2) < 0.85f - (d - w) * 0.17f) cv_px(x, y, sh(R_BLOOD, 0.18f + 0.25f * (1.0f - (d - w) / 6.0f)));   /* the rock reddened by it */
        }
    }
    int drip = 2 + frame * 2;                                         /* what runs out of its foot */
    for (int y = 58; y < 58 + drip && y < 64; y++) cv_px(32 + (int)(5.0f * sinf(58 * 0.13f + ph)), y, (uint8_t)(R_FLAME + 12 - (y - 58)));
    cv_to_tex(t);
}

/* ---- the Deep's floors, ceilings, doors, keys and treasure ---- */

static void gen_floor_glass(int t, uint32_t seed, int broken, float heat) {   /* black glass underfoot, a crack or two; broken through later, the fire under it */
    float px[12], py[12];
    for (int i = 0; i < 12; i++) { px[i] = h01(i, 1, seed) * 64.0f; py[i] = h01(i, 2, seed) * 64.0f; }
    for (int y = 0; y < TS; y++)
        for (int x = 0; x < TS; x++) {
            float v = 0.13f + 0.04f * (fbm(x / 8.0f, y / 8.0f, 8, seed + 3) - 0.5f) + 0.15f * expf(-powf((fbm(x / 16.0f, y / 16.0f, 4, seed + 4) - 0.5f) * 7.0f, 2.0f));
            uint8_t c = sh(R_STONE, v);
            if (broken) {                                              /* shattered: each piece a little askew, the gaps between */
                float d1, d2;
                int k = voro(x, y, px, py, 12, &d1, &d2);
                c = sh(R_STONE, v + 0.07f * (h01(k, 5, seed) - 0.5f));
                if (d2 - d1 < 1.2f) c = heat > 0.15f && h01(k, 6, seed) < 0.5f ? (uint8_t)(R_FLAME + clampi((int)(2.0f + 6.0f * heat), 2, 8)) : sh(R_STONE, 0.02f);
            }
            if (h01(x, y, seed + 7) > 0.998f) c = sh(R_GHOST, 0.4f);
            tput(t, x, y, c);
        }
    for (int k = 0; k < (broken ? 1 : 3); k++) {                      /* a crack or two across it, a lit edge along each */
        float x = h01(k, 11, seed) * 64.0f, y = h01(k, 12, seed) * 64.0f, a = h01(k, 13, seed) * TAU_F;
        int len = 18 + (int)(h01(k, 14, seed) * 26.0f);
        for (int i = 0; i < len; i++) {
            a += (h01(k, i + 20, seed) - 0.5f) * 0.7f; x += cosf(a); y += sinf(a);
            tput(t, (int)x, (int)y, sh(R_STONE, 0.02f));
            tput(t, (int)x + 1, (int)y, sh(R_STONE, 0.34f));
            if (i % 9 == 4) {                                         /* a spur off it */
                float sa = a + 1.4f, sx = x, sy = y;
                for (int j = 0; j < 5; j++) { sx += cosf(sa); sy += sinf(sa); tput(t, (int)sx, (int)sy, sh(R_STONE, 0.04f)); }
            }
        }
    }
}
static void gen_floor_throne(int t, uint32_t seed) {               /* great flags of black glass, gold laid in their joints */
    for (int y = 0; y < TS; y++)
        for (int x = 0; x < TS; x++) {
            int lx = x & 31, ly = y & 31, f = (x >> 5) + (y >> 5) * 2;
            float sd = (float)(lx - ly) + 10.0f * (h01(f, 1, seed) - 0.5f);
            float v = 0.12f + 0.04f * h01(f, 2, seed) + 0.2f * expf(-sd * sd / 18.0f) + 0.03f * (fbm(x / 4.0f, y / 4.0f, 16, seed) - 0.5f);
            uint8_t c = sh(R_STONE, v);
            if (lx == 0 || ly == 0) c = sh(R_GOLD, (lx == 0 && ly == 0) ? 0.85f : 0.32f + 0.12f * h01(x, y, seed + 3));
            else if (lx == 1 || ly == 1 || lx == 31 || ly == 31) c = sh(R_STONE, 0.04f);
            if (abs(lx - 16) + abs(ly - 16) == 5) c = sh(R_GOLD, 0.28f);  /* a lozenge inlaid in each */
            tput(t, x, y, c);
        }
}
static void gen_floor_ash(int t, uint32_t seed) {                  /* ash, drifted in ripples, cinders in it */
    for (int y = 0; y < TS; y++)
        for (int x = 0; x < TS; x++) {
            float v = 0.5f + 0.1f * (fbm(x / 8.0f, y / 8.0f, 8, seed) - 0.5f) + 0.05f * sinf(x * 0.3f + y * 0.14f + 7.0f * fbm(x / 16.0f, y / 16.0f, 4, seed + 1)) + 0.04f * (fbm(x / 2.0f, y / 2.0f, 32, seed + 2) - 0.5f);
            uint8_t c = sh(R_IRON, v);
            float r = h01(x, y, seed + 3);
            if (r > 0.965f) c = sh(R_STONE, 0.08f);
            else if (r < 0.006f) c = sh(R_BONE, 0.55f);
            tput(t, x, y, c);
        }
}
static void gen_floor_cinder(int t, uint32_t seed, float heat, float dark) {   /* clinker and crust, the seams between alight while the rock burns */
    float px[16], py[16];
    for (int i = 0; i < 16; i++) { px[i] = h01(i, 1, seed) * 64.0f; py[i] = h01(i, 2, seed) * 64.0f; }
    for (int y = 0; y < TS; y++)
        for (int x = 0; x < TS; x++) {
            float d1, d2;
            int k = voro(x, y, px, py, 16, &d1, &d2);
            float gx = wrapd(x + 0.5f - px[k]) * (x + 0.5f > px[k] ? 1.0f : -1.0f), gy = wrapd(y + 0.5f - py[k]) * (y + 0.5f > py[k] ? 1.0f : -1.0f);
            float v = 0.2f + 0.08f * (h01(k, 3, seed) - 0.5f) - 0.008f * (gx + gy) + 0.06f * (fbm(x / 2.0f, y / 2.0f, 32, seed + 4) - 0.5f);
            uint8_t c = sh(R_STONE, v);
            if (d2 - d1 < 1.4f) {
                if (heat > 0.1f && fbm(x / 8.0f, y / 8.0f, 8, seed + 5) > 0.7f - 0.1f * heat) c = (uint8_t)(R_FLAME + clampi((int)(2.0f + 5.0f * heat), 2, 7));
                else c = dark > 0.0f ? sh(R_BLOOD, 0.08f) : sh(R_STONE, 0.02f);
            }
            float ember = h01(x, y, seed + 6);
            if (ember > 1.0f - 0.004f * (0.4f + heat) * (1.0f - dark)) c = (uint8_t)(R_FLAME + 12);
            tput(t, x, y, c);
        }
}
static void gen_ceil_crystal(int t, uint32_t seed, float glow) {   /* the roof hung with crystal, a light far up in some */
    gen_crystal(t, M4_CRYSTAL, seed, 0, glow * 0.3f);
    for (int i = 0; i < TS * TS; i++) { uint8_t c = crawl_tex[t][i]; crawl_tex[t][i] = IS_FULLBRIGHT(c) ? (uint8_t)(R_GLOW + 2) : darker(c, 6); }
}
static void gen_ceil_glint(int t, uint32_t seed) {                 /* rough rock with mica in it */
    gen_ceil_rock(t, seed);
    for (int y = 0; y < TS; y++)
        for (int x = 0; x < TS; x++) {
            if (h01(x, y, seed + 5) > 0.996f) tput(t, x, y, sh(R_IRON, 0.5f + 0.2f * h01(x, y, seed + 6)));
            else crawl_tex[t][x * TS + y] = darker(crawl_tex[t][x * TS + y], 1);
        }
}
static void gen_ceil_soot(int t, uint32_t seed, float heat) {      /* rock blackened by the fires under it, its cracks hot */
    for (int y = 0; y < TS; y++)
        for (int x = 0; x < TS; x++) {
            float v = 0.28f + 0.16f * (fbm(x / 8.0f, y / 8.0f, 8, seed) - 0.5f) + 0.06f * (fbm(x / 2.0f, y / 2.0f, 32, seed + 1) - 0.5f);
            if (fbm(x / 8.0f, y / 8.0f, 8, seed + 3) < 0.45f) v -= 0.1f;
            uint8_t c = sh(R_CEIL, v);
            if (fabsf(fbm(x / 8.0f, y / 8.0f, 8, seed + 2) - 0.5f) < 0.012f) c = heat > 0.3f && fbm(x / 16.0f, y / 16.0f, 4, seed + 4) > 0.66f ? (uint8_t)(R_FLAME + clampi((int)(1.0f + 3.0f * heat), 1, 4)) : sh(R_CEIL, 0.05f);
            tput(t, x, y, c);
        }
}
static void decay_flat_deep(int t, float dk, uint32_t seed) {      /* soot on the Deep's floors and roofs; no damp down here */
    if (dk <= 0.0f) return;
    for (int y = 0; y < TS; y++)
        for (int x = 0; x < TS; x++) {
            uint8_t c = crawl_tex[t][x * TS + y];
            if (IS_FULLBRIGHT(c)) continue;
            float n = fbm(x / 8.0f, y / 8.0f, 8, seed + 5);
            if (n < 0.16f + 0.24f * dk) crawl_tex[t][x * TS + y] = darker(c, 1 + (n < 0.14f + 0.1f * dk));
        }
}
static void gen_irondoor(int t, uint32_t seed, int metal, float glow) {   /* black iron in riveted plates, a grille with the fire behind it; a vault banded in its key's metal */
    for (int y = 0; y < TS; y++)
        for (int x = 0; x < TS; x++) {
            int sx = x & 31, sy = y % 21;
            float v = 0.2f + 0.05f * (fbm(x / 4.0f, y / 4.0f, 16, seed) - 0.5f) + 0.04f * h01(x >> 5, y / 21, seed);
            if (sx == 0 || sy == 0) v = 0.05f; else if (sx == 1 || sy == 1) v = 0.34f;
            if ((sx == 3 || sx == 29) && (y % 5) == 2) v = 0.6f;      /* the rivets */
            tput(t, x, y, sh(R_IRON, v));
        }
    for (int y = 8; y <= 22; y++)                                     /* the grille, the glow behind it */
        for (int x = 20; x <= 43; x++) {
            uint8_t c;
            if (x == 20 || x == 43 || y == 8 || y == 22) c = sh(R_IRON, 0.45f);
            else if ((x - 20) % 4 == 0 || y == 15) c = sh(R_IRON, (x - 20) % 4 == 0 ? 0.32f : 0.4f);
            else c = (uint8_t)(R_FLAME + clampi((int)((3.0f + 0.5f * (y - 8)) * (0.35f + 0.65f * glow)), 1, 11));
            tput(t, x, y, c);
        }
    if (metal >= 0) {
        float lo = metal == R_GOLD ? 0.1f : 0.25f;
        for (int b = 0; b < 2; b++)
            for (int y = 30 + b * 18; y < 34 + b * 18; y++)
                for (int x = 0; x < TS; x++) tput(t, x, y, sh(metal, lo + 0.5f + (y == 30 + b * 18 ? 0.3f : 0.0f) - (y == 33 + b * 18 ? 0.25f : 0.0f)));
        for (int y = 36; y <= 46; y++)
            for (int x = 26; x <= 37; x++) {
                float v = lo + 0.45f + (x == 26 || y == 36 ? 0.3f : 0.0f) - (x == 37 || y == 46 ? 0.2f : 0.0f);
                if (x >= 31 && x <= 32 && y >= 39 && y <= 43) { tput(t, x, y, 0); continue; }
                tput(t, x, y, sh(metal, v));
            }
    }
}
static void gen_glassjamb(int t, uint32_t seed) {                  /* the doorway's side: ash on the rock, a basalt column set against it (after the blends) */
    gen_ashrock(t, M4_ASH, seed + 5, 0);
    for (int y = 0; y < TS; y++)
        for (int x = 0; x < 14; x++) {
            float v = (x < 5 ? 0.42f : x < 10 ? 0.32f : 0.2f) + 0.05f * (fbm(x / 2.0f, y / 8.0f, 32, seed) - 0.5f);
            if (x == 13 || (y % 23) == 0) v = 0.06f;
            tput(t, x, y, sh(R_STONE, v));
        }
}
static void spr_key_glass(int id, float scale, int metal) {        /* a key of black glass, its bow and collar in metal */
    cv_begin(cv_sprite, TS, TS);
    #define KX(x) (32.0f + ((x) - 32.0f) * scale)
    float ar = fmaxf(fabsf(scale), 0.18f), lo = metal == R_GOLD ? 0.35f : 0.45f;
    cv_ellipse(KX(18), 38, 6.0f * ar, 6.0f, metal, lo, 1.0f);       /* the bow */
    cv_flat(KX(18), 38, 3.0f * ar, 3.0f, C_CLEAR);
    cv_ellipse(KX(25), 38, 1.8f * ar, 3.2f, metal, lo, 1.0f);       /* the collar */
    cv_capsule(KX(27), 38, KX(50), 38, 1.9f, R_GHOST, 0.25f, 0.8f); /* the glass shank */
    for (int y = 39; y <= 47; y++)
        for (int x = 43; x <= 50; x++) {
            if (x == 46 && y >= 43) continue;
            cv_px((int)KX(x + 0.5f), y, sh(R_GHOST, (x == 43 || y == 47) ? 0.75f : 0.4f - 0.02f * (y - 39)));
        }
    cv_px((int)KX(34.0f), 37, sh(R_GHOST, 1.0f)); cv_px((int)KX(35.0f), 37, sh(R_GHOST, 0.9f));   /* a glint along it */
    #undef KX
    cv_outline(sh(R_IRON, 0.04f));
    cv_store(id);
}
static void spr_molten(int id, int frame) {                        /* coin run molten in the heat, a few still whole at its edge */
    cv_begin(cv_sprite, TS, TS);
    cv_flat(32, 62, 16, 2.2f, R_WARM + 1);
    for (int y = 55; y <= 62; y++)
        for (int x = 17; x <= 47; x++) {
            float dx = (x + 0.5f - 32.0f) / 14.5f, dy = (y + 0.5f - 59.0f) / 3.6f;
            if (dx * dx + dy * dy > 1.0f + 0.2f * (fbm(x / 4.0f, 1.0f, 16, 731) - 0.5f)) continue;
            float v = 0.35f + 0.5f * lit(dx, dy, 0.7f) + 0.1f * sinf(x * 0.9f + frame * 2.0f);
            cv_px(x, y, sh(R_GOLD, v));
        }
    static const float C[3][3] = { { 21, 56, 0.4f }, { 41, 57, -0.3f }, { 35, 54, 0.1f } };
    for (int i = 0; i < 3; i++) cv_ellipse(C[i][0], C[i][1], 3.4f, 1.8f + C[i][2], R_GOLD, 0.4f, 1.0f);
    cv_outline(0);
    for (int i = 0; i < 6; i++) {                                     /* where it is still molten */
        int x = 25 + (int)(h01(i, frame, 733) * 7.0f), y = 59 + (int)(h01(i, frame + 3, 733) * 2.0f);
        cv_px(x, y, (uint8_t)(R_FLAME + 11 + (i & 3)));
    }
    cv_px(frame ? 39 : 23, 55, C_WHITE);
    cv_store(id);
}
static void spr_glowgems(void) {                                   /* a cluster of crystal with the light in it */
    cv_begin(cv_sprite, TS, TS);
    cv_flat(32, 62, 14, 2.0f, R_WARM + 1);
    for (int y = 54; y <= 62; y++)
        for (int x = 20; x <= 44; x++) {
            float dx = (x + 0.5f - 32.0f) / 12.0f, dy = (y + 0.5f - 59.0f) / 4.0f;
            if (dx * dx + dy * dy <= 1.0f) cv_px(x, y, sh(R_STONE, 0.12f + 0.3f * lit(dx, dy, 0.6f)));
        }
    static const float P[5][4] = { { 32, 58, 0.0f, 20 }, { 27, 59, -0.45f, 13 }, { 37, 59, 0.4f, 15 }, { 24, 60, -0.8f, 8 }, { 40, 60, 0.75f, 9 } };
    for (int i = 0; i < 5; i++) {
        float bx = P[i][0], by = P[i][1], a = P[i][2], len = P[i][3], w = 2.6f, dx = sinf(a), dy = -cosf(a);
        for (float s = 0; s < len; s += 0.5f) {
            float hw = s > len - w * 1.5f ? w * (len - s) / (w * 1.5f) : w;
            for (float o = -hw; o <= hw; o += 0.5f)
                cv_px((int)(bx + dx * s - dy * o), (int)(by + dy * s + dx * o), (uint8_t)(R_GLOW + clampi((int)(5.0f + 8.0f * (1.0f - fabsf(o) / (w + 0.01f)) * (0.4f + 0.6f * s / len) + (o < 0 ? 2 : 0)), 3, 15)));
        }
    }
    cv_store(SP_GEMS);
}
static void spr_idol(void) {                                       /* an idol of black glass, a seated king, gold for its eyes and crown */
    cv_begin(cv_sprite, TS, TS);
    cv_flat(32, 62, 12, 2.0f, R_WARM + 1);
    for (int y = 55; y <= 61; y++) for (int x = 22; x <= 42; x++) cv_px(x, y, sh(R_STONE, x == 22 || y == 55 ? 0.4f : 0.1f));   /* its block */
    float body[8] = { 25, 55, 39, 55, 37, 40, 27, 40 };
    cv_poly(body, 4, R_STONE, 0.22f, 0.08f, 40, 15);
    cv_ellipse(32, 35, 4.6f, 5.0f, R_STONE, 0.06f, 0.5f);             /* the head */
    cv_capsule(27, 42, 24, 50, 1.6f, R_STONE, 0.08f, 0.45f); cv_capsule(37, 42, 40, 50, 1.6f, R_STONE, 0.08f, 0.45f);   /* arms on the knees */
    cv_outline(sh(R_STONE, 0.02f));
    for (int y = 41; y <= 54; y++) cv_px(26 + (54 - y) / 7, y, sh(R_GHOST, 0.5f));   /* the glass's edge, lit */
    cv_px(30, 35, sh(R_GOLD, 0.95f)); cv_px(34, 35, sh(R_GOLD, 0.95f));
    for (int i = 0; i < 4; i++) cv_line(29.0f + i * 2.0f, 30.0f, 29.0f + i * 2.0f, 28.0f - (i & 1), sh(R_GOLD, 0.8f));
    cv_line(29, 30, 35, 30, sh(R_GOLD, 0.6f));
    cv_store(SP_CHALICE);
}

static void treasure_plain(void) {                                 /* the treasure as the Crypts have it */
    spr_coins(SP_COINS_A, 0); spr_coins(SP_COINS_B, 1);
    spr_chalice(); spr_gems(); spr_crown(); spr_chest();
}

/* The hall's sign, the Repository and the Portal, and the arenas' gates, fonts and drains, are drawn on
 * plain stone; on a drawn run they stand in its ashlar instead, wherever the plain stone showed */
static void rebase(int t, uint32_t stone_seed, int base) {
    static uint8_t keep[TS * TS];
    memcpy(keep, crawl_tex[T_STONE], sizeof keep);
    gen_stone(T_STONE, R_STONE, stone_seed);
    for (int i = 0; i < TS * TS; i++) if (crawl_tex[t][i] == crawl_tex[T_STONE][i]) crawl_tex[t][i] = crawl_tex[base][i];
    memcpy(crawl_tex[T_STONE], keep, sizeof keep);
}
/* a puzzle's sealed wing: a great iron-banded door, a round seal carved in the stone over it */
static void gen_wingdoor(int t, uint32_t seed) {
    gen_stone(t, R_STONE, seed);
    for (int y = 14; y < 64; y++)
        for (int x = 10; x < 54; x++) {
            float dx = x + 0.5f - 32.0f, top = 24.0f - sqrtf(fmaxf(0.0f, 22.0f * 22.0f - dx * dx)) * 0.45f;
            if (y < top) continue;
            int frame = y < top + 2.0f || x < 12 || x > 51;
            uint8_t c = frame ? sh(R_STONE, 0.7f) : sh(R_WOOD, 0.2f + 0.12f * fbm(x / 3.0f, y / 9.0f, 16, seed) + 0.08f * ((x - 12) % 7 == 0 ? -1.0f : 0.0f));
            if (!frame && ((y - 14) % 14 < 2)) c = sh(R_IRON, 0.35f + 0.2f * ((x & 3) == 0));   /* the bands */
            tput(t, x, y, c);
        }
    for (int a = 0; a < 40; a++) {                                    /* the seal: a ring with a cross of bars through it */
        float an = a * 0.157f;
        tput(t, 32 + (int)lroundf(7.0f * cosf(an)), 11 + (int)lroundf(5.5f * sinf(an)), sh(R_BLOOD, 0.55f));
    }
    for (int k = -4; k <= 4; k++) { tput(t, 32 + k, 11, sh(R_BLOOD, 0.4f)); tput(t, 32, 11 + k * 5 / 7, sh(R_BLOOD, 0.4f)); }
    for (int y = 36; y < 44; y++) { tput(t, 31, y, sh(R_IRON, 0.6f)); tput(t, 33, y, sh(R_IRON, 0.6f)); }   /* a lockless ring */
}
static void hz_walls(void) {                                       /* the runs' hazards (1.0.0b13) */
    gen_hz_slab(T_HZ_SLAB, 109, 0); gen_hz_slab(T_HZ_SLAB_OPEN, 109, 1); gen_hz_cold(T_HZ_COLD, 113);
    gen_hz_sluice(T_HZ_SLUICE, 127, 0); gen_hz_sluice(T_HZ_SLUICE_OPEN, 127, 1);
    gen_hz_saint(T_HZ_SAINT, 131, 0); gen_hz_saint(T_HZ_SAINT_LIT, 131, 1);
    gen_hz_seam(T_HZ_SEAM, 137, 0); gen_hz_seam(T_HZ_SEAM_LIT, 137, 1);
    gen_hz_fissure(T_HZ_FISSURE0, 139, 0); gen_hz_fissure(T_HZ_FISSURE1, 139, 1);
    gen_hz_mirror(T_HZ_MIRROR, 149, 0); gen_hz_mirror(T_HZ_MIRROR_SEEN, 149, 1);
    gen_wingdoor(T_WINGDOOR, 151);
}
static void specials_plain(void) {
    gen_sign(T_SIGN, 91);
    gen_repository(T_REPOSITORY);
    for (int f = 0; f < 4; f++) gen_portal(T_PORTAL0 + f, f, 107);
    gen_gate(T_GATE, 97);
    gen_font(T_FONT, 101, 1); gen_font(T_FONT_DRY, 101, 0);
    gen_grate(T_GRATE, 103);
    hz_walls();
}
static void specials_on(int base) {
    specials_plain();
    rebase(T_SIGN, 91, base); rebase(T_REPOSITORY, 233, base);
    for (int f = 0; f < 4; f++) rebase(T_PORTAL0 + f, 107, base);
    rebase(T_GATE, 97, base); rebase(T_FONT, 101, base); rebase(T_FONT_DRY, 101, base); rebase(T_GRATE, 103, base);
    static const uint32_t HZS[] = { 109, 109, 113, 127, 127, 131, 131, 137, 137, 139, 139, 149, 149, 151 };
    for (int i = 0; i < 14; i++) rebase(T_HZ_SLAB + i, HZS[i], base);
}

/* The run's own textures and keys, for the level's place in it: made from the level (so each level's
 * stones differ), with the rot of its step. Called after crawl_art_level, which sets the palette. */
static void crypts_run(float dk, uint32_t s, uint32_t field) {
    const RunLook *L = &crawl_looks[0];
    for (int v = 0; v < NVAR; v++) {
        uint32_t vs = v ? s + 1u : 0u;
        gen_ashlar(T_MATV(M1_ASHLAR, v), M1_ASHLAR, s, vs);
        gen_rubble_c(T_MATV(M1_RUBBLE, v), M1_RUBBLE, s + 10, vs);
        gen_plaster(T_MATV(M1_PLASTER, v), M1_PLASTER, s + 20, vs);
        gen_bedrock(T_MATV(M1_BEDROCK, v), M1_BEDROCK, s + 30, vs);
        gen_bonewall(T_MATV(M1_BONES, v), M1_BONES, s + 40, vs);
    }
    for (int v = 0; v < NVAR; v++) plaster_flake(T_MATV(M1_PLASTER, v), T_MATV(M1_RUBBLE, 0), dk, field + 30);
    for (int m = 0; m < NMAT; m++)
        for (int v = 0; v < NVAR; v++) decay_wall(T_MATV(m, v), m == M1_BONES ? dk * 0.6f : dk, field, s + 60u + (uint32_t)(m * NVAR + v) * 3u);
    for (int k = 0; k < NBLEND; k++) gen_blend(T_BLEND + k, L->blend[k][0], L->blend[k][1], L->blend[k][0] == M1_PLASTER, s + 70u + (uint32_t)k);
    gen_feat_niche(T_FEAT + 0, s + 80);
    gen_feat_banner(T_FEAT + 1, s + 81, dk);
    gen_feat_slab(T_FEAT + 2, s + 82);
    gen_feat_shrine(T_FEAT + 3, s + 83);
    gen_feat_tablet(T_FEAT + 4, s + 84);
    for (int f = 0; f < 3; f++) gen_feat_torch(T_FEAT + 5 + f, f, 11);
    for (int i = 0; i < 5; i++) decay_wall(T_FEAT + i, dk * 0.7f, field, s + 90u + (uint32_t)i * 3u);
    gen_floor_flags(T_FLOORS + 0, s + 100, 0);
    gen_floor_chapter(T_FLOORS + 1, s + 101);
    gen_floor_setts(T_FLOORS + 2, s + 102);
    gen_floor_earth(T_FLOORS + 3, s + 103);
    gen_floor_flags(T_FLOORS + 4, s + 100, 1);
    gen_ceil_rib(T_CEILS + 0, s + 110);
    gen_ceil_vault(T_CEILS + 1, s + 111);
    gen_ceil_rock(T_CEILS + 2, s + 112);
    for (int i = 0; i < NFLOORS; i++) decay_flat(T_FLOORS + i, dk, s + 130u + (uint32_t)i);
    for (int i = 0; i < NCEILS; i++) decay_flat(T_CEILS + i, dk * 0.7f, s + 140u + (uint32_t)i);
    gen_rdoor(T_RDOOR, s + 120, -1);
    gen_rjamb(T_RJAMB, s);
    gen_rdoor(T_RLOCKED, s + 121, R_GOLD);
    gen_rdoor(T_RLOCKED_S, s + 121, R_IRON);
    gen_rdoor(T_RLOCKED_C, s + 121, R_BLOOD);
    decay_flat(T_RDOOR, dk * 0.5f, s + 150);
    static const float KS[4] = { 1.0f, 0.5f, 0.12f, -0.5f };
    for (int f = 0; f < 4; f++) { spr_key_trefoil(SP_KEY0 + f, KS[f], R_GOLD); spr_key_trefoil(SP_KEYS0 + f, KS[f], R_IRON); spr_key_trefoil(SP_KEYC0 + f, KS[f], R_BLOOD); }
    specials_on(T_MATV(M1_ASHLAR, 0));
}
static void hollows_run(float dk, uint32_t s, uint32_t field) {
    const RunLook *L = &crawl_looks[2];
    for (int v = 0; v < NVAR; v++) {
        uint32_t vs = v ? s + 1u : 0u;
        gen_granite(T_MATV(M3_GRANITE, v), M3_GRANITE, s, vs);
        gen_mineface(T_MATV(M3_FACE, v), M3_FACE, s + 10, vs);
        gen_timber(T_MATV(M3_TIMBER, v), M3_TIMBER, s + 20, vs, dk);
        gen_basalt(T_MATV(M3_BASALT, v), M3_BASALT, s + 30, vs, R_IRON);
        gen_backfill(T_MATV(M3_FILL, v), M3_FILL, s + 40, vs);
    }
    for (int m = 0; m < NMAT; m++)
        for (int v = 0; v < NVAR; v++) decay_hollows(T_MATV(m, v), dk, field, s + 60u + (uint32_t)(m * NVAR + v) * 3u, v);
    for (int k = 0; k < NBLEND; k++) gen_blend(T_BLEND + k, L->blend[k][0], L->blend[k][1], 0, s + 70u + (uint32_t)k);
    gen_feat_ore(T_FEAT + F3_ORE, s + 80);
    gen_feat_fossil(T_FEAT + F3_FOSSIL, s + 81);
    gen_feat_frame(T_FEAT + F3_FRAME, s + 82);
    gen_feat_chute(T_FEAT + F3_CHUTE, s + 83);
    gen_feat_marks(T_FEAT + F3_MARKS, s + 84);
    for (int f = 0; f < 3; f++) gen_feat_lamp(T_FEAT + F3_LAMP + f, f, 11);
    for (int i = 0; i < 5; i++) decay_wall(T_FEAT + i, dk * 0.5f, field, s + 90u + (uint32_t)i * 3u);
    gen_floor_gravel(T_FLOORS + FL3_GRAVEL, s + 100, 0);
    gen_floor_rails(T_FLOORS + FL3_RAILS, s + 101);
    gen_floor_trodden(T_FLOORS + FL3_EARTH, s + 102);
    gen_floor_earth(T_FLOORS + FL3_CHARNEL, s + 103);
    gen_floor_gravel(T_FLOORS + FL3_SCREE, s + 100, 1);
    gen_ceil_lagging(T_CEILS + CE3_LAGGING, s + 110);
    gen_ceil_rock(T_CEILS + CE3_ROCK, s + 111);
    gen_ceil_roots(T_CEILS + CE3_ROOTS, s + 112);
    for (int i = 0; i < NFLOORS; i++) decay_flat(T_FLOORS + i, dk * 0.8f, s + 130u + (uint32_t)i);
    for (int i = 0; i < NCEILS; i++) decay_flat(T_CEILS + i, dk * 0.6f, s + 140u + (uint32_t)i);
    gen_minedoor(T_RDOOR, s + 120, -1);
    gen_timberjamb(T_RJAMB, s);
    gen_minedoor(T_RLOCKED, s + 121, R_GOLD);
    gen_minedoor(T_RLOCKED_S, s + 121, R_IRON);
    gen_minedoor(T_RLOCKED_C, s + 121, R_BLOOD);
    decay_flat(T_RDOOR, dk * 0.5f, s + 150);
    static const float KS[4] = { 1.0f, 0.5f, 0.12f, -0.5f };
    for (int f = 0; f < 4; f++) { spr_key_tag(SP_KEY0 + f, KS[f], R_GOLD); spr_key_tag(SP_KEYS0 + f, KS[f], R_IRON); spr_key_tag(SP_KEYC0 + f, KS[f], R_BLOOD); }
    spr_nuggets(SP_COINS_A, 0); spr_nuggets(SP_COINS_B, 1);
    spr_rawgems(); spr_strongbox();
    specials_on(T_MATV(M3_FACE, 0));
}
static void deep_run(float dk, uint32_t s, uint32_t field) {
    const RunLook *L = &crawl_looks[3];
    float heat = deep_heat(dk), dark = deep_dark(dk), glow = 1.0f - 0.8f * dark;
    for (int v = 0; v < NVAR; v++) {
        uint32_t vs = v ? s + 1u : 0u;
        gen_obsidian(T_MATV(M4_OBSIDIAN, v), M4_OBSIDIAN, s, vs);
        gen_basalt(T_MATV(M4_BASALT, v), M4_BASALT, s + 10, vs, R_STONE);
        gen_ashrock(T_MATV(M4_ASH, v), M4_ASH, s + 20, vs);
        gen_crystal(T_MATV(M4_CRYSTAL, v), M4_CRYSTAL, s + 30, vs, glow);
        gen_boneglass(T_MATV(M4_BONES, v), M4_BONES, s + 40, vs);
    }
    for (int m = 0; m < NMAT; m++)
        for (int v = 0; v < NVAR; v++) decay_deep(T_MATV(m, v), dk, field, s + 60u + (uint32_t)(m * NVAR + v) * 3u, v);
    for (int k = 0; k < NBLEND; k++) gen_blend(T_BLEND + k, L->blend[k][0], L->blend[k][1], 0, s + 70u + (uint32_t)k);
    gen_feat_geode(T_FEAT + F4_GEODE, s + 80, glow);
    gen_feat_seam(T_FEAT + F4_SEAM, s + 81, glow);
    gen_feat_gold(T_FEAT + F4_GOLD, s + 82);
    gen_feat_fused(T_FEAT + F4_FUSED, s + 83);
    gen_feat_frieze(T_FEAT + F4_FRIEZE, s + 84);
    for (int f = 0; f < 3; f++) gen_feat_fissure(T_FEAT + F4_FISSURE + f, f, s + 85, glow);
    for (int i = 0; i < 5; i++) decay_deep(T_FEAT + i, dk * 0.5f, field, s + 90u + (uint32_t)i * 3u, 0);
    gen_floor_glass(T_FLOORS + FL4_GLASS, s + 100, 0, heat);
    gen_floor_throne(T_FLOORS + FL4_THRONE, s + 101);
    gen_floor_ash(T_FLOORS + FL4_ASH, s + 102);
    gen_floor_cinder(T_FLOORS + FL4_CINDER, s + 103, heat, dark);
    gen_floor_glass(T_FLOORS + FL4_SHARDS, s + 100, 1, heat);
    gen_ceil_crystal(T_CEILS + CE4_CRYSTAL, s + 110, glow);           /* after the blends: it borrows the crust's blocks */
    gen_ceil_glint(T_CEILS + CE4_ROCK, s + 111);
    gen_ceil_soot(T_CEILS + CE4_SOOT, s + 112, heat);
    for (int i = 0; i < NFLOORS; i++) decay_flat_deep(T_FLOORS + i, dk * 0.8f, s + 130u + (uint32_t)i);
    for (int i = 0; i < NCEILS; i++) decay_flat_deep(T_CEILS + i, dk * 0.6f, s + 140u + (uint32_t)i);
    gen_irondoor(T_RDOOR, s + 120, -1, glow);
    gen_glassjamb(T_RJAMB, s);
    gen_irondoor(T_RLOCKED, s + 121, R_GOLD, glow);
    gen_irondoor(T_RLOCKED_S, s + 121, R_IRON, glow);
    gen_irondoor(T_RLOCKED_C, s + 121, R_BLOOD, glow);
    decay_flat_deep(T_RDOOR, dk * 0.5f, s + 150);
    static const float KS[4] = { 1.0f, 0.5f, 0.12f, -0.5f };
    for (int f = 0; f < 4; f++) { spr_key_glass(SP_KEY0 + f, KS[f], R_GOLD); spr_key_glass(SP_KEYS0 + f, KS[f], R_IRON); spr_key_glass(SP_KEYC0 + f, KS[f], R_BLOOD); }
    spr_molten(SP_COINS_A, 0); spr_molten(SP_COINS_B, 1);
    spr_glowgems(); spr_idol();
    specials_on(T_MATV(M4_ASH, 0));
}
static void catacombs_run(float dk, uint32_t s, uint32_t field) {
    const RunLook *L = &crawl_looks[1];
    for (int v = 0; v < NVAR; v++) {
        uint32_t vs = v ? s + 1u : 0u;
        gen_slate(T_MATV(M2_SLATE, v), M2_SLATE, s, vs);
        gen_lining(T_MATV(M2_LINING, v), M2_LINING, s + 10, vs);
        gen_silt(T_MATV(M2_SILT, v), M2_SILT, s + 20, vs);
        gen_flow(T_MATV(M2_FLOW, v), M2_FLOW, s + 30, vs);
        gen_bonesilt(T_MATV(M2_BONES, v), M2_BONES, s + 40, vs);
    }
    for (int m = 0; m < NMAT; m++)
        for (int v = 0; v < NVAR; v++) decay_catacomb(T_MATV(m, v), dk, field, s + 60u + (uint32_t)(m * NVAR + v) * 3u);
    for (int k = 0; k < NBLEND; k++) gen_blend(T_BLEND + k, L->blend[k][0], L->blend[k][1], L->blend[k][0] == M2_FLOW, s + 70u + (uint32_t)k);
    gen_feat_fniche(T_FEAT + F2_NICHE, s + 80);
    gen_feat_saint(T_FEAT + F2_SAINT, s + 81);
    gen_feat_culvert(T_FEAT + F2_CULVERT, s + 82);
    gen_feat_hands(T_FEAT + F2_HANDS, s + 83);
    gen_feat_web(T_FEAT + F2_WEB, s + 84);
    for (int f = 0; f < 3; f++) gen_feat_lantern(T_FEAT + F2_LANTERN + f, f, 11);
    for (int i = 0; i < 4; i++) decay_catacomb(T_FEAT + i, dk * 0.7f, field, s + 90u + (uint32_t)i * 3u);
    gen_floor_wetflags(T_FLOORS + FL2_FLAGS, s + 100, 0);
    gen_floor_silt(T_FLOORS + FL2_SILT, s + 101);
    gen_floor_slateedge(T_FLOORS + FL2_SLATE, s + 102);
    gen_floor_water(T_FLOORS + FL2_WATER, s + 103);
    gen_floor_wetflags(T_FLOORS + FL2_PUDDLES, s + 100, 1);
    gen_ceil_slatevault(T_CEILS + CE2_VAULT, s + 110);
    gen_ceil_bedded(T_CEILS + CE2_ROCK, s + 111);
    gen_ceil_drip(T_CEILS + CE2_DRIP, s + 112);
    for (int i = 0; i < NFLOORS; i++) decay_flat2(T_FLOORS + i, dk, s + 130u + (uint32_t)i, i != FL2_WATER);
    for (int i = 0; i < NCEILS; i++) decay_flat2(T_CEILS + i, dk * 0.7f, s + 140u + (uint32_t)i, 0);
    gen_sluice(T_RDOOR, s + 120, -1);
    gen_slatejamb(T_RJAMB, s);
    gen_sluice(T_RLOCKED, s + 121, R_GOLD);
    gen_sluice(T_RLOCKED_S, s + 121, R_IRON);
    gen_sluice(T_RLOCKED_C, s + 121, R_BLOOD);
    static const float KS[4] = { 1.0f, 0.5f, 0.12f, -0.5f };
    for (int f = 0; f < 4; f++) { spr_key_ring(SP_KEY0 + f, KS[f], R_GOLD); spr_key_ring(SP_KEYS0 + f, KS[f], R_IRON); spr_key_ring(SP_KEYC0 + f, KS[f], R_BLOOD); }
    spr_coins_silt(SP_COINS_A, 0); spr_coins_silt(SP_COINS_B, 1);
    spr_reliquary(); spr_pearls();
    spr_crown(); spr_drown(SP_CROWN, s + 160); spr_chest(); spr_drown(SP_CHEST, s + 161);
    specials_on(T_MATV(M2_LINING, 0));
    for (int i = 0; i < TS * TS; i++) {                             /* the drains the bile comes from: culverts in the lining */
        uint8_t o = crawl_tex[T_FEAT + F2_CULVERT][i];
        crawl_tex[T_GRATE][i] = o != C_CLEAR ? o : crawl_tex[T_MATV(M2_LINING, 0)][i];
    }
}
/* ---- the trials (1.0.0b13, docs/PLAN-b13.md part D): each run's door, the lights by it, a bier, the hourglass ---- */
static const char *const TR_SKULL[9] = { "  #####  ", " ####### ", "#########", "##oo#oo##", "##oo#oo##", "#########", " ###o### ", "  #####  ", "  # # #  " };
/* a trial's door: the run's own, with its skull and crossed things on it (D1): the Crypts' iron plate and thighbones,
 * the Catacombs' verdigris plate and oars, the Hollows' red ochre and picks, the Deep's black glass skull, lit from
 * within, over gold bones */
static void gen_trdoor(int run) {
    memcpy(crawl_tex[T_TRDOOR], crawl_tex[T_RDOOR], sizeof crawl_tex[0]);
    int t = T_TRDOOR;
    if (run == 0 || run == 1)                                       /* the plate, riveted */
        for (int y = 12; y < 40; y++)
            for (int x = 18; x < 46; x++) {
                int rim = x == 18 || x == 45 || y == 12 || y == 39, rivet = (x == 20 || x == 43) && (y == 14 || y == 37);
                tput(t, x, y, run ? sh(R_MOSS, rim ? 0.3f : 0.45f + 0.12f * h01(x, y, 61)) : sh(R_IRON, rim ? 0.2f : 0.38f + 0.1f * h01(x, y, 61)));
                if (rivet) tput(t, x, y, run ? sh(R_MOSS, 0.75f) : sh(R_IRON, 0.75f));
            }
    for (int k = 0; k < 2; k++)                                     /* the crossed things, behind the skull */
        for (int i = 0; i <= 22; i++) {
            int x = k ? 43 - i : 21 + i, y = 15 + i * 21 / 22;
            uint8_t c = run == 0 ? sh(R_BONE, 0.65f) : run == 1 ? sh(R_WOOD, 0.4f) : run == 2 ? sh(R_BRICK, 0.55f) : sh(R_GOLD, 0.7f);
            tput(t, x, y, c); tput(t, x + (k ? -1 : 1), y, c);
            int end = i < 3 || i > 19;
            if (run == 0 || run == 3) { if (end) { tput(t, x, y - 1, c); tput(t, x, y + 1, c); tput(t, x + (k ? 1 : -1), y, c); } }   /* knobbed ends */
            else if (run == 1 && i > 16) { tput(t, x, y - 1, c); tput(t, x, y + 1, c); tput(t, x, y + 2, c); }   /* the oars' blades */
            else if (run == 2 && i < 4) for (int j = -3; j <= 3; j++) tput(t, x + j * (k ? 1 : -1) / 2 + j, y - abs(j) / 2, c);   /* the picks' heads */
        }
    for (int r = 0; r < 9; r++)
        for (int c = 0; c < 9; c++) {
            char ch = TR_SKULL[r][c];
            if (ch == ' ') continue;
            uint8_t v = ch == 'o' ? (run == 3 ? (uint8_t)(R_FLAME + 4) : sh(R_IRON, 0.02f))
                      : run == 0 ? sh(R_BONE, 0.7f - 0.04f * r) : run == 1 ? sh(R_BONE, 0.55f - 0.03f * r) : run == 2 ? sh(R_BRICK, 0.6f) : sh(R_IRON, 0.1f + 0.05f * (c < 3));
            tput(t, 28 + c, 20 + r, v);
        }
}
/* the lights by a trial's door, a run each: a red candle on an iron stand; a drowned lantern on a post, burning red;
 * a red lamp hung on a nail; a brazier of black glass, red embers in it. The flames are drawn full bright */
static void spr_trlight(int id, int run) {
    cv_begin(cv_sprite, TS, TS);
    if (run == 3) {
        cv_flat(32, 62, 9, 1.4f, R_WARM + 1);
        for (int y = 46; y < 62; y++) for (int x = 24; x < 41; x++) {
            float w = 8.0f - (float)(61 - y) * 0.15f;
            if (fabsf(x + 0.5f - 32.5f) > w) continue;
            cv_px(x, y, sh(R_IRON, 0.05f + 0.1f * (x < 27) + 0.05f * h01(x, y, 3)));
        }
        for (int x = 25; x < 40; x++) { cv_px(x, 45, (uint8_t)(R_FLAME + 3 + (int)(h01(x, 1, 7) * 4.0f))); if (h01(x, 2, 7) < 0.5f) cv_px(x, 44, (uint8_t)(R_FLAME + 2)); }
        cv_store(id);
        return;
    }
    for (int y = 18; y < 63; y++) { cv_px(31, y, sh(R_IRON, 0.25f)); cv_px(32, y, sh(R_IRON, 0.45f)); }   /* the stand, or post */
    cv_flat(32, 62, 6, 1.2f, sh(R_IRON, 0.2f));
    if (run == 0) {
        cv_flat(32, 19, 5, 1.5f, sh(R_IRON, 0.4f));
        for (int y = 10; y < 19; y++) for (int x = 30; x < 35; x++) cv_px(x, y, sh(R_BLOOD, 0.35f + 0.2f * (x == 30)));
        cv_flat(32, 7, 1.6f, 2.6f, R_FLAME + 3); cv_px(32, 8, R_FLAME + 8);
    } else {
        for (int x = 32; x < 40; x++) cv_px(x, 18, sh(R_IRON, 0.4f));   /* an arm, or a nail */
        int lx = 39;
        cv_line(lx, 18, lx, 22, sh(R_IRON, 0.3f));
        for (int y = 22; y < 33; y++) for (int x = lx - 4; x <= lx + 4; x++) {
            int frame = x == lx - 4 || x == lx + 4 || y == 22 || y == 32;
            cv_px(x, y, frame ? (run == 1 ? sh(R_MOSS, 0.45f) : sh(R_IRON, 0.3f)) : (uint8_t)(R_FLAME + 2 + ((x + y) & 1)));
        }
        if (run == 1) for (int x = lx - 5; x <= lx + 5; x++) cv_px(x, 21, sh(R_MOSS, 0.55f));   /* its cap, gone green */
    }
    cv_store(id);
}
static void spr_tr_prop(int id, int braced) {                      /* a pit prop: leaning loose, or upright and wedged under its cap */
    cv_begin(cv_sprite, TS, TS);
    float lean = braced ? 0.0f : 9.0f;
    cv_capsule(32 - lean * 0.5f, 62, 32 + lean * 0.5f, 10, 2.6f, R_WOOD, 0.2f, 0.55f);
    for (int x = 20; x < 45; x++) for (int y = 6; y < 10; y++) cv_px(x, y, sh(R_WOOD, 0.3f + 0.15f * (y == 6)));   /* its cap */
    if (braced) { float w[6] = { 27, 10, 37, 10, 34, 14 }; cv_poly(w, 3, R_WOOD, 0.6f, 0.4f, 10, 4); cv_line(30, 61, 35, 61, sh(R_STONE, 0.4f)); }
    else for (int k = 0; k < 6; k++) cv_px(24 + k * 3, 12 + (k & 1), sh(R_STONE, 0.4f));   /* grit sifting down */
    cv_outline(sh(R_WOOD, 0.02f));
    cv_store(id);
}
static void spr_tr_bier(void) {                                    /* a bier of grey stone, a pall over it */
    cv_begin(cv_sprite, TS, TS);
    for (int y = 46; y < 64; y++)
        for (int x = 8; x < 56; x++) {
            int top = y < 50, leg = y >= 56 && (x < 13 || x > 50);
            if (y >= 56 && !leg) continue;
            cv_px(x, y, top ? sh(R_BLOOD, 0.2f + 0.1f * (y == 46)) : sh(R_STONE, 0.3f + 0.12f * (x < 11) + 0.05f * h01(x, y, 5)));
        }
    for (int x = 10; x < 54; x += 6) for (int y = 50; y < 54; y++) cv_px(x, y, sh(R_BLOOD, 0.15f));   /* the pall's folds */
    cv_outline(sh(R_STONE, 0.02f));
    cv_store(SP_TR_BIER);
}
void crawl_hourglass(unsigned char *pix, int stride, int x0, int y0, float sand, int state, float t) {
    #define HG(x, y, c) (pix[(y0 + (y)) * stride + x0 + (x)] = (unsigned char)(c))
    sand = state ? 0.0f : clampf(sand, 0.0f, 1.0f);
    for (int x = 0; x < 21; x++) { HG(x, 0, sh(R_WOOD, 0.55f)); HG(x, 1, sh(R_WOOD, 0.35f)); HG(x, 31, sh(R_WOOD, 0.55f)); HG(x, 32, sh(R_WOOD, 0.35f)); }
    for (int y = 2; y < 31; y++) { HG(1, y, sh(R_WOOD, 0.45f)); HG(19, y, sh(R_WOOD, 0.3f)); }   /* its posts */
    for (int r = 0; r < 29; r++) {                                  /* the glass: two bulbs and a neck */
        int y = 2 + r, top = r < 14, d = top ? 13 - r : r - 15;
        float hw = r == 14 ? 0.5f : 0.8f + (float)d * 0.55f;
        if (hw > 7.0f) hw = 7.0f - (float)(d > 11) * 1.5f;
        int w = (int)hw;
        for (int x = 10 - w; x <= 10 + w; x++) {
            int edge = x == 10 - w || x == 10 + w;
            if (state == 1 && edge && ((y * 7 + x) % 5 < 2)) continue;   /* broken: the glass gone in places */
            uint8_t c = edge ? sh(R_GHOST, 0.55f) : sh(R_STONE, 0.06f);
            if (!edge && top && r >= 14 - (int)(sand * 12.0f + 0.5f)) c = sh(R_GOLD, 0.55f + 0.1f * ((x + y) & 1));
            if (!edge && !top && r != 14) {
                float fill = (1.0f - sand) * 12.0f, mound = fill - (float)abs(x - 10) * 0.35f;
                if (state == 1) mound = 1.5f - (float)abs(x - 10) * 0.2f;   /* spilled: a little left in the bottom */
                if ((float)(28 - r) < mound) c = sh(R_GOLD, 0.5f + 0.1f * ((x + y) & 1));
            }
            HG(x, y, c);
        }
    }
    if (state == 0 && sand > 0.0f && sand < 1.0f)                   /* the stream, grains in it */
        for (int y = 16; y < 30; y++) if (((y + (int)(t * 24.0f)) & 3) != 0) HG(10, y, sh(R_GOLD, 0.75f));
    if (state == 2) { HG(7, 8, sh(R_GHOST, 0.9f)); HG(8, 9, sh(R_GHOST, 0.9f)); HG(8, 10, sh(R_GHOST, 0.9f)); HG(9, 11, sh(R_GHOST, 0.9f)); HG(11, 22, sh(R_GHOST, 0.9f)); HG(12, 23, sh(R_GHOST, 0.9f)); }   /* cracked */
    if (state == 1)                                                 /* shards below it */
        for (int k = 0; k < 7; k++) HG(3 + k * 2 + (k & 1), 30 - (k % 3), sh(R_GHOST, 0.7f));
    #undef HG
}

void crawl_art_run(int level) {
    float dk = crawl_decay(level);
    uint32_t s = 7001u + (uint32_t)level * 7u, field = s + 50u;
    if (crawl_set(level) == 1) catacombs_run(dk, s, field);
    else if (crawl_set(level) == 2) hollows_run(dk, s, field);
    else if (crawl_set(level) == 3) deep_run(dk, s, field);
    else crypts_run(dk, s, field);                                  /* the Crypts, and the runs not drawn yet */
    gen_trdoor(crawl_set(level));                                   /* a trial's door, on the run's own */
}

void crawl_art_init(void) {
    static int done;
    if (done) return;
    done = 1;
    build_palette();
    build_colormap();
    build_rank_luts();
    gen_stone(T_STONE, R_STONE, 11);
    gen_rubble(T_RUBBLE, 23);
    gen_skulls(T_SKULLS, 31);
    gen_brick(T_BRICK, 47);
    gen_moss(T_MOSS, 53);
    gen_niche(T_NICHE, 11);
    gen_banner(T_BANNER, 11);
    for (int f = 0; f < 3; f++) gen_torch(T_TORCH0 + f, f, 11);
    gen_door(T_DOOR, 71);
    gen_jamb(T_JAMB, 79);
    gen_locked(T_LOCKED, 83, R_GOLD);
    gen_locked(T_LOCKED_S, 85, R_IRON);
    gen_locked(T_LOCKED_C, 87, R_BLOOD);
    gen_sign(T_SIGN, 91);
    gen_gate(T_GATE, 97);
    gen_font(T_FONT, 101, 1); gen_font(T_FONT_DRY, 101, 0);
    gen_grate(T_GRATE, 103);
    hz_walls();
    for (int f = 0; f < 4; f++) gen_portal(T_PORTAL0 + f, f, 107);
    gen_sealed(T_SEALED);
    gen_repository(T_REPOSITORY);
    gen_exit(T_EXIT0, 0, 0);
    gen_exit(T_EXIT1, 1, 0);
    gen_exit(T_EXIT_OPEN, 1, 1);
    gen_passage(T_PASSAGE, 1);
    for (int k = 0; k < 3; k++) gen_paper(T_PAPER + k, k, 301);
    gen_bdoor(T_BDOOR, 301);
    for (int f = 0; f < 4; f++) gen_visportal(T_VISPORT0 + f, f, 301);
    gen_carpet(T_CARPET, 307);
    for (int k = 0; k < 4; k++) gen_ceiling(T_CEILTILE + k, k, 311);
    for (int k = 0; k < 3; k++) gen_white(T_WHITE + k, k, 401);
    gen_sdoor(T_SDOOR, 401);
    gen_scarpet(T_SCARPET, 407);
    gen_sceiling(T_SCEIL, 0, 411); gen_sceiling(T_SCEILLAMP, 1, 411);

    spr_bones(SP_BONES1, 0, 5);
    spr_bones(SP_BONES2, 1, 9);
    spr_skullpile();
    spr_cobweb();
    spr_chains();
    spr_urn();
    spr_pillar();
    spr_candles();
    spr_skeleton(SP_SKEL_A, 1.0f, 0.0f, 3, 0);
    spr_skeleton(SP_SKEL_B, -1.0f, 0.0f, 3, 0);
    spr_skeleton(SP_SKEL_D1, 0.0f, 0.35f, 3, 0);
    spr_skeleton(SP_SKEL_D2, 0.0f, 0.7f, 3, 0);
    spr_skeleton(SP_SKEL_D3, 0.0f, 1.0f, 3, 0);
    spr_wraith(SP_GHOST_A, 0.0f, 0);
    spr_wraith(SP_GHOST_B, PI_F, 0);
    spr_wraith(SP_GHOST_D1, 1.0f, 1);
    spr_wraith(SP_GHOST_D2, 2.5f, 1);
    spr_bat(SP_BAT_A, 0);
    spr_bat(SP_BAT_B, 1);
    spr_bat(SP_BAT_D, 2);
    spr_fire(SP_FIRE_A, 0);
    spr_fire(SP_FIRE_B, 1);
    for (int f = 0; f < 3; f++) spr_boom(SP_BOOM1 + f, f);
    spr_skeleton(SP_SKEL_T, 0.3f, 0.0f, 3, 1);
    spr_shieldskel(SP_SHIELD_A, 1.0f, 0, 1); spr_shieldskel(SP_SHIELD_B, -1.0f, 0, 1); spr_shieldskel(SP_SHIELD_S, 0.2f, 1, 1);
    spr_shieldskel(SP_SHIELD_XA, 1.0f, 0, 0); spr_shieldskel(SP_SHIELD_XB, -1.0f, 0, 0); spr_shieldskel(SP_SHIELD_XS, 0.2f, 1, 0);
    for (int h = 0; h < 4; h++) spr_wskull(SP_WSKULL0 + h, h);
    for (int f = 0; f < 3; f++) spr_spider(SP_SPIDER_A + f, f);
    spr_spider(SP_SPIDER_D, 3);
    spr_ball(SP_SKULLFIRE_A, 10.0f, R_FLAME, R_BLOOD, 0, 1); spr_ball(SP_SKULLFIRE_B, 10.5f, R_FLAME, R_BLOOD, 1, 1);
    spr_ball(SP_HELL_A, 12.0f, R_FLAME, R_BLOOD, 0, 0); spr_ball(SP_HELL_B, 12.5f, R_FLAME, R_BLOOD, 1, 0);
    spr_frost(); spr_spear(); spr_wisp(); spr_spark(); spr_flask();
    spr_burn(SP_BURN_A, 0); spr_burn(SP_BURN_B, 1);
    spr_soulurn(); spr_ringitem(); spr_pedestal(); spr_items(); spr_black_altar(); spr_double();
    spr_shrine_body(0); spr_shrine_body(1); spr_shrine_glow(0); spr_shrine_glow(1);
    for (int k = 0; k <= 9; k++) spr_srune(k);
    spr_ccoin();
    spr_scrip(); spr_hextotem();
    spr_grit(SP_GRIT0, 0); spr_grit(SP_GRIT1, 1); spr_rock(); spr_rubble();
    spr_erupt(SP_ERUPT0, 0); spr_erupt(SP_ERUPT1, 1); spr_voidcol(); spr_hands();
    spr_desks(); spr_cooler(SP_COOLER, 0); spr_cooler(SP_COOLER_LOW, 1);
    for (int f = 0; f < 4; f++) spr_bonefly(SP_BONEFLY0 + f, f * PI_F * 0.25f);
    spr_coins(SP_COINS_A, 0);
    spr_coins(SP_COINS_B, 1);
    spr_chalice();
    spr_gems();
    spr_crown();
    spr_chest();
    static const float KS[4] = { 1.0f, 0.5f, 0.12f, -0.5f };
    for (int f = 0; f < 4; f++) { spr_key(SP_KEY0 + f, KS[f], R_GOLD); spr_key(SP_KEYS0 + f, KS[f], R_IRON); spr_key(SP_KEYC0 + f, KS[f], R_BLOOD); }
    spr_potion();
    spr_hooks();
    spr_gibbet(SP_GIBBET, 0); spr_gibbet(SP_GIBBET_SKEL, 1);
    spr_chandelier(SP_CHANDELIER_A, 0); spr_chandelier(SP_CHANDELIER_B, 1);
    spr_roots(); spr_websheet();
    for (int k = 0; k < 4; k++) spr_tomb(SP_TOMB_ROUND + k, k);
    spr_tomb(SP_TOMB_FRESH, 4);
    spr_lectern(); spr_mapware(); spr_lampstand();
    spr_orb(SP_ORB_A, 0); spr_orb(SP_ORB_B, 1); spr_lance(); spr_ballista(); spr_fspirit(SP_FSPIRIT_A, 0); spr_fspirit(SP_FSPIRIT_B, 1); spr_rime(); spr_mote();
    spr_deadbell(SP_DBELL, 0); spr_deadbell(SP_DBELL_C, 1); spr_font(SP_FONT, 0); spr_font(SP_FONT_C, 1);
    spr_censer(); spr_puff(SP_SMOKE, R_STONE, 0.25f, 0.6f, 81); spr_puff(SP_VAPOUR, R_MOSS, 0.2f, 0.55f, 83); spr_puff(SP_GAS, R_WARM, 0.35f, 0.75f, 87);
    spr_votive(); spr_hz_reliq(SP_RELIQ, 0); spr_hz_reliq(SP_RELIQ_O, 1); spr_snare(); spr_spikes(); spr_ashfall();
    spr_pz_skull(); spr_pz_plinth(); spr_pz_tablet();             /* puzzles' things (1.0.0b13) */
    for (int k = 0; k < 8; k++) spr_pz_dial(SP_PZ_DIAL0 + k, k);
    spr_pz_lever(SP_PZ_LEVER0, 0); spr_pz_lever(SP_PZ_LEVER1, 1); spr_pz_candle(SP_PZ_CANDLE0, 0); spr_pz_candle(SP_PZ_CANDLE1, 1);
    for (int k = 0; k < 16; k++) spr_pz_mark(SP_MARK0 + k, k);
    for (int n = 1; n <= 9; n++) spr_pz_pips(SP_PIP1 + n - 1, n);
    spr_pz_niche(); for (int k = 0; k < 4; k++) spr_pz_effigy(SP_PZ_EFFIGY0 + k, k);
    spr_pz_pan(); for (int k = 0; k < 3; k++) spr_pz_scale(SP_PZ_SCALE0 + k, k);
    spr_pz_casket(); spr_pz_banner();
    for (int k = 0; k < 9; k++) spr_pz_cistern(SP_PZ_CISTERN0 + k, k);
    spr_pz_saint(); spr_pz_emblem(); spr_pz_spout(); spr_pz_mill(SP_PZ_MILL0, 0); spr_pz_mill(SP_PZ_MILL1, 1); spr_pz_idol(); spr_pz_lock(); spr_pz_wheel();
    spr_pz_board(); spr_pz_box(); spr_pz_sack(); spr_pz_hook(); spr_pz_tag(); spr_pz_socket(); spr_pz_fossil(); spr_pz_stone();
    spr_pz_lamp(SP_PZ_LAMP0, 0); spr_pz_lamp(SP_PZ_LAMP1, 1); for (int k = 0; k < 4; k++) spr_pz_prism(SP_PZ_PRISM0 + k, k);
    spr_pz_geode(SP_PZ_GEODE, 0); spr_pz_geode(SP_PZ_GEODE1, 1); spr_pz_throne(); spr_pz_statue(); spr_pz_vent(SP_PZ_VENT0, 0); spr_pz_vent(SP_PZ_VENT1, 1);
    spr_pz_slab(); spr_pz_numstone(); for (int k = 0; k < 26; k++) spr_pz_letter(SP_PZ_LETTER0 + k, k);
    for (int k = 0; k < 4; k++) spr_hide(SP_HIDE0 + k, k);
    spr_bshrine(); spr_bshrine_eyes(SP_BSHRINE_E0, 0); spr_bshrine_eyes(SP_BSHRINE_E1, 1);
    for (int k = 0; k < 4; k++) spr_trlight(SP_TRLIGHT0 + k, k);
    spr_tr_bier();
    spr_tr_prop(SP_TR_PROP0, 0); spr_tr_prop(SP_TR_PROP1, 1);
    for (int f = 0; f < 3; f++) spr_firepit(SP_FIREPIT_A + f, f);
    spr_totem(SP_TOTEM_A, 0); spr_totem(SP_TOTEM_B, 1);
    spr_shambler(SP_SHAMB_A, 1.0f, 0); spr_shambler(SP_SHAMB_B, -1.0f, 0); spr_shambler(SP_SHAMB_GRAB, 0.0f, 1);
    spr_shambler(SP_SHAMB_RISE1, 0.0f, 2); spr_shambler(SP_SHAMB_RISE2, 0.0f, 3); spr_shambler(SP_SHAMB_D1, 0.0f, 4); spr_shambler(SP_SHAMB_D2, 0.0f, 5);
    spr_drowned(SP_DROWN_A, 1.0f, 0); spr_drowned(SP_DROWN_B, -1.0f, 1); spr_drowned(SP_DROWN_GRAB, 0.0f, 2);
    spr_drowned(SP_DROWN_RISE, 0.0f, 3); spr_drowned(SP_DROWN_BURST, 0.0f, 4); spr_drowned(SP_DROWN_POOL, 0.0f, 5);
    spr_ghoul(SP_GHOUL_A, 1.0f, 0); spr_ghoul(SP_GHOUL_B, -1.0f, 1); spr_ghoul(SP_GHOUL_LEAP, 0.0f, 2); spr_ghoul(SP_GHOUL_FEED, 0.0f, 3); spr_ghoul(SP_GHOUL_D, 0.0f, 4);
    spr_bestiary_bones();
    for (int f = 0; f < 4; f++) { spr_hunter(SP_HUNTER_A + f, f); spr_brood(SP_BROOD_A + f, f); }
    for (int f = 0; f < 3; f++) {                                  /* the Wandering Spider: a pale skull on its back */
        static const char *const SK[6] = { ".XXXXX.", "XXXXXXX", "XooXooX", "XXXXXXX", ".XXoXX.", ".X.X.X." };
        uint8_t *d = crawl_spr[SP_BSPIDER_A + f];
        memcpy(d, crawl_spr[SP_HUNTER_A + f], TS * TS);
        for (int y = 0; y < 6; y++)
            for (int x = 0; x < 7; x++)
                if (SK[y][x] != '.') d[(29 + x) * TS + 35 + y] = SK[y][x] == 'X' ? (uint8_t)(R_BONE + 13 - (y > 3)) : (uint8_t)(R_BONE + 1);
    }
    for (int f = 0; f < 3; f++) spr_sac(SP_SAC_A + f, f);
    spr_ling(SP_LING_A, 0); spr_ling(SP_LING_B, 1);
    spr_wraith_big(SP_WRAITH_A, 0.0f, 0); spr_wraith_big(SP_WRAITH_B, PI_F, 1); spr_wraith_big(SP_WRAITH_REACH, 1.0f, 2); spr_wraith_big(SP_WRAITH_BACK, 2.0f, 3);
    spr_shade(SP_SHADE_A, 0.0f, 0); spr_shade(SP_SHADE_B, PI_F, 1); spr_shade(SP_SHADE_TOUCH, 1.0f, 2); spr_shade(SP_SHADE_D, 2.0f, 3);
    for (int f = 0; f < 4; f++) spr_imp(SP_IMP_A + f, f);
    spr_ember(SP_EMBER_A, 0); spr_ember(SP_EMBER_B, 1);
    for (int f = 0; f < 5; f++) spr_gargoyle(SP_GARG_PERCH + f, f);
    spr_candlestand(SP_CANDLESTAND_A, 0); spr_candlestand(SP_CANDLESTAND_B, 1);
    spr_broken_arms(); spr_mound(SP_MOUND, 0); spr_mound(SP_PIT, 1);
    spr_puddle(SP_PUDDLE_A, 0); spr_puddle(SP_PUDDLE_B, 1); spr_stalag();
    spr_lantern(SP_LANTERN_A, 0); spr_lantern(SP_LANTERN_B, 1);
    for (int f = 0; f < 3; f++) { spr_brazier(SP_BRAZIER_A + f, f); spr_magma(SP_MAGMA_A + f, f); }
    {                                                               /* put out: the fire gone, ash where it sat */
        uint8_t *d = crawl_spr[SP_BRAZIER_OUT];
        memcpy(d, crawl_spr[SP_BRAZIER_A], TS * TS);
        for (int i = 0; i < TS * TS; i++)
            if (IS_FULLBRIGHT(d[i]) && d[i] != C_CLEAR) d[i] = (d[i + 1 < TS * TS ? i + 1 : i] == C_CLEAR || ((i * 7919) % 5)) ? (uint8_t)C_CLEAR : (uint8_t)(R_WARM + 3);
    }
    spr_collar(); spr_icicles(); spr_bell(); spr_carcass(); spr_armourstand(); spr_helm(); spr_timber();
    spr_minelamp(SP_MINELAMP_A, 0); spr_minelamp(SP_MINELAMP_B, 1); spr_cart(); spr_fungus();
    spr_crystal(SP_CRYSTAL_A, 0); spr_crystal(SP_CRYSTAL_B, 1); spr_obsidian();
    spr_altar(SP_ALTAR_A, 0); spr_altar(SP_ALTAR_B, 1);
    memcpy(crawl_spr[SP_TR_BRAZ0], crawl_spr[SP_BRAZIER_OUT], TS * TS); memcpy(crawl_spr[SP_TR_BRAZ1], crawl_spr[SP_BRAZIER_A], TS * TS);   /* a trial's, out and lit */
    spr_sides();
    find_glints();
}
