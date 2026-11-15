/* A part of crawl_art.c (docs/PLAN-tidy.md): it is #included there in its place and never compiled on its own.
 * The crypt's small life (docs/PLAN-ambience.md): moths and flies, the bat, the beetle and the woodlouse, the toad, the glowing fungus and
 * its spores, the puddle and its rings, a wet print and a drip, eyes in a niche, a sleeper's skull and hand. Each is drawn to fill its box,
 * as the mice and the spiderlings are, and shown at a share of a cell by the game. Side views face left and are stored so (the game
 * turns them with DR_FLIPH). Eyes are marked with cv_eye, for the eye pass: bright from anywhere. */

static void lf_wing(const float *p, int n, int ramp, float lo, float hi, float y0, float span, uint32_t seed, float vein) {
    cv_poly(p, n, ramp, lo, hi, y0, span);
    for (int y = 0; y < TS; y++)                                     /* a dust of lighter and darker scales over it */
        for (int x = 0; x < TS; x++) {
            uint8_t c = cv_sprite[y * TS + x];
            if (c == C_CLEAR || c < ramp || c >= ramp + 16) continue;
            float h = h01(x, y, seed);
            int st = c - ramp;
            if (h > 0.82f && st < 15) st++; else if (h < 0.16f && st > 0) st--;
            cv_sprite[y * TS + x] = (uint8_t)(ramp + st);
        }
    for (int i = 0; i < n; i++) {                                    /* the veins: from the root of the wing out to each tip */
        int j = (i + 1) % n;
        if (i == 0) continue;
        if (vein > 0.0f) cv_line(p[0], p[1], (p[2 * i] + p[2 * j]) * 0.5f, (p[2 * i + 1] + p[2 * j + 1]) * 0.5f, sh(ramp, vein));
    }
}

/* A moth, seen from in front and a little above: wings spread (0) or lifted (1), a feathered pair of antennae, a furred body, an eyespot
 * on each wing. Dusty, R_BONE over R_WARM; nothing in it is bright, for a moth is not. */
static void spr_moth(int id, int frame) {
    cv_begin(cv_sprite, TS, TS);
    static const float LW0[] = { 30, 32, 9, 22, 4, 36, 10, 49, 24, 47, 30, 42 };
    static const float LW1[] = { 30, 32, 20, 7, 10, 17, 11, 33, 20, 43, 30, 42 };
    const float *w = frame ? LW1 : LW0;
    float rw[12];
    for (int i = 0; i < 6; i++) { rw[2 * i] = 63.0f - w[2 * i]; rw[2 * i + 1] = w[2 * i + 1]; }
    lf_wing(w, 6, R_BONE, 0.28f, 0.7f, 8, 44, 701, 0.22f);
    lf_wing(rw, 6, R_BONE, 0.28f, 0.7f, 8, 44, 702, 0.22f);
    float ex = frame ? 17.0f : 12.0f, ey = frame ? 24.0f : 38.0f;
    for (int s = 0; s < 2; s++) {                                    /* an eyespot on each */
        float x = s ? 63.0f - ex : ex;
        cv_flat(x, ey, 3.6f, 3.6f, sh(R_WARM, 0.22f)); cv_flat(x, ey, 2.4f, 2.4f, sh(R_BONE, 0.82f)); cv_flat(x, ey, 1.2f, 1.2f, sh(R_WARM, 0.1f));
    }
    cv_ellipse(32, 38, 3.6f, 9.5f, R_WARM, 0.2f, 0.62f);             /* the body, furred */
    for (int y = 30; y < 47; y += 2) cv_px(31 + (y & 2 ? 1 : 0), y, sh(R_BONE, 0.5f));
    cv_ellipse(32, 28, 3.0f, 3.0f, R_WARM, 0.25f, 0.6f);
    for (int s = -1; s <= 1; s += 2) {                               /* the antennae, feathered */
        float x0 = 32 + s * 1.5f, y0 = 26;
        cv_line(x0, y0, 32 + s * 8, 12, sh(R_WARM, 0.4f));
        for (int k = 1; k < 6; k++) cv_line(32 + s * (1.5f + 1.1f * k), 25 - k * 2.2f, 32 + s * (1.5f + 1.1f * k + 1.6f), 24 - k * 2.2f, sh(R_WARM, 0.3f));
    }
    cv_outline(0);
    cv_store(id);
}

/* A fly: a dark bead of a body, a bronze back, wings like glass, held out (0) or lifted and blurred (1) */
static void spr_fly(int id, int frame) {
    cv_begin(cv_sprite, TS, TS);
    static const float W0[] = { 29, 30, 8, 24, 4, 32, 14, 40, 29, 38 };
    static const float W1[] = { 29, 30, 14, 10, 8, 18, 12, 30, 29, 36 };
    const float *w = frame ? W1 : W0;
    float rw[10];
    for (int i = 0; i < 5; i++) { rw[2 * i] = 63.0f - w[2 * i]; rw[2 * i + 1] = w[2 * i + 1]; }
    cv_poly(w, 5, R_GHOST, 0.5f, 0.85f, 10, 30); cv_poly(rw, 5, R_GHOST, 0.5f, 0.85f, 10, 30);
    cv_line(29, 30, frame ? 14 : 8, frame ? 12 : 26, sh(R_GHOST, 0.3f)); cv_line(34, 30, frame ? 49 : 55, frame ? 12 : 26, sh(R_GHOST, 0.3f));
    cv_ellipse(32, 40, 5.5f, 9.0f, R_IRON, 0.1f, 0.5f);              /* the abdomen, banded */
    for (int y = 36; y < 48; y += 4) cv_line(27, y, 37, y, sh(R_IRON, 0.05f));
    cv_ellipse(32, 29, 6.0f, 5.5f, R_WOOD, 0.2f, 0.7f);              /* the thorax, a bronze back */
    cv_flat(27.5f, 22.5f, 3.0f, 3.2f, sh(R_BLOOD, 0.35f)); cv_flat(36.5f, 22.5f, 3.0f, 3.2f, sh(R_BLOOD, 0.35f));
    cv_ellipse(32, 23, 3.4f, 3.0f, R_IRON, 0.15f, 0.4f);
    cv_outline(0);
    cv_store(id);
}

/* The bat roosting: hung by its feet from the top of the box, wrapped in its wings, the face turned down to you; its eyes two bright
 * points, its ears long. R_WOOD for the fur, R_PURPLE for the membranes folded over it. */
static void spr_lf_bat_roost(int id) {
    cv_begin(cv_sprite, TS, TS);
    for (int s = -1; s <= 1; s += 2) cv_capsule(32 + s * 3, 3, 32 + s * 4.5f, 11, 1.4f, R_SKIN, 0.2f, 0.6f);   /* the feet */
    cv_ellipse(32, 29, 11.5f, 19.0f, R_WOOD, 0.1f, 0.55f);           /* the wrapped body */
    for (int s = -1; s <= 1; s += 2) {                               /* a wing round either side, its fingers showing as ridges */
        cv_ellipse(32 + s * 8.5f, 28, 6.5f, 17.0f, R_PURPLE, 0.1f, 0.45f);
        for (int k = 0; k < 3; k++) cv_line(32 + s * (3 + k * 2.5f), 14 + k, 32 + s * (5 + k * 3.5f), 36 + k * 2, sh(R_PURPLE, 0.55f));
    }
    for (int y = 14; y < 44; y++) for (int x = 24; x < 40; x++) {    /* fur on the chest between them */
        uint8_t c = cv_sprite[y * TS + x];
        if (c >= R_WOOD && c < R_WOOD + 16 && h01(x, y, 711) > 0.78f) cv_sprite[y * TS + x] = (uint8_t)(c + 2);
    }
    cv_ellipse(32, 47, 7.0f, 6.0f, R_WOOD, 0.2f, 0.65f);             /* the head, below */
    for (int s = -1; s <= 1; s += 2) {
        float ex[3] = { 32.0f + s * 4.5f, 32.0f + s * 7.5f, 32.0f + s * 5.5f }, ey[3] = { 48, 47, 55 };
        float p[6] = { ex[0], ey[0], ex[1], ey[1] + 10, ex[2], ey[2] };
        cv_poly(p, 3, R_SKIN, 0.2f, 0.55f, 46, 12);                  /* an ear, long, pointing down past the face */
    }
    cv_ellipse(32, 52, 3.2f, 2.6f, R_SKIN, 0.3f, 0.65f);             /* the snout */
    cv_px(31, 53, sh(R_BLOOD, 0.1f)); cv_px(33, 53, sh(R_BLOOD, 0.1f));
    cv_eye(29, 46, sh(R_FLAME, 0.9f)); cv_eye(35, 46, sh(R_FLAME, 0.9f));
    cv_outline(0);
    cv_store(id);
}

/* The bat on the wing, seen from in front: wings stretched out and down (0) or up (1), scalloped between the fingers */
static void spr_lf_bat(int id, int frame) {
    cv_begin(cv_sprite, TS, TS);
    for (int s = -1; s <= 1; s += 2) {
        float tipx = 32 + s * 30, tipy = frame ? 7.0f : 40.0f, midx = 32 + s * 21, midy = frame ? 11.0f : 46.0f;
        float lowx = 32 + s * 11, lowy = frame ? 44.0f : 52.0f;
        float p[16] = { 32 + s * 4.0f, 26, 32 + s * 12, frame ? 16.0f : 28.0f, tipx, tipy, 32 + s * 25.5f, (tipy + midy) * 0.5f + 5.0f,
                        midx, midy, 32 + s * 16.5f, (midy + lowy) * 0.5f + 4.0f, lowx, lowy, 32 + s * 4.0f, 42 };
        cv_poly(p, 8, R_PURPLE, 0.12f, 0.5f, 6, 46);
        cv_line(32 + s * 4, 27, tipx, tipy, sh(R_WOOD, 0.45f));      /* the arm */
        cv_line(32 + s * 12, frame ? 16.0f : 28.0f, midx, midy, sh(R_WOOD, 0.3f));
        cv_line(32 + s * 10, frame ? 20.0f : 31.0f, lowx, lowy, sh(R_WOOD, 0.3f));
    }
    cv_ellipse(32, 35, 5.0f, 9.5f, R_WOOD, 0.2f, 0.6f);
    cv_ellipse(32, 24, 5.0f, 4.6f, R_WOOD, 0.25f, 0.65f);
    for (int s = -1; s <= 1; s += 2) { float p[6] = { 32.0f + s * 2.5f, 21, 32.0f + s * 6.5f, 12, 32.0f + s * 5.5f, 24 }; cv_poly(p, 3, R_SKIN, 0.2f, 0.55f, 12, 12); }
    cv_px(31, 27, sh(R_BLOOD, 0.1f)); cv_px(33, 27, sh(R_BLOOD, 0.1f));
    cv_eye(30, 23, sh(R_FLAME, 0.9f)); cv_eye(34, 23, sh(R_FLAME, 0.9f));
    cv_outline(0);
    cv_store(id);
}

/* A beetle in profile, facing left: a glossy domed shell with the seam between its wing-cases, a small head and horns, six legs of which
 * three show, in two steps. R_IRON with a hard highlight; a black that is not quite black. */
static void spr_beetle(int id, int frame) {
    cv_begin(cv_sprite, TS, TS);
    for (int l = 0; l < 3; l++) {
        float fx = 22 + l * 9.0f, ph = ((l + frame) & 1) ? 2.5f : -2.5f;
        cv_line(fx + 2, 49, fx + ph - 3, 55, sh(R_IRON, 0.3f)); cv_line(fx + ph - 3, 55, fx + ph - 5, 59, sh(R_IRON, 0.2f));
    }
    cv_ellipse(37, 42, 20.0f, 15.0f, R_IRON, 0.05f, 0.82f);          /* the shell */
    cv_line(21, 40, 55, 40, sh(R_IRON, 0.04f));                      /* the seam, along its top */
    for (int x = 26; x < 54; x += 6) cv_px(x, 36 + (x & 1), sh(R_IRON, 0.95f));   /* a hard glint on the lit side */
    cv_px(27, 33, sh(R_BONE, 0.95f)); cv_px(28, 33, sh(R_BONE, 0.8f)); cv_px(26, 34, sh(R_BONE, 0.8f));
    cv_ellipse(14, 46, 7.0f, 6.0f, R_IRON, 0.1f, 0.55f);             /* the head */
    cv_capsule(8, 41, 4, 36, 1.4f, R_IRON, 0.2f, 0.55f);             /* a horn */
    cv_line(11, 41, 5, 32, sh(R_IRON, 0.35f)); cv_line(5, 32, 2, 30, sh(R_IRON, 0.35f));   /* an antenna */
    cv_px(9, 49, sh(R_BONE, 0.5f)); cv_px(8, 50, sh(R_BONE, 0.5f));  /* the jaws */
    cv_eye(12, 43, sh(R_FLAME, 0.7f));
    cv_outline(0);
    cv_store(id);
}

/* A woodlouse in profile, facing left, in its armour of plates, grey and mealy; seven plates laid like tiles and a pair of tails behind */
static void spr_louse(int id, int frame) {
    cv_begin(cv_sprite, TS, TS);
    for (int l = 0; l < 7; l++) {
        float x = 20 + l * 5.4f, ph = ((l + frame) & 1) ? 1.8f : -1.8f;
        cv_line(x, 50, x + ph, 55, sh(R_STONE, 0.5f)); cv_line(x + ph, 55, x + ph - 1, 58, sh(R_STONE, 0.4f));
    }
    for (int i = 0; i < 7; i++) {                                    /* plates, the middle ones tallest, from the back forward */
        float cx = 48.0f - i * 4.4f, h = 12.0f - fabsf(i - 3.0f) * 1.6f, ry = h;
        cv_ellipse(cx + 2, 46 - ry * 0.5f + 5, 6.4f, ry * 0.95f, R_STONE, 0.12f, 0.58f);
        cv_line(cx - 3.5f, 46 - ry * 0.5f + 5 + ry * 0.8f, cx + 6, 46 - ry * 0.5f + 5 + ry * 0.8f, sh(R_STONE, 0.07f));
    }
    cv_ellipse(14, 44, 5.0f, 6.0f, R_STONE, 0.2f, 0.62f);            /* the head */
    cv_line(11, 40, 4, 33, sh(R_STONE, 0.5f)); cv_line(13, 39, 9, 31, sh(R_STONE, 0.5f));   /* the long antennae */
    cv_line(52, 52, 58, 56, sh(R_STONE, 0.4f)); cv_line(51, 54, 57, 59, sh(R_STONE, 0.4f));
    cv_outline(0);
    cv_store(id);
}

/* A toad, from in front: squat and warty, a pale throat, the eyes raised on its brow and gold, a mouth wide enough to be a joke.
 * Sitting (0), or in a hop with its legs thrown out behind (1). R_WOOD with R_MOSS on its warts. */
static void spr_toad(int id, int frame) {
    cv_begin(cv_sprite, TS, TS);
    if (frame == 0) {
        cv_ellipse(32, 44, 21.0f, 15.0f, R_WOOD, 0.1f, 0.62f);
        for (int s = -1; s <= 1; s += 2) {
            cv_ellipse(32 + s * 18.0f, 50, 8.0f, 9.0f, R_WOOD, 0.08f, 0.55f);       /* the folded haunches */
            cv_capsule(32 + s * 9.0f, 52, 32 + s * 11.0f, 59, 2.6f, R_WOOD, 0.1f, 0.5f);   /* the front legs */
            cv_ellipse(32 + s * 11.0f, 59.5f, 4.0f, 1.8f, R_WOOD, 0.1f, 0.5f);
        }
        cv_ellipse(32, 46, 9.0f, 8.0f, R_BONE, 0.18f, 0.5f);         /* the pale throat and belly */
        cv_line(20, 38, 44, 38, sh(R_WOOD, 0.04f));                  /* the mouth */
        cv_line(20, 38, 17, 36, sh(R_WOOD, 0.04f)); cv_line(44, 38, 47, 36, sh(R_WOOD, 0.04f));
        cv_ellipse(32, 32, 14.0f, 8.5f, R_WOOD, 0.2f, 0.66f);        /* the head */
        for (int s = -1; s <= 1; s += 2) {
            cv_ellipse(32 + s * 8.5f, 25.5f, 5.0f, 4.6f, R_WOOD, 0.25f, 0.7f);     /* the brow, raised */
            cv_flat(32 + s * 8.5f, 25.5f, 3.2f, 2.8f, sh(R_GOLD, 0.65f));
            cv_flat(32 + s * 8.5f, 25.5f, 3.2f, 1.0f, sh(R_GOLD, 0.2f));            /* the pupil, a bar */
            cv_eye((int)(32 + s * 8.5f - (s < 0 ? 1 : 0)), 25, sh(R_GOLD, 0.95f));
        }
        cv_px(31, 30, sh(R_WOOD, 0.02f)); cv_px(33, 30, sh(R_WOOD, 0.02f));
    } else {
        cv_ellipse(32, 30, 13.0f, 15.0f, R_WOOD, 0.1f, 0.62f);       /* in the air, long */
        cv_ellipse(32, 36, 7.5f, 9.0f, R_BONE, 0.18f, 0.5f);
        for (int s = -1; s <= 1; s += 2) {
            cv_capsule(32 + s * 10, 40, 32 + s * 21, 51, 3.4f, R_WOOD, 0.1f, 0.55f);        /* the thighs out */
            cv_capsule(32 + s * 21, 51, 32 + s * 24, 60, 2.2f, R_WOOD, 0.1f, 0.5f);
            cv_ellipse(32 + s * 24.5f, 60, 4.2f, 1.6f, R_WOOD, 0.1f, 0.5f);
            cv_capsule(32 + s * 9, 38, 32 + s * 15, 28, 2.2f, R_WOOD, 0.1f, 0.5f);          /* the front legs reaching */
        }
        for (int s = -1; s <= 1; s += 2) {
            cv_ellipse(32 + s * 7.5f, 17.5f, 4.6f, 4.2f, R_WOOD, 0.25f, 0.7f);
            cv_flat(32 + s * 7.5f, 17.5f, 3.0f, 2.6f, sh(R_GOLD, 0.65f));
            cv_eye((int)(32 + s * 7.5f - (s < 0 ? 1 : 0)), 17, sh(R_GOLD, 0.95f));
        }
        cv_line(22, 28, 42, 28, sh(R_WOOD, 0.04f));
    }
    for (int y = 0; y < TS; y++) for (int x = 0; x < TS; x++) {      /* warts: a few paler humps, a few darker pits */
        uint8_t c = cv_sprite[y * TS + x];
        if (c < R_WOOD || c >= R_WOOD + 16) continue;
        float h = h01(x, y, 721);
        if (h > 0.9f) cv_sprite[y * TS + x] = sh(R_MOSS, 0.3f + 0.3f * h01(x, y, 722));
        else if (h < 0.05f) cv_sprite[y * TS + x] = (uint8_t)(c > R_WOOD + 2 ? c - 2 : c);
    }
    cv_outline(0);
    cv_store(id);
}

/* Glowing fungus: a cluster of caps on short stalks, the gills under each showing as a glow. Cold, green-blue; the glow itself full
 * bright (R_GLOW), the caps and stalks in the moss's ramp and lit as stone is. */
static void spr_lf_fungus(int id, int big) {
    cv_begin(cv_sprite, TS, TS);
    static const float CAP[5][4] = { { 32, 40, 12, 7 }, { 17, 49, 8, 5 }, { 47, 47, 9, 5.5f }, { 25, 28, 6, 4 }, { 41, 31, 5, 3.5f } };
    for (int i = big ? 0 : 1; i < (big ? 5 : 3); i++) {
        float cx = CAP[i][0], cy = CAP[i][1], rx = CAP[i][2], ry = CAP[i][3];
        cv_capsule(cx, cy + ry * 0.4f, cx + (i - 2) * 0.8f, 58, 1.8f + rx * 0.12f, R_BONE, 0.25f, 0.6f);   /* the stalk */
        cv_ellipse(cx, cy, rx, ry, R_MOSS, 0.12f, 0.62f);                                                  /* the cap */
        for (int k = 0; k < (int)rx; k++) {                                                                /* the gills' glow along its rim */
            float a = PI_F * (0.1f + 0.8f * k / rx);
            cv_px((int)(cx - cosf(a) * rx * 0.88f), (int)(cy + sinf(a) * ry * 0.55f + 1), (uint8_t)(R_GLOW + 8 + (k & 3)));
        }
        cv_px((int)(cx - rx * 0.35f), (int)(cy - ry * 0.45f), sh(R_MOSS, 0.9f));
    }
    cv_outline(0);
    cv_store(id);
}
static void spr_spore(int id) {                                      /* a mote: a point of the glow and a dim halo round it */
    cv_begin(cv_sprite, TS, TS);
    cv_flat(32, 32, 9.0f, 9.0f, (uint8_t)(R_GLOW + 3)); cv_flat(32, 32, 6.0f, 6.0f, (uint8_t)(R_GLOW + 7)); cv_flat(32, 32, 3.0f, 3.0f, (uint8_t)(R_GLOW + 13));
    cv_store(id);
}

/* Water on the floor, seen from a standing height so it lies flat and wide: a puddle (a dark mirror with a pale streak and a rim of
 * wet stone), three stages of a ring spreading, a boot's wet print left and right, a drop falling */
static void spr_puddle(int id) {
    cv_begin(cv_sprite, TS, TS);
    cv_flat(32, 46, 29.0f, 8.5f, sh(R_STONE, 0.18f));                /* the wet rim */
    cv_flat(32, 46.5f, 26.0f, 6.8f, sh(R_FLOOR, 0.05f));             /* the water, dark */
    cv_flat(32, 45.5f, 24.0f, 5.2f, sh(R_GHOST, 0.12f));
    for (int x = 14; x < 34; x++) cv_px(x, 44 + ((x / 5) & 1), sh(R_GHOST, 0.55f));   /* a pale streak of the ceiling in it */
    for (int x = 38; x < 46; x++) cv_px(x, 47, sh(R_GHOST, 0.4f));
    cv_store(id);
}
static void spr_ring(int id, int stage) {
    cv_begin(cv_sprite, TS, TS);
    static const float RX[3] = { 7.0f, 14.0f, 25.0f }, RY[3] = { 1.7f, 3.4f, 6.0f };
    uint8_t c = sh(R_GHOST, 0.78f - 0.22f * stage);
    for (int k = 0; k < 96; k++) {
        float a = TAU_F * k / 96.0f;
        cv_px((int)(32 + cosf(a) * RX[stage]), (int)(46 + sinf(a) * RY[stage]), c);
        if (stage < 2 && k % 3 == 0) cv_px((int)(32 + cosf(a) * (RX[stage] + 1.4f)), (int)(46 + sinf(a) * (RY[stage] + 0.6f)), sh(R_GHOST, 0.3f));
    }
    cv_store(id);
}
static void spr_print(int id, int right) {                           /* a boot's: a sole and a heel, wet and dark, a sheen on it */
    cv_begin(cv_sprite, TS, TS);
    int d = right ? 1 : -1;
    cv_flat(32 + d * 1.0f, 44, 3.6f, 9.0f, sh(R_FLOOR, 0.06f));
    cv_flat(32 + d * 0.5f, 55, 2.8f, 4.2f, sh(R_FLOOR, 0.06f));
    cv_flat(32 + d * 0.6f, 43, 1.4f, 5.5f, sh(R_GHOST, 0.14f));
    cv_store(id);
}
static void spr_drip(int id) {                                       /* a drop in the air: a bead with a streak above it */
    cv_begin(cv_sprite, TS, TS);
    for (int y = 14; y < 30; y++) cv_px(32, y, sh(R_GHOST, 0.15f + 0.25f * (y - 14) / 16.0f));
    cv_ellipse(32, 36, 2.8f, 4.6f, R_GHOST, 0.3f, 0.9f);
    cv_px(31, 34, sh(R_GHOST, 1.0f));
    cv_store(id);
}

/* Eyes in a niche's dark: a pair, open (0) and half shut (1), in amber. Nothing else: the dark round them is the niche's. */
static void spr_niche_eyes(int id, int frame) {
    cv_begin(cv_sprite, TS, TS);
    for (int s = -1; s <= 1; s += 2) {
        int x = 32 + s * 7, y = 32;
        if (frame == 0) { cv_eye(x - 1, y, sh(R_GOLD, 1.0f)); cv_px(x, y, sh(R_GOLD, 0.95f)); cv_px(x + 1, y, sh(R_GOLD, 0.8f)); cv_px(x - 1, y - 1, sh(R_GOLD, 0.7f)); cv_px(x, y - 1, sh(R_GOLD, 0.75f)); cv_px(x - 1, y + 1, sh(R_GOLD, 0.5f)); cv_px(x, y + 1, sh(R_GOLD, 0.6f)); }
        else { cv_eye(x - 1, y, sh(R_GOLD, 0.9f)); cv_px(x, y, sh(R_GOLD, 0.8f)); cv_px(x + 1, y, sh(R_GOLD, 0.6f)); }
    }
    cv_store(id);
}

/* A sleeper's: the skull on its ledge with its jaw shut (0) or a finger's width ajar (1); the hand lying open, then a finger curled */
static void spr_sleep_skull(int id, int ajar) {
    cv_begin(cv_sprite, TS, TS);
    cv_ellipse(32, 52, 20.0f, 4.0f, R_STONE, 0.1f, 0.35f);           /* the dust it lies in */
    cv_ellipse(32, 31, 15.0f, 14.0f, R_BONE, 0.2f, 0.78f);
    cv_ellipse(32, 41, 10.5f, 6.5f, R_BONE, 0.15f, 0.62f);
    cv_flat(25.5f, 32, 4.0f, 4.6f, sh(R_BONE, 0.02f)); cv_flat(38.5f, 32, 4.0f, 4.6f, sh(R_BONE, 0.02f));
    cv_flat(32, 38, 1.6f, 2.4f, sh(R_BONE, 0.02f));
    int jy = ajar ? 4 : 0;
    cv_ellipse(32, 47 + jy, 10.0f, 5.2f, R_BONE, 0.12f, 0.58f);      /* the jaw */
    for (int i = -4; i <= 4; i++) { cv_px(32 + i * 2, 44, sh(R_BONE, 0.9f)); cv_px(32 + i * 2, 45 + jy, sh(R_BONE, 0.82f)); }
    if (ajar) cv_flat(32, 45, 8.0f, 1.8f, sh(R_BONE, 0.01f));
    cv_outline(0);
    cv_store(id);
}
static void spr_sleep_hand(int id, int curled) {
    cv_begin(cv_sprite, TS, TS);
    cv_ellipse(32, 55, 23.0f, 3.5f, R_STONE, 0.1f, 0.35f);
    cv_bone(14, 50, 26, 46, 2.6f, 0.2f, 0.7f); cv_bone(26, 46, 33, 43, 2.4f, 0.2f, 0.7f);   /* the wrist, the back of the hand */
    cv_ellipse(38, 46, 10.0f, 5.5f, R_BONE, 0.2f, 0.7f);
    for (int f = 0; f < 4; f++) {                                    /* four fingers, the second curling up in the second picture */
        float fy = 42.0f + f * 3.0f, len = 12.0f - (f == 1 || f == 2 ? 0.0f : 2.0f);
        if (curled && f == 1) { cv_bone(46, fy, 52, fy - 4, 1.7f, 0.2f, 0.7f); cv_bone(52, fy - 4, 52, fy - 11, 1.5f, 0.2f, 0.7f); cv_bone(52, fy - 11, 48, fy - 15, 1.4f, 0.2f, 0.7f); }
        else { cv_bone(46, fy, 46 + len * 0.55f, fy + 0.5f, 1.7f, 0.2f, 0.7f); cv_bone(46 + len * 0.55f, fy + 0.5f, 46 + len, fy + 1.0f, 1.4f, 0.2f, 0.7f); }
    }
    cv_bone(34, 48, 40, 53, 2.0f, 0.2f, 0.7f);                       /* the thumb */
    cv_outline(0);
    cv_store(id);
}
