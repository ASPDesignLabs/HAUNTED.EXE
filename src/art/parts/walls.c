/* A part of crawl_art.c (docs/PLAN-tidy.md): it is #included there in its place and never compiled on its own.
 * Wall textures: stone, rubble, brick, moss, the skull wall, niches, banners, torches, doors and locks, gates, the ice, the Portal, fonts, the sign, a secret's passage, the sealed doorway, the Repository. */

/* ------------------------------------------------------------------ */
/* Wall textures                                                       */
/* ------------------------------------------------------------------ */

static void tput(int t, int x, int y, uint8_t c) { crawl_tex[t][(x & 63) * TS + (y & 63)] = c; }

/* big dressed stone blocks, two per row, four rows */
static float block_shade(int x, int y, uint32_t seed) {
    int r = y / 16, yy = y % 16;
    int off = (int)(h01(r, 0, seed) * 32);
    int xx = (x + off) & 63;
    int bi = xx / 32, lx = xx % 32, bw = 32;
    if (h01(r, bi, seed + 3) > 0.55f) { bw = 16; bi = bi * 2 + lx / 16; lx %= 16; }
    float base = 0.40f + 0.22f * h01(r * 7 + bi, 9, seed);
    float n = fbm(x / 8.0f, y / 8.0f, 8, seed) * 0.26f - 0.13f;
    if (lx == 0 || yy == 0) return 0.08f + 0.05f * h01(x, y, seed + 9);           /* mortar */
    float bev = 0.0f;
    if (yy == 1) bev += 0.14f; else if (lx == 1) bev += 0.07f;
    if (yy == 15) bev -= 0.12f; else if (lx == bw - 1) bev -= 0.06f;
    if (h01(x, y, seed + 5) > 0.975f) bev -= 0.15f;                                 /* pits */
    return base + n + bev;
}

static void gen_stone(int t, int ramp, uint32_t seed) {
    for (int y = 0; y < TS; y++)
        for (int x = 0; x < TS; x++) tput(t, x, y, sh(ramp, block_shade(x, y, seed)));
}

static void gen_rubble(int t, uint32_t seed) {
    float px[18], py[18], pb[18];
    for (int i = 0; i < 18; i++) { px[i] = h01(i, 1, seed) * 64; py[i] = h01(i, 2, seed) * 64; pb[i] = 0.32f + 0.3f * h01(i, 3, seed); }
    for (int y = 0; y < TS; y++)
        for (int x = 0; x < TS; x++) {
            float d1 = 1e9f, d2 = 1e9f; int n1 = 0;
            for (int i = 0; i < 18; i++) {
                float dx = fabsf(x + 0.5f - px[i]), dy = fabsf(y + 0.5f - py[i]);
                if (dx > 32) dx = 64 - dx;
                if (dy > 32) dy = 64 - dy;
                float d = sqrtf(dx * dx + dy * dy);
                if (d < d1) { d2 = d1; d1 = d; n1 = i; } else if (d < d2) d2 = d;
            }
            float v;
            if (d2 - d1 < 1.4f) v = 0.06f;
            else {
                float dx = x + 0.5f - px[n1], dy = y + 0.5f - py[n1];
                if (dx > 32) dx -= 64; else if (dx < -32) dx += 64;
                if (dy > 32) dy -= 64; else if (dy < -32) dy += 64;
                v = pb[n1] + 0.010f * (-dx - dy) + fbm(x / 6.0f, y / 6.0f, 8, seed + 1) * 0.2f - 0.1f;
                if (d2 - d1 < 2.6f) v -= 0.08f;
            }
            tput(t, x, y, sh(R_STONE, v));
        }
}

static void gen_brick(int t, uint32_t seed) {
    for (int y = 0; y < TS; y++)
        for (int x = 0; x < TS; x++) {
            int r = y / 8, yy = y % 8;
            int xx = (x + (r & 1) * 8) & 63, bi = xx / 16, lx = xx % 16;
            if (yy == 0 || lx == 0) { tput(t, x, y, sh(R_STONE, 0.18f + 0.06f * h01(x, y, seed))); continue; }
            float v = 0.38f + 0.2f * h01(r, bi, seed) + fbm(x / 6.0f, y / 6.0f, 8, seed) * 0.2f - 0.1f;
            if (yy == 1) v += 0.1f;
            if (yy == 7) v -= 0.1f;
            if (lx == 15) v -= 0.05f;
            tput(t, x, y, sh(R_BRICK, v));
        }
}

static void gen_moss(int t, uint32_t seed) {
    for (int y = 0; y < TS; y++)
        for (int x = 0; x < TS; x++) {
            float base = block_shade(x, y, seed);
            float m = fbm(x / 10.0f, y / 10.0f, 8, seed + 40) + 0.25f * (y / 64.0f) + (h01(x, 0, seed + 41) > 0.82f ? 0.25f * (1.0f - y / 64.0f) : 0);
            if (m > 0.62f) tput(t, x, y, sh(R_MOSS, 0.22f + 0.6f * (m - 0.62f) * 2.5f + 0.1f * (base - 0.4f)));
            else tput(t, x, y, sh(R_STONE, base * 0.9f));
        }
}

/* a 15x15 skull at (ox, oy) into texture t */
static void tex_skull(int t, int ox, int oy, float bright, int glow) {
    for (int y = 0; y < 15; y++)
        for (int x = 0; x < 15; x++) {
            float dx = (x - 7.0f) / 6.8f, dy = (y - 6.0f) / 6.2f;
            int cran = dx * dx + dy * dy <= 1.0f && y < 11;
            int jaw = y >= 10 && y <= 14 && x >= 3 && x <= 11 && (y < 13 || (x >= 4 && x <= 10));
            if (!cran && !jaw) continue;
            float v = bright - 0.45f * (dx * dx + dy * dy) - 0.05f * (dx + dy);
            if (jaw) v = bright - 0.25f - (y - 10) * 0.06f;
            tput(t, ox + x, oy + y, sh(R_BONE, v));
        }
    for (int e = 0; e < 2; e++) {
        float ex = e ? 9.6f : 4.4f;
        for (int y = 4; y < 10; y++)
            for (int x = 1; x < 14; x++) {
                float dx = (x - ex) / 2.2f, dy = (y - 7.0f) / 2.4f;
                if (dx * dx + dy * dy <= 1.0f) tput(t, ox + x, oy + y, (glow && y == 7 && fabsf(x - ex) < 0.6f) ? R_FLAME + 9 : R_BONE);
            }
    }
    tput(t, ox + 7, oy + 10, R_BONE); tput(t, ox + 6, oy + 11, R_BONE + 1); tput(t, ox + 8, oy + 11, R_BONE + 1);
    for (int x = 4; x <= 10; x += 2) { tput(t, ox + x, oy + 12, R_BONE + 1); tput(t, ox + x, oy + 13, R_BONE + 1); }
}

static void gen_skulls(int t, uint32_t seed) {
    for (int y = 0; y < TS; y++)
        for (int x = 0; x < TS; x++) tput(t, x, y, sh(R_BONE, 0.04f + 0.10f * fbm(x / 4.0f, y / 4.0f, 16, seed)));
    for (int row = 0; row < 4; row++) {
        int by = row * 16 + 15;                                   /* a femur across each gap */
        for (int x = 0; x < TS; x++) {
            float v = 0.55f + 0.1f * sinf(x * 0.4f + row);
            tput(t, x, by, sh(R_BONE, v));
            if ((x + row * 5) % 21 == 0) { tput(t, x, by - 1, sh(R_BONE, 0.6f)); tput(t, x + 1, by - 1, sh(R_BONE, 0.5f)); }
        }
        for (int col = 0; col < 4; col++) {
            float b = 0.80f + 0.18f * h01(row, col, seed) - 0.1f;
            tex_skull(t, col * 16 + (row & 1) * 8, row * 16, b, h01(col, row, seed + 2) > 0.94f);
        }
    }
}

static void gen_niche(int t, uint32_t seed) {
    gen_stone(t, R_STONE, seed);
    for (int ny = 0; ny < 2; ny++)
        for (int nx = 0; nx < 2; nx++) {
            int x0 = nx * 32 + 4, y0 = ny * 32 + 4, w = 24, hh = 24;
            float cx = x0 + w / 2.0f;
            for (int y = y0 - 1; y <= y0 + hh; y++)
                for (int x = x0 - 1; x <= x0 + w; x++) {
                    int inside;
                    if (y < y0 + 8) { float dx = (x + 0.5f - cx) / (w / 2.0f), dy = (y + 0.5f - (y0 + 8)) / 8.0f; inside = dx * dx + dy * dy <= 1.0f; }
                    else inside = x >= x0 && x < x0 + w && y < y0 + hh;
                    if (!inside) continue;
                    float depth = (y - y0) / (float)hh;
                    tput(t, x, y, sh(R_STONE, 0.02f + 0.06f * depth));
                }
            for (int x = x0; x < x0 + w; x++) tput(t, x, y0 + hh, sh(R_STONE, 0.72f));   /* sill */
            tex_skull(t, (int)cx - 7, y0 + hh - 15, 0.70f + 0.15f * h01(nx, ny, seed), 0);
        }
}

static void gen_banner(int t, uint32_t seed) {
    gen_stone(t, R_STONE, seed);
    for (int x = 10; x <= 53; x++) { tput(t, x, 3, sh(R_IRON, 0.7f)); tput(t, x, 4, sh(R_IRON, 0.35f)); }
    for (int x = 15; x <= 48; x++) {
        int bottom = 52 + (int)(4.0f * sinf(x * 0.7f) - 3.0f * fabsf(sinf(x * 1.9f)));
        for (int y = 5; y <= bottom; y++) {
            float v = 0.42f + 0.18f * sinf((x - 15) * 0.55f) - 0.003f * y + fbm(x / 5.0f, y / 5.0f, 16, seed) * 0.12f;
            uint8_t c = sh(R_BLOOD, v);
            if (x == 15 || x == 48 || y == 5 || y == 6) c = sh(R_BONE, 0.5f + 0.1f * sinf(y));
            tput(t, x, y, c);
        }
    }
    tex_skull(t, 25, 18, 0.85f, 0);
    for (int i = -7; i <= 7; i++) {                                  /* crossed bones under it */
        tput(t, 32 + i, 37 + i / 2, sh(R_BONE, 0.7f));
        tput(t, 32 + i, 37 - i / 2, sh(R_BONE, 0.6f));
    }
}

static void gen_torch(int t, int frame, uint32_t seed) {
    gen_stone(t, R_STONE, seed);
    for (int y = 38; y <= 47; y++)                                  /* bracket plate */
        for (int x = 27; x <= 36; x++) {
            float v = 0.42f + ((x == 27 || y == 38) ? 0.2f : 0.0f) - ((x == 36 || y == 47) ? 0.2f : 0.0f);
            if ((x == 29 || x == 34) && (y == 40 || y == 45)) v = 0.9f;
            tput(t, x, y, sh(R_IRON, v));
        }
    for (int y = 33; y < 38; y++) for (int x = 30; x <= 33; x++) tput(t, x, y, sh(R_IRON, 0.55f - (x - 30) * 0.08f));
    for (int y = 20; y <= 35; y++)                                  /* the torch */
        for (int x = 30; x <= 33; x++) tput(t, x, y, sh(R_WOOD, 0.5f - fabsf(x - 31.0f) * 0.12f - (y - 20) * 0.01f));
    static const float SWAY[3] = { 0.0f, 1.6f, -1.3f }, TALL[3] = { 1.0f, 0.9f, 1.1f };
    for (int y = 0; y <= 22; y++)
        for (int x = 20; x <= 44; x++) {
            float u = (22 - y) / (21.0f * TALL[frame]);              /* 0 at the base, 1 at the tip */
            if (u > 1.0f) continue;
            float cx = 31.5f + SWAY[frame] * u * u * 2.0f;
            float w = 5.6f * powf(fmaxf(0.0f, sinf(PI_F * (0.18f + 0.82f * (1.0f - u)))), 0.8f) * (1.0f - 0.35f * u);
            if (w < 0.3f) continue;
            float d = fabsf(x + 0.5f - cx) / (w + 0.01f);
            float n = fbm(x / 4.0f, y / 4.0f + frame * 3.0f, 16, seed + 90 + frame) * 0.5f;
            if (d > 1.0f - 0.25f * n) continue;
            float heat = 1.0f - d * 0.7f - u * 0.55f + n * 0.3f;
            tput(t, x, y, (uint8_t)(R_FLAME + clampi((int)(heat * 15.0f), 1, 15)));
        }
}

static void gen_door(int t, uint32_t seed) {
    for (int y = 0; y < TS; y++)
        for (int x = 0; x < TS; x++) {
            int p = x / 8, lx = x % 8;
            float v = 0.42f + 0.14f * h01(p, 0, seed) + 0.07f * sinf(y * 0.7f + p * 3.0f + x * 0.25f)
                    + fbm(x / 4.0f, y / 12.0f, 16, seed) * 0.12f;
            if (lx == 0) v = 0.05f;
            else if (lx == 1) v += 0.08f;
            else if (lx == 7) v -= 0.08f;
            tput(t, x, y, sh(R_WOOD, v));
        }
    for (int band = 0; band < 2; band++) {
        int y0 = band ? 51 : 9;
        for (int y = y0; y < y0 + 4; y++)
            for (int x = 0; x < TS; x++) {
                float v = 0.45f + (y == y0 ? 0.2f : 0.0f) - (y == y0 + 3 ? 0.2f : 0.0f);
                if (x % 8 == 4 && (y == y0 + 1 || y == y0 + 2)) v = 0.9f;
                tput(t, x, y, sh(R_IRON, v));
            }
    }
    for (int y = 15; y <= 26; y++)                                  /* barred window */
        for (int x = 24; x <= 39; x++) {
            int edge = x == 24 || x == 39 || y == 15 || y == 26;
            int bar = x == 28 || x == 32 || x == 36;
            tput(t, x, y, edge ? sh(R_IRON, 0.4f) : (bar ? sh(R_IRON, 0.65f) : 0));
        }
    for (int a = 0; a < 40; a++) {                                  /* ring handle */
        float ang = TAU_F * a / 40;
        int x = 47 + (int)lroundf(4.0f * cosf(ang)), y = 35 + (int)lroundf(4.5f * sinf(ang));
        tput(t, x, y, sh(R_IRON, 0.5f + 0.35f * -sinf(ang)));
    }
}

static void gen_jamb(int t, uint32_t seed) {
    for (int y = 0; y < TS; y++)
        for (int x = 0; x < TS; x++) {
            float v = 0.42f + fbm(x / 8.0f, y / 8.0f, 8, seed) * 0.2f - 0.1f;
            if (y % 16 == 0) v = 0.12f;
            if (x < 3) v += 0.15f;
            if (x > 60) v -= 0.15f;
            tput(t, x, y, sh(R_STONE, v));
        }
    for (int h = 0; h < 2; h++)
        for (int y = 10 + h * 40; y < 15 + h * 40; y++)
            for (int x = 0; x < 22; x++) tput(t, x, y, sh(R_IRON, 0.5f + (y == 10 + h * 40 ? 0.25f : 0.0f) + (x == 18 ? 0.3f : 0.0f)));
}

/* the locked door: dark planks, gold straps and a big lock plate */
static void gen_locked(int t, uint32_t seed, int metal) {
    for (int y = 0; y < TS; y++)
        for (int x = 0; x < TS; x++) {
            int p = x / 8, lx = x % 8;
            float v = 0.30f + 0.12f * h01(p, 0, seed) + 0.06f * sinf(y * 0.7f + p * 3.0f + x * 0.25f)
                    + fbm(x / 4.0f, y / 12.0f, 16, seed) * 0.12f;
            if (lx == 0) v = 0.04f; else if (lx == 1) v += 0.07f; else if (lx == 7) v -= 0.07f;
            tput(t, x, y, sh(R_WOOD, v));
        }
    for (int band = 0; band < 3; band++) {
        int y0 = band == 0 ? 6 : (band == 1 ? 30 : 54);
        for (int y = y0; y < y0 + 4; y++)
            for (int x = 0; x < TS; x++) {
                float v = 0.55f + (y == y0 ? 0.35f : 0.0f) - (y == y0 + 3 ? 0.35f : 0.0f);
                if (x % 10 == 5 && (y == y0 + 1 || y == y0 + 2)) v = 1.0f;
                tput(t, x, y, sh(metal, metal == R_GOLD ? v : 0.2f + v * 0.8f));
            }
    }
    for (int y = 20; y <= 46; y++)                                  /* lock plate with a keyhole */
        for (int x = 23; x <= 40; x++) {
            int edge = x == 23 || y == 20, shadow = x == 40 || y == 46;
            float v = 0.62f + (edge ? 0.3f : 0.0f) - (shadow ? 0.35f : 0.0f) + fbm(x / 3.0f, y / 3.0f, 16, seed + 7) * 0.15f;
            float kx = x + 0.5f - 31.5f, ky = y + 0.5f - 30.0f;
            if (kx * kx + ky * ky < 7.0f || (fabsf(kx) < 1.2f && ky > 0.0f && ky < 9.0f)) { tput(t, x, y, 0); continue; }
            if ((x == 25 || x == 38) && (y == 22 || y == 44)) v = 1.0f;
            tput(t, x, y, sh(metal, metal == R_GOLD ? v : 0.2f + v * 0.8f));
        }
}

/* a boss arena's gate: an arch of dressed stone over a portcullis, and the dark behind its bars */
static void gen_gate(int t, uint32_t seed) {
    gen_stone(t, R_STONE, seed);
    for (int y = 6; y < TS; y++)
        for (int x = 8; x < 56; x++) {
            float dx = (x + 0.5f - 32.0f) / 24.0f, dy = (y + 0.5f - 22.0f) / 16.0f;
            if (y < 22 && dx * dx + dy * dy > 1.0f) continue;
            float edge = y < 22 ? 1.0f - sqrtf(dx * dx + dy * dy) : fminf((x - 8) / 24.0f, (55 - x) / 24.0f);
            uint8_t c = sh(R_STONE, 0.03f + 0.04f * (1.0f - (float)y / TS));
            if (edge < 0.06f) c = sh(R_STONE, 0.75f);             /* the arch's lit edge */
            int bx = (x - 10) % 7, by = (y - 4) % 10;
            if (edge >= 0.06f && (bx == 0 || bx == 1)) c = sh(R_IRON, bx ? 0.42f : 0.62f + 0.1f * h01(x, y, seed));   /* the bars */
            if (edge >= 0.06f && (by == 0 || by == 1) && y > 8) c = sh(R_IRON, by ? 0.3f : 0.55f);
            if (y >= 58 && bx <= 1 && edge >= 0.06f) c = (y & 1) ? sh(R_IRON, 0.8f) : sh(R_IRON, 0.2f);   /* the spikes, a hand off the floor */
            tput(t, x, y, c);
        }
}

/* the tutorial's one-way gate (docs/PLAN-tutorial.md 7.3), its south face: the same portcullis, and riveted across its middle
 * bars an iron plate with a round boss sunk in it, ringed with cut marks, where the seal shows; the north face is the bare gate */
static void gen_gate_seal(int t, uint32_t seed) {
    gen_gate(t, seed);
    for (int y = 25; y <= 44; y++)
        for (int x = 21; x <= 43; x++) {
            float dx = x + 0.5f - 32.0f, dy = y + 0.5f - 34.5f, r = sqrtf(dx * dx + dy * dy);
            int rim = x == 21 || x == 43 || y == 25 || y == 44;
            uint8_t c = sh(R_IRON, (rim ? 0.62f : 0.3f) + 0.12f * (x < 23) - 0.1f * (y > 42) + 0.07f * h01(x / 2, y, seed + 5));
            if ((x == 23 || x == 41) && (y == 27 || y == 42)) c = sh(R_IRON, 0.85f);   /* its rivets */
            if (r < 8.5f) c = sh(R_IRON, 0.08f + 0.18f * fmaxf(0.0f, 0.45f * dx / 8.5f + 0.65f * dy / 8.5f + 0.37f));   /* the boss, sunk: lit from below-right */
            if (r >= 6.4f && r < 7.4f) {                           /* the ring of cut marks round it */
                float a = atan2f(dy, dx) + 3.1416f;
                if (((int)(a * 12.0f / 6.2832f)) & 1) c = sh(R_IRON, 0.7f);
            }
            if (r < 2.0f) c = sh(R_IRON, 0.5f);
            tput(t, x, y, c);
        }
    for (int x = 21; x <= 43; x++) tput(t, x, 45, sh(R_IRON, 0.05f));   /* its shadow on the bars below */
}
/* the tutorial's ice field (7.1): a sheet of old ice, pale and a little green in its depths, cracked across, bubbles caught in it,
 * rime feathered over; it tiles, and the floor shows none of itself under it (the game lays it a texel at a time as it creeps) */
static void gen_ice(int t, uint32_t seed) {
    for (int x = 0; x < TS; x++)
        for (int y = 0; y < TS; y++) {
            float v = 0.66f + 0.2f * (fbm(x / 16.0f, y / 16.0f, 4, seed) - 0.5f) + 0.08f * (fbm(x / 4.0f, y / 4.0f, 16, seed + 3) - 0.5f);
            if (fbm(x / 8.0f, y / 8.0f, 8, seed + 9) < 0.36f) v -= 0.14f;   /* deeper, clearer patches */
            tput(t, x, y, sh(R_GHOST, v));
        }
    for (int i = 0; i < 6; i++) {                                  /* the cracks: a dark line, a bright lip on its lit side */
        float x = h01(i, 1, seed) * 64.0f, y = h01(i, 2, seed) * 64.0f, a = h01(i, 3, seed) * 6.2832f;
        int len = 18 + (int)(h01(i, 4, seed) * 26.0f);
        for (int k = 0; k < len; k++) {
            a += (h01(i * 64 + k, 5, seed) - 0.5f) * 0.7f;
            x += cosf(a); y += sinf(a);
            tput(t, (int)floorf(x), (int)floorf(y), sh(R_GHOST, 0.36f));
            tput(t, (int)floorf(x) - 1, (int)floorf(y) - 1, sh(R_GHOST, 0.97f));
            if (k == len / 2 && (i & 1)) {                          /* a branch */
                float bx = x, by = y, ba = a + 1.2f;
                for (int j = 0; j < 9; j++) { bx += cosf(ba); by += sinf(ba); tput(t, (int)floorf(bx), (int)floorf(by), sh(R_GHOST, 0.42f)); }
            }
        }
    }
    for (int i = 0; i < 14; i++) {                                 /* bubbles caught in it */
        int bx = (int)(h01(i, 7, seed) * 64.0f), by = (int)(h01(i, 8, seed) * 64.0f), r = 1 + (h01(i, 9, seed) > 0.7f);
        for (int dy = -r; dy <= r; dy++) for (int dx = -r; dx <= r; dx++) if (dx * dx + dy * dy <= r * r) tput(t, bx + dx, by + dy, sh(R_GHOST, dx + dy < 0 ? 0.95f : 0.7f));
    }
    for (int i = 0; i < 160; i++) {                                /* rime feathered over it, and glints */
        int fx = (int)(h01(i, 10, seed) * 64.0f), fy = (int)(h01(i, 11, seed) * 64.0f);
        tput(t, fx, fy, i % 23 == 0 ? (uint8_t)C_WHITE : sh(R_GHOST, 0.82f + 0.15f * h01(i, 12, seed)));
        if (i & 1) tput(t, fx + 1, fy + (i & 2 ? 1 : -1), sh(R_GHOST, 0.78f));
    }
}

/* the ice over the floor at a world place, laid k of the way (0 none, 1 all): each texel comes in at its own moment, larger
 * flakes first, a fringe of rime ahead of the sheet; what isn't covered keeps the floor's own texel */
uint8_t crawl_ice_at(float wx, float wy, float k, uint8_t under) {
    if (k <= 0.0f) return under;
    int tx = (int)floorf(wx * TS), ty = (int)floorf(wy * TS);
    uint8_t ice = crawl_tex[T_ICE][(tx & (TS - 1)) * TS + (ty & (TS - 1))];
    if (k >= 1.0f) return ice;
    float th = 0.8f * fbm(wx * 2.5f, wy * 2.5f, 1 << 20, 831) + 0.2f * h01(tx, ty, 833);   /* in drifts, their edges frayed */
    th = clampf((th - 0.2f) / 0.6f, 0.0f, 1.0f);
    if (th < k) return ice;
    if (th < k + 0.07f) return sh(R_GHOST, 0.82f);                 /* the rime running ahead */
    return under;
}

/* the Accursed Portal, in the grave plot's far wall: a stone arch, and in it a slow swirl of red and
 * violet turning in on itself, a few sparks caught in it; four frames, a quarter turn apart */
static void gen_portal(int t, int frame, uint32_t seed) {
    gen_stone(t, R_STONE, seed);
    for (int y = 2; y < TS; y++)
        for (int x = 4; x < 60; x++) {
            float ax = (x + 0.5f - 32.0f) / 28.0f, ay = (y + 0.5f - 24.0f) / 22.0f;
            if (y < 24 && ax * ax + ay * ay > 1.0f) continue;
            float edge = y < 24 ? 1.0f - sqrtf(ax * ax + ay * ay) : fminf((x - 4) / 28.0f, (59 - x) / 28.0f);
            if (edge < 0.1f) { tput(t, x, y, sh(R_STONE, edge < 0.04f ? 0.8f : 0.3f)); continue; }   /* the arch's carved rim */
            float dx = x + 0.5f - 32.0f, dy = y + 0.5f - 36.0f, r = sqrtf(dx * dx + dy * dy), a = atan2f(dy, dx);
            float v = 0.5f + 0.5f * sinf(2.0f * a + r * 0.32f - (float)frame * PI_F * 0.5f) * (0.7f + 0.3f * sinf(r * 0.11f - frame));
            uint8_t c;
            if (r < 3.5f) c = r < 1.6f ? C_WHITE : (uint8_t)(R_PURPLE + 15);   /* its heart */
            else if (v > 0.72f) c = (uint8_t)(R_PURPLE + clampi(11 + (int)((v - 0.72f) * 14.0f), 11, 15));
            else if (v > 0.38f) c = (uint8_t)(R_PURPLE + 6 + (int)((v - 0.38f) * 12.0f));
            else c = (uint8_t)(R_BLOOD + 3 + (int)(v * 10.0f));
            if (h01(x + frame * 7, y, seed) > 0.985f) c = (uint8_t)(R_GLOW + 13);   /* a spark caught in it */
            tput(t, x, y, c);
        }
}

/* a font set into the wall: an arched niche over a stone basin, with a draught of pale water in it (a
 * few full-bright glints so it can be found in the dark), or drunk dry and cracked */
static void gen_font(int t, uint32_t seed, int full) {
    gen_stone(t, R_STONE, seed);
    float cx = 32.0f;
    for (int y = 8; y < 48; y++)
        for (int x = 14; x < 50; x++) {
            float dx = (x + 0.5f - cx) / 18.0f, dy = (y + 0.5f - 22.0f) / 14.0f;
            if (y < 22 && dx * dx + dy * dy > 1.0f) continue;
            tput(t, x, y, sh(R_STONE, 0.03f + 0.08f * (y - 8) / 40.0f));
        }
    for (int y = 38; y < 56; y++)                                   /* the basin, a half bowl standing out of the wall */
        for (int x = 12; x < 52; x++) {
            float dx = (x + 0.5f - cx) / 20.0f, dy = (y + 0.5f - 38.0f) / 16.0f;
            if (dx * dx + dy * dy > 1.0f) continue;
            float v = 0.38f + 0.28f * (-dx * 0.5f) + 0.25f * (1.0f - dy) + fbm(x / 4.0f, y / 4.0f, 16, seed + 5) * 0.12f;
            if (y == 38 || y == 39) v = 0.8f;                       /* its lip */
            tput(t, x, y, sh(R_STONE, v));
        }
    for (int x = 15; x < 49; x++)                                   /* what is in it, seen over the lip */
        for (int y = 34; y < 38; y++) {
            float dx = (x + 0.5f - cx) / 17.0f, dy = (y + 0.5f - 37.5f) / 3.5f;
            if (dx * dx + dy * dy > 1.0f) continue;
            uint8_t c;
            if (full) {
                c = sh(R_GHOST, 0.45f + 0.3f * sinf(x * 0.6f + y * 1.3f) * 0.5f + 0.15f * (y - 34) / 4.0f);
                if (h01(x, y, seed + 9) > 0.86f) c = (uint8_t)(R_GLOW + 9 + (int)(h01(y, x, seed) * 5.0f));
            } else c = sh(R_STONE, 0.05f + 0.05f * h01(x, y, seed));
            tput(t, x, y, c);
        }
    if (!full)                                                      /* a crack down the dry bowl */
        for (int y = 40, x = 30; y < 54; y++) { x += (int)(h01(y, 3, seed) * 3.0f) - 1; tput(t, x, y, sh(R_STONE, 0.06f)); }
}

static void gen_sign(int t, uint32_t seed) {
    gen_stone(t, R_STONE, seed);
    for (int y = 14; y < 48; y++)
        for (int x = 8; x < 56; x++) {
            int edge = y == 14 || y == 47 || x == 8 || x == 55;
            float v = 0.35f + 0.08f * h01(0, (y - 14) / 6, seed) + fbm(x / 6.0f, y / 2.0f, 16, seed + 3) * 0.15f - ((y - 14) % 6 == 0 ? 0.12f : 0.0f);
            tput(t, x, y, edge ? sh(R_WOOD, 0.15f) : sh(R_WOOD, v));
        }
    for (int i = 0; i < 14; i++) { tput(t, 16, i, sh(R_IRON, (i & 1) ? 0.7f : 0.4f)); tput(t, 47, i, sh(R_IRON, (i & 1) ? 0.7f : 0.4f)); }
    for (int y = 18; y < 38; y++)                                  /* a cat's head, burned into the wood */
        for (int x = 18; x < 46; x++) {
            float dx = (x + 0.5f - 32) / 10.0f, dy = (y + 0.5f - 30) / 7.5f;
            int head = dx * dx + dy * dy < 1.0f;
            int ear = (y < 26 && y >= 18) && ((fabsf(x + 0.5f - 25) < (y - 18) * 0.55f) || (fabsf(x + 0.5f - 39) < (y - 18) * 0.55f));
            if (head || ear) tput(t, x, y, sh(R_WOOD, 0.08f));
        }
    tput(t, 28, 29, sh(R_GOLD, 0.9f)); tput(t, 29, 29, sh(R_GOLD, 0.7f)); tput(t, 35, 29, sh(R_GOLD, 0.9f)); tput(t, 36, 29, sh(R_GOLD, 0.7f));
    for (int c = 0; c < 3; c++)                                    /* and three coins */
        for (int y = 40; y < 45; y++)
            for (int x = 22 + c * 8; x < 28 + c * 8; x++)
                if (hypotf(x + 0.5f - (25 + c * 8), y + 0.5f - 42.5f) < 2.6f) tput(t, x, y, sh(R_GOLD, 0.5f + 0.4f * (y < 42)));
}

/* the final door of each level: an arch of old stone, iron doors, a skull for a keystone, and
 * runes that glow; opened, it is nothing but light */
/* A secret's doorway into its pockets (until each theme gives it its own look): an opening cut in the
 * stone, and inside it doorway after doorway going away into a pale light, yellow for the first two
 * sets and cold white for the last two; full-bright, so it shows in the dark of the room */
/* Looking through a doorway (x 18..45, y 10..63) into a room beyond, in one-point perspective towards
 * (32, 34): how far in (s, 1 at the doorway's edge, smaller further in) and which face is seen. */
enum { RV_WALL, RV_CEIL, RV_FLOOR };
static float room_view(int x, int y, int *face) {
    float px = x + 0.5f - 32.0f, py = y + 0.5f - 34.0f;
    float sx = fabsf(px) / 14.0f, st = -py / 24.0f, sb = py / 29.0f, s = sx;
    *face = RV_WALL;
    if (st > s) { s = st; *face = RV_CEIL; }
    if (sb > s) { s = sb; *face = RV_FLOOR; }
    return s;
}

/* A secret's doorway into its pockets, cut through the stone. Into the Backrooms (the first two sets):
 * a glimpse of a pale yellow room under a tube light; into Severance, a white hall and a pale door. */
static void gen_passage(int t, int warm) {
    for (int y = 0; y < TS; y++)
        for (int x = 0; x < TS; x++) {
            uint8_t c;
            if (x < 18 || x > 45 || y < 12) {
                c = sh(R_STONE, block_shade(x, y, 221) * 0.8f);
                if ((x == 17 || x == 46) && y >= 12) c = sh(R_STONE, 0.1f);   /* the cut's edge */
                if (y == 11 && x >= 17 && x <= 46) c = sh(R_STONE, 0.12f);
            } else if (warm) {                                     /* the Backrooms beyond, in bone's pale yellow */
                int face;
                float s = room_view(x, y, &face);
                if (s < 0.4f) c = sh(R_BONE, 0.74f + 0.04f * h01(x, y, 225));          /* the far wall */
                else if (face == RV_CEIL) c = (fabsf(x + 0.5f - 32.0f) < 3.5f * s && s > 0.5f && s < 0.68f) ? C_WHITE : sh(R_BONE, 0.82f - 0.2f * s);
                else if (face == RV_FLOOR) c = sh(R_BONE, 0.42f + 0.05f * h01(x, y, 226) - 0.08f * s);
                else c = sh(R_BONE, 0.66f - 0.14f * s + ((x & 3) == 0 ? 0.04f : 0.0f));
            } else {                                               /* Severance beyond: a white hall, green carpet, a pale door */
                int face;
                float s = room_view(x, y, &face);
                if (s < 0.4f) c = (x >= 30 && x <= 33 && y >= 30 && y <= 42) ? sh(R_MOSS, 0.78f)
                                : (x >= 29 && x <= 34 && y >= 29 && y <= 42) ? sh(R_GHOST, 0.55f) : sh(R_GHOST, 0.88f);   /* the far wall, a door in it */
                else if (face == RV_CEIL) c = (fabsf(x + 0.5f - 32.0f) < 5.0f * s && fmodf(s * 6.0f, 1.0f) < 0.5f) ? C_WHITE : sh(R_GHOST, 0.94f - 0.1f * s);
                else if (face == RV_FLOOR) c = sh(R_MOSS, 0.30f + 0.03f * h01(x, y, 227) - 0.04f * s);
                else c = sh(R_GHOST, 0.86f - 0.12f * s);
            }
            tput(t, x, y, c);
        }
}

/* A chain's doorway after the dweller failed its trial: the same cut in the stone, the frames inside gone
 * dark, and across the opening a dull red seal, cracked through, with a little of it run down the stone */
static void gen_sealed(int t) {
    for (int y = 0; y < TS; y++)
        for (int x = 0; x < TS; x++) {
            uint8_t c;
            if (x < 18 || x > 45 || y < 12) {
                c = sh(R_STONE, block_shade(x, y, 221) * 0.8f);
                if ((x == 17 || x == 46) && y >= 12) c = sh(R_STONE, 0.1f);
                if (y == 11 && x >= 17 && x <= 46) c = sh(R_STONE, 0.12f);
            } else {                                               /* doorway after doorway, unlit */
                int face;
                float s = room_view(x, y, &face), f = fmodf(s * 5.0f, 1.0f);
                c = sh(R_STONE, (f < 0.12f ? 0.13f : 0.04f) + 0.05f * s + (face == RV_FLOOR ? 0.02f : 0.0f));
            }
            float dx = x + 0.5f - 32.0f, dy = y + 0.5f - 33.0f, r = sqrtf(dx * dx + dy * dy);
            float crack = fabsf(dy + dx * 0.55f - 1.5f * sinf(dx * 0.7f));   /* a jagged break, lower left to upper right */
            if (r < 10.5f) {
                float wob = 10.0f + 0.6f * sinf(atan2f(dy, dx) * 7.0f);     /* pressed wax, its rim uneven */
                if (r < wob) {
                    float v = 0.34f + 0.14f * (-dx - dy) / 14.0f - (r > wob - 1.5f ? 0.08f : 0.0f);
                    if (r < 6.0f && r > 4.6f) v -= 0.1f;           /* the ring of the die */
                    if (fabsf(dx) < 0.8f && dy > -4.0f && dy < 4.0f) v -= 0.1f;   /* a mark struck in it */
                    if (fabsf(dy + 1.0f) < 0.8f && fabsf(dx) < 2.6f) v -= 0.1f;
                    c = sh(R_BLOOD, v);
                    if (crack < 0.9f) c = sh(R_STONE, 0.02f);
                    else if (crack < 1.6f) c = sh(R_BLOOD, v - 0.12f);
                }
            }
            if (x >= 27 && x <= 29 && y > 42 && y < 50 + 3 * (x == 28) && r > 9.0f) c = sh(R_BLOOD, 0.22f);   /* a run of it */
            tput(t, x, y, c);
        }
}

/* The Wretched Repository: the stone cut with three narrow arched niches, an iron door shut in each,
 * riveted, a lock plate and a ring, rust running from the rivets */
static void gen_repository(int t) {
    gen_stone(t, R_STONE, 233);
    for (int k = 0; k < 3; k++) {
        int x0 = 3 + k * 21, w = 16, y0 = 10, y1 = 56;
        float cx = x0 + w / 2.0f;
        for (int y = y0 - 1; y <= y1; y++)
            for (int x = x0 - 1; x <= x0 + w; x++) {
                int inside;
                if (y < y0 + 7) { float dx = (x + 0.5f - cx) / (w / 2.0f + 1.0f), dy = (y + 0.5f - (y0 + 7)) / 8.0f; inside = dx * dx + dy * dy <= 1.0f; }
                else inside = x >= x0 - 1 && x <= x0 + w;
                if (!inside) continue;
                uint8_t c = sh(R_STONE, 0.04f);                        /* the niche's dark rim */
                int door = x >= x0 + 1 && x <= x0 + w - 2 && y >= y0 + 2;
                if (door) {
                    float v = 0.30f + 0.10f * (x - x0) / (float)w + (fbm(x / 3.0f, y / 6.0f, 8, 239 + k) - 0.5f) * 0.12f;
                    if (x == x0 + 1 || y == y0 + 2) v += 0.12f;            /* its edge catching the light */
                    if ((y - y0) % 13 == 6) v -= 0.12f;                    /* the bands */
                    c = sh(R_IRON, v);
                    if (((y - y0) % 13 == 5 || (y - y0) % 13 == 7) && (x == x0 + 3 || x == x0 + w - 4)) c = sh(R_IRON, 0.7f);   /* rivets */
                    if (((y - y0) % 13 == 8 || (y - y0) % 13 == 9) && (x == x0 + 3 || x == x0 + w - 4) && h01(x, y, 241 + k) > 0.3f) c = sh(R_WARM, 0.32f);   /* rust run */
                }
                tput(t, x, y, c);
            }
        for (int y = 30; y <= 36; y++) for (int x = (int)cx - 2; x <= (int)cx + 2; x++) tput(t, x, y, sh(R_IRON, 0.62f));   /* the lock plate */
        tput(t, (int)cx, 32, sh(R_IRON, 0.02f)); tput(t, (int)cx, 33, sh(R_IRON, 0.02f));   /* its keyhole */
        for (int a = 0; a < 12; a++) { float an = TAU_F * a / 12.0f; tput(t, (int)(cx + 2.4f * cosf(an)), (int)(41 + 2.4f * sinf(an)), sh(R_IRON, 0.55f + 0.2f * -sinf(an))); }   /* a ring to pull */
        for (int x = x0 - 1; x <= x0 + w; x++) tput(t, x, y1 + 1, sh(R_STONE, 0.7f));   /* the sill */
    }
}

