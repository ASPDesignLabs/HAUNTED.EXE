/* A part of crawl_art.c (docs/PLAN-tidy.md): it is #included there in its place and never compiled on its own.
 * The palette's ramps, the light colormap, and noise (hsh, vnoise, fbm). */

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

