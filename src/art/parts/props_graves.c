/* A part of crawl_art.c (docs/PLAN-tidy.md): it is #included there in its place and never compiled on its own.
 * Props: graves and what is carved on them; chains, manacles, the hook beam, the gibbet, the bronze bell. */

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

