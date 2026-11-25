/* A part of crawl_art.c (docs/PLAN-tidy.md): it is #included there in its place and never compiled on its own.
 * The four new hanging lights (docs/PLAN-lights.md; the owner's sheet, media/sheets/lights/hanging_lights.png) and the events' things
 * (docs/PLAN-events.md; media/sheets/events/event_things.png): the iron corona, the sanctuary lamp, the bone candelabrum and the pierced
 * cage lantern by their frames; the barred and unbarred door, the relic, the socket empty and holding, the lever up and down, the pressure
 * plate up and pressed. Made each level in the run's look, as the sheets drew them, from tools/lightmock.c, which now draws its sheets
 * from these slots. */

static int g_lrun;                                      /* the run drawn for: 0 the Crypts, 1 the Catacombs, 2 the Hollows, 3 the Deep */

/* a candle for what hangs: wax_candle's, its wax the run's (the Deep burns black candles, as its altar does) */
static void hang_candle(float x, float by, float h, float r, int frame, int k) {
    int ramp = g_lrun == 3 ? R_IRON : R_BONE;
    float lo = g_lrun == 3 ? 0.10f : 0.42f, hi = g_lrun == 3 ? 0.30f : 0.48f;
    for (int y = (int)(by - h); y <= (int)by; y++)
        for (int xx = (int)(x - r) - 1; xx <= (int)(x + r) + 1; xx++) {
            float nx = ((float)xx + 0.5f - x) / r;
            if (fabsf(nx) > 1.0f) continue;
            cv_px(xx, y, sh(ramp, lo + hi * lit(nx, 0.0f, sqrtf(1.0f - nx * nx))));
        }
    int top = (int)(by - h);
    if (h >= 4.0f) {                                             /* a run of wax down its front */
        int l = 1 + (int)(h01(k, 3, 751) * (h * 0.5f));
        for (int j = 1; j <= l; j++) cv_px((int)x + (k & 1), top + j, sh(ramp, lo + hi + 0.12f - 0.02f * (float)j));
    }
    cv_px((int)x, top - 1, sh(R_IRON, 0.12f));                   /* the wick */
    prop_flame(x + 0.5f, (float)top - 1.0f, 5.0f + (float)((frame + k) % 3 == 0) + 1.0f * h01(k, frame, 753), 1.8f, (((frame + k) % 3) - 1) * 0.6f);
}

/* a fine chain from (x0, y0) to (x1, y1): a link a pixel or two, light and dark by turns */
static void fine_chain(float x0, float y0, float x1, float y1, int ramp, float lo, float hi) {
    int n = (int)fmaxf(fabsf(x1 - x0), fabsf(y1 - y0)) + 1;
    for (int i = 0; i <= n; i++) {
        float t = (float)i / (float)n;
        cv_px((int)(x0 + (x1 - x0) * t), (int)(y0 + (y1 - y0) * t), sh(ramp, ((i / 2) & 1) ? hi : lo));
    }
}

/* the run on a finished light: the Catacombs' rust and drips are prop_wash's, after it is kept; the Hollows' soot, the iron
 * darkened and the wax greyed over each flame; the Deep's iron glassy, a violet glint in it */
static void hang_runlook(uint32_t seed) {
    for (int y = 0; y < TS; y++)
        for (int x = 0; x < TS; x++) {
            uint8_t *c = &cv_sprite[y * TS + x];
            if (*c == C_CLEAR || *c == 0 || IS_FULLBRIGHT(*c)) continue;
            if (*c >= R_GOLD && *c < R_GOLD + NGOLD) {               /* brass: tarnished in the Hollows, blackened in the Deep */
                if (g_lrun >= 2 && h01(x, y, seed + 3) < (g_lrun == 2 ? 0.5f : 0.8f)) *c = (uint8_t)(R_GOLD + clampi(*c - R_GOLD - (g_lrun == 2 ? 2 : 3), 0, NGOLD - 1));
                continue;
            }
            int ramp = (*c - 1) / 16 * 16 + 1, v = *c - ramp;
            if (g_lrun == 2 && ramp == R_IRON && h01(x, y, seed) < 0.45f) *c = (uint8_t)(R_IRON + clampi(v - 3, 1, 15));
            if (g_lrun == 2 && ramp == R_IRON && h01(x, y, seed + 1) < 0.08f) *c = sh(R_BRICK, 0.12f + 0.03f * (float)v);
            if (g_lrun == 3 && ramp == R_IRON && v > 7 && h01(x, y, seed + 2) < 0.1f) *c = sh(R_PURPLE, 0.25f + 0.03f * (float)v);
        }
}
static void hang_wash(int id, uint32_t seed) { if (g_lrun == 1) prop_wash(id, 1, seed); }   /* the Catacombs' water, by prop_wash */

/* ---- the iron corona (halls and naves): a great wheel of wrought iron on four chains from one hook; two bands, a zigzag of
 * bars between them; eight tall spikes barbed like fleurs, eight candles on prickets between them, a drop under each spike;
 * six spokes to a boss and its long finial; wax hanging from the rim. frame 0-2 ---- */
static void spr_ironcorona(int id, int frame) {
    cv_begin(cv_sprite, TS, TS);
    const float cx = 32, cy = 24, rx = 29, ry = 7, ri = 24;
    #define RIMX(a, r) (cx + (r) * cosf(a))
    #define RIMY(a, r) (cy + (r) / rx * ry * sinf(a))
    chain_down(cx, 0, 4);
    cv_ellipse(cx, 5, 2.2f, 1.7f, R_IRON, 0.2f, 0.85f);              /* the hook's ring */
    for (int k = 0; k < 4; k++) {                                    /* the far chains */
        float a = PI_F * 0.25f + (float)k * PI_F * 0.5f;
        if (sinf(a) < 0.0f) fine_chain(cx, 6, RIMX(a, rx - 1), RIMY(a, rx - 1) - 1, R_IRON, 0.1f, 0.3f);
    }
    for (int pass = 0; pass < 3; pass++) {                           /* 0 the far half, 1 the spokes and boss, 2 the near half */
        if (pass == 1) {
            for (int k = 0; k < 6; k++) {
                float a = TAU_F * (float)k / 6.0f + 0.5f, x1 = RIMX(a, ri), y1 = RIMY(a, ri);
                float v = sinf(a) > 0.0f ? 0.55f : 0.25f;
                cv_line(cx, cy, x1, y1, sh(R_IRON, v)); cv_line(cx, cy + 1, x1, y1 + 1, sh(R_IRON, v * 0.4f));
                cv_px((int)x1, (int)y1 - 1, sh(R_IRON, v + 0.3f)); cv_px((int)x1, (int)y1 + 1, sh(R_IRON, v * 0.3f));   /* a collar where it meets the band */
                cv_px((int)x1 - 1, (int)y1, sh(R_IRON, v + 0.15f)); cv_px((int)x1 + 1, (int)y1, sh(R_IRON, v * 0.6f));
            }
            cv_capsule(cx, cy + 2, cx, cy + 15, 0.9f, R_IRON, 0.2f, 0.7f);   /* the finial */
            cv_ellipse(cx, cy + 6, 2.2f, 1.5f, R_IRON, 0.2f, 0.85f);
            cv_ellipse(cx, cy + 11, 1.6f, 1.2f, R_IRON, 0.2f, 0.8f);
            for (int j = 0; j < 6; j++) {                            /* its drop, pointed */
                float hw = 2.6f - 0.42f * (float)j;
                for (int x = (int)(cx - hw); x <= (int)(cx + hw); x++) cv_px(x, (int)cy + 15 + j, sh(R_IRON, x < (int)cx ? 0.75f : 0.3f));
            }
            cv_ellipse(cx, cy, 4.0f, 2.8f, R_IRON, 0.15f, 0.9f);    /* the boss, a ring of rivets on it */
            for (int k = 0; k < 6; k++) { float a = TAU_F * (float)k / 6.0f + 0.5f; cv_px((int)(cx + 2.8f * cosf(a)), (int)(cy + 1.9f * sinf(a)), sh(R_IRON, sinf(a) < 0.0f ? 0.95f : 0.6f)); }
            cv_px((int)cx - 1, (int)cy - 1, sh(R_IRON, 1.0f));
            continue;
        }
        int near = pass == 2;
        float lo = near ? 0.0f : 0.22f;
        for (int a = 0; a < 240; a++) {                              /* the bands, and the zigzag between */
            float ang = TAU_F * (float)a / 240.0f, sz = sinf(ang);
            if ((sz >= 0.0f) != near) continue;
            int x = (int)RIMX(ang, rx), y = (int)RIMY(ang, rx), xi = (int)RIMX(ang, ri), yi = (int)RIMY(ang, ri);
            float v = near ? 0.4f + 0.45f * sz * (cosf(ang) < 0.3f ? 1.0f : 0.75f) : 0.2f;
            v += 0.1f * (h01(a / 3, 1, 765) - 0.5f);                 /* hammered, unevenly */
            float tw = ((a / 3) & 1) ? 0.14f : -0.1f;                /* the outer band a twisted bar: its turns lit by turns */
            cv_px(x, y - 1, sh(R_IRON, v + 0.1f + tw)); cv_px(x, y, sh(R_IRON, v - tw * 0.6f)); cv_px(x, y + 1, sh(R_IRON, v * 0.45f));
            if (near) cv_px(x, y + 2, sh(R_IRON, 0.06f));
            cv_px(xi, yi, sh(R_IRON, v * 0.85f)); cv_px(xi, yi + 1, sh(R_IRON, v * 0.4f));
            if (a % 10 == 0) {
                float b2 = TAU_F * (float)(a + 5) / 240.0f;
                int up = (a / 10) & 1;
                cv_line((float)(up ? x : xi), (float)(up ? y : yi), RIMX(b2, up ? ri : rx), RIMY(b2, up ? ri : rx), sh(R_IRON, v * 0.75f - lo * 0.2f));
            }
        }
        for (int s = 0; s < 16; s++) {                               /* spikes and candles about it */
            float a = TAU_F * (float)s / 16.0f + 0.1f, sz = sinf(a);
            if ((sz >= 0.0f) != near) continue;
            float x = RIMX(a, rx - 0.5f), y = RIMY(a, rx - 0.5f) - 1.0f;
            if (s & 1) {
                for (int d = -1; d <= 1; d++) cv_px((int)x + d, (int)y, sh(R_IRON, 0.55f - lo));   /* the pricket's dish */
                hang_candle(x, y - 1.0f, 4.0f + 2.5f * h01(s, 1, 755), 1.3f, frame, s);
            } else {
                int hgt = near ? 9 : 6;
                for (int j = 1; j <= hgt; j++) {
                    cv_px((int)x, (int)y - j, sh(R_IRON, (near ? 0.85f : 0.45f) - 0.05f * (float)j));
                    if (j <= 2) cv_px((int)x + 1, (int)y - j, sh(R_IRON, near ? 0.3f : 0.15f));
                }
                cv_px((int)x - 1, (int)y - hgt / 2 - 1, sh(R_IRON, near ? 0.6f : 0.3f)); cv_px((int)x + 1, (int)y - hgt / 2 - 1, sh(R_IRON, near ? 0.4f : 0.2f));   /* its barbs */
                cv_px((int)x - 2, (int)y - hgt / 2, sh(R_IRON, near ? 0.45f : 0.2f)); cv_px((int)x + 2, (int)y - hgt / 2, sh(R_IRON, near ? 0.3f : 0.15f));
                if (near) { cv_px((int)x, (int)y + 3, sh(R_IRON, 0.5f)); cv_px((int)x, (int)y + 4, sh(R_IRON, 0.35f)); cv_px((int)x, (int)y + 5, sh(R_IRON, 0.2f)); }   /* a drop under it */
            }
        }
    }
    for (int k = 0; k < 4; k++) {                                    /* the near chains, over all */
        float a = PI_F * 0.25f + (float)k * PI_F * 0.5f;
        if (sinf(a) >= 0.0f) fine_chain(cx, 6, RIMX(a, rx - 1), RIMY(a, rx - 1) - 2, R_IRON, 0.3f, 0.75f);
    }
    for (int k = 0; k < 9; k++) {                                    /* wax hanging from the rim */
        float a = PI_F * (0.1f + 0.8f * h01(k, 1, 757));
        int x = (int)RIMX(a, rx), y = (int)RIMY(a, rx) + 3, l = 1 + (int)(h01(k, 2, 757) * 4.0f);
        for (int j = 1; j <= l; j++) cv_px(x, y + j, g_lrun == 3 ? sh(R_IRON, 0.25f) : sh(R_BONE, 0.85f - 0.08f * (float)j));
    }
    #undef RIMX
    #undef RIMY
    hang_runlook(761);
    cv_store(id);
    hang_wash(id, 763);
}

/* ---- the sanctuary lamp (chapels, burial vaults): a brass rose of five lobes on the vault, a smoke bell under it ringed with
 * mouldings, pierced for the smoke and beaded at its rim; three fine chains down to a brass cup of gadroons, each lobe turned to
 * the light, a beaded lip, verdigris in its seams; in it a beaker of red glass with a small flame inside, burning low and steady;
 * a banded knop and an acorn drop below. frame 0-1 ---- */
static void spr_sanctuary(int id, int frame) {
    cv_begin(cv_sprite, TS, TS);
    const float cx = 32;
    for (int x = (int)cx - 5; x <= (int)cx + 5; x++) {               /* the rose: five lobes along its lower edge */
        float u = ((float)x + 0.5f - cx) / 5.5f, lobe = fabsf(sinf(((float)x + 0.5f - cx + 5.5f) * PI_F / 2.2f));
        int h = 1 + (lobe > 0.5f);
        for (int y = 0; y <= h; y++) cv_px(x, y, sh(R_GOLD, (y == h ? 0.25f : 0.45f) + 0.35f * (u < 0.0f) - 0.15f * (lobe < 0.3f)));
    }
    fine_chain(cx, 3, cx, 7, R_GOLD, 0.3f, 0.7f);
    for (int y = 7; y <= 12; y++) {                                  /* the smoke bell */
        float hw = 1.2f + (float)(y - 7) * 1.0f;
        for (int x = (int)(cx - hw); x <= (int)(cx + hw); x++) {
            float nx = ((float)x + 0.5f - cx) / (hw + 0.01f), v = 0.18f + 0.5f * lit(nx, -0.3f, sqrtf(fmaxf(0.0f, 1.0f - nx * nx)));
            if (y == 9) v += 0.18f;                                  /* a moulded ring */
            if (y == 10 && ((x - (int)cx) % 2 == 0) && fabsf(nx) < 0.8f) v = 0.04f;   /* pierced for the smoke */
            if (y == 12) v = ((x & 1) ? 0.7f : 0.3f) - 0.2f * (nx > 0.3f);   /* its beaded rim */
            if (h01(x, y, 775) < 0.12f) v -= 0.15f;                 /* tarnish */
            cv_px(x, y, sh(R_GOLD, v));
        }
    }
    const float cupy = 38;
    static const float CH[3] = { -8.5f, 0.0f, 8.5f };
    for (int k = 0; k < 3; k++)                                      /* three fine chains, the middle one in front */
        fine_chain(cx + CH[k] * 0.45f, 13, cx + CH[k], cupy - 1, R_GOLD, k == 1 ? 0.45f : 0.25f, k == 1 ? 0.9f : 0.65f);
    for (int y = (int)cupy - 9; y < (int)cupy; y++) {                /* the beaker of red glass, lit from within */
        float hw = 4.2f + (float)((int)cupy - y) * 0.18f;
        for (int x = (int)(cx - hw); x <= (int)(cx + hw); x++) {
            float nx = ((float)x + 0.5f - cx) / hw, d = hypotf(nx * 1.2f, ((float)y - (cupy - 4.0f)) / 6.0f);
            int g = 1 + (int)((1.0f - d) * 4.0f + 0.4f * (float)frame * (d < 0.5f));
            if (nx < -0.62f && nx > -0.9f && y < cupy - 2) g += 2;   /* a gleam down its side */
            if (y == (int)cupy - 9) g = 5;                           /* its rim */
            cv_px(x, y, (uint8_t)(R_FLAME + clampi(g, 1, 6)));
        }
    }
    flame(cx + 0.5f, cupy - 3.0f, 4.5f + 0.5f * (float)frame, 1.2f, frame ? 0.3f : -0.3f);   /* the flame, seen through it */
    cv_px((int)cx, (int)cupy - 4, (uint8_t)(R_FLAME + 15));
    for (int y = (int)cupy; y <= (int)cupy + 7; y++) {               /* the cup: gadroons, each turned to the light */
        float hw = 9.5f - (float)((y - (int)cupy) * (y - (int)cupy)) * 0.14f;
        for (int x = (int)(cx - hw); x <= (int)(cx + hw); x++) {
            float nx = ((float)x + 0.5f - cx) / (hw + 0.01f);
            float v = 0.16f + 0.48f * lit(nx, 0.2f, sqrtf(fmaxf(0.0f, 1.0f - nx * nx)));
            if (y > (int)cupy + 1) {
                float gu = ((float)x + 0.5f - cx + 9.5f) / 2.5f, f = gu - floorf(gu);
                v += 0.22f * (0.5f - f) * (1.0f - 0.4f * fabsf(nx));  /* the lobe's left lit, its right in shade */
                if (f > 0.82f) {                                     /* the seam between, and verdigris in it */
                    v -= 0.12f;
                    if (h01(x, y, 777) < 0.35f && y > (int)cupy + 3) { cv_px(x, y, sh(R_GHOST, 0.26f + 0.1f * h01(x, y, 779))); continue; }
                }
            }
            if (y == (int)cupy) v = 0.78f - 0.3f * fabsf(nx);        /* its lip, lit red from above */
            if (y == (int)cupy + 1) v = ((x & 1) ? 0.62f : 0.22f) - 0.2f * (nx > 0.3f);   /* a bead under the lip */
            cv_px(x, y, sh(R_GOLD, v));
        }
    }
    for (int x = (int)cx - 7; x <= (int)cx + 7; x += 2) cv_px(x, (int)cupy, (uint8_t)(R_FLAME + 4));   /* the glass's light on the lip */
    cv_ellipse(cx, cupy + 9.5f, 2.4f, 1.5f, R_GOLD, 0.15f, 0.8f);    /* the knop, banded */
    for (int x = (int)cx - 2; x <= (int)cx + 2; x++) cv_px(x, (int)(cupy + 9.5f), sh(R_GOLD, x < (int)cx ? 0.3f : 0.15f));
    cv_capsule(cx, cupy + 11, cx, cupy + 13, 0.8f, R_GOLD, 0.2f, 0.7f);
    cv_ellipse(cx, cupy + 15.0f, 2.1f, 2.1f, R_GOLD, 0.15f, 0.85f);  /* the acorn: its cup, then its nut */
    for (int x = (int)cx - 2; x <= (int)cx + 2; x++) { cv_px(x, (int)cupy + 14, sh(R_GOLD, (x & 1) ? 0.55f : 0.25f)); cv_px(x, (int)cupy + 15, sh(R_GOLD, (x & 1) ? 0.3f : 0.5f)); }
    cv_px((int)cx, (int)cupy + 18, sh(R_GOLD, 0.6f)); cv_px((int)cx, (int)cupy + 19, sh(R_GOLD, 0.35f));
    for (int i = 0; i < TS * TS; i++)                                /* brass, not gold: duller than treasure */
        if (cv_sprite[i] >= R_GOLD && cv_sprite[i] < R_GOLD + NGOLD) cv_sprite[i] = (uint8_t)(R_GOLD + (int)((float)(cv_sprite[i] - R_GOLD) * 0.7f + 0.5f));
    hang_runlook(771);
    cv_store(id);
    hang_wash(id, 773);
}

/* a small skull with candles in its eyes: prop_skull's, its sockets alight, a stub of candle in each and its flame over the brow */
static void eye_candles(int x0, int y0, int mirror, int frame, int k) {
    prop_skull(x0, y0, mirror, 1, 0, 9);
    for (int e = 0; e < 2; e++) {
        int x = x0 + (e ? 8 : 2) - (mirror ? 0 : 0);
        for (int y = y0 + 4; y <= y0 + 5; y++) {                     /* the stub, standing out of the socket */
            cv_px(x, y, g_lrun == 3 ? sh(R_IRON, 0.3f) : sh(R_BONE, 0.9f));
            cv_px(x + 1, y, g_lrun == 3 ? sh(R_IRON, 0.14f) : sh(R_BONE, 0.55f));
        }
        prop_flame((float)x + 1.0f, (float)y0 + 3.0f, 4.5f + (float)((frame + k + e) % 3 == 1), 1.3f, (((frame + k + e) % 3) - 1) * 0.6f);
        cv_px(x, y0 + 7, g_lrun == 3 ? sh(R_IRON, 0.3f) : sh(R_BONE, 0.95f));   /* wax down the cheek */
        cv_px(x, y0 + 8, g_lrun == 3 ? sh(R_IRON, 0.25f) : sh(R_BONE, 0.8f));
    }
}

/* an antler from its burr at (x0, y0) out along a beam to (x1, y1), tines rising from it; s the side (-1 left, 1 right) */
static void antler(float x0, float y0, float x1, float y1, int s) {
    float mx = (x0 + x1) * 0.5f, my = fminf(y0, y1) - 3.0f;
    for (int i = 0; i < 12; i++) {                                   /* the beam, a curve, thinning */
        float t0 = (float)i / 12.0f, t1 = (float)(i + 1) / 12.0f;
        float ax = (1 - t0) * (1 - t0) * x0 + 2 * (1 - t0) * t0 * mx + t0 * t0 * x1, ay = (1 - t0) * (1 - t0) * y0 + 2 * (1 - t0) * t0 * my + t0 * t0 * y1;
        float bx = (1 - t1) * (1 - t1) * x0 + 2 * (1 - t1) * t1 * mx + t1 * t1 * x1, by = (1 - t1) * (1 - t1) * y0 + 2 * (1 - t1) * t1 * my + t1 * t1 * y1;
        cv_capsule(ax, ay, bx, by, 1.6f - 0.8f * t0, R_BONE, 0.18f, 0.7f);
        if (i == 3 || i == 6 || i == 9) {                            /* a tine, up and a little out */
            float l = i == 3 ? 8.0f : i == 6 ? 9.0f : 6.0f;
            cv_capsule(bx, by, bx + (float)s * 1.5f, by - l, 0.9f, R_BONE, 0.2f, 0.75f);
            cv_px((int)(bx + (float)s * 1.5f), (int)(by - l) - 1, sh(R_BONE, 0.85f));
        }
    }
    cv_capsule(x0 + (float)s * 2.0f, y0, x0 + (float)s * 5.0f, y0 + 6.0f, 1.0f, R_BONE, 0.18f, 0.6f);   /* the brow tine, down and forward */
    for (int k = 0; k < 6; k++) cv_px((int)x0 + (int)(h01(k, 1, 781) * 3.0f) - 1, (int)y0 + (int)(h01(k, 2, 781) * 3.0f) - 1, sh(R_WARM, 0.3f));   /* the burr, rough */
}

/* ---- the bone candelabrum (ossuaries, charnel pits): a great skull hung on a chain, a stag's antlers bound to its temples
 * and spread, a skull bound under each antler's end; ribs hung from its jaw round a spine of vertebrae, a jaw at its foot;
 * a candle burning in each eye of each skull, wax down their cheeks; cord where it is bound. frame 0-2 ---- */
static void spr_bonelabrum(int id, int frame) {
    cv_begin(cv_sprite, TS, TS);
    const float cx = 32;
    chain_down(cx, 0, 4);
    cv_ellipse(cx, 5, 1.8f, 1.5f, R_IRON, 0.2f, 0.8f);
    fine_chain(cx - 1, 6, 9, 12, R_IRON, 0.2f, 0.55f); fine_chain(cx + 1, 6, 55, 12, R_IRON, 0.2f, 0.55f);   /* to the antlers */
    antler(27, 17, 6, 13, -1); antler(37, 17, 58, 13, 1);
    for (int s = -1; s <= 1; s += 2) {                               /* the skulls bound under their ends, hanging on cord */
        int ex = s < 0 ? 4 : 50;
        cv_line((float)ex + 5.0f, 14.0f, (float)ex + 5.0f, 20.0f, sh(R_WOOD, 0.5f)); cv_line((float)ex + 6.0f, 14.0f, (float)ex + 6.0f, 20.0f, sh(R_WOOD, 0.25f));
        eye_candles(ex, 20, s > 0, frame, s < 0 ? 0 : 1);
    }
    for (int k = 0; k < 4; k++) {                                    /* ribs hung from the jaw, round the spine */
        float y = 32.0f + (float)k * 3.0f, w = 9.0f - (float)k * 1.3f;
        prop_rib(cx, y, w, 3.0f, PI_F * 0.05f, PI_F * 0.95f, 0.08f * (float)k);
    }
    for (int k = 0; k < 6; k++) prop_vertebra(cx, 31.0f + (float)k * 3.0f, 0.05f * (float)k);   /* the spine */
    prop_jaw(cx, 51);                                                /* a jaw at its foot */
    for (int k = 0; k < 4; k++) cv_px((int)cx - 4 + k * 3, 52 + (k & 1), sh(R_BONE, 0.6f));   /* finger bones hung from it */
    skull_stamp(cx, 22, 0, 0, 0);                                    /* the great skull */
    for (int e = 0; e < 2; e++) {                                    /* its eyes, alight, a candle in each */
        int x = (int)cx - 4 + e * 8 - (e ? 1 : 0);
        for (int y = 21; y <= 23; y++) for (int xx = x - 1; xx <= x + 1; xx++) cv_px(xx, y, (uint8_t)(R_FLAME + 8 + (y == 22) * 2));
        for (int y = 20; y <= 21; y++) { cv_px(x, y, g_lrun == 3 ? sh(R_IRON, 0.3f) : sh(R_BONE, 0.92f)); cv_px(x + 1, y, g_lrun == 3 ? sh(R_IRON, 0.14f) : sh(R_BONE, 0.55f)); }
        prop_flame((float)x + 1.0f, 19.0f, 5.5f + (float)((frame + e) % 3 == 2), 1.6f, (((frame + e * 2) % 3) - 1) * 0.6f);
        cv_px(x - 1, 25, g_lrun == 3 ? sh(R_IRON, 0.3f) : sh(R_BONE, 0.9f)); cv_px(x - 1, 26, g_lrun == 3 ? sh(R_IRON, 0.25f) : sh(R_BONE, 0.75f));
    }
    for (int s = -1; s <= 1; s += 2)                                 /* cord binding the antlers to its temples */
        for (int k = 0; k < 3; k++) cv_px((int)cx + s * 7 + (s > 0 ? k : -k), 16 + k, sh(R_WOOD, 0.35f + 0.1f * (float)(k & 1)));
    hang_runlook(791);
    cv_store(id);
    hang_wash(id, 793);
}

/* ---- the pierced cage lantern (passages, small rooms): tall, of black iron, a spire of fish-scale plates with crockets and a
 * finial, a riveted eave; a body of six faces, three seen, of hammered plate, pierced through with lancets and a quatrefoil in
 * front and holes all over, the light coming through them and soot above each; riveted ribs and bands; a ringed cone below and
 * a drop. frame 0-2 ---- */
static void spr_cagelantern(int id, int frame) {
    cv_begin(cv_sprite, TS, TS);
    const float cx = 32;
    chain_down(cx, 0, 7);
    cv_ellipse(cx, 8, 1.6f, 1.6f, R_IRON, 0.2f, 0.8f);
    cv_px((int)cx, 10, sh(R_IRON, 0.7f)); cv_px((int)cx - 1, 11, sh(R_IRON, 0.5f)); cv_px((int)cx + 1, 11, sh(R_IRON, 0.4f));   /* the finial */
    for (int y = 11; y <= 22; y++) {                                 /* the spire, its plates laid like scales */
        float hw = 0.6f + (float)(y - 11) * 0.72f;
        for (int x = (int)(cx - hw); x <= (int)(cx + hw); x++) {
            float nx = ((float)x + 0.5f - cx) / (hw + 0.01f), v = 0.15f + 0.5f * lit(nx, -0.5f, sqrtf(fmaxf(0.0f, 1.0f - nx * nx)));
            int row = (y - 11) / 3, ly = (y - 11) % 3, lx = ((x + row * 2) % 4 + 4) % 4;
            if (ly == 2 && lx != 1 && lx != 2) v -= 0.14f;           /* each scale's lower edge, curved */
            else if (ly == 0 && (lx == 1 || lx == 2)) v += 0.1f;
            cv_px(x, y, sh(R_IRON, v));
        }
        if (y % 3 == 0 && y > 12) { cv_px((int)(cx - hw) - 1, y - 1, sh(R_IRON, 0.6f)); cv_px((int)(cx + hw) + 1, y - 1, sh(R_IRON, 0.35f)); }   /* crockets */
    }
    for (int x = 20; x <= 44; x++) {                                 /* the eave, riveted */
        cv_px(x, 23, sh(R_IRON, 0.7f - 0.012f * (float)abs(x - 28) + ((x % 4 == 2) ? 0.2f : 0.0f)));
        cv_px(x, 24, sh(R_IRON, (x % 4 == 2) ? 0.08f : 0.22f));
    }
    const int top = 25, bot = 47;
    static uint8_t hole[TS][TS];
    memset(hole, 0, sizeof hole);
    for (int y = top; y <= bot; y++)                                 /* the body: three faces, ribs between, pierced */
        for (int x = 23; x <= 41; x++) {
            int face = x < 28 ? 0 : x <= 36 ? 1 : 2, fx = face == 0 ? x - 23 : face == 1 ? x - 28 : x - 37, fw = face == 1 ? 8 : 5;
            float v = face == 0 ? 0.42f : face == 1 ? 0.3f : 0.14f;
            v += 0.1f * (0.5f - (float)fx / (float)fw);              /* each face turned a little from the light across it */
            int band = y == top || y == top + 1 || y == bot || y == 36;
            int rib = x == 23 || x == 28 || x == 36 || x == 41;
            if (band) { cv_px(x, y, sh(R_IRON, (y == top ? 0.62f : y == top + 1 ? 0.28f : 0.45f) - 0.15f * (face == 2) + ((x % 3 == 1 && y != top + 1) ? 0.22f : 0.0f))); continue; }
            if (rib) { cv_px(x, y, sh(R_IRON, (x == 23 || x == 28 ? 0.7f : 0.38f) + ((y % 4 == 0) ? 0.2f : 0.0f))); continue; }   /* rivets down them */
            int h = 0;
            if (face == 1) {                                         /* in front: a lancet over a quatrefoil */
                int lx = fx - 4, ly = y - top;
                if (ly >= 3 && ly <= 9 && abs(lx) <= 1 && !(ly == 3 && lx != 0)) h = 2;
                int qy = y - 41, ql = abs(lx) + abs(qy);
                if (y > 36 && ((ql == 2 && (lx == 0 || qy == 0)) || (abs(lx) == 1 && abs(qy) == 1) || ql == 0)) h = 2;
                if (!h && ((fx + y) % 4 == 0) && (fx == 1 || fx == 7)) h = 1;
            } else if (((fx + (y / 2) * 2) % 3 == 1) && (y % 2 == 0)) h = 1;   /* the sides: holes in a lattice */
            if (h) {
                hole[x][y] = 1;
                float d = fabsf((float)x - cx) / 9.0f + fabsf((float)y - 38.0f) / 14.0f;
                int g = (h == 2 ? 13 : 11) - (int)(d * 5.0f) - (face != 1) * 2 + (int)(h01(x, y, 801u + (uint32_t)frame) * 3.0f) - 1;
                cv_px(x, y, (uint8_t)(R_FLAME + clampi(g, 3, 15)));
            } else {
                v += 0.14f * (fbm((float)x / 2.5f, (float)y / 2.5f, 16, 805) - 0.5f);   /* hammered */
                if (h01(x, y, 807) > 0.94f) v -= 0.12f;
                cv_px(x, y, sh(R_IRON, v));
            }
        }
    for (int y = top + 2; y < bot; y++)                              /* soot over each hole, the plate's lit edge under it */
        for (int x = 24; x <= 40; x++) {
            if (hole[x][y] || !hole[x][y + 1]) continue;
            uint8_t c = cv_sprite[y * TS + x];
            if (c > R_IRON && c < R_IRON + 16) cv_px(x, y, (uint8_t)(c - 3 < R_IRON + 1 ? R_IRON + 1 : c - 3));
        }
    for (int y = top + 2; y < bot; y++)
        for (int x = 24; x <= 40; x++) {
            if (hole[x][y] || !hole[x][y - 1]) continue;
            uint8_t c = cv_sprite[y * TS + x];
            if (c > R_IRON && c < R_IRON + 14) cv_px(x, y, (uint8_t)(c + 2));
        }
    for (int y = bot + 1; y <= bot + 6; y++) {                       /* the cone under it, ringed */
        float hw = 9.0f - (float)(y - bot) * 1.3f;
        for (int x = (int)(cx - hw); x <= (int)(cx + hw); x++) {
            float nx = ((float)x + 0.5f - cx) / (hw + 0.01f), v = 0.12f + 0.45f * lit(nx, 0.3f, sqrtf(fmaxf(0.0f, 1.0f - nx * nx)));
            if (y == bot + 2) v += 0.15f; else if (y == bot + 3) v -= 0.08f;
            cv_px(x, y, sh(R_IRON, v));
        }
    }
    cv_ellipse(cx, (float)bot + 8.5f, 1.8f, 1.8f, R_IRON, 0.15f, 0.8f);   /* the drop */
    cv_px((int)cx, bot + 11, sh(R_IRON, 0.45f));
    cv_outline(0);
    hang_runlook(811);
    cv_store(id);
    hang_wash(id, 813);
}

/* ---- the events' things (docs/PLAN-events.md) ---- */

static void bput(uint8_t *t, int x, int y, uint8_t c) { t[(x & 63) * TS + (y & 63)] = c; }

/* a chain across a face from (x0, y0) to (x1, y1), links face-on and edge-on by turns, lit from above */
static void face_chain(uint8_t *t, float x0, float y0, float x1, float y1, float sag) {
    float L = hypotf(x1 - x0, y1 - y0);
    int n = (int)(L / 3.0f);
    for (int i = 0; i <= n; i++) {
        float u = (float)i / (float)n, x = x0 + (x1 - x0) * u, y = y0 + (y1 - y0) * u + sag * 4.0f * u * (1.0f - u);
        if (i & 1) {
            for (int k = -1; k <= 1; k++) bput(t, (int)(x + (float)k * (x1 - x0) / L), (int)(y + (float)k * (y1 - y0) / L), sh(R_IRON, k < 0 ? 0.75f : 0.45f));
        } else {
            static const int R[8][2] = { { -1, -1 }, { 0, -2 }, { 1, -1 }, { 1, 0 }, { 1, 1 }, { 0, 2 }, { -1, 1 }, { -1, 0 } };
            for (int k = 0; k < 8; k++) bput(t, (int)x + R[k][0], (int)y + R[k][1], sh(R_IRON, R[k][1] < 0 ? 0.7f : R[k][1] > 0 ? 0.2f : 0.45f));
            bput(t, (int)x, (int)y, 0);
        }
    }
}
/* a round-headed rivet or bolt, two by two: lit at its top left, its shadow at its foot */
static void rivet(uint8_t *t, int x, int y) {
    bput(t, x, y, sh(R_IRON, 0.98f)); bput(t, x + 1, y, sh(R_IRON, 0.55f)); bput(t, x, y + 1, sh(R_IRON, 0.5f)); bput(t, x + 1, y + 1, sh(R_IRON, 0.18f));
    bput(t, x + 1, y + 2, sh(R_IRON, 0.06f));
}

/* the barred door: the run's own door, barred. A great beam across it, adzed oak, its grain running, a knot or two, its ends
 * checked, rounded to the light; two iron bands on it, hammered and riveted; a staple at each side, bolted. A chain crossed over
 * it to a domed plate of hammered iron in the middle, a raised rim, rivets round it, rust; a hand cut in it (it says stop, and
 * nothing in it takes a key). Unbarred: the beam and plate gone, the staples empty, the chain hung loose from one of them. Over
 * the door it is given, column-major. */
static void gen_barred(uint8_t *t, const uint8_t *door, int unbarred, uint32_t seed) {
    memcpy(t, door, TS * TS);
    const int by0 = 27, by1 = 37;
    if (!unbarred) {
        face_chain(t, 6, 6, 58, 57, 2.0f); face_chain(t, 58, 6, 6, 57, 2.0f);
        for (int y = by0; y <= by1; y++)                             /* the beam */
            for (int x = 0; x < TS; x++) {
                float g = 0.11f * sinf((float)y * 2.3f + 4.0f * fbm((float)x / 14.0f, (float)y / 3.0f, 8, seed));   /* its grain */
                float v = 0.19f + g + 0.06f * (fbm((float)x / 3.0f, (float)y, 16, seed + 1) - 0.5f);
                for (int k = 0; k < 2; k++) {                        /* knots */
                    float d = hypotf(((float)x - (k ? 40.5f : 23.5f)) * 0.55f, (float)y - (k ? 34.0f : 30.0f));
                    if (d < 1.2f) v = 0.08f; else if (d < 2.2f) v += 0.12f; else if (d < 3.0f) v -= 0.06f;
                }
                if ((x < 3 || x > 60) && (y == by0 + 3 || y == by0 + 7)) v = 0.04f;   /* its ends, checked */
                if (y == by0) v = 0.62f; else if (y == by0 + 1) v += 0.2f; else if (y == by0 + 2) v += 0.08f;
                else if (y == by1) v = 0.04f; else if (y == by1 - 1) v -= 0.08f;
                bput(t, x, y, sh(R_WOOD, v));
                if (((x >= 15 && x <= 17) || (x >= 46 && x <= 48)) && y >= by0 && y <= by1) {   /* its bands, hammered */
                    float iv = (x == 15 || x == 46 ? 0.72f : x == 16 || x == 47 ? 0.45f : 0.24f) + 0.12f * (h01(x, y, seed + 2) - 0.5f);
                    if (y == by0) iv += 0.15f; else if (y == by1) iv -= 0.15f;
                    bput(t, x, y, sh(R_IRON, iv));
                }
            }
        rivet(t, 16, by0 + 2); rivet(t, 16, by1 - 3); rivet(t, 47, by0 + 2); rivet(t, 47, by1 - 3);
        for (int x = 0; x < TS; x++)                                 /* its shadow on the door below */
            for (int y = by1 + 1; y <= by1 + 3; y++) {
                uint8_t c = t[x * TS + y];
                if (c == C_CLEAR || IS_FULLBRIGHT(c) || c == 0 || c >= R_GOLD) continue;
                int ramp = (c - 1) / 16 * 16 + 1;
                t[x * TS + y] = (uint8_t)(ramp + clampi(c - ramp - (by1 + 5 - y) * 2, 1, 15));
            }
    }
    for (int s = 0; s < 2; s++) {                                    /* the staples, bolted to the door */
        int x0 = s ? 54 : 3;
        for (int y = by0 - 5; y <= by1 + 5; y++)
            for (int x = x0; x <= x0 + 6; x++) {
                int plate = y < by0 - 1 || y > by1 + 1, loop = x == x0 + 1 || x == x0 + 5 || (x >= x0 + 1 && x <= x0 + 5 && (y == by0 - 1 || y == by1 + 1));
                if (!plate && !loop) { if (unbarred && x > x0 + 1 && x < x0 + 5) bput(t, x, y, sh(R_IRON, 0.04f)); continue; }   /* empty, its shadow */
                float v = plate ? 0.4f + 0.12f * (h01(x, y, seed + 4) - 0.5f) : (x == x0 + 1 ? 0.8f : x == x0 + 5 ? 0.35f : 0.55f);
                if (plate && (y == by0 - 5 || y == by1 + 2 || x == x0)) v += 0.22f;   /* its bevelled edges */
                if (plate && (y == by1 + 5 || y == by0 - 2 || x == x0 + 6)) v -= 0.2f;
                bput(t, x, y, sh(R_IRON, v));
            }
        rivet(t, x0 + 2, by0 - 4); rivet(t, x0 + 2, by1 + 2);
    }
    if (!unbarred) {                                                 /* the plate, domed, the hand cut in it */
        for (int y = 21; y <= 43; y++)
            for (int x = 21; x <= 43; x++) {
                float dx = (float)x + 0.5f - 32.0f, dy = (float)y + 0.5f - 32.0f, r = sqrtf(dx * dx + dy * dy);
                if (r > 11.0f) continue;
                float L = -(dx + dy) * 0.707f / (r + 0.01f), v;
                if (r > 9.9f) v = 0.06f;                             /* its edge, standing off the beam */
                else if (r > 8.4f) v = 0.42f + 0.38f * L;            /* the raised rim, turned to the light */
                else {
                    float nz = sqrtf(fmaxf(0.0f, 1.0f - (r / 8.4f) * (r / 8.4f)));
                    v = 0.22f + 0.52f * lit(dx / 8.4f, dy / 8.4f, nz) + 0.1f * (fbm((float)x / 2.0f, (float)y / 2.0f, 16, seed + 3) - 0.5f);
                    if (h01(x, y, seed + 5) > 0.93f) v -= 0.15f;     /* hammer marks */
                }
                uint8_t c = sh(R_IRON, v);
                if (r > 5.5f && r < 9.9f && h01(x / 2, y / 2, seed + 6) < 0.08f) c = sh(R_BRICK, 0.12f + 0.25f * v);   /* rust */
                bput(t, x, y, c);
            }
        for (int k = 0; k < 8; k++) { float a = (float)k * TAU_F / 8.0f + 0.39f; rivet(t, 31 + (int)lroundf(9.2f * cosf(a)), 31 + (int)lroundf(9.2f * sinf(a))); }
        for (int y = 0; y < 7; y++)                                  /* the hand, cut: its floor dark, its lower right walls catching the light */
            for (int x = 0; x < 7; x++)
                if (PZ_MARKS[11][y][x] == '#') {
                    bput(t, 29 + x, 28 + y, sh(R_IRON, 0.03f));
                    if (y == 6 || PZ_MARKS[11][y + 1][x] != '#') bput(t, 29 + x, 29 + y, sh(R_IRON, 0.75f));
                    if (x == 6 || PZ_MARKS[11][y][x + 1] != '#') bput(t, 30 + x, 28 + y, sh(R_IRON, 0.6f));
                }
        for (int y = 0; y < 7; y++) for (int x = 0; x < 7; x++) if (PZ_MARKS[11][y][x] == '#') bput(t, 29 + x, 28 + y, sh(R_IRON, 0.03f));
    } else face_chain(t, 6, by1 + 2, 9, 62, 1.0f);                   /* the chain, hung loose */
}

/* the relic: a long bone, a gold ferrule on its knuckle, bound in three wrappings of gold wire; a key's two teeth cut at its end;
 * a red wax seal, stamped, hung from its middle on a twisted cord; lying on the floor, a glint on it */
static void relic_bone(float x0, float y0, float x1, float y1, int glint) {
    prop_long(x0, y0, x1, y1, 2.3f, 0, 0, 0.0f, 821);
    float L = hypotf(x1 - x0, y1 - y0), ux = (x1 - x0) / L, uy = (y1 - y0) / L, px = -uy, py = ux;
    for (int k = -3; k <= 3; k++)                                    /* the ferrule, capping the knuckle */
        for (int w = 0; w < 3; w++) {
            float x = x0 + ux * (float)(w - 1) + px * (float)k * 0.8f, y = y0 + uy * (float)(w - 1) + py * (float)k * 0.8f;
            cv_px((int)x, (int)y, sh(R_GOLD, (w == 1 ? 0.35f : 0.7f) + 0.25f * (k < 0) - 0.3f * (k > 1)));
        }
    for (int b = 0; b < 3; b++) {                                    /* the wrappings of wire, each turn lit on its top */
        float t = 0.3f + 0.21f * (float)b;
        for (int w = 0; w < 3; w++)
            for (int k = -3; k <= 3; k++) {
                float x = x0 + (x1 - x0) * t + ux * (float)w + px * (float)k * 0.85f, y = y0 + (y1 - y0) * t + uy * (float)w + py * (float)k * 0.85f;
                float v = ((k + w) & 1) ? 0.9f : 0.4f;
                if (k > 1) v -= 0.3f; else if (k < -1) v += 0.05f;
                cv_px((int)x, (int)y, sh(R_GOLD, v));
            }
    }
    for (int k = 0; k < 2; k++) {                                    /* the key's teeth */
        float t = 0.86f + 0.07f * (float)k;
        cv_px((int)(x0 + (x1 - x0) * t - px * 3.0f), (int)(y0 + (y1 - y0) * t - py * 3.0f), sh(R_BONE, 0.7f));
        cv_px((int)(x0 + (x1 - x0) * t - px * 4.0f), (int)(y0 + (y1 - y0) * t - py * 4.0f), sh(R_BONE, 0.5f));
    }
    float mx = (x0 + x1) * 0.5f, my = (y0 + y1) * 0.5f;
    for (int k = 0; k <= 3; k++) cv_px((int)(mx - 0.33f * (float)k), (int)my + 1 + k, sh(R_WOOD, (k & 1) ? 0.55f : 0.22f));   /* the cord, twisted */
    cv_ellipse(mx - 1.0f, my + 6.0f, 2.6f, 2.3f, R_BLOOD, 0.2f, 0.8f);   /* the seal: its wax pressed out round the stamp */
    cv_px((int)mx - 2, (int)my + 5, sh(R_BLOOD, 0.2f)); cv_px((int)mx - 1, (int)my + 6, sh(R_BLOOD, 0.15f)); cv_px((int)mx, (int)my + 7, sh(R_BLOOD, 0.6f));
    cv_px((int)mx - 1, (int)my + 5, sh(R_BLOOD, 0.55f)); cv_px((int)mx - 2, (int)my + 6, sh(R_BLOOD, 0.55f));
    cv_px((int)mx + 1, (int)my + 8, sh(R_BLOOD, 0.4f));              /* a drip of it */
    if (glint) { cv_px((int)(x0 + (x1 - x0) * 0.5f + ux * 1.0f), (int)(y0 + (y1 - y0) * 0.5f) - 1, C_WHITE); cv_px((int)(x0 + (x1 - x0) * 0.3f), (int)(y0 + (y1 - y0) * 0.3f) - 2, C_WHITE); }
}
static void spr_relic(int id) {
    cv_begin(cv_sprite, TS, TS);
    cv_flat(32, 61.5f, 15, 1.8f, R_WARM + 1);
    relic_bone(17, 59, 47, 52, 0);
    cv_outline(0);
    cv_px(31, 54, C_WHITE); cv_px(22, 55, C_WHITE);
    cv_store(id);
}

/* dressed stone on the canvas, a block from (x0, y0) to (x1, y1), base its face's step: the face turned by the light (its left
 * arris lit, its right in shade), its top chamfered and lit, its foot in shade; tooled in fine courses of the chisel, pitted (each
 * pit's lower lip lit), a top corner chipped away; a hairline crack down it when crack is set */
static void dressed(int x0, int y0, int x1, int y1, float base, uint32_t seed, int crack) {
    for (int y = y0; y <= y1; y++)
        for (int x = x0; x <= x1; x++) {
            float v = base + 0.12f * (fbm((float)x / 5.0f, (float)y / 5.0f, 8, seed) - 0.5f);
            if ((y + (int)(h01(x / 4, y, seed + 1) * 2.0f)) & 1) v -= 0.045f;   /* the chisel's courses */
            if (x == x0) v += 0.24f; else if (x == x0 + 1) v += 0.1f;
            if (x == x1) v -= 0.2f; else if (x == x1 - 1) v -= 0.07f;
            if (y == y0) v += 0.26f; else if (y == y0 + 1) v += 0.08f;
            if (y == y1) v -= 0.16f;
            if (h01(x, y, seed + 2) > 0.955f) v -= 0.2f;
            else if (h01(x, y - 1, seed + 2) > 0.955f) v += 0.12f;
            cv_px(x, y, sh(R_STONE, v));
        }
    int cxr = h01(x0, y0, seed + 3) < 0.5f ? x0 : x1, dir = cxr == x0 ? 1 : -1;   /* a corner chipped */
    for (int k = 0; k < 3; k++)
        for (int j = 0; j <= 2 - k; j++) cv_px(cxr + dir * j, y0 + k, j + k == 2 ? sh(R_STONE, base - 0.14f) : (uint8_t)C_CLEAR);
    if (crack) {
        int x = x0 + (x1 - x0) / 3 + (int)(h01(x0, y1, seed + 4) * (float)(x1 - x0) / 3.0f);
        for (int y = y0 + 1; y <= y0 + (y1 - y0) * 2 / 3; y++) {
            float r = h01(x, y, seed + 5);
            x += r < 0.3f ? -1 : r > 0.7f ? 1 : 0;
            if (x <= x0 || x >= x1) break;
            cv_px(x, y, sh(R_STONE, base - 0.28f)); cv_px(x + 1, y, sh(R_STONE, base + 0.12f));
        }
    }
}

/* the socket: a plinth of dressed stone on a chamfered base, a roll moulding at its foot, a sunk panel in its shaft with the
 * relic's shape cut in it, a moulded capital (a fillet, a roll, a cove), and on it two stone hands held up, palms in, fingers
 * curled over, to take the relic between them. Holding: the relic laid across the hands, the shape cut in the panel alight */
static void stone_hand(int s) {                                     /* s -1 the left hand (lit), 1 the right */
    float m = (float)s;
    #define HX(x) (32.0f + m * (32.0f - (x)))
    float palm[12] = { HX(21), 38, HX(30), 38, HX(30.5f), 31, HX(29), 25.5f, HX(24), 24, HX(20.5f), 28 };
    cv_poly(palm, 6, R_STONE, s < 0 ? 0.42f : 0.2f, s < 0 ? 0.72f : 0.45f, 24, 14);
    for (int f = 0; f < 3; f++) {                                    /* the fingers, curled over, the gaps between them dark */
        float x = HX(24.5f + (float)f * 1.8f);
        cv_px((int)x, 25 + (f == 1 ? 0 : 1), sh(R_STONE, s < 0 ? 0.85f : 0.55f));
        if (f < 2) cv_line(HX(25.4f + (float)f * 1.8f), 26, HX(25.8f + (float)f * 1.8f), 30, sh(R_STONE, 0.12f));
    }
    for (int k = 0; k < 3; k++) cv_px((int)HX(24.0f + (float)k * 1.8f), 28, sh(R_STONE, s < 0 ? 0.62f : 0.38f));   /* the knuckles */
    cv_capsule(HX(21), 34, HX(18.5f), 29.5f, 1.3f, R_STONE, s < 0 ? 0.35f : 0.18f, s < 0 ? 0.75f : 0.45f);   /* the thumb */
    cv_px((int)HX(19.0f), 30, sh(R_STONE, s < 0 ? 0.9f : 0.5f));     /* its nail */
    for (int y = 33; y <= 37; y++) cv_px((int)HX(29.5f), y, sh(R_STONE, 0.1f));   /* the heel of the hand, in shadow */
    for (int y = 30; y <= 37; y++) if (h01(y, s + 2, 837) < 0.4f) cv_px((int)HX(22.5f + h01(y, s + 3, 837) * 5.0f), y, sh(R_STONE, s < 0 ? 0.32f : 0.16f));   /* worn */
    #undef HX
}
static void spr_socket(int id, int holding) {
    cv_begin(cv_sprite, TS, TS);
    cv_flat(32, 62, 15, 1.8f, R_WARM + 1);
    dressed(19, 56, 45, 62, 0.32f, 831, 0);                          /* the base */
    for (int x = 21; x <= 43; x++) {                                 /* the roll at the shaft's foot */
        float u = (float)(x - 21) / 22.0f;
        cv_px(x, 54, sh(R_STONE, 0.66f - 0.3f * u)); cv_px(x, 55, sh(R_STONE, 0.18f - 0.06f * u));
    }
    dressed(23, 43, 41, 53, 0.36f, 833, 1);                          /* the shaft */
    for (int y = 44; y <= 52; y++)                                   /* its sunk panel: the shadow in at its top and left, light on its far lips */
        for (int x = 25; x <= 39; x++) {
            uint8_t c = cv_sprite[y * TS + x];
            if (y == 44 || x == 25) c = sh(R_STONE, 0.12f);
            else if (y == 52 || x == 39) c = sh(R_STONE, 0.58f);
            else if (c > R_STONE && c < R_STONE + 16) c = (uint8_t)(c - 1);
            cv_px(x, y, c);
        }
    for (int y = 38; y <= 42; y++)                                   /* the capital: a fillet, a roll, a cove, its shadow */
        for (int x = 20; x <= 44; x++) {
            float u = (float)(x - 20) / 24.0f, v = y == 38 ? 0.74f : y == 39 ? 0.56f : y == 40 ? 0.22f : y == 41 ? 0.42f : 0.1f;
            v += 0.14f * (0.5f - u) + 0.06f * (h01(x, y, 835) - 0.5f);
            if (x == 20 || x == 44) v -= 0.12f;
            cv_px(x, y, sh(R_STONE, v));
        }
    cv_px(44, 38, C_CLEAR); cv_px(43, 38, sh(R_STONE, 0.3f));        /* a corner of it gone */
    stone_hand(1); stone_hand(-1);
    for (int y = 31; y <= 37; y++) for (int x = 31; x <= 33; x++) cv_px(x, y, sh(R_STONE, 0.05f));   /* the hollow between them */
    static const char *const CUT[7] = { "...........", ".##.....##.", "###########", ".##.#.#.##.", "###########", ".##.....##.", "..........." };
    for (int y = 0; y < 7; y++)                                      /* the relic's shape, cut in the panel */
        for (int x = 0; x < 11; x++)
            if (CUT[y][x] == '#') {
                cv_px(27 + x, 45 + y, holding ? (uint8_t)(R_GLOW + 10 + ((x + y) & 1) * 3) : sh(R_STONE, 0.04f));
                if (!holding && (y == 6 || CUT[y + 1][x] != '#')) cv_px(27 + x, 46 + y, sh(R_STONE, 0.62f));
            }
    if (holding) relic_bone(18, 27, 46, 26, 1);
    cv_outline(0);
    if (holding) { cv_px(28, 43, (uint8_t)(R_GLOW + 12)); cv_px(34, 42, (uint8_t)(R_GLOW + 11)); cv_px(31, 41, (uint8_t)(R_GLOW + 13)); }   /* a breath of light off the cut */
    cv_store(id);
}

/* the lever: a dressed stone post on a plinth course, its gabled head coped; on its face an iron quadrant of hammered plate,
 * riveted, toothed round its rim, rust in it; through its slot a twisted iron arm on a riveted boss, a turned grip of wood and a
 * ball at its end; up, it leans back over the post; pulled, it is down and out to the right */
static void spr_lever(int id, int down) {
    cv_begin(cv_sprite, TS, TS);
    cv_flat(32, 62.5f, 13, 1.6f, R_WARM + 1);
    const float pvx = 32, pvy = 45, len = 23;
    float a = down ? 1.2f : -0.55f, ex = pvx + len * sinf(a), ey = pvy - len * cosf(a);
    if (!down) cv_capsule(pvx, pvy, ex, ey, 1.3f, R_IRON, 0.15f, 0.5f);   /* up: behind the head, then over it */
    dressed(23, 38, 41, 56, 0.36f, 841, 1);                          /* the post */
    for (int y = 33; y <= 38; y++)                                   /* its head, gabled, the coping lit on its left slope */
        for (int x = 23; x <= 41; x++) {
            float head = 33.0f + fabsf((float)x + 0.5f - 32.0f) * 0.55f;
            if ((float)y < head) continue;
            float v = 0.4f + 0.06f * (h01(x, y, 843) - 0.5f);
            if ((float)y < head + 1.0f) v = x < 32 ? 0.78f : 0.3f;
            else if ((float)y < head + 2.0f) v = x < 32 ? 0.5f : 0.22f;
            if (y == 38) v = 0.14f;                                  /* the coping's drip */
            cv_px(x, y, sh(R_STONE, v));
        }
    dressed(22, 57, 42, 62, 0.3f, 845, 0);                           /* the plinth course */
    for (int y = 34; y <= 53; y++)                                   /* the quadrant, its slot */
        for (int x = 21; x <= 43; x++) {
            float dx = (float)x + 0.5f - pvx, dy = (float)y + 0.5f - pvy, r = sqrtf(dx * dx + dy * dy);
            if (r > 10.5f || dy > 3.0f) continue;
            float v = r > 9.4f ? 0.75f - 0.3f * (dx > 0) : 0.36f + 0.14f * (dx < 0) - 0.08f * (dy > 1.5f);
            v += 0.08f * (fbm((float)x / 2.0f, (float)y / 2.0f, 16, 847) - 0.5f);   /* hammered */
            if (r > 5.5f && r < 7.0f && dy < 1.0f) v = 0.04f;        /* the slot */
            else if (r > 4.6f && r <= 5.5f && dy < 1.0f) v = 0.62f;  /* its inner lip, lit */
            if (r >= 9.4f && ((int)(atan2f(dy, dx) * 6.0f) & 1)) v = 0.2f;   /* its teeth */
            uint8_t c = sh(R_IRON, v);
            if (r < 9.4f && (r < 5.5f || r > 7.0f) && h01(x / 2, y, 849) < 0.05f) c = sh(R_BRICK, 0.14f + 0.2f * v);   /* rust */
            cv_px(x, y, c);
        }
    for (int k = 0; k < 3; k++) {                                    /* its rivets */
        float ra = -1.25f + 1.25f * (float)k;
        int rx = (int)(pvx + 8.0f * sinf(ra)), ry = (int)(pvy - 8.0f * cosf(ra));
        cv_px(rx, ry, sh(R_IRON, 0.95f)); cv_px(rx + 1, ry + 1, sh(R_IRON, 0.12f));
    }
    if (down) cv_capsule(pvx, pvy, ex, ey, 1.3f, R_IRON, 0.2f, 0.7f);
    else cv_capsule(pvx, pvy, pvx + 8.0f * sinf(a), pvy - 8.0f * cosf(a), 1.3f, R_IRON, 0.2f, 0.7f);
    for (int k = 2; k < 17; k += 3) cv_px((int)(pvx + (float)k * sinf(a)), (int)(pvy - (float)k * cosf(a)), sh(R_IRON, 0.9f));   /* its twist */
    float gx = pvx + 16.0f * sinf(a), gy = pvy - 16.0f * cosf(a);
    cv_capsule(gx, gy, ex, ey, 2.0f, R_WOOD, 0.2f, 0.75f);           /* the grip, turned: rings cut round it */
    for (int k = 1; k < 7; k += 2) { float t = (float)k / 7.0f; cv_px((int)(gx + (ex - gx) * t), (int)(gy + (ey - gy) * t), sh(R_WOOD, 0.12f)); }
    cv_ellipse(ex + 2.0f * sinf(a), ey - 2.0f * cosf(a), 3.0f, 3.0f, R_IRON, 0.18f, 0.9f);   /* the ball */
    cv_px((int)(ex + 2.0f * sinf(a)) - 1, (int)(ey - 2.0f * cosf(a)) - 1, sh(R_IRON, 1.0f));
    cv_ellipse(pvx, pvy, 3.0f, 3.0f, R_IRON, 0.2f, 0.9f);            /* the boss, its pin */
    cv_px((int)pvx, (int)pvy, sh(R_IRON, 0.1f)); cv_px((int)pvx - 1, (int)pvy - 1, sh(R_IRON, 0.75f));
    cv_outline(0);
    cv_store(id);
}

/* the pressure plate, a floor texture laid over the run's floor: a square flagstone of dressed stone set in a deep joint with grit
 * in it, tooled across, worn smooth where it is trodden, two corners chipped and a crack in from one; a border cut round it, its
 * lip lit where it faces the light; a roundel with a raised rose of eight cut in it and a boss at its heart; an iron stud, domed,
 * at each corner. Pressed: sunk a hand's breadth, darker, the pit's side showing at its far edges and dust puffed out at the joint */
static void gen_plate(uint8_t *t, int pressed, uint32_t seed) {
    memset(t, C_CLEAR, TS * TS);                                    /* an overlay: the cell's own floor shows round it */
    const int a = 6, b = 57;
    int o = pressed ? 1 : 0;
    for (int y = a - 2; y <= b + 2; y++)
        for (int x = a - 2; x <= b + 2; x++) {
            if (x < a || x > b || y < a || y > b) {                  /* the joint, grit in it */
                bput(t, x, y, h01(x, y, seed + 9) < 0.2f ? sh(R_WARM, 0.18f) : sh(R_STONE, 0.03f));
                continue;
            }
            if (pressed && (x < a + 3 || y < a + 3)) { bput(t, x, y, sh(R_STONE, x < a + 3 && y < a + 3 ? 0.08f : 0.16f)); continue; }   /* the pit's side */
            int sx = x - o * 2, sy = y - o * 2;
            int ex = sx - a, ey = sy - a, n = b - a;
            if ((ex + ey < 3) || ((n - ex) + ey < 2)) { bput(t, x, y, sh(R_STONE, 0.05f)); continue; }   /* chipped corners */
            float dx = (float)sx + 0.5f - 32.0f, dy = (float)sy + 0.5f - 32.0f, r = sqrtf(dx * dx + dy * dy), L = -(dx + dy) * 0.707f / (r + 0.01f);
            float v = 0.47f + 0.12f * (fbm((float)sx / 8.0f, (float)sy / 8.0f, 8, seed) - 0.5f) + 0.06f * (h01(sx, sy, seed + 1) - 0.5f);
            if (((sx + sy * 2) % 4) == 0) v -= 0.035f;               /* tooled across */
            v += 0.08f * expf(-r * r / 260.0f);                      /* worn smooth where it is trodden */
            if (h01(sx, sy, seed + 2) > 0.965f) v -= 0.16f;          /* pits */
            if (sx == a + o * 2 || sy == a + o * 2) v += pressed ? 0.0f : 0.18f;      /* its near edges, lit */
            if (sx == b || sy == b) v -= 0.18f;
            int e = ex < ey ? ex : ey, e2 = (n - ex) < (n - ey) ? n - ex : n - ey, far = e2 < e;
            if (far) e = e2;
            if (e == 5) v = 0.12f;                                   /* the border, cut; its lip lit where it faces the light */
            else if (e == 6 && !far) v += 0.2f;
            else if (e == 4 && far) v += 0.16f;
            else if (e == 4 && !far) v -= 0.06f;
            if (fabsf(r - 12.0f) < 0.8f) v = 0.1f;                   /* the roundel's groove, its walls lit by turns */
            else if (r > 10.9f && r <= 11.2f) v += 0.24f * fmaxf(0.0f, L) - 0.08f;
            else if (r >= 12.8f && r < 13.6f) v += 0.24f * fmaxf(0.0f, -L) - 0.04f;
            if (r < 10.9f) {                                         /* the rose, raised, its petals rounded */
                float an = atan2f(dy, dx), k = fabsf(sinf(an * 4.0f)), dome = 1.0f - r / 10.9f;
                v += 0.1f * dome + 0.12f * L * (1.0f - dome);
                if (r < 2.2f) v = 0.62f + 0.25f * L;                   /* the boss at its heart */
                else if (r < 3.0f) v = 0.1f;
                else if (k < 0.12f + 0.6f / (r + 1.0f) && r < 10.0f) v = 0.1f;
                else if (k < 0.3f + 0.6f / (r + 1.0f) && r < 10.0f && sinf(an * 4.0f) > 0.0f) v += 0.12f;
            }
            if (pressed) v -= 0.16f;
            bput(t, x, y, sh(R_STONE, v));
        }
    {                                                                /* a crack in from a corner, its far lip lit */
        int x = b - 1, y = b - 14;
        for (int k = 0; k < 18; k++) {
            float r = h01(k, 1, seed + 11);
            x -= 1; y += r < 0.45f ? 1 : 0;
            int px = x + o * 2, py = y + o * 2;
            bput(t, px, py, sh(R_STONE, pressed ? 0.02f : 0.06f)); bput(t, px, py + 1, sh(R_STONE, pressed ? 0.36f : 0.58f));
        }
    }
    static const int ST[4][2] = { { a + 3, a + 3 }, { b - 4, a + 3 }, { a + 3, b - 4 }, { b - 4, b - 4 } };
    for (int k = 0; k < 4; k++) {                                    /* the studs, domed, a ring of rust round each */
        int x = ST[k][0] + o * 2, y = ST[k][1] + o * 2;
        float d = pressed ? 0.25f : 0.0f;
        bput(t, x - 1, y - 1, sh(R_BRICK, 0.2f)); bput(t, x + 2, y + 2, sh(R_BRICK, 0.15f)); bput(t, x + 2, y - 1, sh(R_BRICK, 0.18f));
        bput(t, x, y, sh(R_IRON, 0.98f - d)); bput(t, x + 1, y, sh(R_IRON, 0.55f - d)); bput(t, x, y + 1, sh(R_IRON, 0.5f - d)); bput(t, x + 1, y + 1, sh(R_IRON, 0.16f));
        bput(t, x + 2, y + 1, sh(R_STONE, 0.1f)); bput(t, x + 1, y + 2, sh(R_STONE, 0.1f));
    }
    if (pressed)
        for (int k = 0; k < 40; k++) {                               /* dust puffed from the joint */
            int side = k & 3; float u = h01(k, 1, seed + 7) * (float)(b - a);
            int x = side == 0 ? a + (int)u : side == 1 ? b + 1 + (int)(h01(k, 2, seed) * 3.0f) : side == 2 ? a + (int)u : a - 2 - (int)(h01(k, 2, seed) * 3.0f);
            int y = side == 0 ? a - 2 - (int)(h01(k, 2, seed) * 3.0f) : side == 1 ? a + (int)u : side == 2 ? b + 1 + (int)(h01(k, 2, seed) * 3.0f) : a + (int)u;
            bput(t, x, y, sh(R_WARM, 0.45f + 0.2f * h01(k, 3, seed)));
        }
}

/* each level's: once crawl_art_run has made the run's door (T_RDOOR), in the run's look */
static void crawl_art_lights(int level) {
    g_lrun = crawl_set(level);
    for (int f = 0; f < 3; f++) {
        spr_ironcorona(SP_CORONA0 + f, f);
        if (f < 2) spr_sanctuary(SP_SANCT0 + f, f);
        spr_bonelabrum(SP_BONELAB0 + f, f);
        spr_cagelantern(SP_CAGE0 + f, f);
    }
    gen_barred(crawl_tex[T_BARRED], crawl_tex[T_RDOOR], 0, 851); gen_barred(crawl_tex[T_UNBARRED], crawl_tex[T_RDOOR], 1, 851);
    spr_relic(SP_EV_RELIC);
    spr_socket(SP_EV_SOCKET0, 0); spr_socket(SP_EV_SOCKET1, 1);
    spr_lever(SP_EV_LEVER0, 0); spr_lever(SP_EV_LEVER1, 1);
    gen_plate(crawl_tex[T_PLATE], 0, 861); gen_plate(crawl_tex[T_PLATE_DOWN], 1, 861);
}
