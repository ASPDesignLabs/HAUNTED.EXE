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
    spr_frost(); spr_spear(); spr_wisp(); spr_spark();
    spr_burn(SP_BURN_A, 0); spr_burn(SP_BURN_B, 1);
    spr_soulurn(); spr_ringitem(); spr_pedestal(); spr_items(); spr_black_altar(); spr_double();
    spr_shrine_body(0); spr_shrine_body(1); spr_shrine_glow(0); spr_shrine_glow(1);
    for (int k = 0; k <= 9; k++) spr_srune(k);
    spr_ccoin();
    spr_scrip();
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
    for (int f = 0; f < 3; f++) spr_firepit(SP_FIREPIT_A + f, f);
    spr_totem(SP_TOTEM_A, 0); spr_totem(SP_TOTEM_B, 1);
    spr_shambler(SP_SHAMB_A, 1.0f, 0); spr_shambler(SP_SHAMB_B, -1.0f, 0); spr_shambler(SP_SHAMB_GRAB, 0.0f, 1);
    spr_shambler(SP_SHAMB_RISE1, 0.0f, 2); spr_shambler(SP_SHAMB_RISE2, 0.0f, 3); spr_shambler(SP_SHAMB_D1, 0.0f, 4); spr_shambler(SP_SHAMB_D2, 0.0f, 5);
    spr_drowned(SP_DROWN_A, 1.0f, 0); spr_drowned(SP_DROWN_B, -1.0f, 1); spr_drowned(SP_DROWN_GRAB, 0.0f, 2);
    spr_drowned(SP_DROWN_RISE, 0.0f, 3); spr_drowned(SP_DROWN_BURST, 0.0f, 4); spr_drowned(SP_DROWN_POOL, 0.0f, 5);
    spr_ghoul(SP_GHOUL_A, 1.0f, 0); spr_ghoul(SP_GHOUL_B, -1.0f, 1); spr_ghoul(SP_GHOUL_LEAP, 0.0f, 2); spr_ghoul(SP_GHOUL_FEED, 0.0f, 3); spr_ghoul(SP_GHOUL_D, 0.0f, 4);
    spr_bestiary_bones();
    for (int f = 0; f < 4; f++) { spr_hunter(SP_HUNTER_A + f, f); spr_brood(SP_BROOD_A + f, f); }
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
    spr_collar(); spr_icicles(); spr_bell(); spr_carcass(); spr_armourstand(); spr_helm(); spr_timber();
    spr_minelamp(SP_MINELAMP_A, 0); spr_minelamp(SP_MINELAMP_B, 1); spr_cart(); spr_fungus();
    spr_crystal(SP_CRYSTAL_A, 0); spr_crystal(SP_CRYSTAL_B, 1); spr_obsidian();
    spr_altar(SP_ALTAR_A, 0); spr_altar(SP_ALTAR_B, 1);
    spr_sides();
    find_glints();
}
