/* A part of crawl_art.c (docs/PLAN-tidy.md): it is #included there in its place and never compiled on its own.
 * The ambushes' signs; the pool; treasure (coins, the chalice, gems, the crown, the chest), the phial, the soul urn, the ring; props_level; footprints measured; crawl_art_level. */

/* the ambushes' signs (1.0.0b18), each by a kill room's door: run I bones raked into a drift, claw-lines in the dust; run II a
 * wet drag towards the door and a shred of shroud; run III fresh scree, pale and sharp, fallen in a fan, a split timber; run
 * IV warm cinders, their hearts still red; and in any run's dark room, eyes in the dark */
static void spr_signs(void) {
    cv_begin(cv_sprite, TS, TS);                                   /* raked bones */
    for (int y = 56; y < 64; y++) for (int x = 8; x < 58; x++) {
        float nx = (x + 0.5f - 33.0f) / 25.0f, ny = (y + 0.5f - 60.0f) / 4.0f;
        if (nx * nx + ny * ny > 1.0f) continue;
        float v = 0.18f + 0.1f * h01(x, y, 601);
        if (((x + (int)(y * 1.6f)) % 6) == 0 && x > 30) v = 0.05f;    /* the rake of claws through the dust */
        cv_px(x, y, sh(R_WARM, v));
    }
    prop_long(10, 58, 30, 55, 1.5f, 1, 0, 0.1f, 603); prop_long(12, 61, 29, 58, 1.3f, 0, 1, 0.2f, 605);
    prop_rib(19, 62, 5.0f, 6.0f, PI_F * 1.1f, PI_F * 1.8f, 0.1f);
    prop_skull(22, 49, 0, 0, 1, 0); prop_vertebra(34, 59, 0.1f); prop_fingers(40, 60, 607);
    cv_outline(0);
    cv_store(SP_SIGN_RAKE);

    cv_begin(cv_sprite, TS, TS);                                   /* the wet drag */
    for (int x = 4; x < 60; x++) {
        float t = (x - 4) / 56.0f, half = 1.0f + 3.0f * t, cy = 60.0f - 1.5f * sinf(t * 3.0f);
        for (int y = (int)(cy - half); y <= (int)(cy + half); y++) {
            float e = fabsf(y + 0.5f - cy) / half;
            if (e > 1.0f || h01(x, y, 611) < 0.08f * (1.0f - t)) continue;
            cv_px(x, y, sh(R_GHOST, 0.05f + 0.1f * (1.0f - e) + (e < 0.3f && h01(x, y, 613) < 0.15f ? 0.35f : 0.0f)));
        }
        if (h01(x, 1, 615) < 0.12f) cv_px(x, (int)(cy + half + 1), sh(R_GHOST, 0.3f));   /* drips beside it */
    }
    for (int f = 0; f < 4; f++) cv_line(50.0f + f * 1.7f, 57.0f, 53.0f + f * 2.0f, 61.0f, sh(R_GHOST, 0.03f));   /* fingers dragged through it */
    float scrap[8] = { 18, 55, 27, 54, 25, 60, 16, 61 };
    cv_poly(scrap, 4, R_WARM, 0.25f, 0.55f, 54, 7);
    cv_line(16, 61, 13, 63, sh(R_WARM, 0.3f)); cv_px(22, 57, sh(R_WARM, 0.1f)); cv_px(24, 58, sh(R_WARM, 0.1f));
    cv_store(SP_SIGN_DRAG);

    cv_begin(cv_sprite, TS, TS);                                   /* fresh scree */
    cv_flat(30, 61.5f, 24, 2.5f, sh(R_IRON, 0.05f));
    for (int k = 0; k < 26; k++) {
        float t = h01(k, 1, 621), a = (h01(k, 2, 621) - 0.5f) * 1.6f;
        float cx = 14.0f + 36.0f * t, cy = 62.0f - 7.0f * (1.0f - t) * (0.6f + 0.4f * cosf(a)) * h01(k, 3, 621), r = 0.8f + 2.2f * h01(k, 4, 621) * (1.0f - 0.5f * t);
        float pt[8] = { cx - r, cy, cx - r * 0.2f, cy - r * 1.1f, cx + r, cy - r * 0.3f, cx + r * 0.4f, cy + r * 0.6f };
        cv_poly(pt, 4, R_IRON, 0.4f, 0.92f, cy - r, r * 2.0f);    /* pale, sharp-edged, new-broken (iron's greys hold at every level) */
    }
    cv_capsule(36, 52, 56, 60, 1.6f, R_WOOD, 0.2f, 0.55f);         /* a timber split from its prop */
    for (int k = 0; k < 4; k++) cv_line(36.0f - k, 51.0f + k * 0.8f, 33.0f - k * 0.5f, 49.0f + k, sh(R_WOOD, 0.6f));
    for (int i = 0; i < 50; i++) cv_px(10 + (int)(h01(i, 5, 623) * 44.0f), 59 + (int)(h01(i, 6, 623) * 5.0f), sh(R_IRON, 0.55f + 0.3f * h01(i, 7, 623)));   /* grit */
    cv_outline(0);
    cv_store(SP_SIGN_SCREE);

    cv_begin(cv_sprite, TS, TS);                                   /* warm cinders */
    cv_flat(32, 61.5f, 22, 2.5f, sh(R_IRON, 0.03f));
    for (int k = 0; k < 18; k++) {
        float cx = 14.0f + 36.0f * h01(k, 1, 631), cy = 59.0f + 4.0f * h01(k, 2, 631), r = 1.0f + 2.5f * h01(k, 3, 631);
        cv_ellipse(cx, cy, r * 1.2f, r * 0.8f, R_IRON, 0.02f, 0.22f);
        if (h01(k, 4, 631) < 0.6f) {                                /* its heart still red */
            cv_px((int)cx, (int)cy, (uint8_t)(R_FLAME + 6 + (int)(h01(k, 5, 631) * 5.0f)));
            if (r > 2.0f) cv_px((int)cx + 1, (int)cy, (uint8_t)(R_FLAME + 4));
        }
    }
    for (int i = 0; i < 9; i++) cv_px(16 + (int)(h01(i, 6, 633) * 32.0f), 50 + (int)(h01(i, 7, 633) * 8.0f), (uint8_t)(R_FLAME + 3 + (i % 3)));   /* sparks going up */
    cv_store(SP_SIGN_CINDER);

    cv_begin(cv_sprite, TS, TS);                                   /* eyes in the dark: two pairs, at a man's height and lower, full-bright */
    static const int EY[2][3] = { { 26, 25, 7 }, { 40, 34, 6 } };
    for (int e = 0; e < 2; e++) for (int s = 0; s < 2; s++) {
        int x = EY[e][0] + s * EY[e][2], y = EY[e][1];
        cv_px(x, y, (uint8_t)(R_FLAME + 13 - e * 2)); cv_px(x + 1, y, (uint8_t)(R_FLAME + 9 - e * 2));
        cv_px(x, y + 1, (uint8_t)(R_FLAME + 6)); cv_px(x - 1, y, (uint8_t)(R_FLAME + 3));
    }
    cv_store(SP_SIGN_EYES);
}

static void spr_pool(int id, int stir) {
    cv_begin(cv_sprite, TS, TS);
    uint32_t sd = 621u + (uint32_t)stir * 9u;
    float cx = 32, cy = 58.5f, w = 25, hh = 5.0f, rx = cx + 4.0f, ry = cy + 0.5f;
    for (int y = (int)(cy - hh) - 1; y <= 63; y++)
        for (int x = (int)(cx - w) - 2; x <= (int)(cx + w) + 2; x++) {
            float nx = ((float)x + 0.5f - cx) / w, ny = ((float)y + 0.5f - cy) / hh, sh0 = 0.22f * (fbm((float)x / 5.0f, (float)y / 2.0f, 16, sd) - 0.5f);
            float d = nx * nx + ny * ny + sh0;
            if (d > 1.28f) continue;
            if (d > 1.0f) { cv_px(x, y, sh(R_WARM, 0.05f)); continue; }   /* the wet floor about it */
            float v = 0.05f + 0.07f * (1.0f + ny) * 0.5f;
            if (ny < -0.45f && fabsf(nx) < 0.6f) v += 0.12f * (1.0f - fabsf(nx));   /* the far side lit */
            if (ny < -0.62f && fabsf(nx + 0.15f) < 0.35f && h01(x, y, sd + 1) > 0.4f) v = 0.55f;   /* a light's glint */
            if (stir) {
                float rr = sqrtf(((float)x + 0.5f - rx) * ((float)x + 0.5f - rx) / 9.0f + ((float)y + 0.5f - ry) * ((float)y + 0.5f - ry));
                float ring = sinf(rr * 2.4f);
                if (ring > 0.82f && rr < 5.5f) v = 0.3f + 0.1f * (rr < 3.0f);
            }
            cv_px(x, y, sh(R_GHOST, v));
            if (h01(x, y, sd + 2) < (stir ? 0.02f : 0.05f) && d > 0.35f) cv_px(x, y, sh(R_WARM, 0.22f + 0.12f * h01(x, y, sd + 3)));   /* dust, scum */
        }
    if (!stir) {
        for (int f = 0; f < 4; f++) cv_line(22.0f + (float)f * 1.6f, 59.0f, 21.0f + (float)f * 1.9f, 61.5f, sh(R_GHOST, 0.32f));   /* the hand, under */
        cv_px(24, 58, sh(R_GHOST, 0.36f)); cv_px(25, 58, sh(R_GHOST, 0.3f));
    } else
        for (int i = 0; i < 7; i++) {                               /* bubbles */
            float x = rx + (h01(i, 1, sd) - 0.5f) * 12.0f, y = ry - 1.0f + 2.5f * h01(i, 2, sd);
            cv_px((int)x, (int)y - 1, sh(R_GHOST, 0.85f)); cv_px((int)x + 1, (int)y - 1, sh(R_GHOST, 0.5f));
            if (i < 3) { cv_px((int)x, (int)y - 3, sh(R_GHOST, 0.6f)); }
        }
    for (int k = 0; k < 5; k++) { int x = (int)(cx - w + 2.0f + h01(k, 4, sd) * (w * 2.0f - 4.0f)); cv_px(x, 63, sh(R_STONE, 0.4f)); cv_px(x + 1, 63, sh(R_STONE, 0.25f)); }   /* stones at its near edge */
    cv_store(id);
}

/* P6: the pickups. A coin of the Order lying flat at (x, y), r across, tilt its depth: a rim catching the light on its upper
 * left, the field worn, the cross struck on it (face) still to be read; its edge showing under the near side. dim darkens it
 * (deeper in a heap); clip cuts a chord off it; hole bores it through */
static void coin_flat(float x, float y, float r, float tilt, int face, int dim, int clip, int hole) {
    float ry = r * tilt;
    for (int yy = (int)floorf(y - ry) - 1; yy <= (int)ceilf(y + ry) + 1; yy++)
        for (int xx = (int)floorf(x - r) - 1; xx <= (int)ceilf(x + r) + 1; xx++) {
            float nx = ((float)xx + 0.5f - x) / r, ny = ((float)yy + 0.5f - y) / ry, d = nx * nx + ny * ny;
            if (clip && nx > 0.62f) continue;
            if (d > 1.0f) {                                         /* its edge under the near half; a dark line along the far, against what it lies on */
                float ny2 = ((float)yy - 0.5f - y) / ry;
                if (ny > 0.0f && nx * nx + ny2 * ny2 <= 1.0f) cv_px(xx, yy, (uint8_t)(R_GOLD + clampi(4 - dim - (nx > 0.3f), 1, 12)));
                else if (ny < 0.0f && d < 1.7f) cv_px(xx, yy, (uint8_t)(R_GOLD + 1));
                continue;
            }
            int v;
            if (d > 0.62f) v = (nx + ny < -0.3f) ? 12 : (nx + ny > 0.5f ? 5 : 9);   /* the rim */
            else v = 8 + (int)(2.0f * h01(xx, yy, 31) - 0.5f) - (nx + ny > 0.4f);   /* the field, worn */
            if (face && d < 0.5f && (fabsf(nx) < 0.13f || fabsf(ny) < 0.2f)) v -= 3;   /* the cross struck in it */
            if (hole && fabsf(nx) < 0.2f && fabsf(ny) < 0.3f) { cv_px(xx, yy, 0); continue; }
            cv_px(xx, yy, (uint8_t)(R_GOLD + clampi(v - dim, 0, 12)));
        }
}
/* a coin on its edge at (x, y) (its foot), r high, its face to the left lit */
static void coin_edge(float x, float y, float r, float lean, int dim) {
    for (int yy = (int)(y - 2.0f * r); yy <= (int)y; yy++)
        for (int xx = (int)(x - r) - 1; xx <= (int)(x + r) + 1; xx++) {
            float cy = y - r, ox = (float)xx + 0.5f - x - (cy - (float)yy) * lean, nx = ox / (r * 0.38f), ny = ((float)yy + 0.5f - cy) / r;
            if (nx * nx + ny * ny > 1.0f) continue;
            cv_px(xx, yy, (uint8_t)(R_GOLD + clampi((nx < -0.3f ? 11 : nx < 0.4f ? 8 : 5) - (ny > 0.6f) - dim, 0, 12)));
        }
}
/* the Order's coin, worn: a spill of it lying flat, a short stack, some on edge leaning on each other; the cross struck on the
 * nearest still to be read, one clipped, one bored for a cord; frame 1 moves the glints */
static void spr_coins(int id, int frame) {
    cv_begin(cv_sprite, TS, TS);
    cv_flat(32, 62, 18, 2.2f, R_WARM + 1);
    float cx[16], cy[16];
    int n = 0;
    for (int i = 0; i < 16; i++) {                                  /* the spill, loose, spreading from the stack */
        float r = 3.0f + sqrtf(h01(i, 1, 141)) * 15.0f, a = -0.5f + h01(i, 2, 141) * 1.8f;
        cx[n] = 22.0f + cosf(a) * r; cy[n] = 58.5f + sinf(a) * r * 0.3f + (h01(i, 3, 141) - 0.5f) * 1.5f; n++;
        if (cy[n - 1] > 61.5f) cy[n - 1] = 61.5f;
    }
    for (int i = 0; i < n; i++) for (int j = i + 1; j < n; j++) if (cy[j] < cy[i]) { float t = cy[i]; cy[i] = cy[j]; cy[j] = t; t = cx[i]; cx[i] = cx[j]; cx[j] = t; }
    for (int k = 0; k < 6; k++) coin_flat(19.0f + (float)(k & 1) * 0.7f - (float)(k == 3) * 0.6f, 58.5f - (float)k * 1.6f, 4.3f, 0.45f, k == 5, 0, 0, 0);   /* a stack */
    for (int i = 0; i < n; i++) coin_flat(cx[i], cy[i], 4.2f, 0.45f, i == n - 1 || i == n - 4 || i == n - 8, cy[i] < 57.0f ? 1 : 0, i == n - 3, i == n - 6);
    coin_edge(25.5f, 58.0f, 3.8f, 0.3f, 0);                         /* one on its edge against the stack */
    cv_outline(R_GOLD);
    static const int G[2][4][2] = { { {16, 50}, {39, 57}, {29, 59}, {46, 60} }, { {17, 58}, {33, 58}, {43, 56}, {24, 53} } };
    for (int i = 0; i < 4; i++) cv_px(G[frame][i][0], G[frame][i][1], C_WHITE);
    cv_store(id);
}

/* a stone cut in steps, at (cx, cy), r across, in its ramp: a table on top, the crown's facets lit and shaded by the light from the
 * upper left, the light going through and catching low on its far side; full lets a glint stay white */
static void cut_gem(float cx, float cy, float r, int ramp) {
    for (int y = (int)floorf(cy - r); y <= (int)ceilf(cy + r); y++)
        for (int x = (int)floorf(cx - r); x <= (int)ceilf(cx + r); x++) {
            float dx = ((float)x + 0.5f - cx) / r, dy = ((float)y + 0.5f - cy) / r;
            if (fabsf(dx) + fabsf(dy) > 1.32f || fabsf(dx) > 1.0f || fabsf(dy) > 1.0f) continue;
            float v;
            if (fabsf(dx) < 0.42f && dy > -0.75f && dy < -0.15f) v = 0.86f;   /* the table */
            else if (dy < -0.15f) v = dx < 0.0f ? 0.96f : 0.62f;            /* the crown's facets */
            else v = dx < -0.2f ? 0.42f : (dx > 0.3f && dy > 0.3f ? 0.9f : 0.24f);   /* the pavilion, the light through it */
            cv_px(x, y, sh(ramp, v));
        }
    cv_px((int)(cx - r * 0.35f), (int)(cy - r * 0.55f), C_WHITE);
}
/* a cabochon in its collet at (cx, cy): a dome of the stone in a raised gilt ring */
static void cabochon(float cx, float cy, float r, int ramp) {
    cv_ellipse(cx, cy, r + 1.0f, (r + 1.0f) * 0.7f, R_GOLD, 0.3f, 0.95f);
    cv_ellipse(cx, cy - 0.3f, r, r * 0.7f, ramp, 0.25f, 0.95f);
    cv_px((int)(cx - r * 0.4f), (int)(cy - r * 0.4f), C_WHITE);
}
/* the Order's chalice: a foot of six lobes, a stem with a knop set with enamel lozenges, a deep bowl with an engraved band and
 * a red enamelled cross; the wine in it long gone to a black crust; its lip dented where it fell */
static void spr_chalice(void) {
    cv_begin(cv_sprite, TS, TS);
    const float cx = 32;
    cv_flat(cx, 62, 11, 1.8f, R_WARM + 1);
    for (int y = 51; y <= 61; y++)                                  /* the foot, its six lobes, rising to the stem */
        for (int x = 20; x <= 44; x++) {
            float ox = (float)x + 0.5f - cx;
            if (y >= 57) {
                float a = atan2f(((float)y - 59.0f) * 3.4f, ox), rr = 10.0f * (0.86f + 0.14f * fabsf(cosf(3.0f * a)));
                if (ox * ox + ((float)y - 59.0f) * 3.4f * ((float)y - 59.0f) * 3.4f > rr * rr) continue;
                cv_px(x, y, sh(R_GOLD, (y == 61 ? 0.25f : 0.45f) + 0.45f * (ox < 0.0f ? -ox / 10.0f : 0.0f) - 0.2f * (ox > 4.0f)));
            } else {
                float hw = 1.6f + 6.0f * powf((float)(y - 51) / 6.0f, 2.0f);
                if (fabsf(ox) > hw) continue;
                float nx = ox / hw;
                cv_px(x, y, sh(R_GOLD, 0.3f + 0.65f * lit(nx, -0.4f, sqrtf(fmaxf(0.0f, 1.0f - nx * nx)))));
            }
        }
    cv_capsule(cx, 44, cx, 52, 1.4f, R_GOLD, 0.3f, 1.0f);           /* the stem */
    cv_ellipse(cx, 48, 3.8f, 2.3f, R_GOLD, 0.3f, 1.0f);             /* the knop, */
    for (int k = 0; k < 3; k++) { int x = (int)cx - 2 + k * 2; cv_px(x, 48, k == 1 ? sh(R_BLOOD, 0.6f) : sh(R_GHOST, 0.55f)); cv_px(x, 47, sh(R_GOLD, 0.95f)); }   /* its lozenges */
    for (int y = 32; y <= 44; y++) {                                /* the bowl */
        float t = (float)(y - 32) / 12.0f, hw = 9.2f * sqrtf(fmaxf(0.0f, 1.0f - powf(t, 2.6f))) + 0.6f, dr = -9.2f * 2.6f * powf(t, 1.6f) * 0.5f / 12.0f;
        for (int x = (int)(cx - hw) - 1; x <= (int)(cx + hw) + 1; x++) {
            float nx = ((float)x + 0.5f - cx) / hw;
            if (fabsf(nx) > 1.0f) continue;
            float nz = sqrtf(1.0f - nx * nx), ny = -dr, nl = sqrtf(nx * nx + ny * ny + nz * nz);
            float spec = powf(fmaxf(0.0f, (nx * -0.25f + ny * -0.37f + nz * 0.89f) / nl), 14.0f);
            float v = 0.25f + 0.45f * lit(nx / nl, ny / nl, nz / nl) + 0.4f * spec;
            if (y == 35) v += 0.2f; else if (y == 36) v -= 0.22f;    /* its engraved band */
            cv_px(x, y, sh(R_GOLD, v));
        }
    }
    for (int k = -2; k <= 2; k++) { cv_px((int)cx + k, 39, sh(R_BLOOD, 0.62f)); cv_px((int)cx, 39 + k, sh(R_BLOOD, 0.62f)); }   /* the cross in red enamel */
    cv_px((int)cx - 1, 38, sh(R_BLOOD, 0.85f));
    cv_ellipse(cx, 32.0f, 9.6f, 2.2f, R_GOLD, 0.4f, 0.95f);         /* the lip, and the crust inside */
    cv_flat(cx, 32.2f, 8.0f, 1.3f, sh(R_BLOOD, 0.08f));
    cv_px(27, 32, sh(R_BLOOD, 0.18f)); cv_px(34, 32, sh(R_WARM, 0.1f));
    cv_px(39, 31, C_CLEAR); cv_px(40, 31, C_CLEAR); cv_px(39, 32, sh(R_GOLD, 0.3f)); cv_px(40, 32, sh(R_GOLD, 0.5f));   /* the dent */
    cv_outline(0);
    cv_px(26, 35, C_WHITE); cv_px(25, 38, C_WHITE);
    cv_store(SP_CHALICE);
}

/* gems torn from a reliquary: its gilt mount of filigree on the floor, two cabochons still in their collets and one collet
 * empty with its claws pried open; loose stones about it cut in steps, a ruby, a sapphire, an emerald, an amethyst; a pearl;
 * a few coins under them */
static void spr_gems(void) {
    cv_begin(cv_sprite, TS, TS);
    cv_flat(32, 62, 16, 2.0f, R_WARM + 1);
    for (int k = 0; k < 5; k++) coin_flat(18.0f + (float)k * 7.0f + 2.0f * h01(k, 1, 151), 60.5f + h01(k, 2, 151) * 1.5f, 2.8f, 0.42f, k == 2, 1, 0, 0);
    for (int y = 53; y <= 61; y++)                                  /* the mount, an oval plaque lying flat */
        for (int x = 15; x <= 38; x++) {
            float nx = ((float)x + 0.5f - 26.5f) / 11.0f, ny = ((float)y + 0.5f - 57.0f) / 3.8f, d = nx * nx + ny * ny;
            if (d > 1.0f) continue;
            int v = d > 0.7f ? ((x + y) & 1 ? 11 : 6) : 8 - (nx + ny > 0.4f) + (int)(1.5f * h01(x, y, 153));   /* a filigree rim */
            if (d < 0.7f && ((x * 3 + y * 5) % 7 == 0)) v -= 3;      /* its scrollwork */
            cv_px(x, y, (uint8_t)(R_GOLD + clampi(v, 0, 12)));
        }
    cv_px(37, 59, sh(R_GOLD, 0.2f)); cv_px(38, 59, C_CLEAR);       /* bent at its end */
    cabochon(20.5f, 56.5f, 2.2f, R_BLOOD);
    cabochon(26.5f, 57.5f, 2.6f, R_MOSS);
    cv_ellipse(32.5f, 56.5f, 3.0f, 2.0f, R_GOLD, 0.3f, 0.9f); cv_flat(32.5f, 56.6f, 1.8f, 1.1f, sh(R_GOLD, 0.05f));   /* the empty collet, */
    cv_px(30, 54, sh(R_GOLD, 0.9f)); cv_px(35, 54, sh(R_GOLD, 0.8f)); cv_px(35, 53, sh(R_GOLD, 0.6f));   /* its claws pried up */
    cut_gem(43, 55.5f, 5.0f, R_BLOOD);                              /* where its stone went */
    cut_gem(50, 59.0f, 3.4f, R_GHOST);
    cut_gem(12, 59.5f, 3.0f, R_MOSS);
    cut_gem(36, 60.5f, 2.6f, R_PURPLE);
    cv_ellipse(46, 61.5f, 1.6f, 1.4f, R_BONE, 0.45f, 1.0f);         /* a pearl */
    cv_outline(0);
    cv_px(42, 52, C_WHITE); cv_px(49, 57, C_WHITE);
    cv_store(SP_GEMS);
}

/* a crown of the Order's dead: a gilt circlet seen a little from above, its band chased and set with stones in collets, one
 * collet empty; crosses and fleurons standing about it in turn, the far ones seen across the near; the velvet of its cap gone
 * to rags inside; one fleuron bent down */
static void crown_point(float x, float y, int cross, float lit_v, float bend) {
    int hgt = cross ? 9 : 7;
    for (int k = 0; k <= hgt; k++) {
        float px = x + bend * (float)k * 0.8f, py = y - (float)k * (1.0f - fabsf(bend) * 0.4f);
        cv_px((int)px, (int)py, sh(R_GOLD, lit_v)); cv_px((int)px + 1, (int)py, sh(R_GOLD, lit_v * 0.6f));
        if (cross && k == hgt - 3) for (int j = -2; j <= 3; j++) cv_px((int)px + j, (int)py, sh(R_GOLD, lit_v * (j < 1 ? 1.0f : 0.65f)));
        if (!cross && k == hgt - 3) { cv_px((int)px - 1, (int)py - 1, sh(R_GOLD, lit_v)); cv_px((int)px - 2, (int)py - 2, sh(R_GOLD, lit_v)); cv_px((int)px + 2, (int)py - 1, sh(R_GOLD, lit_v * 0.6f)); cv_px((int)px + 3, (int)py - 2, sh(R_GOLD, lit_v * 0.6f)); }
    }
    cv_ellipse(x + bend * (float)hgt * 0.8f + 0.5f, y - (float)hgt * (1.0f - fabsf(bend) * 0.4f), 1.2f, 1.2f, cross ? R_GOLD : R_BONE, 0.5f, 1.0f);
}
static void spr_crown(void) {
    cv_begin(cv_sprite, TS, TS);
    const float cx = 32, ty = 50, by = 57, rx = 13, ry = 3.4f;
    cv_flat(cx, 62, 15, 2.0f, R_WARM + 1);
    for (int k = 0; k < 8; k++) {                                   /* the far points */
        float th = TAU_F * ((float)k + 0.5f) / 8.0f;
        if (sinf(th) >= 0.0f) continue;
        crown_point(cx + rx * cosf(th), ty + ry * sinf(th), k & 1, 0.45f + 0.15f * (cosf(th) < 0.0f), 0.0f);
    }
    for (int a = 0; a < 160; a++) {                                 /* the band's inside, far half */
        float th = TAU_F * (float)a / 160.0f, s = sinf(th);
        if (s >= 0.0f) continue;
        int x = (int)(cx + rx * cosf(th));
        for (int y = (int)(ty + ry * s); y <= (int)(by + ry * s); y++) cv_px(x, y, sh(R_GOLD, 0.22f + 0.2f * (cosf(th) > 0.0f)));
    }
    for (int y = (int)ty - 1; y <= (int)by; y++)                    /* the cap's velvet, gone to rags */
        for (int x = (int)(cx - rx) + 2; x <= (int)(cx + rx) - 2; x++)
            if (h01(x, y, 161) < 0.55f && (float)y > ty - 3.0f + 2.0f * fabsf(((float)x - cx) / rx)) cv_px(x, y, sh(R_BLOOD, 0.1f + 0.12f * h01(x, y, 163)));
    for (int a = 0; a < 200; a++) {                                 /* the band's face, near half */
        float th = TAU_F * (float)a / 200.0f, s = sinf(th), c = cosf(th);
        if (s < 0.0f) continue;
        int x = (int)(cx + rx * c);
        for (int y = (int)(ty + ry * s); y <= (int)(by + ry * s); y++) {
            float v = 0.42f + 0.5f * (0.5f - 0.5f * c);
            int row = y - (int)(ty + ry * s);
            if (row == 0) v += 0.18f; else if (row == (int)(by - ty)) v -= 0.18f;
            else if (((x + row * 2) % 6) == 0) v -= 0.16f;          /* chased */
            cv_px(x, y, sh(R_GOLD, v));
        }
    }
    static const float SET[4] = { 0.27f, 0.5f, 0.73f, 0.9f };
    static const int SRAMP[4] = { R_GHOST, R_BLOOD, R_MOSS, -1 };
    for (int k = 0; k < 4; k++) {                                   /* its stones; one gone */
        float th = PI_F * SET[k], x = cx + rx * cosf(th), y = (ty + by) * 0.5f + ry * sinf(th);
        if (SRAMP[k] < 0) { cv_ellipse(x, y, 1.8f, 1.6f, R_GOLD, 0.3f, 0.9f); cv_px((int)x, (int)y, sh(R_GOLD, 0.03f)); }
        else cabochon(x, y, k == 1 ? 2.2f : 1.7f, SRAMP[k]);
    }
    for (int k = 0; k < 8; k++) {                                   /* the near points; the one at the right bent down */
        float th = TAU_F * ((float)k + 0.5f) / 8.0f;
        if (sinf(th) < 0.0f) continue;
        crown_point(cx + rx * cosf(th), ty + ry * sinf(th), k & 1, 0.75f + 0.2f * (cosf(th) < 0.0f), k == 0 ? 0.9f : 0.0f);
    }
    cv_outline(0);
    cv_px(22, 52, C_WHITE); cv_px(31, 41, C_WHITE);
    cv_store(SP_CROWN);
}

/* an iron-bound chest with its lid thrown back: its planks with their grain, iron straps riveted across them, corner plates,
 * strap hinges into the lid; its lock plate hanging from one nail where the hasp was forced; heaped with coin, a string of
 * pearls hanging over its edge, a chalice's foot out of the hoard */
static void spr_chest(void) {
    cv_begin(cv_sprite, TS, TS);
    cv_flat(32, 62, 22, 2.2f, R_WARM + 1);
    for (int y = 29; y <= 46; y++)                                  /* the lid, thrown back: its inside, the planks and straps */
        for (int x = 13; x <= 51; x++) {
            float u = (float)(y - 29) / 17.0f, x0 = 17.0f - 3.0f * u, x1 = 47.0f + 3.0f * u;
            if ((float)x < x0 || (float)x > x1) continue;
            int strap = (fabsf((float)x - (21.0f - 1.2f * u)) < 1.3f) || (fabsf((float)x - (43.0f + 1.2f * u)) < 1.3f);
            float v = 0.14f + 0.08f * fbm((float)x / 7.0f, (float)y / 1.6f, 16, 171) + 0.05f * u - (((y - 29) % 6) == 0 ? 0.07f : 0.0f);
            cv_px(x, y, strap ? sh(R_IRON, 0.32f + 0.15f * ((x & 1) == 0)) : sh(R_WOOD, v));
        }
    for (int x = 17; x <= 47; x++) cv_px(x, 29, sh(R_IRON, 0.62f));   /* the lid's iron edge */
    for (int i = 0; i < 26; i++) {                                  /* the hoard, heaped */
        float x = 16.0f + h01(i, 6, 173) * 32.0f, y = 46.5f - h01(i, 7, 173) * 5.0f * (1.0f - fabsf(x - 32.0f) / 18.0f);
        coin_flat(x, y, 3.0f, 0.42f, i % 7 == 3, i < 10, 0, 0);
    }
    cv_ellipse(37, 40.5f, 2.6f, 1.0f, R_GOLD, 0.4f, 0.95f); cv_capsule(37, 41, 37.5f, 44, 0.9f, R_GOLD, 0.35f, 0.9f);   /* a chalice's foot */
    for (int y = 47; y <= 62; y++)                                  /* the box */
        for (int x = 13; x <= 51; x++) {
            int plank = (y - 47) / 5;
            float v = 0.32f + 0.08f * h01(plank, 1, 175) + 0.12f * fbm((float)x / 7.0f, (float)y / 1.4f, 16, 177) - (y == 47 + plank * 5 ? 0.14f : 0.0f) - 0.08f * (x > 48);
            uint8_t c = sh(R_WOOD, v);
            int band = (x >= 19 && x <= 21) || (x >= 43 && x <= 45), corner = (x <= 15 || x >= 49) && (y <= 50 || y >= 59);
            if (band || corner || y >= 61 || y == 47) {
                c = sh(R_IRON, 0.38f + (x == 19 || x == 43 || x == 13 ? 0.22f : 0.0f) + (y == 47 ? 0.2f : 0.0f) - (x > 48 ? 0.12f : 0.0f));
                if ((band && (y - 48) % 3 == 0 && (x == 20 || x == 44)) || (corner && (x == 14 || x == 50) && (y == 49 || y == 60))) c = sh(R_IRON, 0.92f);   /* rivets */
            }
            cv_px(x, y, c);
        }
    for (int y = 49; y <= 57; y++)                                  /* the lock plate, hanging askew from its one nail */
        for (int x = 26; x <= 38; x++) {
            float ox = (float)x - 32.0f, oy = (float)y - 49.5f, rx = ox * 0.9f - oy * 0.42f, ry2 = ox * 0.42f + oy * 0.9f;
            if (fabsf(rx) > 3.2f || ry2 < 0.0f || ry2 > 6.5f) continue;
            uint8_t c = sh(R_IRON, 0.5f + 0.25f * (rx < -2.2f) - 0.15f * (rx > 2.2f));
            if (fabsf(rx) < 0.8f && ry2 > 2.5f && ry2 < 4.6f) c = 0;  /* its keyhole */
            cv_px(x, y, c);
        }
    cv_px(32, 49, sh(R_IRON, 0.95f));                               /* the nail */
    cv_capsule(32, 44.5f, 32, 46.5f, 0.9f, R_IRON, 0.2f, 0.7f);    /* the hasp, torn off short */
    for (int k = 0; k < 9; k++) cv_ellipse(46.0f + (float)k * 0.55f, 46.0f + (float)k * 1.4f - (float)(k * k) * 0.05f, 0.9f, 0.9f, R_BONE, 0.4f, 1.0f);   /* pearls over its edge */
    cv_outline(0);
    cv_px(24, 42, C_WHITE); cv_px(39, 43, C_WHITE);
    cv_store(SP_CHEST);
}

/* a stoppered phial: a round flask of thick glass, the red draught in it to a meniscus, the light through it gathering low on
 * its far side; the cork pushed home under red wax run down the neck, a cord round the neck with a paper on it marked with the
 * apothecary's cross; a bubble caught in the glass */
static void spr_phial(void) {
    cv_begin(cv_sprite, TS, TS);
    const float cx = 31, cy = 52.5f, r = 9.4f, lvl = 47.5f;
    cv_flat(cx, 62, 10, 1.8f, R_WARM + 1);
    for (int y = (int)(cy - r); y <= (int)(cy + r); y++)
        for (int x = (int)(cx - r); x <= (int)(cx + r); x++) {
            float nx = ((float)x + 0.5f - cx) / r, ny = ((float)y + 0.5f - cy) / r, d = nx * nx + ny * ny;
            if (d > 1.0f) continue;
            float nz = sqrtf(1.0f - d), l = lit(nx, ny, nz), surf = lvl + 0.8f * nx * nx;
            uint8_t c;
            if ((float)y < surf) c = sh(R_GHOST, 0.1f + 0.2f * l + (d > 0.8f ? 0.12f : 0.0f));   /* the glass, empty above */
            else if ((float)y < surf + 1.0f) c = sh(R_BLOOD, 0.7f);  /* the meniscus */
            else {                                                  /* the draught: dark where it faces the light, glowing on its far side */
                float through = fmaxf(0.0f, nx * 0.6f + ny * 0.7f);
                c = sh(R_BLOOD, 0.18f + 0.3f * (1.0f - l) * 0.4f + 0.62f * powf(through, 1.4f) + (d > 0.86f ? -0.08f : 0.0f));
            }
            if (d > 0.88f && nx + ny < -0.4f) c = sh(R_GHOST, 0.7f);   /* the glass's edge, lit */
            cv_px(x, y, c);
        }
    for (int a = 0; a < 7; a++) cv_px((int)(cx - 5.0f + cosf((float)a * 0.32f + 3.6f) * 3.0f), (int)(cy - 4.0f + sinf((float)a * 0.32f + 3.6f) * 3.0f), sh(R_GHOST, 0.85f));   /* the window's glint */
    for (int y = 36; y <= 43; y++)                                  /* the neck */
        for (int x = (int)cx - 2; x <= (int)cx + 2; x++) cv_px(x, y, sh(R_GHOST, x == (int)cx - 2 ? 0.62f : (x == (int)cx + 2 ? 0.16f : 0.24f)));
    for (int x = (int)cx - 3; x <= (int)cx + 3; x++) cv_px(x, 43, sh(R_GHOST, 0.4f));
    for (int y = 31; y <= 36; y++)                                  /* the cork under its wax */
        for (int x = (int)cx - 3; x <= (int)cx + 3; x++) cv_px(x, y, y <= 33 ? sh(R_BLOOD, 0.3f + 0.25f * (x < (int)cx) + (y == 31 ? 0.15f : 0.0f)) : sh(R_WOOD, 0.4f - 0.06f * (float)(x - (int)cx)));
    cv_px((int)cx - 3, 34, sh(R_BLOOD, 0.35f)); cv_px((int)cx - 3, 35, sh(R_BLOOD, 0.3f)); cv_px((int)cx + 2, 34, sh(R_BLOOD, 0.25f)); cv_px((int)cx + 2, 35, sh(R_BLOOD, 0.22f)); cv_px((int)cx + 2, 36, sh(R_BLOOD, 0.2f));   /* the wax run down */
    for (int x = (int)cx - 3; x <= (int)cx + 3; x++) cv_px(x, 40, sh(R_WARM, 0.55f - 0.05f * (float)(x - (int)cx)));   /* the cord */
    cv_line(cx + 3.0f, 40.0f, cx + 8.0f, 43.0f, sh(R_WARM, 0.45f));
    for (int y = 43; y <= 50; y++) for (int x = (int)cx + 7; x <= (int)cx + 13; x++) {   /* the paper */
        if (y == 50 && x > (int)cx + 10) continue;
        uint8_t c = sh(R_BONE, 0.62f - 0.04f * (float)(y - 43) - (x == (int)cx + 13 ? 0.15f : 0.0f));
        if ((x == (int)cx + 10 && y >= 44 && y <= 48) || (y == 46 && x >= (int)cx + 8 && x <= (int)cx + 12)) c = sh(R_BLOOD, 0.45f);
        cv_px(x, y, c);
    }
    cv_px((int)cx + 4, 55, sh(R_BLOOD, 0.9f)); cv_px((int)cx + 5, 54, sh(R_BLOOD, 0.75f));   /* a bubble */
    cv_outline(0);
    cv_px((int)cx - 6, 47, C_WHITE); cv_px((int)cx - 6, 48, C_WHITE); cv_px((int)cx - 5, 46, C_WHITE);
    cv_store(SP_POTION);
}

/* the soul urn: a jar of dark bronze, its lid a hooded face with its eyes shut, a band of the Order's crosses about its shoulder
 * pierced through; the soul in it lights the piercings from within and seeps out under the lid, and a thread of it rises */
static void spr_soulurn(void) {
    cv_begin(cv_sprite, TS, TS);
    const float cx = 32;
    cv_flat(cx, 62, 12, 2.0f, R_WARM + 1);
    for (int y = 39; y <= 61; y++) {                                /* the jar */
        float t = (float)(y - 39) / 22.0f, hw = y >= 59 ? 7.5f : 6.5f + 5.0f * sinf(PI_F * fminf(1.0f, t * 1.15f + 0.12f)), dr = 5.0f * PI_F * 1.15f / 22.0f * cosf(PI_F * fminf(1.0f, t * 1.15f + 0.12f));
        if (y >= 56 && y < 59) hw = 6.0f;
        for (int x = (int)(cx - hw) - 1; x <= (int)(cx + hw) + 1; x++) {
            float nx = ((float)x + 0.5f - cx) / hw;
            if (fabsf(nx) > 1.0f) continue;
            float nz = sqrtf(1.0f - nx * nx), ny = -dr * 0.6f, nl = sqrtf(nx * nx + ny * ny + nz * nz);
            float v = 0.08f + 0.42f * lit(nx / nl, ny / nl, nz / nl) + 0.3f * powf(fmaxf(0.0f, (nx * -0.25f + ny * -0.37f + nz * 0.89f) / nl), 12.0f);
            if (y == 41 || y == 47) v += 0.12f; else if (y == 42 || y == 48) v -= 0.08f;   /* the band's edges */
            uint8_t c = sh(R_WOOD, v);
            if (fbm((float)x / 2.5f, (float)y / 3.0f, 16, 181) > 0.66f) c = sh(R_MOSS, 0.12f + 0.25f * v);   /* green with age */
            cv_px(x, y, c);
        }
    }
    for (int k = 0; k < 4; k++) {                                   /* the crosses, pierced, lit from inside */
        float th = PI_F * (0.2f + 0.2f * (float)k), x = cx - cosf(th) * 9.6f;
        int X = (int)x, Y = 44;
        uint8_t g = (uint8_t)(R_GLOW + 13 - (k == 0 || k == 3) * 3);
        cv_px(X, Y, g); cv_px(X, Y - 1, g); cv_px(X, Y + 1, g); cv_px(X - 1, Y, g); cv_px(X + 1, Y, g);
    }
    for (int y = 29; y <= 39; y++)                                  /* the lid: a hooded face, its eyes shut */
        for (int x = (int)cx - 8; x <= (int)cx + 8; x++) {
            float ox = (float)x + 0.5f - cx, hw = y < 32 ? 3.0f + (float)(y - 29) * 1.6f : (y < 38 ? 7.6f : 8.4f);
            if (fabsf(ox) > hw) continue;
            float nx = ox / hw, v = 0.1f + 0.4f * lit(nx, -0.3f, sqrtf(fmaxf(0.0f, 1.0f - nx * nx)));
            int face = y >= 32 && y <= 37 && fabsf(ox) < 4.0f;
            if (face) {
                v += 0.12f;
                if (y == 34 && fabsf(fabsf(ox) - 2.0f) < 1.1f) v = 0.04f;   /* the shut eyes */
                if (y == 35 && fabsf(ox) < 0.6f) v += 0.15f;                /* the nose */
                if (y == 37 && fabsf(ox) < 1.6f) v = 0.06f;                 /* the mouth */
                if (fabsf(ox) > 3.2f) v = 0.04f;                            /* the hood's shadow on it */
            }
            cv_px(x, y, sh(R_WOOD, v));
        }
    cv_outline(0);
    for (int x = (int)cx - 7; x <= (int)cx + 7; x++) if (h01(x, 1, 183) < 0.55f) cv_px(x, 39, (uint8_t)(R_GLOW + 8 + (int)(h01(x, 2, 183) * 6.0f)));   /* seeping under the lid */
    for (int i = 0; i < 14; i++)                                    /* and rising */
        cv_px((int)(cx + sinf((float)i * 0.8f) * 2.0f + (h01(i, 1, 185) - 0.5f) * 3.0f), 28 - (int)((float)i * 1.3f), (uint8_t)(R_GLOW + 8 + (i % 6)));
    cv_store(SP_SOULURN);
}

/* a ring on its velvet: a cushion of violet velvet, piped with gold cord, tasselled at its corners and sunk where the ring stands
 * in it; a gold signet set with a ruby, its shank chased; dust on the velvet */
static void spr_ringcushion(void) {
    cv_begin(cv_sprite, TS, TS);
    const float cx = 32;
    cv_flat(cx, 62.5f, 16, 1.8f, R_WARM + 1);
    for (int y = 50; y <= 62; y++)                                  /* the cushion: its top, then its front */
        for (int x = 15; x <= 49; x++) {
            float ox = (float)x + 0.5f - cx, top = 52.0f + 1.6f * (ox / 17.0f) * (ox / 17.0f) - 1.5f * expf(-ox * ox / 30.0f) * 0.0f, front = 57.5f;
            float edge = 16.5f - 1.2f * fabsf(((float)y - 56.0f) / 6.0f);
            if (fabsf(ox) > edge || (float)y < top) continue;
            float v;
            if ((float)y < front) {
                float dd = expf(-(ox * ox + ((float)y - 54.5f) * ((float)y - 54.5f) * 4.0f) / 18.0f);   /* sunk round the ring */
                v = 0.45f + 0.2f * (ox < 0.0f ? -ox / 17.0f : -ox / 34.0f) - 0.25f * dd;
            } else v = 0.2f + 0.1f * (ox < 0.0f) - 0.03f * (float)(y - 58);
            v += 0.05f * (h01(x, y, 191) - 0.5f);
            cv_px(x, y, sh(R_PURPLE, v));
            if ((int)floorf(front) == y || fabsf(fabsf(ox) - edge) < 0.6f) cv_px(x, y, sh(R_GOLD, (y + x) & 1 ? 0.85f : 0.55f));   /* its piping */
        }
    for (int s = -1; s <= 1; s += 2)                                /* tassels */
        for (int k = 0; k < 3; k++) cv_line(cx + (float)s * 16.0f + (float)k * 0.6f * (float)s, 58.0f, cx + (float)s * 16.5f + (float)(k - 1) * 0.8f, 62.5f, sh(R_GOLD, 0.7f - 0.15f * (float)k));
    for (int a = 0; a < 90; a++) {                                  /* the ring, standing */
        float t = (float)a / 90.0f * TAU_F, c = cosf(t);
        for (int w = -2; w <= 2; w++) {
            float rr = 5.6f + (float)w * 0.45f, v = 0.5f + 0.42f * cosf(t + 2.3f) * (w <= 0 ? 1.0f : 0.6f) - 0.1f * (float)(w > 1);
            if (a % 11 == 0 && w == 0) v -= 0.3f;                  /* chased */
            cv_px((int)(cx + c * rr), (int)(47.0f + sinf(t) * rr), sh(R_GOLD, v));
        }
    }
    cv_ellipse(cx, 41.0f, 3.6f, 2.0f, R_GOLD, 0.35f, 0.95f);        /* the bezel, */
    cut_gem(cx, 39.5f, 2.6f, R_BLOOD);                              /* its stone */
    for (int k = 0; k < 6; k++) cv_px((int)(cx - 12.0f + h01(k, 1, 193) * 24.0f), 53 + (int)(h01(k, 2, 193) * 4.0f), sh(R_WARM, 0.5f));   /* dust */
    cv_outline(0);
    cv_px((int)cx - 4, 44, C_WHITE);
    cv_store(SP_RINGITEM);
}

/* a bone on the floor, for the spear: a femur, its head and its knuckled end, stained, gnawed at the knee */
static void spr_spearbone(void) {
    cv_begin(cv_sprite, TS, TS);
    cv_flat(32, 61.5f, 14, 1.6f, R_WARM + 1);
    prop_long(20, 58, 45, 54, 1.8f, 1, 0, 0.0f, 197);
    for (int k = 0; k < 3; k++) cv_px(41 + k, 54 + (k & 1), sh(R_BONE, 0.15f));   /* gnawed */
    cv_outline(0);
    cv_store(SP_ITEM_BONE);
}

/* every prop drawn so far, at the level's age, and washed in run II */
static void props_level(int level) {
    int wet = crawl_set(level) == 1;
    spr_bonelitter(SP_BONES1, 0); spr_bonelitter(SP_BONES1_2, 1); spr_bonelitter(SP_BONES1_3, 2);
    spr_boneheap(SP_BONES2, 0); spr_boneheap(SP_BONES2_2, 1);
    spr_skullstack(SP_SKULLPILE, 0); spr_skullstack(SP_SKULLPILE_2, 1); spr_skullstack(SP_SKULLPILE_3, 2);
    for (int f = 0; f < 2; f++) { spr_totempost(SP_TOTEM_A + f, 0, f); spr_totempost(SP_TOTEM2_A + f, 1, f); }
    spr_candlecluster(SP_CANDLES, 0); spr_candlecluster(SP_CANDLES_2, 1); spr_candlecluster(SP_CANDLES_3, 2);
    for (int f = 0; f < 2; f++) { spr_prickstand(SP_CANDLESTAND_A + f, f); spr_corona(SP_CHANDELIER_A + f, f); spr_hornlantern(SP_LANTERN_A + f, f); }
    for (int f = 0; f < 3; f++) { spr_hearth(SP_FIREPIT_A + f, f); spr_gbrazier(SP_BRAZIER_A + f, f); }
    static const int16_t GRAVES[4][2] = { { SP_TOMB_ROUND, SP_TOMB_ROUND_2 }, { SP_TOMB_CROSS, SP_TOMB_CROSS_2 }, { SP_TOMB_OBELISK, SP_TOMB_OBELISK_2 }, { SP_TOMB_BROKEN, SP_TOMB_BROKEN_2 } };
    for (int k = 0; k < 4; k++) for (int l = 0; l < 2; l++) spr_grave(GRAVES[k][l], k, l);
    spr_riser_grave(SP_MOUND, 0); spr_riser_grave(SP_PIT, 1);
    spr_hangchains(SP_CHAINS, 0); spr_hangchains(SP_CHAINS_2, 1); spr_hookbeam();
    spr_cage(SP_GIBBET, 0); spr_cage(SP_GIBBET_SKEL, 1); spr_bronzebell();
    spr_hangroots(SP_ROOTS, 0); spr_hangroots(SP_ROOTS_2, 1);
    spr_cornerweb(SP_COBWEB, 0); spr_cornerweb(SP_COBWEB_2, 1); spr_websheet2(SP_WEBSHEET, 0); spr_websheet2(SP_WEBSHEET_2, 1);
    spr_iceteeth();
    static const int16_t URNS[3] = { SP_URN, SP_URN_2, SP_URN_3 };
    spr_gothpillar(SP_PILLAR, 0); spr_gothpillar(SP_PILLAR_2, 1); spr_flowstone(); spr_altarstone();
    for (int k = 0; k < 3; k++) spr_cinerary(URNS[k], k);
    spr_brokenarms(); spr_spikecollar(); spr_pool(SP_PUDDLE_A, 0); spr_pool(SP_PUDDLE_B, 1);
    if (wet) {                                                      /* stone and floor: the silt line, weed, rust (flowstone and water are wet already) */
        static const int16_t FLOORS[] = { SP_PILLAR, SP_PILLAR_2, SP_URN, SP_URN_2, SP_URN_3, SP_BROKEN_ARMS, SP_COLLAR };
        for (int i = 0; i < (int)(sizeof FLOORS / sizeof FLOORS[0]); i++) prop_wash(FLOORS[i], 0, 841u + (uint32_t)i * 5u);
        prop_wash_ex(SP_BALTAR, 0, 0, 879);                          /* black stone, drawn in the iron's ramp: silt, but no rust */
    }
    if (wet) {                                                      /* what hangs: rust, and water dripping off it; the webs beaded */
        static const int16_t HANGS[] = { SP_CHAINS, SP_CHAINS_2, SP_HOOKS, SP_GIBBET, SP_GIBBET_SKEL, SP_BELL, SP_ROOTS, SP_ROOTS_2,
                                         SP_COBWEB, SP_COBWEB_2, SP_WEBSHEET, SP_WEBSHEET_2 };
        for (int i = 0; i < (int)(sizeof HANGS / sizeof HANGS[0]); i++) prop_wash(HANGS[i], 1, 801u + (uint32_t)i * 3u);
    }
    if (wet) {
        for (int k = 0; k < 4; k++) for (int l = 0; l < 2; l++) prop_wash(GRAVES[k][l], 0, 771u + (uint32_t)(k * 2 + l));
        prop_wash(SP_MOUND, 0, 781); prop_wash(SP_PIT, 0, 783);
    }
    if (wet) {
        for (int f = 0; f < 2; f++) prop_wash(SP_CHANDELIER_A + f, 1, 743);   /* iron overhead: rust and drips */
        for (int f = 0; f < 3; f++) { prop_wash(SP_FIREPIT_A + f, 0, 745); prop_wash(SP_BRAZIER_A + f, 0, 747); }
    }
    {                                                               /* the brazier put out: the fire gone, ash where it sat; a trial's, out and lit */
        uint8_t *d = crawl_spr[SP_BRAZIER_OUT];
        memcpy(d, crawl_spr[SP_BRAZIER_A], TS * TS);
        for (int i = 0; i < TS * TS; i++)
            if (IS_FULLBRIGHT(d[i]) && d[i] != C_CLEAR) d[i] = (d[i + 1 < TS * TS ? i + 1 : i] == C_CLEAR || ((i * 7919) % 5)) ? (uint8_t)C_CLEAR : (uint8_t)(R_WARM + 3);
        memcpy(crawl_spr[SP_TR_BRAZ0], crawl_spr[SP_BRAZIER_OUT], TS * TS); memcpy(crawl_spr[SP_TR_BRAZ1], crawl_spr[SP_BRAZIER_A], TS * TS);
    }
    if (wet) {
        static const int16_t STANDS[] = { SP_BONES1, SP_BONES1_2, SP_BONES1_3, SP_BONES2, SP_BONES2_2, SP_SKULLPILE, SP_SKULLPILE_2, SP_SKULLPILE_3,
                                          SP_TOTEM_A, SP_TOTEM_B, SP_TOTEM2_A, SP_TOTEM2_B };
        for (int i = 0; i < (int)(sizeof STANDS / sizeof STANDS[0]); i++) {
            int id = STANDS[i], first = id == SP_TOTEM_B ? SP_TOTEM_A : (id == SP_TOTEM2_B ? SP_TOTEM2_A : id);   /* a prop's frames wash alike */
            prop_wash(id, 0, 61u + (uint32_t)first * 7u);
        }
    }
}
/* which props have other looks: each base's looks, in order (crawl_alt) */
static void props_alts(void) {
    memset(crawl_alt, 0xFF, sizeof crawl_alt);
    static const int16_t ALTS[][NALT] = {
        { SP_BONES1, SP_BONES1_2, SP_BONES1_3, -1 }, { SP_BONES2, SP_BONES2_2, -1, -1 }, { SP_SKULLPILE, SP_SKULLPILE_2, SP_SKULLPILE_3, -1 },
        { SP_TOTEM_A, SP_TOTEM2_A, -1, -1 }, { SP_CANDLES, SP_CANDLES_2, SP_CANDLES_3, -1 },
        { SP_TOMB_ROUND, SP_TOMB_ROUND_2, -1, -1 }, { SP_TOMB_CROSS, SP_TOMB_CROSS_2, -1, -1 }, { SP_TOMB_OBELISK, SP_TOMB_OBELISK_2, -1, -1 },
        { SP_TOMB_BROKEN, SP_TOMB_BROKEN_2, -1, -1 }, { SP_CHAINS, SP_CHAINS_2, -1, -1 }, { SP_ROOTS, SP_ROOTS_2, -1, -1 },
        { SP_COBWEB, SP_COBWEB_2, -1, -1 }, { SP_WEBSHEET, SP_WEBSHEET_2, -1, -1 }, { SP_PILLAR, SP_PILLAR_2, -1, -1 },
        { SP_URN, SP_URN_2, SP_URN_3, -1 },
    };
    for (int i = 0; i < (int)(sizeof ALTS / sizeof ALTS[0]); i++) memcpy(crawl_alt[ALTS[i][0]], ALTS[i], sizeof ALTS[i]);
}

/* each sprite's foot (1.0.0b18.1): the lowest eighth of its drawn rows, and how wide it is there */
uint8_t crawl_foot[NSPR];
static void feet_measure(void) {
    for (int id = 0; id < NSPR; id++) {
        const uint8_t *s = crawl_spr[id];
        int top = TS, bot = -1, x0 = TS, x1 = -1;
        for (int x = 0; x < TS; x++)
            for (int y = 0; y < TS; y++) if (s[x * TS + y] != C_CLEAR) { if (y < top) top = y; if (y > bot) bot = y; }
        if (bot < 0) { crawl_foot[id] = 0; continue; }
        int y0 = bot - (bot - top + 8) / 8 + 1;
        for (int x = 0; x < TS; x++)
            for (int y = y0; y <= bot; y++) if (s[x * TS + y] != C_CLEAR) { if (x < x0) x0 = x; if (x > x1) x1 = x; }
        crawl_foot[id] = (uint8_t)(x1 - x0 + 1);
    }
}
int crawl_prop_foot(int spr, int nfr) {                            /* its widest frame or look */
    int w = 0;
    for (int k = 0; k < NALT; k++) {
        int look = k == 0 ? spr : crawl_alt[spr][k];
        if (look < 0) break;
        for (int f = 0; f < (nfr > 0 ? nfr : 1); f++) if (crawl_foot[look + f] > w) w = crawl_foot[look + f];
    }
    return w;
}
int crawl_foot_part(int w) { return w >= FOOT_WHOLE_W ? FOOT_WHOLE : (w >= FOOT_HALF_W ? FOOT_HALF : FOOT_QUARTER); }

void crawl_art_level(int level) {
    float dk = crawl_decay(level), depth = crawl_depth(level);
    g_decay = fmaxf(dk, depth);                                     /* the dead themselves only get older going down */
    level_palette(level);
    build_colormap();
    gen_stone(T_STONE, R_STONE, 11 + level * 7);
    gen_rubble(T_RUBBLE, 23 + level * 7);
    gen_skulls(T_SKULLS, 31);
    gen_brick(T_BRICK, 47 + level * 7);
    gen_moss(T_MOSS, 53 + level * 7);
    gen_niche(T_NICHE, 11);
    gen_banner(T_BANNER, 11);
    for (int f = 0; f < 3; f++) gen_torch(T_TORCH0 + f, f, 11);
    gen_jamb(T_JAMB, 79);
    gen_passage(T_PASSAGE, crawl_set(level) < 2);
    static const int DECAYS[] = { T_STONE, T_RUBBLE, T_SKULLS, T_BRICK, T_MOSS, T_NICHE, T_BANNER, T_TORCH0, T_TORCH1, T_TORCH2, T_JAMB };
    for (int i = 0; i < (int)(sizeof DECAYS / sizeof DECAYS[0]); i++)
        decay_tex(DECAYS[i], dk, (uint32_t)(DECAYS[i] >= T_TORCH0 && DECAYS[i] <= T_TORCH2 ? 900 : 900 + DECAYS[i] * 13 + level * 31));
    spr_skeleton(SP_SKEL_A, 1.0f, 0.0f, 3, 0);
    spr_skeleton(SP_SKEL_B, -1.0f, 0.0f, 3, 0);
    spr_skeleton(SP_SKEL_D1, 0.0f, 0.35f, 3, 0);
    spr_skeleton(SP_SKEL_D2, 0.0f, 0.7f, 3, 0);
    spr_skeleton(SP_SKEL_D3, 0.0f, 1.0f, 3, 0);
    spr_skeleton(SP_SKEL_T, 0.3f, 0.0f, 3, 1);
    spr_shieldskel(SP_SHIELD_A, 1.0f, 0, 1); spr_shieldskel(SP_SHIELD_B, -1.0f, 0, 1); spr_shieldskel(SP_SHIELD_S, 0.2f, 1, 1);
    spr_shieldskel(SP_SHIELD_XA, 1.0f, 0, 0); spr_shieldskel(SP_SHIELD_XB, -1.0f, 0, 0); spr_shieldskel(SP_SHIELD_XS, 0.2f, 1, 0);
    spr_shield_charge(SP_SHIELD_RAISE, 0); spr_shield_charge(SP_SHIELD_RUSH, 1);   /* C4: the charge, at the level's age */
    spr_bestiary_bones();
    spr_sides();
    props_level(level);                                             /* the props, at its age, washed in run II */
    treasure_plain();
    if (crawl_drawn(level)) crawl_art_run(level);                  /* the run's own look (docs/PLAN-b10.md) */
    else {                                                          /* not drawn yet: the look as it was */
        static const float KS[4] = { 1.0f, 0.5f, 0.12f, -0.5f };
        for (int f = 0; f < 4; f++) { spr_key(SP_KEY0 + f, KS[f], R_GOLD); spr_key(SP_KEYS0 + f, KS[f], R_IRON); spr_key(SP_KEYC0 + f, KS[f], R_BLOOD); }
        specials_plain();
    }
    feet_measure();                                                 /* and how much of a cell each takes (1.0.0b18.1) */
}

