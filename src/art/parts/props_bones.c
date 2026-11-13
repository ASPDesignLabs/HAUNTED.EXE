/* A part of crawl_art.c (docs/PLAN-tidy.md): it is #included there in its place and never compiled on its own.
 * Props: bones, litter, heaps and skull stacks; the totem post; candles, prickets, the corona, hearths, braziers, lanterns. */

/* ---- The props (docs/InitialArtPass.md, section 14): drawn with each level, so that its age and run II's water reach
 * them; the common ones have several looks (crawl_alt), picked where they are drawn by where each stands ---- */
int16_t crawl_alt[NSPR][NALT];

/* the prop skull: the master skull cut down to 11 by 12, for piles, heaps and totems (hex R_BONE, 0 the hollows) */
static const char *const PROP_SKULL[12] = {
    "...7abba8..", ".7ceffedc8.", "8dfffeeddb7", "7cdeeeddcb6", "6aeedbdeda5", "8000bc70005",
    "a1009081005", "6dcb020bba5", ".9b60706a8.", "..8e3f3d6..", "..4000003..", "...7cbba5..",
};
/* a prop skull with its top left at (x0, y0): mirror flips it, jaw 0 leaves its jaw off, dim darkens it (deeper in a pile),
 * glow fills its sockets with a candle's light inside (a step of the fire, 0 none) */
static void prop_skull(int x0, int y0, int mirror, int jaw, int dim, int glow) {
    for (int y = 0; y < (jaw ? 12 : 10); y++)
        for (int x = 0; x < 11; x++) {
            int gx = mirror ? 10 - x : x;
            char ch = PROP_SKULL[y][gx];
            if (ch == '.') continue;
            if (ch == '0') {
                int sock = (y == 5 || y == 6) && ((gx >= 1 && gx <= 3) || (gx >= 7 && gx <= 9));
                cv_px(x0 + x, y0 + y, glow && sock ? (uint8_t)(R_FLAME + glow - (y == 5)) : 0);
                continue;
            }
            int v = ch <= '9' ? ch - '0' : ch - 'a' + 10;
            cv_px(x0 + x, y0 + y, (uint8_t)(R_BONE + clampi(v - dim, 1, 15)));
        }
}
/* a long bone from (x0, y0) to (x1, y1), knuckled as the skeletons' bones are: a femur's ball at its head (femur), or
 * snapped off at the far end, the marrow dark in it (broken); old stains down it; dim darkens it */
static void prop_long(float x0, float y0, float x1, float y1, float r, int femur, int broken, float dim, uint32_t seed) {
    float L = hypotf(x1 - x0, y1 - y0) + 0.01f, ux = (x1 - x0) / L, uy = (y1 - y0) / L, px = -uy, py = ux;
    float lo = fmaxf(0.04f, 0.24f - dim * 0.5f), hi = 0.92f - dim;
    if (broken) {
        cv_capsule(x0, y0, x1, y1, r * 0.85f, R_BONE, lo, hi);
        for (int e = -1; e <= 1; e += 2) cv_ellipse(x0 + px * r * 0.7f * (float)e, y0 + py * r * 0.7f * (float)e, r * 1.05f, r * 1.05f, R_BONE, lo, hi);
        for (int k = -1; k <= 1; k++)                               /* splintered */
            cv_px((int)(x1 + px * (float)k * r * 0.7f + ux * (k == 0 ? 1.4f : 0.3f)), (int)(y1 + py * (float)k * r * 0.7f + uy * (k == 0 ? 1.4f : 0.3f)), sh(R_BONE, hi * 0.75f));
        cv_px((int)x1, (int)y1, sh(R_WARM, 0.08f));
    } else cv_bone(x0, y0, x1, y1, r, lo, hi);
    if (femur) {                                                    /* its head, on a neck off to one side */
        cv_capsule(x0, y0, x0 - ux * r * 0.8f + px * r * 1.4f, y0 - uy * r * 0.8f + py * r * 1.4f, r * 0.7f, R_BONE, lo, hi);
        cv_ellipse(x0 - ux * r * 1.1f + px * r * 1.9f, y0 - uy * r * 1.1f + py * r * 1.9f, r * 1.2f, r * 1.2f, R_BONE, lo + 0.06f, hi);
    }
    for (int k = 0; k < 3; k++) {                                   /* old stains */
        float t = 0.2f + 0.6f * h01(k, 1, seed);
        cv_px((int)(x0 + (x1 - x0) * t), (int)(y0 + (y1 - y0) * t), sh(R_BONE, lo + 0.1f));
    }
}
/* a rib: a thin arc about (cx, cy) from angle a0 to a1, lit along its top, darker as it goes */
static void prop_rib(float cx, float cy, float rx, float ry, float a0, float a1, float dim) {
    int n = (int)(fabsf(a1 - a0) * fmaxf(rx, ry)) + 2;
    for (int i = 0; i <= n; i++) {
        float a = a0 + (a1 - a0) * (float)i / (float)n, x = cx + cosf(a) * rx, y = cy + sinf(a) * ry;
        cv_px((int)x, (int)y, sh(R_BONE, 0.82f - dim - 0.25f * (float)i / (float)n)); cv_px((int)x, (int)y + 1, sh(R_BONE, fmaxf(0.05f, 0.35f - dim)));
    }
}
static void prop_vertebra(float x, float y, float dim) {
    cv_ellipse(x, y, 2.0f, 1.4f, R_BONE, fmaxf(0.05f, 0.3f - dim), 0.85f - dim);
    cv_px((int)x, (int)y - 2, sh(R_BONE, 0.72f - dim)); cv_px((int)x - 2, (int)y - 1, sh(R_BONE, 0.6f - dim)); cv_px((int)x + 2, (int)y - 1, sh(R_BONE, 0.5f - dim));
    cv_px((int)x, (int)y, sh(R_WARM, 0.1f));                        /* the hole for the cord */
}
/* a jaw fallen from its skull, lying open: a U of bone, the teeth along it */
static void prop_jaw(float x, float y) {
    for (int i = -5; i <= 5; i++) {
        float yy = y - 0.11f * (float)(25 - i * i);
        cv_px((int)x + i, (int)yy, sh(R_BONE, 0.78f - 0.04f * (float)(i + 5))); cv_px((int)x + i, (int)yy + 1, sh(R_BONE, 0.35f));
        if (abs(i) < 4 && (i & 1)) cv_px((int)x + i, (int)yy - 1, sh(R_BONE, 0.9f));   /* teeth, one gone */
    }
    cv_px((int)x - 5, (int)y - 3, sh(R_BONE, 0.7f)); cv_px((int)x + 5, (int)y - 3, sh(R_BONE, 0.5f));   /* its ends, rising */
}
static void prop_fingers(float x, float y, uint32_t seed) {
    for (int k = 0; k < 4; k++) {
        float a = h01(k, 1, seed) * PI_F, l = 2.0f + h01(k, 2, seed) * 1.5f, bx = x + (float)k * 2.2f, by = y + h01(k, 3, seed) * 2.0f;
        cv_line(bx, by, bx + cosf(a) * l, by + sinf(a) * l * 0.4f, sh(R_BONE, 0.7f)); cv_px((int)bx, (int)by, sh(R_BONE, 0.85f));
    }
}
/* the level's age on a prop: stains in the bone, and moss low down once its decay is far on */
static void prop_age(uint32_t seed) {
    for (int y = 0; y < TS; y++)
        for (int x = 0; x < TS; x++) {
            uint8_t *c = &cv_sprite[y * TS + x];
            if (*c <= R_BONE + 2 || *c >= R_BONE + 16) continue;
            if (h01(x, y, seed) < g_decay * 0.14f) *c = (uint8_t)(*c - 2);
            else if (g_decay > 0.45f && y > 46 && fbm((float)x / 4.0f, (float)y / 3.0f, 16, seed + 1) > 1.18f - g_decay * 0.45f) *c = sh(R_MOSS, 0.2f + 0.25f * h01(x, y, seed + 2));
        }
}
/* the run's water on a finished prop (docs/InitialArtPass.md section 14, B3): what stands gets a silt line, darker and
 * duller below it, weed hanging at it; iron rusts; everything drips under its overhangs; a wet sheen low down. Lights'
 * flames and wax are left dry; rust 0 spares iron (the Black Altar is black stone in the iron ramp). Over crawl_spr[id] as
 * stored, column-major */
static void prop_wash_ex(int id, int hangs, int rust, uint32_t seed) {
    uint8_t *sp = crawl_spr[id];
    int top = TS, foot = -1;
    for (int x = 0; x < TS; x++) for (int y = 0; y < TS; y++) if (sp[x * TS + y] != C_CLEAR) { if (y < top) top = y; if (y > foot) foot = y; }
    if (foot < 0) return;
    int line = foot - (int)((float)(foot - top) * 0.28f) - 1;
    #define WP(x, y) sp[(x) * TS + (y)]
    #define WON(x, y) ((unsigned)(x) < (unsigned)TS && (unsigned)(y) < (unsigned)TS && WP(x, y) != C_CLEAR)
    for (int x = 0; x < TS; x++)
        for (int y = 0; y < TS; y++) {
            uint8_t c = WP(x, y);
            if (c == C_CLEAR || c == 0 || IS_FULLBRIGHT(c) || (c >= R_WARM && c <= R_WARM + 1)) continue;
            int ramp = (c - 1) / 16 * 16 + 1, v = c - ramp;
            if (rust && ramp == R_IRON && h01(x, y, seed) < 0.35f) { WP(x, y) = sh(R_BRICK, 0.15f + 0.04f * (float)v); continue; }   /* rust */
            if (!hangs && y > line) {                               /* below the silt line: dulled, a silt bloom */
                WP(x, y) = h01(x, y, seed + 1) < 0.55f ? sh(R_WARM, 0.12f + 0.025f * (float)v) : (uint8_t)(ramp + clampi(v - 3, 1, 15));
                if (y < foot - 1 && !WON(x - 1, y - 1) && h01(x, y, seed + 2) < 0.4f) WP(x, y) = sh(R_GHOST, 0.42f);   /* a wet sheen */
            } else if (!hangs && y == line) WP(x, y) = sh(R_WARM, 0.55f);   /* the tide line */
        }
    if (!hangs)
        for (int x = 0; x < TS; x++)                                /* weed hanging at the line */
            if (WON(x, line) && h01(x, 7, seed + 3) < 0.18f)
                for (int k = 1; k <= 2 + (int)(h01(x, 8, seed + 3) * 4.0f); k++) if (WON(x, line + k)) WP(x, line + k) = sh(R_MOSS, 0.32f - 0.04f * (float)k);
    for (int x = 0; x < TS; x++)                                    /* drips under the overhangs */
        for (int y = top; y < TS - 3; y++)
            if (WON(x, y) && !WON(x, y + 1) && !IS_FULLBRIGHT(WP(x, y)) && h01(x, y, seed + 4) < 0.07f) {
                WP(x, y + 1) = sh(R_GHOST, 0.62f);
                if (!WON(x, y + 3) && h01(x, y, seed + 5) < 0.5f) WP(x, y + 3) = sh(R_GHOST, 0.75f);
            }
    #undef WP
    #undef WON
}
static void prop_wash(int id, int hangs, uint32_t seed) { prop_wash_ex(id, hangs, 1, seed); }

/* P1: bones and skulls. Litter by a hall's wall: a femur across a snapped shin, ribs, a jaw; a ribcage open on its spine and
 * a skull; two femurs crossed under a skull, the Order's memento mori. look 0-2 */
static void spr_bonelitter(int id, int look) {
    cv_begin(cv_sprite, TS, TS);
    cv_flat(32, 61.5f, 21, 2.4f, R_WARM + 1);
    if (look == 0) {
        prop_long(30, 61, 52, 54, 1.6f, 0, 1, 0.12f, 13);           /* a shin, snapped */
        prop_long(10, 55, 40, 60, 2.0f, 1, 0, 0.0f, 11);            /* a femur across it */
        for (int k = 0; k < 2; k++) prop_rib(22.0f + (float)k * 5.0f, 63.0f, 6.0f, 7.0f, PI_F * 1.1f, PI_F * 1.75f, 0.0f);
        prop_jaw(51, 62); prop_vertebra(44, 61, 0.05f);
    } else if (look == 1) {
        for (int k = 0; k < 4; k++) prop_rib(17.0f + (float)k * 4.5f, 60.0f, 4.5f, 8.0f - (float)k * 0.8f, PI_F * 1.02f, PI_F * 1.98f, 0.04f * (float)k);   /* a ribcage open on its back */
        for (int k = 0; k < 6; k++) prop_vertebra(12.0f + (float)k * 4.0f, 60.0f, 0.04f * (float)k);   /* its spine */
        prop_skull(37, 49, 1, 1, 0, 0);
        prop_long(48, 61, 58, 58, 1.3f, 0, 1, 0.15f, 17);
    } else {
        prop_long(17, 60, 46, 50, 1.5f, 1, 0, 0.12f, 19);
        prop_long(17, 50, 46, 60, 1.5f, 1, 0, 0.0f, 21);
        prop_skull(26, 45, 0, 1, 0, 0);
        prop_vertebra(52, 61, 0.1f); prop_fingers(8, 60, 23);
    }
    prop_age(25 + (uint32_t)look);
    cv_outline(0);
    cv_store(id);
}

/* a charnel heap: a mound of long bones laid every way over the dark between them, skulls sunk in it, one crowning it; look
 * 1 has a ribcage arching out of its side and a skull rolled off at its foot */
static void spr_boneheap(int id, int look) {
    cv_begin(cv_sprite, TS, TS);
    cv_flat(32, 61.5f, 28, 3.5f, R_WARM + 1);
    for (int y = 34; y < 62; y++)                                   /* the dark of the heap, between the bones */
        for (int x = 4; x < 60; x++) {
            float dx = ((float)x + 0.5f - 32.0f) / 27.0f, h = 61.0f - 25.0f * (1.0f - dx * dx) * (0.85f + 0.15f * h01(x / 3, look, 27));
            if ((float)y < h) continue;
            cv_px(x, y, h01(x, y, 29) < 0.3f ? sh(R_BONE, 0.14f) : sh(R_WARM, 0.08f));
        }
    for (int layer = 0; layer < 3; layer++)                         /* the bones, the far ones dim */
        for (int i = 0; i < 9; i++) {
            uint32_t sd = 31u + (uint32_t)(layer * 17 + i + look * 101);
            float cx = 8.0f + 48.0f * h01(i, 1, sd), dx = (cx - 32.0f) / 27.0f, top = 61.0f - 25.0f * (1.0f - dx * dx);
            float cy = top + 3.0f + (60.0f - top - 3.0f) * h01(i, 2, sd), a = (h01(i, 3, sd) - 0.5f) * 2.4f, l = 6.0f + 6.0f * h01(i, 4, sd);
            if (cy > 60.5f) cy = 60.5f;
            prop_long(cx - cosf(a) * l, cy - sinf(a) * l, cx + cosf(a) * l, cy + sinf(a) * l, 1.25f, h01(i, 5, sd) < 0.3f, h01(i, 6, sd) < 0.25f, 0.32f - 0.14f * (float)layer, sd);
        }
    prop_skull(9, 47, 1, 0, 3, 0);                                  /* skulls sunk in it */
    prop_skull(41, 48, 0, 0, 2, 0);
    if (look == 1) {
        for (int k = 0; k < 4; k++) prop_rib(50.0f, 50.0f + (float)k * 2.5f, 7.0f, 4.0f, PI_F * 1.15f, PI_F * 1.95f, 0.05f * (float)k);   /* a ribcage arching out */
        prop_skull(2, 51, 1, 1, 0, 0);                              /* one rolled off */
    }
    prop_skull(26, 28, look, 1, 0, 0);                              /* and one crowning it */
    prop_age(33 + (uint32_t)look);
    cv_outline(0);
    cv_store(id);
}

/* skulls as an ossuary keeps them. look 0: stacked in courses against the wall, femurs' ends between, stepping in to one on
 * top; 1: a loose pyramid, some jawless, one rolled off, a cross painted on one; 2: a pile with a candle burnt out on
 * it, its wax run down over them */
static void femur_ends(int x0, int x1, int y) {                    /* a course of femurs laid end out: knuckles in two rows, bonded */
    for (int r = 0; r < 2; r++)
        for (int x = x0 + (r ? 2 : 0); x <= x1; x += 4) {
            cv_ellipse((float)x + 1.5f, (float)y + 1.2f + (float)r * 2.6f, 1.9f, 1.5f, R_BONE, 0.2f, 0.78f - 0.06f * (float)r);
            cv_px(x + 1, y + 1 + r * 3, sh(R_BONE, 0.28f));
        }
}
static void spr_skullstack(int id, int look) {
    cv_begin(cv_sprite, TS, TS);
    cv_flat(32, 62, 28, 2.5f, R_WARM + 1);
    if (look == 0) {
        femur_ends(5, 57, 56);
        for (int k = 0; k < 5; k++) prop_skull(5 + k * 11, 44, k & 1, k != 3, (k * 3) % 2, 0);
        femur_ends(9, 53, 39);
        for (int k = 0; k < 4; k++) prop_skull(10 + k * 11, 27, (k + 1) & 1, k != 0, 1, 0);
        femur_ends(15, 47, 22);
        prop_skull(26, 10, 0, 1, 0, 0);
        for (int k = 0; k < 5; k++) cv_px(37, 28 + k, sh(R_BLOOD, 0.4f - 0.03f * (float)k));   /* a cross painted on one's brow, in red ochre */
        for (int k = -2; k <= 2; k++) cv_px(37 + k, 30, sh(R_BLOOD, 0.4f));
    } else {
        static const int ROW[4][5] = { { 4, 15, 26, 37, 48 }, { 9, 20, 31, 42, -1 }, { 15, 26, 37, -1, -1 }, { 21, 32, -1, -1, -1 } };
        for (int r = 0; r < 4; r++)
            for (int k = 0; k < 5 && ROW[r][k] >= 0; k++) {
                int x = ROW[r][k] + (int)(h01(r, k, 43 + (uint32_t)look) * 3.0f) - 1, y = 50 - r * 10 + (int)(h01(k, r, 45) * 2.0f);
                prop_skull(x, y, (r + k + look) & 1, r == 0 || h01(r, k, 47) < 0.5f, r == 0 ? 0 : 1 + (k & 1), 0);
            }
        if (look == 1) {
            prop_skull(51, 51, 1, 1, 0, 0);                         /* rolled off */
            for (int k = 0; k < 4; k++) cv_px(27, 31 + k, sh(R_BLOOD, 0.42f));   /* a cross painted on one's brow, in red ochre */
            for (int k = -1; k <= 1; k++) cv_px(27 + k, 32, sh(R_BLOOD, 0.42f));
        } else {
            for (int y = 9; y < 21; y++) for (int x = 31; x < 34; x++) cv_px(x, y, sh(R_BONE, 0.55f + 0.2f * (x == 31) - 0.01f * (float)(y - 9)));   /* a candle burnt low */
            cv_px(32, 8, 0); cv_px(32, 7, sh(R_IRON, 0.3f));                                            /* its wick, out */
            for (int k = 0; k < 7; k++) {                           /* its wax run down over the skulls */
                int x = 27 + (int)(h01(k, 1, 49) * 11.0f), l = 4 + (int)(h01(k, 2, 49) * 14.0f);
                for (int j = 0; j < l; j++) if (cv_sprite[(20 + j) * TS + x] != C_CLEAR) cv_px(x, 20 + j, sh(R_BONE, 0.82f - 0.02f * (float)j));
            }
            for (int x = 28; x < 37; x++) cv_px(x, 21, sh(R_BONE, 0.78f));
        }
    }
    prop_age(51 + (uint32_t)look);
    cv_outline(0);
    cv_store(id);
}

/* a candle at (x, by), h tall, its flame in a frame; wax run down it */
static void prop_candle(float x, float by, float h, int frame, int seed) {
    for (int y = (int)(by - h); y <= (int)by; y++) { cv_px((int)x, y, sh(R_BONE, 0.72f)); cv_px((int)x + 1, y, sh(R_BONE, 0.48f)); }
    cv_px((int)x - 1, (int)(by - h) + 2 + (seed & 1), sh(R_BONE, 0.8f)); cv_px((int)x + 2, (int)(by - h) + 3, sh(R_BONE, 0.6f));   /* runs of wax */
    flame(x + 1.0f, by - h - 1.0f, 5.5f + (float)((frame + seed) & 1), 1.7f, (frame ? 0.7f : -0.6f) * (seed & 1 ? 1.0f : -1.0f));
}
/* the totem: a black post bound in iron and rope, skulls threaded on it with a candle's light inside each, a crossbar of iron
 * with candles at its ends and one on top, their wax run down. look 1: a great skull on top crowned with candles, two
 * smaller ones hung below it on chains, a strip of the Order's banner nailed to the post. frame 0-1, the flames */
static void spr_totempost(int id, int look, int frame) {
    cv_begin(cv_sprite, TS, TS);
    float cx = 32;
    cv_flat(cx, 62, 10, 1.7f, R_WARM + 1);
    cv_capsule(cx, look ? 20.0f : 14.0f, cx, 62, 2.4f, R_WOOD, 0.06f, 0.42f);   /* the post, black with age */
    for (int b = 0; b < 3; b++) {                                   /* iron bands, and rope lashed below them */
        int y = 58 - b * (look ? 13 : 15);
        for (int x = (int)cx - 3; x <= (int)cx + 3; x++) cv_px(x, y, sh(R_IRON, 0.55f - 0.07f * (float)abs(x - (int)cx + 1)));
        cv_px((int)cx + 1, y, sh(R_IRON, 0.85f));
        for (int k = 1; k <= 2; k++) for (int x = (int)cx - 2; x <= (int)cx + 2; x++) cv_px(x, y + k, ((x + k) & 1) ? sh(R_WARM, 0.55f) : sh(R_WARM, 0.32f));
    }
    if (!look) {
        for (int k = 0; k < 3; k++) prop_skull((int)cx - 5, 47 - k * 12, k & 1, k == 0, k, 4 + ((frame + k) & 1));   /* threaded on it, lit inside */
        cv_capsule(cx - 13, 23, cx + 13, 23, 1.1f, R_IRON, 0.15f, 0.6f);   /* the crossbar */
        for (int s = -1; s <= 1; s += 2) prop_candle(cx + (float)s * 12.0f, 22, 6, frame, s > 0);
        prop_candle(cx - 0.5f, 13, 6, frame, 2);
        for (int k = 0; k < 5; k++) {                               /* wax run down the skulls from above */
            int x = (int)cx - 3 + (int)(h01(k, 1, 53) * 7.0f), y0 = 24, l = 3 + (int)(h01(k, 2, 53) * 8.0f);
            for (int j = 0; j < l; j++) if (cv_sprite[(y0 + j) * TS + x] != C_CLEAR) cv_px(x, y0 + j, sh(R_BONE, 0.8f - 0.02f * (float)j));
        }
    } else {
        for (int s = -1; s <= 1; s += 2) {                          /* two hung on chains from a bar */
            float hx = cx + (float)s * 10.0f;
            for (int y = 25; y < 33; y++) cv_px((int)hx, y, sh(R_IRON, (y & 1) ? 0.6f : 0.25f));
            prop_skull((int)hx - 5, 32, s > 0, 0, 1, 0);
        }
        cv_capsule(cx - 11, 24, cx + 11, 24, 1.0f, R_IRON, 0.15f, 0.55f);
        for (int y = 30; y < 44; y++)                               /* a strip of the Order's banner, nailed on and torn */
            for (int x = (int)cx - 2; x <= (int)cx + 3; x++) {
                if (y > 40 && h01(x, y, 55) < (float)(y - 40) / 4.0f) continue;
                cv_px(x, y, sh(R_BLOOD, 0.2f + 0.08f * (float)(x < (int)cx + 1)));
            }
        cv_px((int)cx, 31, sh(R_IRON, 0.8f)); cv_px((int)cx + 1, 35, (uint8_t)(R_GOLD + 5)); cv_px((int)cx + 1, 36, (uint8_t)(R_GOLD + 4)); cv_px((int)cx, 36, (uint8_t)(R_GOLD + 4)); cv_px((int)cx + 2, 36, (uint8_t)(R_GOLD + 3)); cv_px((int)cx + 1, 37, (uint8_t)(R_GOLD + 4));
        skull_stamp(cx, 13, 0, 0, 0);                               /* the great skull on top */
        for (int y = 9; y < 14; y++) for (int x = (int)cx - 4; x <= (int)cx + 4; x++) {   /* candlelight in its sockets */
            uint8_t c = cv_sprite[y * TS + x];
            if (c == 0) cv_px(x, y, (uint8_t)(R_FLAME + 3 + ((frame + x) & 1)));
        }
        for (int k = -1; k <= 1; k++) prop_candle(cx + (float)k * 4.5f - 0.5f, 6.0f + (float)abs(k), 4 + (k == 0), frame, k + 1);   /* candles in its crown */
        for (int k = 0; k < 6; k++) {                               /* their wax run down its face */
            int x = (int)cx - 6 + (int)(h01(k, 1, 57) * 12.0f), l = 2 + (int)(h01(k, 2, 57) * 9.0f);
            for (int j = 0; j < l; j++) if (cv_sprite[(7 + j) * TS + x] != C_CLEAR && cv_sprite[(7 + j) * TS + x] != 0) cv_px(x, 7 + j, sh(R_BONE, 0.85f - 0.02f * (float)j));
        }
    }
    prop_age(59 + (uint32_t)look);
    cv_outline(0);
    cv_store(id);
}

/* P2: the lights. A flame with a white heart; the smoke over it is smoke_up's, after the outline */
static void prop_flame(float x, float by, float h, float w, float sway) {
    flame(x, by, h, w, sway);
    cv_px((int)x, (int)(by - h * 0.35f), (uint8_t)(R_FLAME + 15)); cv_px((int)x, (int)(by - h * 0.25f), (uint8_t)C_WHITE);
}
/* a thread of smoke going up from (x, y), grey and thinning; drawn after the outline, only where nothing is */
static void smoke_up(float x, float y, float h, uint32_t seed) {
    for (int k = 0; k < (int)h; k++) {
        float t = (float)k / h;
        int px = (int)(x + sinf(t * 6.0f + (float)(seed & 7)) * (0.5f + 2.5f * t)), py = (int)(y - (float)k);
        if ((unsigned)px >= (unsigned)TS || (unsigned)py >= (unsigned)TS || cv_sprite[py * TS + px] != C_CLEAR || h01(px, py, seed) < t * 0.8f) continue;
        cv_px(px, py, sh(R_IRON, 0.5f - 0.25f * t));
    }
}
/* a church candle standing at (x, by), h tall and r round: its wax lit from the left, its rim melted into a cup, wax run down
 * its front in drops; lit: its flame (frame and k vary it), gutter: low and leaning */
static void wax_candle(float x, float by, float h, float r, int lit_, int frame, int k, int gutter) {
    for (int y = (int)(by - h); y <= (int)by; y++)
        for (int xx = (int)(x - r) - 1; xx <= (int)(x + r) + 1; xx++) {
            float nx = ((float)xx + 0.5f - x) / r;
            if (fabsf(nx) > 1.0f) continue;
            cv_px(xx, y, sh(R_BONE, 0.42f + 0.48f * lit(nx, 0.0f, sqrtf(1.0f - nx * nx)) + (y == (int)(by - h) ? 0.08f : 0.0f)));
        }
    int top = (int)(by - h);
    cv_px((int)x, top, sh(R_BONE, 0.3f)); cv_px((int)x + 1, top, sh(R_BONE, 0.35f));   /* the melted cup */
    for (int d = 0; d < 2; d++) {                                   /* runs of wax down its front */
        int dx = (int)x - 1 + d * 2 + ((k + d) & 1), l = 2 + (int)(h01(k, d, 701) * (h * 0.6f));
        for (int j = 1; j <= l; j++) cv_px(dx, top + j, sh(R_BONE, 0.95f - 0.02f * (float)j));
        cv_px(dx, top + l + 1, sh(R_BONE, 0.7f)); cv_px(dx + 1, top + l + 1, sh(R_BONE, 0.3f));   /* its drop, and its shadow */
    }
    cv_px((int)x, top - 1, sh(R_IRON, 0.15f));                      /* the wick */
    if (lit_) prop_flame(x + 0.5f, (float)top - 1.5f, gutter ? 3.5f : 5.5f + (float)((frame + k) & 1), gutter ? 1.3f : 1.8f, gutter ? 1.6f : (((frame + k) % 3) - 1) * 0.7f);
}
/* candles: three looks. 0: church candles of three heights standing in their own pooled wax, one guttering, a stub gone out;
 * 1: two set on a skull's crown, their wax run over its brow; 2: an iron pricket dish, one candle fallen and out in the wax */
static void spr_candlecluster(int id, int look) {
    cv_begin(cv_sprite, TS, TS);
    cv_flat(32, 62, 17, 2.0f, R_WARM + 1);
    if (look == 2) {                                                /* the iron dish on its feet */
        for (int s = -1; s <= 1; s += 2) cv_capsule(32.0f + (float)s * 10.0f, 59.0f, 32.0f + (float)s * 12.0f, 62.0f, 0.9f, R_IRON, 0.2f, 0.6f);
        cv_ellipse(32, 58, 15.0f, 3.2f, R_IRON, 0.15f, 0.62f);
        for (int x = 18; x <= 46; x += 4) cv_px(x, 56, sh(R_IRON, 0.75f));   /* its rim's beading */
    }
    float py = look == 2 ? 57.5f : 60.5f;
    for (int y = (int)py - 2; y <= (int)py + 2; y++)                /* the pool of wax */
        for (int x = 16; x <= 48; x++) {
            float nx = ((float)x + 0.5f - 32.0f) / 15.0f, ny = ((float)y + 0.5f - py) / 2.2f;
            if (nx * nx + ny * ny + 0.25f * sinf((float)x * 0.9f) > 1.0f) continue;
            cv_px(x, y, sh(R_BONE, 0.5f + 0.25f * (ny < 0) + 0.08f * h01(x, y, 703)));
        }
    if (look == 0) {
        wax_candle(33, 60, 24, 2.8f, 1, 0, 1, 0);
        wax_candle(24, 61, 15, 2.4f, 1, 0, 2, 0);
        wax_candle(42, 61, 10, 2.4f, 1, 0, 3, 1);                   /* guttering */
        wax_candle(29, 62, 4, 1.8f, 0, 0, 4, 0);                    /* a stub, out */
    } else if (look == 1) {
        prop_skull(26, 47, 0, 1, 0, 0);
        wax_candle(29, 47, 11, 2.0f, 1, 0, 5, 0);
        wax_candle(35, 48, 7, 2.0f, 1, 0, 6, 0);
        for (int k = 0; k < 6; k++) {                               /* their wax run over its brow, one into a socket */
            int x = 27 + (int)(h01(k, 1, 705) * 9.0f), l = 2 + (int)(h01(k, 2, 705) * 6.0f) + (k == 2 ? 3 : 0);
            for (int j = 0; j < l; j++) if (cv_sprite[(48 + j) * TS + x] != C_CLEAR) cv_px(x, 48 + j, sh(R_BONE, 0.92f - 0.02f * (float)j));
        }
    } else {
        wax_candle(25, 57, 14, 2.3f, 1, 0, 7, 0);
        wax_candle(34, 57, 7, 2.3f, 1, 0, 8, 1);
        wax_candle(41, 57, 18, 2.3f, 1, 0, 9, 0);
        for (int x = 30; x <= 44; x++) { cv_px(x, 60, sh(R_BONE, 0.7f - 0.01f * (float)(x - 30))); cv_px(x, 61, sh(R_BONE, 0.38f)); }   /* one fallen, out */
        cv_px(45, 60, sh(R_IRON, 0.15f));
    }
    cv_outline(0);
    if (look == 0) smoke_up(43.5f, 47.0f, 10.0f, 707);              /* the guttering one smokes */
    if (look == 2) smoke_up(35.5f, 46.0f, 9.0f, 709);
    cv_store(id);
}

/* the candle stand (run I's own, the halls' light on its first levels): a twisted iron shaft with knops on a tripod of
 * claw feet, a tray with a tracery rim, three candles on its prickets, wax hanging from it in stalactites */
static void spr_prickstand(int id, int frame) {
    cv_begin(cv_sprite, TS, TS);
    cv_flat(32, 62, 11, 1.6f, R_WARM + 1);
    for (int s = -1; s <= 1; s++) {                                 /* the feet, curving out, clawed */
        float fx = 32.0f + (float)s * 9.0f;
        cv_capsule(32, 53, (32.0f + fx) * 0.5f, 59.5f - (float)abs(s) * 0.5f, 1.1f, R_IRON, 0.15f, 0.65f);
        cv_capsule((32.0f + fx) * 0.5f, 59.5f, fx, 62.0f - (s == 0 ? 1.0f : 0.0f), 0.9f, R_IRON, 0.15f, 0.6f);
        for (int t = -1; t <= 1; t++) cv_px((int)fx + t + s, 62, sh(R_IRON, 0.5f));
    }
    for (int y = 31; y < 54; y++) cv_px(32, y, sh(R_IRON, ((y / 2) & 1) ? 0.75f : 0.3f)), cv_px(31, y, sh(R_IRON, ((y / 2) & 1) ? 0.35f : 0.55f)), cv_px(33, y, sh(R_IRON, 0.18f));   /* twisted */
    cv_ellipse(32, 50, 2.6f, 1.6f, R_IRON, 0.2f, 0.8f); cv_ellipse(32, 41, 2.2f, 1.4f, R_IRON, 0.2f, 0.8f);   /* knops */
    cv_ellipse(32, 30, 10.5f, 2.4f, R_IRON, 0.12f, 0.7f);           /* the tray */
    for (int x = 23; x <= 41; x += 3) { cv_px(x, 27, sh(R_IRON, 0.6f)); cv_px(x - 1, 28, sh(R_IRON, 0.45f)); cv_px(x + 1, 28, sh(R_IRON, 0.45f)); }   /* its tracery rim */
    for (int k = 0; k < 6; k++) {                                   /* wax hung from it */
        int x = 24 + (int)(h01(k, 1, 711) * 16.0f), l = 2 + (int)(h01(k, 2, 711) * 5.0f);
        for (int j = 1; j <= l; j++) cv_px(x, 32 + j, sh(R_BONE, 0.85f - 0.06f * (float)j));
    }
    wax_candle(26, 28, 8, 1.8f, 1, frame, 0, 0); wax_candle(32, 28, 12, 2.0f, 1, frame, 1, 0); wax_candle(38, 28, 9, 1.8f, 1, frame, 2, 0);
    cv_outline(0);
    cv_store(id);
}

/* the chandelier: an iron corona hung on chains from a hook, fleurons along its rim, candles on prickets round it (those
 * behind dimmer, drawn first), wax hanging from it in stalactites, and a skull hung on a chain at its middle */
static void spr_corona(int id, int frame) {
    cv_begin(cv_sprite, TS, TS);
    float cx = 32, ry = 24, rx = 21;
    chain_down(cx, 0, 7);
    cv_ellipse(cx, 8, 1.6f, 1.6f, R_IRON, 0.2f, 0.8f);              /* the hook's ring */
    for (int k = 0; k < 4; k++) {                                   /* chains down to the ring, the back two dim */
        float ang = PI_F * 0.25f + (float)k * PI_F * 0.5f, x = cx + rx * cosf(ang) * 0.95f, y = ry + 3.5f * sinf(ang);
        if (sinf(ang) < 0.0f) for (int i = 0; i <= 16; i++) { float t = (float)i / 16.0f; cv_px((int)(cx + (x - cx) * t), (int)(9.0f + (y - 9.0f) * t), sh(R_IRON, (i & 1) ? 0.3f : 0.15f)); }
    }
    for (int k = 0; k < 9; k++) {                                   /* candles behind */
        float ang = TAU_F * (float)k / 9.0f + 0.2f, sz = sinf(ang);
        if (sz >= -0.1f) continue;
        wax_candle(cx + rx * cosf(ang), ry + 3.5f * sz - 1.0f, 5.0f + 3.0f * h01(k, 1, 713), 1.2f, 1, frame, k, 0);
    }
    for (int a = 0; a < 90; a++) {                                  /* the ring: a band, fleurons on its rim */
        float ang = TAU_F * (float)a / 90.0f, sz = sinf(ang);
        int x = (int)(cx + rx * cosf(ang)), y = (int)(ry + 3.5f * sz);
        cv_px(x, y, sh(R_IRON, sz > 0 ? 0.35f + 0.45f * sz : 0.16f)); cv_px(x, y + 1, sh(R_IRON, sz > 0 ? 0.22f : 0.1f));
        if (sz > 0) cv_px(x, y + 2, sh(R_IRON, 0.12f));
        if (a % 9 == 4) { cv_px(x, y - 1, sh(R_IRON, 0.55f + 0.3f * fmaxf(0.0f, sz))); cv_px(x, y - 2, sh(R_IRON, 0.45f)); cv_px(x - 1, y - 1, sh(R_IRON, 0.35f)); cv_px(x + 1, y - 1, sh(R_IRON, 0.35f)); }   /* a fleuron */
    }
    for (int k = 0; k < 4; k++) {                                   /* chains in front */
        float ang = PI_F * 0.25f + (float)k * PI_F * 0.5f, x = cx + rx * cosf(ang) * 0.95f, y = ry + 3.5f * sinf(ang);
        if (sinf(ang) >= 0.0f) for (int i = 0; i <= 18; i++) { float t = (float)i / 18.0f; cv_px((int)(cx + (x - cx) * t), (int)(9.0f + (y - 9.0f) * t), sh(R_IRON, (i & 1) ? 0.62f : 0.3f)); }
    }
    for (int i = 9; i < 28; i++) cv_px((int)cx, i, sh(R_IRON, (i & 1) ? 0.55f : 0.25f));   /* the skull's chain */
    prop_skull((int)cx - 5, 27, 0, 0, 0, 0);
    for (int k = 0; k < 9; k++) {                                   /* candles in front */
        float ang = TAU_F * (float)k / 9.0f + 0.2f, sz = sinf(ang);
        if (sz < -0.1f) continue;
        wax_candle(cx + rx * cosf(ang), ry + 3.5f * sz - 1.0f, 5.0f + 3.0f * h01(k, 1, 713), 1.3f, 1, frame, k, 0);
    }
    for (int k = 0; k < 10; k++) {                                  /* wax hanging from its underside */
        float ang = PI_F * (0.08f + 0.84f * h01(k, 1, 715)), x = cx + rx * cosf(ang);
        int y = (int)(ry + 3.5f * sinf(ang)) + 2, l = 1 + (int)(h01(k, 2, 715) * 5.0f);
        for (int j = 1; j <= l; j++) cv_px((int)x, y + j, sh(R_BONE, 0.85f - 0.07f * (float)j));
    }
    cv_store(id);
}

/* the fire pit: a ring of dressed stones gone black inside, charred logs crossed in the ashes with their cracks glowing, a
 * bone among them, embers, flames with white hearts, sparks, smoke. frame 0-2 */
static void spr_hearth(int id, int frame) {
    cv_begin(cv_sprite, TS, TS);
    float cx = 32;
    cv_flat(cx, 61.5f, 22, 2.2f, R_WARM + 1);
    for (int i = 0; i < 12; i++) {                                  /* the stones behind */
        float ang = PI_F + PI_F * (float)i / 11.0f, x = cx - 17.5f * cosf(ang), y = 55.0f + 4.5f * sinf(ang);
        cv_ellipse(x, y, 3.8f, 2.6f, R_STONE, 0.1f, 0.45f);
        cv_px((int)x, (int)y + 1, sh(R_IRON, 0.1f));                /* soot */
    }
    cv_ellipse(cx, 56, 15.0f, 3.6f, R_WARM, 0.05f, 0.25f);         /* the ashes */
    for (int i = 0; i < 40; i++) cv_px((int)(cx + (h01(i, 1, 717) - 0.5f) * 26.0f), (int)(55.0f + (h01(i, 2, 717) - 0.5f) * 5.0f), sh(R_IRON, 0.25f + 0.3f * h01(i, 3, 717)));
    for (int l = 0; l < 2; l++) {                                   /* charred logs, crossed */
        float x0 = l ? 21.0f : 41.0f, x1 = l ? 43.0f : 22.0f;
        cv_capsule(x0, 57.0f - (float)l, x1, 53.0f + (float)l, 2.0f, R_WOOD, 0.02f, 0.18f);
        for (int k = 0; k < 6; k++) { float t = 0.1f + 0.8f * h01(k, l, 719); cv_px((int)(x0 + (x1 - x0) * t), (int)(57.0f - (float)l + (53.0f - 57.0f) * t) + (k & 1) - 1, (uint8_t)(R_FLAME + 4 + ((k + frame) % 4))); }   /* cracks glowing */
    }
    cv_capsule(25, 58, 33, 59, 0.9f, R_BONE, 0.08f, 0.4f); cv_ellipse(25, 57.5f, 1.2f, 1.2f, R_BONE, 0.05f, 0.35f);   /* a charred bone in the ashes */
    for (int i = 0; i < 4; i++) {                                   /* the flames */
        float x = cx - 8.0f + (float)i * 5.5f + (h01(i, frame, 721) - 0.5f) * 3.0f, h = 14.0f + 12.0f * h01(i, frame + 5, 721);
        prop_flame(x, 54, h, 3.2f, (h01(i, frame + 9, 721) - 0.5f) * 5.0f);
    }
    prop_flame(cx + (float)(frame - 1) * 1.2f, 55, 28, 5.5f, (float)(frame - 1) * 2.5f);
    for (int i = 0; i < 10; i++) cv_px((int)(cx + (h01(i, frame, 723) - 0.5f) * 22.0f), (int)(54.0f + h01(i, frame + 1, 723) * 3.0f), (uint8_t)(R_FLAME + 7 + (i & 3)));   /* embers */
    for (int i = 0; i < 9; i++) {                                   /* the stones in front, dressed blocks, a dark joint between */
        float ang = PI_F * ((float)i + 0.5f) / 9.0f, x = cx - 17.5f * cosf(ang), y = 56.5f + 4.5f * sinf(ang), hw = 3.2f, hh = 2.6f;
        for (int pass = 0; pass < 2; pass++)
            for (int yy = (int)(y - hh) - 1; yy <= (int)(y + hh) + 1; yy++)
                for (int xx = (int)(x - hw) - 1; xx <= (int)(x + hw) + 1; xx++) {
                    float nx = ((float)xx + 0.5f - x) / (hw + (pass ? 0.0f : 0.9f)), ny = ((float)yy + 0.5f - y) / (hh + (pass ? 0.0f : 0.9f));
                    if (fabsf(nx) > 1.0f || fabsf(ny) > 1.0f || (fabsf(nx) > 0.75f && fabsf(ny) > 0.6f)) continue;   /* chamfered corners */
                    if (!pass) { cv_px(xx, yy, sh(R_WARM, 0.03f)); continue; }
                    float v = ny < -0.45f ? 0.62f : (nx < -0.6f ? 0.48f : 0.32f);   /* a lit top, a lit left face, the front in shade */
                    v += 0.08f * (h01(xx, yy, 725 + (uint32_t)i) - 0.5f) - (ny < -0.45f && nx > 0.2f ? 0.18f : 0.0f);   /* soot over the fire's side */
                    if (h01(i, 9, 727) < 0.35f && nx > 0.4f && ny > 0.2f) continue;   /* a corner broken off */
                    cv_px(xx, yy, sh(R_STONE, v));
                }
    }
    for (int i = 0; i < 7; i++) cv_px((int)(cx + (h01(i, frame, 729) - 0.5f) * 26.0f), (int)(8.0f + 22.0f * h01(i, frame + 2, 729)), (uint8_t)(R_FLAME + 9 + (i & 3)));   /* sparks */
    cv_outline(0);
    smoke_up(cx + (float)(frame - 1), 26.0f, 22.0f, 731u + (uint32_t)frame);
    cv_store(id);
}

/* the brazier: a deep iron bowl pierced with quatrefoils that glow from within, a skull in relief on its front, spikes round
 * its rim, three legs curving out to claw feet; coals heaped in it, flames, sparks. frame 0-2 */
static void spr_gbrazier(int id, int frame) {
    cv_begin(cv_sprite, TS, TS);
    cv_flat(32, 62, 14, 1.7f, R_WARM + 1);
    cv_capsule(32, 46, 32, 60, 1.1f, R_IRON, 0.08f, 0.3f);          /* the leg behind */
    for (int i = 0; i < 3; i++) {
        float x = 32.0f + (float)(i - 1) * 5.0f + (h01(i, frame, 733) - 0.5f) * 3.0f, h = 12.0f + 10.0f * h01(i, frame + 4, 733);
        prop_flame(x, 34, h, 3.4f, (h01(i, frame + 8, 733) - 0.5f) * 4.0f);
    }
    prop_flame(32, 35, 21.0f + 2.0f * (float)frame, 5.0f, (float)(frame - 1) * 2.0f);
    for (int i = 0; i < 14; i++) cv_px((int)(20.0f + 24.0f * h01(i, 1, 735)), 33 + (int)(h01(i, 2, 735) * 2.0f), (uint8_t)(R_FLAME + 5 + ((i + frame) % 6)));   /* coals heaped over its lip */
    for (int y = 34; y <= 46; y++)                                  /* the bowl */
        for (int x = 16; x <= 48; x++) {
            float hw = 15.5f - (float)(y - 34) * 0.75f, nx = ((float)x + 0.5f - 32.0f) / hw;
            if (fabsf(nx) > 1.0f) continue;
            float v = 0.18f + 0.45f * lit(nx, 0.25f, sqrtf(1.0f - nx * nx)) + (y == 34 ? 0.3f : 0.0f) - (y == 46 ? 0.1f : 0.0f);
            cv_px(x, y, sh(R_IRON, v));
        }
    static const int QX[4] = { 20, 25, 39, 44 };
    for (int q = 0; q < 4; q++) {                                   /* quatrefoils pierced through it, glowing */
        int qx = QX[q], qy = 40;
        for (int j = -2; j <= 2; j++) for (int i = -2; i <= 2; i++) if (abs(i) + abs(j) <= 3) cv_px(qx + i, qy + j, sh(R_IRON, 0.06f));   /* its pierced rim */
        static const int QF[5][2] = { { 0, -1 }, { -1, 0 }, { 1, 0 }, { 0, 1 }, { 0, 0 } };
        for (int k = 0; k < 5; k++) cv_px(qx + QF[k][0], qy + QF[k][1], (uint8_t)(R_FLAME + 6 + (k == 4) * 4 + ((frame + q) & 1)));
    }
    grid_stamp(SKULL_SMALL, 9, 10, 28, 36, R_IRON, R_IRON, 0, 0);   /* a skull in relief on its front */
    for (int y = 36; y < 46; y++) for (int x = 28; x < 37; x++) { uint8_t c = cv_sprite[y * TS + x]; if (c > R_IRON && c < R_IRON + 16) cv_px(x, y, (uint8_t)(c + 1 > R_IRON + 15 ? R_IRON + 15 : c + 1)); }
    for (int k = 0; k < 7; k++) { int x = 18 + k * 5; cv_px(x, 33, sh(R_IRON, 0.7f)); cv_px(x, 32, sh(R_IRON, 0.55f)); cv_px(x, 31, sh(R_IRON, 0.4f)); }   /* spikes round its rim */
    for (int s = -1; s <= 1; s += 2) {                              /* legs, curving out to claw feet */
        float x0 = 32.0f + (float)s * 7.0f, xm = 32.0f + (float)s * 10.0f, x1 = 32.0f + (float)s * 14.0f;
        cv_capsule(x0, 45, xm, 53, 1.3f, R_IRON, 0.15f, 0.65f); cv_capsule(xm, 53, x1, 61, 1.1f, R_IRON, 0.15f, 0.6f);
        for (int t = 0; t < 3; t++) cv_px((int)x1 + s * t, 62 - (t == 2), sh(R_IRON, 0.5f));
    }
    for (int i = 0; i < 5; i++) cv_px((int)(32.0f + (h01(i, frame, 737) - 0.5f) * 20.0f), (int)(4.0f + 10.0f * h01(i, frame + 2, 737)), (uint8_t)(R_FLAME + 10));   /* sparks */
    cv_outline(0);
    cv_store(id);
}

/* the lantern (run II's own, and only ever hung over its water): an iron lantern on its chain, a gothic roof with a finial,
 * horn panes lit amber from within (one cracked), the candle a brighter flame behind them, rust on its iron and a drip from
 * its foot. frame 0-1 */
static void spr_hornlantern(int id, int frame) {
    cv_begin(cv_sprite, TS, TS);
    chain_down(32, 0, 12);
    cv_ellipse(32, 13, 1.6f, 1.6f, R_IRON, 0.2f, 0.75f);            /* its ring */
    for (int y = 15; y <= 21; y++) {                                /* the roof, a spire */
        float hw = 1.0f + (float)(y - 15) * 1.15f;
        for (int x = (int)(32 - hw); x <= (int)(32 + hw); x++) { float nx = ((float)x + 0.5f - 32.0f) / (hw + 0.01f); cv_px(x, y, sh(R_IRON, 0.25f + 0.45f * lit(nx, -0.4f, sqrtf(fmaxf(0.0f, 1.0f - nx * nx))))); }
    }
    for (int x = 23; x <= 41; x++) cv_px(x, 22, sh(R_IRON, 0.6f - 0.02f * (float)abs(x - 30)));   /* the eave */
    for (int y = 23; y < 36; y++)                                   /* the cage: two panes of horn in front, the sides dimmer */
        for (int x = 24; x <= 40; x++) {
            float nx = ((float)x + 0.5f - 32.0f) / 8.0f;
            int bar = x == 24 || x == 40 || x == 32 || x == 27 || x == 37 || y == 23 || y == 35;
            if (bar) { cv_px(x, y, sh(R_IRON, x == 24 || x == 27 ? 0.55f : 0.3f)); continue; }
            float side = (x < 27 || x > 37) ? 0.55f : 1.0f;
            float glow = (1.0f - fabsf(nx) * 0.5f - fabsf((float)y - 29.0f) / 13.0f) * side + (frame ? 0.04f : -0.04f) - 0.12f * fbm((float)x / 2.0f, (float)y / 2.0f, 16, 739);
            cv_px(x, y, (uint8_t)(R_FLAME + clampi((int)(glow * 12.0f), 2, 12)));
        }
    for (int k = 0; k < 6; k++) cv_px(35 - k / 2, 25 + k, (uint8_t)(R_FLAME + 2));   /* a pane cracked */
    prop_flame(32.5f, 32, 6.0f + (float)frame, 1.6f, frame ? 0.5f : -0.5f);
    for (int x = 23; x <= 41; x++) { cv_px(x, 36, sh(R_IRON, 0.55f - 0.015f * (float)abs(x - 30))); cv_px(x, 37, sh(R_IRON, 0.25f)); }   /* its foot */
    cv_px(32, 38, sh(R_IRON, 0.4f));
    for (int k = 0; k < 8; k++) cv_px(24 + (int)(h01(k, 1, 741) * 17.0f), 22 + (int)(h01(k, 2, 741) * 16.0f), sh(R_BRICK, 0.25f));   /* rust */
    for (int x = 23; x <= 41; x += 6) cv_px(x, 22, sh(R_BRICK, 0.3f));
    cv_outline(0);
    cv_px(31, 40, sh(R_GHOST, 0.6f)); cv_px(31, 43, sh(R_GHOST, 0.75f));   /* a drip from its foot */
    cv_store(id);
}

/* P3: the graves. A carving is a mask over the stone: 1 raised (lit, its shadow below and to the right), 2 cut in (dark, its
 * lower lip catching the light) */
static uint8_t g_carve[TS * TS];
static void cmark(int x, int y, int k) { if ((unsigned)x < (unsigned)TS && (unsigned)y < (unsigned)TS) g_carve[y * TS + x] = (uint8_t)k; }
