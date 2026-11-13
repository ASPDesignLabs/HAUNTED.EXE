/* A part of crawl_art.c (docs/PLAN-tidy.md): it is #included there in its place and never compiled on its own.
 * Her hall's things: the Lampstand, the Dead Bell, the Weeping Font, the lectern; the tutorial's rimed seal and its map; the map on sale; Severance's desks and cooler; the caster's double. */

static void spr_lampstand(void) {
    cv_begin(cv_sprite, TS, TS);
    cv_flat(32, 62, 13, 1.8f, R_WARM + 1);
    cv_capsule(32, 53, 20, 62, 1.6f, R_IRON, 0.04f, 0.26f);        /* the feet */
    cv_capsule(32, 53, 44, 62, 1.6f, R_IRON, 0.04f, 0.26f);
    cv_capsule(32, 54, 32, 62, 1.4f, R_IRON, 0.04f, 0.22f);
    cv_capsule(32, 13, 32, 56, 1.9f, R_IRON, 0.05f, 0.3f);         /* the stem */
    cv_capsule(28, 50, 36, 50, 1.3f, R_IRON, 0.06f, 0.32f);        /* a collar */
    static const float ARM[3][6] = { { 32, 45, 13, 41, 9, 33 }, { 32, 37, 18, 30, 16, 24 }, { 32, 27, 26, 22, 25, 17 } };
    for (int i = 0; i < 3; i++)
        for (int side = 0; side < 2; side++) {                     /* the arms, curling up to their bowls */
            float m = side ? -1.0f : 1.0f;
            const float *a = ARM[i];
            cv_capsule(32 + m * (a[0] - 32), a[1], 32 + m * (a[2] - 32), a[3], 1.1f, R_IRON, 0.05f, 0.28f);
            cv_capsule(32 + m * (a[2] - 32), a[3], 32 + m * (a[4] - 32), a[5], 1.0f, R_IRON, 0.05f, 0.28f);
        }
    for (int b = 0; b < 6; b++) {                                  /* the bowls, shallow, catching a little light on the rim */
        float bx = LAMP_BOWL[b][0], by = LAMP_BOWL[b][1] + 1;
        cv_flat(bx, by, 4.6f, 1.9f, sh(R_IRON, 0.12f));
        for (int x = (int)bx - 4; x <= (int)bx + 4; x++) cv_px(x, (int)by - 1, sh(R_IRON, 0.32f + 0.04f * (x - bx)));
        cv_px((int)bx, (int)by + 2, sh(R_IRON, 0.1f));
    }
    cv_capsule(32, 9, 32, 13, 0.9f, R_IRON, 0.1f, 0.38f);          /* a spike at the top */
    cv_outline(0);
    cv_store(SP_LAMPSTAND);
}

/* The Dead Bell (docs/PLAN-b11.md, section 12): a cracked bronze bell under a beam on two posts. Corrupted it goes
 * black, and the crack burns a sickly green */
static void spr_deadbell(int id, int corrupt) {
    cv_begin(cv_sprite, TS, TS);
    cv_flat(32, 62, 18, 1.8f, R_WARM + 1);
    cv_capsule(13, 12, 13, 62, 2.2f, R_WOOD, 0.08f, 0.42f);         /* the posts, */
    cv_capsule(51, 12, 51, 62, 2.2f, R_WOOD, 0.08f, 0.42f);
    cv_capsule(10, 11, 54, 11, 2.4f, R_WOOD, 0.1f, 0.5f);           /* the beam, */
    cv_capsule(16, 18, 13, 14, 1.0f, R_WOOD, 0.08f, 0.35f); cv_capsule(48, 18, 51, 14, 1.0f, R_WOOD, 0.08f, 0.35f);   /* braces */
    cv_capsule(32, 12, 32, 18, 1.0f, R_IRON, 0.06f, 0.3f);          /* the hanger */
    for (int y = 18; y <= 47; y++) {                                /* the bell: a crown, a waist, a flared lip */
        float u = (float)(y - 18) / 29.0f, w = 5.5f + 4.0f * u + 7.5f * u * u * u;
        for (int x = (int)(32 - w); x <= (int)(32 + w); x++) {
            float nx = (x + 0.5f - 32) / (w + 0.01f), l = lit(nx, -0.2f, sqrtf(fmaxf(0.0f, 1.0f - nx * nx)));
            uint8_t c = corrupt ? sh(R_STONE, 0.03f + 0.18f * l) : sh(R_GOLD, 0.22f + 0.6f * l);
            if (y >= 45 || (y > 20 && (y - 20) % 9 == 0)) c = corrupt ? sh(R_STONE, 0.08f + 0.2f * l) : sh(R_GOLD, 0.35f + 0.6f * l);   /* bands, the lip */
            cv_px(x, y, c);
        }
    }
    int cx = 27;                                                    /* the crack, down from the shoulder */
    for (int y = 22; y <= 46; y++) {
        cx += (int)(h01(y, 7, 13) * 3.0f) - 1;
        cv_px(cx, y, corrupt ? (uint8_t)(R_MOSS + 13 + (y & 1)) : sh(R_GOLD, 0.05f));
        if (corrupt && (y & 3) == 0) cv_px(cx + 1, y, (uint8_t)(R_MOSS + 10));
    }
    cv_flat(32, 50, 2.0f, 2.2f, corrupt ? sh(R_STONE, 0.1f) : sh(R_IRON, 0.3f));   /* the clapper */
    if (corrupt)
        for (int i = 0; i < 10; i++) cv_px((int)(20 + h01(i, 2, 17) * 24), (int)(48 + h01(i, 3, 17) * 10), (uint8_t)(R_MOSS + 9 + (i & 3)));   /* what drips from it */
    cv_outline(0);
    cv_store(id);
}
/* The Weeping Font: a stone basin on a short pillar, under a carved face on a slab. Dry, its tracks are dust; cursed, it
 * brims dark red and the face weeps red */
static void spr_font(int id, int cursed) {
    cv_begin(cv_sprite, TS, TS);
    cv_flat(32, 62, 18, 1.8f, R_WARM + 1);
    for (int y = 4; y <= 46; y++)                                   /* a broad slab, arched, a niche cut in it */
        for (int x = 8; x <= 56; x++) {
            float d = hypotf(x + 0.5f - 32, y + 0.5f - 26);
            if (y < 26 && d > 24.5f) continue;
            int niche = y >= 12 && x >= 17 && x <= 47 && (y >= 26 || hypotf(x + 0.5f - 32, y + 0.5f - 27) < 15.5f);
            float v = 0.22f + 0.22f * fbm(x / 4.0f, y / 4.0f, 32, 31) + (niche ? -0.1f : 0.0f) + (x < 11 || (y < 26 && d > 22.5f) ? 0.12f : 0.0f);
            cv_px(x, y, sh(R_STONE, v));
        }
    for (int a = 0; a < 40; a++) {                                  /* a band of carving round the arch */
        float an = PI_F + (float)a / 39.0f * PI_F;
        int x = (int)(32 + cosf(an) * 19.5f), y = (int)(26 + sinf(an) * 19.5f);
        cv_px(x, y, (a & 1) ? sh(R_STONE, 0.5f) : sh(R_STONE, 0.12f));
    }
    cv_flat(32, 26, 9.5f, 11.0f, sh(R_STONE, 0.3f));               /* the face in the niche: */
    cv_flat(28, 23, 2.4f, 1.5f, sh(R_STONE, 0.03f)); cv_flat(36, 23, 2.4f, 1.5f, sh(R_STONE, 0.03f));   /* hollow eyes, */
    cv_capsule(32, 24, 32, 29, 0.9f, R_STONE, 0.35f, 0.55f);       /* a nose, */
    cv_capsule(29, 32, 35, 32, 0.8f, R_STONE, 0.02f, 0.1f);        /* a mouth turned down */
    cv_px(28, 33, sh(R_STONE, 0.05f)); cv_px(36, 33, sh(R_STONE, 0.05f));
    for (int side = 0; side < 2; side++)                            /* the tears' tracks, down to the basin */
        for (int y = 25; y <= 41; y++) {
            int x = (side ? 36 : 28) + (int)(sinf((float)y * 0.5f + side) * 0.8f);
            cv_px(x, y, cursed ? (uint8_t)(R_BLOOD + 9 + (y & 1) * 2) : sh(R_STONE, 0.45f));
        }
    for (int y = 41; y <= 47; y++) {                                /* a shallow basin, wide */
        float u = (float)(y - 41) / 6.0f, w = 21.0f - 8.0f * u * u;
        for (int x = (int)(32 - w); x <= (int)(32 + w); x++) {
            float nx = (x + 0.5f - 32) / w;
            cv_px(x, y, sh(R_STONE, 0.2f + 0.4f * lit(nx, 0.3f, sqrtf(fmaxf(0.0f, 1.0f - nx * nx)))));
        }
    }
    for (int x = 11; x <= 53; x++) {                                /* its brim, and what is in it */
        cv_px(x, 41, sh(R_STONE, 0.62f));
        if (x > 12 && x < 52) cv_px(x, 42, cursed ? (uint8_t)(R_BLOOD + 4 + ((x * 7) % 5 == 0 ? 6 : 0)) : sh(R_STONE, 0.06f));
    }
    cv_capsule(32, 48, 32, 57, 5.5f, R_STONE, 0.12f, 0.42f);        /* a short thick stem, */
    cv_capsule(23, 59, 41, 59, 2.4f, R_STONE, 0.12f, 0.42f);        /* a broad foot */
    if (cursed) for (int i = 0; i < 5; i++) cv_px(14 + i * 9, 43 + (i & 1), (uint8_t)(R_BLOOD + 6));   /* spilling over the lip */
    cv_outline(0);
    cv_store(id);
}

/* a bowl's flame, in texels from the middle of the bowl's rim (dy up is negative): the step of its element's ramp
 * there, 0 none. Taller and fuller as the line ranks, swaying, the core brightest low down */
int crawl_lamp_flame(int dx, int dy, int rank, float t, int seed) {
    float h = 5.0f + 0.25f * (float)(rank < 20 ? rank : 20), u = (float)-dy / h;
    if (u < 0.0f || u > 1.0f) return 0;
    float sway = sinf(t * 9.0f + (float)seed * 1.7f) * 0.7f * u + sinf(t * 23.0f + (float)seed) * 0.3f * u;
    float w = (2.0f + 0.05f * (float)rank) * powf(sinf(3.14159f * (0.15f + 0.85f * u)), 0.7f) * (1.0f - 0.45f * u);
    float x = fabsf((float)dx - sway);
    if (x > w) return 0;
    float k = 1.0f - x / (w + 0.01f) * 0.6f - u * 0.5f;
    return 6 + (int)(k * 9.0f + 0.5f);
}

static void spr_lectern(void) {
    cv_begin(cv_sprite, TS, TS);
    float cx = 32;
    cv_flat(cx, 62, 11, 1.8f, R_WARM + 1);
    const float base[8] = { 22, 61, 42, 61, 39, 56, 25, 56 };
    cv_poly(base, 4, R_WOOD, 0.08f, 0.32f, 56, 6);                 /* its foot */
    cv_capsule(cx, 36, cx, 57, 2.6f, R_WOOD, 0.08f, 0.42f);        /* the stem */
    cv_capsule(cx - 6, 45, cx + 6, 45, 1.2f, R_WOOD, 0.1f, 0.4f);  /* a turned collar */
    const float board[8] = { 11, 35, 53, 35, 47, 27, 17, 27 };
    cv_poly(board, 4, R_WOOD, 0.14f, 0.42f, 27, 9);                /* the sloping top, nearer edge lower */
    for (int x = 11; x <= 53; x++) { cv_px(x, 36, sh(R_WOOD, 0.08f)); cv_px(x, 37, sh(R_WOOD, 0.05f)); }   /* its lip */
    const float cover[8] = { 14, 33, 50, 33, 46, 25, 18, 25 };
    cv_poly(cover, 4, R_BLOOD, 0.1f, 0.24f, 25, 8);                /* the binding, dark red leather */
    for (int side = 0; side < 2; side++) {                         /* the two pages, bowed up to the spine */
        for (int y = 24; y <= 32; y++) {
            float k = (y - 24) / 8.0f, xa = side ? 33.0f : 19.0f - 4.0f * k + 1.0f, xb = side ? 45.0f + 4.0f * k - 1.0f : 31.0f;
            for (int x = (int)xa; x <= (int)xb; x++) {
                float bow = side ? (x - 33) / 13.0f : (31 - x) / 13.0f;
                float v = 0.72f + 0.12f * (1.0f - bow) - (y == 24 ? 0.0f : 0.0f);
                if (((y - 25) % 2 == 0) && y < 32 && x > (int)xa + 1 && x < (int)xb - 1 && h01(x, y, 141 + side) > 0.3f) v -= 0.3f;   /* close-written */
                cv_px(x, y - (int)(1.5f * (1.0f - bow)), sh(R_BONE, v));
            }
        }
    }
    cv_line(32, 22, 32, 32, sh(R_BONE, 0.35f));                    /* the spine's fold */
    cv_line(36, 32, 37, 41, sh(R_BLOOD, 0.55f));                   /* the ribbon */
    cv_px(37, 42, sh(R_BLOOD, 0.4f)); cv_px(36, 42, sh(R_BLOOD, 0.4f));
    cv_outline(0);
    static const int MOTE[5][2] = { { 26, 17 }, { 35, 14 }, { 41, 19 }, { 30, 10 }, { 22, 21 } };
    for (int i = 0; i < 5; i++) cv_px(MOTE[i][0], MOTE[i][1], (uint8_t)(R_GLOW + 11 + (i & 1) * 3));   /* after the rim: they float free */
    cv_store(SP_LECTERN);
}

/* the tutorial's switch (docs/PLAN-tutorial.md 7.2): a squat stele on a stepped foot, a round seal sunk in its face with a
 * six-armed sigil cut in it. Rimed (0): furred white, icicles from its head and the seal's lip, frost on the floor, the sigil
 * pale with cold; thawed (1): dark and running wet, drips, a pool at its foot, a breath of steam; spent (2): the seal split
 * across, a piece of its head gone, dull and dry */
static void spr_mk_rseal(int id, int st) {
    cv_begin(cv_sprite, TS, TS);
    cv_flat(32, 62, 18, 1.8f, R_WARM + 1);
    if (st == 0) cv_flat(32, 61, 21, 2.6f, sh(R_GHOST, 0.62f));   /* frost spread on the floor */
    if (st == 1) cv_flat(33, 62, 17, 1.6f, sh(R_GHOST, 0.22f));   /* the pool */
    float wet = st == 1 ? -0.08f : 0.0f;
    for (int y = 54; y <= 62; y++) for (int x = 17; x <= 47; x++) {   /* the foot, two steps */
        if (y < 58 && (x < 20 || x > 44)) continue;
        int edge = y == 54 || y == 58;
        cv_px(x, y, sh(R_STONE, (edge ? 0.52f : 0.28f) + 0.12f * (x < 21) - 0.08f * (x > 43) + 0.06f * h01(x / 2, y, 801) + wet));
    }
    for (int y = 4; y < 54; y++) for (int x = 21; x <= 43; x++) {   /* the shaft, its head a pointed arch */
        float dx = x + 0.5f - 32.0f, top = 18.0f - g_arch(dx, 11.0f);
        if (fabsf(dx) > 11.0f || y < top) continue;
        if (st == 2 && y < 16.0f - 0.7f * dx && dx > -3.0f) continue;   /* a piece of its head gone */
        int edge = fabsf(dx) > 9.5f || y < top + 1.6f;
        float v = (edge ? 0.42f : 0.27f) + 0.15f * (dx < -8.5f) - 0.1f * (dx > 8.5f) + 0.07f * h01(x / 2, y / 3, 803) + wet;
        if (st == 1 && h01(x, y / 4, 805) > 0.86f) v += 0.25f;      /* wet streaks catching the light */
        cv_px(x, y, sh(R_STONE, v));
    }
    for (int y = 23; y <= 41; y++) for (int x = 23; x <= 41; x++) {   /* the seal, sunk, and its sigil */
        float dx = x + 0.5f - 32.0f, dy = y + 0.5f - 32.0f, r = sqrtf(dx * dx + dy * dy);
        if (r > 9.0f) continue;
        float v = r > 7.8f ? 0.55f : 0.16f + 0.14f * lit(dx / 9.0f, dy / 9.0f, 0.5f);
        int ramp = R_STONE;
        float a = atan2f(dy, dx), arm = fabsf(sinf(3.0f * a));     /* six arms */
        if (r < 6.6f && (arm < 0.16f || (r > 3.6f && r < 4.4f && arm < 0.5f))) {   /* cut, with little cross-twigs */
            if (st == 0) { ramp = R_GHOST; v = 0.9f; } else v = st == 1 ? 0.05f : 0.08f;
        }
        cv_px(x, y, sh(ramp, v + (ramp == R_STONE ? wet : 0.0f)));
    }
    if (st == 2) {                                                 /* split across, the halves a little apart */
        int x = 24;
        for (int y = 22; y <= 53; y++) { x += (int)(h01(y, 1, 807) * 3.0f) - 1; if (y > 26) x = x < 26 ? 26 : x > 40 ? 40 : x; cv_px(x, y, sh(R_STONE, 0.02f)); cv_px(x + 1, y, sh(R_STONE, 0.08f)); }
        cv_flat(50, 59, 4.5f, 2.6f, sh(R_STONE, 0.3f)); cv_flat(49, 58, 2.0f, 1.0f, sh(R_STONE, 0.45f));   /* the piece fallen */
        for (int i = 0; i < 7; i++) cv_flat(12 + h01(i, 2, 809) * 40, 60 + h01(i, 3, 809) * 2, 1.4f, 0.9f, sh(R_STONE, 0.25f + 0.2f * h01(i, 4, 809)));
    }
    if (st == 0) {                                                 /* rime on its head and lips, icicles */
        for (int i = 0; i < 90; i++) {
            int x = 21 + (int)(h01(i, 5, 811) * 23.0f), y = 4 + (int)(h01(i, 6, 811) * 50.0f);
            float dx = x + 0.5f - 32.0f, top = 18.0f - g_arch(dx, 11.0f);
            int nearlip = y < top + 3.0f || fabsf(dx) > 9.0f || (fabsf(hypotf(dx, y + 0.5f - 32.0f) - 9.0f) < 1.2f);
            if (y >= top && (nearlip || h01(i, 7, 811) > 0.6f)) cv_px(x, y, sh(R_GHOST, 0.7f + 0.3f * h01(i, 8, 811)));
        }
        static const int IC[7][2] = { { 22, 17 }, { 25, 19 }, { 40, 18 }, { 43, 16 }, { 28, 41 }, { 32, 42 }, { 36, 41 } };
        for (int i = 0; i < 7; i++) {
            int len = 3 + (int)(h01(i, 9, 811) * 5.0f);
            for (int k = 0; k < len; k++) cv_px(IC[i][0], IC[i][1] + k, sh(R_GHOST, 0.95f - 0.5f * k / len));
        }
        for (int x = 18; x <= 46; x++) if (h01(x, 10, 811) > 0.4f) cv_px(x, 54, sh(R_GHOST, 0.85f));   /* the foot's step rimed */
    }
    if (st == 1) {                                                 /* drips where the icicles were, at full brightness */
        static const int DR[5][2] = { { 22, 20 }, { 40, 21 }, { 43, 19 }, { 28, 44 }, { 36, 44 } };
        for (int i = 0; i < 5; i++) { cv_px(DR[i][0], DR[i][1], sh(R_GHOST, 0.75f)); cv_px(DR[i][0], DR[i][1] + 3, sh(R_GHOST, 0.55f)); }
    }
    cv_outline(0);
    if (st == 1)                                                   /* a breath of steam, after the rim */
        for (int i = 0; i < 9; i++) cv_px(26 + (int)(h01(i, 11, 811) * 12.0f), 1 + (int)(h01(i, 12, 811) * 6.0f), sh(R_GHOST, 0.45f + 0.2f * (i & 1)));
    cv_store(id);
}
/* the tutorial's map (7.4), lying on the floor where it fell: the scroll's sheet half unrolled towards you, foreshortened, the
 * maze in brown ink, its near roller and red seal, the rest still rolled; motes over it, as over hers */
static void spr_mapfloor(void) {
    cv_begin(cv_sprite, TS, TS);
    cv_flat(32, 59, 22, 3.4f, R_WARM + 1);                         /* its shadow */
    for (int y = 49; y <= 59; y++) {                               /* the sheet, wider as it comes nearer */
        float u = (float)(y - 49) / 10.0f, half = 14.0f + 5.0f * u;
        for (int x = (int)(32 - half); x <= (int)(32 + half); x++) {
            float v = 0.74f + 0.12f * (fbm(x / 6.0f, y / 3.0f, 16, 821) - 0.5f) - 0.08f * (x <= (int)(32 - half) + 0 || x >= (int)(32 + half));
            if (h01(x, y, 823) > 0.93f && (x < 32 - half + 2 || x > 32 + half - 2)) continue;   /* its edges a little torn */
            cv_px(x, y, sh(R_BONE, v));
        }
    }
    for (int i = 0; i < 8; i++) {                                  /* the maze, foreshortened */
        float y0 = 50.5f + h01(i, 1, 825) * 7.5f, u = (y0 - 49.0f) / 10.0f, half = 12.0f + 5.0f * u;
        float x0 = 32.0f + (h01(i, 2, 825) - 0.5f) * 2.0f * half * 0.9f;
        if (i & 1) cv_line(x0, y0, fminf(x0 + 4 + h01(i, 3, 825) * 6, 32 + half), y0, sh(R_WOOD, 0.12f));
        else cv_line(x0, y0, x0 + (x0 - 32.0f) * 0.15f, fminf(y0 + 2.0f + h01(i, 3, 825) * 3.0f, 58.0f), sh(R_WOOD, 0.12f));
    }
    cv_px(36, 55, sh(R_BLOOD, 0.75f)); cv_px(37, 55, sh(R_BLOOD, 0.75f));   /* a mark on it */
    cv_capsule(17, 47.5f, 47, 47.5f, 2.6f, R_BONE, 0.32f, 0.7f);   /* the rest, rolled, at the far edge */
    cv_ellipse(15.5f, 47.5f, 1.6f, 2.0f, R_WOOD, 0.1f, 0.45f); cv_ellipse(48.5f, 47.5f, 1.6f, 2.0f, R_WOOD, 0.1f, 0.45f);
    cv_capsule(12, 60.5f, 52, 60.5f, 1.6f, R_WOOD, 0.12f, 0.5f);   /* its near roller */
    cv_ellipse(11, 60.5f, 1.4f, 1.6f, R_WOOD, 0.1f, 0.5f); cv_ellipse(53, 60.5f, 1.4f, 1.6f, R_WOOD, 0.1f, 0.5f);
    cv_line(40, 61, 43, 62, sh(R_BLOOD, 0.4f));                    /* the ribbon, and its seal on the floor */
    cv_ellipse(45, 62, 2.6f, 1.5f, R_BLOOD, 0.15f, 0.55f);
    cv_outline(0);
    static const int MOTE[5][2] = { { 22, 40 }, { 33, 35 }, { 44, 41 }, { 28, 31 }, { 39, 28 } };
    for (int i = 0; i < 5; i++) cv_px(MOTE[i][0], MOTE[i][1], (uint8_t)(R_GLOW + 10 + (i & 1) * 3));
    cv_store(SP_MAPFLOOR);
}

/* The merchant's map (1.0.0b9): a scroll half unrolled between its two rollers, a maze drawn on it in
 * brown ink, a red seal; it floats beside her, so it casts no shadow */
static void spr_mapware(void) {
    cv_begin(cv_sprite, TS, TS);
    for (int x = 17; x <= 47; x++) {                               /* the sheet, its edges a little torn */
        int top = 22 + (int)(1.6f * h01(x, 1, 151)) + (x % 9 == 4), bot = 44 - (int)(1.6f * h01(x, 2, 151)) - (x % 11 == 6);
        for (int y = top; y <= bot; y++) {
            float v = 0.74f + 0.12f * (fbm(x / 6.0f, y / 6.0f, 16, 153) - 0.5f) - 0.1f * ((y - top) < 1 || (bot - y) < 1);
            cv_px(x, y, sh(R_BONE, v));
        }
    }
    for (int i = 0; i < 9; i++) {                                  /* the maze on it */
        int x0 = 21 + (int)(h01(i, 3, 155) * 20.0f), y0 = 26 + (int)(h01(i, 4, 155) * 14.0f), len = 3 + (int)(h01(i, 5, 155) * 6.0f);
        if (i & 1) cv_line(x0, y0, fminf(x0 + len, 44), y0, sh(R_WOOD, 0.12f)); else cv_line(x0, y0, x0, fminf(y0 + len, 41), sh(R_WOOD, 0.12f));
    }
    cv_line(20, 25, 44, 25, sh(R_WOOD, 0.2f)); cv_line(20, 41, 44, 41, sh(R_WOOD, 0.2f));
    cv_line(20, 25, 20, 41, sh(R_WOOD, 0.2f)); cv_line(44, 25, 44, 41, sh(R_WOOD, 0.2f));
    cv_px(38, 37, sh(R_BLOOD, 0.75f)); cv_px(39, 37, sh(R_BLOOD, 0.75f)); cv_px(38, 36, sh(R_BLOOD, 0.6f));   /* you are here */
    for (int k = 0; k < 2; k++) {                                  /* the rollers, dark wood, their knobs */
        float rx = k ? 48.5f : 15.5f;
        cv_capsule(rx, 20, rx, 46, 2.6f, R_BONE, 0.38f, 0.7f);
        cv_ellipse(rx, 17.5f, 1.8f, 1.8f, R_WOOD, 0.1f, 0.45f);
        cv_ellipse(rx, 48.5f, 1.8f, 1.8f, R_WOOD, 0.1f, 0.45f);
    }
    cv_ellipse(32, 45.5f, 3.2f, 2.6f, R_BLOOD, 0.15f, 0.5f);       /* the seal hanging from it */
    cv_line(32, 43, 32, 44, sh(R_BLOOD, 0.3f));
    cv_outline(0);
    static const int MOTE[4][2] = { { 12, 30 }, { 52, 26 }, { 24, 14 }, { 42, 52 } };
    for (int i = 0; i < 4; i++) cv_px(MOTE[i][0], MOTE[i][1], (uint8_t)(R_GLOW + 10 + (i & 1) * 3));
    cv_store(SP_MAPWARE);
}

/* Severance: four desks pushed together behind low green partitions, an old terminal on each, numbers
 * on their dark screens. Drawn in R_FLOOR and R_CEIL, so only a Severance pocket shows it true. */
static void spr_desks(void) {
    cv_begin(cv_sprite, TS, TS);
    static const int MX[2][2] = { { 9, 27 }, { 36, 54 } };          /* two terminals showing over the partitions */
    for (int m = 0; m < 2; m++) {
        int x0 = MX[m][0], x1 = MX[m][1], y0 = m ? 24 : 22, y1 = 39;
        for (int y = y0; y <= y1; y++)
            for (int x = x0; x <= x1; x++) {
                float v = 0.80f - (x - x0) * 0.012f;
                if (x == x0 || y == y0) v = 0.92f;
                if (x == x1) v = 0.55f;
                uint8_t c = sh(R_CEIL, v);
                if (x >= x0 + 3 && x <= x1 - 3 && y >= y0 + 3 && y <= y1 - 4) {   /* the screen */
                    c = sh(R_IRON, 0.08f);
                    if ((y - y0) % 3 == 1 && h01(x, y, 611 + m) > 0.45f) c = sh(R_GHOST, 0.55f + 0.3f * h01(x, y, 612));
                }
                cv_px(x, y, c);
            }
    }
    for (int y = 36; y < 62; y++)                                   /* the partitions */
        for (int x = 3; x <= 60; x++) {
            uint8_t c = sh(R_FLOOR, 0.56f + 0.05f * h01(x, y, 613) - (y > 56 ? 0.08f : 0.0f));
            if (y <= 37) c = sh(R_CEIL, y == 36 ? 0.94f : 0.72f);     /* the white rail along the top */
            if (x <= 4 || x >= 59 || x == 31 || x == 32) c = sh(R_CEIL, x <= 4 || x == 31 ? 0.86f : 0.6f);   /* posts */
            cv_px(x, y, c);
        }
    for (int x = 3; x <= 60; x += 1) { cv_px(x, 62, sh(R_IRON, 0.3f)); cv_px(x, 63, sh(R_IRON, 0.18f)); }   /* feet */
    cv_outline(0);
    cv_store(SP_DESKS);
}

/* Severance's water cooler: a white cabinet, red and blue taps, cups at its side, and its bottle upside
 * down on top, full, or lower after the drink */
static void spr_cooler(int id, int low) {
    cv_begin(cv_sprite, TS, TS);
    for (int y = 36; y < 63; y++)                                   /* the cabinet */
        for (int x = 22; x <= 41; x++) {
            float v = 0.92f - (x - 22) * 0.016f;
            if (y == 36) v = 0.96f;
            uint8_t c = sh(R_CEIL, v);
            if (y >= 40 && y <= 49 && x >= 25 && x <= 38) c = sh(R_CEIL, y == 40 ? 0.3f : 0.45f);   /* the recess */
            if (y >= 48 && y <= 49 && x >= 26 && x <= 37) c = sh(R_IRON, 0.5f);   /* its drip tray */
            cv_px(x, y, c);
        }
    for (int k = 0; k < 2; k++) {                                   /* the taps, hot and cold */
        int tx = k ? 34 : 28;
        for (int y = 41; y <= 44; y++) for (int x = tx; x <= tx + 1; x++) cv_px(x, y, k ? sh(R_GHOST, 0.45f + 0.15f * (x == tx)) : sh(R_BLOOD, 0.55f + 0.2f * (x == tx)));
    }
    for (int y = 39; y <= 52; y++) for (int x = 43; x <= 45; x++) cv_px(x, y, sh(R_CEIL, x == 43 ? 0.95f : 0.7f));   /* the cups */
    cv_px(44, 53, sh(R_CEIL, 0.6f));
    for (int y = 30; y <= 35; y++) for (int x = 29; x <= 34; x++) cv_px(x, y, sh(R_GHOST, 0.6f + 0.15f * (x <= 30)));   /* its neck */
    float level = low ? 24.0f : 12.0f;                              /* the water's top, the bottle being upside down */
    for (int y = 8; y < 30; y++)
        for (int x = 21; x <= 42; x++) {
            float nx = (x + 0.5f - 31.5f) / 10.5f, ny = (y + 0.5f - 19.0f) / 11.0f;
            if (nx * nx * nx * nx + ny * ny * ny * ny > 1.0f) continue;   /* a rounded square, as such bottles are */
            float l = lit(nx, ny, sqrtf(fmaxf(0.0f, 1.0f - nx * nx)));
            float v = y >= level ? 0.42f + 0.25f * l : 0.88f + 0.1f * l;   /* water below, bare plastic above */
            if (y == 14 || y == 20 || y == 26) v += 0.06f;           /* its ribs */
            if ((int)level == y) v = 0.96f;                         /* the line of the water */
            cv_px(x, y, sh(R_GHOST, v));
        }
    cv_outline(0);
    cv_store(id);
}

/* An Exquisite Blink's double: the caster as the creatures see it, hooded and robed, one hand up,
 * all in violet light that thins towards the hem. Drawn at full brightness, and faded as it goes. */
static void spr_double(void) {
    cv_begin(cv_sprite, TS, TS);
    for (int y = 17; y < 63; y++) {                                 /* the robe */
        float t = (y - 17) / 45.0f, hw = 8.0f + 6.5f * t;
        for (int x = (int)(32 - hw) - 1; x <= (int)(32 + hw) + 1; x++) {
            float dx = x + 0.5f - 32;
            if (fabsf(dx) > hw) continue;
            if (y > 48 && h01(x, y, 351) < (y - 48) / 15.0f) continue;   /* thinning away below */
            float u = dx / hw, v = 0.35f + 0.45f * (1.0f - u * u) * (1.0f - 0.4f * t);
            if (sinf(dx * 1.2f + y * 0.04f) > 0.65f) v -= 0.1f;     /* folds */
            cv_px(x, y, sh(R_PURPLE, v));
        }
    }
    cv_capsule(24, 21, 21, 38, 3.2f, R_PURPLE, 0.3f, 0.7f);         /* the left arm, hanging */
    cv_ellipse(21, 39, 1.8f, 1.8f, R_PURPLE, 0.6f, 0.95f);
    cv_capsule(40, 21, 46, 12, 3.0f, R_PURPLE, 0.3f, 0.75f);        /* the right, raised as it cast */
    cv_ellipse(47, 10, 2.0f, 2.0f, R_PURPLE, 0.7f, 1.0f);
    for (int k = 0; k < 10; k++)                                    /* the spell's last sparks about the hand */
        cv_px((int)(47 + (h01(k, 1, 353) - 0.5f) * 9), (int)(9 - 7 * h01(k, 2, 353)), sh(R_PURPLE, 0.85f + 0.15f * h01(k, 3, 353)));
    cv_ellipse(32, 12, 7.5f, 8.0f, R_PURPLE, 0.35f, 0.8f);          /* the hood, and only shadow in it */
    cv_flat(32, 13.5f, 4.8f, 5.6f, 0);
    cv_outline(0);
    cv_store(SP_DOUBLE);
}

/* The spectre's face (docs/InitialArtPass.md, section 8): long and drawn, the brow heavy over hollow sockets with a
 * light deep in each, the cheeks fallen in, the mouth dragged open in a wail down to a narrow chin. In the ghost's ramp. */
static const char *const SPECTRE_FACE[21] = {
    "..789999987..", ".79abbbbba97.", "69bcccdcccb96", "7acdde8eddca7", "7bdca7c7acdb7",
    "6ca317c713ac6", "5b2005b5002b5", "590016b610095", "470@16a61@074", "320148a841023",
    "3436938396343", "379ba575ab973", "2689462649862", "2478320238742", "1367300037631",
    ".35720002753.", ".34620002643.", ".24630003642.", "..464101464..", "..356303653..",
    "...4687864...",
};
/* and turned to the right: the brow and nose at the front, the jaw hanging open */
static const char *const SPECTRE_SIDE[21] = {
    "....789a.", "...79bcc.", "..79bcdd.", "..8bcdee.", "..9cdeed.",
    "..acdcbd9", "..bc7214.", "..bb20@1.", "..ab5125.", "..acb89b.",
    "..9cdcbc8", "..9bccba7", "..8ab964.", "..79720..", "..7850000",
    "..6840000", "..674000.", "..57510..", "..567423.", "...57876.",
    "....566..",
};

