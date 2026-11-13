/* A part of crawl_art.c (docs/PLAN-tidy.md): it is #included there in its place and never compiled on its own.
 * The Wraith, the shade, the ember imp and its ember, the gargoyle. */

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
    int back = g_back;                                              /* 1.0.0b18: from behind, drawn unturned; spr_turn turns it */
    float up = pose == 0 ? -3.0f : 0.0f, hy = 3.0f + up;            /* the head's top row */
    float wl = pose == 0 ? -4.0f : (pose == 2 ? -1.0f : 2.0f);      /* the wings up, or down */
    cv_flat(32, 62, 12, 1.6f, R_WARM + 1);
    for (int pass = 0; pass < 2; pass++) {                          /* the wings: behind it, or from behind before it */
    if (pass != back) {
        if (!back || pass) break;
    } else
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
    if (pass == 0 && back) {                                        /* from behind, the body first, then the wings over it (below) */
    cv_capsule(32, hy + 33, 34, hy + 44, 1.2f, R_BRICK, 0.1f, 0.4f);   /* the tail, hanging between its legs, its ember */
    cv_capsule(34, hy + 44, 30, hy + 53, 1.0f, R_BRICK, 0.1f, 0.4f);
    cv_flat(30, hy + 54, 1.6f, 1.8f, (uint8_t)(R_FLAME + 11)); cv_px(30, (int)hy + 55, (uint8_t)(R_FLAME + 15));
    }
    if (pass == 0 && !back) {
    cv_capsule(35, hy + 33, 47, hy + 42, 1.2f, R_BRICK, 0.1f, 0.4f);   /* the tail, its ember */
    cv_capsule(47, hy + 42, 52, hy + 33, 1.0f, R_BRICK, 0.1f, 0.4f);
    cv_flat(52.5f, hy + 32, 1.6f, 1.8f, (uint8_t)(R_FLAME + 11)); cv_px(52, (int)hy + 31, (uint8_t)(R_FLAME + 15));
    }
    if (pass == 0) {
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
    if (back) {                                                     /* the back of its head: crust, horns from behind (the right broken), torn ears, the nape's fissure */
        for (int s = -1; s <= 1; s += 2) {
            cv_capsule(32 + s * 4.0f, hy + 4, 32 + s * (s > 0 ? 7.0f : 9.5f), hy + (s > 0 ? 0.0f : -3.0f), 1.6f, R_BONE, 0.2f, 0.65f);
            float e[6] = { 32 + s * 5.0f, hy + 6, 32 + s * 11.0f, hy + 4, 32 + s * 6.0f, hy + 10 };
            cv_poly(e, 3, R_BRICK, 0.08f, 0.35f, hy + 4, 6);
            cv_px(32 + s * 9, (int)hy + 5, C_CLEAR);
        }
        for (int y = (int)hy + 2; y <= (int)hy + 15; y++)
            for (int x = 25; x <= 39; x++) {
                float nx = (x + 0.5f - 32.0f) / 6.5f, ny = (y + 0.5f - hy - 8.5f) / 7.0f, d = nx * nx + ny * ny;
                if (d > 1.0f) continue;
                cv_px(x, y, sh(R_BRICK, 0.08f + 0.35f * lit(nx, ny, sqrtf(1.0f - d))));
            }
        for (int y = (int)hy + 9; y <= (int)hy + 33; y++) if ((y & 3) != 3) cv_px(32 + ((y >> 2) & 1), y, (uint8_t)(R_FLAME + 7 + (y % 3)));   /* the fissure down its spine */
    } else
        grid_stamp(IMP_HEAD, 17, 17, 24, (int)hy, R_BRICK, R_BONE, (uint8_t)(R_FLAME + 15), 0);
    }
    }
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
        for (int s = -1; s <= 1 && !g_back; s += 2) {              /* the wings, spread (swept back in a swoop) */
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
    if (g_back && !perch) {                                         /* 1.0.0b18, from behind: the back of its head, its spined back, then the wings over it */
        for (int y = (int)hy; y <= (int)hy + 15; y++)
            for (int x = 25; x <= 39; x++) {
                float nx = (x + 0.5f - 32.0f) / 6.2f, ny = (y + 0.5f - hy - 8.0f) / 7.5f, d = nx * nx + ny * ny;
                if (d <= 1.0f) cv_px(x, y, sh(R_STONE, 0.12f + 0.45f * lit(nx, ny, sqrtf(1.0f - d))));
            }
        for (int s = -1; s <= 1; s += 2) {                          /* the bull's horns from behind, the right snapped short; fin ears */
            cv_capsule(32 + s * 4.0f, hy + 4, 32 + s * 10.0f, hy + 1, 1.6f, R_STONE, 0.2f, 0.7f);
            if (s < 0) cv_capsule(22, hy + 1, 21, hy - 4, 1.2f, R_STONE, 0.25f, 0.75f);
            float e[6] = { 32 + s * 5.5f, hy + 7, 32 + s * 10.0f, hy + 9, 32 + s * 5.5f, hy + 11 };
            cv_poly(e, 3, R_STONE, 0.1f, 0.4f, hy + 7, 4);
        }
        for (int k = 0; k < 6; k++) {                               /* the ridge of spines down its back */
            int y = (int)hy + 18 + k * 3;
            cv_px(32, y, sh(R_STONE, 0.85f)); cv_px(32, y + 1, sh(R_STONE, 0.4f)); cv_px(31, y + 1, sh(R_STONE, 0.6f)); cv_px(33, y + 1, sh(R_STONE, 0.2f));
        }
        float tip = pose == 1 ? -4.0f : (pose == 2 ? 14.0f : 4.0f), out = swoop ? 22.0f : 30.0f;
        for (int s = -1; s <= 1; s += 2) {
            float tips[3][2] = { { 32 + s * out, hy + 6 + tip }, { 32 + s * (out - 4.0f), hy + 20 + tip * 0.5f }, { 32 + s * (out - 12.0f), hy + 28 } };
            garg_wing_front(s, 32 + s * 5.0f, hy + 21, 32 + s * (out - 10.0f), hy + tip * 0.6f, tips, 0.14f, 0.5f);   /* from their back, nearer */
        }
    } else garg_head(hy, !perch);
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

