/* The new hanging lights and the events' things, as sheets for the owner (docs/PLAN-lights.md, step 2; docs/PLAN-events.md).
 * Nothing here is in the game until the owner has seen it. The art is #included whole, so the drawings below use its canvas
 * (cv_*), its candles, chains, skulls and bones as a part of it would, and can move into src/art/parts/ as they stand: where
 * they keep a picture with lm_keep, the game's would be cv_store(SP_...), and where they fill a buffer, tput(T_...).
 *
 *   build/lightmock lights media/sheets/lights/hanging_lights.png
 *   build/lightmock things media/sheets/events/event_things.png
 */
#include "crawl_art.c"
#include <stdio.h>
#include <zlib.h>

/* ================================================================== */
/* The drawings                                                        */
/* ================================================================== */

static int g_lrun;                                      /* the run drawn for: 0 the Crypts, 1 the Catacombs, 2 the Hollows, 3 the Deep */

static void lm_keep(uint8_t *out) {                     /* cv_store's work, into a buffer: column-major */
    for (int x = 0; x < TS; x++)
        for (int y = 0; y < TS; y++) out[x * TS + y] = cv_sprite[y * TS + x];
}

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
static void hang_wash(uint8_t *out, uint32_t seed) {             /* the Catacombs' water, by prop_wash, on a spare slot */
    if (g_lrun != 1) return;
    static uint8_t keep[TS * TS];
    const int slot = SP_DOUBLE;
    memcpy(keep, crawl_spr[slot], TS * TS); memcpy(crawl_spr[slot], out, TS * TS);
    prop_wash(slot, 1, seed);
    memcpy(out, crawl_spr[slot], TS * TS); memcpy(crawl_spr[slot], keep, TS * TS);
}

/* ---- the iron corona (halls and naves): a great wheel of wrought iron on four chains from one hook; two bands, a zigzag of
 * bars between them; eight tall spikes barbed like fleurs, eight candles on prickets between them, a drop under each spike;
 * six spokes to a boss and its long finial; wax hanging from the rim. frame 0-2 ---- */
static void spr_ironcorona(uint8_t *out, int frame) {
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
    lm_keep(out);
    hang_wash(out, 763);
}

/* ---- the sanctuary lamp (chapels, burial vaults): a brass rose of five lobes on the vault, a smoke bell under it ringed with
 * mouldings, pierced for the smoke and beaded at its rim; three fine chains down to a brass cup of gadroons, each lobe turned to
 * the light, a beaded lip, verdigris in its seams; in it a beaker of red glass with a small flame inside, burning low and steady;
 * a banded knop and an acorn drop below. frame 0-1 ---- */
static void spr_sanctuary(uint8_t *out, int frame) {
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
    lm_keep(out);
    hang_wash(out, 773);
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
static void spr_bonelabrum(uint8_t *out, int frame) {
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
    lm_keep(out);
    hang_wash(out, 793);
}

/* ---- the pierced cage lantern (passages, small rooms): tall, of black iron, a spire of fish-scale plates with crockets and a
 * finial, a riveted eave; a body of six faces, three seen, of hammered plate, pierced through with lancets and a quatrefoil in
 * front and holes all over, the light coming through them and soot above each; riveted ribs and bands; a ringed cone below and
 * a drop. frame 0-2 ---- */
static void spr_cagelantern(uint8_t *out, int frame) {
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
    lm_keep(out);
    hang_wash(out, 813);
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
static void spr_relic(uint8_t *out) {
    cv_begin(cv_sprite, TS, TS);
    cv_flat(32, 61.5f, 15, 1.8f, R_WARM + 1);
    relic_bone(17, 59, 47, 52, 0);
    cv_outline(0);
    cv_px(31, 54, C_WHITE); cv_px(22, 55, C_WHITE);
    lm_keep(out);
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
static void spr_socket(uint8_t *out, int holding) {
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
    lm_keep(out);
}

/* the lever: a dressed stone post on a plinth course, its gabled head coped; on its face an iron quadrant of hammered plate,
 * riveted, toothed round its rim, rust in it; through its slot a twisted iron arm on a riveted boss, a turned grip of wood and a
 * ball at its end; up, it leans back over the post; pulled, it is down and out to the right */
static void spr_lever(uint8_t *out, int down) {
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
    lm_keep(out);
}

/* the pressure plate, a floor texture laid over the run's floor: a square flagstone of dressed stone set in a deep joint with grit
 * in it, tooled across, worn smooth where it is trodden, two corners chipped and a crack in from one; a border cut round it, its
 * lip lit where it faces the light; a roundel with a raised rose of eight cut in it and a boss at its heart; an iron stud, domed,
 * at each corner. Pressed: sunk a hand's breadth, darker, the pit's side showing at its far edges and dust puffed out at the joint */
static void gen_plate(uint8_t *t, const uint8_t *floor, int pressed, uint32_t seed) {
    memcpy(t, floor, TS * TS);
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

/* what the game draws today for a plate (draw/floor.c, FM_PLATE): the floor, its edge cut deep; sunk, darker */
static void today_plate(uint8_t *t, const uint8_t *floor, int pressed) {
    for (int x = 0; x < TS; x++)
        for (int y = 0; y < TS; y++) {
            float fx = ((float)x + 0.5f) / TS, fy = ((float)y + 0.5f) / TS, e = fminf(fminf(fx, 1.0f - fx), fminf(fy, 1.0f - fy));
            uint8_t c = floor[x * TS + y];
            if (e < 0.06f) c = R_STONE + 1;
            else if (e < 0.1f) c = (uint8_t)(R_STONE + (pressed ? 4 : 10));
            else if (pressed) { int ramp = (c - 1) / 16 * 16 + 1; c = (uint8_t)(ramp + clampi((int)((float)(c - ramp) * 0.65f), 1, 15)); }
            t[x * TS + y] = c;
        }
}

/* ================================================================== */
/* The sheet                                                           */
/* ================================================================== */

static int iw, ih;
static uint8_t *img;
static void put(int x, int y, int r, int g, int b) { if (x < 0 || y < 0 || x >= iw || y >= ih) return; uint8_t *p = img + ((size_t)y * iw + x) * 3; p[0] = r; p[1] = g; p[2] = b; }
static void rect(int x, int y, int w, int h, int r, int g, int b) { for (int yy = y; yy < y + h; yy++) for (int xx = x; xx < x + w; xx++) put(xx, yy, r, g, b); }
static void text(int x, int y, int sc, const char *s, int r, int g, int b) {
    for (; *s; s++, x += 8 * sc)
        for (int row = 0; row < 8; row++)
            for (int col = 0; col < 8; col++)
                if (font_bios8[(uint8_t)*s][row] >> (7 - col) & 1) rect(x + col * sc, y + row * sc, sc, sc, r, g, b);
}
#define INK 216, 208, 189
#define GOLDINK 233, 194, 76
#define DIMINK 150, 140, 128
static void pal_put(int x, int y, uint8_t c) { put(x, y, crawl_pal[c][0] * 255 / 63, crawl_pal[c][1] * 255 / 63, crawl_pal[c][2] * 255 / 63); }

static const uint8_t BAYER4[4][4] = { { 0, 8, 2, 10 }, { 12, 4, 14, 6 }, { 3, 11, 1, 9 }, { 15, 7, 13, 5 } };
static uint8_t shadeb(uint8_t c, float light, int x, int y) {   /* as draw/floor.c's shade */
    if (IS_FULLBRIGHT(c)) return c;
    int lv = (int)(light * LIGHT_UNIT + BAYER4[y & 3][x & 3] * (1.0f / 16.0f));
    return crawl_cmap[clampi(lv, 0, NLIGHT - 1)][c];
}

/* a cell enlarged: a backdrop (the run's wall over its floor), and over it a sprite or a face, at a light */
static void cell(int ox, int oy, int sc, const uint8_t *wall, const uint8_t *floor, const uint8_t *spr, const uint8_t *face, float light) {
    int n = TS * sc;
    for (int y = 0; y < n; y++)
        for (int x = 0; x < n; x++) {
            int tx = x / sc, ty = y / sc;
            uint8_t c;
            if (face) c = face[tx * TS + ty];
            else {
                c = ty < 52 ? wall[tx * TS + ty] : floor[tx * TS + ((ty - 52) * 5) % TS];
                float l = light * (ty < 52 ? 0.8f : 0.9f);
                c = crawl_cmap[clampi((int)(l * LIGHT_UNIT), 0, NLIGHT - 1)][c];
                if (spr && spr[tx * TS + ty] != C_CLEAR) c = spr[tx * TS + ty];
                else { pal_put(ox + x, oy + y, c); continue; }
            }
            pal_put(ox + x, oy + y, IS_FULLBRIGHT(c) ? c : crawl_cmap[clampi((int)(light * LIGHT_UNIT), 0, NLIGHT - 1)][c]);
        }
}

/* the game's view at its own size (320x200's pixels, FOCAL as the game's): a back wall at wd cells, the floor and vault
 * between; a sprite on the floor at sd cells, or a face in the wall's middle cell, or a texture on the floor's cell under the
 * sprite's place. The light: amb everywhere and a light at (lx, ly) of pw falling off over lr cells; dapple throws it
 * through a pierced lantern's holes. */
typedef struct {
    const uint8_t *wall, *floor, *ceil, *spr, *face, *flat;
    float wd, sd, amb, lx, ly, lz, pw, lr; int dapple;
} View;
static float view_light(const View *v, float x, float y, float z) {
    float dx = x - v->lx, dy = y - v->ly, dz = z - v->lz, d2 = dx * dx + dy * dy + dz * dz * 0.5f;
    float l = v->pw * expf(-d2 / (v->lr * v->lr));
    if (v->dapple) {                                                 /* the holes' light: spots on what it falls on */
        float a = atan2f(dy, dx), e = atan2f(dz, sqrtf(dx * dx + dy * dy));
        float s = 0.5f + 0.5f * sinf(a * 14.0f) * sinf(e * 12.0f + 1.0f);
        l *= 0.35f + 1.1f * (s > 0.62f ? 1.0f : s * 0.5f);
    }
    return v->amb + l;
}
static void view(int ox, int oy, int w, int h, const View *v) {
    const float F = (320 / 2) / 0.75f, camz = 0.5f, hz = (float)h * 0.5f;
    float top = hz - (1.0f - camz) * F / v->wd, bot = hz + camz * F / v->wd;
    for (int y = 0; y < h; y++)
        for (int x = 0; x < w; x++) {
            float cxp = (float)x + 0.5f - (float)w * 0.5f;
            uint8_t c; float l;
            if ((float)y >= top && (float)y < bot) {                 /* the back wall */
                float wx = cxp * v->wd / F + 0.5f, wy = ((float)y - top) / (bot - top);
                int tx = (int)floorf((wx - floorf(wx)) * TS), ty = clampi((int)(wy * TS), 0, TS - 1), mid = wx >= 0.0f && wx < 1.0f;
                c = (mid && v->face ? v->face : v->wall)[tx * TS + ty];
                l = view_light(v, wx - 0.5f, v->wd, 1.0f - wy) * (1.0f / (1.0f + 0.01f * v->wd * v->wd));
            } else {
                int fl = (float)y >= bot;
                float dist = (fl ? camz : 1.0f - camz) * F / fabsf((float)y + 0.5f - hz);
                float wx = cxp * dist / F + 0.5f, wy = dist;
                int tx = (int)floorf((wx - floorf(wx)) * TS) & 63, ty = (int)floorf((wy - floorf(wy)) * TS) & 63;
                const uint8_t *t = fl ? v->floor : v->ceil;
                c = t[tx * TS + ty];
                if (fl && v->flat && wx >= 0.0f && wx < 1.0f && wy >= v->sd - 0.5f && wy < v->sd + 0.5f) {
                    uint8_t k = v->flat[tx * TS + ((int)floorf((v->sd + 0.5f - wy) * TS) & 63)];
                    if (k != C_CLEAR) c = k;
                }
                l = view_light(v, wx - 0.5f, wy, fl ? 0.0f : 1.0f) * (1.0f / (1.0f + 0.01f * dist * dist)) * (fl ? 1.0f : 0.85f);
            }
            pal_put(ox + x, oy + y, shadeb(c, l, x, y));
        }
    if (v->spr) {                                                    /* the sprite, a cell high, on the floor at sd */
        float size = F / v->sd, sx0 = (float)w * 0.5f - size * 0.5f, sy1 = hz + camz * F / v->sd, sy0 = sy1 - size;
        float l = view_light(v, 0.0f, v->sd, 0.5f) * (1.0f / (1.0f + 0.01f * v->sd * v->sd));
        for (int y = (int)ceilf(sy0); y < (int)ceilf(sy1); y++)
            for (int x = (int)ceilf(sx0); x < (int)ceilf(sx0 + size); x++) {
                if (x < 0 || y < 0 || x >= w || y >= h) continue;
                int tx = clampi((int)(((float)x + 0.5f - sx0) / size * TS), 0, TS - 1), ty = clampi((int)(((float)y + 0.5f - sy0) / size * TS), 0, TS - 1);
                uint8_t c = v->spr[tx * TS + ty];
                if (c != C_CLEAR) pal_put(ox + x, oy + y, shadeb(c, l, x, y));
            }
    }
}

static void put32(FILE *f, uint32_t v) { uint8_t b[4] = { v >> 24, v >> 16, v >> 8, v }; fwrite(b, 1, 4, f); }
static void chunk(FILE *f, const char *type, const uint8_t *data, uint32_t len) {
    put32(f, len); fwrite(type, 1, 4, f); if (len) fwrite(data, 1, len, f);
    uint32_t crc = crc32(0, (const Bytef *)type, 4); if (len) crc = crc32(crc, data, len); put32(f, crc);
}
static int write_png(const char *path) {
    size_t rowlen = 1 + (size_t)iw * 3;
    uint8_t *raw = malloc(rowlen * ih);
    for (int y = 0; y < ih; y++) { raw[rowlen * y] = 0; memcpy(raw + rowlen * y + 1, img + (size_t)y * iw * 3, (size_t)iw * 3); }
    uLongf clen = compressBound((uLong)(rowlen * ih)); uint8_t *comp = malloc(clen);
    compress2(comp, &clen, raw, (uLong)(rowlen * ih), 9);
    FILE *f = fopen(path, "wb"); if (!f) { free(raw); free(comp); return 0; }
    static const uint8_t sig[8] = { 0x89, 'P', 'N', 'G', 13, 10, 26, 10 }; fwrite(sig, 1, 8, f);
    uint8_t ihdr[13] = { iw >> 24, iw >> 16, iw >> 8, iw, ih >> 24, ih >> 16, ih >> 8, ih, 8, 2, 0, 0, 0 };
    chunk(f, "IHDR", ihdr, 13); chunk(f, "IDAT", comp, (uint32_t)clen); chunk(f, "IEND", NULL, 0);
    fclose(f); free(raw); free(comp); return 1;
}

static const int RUNLEVEL[4] = { 3, 13, 23, 33 };
static const char *const RUNA[4] = { "THE", "THE", "THE", "THE" };
static const char *const RUNB[4] = { "CRYPTS", "CATACOMBS", "HOLLOWS", "DEEP" };
static void run_art(int r) { crawl_art_level(RUNLEVEL[r]); g_lrun = r; }
static void text_lines(int x, int y, int sc, const char *const *l, int n) { for (int i = 0; i < n; i++) text(x, y + i * 10 * sc, sc, l[i], INK); }

/* ---- sheet 1: the four hanging lights ---- */
#define SC 2
#define CW (TS * SC)
#define PAD 8
#define LAB 168
#define VW 200
#define VH 128
#define VH2 160
#define LIT 1.15f
#define DARK 0.55f
static uint8_t LT[4][4][3][TS * TS];                     /* [light][run][frame] */
static const int NFR[4] = { 3, 2, 3, 3 };
static void make_lights(int r) {
    run_art(r);
    for (int f = 0; f < 3; f++) {
        spr_ironcorona(LT[0][r][f], f);
        if (f < 2) spr_sanctuary(LT[1][r][f], f);
        spr_bonelabrum(LT[2][r][f], f);
        spr_cagelantern(LT[3][r][f], f);
    }
}
static int sheet_lights(const char *path) {
    static const char *const NAME[4] = { "THE IRON CORONA", "THE SANCTUARY LAMP", "THE BONE CANDELABRUM", "THE PIERCED CAGE LANTERN" };
    static const char *const WHERE[4] = {
        "A GREAT WHEEL OF WROUGHT IRON ON FOUR CHAINS: TWO BANDS BARRED BETWEEN, EIGHT BARBED SPIKES, EIGHT CANDLES, SIX",
        "RED GLASS IN A BRASS CUP ON THREE FINE CHAINS FROM A SMOKE BELL, A SMALL FLAME IN IT: A LOW, STEADY GLOW.",
        "A GREAT SKULL WITH A STAG'S ANTLERS BOUND TO IT, A SKULL HUNG UNDER EACH END, RIBS ROUND A SPINE, A JAW AT",
        "TALL, OF BLACK IRON: A CROCKETED SPIRE, A LANCET OVER A QUATREFOIL IN FRONT, HOLES ROUND THE SIDES; ITS",
    };
    static const char *const WHERE2[4] = {
        "SPOKES TO A BOSS AND ITS DROP. FOR HALLS AND NAVES. ITS LIGHT AS THE CHANDELIER'S, OR A LITTLE WIDER.",
        "FOR CHAPELS AND BURIAL VAULTS. ITS LIGHT SMALL AND RED (A COLOUR TO SETTLE IN THE GAME'S STEP).",
        "THE FOOT; A CANDLE IN EACH EYE OF EACH SKULL. FOR OSSUARIES AND CHARNEL PITS.",
        "LIGHT COMES THROUGH THE HOLES. FOR PASSAGES AND SMALL ROOMS.",
    };
    int bandh = 62 + 4 * (CW + PAD + 4);
    iw = PAD + LAB + 4 * (CW + PAD) + VW + PAD * 2; ih = 176 + (30 + CW + 26) + 4 * bandh + 8;
    img = calloc((size_t)iw * ih * 3, 1);
    rect(0, 0, iw, ih, 16, 14, 20);
    text(PAD, 12, 2, "FOUR HANGING LIGHTS, FOR THE OWNER'S LOOK", GOLDINK);
    text(PAD + 42 * 16, 16, 1, "(PLAN-LIGHTS, STEP 2)", DIMINK);
    static const char *const HEAD[] = {
        "NOTHING HERE IS IN THE GAME. EACH IS DRAWN IN EACH RUN'S PALETTE: THE CATACOMBS' WATER RUSTS AND DRIPS ON IT AS",
        "ON THE CHANDELIER, THE HOLLOWS' SOOT DARKENS IT, THE DEEP BURNS BLACK CANDLES ON IRON GONE GLASSY. THE FRAMES ARE",
        "THE FLICKER; THE SWAY IS THE GAME'S, AS THE CHANDELIER'S. EACH FRAME TWICE ITS SIZE IN ITS OWN LIGHT; THEN THE FIRST",
        "DARKER, AS FROM A HALL'S FAR END; THEN AT THE GAME'S OWN SIZE, A CROP OF ITS 320X200 VIEW: THE LIGHT HUNG TWO AND",
        "A HALF CELLS OFF, A WALL FOUR AWAY. THE CAGE'S VIEW THROWS ITS HOLES' LIGHT IN SPOTS, AS IT MIGHT (NOT BUILT).",
    };
    text_lines(PAD, 44, 1, HEAD, 5);
    for (int r = 0; r < 4; r++) make_lights(r);
    /* for scale: the chandelier and the lantern the game hangs today, and the four new, the Crypts' */
    int y = 112;
    text(PAD, y, 2, "FOR SCALE, IN THE CRYPTS", GOLDINK);
    text(PAD + 25 * 16, y + 4, 1, "TODAY'S CHANDELIER AND LANTERN, THEN THE FOUR", INK);
    y += 30;
    run_art(0);
    const uint8_t *wall = crawl_tex[T_MATV(0, 0)], *floor = crawl_tex[T_FLOORS];
    static const char *const SCALE[6] = { "CHANDELIER", "LANTERN", "CORONA", "SANCTUARY", "BONES", "CAGE" };
    for (int k = 0; k < 6; k++) {
        int ox = PAD + k * (CW + PAD) + (k >= 2 ? 16 : 0);
        const uint8_t *s = k == 0 ? crawl_spr[SP_CHANDELIER_A] : k == 1 ? crawl_spr[SP_LANTERN_A] : LT[k - 2][0][0];
        cell(ox, y, SC, wall, floor, s, NULL, LIT);
        if (k < 2) text(ox, y + CW + 6, 1, SCALE[k], DIMINK); else text(ox, y + CW + 6, 1, SCALE[k], INK);
    }
    y += CW + 30;
    for (int L = 0; L < 4; L++) {
        text(PAD, y + 4, 2, NAME[L], GOLDINK);
        text(PAD, y + 24, 1, WHERE[L], INK); text(PAD, y + 34, 1, WHERE2[L], INK);
        y += 62;
        for (int c = 0; c < 4; c++) {
            const char *h = c < 3 ? (c < NFR[L] ? (c == 0 ? "FRAME 1" : c == 1 ? "FRAME 2" : "FRAME 3") : "") : "DARKER";
            if (c < 3) text(PAD + LAB + c * (CW + PAD), y - 12, 1, h, INK); else text(PAD + LAB + c * (CW + PAD), y - 12, 1, h, DIMINK);
        }
        text(PAD + LAB + 4 * (CW + PAD), y - 12, 1, "AT 1X, IN THE GAME'S VIEW", INK);
        for (int r = 0; r < 4; r++) {
            run_art(r);
            wall = crawl_tex[T_MATV(0, 0)]; floor = crawl_tex[T_FLOORS];
            text(PAD, y + CW / 2 - 18, 2, RUNA[r], INK); text(PAD, y + CW / 2 + 2, 2, RUNB[r], INK);
            for (int c = 0; c < 4; c++) {
                if (c < 3 && c >= NFR[L]) continue;
                cell(PAD + LAB + c * (CW + PAD), y, SC, wall, floor, LT[L][r][c < 3 ? c : 0], NULL, c < 3 ? LIT : DARK);
            }
            View v = { wall, floor, crawl_tex[T_CEILS], LT[L][r][0], NULL, NULL, 4.0f, 2.5f, 0.32f, 0.0f, 2.5f, 0.55f, 1.0f, 2.2f, L == 3 };
            view(PAD + LAB + 4 * (CW + PAD), y, VW, VH, &v);
            y += CW + PAD + 4;
        }
    }
    if (!write_png(path)) { fprintf(stderr, "cannot write %s\n", path); return 1; }
    printf("%s: %dx%d\n", path, iw, ih);
    return 0;
}

/* ---- sheet 2: the events' things ---- */
#define NTH 9
static int sheet_things(const char *path) {
    static const char *const COLA[NTH] = { "BARRED", "UNBARRED", "RELIC", "SOCKET", "SOCKET", "LEVER", "LEVER", "PLATE", "PLATE" };
    static const char *const COLB[NTH] = { "DOOR", "DOOR", "", "EMPTY", "HOLDING", "UP", "DOWN", "UP", "PRESSED" };
    iw = PAD + LAB + NTH * (CW + PAD) + PAD; ih = 150 + 6 * (CW + PAD + 6) + 40 + 2 * (VH2 + 22) + 12;
    img = calloc((size_t)iw * ih * 3, 1);
    rect(0, 0, iw, ih, 16, 14, 20);
    text(PAD, 12, 2, "THE EVENTS' THINGS, FOR THE OWNER'S LOOK (PLAN-EVENTS; PLAN-LIGHTS, STEP 2)", GOLDINK);
    static const char *const HEAD[] = {
        "NOTHING HERE IS IN THE GAME. TODAY THESE BORROW THE PUZZLES' LOOKS (THE TOP ROW). THE BARRED DOOR IS THE RUN'S OWN DOOR WITH A BEAM ACROSS IT",
        "IN TWO STAPLES AND A CHAIN CROSSED OVER IT TO AN IRON PLATE, A HAND CUT IN THE PLATE: NO KEYHOLE, SO NOTHING SAYS A KEY OPENS IT. UNBARRED (AN",
        "EVENT'S UNLOCK, BEFORE IT SLIDES): THE BEAM AND PLATE GONE, A CHAIN HUNG LOOSE. THE RELIC IS A BONE BOUND IN GOLD, A KEY'S TEETH AT ITS END, A RED",
        "SEAL ON A CORD. THE SOCKET IS A PLINTH WITH TWO STONE HANDS HELD UP ON IT AND THE RELIC'S SHAPE CUT IN ITS FACE; HOLDING, THE RELIC LIES ACROSS",
        "THE HANDS AND THE SHAPE IS ALIGHT. THE LEVER: A STONE POST, A TOOTHED IRON QUADRANT, A TWISTED ARM. THE PLATE IS A FLOOR TEXTURE: A FLAGSTONE",
        "IN A DEEP JOINT, A ROSE CUT IN A ROUNDEL, IRON STUDS; PRESSED, SUNK AND DARKER, THE PIT'S SIDE SHOWING, DUST PUFFED FROM THE JOINT.",
    };
    text_lines(PAD, 40, 1, HEAD, 6);
    int y = 116;
    for (int c = 0; c < NTH; c++) { text(PAD + LAB + c * (CW + PAD), y, 1, COLA[c], INK); text(PAD + LAB + c * (CW + PAD), y + 10, 1, COLB[c], INK); }
    y += 26;
    static uint8_t T[NTH][TS * TS], P[2][TS * TS], tmp[TS * TS];
    for (int row = 0; row < 6; row++) {                              /* today; the four runs; the Crypts darker */
        int r = row == 0 || row == 5 ? 0 : row - 1;
        run_art(r);
        const uint8_t *wall = crawl_tex[T_MATV(0, 0)], *floor = crawl_tex[T_FLOORS];
        if (row == 0) {
            text(PAD, y + CW / 2 - 18, 2, "TODAY", DIMINK); text(PAD, y + CW / 2 + 2, 1, "THE CRYPTS", DIMINK);
            cell(PAD + LAB, y, SC, wall, floor, NULL, crawl_tex[T_GATE], LIT);
            cell(PAD + LAB + 2 * (CW + PAD), y, SC, wall, floor, crawl_spr[SP_PZ_FOSSIL], NULL, LIT);
            cell(PAD + LAB + 3 * (CW + PAD), y, SC, wall, floor, crawl_spr[SP_PZ_PLINTH], NULL, LIT);
            memcpy(tmp, crawl_spr[SP_PZ_PLINTH], TS * TS);
            for (int i = 0; i < TS * TS; i++) { int x = i / TS, yy = i % TS; if (yy + 17 < TS) { uint8_t c = crawl_spr[SP_PZ_FOSSIL][x * TS + yy + 17]; if (c != C_CLEAR && c != R_WARM + 1) tmp[i] = c; } }
            cell(PAD + LAB + 4 * (CW + PAD), y, SC, wall, floor, tmp, NULL, LIT);
            cell(PAD + LAB + 5 * (CW + PAD), y, SC, wall, floor, crawl_spr[SP_PZ_LEVER0], NULL, LIT);
            cell(PAD + LAB + 6 * (CW + PAD), y, SC, wall, floor, crawl_spr[SP_PZ_LEVER1], NULL, LIT);
            today_plate(P[0], floor, 0); today_plate(P[1], floor, 1);
            cell(PAD + LAB + 7 * (CW + PAD), y, SC, NULL, NULL, NULL, P[0], LIT);
            cell(PAD + LAB + 8 * (CW + PAD), y, SC, NULL, NULL, NULL, P[1], LIT);
            text(PAD + LAB + 1 * (CW + PAD) + 8, y + CW / 2 - 4, 1, "(IT SLIDES)", DIMINK);
            y += CW + PAD + 18;
            continue;
        }
        gen_barred(T[0], crawl_tex[T_RDOOR], 0, 851); gen_barred(T[1], crawl_tex[T_RDOOR], 1, 851);
        spr_relic(T[2]); spr_socket(T[3], 0); spr_socket(T[4], 1); spr_lever(T[5], 0); spr_lever(T[6], 1);
        gen_plate(T[7], floor, 0, 861); gen_plate(T[8], floor, 1, 861);
        if (row == 5) { text(PAD, y + CW / 2 - 18, 2, "DARKER", DIMINK); text(PAD, y + CW / 2 + 2, 1, "THE CRYPTS", DIMINK); }
        else { text(PAD, y + CW / 2 - 18, 2, RUNA[r], INK); text(PAD, y + CW / 2 + 2, 2, RUNB[r], INK); }
        for (int c = 0; c < NTH; c++) {
            int face = c < 2 || c > 6;
            cell(PAD + LAB + c * (CW + PAD), y, SC, wall, floor, face ? NULL : T[c], face ? T[c] : NULL, row == 5 ? DARK : LIT);
        }
        y += CW + PAD + 6;
    }
    y += 12;
    text(PAD, y, 2, "AT 1X, IN THE GAME'S VIEW (A CROP OF 320X200)", GOLDINK);
    y += 40;
    static const char *const VL[6] = { "THE DOOR, 3 CELLS OFF", "UNBARRED, 3 OFF", "THE RELIC, 2 OFF", "THE SOCKET HOLDING, 2.5", "THE LEVER DOWN, 2.5", "THE PLATE, 2 OFF" };
    for (int row = 0; row < 2; row++) {
        int r = row ? 3 : 0;
        run_art(r);
        const uint8_t *wall = crawl_tex[T_MATV(0, 0)], *floor = crawl_tex[T_FLOORS];
        gen_barred(T[0], crawl_tex[T_RDOOR], 0, 851); gen_barred(T[1], crawl_tex[T_RDOOR], 1, 851);
        spr_relic(T[2]); spr_socket(T[4], 1); spr_lever(T[6], 1); gen_plate(T[8], floor, 0, 861);
        text(PAD, y + VH2 / 2 - 18, 2, RUNA[r], INK); text(PAD, y + VH2 / 2 + 2, 2, RUNB[r], INK);
        for (int k = 0; k < 6; k++) {
            View v = { wall, floor, crawl_tex[T_CEILS], NULL, NULL, NULL, 4.0f, 2.0f, 0.45f, 0.6f, 1.2f, 0.6f, 0.75f, 2.6f, 0 };
            if (k < 2) { v.face = T[k]; v.wd = 3.0f; }
            else if (k == 5) { v.flat = T[8]; v.sd = 2.0f; }
            else { v.spr = T[k == 2 ? 2 : k == 3 ? 4 : 6]; v.sd = k == 2 ? 2.0f : 2.5f; }
            int ox = PAD + LAB + k * (VW + 4);
            view(ox, y, VW, VH2, &v);
            if (row == 0) text(ox, y - 12, 1, VL[k], INK);
        }
        y += VH2 + 22;
    }
    if (!write_png(path)) { fprintf(stderr, "cannot write %s\n", path); return 1; }
    printf("%s: %dx%d\n", path, iw, ih);
    return 0;
}

int main(int argc, char **argv) {
    if (argc < 3) { fprintf(stderr, "usage: %s lights|things out.png\n", argv[0]); return 1; }
    crawl_art_init();
    if (!strcmp(argv[1], "lights")) return sheet_lights(argv[2]);
    if (!strcmp(argv[1], "things")) return sheet_things(argv[2]);
    fprintf(stderr, "lights or things\n");
    return 1;
}
