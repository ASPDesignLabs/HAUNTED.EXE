/* A part of crawl_art.c (docs/PLAN-tidy.md): it is #included there in its place and never compiled on its own.
 * The runs' hazards: what stands in the wall, and their sprites (grit, the censer, votives, snares, spikes, ash). */

/* ---- the runs' hazards (1.0.0b13, docs/PLAN-b13.md section 5): what stands in the wall, on plain stone ---- */

/* a burial slab set in the wall, its rings and its line of lettering; cracked, a seam of pale light round it */
static void gen_hz_slab(int t, uint32_t seed, int open) {
    gen_stone(t, R_STONE, seed);
    for (int y = 18; y < 46; y++)
        for (int x = 8; x < 56; x++) {
            int edge = y == 18 || y == 45 || x == 8 || x == 55;
            float v = 0.42f + 0.12f * fbm(x / 6.0f, y / 6.0f, 16, seed + 5) - (y > 40 ? 0.08f : 0.0f);
            uint8_t c = edge ? sh(R_STONE, 0.18f) : sh(R_BONE, v * 0.75f);
            if (!edge && y >= 28 && y <= 31 && x >= 16 && x < 48 && ((x / 3) & 1) && h01(x, y, seed) > 0.25f) c = sh(R_BONE, 0.12f);   /* the lettering */
            tput(t, x, y, c);
        }
    for (int r = 0; r < 2; r++) {                                      /* iron rings to draw it out by */
        int cx = r ? 46 : 18, cy = 38;
        for (int a = 0; a < 20; a++) tput(t, cx + (int)lroundf(2.5f * cosf(a * 0.314f)), cy + (int)lroundf(2.5f * sinf(a * 0.314f)), sh(R_IRON, 0.35f + 0.2f * (a < 10)));
    }
    if (!open) return;
    for (int i = 0; i < 2 * (48 + 28); i++) {                          /* the seam: pale, as if lit from inside */
        int x, y, k = i % (48 + 28);
        if (k < 48) { x = 8 + k; y = i < 48 + 28 ? 17 : 46; } else { y = 18 + (k - 48); x = i < 48 + 28 ? 7 : 56; }
        tput(t, x, y, h01(x, y, seed + 9) > 0.3f ? (uint8_t)(R_GLOW + 13) : (uint8_t)C_WHITE);
    }
    for (int x = 20; x < 30; x++) tput(t, x, 18 + (x - 20) / 3, R_GLOW + 10);   /* and a crack across a corner */
}

/* a crack in the wall that breathes cold: rime furred out round it, a little blue at its heart */
static void gen_hz_cold(int t, uint32_t seed) {
    gen_stone(t, R_STONE, seed);
    float x = 30.0f + 4.0f * h01(1, 1, seed);
    for (int y = 6; y < 60; y++) {
        x += (h01(y, 3, seed) - 0.5f) * 2.4f;
        int w = 1 + (y > 18 && y < 48);
        for (int dx = -w; dx <= w; dx++) tput(t, (int)x + dx, y, dx == 0 ? sh(R_GLOW, 0.15f) : sh(R_STONE, 0.02f));
        for (int k = 0; k < 7; k++) {                                  /* the rime, feathering out */
            float ax = x + (h01(y, k, seed + 2) - 0.5f) * 18.0f;
            if (h01(y, k + 20, seed) < 0.55f - fabsf(ax - x) / 22.0f) tput(t, (int)ax, y, sh(R_GHOST, 0.55f + 0.4f * h01(y, k, seed + 3)));
        }
    }
}

/* a sluice gate low in the wall: an iron plate in its guides, a wheel to wind it; open, the dark and the water coming */
static void gen_hz_sluice(int t, uint32_t seed, int open) {
    gen_stone(t, R_STONE, seed);
    for (int y = 30; y < 64; y++)
        for (int x = 14; x < 50; x++) {
            int guide = x < 17 || x > 46;
            uint8_t c;
            if (guide) c = sh(R_IRON, 0.3f + 0.15f * (x == 15 || x == 48));
            else if (!open) c = sh(R_IRON, 0.18f + 0.1f * ((y - 30) % 8 == 0) + 0.06f * h01(x, y, seed) + 0.1f * ((x * 7 + y) % 23 == 0));   /* riveted plate */
            else if (y < 40) c = sh(R_IRON, 0.22f + 0.08f * ((y - 30) % 8 == 0));   /* thrown up, its edge showing */
            else { float f = fbm(x / 4.0f, (y + 0.0f) / 3.0f, 16, seed + 7); c = f > 0.55f ? sh(R_GHOST, 0.6f + 0.35f * f) : sh(R_STONE, 0.02f + 0.06f * f); }   /* water foaming out of the dark */
            tput(t, x, y, c);
        }
    for (int a = 0; a < 28; a++) {                                     /* the wheel above it */
        float ang = a * 0.2244f;
        tput(t, 32 + (int)lroundf(7.0f * cosf(ang)), 20 + (int)lroundf(7.0f * sinf(ang)), sh(R_IRON, 0.45f));
    }
    for (int k = 0; k < 4; k++) for (int r = 1; r < 7; r++) tput(t, 32 + (int)lroundf(r * cosf(k * 0.785f)), 20 + (int)lroundf(r * sinf(k * 0.785f)), sh(R_IRON, 0.35f));
    for (int y = 27; y < 31; y++) tput(t, 32, y, sh(R_IRON, 0.4f));
}

/* the Weeping Saint: a robed figure in a round-headed alcove, her hands together, tear tracks down her face;
 * drinking, her eyes alight */
static void gen_hz_saint(int t, uint32_t seed, int lit) {
    gen_stone(t, R_STONE, seed);
    for (int y = 6; y < 62; y++)
        for (int x = 14; x < 50; x++) {
            float dx = x + 0.5f - 32.0f, top = 20.0f - sqrtf(fmaxf(0.0f, 18.0f * 18.0f - dx * dx));
            if (y < top + 4.0f) continue;
            tput(t, x, y, sh(R_STONE, 0.03f + 0.04f * h01(x, y, seed)));     /* the alcove's dark */
        }
    for (int y = 14; y < 62; y++)
        for (int x = 18; x < 46; x++) {
            float dx = x + 0.5f - 32.0f, head = (dx * dx) / 25.0f + (y + 0.5f - 19.0f) * (y + 0.5f - 19.0f) / 30.0f;
            float robe = y >= 24 ? fabsf(dx) - (5.0f + (y - 24) * 0.28f) : 99.0f;
            if (head <= 1.0f) tput(t, x, y, sh(R_BONE, 0.5f - 0.04f * dx));
            else if (robe <= 0.0f) tput(t, x, y, sh(R_GHOST, 0.18f + 0.14f * (dx < 0 ? 1.0f : 0.4f) + 0.05f * h01(x, y, seed) - 0.12f * ((int)(dx + 40) % 4 == 0)));
        }
    for (int y = 30; y < 36; y++) for (int x = 29; x < 35; x++) tput(t, x, y, sh(R_BONE, 0.42f));   /* the hands */
    for (int e = 0; e < 2; e++) {
        int ex = e ? 34 : 29;
        tput(t, ex, 18, lit ? (uint8_t)(R_GLOW + 15) : sh(R_STONE, 0.05f));
        tput(t, ex + 1, 18, lit ? (uint8_t)(R_GLOW + 12) : sh(R_STONE, 0.08f));
        for (int y = 19; y < 28; y++) tput(t, ex + (y > 24), y, sh(R_GHOST, 0.75f));   /* the tears */
    }
    for (int x = 18; x < 46; x++) tput(t, x, 61, sh(R_GHOST, 0.5f + 0.3f * h01(x, 61, seed)));   /* her pool's beginning */
}

/* the Lodestone Seam: a band of dark rock veined green, the veins crackling blue-white when it is about to arc */
static void gen_hz_seam(int t, uint32_t seed, int lit) {
    gen_stone(t, R_STONE, seed);
    for (int y = 0; y < TS; y++)
        for (int x = 0; x < TS; x++) {
            float band = fabsf(y - 32.0f - 8.0f * sinf(x * 0.09f + 1.0f)) - 12.0f;
            if (band > 0.0f) continue;
            float v = fbm(x / 5.0f, y / 5.0f, 16, seed + 3);
            uint8_t c = sh(R_IRON, 0.08f + 0.12f * v);
            float vein = fabsf(fbm(x / 9.0f, y / 4.0f, 8, seed + 11) - 0.5f);
            if (vein < 0.035f) c = lit ? (uint8_t)(h01(x, y, seed) > 0.5f ? R_GLOW + 14 : C_WHITE) : sh(R_MOSS, 0.55f + 0.4f * (0.035f - vein) / 0.035f);
            tput(t, x, y, c);
        }
}

/* Fissure Breath: a crack down black rock, glowing from deep inside; bright, it is about to breathe */
static void gen_hz_fissure(int t, uint32_t seed, int hot) {
    gen_stone(t, R_STONE, seed);
    for (int y = 0; y < TS; y++) for (int x = 0; x < TS; x++) tput(t, x, y, sh(R_IRON, 0.05f + 0.1f * fbm(x / 6.0f, y / 6.0f, 16, seed)));
    float x = 32.0f;
    for (int y = 2; y < 62; y++) {
        x += (h01(y, 5, seed) - 0.5f) * 2.0f;
        int w = 1 + (y > 12 && y < 52) + (hot && y > 20 && y < 44);
        for (int dx = -w - 2; dx <= w + 2; dx++) {
            int ad = abs(dx);
            uint8_t c = ad <= w ? (uint8_t)(R_FLAME + (hot ? 15 - ad * 2 : 9 - ad * 3)) : sh(R_BRICK, (hot ? 0.75f : 0.45f) - 0.15f * (ad - w));
            tput(t, (int)x + dx, y, c);
        }
    }
}

/* the Black Mirror: a sheet of obsidian in an iron frame, a cold streak across it; and, drinking, a shape in it */
static void gen_hz_mirror(int t, uint32_t seed, int seen) {
    gen_stone(t, R_STONE, seed);
    for (int y = 6; y < 60; y++)
        for (int x = 12; x < 52; x++) {
            int frame = x < 15 || x > 48 || y < 9 || y > 56;
            uint8_t c = frame ? sh(R_IRON, 0.3f + 0.15f * (x == 12 || y == 6)) : sh(R_IRON, 0.02f + 0.03f * h01(x, y, seed));
            if (!frame && fabsf((x - 15) - (y - 9) * 0.7f - 6.0f) < 1.2f) c = sh(R_GHOST, 0.3f);   /* the streak */
            tput(t, x, y, c);
        }
    if (!seen) return;
    for (int y = 16; y < 56; y++)                                      /* a hooded shape, standing where you stand */
        for (int x = 18; x < 46; x++) {
            float dx = x + 0.5f - 32.0f, hood = dx * dx / 30.0f + (y - 22.0f) * (y - 22.0f) / 40.0f, body = y > 26 ? fabsf(dx) - (7.0f + (y - 26) * 0.25f) : 99.0f;
            if ((hood <= 1.0f || body <= 0.0f) && ((x + y) & 1)) tput(t, x, y, sh(R_PURPLE, 0.25f + 0.15f * (hood <= 1.0f)));
        }
    tput(t, 30, 23, R_GLOW + 12); tput(t, 34, 23, R_GLOW + 12);
}

/* grit falling from the ceiling in a thin column, two frames of it */
static void spr_grit(int id, int frame) {
    cv_begin(cv_sprite, TS, TS);
    uint32_t s = 911u + (uint32_t)frame * 37u;
    for (int i = 0; i < 70; i++) {
        float y = h01(i, 1, s) * 52.0f, x = 32.0f + (h01(i, 2, s) - 0.5f) * (3.0f + y * 0.08f);
        cv_px((int)x, (int)y, sh(R_BONE, 0.35f + 0.4f * h01(i, 3, s)));
        if (h01(i, 4, s) > 0.7f) cv_px((int)x, (int)y + 1, sh(R_STONE, 0.5f));
    }
    cv_store(id);
}

/* the Swinging Censer: a great iron bowl on its chain, pierced, smoke curling out of it (hung from the top) */
static void spr_censer(void) {
    cv_begin(cv_sprite, TS, TS);
    for (int y = 0; y < 40; y++) cv_px(32, y, (y & 3) == 0 ? sh(R_IRON, 0.55f) : sh(R_IRON, 0.32f));   /* the chain */
    for (int k = 0; k < 3; k++) cv_line(32, 38, 22 + k * 10, 44, sh(R_IRON, 0.35f));
    cv_ellipse(32, 49, 11, 7, R_IRON, 0.12f, 0.6f);
    for (int x = 23; x <= 41; x += 4) cv_px(x, 48, R_FLAME + 9);       /* the holes, a coal's light in them */
    cv_flat(32, 43.5f, 10, 1.6f, sh(R_IRON, 0.5f));
    for (int i = 0; i < 26; i++) {                                    /* smoke, rising off it */
        float y = 42.0f - h01(i, 1, 41) * 30.0f, x = 32.0f + sinf(y * 0.3f + i) * (2.0f + (42 - y) * 0.15f);
        cv_px((int)x, (int)y, sh(R_STONE, 0.45f + 0.3f * h01(i, 2, 41)));
    }
    cv_outline(sh(R_IRON, 0.02f));
    cv_store(SP_CENSER);
}
/* a soft puff, dithered at its edge so it shows the crypt through it: smoke, the embalmers' breath, gas */
static void spr_puff(int id, int ramp, float lo, float hi, uint32_t seed) {
    cv_begin(cv_sprite, TS, TS);
    for (int y = 20; y < 64; y++)
        for (int x = 0; x < TS; x++) {
            float dx = (x + 0.5f - 32.0f) / 30.0f, dy = (y + 0.5f - 50.0f) / 14.0f, d = dx * dx + dy * dy;
            d += (fbm(x / 7.0f, y / 5.0f, 16, seed) - 0.5f) * 0.9f;
            if (d > 1.0f || (d > 0.55f && ((x + y) & 1)) || (d > 0.8f && (x & 1))) continue;
            cv_px(x, y, sh(ramp, lo + (hi - lo) * (1.0f - d) * (0.7f + 0.3f * fbm(x / 3.0f, y / 3.0f, 16, seed + 1))));
        }
    cv_store(id);
}
/* a votive rack gone over: an iron frame on its side, cups scattered, wax running from them alight */
static void spr_votive(void) {
    cv_begin(cv_sprite, TS, TS);
    cv_flat(32, 61, 24, 2.5f, R_WARM + 1);
    cv_line(12, 58, 50, 50, sh(R_IRON, 0.4f)); cv_line(12, 59, 50, 51, sh(R_IRON, 0.25f));
    cv_line(16, 57, 16, 48, sh(R_IRON, 0.35f)); cv_line(46, 51, 47, 42, sh(R_IRON, 0.35f));
    for (int i = 0; i < 7; i++) {
        float x = 10.0f + i * 7.0f + 3.0f * h01(i, 1, 51), y = 56.0f + 3.0f * h01(i, 2, 51);
        cv_ellipse(x, y, 2.4f, 1.8f, R_BLOOD, 0.3f, 0.7f);
        cv_line(x + 2, y + 1, x + 6, y + 2, sh(R_BONE, 0.8f));               /* the wax running */
        if (i & 1) { cv_px((int)x, (int)y - 3, R_FLAME + 12); cv_px((int)x, (int)y - 2, R_FLAME + 8); }
    }
    cv_store(SP_VOTIVE);
}
/* the Hungry Reliquary: a gilded casket on a stone plinth, its lid lifted on a violet dark when it feeds */
static void spr_hz_reliq(int id, int open) {
    cv_begin(cv_sprite, TS, TS);
    for (int y = 46; y < 64; y++) for (int x = 20; x < 44; x++) cv_px(x, y, sh(R_STONE, 0.35f + 0.15f * (x < 24) - 0.1f * (y > 60)));   /* the plinth */
    for (int y = 34; y < 46; y++) for (int x = 22; x < 42; x++) cv_px(x, y, sh(R_GOLD, 0.45f + 0.3f * (x < 30) + 0.2f * ((x + y) % 7 == 0)));   /* the casket */
    for (int x = 22; x < 42; x++) cv_px(x, 40, sh(R_GOLD, 0.25f));
    cv_flat(32, 40, 2.2f, 2.2f, sh(R_PURPLE, 0.6f));                  /* a stone in it */
    if (!open) {
        float lid[8] = { 21, 34, 43, 34, 39, 28, 25, 28 };
        cv_poly(lid, 4, R_GOLD, 0.85f, 0.5f, 28, 6);
    } else {
        for (int y = 30; y < 35; y++) for (int x = 24; x < 40; x++) cv_px(x, y, (x + y) & 1 ? (uint8_t)(R_PURPLE + 13) : (uint8_t)(R_GLOW + 6));
        cv_line(22, 34, 30, 18, sh(R_GOLD, 0.8f)); cv_line(42, 34, 36, 18, sh(R_GOLD, 0.6f)); cv_line(30, 18, 36, 18, sh(R_GOLD, 0.9f));   /* the lid thrown up */
    }
    cv_outline(sh(R_STONE, 0.02f));
    cv_store(id);
}
/* a web across a doorway, wall to wall and floor to beam: strands so fine they hardly show */
static void spr_snare(void) {
    cv_begin(cv_sprite, TS, TS);
    for (int k = 0; k < 10; k++) {
        float a = k * 0.628f;
        cv_line(32, 30, 32 + 34.0f * cosf(a), 30 + 34.0f * sinf(a), sh(R_GHOST, 0.35f + 0.1f * (k & 1)));
    }
    for (int r = 4; r < 30; r += 5)
        for (int k = 0; k < 10; k++) {
            float a0 = k * 0.628f, a1 = a0 + 0.628f;
            cv_line(32 + r * cosf(a0), 30 + r * sinf(a0), 32 + r * cosf(a1), 30 + r * sinf(a1), sh(R_GHOST, 0.3f));
        }
    cv_store(SP_SNARE);
}
/* Shatterglass: black glass spikes stood up from a star of cracks, their edges cold */
static void spr_spikes(void) {
    cv_begin(cv_sprite, TS, TS);
    for (int i = 0; i < 9; i++) {
        float x = 10.0f + i * 5.5f + 2.0f * h01(i, 1, 61), h = 14.0f + 22.0f * h01(i, 2, 61) * (1.0f - fabsf(i - 4.0f) / 6.0f);
        float lean = (h01(i, 3, 61) - 0.5f) * 8.0f;
        float tri[6] = { x - 3.0f, 63.0f, x + 3.0f, 63.0f, x + lean, 63.0f - h };
        cv_poly(tri, 3, R_IRON, 0.12f, 0.45f, 63.0f - h, h);
        cv_line(x + 3.0f, 63, x + lean, 63.0f - h, sh(R_GHOST, 0.7f));   /* the edge, catching the light */
        cv_line(x - 3.0f, 63, x + lean, 63.0f - h, sh(R_IRON, 0.05f));
    }
    cv_store(SP_SPIKES);
}
/* ash coming down: grey flakes, thicker below */
static void spr_ashfall(void) {
    cv_begin(cv_sprite, TS, TS);
    for (int i = 0; i < 160; i++) {
        float y = sqrtf(h01(i, 1, 71)) * 63.0f, x = 4.0f + h01(i, 2, 71) * 56.0f;
        cv_px((int)x, (int)y, sh(R_STONE, 0.45f + 0.4f * h01(i, 3, 71)));
        if (h01(i, 4, 71) > 0.5f) cv_px((int)x + 1, (int)y, sh(R_STONE, 0.35f));
    }
    cv_store(SP_ASHFALL);
}

