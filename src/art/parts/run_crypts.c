/* A part of crawl_art.c (docs/PLAN-tidy.md): it is #included there in its place and never compiled on its own.
 * Each run's own look begins: the course grid; the Crypts' materials, blends, what stands on their walls, floors, ceilings, doors and keys. */

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

