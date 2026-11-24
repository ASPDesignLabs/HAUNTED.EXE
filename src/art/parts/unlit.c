/* A part of crawl_art.c (docs/PLAN-tidy.md): it is #included there in its place and never compiled on its own.
 * The unlit torch (docs/PLAN-events.md, step 2; the owner's sheet, media/sheets/events/torch_unlit.png): the run's own light on the
 * wall as an event leaves it. Made from tools/torchmock.c's, which now draws its sheet from these slots. */

static uint8_t unl_shd(int ramp, float v) { int k = (int)(v * 15.0f + 0.5f); return (uint8_t)(ramp + (k < 0 ? 0 : k > 15 ? 15 : k)); }
static float unl_vnoise(float x, float y, uint32_t seed) {          /* value noise, smooth enough for a flame's edge */
    int ix = (int)floorf(x), iy = (int)floorf(y); float fx = x - ix, fy = y - iy;
    #define HH(a, b) ((float)(((uint32_t)(a) * 374761393u + (uint32_t)(b) * 668265263u + seed * 2246822519u) * 3266489917u >> 8 & 0xFFFF) / 65535.0f)
    float a = HH(ix, iy), b = HH(ix + 1, iy), c = HH(ix, iy + 1), d = HH(ix + 1, iy + 1);
    #undef HH
    fx = fx * fx * (3 - 2 * fx); fy = fy * fy * (3 - 2 * fy);
    return a + (b - a) * fx + (c - a) * fy + (a - b - c + d) * fx * fy;
}

/* What an event makes of the lit sconce (T_FEAT + F1_TORCH): the same iron and wood, the flame taken away; then a small flame
 * (guttering), embers and a thread of smoke (just snuffed), or a charred head and nothing (cold). Column-major, as the game's. */
static void unl_make(const uint8_t *lit, uint8_t *out, int v) {
    memcpy(out, lit, TS * TS);
    for (int i = 0; i < TS * TS; i++) if (out[i] >= R_FLAME && out[i] < R_FLAME + 16) out[i] = C_CLEAR;
    for (int y = 19; y <= 23; y++)                                  /* the head, burnt: charcoal, a crack of lighter ash */
        for (int x = 30; x <= 33; x++) {
            uint8_t c = unl_shd(R_IRON, 0.06f + 0.05f * ((x + y) & 1));
            if (y == 19 && (x == 30 || x == 33)) continue;          /* its rounded top */
            if ((x == 31 && y == 21) || (x == 32 && y == 22)) c = unl_shd(R_STONE, 0.35f);
            out[x * TS + y] = c;
        }
    if (v == UL_SNUFFED) {
        static const int EMB[][2] = { { 31, 19 }, { 32, 20 }, { 30, 21 }, { 33, 22 } };
        for (int k = 0; k < 4; k++) out[EMB[k][0] * TS + EMB[k][1]] = (uint8_t)(R_FLAME + 5 + (k & 1) * 3);
        for (int y = 2; y <= 18; y++) {                             /* smoke, rising and leaning, thinning as it goes */
            float u = (18 - y) / 16.0f, cx = 31.5f + 2.2f * sinf(y * 0.42f) * u;
            for (int x = 27; x <= 37; x++) {
                float d = fabsf(x + 0.5f - cx);
                if (d > 1.1f + u * 1.4f || ((x + y) & 1) == (y > 10)) continue;
                if (u > 0.5f && ((x * 7 + y * 3) % 5) < 2) continue;
                out[x * TS + y] = unl_shd(R_STONE, 0.62f - 0.25f * u);
            }
        }
    }
    if (v == UL_GUTTER0 || v == UL_GUTTER1) {                         /* a low flame, blue at the root, as a torch near out */
        float tall = v == UL_GUTTER0 ? 0.42f : 0.55f, sway = v == UL_GUTTER0 ? 1.0f : -0.8f;
        for (int y = 6; y <= 20; y++)
            for (int x = 24; x <= 40; x++) {
                float u = (20 - y) / (14.0f * tall);
                if (u > 1.0f) continue;
                float cx = 31.5f + sway * u * u * 2.0f;
                float w = 3.6f * powf(fmaxf(0.0f, sinf(3.14159f * (0.18f + 0.82f * (1.0f - u)))), 0.8f) * (1.0f - 0.35f * u);
                if (w < 0.3f) continue;
                float d = fabsf(x + 0.5f - cx) / (w + 0.01f), n = unl_vnoise(x / 3.0f, y / 3.0f + v * 5.0f, 77u) * 0.5f;
                if (d > 1.0f - 0.25f * n) continue;
                int heat = (int)((0.75f - d * 0.6f - u * 0.5f + n * 0.3f) * 15.0f);
                out[x * TS + y] = u < 0.18f && d < 0.6f ? unl_shd(R_GHOST, 0.75f) : (uint8_t)(R_FLAME + (heat < 1 ? 1 : heat > 11 ? 11 : heat));
            }
    }
}

/* The other runs' lights (the Catacombs' lantern, the Hollows' lamp, the Deep's seam of fire) keep their own shapes: the flame's
 * hottest pixels stay, low, for guttering; the rest go to what the light leaves behind: dark glass where the flame is held between
 * metal, the bare wall where it stood in the air, and for the Deep a cooled crack with a dull red vein. */
static void unl_make_run(const uint8_t *lit, uint8_t *out, int v, int run) {
    memcpy(out, lit, TS * TS);
    int top = TS, bx = 0, nb = 0;
    for (int x = 0; x < TS; x++) for (int y = 0; y < TS; y++) { uint8_t c = lit[x * TS + y]; if (c >= R_FLAME && c < R_FLAME + 16) { if (y < top) top = y; bx += x; nb++; } }
    if (!nb) return;
    bx /= nb;
    for (int x = 0; x < TS; x++)
        for (int y = 0; y < TS; y++) {
            uint8_t c = lit[x * TS + y];
            if (c < R_FLAME || c >= R_FLAME + 16) continue;
            int h = c - R_FLAME;
            int keep = (v == UL_GUTTER0 && h >= 12) || (v == UL_GUTTER1 && h >= 10);
            if (keep) { out[x * TS + y] = (uint8_t)(R_FLAME + h - (v == UL_GUTTER0 ? 7 : 5)); continue; }
            if (run == 3) { out[x * TS + y] = h >= 11 ? unl_shd(R_BLOOD, 0.22f + 0.06f * ((x + y) & 1)) : unl_shd(R_IRON, 0.05f + 0.04f * (h > 6)); continue; }
            int l = 0, r = 0;                                       /* held between metal? */
            for (int k = 1; k <= 6; k++) {
                if (x - k >= 0) { uint8_t q = lit[(x - k) * TS + y]; if (q != C_CLEAR && (q < R_FLAME || q >= R_FLAME + 16)) l = 1; }
                if (x + k < TS) { uint8_t q = lit[(x + k) * TS + y]; if (q != C_CLEAR && (q < R_FLAME || q >= R_FLAME + 16)) r = 1; }
            }
            out[x * TS + y] = l && r ? unl_shd(R_IRON, 0.10f + 0.08f * (h > 8) + 0.05f * ((x * 3 + y) % 4 == 0)) : C_CLEAR;
        }
    if (v == UL_SNUFFED) {
        int k = 0;
        for (int x = 0; x < TS && k < 3; x++) for (int y = 0; y < TS && k < 3; y++) { uint8_t c = lit[x * TS + y]; if (c >= R_FLAME + 13 && c < R_FLAME + 16 && ((x + y) % 3 == 0)) { out[x * TS + y] = (uint8_t)(R_FLAME + 5 + (k & 1) * 2); k++; } }
        for (int y = top - 14; y < top; y++) {                      /* the thread of smoke from where it burned */
            if (y < 1) continue;
            float u = (top - y) / 14.0f, cx = bx + 0.5f + 2.0f * sinf(y * 0.45f) * u;
            for (int x = bx - 4; x <= bx + 4; x++) {
                if (x < 0 || x >= TS || fabsf(x + 0.5f - cx) > 0.9f + u * 1.2f || ((x + y) & 1) == (u > 0.5f)) continue;
                out[x * TS + y] = unl_shd(R_STONE, 0.6f - 0.25f * u);
            }
        }
    }
}

/* each level's: from the run's lit frame, once crawl_art_run has made it (T_UNLIT + UL_*) */
static void crawl_art_unlit(int level) {
    int run = crawl_set(level);
    for (int v = UL_GUTTER0; v <= UL_COLD; v++) {
        if (run == 0) unl_make(crawl_tex[T_FEAT + F1_TORCH], crawl_tex[T_UNLIT + v], v);
        else unl_make_run(crawl_tex[T_FEAT + F1_TORCH], crawl_tex[T_UNLIT + v], v, run);
    }
}
