/* The new hanging lights and the events' things, as sheets for the owner (docs/PLAN-lights.md, step 2; docs/PLAN-events.md). The owner
 * approved both; the drawings are the art's now (src/art/parts/lights_events.c: SP_CORONA0 ... SP_EV_LEVER1, T_PLATE ... T_UNBARRED,
 * made each level by crawl_art_lights), and this draws the sheets from the game's own slots, byte for byte as they were approved. The
 * art is #included whole, as before, for its palette and slots.
 *
 *   build/lightmock lights media/sheets/lights/hanging_lights.png
 *   build/lightmock things media/sheets/events/event_things.png
 *   build/lightmock relics media/sheets/events/relics.png
 *
 * The twelve relics' looks (the last sheet, not yet approved) are drawn here, in the art's style, to move into src/art/parts/ once
 * the owner has chosen them.
 */
#include "crawl_art.c"
#include <stdio.h>
#include <zlib.h>

/* an event's plate, the art's overlay laid over a floor (the cell's own, in the game) */
static void plate_on(uint8_t *t, const uint8_t *floor, int pressed) {
    const uint8_t *o = crawl_tex[pressed ? T_PLATE_DOWN : T_PLATE];
    for (int i = 0; i < TS * TS; i++) t[i] = o[i] != C_CLEAR ? o[i] : floor[i];
}

/* what the game draws today for a plate (draw/floor.c, FM_PLATE): the floor, its edge cut deep; sunk, darker */
static void today_plate(uint8_t *t, const uint8_t *floor, int pressed) {
    for (int x = 0; x < TS; x++)
        for (int y = 0; y < TS; y++) {
            float fx = ((float)x + 0.5f) / TS, fy = ((float)y + 0.5f) / TS, e = fminf(fminf(fx, 1.0f - fx), fminf(fy, 1.0f - fy));
            uint8_t c = floor[x * TS + y];
            if (e < 0.06f) c = R_STONE + 1;
            else if (e < 0.1f) c = (uint8_t)(R_STONE + (pressed ? 4 : 10));
            else if (pressed) { int ramp = (c - 1) / 16 * 16 + 1; c = (uint8_t)(ramp + clampi((int)((float)(c - ramp) * 0.65f), 1, 15)); }
            t[x * TS + y] = c;
        }
}

/* ---- the twelve relics (docs/PLAN-events.md): a look for each, to be chosen per relic in the editor. Each is drawn with its foot
 * at (cx, by), lit from the top left as everything is, and outlined by its caller; RELIC_CUT is the shape each cuts in the socket's
 * panel. The first is the art's own (relic_bone). As they would stand in src/art/parts/ ---- */
enum { RL_KEY, RL_FINGER, RL_HEART, RL_CROWN, RL_ROSARY, RL_AMPULLA, RL_NAIL, RL_HILT, RL_SEAL, RL_PSALTER, RL_MASK, RL_EYE, NRELIC };

/* a step of brass or bronze (R_GOLD dulled), of silver (R_IRON, brightened), of lead (R_IRON, dull and blue-grey) */
static uint8_t bronze(float v) { return sh(R_GOLD, clampf(v, 0.0f, 1.0f) * 0.62f); }
static uint8_t silver(float v) { return v > 0.88f ? sh(R_GHOST, 0.85f) : sh(R_IRON, 0.25f + 0.75f * clampf(v, 0.0f, 1.0f)); }
static float rlit(float nx, float ny) { float d = 1.0f - nx * nx - ny * ny; return lit(nx, ny, sqrtf(fmaxf(0.0f, d))); }

/* 2: a saint's finger in a gilt crystal tube: a lobed gilt foot, a knop, a crystal cylinder between beaded collars, the finger
 * inside it in three joints with a ring on it, a gilt spire with a cross on top */
static void relic_finger(float cx, float by) {
    int b = (int)by;
    for (int y = b - 3; y <= b; y++) {                               /* the foot, lobed */
        float hw = 3.0f + (float)(y - (b - 3)) * 1.4f;
        for (int x = (int)(cx - hw); x <= (int)(cx + hw); x++) {
            float nx = ((float)x + 0.5f - cx) / (hw + 0.5f), v = 0.35f + 0.55f * rlit(nx, -0.4f);
            if (y == b - 3) v += 0.15f;
            if (y == b && ((x + 1) % 4 == 0)) v -= 0.3f;           /* its lobes */
            cv_px(x, y, sh(R_GOLD, v));
        }
    }
    cv_capsule(cx, (float)b - 8, cx, (float)b - 4, 0.9f, R_GOLD, 0.3f, 0.85f);
    cv_ellipse(cx, (float)b - 6, 2.3f, 1.4f, R_GOLD, 0.25f, 0.95f);  /* the knop */
    for (int x = (int)cx - 4; x <= (int)cx + 4; x++) {               /* the lower collar, beaded */
        cv_px(x, b - 9, sh(R_GOLD, (x & 1) ? 0.9f : 0.45f)); cv_px(x, b - 10, sh(R_GOLD, x < (int)cx ? 0.7f : 0.4f));
    }
    for (int y = b - 22; y <= b - 11; y++)                           /* the crystal: pale, its edges bright, a gleam down it */
        for (int x = (int)cx - 3; x <= (int)cx + 3; x++) {
            int e = x == (int)cx - 3 || x == (int)cx + 3;
            float v = e ? 0.62f : 0.28f + 0.06f * (float)((x + y) & 1);
            if (x == (int)cx - 2) v = 0.55f;
            cv_px(x, y, sh(R_GHOST, v - 0.18f * (x == (int)cx + 3)));
        }
    for (int y = b - 21; y <= b - 12; y++) {                         /* the finger, in three joints, its nail at the top */
        int j = y == b - 18 || y == b - 15;
        cv_px((int)cx, y, sh(R_BONE, j ? 0.35f : 0.82f - 0.02f * (float)(y - (b - 21)))); cv_px((int)cx + 1, y, sh(R_BONE, j ? 0.2f : 0.5f));
        if (!j && y > b - 20) cv_px((int)cx - 1, y, sh(R_BONE, 0.65f));
    }
    cv_px((int)cx, b - 21, sh(R_BONE, 0.95f));
    cv_px((int)cx - 1, b - 14, sh(R_GOLD, 0.95f)); cv_px((int)cx, b - 14, sh(R_GOLD, 0.7f)); cv_px((int)cx + 1, b - 14, sh(R_GOLD, 0.35f));   /* its ring */
    cv_px((int)cx, b - 13, sh(R_BLOOD, 0.7f));                       /* the ring's stone */
    cv_px((int)cx - 2, b - 20, C_WHITE);                             /* the crystal's glint */
    for (int x = (int)cx - 4; x <= (int)cx + 4; x++) { cv_px(x, b - 23, sh(R_GOLD, x < (int)cx ? 0.8f : 0.45f)); cv_px(x, b - 24, sh(R_GOLD, (x & 1) ? 0.85f : 0.4f)); }   /* the upper collar */
    for (int y = b - 29; y <= b - 25; y++) {                         /* the spire */
        float hw = (float)(y - (b - 29)) * 0.65f;
        for (int x = (int)(cx - hw); x <= (int)(cx + hw); x++) cv_px(x, y, sh(R_GOLD, x < (int)cx ? 0.85f : x == (int)cx ? 0.6f : 0.35f));
    }
    cv_px((int)cx, b - 31, sh(R_GOLD, 0.9f)); cv_px((int)cx, b - 30, sh(R_GOLD, 0.7f)); cv_px((int)cx - 1, b - 31, sh(R_GOLD, 0.75f)); cv_px((int)cx + 1, b - 31, sh(R_GOLD, 0.45f));   /* its cross */
    cv_px((int)cx, b - 32, sh(R_GOLD, 0.8f));
}

/* 3: a heart in a small silver casket: a box on four ball feet, quatrefoils chased on its front, its rim lit; the lid thrown back,
 * its silver edge bright, lined with dark velvet; in it a heart, dark red and dried, its vessels cut, a gilt band of thorns round it */
static void relic_heart(float cx, float by) {
    int b = (int)by, x0 = (int)cx - 9, x1 = (int)cx + 9;
    for (int y = b - 16; y <= b - 9; y++) {                          /* the lid, thrown back */
        float u = (float)(y - (b - 16)) / 7.0f;
        int lx0 = x0 + 2 - (int)(u * 1.5f), lx1 = x1 - 2 + (int)(u * 1.5f);
        for (int x = lx0; x <= lx1; x++) {
            int edge = y == b - 16 || y == b - 15 || x == lx0 || x == lx1;
            cv_px(x, y, edge ? silver((y == b - 16 ? 0.95f : 0.6f) - 0.4f * (float)(x - lx0) / (float)(lx1 - lx0)) : sh(R_PURPLE, 0.08f + 0.06f * u + 0.03f * (float)((x + y) & 1)));
        }
    }
    float hx = cx, hy = (float)b - 14;                               /* the heart, standing up out of the casket */
    cv_ellipse(hx - 2.6f, hy - 2.0f, 3.3f, 3.2f, R_BLOOD, 0.2f, 0.8f);
    cv_ellipse(hx + 2.4f, hy - 2.0f, 3.3f, 3.2f, R_BLOOD, 0.15f, 0.6f);
    for (int y = 0; y <= 6; y++) {
        float hw = 5.2f - (float)y * 0.8f;
        for (int x = (int)(hx - hw); x <= (int)(hx + hw); x++) cv_px(x, (int)hy + y, sh(R_BLOOD, 0.42f - 0.04f * (float)y + 0.12f * (x < (int)hx) - 0.1f * (x > (int)hx + 2)));
    }
    cv_px((int)hx - 1, (int)hy - 5, sh(R_BLOOD, 0.1f)); cv_px((int)hx - 1, (int)hy - 6, sh(R_BLOOD, 0.5f)); cv_px((int)hx - 1, (int)hy - 7, sh(R_BLOOD, 0.35f));   /* its vessels, cut */
    cv_px((int)hx + 2, (int)hy - 5, sh(R_BLOOD, 0.1f)); cv_px((int)hx + 2, (int)hy - 6, sh(R_BLOOD, 0.4f));
    cv_px((int)hx - 4, (int)hy - 3, sh(R_BLOOD, 0.95f)); cv_px((int)hx - 3, (int)hy - 4, sh(R_BLOOD, 0.85f));   /* a gleam */
    for (int x = (int)hx - 5; x <= (int)hx + 5; x++) {               /* the band of thorns */
        cv_px(x, (int)hy + 1, sh(R_GOLD, x < (int)hx ? 0.85f : 0.45f));
        if ((x & 1) == 0) cv_px(x, (int)hy, sh(R_GOLD, 0.65f));
        else cv_px(x, (int)hy + 2, sh(R_GOLD, 0.4f));
    }
    for (int y = b - 8; y <= b - 2; y++)                             /* the box */
        for (int x = x0; x <= x1; x++) {
            float u = (float)(x - x0) / (float)(x1 - x0), v = 0.62f - 0.35f * u;
            if (y == b - 8) v = 0.95f - 0.3f * u;                    /* its rim */
            else if (y == b - 7) v = 0.25f;
            else if (y == b - 2) v = 0.12f;
            if (x == x0) v += 0.2f; else if (x == x1) v -= 0.15f;
            if (h01(x, y, 901) < 0.1f) v -= 0.2f;                   /* tarnish */
            cv_px(x, y, silver(v));
        }
    for (int k = 0; k < 3; k++) {                                    /* quatrefoils chased on its front */
        int qx = x0 + 4 + k * 5, qy = b - 5;
        cv_px(qx, qy - 1, silver(0.1f)); cv_px(qx - 1, qy, silver(0.1f)); cv_px(qx + 1, qy, silver(0.1f)); cv_px(qx, qy + 1, silver(0.1f));
        cv_px(qx, qy, silver(0.8f)); cv_px(qx + 1, qy + 1, silver(0.7f));
    }
    for (int s = 0; s < 2; s++) { int fx = s ? x1 - 1 : x0 + 1; cv_px(fx, b - 1, silver(0.7f)); cv_px(fx + 1, b - 1, silver(0.3f)); cv_px(fx, b, silver(0.35f)); cv_px(fx + 1, b, silver(0.15f)); }   /* its feet */
}

/* 4: a crowned skull: the master skull, a gold crown on its dome, five points with balls on them, stones set in its band */
static void relic_crown(float cx, float by) {
    skull_stamp(cx, by - 8.0f, 0, 0, 0);
    int t = (int)by - 16, x0 = (int)cx - 7, x1 = (int)cx + 7;
    for (int y = t; y <= t + 3; y++)                                 /* the band */
        for (int x = x0; x <= x1; x++) {
            float u = (float)(x - x0) / (float)(x1 - x0), v = 0.9f - 0.55f * u;
            if (y == t) v += 0.1f; else if (y == t + 3) v -= 0.3f;
            cv_px(x, y, sh(R_GOLD, v));
        }
    for (int k = 0; k < 5; k++) {                                    /* its points */
        int px = x0 + 1 + k * 3, h = k == 2 ? 5 : (k == 1 || k == 3) ? 4 : 3;
        for (int j = 1; j <= h; j++) { cv_px(px, t - j, sh(R_GOLD, 0.85f - 0.1f * (float)k - 0.03f * (float)j)); if (j < h - 1) cv_px(px + 1, t - j, sh(R_GOLD, 0.35f)); }
        cv_px(px, t - h - 1, sh(R_GOLD, 0.95f - 0.1f * (float)k));   /* a ball on it */
    }
    static const int GEM[3] = { 2, 7, 12 };
    for (int k = 0; k < 3; k++) { cv_px(x0 + GEM[k], t + 1, k == 1 ? sh(R_BLOOD, 0.85f) : sh(R_PURPLE, 0.75f)); cv_px(x0 + GEM[k], t + 2, k == 1 ? sh(R_BLOOD, 0.4f) : sh(R_PURPLE, 0.35f)); }
}

/* 5: a rosary of bone beads lying in a loop, a larger bead at each decade, a tail of beads to a little bone cross */
static void bead(float x, float y, float r, float dim) {
    cv_ellipse(x, y, r, r * 0.9f, R_BONE, fmaxf(0.05f, 0.2f - dim), 0.95f - dim);
}
static void relic_rosary(float cx, float by) {
    float lx = cx - 6.0f, ly = by - 7.0f, rx = 11.0f, ry = 4.6f;
    for (int pass = 0; pass < 2; pass++)                             /* the far beads first */
        for (int k = 0; k < 26; k++) {
            float a = TAU_F * (float)k / 26.0f, sz = sinf(a);
            if ((sz >= 0.0f) != pass) continue;
            if (k == 6) continue;                                    /* where the tail hangs from */
            float big = (k % 7 == 0) ? 1.7f : 1.25f;
            if (pass == 0) cv_px((int)(lx + rx * cosf(a)), (int)(ly + ry * sz) + 1, sh(R_WOOD, 0.15f));   /* the cord */
            bead(lx + rx * cosf(a), ly + ry * sz, big, pass ? 0.0f : 0.22f);
        }
    float tx0 = lx + rx * cosf(TAU_F * 6.0f / 26.0f), ty0 = ly + ry * sinf(TAU_F * 6.0f / 26.0f);
    for (int k = 0; k < 4; k++) bead(tx0 + 2.5f * (float)k + 1.0f, ty0 + 0.5f * (float)k + 0.5f, k == 0 ? 1.7f : 1.2f, 0.0f);
    float ccx = tx0 + 13.5f, ccy = ty0 + 2.5f;                       /* the cross, lying */
    cv_capsule(ccx - 1.5f, ccy - 1.0f, ccx + 5.5f, ccy + 1.0f, 0.95f, R_BONE, 0.25f, 0.95f);
    cv_capsule(ccx + 1.0f, ccy - 2.8f, ccx + 0.5f, ccy + 2.6f, 0.95f, R_BONE, 0.25f, 0.95f);
    cv_px((int)ccx + 1, (int)ccy, sh(R_BONE, 0.3f));                 /* its corpus, worn away to a mark */
}

/* 6: a pilgrim's ampulla of lead: a flat round flask, its rim rolled, a scallop shell in relief on it, a short neck flared at the
 * mouth, two loops at its shoulders and the cord through them; dull blue-grey, whitened here and there where it has oxidised, dented */
static uint8_t lead(float v) { return sh(R_IRON, 0.14f + 0.66f * clampf(v, 0.0f, 1.0f)); }
static void relic_ampulla(float cx, float by) {
    float cy = by - 8.5f, r = 8.0f;
    for (int y = (int)(cy - r); y <= (int)(cy + r); y++)
        for (int x = (int)(cx - r); x <= (int)(cx + r); x++) {
            float dx = (float)x + 0.5f - cx, dy = (float)y + 0.5f - cy, d = sqrtf(dx * dx + dy * dy);
            if (d > r) continue;
            float L = -(dx + dy) * 0.707f / (d + 0.01f), v = 0.2f + 0.28f * rlit(dx / r, dy / r);
            if (d > r - 1.5f) v = 0.45f + 0.45f * L;                 /* the rolled rim */
            if (h01(x, y, 911) < 0.04f) v += 0.3f;                   /* the white of its oxide */
            if (h01(x, y, 913) < 0.04f) v -= 0.2f;                   /* dents */
            cv_px(x, y, lead(v));
        }
    float hx = cx, hy = cy + 4.0f;                                   /* the shell: ribs fanned up from its hinge */
    for (int k = 0; k < 7; k++) {
        float a = -1.1f + 2.2f * (float)k / 6.0f, ex = hx + sinf(a) * 5.8f, ey = hy - cosf(a) * 5.8f;
        int n = 6;
        for (int i = 1; i <= n; i++) {
            float t = (float)i / (float)n;
            int px = (int)(hx + (ex - hx) * t), py = (int)(hy + (ey - hy) * t);
            cv_px(px, py, lead(k < 4 ? 1.0f : 0.8f)); cv_px(px + 1, py, lead(0.0f));
        }
    }
    for (int a = 0; a < 20; a++) {                                   /* its scalloped edge */
        float an = -1.25f + 2.5f * (float)a / 19.0f, rr = 6.2f + 0.6f * fabsf(sinf(an * 3.5f * 1.6f));
        cv_px((int)(hx + sinf(an) * rr), (int)(hy - cosf(an) * rr), lead(an < 0.0f ? 0.85f : 0.5f));
    }
    cv_px((int)hx - 1, (int)hy + 1, lead(0.9f)); cv_px((int)hx, (int)hy + 1, lead(0.7f)); cv_px((int)hx + 1, (int)hy + 1, lead(0.3f));   /* the hinge's ears */
    for (int y = (int)(cy - r) - 4; y < (int)(cy - r); y++) { cv_px((int)cx - 1, y, lead(0.75f)); cv_px((int)cx, y, lead(0.45f)); cv_px((int)cx + 1, y, lead(0.2f)); }   /* the neck */
    for (int x = (int)cx - 3; x <= (int)cx + 2; x++) cv_px(x, (int)(cy - r) - 5, lead(x < (int)cx ? 0.9f : 0.4f));   /* its mouth */
    for (int s = -1; s <= 1; s += 2)                                 /* the loops */
        for (int a = 0; a < 10; a++) {
            float an = PI_F * (float)a / 9.0f;
            cv_px((int)(cx + (float)s * (2.5f + 2.2f * sinf(an))), (int)(cy - r - 3.0f + 3.8f * (1.0f - cosf(an)) * 0.5f), lead(s < 0 ? 0.75f : 0.35f));
        }
    for (int k = 0; k <= 6; k++) cv_px((int)cx - 4 + k + (k > 3), (int)(cy - r) - 6 - (k > 1 && k < 5), sh(R_WOOD, (k & 1) ? 0.5f : 0.25f));   /* the cord */
}

/* 7: a nail wrapped in faded velvet: a great square-shanked iron nail, rusted, its head hammered flat, lying; bound round its
 * middle in velvet gone from purple to grey at its folds, a gilt thread wound over it crosswise, its ends frayed */
static void relic_nail(float cx, float by) {
    float x0 = cx - 14.0f, y0 = by - 6.0f, x1 = cx + 14.0f, y1 = by - 2.0f;
    float L = hypotf(x1 - x0, y1 - y0), ux = (x1 - x0) / L, uy = (y1 - y0) / L, px = -uy, py = ux;
    for (int i = 0; i <= (int)L; i++) {                              /* the shank, squared, tapering to its point */
        float t = (float)i / L, w = t < 0.8f ? 1.2f : 1.2f * (1.0f - t) / 0.2f;
        float x = x0 + ux * (float)i, y = y0 + uy * (float)i;
        for (float k = -w; k <= w + 0.01f; k += 0.6f) {
            float v = k < 0.0f ? 0.6f : 0.25f;
            uint8_t c = h01(i, (int)(k * 3.0f), 921) < 0.35f ? sh(R_BRICK, 0.15f + 0.3f * v) : sh(R_IRON, v);
            cv_px((int)(x + px * k), (int)(y + py * k), c);
        }
    }
    for (int k = -3; k <= 3; k++) for (int w = -1; w <= 0; w++)      /* its head */
        cv_px((int)(x0 + ux * (float)w + px * (float)k), (int)(y0 + uy * (float)w + py * (float)k), sh(R_IRON, (w ? 0.75f : 0.4f) - 0.06f * (float)k));
    float i0 = L * 0.3f, i1 = L * 0.68f;
    for (int i = (int)i0; i <= (int)i1; i++) {                       /* the velvet, gathered in soft folds */
        float t = ((float)i - i0) / (i1 - i0), bulge = 2.4f + 1.1f * sinf(t * PI_F);
        float x = x0 + ux * (float)i, y = y0 + uy * (float)i;
        for (float k = -bulge; k <= bulge + 0.01f; k += 0.5f) {
            float n = k / bulge, v = 0.16f + 0.48f * rlit(0.0f, n * 0.95f);
            float fold = sinf((float)i * 0.9f + n * 2.0f);
            v += 0.07f * fold;
            uint8_t c = fold > 0.85f && n < -0.2f ? sh(R_STONE, 0.45f + 0.2f * v) : sh(R_PURPLE, v);   /* worn grey at the folds' crests */
            cv_px((int)(x + px * k), (int)(y + py * k), c);
        }
    }
    for (int s = 0; s < 2; s++)                                      /* the gilt thread, wound across it twice */
        for (float k = -3.2f; k <= 3.2f; k += 0.4f) {
            float i = L * 0.49f + (s ? -1.0f : 1.0f) * k * 1.6f, x = x0 + ux * i + px * k, y = y0 + uy * i + py * k;
            cv_px((int)x, (int)y, sh(R_GOLD, k < 0.0f ? 0.85f : 0.45f));
        }
    for (int s = 0; s < 2; s++)                                      /* its frayed ends */
        for (int k = 0; k < 4; k++) {
            float t = s ? 0.69f : 0.29f, off = ((float)k - 1.5f) * 1.4f;
            cv_px((int)(x0 + ux * L * t + px * off + ux * (s ? 1.0f : -1.0f)), (int)(y0 + uy * L * t + py * off + uy * (s ? 1.0f : -1.0f)), sh(R_PURPLE, 0.35f));
        }
}

/* 8: a broken sword's hilt, lying: a wheel pommel, a grip bound in leather and wire, a crossguard with knobbed quillons, and a
 * hand's breadth of blade with its fuller, snapped off ragged */
static void relic_hilt(float cx, float by) {
    float ax = cx - 13.0f, ay = by - 9.0f, bx = cx + 13.0f, byy = by - 1.0f;
    float L = hypotf(bx - ax, byy - ay), ux = (bx - ax) / L, uy = (byy - ay) / L, px = -uy, py = ux;
    float gx = ax + ux * 10.0f, gy = ay + uy * 10.0f;                /* the guard's middle */
    for (int i = 11; i <= 24; i++) {                                 /* the blade, its fuller, its snapped end */
        float x = ax + ux * (float)i, y = ay + uy * (float)i, w = 2.2f - 0.03f * (float)(i - 11);
        int ragged = i > 21 && h01(i, 1, 931) < 0.5f;
        for (float k = -w; k <= w + 0.01f; k += 0.5f) {
            if (ragged && k > 0.0f) continue;
            float v = k < -1.2f ? 0.95f : k < 0.0f ? 0.65f : 0.35f;
            if (fabsf(k) < 0.3f) v = 0.3f;                           /* the fuller */
            cv_px((int)(x + px * k), (int)(y + py * k), sh(R_IRON, v));
        }
    }
    for (int i = 2; i <= 9; i++) {                                   /* the grip, leather and wire */
        float x = ax + ux * (float)i, y = ay + uy * (float)i;
        for (float k = -1.3f; k <= 1.31f; k += 0.5f) {
            int wire = ((i + (int)((k + 1.3f) * 1.5f)) % 3) == 0;
            cv_px((int)(x + px * k), (int)(y + py * k), wire ? sh(R_GOLD, k < 0.0f ? 0.75f : 0.35f) : sh(R_WOOD, k < 0.0f ? 0.4f : 0.15f));
        }
    }
    for (float k = -8.0f; k <= 8.01f; k += 0.5f) {                   /* the crossguard, curving to its quillons */
        float x = gx + px * k + ux * (k * k * 0.02f), y = gy + py * k + uy * (k * k * 0.02f);
        cv_px((int)x, (int)y, sh(R_IRON, 0.85f - 0.03f * fabsf(k))); cv_px((int)(x + ux), (int)(y + uy), sh(R_IRON, 0.35f));
    }
    for (int s = -1; s <= 1; s += 2) cv_ellipse(gx + px * 8.5f * (float)s + ux * 1.4f, gy + py * 8.5f * (float)s + uy * 1.4f, 1.4f, 1.4f, R_IRON, 0.2f, 0.95f);   /* its knobs */
    cv_ellipse(ax, ay, 2.8f, 2.8f, R_IRON, 0.15f, 0.9f);              /* the wheel pommel, its boss */
    cv_px((int)ax, (int)ay, sh(R_IRON, 0.25f)); cv_px((int)ax - 1, (int)ay - 1, sh(R_IRON, 1.0f));
}

/* 9: an abbot's seal-matrix of bronze: a pointed oval stood on its point, its rim raised, its legend a ring of letters, and cut in
 * it, reversed, the abbot with his crozier under a canopy; a lug at its top; verdigris in the cuts */
static void relic_seal(float cx, float by) {
    int b = (int)by, h = 23;
    for (int y = b - h; y <= b; y++) {
        float t = (float)(y - (b - h)) / (float)h, hw = 7.5f * sinf(PI_F * t) + 0.4f;
        for (int x = (int)(cx - hw); x <= (int)(cx + hw); x++) {
            float nx = ((float)x + 0.5f - cx) / (hw + 0.01f), v = 0.45f + 0.25f * (nx < 0.0f) - 0.2f * nx;
            float e = hw - fabsf((float)x + 0.5f - cx);
            if (e < 1.2f) v = nx < 0.0f ? 0.95f : 0.25f;             /* the raised rim */
            else if (e < 2.0f) v = 0.2f;
            else if (e < 3.2f && ((y + x) % 2 == 0)) v = 0.15f;      /* the legend's letters */
            if (h01(x, y, 941) < 0.07f) v -= 0.2f;
            cv_px(x, y, bronze(v));
        }
    }
    static const char *const ABBOT[11] = {                          /* the abbot, cut in: his mitre, head, robe and crozier */
        "..#..##", ".###..#", ".###.#.", "..#..#.", "######.", ".###.#.", ".###.#.", "#####..", "#####..", "#####..", ".###...",
    };
    int fx0 = (int)cx - 3, fy0 = b - 17;
    for (int y = 0; y < 11; y++)
        for (int x = 0; x < 7; x++)
            if (ABBOT[y][x] == '#') {
                cv_px(fx0 + x, fy0 + y, bronze(0.04f));
                if (y == 10 || ABBOT[y + 1][x] != '#') cv_px(fx0 + x, fy0 + y + 1, bronze(0.85f));   /* the cut's lower lip, lit */
            }
    for (int y = 0; y < 11; y++) for (int x = 0; x < 7; x++) if (ABBOT[y][x] == '#') cv_px(fx0 + x, fy0 + y, bronze(0.04f));
    for (int y = b - 22; y <= b - 2; y++)                            /* verdigris in the legend's cuts */
        for (int x = (int)cx - 7; x <= (int)cx + 7; x++)
            if (cv_sprite[y * TS + x] == bronze(0.15f) && h01(x, y, 943) < 0.35f) cv_px(x, y, sh(R_GHOST, 0.26f));
    cv_px((int)cx, b - h - 1, bronze(0.8f)); cv_px((int)cx - 1, b - h - 2, bronze(0.7f)); cv_px((int)cx + 1, b - h - 2, bronze(0.35f)); cv_px((int)cx, b - h - 3, bronze(0.6f));   /* its lug */
}

/* 10: a chained psalter: a thick book lying shut, its boards in dark red leather tooled with a border, brass corners and a boss,
 * its pages' edge below, a strap and clasp across its fore-edge; a chain from its board trailing off along the floor */
static void relic_psalter(float cx, float by) {
    int b = (int)by, x0 = (int)cx - 11, x1 = (int)cx + 10, t0 = b - 12, t1 = b - 5;
    for (int y = t0; y <= t1; y++) {                                 /* the board, seen from above, its far edge set back */
        int sk = (t1 - y) / 3, xa = x0 + sk, xb = x1 + sk;
        for (int x = xa; x <= xb; x++) {
            float u = (float)(x - xa) / (float)(xb - xa), v = 0.38f - 0.12f * u + 0.06f * (h01(x, y, 951) - 0.5f);
            if (y == t0 || x == xa) v += 0.2f;
            if (x == xa + 2 || x == xb - 2 || y == t0 + 1 || y == t1 - 1) v -= 0.16f;   /* its tooled border */
            cv_px(x, y, sh(R_BLOOD, v * 0.8f + 0.04f));
        }
        cv_px(xa, y, sh(R_GOLD, 0.6f)); cv_px(xb, y, sh(R_GOLD, 0.35f));
        if (y <= t0 + 1 || y >= t1 - 1) { cv_px(xa + 1, y, sh(R_GOLD, 0.7f)); cv_px(xb - 1, y, sh(R_GOLD, 0.45f)); }   /* brass corners */
    }
    cv_ellipse(cx + 1.0f, (float)(t0 + t1) * 0.5f, 2.2f, 1.6f, R_GOLD, 0.3f, 0.95f);   /* the boss */
    for (int y = t1 + 1; y <= b - 1; y++)                            /* the pages' edge, and the lower board */
        for (int x = x0; x <= x1; x++) cv_px(x, y, y == b - 1 ? sh(R_BLOOD, 0.15f) : sh(R_BONE, (y & 1) ? 0.72f : 0.5f) - (x > x1 - 3 ? 2 : 0));
    for (int y = t0 + 2; y <= b - 1; y++) { cv_px(x1 + 1, y, sh(R_WOOD, 0.3f)); cv_px(x1 + 2, y, sh(R_WOOD, 0.15f)); }   /* the strap */
    cv_px(x1 + 1, t1 + 1, sh(R_GOLD, 0.95f)); cv_px(x1 + 2, t1 + 1, sh(R_GOLD, 0.5f)); cv_px(x1 + 1, t1 + 2, sh(R_GOLD, 0.55f));   /* its clasp */
    float chx = (float)x0, chy = (float)t1 - 1.0f;                   /* the chain, off along the floor */
    for (int k = 0; k < 6; k++) {
        float x = chx - 1.5f - 2.5f * (float)k, y = chy + 1.0f + 0.6f * (float)k;
        if (k & 1) { cv_px((int)x, (int)y, sh(R_IRON, 0.7f)); cv_px((int)x + 1, (int)y, sh(R_IRON, 0.35f)); }
        else { cv_px((int)x - 1, (int)y, sh(R_IRON, 0.75f)); cv_px((int)x, (int)y - 1, sh(R_IRON, 0.6f)); cv_px((int)x + 1, (int)y, sh(R_IRON, 0.3f)); cv_px((int)x, (int)y + 1, sh(R_IRON, 0.2f)); }
    }
}

/* 11: a funerary mask of bronze, propped on its chin: a calm face, broad at the cheekbones, its rim catching the light; its brow
 * ridged over eyes shut in deep almonds, its nose straight, its lips closed; tarnished, verdigris gathered in its hollows, a crack
 * across its cheek, holes for its ties at its edges */
static void relic_mask(float cx, float by) {
    float cy = by - 10.5f, rx = 8.0f, ry = 10.5f;
    for (int y = (int)(cy - ry); y <= (int)(cy + ry); y++)
        for (int x = (int)(cx - rx); x <= (int)(cx + rx); x++) {
            float nx = ((float)x + 0.5f - cx) / rx, ny = ((float)y + 0.5f - cy) / ry;
            float w = ny > 0.15f ? 1.0f - (ny - 0.15f) * 0.85f : ny < -0.5f ? 1.0f - (-0.5f - ny) * 0.5f : 1.0f;   /* a brow, cheekbones, a chin */
            float q = nx * nx / (w * w) + ny * ny;
            if (q > 1.0f) continue;
            float v = 0.2f + 0.55f * rlit(nx / w, ny * 0.8f), ax = fabsf((float)x + 0.5f - cx);
            int ey = (int)(cy - 2.0f), hollow = 0;
            if (q > 0.8f) v = nx < 0.0f || ny < -0.3f ? 0.85f : 0.22f;   /* its rim */
            if (y == ey - 3 && ax < 6.0f) v += 0.22f;                /* the brow */
            if (y == ey - 1 && ax > 1.0f && ax < 6.0f) { v -= 0.18f; hollow = 1; }   /* its shadow over the eyes */
            if ((y == ey || y == ey + 1) && ax > 1.4f && ax < 5.4f && !(y == ey + 1 && (ax < 2.2f || ax > 4.6f))) { v = 0.02f; hollow = 1; }   /* the eyes, shut */
            if (y == ey + 2 && ax > 1.6f && ax < 5.2f) v += 0.18f;   /* the lids' lower edge */
            if (ax < 1.1f && y > ey - 2 && y < (int)cy + 3) v = x < (int)cx ? 0.85f : 0.3f;   /* the nose */
            if (y == (int)cy + 3 && ax < 2.0f) { v = 0.1f; hollow = 1; }   /* its nostrils */
            if (y == (int)cy + 5 && ax < 3.2f) { v = 0.04f; hollow = 1; }  /* the lips, closed */
            if (y == (int)cy + 6 && ax < 2.6f) v += 0.22f;
            uint8_t c = bronze(v);
            if (hollow && h01(x, y, 963) < 0.6f && v > 0.02f) c = sh(R_GHOST, 0.22f + 0.15f * v);   /* verdigris in the hollows */
            else if (q < 0.8f && h01(x, y, 961) < 0.05f) c = sh(R_GHOST, 0.3f);
            cv_px(x, y, c);
        }
    for (int k = 0; k < 6; k++) cv_px((int)cx + 3 + k / 2, (int)cy + 1 + k, bronze(0.02f));   /* a crack */
    cv_px((int)(cx - rx) + 1, (int)cy - 3, 0); cv_px((int)(cx + rx) - 2, (int)cy - 3, 0);   /* the holes for its ties */
}

/* 12: a weeping eye: an eye of crystal, its iris lit within, set in a gilt mount with rays about it, on a short stem and three
 * claw feet; a red tear fallen from it down the stem */
static void relic_eye(float cx, float by) {
    int b = (int)by;
    float ey = by - 13.0f;
    for (int k = 0; k < 12; k++) {                                   /* the rays */
        float a = TAU_F * (float)k / 12.0f, l = (k & 1) ? 3.0f : 5.0f;
        for (int j = 0; j < (int)l; j++) cv_px((int)(cx + cosf(a) * (10.0f + (float)j)), (int)(ey + sinf(a) * (6.5f + (float)j * 0.7f)), sh(R_GOLD, (cosf(a) + sinf(a) < 0.0f ? 0.8f : 0.4f) - 0.06f * (float)j));
    }
    for (int y = (int)ey - 6; y <= (int)ey + 6; y++)                 /* the mount and the eye in it */
        for (int x = (int)cx - 10; x <= (int)cx + 10; x++) {
            float dx = ((float)x + 0.5f - cx) / 10.0f, dy = ((float)y + 0.5f - ey);
            float hh = 6.0f * (1.0f - dx * dx);
            if (fabsf(dy) > hh) continue;
            float e = hh - fabsf(dy);
            uint8_t c;
            if (e < 1.3f) c = sh(R_GOLD, dy < 0.0f || dx < -0.6f ? 0.9f : 0.35f);   /* the lids of gilt */
            else {
                float ix = (float)x + 0.5f - cx, r = sqrtf(ix * ix + dy * dy);
                if (r < 1.3f) c = 0;                                 /* the pupil */
                else if (r < 3.8f) c = (uint8_t)(R_GLOW + 7 + (int)((3.8f - r) * 1.4f) - (dy > 1.0f));   /* the iris, lit within */
                else c = sh(R_GHOST, 0.72f - 0.25f * (dy > 0.0f) - 0.1f * fabsf(dx));   /* the white of the crystal */
            }
            cv_px(x, y, c);
        }
    cv_px((int)cx - 2, (int)ey - 2, C_WHITE);
    cv_capsule(cx, ey + 6.0f, cx, (float)b - 3.0f, 1.0f, R_GOLD, 0.3f, 0.85f);   /* the stem */
    cv_ellipse(cx, (float)b - 4.0f, 1.8f, 1.2f, R_GOLD, 0.25f, 0.9f);
    for (int s = -1; s <= 1; s++) { cv_line(cx, (float)b - 3, cx + (float)s * 5.0f, (float)b, sh(R_GOLD, s < 0 ? 0.75f : 0.4f)); cv_px((int)cx + s * 5 + s, b, sh(R_GOLD, 0.5f)); }   /* its claw feet */
    int tx = (int)cx + 2;                                            /* the tear, from the lower lid down the stem */
    cv_px(tx, (int)ey + 5, sh(R_BLOOD, 0.85f)); cv_px(tx, (int)ey + 6, sh(R_BLOOD, 0.75f));
    cv_px(tx - 1, (int)ey + 8, sh(R_BLOOD, 0.95f)); cv_px(tx, (int)ey + 8, sh(R_BLOOD, 0.7f)); cv_px(tx - 1, (int)ey + 9, sh(R_BLOOD, 0.55f)); cv_px(tx, (int)ey + 9, sh(R_BLOOD, 0.4f));
    cv_px(tx - 1, (int)ey + 7, sh(R_BLOOD, 0.6f));
}

/* each relic, its foot at (cx, by): on the floor or in the socket's hands. The key is the art's own (relic_bone), laid as it is */
static void relic_draw(int k, float cx, float by, int held) {
    switch (k) {
    case RL_KEY: if (held) relic_bone(18, 27, 46, 26, 1); else relic_bone(17, 59, 47, 52, 0); break;
    case RL_FINGER: relic_finger(cx, by); break;
    case RL_HEART: relic_heart(cx, by); break;
    case RL_CROWN: relic_crown(cx, by); break;
    case RL_ROSARY: relic_rosary(cx, by); break;
    case RL_AMPULLA: relic_ampulla(cx, by); break;
    case RL_NAIL: relic_nail(cx, by); break;
    case RL_HILT: relic_hilt(cx, by); break;
    case RL_SEAL: relic_seal(cx, by); break;
    case RL_PSALTER: relic_psalter(cx, by); break;
    case RL_MASK: relic_mask(cx, by); break;
    case RL_EYE: relic_eye(cx, by); break;
    }
}

/* the shape each relic cuts in the socket's panel (eleven by seven), lit when it is held */
static const char *const RELIC_CUT[NRELIC][7] = {
    { "...........", ".##.....##.", "###########", ".##.#.#.##.", "###########", ".##.....##.", "..........." },
    { "....###....", "...#...#...", "...#.#.#...", "...#.#.#...", "...#...#...", "...#####...", "..#######.." },
    { "..##...##..", ".####.####.", ".#########.", "..#######..", "...#####...", "....###....", ".....#....." },
    { ".#..#.#..#.", ".##.###.##.", ".#########.", ".#########.", ".#.#.#.#.#.", ".#########.", "..........." },
    { "..#.#.#.#..", ".#.......#.", "..#.#.#.#..", ".....#.....", "....###....", ".....#.....", "..........." },
    { "..#.###.#..", "..##.#.##..", "...#####...", "..##...##..", "..#..#..#..", "..##...##..", "...#####..." },
    { "...........", ".#.........", ".##........", ".##########", ".##........", ".#.........", "..........." },
    { "....###....", ".....#.....", ".....#.....", ".#########.", "....###....", "....#.#....", "..........." },
    { ".....#.....", "....#.#....", "...#.#.#...", "...#.#.#...", "...#.#.#...", "....#.#....", ".....#....." },
    { ".#########.", ".#.......#.", ".#.#####.##", ".#.......##", ".#.#####.#.", ".#.......#.", ".#########." },
    { "...#####...", "..#.....#..", "..#.#.#.#..", "..#..#..#..", "..#.###.#..", "...#...#...", "....###...." },
    { "...........", "...#####...", ".##.###.##.", "#..#####..#", ".##.###.##.", "...#####...", ".....#....." },
};

/* a relic on the floor, as a sprite: its shadow, the relic, its outline, a glint */
static void spr_relic_k(uint8_t *out, int k) {
    cv_begin(cv_sprite, TS, TS);
    cv_flat(32, 61.5f, k == RL_FINGER || k == RL_MASK || k == RL_SEAL || k == RL_EYE ? 9.0f : 15.0f, 1.8f, R_WARM + 1);
    relic_draw(k, 32, 60, 0);
    cv_outline(0);
    if (k == RL_KEY) { cv_px(31, 54, C_WHITE); cv_px(22, 55, C_WHITE); }
    for (int x = 0; x < TS; x++) for (int y = 0; y < TS; y++) out[x * TS + y] = cv_sprite[y * TS + x];
}
/* a relic held: the socket as the art makes it, its panel's cut made the relic's own and alight, the relic laid in the hands */
static void spr_socket_k(uint8_t *out, int k) {
    memcpy(out, crawl_spr[SP_EV_SOCKET0], TS * TS);
    for (int y = 45; y <= 52; y++)                                   /* the panel, its cut filled in again */
        for (int x = 26; x <= 38; x++) out[x * TS + y] = sh(R_STONE, 0.3f + 0.05f * (h01(x, y, 833) - 0.5f));
    for (int y = 0; y < 7; y++)
        for (int x = 0; x < 11; x++)
            if (RELIC_CUT[k][y][x] == '#') out[(27 + x) * TS + 45 + y] = (uint8_t)(R_GLOW + 10 + ((x + y) & 1) * 3);
    cv_begin(cv_sprite, TS, TS);
    relic_draw(k, 32, k == RL_ROSARY || k == RL_NAIL || k == RL_HILT || k == RL_PSALTER ? 31 : 32, 1);
    cv_outline(0);
    for (int x = 0; x < TS; x++) for (int y = 0; y < TS; y++) if (cv_sprite[y * TS + x] != C_CLEAR) out[x * TS + y] = cv_sprite[y * TS + x];
    out[28 * TS + 43] = (uint8_t)(R_GLOW + 12); out[34 * TS + 42] = (uint8_t)(R_GLOW + 11); out[31 * TS + 41] = (uint8_t)(R_GLOW + 13);
}

/* ================================================================== */
/* The sheet                                                           */
/* ================================================================== */

static int iw, ih;
static uint8_t *img;
static void put(int x, int y, int r, int g, int b) { if (x < 0 || y < 0 || x >= iw || y >= ih) return; uint8_t *p = img + ((size_t)y * iw + x) * 3; p[0] = r; p[1] = g; p[2] = b; }
static void rect(int x, int y, int w, int h, int r, int g, int b) { for (int yy = y; yy < y + h; yy++) for (int xx = x; xx < x + w; xx++) put(xx, yy, r, g, b); }
static void text(int x, int y, int sc, const char *s, int r, int g, int b) {
    for (; *s; s++, x += 8 * sc)
        for (int row = 0; row < 8; row++)
            for (int col = 0; col < 8; col++)
                if (font_bios8[(uint8_t)*s][row] >> (7 - col) & 1) rect(x + col * sc, y + row * sc, sc, sc, r, g, b);
}
#define INK 216, 208, 189
#define GOLDINK 233, 194, 76
#define DIMINK 150, 140, 128
static void pal_put(int x, int y, uint8_t c) { put(x, y, crawl_pal[c][0] * 255 / 63, crawl_pal[c][1] * 255 / 63, crawl_pal[c][2] * 255 / 63); }

static const uint8_t BAYER4[4][4] = { { 0, 8, 2, 10 }, { 12, 4, 14, 6 }, { 3, 11, 1, 9 }, { 15, 7, 13, 5 } };
static uint8_t shadeb(uint8_t c, float light, int x, int y) {   /* as draw/floor.c's shade */
    if (IS_FULLBRIGHT(c)) return c;
    int lv = (int)(light * LIGHT_UNIT + BAYER4[y & 3][x & 3] * (1.0f / 16.0f));
    return crawl_cmap[clampi(lv, 0, NLIGHT - 1)][c];
}

/* a cell enlarged: a backdrop (the run's wall over its floor), and over it a sprite or a face, at a light */
static void cell(int ox, int oy, int sc, const uint8_t *wall, const uint8_t *floor, const uint8_t *spr, const uint8_t *face, float light) {
    int n = TS * sc;
    for (int y = 0; y < n; y++)
        for (int x = 0; x < n; x++) {
            int tx = x / sc, ty = y / sc;
            uint8_t c;
            if (face) c = face[tx * TS + ty];
            else {
                c = ty < 52 ? wall[tx * TS + ty] : floor[tx * TS + ((ty - 52) * 5) % TS];
                float l = light * (ty < 52 ? 0.8f : 0.9f);
                c = crawl_cmap[clampi((int)(l * LIGHT_UNIT), 0, NLIGHT - 1)][c];
                if (spr && spr[tx * TS + ty] != C_CLEAR) c = spr[tx * TS + ty];
                else { pal_put(ox + x, oy + y, c); continue; }
            }
            pal_put(ox + x, oy + y, IS_FULLBRIGHT(c) ? c : crawl_cmap[clampi((int)(light * LIGHT_UNIT), 0, NLIGHT - 1)][c]);
        }
}

/* the game's view at its own size (320x200's pixels, FOCAL as the game's): a back wall at wd cells, the floor and vault
 * between; a sprite on the floor at sd cells, or a face in the wall's middle cell, or a texture on the floor's cell under the
 * sprite's place. The light: amb everywhere and a light at (lx, ly) of pw falling off over lr cells; dapple throws it
 * through a pierced lantern's holes. */
typedef struct {
    const uint8_t *wall, *floor, *ceil, *spr, *face, *flat;
    float wd, sd, amb, lx, ly, lz, pw, lr; int dapple;
} View;
static float view_light(const View *v, float x, float y, float z) {
    float dx = x - v->lx, dy = y - v->ly, dz = z - v->lz, d2 = dx * dx + dy * dy + dz * dz * 0.5f;
    float l = v->pw * expf(-d2 / (v->lr * v->lr));
    if (v->dapple) {                                                 /* the holes' light: spots on what it falls on */
        float a = atan2f(dy, dx), e = atan2f(dz, sqrtf(dx * dx + dy * dy));
        float s = 0.5f + 0.5f * sinf(a * 14.0f) * sinf(e * 12.0f + 1.0f);
        l *= 0.35f + 1.1f * (s > 0.62f ? 1.0f : s * 0.5f);
    }
    return v->amb + l;
}
static void view(int ox, int oy, int w, int h, const View *v) {
    const float F = (320 / 2) / 0.75f, camz = 0.5f, hz = (float)h * 0.5f;
    float top = hz - (1.0f - camz) * F / v->wd, bot = hz + camz * F / v->wd;
    for (int y = 0; y < h; y++)
        for (int x = 0; x < w; x++) {
            float cxp = (float)x + 0.5f - (float)w * 0.5f;
            uint8_t c; float l;
            if ((float)y >= top && (float)y < bot) {                 /* the back wall */
                float wx = cxp * v->wd / F + 0.5f, wy = ((float)y - top) / (bot - top);
                int tx = (int)floorf((wx - floorf(wx)) * TS), ty = clampi((int)(wy * TS), 0, TS - 1), mid = wx >= 0.0f && wx < 1.0f;
                c = (mid && v->face ? v->face : v->wall)[tx * TS + ty];
                l = view_light(v, wx - 0.5f, v->wd, 1.0f - wy) * (1.0f / (1.0f + 0.01f * v->wd * v->wd));
            } else {
                int fl = (float)y >= bot;
                float dist = (fl ? camz : 1.0f - camz) * F / fabsf((float)y + 0.5f - hz);
                float wx = cxp * dist / F + 0.5f, wy = dist;
                int tx = (int)floorf((wx - floorf(wx)) * TS) & 63, ty = (int)floorf((wy - floorf(wy)) * TS) & 63;
                const uint8_t *t = fl ? v->floor : v->ceil;
                c = t[tx * TS + ty];
                if (fl && v->flat && wx >= 0.0f && wx < 1.0f && wy >= v->sd - 0.5f && wy < v->sd + 0.5f) {
                    uint8_t k = v->flat[tx * TS + ((int)floorf((v->sd + 0.5f - wy) * TS) & 63)];
                    if (k != C_CLEAR) c = k;
                }
                l = view_light(v, wx - 0.5f, wy, fl ? 0.0f : 1.0f) * (1.0f / (1.0f + 0.01f * dist * dist)) * (fl ? 1.0f : 0.85f);
            }
            pal_put(ox + x, oy + y, shadeb(c, l, x, y));
        }
    if (v->spr) {                                                    /* the sprite, a cell high, on the floor at sd */
        float size = F / v->sd, sx0 = (float)w * 0.5f - size * 0.5f, sy1 = hz + camz * F / v->sd, sy0 = sy1 - size;
        float l = view_light(v, 0.0f, v->sd, 0.5f) * (1.0f / (1.0f + 0.01f * v->sd * v->sd));
        for (int y = (int)ceilf(sy0); y < (int)ceilf(sy1); y++)
            for (int x = (int)ceilf(sx0); x < (int)ceilf(sx0 + size); x++) {
                if (x < 0 || y < 0 || x >= w || y >= h) continue;
                int tx = clampi((int)(((float)x + 0.5f - sx0) / size * TS), 0, TS - 1), ty = clampi((int)(((float)y + 0.5f - sy0) / size * TS), 0, TS - 1);
                uint8_t c = v->spr[tx * TS + ty];
                if (c != C_CLEAR) pal_put(ox + x, oy + y, shadeb(c, l, x, y));
            }
    }
}

static void put32(FILE *f, uint32_t v) { uint8_t b[4] = { v >> 24, v >> 16, v >> 8, v }; fwrite(b, 1, 4, f); }
static void chunk(FILE *f, const char *type, const uint8_t *data, uint32_t len) {
    put32(f, len); fwrite(type, 1, 4, f); if (len) fwrite(data, 1, len, f);
    uint32_t crc = crc32(0, (const Bytef *)type, 4); if (len) crc = crc32(crc, data, len); put32(f, crc);
}
static int write_png(const char *path) {
    size_t rowlen = 1 + (size_t)iw * 3;
    uint8_t *raw = malloc(rowlen * ih);
    for (int y = 0; y < ih; y++) { raw[rowlen * y] = 0; memcpy(raw + rowlen * y + 1, img + (size_t)y * iw * 3, (size_t)iw * 3); }
    uLongf clen = compressBound((uLong)(rowlen * ih)); uint8_t *comp = malloc(clen);
    compress2(comp, &clen, raw, (uLong)(rowlen * ih), 9);
    FILE *f = fopen(path, "wb"); if (!f) { free(raw); free(comp); return 0; }
    static const uint8_t sig[8] = { 0x89, 'P', 'N', 'G', 13, 10, 26, 10 }; fwrite(sig, 1, 8, f);
    uint8_t ihdr[13] = { iw >> 24, iw >> 16, iw >> 8, iw, ih >> 24, ih >> 16, ih >> 8, ih, 8, 2, 0, 0, 0 };
    chunk(f, "IHDR", ihdr, 13); chunk(f, "IDAT", comp, (uint32_t)clen); chunk(f, "IEND", NULL, 0);
    fclose(f); free(raw); free(comp); return 1;
}

static const int RUNLEVEL[4] = { 3, 13, 23, 33 };
static const char *const RUNA[4] = { "THE", "THE", "THE", "THE" };
static const char *const RUNB[4] = { "CRYPTS", "CATACOMBS", "HOLLOWS", "DEEP" };
static void run_art(int r) { crawl_art_level(RUNLEVEL[r]); }
static void text_lines(int x, int y, int sc, const char *const *l, int n) { for (int i = 0; i < n; i++) text(x, y + i * 10 * sc, sc, l[i], INK); }

/* ---- sheet 1: the four hanging lights ---- */
#define SC 2
#define CW (TS * SC)
#define PAD 8
#define LAB 168
#define VW 200
#define VH 128
#define VH2 160
#define LIT 1.15f
#define DARK 0.55f
static uint8_t LT[4][4][3][TS * TS];                     /* [light][run][frame] */
static const int NFR[4] = { 3, 2, 3, 3 };
static void make_lights(int r) {
    run_art(r);
    for (int f = 0; f < 3; f++) {
        memcpy(LT[0][r][f], crawl_spr[SP_CORONA0 + f], TS * TS);       /* the game's own, made for the run's level */
        if (f < 2) memcpy(LT[1][r][f], crawl_spr[SP_SANCT0 + f], TS * TS);
        memcpy(LT[2][r][f], crawl_spr[SP_BONELAB0 + f], TS * TS);
        memcpy(LT[3][r][f], crawl_spr[SP_CAGE0 + f], TS * TS);
    }
}
static int sheet_lights(const char *path) {
    static const char *const NAME[4] = { "THE IRON CORONA", "THE SANCTUARY LAMP", "THE BONE CANDELABRUM", "THE PIERCED CAGE LANTERN" };
    static const char *const WHERE[4] = {
        "A GREAT WHEEL OF WROUGHT IRON ON FOUR CHAINS: TWO BANDS BARRED BETWEEN, EIGHT BARBED SPIKES, EIGHT CANDLES, SIX",
        "RED GLASS IN A BRASS CUP ON THREE FINE CHAINS FROM A SMOKE BELL, A SMALL FLAME IN IT: A LOW, STEADY GLOW.",
        "A GREAT SKULL WITH A STAG'S ANTLERS BOUND TO IT, A SKULL HUNG UNDER EACH END, RIBS ROUND A SPINE, A JAW AT",
        "TALL, OF BLACK IRON: A CROCKETED SPIRE, A LANCET OVER A QUATREFOIL IN FRONT, HOLES ROUND THE SIDES; ITS",
    };
    static const char *const WHERE2[4] = {
        "SPOKES TO A BOSS AND ITS DROP. FOR HALLS AND NAVES. ITS LIGHT AS THE CHANDELIER'S, OR A LITTLE WIDER.",
        "FOR CHAPELS AND BURIAL VAULTS. ITS LIGHT SMALL AND RED (A COLOUR TO SETTLE IN THE GAME'S STEP).",
        "THE FOOT; A CANDLE IN EACH EYE OF EACH SKULL. FOR OSSUARIES AND CHARNEL PITS.",
        "LIGHT COMES THROUGH THE HOLES. FOR PASSAGES AND SMALL ROOMS.",
    };
    int bandh = 62 + 4 * (CW + PAD + 4);
    iw = PAD + LAB + 4 * (CW + PAD) + VW + PAD * 2; ih = 176 + (30 + CW + 26) + 4 * bandh + 8;
    img = calloc((size_t)iw * ih * 3, 1);
    rect(0, 0, iw, ih, 16, 14, 20);
    text(PAD, 12, 2, "FOUR HANGING LIGHTS, FOR THE OWNER'S LOOK", GOLDINK);
    text(PAD + 42 * 16, 16, 1, "(PLAN-LIGHTS, STEP 2)", DIMINK);
    static const char *const HEAD[] = {
        "NOTHING HERE IS IN THE GAME. EACH IS DRAWN IN EACH RUN'S PALETTE: THE CATACOMBS' WATER RUSTS AND DRIPS ON IT AS",
        "ON THE CHANDELIER, THE HOLLOWS' SOOT DARKENS IT, THE DEEP BURNS BLACK CANDLES ON IRON GONE GLASSY. THE FRAMES ARE",
        "THE FLICKER; THE SWAY IS THE GAME'S, AS THE CHANDELIER'S. EACH FRAME TWICE ITS SIZE IN ITS OWN LIGHT; THEN THE FIRST",
        "DARKER, AS FROM A HALL'S FAR END; THEN AT THE GAME'S OWN SIZE, A CROP OF ITS 320X200 VIEW: THE LIGHT HUNG TWO AND",
        "A HALF CELLS OFF, A WALL FOUR AWAY. THE CAGE'S VIEW THROWS ITS HOLES' LIGHT IN SPOTS, AS IT MIGHT (NOT BUILT).",
    };
    text_lines(PAD, 44, 1, HEAD, 5);
    for (int r = 0; r < 4; r++) make_lights(r);
    /* for scale: the chandelier and the lantern the game hangs today, and the four new, the Crypts' */
    int y = 112;
    text(PAD, y, 2, "FOR SCALE, IN THE CRYPTS", GOLDINK);
    text(PAD + 25 * 16, y + 4, 1, "TODAY'S CHANDELIER AND LANTERN, THEN THE FOUR", INK);
    y += 30;
    run_art(0);
    const uint8_t *wall = crawl_tex[T_MATV(0, 0)], *floor = crawl_tex[T_FLOORS];
    static const char *const SCALE[6] = { "CHANDELIER", "LANTERN", "CORONA", "SANCTUARY", "BONES", "CAGE" };
    for (int k = 0; k < 6; k++) {
        int ox = PAD + k * (CW + PAD) + (k >= 2 ? 16 : 0);
        const uint8_t *s = k == 0 ? crawl_spr[SP_CHANDELIER_A] : k == 1 ? crawl_spr[SP_LANTERN_A] : LT[k - 2][0][0];
        cell(ox, y, SC, wall, floor, s, NULL, LIT);
        if (k < 2) text(ox, y + CW + 6, 1, SCALE[k], DIMINK); else text(ox, y + CW + 6, 1, SCALE[k], INK);
    }
    y += CW + 30;
    for (int L = 0; L < 4; L++) {
        text(PAD, y + 4, 2, NAME[L], GOLDINK);
        text(PAD, y + 24, 1, WHERE[L], INK); text(PAD, y + 34, 1, WHERE2[L], INK);
        y += 62;
        for (int c = 0; c < 4; c++) {
            const char *h = c < 3 ? (c < NFR[L] ? (c == 0 ? "FRAME 1" : c == 1 ? "FRAME 2" : "FRAME 3") : "") : "DARKER";
            if (c < 3) text(PAD + LAB + c * (CW + PAD), y - 12, 1, h, INK); else text(PAD + LAB + c * (CW + PAD), y - 12, 1, h, DIMINK);
        }
        text(PAD + LAB + 4 * (CW + PAD), y - 12, 1, "AT 1X, IN THE GAME'S VIEW", INK);
        for (int r = 0; r < 4; r++) {
            run_art(r);
            wall = crawl_tex[T_MATV(0, 0)]; floor = crawl_tex[T_FLOORS];
            text(PAD, y + CW / 2 - 18, 2, RUNA[r], INK); text(PAD, y + CW / 2 + 2, 2, RUNB[r], INK);
            for (int c = 0; c < 4; c++) {
                if (c < 3 && c >= NFR[L]) continue;
                cell(PAD + LAB + c * (CW + PAD), y, SC, wall, floor, LT[L][r][c < 3 ? c : 0], NULL, c < 3 ? LIT : DARK);
            }
            View v = { wall, floor, crawl_tex[T_CEILS], LT[L][r][0], NULL, NULL, 4.0f, 2.5f, 0.32f, 0.0f, 2.5f, 0.55f, 1.0f, 2.2f, L == 3 };
            view(PAD + LAB + 4 * (CW + PAD), y, VW, VH, &v);
            y += CW + PAD + 4;
        }
    }
    if (!write_png(path)) { fprintf(stderr, "cannot write %s\n", path); return 1; }
    printf("%s: %dx%d\n", path, iw, ih);
    return 0;
}

/* ---- sheet 2: the events' things ---- */
#define NTH 9
static int sheet_things(const char *path) {
    static const char *const COLA[NTH] = { "BARRED", "UNBARRED", "RELIC", "SOCKET", "SOCKET", "LEVER", "LEVER", "PLATE", "PLATE" };
    static const char *const COLB[NTH] = { "DOOR", "DOOR", "", "EMPTY", "HOLDING", "UP", "DOWN", "UP", "PRESSED" };
    iw = PAD + LAB + NTH * (CW + PAD) + PAD; ih = 150 + 6 * (CW + PAD + 6) + 40 + 2 * (VH2 + 22) + 12;
    img = calloc((size_t)iw * ih * 3, 1);
    rect(0, 0, iw, ih, 16, 14, 20);
    text(PAD, 12, 2, "THE EVENTS' THINGS, FOR THE OWNER'S LOOK (PLAN-EVENTS; PLAN-LIGHTS, STEP 2)", GOLDINK);
    static const char *const HEAD[] = {
        "NOTHING HERE IS IN THE GAME. TODAY THESE BORROW THE PUZZLES' LOOKS (THE TOP ROW). THE BARRED DOOR IS THE RUN'S OWN DOOR WITH A BEAM ACROSS IT",
        "IN TWO STAPLES AND A CHAIN CROSSED OVER IT TO AN IRON PLATE, A HAND CUT IN THE PLATE: NO KEYHOLE, SO NOTHING SAYS A KEY OPENS IT. UNBARRED (AN",
        "EVENT'S UNLOCK, BEFORE IT SLIDES): THE BEAM AND PLATE GONE, A CHAIN HUNG LOOSE. THE RELIC IS A BONE BOUND IN GOLD, A KEY'S TEETH AT ITS END, A RED",
        "SEAL ON A CORD. THE SOCKET IS A PLINTH WITH TWO STONE HANDS HELD UP ON IT AND THE RELIC'S SHAPE CUT IN ITS FACE; HOLDING, THE RELIC LIES ACROSS",
        "THE HANDS AND THE SHAPE IS ALIGHT. THE LEVER: A STONE POST, A TOOTHED IRON QUADRANT, A TWISTED ARM. THE PLATE IS A FLOOR TEXTURE: A FLAGSTONE",
        "IN A DEEP JOINT, A ROSE CUT IN A ROUNDEL, IRON STUDS; PRESSED, SUNK AND DARKER, THE PIT'S SIDE SHOWING, DUST PUFFED FROM THE JOINT.",
    };
    text_lines(PAD, 40, 1, HEAD, 6);
    int y = 116;
    for (int c = 0; c < NTH; c++) { text(PAD + LAB + c * (CW + PAD), y, 1, COLA[c], INK); text(PAD + LAB + c * (CW + PAD), y + 10, 1, COLB[c], INK); }
    y += 26;
    static uint8_t T[NTH][TS * TS], P[2][TS * TS], tmp[TS * TS];
    for (int row = 0; row < 6; row++) {                              /* today; the four runs; the Crypts darker */
        int r = row == 0 || row == 5 ? 0 : row - 1;
        run_art(r);
        const uint8_t *wall = crawl_tex[T_MATV(0, 0)], *floor = crawl_tex[T_FLOORS];
        if (row == 0) {
            text(PAD, y + CW / 2 - 18, 2, "TODAY", DIMINK); text(PAD, y + CW / 2 + 2, 1, "THE CRYPTS", DIMINK);
            cell(PAD + LAB, y, SC, wall, floor, NULL, crawl_tex[T_GATE], LIT);
            cell(PAD + LAB + 2 * (CW + PAD), y, SC, wall, floor, crawl_spr[SP_PZ_FOSSIL], NULL, LIT);
            cell(PAD + LAB + 3 * (CW + PAD), y, SC, wall, floor, crawl_spr[SP_PZ_PLINTH], NULL, LIT);
            memcpy(tmp, crawl_spr[SP_PZ_PLINTH], TS * TS);
            for (int i = 0; i < TS * TS; i++) { int x = i / TS, yy = i % TS; if (yy + 17 < TS) { uint8_t c = crawl_spr[SP_PZ_FOSSIL][x * TS + yy + 17]; if (c != C_CLEAR && c != R_WARM + 1) tmp[i] = c; } }
            cell(PAD + LAB + 4 * (CW + PAD), y, SC, wall, floor, tmp, NULL, LIT);
            cell(PAD + LAB + 5 * (CW + PAD), y, SC, wall, floor, crawl_spr[SP_PZ_LEVER0], NULL, LIT);
            cell(PAD + LAB + 6 * (CW + PAD), y, SC, wall, floor, crawl_spr[SP_PZ_LEVER1], NULL, LIT);
            today_plate(P[0], floor, 0); today_plate(P[1], floor, 1);
            cell(PAD + LAB + 7 * (CW + PAD), y, SC, NULL, NULL, NULL, P[0], LIT);
            cell(PAD + LAB + 8 * (CW + PAD), y, SC, NULL, NULL, NULL, P[1], LIT);
            text(PAD + LAB + 1 * (CW + PAD) + 8, y + CW / 2 - 4, 1, "(IT SLIDES)", DIMINK);
            y += CW + PAD + 18;
            continue;
        }
        static const int16_t ID[NTH] = { T_BARRED, T_UNBARRED, SP_EV_RELIC, SP_EV_SOCKET0, SP_EV_SOCKET1, SP_EV_LEVER0, SP_EV_LEVER1, -1, -1 };
        for (int c = 0; c < 7; c++) memcpy(T[c], c < 2 ? crawl_tex[ID[c]] : crawl_spr[ID[c]], TS * TS);   /* the game's own */
        plate_on(T[7], floor, 0); plate_on(T[8], floor, 1);
        if (row == 5) { text(PAD, y + CW / 2 - 18, 2, "DARKER", DIMINK); text(PAD, y + CW / 2 + 2, 1, "THE CRYPTS", DIMINK); }
        else { text(PAD, y + CW / 2 - 18, 2, RUNA[r], INK); text(PAD, y + CW / 2 + 2, 2, RUNB[r], INK); }
        for (int c = 0; c < NTH; c++) {
            int face = c < 2 || c > 6;
            cell(PAD + LAB + c * (CW + PAD), y, SC, wall, floor, face ? NULL : T[c], face ? T[c] : NULL, row == 5 ? DARK : LIT);
        }
        y += CW + PAD + 6;
    }
    y += 12;
    text(PAD, y, 2, "AT 1X, IN THE GAME'S VIEW (A CROP OF 320X200)", GOLDINK);
    y += 40;
    static const char *const VL[6] = { "THE DOOR, 3 CELLS OFF", "UNBARRED, 3 OFF", "THE RELIC, 2 OFF", "THE SOCKET HOLDING, 2.5", "THE LEVER DOWN, 2.5", "THE PLATE, 2 OFF" };
    for (int row = 0; row < 2; row++) {
        int r = row ? 3 : 0;
        run_art(r);
        const uint8_t *wall = crawl_tex[T_MATV(0, 0)], *floor = crawl_tex[T_FLOORS];
        memcpy(T[0], crawl_tex[T_BARRED], TS * TS); memcpy(T[1], crawl_tex[T_UNBARRED], TS * TS);
        memcpy(T[2], crawl_spr[SP_EV_RELIC], TS * TS); memcpy(T[4], crawl_spr[SP_EV_SOCKET1], TS * TS); memcpy(T[6], crawl_spr[SP_EV_LEVER1], TS * TS);
        plate_on(T[8], floor, 0);
        text(PAD, y + VH2 / 2 - 18, 2, RUNA[r], INK); text(PAD, y + VH2 / 2 + 2, 2, RUNB[r], INK);
        for (int k = 0; k < 6; k++) {
            View v = { wall, floor, crawl_tex[T_CEILS], NULL, NULL, NULL, 4.0f, 2.0f, 0.45f, 0.6f, 1.2f, 0.6f, 0.75f, 2.6f, 0 };
            if (k < 2) { v.face = T[k]; v.wd = 3.0f; }
            else if (k == 5) { v.flat = T[8]; v.sd = 2.0f; }
            else { v.spr = T[k == 2 ? 2 : k == 3 ? 4 : 6]; v.sd = k == 2 ? 2.0f : 2.5f; }
            int ox = PAD + LAB + k * (VW + 4);
            view(ox, y, VW, VH2, &v);
            if (row == 0) text(ox, y - 12, 1, VL[k], INK);
        }
        y += VH2 + 22;
    }
    if (!write_png(path)) { fprintf(stderr, "cannot write %s\n", path); return 1; }
    printf("%s: %dx%d\n", path, iw, ih);
    return 0;
}

/* ---- sheet 3: the twelve relics ---- */
#define RLAB 200
#define RV 128
static int sheet_relics(const char *path) {
    static const char *const NAME[NRELIC] = { "THE BONE KEY", "THE SAINT'S FINGER", "THE HEART", "THE CROWNED SKULL", "THE ROSARY", "THE AMPULLA",
                                              "THE NAIL", "THE BROKEN HILT", "THE ABBOT'S SEAL", "THE PSALTER", "THE MASK", "THE WEEPING EYE" };
    static const char *const SAY[NRELIC][3] = {
        { "A BONE BOUND IN GOLD", "WIRE, A KEY'S TEETH,", "A RED SEAL. TODAY'S." },
        { "IN A CRYSTAL TUBE ON A", "GILT FOOT, A RING ON IT,", "A SPIRE AND A CROSS." },
        { "DRIED, IN A SILVER", "CASKET, ITS LID BACK,", "A BAND OF GILT THORNS." },
        { "A GOLD CROWN OF FIVE", "POINTS, A RUBY AND TWO", "AMETHYSTS IN ITS BAND." },
        { "BONE BEADS IN A LOOP,", "A BIGGER ONE EACH", "DECADE, A BONE CROSS." },
        { "A PILGRIM'S FLASK OF", "LEAD, A SCALLOP SHELL", "ON IT, LOOPS, A CORD." },
        { "SQUARE IRON, RUSTED,", "BOUND IN FADED VELVET", "AND GILT THREAD." },
        { "WHEEL POMMEL, WIRED", "GRIP, KNOBBED GUARD, A", "HAND OF SNAPPED BLADE." },
        { "A POINTED OVAL OF", "BRONZE, THE ABBOT CUT", "IN IT, ITS LUG ON TOP." },
        { "RED LEATHER, BRASS", "CORNERS AND CLASP, A", "CHAIN FROM ITS BOARD." },
        { "FUNERARY, OF BRONZE,", "EYES SHUT, VERDIGRIS", "IN ITS HOLLOWS." },
        { "CRYSTAL, LIT WITHIN, IN", "A RAYED GILT MOUNT; A", "RED TEAR DOWN ITS STEM." },
    };
    static const char *const COL[10] = { "THE CRYPTS", "THE CATACOMBS", "THE HOLLOWS", "THE DEEP", "DARKER", "IN THE SOCKET",
                                         "1X: FLOOR", "1X: SOCKET", "1X: FLOOR", "1X: SOCKET" };
    int rowh = CW + 10, x1x = PAD + RLAB + 6 * (CW + PAD);
    iw = x1x + 4 * (RV + 6) + PAD; ih = 132 + NRELIC * rowh + 8;
    img = calloc((size_t)iw * ih * 3, 1);
    rect(0, 0, iw, ih, 16, 14, 20);
    text(PAD, 12, 2, "TWELVE RELICS, FOR THE OWNER'S LOOK (PLAN-EVENTS)", GOLDINK);
    static const char *const HEAD[] = {
        "NOTHING HERE IS IN THE GAME. A LOOK FOR EACH RELIC, TO BE CHOSEN FOR IT IN THE EDITOR ONE DAY; THE FIRST IS TODAY'S. EACH LIES ON THE FLOOR IN EACH",
        "RUN'S PALETTE, TWICE ITS SIZE IN A TORCH'S LIGHT, THEN DARKER; THEN HELD IN THE SOCKET, WHOSE PANEL CUTS EACH ONE'S OWN SHAPE AND LIGHTS IT; THEN",
        "AT THE GAME'S OWN SIZE, A CROP OF ITS 320X200 VIEW: ON THE FLOOR TWO CELLS OFF AND IN THE SOCKET TWO AND A HALF, IN THE CRYPTS AND IN THE DEEP.",
    };
    text_lines(PAD, 40, 1, HEAD, 3);
    int y = 84;
    for (int c = 0; c < 10; c++) {
        int ox = c < 6 ? PAD + RLAB + c * (CW + PAD) : x1x + (c - 6) * (RV + 6);
        text(ox, y, 1, COL[c], c == 4 ? 150 : 216, c == 4 ? 140 : 208, c == 4 ? 128 : 189);
        if (c >= 6) text(ox, y + 10, 1, c < 8 ? "THE CRYPTS" : "THE DEEP", 150, 140, 128);
    }
    y += 32;
    static uint8_t R[TS * TS], S[TS * TS];
    for (int k = 0; k < NRELIC; k++, y += rowh) {
        char num[4]; snprintf(num, sizeof num, "%d", k + 1);
        text(PAD, y + 6, 2, num, GOLDINK);
        text(PAD + 36, y + 10, 1, NAME[k], GOLDINK);
        for (int l = 0; l < 3; l++) text(PAD, y + 36 + l * 12, 1, SAY[k][l], INK);
        for (int c = 0; c < 6; c++) {
            int r = c < 4 ? c : 0;
            run_art(r);
            spr_relic_k(R, k);
            const uint8_t *wall = crawl_tex[T_MATV(0, 0)], *floor = crawl_tex[T_FLOORS];
            if (c == 5) spr_socket_k(S, k);
            cell(PAD + RLAB + c * (CW + PAD), y, SC, wall, floor, c == 5 ? S : R, NULL, c == 4 ? DARK : LIT);
        }
        for (int c = 0; c < 4; c++) {
            run_art(c < 2 ? 0 : 3);
            spr_relic_k(R, k); spr_socket_k(S, k);
            View v = { crawl_tex[T_MATV(0, 0)], crawl_tex[T_FLOORS], crawl_tex[T_CEILS], (c & 1) ? S : R, NULL, NULL, 4.0f, (c & 1) ? 2.5f : 2.0f, 0.45f, 0.6f, 1.2f, 0.6f, 0.75f, 2.6f, 0 };
            view(x1x + c * (RV + 6), y, RV, RV, &v);
        }
    }
    if (!write_png(path)) { fprintf(stderr, "cannot write %s\n", path); return 1; }
    printf("%s: %dx%d\n", path, iw, ih);
    return 0;
}

int main(int argc, char **argv) {
    if (argc < 3) { fprintf(stderr, "usage: %s lights|things|relics out.png\n", argv[0]); return 1; }
    crawl_art_init();
    if (!strcmp(argv[1], "lights")) return sheet_lights(argv[2]);
    if (!strcmp(argv[1], "things")) return sheet_things(argv[2]);
    if (!strcmp(argv[1], "relics")) return sheet_relics(argv[2]);
    fprintf(stderr, "lights, things or relics\n");
    return 1;
}
