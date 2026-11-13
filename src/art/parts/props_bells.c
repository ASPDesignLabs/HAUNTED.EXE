/* A part of crawl_art.c (docs/PLAN-tidy.md): it is #included there in its place and never compiled on its own.
 * The bell puzzles' four bells, run II's dead bell, the soul-lamp. */

/* ---- 1.0.0b18.1 (docs/PLAN-b18-1.md, sections 5 and 6): the bell puzzles' bells, run II's dead bell, the soul-lamp ---- */

/* what is drawn, turned by a about (px, py): a bell swung from its beam, the dead bell hung askew */
static void cv_swing(float px, float py, float a) {
    static uint8_t tmp[TS * TS];
    memcpy(tmp, cv_sprite, sizeof tmp);
    memset(cv_sprite, C_CLEAR, sizeof tmp);
    float c = cosf(a), s = sinf(a);
    for (int y = 0; y < TS; y++)
        for (int x = 0; x < TS; x++) {
            float dx = (float)x + 0.5f - px, dy = (float)y + 0.5f - py;
            int sx = (int)floorf(px + c * dx + s * dy), sy = (int)floorf(py - s * dx + c * dy);   /* back to where it came from */
            if ((unsigned)sx < TS && (unsigned)sy < TS) cv_sprite[y * TS + x] = tmp[sy * TS + sx];
        }
}

/* a numeral cut in the metal, centred on (cx, cy), h tall: each stroke two pixels wide in the stroke's shade, the pixel under
 * it in the edge's (cut: dark, the lower edge catching the light; on black iron, inlaid pale) */
static void cut_numeral(float cx, float cy, int n, int h, int ramp, float stroke, float edge) {
    static const char *const NUM[5] = { "", "I", "II", "III", "IV" };
    const char *s = NUM[clampi(n, 1, 4)];
    int vw = (h * 3 / 4) | 1, w = 0;
    for (const char *p = s; *p; p++) w += (*p == 'I' ? 4 : vw) + (p[1] ? 1 : 0);
    static uint8_t m[12][24];
    memset(m, 0, sizeof m);
    h = clampi(h, 3, 12);
    int ox = 0;
    for (const char *p = s; *p; p++) {
        if (*p == 'I') {                                           /* an I, with its serifs */
            for (int y = 0; y < h; y++) m[y][ox + 1] = m[y][ox + 2] = 1;
            for (int x = 0; x < 4; x++) m[0][ox + x] = m[h - 1][ox + x] = 1;
            ox += 5; continue;
        }
        for (int y = 0; y < h; y++) {                              /* a V: two strokes meeting at its foot */
            int a = (int)((float)y / (float)(h - 1) * (float)(vw - 1) * 0.5f + 0.5f), b = vw - 1 - a;
            m[y][ox + a] = m[y][ox + b] = 1;
            if (a + 1 < b) { m[y][ox + a + 1] = 1; m[y][ox + b - 1] = 1; }
        }
        m[0][ox] = m[0][ox + 1] = m[0][ox + 2] = m[0][ox + vw - 3] = m[0][ox + vw - 2] = m[0][ox + vw - 1] = 1;   /* its serifs */
        ox += vw + 1;
    }
    int x0 = (int)(cx - (float)w * 0.5f + 0.5f), y0 = (int)(cy - (float)h * 0.5f + 0.5f);
    for (int y = 0; y <= h; y++)
        for (int x = 0; x < w && x < 24; x++) {
            if (y < h && m[y][x]) cv_px(x0 + x, y0 + y, sh(ramp, stroke));
            else if (y > 0 && m[y - 1][x]) cv_px(x0 + x, y0 + y, sh(ramp, edge));
        }
}

typedef struct {
    float top, bot;                          /* the shoulder's top and the lip, in pixels */
    float sh, lip, pw, waist;                /* the radius at the shoulder and at the lip, the flare's power, how far the waist draws in */
    int ramp; float dim;                     /* the metal, and how dark it is kept (black iron) */
    int bands, verd, thick;                  /* bands cast round it; verdigris (bronze); a thick rounded lip (the great bell) */
} BellForm;
static float bell_r(const BellForm *B, float t) {
    t = clampf(t, 0.0f, 1.0f);
    float r = B->sh + (B->lip - B->sh) * powf(t, B->pw) - B->waist * sinf(PI_F * t) * (1.0f - 0.4f * t);
    if (t < 0.14f) { float q = (0.14f - t) / 0.14f; r *= sqrtf(1.0f - q * q * 0.75f); }   /* the shoulder, rounded */
    return r;
}
static void bell_body(const BellForm *B, float cx, int numeral) {
    const float top = B->top, bot = B->bot, H = bot - top;
    float band[4] = { 0 };
    for (int k = 0; k < B->bands && k < 4; k++) band[k] = top + H * (B->bands == 1 ? 0.7f : 0.5f + 0.4f * (float)k / (float)(B->bands - 1));
    for (int y = (int)top; y <= (int)bot + 4; y++) {
        float t = ((float)y - top) / H, r = bell_r(B, t), dr = (bell_r(B, t + 0.03f) - bell_r(B, t - 0.03f)) / (0.06f * H);
        for (int x = (int)(cx - r - 1); x <= (int)(cx + r + 1); x++) {
            float nx = ((float)x + 0.5f - cx) / r;
            if (fabsf(nx) > 1.0f) continue;
            float nz = sqrtf(1.0f - nx * nx), lipy = bot + (B->thick ? 3.2f : 2.2f) * nz;
            if ((float)y > lipy) continue;
            float ny = -dr * 0.9f, nl = sqrtf(nx * nx + ny * ny + nz * nz), ux = nx / nl, uy = ny / nl, uz = nz / nl;
            float spec = powf(fmaxf(0.0f, ux * -0.25f + uy * -0.37f + uz * 0.89f), 18.0f);
            float v = 0.1f + 0.42f * lit(ux, uy, uz) + 0.6f * spec + (nx > 0.82f ? 0.1f : 0.0f);
            if (fabsf(nx) < 0.94f)
                for (int k = 0; k < B->bands && k < 4; k++) {          /* a band: a raised line, its shadow under it */
                    int d = (int)floorf((float)y - (band[k] - 1.0f * nz));
                    if (d == 0) v += 0.32f; else if (d == 1) v -= 0.26f; else if (d == -1) v += 0.1f;
                }
            if ((float)y > lipy - (B->thick ? 3.0f : 1.0f)) v = 0.2f + 0.34f * (1.0f - fabsf(nx)) + (B->thick ? 0.16f * nz : 0.0f);   /* the lip */
            v = B->dim < 0.0f ? 0.3f + v * 0.85f : v * (1.0f - B->dim);   /* silver kept bright, black iron dark */
            int ramp = B->ramp;
            if (B->verd && fbm((float)x / 2.5f, (float)y / 6.0f, 32, 471) + (t < 0.12f ? 0.2f : 0.0f) > 0.8f) { ramp = R_MOSS; v = 0.22f + 0.3f * v; }
            if (v > 0.9f && ramp == R_WOOD) { cv_px(x, y, sh(R_BONE, v)); continue; }
            cv_px(x, y, sh(ramp, v));
        }
    }
    if (numeral) {                                                 /* between the shoulder and the first band */
        float n0 = B->bands ? band[0] : top + H * 0.6f;
        int h = clampi((int)((n0 - top) * 0.55f), 5, 9);
        float cy = top + (n0 - top) * 0.56f + 0.5f;
        if (B->dim > 0.3f) cut_numeral(32.0f, cy, numeral, h, R_IRON, 0.82f, 0.08f);   /* black iron: inlaid pale */
        else cut_numeral(32.0f, cy, numeral, h, B->ramp, 0.03f, B->dim < 0.0f ? 0.98f : 0.7f);
    }
}

/* The four: I silver, a narrow handbell on a wooden yoke; II bronze, tall and waisted, verdigris in its folds; III iron, squat
 * and wide-mouthed; IV the great bell in black iron, crowned with three loops, thick at the lip. One to four bands, the
 * numeral in the waist. swung: leaning from the beam, the clapper lagging, the ring going out from its lip. */
static void spr_pbell(int id, int k, int swung) {
    static const BellForm F[4] = {
        { 17, 41, 4.2f, 9.2f, 4.0f, 0.3f, R_IRON, -0.35f, 1, 0, 0 },
        { 10, 45, 6.0f, 14.0f, 3.2f, 1.3f, R_WOOD, 0.0f, 2, 1, 0 },
        { 14, 38, 7.0f, 20.0f, 1.8f, 0.0f, R_IRON, 0.0f, 3, 0, 0 },
        { 12, 46, 8.5f, 18.0f, 3.0f, 0.6f, R_IRON, 0.45f, 4, 0, 1 },
    };
    const BellForm *B = &F[k];
    static const int BX[2] = { 22, 40 };
    const float cx = 32.0f, pa = swung ? 0.27f : 0.0f;
    cv_begin(cv_sprite, TS, TS);
    if (k == 0) {                                                  /* the yoke: a headstock of oak, hung on two iron hangers */
        for (int i = 0; i < 2; i++) cv_capsule(25.0f + 14.0f * (float)i, 4.5f, 25.0f + 14.0f * (float)i, 10.5f, 0.9f, R_IRON, 0.15f, 0.8f);
        for (int y = 10; y <= 14; y++)
            for (int x = 21; x <= 43; x++) {
                float v = y == 10 ? 0.55f : 0.38f - 0.05f * (float)(y - 10) + (fbm((float)x / 6.0f, (float)y, 16, 473) - 0.5f) * 0.25f;
                if (x <= 22 || x >= 42) v -= 0.1f;
                cv_px(x, y, sh(R_WOOD, v));
            }
        for (int i = 0; i < 2; i++) cv_capsule(29.0f + 6.0f * (float)i, 13.0f, 30.0f + 4.0f * (float)i, 18.0f, 0.8f, R_IRON, 0.12f, 0.75f);   /* its straps */
    } else {                                                       /* the stirrup's straps, and the crown */
        float ct = B->top - (k == 3 ? 5.0f : 0.5f);
        for (int i = 0; i < 2; i++) {
            float x = cx - 4.5f + 9.0f * (float)i;
            cv_capsule(x, 5.0f, x + (i ? -1.0f : 1.0f), ct + 2.0f, 1.0f, R_IRON, 0.15f, 0.78f);
        }
        if (k == 3) for (int i = -1; i <= 1; i++) iron_ring(cx + 4.5f * (float)i, B->top - 2.5f, 2.4f, 2.6f, 1.6f, PI_F, TAU_F, 0.12f);   /* three canons */
        else iron_ring(cx, B->top - 0.5f, 3.0f, 2.6f, 1.6f, PI_F, TAU_F, 0.06f);
    }
    bell_body(B, cx, k + 1);
    if (!swung) {                                                  /* the clapper, hanging still */
        float cr = k == 3 ? 3.0f : k == 2 ? 2.6f : 2.1f;
        cv_capsule(cx + 0.5f, B->bot - 1.0f, cx + 0.5f, B->bot + 3.0f, 0.8f, R_IRON, 0.1f, 0.5f);
        cv_ellipse(cx + 0.5f, B->bot + 3.5f + cr, cr, cr * 0.9f, R_IRON, 0.08f, 0.62f);
    }
    cv_swing(cx, 5.0f, pa);
    if (swung) {                                                   /* the bell's mouth swings left; the clapper lags, and strikes its right lip */
        float cr = k == 3 ? 3.0f : k == 2 ? 2.6f : 2.1f, a2 = pa * 0.25f, s2 = sinf(a2), c2 = cosf(a2), sp = sinf(pa), cp = cosf(pa);
        float d0 = B->bot - 7.0f, d1 = B->bot - 2.0f, d2 = B->bot - 1.5f + cr;
        cv_capsule(cx - s2 * d0, 5.0f + c2 * d0, cx - s2 * d1, 5.0f + c2 * d1, 0.8f, R_IRON, 0.1f, 0.5f);
        cv_ellipse(cx - s2 * d2, 5.0f + c2 * d2, cr, cr * 0.9f, R_IRON, 0.08f, 0.62f);
        float dl = B->bot - 5.0f, lx = cx - sp * dl + B->lip * cp + 1.0f, ly = 5.0f + cp * dl + B->lip * sp;
        for (int r = 0; r < 3; r++)                                /* the ring going out from where it struck */
            for (int s = -3; s <= 3; s++) {
                float a = (float)s * 0.18f, rr = 3.0f + 3.0f * (float)r;
                cv_px((int)(lx + cosf(a) * rr), (int)(ly + sinf(a) * rr), sh(R_GHOST, 0.95f - 0.22f * (float)r));
            }
    }
    timber_beam(4, 60, 0, 4, BX, 2, 461 + k);
    cv_outline(0);
    cv_store(id);
}

/* run II's hanging bell, plainly dead, so that it can't be taken for one that rings: bronze gone green, cracked through from
 * the lip to the shoulder, a piece of the lip gone, no clapper, hung askew by its one strap left from a beam broken through */
static void spr_bell_dead(void) {
    static const BellForm B = { 12, 42, 6.0f, 14.0f, 3.2f, 0.9f, R_WOOD, 0.12f, 1, 1, 0 };
    static const int BX[1] = { 18 };
    cv_begin(cv_sprite, TS, TS);
    cv_capsule(27.5f, 5.0f, 28.5f, 13.5f, 1.0f, R_IRON, 0.12f, 0.7f);   /* the one strap left */
    cv_capsule(36.5f, 9.0f, 36.0f, 13.0f, 1.0f, R_IRON, 0.08f, 0.45f);   /* the other's stump */
    iron_ring(32.0f, 11.5f, 3.0f, 2.6f, 1.6f, PI_F, TAU_F, 0.2f);
    bell_body(&B, 32.0f, 0);
    for (int y = 12; y <= 46; y++)                                 /* the green run down it, deeper than the living bell's */
        for (int x = 14; x <= 50; x++) {
            uint8_t c = cv_sprite[y * TS + x];
            if (c >= R_WOOD && c < R_WOOD + 16 && fbm((float)x / 3.0f, (float)y / 8.0f, 32, 477) > 0.62f) cv_px(x, y, sh(R_MOSS, 0.12f + 0.02f * (float)(c - R_WOOD)));
        }
    for (int k = 0; k < 30; k++) {                                 /* the crack, through from the lip to the shoulder */
        int y = 44 - k, x = 35 - (int)(h01(k / 3, 1, 479) * 3.0f) + k / 9;
        cv_px(x, y, sh(R_WOOD, 0.0f)); cv_px(x - 1, y, sh(R_WOOD, 0.02f)); cv_px(x + 1, y, sh(R_MOSS, 0.5f));
    }
    for (int x = 37; x <= 47; x++) {                               /* a piece of the lip gone: a wedge, ragged */
        int depth = 7 - (int)(fabsf((float)x - 42.0f) * 1.3f) + (int)(h01(x, 2, 481) * 2.0f);
        for (int y = 44 - depth; y <= 47; y++) cv_px(x, y, C_CLEAR);
    }
    cv_swing(28.0f, 5.0f, -0.33f);                                 /* askew, from the strap left */
    timber_beam(4, 33, 0, 4, BX, 1, 483);                          /* the beam, broken through: its far end fallen a little */
    for (int y = 3; y <= 8; y++)
        for (int x = 36; x <= 60; x++) {
            float v = y == 3 + (x - 36) / 12 ? 0.48f : 0.3f - 0.04f * (float)(y - 3) + (fbm((float)x / 9.0f, (float)y * 1.4f, 16, 485) - 0.5f) * 0.3f;
            if (y < 3 + (x - 36) / 12 || y > 7 + (x - 36) / 12) continue;
            cv_px(x, y, sh(R_WOOD, v));
        }
    for (int y = 0; y <= 8; y++) {                                 /* the splintered ends */
        if (h01(y, 3, 487) > 0.4f) cv_px(34, y, sh(R_WOOD, 0.5f));
        if (y >= 3 && h01(y, 4, 489) > 0.35f) cv_px(35, y, sh(R_WOOD, 0.4f));
    }
    for (int k = 0; k < 3; k++)                                    /* cobwebs from the beam to the shoulder, and across */
        for (int s = 0; s <= 14; s++) {
            float t = (float)s / 14.0f, x = (8.0f + 6.0f * (float)k) + (22.0f + 4.0f * (float)k - (8.0f + 6.0f * (float)k)) * t, y = 4.0f + (14.0f + 2.0f * (float)k) * t + sinf(t * PI_F) * 2.0f;
            cv_px((int)x, (int)y, sh(R_STONE, 0.72f - 0.12f * (float)((s + k) % 3 == 0)));
        }
    cv_outline(0);
    cv_store(SP_BELL_DEAD);
}

/* The soul-lamp, by each challenge: E offers it a soul, and while it burns the Wraith can't come into the challenge's ground.
 * Hung on the wall, a vessel for the soul's flame (pale violet, full bright) and five pips round its foot, one for each soul
 * banked; drawn in each run's look. state: 0 out, 1 and 2 lit (the flame's two frames); lit, every pip shows lit. */
static void lamp_flame(float cx, float cy, float h, int frame) {
    for (int y = (int)(cy - h); y <= (int)cy + 1; y++) {
        float t = (cy - (float)y) / h, w = (1.0f - t) * (1.0f - t) * 3.4f + 0.4f;   /* a teardrop, leaning by the frame */
        float sx = cx + (frame == 1 ? 0.9f : -0.9f) * t * t * 2.0f;
        for (int x = (int)(sx - w - 1); x <= (int)(sx + w + 1); x++) {
            float d = fabsf((float)x + 0.5f - sx) / w;
            if (d > 1.0f) continue;
            cv_px(x, y, (uint8_t)(R_GLOW + clampi((int)(15.0f - 9.0f * d - 4.0f * t), 4, 15)));
        }
    }
}
static void lamp_pips(float cx, float y, int on, int ramp) {
    for (int i = 0; i < 5; i++) {
        int x = (int)(cx - 8.0f + 4.0f * (float)i);
        if (on) { cv_px(x, y, (uint8_t)(R_GLOW + 15)); cv_px(x + 1, y, (uint8_t)(R_GLOW + 12)); cv_px(x, y + 1, (uint8_t)(R_GLOW + 11)); cv_px(x + 1, y + 1, (uint8_t)(R_GLOW + 9)); }
        else { cv_px(x, y, sh(ramp, 0.04f)); cv_px(x + 1, y, sh(ramp, 0.08f)); cv_px(x, y + 1, sh(ramp, 0.1f)); cv_px(x + 1, y + 1, sh(ramp, 0.2f)); }
    }
}
static void spr_wlamp(int id, int run, int state) {
    const float cx = 32.0f;
    int on = state > 0;
    cv_begin(cv_sprite, TS, TS);
    switch (run) {
    case 0: {                                                      /* the Order's: an iron cage on a plate pierced with its cross */
        for (int y = 8; y <= 30; y++)
            for (int x = 25; x <= 39; x++) {
                float v = 0.34f + (fbm((float)x / 4.0f, (float)y / 4.0f, 16, 491) - 0.5f) * 0.2f - (x == 25 || x == 39 || y == 8 || y == 30 ? 0.12f : 0.0f);
                int cross = (abs(x - 32) <= 0 && y >= 12 && y <= 22) || (abs(y - 15) <= 0 && x >= 29 && x <= 35);
                cv_px(x, y, cross ? sh(R_IRON, 0.02f) : sh(R_IRON, v));
            }
        cv_capsule(cx, 30.0f, cx, 34.0f, 1.2f, R_IRON, 0.1f, 0.6f);  /* the bracket */
        for (int y = 34; y <= 50; y++) {                           /* the cage: four bars, a cap and a foot */
            for (int i = 0; i < 4; i++) cv_px((int)(cx - 6.0f + 4.0f * (float)i), y, sh(R_IRON, 0.25f + 0.2f * (float)(i == 1)));
            if (y == 34 || y == 35 || y >= 49) for (int x = (int)cx - 7; x <= (int)cx + 7; x++) cv_px(x, y, sh(R_IRON, y == 34 ? 0.55f : 0.3f));
        }
        if (on) lamp_flame(cx, 45.0f, 9.0f, state);
        for (int i = 0; i < 4; i++) cv_px((int)(cx - 6.0f + 4.0f * (float)i), 40, sh(R_IRON, 0.3f));   /* the bars over the flame */
        lamp_pips(cx, 51.0f, on, R_IRON);
        for (int x = (int)cx - 9; x <= (int)cx + 9; x++) { cv_px(x, 53, sh(R_IRON, 0.22f)); cv_px(x, 54, sh(R_IRON, 0.1f)); }
        break;
    }
    case 1: {                                                      /* the drowned catacombs': a bronze bowl on three chains from a bracket */
        for (int x = 18; x <= 40; x++) for (int y = 9; y <= 11; y++) cv_px(x, y, sh(R_WOOD, y == 9 ? 0.5f : 0.3f));
        cv_capsule(19.0f, 7.0f, 19.0f, 15.0f, 1.3f, R_WOOD, 0.15f, 0.6f);
        for (int i = 0; i < 3; i++)                                /* the chains */
            for (int y = 12; y <= 36; y += 2) cv_px((int)(cx - 9.0f + 9.0f * (float)i + (float)(y - 12) / 24.0f * (float)(i - 1) * -3.0f), y, sh(R_IRON, (y / 2) % 2 ? 0.5f : 0.25f));
        cv_px((int)cx, 11, sh(R_IRON, 0.7f));
        for (int y = 36; y <= 46; y++) {                           /* the bowl */
            float t = (float)(y - 36) / 10.0f, r = 11.0f - 7.0f * t * t;
            for (int x = (int)(cx - r); x <= (int)(cx + r); x++) {
                float nx = ((float)x + 0.5f - cx) / r, v = 0.18f + 0.4f * (1.0f - fabsf(nx)) + (y == 36 ? 0.3f : 0.0f);
                int ramp = fbm((float)x / 2.5f, (float)y / 4.0f, 32, 493) > 0.7f ? R_MOSS : R_WOOD;
                cv_px(x, y, sh(ramp, v));
            }
        }
        if (on) lamp_flame(cx, 36.0f, 10.0f, state);
        lamp_pips(cx, 40.0f, on, R_WOOD);
        for (int k = 0; k < 3; k++) cv_px((int)cx - 3 + 3 * k, 48 + k + (state == 2 ? 1 : 0), sh(R_GHOST, 0.45f));   /* water dripping from it */
        break;
    }
    case 2: {                                                      /* the charnel mine's: a miner's lantern on a hook from a timber bracket */
        for (int y = 6; y <= 12; y++) for (int x = 16; x <= 44; x++) cv_px(x, y, sh(R_WOOD, (y == 6 ? 0.48f : 0.32f) + (fbm((float)x / 7.0f, (float)y, 16, 495) - 0.5f) * 0.3f));
        for (int y = 12; y <= 26; y++) cv_px(18 + (y - 12) / 3, y, sh(R_WOOD, 0.3f)), cv_px(19 + (y - 12) / 3, y, sh(R_WOOD, 0.22f));   /* the brace */
        iron_ring(cx, 16.0f, 2.2f, 3.0f, 1.3f, 0.0f, TAU_F, 0.1f); /* the hook */
        for (int y = 20; y <= 48; y++) {                           /* a tin frame, horn panes glowing through */
            float t = (float)(y - 20) / 28.0f, r = y < 24 ? 4.0f + (float)(y - 20) : 8.0f;
            for (int x = (int)(cx - r); x <= (int)(cx + r); x++) {
                int edge = x <= (int)(cx - r) + 1 || x >= (int)(cx + r) - 1 || y < 25 || y > 45;
                if (edge) cv_px(x, y, sh(R_IRON, 0.2f + 0.3f * (1.0f - t)));
                else cv_px(x, y, sh(R_WARM, on ? 0.55f + 0.3f * (1.0f - fabsf(((float)x + 0.5f - cx) / r)) : 0.2f));
            }
        }
        if (on) lamp_flame(cx, 43.0f, 11.0f, state);
        lamp_pips(cx, 46.0f, on, R_IRON);
        break;
    }
    default: {                                                     /* the black glass's: an obsidian vessel, the flame seen through its wall */
        cv_capsule(cx, 6.0f, cx, 20.0f, 1.3f, R_STONE, 0.05f, 0.3f);
        for (int y = 20; y <= 50; y++) {
            float t = (float)(y - 20) / 30.0f, r = 3.0f + 9.0f * sinf(PI_F * fminf(1.0f, t * 1.1f));
            for (int x = (int)(cx - r); x <= (int)(cx + r); x++) {
                float nx = ((float)x + 0.5f - cx) / (r + 0.01f), v = 0.06f + 0.18f * (1.0f - fabsf(nx)) + (fabsf(nx + 0.45f) < 0.08f ? 0.4f : 0.0f);
                uint8_t c = sh(R_STONE, v);
                if (h01(x, y, 497) > 0.94f) c = sh(R_PURPLE, 0.6f);    /* glints in the glass */
                if (on && fabsf(nx) < 0.55f && t > 0.25f && t < 0.85f) c = (uint8_t)(R_GLOW + clampi((int)(9.0f - 6.0f * fabsf(nx)), 3, 9));   /* the flame, through it */
                cv_px(x, y, c);
            }
        }
        if (on) lamp_flame(cx, 40.0f, 9.0f, state);
        lamp_pips(cx, 47.0f, on, R_STONE);
        break;
    }
    }
    cv_outline(0);
    cv_store(id);
}

