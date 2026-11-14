/* A part of crawl_art.c (docs/PLAN-tidy.md): it is #included there in its place and never compiled on its own.
 * The mice (docs/PLAN-mice.md): the small ones at the level's size, the charnel rat they grow into, the holes in the run's walls. */

/* ------------------------------------------------------------------ */
/* The mice                                                            */
/* ------------------------------------------------------------------ */
/* Each size is drawn to fill its box, as the spiderlings are, and shown at the box's share of a cell (crawl_mouse_box). In profile
 * they are drawn facing left and stored facing right, as every side view is (DR_FLIPH turns them back). Fur R_WARM while they are
 * mice, R_WOOD from the Catacombs (the rat's rank family, PF_HIDE); ears, feet and tail R_SKIN; the eyes in the moss's top step on
 * the sprite, for the pages, and the palette's last entry in the world (C_MOUSE_EYE, the eye pass). */
typedef struct {
    float len, tall, head, snout, ear, leg, tail;    /* the torso's half-length and half-height, the head's radius, the snout ... */
    float mange, hunch, ribs, tooth, scar, lo, hi, eye;
    int ramp;
} MouseLook;
static const MouseLook MOUSE_LOOK[MOUSE_STAGES + 1] = {
    { 11.0f, 8.0f, 7.0f, 4.0f, 5.4f, 2.0f, 1.2f,  0, 0, 0, 0, 0,     0.24f, 0.80f, 1.8f, R_WARM },   /* a house mouse */
    { 11.5f, 8.0f, 7.0f, 4.3f, 5.1f, 2.2f, 1.25f, 0, 0, 0, 0, 0,     0.22f, 0.78f, 1.8f, R_WARM },
    { 12.5f, 7.8f, 6.8f, 4.8f, 4.7f, 2.5f, 1.3f,  0, 0, 0, 0, 0,     0.20f, 0.74f, 1.7f, R_WARM },   /* leaner */
    { 13.5f, 7.6f, 6.6f, 5.4f, 4.3f, 2.8f, 1.4f,  0, 0, 0, 0.6f, 0,  0.18f, 0.70f, 1.7f, R_WARM },   /* the snout longer */
    { 14.5f, 7.4f, 6.4f, 6.0f, 3.9f, 3.0f, 1.5f,  0, 0, 0, 1.0f, 0.4f, 0.16f, 0.66f, 1.6f, R_WARM }, /* rat-shaped */
    { 15.0f, 7.5f, 6.3f, 6.5f, 3.6f, 3.4f, 1.6f,  0.06f, 0, 0, 1.2f, 0.6f, 0.16f, 0.62f, 1.7f, R_WOOD },   /* darker, wet */
    { 16.0f, 8.5f, 6.6f, 7.0f, 3.4f, 5.0f, 1.9f,  0.30f, 2.5f, 1.0f, 2.0f, 1.0f, 0.13f, 0.58f, 2.0f, R_WOOD },   /* the spurt */
    { 16.5f, 9.5f, 7.6f, 7.5f, 3.7f, 12.0f, 2.4f, 0.45f, 3.0f, 0.7f, 3.0f, 1.0f, 0.11f, 0.52f, 2.2f, R_WOOD },   /* the charnel rat */
};
#define MOUSE_EYE_PIX (R_MOSS + 15)

static void mo_fur(const MouseLook *L, uint32_t seed) {          /* each pixel of the fur a step either way, its grain, bare patches */
    for (int y = 0; y < TS; y++)
        for (int x = 0; x < TS; x++) {
            uint8_t c = cv_sprite[y * TS + x];
            if (c == C_CLEAR || c < L->ramp || c >= L->ramp + 16) continue;
            int st = c - L->ramp;
            float n = h01(x, y, seed);
            if (n > 0.78f && st < 15) st++;
            else if (n < 0.22f && st > 0) st--;
            if (((x + y * 3) % 5 == 0) && st > 1) st--;
            if (L->mange > 0.0f && fbm(x * 0.18f, y * 0.18f, 64, seed + 9) > 1.0f - L->mange * 0.9f)
                cv_sprite[y * TS + x] = (uint8_t)(R_SKIN + clampi(st / 2 + 2, 1, 10));
            else cv_sprite[y * TS + x] = (uint8_t)(L->ramp + st);
        }
}
static int mo_bez(float *out, float x0, float y0, float x1, float y1, float x2, float y2, float x3, float y3, int n) {
    for (int i = 0; i < n; i++) {
        float t = (float)i / (float)(n - 1), u = 1 - t;
        out[2 * i] = u * u * u * x0 + 3 * u * u * t * x1 + 3 * u * t * t * x2 + t * t * t * x3;
        out[2 * i + 1] = u * u * u * y0 + 3 * u * u * t * y1 + 3 * u * t * t * y2 + t * t * t * y3;
    }
    return n;
}
static void mo_tail(const float *pts, int n, float r0, float r1, int scaly) {
    for (int i = 0; i + 1 < n; i++) {
        float t = (float)i / (float)(n - 1), r = r0 + (r1 - r0) * t;
        cv_capsule(pts[2 * i], pts[2 * i + 1], pts[2 * i + 2], pts[2 * i + 3], r, R_SKIN, 0.18f, 0.55f - 0.15f * t);
        if (scaly && (i & 1)) cv_px((int)pts[2 * i], (int)pts[2 * i + 1], (uint8_t)(R_SKIN + 2));
    }
}
static void mo_ear(float x, float y, float r, int back, int torn) {
    cv_ellipse(x, y, r, r * 1.1f, R_SKIN, 0.20f, 0.55f);
    if (!back) cv_ellipse(x + r * 0.1f, y + r * 0.15f, r * 0.62f, r * 0.72f, R_SKIN, 0.45f, 0.75f);   /* its pink inside */
    if (torn) for (int k = 0; k < 3; k++) { cv_px((int)(x + r * 0.7f), (int)(y - r * 0.5f + k), C_CLEAR); cv_px((int)(x + r * 0.9f), (int)(y - r * 0.4f + k), C_CLEAR); }
}
static void mo_whiskers(float x, float y, float len, int dir) {  /* dim and few: bright ones sparkle when the sprite is small */
    uint8_t c = sh(R_BONE, 0.42f);
    cv_line(x + dir * 1.5f, y - 0.5f, x + dir * len, y - len * 0.22f, c);
    cv_line(x + dir * 1.5f, y + 0.8f, x + dir * len * 0.9f, y + len * 0.25f, c);
}
static void mo_eye(float x, float y, float r) {
    cv_flat(x, y, r, r, 0);
    cv_eye((int)floorf(x - r * 0.3f), (int)floorf(y - r * 0.3f), MOUSE_EYE_PIX);
}

/* in profile, facing left: 0 scurrying, 1 its legs gathered, 2 sat up preening, 3 sat up telling (the rat's rearing), 4 the bite */
static void mo_side(const MouseLook *L, int pose, uint32_t seed) {
    float gy = 62.0f, st = pose == 0 ? 1.07f : pose == 1 ? 0.94f : 1.0f;
    float rx = L->len * st, ry = L->tall / sqrtf(st);
    float bx = 5.0f + L->snout + L->head * 1.6f + rx * 0.95f, by = gy - L->leg - ry * 0.85f;
    int big = L->leg > 8.0f;
    if (pose == 2 || pose == 3) {                                   /* sat up on its haunches */
        float tell = pose == 3 ? 1.0f : 0.0f, hx0 = bx + rx * 0.15f;
        float pts[2 * 16];
        int n = mo_bez(pts, hx0 + rx * 0.4f, gy - 2, hx0 + rx * 1.1f, gy - 1, hx0 - rx * 0.3f, gy - 0.5f, hx0 - rx * 1.3f, gy - 1.5f, 16);
        mo_tail(pts, n, L->tail, 0.6f, L->tooth > 0.5f);           /* curled round it on the floor */
        float lean = tell ? 0.05f : 0.32f, tall = ry * (tell ? 1.75f : 1.5f) * (big ? 1.25f : 1.0f), base = gy - 1.0f;
        for (int k = 0; k <= 10; k++) {                            /* one body: a pear, the weight low */
            float t = k / 10.0f, w = rx * (0.62f - 0.30f * t) * (1.0f - 0.25f * t * t), cy = base - w * 0.55f - t * tall * 1.15f;
            cv_ellipse(hx0 - t * tall * lean, cy, w, w * 0.9f, L->ramp, L->lo + 0.05f * t, L->hi * (0.85f + 0.15f * t));
        }
        float topx = hx0 - tall * lean, topy = base - rx * 0.32f * 0.55f - tall * 1.15f;
        cv_ellipse(hx0 + rx * 0.05f, gy - ry * 0.75f, rx * 0.5f, ry * 0.7f, L->ramp, L->lo, L->hi * 0.75f);   /* the thigh */
        cv_ellipse(hx0 - rx * 0.12f, gy - 1.0f, 2.8f + big, 1.2f, R_SKIN, 0.35f, 0.7f);                     /* the hind foot */
        cv_ellipse(topx - rx * 0.12f, (topy + base) * 0.5f, rx * 0.2f, tall * 0.45f, L->ramp, L->lo + 0.25f, L->hi + 0.12f);   /* the belly */
        float hx = topx - L->head * 0.35f, hy = topy - L->head * (tell ? 0.7f : 0.25f);
        mo_ear(hx + L->head * (tell ? 0.3f : 0.6f), hy - L->head * 0.9f, L->ear, 0, L->scar > 0.5f);
        cv_ellipse(hx, hy, L->head, L->head * 0.88f, L->ramp, L->lo, L->hi);
        float sdx = tell ? -0.85f : -0.55f, sdy = tell ? -0.5f : 0.85f;   /* down to the paws preening, up and out telling */
        for (int k = 0; k < 6; k++) {
            float t = k / 5.0f;
            cv_ellipse(hx + sdx * (L->head * 0.6f + L->snout * t), hy + sdy * (L->head * 0.5f + L->snout * t), L->head * (0.72f - 0.5f * t), L->head * (0.6f - 0.4f * t), L->ramp, L->lo, L->hi);
        }
        float nx = hx + sdx * (L->head * 0.6f + L->snout), ny = hy + sdy * (L->head * 0.5f + L->snout);
        cv_flat(nx, ny, 1.1f, 1.1f, sh(R_SKIN, 0.8f));
        if (tell) {                                                 /* its mouth open, calling */
            cv_line(nx + 1.5f, ny + 1.5f, nx + 4.0f, ny + 1.0f, 0); cv_line(nx + 1.0f, ny + 2.0f, nx + 3.5f, ny + 2.5f, 0);
            if (L->tooth > 0) cv_line(nx + 0.5f, ny + 1.2f, nx + 0.5f, ny + 1.2f + L->tooth, sh(R_BONE, 0.9f));
            mo_whiskers(nx + 1.0f, ny, L->head * 1.2f, -1);
        } else if (L->tooth > 0) cv_line(nx + 1.2f, ny + 0.6f, nx + 1.6f, ny + 0.6f + L->tooth * 0.7f, sh(R_BONE, 0.9f));
        float px = tell ? topx - rx * 0.3f : nx + 1.5f, py = tell ? topy + tall * 0.25f : ny - 0.5f;   /* the forepaws: washing, or held */
        cv_ellipse(px, py, 1.7f + big * 0.6f, 1.4f, R_SKIN, 0.4f, 0.78f);
        cv_ellipse(px + 1.8f, py + 1.2f, 1.6f + big * 0.6f, 1.3f, R_SKIN, 0.35f, 0.72f);
        cv_capsule(px + 1.0f, py + 0.8f, topx - rx * 0.05f, topy + tall * 0.3f, 1.2f + big * 0.6f, L->ramp, L->lo, L->hi * 0.8f);   /* the foreleg */
        mo_eye(hx - L->head * 0.3f, hy - L->head * (tell ? 0.25f : 0.1f), L->eye * (tell ? 1.0f : 0.8f));
        if (L->hunch > 0) for (int k = 0; k < 7; k++) cv_px((int)(hx0 + rx * 0.45f - k * tall * lean * 0.16f), (int)(base - rx * 0.5f - k * tall * 0.16f), sh(R_SKIN, 0.55f));
        mo_fur(L, seed); cv_outline(0); return;
    }
    float lunge = pose == 4 ? 1.0f : 0.0f;
    by += lunge * 2.0f;
    {                                                               /* the tail, behind everything */
        float pts[2 * 18], tx = bx + rx * 0.92f, ty = by + ry * 0.25f, wave = pose == 1 ? -3.0f : 2.0f;
        int n = big ? mo_bez(pts, tx, ty, tx + 6, ty + 6, 60, gy - 2, 50, gy - 1.5f, 18)
                    : mo_bez(pts, tx, ty, tx + 8, ty - 6 + wave, 60 - wave, ty + 2, 62, ty - 8 - wave, 18);
        mo_tail(pts, n, L->tail, 0.55f, L->tooth > 0.5f);
    }
    float sw = pose == 0 ? 1.0f : pose == 1 ? -0.6f : 0.4f, lr = big ? 1.6f : 0.9f + L->leg * 0.12f;
    cv_capsule(bx - rx * 0.55f, by + ry * 0.4f, bx - rx * 0.55f - sw * L->leg * 0.5f - 1, gy - 0.8f, lr, L->ramp, 0.05f, 0.3f);   /* the far legs */
    cv_capsule(bx + rx * 0.45f, by + ry * 0.4f, bx + rx * 0.45f + sw * L->leg * 0.6f + 1, gy - 0.8f, lr * 1.1f, L->ramp, 0.05f, 0.3f);
    cv_ellipse(bx - rx * 0.42f, by + 0.6f + lunge, rx * 0.62f, ry * 0.86f, L->ramp, L->lo, L->hi);                      /* the shoulders */
    cv_ellipse(bx + rx * 0.3f, by - L->hunch * 0.6f, rx * 0.72f, ry * (1.0f + L->hunch * 0.06f), L->ramp, L->lo, L->hi);   /* the haunch */
    cv_ellipse(bx - rx * 0.1f, by + ry * 0.45f, rx * 0.7f, ry * 0.4f, L->ramp, L->lo + 0.18f, L->hi + 0.1f);            /* the belly */
    float fx = bx - rx * 0.6f - sw * L->leg * 0.7f - 1.5f, hx2 = bx + rx * 0.5f + sw * L->leg * 0.8f;
    cv_capsule(bx - rx * 0.6f, by + ry * 0.5f, fx, gy - 1, lr, L->ramp, L->lo, L->hi * 0.8f);                           /* the near legs */
    cv_capsule(bx + rx * 0.55f, by + ry * 0.2f, hx2, gy - 1, lr * 1.25f, L->ramp, L->lo, L->hi * 0.8f);
    cv_ellipse(fx - 0.8f, gy - 0.8f, 1.6f + big, 0.9f, R_SKIN, 0.35f, 0.7f);
    cv_ellipse(hx2 + 0.5f, gy - 0.8f, 2.4f + big, 1.0f, R_SKIN, 0.35f, 0.7f);
    if (big) for (int k = 0; k < 3; k++) { cv_px((int)fx - 3 + k, (int)gy, sh(R_BONE, 0.7f)); cv_px((int)hx2 - 1 + k, (int)gy, sh(R_BONE, 0.7f)); }   /* claws */
    if (L->hunch > 0)                                               /* the spine showing */
        for (int k = 0; k < 7; k++) {
            float a = PI_F * (0.15f + 0.7f * k / 6.0f), x = bx + rx * 0.3f - cosf(a) * rx * 0.95f, y = by - L->hunch * 0.6f - sinf(a) * ry * (1.0f + L->hunch * 0.06f);
            cv_px((int)x, (int)y, sh(R_SKIN, 0.62f)); cv_px((int)x, (int)y + 1, sh(R_SKIN, 0.4f));
        }
    if (L->ribs > 0) for (int k = 0; k < 4; k++) { float x = bx - rx * 0.35f + k * 2.5f; cv_line(x, by - ry * 0.35f, x - 1.5f, by + ry * 0.35f, (uint8_t)(L->ramp + 2)); }
    float hx = bx - rx * 0.98f - L->head * 0.25f + lunge * 2.0f, hy = by + ry * 0.12f + lunge * 3.0f;
    mo_ear(hx + L->head * 0.45f, hy - L->head * 0.95f, L->ear, 0, L->scar > 0.5f);
    cv_ellipse(hx, hy, L->head, L->head * 0.86f, L->ramp, L->lo, L->hi);
    for (int k = 0; k < 6; k++) {
        float t = k / 5.0f;
        cv_ellipse(hx - L->head * 0.55f - L->snout * t, hy + L->head * 0.25f + t * L->head * 0.25f + lunge * t * 2.0f, L->head * (0.7f - 0.48f * t), L->head * (0.58f - 0.36f * t), L->ramp, L->lo, L->hi);
    }
    float nx = hx - L->head * 0.55f - L->snout, ny = hy + L->head * 0.5f + lunge * 2.0f;
    cv_flat(nx, ny, 1.1f, 1.0f, sh(R_SKIN, 0.82f));
    if (lunge) {                                                    /* the jaws wide */
        cv_poly((const float[]){ nx + 1, ny + 1.5f, nx + L->head * 1.2f, ny + 0.5f, nx + L->head * 0.9f, ny + 5.0f }, 3, R_BLOOD, 0.1f, 0.3f, ny, 5.0f);
        cv_line(nx + 0.5f, ny + 0.5f, nx + 0.5f, ny + 1.0f + L->tooth, sh(R_BONE, 0.9f)); cv_line(nx + 1.5f, ny + 0.5f, nx + 1.5f, ny + 1.0f + L->tooth, sh(R_BONE, 0.8f));
        cv_line(nx + 2.5f, ny + 5.0f, nx + 2.0f, ny + 5.0f - L->tooth * 0.7f, sh(R_BONE, 0.8f));
    } else if (L->tooth > 0) {
        cv_line(nx + 0.6f, ny + 0.8f, nx + 0.6f, ny + 0.6f + L->tooth * 0.6f, sh(R_BONE, 0.92f));   /* the incisors, yellowing */
        cv_line(nx + 1.4f, ny + 0.8f, nx + 1.4f, ny + 0.6f + L->tooth * 0.6f, sh(R_BONE, 0.7f));
    }
    mo_whiskers(nx + 1.0f, ny - 0.5f, L->head * 1.5f, -1);
    mo_eye(hx - L->head * 0.3f, hy - L->head * 0.12f, L->eye);
    if (L->scar > 0.9f) cv_line(hx + L->head * 0.1f, hy - L->head * 0.5f, hx + L->head * 0.6f, hy + L->head * 0.2f, sh(R_SKIN, 0.7f));   /* an old scar */
    mo_fur(L, seed);
    cv_outline(0);
}

/* facing you: step 0/1 the feet in turn; bite: its head down and its jaws wide */
static void mo_front(const MouseLook *L, int step, int bite, uint32_t seed) {
    float gy = 62.0f, big = L->leg > 8.0f, sw = step ? 1.0f : -1.0f;
    float pts[2 * 16];
    int n = mo_bez(pts, 32 + L->len * 0.4f, gy - 2, 44, gy - 1 - sw, 52, gy - 1, 56, gy - 7 + sw, 16);
    mo_tail(pts, n, L->tail, 0.55f, L->tooth > 0.5f);
    float bodyy = gy - L->leg * 0.6f - L->tall * 1.1f;
    cv_ellipse(32, bodyy, L->len * 0.62f, L->tall * 1.1f, L->ramp, L->lo, L->hi);
    if (L->hunch > 0) for (int k = -3; k <= 3; k++) cv_px(32 + k, (int)(bodyy - L->tall * 1.08f + abs(k) * 0.4f), sh(R_SKIN, 0.55f));
    if (big)                                                        /* the forelegs, long, splayed */
        for (int s = -1; s <= 1; s += 2) {
            float lift = (s > 0) == (step != 0) ? 1.5f : 0.0f;
            cv_capsule(32 + s * L->len * 0.38f, bodyy + L->tall * 0.4f, 32 + s * L->len * 0.48f, gy - 1 - lift, 1.8f, L->ramp, L->lo, L->hi * 0.8f);
            cv_ellipse(32 + s * L->len * 0.5f, gy - 0.8f - lift, 2.6f, 1.0f, R_SKIN, 0.35f, 0.7f);
            for (int k = -1; k <= 1; k++) cv_px((int)(32 + s * L->len * 0.5f) + k, (int)(gy - lift), sh(R_BONE, 0.7f));
        }
    float hy = gy - L->leg * (big ? 0.95f : 0.2f) - L->head * 1.15f + (bite ? L->head * 0.45f : 0.0f);
    for (int s = -1; s <= 1; s += 2) mo_ear(32 + s * L->head * 1.05f, hy - L->head * (bite ? 0.7f : 0.85f), L->ear, 0, s > 0 && L->scar > 0.5f);
    cv_ellipse(32, hy, L->head * 1.08f, L->head * 0.95f, L->ramp, L->lo, L->hi);
    cv_ellipse(32, hy + L->head * 0.5f, L->head * 0.55f, L->head * 0.45f, L->ramp, L->lo + 0.12f, L->hi);   /* the muzzle, towards you */
    cv_flat(32, hy + L->head * 0.62f, 1.3f, 1.0f, sh(R_SKIN, 0.82f));
    if (bite) {                                                     /* the jaws: dark red within, the incisors long */
        cv_flat(32, hy + L->head * 1.05f, L->head * 0.5f, L->head * 0.38f, sh(R_BLOOD, 0.18f));
        cv_line(31, hy + L->head * 0.75f, 31, hy + L->head * 0.75f + L->tooth * 1.2f, sh(R_BONE, 0.92f));
        cv_line(32, hy + L->head * 0.75f, 32, hy + L->head * 0.75f + L->tooth * 1.2f, sh(R_BONE, 0.75f));
        cv_line(31, hy + L->head * 1.35f, 31, hy + L->head * 1.35f - L->tooth * 0.6f, sh(R_BONE, 0.7f));
        cv_line(33, hy + L->head * 1.35f, 33, hy + L->head * 1.35f - L->tooth * 0.6f, sh(R_BONE, 0.6f));
    } else if (L->tooth > 0) {
        float tl = L->tooth * (big ? 0.9f : 0.6f);
        if (big) cv_flat(32, hy + L->head * 0.95f, L->head * 0.38f, 1.6f, sh(R_BLOOD, 0.15f));   /* the lips back */
        cv_line(31, hy + L->head * 0.8f, 31, hy + L->head * 0.8f + tl, sh(R_BONE, 0.92f));
        cv_line(32, hy + L->head * 0.8f, 32, hy + L->head * 0.8f + tl, sh(R_BONE, 0.75f));
    }
    mo_whiskers(32 - 1.5f, hy + L->head * 0.62f, L->head * 1.5f, -1);
    mo_whiskers(32 + 1.5f, hy + L->head * 0.62f, L->head * 1.5f, 1);
    for (int s = -1; s <= 1; s += 2) mo_eye(32 + s * L->head * 0.46f, hy - L->head * 0.12f, L->eye);
    if (!big) for (int s = -1; s <= 1; s += 2) cv_ellipse(32 + s * L->head * 0.55f, gy - 0.9f - (s > 0 && step ? 0.8f : 0.0f), 1.6f, 1.0f, R_SKIN, 0.35f, 0.7f);
    mo_fur(L, seed);
    cv_outline(0);
}

/* from behind: no eyes */
static void mo_back(const MouseLook *L, int step, uint32_t seed) {
    float gy = 62.0f, big = L->leg > 8.0f, sw = step ? 1.0f : -1.0f;
    float bodyy = gy - L->leg * 0.55f - L->tall * 1.1f, hy = bodyy - L->tall * 1.0f;
    for (int s = -1; s <= 1; s += 2) mo_ear(32 + s * L->head * 0.95f, hy - L->head * 0.7f, L->ear * 0.95f, 1, s < 0 && L->scar > 0.5f);
    cv_ellipse(32, hy + 1, L->head * 0.95f, L->head * 0.8f, L->ramp, L->lo * 0.8f, L->hi * 0.85f);   /* the back of its head */
    if (big) for (int s = -1; s <= 1; s += 2) cv_capsule(32 + s * L->len * 0.42f, bodyy + 2, 32 + s * L->len * 0.5f, gy - 1 - ((s > 0) == (step != 0) ? 1.5f : 0.0f), 2.0f, L->ramp, L->lo, L->hi * 0.8f);
    cv_ellipse(32, bodyy, L->len * 0.7f, L->tall * 1.15f, L->ramp, L->lo, L->hi);
    if (L->hunch > 0) for (int k = 0; k < 6; k++) cv_px(32, (int)(bodyy - L->tall * 0.9f + k * 2.2f), sh(R_SKIN, 0.6f));
    for (int s = -1; s <= 1; s += 2) cv_ellipse(32 + s * L->len * 0.42f, gy - 0.9f - ((s > 0) == (step != 0) ? 1.0f : 0.0f), 2.0f + big, 1.0f, R_SKIN, 0.4f, 0.75f);   /* the soles */
    float pts[2 * 16];
    int n = mo_bez(pts, 32, bodyy + L->tall * 0.7f, 33, gy, 40 + sw * 2, gy - 0.5f, 50 + sw * 3, gy - 3, 16);
    mo_tail(pts, n, L->tail * 1.1f, 0.55f, L->tooth > 0.5f);
    mo_fur(L, seed);
    cv_outline(0);
}

/* the rat dead: on its back, its legs up and curled, its tail out along the floor */
static void mo_dead(const MouseLook *L, uint32_t seed) {
    float gy = 62.0f, bx = 34.0f, by = gy - L->tall * 0.9f;
    float pts[2 * 16];
    int n = mo_bez(pts, bx + L->len * 0.9f, by + 2, bx + L->len * 1.3f, gy - 0.5f, 58, gy, 62, gy - 2, 16);
    mo_tail(pts, n, L->tail, 0.55f, 1);
    cv_ellipse(bx, by, L->len * 1.05f, L->tall * 0.95f, L->ramp, L->lo, L->hi);
    cv_ellipse(bx - L->len * 0.1f, by - L->tall * 0.3f, L->len * 0.75f, L->tall * 0.45f, R_SKIN, 0.3f, 0.6f);   /* the bare belly, up */
    for (int k = 0; k < 4; k++) {                                   /* the legs, up and curled */
        float lx = bx - L->len * 0.6f + k * L->len * 0.4f, ly = by - L->tall * 0.7f;
        cv_capsule(lx, ly, lx + (k & 1 ? 2.0f : -1.5f), ly - 4.5f, 1.3f, L->ramp, L->lo, L->hi * 0.8f);
        cv_ellipse(lx + (k & 1 ? 2.5f : -2.0f), ly - 5.0f, 1.6f, 1.1f, R_SKIN, 0.35f, 0.7f);
    }
    float hx = bx - L->len * 1.05f - L->head * 0.4f, hy = by + 1.5f;  /* the head fallen back, its jaw open */
    cv_ellipse(hx, hy, L->head, L->head * 0.8f, L->ramp, L->lo, L->hi * 0.9f);
    cv_ellipse(hx - L->head * 0.9f, hy + L->head * 0.4f, L->head * 0.6f, L->head * 0.45f, L->ramp, L->lo, L->hi * 0.9f);
    cv_flat(hx - L->head * 1.4f, hy + L->head * 0.5f, 1.0f, 1.0f, sh(R_SKIN, 0.7f));
    cv_line(hx - L->head * 1.2f, hy + L->head * 0.9f, hx - L->head * 1.2f, hy + L->head * 0.9f + L->tooth, sh(R_BONE, 0.85f));
    cv_flat(hx - L->head * 0.2f, hy - L->head * 0.2f, 1.2f, 0.6f, 0);  /* its eye shut */
    mo_fur(L, seed);
    cv_outline(0);
}

/* a pose drawn into cv_sprite, row-major, eyes in g_eyes: stage 0-6 the small ones, 7 the rat; MP_* (crawl.h); side views turned
 * to face right */
static void mo_draw(int stage, int pose, uint32_t seed) {
    const MouseLook *L = &MOUSE_LOOK[clampi(stage, 0, MOUSE_STAGES)];
    cv_begin(cv_sprite, TS, TS);
    int side = 0;
    switch (pose) {
    case MP_RUNA: mo_side(L, 0, seed); side = 1; break;
    case MP_RUNB: mo_side(L, 1, seed); side = 1; break;
    case MP_FRONT: mo_front(L, 0, 0, seed); break;
    case MP_FRONT2: mo_front(L, 1, 0, seed); break;
    case MP_BACK: mo_back(L, 0, seed); break;
    case MP_BACK2: mo_back(L, 1, seed); break;
    case MP_PREEN: mo_side(L, 2, seed); side = 1; break;
    case MP_TELL: mo_side(L, 3, seed); side = 1; break;
    case MP_BITE: mo_front(L, 0, 1, seed); break;
    case MP_DEAD: mo_dead(L, seed); side = 1; break;
    default: break;
    }
    if (side) {
        cv_mirror();
        for (int i = 0; i < g_eyes.n; i++) g_eyes.x[i] = (int8_t)(TS - 1 - g_eyes.x[i]);
    }
}
void crawl_mouse_sprite(int stage, int pose, uint8_t *out, EyeAt *eyes) {   /* for the bestiary's page and the sheets */
    mo_draw(stage, pose, 900u + (uint32_t)stage * 31u + (uint32_t)pose);
    memcpy(out, cv_sprite, TS * TS);
    if (eyes) *eyes = g_eyes;
}

/* the charnel rat's own, once (crawl_art_init) */
static void spr_rats(void) {
    static const struct { int id, pose; } R[] = {
        { SP_RAT_A, MP_FRONT }, { SP_RAT_B, MP_FRONT2 }, { SP_RAT_SA, MP_RUNA }, { SP_RAT_SB, MP_RUNB },
        { SP_RAT_BKA, MP_BACK }, { SP_RAT_BKB, MP_BACK2 }, { SP_RAT_BITE, MP_BITE }, { SP_RAT_REAR, MP_TELL }, { SP_RAT_D, MP_DEAD } };
    for (int i = 0; i < (int)(sizeof R / sizeof R[0]); i++) { mo_draw(MOUSE_STAGES, R[i].pose, 1700u + (uint32_t)i); cv_store(R[i].id); }
}

/* ------------------------------------------------------------------ */
/* Their holes                                                         */
/* ------------------------------------------------------------------ */
/* A hole at the foot of a wall, in its middle: black within, its lip shadowed over the top and worn pale low down, a grease-mark
 * along the foot where they run; tooth marks from the fifth level, cracks and a fallen piece from the spurt; slime on its sill in
 * the Catacombs, soot round it in the Deep. Drawn into a copy of each of the run's materials, both looks (T_MHOLE). */
typedef struct { int w, h, wet, torn, rub, gnaw, ash; } MHoleLook;
static uint8_t mh_darker(uint8_t c, int k) {
    if (c == 0 || c == C_CLEAR || c >= C_WHITE || IS_FULLBRIGHT(c)) return c;
    int base = ((c - 1) / 16) * 16 + 1, st = (c - 1) % 16;
    return (uint8_t)(base + (st > k ? st - k : 0));
}
static void mh_draw(uint8_t *tex, const MHoleLook *H, int cx, uint32_t seed) {   /* tex column-major, as the textures are */
    #define MT(x, y) tex[(x) * TS + (y)]
    float hw = H->w * 0.5f, hh = (float)H->h, mn = fminf(hw, hh);
    if (H->rub)
        for (int y = TS - 5; y < TS; y++)
            for (int x = 0; x < TS; x++) {
                float d = fabsf((float)(x - cx)) - hw; if (d < 0) d = 0;
                float k = (1.0f - d / (14.0f + H->rub * 10.0f)) * (1.0f - (TS - 1 - y) / 5.0f) * (0.6f + 0.4f * h01(x, y, seed));
                if (k > 0.25f) MT(x, y) = mh_darker(MT(x, y), k > 0.6f ? 3 : k > 0.4f ? 2 : 1);
            }
    if (H->torn)
        for (int c = 0; c < 2 + (H->torn > 1); c++) {
            float a = PI_F * (0.3f + 0.4f * h01(c, 1, seed)), x = cx + cosf(a) * hw * 0.95f, y = 63.5f - sinf(a) * hh * 0.95f;
            for (int k = 0; k < 4 + H->torn * 3; k++) {
                a += (h01(c, k, seed + 3) - 0.5f) * 0.5f;
                x += cosf(a); y -= sinf(a);
                if (x < 0 || x > 63 || y < 0 || y > 63) break;
                MT((int)x, (int)y) = mh_darker(MT((int)x, (int)y), 5);
            }
        }
    for (int y = 0; y < TS; y++)
        for (int x = 0; x < TS; x++) {
            float nx = (x + 0.5f - cx) / hw, ny = (63.5f - y) / hh;
            if (ny < -0.01f) continue;
            float ang = atan2f(fmaxf(ny, 0.0f), nx), r = sqrtf(nx * nx + ny * ny);
            float amp = H->torn ? 0.10f : H->gnaw ? 0.06f : 0.03f;
            float edge = 1.0f + amp * (vnoise(ang * 4.0f, 0.5f, 64, seed) - 0.5f) * 2.0f;
            float d = (r - edge) * mn;
            uint8_t c = MT(x, y);
            if (d < -1.0f) MT(x, y) = 0;
            else if (d < 0.0f) MT(x, y) = ny < 0.35f ? (uint8_t)(R_STONE + 2) : mh_darker(c, 11);
            else if (d < 1.2f + 0.4f * (H->gnaw > 1))
                MT(x, y) = ny > 0.5f ? mh_darker(c, 3) : H->gnaw && c && c < C_WHITE && !IS_FULLBRIGHT(c) ? (uint8_t)(c + 1 < ((c - 1) / 16) * 16 + 16 ? c + 1 : ((c - 1) / 16) * 16 + 16) : c;
        }
    if (H->gnaw)
        for (int g = 0; g < 2 + H->gnaw; g++) {
            float a = PI_F * (0.15f + 0.7f * h01(g, 11, seed)), rr = 1.0f + 1.6f / mn;
            for (int k = -1; k <= 1; k++) {
                float b = a + k * 0.9f / mn;
                int xx = (int)(cx + cosf(b) * hw * rr), yy = (int)(63.5f - sinf(b) * hh * rr);
                if (xx >= 0 && xx < TS && yy >= 0 && yy < TS) MT(xx, yy) = mh_darker(MT(xx, yy), k ? 3 : 5);
            }
        }
    if (H->wet)
        for (int x = (int)(cx - hw - 3); x <= (int)(cx + hw + 3); x++) {
            if (x < 0 || x > 63) continue;
            int in = fabsf(x + 0.5f - cx) < hw - 1;
            MT(x, 63) = (uint8_t)(R_MOSS + (in ? 2 : 4) + (int)(3 * h01(x, 0, seed)));
            if (!in && h01(x, 1, seed) > 0.5f) MT(x, 62) = (uint8_t)(R_MOSS + 3 + (int)(3 * h01(x, 2, seed)));
        }
    if (H->ash)
        for (int y = (int)(63 - hh * 1.6f); y < TS; y++)
            for (int x = (int)(cx - hw * 1.6f); x <= (int)(cx + hw * 1.6f); x++) {
                if (x < 0 || x > 63 || y < 0) continue;
                float nx = (x + 0.5f - cx) / (hw * 1.6f), ny = (63.5f - y) / (hh * 1.6f), r = nx * nx + ny * ny;
                if (r < 1.0f && r > 0.5f && h01(x, y, seed + 8) > 0.35f) MT(x, y) = mh_darker(MT(x, y), 3);
            }
    if (H->torn > 1)
        for (int k = 0; k < 6; k++) {
            int x = (int)(cx + hw + 1 + h01(k, 21, seed) * 6), y = 63 - (int)(h01(k, 22, seed) * 2);
            if (x < TS) MT(x, y) = (uint8_t)(R_STONE + 8 + (int)(4 * h01(k, 23, seed)));
        }
    #undef MT
}

/* the level's: its small ones' six sprites at its size, and the holes on its run's materials (after crawl_art_run) */
void crawl_art_mice(int level) {
    int st = crawl_mouse_stage(level), hs = crawl_mhole_stage(level), set = crawl_set(level);
#ifdef CRAWL_DEBUG
    if (getenv("CRAWL_MICESTAGE")) st = hs = clampi(atoi(getenv("CRAWL_MICESTAGE")), 0, MOUSE_STAGES - 1);
#endif
    static const struct { int id, pose; } S[] = {
        { SP_MOUSE_SA, MP_RUNA }, { SP_MOUSE_SB, MP_RUNB }, { SP_MOUSE_F, MP_FRONT }, { SP_MOUSE_BK, MP_BACK },
        { SP_MOUSE_PREEN, MP_PREEN }, { SP_MOUSE_TELL, MP_TELL } };
    for (int i = 0; i < (int)(sizeof S / sizeof S[0]); i++) { mo_draw(st, S[i].pose, 900u + (uint32_t)st * 31u + (uint32_t)S[i].pose); cv_store(S[i].id); }
    int w, h;
    crawl_mhole_size(hs, &w, &h);
    MHoleLook H = { w, h, set == 1, hs >= 6 ? 1 + (level >= 15) : 0, hs >= 1 ? 1 + (hs >= 4) : 0, hs >= 2 ? 1 + (hs >= 4) + (hs >= 6) : 0, set == 3 };
    for (int m = 0; m < NMAT; m++)
        for (int v = 0; v < NVAR; v++) {
            memcpy(crawl_tex[T_MHOLE + m * NVAR + v], crawl_tex[T_MATV(m, v)], TS * TS);
            mh_draw(crawl_tex[T_MHOLE + m * NVAR + v], &H, TS / 2, 300u + (uint32_t)level * 7u + (uint32_t)(m * NVAR + v));
        }
}
