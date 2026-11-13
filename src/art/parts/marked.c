/* A part of crawl_art.c (docs/PLAN-tidy.md): it is #included there in its place and never compiled on its own.
 * Things that want a spell: run I's and one for each line, the moving and timed ones, sequences and things for fights; crawl_mark (the seal and its motes). */

/* ---- Things that want a spell (1.0.0b16, docs/PLAN-play.md section 4): run I's, and one for each line ---- */
/* a pricket stand of black iron with a thick candle, cold or burning (fire) */
static void spr_mk_pricket(int id, int on) {
    cv_begin(cv_sprite, TS, TS);
    cv_flat(32, 62, 9, 1.6f, R_WARM + 1);
    cv_capsule(32, 55, 25, 62, 1.3f, R_IRON, 0.04f, 0.28f); cv_capsule(32, 55, 39, 62, 1.3f, R_IRON, 0.04f, 0.28f);   /* three feet */
    cv_capsule(32, 56, 32, 62, 1.1f, R_IRON, 0.04f, 0.22f);
    cv_capsule(32, 26, 32, 56, 1.5f, R_IRON, 0.05f, 0.32f);         /* the stem, a knop halfway */
    cv_ellipse(32, 41, 2.8f, 1.9f, R_IRON, 0.08f, 0.4f);
    cv_flat(32, 26, 7.5f, 1.8f, sh(R_IRON, 0.18f));                 /* the drip-pan */
    for (int x = 25; x <= 39; x++) cv_px(x, 25, sh(R_IRON, 0.38f + 0.02f * (x - 25)));
    for (int y = 13; y <= 25; y++)                                   /* the candle, tallow-grey, guttered down one side */
        for (int x = 29; x <= 35; x++) cv_px(x, y, sh(R_BONE, 0.45f + 0.35f * (x < 32) - 0.12f * (x == 35)));
    for (int y = 19; y <= 26; y++) cv_px(28, y, sh(R_BONE, 0.55f));
    cv_px(28, 27, sh(R_BONE, 0.4f));
    cv_line(32, 11, 32, 13, sh(R_STONE, 0.15f));                     /* the wick */
    if (on) {
        cv_ellipse(32, 7.5f, 2.3f, 4.5f, R_FLAME, 0.55f, 1.0f);
        cv_flat(32, 9, 1.0f, 1.8f, (uint8_t)(R_FLAME + 15));
    }
    cv_outline(0);
    cv_store(id);
}
/* a coffer of black oak bound in three chains and a lock; done, the chains fallen and the lid up on gold */
static void spr_mk_coffer(int id, int open) {
    cv_begin(cv_sprite, TS, TS);
    cv_flat(32, 62, 20, 2.0f, R_WARM + 1);
    for (int y = 42; y <= 61; y++)                                   /* the box */
        for (int x = 15; x <= 49; x++) {
            int plank = (y - 42) / 5;
            float v = 0.24f + 0.08f * h01(plank, 1, 611) + 0.1f * fbm((float)x / 7.0f, (float)y / 1.4f, 16, 613) - (y == 42 + plank * 5 ? 0.12f : 0.0f) - 0.08f * (x > 46);
            uint8_t c = sh(R_WOOD, v);
            if (x <= 16 || x >= 48 || y >= 60) c = sh(R_IRON, 0.32f - 0.1f * (x >= 48));
            cv_px(x, y, c);
        }
    if (!open) {
        float lid[8] = { 14, 42, 50, 42, 47, 33, 17, 33 };               /* the lid, shut, curved */
        cv_poly(lid, 4, R_WOOD, 0.42f, 0.2f, 33, 9);
        for (int x = 15; x <= 49; x++) cv_px(x, 42, sh(R_IRON, 0.5f));
        static const float CH[3][4] = { { 21, 31, 19, 62 }, { 32, 31, 32, 62 }, { 43, 31, 45, 62 } };   /* the chains, over lid and box */
        for (int c = 0; c < 3; c++)
            for (int k = 0; k < 16; k++) {
                float u = (float)k / 15.0f, x = CH[c][0] + (CH[c][2] - CH[c][0]) * u, y = CH[c][1] + (CH[c][3] - CH[c][1]) * u;
                if (k & 1) cv_flat(x, y, 1.6f, 1.0f, sh(R_IRON, 0.62f)); else cv_flat(x, y, 1.0f, 1.6f, sh(R_IRON, 0.48f));
            }
        cv_capsule(28, 44, 36, 44, 3.0f, R_IRON, 0.25f, 0.75f);          /* the lock, heavy */
        cv_flat(32, 45, 1.0f, 1.6f, sh(R_STONE, 0.02f));
    } else {
        for (int y = 30; y <= 41; y++)                                   /* the lid, thrown back: its inside, planks and two straps */
            for (int x = 15; x <= 49; x++) {
                float u = (float)(y - 30) / 11.0f;
                if (x < 17 - 2 * u || x > 47 + 2 * u) continue;
                int strap = abs(x - 22) <= 1 || abs(x - 42) <= 1;
                cv_px(x, y, strap ? sh(R_IRON, 0.35f + 0.2f * (x == 21 || x == 41)) : sh(R_WOOD, 0.2f + 0.08f * h01(x, y, 619) + 0.1f * u - ((y - 30) % 4 == 0 ? 0.06f : 0.0f)));
            }
        for (int x = 17; x <= 47; x++) cv_px(x, 30, sh(R_IRON, 0.6f));
        for (int i = 0; i < 22; i++) cv_flat(18 + h01(i, 3, 617) * 28, 41.5f - h01(i, 4, 617) * 3, 2.2f, 1.2f, sh(R_GOLD, 0.5f + 0.45f * h01(i, 5, 617)));   /* gold, heaped to the brim */
        for (int k = 0; k < 3; k++)                                      /* the chains, in a heap at its foot */
            for (int j = 0; j < 7; j++) cv_flat(14 + k * 15 + j * 2.2f, 61.5f - (j & 1), 1.3f, 0.9f, sh(R_IRON, 0.4f + 0.2f * (j & 1)));
        cv_capsule(48, 60, 54, 60, 2.2f, R_IRON, 0.2f, 0.6f);            /* the lock, broken off */
    }
    cv_outline(0);
    cv_store(id);
}
/* a gibbet cage on a chain from the roof, a hand through its bars and a glint inside; done, fallen and burst */
static void spr_mk_cage(int id, int down) {
    cv_begin(cv_sprite, TS, TS);
    float top = down ? 40.0f : 10.0f, bot = down ? 62.0f : 40.0f;
    if (!down) for (int y = 0; y < 10; y += 2) cv_flat(32, (float)y + 1, (y & 2) ? 1.5f : 0.9f, (y & 2) ? 0.9f : 1.5f, sh(R_IRON, 0.5f));   /* its chain */
    else cv_flat(32, 62, 14, 1.8f, R_WARM + 1);
    for (int b = 0; b < 7; b++) {                                    /* the bars, bellied */
        float u = (float)b / 6.0f, x = 22 + 20 * u;
        if (down && (b == 2 || b == 3)) {                              /* sprung where it hit */
            cv_line(x, top + 4, x - 6 + 4 * b - 8, bot, sh(R_IRON, 0.45f));
            continue;
        }
        float bulge = 3.0f * sinf(u * PI_F);
        cv_line(x, top + 2, x + (u - 0.5f) * bulge, (top + bot) * 0.5f, sh(R_IRON, 0.35f + 0.25f * (b < 3)));
        cv_line(x + (u - 0.5f) * bulge, (top + bot) * 0.5f, x, bot, sh(R_IRON, 0.35f + 0.25f * (b < 3)));
    }
    for (int x = 21; x <= 43; x++) { cv_px(x, (int)top + 2, sh(R_IRON, 0.55f)); cv_px(x, (int)bot, sh(R_IRON, 0.4f)); }
    cv_ellipse(32, top + 1, 4, 2, R_IRON, 0.2f, 0.6f);                 /* its cap and ring */
    if (!down) {
        cv_flat(31, bot - 3, 4, 2, sh(R_GOLD, 0.8f)); cv_px(30, (int)bot - 4, (uint8_t)(R_GOLD + 12));   /* something worth having */
        cv_bone(24, 28, 15, 33, 0.9f, 0.4f, 0.8f); cv_bone(15, 33, 13, 37, 0.7f, 0.4f, 0.8f);          /* a hand out between the bars */
        for (int f = 0; f < 3; f++) cv_line(13, 37, 11 + f * 1.5f, 40, sh(R_BONE, 0.6f));
    } else {
        for (int i = 0; i < 10; i++) cv_flat(18 + h01(i, 1, 621) * 30, 58 + h01(i, 2, 621) * 4, 2.0f, 1.0f, sh(R_GOLD, 0.45f + 0.5f * h01(i, 3, 621)));
        cv_skull(44, 58, 0.55f, 0.2f, 0.7f);
    }
    cv_outline(0);
    cv_store(id);
}
/* a votive rack: three tiers of an oak frame, a dozen candles, dead or lit (fire) */
static void spr_mk_rack(int id, int on) {
    cv_begin(cv_sprite, TS, TS);
    cv_flat(32, 62, 20, 1.8f, R_WARM + 1);
    for (int t = 0; t < 3; t++) {
        float y = 58 - t * 11.0f, x0 = 14 + t * 4.0f, x1 = 50 - t * 4.0f;
        for (int x = (int)x0; x <= (int)x1; x++) { cv_px(x, (int)y, sh(R_WOOD, 0.42f)); cv_px(x, (int)y + 1, sh(R_WOOD, 0.22f)); cv_px(x, (int)y + 2, sh(R_WOOD, 0.12f)); }
        int n = 5 - t;
        for (int k = 0; k < n; k++) {
            float x = x0 + 3 + (x1 - x0 - 6) * (float)k / (float)(n - 1 > 0 ? n - 1 : 1), hgt = 4 + 3 * h01(k, t, 623);
            for (int yy = (int)(y - hgt); yy < (int)y; yy++) { cv_px((int)x, yy, sh(R_BONE, 0.7f)); cv_px((int)x + 1, yy, sh(R_BONE, 0.45f)); }
            if (on) { cv_px((int)x, (int)(y - hgt) - 1, (uint8_t)(R_FLAME + 15)); cv_px((int)x, (int)(y - hgt) - 2, (uint8_t)(R_FLAME + 11)); cv_px((int)x, (int)(y - hgt) - 3, (uint8_t)(R_FLAME + 7)); }
            else cv_px((int)x, (int)(y - hgt) - 1, sh(R_STONE, 0.1f));
        }
    }
    cv_capsule(13, 38, 13, 62, 1.0f, R_WOOD, 0.1f, 0.4f); cv_capsule(51, 38, 51, 62, 1.0f, R_WOOD, 0.1f, 0.4f);   /* its uprights */
    cv_outline(0);
    cv_store(id);
}
/* the keeper's urn: a bronze urn on an iron tripod, full of coals that never go out; quenched, rimed grey (frost) */
static void spr_mk_urn(int id, int out) {
    cv_begin(cv_sprite, TS, TS);
    cv_flat(32, 62, 14, 1.8f, R_WARM + 1);
    cv_capsule(32, 50, 21, 62, 1.2f, R_IRON, 0.05f, 0.3f); cv_capsule(32, 50, 43, 62, 1.2f, R_IRON, 0.05f, 0.3f); cv_capsule(32, 50, 32, 62, 1.0f, R_IRON, 0.05f, 0.24f);
    for (int y = 30; y <= 50; y++) {                                 /* the urn: a wide belly, a narrow foot */
        float u = (float)(y - 30) / 20.0f, w = 4 + 9.5f * sinf(u * PI_F * 0.9f + 0.25f);
        for (int x = (int)(32 - w); x <= (int)(32 + w); x++) {
            float nx = (x + 0.5f - 32) / (w + 0.01f);
            cv_px(x, y, sh(R_WARM, 0.18f + 0.5f * lit(nx, -0.2f, sqrtf(fmaxf(0.0f, 1.0f - nx * nx)))));
        }
    }
    for (int x = 20; x <= 44; x++) cv_px(x, 30, sh(R_WARM, 0.6f));    /* its lip */
    for (int i = 0; i < 26; i++) {                                    /* the coals heaped in it */
        float x = 22 + h01(i, 1, 627) * 20, y = 29 - h01(i, 2, 627) * 5 * (1.0f - fabsf(x - 32) / 11);
        if (out) cv_flat(x, y, 1.6f, 1.1f, i % 4 ? sh(R_STONE, 0.15f + 0.1f * h01(i, 3, 627)) : sh(R_GHOST, 0.85f));
        else cv_flat(x, y, 1.6f, 1.1f, (uint8_t)(i % 3 ? R_FLAME + 6 + (i % 5) : R_FLAME + 12));
    }
    if (!out) for (int k = 0; k < 4; k++) cv_px((int)(26 + k * 4), (int)(20 - 2 * (k & 1)), (uint8_t)(R_FLAME + 9));   /* sparks rising */
    else for (int k = 0; k < 9; k++) cv_px((int)(21 + h01(k, 5, 629) * 22), (int)(31 + h01(k, 6, 629) * 14), sh(R_GHOST, 0.95f));   /* rime on the bronze */
    cv_outline(0);
    cv_store(id);
}
/* a galvanic coil: an iron frame, a copper winding round a core, a lamp-globe on top, dead or burning gold (lightning) */
static void spr_mk_coil(int id, int on) {
    cv_begin(cv_sprite, TS, TS);
    cv_flat(32, 62, 16, 1.8f, R_WARM + 1);
    for (int y = 56; y <= 62; y++) for (int x = 18; x <= 46; x++) cv_px(x, y, sh(R_STONE, 0.3f + 0.12f * (y == 56)));   /* its plinth */
    cv_capsule(21, 24, 21, 56, 1.3f, R_IRON, 0.05f, 0.35f); cv_capsule(43, 24, 43, 56, 1.3f, R_IRON, 0.05f, 0.35f);   /* the frame */
    cv_capsule(21, 24, 43, 24, 1.3f, R_IRON, 0.05f, 0.35f);
    cv_capsule(32, 28, 32, 54, 3.0f, R_IRON, 0.1f, 0.4f);              /* the core */
    for (int y = 30; y <= 52; y += 2)                                  /* the winding, copper */
        for (int x = 27; x <= 37; x++) cv_px(x, y + ((x + y) & 1), sh(R_WARM, 0.35f + 0.4f * (x < 31)));
    cv_capsule(32, 16, 32, 24, 0.9f, R_IRON, 0.1f, 0.4f);
    cv_ellipse(32, 12, 5, 5, on ? R_GOLD : R_STONE, on ? 0.6f : 0.12f, on ? 1.0f : 0.4f);   /* the globe */
    if (on) {
        cv_flat(32, 12, 1.6f, 1.6f, C_WHITE);
        static const float ARC[5][4] = { { 21, 30, 27, 33 }, { 43, 36, 37, 40 }, { 21, 46, 27, 44 }, { 27, 12, 22, 8 }, { 37, 12, 43, 6 } };
        for (int k = 0; k < 5; k++) { cv_line(ARC[k][0], ARC[k][1], (ARC[k][0] + ARC[k][2]) * 0.5f + 1, (ARC[k][1] + ARC[k][3]) * 0.5f - 1, (uint8_t)(R_GOLD + 12)); cv_line((ARC[k][0] + ARC[k][2]) * 0.5f + 1, (ARC[k][1] + ARC[k][3]) * 0.5f - 1, ARC[k][2], ARC[k][3], (uint8_t)(R_GOLD + 9)); }
    }
    cv_outline(0);
    cv_store(id);
}
/* roots grown over a small stone box in a niche, green and fat; done, withered black and fallen back (poison) */
static void spr_mk_vine(int id, int dead) {
    cv_begin(cv_sprite, TS, TS);
    cv_flat(32, 62, 18, 1.8f, R_WARM + 1);
    for (int y = 44; y <= 61; y++) for (int x = 22; x <= 42; x++) cv_px(x, y, sh(R_STONE, 0.28f + 0.14f * (y == 44) - 0.08f * (x > 39)));   /* the box */
    for (int x = 22; x <= 42; x++) cv_px(x, 52, sh(R_STONE, 0.12f));
    if (dead) cv_flat(32, 49, 2.5f, 1.4f, sh(R_GOLD, 0.8f));             /* what it kept, showing */
    int n = dead ? 9 : 16;
    for (int r = 0; r < n; r++) {                                    /* the roots: down from above, round the box, into the floor */
        float x0 = 14 + h01(r, 1, 631) * 36, y0 = dead ? 40 + h01(r, 2, 631) * 8 : 6 + h01(r, 2, 631) * 12;
        float x = x0, y = y0;
        for (int s = 0; s < 40; s++) {
            float nx = x + (h01(r, s, 633) - 0.5f) * 2.2f + (x < 32 ? 0.12f : -0.12f) * (dead ? -1 : 1), ny = y + (dead ? 0.6f : 1.3f);
            if (ny > 62) break;
            uint8_t c = dead ? sh(R_WOOD, 0.08f + 0.06f * (s & 1)) : (s % 7 == 3 ? sh(R_MOSS, 0.75f) : sh(R_MOSS, 0.3f + 0.15f * h01(r, s, 635)));
            cv_capsule(x, y, nx, ny, dead ? 0.6f : 1.1f - 0.4f * (float)s / 40.0f, dead ? R_WOOD : R_MOSS, dead ? 0.04f : 0.2f, dead ? 0.18f : 0.6f);
            if (!dead && s % 9 == 4) cv_px((int)nx + 1, (int)ny, c);
            x = nx; y = ny;
        }
    }
    if (!dead) for (int k = 0; k < 6; k++) cv_flat(18 + h01(k, 7, 637) * 28, 18 + h01(k, 8, 637) * 30, 1.4f, 1.1f, sh(R_MOSS, 0.85f));   /* buds */
    cv_outline(0);
    cv_store(id);
}
/* a soul in a lantern on an iron post, a face in a violet smoke pressing at the glass; done, the door open and dark (spirit) */
static void spr_mk_lantern(int id, int freed) {
    cv_begin(cv_sprite, TS, TS);
    cv_flat(32, 62, 8, 1.6f, R_WARM + 1);
    cv_capsule(26, 10, 26, 62, 1.4f, R_IRON, 0.05f, 0.32f);            /* the post and its arm */
    cv_capsule(26, 12, 38, 12, 1.0f, R_IRON, 0.05f, 0.32f);
    cv_capsule(38, 12, 38, 18, 0.7f, R_IRON, 0.1f, 0.4f);
    for (int y = 20; y <= 40; y++)                                     /* the lantern: a frame round dark glass */
        for (int x = 31; x <= 45; x++) {
            int frame = x == 31 || x == 45 || y == 20 || y == 40 || x == 38;
            cv_px(x, y, frame ? sh(R_IRON, 0.38f + 0.2f * (x == 31)) : sh(R_GHOST, freed ? 0.08f : 0.18f));
        }
    for (int x = 30; x <= 46; x++) { cv_px(x, 19, sh(R_IRON, 0.5f)); cv_px(x, 41, sh(R_IRON, 0.4f)); }
    cv_ellipse(38, 17, 5, 2, R_IRON, 0.15f, 0.5f);
    if (!freed) {
        for (int i = 0; i < 30; i++) { float a = i * 0.7f, r = 1.0f + 0.22f * i; cv_px((int)(38 + cosf(a) * r * 0.9f), (int)(30 + sinf(a) * r * 1.1f), (uint8_t)(R_PURPLE + 8 + (i % 5))); }
        cv_px(36, 28, (uint8_t)(R_GLOW + 14)); cv_px(40, 28, (uint8_t)(R_GLOW + 14));   /* its eyes */
        for (int x = 36; x <= 40; x++) cv_px(x, 33, (uint8_t)(R_PURPLE + 5));            /* its mouth, open */
    } else {
        for (int y = 21; y <= 39; y++) cv_px(46 + (y - 21) / 4, y, sh(R_IRON, 0.35f));    /* the door hanging open */
    }
    cv_outline(0);
    cv_store(id);
}
/* the Order's dead heaped across a way, skulls in among the long bones, to a man's height; done, brought down (physical) */
static void spr_mk_barricade(int id, int down) {
    cv_begin(cv_sprite, TS, TS);
    cv_flat(32, 62, 28, 2.0f, R_WARM + 1);
    int n = down ? 26 : 46;
    float H = down ? 12.0f : 44.0f;
    for (int i = 0; i < n; i++) {
        float y = 61 - h01(i, 1, 641) * H * (1.0f - 0.3f * h01(i, 9, 641)), x = 6 + h01(i, 2, 641) * 52;
        if (!down && fabsf(x - 32) > 24 - (61 - y) * 0.25f) continue;
        float a = h01(i, 3, 641) * PI_F, l = 6 + 5 * h01(i, 4, 641);
        cv_bone(x - cosf(a) * l, y - sinf(a) * l * 0.4f, x + cosf(a) * l, y + sinf(a) * l * 0.4f, 0.9f, 0.25f, 0.75f);
    }
    for (int i = 0; i < (down ? 4 : 9); i++) cv_skull(10 + h01(i, 5, 643) * 44, 61 - h01(i, 6, 643) * H * 0.9f, 0.6f, 0.2f, 0.8f);
    if (!down) cv_capsule(8, 22, 56, 26, 1.4f, R_WOOD, 0.08f, 0.35f);  /* a beam across it */
    cv_outline(0);
    cv_store(id);
}

/* ---- step 6's things (docs/PLAN-play.md section 4.3 and 4.6): moving, timed, wanting two spells or more ---- */
/* a corpse-light: a pale drifting light with a face in it, a tail of itself behind; two frames */
static void spr_mk_corpselight(int id, int f) {
    cv_begin(cv_sprite, TS, TS);
    for (int k = 0; k < 14; k++) {                                  /* the tail, thinning */
        float u = (float)k / 13.0f, x = 32 + 13 * u + 3 * sinf(u * 5 + f * 1.7f), y = 30 + 7 * u - 3 * cosf(u * 4 + f);
        cv_flat(x, y, 3.2f * (1 - u) + 0.5f, 2.6f * (1 - u) + 0.5f, (uint8_t)(R_GHOST + 9 - (int)(6 * u)));
    }
    for (int y = 20; y <= 40; y++)                                  /* the halo, dithered */
        for (int x = 22; x <= 42; x++) {
            float d = hypotf((float)x - 32, (float)y - 30);
            if (d < 10 && d > 6 && ((x + y + f) & 1)) cv_px(x, y, (uint8_t)(R_GHOST + 7));
        }
    cv_ellipse(32, 30, 6, 6, R_GHOST, 0.6f, 1.0f);
    cv_flat(32, 30, 3, 3, C_WHITE);
    cv_px(30, 29, (uint8_t)(R_GHOST + 4)); cv_px(34, 29, (uint8_t)(R_GHOST + 4));   /* a face, hardly */
    cv_px(31 + f, 32, (uint8_t)(R_GHOST + 5)); cv_px(32 + f, 32, (uint8_t)(R_GHOST + 5));
    cv_store(id);
}
/* a skull rolling, two turns of it */
static void spr_mk_rollskull(int id, int f) {
    cv_begin(cv_sprite, TS, TS);
    cv_flat(32, 62, 7, 1.4f, R_WARM + 1);
    cv_skull(32, 54, 1.05f, 0.25f, 0.85f);
    if (f) {                                                       /* half a turn on: the crown towards you, the jaw under */
        for (int y = 47; y <= 60; y++) for (int x = 25; x <= 39; x++) {
            float d = hypotf((float)x - 32, ((float)y - 54) * 1.1f);
            if (d < 7) cv_px(x, y, sh(R_BONE, 0.35f + 0.45f * lit((x - 32) / 7.0f, (y - 54) / 7.0f, sqrtf(fmaxf(0, 1 - d * d / 49)))));
        }
        cv_line(28, 50, 33, 57, sh(R_BONE, 0.2f)); cv_line(33, 57, 36, 52, sh(R_BONE, 0.2f));   /* a suture */
    }
    for (int i = 0; i < 4; i++) cv_px(20 - i * 3, 58 + (i & 1), sh(R_STONE, 0.4f));   /* grit kicked up behind */
    cv_outline(0);
    cv_store(id);
}
/* a seal of old stone in a ring of iron over a niche: cold; hot from fire (its cracks glowing); broken by frost while hot */
static void spr_mk_heatseal(int id, int st) {
    cv_begin(cv_sprite, TS, TS);
    cv_flat(32, 62, 18, 1.8f, R_WARM + 1);
    for (int y = 30; y <= 62; y++) for (int x = 14; x <= 50; x++) cv_px(x, y, sh(R_STONE, 0.2f + 0.1f * (x < 17) - 0.08f * (x > 47) + 0.06f * h01(x, y, 651)));   /* its frame */
    if (st < 2) {
        for (int y = 33; y <= 61; y++)
            for (int x = 17; x <= 47; x++) {
                float d = hypotf((float)x - 32, (float)y - 47);
                if (d > 14.5f) continue;
                uint8_t c = d > 13 ? sh(R_IRON, 0.5f + 0.3f * (x < 32)) : sh(R_STONE, 0.35f + 0.25f * lit((x - 32) / 14.0f, (y - 47) / 14.0f, sqrtf(fmaxf(0, 1 - d * d / 196))));
                if (st == 1 && d <= 13) c = sh(R_BRICK, 0.4f + 0.35f * lit((x - 32) / 14.0f, (y - 47) / 14.0f, sqrtf(fmaxf(0, 1 - d * d / 196))));   /* hot */
                cv_px(x, y, c);
            }
        int cx = 26;                                               /* its cracks */
        for (int y = 36; y <= 58; y++) { cx += (int)(h01(y, 1, 653) * 3) - 1; cv_px(cx, y, st == 1 ? (uint8_t)(R_FLAME + 12) : sh(R_STONE, 0.05f)); }
        for (int x = 30; x <= 44; x++) { int y = 46 + (int)(3 * sinf(x * 0.7f)); cv_px(x, y, st == 1 ? (uint8_t)(R_FLAME + 10) : sh(R_STONE, 0.06f)); }
    } else {
        for (int y = 33; y <= 61; y++) for (int x = 18; x <= 46; x++) if (hypotf((float)x - 32, (float)y - 47) < 13) cv_px(x, y, sh(R_STONE, 0.02f));   /* the niche, open */
        cv_flat(32, 55, 3, 1.5f, sh(R_GOLD, 0.85f)); cv_px(31, 54, (uint8_t)(R_GOLD + 12));
        for (int i = 0; i < 14; i++) cv_flat(10 + h01(i, 2, 655) * 44, 59 + h01(i, 3, 655) * 4, 2.2f, 1.1f, sh(R_STONE, 0.3f + 0.3f * h01(i, 4, 655)));   /* its shards */
        for (int i = 0; i < 6; i++) cv_px((int)(16 + h01(i, 5, 655) * 32), (int)(56 + h01(i, 6, 655) * 6), sh(R_GHOST, 0.9f));   /* rime on them */
    }
    cv_outline(0);
    cv_store(id);
}
/* a small bell on a bracket from the roof: still, and swung ringing */
static void spr_mk_bell(int id, int ring) {
    cv_begin(cv_sprite, TS, TS);
    cv_capsule(26, 0, 38, 0, 1.2f, R_IRON, 0.1f, 0.4f);
    cv_line(32, 1, 32, 8, sh(R_IRON, 0.45f));
    float tilt = ring ? 0.35f : 0.0f;
    for (int y = 8; y <= 26; y++) {
        float u = (float)(y - 8) / 18.0f, w = 3 + 3 * u + 4 * u * u * u, cx = 32 + tilt * (y - 8);
        for (int x = (int)(cx - w); x <= (int)(cx + w); x++) {
            float nx = (x + 0.5f - cx) / (w + 0.01f);
            cv_px(x, y, sh(R_WARM, (y >= 25 ? 0.5f : 0.25f) + 0.5f * lit(nx, -0.2f, sqrtf(fmaxf(0, 1 - nx * nx)))));
        }
    }
    cv_flat(32 + tilt * 20 + (ring ? 2 : 0), 28, 1.4f, 1.4f, sh(R_IRON, 0.3f));
    if (ring) for (int k = 0; k < 3; k++) { cv_px(46 + k * 3, 14 + k * 2, sh(R_BONE, 0.8f)); cv_px(47 + k * 3, 18 + k * 2, sh(R_BONE, 0.6f)); cv_px(18 - k * 3, 16 + k * 2, sh(R_BONE, 0.7f)); }
    cv_outline(0);
    cv_store(id);
}
/* a winch on an iron frame, its drum wound in copper, a chain going up out of sight; woken by lightning it turns, sparking */
static void spr_mk_winch(int id, int on) {
    cv_begin(cv_sprite, TS, TS);
    cv_flat(32, 62, 16, 1.8f, R_WARM + 1);
    cv_capsule(18, 30, 18, 62, 1.6f, R_IRON, 0.05f, 0.35f); cv_capsule(46, 30, 46, 62, 1.6f, R_IRON, 0.05f, 0.35f);   /* its stands */
    for (int y = 34; y <= 48; y++)                                  /* the drum, copper-wound */
        for (int x = 20; x <= 44; x++) cv_px(x, y, ((x + (on ? y / 2 : 0)) & 1) ? sh(R_WARM, 0.3f + 0.4f * (y < 39)) : sh(R_WARM, 0.15f + 0.3f * (y < 39)));
    cv_capsule(20, 34, 20, 48, 1.5f, R_IRON, 0.1f, 0.5f); cv_capsule(44, 34, 44, 48, 1.5f, R_IRON, 0.1f, 0.5f);
    for (int y = 0; y < 34; y += 2) cv_flat(30, (float)y + 1, (y & 2) ? 1.4f : 0.8f, (y & 2) ? 0.8f : 1.4f, sh(R_IRON, on ? 0.6f : 0.4f));   /* the chain */
    cv_capsule(46, 41, 54, 36, 1.0f, R_IRON, 0.1f, 0.4f); cv_capsule(54, 36, 54, 30, 1.0f, R_IRON, 0.1f, 0.4f);   /* the crank */
    if (on) for (int k = 0; k < 6; k++) cv_line(20 + k * 5, 33, 22 + k * 5, 30 - (k & 1) * 2, (uint8_t)(R_GOLD + 9 + (k & 1) * 3));
    cv_outline(0);
    cv_store(id);
}
/* a hearth of old brick with an iron grate: dying, a few coals; fed, roaring */
static void spr_mk_hearth(int id, int roar) {
    cv_begin(cv_sprite, TS, TS);
    for (int y = 30; y <= 62; y++) for (int x = 12; x <= 52; x++) {
        int brick = ((y - 30) / 4 + (x + ((y - 30) / 4 & 1) * 3) / 6) & 1;
        cv_px(x, y, sh(R_BRICK, 0.25f + 0.12f * brick + 0.08f * h01(x, y, 661) - ((y - 30) % 4 == 0 ? 0.1f : 0.0f)));
    }
    for (int y = 40; y <= 62; y++) for (int x = 20; x <= 44; x++) cv_px(x, y, sh(R_STONE, 0.02f + 0.04f * (y > 58)));   /* the mouth */
    for (int x = 20; x <= 44; x++) cv_px(x, 39, sh(R_STONE, 0.3f));
    for (int x = 21; x <= 43; x += 3) cv_line(x, 56, x, 61, sh(R_IRON, 0.4f));   /* the grate */
    for (int i = 0; i < 9; i++) cv_flat(23 + h01(i, 1, 663) * 18, 57 + h01(i, 2, 663) * 3, 1.4f, 0.9f, (uint8_t)(R_FLAME + (roar ? 8 : 3) + (i % 3)));
    if (roar)
        for (int k = 0; k < 5; k++) {
            float x = 24 + k * 4, top = 42 + 4 * h01(k, 3, 665);
            for (int y = (int)top; y < 57; y++) cv_px((int)(x + sinf(y * 0.5f + k) * 1.2f), y, (uint8_t)(R_FLAME + 15 - (57 - y) / 3));
        }
    cv_outline(0);
    cv_store(id);
}
/* a cracked iron pipe out of the floor, venting scalding steam; frozen, sheathed in ice and hung with icicles */
static void spr_mk_vent(int id, int iced) {
    cv_begin(cv_sprite, TS, TS);
    cv_flat(32, 62, 10, 1.6f, R_WARM + 1);
    cv_capsule(32, 44, 32, 62, 3.0f, R_IRON, 0.08f, 0.45f);
    cv_capsule(32, 44, 38, 38, 3.0f, R_IRON, 0.08f, 0.45f);
    cv_line(37, 37, 39, 40, sh(R_STONE, 0.02f));                    /* its crack */
    if (!iced)
        for (int i = 0; i < 22; i++) {
            float u = h01(i, 1, 671), x = 40 + u * 10 - 6 * u * u, y = 36 - u * 32;
            cv_flat(x + h01(i, 2, 671) * 6 - 3, y, 2 + 4 * u, 1.5f + 3 * u, (uint8_t)(R_GHOST + 13 - (int)(7 * u)));
        }
    else {
        cv_capsule(32, 42, 32, 62, 4.2f, R_GHOST, 0.4f, 0.95f); cv_capsule(32, 43, 38, 37, 4.2f, R_GHOST, 0.4f, 0.95f);
        for (int k = 0; k < 6; k++) cv_line(27 + k * 2.3f, 36 + (k & 1) * 4, 27 + k * 2.3f, 43 + (k & 1) * 4 + 3, sh(R_GHOST, 0.85f));
    }
    cv_outline(0);
    cv_store(id);
}
/* run III: an ore cart on its rails, heaped, a glint in it; two turns of its wheels */
static void spr_mk_cart(int id, int f) {
    cv_begin(cv_sprite, TS, TS);
    for (int x = 4; x <= 60; x++) { cv_px(x, 61, sh(R_IRON, 0.45f)); if ((x & 7) == 0) for (int y = 60; y <= 63; y++) cv_px(x, y, sh(R_WOOD, 0.3f)); }   /* the rails */
    float pts[8] = { 14, 36, 50, 36, 46, 54, 18, 54 };
    cv_poly(pts, 4, R_WOOD, 0.45f, 0.18f, 36, 18);
    for (int x = 14; x <= 50; x++) cv_px(x, 36, sh(R_IRON, 0.55f));
    cv_line(16, 44, 48, 44, sh(R_IRON, 0.35f));
    for (int i = 0; i < 16; i++) cv_flat(18 + h01(i, 1, 681) * 28, 33 - h01(i, 2, 681) * 5 * (1 - fabsf(h01(i, 1, 681) - 0.5f) * 2), 2.4f, 1.6f, sh(R_STONE, 0.25f + 0.3f * h01(i, 3, 681)));
    cv_flat(30, 30, 1.6f, 1.2f, sh(R_GOLD, 0.9f));
    for (int w = 0; w < 2; w++) {
        float wx = w ? 42 : 22;
        cv_ellipse(wx, 56, 4.5f, 4.5f, R_IRON, 0.1f, 0.5f);
        for (int s = 0; s < 4; s++) { float a = s * 0.785f + f * 0.39f; cv_line(wx, 56, wx + cosf(a) * 3.5f, 56 + sinf(a) * 3.5f, sh(R_IRON, 0.7f)); }
    }
    cv_outline(0);
    cv_store(id);
}
/* run III: a lodestone in an iron cradle, veined; charged, its veins burning gold and sparks leaping */
static void spr_mk_lodestone(int id, int on) {
    cv_begin(cv_sprite, TS, TS);
    cv_flat(32, 62, 16, 1.8f, R_WARM + 1);
    cv_capsule(18, 50, 18, 62, 1.4f, R_IRON, 0.05f, 0.3f); cv_capsule(46, 50, 46, 62, 1.4f, R_IRON, 0.05f, 0.3f); cv_capsule(16, 52, 48, 52, 1.2f, R_IRON, 0.05f, 0.35f);
    cv_ellipse(32, 38, 14, 13, R_STONE, 0.04f, 0.32f);
    for (int v = 0; v < 4; v++) {                                   /* its veins */
        float x = 22 + v * 6, y = 28;
        for (int s = 0; s < 18; s++) { float nx = x + (h01(v, s, 691) - 0.5f) * 3, ny = y + 1.2f; if (hypotf(nx - 32, (ny - 38) * 1.07f) < 13) cv_px((int)nx, (int)ny, on ? (uint8_t)(R_GOLD + 9 + (s % 4)) : sh(R_IRON, 0.55f)); x = nx; y = ny; }
    }
    if (on) for (int k = 0; k < 5; k++) { float a = k * 1.25f; cv_line(32 + cosf(a) * 14, 38 + sinf(a) * 13, 32 + cosf(a) * 19, 38 + sinf(a) * 17, (uint8_t)(R_GOLD + 11)); }
    cv_outline(0);
    cv_store(id);
}
/* run II: a sheet of new ice on dark water, cracked at its edges: what frost lays for a crossing */
static void spr_mk_ice(int id) {
    cv_begin(cv_sprite, TS, TS);
    for (int y = 52; y <= 63; y++)
        for (int x = 0; x < 64; x++) {
            float e = fabsf((float)x - 32) / 32.0f + fabsf((float)y - 58) / 12.0f;
            if (e > 1.05f + 0.1f * h01(x, y, 697)) continue;
            cv_px(x, y, sh(R_GHOST, 0.45f + 0.35f * h01(x / 3, y, 699) + (y < 54 ? 0.15f : 0.0f)));
        }
    for (int k = 0; k < 5; k++) cv_line(8 + k * 12, 55 + (k & 1) * 3, 14 + k * 12, 61 - (k & 1) * 2, sh(R_GHOST, 0.2f));
    cv_store(id);
}

/* ---- step 8 (sections 4.4 and 4.5): sequences, and things for fights, in a gothic hand ---- */
static float g_arch(float dx, float w) { float a = w + fabsf(dx); return 4 * w * w - a * a > 0 ? sqrtf(4 * w * w - a * a) : 0.0f; }   /* an equilateral
                                                                       pointed arch's height over its springing, dx from its middle */
static void g_crockets(float x0, float y0, float x1, float y1, int n, uint8_t c) {   /* little leaves up a sloping edge */
    for (int k = 1; k < n; k++) { float u = (float)k / n; int x = (int)(x0 + (x1 - x0) * u), y = (int)(y0 + (y1 - y0) * u); cv_px(x + (x1 > x0 ? -1 : 1), y - 1, c); cv_px(x, y - 1, c); }
}
static void g_finial(float cx, float top, int ramp, float v) {    /* a finial: a knop and a cross-leaf */
    cv_px((int)cx, (int)top, sh(ramp, v + 0.3f)); cv_px((int)cx - 1, (int)top + 1, sh(ramp, v + 0.1f)); cv_px((int)cx + 1, (int)top + 1, sh(ramp, v));
    cv_px((int)cx, (int)top + 1, sh(ramp, v + 0.2f)); cv_px((int)cx, (int)top + 2, sh(ramp, v));
}
static void g_lancet(float cx, float ys, float w, float yb, uint8_t c, uint8_t rim) {   /* a lancet window or niche, sunk */
    for (int y = (int)(ys - g_arch(0, w)) - 1; y <= (int)yb; y++)
        for (int x = (int)(cx - w) - 1; x <= (int)(cx + w) + 1; x++) {
            float dx = x + 0.5f - cx, top = ys - g_arch(dx, w);
            if (fabsf(dx) > w + 0.5f || y < top - 0.6f) continue;
            cv_px(x, y, (fabsf(dx) > w - 0.6f || y < top + 0.6f) ? rim : c);
        }
}
static void g_quatrefoil(float cx, float cy, float r, uint8_t c) {
    for (int k = 0; k < 4; k++) { float a = k * 1.5708f; cv_flat(cx + cosf(a) * r * 0.55f, cy + sinf(a) * r * 0.55f, r * 0.5f, r * 0.5f, c); }
}
/* the tablet: a lancet-headed slab with a crocketed hood and finial, a trefoil in its head, three sockets for the glyphs it
 * shows, worn black-letter under them and a skull carved at its foot, on a moulded step */
static void spr_mk_tablet(int id) {
    cv_begin(cv_sprite, TS, TS);
    cv_flat(32, 62, 20, 1.8f, R_WARM + 1);
    for (int y = 56; y <= 62; y++) for (int x = 10; x <= 54; x++) cv_px(x, y, sh(R_STONE, 0.2f + 0.14f * (y == 56) + 0.08f * (y == 57) + 0.05f * h01(x, y, 701)));
    for (int y = 4; y <= 56; y++)
        for (int x = 13; x <= 51; x++) {
            float dx = x + 0.5f - 32, top = 26 - g_arch(dx, 19);
            if (fabsf(dx) > 19 || y < top) continue;
            int edge = fabsf(dx) > 16.5f || y < top + 2.5f;           /* the hood mould */
            cv_px(x, y, sh(R_STONE, (edge ? 0.42f : 0.28f) + 0.14f * (dx < -14) - 0.1f * (dx > 14) + 0.06f * h01(x / 2, y / 2, 703)));
        }
    g_crockets(13, 26, 32, 26 - g_arch(0, 19), 6, sh(R_STONE, 0.55f)); g_crockets(51, 26, 32, 26 - g_arch(0, 19), 6, sh(R_STONE, 0.4f));
    g_finial(32, 26 - g_arch(0, 19) - 3, R_STONE, 0.35f);
    for (int k = 0; k < 3; k++) { float a = -1.5708f + k * 2.094f; cv_flat(32 + cosf(a) * 2.6f, 14 + sinf(a) * 2.6f, 2.0f, 2.0f, sh(R_STONE, 0.08f)); }   /* a trefoil */
    for (int k = 0; k < 3; k++) {                                   /* the sockets, each in its own small pointed niche */
        float cx = 20 + 12 * k;
        g_lancet(cx, 26, 5.2f, 34, sh(R_STONE, 0.07f), sh(R_STONE, 0.5f));
    }
    for (int r = 0; r < 3; r++) for (int x = 18; x <= 46; x++) if (h01(x, r, 705) > 0.4f) cv_px(x, 39 + r * 3, sh(R_STONE, 0.1f + 0.1f * ((x & 3) == 0)));   /* black-letter, worn */
    cv_skull(32, 50, 0.55f, 0.12f, 0.45f);
    for (int i = 0; i < 16; i++) cv_px((int)(15 + h01(i, 1, 707) * 34), (int)(30 + h01(i, 2, 707) * 26), sh(R_MOSS, 0.35f));
    cv_outline(0);
    cv_store(id);
}
/* a plinth like a buttress's pinnacle: stepped, a lancet niche in its face, an iron dish with spikes on its crocketed top;
 * struck right, the niche burns gold and the dish smoulders */
static void spr_mk_plinth(int id, int lit) {
    cv_begin(cv_sprite, TS, TS);
    cv_flat(32, 62, 11, 1.6f, R_WARM + 1);
    for (int y = 28; y <= 62; y++) {
        int w = y > 54 ? 9 : y > 46 ? 8 : 7;                         /* set-offs, as a buttress steps */
        for (int x = 32 - w; x <= 32 + w; x++) cv_px(x, y, sh(R_STONE, 0.26f + 0.18f * (x < 32 - w + 3) - 0.1f * (x > 32 + w - 3) + 0.05f * h01(x, y, 711) + 0.12f * (y == 47 || y == 55)));
    }
    g_lancet(32, 40, 3.6f, 52, lit ? (uint8_t)(R_GOLD + 8) : sh(R_STONE, 0.06f), lit ? (uint8_t)(R_GOLD + 12) : sh(R_STONE, 0.45f));
    if (lit) for (int y = 36; y <= 51; y += 3) cv_px(32, y, (uint8_t)(R_GOLD + 12));
    for (int y = 22; y <= 28; y++) { int w = 7 + (28 - y) / 2; for (int x = 32 - w; x <= 32 + w; x++) cv_px(x, y, sh(R_STONE, 0.35f + 0.1f * (x < 32))); }   /* its capital */
    g_crockets(22, 26, 25, 22, 3, sh(R_STONE, 0.6f)); g_crockets(42, 26, 39, 22, 3, sh(R_STONE, 0.45f));
    cv_flat(32, 20, 9, 2.0f, sh(R_IRON, 0.2f)); for (int x = 23; x <= 41; x++) cv_px(x, 18, sh(R_IRON, 0.45f));   /* the dish */
    for (int k = 0; k < 5; k++) { int x = 24 + k * 4; cv_line(x, 18, x, 14 - (k & 1), sh(R_IRON, 0.5f)); }   /* its spikes */
    if (lit) for (int i = 0; i < 7; i++) cv_flat(25 + i * 2.2f, 17 - (i & 1), 1.2f, 1.6f, (uint8_t)(R_GOLD + 8 + (i % 4)));
    cv_outline(0);
    cv_store(id);
}
/* a gilt shrine on a clustered shaft, turning: a steep spire with crockets, pinnacles at its shoulders, a lancet window with
 * a quatrefoil over; turned, its narrow side; answered, its doors open on gold and a skull in a crown */
static void spr_mk_reliq(int id, int f) {
    cv_begin(cv_sprite, TS, TS);
    cv_flat(32, 62, 10, 1.6f, R_WARM + 1);
    for (int k = -1; k <= 1; k++) cv_capsule(32 + k * 2.2f, 46, 32 + k * 2.2f, 61, 1.3f, R_STONE, 0.15f + 0.1f * (k < 0), 0.5f);   /* a clustered shaft */
    for (int x = 25; x <= 39; x++) { cv_px(x, 61, sh(R_STONE, 0.35f)); cv_px(x, 62, sh(R_STONE, 0.2f)); }
    cv_flat(32, 46, 8, 1.6f, sh(R_GOLD, 0.45f));
    int hw = f == 1 ? 5 : 11;
    for (int y = 26; y <= 45; y++) for (int x = 32 - hw; x <= 32 + hw; x++) cv_px(x, y, sh(R_GOLD, 0.35f + 0.38f * (x < 32 - hw + 3) - 0.15f * (x > 32 + hw - 3) + 0.08f * h01(x, y, 721)));
    for (int x = 32 - hw - 1; x <= 32 + hw + 1; x++) { cv_px(x, 26, sh(R_GOLD, 0.7f)); cv_px(x, 45, sh(R_GOLD, 0.5f)); }
    float spire = f == 1 ? 6 : 13;                                   /* the spire, steep */
    for (int y = (int)(26 - spire * 1.4f); y <= 26; y++) { float w = (y - (26 - spire * 1.4f)) / (spire * 1.4f) * (hw - 1); for (int x = (int)(32 - w); x <= (int)(32 + w); x++) cv_px(x, y, sh(R_GOLD, 0.3f + 0.35f * (x < 32) + 0.1f * ((y & 3) == 0))); }
    g_crockets(32 - hw + 1, 26, 32, 26 - spire * 1.4f, 5, (uint8_t)(R_GOLD + 11));
    g_crockets(32 + hw - 1, 26, 32, 26 - spire * 1.4f, 5, sh(R_GOLD, 0.6f));
    g_finial(32, 26 - spire * 1.4f - 3, R_GOLD, 0.6f);
    if (f != 1) for (int s = -1; s <= 1; s += 2) { cv_capsule(32 + s * 12.5f, 20, 32 + s * 12.5f, 26, 0.9f, R_GOLD, 0.4f, 0.8f); cv_px(32 + s * 12, 18, (uint8_t)(R_GOLD + 12)); }   /* pinnacles */
    if (f == 0) { g_lancet(32, 36, 4.5f, 43, sh(R_STONE, 0.05f), sh(R_GOLD, 0.95f)); g_quatrefoil(32, 29.5f, 3.0f, sh(R_STONE, 0.06f)); }
    if (f == 1) g_lancet(32, 36, 2.2f, 43, sh(R_STONE, 0.05f), sh(R_GOLD, 0.8f));
    if (f == 2) {                                                   /* open: doors out like wings, a crowned skull on gold */
        for (int y = 27; y <= 44; y++) for (int x = 23; x <= 41; x++) cv_px(x, y, sh(R_STONE, 0.04f));
        for (int y = 27; y <= 44; y++) for (int k = 0; k < 3; k++) { cv_px(16 + k, y + (k == 0), sh(R_GOLD, 0.45f + 0.15f * k)); cv_px(46 + k, y + (k == 2), sh(R_GOLD, 0.6f - 0.15f * k)); }
        cv_flat(32, 42, 6, 2, sh(R_GOLD, 0.85f));
        cv_skull(32, 36, 0.5f, 0.3f, 0.8f);
        for (int k = 0; k < 5; k++) cv_px(29 + k + (k > 2), 31 - (k & 1), (uint8_t)(R_GOLD + 12));   /* its crown */
    }
    cv_outline(0);
    cv_store(id);
}
/* the ward-stone: a lancet-headed stone before a door, a Calvary cross in relief over an iron ring and a skull at its foot;
 * broken, split down, its head fallen */
static void spr_mk_wardstone(int id, int broken) {
    cv_begin(cv_sprite, TS, TS);
    cv_flat(32, 62, 15, 1.8f, R_WARM + 1);
    for (int y = 2; y <= 62; y++) for (int x = 19; x <= 45; x++) {
        float dx = x + 0.5f - 32, top = 20 - g_arch(dx, 13);
        if (fabsf(dx) > 13 || y < top) continue;
        if (broken && y < 30 - (int)(dx * 0.6f)) continue;          /* its head gone, a slanting break */
        int edge = fabsf(dx) > 11 || y < top + 2;
        cv_px(x, y, sh(R_STONE, (edge ? 0.38f : 0.26f) + 0.16f * (dx < -10) - 0.1f * (dx > 10) + 0.06f * h01(x / 2, y / 3, 731)));
    }
    if (!broken) {
        for (int y = 6; y <= 22; y++) cv_px(32, y, sh(R_STONE, 0.6f)), cv_px(31, y, sh(R_STONE, 0.15f));   /* the cross, raised */
        for (int x = 27; x <= 37; x++) cv_px(x, 11, sh(R_STONE, 0.6f)), cv_px(x, 12, sh(R_STONE, 0.15f));
        for (int x = 29; x <= 35; x++) cv_px(x, 23, sh(R_STONE, 0.5f));
        g_finial(32, -1, R_STONE, 0.4f);
    }
    for (int a = 0; a < 30; a++) { float t = a * 0.2094f; cv_px((int)(32 + cosf(t) * 5.5f), (int)(33 + sinf(t) * 5.5f), sh(R_IRON, 0.45f + 0.3f * (sinf(t) < 0))); }
    cv_px(32, 27, sh(R_IRON, 0.6f)); cv_px(32, 28, sh(R_IRON, 0.5f));
    cv_skull(32, 50, 0.5f, 0.1f, 0.4f);
    if (broken) {
        int x = 33; for (int y = 22; y <= 62; y++) { x += (int)(h01(y, 3, 733) * 3) - 1; cv_px(x, y, sh(R_STONE, 0.02f)); }
        cv_flat(52, 57, 7, 3.5f, sh(R_STONE, 0.3f)); for (int y = 53; y <= 58; y++) cv_px(52, y, sh(R_STONE, 0.6f));   /* its head, the cross on it */
        for (int i = 0; i < 8; i++) cv_flat(10 + h01(i, 1, 735) * 10, 59 + h01(i, 2, 735) * 3, 1.8f, 1.1f, sh(R_STONE, 0.25f + 0.2f * h01(i, 3, 735)));
    }
    cv_outline(0);
    cv_store(id);
}
/* the relay's lamp: a gothic lantern of black iron, lancet panes and a spired cap with a finial, on a spiked post with a
 * curled bracket; dead (dark glass), lit, frozen (rimed, icicles) */
static void spr_mk_relay(int id, int st) {
    cv_begin(cv_sprite, TS, TS);
    cv_flat(32, 62, 8, 1.4f, R_WARM + 1);
    cv_capsule(32, 34, 32, 62, 1.4f, R_IRON, 0.05f, 0.35f);
    cv_capsule(26, 61, 38, 61, 1.3f, R_IRON, 0.05f, 0.3f);
    for (int k = 0; k < 4; k++) cv_px(27 + k * 3, 59, sh(R_IRON, 0.45f));
    for (int a = 0; a < 10; a++) { float t = a * 0.45f; cv_px((int)(36 + cosf(t) * 3), (int)(38 + sinf(t) * 3), sh(R_IRON, 0.4f)); }   /* a curl */
    for (int p = -1; p <= 1; p++)                                    /* three lancet panes */
        g_lancet(32 + p * 4.4f, 20, 1.8f, 32,
                 st == 1 ? sh(R_FLAME, 0.6f + 0.25f * (p == 0)) : st == 2 ? sh(R_GHOST, 0.55f + 0.2f * (p == 0)) : sh(R_STONE, 0.06f), sh(R_IRON, 0.4f));
    for (int x = 25; x <= 39; x++) { cv_px(x, 33, sh(R_IRON, 0.45f)); cv_px(x, 34, sh(R_IRON, 0.25f)); }
    for (int y = 4; y <= 15; y++) { int w = (y - 4) * 8 / 11; for (int x = 32 - w; x <= 32 + w; x++) cv_px(x, y, sh(R_IRON, 0.18f + 0.25f * (x < 32))); }   /* the cap */
    g_crockets(24, 15, 32, 4, 4, sh(R_IRON, 0.5f)); g_crockets(40, 15, 32, 4, 4, sh(R_IRON, 0.35f));
    g_finial(32, 1, R_IRON, 0.4f);
    if (st == 1) cv_ellipse(32, 26, 1.8f, 3.4f, R_FLAME, 0.75f, 1.0f);
    if (st == 2) for (int k = 0; k < 7; k++) cv_line(25 + k * 2.3f, 35, 25 + k * 2.3f, 37 + (k & 1) * 3 + (k == 3) * 2, sh(R_GHOST, 0.85f));
    cv_outline(0);
    cv_store(id);
}
/* lamp oil: two casks on a cradle, banded in riveted iron, burnt with the Order's cross, a black candle stuck on top;
 * burst, the staves sprung and the oil run out dark over the floor */
static void spr_mk_cask(int id, int burst) {
    cv_begin(cv_sprite, TS, TS);
    if (burst) {
        for (int y = 52; y <= 63; y++) for (int x = 1; x <= 63; x++) { float e = fabsf(x - 32.0f) / 31 + fabsf(y - 58.0f) / 5.5f; if (e < 1 + 0.15f * h01(x, y, 751)) cv_px(x, y, sh(R_IRON, 0.06f + 0.1f * h01(x / 3, y, 753))); }
        for (int k = 0; k < 7; k++) cv_capsule(13 + k * 6, 49 + (k & 1) * 4, 17 + k * 6, 60 - (k & 1) * 3, 1.3f, R_WOOD, 0.12f, 0.4f);
        cv_capsule(14, 56, 48, 55, 1.0f, R_IRON, 0.1f, 0.4f); for (int k = 0; k < 6; k++) cv_px(16 + k * 6, 55, sh(R_IRON, 0.7f));
        cv_capsule(42, 52, 52, 60, 1.2f, R_WOOD, 0.1f, 0.35f);         /* the cradle, fallen */
    } else {
        cv_capsule(16, 62, 22, 52, 1.3f, R_WOOD, 0.1f, 0.35f); cv_capsule(48, 62, 42, 52, 1.3f, R_WOOD, 0.1f, 0.35f);   /* the cradle */
        for (int c = 0; c < 2; c++) {
            float cy = c ? 28 : 46, rx = 13, ry = 10;
            for (int y = (int)(cy - ry); y <= (int)(cy + ry); y++) for (int x = 32 - (int)rx; x <= 32 + (int)rx; x++) {
                float u = (y - cy) / ry, w = rx * (0.84f + 0.16f * sqrtf(fmaxf(0, 1 - u * u)));
                if (fabsf(x - 32.0f) > w) continue;
                cv_px(x, y, ((x - 19) % 4 == 0) ? sh(R_WOOD, 0.12f) : sh(R_WOOD, 0.24f + 0.3f * lit((x - 32) / w, u, 0.5f)));
            }
            for (int b = -1; b <= 1; b += 2) for (int x = 20; x <= 44; x++) { int y = (int)(cy + b * ry * 0.62f); cv_px(x, y, sh(R_IRON, 0.4f)); if ((x & 3) == 0) cv_px(x, y, sh(R_IRON, 0.75f)); }
            cv_line(32, cy - 4, 32, cy + 4, sh(R_BRICK, 0.08f)); cv_line(28, cy - 1, 36, cy - 1, sh(R_BRICK, 0.08f));   /* the Order's cross, burnt */
        }
        cv_capsule(36, 13, 36, 18, 1.2f, R_STONE, 0.05f, 0.2f); cv_px(36, 11, sh(R_STONE, 0.3f));   /* a black candle, unlit */
        for (int y = 17; y <= 21; y++) cv_px(35 + (y & 1), y, sh(R_STONE, 0.2f));
        cv_flat(40, 62, 5, 1.0f, sh(R_IRON, 0.08f));                 /* a weeping stave */
    }
    cv_outline(0);
    cv_store(id);
}
/* loose masonry: a clustered column on a moulded base, a capital of skulls under its abacus, a crack down it and dust at its
 * foot; brought down, the stump and its drums in a heap, a skull rolled clear */
static void spr_mk_pillar(int id, int down) {
    cv_begin(cv_sprite, TS, TS);
    cv_flat(32, 62, 15, 1.8f, R_WARM + 1);
    for (int y = 56; y <= 62; y++) for (int x = 21; x <= 43; x++) cv_px(x, y, sh(R_STONE, 0.3f + 0.15f * (y == 56) - 0.1f * (y == 62) + 0.12f * (x < 24)));   /* the base */
    int top = down ? 44 : 12;
    for (int y = top; y <= 55; y++) for (int s = -1; s <= 1; s++) {    /* three shafts, clustered */
        int cx = 32 + s * 5, r = s ? 3 : 4;
        for (int x = cx - r; x <= cx + r; x++) { float u = (x - cx) / (float)r; cv_px(x, y, sh(R_STONE, 0.2f + 0.3f * lit(u, 0, sqrtf(fmaxf(0, 1 - u * u))) + 0.04f * h01(x, y / 3, 761))); }
    }
    if (!down) {
        for (int y = 4; y <= 11; y++) for (int x = 22; x <= 42; x++) cv_px(x, y, sh(R_STONE, 0.32f + 0.12f * (y == 4) + 0.1f * (x < 25)));   /* the capital */
        for (int k = 0; k < 3; k++) cv_skull(25 + k * 7, 8, 0.38f, 0.15f, 0.45f);
        for (int x = 20; x <= 44; x++) { cv_px(x, 2, sh(R_STONE, 0.45f)); cv_px(x, 3, sh(R_STONE, 0.3f)); }
        int x = 30; for (int y = 4; y <= 58; y++) { x += (int)(h01(y, 1, 763) * 3) - 1; x = x < 25 ? 25 : x > 39 ? 39 : x; cv_px(x, y, sh(R_STONE, 0.02f)); if (h01(y, 2, 763) > 0.7f) cv_px(x + 1, y, sh(R_STONE, 0.04f)); }
        for (int i = 0; i < 10; i++) cv_px((int)(18 + h01(i, 3, 765) * 28), 62 - (i & 1), sh(R_STONE, 0.5f));
    } else {
        for (int d = 0; d < 4; d++) { float cx = 9 + d * 15 + h01(d, 4, 767) * 4, cy = 55 - (d & 1) * 2; cv_ellipse(cx, cy, 7, 4.5f, R_STONE, 0.18f, 0.5f); cv_line(cx - 5, cy, cx + 5, cy, sh(R_STONE, 0.12f)); }
        cv_skull(56, 58, 0.45f, 0.15f, 0.45f);
    }
    cv_outline(0);
    cv_store(id);
}
/* the poison font: an octagonal bowl girt with a blind arcade of little pointed arches, on a clustered stem with a skull at
 * its knot; dry and stained, or brimming with bile and bubbling */
static void spr_mk_pfont(int id, int full) {
    cv_begin(cv_sprite, TS, TS);
    cv_flat(32, 62, 13, 1.6f, R_WARM + 1);
    for (int y = 57; y <= 62; y++) { int w = 9 + (y - 57); for (int x = 32 - w; x <= 32 + w; x++) cv_px(x, y, sh(R_STONE, 0.25f + 0.15f * (y == 57) + 0.1f * (x < 32 - w + 3))); }
    for (int s = -1; s <= 1; s++) cv_capsule(32 + s * 3, 40, 32 + s * 3, 57, 1.5f, R_STONE, 0.15f + 0.1f * (s < 0), 0.5f);
    cv_skull(32, 46, 0.42f, 0.12f, 0.45f);
    for (int y = 24; y <= 40; y++) { float u = (y - 24) / 16.0f; int w = (int)(17 - 8 * u * u); for (int x = 32 - w; x <= 32 + w; x++) cv_px(x, y, sh(R_STONE, 0.26f + 0.24f * (x < 32 - w + 3) - 0.1f * (x > 32 + w - 3) + 0.08f * (((x - 32 + 40) % 7) == 0))); }   /* eight faces */
    for (int k = 0; k < 4; k++) g_lancet(20 + k * 8, 31, 2.0f, 36, sh(R_STONE, 0.1f), sh(R_STONE, 0.45f));   /* its arcade */
    cv_flat(32, 24, 17, 3, sh(R_STONE, 0.45f));
    cv_flat(32, 24, 14.5f, 2.2f, full ? sh(R_MOSS, 0.6f) : sh(R_STONE, 0.08f));
    if (full) { for (int i = 0; i < 7; i++) cv_flat(21 + h01(i, 1, 771) * 22, 23.5f + h01(i, 2, 771), 1.1f, 0.8f, sh(R_MOSS, 0.9f)); for (int k = 0; k < 4; k++) cv_line(19 + k * 8, 26, 18 + k * 8, 31 + (k & 1) * 3, sh(R_MOSS, 0.55f)); }
    else for (int k = 0; k < 3; k++) cv_line(22 + k * 9, 27, 22 + k * 9, 33, sh(R_MOSS, 0.25f));
    cv_outline(0);
    cv_store(id);
}
/* the Order's war drum on a trestle, its shell red and gilt-banded, the skin painted with the Order's cross, skulls hung from
 * its cords and iron spikes round its rim; the sticks of bone raised; silenced, the skin slit and the sticks dropped */
static void spr_mk_drum(int id, int slit) {
    cv_begin(cv_sprite, TS, TS);
    cv_flat(32, 62, 17, 1.8f, R_WARM + 1);
    cv_capsule(17, 62, 26, 44, 1.3f, R_WOOD, 0.08f, 0.35f); cv_capsule(47, 62, 38, 44, 1.3f, R_WOOD, 0.08f, 0.35f); cv_capsule(20, 56, 44, 56, 1.0f, R_WOOD, 0.08f, 0.3f);
    for (int y = 24; y <= 48; y++) for (int x = 14; x <= 50; x++) { float u = (y - 36) / 12.0f; if (fabsf(u) > 1) continue; cv_px(x, y, sh(R_BRICK, 0.22f + 0.3f * lit((x - 32) / 18.0f, u, 0.6f))); }
    for (int x = 14; x <= 50; x++) for (int k = 0; k < 2; k++) cv_px(x, k ? 26 : 46, sh(R_GOLD, 0.55f + 0.2f * ((x & 3) == 0)));
    for (int k = 0; k < 6; k++) cv_line(16 + k * 6.5f, 27, 19 + k * 6.5f, 45, sh(R_BONE, 0.5f));   /* its cords, zig-zagged */
    for (int k = 0; k < 2; k++) cv_skull(20 + k * 24, 38, 0.38f, 0.15f, 0.5f);   /* skulls hung on them */
    for (int y = 20; y <= 26; y++) for (int x = 15; x <= 49; x++) { float d = abs(y - 23) / 3.5f + abs(x - 32) / 17.5f; if (d < 1) cv_px(x, y, sh(R_BONE, 0.55f + 0.22f * (y < 23))); }   /* the skin */
    for (int k = 0; k < 9; k++) { int x = 15 + k * 4; cv_line(x, 24, x, 21 - (k & 1), sh(R_IRON, 0.5f)); }   /* spikes round the rim */
    cv_line(32, 20, 32, 26, (uint8_t)(R_BLOOD + 10)); cv_line(26, 22, 38, 22, (uint8_t)(R_BLOOD + 10));   /* the Order's cross */
    if (slit) { cv_line(22, 21, 41, 25, sh(R_STONE, 0.02f)); cv_bone(6, 61, 20, 58, 1.0f, 0.3f, 0.6f); cv_bone(44, 60, 58, 62, 1.0f, 0.3f, 0.6f); }
    else { cv_bone(41, 4, 35, 19, 1.0f, 0.35f, 0.7f); cv_bone(23, 3, 28, 19, 1.0f, 0.35f, 0.7f); }   /* the sticks, of bone */
    cv_outline(0);
    cv_store(id);
}

/* The mark on a thing that wants a spell (section 4.1): a seal of dark iron with the element's glyph, motes of its
 * colour drawn in towards it (t, in seconds, turns them), a row of pips for the hits it wants (lit as they land); just
 * struck, the seal flares; struck by the wrong spell, a dull ring and sparks thrown back; done, the seal dark. Into a
 * row-major buffer w by h; it is drawn at full brightness, as the eyes are, so it shows through dark and fog */
static const int MK_RAMP[6] = { R_FLAME, R_GLOW, R_GOLD, R_MOSS, R_PURPLE, R_BONE };
static int mkc(int ramp, int v) { v = v < 0 ? 0 : v > 15 ? 15 : v; return ramp == R_GOLD ? R_GOLD + v * (NGOLD - 1) / 15 : ramp + v; }   /* gold has thirteen */
void crawl_mark(uint8_t *buf, int w, int h, int cx, int cy, int el, int state, int pips, int lit_n, float t) {
#define MKPX(x, y, c) do { int x_ = (x), y_ = (y); if ((unsigned)x_ < (unsigned)w && (unsigned)y_ < (unsigned)h) buf[y_ * w + x_] = (uint8_t)(c); } while (0)
    int ramp = MK_RAMP[el < 0 || el > 5 ? 0 : el];
    if (state == MK_WAIT || state == MK_HIT)                          /* the motes, spiralling in */
        for (int k = 0; k < 8; k++) {
            float p = t * 0.35f + (float)k / 8.0f; p -= floorf(p);
            float r = 4.5f + 15.0f * (1.0f - p), a = (float)k * 0.785f + p * 2.4f;
            int x = cx + (int)lroundf(cosf(a) * r), y = cy + (int)lroundf(sinf(a) * r * 0.8f);
            int tx = cx + (int)lroundf(cosf(a - 0.25f) * (r + 2.0f)), ty = cy + (int)lroundf(sinf(a - 0.25f) * (r + 2.0f) * 0.8f);
            MKPX(tx, ty, mkc(ramp, 5 + (int)(4 * p)));
            MKPX(x, y, mkc(ramp, 9 + (int)(6 * p)));
        }
    for (int y = -6; y <= 6; y++)                                     /* the seal */
        for (int x = -6; x <= 6; x++) {
            float d = sqrtf((float)(x * x + y * y));
            if (d > 6.0f) continue;
            int c = d > 4.9f ? R_IRON + (x + y < 0 ? 9 : 4) : R_IRON + 1;
            if (state == MK_DONE) c = d > 4.9f ? R_IRON + 3 : R_IRON;
            MKPX(cx + x, cy + y, c);
        }
    for (int r = 0; r < 7; r++)                                       /* its glyph */
        for (int c = 0; c < 7; c++) {
            char ch = ELEM_GLYPH[el < 0 || el > 5 ? 0 : el][r][c];
            if (ch != '#' && ch != 'o') continue;
            int col = state == MK_DONE ? R_STONE + 3 : state == MK_WRONG ? R_STONE + (ch == 'o' ? 9 : 7)
                    : state == MK_HIT ? (ch == 'o' ? C_WHITE : mkc(ramp, 15)) : mkc(ramp, ch == 'o' ? 15 : 11);
            MKPX(cx - 3 + c, cy - 3 + r, col);
        }
    if (state == MK_HIT)                                              /* a flare round it */
        for (int k = 0; k < 24; k++) { float a = (float)k * 0.2618f; if (k & 1) MKPX(cx + (int)lroundf(cosf(a) * 9), cy + (int)lroundf(sinf(a) * 9), mkc(ramp, 13)); }
    if (state == MK_WRONG) {                                          /* a dull ring, and the sparks thrown back */
        for (int k = 0; k < 28; k++) { float a = (float)k * 0.2244f; if (k % 3) MKPX(cx + (int)lroundf(cosf(a) * 9), cy + (int)lroundf(sinf(a) * 9), R_STONE + 8); }
        static const int SP[6][2] = { { -13, -6 }, { 12, -8 }, { -11, 7 }, { 14, 5 }, { -3, -14 }, { 4, 13 } };
        for (int k = 0; k < 6; k++) { MKPX(cx + SP[k][0], cy + SP[k][1], R_BONE + 12); MKPX(cx + SP[k][0] * 9 / 10, cy + SP[k][1] * 9 / 10, R_STONE + 7); }
    }
    for (int k = 0; k < pips && pips > 1; k++) {                      /* the hits it wants, a row under it */
        int x = cx - (pips - 1) * 2 + k * 4, y = cy + 9;
        int on = k < lit_n;
        MKPX(x, y, on ? mkc(ramp, 15) : R_STONE + 5); MKPX(x + 1, y, on ? mkc(ramp, 12) : R_STONE + 3);
        MKPX(x, y + 1, on ? mkc(ramp, 12) : R_STONE + 3); MKPX(x + 1, y + 1, on ? mkc(ramp, 9) : R_STONE + 2);
    }
#undef MKPX
}

/* The Corpus Compendium: a great tome open on a lectern of black oak, its pages close-written, a red
 * ribbon hanging from it, and a few motes of light over it from whatever it remembers */
/* The Lampstand (docs/PLAN-b11.md, section 8): six iron bowls on a black iron stand, fanned out from its stem on
 * curled arms, the elements' order left to right. Its flames are the game's to draw, each bowl's in its element's
 * colour once its line is open (LAMP_BOWL: where each sits) */
const signed char LAMP_BOWL[6][2] = { { 9, 30 }, { 16, 21 }, { 25, 14 }, { 39, 14 }, { 48, 21 }, { 55, 30 } };
