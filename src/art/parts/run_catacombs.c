/* A part of crawl_art.c (docs/PLAN-tidy.md): it is #included there in its place and never compiled on its own.
 * The Catacombs' materials, what stands on their walls, floors and ceilings, doors, keys and treasure. */

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
