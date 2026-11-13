/* A part of crawl_art.c (docs/PLAN-tidy.md): it is #included there in its place and never compiled on its own.
 * The Deep's materials, what stands on its walls, floors, ceilings, doors, keys and treasure. */

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

