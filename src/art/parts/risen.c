/* A part of crawl_art.c (docs/PLAN-tidy.md): it is #included there in its place and never compiled on its own.
 * The bestiary begins: rot and soil; the shambler and the Drowned; the shield-bearer's charge, the spew, dust. */

/* ------------------------------------------------------------------ */
/* The bestiary (docs/PLAN-bestiary.md). A creature's body is drawn in  */
/* its family's ramp (crawl.h, PF_*) so that a rank can recolour it;    */
/* everything else on it keeps its colour.                              */
/* ------------------------------------------------------------------ */

/* move the drawing down by dy and cut it off at the ground, for things climbing out of it */
static void cv_sink(int dy, int ground) {
    EyeAt e = { 0 };
    for (int i = 0; i < g_eyes.n; i++)
        if (g_eyes.y[i] + dy < ground) { e.x[e.n] = g_eyes.x[i]; e.y[e.n] = (int8_t)(g_eyes.y[i] + dy); e.n++; }
    g_eyes = e;
    for (int y = TS - 1; y >= 0; y--)
        for (int x = 0; x < TS; x++) {
            int sy = y - dy;
            cv_sprite[y * TS + x] = (y >= ground || sy < 0) ? C_CLEAR : cv_sprite[sy * TS + x];
        }
}

/* rot on dead skin: darker patches, green mould, and here and there a raw wound */
static void cv_rot(int ramp, float amount, uint32_t seed) {
    for (int y = 0; y < TS; y++)
        for (int x = 0; x < TS; x++) {
            uint8_t *p = &cv_sprite[y * TS + x];
            if (*p < ramp || *p >= ramp + 16) continue;
            float n = fbm(x / 5.0f, y / 5.0f, 16, seed), m = fbm(x / 3.0f, y / 3.0f, 32, seed + 7);
            if (n < 0.28f + 0.08f * amount) *p = (uint8_t)(*p - 2 < ramp + 1 ? ramp + 1 : *p - 2);
            if (m > 0.74f - 0.08f * amount) *p = sh(R_MOSS, 0.15f + 0.3f * (*p - ramp) / 15.0f);
            if (m < 0.2f && n > 0.55f) *p = sh(R_BLOOD, 0.2f + 0.15f * n);
        }
}

/* a dead hand: palm and four fingers hanging or clawing from (x, y), pointing along angle a */
static void dead_hand(float x, float y, float a, float spread, int ramp, float s) {
    cv_ellipse(x, y, 2.4f * s, 2.2f * s, ramp, 0.35f, 0.9f);
    for (int f = 0; f < 4; f++) {
        float fa = a + (f - 1.5f) * spread, l = (3.6f + (f == 1 || f == 2) * 0.8f) * s;
        float mx = x + cosf(fa) * l * 0.6f, my = y + sinf(fa) * l * 0.6f;
        cv_line(x + cosf(fa) * 1.5f * s, y + sinf(fa) * 1.5f * s, mx, my, sh(ramp, 0.7f));
        cv_line(mx, my, mx + cosf(fa + 0.5f) * l * 0.5f, my + sinf(fa + 0.5f) * l * 0.5f, sh(ramp, 0.5f));
    }
}

/* clods and a mound of turned earth in the grave's ramp-free brown */
static void soil(float cx, float w, int ground, uint32_t seed) {
    cv_ellipse(cx, ground + 2.0f, w, 5.0f, R_WOOD, 0.04f, 0.32f);
    for (int i = 0; i < 9; i++) {
        float x = cx + (h01(i, 1, seed) - 0.5f) * w * 2.0f, y = ground - 1.0f + 4.0f * h01(i, 2, seed);
        cv_ellipse(x, y, 1.6f + 1.6f * h01(i, 3, seed), 1.3f + 1.0f * h01(i, 4, seed), R_WOOD, 0.05f, 0.4f);
    }
    for (int i = 0; i < 5; i++) cv_px((int)(cx + (h01(i, 5, seed) - 0.5f) * w * 1.6f), ground - 3 - (int)(5 * h01(i, 6, seed)), sh(R_WOOD, 0.3f));   /* flung earth */
}

/* the shambler's face (docs/InitialArtPass.md section 8): sunken sockets, the nose rotted to two holes, the jaw dropped
 * open on yellowed teeth, hair in clumps; hex R_WARM (PF_FLESH), g-v R_IRON for the hair */
static const char *const SHAMB_HEAD[16] = {
    "...hjjhj.h...", "..hj6787jh...", ".hj689a986j..", ".j6789aa9875.", ".5679aaa9865.", ".46000900064.", ".460@797@064.",
    "..520696025..", "..47a0a0a74..", "..4589a9854..", "..3e0e00e03..", "...2000002...", "...3000003...", "...4d00d04...",
    "....57975....", ".....363.....",
};
/* The shambler: a corpse in rags out of its grave, one arm reaching, the other hanging.
 * pose: 0 walking, 1 grabbing, 2-3 climbing out (head and hands, then to the waist), 4 sinking to
 * its knees, 5 down. Its skin is R_WARM (PF_FLESH). */
static void spr_shambler(int id, float w, int pose) {
    cv_begin(cv_sprite, TS, TS);
    int back = g_back;                                              /* 1.0.0b18: its back (walking), drawn unturned; spr_turn turns it */
    if (back) pose = 0;
    int grab = pose == 1 || pose == 2 || pose == 3;
    if (pose == 5) {                                               /* down: lying along the floor, an arm flung out */
        cv_flat(32, 61.5f, 26, 2.5f, R_WARM + 1);
        cv_capsule(40, 58, 56, 60, 2.6f, R_STONE, 0.1f, 0.4f);     /* legs, bound */
        cv_capsule(40, 60, 55, 62, 2.4f, R_STONE, 0.08f, 0.35f);
        cv_ellipse(30, 57.5f, 11, 4.2f, R_STONE, 0.12f, 0.5f);     /* the shroud over the chest */
        cv_capsule(24, 55, 18, 47, 1.8f, R_WARM, 0.35f, 0.9f);      /* an arm up off the floor */
        dead_hand(17, 46, -1.9f, 0.35f, R_WARM, 1.0f);
        cv_ellipse(14, 58, 5.0f, 4.4f, R_WARM, 0.2f, 0.8f);        /* the head, on its side */
        cv_flat(12, 57, 1.2f, 1.2f, 0); cv_flat(15, 56, 1.2f, 1.2f, 0);
        cv_line(10, 60, 14, 61, sh(R_BLOOD, 0.2f));
        cv_rot(R_WARM, 1.0f, 211);
        cv_outline(0); cv_store(id); return;
    }
    if (pose <= 1) cv_flat(32, 62, 12, 1.6f, R_WARM + 1);
    float kneel = pose == 4 ? 1.0f : 0.0f, sway = w * 0.8f + kneel * 3.0f;
    float hx = 30 + sway + kneel * 4.0f, hy = 13 + (grab ? 1.0f : 0.0f) + kneel * 14.0f;   /* head, tilted over */
    float sy = 23 + kneel * 13.0f;                                  /* shoulders */
    if (!grab && pose != 4) {                                       /* the hanging arm, behind */
        cv_capsule(22 + sway, sy + 1, 20 + w * 1.5f, sy + 10, 1.9f, R_WARM, 0.3f, 0.85f);
        cv_capsule(20 + w * 1.5f, sy + 10, 21 + w * 2.5f, sy + 18, 1.7f, R_WARM, 0.3f, 0.8f);
        dead_hand(21 + w * 2.5f, sy + 19, 1.7f, 0.25f, R_WARM, 0.9f);
    }
    if (pose == 4) {                                                /* on its knees, shins under it */
        cv_capsule(27, 50, 22, 61, 2.6f, R_STONE, 0.1f, 0.4f);
        cv_capsule(37, 50, 42, 61, 2.6f, R_STONE, 0.1f, 0.4f);
        cv_capsule(22, 61, 14, 62, 2.0f, R_WARM, 0.3f, 0.75f);
        cv_capsule(42, 61, 50, 62, 2.0f, R_WARM, 0.3f, 0.75f);
    } else {
        for (int s = -1; s <= 1; s += 2) {                          /* legs: rags to the knee, bare shins, a dragging foot */
            float k = s * w, kx = 32 + s * 4.5f - k * 2.0f, fx = 32 + s * 5.0f - k * 3.5f;
            cv_capsule(32 + s * 3.5f, 42, kx, 52, 2.8f, R_STONE, 0.12f, 0.42f);   /* bound in linen strips */
            cv_capsule(kx, 51, fx, 60.5f, 2.0f, R_WARM, 0.3f, 0.85f);
            cv_ellipse(fx + s * 1.5f, 61.5f, 3.2f, 1.6f, R_WARM, 0.3f, 0.75f);
        }
    }
    /* the torso, grey skin under a rotten shirt torn open over the ribs */
    float bot = pose == 4 ? 52.0f : 44.0f, cx = 31.0f + sway * 0.7f;
    float tp[8] = { 22 + sway, sy, 40 + sway, sy, 37 + sway * 0.5f, bot, 25 + sway * 0.5f, bot };
    cv_poly(tp, 4, R_WARM, 0.85f, 0.4f, sy, 22);
    cv_ellipse(23 + sway, sy + 2, 3.4f, 3.0f, R_WARM, 0.35f, 0.9f); cv_ellipse(39 + sway, sy + 2, 3.4f, 3.0f, R_WARM, 0.35f, 0.9f);
    for (int y = (int)sy - 1; y <= (int)bot + 3; y++)               /* the shirt: faded, folded, ragged at the hem, torn open */
        for (int x = 18; x <= 45; x++) {
            uint8_t c = cv_sprite[y * TS + x];
            int skin = c >= R_WARM && c < R_WARM + 16;
            if (!skin && !(y > bot && y <= bot + 3 && x > tp[6] && x < tp[4] && h01(x, 9, 215) > 0.45f)) continue;
            if (!back && x > cx - 6 && x < cx + 0.5f - (y - sy) * 0.15f && y > sy + 2 && y < sy + 16) continue;   /* the tear, down to the belly */
            if (back && fabsf(x + 0.5f - cx - 1.0f + (y - sy) * 0.08f) < 1.6f && y > sy + 3 && y < bot - 2) continue;   /* from behind, a rent down the spine */
            if (y > bot - 1 && h01(x, 7, 215) < 0.3f) continue;     /* ragged hem */
            float fold = 0.5f + 0.5f * sinf((x - cx) * 0.9f + y * 0.15f);
            float v = 0.22f + 0.3f * (1.0f - fabsf(x + 0.5f - cx) / 12.0f) + 0.1f * fold;
            cv_px(x, y, sh(R_STONE, v - 0.02f - (((int)(y + x * 0.5f)) % 5 == 0 ? 0.12f : 0.0f)));   /* burial linen, wound round it, gone grey */
        }
    if (back) for (int y = (int)sy + 4; y < (int)bot - 2; y += 2) {  /* the spine's knobs in the rent */
        float x = cx + 1.0f - (y - sy) * 0.08f;
        cv_px((int)x, y, sh(R_BONE, 0.8f)); cv_px((int)x, y + 1, sh(R_BONE, 0.3f)); cv_px((int)x - 1, y, sh(R_WARM, 0.25f));
    }
    else {
        for (int i = 0; i < 2; i++) {                               /* ribs showing in the tear, and the rot inside */
            float y = sy + 6 + i * 3.5f;
            for (int k = 0; k <= 4; k++) cv_px((int)(cx - 5 + k), (int)(y + 0.25f * (k - 2) * (k - 2)), sh(R_BONE, 0.85f - 0.08f * k));
        }
        cv_flat(cx - 2.5f, sy + 13, 1.8f, 1.3f, sh(R_BLOOD, 0.2f));
    }
    cv_capsule(hx, hy + 5, 30.5f + sway, sy + 1, 2.2f, R_WARM, 0.3f, 0.75f);   /* the neck */
    /* arms */
    if (grab) {                                                     /* both up and out at you, fingers hooked */
        cv_capsule(22 + sway, sy + 1, 16 + sway, sy - 1, 2.0f, R_WARM, 0.35f, 0.9f);
        cv_capsule(16 + sway, sy - 1, 11 + sway, sy - 5, 1.8f, R_WARM, 0.35f, 0.9f);
        dead_hand(10 + sway, sy - 6, -2.2f, 0.4f, R_WARM, 1.15f);
        cv_capsule(40 + sway, sy + 1, 46 + sway, sy - 1, 2.0f, R_WARM, 0.35f, 0.9f);
        cv_capsule(46 + sway, sy - 1, 51 + sway, sy - 5, 1.8f, R_WARM, 0.35f, 0.9f);
        dead_hand(52 + sway, sy - 6, -0.9f, 0.4f, R_WARM, 1.15f);
    } else if (pose == 4) {                                         /* hanging slack */
        cv_capsule(22 + sway, sy + 1, 22 + sway, sy + 14, 1.9f, R_WARM, 0.3f, 0.8f);
        dead_hand(22 + sway, sy + 15, 1.6f, 0.2f, R_WARM, 0.9f);
        cv_capsule(40 + sway, sy + 1, 42 + sway, sy + 14, 1.9f, R_WARM, 0.3f, 0.8f);
        dead_hand(42 + sway, sy + 15, 1.5f, 0.2f, R_WARM, 0.9f);
    } else if (back) {                                              /* the reaching arm, out away from you: its shoulder and upper arm */
        cv_capsule(40 + sway, sy + 1, 43 + sway, sy + 7, 2.0f, R_WARM, 0.3f, 0.8f);
    } else {                                                        /* the reaching arm, out towards you */
        cv_capsule(40 + sway, sy + 1, 45 + sway, sy + 3, 2.0f, R_WARM, 0.35f, 0.9f);
        cv_capsule(45 + sway, sy + 3, 49 + sway - w * 0.5f, sy + 0, 1.9f, R_WARM, 0.4f, 0.95f);
        dead_hand(50 + sway - w * 0.5f, sy - 1, 0.9f, 0.3f, R_WARM, 1.1f);
    }
    /* the head (SHAMB_HEAD): sunken sockets with a sulphur light in them, the jaw dropped, hair in clumps */
    if (back) {                                                     /* the back of its head: grey scalp, the hair in clumps, bald where it has rotted */
        for (int y = (int)hy - 7; y <= (int)hy + 7; y++)
            for (int x = (int)hx - 6; x <= (int)hx + 6; x++) {
                float nx = (x + 0.5f - hx) / 5.8f, ny = (y + 0.5f - hy) / 7.0f, d = nx * nx + ny * ny;
                if (d > 1.0f) continue;
                float v = 0.2f + 0.45f * lit(nx, ny, sqrtf(1.0f - d));
                cv_px(x, y, sh(R_WARM, v));
            }
        for (int i = 0; i < 12; i++) {                              /* lank hair in strands from the crown, a rotted bald patch left of it */
            float sx = hx - 5.5f + i * 1.0f, top = hy - 6.5f + 0.12f * (sx - hx) * (sx - hx), len = 7.0f + 5.0f * h01(i, 3, 225);
            if (i >= 6 && i <= 8) { top += 4.0f; len -= 3.0f; }
            for (int j = 0; j < (int)len; j++) cv_px((int)(sx + (sx - hx) * 0.06f * j), (int)(top + j), sh(R_IRON, 0.12f + 0.16f * h01(i, j, 227) + (j == 0 ? 0.1f : 0.0f)));
        }
        cv_px((int)hx - 6, (int)hy + 1, sh(R_WARM, 0.4f)); cv_px((int)hx + 6, (int)hy + 1, sh(R_WARM, 0.3f));   /* the ears' backs */
    } else
        grid_stamp(SHAMB_HEAD, 13, 16, (int)(hx - 6.0f), (int)(hy - 7.0f), R_WARM, R_IRON, (uint8_t)(R_FLAME + (grab ? 15 : 13)), 0);
    for (int i = 0; i < 7; i++) {                                   /* lank hair over the crown and down the sides */
        float a = PI_F * (1.05f + 0.9f * i / 6.0f), x = hx + cosf(a) * 6.0f, y = hy - 1.0f + sinf(a) * 6.5f;
        float drop = (i == 0 || i == 6) ? 6.0f + 3.0f * h01(i, 2, 219) : 1.5f;
        cv_line(x, y, x + (x < hx ? -0.8f : 0.8f), y + drop, sh(R_IRON, 0.18f + 0.12f * h01(i, 1, 219)));
    }
    cv_rot(R_WARM, 0.6f, 211);
    for (int y = 0; y < TS; y++)                                    /* grave-dirt clotted on it, more of it lower down */
        for (int x = 0; x < TS; x++) {
            uint8_t c = cv_sprite[y * TS + x];
            if (!((c > R_WARM && c < R_WARM + 16) || (c > R_STONE && c < R_STONE + 16))) continue;
            float n = fbm(x / 3.0f, y / 3.0f, 16, 213);
            if (n > 0.74f - (float)y * 0.004f) cv_px(x, y, sh(R_WOOD, 0.06f + 0.14f * h01(x, y, 214)));
        }
    if (pose <= 1 && !back)                                         /* roots out of the tear in its chest, from the grave */
        for (int k = 0; k < 3; k++) {
            float x = cx - 3.0f + k * 1.5f, y = sy + 8.0f + k * 2.0f, a = 2.2f + k * 0.5f;
            for (int i = 0; i < 6 + k * 2; i++) { a += (h01(k, i, 217) - 0.5f) * 0.9f; x += cosf(a) * 0.9f; y += sinf(a) * 0.9f; cv_px((int)x, (int)y, sh(R_WOOD, 0.3f + 0.15f * (i & 1))); }
        }
    if (pose == 2 || pose == 3) {                                   /* climbing out of the grave */
        cv_sink(pose == 2 ? 30 : 14, 58);
        soil(32, 17, 58, 221);
    }
    cv_outline(0);
    cv_store(id);
}

/* the shambler's dressing by rank (docs/InitialArtPass.md 6.1): II more of its linen hanging, grave-mould; III frost and
 * an ear torn away; IV an arrow in its shoulder and old blood from the tear and the mouth; V charred, an iron circlet and a
 * grave's shackle on its wrist; gilded gold leaf and gilt teeth */
static void shambler_dress(int id, float w, int grab) {
    float sway = w * 0.8f, hx = 30 + sway, hy = 13 + (grab ? 1.0f : 0.0f), sy = 23, cx = 31.0f + sway * 0.7f;
    for (int r = RK_II; r < NRANK; r++) {
        dress_begin(id);
        if (r != RK_GILDED) {
            kit_rags(24 + sway, 43, 38 + sway, 44, 9, w, R_STONE, 0.1f, 0.35f, 0, 491);
            kit_rags(22 + sway, sy, 26 + sway, sy + 1, 6, w * 0.5f, R_STONE, 0.1f, 0.35f, 0, 493);
        }
        if (r == RK_II) kit_moss(0.3f, 3, 495);
        if (r == RK_III) kit_frost(0.75f, 497);
        if (r >= RK_III && r != RK_GILDED) { cv_px((int)hx + 6, (int)hy - 2, 0); cv_px((int)hx + 6, (int)hy - 1, 0); cv_px((int)hx + 5, (int)hy - 3, 0); }   /* an ear gone */
        if (r >= RK_IV && r != RK_GILDED) { kit_arrow(38 + sway, sy + 2, -0.9f, 8.0f); kit_blood(cx - 3, sy + 12, 10, 2, 499); kit_blood(hx, hy + 6, 7, 2, 501); }
        if (r == RK_V) { kit_char(0.3f, 503); kit_crown(hx, hy - 7, 11, 505); if (!grab) kit_chain(21 + w * 2.5f, sy + 19, 19 + w * 2.0f, sy + 33, 1.5f, R_IRON); }
        if (r == RK_GILDED) {
            kit_gold(R_WARM, 0.5f, 507);
            for (int y = (int)hy + 2; y < (int)hy + 8; y++)                                   /* gilt teeth */
                for (int x = (int)hx - 5; x <= (int)hx + 5; x++) { uint8_t b = base_c(x, y); if (b >= R_WARM + 12 && b < R_WARM + 16) cv_px(x, y, (uint8_t)(R_GOLD + 11)); }
            kit_chain(24 + sway, sy + 2, 38 + sway, sy + 2, 2.5f, R_GOLD);
        }
        dress_store(id, r, R_WARM);
    }
}

/* the water a drowned one rises from, or the foul pool it leaves when it bursts */
static void drowned_pool(float cx, float w, int foul, uint32_t seed) {
    int ramp = foul ? R_MOSS : R_GHOST;
    cv_flat(cx, 59.5f, w, 4.2f, sh(ramp, 0.1f));
    for (int y = 55; y <= 63; y++)
        for (int x = (int)(cx - w); x <= (int)(cx + w); x++) {
            float nx = (x + 0.5f - cx) / w, ny = (y + 0.5f - 59.5f) / 4.2f, d = sqrtf(nx * nx + ny * ny);
            if (d > 1.0f) continue;
            float r = sinf(d * 11.0f + h01((int)seed, 0, 9) * 6.0f);
            if (r > 0.75f) cv_px(x, y, sh(ramp, 0.3f + 0.3f * r * (1.0f - d)));
            else if (ny < -0.3f && h01(x, y, seed) > 0.85f) cv_px(x, y, sh(ramp, 0.5f));   /* the light on the far side */
        }
    for (int i = 0; i < 5; i++) {                                   /* bubbles */
        float x = cx + (h01(i, 1, seed) - 0.5f) * w * 1.4f, y = 58 + 3 * h01(i, 2, seed);
        cv_px((int)x, (int)y, sh(ramp, 0.75f)); cv_px((int)x + 1, (int)y - 1, sh(ramp, 0.5f));
    }
}

/* The Drowned (docs/InitialArtPass.md section 8): the body the water made of a man, long and wrong. Waxen green-white skin
 * (R_GHOST, PF_DROWN), a gaunt, too-tall frame round a belly swollen tight and splitting at a seam; sloping shoulders, ribs,
 * thin arms hanging past the knees with long wrinkled fingers; a long face, the hair plastered flat down it, small cold
 * eyes in deep sockets, the mouth a black slack hole; a bruise round the throat; the Order's shroud sodden and dragged on
 * one side; a rope round the ankle and the stone it was weighted with; weed; it drips. pose 0-1 wading, 2 reaching for
 * you, 3 rising from its puddle, 4 bursting, 5 the foul pool it leaves. */
static const char *const DROWN_HEAD[15] = {
    "....jkkkj....", "..jklmmmlkj..", ".jk6789876kj.", ".kj79aaa97jk.", "jk7789a9877kj", "jk50@080@05kj", "jk743292347kj",
    "kj6687a7866jk", "kj566797665jk", "jk467000764kj", "jk360000063kj", "j.360000063.j", "j..4600064..j", "j..3578753..j",
    "....24642....",
};
static void spr_drowned(int id, float w, int pose) {
    cv_begin(cv_sprite, TS, TS);
    if (pose == 5) { drowned_pool(32, 20, 1, 231); cv_store(id); return; }
    if (pose == 4) {                                               /* burst: skin and shroud flung out round a cloud of gas */
        for (int i = 0; i < 170; i++) {
            float a = h01(i, 1, 233) * TAU_F, r = 4 + 21 * sqrtf(h01(i, 2, 233));
            float x = 32 + cosf(a) * r, y = 36 + sinf(a) * r * 0.8f;
            if ((i & 1) && h01(i, 3, 233) > 0.3f) cv_px((int)x, (int)y, sh(R_MOSS, 0.25f + 0.4f * h01(i, 4, 233)));
        }
        for (int i = 0; i < 10; i++) {                             /* rags of skin and the shroud */
            float a = h01(i, 5, 233) * TAU_F, r = 8 + 15 * h01(i, 6, 233);
            float x = 32 + cosf(a) * r, y = 38 + sinf(a) * r * 0.7f;
            cv_capsule(x, y, x + cosf(a) * 3.0f, y + 2.5f, 1.0f, (i & 1) ? R_GHOST : R_WARM, 0.15f, 0.55f);
        }
        cv_ellipse(32, 46, 7, 6, R_GHOST, 0.1f, 0.4f);              /* what is left, slumping */
        cv_flat(32, 45, 3.5f, 2.5f, sh(R_BLOOD, 0.12f));
        drowned_pool(32, 16, 1, 235);
        cv_outline(0); cv_store(id); return;
    }
    int back = g_back;                                              /* 1.0.0b18: its back (walking), drawn unturned; spr_turn turns it */
    if (back) pose = 0;
    int grab = pose == 2, swell = pose == 6, spew = pose == 7;      /* C4: swelling with water, then spewing it */
    float sw = w * 0.8f;
    cv_flat(32, 62, 15, 2.0f, R_WARM + 1);                          /* the wet where it stands */
    cv_flat(32, 62, 10, 1.2f, sh(R_GHOST, 0.12f));
    for (int s = -1; s <= 1; s += 2) {                              /* legs: thin, knock-kneed, too long */
        float kx = 32 + s * 4.5f - s * w * 1.0f, ky = 52;
        cv_capsule(32 + s * 4.0f, 44, kx, ky, 2.0f, R_GHOST, 0.15f, 0.55f);
        cv_ellipse(kx, ky, 2.2f, 1.8f, R_GHOST, 0.2f, 0.6f);
        cv_capsule(kx, ky, 32 + s * 5.0f + s * w * 1.0f, 61, 1.7f, R_GHOST, 0.12f, 0.5f);
        cv_ellipse(32 + s * 5.5f + s * w, 61.5f, 3.0f, 1.2f, R_GHOST, 0.15f, 0.5f);
    }
    float rx = 37.0f + w, ry = 60.0f;                               /* a rope round the right ankle, to the stone it sank with */
    cv_line(rx - 1, ry - 1, rx + 2, ry - 1, sh(R_WOOD, 0.45f)); cv_line(rx - 1, ry, rx + 2, ry, sh(R_WOOD, 0.3f));
    cv_line(rx + 2, ry, 47, 61, sh(R_WOOD, 0.35f));
    cv_ellipse(49, 60.5f, 3.2f, 2.4f, R_STONE, 0.15f, 0.55f);
    if (swell) cv_ellipse(32 + sw * 0.5f, 36, 12.0f, 12.0f, R_GHOST, 0.2f, 0.82f);   /* swelling: the belly drum-tight, shining */
    else cv_ellipse(32 + sw * 0.5f, 37, spew ? 8.0f : 9.5f, spew ? 9.0f : 10.0f, R_GHOST, 0.15f, 0.7f);   /* the belly, swollen tight */
    cv_ellipse(32 + sw * 0.6f, 26, swell ? 9.0f : 7.5f, swell ? 6.0f : 5.0f, R_GHOST, 0.15f, 0.6f);    /* a narrow chest over it, ribs showing */
    for (int r = 0; r < 3; r++) for (int s = -1; s <= 1; s += 2) cv_line(32 + sw * 0.6f + s * 1.5f, 24 + r * 2.2f, 32 + sw * 0.6f + s * 6.5f, 25.5f + r * 2.2f, sh(R_GHOST, 0.25f));
    if (back) {                                                     /* from behind: a narrow back, the belly bulging past it at the flanks */
        cv_ellipse(32 + sw * 0.5f, 33, 6.5f, 12.0f, R_GHOST, 0.1f, 0.5f);
        for (int y = 21; y < 45; y += 2) { cv_px((int)(32 + sw * 0.55f), y, sh(R_GHOST, 0.7f)); cv_px((int)(32 + sw * 0.55f), y + 1, sh(R_GHOST, 0.15f)); }   /* the spine */
        for (int s = -1; s <= 1; s += 2) {                          /* the shoulder blades, sharp under the skin */
            cv_line(32 + sw * 0.6f + s * 2.0f, 22, 32 + sw * 0.6f + s * 6.0f, 23, sh(R_GHOST, 0.6f));
            cv_line(32 + sw * 0.6f + s * 6.0f, 23, 32 + sw * 0.6f + s * 3.0f, 29, sh(R_GHOST, 0.3f));
        }
        for (int k = 0; k < 3; k++) cv_line(27 + k * 4 + sw * 0.5f, 38 + k, 28 + k * 4 + sw * 0.5f, 42 + k, sh(R_PURPLE, 0.18f));   /* the lividity, where the blood pooled */
    } else {
        for (int k = 0; k < 4; k++) {                               /* veins on the belly, and the seam where it is splitting */
            float x = 26 + 12 * h01(k, 1, 237), y = 31 + 10 * h01(k, 2, 237);
            cv_line(x, y, x + 3 * (h01(k, 3, 237) - 0.5f), y + 4, sh(R_PURPLE, 0.2f));
        }
        cv_line(31 + sw * 0.5f, 32, 34 + sw * 0.5f, 44, sh(R_GHOST, 0.05f)); cv_line(32 + sw * 0.5f, 33, 35 + sw * 0.5f, 43, sh(R_MOSS, 0.35f));
    }
    for (int i = 0; i < 8; i++)                                     /* barnacles, a shell or two */
        cv_px((int)(24 + 16 * h01(i, 3, 239) + sw), (int)(29 + 16 * h01(i, 4, 239)), sh(R_BONE, 0.5f + 0.35f * h01(i, 5, 239)));
    /* the Order's shroud, sodden, hung from the left shoulder across the belly and dragging on that side */
    { float sv[10] = { 22 + sw, 21, 28 + sw, 22, 27 + sw, 44, 24 + sw, 55, 19 + sw, 50 };
      cv_poly(sv, 5, R_WARM, 0.42f, 0.18f, 21, 34);
      for (int y = 46; y < 56; y++) if (h01(y, 1, 241) > 0.45f) cv_px((int)(19 + h01(y, 2, 241) * 8 + sw), y, C_CLEAR);   /* torn */
      for (int k = 0; k < 3; k++) cv_line(23 + k * 2 + sw, 23, 21 + k * 2 + sw, 50, sh(R_WARM, 0.16f)); }
    float sy = 21;
    for (int s = -1; s <= 1; s += 2) {                              /* arms: thin and too long, hanging past the knees, or up to take you */
        float sx = 32 + s * 8.0f + sw * 0.6f;
        if (grab) {
            cv_capsule(sx, sy, 32 + s * 13.0f, sy - 4, 1.8f, R_GHOST, 0.2f, 0.65f);
            cv_capsule(32 + s * 13.0f, sy - 4, 32 + s * 12.0f, sy - 14, 1.5f, R_GHOST, 0.2f, 0.6f);
            dead_hand(32 + s * 12.0f, sy - 16, s < 0 ? -2.0f : -1.1f, 0.5f, R_GHOST, 1.15f);
        } else if (swell) {                                         /* clutching the belly as it fills */
            cv_capsule(sx, sy, 32 + s * 14.0f, sy + 10, 1.8f, R_GHOST, 0.15f, 0.6f);
            cv_capsule(32 + s * 14.0f, sy + 10, 32 + s * 9.0f, sy + 17, 1.5f, R_GHOST, 0.15f, 0.55f);
            dead_hand(32 + s * 8.0f, sy + 18, s < 0 ? 0.3f : 2.8f, 0.3f, R_GHOST, 1.0f);
        } else if (spew) {                                          /* flung back and out, the whole of it behind the gout */
            cv_capsule(sx, sy, 32 + s * 15.0f, sy + 3, 1.8f, R_GHOST, 0.15f, 0.6f);
            cv_capsule(32 + s * 15.0f, sy + 3, 32 + s * 19.0f, sy + 12, 1.5f, R_GHOST, 0.15f, 0.55f);
            dead_hand(32 + s * 19.5f, sy + 14, s < 0 ? 2.2f : 0.9f, 0.6f, R_GHOST, 1.1f);
        } else {
            float ex = 32 + s * 11.0f + sw * 0.4f, ey = sy + 14, hx = 32 + s * 11.5f - s * w * 0.5f, hy = sy + 28;
            cv_capsule(sx, sy, ex, ey, 1.8f, R_GHOST, 0.15f, 0.6f);
            cv_capsule(ex, ey, hx, hy, 1.5f, R_GHOST, 0.15f, 0.55f);
            dead_hand(hx, hy + 2, 1.6f, 0.25f, R_GHOST, 1.15f);
            cv_px((int)hx, (int)hy + 7, sh(R_GHOST, 0.85f)); cv_px((int)hx, (int)hy + 9, sh(R_GHOST, 0.6f));   /* dripping */
        }
        cv_ellipse(sx, sy, 2.4f, 1.8f, R_GHOST, 0.2f, 0.7f);        /* sloping shoulders */
    }
    float hx0 = 26 + sw, hy0 = 4;                                   /* the head, hung forward on its neck */
    cv_capsule(32 + sw, 15, 32 + sw * 0.8f, 21, 1.8f, R_GHOST, 0.15f, 0.55f);
    for (int x = -2; x <= 2; x++) cv_px((int)(32 + sw * 0.8f) + x, 18, sh(R_PURPLE, 0.18f));   /* the bruise where something held it */
    for (int i = 0; i < 6; i++) {                                   /* weed hung over the shoulders */
        float x = 24 + i * 3.2f + sw, len = 5 + 7 * h01(i, 1, 243);
        for (int y = 0; y < (int)len; y++) cv_px((int)(x + sinf(y * 0.6f + i + w) * 0.7f), 20 + y, sh(R_MOSS, 0.12f + 0.25f * (1.0f - y / len)));
    }
    if (back) {                                                     /* the back of its long head, the hair plastered down it and hanging past the neck */
        float cx = hx0 + 6.5f, cy = hy0 + 7.0f;
        for (int y = (int)hy0; y <= (int)hy0 + 17; y++)
            for (int x = (int)hx0; x <= (int)hx0 + 13; x++) {
                float nx = (x + 0.5f - cx) / 5.8f, ny = (y + 0.5f - cy) / 7.6f, d = nx * nx + ny * ny;
                int hang = y > cy + 4 && fabsf(nx) < 0.85f - (y - cy - 4) * 0.05f && y < hy0 + 18;
                if (d > 1.0f && !hang) continue;
                float v = 0.1f + 0.34f * (d <= 1.0f ? lit(nx, ny, sqrtf(fmaxf(0.0f, 1.0f - d))) : 0.15f) + (((x * 2 + (y > cy ? 1 : 0)) % 3) == 0 ? 0.12f : -0.03f);
                if (fabsf(x + 0.5f - cx) < 0.6f && y < cy) v = 0.05f;   /* the parting */
                if (d <= 1.0f && h01(x, y, 247) < 0.06f && nx < 0.2f) { cv_px(x, y, sh(R_GHOST, 0.62f)); continue; }   /* the wet's sheen */
                cv_px(x, y, sh(R_IRON, v));                         /* black hair, wet, in ropes */
            }
        cv_px((int)hx0 + 4, (int)hy0 + 18, sh(R_GHOST, 0.75f)); cv_px((int)hx0 + 8, (int)hy0 + 19, sh(R_GHOST, 0.55f));   /* dripping from it */
    } else {
        grid_stamp(DROWN_HEAD, 13, 15, (int)hx0, (int)hy0, R_GHOST, R_IRON, (uint8_t)(R_GLOW + 14), 0);
        cv_px((int)hx0 + 6, (int)hy0 + 15, sh(R_GHOST, 0.75f)); cv_px((int)hx0 + 6, (int)hy0 + 17, sh(R_GHOST, 0.5f));   /* water from its mouth */
        if (swell) {                                                /* the throat and cheeks filling, the lips pressed shut */
            cv_ellipse(32 + sw * 0.8f, 18.5f, 5.0f, 3.2f, R_GHOST, 0.25f, 0.85f);
            cv_ellipse(hx0 + 1.5f, hy0 + 11.0f, 2.2f, 2.0f, R_GHOST, 0.3f, 0.8f); cv_ellipse(hx0 + 11.5f, hy0 + 11.0f, 2.2f, 2.0f, R_GHOST, 0.3f, 0.8f);
            for (int x = -2; x <= 2; x++) cv_px((int)(hx0 + 6.5f) + x, (int)hy0 + 13, sh(R_IRON, 0.08f));
            for (int k = 0; k < 4; k++) cv_line(25 + k * 4 + sw * 0.5f, 30 + (k & 1) * 3, 27 + k * 4 + sw * 0.5f, 40 - (k & 1) * 2, sh(R_PURPLE, 0.32f));   /* the veins standing out */
        }
        if (spew) {                                                 /* the jaw dropped wide, and the water coming */
            cv_flat(hx0 + 6.5f, hy0 + 12.5f, 3.0f, 2.8f, sh(R_IRON, 0.04f));
            for (int i = 0; i < 160; i++) {                         /* a gout, widening as it comes at you */
                float u = h01(i, 1, 249), y = hy0 + 13.0f + u * 30.0f, half = 1.5f + u * 9.0f;
                float x = hx0 + 6.5f + (h01(i, 2, 249) - 0.5f) * 2.0f * half;
                cv_px((int)x, (int)y, h01(i, 3, 249) > 0.7f ? sh(R_GHOST, 0.85f) : sh(R_MOSS, 0.3f + 0.4f * h01(i, 4, 249)));
            }
        }
    }
    cv_px((int)hx0, (int)hy0 + 15, sh(R_GHOST, 0.6f)); cv_px((int)hx0 + 12, (int)hy0 + 16, sh(R_GHOST, 0.6f));      /* and its hair */
    cv_rot(R_GHOST, 0.2f, 236);
    if (pose == 3) { cv_sink(18, 58); drowned_pool(32, 22, 0, 245); }
    cv_outline(0);
    cv_store(id);
}

/* C4 (docs/PLAN-combat.md, section 8.2): the shield-bearer's charge. The stamp: a foot planted, the shield brought up to the
 * jaw, the blade drawn back; the rush: behind the shield, the skull's eyes over its rim, a long stride under it */
static void spr_shield_charge(int id, int rush) {
    float w = rush ? 1.5f : 0.8f;
    skel_draw(w, 0.0f, 13, 0);
    if (rush) {
        cv_capsule(47, 27, 54, 33, 1.0f, R_IRON, 0.5f, 1.0f);       /* the blade's point, low and forward */
    } else {
        cv_capsule(46, 30, 55, 44, 1.0f, R_IRON, 0.5f, 1.0f);       /* the blade drawn back to the hip */
        cv_capsule(44, 28, 48, 33, 0.9f, R_WOOD, 0.3f, 0.6f);
    }
    float cx = rush ? 31.0f : 25.0f, cy = rush ? 28.0f : 25.0f, rx = rush ? 15.5f : 13.5f, ry = rush ? 16.5f : 15.0f;
    for (int y = (int)(cy - ry); y <= (int)(cy + ry); y++)        /* the shield, as the walking one's, nearer */
        for (int x = (int)(cx - rx); x <= (int)(cx + rx); x++) {
            float nx = (x + 0.5f - cx) / rx, ny = (y + 0.5f - cy) / ry, d = nx * nx + ny * ny;
            if (d > 1.0f) continue;
            float v = 0.32f + 0.45f * lit(nx * 0.4f, ny * 0.4f, sqrtf(1.0f - d * 0.6f));
            uint8_t c;
            if (d > 0.8f) c = sh(R_IRON, v + 0.25f);
            else if (d < 0.05f) c = sh(R_IRON, v + 0.45f);
            else {
                int plank = (int)((x - (cx - rx)) / 6.0f);
                c = sh(R_WOOD, v + 0.08f * h01(plank, 3, 51) - (((int)(x - (cx - rx)) % 7) == 0 ? 0.12f : 0.0f) - g_decay * 0.15f * h01(x / 3, y / 3, 52));
                if (fabsf(y + 0.5f - cy) < 1.2f) c = sh(R_IRON, v + 0.1f);
            }
            cv_px(x, y, c);
        }
    cv_px((int)(cx - 4), (int)(cy - 6), sh(R_IRON, 0.9f)); cv_px((int)(cx + 5), (int)(cy + 6), sh(R_IRON, 0.9f));
    cv_outline(0);
    cv_store(id);
    skel_dress(id, w, rush ? 52.0f : 54.0f, rush ? 31.0f : 42.0f);
}
/* C4: a gout of the Drowned's spew, murky and bright where it catches the light; and the dust a stamp raises */
static void spr_spew(int id, int f) {
    cv_begin(cv_sprite, TS, TS);
    for (int i = 0; i < 90; i++) {
        float a = h01(i, 1, 251 + f) * TAU_F, r = 11.0f * sqrtf(h01(i, 2, 251 + f));
        float x = 32 + cosf(a) * r * 1.2f, y = 32 + sinf(a) * r * 0.8f;
        int big = h01(i, 3, 251 + f) > 0.6f;
        uint8_t c = h01(i, 4, 251 + f) > 0.78f ? sh(R_GHOST, 0.9f) : sh(R_MOSS, 0.25f + 0.45f * h01(i, 5, 251 + f));
        cv_px((int)x, (int)y, c);
        if (big) { cv_px((int)x + 1, (int)y, c); cv_px((int)x, (int)y + 1, sh(R_MOSS, 0.2f)); }
    }
    cv_store(id);
}
static void spr_dust(int id, int f) {
    cv_begin(cv_sprite, TS, TS);
    for (int i = 0; i < 420; i++) {
        float a = h01(i, 1, 261 + f) * PI_F, r = (f ? 22.0f : 14.0f) * sqrtf(h01(i, 2, 261 + f));
        float x = 32 + cosf(a) * r * 1.4f, y = 62 - sinf(a) * r * (f ? 0.75f : 0.6f);
        if (h01(i, 3, 261 + f) < 0.1f + 0.3f * (float)f) continue;   /* thinning as it spreads */
        cv_px((int)x, (int)y, sh(h01(i, 4, 261 + f) > 0.5f ? R_STONE : R_WARM, 0.3f + 0.35f * h01(i, 5, 261 + f)));
    }
    cv_store(id);
}

/* the Drowned's dressing by rank: II weed and mould, the shroud more torn; III frost in its hair and on its shoulders, the
 * seam split wider; IV bleeding from its seams and its mouth; V black, a thing of the deep, a chain from its ankle's
 * stone to its wrist and a crown of rusted iron; gilded the Order's gold, gone green in the water */
static void drowned_dress(int id, int grab) {
    for (int r = RK_II; r < NRANK; r++) {
        dress_begin(id);
        if (r != RK_GILDED) kit_rags(22, 48, 30, 50, 8, 0.0f, R_WARM, 0.08f, 0.25f, 0, 471);   /* more of the shroud, torn */
        if (r == RK_II) kit_moss(0.32f, 2, 473);
        if (r == RK_III) kit_frost(0.8f, 475);
        if (r >= RK_III && r != RK_GILDED) { cv_line(31, 32, 35, 45, 0); cv_line(32, 33, 36, 44, sh(R_MOSS, 0.4f)); }   /* the seam split */
        if (r >= RK_IV && r != RK_GILDED) { kit_blood(33, 34, 12, 2, 477); kit_blood(32, 16, 6, 1, 479); }
        if (r == RK_V) { kit_char(0.3f, 481); kit_crown(32, 5, 11, 483); if (!grab) kit_chain(48, 59, 44, 50, -3.0f, R_IRON); }
        if (r == RK_GILDED) {
            kit_gold(R_GHOST, 0.5f, 485);
            for (int y = 0; y < TS; y++) for (int x = 0; x < TS; x++) if (cv_sprite[y * TS + x] >= R_GOLD && cv_sprite[y * TS + x] < R_GOLD + NGOLD && h01(x, y, 487) < 0.3f) cv_px(x, y, sh(R_MOSS, 0.55f));   /* verdigris */
            kit_chain(26, 22, 38, 22, 2.0f, R_GOLD);
        }
        dress_store(id, r, R_GHOST);
    }
}


/* The ghoul (docs/InitialArtPass.md section 8): starved, low, on all fours, skin tight over a beaded spine and ribs, long
 * thin arms with long claws; a long-muzzled face (GHOUL_HEAD: close-set eyes deep in their sockets, slit nostrils at the
 * muzzle's tip, lips peeled back from fangs, the jaw hanging open), long ears swept up and torn. Skin R_WARM (PF_FLESH).
 * pose: 0-1 loping, 2 leaping, 3 feeding on bones, 4 dead. */
static const char *const GHOUL_HEAD[15] = {
    "....4678764....", "...59abcba95...", "..48abdddba74..", ".36a900c009a63.", ".359@0aca0@953.", "..48900a00984..",
    "..37abcdcba73..", "...59bcecb95...", "...4a0dcd0a4...", "..2u2u0u0u2u2..", "..2u1000001u2..", "..3s0000000s3..",
    "...3s0s0s0s3...", "....3589853....", ".....36663.....",
};
