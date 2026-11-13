/* A part of crawl_art.c (docs/PLAN-tidy.md): it is #included there in its place and never compiled on its own.
 * The Whipper-In, the Sexton and the Lightless, with their side views; the bestiary's bones. */

/* ---- the three new kinds (1.0.0b15, docs/PLAN-ai.md section 22): the Whipper-In, the Sexton, the Lightless ---- */

static void cbez(const float *p, float t, float *x, float *y) {    /* a curve through four points */
    float u = 1.0f - t;
    *x = u * u * u * p[0] + 3.0f * u * u * t * p[2] + 3.0f * u * t * t * p[4] + t * t * t * p[6];
    *y = u * u * u * p[1] + 3.0f * u * u * t * p[3] + 3.0f * u * t * t * p[5] + t * t * t * p[7];
}
static float pwl(const float *xs, const float *ys, int n, float x) {   /* a line through points, held at its ends */
    if (x <= xs[0]) return ys[0];
    for (int i = 1; i < n; i++) if (x <= xs[i]) return lerpf(ys[i - 1], ys[i], (x - xs[i - 1]) / (xs[i] - xs[i - 1]));
    return ys[n - 1];
}

/* a whip's thong of knotted sinew along a curve (four points), thick at the stock and thinning to a frayed cracker;
 * crack: the end snapping, a white flash and splinters of air round it */
static void sinew_lash(const float *p, int crack, uint32_t seed) {
    float acc = 0.0f, px = p[0], py = p[1];
    for (int i = 0; i <= 120; i++) {
        float t = (float)i / 120.0f, x, y;
        cbez(p, t, &x, &y);
        acc += hypotf(x - px, y - py); px = x; py = y;
        float v = 0.2f + 0.16f * h01(i / 4, 1, seed) + ((i / 3) & 1 ? 0.06f : 0.0f);   /* twisted: light and dark by turns */
        cv_px((int)x, (int)y, sh(R_WARM, v));
        if (t < 0.4f) cv_px((int)x, (int)y + 1, sh(R_WARM, v * 0.55f));
        if (acc > 5.5f && t < 0.9f) {                               /* a knot */
            acc = 0.0f;
            cv_px((int)x, (int)y, sh(R_WARM, 0.62f)); cv_px((int)x + 1, (int)y, sh(R_WARM, 0.42f));
            cv_px((int)x, (int)y + 1, sh(R_WARM, 0.18f)); cv_px((int)x + 1, (int)y + 1, sh(R_WARM, 0.12f));
        }
    }
    float ex = p[6], ey = p[7], dx = p[6] - p[4], dy = p[7] - p[5], L = hypotf(dx, dy) + 1e-3f;
    dx /= L; dy /= L;
    for (int k = -1; k <= 1; k++)                                   /* the cracker, frayed */
        for (int j = 1; j <= 3; j++) cv_px((int)(ex + dx * j - dy * k * j * 0.5f), (int)(ey + dy * j + dx * k * j * 0.5f), sh(R_WARM, 0.7f - 0.12f * j));
    if (crack) {
        float cx = ex + dx * 4.0f, cy = ey + dy * 4.0f;
        cv_px((int)cx, (int)cy, C_WHITE);
        for (int a = 0; a < 8; a++) {
            float an = a * (TAU_F / 8.0f) + 0.2f, r0 = 2.0f, r1 = (a & 1) ? 3.5f : 5.0f;
            for (float r = r0; r <= r1; r += 1.0f) cv_px((int)(cx + cosf(an) * r), (int)(cy + sinf(an) * r), r < 3.0f ? (uint8_t)C_WHITE : sh(R_GHOST, 0.8f));
        }
    }
}

/* the huntsman's horn: an ox horn from its mouthpiece (x0, y0) bending by bend to its bell (x1, y1), dark at the narrow
 * end and pale at the wide, banded in brass, a brass mouthpiece */
static void ox_horn(float x0, float y0, float x1, float y1, float bend) {
    float L = hypotf(x1 - x0, y1 - y0) + 1e-3f, nx = -(y1 - y0) / L, ny = (x1 - x0) / L;
    float p[8] = { x0, y0, x0 + (x1 - x0) * 0.33f + nx * bend, y0 + (y1 - y0) * 0.33f + ny * bend,
                   x0 + (x1 - x0) * 0.66f + nx * bend, y0 + (y1 - y0) * 0.66f + ny * bend, x1, y1 };
    for (int i = 0; i <= 48; i++) {
        float t = (float)i / 48.0f, x, y;
        cbez(p, t, &x, &y);
        float r = 0.5f + 1.5f * t * t + (t > 0.88f ? (t - 0.88f) * 7.0f : 0.0f);   /* narrow, then the bell's flare */
        int band = fabsf(t - 0.4f) < 0.03f || fabsf(t - 0.74f) < 0.03f;
        if (t < 0.07f) cv_ellipse(x, y, 0.8f, 0.8f, R_WOOD, 0.55f, 1.0f);   /* the brass mouthpiece */
        else if (band) cv_ellipse(x, y, r + 0.3f, r + 0.3f, R_WOOD, 0.5f, 1.0f);
        else if (t < 0.22f) cv_ellipse(x, y, r, r, R_WOOD, 0.1f, 0.45f);   /* the horn's dark tip */
        else cv_ellipse(x, y, r, r, R_SKIN, 0.3f + 0.2f * t, 0.75f + 0.2f * t);   /* pale horn, creamier to the bell */
    }
    float dx = x1 - p[4], dy = y1 - p[5], d = hypotf(dx, dy) + 1e-3f;
    cv_flat(x1 + dx / d * 0.6f, y1 + dy / d * 0.6f, 1.6f, 1.6f, sh(R_SKIN, 0.05f));   /* its mouth, dark */
    cv_px((int)(x1 - dy / d * 2.2f), (int)(y1 + dx / d * 2.2f), sh(R_WOOD, 0.95f));   /* a brass rim catching the light */
}

/* the Whipper-In's skull: the small one, with its brow under the cap's peak */
static void whip_skull(int x0, int y0, uint8_t eye) {
    grid_stamp(g_back ? SKULL_SMALL_BACK : SKULL_SMALL, 9, 10, x0, y0, R_BONE, R_BONE, g_back ? 0 : eye, 0);
    g_skbox[0] = x0; g_skbox[1] = y0; g_skbox[2] = x0 + 9; g_skbox[3] = y0 + 10;
}
/* the black velvet hunting cap, its peak over the brow at y0 + 7 */
static void hunt_cap(float cx, float y0) {
    for (int y = (int)y0; y <= (int)y0 + 6; y++)
        for (int x = (int)cx - 6; x <= (int)cx + 6; x++) {
            float nx = (x + 0.5f - cx) / 5.6f, ny = (y + 0.5f - (y0 + 6.4f)) / 6.0f, d = nx * nx + ny * ny;
            if (d > 1.0f) continue;
            float v = 0.06f + 0.42f * lit(nx, ny, sqrtf(1.0f - d));
            if (fabsf(x + 0.5f - cx + 1.5f) < 0.6f && y < y0 + 5) v += 0.05f;   /* a seam, nap catching the light */
            cv_px(x, y, sh(R_IRON, v));
        }
    for (int x = (int)cx - 6; x <= (int)cx + 6; x++) cv_px(x, (int)y0 + 7, sh(R_IRON, 0.34f - 0.02f * (x - cx + 6)));   /* the peak */
    cv_px((int)cx, (int)y0 + 1, sh(R_IRON, 0.7f));                  /* the button on top */
}

/* The Whipper-In, the Order's huntsman: a tall skeleton in what is left of a scarlet hunting coat (R_BLOOD, faded and
 * holed), a black velvet cap, a pale stock at the throat, top boots with brown cuffs, an ox horn on a cord across his
 * chest and a whip of knotted sinew. pose 0-1 walking, 2 the horn to his jaw, 3 the whip's crack, 4 fallen. */
static void spr_whipper(int id, float w, int pose) {
    cv_begin(cv_sprite, TS, TS);
    uint8_t eye = (uint8_t)(R_FLAME + 11);
    if (pose == 4) {
        cv_flat(32, 61.5f, 26, 2.5f, R_WARM + 1);
        float m[12] = { 8, 62, 13, 55, 26, 53, 40, 54, 53, 57, 57, 62 };   /* the coat, spread where he fell */
        cv_poly(m, 6, R_BLOOD, 0.32f, 0.1f, 53, 9);
        for (int y = 53; y < 63; y++) for (int x = 8; x < 58; x++) if (fbm(x / 2.5f, y / 2.5f, 8, 611) > 0.78f) cv_px(x, y, 0);   /* its holes */
        for (int i = 0; i < 6; i++) {                               /* bones in it */
            float cx = 32 + (h01(i, 1, 613) - 0.5f) * 36, cy = 59 - 3 * h01(i, 2, 613), a = (h01(i, 3, 613) - 0.5f) * 1.6f, l = 4 + 4 * h01(i, 4, 613);
            cv_bone(cx - cosf(a) * l, cy - sinf(a) * l, cx + cosf(a) * l, cy + sinf(a) * l, 1.2f, 0.25f, 0.9f);
        }
        cv_capsule(36, 56, 44, 61, 2.2f, R_IRON, 0.08f, 0.5f);       /* a boot */
        cv_capsule(36, 55, 37, 56, 1.4f, R_WOOD, 0.3f, 0.7f);
        whip_skull(19, 50, 0);
        cv_ellipse(14, 59, 5.0f, 2.6f, R_IRON, 0.05f, 0.4f);        /* the cap, rolled off */
        ox_horn(46, 55, 56, 50, -1.5f);
        float ws[8] = { 27, 58, 34, 52, 44, 63, 52, 60 };           /* the whip, slack across it */
        sinew_lash(ws, 0, 615);
        cv_bone_decay(617);
        cv_outline(0); cv_store(id); return;
    }
    int back = g_back;                                              /* 1.0.0b18: from behind (walking), drawn as from the front and turned */
    if (back) pose &= 1;
    int horn = pose == 2, crack = pose == 3;
    float hb = horn ? -1.0f : 0.0f;                                 /* the head tipped back to blow */
    cv_flat(32, 62, 15, 1.8f, R_WARM + 1);
    /* the coat's tails behind, seen between the legs */
    for (int y = 36; y < 55; y++) {
        float hw = 3.5f + (y - 36) * 0.12f, sw = w * (y - 36) * 0.06f;
        for (int x = (int)(32 - hw + sw); x <= (int)(32 + hw + sw); x++) {
            if (y > 44 && fabsf(x + 0.5f - 32 - sw) < 0.8f) continue;   /* the vent */
            if (y > 50 && h01(x, y, 619) < (y - 50) / 4.0f) continue;
            cv_px(x, y, sh(R_BLOOD, 0.1f + 0.05f * h01(x, y / 3, 621)));
        }
    }
    /* legs: long bone, knee-high top boots with brown cuffs, a spur at the heel */
    for (int s = -1; s <= 1; s += 2) {
        float k = s * (crack || horn ? 0.3f : w), hx = 32 + s * 3.0f, kx = 32 + s * 4.0f - k * 2.5f, fx = 32 + s * 4.6f - k * 4.0f;
        if (crack) { kx += s * 1.0f; fx += s * 2.5f; }
        cv_capsule(hx, 41, kx, 50, 1.1f, R_BONE, 0.25f, 0.95f);
        cv_ellipse(kx, 50.2f, 1.5f, 1.3f, R_BONE, 0.35f, 1.0f);     /* the kneecap */
        cv_capsule(kx, 52.5f, fx, 59.5f, 2.2f, R_IRON, 0.08f, 0.62f);
        cv_ellipse(fx + s * 0.6f, 61.2f, 3.3f, 1.6f, R_IRON, 0.08f, 0.55f);
        for (int x = (int)(kx - 2.6f); x <= (int)(kx + 2.6f); x++) { cv_px(x, 51, sh(R_WOOD, 0.5f - 0.06f * (x - kx))); cv_px(x, 52, sh(R_WOOD, 0.32f - 0.05f * (x - kx))); }   /* the cuff */
        cv_px((int)(fx + s * 3.6f), 60, sh(R_GHOST, 0.55f));
    }
    /* spine, ribs, collarbones, pelvis: the bones the coat leaves showing */
    cv_capsule(32, 16, 32, 39, 1.2f, R_BONE, 0.3f, 0.9f);
    for (int i = 0; i < 4; i++) {
        float y = 20.5f + i * 3.1f;
        cv_capsule(31, y, 26.5f + i * 0.5f, y + 2, 0.85f, R_BONE, 0.25f, 0.9f); cv_capsule(26.5f + i * 0.5f, y + 2, 28.0f + i * 0.4f, y + 4.0f, 0.75f, R_BONE, 0.25f, 0.85f);
        cv_capsule(33, y, 37.5f - i * 0.5f, y + 2, 0.85f, R_BONE, 0.25f, 0.9f); cv_capsule(37.5f - i * 0.5f, y + 2, 36.0f - i * 0.4f, y + 4.0f, 0.75f, R_BONE, 0.25f, 0.85f);
    }
    cv_ellipse(32, 39.5f, 4.6f, 2.4f, R_BONE, 0.2f, 0.85f);
    cv_flat(30.0f, 39.5f, 0.9f, 0.9f, 0); cv_flat(34.0f, 39.5f, 0.9f, 0.9f, 0);
    /* the coat: fitted at the waist, open over the ribs, its skirts parted and flaring to the knee; faded, stained, holed */
    static const float YS[6] = { 17, 19, 24, 30, 34, 48 }, HW[6] = { 8.5f, 10.5f, 9.5f, 7.5f, 8.5f, 12.5f };
    static const float YO[8] = { 17, 19, 23, 28, 30, 32, 34, 48 }, OP[8] = { 1.5f, 3.0f, 5.0f, 2.5f, 0.5f, 0.5f, 1.0f, 6.5f };
    for (int y = 17; y <= 48; y++) {
        float hw = pwl(YS, HW, 6, (float)y), op = back ? (y > 40 ? 0.6f : 0.0f) : pwl(YO, OP, 8, (float)y), sw = y > 30 ? w * (y - 30) * 0.08f : 0.0f;   /* from behind: closed, the vent below */
        if (crack && y > 30) sw += (y - 30) * 0.05f;
        for (int x = (int)(32 - hw + sw) - 1; x <= (int)(32 + hw + sw) + 1; x++) {
            float dx = x + 0.5f - 32 - sw, a = fabsf(dx);
            if (a > hw || a < op) continue;
            if (y > 43 && h01(x, y, 623) < (y - 43) / 6.0f) continue;   /* the hem, ragged */
            if (fbm(x / 2.5f, y / 2.5f, 8, 611) > 0.8f) continue;       /* holed */
            float u = dx / hw, v = 0.2f + 0.24f * (1.0f - u * u) - 0.07f * u;
            if (back) { if (y < 20) v = -1.0f; else if (a < 0.7f) v -= 0.08f; }   /* the velvet collar at the nape; the back seam */
            else if (y < 30 && a - op < 1.6f) v = y < 21 ? -1.0f : 0.1f;   /* the lapel's facing; the velvet collar */
            if (y > 32 && sinf(dx * 1.3f + 0.7f) > 0.62f) v -= 0.07f;    /* folds in the skirts */
            if (fbm(x / 4.0f, y / 4.0f, 16, 625) < 0.3f) v -= 0.06f;     /* old stains */
            cv_px(x, y, v < 0.0f ? sh(R_IRON, 0.22f) : sh(R_BLOOD, v));
        }
    }
    if (back) for (int s = -1; s <= 1; s += 2) { cv_px(32 + s * 2, 31, sh(R_WOOD, 0.85f)); cv_px(32 + s * 2 - 1, 31, sh(R_SKIN, 0.8f)); }   /* the two buttons at the
                                                                      back of the waist, where a hunting coat has them */
    if (!back) for (int k = 0; k < 2; k++) for (int s = -1; s <= 1; s += 2) {   /* brass buttons down the lapels, and the one at the waist */
        float bx = 32 + s * (pwl(YO, OP, 8, 24.0f + k * 3.0f) + 1.8f);
        cv_px((int)bx, 24 + k * 3, sh(R_WOOD, 0.85f)); cv_px((int)bx - 1, 24 + k * 3, sh(R_SKIN, 0.85f));
    }
    if (!back) {
        cv_px(32, 31, sh(R_WOOD, 0.9f)); cv_px(31, 31, sh(R_SKIN, 0.85f));
        /* the stock at his throat, a pale linen, knotted, its ends torn */
        cv_ellipse(32, 17.5f, 2.4f, 1.6f, R_STONE, 0.35f, 0.95f);
        cv_capsule(31.5f, 18.5f, 30.5f, 22.5f, 0.8f, R_STONE, 0.3f, 0.85f); cv_capsule(32.5f, 18.5f, 33.5f, 21.5f, 0.8f, R_STONE, 0.25f, 0.75f);
    } else cv_capsule(30, 17, 34, 17, 0.8f, R_STONE, 0.3f, 0.7f);    /* the stock's band, round the back of the neck */
    /* arms: ragged sleeves to the elbow, bone below */
    for (int s = -1; s <= 1; s += 2) {
        float sx = 32 + s * 9.5f, sy = 19.5f, ex, ey, hx, hy;
        if (s < 0 && horn) { ex = 19; ey = 25; hx = 26; hy = 14; }
        else if (s > 0 && crack) { ex = 46; ey = 12; hx = 44; hy = 5; }
        else if (s < 0 && crack) { ex = 18; ey = 25; hx = 13; hy = 30; }
        else if (s < 0) { ex = 20 - w * 0.6f; ey = 29; hx = 21 - w * 1.2f; hy = 37; }
        else { ex = 44 + w * 0.3f; ey = 28.5f; hx = 44.5f + w * 0.6f; hy = 36; }
        float cx = sx + (ex - sx) * 0.92f, cy = sy + (ey - sy) * 0.92f;
        cv_bone(cx, cy, hx, hy, 1.0f, 0.25f, 0.92f);
        cv_capsule(sx, sy, cx, cy, 2.6f, R_BLOOD, 0.1f, 0.42f);
        for (int r = 0; r < 4; r++) {                               /* the cuff, ragged */
            float a = atan2f(ey - sy, ex - sx) + (r - 1.5f) * 0.55f;
            float l = 1.5f + 2.5f * h01(r, s + 2 + pose, 627);
            for (int j = 0; j <= (int)l; j++) cv_px((int)(cx + cosf(a + 1.57f) * (r - 1.5f) * 1.2f + (ex - sx) * 0.03f * j), (int)(cy + j), sh(R_BLOOD, 0.22f - 0.04f * j));
        }
        cv_ellipse(hx, hy, 1.6f, 1.5f, R_BONE, 0.3f, 0.95f);
        for (int f = 0; f < 3; f++) cv_line(hx, hy, hx + s * (0.5f + f * 0.5f), hy + 2.5f + f * 0.3f, sh(R_BONE, 0.8f - 0.1f * f));
    }
    /* the horn: at his hip on its cord, or at his jaw */
    if (horn) { cv_line(39, 19, 30, 15, sh(R_WARM, 0.25f)); ox_horn(31.5f, 13.0f + hb, 14, 4, -3.0f); cv_ellipse(26, 14, 1.7f, 1.6f, R_BONE, 0.3f, 0.95f); }
    else { cv_line(39, 19, 28, 34, sh(R_WARM, 0.22f)); cv_line(40, 19, 29, 34, sh(R_WARM, 0.12f)); ox_horn(29.5f, 33, 24.0f, 42.5f, -2.0f); }
    /* the whip: its stock in his hand, the thong trailing on the floor, or thrown out overhead and cracking */
    if (crack) {
        cv_capsule(44, 6, 41, 1, 0.9f, R_WOOD, 0.2f, 0.6f);
        float ws[8] = { 41, 1, 26, -6, 20, 20, 5, 13 };
        sinew_lash(ws, 1, 629);
    } else {
        float hx = 44.5f + w * 0.6f;
        cv_capsule(hx, 36, hx + 2.5f, 44, 0.9f, R_WOOD, 0.2f, 0.6f);
        cv_px((int)hx, 35, sh(R_WOOD, 0.9f));
        float ws[8] = { hx + 2.5f, 44, hx + 6, 54, hx + 7, 63, 61, 61.5f };
        sinew_lash(ws, 0, 629);
    }
    /* the neck, the skull, the cap */
    cv_capsule(32, 13 + hb, 32, 16, 1.0f, R_BONE, 0.3f, 0.85f);
    whip_skull(28, 5 + (int)hb, eye);
    hunt_cap(32, hb - 1.0f);
    cv_bone_decay(631);
    if (back) cv_mirror();
    cv_outline(0);
    cv_store(id);
}

/* the Whipper-In's dressing by rank (the layer lies on his bones and his coat): II mould and the coat in tatters, III frost
 * and a notch out of the skull, IV an arrow in the ribs and old blood dried black on the scarlet, V char, the dead's cloak
 * behind him, an iron chain of office and a stag's tines grown through the cap; gilded gold on the bones, gold braid on the
 * coat's edges and a death-mask */
static void whipper_back_dress(int id, float w) {                 /* from behind: tatters, frost, an arrow out of the back, char and the cloak, gilt braid */
    g_dress_ramp2 = R_BLOOD;
    for (int r = RK_II; r < NRANK; r++) {
        dress_begin(id);
        uint32_t sd = 661u + (uint32_t)r * 7u;
        if (r == RK_GILDED) {
            kit_gold(R_BONE, 0.55f, sd);
            for (int y = 17; y < 50; y++) for (int x = 16; x < 50; x++) {
                uint8_t b = base_c(x, y);
                if (b >= R_BLOOD && b < R_BLOOD + 16 && (!dress_on(base_c(x - 1, y), R_BLOOD) || !dress_on(base_c(x + 1, y), R_BLOOD) || !dress_on(base_c(x, y + 1), R_BLOOD)))
                    cv_px(x, y, (uint8_t)(R_GOLD + 8 + (int)(h01(x, y, sd) * 3.0f)));
            }
        } else {
            kit_rags(23, 46, 41, 47, r == RK_II ? 6.0f : 8.0f, w, R_BLOOD, 0.06f, 0.24f, 0, sd);
            if (r == RK_II) kit_moss(0.25f, 2, sd + 1);
            if (r == RK_III) kit_frost(0.75f, sd + 2);
            if (r >= RK_IV) { kit_arrow(35, 27, -0.8f, 8.0f); kit_blood(35, 28, 9, 1, sd + 3); }
            if (r == RK_V) { kit_char(0.3f, sd + 4); kit_rags(22, 19, 42, 20, 28, w * 0.6f, R_IRON, 0.03f, 0.2f, 0, sd + 5); }
        }
        dress_store(id, r, R_BONE);
    }
}
static void whipper_dress(int id, float w, int pose) {
    float hb = pose == 2 ? -1.0f : 0.0f;
    g_dress_ramp2 = R_BLOOD;
    for (int r = RK_II; r < NRANK; r++) {
        dress_begin(id);
        uint32_t sd = 641u + (uint32_t)r * 7u;
        if (r == RK_GILDED) {
            kit_gold(R_BONE, 0.55f, sd);
            for (int y = 17; y < 50; y++) for (int x = 16; x < 50; x++) {
                uint8_t b = base_c(x, y);
                if (b >= R_BLOOD && b < R_BLOOD + 16 && (!dress_on(base_c(x - 1, y), R_BLOOD) || !dress_on(base_c(x + 1, y), R_BLOOD) || !dress_on(base_c(x, y + 1), R_BLOOD)))
                    cv_px(x, y, (uint8_t)(R_GOLD + 8 + (int)(h01(x, y, sd) * 3.0f)));   /* gold braid at the coat's edges */
            }
            kit_mask(28, 5 + (int)hb, 37, 15 + (int)hb);
        } else {
            kit_rags(22, 46, 42, 46.5f, r == RK_II ? 5.0f : r == RK_III ? 7.0f : 9.0f, w, R_BLOOD, 0.08f, 0.3f, 0, sd + 1);   /* the hem, in strips */
            for (int k = 0; k < 6 + 3 * (r - RK_II); k++) {          /* more holes, rank by rank */
                int x = 22 + (int)(h01(k, 1, sd) * 20.0f), y = 20 + (int)(h01(k, 2, sd) * 26.0f);
                if (dress_on(base_c(x, y), R_BLOOD)) { cv_px(x, y, 0); cv_px(x + 1, y, 0); cv_px(x, y + 1, 0); }
            }
            if (r == RK_II) kit_moss(0.3f, 3, sd + 2);
            if (r == RK_III) kit_frost(0.75f, sd + 3);
            if (r >= RK_III) { cv_px(35, 6 + (int)hb, 0); cv_px(36, 7 + (int)hb, 0); cv_line(34, 8 + hb, 34, 11 + hb, sh(R_BONE, 0.15f)); }   /* a notch out of the skull */
            if (r >= RK_IV) {
                kit_arrow(35, 28, -0.5f, 9.0f); kit_blood(35, 29, 9, 2, sd + 4);
                for (int y = 30; y < 50; y++) for (int x = 18; x < 48; x++) if (dress_on(base_c(x, y), R_BLOOD) && fbm(x / 3.0f, y / 3.0f, 16, sd + 5) > 0.66f) cv_px(x, y, sh(R_BLOOD, 0.04f));   /* old blood, dried black */
            }
            if (r == RK_V) {
                kit_char(0.3f, sd + 6);
                kit_rags(22, 19, 42, 20, 34, w * 0.6f, R_IRON, 0.03f, 0.2f, 1, sd + 7);   /* the dead's cloak */
                kit_chain(25, 20, 39, 20, 6.0f, R_IRON);
                cv_flat(32, 26, 1.8f, 1.8f, sh(R_IRON, 0.6f)); cv_px(31, 25, sh(R_IRON, 0.9f));   /* its badge */
                for (int s = -1; s <= 1; s += 2) {                     /* a stag's antlers grown through the cap, spreading */
                    float bx = 32 + s * 5.0f, by = 4.0f + hb;
                    cv_line(bx, by, bx + s * 9.0f, by - 3.5f, sh(R_BONE, 0.8f)); cv_line(bx + s * 1.0f, by + 1.0f, bx + s * 9.0f, by - 2.5f, sh(R_BONE, 0.45f));
                    cv_line(bx + s * 3.0f, by - 1.5f, bx + s * 2.5f, by - 5.0f, sh(R_BONE, 0.75f));
                    cv_line(bx + s * 6.0f, by - 2.5f, bx + s * 6.5f, by - 6.0f, sh(R_BONE, 0.7f));
                    cv_line(bx + s * 9.0f, by - 3.5f, bx + s * 11.0f, by - 6.5f, sh(R_BONE, 0.65f));
                }
            }
        }
        dress_store(id, r, R_BONE);
    }
    g_dress_ramp2 = -1;
}


/* a handbell of bronze (R_BRICK, lit in R_SKIN) hanging from (x, y) down its axis at angle a (0 straight down, positive
 * swinging its mouth to the right), the clapper showing past the lip; rime: frozen, crusted and hung with icicles */
static void hand_bell(float x, float y, float a, int rime) {
    float dx = sinf(a), dy = cosf(a), px = dy, py = -dx;
    static const float US[7] = { 0.0f, 2.0f, 2.6f, 4.0f, 8.5f, 11.5f, 12.8f }, HB[7] = { 1.1f, 1.1f, 2.8f, 3.6f, 3.9f, 6.2f, 6.6f };
    for (int Y = (int)y - 14; Y <= (int)y + 14; Y++)
        for (int X = (int)x - 14; X <= (int)x + 14; X++) {
            float rx = X + 0.5f - x, ry = Y + 0.5f - y, u = rx * dx + ry * dy, v = rx * px + ry * py;
            if (u < 0.0f || u > 12.8f) continue;
            float hw = pwl(US, HB, 7, u);
            if (fabsf(v) > hw) continue;
            float n = v / hw, l = lit(n * px, n * py - 0.3f, sqrtf(fmaxf(0.0f, 1.0f - n * n)));
            int g = 1 + (int)(7.5f * l);                              /* bronze: the gold's dark steps */
            if (u < 2.0f) g = 1 + (int)(4.0f * l);                    /* the handle */
            else if (u > 11.6f) g = 3 + (int)(6.0f * l);              /* the lip, thicker */
            if (l > 0.8f && u > 3.0f && u < 11.6f) g = 10;            /* catching the light */
            if (fabsf(u - 4.3f) < 0.5f || fabsf(u - 10.6f) < 0.45f) g = 1 + (int)(2.5f * l);   /* bands cast round it */
            if (fbm(X / 2.0f, Y / 2.0f, 8, 707) > 0.7f && g > 2) g -= 2;   /* verdigris dulling it */
            uint8_t c = (uint8_t)(R_GOLD + clampi(g, 0, NGOLD - 1));
            if (fbm(X / 2.0f, Y / 2.0f, 8, 707) > 0.82f) c = sh(R_MOSS, 0.35f + 0.2f * l);
            if (rime && h01(X, Y, 701) < 0.3f + 0.35f * (n < 0.0f) + 0.2f * (u < 4.5f)) c = (uint8_t)(R_GHOST + 9 + (int)(h01(X, Y, 703) * 6.0f));
            cv_px(X, Y, c);
        }
    float lx = x + dx * 13.4f, ly = y + dy * 13.4f;                  /* the clapper, past the lip */
    cv_flat(lx, ly, 1.3f, 1.3f, sh(R_IRON, 0.2f)); cv_px((int)lx - 1, (int)ly - 1, sh(R_IRON, 0.5f));
    if (rime)                                                        /* icicles off the lip, straight down whatever the swing */
        for (int k = -5; k <= 5; k += 2) {
            float ix = x + dx * 12.6f + px * k * 1.2f, iy = y + dy * 12.6f + py * k * 1.2f;
            int l = 1 + (int)(h01(k + 9, 1, 705) * 4.0f);
            for (int j = 1; j <= l; j++) cv_px((int)ix, (int)iy + j, (uint8_t)(R_GHOST + 14 - j * 2));
        }
}
/* a bell's ring, drawn: arcs either side of the bell hung at (x, y) along the angle a, as it shakes */
static void bell_rings(float x, float y, float a, uint32_t seed) {
    float dx = sinf(a), dy = cosf(a), cx = x + dx * 7.0f, cy = y + dy * 7.0f;
    for (int side = -1; side <= 1; side += 2) {
        float ang = atan2f(-dx * side, dy * side);                    /* out across the bell */
        for (int k = 0; k < 2; k++) {
            float r = 9.0f + 3.5f * k;
            for (float t = -0.55f; t <= 0.55f; t += 0.8f / r) {
                if (h01((int)(t * 20.0f), k + side * 3, seed) < 0.15f) continue;
                cv_px((int)(cx + cosf(ang + t) * r), (int)(cy + sinf(ang + t) * r), sh(R_GHOST, 0.9f - 0.2f * k));
            }
        }
    }
}

/* the Sexton's face: an old man long dead, the nose long, the cheeks fallen in under the bones, sunken sockets with a
 * candle's light low in each, a lipless mouth and a few teeth; grey hair under the hat. Hex R_WARM, g-v R_STONE. */
static const char *const SEXTON_FACE_ROWS[14] = {
    "r.356777653.r",
    "q35789a98653q",
    "q4000a9a0004q",
    "q50@08a80@05q",
    "q56007a70065q",
    ".47896a69874.",
    ".35245b54253.",
    ".35237c73253.",
    "..423060324..",
    "..324555423..",
    "..30b0c0b03..",
    "..340000043..",
    "...3567653...",
    "....34543....",
};
static const char *const SEXTON_SHOUT[4] = {                       /* rows 9-12 as he rings: the mouth open */
    "..324000423..",
    "..300000003..",
    "..340b0b043..",
    "...3500053...",
};

/* the slouch hat, its brim wide, its crown dented, its band; seen from a little above, as he stoops */
static void slouch_hat(float cx, float by) {
    for (int y = (int)by - 4; y <= (int)by + 4; y++)                  /* the brim */
        for (int x = (int)cx - 16; x <= (int)cx + 16; x++) {
            float nx = (x + 0.5f - cx) / 15.0f, ny = (y + 0.5f - by) / 3.4f;
            if (nx * nx + ny * ny > 1.0f) continue;
            if (nx > 0.62f && nx < 0.74f && ny > 0.1f) continue;     /* a tear in it */
            float v = ny < 0.0f ? 0.3f - 0.12f * nx : 0.2f - 0.08f * nx;
            if (ny > 0.72f) v = 0.06f;                                /* its front edge, turned down */
            cv_px(x, y, sh(R_IRON, v + 0.03f * h01(x, y, 711)));
        }
    for (int y = (int)by - 10; y <= (int)by - 1; y++)                 /* the crown */
        for (int x = (int)cx - 7; x <= (int)cx + 7; x++) {
            float nx = (x + 0.5f - cx) / 6.4f, ny = (y + 0.5f - (by - 3.0f)) / 7.5f, d = nx * nx + ny * ny;
            if (d > 1.0f || y > by - 1) continue;
            float v = 0.05f + 0.36f * lit(nx, ny, sqrtf(1.0f - d));
            if (fabsf(x + 0.5f - cx - 1.0f) < 0.8f && y < by - 6) v -= 0.08f;   /* the dent */
            if (y >= by - 3) v = 0.04f + 0.04f * (nx < 0.0f);              /* the band */
            cv_px(x, y, sh(R_IRON, v));
        }
}

/* The Sexton, the bell-ringer: a stooped keeper of the dead in a long coat and a caped collar (R_WOOD, PF_HIDE: the
 * coat takes the rank), a battered slouch hat, his spade over his shoulder and a handbell on a rope. pose 0-1 walking,
 * 2-3 ringing hard (the bell swung up, then down), 4 walking with the bell frozen, 5 fallen. */
static void spr_sexton(int id, float w, int pose) {
    cv_begin(cv_sprite, TS, TS);
    uint8_t eye = (uint8_t)(R_FLAME + 13);
    if (pose == 5) {
        cv_flat(32, 61.5f, 25, 2.5f, R_WARM + 1);
        float m[12] = { 10, 62, 15, 54, 28, 50, 42, 52, 52, 56, 55, 62 };   /* the coat, empty but for bones */
        cv_poly(m, 6, R_WOOD, 0.36f, 0.08f, 50, 12);
        cv_capsule(6, 52, 46, 61, 0.9f, R_WARM, 0.2f, 0.5f);         /* the spade, fallen across */
        float bl[8] = { 1, 49, 7, 47, 9, 54, 3, 56 };
        cv_poly(bl, 4, R_IRON, 0.55f, 0.2f, 47, 9);
        grid_stamp(SEXTON_FACE_ROWS, 13, 14, 24, 45, R_WARM, R_STONE, 0, 0);
        for (int y = 45; y < 59; y++) for (int x = 24; x < 37; x++) { uint8_t c = cv_sprite[y * TS + x]; if (c > R_WARM + 2 && c < R_WARM + 16) cv_px(x, y, (uint8_t)(c - 2)); }
        slouch_hat(15, 59);
        hand_bell(45, 51, 1.45f, 0);
        cv_capsule(33, 57, 40, 60, 1.6f, R_WARM, 0.2f, 0.6f);         /* a hand out of the sleeve */
        cv_outline(0); cv_store(id); return;
    }
    int back = g_back;                                              /* 1.0.0b18: from behind, walking; drawn as from the front and turned */
    if (back) pose &= 1;
    int ring = pose == 2 || pose == 3, iced = pose == 4;
    cv_flat(32, 62, 14, 1.8f, R_WARM + 1);
    /* thin legs in black stockings under the coat, buckled shoes; the knees bent */
    for (int s = -1; s <= 1; s += 2) {
        float k = s * (ring ? 0.3f : w), x = 32 + s * 3.5f - k * 2.0f;
        cv_capsule(x, 54, x - k, 60, 1.3f, R_IRON, 0.1f, 0.45f);
        cv_ellipse(x - k + s * 0.5f, 61.2f, 2.8f, 1.4f, R_IRON, 0.08f, 0.5f);
        cv_px((int)(x - k), 60, sh(R_STONE, 0.75f));
    }
    /* the long coat, the shoulders hunched up round his ears, the hem down to his shins */
    static const float YS[6] = { 20, 24, 32, 42, 50, 57 }, HW[6] = { 11.0f, 13.0f, 11.0f, 10.0f, 11.5f, 12.5f };
    for (int y = 20; y <= 58; y++) {
        float hw = pwl(YS, HW, 6, (float)y), sw = y > 30 ? w * (y - 30) * 0.06f : 0.0f;
        for (int x = (int)(32 - hw + sw) - 1; x <= (int)(32 + hw + sw) + 1; x++) {
            float dx = x + 0.5f - 32 - sw;
            if (fabsf(dx) > hw) continue;
            if (y > 54 && h01(x, y, 713) < (y - 54) / 5.0f) continue;   /* the hem, frayed */
            float u = dx / hw, v = 0.12f + 0.3f * (1.0f - u * u) - 0.06f * u;
            if (sinf(dx * 1.5f + 0.4f) > 0.6f && y > 30) v -= 0.07f;   /* folds */
            if (fabsf(dx) < 0.6f && y > 26) v = back ? v - 0.06f : 0.05f;   /* where it closes; from behind, its seam */
            if (back && y > 44 && fabsf(dx) < 0.7f) v = 0.04f;              /* and the slit up the back of the skirts */
            if (fbm(x / 4.0f, y / 4.0f, 16, 715) < 0.28f) v -= 0.05f;   /* worn */
            cv_px(x, y, sh(R_WOOD, v));
        }
        if (!back && y > 33 && y < 54 && (y - 34) % 5 == 0) { cv_px(31, y, sh(R_IRON, 0.5f)); cv_px(32, y, sh(R_IRON, 0.3f)); }   /* its buttons */
    }
    for (int y = 8; y <= 30; y++)                                     /* the hump of his back, above his bowed head */
        for (int x = 19; x <= 45; x++) {
            float nx = (x + 0.5f - 32.0f) / 12.0f, ny = (y + 0.5f - 21.0f) / 12.0f, d = nx * nx + ny * ny;
            if (d > 1.0f) continue;
            cv_px(x, y, sh(R_WOOD, 0.08f + 0.4f * lit(nx, ny, sqrtf(1.0f - d))));
        }
    for (int y = 14; y <= 35; y++) {                                  /* the cape over his shoulders, its edge scalloped */
        float hw = y < 22 ? 9.0f + (y - 14) * 0.6f : 13.8f + (y - 22) * 0.12f;
        for (int x = (int)(32 - hw) - 1; x <= (int)(32 + hw) + 1; x++) {
            float dx = x + 0.5f - 32, u = dx / hw;
            if (fabsf(dx) > hw) continue;
            float edge = 32.5f + 2.0f * fabsf(sinf(dx * 0.55f));
            if (y > edge) continue;
            float ny = (y - 14.0f) / 20.0f, v = 0.1f + 0.4f * lit(u * 0.8f, ny - 0.5f, sqrtf(fmaxf(0.0f, 1.0f - u * u * 0.64f)));
            if (y > edge - 1.0f) v += 0.1f;                           /* its hem catching the light */
            if (y < 16 + fabsf(dx) * 0.3f && fabsf(dx) > 8.0f) continue;
            cv_px(x, y, sh(R_WOOD, v));
        }
    }
    /* the arms: the left on the spade's haft, the right on the bell's rope */
    float bx, by, ba;                                                 /* where the bell hangs and its swing */
    {
        float sx = 21, sy = 26, ex = 18, ey = 32, hx = 21, hy = 37;
        cv_capsule(sx, sy, ex, ey, 2.8f, R_WOOD, 0.1f, 0.4f); cv_capsule(ex, ey, hx, hy - 1, 2.5f, R_WOOD, 0.1f, 0.4f);
        float rx = 43, ry = 26, qx, qy, gx, gy;
        if (pose == 2) { qx = 49; qy = 21; gx = 48; gy = 14; bx = 49.0f; by = 13.0f; ba = 2.5f; }
        else if (pose == 3) { qx = 49; qy = 21; gx = 48; gy = 14; bx = 50.0f; by = 17.0f; ba = 0.45f; }
        else { qx = 46 + w * 0.3f; qy = 32; gx = 46 + w * 0.5f; gy = 38; bx = 46.5f + w * 0.8f; by = 42.0f; ba = -w * 0.1f; }
        cv_capsule(rx, ry, qx, qy, 2.8f, R_WOOD, 0.1f, 0.45f); cv_capsule(qx, qy, gx, gy + 1, 2.5f, R_WOOD, 0.1f, 0.45f);
        cv_line(gx, gy, bx, by, sh(R_WARM, 0.35f)); cv_line(gx + 1, gy, bx + 1, by, sh(R_WARM, 0.2f));   /* the rope */
        cv_ellipse(gx, gy, 1.9f, 1.7f, R_WARM, 0.25f, 0.7f);          /* the hand on it, long fingers round it */
        for (int f = 0; f < 3; f++) cv_px((int)gx - 1 + f, (int)gy + 2, sh(R_WARM, 0.45f - 0.08f * f));
        hand_bell(bx, by, ba, iced);
        if (ring) bell_rings(bx, by, ba, 717 + (uint32_t)pose);
    }
    /* the spade over his shoulder: its haft in his hand, its blade up behind, rusty and caked */
    {
        float hx = 21, hy = 37, tx = 11.5f, ty = 6, ax = tx - hx, ay = ty - hy, L = hypotf(ax, ay);
        ax /= L; ay /= L;
        float pxx = -ay, pyy = ax;
        cv_capsule(hx - ax * 4.0f, hy - ay * 4.0f, tx, ty, 0.9f, R_WARM, 0.18f, 0.55f);
        cv_capsule(hx - ax * 4.0f - pxx * 1.6f, hy - ay * 4.0f - pyy * 1.6f, hx - ax * 4.0f + pxx * 1.6f, hy - ay * 4.0f + pyy * 1.6f, 0.7f, R_WARM, 0.2f, 0.55f);   /* the grip */
        float b[8] = { tx + pxx * 3.2f, ty + pyy * 3.2f, tx - pxx * 3.2f, ty - pyy * 3.2f, tx + ax * 9.0f - pxx * 2.6f, ty + ay * 9.0f - pyy * 2.6f, tx + ax * 9.0f + pxx * 2.6f, ty + ay * 9.0f + pyy * 2.6f };
        cv_poly(b, 4, R_IRON, 0.65f, 0.2f, ty + ay * 9.0f, 9);
        for (int y = 0; y < 16; y++) for (int x = 1; x < 20; x++) {
            uint8_t c = cv_sprite[y * TS + x];
            if (c > R_IRON && c < R_IRON + 16 && fbm(x / 2.0f, y / 2.0f, 8, 719) > 0.62f) cv_px(x, y, fbm(x / 3.0f, y / 3.0f, 8, 721) > 0.5f ? sh(R_BRICK, 0.3f) : sh(R_WARM, 0.15f));   /* rust and grave-dirt */
        }
        cv_ellipse(hx, hy, 1.9f, 1.7f, R_WARM, 0.25f, 0.7f);
        for (int f = 0; f < 3; f++) cv_px((int)hx + 1, (int)hy - 1 + f, sh(R_WARM, 0.5f - 0.1f * f));
    }
    /* the face, jutting forward and low under the hat's brim */
    {
        const char *rows[14];
        for (int i = 0; i < 14; i++) rows[i] = SEXTON_FACE_ROWS[i];
        if (ring) for (int i = 0; i < 4; i++) rows[9 + i] = SEXTON_SHOUT[i];
        if (back) {                                                 /* from behind: no face, but the coat's collar turned up, lank hair under the brim */
            for (int y = 20; y <= 27; y++) for (int x = 26; x <= 38; x++) {
                float nx = (x + 0.5f - 32.0f) / 6.5f;
                if (fabsf(nx) > 1.0f) continue;
                cv_px(x, y, (x * 2 + y / 3) % 3 == 0 ? sh(R_IRON, 0.55f) : sh(R_IRON, 0.28f + 0.12f * (1.0f - nx * nx)));   /* grey, lank, in strands */
            }
            for (int x = 24; x <= 40; x++) for (int y = 26; y <= 30; y++) cv_px(x, y, sh(R_WOOD, 0.14f + 0.05f * (y == 26)));
        } else {
            cv_flat(32, 31, 6.5f, 4.5f, sh(R_WOOD, 0.06f));          /* his collar, dark behind it */
            grid_stamp(rows, 13, 14, 26, 20, R_WARM, R_STONE, eye, 0);
        }
    }
    slouch_hat(32, 20);
    if (back) cv_mirror();
    cv_outline(0);
    cv_store(id);
}

/* the Sexton's dressing by rank, on the coat: II mould and the hem torn, III frost on the cape and hat, IV blood soaked up
 * the hem and down the front, V the coat charred and the keys of the dead on an iron ring at his belt, gilded gold thread
 * at the coat's edges and gold buttons */
static void sexton_dress(int id, float w) {
    for (int r = RK_II; r < NRANK; r++) {
        dress_begin(id);
        uint32_t sd = 731u + (uint32_t)r * 5u;
        if (r == RK_GILDED) {
            for (int y = 0; y < TS; y++) for (int x = 0; x < TS; x++) {
                uint8_t b = base_c(x, y);
                if (dress_on(b, R_WOOD) && (!dress_on(base_c(x - 1, y), R_WOOD) || !dress_on(base_c(x + 1, y), R_WOOD) || !dress_on(base_c(x, y + 1), R_WOOD)))
                    cv_px(x, y, (uint8_t)(R_GOLD + 8 + (int)(h01(x, y, sd) * 3.0f)));
            }
            if (!g_back) for (int y = 29; y < 54; y += 6) { cv_px(31, y, (uint8_t)(R_GOLD + 11)); cv_px(32, y, (uint8_t)(R_GOLD + 7)); }   /* buttons, in front */
        } else {
            for (int x = 18; x < 47; x++) if (base_on(x, 56) && h01(x, r, sd) < 0.3f + 0.05f * r) for (int y = 52 + (int)(h01(x, 1, sd) * 4.0f); y < 60; y++) if (dress_on(base_c(x, y), R_WOOD)) cv_px(x, y, 0);   /* the hem torn */
            if (r == RK_II) kit_moss(0.32f, 2, sd + 1);
            if (r == RK_III) kit_frost(0.8f, sd + 2);
            if (r >= RK_IV) for (int y = 30; y < 60; y++) for (int x = 18; x < 47; x++) if (dress_on(base_c(x, y), R_WOOD) && fbm(x / 3.0f, y / 2.0f, 16, sd + 3) > 0.62f - (y - 30) * 0.008f) cv_px(x, y, sh(R_BLOOD, 0.1f + 0.12f * h01(x, y, sd + 4)));
            if (r == RK_V) {
                kit_char(0.3f, sd + 5);
                kit_chain(26, 38, 24, 46, 1.0f, R_IRON);
                float kx = 24, ky = 47;
                for (int y = -2; y <= 2; y++) for (int x = -2; x <= 2; x++) if (abs(x * x + y * y - 4) <= 1) cv_px((int)kx + x, (int)ky + y, sh(R_IRON, 0.55f + 0.1f * (x < 0)));   /* the ring */
                for (int k = 0; k < 3; k++) {                          /* its keys */
                    float a = 1.2f + k * 0.45f, ex = kx + cosf(a) * 7.0f, ey = ky + sinf(a) * 7.0f;
                    cv_line(kx + cosf(a) * 2.0f, ky + sinf(a) * 2.0f, ex, ey, sh(R_IRON, 0.6f));
                    cv_px((int)ex, (int)ey, sh(R_IRON, 0.75f)); cv_px((int)ex + 1, (int)ey, sh(R_IRON, 0.45f));
                }
            }
        }
        dress_store(id, r, R_WOOD);
    }
}


/* a shard of black glass (R_IRON) from (x0, y0), w0 across, to (x1, y1), w1 across: two facets either side of a crest,
 * the one towards the light a little less black, its edge catching the light (R_GHOST) in a broken line, the crest a
 * thin gleam, the edge away from it lost in the dark */
static void glass_shard(float x0, float y0, float x1, float y1, float w0, float w1, uint32_t seed) {
    float ax = x1 - x0, ay = y1 - y0, L = hypotf(ax, ay) + 1e-3f;
    ax /= L; ay /= L;
    float px = -ay, py = ax, wm = fmaxf(w0, w1) * 0.5f + 1.0f;
    for (int Y = (int)floorf(fminf(y0, y1) - wm); Y <= (int)ceilf(fmaxf(y0, y1) + wm); Y++)
        for (int X = (int)floorf(fminf(x0, x1) - wm); X <= (int)ceilf(fmaxf(x0, x1) + wm); X++) {
            float rx = X + 0.5f - x0, ry = Y + 0.5f - y0, t = (rx * ax + ry * ay) / L, s = rx * px + ry * py;
            if (t < 0.0f || t > 1.0f) continue;
            float hw = lerpf(w0, w1, t) * 0.5f;
            if (fabsf(s) > hw || hw < 0.25f) continue;
            float sg = s < 0.0f ? -1.0f : 1.0f, l = lit(px * sg * 0.75f, py * sg * 0.75f, 0.66f), e = hw - fabsf(s);
            float v = 0.04f + 0.36f * l * (0.6f + 0.4f * fabsf(s) / hw);   /* the facet, lighter towards its edge */
            uint8_t c;
            if (hw > 2.2f) {                                          /* a third facet, facing out, between creases */
                if (fabsf(s) < hw * 0.3f) v = 0.19f + 0.07f * (s > 0.0f);
                else if (fabsf(s) < hw * 0.3f + 0.8f) v = 0.02f;
            }
            float n = fbm(X / 3.0f, Y / 3.0f, 8, seed);
            if (e < 0.75f && l > 0.5f) c = n > 0.78f ? (uint8_t)C_WHITE : sh(R_GHOST, 0.25f + 0.6f * n);   /* the edge, catching the light */
            else c = sh(R_IRON, v);
            cv_px(X, Y, c);
        }
}
/* its fingers: three spikes of glass from (x, y), spread about the angle a */
static void glass_fingers(float x, float y, float a, float len, float spread, uint32_t seed) {
    for (int f = -1; f <= 1; f++) {
        float b = a + f * spread, l = len * (f == 0 ? 1.0f : 0.85f);
        glass_shard(x, y, x + cosf(b) * l, y + sinf(b) * l, 2.2f, 0.1f, seed + (uint32_t)(f + 1));
    }
}
/* its head: a long lozenge of glass with no face, its crest down the middle; tilted by a (radians) about its neck */
static void glass_head(float nx, float ny, float a, float narrow, uint32_t seed) {
    float c = cosf(a), s = sinf(a);
    #define HR(dx, dy) nx + (dx) * c - (dy) * s, ny + (dx) * s + (dy) * c
    for (int k = -1; k <= 1; k++)                                     /* a crest of shards swept back off its crown */
        glass_shard(HR(k * 2.2f * narrow, -12.0f + fabsf((float)k)), HR(k * 6.5f * narrow, -18.5f + fabsf((float)k) * 2.5f), 2.6f, 0.2f, seed + 5 + (uint32_t)(k + 1));
    glass_shard(HR(0.0f, -15.5f), HR(0.0f, -8.0f), 2.0f, 9.5f * narrow, seed);
    glass_shard(HR(0.0f, -8.0f), HR(0.0f, 0.0f), 9.5f * narrow, 2.0f, seed + 1);
    #undef HR
}

/* The Lightless, the stalker of the deep: tall and thin, of black glass (R_IRON, PF_CHITIN: it takes the rank in its
 * dark), no eyes and no face, all facets and points, its edges catching what light there is. pose 0-1 walking, 2 the
 * strike, 3 listening (still, its head turned and tilted, a hand up feeling the air), 4 shattered. */
static void spr_lightless(int id, float w, int pose) {
    cv_begin(cv_sprite, TS, TS);
    uint32_t sd = 801;
    if (pose == 4) {
        cv_flat(32, 61.5f, 24, 2.5f, R_WARM + 1);
        for (int i = 0; i < 16; i++) {                               /* shards on the floor, a few still standing */
            float cx = 32 + (h01(i, 1, sd) - 0.5f) * 44, cy = 61 - 4 * h01(i, 2, sd), a = h01(i, 3, sd) * PI_F, l = 3 + 6 * h01(i, 4, sd);
            if (i < 4) glass_shard(cx, cy, cx + (h01(i, 5, sd) - 0.5f) * 6, cy - l * 1.3f, 2.8f, 0.2f, sd + i);
            else glass_shard(cx - cosf(a) * l * 0.5f, cy - sinf(a) * l * 0.25f, cx + cosf(a) * l * 0.5f, cy + sinf(a) * l * 0.25f, 2.4f, 0.6f, sd + i);
        }
        glass_head(20, 58, -1.35f, 1.0f, sd + 40);
        cv_outline(0); cv_store(id); return;
    }
    int strike = pose == 2, listen = pose == 3;
    float ww = strike || listen ? 0.2f : w;
    cv_flat(32, 62, 13, 1.6f, R_WARM + 1);
    /* legs: long, the knee a spur behind it, the foot a point */
    for (int s = -1; s <= 1; s += 2) {
        float k = s * ww, hx = 32 + s * 3.0f, kx = 32 + s * 4.5f - k * 2.5f, fx = 32 + s * 4.0f - k * 4.5f;
        if (strike) { kx += s * 2.0f; fx += s * 4.0f; }
        glass_shard(hx, 40, kx, 51, 5.0f, 3.2f, sd + 2 + (uint32_t)(s + 1));
        glass_shard(kx + s * 0.5f, 50, kx + s * 3.0f, 46.0f, 2.4f, 0.1f, sd + 5);   /* the spur */
        glass_shard(kx, 51, fx, 60, 3.2f, 1.8f, sd + 6 + (uint32_t)(s + 1));
        glass_shard(fx - s * 1.0f, 60, fx + s * 4.0f, 62.5f, 2.4f, 0.2f, sd + 9);
    }
    /* the hips, then the body: broad at the chest, narrowing to a waist you could close a hand round */
    glass_shard(32, 35, 32, 43, 10.0f, 2.5f, sd + 10);
    glass_shard(32, 18, 32, 38, 16.0f, 4.0f, sd + 11);
    for (int k = 0; k < 4; k++) {                                     /* ribs of it, the light along their left */
        float y = 23.0f + k * 3.5f, hw = 6.0f - k * 1.1f;
        for (float x = 32 - hw; x < 31.5f; x += 1.0f) if (h01((int)x, k, sd) < 0.7f) cv_px((int)x, (int)(y - (x - 32 + hw) * 0.25f), sh(R_GHOST, 0.22f + 0.04f * (x - 32 + hw)));
        cv_line(32.5f, y - hw * 0.25f, 32 + hw, y, sh(R_IRON, 0.01f));
    }
    /* the yoke of the shoulders, pointed, a spike rising from each */
    glass_shard(32, 19, 18.5f, 21.5f, 6.5f, 1.0f, sd + 12);
    glass_shard(32, 19, 45.5f, 21.5f, 6.5f, 1.0f, sd + 13);
    glass_shard(21.5f, 20, 18.0f, 13.0f, 2.8f, 0.2f, sd + 24);
    glass_shard(42.5f, 20, 46.0f, 13.0f, 2.8f, 0.2f, sd + 25);
    /* arms: long enough to reach its knees, the fingers long spikes */
    for (int s = -1; s <= 1; s += 2) {
        float sx = 32 + s * 11.5f, sy = 21.5f, ex, ey, hx, hy, fa, fl = 10.0f;
        if (strike) { ex = 32 + s * 18.0f; ey = 12; hx = 32 + s * 10.0f; hy = 4; fa = s < 0 ? -0.4f : -2.75f; }
        else if (listen && s > 0) { ex = 47; ey = 14; hx = 41; hy = 7; fa = -1.9f; fl = 8.0f; }
        else { ex = 32 + s * 15.0f + s * ww * 0.6f; ey = 34; hx = 32 + s * 14.0f - s * ww * 1.2f; hy = 46; fa = 1.5708f - s * 0.15f; }
        glass_shard(sx, sy, ex, ey, 4.2f, 3.0f, sd + 14 + (uint32_t)(s + 1));
        glass_shard(ex, ey, hx, hy, 3.0f, 2.0f, sd + 17 + (uint32_t)(s + 1));
        glass_fingers(hx, hy, fa, fl, listen && s > 0 ? 0.45f : 0.2f, sd + 20 + (uint32_t)(s + 1) * 3u);
    }
    /* the neck, and the head with nothing in its face */
    glass_shard(32, 13, 32, 20, 3.0f, 2.6f, sd + 30);
    glass_head(listen ? 31.0f : 32.0f, 16.0f, listen ? -0.45f : strike ? 0.0f : ww * 0.04f, listen ? 0.7f : 1.0f, sd + 40);
    for (int i = 0; i < 4; i++) cv_px(27 + i, 25 + i, sh(R_IRON, 0.36f - 0.04f * i));   /* a gleam inside it, deep in the glass */
    cv_outline(0);
    cv_store(id);
}

/* the Lightless's dressing by rank: II a dull film in its seams and the edges scuffed, III rime on its upper edges and a
 * crack across its chest, IV cracks more, blood dried on its fingers and run down them, V soot and an iron collar on
 * its chain, gilded mended with gold, as broken pots are, its seams and cracks gold */
static void lightless_dress(int id, int pose) {
    g_dress_ramp2 = R_GHOST;
    for (int r = RK_II; r < NRANK; r++) {
        dress_begin(id);
        uint32_t sd = 821u + (uint32_t)r * 9u;
        if (r == RK_GILDED) {
            for (int k = 0; k < 7; k++) {                             /* veins of gold, wandering */
                float x = 22 + 20 * h01(k, 1, sd), y = 14 + 40 * h01(k, 2, sd), a = h01(k, 3, sd) * TAU_F;
                for (int i = 0; i < 16; i++) {
                    a += (h01(k, i + 4, sd) - 0.5f) * 1.2f; x += cosf(a); y += sinf(a);
                    if (base_on((int)x, (int)y)) cv_px((int)x, (int)y, (uint8_t)(R_GOLD + 8 + (i & 3)));
                }
            }
            for (int y = 0; y < TS; y++) for (int x = 0; x < TS; x++) if (dress_on(base_c(x, y), R_GHOST) && h01(x, y, sd) < 0.5f) cv_px(x, y, (uint8_t)(R_GOLD + 10 + (int)(h01(x, y, sd + 1) * 3.0f)));
        } else {
            for (int y = 0; y < TS; y++) for (int x = 0; x < TS; x++)   /* the edges scuffed */
                if (dress_on(base_c(x, y), R_GHOST) && h01(x, y, sd + 2) < 0.12f + 0.06f * r) cv_px(x, y, sh(R_IRON, 0.2f));
            if (r == RK_II) kit_moss(0.22f, 0, sd + 3);
            if (r == RK_III) kit_frost(0.85f, sd + 4);
            if (r >= RK_III) {                                         /* a crack across the chest, pale where it catches */
                float x = 27, y = 22;
                for (int i = 0; i < 12; i++) { x += 0.9f; y += 0.4f + (h01(i, 1, sd) - 0.5f) * 1.2f; if (base_on((int)x, (int)y)) cv_px((int)x, (int)y, sh(R_GHOST, 0.75f - 0.03f * i)); }
            }
            if (r >= RK_IV) {
                float x = 34, y = 30;
                for (int i = 0; i < 10; i++) { x -= 0.8f; y += 0.9f + (h01(i, 2, sd) - 0.5f); if (base_on((int)x, (int)y)) cv_px((int)x, (int)y, sh(R_GHOST, 0.7f)); }
                if (pose < 2) { kit_blood(19, 48, 8, 3, sd + 5); kit_blood(45, 48, 8, 3, sd + 6); }
                else kit_blood(pose == 2 ? 22 : 40, pose == 2 ? 3 : 6, 7, 3, sd + 5);
            }
            if (r == RK_V) {
                kit_char(0.32f, sd + 7);
                for (int x = 28; x <= 36; x++) { cv_px(x, 17, sh(R_IRON, 0.6f - 0.03f * (x - 28))); cv_px(x, 18, sh(R_IRON, 0.3f)); }   /* the collar */
                kit_chain(36, 18, 47, 30, 2.0f, R_IRON);
            }
        }
        dress_store(id, r, R_IRON);
    }
    g_dress_ramp2 = -1;
}


/* the small skull in profile, facing right (the Whipper-In's): hex R_BONE, @ the eye */
static const char *const SKULL_SMALL_SIDE[10] = {
    "..356775..", ".36899987.", "3689999a97", "468999a008", "479aa9a0@9", "36899a9a06", ".5799b8a97", "..57a0a0a6", "..48a0a0a.", "...4677...",
};

/* the Whipper-In from the side, walking right: his coat's tails flying behind, the cap's peak ahead, the whip trailing */
static void spr_whip_side(int id, float w) {
    cv_begin(cv_sprite, TS, TS);
    float bob = fabsf(w) * 0.6f;
    w *= 1.5f;
    cv_flat(31, 62, 14, 1.8f, R_WARM + 1);
    float ws[8] = { 26, 44, 20, 52, 14, 62, 3, 61 };                  /* the whip's thong, dragging behind */
    sinew_lash(ws, 0, 651);
    /* the far leg, darker, in its boot */
    float fk = 31 - 4 * w, ff = 31 - 5 * w;
    cv_capsule(31, 41, fk, 51, 1.1f, R_BONE, 0.12f, 0.5f);
    cv_capsule(fk, 52.5f, ff, 59.5f, 2.1f, R_IRON, 0.05f, 0.3f); cv_capsule(ff, 61, ff + 4.5f, 61.5f, 1.3f, R_IRON, 0.05f, 0.3f);
    cv_capsule(fk - 2.3f, 51.5f, fk + 2.3f, 51.5f, 1.0f, R_WOOD, 0.15f, 0.35f);
    /* the coat's tails, flying back with the stride */
    float tl[10] = { 27, 30, 33, 31, 31, 46, 22 - w * 1.5f, 53, 17 - w * 2.0f, 47 };
    cv_poly(tl, 5, R_BLOOD, 0.08f, 0.22f, 30, 23);
    for (int y = 44; y < 54; y++) for (int x = 14; x < 30; x++) if (h01(x, y, 653) < (y - 44) / 12.0f && cv_sprite[y * TS + x] >= R_BLOOD && cv_sprite[y * TS + x] < R_BLOOD + 16) cv_px(x, y, C_CLEAR);
    /* spine, ribs in profile, the pelvis */
    cv_capsule(31, 16 + bob, 29.5f, 27 + bob, 1.2f, R_BONE, 0.3f, 0.9f); cv_capsule(29.5f, 27 + bob, 31, 39, 1.2f, R_BONE, 0.3f, 0.9f);
    for (int i = 0; i < 4; i++) {
        float y0 = 20.5f + i * 3.2f + bob;
        cv_capsule(30, y0, 36.0f - i * 0.4f, y0 + 1.5f, 0.75f, R_BONE, 0.25f, 0.9f); cv_capsule(36.0f - i * 0.4f, y0 + 1.5f, 34.5f - i * 0.5f, y0 + 4.0f, 0.7f, R_BONE, 0.25f, 0.85f);
    }
    cv_ellipse(31, 40, 3.6f, 2.8f, R_BONE, 0.3f, 0.9f);
    /* the coat over his back and shoulder, open at the front, its skirt to the knee */
    for (int y = 17; y <= 47; y++) {
        float back = y < 30 ? 24.5f + (y - 17) * 0.15f : 26.5f - (y - 30) * 0.3f, front = y < 30 ? 30.0f : 33.0f + (y - 30) * 0.25f + w * (y - 30) * 0.08f;
        if (y < 21) front = 33.0f;
        for (int x = (int)back; x <= (int)front; x++) {
            if (y > 43 && h01(x, y, 655) < (y - 43) / 6.0f) continue;
            if (fbm(x / 2.5f, y / 2.5f, 8, 657) > 0.8f) continue;
            float u = (x + 0.5f - back) / (front - back + 0.01f), v = 0.12f + 0.3f * sinf(u * PI_F) + 0.06f * u;
            cv_px(x, y, sh(R_BLOOD, v));
        }
    }
    cv_ellipse(31, 18, 2.0f, 1.6f, R_STONE, 0.35f, 0.95f);            /* the stock */
    cv_capsule(32, 19, 33.5f, 23, 0.8f, R_STONE, 0.3f, 0.85f);
    /* the near leg, striding, its boot */
    float nk = 31 + 4 * w, nf = 31 + 5 * w - 1;
    cv_capsule(31, 41, nk, 51, 1.2f, R_BONE, 0.3f, 0.95f);
    cv_ellipse(nk + 0.5f, 50.3f, 1.4f, 1.3f, R_BONE, 0.35f, 1.0f);
    cv_capsule(nk, 52.5f, nf, 59.5f, 2.2f, R_IRON, 0.08f, 0.62f); cv_capsule(nf, 61, nf + 5, 61.5f, 1.4f, R_IRON, 0.08f, 0.6f);
    cv_capsule(nk - 2.4f, 51.5f, nk + 2.4f, 51.5f, 1.1f, R_WOOD, 0.3f, 0.7f);
    cv_px((int)nf - 2, 60, sh(R_GHOST, 0.55f));                       /* the spur */
    /* the horn at his hip on its cord */
    cv_line(33, 19, 30, 36, sh(R_WARM, 0.22f));
    ox_horn(30, 36, 39, 39, 2.0f);
    /* the near arm swinging, the whip's stock in its hand, a ragged sleeve to the elbow */
    float ex = 31 - 3 * w, ey = 29, hx = 32 - 5 * w, hy = 36;
    cv_capsule(30.5f, 20 + bob, ex, ey, 2.6f, R_BLOOD, 0.1f, 0.42f);
    cv_bone(ex, ey, hx, hy, 1.0f, 0.25f, 0.92f);
    for (int j = 0; j < 3; j++) cv_px((int)ex - 1 + j, (int)ey + 2 + (j & 1), sh(R_BLOOD, 0.18f));
    cv_ellipse(hx, hy, 1.6f, 1.5f, R_BONE, 0.3f, 0.95f);
    cv_capsule(hx, hy, hx - 3.0f, hy + 7.0f, 0.9f, R_WOOD, 0.2f, 0.6f);   /* the stock, the thong runs back from it */
    cv_line(hx - 3.0f, hy + 7.0f, 26, 44, sh(R_WARM, 0.3f));
    /* the neck, the skull in profile, the cap with its peak ahead */
    cv_capsule(31, 13 + bob, 31, 17 + bob, 1.0f, R_BONE, 0.3f, 0.85f);
    grid_stamp(SKULL_SMALL_SIDE, 10, 10, 27, 5 + (int)bob, R_BONE, R_BONE, (uint8_t)(R_FLAME + 11), 0);
    g_skbox[0] = 27; g_skbox[1] = 5 + (int)bob; g_skbox[2] = 37; g_skbox[3] = 15 + (int)bob;
    hunt_cap(31, bob - 1.0f);
    for (int x = 34; x <= 39; x++) cv_px(x, 6 + (int)bob, sh(R_IRON, 0.3f - 0.03f * (x - 34)));   /* the peak, out in front */
    cv_bone_decay(659);
    cv_outline(0);
    cv_store(id);
}

/* the Sexton from the side, shuffling right: bent nearly double under his hump, the hat's brim level with the floor, the
 * bell held out ahead on its rope, the spade over his shoulder with its blade behind */
static void spr_sexton_side(int id, float w) {
    cv_begin(cv_sprite, TS, TS);
    cv_flat(32, 62, 14, 1.8f, R_WARM + 1);
    /* the spade, its blade up behind him */
    cv_capsule(41, 34, 13, 12, 0.9f, R_WARM, 0.15f, 0.5f);
    float b[8] = { 15, 9, 11, 15, 4, 10, 8, 4 };
    cv_poly(b, 4, R_IRON, 0.55f, 0.2f, 4, 11);
    for (int y = 2; y < 18; y++) for (int x = 2; x < 18; x++) { uint8_t c = cv_sprite[y * TS + x]; if (c > R_IRON && c < R_IRON + 16 && fbm(x / 2.0f, y / 2.0f, 8, 741) > 0.6f) cv_px(x, y, sh(R_BRICK, 0.3f)); }
    /* legs, short steps */
    for (int s = -1; s <= 1; s += 2) {
        float x = 31 + s * w * 2.5f;
        cv_capsule(31, 52, x, 60, 1.3f, R_IRON, s < 0 ? 0.05f : 0.1f, s < 0 ? 0.25f : 0.45f);
        cv_capsule(x - 1, 61.2f, x + 3.5f, 61.5f, 1.3f, R_IRON, 0.08f, s < 0 ? 0.3f : 0.5f);
    }
    /* the coat: the hump high behind, the body bent forward, the hem down to his shins */
    for (int y = 12; y <= 57; y++)
        for (int x = 14; x <= 46; x++) {
            float hump = (x + 0.5f - 27.0f) * (x + 0.5f - 27.0f) / 110.0f + (y + 0.5f - 23.0f) * (y + 0.5f - 23.0f) / 120.0f;
            float back = 21.0f + (y - 30) * 0.12f, front = 37.0f + (y - 30) * 0.25f + w * (y - 30) * 0.05f;
            int in = hump < 1.0f || (y >= 26 && x + 0.5f > back && x + 0.5f < front);
            if (!in) continue;
            if (y > 54 && h01(x, y, 743) < (y - 54) / 4.0f) continue;
            float u = (x + 0.5f - 18.0f) / 24.0f, v = 0.1f + 0.3f * (1.0f - fabsf(u - 0.45f) * 1.6f) - (y > 26 ? 0.0f : 0.0f);
            if (hump < 1.0f && y < 28) v = 0.08f + 0.42f * lit((x + 0.5f - 27.0f) / 10.5f, (y + 0.5f - 23.0f) / 11.0f, 0.5f);
            if (sinf(x * 1.3f + y * 0.15f) > 0.65f && y > 32) v -= 0.06f;
            cv_px(x, y, sh(R_WOOD, v));
        }
    for (int x = 16; x <= 42; x++) { int y = 32 + (int)(2.0f * fabsf(sinf(x * 0.55f))); cv_px(x, y, sh(R_WOOD, 0.42f)); cv_px(x, y + 1, sh(R_WOOD, 0.05f)); }   /* the cape's scalloped edge over it */
    /* the near arm out ahead, the hand on the rope, the bell under it */
    cv_capsule(32, 26, 40, 31, 2.6f, R_WOOD, 0.1f, 0.42f);
    cv_capsule(40, 31, 45, 34, 2.3f, R_WOOD, 0.1f, 0.42f);
    cv_ellipse(46, 35, 1.8f, 1.7f, R_WARM, 0.25f, 0.7f);
    cv_line(46, 36, 47 + w, 40, sh(R_WARM, 0.35f));
    hand_bell(47 + w, 40, -w * 0.12f, 0);
    /* the head, thrust forward and low: a long nose, a sunken eye, grey hair, the hat */
    cv_ellipse(39, 25, 4.6f, 5.0f, R_WARM, 0.15f, 0.65f);
    cv_capsule(42.5f, 24, 45.5f, 27.5f, 1.0f, R_WARM, 0.25f, 0.75f);   /* the nose, long and hooked */
    cv_px(45, 28, sh(R_WARM, 0.3f));
    cv_flat(41, 23, 1.3f, 1.2f, 0); cv_eye(41, 23, (uint8_t)(R_FLAME + 13));
    cv_line(40, 29, 43, 29, 0); cv_px(42, 29, sh(R_BONE, 0.7f));        /* the mouth, a tooth */
    cv_capsule(38, 30, 41, 31, 1.0f, R_WARM, 0.15f, 0.5f);             /* the chin */
    for (int k = 0; k < 4; k++) cv_line(35 - k * 0.5f, 22 + k, 34 - k, 27 + k, sh(R_STONE, 0.55f + 0.08f * (k & 1)));   /* hair */
    cv_px(36, 25, sh(R_WARM, 0.2f)); cv_px(36, 26, sh(R_WARM, 0.3f));  /* the ear */
    for (int y = 13; y <= 22; y++)                                    /* the hat: its crown, then the brim level */
        for (int x = 32; x <= 46; x++) {
            float nx = (x + 0.5f - 39.0f) / 6.0f, ny = (y + 0.5f - 19.5f) / 6.5f, d = nx * nx + ny * ny;
            if (d > 1.0f || y > 20) continue;
            cv_px(x, y, sh(R_IRON, y >= 18 ? 0.04f : 0.05f + 0.35f * lit(nx, ny, sqrtf(1.0f - d))));
        }
    for (int x = 28; x <= 50; x++) { cv_px(x, 21, sh(R_IRON, 0.32f - 0.01f * (x - 28))); cv_px(x, 22, sh(R_IRON, 0.08f)); }
    cv_outline(0);
    cv_store(id);
}

/* the Lightless from the side, stalking right: its head out ahead like a blade with the crest swept back, the chest a keel,
 * the long arms hanging forward, the knees' spurs behind */
static void spr_light_side(int id, float w) {
    cv_begin(cv_sprite, TS, TS);
    uint32_t sd = 861;
    w *= 1.4f;
    cv_flat(31, 62, 13, 1.6f, R_WARM + 1);
    /* the far leg and arm, darker */
    float fk = 30 - 3.5f * w, ff = 31 - 5.5f * w;
    glass_shard(30, 40, fk + 2.0f, 51, 4.0f, 2.6f, sd);
    glass_shard(fk + 2.0f, 51, ff, 60, 2.6f, 1.4f, sd + 1);
    glass_shard(ff, 60.5f, ff + 5, 62, 2.0f, 0.2f, sd + 2);
    glass_shard(34, 22, 38 - 2 * w, 34, 3.2f, 2.4f, sd + 3);
    glass_shard(38 - 2 * w, 34, 42 - 3 * w, 45, 2.4f, 1.6f, sd + 4);
    glass_fingers(42 - 3 * w, 45, 1.25f, 9.0f, 0.22f, sd + 5);
    for (int y = 0; y < TS; y++) for (int x = 0; x < TS; x++) { uint8_t *p = &cv_sprite[y * TS + x]; if (*p != C_CLEAR && *p > R_IRON + 1 && *p < R_IRON + 16) *p = (uint8_t)(*p - 1); else if (*p >= R_GHOST && *p < R_GHOST + 16) *p = sh(R_IRON, 0.25f); }
    /* the body: hips, a narrow waist, the chest a keel thrust forward, the back bent */
    glass_shard(29, 35, 30, 43, 7.0f, 2.5f, sd + 6);
    glass_shard(31, 19, 29, 38, 9.0f, 3.0f, sd + 7);
    glass_shard(31, 21, 37, 27, 4.0f, 0.4f, sd + 8);                   /* the keel */
    glass_shard(27, 19, 22, 13, 3.0f, 0.2f, sd + 9);                   /* a spike off the shoulder, back */
    /* the near leg, striding, the spur behind the knee */
    float nk = 32 + 3.5f * w, nf = 31 + 5.5f * w;
    glass_shard(30, 40, nk, 51, 4.6f, 3.0f, sd + 10);
    glass_shard(nk - 0.5f, 50, nk - 4.0f, 47, 2.4f, 0.1f, sd + 11);
    glass_shard(nk, 51, nf, 60, 3.0f, 1.6f, sd + 12);
    glass_shard(nf, 60.5f, nf + 5, 62, 2.2f, 0.2f, sd + 13);
    /* the near arm, hanging forward to the knee */
    glass_shard(32, 21, 37 + 2 * w, 33, 3.6f, 2.6f, sd + 14);
    glass_shard(37 + 2 * w, 33, 41 + 3 * w, 44, 2.6f, 1.8f, sd + 15);
    glass_fingers(41 + 3 * w, 44, 1.2f, 10.0f, 0.22f, sd + 16);
    /* the neck forward, the head out ahead like a blade, its crest back */
    glass_shard(32, 19, 37, 13, 3.0f, 2.4f, sd + 20);
    glass_shard(36, 11.5f, 46, 15, 7.0f, 1.0f, sd + 21);               /* the face, to a point, with nothing in it */
    glass_shard(36, 11.5f, 30, 9.5f, 7.0f, 2.0f, sd + 22);             /* the back of the skull */
    for (int k = 0; k < 3; k++) glass_shard(34.0f - k * 1.5f, 9.0f + k * 0.5f, 25.0f - k * 2.5f, 4.0f + k * 3.0f, 2.4f, 0.2f, sd + 23 + (uint32_t)k);   /* the crest */
    cv_outline(0);
    cv_store(id);
}

/* the bestiary's bones age with the level, like the skeletons' */
static void spr_bestiary_bones(void) {
    spr_hound(SP_HOUND_A, 1.0f, 0); spr_hound(SP_HOUND_B, -1.0f, 1); spr_hound(SP_HOUND_BITE, 0.0f, 2); spr_hound(SP_HOUND_D, 0.0f, 3);
    hound_dress(SP_HOUND_A, 1.0f, 0); hound_dress(SP_HOUND_B, -1.0f, 1); hound_dress(SP_HOUND_BITE, 0.0f, 2);
    spr_hound_back(SP_HOUND_BKA, 1.0f); spr_hound_back(SP_HOUND_BKB, -1.0f); hound_dress_back(SP_HOUND_BKA); hound_dress_back(SP_HOUND_BKB);   /* 1.0.0b18 */
    for (int st = 0; st < 3; st++) {
        int base = SP_KNIGHT_A + st * 4;
        spr_knight(base, 1.0f, 0, st); spr_knight(base + 1, -1.0f, 1, st); spr_knight(base + 2, 0.0f, 2, st); spr_knight(base + 3, 0.0f, 3, st);
        for (int k = 0; k < 4; k++) knight_dress(base + k, st);
        spr_knight(SP_KNIGHT_P0 + st, 0.0f, 4, st);
        g_back = 1;                                                 /* 1.0.0b18: its back, walking, in each state of its plate */
        spr_knight(SP_KNIGHT_BKA + st * 2, 1.0f, 0, st); spr_knight(SP_KNIGHT_BKA + st * 2 + 1, -1.0f, 1, st);
        g_back = 0;
        knight_dress_back(SP_KNIGHT_BKA + st * 2, st); knight_dress_back(SP_KNIGHT_BKA + st * 2 + 1, st);
    }
    spr_knight_dead(SP_KNIGHT_D);
    for (int st = 0; st < 3; st++) {                                /* the Ill-Fated Knight's own */
        int base = SP_BKNIGHT_A + st * 4;
        spr_bknight(base, 1.0f, 0, st); spr_bknight(base + 1, -1.0f, 1, st); spr_bknight(base + 2, 0.0f, 2, st); spr_bknight(base + 3, 0.0f, 3, st);
        spr_bknight(SP_BKNIGHT_P0 + st, 0.0f, 4, st);
    }
    spr_deacon(SP_DEACON_A, 1.0f, 0); spr_deacon(SP_DEACON_B, -1.0f, 1); spr_deacon(SP_DEACON_CHANT, 0.0f, 2); spr_deacon(SP_DEACON_D, 0.0f, 3);
    deacon_dress(SP_DEACON_A); deacon_dress(SP_DEACON_B); deacon_dress(SP_DEACON_CHANT);
    g_back = 1; spr_deacon(SP_DEACON_BKA, 1.0f, 0); spr_deacon(SP_DEACON_BKB, -1.0f, 1); g_back = 0;   /* 1.0.0b18: his back */
    deacon_dress(SP_DEACON_BKA); deacon_dress(SP_DEACON_BKB);
    spr_bdeacon(SP_BDEACON_A, 1.0f, 0); spr_bdeacon(SP_BDEACON_B, -1.0f, 1); spr_bdeacon(SP_BDEACON_CHANT, 0.0f, 2); spr_bdeacon(SP_BDEACON_D, 0.0f, 3);   /* the Black Deacon's own */
    spr_whipper(SP_WHIP_A, 1.0f, 0); spr_whipper(SP_WHIP_B, -1.0f, 1); spr_whipper(SP_WHIP_HORN, 0.0f, 2); spr_whipper(SP_WHIP_CRACK, 0.0f, 3); spr_whipper(SP_WHIP_D, 0.0f, 4);
    g_back = 1; spr_whipper(SP_WHIP_BKA, 1.0f, 0); spr_whipper(SP_WHIP_BKB, -1.0f, 1); g_back = 0;   /* 1.0.0b18: his back */
    whipper_back_dress(SP_WHIP_BKA, -1.0f); whipper_back_dress(SP_WHIP_BKB, 1.0f);
    whipper_dress(SP_WHIP_A, 1.0f, 0); whipper_dress(SP_WHIP_B, -1.0f, 1); whipper_dress(SP_WHIP_HORN, 0.0f, 2); whipper_dress(SP_WHIP_CRACK, 0.0f, 3);
    for (int f = 0; f < 6; f++) { spr_sexton(SP_SEXTON_A + f, f == 0 ? 1.0f : f == 1 ? -1.0f : f == 4 ? 1.0f : 0.0f, f); if (f < 5) sexton_dress(SP_SEXTON_A + f, f == 1 ? -1.0f : 1.0f); }
    g_back = 1;                                                     /* 1.0.0b18: his back, and its dressing without the buttons */
    spr_sexton(SP_SEXTON_BKA, 1.0f, 0); spr_sexton(SP_SEXTON_BKB, -1.0f, 1);
    sexton_dress(SP_SEXTON_BKA, -1.0f); sexton_dress(SP_SEXTON_BKB, 1.0f);
    g_back = 0;
    for (int f = 0; f < 5; f++) { spr_lightless(SP_LIGHT_A + f, f == 0 ? 1.0f : f == 1 ? -1.0f : 0.0f, f); if (f < 4) lightless_dress(SP_LIGHT_A + f, f); }
}

/* The hunting spider's face (docs/InitialArtPass.md, section 8), a wolf spider's: two great eyes shining in front, two
 * more above them, a row of four small ones below, all fire; fur in R_WOOD (hex) */
static const char *const HUNTER_HEAD[10] = {
    "....4567654....", "..356J898J653..", ".3567aaaaa7653.", "3560FNF7FNF0653", "3570N@N8N@N0753",
    "3460FMF7FMF0643", ".3567898987653.", ".35H6H787H6H53.", "..34567876543..", "...345565543...",
};

