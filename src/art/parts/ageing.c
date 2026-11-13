/* A part of crawl_art.c (docs/PLAN-tidy.md): it is #included there in its place and never compiled on its own.
 * Ranks' colours (build_rank_luts); each level's age: decay_tex, level_palette; a pocket's theme (crawl_art_theme). */

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
