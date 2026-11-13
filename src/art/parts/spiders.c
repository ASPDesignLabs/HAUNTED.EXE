/* A part of crawl_art.c (docs/PLAN-tidy.md): it is #included there in its place and never compiled on its own.
 * The hunting spider; the Wandering Spider; the brood mother, her sacs and spiderlings. */

/* a hairy abdomen: fur in short strokes, paler at its rim, bristles; mark 0 a dark stripe down it and chevrons either
 * side, 1 nothing (the side view draws its own) */
static void hunter_abdomen(float cx, float cy, float rx, float ry, int ramp, int mark, uint32_t seed) {
    for (int y = (int)(cy - ry) - 1; y <= (int)(cy + ry) + 1; y++)
        for (int x = (int)(cx - rx) - 1; x <= (int)(cx + rx) + 1; x++) {
            float nx = (x + 0.5f - cx) / rx, ny = (y + 0.5f - cy) / ry, d = nx * nx + ny * ny;
            if (d > 1.0f) continue;
            float v = 0.12f + 0.4f * (1.0f - d) * (0.65f + 0.35f * (-ny * 0.8f + 0.4f)) + (h01(x, y, seed) - 0.5f) * 0.16f;
            if (h01(x / 2, y, seed + 1) > 0.8f) v += 0.12f;            /* a lighter stroke of hair */
            if (mark == 0 && fabsf(nx) < 0.12f) v -= 0.18f;            /* the stripe down its back */
            if (mark == 0 && fabsf(fabsf(nx) - 0.45f + ny * 0.35f) < 0.08f && ny > -0.6f) v -= 0.15f;   /* chevrons */
            cv_px(x, y, sh(ramp, v));
        }
    for (int q = 0; q < 36; q++) {                                   /* bristles round its rim */
        float an = TAU_F * q / 36.0f + h01(q, 3, seed) * 0.15f, l = 1.2f + 1.8f * h01(q, 4, seed);
        float ex = cx + cosf(an) * rx, ey = cy + sinf(an) * ry;
        cv_line(ex, ey, ex + cosf(an) * l, ey + sinf(an) * l, sh(ramp, 0.42f));
    }
}

/* The hunting spider: a big brown wolf spider, hairy, low and wide, its two great front eyes shining back at you, its
 * legs banded dark and bristling at the knees, fangs with venom on them. Body R_WOOD (PF_HIDE). pose 0-1 running,
 * 2 leaping, 3 dead. */
static void spr_hunter(int id, int frame) {
    cv_begin(cv_sprite, TS, TS);
    float cx = 32, cy = frame == 3 ? 54 : 44;
    if (frame != 2 && frame != 3) cv_flat(32, 62, 24, 2.0f, R_WARM + 1);
    if (frame == 3) {                                                /* dead on its back, its legs curled over it */
        hunter_abdomen(cx, cy - 2, 10.0f, 6.5f, R_WOOD, 0, 481);
        for (int s = -1; s <= 1; s += 2)
            for (int l = 0; l < 4; l++)
                chitin_leg(cx + s * (3 + l), cy - 5, cx + s * (8 + l * 2), cy - 10 - l * 2, cx + s * (4 + l), cy - 16 - l * 1.5f, 1.4f, R_WOOD, 0.12f, 0.55f, R_IRON, 483 + (uint32_t)(l * 2 + s + 1));
        cv_ellipse(cx, cy - 6, 7.0f, 4.5f, R_WOOD, 0.12f, 0.5f);
        cv_outline(0);
        cv_store(id);
        return;
    }
    float ay = frame == 2 ? cy - 6 : cy - 5;
    int back = g_back;                                              /* 1.0.0b18: from behind, the abdomen nearest, over the head */
    if (!back) hunter_abdomen(cx, ay, 10.5f, 7.5f, R_WOOD, 0, 485);   /* the abdomen, behind and above */
    for (int s = -1; s <= 1; s += 2)
        for (int l = 3; l >= 0; l--) {                              /* legs: up into a high knee, then out and down */
            float k = (frame == 1 ? (l & 1 ? 1.0f : -1.0f) : (frame == 0 ? (l & 1 ? -1.0f : 1.0f) : 0.0f)) * s;
            float kx = cx + s * (5 + l * 1.2f), ky = cy - 2 + l * 2.0f;
            float mx = cx + s * (14 + l * 3.0f), my = cy - 14 + l * 3.0f + k * 2.0f;
            float fx = cx + s * (20 + l * 4.0f) + k, fy = 61 - (l == 0 ? 2.0f : 0.0f);
            if (frame == 2) { mx = cx + s * (12 + l * 2); my = cy - 16 - (3 - l) * 2.0f; fx = cx + s * (16 + l * 4); fy = cy - 24 + l * 6.0f; }
            chitin_leg(kx, ky, mx, my, fx, fy, 1.5f, R_WOOD, 0.12f, 0.6f, R_IRON, 487 + (uint32_t)(l * 2 + s + 1));
        }
    if (back) {                                                     /* the fore-body beyond it, then the abdomen near and large */
        cv_ellipse(cx, cy - 4, 7.0f, 5.0f, R_WOOD, 0.1f, 0.45f);
        hunter_abdomen(cx, cy + 2, 12.0f, 9.0f, R_WOOD, 0, 485);
        for (int k = -1; k <= 1; k += 2) cv_px((int)cx + k, (int)cy + 10, sh(R_WOOD, 0.1f));   /* the spinnerets */
        cv_px((int)cx, (int)cy + 11, sh(R_WOOD, 0.3f));
        cv_outline(0);
        cv_store(id);
        return;
    }
    cv_ellipse(cx, cy + 4, 8.0f, 6.0f, R_WOOD, 0.15f, 0.62f);       /* the head and fore-body */
    for (int y = (int)cy - 2; y < (int)cy + 11; y++)                /* fur on it */
        for (int x = (int)cx - 9; x < (int)cx + 9; x++) {
            uint8_t c = cv_sprite[y * TS + x];
            if (c >= R_WOOD && c < R_WOOD + 16 && h01(x, y, 489) > 0.8f) cv_px(x, y, (uint8_t)(c + 3 > R_WOOD + 15 ? R_WOOD + 15 : c + 3));
        }
    grid_stamp(HUNTER_HEAD, 15, 10, (int)cx - 7, (int)cy - 1, R_WOOD, R_WOOD, (uint8_t)(R_FLAME + 15), 0);
    float open = frame == 2 ? 3.0f : 1.0f;
    for (int k = -1; k <= 1; k += 2) {                              /* palps, then fangs with a drop on each */
        cv_capsule(cx + k * 6.0f, cy + 7, cx + k * 7.0f, cy + 12, 1.2f, R_WOOD, 0.15f, 0.5f);
        cv_ellipse(cx + k * 2.0f, cy + 9, 1.7f, 1.8f, R_WOOD, 0.1f, 0.35f);
        float tx = cx + k * (2.0f + open * 0.5f), ty = cy + 11 + open;
        cv_line(cx + k * 2.0f, cy + 9, tx, ty, sh(R_IRON, 0.35f)); cv_px((int)tx, (int)ty, sh(R_BONE, 0.85f));
        cv_px((int)tx, (int)ty + 1, (uint8_t)(R_MOSS + 14)); cv_px((int)tx, (int)ty + 2, (uint8_t)(R_MOSS + 9));
    }
    cv_outline(0);
    cv_store(id);
}

/* the hunting spider's dressing by rank: II silk on it and mould, III frost and a leg's end snapped, IV old blood on
 * its fangs and a bald scar torn in its fur, V charred and the pale skull on its back, gilded gold in its fur */
static void hunter_dress(int id, int frame) {
    float cx = 32, cy = 44, ay = frame == 2 ? cy - 6 : cy - 5;
    for (int r = RK_II; r < NRANK; r++) {
        g_dress_ramp2 = R_IRON;                                     /* its bands and fangs */
        dress_begin(id);
        if (r == RK_II) {
            kit_moss(0.25f, 0, 491);
            for (int q = 0; q < 4; q++) {
                float x0 = 12 + 40 * h01(q, 1, 493), y0 = 30 + 10 * h01(q, 2, 493), x1 = x0 + (h01(q, 3, 493) - 0.5f) * 14, y1 = y0 + 8 + 8 * h01(q, 4, 493);
                for (int k = 0; k <= 12; k++) { float t = k / 12.0f; int x = (int)(x0 + (x1 - x0) * t), y = (int)(y0 + (y1 - y0) * t + sinf(t * PI_F) * 2.0f); if (base_on(x, y) || (k & 1)) cv_px(x, y, sh(R_GHOST, 0.65f)); }
            }
        }
        if (r == RK_III) {
            kit_frost(0.65f, 495);
            float x = cx - 5, y = ay - 6;                            /* its back cracked */
            for (int k = 0; k < 11; k++) { cv_px((int)x, (int)y, 0); cv_px((int)x + 1, (int)y, sh(R_WOOD, 0.75f)); x += h01(k, 1, 497) < 0.5f ? 1.0f : 0.0f; y += 1.0f; }
        }
        if (r >= RK_IV && r != RK_GILDED) {
            kit_blood(cx - 2, cy + 10, 5, 2, 499); kit_blood(cx + 2, cy + 10, 5, 2, 501);
            for (int y = 0; y < 5; y++) for (int x = 0; x < 7 - y; x++) cv_px((int)cx + 3 + x, (int)ay - 3 + y, (x + y) & 1 ? (uint8_t)(R_SKIN + 6) : (uint8_t)(R_BLOOD + 5));   /* torn bald */
        }
        if (r == RK_V) {
            g_dress_ramp2 = R_BONE;
            kit_char(0.3f, 503);
            cv_flat(cx + 0.5f, ay - 0.2f, 6.2f, 6.6f, 0);
            grid_stamp(SKULL_SMALL, 9, 10, (int)cx - 4, (int)ay - 5, R_BONE, R_BONE, 0, 0);
        }
        if (r == RK_GILDED) kit_gold(R_WOOD, 0.55f, 505);
        dress_store(id, r, R_WOOD);
    }
    g_dress_ramp2 = -1;
}

/* ---- The Wandering Spider's own (docs/InitialArtPass.md, part three; the owner chose an old matriarch) ---- */

/* her face: two great burning eyes in black sockets under a brow of fur, six milky blind ones ringed round them (two
 * above, four small below, the last scarred over); fur in R_WARM (hex), the milky eyes R_GHOST (g-v), fire A-P */
static const char *const MATRIARCH_FACE[12] = {
    ".....234566654432....", "...2457889a9887765...", ".0rs0788999887760qr0.", ".0pq0008898800060op0.",
    "24000DGD00800CFC00021", "2567GP@KG776FN@JF3321", "2456JNMKG665IMLJF2211", "2345GKJGD554FJIFC2211",
    ".2300CDC00400BCb00...", "..233000333220c011...", "...220s0r020r0c01....", "....111111111b11.....",
};

/* grizzled fur over an ellipse: dark at the roots, lit from the upper left, long hairs gone grey-white lying down and
 * out over it (grey: how many; fewer in the shade), and a shaggy fringe round its rim */
static void grizzled(float cx, float cy, float rx, float ry, float lo, float hi, float grey, uint32_t seed) {
    for (int y = (int)(cy - ry) - 1; y <= (int)(cy + ry) + 1; y++)
        for (int x = (int)(cx - rx) - 1; x <= (int)(cx + rx) + 1; x++) {
            float nx = (x + 0.5f - cx) / rx, ny = (y + 0.5f - cy) / ry, d = nx * nx + ny * ny;
            if (d > 1.0f) continue;
            float v = lo + (hi - lo) * lit(nx, ny, sqrtf(1.0f - d)) + (h01(x, y, seed) - 0.5f) * 0.1f;
            if (h01(x / 2, y, seed + 1) > 0.78f) v += 0.08f;           /* the lie of the hair */
            int gr = h01(x, y / 2, seed + 7) < grey * (0.25f + 1.2f * (v - lo) / (hi - lo + 0.01f));   /* gone grey */
            cv_px(x, y, gr ? sh(R_IRON, v + 0.18f) : sh(R_WARM, v));
        }
    int n = (int)(rx * ry * grey);
    for (int i = 0; i < n; i++) {                                     /* grey hairs, down and out from where they grow */
        float a = h01(i, 1, seed + 2) * TAU_F, r = sqrtf(h01(i, 2, seed + 2)) * 0.9f;
        float nx = cosf(a) * r, ny = sinf(a) * r, x = cx + nx * rx, y = cy + ny * ry;
        float light = lit(nx, ny, sqrtf(fmaxf(0.0f, 1.0f - nx * nx - ny * ny)));
        if (h01(i, 4, seed + 2) > 0.3f + light) continue;
        float dx = nx * 0.6f, dy = 0.8f + ny * 0.2f, L = 1.5f + 2.0f * h01(i, 3, seed + 2);
        for (int k = 0; k <= (int)L; k++)
            cv_px((int)(x + dx * k), (int)(y + dy * k), k == (int)L ? sh(R_IRON, 0.68f + 0.25f * light) : sh(R_WARM, 0.4f + 0.3f * light));
    }
    int nf = (int)((rx + ry) * 2.0f);
    for (int q = 0; q < nf; q++) {                                    /* the fringe round its rim */
        float an = TAU_F * (float)q / (float)nf + h01(q, 5, seed) * 0.1f, l = 1.0f + 2.5f * h01(q, 6, seed);
        float ex = cx + cosf(an) * rx, ey = cy + sinf(an) * ry, dx = cosf(an) * 0.7f, dy = sinf(an) * 0.7f + 0.5f;
        for (int k = 1; k <= (int)l; k++) cv_px((int)(ex + dx * k), (int)(ey + dy * k), (q % 3) == 0 ? sh(R_IRON, 0.6f) : sh(R_WARM, 0.22f));
    }
}

/* one of her legs in three joints, thinning as they go: the thigh up to the knee, the shin bowing out, the foot; banded
 * dark at each joint, grey hairs hanging off the thigh, a bone-pale spur curving out past the knee and smaller ones down
 * the shin; s the side; broken: the shin snapped off short, a jagged end */
static void spurred_leg(float ax, float ay, float kx, float ky, float fx, float fy, float r, int s, int broken, uint32_t seed) {
    float mx = kx + (fx - kx) * 0.55f + (float)s * 2.2f, my = ky + (fy - ky) * 0.55f;   /* the second joint, bowed out */
    if (broken) { fx = kx + (mx - kx) * 0.6f; fy = ky + (my - ky) * 0.6f; mx = fx; my = fy; }
    cv_capsule(ax, ay, kx, ky, r + 0.7f, R_WARM, 0.0f, 0.0f);       /* a dark edge, to part it from the legs behind */
    cv_capsule(kx, ky, mx, my, r * 0.62f + 0.6f, R_WARM, 0.0f, 0.0f);
    cv_capsule(ax, ay, kx, ky, r, R_WARM, 0.07f, 0.52f);
    cv_capsule(kx, ky, mx, my, r * 0.62f, R_WARM, 0.07f, 0.5f);
    if (!broken) cv_capsule(mx, my, fx, fy, r * 0.4f, R_WARM, 0.06f, 0.44f);
    cv_ellipse(kx, ky, r * 1.15f, r * 1.15f, R_WARM, 0.1f, 0.62f);   /* the knee, knotted */
    float S[3][5] = { { ax, ay, kx, ky, r }, { kx, ky, mx, my, r * 0.62f }, { mx, my, fx, fy, r * 0.4f } };
    for (int g = 0; g < 3 - broken; g++) {                           /* bristles all along it, standing off its edges */
        float vx = S[g][2] - S[g][0], vy = S[g][3] - S[g][1], vl = hypotf(vx, vy) + 0.01f, nx = -vy / vl, ny = vx / vl;
        int e = nx * (float)s > 0.0f || (fabsf(nx) < 0.3f && ny < 0.0f) ? 1 : -1;   /* the side away from her */
        for (float d = 1.5f; d < vl - 0.5f; d += 3.2f) {
                float bx = S[g][0] + vx / vl * d + nx * (float)e * (S[g][4] + 0.3f), by = S[g][1] + vy / vl * d + ny * (float)e * (S[g][4] + 0.3f);
                float l = 0.8f + 1.2f * h01((int)(d * 3.0f), e + g * 4, seed), grey = h01((int)(d * 5.0f), e + g * 4, seed + 1) < 0.35f;
                cv_line(bx, by, bx + nx * (float)e * l + vx / vl * 0.6f, by + ny * (float)e * l + 0.7f, grey ? sh(R_IRON, 0.55f) : sh(R_WARM, 0.16f));
        }
    }
    for (int q = 0; q < 6; q++) {                                    /* grey in the hair of the thigh */
        float t = h01(q, 7, seed), x = ax + (kx - ax) * t, y = ay + (ky - ay) * t;
        cv_px((int)(x - (float)s * 0.6f), (int)y, sh(R_IRON, 0.55f + 0.2f * h01(q, 8, seed)));
    }
    if (broken) for (int q = -1; q <= 1; q++) cv_line(fx, fy, fx + (float)q * 1.2f + (float)s * 0.6f, fy + 1.5f + (float)(q & 1), sh(R_BONE, 0.45f));   /* splintered */
    float J[3][2] = { { kx, ky }, { mx, my }, { (ax + kx) * 0.5f, (ay + ky) * 0.5f } };
    for (int j = 0; j < 3 - broken; j++)                             /* dark bands at the joints, and one round the thigh */
        for (int q = -2; q <= 2; q++) cv_px((int)(J[j][0] + (float)q * 0.7f), (int)(J[j][1] + (float)q * 0.15f), (uint8_t)(R_IRON + 1 + (q == 0)));
    for (int q = 0; q < 5; q++) {                                    /* grey hairs hanging off the thigh and shin */
        float t = 0.2f + 0.2f * (float)q, x, y;
        if (q < 3) { x = ax + (kx - ax) * t * 1.4f; y = ay + (ky - ay) * t * 1.4f; } else { x = kx + (mx - kx) * (t - 0.5f); y = ky + (my - ky) * (t - 0.5f); }
        float l = 2.0f + 2.5f * h01(q, 1, seed);
        cv_line(x + (float)s * r * 0.8f, y, x + (float)s * (r * 0.8f + l * 0.5f), y + l, sh(R_IRON, 0.5f + 0.2f * (float)(q & 1)));
    }
    float ux = kx - ax, uy = ky - ay, L = hypotf(ux, uy);            /* the spur, on past the knee and curving up */
    ux /= L; uy /= L;
    for (int k = 0; k <= 5; k++) {
        float t = (float)k / 5.0f, bx = kx + ux * 1.3f * (float)k + (float)s * 0.5f * (float)k, by = ky + uy * 1.3f * (float)k - 0.22f * (float)(k * k);
        cv_px((int)bx, (int)by, sh(R_BONE, 0.3f + 0.5f * t));
        if (k < 3) { cv_px((int)bx + s, (int)by, sh(R_BONE, 0.2f + 0.1f * t)); cv_px((int)bx, (int)by + 1, sh(R_BONE, 0.12f)); }   /* thick at its root */
    }
    if (!broken) {                                                   /* a smaller one down the shin, pointing back */
        float bx = kx + (mx - kx) * 0.45f, by = ky + (my - ky) * 0.45f;
        cv_line(bx, by, bx + (float)s * 2.5f, by - 1.5f, sh(R_BONE, 0.35f));
        cv_px((int)(bx + (float)s * 2.5f), (int)(by - 1.5f), sh(R_BONE, 0.55f));
    }
}

/* a husk of her prey wound in silk, hung at (x, y) on a thread from (tx, ty): what 0 a small skull showing at its top,
 * 1 a bat's wing out of its side, 2 a rat's tail curling from its foot */
static void silk_husk(float x, float y, float tx, float ty, int what, int s) {
    cv_line(tx, ty, x, y - 4.0f, sh(R_IRON, 0.62f));
    if (what == 1) {                                                 /* the wing, behind it */
        float p[8] = { x + (float)s * 1.0f, y - 2.0f, x + (float)s * 6.5f, y - 5.5f, x + (float)s * 5.0f, y - 0.5f, x + (float)s * 1.0f, y + 1.0f };
        cv_poly(p, 4, R_IRON, 0.05f, 0.2f, y - 5.5f, 7.0f);
        cv_line(x + (float)s * 1.0f, y - 2.0f, x + (float)s * 6.5f, y - 5.5f, sh(R_IRON, 0.35f));
    }
    for (int j = -6; j <= 6; j++) {                                  /* a spindle, pointed at both ends */
        float u = (float)j / 6.5f, w = 2.9f * powf(fmaxf(0.0f, 1.0f - u * u), 0.8f);
        for (int i = (int)floorf(x - w); i <= (int)ceilf(x + w); i++) {
            float nx = ((float)i + 0.5f - x) / (w + 0.01f);
            if (fabsf(nx) > 1.0f) continue;
            float v = 0.3f + 0.45f * lit(nx, u, sqrtf(fmaxf(0.0f, 1.0f - nx * nx))), band = fmodf((float)j + 0.4f * nx * 3.0f + 20.0f, 2.6f);
            cv_px(i, (int)y + j, sh(R_BONE, band < 0.8f ? v - 0.2f : v));   /* wound round, slantwise */
        }
    }
    if (what == 0) {                                                 /* a small skull, its crown and sockets */
        cv_px((int)x - 1, (int)y - 4, sh(R_BONE, 0.8f)); cv_px((int)x, (int)y - 5, sh(R_BONE, 0.85f)); cv_px((int)x + 1, (int)y - 4, sh(R_BONE, 0.7f));
        cv_px((int)x - 1, (int)y - 3, sh(R_BONE, 0.05f)); cv_px((int)x + 1, (int)y - 3, sh(R_BONE, 0.05f));
    }
    if (what == 2)                                                   /* a rat's tail */
        for (int k = 0; k < 6; k++) cv_px((int)(x + sinf((float)k * 0.9f) * 1.5f), (int)(y + 4.0f + (float)k), sh(R_SKIN, 0.3f - 0.03f * (float)k));
}

/* a strand of her torn web trailing from (x0, y0) towards (x1, y1), sagging; torn: how much of it is gone (it frays at
 * the break). Drawn after the outline, and only where nothing of her is: it is behind her, and thin */
static void web_trail(float x0, float y0, float x1, float y1, float sag, float torn) {
    int n = (int)hypotf(x1 - x0, y1 - y0) + 1, end = (int)((float)n * (1.0f - torn));
    for (int i = 0; i <= end; i++) {
        float t = (float)i / (float)n;
        int x = (int)(x0 + (x1 - x0) * t), y = (int)(y0 + (y1 - y0) * t + sag * 4.0f * t * (1.0f - t));
        if (i > end - 4 && (i & 1)) continue;
        if ((unsigned)x < (unsigned)TS && (unsigned)y < (unsigned)TS && cv_sprite[y * TS + x] == C_CLEAR) cv_sprite[y * TS + x] = sh(R_IRON, 0.62f - 0.18f * t);
    }
}

/* her leg l (0 the front pair to 3 the hind) on side s in a frame of spr_bspider; hy the head's middle. The hind pair is
 * drawn behind her abdomen, the rest before it; her right hind leg is snapped */
static void matriarch_leg(int frame, int s, int l, float hy) {
    static const float KX[4] = { 14.5f, 21.0f, 27.0f, 19.0f }, KY[4] = { 27, 8, 23, 12 }, FX[4] = { 12.5f, 26.0f, 27.5f, 27.5f }, FY[4] = { 62, 62, 61, 57 };
    static const float LKX[4] = { 15, 19, 23, 21 }, LKY[4] = { 26, 9, 10, 16 }, LFX[4] = { 9, 24, 31, 29 }, LFY[4] = { 6, 1, 8, 40 };
    static const float RKX[2] = { 15, 19 }, RKY[2] = { 22, 8 }, RFX[2] = { 10, 26 }, RFY[2] = { 2, 1 };
    float k = (frame == 1 ? (l & 1 ? 1.0f : -1.0f) : (frame == 0 ? (l & 1 ? -1.0f : 1.0f) : 0.0f)) * (float)s;
    float ax = 32 + s * (8.5f + l * 0.6f), ay = hy + 1.0f - l * 1.6f;
    float kx = 32 + s * KX[l], ky = KY[l] + k * 2.0f, fx = 32 + s * FX[l] + k, fy = FY[l];
    if (frame == 2) { kx = 32 + s * LKX[l]; ky = LKY[l]; fx = 32 + s * LFX[l]; fy = LFY[l]; }   /* the leap: the front legs flung up to take hold */
    if (frame == 3 && l < 2) { kx = 32 + s * RKX[l]; ky = RKY[l]; fx = 32 + s * RFX[l]; fy = RFY[l]; }   /* rearing: the front two pairs raised */
    spurred_leg(ax, ay, kx, ky, fx, fy, l == 1 || l == 2 ? 1.75f : 1.6f, s, s > 0 && l == 3, 711 + (uint32_t)(l * 2 + s + 1));
}

/* The Wandering Spider: the hunting spider's kind grown huge and grizzled, its fur gone grey-white, a cluster of milky
 * blind eyes round two great burning ones, its shell scarred, its legs spurred, torn web trailing from it with the husks
 * of its prey in it, and the skull on its back in raised bone. frame 0-1 walking (the renderer turns them over on the
 * ceiling), 2 the leap, 3 rearing to spit its web, 4 dead on its back. Fur R_WARM, drawn as it is (the boss has no rank). */
static void spr_bspider(int id, int frame) {
    cv_begin(cv_sprite, TS, TS);
    if (frame == 4) {                                                /* dead on her back, her legs curled over her */
        grizzled(32, 52, 17.0f, 8.5f, 0.12f, 0.55f, 0.3f, 741);
        for (int s = -1; s <= 1; s += 2)
            for (int l = 0; l < 4; l++)
                spurred_leg(32 + s * (5 + l * 2.5f), 47, 32 + s * (13 + l * 3.0f), 34 - l * 3.0f, 32 + s * (6 + l * 2.0f), 27 - l * 2.0f, 1.4f, s, s > 0 && l == 3, 743 + (uint32_t)(l * 2 + s + 1));
        grizzled(32, 46, 10.0f, 5.5f, 0.08f, 0.45f, 0.3f, 745);
        for (int k = -1; k <= 1; k += 2) cv_flat(32 + k * 2.5f, 45, 1.6f, 1.2f, (uint8_t)(R_BLOOD + 4));   /* her eyes gone out */
        cv_outline(0);
        web_trail(18, 56, 2, 62, 0.5f, 0.0f); web_trail(46, 57, 63, 61, 0.4f, 0.1f); web_trail(40, 59, 52, 63, 0.2f, 0.0f);
        cv_store(id);
        return;
    }
    float hy = frame == 3 ? 41.0f : 47.0f, ay = frame == 3 ? 21.0f : 24.0f;   /* the head's middle and the abdomen's */
    if (frame != 2) cv_flat(32, 62, 21, 2.0f, R_WARM + 1);         /* her shadow, under her body (the side legs reach past it) */
    for (int sd = -1; sd <= 1; sd += 2) matriarch_leg(frame, sd, 3, hy);   /* the hind pair, behind her */
    grizzled(32, ay, 15.5f, 12.5f, 0.05f, 0.5f, 0.55f, 701);         /* the abdomen */
    for (int k = 0; k < 3; k++)                                      /* claws raked across it once, long healed */
        for (int i = 0; i < 9; i++) {
            int x = 38 + i, y = (int)(ay - 9.0f) + k * 3 + (i * 2) / 3;
            cv_px(x, y, sh(R_WARM, 0.78f - 0.04f * (float)i)); cv_px(x, y + 1, sh(R_WARM, 0.02f));
        }
    for (int i = 0; i < 10; i++) {                                    /* and a gouge down the left, the shell bare about it */
        int x = 20 + i / 3, y = (int)ay - 3 + i;
        cv_px(x - 1, y, sh(R_IRON, 0.25f)); cv_px(x, y, sh(R_WARM, 0.8f)); cv_px(x + 1, y, sh(R_IRON, 0.12f));
    }
    for (int y = 0; y < SKULL_H; y++)                                 /* the skull on its back, raised: its shadow first */
        for (int x = 0; x < SKULL_W; x++)
            if (SKULL_FRONT[y][x] != '.') cv_px(32 - SKULL_W / 2 + x + 1, (int)ay - 9 + y + 1, sh(R_WARM, 0.0f));
    skull_stamp(32, ay - 1.0f, 0, 0, 0);
    for (int l = 2; l >= 0; l--)                                     /* the rest before her, the back ones first */
        for (int sd = -1; sd <= 1; sd += 2) matriarch_leg(frame, sd, l, hy);
    for (int y = 0; y < TS; y++)                                     /* the fur's grain over legs and body */
        for (int x = 0; x < TS; x++) {
            uint8_t *c = &cv_sprite[y * TS + x];
            if (*c <= R_WARM + 1 || *c >= R_WARM + 16) continue;
            float n = h01(x, y, 731);
            if (n < 0.15f) *c = (uint8_t)(*c - 1 - (n < 0.05f)); else if (n > 0.9f && *c < R_WARM + 14) *c = (uint8_t)(*c + 1);
        }
    if (frame != 3) {                                                /* the husks of her prey, hung in the web she trails */
        silk_husk(7.0f, 34.0f, 6.0f, 20.0f, 0, -1);
        silk_husk(57.5f, 38.0f, 57.0f, 24.0f, 1, 1);
    }
    grizzled(32, hy, 12.5f, 9.5f, 0.06f, 0.5f, 0.4f, 721);          /* the head */
    grid_stamp(MATRIARCH_FACE, 21, 12, 22, (int)hy - 8, R_WARM, R_GHOST, (uint8_t)(R_FLAME + 15), 0);
    float open = frame == 2 || frame == 3 ? 1.0f : 0.0f;
    for (int k = -1; k <= 1; k += 2) {                              /* palps, her jaws' great bulges, and the fangs */
        cv_capsule(32 + k * 9.0f, hy + 3.0f, 32 + k * (10.5f + open), hy + 11.0f, 1.4f, R_WARM, 0.1f, 0.45f);
        cv_px((int)(32 + k * (10.5f + open)), (int)hy + 12, sh(R_IRON, 0.6f));
        cv_ellipse(32 + k * (3.4f + open), hy + 7.0f, 3.2f, 4.0f, R_WARM, 0.06f, 0.42f);
        for (int q = 0; q < 3; q++) cv_px((int)(32 + k * (2.5f + open + q)), (int)hy + 5 + q * 2, sh(R_IRON, 0.62f));   /* grey hair on them */
        float px = 0.0f, py = 0.0f;
        for (int j = 0; j <= 5; j++) {
            float t = (float)j / 5.0f;
            px = 32 + k * (3.6f + open * 2.0f - t * (1.8f + open) + sinf(t * PI_F) * 0.9f); py = hy + 10.0f + (float)j;
            cv_px((int)px, (int)py, sh(R_IRON, 0.14f + 0.38f * t));
        }
        cv_px((int)px, (int)py + 1, (uint8_t)(R_MOSS + 14)); cv_px((int)px, (int)py + 2, (uint8_t)(R_MOSS + 9));   /* venom (no glint) */
    }
    if (frame == 3) {                                                /* the web gathering at her jaws */
        cv_ellipse(32, hy + 15.0f, 2.6f, 2.2f, R_GHOST, 0.5f, 0.9f);
        for (int k = -1; k <= 1; k += 2) cv_line(32 + k * 2.0f, hy + 11.0f, 32 + k * 1.0f, hy + 14.0f, sh(R_GHOST, 0.75f));
    }
    cv_outline(0);
    if (frame != 3) {                                                /* the torn web she trails, hanging from her */
        web_trail(17, ay + 8.0f, 2, 60, 1.5f, 0.0f);
        web_trail(19, ay + 10.0f, 9, 63, 0.5f, 0.25f);
        web_trail(47, ay + 8.0f, 63, 58, 1.2f, 0.1f);
        web_trail(45, ay + 11.0f, 56, 63, 0.6f, 0.0f);
        web_trail(6, 20, 1, 44, 0.8f, 0.3f);
        web_trail(57, 24, 63, 45, 0.6f, 0.2f);
    }
    cv_store(id);
}

/* her abdomen: the web spider's pitted shell stretched over eggs that press pale through the skin, each with a dark
 * rim where the skin is drawn tight, veins running between them, her mark at the front */
static void brood_abdomen(float cx, float cy, float rx, float ry) {
    spider_abdomen(cx, cy, rx, ry, R_IRON, 511);
    for (int i = 0; i < 16; i++) {                                   /* eggs pressing through */
        float a = h01(i, 1, 411) * TAU_F, r = 0.75f * sqrtf(h01(i, 2, 411));
        float ex = cx + cosf(a) * r * rx, ey = cy + sinf(a) * r * ry * 0.85f, er = 2.2f + 0.6f * h01(i, 3, 411);
        for (int y = (int)(ey - er) - 1; y <= (int)(ey + er) + 1; y++)
            for (int x = (int)(ex - er) - 1; x <= (int)(ex + er) + 1; x++) {
                float d = hypotf(x + 0.5f - ex, y + 0.5f - ey) / er;
                if (d > 1.25f) continue;
                if (d > 1.0f) { if (y + 0.5f > ey) cv_px(x, y, sh(R_IRON, 0.02f)); continue; }   /* the skin drawn tight under it */
                float lit = (1.0f - d) * 0.5f + ((x + 0.5f < ex && y + 0.5f < ey) ? 0.2f : 0.0f);
                cv_px(x, y, sh(R_BONE, 0.12f + lit * 0.55f));
            }
    }
    for (int q = 0; q < 6; q++) {                                    /* veins between them */
        float x = cx + (h01(q, 5, 413) - 0.5f) * rx * 1.4f, y = cy - ry * 0.7f;
        for (int k = 0; k < (int)(ry * 1.3f); k++) {
            x += (h01(q, k, 415) - 0.5f) * 1.6f; y += 1.0f;
            if ((x - cx) * (x - cx) / (rx * rx) + (y - cy) * (y - cy) / (ry * ry) > 0.85f) break;
            uint8_t c = cv_sprite[(int)y * TS + (int)x];
            if (c >= R_IRON && c < R_IRON + 16) cv_px((int)x, (int)y, (uint8_t)(R_BLOOD + 3));
        }
    }
    for (int k = 0; k < 7; k++) {                                    /* her mark, an hourglass in blood */
        int w = k < 3 ? 3 - k : (k > 3 ? k - 3 : 0);
        for (int q = -w; q <= w; q++) cv_px((int)cx + q, (int)(cy + ry * 0.45f) + k, (uint8_t)(R_BLOOD + 8 + (q == 0) * 2));
    }
}

/* The brood mother: a black spider the size of a dog, her abdomen swollen with eggs that press pale through the skin,
 * veined between them; the web spider's eight eyes and pale fangs, thick banded legs. Body R_IRON (PF_CHITIN). pose
 * 0-1 walking, 2 laying (a sac coming down on its thread), 3 dead. */
static void spr_brood(int id, int frame) {
    cv_begin(cv_sprite, TS, TS);
    float cx = 32, cy = frame == 3 ? 52 : 42, ab = frame == 2 ? -6.0f : 0.0f;
    if (frame == 3) {                                                /* dead on her back, her legs curled over her */
        brood_abdomen(cx, cy - 2, 15.0f, 9.0f);
        for (int s = -1; s <= 1; s += 2)
            for (int l = 0; l < 4; l++)
                chitin_leg(cx + s * (4 + l * 1.5f), cy - 8, cx + s * (10 + l * 2), cy - 14 - l * 2, cx + s * (5 + l), cy - 20 - l, 1.8f, R_IRON, 0.05f, 0.35f, -1, 517 + (uint32_t)(l * 2 + s + 1));
        cv_ellipse(cx, cy - 9, 8.0f, 5.0f, R_IRON, 0.06f, 0.35f);
        cv_outline(0);
        cv_store(id);
        return;
    }
    cv_flat(32, 62, 28, 2.2f, R_WARM + 1);
    int back = g_back;                                              /* 1.0.0b18: from behind, her abdomen nearest, her head hidden */
    if (!back) brood_abdomen(cx, cy - 12 + ab, 17.0f, 13.0f);       /* the abdomen, raised behind her */
    for (int s = -1; s <= 1; s += 2)
        for (int l = 3; l >= 0; l--) {
            float k = (frame == 1 ? (l & 1 ? 1.0f : -1.0f) : (frame == 0 ? (l & 1 ? -1.0f : 1.0f) : 0.0f)) * s;
            float kx = cx + s * (7 + l * 1.5f), ky = cy + l * 2.0f;
            float mx = cx + s * (18 + l * 2.5f), my = cy - 14 + l * 3.5f + k * 2.0f;
            float fx = cx + s * (24 + l * 2.0f) + k, fy = 61;
            chitin_leg(kx, ky, mx, my, fx, fy, 2.0f, R_IRON, 0.05f, 0.42f, -1, 521 + (uint32_t)(l * 2 + s + 1));
        }
    if (back) {                                                     /* the eggs she carries, near and swollen, over all of her but the legs */
        brood_abdomen(cx, cy - 6 + ab, 18.0f, 15.0f);
        cv_px((int)cx, (int)(cy + 8 + ab), sh(R_IRON, 0.3f)); cv_px((int)cx - 1, (int)(cy + 9 + ab), sh(R_IRON, 0.15f)); cv_px((int)cx + 1, (int)(cy + 9 + ab), sh(R_IRON, 0.15f));
        cv_outline(0);
        cv_store(id);
        return;
    }
    cv_ellipse(cx, cy + 3, 9.0f, 7.0f, R_IRON, 0.06f, 0.45f);     /* head */
    grid_stamp(SPIDER_HEAD, 13, 8, (int)cx - 6, (int)cy - 1, R_IRON, R_IRON, (uint8_t)(R_FLAME + 14), 0);
    spider_fangs(cx, cy + 7, 5.0f, 0);
    if (frame == 2) {                                               /* a sac coming down on its thread, before her jaws */
        cv_line(cx + 6, cy + 4, cx + 6, cy + 12, sh(R_STONE, 0.8f));
        cv_ellipse(cx + 6, cy + 16, 5.0f, 5.5f, R_BONE, 0.35f, 0.85f);
        for (int k = -4; k <= 4; k += 2) cv_line(cx + 1.5f, cy + 16 + k, cx + 10.5f, cy + 16 + k * 0.6f, sh(R_BONE, 0.95f));   /* wound round */
    }
    cv_outline(0);
    cv_store(id);
}

/* the brood mother's dressing by rank: II silk and mould, III frost and her shell cracked, IV old blood on her fangs and
 * an egg burst, its yolk run, V charred and the pale skull on her back, gilded gold in her bands and along her shell */
static void brood_dress(int id, int frame) {
    float cx = 32, cy = 42, ab = frame == 2 ? -6.0f : 0.0f, ay = cy - 12 + ab;
    for (int r = RK_II; r < NRANK; r++) {
        g_dress_ramp2 = r == RK_V ? R_BLOOD : R_BONE;               /* her eggs and fangs; at V over her mark and eggs too */
        g_dress_ramp3 = r == RK_V ? R_BONE : -1;
        dress_begin(id);
        if (r == RK_II) {
            kit_moss(0.25f, 0, 531);
            for (int q = 0; q < 5; q++) {
                float x0 = 8 + 48 * h01(q, 1, 533), y0 = 22 + 12 * h01(q, 2, 533), x1 = x0 + (h01(q, 3, 533) - 0.5f) * 16, y1 = y0 + 8 + 10 * h01(q, 4, 533);
                for (int k = 0; k <= 12; k++) { float t = k / 12.0f; int x = (int)(x0 + (x1 - x0) * t), y = (int)(y0 + (y1 - y0) * t + sinf(t * PI_F) * 2.0f); if (base_on(x, y) || (k & 1)) cv_px(x, y, sh(R_GHOST, 0.65f)); }
            }
        }
        if (r == RK_III) {
            kit_frost(0.6f, 535);
            float x = cx - 9, y = ay - 10;
            for (int k = 0; k < 14; k++) { cv_px((int)x, (int)y, 0); cv_px((int)x + 1, (int)y, sh(R_IRON, 0.5f)); x += h01(k, 1, 537) < 0.5f ? 1.0f : 0.0f; y += 1.0f; }
        }
        if (r >= RK_IV && r != RK_GILDED) {
            kit_blood(cx - 2.5f, cy + 8, 6, 2, 539); kit_blood(cx + 2.5f, cy + 8, 6, 2, 541);
            cv_flat(cx + 7, ay - 2, 3.0f, 2.6f, 0); cv_flat(cx + 7, ay - 2, 1.8f, 1.5f, (uint8_t)(R_GOLD + 3));   /* an egg burst */
            for (int k = 0; k < 6; k++) cv_px((int)cx + 7 + (k & 1), (int)ay + 1 + k, (uint8_t)(R_GOLD + 4 - k / 3));
        }
        if (r == RK_V) {
            kit_char(0.3f, 543);
            cv_flat(cx + 0.5f, ay + 0.8f, 6.2f, 6.6f, 0);
            grid_stamp(SKULL_SMALL, 9, 10, (int)cx - 4, (int)ay - 4, R_BONE, R_BONE, 0, 0);
        }
        if (r == RK_GILDED)
            for (int y = 0; y < TS; y++)
                for (int x = 0; x < TS; x++) {
                    uint8_t b = base_c(x, y);
                    if (b >= R_IRON + 6 && b < R_IRON + 16 && h01(x, y, 545) < 0.8f) cv_px(x, y, (uint8_t)(R_GOLD + clampi(b - R_IRON, 4, NGOLD - 1)));
                    else if (b >= R_IRON + 4 && b < R_IRON + 6 && !base_on(x, y - 1)) cv_px(x, y, (uint8_t)(R_GOLD + 6));
                }
        dress_store(id, r, R_IRON);
    }
    g_dress_ramp2 = g_dress_ramp3 = -1;
}

/* an egg sac, slung from the wall: silk wound round and round it in strands, the young pressed dark against it inside,
 * still, then stirring (their legs at the silk, a few eyes catching), then split and empty, torn lips round a hollow */
static void spr_sac(int id, int frame) {
    cv_begin(cv_sprite, TS, TS);
    float cx = 32, cy = 40, r = frame == 1 ? 13.0f : 12.0f;
    for (int i = 0; i < 9; i++) {                                   /* the silk that holds it, slung from above */
        float x = cx + (h01(i, 1, 421) - 0.5f) * 30;
        cv_line(x, 0, cx + (x - cx) * 0.4f, cy - r + 2, sh(R_STONE, 0.6f + 0.2f * (i & 1)));
    }
    if (frame == 2) {                                               /* split: torn lips of silk round a dark hollow */
        cv_ellipse(cx, cy, r, r * 1.1f, R_BONE, 0.2f, 0.7f);
        cv_flat(cx, cy + 1, r * 0.6f, r * 0.8f, 0);
        for (int i = 0; i < 14; i++) {                              /* its lips, torn outwards */
            float a = h01(i, 2, 423) * TAU_F, l = 0.2f + 0.25f * h01(i, 3, 423);
            cv_line(cx + cosf(a) * r * 0.55f, cy + sinf(a) * r * 0.75f, cx + cosf(a) * r * (0.75f + l), cy + sinf(a) * r * (0.95f + l), sh(R_BONE, 0.9f));
        }
        for (int i = 0; i < 3; i++) cv_line(cx - 2 + i * 2, cy + r, cx - 1 + i * 2.5f, cy + r + 6 + i * 2, sh(R_BONE, 0.5f));   /* strands trailing */
        cv_outline(0); cv_store(id); return;
    }
    cv_ellipse(cx, cy, r, r * 1.1f, R_BONE, 0.3f, 0.92f);
    for (int y = (int)(cy - r * 1.1f); y <= (int)(cy + r * 1.1f); y++)   /* the young pressed dark against it, inside */
        for (int x = (int)(cx - r); x <= (int)(cx + r); x++) {
            uint8_t *p = &cv_sprite[y * TS + x];
            if (*p < R_BONE || *p >= R_BONE + 16) continue;
            float n = fbm(x / 3.0f + frame * 2.3f, y / 3.0f, 32, 425);
            if (n > 0.6f) *p = sh(R_IRON, 0.18f + 0.25f * (1.0f - n));
            else if (n > 0.55f) *p = sh(R_BONE, 0.25f);             /* the silk's shadow round them */
        }
    for (int k = 0; k < 11; k++) {                                  /* silk wound round it, strand on strand */
        float y0 = cy - r * 1.0f + k * r * 0.2f, tilt = 0.35f + 0.1f * h01(k, 1, 427);
        for (int x = (int)(cx - r); x <= (int)(cx + r); x++) {
            float dx = (x + 0.5f - cx) / r, y = y0 + dx * r * tilt;
            if (dx * dx + ((y - cy) / (r * 1.1f)) * ((y - cy) / (r * 1.1f)) > 0.95f) continue;
            if (((x + k) % 5) == 0) continue;                       /* it breaks off here and there */
            cv_px(x, (int)y, sh(R_BONE, 0.82f + 0.12f * (1.0f - dx * dx)));
        }
    }
    if (frame == 1)
        for (int i = 0; i < 6; i++) {                               /* stirring: legs at the silk, and eyes */
            float a = h01(i, 4, 429) * TAU_F, rr = r * 0.85f;
            cv_line(cx + cosf(a) * rr * 0.5f, cy + sinf(a) * rr * 0.5f, cx + cosf(a) * rr, cy + sinf(a) * rr * 1.05f, sh(R_IRON, 0.12f));
            if (i < 4) cv_px((int)(cx - 6 + i * 4), (int)(cy - 3 + (i & 1) * 5), (uint8_t)(R_FLAME + 3));
        }
    cv_outline(0);
    cv_store(id);
}

/* spiderlings: drawn bold, to be shown at a fifth of the size: banded legs, its mother's mark, two great eyes and the
 * small ones beside them, pale fang tips */
static void spr_ling(int id, int frame) {
    cv_begin(cv_sprite, TS, TS);
    for (int s = -1; s <= 1; s += 2)
        for (int l = 0; l < 4; l++) {
            float k = (frame ? (l & 1 ? 1.0f : -1.0f) : (l & 1 ? -1.0f : 1.0f)) * s * 3.0f;
            float kx = 32 + s * (20 + l * 2), ky = 30 + l * 6 + k, fx = 32 + s * (26 + l * 1.5f);
            cv_capsule(32 + s * 6, 40 + l * 3, kx, ky, 2.4f, R_IRON, 0.05f, 0.4f);
            cv_capsule(kx, ky, fx, 60, 2.0f, R_IRON, 0.05f, 0.35f);
            cv_ellipse(kx, ky, 2.6f, 2.6f, R_IRON, 0.35f, 0.6f);    /* a pale band at the knee */
            cv_ellipse((kx + fx) * 0.5f, (ky + 60) * 0.5f, 2.2f, 2.2f, R_IRON, 0.3f, 0.55f);
        }
    cv_ellipse(32, 38, 12, 10, R_IRON, 0.06f, 0.45f);
    for (int k = 0; k < 5; k++) { int w = k < 2 ? 2 - k : k - 2; for (int q = -w; q <= w; q++) cv_px(32 + q, 32 + k, (uint8_t)(R_BLOOD + 8)); }   /* its mother's mark */
    cv_ellipse(32, 48, 8, 7, R_IRON, 0.08f, 0.5f);
    for (int k = -1; k <= 1; k += 2) {
        cv_flat(32 + k * 3.5f, 47, 2.2f, 2.2f, (uint8_t)(R_FLAME + 5)); cv_eye_mark((int)(32 + k * 3.5f), 47);
        cv_flat(32 + k * 6.5f, 45, 1.0f, 1.0f, (uint8_t)(R_FLAME + 2));
        cv_line(32 + k * 2.0f, 53, 32 + k * 1.0f, 56, sh(R_BONE, 0.45f));   /* fang tips */
    }
    cv_outline(0);
    cv_store(id);
}

