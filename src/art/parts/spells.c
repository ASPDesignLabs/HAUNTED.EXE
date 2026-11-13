/* A part of crawl_art.c (docs/PLAN-tidy.md): it is #included there in its place and never compiled on its own.
 * Spells and what they leave (bolts, shards, the spear, wisps, the flask, the orb, the lance, rime, motes, burns); the pedestal and the items; the altar's fire; the Rune Shrine and its runes; the Cursed Coin. */

/* projectiles and the floor burning */
static void spr_ball(int id, float r, int ramp, int dark, int frame, int skull) {
    cv_begin(cv_sprite, TS, TS);
    for (int y = 0; y < TS; y++)
        for (int x = 0; x < TS; x++) {
            float dx = x + 0.5f - 32, dy = y + 0.5f - 32, d = sqrtf(dx * dx + dy * dy);
            float n = fbm(x / 3.0f + frame * 5.0f, y / 3.0f, 32, 61 + ramp) * 0.6f, rr = r * (0.85f + n);
            if (d > rr) continue;
            float heat = 1.05f - d / rr * 0.75f;
            uint8_t c = (uint8_t)(ramp + clampi((int)(heat * 15), 3, 15));
            if (dark && n > 0.42f && d > rr * 0.4f) c = (uint8_t)(dark + 2 + (int)(n * 6));
            cv_px(x, y, c);
        }
    if (skull) {                                                   /* a skull face in the flames */
        cv_flat(28, 30, 2.4f, 2.6f, R_BLOOD + 2); cv_flat(36, 30, 2.4f, 2.6f, R_BLOOD + 2);
        cv_flat(32, 35, 1.0f, 1.4f, R_BLOOD + 2);
        for (int i = -2; i <= 2; i++) cv_px(32 + i * 2, 39, R_BLOOD + 3);
    }
    for (int i = 0; i < 10; i++) {
        float a = h01(i, frame, 7 + ramp) * TAU_F, l = r + 2 + h01(i, frame + 3, 7) * 5;
        cv_px((int)(32 + cosf(a) * l), (int)(32 + sinf(a) * l), (uint8_t)(ramp + 8 + (i & 3)));
    }
    cv_store(id);
}

static void spr_frost(void) {
    cv_begin(cv_sprite, TS, TS);
    for (int k = 0; k < 5; k++) {                                  /* a cluster of ice shards */
        float a = k * TAU_F / 5 + 0.3f, l = 6 + (k & 1) * 3;
        float tri[6] = { 32 + cosf(a) * l, 32 + sinf(a) * l, 32 + cosf(a + 0.5f) * 2, 32 + sinf(a + 0.5f) * 2, 32 + cosf(a - 0.5f) * 2, 32 + sinf(a - 0.5f) * 2 };
        cv_poly(tri, 3, R_GLOW, 1.0f, 0.55f, 32 - l, 2 * l);
    }
    cv_flat(32, 32, 2.5f, 2.5f, C_WHITE);
    cv_store(SP_FROST);
}

static void spr_spear(void) {
    cv_begin(cv_sprite, TS, TS);
    for (int y = 0; y < TS; y++)                                   /* a sharpened bone, seen point-first, with a pale trail */
        for (int x = 0; x < TS; x++) {
            float dx = (x + 0.5f - 32) / 4.5f, dy = (y + 0.5f - 32) / 4.5f, d = dx * dx + dy * dy;
            if (d < 1.0f) cv_px(x, y, sh(R_BONE, 0.45f + 0.5f * lit(dx, dy, sqrtf(1.0f - d))));
            else if (d < 2.6f && h01(x, y, 3) < 0.5f) cv_px(x, y, sh(R_BONE, 0.25f));
        }
    cv_flat(31, 31, 1.2f, 1.2f, sh(R_BONE, 1.0f));
    cv_store(SP_SPEAR);
}

static void spr_wisp(void) {
    cv_begin(cv_sprite, TS, TS);
    for (int y = 0; y < TS; y++)
        for (int x = 0; x < TS; x++) {
            float d = hypotf(x + 0.5f - 32, y + 0.5f - 32);
            if (d < 4.5f) cv_px(x, y, (uint8_t)(R_GLOW + clampi((int)(15 - d * 2.2f), 4, 15)));
        }
    for (int i = 0; i < 8; i++) cv_px(32 + (int)((h01(i, 1, 9) - 0.5f) * 14), 32 + (int)((h01(i, 2, 9) - 0.5f) * 14), (uint8_t)(R_GLOW + 9));
    cv_store(SP_WISP);
}

static void spr_spark(void) {
    cv_begin(cv_sprite, TS, TS);
    for (int k = -5; k <= 5; k++) { cv_px(32 + k, 32, (uint8_t)(R_FLAME + 15 - abs(k))); cv_px(32, 32 + k, (uint8_t)(R_FLAME + 15 - abs(k))); }
    for (int k = -3; k <= 3; k++) { cv_px(32 + k, 32 + k, (uint8_t)(R_FLAME + 12 - abs(k))); cv_px(32 + k, 32 - k, (uint8_t)(R_FLAME + 12 - abs(k))); }
    cv_store(SP_SPARK);
}

/* the Blight Flask (docs/PLAN-b11.md, section 10): a squat stoppered flask, green venom behind the glass, turning
 * as it flies */
static void spr_flask(void) {
    cv_begin(cv_sprite, TS, TS);
    for (int y = 0; y < TS; y++)
        for (int x = 0; x < TS; x++) {
            float dx = (x + 0.5f - 32) / 6.5f, dy = (y + 0.5f - 34) / 6.0f, d = dx * dx + dy * dy;
            if (d < 1.0f) {                                         /* the belly: glass over the venom */
                float l = lit(dx, dy, sqrtf(1.0f - d));
                uint8_t c = dy > -0.35f ? sh(R_MOSS, 0.35f + 0.55f * l) : sh(R_GHOST, 0.25f + 0.4f * l);
                if (dx < -0.3f && dy < -0.1f && d > 0.35f && d < 0.6f) c = (uint8_t)(R_GHOST + 14);   /* a gleam */
                if (dy > 0.1f && h01(x, y, 5) < 0.08f) c = (uint8_t)(R_MOSS + 14);                     /* a bubble */
                cv_px(x, y, c);
            } else if (d < 1.25f) cv_px(x, y, sh(R_GHOST, 0.2f));
        }
    for (int y = 23; y < 28; y++) for (int x = 30; x < 35; x++) cv_px(x, y, sh(R_GHOST, 0.3f + 0.1f * (x - 30)));   /* the neck */
    for (int y = 20; y < 24; y++) for (int x = 29; x < 36; x++) cv_px(x, y, sh(R_WOOD, 0.35f + 0.08f * (x - 29)));   /* the stopper */
    cv_store(SP_FLASK);
}

/* the ultimates (docs/PLAN-b11.md, sections 9-10) */
static void spr_orb(int id, int frame) {                           /* Amperage: a ball of lightning, threads crackling off it */
    cv_begin(cv_sprite, TS, TS);
    for (int y = 0; y < TS; y++)
        for (int x = 0; x < TS; x++) {
            float dx = x + 0.5f - 32, dy = y + 0.5f - 32, d = sqrtf(dx * dx + dy * dy);
            float n = fbm(x / 2.5f + frame * 7.0f, y / 2.5f, 32, 83) * 0.35f, rr = 9.0f * (0.9f + n);
            if (d > rr) continue;
            if (d < 3.2f) { cv_px(x, y, C_WHITE); continue; }
            float sw = sinf(atan2f(dy, dx) * 3.0f + d * 0.7f + frame * 2.0f + n * 9.0f);   /* swirling */
            cv_px(x, y, sh(R_GOLD, clampf(1.0f - d / rr * 0.55f + 0.2f * sw, 0.35f, 1.0f)));
        }
    for (int k = 0; k < 6; k++) {                                  /* the threads, jagged */
        float a = h01(k, frame, 41) * TAU_F, x = 32 + cosf(a) * 7.0f, y = 32 + sinf(a) * 7.0f, l = 9.0f + 10.0f * h01(k, frame, 43);
        for (int s = 0; s < 5; s++) {
            float b = a + (h01(k * 5 + s, frame, 47) - 0.5f) * 1.4f, nx = x + cosf(b) * l / 5.0f, ny = y + sinf(b) * l / 5.0f;
            cv_line(x, y, nx, ny, s < 2 ? C_WHITE : sh(R_GOLD, 0.9f - 0.1f * s));
            x = nx; y = ny;
        }
    }
    cv_store(id);
}
static void spr_lance(void) {                                      /* Winter's Lance: a long star of ice */
    cv_begin(cv_sprite, TS, TS);
    for (int k = 0; k < 6; k++) {
        float a = k * TAU_F / 6.0f + 0.26f, l = k & 1 ? 11.0f : 17.0f;
        float tri[6] = { 32 + cosf(a) * l, 32 + sinf(a) * l, 32 + cosf(a + 0.42f) * 3.5f, 32 + sinf(a + 0.42f) * 3.5f,
                         32 + cosf(a - 0.42f) * 3.5f, 32 + sinf(a - 0.42f) * 3.5f };
        cv_poly(tri, 3, R_GLOW, 0.45f, 1.0f, 32 - l, 2 * l);
    }
    cv_flat(32, 32, 4.0f, 4.0f, sh(R_GLOW, 0.95f));
    cv_flat(32, 32, 2.2f, 2.2f, C_WHITE);
    for (int i = 0; i < 14; i++) {                                 /* frost streaming off it */
        float a = h01(i, 1, 51) * TAU_F, l = 6.0f + 16.0f * h01(i, 2, 51);
        cv_px((int)(32 + cosf(a) * l), (int)(32 + sinf(a) * l), i & 1 ? C_WHITE : sh(R_GLOW, 0.7f));
    }
    cv_store(SP_LANCE);
}
static void spr_ballista(void) {                                   /* the Ballista's spear, point first, its head bound in iron */
    cv_begin(cv_sprite, TS, TS);
    for (int y = 0; y < TS; y++)
        for (int x = 0; x < TS; x++) {
            float dx = (x + 0.5f - 32) / 7.5f, dy = (y + 0.5f - 32) / 7.5f, d = dx * dx + dy * dy;
            if (d < 0.55f) cv_px(x, y, sh(R_BONE, 0.5f + 0.5f * lit(dx, dy, sqrtf(1.0f - d))));
            else if (d < 1.0f) cv_px(x, y, sh(R_IRON, 0.25f + 0.6f * lit(dx, dy, sqrtf(1.0f - d)) + 0.1f * ((x + y) & 1)));   /* the binding */
            else if (d < 2.4f && h01(x, y, 5) < 0.4f) cv_px(x, y, sh(R_BONE, 0.3f));   /* the wind of it */
        }
    for (int k = 0; k < 4; k++) cv_px(32 + (k & 1 ? 5 : -5), 32 + (k & 2 ? 5 : -5), sh(R_IRON, 0.9f));   /* rivets */
    cv_flat(31.5f, 31.5f, 1.5f, 1.5f, sh(R_BONE, 1.0f));
    cv_store(SP_BALLISTA);
}
static void spr_fspirit(int id, int frame) {                       /* Firewall's flame spirits */
    cv_begin(cv_sprite, TS, TS);
    for (int y = 16; y < 50; y++) {
        float u = (float)(y - 16) / 34.0f, w = 9.0f * sinf(PI_F * fminf(1.0f, u * 1.15f)) * (0.5f + 0.5f * u);
        float wob = sinf(u * 9.0f + frame * 2.6f) * 2.0f * (1.0f - u);
        for (int x = (int)(32 + wob - w); x <= (int)(32 + wob + w); x++) {
            float e = fabsf(x + 0.5f - 32 - wob) / (w + 0.01f), heat = (1.0f - e) * 0.7f + u * 0.35f;
            if (u < 0.3f && h01(x, y + frame * 13, 9) < 0.3f * (1.0f - u / 0.3f)) continue;   /* its tip, licking */
            cv_px(x, y, (uint8_t)(R_FLAME + clampi((int)(heat * 15.0f), 3, 15)));
        }
    }
    cv_flat(29.0f, 36.0f, 1.4f, 2.2f, (uint8_t)(R_BLOOD + 1)); cv_flat(35.0f, 36.0f, 1.4f, 2.2f, (uint8_t)(R_BLOOD + 1));   /* hollow eyes */
    cv_store(id);
}
static void spr_rime(void) {                                       /* the Lance's rime: frost feathering out on the floor */
    cv_begin(cv_sprite, TS, TS);
    for (int y = 54; y <= 63; y++)
        for (int x = 6; x < 58; x++) {
            float nx = (x + 0.5f - 32) / 25.0f, ny = (y + 0.5f - 59.0f) / 4.5f, d = sqrtf(nx * nx + ny * ny);
            if (d > 1.0f) continue;
            float f = fbm(x / 3.0f, y / 1.5f, 32, 97);
            if (f > 0.42f + 0.25f * d) cv_px(x, y, f > 0.62f ? C_WHITE : sh(R_GLOW, 0.35f + 0.5f * f));
            else if (h01(x, y, 99) < 0.12f * (1.0f - d)) cv_px(x, y, sh(R_GLOW, 0.8f));
        }
    cv_store(SP_RIME);
}

static void spr_mote(void) {                                       /* a soul where the dead fell (section 12) */
    cv_begin(cv_sprite, TS, TS);
    for (int y = 18; y < 44; y++) {
        float u = (float)(y - 18) / 26.0f, w = 6.0f * sinf(PI_F * fminf(1.0f, 0.15f + u * 0.95f)) * (0.35f + 0.65f * u);
        for (int x = (int)(32 - w); x <= (int)(32 + w); x++) {
            float e = fabsf(x + 0.5f - 32) / (w + 0.01f);
            if (u < 0.35f && h01(x, y, 61) < 0.25f) continue;      /* its tip, wavering */
            cv_px(x, y, (uint8_t)(R_GHOST + clampi((int)((1.0f - e * 0.7f) * 13.0f + u * 3.0f), 4, 15)));
        }
    }
    cv_px(30, 34, (uint8_t)(R_GHOST + 3)); cv_px(34, 34, (uint8_t)(R_GHOST + 3));   /* two dim eyes */
    cv_px(31, 37, (uint8_t)(R_GHOST + 5)); cv_px(32, 37, (uint8_t)(R_GHOST + 5)); cv_px(33, 37, (uint8_t)(R_GHOST + 5));   /* a mouth, open */
    for (int i = 0; i < 6; i++) cv_px((int)(32 + (h01(i, 3, 71) - 0.5f) * 18), (int)(26 + h01(i, 4, 71) * 20), (uint8_t)(R_GHOST + 12));
    cv_store(SP_MOTE);
}

static void spr_burn(int id, int frame) {
    cv_begin(cv_sprite, TS, TS);
    for (int i = 0; i < 9; i++) {
        float cx = 8 + i * 6 + (h01(i, frame, 3) - 0.5f) * 3, h = 8 + 10 * h01(i, frame + 7, 3);
        for (int y = (int)(64 - h); y < 64; y++)
            for (int x = (int)(cx - 3); x <= (int)(cx + 3); x++) {
                float u = (y - (64 - h)) / h, w = 3.0f * u;
                if (fabsf(x + 0.5f - cx) > w) continue;
                cv_px(x, y, (uint8_t)(R_FLAME + clampi((int)(4 + u * 11), 2, 15)));
            }
    }
    cv_store(id);
}



/* the merchant's pedestals and wares; anything in R_PURPLE takes the colour of what is on offer */
static void spr_pedestal(void) {
    cv_begin(cv_sprite, TS, TS);
    for (int y = 30; y < 64; y++)
        for (int x = 14; x < 50; x++) {
            int cap = y < 36 || y >= 60;
            float hw = cap ? 17.5f : 13.0f - (y < 40 ? 0.0f : 0.0f);
            if (fabsf(x + 0.5f - 32) > hw) continue;
            float u = (x + 0.5f - 32) / hw;
            float v = 0.25f + 0.5f * lit(u, cap ? -0.6f : 0.0f, sqrtf(fmaxf(0.0f, 1 - u * u))) + (y == 30 ? 0.2f : 0.0f) - (y == 35 || y == 59 ? 0.12f : 0.0f);
            if (!cap && (x - 14) % 7 == 0) v -= 0.08f;
            cv_px(x, y, sh(R_STONE, v));
        }
    cv_outline(0);
    cv_store(SP_PEDESTAL);
}

static void spr_items(void) {
    /* a spellbook, open, pages glowing */
    cv_begin(cv_sprite, TS, TS);
    for (int s = -1; s <= 1; s += 2) {
        float cover[8] = { 32, 42, 32 + s * 22, 38, 32 + s * 22, 24, 32, 28 };
        cv_poly(cover, 4, R_PURPLE, 0.75f, 0.35f, 24, 18);
        float page[8] = { 32, 40, 32 + s * 19, 36, 32 + s * 19, 24, 32, 27 };
        cv_poly(page, 4, R_BONE, 0.95f, 0.7f, 24, 16);
        for (int l = 0; l < 4; l++) cv_line(32 + s * 4, 30 + l * 2.5f, 32 + s * 16, 28 + l * 2.5f, sh(R_PURPLE, 0.6f));
    }
    cv_line(32, 27, 32, 41, sh(R_PURPLE, 0.2f));
    cv_outline(0);
    for (int i = 0; i < 10; i++) cv_px(20 + (int)(h01(i, 1, 21) * 24), 10 + (int)(h01(i, 2, 21) * 14), sh(R_PURPLE, 1.0f));
    cv_store(SP_ITEM_TOME);
    /* a rune stone */
    cv_begin(cv_sprite, TS, TS);
    for (int y = 14; y < 46; y++)
        for (int x = 18; x < 46; x++) {
            float u = (x + 0.5f - 32) / 14.0f, v = (y + 0.5f - 30) / 16.0f;
            if (u * u * u * u + v * v * v * v > 1.0f) continue;
            cv_px(x, y, sh(R_STONE, 0.3f + 0.4f * lit(u, v, 0.6f) + fbm(x / 4.0f, y / 4.0f, 16, 23) * 0.15f));
        }
    static const char *RUNE[9] = { "..#..", ".###.", "#.#.#", "..#..", ".#.#.", "#...#", "..#..", "..#..", "..#.." };
    for (int y = 0; y < 9; y++) for (int x = 0; x < 5; x++) if (RUNE[y][x] == '#') { cv_px(27 + x * 2, 21 + y * 2, sh(R_PURPLE, 1.0f)); cv_px(28 + x * 2, 21 + y * 2, sh(R_PURPLE, 0.85f)); cv_px(27 + x * 2, 22 + y * 2, sh(R_PURPLE, 0.85f)); }
    cv_outline(0);
    cv_store(SP_ITEM_RUNE);
    /* a robe, folded */
    cv_begin(cv_sprite, TS, TS);
    for (int y = 26; y < 46; y++)
        for (int x = 12; x < 52; x++) {
            float u = (x + 0.5f - 32) / 20.0f, v = (y - 26) / 20.0f;
            if (fabsf(u) > 1.0f - v * 0.1f) continue;
            float f = 0.35f + 0.35f * (1.0f - v) + 0.12f * sinf(y * 1.2f) - (((y - 26) % 6) == 0 ? 0.15f : 0.0f);
            cv_px(x, y, sh(R_PURPLE, f));
        }
    for (int x = 13; x < 51; x++) cv_px(x, 33, sh(R_GOLD, 0.8f));
    cv_outline(0);
    cv_store(SP_ITEM_ROBE);
    /* a hood, standing up as if worn by no one */
    cv_begin(cv_sprite, TS, TS);
    for (int y = 10; y < 48; y++)
        for (int x = 12; x < 52; x++) {
            float u = (x + 0.5f - 32) / (10.0f + (y - 10) * 0.28f), v = (y - 10) / 38.0f;
            if (fabsf(u) > 1.0f || (y < 16 && fabsf(u) > (y - 9) / 7.0f)) continue;
            float hole = hypotf((x + 0.5f - 32) / 7.0f, (y + 0.5f - 28) / 10.0f);
            cv_px(x, y, hole < 1.0f ? 0 : sh(R_PURPLE, 0.2f + 0.5f * lit(u, v - 0.5f, sqrtf(fmaxf(0.0f, 1 - u * u)))));
        }
    cv_outline(0);
    cv_store(SP_ITEM_HOOD);
    /* a ring */
    cv_begin(cv_sprite, TS, TS);
    for (int a = 0; a < 90; a++) {
        float t = a / 90.0f * TAU_F;
        for (float w = -1.6f; w <= 1.6f; w += 0.5f) cv_px((int)(32 + cosf(t) * (9 + w)), (int)(34 + sinf(t) * (9 + w) * 0.8f), sh(R_GOLD, 0.45f + 0.5f * cosf(t + 2.3f)));
    }
    gem(32, 25, 4.5f, R_PURPLE);
    cv_outline(0);
    cv_px(30, 22, C_WHITE);
    cv_store(SP_ITEM_RING);
    /* a bracer, lying on its side: a cuff of cloth and leather, two straps, studs */
    cv_begin(cv_sprite, TS, TS);
    for (int y = 20; y < 46; y++)
        for (int x = 18; x < 47; x++) {
            float hh = 7.0f + 2.0f * (x - 18) / 28.0f, v = (y + 0.5f - 33) / hh;
            if (fabsf(v) > 1.0f) continue;
            float f = 0.2f + 0.65f * lit(0.0f, v, sqrtf(1.0f - v * v)) - ((x - 18) % 5 == 0 ? 0.06f : 0.0f);
            int strap = (x >= 24 && x <= 26) || (x >= 37 && x <= 39);
            cv_px(x, y, strap ? sh(R_WOOD, f * 0.8f) : sh(R_PURPLE, f));
        }
    for (int y = 25; y < 42; y++) cv_px(46, y, sh(R_IRON, 0.08f));   /* the open end */
    cv_px(25, 31, sh(R_GOLD, 0.95f)); cv_px(38, 30, sh(R_GOLD, 0.95f));
    cv_outline(0);
    cv_store(SP_ITEM_BRACER);
    /* boots, a pair, side by side */
    cv_begin(cv_sprite, TS, TS);
    for (int b = 0; b < 2; b++) {
        float dx = b ? 0.0f : -7.0f, dy = b ? 0.0f : -2.0f, lo = b ? 0.75f : 0.5f, hi = b ? 0.3f : 0.15f;
        float boot[14] = { 21 + dx, 16 + dy, 31 + dx, 16 + dy, 31 + dx, 36 + dy, 42 + dx, 38 + dy, 44 + dx, 42 + dy, 44 + dx, 46 + dy, 21 + dx, 46 + dy };
        cv_poly(boot, 7, R_PURPLE, lo, hi, 16 + dy, 30);
        cv_line(21 + dx, 46 + dy, 44 + dx, 46 + dy, sh(R_IRON, 0.25f));
        cv_line(21 + dx, 19 + dy, 31 + dx, 19 + dy, sh(R_PURPLE, b ? 0.95f : 0.7f));
        cv_line(24 + dx, 46 + dy, 24 + dx, 41 + dy, sh(R_IRON, 0.3f));
    }
    cv_outline(0);
    cv_store(SP_ITEM_BOOTS);
    /* an amulet: a chain, a gold rim, a stone in the colour of what it is */
    cv_begin(cv_sprite, TS, TS);
    for (int i = 0; i <= 12; i++) {
        float t = i / 12.0f;
        cv_px((int)(20 + 10 * t), (int)(12 + 16 * t * (2 - t)), sh(R_IRON, 0.7f - 0.3f * (i & 1)));
        cv_px((int)(44 - 10 * t), (int)(12 + 16 * t * (2 - t)), sh(R_IRON, 0.7f - 0.3f * (i & 1)));
    }
    cv_ellipse(32, 37, 8.5f, 9.5f, R_GOLD, 0.35f, 0.95f);
    gem(32, 37, 6.0f, R_PURPLE);
    cv_outline(0);
    cv_px(30, 33, C_WHITE);
    cv_store(SP_ITEM_AMULET);
    /* a bag, tied at the neck */
    cv_begin(cv_sprite, TS, TS);
    cv_ellipse(32, 38, 13.5f, 10.5f, R_PURPLE, 0.15f, 0.85f);
    float neck[8] = { 28, 29, 36, 29, 40, 20, 24, 20 };
    cv_poly(neck, 4, R_PURPLE, 0.75f, 0.35f, 20, 10);
    for (int x = 27; x <= 37; x++) { cv_px(x, 28, sh(R_WOOD, 0.6f)); cv_px(x, 29, sh(R_WOOD, 0.35f)); }
    cv_line(37, 29, 41, 34, sh(R_WOOD, 0.5f));
    for (int k = 0; k < 4; k++) cv_line(26 + k * 4, 20, 25 + k * 4.4f, 17, sh(R_PURPLE, 0.6f));   /* the gathered cloth */
    cv_outline(0);
    cv_store(SP_ITEM_BAG);
}

/* The Black Altar's fire, drawn apart from its stone (spr_altarstone, with each level) and at full brightness: violet or
 * gold, two frames each, standing in its bowl */
static void spr_altarfire(void) {
    for (int c = 0; c < 2; c++)                                     /* the fire: violet, then gold */
        for (int fr = 0; fr < 2; fr++) {
            cv_begin(cv_sprite, TS, TS);
            int ramp = c ? R_GOLD : R_PURPLE;
            for (int y = 13; y < 32; y++)
                for (int x = 24; x < 41; x++) {
                    float h = (32 - y) / 19.0f;                     /* 0 at the bowl, 1 at the tip */
                    float sway = 1.6f * sinf(h * 6.0f + fr * 2.6f) * h;
                    float w = (6.2f - 5.4f * h) * (0.8f + 0.2f * sinf(y * 1.7f + fr * 3.0f));
                    float dx = fabsf(x + 0.5f - 32 - sway);
                    if (dx > w || (dx > w * 0.55f && h01(x, y + fr * 7, 47 + c) < 0.3f)) continue;   /* ragged at the edges */
                    float core = 1.0f - dx / w;
                    if (core > 0.65f && h < 0.45f) { cv_px(x, y, C_WHITE); continue; }
                    cv_px(x, y, sh(ramp, fminf(1.0f, 0.3f + 0.65f * core * (1.0f - 0.4f * h))));
                }
            cv_store(c ? SP_BFLAME_G0 + fr : SP_BFLAME_V0 + fr);
        }
}

const char *const RUNE_GLYPH[9][7] = {                             /* by DF_*: ward, blink, mirror, mend, purify, nova, dread, slip, reap */
    { "#.#.#", "#.#.#", ".###.", "..#..", "..#..", "..#..", "..#.." },   /* algiz: the ward */
    { "##...", "#.#..", "#..#.", "#.#..", "##...", "#.#..", "#..#." },   /* raido: the journey */
    { "#...#", "##.##", "#.#.#", "#...#", "#...#", "#...#", "#...#" },   /* mannaz: the mirror */
    { "##...", "#.#..", "#.#..", "##...", "#.#..", "#.#..", "##..." },   /* berkano: healing */
    { "#....", "##...", "#.#..", "#....", "#....", "#....", "#...." },   /* laguz: water, cleansing */
    { "#.#.#", ".###.", "#####", ".###.", "#.#.#", "..#..", "..#.." },   /* frost, a star */
    { "#....", "##...", "#.#..", "#..#.", "#.#..", "##...", "#...." },   /* thurisaz: the thorn, dread */
    { "#...#", "##.##", "#.#.#", "#...#", "#.#.#", "##.##", "#...#" },   /* dagaz: the turning of time */
    { "###..", "#.#..", "#..#.", "#.#..", "###..", "#.#..", "#..#." },   /* perthro: lots, fate, souls */
};

/* The elements' glyphs (docs/PLAN-b11.md, section 2), 7 by 7, by EL_*: '#' in the element's colour, 'o' its
 * bright core. On the target's readout, the bestiary, the Alignment and the status bar. */
const char *const ELEM_GLYPH[6][7] = {
    { "...#...", "..##...", ".###.#.", ".##o##.", "##ooo##", "##ooo##", ".#####." },   /* a flame */
    { "...#...", "#..#..#", ".#.#.#.", "..#o#..", ".#.#.#.", "#..#..#", "...#..." },   /* a six-armed flake */
    { "....###", "...###.", "..###..", ".######", "...##..", "..#.#..", ".#...#." },   /* a forked bolt */
    { "...#...", "...#...", "..###..", ".##o##.", "##o.o##", "###o###", ".#####." },   /* a drop with a bubble in it */
    { "...#...", "...##..", "..###..", ".#####.", ".#o#o#.", ".#ooo#.", "..###.." },   /* a wisp's teardrop flame, looking out */
    { ".......", "#.....#", "##...##", ".#####.", "##...##", "#.....#", "......." },   /* a bone */
};
/* The runes' marks for their sockets (section 3), 11 by 11, by DF_*: the same two tones */
const char *const RUNE_MARK[9][11] = {
    { "##.......##", "###.....###", ".###...###.", "...##.##...", "....###....", ".....#.....",
      "....###....", "...##.##...", ".###...###.", "###.....###", "##.......##" },   /* crossed bones */
    { "...........", "...#####...", ".##.....##.", "#...ooo...#", ".##.ooo.##.", "...#####...",
      "...........", "........#..", ".#########.", "........#..", "..........." },   /* an eye, and a step away */
    { "..####.....", ".#oooo#....", "#o#oooo#...", "#oo#oooo#..", "#ooooooo#..", ".#ooooo#...",
      "..#####....", "......#....", ".......#...", "........##.", ".........##" },   /* a hand mirror */
    { "...........", ".###...###.", "#####.#####", "####ooo####", "#####.#####", ".###ooo###.",
      "..###.###..", "...#ooo#...", "....#.#....", ".....#.....", "..........." },   /* a stitched heart */
    { "...#####...", "..#.....#..", ".#...#...#.", "#...###...#", "#..#####..#", "#..##o##..#",
      "#..#####..#", "#...###...#", ".#.......#.", "..#.....#..", "...#####..." },   /* a drop in a ring */
    { ".....#.....", ".#...#...#.", "..#..#..#..", "...#.#.#...", "....ooo....", "#####o#####",
      "....ooo....", "...#.#.#...", "..#..#..#..", ".#...#...#.", ".....#....." },   /* a burst */
    { "...#####...", "..#######..", ".#########.", ".##..#..##.", ".##..#..##.", ".#########.",
      "..###.###..", "...#####...", "...#.#.#...", "...#####...", "..........." },   /* a skull */
    { "###########", ".#ooooooo#.", "..#ooooo#..", "...#ooo#...", "....#o#....", ".....#.....",
      "....#.#....", "...#...#...", "..#..o..#..", ".#.ooooo.#.", "###########" },   /* an hourglass */
    { "..######...", ".#.....###.", ".........##", "..........#", ".........#.", "........#..",
      ".......#...", "......#....", ".....#.....", "....#......", "...#......." },   /* a scythe */
};

/* The classes' marks (docs/PLAN-b11.md, section 11), 11 by 11, by the class's element: the same two tones */
const char *const CLASS_MARK[6][11] = {
    { ".....#.....", "....###....", ".....#.....", "..#######..", ".....#.....", ".....#.....",
      ".#...#...#.", ".##.o#o.##.", ".#oo###oo#.", "##ooo#ooo##", "###########" },   /* the Witchfinder: a stake, a pyre */
    { "..........#", ".........##", "........##.", ".......##..", "......##...", ".....##....",
      "....##.....", "...#o#.....", "..#oo#.....", ".#oo#......", ".###......." },   /* the Anatomist: a scalpel */
    { ".#.......#.", ".#.......#.", ".#.......#.", ".#...#...#.", ".#..##...#.", ".#.#oo#..#.",
      ".#..#oo#.#.", ".#...##..#.", ".#...#...#.", "###.....###", "###.....###" },   /* the Galvanist: two rods, a spark */
    { "...#####...", "...#####...", ".#########.", "...........", "..####.....", ".#o##o#....",
      ".#######...", "..#######..", "....######.", "......####.", "........##." },   /* the Plague Doctor: hat, lenses, the beak */
    { "....ooo....", "...o#o#o...", "....ooo....", "...........", "..#######..", ".##.#.#.##.",
      "###.#.#.###", "###########", "###########", ".#########.", "..........." },   /* the Sin-Eater: a coin on scored bread */
    { ".####......", "#####.#....", "######o#...", "#######o#..", "######o#...", "#####.#....",
      ".####.#....", "......#....", "......#....", "......#....", "......#...." },   /* the Headsman: the axe */
};

/* The Rune Shrine: a stele with an arched head on two steps, a disc cut in its face. Sanctified, the
 * stone is pale and clean, the disc gilded with a four-pointed star, a white cloth with a gold hem laid
 * over the step and two candles cold. Cursed, the same stone gone dark with soot and cracked, the disc
 * broken and the star turned over, the cloth burnt to rags, the candles guttered black; what glows
 * (the cut channels, a column of small marks down each side, the flames) is drawn apart at full
 * brightness, in R_PURPLE that takes the shrine's rank's colour. */
static int shrine_stele(int x, int y) {                             /* inside the stele's outline? */
    if (x < 21 || x > 42 || y > 50) return 0;
    if (y >= 22) return y >= 12;
    float dx = x + 0.5f - 32.0f, dy = y + 0.5f - 22.0f;
    return dx * dx + dy * dy <= 11.0f * 11.0f;
}
static void spr_shrine_body(int cursed) {
    cv_begin(cv_sprite, TS, TS);
    for (int y = 0; y < TS; y++)                                    /* the stele */
        for (int x = 0; x < TS; x++) {
            if (!shrine_stele(x, y)) continue;
            float u = (x + 0.5f - 32.0f) / 11.0f;
            float v = 0.38f + 0.3f * lit(u, -0.3f, sqrtf(fmaxf(0.0f, 1.0f - u * u))) + 0.06f * fbm(x / 4.0f, y / 4.0f, 8, 501);
            if (cursed) {
                v *= 0.55f;
                if (fbm(x / 3.0f, y / 6.0f, 8, 503) > 0.58f) v *= 0.6f;   /* soot running down */
            }
            cv_px(x, y, sh(R_STONE, v));
        }
    for (int y = 50; y < 64; y++)                                   /* two steps */
        for (int x = 8; x < 56; x++) {
            int top = y < 55;
            if (top && (x < 14 || x > 49)) continue;
            float v = (y == 50 || y == 55 ? 0.62f : 0.42f) + 0.05f * fbm(x / 3.0f, y / 2.0f, 8, 507);
            cv_px(x, y, sh(R_STONE, cursed ? v * 0.5f : v));
        }
    if (!cursed) {
        for (int y = 0; y < TS; y++)                                /* the disc, gilded, and its star */
            for (int x = 0; x < TS; x++) {
                float dx = x + 0.5f - 32.0f, dy = y + 0.5f - 27.0f, r = sqrtf(dx * dx + dy * dy);
                if (r > 6.2f && r < 7.6f) cv_px(x, y, sh(R_GOLD, 0.55f + 0.35f * (dy < 0)));
                float ax = fabsf(dx), ay = fabsf(dy);
                if ((ax < 1.0f && ay < 6.0f) || (ay < 1.0f && ax < 6.0f) || (ax + ay < 2.6f)) cv_px(x, y, sh(R_GOLD, 0.7f + 0.25f * (dy < 0)));
            }
        for (int x = 15; x <= 48; x++) {                            /* the cloth, falling over the step to a point */
            int drop = 6 - abs(x - 32) / 4;
            for (int y = 49; y <= 52 + drop; y++) cv_px(x, y, sh(R_BONE, y == 49 ? 0.95f : 0.8f - 0.05f * (y - 49) + 0.08f * ((x & 3) == 0)));
            cv_px(x, 53 + drop, sh(R_GOLD, 0.7f));                  /* its hem */
        }
        for (int s = -1; s <= 1; s += 2) {                          /* two candles, cold */
            int cx = 32 + s * 15;
            for (int y = 42; y < 50; y++) { cv_px(cx, y, sh(R_BONE, 0.85f)); cv_px(cx + 1, y, sh(R_BONE, 0.7f)); }
            cv_px(cx, 41, 0);
        }
    } else {
        for (int y = 0; y < TS; y++)                                /* the disc, broken: its cut channels dark */
            for (int x = 0; x < TS; x++) {
                float dx = x + 0.5f - 32.0f, dy = y + 0.5f - 27.0f, r = sqrtf(dx * dx + dy * dy);
                float a = atan2f(dy, dx);
                if (r > 6.2f && r < 7.6f && !(a > 0.9f && a < 1.5f)) cv_px(x, y, sh(R_STONE, 0.05f));
            }
        static const float CRACK[][4] = { { 26, 14, 23, 30 }, { 23, 30, 25, 44 }, { 38, 33, 41, 47 }, { 35, 15, 37, 21 } };
        for (int i = 0; i < 4; i++) cv_line(CRACK[i][0], CRACK[i][1], CRACK[i][2], CRACK[i][3], sh(R_STONE, 0.02f));
        for (int x = 16; x <= 47; x++) {                            /* what is left of the cloth, burnt */
            if (h01(x, 0, 509) < 0.35f) continue;
            int drop = (int)(4 * h01(x, 1, 509));
            for (int y = 49; y <= 50 + drop; y++) cv_px(x, y, sh(y == 50 + drop ? R_WARM : R_BONE, y == 50 + drop ? 0.08f : 0.35f));
        }
        for (int s = -1; s <= 1; s += 2) {                          /* the candles, burnt down black */
            int cx = 32 + s * 15;
            for (int y = 45; y < 50; y++) { cv_px(cx, y, sh(R_IRON, 0.2f)); cv_px(cx + 1, y, sh(R_IRON, 0.12f)); }
            cv_px(cx - 1, 49, sh(R_IRON, 0.15f)); cv_px(cx + 2, 49, sh(R_IRON, 0.1f));   /* the wax run down */
        }
        for (int y = 33; y < 49; y++) if (h01(31, y, 511) < 0.8f) cv_px(31 + (y > 40), y, sh(R_BLOOD, 0.3f));   /* a dark run from the disc */
    }
    cv_outline(0);
    cv_store(cursed ? SP_SHRINE_C : SP_SHRINE_S);
}
static void spr_shrine_glow(int fr) {
    cv_begin(cv_sprite, TS, TS);
    for (int y = 0; y < TS; y++)                                    /* the broken disc and the star turned over */
        for (int x = 0; x < TS; x++) {
            float dx = x + 0.5f - 32.0f, dy = y + 0.5f - 27.0f, r = sqrtf(dx * dx + dy * dy), a = atan2f(dy, dx);
            float ax = fabsf(dx);
            if (r > 6.4f && r < 7.4f && !(a > 0.9f && a < 1.5f)) cv_px(x, y, sh(R_PURPLE, 0.75f + 0.2f * fr));
            if ((ax < 0.9f && dy > -5.5f && dy < 5.5f) || (fabsf(dy - 2.5f) < 0.9f && ax < 3.6f))   /* the cross, its bar low */
                cv_px(x, y, sh(R_PURPLE, 0.95f));
        }
    for (int s = -1; s <= 1; s += 2)                                /* a column of small marks down each side */
        for (int k = 0; k < 5; k++) {
            int gx = 32 + s * 7 - 1, gy = 37 + k * 3 - 15 * (k > 4);
            if (gy > 48) break;
            cv_px(gx, gy, sh(R_PURPLE, 0.7f + 0.3f * ((k + fr) & 1))); cv_px(gx + (int)(h01(k, s + 2, 513) * 2.0f), gy + 1, sh(R_PURPLE, 0.6f));
        }
    for (int s = -1; s <= 1; s += 2) {                              /* the candle flames */
        int cx = 32 + s * 15;
        float sway = (fr ? 0.6f : -0.5f) * s;
        for (int y = 39; y < 45; y++) {
            float h = (45 - y) / 6.0f, w = 1.6f * (1.0f - h) + 0.3f;
            for (int x = cx - 2; x <= cx + 2; x++) {
                float d = fabsf(x + 0.5f - cx - 0.5f - sway * h);
                if (d > w) continue;
                cv_px(x, y, d < w * 0.45f && h < 0.6f ? C_WHITE : sh(R_FLAME, 0.55f + 0.4f * (1.0f - h)));
            }
        }
    }
    cv_store(fr ? SP_SHRINE_G1 : SP_SHRINE_G0);
}
/* a rune about the shrine: its mark, big in the frame (drawn small), in R_PURPLE with a soft rim;
 * k = NDEF for the plain mark it shows before it is attuned to any */
static void spr_srune(int k) {
    static const char *const PLAIN[7] = { "..#..", ".###.", "#.#.#", "..#..", "#.#.#", ".###.", "..#.." };
    const char *const *g = k < 9 ? RUNE_GLYPH[k] : PLAIN;
    cv_begin(cv_sprite, TS, TS);
    for (int pass = 0; pass < 2; pass++)
        for (int y = 0; y < 7; y++)
            for (int x = 0; x < 5; x++) {
                if (g[y][x] != '#') continue;
                int x0 = 17 + x * 6, y0 = 11 + y * 6;
                for (int yy = -1 + pass; yy < 7 - pass; yy++)
                    for (int xx = -1 + pass; xx < 7 - pass; xx++)
                        cv_px(x0 + xx, y0 + yy, pass ? sh(R_PURPLE, 1.0f - 0.25f * (float)(y0 + yy - 11) / 42.0f) : sh(R_PURPLE, 0.35f));
            }
    cv_store(k < 9 ? SP_SRUNE0 + k : SP_SRUNE_X);
}
/* a Cursed Coin, standing on its edge: old gold gone dark, a raised rim, green in the worn places, a
 * skull struck on its face with one red eye */
static void spr_ccoin(void) {
    cv_begin(cv_sprite, TS, TS);
    cv_flat(32, 61.5f, 9.5f, 1.6f, sh(R_WARM, 0.05f));
    for (int y = 0; y < TS; y++)
        for (int x = 0; x < TS; x++) {
            float dx = (x + 0.5f - 32.0f) / 10.0f, dy = (y + 0.5f - 50.0f) / 10.0f, d = dx * dx + dy * dy;
            if (d > 1.0f) continue;
            float v;
            if (d > 0.74f) v = 0.5f + 0.4f * lit(dx, dy, sqrtf(1.0f - d));   /* the rim, catching the light */
            else if (d > 0.6f) v = 0.08f;                          /* the groove inside it */
            else v = 0.22f + 0.12f * (-dx - dy);                    /* the dark field */
            uint8_t c = sh(R_GOLD, v);
            if (d <= 0.6f && fbm(x / 1.5f, y / 1.5f, 8, 515) > 0.66f) c = sh(R_MOSS, 0.25f);   /* verdigris */
            cv_px(x, y, c);
        }
    cv_ellipse(32, 48.5f, 3.6f, 3.3f, R_BONE, 0.4f, 0.8f);         /* the skull */
    cv_flat(32, 52.2f, 2.0f, 1.2f, sh(R_BONE, 0.5f));
    cv_px(30, 48, 0); cv_px(31, 48, 0); cv_px(33, 48, (uint8_t)(R_BLOOD + 14)); cv_px(34, 48, (uint8_t)(R_BLOOD + 12));   /* one eye dark, one red */
    cv_px(32, 50, 0);
    for (int k = 0; k < 3; k++) cv_px(31 + k, 53, 0);              /* teeth */
    cv_outline(0);
    cv_store(SP_CCOIN);
}

/* a drain low in the wall, barred, a dark stain down from it: where a cistern's bile comes out */
static void gen_grate(int t, uint32_t seed) {
    gen_stone(t, R_STONE, seed);
    for (int y = 38; y < 58; y++)
        for (int x = 14; x < 50; x++) {
            int edge = y == 38 || x == 14 || x == 49 || y == 57;
            uint8_t c = edge ? sh(R_STONE, 0.72f) : sh(R_STONE, 0.02f + 0.03f * h01(x, y, seed));
            if (!edge && (x - 16) % 6 < 2) c = sh(R_IRON, (x - 16) % 6 ? 0.35f : 0.6f);   /* the bars */
            if (!edge && y == 47) c = sh(R_IRON, 0.5f);
            tput(t, x, y, c);
        }
    for (int y = 58; y < TS; y++)                                   /* the stain, down to the floor */
        for (int x = 20; x < 44; x++)
            if (fbm(x / 3.0f, y / 6.0f, 16, seed + 3) > 0.42f) tput(t, x, y, sh(R_MOSS, 0.12f + 0.1f * h01(x, y, seed)));
}

