/* A part of crawl_art.c (docs/PLAN-tidy.md): it is #included there in its place and never compiled on its own.
 * The crypt's small life (docs/PLAN-ambience.md): moths and flies, the bat, the beetle and the woodlouse, the toad, the glowing fungus and
 * its spores, the puddle and its rings, a wet print and a drop, eyes in a niche, a sleeper's skull and hand. Each is drawn to fill its box,
 * as the mice and the spiderlings are, and shown at a share of a cell by the game. Side views face left and are stored so (the game
 * turns them with DR_FLIPH). Eyes are marked with cv_eye, for the eye pass: bright from anywhere.
 *
 * Drawn as the real thing is made, not as a shape and a ramp: each from its anatomy (the wing's veins, the elytra's grooves, the
 * toad's glands), its surface worked over (grain, scales, fur in strokes, wear), nothing quite alike on its two sides, and a light
 * from the upper left laid over the whole of it last (lf_relight: a rim of it on the edges that face it, shade on those that do not). */

static int lf_base(uint8_t c) { return (c < 1 || c >= R_FLAME) ? -1 : ((c - 1) >> 4 << 4) + 1; }
static int lf_clear(int x, int y) { return (unsigned)x >= (unsigned)TS || (unsigned)y >= (unsigned)TS || cv_sprite[y * TS + x] == C_CLEAR; }
static void lf_step(int x, int y, int d) {                           /* a pixel d steps up (+) or down (-) its own ramp */
    if ((unsigned)x >= (unsigned)TS || (unsigned)y >= (unsigned)TS) return;
    uint8_t c = cv_sprite[y * TS + x]; int b = lf_base(c);
    if (b >= 0) cv_sprite[y * TS + x] = (uint8_t)(b + clampi(c - b + d, 0, 15));
}
static void lf_relight(int up, int dn) {                             /* the key light: up and to the left */
    static uint8_t t[TS * TS];
    memcpy(t, cv_sprite, sizeof t);
    for (int y = 0; y < TS; y++)
        for (int x = 0; x < TS; x++) {
            uint8_t c = t[y * TS + x]; int b = lf_base(c);
            if (b < 0) continue;
            int d = 0;
            if (x == 0 || t[y * TS + x - 1] == C_CLEAR || y == 0 || t[(y - 1) * TS + x] == C_CLEAR) d += up;
            if (x == TS - 1 || t[y * TS + x + 1] == C_CLEAR || y == TS - 1 || t[(y + 1) * TS + x] == C_CLEAR) d -= dn;
            if (d) cv_sprite[y * TS + x] = (uint8_t)(b + clampi(c - b + d, 0, 15));
        }
}
static void lf_grain(uint32_t seed, float amt) {                     /* every pixel of a ramp a step either way, now and then */
    for (int y = 0; y < TS; y++)
        for (int x = 0; x < TS; x++) {
            float h = h01(x, y, seed);
            if (h > 1.0f - amt) lf_step(x, y, 1); else if (h < amt) lf_step(x, y, -1);
        }
}
static void lf_stroke(float x, float y, float dx, float dy, int len, int d) {   /* a short stroke of hair or scale, a step lighter or darker */
    for (int i = 0; i < len; i++) lf_step((int)(x + dx * i), (int)(y + dy * i), d);
}
static void lf_fur(float cx, float cy, float rx, float ry, uint32_t seed, int n, float dx, float dy, int len) {   /* fur in strokes over an ellipse */
    for (int i = 0; i < n; i++) {
        float a = TAU_F * h01(i, 1, seed), r = sqrtf(h01(i, 2, seed));
        float x = cx + cosf(a) * rx * r, y = cy + sinf(a) * ry * r;
        float ox = dx + (h01(i, 3, seed) - 0.5f) * 0.8f, oy = dy + (h01(i, 4, seed) - 0.5f) * 0.8f;
        lf_stroke(x, y, ox, oy, len, (i & 1) ? 2 : -2);
    }
}
static void lf_nibble(int n, uint32_t seed) {                        /* an edge worn or eaten: a few bites out of the silhouette */
    for (int i = 0; i < n; i++) {
        int x = (int)(h01(i, 7, seed) * 60) + 2, y = (int)(h01(i, 8, seed) * 60) + 2;
        for (int k = 0; k < 90; k++) {                               /* walk in from where it was picked to the nearest edge */
            int ok = 0;
            for (int d = 0; d < 4 && !ok; d++) { int nx = x + (d == 0) - (d == 1), ny = y + (d == 2) - (d == 3); if (lf_clear(nx, ny) && !lf_clear(x, y)) ok = 1; }
            if (ok) { cv_px(x, y, C_CLEAR); if (h01(x, y, seed) > 0.4f) cv_px(x + 1, y, C_CLEAR); break; }
            x += (int)((32 - x) * 0.04f) + (h01(k, 9, seed) > 0.5f ? 1 : -1) * 0; y += 1 - 2 * (y > 32);
        }
    }
}

/* a wing painted: its polygon laid, then each pixel from how far it is from the root of the wing: bands across it, a darker margin,
 * pale veins running out to each tip (the tips are the polygon's points from tip0 for ntip of them) */
static void lf_wing_paint(const float *p, int n, float rx, float ry, uint32_t seed, int ramp, float lo, float hi, float band, int tip0, int ntip, float vein) {
    float rmax = 0.0f;
    for (int i = 0; i < n; i++) rmax = fmaxf(rmax, hypotf(p[2 * i] - rx, p[2 * i + 1] - ry));
    cv_poly(p, n, ramp, lo, hi, 0, 64);
    for (int y = 0; y < TS; y++)
        for (int x = 0; x < TS; x++) {
            uint8_t c = cv_sprite[y * TS + x];
            if (c == C_CLEAR || c < ramp || c >= ramp + 16) continue;
            float r = hypotf(x - rx, y - ry) / rmax, w = h01(x / 2, y / 2, seed) - 0.5f;
            float v = lo + (hi - lo) * (0.55f + 0.35f * (1.0f - r));
            float bd = sinf(r * 11.0f + w * 2.6f);                   /* the cross-bands */
            if (bd > 0.55f) v -= band; else if (bd < -0.75f) v += band * 0.6f;
            if (r > 0.9f) v -= band * 0.7f;                          /* the margin darkens */
            v += h01(x, y, seed + 1) * 0.12f - 0.06f;
            cv_sprite[y * TS + x] = sh(ramp, v);
        }
    for (int i = 0; i < ntip; i++) {                                 /* the veins, fading out */
        float tx = p[2 * (tip0 + i)], ty = p[2 * (tip0 + i) + 1];
        int m = (int)hypotf(tx - rx, ty - ry);
        for (int k = 2; k < m; k++) {
            float f = (float)k / (float)m, x = rx + (tx - rx) * f, y = ry + (ty - ry) * f;
            if (!lf_clear((int)x, (int)y) && !(k & 3 && f > 0.7f)) lf_step((int)x, (int)y, vein > 0 ? 3 : -3);
        }
    }
}

/* A moth from in front, wings spread (0) or held up in a V (1): the forewing over the hindwing, each banded across and darkened at the
 * margin, an eyespot on the hindwing, nibbled at the edges, the thorax in fur, a tapering ringed abdomen and feathered antennae.
 * It is not quite alike on its two sides. */
static void spr_moth(int id, int frame) {
    cv_begin(cv_sprite, TS, TS);
    static const float FW0[] = { 30, 30, 20, 21, 9, 17, 3, 24, 4, 33, 8, 39, 15, 40, 24, 38, 29, 36 };       /* forewing, the left one, spread */
    static const float HW0[] = { 30, 34, 20, 40, 11, 44, 8, 52, 14, 58, 23, 56, 29, 49, 31, 41 };            /* the hindwing under it */
    static const float FW1[] = { 30, 30, 26, 16, 20, 6, 12, 3, 8, 10, 9, 19, 14, 28, 22, 34, 29, 37 };
    static const float HW1[] = { 30, 36, 24, 40, 17, 45, 15, 53, 21, 57, 27, 53, 30, 46 };
    const float *fw = frame ? FW1 : FW0, *hw = frame ? HW1 : HW0;
    int nf = 9, nh = frame ? 7 : 8;
    float rf[18], rh[16];
    for (int i = 0; i < nf; i++) { rf[2 * i] = 63.0f - fw[2 * i]; rf[2 * i + 1] = fw[2 * i + 1] + (i == 3 ? 1.0f : 0.0f); }   /* the right one a little askew */
    for (int i = 0; i < nh; i++) { rh[2 * i] = 63.0f - hw[2 * i]; rh[2 * i + 1] = hw[2 * i + 1] - 1.0f; }
    lf_wing_paint(hw, nh, 30, 35, 731, R_WARM, 0.18f, 0.62f, 0.22f, 2, nh - 3, 0);
    lf_wing_paint(rh, nh, 33, 35, 732, R_WARM, 0.18f, 0.62f, 0.22f, 2, nh - 3, 0);
    for (int s = 0; s < 2; s++) {                                    /* the eyespot on the hindwing: ring, iris, pupil, a glint */
        float x = (s ? 63.0f - 15.5f : 15.5f), y = frame ? 49.0f : 49.0f;
        cv_flat(x, y, 4.4f, 4.4f, sh(R_BONE, 0.12f)); cv_flat(x, y, 3.4f, 3.4f, sh(R_GOLD, 0.45f)); cv_flat(x, y, 2.0f, 2.0f, sh(R_BONE, 0.05f));
        cv_px((int)x - 1, (int)y - 1, sh(R_BONE, 0.95f));
    }
    lf_wing_paint(fw, nf, 30, 30, 733, R_BONE, 0.2f, 0.74f, 0.26f, 1, nf - 3, 1);
    lf_wing_paint(rf, nf, 33, 30, 734, R_BONE, 0.2f, 0.74f, 0.26f, 1, nf - 3, 1);
    for (int s = 0; s < 2; s++) {                                    /* the zigzag line across each forewing */
        const float *w = s ? rf : fw;
        for (int i = 2; i < nf - 2; i++) {
            float x0 = w[2 * i] + (30.5f - w[2 * i]) * 0.32f, y0 = w[2 * i + 1] + (31.0f - w[2 * i + 1]) * 0.32f, x1 = w[2 * i + 2] + (30.5f - w[2 * i + 2]) * 0.32f, y1 = w[2 * i + 3] + (31.0f - w[2 * i + 3]) * 0.32f;
            for (int k = 0; k < 4; k++) { float f = k / 4.0f; if ((i + k) & 1) lf_step((int)(x0 + (x1 - x0) * f), (int)(y0 + (y1 - y0) * f), -5); }
        }
    }
    cv_capsule(32, 37, 32, 52, 2.8f, R_WARM, 0.15f, 0.55f);          /* the abdomen, tapering, ringed */
    cv_capsule(32, 45, 32, 56, 1.6f, R_WARM, 0.12f, 0.5f);
    for (int y = 39; y < 56; y += 3) cv_line(30.0f + (y > 46) * 1.0f, y, 34.0f - (y > 46) * 1.0f, y, sh(R_WARM, 0.08f));
    cv_ellipse(32, 31, 5.0f, 5.6f, R_WARM, 0.2f, 0.72f);             /* the thorax, in fur */
    lf_fur(32, 31, 4.5f, 5.0f, 735, 46, 0, 1, 3);
    cv_ellipse(32, 25, 3.2f, 2.8f, R_WARM, 0.2f, 0.6f);              /* the head, its two dark eyes */
    cv_px(30, 25, sh(R_IRON, 0.02f)); cv_px(34, 25, sh(R_IRON, 0.02f)); cv_px(30, 24, sh(R_BONE, 0.7f));
    for (int s = -1; s <= 1; s += 2) {                               /* the antennae: a shaft, and teeth along it like a comb */
        float x0 = 32 + s * 1.5f, y0 = 23;
        for (int k = 0; k < 9; k++) {
            float f = k / 8.0f, bx = x0 + s * (2 + 8 * f) * (0.6f + 0.4f * f), by = y0 - 3 - 12 * f + 7 * f * f;
            cv_px((int)bx, (int)by, sh(R_WARM, 0.5f));
            if (k > 0 && k < 8) cv_line(bx, by, bx + s * 1.6f, by - 2.0f - (k & 1), sh(R_WARM, 0.32f));
        }
    }
    lf_nibble(3, 739);
    lf_relight(3, 3);
    cv_outline(0);
    cv_store(id);
}

/* A fly from in front, in the air: a bristled thorax with a metal sheen, the abdomen banded and bristled, two great red compound eyes
 * faceted and lit, six jointed legs hanging, the two wings glassy with their veins and a faint rainbow of thin film, held out (0) or beating
 * (1) when they blur */
static void spr_fly(int id, int frame) {
    cv_begin(cv_sprite, TS, TS);
    for (int s = -1; s <= 1; s += 2) {                               /* the legs, each a thigh, a shin and a foot */
        for (int l = 0; l < 3; l++) {
            float x0 = 32 + s * 3.0f, y0 = 30 + l * 3.0f, kx = x0 + s * (7 + l * 1.5f), ky = y0 + 4 + l * 2.0f, fx = kx + s * (2 + l), fy = ky + 7 + l;
            cv_line(x0, y0, kx, ky, sh(R_IRON, 0.22f)); cv_line(kx, ky, fx, fy, sh(R_IRON, 0.16f));
            cv_px((int)fx, (int)fy + 1, sh(R_IRON, 0.3f)); cv_px((int)kx, (int)ky, sh(R_IRON, 0.45f));
        }
    }
    static const float W0[] = { 30, 29, 22, 19, 11, 14, 4, 20, 5, 29, 14, 36, 25, 36 };
    static const float W1[] = { 30, 29, 27, 17, 21, 6, 13, 3, 10, 10, 15, 21, 24, 29 };
    const float *w = frame ? W1 : W0;
    float rw[14];
    for (int i = 0; i < 7; i++) { rw[2 * i] = 63.0f - w[2 * i] + (i == 3 ? 1 : 0); rw[2 * i + 1] = w[2 * i + 1]; }
    for (int s = 0; s < 2; s++) {
        const float *q = s ? rw : w;
        cv_poly(q, 7, R_GHOST, 0.46f, 0.82f, 4, 34);
        for (int i = 2; i < 6; i++) cv_line(s ? 33.0f : 30.0f, 29, q[2 * i], q[2 * i + 1], sh(R_GHOST, 0.28f));     /* the veins */
        cv_line(q[6], q[7], q[8], q[9], sh(R_GHOST, 0.22f));
        for (int y = 0; y < TS; y++) for (int x = 0; x < TS; x++) {     /* thin film: a rainbow at the thick edge, a clear middle */
            uint8_t c = cv_sprite[y * TS + x];
            if (c < R_GHOST || c >= R_GHOST + 16 || frame) continue;
            float h = h01(x, y, 741), edge = hypotf(x - 32.0f, y - 29.0f) / 28.0f;
            if (h > 0.93f - 0.1f * edge) cv_sprite[y * TS + x] = sh(R_PURPLE, 0.5f); else if (h < 0.04f + 0.06f * edge) cv_sprite[y * TS + x] = sh(R_MOSS, 0.55f);
        }
    }
    cv_ellipse(32, 42, 5.6f, 10.0f, R_IRON, 0.08f, 0.5f);            /* the abdomen */
    for (int y = 36; y < 51; y += 4) { cv_line(27, y, 37, y, sh(R_IRON, 0.03f)); cv_line(28, y + 1, 36, y + 1, sh(R_IRON, 0.32f)); }
    for (int k = 0; k < 14; k++) { float a = 0.3f + 2.5f * k / 14.0f; cv_px((int)(32 + cosf(a + 1.57f) * 6.2f), (int)(42 + sinf(a + 1.57f) * 10.6f), sh(R_IRON, 0.58f)); }
    cv_ellipse(32, 30, 6.4f, 5.8f, R_WOOD, 0.12f, 0.78f);            /* the thorax: the sheen of bronze and green on it, in stripes */
    for (int x = 28; x < 37; x += 3) cv_line(x, 26, x + (x > 32), 34, sh(R_IRON, 0.1f));
    for (int k = 0; k < 8; k++) cv_px(26 + k * 1.5f, 27 + (k & 1) * 6, sh(R_MOSS, 0.6f));
    cv_ellipse(32, 23, 3.8f, 3.2f, R_IRON, 0.12f, 0.4f);
    for (int s = -1; s <= 1; s += 2) {                               /* the eyes: red, faceted, a bright spot on each */
        float ex = 32 + s * 4.4f;
        cv_ellipse(ex, 22.5f, 3.6f, 4.2f, R_BLOOD, 0.28f, 0.82f);
        for (int y = 19; y < 27; y += 2) for (int x = (int)ex - 3; x <= (int)ex + 3; x += 2) if (!lf_clear(x, y) && ((x + y) & 3) == 0) lf_step(x, y, -3);
        cv_px((int)ex - 1, 20, sh(R_BLOOD, 1.0f)); cv_px((int)ex - 1, 21, sh(R_FLAME, 0.4f));
    }
    for (int k = 0; k < 18; k++) { float a = TAU_F * k / 18.0f; cv_px((int)(32 + cosf(a) * 7.0f), (int)(30 + sinf(a) * 6.4f), sh(R_IRON, 0.55f)); }   /* the bristles */
    cv_line(32, 19, 32, 15, sh(R_IRON, 0.4f)); cv_px(31, 15, sh(R_IRON, 0.5f)); cv_px(33, 15, sh(R_IRON, 0.5f));
    lf_relight(3, 3);
    cv_outline(0);
    cv_store(id);
}

/* A bat's wing laid on the page: the arm from the shoulder, the wrist, four fingers fanned from it, the membrane between them scalloped
 * and veined, a thumb with its claw above the wrist. s is -1 or 1, the side it is drawn on. */
static void lf_bat_wing(int s, float sx, float sy, float wx, float wy, const float *tip, float hx, float hy, int frame, uint32_t seed) {
    float p[20]; int n = 0;
    p[n++] = sx; p[n++] = sy; p[n++] = wx; p[n++] = wy;
    for (int i = 0; i < 4; i++) {
        p[n++] = tip[2 * i]; p[n++] = tip[2 * i + 1];
        if (i < 3) {                                                 /* the scallop between this tip and the next, pulled in toward the wrist */
            float mx = (tip[2 * i] + tip[2 * i + 2]) * 0.5f, my = (tip[2 * i + 1] + tip[2 * i + 3]) * 0.5f;
            p[n++] = mx + (wx - mx) * 0.24f; p[n++] = my + (wy - my) * 0.24f;
        }
    }
    p[n++] = hx; p[n++] = hy;
    cv_poly(p, n / 2, R_PURPLE, 0.1f, 0.56f, 4, 52);
    for (int y = 0; y < TS; y++) for (int x = 0; x < TS; x++) {      /* the membrane thin and lit near its edge, dark and veined near the arm */
        uint8_t c = cv_sprite[y * TS + x];
        if (c < R_PURPLE || c >= R_PURPLE + 16) continue;
        float h = h01(x, y, seed);
        if (h > 0.85f) lf_step(x, y, 2); else if (h < 0.1f) lf_step(x, y, -2);
    }
    for (int i = 0; i < 4; i++) {                                    /* the fingers: thin bones, lighter, with a knuckle at the wrist */
        cv_line(wx, wy, tip[2 * i], tip[2 * i + 1], sh(R_WOOD, 0.5f - 0.05f * i));
        cv_px((int)(wx + (tip[2 * i] - wx) * 0.5f), (int)(wy + (tip[2 * i + 1] - wy) * 0.5f), sh(R_WOOD, 0.72f));
    }
    cv_line(wx, wy, hx, hy, sh(R_PURPLE, 0.3f));
    cv_capsule(sx, sy, wx, wy, 1.6f, R_WOOD, 0.22f, 0.66f);          /* the arm, furred */
    cv_capsule(wx, wy, wx + s * 2.5f, wy - 4.0f, 0.9f, R_BONE, 0.4f, 0.8f); cv_px((int)(wx + s * 3.0f), (int)(wy - 5.0f), sh(R_BONE, 0.95f));   /* the thumb's claw */
    (void)frame;
}
static void lf_bat_head(float cx, float cy, int open) {              /* a bat's face from in front: the long ears, the leaf of the nose, the little fangs */
    cv_ellipse(cx, cy, 5.4f, 5.0f, R_WOOD, 0.22f, 0.7f);
    for (int s = -1; s <= 1; s += 2) {
        float p[8] = { cx + s * 2.2f, cy - 3.0f, cx + s * 4.2f, cy - 11.0f, cx + s * 7.8f, cy - 12.0f, cx + s * 6.2f, cy - 1.0f };
        cv_poly(p, 4, R_SKIN, 0.25f, 0.58f, cy - 12.0f, 11);
        cv_line(cx + s * 4.2f, cy - 8.0f, cx + s * 5.6f, cy - 3.0f, sh(R_SKIN, 0.55f)); cv_line(cx + s * 5.4f, cy - 9.0f, cx + s * 6.2f, cy - 3.0f, sh(R_SKIN, 0.18f));   /* the ear's ribs */
    }
    cv_ellipse(cx, cy + 2.2f, 2.8f, 2.0f, R_SKIN, 0.32f, 0.7f);
    cv_px((int)cx - 1, (int)cy + 2, sh(R_BLOOD, 0.08f)); cv_px((int)cx, (int)cy + 2, sh(R_BLOOD, 0.08f));
    if (open) { cv_line(cx - 2, cy + 4, cx + 2, cy + 4, sh(R_BLOOD, 0.06f)); cv_px((int)cx - 2, (int)cy + 5, sh(R_BONE, 1.0f)); cv_px((int)cx + 1, (int)cy + 5, sh(R_BONE, 1.0f)); }
    cv_eye((int)cx - 3, (int)cy - 1, sh(R_FLAME, 0.92f)); cv_eye((int)cx + 2, (int)cy - 1, sh(R_FLAME, 0.92f));
}

/* The bat roosting: hung head down by hooked feet from a lip of stone, the wings drawn about it as a cloak, the finger bones ridging them,
 * the thumbs' claws out at the shoulders, the fur of the chest in strokes, the face turned to you, its mouth a little open. */
static void spr_lf_bat_roost(int id) {
    cv_begin(cv_sprite, TS, TS);
    cv_flat(32, 2, 18.0f, 2.2f, sh(R_STONE, 0.3f)); cv_line(14, 3, 50, 3, sh(R_STONE, 0.12f));   /* the lip it hangs from */
    for (int s = -1; s <= 1; s += 2) {                               /* the feet: a leg, and four small hooked toes */
        cv_capsule(32 + s * 3, 4, 32 + s * 4.5f, 12, 1.2f, R_SKIN, 0.2f, 0.6f);
        for (int k = 0; k < 3; k++) { cv_line(32 + s * (2 + k * 1.8f), 3, 32 + s * (3 + k * 1.8f), 6, sh(R_SKIN, 0.4f)); cv_px((int)(32 + s * (3.5f + k * 1.8f)), 7, sh(R_BONE, 0.8f)); }
    }
    cv_ellipse(32, 29, 12.0f, 19.0f, R_WOOD, 0.1f, 0.58f);           /* the body, then the wings over it */
    for (int s = -1; s <= 1; s += 2) {
        float p[12] = { 32.0f + s * 3, 9, 32.0f + s * 14, 14, 32.0f + s * 16, 30, 32.0f + s * 11, 42, 32.0f + s * 5, 46, 32.0f + s * 2, 28 };
        cv_poly(p, 6, R_PURPLE, 0.1f, 0.5f, 9, 38);
        for (int k = 0; k < 4; k++) cv_line(32.0f + s * (3 + k), 10 + k, 32.0f + s * (11 + k * 1.4f), 36 + k * 2.2f, sh(R_PURPLE, 0.62f - 0.08f * k));   /* the finger bones, ridging it */
        cv_line(32.0f + s * 3, 10, 32.0f + s * 14, 14, sh(R_WOOD, 0.5f));
        cv_px((int)(32 + s * 15), 12, sh(R_BONE, 0.9f)); cv_px((int)(32 + s * 15.5f), 11, sh(R_BONE, 0.9f));    /* the thumb's claw at the shoulder */
    }
    lf_fur(32, 28, 6.0f, 15.0f, 751, 120, 0, 1, 3);                  /* the chest between the wings: fur in strokes, hanging down */
    lf_bat_head(32, 49, 1);
    lf_grain(752, 0.08f);
    lf_nibble(2, 753);
    lf_relight(3, 3);
    cv_outline(0);
    cv_store(id);
}

/* The bat on the wing, from in front: a furred body with the head up between the ears, a fan-fingered wing either side, the feet tucked
 * under a tail membrane that spreads between the legs; the wings thrown down (0) or up (1). A tear in one. */
static void spr_lf_bat(int id, int frame) {
    cv_begin(cv_sprite, TS, TS);
    static const float DL[8] = { 30, 27, 28, 43, 20, 53, 12, 54 }, UL[8] = { 30, 8, 24, 2, 16, 3, 10, 11 };
    static const float DR[8] = { 29, 29, 27, 42, 21, 52, 13, 55 }, UR[8] = { 30, 9, 26, 3, 18, 3, 11, 12 };
    for (int s = -1; s <= 1; s += 2) {
        const float *t = s < 0 ? (frame ? UL : DL) : (frame ? UR : DR);
        float tip[8];
        for (int i = 0; i < 4; i++) { tip[2 * i] = 32 + s * t[2 * i] - (s < 0 ? 0 : 0); tip[2 * i + 1] = t[2 * i + 1]; }
        lf_bat_wing(s, 32 + s * 3.0f, 28, 32 + s * (frame ? 15.0f : 18.0f), frame ? 20.0f : 24.0f, tip, 32 + s * 4.0f, 45, frame, s < 0 ? 761 : 762);
    }
    float tm[12] = { 28, 44, 36, 44, 40, 56, 36, 59, 32, 55, 28, 59 };   /* the tail membrane */
    cv_poly(tm, 6, R_PURPLE, 0.1f, 0.5f, 44, 14);
    cv_capsule(32, 28, 32, 46, 4.4f, R_WOOD, 0.2f, 0.66f);           /* the body */
    cv_ellipse(32, 33, 4.6f, 8.0f, R_WOOD, 0.22f, 0.7f);
    lf_fur(32, 36, 4.0f, 9.0f, 763, 60, 0, 1, 3);
    for (int s = -1; s <= 1; s += 2) cv_line(32 + s * 2, 47, 32 + s * 4, 53, sh(R_SKIN, 0.4f));   /* the feet, tucked */
    lf_bat_head(32, 20, 0);
    lf_grain(764, 0.07f);
    if (frame == 0) { for (int k = 0; k < 7; k++) cv_px(20 + k, 51 + (k > 3), C_CLEAR); }          /* a tear in the left wing's edge */
    lf_relight(3, 3);
    cv_outline(0);
    cv_store(id);
}

/* A leg of an insect: a thigh, a shin with spines, a foot; each joint a darker pixel and the knee a lighter one */
static void lf_leg(float x0, float y0, float kx, float ky, float fx, float fy, int ramp, float v) {
    cv_line(x0, y0, kx, ky, sh(ramp, v + 0.1f)); cv_line(x0 + 0.5f, y0, kx + 0.5f, ky, sh(ramp, v));
    cv_line(kx, ky, fx, fy, sh(ramp, v)); cv_px((int)kx, (int)ky, sh(ramp, v + 0.38f));
    float dx = fx - kx, dy = fy - ky, L = hypotf(dx, dy) + 0.01f;
    for (int k = 1; k < 4; k++) { float f = k / 4.0f; cv_px((int)(kx + dx * f - dy / L * 1.4f), (int)(ky + dy * f + dx / L * 1.4f), sh(ramp, v + 0.2f)); }   /* the spines */
    cv_px((int)fx, (int)fy + 1, sh(ramp, v + 0.25f)); cv_px((int)fx - 1, (int)fy + 1, sh(ramp, v + 0.15f));
}

/* A ground beetle in profile, facing left: elytra grooved lengthwise and pitted between, a pronotum of its own with a rolled rim, a head with
 * a pair of sickle jaws, jointed antennae, three legs showing, each in three parts and spined, in two steps. A blue-green sheen at the lit edge of
 * a black carapace and the window's reflection on the dome. */
static void spr_beetle(int id, int frame) {
    cv_begin(cv_sprite, TS, TS);
    for (int l = 0; l < 3; l++) {                                    /* legs behind the body first: far side, dimmer */
        float fx = 24 + l * 9.0f, ph = ((l + frame) & 1) ? 3.0f : -3.0f;
        lf_leg(fx + 2, 49, fx + ph - 5, 54, fx + ph - 8, 59, R_IRON, 0.12f);
    }
    cv_ellipse(40, 41, 18.0f, 14.0f, R_IRON, 0.06f, 0.78f);          /* the elytra: a dome */
    for (int g = 0; g < 7; g++) {                                    /* the grooves, each following the dome from the shoulder to the tail */
        float f = (g + 0.5f) / 7.0f;
        for (int x = 24; x < 58; x++) {
            float nx = (x - 40.0f) / 18.0f; if (fabsf(nx) > 0.97f) continue;
            float yy = 41.0f - 14.0f * sqrtf(fmaxf(0.0f, 1.0f - nx * nx)) * (1.0f - f * 1.0f) + 14.0f * f * 0.55f * (1.0f - nx * nx) * 0.0f;
            yy = 41.0f + 14.0f * (f * 2.0f - 1.0f) * 0.82f * sqrtf(fmaxf(0.0f, 1.0f - nx * nx));
            if (yy < 30.0f || yy > 55.0f) continue;
            lf_step(x, (int)yy, -4);
            if ((x + g) % 3 == 0) lf_step(x, (int)yy + 1, 2);        /* the pits between */
        }
    }
    cv_ellipse(24, 40, 7.0f, 11.0f, R_IRON, 0.08f, 0.74f);           /* the pronotum, a plate before the elytra, its rim rolled */
    for (int y = 30; y < 51; y++) { lf_step(30, y, -5); lf_step(24 - (int)(sqrtf(fmaxf(0.0f, 1.0f - ((y - 40.5f) / 11.0f) * ((y - 40.5f) / 11.0f))) * 7.0f), y, 3); }
    cv_ellipse(14, 45, 6.5f, 5.6f, R_IRON, 0.1f, 0.62f);             /* the head */
    cv_capsule(10, 49, 5, 51, 1.3f, R_IRON, 0.2f, 0.7f); cv_capsule(12, 50, 8, 53, 1.1f, R_IRON, 0.2f, 0.7f);   /* the jaws, crossed */
    cv_px(5, 52, sh(R_BONE, 0.6f)); cv_px(8, 54, sh(R_BONE, 0.55f));
    for (int s = 0; s < 2; s++) {                                    /* an antenna of eleven beads, and a thicker one beyond the knee */
        float x = 12.0f - s * 2.0f, y = 41.0f - s * 1.0f;
        for (int k = 0; k < 8; k++) { x -= 1.5f - s * 0.2f; y -= 1.3f - 0.04f * k; cv_px((int)x, (int)y, sh(R_IRON, 0.3f + 0.05f * k)); if (k == 3 + s) y -= 1.0f; }
    }
    for (int l = 0; l < 3; l++) {                                    /* the near legs */
        float fx = 20 + l * 10.0f, ph = ((l + frame + 1) & 1) ? 3.0f : -3.0f;
        lf_leg(fx + 3, 51, fx + ph - 4, 55, fx + ph - 7, 60, R_IRON, 0.22f);
    }
    for (int k = 0; k < 5; k++) { float f = k / 4.0f; cv_px((int)(30 + f * 15), 33 + (int)(f * 2), sh(R_MOSS, 0.55f + 0.1f * (k & 1))); }   /* the sheen at the lit edge */
    for (int y = 33; y < 38; y++) for (int x = 31; x < 37; x++) if (!lf_clear(x, y) && x + y < 69) lf_step(x, y, 2 + (x + y) % 2);   /* the window in it */
    cv_px(33, 34, sh(R_BONE, 1.0f)); cv_px(34, 34, sh(R_BONE, 0.9f)); cv_px(33, 35, sh(R_BONE, 0.85f));
    cv_eye(11, 43, sh(R_FLAME, 0.7f));
    lf_grain(771, 0.05f);
    lf_relight(3, 4);
    cv_outline(0);
    cv_store(id);
}

/* A woodlouse in profile, facing left: seven tergites overlapping like roof tiles, each with a pale mottled edge and a dark underlap, a
 * granular grey back, the head with its eye and two jointed antennae, the two tails of the last segment, seven pairs of legs under the
 * rim, walking in two steps */
static void spr_louse(int id, int frame) {
    cv_begin(cv_sprite, TS, TS);
    for (int l = 0; l < 7; l++) {                                    /* the legs, under the rim: a thigh and a foot */
        float x = 19 + l * 5.2f, ph = ((l + frame) & 1) ? 2.0f : -2.0f;
        lf_leg(x, 49, x + ph * 0.5f, 53, x + ph - 1, 58, R_STONE, 0.3f);
    }
    for (int i = 0; i < 7; i++) {                                    /* the plates, from the tail forward so each overlaps the one behind it */
        float t = i / 6.0f, cx = 51.0f - i * 5.0f, hh = 7.0f + 4.2f * sinf(t * PI_F), top = 49.0f - 2.0f * hh;
        cv_ellipse(cx, 49.0f - hh, 5.4f, hh, R_STONE, 0.14f, 0.7f);
        for (int y = (int)top; y < 50; y++) for (int x = (int)cx - 6; x < (int)cx + 6; x++) {   /* granular, and a dark line where the next plate laps it */
            if (lf_clear(x, y)) continue;
            if (x == (int)cx - 5 || x == (int)cx - 4) lf_step(x, y, -4);
            if (h01(x, y, 781 + (uint32_t)i) > 0.8f) lf_step(x, y, 2);
        }
        for (int y = (int)(49.0f - hh * 0.4f); y < 50; y++) if (!lf_clear((int)cx + 2, y)) { lf_step((int)cx + 2, y, 3); lf_step((int)cx + 3, y, 2); }   /* the pale mottle at the margin */
        cv_px((int)cx + 1, (int)(49.0f - hh * 1.55f), sh(R_BONE, 0.7f));
    }
    cv_ellipse(14, 46, 4.6f, 5.6f, R_STONE, 0.18f, 0.7f);            /* the head */
    cv_px(12, 44, sh(R_BONE, 0.9f)); cv_px(12, 45, sh(R_IRON, 0.05f));
    for (int s = 0; s < 2; s++) { float x = 11.0f - s * 2.0f, y = 42.0f - s * 2.0f; for (int k = 0; k < 7; k++) { x -= 1.2f; y -= 1.0f + 0.1f * s; cv_px((int)x, (int)y, sh(R_STONE, 0.5f + 0.03f * k)); if (k == 3) y -= 1.0f; } }
    for (int s = 0; s < 2; s++) { cv_line(55, 47 + s * 3, 60, 52 + s * 3, sh(R_STONE, 0.42f)); cv_px(60, 52 + s * 3, sh(R_STONE, 0.6f)); }
    lf_grain(782, 0.07f);
    lf_relight(3, 4);
    cv_outline(0);
    cv_store(id);
}

/* A toad from in front, squat: parotoid glands swelling behind the eyes, warts in clusters each lit above and shadowed below, blotched
 * olive and brown, a pale throat with its folds, nostrils, a mouth wide enough to be a joke, the eyes raised with a gold iris and a bar of
 * pupil and a wet glint, long toes webbed behind; sitting (0) or in a hop with the hind legs thrown out (1). */
static void lf_wart(int x, int y) { cv_px(x, y, sh(R_MOSS, 0.55f)); cv_px(x - 1, y - 1, sh(R_WOOD, 0.8f)); cv_px(x, y + 1, sh(R_WOOD, 0.06f)); cv_px(x + 1, y + 1, sh(R_WOOD, 0.1f)); }
static void lf_toad_eye(float x, float y, int s) {
    cv_ellipse(x, y, 5.2f, 4.8f, R_WOOD, 0.3f, 0.8f);                // the brow
    cv_flat(x, y, 3.5f, 3.1f, sh(R_GOLD, 0.35f)); cv_flat(x, y, 3.0f, 2.6f, sh(R_GOLD, 0.7f)); cv_flat(x, y - 0.4f, 2.8f, 0.9f, sh(R_BONE, 0.05f));   /* the iris, and a bar of pupil */
    cv_eye((int)(x - 1.0f), (int)(y - 2.0f), sh(R_GOLD, 1.0f)); cv_px((int)(x + s), (int)(y + 1), sh(R_GOLD, 0.95f));
    cv_px((int)(x - 2), (int)(y - 2), sh(R_BONE, 1.0f));
}
static void spr_toad(int id, int frame) {
    cv_begin(cv_sprite, TS, TS);
    if (frame == 0) {
        cv_ellipse(32, 44, 22.0f, 15.0f, R_WOOD, 0.1f, 0.62f);
        for (int s = -1; s <= 1; s += 2) {
            cv_ellipse(32 + s * 19.0f, 49, 8.0f, 10.0f, R_WOOD, 0.08f, 0.55f);                      /* the folded haunches */
            cv_line(32 + s * 14.0f, 46, 32 + s * 20.0f, 56, sh(R_WOOD, 0.05f));                     /* the fold of the thigh */
            cv_capsule(32 + s * 9.0f, 50, 32 + s * 10.0f, 58, 2.7f, R_WOOD, 0.1f, 0.52f);           /* the forearm */
            for (int t = 0; t < 4; t++) cv_capsule(32 + s * (7.0f + t * 2.2f), 59, 32 + s * (8.0f + t * 2.8f), 61, 0.9f, R_SKIN, 0.2f, 0.6f);   /* four long toes */
            for (int t = 0; t < 4; t++) cv_px((int)(32 + s * (8.5f + t * 2.8f)), 62, sh(R_SKIN, 0.7f));
        }
        cv_ellipse(32, 46, 10.0f, 8.0f, R_BONE, 0.2f, 0.55f);        /* the throat and belly, pale */
        for (int k = 0; k < 4; k++) cv_line(25 + k * 3, 43 + k % 2, 24 + k * 4, 52, sh(R_BONE, 0.3f));   /* its folds */
        cv_ellipse(32, 32, 15.0f, 8.8f, R_WOOD, 0.2f, 0.66f);        /* the head */
        for (int s = -1; s <= 1; s += 2) cv_ellipse(32 + s * 13.0f, 34.0f, 4.8f, 6.8f, R_WOOD, 0.22f, 0.7f);   /* the glands, swollen */
        cv_line(19, 38, 45, 38, sh(R_WOOD, 0.03f)); cv_line(19, 38, 16, 35, sh(R_WOOD, 0.03f)); cv_line(45, 38, 48, 35, sh(R_WOOD, 0.03f));   /* the mouth */
        for (int k = 0; k < 8; k++) cv_px(20 + k * 3, 39, sh(R_BONE, 0.38f));                     /* its lip, wet */
        cv_px(30, 31, sh(R_WOOD, 0.02f)); cv_px(34, 31, sh(R_WOOD, 0.02f));
        lf_toad_eye(24.0f, 25.0f, -1); lf_toad_eye(40.0f, 25.0f, 1);
    } else {
        cv_ellipse(32, 30, 13.0f, 15.5f, R_WOOD, 0.1f, 0.62f);
        cv_ellipse(32, 36, 7.5f, 9.5f, R_BONE, 0.2f, 0.55f);
        for (int s = -1; s <= 1; s += 2) {
            cv_capsule(32 + s * 10, 40, 32 + s * 21, 51, 3.6f, R_WOOD, 0.1f, 0.56f);                  /* the thigh out */
            cv_capsule(32 + s * 21, 51, 32 + s * 25, 60, 2.2f, R_WOOD, 0.1f, 0.5f);                  /* the shin */
            for (int t = 0; t < 4; t++) { cv_line(32 + s * 25, 60, 32 + s * (22 + t * 3.2f), 63, sh(R_SKIN, 0.4f)); }   /* the toes, splayed */
            cv_poly((float[]){ 32.0f + s * 25, 60, 32.0f + s * 22, 63, 32.0f + s * 35, 63 }, 3, R_SKIN, 0.2f, 0.4f, 60, 3);   /* the web */
            cv_capsule(32 + s * 9, 37, 32 + s * 15, 27, 2.2f, R_WOOD, 0.1f, 0.52f);                  /* the arms forward */
            cv_ellipse(32 + s * 13.0f, 30.0f, 4.0f, 5.5f, R_WOOD, 0.22f, 0.7f);
        }
        cv_line(22, 28, 42, 28, sh(R_WOOD, 0.03f));
        lf_toad_eye(25.0f, 18.0f, -1); lf_toad_eye(39.0f, 18.0f, 1);
    }
    for (int y = 0; y < TS; y++) for (int x = 0; x < TS; x++) {      /* blotched olive and brown over all of it, and the warts in clusters */
        uint8_t c = cv_sprite[y * TS + x];
        if (c < R_WOOD || c >= R_WOOD + 16) continue;
        float b = fbm(x * 0.13f, y * 0.13f, 64, 791);
        if (b > 1.05f) cv_sprite[y * TS + x] = sh(R_MOSS, 0.18f + 0.32f * (c - R_WOOD) / 15.0f);
        else if (b < 0.75f) lf_step(x, y, -2);
    }
    for (int k = 0; k < 60; k++) {
        int x = (int)(h01(k, 1, 792) * 44) + 10, y = (int)(h01(k, 2, 792) * (frame ? 40 : 28)) + (frame ? 10 : 30);
        float cl = fbm(x * 0.21f, y * 0.21f, 64, 793);
        if (cl > 1.0f && !lf_clear(x, y) && cv_sprite[y * TS + x] >= R_WOOD && cv_sprite[y * TS + x] < R_WOOD + 16) lf_wart(x, y);
    }
    lf_relight(3, 3);
    cv_outline(0);
    cv_store(id);
}

/* Glowing fungus: a clump of fruiting bodies on a crust of moss: caps of three shapes (a bell, a dome, a flat one with its edge turned up),
 * the gills showing as a glow where the cap lifts, stalks with a ring on the tallest, a few spots on the caps, a bud or two, threads of
 * root in the crust. Cold, green-blue; the glow itself full bright (R_GLOW) and a soft dither of it round the gills. */
static void lf_cap(float cx, float cy, float rx, float ry, int shape, uint32_t seed, int glow) {
    float sy = cy + ry * 0.35f;
    cv_capsule(cx + 0.5f, sy, cx + (h01(1, 1, seed) - 0.5f) * 3.0f, 58, 1.6f + rx * 0.14f, R_BONE, 0.3f, 0.7f);    /* the stalk */
    if (shape == 0) {                                                // a bell
        float p[12] = { cx - rx, cy + ry * 0.7f, cx - rx * 0.85f, cy - ry * 0.4f, cx - rx * 0.3f, cy - ry * 1.15f, cx + rx * 0.3f, cy - ry * 1.15f, cx + rx * 0.85f, cy - ry * 0.4f, cx + rx, cy + ry * 0.7f };
        cv_poly(p, 6, R_MOSS, 0.15f, 0.68f, cy - ry * 1.15f, ry * 2.0f);
    } else if (shape == 1) cv_ellipse(cx, cy, rx, ry, R_MOSS, 0.12f, 0.7f);                                            // a dome
    else { cv_ellipse(cx, cy, rx, ry * 0.6f, R_MOSS, 0.12f, 0.66f); cv_ellipse(cx, cy + ry * 0.35f, rx * 0.8f, ry * 0.3f, R_MOSS, 0.05f, 0.3f); }   // a flat one, edge up
    for (int k = 0; k < (int)(rx * 0.9f); k++) { float a = PI_F * (0.08f + 0.84f * k / (rx * 0.9f)); cv_px((int)(cx - cosf(a) * rx * 0.9f), (int)(cy + ry * (shape == 2 ? 0.42f : 0.72f) + sinf(a) * 1.0f), (uint8_t)(R_GLOW + 7 + (k % 4))); }   /* the gills' glow */
    for (int k = 0; k < 3; k++) cv_px((int)(cx + (h01(k, 5, seed) - 0.5f) * rx * 1.2f), (int)(cy - ry * 0.4f + (h01(k, 6, seed) - 0.5f) * ry * 0.6f), sh(R_BONE, 0.55f));   /* spots */
    cv_px((int)(cx - rx * 0.4f), (int)(cy - ry * 0.5f), sh(R_MOSS, 0.95f));
    (void)glow;
}
static void spr_lf_fungus(int id, int big) {
    cv_begin(cv_sprite, TS, TS);
    cv_ellipse(32, 59, 28.0f, 5.0f, R_MOSS, 0.08f, 0.4f);            /* the crust it grows from */
    for (int k = 0; k < 14; k++) cv_line(32 + (h01(k, 1, 801) - 0.5f) * 40, 58, 32 + (h01(k, 2, 801) - 0.5f) * 56, 61, sh(R_BONE, 0.3f));   /* threads of root */
    static const float C[5][5] = { { 32, 38, 11, 7, 1 }, { 15, 49, 8, 5, 0 }, { 49, 46, 9, 5, 2 }, { 24, 25, 6, 4, 0 }, { 42, 29, 5, 3.6f, 1 } };
    for (int i = (big ? 4 : 2); i >= (big ? 0 : 1); i--) lf_cap(C[i][0], C[i][1], C[i][2], C[i][3], (int)C[i][4], 802 + (uint32_t)i, 1);
    if (big) { cv_line(30, 45, 35, 46, sh(R_BONE, 0.6f)); cv_line(30, 46, 35, 47, sh(R_BONE, 0.2f)); }                               /* the ring on the tallest stalk */
    for (int k = 0; k < 3; k++) cv_ellipse(8 + k * 22 + (k == 1) * 3, 57, 1.8f, 2.2f, R_MOSS, 0.3f, 0.7f);                           /* buds */
    lf_grain(803, 0.06f);
    lf_relight(2, 3);
    cv_outline(0);
    for (int y = 0; y < TS; y++) for (int x = 0; x < TS; x++) {      /* a dithered glow in the air round the gills */
        if (cv_sprite[y * TS + x] != C_CLEAR || ((x + y) & 1)) continue;
        for (int d = 0; d < 4; d++) { int nx = x + (d == 0) - (d == 1), ny = y + (d == 2) - (d == 3); if (nx >= 0 && ny >= 0 && nx < TS && ny < TS && cv_sprite[ny * TS + nx] >= R_GLOW + 7 && cv_sprite[ny * TS + nx] <= R_GLOW + 10) { cv_sprite[y * TS + x] = (uint8_t)(R_GLOW + 4); break; } }
    }
    cv_store(id);
}
static void spr_spore(int id) {                                      /* a mote: a hard point, its glow falling away in a dither */
    cv_begin(cv_sprite, TS, TS);
    for (int y = 20; y < 44; y++) for (int x = 20; x < 44; x++) {
        float d = hypotf(x + 0.5f - 32.0f, y + 0.5f - 32.0f);
        if (d < 2.0f) cv_px(x, y, (uint8_t)(R_GLOW + 14)); else if (d < 4.2f) cv_px(x, y, (uint8_t)(R_GLOW + 9)); else if (d < 8.0f && (((x ^ y) & 1) == 0) && h01(x, y, 811) > (d - 4.0f) / 5.0f) cv_px(x, y, (uint8_t)(R_GLOW + 4));
    }
    cv_store(id);
}

/* Water on the floor, seen low so it lies flat and wide: a puddle with an uneven edge, the wet stone round it darker, the pale of the
 * ceiling and a torch's flare in it, a leaf-fall of grit; rings spreading from a drop (a ring and a fainter one inside it, the far
 * side of each brighter); a boot's wet print with its tread; a drop with its streak and a spark of the light in it */
static void spr_puddle(int id) {
    cv_begin(cv_sprite, TS, TS);
    for (int y = 34; y < 58; y++) for (int x = 2; x < 62; x++) {
        float nx = (x + 0.5f - 32.0f) / 29.0f, ny = (y + 0.5f - 46.0f) / 9.0f, e = nx * nx + ny * ny;
        float n = 0.12f * (fbm(x * 0.2f, y * 0.4f, 64, 821) - 0.9f), lim = 1.0f + n * 2.0f;
        if (e < lim * 1.0f) {
            if (e > lim * 0.78f) cv_px(x, y, sh(R_STONE, 0.1f + 0.18f * (1.0f - (e - lim * 0.78f) / (lim * 0.22f))));                   /* the wet rim, darker stone */
            else cv_px(x, y, sh(R_FLOOR, 0.1f + 0.1f * (ny + 1.0f)));                                                                  /* the water */
        }
    }
    for (int y = 40; y < 52; y++) for (int x = 6; x < 58; x++) {     /* the ceiling in it, in bars; a flare */
        if (cv_sprite[y * TS + x] < R_FLOOR || cv_sprite[y * TS + x] >= R_FLOOR + 16) continue;
        float w = sinf(x * 0.35f + y * 1.7f) + 0.6f * sinf(x * 0.11f);
        if (w > 1.0f && (y & 1)) cv_px(x, y, sh(R_GHOST, 0.3f + 0.1f * (w - 1.0f)));
    }
    for (int k = 0; k < 6; k++) cv_px(40 + k, 44 + (k & 1), sh(R_FLAME, 0.4f + 0.1f * k)), cv_px(41 + k, 45, sh(R_FLAME, 0.2f));        /* the torch's flare */
    for (int k = 0; k < 9; k++) { int x = 8 + (int)(h01(k, 1, 822) * 48), y = 40 + (int)(h01(k, 2, 822) * 12); if (cv_sprite[y * TS + x] != C_CLEAR) cv_px(x, y, sh(R_STONE, 0.4f)); }   /* grit */
    cv_store(id);
}
static void spr_ring(int id, int stage) {
    cv_begin(cv_sprite, TS, TS);
    static const float RX[3] = { 7.0f, 14.0f, 25.0f }, RY[3] = { 1.8f, 3.5f, 6.2f };
    for (int pass = 0; pass < 2; pass++) {
        float rx = RX[stage] * (pass ? 0.62f : 1.0f), ry = RY[stage] * (pass ? 0.62f : 1.0f);
        if (pass && stage == 0) continue;
        for (int k = 0; k < 160; k++) {
            float a = TAU_F * k / 160.0f, far = 0.5f - 0.5f * sinf(a);              // the far side (up the page) is brighter
            float v = (0.82f - 0.2f * stage - 0.18f * pass) * (0.45f + 0.55f * far);
            if (v < 0.22f || (stage == 2 && (k % 5) < 2)) continue;
            cv_px((int)(32 + cosf(a) * rx), (int)(46 + sinf(a) * ry), sh(R_GHOST, v));
            if (far > 0.8f && stage < 2) cv_px((int)(32 + cosf(a) * rx), (int)(45 + sinf(a) * ry), sh(R_GHOST, v * 0.5f));
        }
    }
    cv_store(id);
}
static void spr_print(int id, int right) {                           /* a boot's: the sole with its tread, the heel apart, the arch narrow, a few drops flung */
    cv_begin(cv_sprite, TS, TS);
    int d = right ? 1 : -1;
    for (int y = 30; y < 62; y++) {
        float w = y < 46 ? 4.6f - 0.22f * fabsf(y - 39.0f) : (y < 51 ? 1.2f : 3.5f);                  // toe, arch, heel
        if (y < 33) w *= 0.55f + 0.14f * (y - 30);
        if (y > 58) w = 3.5f - 0.9f * (y - 58.0f);
        float cx = 32.0f + d * (y < 46 ? 0.8f : 0.0f);
        for (int x = (int)(cx - w); x <= (int)(cx + w); x++) cv_px(x, y, sh(R_FLOOR, 0.05f + 0.03f * h01(x, y, 831)));
        if (y < 46 && (y % 4) == 0) for (int x = (int)(cx - w) + 1; x < (int)(cx + w); x++) cv_px(x, y, sh(R_FLOOR, 0.0f));   // the tread
        if (y > 52 && (y % 3) == 0) cv_px((int)cx, y, sh(R_FLOOR, 0.0f));
    }
    for (int y = 33; y < 44; y++) cv_px(32 + d + (y > 40), y, sh(R_GHOST, 0.2f));                            // a sheen
    for (int k = 0; k < 5; k++) cv_px(32 + d * (7 + k * 2), 36 + ((k * 7) % 9), sh(R_FLOOR, 0.1f));         // drops flung
    cv_store(id);
}
static void spr_drip(int id) {                                       /* a drop in the air: a bead drawn out above, the streak behind it, the sky in it */
    cv_begin(cv_sprite, TS, TS);
    for (int y = 8; y < 30; y++) cv_px(32, y, sh(R_GHOST, 0.1f + 0.2f * (y - 8) / 22.0f));
    for (int y = 22; y < 32; y++) { cv_px(31, y, sh(R_GHOST, 0.12f)); cv_px(33, y, sh(R_GHOST, 0.12f)); }
    float p[10] = { 32, 28, 29.5f, 36, 29.8f, 40, 32, 43, 34.2f, 40 };
    cv_poly(p, 5, R_GHOST, 0.2f, 0.7f, 28, 16);
    cv_ellipse(32, 37, 2.7f, 3.4f, R_GHOST, 0.25f, 0.85f);
    cv_px(31, 35, sh(R_GHOST, 1.0f)); cv_px(31, 36, sh(R_BONE, 0.95f)); cv_px(33, 40, sh(R_GHOST, 0.25f));
    cv_store(id);
}

/* Eyes in a niche's dark: the dark itself first, a low brow and the suggestion of a head that is not there, and in it a pair of
 * eyes: an almond of lid, an amber iris with a darker rim, a slit of pupil, a spark; open (0) and half shut under a lowered lid (1) */
static void spr_niche_eyes(int id, int frame) {
    cv_begin(cv_sprite, TS, TS);
    for (int y = 14; y < 52; y++) for (int x = 10; x < 54; x++) {     /* a head's shape, black on the dark, with a ruff of fur at its edge */
        float nx = (x + 0.5f - 32.0f) / 22.0f, ny = (y + 0.5f - 34.0f) / 17.0f, e = nx * nx + ny * ny + 0.12f * (fbm(x * 0.3f, y * 0.3f, 64, 841) - 0.9f);
        if (e < 1.0f) cv_px(x, y, sh(R_STONE, 0.0f + 0.05f * (1.0f - e) * 0.0f));
        else if (e < 1.12f && h01(x, y, 842) > 0.55f) cv_px(x, y, sh(R_STONE, 0.12f));
    }
    for (int s = -1; s <= 1; s += 2) {
        int cx = 32 + s * 8, cy = 33;
        for (int x = -4; x <= 4; x++) {                                // the almond: a lid above and a lid below
            int top = frame ? 0 : -2 + (abs(x) > 2), bot = 1 + (abs(x) < 3);
            for (int y = top; y <= bot; y++) {
                float n = (float)abs(x) / 4.5f;
                uint8_t c = sh(R_GOLD, 0.62f - 0.3f * n * n + 0.12f * (y == bot ? -1 : 0));
                if (abs(x) <= 1 && y >= top && y <= bot) c = sh(R_BONE, 0.0f);                          // the slit of the pupil
                cv_px(cx + x, cy + y, c);
            }
        }
        cv_px(cx - 3, cy - (frame ? 0 : 1), sh(R_GOLD, 0.3f));   // the rim
        cv_eye(cx - 2, cy - (frame ? 0 : 1), sh(R_GOLD, 1.0f)); cv_px(cx + 2, cy, sh(R_GOLD, 0.9f));
        if (frame) for (int x = -4; x <= 4; x++) cv_px(cx + x, cy - 1, sh(R_STONE, 0.0f));          // the lid, drawn down
    }
    cv_store(id);
}

/* A sleeper's skull on its ledge: a cranium with its sutures and a crack, the brow and cheekbones, deep sockets shaded to their rims, the
 * nasal heart, teeth one by one, the jaw hinged below at its two rami, shut (0) or a finger's width ajar (1); stained brown where the earth
 * lay on it, the moss at its foot, dust in the hollows, a thread of web from the brow to the ledge */
static void spr_sleep_skull(int id, int ajar) {
    cv_begin(cv_sprite, TS, TS);
    cv_ellipse(32, 55, 22.0f, 4.5f, R_STONE, 0.1f, 0.35f);           /* the dust and the ledge */
    for (int k = 0; k < 20; k++) cv_px(10 + (int)(h01(k, 1, 851) * 44), 52 + (int)(h01(k, 2, 851) * 6), sh(R_BONE, 0.2f + 0.2f * h01(k, 3, 851)));
    cv_ellipse(22, 55, 6.0f, 2.4f, R_MOSS, 0.15f, 0.5f);             /* moss at its foot */
    cv_ellipse(32, 29, 16.0f, 14.5f, R_BONE, 0.2f, 0.8f);            /* the cranium */
    cv_ellipse(32, 38, 12.5f, 8.0f, R_BONE, 0.14f, 0.66f);           /* the face under it */
    for (int s = -1; s <= 1; s += 2) cv_ellipse(32 + s * 12.5f, 38, 3.0f, 5.0f, R_BONE, 0.16f, 0.7f);   /* the cheekbones */
    for (int s = -1; s <= 1; s += 2) {                               /* the sockets: dark, and a lit lower rim, a ridge of brow above */
        float ex = 32 + s * 6.6f;
        cv_flat(ex, 33, 4.6f, 5.0f, sh(R_BONE, 0.0f)); cv_flat(ex, 33.3f, 3.4f, 3.8f, sh(R_STONE, 0.0f));
        for (int x = -4; x <= 4; x++) cv_px((int)(ex + x), 38, sh(R_BONE, 0.9f)); lf_step((int)ex, 27, 3); cv_line(ex - 4.5f, 27, ex + 4.5f, 27, sh(R_BONE, 0.88f));
    }
    cv_flat(32, 40, 2.0f, 2.8f, sh(R_BONE, 0.0f)); cv_px(32, 37, sh(R_BONE, 0.0f)); cv_px(31, 38, sh(R_BONE, 0.0f)); cv_px(33, 38, sh(R_BONE, 0.0f));   /* the nasal heart */
    cv_line(32, 16, 31, 20, sh(R_BONE, 0.2f)); cv_line(31, 20, 33, 23, sh(R_BONE, 0.18f)); cv_line(33, 23, 32, 26, sh(R_BONE, 0.2f));   /* the suture */
    cv_line(41, 18, 44, 23, sh(R_BONE, 0.15f)); cv_line(44, 23, 43, 27, sh(R_BONE, 0.12f)); cv_line(21, 20, 19, 26, sh(R_BONE, 0.15f)); /* a crack, and the join */
    int jy = ajar ? 5 : 0;
    for (int i = -5; i <= 5; i++) { cv_flat(32 + i * 1.9f, 45, 0.9f, 1.5f, sh(R_BONE, 0.85f - 0.05f * (i & 1))); }   /* the upper teeth, a gap in them */
    cv_px(32 + 4 * 2 - 1, 44, C_CLEAR);
    if (ajar) cv_flat(32, 47, 10.0f, 2.8f, sh(R_BONE, 0.0f));
    for (int s = -1; s <= 1; s += 2) cv_capsule(32 + s * 11.0f, 40 + jy, 32 + s * 10.0f, 49 + jy, 1.6f, R_BONE, 0.18f, 0.66f);   /* the rami */
    cv_ellipse(32, 50 + jy, 10.0f, 4.4f, R_BONE, 0.14f, 0.66f);      /* the jaw */
    for (int i = -4; i <= 4; i++) cv_flat(32 + i * 2.0f, 47 + jy, 0.8f, 1.2f, sh(R_BONE, 0.78f));   /* the lower teeth */
    for (int y = 0; y < TS; y++) for (int x = 0; x < TS; x++) {      /* earth stains, the dust in the hollows */
        uint8_t c = cv_sprite[y * TS + x];
        if (c < R_BONE || c >= R_BONE + 16) continue;
        float b = fbm(x * 0.15f, y * 0.15f, 64, 852);
        if (b > 1.12f) cv_sprite[y * TS + x] = sh(R_WARM, 0.2f + 0.45f * (c - R_BONE) / 15.0f);
    }
    for (int k = 0; k < 14; k++) { float f = k / 13.0f; cv_px((int)(43 + 13 * f), (int)(21 + 33 * f + 5 * sinf(f * PI_F)), sh(R_BONE, 0.45f - 0.1f * f)); }   /* a web from the cheek, drooping to the ledge */
    lf_grain(853, 0.07f);
    lf_relight(3, 3);
    cv_outline(0);
    cv_store(id);
}

/* A sleeper's hand lying open on the ledge, seen from above: the two bones of the forearm, the broken end of one, eight small carpals, four
 * metacarpals fanned and four fingers of three bones each, a thumb of two; a tarnished ring on the third finger; in dust. In the second
 * picture one finger is curled up and over, as if it had begun to close. */
static void spr_sleep_hand(int id, int curled) {
    cv_begin(cv_sprite, TS, TS);
    cv_ellipse(32, 56, 25.0f, 3.5f, R_STONE, 0.1f, 0.35f);
    cv_bone(3, 49, 17, 46, 2.5f, 0.25f, 0.74f); cv_bone(4, 54, 18, 51, 2.2f, 0.22f, 0.7f);          /* the radius and the ulna */
    for (int k = 0; k < 4; k++) cv_px(3, 47 + k * 2, C_CLEAR);                                       /* a break at the end of one */
    for (int k = 0; k < 8; k++) cv_ellipse(19.0f + (k % 3) * 2.8f, 44.0f + (k / 3) * 2.8f + (k % 2), 2.0f, 1.8f, R_BONE, 0.2f, 0.72f);   /* the carpals */
    static const float MCY[4] = { 40.0f, 43.5f, 47.0f, 51.0f };
    for (int f = 0; f < 4; f++) {
        float y0 = MCY[f] + 1.0f, y1 = MCY[f] - 1.0f + (f - 1.5f) * 1.2f;
        cv_bone(26.0f, y0 + 1.0f, 36.0f, y1 + 2.0f, 1.9f, 0.22f, 0.72f);                            /* the metacarpal */
        float len = 8.0f - (f == 3 ? 1.5f : 0.0f) + (f == 1 ? 1.5f : 0.0f), kx = 36.0f, ky = y1 + 2.0f;
        for (int p = 0; p < 3; p++) {                                                               /* three phalanges, each a little shorter */
            float l = len * (0.62f - 0.12f * p) + 1.5f, nx = kx + l, ny = ky + 0.4f + 0.2f * f;
            if (curled && f == 1) { nx = kx + l * (p == 0 ? 0.6f : 0.15f); ny = ky - l * (p == 0 ? 0.8f : 1.0f); }
            cv_bone(kx, ky, nx, ny, 1.45f - 0.15f * p, 0.22f, 0.74f);
            kx = nx; ky = ny;
        }
    }
    cv_line(41, 46, 42, 49, sh(R_GOLD, 0.35f)); cv_line(41, 47, 43, 49, sh(R_GOLD, 0.12f)); cv_px(41, 46, sh(R_GOLD, 0.7f)); cv_px(40, 46, sh(R_GOLD, 0.5f));   /* the ring, tarnished */
    cv_bone(22, 52, 28, 57, 2.0f, 0.2f, 0.7f); cv_bone(28, 57, 34, 58.5f, 1.8f, 0.2f, 0.7f);       /* the thumb */
    for (int k = 0; k < 18; k++) cv_px(8 + (int)(h01(k, 1, 861) * 46), 52 + (int)(h01(k, 2, 861) * 7), sh(R_BONE, 0.2f + 0.25f * h01(k, 3, 861)));   /* dust */
    for (int y = 0; y < TS; y++) for (int x = 0; x < TS; x++) {
        uint8_t c = cv_sprite[y * TS + x];
        if (c < R_BONE || c >= R_BONE + 16) continue;
        if (fbm(x * 0.15f, y * 0.15f, 64, 862) > 1.1f) cv_sprite[y * TS + x] = sh(R_WARM, 0.2f + 0.45f * (c - R_BONE) / 15.0f);
    }
    lf_grain(863, 0.06f);
    lf_relight(3, 3);
    cv_outline(0);
    cv_store(id);
}
