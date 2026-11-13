/* A part of crawl_art.c (docs/PLAN-tidy.md): it is #included there in its place and never compiled on its own.
 * The Hollows' materials, what stands on their walls, floors and ceilings, doors, keys and treasure. */

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

