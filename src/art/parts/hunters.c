/* A part of crawl_art.c (docs/PLAN-tidy.md): it is #included there in its place and never compiled on its own.
 * The ghoul, the bone hound, the skeleton knight, the deacon, with their backs and dressing. */

static void spr_ghoul(int id, float w, int pose) {
    cv_begin(cv_sprite, TS, TS);
    if (pose == 4) {
        cv_flat(32, 61.5f, 24, 2.5f, R_WARM + 1);
        cv_ellipse(34, 58, 13, 4.0f, R_WARM, 0.12f, 0.55f);
        for (int i = 0; i < 5; i++) cv_px(26 + i * 4, 54, sh(R_BONE, 0.6f));
        cv_capsule(22, 58, 10, 61, 1.6f, R_WARM, 0.15f, 0.5f); cv_capsule(46, 58, 56, 61, 1.6f, R_WARM, 0.15f, 0.5f);
        cv_ellipse(18, 57, 4.4f, 3.6f, R_WARM, 0.28f, 0.8f);
        cv_flat(16, 56, 1.0f, 1.0f, 0);
        cv_rot(R_WARM, 0.4f, 251);
        cv_outline(0); cv_store(id); return;
    }
    float lift = pose == 2 ? -10.0f : 0.0f, feed = pose == 3 ? 1.0f : 0.0f;
    float hx = 32, hy = 40 + lift + feed * 10.0f;                   /* the head, thrust forward and low */
    if (pose != 2) cv_flat(32, 62, 18, 1.6f, R_WARM + 1);
    if (pose == 3)                                                  /* what it is feeding on */
        for (int i = 0; i < 5; i++) {
            float cx = 32 + (h01(i, 1, 255) - 0.5f) * 26, a = (h01(i, 2, 255) - 0.5f) * 1.2f, l = 5 + 3 * h01(i, 3, 255);
            cv_bone(cx - cosf(a) * l, 60 - sinf(a) * l, cx + cosf(a) * l, 60 + sinf(a) * l, 1.2f, 0.25f, 0.9f);
        }
    /* hind legs, behind, folded */
    for (int s = -1; s <= 1; s += 2) {
        float kx = 32 + s * 13, ky = 44 + lift * 0.6f;
        cv_capsule(32 + s * 6, 34 + lift, kx, ky, 2.0f, R_WARM, 0.15f, 0.55f);
        cv_ellipse(kx, ky, 2.2f, 2.0f, R_WARM, 0.2f, 0.6f);                       /* a knobbed knee */
        cv_capsule(kx, ky, 32 + s * 9 + (pose == 2 ? s * 6 : 0), 58 + lift * 0.3f, 1.5f, R_WARM, 0.15f, 0.5f);
    }
    /* the back, arched high, with the spine standing out */
    cv_ellipse(32, 33 + lift + feed * 4, 9.5f, 7.0f, R_WARM, 0.2f, 0.8f);   /* the back, arched, starved */
    for (int i = 0; i < 6; i++) cv_ellipse(32, 25.5f + lift + feed * 4 + i * 2.2f, 1.6f - i * 0.12f, 1.0f, R_WARM, 0.55f, 1.0f);   /* the spine, beaded */
    for (int s2 = -1; s2 <= 1; s2 += 2) {                           /* shoulder blades jutting */
        cv_ellipse(32 + s2 * 6.0f, 28.5f + lift + feed * 4, 3.2f, 2.0f, R_WARM, 0.35f, 0.95f);
        cv_line(32 + s2 * 3.5f, 27 + lift + feed * 4, 32 + s2 * 8.5f, 30 + lift + feed * 4, sh(R_WARM, 0.1f));
    }
    for (int i = 0; i < 5; i++) {                                   /* ribs down the flanks, the skin sunk between them */
        float y = 30 + lift + feed * 4 + i * 2.0f;
        cv_line(24, y, 28, y + 1.5f, sh(R_WARM, 0.08f)); cv_line(40, y, 36, y + 1.5f, sh(R_WARM, 0.08f));
        cv_line(24, y + 1, 27, y + 2.2f, sh(R_WARM, 0.55f)); cv_line(40, y + 1, 37, y + 2.2f, sh(R_WARM, 0.55f));
    }
    /* front arms: long, bony, planted wide, or flung out in the leap */
    for (int s = -1; s <= 1; s += 2) {
        float ex = 32 + s * 15, ey = 38 + lift, fx, fy;
        if (pose == 2) { ex = 32 + s * 17; ey = 26 + lift; fx = 32 + s * 24; fy = 18 + lift; }
        else { float step = s * w * 3.0f; fx = 32 + s * 17 + step * 0.3f; fy = 60 - fmaxf(0.0f, step) * 1.5f; }
        cv_capsule(32 + s * 8, 32 + lift + feed * 4, ex, ey, 1.8f, R_WARM, 0.25f, 0.8f);
        cv_ellipse(ex, ey, 2.1f, 2.0f, R_WARM, 0.3f, 0.85f);                     /* a knobbed elbow */
        cv_capsule(ex, ey, fx, fy, 1.5f, R_WARM, 0.22f, 0.75f);
        for (int c = -1; c <= 1; c++) {                             /* claws, long and hooked */
            float ca = (pose == 2 ? (s < 0 ? -2.3f : -0.8f) : 1.57f + s * 0.3f) + c * 0.35f;
            cv_line(fx, fy, fx + cosf(ca) * 6.0f, fy + sinf(ca) * 6.0f, sh(R_BONE, 0.7f));
            cv_px((int)(fx + cosf(ca) * 6.0f + s), (int)(fy + sinf(ca) * 6.0f), sh(R_BONE, 0.5f));
        }
    }
    /* the head: long ears swept up and torn, then the face (GHOUL_HEAD) */
    for (int s = -1; s <= 1; s += 2) {
        float e[6] = { hx + s * 3.0f, hy - 6, hx + s * 16, hy - 15, hx + s * 8.5f, hy + 1.0f };   /* broad, leathery */
        cv_poly(e, 3, R_WARM, 0.25f, 0.75f, hy - 15, 16);
        float in[6] = { hx + s * 5.0f, hy - 4.5f, hx + s * 13.5f, hy - 12.5f, hx + s * 8.0f, hy - 1.0f };
        cv_poly(in, 3, R_WARM, 0.06f, 0.16f, hy - 12.5f, 11);
        cv_px((int)(hx + s * 12.0f), (int)(hy - 9), C_CLEAR); cv_px((int)(hx + s * 12.5f), (int)(hy - 8), C_CLEAR);   /* a tear in each */
    }
    grid_stamp(GHOUL_HEAD, 15, 15, (int)(hx - 7.0f), (int)(hy - 7.0f), R_WARM, R_BONE, (uint8_t)(R_FLAME + 13), 0);
    if (pose == 3) { cv_line(hx - 1, hy + 4, hx + 1, hy + 11, sh(R_BLOOD, 0.3f)); cv_line(hx + 2, hy + 4, hx + 3, hy + 9, sh(R_BLOOD, 0.22f)); }   /* feeding */
    cv_rot(R_WARM, 0.3f, 253);
    cv_outline(0);
    cv_store(id);
}

/* the ghoul from behind (1.0.0b18): the hindquarters, the arched back with its spine running away from you, the hip bones,
 * the hind legs folded large, the front claws planted beyond, the ears' tips over the back; its face out of sight */
static void spr_ghoul_back(int id, float w) {
    cv_begin(cv_sprite, TS, TS);
    cv_flat(32, 62, 18, 1.6f, R_WARM + 1);
    for (int s = -1; s <= 1; s += 2) {                              /* the front arms, planted wide beyond it */
        float step = s * w * 3.0f, fx = 32 + s * 16 + step * 0.3f, fy = 57 - fmaxf(0.0f, step);
        cv_capsule(32 + s * 8, 31, 32 + s * 13, 38, 1.7f, R_WARM, 0.12f, 0.45f);
        cv_capsule(32 + s * 13, 38, fx, fy, 1.4f, R_WARM, 0.1f, 0.4f);
        for (int c = -1; c <= 1; c++) cv_line(fx, fy, fx + c * 2.0f + s, fy + 3.5f, sh(R_BONE, 0.45f));
        float e[6] = { 32 + s * 3.0f, 30, 32 + s * 12, 19, 32 + s * 8.0f, 31 };   /* an ear's tip, over the shoulder */
        cv_poly(e, 3, R_WARM, 0.2f, 0.6f, 19, 12);
        cv_px(32 + s * 10, 22, C_CLEAR);                            /* its tear */
    }
    cv_ellipse(32, 32, 9.5f, 8.0f, R_WARM, 0.18f, 0.78f);          /* the back, arched, starved */
    for (int i = 0; i < 8; i++) cv_ellipse(32, 25.0f + i * 2.0f, 1.5f - i * 0.06f, 0.9f, R_WARM, 0.55f, 1.0f);   /* the spine, beaded, running to the rump */
    for (int s = -1; s <= 1; s += 2) {
        cv_ellipse(32 + s * 6.0f, 27.0f, 3.0f, 1.8f, R_WARM, 0.35f, 0.95f);   /* the shoulder blades, far off */
        for (int i = 0; i < 4; i++) { float y = 31 + i * 2.0f; cv_line(32 + s * 3.0f, y, 32 + s * 8.0f, y + 1.0f, sh(R_WARM, 0.1f)); cv_line(32 + s * 3.0f, y + 1, 32 + s * 7.0f, y + 2.0f, sh(R_WARM, 0.5f)); }
        cv_ellipse(32 + s * 5.0f, 39.0f, 2.4f, 2.0f, R_WARM, 0.4f, 1.0f);   /* the hip bones, sharp */
    }
    for (int s = -1; s <= 1; s += 2) {                              /* the hind legs, folded under it, near and large */
        float k = s * w, kx = 32 + s * 12 - k, ky = 47, fx = 32 + s * 7 - k * 1.5f;
        cv_capsule(32 + s * 5, 40, kx, ky, 2.4f, R_WARM, 0.2f, 0.7f);
        cv_ellipse(kx, ky, 2.4f, 2.2f, R_WARM, 0.3f, 0.8f);         /* a knobbed knee */
        cv_capsule(kx, ky, kx - s * 1.0f, 55, 1.6f, R_WARM, 0.15f, 0.55f);   /* the shin, to the heel, high off the ground */
        cv_capsule(kx - s * 1.0f, 55, fx, 61, 1.3f, R_WARM, 0.15f, 0.5f);
        cv_px((int)kx - s, 55, sh(R_BONE, 0.7f));                    /* the heel's bone */
        for (int c = -1; c <= 1; c++) cv_line(fx, 61, fx + c * 2.0f, 62.5f, sh(R_BONE, 0.5f));
    }
    cv_rot(R_WARM, 0.3f, 253);
    cv_outline(0);
    cv_store(id);
}
/* the ghoul's dressing by rank: II loose skin hanging and mould, III frost and an ear torn through, IV a gash down its
 * back and its claws and mouth bloodied, V charred in a chained beast's collar, gilded gold leaf and gilt fangs */
static void ghoul_dress(int id, float lift, float feed) {
    float hx = 32, hy = 40 + lift + feed * 10.0f;
    for (int r = RK_II; r < NRANK; r++) {
        dress_begin(id);
        if (r != RK_GILDED) { kit_rags(25, 36 + lift + feed * 4, 30, 38 + lift + feed * 4, 6, 0.0f, R_WARM, 0.1f, 0.3f, 0, 511); kit_rags(35, 37 + lift + feed * 4, 39, 36 + lift + feed * 4, 5, 0.0f, R_WARM, 0.1f, 0.3f, 0, 513); }
        if (r == RK_II) kit_moss(0.32f, 2, 515);
        if (r == RK_III) kit_frost(0.75f, 517);
        if (r >= RK_III && r != RK_GILDED) { cv_line(hx + 13, hy - 13, hx + 15, hy - 10, 0); cv_line(hx + 14, hy - 13, hx + 16, hy - 11, 0); }   /* an ear torn through */
        if (r >= RK_IV && r != RK_GILDED) {
            cv_line(27, 27 + lift + feed * 4, 36, 36 + lift + feed * 4, sh(R_BLOOD, 0.15f)); cv_line(28, 27 + lift + feed * 4, 37, 36 + lift + feed * 4, sh(R_BLOOD, 0.35f));   /* a gash down its back */
            kit_blood(hx, hy + 6, 6, 3, 519);
            for (int y = 0; y < TS; y++) for (int x = 0; x < TS; x++) { uint8_t b = base_c(x, y); if (b >= R_BONE && b < R_BONE + 16 && y > hy + 8 && h01(x, y, 521) < 0.5f) cv_px(x, y, sh(R_BLOOD, 0.3f)); }   /* bloodied claws */
        }
        if (r == RK_V) { kit_char(0.3f, 523); if (!g_back) { for (int x = (int)hx - 7; x <= (int)hx + 7; x++) { cv_px(x, (int)hy + 8, sh(R_IRON, 0.55f)); cv_px(x, (int)hy + 9, sh(R_IRON, 0.25f)); } kit_chain(hx + 6, hy + 9, hx + 14, hy + 20, 2.0f, R_IRON); }
                         else kit_chain(32, 26, 36, 18, 1.0f, R_IRON); }   /* from behind, the collar's chain over its shoulders */
        if (r == RK_GILDED) {
            kit_gold(R_WARM, 0.5f, 525);
            if (!g_back) for (int y = 0; y < TS; y++) for (int x = 0; x < TS; x++) { uint8_t b = base_c(x, y); if (b >= R_BONE + 11 && b < R_BONE + 16 && y < hy + 8 && y > hy) cv_px(x, y, (uint8_t)(R_GOLD + 11)); }   /* gilt fangs */
        }
        dress_store(id, r, R_WARM);
    }
}

/* grime and moss on bones, deeper down, as on the skeletons */
static void cv_bone_decay(uint32_t seed) {
    if (g_decay <= 0.0f) return;
    for (int y = 0; y < TS; y++)
        for (int x = 0; x < TS; x++) {
            uint8_t c = cv_sprite[y * TS + x];
            if (c < R_BONE || c >= R_BONE + 16) continue;
            float n = fbm(x / 5.0f, y / 5.0f, 16, seed);
            if (x >= g_skbox[0] && x < g_skbox[2] && y >= g_skbox[1] && y < g_skbox[3]) {   /* a hand-drawn skull keeps its face */
                if (n < 0.2f + 0.25f * g_decay && c > R_BONE + 4) cv_sprite[y * TS + x] = (uint8_t)(c - 1);
                continue;
            }
            if (n < 0.28f + 0.32f * g_decay) cv_sprite[y * TS + x] = (uint8_t)(c - 3 < R_BONE + 1 ? R_BONE + 1 : c - 3);
            if (g_decay > 0.35f && n > 0.78f - 0.12f * g_decay) cv_sprite[y * TS + x] = sh(R_MOSS, 0.3f + 0.3f * n);
        }
}

/* The bone hound: a dog's skeleton in a spiked collar, a broken chain hung from it, coming at you a little from the side.
 * pose: 0-1 running, 2 biting, 3 a heap. All bone (PF_BONE). */
/* the dog's skull, by hand (docs/InitialArtPass.md section 8), facing left: a long muzzle, the nasal opening at its tip,
 * a deep orbit, the cheekbone's arch, a canine hanging; the lower jaw apart, hinged under the orbit, its front dropping
 * by jaw pixels. x, y as before: the eye a little right of x, the snout's tip nine to the left. */
static const char *const DOG_UPPER[8] = {
    "...........68a986", ".........59bccdb8", ".....3679abcddcb7", "..4689abcdb00@0b8", ".0789aab9a9800ba7",
    "0689a9887ab9cba96", ".4e5e7e67789a9864", "...f.............",
};
static const char *const DOG_JAW[4] = { ".e5e5e5e6789a9.", ".46789aa9a9a985", "..3567788898642", "....23445555432" };
static void dog_skull_at(int x0, int y0, float jaw, int eye, int mirror) {   /* top left at x0, y0; mirror: facing right */
    #define DX(c) (mirror ? x0 + 16 - (c) : x0 + (c))
    if (jaw > 1.5f)                                                 /* the gape, dark behind the teeth */
        for (int c = 1; c < 13; c++) {
            int d = (int)(jaw * (1.0f - (float)c / 14.0f));
            for (int k = 0; k < d; k++) cv_px(DX(c), y0 + 7 + k, 0);
        }
    for (int r = 0; r < 4; r++)                                     /* the jaw, its front dropped further than its back */
        for (int c = 0; c < 15; c++) {
            char ch = DOG_JAW[r][c];
            if (ch == '.') continue;
            int d = (int)(jaw * (1.0f - (float)c / 14.0f) + 0.5f);
            cv_px(DX(1 + c), y0 + 7 + r + d, (uint8_t)(R_BONE + (ch <= '9' ? ch - '0' : ch - 'a' + 10)));
        }
    grid_stamp(DOG_UPPER, 17, 8, x0, y0, R_BONE, R_BONE, eye ? (uint8_t)(R_FLAME + 11) : 0, mirror);
    if (!eye) cv_px(DX(13), y0 + 3, 0);                            /* a dead one's socket, empty */
    #undef DX
}
static void dog_skull(float x, float y, float jaw, int eye) { dog_skull_at((int)x - 9, (int)y - 5, jaw, eye, 0); }

static void spr_hound(int id, float w, int pose) {
    cv_begin(cv_sprite, TS, TS);
    if (pose == 3) {
        cv_flat(32, 61.5f, 24, 2.5f, R_WARM + 1);
        for (int i = 0; i < 8; i++) {
            float cx = 32 + (h01(i, 1, 301) - 0.5f) * 36, cy = 59 - 4 * h01(i, 2, 301), a = (h01(i, 3, 301) - 0.5f) * 2.0f, l = 4 + 5 * h01(i, 4, 301);
            cv_bone(cx - cosf(a) * l, cy - sinf(a) * l, cx + cosf(a) * l, cy + sinf(a) * l, 1.1f, 0.25f, 0.9f);
        }
        for (int i = 0; i < 5; i++) cv_line(30 + i * 3, 52, 28 + i * 3, 58, sh(R_BONE, 0.7f));   /* the ribs, fallen in */
        dog_skull(20, 52, 2.0f, 0);
        cv_bone_decay(303);
        cv_outline(0); cv_store(id); return;
    }
    float bite = pose == 2 ? 1.0f : 0.0f, bob = (pose == 0 ? 1.0f : 0.0f) * w;
    cv_flat(32, 62, 20, 2.0f, R_WARM + 1);
    /* the far legs, thinner and darker */
    cv_capsule(44, 32 + bob, 48 - w * 2, 44, 1.2f, R_BONE, 0.15f, 0.5f); cv_capsule(48 - w * 2, 44, 45 - w * 3, 58, 1.0f, R_BONE, 0.15f, 0.5f);
    cv_capsule(30, 37 + bob, 33 + w * 2, 48, 1.2f, R_BONE, 0.15f, 0.5f); cv_capsule(33 + w * 2, 48, 32 + w * 3, 60, 1.0f, R_BONE, 0.15f, 0.5f);
    /* the tail, up and wagging for no one */
    cv_capsule(46, 29 + bob, 52, 23 - w, 1.1f, R_BONE, 0.3f, 0.8f); cv_capsule(52, 23 - w, 57, 21 + w, 0.9f, R_BONE, 0.3f, 0.8f);
    /* the spine and the cage of ribs hanging from it */
    for (int i = 0; i < 9; i++) cv_ellipse(24 + i * 2.5f, 30 + bob - sinf(i / 8.0f * PI_F) * 2.0f, 1.5f, 1.3f, R_BONE, 0.35f, 0.95f);
    for (int i = 0; i < 5; i++) {
        float x = 27 + i * 3.0f, top = 31 + bob - sinf((x - 24) / 20.0f * PI_F) * 2.0f, drop = 10 - fabsf(i - 1.5f) * 1.5f;
        cv_capsule(x, top, x - 2.5f, top + drop * 0.6f, 0.8f, R_BONE, 0.3f, 0.85f);
        cv_capsule(x - 2.5f, top + drop * 0.6f, x - 1.0f, top + drop, 0.7f, R_BONE, 0.25f, 0.75f);
    }
    cv_ellipse(43, 33 + bob, 4.0f, 3.0f, R_BONE, 0.3f, 0.85f);      /* the pelvis */
    /* the near legs */
    cv_bone(42, 34 + bob, 39 + w * 2, 46, 1.2f, 0.3f, 0.9f); cv_capsule(39 + w * 2, 46, 42 + w, 52, 1.1f, R_BONE, 0.3f, 0.85f);
    cv_capsule(42 + w, 52, 40 + w * 3, 61, 1.0f, R_BONE, 0.3f, 0.85f); cv_ellipse(39 + w * 3, 61.5f, 2.2f, 1.2f, R_BONE, 0.3f, 0.8f);
    cv_bone(27, 37 + bob, 24 - w * 2, 48, 1.3f, 0.3f, 0.95f); cv_capsule(24 - w * 2, 48, 25 - w * 3, 60, 1.1f, R_BONE, 0.3f, 0.9f);
    cv_ellipse(24 - w * 3, 61.5f, 2.4f, 1.2f, R_BONE, 0.3f, 0.85f);
    /* the collar, spiked, and the head */
    float hx = 18, hy = 26 + bob + bite * 6.0f;
    for (int a = 0; a < 20; a++) {
        float ang = TAU_F * a / 20.0f, sz = sinf(ang);
        if (sz < -0.3f) continue;
        int x = (int)(23 + 4.5f * cosf(ang)), y = (int)(hy + 5 + 2.0f * sz);
        cv_px(x, y, sh(R_IRON, 0.3f + 0.4f * sz)); cv_px(x, y + 1, sh(R_IRON, 0.2f));
        if (a % 4 == 1 && sz > 0.2f) cv_px(x, y + 2, sh(R_IRON, 0.85f));
    }
    for (int k = 0; k < 4; k++)                                     /* a chain from the collar, broken off */
        cv_px(24 + k / 2, (int)(hy + 8 + k * 1.6f), sh(R_IRON, (k & 1) ? 0.65f : 0.3f)), cv_px(25 + k / 2, (int)(hy + 9 + k * 1.6f), sh(R_IRON, 0.2f));
    dog_skull(hx, hy, bite ? 7.0f : 1.0f + (w > 0 ? 0.6f : 0.0f), 1);
    cv_bone_decay(305);
    cv_outline(0);
    cv_store(id);
}

/* the bone hound from behind (1.0.0b18): its tail up, the pelvis and the hind legs near and large, the cage of ribs going away,
 * the forelegs beyond, the collar and the back of its skull past the spine */
static void spr_hound_back(int id, float w) {
    cv_begin(cv_sprite, TS, TS);
    cv_flat(32, 62, 14, 2.0f, R_WARM + 1);
    float bob = w;
    for (int s = -1; s <= 1; s += 2) {                              /* the forelegs, beyond */
        float k = -s * w;
        cv_capsule(32 + s * 5, 34 + bob, 32 + s * 6 + k, 47, 1.0f, R_BONE, 0.12f, 0.45f);
        cv_capsule(32 + s * 6 + k, 47, 32 + s * 6 + k * 1.5f, 59, 0.9f, R_BONE, 0.12f, 0.45f);
        cv_ellipse(32 + s * 6 + k * 1.5f, 60.5f, 1.8f, 1.0f, R_BONE, 0.12f, 0.45f);
    }
    for (int y = 22; y <= 32; y++) {                                /* the back of its skull, low past the spine: the crest of the occiput */
        float hw = 4.0f - fabsf(y - 26.5f) * 0.55f;
        for (int x = (int)(32 - hw); x <= (int)(32 + hw); x++) cv_px(x, (int)(y + bob), sh(R_BONE, 0.3f + 0.4f * (1.0f - fabsf(x + 0.5f - 32) / (hw + 0.5f)) - (y - 22) * 0.02f));
    }
    for (int a = 0; a < 20; a++) {                                  /* the spiked collar, round the neck */
        float ang = TAU_F * a / 20.0f, sz = sinf(ang);
        int x = (int)(32 + 4.5f * cosf(ang)), y = (int)(31 + bob + 1.6f * sz);
        cv_px(x, y, sh(R_IRON, 0.25f + 0.35f * (sz + 1.0f) * 0.5f));
        if (a % 4 == 1 && sz < -0.2f) cv_px(x, y - 1, sh(R_IRON, 0.85f));
    }
    for (int i = 0; i < 5; i++) {                                   /* the ribs, going away from you, smaller as they go */
        float y = 39 - i * 1.6f + bob, rx = 7.0f - i * 0.6f, ry = 4.0f - i * 0.4f;
        for (int k = 0; k <= 16; k++) {
            float a = PI_F * (0.1f + 0.8f * k / 16.0f), x = 32 + cosf(a) * rx, yy = y + sinf(a) * ry - ry;
            cv_px((int)x, (int)yy, sh(R_BONE, 0.55f - i * 0.07f));
        }
    }
    for (int i = 0; i < 5; i++) cv_ellipse(32, 34.5f + bob - i * 1.3f, 1.3f - i * 0.12f, 0.9f, R_BONE, 0.4f, 0.95f);   /* the spine, receding */
    cv_ellipse(32, 38 + bob, 5.5f, 3.2f, R_BONE, 0.3f, 0.9f);       /* the pelvis, its wings */
    cv_flat(29.5f, 38.5f + bob, 0.9f, 0.9f, 0); cv_flat(34.5f, 38.5f + bob, 0.9f, 0.9f, 0);
    cv_capsule(32, 36 + bob, 33, 28 - w, 1.1f, R_BONE, 0.35f, 0.85f);   /* the tail, up and wagging for no one */
    cv_capsule(33, 28 - w, 32 + w * 2.0f, 21, 0.9f, R_BONE, 0.35f, 0.85f);
    for (int s = -1; s <= 1; s += 2) {                              /* the hind legs, near: thigh, the hock high, the paw */
        float k = s * w;
        cv_bone(32 + s * 4, 39 + bob, 32 + s * 7 - k, 47, 1.3f, 0.3f, 0.95f);
        cv_capsule(32 + s * 7 - k, 47, 32 + s * 6 - k, 53, 1.1f, R_BONE, 0.3f, 0.85f);
        cv_ellipse(32 + s * 6 - k, 53, 1.5f, 1.3f, R_BONE, 0.4f, 0.95f);   /* the hock */
        cv_capsule(32 + s * 6 - k, 53, 32 + s * 6 - k * 1.5f, 60.5f, 1.0f, R_BONE, 0.3f, 0.85f);
        cv_ellipse(32 + s * 6 - k * 1.5f, 61.5f, 2.2f, 1.2f, R_BONE, 0.3f, 0.8f);
    }
    cv_bone_decay(307);
    cv_outline(0);
    cv_store(id);
}
static void hound_dress_back(int id) {                             /* from behind: moss, frost, an arrow in its flank, char and a chain, gold */
    for (int r = RK_II; r < NRANK; r++) {
        dress_begin(id);
        if (r == RK_II) kit_moss(0.3f, 2, 545);
        if (r == RK_III) kit_frost(0.8f, 547);
        if (r >= RK_IV && r != RK_GILDED) { kit_arrow(27, 37, -2.4f, 7.0f); kit_blood(27, 38, 6, 1, 549); }
        if (r == RK_V) { kit_char(0.3f, 551); kit_chain(29, 32, 22, 44, 3.0f, R_IRON); }
        if (r == RK_GILDED) { kit_gold(R_BONE, 0.5f, 553); for (int a = 0; a < 20; a++) { float ang = TAU_F * a / 20.0f; cv_px((int)(32 + 4.5f * cosf(ang)), (int)(32 + 1.6f * sinf(ang)), (uint8_t)(R_GOLD + 9)); } }
        dress_store(id, r, R_BONE);
    }
}
/* the bone hound's dressing by rank: II a strip of hide from its collar and moss in its bones, III frost and its skull
 * cracked, IV an arrow in its ribs and old blood about its jaws, V charred under a spiked iron muzzle-strap and its
 * master's chain, gilded a gold collar and gold leaf */
static void hound_dress(int id, float w, int pose) {
    float bob = (pose == 0 ? 1.0f : 0.0f) * w, hy = 26 + bob + (pose == 2 ? 6.0f : 0.0f);
    for (int r = RK_II; r < NRANK; r++) {
        dress_begin(id);
        if (r != RK_GILDED) kit_rags(21, hy + 6, 26, hy + 7, r == RK_II ? 6.0f : 8.0f, w * 0.5f, R_WOOD, 0.1f, 0.35f, 0, 531);
        if (r == RK_II) kit_moss(0.3f, 2, 533);
        if (r == RK_III) kit_frost(0.8f, 535);
        if (r >= RK_III && r != RK_GILDED) { cv_line(21, hy - 4, 23, hy, 0); cv_px(24, (int)hy - 1, 0); }   /* the skull cracked */
        if (r >= RK_IV && r != RK_GILDED) { kit_arrow(33, 33 + bob, -1.2f, 8.0f); kit_blood(12, hy + 2, 6, 4, 537); kit_blood(33, 34 + bob, 7, 1, 539); }
        if (r == RK_V) { kit_char(0.3f, 541); cv_line(10, hy, 18, hy - 3, sh(R_IRON, 0.55f)); cv_line(10, hy + 1, 18, hy - 2, sh(R_IRON, 0.25f)); kit_chain(24, hy + 8, 30, hy + 22, 3.0f, R_IRON); }
        if (r == RK_GILDED) { kit_gold(R_BONE, 0.5f, 543); for (int a = 0; a < 20; a++) { float ang = TAU_F * a / 20.0f, sz = sinf(ang); if (sz >= -0.3f) cv_px((int)(23 + 4.5f * cosf(ang)), (int)(hy + 5 + 2.0f * sz), (uint8_t)(R_GOLD + 8 + (int)(3.0f * sz))); } }
        dress_store(id, r, R_BONE);
    }
}

/* The skeleton knight: plate over bone, a two-handed sword with a steel blade (R_GHOST, so the
 * plate's rank leaves it alone). pose 0-1 walking, 2 the long wind-up, 3 the blow. state: 0 full
 * plate, 1 helm and half the plate hacked off, 2 down to a cracked breastplate. */
static void plate(float x0, float y0, float x1, float y1, float r, float dent) {
    cv_capsule(x0, y0, x1, y1, r, R_IRON, 0.25f - dent, 1.0f - dent);
}
static void sword(float gx, float gy, float a) {
    float dx = cosf(a), dy = sinf(a), px = -dy, py = dx;
    cv_ellipse(gx - dx * 6.0f, gy - dy * 6.0f, 1.8f, 1.8f, R_IRON, 0.4f, 1.0f);   /* pommel */
    cv_capsule(gx - dx * 5.0f, gy - dy * 5.0f, gx + dx * 3.0f, gy + dy * 3.0f, 1.2f, R_WOOD, 0.2f, 0.6f);   /* the grip */
    cv_capsule(gx + dx * 3.5f - px * 6.0f, gy + dy * 3.5f - py * 6.0f, gx + dx * 3.5f + px * 6.0f, gy + dy * 3.5f + py * 6.0f, 1.1f, R_IRON, 0.35f, 0.95f);
    for (int i = 4; i < 40; i++) {                                  /* the blade, a bright edge and a fuller down it */
        float t = (float)i, wdt = i > 34 ? (40 - i) * 0.35f : 1.9f;
        for (float o = -wdt; o <= wdt; o += 0.5f) {
            float v = 0.55f + 0.4f * (o / (wdt + 0.01f)) * -0.5f + (fabsf(o) < 0.4f ? -0.15f : 0.0f) + (o > wdt - 0.6f ? 0.35f : 0.0f);
            cv_px((int)(gx + dx * t + px * o), (int)(gy + dy * t + py * o), sh(R_GHOST, v));
        }
    }
}
/* the great helm, by hand (docs/InitialArtPass.md section 8): a ridged crest and a plume's stump (R_BLOOD), rivets, a
 * slit in a cross with a crimson light in it, breath-holes below; hex R_IRON (PF_ARMOUR) */
static const char *const KNIGHT_HELM[16] = {
    "......mpm......", ".....mrqrm.....", "....59bdb95....", "...69bcecb96...", "..68abcecba86..", ".578abbdbba875.",
    ".6fa7bbdbb7af6.", ".6f0@00c00@0f6.", ".589a9a0a9a985.", ".57898a0a89875.", ".5680908090865.", ".5679a9a9a9765.",
    ".4680908090864.", ".3f6789a9876f3.", "..24567876542..", "...234565432...",
};
/* the helm from behind (1.0.0b18): the crest running down to the nape, the plume's stump falling down it, rivets, the seam */
static const char *const KNIGHT_HELM_BACK[16] = {
    "......mpm......",
    ".....mrqrm.....",
    "....59bnb95....",
    "...69bcocb96...",
    "..68abcncba86..",
    ".578abbmbba875.",
    ".6fa7bblbb7af6.",
    ".6f9999k9999f6.",
    ".589a9a6a9a985.",
    ".5f898a6a898f5.",
    ".5687976797865.",
    ".5679a969a9765.",
    ".4f879767978f4.",
    ".3f675969576f3.",
    "..24567876542..",
    "...234565432...",
};
static void spr_knight(int id, float w, int pose, int state) {
    cv_begin(cv_sprite, TS, TS);
    int back = g_back;                                              /* 1.0.0b18: its back, drawn as from the front and turned about */
    if (back) { pose &= 1; sword(37, 34, -1.05f); }                 /* the sword held before it: only the blade past its shoulder shows */
    float gx, gy, ga;                                               /* where the hands grip, and where the blade points */
    if (pose == 2) { gx = 41; gy = 11; ga = -2.9f; }
    else if (pose == 3) { gx = 27; gy = 40; ga = 2.55f; }
    else if (pose == 4) { gx = 32; gy = 25; ga = 1.5708f; }        /* planted: both hands on the pommel, the point in the floor */
    else { gx = 37; gy = 34; ga = -1.05f; }
    cv_flat(32, 62, 16, 2.0f, R_WARM + 1);
    if (pose == 2 && !back) sword(gx, gy, ga);                      /* raised behind the head */
    /* legs: bone, with plate on the thighs and shins unless it has been hacked away */
    for (int s = -1; s <= 1; s += 2) {
        float k = s * w, hx = 32 + s * 4.0f, kx = 32 + s * 5.0f - k * 2.5f, fx = 32 + s * 5.5f - k * 4.0f;
        if (pose == 3 || pose == 4) { kx += s * 1.5f; fx += s * 3.0f; }   /* braced for the blow, or the shout */
        cv_bone(hx, 41, kx, 51, 1.6f, 0.25f, 0.95f);
        cv_bone(kx, 51, fx, 60, 1.4f, 0.25f, 0.9f);
        if (state < 1 || (state < 2 && s > 0)) plate(hx, 42, kx, 50, 3.0f, 0.0f);
        if (state < 2) { plate(kx, 52, fx, 59, 2.6f, 0.0f); cv_ellipse(kx, 51, 2.6f, 2.2f, R_IRON, 0.4f, 1.0f); }
        if (state < 2) cv_ellipse(fx - s * 1.0f, 61.5f, 3.6f, 1.6f, R_IRON, 0.3f, 0.9f);
        else cv_capsule(fx, 61.5f, fx - s * 3.5f, 62, 1.0f, R_BONE, 0.3f, 0.8f);
    }
    /* the spine and ribs under the plate, which show where it is gone */
    cv_capsule(32, 18, 32, 40, 1.4f, R_BONE, 0.3f, 0.9f);
    for (int i = 0; i < 4; i++) {
        float y = 21 + i * 3.6f;
        cv_capsule(31, y, 25 + i * 0.6f, y + 2, 0.9f, R_BONE, 0.25f, 0.9f); cv_capsule(33, y, 39 - i * 0.6f, y + 2, 0.9f, R_BONE, 0.25f, 0.9f);
    }
    cv_ellipse(32, 40, 7, 3.4f, R_BONE, 0.25f, 0.9f);
    /* the fauld: bands of plate over the hips */
    if (state < 2)
        for (int b = 0; b < 3; b++) {
            float y = 37 + b * 2.6f, hw = 9.5f + b * 0.8f;
            for (int x = (int)(32 - hw); x <= (int)(32 + hw); x++) {
                if (state == 1 && x < 28 && b > 0) continue;
                float nx = (x + 0.5f - 32) / hw;
                for (int yy = 0; yy < 3; yy++) cv_px(x, (int)y + yy, sh(R_IRON, 0.3f + 0.5f * lit(nx, -0.2f, sqrtf(fmaxf(0, 1 - nx * nx))) - yy * 0.08f));
            }
        }
    if (state < 2) {                                                /* the Order's tabard below the plate, its cross on it; torn once hacked */
        for (int y = 41; y < 55; y++) {
            float hw = 5.0f + (y - 41) * 0.15f;
            for (int x = (int)(32 - hw); x <= (int)(32 + hw); x++) {
                if (y > 51 && h01(x, y, 315) < (y - 51) / 4.0f) continue;   /* the hem frayed */
                if (state == 1 && x > 32 - (y - 41) * 0.5f + 4) continue;   /* hacked away on the one side */
                float fold = sinf((x - 32) * 1.3f) * 0.06f;
                cv_px(x, y, sh(R_BLOOD, 0.22f + fold - (y - 41) * 0.006f));
            }
        }
        for (int k = -2; k <= 2; k++) { cv_px(32 + k, 46, sh(R_BONE, 0.75f)); cv_px(32, 46 + k, sh(R_BONE, 0.75f)); }   /* the cross, on both sides of it */
        cv_px(30, 44, sh(R_BONE, 0.6f)); cv_px(34, 44, sh(R_BONE, 0.6f)); cv_px(30, 48, sh(R_BONE, 0.6f)); cv_px(34, 48, sh(R_BONE, 0.6f));
    }
    /* the breastplate, with a ridge down it; hacked and split as the knight is worn down */
    for (int y = 18; y <= 37; y++) {
        float hw = 9.0f + 1.5f * sinf((y - 18) / 19.0f * PI_F) - (y > 31 ? (y - 31) * 0.6f : 0.0f);
        for (int x = (int)(32 - hw); x <= (int)(32 + hw); x++) {
            float nx = (x + 0.5f - 32) / hw;
            if (state == 2 && (x < 30 - (y - 18) * 0.15f || y > 32)) continue;   /* the left half and the belly gone */
            float v = 0.25f + 0.6f * lit(nx, (y - 26) / 14.0f, sqrtf(fmaxf(0.0f, 1.0f - nx * nx)));
            if (back) {                                             /* the backplate: a channel down the spine, the blades swelling either side */
                if (fabsf(x + 0.5f - 32) < 0.8f) v -= 0.18f;
                else if (y < 28 && fabsf(fabsf(x + 0.5f - 32) - 4.5f) < 2.0f) v += 0.1f;
                if (y == 30 && fabsf(x + 0.5f - 32) < hw - 1.0f) v -= 0.15f;   /* the lames' join */
            } else if (fabsf(x + 0.5f - 32) < 0.8f) v += 0.25f;
            if (state >= 1 && h01(x / 2, y / 2, 311) > 0.86f) v -= 0.3f;    /* dents */
            cv_px(x, y, sh(R_IRON, v));
        }
    }
    if (state == 2) cv_line(30, 19, 33, 33, sh(R_IRON, 0.05f));
    for (int k = 0; k < 4; k++) { cv_px(24, 21 + k * 4, (uint8_t)(R_IRON + 14)); if (state < 2) cv_px(40, 21 + k * 4, (uint8_t)(R_IRON + 13)); }   /* rivets down the plate's edges */
    /* arms: both hands on the grip (from behind, only the upper arm and the pauldron) */
    for (int s = -1; s <= 1; s += 2) {
        if (back) {
            float sx = 32 + s * 9.0f, sy = 20;
            cv_bone(sx, sy, sx + s * 1.5f, sy + 9.0f, 1.3f, 0.2f, 0.75f);
            if (state < 2) plate(sx + s * 0.5f, sy + 3.0f, sx + s * 1.5f, sy + 9.0f, 2.2f, 0.1f);   /* the rerebrace */
            int keep = state == 0 || s > 0;
            if (keep) { cv_ellipse(sx, sy - 0.5f, 5.0f, 4.0f, R_IRON, 0.3f, 1.0f); cv_px((int)sx - 2, (int)sy - 3, (uint8_t)(R_IRON + 15)); cv_px((int)sx + 2, (int)sy - 3, (uint8_t)(R_IRON + 15)); }
            continue;
        }
        float sx = 32 + s * 9.0f, sy = 20;
        float ex = (sx + gx) * 0.5f + s * 4.0f, ey = (sy + gy) * 0.5f + 4.0f;
        if (pose == 2) { ex = (sx + gx) * 0.5f + s * 3.0f; ey = (sy + gy) * 0.5f + 3.0f; }
        cv_bone(sx, sy, ex, ey, 1.3f, 0.25f, 0.9f);
        cv_bone(ex, ey, gx + s * 1.0f, gy, 1.1f, 0.25f, 0.9f);
        int keep = state == 0 || (state == 1 && s > 0) || (state == 2 && s > 0);
        if (keep) { cv_ellipse(sx, sy - 0.5f, 5.0f, 4.0f, R_IRON, 0.3f, 1.0f); cv_px((int)sx - 2, (int)sy - 3, (uint8_t)(R_IRON + 15)); cv_px((int)sx + 2, (int)sy - 3, (uint8_t)(R_IRON + 15)); cv_px((int)sx, (int)sy + 2, (uint8_t)(R_IRON + 14)); }   /* pauldron, riveted */
        if (state < 2) plate(ex, ey, gx + s * 1.0f, gy, 2.0f, 0.05f);          /* vambrace */
    }
    if (pose != 2 && !back) sword(gx, gy, ga);
    if (!back) for (int s = -1; s <= 1; s += 2) cv_ellipse(gx + s * 1.2f, gy + 0.5f, 2.2f, 2.0f, state < 2 ? R_IRON : R_BONE, 0.3f, 0.9f);   /* gauntlets, or bare bone */
    /* the head: a great helm with a slit, and two embers in it; or the bare skull once the helm is off */
    cv_capsule(32, 15, 32, 18, 1.2f, R_BONE, 0.3f, 0.9f);
    if (state == 0) grid_stamp(back ? KNIGHT_HELM_BACK : KNIGHT_HELM, 15, 16, 25, 1, R_IRON, R_BLOOD, (uint8_t)(R_FLAME + 9), 0);   /* the great helm */
    else {                                                          /* the helm hacked off: the skull under it */
        skull_stamp(32, 10, 0, 0, back ? 0 : (uint8_t)(R_FLAME + 9));
        if (state == 1) for (int x = 25; x <= 39; x++) { cv_px(x, 2 + (x - 32) * (x - 32) / 12, sh(R_IRON, 0.5f)); cv_px(x, 3 + (x - 32) * (x - 32) / 12, sh(R_IRON, 0.25f)); }   /* the helm's rim, all that is left */
    }
    cv_bone_decay(313);
    if (back) cv_mirror();
    cv_outline(0);
    cv_store(id);
}

/* the knight's dressing from behind: rust and moss, frost and dents, an arrow in the backplate and its blood, char under the
 * crown and a chain, gilt edges */
static void knight_dress_back(int id, int state) {
    for (int r = RK_II; r < NRANK; r++) {
        dress_begin(id);
        if (r == RK_II) {
            kit_moss(0.28f, 0, 571);
            for (int y = 0; y < TS; y++) for (int x = 0; x < TS; x++) { uint8_t b = base_c(x, y); if (b > R_IRON && b < R_IRON + 16 && fbm(x / 3.0f, y / 3.0f, 16, 573) > 0.7f) cv_px(x, y, sh(R_BRICK, 0.3f + 0.2f * h01(x, y, 575))); }
        }
        if (r == RK_III) kit_frost(0.8f, 577);
        if (r >= RK_III && r != RK_GILDED) for (int k = 0; k < 5; k++) cv_px(26 + (int)(h01(k, 1, 579) * 12.0f), 20 + (int)(h01(k, 2, 579) * 14.0f), sh(R_IRON, 0.05f));
        if (r >= RK_IV && r != RK_GILDED) { kit_arrow(36, 26, -0.9f, 8.0f); kit_blood(36, 27, 8, 1, 581); if (state < 2) kit_arrow(27, 31, -2.2f, 6.0f); }
        if (r == RK_V) { kit_char(0.3f, 583); kit_crown(32, state == 0 ? 3 : 2, 13, 585); kit_chain(41, 22, 44, 36, 1.5f, R_IRON); }
        if (r == RK_GILDED)
            for (int y = 0; y < TS; y++) for (int x = 0; x < TS; x++) {
                uint8_t b = base_c(x, y);
                if (b > R_IRON && b < R_IRON + 16 && (b - R_IRON >= 12 || (!base_on(x - 1, y) || !base_on(x + 1, y) || !base_on(x, y - 1)))) cv_px(x, y, (uint8_t)(R_GOLD + clampi(b - R_IRON - 2, 5, NGOLD - 1)));
            }
        dress_store(id, r, R_IRON);
    }
}

/* the knight's dressing by rank: II rust and moss on the plate, III frost on it and dents, IV a broken blade in the
 * breastplate and an arrow in the pauldron, old blood from them, V charred plate under an iron crown and a chain,
 * gilded gilt on the plate's edges and rivets (docs/InitialArtPass.md 6.2) */
static void knight_dress(int id, int state) {
    for (int r = RK_II; r < NRANK; r++) {
        dress_begin(id);
        if (r == RK_II) {
            kit_moss(0.28f, 0, 551);
            for (int y = 0; y < TS; y++) for (int x = 0; x < TS; x++) { uint8_t b = base_c(x, y); if (b > R_IRON && b < R_IRON + 16 && fbm(x / 3.0f, y / 3.0f, 16, 553) > 0.7f) cv_px(x, y, sh(R_BRICK, 0.3f + 0.2f * h01(x, y, 555))); }   /* rust */
        }
        if (r == RK_III) kit_frost(0.8f, 557);
        if (r >= RK_III && r != RK_GILDED) for (int k = 0; k < 5; k++) cv_px(26 + (int)(h01(k, 1, 559) * 12.0f), 20 + (int)(h01(k, 2, 559) * 14.0f), sh(R_IRON, 0.05f));   /* dents */
        if (r >= RK_IV && r != RK_GILDED) {
            if (state < 2) { cv_line(35, 24, 41, 18, sh(R_GHOST, 0.7f)); cv_line(36, 25, 42, 19, sh(R_GHOST, 0.45f)); cv_line(41, 18, 44, 15, sh(R_WOOD, 0.4f)); kit_blood(35, 26, 9, 2, 561); }   /* a broken blade lodged */
            kit_arrow(22, 18, -2.3f, 7.0f);
        }
        if (r == RK_V) { kit_char(0.3f, 563); kit_crown(32, state == 0 ? 3 : 2, 13, 565); kit_chain(23, 22, 20, 36, 1.5f, R_IRON); }
        if (r == RK_GILDED)
            for (int y = 0; y < TS; y++) for (int x = 0; x < TS; x++) {
                uint8_t b = base_c(x, y);
                if (b > R_IRON && b < R_IRON + 16 && (b - R_IRON >= 12 || (!base_on(x - 1, y) || !base_on(x + 1, y) || !base_on(x, y - 1)))) cv_px(x, y, (uint8_t)(R_GOLD + clampi(b - R_IRON - 2, 5, NGOLD - 1)));   /* gilt edges */
            }
        dress_store(id, r, R_IRON);
    }
}

static void spr_knight_dead(int id) {
    cv_begin(cv_sprite, TS, TS);
    cv_flat(32, 61.5f, 26, 2.5f, R_WARM + 1);
    for (int i = 0; i < 6; i++) {
        float cx = 32 + (h01(i, 1, 321) - 0.5f) * 34, cy = 59 - 3 * h01(i, 2, 321), a = (h01(i, 3, 321) - 0.5f) * 1.6f, l = 5 + 4 * h01(i, 4, 321);
        cv_bone(cx - cosf(a) * l, cy - sinf(a) * l, cx + cosf(a) * l, cy + sinf(a) * l, 1.3f, 0.25f, 0.9f);
    }
    cv_ellipse(30, 56, 9, 5, R_IRON, 0.2f, 0.9f);                  /* the breastplate, on its back */
    cv_line(30, 52, 30, 60, sh(R_IRON, 0.9f));
    cv_ellipse(47, 57, 5, 4, R_IRON, 0.2f, 0.85f);                 /* a pauldron */
    for (int y = 49; y <= 61; y++)                                  /* the helm, rolled on its side */
        for (int x = 9; x <= 21; x++) {
            float nx = (x + 0.5f - 15) / 6.5f, ny = (y + 0.5f - 55) / 6.5f;
            if (nx * nx + ny * ny > 1.0f) continue;
            float v = 0.25f + 0.6f * lit(nx, ny, sqrtf(fmaxf(0, 1 - nx * nx - ny * ny)));
            if (fabsf(x + 0.5f - 13.0f) < 0.8f && ny > -0.6f) v = 0.0f;   /* the slit, now upright */
            cv_px(x, y, sh(R_IRON, v));
        }
    for (int i = 4; i < 34; i++) cv_px(20 + i, 62 - i / 8, sh(R_GHOST, 0.7f - (i & 1) * 0.15f));   /* the sword, flat on the floor */
    cv_bone_decay(323);
    cv_outline(0);
    cv_store(id);
}

/* The deacon: a skeleton priest in a hooded violet robe (PF_ROBE) with a red stole, a staff with a
 * skull on it. pose 0-1 walking, 2 raising the dead, 3 fallen. */
static void spr_deacon(int id, float w, int pose) {
    cv_begin(cv_sprite, TS, TS);
    if (pose == 3) {
        cv_flat(32, 61.5f, 24, 2.5f, R_WARM + 1);
        float m[12] = { 10, 62, 16, 52, 28, 48, 42, 50, 54, 57, 56, 62 };
        cv_poly(m, 6, R_PURPLE, 0.45f, 0.1f, 48, 14);
        cv_capsule(20, 60, 50, 55, 1.2f, R_BLOOD, 0.15f, 0.45f);
        cv_capsule(6, 61, 58, 59, 1.1f, R_WOOD, 0.2f, 0.6f);        /* the staff, across */
        grid_stamp(SKULL_SMALL, 9, 10, 26, 45, R_BONE, R_BONE, 0, 0);
        cv_bone_decay(331);
        cv_outline(0); cv_store(id); return;
    }
    int back = g_back;                                              /* 1.0.0b18: from behind, walking; drawn as from the front and turned */
    if (back) pose &= 1;
    int chant = pose == 2;
    cv_flat(32, 62, 15, 1.8f, R_WARM + 1);
    if (chant)                                                      /* the dead's light, rising round it */
        for (int i = 0; i < 26; i++) {
            float x = 32 + (h01(i, 1, 333) - 0.5f) * 44, y = 30 + 32 * h01(i, 2, 333);
            cv_px((int)x, (int)y, (uint8_t)(R_GLOW + 6 + (int)(8 * h01(i, 3, 333))));
        }
    /* the staff, in its right hand */
    float stx = chant ? 47.0f : 45.0f + w * 0.5f, sty = chant ? 0.0f : 6.0f;
    cv_capsule(stx, sty + 6, stx - 1, 62, 1.2f, R_WOOD, 0.2f, 0.65f);
    grid_stamp(back ? SKULL_SMALL_BACK : SKULL_SMALL, 9, 10, (int)stx - 4, (int)sty - 2, R_BONE, R_BONE, 0, 0);   /* the staff's skull, its eyes its own (not the deacon's) */
    if (!back) { cv_px((int)stx - 2, (int)sty + 1, (uint8_t)(chant ? R_GLOW + 14 : R_FLAME + 6)); cv_px((int)stx + 2, (int)sty + 1, (uint8_t)(chant ? R_GLOW + 14 : R_FLAME + 6)); }
    /* the robe, falling from the shoulders to a hem that drags */
    for (int y = 16; y < 63; y++) {
        float t = (y - 16) / 46.0f, hw = 6.5f + 6.0f * t, sway = w * 1.2f * t;   /* taller and thinner */
        for (int x = (int)(32 - hw + sway) - 1; x <= (int)(32 + hw + sway) + 1; x++) {
            float dx = x + 0.5f - 32 - sway;
            if (fabsf(dx) > hw) continue;
            if (y > 58 && h01(x, y, 335 + (w > 0)) < (y - 58) / 5.0f) continue;   /* ragged hem */
            float u = dx / hw, v = 0.15f + 0.4f * (1.0f - u * u) * (1.0f - 0.3f * t);
            if (sinf(dx * 1.7f + y * 0.02f) > 0.55f) v -= 0.12f;    /* long folds, falling straight */
            cv_px(x, y, sh(R_PURPLE, v));
        }
    }
    if (back) for (int x = 27; x <= 37; x++) { cv_px(x, 19 + abs(x - 32) / 3, sh(R_BLOOD, 0.3f)); cv_px(x, 20 + abs(x - 32) / 3, sh(R_BLOOD, 0.18f)); }   /* the stole over the nape */
    for (int s = -1; s <= 1 && !back; s += 2)                       /* the stole down the front */
        for (int y = 20; y < 52; y++) {
            int x = 32 + s * 3 + (int)(w * 1.0f * (y - 16) / 46.0f);
            cv_px(x, y, sh(R_BLOOD, 0.3f + 0.15f * (s > 0))); cv_px(x + s, y, sh(R_BLOOD, 0.22f));
            if (y == 49 || y == 50) { cv_px(x - s, y, sh(R_BONE, 0.7f)); }
        }
    /* arms: sleeves, and bone hands out of them */
    for (int s = -1; s <= 1; s += 2) {
        float sx = 32 + s * 7, sy = 20, hx, hy;
        if (chant) { hx = 32 + s * 17; hy = 6; }
        else if (s > 0) { hx = stx - 1; hy = 34; }
        else { hx = 21 - w; hy = 38; }
        float ex = (sx + hx) * 0.5f + s * 3.0f, ey = (sy + hy) * 0.5f + 2.0f;
        if (chant) { cv_bone(sx, sy, ex, ey, 1.2f, 0.25f, 0.9f); cv_bone(ex, ey, hx, hy + 2, 1.0f, 0.25f, 0.9f); }   /* the sleeves fallen back */
        cv_capsule(sx, sy, chant ? sx + s * 2.0f : ex, chant ? sy + 4.0f : ey, 3.4f, R_PURPLE, 0.15f, 0.55f);
        if (!chant) cv_capsule(ex, ey, hx, hy - 1, 3.0f, R_PURPLE, 0.15f, 0.5f);
        cv_ellipse(hx, hy, 1.9f, 1.8f, R_BONE, 0.3f, 0.95f);
        for (int f = 0; f < 3; f++) cv_line(hx, hy, hx + s * (1.0f + f) * (chant ? 0.6f : 0.4f), hy + (chant ? -3.0f : 3.0f) + f * (chant ? 0.0f : 0.5f), sh(R_BONE, 0.8f));
        if (chant) for (int k = 0; k < 6; k++) cv_px((int)(hx + (h01(k, s + 2, 337) - 0.5f) * 8), (int)(hy - 2 - 6 * h01(k, s + 5, 337)), (uint8_t)(R_GLOW + 10 + (k & 3)));
    }
    if (!chant && !back)                                            /* a rosary of knuckle-bones on its cord, a small cross at its end */
        for (int k = 0; k < 9; k++) {
            float x = 25.0f - w * 0.5f + sinf((float)k * 0.5f) * 1.5f, y = 33.0f + (float)k * 1.5f;
            cv_px((int)x, (int)y, (k & 1) ? sh(R_BONE, 0.85f) : sh(R_BONE, 0.55f));
            if (k == 8) { cv_px((int)x, (int)y + 1, sh(R_BONE, 0.8f)); cv_px((int)x, (int)y + 2, sh(R_BONE, 0.7f)); cv_px((int)x - 1, (int)y + 1, sh(R_BONE, 0.7f)); cv_px((int)x + 1, (int)y + 1, sh(R_BONE, 0.7f)); }
        }
    /* the hood, and the skull in its shadow */
    for (int y = 0; y <= 21; y++)                                   /* the hood, peaked in a lancet, its rim catching the light */
        for (int x = 20; x <= 44; x++) {
            float dx = x + 0.5f - 32, hw = y < 13 ? 8.5f * sqrtf(fmaxf(0.0f, (float)y / 13.0f)) : 8.5f + (y - 13) * 0.25f;
            if (fabsf(dx) > hw) continue;
            float u = dx / (hw + 0.01f), v = 0.12f + 0.32f * (1.0f - u * u) - (u > 0.0f ? 0.06f : 0.0f);
            if (fabsf(u) > 0.82f) v += 0.18f;
            cv_px(x, y, sh(R_PURPLE, v));
        }
    if (back) {                                                     /* the hood's back: a seam down its peak, and its tail down the robe */
        for (int y = 2; y <= 20; y++) cv_px(32, y, sh(R_PURPLE, 0.1f));
        for (int y = 18; y <= 34; y++) {
            float hw = 2.6f - (y - 18) * 0.12f;
            for (int x = (int)(32 - hw); x <= (int)(32 + hw); x++) cv_px(x, y, sh(R_PURPLE, 0.3f + 0.12f * (1.0f - fabsf(x + 0.5f - 32) / (hw + 0.5f)) - (y > 31 ? 0.08f : 0.0f)));
            cv_px((int)(32 - hw) - 1, y, sh(R_PURPLE, 0.05f));
        }
    } else {
        for (int y = 6; y <= 19; y++)                               /* its mouth, deep */
            for (int x = 26; x <= 38; x++) { float nx = (x + 0.5f - 32) / 5.2f, ny = (y + 0.5f - 13.0f) / 7.0f; if (nx * nx + ny * ny <= 1.0f) cv_px(x, y, 0); }
        grid_stamp(SKULL_SMALL, 9, 10, 28, 9, R_BONE, R_BONE, (uint8_t)(chant ? R_GLOW + 14 : C_WHITE), 0);   /* the skull, a pale gleam in there, two lights */
        for (int y = 9; y < 19; y++) for (int x = 28; x < 37; x++) { uint8_t c = cv_sprite[y * TS + x]; if (c > R_BONE + 2 && c < R_BONE + 16) cv_px(x, y, (uint8_t)(c - 2)); }   /* in shadow */
    }
    cv_bone_decay(339);
    if (back) cv_mirror();
    cv_outline(0);
    cv_store(id);
}

/* the deacon's dressing by rank: II mildew on the robe, its hem more torn, III frost, IV blood soaked up the hem and the
 * stole, V the robe charred and a black chain of office, gilded gold thread at the robe's edges (the stole, not being the robe,
 * keeps its red: the layer stays off what is not the body) */
static void deacon_dress(int id) {
    for (int r = RK_II; r < NRANK; r++) {
        dress_begin(id);
        if (r == RK_II) kit_moss(0.3f, 0, 571);
        if (r != RK_GILDED) for (int x = 18; x < 47; x++) if (base_on(x, 60) && h01(x, r, 573) < 0.35f) for (int y = 56 + (int)(h01(x, 1, 573) * 4.0f); y < 64; y++) if (base_on(x, y)) cv_px(x, y, 0);   /* the hem torn */
        if (r == RK_III) kit_frost(0.8f, 575);
        if (r >= RK_IV && r != RK_GILDED)
            for (int y = 48; y < 64; y++) for (int x = 16; x < 48; x++) { uint8_t b = base_c(x, y); if (b > R_PURPLE && b < R_PURPLE + 16 && fbm(x / 3.0f, y / 2.0f, 16, 577) > 0.62f - (y - 48) * 0.012f) cv_px(x, y, sh(R_BLOOD, 0.12f + 0.12f * h01(x, y, 579))); }   /* blood soaked up the hem */
        if (r == RK_V) { kit_char(0.3f, 581); kit_chain(24, 21, 40, 21, 4.0f, R_IRON); }
        if (r == RK_GILDED) {
            for (int y = 0; y < TS; y++) for (int x = 0; x < TS; x++) {
                uint8_t b = base_c(x, y);
                if (b > R_PURPLE && b < R_PURPLE + 16 && (!base_on(x - 1, y) || !base_on(x + 1, y))) cv_px(x, y, (uint8_t)(R_GOLD + 8 + (int)(h01(x, y, 583) * 3.0f)));   /* gold thread at the edges */
            }
        }
        dress_store(id, r, R_PURPLE);
    }
}

