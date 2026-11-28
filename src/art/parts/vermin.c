/* A part of crawl_art.c (docs/PLAN-tidy.md): it is #included there in its place and never compiled on its own.
 * Small things that move (docs/PLAN-vermin.md): the skull hopper, the bile leech and the coin crab. Each is drawn in the canvas's lower
 * half, a tile at the side passages' half size. The second pass (28 November): the hopper is the game's own skull in profile
 * (SKULL_SIDE, doubled), settled and squat, or tipped back in its leap with its jaw dropped; the leech ringed and striped, a toothed
 * sucker at its head and a flared one at its tail; the crab a milled coin with a face struck on it, on jointed legs, its pincers up. */

static uint8_t g_hop[64 * 64];                                     /* the skull, doubled, before it is tipped onto the canvas */
static void hop_double(const char *const *g, int r0, int r1, int ox, int oy, int *eyex, int *eyey) {   /* rows r0..r1, EPX at 2x */
    #define HG(yy, xx) ((yy) < r0 || (yy) > r1 || (xx) < 0 || (xx) >= SKULL_W ? '.' : g[yy][xx])
    #define HC(ch) ((ch) == '.' ? (uint8_t)C_CLEAR : ((ch) == '0' || (ch) == '@') ? (uint8_t)0 : (uint8_t)(R_BONE + ((ch) <= '9' ? (ch) - '0' : (ch) - 'a' + 10)))
    for (int y = r0; y <= r1; y++)
        for (int x = 0; x < SKULL_W; x++) {
            uint8_t P = HC(HG(y, x)), A = HC(HG(y - 1, x)), B = HC(HG(y, x + 1)), C = HC(HG(y, x - 1)), D = HC(HG(y + 1, x)), o[4] = { P, P, P, P };
            if (C == A && C != D && A != B) o[0] = A;
            if (A == B && A != C && B != D) o[1] = B;
            if (D == C && D != B && C != A) o[2] = C;
            if (B == D && B != A && D != C) o[3] = D;
            for (int k = 0; k < 4; k++) {
                int px = ox + x * 2 + (k & 1), py = oy + (y - r0) * 2 + (k >> 1);
                if ((unsigned)px < 64u && (unsigned)py < 64u && o[k] != C_CLEAR) g_hop[py * 64 + px] = o[k];
            }
            if (g[y][x] == '@' && eyex) { *eyex = ox + x * 2 + 1; *eyey = oy + (y - r0) * 2 + 1; }
        }
    #undef HG
    #undef HC
}
static void spr_hopper(int id, int frame) {                        /* 0 settled, squat, grit kicked up; 1 in the leap: tipped back, stretched, its jaw dropped */
    cv_begin(cv_sprite, TS, TS);
    memset(g_hop, C_CLEAR, sizeof g_hop);
    int ex = 0, ey = 0, jaw = frame ? 3 : 0;
    hop_double(SKULL_SIDE, 0, 11, 17, 8, &ex, &ey);                /* the cranium and the face, its eye */
    hop_double(SKULL_SIDE, 12, 15, 17 - (frame ? 1 : 0), 8 + 24 + jaw, NULL, NULL);   /* the jaw, hanging open in the leap */
    if (frame) for (int x = 33; x <= 46; x++) for (int y = 32; y < 32 + jaw; y++) if (g_hop[(y - 1) * 64 + x] != C_CLEAR) g_hop[y * 64 + x] = 0;   /* the mouth's dark */
    float a = frame ? -0.22f : 0.06f, sx = frame ? 0.94f : 1.07f, sy = frame ? 1.10f : 0.90f;   /* tipped back and stretched; nodding, squashed */
    float pcx = 32.0f, pcy = 24.0f, ca = cosf(a), sa = sinf(a);    /* about the skull's middle in g_hop, its foot to the canvas's at 62 */
    float base = frame ? 60.0f : 63.0f, half = 16.0f * sy;
    for (int y = 0; y < TS; y++)
        for (int x = 0; x < TS; x++) {
            float dx = (x + 0.5f - 32.0f) / sx, dy = (y + 0.5f - (base - half)) / sy;
            float u = ca * dx + sa * dy + pcx, v = -sa * dx + ca * dy + pcy;
            int iu = (int)floorf(u), iv = (int)floorf(v);
            if ((unsigned)iu >= 64u || (unsigned)iv >= 64u) continue;
            uint8_t c = g_hop[iv * 64 + iu];
            if (c == C_CLEAR) continue;
            if (c > R_BONE && c < R_BONE + 16) {                   /* the bone's grain, finer than its pixels; grave-earth on its underside */
                float r = h01(iu, iv, 371 + frame), n = fbm(iu / 4.0f, iv / 4.0f, 16, 373);
                int st = c - R_BONE + (r < 0.10f ? -1 : r > 0.94f ? 1 : 0) - (n < 0.28f ? 1 : 0);
                if (iv > 35 && h01(iu, iv, 377) < 0.12f + (iv - 35) * 0.05f) { cv_px(x, y, (uint8_t)(R_WARM + clampi(st / 2, 2, 6))); continue; }
                c = (uint8_t)(R_BONE + clampi(st, 1, 15));
            }
            cv_px(x, y, c);
        }
    { float dx = ex + 0.5f - pcx, dy = ey + 0.5f - pcy;            /* the eye, where the tipping took it: a coal in the socket */
      float x = 32.0f + sx * (ca * dx - sa * dy), y = base - half + sy * (sa * dx + ca * dy);
      cv_flat(x, y, 1.6f, 1.6f, (uint8_t)(R_FLAME + 7)); cv_px((int)x, (int)y, (uint8_t)(R_FLAME + 13)); cv_eye_mark((int)x, (int)y); }
    static const float CR[2][3] = { { 29, 1.2f, 9 }, { 22, 0.5f, 6 } };   /* a crack down from the crown, one from the back */
    for (int k = 0; k < 2; k++) {
        float x = CR[k][0], y = 0.0f, d = CR[k][1];
        while (y < TS - 1 && cv_sprite[(int)y * TS + (int)x] == C_CLEAR) y += 1.0f;
        y += 1.0f;
        for (int i = 0; i < (int)CR[k][2]; i++) {
            d += (h01(k, i, 379) - 0.5f) * 1.0f; x += cosf(d); y += sinf(d);
            uint8_t c = cv_sprite[(int)y * TS + (int)x];
            if (c > R_BONE + 4 && c < R_BONE + 16) cv_px((int)x, (int)y, (uint8_t)(R_BONE + 2));
        }
    }
    for (int k = 0; k < 4; k++) {                                  /* moss in the back of the crown, where it lay */
        int x = 20 + (int)(h01(k, 1, 381) * 5), y = (frame ? 22 : 30) + (int)(h01(k, 2, 381) * 4);
        if (cv_sprite[y * TS + x] != C_CLEAR && cv_sprite[y * TS + x] != 0) cv_px(x, y, (uint8_t)(R_MOSS + 5 + (k & 1) * 3));
    }
    cv_outline(0);
    if (!frame) for (int s = -1; s <= 1; s += 2) for (int k = 0; k < 3; k++)   /* grit thrown out as it lands */
        cv_px(32 + s * (17 + k * 2 + (int)(h01(k, s + 2, 383) * 2)), 62 - k - (k == 2), sh(R_STONE, 0.45f + 0.12f * k));
    else for (int k = 0; k < 3; k++) cv_line(13 - k, 52 + k * 3, 17 - k, 50 + k * 3, sh(R_STONE, 0.35f - 0.08f * k));   /* the air it leaves */
    cv_store(id);
}

/* the leech: a spine from its tail (left) to its head (right), the body a run of discs along it, each pixel shaded by which side of the
 * spine it lies and how far out; rings every few pixels along it, a stripe of sallow spots down its back, a pale belly; a flared sucker
 * at the tail, a round mouth at the head (shut lying, gaping in the leap, its teeth a ring of bone) */
static void spr_leech(int id, int frame) {
    enum { N = 48 };
    float sx[N], sy[N], sr[N], arc[N];
    cv_begin(cv_sprite, TS, TS);
    for (int i = 0; i < N; i++) {
        float t = (float)i / (N - 1);
        sx[i] = 11.0f + t * 40.0f;
        sy[i] = frame ? 58.0f - 24.0f * sinf(t * PI_F * 0.92f + 0.10f) + 6.0f * t : 57.5f - 1.8f * sinf(t * PI_F * 2.4f + 0.4f);
        sr[i] = 2.6f + 4.4f * sinf(PI_F * powf(t, 0.75f)) + (t < 0.06f ? 1.4f * (1.0f - t / 0.06f) : 0.0f);
        if (frame) sr[i] *= 0.9f;                                  /* drawn thinner, stretched in the leap */
        arc[i] = i ? arc[i - 1] + hypotf(sx[i] - sx[i - 1], sy[i] - sy[i - 1]) : 0.0f;
    }
    if (!frame) cv_flat(32, 62.5f, 18, 1.6f, (uint8_t)(R_MOSS + 2));   /* the slick it lies in */
    for (int y = 20; y < TS; y++)
        for (int x = 2; x < TS - 2; x++) {
            float best = 1e9f, side = 0.0f, along = 0.0f; int bi = -1;
            for (int i = 0; i < N; i++) {
                float d = hypotf(x + 0.5f - sx[i], y + 0.5f - sy[i]) - sr[i];
                if (d < best) { best = d; bi = i; }
            }
            if (best > 0.0f) continue;
            int i0 = bi > 0 ? bi - 1 : bi, i1 = bi < N - 1 ? bi + 1 : bi;
            float tx = sx[i1] - sx[i0], ty = sy[i1] - sy[i0], tl = hypotf(tx, ty) + 1e-4f, nx = ty / tl, ny = -tx / tl;   /* the spine's normal, upward */
            float ox = x + 0.5f - sx[bi], oy = y + 0.5f - sy[bi], v = (ox * nx + oy * ny) / sr[bi];   /* 1 its back's edge, -1 its belly's */
            side = v; along = arc[bi];
            float w = clampf(fabsf(v), 0.0f, 1.0f), z = sqrtf(fmaxf(0.0f, 1.0f - w * w));
            float L = 0.18f + 0.55f * lit(-nx * v * 0.6f, -ny * v, z);
            int st;
            if (v < -0.30f) st = (int)(8 + 5 * L);                 /* the belly, paler and flat */
            else st = (int)(1 + 9 * L);
            if (fmodf(along + 0.6f, 3.2f) < 0.9f && w < 0.95f) st -= 3;   /* its rings */
            if (v > 0.25f && v < 0.62f && fmodf(along, 5.6f) < 1.6f && bi > 4 && bi < N - 4) st = 12 + (fmodf(along, 5.6f) < 0.8f);   /* the stripe's spots */
            if (v > 0.45f && v < 0.7f && h01(x, y, 391) < 0.10f) st = 15;   /* wet: a glint */
            cv_px(x, y, (uint8_t)(R_MOSS + clampi(st, 1, 15)));
            (void)side;
        }
    {                                                              /* the tail's sucker: a flared disc, edge on */
        float tx = sx[0], ty = sy[0];
        cv_ellipse(tx - 1.0f, ty + 0.5f, 2.0f, sr[0] + 1.2f, R_MOSS, 0.05f, 0.35f);
        cv_line(tx - 2.0f, ty - sr[0], tx - 2.0f, ty + sr[0] + 1.0f, (uint8_t)(R_MOSS + 1));
    }
    {                                                              /* the head: the mouth turned to its way */
        float hx = sx[N - 1] + 1.0f, hy = sy[N - 1], ax = sx[N - 1] - sx[N - 4], ay = sy[N - 1] - sy[N - 4], al = hypotf(ax, ay) + 1e-4f;
        ax /= al; ay /= al;
        float r = frame ? 5.0f : 3.8f;
        for (int y = (int)(hy - r - 1); y <= (int)(hy + r + 1); y++)
            for (int x = (int)(hx - r - 1); x <= (int)(hx + r + 1); x++) {
                float px = x + 0.5f - hx, py = y + 0.5f - hy, u = px * ax + py * ay, w = -px * ay + py * ax;   /* u along its way, w across */
                float d = (u * u) / (r * 0.55f * r * 0.55f) + (w * w) / (r * r);
                if (d > 1.0f) continue;
                cv_px(x, y, d > 0.62f ? (uint8_t)(R_BLOOD + 6 + (w < 0)) : d > 0.30f ? (uint8_t)(R_BLOOD + 3) : (uint8_t)0);
            }
        int nt = frame ? 8 : 5;                                    /* its teeth, round the gape */
        for (int k = 0; k < nt; k++) {
            float q = (float)k / nt * 2.0f * PI_F, tw = cosf(q) * r * 0.72f, tu = sinf(q) * r * 0.3f;
            cv_px((int)(hx + tu * ax - tw * ay), (int)(hy + tu * ay + tw * ax), sh(R_BONE, 0.85f + 0.1f * (k & 1)));
        }
        for (int k = 0; k < 2; k++) {                              /* eyespots on its head: two pairs, the first bright */
            float ex = sx[N - 6 - k * 4] - ay * sr[N - 6] * 0.55f, ey = sy[N - 6 - k * 4] + ax * -sr[N - 6] * 0.55f;
            cv_px((int)ex, (int)ey, k ? (uint8_t)(R_BLOOD + 9) : (uint8_t)(R_FLAME + 11));
            if (!k) cv_eye_mark((int)ex, (int)ey);
        }
    }
    cv_outline(0);
    if (frame) for (int k = 0; k < 4; k++) {                       /* bile dripping off it as it goes over */
        float x = sx[10 + k * 9], y = sy[10 + k * 9] + sr[10 + k * 9] + 2.0f + h01(k, 1, 393) * 5.0f;
        cv_px((int)x, (int)y, (uint8_t)(R_MOSS + 12)); cv_px((int)x, (int)y + 1, (uint8_t)(R_MOSS + 9));
        if (k & 1) cv_px((int)x, (int)y - 2, (uint8_t)(R_MOSS + 7));
    }
    cv_store(id);
}

/* the crab: a coin for a shell, milled at its rim, a head struck on it, worn; under it a dark body on six jointed legs, a pincer
 * either side up before it, eyes on stalks over the rim. Still, it is a coin lying on the floor, a leg's tip showing for those who look */
static void crab_coin(float cy, float ry, float tilt) {            /* the coin, its middle at 32, cy; ry its height as seen */
    const float rx = 12.5f;
    for (int y = (int)(cy - ry - 2); y <= (int)(cy + ry + 2); y++)
        for (int x = 18; x <= 46; x++) {
            float nx = (x + 0.5f - 32.0f) / rx, ny = (y + 0.5f - cy) / ry, d = nx * nx + ny * ny;
            if (d > 1.0f) continue;
            float rim = sqrtf(d);
            int k;
            if (rim > 0.84f) {                                     /* the milled edge, a step up and down round it */
                float ang = atan2f(ny, nx);
                k = 6 + (((int)((ang + PI_F) * 9.0f)) & 1) * 2 + (int)(3.0f * lit(nx, ny, 0.4f));
            } else if (rim > 0.74f) k = 4;                         /* the struck ring inside it */
            else {
                float z = sqrtf(fmaxf(0.0f, 1.0f - d * 0.6f));
                k = 6 + (int)(5.0f * lit(nx * 0.4f, ny * 0.4f - tilt, z));
            }
            cv_px(x, y, (uint8_t)(R_GOLD + clampi(k, 0, NGOLD - 1)));
        }
    /* the face struck on it, in profile, worn: a brow, a nose, a lip and a crown's three points over it, raised a step */
    static const char *const HEAD[11] = { "...X.X.X..", "...XXXXX..", "..XXXXXX..", ".XXXXXXXX.", ".XXX.XXXXX", "XXXXXXXXXX",
                                          ".XXXXXXXX.", "..XXXXXX..", "...XXXX...", "...XXXX...", "..XXXXXX.." };
    for (int y = 0; y < 11; y++)
        for (int x = 0; x < 10; x++) {
            int py = (int)floorf(cy + (y - 5.0f) * ry / 7.6f + 0.5f), px = 27 + x;
            if (HEAD[y][x] != 'X') continue;
            uint8_t c = cv_sprite[py * TS + px];
            if (c < R_GOLD || c >= R_GOLD + NGOLD) continue;
            int up = y == 0 || HEAD[y - 1][x] != 'X', lf = x == 0 || HEAD[y][x - 1] != 'X';
            int dn = y == 10 || HEAD[y + 1][x] != 'X', rt = x == 9 || HEAD[y][x + 1] != 'X';
            cv_px(px, py, (uint8_t)(R_GOLD + ((up || lf) ? 11 : (dn || rt) ? 4 : 8)));   /* raised: lit above and left, shadowed below and right */
        }
}
static void spr_crab(int id, int pose) {                           /* pose 0: still, a coin; 1 and 2: running, its legs at one step and the next */
    cv_begin(cv_sprite, TS, TS);
    if (!pose) {
        cv_flat(32, 62.5f, 13, 1.2f, (uint8_t)(R_WARM + 1));      /* its shadow */
        crab_coin(58.5f, 4.4f, 0.6f);
        cv_line(19, 60, 17, 62, sh(R_IRON, 0.35f)); cv_px(16, 62, sh(R_BONE, 0.6f));   /* a leg's tip, for those who look */
        cv_outline(0);
        cv_px(36, 56, C_WHITE); cv_px(37, 57, (uint8_t)(R_GOLD + 12));   /* a glint */
        cv_store(id);
        return;
    }
    int st = pose == 2;
    cv_flat(32, 62.5f, 15, 1.3f, (uint8_t)(R_WARM + 1));
    for (int s = -1; s <= 1; s += 2) for (int l = 2; l >= 0; l--) {   /* six legs, three a side: hip, knee high, foot; one set up as the other bears */
        int up = ((l + st + (s > 0)) & 1);
        float hx = 32 + s * (7 + l * 2.0f), hy = 53 + l * 0.8f;
        float kx = 32 + s * (15 + l * 4.0f), ky = 48 + l * 1.5f - up * 2.5f;
        float fx = 32 + s * (17 + l * 5.0f) + (up ? -s * 1.5f : 0.0f), fy = 62 - up * 2.0f;
        cv_capsule(hx, hy, kx, ky, 1.3f, R_IRON, 0.12f, 0.60f);
        cv_capsule(kx, ky, fx, fy, 1.0f, R_IRON, 0.08f, 0.50f);
        cv_px((int)kx, (int)ky - 1, sh(R_IRON, 0.75f));            /* the knee's sheen */
        cv_px((int)fx, (int)fy, sh(R_BONE, 0.55f));                /* a pale point */
    }
    cv_ellipse(32, 53, 10, 4.5f, R_IRON, 0.04f, 0.35f);            /* its body under the coin, plated */
    for (int k = -2; k <= 2; k++) cv_line(32 + k * 3.0f, 51, 32 + k * 3.2f, 56, sh(R_IRON, 0.12f));
    crab_coin(48.0f - st * 0.5f, 7.6f, 0.2f);                      /* the coin carried as a shell, tipped up */
    for (int s = -1; s <= 1; s += 2) {                             /* the pincers, raised either side of it: an arm, a palm, two fingers */
        float wx = 32 + s * 11, wy = 51, ex = 32 + s * 16, ey = 44, px = 32 + s * 17.5f, py = 37 - st;
        cv_capsule(wx, wy, ex, ey, 1.3f, R_IRON, 0.12f, 0.6f);
        cv_capsule(ex, ey, px, py + 2.5f, 1.4f, R_IRON, 0.15f, 0.65f);
        cv_ellipse(px, py, 3.0f, 2.6f, R_IRON, 0.25f, 0.85f);
        float open = (st ^ (s > 0)) ? 1.2f : 0.2f;
        cv_capsule(px + s * 1.8f, py - 1.5f, px + s * (2.6f + open), py - 6.0f, 1.0f, R_IRON, 0.25f, 0.85f);   /* the finger that moves */
        cv_capsule(px - s * 1.2f, py - 1.8f, px - s * 0.6f, py - 5.5f + open * 0.5f, 0.9f, R_IRON, 0.2f, 0.7f);
        cv_px((int)(px + s * (2.6f + open)), (int)(py - 6.0f), sh(R_BONE, 0.6f));   /* its tips pale */
    }
    for (int s = -1; s <= 1; s += 2) {                             /* eyes on stalks over its rim */
        float ex = 32 + s * 4.0f + st * 0.5f, ey = 37.0f - st * 0.5f;
        cv_line(32 + s * 3.0f, 41, ex, ey + 1, sh(R_IRON, 0.5f));
        cv_flat(ex, ey, 1.3f, 1.3f, (uint8_t)(R_GLOW + 9)); cv_px((int)ex, (int)ey, (uint8_t)(R_GLOW + 14)); cv_eye_mark((int)ex, (int)ey);
    }
    cv_outline(0);
    cv_px(36, 43, C_WHITE);                                        /* the coin's glint */
    for (int k = 0; k < 3; k++) cv_px(14 - k * 3 + st, 61 - (k & 1), sh(R_STONE, 0.4f + 0.1f * k));   /* dust behind */
    cv_store(id);
}
static void vermin_sprites(void) {
    spr_hopper(SP_HOP_A, 0); spr_hopper(SP_HOP_B, 1);
    spr_leech(SP_LEECH_A, 0); spr_leech(SP_LEECH_B, 1);
    spr_crab(SP_CRAB_COIN, 0); spr_crab(SP_CRAB_A, 1); spr_crab(SP_CRAB_B, 2);
}
