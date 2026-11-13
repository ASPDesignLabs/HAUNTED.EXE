/* A part of crawl_art.c (docs/PLAN-tidy.md): it is #included there in its place and never compiled on its own.
 * The dressing that comes in as you go down: carcasses, armour stands, timber, mine lamps, carts, fungus, crystal, magma, obsidian, the altar. */

/* ------------------------------------------------------------------ */
/* The dressing that comes in as you go down (PLAN-bestiary, section 8) */
/* ------------------------------------------------------------------ */

/* a candle of bone-coloured wax, h tall, standing on (x, by), lit */
static void candle(float x, float by, float h, int frame, int k, int ramp) {
    for (int y = (int)(by - h); y <= (int)by; y++) { cv_px((int)x, y, sh(ramp, 0.6f)); cv_px((int)x + 1, y, sh(ramp, 0.4f)); }
    cv_px((int)x, (int)(by - h + 2), sh(ramp, 0.95f));
    flame(x + 1.0f, by - h - 1, 5.5f + (float)((frame + k) & 1), 1.7f, (((frame + k) % 3) - 1) * 0.7f);
}







/* a carcass picked nearly clean: a cage of ribs with dark meat still on it */
static void spr_carcass(void) {
    cv_begin(cv_sprite, TS, TS);
    cv_flat(32, 62, 26, 2.5f, R_WARM + 1);
    cv_ellipse(32, 58, 20, 4.0f, R_BLOOD, 0.05f, 0.2f);            /* what has soaked into the floor */
    cv_capsule(12, 54, 50, 52, 1.5f, R_BONE, 0.3f, 0.9f);           /* the spine */
    for (int i = 0; i < 7; i++) {                                   /* ribs arching up */
        float x = 18 + i * 4.5f, h = 10 - fabsf(i - 3.0f) * 1.5f;
        for (int k = 0; k <= 10; k++) {
            float t = k / 10.0f, xx = x + sinf(t * PI_F) * 3.0f, yy = 53 - sinf(t * PI_F * 0.95f) * h + t * 6.0f;
            cv_px((int)xx, (int)yy, sh(R_BONE, 0.85f - 0.3f * t));
            if (h01(i, k, 671) > 0.75f) cv_px((int)xx + 1, (int)yy, sh(R_BLOOD, 0.2f + 0.1f * h01(i, k, 673)));   /* shreds of meat */
        }
    }
    cv_ellipse(52, 52, 5, 4, R_BONE, 0.3f, 0.95f);                  /* the skull, a beast's */
    cv_capsule(52, 54, 59, 57, 1.8f, R_BONE, 0.25f, 0.85f);
    cv_flat(51, 51, 1.3f, 1.3f, 0);
    cv_capsule(16, 56, 6, 61, 1.3f, R_BONE, 0.25f, 0.8f); cv_capsule(48, 57, 40, 61, 1.2f, R_BONE, 0.25f, 0.8f);   /* legs, gnawed */
    cv_outline(0);
    cv_store(SP_CARCASS);
}

/* plate on a wooden stand, as the knights left it; and a great helm on the floor */
static void spr_armourstand(void) {
    cv_begin(cv_sprite, TS, TS);
    cv_flat(32, 62, 12, 1.6f, R_WARM + 1);
    cv_capsule(32, 10, 32, 61, 1.3f, R_WOOD, 0.15f, 0.6f);
    cv_capsule(24, 61, 40, 61, 1.3f, R_WOOD, 0.15f, 0.55f);
    cv_capsule(20, 21, 44, 21, 1.4f, R_WOOD, 0.15f, 0.6f);          /* the shoulders */
    for (int y = 20; y <= 40; y++) {                                /* the breastplate */
        float hw = 9.0f + 1.5f * sinf((y - 20) / 20.0f * PI_F) - (y > 34 ? (y - 34) * 0.7f : 0.0f);
        for (int x = (int)(32 - hw); x <= (int)(32 + hw); x++) {
            float nx = (x + 0.5f - 32) / hw;
            cv_px(x, y, sh(R_IRON, 0.25f + 0.6f * lit(nx, (y - 28) / 14.0f, sqrtf(fmaxf(0, 1 - nx * nx))) + (fabsf(x + 0.5f - 32) < 0.8f ? 0.2f : 0.0f) - 0.25f * (h01(x / 3, y / 3, 681) > 0.9f)));
        }
    }
    cv_ellipse(22, 21, 5, 4, R_IRON, 0.3f, 1.0f); cv_ellipse(42, 21, 5, 4, R_IRON, 0.3f, 1.0f);
    for (int y = 3; y <= 16; y++)                                   /* the helm on top */
        for (int x = 25; x <= 39; x++) {
            float nx = (x + 0.5f - 32) / 7.0f, top = 3 + 3.0f * nx * nx;
            if (fabsf(nx) > 1.0f || y < top) continue;
            float v = 0.25f + 0.6f * lit(nx, (y - 9) / 9.0f, sqrtf(fmaxf(0, 1 - nx * nx)));
            if (y == 9 || (y == 10 && fabsf(nx) < 0.85f)) v = 0.0f;
            cv_px(x, y, sh(R_IRON, v));
        }
    for (int i = 0; i < 6; i++) cv_line(26 + i * 2.4f, 1 + (i & 1), 27 + i * 2.4f, 4, sh(R_STONE, 0.75f));   /* cobweb strands from it */
    cv_outline(0);
    cv_store(SP_ARMOURSTAND);
}
static void spr_helm(void) {
    cv_begin(cv_sprite, TS, TS);
    cv_flat(32, 62, 12, 1.6f, R_WARM + 1);
    for (int y = 46; y <= 62; y++)
        for (int x = 23; x <= 41; x++) {
            float nx = (x + 0.5f - 32) / 8.5f, ny = (y + 0.5f - 54) / 8.0f;
            if (nx * nx + ny * ny > 1.0f) continue;
            float v = 0.25f + 0.65f * lit(nx, ny, sqrtf(fmaxf(0, 1 - nx * nx - ny * ny)));
            if (fabsf(x + 0.5f - 29.0f) < 0.9f && ny > -0.7f && ny < 0.5f) v = 0.0f;   /* the slit, on its side */
            if (h01(x / 2, y / 2, 683) > 0.88f) v -= 0.3f;           /* a dent */
            cv_px(x, y, sh(R_IRON, v));
        }
    cv_outline(0);
    cv_store(SP_HELM);
}

/* a mine prop: a post, a cap-beam across the whole hall, a brace, and the rock it holds up */
static void spr_timber(void) {
    cv_begin(cv_sprite, TS, TS);
    for (int x = 0; x < TS; x++) for (int y = 0; y < 3; y++) cv_px(x, y, sh(R_STONE, 0.15f + 0.25f * h01(x, y, 691)));
    cv_capsule(-2, 6, 66, 6, 3.2f, R_WOOD, 0.12f, 0.55f);          /* the cap */
    cv_capsule(32, 9, 32, 62, 3.0f, R_WOOD, 0.12f, 0.6f);           /* the post */
    cv_capsule(32, 22, 50, 8, 1.8f, R_WOOD, 0.12f, 0.5f);           /* a brace */
    for (int i = 0; i < 6; i++) cv_line(30, 14 + i * 8, 34, 15 + i * 8, sh(R_WOOD, 0.05f));   /* grain and splits */
    cv_ellipse(32, 6, 1.0f, 1.0f, R_IRON, 0.3f, 0.8f);              /* a spike in the joint */
    cv_outline(0);
    cv_store(SP_TIMBER);
}

/* a miner's lamp hung from a nail on a short post */
static void spr_minelamp(int id, int frame) {
    cv_begin(cv_sprite, TS, TS);
    cv_flat(32, 62, 8, 1.4f, R_WARM + 1);
    cv_capsule(32, 18, 32, 62, 2.2f, R_WOOD, 0.12f, 0.55f);
    cv_capsule(32, 22, 38, 22, 0.8f, R_IRON, 0.3f, 0.8f);           /* the nail */
    cv_line(38, 22, 38, 26, sh(R_IRON, 0.5f));                     /* the hook */
    cv_ellipse(38, 27, 3.0f, 1.4f, R_IRON, 0.25f, 0.8f);
    for (int y = 28; y < 36; y++)                                   /* the glass */
        for (int x = 35; x <= 41; x++) {
            float nx = (x + 0.5f - 38) / 3.5f;
            if (fabsf(nx) > 1.0f) continue;
            cv_px(x, y, fabsf(nx) > 0.75f ? sh(R_IRON, 0.4f) : (uint8_t)(R_FLAME + clampi((int)((1.0f - fabsf(nx) * 0.5f - fabsf(y - 32.0f) / 9.0f) * 13.0f) + frame, 3, 14)));
        }
    flame(38.5f, 33, 4.0f + frame, 1.2f, frame ? 0.4f : -0.4f);
    cv_ellipse(38, 36, 3.6f, 1.4f, R_IRON, 0.2f, 0.7f);
    cv_store(id);
}

/* a mine cart, heaped with ore: copper-green and iron-red in the broken rock */
static void spr_cart(void) {
    cv_begin(cv_sprite, TS, TS);
    cv_flat(32, 62, 26, 2.0f, R_WARM + 1);
    for (int i = 0; i < 16; i++) {                                  /* the ore, heaped over the rim */
        float x = 14 + 36 * h01(i, 1, 701), y = 34 - 6 * sinf((x - 14) / 36.0f * PI_F) + 4 * h01(i, 2, 701);
        int ramp = i % 5 == 0 ? R_MOSS : (i % 5 == 1 ? R_BRICK : R_STONE);
        cv_ellipse(x, y, 3.0f + 2.0f * h01(i, 3, 701), 2.4f + 1.4f * h01(i, 4, 701), ramp, 0.15f, ramp == R_STONE ? 0.7f : 0.8f);
    }
    for (int y = 34; y <= 52; y++)                                  /* the box: planks bound with iron */
        for (int x = 11; x <= 53; x++) {
            float lean = (y - 34) * 0.18f;
            if (x < 11 + lean || x > 53 - lean) continue;
            int band = x < 14 + lean || x > 50 - lean || y == 34 || y == 35 || y == 51 || y == 52 || (y > 41 && y < 44);
            float v = band ? 0.3f + 0.35f * (y < 36) : 0.12f + 0.2f * h01(x / 6, 1, 703) + 0.08f * (1.0f - (y - 34) / 18.0f);
            cv_px(x, y, band ? sh(R_IRON, v) : sh(R_WOOD, v - ((y - 34) % 6 == 0 ? 0.08f : 0.0f)));
        }
    for (int s = -1; s <= 1; s += 2) {                              /* wheels */
        cv_ellipse(32 + s * 13, 56, 6.0f, 6.0f, R_IRON, 0.15f, 0.7f);
        cv_ellipse(32 + s * 13, 56, 2.2f, 2.2f, R_IRON, 0.3f, 0.9f);
    }
    cv_outline(0);
    cv_store(SP_CART);
}

/* pale fungus in a cluster */
static void spr_fungus(void) {
    cv_begin(cv_sprite, TS, TS);
    cv_flat(32, 62, 18, 1.8f, R_WARM + 1);
    static const float M[7][4] = { {22, 9, 5.5f, 0.9f}, {30, 15, 7.0f, 1.0f}, {39, 10, 5.0f, 0.85f}, {45, 6, 3.5f, 0.8f}, {17, 5, 3.0f, 0.75f}, {34, 5, 3.0f, 0.8f}, {27, 7, 3.5f, 0.85f} };
    for (int i = 0; i < 7; i++) {
        float x = M[i][0], h = M[i][1], r = M[i][2];
        cv_capsule(x, 62, x + (i & 1 ? 0.8f : -0.8f), 62 - h, r * 0.22f + 0.6f, R_BONE, 0.3f, M[i][3]);
        cv_ellipse(x + (i & 1 ? 0.8f : -0.8f), 62 - h, r, r * 0.5f, R_BONE, 0.3f, M[i][3]);
        for (int k = -1; k <= 1; k++) cv_px((int)(x + k * r * 0.5f), (int)(62 - h + r * 0.35f), sh(R_WARM, 0.25f));   /* gills */
        for (int k = 0; k < 3; k++) cv_px((int)(x - r * 0.4f + k * r * 0.4f), (int)(62 - h - r * 0.2f), sh(R_WARM, 0.45f));   /* spots */
    }
    cv_outline(0);
    cv_store(SP_FUNGUS);
}

/* a cluster of crystals glowing by themselves, a little brighter on the second frame */
static void spr_crystal(int id, int frame) {
    cv_begin(cv_sprite, TS, TS);
    cv_flat(32, 62, 20, 2.0f, R_WARM + 1);
    for (int i = 0; i < 7; i++) {
        float bx = 18 + 28 * h01(i, 1, 711), a = (h01(i, 2, 711) - 0.5f) * 0.9f, len = 14 + 30 * h01(i, 3, 711) * (i == 3 ? 1.4f : 1.0f), w = 2.5f + 2.5f * h01(i, 4, 711);
        float dx = sinf(a), dy = -cosf(a);
        for (float t = 0; t < len; t += 0.5f) {
            float hw = t > len - w * 1.6f ? w * (len - t) / (w * 1.6f) : w;
            for (float o = -hw; o <= hw; o += 0.5f) {
                float u = o / (w + 0.01f);
                int glow = clampi((int)(4 + 8 * (1.0f - fabsf(u)) * (0.4f + 0.6f * t / len) + (u < -0.2f ? 2 : 0)) + frame, 2, 15);
                cv_px((int)(bx + dx * t - dy * o), (int)(62 + dy * t + dx * o), (uint8_t)(R_GLOW + glow));
            }
        }
    }
    for (int i = 0; i < 5; i++) cv_ellipse(16 + 32 * h01(i, 5, 713), 61, 3.0f, 2.0f, R_IRON, 0.1f, 0.4f);   /* the rock they grow out of */
    cv_store(id);
}

/* a vent of molten rock: a crust of black stone split over the fire, a pool of it bubbling on top */
static void spr_magma(int id, int frame) {
    cv_begin(cv_sprite, TS, TS);
    for (int i = 0; i < 3; i++) {
        float x = 26 + i * 6 + (h01(i, frame, 721) - 0.5f) * 3;
        flame(x, 42, 8 + 8 * h01(i, frame + 3, 721), 3.0f, (h01(i, frame + 6, 721) - 0.5f) * 3.0f);
    }
    for (int y = 40; y < 63; y++)                                   /* the cone of crust */
        for (int x = 6; x <= 58; x++) {
            float hw = 9.0f + (y - 40) * 1.1f, nx = (x + 0.5f - 32) / hw;
            if (fabsf(nx) > 1.0f) continue;
            float n = fbm(x / 4.0f, y / 4.0f, 16, 723);
            uint8_t c = sh(R_IRON, 0.08f + 0.3f * lit(nx, -0.3f, sqrtf(1 - nx * nx)) + 0.1f * n);
            float crack = fabsf(fbm(x / 6.0f, y / 3.0f, 16, 725 + frame) - 0.5f);
            if (crack < 0.035f + 0.01f * frame) c = (uint8_t)(R_FLAME + clampi((int)(12 - crack * 150), 4, 14));
            cv_px(x, y, c);
        }
    cv_flat(32, 41, 9, 2.2f, (uint8_t)(R_FLAME + 10));              /* the pool at the top */
    for (int i = 0; i < 5; i++) cv_px((int)(25 + 14 * h01(i, frame, 727)), 40 + (int)(2 * h01(i, frame + 1, 727)), (uint8_t)(R_FLAME + 15));
    cv_store(id);
}

/* shards of black glass, standing up from the floor, edges catching the light */
static void spr_obsidian(void) {
    cv_begin(cv_sprite, TS, TS);
    cv_flat(32, 62, 18, 2.0f, R_WARM + 1);
    for (int i = 0; i < 6; i++) {
        float bx = 20 + 24 * h01(i, 1, 731), top = 22 + 30 * h01(i, 2, 731), w = 3 + 4 * h01(i, 3, 731), lean = (h01(i, 4, 731) - 0.5f) * 10;
        float p[6] = { bx - w, 62, bx + w, 62, bx + lean, top };
        cv_poly(p, 3, R_IRON, 0.06f, 0.32f, top, 62 - top);
        cv_line(bx + w * 0.3f, 61, bx + lean, top, sh(R_GHOST, 0.7f));   /* the glassy edges */
        cv_line(bx - w * 0.7f, 61, bx + lean, top, sh(R_PURPLE, 0.35f));
        cv_px((int)(bx + lean), (int)top, sh(R_GHOST, 0.95f));
    }
    cv_outline(0);
    cv_store(SP_OBSIDIAN);
}

/* an altar: a block of stone under a dark cloth, a skull, black candles burning */
static void spr_altar(int id, int frame) {
    cv_begin(cv_sprite, TS, TS);
    cv_flat(32, 62, 24, 2.0f, R_WARM + 1);
    for (int y = 40; y < 63; y++)
        for (int x = 9; x <= 55; x++) stone_px(x, y, (x + 0.5f - 32) / 23.0f, y < 42 ? 0.15f : 0.0f, 741);
    for (int y = 39; y < 54; y++)                                   /* the cloth over it, hanging to a fringe */
        for (int x = 14; x <= 50; x++) {
            if (y > 50 && (x & 1)) continue;
            cv_px(x, y, sh(R_BLOOD, 0.08f + 0.12f * (y < 41) + 0.04f * sinf(x * 0.9f)));
        }
    cv_skull(32, 34, 0.8f, 0.3f, 0.95f);
    static const float C[4][2] = { {16, 10}, {22, 14}, {42, 12}, {48, 8} };
    for (int i = 0; i < 4; i++) candle(C[i][0], 39, C[i][1], frame, i, R_IRON);
    cv_outline(0);
    cv_store(id);
}

