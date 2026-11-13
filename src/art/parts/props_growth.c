/* A part of crawl_art.c (docs/PLAN-tidy.md): it is #included there in its place and never compiled on its own.
 * Props: hanging roots, webs and cocoons, ice, the gothic pillar, flowstone, the altar stone, urns, broken arms, the spiked collar; the roost asleep. */

/* a root come through the vault from (x, y0), len long, w thick at its head: twisting as it goes, the bark furrowed along it,
 * a knot or two, rootlets off its sides; forks: how many lesser roots go off it */
static void hang_root(float x, float y0, float len, float w, float ph, uint32_t seed, int forks) {
    float drift = (h01(1, 1, seed) - 0.5f) * 8.0f;
    for (int y = (int)y0; y < (int)(y0 + len) && y < TS; y++) {
        float u = ((float)y - y0) / len, xx = x + sinf((float)y * 0.21f + ph) * (1.0f + 2.4f * u) + drift * u * u, ww = w * (1.0f - u * 0.82f);
        for (int dx = (int)(-ww - 1); dx <= (int)(ww + 1); dx++) {
            float nx = ((float)dx + 0.5f + (xx - floorf(xx)) - 0.5f) / (ww + 0.01f);
            if (fabsf(nx) > 1.0f) continue;
            float v = 0.1f + 0.36f * lit(nx, 0.0f, sqrtf(1.0f - nx * nx)) - 0.06f * u;
            if (ww > 1.4f && h01((int)(nx * 3.0f + 3.0f), (y + (int)(nx * 2.0f)) / 3, seed + 2) < 0.3f) v -= 0.09f;   /* furrows */
            cv_px((int)xx + dx, y, sh(R_WOOD, v));
        }
        if (ww > 1.5f && h01(y, 3, seed) > 0.96f) {                 /* a knot */
            cv_ellipse(xx + 0.3f, (float)y, ww * 0.7f, 1.6f, R_WOOD, 0.08f, 0.4f);
            cv_px((int)(xx + 0.3f), y, sh(R_WOOD, 0.04f));
        }
        if (h01(y, 4, seed) > 0.9f) {                               /* a rootlet */
            float s = h01(y, 5, seed) < 0.5f ? -1.0f : 1.0f, l = 3.0f + 5.0f * h01(y, 6, seed);
            cv_line(xx + s * ww, (float)y, xx + s * (ww + l), (float)y + l * 0.9f, sh(R_WOOD, 0.14f));
        }
        if (forks > 0 && u > 0.25f && u < 0.6f && h01(y, 7, seed) > 0.93f) {
            forks--;
            hang_root(xx + (h01(y, 8, seed) < 0.5f ? -1.0f : 1.0f) * ww * 0.6f, (float)y, (len - ((float)y - y0)) * (0.6f + 0.5f * h01(y, 9, seed)), ww * 0.55f, ph + 2.0f, seed * 3u + (uint32_t)y, 0);
        }
    }
}
/* the earth of the vault the roots come through: a ragged band, stones in it, clods hanging off it */
static void root_soil(uint32_t seed) {
    for (int x = 0; x < TS; x++) {
        int d = 2 + (int)(4.0f * fbm((float)x / 6.0f, 0.5f, 16, seed));
        for (int y = 0; y <= d; y++) cv_px(x, y, sh(R_WARM, 0.06f + 0.2f * h01(x, y, seed + 1) + 0.08f * (float)(y == d)));
        if (h01(x / 3, 2, seed) > 0.85f) cv_px(x, d + 1, sh(R_WARM, 0.2f));
    }
    for (int k = 0; k < 4; k++) cv_ellipse(6.0f + 52.0f * h01(k, 3, seed), 2.0f + 1.5f * h01(k, 4, seed), 1.8f, 1.3f, R_STONE, 0.15f, 0.6f);
}
/* soil falling from (x, y0): grains thinning as they go, a clod among them */
static void soil_fall(float x, int y0, int y1, uint32_t seed) {
    for (int y = y0; y < y1; y++)
        if (h01(y, 1, seed) < 0.42f * (1.0f - (float)(y - y0) / (float)(y1 - y0)) + 0.06f)
            cv_px((int)(x + (h01(y, 2, seed) - 0.5f) * 2.5f), y, sh(R_WARM, 0.22f + 0.18f * h01(y, 3, seed)));
    cv_ellipse(x + 0.5f, (float)(y0 + (y1 - y0) / 2), 1.2f, 1.1f, R_WARM, 0.1f, 0.4f);
}
/* roots through the vault. look 0: five roots, a skull of the dead above held in them, a rootlet in its eye; look 1: one great
 * root bearded with hair-roots, a board of the coffin it came through caught in it, its nail still in */
static void spr_hangroots(int id, int look) {
    cv_begin(cv_sprite, TS, TS);
    if (look == 0) {
        prop_skull(25, 21, 0, 0, 2, 0);
        static const float X[5] = { 8, 21, 34, 46, 57 }, LN[5] = { 22, 40, 33, 46, 18 }, W[5] = { 2.2f, 3.0f, 2.6f, 3.4f, 1.8f };
        for (int r = 0; r < 5; r++) hang_root(X[r], 2, LN[r], W[r], (float)r * 1.7f, 471 + (uint32_t)r, r == 1 || r == 3);
        cv_line(28.0f, 16.0f, 28.0f, 26.5f, sh(R_WOOD, 0.24f));      /* into its socket */
        cv_line(31.5f, 12.0f, 33.5f, 21.0f, sh(R_WOOD, 0.3f)); cv_line(33.5f, 21.0f, 37.0f, 24.0f, sh(R_WOOD, 0.2f));
        soil_fall(40, 7, 44, 473);
    } else {
        hang_root(30, 2, 30, 5.0f, 0.4f, 481, 2);
        hang_root(12, 2, 20, 2.0f, 2.0f, 483, 0); hang_root(52, 2, 26, 2.4f, 3.1f, 485, 1);
        for (int k = 0; k < 16; k++) {                               /* its beard */
            float x0 = 25.0f + 11.0f * h01(k, 1, 487), y0 = 22.0f + 8.0f * h01(k, 2, 487), l = 14.0f + 20.0f * h01(k, 3, 487);
            for (int y = 0; y < (int)l; y++) if (h01(k, y, 489) > 0.12f) cv_px((int)(x0 + sinf((float)y * 0.3f + (float)k) * 1.2f + (float)y * (h01(k, 4, 487) - 0.5f) * 0.25f), (int)y0 + y, sh(R_WOOD, 0.2f + 0.1f * h01(k, y / 4, 491) - 0.006f * (float)y));
        }
        for (int y = 15; y < 20; y++)                               /* the board, split, slant across the roots */
            for (int x = 16; x < 33; x++) {
                float yy = (float)y + ((float)x - 16.0f) * 0.22f;
                if (x > 29 && h01(x, y, 493) < 0.45f) continue;     /* splintered */
                cv_px(x, (int)yy, sh(R_WARM, (y == 15 ? 0.46f : 0.3f) - (y == 19 ? 0.16f : 0.0f) + (fmodf((float)x * 0.7f + (float)y, 3.0f) < 0.5f ? -0.06f : 0.0f)));
            }
        cv_line(20, 13, 20, 16.5f, sh(R_IRON, 0.75f)); cv_px(20, 13, sh(R_IRON, 0.95f));   /* its nail */
        soil_fall(22, 8, 40, 495); soil_fall(45, 8, 30, 497);
    }
    root_soil(499 + (uint32_t)look);
    cv_outline(0);
    cv_store(id);
}

/* a web's thread from (x0, y0) to (x1, y1), sagging: lit where it faces the light, a pixel dropped here and there with wear */
static void web_line(float x0, float y0, float x1, float y1, float sag, float v, uint32_t seed) {
    int n = (int)fmaxf(fabsf(x1 - x0), fabsf(y1 - y0) + sag) + 1;
    float slope = fabsf(y1 - y0) / (fabsf(x1 - x0) + 0.5f);
    for (int i = 0; i <= n; i++) {
        float t = (float)i / (float)n;
        int x = (int)(x0 + (x1 - x0) * t), y = (int)(y0 + (y1 - y0) * t + sag * 4.0f * t * (1.0f - t));
        if (h01(x, y, seed) < 0.06f) continue;
        cv_px(x, y, (uint8_t)(R_STONE + clampi((int)(v * 15.0f) + ((i & 1) ? 0 : -2) + (slope < 1.0f ? 1 : 0), 4, 15)));
    }
}
/* dust caught on a thread at (x, y) */
static void web_dust(float x, float y, uint32_t seed) {
    for (int k = 0; k < 4; k++) cv_px((int)(x + (h01(k, 1, seed) - 0.5f) * 3.0f), (int)(y + h01(k, 2, seed) * 2.0f), sh(R_WARM, 0.3f + 0.2f * h01(k, 3, seed)));
}
/* a dead spider, curled, at (x, y) */
static void dead_spider(float x, float y) {
    cv_ellipse(x + 0.3f, y + 2.2f, 3.2f, 2.8f, R_WARM, 0.12f, 0.52f);
    cv_px((int)x, (int)y + 1, sh(R_WARM, 0.62f)); cv_px((int)x + 1, (int)y + 3, sh(R_BONE, 0.3f));
    cv_ellipse(x - 0.2f, y - 1.4f, 1.9f, 1.6f, R_WARM, 0.14f, 0.55f);
    for (int s = -1; s <= 1; s += 2)
        for (int l = 0; l < 4; l++) {
            float ax = x + (float)s * 1.4f, ay = y - 1.5f + (float)l * 0.9f, kx = ax + (float)s * (3.4f - 0.3f * (float)l), ky = ay - 1.5f + (float)l * 1.0f;
            cv_line(ax, ay, kx, ky, sh(R_WARM, 0.42f)); cv_line(kx, ky, kx - (float)s * 2.0f, ky + 2.4f, sh(R_WARM, 0.3f));   /* folded under */
        }
}
/* a web in a corner of the vault: spokes from a hub set off its middle to anchors along the stone, the spiral slung between
 * them, a sector torn and its threads hanging, dust caught in it. look 0 has a husk wound in it; look 1 is old and mostly
 * gone, a dead spider curled at the end of a thread */
static void spr_cornerweb(int id, int look) {
    cv_begin(cv_sprite, TS, TS);
    uint32_t sd = 501u + (uint32_t)look * 37u;
    float hx = look ? 34.0f : 28.0f, hy = look ? 7.0f : 10.0f;
    float ang[9], len[9];
    int ns = look ? 6 : 9;
    for (int s = 0; s < ns; s++) {                                  /* the spokes, from the stone round to the stone */
        ang[s] = PI_F * (-0.05f + 1.1f * (float)s / (float)(ns - 1)) + (h01(s, 1, sd) - 0.5f) * 0.18f;
        len[s] = (look ? 24.0f : 30.0f) + 8.0f * h01(s, 2, sd);
        float ex = hx + cosf(ang[s]) * len[s], ey = fmaxf(0.0f, hy + sinf(ang[s]) * len[s]);
        web_line(hx, hy, ex, ey, 0.0f, 0.78f, sd + 3);
    }
    for (int s = 0; s < 4; s++) web_line(hx, hy, 6.0f + 18.0f * (float)s, 0.0f, 0.0f, 0.7f, sd + 5);   /* to the vault */
    int rings = look ? 3 : 7;
    for (int r = 1; r <= rings; r++) {
        float f = (float)r / 7.6f;
        for (int s = 0; s + 1 < ns; s++) {
            if (!look && s >= 5 && s <= 6 && r > 3) {               /* torn: the threads hang */
                if (h01(r, s, sd + 7) < 0.6f) { float x0 = hx + cosf(ang[s]) * len[s] * f, y0 = hy + sinf(ang[s]) * len[s] * f; web_line(x0, y0, x0 + 1.5f, y0 + 5.0f + 6.0f * h01(r, s, sd + 8), 0.0f, 0.6f, sd + 9); }
                continue;
            }
            float x0 = hx + cosf(ang[s]) * len[s] * f, y0 = hy + sinf(ang[s]) * len[s] * f, x1 = hx + cosf(ang[s + 1]) * len[s + 1] * f, y1 = hy + sinf(ang[s + 1]) * len[s + 1] * f;
            web_line(x0, fmaxf(0.0f, y0), x1, fmaxf(0.0f, y1), 0.6f + 1.4f * f, 0.82f - 0.25f * f, sd + 11);
        }
    }
    if (!look) {
        web_dust(hx + 9.0f, hy + 9.0f, sd + 13); web_dust(hx - 14.0f, hy + 6.0f, sd + 14); web_dust(hx + 3.0f, hy + 20.0f, sd + 15);
        silk_husk(hx + 13.0f, hy + 18.0f, hx + 11.0f, hy + 8.0f, 1, 1);
    } else {
        for (int k = 0; k < 4; k++) {                               /* long threads, slung and hanging */
            float x0 = 4.0f + 14.0f * (float)k + 6.0f * h01(k, 1, sd), x1 = x0 + 10.0f + 12.0f * h01(k, 2, sd);
            web_line(x0, 0.0f, x1, 1.0f, 6.0f + 10.0f * h01(k, 3, sd), 0.62f, sd + 17);
            web_dust(x0 + (x1 - x0) * 0.5f, 6.0f + 9.0f * h01(k, 3, sd), sd + 19 + (uint32_t)k);
        }
        for (int k = 0; k < 5; k++) { float x = 10.0f + 10.0f * (float)k + 4.0f * h01(k, 5, sd); web_line(x, 2.0f, x + 1.0f, 12.0f + 14.0f * h01(k, 6, sd), 0.0f, 0.56f, sd + 23); }
        web_line(hx + 6.0f, hy + 4.0f, hx + 7.0f, 38.0f, 0.0f, 0.6f, sd + 25);
        dead_spider(hx + 7.0f, 40.0f);
    }
    cv_store(id);
}

/* a body wound in silk, hung at (x, y), hh long and ww across, its stuff showing through: bony fingers out of its side */
static void cocoon(float x, float y, float hh, float ww, uint32_t seed) {
    for (int j = (int)-hh; j <= (int)hh; j++) {
        float u = (float)j / (hh + 0.5f), w = ww * powf(fmaxf(0.0f, 1.0f - u * u), 0.7f) * (1.0f + 0.12f * sinf((float)j * 0.7f));
        for (int i = (int)floorf(x - w); i <= (int)ceilf(x + w); i++) {
            float nx = ((float)i + 0.5f - x) / (w + 0.01f);
            if (fabsf(nx) > 1.0f) continue;
            float v = 0.24f + 0.48f * lit(nx, u * 0.6f, sqrtf(fmaxf(0.0f, 1.0f - nx * nx))), band = fmodf((float)j * 0.8f + nx * 2.4f + 20.0f, 2.2f);
            if (band < 0.7f) v -= 0.16f;                            /* wound slantwise */
            if (h01(i, j + 40, seed) < 0.08f) v -= 0.2f;
            cv_px(i, (int)y + j, sh(R_BONE, v * 0.85f));
        }
    }
    for (int f = 0; f < 3; f++) {                                   /* fingers out through the silk */
        float bx = x + ww * 0.8f, by = y + 2.0f + (float)f * 1.6f;
        cv_line(bx, by, bx + 3.0f, by + 1.5f + (float)f * 0.5f, sh(R_BONE, 0.82f - 0.1f * (float)f));
        cv_line(bx + 3.0f, by + 1.5f + (float)f * 0.5f, bx + 4.0f, by + 3.5f + (float)f * 0.6f, sh(R_BONE, 0.7f - 0.1f * (float)f));
    }
}
/* a sheet of web. look 0: slung from the vault and sagging into a hammock, its weave thicker where it gathers, its edge a
 * cord, a hole torn in it, tatters hanging, a husk and a rib lying in its belly; look 1: a veil of threads hung straight
 * down, tied across here and there and gathered at their feet, a body wound in it with its fingers out */
static void spr_websheet2(int id, int look) {
    cv_begin(cv_sprite, TS, TS);
    uint32_t sd = 521u + (uint32_t)look * 41u;
    if (look == 0) {
        float hx = 41.0f, hy = 17.0f;
        for (int x = 0; x < TS; x++) {
            float t = (float)x / 63.0f, edge = 3.0f + 2.0f * t + 27.0f * 4.0f * t * (1.0f - t) * (0.85f + 0.15f * sinf((float)x * 0.4f));
            for (int y = 0; y <= (int)edge; y++) {
                float dd = (float)y / edge, ox = ((float)x - hx) / 6.5f, oy = ((float)y - hy) / 4.5f;
                if (ox * ox + oy * oy < 1.0f + 0.3f * h01(x, y, sd)) continue;   /* the hole */
                int mesh = fmodf((float)x + (float)y * 0.6f + 40.0f, 5.0f) < 1.0f || fmodf((float)x - (float)y * 0.7f + 80.0f, 6.0f) < 1.0f;
                float keep = mesh ? 0.45f + 0.5f * dd : 0.04f + 0.3f * dd * dd;
                if (h01(x, y, sd + 1) > keep) continue;
                cv_px(x, y, (uint8_t)(R_STONE + clampi(7 + (int)(dd * 5.0f) + (int)(h01(x, y, sd + 2) * 2.0f) - (x > 40) , 4, 14)));
            }
            cv_px(x, (int)edge, (uint8_t)(R_STONE + 13 - (x & 1)));        /* its edge, a cord */
            cv_px(x, (int)edge + 1, (uint8_t)(R_STONE + 9));
            if (h01(x / 2, 3, sd) > 0.78f) web_line((float)x, edge + 1.0f, (float)x + (h01(x, 4, sd) - 0.5f) * 3.0f, edge + 4.0f + 14.0f * h01(x, 5, sd), 0.0f, 0.58f, sd + 6);   /* tatters */
        }
        for (int k = 0; k < 6; k++) {                               /* the hole's frayed lip */
            float a = PI_F * (0.1f + 0.8f * (float)k / 5.0f), x0 = hx + cosf(a) * 6.5f, y0 = hy + sinf(a) * 4.5f;
            web_line(x0, y0, x0 + (h01(k, 1, sd) - 0.5f) * 2.0f, y0 + 3.0f + 6.0f * h01(k, 2, sd), 0.0f, 0.6f, sd + 7);
        }
        silk_husk(20, 25, 20, 19, 0, -1);
        prop_rib(29, 30, 4.0f, 2.0f, PI_F * 1.05f, PI_F * 1.9f, 0.1f);
        web_dust(12, 20, sd + 8); web_dust(52, 14, sd + 9); web_dust(33, 27, sd + 10);
    } else {
        int bundle[4] = { 12, 27, 41, 55 };
        for (int s = 0; s < 12; s++) {                              /* the threads, drawn in to their clumps */
            float x0 = 2.0f + 60.0f * ((float)s + h01(s, 1, sd)) / 12.0f, l = 26.0f + 26.0f * h01(s, 2, sd);
            float bx = (float)bundle[clampi((int)(x0 / 16.0f), 0, 3)] + (h01(s, 3, sd) - 0.5f) * 3.0f, mx = (x0 + bx) * 0.5f + (h01(s, 4, sd) - 0.5f) * 6.0f;
            web_line(x0, 0.0f, mx, l * 0.5f, 0.0f, 0.42f + 0.22f * h01(s, 5, sd), sd + 11 + (uint32_t)s);
            web_line(mx, l * 0.5f, bx, l, 0.0f, 0.42f + 0.22f * h01(s, 5, sd), sd + 12 + (uint32_t)s);
        }
        for (int k = 0; k < 16; k++) {                              /* tied across, sagging */
            float x0 = 2.0f + 54.0f * h01(k, 6, sd), y0 = 3.0f + 26.0f * h01(k, 7, sd);
            web_line(x0, y0, x0 + 5.0f + 8.0f * h01(k, 8, sd), y0 + (h01(k, 9, sd) - 0.5f) * 4.0f, 1.5f, 0.6f, sd + 31);
        }
        for (int b = 0; b < 4; b++) {                               /* gathered into clumps */
            float y = 40.0f + 14.0f * h01(b, 10, sd);
            for (int k = 0; k < 6; k++) cv_px(bundle[b] + (int)((h01(b, k, sd + 33) - 0.5f) * 3.0f), (int)y + k / 2, (uint8_t)(R_STONE + 10 + (k & 1)));
            web_dust((float)bundle[b], y + 2.0f, sd + 35 + (uint32_t)b);
        }
        web_line(33, 0, 33, 16, 0.0f, 0.8f, sd + 37);
        cocoon(33, 27, 11.0f, 4.6f, sd + 39);
    }
    cv_store(id);
}

/* icicles where something cold has gone by: rime feathered along the stone, icicles of every length ringed as they grew,
 * clear at their edges, a drop at each tip; in the longest, a moth caught in the ice */
static void spr_iceteeth(void) {
    cv_begin(cv_sprite, TS, TS);
    for (int i = 0; i < 13; i++) {
        float x = 4.0f + 56.0f * ((float)i + 0.5f * h01(i, 1, 651)) / 13.0f, len = 6.0f + 30.0f * h01(i, 2, 651) * h01(i, 3, 651) + 4.0f, w = 1.3f + 1.7f * h01(i, 4, 651);
        if (i == 6) { len = 41.0f; w = 3.4f; }
        for (int y = 0; y < (int)len; y++) {
            float u = (float)y / len, hw = w * (1.0f - u) * (1.0f + 0.14f * sinf((float)y * 1.3f + (float)i));
            for (int dx = (int)(-hw - 1); dx <= (int)(hw + 1); dx++) {
                float nx = ((float)dx + 0.5f) / (hw + 0.01f);
                if (fabsf(nx) > 1.0f) continue;
                float v = 0.26f + 0.3f * (1.0f - fabsf(nx)) * 0.5f - 0.12f * u;
                if (nx < -0.55f) v += 0.38f;                         /* its lit edge */
                else if (nx > 0.6f) v -= 0.1f;
                if (fabsf(nx + 0.15f) < 0.2f && hw > 1.4f) v += 0.12f;   /* the streak down its core */
                if (((int)(y + (int)(nx * 2.0f)) % 4) == 0) v += 0.07f;   /* rings */
                cv_px((int)x + dx, y, sh(R_GHOST, v));
            }
        }
        if (h01(i, 5, 651) < 0.6f) cv_px((int)x, (int)len + 1, sh(R_GHOST, 0.85f));
        if (h01(i, 5, 651) < 0.2f) cv_px((int)x, (int)len + 4, sh(R_GHOST, 0.7f));
    }
    {                                                               /* the moth */
        float x = 4.0f + 56.0f * (6.0f + 0.5f * h01(6, 1, 651)) / 13.0f, y = 15.0f;
        cv_px((int)x, (int)y, sh(R_WARM, 0.12f)); cv_px((int)x, (int)y + 1, sh(R_WARM, 0.1f)); cv_px((int)x, (int)y + 2, sh(R_WARM, 0.12f));
        cv_px((int)x - 1, (int)y, sh(R_WARM, 0.3f)); cv_px((int)x + 1, (int)y, sh(R_WARM, 0.24f));
        cv_px((int)x - 1, (int)y + 1, sh(R_WARM, 0.22f)); cv_px((int)x + 1, (int)y + 1, sh(R_WARM, 0.18f));
        cv_px((int)x - 2, (int)y - 1, sh(R_WARM, 0.26f)); cv_px((int)x + 2, (int)y - 1, sh(R_WARM, 0.2f)); cv_px((int)x, (int)y - 1, sh(R_WARM, 0.08f));
    }
    for (int x = 0; x < TS; x++) {                                  /* the rime along the stone, feathered */
        int d = 1 + (int)(2.5f * fbm((float)x / 5.0f, 0.3f, 16, 653));
        for (int y = 0; y <= d; y++) cv_px(x, y, sh(R_GHOST, 0.5f + 0.3f * h01(x, y, 655) - 0.1f * (float)y));
        if (h01(x, 1, 657) > 0.55f) cv_px(x + ((x & 1) ? 1 : -1), d + 1, sh(R_GHOST, 0.72f));
        if (h01(x, 2, 657) > 0.8f) cv_px(x, d + 2, sh(R_GHOST, 0.6f));
    }
    cv_outline(0);
    cv_store(SP_ICICLES);
}

/* P5: stone and floor. The pillar of a colonnade: a clustered shaft, three colonnettes round its core with deep fillets
 * between, ringed at its middle, on a base of two rolls and a hollow over a square plinth, under a capital of stiff
 * leaves and its abacus; damp run down it from the capital, lichen at its foot. look 0: a prisoner's tally scratched in its
 * front shaft, the last count unfinished; look 1: a piece broken out of its right side to the rubble core, and an empty
 * iron bracket that held a torch */
static float roll(float ox, float r, float ny) {                    /* a moulding's roll, r either side, ny its row's tilt */
    float nx = ox / r;
    return fabsf(nx) > 1.0f ? -1.0f : lit(nx, ny, sqrtf(fmaxf(0.05f, 1.0f - nx * nx - ny * ny * 0.5f)));
}
static void spr_gothpillar(int id, int look) {
    cv_begin(cv_sprite, TS, TS);
    const float cx = 32.0f;
    uint32_t sd = 541u + (uint32_t)look * 13u;
    for (int y = 0; y < TS; y++)
        for (int x = 0; x < TS; x++) {
            float ox = (float)x + 0.5f - cx, v = -1.0f, l;
            if (y <= 2) {                                           /* the abacus, seen from below */
                if (fabsf(ox) <= 15.0f) v = y == 2 ? 0.1f : (ox < -12.0f ? 0.42f : ox > 12.0f ? 0.2f : 0.32f);
            } else if (y <= 10) {                                   /* the capital's bell, its stiff leaves */
                float t = (float)(y - 3) / 7.0f, r = 12.4f + 4.0f * powf(1.0f - t, 1.4f);
                if (fabsf(ox) <= r) {
                    float ph = (ox + 21.0f + (float)look * 2.0f) / 6.6f, fr = ph - floorf(ph) - 0.5f, bell = roll(ox, r, 0.2f);
                    if (y <= 5) {                                   /* the tips, curled out: a ball each, lit on its upper left */
                        float bx = fr / 0.36f, by = ((float)y - 4.0f) / 1.6f, d = bx * bx + by * by;
                        v = d <= 1.0f ? 0.16f + 0.7f * lit(bx, by, sqrtf(1.0f - d)) * (0.55f + 0.45f * bell) : 0.04f;
                    } else {                                        /* the blades, narrowing down the bell to the necking */
                        float bw = 0.34f * (1.0f - (float)(y - 6) / 6.0f);
                        v = fabsf(fr) < bw ? 0.14f + 0.5f * bell + (fabsf(fr) < 0.07f ? 0.08f : 0.0f) - (fr > bw - 0.08f ? 0.1f : 0.0f) : 0.06f + 0.14f * bell;
                    }
                }
            } else if (y <= 12) {                                   /* the necking */
                if ((l = roll(ox, 12.4f, y == 11 ? -0.6f : 0.6f)) >= 0.0f) v = 0.1f + 0.55f * l;
            } else if (y < 53) {
                if (y >= 31 && y <= 33) {                           /* the ring round its middle */
                    if ((l = roll(ox, 12.8f, (float)(y - 32) * 0.7f)) >= 0.0f) v = 0.1f + 0.62f * l;
                } else {
                    float df = ox / 4.8f;
                    if (fabsf(df) <= 1.0f) v = 0.18f + 0.66f * lit(df, 0.0f, sqrtf(1.0f - df * df));
                    else if (fabsf(ox) <= 5.5f) v = 0.04f;           /* the fillets, deep */
                    else {
                        float s = ox < 0.0f ? -1.0f : 1.0f, ds = (ox - s * 7.8f) / 4.2f;
                        if (fabsf(ds) <= 1.0f) v = (s < 0.0f ? 0.16f : 0.1f) + (s < 0.0f ? 0.6f : 0.42f) * lit(ds, 0.0f, sqrtf(1.0f - ds * ds));
                    }
                }
            } else if (y <= 54) {                                   /* the base: a roll, */
                if ((l = roll(ox, 12.8f, y == 53 ? -0.6f : 0.6f)) >= 0.0f) v = 0.1f + 0.55f * l;
            } else if (y == 55) {                                   /* a hollow, */
                if (fabsf(ox) <= 11.8f) v = 0.05f;
            } else if (y <= 58) {                                   /* a greater roll, */
                if ((l = roll(ox, 13.8f, (float)(y - 57) * 0.7f)) >= 0.0f) v = 0.1f + 0.6f * l;
            } else if (fabsf(ox) <= 15.0f) {                        /* the plinth: its top, then its face */
                v = y <= 60 ? 0.46f - 0.08f * (ox > 11.0f) : (ox < -13.0f ? 0.36f : ox > 12.0f ? 0.16f : 0.26f);
                if (look == 1 && ox > 11.0f && y <= 61 && (float)y < 59.0f + (ox - 11.0f) * 0.8f) v = -1.0f;   /* its corner knocked off */
            }
            if (v < 0.0f) continue;
            v += (fbm((float)x / 5.0f, (float)y / 5.0f, 16, sd) - 0.5f) * 0.14f;   /* the stone's grain */
            if (y > 6 && y < 53 && h01(x, 1, sd + 1) > 0.84f && h01(x, y / 7, sd + 2) > 0.35f) v -= 0.06f * (1.0f - (float)(y - 6) / 50.0f);   /* damp, run down */
            cv_px(x, y, sh(R_STONE, v));
        }
    if (look == 0) {
        for (int y = 14; y < 29; y++) { int x = 33 + (int)(sinf((float)y * 0.7f) * 1.2f); cv_px(x, y, sh(R_STONE, 0.04f)); if (y & 1) cv_px(x - 1, y, sh(R_STONE, 0.7f)); }   /* a crack */
        for (int g = 0; g < 3; g++) {                               /* the tally: two fives and three */
            int y0 = 36 + g * 6, n = g < 2 ? 4 : 3;
            for (int k = 0; k < n; k++) for (int y = y0; y < y0 + 4; y++) { cv_px(29 + k * 2, y, sh(R_STONE, 0.84f)); cv_px(30 + k * 2, y, sh(R_STONE, 0.08f)); }
            if (g < 2) for (int k = 0; k < 8; k++) cv_px(28 + k, y0 + 3 - k / 2, sh(R_STONE, 0.92f));
        }
    } else {
        for (int y = 37; y < 49; y++) {                             /* the broken piece, to its rubble core */
            float bite = 3.5f - fabsf((float)y - 42.5f) * 0.55f + (h01(y, 1, sd) - 0.5f) * 1.6f;
            for (int x = 32 + 12 - (int)bite; x <= 32 + 12; x++) cv_px(x, y, C_CLEAR);
            for (int x = 32 + 12 - (int)bite - 3; x < 32 + 12 - (int)bite; x++)
                cv_px(x, y, (x + y * 3) % 5 == 0 ? sh(R_STONE, 0.04f) : sh(R_STONE, 0.12f + 0.16f * h01(x / 2, y / 2, sd + 3)));
        }
        cv_capsule(25, 22, 21, 22, 0.9f, R_IRON, 0.2f, 0.8f);       /* the bracket, its torch long gone */
        cv_px(26, 21, sh(R_IRON, 0.9f)); cv_px(26, 23, sh(R_IRON, 0.4f));
        iron_ring(19.5f, 23.0f, 2.2f, 1.0f, 1.0f, 0.0f, TAU_F, 0.0f);
        cv_line(21, 23, 21, 27, sh(R_IRON, 0.5f)); cv_px(21, 27, sh(R_BRICK, 0.2f));   /* rust run from it */
    }
    weather(55, 63, 0.1f + 0.5f * g_decay, sd + 5);
    cv_outline(0);
    cv_store(id);
}

/* flowstone (runs II and III): a stalagmite built drip by drip, hung with calcite in folds like a curtain, rimstone dams
 * stepping down round its foot each holding a skin of water, the drip falling to it from a straw on the vault; at its foot a
 * skull half swallowed by the stone */
static void spr_flowstone(void) {
    cv_begin(cv_sprite, TS, TS);
    const float cx = 31.0f;
    for (int y = 0; y < 9; y++) { float hw = 2.2f * (1.0f - (float)y / 9.0f); for (int x = (int)(33 - hw); x <= (int)(33 + hw); x++) cv_px(x, y, sh(R_BONE, 0.32f + 0.3f * (x < 33))); }   /* the straw */
    cv_px(33, 9, sh(R_GHOST, 0.85f));
    for (int k = 0; k < 3; k++) { cv_px(33, 13 + k * 5, sh(R_GHOST, 0.7f)); cv_px(33, 14 + k * 5, sh(R_GHOST, 0.45f)); }   /* the drip, falling */
    for (int y = 22; y < 56; y++) {                                 /* the stalagmite */
        float t = (float)(y - 22) / 34.0f, hw = 2.4f + 10.0f * powf(t, 1.15f) + 1.3f * sinf((float)y * 0.42f) * t + 1.5f * (fbm(0.5f, (float)y / 4.0f, 16, 551) - 0.5f);
        for (int x = (int)(cx - hw) - 1; x <= (int)(cx + hw) + 1; x++) {
            float nx = ((float)x + 0.5f - cx) / (hw + 0.01f);
            if (fabsf(nx) > 1.0f) continue;
            float fold = sinf(((float)x - cx) * 1.25f + (float)y * 0.06f), v = 0.14f + 0.56f * lit(nx, -0.15f, sqrtf(1.0f - nx * nx)) + 0.1f * fold;
            float skin = fbm((float)x / 2.5f, (float)y / 9.0f, 32, 553) + 0.3f * (1.0f - t);
            if (y < 26) skin += 0.4f;                               /* fresh calcite at its tip, where the drip lands */
            cv_px(x, y, skin > 0.6f ? sh(R_BONE, v * 0.9f + 0.06f) : sh(R_STONE, v - 0.04f));
            if (nx < -0.45f && nx > -0.7f && h01(x, y, 555) < 0.3f) cv_px(x, y, sh(R_GHOST, 0.62f));   /* wet, catching the light */
        }
    }
    static const float TY[3] = { 54, 58, 61.5f }, TW[3] = { 14, 19, 24 };
    for (int k = 0; k < 3; k++) {                                   /* the rimstone dams, the far first */
        float y0 = TY[k], w = TW[k];
        for (int y = (int)(y0 - 3.0f); y <= (int)y0 + 2; y++)
            for (int x = (int)(cx - w); x <= (int)(cx + w); x++) {
                float nx = ((float)x + 0.5f - cx) / w;
                if (fabsf(nx) > 1.0f) continue;
                float lip = y0 + 1.6f * sqrtf(1.0f - nx * nx) + 0.6f * sinf((float)x * 1.1f + (float)k), back = y0 - 2.6f * sqrtf(1.0f - nx * nx);
                if ((float)y < back || (float)y > lip + 1.0f) continue;
                if ((float)y > lip - 0.5f) cv_px(x, y, sh(R_BONE, (float)y > lip ? 0.24f : 0.58f - 0.2f * nx));   /* its lip, lumpy */
                else cv_px(x, y, sh(R_GHOST, 0.1f + 0.12f * (1.0f - fabsf(nx)) + (y == (int)(y0 - 1.0f) && h01(x, k, 557) > 0.55f ? 0.35f : 0.0f)));   /* the water held */
            }
    }
    for (int a = 0; a < 16; a++) { float an = TAU_F * (float)a / 16.0f; cv_px((int)(cx + 2.0f + cosf(an) * 3.0f), (int)(52.5f + sinf(an) * 0.8f), sh(R_GHOST, 0.5f)); }   /* where the drip lands */
    prop_skull(14, 47, 0, 0, 3, 0);                                 /* half swallowed */
    for (int y = 51; y < 60; y++) for (int x = 18; x < 26; x++) if ((float)(x - 18) + (float)(y - 51) * 0.6f > 4.0f + 2.0f * h01(x, y, 559) && cv_sprite[y * TS + x] != C_CLEAR) cv_px(x, y, sh(R_BONE, 0.3f + 0.25f * h01(x, y, 561)));
    cv_outline(0);
    cv_store(SP_STALAG);
}

/* the Black Altar: a block of black stone under a mensa slab, its front a blind arcade of three trefoiled arches on
 * colonnettes: in the middle its rune, faintly violet; in the others two of the Order's marks, the skull and the eye, cut and
 * edged pale; a moulded plinth; on the slab the iron bowl its fire burns in, on three clawed feet (its rim where the fire's
 * sprites stand); old blood run from the bowl over the slab's edge and dried black */
static void spr_altarstone(void) {
    cv_begin(cv_sprite, TS, TS);
    for (int y = 42; y < 59; y++)                                   /* the block */
        for (int x = 17; x < 48; x++) {
            float u = ((float)x + 0.5f - 32.5f) / 15.5f, v = 0.1f + 0.2f * lit(u, -0.1f, sqrtf(fmaxf(0.0f, 1.0f - u * u))) + 0.05f * fbm((float)x / 3.0f, (float)y / 3.0f, 8, 571);
            int col = x == 17 || x == 18 || x == 27 || x == 28 || x == 37 || x == 38 || x == 46 || x == 47;   /* the colonnettes */
            if (col) v = ((x & 1) ? 0.36f : 0.2f) - 0.1f * (x > 40);
            else if (y >= 45 && y <= 56) {                          /* the arches' panels, sunk, trefoiled at their heads */
                int p = x < 27 ? 0 : x < 37 ? 1 : 2, px0 = p == 0 ? 19 : p == 1 ? 29 : 39, w = p == 2 ? 7 : 8, lx = x - px0;
                int head = y == 45 ? (lx == 3 || lx == 4) : y == 46 ? (lx >= 1 && lx <= w - 2 && lx != 2 && lx != w - 3) : 1;
                if (head) v = 0.05f;
                if (head && (y == 45 || lx == 0)) v = 0.02f;               /* its top and left in shadow, its right and foot lit */
                if (head && y > 46 && (lx == w - 1 || y == 56)) v = 0.2f;
            }
            if (y == 42 || y == 43) v += y == 42 ? 0.16f : 0.04f;   /* a moulding under the slab */
            cv_px(x, y, sh(R_IRON, v));
        }
    for (int y = 0; y < 7; y++)                                     /* the rune, faintly violet */
        for (int x = 0; x < 5; x++)
            if (RUNE_GLYPH[2][y][x] == '#') { cv_px(30 + x, 48 + y, sh(R_PURPLE, 0.52f)); if (h01(x, y, 573) < 0.5f) cv_px(31 + x, 48 + y, sh(R_PURPLE, 0.22f)); }
    for (int m = 0; m < 2; m++)                                     /* the Order's marks: the skull and the eye */
        for (int y = 0; y < 7; y++)
            for (int x = 0; x < 7; x++)
                if (PZ_MARKS[m ? 10 : 15][y][x] == '#') { int X = (m ? 39 : 19) + x, Y = 48 + y; cv_px(X, Y, sh(R_IRON, 0.02f)); if (cv_sprite[(Y + 1) * TS + X] != C_CLEAR && !(y < 6 && PZ_MARKS[m ? 10 : 15][y + 1][x] == '#')) cv_px(X, Y + 1, sh(R_IRON, 0.4f)); }
    for (int y = 59; y < 64; y++)                                   /* the plinth */
        for (int x = 15; x < 50; x++) cv_px(x, y, sh(R_IRON, y == 59 ? 0.4f : (x < 17 ? 0.3f : x > 46 ? 0.1f : 0.18f) + 0.04f * h01(x, y, 575)));
    for (int y = 37; y < 42; y++)                                   /* the mensa: its top, seen from above, then its edge */
        for (int x = 13; x < 52; x++) {
            if (x == 13 && y == 37) continue;
            float v = y <= 38 ? 0.36f + 0.04f * h01(x, y, 577) : y == 39 ? 0.5f : y == 40 ? 0.22f : 0.08f;
            if (x < 15) v += 0.08f; else if (x > 49) v -= 0.1f;
            cv_px(x, y, sh(R_IRON, v));
        }
    for (int k = 0; k < 5; k++) cv_px(14 + k, 39 + (k > 2), sh(R_IRON, 0.05f));   /* a crack across its corner */
    for (int a = 0; a < 3; a++) {                                   /* the bowl's feet, clawed */
        float fx = 26.0f + 6.0f * (float)a;
        cv_capsule(fx + (float)(a - 1) * 0.8f, 35.0f, fx + (float)(a - 1) * 1.6f, 37.5f, 0.8f, R_IRON, 0.15f, 0.7f);
        cv_px((int)(fx + (float)(a - 1) * 1.6f) - 1, 38, sh(R_IRON, 0.6f)); cv_px((int)(fx + (float)(a - 1) * 1.6f) + 1, 38, sh(R_IRON, 0.45f));
    }
    cv_ellipse(32.5f, 32.5f, 8.6f, 3.4f, R_IRON, 0.1f, 0.7f);       /* the bowl */
    for (int x = 24; x <= 41; x++) cv_px(x, 31, sh(R_IRON, x < 33 ? 0.85f : 0.55f));   /* its rim */
    for (int x = 25; x <= 40; x++) cv_px(x, 30, sh(R_IRON, 0.06f));  /* the black inside, under the fire */
    for (int y = 33; y < 52; y++) {                                 /* old blood, run over the edge and down, dried */
        int x = y < 37 ? 39 : (y < 41 ? 41 : 42) + (y > 46);
        if (y > 38 && y < 41) { cv_px(x, y, sh(R_BLOOD, 0.14f)); cv_px(x - 1, y, sh(R_BLOOD, 0.1f)); continue; }
        if (y >= 41 && h01(y, 1, 579) < 0.15f) continue;
        cv_px(x, y, sh(R_BLOOD, 0.1f + 0.06f * (y < 41)));
    }
    cv_px(36, 37, sh(R_BLOOD, 0.12f)); cv_px(37, 37, sh(R_BLOOD, 0.1f)); cv_px(38, 38, sh(R_BLOOD, 0.12f));
    cv_outline(0);
    cv_store(SP_BALTAR);
}

/* an urn's body: radius r(y) given row by row from y0, lit as a turned pot, its ramp's light and shade */
static void urn_body(float cx, int y0, int y1, const float *r, int ramp, float lo, float hi, uint32_t seed) {
    for (int y = y0; y <= y1; y++) {
        float rr = r[y - y0], dr = (y < y1 ? r[y - y0 + 1] : rr) - (y > y0 ? r[y - y0 - 1] : rr);
        for (int x = (int)(cx - rr) - 1; x <= (int)(cx + rr) + 1; x++) {
            float nx = ((float)x + 0.5f - cx) / (rr + 0.01f);
            if (fabsf(nx) > 1.0f) continue;
            float nz = sqrtf(1.0f - nx * nx), ny = -dr * 0.5f, nl = sqrtf(nx * nx + ny * ny + nz * nz);
            float v = lo + (hi - lo) * lit(nx / nl, ny / nl, nz / nl) + 0.05f * (fbm((float)x / 3.0f, (float)y / 3.0f, 16, seed) - 0.5f);
            cv_px(x, y, sh(ramp, v));
        }
    }
}
/* the urn by a room's wall, in three looks. 0: a cinerary urn, its lid on, a band of skulls in relief about its shoulder,
 * small handles, a crack down from a chip in its lip; 1: a tall jar, its lid knocked off beside it and the ashes spilt from
 * its mouth with a scrap of bone in them, a band of crosses cut round it; 2: a stone ossuary jar broken, its top gone in a
 * jagged edge and the bones it held out over the floor */
static void spr_cinerary(int id, int look) {
    cv_begin(cv_sprite, TS, TS);
    float r[40];
    cv_flat(32, 62, 14, 2.0f, R_WARM + 1);
    if (look == 0) {
        for (int y = 0; y < 34; y++) {                              /* from its lip at 30 to its foot at 63 */
            float s = (float)(y - 4) / 23.0f;
            r[y] = y < 2 ? 7.2f : y < 4 ? 5.6f : y < 28 ? 5.6f + 6.0f * powf(fmaxf(0.0f, sinf(PI_F * s)), 0.75f) : y < 30 ? 4.8f : 6.6f;
        }
        urn_body(32, 30, 63, r, R_BRICK, 0.1f, 0.82f, 581);
        cv_ellipse(32, 28.5f, 7.0f, 2.6f, R_BRICK, 0.2f, 0.85f);    /* the lid */
        cv_ellipse(32, 25.0f, 1.8f, 1.8f, R_BRICK, 0.2f, 0.9f);     /* its knob */
        for (int x = 25; x <= 39; x++) cv_px(x, 30, sh(R_BRICK, 0.12f));
        for (int k = 0; k < 4; k++) {                               /* skulls in relief about the shoulder */
            int sx = 25 + k * 4 + (k > 1);
            cv_px(sx, 41, sh(R_BRICK, 0.9f - 0.1f * (float)k)); cv_px(sx + 1, 41, sh(R_BRICK, 0.8f - 0.1f * (float)k));
            cv_px(sx, 42, sh(R_BRICK, 0.06f)); cv_px(sx + 1, 42, sh(R_BRICK, 0.06f)); cv_px(sx, 43, sh(R_BRICK, 0.6f - 0.1f * (float)k));
        }
        for (int x = 22; x <= 42; x++) { cv_px(x, 39, sh(R_BRICK, 0.7f - 0.02f * (float)(x - 22))); cv_px(x, 45, sh(R_BRICK, 0.18f)); }
        for (int s = -1; s <= 1; s += 2)                           /* its handles */
            for (int a = 0; a < 14; a++) {
                float an = TAU_F * (float)a / 14.0f, c = cosf(an);
                if (c * (float)s < -0.2f) continue;
                cv_px((int)(32.0f + (float)s * 11.0f + c * 2.0f), (int)(41.5f + sinf(an) * 2.4f), sh(R_BRICK, 0.3f + 0.4f * (c < 0.0f) - 0.1f * (s > 0)));
            }
        for (int y = 31; y < 52; y++) { int x = 37 + (y - 31) / 6; cv_px(x, y, sh(R_BRICK, 0.04f)); cv_px(x - 1, y, sh(R_BRICK, 0.7f)); }   /* the crack */
        cv_px(37, 30, C_CLEAR); cv_px(38, 30, C_CLEAR); cv_px(38, 31, C_CLEAR);
    } else if (look == 1) {
        for (int y = 0; y < 36; y++) {                              /* from its mouth at 28 */
            float t = (float)y / 35.0f;
            r[y] = y < 2 ? 5.8f : y < 6 ? 4.6f : 4.6f + 7.2f * sinf(PI_F * fminf(1.0f, (t - 0.17f) / 0.86f)) * (t < 0.93f ? 1.0f : 0.9f);
            if (y > 33) r[y] = 6.0f;
        }
        urn_body(30, 28, 63, r, R_BRICK, 0.1f, 0.78f, 583);
        for (int x = 25; x <= 35; x++) cv_px(x, 28, sh(R_BRICK, x < 30 ? 0.7f : 0.45f));
        for (int x = 26; x <= 34; x++) cv_px(x, 29, sh(R_WARM, 0.04f));   /* its mouth, open */
        for (int x = 20; x <= 40; x++) if (((x + 1) % 4) != 0) { cv_px(x, 45, sh(R_BRICK, 0.08f)); if (x % 4 == 1) { cv_px(x, 44, sh(R_BRICK, 0.08f)); cv_px(x, 46, sh(R_BRICK, 0.08f)); } }   /* the crosses */
        cv_ellipse(49, 59.0f, 2.6f, 4.6f, R_BRICK, 0.15f, 0.75f);   /* the lid, on its edge */
        cv_ellipse(50.5f, 59.0f, 2.2f, 4.2f, R_BRICK, 0.1f, 0.5f);
        cv_ellipse(51.5f, 59.0f, 0.9f, 1.2f, R_BRICK, 0.2f, 0.7f);  /* its knob */
        for (int y = 56; y < 64; y++)                               /* the ashes, spilt */
            for (int x = 6; x < 26; x++) {
                float h = 62.5f - 4.5f * expf(-((float)x - 19.0f) * ((float)x - 19.0f) / 30.0f) - 1.0f * h01(x / 2, 1, 585);
                if ((float)y >= h) cv_px(x, y, sh(R_WARM, 0.36f + 0.2f * h01(x, y, 587) - ((float)y > 62.0f ? 0.12f : 0.0f)));
            }
        cv_line(13, 61, 16, 60, sh(R_BONE, 0.85f)); cv_px(13, 62, sh(R_BONE, 0.5f));   /* a scrap of bone in them */
        for (int k = 0; k < 6; k++) cv_px(24 + k, 55 + (k & 1) * 2 - k / 3, sh(R_WARM, 0.42f));   /* run down from the mouth's lip */
    } else {
        for (int y = 0; y < 28; y++) { float t = (float)y / 27.0f; r[y] = 7.0f + 5.2f * powf(fmaxf(0.0f, sinf(PI_F * (0.12f + 0.8f * t))), 0.7f) - (y > 24 ? 1.5f : 0.0f); }
        urn_body(32, 36, 63, r, R_STONE, 0.12f, 0.75f, 589);
        for (int x = 19; x <= 45; x++) {                            /* broken off, jagged: its wall's edge pale, the dark in it */
            int top = 39 + (int)(4.0f * h01(x / 2, 1, 591)) + (x > 36 ? 4 : 0) - (x < 25 ? 2 : 0);
            for (int y = 34; y < top; y++) cv_px(x, y, C_CLEAR);
            cv_px(x, top, sh(R_STONE, 0.7f)); cv_px(x, top + 1, sh(R_STONE, 0.03f)); if (x > 22 && x < 42) cv_px(x, top + 2, sh(R_STONE, 0.06f));
        }
        for (int k = -2; k <= 2; k++) { cv_px(33 + k, 52, sh(R_STONE, 0.05f)); cv_px(33, 52 + k, sh(R_STONE, 0.05f)); cv_px(34 + k, 53, sh(R_STONE, 0.55f)); }   /* a cross cut in it */
        prop_long(28, 46, 25, 30, 1.6f, 1, 0, 0.0f, 593);           /* a femur out of it */
        prop_skull(33, 35, 1, 0, 1, 0);
        cv_ellipse(50, 61, 5.0f, 2.0f, R_STONE, 0.1f, 0.55f);       /* a shard of it */
        prop_jaw(14, 62); prop_fingers(43, 61, 595);
    }
    cv_outline(0);
    cv_store(id);
}

/* broken arms (runs I and II): a heater shield fallen on its back and split down its planks, the Order's pale cross on its
 * crimson, the paint flaking to the wood; a sword snapped, its edge notched, the bones of the hand that held it still
 * curled about the grip; the point lying apart */
static void spr_brokenarms(void) {
    cv_begin(cv_sprite, TS, TS);
    cv_flat(32, 62, 27, 2.0f, R_WARM + 1);
    for (int y = 47; y < 63; y++)                                   /* the shield, foreshortened: its top edge far, its point near */
        for (int x = 6; x < 38; x++) {
            float u = ((float)y - 47.0f) / 15.0f, hw = 14.0f * (u < 0.45f ? 1.0f : sqrtf(fmaxf(0.0f, 1.0f - (u - 0.45f) * (u - 0.45f) / 0.3025f))), cxs = 21.0f + u * 2.0f;
            float ox = (float)x + 0.5f - cxs, split = 24.5f - u * 4.0f;
            if (fabsf(ox) > hw) continue;
            int right = (float)x + 0.5f > split;
            if (fabsf((float)x + 0.5f - split) < 0.9f) continue;   /* split down its planks */
            int X = x + right * 2, Y = y + right;
            float rim = hw - fabsf(ox);
            uint8_t c;
            if (rim < 1.2f || y == 47) c = sh(R_IRON, 0.3f + 0.4f * (ox < 0.0f) + 0.1f * (y == 47));
            else if (fabsf(ox + 1.0f) < 1.3f || fabsf((float)y - 52.0f) < 1.0f) c = sh(R_BONE, 0.62f - 0.15f * right);   /* the cross */
            else c = sh(R_BLOOD, 0.18f + 0.12f * (ox < 0.0f) - 0.06f * right);
            if (fbm((float)x / 2.0f, (float)y / 1.5f, 32, 601) > 0.66f && rim >= 1.2f) c = sh(R_WOOD, 0.2f + 0.14f * h01(x, y, 603) - (((x - 6) % 5) == 0 ? 0.08f : 0.0f));   /* flaked to the wood */
            cv_px(X, Y, c);
        }
    cv_ellipse(22, 54, 2.2f, 1.2f, R_IRON, 0.2f, 0.85f);            /* its boss */
    for (int i = 0; i <= 22; i++) {                                 /* the blade, snapped, notched */
        float t = (float)i / 22.0f, x = 34.0f + 19.0f * t, y = 60.5f - 5.5f * t;
        if (!(i == 7 || i == 13 || i == 17)) cv_px((int)x, (int)y - 1, sh(R_IRON, 0.92f));
        cv_px((int)x, (int)y, sh(R_IRON, 0.32f)); cv_px((int)x, (int)y + 1, sh(R_IRON, 0.55f));
    }
    for (int k = 0; k < 3; k++) cv_px(53 + k, 55 - (k == 1), sh(R_IRON, 0.75f - 0.2f * (float)k));   /* torn */
    cv_capsule(32, 57.5f, 35, 63, 0.9f, R_IRON, 0.3f, 0.8f);       /* the guard */
    cv_capsule(27, 63, 32, 61, 1.2f, R_WOOD, 0.08f, 0.38f);         /* the grip, bound */
    for (int k = 0; k < 3; k++) cv_px(28 + k * 2, 62 - k / 2, sh(R_WOOD, 0.04f));
    cv_ellipse(26, 63, 1.6f, 1.2f, R_IRON, 0.3f, 0.9f);             /* the pommel */
    for (int f = 0; f < 4; f++) {                                   /* the hand still about it */
        int x = 28 + f;
        cv_px(x, 59 + (f & 1), sh(R_BONE, 0.85f - 0.06f * (float)f)); cv_px(x, 60 + (f & 1), sh(R_BONE, 0.55f));
    }
    cv_line(27, 59, 21, 61, sh(R_BONE, 0.7f)); cv_line(26, 60, 20, 62, sh(R_BONE, 0.45f));   /* its wrist's bones */
    for (int i = 0; i <= 7; i++) { cv_px(51 + i, 59 - i / 3, sh(R_IRON, 0.9f)); cv_px(51 + i, 60 - i / 3, sh(R_IRON, 0.35f)); }   /* the point, apart */
    cv_outline(0);
    cv_store(SP_BROKEN_ARMS);
}

/* the collar (runs II and III): a ring bolt in the floor, a chain lying slack from it, and a spiked collar open on its
 * hinge, still about the neck-bones of what wore it; bones gnawed for their marrow */
static void spr_spikecollar(void) {
    cv_begin(cv_sprite, TS, TS);
    cv_flat(32, 62, 25, 2.0f, R_WARM + 1);
    prop_long(10, 56, 22, 52, 1.3f, 0, 1, 0.08f, 611);              /* gnawed, split */
    prop_long(44, 52, 56, 55, 1.2f, 0, 0, 0.05f, 613);
    for (int k = 0; k < 4; k++) cv_px(47 + k * 2, 53 + (k & 1), sh(R_BONE, 0.12f));   /* tooth marks */
    cv_px(18, 52, sh(R_BONE, 0.1f)); cv_px(15, 54, sh(R_BONE, 0.1f));
    cv_ellipse(8.5f, 61.0f, 3.0f, 1.3f, R_IRON, 0.1f, 0.6f);        /* the bolt's plate */
    iron_ring(8.5f, 59.6f, 2.4f, 1.1f, 1.0f, 0.0f, TAU_F, 0.0f);
    forged_chain(11, 60, 21, 62, 1.0f, 0, 0.04f);
    forged_chain(21, 62, 31, 58, -1.5f, 1, 0.0f);
    float cx = 40, cy = 55;
    for (int a = 0; a < 11; a++) {                                  /* its spikes, the far ones standing up behind it */
        float an = PI_F * (1.05f + 0.9f * (float)a / 10.0f), ex = cosf(an), ey = sinf(an), x0 = cx + ex * 9.6f, y0 = cy + ey * 4.0f;
        for (int k = 0; k < 4; k++) cv_px((int)(x0 + ex * (float)k * 0.5f), (int)(y0 - (float)k), sh(R_IRON, 0.35f + 0.15f * (float)k - 0.2f * (ex > 0.3f)));
        cv_px((int)(x0 + ex * 2.0f), (int)(y0 - 4.0f), sh(R_IRON, 0.95f - 0.25f * (ex > 0.3f)));
    }
    for (int k = 0; k < 3; k++) prop_vertebra(cx - 2.6f + (float)k * 2.6f, cy + 0.8f - (float)k * 0.4f, 0.12f);   /* the neck it was about */
    iron_ring(cx, cy, 9.0f, 3.8f, 3.0f, PI_F * 0.66f, PI_F * 2.3f, 0.0f);   /* the collar, a heavy band, open at the front */
    for (int a = 0; a < 6; a++) { float an = PI_F * (0.75f + 0.28f * (float)a); cv_px((int)(cx + cosf(an) * 9.0f), (int)(cy + sinf(an) * 3.8f), sh(R_IRON, 0.95f)); }   /* rivets */
    for (int s = -1; s <= 1; s += 2) {                              /* its open ends, a spike on each pointing at you */
        float an = PI_F * (s < 0 ? 0.66f : 0.3f), x0 = cx + cosf(an) * 9.0f, y0 = cy + sinf(an) * 3.8f;
        cv_px((int)x0, (int)y0 + 1, sh(R_IRON, 0.9f)); cv_px((int)x0 + s, (int)y0 + 2, sh(R_IRON, 0.6f));
    }
    cv_ellipse(cx - 7.0f, cy - 2.4f, 1.5f, 1.5f, R_IRON, 0.2f, 0.85f);   /* the hinge */
    cv_px(54, 61, sh(R_BONE, 0.9f)); cv_px(55, 60, sh(R_BONE, 0.7f));   /* a tooth */
    cv_outline(0);
    cv_store(SP_COLLAR);
}

/* standing water (run II, and where the Drowned lie): its shore unsure, the floor dark and wet round it, the far side
 * catching a light, dust and scum floating; still, the bones of a hand pale just under it; stirring, rings going out from
 * something under it, bubbles, the scum broken */
/* the roost (1.0.0b18): a bat asleep, hanging by its feet from the ceiling, wrapped in its wings like a cloak, its head at
 * the bottom with the ears hanging; drawn as it hangs. Stirring, one wing loosens and its eyes open */
static void spr_bat_roost(int id, int stir) {
    cv_begin(cv_sprite, TS, TS);
    for (int s = -1; s <= 1; s += 2) {                              /* the feet, gripping */
        cv_line(32 + s * 1.5f, 3, 32 + s * 1.5f, 8, sh(R_IRON, 0.3f));
        cv_px(32 + s * 1 , 2, sh(R_IRON, 0.6f)); cv_px(32 + s * 2, 2, sh(R_IRON, 0.5f)); cv_px(32 + s * 3, 3, sh(R_IRON, 0.4f));
    }
    bat_body(32, 15);
    for (int s = -1; s <= 1; s += 2) {                              /* the wings folded about it */
        int open = stir && s > 0;
        float wx = 32 + s * 5.0f, wy = 7.0f;                        /* the wrist up by the feet, its thumb hooked */
        float p[12] = { 32 + s * 1.5f, 6, wx, wy, 32 + s * (open ? 14.0f : 8.5f), open ? 14.0f : 17.0f,
                        32 + s * (open ? 17.0f : 8.0f), open ? 25.0f : 27.0f, 32 + s * (open ? 9.0f : 4.5f), 34.0f, 32 + s * 1.0f, 30.0f };
        cv_poly(p, 6, R_PURPLE, 0.05f, 0.3f, 6, 28);
        for (int y = 6; y < 36; y++)                                /* the membrane's folds, dithered, the near edge lit */
            for (int x = s < 0 ? 18 : 32; x < (s < 0 ? 32 : 50); x++) {
                uint8_t c = cv_sprite[y * TS + x];
                if (c < R_PURPLE || c >= R_PURPLE + 16) continue;
                float fold = 0.5f + 0.5f * sinf((x - 32) * 1.3f * s + y * 0.15f);
                float v = 0.06f + 0.2f * fold * (s < 0 ? 1.0f : 0.7f) + (((x + y) & 1) ? 0.02f : -0.02f);
                cv_px(x, y, sh(R_PURPLE, v));
            }
        for (int f = 0; f < 3; f++) {                               /* the fingers down the folds */
            float tx = 32 + s * (open ? 10.0f + f * 3.5f : 5.0f + f * 1.4f), ty = open ? 18.0f + f * 5.0f : 22.0f + f * 4.0f;
            cv_line(wx, wy + 1, tx, ty, sh(R_IRON, 0.35f + 0.05f * f));
        }
        cv_px((int)wx, (int)wy, sh(R_IRON, 0.75f)); cv_px((int)wx + s, (int)wy - 2, sh(R_IRON, 0.8f));
        cv_line(wx, wy, wx + s * 0.5f, wy - 2.0f, sh(R_IRON, 0.55f));
    }
    for (int y = 0; y < 13; y++)                                    /* its face, upside down, peering out at the foot of the cloak */
        for (int x = 0; x < 13; x++) {
            char ch = BAT_FACE[12 - y][x];
            int px = 26 + x, py = 27 + y;
            if (ch == '.') continue;
            if (ch == '@') { if (stir) cv_eye(px, py, (uint8_t)(R_FLAME + 8)); else cv_px(px, py, sh(R_SKIN, 0.2f)); continue; }   /* asleep: lids shut */
            if (ch == '0') cv_px(px, py, 0);
            else if (ch >= '1' && ch <= '9') cv_px(px, py, (uint8_t)(R_IRON + ch - '0'));
            else if (ch >= 'a' && ch <= 'f') cv_px(px, py, (uint8_t)(R_IRON + ch - 'a' + 10));
            else if (ch >= 'g' && ch <= 'v') cv_px(px, py, (uint8_t)(R_SKIN + ch - 'g'));
        }
    if (!stir) for (int x = 29; x <= 35; x++) if (cv_sprite[33 * TS + x] == 0) cv_px(x, 33, sh(R_IRON, 0.3f));   /* its mouth shut */
    cv_outline(0);
    cv_store(id);
}

