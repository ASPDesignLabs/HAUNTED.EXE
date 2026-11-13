/* A part of crawl_art.c (docs/PLAN-tidy.md): it is #included there in its place and never compiled on its own.
 * Side views: each creature walking in profile; the glints found. */

/* ------------------------------------------------------------------ */
/* Side views: each creature walking in profile, facing right. The     */
/* bestiary mirrors them for left.                                     */
/* ------------------------------------------------------------------ */

/* a curved line through three points, for ribs seen from the side */
static void cv_curve(float x0, float y0, float cx, float cy, float x1, float y1, uint8_t c) {
    for (int i = 0; i <= 16; i++) {
        float t = i / 16.0f, u = 1.0f - t;
        cv_px((int)(u * u * x0 + 2 * u * t * cx + t * t * x1), (int)(u * u * y0 + 2 * u * t * cy + t * t * y1), c);
    }
}


/* a skeleton walking right. w: the stride, +1 or -1; arm: 0 swinging, 1 a sword up and a shield on */
static void skel_side(float w, int arm) {
    float bob = fabsf(w) * 0.6f;
    w *= 1.5f;                                                     /* a longer stride than the front view needs */
    cv_flat(32, 62, 14, 1.8f, R_WARM + 1);
    /* the far leg and arm first, darker */
    cv_capsule(31, 41, 31 - 4 * w + 1, 51, 1.4f, R_BONE, 0.12f, 0.55f); cv_capsule(31 - 4 * w + 1, 51, 31 - 5 * w, 60, 1.2f, R_BONE, 0.12f, 0.5f);
    cv_capsule(31 - 5 * w, 61, 31 - 5 * w + 5, 61.5f, 1.0f, R_BONE, 0.12f, 0.5f);
    if (!arm) { cv_capsule(31, 21 + bob, 31 + 4 * w, 29, 1.1f, R_BONE, 0.12f, 0.5f); cv_capsule(31 + 4 * w, 29, 32 + 6 * w, 36, 1.0f, R_BONE, 0.12f, 0.5f); }
    /* spine, ribs, sternum, pelvis */
    cv_capsule(31, 17 + bob, 29.5f, 27 + bob, 1.3f, R_BONE, 0.3f, 0.9f); cv_capsule(29.5f, 27 + bob, 31, 38, 1.3f, R_BONE, 0.3f, 0.9f);
    for (int i = 0; i < 5; i++) {
        float y0 = 20.5f + i * 3.3f + bob;
        cv_curve(30, y0, 37.5f - i * 0.3f, y0 - 1.0f, 36.5f - i * 0.6f, y0 + 4.0f, sh(R_BONE, 0.85f - i * 0.06f));
        cv_curve(30, y0 + 1, 37.0f - i * 0.3f, y0, 36.0f - i * 0.6f, y0 + 4.8f, sh(R_BONE, 0.45f));
    }
    cv_line(37.5f, 21 + bob, 36.0f, 33 + bob, sh(R_BONE, 0.95f));
    cv_ellipse(31, 40, 4.2f, 3.3f, R_BONE, 0.3f, 0.9f); cv_flat(32, 40.5f, 1.4f, 1.2f, 0);
    /* the near leg, striding */
    cv_bone(31, 41, 31 + 4 * w, 51, 1.5f, 0.3f, 0.95f); cv_bone(31 + 4 * w, 51, 31 + 5 * w - 1, 60, 1.3f, 0.3f, 0.9f);
    cv_capsule(31 + 5 * w - 1, 61, 31 + 5 * w + 4, 61.5f, 1.1f, R_BONE, 0.3f, 0.9f);
    if (!arm) { cv_bone(32, 21 + bob, 32 - 4 * w, 29, 1.2f, 0.3f, 0.95f); cv_capsule(32 - 4 * w, 29, 31 - 6 * w, 36, 1.1f, R_BONE, 0.3f, 0.9f); }
    else {                                                         /* sword arm up and forward */
        cv_bone(32, 21 + bob, 36, 28, 1.2f, 0.3f, 0.95f); cv_capsule(36, 28, 39, 22, 1.1f, R_BONE, 0.3f, 0.9f);
        cv_capsule(39, 22, 47, 5, 1.0f, R_IRON, 0.5f, 1.0f);       /* the blade */
        cv_capsule(36, 23, 42, 21, 0.9f, R_WOOD, 0.3f, 0.6f);      /* the guard */
    }
    cv_capsule(31.5f, 14 + bob, 31, 17 + bob, 1.1f, R_BONE, 0.3f, 0.9f);
    skull_stamp(33, 9.5f + bob, 1, 0, g_decay > 0.55f ? (uint8_t)(R_GLOW + 13) : (uint8_t)(R_FLAME + 10));
    if (arm) {                                                     /* the shield, held out in front, seen at a slant */
        for (int y = 17; y < 48; y++)
            for (int x = 36; x < 46; x++) {
                float nx = (x + 0.5f - 41.0f) / 4.5f, ny = (y + 0.5f - 32.5f) / 15.0f, d = nx * nx + ny * ny;
                if (d > 1.0f) continue;
                uint8_t c = d > 0.72f ? sh(R_IRON, 0.4f + 0.4f * -ny) : sh(R_WOOD, 0.2f + 0.3f * lit(nx, ny, sqrtf(1 - d)) - (((y - 17) % 6) == 0 ? 0.08f : 0.0f));
                if (fabsf(y + 0.5f - 32.5f) < 1.0f && d <= 0.72f) c = sh(R_IRON, 0.5f);
                cv_px(x, y, c);
            }
    }
    cv_bone_decay(901);
    cv_outline(0);
}
static void spr_skel_side(int id, float w, int arm) { cv_begin(cv_sprite, TS, TS); skel_side(w, arm); cv_store(id); }

/* a bat's wing from the shoulder: a membrane between three long fingers, scalloped at the edge */
static void bat_wing(float sx, float sy, float wy, float span, float lo, float hi) {
    float tips[3][2] = { { sx - 6 * span, sy + 16 * wy * span }, { sx - 14 * span, sy + 15 * wy * span }, { sx - 17 * span, sy + 6 * wy * span } };
    float elbow_x = sx - 4 * span, elbow_y = sy + 7 * wy * span;
    for (int f = 0; f < 3; f++) {
        float nx = f < 2 ? tips[f + 1][0] : sx - 9 * span, ny = f < 2 ? tips[f + 1][1] : sy + 1 * wy;
        float mx = (tips[f][0] + nx) * 0.5f + 2.0f * span, my = (tips[f][1] + ny) * 0.5f - 2.0f * wy * span;   /* the scallop between */
        float tri[8] = { elbow_x, elbow_y, tips[f][0], tips[f][1], mx, my, nx, ny };
        cv_poly(tri, 4, R_PURPLE, hi, lo, fminf(elbow_y, fminf(tips[f][1], ny)), 18);
    }
    cv_line(sx, sy, elbow_x, elbow_y, sh(R_IRON, 0.45f));
    for (int f = 0; f < 3; f++) cv_line(elbow_x, elbow_y, tips[f][0], tips[f][1], sh(R_IRON, 0.38f));
    cv_px((int)elbow_x, (int)elbow_y, sh(R_IRON, 0.65f)); cv_px((int)(elbow_x + span), (int)(elbow_y - 2.0f * wy), sh(R_IRON, 0.7f));   /* the wrist, its thumb */
}

/* a bat from the side, wings up or down: its ear back and its snout ahead, a fang showing, a coal of an eye */
static void spr_bat_side(int id, int up) {
    cv_begin(cv_sprite, TS, TS);
    float wy = up ? -1.0f : 1.0f;
    bat_wing(31, 28, wy, 1.25f, 0.03f, 0.15f);                     /* the far wing */
    cv_ellipse(31, 30, 6.0f, 3.6f, R_IRON, 0.1f, 0.38f);          /* the body, level in flight */
    cv_capsule(25, 31, 21, 34, 0.8f, R_IRON, 0.05f, 0.3f);         /* feet trailing, clawed */
    cv_px(20, 35, sh(R_IRON, 0.6f)); cv_px(21, 35, sh(R_IRON, 0.5f));
    grid_stamp(BAT_SIDE, 9, 11, 35, 22, R_IRON, R_SKIN, (uint8_t)(R_FLAME + 8), 0);
    bat_wing(34, 29, wy, 1.5f, 0.06f, 0.3f);                       /* the near one */
    cv_outline(0);
    cv_store(id);
}

/* the web spider from the side: the pitted abdomen behind with its mark on top, the head in front with three of its
 * eyes showing, a fang with its drop, banded legs either side bristling at the knee */
static void spr_spider_side(int id, int frame) {
    cv_begin(cv_sprite, TS, TS);
    cv_flat(32, 62, 24, 1.8f, R_WARM + 1);
    for (int l = 0; l < 4; l++) {                                   /* the far legs */
        float k = ((l + frame) & 1) ? 2.5f : -2.5f, ax = 34 + l * 1.5f, fx = 14 + l * 11.0f + k, kx = (ax + fx) * 0.5f + (l < 2 ? -3 : 3);
        chitin_leg(ax, 44, kx, 33 + l * 0.6f, fx, 61, 1.1f, R_IRON, 0.02f, 0.2f, -1, 471 + (uint32_t)l);
    }
    spider_abdomen(23, 42, 10.0f, 8.0f, R_IRON, 473);               /* the abdomen, its mark on top */
    for (int q = -2; q <= 2; q++) cv_px(22 + q, 34 + (q == 0), (uint8_t)(R_BLOOD + 8 + (q == 0) * 2));
    cv_px(22, 35, (uint8_t)(R_BLOOD + 7));
    cv_ellipse(37, 45, 6.5f, 4.6f, R_IRON, 0.06f, 0.42f);          /* the head */
    for (int l = 0; l < 4; l++) {                                   /* the near legs */
        float k = ((l + frame + 1) & 1) ? 2.5f : -2.5f, ax = 35 + l * 1.5f, fx = 12 + l * 12.0f + k, kx = (ax + fx) * 0.5f + (l < 2 ? -4 : 4);
        chitin_leg(ax, 46, kx, 31 + l * 0.8f, fx, 61.5f, 1.3f, R_IRON, 0.06f, 0.38f, -1, 477 + (uint32_t)l);
    }
    cv_ellipse(41.5f, 44.5f, 3.2f, 2.8f, R_IRON, 0.1f, 0.42f);     /* its face, before the legs */
    cv_px(41, 42, (uint8_t)(R_FLAME + 9)); cv_px(40, 43, (uint8_t)(R_FLAME + 7));   /* three of its eyes */
    cv_eye(43, 43, (uint8_t)(R_FLAME + 13)); cv_px(43, 44, (uint8_t)(R_FLAME + 10));
    cv_ellipse(42.5f, 47.5f, 1.6f, 1.8f, R_IRON, 0.12f, 0.34f);    /* the jaw, and its fang */
    for (int k = 0; k <= 4; k++) cv_px(43 - (k > 2), 48 + k, sh(R_BONE, 0.45f + 0.1f * k));
    cv_px(42, 53, (uint8_t)(R_MOSS + 14)); cv_px(42, 54, (uint8_t)(R_MOSS + 10));
    cv_outline(0);
    cv_store(id);
}

/* the spectre from the side: leaning on, its hem streaming back, a hand held out */
static void spr_ghost_side(int id, float ph) {             /* the spectre turned: the long face forward, its shroud streaming behind */
    cv_begin(cv_sprite, TS, TS);
    for (int y = 2; y < TS; y++)
        for (int x = 0; x < TS; x++) {
            float yy = y + 0.5f, xx = x + 0.5f, front, back;
            if (yy < 15.0f) {                                         /* the hood's crown, round, bulging back */
                float k = (yy - 15.0f) / 12.5f;
                if (fabsf(k) >= 1.0f) continue;
                float r = 10.5f * sqrtf(1.0f - k * k);
                front = 35.0f + r * 0.95f; back = 35.0f - r * 1.15f;
            } else if (yy < 28.0f) {                                  /* and falling to the shoulders */
                float k = (yy - 15.0f) / 13.0f;
                front = 45.0f - 2.0f * k; back = 22.9f + 1.6f * k;
            } else {                                                  /* the shroud streaming back and down */
                float t = (yy - 28.0f) / 35.0f;
                front = 43.0f - 13.0f * t; back = 24.5f - 12.0f * t + sinf(yy * 0.18f + ph) * 2.5f * t;
            }
            if (xx > front || xx < back) continue;
            if (yy > 46.0f && h01((int)(xx * 0.7f), (int)(yy / 3.0f), 981 + (int)(ph * 3.0f)) * 0.7f + h01(x, y, 983) * 0.3f < (yy - 46.0f) / 20.0f) continue;
            float u = (xx - back) / (front - back + 0.01f), v = 0.09f + 0.2f * sinf(u * PI_F) - (yy > 28.0f ? (yy - 28.0f) * 0.002f : 0.0f);
            if (fmodf(yy * 0.3f - xx * 0.25f + sinf(xx * 0.1f + ph) + 20.0f, 2.4f) < 0.4f) v -= 0.05f;   /* folds, streaming */
            if (u > 0.9f || u < 0.06f) v += 0.25f;                 /* the cold rim */
            float ox = (xx - 40.0f) / 4.6f, oyy = (yy - 17.0f) / 11.0f;
            if (ox * ox + oyy * oyy <= 1.0f) v = 0.02f;             /* the hood's mouth */
            cv_px(x, y, sh(R_GHOST, v));
        }
    grid_stamp(SPECTRE_SIDE, 9, 21, 36, 7, R_GHOST, R_GHOST, C_WHITE, 0);
    for (int x = 35; x <= 45; x++) {                                /* the brim over the brow */
        float ox = (x + 0.5f - 40.0f) / 5.2f;
        if (fabsf(ox) >= 1.0f) continue;
        int ye = (int)(17.0f - 11.0f * sqrtf(1.0f - ox * ox));
        cv_px(x, ye, sh(R_GHOST, 0.42f)); cv_px(x, ye + 1, sh(R_GHOST, 0.3f));
    }
    float hy = 29.0f + 1.5f * sinf(ph);
    cv_capsule(34, 26, 47, hy - 1.0f, 3.2f, R_GHOST, 0.12f, 0.48f);   /* the sleeve */
    cv_ellipse(47, hy - 1.0f, 3.0f, 2.6f, R_GHOST, 0.15f, 0.5f);
    mist_hand(50, hy, 1, 0.35f, ph, 991);
    cv_store(id);
}

/* the wall skull turned to show its side: a great skull in profile, a coal in its eye */
static void spr_wskull_side(void) {                         /* the master profile three times over, its eye alight */
    cv_begin(cv_sprite, TS, TS);
    grid_scale3(SKULL_SIDE, SKULL_W, SKULL_H, 0);
    int x0 = 32 - g_bigw / 2, y0 = 6;
    for (int y = 0; y < g_bigh; y++)
        for (int x = 0; x < g_bigw; x++) {
            uint8_t c = g_big[y * 64 + x];
            if (c == C_CLEAR) continue;
            if (c > R_BONE && c < R_BONE + 16) { float r = h01(x, y, 71); c = (uint8_t)(R_BONE + clampi(c - R_BONE + (r < 0.12f ? -1 : r > 0.93f ? 1 : 0), 1, 15)); }
            cv_px(x0 + x, y0 + y, c);
        }
    for (int y = 0; y < SKULL_H; y++)
        for (int x = 0; x < SKULL_W; x++)
            if (SKULL_SIDE[y][x] == '@') { cv_flat(x0 + x * 3 + 1.5f, y0 + y * 3 + 1.5f, 2.6f, 2.8f, (uint8_t)(R_FLAME + 8)); cv_flat(x0 + x * 3 + 1.5f, y0 + y * 3 + 1.5f, 1.2f, 1.3f, (uint8_t)(R_FLAME + 13)); }
    cv_outline(0);
    cv_store(SP_WSKULL_S);
}

/* the shambler from the side: bent forward, arms out ahead of it, one foot dragging */
static void spr_shambler_side(int id, float w) {
    cv_begin(cv_sprite, TS, TS);
    cv_flat(32, 62, 14, 1.8f, R_WARM + 1);
    cv_capsule(31, 42, 31 - 3 * w, 52, 2.6f, R_STONE, 0.05f, 0.28f);  /* the far leg, bound in linen */
    cv_capsule(31 - 3 * w, 52, 30 - 6 * w, 60.5f, 1.8f, R_WARM, 0.15f, 0.5f);
    cv_capsule(30 - 6 * w, 61.5f, 34 - 6 * w, 61.5f, 1.4f, R_WARM, 0.15f, 0.5f);
    cv_capsule(35, 22, 48, 21, 1.8f, R_WARM, 0.15f, 0.55f);        /* the far arm, out ahead */
    dead_hand(50, 22, 0.5f, 0.3f, R_WARM, 0.9f);
    float tp[8] = { 31, 22, 38, 23, 35, 43, 27, 43 };              /* the body, bent at the hips */
    cv_poly(tp, 4, R_WARM, 0.85f, 0.4f, 22, 21);
    for (int y = 21; y <= 45; y++)                                  /* the shirt, ragged */
        for (int x = 24; x <= 40; x++) {
            uint8_t c = cv_sprite[y * TS + x];
            if (c < R_WARM || c >= R_WARM + 16 || (y > 41 && h01(x, 7, 961) < 0.35f)) continue;
            cv_px(x, y, sh(R_STONE, 0.2f + 0.25f * (x - 24) / 16.0f - (((int)(y + x * 0.5f)) % 5 == 0 ? 0.12f : 0.0f)));   /* burial linen, wound */
        }
    cv_capsule(31, 42, 31 + 3 * w, 52, 2.8f, R_STONE, 0.1f, 0.4f);    /* the near leg, bound */
    cv_capsule(31 + 3 * w, 52, 32 + 5 * w, 60.5f, 2.0f, R_WARM, 0.3f, 0.8f);
    cv_capsule(32 + 5 * w, 61.5f, 37 + 5 * w, 61.5f, 1.5f, R_WARM, 0.3f, 0.8f);
    cv_capsule(37, 21, 39, 16, 2.2f, R_WARM, 0.3f, 0.75f);         /* the neck, craned */
    cv_ellipse(41, 13, 5.2f, 6.0f, R_WARM, 0.35f, 1.0f);            /* the head, hung forward */
    cv_ellipse(43, 19, 3.0f, 2.6f, R_WARM, 0.2f, 0.6f);            /* the jaw, dropped */
    cv_flat(44.5f, 17, 1.8f, 1.8f, 0);                              /* the mouth hanging open, a tooth or two */
    cv_px(45, 16, sh(R_WARM, 0.95f)); cv_px(44, 19, sh(R_WARM, 0.9f));
    cv_flat(44, 11.5f, 1.6f, 1.5f, 0); cv_eye(44, 12, (uint8_t)(R_FLAME + 13));   /* a sunken socket, its sulphur light */
    cv_px(46, 14, 0);                                               /* the nose, rotted to a hole */
    for (int i = 0; i < 5; i++) cv_line(37 + i, 8, 34 + i * 0.8f, 13 + i * 1.5f, sh(R_IRON, 0.2f));   /* hair, down the back */
    cv_capsule(37, 23, 52, 25, 2.0f, R_WARM, 0.35f, 0.9f);         /* the near arm, reaching */
    dead_hand(54, 26, 0.6f, 0.35f, R_WARM, 1.1f);
    cv_rot(R_WARM, 0.6f, 963);
    for (int y = 0; y < TS; y++)                                    /* grave-dirt clotted on it */
        for (int x = 0; x < TS; x++) {
            uint8_t c = cv_sprite[y * TS + x];
            if (((c > R_WARM && c < R_WARM + 16) || (c > R_STONE && c < R_STONE + 16)) && fbm(x / 3.0f, y / 3.0f, 16, 213) > 0.74f - (float)y * 0.004f)
                cv_px(x, y, sh(R_WOOD, 0.06f + 0.14f * h01(x, y, 214)));
        }
    cv_outline(0);
    cv_store(id);
}

/* the Drowned from the side: the swollen belly out in front, the head sunk, weed down its back */
static void spr_drowned_side(int id, float w) {
    cv_begin(cv_sprite, TS, TS);
    cv_flat(32, 62, 15, 1.8f, R_WARM + 1);
    cv_capsule(28, 46, 28 - 3 * w, 60, 2.0f, R_GHOST, 0.1f, 0.4f); /* the far leg */
    cv_capsule(30, 26, 38, 40, 3.0f, R_GHOST, 0.1f, 0.4f);         /* the far arm, hanging */
    cv_ellipse(33, 38, 8.0f, 8.5f, R_GHOST, 0.15f, 0.7f);          /* the belly, swollen tight on a thin frame */
    cv_ellipse(33, 27, 4.5f, 5.0f, R_GHOST, 0.15f, 0.6f);          /* a narrow chest over it, hunched */
    cv_flat(44, 40, 1.4f, 1.8f, sh(R_GHOST, 0.2f));
    cv_rot(R_GHOST, 0.3f, 965);
    for (int i = 0; i < 10; i++) cv_px((int)(22 + 20 * h01(i, 3, 967)), (int)(26 + 22 * h01(i, 4, 967)), sh(R_BONE, 0.55f + 0.3f * h01(i, 5, 967)));
    cv_capsule(31, 46, 31 + 3 * w, 60, 2.2f, R_GHOST, 0.15f, 0.6f); /* the near leg */
    cv_ellipse(35 + 3 * w, 61.5f, 4.0f, 1.6f, R_GHOST, 0.15f, 0.55f);
    cv_capsule(34, 22, 37, 18, 1.6f, R_GHOST, 0.15f, 0.55f);       /* the neck, the head hung forward on it */
    cv_ellipse(38, 15, 5.0f, 7.5f, R_GHOST, 0.2f, 0.75f);          /* a long face, the hair plastered down its back */
    for (int y = 8; y < 26; y++) cv_px(34 - (y > 16) - (y > 21), y, sh(R_IRON, 0.12f)), cv_px(35 - (y > 18), y, sh(R_IRON, 0.18f));
    cv_flat(41, 16, 1.6f, 1.3f, 0); cv_eye(41, 16, (uint8_t)(R_GLOW + 14));   /* a small cold eye in its socket */
    cv_flat(41.5f, 20, 1.6f, 2.6f, 0);                              /* the mouth, slack and black, and the water out of it */
    for (int y = 22; y < 34; y++) if (h01(y, 1, 969) > 0.3f) cv_px(43 + (y & 1), y, sh(R_GHOST, 0.5f + 0.3f * h01(y, 2, 969)));
    for (int i = 0; i < 6; i++) {                                   /* weed, down the back of its head and shoulders */
        float x = 30 + i * 1.5f, len = 8 + 10 * h01(i, 1, 971);
        for (int y = 0; y < (int)len; y++) cv_px((int)(x - y * 0.3f + sinf(y * 0.5f + i + w) * 0.8f), 11 + y, i & 1 ? sh(R_IRON, 0.12f) : sh(R_MOSS, 0.1f + 0.22f * (1.0f - y / len)));   /* hair and weed */
    }
    cv_capsule(36, 24, 42, 44, 1.8f, R_GHOST, 0.25f, 0.8f);        /* the near arm, too long */
    dead_hand(42, 47, 1.5f, 0.3f, R_GHOST, 1.15f);
    for (int i = 0; i < 6; i++) cv_px((int)(22 + 22 * h01(i, 3, 973)), (int)(42 + 16 * h01(i, (int)(w * 3 + 4), 973)), sh(R_GHOST, 0.8f));   /* drips */
    cv_outline(0);
    cv_store(id);
}

/* the bone hound from the side: running right, the long skull low, the tail up */
static void spr_hound_side(int id, float w) {
    cv_begin(cv_sprite, TS, TS);
    cv_flat(32, 62, 22, 1.8f, R_WARM + 1);
    float bob = w * 0.8f;
    /* far legs, darker */
    cv_capsule(42, 33 + bob, 40 - 4 * w, 46, 1.1f, R_BONE, 0.12f, 0.45f); cv_capsule(40 - 4 * w, 46, 42 - 6 * w, 60, 1.0f, R_BONE, 0.12f, 0.45f);
    cv_capsule(18, 32 - bob, 20 + 4 * w, 45, 1.2f, R_BONE, 0.12f, 0.45f); cv_capsule(20 + 4 * w, 45, 16 + 5 * w, 60, 1.0f, R_BONE, 0.12f, 0.45f);
    cv_capsule(16, 30, 9, 24, 1.0f, R_BONE, 0.3f, 0.8f); cv_capsule(9, 24, 6, 17 + w, 0.9f, R_BONE, 0.3f, 0.8f);   /* the tail */
    for (int i = 0; i < 12; i++) cv_ellipse(17 + i * 2.5f, 30 + bob * 0.5f - sinf(i / 11.0f * PI_F) * 2.0f, 1.5f, 1.3f, R_BONE, 0.35f, 0.95f);   /* the spine */
    for (int i = 0; i < 6; i++) {                                   /* ribs, hanging under it */
        float x = 25 + i * 3.0f, top = 31 + bob * 0.5f - sinf((x - 17) / 28.0f * PI_F) * 2.0f, drop = 11 - fabsf(i - 2.0f) * 1.4f;
        cv_curve(x, top, x + 3.5f, top + drop * 0.6f, x + 1.0f, top + drop, sh(R_BONE, 0.85f));
    }
    cv_ellipse(18, 33, 4.5f, 3.2f, R_BONE, 0.3f, 0.85f);            /* the pelvis */
    /* near legs */
    cv_bone(42, 34 + bob, 42 + 5 * w, 46, 1.2f, 0.3f, 0.9f); cv_capsule(42 + 5 * w, 46, 44 + 7 * w, 60, 1.1f, R_BONE, 0.3f, 0.9f);
    cv_capsule(44 + 7 * w, 61, 48 + 7 * w, 61.5f, 1.0f, R_BONE, 0.3f, 0.85f);
    cv_bone(19, 33 - bob, 17 - 4 * w, 45, 1.3f, 0.3f, 0.95f); cv_capsule(17 - 4 * w, 45, 22 - 5 * w, 52, 1.1f, R_BONE, 0.3f, 0.9f);
    cv_capsule(22 - 5 * w, 52, 19 - 5 * w, 60, 1.0f, R_BONE, 0.3f, 0.9f); cv_capsule(19 - 5 * w, 61, 23 - 5 * w, 61.5f, 1.0f, R_BONE, 0.3f, 0.85f);
    for (int y = 26; y < 35; y++) cv_px(46 + (y & 1), y + (int)bob, sh(R_IRON, 0.35f + 0.3f * (y & 1)));   /* the collar */
    cv_px(48, 27 + (int)bob, sh(R_IRON, 0.9f)); cv_px(48, 31 + (int)bob, sh(R_IRON, 0.9f));
    float hx = 51, hy = 28 + bob;                                   /* the skull, long in the snout */
    dog_skull_at((int)hx - 5, (int)hy - 4, 1.0f, 1, 1);             /* the hand-drawn skull, facing right */
    cv_bone_decay(975);
    cv_outline(0);
    cv_store(id);
}

/* the Wraith from the side: leaning on, its peaked hood ahead of it, the void in its mouth and one light, the cerements
 * wound slantwise and streaming back into strips, one long hand held out low */
static void spr_wraith_side(int id, float ph) {
    cv_begin(cv_sprite, TS, TS);
    for (int y = 4; y < TS; y++)
        for (int x = 0; x < TS; x++) {
            float t = (y - 4) / 59.0f, lean = 7.0f * (1.0f - t);
            float front = 40.0f + lean - (y < 16 ? (16 - y) * 0.7f : 0.0f) + sinf(y * 0.25f + ph) * 0.6f;
            float back = 26.0f + lean - t * 20.0f + sinf(y * 0.18f + ph) * 2.0f;
            if (x > front || x < back) continue;
            if (y > 42) {                                            /* coming loose into strips, streaming back */
                float at = y * 0.9f + x * 0.45f + sinf(ph + y * 0.2f) * 1.5f + 64.0f;
                int sid = (int)floorf(at / 3.0f);
                float within = at - sid * 3.0f, end = 50.0f + 11.0f * h01(sid, 3, 981);
                if (y > end || within < 0.7f) continue;
            }
            float u = (x - back) / (front - back + 0.01f), v = 0.07f + 0.14f * sinf(u * PI_F) - t * 0.05f;
            float b = (y - x * 0.35f + 3.0f * (fbm(x / 7.0f, y / 7.0f, 16, 983) - 0.45f)) / 4.8f, fb = b - floorf(b);   /* wound slantwise */
            if (u > 0.9f || u < 0.06f) { cv_px(x, y, sh(R_GHOST, 0.28f + 0.3f * (u > 0.9f))); continue; }   /* the cold rim */
            if (fb < 0.13f) { cv_px(x, y, 0); continue; }
            if (fb < 0.23f && u > 0.6f && h01((int)floorf(b), 1, 985) < 0.45f) { cv_px(x, y, sh(R_GHOST, 0.14f)); continue; }
            if (fb > 0.45f && fb < 0.72f) v += 0.07f;
            if (y > 18 && fbm(x / 3.5f, y / 3.5f, 32, 987) > 0.7f) { cv_px(x, y, 0); continue; }   /* torn through */
            cv_px(x, y, sh(R_IRON, v));
        }
    for (int y = 0; y < 22; y++)                                    /* the hood, peaked forward, ragged at the shoulder */
        for (int x = 28; x < 50; x++) {
            float top = 2.0f + (x - 43) * (x - 43) * 0.07f;
            float fr = 47.5f - (y > 10 ? (y - 10) * 0.55f : 0.0f), bk = 31.0f + y * 0.05f - (y > 12 ? (y - 12) * 0.5f : 0.0f);
            if (y < top || x > fr || x < bk || y > 18.0f + 3.0f * h01(x / 2, 5, 989)) continue;
            float u = (x - bk) / (fr - bk);
            uint8_t c = x > fr - 1.5f || y < top + 1.0f ? sh(R_GHOST, 0.32f) : sh(R_IRON, 0.08f + 0.12f * u - (sinf((x - 43) * 0.9f + y * 0.3f) > 0.6f ? 0.06f : 0.0f));
            cv_px(x, y, c);
        }
    for (int y = 5; y < 18; y++)                                    /* its mouth, from the side: a deep notch of nothing */
        for (int x = 40; x < 48; x++) {
            float w = y < 9 ? 3.2f * powf((y - 4.5f) / 4.5f, 0.7f) : 3.2f - (y > 15 ? (y - 15) * 0.8f : 0.0f);
            if (x + 0.5f > 47.0f - (y > 10 ? (y - 10) * 0.55f : 0.0f) - 2.0f * w) cv_px(x, y, 0);
        }
    cv_eye(45, 11, C_WHITE); cv_px(45, 12, (uint8_t)(R_GLOW + 13)); cv_px(44, 11, (uint8_t)(R_GLOW + 10));   /* one light */
    cv_capsule(38, 18, 46, 32, 3.8f, R_IRON, 0.06f, 0.3f);          /* the sleeve */
    cv_line(41, 17, 49, 31, sh(R_GHOST, 0.4f));
    for (int j = 0; j < 3; j++) {                                   /* rags off it, streaming back */
        float px = 40.0f + j * 2.5f, py = 22.0f + j * 4.0f;
        for (int q = 0; q < 7 + j * 2; q++) cv_px((int)(px - q * 0.6f + sinf(ph * 1.4f + q * 0.4f + j) * 0.8f), (int)(py + q * 0.7f), sh(R_IRON, 0.1f));
    }
    wraith_hand(48, 35, 0.75f, 1, 1.0f);                            /* the hand, and fingers too long for it */
    for (int i = 0; i < 14; i++) {                                  /* frost falling off it */
        float x = 8 + 40 * h01(i, 1, 979), y = 50 + 13 * h01(i, (int)(ph * 2) + 2, 979);
        if (cv_sprite[(int)y * TS + (int)x] == C_CLEAR) cv_px((int)x, (int)y, sh(R_GHOST, 0.45f + 0.4f * h01(i, 3, 979)));
    }
    cv_store(id);
}

/* a spider from the side, in a body ramp: legs banded dark (band) or plain, bristling at the knee; the abdomen as big as
 * it is, furred with chevrons (banded) or pitted, eggs in it; the face drawn over the near legs, one great eye or three
 * small ones showing, a fang with its drop */
static void spider_side(int frame, int ramp, int band, float ab, int eggs, int big_eyes) {
    cv_flat(32, 62, 24, 1.8f, R_WARM + 1);
    for (int l = 0; l < 4; l++) {                                   /* far legs */
        float k = ((l + frame) & 1) ? 2.5f : -2.5f, ax = 34 + l * 1.5f, fx = 14 + l * 11.0f + k, kx = (ax + fx) * 0.5f + (l < 2 ? -3 : 3);
        chitin_leg(ax, 44, kx, 33 + l * 0.6f, fx, 61, 1.1f, ramp, 0.02f, 0.22f, band, 961 + (uint32_t)l);
    }
    float acx = 25 - (ab - 1.0f) * 6, acy = 42 - (ab - 1.0f) * 6;
    if (band >= 0) {                                                /* furred, chevrons along it */
        hunter_abdomen(acx, acy, 10.0f * ab, 8.0f * ab, ramp, 1, 963);
        for (int i = 0; i < 3; i++) cv_line(18 + i * 5, 35, 20 + i * 5, 49, sh(band, 0.12f));
    } else spider_abdomen(acx, acy, 10.0f * ab, 8.0f * ab, ramp, 965);
    if (eggs) for (int i = 0; i < 10; i++) {
        float a = h01(i, 1, 981) * TAU_F, r = 8.0f * ab * sqrtf(h01(i, 2, 981));
        cv_ellipse(acx + cosf(a) * r, acy + sinf(a) * r * 0.75f, 2.0f, 1.7f, R_BONE, 0.15f, 0.45f);
    }
    if (band < 0) cv_flat(22, 36, 2.6f, 1.6f, (uint8_t)(R_BLOOD + 9));
    cv_ellipse(37, 45, 6.5f, 4.6f, ramp, 0.06f, 0.45f);            /* the head */
    for (int l = 0; l < 4; l++) {                                   /* near legs */
        float k = ((l + frame + 1) & 1) ? 2.5f : -2.5f, ax = 35 + l * 1.5f, fx = 12 + l * 12.0f + k, kx = (ax + fx) * 0.5f + (l < 2 ? -4 : 4);
        chitin_leg(ax, 46, kx, 31 + l * 0.8f, fx, 61.5f, 1.3f, ramp, 0.06f, 0.42f, band, 967 + (uint32_t)l);
    }
    cv_ellipse(41.5f, 44.5f, 3.2f, 2.8f, ramp, 0.1f, 0.45f);       /* its face, before the legs */
    if (big_eyes) { cv_flat(42.5f, 43.5f, 1.8f, 1.8f, 0); cv_eye(42, 43, (uint8_t)(R_FLAME + 14)); cv_px(43, 43, (uint8_t)(R_FLAME + 11)); cv_px(42, 44, (uint8_t)(R_FLAME + 10)); cv_px(40, 46, (uint8_t)(R_FLAME + 6)); }
    else { cv_px(41, 42, (uint8_t)(R_FLAME + 9)); cv_px(40, 43, (uint8_t)(R_FLAME + 7)); cv_eye(43, 43, (uint8_t)(R_FLAME + 13)); }
    cv_ellipse(42.5f, 47.5f, 1.6f, 1.8f, ramp, 0.12f, 0.36f);      /* the jaw, and its fang */
    for (int k = 0; k <= 4; k++) cv_px(43 - (k > 2), 48 + k, eggs ? sh(R_BONE, 0.45f + 0.1f * k) : sh(R_IRON, 0.3f + 0.08f * k));
    cv_px(42, 53, (uint8_t)(R_MOSS + 14)); cv_px(42, 54, (uint8_t)(R_MOSS + 10));
    cv_outline(0);
}
static void spr_hunter_side(int id, int frame) { cv_begin(cv_sprite, TS, TS); spider_side(frame, R_WOOD, R_IRON, 1.0f, 0, 1); cv_store(id); }
static void spr_brood_side(int id, int frame) { cv_begin(cv_sprite, TS, TS); spider_side(frame, R_IRON, -1, 1.5f, 1, 0); cv_store(id); }

/* the ghoul from the side: on all fours, the back arched high, the head low and forward */
static void spr_ghoul_side(int id, float w) {
    cv_begin(cv_sprite, TS, TS);
    cv_flat(32, 62, 22, 1.8f, R_WARM + 1);
    cv_capsule(20, 38, 22 - 3 * w, 50, 2.4f, R_WARM, 0.15f, 0.45f); cv_capsule(22 - 3 * w, 50, 18 - 3 * w, 61, 1.8f, R_WARM, 0.15f, 0.45f);   /* far hind leg */
    cv_capsule(40, 36, 44 - 3 * w, 48, 2.0f, R_WARM, 0.15f, 0.45f); cv_capsule(44 - 3 * w, 48, 45 - 4 * w, 61, 1.6f, R_WARM, 0.15f, 0.45f);   /* far arm */
    float back[10] = { 16, 40, 22, 28, 32, 25, 42, 30, 40, 42 };   /* the body, arched */
    cv_poly(back, 5, R_WARM, 0.85f, 0.35f, 25, 17);
    for (int i = 0; i < 5; i++) cv_ellipse(22 + i * 4.5f, 28 - sinf((i + 0.5f) / 5.0f * PI_F) * 3.0f, 1.6f, 1.3f, R_WARM, 0.5f, 1.0f);   /* spine */
    for (int i = 0; i < 4; i++) cv_line(26 + i * 4, 32, 25 + i * 4, 39, sh(R_WARM, 0.25f));   /* ribs */
    cv_capsule(20, 40, 24 + 3 * w, 50, 2.6f, R_WARM, 0.3f, 0.8f); cv_capsule(24 + 3 * w, 50, 20 + 3 * w, 61, 2.0f, R_WARM, 0.3f, 0.8f);   /* near hind leg */
    cv_capsule(41, 36, 47 + 3 * w, 47, 2.2f, R_WARM, 0.3f, 0.85f); cv_capsule(47 + 3 * w, 47, 48 + 5 * w, 61, 1.8f, R_WARM, 0.3f, 0.85f);   /* near arm */
    for (int c = -1; c <= 1; c++) cv_line(48 + 5 * w, 61, 51 + 5 * w + c, 62, sh(R_BONE, 0.7f));
    float ear[6] = { 45, 33, 39, 26, 43, 35 };                       /* an ear laid back */
    cv_poly(ear, 3, R_WARM, 0.4f, 0.9f, 26, 9);
    cv_ellipse(48, 37, 5.5f, 4.8f, R_WARM, 0.35f, 1.0f);            /* the head, low */
    cv_capsule(50, 39, 55, 41, 2.2f, R_WARM, 0.25f, 0.8f);         /* the muzzle */
    cv_flat(54, 42, 2.0f, 1.0f, 0);
    for (int i = 0; i < 3; i++) cv_px(51 + i * 2, 42, sh(R_BONE, 0.9f));
    cv_flat(50, 36, 1.4f, 1.2f, 0); cv_px(50, 36, (uint8_t)(R_FLAME + 9));
    cv_rot(R_WARM, 0.3f, 983);
    cv_outline(0);
    cv_store(id);
}

/* the knight from the side: skeleton in plate, the greatsword held upright before it */
static void spr_knight_side(int id, float w) {
    cv_begin(cv_sprite, TS, TS);
    skel_side(w, 0);
    float sw = w * 1.5f;
    cv_capsule(31 - 4 * sw + 1, 52, 31 - 5 * sw, 59, 2.4f, R_IRON, 0.12f, 0.5f);   /* far greave */
    cv_capsule(31, 42, 31 + 4 * sw, 50, 2.8f, R_IRON, 0.25f, 0.95f);   /* near cuisse and greave */
    cv_capsule(31 + 4 * sw, 52, 31 + 5 * sw - 1, 59, 2.4f, R_IRON, 0.25f, 0.95f);
    cv_ellipse(31 + 4 * sw, 51, 2.4f, 2.0f, R_IRON, 0.4f, 1.0f);
    cv_ellipse(31 + 5 * sw + 1, 61.5f, 3.6f, 1.5f, R_IRON, 0.3f, 0.9f);
    for (int y = 19; y <= 38; y++)                                  /* the breastplate, curved out */
        for (int x = 26; x <= 40; x++) {
            float front = 38.0f + 2.0f * sinf((y - 19) / 19.0f * PI_F) - (y > 32 ? (y - 32) * 0.5f : 0.0f), back = 27.0f + (y > 30 ? (y - 30) * 0.2f : 0.0f);
            if (x < back || x > front) continue;
            float u = (x - back) / (front - back);
            cv_px(x, y, sh(R_IRON, 0.25f + 0.6f * u - (x == (int)front ? 0.0f : 0.0f) + (fabsf(u - 0.85f) < 0.08f ? 0.2f : 0.0f)));
        }
    for (int b = 0; b < 3; b++) for (int x = 26; x <= 37; x++) cv_px(x, 39 + b * 2, sh(R_IRON, 0.35f + 0.3f * (x - 26) / 11.0f));   /* the fauld */
    cv_ellipse(31, 21, 5.0f, 4.0f, R_IRON, 0.3f, 1.0f);            /* pauldron */
    cv_capsule(32, 22, 37, 30, 1.4f, R_BONE, 0.3f, 0.9f);          /* arms to the grip */
    cv_capsule(37, 30, 41, 29, 1.2f, R_BONE, 0.3f, 0.9f);
    cv_ellipse(41.5f, 29, 2.2f, 2.0f, R_IRON, 0.3f, 0.9f);         /* gauntlets */
    cv_capsule(42, 33, 42, 27, 1.2f, R_WOOD, 0.2f, 0.6f);          /* the grip, the guard, the blade up */
    cv_capsule(38, 26, 46, 26, 1.0f, R_IRON, 0.35f, 0.95f);
    for (int y = 2; y < 26; y++) for (int o = -1; o <= 1; o++) cv_px(42 + o, y, sh(R_GHOST, 0.55f + 0.2f * o + (y < 4 ? -0.1f : 0.0f)));
    for (int y = 2; y <= 16; y++)                                   /* the great helm, in profile */
        for (int x = 26; x <= 39; x++) {
            float nx = (x + 0.5f - 32.5f) / 6.5f, top = 2.0f + 3.0f * nx * nx;
            if (fabsf(nx) > 1.0f || y < top) continue;
            float v = 0.25f + 0.6f * lit(nx, (y - 9) / 9.0f, sqrtf(fmaxf(0, 1 - nx * nx)));
            if (y == 9 && x > 34) v = 0.0f;                         /* the slit at the front */
            if (y > 11 && x > 35 && (y & 1)) v -= 0.3f;             /* breaths */
            cv_px(x, y, sh(R_IRON, v));
        }
    cv_px(37, 9, (uint8_t)(R_FLAME + 11));
    cv_outline(0);
    cv_store(id);
}

/* the shade from the side: narrower, leaning into its drift, one arm of smoke reaching ahead */
static void spr_shade_side(int id, float ph) {
    cv_begin(cv_sprite, TS, TS);
    for (int y = 4; y < TS; y++)
        for (int x = 0; x < TS; x++) {
            float dx = x + 0.5f - 31.0f + (y - 30) * 0.12f, hw;
            if (y < 16) { float hy = (y - 10.0f) / 6.0f; hw = 5.0f * sqrtf(fmaxf(0.0f, 1.0f - hy * hy)); dx -= 1.5f; }   /* the head, forward */
            else if (y < 20) { hw = 3.0f; dx -= 1.0f; }
            else hw = 8.5f + (y < 30 ? (y - 20) * 0.2f : 2.0f - (y - 30) * 0.1f);
            hw += sinf(y * 0.4f + ph) * 0.8f;
            float edge = hw - fabsf(dx);
            if (edge < 0.0f) continue;
            float fadey = y > 44 ? (y - 44) / 19.0f : 0.0f, n = h01(x, y, 515 + (int)(ph * 5));
            if (n < fadey * 1.1f) continue;
            if (edge < 1.5f && ((x + y) & 1)) continue;
            cv_px(x, y, edge < 2.5f ? sh(R_PURPLE, 0.12f + 0.12f * (1.0f - fadey)) : sh(R_IRON, 0.02f + 0.04f * n));
        }
    for (int i = 0; i <= 3; i++) cv_px(26 - i, 7 + i * 2, sh(R_PURPLE, 0.15f));   /* the hood's point, trailing */
    for (int i = 0; i <= 14; i++) {                                 /* an arm of smoke, reaching */
        float t = i / 14.0f, x = lerpf(35, 50, t), y = lerpf(23, 33 + sinf(ph) * 2.0f, t), r = 3.0f - t * 1.3f;
        for (int yy = (int)(y - r); yy <= (int)(y + r); yy++)
            for (int xx = (int)(x - r); xx <= (int)(x + r); xx++)
                if (hypotf(xx + 0.5f - x, yy + 0.5f - y) <= r) cv_px(xx, yy, (xx + yy) & 1 ? sh(R_PURPLE, 0.15f) : sh(R_IRON, 0.04f));
    }
    cv_px(35, 10, sh(R_PURPLE, 0.85f)); cv_px(35, 11, sh(R_PURPLE, 0.5f));
    cv_store(id);
}

/* the imp from the side: a hop, the tail and its ember behind, a wing up, the long muzzle ahead (IMP_SIDE) */
static void spr_imp_side(int id, int pose) {
    cv_begin(cv_sprite, TS, TS);
    float up = pose == 0 ? -3.0f : 0.0f, hy = 5.0f + up;
    cv_flat(32, 62, 11, 1.6f, R_WARM + 1);
    cv_capsule(28, hy + 33, 16, hy + 40, 1.2f, R_BRICK, 0.1f, 0.4f);   /* the tail, its ember */
    cv_capsule(16, hy + 40, 10, hy + 31, 1.0f, R_BRICK, 0.1f, 0.4f);
    cv_flat(9.5f, hy + 30, 1.6f, 1.8f, (uint8_t)(R_FLAME + 11));
    float fw[8] = { 30, hy + 20, 12, hy + 2 + up, 10, hy + 18, 22, hy + 27 };   /* the far wing */
    cv_poly(fw, 4, R_BRICK, 0.03f, 0.15f, hy + 2 + up, 26);
    cv_capsule(30, hy + 35, 36, hy + 43, 2.2f, R_BRICK, 0.08f, 0.4f);   /* the far leg */
    cv_capsule(36, hy + 43, 32, 55, 1.4f, R_BRICK, 0.08f, 0.4f); cv_capsule(32, 55, 35, 61, 1.1f, R_BRICK, 0.08f, 0.4f);
    cv_ellipse(32, hy + 24, 5.0f, 6.0f, R_BRICK, 0.15f, 0.6f);     /* chest, hunched forward */
    cv_ellipse(31, hy + 32, 4.0f, 4.5f, R_BRICK, 0.1f, 0.45f);
    cv_capsule(32, hy + 35, 39, hy + 44, 2.4f, R_BRICK, 0.12f, 0.55f);   /* the near leg */
    cv_capsule(39, hy + 44, 35, 55, 1.6f, R_BRICK, 0.12f, 0.5f); cv_capsule(35, 55, 39, 61, 1.2f, R_BRICK, 0.12f, 0.5f);
    for (int c = -1; c <= 1; c++) cv_line(39, 61, 42 + c * 1.5f, 62, sh(R_BONE, 0.5f));
    float nw[8] = { 31, hy + 20, 16, hy + 4 + up, 13, hy + 20, 24, hy + 29 };   /* the near wing */
    cv_poly(nw, 4, R_BRICK, 0.06f, 0.26f, hy + 4 + up, 25);
    cv_capsule(31, hy + 20, 16, hy + 4 + up, 0.9f, R_BRICK, 0.1f, 0.4f);
    cv_capsule(34, hy + 21, 40, hy + 29, 1.5f, R_BRICK, 0.15f, 0.55f);   /* an arm, out ahead, talons */
    cv_capsule(40, hy + 29, 45, hy + 33, 1.2f, R_BRICK, 0.15f, 0.5f);
    for (int c = -1; c <= 1; c++) cv_line(45, hy + 33, 47 + c * 0.8f, hy + 36, sh(R_BONE, 0.6f));
    imp_crust(hy + 17, hy + 45, 6, 525);
    grid_stamp(IMP_SIDE, 15, 15, 28, (int)hy, R_BRICK, R_BONE, (uint8_t)(R_FLAME + 15), 0);
    cv_outline(0);
    cv_store(id);
}

/* the deacon from the side: the robe dragging behind, the staff carried before it, the skull in the hood */
static void spr_deacon_side(int id, float w) {
    cv_begin(cv_sprite, TS, TS);
    cv_flat(31, 62, 14, 1.8f, R_WARM + 1);
    float stx = 43.0f + w * 1.5f;                                   /* the staff, planted ahead */
    cv_capsule(stx, 9, stx - 1.0f, 62, 1.2f, R_WOOD, 0.2f, 0.65f);
    cv_skull(stx, 6, 0.7f, 0.3f, 1.0f);
    cv_px((int)stx - 1, 7, (uint8_t)(R_FLAME + 6)); cv_px((int)stx + 2, 7, (uint8_t)(R_FLAME + 6));
    for (int y = 16; y < 63; y++) {                                 /* the robe */
        float t = (y - 16) / 46.0f, front = 36.0f + 4.0f * t + w * 2.0f * t, back = 26.0f - 6.0f * t - (y > 50 ? (y - 50) * 0.5f : 0.0f) + w * 0.8f * t;
        for (int x = (int)back - 1; x <= (int)front + 1; x++) {
            if (x + 0.5f < back || x + 0.5f > front) continue;
            if (y > 58 && h01(x, y, 341 + (w > 0)) < (y - 58) / 5.0f) continue;
            float u = (x + 0.5f - back) / (front - back), v = 0.12f + 0.42f * sinf(u * PI_F) * (1.0f - 0.3f * t) + 0.08f * u;
            if (sinf(x * 0.9f + y * 0.12f) > 0.65f) v -= 0.08f;     /* folds */
            cv_px(x, y, sh(R_PURPLE, v));
        }
        if (y >= 20 && y < 52) { int x = (int)(front - 1.0f); cv_px(x, y, sh(R_BLOOD, 0.35f)); cv_px(x - 1, y, sh(R_BLOOD, 0.25f)); }   /* the stole */
    }
    cv_capsule(31, 21, 37, 30, 3.4f, R_PURPLE, 0.15f, 0.55f);      /* the sleeve, out to the staff */
    cv_capsule(37, 30, stx - 1.5f, 33, 3.0f, R_PURPLE, 0.15f, 0.5f);
    cv_ellipse(stx - 0.5f, 33, 1.9f, 1.8f, R_BONE, 0.3f, 0.95f);
    cv_ellipse(30, 12, 7.5f, 8.5f, R_PURPLE, 0.15f, 0.6f);         /* the hood, its point behind */
    float pt[6] = { 25, 6, 19, 12, 25, 15 };
    cv_poly(pt, 3, R_PURPLE, 0.1f, 0.35f, 6, 9);
    cv_flat(35, 13.5f, 3.4f, 5.6f, 0);                              /* the skull, in the shadow at the front */
    cv_ellipse(35.5f, 13.0f, 2.4f, 3.4f, R_BONE, 0.25f, 0.8f);
    cv_px(36, 12, (uint8_t)(R_FLAME + 8));
    for (int i = 0; i < 3; i++) cv_px(35 + i, 16, sh(R_BONE, i & 1 ? 0.4f : 0.85f));
    cv_bone_decay(343);
    cv_outline(0);
    cv_store(id);
}

/* the gargoyle from the side, in flight: the wings up and then down, the tail behind, claws hanging */
static void garg_wing(int down, float ox, float oy, float lo, float hi) {
    static const float UP[14] = { 31, 27, 27, 8, 20, 1, 12, 7, 7, 18, 19, 24, 26, 32 }, DN[14] = { 31, 28, 28, 45, 22, 57, 14, 53, 9, 44, 18, 37, 25, 33 };
    const float *w = down ? DN : UP;
    float p[14];
    for (int i = 0; i < 7; i++) { p[2 * i] = w[2 * i] + ox; p[2 * i + 1] = w[2 * i + 1] + oy; }
    cv_poly(p, 7, R_STONE, lo, hi, down ? 28 + oy : 1 + oy, 30);
    for (int i = 1; i <= 4; i++) cv_line(p[0], p[1], p[2 * i], p[2 * i + 1], sh(R_STONE, lo + 0.08f));   /* ribs */
}
static void spr_garg_side(int id, int down) {
    cv_begin(cv_sprite, TS, TS);
    garg_wing(down, 3.0f, -1.0f, 0.04f, 0.25f);                     /* the far wing */
    cv_capsule(23, 38, 13, 44, 1.8f, R_STONE, 0.15f, 0.6f);        /* the tail and its spade */
    cv_capsule(13, 44, 8, 40, 1.3f, R_STONE, 0.15f, 0.6f);
    float sp[6] = { 8, 36, 4, 41, 10, 42 };
    cv_poly(sp, 3, R_STONE, 0.15f, 0.6f, 36, 6);
    cv_capsule(29, 39, 31, 48, 2.6f, R_STONE, 0.12f, 0.5f);        /* the far leg, hanging */
    cv_capsule(31, 48, 28, 55, 2.0f, R_STONE, 0.12f, 0.5f);
    cv_ellipse(32, 34, 10.0f, 7.0f, R_STONE, 0.2f, 0.85f);         /* the body, hunched forward */
    cv_capsule(34, 40, 37, 49, 2.8f, R_STONE, 0.2f, 0.75f);        /* the near leg */
    cv_capsule(37, 49, 34, 56, 2.2f, R_STONE, 0.2f, 0.7f);
    for (int c = -1; c <= 1; c++) { cv_line(34, 56, 35 + c * 2.0f, 59.5f, sh(R_STONE, 0.35f)); cv_line(28, 55, 29 + c * 2.0f, 58.5f, sh(R_STONE, 0.25f)); }
    cv_capsule(38, 31, 45, 38, 2.2f, R_STONE, 0.25f, 0.8f);        /* an arm, claws ahead */
    for (int c = -1; c <= 1; c++) cv_line(45, 38, 47.5f, 40.0f + c * 1.5f, sh(R_STONE, 0.35f));
    garg_wing(down, 0.0f, 0.0f, 0.12f, 0.6f);                       /* the near wing */
    { float ev[6] = { 41, 19, 34, 15, 40, 23 }; cv_poly(ev, 3, R_STONE, 0.15f, 0.55f, 15, 8); }   /* a fin ear, swept back */
    cv_capsule(42, 15, 40, 10, 1.7f, R_STONE, 0.2f, 0.8f);         /* a bull's horn, up and forward */
    cv_capsule(40, 10, 44, 7, 1.2f, R_STONE, 0.2f, 0.75f);
    grid_stamp(GARG_SIDE, 14, 15, 38, 14, R_STONE, R_STONE, (uint8_t)(R_FLAME + 12), 0);
    for (int y = 0; y < TS; y++)                                    /* weathering on the stone */
        for (int x = 0; x < TS; x++) {
            uint8_t *p = &cv_sprite[y * TS + x];
            if (*p < R_STONE || *p >= R_STONE + 16) continue;
            float n = fbm(x / 4.0f, y / 4.0f, 16, 549);
            if (n < 0.3f) *p = (uint8_t)(*p - 2 < R_STONE + 1 ? R_STONE + 1 : *p - 2);
            else if (h01(x, y, 551) > 0.96f) *p = (uint8_t)(*p - 4 < R_STONE + 1 ? R_STONE + 1 : *p - 4);
        }
    cv_outline(0);
    cv_store(id);
}

/* the side views of bone age with the level, like the skeletons */
static uint8_t side_drawn[SP_GARG_SB - SP_SHAMB_SA + 1];          /* the new creatures' side views, as they are drawn */
static void spr_sides(void) {
    spr_skel_side(SP_SKEL_SA, 1.0f, 0); spr_skel_side(SP_SKEL_SB, -1.0f, 0);
    spr_skel_back(SP_SKEL_BKA, 1.0f); spr_skel_back(SP_SKEL_BKB, -1.0f);   /* 1.0.0b18: the backs */
    spr_shield_back(SP_SHIELD_BKA, 1.0f, 1); spr_shield_back(SP_SHIELD_BKB, -1.0f, 1);
    spr_skel_side(SP_SHIELD_SA, 1.0f, 1); spr_skel_side(SP_SHIELD_SB, -1.0f, 1);
    spr_bat_side(SP_BAT_SA, 1); spr_bat_side(SP_BAT_SB, 0);
    spr_spider_side(SP_SPIDER_SA, 0); spr_spider_side(SP_SPIDER_SB, 1);
    spr_ghost_side(SP_GHOST_SA, 0.0f); spr_ghost_side(SP_GHOST_SB, PI_F);
    spr_wskull_side();
    spr_shambler_side(SP_SHAMB_SA, 1.0f); spr_shambler_side(SP_SHAMB_SB, -1.0f);
    spr_drowned_side(SP_DROWN_SA, 1.0f); spr_drowned_side(SP_DROWN_SB, -1.0f);
    spr_hound_side(SP_HOUND_SA, 1.0f); spr_hound_side(SP_HOUND_SB, -1.0f);
    spr_wraith_side(SP_WRAITH_SA, 0.0f); spr_wraith_side(SP_WRAITH_SB, PI_F);
    spr_hunter_side(SP_HUNTER_SA, 0); spr_hunter_side(SP_HUNTER_SB, 1);
    spr_ghoul_side(SP_GHOUL_SA, 1.0f); spr_ghoul_side(SP_GHOUL_SB, -1.0f);
    spr_knight_side(SP_KNIGHT_SA, 1.0f); spr_knight_side(SP_KNIGHT_SB, -1.0f);
    spr_brood_side(SP_BROOD_SA, 0); spr_brood_side(SP_BROOD_SB, 1);
    spr_shade_side(SP_SHADE_SA, 0.0f); spr_shade_side(SP_SHADE_SB, 1.6f);
    spr_imp_side(SP_IMP_SA, 0); spr_imp_side(SP_IMP_SB, 1);
    spr_deacon_side(SP_DEACON_SA, 1.0f); spr_deacon_side(SP_DEACON_SB, -1.0f);
    spr_garg_side(SP_GARG_SA, 0); spr_garg_side(SP_GARG_SB, 1);
    spr_whip_side(SP_WHIP_SA, 1.0f); spr_whip_side(SP_WHIP_SB, -1.0f);
    spr_sexton_side(SP_SEXTON_SA, 1.0f); spr_sexton_side(SP_SEXTON_SB, -1.0f);
    spr_light_side(SP_LIGHT_SA, 1.0f); spr_light_side(SP_LIGHT_SB, -1.0f);
    for (int i = SP_SHAMB_SA; i <= SP_GARG_SB; i++) side_drawn[i - SP_SHAMB_SA] = 1;
    static const int FRONT[][3] = {                                /* not drawn from the side yet: the front view stands in */
        { SP_SHAMB_SA, SP_SHAMB_A, SP_SHAMB_B }, { SP_DROWN_SA, SP_DROWN_A, SP_DROWN_B }, { SP_HOUND_SA, SP_HOUND_A, SP_HOUND_B },
        { SP_WRAITH_SA, SP_WRAITH_A, SP_WRAITH_B }, { SP_HUNTER_SA, SP_HUNTER_A, SP_HUNTER_B }, { SP_GHOUL_SA, SP_GHOUL_A, SP_GHOUL_B },
        { SP_KNIGHT_SA, SP_KNIGHT_A, SP_KNIGHT_B }, { SP_BROOD_SA, SP_BROOD_A, SP_BROOD_B }, { SP_SHADE_SA, SP_SHADE_A, SP_SHADE_B },
        { SP_IMP_SA, SP_IMP_A, SP_IMP_B }, { SP_DEACON_SA, SP_DEACON_A, SP_DEACON_B }, { SP_GARG_SA, SP_GARG_A, SP_GARG_B },
    };
    for (int i = 0; i < (int)(sizeof FRONT / sizeof FRONT[0]); i++)
        if (!side_drawn[FRONT[i][0] - SP_SHAMB_SA]) {
            memcpy(crawl_spr[FRONT[i][0]], crawl_spr[FRONT[i][1]], TS * TS);
            memcpy(crawl_spr[FRONT[i][0] + 1], crawl_spr[FRONT[i][2]], TS * TS);
        }
}

static void find_glints(void) {
    for (int i = 0; i < NSPR; i++) {
        int bx = -1, by = -1, best = -1;
        for (int x = 0; x < TS; x++)
            for (int y = 0; y < TS; y++) {
                int c = crawl_spr[i][x * TS + y];
                if (c >= R_GOLD && c < R_GOLD + NGOLD && c > best) { best = c; bx = x; by = y; }
            }
        crawl_glint[i][0] = (int8_t)bx; crawl_glint[i][1] = (int8_t)by;
    }
}

static void spr_bestiary_bones(void);
static void spr_sides(void);

