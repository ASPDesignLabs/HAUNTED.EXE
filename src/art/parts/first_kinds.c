/* A part of crawl_art.c (docs/PLAN-tidy.md): it is #included there in its place and never compiled on its own.
 * The spectre and the bat; fire, the blast, a bone in flight; gems and the key; a chain, the tombstone. */

/* a hand of mist at hx, hy: a pale palm, long fingers fanned towards a (radians, 0 = out to side s), their tips
 * trailing off into wisps, and a few motes falling from it */
static void mist_hand(float hx, float hy, int s, float a, float ph, uint32_t seed) {
    cv_ellipse(hx, hy, 2.2f, 1.9f, R_GHOST, 0.5f, 0.85f);
    for (int f = 0; f < 4; f++) {
        float fa = a + (f - 1.5f) * 0.32f, len = 6.0f + (f == 1 || f == 2 ? 1.5f : 0.0f);
        float tx = hx + s * cosf(fa) * len, ty = hy + sinf(fa) * len;
        cv_line(hx, hy, tx, ty, sh(R_GHOST, 0.8f));
        for (int k = 1; k <= 4; k++) {                              /* the tip going to mist, drifting down */
            float wx = tx + s * cosf(fa) * k * 1.2f, wy = ty + sinf(fa) * k * 1.2f + k * k * 0.25f;
            if ((k & 1) || h01(f, k, seed) < 0.4f) cv_px((int)wx, (int)wy, sh(R_GHOST, 0.62f - k * 0.08f));
        }
    }
    for (int k = 0; k < 5; k++) {                                   /* motes falling from the hand */
        float d = fmodf(h01(k, 1, seed) * 9.0f + ph * 1.3f, 9.0f);
        cv_px((int)(hx + (h01(k, 2, seed) - 0.5f) * 7.0f), (int)(hy + 3.0f + d), sh(R_GHOST, 0.55f - d * 0.04f));
    }
}

/* The spectre: the long face pale in the mouth of a hood, a shroud trailing to a tail that sways and frays, hands of
 * mist held out. recoil: thrown back, its arms flung up, the shroud flaring (the shriek as it goes). */
static void spr_spectre(int id, float ph, int recoil) {
    cv_begin(cv_sprite, TS, TS);
    float lift = recoil ? -3.0f : 0.0f, oy = 17.0f + lift;          /* the hood's opening, about the face */
    for (int y = 1; y < TS; y++)
        for (int x = 0; x < TS; x++) {
            float yy = y + 0.5f - lift, t = (yy - 26.0f) / 37.0f;
            float sway = t > 0.0f ? sinf(ph + t * 2.6f) * 4.5f * t : 0.0f;   /* the tail drifts */
            float dx = x + 0.5f - 32.0f - sway, hw, flare = recoil ? 0.36f : 0.2f;
            if (yy < 2.0f) continue;
            if (yy < 12.0f) hw = 10.8f * sqrtf((yy - 2.0f) / 10.0f);  /* the hood's crown */
            else if (yy < 26.0f) hw = 10.8f + (yy - 12.0f) * 0.1f;   /* falling past the face */
            else if (yy < 40.0f) hw = 12.2f + (yy - 26.0f) * flare;   /* the shoulders, the breast */
            else hw = (12.2f + 14.0f * flare) * (1.0f - (yy - 40.0f) / 31.0f);   /* narrowing to the tail */
            if (fabsf(dx) > hw) continue;
            if (yy > 30.0f && fabsf(dx) > hw - 2.0f && h01(dx > 0.0f, (int)(yy / 2.0f), 967) < 0.3f) continue;   /* torn at its edges */
            if (yy > 44.0f && h01(x, (int)(yy / 3.0f), 961 + (int)(ph * 3.0f)) * 0.7f + h01(x, y, 963) * 0.3f < (yy - 44.0f) / 22.0f) continue;   /* fraying in streaks */
            float u = dx / hw, v = 0.1f + 0.2f * (1.0f - u * u) - fmaxf(0.0f, t) * 0.07f;
            if (fmodf(dx * 0.42f + 20.0f + sinf(yy * 0.08f + ph) * 0.8f, 2.2f) < 0.4f) v -= 0.06f;   /* folds */
            if (fabsf(u) > 0.86f) v += 0.28f;                       /* a cold rim of light */
            float ox = (x + 0.5f - 32.0f) / 7.8f, oyy = (y + 0.5f - oy) / 11.2f, o = ox * ox + oyy * oyy;
            if (o <= 1.0f) v = 0.02f;                               /* the hood's mouth, all shadow */
            else if (o <= 1.35f && y < oy + 9.0f) v += 0.22f;       /* its lip */
            cv_px(x, y, sh(R_GHOST, v));
        }
    grid_stamp(SPECTRE_FACE, 13, 21, 26, (int)(7.0f + lift), R_GHOST, R_GHOST, C_WHITE, 0);
    for (int x = 25; x <= 39; x++) {                                /* the hood's brim over the brow */
        float ox = (x + 0.5f - 32.0f) / 7.8f;
        if (fabsf(ox) >= 1.0f) continue;
        int ye = (int)(oy - 11.2f * sqrtf(1.0f - ox * ox));
        cv_px(x, ye, sh(R_GHOST, 0.42f)); cv_px(x, ye + 1, sh(R_GHOST, 0.3f));
    }
    for (int s = -1; s <= 1; s += 2) {                              /* sleeves, and the hands of mist */
        float sx = 32.0f + s * 10.0f, sy = 25.0f + lift;
        float hx = recoil ? 32.0f + s * 22.0f : 32.0f + s * 19.0f, hy = recoil ? 11.0f + lift : 33.0f + 2.0f * sinf(ph + s);
        cv_capsule(sx, sy, hx - s * 2.5f, hy + (recoil ? 2.0f : -1.0f), 3.3f, R_GHOST, 0.12f, 0.48f);
        cv_ellipse(hx - s * 2.5f, hy + (recoil ? 2.0f : -1.0f), 3.4f, 2.6f, R_GHOST, 0.15f, 0.5f);   /* the sleeve's mouth */
        mist_hand(hx, hy, s, recoil ? -1.15f : 0.75f, ph, 971 + (uint32_t)(s + 1));
    }
    cv_store(id);
}

/* The bat's face (docs/InitialArtPass.md, section 8): ears taller than its head and flesh inside, a small pointed face,
 * a leaf of a nose, the mouth open on two fine fangs, eyes like coals. Fur in R_IRON (hex), skin in R_SKIN (g-v). */
static const char *const BAT_FACE[13] = {
    "1...........1", "21.........12", "3l1.......1l3", "3ml2.....2lm3", "3nml2...2lmn3",
    ".3nl34543ln3.", ".2l4566654l2.", "..35@767@53..", "..2457o7542..", "...35non53...",
    "...4f000f4...", "....f0h0f....", ".....363.....",
};
/* and turned to the right: the ear back, the snout and its leaf ahead, a fang showing */
static const char *const BAT_SIDE[11] = {
    "..1......", ".1l1.....", ".2ml2....", ".3nm3....", "..3n4....", ".345653..",
    "34567@75.", "4567776on", "3456500..", ".345f0f..", "..343....",
};

/* the bat's wing from the front, side s, raised by up: an arm to the wrist, three long fingers fanning to the trailing
 * edge, the membrane scalloped between them and torn at its edge, a hooked thumb at the wrist */
static void bat_wing_front(int s, float up, uint32_t seed) {
    float shx = 32 + s * 4.0f, shy = 27.0f, wx = 32 + s * 13.0f, wy = 20.0f - 5.0f * up;
    float tip[4][2] = { { 32 + s * 25.0f, 14.0f - 8.0f * up }, { 32 + s * 28.0f, 26.0f - 5.0f * up }, { 32 + s * 22.0f, 35.0f - 1.5f * up },
                        { 32 + s * 5.0f, 36.0f } };                  /* the last is the membrane's foot, at the leg */
    float p[24]; int n = 0;
    p[n++] = shx; p[n++] = shy; p[n++] = wx; p[n++] = wy;
    for (int f = 0; f < 3; f++) {
        p[n++] = tip[f][0]; p[n++] = tip[f][1];
        for (int k = 1; k <= 2; k++) {                               /* the scallop to the next, drawn in towards the wrist */
            float t = k / 3.0f, mx = tip[f][0] + (tip[f + 1][0] - tip[f][0]) * t, my = tip[f][1] + (tip[f + 1][1] - tip[f][1]) * t;
            float pull = 0.22f * sinf(t * PI_F);
            p[n++] = mx + (wx - mx) * pull; p[n++] = my + (wy - my) * pull;
        }
    }
    p[n++] = tip[3][0]; p[n++] = tip[3][1];
    cv_poly(p, n / 2, R_PURPLE, 0.1f, 0.34f, wy, 22.0f);
    for (int y = 0; y < TS; y++)                                     /* the membrane's shading, dithered, thinner at its edge */
        for (int x = s < 0 ? 0 : 32; x < (s < 0 ? 32 : TS); x++) {
            uint8_t c = cv_sprite[y * TS + x];
            if (c < R_PURPLE || c >= R_PURPLE + 16) continue;
            float v = 0.1f + 0.24f * clampf(1.0f - (y + 0.5f - wy) / 22.0f, 0.0f, 1.0f) + (((x + y) & 1) ? 0.03f : -0.03f);
            float far = fabsf(x + 0.5f - 32.0f) / 28.0f;
            cv_px(x, y, sh(R_PURPLE, v + 0.08f * far * far));
        }
    for (int k = 0; k < 2; k++) {                                    /* torn at the edge: a notch or two */
        int f = (int)(h01(k, 1, seed) * 2.99f);
        float t = 0.3f + 0.4f * h01(k, 2, seed), mx = tip[f][0] + (tip[f + 1][0] - tip[f][0]) * t, my = tip[f][1] + (tip[f + 1][1] - tip[f][1]) * t;
        float pull = 0.22f * sinf(t * PI_F); mx += (wx - mx) * pull; my += (wy - my) * pull;
        for (int q = 0; q < 3; q++) { float qx = mx + (wx - mx) * 0.05f * q, qy = my + (wy - my) * 0.05f * q; cv_px((int)qx, (int)qy, C_CLEAR); cv_px((int)qx + s, (int)qy, q < 2 ? C_CLEAR : sh(R_PURPLE, 0.05f)); }
    }
    cv_capsule(shx, shy, wx, wy, 0.9f, R_IRON, 0.3f, 0.5f);         /* the arm */
    for (int f = 0; f < 3; f++) {                                    /* the fingers, a knuckle halfway */
        float kx = wx + (tip[f][0] - wx) * 0.45f, ky = wy + (tip[f][1] - wy) * 0.45f;
        cv_line(wx, wy, kx, ky, sh(R_IRON, 0.5f)); cv_line(kx, ky, tip[f][0], tip[f][1], sh(R_IRON, 0.4f));
        cv_px((int)kx, (int)ky, sh(R_IRON, 0.68f));
    }
    cv_px((int)wx, (int)wy, sh(R_IRON, 0.7f));                       /* the wrist, and its thumb's hook */
    cv_line(wx, wy, wx + s * 1.0f, wy - 3.0f, sh(R_IRON, 0.55f)); cv_px((int)(wx + s * 2.0f), (int)(wy - 3.0f), sh(R_IRON, 0.8f));
}

/* the bat's body: a small furred barrel, paler at the breast, its feet drawn up with claws */
static void bat_body(float cx, float cy) {
    for (int y = (int)(cy - 6); y <= (int)(cy + 6); y++)
        for (int x = (int)(cx - 5); x <= (int)(cx + 5); x++) {
            float nx = (x + 0.5f - cx) / 4.2f, ny = (y + 0.5f - cy) / 5.6f, d = nx * nx + ny * ny;
            if (d > 1.0f) continue;
            float v = 0.12f + 0.22f * (1.0f - d) + (fabsf(nx) < 0.45f && ny > -0.3f ? 0.08f : 0.0f) + (h01(x, y, 561) - 0.5f) * 0.14f;   /* fur, its breast paler */
            cv_px(x, y, sh(R_IRON, v));
        }
    for (int s = -1; s <= 1; s += 2) {                               /* feet */
        cv_line(cx + s * 1.5f, cy + 5.0f, cx + s * 2.5f, cy + 7.5f, sh(R_IRON, 0.3f));
        cv_px((int)(cx + s * 3.0f), (int)(cy + 8.0f), sh(R_IRON, 0.6f)); cv_px((int)(cx + s * 2.0f), (int)(cy + 8.0f), sh(R_IRON, 0.5f));
    }
}

static void spr_bat(int id, int frame) {
    cv_begin(cv_sprite, TS, TS);
    if (frame == 2) {                                                /* dead: the wings folded about it, falling */
        for (int s = -1; s <= 1; s += 2) {
            float wl[8] = { 32 + s * 2.0f, 33, 32 + s * 7.0f, 38, 32 + s * 6.0f, 56, 32 + s * 1.0f, 52 };
            cv_poly(wl, 4, R_PURPLE, 0.06f, 0.24f, 33, 22);
            cv_line(32 + s * 3.0f, 34, 32 + s * 6.0f, 55, sh(R_IRON, 0.45f));
        }
        bat_body(32, 44);
        grid_stamp(BAT_FACE, 13, 13, 26, 27, R_IRON, R_SKIN, 0, 0);  /* its eyes gone dark */
        cv_outline(0);
        cv_store(id);
        return;
    }
    float up = frame == 0 ? 1.0f : -1.0f;
    for (int s = -1; s <= 1; s += 2) bat_wing_front(s, up, 571 + (uint32_t)(s + 1) * 3u);
    bat_body(32, 31);
    grid_stamp(BAT_FACE, 13, 13, 26, 17, R_IRON, R_SKIN, (uint8_t)(R_FLAME + 8), 0);
    cv_outline(0);
    cv_store(id);
}

/* the bat's dressing by rank (it stops at IV): II its wings holed and mould in its fur, III frost on its ears and bones,
 * a finger broken and a wing ripped through, IV old blood about its jaws and down its fangs, the wings' edges shredded
 * and dark with it, gilded gold on its bones and its ears' rims */
static void bat_dress(int id, float up) {
    g_dress_ramp2 = R_IRON;
    for (int r = RK_II; r < NRANK; r++) {
        if (r == RK_V) continue;
        dress_begin(id);
        if (r != RK_GILDED)
            for (int k = 0; k < 6 + 4 * (r - RK_II); k++) {          /* holes worn through the membrane */
                int x = 6 + (int)(h01(k, r, 581) * 52.0f), y = 12 + (int)(h01(k, r + 9, 581) * 24.0f);
                if (base_c(x, y) >= R_PURPLE && base_c(x, y) < R_PURPLE + 16 && base_c(x + 1, y) >= R_PURPLE && base_c(x + 1, y) < R_PURPLE + 16) { cv_px(x, y, 0); if (k & 1) cv_px(x + 1, y, 0); }
            }
        if (r == RK_II) kit_moss(0.22f, 0, 583);
        if (r == RK_III) {
            kit_frost(0.7f, 585);
            float wx = 32 - 13.0f, wy = 20.0f - 5.0f * up, tx = 32 - 25.0f, ty = 14.0f - 8.0f * up;   /* a finger broken, bent back */
            float kx = wx + (tx - wx) * 0.5f, ky = wy + (ty - wy) * 0.5f;
            cv_line(kx, ky, kx - 3.0f, ky + 5.0f, sh(R_IRON, 0.4f));
            for (int q = 0; q < 9; q++) cv_px((int)(32 + 20 - q * 0.4f + (q & 1)), (int)(30 - 1.5f * up - q * 1.1f), 0);   /* a rip in the other */
        }
        if (r >= RK_IV && r != RK_GILDED) {
            kit_blood(29, 27, 5, 2, 587); kit_blood(35, 27, 5, 2, 589);
            for (int y = 0; y < TS; y++)                              /* the edges shredded and dark */
                for (int x = 0; x < TS; x++) {
                    uint8_t b = base_c(x, y);
                    if (b < R_PURPLE || b >= R_PURPLE + 16 || (base_on(x, y + 1) && base_on(x - 1, y) && base_on(x + 1, y))) continue;
                    float hh = h01(x, y, 591);
                    if (hh < 0.3f) cv_px(x, y, 0); else if (hh < 0.7f) cv_px(x, y, (uint8_t)(R_BLOOD + 3));
                }
        }
        if (r == RK_GILDED)
            for (int y = 0; y < TS; y++)                              /* gold on its bones and the rims of its ears */
                for (int x = 0; x < TS; x++) {
                    uint8_t b = base_c(x, y);
                    if (b >= R_IRON + 6 && b < R_IRON + 16 && (y < 22 || !base_on(x, y - 1) || h01(x, y, 593) < 0.5f)) cv_px(x, y, (uint8_t)(R_GOLD + clampi(b - R_IRON - 2, 4, NGOLD - 1)));
                }
        dress_store(id, r, R_PURPLE);
    }
    g_dress_ramp2 = -1;
}

static void spr_fire(int id, int frame) {
    cv_begin(cv_sprite, TS, TS);
    float cx = 32, cy = 33, r = 7.5f;
    for (int y = 18; y < 48; y++)
        for (int x = 16; x < 48; x++) {
            float dx = x + 0.5f - cx, dy = y + 0.5f - cy, d = sqrtf(dx * dx + dy * dy);
            float n = fbm(x / 3.0f + frame * 5.0f, y / 3.0f, 32, 61) * 0.6f;
            float rr = r * (0.85f + n);
            if (d > rr) continue;
            float heat = 1.05f - d / rr * 0.75f;
            cv_px(x, y, (uint8_t)(R_FLAME + clampi((int)(heat * 15), 3, 15)));
        }
    for (int i = 0; i < 10; i++) {
        float a = h01(i, frame, 7) * TAU_F, l = r + 2 + h01(i, frame + 3, 7) * 5;
        cv_px((int)(cx + cosf(a) * l), (int)(cy + sinf(a) * l), (uint8_t)(R_FLAME + 8 + (i & 3)));
    }
    cv_store(id);
}

static void spr_boom(int id, int frame) {
    cv_begin(cv_sprite, TS, TS);
    float cx = 32, cy = 32, r = 9.0f + frame * 6.5f;
    for (int y = 0; y < TS; y++)
        for (int x = 0; x < TS; x++) {
            float dx = x + 0.5f - cx, dy = y + 0.5f - cy, d = sqrtf(dx * dx + dy * dy);
            float n = fbm(x / 3.0f, y / 3.0f + frame * 7.0f, 32, 99 + frame) * 0.7f;
            float edge = r * (0.75f + n * 0.5f);
            if (d > edge) continue;
            float heat = (1.0f - d / edge) * (1.2f - frame * 0.32f) + n * 0.3f - (frame == 2 ? 0.25f : 0.0f);
            if (frame == 2 && h01(x, y, 5) > 0.55f) continue;
            if (heat < 0.12f) { if (frame > 0) cv_px(x, y, sh(R_WARM, 0.15f + n * 0.3f)); continue; }
            cv_px(x, y, (uint8_t)(R_FLAME + clampi((int)(heat * 15), 1, 15)));
        }
    cv_store(id);
}

static void spr_bonefly(int id, float ang) {
    cv_begin(cv_sprite, TS, TS);
    float c = cosf(ang) * 8.0f, s = sinf(ang) * 8.0f;
    cv_bone(32 - c, 34 - s, 32 + c, 34 + s, 1.3f, 0.3f, 0.95f);
    cv_outline(0);
    cv_store(id);
}

/* ---- treasure ---- */



static void gem(float cx, float cy, float r, int ramp) {
    for (int y = (int)(cy - r); y <= (int)(cy + r); y++)
        for (int x = (int)(cx - r); x <= (int)(cx + r); x++) {
            float dx = (x + 0.5f - cx) / r, dy = (y + 0.5f - cy) / r;
            if (fabsf(dx) + fabsf(dy) * 0.8f > 1.0f) continue;
            float v = dx < 0 && dy < 0 ? 0.95f : (dx >= 0 && dy < 0 ? 0.7f : (dx < 0 ? 0.5f : 0.3f));
            if (fabsf(dx) < 0.3f && fabsf(dy) < 0.3f) v = 0.85f;
            cv_px(x, y, sh(ramp, v));
        }
}




static void spr_key(int id, float scale, int metal) {
    cv_begin(cv_sprite, TS, TS);
    #define KX(x) (32.0f + ((x) - 32.0f) * scale)
    float ar = fmaxf(fabsf(scale), 0.18f);
    float lo = metal == R_GOLD ? 0.35f : 0.45f;
    cv_ellipse(KX(21), 38, 6.5f * ar, 6.5f, metal, lo, 1.0f);       /* the bow */
    cv_flat(KX(21), 38, 3.0f * ar, 3.0f, C_CLEAR);
    cv_capsule(KX(26), 38, KX(47), 38, 2.0f, metal, lo, 1.0f);       /* the shank */
    cv_capsule(KX(43), 38, KX(43), 45, 1.7f, metal, lo - 0.05f, 0.95f);   /* the bit */
    cv_capsule(KX(47), 38, KX(47), 44, 1.7f, metal, lo - 0.05f, 0.95f);
    cv_ellipse(KX(29), 38, 2.0f * ar, 3.0f, metal, 0.5f, 1.0f);      /* collar */
    #undef KX
    cv_outline(sh(metal, 0.05f));
    cv_store(id);
}


/* ------------------------------------------------------------------ */
/* Set dressing: things that hang from the ceiling, graves, fire        */
/* ------------------------------------------------------------------ */

/* a chain straight down from y0 to y1: links alternately edge-on and face-on */
static void chain_down(float x, int y0, int y1) {
    for (int y = y0; y < y1; y += 4) {
        if (((y - y0) / 4) & 1) cv_capsule(x, (float)y, x, (float)(y + 4), 0.9f, R_IRON, 0.2f, 0.85f);
        else for (int a = 0; a < 16; a++) {
            float ang = TAU_F * a / 16;
            cv_px((int)(x + 0.5f + 2.0f * cosf(ang)), (int)(y + 2.5f + 2.6f * sinf(ang)), sh(R_IRON, 0.45f - 0.3f * sinf(ang + 0.7f)));
        }
    }
}

static void stone_px(int x, int y, float nx, float shade_v, int seed) {
    float v = 0.2f + 0.55f * lit(nx, 0.0f, sqrtf(fmaxf(0.0f, 1.0f - nx * nx))) + shade_v + (fbm(x / 4.0f, y / 4.0f, 16, (uint32_t)seed) - 0.5f) * 0.22f;
    if (h01(x, y, seed + 5) > 0.97f) v -= 0.2f;                    /* pits */
    if (y > 50 && fbm(x / 3.0f, y / 3.0f, 32, (uint32_t)seed + 9) > 0.6f) { cv_px(x, y, sh(R_MOSS, 0.15f + 0.4f * v)); return; }   /* moss creeping up */
    cv_px(x, y, sh(R_STONE, v));
}
static void soil(float cx, float w, int ground, uint32_t seed);
static void spr_tomb(int id, int kind) {                           /* her plot's newest stone (the graves of the runs are spr_grave's) */
    cv_begin(cv_sprite, TS, TS);
    switch (kind) {
    case 4: {                                                      /* the newest: clean-cut and pale, the earth before it still turned */
        float cx = 32, hw = 11, top = 25;
        for (int y = (int)top; y <= 59; y++)
            for (int x = (int)(cx - hw); x <= (int)(cx + hw); x++) {
                float nx = (x + 0.5f - cx) / hw, dy = y + 0.5f - (top + hw);
                if (dy < 0 && (x + 0.5f - cx) * (x + 0.5f - cx) + dy * dy > hw * hw) continue;
                float v = 0.42f + 0.45f * lit(nx, 0.0f, sqrtf(fmaxf(0.0f, 1.0f - nx * nx))) + (fbm(x / 5.0f, y / 5.0f, 12, 83) - 0.5f) * 0.08f;
                if (dy < 0 && (x + 0.5f - cx) * (x + 0.5f - cx) + dy * dy > (hw - 1.3f) * (hw - 1.3f)) v += 0.1f;   /* the dressed edge */
                if ((fabsf(x + 0.5f - cx) < 1.0f && y > 30 && y < 43) || (fabsf(y + 0.5f - 34) < 1.0f && fabsf(x + 0.5f - cx) < 5)) v -= 0.3f;   /* a cross cut sharp */
                if ((y == 47 || y == 50 || y == 53) && fabsf(x + 0.5f - cx) < 7 - (y == 53) * 3 && (x * 5 + y) % 4) v -= 0.28f;   /* fresh letters */
                cv_px(x, y, sh(R_STONE, v));
            }
        soil(cx, 17, 59, 89);
        break;
    }
    default: break;
    }
    cv_outline(0);
    cv_store(id);
}

