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

EyeAt crawl_eyes[NSPR];
static EyeAt g_eyes;                                    /* the eyes drawn on the sprite being made, for cv_store */
static int g_skbox[4] = { -1, -1, -1, -1 };             /* where the last hand-drawn skull went: the grime goes lightly on it */
static void cv_begin(uint8_t *buf, int w, int h) { cvp = buf; cvw = w; cvh = h; memset(buf, C_CLEAR, (size_t)w * h); memset(&g_eyes, 0, sizeof g_eyes); g_skbox[0] = g_skbox[2] = -1; }
static void cv_px(int x, int y, uint8_t c) { if ((unsigned)x < (unsigned)cvw && (unsigned)y < (unsigned)cvh) cvp[y * cvw + x] = c; }
static void cv_eye_mark(int x, int y) {                 /* an eye here, for the eye pass (the first two count) */
    if (g_eyes.n < 2 && (unsigned)x < (unsigned)TS && (unsigned)y < (unsigned)TS) { g_eyes.x[g_eyes.n] = (int8_t)x; g_eyes.y[g_eyes.n] = (int8_t)y; g_eyes.n++; }
}
static void cv_eye(int x, int y, uint8_t c) { cv_px(x, y, c); cv_eye_mark(x, y); }

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
    crawl_eyes[id] = g_eyes;
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

/* The skull, drawn by hand (docs/InitialArtPass.md section 4.1): a character a pixel, a step of the bone ramp in hex,
 * 0 black, @ an eye, . clear. The key light is up and to the left; the cavities are the darkest things on it; the jaw
 * hangs a pixel open and a tooth is gone. Front, and the profile facing right. */
#define SKULL_W 15
#define SKULL_H 16
_Static_assert(SKULL_H == 16, "the skulls' grids are sixteen rows: keep them so when redrawing");
static const char *const SKULL_FRONT[SKULL_H] = {
    ".....7aa97.....", "...7bdeedcb8...", "..8ceffeedcb8..", ".9dffeeeeddcb8.", ".7cdeeedddccb7.",
    ".5acdddddccb95.", ".6deeedbdeeda6.", ".800009c900006.", ".b0@01ad910@08.", "5d1003908200194", ".bedc90208bba6.",
    "..7cb60706a95..", "...8e3f4e3c6...", "...400000003...", "...7d2c0b2a5...", "....5acba84....",
};
static const char *const SKULL_SIDE[SKULL_H] = {
    "...68abbba86...", ".6acdeeeedcb7..", "6bdeeffeeddca6.", "8ceeffeeeddcb8.", "7bcdeeeedddccb7",
    "6abcdddddddccde", "59abccddddcb00a", "489abbccdcb0@0b", ".789abbccb9000d", ".6890abdeedcb0c", "..789a8bccdcb9b",
    "...789637bdccdb", "....8a5.9e3f3fd", "....79400000...", "....69ac2b2b2a.", ".....589abba96.",
};
/* a skull with its middle at cx, cy (its eyes a row above the middle); side: the profile, mirror: facing left;
 * eye: the eyes' colour (recorded for the eye pass), 0 dark sockets */
static void skull_stamp(float cx, float cy, int side, int mirror, uint8_t eye) {
    const char *const *g = side ? SKULL_SIDE : SKULL_FRONT;
    int x0 = (int)floorf(cx - SKULL_W / 2.0f + 0.5f), y0 = (int)floorf(cy - SKULL_H / 2.0f + 0.5f);
    g_skbox[0] = x0; g_skbox[1] = y0; g_skbox[2] = x0 + SKULL_W; g_skbox[3] = y0 + SKULL_H;
    for (int y = 0; y < SKULL_H; y++)
        for (int x = 0; x < SKULL_W; x++) {
            char ch = g[y][mirror ? SKULL_W - 1 - x : x];
            if (ch == '.') continue;
            if (ch == '@') { if (eye) cv_eye(x0 + x, y0 + y, eye); else cv_px(x0 + x, y0 + y, 0); continue; }
            cv_px(x0 + x, y0 + y, ch == '0' ? 0 : (uint8_t)(R_BONE + (ch <= '9' ? ch - '0' : ch - 'a' + 10)));
        }
}

/* A hand-drawn grid of any size (docs/InitialArtPass.md): hex 1-f a step of the body's ramp r1, g-v a step of a second
 * ramp r2 (g the darkest), A-P a step of the fire (full-bright), 0 black, @ an eye (recorded; drawn in eye), . clear.
 * Its top left at x0, y0; mirror flips it. */
static void grid_stamp(const char *const *g, int w, int h, int x0, int y0, int r1, int r2, uint8_t eye, int mirror) {
    for (int y = 0; y < h; y++)
        for (int x = 0; x < w; x++) {
            char ch = g[y][mirror ? w - 1 - x : x];
            int px = x0 + x, py = y0 + y;
            if (ch == '.') continue;
            if (ch == '@') { if (eye) cv_eye(px, py, eye); else cv_px(px, py, (uint8_t)(r1 + 4)); continue; }   /* eye 0: blank, a statue's */
            if (ch == '0') cv_px(px, py, 0);
            else if (ch >= '1' && ch <= '9') cv_px(px, py, (uint8_t)(r1 + ch - '0'));
            else if (ch >= 'a' && ch <= 'f') cv_px(px, py, (uint8_t)(r1 + ch - 'a' + 10));
            else if (ch >= 'g' && ch <= 'v') cv_px(px, py, (uint8_t)(r2 + ch - 'g'));
            else if (ch >= 'A' && ch <= 'P') cv_px(px, py, (uint8_t)(R_FLAME + ch - 'A'));
        }
}

/* ------------------------------------------------------------------ */
/* The caster's hand in motion                                         */
/* ------------------------------------------------------------------ */
/* The traced hand bent at its knuckles, for an idle and a cast. Each finger and the thumb has an axis from knuckle to
 * tip, through the middle of the finger as traced; a frame turns each about its knuckle (anticlockwise on the screen
 * for a positive turn) and draws it in along its length, which is a curl seen from behind. The bend comes in over the
 * finger's base, so the back of the hand stays as traced, and a curled finger's tip darkens, turned from the light.
 * Each pixel goes with the finger nearest it, blended a pixel or so either side of the line between two. */
static const float HAND_AXIS[5][4] = {                     /* knuckle x, y; tip x, y: index to little, then the thumb */
    { 28, 46, 19.2f, 11 }, { 48, 44, 25.2f, 1.5f }, { 67, 45, 43.5f, 7 }, { 83, 47, 64.5f, 19 }, { 30, 86, 3, 67 } };
static const float HAND_POSE[NHF][5][2] = {               /* each one's turn (radians) and length */
    [HF_REST]   = { { 0, 1 }, { 0, 1 }, { 0, 1 }, { 0, 1 }, { 0, 1 } },
    [HF_IDLE1]  = { { -0.02f, 0.97f }, { -0.01f, 0.96f }, { 0.01f, 0.955f }, { 0.03f, 0.95f }, { -0.03f, 0.99f } },
    [HF_IDLE2]  = { { -0.04f, 0.93f }, { -0.015f, 0.92f }, { 0.025f, 0.9f }, { 0.05f, 0.88f }, { -0.06f, 0.97f } },
    [HF_GATHER] = { { -0.07f, 0.85f }, { -0.02f, 0.82f }, { 0.04f, 0.8f }, { 0.09f, 0.78f }, { -0.14f, 0.92f } },   /* drawn in */
    [HF_CLAW]   = { { -0.08f, 0.72f }, { -0.02f, 0.68f }, { 0.05f, 0.66f }, { 0.1f, 0.64f }, { -0.2f, 0.86f } },     /* a claw */
    [HF_LOOSE]  = { { 0.1f, 1.03f }, { 0.035f, 1.04f }, { -0.035f, 1.03f }, { -0.1f, 1.02f }, { 0.12f, 1.0f } },      /* flung open */
    [HF_AFTER]  = { { 0.05f, 0.98f }, { 0.017f, 0.97f }, { -0.017f, 0.96f }, { -0.05f, 0.95f }, { 0.05f, 0.99f } },   /* easing */
};
const int8_t crawl_hand_shift[NHF][2] = { [HF_LOOSE] = { -3, -5 }, [HF_AFTER] = { -1, -2 } };   /* thrust up and out as it goes */
enum { HWX = 112, HWY = 108 };                             /* the part of the hand that bends: fingers and thumb */
static struct { float kx, ky, ux, uy, L, c, s, len, curl; } hbone[5];

static void hand_pose(int k) {                             /* the frame's bones, worked out once */
    for (int f = 0; f < 5; f++) {
        const float *a = HAND_AXIS[f];
        float ux = a[2] - a[0], uy = a[3] - a[1], L = sqrtf(ux * ux + uy * uy);
        hbone[f].kx = a[0]; hbone[f].ky = a[1]; hbone[f].ux = ux / L; hbone[f].uy = uy / L; hbone[f].L = L;
        hbone[f].c = cosf(HAND_POSE[k][f][0]); hbone[f].s = sinf(HAND_POSE[k][f][0]); hbone[f].len = HAND_POSE[k][f][1];
        hbone[f].curl = clampf((1.0f - HAND_POSE[k][f][1]) / 0.34f, 0.0f, 1.0f);
    }
}

static void hand_disp(float px, float py, float *dx, float *dy, float *curl) {
    float w[5], d[5][2], wa[5], frac[5], dmin = 1e9f, sum = 0.0f;
    for (int f = 0; f < 5; f++) {
        const float kx = hbone[f].kx, ky = hbone[f].ky, ux = hbone[f].ux, uy = hbone[f].uy, L = hbone[f].L;
        float rx = px - kx, ry = py - ky, along = rx * ux + ry * uy, perp = -rx * uy + ry * ux;
        float dist = fabsf(perp) + (along > L ? along - L : 0.0f);
        w[f] = dist;                                          /* no reach: past the tips it is air, and goes with them */
        if (dist < dmin) dmin = dist;
        wa[f] = smoothstepf(-3.0f, 9.0f, along);
        frac[f] = along / L;
        float al = along > 0.0f ? along * hbone[f].len : along;
        float qx = ux * al - uy * perp, qy = uy * al + ux * perp;   /* drawn in along it, then turned about the knuckle */
        d[f][0] = kx + qx * hbone[f].c + qy * hbone[f].s - px; d[f][1] = ky - qx * hbone[f].s + qy * hbone[f].c - py;
    }
    *dx = *dy = *curl = 0.0f;
    for (int f = 0; f < 5; f++) {                             /* the nearest finger leads, the next blends in over a few pixels */
        float e = (w[f] - dmin) / 2.8f;
        w[f] = 1.0f / (1.0f + e * e);
        sum += w[f];
    }
    for (int f = 0; f < 5; f++) {
        float k01 = w[f] / sum;
        *dx += k01 * wa[f] * d[f][0]; *dy += k01 * wa[f] * d[f][1];
        *curl += k01 * hbone[f].curl * clampf((frac[f] - 0.5f) * 2.0f, 0.0f, 1.0f);
    }
}

static int hand_ramp(uint8_t c) { return c >= R_GOLD && c < R_GOLD + 13 ? 100 : (c - 1) >> 4; }

void crawl_hand_frame(int k, const uint8_t *src, const uint8_t *lab, uint8_t *dst, uint8_t *dlab, int tip[2]) {
    memcpy(dst, src, HAND_W * HAND_H); memcpy(dlab, lab, HAND_W * HAND_H);
    hand_pose(k);
    float tx, ty, tc;
    hand_disp(32.0f, 1.0f, &tx, &ty, &tc);                         /* where the spell forms, between the tallest fingertips */
    tip[0] = (int)lroundf(32.0f + tx); tip[1] = (int)lroundf(1.0f + ty);
    if (k == HF_REST) return;
    static uint8_t dark[HWY * HWX];                                /* the darkening of a curled tip, by source pixel */
    static float mx[(HWY + 2) * (HWX + 2)], my[(HWY + 2) * (HWX + 2)];   /* where each pixel's middle came from */
    for (int y = 0; y < HWY; y++)
        for (int x = 0; x < HWX; x++) {
            float a, b, c;
            hand_disp(x + 0.5f, y + 0.5f, &a, &b, &c);
            dark[y * HWX + x] = (uint8_t)lroundf(c * 2.5f);
        }
    for (int y = -1; y <= HWY; y++)
        for (int x = -1; x <= HWX; x++) {
            float qx = x + 0.5f, qy = y + 0.5f, px = qx, py = qy, a, b, c;
            for (int it = 0; it < 6; it++) { hand_disp(px, py, &a, &b, &c); px = qx - a; py = qy - b; }
            mx[(y + 1) * (HWX + 2) + x + 1] = px; my[(y + 1) * (HWX + 2) + x + 1] = py;
        }
    for (int y = 0; y < HWY; y++)
        for (int x = 0; x < HWX; x++) {
            int m = (y + 1) * (HWX + 2) + x + 1, ctr = -1, n = 0;
            float px = mx[m], py = my[m];                          /* and its neighbours', for the samples between */
            float ax = (mx[m + 1] - mx[m - 1]) * 0.5f, ay = (my[m + 1] - my[m - 1]) * 0.5f;
            float bx = (mx[m + HWX + 2] - mx[m - HWX - 2]) * 0.5f, by = (my[m + HWX + 2] - my[m - HWX - 2]) * 0.5f;
            if (fabsf(ax) + fabsf(ay) > 2.0f || fabsf(bx) + fabsf(by) > 2.0f) { ax = 1.0f; ay = 0.0f; bx = 0.0f; by = 1.0f; }   /* a fold: plain samples */
            uint8_t cs[9];
            for (int sy = -1; sy <= 1; sy++)
                for (int sx = -1; sx <= 1; sx++) {
                    int ix = (int)floorf(px + (ax * sx + bx * sy) / 3.0f), iy = (int)floorf(py + (ay * sx + by * sy) / 3.0f);
                    if (ix < 0 || iy < 0 || ix >= HAND_W || iy >= HAND_H) continue;
                    uint8_t col = src[iy * HAND_W + ix];
                    if (col == C_CLEAR) continue;
                    if (col >= R_SKIN && col < R_SKIN + 16 && ix < HWX && iy < HWY)
                        col = (uint8_t)(R_SKIN + clampi(col - R_SKIN - dark[iy * HWX + ix], 1, 15));
                    if (!sx && !sy) ctr = iy * HAND_W + ix;
                    cs[n++] = col;
                }
            int at = y * HAND_W + x;
            if (n < 5) { dst[at] = C_CLEAR; dlab[at] = 0; continue; }
            int best = -1, bn = 0;                                 /* the ramp most of it fell in, at its middling step */
            for (int i = 0; i < n; i++) {
                int c = 0;
                for (int j = 0; j < n; j++) c += hand_ramp(cs[j]) == hand_ramp(cs[i]);
                if (c > bn) { bn = c; best = hand_ramp(cs[i]); }
            }
            uint8_t v[9]; int nv = 0;
            for (int i = 0; i < n; i++) if (hand_ramp(cs[i]) == best) {
                int j = nv++;
                while (j > 0 && v[j - 1] > cs[i]) { v[j] = v[j - 1]; j--; }
                v[j] = cs[i];
            }
            dst[at] = v[nv / 2];
            dlab[at] = ctr >= 0 ? lab[ctr] : 0;
        }
    for (int y = 0; y < HWY; y++)                                  /* the outline again, where the bend has moved it */
        for (int x = 0; x < HWX; x++) {
            uint8_t *p = &dst[y * HAND_W + x];
            if (*p < R_SKIN + 4 || *p >= R_SKIN + 16) continue;
            int edge = x == 0 || y == 0 || dst[y * HAND_W + x - 1] == C_CLEAR || dst[y * HAND_W + x + 1] == C_CLEAR
                    || dst[(y - 1) * HAND_W + x] == C_CLEAR || dst[(y + 1) * HAND_W + x] == C_CLEAR;
            if (edge) *p = (uint8_t)(R_SKIN + 1);
        }
}

/* ------------------------------------------------------------------ */
/* Sprites                                                             */
/* ------------------------------------------------------------------ */



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

/* the skeleton: a list of bones that can collapse into a heap */
typedef struct { int kind; float x0, y0, x1, y1, r; } Part;   /* kind 0 bone, 1 skull, 2 pelvis */

static float g_decay;                                   /* 0 on the first level, 1 eight levels down */
static void gem(float cx, float cy, float r, int ramp);
static void skel_draw(float walk, float k, uint32_t seed, int raise) {
    Part P[32]; int n = 0;
    g_skbox[0] = g_skbox[2] = -1;
    #define BONE(a, b, c, d, r) (P[n++] = (Part){ 0, a, b, c, d, r })
    float w = walk;
    BONE(29, 42, 27 + w * 3, 52, 1.6f); BONE(27 + w * 3, 52, 27 + w * 4.5f, 61, 1.4f); BONE(27 + w * 4.5f, 62, 23 + w * 4.5f, 62.5f, 1.0f);
    BONE(35, 42, 37 - w * 3, 52, 1.6f); BONE(37 - w * 3, 52, 37 - w * 4.5f, 61, 1.4f); BONE(37 - w * 4.5f, 62, 41 - w * 4.5f, 62.5f, 1.0f);
    P[n++] = (Part){ 2, 32, 40, 0, 0, 0 };
    BONE(32, 19, 32, 38, 1.4f);
    for (int i = 0; i < 4; i++) {
        float y = 23.5f + i * 3.2f;
        BONE(31, y, 25 + i * 0.6f, y + 2, 0.9f); BONE(25 + i * 0.6f, y + 2, 27 + i * 0.5f, y + 4.2f, 0.8f);
        BONE(33, y, 39 - i * 0.6f, y + 2, 0.9f); BONE(39 - i * 0.6f, y + 2, 37 - i * 0.5f, y + 4.2f, 0.8f);
    }
    BONE(24.5f, 21, 39.5f, 21.5f, 1.1f);
    BONE(24.5f, 21, 21, 31, 1.3f); BONE(21, 31, 19 - w * 2, 41, 1.1f);
    if (raise == 1) { BONE(39.5f, 21.5f, 46, 13, 1.3f); BONE(46, 13, 45, 5, 1.1f); BONE(39, 6, 51, 3, 1.0f); }   /* bone held high */
    else if (raise == 2) { BONE(39.5f, 21.5f, 47, 14, 1.3f); BONE(47, 14, 48, 6, 1.1f); }   /* arm up, to strike */
    else { BONE(39.5f, 21.5f, 43, 31, 1.3f); BONE(43, 31, 48, 24, 1.1f); }
    BONE(32, 17, 32, 20, 1.2f);
    P[n++] = (Part){ 1, 32, 10, 0, 0, 1.0f };
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
            uint8_t eye = g_decay > 0.55f ? (uint8_t)(R_GLOW + 13) : (uint8_t)(R_FLAME + 10);   /* the eyes go cold */
            skull_stamp(ncx, ncy - (kk > 0.9f ? 1.5f : 0.0f), kk > 0.6f, 0, k < 0.3f ? eye : 0);   /* fallen, it rolls aside */
            if (g_decay > 0.3f) { cv_line(ncx + 2, ncy - 8, ncx + 3, ncy - 3, R_BONE + 3); cv_px((int)ncx + 4, (int)ncy - 3, R_BONE + 2); }   /* a cracked skull */
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
                if (x >= g_skbox[0] && x < g_skbox[2] && y >= g_skbox[1] && y < g_skbox[3]) {   /* the skull keeps its face */
                    if (n < 0.2f + 0.25f * g_decay && c > R_BONE + 4) cv_sprite[y * TS + x] = (uint8_t)(c - 1);
                    continue;
                }
                if (n < 0.28f + 0.32f * g_decay) cv_sprite[y * TS + x] = (uint8_t)(c - 3 < R_BONE + 1 ? R_BONE + 1 : c - 3);
                if (g_decay > 0.35f && n > 0.78f - 0.12f * g_decay) cv_sprite[y * TS + x] = sh(R_MOSS, 0.3f + 0.3f * n);
            }
}

/* ---- the dressing (docs/InitialArtPass.md section 6): what a rank has done to a creature, as a layer over its sprite ----
 * The marks add up rank by rank: rags from II, damage from III, wounds from IV, regalia at V; the condition is the
 * rank's own: II moss and mould, III frost, IV old blood, V char; the gilded are dressed, not hurt. A creature's
 * function draws its sprite, stores it, then for each rank begins a layer (dress_begin), draws its kit and stores it. */
uint8_t crawl_dress[NDRESS][TS * TS];
int16_t crawl_dress_ix[NSPR][NRANK];
static int g_dress_n;
static const uint8_t *g_base;                           /* the body being dressed, column-major, for the kit's masks */
static int base_on(int x, int y) { return (unsigned)x < (unsigned)TS && (unsigned)y < (unsigned)TS && g_base[x * TS + y] != C_CLEAR; }
static uint8_t base_c(int x, int y) { return base_on(x, y) ? g_base[x * TS + y] : (uint8_t)C_CLEAR; }
static void dress_begin(int id) { g_base = crawl_spr[id]; cv_begin(cv_sprite, TS, TS); }
/* keep the layer where it shows the body (ramp) or hangs in the air, not over what the creature carries; a dark rim
 * where it stands out of the body */
static int g_dress_ramp2 = -1, g_dress_ramp3 = -1;         /* other ramps the layer may lie on (the bat's fur; the brood's eggs) */
static int dress_on(uint8_t b, int r) { return r >= 0 && b >= r && b < r + 16; }
static void dress_store(int id, int rank, int ramp) {
    static uint8_t tmp[TS * TS];
    for (int y = 0; y < TS; y++)
        for (int x = 0; x < TS; x++) {
            uint8_t b = base_c(x, y);
            if (b != C_CLEAR && b != 0 && !dress_on(b, ramp) && !dress_on(b, g_dress_ramp2) && !dress_on(b, g_dress_ramp3)) cv_sprite[y * TS + x] = C_CLEAR;
        }
    memcpy(tmp, cv_sprite, sizeof tmp);
    for (int y = 0; y < TS; y++)
        for (int x = 0; x < TS; x++) {
            if (tmp[y * TS + x] != C_CLEAR || base_on(x, y)) continue;
            int n = (x > 0 && tmp[y * TS + x - 1] != C_CLEAR && !IS_FULLBRIGHT(tmp[y * TS + x - 1]))
                  + (x < TS - 1 && tmp[y * TS + x + 1] != C_CLEAR && !IS_FULLBRIGHT(tmp[y * TS + x + 1]))
                  + (y < TS - 1 && tmp[(y + 1) * TS + x] != C_CLEAR && !IS_FULLBRIGHT(tmp[(y + 1) * TS + x]));
            if (n) cv_sprite[y * TS + x] = 0;
        }
    int ix = crawl_dress_ix[id][rank];
    if (ix < 0) { if (g_dress_n >= NDRESS) return; ix = g_dress_n++; crawl_dress_ix[id][rank] = (int16_t)ix; }
    for (int x = 0; x < TS; x++) for (int y = 0; y < TS; y++) crawl_dress[ix][x * TS + y] = cv_sprite[y * TS + x];
}
/* rags hanging from a line, len long, ragged and torn, swung by w; behind: only where the body isn't (a cloak) */
static void kit_rags(float x0, float y0, float x1, float y1, float len, float w, int ramp, float lo, float hi, int behind, uint32_t seed) {
    int n = (int)hypotf(x1 - x0, y1 - y0) + 1;
    for (int i = 0; i <= n; i++) {
        float t = (float)i / (float)n, x = x0 + (x1 - x0) * t, y = y0 + (y1 - y0) * t;
        float l = len * (0.5f + 0.5f * h01(i, 1, seed));
        if (h01(i, 2, seed) < 0.15f) l *= 0.35f;                    /* torn away */
        for (int k = 0; k < (int)l; k++) {
            float u = (float)k / len;
            int px = (int)(x + w * u * 1.6f + sinf((float)k * 0.6f + (float)i) * 0.35f), py = (int)(y + (float)k);
            if (behind && base_on(px, py)) continue;
            float v = lo + (hi - lo) * (1.0f - 0.7f * u) * (0.65f + 0.35f * h01(i, k, seed + 3)) - (((i + (int)(seed & 3)) % 4) == 0 ? 0.07f : 0.0f);
            cv_px(px, py, sh(ramp, v));
        }
    }
}
/* frost along the tops of the body and icicles under it; amount 0-1 */
static void kit_frost(float amount, uint32_t seed) {
    for (int y = 0; y < TS; y++)
        for (int x = 0; x < TS; x++) {
            if (!base_on(x, y) || base_c(x, y) == 0) continue;
            float r = h01(x, y, seed);
            if (!base_on(x, y - 1) && r < amount) cv_px(x, y, r < amount * 0.15f ? (uint8_t)C_WHITE : (uint8_t)(R_GHOST + 12 + (int)(r * 7.0f) % 4));
            else if (!base_on(x, y + 1) && r < amount * 0.5f) {   /* an icicle */
                int l = 1 + (int)(h01(x, y, seed + 1) * 3.5f * amount);
                for (int k = 1; k <= l; k++) cv_px(x, y + k, (uint8_t)(R_GHOST + 14 - k * 2));
            } else if (r < amount * 0.12f) cv_px(x, y, (uint8_t)(R_GHOST + 10));   /* rime */
        }
}
/* moss and mould on the body, and a few pale caps of fungus */
static void kit_moss(float amount, int caps, uint32_t seed) {
    for (int y = 0; y < TS; y++)
        for (int x = 0; x < TS; x++) {
            if (!base_on(x, y) || base_c(x, y) == 0) continue;
            float n = fbm(x / 4.0f, y / 4.0f, 16, seed);
            if (n > 1.0f - amount) cv_px(x, y, sh(R_MOSS, 0.15f + 0.5f * (n - 1.0f + amount) / amount + 0.15f * h01(x, y, seed)));
        }
    for (int i = 0, made = 0; i < 200 && made < caps; i++) {
        int x = 16 + (int)(h01(i, 1, seed) * 32.0f), y = 20 + (int)(h01(i, 2, seed) * 30.0f);
        if (!base_on(x, y) || base_on(x, y - 1)) continue;          /* on a top edge */
        cv_px(x, y - 1, sh(R_SKIN, 0.85f)); cv_px(x + 1, y - 1, sh(R_SKIN, 0.7f)); cv_px(x, y, sh(R_SKIN, 0.4f));
        made++;
    }
}
/* old blood run down from a wound, following the body */
static void kit_blood(float cx, float cy, int len, int width, uint32_t seed) {
    for (int c = 0; c < width; c++) {
        int x = (int)cx + c - width / 2, l = (int)((float)len * (0.5f + 0.5f * h01(c, 1, seed)));
        for (int k = 0; k < l; k++) {
            int y = (int)cy + k;
            if (!base_on(x, y)) { if (k > 2) break; continue; }
            cv_px(x, y, (uint8_t)(R_BLOOD + 2 + (int)(4.0f * (1.0f - (float)k / (float)(l + 1))) + (int)(h01(x, y, seed) * 2.0f)));
        }
    }
}
/* char and soot on the body, an ember or two still in it */
static void kit_char(float amount, uint32_t seed) {
    for (int y = 0; y < TS; y++)
        for (int x = 0; x < TS; x++) {
            if (!base_on(x, y) || base_c(x, y) == 0) continue;
            float n = fbm(x / 3.0f, y / 3.0f, 16, seed);
            if (n < amount) cv_px(x, y, (uint8_t)(R_IRON + 1 + (int)(n / amount * 4.0f)));
            else if (h01(x, y, seed + 5) > 0.992f) cv_px(x, y, (uint8_t)(R_FLAME + 4));
        }
}
/* gold leaf laid on the body's lit edges */
static void kit_gold(int ramp, float amount, uint32_t seed) {
    for (int y = 0; y < TS; y++)
        for (int x = 0; x < TS; x++) {
            uint8_t b = base_c(x, y);
            if (b < ramp || b >= ramp + 16) continue;
            int v = b - ramp;
            if (v >= 9 && fbm(x / 3.0f, y / 3.0f, 16, seed) > 1.0f - amount) cv_px(x, y, (uint8_t)(R_GOLD + clampi(v - 3, 4, NGOLD - 1)));
        }
}
/* an arrow in it: the shaft out at angle a, fletched */
static void kit_arrow(float x, float y, float a, float len) {
    float dx = cosf(a), dy = sinf(a);
    for (int k = 0; k <= (int)len; k++) cv_px((int)(x + dx * k), (int)(y + dy * k), sh(R_WOOD, 0.55f - 0.2f * (float)k / len));
    float ex = x + dx * len, ey = y + dy * len;
    for (int k = 0; k < 3; k++) { cv_px((int)(ex - dx * k - dy), (int)(ey - dy * k + dx), sh(R_GHOST, 0.75f)); cv_px((int)(ex - dx * k + dy), (int)(ey - dy * k - dx), sh(R_GHOST, 0.55f)); }
}
/* an iron circlet, broken: a band with spikes, one snapped */
static void kit_crown(float cx, float cy, float w, uint32_t seed) {
    for (int x = (int)(cx - w / 2); x <= (int)(cx + w / 2); x++) {
        float u = (x - cx) / (w / 2);
        cv_px(x, (int)cy, sh(R_IRON, 0.55f - 0.25f * u)); cv_px(x, (int)cy + 1, sh(R_IRON, 0.3f - 0.15f * u));
    }
    for (int k = 0; k < 4; k++) {
        float x = cx - w / 2 + 1.5f + (float)k * (w - 3.0f) / 3.0f;
        int h = k == 2 ? 1 : 3 + (k == 1);
        for (int j = 1; j <= h; j++) cv_px((int)x, (int)cy - j, sh(R_IRON, 0.7f - 0.1f * j - 0.15f * (x > cx)));
    }
    cv_px((int)(cx + 1), (int)cy + 1, sh(R_BRICK, 0.45f));               /* rust */
}
/* a chain sagging from (x0,y0) to (x1,y1), links light and dark; a shackle at the first end */
static void kit_chain(float x0, float y0, float x1, float y1, float sag, int ramp) {
    int n = (int)(hypotf(x1 - x0, y1 - y0) * 1.2f) + 1;
    for (int i = 0; i <= n; i++) {
        float t = (float)i / (float)n;
        int x = (int)(x0 + (x1 - x0) * t), y = (int)(y0 + (y1 - y0) * t + sag * 4.0f * t * (1.0f - t));
        cv_px(x, y, ramp == R_GOLD ? (uint8_t)(R_GOLD + ((i & 1) ? 11 : 6)) : sh(ramp, (i & 1) ? 0.7f : 0.3f));
    }
    if (ramp != R_GOLD) { cv_flat(x0, y0, 1.6f, 1.6f, sh(ramp, 0.5f)); cv_px((int)x0, (int)y0, 0); }
}
/* the skull's face turned to gold: a death-mask, the sockets left dark */
static void kit_mask(int x0, int y0, int x1, int y1) {
    for (int y = y0; y < y1; y++)
        for (int x = x0; x < x1; x++) {
            uint8_t b = base_c(x, y);
            if (b > R_BONE && b < R_BONE + 16) cv_px(x, y, (uint8_t)(R_GOLD + clampi((b - R_BONE) - 2, 2, NGOLD - 1)));
        }
}

/* the skeleton's dressing (and the shield-bearer's): hx, hy its right hand, w its stride */
static void skel_dress(int id, float w, float hx, float hy) {
    uint32_t sd = (uint32_t)id * 7u + 101u;
    for (int r = RK_II; r < NRANK; r++) {
        dress_begin(id);
        int gild = r == RK_GILDED;
        if (gild) {                                                /* the honoured dead: a clean shroud, gold on it */
            kit_rags(26, 42, 38, 42.5f, 8, w * 0.3f, R_WARM, 0.45f, 0.8f, 0, sd);
            for (int x = 26; x <= 38; x++) cv_px(x, 42, (uint8_t)(R_GOLD + 9 + (x & 1)));
            kit_gold(R_BONE, 0.55f, sd + 1);
            kit_mask(25, 3, 40, 14);
            kit_chain(26, 24, 38, 25, 2.5f, R_GOLD);
        } else {
            /* rags from II, longer each rank; the dead's cloak at V, behind them */
            float len = r == RK_II ? 9.0f : r == RK_III ? 10.0f : 11.0f;
            int cr = r == RK_IV ? R_BLOOD : r == RK_V ? R_IRON : R_WARM;
            kit_rags(26, 42, 38, 43, len, w * (r == RK_III ? 0.25f : 1.0f), cr, cr == R_WARM ? 0.2f : 0.08f, cr == R_WARM ? 0.55f : 0.35f, 0, sd);
            kit_rags(23, 20.5f, 28, 21.5f, r == RK_II ? 5.0f : 7.0f, w * 0.5f, cr, cr == R_WARM ? 0.2f : 0.08f, cr == R_WARM ? 0.5f : 0.3f, 0, sd + 9);
            if (r == RK_V) kit_rags(22, 20, 42, 21, 30, w * 0.6f, R_IRON, 0.03f, 0.2f, 1, sd + 17);
            /* the condition: the rank's own */
            if (r == RK_II) kit_moss(0.3f, 3, sd + 2);
            if (r == RK_III) kit_frost(0.75f, sd + 3);
            if (r == RK_V) kit_char(0.32f, sd + 4);
            /* damage from III: a notch out of the crown, the crack wider */
            if (r >= RK_III) { cv_px(35, 3, 0); cv_px(36, 4, 0); cv_px(36, 3, 0); cv_line(35, 5, 34, 9, sh(R_BONE, 0.15f)); }
            /* wounds from IV: an arrow in the ribs, a cut across the brow, and what ran from them */
            if (r >= RK_IV) {
                kit_blood(34, 29, 12, 2, sd + 5); kit_blood(28, 8, 5, 1, sd + 6);
                kit_arrow(34, 28, -0.6f, 9.0f);
                cv_line(27, 6, 30, 8, 0); cv_px(28, 6, sh(R_BLOOD, 0.3f));
            }
            /* regalia at V: an iron crown, broken, and a shackle on the wrist with its chain */
            if (r == RK_V) { kit_crown(32, 3, 12, sd); kit_chain(hx, hy, hx + 3, hy + 14, 1.5f, R_IRON); }
        }
        dress_store(id, r, R_BONE);
    }
}

static void spr_skeleton(int id, float walk, float k, uint32_t seed, int raise) {
    skel_draw(walk, k, seed, raise);
    cv_outline(0);
    cv_store(id);
    if (k < 0.3f) skel_dress(id, walk, raise == 1 ? 46.0f : raise == 2 ? 48.0f : 48.0f, raise == 1 ? 13.0f : raise == 2 ? 6.0f : 24.0f);
}

/* A skeleton with a round wooden shield on its arm and a sword: it closes in and strikes */
static void spr_shieldskel(int id, float walk, int swing, int shield) {
    skel_draw(walk, 0.0f, 13, swing ? 2 : 0);
    if (swing) {                                                   /* the blade raised over its head */
        cv_capsule(48, 6, 46, -18, 1.0f, R_IRON, 0.5f, 1.0f);
        cv_capsule(44, 7, 52, 5, 0.9f, R_WOOD, 0.3f, 0.6f);
    } else {                                                       /* held low and forward */
        cv_capsule(48, 24, 55, 9, 1.0f, R_IRON, 0.5f, 1.0f);
        cv_capsule(45, 26, 51, 22, 0.9f, R_WOOD, 0.3f, 0.6f);
    }
    if (shield) {                                                  /* the shield on the left arm, in front of the ribs */
        float cx = 22.5f, cy = 32.0f;
        for (int y = 17; y < 48; y++)
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
        cv_px(18, 26, sh(R_IRON, 0.9f)); cv_px(27, 38, sh(R_IRON, 0.9f));
    } else {                                                       /* what is left of it: a splintered strap */
        cv_capsule(19, 28, 26, 35, 1.0f, R_WOOD, 0.2f, 0.45f);
        cv_line(24, 29, 21, 24, sh(R_WOOD, 0.4f)); cv_line(26, 33, 29, 29, sh(R_WOOD, 0.35f));
    }
    cv_outline(0);
    cv_store(id);
    skel_dress(id, walk, 48.0f, swing ? 6.0f : 24.0f);
}

/* The wall skull (docs/InitialArtPass.md section 8): the master skull three times over (Scale3x keeps its corners), grain
 * and cracks at the finer scale, set in the stone to watch the halls. Asleep, embers deep in its sockets; woken, fire
 * gathers in them; blazing, it shows through the teeth and the cracks; broken, it splits and the right half falls. */
static uint8_t g_big[64 * 64];
static int g_bigw, g_bigh;
static void grid_scale3(const char *const *g, int w, int h, int mirror) {   /* into g_big: palette indices, C_CLEAR, 0 the sockets */
    uint8_t src[SKULL_H][SKULL_W];
    for (int y = 0; y < h; y++)
        for (int x = 0; x < w; x++) {
            char ch = g[y][mirror ? w - 1 - x : x];
            src[y][x] = ch == '.' ? (uint8_t)C_CLEAR : (ch == '0' || ch == '@') ? 0 : (uint8_t)(R_BONE + (ch <= '9' ? ch - '0' : ch - 'a' + 10));
        }
    #define S3(yy, xx) ((yy) < 0 || (yy) >= h || (xx) < 0 || (xx) >= w ? (uint8_t)C_CLEAR : src[yy][xx])
    g_bigw = w * 3; g_bigh = h * 3;
    for (int y = 0; y < h; y++)
        for (int x = 0; x < w; x++) {
            uint8_t A = S3(y - 1, x - 1), B = S3(y - 1, x), C = S3(y - 1, x + 1), D = S3(y, x - 1), E = src[y][x], F = S3(y, x + 1);
            uint8_t G = S3(y + 1, x - 1), H = S3(y + 1, x), I = S3(y + 1, x + 1), o[9];
            int db = D == B && D != H && B != F, bf = B == F && B != D && F != H, dh = D == H && D != B && H != F, hf = H == F && D != H && B != F;
            o[0] = db ? D : E; o[1] = (db && E != C) || (bf && E != A) ? B : E; o[2] = bf ? F : E;
            o[3] = (db && E != G) || (dh && E != A) ? D : E; o[4] = E; o[5] = (bf && E != I) || (hf && E != C) ? F : E;
            o[6] = dh ? D : E; o[7] = (dh && E != I) || (hf && E != G) ? H : E; o[8] = hf ? F : E;
            for (int k = 0; k < 9; k++) g_big[(y * 3 + k / 3) * 64 + x * 3 + k % 3] = o[k];
        }
    #undef S3
}
static void spr_wskull(int id, int heat) {
    cv_begin(cv_sprite, TS, TS);
    grid_scale3(SKULL_FRONT, SKULL_W, SKULL_H, 0);
    int x0 = 32 - g_bigw / 2, y0 = 6;
    for (int y = 0; y < g_bigh; y++)                               /* the bone, with a grain finer than its pixels */
        for (int x = 0; x < g_bigw; x++) {
            uint8_t c = g_big[y * 64 + x];
            if (c == C_CLEAR) continue;
            if (c > R_BONE && c < R_BONE + 16) {
                float r = h01(x, y, 71), n = fbm(x / 5.0f, y / 5.0f, 16, 73);
                int v = c - R_BONE + (r < 0.12f ? -1 : r > 0.93f ? 1 : 0) - (n < 0.3f ? 1 : 0);
                c = (uint8_t)(R_BONE + clampi(v, 1, 15));
            }
            cv_px(x0 + x, y0 + y, c);
        }
    static const float CR[3][4] = { { 33, 6, 1.1f, 13 }, { 15, 18, 0.5f, 9 }, { 40, 34, 1.9f, 8 } };   /* cracks wandering through it */
    for (int k = 0; k < 3; k++) {
        float x = CR[k][0], y = CR[k][1], a = CR[k][2];
        for (int i = 0; i < (int)CR[k][3]; i++) {
            a += (h01(k, i, 79) - 0.5f) * 0.9f; x += cosf(a); y += sinf(a);
            uint8_t c = cv_sprite[(int)y * TS + (int)x];
            if (c > R_BONE && c < R_BONE + 16) cv_px((int)x, (int)y, heat == 2 ? (uint8_t)(R_FLAME + 8 + (i & 3)) : (uint8_t)(R_BONE + 1));
        }
    }
    /* its eyes: the sockets' middles, 3 by 3 from the grid's @ */
    float ex[2], ey[2]; int ne = 0;
    for (int y = 0; y < SKULL_H; y++) for (int x = 0; x < SKULL_W && ne < 2; x++) if (SKULL_FRONT[y][x] == '@') { ex[ne] = x0 + x * 3 + 1.5f; ey[ne] = y0 + y * 3 + 1.5f; ne++; }
    if (heat == 3) {                                               /* broken: split, the right half falling away */
        static uint8_t tmp[TS * TS];
        memcpy(tmp, cv_sprite, sizeof tmp);
        for (int y = 0; y < TS; y++)
            for (int x = 0; x < TS; x++) {
                int jag = 33 + (int)(3.0f * sinf(y * 0.7f)) + (y & 1);
                if (x >= jag - 1 && x <= jag) { cv_sprite[y * TS + x] = C_CLEAR; continue; }
                if (x > jag) { cv_sprite[y * TS + x] = C_CLEAR; int sy = y - 6, sx = x - 3; if (sy >= 0 && sx > jag) cv_sprite[y * TS + x] = tmp[sy * TS + sx]; }
            }
        for (int i = 0; i < 7; i++) cv_ellipse(14 + h01(i, 1, 81) * 36, 57 + h01(i, 2, 81) * 5, 1.5f + h01(i, 3, 81) * 2, 1.2f, R_BONE, 0.2f, 0.8f);   /* shards */
        cv_outline(0); cv_store(id); return;
    }
    for (int k = 0; k < ne; k++) {
        cv_eye_mark((int)ex[k], (int)ey[k]);
        if (!heat) { cv_px((int)ex[k], (int)ey[k], (uint8_t)(R_FLAME + 3)); continue; }   /* a last ember */
        float r = heat == 1 ? 3.0f : 5.5f;
        for (int y = (int)(ey[k] - 7); y <= (int)(ey[k] + 7); y++)   /* fire gathering in the socket, and only there */
            for (int x = (int)(ex[k] - 7); x <= (int)(ex[k] + 7); x++) {
                if ((unsigned)x >= TS || (unsigned)y >= TS || cv_sprite[y * TS + x] != 0) continue;
                float d = hypotf(x + 0.5f - ex[k], (y + 0.5f - ey[k]) * 0.9f) / r;
                if (d < 1.0f) cv_px(x, y, (uint8_t)(R_FLAME + clampi((int)((1.1f - d * 0.8f) * 15), 2, 15)));
            }
        if (heat == 2) for (int i = 0; i < 6; i++) cv_px((int)(ex[k] + (h01(i, k + 2, 83) - 0.5f) * 7), (int)(ey[k] - 5 - h01(i, k + 5, 83) * 6), (uint8_t)(R_FLAME + 9 + (i & 3)));
    }
    if (heat)                                                      /* and in its mouth, and up its nose when it blazes */
        for (int y = y0 + 27; y < y0 + 48; y++)
            for (int x = x0; x < x0 + g_bigw; x++) {
                if (cv_sprite[y * TS + x] != 0) continue;
                int mouth = y >= y0 + 39, nose = y < y0 + 39 && x > x0 + 15 && x < x0 + 30;
                if (mouth) cv_px(x, y, (uint8_t)(R_FLAME + (heat == 2 ? 7 + (int)(h01(x, y, 85) * 6.0f) : 2 + (int)(h01(x, y, 85) * 3.0f))));
                else if (nose && heat == 2) cv_px(x, y, (uint8_t)(R_FLAME + 4 + (int)(h01(x, y, 87) * 4.0f)));
            }
    cv_outline(0);
    cv_store(id);
}

/* the wall skull's dressing: what its rank has done to it, as the skeletons' */
static void wskull_dress(int id) {
    for (int r = RK_II; r < NRANK; r++) {
        dress_begin(id);
        if (r == RK_II) kit_moss(0.35f, 2, 401);
        if (r == RK_III) kit_frost(0.8f, 403);
        if (r == RK_IV) { kit_blood(23, 37, 14, 2, 405); kit_blood(41, 37, 12, 2, 407); }   /* tears of old blood */
        if (r == RK_V) { kit_char(0.35f, 409); kit_crown(32, 7, 26, 411); }
        if (r == RK_GILDED) { kit_gold(R_BONE, 0.6f, 413); kit_mask(10, 6, 55, 42); }
        dress_store(id, r, R_BONE);
    }
}

/* a small skull by hand, for the deacon's hood (dim, in shadow) and his staff, and the web spider's mark at rank V: hex R_BONE, @ the eyes */
static const char *const SKULL_SMALL[10] = {
    "..56775..", ".5799975.", "4799a9974", "40@090@04", "450090054", ".5770775.", ".4a0a0a4.", "..30003..", "..4a0a4..", "...353...",
};

/* The web spider's head from the front (docs/InitialArtPass.md, section 8): a cluster of eight eyes like coals, two
 * great ones low in front, two above them, two smaller still, two at the sides. Chitin in R_IRON (hex), the eyes fire. */
static const char *const SPIDER_HEAD[8] = {
    "...2345432...", "..34H676H43..", ".3456J7J6543.", "2H567888765H2",
    "3567@N7N@7653", "2456NM6MN6542", ".34567876543.", "..345666543..",
};

/* a spider's leg in two long joints, from its body to the knee and down to the foot: banded where the joints' plates
 * meet (paler, or dark in the band ramp if one is given), bristles standing out at the knee */
static void chitin_leg(float ax, float ay, float kx, float ky, float fx, float fy, float r, int ramp, float lo, float hi, int band, uint32_t seed) {
    cv_capsule(ax, ay, kx, ky, r, ramp, lo, hi);
    cv_capsule(kx, ky, fx, fy, r * 0.75f, ramp, lo, hi * 0.9f);
    for (int seg = 0; seg < 2; seg++) {                              /* bands across each joint */
        float x0 = seg ? kx : ax, y0 = seg ? ky : ay, x1 = seg ? fx : kx, y1 = seg ? fy : ky, L = hypotf(x1 - x0, y1 - y0);
        float nx = -(y1 - y0) / L, ny = (x1 - x0) / L;
        for (int k = 1; k <= 2; k++) {
            float t = k / 3.0f, bx = x0 + (x1 - x0) * t, by = y0 + (y1 - y0) * t;
            for (int q = -1; q <= 1; q++) cv_px((int)(bx + nx * q * 0.8f), (int)(by + ny * q * 0.8f), band >= 0 ? (uint8_t)(band + 1 + (q == 0)) : sh(ramp, hi + 0.12f));
        }
    }
    for (int q = 0; q < 4; q++) {                                    /* bristles at the knee */
        float an = h01(q, 1, seed) * TAU_F, l = 1.5f + 1.5f * h01(q, 2, seed);
        cv_line(kx, ky, kx + cosf(an) * l, ky + sinf(an) * l, sh(ramp, hi * 0.8f));
    }
    cv_px((int)kx, (int)ky, sh(ramp, hi + 0.15f));
}

/* a spider's abdomen: a swollen shell pitted all over, bristles round its rim, lit from above */
static void spider_abdomen(float cx, float cy, float rx, float ry, int ramp, uint32_t seed) {
    for (int y = (int)(cy - ry) - 1; y <= (int)(cy + ry) + 1; y++)
        for (int x = (int)(cx - rx) - 1; x <= (int)(cx + rx) + 1; x++) {
            float nx = (x + 0.5f - cx) / rx, ny = (y + 0.5f - cy) / ry, d = nx * nx + ny * ny;
            if (d > 1.0f) continue;
            float v = 0.05f + 0.3f * (1.0f - d) * (0.6f + 0.4f * (-nx * 0.5f - ny * 0.7f + 0.5f));
            int gx = (int)floorf((x + 64) / 4.0f), gy = (int)floorf((y + 64) / 4.0f);   /* pits, one to a few pixels */
            float px = gx * 4.0f - 64.0f + 1.0f + 2.0f * h01(gx, gy, seed), py = gy * 4.0f - 64.0f + 1.0f + 2.0f * h01(gx, gy, seed + 1);
            float pd = hypotf(x + 0.5f - px, y + 0.5f - py);
            if (pd < 0.9f) v -= 0.12f; else if (pd < 1.7f && x + 0.5f > px && y + 0.5f > py) v += 0.06f;
            cv_px(x, y, sh(ramp, v));
        }
    for (int q = 0; q < 30; q++) {                                   /* bristles round its rim */
        float an = TAU_F * q / 30.0f + h01(q, 3, seed) * 0.15f, l = 1.0f + 1.6f * h01(q, 4, seed);
        float ex = cx + cosf(an) * rx, ey = cy + sinf(an) * ry;
        cv_line(ex, ey, ex + cosf(an) * l, ey + sinf(an) * l, sh(ramp, 0.28f));
    }
}

/* fangs: curved down and in from the jaws' bulges, pale at the tips, a drop of venom on each (falling, biting) */
static void spider_fangs(float cx, float y0, float len, int biting) {
    for (int s = -1; s <= 1; s += 2) {
        cv_ellipse(cx + s * 2.2f, y0, 1.8f, 2.2f, R_IRON, 0.12f, 0.34f);   /* the jaw's bulge */
        float spread = biting ? 1.8f : 0.0f, px = cx + s * (2.4f + spread), py = y0 + 1.0f;
        for (int k = 0; k <= (int)len; k++) {
            float t = k / len, x = cx + s * (2.4f + spread - t * (1.6f + spread * 0.6f) + sinf(t * PI_F) * 0.8f), y = y0 + 1.0f + k;
            cv_px((int)x, (int)y, sh(R_BONE, 0.45f + 0.45f * t)); px = x; py = y;
        }
        cv_px((int)px, (int)py + 1, (uint8_t)(R_MOSS + 14)); cv_px((int)px, (int)py + 2, (uint8_t)(R_MOSS + 10));   /* the drop (no glint: a pale pair there reads as a second pair of eyes) */
        if (biting) cv_px((int)px, (int)py + 5, (uint8_t)(R_MOSS + 11));   /* another falling */
    }
}

/* The web spider: a black shell pitted all over and a red mark, the head before it with its eight coals of eyes, pale
 * fangs with venom on them, banded legs bristling at the knee. Its red eyes are all you see of it in its web until it
 * drops. frame 0-1 walking, 2 biting (the front legs up), 3 dead on its back. Chitin R_IRON (PF_CHITIN). */
static void spr_spider(int id, int frame) {
    cv_begin(cv_sprite, TS, TS);
    if (frame == 3) {                                              /* dead on its back, its legs curled over it */
        spider_abdomen(32, 52, 10.0f, 7.0f, R_IRON, 421);
        for (int s = -1; s <= 1; s += 2)
            for (int l = 0; l < 4; l++)
                chitin_leg(32 + s * (3 + l), 48, 32 + s * (9 + l * 2.5f), 38 - l * 2.0f, 32 + s * (5 + l * 1.5f), 33 - l, 1.1f, R_IRON, 0.05f, 0.3f, -1, 423 + (uint32_t)(l * 2 + s + 1));
        cv_ellipse(32, 46, 6.0f, 4.0f, R_IRON, 0.06f, 0.3f);
        cv_outline(0);
        cv_store(id);
        return;
    }
    float bob = frame == 1 ? 1.0f : 0.0f;
    spider_abdomen(32, 28 + bob, 10.5f, 9.5f, R_IRON, 431);         /* the abdomen, looming behind */
    for (int k = 0; k < 7; k++) {                                   /* its mark: an hourglass in blood */
        int w = k < 3 ? 3 - k : (k > 3 ? k - 3 : 0);
        for (int q = -w; q <= w; q++) cv_px(32 + q, (int)(24 + bob) + k, (uint8_t)(R_BLOOD + 8 + (q == 0) * 2));
    }
    for (int s = -1; s <= 1; s += 2)
        for (int l = 3; l >= 0; l--) {                              /* legs: up to a high knee and down, the back ones first */
            float sw = frame == 1 ? (((l + (s > 0)) & 1) ? 2.0f : -2.0f) : 0.0f;
            float ax = 32 + s * (4.0f + l * 0.8f), ay = 40 + l * 0.6f;
            float kx = 32 + s * (12.0f + l * 4.0f) + sw, ky = 29 - l * 0.8f + sw * 0.5f;
            float fx = 32 + s * (14.0f + l * 5.0f) + sw * 1.5f, fy = 59 - l * 2.5f;
            if (frame == 2 && l == 0) { kx = 32 + s * 11.0f; ky = 22; fx = 32 + s * 7.0f; fy = 14; }   /* the front legs raised */
            if (frame == 2 && l == 1) { ky -= 4; fy -= 6; }
            chitin_leg(ax, ay, kx, ky, fx, fy, 1.25f, R_IRON, 0.05f, 0.36f, -1, 441 + (uint32_t)(l * 2 + s + 1));
        }
    cv_ellipse(32, 40, 7.0f, 5.0f, R_IRON, 0.06f, 0.4f);            /* the head's shell */
    grid_stamp(SPIDER_HEAD, 13, 8, 26, 35, R_IRON, R_IRON, (uint8_t)(R_FLAME + 14), 0);
    spider_fangs(32, 44, frame == 2 ? 6.0f : 4.0f, frame == 2);
    cv_outline(0);
    cv_store(id);
}

/* the web spider's dressing by rank: II silk clinging in strands and mould on its shell, III frost and its shell
 * cracked, IV old blood on its fangs and a pale scar across its back, V charred, the pale skull on its back as the
 * Wandering Spider wears it, gilded gold in its bands and along its shell */
static void spider_dress(int id) {
    for (int r = RK_II; r < NRANK; r++) {
        g_dress_ramp2 = r == RK_V ? R_BLOOD : R_BONE;               /* its fangs too; at V over its mark, which the skull takes */
        dress_begin(id);
        if (r == RK_II) {
            kit_moss(0.25f, 0, 451);
            for (int q = 0; q < 5; q++) {                            /* silk, clinging */
                float x0 = 10 + 44 * h01(q, 1, 453), y0 = 24 + 10 * h01(q, 2, 453), x1 = x0 + (h01(q, 3, 453) - 0.5f) * 16, y1 = y0 + 6 + 10 * h01(q, 4, 453);
                for (int k = 0; k <= 12; k++) { float t = k / 12.0f; int x = (int)(x0 + (x1 - x0) * t), y = (int)(y0 + (y1 - y0) * t + sinf(t * PI_F) * 2.0f); if (base_on(x, y) || (k & 1)) cv_px(x, y, sh(R_GHOST, 0.65f)); }
            }
        }
        if (r == RK_III) {
            kit_frost(0.7f, 455);
            float x = 26, y = 22;                                    /* the shell cracked */
            for (int k = 0; k < 12; k++) { cv_px((int)x, (int)y, 0); cv_px((int)x + 1, (int)y, sh(R_IRON, 0.5f)); x += h01(k, 1, 457) < 0.5f ? 1.0f : 0.0f; y += 1.0f; }
        }
        if (r >= RK_IV && r != RK_GILDED) {
            kit_blood(30, 45, 6, 2, 459); kit_blood(34, 45, 6, 2, 461);
            for (int k = 0; k < 13; k++) cv_px(24 + k, 31 - k / 3 + (k & 1), sh(R_BONE, 0.7f - 0.2f * (k & 1)));   /* a scar */
        }
        if (r == RK_V) {
            kit_char(0.3f, 463);
            cv_flat(32.5f, 25.8f, 6.2f, 6.6f, 0);                 /* the skull's ground, dark */
            grid_stamp(SKULL_SMALL, 9, 10, 28, 21, R_BONE, R_BONE, 0, 0);
        }
        if (r == RK_GILDED)
            for (int y = 0; y < TS; y++)
                for (int x = 0; x < TS; x++) {
                    uint8_t b = base_c(x, y);
                    if (b >= R_IRON + 6 && b < R_IRON + 16 && h01(x, y, 465) < 0.8f) cv_px(x, y, (uint8_t)(R_GOLD + clampi(b - R_IRON, 4, NGOLD - 1)));
                    else if (b >= R_IRON + 4 && b < R_IRON + 6 && !base_on(x, y - 1)) cv_px(x, y, (uint8_t)(R_GOLD + 6));
                }
        dress_store(id, r, R_IRON);
    }
    g_dress_ramp2 = -1;
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

/* The spectre's face (docs/InitialArtPass.md, section 8): long and drawn, the brow heavy over hollow sockets with a
 * light deep in each, the cheeks fallen in, the mouth dragged open in a wail down to a narrow chin. In the ghost's ramp. */
static const char *const SPECTRE_FACE[21] = {
    "..789999987..", ".79abbbbba97.", "69bcccdcccb96", "7acdde8eddca7", "7bdca7c7acdb7",
    "6ca317c713ac6", "5b2005b5002b5", "590016b610095", "470@16a61@074", "320148a841023",
    "3436938396343", "379ba575ab973", "2689462649862", "2478320238742", "1367300037631",
    ".35720002753.", ".34620002643.", ".24630003642.", "..464101464..", "..356303653..",
    "...4687864...",
};
/* and turned to the right: the brow and nose at the front, the jaw hanging open */
static const char *const SPECTRE_SIDE[21] = {
    "....789a.", "...79bcc.", "..79bcdd.", "..8bcdee.", "..9cdeed.",
    "..acdcbd9", "..bc7214.", "..bb20@1.", "..ab5125.", "..acb89b.",
    "..9cdcbc8", "..9bccba7", "..8ab964.", "..79720..", "..7850000",
    "..6840000", "..674000.", "..57510..", "..567423.", "...57876.",
    "....566..",
};

/* a hand of mist at hx, hy: a pale palm, long fingers fanned towards a (radians, 0 = out to side s), their tips
 * trailing off into wisps, and a few motes falling from it */
static void mist_hand(float hx, float hy, int s, float a, float ph, uint32_t seed) {
    cv_ellipse(hx, hy, 2.2f, 1.9f, R_GHOST, 0.5f, 0.85f);
    for (int f = 0; f < 4; f++) {
        float fa = a + (f - 1.5f) * 0.32f, len = 6.0f + (f == 1 || f == 2 ? 1.5f : 0.0f);
        float tx = hx + s * cosf(fa) * len, ty = hy + sinf(fa) * len;
        cv_line(hx, hy, tx, ty, sh(R_GHOST, 0.8f));
        for (int k = 1; k <= 4; k++) {                              /* the tip going to mist, drifting down */
            float wx = tx + s * cosf(fa) * k * 1.2f, wy = ty + sinf(fa) * k * 1.2f + k * k * 0.25f;
            if ((k & 1) || h01(f, k, seed) < 0.4f) cv_px((int)wx, (int)wy, sh(R_GHOST, 0.62f - k * 0.08f));
        }
    }
    for (int k = 0; k < 5; k++) {                                   /* motes falling from the hand */
        float d = fmodf(h01(k, 1, seed) * 9.0f + ph * 1.3f, 9.0f);
        cv_px((int)(hx + (h01(k, 2, seed) - 0.5f) * 7.0f), (int)(hy + 3.0f + d), sh(R_GHOST, 0.55f - d * 0.04f));
    }
}

/* The spectre: the long face pale in the mouth of a hood, a shroud trailing to a tail that sways and frays, hands of
 * mist held out. recoil: thrown back, its arms flung up, the shroud flaring (the shriek as it goes). */
static void spr_spectre(int id, float ph, int recoil) {
    cv_begin(cv_sprite, TS, TS);
    float lift = recoil ? -3.0f : 0.0f, oy = 17.0f + lift;          /* the hood's opening, about the face */
    for (int y = 1; y < TS; y++)
        for (int x = 0; x < TS; x++) {
            float yy = y + 0.5f - lift, t = (yy - 26.0f) / 37.0f;
            float sway = t > 0.0f ? sinf(ph + t * 2.6f) * 4.5f * t : 0.0f;   /* the tail drifts */
            float dx = x + 0.5f - 32.0f - sway, hw, flare = recoil ? 0.36f : 0.2f;
            if (yy < 2.0f) continue;
            if (yy < 12.0f) hw = 10.8f * sqrtf((yy - 2.0f) / 10.0f);  /* the hood's crown */
            else if (yy < 26.0f) hw = 10.8f + (yy - 12.0f) * 0.1f;   /* falling past the face */
            else if (yy < 40.0f) hw = 12.2f + (yy - 26.0f) * flare;   /* the shoulders, the breast */
            else hw = (12.2f + 14.0f * flare) * (1.0f - (yy - 40.0f) / 31.0f);   /* narrowing to the tail */
            if (fabsf(dx) > hw) continue;
            if (yy > 30.0f && fabsf(dx) > hw - 2.0f && h01(dx > 0.0f, (int)(yy / 2.0f), 967) < 0.3f) continue;   /* torn at its edges */
            if (yy > 44.0f && h01(x, (int)(yy / 3.0f), 961 + (int)(ph * 3.0f)) * 0.7f + h01(x, y, 963) * 0.3f < (yy - 44.0f) / 22.0f) continue;   /* fraying in streaks */
            float u = dx / hw, v = 0.1f + 0.2f * (1.0f - u * u) - fmaxf(0.0f, t) * 0.07f;
            if (fmodf(dx * 0.42f + 20.0f + sinf(yy * 0.08f + ph) * 0.8f, 2.2f) < 0.4f) v -= 0.06f;   /* folds */
            if (fabsf(u) > 0.86f) v += 0.28f;                       /* a cold rim of light */
            float ox = (x + 0.5f - 32.0f) / 7.8f, oyy = (y + 0.5f - oy) / 11.2f, o = ox * ox + oyy * oyy;
            if (o <= 1.0f) v = 0.02f;                               /* the hood's mouth, all shadow */
            else if (o <= 1.35f && y < oy + 9.0f) v += 0.22f;       /* its lip */
            cv_px(x, y, sh(R_GHOST, v));
        }
    grid_stamp(SPECTRE_FACE, 13, 21, 26, (int)(7.0f + lift), R_GHOST, R_GHOST, C_WHITE, 0);
    for (int x = 25; x <= 39; x++) {                                /* the hood's brim over the brow */
        float ox = (x + 0.5f - 32.0f) / 7.8f;
        if (fabsf(ox) >= 1.0f) continue;
        int ye = (int)(oy - 11.2f * sqrtf(1.0f - ox * ox));
        cv_px(x, ye, sh(R_GHOST, 0.42f)); cv_px(x, ye + 1, sh(R_GHOST, 0.3f));
    }
    for (int s = -1; s <= 1; s += 2) {                              /* sleeves, and the hands of mist */
        float sx = 32.0f + s * 10.0f, sy = 25.0f + lift;
        float hx = recoil ? 32.0f + s * 22.0f : 32.0f + s * 19.0f, hy = recoil ? 11.0f + lift : 33.0f + 2.0f * sinf(ph + s);
        cv_capsule(sx, sy, hx - s * 2.5f, hy + (recoil ? 2.0f : -1.0f), 3.3f, R_GHOST, 0.12f, 0.48f);
        cv_ellipse(hx - s * 2.5f, hy + (recoil ? 2.0f : -1.0f), 3.4f, 2.6f, R_GHOST, 0.15f, 0.5f);   /* the sleeve's mouth */
        mist_hand(hx, hy, s, recoil ? -1.15f : 0.75f, ph, 971 + (uint32_t)(s + 1));
    }
    cv_store(id);
}

/* The bat's face (docs/InitialArtPass.md, section 8): ears taller than its head and flesh inside, a small pointed face,
 * a leaf of a nose, the mouth open on two fine fangs, eyes like coals. Fur in R_IRON (hex), skin in R_SKIN (g-v). */
static const char *const BAT_FACE[13] = {
    "1...........1", "21.........12", "3l1.......1l3", "3ml2.....2lm3", "3nml2...2lmn3",
    ".3nl34543ln3.", ".2l4566654l2.", "..35@767@53..", "..2457o7542..", "...35non53...",
    "...4f000f4...", "....f0h0f....", ".....363.....",
};
/* and turned to the right: the ear back, the snout and its leaf ahead, a fang showing */
static const char *const BAT_SIDE[11] = {
    "..1......", ".1l1.....", ".2ml2....", ".3nm3....", "..3n4....", ".345653..",
    "34567@75.", "4567776on", "3456500..", ".345f0f..", "..343....",
};

/* the bat's wing from the front, side s, raised by up: an arm to the wrist, three long fingers fanning to the trailing
 * edge, the membrane scalloped between them and torn at its edge, a hooked thumb at the wrist */
static void bat_wing_front(int s, float up, uint32_t seed) {
    float shx = 32 + s * 4.0f, shy = 27.0f, wx = 32 + s * 13.0f, wy = 20.0f - 5.0f * up;
    float tip[4][2] = { { 32 + s * 25.0f, 14.0f - 8.0f * up }, { 32 + s * 28.0f, 26.0f - 5.0f * up }, { 32 + s * 22.0f, 35.0f - 1.5f * up },
                        { 32 + s * 5.0f, 36.0f } };                  /* the last is the membrane's foot, at the leg */
    float p[24]; int n = 0;
    p[n++] = shx; p[n++] = shy; p[n++] = wx; p[n++] = wy;
    for (int f = 0; f < 3; f++) {
        p[n++] = tip[f][0]; p[n++] = tip[f][1];
        for (int k = 1; k <= 2; k++) {                               /* the scallop to the next, drawn in towards the wrist */
            float t = k / 3.0f, mx = tip[f][0] + (tip[f + 1][0] - tip[f][0]) * t, my = tip[f][1] + (tip[f + 1][1] - tip[f][1]) * t;
            float pull = 0.22f * sinf(t * PI_F);
            p[n++] = mx + (wx - mx) * pull; p[n++] = my + (wy - my) * pull;
        }
    }
    p[n++] = tip[3][0]; p[n++] = tip[3][1];
    cv_poly(p, n / 2, R_PURPLE, 0.1f, 0.34f, wy, 22.0f);
    for (int y = 0; y < TS; y++)                                     /* the membrane's shading, dithered, thinner at its edge */
        for (int x = s < 0 ? 0 : 32; x < (s < 0 ? 32 : TS); x++) {
            uint8_t c = cv_sprite[y * TS + x];
            if (c < R_PURPLE || c >= R_PURPLE + 16) continue;
            float v = 0.1f + 0.24f * clampf(1.0f - (y + 0.5f - wy) / 22.0f, 0.0f, 1.0f) + (((x + y) & 1) ? 0.03f : -0.03f);
            float far = fabsf(x + 0.5f - 32.0f) / 28.0f;
            cv_px(x, y, sh(R_PURPLE, v + 0.08f * far * far));
        }
    for (int k = 0; k < 2; k++) {                                    /* torn at the edge: a notch or two */
        int f = (int)(h01(k, 1, seed) * 2.99f);
        float t = 0.3f + 0.4f * h01(k, 2, seed), mx = tip[f][0] + (tip[f + 1][0] - tip[f][0]) * t, my = tip[f][1] + (tip[f + 1][1] - tip[f][1]) * t;
        float pull = 0.22f * sinf(t * PI_F); mx += (wx - mx) * pull; my += (wy - my) * pull;
        for (int q = 0; q < 3; q++) { float qx = mx + (wx - mx) * 0.05f * q, qy = my + (wy - my) * 0.05f * q; cv_px((int)qx, (int)qy, C_CLEAR); cv_px((int)qx + s, (int)qy, q < 2 ? C_CLEAR : sh(R_PURPLE, 0.05f)); }
    }
    cv_capsule(shx, shy, wx, wy, 0.9f, R_IRON, 0.3f, 0.5f);         /* the arm */
    for (int f = 0; f < 3; f++) {                                    /* the fingers, a knuckle halfway */
        float kx = wx + (tip[f][0] - wx) * 0.45f, ky = wy + (tip[f][1] - wy) * 0.45f;
        cv_line(wx, wy, kx, ky, sh(R_IRON, 0.5f)); cv_line(kx, ky, tip[f][0], tip[f][1], sh(R_IRON, 0.4f));
        cv_px((int)kx, (int)ky, sh(R_IRON, 0.68f));
    }
    cv_px((int)wx, (int)wy, sh(R_IRON, 0.7f));                       /* the wrist, and its thumb's hook */
    cv_line(wx, wy, wx + s * 1.0f, wy - 3.0f, sh(R_IRON, 0.55f)); cv_px((int)(wx + s * 2.0f), (int)(wy - 3.0f), sh(R_IRON, 0.8f));
}

/* the bat's body: a small furred barrel, paler at the breast, its feet drawn up with claws */
static void bat_body(float cx, float cy) {
    for (int y = (int)(cy - 6); y <= (int)(cy + 6); y++)
        for (int x = (int)(cx - 5); x <= (int)(cx + 5); x++) {
            float nx = (x + 0.5f - cx) / 4.2f, ny = (y + 0.5f - cy) / 5.6f, d = nx * nx + ny * ny;
            if (d > 1.0f) continue;
            float v = 0.12f + 0.22f * (1.0f - d) + (fabsf(nx) < 0.45f && ny > -0.3f ? 0.08f : 0.0f) + (h01(x, y, 561) - 0.5f) * 0.14f;   /* fur, its breast paler */
            cv_px(x, y, sh(R_IRON, v));
        }
    for (int s = -1; s <= 1; s += 2) {                               /* feet */
        cv_line(cx + s * 1.5f, cy + 5.0f, cx + s * 2.5f, cy + 7.5f, sh(R_IRON, 0.3f));
        cv_px((int)(cx + s * 3.0f), (int)(cy + 8.0f), sh(R_IRON, 0.6f)); cv_px((int)(cx + s * 2.0f), (int)(cy + 8.0f), sh(R_IRON, 0.5f));
    }
}

static void spr_bat(int id, int frame) {
    cv_begin(cv_sprite, TS, TS);
    if (frame == 2) {                                                /* dead: the wings folded about it, falling */
        for (int s = -1; s <= 1; s += 2) {
            float wl[8] = { 32 + s * 2.0f, 33, 32 + s * 7.0f, 38, 32 + s * 6.0f, 56, 32 + s * 1.0f, 52 };
            cv_poly(wl, 4, R_PURPLE, 0.06f, 0.24f, 33, 22);
            cv_line(32 + s * 3.0f, 34, 32 + s * 6.0f, 55, sh(R_IRON, 0.45f));
        }
        bat_body(32, 44);
        grid_stamp(BAT_FACE, 13, 13, 26, 27, R_IRON, R_SKIN, 0, 0);  /* its eyes gone dark */
        cv_outline(0);
        cv_store(id);
        return;
    }
    float up = frame == 0 ? 1.0f : -1.0f;
    for (int s = -1; s <= 1; s += 2) bat_wing_front(s, up, 571 + (uint32_t)(s + 1) * 3u);
    bat_body(32, 31);
    grid_stamp(BAT_FACE, 13, 13, 26, 17, R_IRON, R_SKIN, (uint8_t)(R_FLAME + 8), 0);
    cv_outline(0);
    cv_store(id);
}

/* the bat's dressing by rank (it stops at IV): II its wings holed and mould in its fur, III frost on its ears and bones,
 * a finger broken and a wing ripped through, IV old blood about its jaws and down its fangs, the wings' edges shredded
 * and dark with it, gilded gold on its bones and its ears' rims */
static void bat_dress(int id, float up) {
    g_dress_ramp2 = R_IRON;
    for (int r = RK_II; r < NRANK; r++) {
        if (r == RK_V) continue;
        dress_begin(id);
        if (r != RK_GILDED)
            for (int k = 0; k < 6 + 4 * (r - RK_II); k++) {          /* holes worn through the membrane */
                int x = 6 + (int)(h01(k, r, 581) * 52.0f), y = 12 + (int)(h01(k, r + 9, 581) * 24.0f);
                if (base_c(x, y) >= R_PURPLE && base_c(x, y) < R_PURPLE + 16 && base_c(x + 1, y) >= R_PURPLE && base_c(x + 1, y) < R_PURPLE + 16) { cv_px(x, y, 0); if (k & 1) cv_px(x + 1, y, 0); }
            }
        if (r == RK_II) kit_moss(0.22f, 0, 583);
        if (r == RK_III) {
            kit_frost(0.7f, 585);
            float wx = 32 - 13.0f, wy = 20.0f - 5.0f * up, tx = 32 - 25.0f, ty = 14.0f - 8.0f * up;   /* a finger broken, bent back */
            float kx = wx + (tx - wx) * 0.5f, ky = wy + (ty - wy) * 0.5f;
            cv_line(kx, ky, kx - 3.0f, ky + 5.0f, sh(R_IRON, 0.4f));
            for (int q = 0; q < 9; q++) cv_px((int)(32 + 20 - q * 0.4f + (q & 1)), (int)(30 - 1.5f * up - q * 1.1f), 0);   /* a rip in the other */
        }
        if (r >= RK_IV && r != RK_GILDED) {
            kit_blood(29, 27, 5, 2, 587); kit_blood(35, 27, 5, 2, 589);
            for (int y = 0; y < TS; y++)                              /* the edges shredded and dark */
                for (int x = 0; x < TS; x++) {
                    uint8_t b = base_c(x, y);
                    if (b < R_PURPLE || b >= R_PURPLE + 16 || (base_on(x, y + 1) && base_on(x - 1, y) && base_on(x + 1, y))) continue;
                    float hh = h01(x, y, 591);
                    if (hh < 0.3f) cv_px(x, y, 0); else if (hh < 0.7f) cv_px(x, y, (uint8_t)(R_BLOOD + 3));
                }
        }
        if (r == RK_GILDED)
            for (int y = 0; y < TS; y++)                              /* gold on its bones and the rims of its ears */
                for (int x = 0; x < TS; x++) {
                    uint8_t b = base_c(x, y);
                    if (b >= R_IRON + 6 && b < R_IRON + 16 && (y < 22 || !base_on(x, y - 1) || h01(x, y, 593) < 0.5f)) cv_px(x, y, (uint8_t)(R_GOLD + clampi(b - R_IRON - 2, 4, NGOLD - 1)));
                }
        dress_store(id, r, R_PURPLE);
    }
    g_dress_ramp2 = -1;
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

static void stone_px(int x, int y, float nx, float shade_v, int seed) {
    float v = 0.2f + 0.55f * lit(nx, 0.0f, sqrtf(fmaxf(0.0f, 1.0f - nx * nx))) + shade_v + (fbm(x / 4.0f, y / 4.0f, 16, (uint32_t)seed) - 0.5f) * 0.22f;
    if (h01(x, y, seed + 5) > 0.97f) v -= 0.2f;                    /* pits */
    if (y > 50 && fbm(x / 3.0f, y / 3.0f, 32, (uint32_t)seed + 9) > 0.6f) { cv_px(x, y, sh(R_MOSS, 0.15f + 0.4f * v)); return; }   /* moss creeping up */
    cv_px(x, y, sh(R_STONE, v));
}
static void soil(float cx, float w, int ground, uint32_t seed);
static void spr_tomb(int id, int kind) {                           /* her plot's newest stone (the graves of the runs are spr_grave's) */
    cv_begin(cv_sprite, TS, TS);
    switch (kind) {
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
    default: break;
    }
    cv_outline(0);
    cv_store(id);
}

/* ------------------------------------------------------------------ */
/* The bestiary (docs/PLAN-bestiary.md). A creature's body is drawn in  */
/* its family's ramp (crawl.h, PF_*) so that a rank can recolour it;    */
/* everything else on it keeps its colour.                              */
/* ------------------------------------------------------------------ */

/* move the drawing down by dy and cut it off at the ground, for things climbing out of it */
static void cv_sink(int dy, int ground) {
    EyeAt e = { 0 };
    for (int i = 0; i < g_eyes.n; i++)
        if (g_eyes.y[i] + dy < ground) { e.x[e.n] = g_eyes.x[i]; e.y[e.n] = (int8_t)(g_eyes.y[i] + dy); e.n++; }
    g_eyes = e;
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

/* the shambler's face (docs/InitialArtPass.md section 8): sunken sockets, the nose rotted to two holes, the jaw dropped
 * open on yellowed teeth, hair in clumps; hex R_WARM (PF_FLESH), g-v R_IRON for the hair */
static const char *const SHAMB_HEAD[16] = {
    "...hjjhj.h...", "..hj6787jh...", ".hj689a986j..", ".j6789aa9875.", ".5679aaa9865.", ".46000900064.", ".460@797@064.",
    "..520696025..", "..47a0a0a74..", "..4589a9854..", "..3e0e00e03..", "...2000002...", "...3000003...", "...4d00d04...",
    "....57975....", ".....363.....",
};
/* The shambler: a corpse in rags out of its grave, one arm reaching, the other hanging.
 * pose: 0 walking, 1 grabbing, 2-3 climbing out (head and hands, then to the waist), 4 sinking to
 * its knees, 5 down. Its skin is R_WARM (PF_FLESH). */
static void spr_shambler(int id, float w, int pose) {
    cv_begin(cv_sprite, TS, TS);
    int grab = pose == 1 || pose == 2 || pose == 3;
    if (pose == 5) {                                               /* down: lying along the floor, an arm flung out */
        cv_flat(32, 61.5f, 26, 2.5f, R_WARM + 1);
        cv_capsule(40, 58, 56, 60, 2.6f, R_STONE, 0.1f, 0.4f);     /* legs, bound */
        cv_capsule(40, 60, 55, 62, 2.4f, R_STONE, 0.08f, 0.35f);
        cv_ellipse(30, 57.5f, 11, 4.2f, R_STONE, 0.12f, 0.5f);     /* the shroud over the chest */
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
        cv_capsule(27, 50, 22, 61, 2.6f, R_STONE, 0.1f, 0.4f);
        cv_capsule(37, 50, 42, 61, 2.6f, R_STONE, 0.1f, 0.4f);
        cv_capsule(22, 61, 14, 62, 2.0f, R_WARM, 0.3f, 0.75f);
        cv_capsule(42, 61, 50, 62, 2.0f, R_WARM, 0.3f, 0.75f);
    } else {
        for (int s = -1; s <= 1; s += 2) {                          /* legs: rags to the knee, bare shins, a dragging foot */
            float k = s * w, kx = 32 + s * 4.5f - k * 2.0f, fx = 32 + s * 5.0f - k * 3.5f;
            cv_capsule(32 + s * 3.5f, 42, kx, 52, 2.8f, R_STONE, 0.12f, 0.42f);   /* bound in linen strips */
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
            cv_px(x, y, sh(R_STONE, v - 0.02f - (((int)(y + x * 0.5f)) % 5 == 0 ? 0.12f : 0.0f)));   /* burial linen, wound round it, gone grey */
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
    /* the head (SHAMB_HEAD): sunken sockets with a sulphur light in them, the jaw dropped, hair in clumps */
    grid_stamp(SHAMB_HEAD, 13, 16, (int)(hx - 6.0f), (int)(hy - 7.0f), R_WARM, R_IRON, (uint8_t)(R_FLAME + (grab ? 15 : 13)), 0);
    for (int i = 0; i < 7; i++) {                                   /* lank hair over the crown and down the sides */
        float a = PI_F * (1.05f + 0.9f * i / 6.0f), x = hx + cosf(a) * 6.0f, y = hy - 1.0f + sinf(a) * 6.5f;
        float drop = (i == 0 || i == 6) ? 6.0f + 3.0f * h01(i, 2, 219) : 1.5f;
        cv_line(x, y, x + (x < hx ? -0.8f : 0.8f), y + drop, sh(R_IRON, 0.18f + 0.12f * h01(i, 1, 219)));
    }
    cv_rot(R_WARM, 0.6f, 211);
    for (int y = 0; y < TS; y++)                                    /* grave-dirt clotted on it, more of it lower down */
        for (int x = 0; x < TS; x++) {
            uint8_t c = cv_sprite[y * TS + x];
            if (!((c > R_WARM && c < R_WARM + 16) || (c > R_STONE && c < R_STONE + 16))) continue;
            float n = fbm(x / 3.0f, y / 3.0f, 16, 213);
            if (n > 0.74f - (float)y * 0.004f) cv_px(x, y, sh(R_WOOD, 0.06f + 0.14f * h01(x, y, 214)));
        }
    if (pose <= 1)                                                  /* roots out of the tear in its chest, from the grave */
        for (int k = 0; k < 3; k++) {
            float x = cx - 3.0f + k * 1.5f, y = sy + 8.0f + k * 2.0f, a = 2.2f + k * 0.5f;
            for (int i = 0; i < 6 + k * 2; i++) { a += (h01(k, i, 217) - 0.5f) * 0.9f; x += cosf(a) * 0.9f; y += sinf(a) * 0.9f; cv_px((int)x, (int)y, sh(R_WOOD, 0.3f + 0.15f * (i & 1))); }
        }
    if (pose == 2 || pose == 3) {                                   /* climbing out of the grave */
        cv_sink(pose == 2 ? 30 : 14, 58);
        soil(32, 17, 58, 221);
    }
    cv_outline(0);
    cv_store(id);
}

/* the shambler's dressing by rank (docs/InitialArtPass.md 6.1): II more of its linen hanging, grave-mould; III frost and
 * an ear torn away; IV an arrow in its shoulder and old blood from the tear and the mouth; V charred, an iron circlet and a
 * grave's shackle on its wrist; gilded gold leaf and gilt teeth */
static void shambler_dress(int id, float w, int grab) {
    float sway = w * 0.8f, hx = 30 + sway, hy = 13 + (grab ? 1.0f : 0.0f), sy = 23, cx = 31.0f + sway * 0.7f;
    for (int r = RK_II; r < NRANK; r++) {
        dress_begin(id);
        if (r != RK_GILDED) {
            kit_rags(24 + sway, 43, 38 + sway, 44, 9, w, R_STONE, 0.1f, 0.35f, 0, 491);
            kit_rags(22 + sway, sy, 26 + sway, sy + 1, 6, w * 0.5f, R_STONE, 0.1f, 0.35f, 0, 493);
        }
        if (r == RK_II) kit_moss(0.3f, 3, 495);
        if (r == RK_III) kit_frost(0.75f, 497);
        if (r >= RK_III && r != RK_GILDED) { cv_px((int)hx + 6, (int)hy - 2, 0); cv_px((int)hx + 6, (int)hy - 1, 0); cv_px((int)hx + 5, (int)hy - 3, 0); }   /* an ear gone */
        if (r >= RK_IV && r != RK_GILDED) { kit_arrow(38 + sway, sy + 2, -0.9f, 8.0f); kit_blood(cx - 3, sy + 12, 10, 2, 499); kit_blood(hx, hy + 6, 7, 2, 501); }
        if (r == RK_V) { kit_char(0.3f, 503); kit_crown(hx, hy - 7, 11, 505); if (!grab) kit_chain(21 + w * 2.5f, sy + 19, 19 + w * 2.0f, sy + 33, 1.5f, R_IRON); }
        if (r == RK_GILDED) {
            kit_gold(R_WARM, 0.5f, 507);
            for (int y = (int)hy + 2; y < (int)hy + 8; y++)                                   /* gilt teeth */
                for (int x = (int)hx - 5; x <= (int)hx + 5; x++) { uint8_t b = base_c(x, y); if (b >= R_WARM + 12 && b < R_WARM + 16) cv_px(x, y, (uint8_t)(R_GOLD + 11)); }
            kit_chain(24 + sway, sy + 2, 38 + sway, sy + 2, 2.5f, R_GOLD);
        }
        dress_store(id, r, R_WARM);
    }
}

/* the water a drowned one rises from, or the foul pool it leaves when it bursts */
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

/* The Drowned (docs/InitialArtPass.md section 8): the body the water made of a man, long and wrong. Waxen green-white skin
 * (R_GHOST, PF_DROWN), a gaunt, too-tall frame round a belly swollen tight and splitting at a seam; sloping shoulders, ribs,
 * thin arms hanging past the knees with long wrinkled fingers; a long face, the hair plastered flat down it, small cold
 * eyes in deep sockets, the mouth a black slack hole; a bruise round the throat; the Order's shroud sodden and dragged on
 * one side; a rope round the ankle and the stone it was weighted with; weed; it drips. pose 0-1 wading, 2 reaching for
 * you, 3 rising from its puddle, 4 bursting, 5 the foul pool it leaves. */
static const char *const DROWN_HEAD[15] = {
    "....jkkkj....", "..jklmmmlkj..", ".jk6789876kj.", ".kj79aaa97jk.", "jk7789a9877kj", "jk50@080@05kj", "jk743292347kj",
    "kj6687a7866jk", "kj566797665jk", "jk467000764kj", "jk360000063kj", "j.360000063.j", "j..4600064..j", "j..3578753..j",
    "....24642....",
};
static void spr_drowned(int id, float w, int pose) {
    cv_begin(cv_sprite, TS, TS);
    if (pose == 5) { drowned_pool(32, 20, 1, 231); cv_store(id); return; }
    if (pose == 4) {                                               /* burst: skin and shroud flung out round a cloud of gas */
        for (int i = 0; i < 170; i++) {
            float a = h01(i, 1, 233) * TAU_F, r = 4 + 21 * sqrtf(h01(i, 2, 233));
            float x = 32 + cosf(a) * r, y = 36 + sinf(a) * r * 0.8f;
            if ((i & 1) && h01(i, 3, 233) > 0.3f) cv_px((int)x, (int)y, sh(R_MOSS, 0.25f + 0.4f * h01(i, 4, 233)));
        }
        for (int i = 0; i < 10; i++) {                             /* rags of skin and the shroud */
            float a = h01(i, 5, 233) * TAU_F, r = 8 + 15 * h01(i, 6, 233);
            float x = 32 + cosf(a) * r, y = 38 + sinf(a) * r * 0.7f;
            cv_capsule(x, y, x + cosf(a) * 3.0f, y + 2.5f, 1.0f, (i & 1) ? R_GHOST : R_WARM, 0.15f, 0.55f);
        }
        cv_ellipse(32, 46, 7, 6, R_GHOST, 0.1f, 0.4f);              /* what is left, slumping */
        cv_flat(32, 45, 3.5f, 2.5f, sh(R_BLOOD, 0.12f));
        drowned_pool(32, 16, 1, 235);
        cv_outline(0); cv_store(id); return;
    }
    int grab = pose == 2;
    float sw = w * 0.8f;
    cv_flat(32, 62, 15, 2.0f, R_WARM + 1);                          /* the wet where it stands */
    cv_flat(32, 62, 10, 1.2f, sh(R_GHOST, 0.12f));
    for (int s = -1; s <= 1; s += 2) {                              /* legs: thin, knock-kneed, too long */
        float kx = 32 + s * 4.5f - s * w * 1.0f, ky = 52;
        cv_capsule(32 + s * 4.0f, 44, kx, ky, 2.0f, R_GHOST, 0.15f, 0.55f);
        cv_ellipse(kx, ky, 2.2f, 1.8f, R_GHOST, 0.2f, 0.6f);
        cv_capsule(kx, ky, 32 + s * 5.0f + s * w * 1.0f, 61, 1.7f, R_GHOST, 0.12f, 0.5f);
        cv_ellipse(32 + s * 5.5f + s * w, 61.5f, 3.0f, 1.2f, R_GHOST, 0.15f, 0.5f);
    }
    float rx = 37.0f + w, ry = 60.0f;                               /* a rope round the right ankle, to the stone it sank with */
    cv_line(rx - 1, ry - 1, rx + 2, ry - 1, sh(R_WOOD, 0.45f)); cv_line(rx - 1, ry, rx + 2, ry, sh(R_WOOD, 0.3f));
    cv_line(rx + 2, ry, 47, 61, sh(R_WOOD, 0.35f));
    cv_ellipse(49, 60.5f, 3.2f, 2.4f, R_STONE, 0.15f, 0.55f);
    cv_ellipse(32 + sw * 0.5f, 37, 9.5f, 10.0f, R_GHOST, 0.15f, 0.7f);   /* the belly, swollen tight */
    cv_ellipse(32 + sw * 0.6f, 26, 7.5f, 5.0f, R_GHOST, 0.15f, 0.6f);    /* a narrow chest over it, ribs showing */
    for (int r = 0; r < 3; r++) for (int s = -1; s <= 1; s += 2) cv_line(32 + sw * 0.6f + s * 1.5f, 24 + r * 2.2f, 32 + sw * 0.6f + s * 6.5f, 25.5f + r * 2.2f, sh(R_GHOST, 0.25f));
    for (int k = 0; k < 4; k++) {                                   /* veins on the belly, and the seam where it is splitting */
        float x = 26 + 12 * h01(k, 1, 237), y = 31 + 10 * h01(k, 2, 237);
        cv_line(x, y, x + 3 * (h01(k, 3, 237) - 0.5f), y + 4, sh(R_PURPLE, 0.2f));
    }
    cv_line(31 + sw * 0.5f, 32, 34 + sw * 0.5f, 44, sh(R_GHOST, 0.05f)); cv_line(32 + sw * 0.5f, 33, 35 + sw * 0.5f, 43, sh(R_MOSS, 0.35f));
    for (int i = 0; i < 8; i++)                                     /* barnacles, a shell or two */
        cv_px((int)(24 + 16 * h01(i, 3, 239) + sw), (int)(29 + 16 * h01(i, 4, 239)), sh(R_BONE, 0.5f + 0.35f * h01(i, 5, 239)));
    /* the Order's shroud, sodden, hung from the left shoulder across the belly and dragging on that side */
    { float sv[10] = { 22 + sw, 21, 28 + sw, 22, 27 + sw, 44, 24 + sw, 55, 19 + sw, 50 };
      cv_poly(sv, 5, R_WARM, 0.42f, 0.18f, 21, 34);
      for (int y = 46; y < 56; y++) if (h01(y, 1, 241) > 0.45f) cv_px((int)(19 + h01(y, 2, 241) * 8 + sw), y, C_CLEAR);   /* torn */
      for (int k = 0; k < 3; k++) cv_line(23 + k * 2 + sw, 23, 21 + k * 2 + sw, 50, sh(R_WARM, 0.16f)); }
    float sy = 21;
    for (int s = -1; s <= 1; s += 2) {                              /* arms: thin and too long, hanging past the knees, or up to take you */
        float sx = 32 + s * 8.0f + sw * 0.6f;
        if (grab) {
            cv_capsule(sx, sy, 32 + s * 13.0f, sy - 4, 1.8f, R_GHOST, 0.2f, 0.65f);
            cv_capsule(32 + s * 13.0f, sy - 4, 32 + s * 12.0f, sy - 14, 1.5f, R_GHOST, 0.2f, 0.6f);
            dead_hand(32 + s * 12.0f, sy - 16, s < 0 ? -2.0f : -1.1f, 0.5f, R_GHOST, 1.15f);
        } else {
            float ex = 32 + s * 11.0f + sw * 0.4f, ey = sy + 14, hx = 32 + s * 11.5f - s * w * 0.5f, hy = sy + 28;
            cv_capsule(sx, sy, ex, ey, 1.8f, R_GHOST, 0.15f, 0.6f);
            cv_capsule(ex, ey, hx, hy, 1.5f, R_GHOST, 0.15f, 0.55f);
            dead_hand(hx, hy + 2, 1.6f, 0.25f, R_GHOST, 1.15f);
            cv_px((int)hx, (int)hy + 7, sh(R_GHOST, 0.85f)); cv_px((int)hx, (int)hy + 9, sh(R_GHOST, 0.6f));   /* dripping */
        }
        cv_ellipse(sx, sy, 2.4f, 1.8f, R_GHOST, 0.2f, 0.7f);        /* sloping shoulders */
    }
    float hx0 = 26 + sw, hy0 = 4;                                   /* the head, hung forward on its neck */
    cv_capsule(32 + sw, 15, 32 + sw * 0.8f, 21, 1.8f, R_GHOST, 0.15f, 0.55f);
    for (int x = -2; x <= 2; x++) cv_px((int)(32 + sw * 0.8f) + x, 18, sh(R_PURPLE, 0.18f));   /* the bruise where something held it */
    for (int i = 0; i < 6; i++) {                                   /* weed hung over the shoulders */
        float x = 24 + i * 3.2f + sw, len = 5 + 7 * h01(i, 1, 243);
        for (int y = 0; y < (int)len; y++) cv_px((int)(x + sinf(y * 0.6f + i + w) * 0.7f), 20 + y, sh(R_MOSS, 0.12f + 0.25f * (1.0f - y / len)));
    }
    grid_stamp(DROWN_HEAD, 13, 15, (int)hx0, (int)hy0, R_GHOST, R_IRON, (uint8_t)(R_GLOW + 14), 0);
    cv_px((int)hx0 + 6, (int)hy0 + 15, sh(R_GHOST, 0.75f)); cv_px((int)hx0 + 6, (int)hy0 + 17, sh(R_GHOST, 0.5f));   /* water from its mouth */
    cv_px((int)hx0, (int)hy0 + 15, sh(R_GHOST, 0.6f)); cv_px((int)hx0 + 12, (int)hy0 + 16, sh(R_GHOST, 0.6f));      /* and its hair */
    cv_rot(R_GHOST, 0.2f, 236);
    if (pose == 3) { cv_sink(18, 58); drowned_pool(32, 22, 0, 245); }
    cv_outline(0);
    cv_store(id);
}

/* the Drowned's dressing by rank: II weed and mould, the shroud more torn; III frost in its hair and on its shoulders, the
 * seam split wider; IV bleeding from its seams and its mouth; V black, a thing of the deep, a chain from its ankle's
 * stone to its wrist and a crown of rusted iron; gilded the Order's gold, gone green in the water */
static void drowned_dress(int id, int grab) {
    for (int r = RK_II; r < NRANK; r++) {
        dress_begin(id);
        if (r != RK_GILDED) kit_rags(22, 48, 30, 50, 8, 0.0f, R_WARM, 0.08f, 0.25f, 0, 471);   /* more of the shroud, torn */
        if (r == RK_II) kit_moss(0.32f, 2, 473);
        if (r == RK_III) kit_frost(0.8f, 475);
        if (r >= RK_III && r != RK_GILDED) { cv_line(31, 32, 35, 45, 0); cv_line(32, 33, 36, 44, sh(R_MOSS, 0.4f)); }   /* the seam split */
        if (r >= RK_IV && r != RK_GILDED) { kit_blood(33, 34, 12, 2, 477); kit_blood(32, 16, 6, 1, 479); }
        if (r == RK_V) { kit_char(0.3f, 481); kit_crown(32, 5, 11, 483); if (!grab) kit_chain(48, 59, 44, 50, -3.0f, R_IRON); }
        if (r == RK_GILDED) {
            kit_gold(R_GHOST, 0.5f, 485);
            for (int y = 0; y < TS; y++) for (int x = 0; x < TS; x++) if (cv_sprite[y * TS + x] >= R_GOLD && cv_sprite[y * TS + x] < R_GOLD + NGOLD && h01(x, y, 487) < 0.3f) cv_px(x, y, sh(R_MOSS, 0.55f));   /* verdigris */
            kit_chain(26, 22, 38, 22, 2.0f, R_GOLD);
        }
        dress_store(id, r, R_GHOST);
    }
}


/* The ghoul (docs/InitialArtPass.md section 8): starved, low, on all fours, skin tight over a beaded spine and ribs, long
 * thin arms with long claws; a long-muzzled face (GHOUL_HEAD: close-set eyes deep in their sockets, slit nostrils at the
 * muzzle's tip, lips peeled back from fangs, the jaw hanging open), long ears swept up and torn. Skin R_WARM (PF_FLESH).
 * pose: 0-1 loping, 2 leaping, 3 feeding on bones, 4 dead. */
static const char *const GHOUL_HEAD[15] = {
    "....4678764....", "...59abcba95...", "..48abdddba74..", ".36a900c009a63.", ".359@0aca0@953.", "..48900a00984..",
    "..37abcdcba73..", "...59bcecb95...", "...4a0dcd0a4...", "..2u2u0u0u2u2..", "..2u1000001u2..", "..3s0000000s3..",
    "...3s0s0s0s3...", "....3589853....", ".....36663.....",
};
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
        cv_capsule(32 + s * 6, 34 + lift, kx, ky, 2.0f, R_WARM, 0.15f, 0.55f);
        cv_ellipse(kx, ky, 2.2f, 2.0f, R_WARM, 0.2f, 0.6f);                       /* a knobbed knee */
        cv_capsule(kx, ky, 32 + s * 9 + (pose == 2 ? s * 6 : 0), 58 + lift * 0.3f, 1.5f, R_WARM, 0.15f, 0.5f);
    }
    /* the back, arched high, with the spine standing out */
    cv_ellipse(32, 33 + lift + feed * 4, 9.5f, 7.0f, R_WARM, 0.2f, 0.8f);   /* the back, arched, starved */
    for (int i = 0; i < 6; i++) cv_ellipse(32, 25.5f + lift + feed * 4 + i * 2.2f, 1.6f - i * 0.12f, 1.0f, R_WARM, 0.55f, 1.0f);   /* the spine, beaded */
    for (int s2 = -1; s2 <= 1; s2 += 2) {                           /* shoulder blades jutting */
        cv_ellipse(32 + s2 * 6.0f, 28.5f + lift + feed * 4, 3.2f, 2.0f, R_WARM, 0.35f, 0.95f);
        cv_line(32 + s2 * 3.5f, 27 + lift + feed * 4, 32 + s2 * 8.5f, 30 + lift + feed * 4, sh(R_WARM, 0.1f));
    }
    for (int i = 0; i < 5; i++) {                                   /* ribs down the flanks, the skin sunk between them */
        float y = 30 + lift + feed * 4 + i * 2.0f;
        cv_line(24, y, 28, y + 1.5f, sh(R_WARM, 0.08f)); cv_line(40, y, 36, y + 1.5f, sh(R_WARM, 0.08f));
        cv_line(24, y + 1, 27, y + 2.2f, sh(R_WARM, 0.55f)); cv_line(40, y + 1, 37, y + 2.2f, sh(R_WARM, 0.55f));
    }
    /* front arms: long, bony, planted wide, or flung out in the leap */
    for (int s = -1; s <= 1; s += 2) {
        float ex = 32 + s * 15, ey = 38 + lift, fx, fy;
        if (pose == 2) { ex = 32 + s * 17; ey = 26 + lift; fx = 32 + s * 24; fy = 18 + lift; }
        else { float step = s * w * 3.0f; fx = 32 + s * 17 + step * 0.3f; fy = 60 - fmaxf(0.0f, step) * 1.5f; }
        cv_capsule(32 + s * 8, 32 + lift + feed * 4, ex, ey, 1.8f, R_WARM, 0.25f, 0.8f);
        cv_ellipse(ex, ey, 2.1f, 2.0f, R_WARM, 0.3f, 0.85f);                     /* a knobbed elbow */
        cv_capsule(ex, ey, fx, fy, 1.5f, R_WARM, 0.22f, 0.75f);
        for (int c = -1; c <= 1; c++) {                             /* claws, long and hooked */
            float ca = (pose == 2 ? (s < 0 ? -2.3f : -0.8f) : 1.57f + s * 0.3f) + c * 0.35f;
            cv_line(fx, fy, fx + cosf(ca) * 6.0f, fy + sinf(ca) * 6.0f, sh(R_BONE, 0.7f));
            cv_px((int)(fx + cosf(ca) * 6.0f + s), (int)(fy + sinf(ca) * 6.0f), sh(R_BONE, 0.5f));
        }
    }
    /* the head: long ears swept up and torn, then the face (GHOUL_HEAD) */
    for (int s = -1; s <= 1; s += 2) {
        float e[6] = { hx + s * 3.0f, hy - 6, hx + s * 16, hy - 15, hx + s * 8.5f, hy + 1.0f };   /* broad, leathery */
        cv_poly(e, 3, R_WARM, 0.25f, 0.75f, hy - 15, 16);
        float in[6] = { hx + s * 5.0f, hy - 4.5f, hx + s * 13.5f, hy - 12.5f, hx + s * 8.0f, hy - 1.0f };
        cv_poly(in, 3, R_WARM, 0.06f, 0.16f, hy - 12.5f, 11);
        cv_px((int)(hx + s * 12.0f), (int)(hy - 9), C_CLEAR); cv_px((int)(hx + s * 12.5f), (int)(hy - 8), C_CLEAR);   /* a tear in each */
    }
    grid_stamp(GHOUL_HEAD, 15, 15, (int)(hx - 7.0f), (int)(hy - 7.0f), R_WARM, R_BONE, (uint8_t)(R_FLAME + 13), 0);
    if (pose == 3) { cv_line(hx - 1, hy + 4, hx + 1, hy + 11, sh(R_BLOOD, 0.3f)); cv_line(hx + 2, hy + 4, hx + 3, hy + 9, sh(R_BLOOD, 0.22f)); }   /* feeding */
    cv_rot(R_WARM, 0.3f, 253);
    cv_outline(0);
    cv_store(id);
}

/* the ghoul's dressing by rank: II loose skin hanging and mould, III frost and an ear torn through, IV a gash down its
 * back and its claws and mouth bloodied, V charred in a chained beast's collar, gilded gold leaf and gilt fangs */
static void ghoul_dress(int id, float lift, float feed) {
    float hx = 32, hy = 40 + lift + feed * 10.0f;
    for (int r = RK_II; r < NRANK; r++) {
        dress_begin(id);
        if (r != RK_GILDED) { kit_rags(25, 36 + lift + feed * 4, 30, 38 + lift + feed * 4, 6, 0.0f, R_WARM, 0.1f, 0.3f, 0, 511); kit_rags(35, 37 + lift + feed * 4, 39, 36 + lift + feed * 4, 5, 0.0f, R_WARM, 0.1f, 0.3f, 0, 513); }
        if (r == RK_II) kit_moss(0.32f, 2, 515);
        if (r == RK_III) kit_frost(0.75f, 517);
        if (r >= RK_III && r != RK_GILDED) { cv_line(hx + 13, hy - 13, hx + 15, hy - 10, 0); cv_line(hx + 14, hy - 13, hx + 16, hy - 11, 0); }   /* an ear torn through */
        if (r >= RK_IV && r != RK_GILDED) {
            cv_line(27, 27 + lift + feed * 4, 36, 36 + lift + feed * 4, sh(R_BLOOD, 0.15f)); cv_line(28, 27 + lift + feed * 4, 37, 36 + lift + feed * 4, sh(R_BLOOD, 0.35f));   /* a gash down its back */
            kit_blood(hx, hy + 6, 6, 3, 519);
            for (int y = 0; y < TS; y++) for (int x = 0; x < TS; x++) { uint8_t b = base_c(x, y); if (b >= R_BONE && b < R_BONE + 16 && y > hy + 8 && h01(x, y, 521) < 0.5f) cv_px(x, y, sh(R_BLOOD, 0.3f)); }   /* bloodied claws */
        }
        if (r == RK_V) { kit_char(0.3f, 523); for (int x = (int)hx - 7; x <= (int)hx + 7; x++) { cv_px(x, (int)hy + 8, sh(R_IRON, 0.55f)); cv_px(x, (int)hy + 9, sh(R_IRON, 0.25f)); } kit_chain(hx + 6, hy + 9, hx + 14, hy + 20, 2.0f, R_IRON); }
        if (r == RK_GILDED) {
            kit_gold(R_WARM, 0.5f, 525);
            for (int y = 0; y < TS; y++) for (int x = 0; x < TS; x++) { uint8_t b = base_c(x, y); if (b >= R_BONE + 11 && b < R_BONE + 16 && y < hy + 8 && y > hy) cv_px(x, y, (uint8_t)(R_GOLD + 11)); }   /* gilt fangs */
        }
        dress_store(id, r, R_WARM);
    }
}

/* grime and moss on bones, deeper down, as on the skeletons */
static void cv_bone_decay(uint32_t seed) {
    if (g_decay <= 0.0f) return;
    for (int y = 0; y < TS; y++)
        for (int x = 0; x < TS; x++) {
            uint8_t c = cv_sprite[y * TS + x];
            if (c < R_BONE || c >= R_BONE + 16) continue;
            float n = fbm(x / 5.0f, y / 5.0f, 16, seed);
            if (x >= g_skbox[0] && x < g_skbox[2] && y >= g_skbox[1] && y < g_skbox[3]) {   /* a hand-drawn skull keeps its face */
                if (n < 0.2f + 0.25f * g_decay && c > R_BONE + 4) cv_sprite[y * TS + x] = (uint8_t)(c - 1);
                continue;
            }
            if (n < 0.28f + 0.32f * g_decay) cv_sprite[y * TS + x] = (uint8_t)(c - 3 < R_BONE + 1 ? R_BONE + 1 : c - 3);
            if (g_decay > 0.35f && n > 0.78f - 0.12f * g_decay) cv_sprite[y * TS + x] = sh(R_MOSS, 0.3f + 0.3f * n);
        }
}

/* The bone hound: a dog's skeleton in a spiked collar, a broken chain hung from it, coming at you a little from the side.
 * pose: 0-1 running, 2 biting, 3 a heap. All bone (PF_BONE). */
/* the dog's skull, by hand (docs/InitialArtPass.md section 8), facing left: a long muzzle, the nasal opening at its tip,
 * a deep orbit, the cheekbone's arch, a canine hanging; the lower jaw apart, hinged under the orbit, its front dropping
 * by jaw pixels. x, y as before: the eye a little right of x, the snout's tip nine to the left. */
static const char *const DOG_UPPER[8] = {
    "...........68a986", ".........59bccdb8", ".....3679abcddcb7", "..4689abcdb00@0b8", ".0789aab9a9800ba7",
    "0689a9887ab9cba96", ".4e5e7e67789a9864", "...f.............",
};
static const char *const DOG_JAW[4] = { ".e5e5e5e6789a9.", ".46789aa9a9a985", "..3567788898642", "....23445555432" };
static void dog_skull_at(int x0, int y0, float jaw, int eye, int mirror) {   /* top left at x0, y0; mirror: facing right */
    #define DX(c) (mirror ? x0 + 16 - (c) : x0 + (c))
    if (jaw > 1.5f)                                                 /* the gape, dark behind the teeth */
        for (int c = 1; c < 13; c++) {
            int d = (int)(jaw * (1.0f - (float)c / 14.0f));
            for (int k = 0; k < d; k++) cv_px(DX(c), y0 + 7 + k, 0);
        }
    for (int r = 0; r < 4; r++)                                     /* the jaw, its front dropped further than its back */
        for (int c = 0; c < 15; c++) {
            char ch = DOG_JAW[r][c];
            if (ch == '.') continue;
            int d = (int)(jaw * (1.0f - (float)c / 14.0f) + 0.5f);
            cv_px(DX(1 + c), y0 + 7 + r + d, (uint8_t)(R_BONE + (ch <= '9' ? ch - '0' : ch - 'a' + 10)));
        }
    grid_stamp(DOG_UPPER, 17, 8, x0, y0, R_BONE, R_BONE, eye ? (uint8_t)(R_FLAME + 11) : 0, mirror);
    if (!eye) cv_px(DX(13), y0 + 3, 0);                            /* a dead one's socket, empty */
    #undef DX
}
static void dog_skull(float x, float y, float jaw, int eye) { dog_skull_at((int)x - 9, (int)y - 5, jaw, eye, 0); }

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
    for (int k = 0; k < 4; k++)                                     /* a chain from the collar, broken off */
        cv_px(24 + k / 2, (int)(hy + 8 + k * 1.6f), sh(R_IRON, (k & 1) ? 0.65f : 0.3f)), cv_px(25 + k / 2, (int)(hy + 9 + k * 1.6f), sh(R_IRON, 0.2f));
    dog_skull(hx, hy, bite ? 7.0f : 1.0f + (w > 0 ? 0.6f : 0.0f), 1);
    cv_bone_decay(305);
    cv_outline(0);
    cv_store(id);
}

/* the bone hound's dressing by rank: II a strip of hide from its collar and moss in its bones, III frost and its skull
 * cracked, IV an arrow in its ribs and old blood about its jaws, V charred under a spiked iron muzzle-strap and its
 * master's chain, gilded a gold collar and gold leaf */
static void hound_dress(int id, float w, int pose) {
    float bob = (pose == 0 ? 1.0f : 0.0f) * w, hy = 26 + bob + (pose == 2 ? 6.0f : 0.0f);
    for (int r = RK_II; r < NRANK; r++) {
        dress_begin(id);
        if (r != RK_GILDED) kit_rags(21, hy + 6, 26, hy + 7, r == RK_II ? 6.0f : 8.0f, w * 0.5f, R_WOOD, 0.1f, 0.35f, 0, 531);
        if (r == RK_II) kit_moss(0.3f, 2, 533);
        if (r == RK_III) kit_frost(0.8f, 535);
        if (r >= RK_III && r != RK_GILDED) { cv_line(21, hy - 4, 23, hy, 0); cv_px(24, (int)hy - 1, 0); }   /* the skull cracked */
        if (r >= RK_IV && r != RK_GILDED) { kit_arrow(33, 33 + bob, -1.2f, 8.0f); kit_blood(12, hy + 2, 6, 4, 537); kit_blood(33, 34 + bob, 7, 1, 539); }
        if (r == RK_V) { kit_char(0.3f, 541); cv_line(10, hy, 18, hy - 3, sh(R_IRON, 0.55f)); cv_line(10, hy + 1, 18, hy - 2, sh(R_IRON, 0.25f)); kit_chain(24, hy + 8, 30, hy + 22, 3.0f, R_IRON); }
        if (r == RK_GILDED) { kit_gold(R_BONE, 0.5f, 543); for (int a = 0; a < 20; a++) { float ang = TAU_F * a / 20.0f, sz = sinf(ang); if (sz >= -0.3f) cv_px((int)(23 + 4.5f * cosf(ang)), (int)(hy + 5 + 2.0f * sz), (uint8_t)(R_GOLD + 8 + (int)(3.0f * sz))); } }
        dress_store(id, r, R_BONE);
    }
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
/* the great helm, by hand (docs/InitialArtPass.md section 8): a ridged crest and a plume's stump (R_BLOOD), rivets, a
 * slit in a cross with a crimson light in it, breath-holes below; hex R_IRON (PF_ARMOUR) */
static const char *const KNIGHT_HELM[16] = {
    "......mpm......", ".....mrqrm.....", "....59bdb95....", "...69bcecb96...", "..68abcecba86..", ".578abbdbba875.",
    ".6fa7bbdbb7af6.", ".6f0@00c00@0f6.", ".589a9a0a9a985.", ".57898a0a89875.", ".5680908090865.", ".5679a9a9a9765.",
    ".4680908090864.", ".3f6789a9876f3.", "..24567876542..", "...234565432...",
};
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
    if (state < 2) {                                                /* the Order's tabard below the plate, its cross on it; torn once hacked */
        for (int y = 41; y < 55; y++) {
            float hw = 5.0f + (y - 41) * 0.15f;
            for (int x = (int)(32 - hw); x <= (int)(32 + hw); x++) {
                if (y > 51 && h01(x, y, 315) < (y - 51) / 4.0f) continue;   /* the hem frayed */
                if (state == 1 && x > 32 - (y - 41) * 0.5f + 4) continue;   /* hacked away on the one side */
                float fold = sinf((x - 32) * 1.3f) * 0.06f;
                cv_px(x, y, sh(R_BLOOD, 0.22f + fold - (y - 41) * 0.006f));
            }
        }
        for (int k = -2; k <= 2; k++) { cv_px(32 + k, 46, sh(R_BONE, 0.75f)); cv_px(32, 46 + k, sh(R_BONE, 0.75f)); }   /* the cross */
        cv_px(30, 44, sh(R_BONE, 0.6f)); cv_px(34, 44, sh(R_BONE, 0.6f)); cv_px(30, 48, sh(R_BONE, 0.6f)); cv_px(34, 48, sh(R_BONE, 0.6f));
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
    for (int k = 0; k < 4; k++) { cv_px(24, 21 + k * 4, (uint8_t)(R_IRON + 14)); if (state < 2) cv_px(40, 21 + k * 4, (uint8_t)(R_IRON + 13)); }   /* rivets down the plate's edges */
    /* arms: both hands on the grip */
    for (int s = -1; s <= 1; s += 2) {
        float sx = 32 + s * 9.0f, sy = 20;
        float ex = (sx + gx) * 0.5f + s * 4.0f, ey = (sy + gy) * 0.5f + 4.0f;
        if (pose == 2) { ex = (sx + gx) * 0.5f + s * 3.0f; ey = (sy + gy) * 0.5f + 3.0f; }
        cv_bone(sx, sy, ex, ey, 1.3f, 0.25f, 0.9f);
        cv_bone(ex, ey, gx + s * 1.0f, gy, 1.1f, 0.25f, 0.9f);
        int keep = state == 0 || (state == 1 && s > 0) || (state == 2 && s > 0);
        if (keep) { cv_ellipse(sx, sy - 0.5f, 5.0f, 4.0f, R_IRON, 0.3f, 1.0f); cv_px((int)sx - 2, (int)sy - 3, (uint8_t)(R_IRON + 15)); cv_px((int)sx + 2, (int)sy - 3, (uint8_t)(R_IRON + 15)); cv_px((int)sx, (int)sy + 2, (uint8_t)(R_IRON + 14)); }   /* pauldron, riveted */
        if (state < 2) plate(ex, ey, gx + s * 1.0f, gy, 2.0f, 0.05f);          /* vambrace */
    }
    if (pose != 2) sword(gx, gy, ga);
    for (int s = -1; s <= 1; s += 2) cv_ellipse(gx + s * 1.2f, gy + 0.5f, 2.2f, 2.0f, state < 2 ? R_IRON : R_BONE, 0.3f, 0.9f);   /* gauntlets, or bare bone */
    /* the head: a great helm with a slit, and two embers in it; or the bare skull once the helm is off */
    cv_capsule(32, 15, 32, 18, 1.2f, R_BONE, 0.3f, 0.9f);
    if (state == 0) grid_stamp(KNIGHT_HELM, 15, 16, 25, 1, R_IRON, R_BLOOD, (uint8_t)(R_FLAME + 9), 0);   /* the great helm */
    else {                                                          /* the helm hacked off: the skull under it */
        skull_stamp(32, 10, 0, 0, (uint8_t)(R_FLAME + 9));
        if (state == 1) for (int x = 25; x <= 39; x++) { cv_px(x, 2 + (x - 32) * (x - 32) / 12, sh(R_IRON, 0.5f)); cv_px(x, 3 + (x - 32) * (x - 32) / 12, sh(R_IRON, 0.25f)); }   /* the helm's rim, all that is left */
    }
    cv_bone_decay(313);
    cv_outline(0);
    cv_store(id);
}

/* the knight's dressing by rank: II rust and moss on the plate, III frost on it and dents, IV a broken blade in the
 * breastplate and an arrow in the pauldron, old blood from them, V charred plate under an iron crown and a chain,
 * gilded gilt on the plate's edges and rivets (docs/InitialArtPass.md 6.2) */
static void knight_dress(int id, int state) {
    for (int r = RK_II; r < NRANK; r++) {
        dress_begin(id);
        if (r == RK_II) {
            kit_moss(0.28f, 0, 551);
            for (int y = 0; y < TS; y++) for (int x = 0; x < TS; x++) { uint8_t b = base_c(x, y); if (b > R_IRON && b < R_IRON + 16 && fbm(x / 3.0f, y / 3.0f, 16, 553) > 0.7f) cv_px(x, y, sh(R_BRICK, 0.3f + 0.2f * h01(x, y, 555))); }   /* rust */
        }
        if (r == RK_III) kit_frost(0.8f, 557);
        if (r >= RK_III && r != RK_GILDED) for (int k = 0; k < 5; k++) cv_px(26 + (int)(h01(k, 1, 559) * 12.0f), 20 + (int)(h01(k, 2, 559) * 14.0f), sh(R_IRON, 0.05f));   /* dents */
        if (r >= RK_IV && r != RK_GILDED) {
            if (state < 2) { cv_line(35, 24, 41, 18, sh(R_GHOST, 0.7f)); cv_line(36, 25, 42, 19, sh(R_GHOST, 0.45f)); cv_line(41, 18, 44, 15, sh(R_WOOD, 0.4f)); kit_blood(35, 26, 9, 2, 561); }   /* a broken blade lodged */
            kit_arrow(22, 18, -2.3f, 7.0f);
        }
        if (r == RK_V) { kit_char(0.3f, 563); kit_crown(32, state == 0 ? 3 : 2, 13, 565); kit_chain(23, 22, 20, 36, 1.5f, R_IRON); }
        if (r == RK_GILDED)
            for (int y = 0; y < TS; y++) for (int x = 0; x < TS; x++) {
                uint8_t b = base_c(x, y);
                if (b > R_IRON && b < R_IRON + 16 && (b - R_IRON >= 12 || (!base_on(x - 1, y) || !base_on(x + 1, y) || !base_on(x, y - 1)))) cv_px(x, y, (uint8_t)(R_GOLD + clampi(b - R_IRON - 2, 5, NGOLD - 1)));   /* gilt edges */
            }
        dress_store(id, r, R_IRON);
    }
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
        grid_stamp(SKULL_SMALL, 9, 10, 26, 45, R_BONE, R_BONE, 0, 0);
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
    grid_stamp(SKULL_SMALL, 9, 10, (int)stx - 4, (int)sty - 2, R_BONE, R_BONE, 0, 0);   /* the staff's skull, its eyes its own (not the deacon's) */
    cv_px((int)stx - 2, (int)sty + 1, (uint8_t)(chant ? R_GLOW + 14 : R_FLAME + 6)); cv_px((int)stx + 2, (int)sty + 1, (uint8_t)(chant ? R_GLOW + 14 : R_FLAME + 6));
    /* the robe, falling from the shoulders to a hem that drags */
    for (int y = 16; y < 63; y++) {
        float t = (y - 16) / 46.0f, hw = 6.5f + 6.0f * t, sway = w * 1.2f * t;   /* taller and thinner */
        for (int x = (int)(32 - hw + sway) - 1; x <= (int)(32 + hw + sway) + 1; x++) {
            float dx = x + 0.5f - 32 - sway;
            if (fabsf(dx) > hw) continue;
            if (y > 58 && h01(x, y, 335 + (w > 0)) < (y - 58) / 5.0f) continue;   /* ragged hem */
            float u = dx / hw, v = 0.15f + 0.4f * (1.0f - u * u) * (1.0f - 0.3f * t);
            if (sinf(dx * 1.7f + y * 0.02f) > 0.55f) v -= 0.12f;    /* long folds, falling straight */
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
        float sx = 32 + s * 7, sy = 20, hx, hy;
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
    if (!chant)                                                     /* a rosary of knuckle-bones on its cord, a small cross at its end */
        for (int k = 0; k < 9; k++) {
            float x = 25.0f - w * 0.5f + sinf((float)k * 0.5f) * 1.5f, y = 33.0f + (float)k * 1.5f;
            cv_px((int)x, (int)y, (k & 1) ? sh(R_BONE, 0.85f) : sh(R_BONE, 0.55f));
            if (k == 8) { cv_px((int)x, (int)y + 1, sh(R_BONE, 0.8f)); cv_px((int)x, (int)y + 2, sh(R_BONE, 0.7f)); cv_px((int)x - 1, (int)y + 1, sh(R_BONE, 0.7f)); cv_px((int)x + 1, (int)y + 1, sh(R_BONE, 0.7f)); }
        }
    /* the hood, and the skull in its shadow */
    for (int y = 0; y <= 21; y++)                                   /* the hood, peaked in a lancet, its rim catching the light */
        for (int x = 20; x <= 44; x++) {
            float dx = x + 0.5f - 32, hw = y < 13 ? 8.5f * sqrtf(fmaxf(0.0f, (float)y / 13.0f)) : 8.5f + (y - 13) * 0.25f;
            if (fabsf(dx) > hw) continue;
            float u = dx / (hw + 0.01f), v = 0.12f + 0.32f * (1.0f - u * u) - (u > 0.0f ? 0.06f : 0.0f);
            if (fabsf(u) > 0.82f) v += 0.18f;
            cv_px(x, y, sh(R_PURPLE, v));
        }
    for (int y = 6; y <= 19; y++)                                   /* its mouth, deep */
        for (int x = 26; x <= 38; x++) { float nx = (x + 0.5f - 32) / 5.2f, ny = (y + 0.5f - 13.0f) / 7.0f; if (nx * nx + ny * ny <= 1.0f) cv_px(x, y, 0); }
    grid_stamp(SKULL_SMALL, 9, 10, 28, 9, R_BONE, R_BONE, (uint8_t)(chant ? R_GLOW + 14 : C_WHITE), 0);   /* the skull, a pale gleam in there, two lights */
    for (int y = 9; y < 19; y++) for (int x = 28; x < 37; x++) { uint8_t c = cv_sprite[y * TS + x]; if (c > R_BONE + 2 && c < R_BONE + 16) cv_px(x, y, (uint8_t)(c - 2)); }   /* in shadow */
    cv_bone_decay(339);
    cv_outline(0);
    cv_store(id);
}

/* the deacon's dressing by rank: II mildew on the robe, its hem more torn, III frost, IV blood soaked up the hem and the
 * stole, V the robe charred and a black chain of office, gilded gold thread at the robe's edges (the stole, not being the robe,
 * keeps its red: the layer stays off what is not the body) */
static void deacon_dress(int id) {
    for (int r = RK_II; r < NRANK; r++) {
        dress_begin(id);
        if (r == RK_II) kit_moss(0.3f, 0, 571);
        if (r != RK_GILDED) for (int x = 18; x < 47; x++) if (base_on(x, 60) && h01(x, r, 573) < 0.35f) for (int y = 56 + (int)(h01(x, 1, 573) * 4.0f); y < 64; y++) if (base_on(x, y)) cv_px(x, y, 0);   /* the hem torn */
        if (r == RK_III) kit_frost(0.8f, 575);
        if (r >= RK_IV && r != RK_GILDED)
            for (int y = 48; y < 64; y++) for (int x = 16; x < 48; x++) { uint8_t b = base_c(x, y); if (b > R_PURPLE && b < R_PURPLE + 16 && fbm(x / 3.0f, y / 2.0f, 16, 577) > 0.62f - (y - 48) * 0.012f) cv_px(x, y, sh(R_BLOOD, 0.12f + 0.12f * h01(x, y, 579))); }   /* blood soaked up the hem */
        if (r == RK_V) { kit_char(0.3f, 581); kit_chain(24, 21, 40, 21, 4.0f, R_IRON); }
        if (r == RK_GILDED) {
            for (int y = 0; y < TS; y++) for (int x = 0; x < TS; x++) {
                uint8_t b = base_c(x, y);
                if (b > R_PURPLE && b < R_PURPLE + 16 && (!base_on(x - 1, y) || !base_on(x + 1, y))) cv_px(x, y, (uint8_t)(R_GOLD + 8 + (int)(h01(x, y, 583) * 3.0f)));   /* gold thread at the edges */
            }
        }
        dress_store(id, r, R_PURPLE);
    }
}

/* a crown of iron spikes on a band round a brow at (cx, y), hw wide either side; broken: a spike or two snapped */
static void iron_crown(float cx, float y, float hw, int broken) {
    for (int x = (int)(cx - hw); x <= (int)(cx + hw); x++) {
        float u = (x + 0.5f - cx) / hw, by = y + u * u * 1.5f;      /* the band, curving back round the head */
        cv_px(x, (int)by, sh(R_IRON, 0.62f - 0.3f * u)); cv_px(x, (int)by + 1, sh(R_IRON, 0.3f - 0.12f * u));
    }
    for (int k = 0; k < 7; k++) {
        float u = (k - 3) / 3.0f, x = cx + u * (hw - 1.0f), by = y + u * u * 1.5f;
        int h = (int)(4.5f - fabsf(u) * 2.0f) - (broken && (k == 1 || k == 5) ? 2 : 0);
        for (int j = 1; j <= h; j++) cv_px((int)x, (int)by - j, sh(R_IRON, 0.8f - 0.08f * j - 0.2f * (u > 0)));
        if (h > 0) cv_px((int)x, (int)by - h - 1, sh(R_IRON, 0.95f));   /* its point catching the light */
    }
}

/* the greatsword: a long blade with a fuller and a bright edge, a broad guard, a ruby in the pommel */
static void greatsword(float gx, float gy, float a) {
    float dx = cosf(a), dy = sinf(a), px = -dy, py = dx;
    cv_ellipse(gx - dx * 7.0f, gy - dy * 7.0f, 2.2f, 2.2f, R_IRON, 0.4f, 1.0f);   /* pommel, and its ruby */
    cv_px((int)(gx - dx * 7.0f), (int)(gy - dy * 7.0f), (uint8_t)(R_BLOOD + 13)); cv_px((int)(gx - dx * 7.0f - px), (int)(gy - dy * 7.0f - py), (uint8_t)(R_BLOOD + 10));
    cv_capsule(gx - dx * 5.0f, gy - dy * 5.0f, gx + dx * 3.0f, gy + dy * 3.0f, 1.3f, R_WOOD, 0.2f, 0.55f);   /* the grip, bound */
    for (int k = -2; k <= 2; k++) cv_px((int)(gx + dx * k * 1.6f), (int)(gy + dy * k * 1.6f), sh(R_WOOD, 0.75f));
    cv_capsule(gx + dx * 3.6f - px * 8.0f, gy + dy * 3.6f - py * 8.0f, gx + dx * 3.6f + px * 8.0f, gy + dy * 3.6f + py * 8.0f, 1.2f, R_IRON, 0.35f, 0.95f);
    cv_ellipse(gx + dx * 3.6f - px * 8.0f, gy + dy * 3.6f - py * 8.0f, 1.6f, 1.6f, R_IRON, 0.5f, 1.0f);   /* the guard's ends */
    cv_ellipse(gx + dx * 3.6f + px * 8.0f, gy + dy * 3.6f + py * 8.0f, 1.6f, 1.6f, R_IRON, 0.5f, 1.0f);
    for (int i = 5; i < 50; i++) {                                  /* the blade */
        float t = (float)i, wdt = i > 43 ? (50 - i) * 0.33f : 2.2f;
        for (float o = -wdt; o <= wdt; o += 0.5f) {
            float v = 0.55f + 0.4f * (o / (wdt + 0.01f)) * -0.5f + (fabsf(o) < 0.5f ? -0.18f : 0.0f) + (o > wdt - 0.6f ? 0.35f : 0.0f);
            cv_px((int)(gx + dx * t + px * o), (int)(gy + dy * t + py * o), sh(R_GHOST, v));
        }
    }
}

/* The Ill-Fated Knight's own (docs/InitialArtPass.md, part three; the owner chose a fallen paladin): the Order's
 * champion gone to bone in its plate. A tall crested great helm under a crown of iron spikes, a torn crimson plume
 * streaming from the crest; the Order's cross inlaid on the breastplate, its edges once gilt; chains wound round the
 * plate; a ragged crimson cloak behind and a long surcoat below, split and torn; a greatsword with a ruby in its pommel.
 * Shedding plate bares the skeleton, and the helm comes off last, the crown still on the skull and a light in its
 * sockets. pose 0-1 walking, 2 the wind-up, 3 the blow, 4 planted; state 0 full plate, 1 half, 2 the helm gone. */
static void spr_bknight(int id, float w, int pose, int state) {
    cv_begin(cv_sprite, TS, TS);
    float gx, gy, ga;
    if (pose == 2) { gx = 41; gy = 11; ga = -2.9f; }
    else if (pose == 3) { gx = 27; gy = 40; ga = 2.55f; }
    else if (pose == 4) { gx = 32; gy = 25; ga = 1.5708f; }
    else { gx = 37; gy = 34; ga = -1.05f; }
    cv_flat(32, 62, 18, 2.2f, R_WARM + 1);
    for (int y = 17; y < 61; y++) {                                  /* the cloak, behind it all, ragged */
        float t = (y - 17) / 43.0f, hw = 10.5f + 6.0f * t;
        for (int x = (int)(32 - hw); x <= (int)(32 + hw); x++) {
            float dx = x + 0.5f - 32.0f;
            float hem = 50.0f + 6.0f * h01((int)(dx + 40) / 2, 1, 601) - (state == 2 ? 8.0f : 0.0f);
            if (y > hem) continue;
            float v = 0.1f + 0.14f * (1.0f - fabsf(dx) / hw) + (sinf(dx * 0.9f + y * 0.05f) > 0.7f ? -0.06f : 0.0f);
            cv_px(x, y, sh(R_BLOOD, v));
        }
    }
    if (pose == 2) greatsword(gx, gy, ga);
    for (int s = -1; s <= 1; s += 2) {                               /* legs: bone, and plate unless hacked away */
        float k = s * w, hx = 32 + s * 4.0f, kx = 32 + s * 5.0f - k * 2.5f, fx = 32 + s * 5.5f - k * 4.0f;
        if (pose == 3 || pose == 4) { kx += s * 1.5f; fx += s * 3.0f; }
        cv_bone(hx, 41, kx, 51, 1.6f, 0.25f, 0.95f);
        cv_bone(kx, 51, fx, 60, 1.4f, 0.25f, 0.9f);
        if (state < 1 || (state < 2 && s > 0)) plate(hx, 42, kx, 50, 3.1f, 0.0f);
        if (state < 2) { plate(kx, 52, fx, 59, 2.7f, 0.0f); cv_ellipse(kx, 51, 2.8f, 2.4f, R_IRON, 0.4f, 1.0f); cv_px((int)kx, 50, (uint8_t)(R_GOLD + 6)); }   /* poleyn, gilt */
        if (state < 2) cv_ellipse(fx - s * 1.0f, 61.5f, 3.8f, 1.6f, R_IRON, 0.3f, 0.9f);
        else cv_capsule(fx, 61.5f, fx - s * 3.5f, 62, 1.0f, R_BONE, 0.3f, 0.8f);
    }
    for (int s = -1; s <= 1; s += 2) {                               /* the surcoat below the plate, split before, torn */
        for (int y = 40; y < 59; y++) {
            float x0 = 32 + s * 1.0f, x1 = 32 + s * (9.5f + (y - 40) * 0.12f);
            float hem = 57.0f - 5.0f * h01((int)(y > 0) + (int)fabsf(x1), 3, 603);
            if (state == 1 && s < 0) hem = 46.0f;                    /* hacked short on the bared side */
            if (state == 2) hem = 44.0f + 3.0f * (s > 0);
            for (int x = (int)fminf(x0, x1); x <= (int)fmaxf(x0, x1); x++) {
                float h = hem + 3.0f * h01(x, 7, 605);
                if (y > h) continue;
                float u = fabsf(x + 0.5f - 32.0f) / 10.0f, fold = sinf((x - 32) * 1.1f + s) * 0.06f;
                cv_px(x, y, sh(R_BLOOD, 0.24f + fold - u * 0.06f - (y - 40) * 0.004f));
            }
        }
    }
    cv_capsule(32, 18, 32, 40, 1.4f, R_BONE, 0.3f, 0.9f);           /* spine and ribs, where the plate is gone */
    for (int i = 0; i < 4; i++) {
        float y = 21 + i * 3.6f;
        cv_capsule(31, y, 25 + i * 0.6f, y + 2, 0.9f, R_BONE, 0.25f, 0.9f); cv_capsule(33, y, 39 - i * 0.6f, y + 2, 0.9f, R_BONE, 0.25f, 0.9f);
    }
    if (state < 2)                                                   /* the fauld, its lowest band gilt */
        for (int b = 0; b < 3; b++) {
            float y = 37 + b * 2.6f, hw = 9.5f + b * 0.8f;
            for (int x = (int)(32 - hw); x <= (int)(32 + hw); x++) {
                if (state == 1 && x < 28 && b > 0) continue;
                float nx = (x + 0.5f - 32) / hw;
                for (int yy = 0; yy < 3; yy++) cv_px(x, (int)y + yy, yy == 2 && b == 2 ? (uint8_t)(R_GOLD + 4 + (int)(3.0f * (1.0f - fabsf(nx)))) : sh(R_IRON, 0.3f + 0.5f * lit(nx, -0.2f, sqrtf(fmaxf(0, 1 - nx * nx))) - yy * 0.08f));
            }
        }
    for (int y = 18; y <= 37; y++) {                                 /* the breastplate: a ridge, the Order's cross, gilt edges */
        float hw = 9.5f + 1.5f * sinf((y - 18) / 19.0f * PI_F) - (y > 31 ? (y - 31) * 0.6f : 0.0f);
        for (int x = (int)(32 - hw); x <= (int)(32 + hw); x++) {
            float nx = (x + 0.5f - 32) / hw;
            if (state == 2 && (x < 30 - (y - 18) * 0.15f || y > 32)) continue;
            float v = 0.25f + 0.62f * lit(nx, (y - 26) / 14.0f, sqrtf(fmaxf(0.0f, 1.0f - nx * nx)));
            if (fabsf(x + 0.5f - 32) < 0.8f) v += 0.25f;
            if (state >= 1 && h01(x / 2, y / 2, 607) > 0.86f) v -= 0.3f;
            uint8_t c = sh(R_IRON, v);
            if (fabsf(fabsf(x + 0.5f - 32) - hw) < 1.0f) c = (uint8_t)(R_GOLD + 3 + (int)(4.0f * (1.0f - fabsf(nx)) + 2.0f * (y < 26)));   /* the gilt, worn */
            int cross = (abs(x - 32) <= 1 && y >= 20 && y <= 34) || (y >= 24 && y <= 25 && abs(x - 32) <= 5);
            if (cross && !(state == 2 && x < 31)) c = abs(x - 32) == 0 || y == 24 ? sh(R_BONE, 0.85f) : sh(R_BONE, 0.6f);
            cv_px(x, y, c);
        }
    }
    if (state == 2) { cv_line(30, 19, 33, 33, sh(R_IRON, 0.05f)); cv_line(34, 22, 37, 30, sh(R_IRON, 0.05f)); }   /* cracked through */
    if (state < 2)                                                   /* chains wound about it, their shadow on the plate */
        for (int c = 0; c < 2; c++) {
            float y0 = c ? 25.0f : 21.0f, y1 = c ? 37.0f : 35.0f, sg = c ? 1.5f : 2.0f;
            for (int i = 0; i <= 22; i++) { float t = i / 22.0f; cv_px((int)(24.5f + c + 15.0f * t - c), (int)(y0 + (y1 - y0) * t + sg * 4.0f * t * (1.0f - t)) + 1, sh(R_IRON, 0.02f)); }
            kit_chain(24.0f + c, y0, 40.0f - c, y1, sg, R_IRON);
        }
    for (int s = -1; s <= 1; s += 2) {                               /* arms: both hands on the grip */
        float sx = 32 + s * 9.5f, sy = 20;
        float ex = (sx + gx) * 0.5f + s * 4.0f, ey = (sy + gy) * 0.5f + 4.0f;
        if (pose == 2) { ex = (sx + gx) * 0.5f + s * 3.0f; ey = (sy + gy) * 0.5f + 3.0f; }
        cv_bone(sx, sy, ex, ey, 1.3f, 0.25f, 0.9f);
        cv_bone(ex, ey, gx + s * 1.0f, gy, 1.1f, 0.25f, 0.9f);
        int keep = state == 0 || s > 0;
        if (keep) {                                                  /* a great pauldron, its rim gilt, a spike */
            cv_ellipse(sx, sy - 0.5f, 5.6f, 4.4f, R_IRON, 0.3f, 1.0f);
            for (int a = 0; a < 14; a++) { float an = PI_F + a * PI_F / 13.0f; cv_px((int)(sx + cosf(an) * 5.4f), (int)(sy - 0.5f + sinf(an) * 4.2f), (uint8_t)(R_GOLD + 5 + (a & 1))); }
            cv_line(sx + s * 2.0f, sy - 4.0f, sx + s * 4.0f, sy - 8.0f, sh(R_IRON, 0.8f));
        }
        if (state < 2 && (state == 0 || s > 0)) plate(ex, ey, gx + s * 1.0f, gy, 2.1f, 0.05f);   /* vambrace */
        if (s > 0 && state < 2) kit_chain(ex - 1.0f, ey - 2.0f, ex + 3.0f, ey + 3.0f, 0.5f, R_IRON);   /* a chain about the forearm */
    }
    if (pose != 2) greatsword(gx, gy, ga);
    for (int s = -1; s <= 1; s += 2) cv_ellipse(gx + s * 1.2f, gy + 0.5f, 2.3f, 2.1f, state < 2 ? R_IRON : R_BONE, 0.3f, 0.9f);   /* gauntlets, or bone */
    cv_capsule(32, 15, 32, 18, 1.2f, R_BONE, 0.3f, 0.9f);
    if (state < 2) {                                                 /* the plume, torn, streaming from the crest; the helm; its crown */
        for (int k = 0; k < 30; k++) {
            float t = k / 29.0f, px = 33.0f + t * 20.0f, py = 2.0f + t * 12.0f + sinf(t * 7.0f) * 2.0f, wdt = 2.8f - t * 1.8f;
            if (k > 18 && h01(k, 1, 609) < 0.4f) continue;           /* torn at its end */
            for (float o = -wdt; o <= wdt; o += 0.5f) cv_px((int)px, (int)(py + o), sh(R_BLOOD, 0.35f + 0.25f * (1.0f - fabsf(o) / (wdt + 0.01f)) - t * 0.15f));
        }
        grid_stamp(KNIGHT_HELM, 15, 16, 25, 2, R_IRON, R_BLOOD, (uint8_t)(R_FLAME + 9), 0);
        iron_crown(32, 6, 7.5f, state == 1);
    } else {                                                         /* the helm gone: the crowned skull, a light in it */
        skull_stamp(32, 10, 0, 0, (uint8_t)(R_FLAME + 12));
        iron_crown(32, 4, 7.0f, 1);
    }
    cv_bone_decay(611);
    cv_outline(0);
    cv_store(id);
}

/* ---- The Black Deacon's own (docs/InitialArtPass.md, part three; the owner chose a profane bishop) ---- */

/* a bone hand at (hx, hy), its fingers closed round something going up and down (grip) or reaching (a: the way) */
static void bishop_hand(float hx, float hy, int s) {
    cv_ellipse(hx, hy, 1.9f, 1.7f, R_BONE, 0.25f, 0.85f);
    for (int f = 0; f < 3; f++) cv_px((int)(hx - (float)s * 1.5f), (int)(hy - 1.0f + (float)f), sh(R_BONE, 0.7f - 0.1f * (float)f));   /* knuckles round the grip */
}

/* the mitre over a brow at (cx, by): tall and pointed, black, a crimson band up its front and another round its foot,
 * worn gilt at its edges, an inverted cross on its front; tilt leans it (the fallen one) */
static void bishop_mitre(float cx, float by, float h, float hw) {
    for (int y = (int)(by - h); y <= (int)by + 2; y++)
        for (int x = (int)(cx - hw) - 1; x <= (int)(cx + hw) + 1; x++) {
            float t = clampf(((float)y - (by - h)) / h, 0.0f, 1.0f), w = hw * powf(t, 0.65f) + 0.4f;
            float dx = (float)x + 0.5f - cx;
            if (fabsf(dx) > w) continue;
            float u = dx / (w + 0.01f), v = 0.1f + 0.22f * (1.0f - u * u) * (0.6f + 0.4f * (1.0f - t)) - (u > 0.3f ? 0.05f : 0.0f);
            uint8_t c = sh(R_IRON, v);
            if (fabsf(dx) < 1.6f && t > 0.12f) c = sh(R_BLOOD, 0.22f + 0.12f * (dx < 0));   /* the band up its front */
            if (y >= (int)by) c = sh(R_BLOOD, 0.2f + 0.12f * (1.0f - fabsf(u)) + (y == (int)by ? 0.08f : 0.0f));   /* the band round its foot */
            if (fabsf(u) > 0.84f && ((x + y) & 1) == 0) c = (uint8_t)(R_GOLD + 3 + (int)(3.0f * (1.0f - t) * (dx < 0)));   /* the gilt, worn */
            if (y == (int)by - 1 || y == (int)by + 3) { if ((x & 1) == 0) c = (uint8_t)(R_GOLD + 4); }
            cv_px(x, y, c);
        }
    int cy = (int)(by - h * 0.62f);                                  /* the cross on its front, upside down */
    for (int k = -3; k <= 2; k++) cv_px((int)cx, cy + k, (uint8_t)(R_GOLD + 7 + (k < 0)));
    for (int k = -2; k <= 2; k++) cv_px((int)cx + k, cy + 1, (uint8_t)(R_GOLD + 6 + (k == 0) * 3));
}

/* his crozier: a staff of black iron banded in gilt from (x0, y0) up to (x1, y1), crowned with a skull, its crook
 * arched over it, chains hung from the crook's end */
static void bishop_crozier(float x0, float y0, float x1, float y1, float sway) {
    cv_capsule(x0, y0, x1, y1, 1.1f, R_IRON, 0.12f, 0.55f);
    float L = hypotf(x1 - x0, y1 - y0);
    for (int b = 1; b <= 4; b++) {                                   /* gilt bands */
        float t = (float)b / 5.0f, x = x0 + (x1 - x0) * t, y = y0 + (y1 - y0) * t;
        cv_px((int)x - 1, (int)y, (uint8_t)(R_GOLD + 5)); cv_px((int)x, (int)y, (uint8_t)(R_GOLD + 8)); cv_px((int)x + 1, (int)y, (uint8_t)(R_GOLD + 4));
    }
    (void)L;
    float sx = x1, sy = y1 - 4.0f;                                   /* the skull on its head */
    for (int k = 0; k <= 40; k++) {                                  /* the crook: a thick volute, up from behind the skull, over it, curling in */
        float t = (float)k / 40.0f, a = PI_F * 0.95f + t * PI_F * 1.75f, r = 6.6f - 3.4f * t * t;
        float x = sx + 1.0f + cosf(a) * r, y = sy - 0.5f + sinf(a) * r;
        float lv = 0.35f + 0.35f * (-cosf(a) * 0.5f - sinf(a) * 0.6f);
        cv_px((int)x, (int)y, sh(R_IRON, lv)); cv_px((int)(x - cosf(a)), (int)(y - sinf(a)), sh(R_IRON, lv * 0.6f));
        if (k % 8 == 4) cv_px((int)(x + cosf(a) * 0.8f), (int)(y + sinf(a) * 0.8f), (uint8_t)(R_GOLD + 6));   /* crockets of gilt along it */
    }
    grid_stamp(SKULL_SMALL, 9, 10, (int)sx - 4, (int)sy - 4, R_BONE, R_BONE, 0, 0);
    cv_px((int)sx - 2, (int)sy - 1, (uint8_t)(R_MOSS + 9)); cv_px((int)sx + 2, (int)sy - 1, (uint8_t)(R_MOSS + 8));   /* a green spark in its sockets */
    for (int c = 0; c < 2; c++) {                                    /* chains hung from the crook, links light and dark */
        float cx0 = c ? sx - 5.5f : sx + 5.0f, cy0 = c ? sy + 2.0f : sy + 3.0f;
        for (int k = 0; k < 9 - 2 * c; k++) cv_px((int)(cx0 + sway * 0.12f * (float)k), (int)cy0 + k, sh(R_IRON, (k & 1) ? 0.62f : 0.25f));
    }
}

/* his censer on its chains from a hand at (hx, hy), the bowl at (cx, cy): brass gone dark, its pierced holes glowing green,
 * the smoke going up from it in strands (more: how much) */
static void bishop_censer(float hx, float hy, float cx, float cy, float more) {   /* more: unused, the smoke is bishop_smoke's */
    for (int c = -1; c <= 1; c++) cv_line(hx, hy, cx + (float)c * 2.2f, cy - 3.0f, sh(R_IRON, 0.45f + 0.15f * (float)c));
    cv_ellipse(cx, cy - 3.5f, 2.0f, 1.4f, R_GOLD, 0.2f, 0.6f);         /* its lid */
    cv_px((int)cx, (int)cy - 5, (uint8_t)(R_GOLD + 6));
    cv_ellipse(cx, cy, 3.4f, 3.0f, R_GOLD, 0.08f, 0.5f);              /* the bowl */
    for (int k = 0; k < 5; k++) cv_px((int)(cx - 2.0f + (float)k), (int)cy - (k & 1), (uint8_t)(R_MOSS + 13 + (k & 1) * 2));   /* holes, green inside */
    cv_px((int)cx, (int)cy + 3, (uint8_t)(R_GOLD + 3));
    (void)more;
}

/* the censer's smoke going up from a bowl at (cx, cy) in thin puffs, drifting off as it climbs and thinning (more: how
 * much). Drawn after the outline, so that it has none */
static void bishop_smoke(float cx, float cy, float more) {
    int n = (int)(18.0f * more);
    for (int s = 0; s < 2; s++)
        for (int k = 0; k < n; k++) {
            float t = (float)k / 18.0f, x = cx + 1.0f + sinf(t * 4.5f + (float)s * 2.6f) * (0.8f + 2.5f * t) + (5.0f + 3.0f * (float)s) * t * t * 1.4f;
            float y = cy - 5.0f - (float)k * 1.5f, r = 0.9f + 1.8f * t;
            for (int j = (int)(-r); j <= (int)r; j++)
                for (int i = (int)(-r); i <= (int)r; i++) {
                    int px = (int)x + i, py = (int)y + j;
                    if ((float)(i * i + j * j) > r * r || h01(px, py, 601 + (uint32_t)s) > 0.6f - 0.45f * t) continue;
                    cv_px(px, py, sh(R_MOSS, 0.55f - 0.3f * t + (j < 0 ? 0.06f : 0.0f)));
                }
        }
}

/* The Black Deacon's own (docs/InitialArtPass.md, part three; the owner chose a profane bishop): the Order's bishop gone
 * to bone and kept on saying the rites. Towering in black vestments with a crimson orphrey and an inverted cross, a tall
 * mitre, crimson lappets; a skull face lit green from below; a crozier crowned with a skull and hung with chains in one
 * hand, a censer swinging green smoke from the other. His bone ward is drawn by the game, wheeling about him. pose 0-1
 * walking (the censer swinging), 2 chanting (the crozier and censer raised, the smoke billowing), 3 fallen. */
static void spr_bdeacon(int id, float w, int pose) {
    cv_begin(cv_sprite, TS, TS);
    if (pose == 3) {                                                 /* fallen: the vestments in a heap, the mitre rolled off */
        cv_flat(32, 61.5f, 26, 2.5f, R_WARM + 1);
        float m[12] = { 6, 62, 12, 51, 26, 46, 42, 48, 54, 55, 58, 62 };
        cv_poly(m, 6, R_IRON, 0.3f, 0.05f, 46, 16);
        for (int y = 47; y < 63; y++) for (int k = -2; k <= 2; k++) cv_px(30 + (y - 47) / 3 + k, y, sh(R_BLOOD, 0.25f - 0.03f * (float)abs(k)));   /* the orphrey, twisted */
        bishop_crozier(4, 61, 50, 49, 1.0f);
        skull_stamp(24, 50, 1, 1, 0);                                /* the skull on its side, its light gone */
        bishop_mitre(49, 58, 9, 5.5f);
        float cx = 14, cy = 58;                                      /* the censer spilled, its coals still green */
        cv_ellipse(cx, cy, 3.2f, 2.4f, R_GOLD, 0.08f, 0.45f);
        for (int k = 0; k < 6; k++) cv_px((int)(cx + 3 + (float)k * 1.3f), (int)cy + 1 + (k & 1), (uint8_t)(R_MOSS + 10 + (k % 3) * 2));
        cv_bone_decay(589);
        cv_outline(0);
        cv_store(id);
        return;
    }
    int chant = pose == 2;
    cv_flat(32, 62, 17, 1.8f, R_WARM + 1);
    for (int s = -1; s <= 1; s += 2)                                 /* the lappets, hanging behind from the mitre */
        for (int y = 11; y < 30; y++) {
            float x = 32 + (float)s * (5.5f + (float)(y - 11) * 0.12f) + w * 0.3f * (float)(y - 11) / 19.0f;
            cv_px((int)x, y, sh(R_BLOOD, 0.18f + 0.08f * (s < 0))); cv_px((int)x + s, y, sh(R_BLOOD, 0.12f));
            if (y >= 27) { cv_px((int)x, y + 1, (uint8_t)(R_GOLD + 4)); cv_px((int)x + s, y + 2, (uint8_t)(R_GOLD + 3)); }   /* gilt fringe */
        }
    for (int y = 20; y < 63; y++) {                                  /* the chasuble: black, belled, its folds falling straight */
        float t = (float)(y - 20) / 42.0f, hw = 9.5f + 8.0f * t, sway = w * 1.4f * t;
        for (int x = (int)(32 - hw + sway) - 1; x <= (int)(32 + hw + sway) + 1; x++) {
            float dx = (float)x + 0.5f - 32.0f - sway;
            if (fabsf(dx) > hw) continue;
            if (y > 59 && h01(x, y, 591 + (w > 0)) < (float)(y - 59) / 4.0f) continue;   /* the hem dragging, frayed */
            float u = dx / hw, v = 0.05f + 0.26f * (1.0f - u * u) * (1.0f - 0.25f * t) + (u < -0.3f ? 0.04f : 0.0f);
            if (sinf(dx * 1.5f + (float)y * 0.03f) > 0.6f) v -= 0.07f;
            uint8_t c = sh(R_IRON, v);
            if (fabsf(u) > 0.93f) c = sh(R_BLOOD, 0.22f);           /* its crimson lining, turned at the edge */
            if (y >= 59 && ((x + y) & 1) == 0 && y <= 60) c = (uint8_t)(R_GOLD + 3);   /* the hem's gilt, worn */
            cv_px(x, y, c);
        }
    }
    for (int y = 21; y < 62; y++) {                                  /* the orphrey: a crimson Y from the shoulders, gilt at its edges */
        float sway = w * 1.4f * (float)(y - 20) / 42.0f;
        if (y < 33) {
            for (int s = -1; s <= 1; s += 2) {
                float cx = 32.0f + (float)s * (8.0f - (float)(y - 21) * 0.62f) + sway;
                for (int k = -1; k <= 1; k++) cv_px((int)cx + k, y, sh(R_BLOOD, 0.3f + 0.08f * (s < 0) - 0.04f * (float)abs(k)));
                cv_px((int)cx + 2 * s, y, (uint8_t)(R_GOLD + 4)); 
            }
        } else {
            for (int k = -2; k <= 2; k++) cv_px((int)(32.0f + sway) + k, y, abs(k) == 2 ? (((y & 1) == 0) ? (uint8_t)(R_GOLD + 4) : sh(R_BLOOD, 0.15f)) : sh(R_BLOOD, 0.3f + 0.06f * (k < 0)));
        }
    }
    {                                                                /* the inverted cross on it, in bone */
        int cx = (int)(32.0f + w * 1.4f * 0.45f);
        for (int y = 37; y <= 47; y++) cv_px(cx, y, sh(R_BONE, 0.8f - 0.02f * (float)(y - 37)));
        for (int k = -2; k <= 2; k++) cv_px(cx + k, 44, sh(R_BONE, 0.75f - 0.08f * (float)abs(k)));
    }
    for (int x = 25; x <= 39; x++)                                   /* the amice: a high dark collar, crimson in its fold */
        for (int y = 20; y <= 23; y++) {
            float u = ((float)x + 0.5f - 32.0f) / 7.5f;
            if (u * u + ((float)y - 23.5f) * ((float)y - 23.5f) / 12.0f > 1.0f) continue;
            cv_px(x, y, y == 20 ? sh(R_BLOOD, 0.2f) : sh(R_IRON, 0.1f + 0.1f * (1.0f - fabsf(u))));
        }
    /* the crozier in his right hand (to our left), the censer in his left */
    float chx = chant ? 15.0f : 16.0f, chy = chant ? 22.0f : 35.0f;
    float sx = chant ? 12.0f : 13.5f, sy0 = chant ? 50.0f : 61.0f, sy1 = chant ? 4.0f : 12.0f;
    bishop_crozier(sx, sy0, sx + (chant ? 1.0f : 0.0f), sy1, w);
    float hx = chant ? 51.0f : 46.0f, hy = chant ? 10.0f : 31.0f;
    float bx = chant ? 55.0f : 46.0f + 4.0f * w, by = chant ? 24.0f : 46.0f;
    for (int s = -1; s <= 1; s += 2) {                               /* sleeves, belled, black lined with crimson */
        float shx = 32.0f + (float)s * 9.0f, shy = 23.0f, tx = s < 0 ? chx : hx, ty = s < 0 ? chy : hy;
        float ex = (shx + tx) * 0.5f + (float)s * 2.5f, ey = (shy + ty) * 0.5f + (chant ? 2.0f : 3.0f);
        if (chant) { cv_bone(ex, ey, tx, ty + 1.5f, 1.0f, 0.25f, 0.85f); }   /* raised: the sleeve fallen back from the bone */
        cv_capsule(shx, shy, ex, ey, 3.4f, R_IRON, 0.06f, 0.36f);
        if (!chant) {
            cv_capsule(ex, ey, tx + (float)s * 0.5f, ty - 2.0f, 3.2f, R_IRON, 0.06f, 0.34f);
            cv_ellipse(tx + (float)s * 0.6f, ty - 1.5f, 3.6f, 2.0f, R_BLOOD, 0.12f, 0.3f);   /* the cuff's lining */
        } else cv_ellipse(ex, ey + 1.0f, 3.8f, 2.4f, R_BLOOD, 0.12f, 0.3f);
        bishop_hand(tx, ty, s);
    }
    if (chant) cv_capsule(sx - 1.0f, chy - 3.0f, sx + 2.0f, chy + 3.0f, 0.9f, R_IRON, 0.12f, 0.55f);   /* the staff again, in front of the hand */
    bishop_censer(hx, hy + 1.0f, bx, by, 0.0f);
    bishop_mitre(32, 10, 11, 7.5f);
    skull_stamp(32, 17, 0, 0, (uint8_t)(chant ? R_GLOW + 14 : C_WHITE));   /* his face, under it */
    static uint8_t face[TS * TS];                                   /* lit green from below: the lower planes catch it, the brow and dome stay dark */
    memcpy(face, cv_sprite, sizeof face);
    for (int y = 11; y < 25; y++)
        for (int x = 24; x < 41; x++) {
            uint8_t c = face[y * TS + x];
            if (c <= R_BONE || c >= R_BONE + 16) continue;
            float f = clampf(((float)y - 13.0f) / 10.0f, 0.0f, 1.0f), b = (float)(c - R_BONE) / 15.0f;
            int below = y + 1 < TS && (face[(y + 1) * TS + x] == 0 || face[(y + 1) * TS + x] == C_CLEAR);   /* a lower edge, over a hollow: the light's catch */
            float g = 0.06f + 0.8f * f * b * b + (below ? 0.3f : 0.0f) + (chant ? 0.08f : 0.0f);
            cv_px(x, y, f > 0.25f || below ? sh(R_MOSS, g) : sh(R_BONE, 0.12f + b * 0.3f));
        }
    if (chant)                                                       /* the green rising round him */
        for (int i = 0; i < 18; i++) cv_px((int)(32.0f + (h01(i, 1, 595) - 0.5f) * 40.0f), (int)(28.0f + 30.0f * h01(i, 2, 595)), (uint8_t)(R_MOSS + 10 + (i % 5)));
    cv_bone_decay(597);
    cv_outline(0);
    bishop_smoke(bx, by, chant ? 1.4f : 0.85f);
    cv_store(id);
}

/* the bestiary's bones age with the level, like the skeletons' */
static void spr_bestiary_bones(void) {
    spr_hound(SP_HOUND_A, 1.0f, 0); spr_hound(SP_HOUND_B, -1.0f, 1); spr_hound(SP_HOUND_BITE, 0.0f, 2); spr_hound(SP_HOUND_D, 0.0f, 3);
    hound_dress(SP_HOUND_A, 1.0f, 0); hound_dress(SP_HOUND_B, -1.0f, 1); hound_dress(SP_HOUND_BITE, 0.0f, 2);
    for (int st = 0; st < 3; st++) {
        int base = SP_KNIGHT_A + st * 4;
        spr_knight(base, 1.0f, 0, st); spr_knight(base + 1, -1.0f, 1, st); spr_knight(base + 2, 0.0f, 2, st); spr_knight(base + 3, 0.0f, 3, st);
        for (int k = 0; k < 4; k++) knight_dress(base + k, st);
        spr_knight(SP_KNIGHT_P0 + st, 0.0f, 4, st);
    }
    spr_knight_dead(SP_KNIGHT_D);
    for (int st = 0; st < 3; st++) {                                /* the Ill-Fated Knight's own */
        int base = SP_BKNIGHT_A + st * 4;
        spr_bknight(base, 1.0f, 0, st); spr_bknight(base + 1, -1.0f, 1, st); spr_bknight(base + 2, 0.0f, 2, st); spr_bknight(base + 3, 0.0f, 3, st);
        spr_bknight(SP_BKNIGHT_P0 + st, 0.0f, 4, st);
    }
    spr_deacon(SP_DEACON_A, 1.0f, 0); spr_deacon(SP_DEACON_B, -1.0f, 1); spr_deacon(SP_DEACON_CHANT, 0.0f, 2); spr_deacon(SP_DEACON_D, 0.0f, 3);
    deacon_dress(SP_DEACON_A); deacon_dress(SP_DEACON_B); deacon_dress(SP_DEACON_CHANT);
    spr_bdeacon(SP_BDEACON_A, 1.0f, 0); spr_bdeacon(SP_BDEACON_B, -1.0f, 1); spr_bdeacon(SP_BDEACON_CHANT, 0.0f, 2); spr_bdeacon(SP_BDEACON_D, 0.0f, 3);   /* the Black Deacon's own */
}

/* The hunting spider's face (docs/InitialArtPass.md, section 8), a wolf spider's: two great eyes shining in front, two
 * more above them, a row of four small ones below, all fire; fur in R_WOOD (hex) */
static const char *const HUNTER_HEAD[10] = {
    "....4567654....", "..356J898J653..", ".3567aaaaa7653.", "3560FNF7FNF0653", "3570N@N8N@N0753",
    "3460FMF7FMF0643", ".3567898987653.", ".35H6H787H6H53.", "..34567876543..", "...345565543...",
};

/* a hairy abdomen: fur in short strokes, paler at its rim, bristles; mark 0 a dark stripe down it and chevrons either
 * side, 1 nothing (the side view draws its own) */
static void hunter_abdomen(float cx, float cy, float rx, float ry, int ramp, int mark, uint32_t seed) {
    for (int y = (int)(cy - ry) - 1; y <= (int)(cy + ry) + 1; y++)
        for (int x = (int)(cx - rx) - 1; x <= (int)(cx + rx) + 1; x++) {
            float nx = (x + 0.5f - cx) / rx, ny = (y + 0.5f - cy) / ry, d = nx * nx + ny * ny;
            if (d > 1.0f) continue;
            float v = 0.12f + 0.4f * (1.0f - d) * (0.65f + 0.35f * (-ny * 0.8f + 0.4f)) + (h01(x, y, seed) - 0.5f) * 0.16f;
            if (h01(x / 2, y, seed + 1) > 0.8f) v += 0.12f;            /* a lighter stroke of hair */
            if (mark == 0 && fabsf(nx) < 0.12f) v -= 0.18f;            /* the stripe down its back */
            if (mark == 0 && fabsf(fabsf(nx) - 0.45f + ny * 0.35f) < 0.08f && ny > -0.6f) v -= 0.15f;   /* chevrons */
            cv_px(x, y, sh(ramp, v));
        }
    for (int q = 0; q < 36; q++) {                                   /* bristles round its rim */
        float an = TAU_F * q / 36.0f + h01(q, 3, seed) * 0.15f, l = 1.2f + 1.8f * h01(q, 4, seed);
        float ex = cx + cosf(an) * rx, ey = cy + sinf(an) * ry;
        cv_line(ex, ey, ex + cosf(an) * l, ey + sinf(an) * l, sh(ramp, 0.42f));
    }
}

/* The hunting spider: a big brown wolf spider, hairy, low and wide, its two great front eyes shining back at you, its
 * legs banded dark and bristling at the knees, fangs with venom on them. Body R_WOOD (PF_HIDE). pose 0-1 running,
 * 2 leaping, 3 dead. */
static void spr_hunter(int id, int frame) {
    cv_begin(cv_sprite, TS, TS);
    float cx = 32, cy = frame == 3 ? 54 : 44;
    if (frame != 2 && frame != 3) cv_flat(32, 62, 24, 2.0f, R_WARM + 1);
    if (frame == 3) {                                                /* dead on its back, its legs curled over it */
        hunter_abdomen(cx, cy - 2, 10.0f, 6.5f, R_WOOD, 0, 481);
        for (int s = -1; s <= 1; s += 2)
            for (int l = 0; l < 4; l++)
                chitin_leg(cx + s * (3 + l), cy - 5, cx + s * (8 + l * 2), cy - 10 - l * 2, cx + s * (4 + l), cy - 16 - l * 1.5f, 1.4f, R_WOOD, 0.12f, 0.55f, R_IRON, 483 + (uint32_t)(l * 2 + s + 1));
        cv_ellipse(cx, cy - 6, 7.0f, 4.5f, R_WOOD, 0.12f, 0.5f);
        cv_outline(0);
        cv_store(id);
        return;
    }
    float ay = frame == 2 ? cy - 6 : cy - 5;
    hunter_abdomen(cx, ay, 10.5f, 7.5f, R_WOOD, 0, 485);            /* the abdomen, behind and above */
    for (int s = -1; s <= 1; s += 2)
        for (int l = 3; l >= 0; l--) {                              /* legs: up into a high knee, then out and down */
            float k = (frame == 1 ? (l & 1 ? 1.0f : -1.0f) : (frame == 0 ? (l & 1 ? -1.0f : 1.0f) : 0.0f)) * s;
            float kx = cx + s * (5 + l * 1.2f), ky = cy - 2 + l * 2.0f;
            float mx = cx + s * (14 + l * 3.0f), my = cy - 14 + l * 3.0f + k * 2.0f;
            float fx = cx + s * (20 + l * 4.0f) + k, fy = 61 - (l == 0 ? 2.0f : 0.0f);
            if (frame == 2) { mx = cx + s * (12 + l * 2); my = cy - 16 - (3 - l) * 2.0f; fx = cx + s * (16 + l * 4); fy = cy - 24 + l * 6.0f; }
            chitin_leg(kx, ky, mx, my, fx, fy, 1.5f, R_WOOD, 0.12f, 0.6f, R_IRON, 487 + (uint32_t)(l * 2 + s + 1));
        }
    cv_ellipse(cx, cy + 4, 8.0f, 6.0f, R_WOOD, 0.15f, 0.62f);       /* the head and fore-body */
    for (int y = (int)cy - 2; y < (int)cy + 11; y++)                /* fur on it */
        for (int x = (int)cx - 9; x < (int)cx + 9; x++) {
            uint8_t c = cv_sprite[y * TS + x];
            if (c >= R_WOOD && c < R_WOOD + 16 && h01(x, y, 489) > 0.8f) cv_px(x, y, (uint8_t)(c + 3 > R_WOOD + 15 ? R_WOOD + 15 : c + 3));
        }
    grid_stamp(HUNTER_HEAD, 15, 10, (int)cx - 7, (int)cy - 1, R_WOOD, R_WOOD, (uint8_t)(R_FLAME + 15), 0);
    float open = frame == 2 ? 3.0f : 1.0f;
    for (int k = -1; k <= 1; k += 2) {                              /* palps, then fangs with a drop on each */
        cv_capsule(cx + k * 6.0f, cy + 7, cx + k * 7.0f, cy + 12, 1.2f, R_WOOD, 0.15f, 0.5f);
        cv_ellipse(cx + k * 2.0f, cy + 9, 1.7f, 1.8f, R_WOOD, 0.1f, 0.35f);
        float tx = cx + k * (2.0f + open * 0.5f), ty = cy + 11 + open;
        cv_line(cx + k * 2.0f, cy + 9, tx, ty, sh(R_IRON, 0.35f)); cv_px((int)tx, (int)ty, sh(R_BONE, 0.85f));
        cv_px((int)tx, (int)ty + 1, (uint8_t)(R_MOSS + 14)); cv_px((int)tx, (int)ty + 2, (uint8_t)(R_MOSS + 9));
    }
    cv_outline(0);
    cv_store(id);
}

/* the hunting spider's dressing by rank: II silk on it and mould, III frost and a leg's end snapped, IV old blood on
 * its fangs and a bald scar torn in its fur, V charred and the pale skull on its back, gilded gold in its fur */
static void hunter_dress(int id, int frame) {
    float cx = 32, cy = 44, ay = frame == 2 ? cy - 6 : cy - 5;
    for (int r = RK_II; r < NRANK; r++) {
        g_dress_ramp2 = R_IRON;                                     /* its bands and fangs */
        dress_begin(id);
        if (r == RK_II) {
            kit_moss(0.25f, 0, 491);
            for (int q = 0; q < 4; q++) {
                float x0 = 12 + 40 * h01(q, 1, 493), y0 = 30 + 10 * h01(q, 2, 493), x1 = x0 + (h01(q, 3, 493) - 0.5f) * 14, y1 = y0 + 8 + 8 * h01(q, 4, 493);
                for (int k = 0; k <= 12; k++) { float t = k / 12.0f; int x = (int)(x0 + (x1 - x0) * t), y = (int)(y0 + (y1 - y0) * t + sinf(t * PI_F) * 2.0f); if (base_on(x, y) || (k & 1)) cv_px(x, y, sh(R_GHOST, 0.65f)); }
            }
        }
        if (r == RK_III) {
            kit_frost(0.65f, 495);
            float x = cx - 5, y = ay - 6;                            /* its back cracked */
            for (int k = 0; k < 11; k++) { cv_px((int)x, (int)y, 0); cv_px((int)x + 1, (int)y, sh(R_WOOD, 0.75f)); x += h01(k, 1, 497) < 0.5f ? 1.0f : 0.0f; y += 1.0f; }
        }
        if (r >= RK_IV && r != RK_GILDED) {
            kit_blood(cx - 2, cy + 10, 5, 2, 499); kit_blood(cx + 2, cy + 10, 5, 2, 501);
            for (int y = 0; y < 5; y++) for (int x = 0; x < 7 - y; x++) cv_px((int)cx + 3 + x, (int)ay - 3 + y, (x + y) & 1 ? (uint8_t)(R_SKIN + 6) : (uint8_t)(R_BLOOD + 5));   /* torn bald */
        }
        if (r == RK_V) {
            g_dress_ramp2 = R_BONE;
            kit_char(0.3f, 503);
            cv_flat(cx + 0.5f, ay - 0.2f, 6.2f, 6.6f, 0);
            grid_stamp(SKULL_SMALL, 9, 10, (int)cx - 4, (int)ay - 5, R_BONE, R_BONE, 0, 0);
        }
        if (r == RK_GILDED) kit_gold(R_WOOD, 0.55f, 505);
        dress_store(id, r, R_WOOD);
    }
    g_dress_ramp2 = -1;
}

/* ---- The Wandering Spider's own (docs/InitialArtPass.md, part three; the owner chose an old matriarch) ---- */

/* her face: two great burning eyes in black sockets under a brow of fur, six milky blind ones ringed round them (two
 * above, four small below, the last scarred over); fur in R_WARM (hex), the milky eyes R_GHOST (g-v), fire A-P */
static const char *const MATRIARCH_FACE[12] = {
    ".....234566654432....", "...2457889a9887765...", ".0rs0788999887760qr0.", ".0pq0008898800060op0.",
    "24000DGD00800CFC00021", "2567GP@KG776FN@JF3321", "2456JNMKG665IMLJF2211", "2345GKJGD554FJIFC2211",
    ".2300CDC00400BCb00...", "..233000333220c011...", "...220s0r020r0c01....", "....111111111b11.....",
};

/* grizzled fur over an ellipse: dark at the roots, lit from the upper left, long hairs gone grey-white lying down and
 * out over it (grey: how many; fewer in the shade), and a shaggy fringe round its rim */
static void grizzled(float cx, float cy, float rx, float ry, float lo, float hi, float grey, uint32_t seed) {
    for (int y = (int)(cy - ry) - 1; y <= (int)(cy + ry) + 1; y++)
        for (int x = (int)(cx - rx) - 1; x <= (int)(cx + rx) + 1; x++) {
            float nx = (x + 0.5f - cx) / rx, ny = (y + 0.5f - cy) / ry, d = nx * nx + ny * ny;
            if (d > 1.0f) continue;
            float v = lo + (hi - lo) * lit(nx, ny, sqrtf(1.0f - d)) + (h01(x, y, seed) - 0.5f) * 0.1f;
            if (h01(x / 2, y, seed + 1) > 0.78f) v += 0.08f;           /* the lie of the hair */
            int gr = h01(x, y / 2, seed + 7) < grey * (0.25f + 1.2f * (v - lo) / (hi - lo + 0.01f));   /* gone grey */
            cv_px(x, y, gr ? sh(R_IRON, v + 0.18f) : sh(R_WARM, v));
        }
    int n = (int)(rx * ry * grey);
    for (int i = 0; i < n; i++) {                                     /* grey hairs, down and out from where they grow */
        float a = h01(i, 1, seed + 2) * TAU_F, r = sqrtf(h01(i, 2, seed + 2)) * 0.9f;
        float nx = cosf(a) * r, ny = sinf(a) * r, x = cx + nx * rx, y = cy + ny * ry;
        float light = lit(nx, ny, sqrtf(fmaxf(0.0f, 1.0f - nx * nx - ny * ny)));
        if (h01(i, 4, seed + 2) > 0.3f + light) continue;
        float dx = nx * 0.6f, dy = 0.8f + ny * 0.2f, L = 1.5f + 2.0f * h01(i, 3, seed + 2);
        for (int k = 0; k <= (int)L; k++)
            cv_px((int)(x + dx * k), (int)(y + dy * k), k == (int)L ? sh(R_IRON, 0.68f + 0.25f * light) : sh(R_WARM, 0.4f + 0.3f * light));
    }
    int nf = (int)((rx + ry) * 2.0f);
    for (int q = 0; q < nf; q++) {                                    /* the fringe round its rim */
        float an = TAU_F * (float)q / (float)nf + h01(q, 5, seed) * 0.1f, l = 1.0f + 2.5f * h01(q, 6, seed);
        float ex = cx + cosf(an) * rx, ey = cy + sinf(an) * ry, dx = cosf(an) * 0.7f, dy = sinf(an) * 0.7f + 0.5f;
        for (int k = 1; k <= (int)l; k++) cv_px((int)(ex + dx * k), (int)(ey + dy * k), (q % 3) == 0 ? sh(R_IRON, 0.6f) : sh(R_WARM, 0.22f));
    }
}

/* one of her legs in three joints, thinning as they go: the thigh up to the knee, the shin bowing out, the foot; banded
 * dark at each joint, grey hairs hanging off the thigh, a bone-pale spur curving out past the knee and smaller ones down
 * the shin; s the side; broken: the shin snapped off short, a jagged end */
static void spurred_leg(float ax, float ay, float kx, float ky, float fx, float fy, float r, int s, int broken, uint32_t seed) {
    float mx = kx + (fx - kx) * 0.55f + (float)s * 2.2f, my = ky + (fy - ky) * 0.55f;   /* the second joint, bowed out */
    if (broken) { fx = kx + (mx - kx) * 0.6f; fy = ky + (my - ky) * 0.6f; mx = fx; my = fy; }
    cv_capsule(ax, ay, kx, ky, r + 0.7f, R_WARM, 0.0f, 0.0f);       /* a dark edge, to part it from the legs behind */
    cv_capsule(kx, ky, mx, my, r * 0.62f + 0.6f, R_WARM, 0.0f, 0.0f);
    cv_capsule(ax, ay, kx, ky, r, R_WARM, 0.07f, 0.52f);
    cv_capsule(kx, ky, mx, my, r * 0.62f, R_WARM, 0.07f, 0.5f);
    if (!broken) cv_capsule(mx, my, fx, fy, r * 0.4f, R_WARM, 0.06f, 0.44f);
    cv_ellipse(kx, ky, r * 1.15f, r * 1.15f, R_WARM, 0.1f, 0.62f);   /* the knee, knotted */
    float S[3][5] = { { ax, ay, kx, ky, r }, { kx, ky, mx, my, r * 0.62f }, { mx, my, fx, fy, r * 0.4f } };
    for (int g = 0; g < 3 - broken; g++) {                           /* bristles all along it, standing off its edges */
        float vx = S[g][2] - S[g][0], vy = S[g][3] - S[g][1], vl = hypotf(vx, vy) + 0.01f, nx = -vy / vl, ny = vx / vl;
        int e = nx * (float)s > 0.0f || (fabsf(nx) < 0.3f && ny < 0.0f) ? 1 : -1;   /* the side away from her */
        for (float d = 1.5f; d < vl - 0.5f; d += 3.2f) {
                float bx = S[g][0] + vx / vl * d + nx * (float)e * (S[g][4] + 0.3f), by = S[g][1] + vy / vl * d + ny * (float)e * (S[g][4] + 0.3f);
                float l = 0.8f + 1.2f * h01((int)(d * 3.0f), e + g * 4, seed), grey = h01((int)(d * 5.0f), e + g * 4, seed + 1) < 0.35f;
                cv_line(bx, by, bx + nx * (float)e * l + vx / vl * 0.6f, by + ny * (float)e * l + 0.7f, grey ? sh(R_IRON, 0.55f) : sh(R_WARM, 0.16f));
        }
    }
    for (int q = 0; q < 6; q++) {                                    /* grey in the hair of the thigh */
        float t = h01(q, 7, seed), x = ax + (kx - ax) * t, y = ay + (ky - ay) * t;
        cv_px((int)(x - (float)s * 0.6f), (int)y, sh(R_IRON, 0.55f + 0.2f * h01(q, 8, seed)));
    }
    if (broken) for (int q = -1; q <= 1; q++) cv_line(fx, fy, fx + (float)q * 1.2f + (float)s * 0.6f, fy + 1.5f + (float)(q & 1), sh(R_BONE, 0.45f));   /* splintered */
    float J[3][2] = { { kx, ky }, { mx, my }, { (ax + kx) * 0.5f, (ay + ky) * 0.5f } };
    for (int j = 0; j < 3 - broken; j++)                             /* dark bands at the joints, and one round the thigh */
        for (int q = -2; q <= 2; q++) cv_px((int)(J[j][0] + (float)q * 0.7f), (int)(J[j][1] + (float)q * 0.15f), (uint8_t)(R_IRON + 1 + (q == 0)));
    for (int q = 0; q < 5; q++) {                                    /* grey hairs hanging off the thigh and shin */
        float t = 0.2f + 0.2f * (float)q, x, y;
        if (q < 3) { x = ax + (kx - ax) * t * 1.4f; y = ay + (ky - ay) * t * 1.4f; } else { x = kx + (mx - kx) * (t - 0.5f); y = ky + (my - ky) * (t - 0.5f); }
        float l = 2.0f + 2.5f * h01(q, 1, seed);
        cv_line(x + (float)s * r * 0.8f, y, x + (float)s * (r * 0.8f + l * 0.5f), y + l, sh(R_IRON, 0.5f + 0.2f * (float)(q & 1)));
    }
    float ux = kx - ax, uy = ky - ay, L = hypotf(ux, uy);            /* the spur, on past the knee and curving up */
    ux /= L; uy /= L;
    for (int k = 0; k <= 5; k++) {
        float t = (float)k / 5.0f, bx = kx + ux * 1.3f * (float)k + (float)s * 0.5f * (float)k, by = ky + uy * 1.3f * (float)k - 0.22f * (float)(k * k);
        cv_px((int)bx, (int)by, sh(R_BONE, 0.3f + 0.5f * t));
        if (k < 3) { cv_px((int)bx + s, (int)by, sh(R_BONE, 0.2f + 0.1f * t)); cv_px((int)bx, (int)by + 1, sh(R_BONE, 0.12f)); }   /* thick at its root */
    }
    if (!broken) {                                                   /* a smaller one down the shin, pointing back */
        float bx = kx + (mx - kx) * 0.45f, by = ky + (my - ky) * 0.45f;
        cv_line(bx, by, bx + (float)s * 2.5f, by - 1.5f, sh(R_BONE, 0.35f));
        cv_px((int)(bx + (float)s * 2.5f), (int)(by - 1.5f), sh(R_BONE, 0.55f));
    }
}

/* a husk of her prey wound in silk, hung at (x, y) on a thread from (tx, ty): what 0 a small skull showing at its top,
 * 1 a bat's wing out of its side, 2 a rat's tail curling from its foot */
static void silk_husk(float x, float y, float tx, float ty, int what, int s) {
    cv_line(tx, ty, x, y - 4.0f, sh(R_IRON, 0.62f));
    if (what == 1) {                                                 /* the wing, behind it */
        float p[8] = { x + (float)s * 1.0f, y - 2.0f, x + (float)s * 6.5f, y - 5.5f, x + (float)s * 5.0f, y - 0.5f, x + (float)s * 1.0f, y + 1.0f };
        cv_poly(p, 4, R_IRON, 0.05f, 0.2f, y - 5.5f, 7.0f);
        cv_line(x + (float)s * 1.0f, y - 2.0f, x + (float)s * 6.5f, y - 5.5f, sh(R_IRON, 0.35f));
    }
    for (int j = -6; j <= 6; j++) {                                  /* a spindle, pointed at both ends */
        float u = (float)j / 6.5f, w = 2.9f * powf(fmaxf(0.0f, 1.0f - u * u), 0.8f);
        for (int i = (int)floorf(x - w); i <= (int)ceilf(x + w); i++) {
            float nx = ((float)i + 0.5f - x) / (w + 0.01f);
            if (fabsf(nx) > 1.0f) continue;
            float v = 0.3f + 0.45f * lit(nx, u, sqrtf(fmaxf(0.0f, 1.0f - nx * nx))), band = fmodf((float)j + 0.4f * nx * 3.0f + 20.0f, 2.6f);
            cv_px(i, (int)y + j, sh(R_BONE, band < 0.8f ? v - 0.2f : v));   /* wound round, slantwise */
        }
    }
    if (what == 0) {                                                 /* a small skull, its crown and sockets */
        cv_px((int)x - 1, (int)y - 4, sh(R_BONE, 0.8f)); cv_px((int)x, (int)y - 5, sh(R_BONE, 0.85f)); cv_px((int)x + 1, (int)y - 4, sh(R_BONE, 0.7f));
        cv_px((int)x - 1, (int)y - 3, sh(R_BONE, 0.05f)); cv_px((int)x + 1, (int)y - 3, sh(R_BONE, 0.05f));
    }
    if (what == 2)                                                   /* a rat's tail */
        for (int k = 0; k < 6; k++) cv_px((int)(x + sinf((float)k * 0.9f) * 1.5f), (int)(y + 4.0f + (float)k), sh(R_SKIN, 0.3f - 0.03f * (float)k));
}

/* a strand of her torn web trailing from (x0, y0) towards (x1, y1), sagging; torn: how much of it is gone (it frays at
 * the break). Drawn after the outline, and only where nothing of her is: it is behind her, and thin */
static void web_trail(float x0, float y0, float x1, float y1, float sag, float torn) {
    int n = (int)hypotf(x1 - x0, y1 - y0) + 1, end = (int)((float)n * (1.0f - torn));
    for (int i = 0; i <= end; i++) {
        float t = (float)i / (float)n;
        int x = (int)(x0 + (x1 - x0) * t), y = (int)(y0 + (y1 - y0) * t + sag * 4.0f * t * (1.0f - t));
        if (i > end - 4 && (i & 1)) continue;
        if ((unsigned)x < (unsigned)TS && (unsigned)y < (unsigned)TS && cv_sprite[y * TS + x] == C_CLEAR) cv_sprite[y * TS + x] = sh(R_IRON, 0.62f - 0.18f * t);
    }
}

/* her leg l (0 the front pair to 3 the hind) on side s in a frame of spr_bspider; hy the head's middle. The hind pair is
 * drawn behind her abdomen, the rest before it; her right hind leg is snapped */
static void matriarch_leg(int frame, int s, int l, float hy) {
    static const float KX[4] = { 14.5f, 21.0f, 27.0f, 19.0f }, KY[4] = { 27, 8, 23, 12 }, FX[4] = { 12.5f, 26.0f, 27.5f, 27.5f }, FY[4] = { 62, 62, 61, 57 };
    static const float LKX[4] = { 15, 19, 23, 21 }, LKY[4] = { 26, 9, 10, 16 }, LFX[4] = { 9, 24, 31, 29 }, LFY[4] = { 6, 1, 8, 40 };
    static const float RKX[2] = { 15, 19 }, RKY[2] = { 22, 8 }, RFX[2] = { 10, 26 }, RFY[2] = { 2, 1 };
    float k = (frame == 1 ? (l & 1 ? 1.0f : -1.0f) : (frame == 0 ? (l & 1 ? -1.0f : 1.0f) : 0.0f)) * (float)s;
    float ax = 32 + s * (8.5f + l * 0.6f), ay = hy + 1.0f - l * 1.6f;
    float kx = 32 + s * KX[l], ky = KY[l] + k * 2.0f, fx = 32 + s * FX[l] + k, fy = FY[l];
    if (frame == 2) { kx = 32 + s * LKX[l]; ky = LKY[l]; fx = 32 + s * LFX[l]; fy = LFY[l]; }   /* the leap: the front legs flung up to take hold */
    if (frame == 3 && l < 2) { kx = 32 + s * RKX[l]; ky = RKY[l]; fx = 32 + s * RFX[l]; fy = RFY[l]; }   /* rearing: the front two pairs raised */
    spurred_leg(ax, ay, kx, ky, fx, fy, l == 1 || l == 2 ? 1.75f : 1.6f, s, s > 0 && l == 3, 711 + (uint32_t)(l * 2 + s + 1));
}

/* The Wandering Spider: the hunting spider's kind grown huge and grizzled, its fur gone grey-white, a cluster of milky
 * blind eyes round two great burning ones, its shell scarred, its legs spurred, torn web trailing from it with the husks
 * of its prey in it, and the skull on its back in raised bone. frame 0-1 walking (the renderer turns them over on the
 * ceiling), 2 the leap, 3 rearing to spit its web, 4 dead on its back. Fur R_WARM, drawn as it is (the boss has no rank). */
static void spr_bspider(int id, int frame) {
    cv_begin(cv_sprite, TS, TS);
    if (frame == 4) {                                                /* dead on her back, her legs curled over her */
        grizzled(32, 52, 17.0f, 8.5f, 0.12f, 0.55f, 0.3f, 741);
        for (int s = -1; s <= 1; s += 2)
            for (int l = 0; l < 4; l++)
                spurred_leg(32 + s * (5 + l * 2.5f), 47, 32 + s * (13 + l * 3.0f), 34 - l * 3.0f, 32 + s * (6 + l * 2.0f), 27 - l * 2.0f, 1.4f, s, s > 0 && l == 3, 743 + (uint32_t)(l * 2 + s + 1));
        grizzled(32, 46, 10.0f, 5.5f, 0.08f, 0.45f, 0.3f, 745);
        for (int k = -1; k <= 1; k += 2) cv_flat(32 + k * 2.5f, 45, 1.6f, 1.2f, (uint8_t)(R_BLOOD + 4));   /* her eyes gone out */
        cv_outline(0);
        web_trail(18, 56, 2, 62, 0.5f, 0.0f); web_trail(46, 57, 63, 61, 0.4f, 0.1f); web_trail(40, 59, 52, 63, 0.2f, 0.0f);
        cv_store(id);
        return;
    }
    float hy = frame == 3 ? 41.0f : 47.0f, ay = frame == 3 ? 21.0f : 24.0f;   /* the head's middle and the abdomen's */
    if (frame != 2) cv_flat(32, 62, 21, 2.0f, R_WARM + 1);         /* her shadow, under her body (the side legs reach past it) */
    for (int sd = -1; sd <= 1; sd += 2) matriarch_leg(frame, sd, 3, hy);   /* the hind pair, behind her */
    grizzled(32, ay, 15.5f, 12.5f, 0.05f, 0.5f, 0.55f, 701);         /* the abdomen */
    for (int k = 0; k < 3; k++)                                      /* claws raked across it once, long healed */
        for (int i = 0; i < 9; i++) {
            int x = 38 + i, y = (int)(ay - 9.0f) + k * 3 + (i * 2) / 3;
            cv_px(x, y, sh(R_WARM, 0.78f - 0.04f * (float)i)); cv_px(x, y + 1, sh(R_WARM, 0.02f));
        }
    for (int i = 0; i < 10; i++) {                                    /* and a gouge down the left, the shell bare about it */
        int x = 20 + i / 3, y = (int)ay - 3 + i;
        cv_px(x - 1, y, sh(R_IRON, 0.25f)); cv_px(x, y, sh(R_WARM, 0.8f)); cv_px(x + 1, y, sh(R_IRON, 0.12f));
    }
    for (int y = 0; y < SKULL_H; y++)                                 /* the skull on its back, raised: its shadow first */
        for (int x = 0; x < SKULL_W; x++)
            if (SKULL_FRONT[y][x] != '.') cv_px(32 - SKULL_W / 2 + x + 1, (int)ay - 9 + y + 1, sh(R_WARM, 0.0f));
    skull_stamp(32, ay - 1.0f, 0, 0, 0);
    for (int l = 2; l >= 0; l--)                                     /* the rest before her, the back ones first */
        for (int sd = -1; sd <= 1; sd += 2) matriarch_leg(frame, sd, l, hy);
    for (int y = 0; y < TS; y++)                                     /* the fur's grain over legs and body */
        for (int x = 0; x < TS; x++) {
            uint8_t *c = &cv_sprite[y * TS + x];
            if (*c <= R_WARM + 1 || *c >= R_WARM + 16) continue;
            float n = h01(x, y, 731);
            if (n < 0.15f) *c = (uint8_t)(*c - 1 - (n < 0.05f)); else if (n > 0.9f && *c < R_WARM + 14) *c = (uint8_t)(*c + 1);
        }
    if (frame != 3) {                                                /* the husks of her prey, hung in the web she trails */
        silk_husk(7.0f, 34.0f, 6.0f, 20.0f, 0, -1);
        silk_husk(57.5f, 38.0f, 57.0f, 24.0f, 1, 1);
    }
    grizzled(32, hy, 12.5f, 9.5f, 0.06f, 0.5f, 0.4f, 721);          /* the head */
    grid_stamp(MATRIARCH_FACE, 21, 12, 22, (int)hy - 8, R_WARM, R_GHOST, (uint8_t)(R_FLAME + 15), 0);
    float open = frame == 2 || frame == 3 ? 1.0f : 0.0f;
    for (int k = -1; k <= 1; k += 2) {                              /* palps, her jaws' great bulges, and the fangs */
        cv_capsule(32 + k * 9.0f, hy + 3.0f, 32 + k * (10.5f + open), hy + 11.0f, 1.4f, R_WARM, 0.1f, 0.45f);
        cv_px((int)(32 + k * (10.5f + open)), (int)hy + 12, sh(R_IRON, 0.6f));
        cv_ellipse(32 + k * (3.4f + open), hy + 7.0f, 3.2f, 4.0f, R_WARM, 0.06f, 0.42f);
        for (int q = 0; q < 3; q++) cv_px((int)(32 + k * (2.5f + open + q)), (int)hy + 5 + q * 2, sh(R_IRON, 0.62f));   /* grey hair on them */
        float px = 0.0f, py = 0.0f;
        for (int j = 0; j <= 5; j++) {
            float t = (float)j / 5.0f;
            px = 32 + k * (3.6f + open * 2.0f - t * (1.8f + open) + sinf(t * PI_F) * 0.9f); py = hy + 10.0f + (float)j;
            cv_px((int)px, (int)py, sh(R_IRON, 0.14f + 0.38f * t));
        }
        cv_px((int)px, (int)py + 1, (uint8_t)(R_MOSS + 14)); cv_px((int)px, (int)py + 2, (uint8_t)(R_MOSS + 9));   /* venom (no glint) */
    }
    if (frame == 3) {                                                /* the web gathering at her jaws */
        cv_ellipse(32, hy + 15.0f, 2.6f, 2.2f, R_GHOST, 0.5f, 0.9f);
        for (int k = -1; k <= 1; k += 2) cv_line(32 + k * 2.0f, hy + 11.0f, 32 + k * 1.0f, hy + 14.0f, sh(R_GHOST, 0.75f));
    }
    cv_outline(0);
    if (frame != 3) {                                                /* the torn web she trails, hanging from her */
        web_trail(17, ay + 8.0f, 2, 60, 1.5f, 0.0f);
        web_trail(19, ay + 10.0f, 9, 63, 0.5f, 0.25f);
        web_trail(47, ay + 8.0f, 63, 58, 1.2f, 0.1f);
        web_trail(45, ay + 11.0f, 56, 63, 0.6f, 0.0f);
        web_trail(6, 20, 1, 44, 0.8f, 0.3f);
        web_trail(57, 24, 63, 45, 0.6f, 0.2f);
    }
    cv_store(id);
}

/* her abdomen: the web spider's pitted shell stretched over eggs that press pale through the skin, each with a dark
 * rim where the skin is drawn tight, veins running between them, her mark at the front */
static void brood_abdomen(float cx, float cy, float rx, float ry) {
    spider_abdomen(cx, cy, rx, ry, R_IRON, 511);
    for (int i = 0; i < 16; i++) {                                   /* eggs pressing through */
        float a = h01(i, 1, 411) * TAU_F, r = 0.75f * sqrtf(h01(i, 2, 411));
        float ex = cx + cosf(a) * r * rx, ey = cy + sinf(a) * r * ry * 0.85f, er = 2.2f + 0.6f * h01(i, 3, 411);
        for (int y = (int)(ey - er) - 1; y <= (int)(ey + er) + 1; y++)
            for (int x = (int)(ex - er) - 1; x <= (int)(ex + er) + 1; x++) {
                float d = hypotf(x + 0.5f - ex, y + 0.5f - ey) / er;
                if (d > 1.25f) continue;
                if (d > 1.0f) { if (y + 0.5f > ey) cv_px(x, y, sh(R_IRON, 0.02f)); continue; }   /* the skin drawn tight under it */
                float lit = (1.0f - d) * 0.5f + ((x + 0.5f < ex && y + 0.5f < ey) ? 0.2f : 0.0f);
                cv_px(x, y, sh(R_BONE, 0.12f + lit * 0.55f));
            }
    }
    for (int q = 0; q < 6; q++) {                                    /* veins between them */
        float x = cx + (h01(q, 5, 413) - 0.5f) * rx * 1.4f, y = cy - ry * 0.7f;
        for (int k = 0; k < (int)(ry * 1.3f); k++) {
            x += (h01(q, k, 415) - 0.5f) * 1.6f; y += 1.0f;
            if ((x - cx) * (x - cx) / (rx * rx) + (y - cy) * (y - cy) / (ry * ry) > 0.85f) break;
            uint8_t c = cv_sprite[(int)y * TS + (int)x];
            if (c >= R_IRON && c < R_IRON + 16) cv_px((int)x, (int)y, (uint8_t)(R_BLOOD + 3));
        }
    }
    for (int k = 0; k < 7; k++) {                                    /* her mark, an hourglass in blood */
        int w = k < 3 ? 3 - k : (k > 3 ? k - 3 : 0);
        for (int q = -w; q <= w; q++) cv_px((int)cx + q, (int)(cy + ry * 0.45f) + k, (uint8_t)(R_BLOOD + 8 + (q == 0) * 2));
    }
}

/* The brood mother: a black spider the size of a dog, her abdomen swollen with eggs that press pale through the skin,
 * veined between them; the web spider's eight eyes and pale fangs, thick banded legs. Body R_IRON (PF_CHITIN). pose
 * 0-1 walking, 2 laying (a sac coming down on its thread), 3 dead. */
static void spr_brood(int id, int frame) {
    cv_begin(cv_sprite, TS, TS);
    float cx = 32, cy = frame == 3 ? 52 : 42, ab = frame == 2 ? -6.0f : 0.0f;
    if (frame == 3) {                                                /* dead on her back, her legs curled over her */
        brood_abdomen(cx, cy - 2, 15.0f, 9.0f);
        for (int s = -1; s <= 1; s += 2)
            for (int l = 0; l < 4; l++)
                chitin_leg(cx + s * (4 + l * 1.5f), cy - 8, cx + s * (10 + l * 2), cy - 14 - l * 2, cx + s * (5 + l), cy - 20 - l, 1.8f, R_IRON, 0.05f, 0.35f, -1, 517 + (uint32_t)(l * 2 + s + 1));
        cv_ellipse(cx, cy - 9, 8.0f, 5.0f, R_IRON, 0.06f, 0.35f);
        cv_outline(0);
        cv_store(id);
        return;
    }
    cv_flat(32, 62, 28, 2.2f, R_WARM + 1);
    brood_abdomen(cx, cy - 12 + ab, 17.0f, 13.0f);                  /* the abdomen, raised behind her */
    for (int s = -1; s <= 1; s += 2)
        for (int l = 3; l >= 0; l--) {
            float k = (frame == 1 ? (l & 1 ? 1.0f : -1.0f) : (frame == 0 ? (l & 1 ? -1.0f : 1.0f) : 0.0f)) * s;
            float kx = cx + s * (7 + l * 1.5f), ky = cy + l * 2.0f;
            float mx = cx + s * (18 + l * 2.5f), my = cy - 14 + l * 3.5f + k * 2.0f;
            float fx = cx + s * (24 + l * 2.0f) + k, fy = 61;
            chitin_leg(kx, ky, mx, my, fx, fy, 2.0f, R_IRON, 0.05f, 0.42f, -1, 521 + (uint32_t)(l * 2 + s + 1));
        }
    cv_ellipse(cx, cy + 3, 9.0f, 7.0f, R_IRON, 0.06f, 0.45f);     /* head */
    grid_stamp(SPIDER_HEAD, 13, 8, (int)cx - 6, (int)cy - 1, R_IRON, R_IRON, (uint8_t)(R_FLAME + 14), 0);
    spider_fangs(cx, cy + 7, 5.0f, 0);
    if (frame == 2) {                                               /* a sac coming down on its thread, before her jaws */
        cv_line(cx + 6, cy + 4, cx + 6, cy + 12, sh(R_STONE, 0.8f));
        cv_ellipse(cx + 6, cy + 16, 5.0f, 5.5f, R_BONE, 0.35f, 0.85f);
        for (int k = -4; k <= 4; k += 2) cv_line(cx + 1.5f, cy + 16 + k, cx + 10.5f, cy + 16 + k * 0.6f, sh(R_BONE, 0.95f));   /* wound round */
    }
    cv_outline(0);
    cv_store(id);
}

/* the brood mother's dressing by rank: II silk and mould, III frost and her shell cracked, IV old blood on her fangs and
 * an egg burst, its yolk run, V charred and the pale skull on her back, gilded gold in her bands and along her shell */
static void brood_dress(int id, int frame) {
    float cx = 32, cy = 42, ab = frame == 2 ? -6.0f : 0.0f, ay = cy - 12 + ab;
    for (int r = RK_II; r < NRANK; r++) {
        g_dress_ramp2 = r == RK_V ? R_BLOOD : R_BONE;               /* her eggs and fangs; at V over her mark and eggs too */
        g_dress_ramp3 = r == RK_V ? R_BONE : -1;
        dress_begin(id);
        if (r == RK_II) {
            kit_moss(0.25f, 0, 531);
            for (int q = 0; q < 5; q++) {
                float x0 = 8 + 48 * h01(q, 1, 533), y0 = 22 + 12 * h01(q, 2, 533), x1 = x0 + (h01(q, 3, 533) - 0.5f) * 16, y1 = y0 + 8 + 10 * h01(q, 4, 533);
                for (int k = 0; k <= 12; k++) { float t = k / 12.0f; int x = (int)(x0 + (x1 - x0) * t), y = (int)(y0 + (y1 - y0) * t + sinf(t * PI_F) * 2.0f); if (base_on(x, y) || (k & 1)) cv_px(x, y, sh(R_GHOST, 0.65f)); }
            }
        }
        if (r == RK_III) {
            kit_frost(0.6f, 535);
            float x = cx - 9, y = ay - 10;
            for (int k = 0; k < 14; k++) { cv_px((int)x, (int)y, 0); cv_px((int)x + 1, (int)y, sh(R_IRON, 0.5f)); x += h01(k, 1, 537) < 0.5f ? 1.0f : 0.0f; y += 1.0f; }
        }
        if (r >= RK_IV && r != RK_GILDED) {
            kit_blood(cx - 2.5f, cy + 8, 6, 2, 539); kit_blood(cx + 2.5f, cy + 8, 6, 2, 541);
            cv_flat(cx + 7, ay - 2, 3.0f, 2.6f, 0); cv_flat(cx + 7, ay - 2, 1.8f, 1.5f, (uint8_t)(R_GOLD + 3));   /* an egg burst */
            for (int k = 0; k < 6; k++) cv_px((int)cx + 7 + (k & 1), (int)ay + 1 + k, (uint8_t)(R_GOLD + 4 - k / 3));
        }
        if (r == RK_V) {
            kit_char(0.3f, 543);
            cv_flat(cx + 0.5f, ay + 0.8f, 6.2f, 6.6f, 0);
            grid_stamp(SKULL_SMALL, 9, 10, (int)cx - 4, (int)ay - 4, R_BONE, R_BONE, 0, 0);
        }
        if (r == RK_GILDED)
            for (int y = 0; y < TS; y++)
                for (int x = 0; x < TS; x++) {
                    uint8_t b = base_c(x, y);
                    if (b >= R_IRON + 6 && b < R_IRON + 16 && h01(x, y, 545) < 0.8f) cv_px(x, y, (uint8_t)(R_GOLD + clampi(b - R_IRON, 4, NGOLD - 1)));
                    else if (b >= R_IRON + 4 && b < R_IRON + 6 && !base_on(x, y - 1)) cv_px(x, y, (uint8_t)(R_GOLD + 6));
                }
        dress_store(id, r, R_IRON);
    }
    g_dress_ramp2 = g_dress_ramp3 = -1;
}

/* an egg sac, slung from the wall: silk wound round and round it in strands, the young pressed dark against it inside,
 * still, then stirring (their legs at the silk, a few eyes catching), then split and empty, torn lips round a hollow */
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
        for (int i = 0; i < 14; i++) {                              /* its lips, torn outwards */
            float a = h01(i, 2, 423) * TAU_F, l = 0.2f + 0.25f * h01(i, 3, 423);
            cv_line(cx + cosf(a) * r * 0.55f, cy + sinf(a) * r * 0.75f, cx + cosf(a) * r * (0.75f + l), cy + sinf(a) * r * (0.95f + l), sh(R_BONE, 0.9f));
        }
        for (int i = 0; i < 3; i++) cv_line(cx - 2 + i * 2, cy + r, cx - 1 + i * 2.5f, cy + r + 6 + i * 2, sh(R_BONE, 0.5f));   /* strands trailing */
        cv_outline(0); cv_store(id); return;
    }
    cv_ellipse(cx, cy, r, r * 1.1f, R_BONE, 0.3f, 0.92f);
    for (int y = (int)(cy - r * 1.1f); y <= (int)(cy + r * 1.1f); y++)   /* the young pressed dark against it, inside */
        for (int x = (int)(cx - r); x <= (int)(cx + r); x++) {
            uint8_t *p = &cv_sprite[y * TS + x];
            if (*p < R_BONE || *p >= R_BONE + 16) continue;
            float n = fbm(x / 3.0f + frame * 2.3f, y / 3.0f, 32, 425);
            if (n > 0.6f) *p = sh(R_IRON, 0.18f + 0.25f * (1.0f - n));
            else if (n > 0.55f) *p = sh(R_BONE, 0.25f);             /* the silk's shadow round them */
        }
    for (int k = 0; k < 11; k++) {                                  /* silk wound round it, strand on strand */
        float y0 = cy - r * 1.0f + k * r * 0.2f, tilt = 0.35f + 0.1f * h01(k, 1, 427);
        for (int x = (int)(cx - r); x <= (int)(cx + r); x++) {
            float dx = (x + 0.5f - cx) / r, y = y0 + dx * r * tilt;
            if (dx * dx + ((y - cy) / (r * 1.1f)) * ((y - cy) / (r * 1.1f)) > 0.95f) continue;
            if (((x + k) % 5) == 0) continue;                       /* it breaks off here and there */
            cv_px(x, (int)y, sh(R_BONE, 0.82f + 0.12f * (1.0f - dx * dx)));
        }
    }
    if (frame == 1)
        for (int i = 0; i < 6; i++) {                               /* stirring: legs at the silk, and eyes */
            float a = h01(i, 4, 429) * TAU_F, rr = r * 0.85f;
            cv_line(cx + cosf(a) * rr * 0.5f, cy + sinf(a) * rr * 0.5f, cx + cosf(a) * rr, cy + sinf(a) * rr * 1.05f, sh(R_IRON, 0.12f));
            if (i < 4) cv_px((int)(cx - 6 + i * 4), (int)(cy - 3 + (i & 1) * 5), (uint8_t)(R_FLAME + 3));
        }
    cv_outline(0);
    cv_store(id);
}

/* spiderlings: drawn bold, to be shown at a fifth of the size: banded legs, its mother's mark, two great eyes and the
 * small ones beside them, pale fang tips */
static void spr_ling(int id, int frame) {
    cv_begin(cv_sprite, TS, TS);
    for (int s = -1; s <= 1; s += 2)
        for (int l = 0; l < 4; l++) {
            float k = (frame ? (l & 1 ? 1.0f : -1.0f) : (l & 1 ? -1.0f : 1.0f)) * s * 3.0f;
            float kx = 32 + s * (20 + l * 2), ky = 30 + l * 6 + k, fx = 32 + s * (26 + l * 1.5f);
            cv_capsule(32 + s * 6, 40 + l * 3, kx, ky, 2.4f, R_IRON, 0.05f, 0.4f);
            cv_capsule(kx, ky, fx, 60, 2.0f, R_IRON, 0.05f, 0.35f);
            cv_ellipse(kx, ky, 2.6f, 2.6f, R_IRON, 0.35f, 0.6f);    /* a pale band at the knee */
            cv_ellipse((kx + fx) * 0.5f, (ky + 60) * 0.5f, 2.2f, 2.2f, R_IRON, 0.3f, 0.55f);
        }
    cv_ellipse(32, 38, 12, 10, R_IRON, 0.06f, 0.45f);
    for (int k = 0; k < 5; k++) { int w = k < 2 ? 2 - k : k - 2; for (int q = -w; q <= w; q++) cv_px(32 + q, 32 + k, (uint8_t)(R_BLOOD + 8)); }   /* its mother's mark */
    cv_ellipse(32, 48, 8, 7, R_IRON, 0.08f, 0.5f);
    for (int k = -1; k <= 1; k += 2) {
        cv_flat(32 + k * 3.5f, 47, 2.2f, 2.2f, (uint8_t)(R_FLAME + 5)); cv_eye_mark((int)(32 + k * 3.5f), 47);
        cv_flat(32 + k * 6.5f, 45, 1.0f, 1.0f, (uint8_t)(R_FLAME + 2));
        cv_line(32 + k * 2.0f, 53, 32 + k * 1.0f, 56, sh(R_BONE, 0.45f));   /* fang tips */
    }
    cv_outline(0);
    cv_store(id);
}

/* a long thin hand at hx, hy: a palm gone grey, four fingers of three joints each curling as they go,
 * knuckles catching the light, nails rimed with frost; a reaches the way the fingers point, sc sizes it */
static void wraith_hand(float hx, float hy, float a, int s, float sc) {
    cv_ellipse(hx, hy, 1.9f * sc, 1.6f * sc, R_IRON, 0.25f, 0.5f);
    for (int f = 0; f < 4; f++) {
        float ang = a + (f - 1.5f) * 0.3f, x0 = hx, y0 = hy, L = (4.2f + (f == 1 || f == 2) * 0.9f) * sc;
        for (int j = 0; j < 3; j++) {
            float x1 = x0 + cosf(ang) * L, y1 = y0 + sinf(ang) * L;
            cv_line(x0, y0, x1, y1, sh(R_IRON, 0.48f - j * 0.06f));
            if (j < 2) cv_px((int)x1, (int)y1, sh(R_IRON, 0.72f));       /* a knuckle */
            x0 = x1; y0 = y1; ang += 0.24f * s; L *= 0.82f;
        }
        cv_px((int)x0, (int)y0, sh(R_GHOST, 0.85f));                    /* the nail, frosted */
    }
    float ta = a - 0.95f * s;                                        /* the thumb, from the side of the palm */
    cv_line(hx, hy, hx + cosf(ta) * 3.5f * sc, hy + sinf(ta) * 3.5f * sc, sh(R_IRON, 0.42f));
}

/* The Wraith (docs/InitialArtPass.md, section 8): a peaked hood over a void that is nothing but two lights, a body of
 * black cerements wound about it and torn through, coming loose below into strips that end in wisps, rags hanging from
 * the sleeves, long thin hands. Drawn to be shown half as tall again as a man; off the rank ladder. pose 0-1 gliding,
 * 2 reaching for you, 3 driven back, its arms over its face. */
static void spr_wraith_big(int id, float ph, int pose) {
    cv_begin(cv_sprite, TS, TS);
    float flare = pose == 3 ? 1.0f : 0.0f;
    for (int y = 6; y < TS; y++)                                    /* the body, wound in its cerements */
        for (int x = 0; x < TS; x++) {
            float dx = x + 0.5f - 32.0f, t = (y - 6) / 57.0f;
            float hw = (y < 16 ? 9.0f + (y - 6) * 0.55f : 14.5f + (y - 16) * (0.12f + 0.12f * flare)) + sinf(y * 0.21f + ph) * 0.8f * t;
            if (fabsf(dx) > hw) continue;
            if (y > 40) {                                            /* below, coming loose: strips, each its own length */
                float sw = sinf(ph + y * 0.15f) * (y - 40) * 0.06f, at = x + 64.0f - sw;
                int sid = (int)floorf(at / 3.0f);
                float within = at - sid * 3.0f, end = 49.0f + 12.0f * h01(sid, 7, 511) + 2.0f * sinf(ph * 1.3f + sid);
                if (y > end || (y > end - 3.0f && (within < 0.9f || within > 2.1f))) continue;   /* its pointed end */
                if (y > 44.0f + 6.0f * h01(sid, 9, 511) && within < 0.6f) continue;   /* parted from the next */
            }
            float u = dx / hw, v = 0.07f + 0.13f * (1.0f - u * u) - t * 0.05f;
            float b = (y + dx * 0.28f - u * u * 3.0f + 3.0f * (fbm(x / 7.0f, y / 7.0f, 16, 521) - 0.45f)) / 4.8f, fb = b - floorf(b);   /* the wrappings, wound round it, loosely */
            if (fb < 0.13f) { cv_px(x, y, 0); continue; }           /* the black between two turns */
            if (fb < 0.23f && fabsf(u) > 0.35f && h01((int)floorf(b), 1, 523) < 0.45f) { cv_px(x, y, sh(R_GHOST, 0.1f + 0.06f * fabsf(u))); continue; }   /* an edge turning to the cold */
            if (fb > 0.45f && fb < 0.72f) v += 0.07f;
            if (sinf(dx * 0.75f + sinf(y * 0.08f + ph) * 1.2f) > 0.82f) v -= 0.04f;   /* deep folds */
            if (y > 18 && fabsf(u) < 0.8f && fbm(x / 3.5f, y / 3.5f, 32, 513) > 0.7f) { cv_px(x, y, 0); continue; }   /* torn through */
            if (fabsf(u) > 0.88f) { cv_px(x, y, sh(R_GHOST, 0.22f + 0.3f * (fabsf(u) - 0.88f) / 0.12f)); continue; }   /* the cold rim */
            cv_px(x, y, sh(R_IRON, v));
        }
    for (int sid = 0; sid < 43; sid++) {                            /* wisps curling off the strips' ends */
        float end = 49.0f + 12.0f * h01(sid, 7, 511) + 2.0f * sinf(ph * 1.3f + sid);
        float cx = sid * 3.0f - 64.0f + 1.5f + sinf(ph + end * 0.15f) * (end - 40.0f) * 0.06f;
        float hw = 14.5f + (end - 16.0f) * (0.12f + 0.12f * flare);
        if (fabsf(cx + 0.5f - 32.0f) > hw - 1.0f) continue;
        for (int k = 1; k <= 5; k += 2) {
            int wx = (int)(cx + sinf(ph * 1.7f + sid + k * 0.9f) * k * 0.5f), wy = (int)(end + k * 1.2f);
            if (wy < TS && cv_sprite[wy * TS + wx] == C_CLEAR && h01(sid, k, 525) < 0.6f) cv_px(wx, wy, sh(R_GHOST, 0.3f - k * 0.04f));
        }
    }
    for (int y = 0; y < 25; y++)                                    /* the hood: high and peaked, falling to the shoulders */
        for (int x = 14; x < 50; x++) {
            float dx = x + 0.5f - 32.0f, ad = fabsf(dx);
            float hw = y < 10 ? (y + 0.6f) * 1.0f : 10.6f + (y - 10) * 0.32f;
            if (ad > hw) continue;
            float lip = 20.0f + 4.0f * h01((int)(dx + 40.0f) / 2, 3, 517) + (ad > 9.0f ? 1.5f : 0.0f);   /* ragged over the shoulders */
            if (y > lip) continue;
            float u = dx / hw, v = 0.08f + 0.16f * (1.0f - u * u);
            if (sinf(atan2f(dx, y + 3.0f) * 10.0f) > 0.6f) v -= 0.07f;   /* folds from the peak */
            if (fabsf(u) > 0.84f || y < 2) { cv_px(x, y, sh(R_GHOST, 0.3f + (y < 2) * 0.1f)); continue; }
            cv_px(x, y, sh(R_IRON, v));
        }
    for (int y = 4; y < 21; y++)                                    /* its mouth: a lancet of nothing */
        for (int x = 24; x < 41; x++) {
            float ad = fabsf(x + 0.5f - 32.0f);
            float w = y < 10 ? 5.4f * powf(fmaxf(0.0f, (y - 4.0f) / 6.0f), 0.7f) : 5.4f - (y > 17 ? (y - 17) * 0.9f : 0.0f);
            if (ad <= w) cv_px(x, y, 0);
            else if (ad <= w + 1.0f) cv_px(x, y, sh(R_IRON, 0.0f));   /* the lining, a shade off black */
        }
    float ey = 12.5f, gap = pose == 2 ? 3.0f : 2.5f;                /* two lights, and nothing else */
    for (int k = -1; k <= 1; k += 2) {
        int ex = (int)(32 + k * gap - (k < 0));
        if (pose == 3) { cv_eye(ex, (int)ey, (uint8_t)(R_GLOW + 12)); cv_px(ex + k, (int)ey, (uint8_t)(R_GLOW + 9)); continue; }   /* narrowed */
        cv_eye(ex, (int)ey, C_WHITE); cv_px(ex, (int)ey + 1, (uint8_t)(R_GLOW + 13));
        cv_px(ex + k, (int)ey, (uint8_t)(R_GLOW + 11)); cv_px(ex, (int)ey - 1, (uint8_t)(R_GLOW + 9));
        if (pose == 2) { cv_px(ex - k, (int)ey, (uint8_t)(R_GLOW + 11)); cv_px(ex, (int)ey + 2, (uint8_t)(R_GLOW + 8)); }
    }
    for (int s = -1; s <= 1; s += 2) {                              /* the arms: long sleeves, rags, longer fingers */
        float sx = 32 + s * 12.0f, sy = 18, hx, hy;
        if (pose == 3) { hx = 32 + s * 4.0f; hy = 9; }             /* over its face */
        else if (pose == 2 && s > 0) { hx = 45; hy = 22; }          /* reaching out at you */
        else { hx = 32 + s * 17.0f + sinf(ph + s) * 1.0f; hy = 43 + cosf(ph * 0.7f + s) * 1.5f; }
        float ex = (sx + hx) * 0.5f + s * 3.0f, ey2 = (sy + hy) * 0.5f;
        cv_capsule(sx, sy, ex, ey2, 4.0f, R_IRON, 0.06f, 0.3f);
        cv_capsule(ex, ey2, hx, hy - 2, 3.2f, R_IRON, 0.06f, 0.28f);
        cv_line(ex + s * 3.5f, ey2, hx + s * 3.0f, hy - 2, sh(R_GHOST, 0.38f));   /* the rim catching the sleeve */
        cv_line(ex - s * 3.0f, ey2 + 1.0f, hx - s * 2.6f, hy - 1, 0);   /* and its shadow on the body */
        for (int j = 0; j < 3; j++) {                               /* rags hanging from the sleeve */
            float k01 = 0.2f + 0.28f * j, px = ex + (hx - ex) * k01, py = ey2 + (hy - 2 - ey2) * k01 + 2.0f;
            int len = 6 + (int)(6.0f * h01(j, s + 2, 519));
            for (int q = 0; q < len; q++) {
                float rx = px + sinf(ph * 1.4f + q * 0.3f + j) * q * 0.14f;
                cv_px((int)rx, (int)(py + q), sh(R_IRON, 0.1f - q * 0.008f));
                if (q < len / 2) cv_px((int)rx + 1, (int)(py + q), sh(R_IRON, 0.05f));
            }
            cv_px((int)(px + sinf(ph * 1.4f + len * 0.3f + j) * len * 0.14f), (int)(py + len + 1), sh(R_GHOST, 0.32f));   /* a wisp off its end */
        }
        float sc = pose == 2 && s > 0 ? 1.5f : 1.0f, base = pose == 3 ? -1.57f - s * 0.4f : (pose == 2 && s > 0 ? -0.3f : 1.57f + s * 0.15f);
        wraith_hand(hx, hy, base, s, sc);
    }
    for (int i = 0; i < 12; i++) {                                  /* frost falling from it */
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

/* The ember imp (docs/InitialArtPass.md section 8): a sinewy, soot-dark thing still cooling, its crust (R_BRICK, PF_CLAY)
 * split by fissures of fire; a narrow head with a long muzzle, horns swept back (the right one broken, an ember in its
 * stump), torn bat's ears, slit eyes and a grin full of needles lit from inside; wiry arms to the knee with talons,
 * reverse-jointed legs, ragged wings behind, a tail with an ember at its tip. The fire keeps burning whatever its rank.
 * pose 0-1 hopping, 2 throwing, 3 crumbled. */
static const char *const IMP_HEAD[17] = {
    ".N...............", ".Mr..............", "..qr..........J..", "...pq........qo..", "4...op23F32po...4", "34.23345G54332.43",
    ".45234566G6532.54", "3453456a8a6543543", ".3424896769842.43", "..31K@A676A@K13..", "...2H5606065H2...", "...13678987631...",
    "...2u0uJJJu0u2...", "....3LvLKLvL3....", ".....3689863.....", "......36863......", ".......242.......",
};
static const char *const IMP_SIDE[15] = {
    "N..............", "Mq.............", ".pq............", "..pq.2343......", "4..op346F42....", "45.2345G65432..",
    ".56345678a7542.", "..4568aK@A6764.", "...35789879877.", "...2468998a9H96", "....357H7896580", "....24uJuJuJu4.",
    ".....3vLvLvL3..", "......3678763..", ".......24642...",
};
static void imp_crust(float y0, float y1, int cracks, uint32_t seed) {   /* the crust darkened unevenly, and split by fire */
    for (int y = 0; y < TS; y++)
        for (int x = 0; x < TS; x++) {
            uint8_t *p = &cv_sprite[y * TS + x];
            if (*p <= R_BRICK || *p >= R_BRICK + 16) continue;
            float n = fbm(x / 3.0f, y / 3.0f, 16, seed);
            if (n < 0.45f) *p = (uint8_t)(*p - 2 < R_BRICK + 1 ? R_BRICK + 1 : *p - 2);
        }
    for (int k = 0; k < cracks; k++) {
        float x = 22 + 20 * h01(k, 1, seed), y = y0 + (y1 - y0) * h01(k, 2, seed), a = h01(k, 3, seed) * TAU_F;
        for (int i = 0; i < 9; i++) {
            a += (h01(k, i + 4, seed) - 0.5f) * 1.3f; x += cosf(a); y += sinf(a);
            if (x < 0 || y < 0 || x >= TS || y >= TS) break;
            uint8_t c = cv_sprite[(int)y * TS + (int)x];
            if (c > R_BRICK && c < R_BRICK + 16) cv_px((int)x, (int)y, (uint8_t)(R_FLAME + 6 + (int)(4.0f * (1.0f - fabsf(4.0f - (float)i) / 4.0f))));
        }
    }
}
static void spr_imp(int id, int pose) {
    cv_begin(cv_sprite, TS, TS);
    if (pose == 3) {                                               /* crumbled: crust and ash, the fire going out */
        cv_flat(32, 61.5f, 20, 2.5f, R_WARM + 1);
        for (int i = 0; i < 30; i++) {
            float x = 32 + (h01(i, 1, 521) - 0.5f) * 34, y = 52 + 10 * h01(i, 2, 521);
            cv_ellipse(x, y, 1.0f + 2.0f * h01(i, 3, 521), 1.0f + 1.4f * h01(i, 4, 521), i % 3 ? R_IRON : R_BRICK, 0.05f, 0.3f);
        }
        cv_capsule(20, 56, 12, 50, 1.0f, R_BONE, 0.2f, 0.6f);      /* a horn, fallen */
        for (int i = 0; i < 8; i++) cv_px((int)(20 + 24 * h01(i, 5, 521)), (int)(46 + 14 * h01(i, 6, 521)), (uint8_t)(R_FLAME + 5 + (i & 3)));
        cv_outline(0); cv_store(id); return;
    }
    float up = pose == 0 ? -3.0f : 0.0f, hy = 3.0f + up;            /* the head's top row */
    float wl = pose == 0 ? -4.0f : (pose == 2 ? -1.0f : 2.0f);      /* the wings up, or down */
    cv_flat(32, 62, 12, 1.6f, R_WARM + 1);
    for (int s = -1; s <= 1; s += 2) {                              /* the wings, behind: bone struts, the leather ragged */
        float bx = 32 + s * 5.0f, by = hy + 20.0f, wx = 32 + s * 21.0f, wy = hy + 5.0f + wl;
        float tips[3][2] = { { 32 + s * 30.0f, hy + 18.0f + wl * 0.5f }, { 32 + s * 26.0f, hy + 27.0f }, { 32 + s * 17.0f, hy + 30.0f } };
        for (int f = 0; f < 3; f++) {
            float m[8] = { wx, wy, tips[f][0], tips[f][1], f < 2 ? tips[f + 1][0] : bx, f < 2 ? tips[f + 1][1] : by + 6.0f, bx, by };
            cv_poly(m, 4, R_BRICK, 0.04f, 0.2f, wy, 30);
        }
        for (int f = 0; f < 3; f++)                                 /* scallops torn in the hem */
            cv_flat((tips[f][0] + (f < 2 ? tips[f + 1][0] : bx)) * 0.5f, (tips[f][1] + (f < 2 ? tips[f + 1][1] : by + 6.0f)) * 0.5f + 1.5f, 2.2f, 2.0f, C_CLEAR);
        cv_capsule(bx, by, wx, wy, 1.2f, R_BRICK, 0.1f, 0.4f);
        for (int f = 0; f < 3; f++) cv_capsule(wx, wy, tips[f][0], tips[f][1], 0.7f, R_BRICK, 0.08f, 0.35f);
        cv_px((int)wx, (int)wy - 1, sh(R_BONE, 0.6f));             /* the claw at the wing's wrist */
    }
    cv_capsule(35, hy + 33, 47, hy + 42, 1.2f, R_BRICK, 0.1f, 0.4f);   /* the tail, its ember */
    cv_capsule(47, hy + 42, 52, hy + 33, 1.0f, R_BRICK, 0.1f, 0.4f);
    cv_flat(52.5f, hy + 32, 1.6f, 1.8f, (uint8_t)(R_FLAME + 11)); cv_px(52, (int)hy + 31, (uint8_t)(R_FLAME + 15));
    for (int s = -1; s <= 1; s += 2) {                              /* legs, reverse-jointed, sprung */
        float hx = 32 + s * 4.0f, hipy = hy + 35.0f, kx = 32 + s * 9.0f, ky = 46 + up * 0.5f, ax = 32 + s * 6.0f, ay = 55.0f;
        cv_capsule(hx, hipy, kx, ky, 2.3f, R_BRICK, 0.1f, 0.5f);
        cv_capsule(kx, ky, ax, ay, 1.5f, R_BRICK, 0.1f, 0.45f);
        cv_capsule(ax, ay, 32 + s * 9.0f, 61, 1.2f, R_BRICK, 0.1f, 0.45f);
        for (int c = -1; c <= 1; c++) cv_line(32 + s * 9.0f, 61, 32 + s * 9.0f + s * 2.0f + c * 1.5f, 62, sh(R_BONE, 0.5f));
    }
    cv_ellipse(32, hy + 23, 6.5f, 5.5f, R_BRICK, 0.15f, 0.6f);     /* the chest, narrow, ribs under the crust */
    cv_ellipse(32, hy + 31, 4.5f, 5.0f, R_BRICK, 0.1f, 0.45f);     /* the waist, drawn in */
    for (int r = 0; r < 3; r++) for (int s = -1; s <= 1; s += 2) cv_line(32 + s * 1.5f, hy + 21 + r * 2.5f, 32 + s * 5.5f, hy + 22.5f + r * 2.5f, sh(R_BRICK, 0.08f));
    for (int s = -1; s <= 1; s += 2) {                              /* arms: long, wiry, to the knees; one up with a coal */
        float sx = 32 + s * 6.5f, sy = hy + 19.0f, ex, ey, hx2, hy2;
        if (pose == 2 && s > 0) { ex = 44; ey = hy + 13; hx2 = 46; hy2 = hy + 4; }
        else { ex = 32 + s * 12.0f; ey = hy + 28; hx2 = 32 + s * 11.0f; hy2 = hy + 37; }
        cv_capsule(sx, sy, ex, ey, 1.6f, R_BRICK, 0.15f, 0.55f);
        cv_capsule(ex, ey, hx2, hy2, 1.3f, R_BRICK, 0.15f, 0.5f);
        for (int c = -1; c <= 1; c++) cv_line(hx2, hy2, hx2 + c * 1.6f + s * 0.8f, hy2 + (pose == 2 && s > 0 ? -3.0f : 3.0f), sh(R_BONE, 0.6f));   /* talons */
        cv_ellipse(sx, sy, 2.4f, 2.0f, R_BRICK, 0.2f, 0.6f);        /* knobbed shoulders */
        if (pose == 2 && s > 0) { cv_flat(hx2 + 1, hy2 - 3, 3.0f, 3.0f, (uint8_t)(R_FLAME + 10)); cv_flat(hx2 + 1, hy2 - 3, 1.6f, 1.6f, (uint8_t)(R_FLAME + 14)); }
    }
    imp_crust(hy + 17, hy + 52, 8, 523);
    grid_stamp(IMP_HEAD, 17, 17, 24, (int)hy, R_BRICK, R_BONE, (uint8_t)(R_FLAME + 15), 0);
    cv_outline(0);
    cv_store(id);
}

/* the imp's dressing by rank: II a sulphurous mould and its wings torn, III frost on the crust and a chipped horn, IV the
 * fissures opened and a wound in its chest, V nearly all fire under an iron collar and its chain, gilded gold in every
 * crack (docs/InitialArtPass.md 6.1, section 8) */
static void imp_dress(int id) {
    for (int r = RK_II; r < NRANK; r++) {
        dress_begin(id);
        if (r == RK_GILDED) {
            for (int y = 0; y < TS; y++)
                for (int x = 0; x < TS; x++) {
                    uint8_t b = base_c(x, y);
                    if (b >= R_FLAME + 5 && b < R_FLAME + 13) cv_px(x, y, (uint8_t)(R_GOLD + clampi(b - R_FLAME, 6, NGOLD - 1)));   /* gold in the cracks */
                    else if (b > R_BRICK + 3 && b < R_BRICK + 16 && fbm(x / 3.0f, y / 3.0f, 16, 431) > 0.68f) cv_px(x, y, (uint8_t)(R_GOLD + 5));
                }
            kit_chain(26, 22, 38, 22, 1.5f, R_GOLD);
            dress_store(id, r, R_BRICK);
            continue;
        }
        for (int y = 0; y < TS && r >= RK_II; y++)                 /* torn wings: dark rents along the hem */
            for (int x = 0; x < TS; x++)
                if ((x < 22 || x > 42) && y < 36 && base_on(x, y) && !base_on(x, y + 1) && h01(x, y, 433) < 0.25f) { cv_px(x, y, 0); cv_px(x, y - 1, 0); }
        if (r == RK_II) kit_moss(0.2f, 0, 435);
        if (r == RK_III) kit_frost(0.65f, 437);
        if (r >= RK_III) { cv_px(26, 3, 0); cv_px(27, 4, 0); }     /* a chip out of the left horn */
        if (r >= RK_IV) {                                          /* the fissures opened, a wound in the chest */
            for (int k = 0; k < 6; k++) {
                float x = 26 + 12 * h01(k, 1, 439), y = 24 + 18 * h01(k, 2, 439), a = h01(k, 3, 439) * TAU_F;
                for (int i = 0; i < 7; i++) { a += (h01(k, i + 4, 439) - 0.5f) * 1.2f; x += cosf(a); y += sinf(a); if (base_on((int)x, (int)y)) cv_px((int)x, (int)y, (uint8_t)(R_FLAME + 9 + (i & 3))); }
            }
            cv_line(29, 25, 35, 28, (uint8_t)(R_FLAME + 14)); cv_line(29, 26, 35, 29, (uint8_t)(R_FLAME + 10));
        }
        if (r == RK_V) {                                           /* nearly all fire, an iron collar and its chain */
            for (int y = 0; y < TS; y++)
                for (int x = 0; x < TS; x++) {
                    uint8_t b = base_c(x, y);
                    if (b > R_BRICK && b < R_BRICK + 16 && fabsf(fbm(x / 3.5f, y / 3.5f, 16, 441) - 0.5f) < 0.06f) cv_px(x, y, (uint8_t)(R_FLAME + 5 + (int)(h01(x, y, 443) * 6.0f)));   /* veins of fire: the noise's ridges */
                }
            for (int x = 27; x <= 37; x++) { cv_px(x, 21, sh(R_IRON, 0.55f)); cv_px(x, 22, sh(R_IRON, 0.25f)); }
            kit_chain(36, 22, 41, 36, 2.0f, R_IRON);
        }
        dress_store(id, r, R_BRICK);
    }
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

/* The gargoyle (docs/InitialArtPass.md section 8): a grotesque in the walls' own stone (R_STONE, PF_STONE), weathered: a
 * heavy brow over deep-set eyes, a flat snout, a lipless snarl with tusks, the mouth a rainspout's; fin ears swept back,
 * bull's horns curving forward; slab shoulders, wings of bat's bones, torn. Rain has stained it from the eyes and mouth
 * down, lichen sits in its hollows. pose 0 perched on its plinth, asleep, its eyes blank stone (and the statues in the
 * alcoves are this); 1-2 flying, wings up and down, its cracks alight; 3 swooping, claws first; 4 broken. */
static const char *const GARG_FACE[17] = {
    "...4567d7654...", "..45799d99875..", ".356ab989ba653.", "36bcddc8cddcb63", "245677666776542", ".310@04340@013.",
    ".4628347438264.", "..57289998275..", "..46289a98264..", ".3462708072643.", ".3a635787536a3.", ".3e20d0d0d02e3.",
    ".2d000000000d2.", ".3c06b0b0b60c3.", "..4a789a987a4..", "...357898753...", "....2456542....",
};
static const char *const GARG_SIDE[15] = {
    "....45676.....", "..4689ab976...", ".469bcddcb975.", "3579bcddddddc8", "35789988776542", "2468a7420@0431",
    ".3579a8742373.", "..468a98778a96", "..3579a9889ab7", "...4689753e00.", "...3578d0d0d0.", "...3570000000.",
    "...3586b6b6b4.", "....4789a9864.", ".....3566543..",
};
static void garg_head(float hy, int awake) {                      /* horns, fin ears, then the face */
    for (int s = -1; s <= 1; s += 2) {
        float ex[6] = { 32 + s * 6.0f, hy + 5, 32 + s * 13.0f, hy + 1, 32 + s * 7.0f, hy + 10 };   /* a fin ear, swept back */
        cv_poly(ex, 3, R_STONE, 0.15f, 0.55f, hy + 1, 9);
        cv_line(32 + s * 7.0f, hy + 6, 32 + s * 11.0f, hy + 3, sh(R_STONE, 0.1f));
        cv_capsule(32 + s * 5.0f, hy + 2, 32 + s * 10.0f, hy - 2, 1.7f, R_STONE, 0.2f, 0.8f);   /* a bull's horn, out and forward */
        cv_capsule(32 + s * 10.0f, hy - 2, 32 + s * 13.0f, hy + 1, 1.3f, R_STONE, 0.2f, 0.75f);
        if (s < 0) cv_capsule(32 + s * 13.0f, hy + 1, 32 + s * 12.5f, hy + 4, 0.9f, R_STONE, 0.25f, 0.8f);   /* the right one broken short */
    }
    grid_stamp(GARG_FACE, 15, 17, 25, (int)hy, R_STONE, R_STONE, awake ? (uint8_t)(R_FLAME + 12) : 0, 0);
}
static void garg_wing_front(int s, float sx, float sy, float wx, float wy, float tips[3][2], float lo, float hi) {   /* membrane, struts, a torn hem */
    for (int f = 0; f < 3; f++) {
        float nx = f < 2 ? tips[f + 1][0] : sx, ny = f < 2 ? tips[f + 1][1] : sy + 6.0f;
        float m[8] = { wx, wy, tips[f][0], tips[f][1], nx, ny, sx, sy };
        cv_poly(m, 4, R_STONE, lo, hi, wy < tips[f][1] ? wy : tips[f][1], 34);
        cv_flat((tips[f][0] + nx) * 0.5f, (tips[f][1] + ny) * 0.5f + 1.2f, 1.8f, 1.6f, C_CLEAR);   /* a scallop torn out */
    }
    cv_capsule(sx, sy, wx, wy, 1.6f, R_STONE, lo + 0.1f, hi + 0.2f);
    for (int f = 0; f < 3; f++) cv_capsule(wx, wy, tips[f][0], tips[f][1], 0.8f, R_STONE, lo + 0.05f, hi + 0.1f);
    cv_line(wx, wy, wx + s * 1.0f, wy - 3.0f, sh(R_STONE, 0.7f));   /* the claw at its wrist */
}
static void garg_weather(int awake, uint32_t seed) {               /* rain stains, lichen, chips; and awake, its cracks lit */
    for (int y = 0; y < TS; y++)
        for (int x = 0; x < TS; x++) {
            uint8_t *p = &cv_sprite[y * TS + x];
            if (*p <= R_STONE || *p >= R_STONE + 16 || y >= 52) continue;
            float n = fbm(x / 4.0f, y / 4.0f, 16, seed);
            int face = x >= 25 && x < 40 && y >= 2 && y < 34;
            if (n < (face ? 0.18f : 0.3f)) *p = (uint8_t)(*p - 2 < R_STONE + 1 ? R_STONE + 1 : *p - 2);
            else if (!face && h01(x, y, seed + 2) > 0.965f) *p = (uint8_t)(*p - 4 < R_STONE + 1 ? R_STONE + 1 : *p - 4);
            if (*p <= R_STONE + 4 && h01(x, y, seed + 4) > 0.86f) *p = sh(R_MOSS, 0.12f + 0.12f * h01(x, y, seed + 5));   /* lichen in the hollows */
        }
    for (int k = 0; k < 4; k++) {                                  /* rain, run down from the eyes and the mouth's corners */
        static const int SRC[4] = { 29, 35, 27, 37 };
        int x = SRC[k], y = 5;
        while (y < TS && cv_sprite[y * TS + x] != 0) y++;          /* below the first dark of the face */
        for (int j = 1; j < 9 + (k & 1) * 4 && y + j < TS; j++) {
            uint8_t *p = &cv_sprite[(y + j) * TS + x];
            if (*p > R_STONE && *p < R_STONE + 16) *p = (uint8_t)(*p - 3 < R_STONE + 1 ? R_STONE + 1 : *p - 3);
        }
    }
    if (awake)
        for (int k = 0; k < 4; k++) {                              /* the cracks alight, dimly, as it wakes */
            float x = 20 + 24 * h01(k, 1, seed + 7), y = 30 + 16 * h01(k, 2, seed + 7), a = h01(k, 3, seed + 7) * TAU_F;
            for (int i = 0; i < 7; i++) {
                a += (h01(k, i + 4, seed + 7) - 0.5f) * 1.2f; x += cosf(a); y += sinf(a);
                uint8_t c = cv_sprite[(int)y * TS + (int)x];
                if (c > R_STONE && c < R_STONE + 16) cv_px((int)x, (int)y, (uint8_t)(R_FLAME + 3 + (i & 1) * 2));
            }
        }
}
static void spr_gargoyle(int id, int pose) {
    cv_begin(cv_sprite, TS, TS);
    if (pose == 4) {                                               /* broken: rubble, a wing, the face looking up */
        cv_flat(32, 61.5f, 24, 2.5f, R_WARM + 1);
        for (int i = 0; i < 10; i++) {
            float x = 32 + (h01(i, 1, 541) - 0.5f) * 44, y = 56 + 6 * h01(i, 2, 541);
            cv_ellipse(x, y, 2.5f + 3.0f * h01(i, 3, 541), 2.0f + 2.0f * h01(i, 4, 541), R_STONE, 0.1f, 0.6f);
        }
        float wv[8] = { 34, 50, 54, 42, 52, 54, 38, 57 };
        cv_poly(wv, 4, R_STONE, 0.12f, 0.5f, 42, 15);
        grid_stamp(GARG_FACE, 15, 17, 4, 44, R_STONE, R_STONE, 0, 0);
        cv_outline(0); cv_store(id); return;
    }
    int perch = pose == 0, swoop = pose == 3;
    float hy = perch ? 13.0f : 3.0f;                               /* the head's top: lowered asleep, up awake */
    if (perch) {                                                   /* the plinth */
        for (int y = 52; y < 64; y++)
            for (int x = 18; x <= 46; x++) stone_px(x, y, (x + 0.5f - 32) / 14.0f, y < 54 ? 0.15f : 0.0f, 543);
        for (int s = -1; s <= 1; s += 2) {                         /* wings folded up round it, high as a cloak */
            float wv[10] = { 32 + s * 9.0f, hy + 20, 32 + s * 17.0f, hy - 6, 32 + s * 20.0f, hy + 6, 32 + s * 17.0f, 51, 32 + s * 10.0f, 51 };
            cv_poly(wv, 5, R_STONE, 0.08f, 0.45f, hy - 6, 50);
            cv_line(32 + s * 17.0f, hy - 6, 32 + s * 16.0f, 50, sh(R_STONE, 0.12f));
            cv_line(32 + s * 18.0f, hy + 2, 32 + s * 12.0f, 50, sh(R_STONE, 0.15f));
        }
        cv_ellipse(32, hy + 21, 11.0f, 4.0f, R_STONE, 0.2f, 0.75f);   /* slab shoulders, a hunched back */
        cv_ellipse(32, hy + 29, 8.0f, 8.0f, R_STONE, 0.15f, 0.65f);
        for (int s = -1; s <= 1; s += 2) {                         /* haunches folded, claws over the plinth's edge, hands on it */
            cv_ellipse(32 + s * 8.0f, 45, 4.5f, 5.0f, R_STONE, 0.15f, 0.7f);
            cv_capsule(32 + s * 8.0f, 48, 32 + s * 8.5f, 52, 2.2f, R_STONE, 0.15f, 0.6f);
            for (int c = -1; c <= 1; c++) cv_line(32 + s * 8.5f + c * 1.4f, 52, 32 + s * 8.5f + c * 1.8f, 55, sh(R_STONE, 0.3f));
            cv_capsule(32 + s * 9.0f, hy + 22, 32 + s * 4.0f, 46, 2.2f, R_STONE, 0.2f, 0.7f);
            for (int c = -1; c <= 1; c++) cv_line(32 + s * 4.0f, 46, 32 + s * 4.0f + c * 1.3f, 50, sh(R_STONE, 0.3f));
        }
    } else {
        float tip = pose == 1 ? -4.0f : (pose == 2 ? 14.0f : 4.0f), out = swoop ? 22.0f : 30.0f;
        for (int s = -1; s <= 1; s += 2) {                         /* the wings, spread (swept back in a swoop) */
            float tips[3][2] = { { 32 + s * out, hy + 6 + tip }, { 32 + s * (out - 4.0f), hy + 20 + tip * 0.5f }, { 32 + s * (out - 12.0f), hy + 28 } };
            garg_wing_front(s, 32 + s * 8.0f, hy + 20, 32 + s * (out - 10.0f), hy + tip * 0.6f, tips, 0.08f, 0.4f);
        }
        for (int s = -1; s <= 1; s += 2) {                         /* legs: hanging, or thrust forward with the talons out */
            float kx = 32 + s * 8.0f, ky = swoop ? hy + 33 : hy + 40, fx = 32 + s * (swoop ? 11.0f : 7.0f), fy = swoop ? hy + 42 : hy + 50;
            cv_capsule(32 + s * 5.0f, hy + 33, kx, ky, 3.0f, R_STONE, 0.15f, 0.7f);
            cv_capsule(kx, ky, fx, fy, 2.2f, R_STONE, 0.15f, 0.65f);
            for (int c = -1; c <= 1; c++) cv_line(fx, fy, fx + c * 2.0f, fy + 3.5f, sh(R_STONE, 0.35f));
        }
        cv_ellipse(32, hy + 21, 11.0f, 4.0f, R_STONE, 0.2f, 0.8f);  /* slab shoulders, the chest */
        cv_ellipse(32, hy + 28, 8.0f, 8.5f, R_STONE, 0.15f, 0.7f);
        for (int r = 0; r < 3; r++) cv_line(27, hy + 25 + r * 3, 37, hy + 25 + r * 3, sh(R_STONE, 0.12f));
        for (int s = -1; s <= 1; s += 2) {                         /* arms: claws ahead in a swoop, else low */
            float ex = 32 + s * (swoop ? 15.0f : 13.0f), ey = swoop ? hy + 16 : hy + 29, hx2 = 32 + s * (swoop ? 14.0f : 11.0f), hy2 = swoop ? hy + 8 : hy + 37;
            cv_capsule(32 + s * 10.0f, hy + 21, ex, ey, 2.3f, R_STONE, 0.2f, 0.75f);
            cv_capsule(ex, ey, hx2, hy2, 1.9f, R_STONE, 0.2f, 0.7f);
            for (int c = -1; c <= 1; c++) cv_line(hx2, hy2, hx2 + c * 1.6f, hy2 + (swoop ? -3.5f : 3.5f), sh(R_STONE, 0.35f));
        }
    }
    garg_head(hy, !perch);
    garg_weather(!perch, 545);
    cv_outline(0);
    cv_store(id);
}

/* the gargoyle's ranks are its stone (docs/InitialArtPass.md section 8): II moss on limestone and its wings torn, III
 * frost-glazed and a horn chipped, IV red with rust run down it and a wing cracked, V black basalt veined with a cold
 * light under an iron collar, gilded marble with gold in its veins. Not on the statue: a statue's rank doesn't show. */
static void garg_dress(int id) {
    for (int r = RK_II; r < NRANK; r++) {
        dress_begin(id);
        for (int y = 0; y < 40; y++)                               /* torn wings from II: dark rents in the hem */
            for (int x = 0; x < TS; x++)
                if ((x < 20 || x > 44) && base_on(x, y) && !base_on(x, y + 1) && h01(x, y, 451) < 0.3f) { cv_px(x, y, 0); cv_px(x, y - 1, 0); }
        if (r == RK_II) kit_moss(0.38f, 3, 453);
        if (r == RK_III) kit_frost(0.85f, 455);
        if (r >= RK_III && r != RK_GILDED) { cv_px(42, 3, 0); cv_px(43, 2, 0); cv_px(43, 3, 0); }   /* a chip off the horn */
        if (r == RK_IV)                                            /* rust run down it */
            for (int k = 0; k < 7; k++) {
                int x = 22 + (int)(h01(k, 1, 457) * 20.0f), y0 = 18 + (int)(h01(k, 2, 457) * 14.0f);
                for (int j = 0; j < 6 + (int)(h01(k, 3, 457) * 10.0f); j++) if (base_on(x, y0 + j)) cv_px(x, y0 + j, (uint8_t)(R_BRICK + 4 + (j < 3 ? 2 : 0)));
            }
        if (r >= RK_IV && r != RK_GILDED) { cv_line(8, 14, 16, 22, 0); cv_line(16, 22, 13, 28, 0); }   /* a wing cracked through */
        if (r == RK_V || r == RK_GILDED)                           /* veins: a cold light in the basalt, or gold in the marble */
            for (int y = 0; y < 52; y++)
                for (int x = 0; x < TS; x++) {
                    uint8_t b = base_c(x, y);
                    if (b <= R_STONE || b >= R_STONE + 16) continue;
                    if (fabsf(fbm(x / 5.0f, y / 5.0f, 16, 459) - 0.5f) < 0.022f)   /* the noise's ridges: thin veins, most of them dim */
                        cv_px(x, y, r == RK_V ? (uint8_t)(R_GLOW + (h01(x, y, 461) < 0.25f ? 12 : 6 + (int)(h01(x, y, 465) * 3.0f))) : (uint8_t)(R_GOLD + 8 + (int)(h01(x, y, 461) * 4.0f)));
                }
        if (r == RK_V) { for (int x = 26; x <= 38; x++) { cv_px(x, 21, sh(R_IRON, 0.55f)); cv_px(x, 22, sh(R_IRON, 0.25f)); } kit_chain(37, 22, 44, 36, 2.0f, R_IRON); }
        if (r == RK_GILDED) { kit_gold(R_STONE, 0.45f, 463); kit_chain(26, 22, 38, 22, 1.5f, R_GOLD); }
        dress_store(id, r, R_STONE);
    }
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
    skull_stamp(33, 9.5f + bob, 1, 0, g_decay > 0.55f ? (uint8_t)(R_GLOW + 13) : (uint8_t)(R_FLAME + 10));
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
    cv_line(sx, sy, elbow_x, elbow_y, sh(R_IRON, 0.45f));
    for (int f = 0; f < 3; f++) cv_line(elbow_x, elbow_y, tips[f][0], tips[f][1], sh(R_IRON, 0.38f));
    cv_px((int)elbow_x, (int)elbow_y, sh(R_IRON, 0.65f)); cv_px((int)(elbow_x + span), (int)(elbow_y - 2.0f * wy), sh(R_IRON, 0.7f));   /* the wrist, its thumb */
}

/* a bat from the side, wings up or down: its ear back and its snout ahead, a fang showing, a coal of an eye */
static void spr_bat_side(int id, int up) {
    cv_begin(cv_sprite, TS, TS);
    float wy = up ? -1.0f : 1.0f;
    bat_wing(31, 28, wy, 1.25f, 0.03f, 0.15f);                     /* the far wing */
    cv_ellipse(31, 30, 6.0f, 3.6f, R_IRON, 0.1f, 0.38f);          /* the body, level in flight */
    cv_capsule(25, 31, 21, 34, 0.8f, R_IRON, 0.05f, 0.3f);         /* feet trailing, clawed */
    cv_px(20, 35, sh(R_IRON, 0.6f)); cv_px(21, 35, sh(R_IRON, 0.5f));
    grid_stamp(BAT_SIDE, 9, 11, 35, 22, R_IRON, R_SKIN, (uint8_t)(R_FLAME + 8), 0);
    bat_wing(34, 29, wy, 1.5f, 0.06f, 0.3f);                       /* the near one */
    cv_outline(0);
    cv_store(id);
}

/* the web spider from the side: the pitted abdomen behind with its mark on top, the head in front with three of its
 * eyes showing, a fang with its drop, banded legs either side bristling at the knee */
static void spr_spider_side(int id, int frame) {
    cv_begin(cv_sprite, TS, TS);
    cv_flat(32, 62, 24, 1.8f, R_WARM + 1);
    for (int l = 0; l < 4; l++) {                                   /* the far legs */
        float k = ((l + frame) & 1) ? 2.5f : -2.5f, ax = 34 + l * 1.5f, fx = 14 + l * 11.0f + k, kx = (ax + fx) * 0.5f + (l < 2 ? -3 : 3);
        chitin_leg(ax, 44, kx, 33 + l * 0.6f, fx, 61, 1.1f, R_IRON, 0.02f, 0.2f, -1, 471 + (uint32_t)l);
    }
    spider_abdomen(23, 42, 10.0f, 8.0f, R_IRON, 473);               /* the abdomen, its mark on top */
    for (int q = -2; q <= 2; q++) cv_px(22 + q, 34 + (q == 0), (uint8_t)(R_BLOOD + 8 + (q == 0) * 2));
    cv_px(22, 35, (uint8_t)(R_BLOOD + 7));
    cv_ellipse(37, 45, 6.5f, 4.6f, R_IRON, 0.06f, 0.42f);          /* the head */
    for (int l = 0; l < 4; l++) {                                   /* the near legs */
        float k = ((l + frame + 1) & 1) ? 2.5f : -2.5f, ax = 35 + l * 1.5f, fx = 12 + l * 12.0f + k, kx = (ax + fx) * 0.5f + (l < 2 ? -4 : 4);
        chitin_leg(ax, 46, kx, 31 + l * 0.8f, fx, 61.5f, 1.3f, R_IRON, 0.06f, 0.38f, -1, 477 + (uint32_t)l);
    }
    cv_ellipse(41.5f, 44.5f, 3.2f, 2.8f, R_IRON, 0.1f, 0.42f);     /* its face, before the legs */
    cv_px(41, 42, (uint8_t)(R_FLAME + 9)); cv_px(40, 43, (uint8_t)(R_FLAME + 7));   /* three of its eyes */
    cv_eye(43, 43, (uint8_t)(R_FLAME + 13)); cv_px(43, 44, (uint8_t)(R_FLAME + 10));
    cv_ellipse(42.5f, 47.5f, 1.6f, 1.8f, R_IRON, 0.12f, 0.34f);    /* the jaw, and its fang */
    for (int k = 0; k <= 4; k++) cv_px(43 - (k > 2), 48 + k, sh(R_BONE, 0.45f + 0.1f * k));
    cv_px(42, 53, (uint8_t)(R_MOSS + 14)); cv_px(42, 54, (uint8_t)(R_MOSS + 10));
    cv_outline(0);
    cv_store(id);
}

/* the spectre from the side: leaning on, its hem streaming back, a hand held out */
static void spr_ghost_side(int id, float ph) {             /* the spectre turned: the long face forward, its shroud streaming behind */
    cv_begin(cv_sprite, TS, TS);
    for (int y = 2; y < TS; y++)
        for (int x = 0; x < TS; x++) {
            float yy = y + 0.5f, xx = x + 0.5f, front, back;
            if (yy < 15.0f) {                                         /* the hood's crown, round, bulging back */
                float k = (yy - 15.0f) / 12.5f;
                if (fabsf(k) >= 1.0f) continue;
                float r = 10.5f * sqrtf(1.0f - k * k);
                front = 35.0f + r * 0.95f; back = 35.0f - r * 1.15f;
            } else if (yy < 28.0f) {                                  /* and falling to the shoulders */
                float k = (yy - 15.0f) / 13.0f;
                front = 45.0f - 2.0f * k; back = 22.9f + 1.6f * k;
            } else {                                                  /* the shroud streaming back and down */
                float t = (yy - 28.0f) / 35.0f;
                front = 43.0f - 13.0f * t; back = 24.5f - 12.0f * t + sinf(yy * 0.18f + ph) * 2.5f * t;
            }
            if (xx > front || xx < back) continue;
            if (yy > 46.0f && h01((int)(xx * 0.7f), (int)(yy / 3.0f), 981 + (int)(ph * 3.0f)) * 0.7f + h01(x, y, 983) * 0.3f < (yy - 46.0f) / 20.0f) continue;
            float u = (xx - back) / (front - back + 0.01f), v = 0.09f + 0.2f * sinf(u * PI_F) - (yy > 28.0f ? (yy - 28.0f) * 0.002f : 0.0f);
            if (fmodf(yy * 0.3f - xx * 0.25f + sinf(xx * 0.1f + ph) + 20.0f, 2.4f) < 0.4f) v -= 0.05f;   /* folds, streaming */
            if (u > 0.9f || u < 0.06f) v += 0.25f;                 /* the cold rim */
            float ox = (xx - 40.0f) / 4.6f, oyy = (yy - 17.0f) / 11.0f;
            if (ox * ox + oyy * oyy <= 1.0f) v = 0.02f;             /* the hood's mouth */
            cv_px(x, y, sh(R_GHOST, v));
        }
    grid_stamp(SPECTRE_SIDE, 9, 21, 36, 7, R_GHOST, R_GHOST, C_WHITE, 0);
    for (int x = 35; x <= 45; x++) {                                /* the brim over the brow */
        float ox = (x + 0.5f - 40.0f) / 5.2f;
        if (fabsf(ox) >= 1.0f) continue;
        int ye = (int)(17.0f - 11.0f * sqrtf(1.0f - ox * ox));
        cv_px(x, ye, sh(R_GHOST, 0.42f)); cv_px(x, ye + 1, sh(R_GHOST, 0.3f));
    }
    float hy = 29.0f + 1.5f * sinf(ph);
    cv_capsule(34, 26, 47, hy - 1.0f, 3.2f, R_GHOST, 0.12f, 0.48f);   /* the sleeve */
    cv_ellipse(47, hy - 1.0f, 3.0f, 2.6f, R_GHOST, 0.15f, 0.5f);
    mist_hand(50, hy, 1, 0.35f, ph, 991);
    cv_store(id);
}

/* the wall skull turned to show its side: a great skull in profile, a coal in its eye */
static void spr_wskull_side(void) {                         /* the master profile three times over, its eye alight */
    cv_begin(cv_sprite, TS, TS);
    grid_scale3(SKULL_SIDE, SKULL_W, SKULL_H, 0);
    int x0 = 32 - g_bigw / 2, y0 = 6;
    for (int y = 0; y < g_bigh; y++)
        for (int x = 0; x < g_bigw; x++) {
            uint8_t c = g_big[y * 64 + x];
            if (c == C_CLEAR) continue;
            if (c > R_BONE && c < R_BONE + 16) { float r = h01(x, y, 71); c = (uint8_t)(R_BONE + clampi(c - R_BONE + (r < 0.12f ? -1 : r > 0.93f ? 1 : 0), 1, 15)); }
            cv_px(x0 + x, y0 + y, c);
        }
    for (int y = 0; y < SKULL_H; y++)
        for (int x = 0; x < SKULL_W; x++)
            if (SKULL_SIDE[y][x] == '@') { cv_flat(x0 + x * 3 + 1.5f, y0 + y * 3 + 1.5f, 2.6f, 2.8f, (uint8_t)(R_FLAME + 8)); cv_flat(x0 + x * 3 + 1.5f, y0 + y * 3 + 1.5f, 1.2f, 1.3f, (uint8_t)(R_FLAME + 13)); }
    cv_outline(0);
    cv_store(SP_WSKULL_S);
}

/* the shambler from the side: bent forward, arms out ahead of it, one foot dragging */
static void spr_shambler_side(int id, float w) {
    cv_begin(cv_sprite, TS, TS);
    cv_flat(32, 62, 14, 1.8f, R_WARM + 1);
    cv_capsule(31, 42, 31 - 3 * w, 52, 2.6f, R_STONE, 0.05f, 0.28f);  /* the far leg, bound in linen */
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
            cv_px(x, y, sh(R_STONE, 0.2f + 0.25f * (x - 24) / 16.0f - (((int)(y + x * 0.5f)) % 5 == 0 ? 0.12f : 0.0f)));   /* burial linen, wound */
        }
    cv_capsule(31, 42, 31 + 3 * w, 52, 2.8f, R_STONE, 0.1f, 0.4f);    /* the near leg, bound */
    cv_capsule(31 + 3 * w, 52, 32 + 5 * w, 60.5f, 2.0f, R_WARM, 0.3f, 0.8f);
    cv_capsule(32 + 5 * w, 61.5f, 37 + 5 * w, 61.5f, 1.5f, R_WARM, 0.3f, 0.8f);
    cv_capsule(37, 21, 39, 16, 2.2f, R_WARM, 0.3f, 0.75f);         /* the neck, craned */
    cv_ellipse(41, 13, 5.2f, 6.0f, R_WARM, 0.35f, 1.0f);            /* the head, hung forward */
    cv_ellipse(43, 19, 3.0f, 2.6f, R_WARM, 0.2f, 0.6f);            /* the jaw, dropped */
    cv_flat(44.5f, 17, 1.8f, 1.8f, 0);                              /* the mouth hanging open, a tooth or two */
    cv_px(45, 16, sh(R_WARM, 0.95f)); cv_px(44, 19, sh(R_WARM, 0.9f));
    cv_flat(44, 11.5f, 1.6f, 1.5f, 0); cv_eye(44, 12, (uint8_t)(R_FLAME + 13));   /* a sunken socket, its sulphur light */
    cv_px(46, 14, 0);                                               /* the nose, rotted to a hole */
    for (int i = 0; i < 5; i++) cv_line(37 + i, 8, 34 + i * 0.8f, 13 + i * 1.5f, sh(R_IRON, 0.2f));   /* hair, down the back */
    cv_capsule(37, 23, 52, 25, 2.0f, R_WARM, 0.35f, 0.9f);         /* the near arm, reaching */
    dead_hand(54, 26, 0.6f, 0.35f, R_WARM, 1.1f);
    cv_rot(R_WARM, 0.6f, 963);
    for (int y = 0; y < TS; y++)                                    /* grave-dirt clotted on it */
        for (int x = 0; x < TS; x++) {
            uint8_t c = cv_sprite[y * TS + x];
            if (((c > R_WARM && c < R_WARM + 16) || (c > R_STONE && c < R_STONE + 16)) && fbm(x / 3.0f, y / 3.0f, 16, 213) > 0.74f - (float)y * 0.004f)
                cv_px(x, y, sh(R_WOOD, 0.06f + 0.14f * h01(x, y, 214)));
        }
    cv_outline(0);
    cv_store(id);
}

/* the Drowned from the side: the swollen belly out in front, the head sunk, weed down its back */
static void spr_drowned_side(int id, float w) {
    cv_begin(cv_sprite, TS, TS);
    cv_flat(32, 62, 15, 1.8f, R_WARM + 1);
    cv_capsule(28, 46, 28 - 3 * w, 60, 2.0f, R_GHOST, 0.1f, 0.4f); /* the far leg */
    cv_capsule(30, 26, 38, 40, 3.0f, R_GHOST, 0.1f, 0.4f);         /* the far arm, hanging */
    cv_ellipse(33, 38, 8.0f, 8.5f, R_GHOST, 0.15f, 0.7f);          /* the belly, swollen tight on a thin frame */
    cv_ellipse(33, 27, 4.5f, 5.0f, R_GHOST, 0.15f, 0.6f);          /* a narrow chest over it, hunched */
    cv_flat(44, 40, 1.4f, 1.8f, sh(R_GHOST, 0.2f));
    cv_rot(R_GHOST, 0.3f, 965);
    for (int i = 0; i < 10; i++) cv_px((int)(22 + 20 * h01(i, 3, 967)), (int)(26 + 22 * h01(i, 4, 967)), sh(R_BONE, 0.55f + 0.3f * h01(i, 5, 967)));
    cv_capsule(31, 46, 31 + 3 * w, 60, 2.2f, R_GHOST, 0.15f, 0.6f); /* the near leg */
    cv_ellipse(35 + 3 * w, 61.5f, 4.0f, 1.6f, R_GHOST, 0.15f, 0.55f);
    cv_capsule(34, 22, 37, 18, 1.6f, R_GHOST, 0.15f, 0.55f);       /* the neck, the head hung forward on it */
    cv_ellipse(38, 15, 5.0f, 7.5f, R_GHOST, 0.2f, 0.75f);          /* a long face, the hair plastered down its back */
    for (int y = 8; y < 26; y++) cv_px(34 - (y > 16) - (y > 21), y, sh(R_IRON, 0.12f)), cv_px(35 - (y > 18), y, sh(R_IRON, 0.18f));
    cv_flat(41, 16, 1.6f, 1.3f, 0); cv_eye(41, 16, (uint8_t)(R_GLOW + 14));   /* a small cold eye in its socket */
    cv_flat(41.5f, 20, 1.6f, 2.6f, 0);                              /* the mouth, slack and black, and the water out of it */
    for (int y = 22; y < 34; y++) if (h01(y, 1, 969) > 0.3f) cv_px(43 + (y & 1), y, sh(R_GHOST, 0.5f + 0.3f * h01(y, 2, 969)));
    for (int i = 0; i < 6; i++) {                                   /* weed, down the back of its head and shoulders */
        float x = 30 + i * 1.5f, len = 8 + 10 * h01(i, 1, 971);
        for (int y = 0; y < (int)len; y++) cv_px((int)(x - y * 0.3f + sinf(y * 0.5f + i + w) * 0.8f), 11 + y, i & 1 ? sh(R_IRON, 0.12f) : sh(R_MOSS, 0.1f + 0.22f * (1.0f - y / len)));   /* hair and weed */
    }
    cv_capsule(36, 24, 42, 44, 1.8f, R_GHOST, 0.25f, 0.8f);        /* the near arm, too long */
    dead_hand(42, 47, 1.5f, 0.3f, R_GHOST, 1.15f);
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
    dog_skull_at((int)hx - 5, (int)hy - 4, 1.0f, 1, 1);             /* the hand-drawn skull, facing right */
    cv_bone_decay(975);
    cv_outline(0);
    cv_store(id);
}

/* the Wraith from the side: leaning on, its peaked hood ahead of it, the void in its mouth and one light, the cerements
 * wound slantwise and streaming back into strips, one long hand held out low */
static void spr_wraith_side(int id, float ph) {
    cv_begin(cv_sprite, TS, TS);
    for (int y = 4; y < TS; y++)
        for (int x = 0; x < TS; x++) {
            float t = (y - 4) / 59.0f, lean = 7.0f * (1.0f - t);
            float front = 40.0f + lean - (y < 16 ? (16 - y) * 0.7f : 0.0f) + sinf(y * 0.25f + ph) * 0.6f;
            float back = 26.0f + lean - t * 20.0f + sinf(y * 0.18f + ph) * 2.0f;
            if (x > front || x < back) continue;
            if (y > 42) {                                            /* coming loose into strips, streaming back */
                float at = y * 0.9f + x * 0.45f + sinf(ph + y * 0.2f) * 1.5f + 64.0f;
                int sid = (int)floorf(at / 3.0f);
                float within = at - sid * 3.0f, end = 50.0f + 11.0f * h01(sid, 3, 981);
                if (y > end || within < 0.7f) continue;
            }
            float u = (x - back) / (front - back + 0.01f), v = 0.07f + 0.14f * sinf(u * PI_F) - t * 0.05f;
            float b = (y - x * 0.35f + 3.0f * (fbm(x / 7.0f, y / 7.0f, 16, 983) - 0.45f)) / 4.8f, fb = b - floorf(b);   /* wound slantwise */
            if (u > 0.9f || u < 0.06f) { cv_px(x, y, sh(R_GHOST, 0.28f + 0.3f * (u > 0.9f))); continue; }   /* the cold rim */
            if (fb < 0.13f) { cv_px(x, y, 0); continue; }
            if (fb < 0.23f && u > 0.6f && h01((int)floorf(b), 1, 985) < 0.45f) { cv_px(x, y, sh(R_GHOST, 0.14f)); continue; }
            if (fb > 0.45f && fb < 0.72f) v += 0.07f;
            if (y > 18 && fbm(x / 3.5f, y / 3.5f, 32, 987) > 0.7f) { cv_px(x, y, 0); continue; }   /* torn through */
            cv_px(x, y, sh(R_IRON, v));
        }
    for (int y = 0; y < 22; y++)                                    /* the hood, peaked forward, ragged at the shoulder */
        for (int x = 28; x < 50; x++) {
            float top = 2.0f + (x - 43) * (x - 43) * 0.07f;
            float fr = 47.5f - (y > 10 ? (y - 10) * 0.55f : 0.0f), bk = 31.0f + y * 0.05f - (y > 12 ? (y - 12) * 0.5f : 0.0f);
            if (y < top || x > fr || x < bk || y > 18.0f + 3.0f * h01(x / 2, 5, 989)) continue;
            float u = (x - bk) / (fr - bk);
            uint8_t c = x > fr - 1.5f || y < top + 1.0f ? sh(R_GHOST, 0.32f) : sh(R_IRON, 0.08f + 0.12f * u - (sinf((x - 43) * 0.9f + y * 0.3f) > 0.6f ? 0.06f : 0.0f));
            cv_px(x, y, c);
        }
    for (int y = 5; y < 18; y++)                                    /* its mouth, from the side: a deep notch of nothing */
        for (int x = 40; x < 48; x++) {
            float w = y < 9 ? 3.2f * powf((y - 4.5f) / 4.5f, 0.7f) : 3.2f - (y > 15 ? (y - 15) * 0.8f : 0.0f);
            if (x + 0.5f > 47.0f - (y > 10 ? (y - 10) * 0.55f : 0.0f) - 2.0f * w) cv_px(x, y, 0);
        }
    cv_eye(45, 11, C_WHITE); cv_px(45, 12, (uint8_t)(R_GLOW + 13)); cv_px(44, 11, (uint8_t)(R_GLOW + 10));   /* one light */
    cv_capsule(38, 18, 46, 32, 3.8f, R_IRON, 0.06f, 0.3f);          /* the sleeve */
    cv_line(41, 17, 49, 31, sh(R_GHOST, 0.4f));
    for (int j = 0; j < 3; j++) {                                   /* rags off it, streaming back */
        float px = 40.0f + j * 2.5f, py = 22.0f + j * 4.0f;
        for (int q = 0; q < 7 + j * 2; q++) cv_px((int)(px - q * 0.6f + sinf(ph * 1.4f + q * 0.4f + j) * 0.8f), (int)(py + q * 0.7f), sh(R_IRON, 0.1f));
    }
    wraith_hand(48, 35, 0.75f, 1, 1.0f);                            /* the hand, and fingers too long for it */
    for (int i = 0; i < 14; i++) {                                  /* frost falling off it */
        float x = 8 + 40 * h01(i, 1, 979), y = 50 + 13 * h01(i, (int)(ph * 2) + 2, 979);
        if (cv_sprite[(int)y * TS + (int)x] == C_CLEAR) cv_px((int)x, (int)y, sh(R_GHOST, 0.45f + 0.4f * h01(i, 3, 979)));
    }
    cv_store(id);
}

/* a spider from the side, in a body ramp: legs banded dark (band) or plain, bristling at the knee; the abdomen as big as
 * it is, furred with chevrons (banded) or pitted, eggs in it; the face drawn over the near legs, one great eye or three
 * small ones showing, a fang with its drop */
static void spider_side(int frame, int ramp, int band, float ab, int eggs, int big_eyes) {
    cv_flat(32, 62, 24, 1.8f, R_WARM + 1);
    for (int l = 0; l < 4; l++) {                                   /* far legs */
        float k = ((l + frame) & 1) ? 2.5f : -2.5f, ax = 34 + l * 1.5f, fx = 14 + l * 11.0f + k, kx = (ax + fx) * 0.5f + (l < 2 ? -3 : 3);
        chitin_leg(ax, 44, kx, 33 + l * 0.6f, fx, 61, 1.1f, ramp, 0.02f, 0.22f, band, 961 + (uint32_t)l);
    }
    float acx = 25 - (ab - 1.0f) * 6, acy = 42 - (ab - 1.0f) * 6;
    if (band >= 0) {                                                /* furred, chevrons along it */
        hunter_abdomen(acx, acy, 10.0f * ab, 8.0f * ab, ramp, 1, 963);
        for (int i = 0; i < 3; i++) cv_line(18 + i * 5, 35, 20 + i * 5, 49, sh(band, 0.12f));
    } else spider_abdomen(acx, acy, 10.0f * ab, 8.0f * ab, ramp, 965);
    if (eggs) for (int i = 0; i < 10; i++) {
        float a = h01(i, 1, 981) * TAU_F, r = 8.0f * ab * sqrtf(h01(i, 2, 981));
        cv_ellipse(acx + cosf(a) * r, acy + sinf(a) * r * 0.75f, 2.0f, 1.7f, R_BONE, 0.15f, 0.45f);
    }
    if (band < 0) cv_flat(22, 36, 2.6f, 1.6f, (uint8_t)(R_BLOOD + 9));
    cv_ellipse(37, 45, 6.5f, 4.6f, ramp, 0.06f, 0.45f);            /* the head */
    for (int l = 0; l < 4; l++) {                                   /* near legs */
        float k = ((l + frame + 1) & 1) ? 2.5f : -2.5f, ax = 35 + l * 1.5f, fx = 12 + l * 12.0f + k, kx = (ax + fx) * 0.5f + (l < 2 ? -4 : 4);
        chitin_leg(ax, 46, kx, 31 + l * 0.8f, fx, 61.5f, 1.3f, ramp, 0.06f, 0.42f, band, 967 + (uint32_t)l);
    }
    cv_ellipse(41.5f, 44.5f, 3.2f, 2.8f, ramp, 0.1f, 0.45f);       /* its face, before the legs */
    if (big_eyes) { cv_flat(42.5f, 43.5f, 1.8f, 1.8f, 0); cv_eye(42, 43, (uint8_t)(R_FLAME + 14)); cv_px(43, 43, (uint8_t)(R_FLAME + 11)); cv_px(42, 44, (uint8_t)(R_FLAME + 10)); cv_px(40, 46, (uint8_t)(R_FLAME + 6)); }
    else { cv_px(41, 42, (uint8_t)(R_FLAME + 9)); cv_px(40, 43, (uint8_t)(R_FLAME + 7)); cv_eye(43, 43, (uint8_t)(R_FLAME + 13)); }
    cv_ellipse(42.5f, 47.5f, 1.6f, 1.8f, ramp, 0.12f, 0.36f);      /* the jaw, and its fang */
    for (int k = 0; k <= 4; k++) cv_px(43 - (k > 2), 48 + k, eggs ? sh(R_BONE, 0.45f + 0.1f * k) : sh(R_IRON, 0.3f + 0.08f * k));
    cv_px(42, 53, (uint8_t)(R_MOSS + 14)); cv_px(42, 54, (uint8_t)(R_MOSS + 10));
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

/* the imp from the side: a hop, the tail and its ember behind, a wing up, the long muzzle ahead (IMP_SIDE) */
static void spr_imp_side(int id, int pose) {
    cv_begin(cv_sprite, TS, TS);
    float up = pose == 0 ? -3.0f : 0.0f, hy = 5.0f + up;
    cv_flat(32, 62, 11, 1.6f, R_WARM + 1);
    cv_capsule(28, hy + 33, 16, hy + 40, 1.2f, R_BRICK, 0.1f, 0.4f);   /* the tail, its ember */
    cv_capsule(16, hy + 40, 10, hy + 31, 1.0f, R_BRICK, 0.1f, 0.4f);
    cv_flat(9.5f, hy + 30, 1.6f, 1.8f, (uint8_t)(R_FLAME + 11));
    float fw[8] = { 30, hy + 20, 12, hy + 2 + up, 10, hy + 18, 22, hy + 27 };   /* the far wing */
    cv_poly(fw, 4, R_BRICK, 0.03f, 0.15f, hy + 2 + up, 26);
    cv_capsule(30, hy + 35, 36, hy + 43, 2.2f, R_BRICK, 0.08f, 0.4f);   /* the far leg */
    cv_capsule(36, hy + 43, 32, 55, 1.4f, R_BRICK, 0.08f, 0.4f); cv_capsule(32, 55, 35, 61, 1.1f, R_BRICK, 0.08f, 0.4f);
    cv_ellipse(32, hy + 24, 5.0f, 6.0f, R_BRICK, 0.15f, 0.6f);     /* chest, hunched forward */
    cv_ellipse(31, hy + 32, 4.0f, 4.5f, R_BRICK, 0.1f, 0.45f);
    cv_capsule(32, hy + 35, 39, hy + 44, 2.4f, R_BRICK, 0.12f, 0.55f);   /* the near leg */
    cv_capsule(39, hy + 44, 35, 55, 1.6f, R_BRICK, 0.12f, 0.5f); cv_capsule(35, 55, 39, 61, 1.2f, R_BRICK, 0.12f, 0.5f);
    for (int c = -1; c <= 1; c++) cv_line(39, 61, 42 + c * 1.5f, 62, sh(R_BONE, 0.5f));
    float nw[8] = { 31, hy + 20, 16, hy + 4 + up, 13, hy + 20, 24, hy + 29 };   /* the near wing */
    cv_poly(nw, 4, R_BRICK, 0.06f, 0.26f, hy + 4 + up, 25);
    cv_capsule(31, hy + 20, 16, hy + 4 + up, 0.9f, R_BRICK, 0.1f, 0.4f);
    cv_capsule(34, hy + 21, 40, hy + 29, 1.5f, R_BRICK, 0.15f, 0.55f);   /* an arm, out ahead, talons */
    cv_capsule(40, hy + 29, 45, hy + 33, 1.2f, R_BRICK, 0.15f, 0.5f);
    for (int c = -1; c <= 1; c++) cv_line(45, hy + 33, 47 + c * 0.8f, hy + 36, sh(R_BONE, 0.6f));
    imp_crust(hy + 17, hy + 45, 6, 525);
    grid_stamp(IMP_SIDE, 15, 15, 28, (int)hy, R_BRICK, R_BONE, (uint8_t)(R_FLAME + 15), 0);
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
    { float ev[6] = { 41, 19, 34, 15, 40, 23 }; cv_poly(ev, 3, R_STONE, 0.15f, 0.55f, 15, 8); }   /* a fin ear, swept back */
    cv_capsule(42, 15, 40, 10, 1.7f, R_STONE, 0.2f, 0.8f);         /* a bull's horn, up and forward */
    cv_capsule(40, 10, 44, 7, 1.2f, R_STONE, 0.2f, 0.75f);
    grid_stamp(GARG_SIDE, 14, 15, 38, 14, R_STONE, R_STONE, (uint8_t)(R_FLAME + 12), 0);
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
    static const int BODY[NPF] = { R_BONE, R_WARM, R_IRON, R_WOOD, R_PURPLE, R_BRICK, R_STONE, R_IRON, R_PURPLE, R_GHOST };
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
            /* the rank takes the shadows and the middle; the lights keep the stuff it is made of, so bone still looks
             * like bone (docs/InitialArtPass.md, A6). The black keep less of it, the gilded a little more gold. */
            static const int KEEP[NRANK] = { 16, 11, 11, 11, 13, 12 };
            if (to != R_BONE || BODY[f] == R_BONE)
                for (int k = KEEP[r]; k < 16; k++) lut[BODY[f] + k] = (uint8_t)(BODY[f] + k);
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
/* ---- The props (docs/InitialArtPass.md, section 14): drawn with each level, so that its age and run II's water reach
 * them; the common ones have several looks (crawl_alt), picked where they are drawn by where each stands ---- */
int16_t crawl_alt[NSPR][NALT];

/* the prop skull: the master skull cut down to 11 by 12, for piles, heaps and totems (hex R_BONE, 0 the hollows) */
static const char *const PROP_SKULL[12] = {
    "...7abba8..", ".7ceffedc8.", "8dfffeeddb7", "7cdeeeddcb6", "6aeedbdeda5", "8000bc70005",
    "a1009081005", "6dcb020bba5", ".9b60706a8.", "..8e3f3d6..", "..4000003..", "...7cbba5..",
};
/* a prop skull with its top left at (x0, y0): mirror flips it, jaw 0 leaves its jaw off, dim darkens it (deeper in a pile),
 * glow fills its sockets with a candle's light inside (a step of the fire, 0 none) */
static void prop_skull(int x0, int y0, int mirror, int jaw, int dim, int glow) {
    for (int y = 0; y < (jaw ? 12 : 10); y++)
        for (int x = 0; x < 11; x++) {
            int gx = mirror ? 10 - x : x;
            char ch = PROP_SKULL[y][gx];
            if (ch == '.') continue;
            if (ch == '0') {
                int sock = (y == 5 || y == 6) && ((gx >= 1 && gx <= 3) || (gx >= 7 && gx <= 9));
                cv_px(x0 + x, y0 + y, glow && sock ? (uint8_t)(R_FLAME + glow - (y == 5)) : 0);
                continue;
            }
            int v = ch <= '9' ? ch - '0' : ch - 'a' + 10;
            cv_px(x0 + x, y0 + y, (uint8_t)(R_BONE + clampi(v - dim, 1, 15)));
        }
}
/* a long bone from (x0, y0) to (x1, y1), knuckled as the skeletons' bones are: a femur's ball at its head (femur), or
 * snapped off at the far end, the marrow dark in it (broken); old stains down it; dim darkens it */
static void prop_long(float x0, float y0, float x1, float y1, float r, int femur, int broken, float dim, uint32_t seed) {
    float L = hypotf(x1 - x0, y1 - y0) + 0.01f, ux = (x1 - x0) / L, uy = (y1 - y0) / L, px = -uy, py = ux;
    float lo = fmaxf(0.04f, 0.24f - dim * 0.5f), hi = 0.92f - dim;
    if (broken) {
        cv_capsule(x0, y0, x1, y1, r * 0.85f, R_BONE, lo, hi);
        for (int e = -1; e <= 1; e += 2) cv_ellipse(x0 + px * r * 0.7f * (float)e, y0 + py * r * 0.7f * (float)e, r * 1.05f, r * 1.05f, R_BONE, lo, hi);
        for (int k = -1; k <= 1; k++)                               /* splintered */
            cv_px((int)(x1 + px * (float)k * r * 0.7f + ux * (k == 0 ? 1.4f : 0.3f)), (int)(y1 + py * (float)k * r * 0.7f + uy * (k == 0 ? 1.4f : 0.3f)), sh(R_BONE, hi * 0.75f));
        cv_px((int)x1, (int)y1, sh(R_WARM, 0.08f));
    } else cv_bone(x0, y0, x1, y1, r, lo, hi);
    if (femur) {                                                    /* its head, on a neck off to one side */
        cv_capsule(x0, y0, x0 - ux * r * 0.8f + px * r * 1.4f, y0 - uy * r * 0.8f + py * r * 1.4f, r * 0.7f, R_BONE, lo, hi);
        cv_ellipse(x0 - ux * r * 1.1f + px * r * 1.9f, y0 - uy * r * 1.1f + py * r * 1.9f, r * 1.2f, r * 1.2f, R_BONE, lo + 0.06f, hi);
    }
    for (int k = 0; k < 3; k++) {                                   /* old stains */
        float t = 0.2f + 0.6f * h01(k, 1, seed);
        cv_px((int)(x0 + (x1 - x0) * t), (int)(y0 + (y1 - y0) * t), sh(R_BONE, lo + 0.1f));
    }
}
/* a rib: a thin arc about (cx, cy) from angle a0 to a1, lit along its top, darker as it goes */
static void prop_rib(float cx, float cy, float rx, float ry, float a0, float a1, float dim) {
    int n = (int)(fabsf(a1 - a0) * fmaxf(rx, ry)) + 2;
    for (int i = 0; i <= n; i++) {
        float a = a0 + (a1 - a0) * (float)i / (float)n, x = cx + cosf(a) * rx, y = cy + sinf(a) * ry;
        cv_px((int)x, (int)y, sh(R_BONE, 0.82f - dim - 0.25f * (float)i / (float)n)); cv_px((int)x, (int)y + 1, sh(R_BONE, fmaxf(0.05f, 0.35f - dim)));
    }
}
static void prop_vertebra(float x, float y, float dim) {
    cv_ellipse(x, y, 2.0f, 1.4f, R_BONE, fmaxf(0.05f, 0.3f - dim), 0.85f - dim);
    cv_px((int)x, (int)y - 2, sh(R_BONE, 0.72f - dim)); cv_px((int)x - 2, (int)y - 1, sh(R_BONE, 0.6f - dim)); cv_px((int)x + 2, (int)y - 1, sh(R_BONE, 0.5f - dim));
    cv_px((int)x, (int)y, sh(R_WARM, 0.1f));                        /* the hole for the cord */
}
/* a jaw fallen from its skull, lying open: a U of bone, the teeth along it */
static void prop_jaw(float x, float y) {
    for (int i = -5; i <= 5; i++) {
        float yy = y - 0.11f * (float)(25 - i * i);
        cv_px((int)x + i, (int)yy, sh(R_BONE, 0.78f - 0.04f * (float)(i + 5))); cv_px((int)x + i, (int)yy + 1, sh(R_BONE, 0.35f));
        if (abs(i) < 4 && (i & 1)) cv_px((int)x + i, (int)yy - 1, sh(R_BONE, 0.9f));   /* teeth, one gone */
    }
    cv_px((int)x - 5, (int)y - 3, sh(R_BONE, 0.7f)); cv_px((int)x + 5, (int)y - 3, sh(R_BONE, 0.5f));   /* its ends, rising */
}
static void prop_fingers(float x, float y, uint32_t seed) {
    for (int k = 0; k < 4; k++) {
        float a = h01(k, 1, seed) * PI_F, l = 2.0f + h01(k, 2, seed) * 1.5f, bx = x + (float)k * 2.2f, by = y + h01(k, 3, seed) * 2.0f;
        cv_line(bx, by, bx + cosf(a) * l, by + sinf(a) * l * 0.4f, sh(R_BONE, 0.7f)); cv_px((int)bx, (int)by, sh(R_BONE, 0.85f));
    }
}
/* the level's age on a prop: stains in the bone, and moss low down once its decay is far on */
static void prop_age(uint32_t seed) {
    for (int y = 0; y < TS; y++)
        for (int x = 0; x < TS; x++) {
            uint8_t *c = &cv_sprite[y * TS + x];
            if (*c <= R_BONE + 2 || *c >= R_BONE + 16) continue;
            if (h01(x, y, seed) < g_decay * 0.14f) *c = (uint8_t)(*c - 2);
            else if (g_decay > 0.45f && y > 46 && fbm((float)x / 4.0f, (float)y / 3.0f, 16, seed + 1) > 1.18f - g_decay * 0.45f) *c = sh(R_MOSS, 0.2f + 0.25f * h01(x, y, seed + 2));
        }
}
/* the run's water on a finished prop (docs/InitialArtPass.md section 14, B3): what stands gets a silt line, darker and
 * duller below it, weed hanging at it; iron rusts; everything drips under its overhangs; a wet sheen low down. Lights'
 * flames and wax are left dry. Over crawl_spr[id] as stored, column-major */
static void prop_wash(int id, int hangs, uint32_t seed) {
    uint8_t *sp = crawl_spr[id];
    int top = TS, foot = -1;
    for (int x = 0; x < TS; x++) for (int y = 0; y < TS; y++) if (sp[x * TS + y] != C_CLEAR) { if (y < top) top = y; if (y > foot) foot = y; }
    if (foot < 0) return;
    int line = foot - (int)((float)(foot - top) * 0.28f) - 1;
    #define WP(x, y) sp[(x) * TS + (y)]
    #define WON(x, y) ((unsigned)(x) < (unsigned)TS && (unsigned)(y) < (unsigned)TS && WP(x, y) != C_CLEAR)
    for (int x = 0; x < TS; x++)
        for (int y = 0; y < TS; y++) {
            uint8_t c = WP(x, y);
            if (c == C_CLEAR || c == 0 || IS_FULLBRIGHT(c) || (c >= R_WARM && c <= R_WARM + 1)) continue;
            int ramp = (c - 1) / 16 * 16 + 1, v = c - ramp;
            if (ramp == R_IRON && h01(x, y, seed) < 0.35f) { WP(x, y) = sh(R_BRICK, 0.15f + 0.04f * (float)v); continue; }   /* rust */
            if (!hangs && y > line) {                               /* below the silt line: dulled, a silt bloom */
                WP(x, y) = h01(x, y, seed + 1) < 0.55f ? sh(R_WARM, 0.12f + 0.025f * (float)v) : (uint8_t)(ramp + clampi(v - 3, 1, 15));
                if (y < foot - 1 && !WON(x - 1, y - 1) && h01(x, y, seed + 2) < 0.4f) WP(x, y) = sh(R_GHOST, 0.42f);   /* a wet sheen */
            } else if (!hangs && y == line) WP(x, y) = sh(R_WARM, 0.55f);   /* the tide line */
        }
    if (!hangs)
        for (int x = 0; x < TS; x++)                                /* weed hanging at the line */
            if (WON(x, line) && h01(x, 7, seed + 3) < 0.18f)
                for (int k = 1; k <= 2 + (int)(h01(x, 8, seed + 3) * 4.0f); k++) if (WON(x, line + k)) WP(x, line + k) = sh(R_MOSS, 0.32f - 0.04f * (float)k);
    for (int x = 0; x < TS; x++)                                    /* drips under the overhangs */
        for (int y = top; y < TS - 3; y++)
            if (WON(x, y) && !WON(x, y + 1) && !IS_FULLBRIGHT(WP(x, y)) && h01(x, y, seed + 4) < 0.07f) {
                WP(x, y + 1) = sh(R_GHOST, 0.62f);
                if (!WON(x, y + 3) && h01(x, y, seed + 5) < 0.5f) WP(x, y + 3) = sh(R_GHOST, 0.75f);
            }
    #undef WP
    #undef WON
}

/* P1: bones and skulls. Litter by a hall's wall: a femur across a snapped shin, ribs, a jaw; a ribcage open on its spine and
 * a skull; two femurs crossed under a skull, the Order's memento mori. look 0-2 */
static void spr_bonelitter(int id, int look) {
    cv_begin(cv_sprite, TS, TS);
    cv_flat(32, 61.5f, 21, 2.4f, R_WARM + 1);
    if (look == 0) {
        prop_long(30, 61, 52, 54, 1.6f, 0, 1, 0.12f, 13);           /* a shin, snapped */
        prop_long(10, 55, 40, 60, 2.0f, 1, 0, 0.0f, 11);            /* a femur across it */
        for (int k = 0; k < 2; k++) prop_rib(22.0f + (float)k * 5.0f, 63.0f, 6.0f, 7.0f, PI_F * 1.1f, PI_F * 1.75f, 0.0f);
        prop_jaw(51, 62); prop_vertebra(44, 61, 0.05f);
    } else if (look == 1) {
        for (int k = 0; k < 4; k++) prop_rib(17.0f + (float)k * 4.5f, 60.0f, 4.5f, 8.0f - (float)k * 0.8f, PI_F * 1.02f, PI_F * 1.98f, 0.04f * (float)k);   /* a ribcage open on its back */
        for (int k = 0; k < 6; k++) prop_vertebra(12.0f + (float)k * 4.0f, 60.0f, 0.04f * (float)k);   /* its spine */
        prop_skull(37, 49, 1, 1, 0, 0);
        prop_long(48, 61, 58, 58, 1.3f, 0, 1, 0.15f, 17);
    } else {
        prop_long(17, 60, 46, 50, 1.5f, 1, 0, 0.12f, 19);
        prop_long(17, 50, 46, 60, 1.5f, 1, 0, 0.0f, 21);
        prop_skull(26, 45, 0, 1, 0, 0);
        prop_vertebra(52, 61, 0.1f); prop_fingers(8, 60, 23);
    }
    prop_age(25 + (uint32_t)look);
    cv_outline(0);
    cv_store(id);
}

/* a charnel heap: a mound of long bones laid every way over the dark between them, skulls sunk in it, one crowning it; look
 * 1 has a ribcage arching out of its side and a skull rolled off at its foot */
static void spr_boneheap(int id, int look) {
    cv_begin(cv_sprite, TS, TS);
    cv_flat(32, 61.5f, 28, 3.5f, R_WARM + 1);
    for (int y = 34; y < 62; y++)                                   /* the dark of the heap, between the bones */
        for (int x = 4; x < 60; x++) {
            float dx = ((float)x + 0.5f - 32.0f) / 27.0f, h = 61.0f - 25.0f * (1.0f - dx * dx) * (0.85f + 0.15f * h01(x / 3, look, 27));
            if ((float)y < h) continue;
            cv_px(x, y, h01(x, y, 29) < 0.3f ? sh(R_BONE, 0.14f) : sh(R_WARM, 0.08f));
        }
    for (int layer = 0; layer < 3; layer++)                         /* the bones, the far ones dim */
        for (int i = 0; i < 9; i++) {
            uint32_t sd = 31u + (uint32_t)(layer * 17 + i + look * 101);
            float cx = 8.0f + 48.0f * h01(i, 1, sd), dx = (cx - 32.0f) / 27.0f, top = 61.0f - 25.0f * (1.0f - dx * dx);
            float cy = top + 3.0f + (60.0f - top - 3.0f) * h01(i, 2, sd), a = (h01(i, 3, sd) - 0.5f) * 2.4f, l = 6.0f + 6.0f * h01(i, 4, sd);
            if (cy > 60.5f) cy = 60.5f;
            prop_long(cx - cosf(a) * l, cy - sinf(a) * l, cx + cosf(a) * l, cy + sinf(a) * l, 1.25f, h01(i, 5, sd) < 0.3f, h01(i, 6, sd) < 0.25f, 0.32f - 0.14f * (float)layer, sd);
        }
    prop_skull(9, 47, 1, 0, 3, 0);                                  /* skulls sunk in it */
    prop_skull(41, 48, 0, 0, 2, 0);
    if (look == 1) {
        for (int k = 0; k < 4; k++) prop_rib(50.0f, 50.0f + (float)k * 2.5f, 7.0f, 4.0f, PI_F * 1.15f, PI_F * 1.95f, 0.05f * (float)k);   /* a ribcage arching out */
        prop_skull(2, 51, 1, 1, 0, 0);                              /* one rolled off */
    }
    prop_skull(26, 28, look, 1, 0, 0);                              /* and one crowning it */
    prop_age(33 + (uint32_t)look);
    cv_outline(0);
    cv_store(id);
}

/* skulls as an ossuary keeps them. look 0: stacked in courses against the wall, femurs' ends between, stepping in to one on
 * top; 1: a loose pyramid, some jawless, one rolled off, a cross painted on one; 2: a pile with a candle burnt out on
 * it, its wax run down over them */
static void femur_ends(int x0, int x1, int y) {                    /* a course of femurs laid end out: knuckles in two rows, bonded */
    for (int r = 0; r < 2; r++)
        for (int x = x0 + (r ? 2 : 0); x <= x1; x += 4) {
            cv_ellipse((float)x + 1.5f, (float)y + 1.2f + (float)r * 2.6f, 1.9f, 1.5f, R_BONE, 0.2f, 0.78f - 0.06f * (float)r);
            cv_px(x + 1, y + 1 + r * 3, sh(R_BONE, 0.28f));
        }
}
static void spr_skullstack(int id, int look) {
    cv_begin(cv_sprite, TS, TS);
    cv_flat(32, 62, 28, 2.5f, R_WARM + 1);
    if (look == 0) {
        femur_ends(5, 57, 56);
        for (int k = 0; k < 5; k++) prop_skull(5 + k * 11, 44, k & 1, k != 3, (k * 3) % 2, 0);
        femur_ends(9, 53, 39);
        for (int k = 0; k < 4; k++) prop_skull(10 + k * 11, 27, (k + 1) & 1, k != 0, 1, 0);
        femur_ends(15, 47, 22);
        prop_skull(26, 10, 0, 1, 0, 0);
        for (int k = 0; k < 5; k++) cv_px(37, 28 + k, sh(R_BLOOD, 0.4f - 0.03f * (float)k));   /* a cross painted on one's brow, in red ochre */
        for (int k = -2; k <= 2; k++) cv_px(37 + k, 30, sh(R_BLOOD, 0.4f));
    } else {
        static const int ROW[4][5] = { { 4, 15, 26, 37, 48 }, { 9, 20, 31, 42, -1 }, { 15, 26, 37, -1, -1 }, { 21, 32, -1, -1, -1 } };
        for (int r = 0; r < 4; r++)
            for (int k = 0; k < 5 && ROW[r][k] >= 0; k++) {
                int x = ROW[r][k] + (int)(h01(r, k, 43 + (uint32_t)look) * 3.0f) - 1, y = 50 - r * 10 + (int)(h01(k, r, 45) * 2.0f);
                prop_skull(x, y, (r + k + look) & 1, r == 0 || h01(r, k, 47) < 0.5f, r == 0 ? 0 : 1 + (k & 1), 0);
            }
        if (look == 1) {
            prop_skull(51, 51, 1, 1, 0, 0);                         /* rolled off */
            for (int k = 0; k < 4; k++) cv_px(27, 31 + k, sh(R_BLOOD, 0.42f));   /* a cross painted on one's brow, in red ochre */
            for (int k = -1; k <= 1; k++) cv_px(27 + k, 32, sh(R_BLOOD, 0.42f));
        } else {
            for (int y = 9; y < 21; y++) for (int x = 31; x < 34; x++) cv_px(x, y, sh(R_BONE, 0.55f + 0.2f * (x == 31) - 0.01f * (float)(y - 9)));   /* a candle burnt low */
            cv_px(32, 8, 0); cv_px(32, 7, sh(R_IRON, 0.3f));                                            /* its wick, out */
            for (int k = 0; k < 7; k++) {                           /* its wax run down over the skulls */
                int x = 27 + (int)(h01(k, 1, 49) * 11.0f), l = 4 + (int)(h01(k, 2, 49) * 14.0f);
                for (int j = 0; j < l; j++) if (cv_sprite[(20 + j) * TS + x] != C_CLEAR) cv_px(x, 20 + j, sh(R_BONE, 0.82f - 0.02f * (float)j));
            }
            for (int x = 28; x < 37; x++) cv_px(x, 21, sh(R_BONE, 0.78f));
        }
    }
    prop_age(51 + (uint32_t)look);
    cv_outline(0);
    cv_store(id);
}

/* a candle at (x, by), h tall, its flame in a frame; wax run down it */
static void prop_candle(float x, float by, float h, int frame, int seed) {
    for (int y = (int)(by - h); y <= (int)by; y++) { cv_px((int)x, y, sh(R_BONE, 0.72f)); cv_px((int)x + 1, y, sh(R_BONE, 0.48f)); }
    cv_px((int)x - 1, (int)(by - h) + 2 + (seed & 1), sh(R_BONE, 0.8f)); cv_px((int)x + 2, (int)(by - h) + 3, sh(R_BONE, 0.6f));   /* runs of wax */
    flame(x + 1.0f, by - h - 1.0f, 5.5f + (float)((frame + seed) & 1), 1.7f, (frame ? 0.7f : -0.6f) * (seed & 1 ? 1.0f : -1.0f));
}
/* the totem: a black post bound in iron and rope, skulls threaded on it with a candle's light inside each, a crossbar of iron
 * with candles at its ends and one on top, their wax run down. look 1: a great skull on top crowned with candles, two
 * smaller ones hung below it on chains, a strip of the Order's banner nailed to the post. frame 0-1, the flames */
static void spr_totempost(int id, int look, int frame) {
    cv_begin(cv_sprite, TS, TS);
    float cx = 32;
    cv_flat(cx, 62, 10, 1.7f, R_WARM + 1);
    cv_capsule(cx, look ? 20.0f : 14.0f, cx, 62, 2.4f, R_WOOD, 0.06f, 0.42f);   /* the post, black with age */
    for (int b = 0; b < 3; b++) {                                   /* iron bands, and rope lashed below them */
        int y = 58 - b * (look ? 13 : 15);
        for (int x = (int)cx - 3; x <= (int)cx + 3; x++) cv_px(x, y, sh(R_IRON, 0.55f - 0.07f * (float)abs(x - (int)cx + 1)));
        cv_px((int)cx + 1, y, sh(R_IRON, 0.85f));
        for (int k = 1; k <= 2; k++) for (int x = (int)cx - 2; x <= (int)cx + 2; x++) cv_px(x, y + k, ((x + k) & 1) ? sh(R_WARM, 0.55f) : sh(R_WARM, 0.32f));
    }
    if (!look) {
        for (int k = 0; k < 3; k++) prop_skull((int)cx - 5, 47 - k * 12, k & 1, k == 0, k, 4 + ((frame + k) & 1));   /* threaded on it, lit inside */
        cv_capsule(cx - 13, 23, cx + 13, 23, 1.1f, R_IRON, 0.15f, 0.6f);   /* the crossbar */
        for (int s = -1; s <= 1; s += 2) prop_candle(cx + (float)s * 12.0f, 22, 6, frame, s > 0);
        prop_candle(cx - 0.5f, 13, 6, frame, 2);
        for (int k = 0; k < 5; k++) {                               /* wax run down the skulls from above */
            int x = (int)cx - 3 + (int)(h01(k, 1, 53) * 7.0f), y0 = 24, l = 3 + (int)(h01(k, 2, 53) * 8.0f);
            for (int j = 0; j < l; j++) if (cv_sprite[(y0 + j) * TS + x] != C_CLEAR) cv_px(x, y0 + j, sh(R_BONE, 0.8f - 0.02f * (float)j));
        }
    } else {
        for (int s = -1; s <= 1; s += 2) {                          /* two hung on chains from a bar */
            float hx = cx + (float)s * 10.0f;
            for (int y = 25; y < 33; y++) cv_px((int)hx, y, sh(R_IRON, (y & 1) ? 0.6f : 0.25f));
            prop_skull((int)hx - 5, 32, s > 0, 0, 1, 0);
        }
        cv_capsule(cx - 11, 24, cx + 11, 24, 1.0f, R_IRON, 0.15f, 0.55f);
        for (int y = 30; y < 44; y++)                               /* a strip of the Order's banner, nailed on and torn */
            for (int x = (int)cx - 2; x <= (int)cx + 3; x++) {
                if (y > 40 && h01(x, y, 55) < (float)(y - 40) / 4.0f) continue;
                cv_px(x, y, sh(R_BLOOD, 0.2f + 0.08f * (float)(x < (int)cx + 1)));
            }
        cv_px((int)cx, 31, sh(R_IRON, 0.8f)); cv_px((int)cx + 1, 35, (uint8_t)(R_GOLD + 5)); cv_px((int)cx + 1, 36, (uint8_t)(R_GOLD + 4)); cv_px((int)cx, 36, (uint8_t)(R_GOLD + 4)); cv_px((int)cx + 2, 36, (uint8_t)(R_GOLD + 3)); cv_px((int)cx + 1, 37, (uint8_t)(R_GOLD + 4));
        skull_stamp(cx, 13, 0, 0, 0);                               /* the great skull on top */
        for (int y = 9; y < 14; y++) for (int x = (int)cx - 4; x <= (int)cx + 4; x++) {   /* candlelight in its sockets */
            uint8_t c = cv_sprite[y * TS + x];
            if (c == 0) cv_px(x, y, (uint8_t)(R_FLAME + 3 + ((frame + x) & 1)));
        }
        for (int k = -1; k <= 1; k++) prop_candle(cx + (float)k * 4.5f - 0.5f, 6.0f + (float)abs(k), 4 + (k == 0), frame, k + 1);   /* candles in its crown */
        for (int k = 0; k < 6; k++) {                               /* their wax run down its face */
            int x = (int)cx - 6 + (int)(h01(k, 1, 57) * 12.0f), l = 2 + (int)(h01(k, 2, 57) * 9.0f);
            for (int j = 0; j < l; j++) if (cv_sprite[(7 + j) * TS + x] != C_CLEAR && cv_sprite[(7 + j) * TS + x] != 0) cv_px(x, 7 + j, sh(R_BONE, 0.85f - 0.02f * (float)j));
        }
    }
    prop_age(59 + (uint32_t)look);
    cv_outline(0);
    cv_store(id);
}

/* P2: the lights. A flame with a white heart; the smoke over it is smoke_up's, after the outline */
static void prop_flame(float x, float by, float h, float w, float sway) {
    flame(x, by, h, w, sway);
    cv_px((int)x, (int)(by - h * 0.35f), (uint8_t)(R_FLAME + 15)); cv_px((int)x, (int)(by - h * 0.25f), (uint8_t)C_WHITE);
}
/* a thread of smoke going up from (x, y), grey and thinning; drawn after the outline, only where nothing is */
static void smoke_up(float x, float y, float h, uint32_t seed) {
    for (int k = 0; k < (int)h; k++) {
        float t = (float)k / h;
        int px = (int)(x + sinf(t * 6.0f + (float)(seed & 7)) * (0.5f + 2.5f * t)), py = (int)(y - (float)k);
        if ((unsigned)px >= (unsigned)TS || (unsigned)py >= (unsigned)TS || cv_sprite[py * TS + px] != C_CLEAR || h01(px, py, seed) < t * 0.8f) continue;
        cv_px(px, py, sh(R_IRON, 0.5f - 0.25f * t));
    }
}
/* a church candle standing at (x, by), h tall and r round: its wax lit from the left, its rim melted into a cup, wax run down
 * its front in drops; lit: its flame (frame and k vary it), gutter: low and leaning */
static void wax_candle(float x, float by, float h, float r, int lit_, int frame, int k, int gutter) {
    for (int y = (int)(by - h); y <= (int)by; y++)
        for (int xx = (int)(x - r) - 1; xx <= (int)(x + r) + 1; xx++) {
            float nx = ((float)xx + 0.5f - x) / r;
            if (fabsf(nx) > 1.0f) continue;
            cv_px(xx, y, sh(R_BONE, 0.42f + 0.48f * lit(nx, 0.0f, sqrtf(1.0f - nx * nx)) + (y == (int)(by - h) ? 0.08f : 0.0f)));
        }
    int top = (int)(by - h);
    cv_px((int)x, top, sh(R_BONE, 0.3f)); cv_px((int)x + 1, top, sh(R_BONE, 0.35f));   /* the melted cup */
    for (int d = 0; d < 2; d++) {                                   /* runs of wax down its front */
        int dx = (int)x - 1 + d * 2 + ((k + d) & 1), l = 2 + (int)(h01(k, d, 701) * (h * 0.6f));
        for (int j = 1; j <= l; j++) cv_px(dx, top + j, sh(R_BONE, 0.95f - 0.02f * (float)j));
        cv_px(dx, top + l + 1, sh(R_BONE, 0.7f)); cv_px(dx + 1, top + l + 1, sh(R_BONE, 0.3f));   /* its drop, and its shadow */
    }
    cv_px((int)x, top - 1, sh(R_IRON, 0.15f));                      /* the wick */
    if (lit_) prop_flame(x + 0.5f, (float)top - 1.5f, gutter ? 3.5f : 5.5f + (float)((frame + k) & 1), gutter ? 1.3f : 1.8f, gutter ? 1.6f : (((frame + k) % 3) - 1) * 0.7f);
}
/* candles: three looks. 0: church candles of three heights standing in their own pooled wax, one guttering, a stub gone out;
 * 1: two set on a skull's crown, their wax run over its brow; 2: an iron pricket dish, one candle fallen and out in the wax */
static void spr_candlecluster(int id, int look) {
    cv_begin(cv_sprite, TS, TS);
    cv_flat(32, 62, 17, 2.0f, R_WARM + 1);
    if (look == 2) {                                                /* the iron dish on its feet */
        for (int s = -1; s <= 1; s += 2) cv_capsule(32.0f + (float)s * 10.0f, 59.0f, 32.0f + (float)s * 12.0f, 62.0f, 0.9f, R_IRON, 0.2f, 0.6f);
        cv_ellipse(32, 58, 15.0f, 3.2f, R_IRON, 0.15f, 0.62f);
        for (int x = 18; x <= 46; x += 4) cv_px(x, 56, sh(R_IRON, 0.75f));   /* its rim's beading */
    }
    float py = look == 2 ? 57.5f : 60.5f;
    for (int y = (int)py - 2; y <= (int)py + 2; y++)                /* the pool of wax */
        for (int x = 16; x <= 48; x++) {
            float nx = ((float)x + 0.5f - 32.0f) / 15.0f, ny = ((float)y + 0.5f - py) / 2.2f;
            if (nx * nx + ny * ny + 0.25f * sinf((float)x * 0.9f) > 1.0f) continue;
            cv_px(x, y, sh(R_BONE, 0.5f + 0.25f * (ny < 0) + 0.08f * h01(x, y, 703)));
        }
    if (look == 0) {
        wax_candle(33, 60, 24, 2.8f, 1, 0, 1, 0);
        wax_candle(24, 61, 15, 2.4f, 1, 0, 2, 0);
        wax_candle(42, 61, 10, 2.4f, 1, 0, 3, 1);                   /* guttering */
        wax_candle(29, 62, 4, 1.8f, 0, 0, 4, 0);                    /* a stub, out */
    } else if (look == 1) {
        prop_skull(26, 47, 0, 1, 0, 0);
        wax_candle(29, 47, 11, 2.0f, 1, 0, 5, 0);
        wax_candle(35, 48, 7, 2.0f, 1, 0, 6, 0);
        for (int k = 0; k < 6; k++) {                               /* their wax run over its brow, one into a socket */
            int x = 27 + (int)(h01(k, 1, 705) * 9.0f), l = 2 + (int)(h01(k, 2, 705) * 6.0f) + (k == 2 ? 3 : 0);
            for (int j = 0; j < l; j++) if (cv_sprite[(48 + j) * TS + x] != C_CLEAR) cv_px(x, 48 + j, sh(R_BONE, 0.92f - 0.02f * (float)j));
        }
    } else {
        wax_candle(25, 57, 14, 2.3f, 1, 0, 7, 0);
        wax_candle(34, 57, 7, 2.3f, 1, 0, 8, 1);
        wax_candle(41, 57, 18, 2.3f, 1, 0, 9, 0);
        for (int x = 30; x <= 44; x++) { cv_px(x, 60, sh(R_BONE, 0.7f - 0.01f * (float)(x - 30))); cv_px(x, 61, sh(R_BONE, 0.38f)); }   /* one fallen, out */
        cv_px(45, 60, sh(R_IRON, 0.15f));
    }
    cv_outline(0);
    if (look == 0) smoke_up(43.5f, 47.0f, 10.0f, 707);              /* the guttering one smokes */
    if (look == 2) smoke_up(35.5f, 46.0f, 9.0f, 709);
    cv_store(id);
}

/* the candle stand (run I's own, the halls' light on its first levels): a twisted iron shaft with knops on a tripod of
 * claw feet, a tray with a tracery rim, three candles on its prickets, wax hanging from it in stalactites */
static void spr_prickstand(int id, int frame) {
    cv_begin(cv_sprite, TS, TS);
    cv_flat(32, 62, 11, 1.6f, R_WARM + 1);
    for (int s = -1; s <= 1; s++) {                                 /* the feet, curving out, clawed */
        float fx = 32.0f + (float)s * 9.0f;
        cv_capsule(32, 53, (32.0f + fx) * 0.5f, 59.5f - (float)abs(s) * 0.5f, 1.1f, R_IRON, 0.15f, 0.65f);
        cv_capsule((32.0f + fx) * 0.5f, 59.5f, fx, 62.0f - (s == 0 ? 1.0f : 0.0f), 0.9f, R_IRON, 0.15f, 0.6f);
        for (int t = -1; t <= 1; t++) cv_px((int)fx + t + s, 62, sh(R_IRON, 0.5f));
    }
    for (int y = 31; y < 54; y++) cv_px(32, y, sh(R_IRON, ((y / 2) & 1) ? 0.75f : 0.3f)), cv_px(31, y, sh(R_IRON, ((y / 2) & 1) ? 0.35f : 0.55f)), cv_px(33, y, sh(R_IRON, 0.18f));   /* twisted */
    cv_ellipse(32, 50, 2.6f, 1.6f, R_IRON, 0.2f, 0.8f); cv_ellipse(32, 41, 2.2f, 1.4f, R_IRON, 0.2f, 0.8f);   /* knops */
    cv_ellipse(32, 30, 10.5f, 2.4f, R_IRON, 0.12f, 0.7f);           /* the tray */
    for (int x = 23; x <= 41; x += 3) { cv_px(x, 27, sh(R_IRON, 0.6f)); cv_px(x - 1, 28, sh(R_IRON, 0.45f)); cv_px(x + 1, 28, sh(R_IRON, 0.45f)); }   /* its tracery rim */
    for (int k = 0; k < 6; k++) {                                   /* wax hung from it */
        int x = 24 + (int)(h01(k, 1, 711) * 16.0f), l = 2 + (int)(h01(k, 2, 711) * 5.0f);
        for (int j = 1; j <= l; j++) cv_px(x, 32 + j, sh(R_BONE, 0.85f - 0.06f * (float)j));
    }
    wax_candle(26, 28, 8, 1.8f, 1, frame, 0, 0); wax_candle(32, 28, 12, 2.0f, 1, frame, 1, 0); wax_candle(38, 28, 9, 1.8f, 1, frame, 2, 0);
    cv_outline(0);
    cv_store(id);
}

/* the chandelier: an iron corona hung on chains from a hook, fleurons along its rim, candles on prickets round it (those
 * behind dimmer, drawn first), wax hanging from it in stalactites, and a skull hung on a chain at its middle */
static void spr_corona(int id, int frame) {
    cv_begin(cv_sprite, TS, TS);
    float cx = 32, ry = 24, rx = 21;
    chain_down(cx, 0, 7);
    cv_ellipse(cx, 8, 1.6f, 1.6f, R_IRON, 0.2f, 0.8f);              /* the hook's ring */
    for (int k = 0; k < 4; k++) {                                   /* chains down to the ring, the back two dim */
        float ang = PI_F * 0.25f + (float)k * PI_F * 0.5f, x = cx + rx * cosf(ang) * 0.95f, y = ry + 3.5f * sinf(ang);
        if (sinf(ang) < 0.0f) for (int i = 0; i <= 16; i++) { float t = (float)i / 16.0f; cv_px((int)(cx + (x - cx) * t), (int)(9.0f + (y - 9.0f) * t), sh(R_IRON, (i & 1) ? 0.3f : 0.15f)); }
    }
    for (int k = 0; k < 9; k++) {                                   /* candles behind */
        float ang = TAU_F * (float)k / 9.0f + 0.2f, sz = sinf(ang);
        if (sz >= -0.1f) continue;
        wax_candle(cx + rx * cosf(ang), ry + 3.5f * sz - 1.0f, 5.0f + 3.0f * h01(k, 1, 713), 1.2f, 1, frame, k, 0);
    }
    for (int a = 0; a < 90; a++) {                                  /* the ring: a band, fleurons on its rim */
        float ang = TAU_F * (float)a / 90.0f, sz = sinf(ang);
        int x = (int)(cx + rx * cosf(ang)), y = (int)(ry + 3.5f * sz);
        cv_px(x, y, sh(R_IRON, sz > 0 ? 0.35f + 0.45f * sz : 0.16f)); cv_px(x, y + 1, sh(R_IRON, sz > 0 ? 0.22f : 0.1f));
        if (sz > 0) cv_px(x, y + 2, sh(R_IRON, 0.12f));
        if (a % 9 == 4) { cv_px(x, y - 1, sh(R_IRON, 0.55f + 0.3f * fmaxf(0.0f, sz))); cv_px(x, y - 2, sh(R_IRON, 0.45f)); cv_px(x - 1, y - 1, sh(R_IRON, 0.35f)); cv_px(x + 1, y - 1, sh(R_IRON, 0.35f)); }   /* a fleuron */
    }
    for (int k = 0; k < 4; k++) {                                   /* chains in front */
        float ang = PI_F * 0.25f + (float)k * PI_F * 0.5f, x = cx + rx * cosf(ang) * 0.95f, y = ry + 3.5f * sinf(ang);
        if (sinf(ang) >= 0.0f) for (int i = 0; i <= 18; i++) { float t = (float)i / 18.0f; cv_px((int)(cx + (x - cx) * t), (int)(9.0f + (y - 9.0f) * t), sh(R_IRON, (i & 1) ? 0.62f : 0.3f)); }
    }
    for (int i = 9; i < 28; i++) cv_px((int)cx, i, sh(R_IRON, (i & 1) ? 0.55f : 0.25f));   /* the skull's chain */
    prop_skull((int)cx - 5, 27, 0, 0, 0, 0);
    for (int k = 0; k < 9; k++) {                                   /* candles in front */
        float ang = TAU_F * (float)k / 9.0f + 0.2f, sz = sinf(ang);
        if (sz < -0.1f) continue;
        wax_candle(cx + rx * cosf(ang), ry + 3.5f * sz - 1.0f, 5.0f + 3.0f * h01(k, 1, 713), 1.3f, 1, frame, k, 0);
    }
    for (int k = 0; k < 10; k++) {                                  /* wax hanging from its underside */
        float ang = PI_F * (0.08f + 0.84f * h01(k, 1, 715)), x = cx + rx * cosf(ang);
        int y = (int)(ry + 3.5f * sinf(ang)) + 2, l = 1 + (int)(h01(k, 2, 715) * 5.0f);
        for (int j = 1; j <= l; j++) cv_px((int)x, y + j, sh(R_BONE, 0.85f - 0.07f * (float)j));
    }
    cv_store(id);
}

/* the fire pit: a ring of dressed stones gone black inside, charred logs crossed in the ashes with their cracks glowing, a
 * bone among them, embers, flames with white hearts, sparks, smoke. frame 0-2 */
static void spr_hearth(int id, int frame) {
    cv_begin(cv_sprite, TS, TS);
    float cx = 32;
    cv_flat(cx, 61.5f, 22, 2.2f, R_WARM + 1);
    for (int i = 0; i < 12; i++) {                                  /* the stones behind */
        float ang = PI_F + PI_F * (float)i / 11.0f, x = cx - 17.5f * cosf(ang), y = 55.0f + 4.5f * sinf(ang);
        cv_ellipse(x, y, 3.8f, 2.6f, R_STONE, 0.1f, 0.45f);
        cv_px((int)x, (int)y + 1, sh(R_IRON, 0.1f));                /* soot */
    }
    cv_ellipse(cx, 56, 15.0f, 3.6f, R_WARM, 0.05f, 0.25f);         /* the ashes */
    for (int i = 0; i < 40; i++) cv_px((int)(cx + (h01(i, 1, 717) - 0.5f) * 26.0f), (int)(55.0f + (h01(i, 2, 717) - 0.5f) * 5.0f), sh(R_IRON, 0.25f + 0.3f * h01(i, 3, 717)));
    for (int l = 0; l < 2; l++) {                                   /* charred logs, crossed */
        float x0 = l ? 21.0f : 41.0f, x1 = l ? 43.0f : 22.0f;
        cv_capsule(x0, 57.0f - (float)l, x1, 53.0f + (float)l, 2.0f, R_WOOD, 0.02f, 0.18f);
        for (int k = 0; k < 6; k++) { float t = 0.1f + 0.8f * h01(k, l, 719); cv_px((int)(x0 + (x1 - x0) * t), (int)(57.0f - (float)l + (53.0f - 57.0f) * t) + (k & 1) - 1, (uint8_t)(R_FLAME + 4 + ((k + frame) % 4))); }   /* cracks glowing */
    }
    cv_capsule(25, 58, 33, 59, 0.9f, R_BONE, 0.08f, 0.4f); cv_ellipse(25, 57.5f, 1.2f, 1.2f, R_BONE, 0.05f, 0.35f);   /* a charred bone in the ashes */
    for (int i = 0; i < 4; i++) {                                   /* the flames */
        float x = cx - 8.0f + (float)i * 5.5f + (h01(i, frame, 721) - 0.5f) * 3.0f, h = 14.0f + 12.0f * h01(i, frame + 5, 721);
        prop_flame(x, 54, h, 3.2f, (h01(i, frame + 9, 721) - 0.5f) * 5.0f);
    }
    prop_flame(cx + (float)(frame - 1) * 1.2f, 55, 28, 5.5f, (float)(frame - 1) * 2.5f);
    for (int i = 0; i < 10; i++) cv_px((int)(cx + (h01(i, frame, 723) - 0.5f) * 22.0f), (int)(54.0f + h01(i, frame + 1, 723) * 3.0f), (uint8_t)(R_FLAME + 7 + (i & 3)));   /* embers */
    for (int i = 0; i < 9; i++) {                                   /* the stones in front, dressed blocks, a dark joint between */
        float ang = PI_F * ((float)i + 0.5f) / 9.0f, x = cx - 17.5f * cosf(ang), y = 56.5f + 4.5f * sinf(ang), hw = 3.2f, hh = 2.6f;
        for (int pass = 0; pass < 2; pass++)
            for (int yy = (int)(y - hh) - 1; yy <= (int)(y + hh) + 1; yy++)
                for (int xx = (int)(x - hw) - 1; xx <= (int)(x + hw) + 1; xx++) {
                    float nx = ((float)xx + 0.5f - x) / (hw + (pass ? 0.0f : 0.9f)), ny = ((float)yy + 0.5f - y) / (hh + (pass ? 0.0f : 0.9f));
                    if (fabsf(nx) > 1.0f || fabsf(ny) > 1.0f || (fabsf(nx) > 0.75f && fabsf(ny) > 0.6f)) continue;   /* chamfered corners */
                    if (!pass) { cv_px(xx, yy, sh(R_WARM, 0.03f)); continue; }
                    float v = ny < -0.45f ? 0.62f : (nx < -0.6f ? 0.48f : 0.32f);   /* a lit top, a lit left face, the front in shade */
                    v += 0.08f * (h01(xx, yy, 725 + (uint32_t)i) - 0.5f) - (ny < -0.45f && nx > 0.2f ? 0.18f : 0.0f);   /* soot over the fire's side */
                    if (h01(i, 9, 727) < 0.35f && nx > 0.4f && ny > 0.2f) continue;   /* a corner broken off */
                    cv_px(xx, yy, sh(R_STONE, v));
                }
    }
    for (int i = 0; i < 7; i++) cv_px((int)(cx + (h01(i, frame, 729) - 0.5f) * 26.0f), (int)(8.0f + 22.0f * h01(i, frame + 2, 729)), (uint8_t)(R_FLAME + 9 + (i & 3)));   /* sparks */
    cv_outline(0);
    smoke_up(cx + (float)(frame - 1), 26.0f, 22.0f, 731u + (uint32_t)frame);
    cv_store(id);
}

/* the brazier: a deep iron bowl pierced with quatrefoils that glow from within, a skull in relief on its front, spikes round
 * its rim, three legs curving out to claw feet; coals heaped in it, flames, sparks. frame 0-2 */
static void spr_gbrazier(int id, int frame) {
    cv_begin(cv_sprite, TS, TS);
    cv_flat(32, 62, 14, 1.7f, R_WARM + 1);
    cv_capsule(32, 46, 32, 60, 1.1f, R_IRON, 0.08f, 0.3f);          /* the leg behind */
    for (int i = 0; i < 3; i++) {
        float x = 32.0f + (float)(i - 1) * 5.0f + (h01(i, frame, 733) - 0.5f) * 3.0f, h = 12.0f + 10.0f * h01(i, frame + 4, 733);
        prop_flame(x, 34, h, 3.4f, (h01(i, frame + 8, 733) - 0.5f) * 4.0f);
    }
    prop_flame(32, 35, 21.0f + 2.0f * (float)frame, 5.0f, (float)(frame - 1) * 2.0f);
    for (int i = 0; i < 14; i++) cv_px((int)(20.0f + 24.0f * h01(i, 1, 735)), 33 + (int)(h01(i, 2, 735) * 2.0f), (uint8_t)(R_FLAME + 5 + ((i + frame) % 6)));   /* coals heaped over its lip */
    for (int y = 34; y <= 46; y++)                                  /* the bowl */
        for (int x = 16; x <= 48; x++) {
            float hw = 15.5f - (float)(y - 34) * 0.75f, nx = ((float)x + 0.5f - 32.0f) / hw;
            if (fabsf(nx) > 1.0f) continue;
            float v = 0.18f + 0.45f * lit(nx, 0.25f, sqrtf(1.0f - nx * nx)) + (y == 34 ? 0.3f : 0.0f) - (y == 46 ? 0.1f : 0.0f);
            cv_px(x, y, sh(R_IRON, v));
        }
    static const int QX[4] = { 20, 25, 39, 44 };
    for (int q = 0; q < 4; q++) {                                   /* quatrefoils pierced through it, glowing */
        int qx = QX[q], qy = 40;
        for (int j = -2; j <= 2; j++) for (int i = -2; i <= 2; i++) if (abs(i) + abs(j) <= 3) cv_px(qx + i, qy + j, sh(R_IRON, 0.06f));   /* its pierced rim */
        static const int QF[5][2] = { { 0, -1 }, { -1, 0 }, { 1, 0 }, { 0, 1 }, { 0, 0 } };
        for (int k = 0; k < 5; k++) cv_px(qx + QF[k][0], qy + QF[k][1], (uint8_t)(R_FLAME + 6 + (k == 4) * 4 + ((frame + q) & 1)));
    }
    grid_stamp(SKULL_SMALL, 9, 10, 28, 36, R_IRON, R_IRON, 0, 0);   /* a skull in relief on its front */
    for (int y = 36; y < 46; y++) for (int x = 28; x < 37; x++) { uint8_t c = cv_sprite[y * TS + x]; if (c > R_IRON && c < R_IRON + 16) cv_px(x, y, (uint8_t)(c + 1 > R_IRON + 15 ? R_IRON + 15 : c + 1)); }
    for (int k = 0; k < 7; k++) { int x = 18 + k * 5; cv_px(x, 33, sh(R_IRON, 0.7f)); cv_px(x, 32, sh(R_IRON, 0.55f)); cv_px(x, 31, sh(R_IRON, 0.4f)); }   /* spikes round its rim */
    for (int s = -1; s <= 1; s += 2) {                              /* legs, curving out to claw feet */
        float x0 = 32.0f + (float)s * 7.0f, xm = 32.0f + (float)s * 10.0f, x1 = 32.0f + (float)s * 14.0f;
        cv_capsule(x0, 45, xm, 53, 1.3f, R_IRON, 0.15f, 0.65f); cv_capsule(xm, 53, x1, 61, 1.1f, R_IRON, 0.15f, 0.6f);
        for (int t = 0; t < 3; t++) cv_px((int)x1 + s * t, 62 - (t == 2), sh(R_IRON, 0.5f));
    }
    for (int i = 0; i < 5; i++) cv_px((int)(32.0f + (h01(i, frame, 737) - 0.5f) * 20.0f), (int)(4.0f + 10.0f * h01(i, frame + 2, 737)), (uint8_t)(R_FLAME + 10));   /* sparks */
    cv_outline(0);
    cv_store(id);
}

/* the lantern (run II's own, and only ever hung over its water): an iron lantern on its chain, a gothic roof with a finial,
 * horn panes lit amber from within (one cracked), the candle a brighter flame behind them, rust on its iron and a drip from
 * its foot. frame 0-1 */
static void spr_hornlantern(int id, int frame) {
    cv_begin(cv_sprite, TS, TS);
    chain_down(32, 0, 12);
    cv_ellipse(32, 13, 1.6f, 1.6f, R_IRON, 0.2f, 0.75f);            /* its ring */
    for (int y = 15; y <= 21; y++) {                                /* the roof, a spire */
        float hw = 1.0f + (float)(y - 15) * 1.15f;
        for (int x = (int)(32 - hw); x <= (int)(32 + hw); x++) { float nx = ((float)x + 0.5f - 32.0f) / (hw + 0.01f); cv_px(x, y, sh(R_IRON, 0.25f + 0.45f * lit(nx, -0.4f, sqrtf(fmaxf(0.0f, 1.0f - nx * nx))))); }
    }
    for (int x = 23; x <= 41; x++) cv_px(x, 22, sh(R_IRON, 0.6f - 0.02f * (float)abs(x - 30)));   /* the eave */
    for (int y = 23; y < 36; y++)                                   /* the cage: two panes of horn in front, the sides dimmer */
        for (int x = 24; x <= 40; x++) {
            float nx = ((float)x + 0.5f - 32.0f) / 8.0f;
            int bar = x == 24 || x == 40 || x == 32 || x == 27 || x == 37 || y == 23 || y == 35;
            if (bar) { cv_px(x, y, sh(R_IRON, x == 24 || x == 27 ? 0.55f : 0.3f)); continue; }
            float side = (x < 27 || x > 37) ? 0.55f : 1.0f;
            float glow = (1.0f - fabsf(nx) * 0.5f - fabsf((float)y - 29.0f) / 13.0f) * side + (frame ? 0.04f : -0.04f) - 0.12f * fbm((float)x / 2.0f, (float)y / 2.0f, 16, 739);
            cv_px(x, y, (uint8_t)(R_FLAME + clampi((int)(glow * 12.0f), 2, 12)));
        }
    for (int k = 0; k < 6; k++) cv_px(35 - k / 2, 25 + k, (uint8_t)(R_FLAME + 2));   /* a pane cracked */
    prop_flame(32.5f, 32, 6.0f + (float)frame, 1.6f, frame ? 0.5f : -0.5f);
    for (int x = 23; x <= 41; x++) { cv_px(x, 36, sh(R_IRON, 0.55f - 0.015f * (float)abs(x - 30))); cv_px(x, 37, sh(R_IRON, 0.25f)); }   /* its foot */
    cv_px(32, 38, sh(R_IRON, 0.4f));
    for (int k = 0; k < 8; k++) cv_px(24 + (int)(h01(k, 1, 741) * 17.0f), 22 + (int)(h01(k, 2, 741) * 16.0f), sh(R_BRICK, 0.25f));   /* rust */
    for (int x = 23; x <= 41; x += 6) cv_px(x, 22, sh(R_BRICK, 0.3f));
    cv_outline(0);
    cv_px(31, 40, sh(R_GHOST, 0.6f)); cv_px(31, 43, sh(R_GHOST, 0.75f));   /* a drip from its foot */
    cv_store(id);
}

/* P3: the graves. A carving is a mask over the stone: 1 raised (lit, its shadow below and to the right), 2 cut in (dark, its
 * lower lip catching the light) */
static uint8_t g_carve[TS * TS];
static void cmark(int x, int y, int k) { if ((unsigned)x < (unsigned)TS && (unsigned)y < (unsigned)TS) g_carve[y * TS + x] = (uint8_t)k; }
static void carve_apply(void) {
    for (int y = 0; y < TS; y++)
        for (int x = 0; x < TS; x++) {
            uint8_t *p = &cv_sprite[y * TS + x];
            if (*p <= R_STONE || *p >= R_STONE + 16) continue;
            int k = g_carve[y * TS + x], up = y > 0 ? g_carve[(y - 1) * TS + x] : 0, ul = x > 0 && y > 0 ? g_carve[(y - 1) * TS + x - 1] : 0, v = *p - R_STONE;
            if (k == 1) v += 3; else if (k == 2) v -= 5; else if (ul == 1) v -= 3; else if (up == 2) v += 2;
            *p = (uint8_t)(R_STONE + clampi(v, 1, 15));
        }
    memset(g_carve, 0, sizeof g_carve);
}
/* lines of worn lettering cut between x0 and x1, from y0, three rows apart */
static void carve_lines(int x0, int x1, int y0, int n, uint32_t seed) {
    for (int i = 0; i < n; i++) {
        int y = y0 + i * 3, a = x0 + (i == n - 1 ? 2 : 0), b = x1 - (i == n - 1 ? 3 : (int)(h01(i, 1, seed) * 2.0f));
        for (int x = a; x <= b; x++) {
            float r = h01(x, i, seed);
            if (r < 0.28f) continue;                                /* between the letters */
            if (r > 0.92f) continue;                                /* worn away */
            cmark(x, y, 2);
            if (r > 0.72f) cmark(x, y - 1, 2);                      /* an upstroke */
        }
    }
}
/* a winged skull in relief, the Order's memento mori, its middle at (cx, cy) */
static void carve_wskull(int cx, int cy) {
    static const char *const SK[6] = { ".###.", "#####", "#o#o#", "##.##", ".###.", ".#.#." };
    for (int y = 0; y < 6; y++) for (int x = 0; x < 5; x++) { char c = SK[y][x]; if (c == '#') cmark(cx - 2 + x, cy - 3 + y, 1); else if (c == 'o') cmark(cx - 2 + x, cy - 3 + y, 2); }
    for (int s = -1; s <= 1; s += 2)
        for (int f = 0; f < 4; f++) {                               /* feathers, fanned out and up */
            float a = (s < 0 ? PI_F : 0.0f) - (float)s * (0.15f + 0.22f * (float)f), l = 6.0f - (float)f * 0.8f;
            for (int k = 0; k <= (int)l; k++) cmark(cx + s * 3 + (int)(cosf(a) * (float)k), cy - 1 + f - (int)(sinf(fabsf(a - (s < 0 ? PI_F : 0.0f))) * (float)k * 0.9f), 1);
        }
}
static void carve_hourglass(int cx, int cy) {
    for (int k = 0; k < 4; k++) for (int x = -3 + k; x <= 3 - k; x++) { cmark(cx + x, cy - 4 + k, 2); cmark(cx + x, cy + 4 - k, 2); }
    cmark(cx, cy, 2);
    for (int x = -4; x <= 4; x++) { cmark(cx + x, cy - 5, 1); cmark(cx + x, cy + 5, 1); }
}
static void carve_wreath(int cx, int cy, float r) {
    for (int a = 0; a < 28; a++) { float an = TAU_F * (float)a / 28.0f; cmark(cx + (int)roundf(cosf(an) * r), cy + (int)roundf(sinf(an) * r), 1); if (a % 3 == 0) cmark(cx + (int)roundf(cosf(an) * (r + 1.2f)), cy + (int)roundf(sinf(an) * (r + 1.2f)), 1); }
    cmark(cx - 1, cy + (int)r + 1, 1); cmark(cx + 1, cy + (int)r + 2, 1);   /* its ribbon */
}
/* a slab of the grave's stone, cx its middle at the foot, from top to bottom, hw either side: shape 0 a round top, 1 a lancet,
 * 2 shouldered; lean tips it (a pixel every so many rows); lit from the left, a bevel round its edge catching the light on
 * the left and top, weathered, rain-streaked, the soil splashed up its foot */
static int slab_in(float dx, float ady, float hw, int shape) {
    if (fabsf(dx) > hw) return 0;
    if (shape == 0 && ady < hw) return dx * dx + (hw - ady) * (hw - ady) <= hw * hw;
    if (shape == 1) { float sp = hw * 1.7f; if (ady < sp) { float h = sp - ady, c = dx < 0.0f ? hw : -hw; return (dx - c) * (dx - c) + h * h <= 4.0f * hw * hw; } }
    if (shape == 2 && ady < 3.0f) return fabsf(dx) <= hw - 2.5f;
    return 1;
}
static void slab(float cx, int top, int bottom, float hw, int shape, float lean, uint32_t seed) {
    for (int y = top; y <= bottom; y++) {
        float ox = (float)(bottom - y) * lean;
        for (int x = (int)(cx - hw - 3.0f + ox); x <= (int)(cx + hw + 3.0f + ox); x++) {
            float dx = (float)x + 0.5f - cx - ox, ady = (float)(y - top) + 0.5f;
            if (!slab_in(dx, ady, hw, shape)) continue;
            float v = 0.4f - 0.08f * dx / hw + (fbm((float)x / 3.0f, (float)y / 3.0f, 16, seed) - 0.5f) * 0.14f;
            if (!slab_in(dx - 1.2f, ady, hw, shape)) v += 0.17f;   /* its left bevel, lit */
            else if (!slab_in(dx + 1.2f, ady, hw, shape)) v -= 0.14f;
            if (!slab_in(dx, ady - 1.2f, hw, shape)) v += 0.14f;   /* its top */
            if (h01(x, y, seed + 1) > 0.965f) v -= 0.15f;           /* pitted */
            if (h01(x, 0, seed + 2) < 0.12f && h01(x, y / 3, seed + 3) < 0.7f) v -= 0.06f;   /* rain streaks */
            if (y > bottom - 2) v -= 0.12f;                         /* soil splashed up its foot */
            cv_px(x, y, sh(R_STONE, v));
        }
    }
}
/* lichen in pale crusts and moss low down, over the stone (after the carvings) */
static void weather(int top, int bottom, float amount, uint32_t seed) {
    for (int y = top; y <= bottom; y++)
        for (int x = 0; x < TS; x++) {
            uint8_t *p = &cv_sprite[y * TS + x];
            if (*p <= R_STONE || *p >= R_STONE + 16) continue;
            if (fbm((float)x / 2.5f, (float)y / 2.5f, 32, seed) > 0.78f - 0.1f * amount) *p = sh(R_BONE, 0.42f + 0.18f * h01(x, y, seed + 1));
            else if (y > bottom - 7 && fbm((float)x / 3.0f, (float)y / 2.0f, 32, seed + 5) > 0.64f - 0.1f * amount) *p = sh(R_MOSS, 0.18f + 0.3f * h01(x, y, seed + 2));
        }
}
/* the grave's earth: a mound of clotted soil with pebbles and moss in it, w either side of cx */
static void grave_earth(float cx, float w, float h, uint32_t seed) {
    for (int y = (int)(61.0f - h); y <= 63; y++)
        for (int x = (int)(cx - w); x <= (int)(cx + w); x++) {
            float nx = ((float)x + 0.5f - cx) / w, top = 61.0f - h * (1.0f - nx * nx) + 1.2f * (h01(x / 2, 1, seed) - 0.5f);
            if ((float)y < top || fabsf(nx) > 1.0f) continue;
            float v = 0.1f + 0.22f * ((float)y < top + 1.5f ? 1.0f : 0.4f) - 0.1f * nx + 0.1f * (h01(x, y, seed) - 0.5f);
            uint8_t c = sh(R_WOOD, v);
            float r = h01(x, y, seed + 1);
            if (r > 0.93f) c = sh(R_STONE, 0.45f + 0.2f * h01(x, y, seed + 2));   /* a pebble */
            else if (r < 0.08f && (float)y < top + 2.0f) c = sh(R_MOSS, 0.25f + 0.2f * h01(x, y, seed + 3));
            cv_px(x, y, c);
        }
}
/* The graves of runs I and II (and the Deacon's chapel): four kinds, two looks each. kind 0 a headstone (a lancet slab with a
 * winged skull and a dried wreath at its foot; a round-topped one with an hourglass, a corner broken, a candle stub burnt
 * out), 1 a cross (ringed, on two steps, furred with lichen; a trefoil cross on a plinth, one arm broken off and lying at
 * its foot), 2 an obelisk (on a stepped plinth with a wreath; a draped urn on a pedestal with an inscription panel), 3
 * broken (the stump, its top fallen beside it; a ledger stone flat in heaved earth, cracked across) */
static void spr_grave(int id, int kind, int look) {
    cv_begin(cv_sprite, TS, TS);
    memset(g_carve, 0, sizeof g_carve);
    uint32_t sd = 751u + (uint32_t)(kind * 2 + look) * 13u;
    if (kind == 0) {
        grave_earth(32, 17, 3.5f, sd);
        if (!look) {
            slab(32, 19, 59, 10.0f, 1, 0.04f, sd);
            carve_wskull(33, 32); carve_lines(25, 39, 41, 4, sd);
            carve_apply();
            cv_line(38, 22, 34, 31, sh(R_STONE, 0.06f)); cv_line(34, 31, 35, 36, sh(R_STONE, 0.08f));   /* a crack down through it */
            weather(19, 59, 0.6f, sd + 7);
            for (int a = 0; a < 22; a++) {                          /* a wreath of dried flowers leant at its foot */
                float an = TAU_F * (float)a / 22.0f;
                int x = 32 + (int)(cosf(an) * 5.5f), y = 55 + (int)(sinf(an) * 3.6f);
                cv_px(x, y, (a & 1) ? sh(R_WOOD, 0.35f) : sh(R_BONE, 0.5f + 0.2f * (float)(a % 3 == 0)));
                if (a % 4 == 0) cv_px(x, y - 1, sh(R_BRICK, 0.35f));   /* a petal's old red */
            }
        } else {
            slab(32, 23, 59, 11.0f, 0, -0.03f, sd);
            for (int y = 23; y < 30; y++) for (int x = 38; x < 46; x++) if (y - 23 < (x - 38) - 1) cv_px(x, y, C_CLEAR);   /* a corner broken off */
            carve_hourglass(32, 34); carve_lines(24, 39, 45, 3, sd);
            carve_apply();
            weather(23, 59, 0.4f, sd + 7);
            for (int y = 54; y <= 59; y++) { cv_px(23, y, sh(R_BONE, 0.7f)); cv_px(24, y, sh(R_BONE, 0.45f)); }   /* a candle stub, out */
            cv_px(23, 53, sh(R_IRON, 0.15f)); cv_px(25, 59, sh(R_BONE, 0.75f)); cv_px(22, 60, sh(R_BONE, 0.6f));
        }
    } else if (kind == 1) {
        grave_earth(32, 15, 3.0f, sd);
        if (!look) {
            slab(32, 54, 60, 9.0f, 2, 0.0f, sd); slab(32, 49, 54, 6.0f, 2, 0.0f, sd + 1);
            for (int a = 0; a < 64; a++) {                          /* the ring, behind the cross, open between its arms */
                float an = TAU_F * (float)a / 64.0f;
                for (float rr = 7.2f; rr <= 9.0f; rr += 0.5f) {
                    int x = 32 + (int)roundf(cosf(an) * rr), y = 29 + (int)roundf(sinf(an) * rr);
                    cv_px(x, y, sh(R_STONE, 0.3f + 0.26f * (-sinf(an) * 0.6f - cosf(an) * 0.4f) + (rr > 8.4f ? -0.1f : 0.05f)));
                }
            }
            slab(32, 15, 49, 2.6f, 2, 0.0f, sd + 2);                /* the shaft */
            slab(32, 27, 31, 12.0f, 2, 0.0f, sd + 3);               /* its arms */
            for (int y = 34; y < 48; y += 3) { cmark(31, y, 2); cmark(33, y + 1, 2); cmark(32, y + 2, 1); }   /* knotwork down the shaft */
            carve_lines(26, 38, 56, 1, sd);
            carve_apply();
            weather(16, 60, 1.0f, sd + 7);
        } else {
            slab(32, 50, 59, 7.5f, 2, 0.0f, sd);                    /* the plinth */
            slab(32, 17, 50, 2.5f, 2, 0.0f, sd + 1);                /* the shaft */
            slab(29, 27, 31, 8.5f, 2, 0.0f, sd + 2);                /* the arms, the right one snapped short */
            for (int y = 27; y <= 31; y++) for (int x = 36; x <= 38; x++) if (x > 36 + (y & 1) - (y == 29)) cv_px(x, y, C_CLEAR);
            for (int y = 27; y <= 31; y++) cv_px(36 + (y & 1) - (y == 29), y, sh(R_STONE, 0.2f));   /* its raw end */
            static const float TOPL[3][2] = { { 30.0f, 17.2f }, { 32.0f, 15.0f }, { 34.0f, 17.2f } }, LEFT[3][2] = { { 21.6f, 27.2f }, { 19.4f, 29.0f }, { 21.6f, 30.8f } };
            for (int k = 0; k < 3; k++) {                           /* trefoil ends at the left and the top */
                cv_ellipse(TOPL[k][0], TOPL[k][1], 1.7f, 1.7f, R_STONE, 0.22f, 0.72f);
                cv_ellipse(LEFT[k][0], LEFT[k][1], 1.7f, 1.7f, R_STONE, 0.22f, 0.72f);
            }
            static const float TRE[3][2] = { { 0.0f, -2.2f }, { -2.2f, 0.0f }, { 0.0f, 2.2f } };
            slab(48, 57, 60, 4.5f, 2, 0.0f, sd + 3);                /* the broken end, lying at its foot */
            for (int k = 0; k < 3; k++) cv_ellipse(53.5f, 58.5f + TRE[k][1] * 0.8f, 1.5f, 1.4f, R_STONE, 0.22f, 0.62f);
            carve_lines(27, 37, 53, 2, sd);
            carve_apply();
            weather(17, 60, 0.5f, sd + 7);
        }
    } else if (kind == 2) {
        grave_earth(32, 16, 3.0f, sd);
        if (!look) {
            slab(32, 54, 60, 11.0f, 2, 0.0f, sd); slab(32, 48, 54, 8.5f, 2, 0.0f, sd + 1);
            for (int y = 8; y <= 48; y++) {                         /* the shaft, tapering, its pyramidion; two faces, one lit */
                float hw = y < 14 ? (float)(y - 8) * 0.8f + 0.5f : 5.4f - (float)(y - 14) * 0.035f;
                for (int x = (int)(32.0f - hw) - 1; x <= (int)(32.0f + hw) + 1; x++) {
                    float dx = (float)x + 0.5f - 32.0f;
                    if (fabsf(dx) > hw) continue;
                    float v = (dx < 0.0f ? 0.5f : 0.26f) + (fbm((float)x / 3.0f, (float)y / 3.0f, 16, sd + 2) - 0.5f) * 0.12f + (y < 14 ? 0.1f : 0.0f);
                    if (fabsf(dx) < 0.6f) v -= 0.08f;               /* the arris */
                    cv_px(x, y, sh(R_STONE, v));
                }
            }
            carve_wreath(32, 27, 3.6f); carve_lines(25, 39, 51, 1, sd);
            carve_apply();
            weather(8, 60, 0.5f, sd + 7);
        } else {
            slab(32, 54, 60, 10.0f, 2, 0.0f, sd); slab(32, 40, 54, 7.5f, 2, 0.0f, sd + 1); slab(32, 36, 40, 9.0f, 2, 0.0f, sd + 2);
            for (int x = 27; x <= 37; x++) { cmark(x, 43, 2); cmark(x, 52, 2); }   /* its panel */
            for (int y = 43; y <= 52; y++) { cmark(27, y, 2); cmark(37, y, 2); }
            carve_lines(29, 35, 46, 2, sd);
            for (int y = 16; y < 36; y++)                           /* the urn */
                for (int x = 22; x <= 42; x++) {
                    float t = (float)(y - 16) / 20.0f, hw = y < 19 ? 3.0f : (y < 22 ? 5.5f : 8.5f * sinf(PI_F * (0.15f + 0.85f * (float)(y - 22) / 14.0f)) + 1.5f);
                    if (y >= 33) hw = 3.5f + (float)(y - 33);
                    float dx = (float)x + 0.5f - 32.0f;
                    if (fabsf(dx) > hw) continue;
                    float nx = dx / hw, v = 0.2f + 0.55f * lit(nx, 0.0f, sqrtf(fmaxf(0.0f, 1.0f - nx * nx))) + (fbm((float)x / 3.0f, (float)y / 3.0f, 16, sd + 4) - 0.5f) * 0.1f;
                    (void)t;
                    cv_px(x, y, sh(R_STONE, v));
                }
            cv_ellipse(32, 14.5f, 2.2f, 1.6f, R_STONE, 0.3f, 0.75f);   /* its finial */
            for (int s = -1; s <= 1; s += 2)                        /* its handles, at the shoulder */
                for (int a = 0; a < 10; a++) { float an = PI_F * 0.5f + (float)s * (PI_F * 0.5f) * ((float)a / 9.0f) * -1.0f; cv_px(32 + s * 9 + (int)roundf(cosf(an) * 2.0f * (float)s), 25 + (int)roundf(sinf(an) * 3.0f), sh(R_STONE, s < 0 ? 0.6f : 0.3f)); }
            for (int y = 21; y < 34; y++) {                         /* a cloth draped over it, in folds down its left */
                int x0 = 23 + (y - 21) / 4, x1 = 30 - (y - 21) / 2;
                for (int x = x0; x <= x1; x++) {
                    if (cv_sprite[y * TS + x] == C_CLEAR) continue;
                    float fold = sinf((float)(x - x0) * 1.6f + (float)y * 0.2f);
                    cv_px(x, y, sh(R_STONE, 0.42f + 0.14f * fold + (y == 33 && (x & 1) ? -0.1f : 0.0f)));
                }
            }
            carve_apply();
            weather(14, 60, 0.6f, sd + 7);
        }
    } else {
        if (!look) {
            grave_earth(32, 20, 3.0f, sd);
            slab(26, 30, 59, 9.0f, 2, 0.0f, sd);
            for (int y = 30; y < 59; y++) for (int x = 15; x <= 37; x++) if ((float)y < 36.0f + (float)(x - 17) * 0.45f + 1.5f * (h01(x, 1, sd) - 0.5f)) cv_px(x, y, C_CLEAR);   /* snapped on a slant */
            carve_lines(19, 32, 47, 3, sd);
            for (int y = 51; y <= 59; y++)                          /* its top, fallen on its side */
                for (int x = 37; x <= 58; x++) {
                    float dx = (float)x + 0.5f - 47.0f, dy = (float)y + 0.5f - 59.0f, r = 10.0f;
                    if (dx * dx / (r * r) + dy * dy / 64.0f > 1.0f || dy > 0.5f) continue;
                    float v = 0.38f + 0.14f * (-dy / 8.0f) - 0.06f * dx / r + (fbm((float)x / 3.0f, (float)y / 3.0f, 16, sd + 3) - 0.5f) * 0.12f;
                    if (dx < -8.6f) v -= 0.12f;                     /* its broken face */
                    cv_px(x, y, sh(R_STONE, v));
                }
            carve_lines(41, 53, 54, 1, sd + 1);
            carve_apply();
            for (int k = 0; k < 6; k++) cv_ellipse(20.0f + (float)k * 5.0f + h01(k, 1, sd) * 3.0f, 60.5f, 1.2f, 0.9f, R_STONE, 0.25f, 0.55f);   /* chips of it */
            weather(30, 60, 0.6f, sd + 7);
        } else {
            grave_earth(32, 24, 4.5f, sd);                          /* heaved earth round it */
            for (int y = 50; y <= 60; y++) {                        /* a ledger stone, flat, its far edge narrower */
                float t = (float)(y - 50) / 10.0f, x0 = 19.0f - 6.0f * t, x1 = 45.0f + 6.0f * t;
                int sunk = 0;
                for (int x = (int)x0; x <= (int)x1; x++) {
                    float crack = 20.0f + 2.4f * (float)(y - 50);    /* cracked across; the left half sunk a little */
                    sunk = (float)x < crack;
                    int yy = y + sunk;
                    float v = 0.48f - 0.12f * t + (fbm((float)x / 3.0f, (float)y / 2.0f, 16, sd + 2) - 0.5f) * 0.12f;
                    if (fabsf((float)x - crack) < 0.8f) v = 0.06f;
                    cv_px(x, yy, sh(R_STONE, v));
                }
                (void)sunk;
            }
            for (int x = 13; x <= 51; x++) { cv_px(x, 61, sh(R_STONE, 0.18f)); }   /* its near edge */
            static const char *const SB[5] = { "..###..", ".#####.", ".#o#o#.", "#.###.#", ".#.#.#." };
            for (int y = 0; y < 5; y++) for (int x = 0; x < 7; x++) { char c = SB[y][x]; if (c == '#') cmark(34 + x, 51 + y, 1); else if (c == 'o') cmark(34 + x, 51 + y, 2); }
            for (int k = 0; k < 9; k++) { cmark(31 + k, 52 + k / 2, 1); cmark(43 - k, 52 + k / 2, 1); }   /* crossbones */
            carve_lines(22, 30, 53, 2, sd);
            carve_apply();
            weather(49, 61, 0.4f, sd + 7);
        }
    }
    cv_outline(0);
    cv_store(id);
}
/* a riser's grave: the earth heaved up from below and cracking, a marker of two sticks bound into a cross leaning at its head,
 * a rag on it; open: the earth torn back both sides of a black hole, the coffin's lid split and its boards up, roots hanging in */
static void spr_riser_grave(int id, int open) {
    cv_begin(cv_sprite, TS, TS);
    if (!open) {
        grave_earth(30, 20, 7.0f, 761);
        for (int i = 0; i < 4; i++) {                               /* cracks across it, the earth lifting */
            float x = 18.0f + (float)i * 7.0f;
            cv_line(x, 59, x + 3.0f, 54.0f + h01(i, 1, 763) * 2.0f, sh(R_WOOD, 0.02f));
            cv_px((int)x + 1, 56, sh(R_WOOD, 0.4f));
        }
        cv_capsule(46, 61, 49, 44, 1.0f, R_WOOD, 0.15f, 0.5f);      /* the marker, leaning */
        cv_capsule(43, 49, 53, 48, 0.9f, R_WOOD, 0.15f, 0.5f);
        for (int k = 0; k < 4; k++) cv_px(47 + (k & 1), 48 + k / 2, sh(R_WARM, 0.2f));   /* bound with cord */
        for (int y = 49; y < 55; y++) { cv_px(51, y, sh(R_WARM, 0.45f - 0.03f * (float)(y - 49))); if (y < 53) cv_px(52, y, sh(R_WARM, 0.35f)); }   /* a rag */
    } else {
        grave_earth(14, 11, 6.0f, 765); grave_earth(50, 11, 6.0f, 767);
        for (int y = 54; y <= 61; y++) {                            /* the hole */
            float t = (float)(y - 54) / 7.0f, x0 = 21.0f - 3.0f * t, x1 = 43.0f + 3.0f * t;
            for (int x = (int)x0; x <= (int)x1; x++) cv_px(x, y, y == 54 ? sh(R_WOOD, 0.12f) : 0);
        }
        for (int k = 0; k < 5; k++) cv_line(23.0f + (float)k * 4.5f, 55, 23.5f + (float)k * 4.5f, 57.0f + h01(k, 1, 769) * 3.0f, sh(R_WOOD, 0.25f));   /* roots hanging in */
        cv_capsule(24, 60, 31, 50, 1.6f, R_WOOD, 0.12f, 0.5f);      /* the lid, split, its boards up */
        cv_capsule(33, 50, 41, 59, 1.6f, R_WOOD, 0.12f, 0.45f);
        cv_line(31, 50, 32, 46, sh(R_WOOD, 0.45f)); cv_line(33, 50, 33, 47, sh(R_WOOD, 0.4f));   /* splinters */
        cv_px(27, 55, sh(R_IRON, 0.6f)); cv_px(37, 54, sh(R_IRON, 0.6f));   /* nails */
    }
    cv_outline(0);
    cv_store(id);
}

/* P4: what hangs. A link of forged iron at (x, y), long along (ux, uy): face-on a ring round its dark eye, edge-on a bar; lit
 * from the upper left; dim darkens it (the far side of a thing) */
static void iron_link(float x, float y, float ux, float uy, int face, float dim) {
    float lo = fmaxf(0.03f, 0.16f - dim), hi = 0.86f - dim;
    if (!face) { cv_capsule(x - ux * 2.3f, y - uy * 2.3f, x + ux * 2.3f, y + uy * 2.3f, 0.8f, R_IRON, lo, hi); return; }
    for (int yy = (int)floorf(y - 3.5f); yy <= (int)ceilf(y + 3.5f); yy++)
        for (int xx = (int)floorf(x - 3.5f); xx <= (int)ceilf(x + 3.5f); xx++) {
            float ox = (float)xx + 0.5f - x, oy = (float)yy + 0.5f - y, a = ox * ux + oy * uy, b = -ox * uy + oy * ux;
            float e = sqrtf((a / 2.9f) * (a / 2.9f) + (b / 2.0f) * (b / 2.0f));
            if (e < 0.45f || e > 1.0f) continue;
            float s = (e - 0.45f) / 0.55f * 2.0f - 1.0f, r = sqrtf(ox * ox + oy * oy) + 1e-3f;
            cv_px(xx, yy, sh(R_IRON, lo + (hi - lo) * lit(ox / r * s, oy / r * s, sqrtf(fmaxf(0.0f, 1.0f - s * s)))));
        }
}
/* an iron ring about (cx, cy), rx by ry, th thick, from angle a0 to a1 (y down: pi/2 is its foot), a tube lit from the upper left */
static void iron_ring(float cx, float cy, float rx, float ry, float th, float a0, float a1, float dim) {
    float lo = fmaxf(0.03f, 0.15f - dim), hi = 0.88f - dim, R = fmaxf(rx, ry) + th;
    for (int yy = (int)floorf(cy - R); yy <= (int)ceilf(cy + R); yy++)
        for (int xx = (int)floorf(cx - R); xx <= (int)ceilf(cx + R); xx++) {
            float ox = (float)xx + 0.5f - cx, oy = (float)yy + 0.5f - cy, ang = atan2f(oy, ox), a = ang;
            while (a < a0) a += TAU_F;
            while (a >= a0 + TAU_F) a -= TAU_F;
            if (a > a1) continue;
            float d = hypotf(ox, oy), mid = hypotf(cosf(ang) * rx, sinf(ang) * ry), s = (d - mid) / (th * 0.5f);
            if (fabsf(s) > 1.0f) continue;
            cv_px(xx, yy, sh(R_IRON, lo + (hi - lo) * lit(ox / (d + 1e-3f) * s, oy / (d + 1e-3f) * s, sqrtf(fmaxf(0.0f, 1.0f - s * s)))));
        }
}
/* a forged chain from (x0, y0) to (x1, y1), sagging by sag, a link every four pixels along it, face-on and edge-on by turns
 * (face: whether the first is face-on) */
static void forged_chain(float x0, float y0, float x1, float y1, float sag, int face, float dim) {
    float px = x0, py = y0, run = 2.0f;
    int n = 0;
    for (int i = 1; i <= 240; i++) {
        float t = (float)i / 240.0f, x = x0 + (x1 - x0) * t, y = y0 + (y1 - y0) * t + sag * 4.0f * t * (1.0f - t);
        float dx = x - px, dy = y - py, l = hypotf(dx, dy) + 1e-5f;
        run += l;
        if (run >= 4.0f) { iron_link(x, y, dx / l, dy / l, ((n + face) & 1) == 0 ? 0 : 1, dim); n++; run -= 4.0f; }
        px = x; py = y;
    }
}
/* a ring bolt in the vault at (x, y): a plate, two rivets, an eye; what hangs from it starts at (x, y + 4) */
static void ring_bolt(float x, float y) {
    cv_ellipse(x, y + 0.6f, 3.6f, 1.6f, R_IRON, 0.08f, 0.6f);
    cv_px((int)(x - 2.6f), (int)y, sh(R_IRON, 0.92f)); cv_px((int)(x + 2.0f), (int)y, sh(R_IRON, 0.55f));
    iron_ring(x, y + 2.6f, 1.5f, 1.4f, 1.2f, 0.0f, TAU_F, 0.05f);
}
/* a manacle hung by its swivel at (x, y): a cuff of flat iron on its pin; open, its right half swung out and down on the
 * hinge; shut, the lock's lug at its foot */
static void manacle(float x, float y, int open, float dim) {
    float r = 3.8f, cy = y + 1.5f + r;
    cv_capsule(x, y - 0.5f, x, y + 1.2f, 0.9f, R_IRON, 0.2f, 0.8f - dim);
    iron_ring(x, cy, r, r * 0.92f, 2.0f, PI_F * 0.5f, PI_F * 1.5f, dim);
    if (!open) {
        iron_ring(x, cy, r, r * 0.92f, 2.0f, -PI_F * 0.5f, PI_F * 0.5f, dim + 0.06f);
        cv_ellipse(x + 0.4f, cy + r + 1.0f, 1.8f, 1.4f, R_IRON, 0.08f, 0.62f - dim);
        cv_px((int)(x + 0.6f), (int)(cy + r + 1.4f), 0);           /* its keyhole */
    } else {
        float hx = x, hy = cy - r * 0.92f, th = -1.2f, c = cosf(th), s = sinf(th);
        float ccx = hx + c * (x - hx) - s * (cy - hy), ccy = hy + s * (x - hx) + c * (cy - hy);
        iron_ring(ccx, ccy, r, r * 0.92f, 2.0f, -PI_F * 0.5f + th, PI_F * 0.5f + th, dim + 0.04f);
    }
}
/* the bones of a hand hanging from a shut cuff, its wrist at (x, y): the knuckled fingers down and a little curled, the thumb
 * apart */
static void hand_bones(float x, float y, uint32_t seed) {
    cv_ellipse(x, y, 2.6f, 1.6f, R_BONE, 0.25f, 0.85f);
    for (int f = 0; f < 4; f++) {
        float bx = x - 2.6f + (float)f * 1.75f, by = y + 1.2f, a = 1.57f - ((float)f - 1.5f) * 0.2f + (h01(f, 1, seed) - 0.5f) * 0.12f;
        static const float L[3] = { 4.6f, 3.0f, 2.0f };
        for (int k = 0; k < 3; k++) {
            float l = L[k] + (f == 1 || f == 2 ? 0.6f : 0.0f), ex = bx + cosf(a) * l, ey = by + sinf(a) * l;
            cv_line(bx, by, ex, ey, sh(R_BONE, 0.74f - 0.07f * (float)f - 0.08f * (float)k));
            cv_px((int)bx, (int)by, sh(R_BONE, 0.9f - 0.05f * (float)f));   /* the knuckle */
            bx = ex; by = ey; a -= 0.26f;                          /* curling in */
        }
    }
    cv_line(x - 2.4f, y + 0.5f, x - 5.2f, y + 3.6f, sh(R_BONE, 0.72f)); cv_line(x - 5.2f, y + 3.6f, x - 5.8f, y + 6.4f, sh(R_BONE, 0.6f));
    cv_px((int)(x - 5.2f), (int)(y + 3.6f), sh(R_BONE, 0.9f));
}
/* the iron's age on a hanging thing: rust coming through in patches as the level's decay goes on (on cv_sprite, before the
 * outline) */
static void iron_age(uint32_t seed) {
    for (int y = 0; y < TS; y++)
        for (int x = 0; x < TS; x++) {
            uint8_t *c = &cv_sprite[y * TS + x];
            if (*c < R_IRON || *c >= R_IRON + 16) continue;
            if (fbm((float)x / 3.0f, (float)y / 3.0f, 32, seed) > 1.02f - g_decay * 0.42f && h01(x, y, seed + 1) < 0.7f)
                *c = sh(R_BRICK, 0.1f + 0.035f * (float)(*c - R_IRON));
        }
}

/* the chains on a hall's wall. look 0: two ring bolts, a long chain to a manacle hung open and a longer to one shut on the
 * bones of a hand, and a few links snapped short from a third; look 1: a chain slung between two bolts and from its belly
 * a short drop to a pair of leg irons, one shut and empty, one sprung, a rag caught on it */
static void spr_hangchains(int id, int look) {
    cv_begin(cv_sprite, TS, TS);
    if (look == 0) {
        ring_bolt(32, 0);
        forged_chain(32, 4, 32.5f, 13, 0.0f, 1, 0.14f);            /* snapped short, further back */
        iron_ring(32.5f, 15.0f, 1.4f, 2.4f, 1.1f, -PI_F * 0.35f, PI_F * 1.25f, 0.12f);   /* its last link sprung open */
        ring_bolt(22, 1);
        forged_chain(22, 5, 21.5f, 40, 0.0f, 0, 0.0f);
        manacle(21.5f, 41, 1, 0.0f);
        ring_bolt(42, 0);
        forged_chain(42, 4, 42.5f, 41, 0.0f, 1, 0.0f);
        manacle(42.5f, 42, 0, 0.0f);
        hand_bones(42.5f, 48.5f, 3);
    } else {
        ring_bolt(11, 1); ring_bolt(53, 0);
        forged_chain(11, 5, 53, 4, 13.0f, 0, 0.04f);               /* the swag */
        forged_chain(32, 20, 32.5f, 30, 0.0f, 1, 0.0f);
        cv_ellipse(32.5f, 33.0f, 3.2f, 2.6f, R_IRON, 0.1f, 0.7f);  /* a padlock through the last link */
        iron_ring(32.5f, 30.6f, 1.8f, 1.8f, 1.1f, PI_F, TAU_F, 0.0f);
        cv_px(33, 33, 0); cv_px(33, 34, 0);
        forged_chain(31, 35, 26, 44, 1.5f, 0, 0.0f);
        forged_chain(34, 35, 39, 41, 1.0f, 1, 0.0f);
        manacle(25.5f, 45, 0, 0.06f);
        manacle(39.5f, 42, 1, 0.0f);
        for (int y = 44; y < 59; y++) {                            /* the rag, snagged on the sprung cuff */
            float u = (float)(y - 44) / 15.0f, w = 2.6f - 1.2f * u, cx = 44.0f + sinf((float)y * 0.5f) * 0.8f;
            for (int x = (int)(cx - w - 1); x <= (int)(cx + w + 1); x++) {
                if (fabsf((float)x + 0.5f - cx) > w || (y > 52 && h01(x, y, 61) < 0.35f)) continue;
                cv_px(x, y, sh(R_WARM, 0.14f + 0.22f * lit(((float)x + 0.5f - cx) / (w + 0.01f), 0.0f, 0.8f) + 0.08f * h01(x, y / 2, 62)));
            }
        }
    }
    iron_age(91 + (uint32_t)look);
    cv_outline(0);
    cv_store(id);
}

/* a squared beam from x0 to x1, its top at y0 and its foot at y1: an adzed face with the grain run along it, the end grain at
 * its left, iron bands at bx[] round it */
static void timber_beam(int x0, int x1, int y0, int y1, const int *bx, int nb, uint32_t seed) {
    for (int y = y0; y <= y1; y++)
        for (int x = x0; x <= x1; x++) {
            float v = y == y0 ? 0.52f : 0.36f - 0.05f * (float)(y - y0);
            v += (fbm((float)x / 9.0f, (float)y * 1.4f, 16, seed) - 0.5f) * 0.3f;   /* the grain */
            if (fmodf((float)x + 3.0f * h01(x / 7, 1, seed), 7.0f) < 1.0f && y > y0) v -= 0.08f;   /* adze marks */
            if (x <= x0 + 1) v = 0.42f + 0.12f * h01(x, y, seed + 1) - 0.06f * (float)(abs(y - (y0 + y1) / 2) == 1);   /* the end grain */
            if (x >= x1 - 1) v -= 0.12f;
            if (y == y1) v = 0.12f;
            cv_px(x, y, sh(R_WOOD, v));
        }
    for (int i = 0; i < 3; i++) {                                   /* a split in it */
        int x = x0 + 6 + (int)(h01(i, 2, seed) * (float)(x1 - x0 - 12)), y = y0 + 2 + (i & 1);
        for (int k = 0; k < 4 + i * 2; k++) cv_px(x + k, y + (k > 3), sh(R_WOOD, 0.06f));
    }
    for (int b = 0; b < nb; b++)
        for (int y = y0; y <= y1 + 1; y++)
            for (int x = bx[b]; x < bx[b] + 3; x++) {
                float v = (y == y0 ? 0.6f : 0.42f) - 0.1f * (float)(x - bx[b]) - (y == y1 + 1 ? 0.2f : 0.0f);
                cv_px(x, y, (y == y0 + 2 && x == bx[b] + 1) ? sh(R_IRON, 0.9f) : sh(R_IRON, v));
            }
}
/* a butcher's hook hung from (x, y): its eye through the chain's last link, the shank, the bowl round to the left and the
 * point turned up, sharp */
static void butcher_hook(float x, float y, float dim) {
    iron_ring(x, y + 1.4f, 1.3f, 1.6f, 1.2f, 0.0f, TAU_F, dim);
    cv_capsule(x, y + 3.0f, x + 0.3f, y + 10.0f, 0.95f, R_IRON, 0.18f, 0.84f - dim);
    iron_ring(x - 2.8f, y + 10.0f, 3.1f, 3.3f, 1.8f, -0.1f, PI_F * 1.05f, dim);
    for (int k = 0; k < 5; k++) cv_px((int)(x - 5.9f - 0.15f * (float)k), (int)(y + 9.5f - (float)k), sh(R_IRON, (k == 4 ? 0.95f : 0.62f) - dim));
}
/* hooks from a beam: three chains, the right one swung aside, and on the middle hook a strip of something long dead, hide and
 * dried meat with a rib through it, old blood gone black down the hook and in drops below */
static void spr_hookbeam(void) {
    cv_begin(cv_sprite, TS, TS);
    static const int BX[2] = { 10, 50 };
    static const float X[3] = { 19, 33, 46 }, L[3] = { 22, 30, 15 };
    for (int i = 0; i < 3; i++) {
        float xe = X[i] + (i == 2 ? 3.0f : 0.0f);
        forged_chain(X[i], 6, xe, L[i], 0.0f, i & 1, i == 0 ? 0.06f : 0.0f);
        butcher_hook(xe, L[i] + 1.0f, i == 0 ? 0.06f : 0.0f);
    }
    timber_beam(3, 61, 0, 5, BX, 2, 401);
    float hx = 33, hy = 31;                                         /* the strip on the middle hook, the point through it */
    for (int y = (int)hy + 8; y < (int)hy + 29; y++) {
        float u = ((float)y - hy - 8.0f) / 21.0f, cx = hx - 5.0f + sinf((float)y * 0.31f) * 1.2f + u * 1.5f;
        float wl = 3.6f - 1.6f * u + 0.8f * sinf((float)y * 0.9f), wr = 3.0f - 2.0f * u * u + 0.6f * sinf((float)y * 1.3f + 1.0f);
        for (int x = (int)(cx - wl - 1); x <= (int)(cx + wr + 1); x++) {
            float o = (float)x + 0.5f - cx;
            if (o < -wl || o > wr) continue;
            if (u > 0.75f && h01(x, y, 405) < (u - 0.75f) * 2.4f) continue;   /* torn at the foot */
            float nx = o / (o < 0.0f ? wl : wr), l = lit(nx, 0.0f, sqrtf(fmaxf(0.0f, 1.0f - nx * nx)));
            int hide = fbm((float)x / 2.5f, (float)y / 3.0f, 16, 407) > 0.52f;
            cv_px(x, y, hide ? sh(R_SKIN, 0.08f + 0.22f * l) : sh(R_BLOOD, 0.06f + 0.2f * l + 0.06f * h01(x, y, 409)));
            if (fabsf(o - 0.4f) < 0.5f && u < 0.6f && h01(x, y / 3, 411) < 0.5f) cv_px(x, y, sh(R_SKIN, 0.42f));   /* a sinew */
        }
    }
    for (int k = 0; k < 8; k++) {                                   /* a rib through it, out to the right */
        float a = -1.1f + 0.2f * (float)k, x = hx - 6.0f + cosf(a) * 9.0f, y = hy + 22.0f + sinf(a) * 4.0f;
        if (k > 1) { cv_px((int)x, (int)y, sh(R_BONE, 0.82f - 0.05f * (float)k)); cv_px((int)x, (int)y + 1, sh(R_BONE, 0.3f)); }
    }
    butcher_hook(hx, hy + 1.0f, 0.0f);
    for (int k = 0; k < 5; k++) cv_px((int)hx + (k & 1), (int)hy + 5 + k, sh(R_BLOOD, 0.1f));   /* black down the shank */
    for (int k = 0; k < 3; k++) { int x = (int)(hx - 6.0f + h01(k, 1, 413) * 5.0f), y = (int)hy + 31 + k * 2; cv_px(x, y, sh(R_BLOOD, 0.14f)); cv_px(x, y + 1, sh(R_BLOOD, 0.08f)); }
    iron_age(97);
    cv_outline(0);
    cv_store(SP_HOOKS);
}

/* the gibbet: a cage of flat iron straps, riveted where they cross, on a chain from a ring bolt; its hoops above the eye are
 * seen from below and those below it from above. A hoop at y, rx across: the near half (front) or the far */
static float gib_tilt(float y) { return (y - 32.0f) / 16.0f * 2.4f; }
static void gib_hoop(float cx, float y, float rx, int front) {
    for (int a = 0; a < 120; a++) {
        float ang = TAU_F * (float)a / 120.0f, sz = sinf(ang), side = 0.5f - 0.5f * cosf(ang);
        if ((sz >= 0.0f) != front) continue;
        int x = (int)floorf(cx + rx * cosf(ang)), yy = (int)floorf(y + gib_tilt(y) * sz);
        float v = front ? 0.3f + 0.42f * side : 0.1f + 0.1f * (1.0f - side);
        cv_px(x, yy, sh(R_IRON, v + 0.16f)); cv_px(x, yy + 1, sh(R_IRON, v - 0.08f));
    }
}
/* the cage's radius at height y: a dome to the crown ring, straight sides, a short round under the floor */
static float gib_r(float y) {
    const float R = 11.0f;
    if (y < 18.0f) { float u = (18.0f - y) / 7.0f; return u >= 1.0f ? 0.0f : R * sqrtf(1.0f - u * u); }
    if (y > 48.0f) { float u = (y - 48.0f) / 4.0f; return u >= 1.0f ? 0.0f : R * sqrtf(1.0f - u * u); }
    return R + 0.5f * sinf(PI_F * (y - 18.0f) / 30.0f);
}
/* the cage's straps, the near ones (front) or the far: skip leaves the door's middle strap out between y 21 and 44 */
static void gib_straps(float cx, int front, int skip) {
    for (int k = 0; k < 10; k++) {
        float th = PI_F * (float)(2 * k + 1) / 10.0f, s = sinf(th), c = cosf(th);
        if ((s >= 0.0f) != front) continue;
        for (int y = 12; y <= 51; y++) {
            if (skip && k == 2 && y > 21 && y < 44) continue;
            float r = gib_r((float)y);
            if (r <= 0.3f) continue;
            int x = (int)floorf(cx + r * c), w = fabsf(s) > 0.45f ? 2 : 1;
            float v = front ? 0.28f + 0.42f * (0.5f - 0.5f * c) : 0.08f + 0.1f * (0.5f + 0.5f * c);
            for (int i = 0; i < w; i++) cv_px(x + i, y, sh(R_IRON, v + (i == 0 ? 0.08f : -0.1f)));
        }
    }
}
static void gib_rivets(float cx) {
    static const float HY[3] = { 18, 33, 48 };
    for (int h = 0; h < 3; h++)
        for (int k = 0; k < 5; k++) {
            float th = PI_F * (float)(2 * k + 1) / 10.0f, r = gib_r(HY[h]);
            int x = (int)floorf(cx + r * cosf(th)), y = (int)floorf(HY[h] + gib_tilt(HY[h]) * sinf(th));
            cv_px(x, y, sh(R_IRON, 0.95f - 0.12f * (float)(k < 2))); cv_px(x + 1, y + 1, sh(R_IRON, 0.08f));
        }
}
static void spr_cage(int id, int occupied) {
    cv_begin(cv_sprite, TS, TS);
    const float cx = 32;
    ring_bolt(32, 0);
    forged_chain(32, 4, 32, 10, 0.0f, 0, 0.0f);
    iron_ring(32, 11.5f, 2.2f, 1.6f, 1.4f, 0.0f, TAU_F, 0.0f);       /* the crown ring */
    gib_straps(cx, 0, 0);                                           /* the far side, dim */
    gib_hoop(cx, 18, gib_r(18), 0); gib_hoop(cx, 33, gib_r(33), 0); gib_hoop(cx, 48, gib_r(48), 0);
    for (int x = (int)cx - 9; x <= (int)cx + 9; x++)                /* the floor's straps, seen from above */
        if ((x - (int)cx + 20) % 5 == 0) for (int y = 47; y <= 50; y++) cv_px(x, y, sh(R_IRON, 0.14f));
    if (occupied) {                                                 /* slumped against the right side, knees up */
        prop_long(29, 44, 36, 36, 1.2f, 0, 0, 0.08f, 421);          /* the right thigh, the knee up */
        prop_long(36, 36, 39, 47, 1.0f, 0, 0, 0.12f, 423);          /* its shin */
        prop_long(28, 44, 24, 37, 1.2f, 0, 0, 0.16f, 425);          /* the left, fallen open */
        prop_long(24, 37, 23, 47, 1.0f, 0, 0, 0.2f, 427);
        cv_ellipse(29.5f, 44.0f, 3.6f, 2.2f, R_BONE, 0.12f, 0.6f);  /* the pelvis */
        cv_px(28, 44, 0); cv_px(31, 44, 0);
        for (int k = 0; k < 6; k++) prop_vertebra(30.0f + (float)k * 0.7f, 41.5f - (float)k * 2.0f, 0.05f);
        for (int k = 0; k < 4; k++) {                               /* the ribs, and the rags of a shirt on them */
            prop_rib(33.5f + (float)k * 0.5f, 32.0f + (float)k * 2.0f, 4.6f - (float)k * 0.4f, 2.2f, PI_F * 0.15f, PI_F * 0.95f, 0.08f * (float)k);
            prop_rib(31.0f + (float)k * 0.5f, 32.0f + (float)k * 2.0f, 4.2f - (float)k * 0.4f, 2.2f, PI_F * 0.05f, PI_F * 0.85f, 0.15f + 0.08f * (float)k);
        }
        for (int y = 33; y < 40; y++) for (int x = 27; x < 38; x++)
            if (h01(x, y, 431) < 0.22f && cv_sprite[y * TS + x] == C_CLEAR) cv_px(x, y, sh(R_WARM, 0.12f + 0.1f * h01(x, y, 433)));
        cv_bone(29, 30, 26.5f, 38, 1.0f, 0.2f, 0.7f);              /* the left arm in its lap */
        cv_bone(26.5f, 38, 33, 39, 0.9f, 0.2f, 0.7f);
        prop_fingers(32, 40, 435);
        prop_skull(33, 18, 1, 0, 0, 0);                             /* lolled against the bars, its jaw gone */
        cv_px(37, 29, sh(R_BONE, 0.7f)); cv_px(38, 30, sh(R_BONE, 0.6f));
    } else {
        prop_jaw(36, 47);                                           /* what is left of the last one */
        prop_fingers(26, 47, 437);
        prop_vertebra(30, 48, 0.1f);
    }
    gib_straps(cx, 1, !occupied);
    gib_hoop(cx, 18, gib_r(18), 1); gib_hoop(cx, 33, gib_r(33), 1); gib_hoop(cx, 48, gib_r(48), 1);
    if (occupied) {
        for (int y = 21; y <= 44; y += 23) for (int x = (int)cx - 6; x <= (int)cx + 6; x++) cv_px(x, y + (int)gib_tilt((float)y), sh(R_IRON, 0.62f - 0.03f * (float)(x - (int)cx + 6)));
        cv_ellipse(cx + 7.5f, 34.5f, 2.3f, 2.0f, R_IRON, 0.1f, 0.68f);   /* the padlock on the door */
        cv_px((int)cx + 8, 35, 0);
        cv_bone(37, 29, 44.5f, 34, 1.0f, 0.25f, 0.85f);             /* the right arm out through the bars */
        cv_bone(44.5f, 34, 45.5f, 45, 0.9f, 0.25f, 0.8f);
        for (int f = 0; f < 4; f++) cv_line(44.0f + (float)f * 0.8f, 46.0f, 43.8f + (float)f, 50.0f + (float)(f == 1 || f == 2), sh(R_BONE, 0.72f - 0.08f * (float)f));
    } else {                                                        /* the door hung open on its hinges, swung out to the left */
        float hx = cx - 6.5f;
        cv_line(hx, 21, hx - 9.0f, 19.0f, sh(R_IRON, 0.7f)); cv_line(hx, 22, hx - 9.0f, 20.0f, sh(R_IRON, 0.35f));
        cv_line(hx, 44, hx - 9.0f, 46.5f, sh(R_IRON, 0.66f)); cv_line(hx, 45, hx - 9.0f, 47.5f, sh(R_IRON, 0.3f));
        cv_line(hx - 4.5f, 20, hx - 4.5f, 45.5f, sh(R_IRON, 0.6f));
        cv_line(hx - 9.0f, 19, hx - 9.0f, 47.0f, sh(R_IRON, 0.78f)); cv_line(hx - 8.0f, 20, hx - 8.0f, 46.5f, sh(R_IRON, 0.4f));
        for (int k = 0; k < 2; k++) cv_ellipse(hx, 23.0f + (float)k * 19.0f, 1.3f, 1.3f, R_IRON, 0.15f, 0.8f);   /* the hinges */
        iron_ring(hx - 10.0f, 33.0f, 1.3f, 1.8f, 1.0f, PI_F * 0.4f, PI_F * 1.6f, 0.0f);   /* its hasp */
        for (int y = 34; y < 49; y++) {                             /* a rag caught on it */
            float u = (float)(y - 34) / 15.0f, w = 1.9f - 0.9f * u, c0 = hx - 11.0f + sinf((float)y * 0.6f) * 0.7f;
            for (int x = (int)(c0 - w - 1); x <= (int)(c0 + w + 1); x++)
                if (fabsf((float)x + 0.5f - c0) <= w && !(y > 44 && h01(x, y, 439) < 0.4f)) cv_px(x, y, sh(R_WARM, 0.18f + 0.18f * (float)(x < (int)c0) + 0.06f * h01(x, y / 2, 441)));
        }
    }
    gib_rivets(cx);
    iron_age(103 + (uint32_t)occupied);
    cv_outline(0);
    cv_store(id);
}

/* the bell (run II's; the puzzles' and the trials' bell too): bronze, hung by its crown in an iron stirrup from a beam; a band
 * of raised letters round its waist between raised lines, two more at the sound bow; verdigris run down from the crown and
 * the band; a crack up from a chip in the lip, so that it will not ring true; the clapper still below; its rope cut */
static void spr_bronzebell(void) {
    cv_begin(cv_sprite, TS, TS);
    static const int BX[2] = { 22, 40 };
    const float cx = 32, top = 10, bot = 41, H = bot - top;
    for (int y = 0; y < 27; y++) {                                  /* the rope, cut */
        float x = 12.0f + sinf((float)y * 0.2f) * 0.5f;
        for (int i = 0; i < 2; i++) cv_px((int)x + i, y, sh(R_WARM, ((y + i) % 3 == 0 ? 0.2f : 0.42f) - 0.14f * (float)i));
    }
    for (int k = 0; k < 4; k++) cv_line(12.5f, 27, 11.0f + (float)k, 29.0f + (float)(k & 1) * 1.5f, sh(R_WARM, 0.3f));
    cv_capsule(cx + 0.5f, bot + 1.0f, cx + 0.5f, bot + 4.0f, 0.8f, R_IRON, 0.1f, 0.5f);   /* the clapper */
    cv_ellipse(cx + 0.5f, bot + 5.5f, 2.3f, 2.1f, R_IRON, 0.08f, 0.62f);
    for (int k = 0; k < 2; k++) {                                   /* the stirrup's straps */
        float x = cx - 4.5f + (float)k * 8.0f;
        cv_capsule(x, 5, x + (k ? -1.0f : 1.0f), 12, 1.0f, R_IRON, 0.15f, 0.78f);
        cv_px((int)x, 8, sh(R_IRON, 0.95f));
    }
    iron_ring(cx, 9.5f, 3.0f, 2.6f, 1.6f, PI_F, TAU_F, 0.06f);        /* the crown's loop */
    for (int y = (int)top; y <= (int)bot + 3; y++) {
        float t = ((float)y - top) / H, tt = fminf(t, 1.0f), r = 6.4f + 0.6f * tt + 7.2f * powf(tt, 3.4f), dr = (0.6f + 24.5f * powf(tt, 2.4f)) / H;
        if (t < 0.14f) { float q = (0.14f - t) / 0.14f; r *= sqrtf(1.0f - q * q * 0.75f); dr -= q * 0.6f; }   /* the shoulder, rounded */
        for (int x = (int)(cx - r - 1); x <= (int)(cx + r + 1); x++) {
            float nx = ((float)x + 0.5f - cx) / r;
            if (fabsf(nx) > 1.0f) continue;
            float nz = sqrtf(1.0f - nx * nx), lipy = bot + 2.2f * nz;
            if ((float)y > lipy) continue;
            float ny = -dr * 0.9f, nl = sqrtf(nx * nx + ny * ny + nz * nz), ux = nx / nl, uy = ny / nl, uz = nz / nl;
            float spec = powf(fmaxf(0.0f, ux * -0.25f + uy * -0.37f + uz * 0.89f), 18.0f);
            float v = 0.1f + 0.42f * lit(ux, uy, uz) + 0.6f * spec + (nx > 0.82f ? 0.1f : 0.0f);
            float by = top + 0.52f * H - 1.0f * nz, sb = top + 0.88f * H + 0.6f * nz;   /* the band's lines, curving with the bell */
            float dy = (float)y - by;
            if (fabsf(nx) < 0.93f) {
                if ((int)floorf(dy) == 0 || (int)floorf(dy) == 4) v += 0.2f;
                else if ((int)floorf(dy) == 1 || (int)floorf(dy) == 5) v -= 0.14f;
                else if (dy > 1.0f && dy < 4.0f && fabsf(nx) < 0.82f) {   /* the letters, raised */
                    int col = (int)floorf(asinf(nx) * 9.0f + 40.0f);
                    if (col % 4 != 3 && h01(col, (int)dy, 451) < 0.62f) v += (int)dy == 2 ? 0.3f : 0.12f;
                    else v -= 0.08f;
                }
                int sy = (int)floorf((float)y - sb);
                if (sy == 0 || sy == 2) v += 0.16f; else if (sy == 1 || sy == 3) v -= 0.12f;
            }
            if ((float)y > lipy - 1.0f) v = 0.18f + 0.3f * (1.0f - fabsf(nx));   /* the lip's edge */
            int ramp = R_WOOD;
            float g = fbm((float)x / 2.5f, (float)y / 6.0f, 32, 453) + (h01(x, 1, 455) > 0.55f && h01(x, (int)(t * 4.0f), 456) > 0.3f ? 0.4f * (1.0f - t) : 0.0f) + (dy > 4.0f && dy < 12.0f && h01(x, 2, 457) > 0.72f ? 0.25f : 0.0f) + (t < 0.1f ? 0.25f : 0.0f);
            if (g > 0.76f) { ramp = R_MOSS; v = 0.22f + 0.32f * v; }   /* verdigris */
            if (v > 0.9f && ramp == R_WOOD) { cv_px(x, y, sh(R_BONE, v)); continue; }
            cv_px(x, y, sh(ramp, v));
        }
    }
    for (int k = 0; k < 15; k++) {                                  /* the crack, up from the chip, a branch off it */
        int y = (int)bot - 1 - k, x = 38 - (int)(h01(k / 2, 1, 459) * 2.0f) - k / 5;
        cv_px(x, y, sh(R_WOOD, 0.02f)); cv_px(x - 1, y, sh(R_WOOD, 0.7f));
        if (k > 5 && k < 10) { cv_px(x + (k - 5), y - 1, sh(R_WOOD, 0.04f)); cv_px(x + (k - 5), y, sh(R_WOOD, 0.5f)); }
    }
    for (int x = 36; x <= 40; x++) for (int y = (int)bot + 1 - (2 - abs(x - 38)); y <= (int)bot + 4; y++) cv_px(x, y, C_CLEAR);   /* the chip */
    timber_beam(4, 60, 0, 4, BX, 2, 461);
    cv_outline(0);
    cv_store(SP_BELL);
}

/* a root come through the vault from (x, y0), len long, w thick at its head: twisting as it goes, the bark furrowed along it,
 * a knot or two, rootlets off its sides; forks: how many lesser roots go off it */
static void hang_root(float x, float y0, float len, float w, float ph, uint32_t seed, int forks) {
    float drift = (h01(1, 1, seed) - 0.5f) * 8.0f;
    for (int y = (int)y0; y < (int)(y0 + len) && y < TS; y++) {
        float u = ((float)y - y0) / len, xx = x + sinf((float)y * 0.21f + ph) * (1.0f + 2.4f * u) + drift * u * u, ww = w * (1.0f - u * 0.82f);
        for (int dx = (int)(-ww - 1); dx <= (int)(ww + 1); dx++) {
            float nx = ((float)dx + 0.5f + (xx - floorf(xx)) - 0.5f) / (ww + 0.01f);
            if (fabsf(nx) > 1.0f) continue;
            float v = 0.1f + 0.36f * lit(nx, 0.0f, sqrtf(1.0f - nx * nx)) - 0.06f * u;
            if (ww > 1.4f && h01((int)(nx * 3.0f + 3.0f), (y + (int)(nx * 2.0f)) / 3, seed + 2) < 0.3f) v -= 0.09f;   /* furrows */
            cv_px((int)xx + dx, y, sh(R_WOOD, v));
        }
        if (ww > 1.5f && h01(y, 3, seed) > 0.96f) {                 /* a knot */
            cv_ellipse(xx + 0.3f, (float)y, ww * 0.7f, 1.6f, R_WOOD, 0.08f, 0.4f);
            cv_px((int)(xx + 0.3f), y, sh(R_WOOD, 0.04f));
        }
        if (h01(y, 4, seed) > 0.9f) {                               /* a rootlet */
            float s = h01(y, 5, seed) < 0.5f ? -1.0f : 1.0f, l = 3.0f + 5.0f * h01(y, 6, seed);
            cv_line(xx + s * ww, (float)y, xx + s * (ww + l), (float)y + l * 0.9f, sh(R_WOOD, 0.14f));
        }
        if (forks > 0 && u > 0.25f && u < 0.6f && h01(y, 7, seed) > 0.93f) {
            forks--;
            hang_root(xx + (h01(y, 8, seed) < 0.5f ? -1.0f : 1.0f) * ww * 0.6f, (float)y, (len - ((float)y - y0)) * (0.6f + 0.5f * h01(y, 9, seed)), ww * 0.55f, ph + 2.0f, seed * 3u + (uint32_t)y, 0);
        }
    }
}
/* the earth of the vault the roots come through: a ragged band, stones in it, clods hanging off it */
static void root_soil(uint32_t seed) {
    for (int x = 0; x < TS; x++) {
        int d = 2 + (int)(4.0f * fbm((float)x / 6.0f, 0.5f, 16, seed));
        for (int y = 0; y <= d; y++) cv_px(x, y, sh(R_WARM, 0.06f + 0.2f * h01(x, y, seed + 1) + 0.08f * (float)(y == d)));
        if (h01(x / 3, 2, seed) > 0.85f) cv_px(x, d + 1, sh(R_WARM, 0.2f));
    }
    for (int k = 0; k < 4; k++) cv_ellipse(6.0f + 52.0f * h01(k, 3, seed), 2.0f + 1.5f * h01(k, 4, seed), 1.8f, 1.3f, R_STONE, 0.15f, 0.6f);
}
/* soil falling from (x, y0): grains thinning as they go, a clod among them */
static void soil_fall(float x, int y0, int y1, uint32_t seed) {
    for (int y = y0; y < y1; y++)
        if (h01(y, 1, seed) < 0.42f * (1.0f - (float)(y - y0) / (float)(y1 - y0)) + 0.06f)
            cv_px((int)(x + (h01(y, 2, seed) - 0.5f) * 2.5f), y, sh(R_WARM, 0.22f + 0.18f * h01(y, 3, seed)));
    cv_ellipse(x + 0.5f, (float)(y0 + (y1 - y0) / 2), 1.2f, 1.1f, R_WARM, 0.1f, 0.4f);
}
/* roots through the vault. look 0: five roots, a skull of the dead above held in them, a rootlet in its eye; look 1: one great
 * root bearded with hair-roots, a board of the coffin it came through caught in it, its nail still in */
static void spr_hangroots(int id, int look) {
    cv_begin(cv_sprite, TS, TS);
    if (look == 0) {
        prop_skull(25, 21, 0, 0, 2, 0);
        static const float X[5] = { 8, 21, 34, 46, 57 }, LN[5] = { 22, 40, 33, 46, 18 }, W[5] = { 2.2f, 3.0f, 2.6f, 3.4f, 1.8f };
        for (int r = 0; r < 5; r++) hang_root(X[r], 2, LN[r], W[r], (float)r * 1.7f, 471 + (uint32_t)r, r == 1 || r == 3);
        cv_line(28.0f, 16.0f, 28.0f, 26.5f, sh(R_WOOD, 0.24f));      /* into its socket */
        cv_line(31.5f, 12.0f, 33.5f, 21.0f, sh(R_WOOD, 0.3f)); cv_line(33.5f, 21.0f, 37.0f, 24.0f, sh(R_WOOD, 0.2f));
        soil_fall(40, 7, 44, 473);
    } else {
        hang_root(30, 2, 30, 5.0f, 0.4f, 481, 2);
        hang_root(12, 2, 20, 2.0f, 2.0f, 483, 0); hang_root(52, 2, 26, 2.4f, 3.1f, 485, 1);
        for (int k = 0; k < 16; k++) {                               /* its beard */
            float x0 = 25.0f + 11.0f * h01(k, 1, 487), y0 = 22.0f + 8.0f * h01(k, 2, 487), l = 14.0f + 20.0f * h01(k, 3, 487);
            for (int y = 0; y < (int)l; y++) if (h01(k, y, 489) > 0.12f) cv_px((int)(x0 + sinf((float)y * 0.3f + (float)k) * 1.2f + (float)y * (h01(k, 4, 487) - 0.5f) * 0.25f), (int)y0 + y, sh(R_WOOD, 0.2f + 0.1f * h01(k, y / 4, 491) - 0.006f * (float)y));
        }
        for (int y = 15; y < 20; y++)                               /* the board, split, slant across the roots */
            for (int x = 16; x < 33; x++) {
                float yy = (float)y + ((float)x - 16.0f) * 0.22f;
                if (x > 29 && h01(x, y, 493) < 0.45f) continue;     /* splintered */
                cv_px(x, (int)yy, sh(R_WARM, (y == 15 ? 0.46f : 0.3f) - (y == 19 ? 0.16f : 0.0f) + (fmodf((float)x * 0.7f + (float)y, 3.0f) < 0.5f ? -0.06f : 0.0f)));
            }
        cv_line(20, 13, 20, 16.5f, sh(R_IRON, 0.75f)); cv_px(20, 13, sh(R_IRON, 0.95f));   /* its nail */
        soil_fall(22, 8, 40, 495); soil_fall(45, 8, 30, 497);
    }
    root_soil(499 + (uint32_t)look);
    cv_outline(0);
    cv_store(id);
}

/* a web's thread from (x0, y0) to (x1, y1), sagging: lit where it faces the light, a pixel dropped here and there with wear */
static void web_line(float x0, float y0, float x1, float y1, float sag, float v, uint32_t seed) {
    int n = (int)fmaxf(fabsf(x1 - x0), fabsf(y1 - y0) + sag) + 1;
    float slope = fabsf(y1 - y0) / (fabsf(x1 - x0) + 0.5f);
    for (int i = 0; i <= n; i++) {
        float t = (float)i / (float)n;
        int x = (int)(x0 + (x1 - x0) * t), y = (int)(y0 + (y1 - y0) * t + sag * 4.0f * t * (1.0f - t));
        if (h01(x, y, seed) < 0.06f) continue;
        cv_px(x, y, (uint8_t)(R_STONE + clampi((int)(v * 15.0f) + ((i & 1) ? 0 : -2) + (slope < 1.0f ? 1 : 0), 4, 15)));
    }
}
/* dust caught on a thread at (x, y) */
static void web_dust(float x, float y, uint32_t seed) {
    for (int k = 0; k < 4; k++) cv_px((int)(x + (h01(k, 1, seed) - 0.5f) * 3.0f), (int)(y + h01(k, 2, seed) * 2.0f), sh(R_WARM, 0.3f + 0.2f * h01(k, 3, seed)));
}
/* a dead spider, curled, at (x, y) */
static void dead_spider(float x, float y) {
    cv_ellipse(x + 0.3f, y + 2.2f, 3.2f, 2.8f, R_WARM, 0.12f, 0.52f);
    cv_px((int)x, (int)y + 1, sh(R_WARM, 0.62f)); cv_px((int)x + 1, (int)y + 3, sh(R_BONE, 0.3f));
    cv_ellipse(x - 0.2f, y - 1.4f, 1.9f, 1.6f, R_WARM, 0.14f, 0.55f);
    for (int s = -1; s <= 1; s += 2)
        for (int l = 0; l < 4; l++) {
            float ax = x + (float)s * 1.4f, ay = y - 1.5f + (float)l * 0.9f, kx = ax + (float)s * (3.4f - 0.3f * (float)l), ky = ay - 1.5f + (float)l * 1.0f;
            cv_line(ax, ay, kx, ky, sh(R_WARM, 0.42f)); cv_line(kx, ky, kx - (float)s * 2.0f, ky + 2.4f, sh(R_WARM, 0.3f));   /* folded under */
        }
}
/* a web in a corner of the vault: spokes from a hub set off its middle to anchors along the stone, the spiral slung between
 * them, a sector torn and its threads hanging, dust caught in it. look 0 has a husk wound in it; look 1 is old and mostly
 * gone, a dead spider curled at the end of a thread */
static void spr_cornerweb(int id, int look) {
    cv_begin(cv_sprite, TS, TS);
    uint32_t sd = 501u + (uint32_t)look * 37u;
    float hx = look ? 34.0f : 28.0f, hy = look ? 7.0f : 10.0f;
    float ang[9], len[9];
    int ns = look ? 6 : 9;
    for (int s = 0; s < ns; s++) {                                  /* the spokes, from the stone round to the stone */
        ang[s] = PI_F * (-0.05f + 1.1f * (float)s / (float)(ns - 1)) + (h01(s, 1, sd) - 0.5f) * 0.18f;
        len[s] = (look ? 24.0f : 30.0f) + 8.0f * h01(s, 2, sd);
        float ex = hx + cosf(ang[s]) * len[s], ey = fmaxf(0.0f, hy + sinf(ang[s]) * len[s]);
        web_line(hx, hy, ex, ey, 0.0f, 0.78f, sd + 3);
    }
    for (int s = 0; s < 4; s++) web_line(hx, hy, 6.0f + 18.0f * (float)s, 0.0f, 0.0f, 0.7f, sd + 5);   /* to the vault */
    int rings = look ? 3 : 7;
    for (int r = 1; r <= rings; r++) {
        float f = (float)r / 7.6f;
        for (int s = 0; s + 1 < ns; s++) {
            if (!look && s >= 5 && s <= 6 && r > 3) {               /* torn: the threads hang */
                if (h01(r, s, sd + 7) < 0.6f) { float x0 = hx + cosf(ang[s]) * len[s] * f, y0 = hy + sinf(ang[s]) * len[s] * f; web_line(x0, y0, x0 + 1.5f, y0 + 5.0f + 6.0f * h01(r, s, sd + 8), 0.0f, 0.6f, sd + 9); }
                continue;
            }
            float x0 = hx + cosf(ang[s]) * len[s] * f, y0 = hy + sinf(ang[s]) * len[s] * f, x1 = hx + cosf(ang[s + 1]) * len[s + 1] * f, y1 = hy + sinf(ang[s + 1]) * len[s + 1] * f;
            web_line(x0, fmaxf(0.0f, y0), x1, fmaxf(0.0f, y1), 0.6f + 1.4f * f, 0.82f - 0.25f * f, sd + 11);
        }
    }
    if (!look) {
        web_dust(hx + 9.0f, hy + 9.0f, sd + 13); web_dust(hx - 14.0f, hy + 6.0f, sd + 14); web_dust(hx + 3.0f, hy + 20.0f, sd + 15);
        silk_husk(hx + 13.0f, hy + 18.0f, hx + 11.0f, hy + 8.0f, 1, 1);
    } else {
        for (int k = 0; k < 4; k++) {                               /* long threads, slung and hanging */
            float x0 = 4.0f + 14.0f * (float)k + 6.0f * h01(k, 1, sd), x1 = x0 + 10.0f + 12.0f * h01(k, 2, sd);
            web_line(x0, 0.0f, x1, 1.0f, 6.0f + 10.0f * h01(k, 3, sd), 0.62f, sd + 17);
            web_dust(x0 + (x1 - x0) * 0.5f, 6.0f + 9.0f * h01(k, 3, sd), sd + 19 + (uint32_t)k);
        }
        for (int k = 0; k < 5; k++) { float x = 10.0f + 10.0f * (float)k + 4.0f * h01(k, 5, sd); web_line(x, 2.0f, x + 1.0f, 12.0f + 14.0f * h01(k, 6, sd), 0.0f, 0.56f, sd + 23); }
        web_line(hx + 6.0f, hy + 4.0f, hx + 7.0f, 38.0f, 0.0f, 0.6f, sd + 25);
        dead_spider(hx + 7.0f, 40.0f);
    }
    cv_store(id);
}

/* a body wound in silk, hung at (x, y), hh long and ww across, its stuff showing through: bony fingers out of its side */
static void cocoon(float x, float y, float hh, float ww, uint32_t seed) {
    for (int j = (int)-hh; j <= (int)hh; j++) {
        float u = (float)j / (hh + 0.5f), w = ww * powf(fmaxf(0.0f, 1.0f - u * u), 0.7f) * (1.0f + 0.12f * sinf((float)j * 0.7f));
        for (int i = (int)floorf(x - w); i <= (int)ceilf(x + w); i++) {
            float nx = ((float)i + 0.5f - x) / (w + 0.01f);
            if (fabsf(nx) > 1.0f) continue;
            float v = 0.24f + 0.48f * lit(nx, u * 0.6f, sqrtf(fmaxf(0.0f, 1.0f - nx * nx))), band = fmodf((float)j * 0.8f + nx * 2.4f + 20.0f, 2.2f);
            if (band < 0.7f) v -= 0.16f;                            /* wound slantwise */
            if (h01(i, j + 40, seed) < 0.08f) v -= 0.2f;
            cv_px(i, (int)y + j, sh(R_BONE, v * 0.85f));
        }
    }
    for (int f = 0; f < 3; f++) {                                   /* fingers out through the silk */
        float bx = x + ww * 0.8f, by = y + 2.0f + (float)f * 1.6f;
        cv_line(bx, by, bx + 3.0f, by + 1.5f + (float)f * 0.5f, sh(R_BONE, 0.82f - 0.1f * (float)f));
        cv_line(bx + 3.0f, by + 1.5f + (float)f * 0.5f, bx + 4.0f, by + 3.5f + (float)f * 0.6f, sh(R_BONE, 0.7f - 0.1f * (float)f));
    }
}
/* a sheet of web. look 0: slung from the vault and sagging into a hammock, its weave thicker where it gathers, its edge a
 * cord, a hole torn in it, tatters hanging, a husk and a rib lying in its belly; look 1: a veil of threads hung straight
 * down, tied across here and there and gathered at their feet, a body wound in it with its fingers out */
static void spr_websheet2(int id, int look) {
    cv_begin(cv_sprite, TS, TS);
    uint32_t sd = 521u + (uint32_t)look * 41u;
    if (look == 0) {
        float hx = 41.0f, hy = 17.0f;
        for (int x = 0; x < TS; x++) {
            float t = (float)x / 63.0f, edge = 3.0f + 2.0f * t + 27.0f * 4.0f * t * (1.0f - t) * (0.85f + 0.15f * sinf((float)x * 0.4f));
            for (int y = 0; y <= (int)edge; y++) {
                float dd = (float)y / edge, ox = ((float)x - hx) / 6.5f, oy = ((float)y - hy) / 4.5f;
                if (ox * ox + oy * oy < 1.0f + 0.3f * h01(x, y, sd)) continue;   /* the hole */
                int mesh = fmodf((float)x + (float)y * 0.6f + 40.0f, 5.0f) < 1.0f || fmodf((float)x - (float)y * 0.7f + 80.0f, 6.0f) < 1.0f;
                float keep = mesh ? 0.45f + 0.5f * dd : 0.04f + 0.3f * dd * dd;
                if (h01(x, y, sd + 1) > keep) continue;
                cv_px(x, y, (uint8_t)(R_STONE + clampi(7 + (int)(dd * 5.0f) + (int)(h01(x, y, sd + 2) * 2.0f) - (x > 40) , 4, 14)));
            }
            cv_px(x, (int)edge, (uint8_t)(R_STONE + 13 - (x & 1)));        /* its edge, a cord */
            cv_px(x, (int)edge + 1, (uint8_t)(R_STONE + 9));
            if (h01(x / 2, 3, sd) > 0.78f) web_line((float)x, edge + 1.0f, (float)x + (h01(x, 4, sd) - 0.5f) * 3.0f, edge + 4.0f + 14.0f * h01(x, 5, sd), 0.0f, 0.58f, sd + 6);   /* tatters */
        }
        for (int k = 0; k < 6; k++) {                               /* the hole's frayed lip */
            float a = PI_F * (0.1f + 0.8f * (float)k / 5.0f), x0 = hx + cosf(a) * 6.5f, y0 = hy + sinf(a) * 4.5f;
            web_line(x0, y0, x0 + (h01(k, 1, sd) - 0.5f) * 2.0f, y0 + 3.0f + 6.0f * h01(k, 2, sd), 0.0f, 0.6f, sd + 7);
        }
        silk_husk(20, 25, 20, 19, 0, -1);
        prop_rib(29, 30, 4.0f, 2.0f, PI_F * 1.05f, PI_F * 1.9f, 0.1f);
        web_dust(12, 20, sd + 8); web_dust(52, 14, sd + 9); web_dust(33, 27, sd + 10);
    } else {
        int bundle[4] = { 12, 27, 41, 55 };
        for (int s = 0; s < 12; s++) {                              /* the threads, drawn in to their clumps */
            float x0 = 2.0f + 60.0f * ((float)s + h01(s, 1, sd)) / 12.0f, l = 26.0f + 26.0f * h01(s, 2, sd);
            float bx = (float)bundle[clampi((int)(x0 / 16.0f), 0, 3)] + (h01(s, 3, sd) - 0.5f) * 3.0f, mx = (x0 + bx) * 0.5f + (h01(s, 4, sd) - 0.5f) * 6.0f;
            web_line(x0, 0.0f, mx, l * 0.5f, 0.0f, 0.42f + 0.22f * h01(s, 5, sd), sd + 11 + (uint32_t)s);
            web_line(mx, l * 0.5f, bx, l, 0.0f, 0.42f + 0.22f * h01(s, 5, sd), sd + 12 + (uint32_t)s);
        }
        for (int k = 0; k < 16; k++) {                              /* tied across, sagging */
            float x0 = 2.0f + 54.0f * h01(k, 6, sd), y0 = 3.0f + 26.0f * h01(k, 7, sd);
            web_line(x0, y0, x0 + 5.0f + 8.0f * h01(k, 8, sd), y0 + (h01(k, 9, sd) - 0.5f) * 4.0f, 1.5f, 0.6f, sd + 31);
        }
        for (int b = 0; b < 4; b++) {                               /* gathered into clumps */
            float y = 40.0f + 14.0f * h01(b, 10, sd);
            for (int k = 0; k < 6; k++) cv_px(bundle[b] + (int)((h01(b, k, sd + 33) - 0.5f) * 3.0f), (int)y + k / 2, (uint8_t)(R_STONE + 10 + (k & 1)));
            web_dust((float)bundle[b], y + 2.0f, sd + 35 + (uint32_t)b);
        }
        web_line(33, 0, 33, 16, 0.0f, 0.8f, sd + 37);
        cocoon(33, 27, 11.0f, 4.6f, sd + 39);
    }
    cv_store(id);
}

/* icicles where something cold has gone by: rime feathered along the stone, icicles of every length ringed as they grew,
 * clear at their edges, a drop at each tip; in the longest, a moth caught in the ice */
static void spr_iceteeth(void) {
    cv_begin(cv_sprite, TS, TS);
    for (int i = 0; i < 13; i++) {
        float x = 4.0f + 56.0f * ((float)i + 0.5f * h01(i, 1, 651)) / 13.0f, len = 6.0f + 30.0f * h01(i, 2, 651) * h01(i, 3, 651) + 4.0f, w = 1.3f + 1.7f * h01(i, 4, 651);
        if (i == 6) { len = 41.0f; w = 3.4f; }
        for (int y = 0; y < (int)len; y++) {
            float u = (float)y / len, hw = w * (1.0f - u) * (1.0f + 0.14f * sinf((float)y * 1.3f + (float)i));
            for (int dx = (int)(-hw - 1); dx <= (int)(hw + 1); dx++) {
                float nx = ((float)dx + 0.5f) / (hw + 0.01f);
                if (fabsf(nx) > 1.0f) continue;
                float v = 0.26f + 0.3f * (1.0f - fabsf(nx)) * 0.5f - 0.12f * u;
                if (nx < -0.55f) v += 0.38f;                         /* its lit edge */
                else if (nx > 0.6f) v -= 0.1f;
                if (fabsf(nx + 0.15f) < 0.2f && hw > 1.4f) v += 0.12f;   /* the streak down its core */
                if (((int)(y + (int)(nx * 2.0f)) % 4) == 0) v += 0.07f;   /* rings */
                cv_px((int)x + dx, y, sh(R_GHOST, v));
            }
        }
        if (h01(i, 5, 651) < 0.6f) cv_px((int)x, (int)len + 1, sh(R_GHOST, 0.85f));
        if (h01(i, 5, 651) < 0.2f) cv_px((int)x, (int)len + 4, sh(R_GHOST, 0.7f));
    }
    {                                                               /* the moth */
        float x = 4.0f + 56.0f * (6.0f + 0.5f * h01(6, 1, 651)) / 13.0f, y = 15.0f;
        cv_px((int)x, (int)y, sh(R_WARM, 0.12f)); cv_px((int)x, (int)y + 1, sh(R_WARM, 0.1f)); cv_px((int)x, (int)y + 2, sh(R_WARM, 0.12f));
        cv_px((int)x - 1, (int)y, sh(R_WARM, 0.3f)); cv_px((int)x + 1, (int)y, sh(R_WARM, 0.24f));
        cv_px((int)x - 1, (int)y + 1, sh(R_WARM, 0.22f)); cv_px((int)x + 1, (int)y + 1, sh(R_WARM, 0.18f));
        cv_px((int)x - 2, (int)y - 1, sh(R_WARM, 0.26f)); cv_px((int)x + 2, (int)y - 1, sh(R_WARM, 0.2f)); cv_px((int)x, (int)y - 1, sh(R_WARM, 0.08f));
    }
    for (int x = 0; x < TS; x++) {                                  /* the rime along the stone, feathered */
        int d = 1 + (int)(2.5f * fbm((float)x / 5.0f, 0.3f, 16, 653));
        for (int y = 0; y <= d; y++) cv_px(x, y, sh(R_GHOST, 0.5f + 0.3f * h01(x, y, 655) - 0.1f * (float)y));
        if (h01(x, 1, 657) > 0.55f) cv_px(x + ((x & 1) ? 1 : -1), d + 1, sh(R_GHOST, 0.72f));
        if (h01(x, 2, 657) > 0.8f) cv_px(x, d + 2, sh(R_GHOST, 0.6f));
    }
    cv_outline(0);
    cv_store(SP_ICICLES);
}

/* every prop drawn so far, at the level's age, and washed in run II */
static void props_level(int level) {
    int wet = crawl_set(level) == 1;
    spr_bonelitter(SP_BONES1, 0); spr_bonelitter(SP_BONES1_2, 1); spr_bonelitter(SP_BONES1_3, 2);
    spr_boneheap(SP_BONES2, 0); spr_boneheap(SP_BONES2_2, 1);
    spr_skullstack(SP_SKULLPILE, 0); spr_skullstack(SP_SKULLPILE_2, 1); spr_skullstack(SP_SKULLPILE_3, 2);
    for (int f = 0; f < 2; f++) { spr_totempost(SP_TOTEM_A + f, 0, f); spr_totempost(SP_TOTEM2_A + f, 1, f); }
    spr_candlecluster(SP_CANDLES, 0); spr_candlecluster(SP_CANDLES_2, 1); spr_candlecluster(SP_CANDLES_3, 2);
    for (int f = 0; f < 2; f++) { spr_prickstand(SP_CANDLESTAND_A + f, f); spr_corona(SP_CHANDELIER_A + f, f); spr_hornlantern(SP_LANTERN_A + f, f); }
    for (int f = 0; f < 3; f++) { spr_hearth(SP_FIREPIT_A + f, f); spr_gbrazier(SP_BRAZIER_A + f, f); }
    static const int16_t GRAVES[4][2] = { { SP_TOMB_ROUND, SP_TOMB_ROUND_2 }, { SP_TOMB_CROSS, SP_TOMB_CROSS_2 }, { SP_TOMB_OBELISK, SP_TOMB_OBELISK_2 }, { SP_TOMB_BROKEN, SP_TOMB_BROKEN_2 } };
    for (int k = 0; k < 4; k++) for (int l = 0; l < 2; l++) spr_grave(GRAVES[k][l], k, l);
    spr_riser_grave(SP_MOUND, 0); spr_riser_grave(SP_PIT, 1);
    spr_hangchains(SP_CHAINS, 0); spr_hangchains(SP_CHAINS_2, 1); spr_hookbeam();
    spr_cage(SP_GIBBET, 0); spr_cage(SP_GIBBET_SKEL, 1); spr_bronzebell();
    spr_hangroots(SP_ROOTS, 0); spr_hangroots(SP_ROOTS_2, 1);
    spr_cornerweb(SP_COBWEB, 0); spr_cornerweb(SP_COBWEB_2, 1); spr_websheet2(SP_WEBSHEET, 0); spr_websheet2(SP_WEBSHEET_2, 1);
    spr_iceteeth();
    if (wet) {                                                      /* what hangs: rust, and water dripping off it; the webs beaded */
        static const int16_t HANGS[] = { SP_CHAINS, SP_CHAINS_2, SP_HOOKS, SP_GIBBET, SP_GIBBET_SKEL, SP_BELL, SP_ROOTS, SP_ROOTS_2,
                                         SP_COBWEB, SP_COBWEB_2, SP_WEBSHEET, SP_WEBSHEET_2 };
        for (int i = 0; i < (int)(sizeof HANGS / sizeof HANGS[0]); i++) prop_wash(HANGS[i], 1, 801u + (uint32_t)i * 3u);
    }
    if (wet) {
        for (int k = 0; k < 4; k++) for (int l = 0; l < 2; l++) prop_wash(GRAVES[k][l], 0, 771u + (uint32_t)(k * 2 + l));
        prop_wash(SP_MOUND, 0, 781); prop_wash(SP_PIT, 0, 783);
    }
    if (wet) {
        for (int f = 0; f < 2; f++) prop_wash(SP_CHANDELIER_A + f, 1, 743);   /* iron overhead: rust and drips */
        for (int f = 0; f < 3; f++) { prop_wash(SP_FIREPIT_A + f, 0, 745); prop_wash(SP_BRAZIER_A + f, 0, 747); }
    }
    {                                                               /* the brazier put out: the fire gone, ash where it sat; a trial's, out and lit */
        uint8_t *d = crawl_spr[SP_BRAZIER_OUT];
        memcpy(d, crawl_spr[SP_BRAZIER_A], TS * TS);
        for (int i = 0; i < TS * TS; i++)
            if (IS_FULLBRIGHT(d[i]) && d[i] != C_CLEAR) d[i] = (d[i + 1 < TS * TS ? i + 1 : i] == C_CLEAR || ((i * 7919) % 5)) ? (uint8_t)C_CLEAR : (uint8_t)(R_WARM + 3);
        memcpy(crawl_spr[SP_TR_BRAZ0], crawl_spr[SP_BRAZIER_OUT], TS * TS); memcpy(crawl_spr[SP_TR_BRAZ1], crawl_spr[SP_BRAZIER_A], TS * TS);
    }
    if (wet) {
        static const int16_t STANDS[] = { SP_BONES1, SP_BONES1_2, SP_BONES1_3, SP_BONES2, SP_BONES2_2, SP_SKULLPILE, SP_SKULLPILE_2, SP_SKULLPILE_3,
                                          SP_TOTEM_A, SP_TOTEM_B, SP_TOTEM2_A, SP_TOTEM2_B };
        for (int i = 0; i < (int)(sizeof STANDS / sizeof STANDS[0]); i++) {
            int id = STANDS[i], first = id == SP_TOTEM_B ? SP_TOTEM_A : (id == SP_TOTEM2_B ? SP_TOTEM2_A : id);   /* a prop's frames wash alike */
            prop_wash(id, 0, 61u + (uint32_t)first * 7u);
        }
    }
}
/* which props have other looks: each base's looks, in order (crawl_alt) */
static void props_alts(void) {
    memset(crawl_alt, 0xFF, sizeof crawl_alt);
    static const int16_t ALTS[][NALT] = {
        { SP_BONES1, SP_BONES1_2, SP_BONES1_3, -1 }, { SP_BONES2, SP_BONES2_2, -1, -1 }, { SP_SKULLPILE, SP_SKULLPILE_2, SP_SKULLPILE_3, -1 },
        { SP_TOTEM_A, SP_TOTEM2_A, -1, -1 }, { SP_CANDLES, SP_CANDLES_2, SP_CANDLES_3, -1 },
        { SP_TOMB_ROUND, SP_TOMB_ROUND_2, -1, -1 }, { SP_TOMB_CROSS, SP_TOMB_CROSS_2, -1, -1 }, { SP_TOMB_OBELISK, SP_TOMB_OBELISK_2, -1, -1 },
        { SP_TOMB_BROKEN, SP_TOMB_BROKEN_2, -1, -1 }, { SP_CHAINS, SP_CHAINS_2, -1, -1 }, { SP_ROOTS, SP_ROOTS_2, -1, -1 },
        { SP_COBWEB, SP_COBWEB_2, -1, -1 }, { SP_WEBSHEET, SP_WEBSHEET_2, -1, -1 },
    };
    for (int i = 0; i < (int)(sizeof ALTS / sizeof ALTS[0]); i++) memcpy(crawl_alt[ALTS[i][0]], ALTS[i], sizeof ALTS[i]);
}

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
    props_level(level);                                             /* the props, at its age, washed in run II */
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
    memset(crawl_dress_ix, 0xFF, sizeof crawl_dress_ix);           /* no dressing until a creature draws its own */
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

    props_alts();
    spr_urn();
    spr_pillar();
    spr_skeleton(SP_SKEL_A, 1.0f, 0.0f, 3, 0);
    spr_skeleton(SP_SKEL_B, -1.0f, 0.0f, 3, 0);
    spr_skeleton(SP_SKEL_D1, 0.0f, 0.35f, 3, 0);
    spr_skeleton(SP_SKEL_D2, 0.0f, 0.7f, 3, 0);
    spr_skeleton(SP_SKEL_D3, 0.0f, 1.0f, 3, 0);
    spr_spectre(SP_GHOST_A, 0.0f, 0);
    spr_spectre(SP_GHOST_B, PI_F, 0);
    spr_spectre(SP_GHOST_D1, 1.0f, 1);
    spr_spectre(SP_GHOST_D2, 2.5f, 1);
    spr_bat(SP_BAT_A, 0);
    spr_bat(SP_BAT_B, 1);
    spr_bat(SP_BAT_D, 2);
    bat_dress(SP_BAT_A, 1.0f); bat_dress(SP_BAT_B, -1.0f);
    spr_fire(SP_FIRE_A, 0);
    spr_fire(SP_FIRE_B, 1);
    for (int f = 0; f < 3; f++) spr_boom(SP_BOOM1 + f, f);
    spr_skeleton(SP_SKEL_T, 0.3f, 0.0f, 3, 1);
    spr_shieldskel(SP_SHIELD_A, 1.0f, 0, 1); spr_shieldskel(SP_SHIELD_B, -1.0f, 0, 1); spr_shieldskel(SP_SHIELD_S, 0.2f, 1, 1);
    spr_shieldskel(SP_SHIELD_XA, 1.0f, 0, 0); spr_shieldskel(SP_SHIELD_XB, -1.0f, 0, 0); spr_shieldskel(SP_SHIELD_XS, 0.2f, 1, 0);
    for (int h = 0; h < 4; h++) { spr_wskull(SP_WSKULL0 + h, h); if (h < 3) wskull_dress(SP_WSKULL0 + h); }
    for (int f = 0; f < 3; f++) { spr_spider(SP_SPIDER_A + f, f); spider_dress(SP_SPIDER_A + f); }
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
    spr_tomb(SP_TOMB_FRESH, 4);                                     /* her plot's newest (the four kinds are props_level's) */
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
    spr_shambler(SP_SHAMB_A, 1.0f, 0); spr_shambler(SP_SHAMB_B, -1.0f, 0); spr_shambler(SP_SHAMB_GRAB, 0.0f, 1);
    shambler_dress(SP_SHAMB_A, 1.0f, 0); shambler_dress(SP_SHAMB_B, -1.0f, 0); shambler_dress(SP_SHAMB_GRAB, 0.0f, 1);
    spr_shambler(SP_SHAMB_RISE1, 0.0f, 2); spr_shambler(SP_SHAMB_RISE2, 0.0f, 3); spr_shambler(SP_SHAMB_D1, 0.0f, 4); spr_shambler(SP_SHAMB_D2, 0.0f, 5);
    spr_drowned(SP_DROWN_A, 1.0f, 0); spr_drowned(SP_DROWN_B, -1.0f, 1); spr_drowned(SP_DROWN_GRAB, 0.0f, 2);
    drowned_dress(SP_DROWN_A, 0); drowned_dress(SP_DROWN_B, 0); drowned_dress(SP_DROWN_GRAB, 1);
    spr_drowned(SP_DROWN_RISE, 0.0f, 3); spr_drowned(SP_DROWN_BURST, 0.0f, 4); spr_drowned(SP_DROWN_POOL, 0.0f, 5);
    spr_ghoul(SP_GHOUL_A, 1.0f, 0); spr_ghoul(SP_GHOUL_B, -1.0f, 1); spr_ghoul(SP_GHOUL_LEAP, 0.0f, 2); spr_ghoul(SP_GHOUL_FEED, 0.0f, 3); spr_ghoul(SP_GHOUL_D, 0.0f, 4);
    ghoul_dress(SP_GHOUL_A, 0.0f, 0.0f); ghoul_dress(SP_GHOUL_B, 0.0f, 0.0f); ghoul_dress(SP_GHOUL_LEAP, -10.0f, 0.0f); ghoul_dress(SP_GHOUL_FEED, 0.0f, 1.0f);
    spr_bestiary_bones();
    for (int f = 0; f < 4; f++) { spr_hunter(SP_HUNTER_A + f, f); if (f < 3) hunter_dress(SP_HUNTER_A + f, f); spr_brood(SP_BROOD_A + f, f); if (f < 3) brood_dress(SP_BROOD_A + f, f); }
    for (int f = 0; f < 3; f++) spr_bspider(SP_BSPIDER_A + f, f);   /* the Wandering Spider's own */
    spr_bspider(SP_BSPIDER_REAR, 3); spr_bspider(SP_BSPIDER_D, 4);
    for (int f = 0; f < 3; f++) spr_sac(SP_SAC_A + f, f);
    spr_ling(SP_LING_A, 0); spr_ling(SP_LING_B, 1);
    spr_wraith_big(SP_WRAITH_A, 0.0f, 0); spr_wraith_big(SP_WRAITH_B, PI_F, 1); spr_wraith_big(SP_WRAITH_REACH, 1.0f, 2); spr_wraith_big(SP_WRAITH_BACK, 2.0f, 3);
    spr_shade(SP_SHADE_A, 0.0f, 0); spr_shade(SP_SHADE_B, PI_F, 1); spr_shade(SP_SHADE_TOUCH, 1.0f, 2); spr_shade(SP_SHADE_D, 2.0f, 3);
    for (int f = 0; f < 4; f++) { spr_imp(SP_IMP_A + f, f); if (f < 3) imp_dress(SP_IMP_A + f); }
    spr_ember(SP_EMBER_A, 0); spr_ember(SP_EMBER_B, 1);
    for (int f = 0; f < 5; f++) { spr_gargoyle(SP_GARG_PERCH + f, f); if (f >= 1 && f <= 3) garg_dress(SP_GARG_PERCH + f); }
    spr_broken_arms();
    spr_puddle(SP_PUDDLE_A, 0); spr_puddle(SP_PUDDLE_B, 1); spr_stalag();
    for (int f = 0; f < 3; f++) spr_magma(SP_MAGMA_A + f, f);
    spr_collar(); spr_carcass(); spr_armourstand(); spr_helm(); spr_timber();
    spr_minelamp(SP_MINELAMP_A, 0); spr_minelamp(SP_MINELAMP_B, 1); spr_cart(); spr_fungus();
    spr_crystal(SP_CRYSTAL_A, 0); spr_crystal(SP_CRYSTAL_B, 1); spr_obsidian();
    spr_altar(SP_ALTAR_A, 0); spr_altar(SP_ALTAR_B, 1);
    spr_sides();
    find_glints();
}
