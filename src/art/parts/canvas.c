/* A part of crawl_art.c (docs/PLAN-tidy.md): it is #included there in its place and never compiled on its own.
 * The sprite canvas (cv_*), the master skull and its stamps; the caster's hand in motion (crawl_hand_frame). */

/* ------------------------------------------------------------------ */
/* Sprite canvas                                                       */
/* ------------------------------------------------------------------ */

static uint8_t *cvp;
static int cvw, cvh;
static uint8_t cv_sprite[TS * TS];

EyeAt crawl_eyes[NSPR];
static EyeAt g_eyes;                                    /* the eyes drawn on the sprite being made, for cv_store */
static int g_back, g_keep;                              /* 1.0.0b18: a back view (skel_draw, skull_stamp); skel_draw keeps what is drawn */
static int g_skbox[4] = { -1, -1, -1, -1 };             /* where the last hand-drawn skull went: the grime goes lightly on it */
static void cv_begin(uint8_t *buf, int w, int h) { cvp = buf; cvw = w; cvh = h; memset(buf, C_CLEAR, (size_t)w * h); memset(&g_eyes, 0, sizeof g_eyes); g_skbox[0] = g_skbox[2] = -1; }
static void cv_px(int x, int y, uint8_t c) { if ((unsigned)x < (unsigned)cvw && (unsigned)y < (unsigned)cvh) cvp[y * cvw + x] = c; }
static void cv_eye_mark(int x, int y) {                 /* an eye here, for the eye pass (the first two count) */
    if (g_eyes.n < 2 && (unsigned)x < (unsigned)TS && (unsigned)y < (unsigned)TS) { g_eyes.x[g_eyes.n] = (int8_t)x; g_eyes.y[g_eyes.n] = (int8_t)y; g_eyes.n++; }
}
static void cv_eye(int x, int y, uint8_t c) { cv_px(x, y, c); cv_eye_mark(x, y); }

static float lit(float nx, float ny, float nz) {
    float d = nx * -0.45f + ny * -0.65f + nz * 0.62f;
    return d < 0.0f ? 0.0f : d;
}

static void cv_ellipse(float cx, float cy, float rx, float ry, int ramp, float lo, float hi) {
    for (int y = (int)floorf(cy - ry); y <= (int)ceilf(cy + ry); y++)
        for (int x = (int)floorf(cx - rx); x <= (int)ceilf(cx + rx); x++) {
            float nx = (x + 0.5f - cx) / rx, ny = (y + 0.5f - cy) / ry, d = nx * nx + ny * ny;
            if (d > 1.0f) continue;
            cv_px(x, y, sh(ramp, lo + (hi - lo) * lit(nx, ny, sqrtf(1.0f - d))));
        }
}

static void cv_flat(float cx, float cy, float rx, float ry, uint8_t c) {
    for (int y = (int)floorf(cy - ry); y <= (int)ceilf(cy + ry); y++)
        for (int x = (int)floorf(cx - rx); x <= (int)ceilf(cx + rx); x++) {
            float nx = (x + 0.5f - cx) / rx, ny = (y + 0.5f - cy) / ry;
            if (nx * nx + ny * ny <= 1.0f) cv_px(x, y, c);
        }
}

static void cv_capsule(float x0, float y0, float x1, float y1, float r, int ramp, float lo, float hi) {
    float vx = x1 - x0, vy = y1 - y0, len2 = vx * vx + vy * vy + 1e-6f;
    int xa = (int)floorf(fminf(x0, x1) - r), xb = (int)ceilf(fmaxf(x0, x1) + r);
    int ya = (int)floorf(fminf(y0, y1) - r), yb = (int)ceilf(fmaxf(y0, y1) + r);
    for (int y = ya; y <= yb; y++)
        for (int x = xa; x <= xb; x++) {
            float px = x + 0.5f, py = y + 0.5f;
            float t = clampf(((px - x0) * vx + (py - y0) * vy) / len2, 0.0f, 1.0f);
            float ox = (px - (x0 + vx * t)) / r, oy = (py - (y0 + vy * t)) / r, d = ox * ox + oy * oy;
            if (d > 1.0f) continue;
            cv_px(x, y, sh(ramp, lo + (hi - lo) * lit(ox, oy, sqrtf(1.0f - d))));
        }
}

static void cv_line(float x0, float y0, float x1, float y1, uint8_t c) {
    int n = (int)(fmaxf(fabsf(x1 - x0), fabsf(y1 - y0))) + 1;
    for (int i = 0; i <= n; i++) cv_px((int)(x0 + (x1 - x0) * i / n), (int)(y0 + (y1 - y0) * i / n), c);
}

static void cv_poly(const float *p, int n, int ramp, float lo, float hi, float ystart, float yspan) {
    float ymin = 1e9f, ymax = -1e9f;
    for (int i = 0; i < n; i++) { ymin = fminf(ymin, p[2 * i + 1]); ymax = fmaxf(ymax, p[2 * i + 1]); }
    for (int y = (int)floorf(ymin); y <= (int)ceilf(ymax); y++) {
        float yc = y + 0.5f, xs[16]; int k = 0;
        for (int i = 0; i < n && k < 16; i++) {
            float ax = p[2 * i], ay = p[2 * i + 1], bx = p[2 * ((i + 1) % n)], by = p[2 * ((i + 1) % n) + 1];
            if ((ay <= yc && by > yc) || (by <= yc && ay > yc)) xs[k++] = ax + (bx - ax) * (yc - ay) / (by - ay);
        }
        for (int i = 1; i < k; i++) { float v = xs[i]; int j = i - 1; while (j >= 0 && xs[j] > v) { xs[j + 1] = xs[j]; j--; } xs[j + 1] = v; }
        for (int i = 0; i + 1 < k; i += 2)
            for (int x = (int)ceilf(xs[i] - 0.5f); x <= (int)floorf(xs[i + 1] - 0.5f); x++) {
                float v = lo + (hi - lo) * clampf(1.0f - (yc - ystart) / yspan, 0.0f, 1.0f);
                cv_px(x, y, sh(ramp, v));
            }
    }
}

static void cv_outline(uint8_t c) {                    /* dark rim around the silhouette */
    static uint8_t tmp[HAND_W * HAND_H];
    memcpy(tmp, cvp, (size_t)cvw * cvh);
    for (int y = 0; y < cvh; y++)
        for (int x = 0; x < cvw; x++) {
            if (tmp[y * cvw + x] != C_CLEAR) continue;
            int n = 0;
            if (x > 0 && tmp[y * cvw + x - 1] != C_CLEAR && !IS_FULLBRIGHT(tmp[y * cvw + x - 1])) n++;
            if (x < cvw - 1 && tmp[y * cvw + x + 1] != C_CLEAR && !IS_FULLBRIGHT(tmp[y * cvw + x + 1])) n++;
            if (y > 0 && tmp[(y - 1) * cvw + x] != C_CLEAR && !IS_FULLBRIGHT(tmp[(y - 1) * cvw + x])) n++;
            if (y < cvh - 1 && tmp[(y + 1) * cvw + x] != C_CLEAR && !IS_FULLBRIGHT(tmp[(y + 1) * cvw + x])) n++;
            if (n) cvp[y * cvw + x] = c;
        }
}

static void cv_store(int id) {
    crawl_eyes[id] = g_eyes;
    for (int x = 0; x < TS; x++)
        for (int y = 0; y < TS; y++) crawl_spr[id][x * TS + y] = cv_sprite[y * TS + x];
}

static void cv_bone(float x0, float y0, float x1, float y1, float r, float lo, float hi) {
    float dx = x1 - x0, dy = y1 - y0, L = sqrtf(dx * dx + dy * dy) + 1e-3f, px = -dy / L, py = dx / L;
    cv_capsule(x0, y0, x1, y1, r, R_BONE, lo, hi);
    for (int e = 0; e < 2; e++) {
        float ex = e ? x1 : x0, ey = e ? y1 : y0;
        cv_ellipse(ex + px * r * 0.7f, ey + py * r * 0.7f, r * 1.05f, r * 1.05f, R_BONE, lo, hi);
        cv_ellipse(ex - px * r * 0.7f, ey - py * r * 0.7f, r * 1.05f, r * 1.05f, R_BONE, lo, hi);
    }
}

static void cv_skull(float cx, float cy, float s, float lo, float hi) {
    cv_ellipse(cx, cy, 6.2f * s, 5.8f * s, R_BONE, lo, hi);
    cv_ellipse(cx, cy + 4.2f * s, 4.4f * s, 2.9f * s, R_BONE, lo * 0.9f, hi * 0.85f);
    cv_flat(cx - 2.4f * s, cy + 0.9f * s, 1.9f * s, 2.1f * s, R_BONE);
    cv_flat(cx + 2.4f * s, cy + 0.9f * s, 1.9f * s, 2.1f * s, R_BONE);
    cv_flat(cx, cy + 3.1f * s, 0.8f * s, 1.1f * s, R_BONE);
    for (int i = -2; i <= 2; i++) cv_px((int)(cx + i * 1.3f * s), (int)(cy + 5.3f * s), R_BONE + 1);
}

/* The skull, drawn by hand (docs/InitialArtPass.md section 4.1): a character a pixel, a step of the bone ramp in hex,
 * 0 black, @ an eye, . clear. The key light is up and to the left; the cavities are the darkest things on it; the jaw
 * hangs a pixel open and a tooth is gone. Front, and the profile facing right. */
#define SKULL_W 15
#define SKULL_H 16
_Static_assert(SKULL_H == 16, "the skulls' grids are sixteen rows: keep them so when redrawing");
static const char *const SKULL_FRONT[SKULL_H] = {
    ".....7aa97.....", "...7bdeedcb8...", "..8ceffeedcb8..", ".9dffeeeeddcb8.", ".7cdeeedddccb7.",
    ".5acdddddccb95.", ".6deeedbdeeda6.", ".800009c900006.", ".b0@01ad910@08.", "5d1003908200194", ".bedc90208bba6.",
    "..7cb60706a95..", "...8e3f4e3c6...", "...400000003...", "...7d2c0b2a5...", "....5acba84....",
};
static const char *const SKULL_SIDE[SKULL_H] = {
    "...68abbba86...", ".6acdeeeedcb7..", "6bdeeffeeddca6.", "8ceeffeeeddcb8.", "7bcdeeeedddccb7",
    "6abcdddddddccde", "59abccddddcb00a", "489abbccdcb0@0b", ".789abbccb9000d", ".6890abdeedcb0c", "..789a8bccdcb9b",
    "...789637bdccdb", "....8a5.9e3f3fd", "....79400000...", "....69ac2b2b2a.", ".....589abba96.",
};
/* the skull from behind (1.0.0b18): the dome, the suture across it, the bump of the occiput, the dark of the foramen with
 * the neck going down from it, the jaw's back corners either side; no eyes, the one exception the art pass allows */
static const char *const SKULL_BACK[SKULL_H] = {
    ".....7aa97.....",
    "...7bdebdcb8...",
    "..8ceffbedcb8..",
    ".9dffeebeddcb8.",
    ".8deeeebddccb7.",
    ".7cdeee6ddcba6.",
    ".6cdee6d6dcba6.",
    ".6bcd5ddc5bba5.",
    ".5bc5ddccc5a95.",
    "5abccccbbba9a74",
    ".7abccbba9a885.",
    "..69ab8008a96..",
    "..5a7.0000.693.",
    "..48...00...5..",
    "......5a5......",
    "......4a4......",
};
/* a skull with its middle at cx, cy (its eyes a row above the middle); side: the profile, mirror: facing left;
 * eye: the eyes' colour (recorded for the eye pass), 0 dark sockets */
static void skull_stamp(float cx, float cy, int side, int mirror, uint8_t eye) {
    const char *const *g = g_back ? SKULL_BACK : side ? SKULL_SIDE : SKULL_FRONT;
    int x0 = (int)floorf(cx - SKULL_W / 2.0f + 0.5f), y0 = (int)floorf(cy - SKULL_H / 2.0f + 0.5f);
    g_skbox[0] = x0; g_skbox[1] = y0; g_skbox[2] = x0 + SKULL_W; g_skbox[3] = y0 + SKULL_H;
    for (int y = 0; y < SKULL_H; y++)
        for (int x = 0; x < SKULL_W; x++) {
            char ch = g[y][mirror ? SKULL_W - 1 - x : x];
            if (ch == '.') continue;
            if (ch == '@') { if (eye) cv_eye(x0 + x, y0 + y, eye); else cv_px(x0 + x, y0 + y, 0); continue; }
            cv_px(x0 + x, y0 + y, ch == '0' ? 0 : (uint8_t)(R_BONE + (ch <= '9' ? ch - '0' : ch - 'a' + 10)));
        }
}

/* A hand-drawn grid of any size (docs/InitialArtPass.md): hex 1-f a step of the body's ramp r1, g-v a step of a second
 * ramp r2 (g the darkest), A-P a step of the fire (full-bright), 0 black, @ an eye (recorded; drawn in eye), . clear.
 * Its top left at x0, y0; mirror flips it. */
static void grid_stamp(const char *const *g, int w, int h, int x0, int y0, int r1, int r2, uint8_t eye, int mirror) {
    for (int y = 0; y < h; y++)
        for (int x = 0; x < w; x++) {
            char ch = g[y][mirror ? w - 1 - x : x];
            int px = x0 + x, py = y0 + y;
            if (ch == '.') continue;
            if (ch == '@') { if (eye) cv_eye(px, py, eye); else cv_px(px, py, (uint8_t)(r1 + 4)); continue; }   /* eye 0: blank, a statue's */
            if (ch == '0') cv_px(px, py, 0);
            else if (ch >= '1' && ch <= '9') cv_px(px, py, (uint8_t)(r1 + ch - '0'));
            else if (ch >= 'a' && ch <= 'f') cv_px(px, py, (uint8_t)(r1 + ch - 'a' + 10));
            else if (ch >= 'g' && ch <= 'v') cv_px(px, py, (uint8_t)(r2 + ch - 'g'));
            else if (ch >= 'A' && ch <= 'P') cv_px(px, py, (uint8_t)(R_FLAME + ch - 'A'));
        }
}

/* ------------------------------------------------------------------ */
/* The caster's hand in motion                                         */
/* ------------------------------------------------------------------ */
/* The traced hand bent at its knuckles, for an idle and a cast. Each finger and the thumb has an axis from knuckle to
 * tip, through the middle of the finger as traced; a frame turns each about its knuckle (anticlockwise on the screen
 * for a positive turn) and draws it in along its length, which is a curl seen from behind. The bend comes in over the
 * finger's base, so the back of the hand stays as traced, and a curled finger's tip darkens, turned from the light.
 * Each pixel goes with the finger nearest it, blended a pixel or so either side of the line between two. */
static const float HAND_AXIS[5][4] = {                     /* knuckle x, y; tip x, y: index to little, then the thumb */
    { 28, 46, 19.2f, 11 }, { 48, 44, 25.2f, 1.5f }, { 67, 45, 43.5f, 7 }, { 83, 47, 64.5f, 19 }, { 30, 86, 3, 67 } };
static const float HAND_POSE[NHF][5][2] = {               /* each one's turn (radians) and length */
    [HF_REST]   = { { 0, 1 }, { 0, 1 }, { 0, 1 }, { 0, 1 }, { 0, 1 } },
    [HF_IDLE1]  = { { -0.02f, 0.97f }, { -0.01f, 0.96f }, { 0.01f, 0.955f }, { 0.03f, 0.95f }, { -0.03f, 0.99f } },
    [HF_IDLE2]  = { { -0.04f, 0.93f }, { -0.015f, 0.92f }, { 0.025f, 0.9f }, { 0.05f, 0.88f }, { -0.06f, 0.97f } },
    [HF_GATHER] = { { -0.07f, 0.85f }, { -0.02f, 0.82f }, { 0.04f, 0.8f }, { 0.09f, 0.78f }, { -0.14f, 0.92f } },   /* drawn in */
    [HF_CLAW]   = { { -0.08f, 0.72f }, { -0.02f, 0.68f }, { 0.05f, 0.66f }, { 0.1f, 0.64f }, { -0.2f, 0.86f } },     /* a claw */
    [HF_LOOSE]  = { { 0.1f, 1.03f }, { 0.035f, 1.04f }, { -0.035f, 1.03f }, { -0.1f, 1.02f }, { 0.12f, 1.0f } },      /* flung open */
    [HF_AFTER]  = { { 0.05f, 0.98f }, { 0.017f, 0.97f }, { -0.017f, 0.96f }, { -0.05f, 0.95f }, { 0.05f, 0.99f } },   /* easing */
};
const int8_t crawl_hand_shift[NHF][2] = { [HF_LOOSE] = { -3, -5 }, [HF_AFTER] = { -1, -2 } };   /* thrust up and out as it goes */
enum { HWX = 112, HWY = 108 };                             /* the part of the hand that bends: fingers and thumb */
static struct { float kx, ky, ux, uy, L, c, s, len, curl; } hbone[5];

static void hand_pose(int k) {                             /* the frame's bones, worked out once */
    for (int f = 0; f < 5; f++) {
        const float *a = HAND_AXIS[f];
        float ux = a[2] - a[0], uy = a[3] - a[1], L = sqrtf(ux * ux + uy * uy);
        hbone[f].kx = a[0]; hbone[f].ky = a[1]; hbone[f].ux = ux / L; hbone[f].uy = uy / L; hbone[f].L = L;
        hbone[f].c = cosf(HAND_POSE[k][f][0]); hbone[f].s = sinf(HAND_POSE[k][f][0]); hbone[f].len = HAND_POSE[k][f][1];
        hbone[f].curl = clampf((1.0f - HAND_POSE[k][f][1]) / 0.34f, 0.0f, 1.0f);
    }
}

static void hand_disp(float px, float py, float *dx, float *dy, float *curl) {
    float w[5], d[5][2], wa[5], frac[5], dmin = 1e9f, sum = 0.0f;
    for (int f = 0; f < 5; f++) {
        const float kx = hbone[f].kx, ky = hbone[f].ky, ux = hbone[f].ux, uy = hbone[f].uy, L = hbone[f].L;
        float rx = px - kx, ry = py - ky, along = rx * ux + ry * uy, perp = -rx * uy + ry * ux;
        float dist = fabsf(perp) + (along > L ? along - L : 0.0f);
        w[f] = dist;                                          /* no reach: past the tips it is air, and goes with them */
        if (dist < dmin) dmin = dist;
        wa[f] = smoothstepf(-3.0f, 9.0f, along);
        frac[f] = along / L;
        float al = along > 0.0f ? along * hbone[f].len : along;
        float qx = ux * al - uy * perp, qy = uy * al + ux * perp;   /* drawn in along it, then turned about the knuckle */
        d[f][0] = kx + qx * hbone[f].c + qy * hbone[f].s - px; d[f][1] = ky - qx * hbone[f].s + qy * hbone[f].c - py;
    }
    *dx = *dy = *curl = 0.0f;
    for (int f = 0; f < 5; f++) {                             /* the nearest finger leads, the next blends in over a few pixels */
        float e = (w[f] - dmin) / 2.8f;
        w[f] = 1.0f / (1.0f + e * e);
        sum += w[f];
    }
    for (int f = 0; f < 5; f++) {
        float k01 = w[f] / sum;
        *dx += k01 * wa[f] * d[f][0]; *dy += k01 * wa[f] * d[f][1];
        *curl += k01 * hbone[f].curl * clampf((frac[f] - 0.5f) * 2.0f, 0.0f, 1.0f);
    }
}

static int hand_ramp(uint8_t c) { return c >= R_GOLD && c < R_GOLD + 13 ? 100 : (c - 1) >> 4; }

void crawl_hand_frame(int k, const uint8_t *src, const uint8_t *lab, uint8_t *dst, uint8_t *dlab, int tip[2]) {
    memcpy(dst, src, HAND_W * HAND_H); memcpy(dlab, lab, HAND_W * HAND_H);
    hand_pose(k);
    float tx, ty, tc;
    hand_disp(32.0f, 1.0f, &tx, &ty, &tc);                         /* where the spell forms, between the tallest fingertips */
    tip[0] = (int)lroundf(32.0f + tx); tip[1] = (int)lroundf(1.0f + ty);
    if (k == HF_REST) return;
    static uint8_t dark[HWY * HWX];                                /* the darkening of a curled tip, by source pixel */
    static float mx[(HWY + 2) * (HWX + 2)], my[(HWY + 2) * (HWX + 2)];   /* where each pixel's middle came from */
    for (int y = 0; y < HWY; y++)
        for (int x = 0; x < HWX; x++) {
            float a, b, c;
            hand_disp(x + 0.5f, y + 0.5f, &a, &b, &c);
            dark[y * HWX + x] = (uint8_t)lroundf(c * 2.5f);
        }
    for (int y = -1; y <= HWY; y++)
        for (int x = -1; x <= HWX; x++) {
            float qx = x + 0.5f, qy = y + 0.5f, px = qx, py = qy, a, b, c;
            for (int it = 0; it < 6; it++) { hand_disp(px, py, &a, &b, &c); px = qx - a; py = qy - b; }
            mx[(y + 1) * (HWX + 2) + x + 1] = px; my[(y + 1) * (HWX + 2) + x + 1] = py;
        }
    for (int y = 0; y < HWY; y++)
        for (int x = 0; x < HWX; x++) {
            int m = (y + 1) * (HWX + 2) + x + 1, ctr = -1, n = 0;
            float px = mx[m], py = my[m];                          /* and its neighbours', for the samples between */
            float ax = (mx[m + 1] - mx[m - 1]) * 0.5f, ay = (my[m + 1] - my[m - 1]) * 0.5f;
            float bx = (mx[m + HWX + 2] - mx[m - HWX - 2]) * 0.5f, by = (my[m + HWX + 2] - my[m - HWX - 2]) * 0.5f;
            if (fabsf(ax) + fabsf(ay) > 2.0f || fabsf(bx) + fabsf(by) > 2.0f) { ax = 1.0f; ay = 0.0f; bx = 0.0f; by = 1.0f; }   /* a fold: plain samples */
            uint8_t cs[9];
            for (int sy = -1; sy <= 1; sy++)
                for (int sx = -1; sx <= 1; sx++) {
                    int ix = (int)floorf(px + (ax * sx + bx * sy) / 3.0f), iy = (int)floorf(py + (ay * sx + by * sy) / 3.0f);
                    if (ix < 0 || iy < 0 || ix >= HAND_W || iy >= HAND_H) continue;
                    uint8_t col = src[iy * HAND_W + ix];
                    if (col == C_CLEAR) continue;
                    if (col >= R_SKIN && col < R_SKIN + 16 && ix < HWX && iy < HWY)
                        col = (uint8_t)(R_SKIN + clampi(col - R_SKIN - dark[iy * HWX + ix], 1, 15));
                    if (!sx && !sy) ctr = iy * HAND_W + ix;
                    cs[n++] = col;
                }
            int at = y * HAND_W + x;
            if (n < 5) { dst[at] = C_CLEAR; dlab[at] = 0; continue; }
            int best = -1, bn = 0;                                 /* the ramp most of it fell in, at its middling step */
            for (int i = 0; i < n; i++) {
                int c = 0;
                for (int j = 0; j < n; j++) c += hand_ramp(cs[j]) == hand_ramp(cs[i]);
                if (c > bn) { bn = c; best = hand_ramp(cs[i]); }
            }
            uint8_t v[9]; int nv = 0;
            for (int i = 0; i < n; i++) if (hand_ramp(cs[i]) == best) {
                int j = nv++;
                while (j > 0 && v[j - 1] > cs[i]) { v[j] = v[j - 1]; j--; }
                v[j] = cs[i];
            }
            dst[at] = v[nv / 2];
            dlab[at] = ctr >= 0 ? lab[ctr] : 0;
        }
    for (int y = 0; y < HWY; y++)                                  /* the outline again, where the bend has moved it */
        for (int x = 0; x < HWX; x++) {
            uint8_t *p = &dst[y * HAND_W + x];
            if (*p < R_SKIN + 4 || *p >= R_SKIN + 16) continue;
            int edge = x == 0 || y == 0 || dst[y * HAND_W + x - 1] == C_CLEAR || dst[y * HAND_W + x + 1] == C_CLEAR
                    || dst[(y - 1) * HAND_W + x] == C_CLEAR || dst[(y + 1) * HAND_W + x] == C_CLEAR;
            if (edge) *p = (uint8_t)(R_SKIN + 1);
        }
}

