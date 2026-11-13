/* A part of scene_crawl.c (docs/PLAN-tidy.md): it is #included there in its place and never compiled on its own.
 * The hand: built and posed, the charge, Restoration's glow, render_hand; the latched bat, the vessel's slip, wisps. */

static int is_skin(int x, int y) {
    if (x < 0 || y < 0 || x >= HAND_W || y >= HAND_H) return 0;
    uint8_t c = crawl_hand_img[y * HAND_W + x];
    return c >= R_SKIN && c < R_SKIN + 16;
}

static void build_hand(void) {
    const Gear *g = &RUN.gear;
    const Piece *robe = &g->worn[SL_ROBE], *ring = &g->worn[SL_RING];
    memcpy(hand_buf, crawl_hand_img, sizeof hand_buf);
    memset(hand_rune, 0, sizeof hand_rune);
    int ramp = robe->kind ? QUAL_RAMP[robe->quality] : R_PURPLE, top = ramp == R_GOLD ? 12 : 15;
    int shift = robe->kind && robe->quality == Q_COMMON ? -3 : 0;  /* plain cloth, not bone-white */
    for (int y = 0; y < HAND_H; y++)                               /* the sleeve */
        for (int x = 0; x < HAND_W; x++) {
            uint8_t *p = &hand_buf[y * HAND_W + x];
            if (*p < R_PURPLE || *p >= R_PURPLE + 16) continue;
            *p = (uint8_t)(ramp + clampi(*p - R_PURPLE + shift, 0, top));
        }
    if (robe->kind && robe->anointed)                              /* anointed: a second band of trim */
        for (int y = 1; y < HAND_H; y++)
            for (int x = 0; x < HAND_W - 6; x++)
                if (crawl_hand_img[y * HAND_W + x] >= R_GOLD && crawl_hand_img[y * HAND_W + x] < R_GOLD + 13) {
                    int tx = x + 4, ty = y + 5;
                    if (tx < HAND_W && ty < HAND_H && crawl_hand_img[ty * HAND_W + tx] >= R_PURPLE && crawl_hand_img[ty * HAND_W + tx] < R_PURPLE + 16)
                        hand_buf[ty * HAND_W + tx] = (uint8_t)(R_GOLD + 6 + (x & 1) * 3);
                }
    /* the right bracer: an iron cuff on the wrist just above the sleeve's trim, inlaid in its quality's colour */
    const Piece *br = &g->worn[SL_WRIST_R];
    if (br->kind) {
        int cr = QUAL_RAMP[br->quality];
        for (int x = 0; x < HAND_W; x++) {
            int top = -1;
            for (int y = 0; y < HAND_H && top < 0; y++)
                if (crawl_hand_img[y * HAND_W + x] >= R_GOLD && crawl_hand_img[y * HAND_W + x] < R_GOLD + 13) top = y;
            if (top < 0) continue;
            for (int dy = 1; dy <= 7; dy++) {
                int y = top - dy;
                if (!is_skin(x, y)) continue;
                static const int IRON[8] = { 0, 4, 7, -1, -1, -1, 10, 13 };   /* shadow by the trim, the inlay, a lit edge */
                uint8_t c = IRON[dy] >= 0 ? (uint8_t)(R_IRON + IRON[dy]) : ramp_at(cr, dy == 3 ? 9 : (dy == 4 ? 13 : 11));
                if (dy >= 6 && x % 10 == 5) c = (uint8_t)(R_GOLD + (dy == 7 ? 12 : 9));   /* rivets */
                hand_buf[y * HAND_W + x] = c;
            }
        }
    }
    /* the ring: a band across the finger, a stone set in it */
    if (ring->kind) {
        static const int METAL[NQUAL] = { R_IRON, R_BONE, R_BONE, R_GOLD, R_GOLD };
        int q = ring->quality, wide = q >= Q_ULTRA ? 2 : 1, metal = METAL[q];
        float ux = 0.29f, uy = 0.96f, vx = 0.96f, vy = -0.29f;      /* along the finger, and across it */
        float cx = (float)RING_AT[0], cy = (float)RING_AT[1];
        for (int a = -wide; a <= 1; a++)
            for (int side = -1; side <= 1; side += 2)
                for (int sd = side < 0 ? 0 : 1; sd <= 7; sd++) {
                    float s = (float)(side * sd);
                    int x = (int)lroundf(cx + vx * s + ux * a), y = (int)lroundf(cy + vy * s + uy * a);
                    if (!is_skin(x, y) || crawl_hand_img[y * HAND_W + x] <= R_SKIN + 1) break;   /* the edge of the finger */
                    float lit = 0.5f + 0.5f * cosf(s * 0.25f + 0.6f) - (a == 1 ? 0.25f : 0.0f) - (a == -wide ? 0.1f : 0.0f);
                    hand_buf[y * HAND_W + x] = ramp_at(metal, clampi((int)(4 + lit * 10), 2, 14));
                }
        if (q > Q_COMMON) {
            static const int GEM[NQUAL] = { 0, R_MOSS, R_GLOW, R_PURPLE, R_BLOOD };
            int gx = (int)lroundf(cx - ux * 0.5f), gy = (int)lroundf(cy - uy * 0.5f), r = q >= Q_ULTRA ? 2 : 1;
            for (int y = -r; y <= r; y++)
                for (int x = -r; x <= r; x++)
                    if (abs(x) + abs(y) <= r + (r > 1)) hand_buf[(gy + y) * HAND_W + gx + x] = (uint8_t)(GEM[q] + 9 + (x < 0) * 3 + (y < 0) * 2);
            hand_buf[(gy - r) * HAND_W + gx - (r > 1)] = C_WHITE;
        }
    }
    /* rune tattoos */
    for (int k = 0; k < NDEF; k++) {
        if (g->def[k] <= 0) continue;
        for (int y = 0; y < 7; y++)
            for (int x = 0; x < 5; x++) {
                if (RUNE_GLYPH[k][y][x] != '#') continue;
                int hx = RUNE_AT[k][0] - 2 + x + (6 - y) / 3, hy = RUNE_AT[k][1] - 3 + y;   /* slanted with the hand */
                if (!is_skin(hx, hy)) continue;
                hand_buf[hy * HAND_W + hx] = (uint8_t)(R_IRON + 4);
                hand_rune[hy * HAND_W + hx] = (uint8_t)(1 + k);
            }
    }
    memcpy(hand_worn, g->worn, sizeof hand_worn); memcpy(hand_def, g->def, sizeof hand_def); hand_built = 1;
    memset(hand_fr_ok, 0, sizeof hand_fr_ok);
}

/* which frame the hand is in: drawn in as a cast begins, a claw while it gathers (or a charge is held), flung open as it
 * goes, easing after; otherwise idling, the fingers settling and lifting a little */
static int hand_frame(void) {
    float ct = -1.0f, fire = 0.0f, len = 0.0f, up = 0.0f;
    if (G.chg_on) return HF_CLAW;
    if (G.state == PS_CAST) { ct = G.st; fire = G.cast_wind; len = G.cast_len; up = fminf(0.2f, fire); }
    else if (G.player && G.pc_t >= 0.0f) { ct = G.pc_t; fire = G.pc_wind; len = G.pc_len; up = fminf(0.18f, fire); }
    if (ct >= 0.0f) {
        if (ct < fire) return ct < up ? HF_GATHER : HF_CLAW;
        if (ct < fire + 0.1f) return HF_LOOSE;
        if (ct < len) return HF_AFTER;
    }
    static const int8_t IDLE[4] = { HF_REST, HF_IDLE1, HF_IDLE2, HF_IDLE1 };
    return IDLE[(int)(G.t / 0.34) & 3];
}

/* The charging effect (docs/PLAN-b11.md, section 7): by the hand, a gathering in the element's colour, turning slowly
 * and quicker as the charge fills; full, it pulses. Embers swirl in, flakes are drawn together, threads crackle, drops
 * and bubbles, violet wisps, splinters of bone */
static void render_charge(Frame *f, int ox, int oy) {
    if (!G.chg_on || !spell_holds(G.chg_sp) || G.chg_t < 0.12f) return;
    int el = ATK_ELEM[G.chg_sp];
    if (el < 0) return;
    float k = clampf(G.chg_t / hold_span(G.chg_sp), 0.0f, 1.0f), t = (float)G.t, full = k >= 0.999f;   /* Burning Intensity's over three s */
    float cx = (float)ox - 20.0f, cy = (float)oy + 46.0f, R = 16.0f + 9.0f * k, spin = 0.8f + 5.0f * k;   /* left of the palm, under the fingers */
    int ramp = ELEM_RAMP[el];
    float pulse = full ? 0.5f + 0.5f * sinf(t * 9.0f) : 0.0f;
    float core = 2.0f + 4.5f * k + 1.5f * pulse;                   /* the heart of it */
    for (int y = (int)(cy - core - 1); y <= (int)(cy + core + 1); y++)
        for (int x = (int)(cx - core - 1); x <= (int)(cx + core + 1); x++) {
            if (x < 0 || x >= W || y < 0 || y >= H) continue;
            float d = hypotf(x + 0.5f - cx, y + 0.5f - cy) / core;
            if (d < 1.0f) f->pix[y * W + x] = d < 0.35f && k > 0.5f ? (uint8_t)C_WHITE : ramp_at(ramp, clampi((int)((1.1f - d * 0.7f) * 15.0f), 4, 15));
        }
    int np = 14 + (int)(12.0f * k);
    for (int i = 0; i < np; i++) {
        float h = (float)(hash2(i, 77) % 1000) / 1000.0f, h2 = (float)(hash2(i, 91) % 1000) / 1000.0f;
        float ph = t * (0.4f + 0.9f * k) + h, u = ph - floorf(ph);  /* each comes in from the rim, again and again */
        float r = R * (1.0f - u) + core, a = h2 * TAU_F + t * spin * (el == EL_FROST || el == EL_PHYS ? 0.3f : 1.0f) + u * 2.0f;
        float px = cx + cosf(a) * r, py = cy + sinf(a) * r * 0.8f;
        uint8_t c = ramp_at(ramp, clampi((int)(9.0f + 6.0f * u), 6, 15));
        int ix = (int)px, iy = (int)py;
        switch (el) {
        case EL_FIRE:                                              /* embers, each with a spark behind it */
            if (ix >= 0 && ix < W && iy >= 0 && iy < H) f->pix[iy * W + ix] = c;
            px -= cosf(a + 1.2f) * 1.5f; py -= sinf(a + 1.2f) * 1.2f;
            if ((int)px >= 0 && (int)px < W && (int)py >= 0 && (int)py < H) f->pix[(int)py * W + (int)px] = ramp_at(ramp, 6);
            break;
        case EL_FROST:                                             /* flakes: a little cross each */
            for (int d = -1; d <= 1; d++) {
                if (ix + d >= 0 && ix + d < W && iy >= 0 && iy < H) f->pix[iy * W + ix + d] = d ? c : (uint8_t)C_WHITE;
                if (ix >= 0 && ix < W && iy + d >= 0 && iy + d < H) f->pix[(iy + d) * W + ix] = d ? c : (uint8_t)C_WHITE;
            }
            break;
        case EL_LIGHT: {                                           /* threads crackling from the heart out */
            if (i % 3) break;
            float x0 = cx, y0 = cy;
            uint32_t jr = hash2(i, (int)(t * 18.0f));
            for (int s = 1; s <= 4; s++) {
                float ra = a + ((float)(rng_next(&jr) % 100) / 100.0f - 0.5f) * 1.1f, l = r * (float)s / 4.0f;
                float x1 = cx + cosf(ra) * l, y1 = cy + sinf(ra) * l * 0.8f;
                for (int q = 0; q <= 4; q++) {
                    int lx = (int)(x0 + (x1 - x0) * q / 4.0f), ly = (int)(y0 + (y1 - y0) * q / 4.0f);
                    if (lx >= 0 && lx < W && ly >= 0 && ly < H) f->pix[ly * W + lx] = s < 3 ? (uint8_t)C_WHITE : c;
                }
                x0 = x1; y0 = y1;
            }
            break;
        }
        case EL_POISON:                                            /* droplets falling in, bubbles rising off */
            if (i & 1) { py = cy - (R + 4.0f) * u + 3.0f; px = cx + sinf(h2 * 13.0f + t * 2.0f) * (R * 0.6f); iy = (int)py; ix = (int)px;
                         for (int d = 0; d < 4; d++) { int bx = ix + (d & 1), by = iy + (d >> 1);
                             if (bx >= 0 && bx < W && by >= 0 && by < H && d != 3) f->pix[by * W + bx] = ramp_at(ramp, 12); } }
            else for (int d = 0; d < 2; d++) if (ix >= 0 && ix < W && iy + d >= 0 && iy + d < H) f->pix[(iy + d) * W + ix] = c;
            break;
        case EL_SPIRIT:                                            /* wisps: a tail curling behind each */
            for (int q = 0; q < 4; q++) {
                float qa = a - (float)q * 0.18f, qr = r + (float)q * 0.8f;
                int qx = (int)(cx + cosf(qa) * qr), qy = (int)(cy + sinf(qa) * qr * 0.8f);
                if (qx >= 0 && qx < W && qy >= 0 && qy < H) f->pix[qy * W + qx] = ramp_at(ramp, 15 - q * 3);
            }
            break;
        default: {                                                 /* splinters of bone, pointing in */
            float ex = cosf(a) * 2.5f, ey = sinf(a) * 2.0f;
            for (int q = 0; q <= 3; q++) {
                int qx = (int)(px + ex * (float)q / 3.0f), qy = (int)(py + ey * (float)q / 3.0f);
                if (qx >= 0 && qx < W && qy >= 0 && qy < H) f->pix[qy * W + qx] = ramp_at(ramp, q ? 9 + q * 2 : 15);
            }
            break;
        }
        }
    }
    if (full) {                                                    /* full: a ring going out with the pulse */
        float rr = core + 2.0f + 4.0f * (1.0f - pulse);
        for (int q = 0; q < 48; q++) {
            int qx = (int)(cx + cosf((float)q * TAU_F / 48.0f) * rr), qy = (int)(cy + sinf((float)q * TAU_F / 48.0f) * rr * 0.8f);
            if ((q + (int)(t * 20.0f)) & 1 && qx >= 0 && qx < W && qy >= 0 && qy < H) f->pix[qy * W + qx] = ramp_at(ramp, 10);
        }
    }
}

/* Restoration (1.0.0b20, PLAN-combat 10.3; the dead's track's, D5): while it is channelled, a pale glow climbs the arm from the
 * fingers into the sleeve, a band at a time, the hand's own shading kept under it, and motes rise off the fingertips. Drawn
 * over the hand as it was drawn, from its frame's pixels. restore_glow: how much (G.rst_k, 0 none); CRAWL_RESTSHOW=1 (debug)
 * at full, for the sheet */
static float restore_glow(void) {
#ifdef CRAWL_DEBUG
    if (getenv("CRAWL_RESTSHOW")) return 1.0f;
#endif
    return G.rst_k;
}
static void render_restore(Frame *f, int ox, int oy, int hf) {
    float k = restore_glow();
    if (k <= 0.0f) return;
    const uint8_t *hb = hand_fr[hf];
    int ty = hand_tip[hf][1], tx = hand_tip[hf][0];
    float t = (float)G.t;
    for (int y = 0; y < HAND_H; y++) {
        int sy = oy + y;
        if (sy < 0 || sy >= H) continue;
        float u = clampf((float)(y - ty) / (float)imax(1, HAND_H - ty), 0.0f, 1.0f);   /* 0 at the fingers, 1 down in the sleeve */
        float band = 0.0f;
        for (int b = 0; b < 3; b++) {                              /* three bands going down the arm, into the caster */
            float c = fmodf(t * 0.6f + (float)b / 3.0f, 1.0f) * 1.15f - 0.05f, d = (u - c) / 0.07f;
            band = fmaxf(band, expf(-d * d));
        }
        float glow = k * (0.14f * (1.0f - u) * (1.0f - u) + 0.62f * band * (1.0f - 0.6f * u));
        int cover = (int)(glow * 16.0f + 0.5f);                    /* a dithered wash: the skin shows through it */
        if (cover <= 0) continue;
        for (int x = 0; x < HAND_W; x++) {
            uint8_t c = hb[y * HAND_W + x];
            if (c == C_CLEAR) continue;
            int sx = ox + x;
            if (sx < 0 || sx >= W || BAYER[sy & 3][sx & 3] >= cover) continue;
            int lum = (crawl_pal[c][0] * 3 + crawl_pal[c][1] * 6 + crawl_pal[c][2]) * 15 / 630;
            f->pix[sy * W + sx] = ramp_at(R_GHOST, 5 + lum * 2 / 3);
        }
    }
    for (int i = 0; i < 9; i++) {                                  /* motes off the fingertips, rising and thinning */
        uint32_t h = (uint32_t)i * 2654435761u;
        float life = 0.9f + 0.6f * (float)(h >> 8 & 255) / 255.0f, ph = fmodf(t / life + (float)(h >> 16 & 255) / 255.0f, 1.0f);
        float mx = (float)(ox + tx) + ((float)(h & 255) / 255.0f - 0.5f) * 34.0f + 3.0f * sinf(t * 2.3f + (float)i);
        float my = (float)(oy + ty) + 14.0f - ph * 30.0f;
        int lv = (int)(k * 15.0f * (1.0f - ph) * fminf(1.0f, ph * 6.0f));
        if (lv < 4) continue;
        int px = (int)mx, py = (int)my;
        for (int q = 0; q < 5; q++) {
            int qx = px + (q == 1) - (q == 2), qy = py + (q == 3) - (q == 4);
            if (qx >= 0 && qx < W && qy >= 0 && qy < H && (q == 0 || lv >= 9)) f->pix[qy * W + qx] = ramp_at(R_GHOST, q == 0 ? lv : lv / 2 + 2);
        }
    }
}
static void render_hand(Frame *f) {
    float hand = hand_raise();
#ifdef CRAWL_DEBUG
    if (getenv("CRAWL_SHOWHAND")) { hand = 1.0f; G.rune_glow[atoi(getenv("CRAWL_SHOWHAND")) % NDEF] = 1.0f; }
#endif
    if (hand <= 0.0f) return;
    if (!hand_built || memcmp(hand_worn, RUN.gear.worn, sizeof hand_worn) || memcmp(hand_def, RUN.gear.def, sizeof hand_def)) build_hand();
    int hf = hand_frame();
#ifdef CRAWL_DEBUG
    if (getenv("CRAWL_HANDFRAME")) hf = clampi(atoi(getenv("CRAWL_HANDFRAME")), 0, NHF - 1);
#endif
    if (!hand_fr_ok[hf]) { crawl_hand_frame(hf, hand_buf, hand_rune, hand_fr[hf], hand_frl[hf], hand_tip[hf]); hand_fr_ok[hf] = 1; }
    const uint8_t *hbuf = hand_fr[hf], *hrune = hand_frl[hf];
    float fox, foy;
    hand_origin(hand, &fox, &foy);
    int ox = (int)fox + crawl_hand_shift[hf][0], oy = (int)foy + crawl_hand_shift[hf][1];
    float light = fminf(lm_at(G.px, G.py) + 0.6f * hand, 1.25f);
    static uint8_t grey[256]; static int grey_built;
    if (!grey_built) {                                             /* each colour as a grey of its brightness */
        for (int c = 0; c < 256; c++) grey[c] = (uint8_t)(R_STONE + clampi((crawl_pal[c][0] * 3 + crawl_pal[c][1] * 6 + crawl_pal[c][2]) * 15 / 630, 0, 15));
        grey_built = 1;
    }
    int greyed = G.silenced || af_on(AF_UNBOUND);
    oy += (int)(fmaxf(G.sil_k, af_on(AF_UNBOUND) ? 1.0f : 0.0f) * 18.0f);   /* the void, or Unbound: the hand drops */
    oy += (int)(G.af_k * 90.0f);                                   /* knocked down: out of sight */
    if (G.af_shake > 0.0f && af_on(AF_DOWN)) { ox += (int)(4.0f * sinf((float)G.t * 23.0f)); oy -= 6; }   /* grappled: it shoves */
    for (int y = 0; y < HAND_H; y++) {
        int sy = oy + y;
        if (sy < 0 || sy >= H) continue;
        for (int x = 0; x < HAND_W; x++) {
            uint8_t c = hbuf[y * HAND_W + x];
            if (c == C_CLEAR) continue;
            int sx = ox + x;
            if (sx < 0 || sx >= W) continue;
            int rk = hrune[y * HAND_W + x];
            if (rk && G.rune_glow[rk - 1] > 0.0f) {                    /* the rune burning as its spell goes off */
                static const int GLOW[NDEF] = { R_GLOW, R_GLOW, R_GLOW, R_GOLD, R_MOSS, R_GLOW, R_FLAME, R_GLOW, R_GLOW };
                float k = fminf(1.0f, G.rune_glow[rk - 1] / 0.8f);
                f->pix[sy * W + sx] = ramp_at(GLOW[rk - 1], (int)(6 + 9 * k));
                continue;
            }
            float lit = light;
            if (G.wwalk_t > 0.0f && ((sx + sy + (int)(G.t * 20.0)) & 1)) continue;   /* Wraithwalk: the hand half there */
            f->pix[sy * W + sx] = shade(greyed || G.wwalk_t > 0.0f ? grey[c] : c, lit, sx, sy);
        }
    }
    render_charge(f, ox, oy);
    render_restore(f, ox, oy, hf);                                 /* Restoration channelled (D5) */
    float ct = -1.0f, fire = 0.0f;                                 /* how far into a cast, and when it lets go */
    if (G.state == PS_CAST) { ct = G.st; fire = G.cast_wind; }
    else if (G.player && G.pc_t >= 0.0f) { ct = G.pc_t; fire = G.pc_wind; }
    if (ct >= 0.0f && ct < fire + 0.03f) {                         /* the spell forming over the fingertips */
        static const int RAMP[NATK] = { R_FLAME, R_GLOW, R_GOLD, R_BONE, R_PURPLE, R_FLAME, R_PURPLE, R_MOSS, R_PURPLE };   /* the element's colour */
        static const float SIZE[NATK] = { 1.0f, 0.8f, 0.8f, 0.7f, 0.9f, 1.35f, 0.75f, 0.85f, 1.2f };
        int sp = clampi(G.spell, 0, NATK - 1), ramp = RAMP[sp];
        float r = (3.0f + 7.0f * clampf(ct / (fire * 0.8f), 0.0f, 1.0f)) * SIZE[sp];
        int cx = ox + hand_tip[hf][0], cy = oy + hand_tip[hf][1];  /* between the tallest fingertips, as this frame has them */
        uint32_t jr = (uint32_t)(G.t * 40.0) * 2246822519u;
        for (int y = (int)(cy - r - 2); y <= (int)(cy + r + 2); y++)
            for (int x = (int)(cx - r - 2); x <= (int)(cx + r + 2); x++) {
                if (x < 0 || x >= W || y < 0 || y >= H) continue;
                float d = hypotf(x + 0.5f - cx, y + 0.5f - cy) / r;
                if (sp == AT_BFS) {                                /* a hole in the air with a violet rim */
                    if (d < 0.8f) f->pix[y * W + x] = 0;
                    else if (d < 1.15f) f->pix[y * W + x] = (uint8_t)(R_PURPLE + 15 - (int)((d - 0.8f) * 20));
                    continue;
                }
                if (sp == AT_CHAIN && d > 0.6f && d < 1.3f && (rng_next(&jr) & 7) == 0) { f->pix[y * W + x] = C_WHITE; continue; }   /* crackle */
                if (d > 1.0f) continue;
                f->pix[y * W + x] = ramp_at(ramp, clampi((int)((1.05f - d * 0.8f) * 15), 2, 15));
            }
    }
}

/* Soul Sap: the bat across the lower part of the view, wings beating, slipping as it loosens; the edges of sight violet */
static void render_latch(Frame *f) {
    if (!G.latch) return;
    const Mob *m = &G.mobs[G.latch - 1];
    float t = (float)G.t, need = SHAKE_NEED[clampi(m->rank, 0, NRANK - 1)], k = need > 0.0f ? clampf(G.shake / need, 0.0f, 1.0f) : 0.0f;
    int view = H - (G.bar > 0.0f ? (int)(44.0f * G.bar) : 0);
    for (int y = 0; y < view; y++)                                 /* the edges tinged violet, pulsing with its feeding */
        for (int x = 0; x < W; x++) {
            int e = imin(imin(x, W - 1 - x), imin(y, view - 1 - y));
            if (e > 16) { x = W - 18; continue; }
            float a = (1.0f - (float)e / 17.0f) * (0.55f + 0.2f * sinf(t * 5.0f));
            if (a * 16.0f > BAYER[y & 3][x & 3] + 0.5f) f->pix[y * W + x] = (uint8_t)(R_PURPLE + 3 + (e < 4 ? 3 : 0));
        }
    int fr = ((int)(t * 11.0f)) & 1, spr = fr ? SP_BAT_B : SP_BAT_A, sc = 5;   /* wings from edge to edge */
    const uint8_t *img = crawl_spr[spr], *lut = m->rank != RK_I && KINDS[m->type].family >= 0 ? crawl_rank_lut[KINDS[m->type].family][m->rank] : NULL;
    int dix = crawl_dress_ix[spr][clampi(m->rank, 0, NRANK - 1)];
    const uint8_t *ov = dix >= 0 ? crawl_dress[dix] : NULL;        /* its rank's marks, as in the crypt */
    int ox = W / 2 - TS * sc / 2 + (int)(sinf(t * 37.0f) * 7.0f * G.slip_k) + (int)(sinf(t * 2.1f) * 4.0f);
    int oy = view - 26 - (TS / 2) * sc + (int)(k * 34.0f + G.slip_k * 8.0f);   /* its body low in the view, lower as its grip goes */
    for (int xx = 0; xx < TS * sc; xx++) {
        int sx = ox + xx;
        if (sx < 0 || sx >= W) continue;
        for (int yy = 0; yy < TS * sc; yy++) {
            int sy = oy + yy;
            if (sy < 0 || sy >= view) continue;
            uint8_t c = img[(xx / sc) * TS + yy / sc], o = ov ? ov[(xx / sc) * TS + yy / sc] : (uint8_t)C_CLEAR;
            if (o != C_CLEAR) c = o;
            else if (c == C_CLEAR) continue;
            else if (lut) c = lut[c];
            f->pix[sy * W + sx] = shade(c, 0.95f, sx, sy);        /* lit by your own glow, this close */
        }
    }
}
/* souls slipping from a full vessel: a pale wisp rising off the hand, or from the foot of the view */
static void render_vslip(Frame *f) {
    if (G.vslip_t <= 0.0f) return;
    float k = 1.0f - G.vslip_t, ox, oy;
    hand_origin(fmaxf(hand_raise(), 0.25f), &ox, &oy);
    float cx = ox + 30.0f + 6.0f * sinf(k * 9.0f), cy = fminf(oy, (float)(H - 40)) - k * 60.0f;
    for (int t = 0; t < 5; t++) {                                  /* a tail of fading puffs */
        float px = cx - 2.0f * sinf(k * 9.0f - (float)t), py = cy + (float)t * 4.0f, r = 3.0f - 0.5f * (float)t;
        for (int y = (int)(py - r); y <= (int)(py + r); y++)
            for (int x = (int)(px - r); x <= (int)(px + r); x++)
                if (x >= 0 && x < W && y >= 0 && y < H && hypotf(x + 0.5f - px, y + 0.5f - py) <= r && BAYER[y & 3][x & 3] < (int)(G.vslip_t * 16.0f))
                    f->pix[y * W + x] = (uint8_t)(R_GHOST + 14 - t * 2);
    }
}
/* a soft ball of wisp-light: pass 0 lays down its dithered halo, pass 1 the ball, white at the heart */
static int wisp_ramp = R_GLOW;                                     /* the colour of the wisp being drawn */
/* a glowing ball on the screen: pass 0 its dithered halo, pass 1 its body; depth hides it behind the walls */
static void wisp_ball(Frame *f, float cx, float cy, float r, float a, int pass, float depth) {
    float reach = pass ? 1.0f : 2.0f;
    int x0 = (int)floorf(cx - r * reach), x1 = (int)ceilf(cx + r * reach), y0 = (int)floorf(cy - r * reach), y1 = (int)ceilf(cy + r * reach);
    for (int x = x0 < 0 ? 0 : x0; x <= x1 && x < W; x++) {
        if (depth > zbuf[x]) continue;
        for (int y = y0 < 0 ? 0 : y0; y <= y1 && y < H; y++) {
            float d = hypotf(x + 0.5f - cx, y + 0.5f - cy) / r;
            if (pass) {
                if (d >= 1.0f) continue;
                f->pix[y * W + x] = d < 0.42f && r > 1.6f ? C_WHITE : (uint8_t)(wisp_ramp + 15 - (int)(d * 6.0f));
            } else if (d >= 1.0f && d < 2.0f) {
                float k = (2.0f - d) * a;
                if (k * 16.0f > BAYER[y & 3][x & 3] + 0.5f) f->pix[y * W + x] = (uint8_t)(wisp_ramp + 6 + (int)(k * 4.0f));
            }
        }
    }
}

/* the wisps in the crypt: the motes they left hanging, then each body, its flame licking back the way
 * it came (or up, while it hovers) */
static void render_wisps(Frame *f, float dx, float dy, float horizon) {
    float t = (float)G.t;
    if (G.cshown && !PK.depth)                                     /* a corrupted level's secrets, shown: a violet wisp at each shut */
        for (int i = 0; i < G.nsecrets; i++) {
            const Secret *sc = &G.secrets[i];
            float sx, sy, dp;
            if (sc->state || !project(sc->fx + 0.5f, sc->fy + 0.5f, 0.42f + 0.05f * sinf(t * 2.0f + (float)i), dx, dy, horizon, &sx, &sy, &dp) || dp < 0.35f) continue;
            float s = clampf(FOCAL / dp * 0.009f, 0.2f, 2.0f), flick = 1.0f + 0.08f * sinf(t * 13.0f + (float)i);
            wisp_ramp = R_PURPLE;
            for (int pass = 0; pass < 2; pass++) wisp_ball(f, sx, sy, 5.0f * s * flick, 0.8f, pass, dp);
        }
    for (int i = 0; i < MAXWISPS; i++) {
        const Wisp *w = &G.wisps[i];
        if (!w->on) continue;
        wisp_ramp = wisp_ramp_of(w);
        for (int k = 0; k < WISP_MOTES; k++) {
            const Mote *m = &w->motes[k];
            float age = t - m->t, sx, sy, dp;
            if (m->t <= 0.0f || age < 0.0f || age > WISP_MOTE_T) continue;
            if (!project(m->x, m->y, m->z + 0.03f * sinf(t * 1.3f + k), dx, dy, horizon, &sx, &sy, &dp) || dp < 0.6f) continue;   /* not those overhead */
            int x = (int)sx, y = (int)sy;
            if (x < 0 || x >= W || y < 0 || y >= H || dp > zbuf[x]) continue;
            float k01 = 1.0f - age / WISP_MOTE_T, tw = 0.5f + 0.5f * sinf(t * 5.0f + k * 1.7f);
            if (k01 < 0.4f && BAYER[y & 3][x & 3] >= (int)(k01 / 0.4f * 16.0f)) continue;   /* thinning out as it goes */
            float r = clampf(0.03f * FOCAL / dp, 0.6f, 3.5f) * (0.55f + 0.45f * k01) * (0.85f + 0.3f * tw);
            if (r < 1.1f) { f->pix[y * W + x] = (uint8_t)(wisp_ramp + 8 + (int)(k01 * 5.0f + tw * 2.0f)); continue; }
            if (k01 > 0.75f) wisp_ball(f, sx, sy, r, 0.5f * k01, 0, dp);   /* fresh ones with a glow about them */
            wisp_ball(f, sx, sy, r, 1.0f, 1, dp);
        }
        float sx, sy, dp, e = clampf(w->a, 0.0f, 1.0f);
        if (w->a <= 0.0f || !project(w->x, w->y, w->z, dx, dy, horizon, &sx, &sy, &dp) || dp < 0.35f) continue;
        float flick = 1.0f + 0.09f * sinf(t * 17.0f + i) + 0.06f * sinf(t * 29.0f + 1.3f);
        float s = clampf(FOCAL / dp * 0.011f, 0.22f, 2.6f) * (0.35f + 0.65f * e) * (w->state == WS_GONE ? fmaxf(w->a, 0.05f) : 1.0f);
        float tdx = 0.0f, tdy = -1.0f, bx, by, bd;                 /* which way its flame streams */
        if (hypotf(w->vx, w->vy) > 0.3f && project(w->x - w->vx * 0.2f, w->y - w->vy * 0.2f, w->z, dx, dy, horizon, &bx, &by, &bd)) {
            float l = hypotf(bx - sx, by - sy);
            if (l > 0.5f) { tdx = (bx - sx) / l; tdy = (by - sy) / l; }
        }
        if (w->kind == WK_EXIT && G.blessed) {                     /* blessed: a pale gold halo about it */
            wisp_ramp = R_GOLD + 4;                                /* the halo's colours, from the pale end of the gold */
            wisp_ball(f, sx, sy, 7.5f * s * flick, 0.5f * e, 0, dp);
            wisp_ramp = wisp_ramp_of(w);
        }
        for (int pass = 0; pass < 2; pass++) {
            for (int k = 4; k >= 1; k--) {                         /* tongues of it, licking back */
                float wv = sinf(t * 11.0f + k * 2.1f + i) * 0.9f * k;
                wisp_ball(f, sx + (tdx * 3.4f * k - tdy * wv) * s, sy + (tdy * 3.4f * k + tdx * wv) * s, (5.6f - 1.0f * k) * s * flick, 0.75f * e, pass, dp);
            }
            for (int k = 1; k <= 2; k++) {                         /* and flickering up off its crown */
                float ph = t * (2.3f + 0.7f * k) + k * 1.7f + i, u = ph - floorf(ph);
                wisp_ball(f, sx + sinf(ph * 5.0f) * 1.5f * s, sy - (4.0f + 7.0f * u) * s, (2.4f - 1.6f * u) * s, 0.6f * e * (1.0f - u), pass, dp);
            }
            wisp_ball(f, sx, sy, 6.2f * s * flick, e, pass, dp);
        }
    }
}

