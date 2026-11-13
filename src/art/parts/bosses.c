/* A part of crawl_art.c (docs/PLAN-tidy.md): it is #included there in its place and never compiled on its own.
 * The Ill-Fated Knight and the Black Deacon. */

/* a crown of iron spikes on a band round a brow at (cx, y), hw wide either side; broken: a spike or two snapped */
static void iron_crown(float cx, float y, float hw, int broken) {
    for (int x = (int)(cx - hw); x <= (int)(cx + hw); x++) {
        float u = (x + 0.5f - cx) / hw, by = y + u * u * 1.5f;      /* the band, curving back round the head */
        cv_px(x, (int)by, sh(R_IRON, 0.62f - 0.3f * u)); cv_px(x, (int)by + 1, sh(R_IRON, 0.3f - 0.12f * u));
    }
    for (int k = 0; k < 7; k++) {
        float u = (k - 3) / 3.0f, x = cx + u * (hw - 1.0f), by = y + u * u * 1.5f;
        int h = (int)(4.5f - fabsf(u) * 2.0f) - (broken && (k == 1 || k == 5) ? 2 : 0);
        for (int j = 1; j <= h; j++) cv_px((int)x, (int)by - j, sh(R_IRON, 0.8f - 0.08f * j - 0.2f * (u > 0)));
        if (h > 0) cv_px((int)x, (int)by - h - 1, sh(R_IRON, 0.95f));   /* its point catching the light */
    }
}

/* the greatsword: a long blade with a fuller and a bright edge, a broad guard, a ruby in the pommel */
static void greatsword(float gx, float gy, float a) {
    float dx = cosf(a), dy = sinf(a), px = -dy, py = dx;
    cv_ellipse(gx - dx * 7.0f, gy - dy * 7.0f, 2.2f, 2.2f, R_IRON, 0.4f, 1.0f);   /* pommel, and its ruby */
    cv_px((int)(gx - dx * 7.0f), (int)(gy - dy * 7.0f), (uint8_t)(R_BLOOD + 13)); cv_px((int)(gx - dx * 7.0f - px), (int)(gy - dy * 7.0f - py), (uint8_t)(R_BLOOD + 10));
    cv_capsule(gx - dx * 5.0f, gy - dy * 5.0f, gx + dx * 3.0f, gy + dy * 3.0f, 1.3f, R_WOOD, 0.2f, 0.55f);   /* the grip, bound */
    for (int k = -2; k <= 2; k++) cv_px((int)(gx + dx * k * 1.6f), (int)(gy + dy * k * 1.6f), sh(R_WOOD, 0.75f));
    cv_capsule(gx + dx * 3.6f - px * 8.0f, gy + dy * 3.6f - py * 8.0f, gx + dx * 3.6f + px * 8.0f, gy + dy * 3.6f + py * 8.0f, 1.2f, R_IRON, 0.35f, 0.95f);
    cv_ellipse(gx + dx * 3.6f - px * 8.0f, gy + dy * 3.6f - py * 8.0f, 1.6f, 1.6f, R_IRON, 0.5f, 1.0f);   /* the guard's ends */
    cv_ellipse(gx + dx * 3.6f + px * 8.0f, gy + dy * 3.6f + py * 8.0f, 1.6f, 1.6f, R_IRON, 0.5f, 1.0f);
    for (int i = 5; i < 50; i++) {                                  /* the blade */
        float t = (float)i, wdt = i > 43 ? (50 - i) * 0.33f : 2.2f;
        for (float o = -wdt; o <= wdt; o += 0.5f) {
            float v = 0.55f + 0.4f * (o / (wdt + 0.01f)) * -0.5f + (fabsf(o) < 0.5f ? -0.18f : 0.0f) + (o > wdt - 0.6f ? 0.35f : 0.0f);
            cv_px((int)(gx + dx * t + px * o), (int)(gy + dy * t + py * o), sh(R_GHOST, v));
        }
    }
}

/* The Ill-Fated Knight's own (docs/InitialArtPass.md, part three; the owner chose a fallen paladin): the Order's
 * champion gone to bone in its plate. A tall crested great helm under a crown of iron spikes, a torn crimson plume
 * streaming from the crest; the Order's cross inlaid on the breastplate, its edges once gilt; chains wound round the
 * plate; a ragged crimson cloak behind and a long surcoat below, split and torn; a greatsword with a ruby in its pommel.
 * Shedding plate bares the skeleton, and the helm comes off last, the crown still on the skull and a light in its
 * sockets. pose 0-1 walking, 2 the wind-up, 3 the blow, 4 planted; state 0 full plate, 1 half, 2 the helm gone. */
static void spr_bknight(int id, float w, int pose, int state) {
    cv_begin(cv_sprite, TS, TS);
    float gx, gy, ga;
    if (pose == 2) { gx = 41; gy = 11; ga = -2.9f; }
    else if (pose == 3) { gx = 27; gy = 40; ga = 2.55f; }
    else if (pose == 4) { gx = 32; gy = 25; ga = 1.5708f; }
    else { gx = 37; gy = 34; ga = -1.05f; }
    cv_flat(32, 62, 18, 2.2f, R_WARM + 1);
    for (int y = 17; y < 61; y++) {                                  /* the cloak, behind it all, ragged */
        float t = (y - 17) / 43.0f, hw = 10.5f + 6.0f * t;
        for (int x = (int)(32 - hw); x <= (int)(32 + hw); x++) {
            float dx = x + 0.5f - 32.0f;
            float hem = 50.0f + 6.0f * h01((int)(dx + 40) / 2, 1, 601) - (state == 2 ? 8.0f : 0.0f);
            if (y > hem) continue;
            float v = 0.1f + 0.14f * (1.0f - fabsf(dx) / hw) + (sinf(dx * 0.9f + y * 0.05f) > 0.7f ? -0.06f : 0.0f);
            cv_px(x, y, sh(R_BLOOD, v));
        }
    }
    if (pose == 2) greatsword(gx, gy, ga);
    for (int s = -1; s <= 1; s += 2) {                               /* legs: bone, and plate unless hacked away */
        float k = s * w, hx = 32 + s * 4.0f, kx = 32 + s * 5.0f - k * 2.5f, fx = 32 + s * 5.5f - k * 4.0f;
        if (pose == 3 || pose == 4) { kx += s * 1.5f; fx += s * 3.0f; }
        cv_bone(hx, 41, kx, 51, 1.6f, 0.25f, 0.95f);
        cv_bone(kx, 51, fx, 60, 1.4f, 0.25f, 0.9f);
        if (state < 1 || (state < 2 && s > 0)) plate(hx, 42, kx, 50, 3.1f, 0.0f);
        if (state < 2) { plate(kx, 52, fx, 59, 2.7f, 0.0f); cv_ellipse(kx, 51, 2.8f, 2.4f, R_IRON, 0.4f, 1.0f); cv_px((int)kx, 50, (uint8_t)(R_GOLD + 6)); }   /* poleyn, gilt */
        if (state < 2) cv_ellipse(fx - s * 1.0f, 61.5f, 3.8f, 1.6f, R_IRON, 0.3f, 0.9f);
        else cv_capsule(fx, 61.5f, fx - s * 3.5f, 62, 1.0f, R_BONE, 0.3f, 0.8f);
    }
    for (int s = -1; s <= 1; s += 2) {                               /* the surcoat below the plate, split before, torn */
        for (int y = 40; y < 59; y++) {
            float x0 = 32 + s * 1.0f, x1 = 32 + s * (9.5f + (y - 40) * 0.12f);
            float hem = 57.0f - 5.0f * h01((int)(y > 0) + (int)fabsf(x1), 3, 603);
            if (state == 1 && s < 0) hem = 46.0f;                    /* hacked short on the bared side */
            if (state == 2) hem = 44.0f + 3.0f * (s > 0);
            for (int x = (int)fminf(x0, x1); x <= (int)fmaxf(x0, x1); x++) {
                float h = hem + 3.0f * h01(x, 7, 605);
                if (y > h) continue;
                float u = fabsf(x + 0.5f - 32.0f) / 10.0f, fold = sinf((x - 32) * 1.1f + s) * 0.06f;
                cv_px(x, y, sh(R_BLOOD, 0.24f + fold - u * 0.06f - (y - 40) * 0.004f));
            }
        }
    }
    cv_capsule(32, 18, 32, 40, 1.4f, R_BONE, 0.3f, 0.9f);           /* spine and ribs, where the plate is gone */
    for (int i = 0; i < 4; i++) {
        float y = 21 + i * 3.6f;
        cv_capsule(31, y, 25 + i * 0.6f, y + 2, 0.9f, R_BONE, 0.25f, 0.9f); cv_capsule(33, y, 39 - i * 0.6f, y + 2, 0.9f, R_BONE, 0.25f, 0.9f);
    }
    if (state < 2)                                                   /* the fauld, its lowest band gilt */
        for (int b = 0; b < 3; b++) {
            float y = 37 + b * 2.6f, hw = 9.5f + b * 0.8f;
            for (int x = (int)(32 - hw); x <= (int)(32 + hw); x++) {
                if (state == 1 && x < 28 && b > 0) continue;
                float nx = (x + 0.5f - 32) / hw;
                for (int yy = 0; yy < 3; yy++) cv_px(x, (int)y + yy, yy == 2 && b == 2 ? (uint8_t)(R_GOLD + 4 + (int)(3.0f * (1.0f - fabsf(nx)))) : sh(R_IRON, 0.3f + 0.5f * lit(nx, -0.2f, sqrtf(fmaxf(0, 1 - nx * nx))) - yy * 0.08f));
            }
        }
    for (int y = 18; y <= 37; y++) {                                 /* the breastplate: a ridge, the Order's cross, gilt edges */
        float hw = 9.5f + 1.5f * sinf((y - 18) / 19.0f * PI_F) - (y > 31 ? (y - 31) * 0.6f : 0.0f);
        for (int x = (int)(32 - hw); x <= (int)(32 + hw); x++) {
            float nx = (x + 0.5f - 32) / hw;
            if (state == 2 && (x < 30 - (y - 18) * 0.15f || y > 32)) continue;
            float v = 0.25f + 0.62f * lit(nx, (y - 26) / 14.0f, sqrtf(fmaxf(0.0f, 1.0f - nx * nx)));
            if (fabsf(x + 0.5f - 32) < 0.8f) v += 0.25f;
            if (state >= 1 && h01(x / 2, y / 2, 607) > 0.86f) v -= 0.3f;
            uint8_t c = sh(R_IRON, v);
            if (fabsf(fabsf(x + 0.5f - 32) - hw) < 1.0f) c = (uint8_t)(R_GOLD + 3 + (int)(4.0f * (1.0f - fabsf(nx)) + 2.0f * (y < 26)));   /* the gilt, worn */
            int cross = (abs(x - 32) <= 1 && y >= 20 && y <= 34) || (y >= 24 && y <= 25 && abs(x - 32) <= 5);
            if (cross && !(state == 2 && x < 31)) c = abs(x - 32) == 0 || y == 24 ? sh(R_BONE, 0.85f) : sh(R_BONE, 0.6f);
            cv_px(x, y, c);
        }
    }
    if (state == 2) { cv_line(30, 19, 33, 33, sh(R_IRON, 0.05f)); cv_line(34, 22, 37, 30, sh(R_IRON, 0.05f)); }   /* cracked through */
    if (state < 2)                                                   /* chains wound about it, their shadow on the plate */
        for (int c = 0; c < 2; c++) {
            float y0 = c ? 25.0f : 21.0f, y1 = c ? 37.0f : 35.0f, sg = c ? 1.5f : 2.0f;
            for (int i = 0; i <= 22; i++) { float t = i / 22.0f; cv_px((int)(24.5f + c + 15.0f * t - c), (int)(y0 + (y1 - y0) * t + sg * 4.0f * t * (1.0f - t)) + 1, sh(R_IRON, 0.02f)); }
            kit_chain(24.0f + c, y0, 40.0f - c, y1, sg, R_IRON);
        }
    for (int s = -1; s <= 1; s += 2) {                               /* arms: both hands on the grip */
        float sx = 32 + s * 9.5f, sy = 20;
        float ex = (sx + gx) * 0.5f + s * 4.0f, ey = (sy + gy) * 0.5f + 4.0f;
        if (pose == 2) { ex = (sx + gx) * 0.5f + s * 3.0f; ey = (sy + gy) * 0.5f + 3.0f; }
        cv_bone(sx, sy, ex, ey, 1.3f, 0.25f, 0.9f);
        cv_bone(ex, ey, gx + s * 1.0f, gy, 1.1f, 0.25f, 0.9f);
        int keep = state == 0 || s > 0;
        if (keep) {                                                  /* a great pauldron, its rim gilt, a spike */
            cv_ellipse(sx, sy - 0.5f, 5.6f, 4.4f, R_IRON, 0.3f, 1.0f);
            for (int a = 0; a < 14; a++) { float an = PI_F + a * PI_F / 13.0f; cv_px((int)(sx + cosf(an) * 5.4f), (int)(sy - 0.5f + sinf(an) * 4.2f), (uint8_t)(R_GOLD + 5 + (a & 1))); }
            cv_line(sx + s * 2.0f, sy - 4.0f, sx + s * 4.0f, sy - 8.0f, sh(R_IRON, 0.8f));
        }
        if (state < 2 && (state == 0 || s > 0)) plate(ex, ey, gx + s * 1.0f, gy, 2.1f, 0.05f);   /* vambrace */
        if (s > 0 && state < 2) kit_chain(ex - 1.0f, ey - 2.0f, ex + 3.0f, ey + 3.0f, 0.5f, R_IRON);   /* a chain about the forearm */
    }
    if (pose != 2) greatsword(gx, gy, ga);
    for (int s = -1; s <= 1; s += 2) cv_ellipse(gx + s * 1.2f, gy + 0.5f, 2.3f, 2.1f, state < 2 ? R_IRON : R_BONE, 0.3f, 0.9f);   /* gauntlets, or bone */
    cv_capsule(32, 15, 32, 18, 1.2f, R_BONE, 0.3f, 0.9f);
    if (state < 2) {                                                 /* the plume, torn, streaming from the crest; the helm; its crown */
        for (int k = 0; k < 30; k++) {
            float t = k / 29.0f, px = 33.0f + t * 20.0f, py = 2.0f + t * 12.0f + sinf(t * 7.0f) * 2.0f, wdt = 2.8f - t * 1.8f;
            if (k > 18 && h01(k, 1, 609) < 0.4f) continue;           /* torn at its end */
            for (float o = -wdt; o <= wdt; o += 0.5f) cv_px((int)px, (int)(py + o), sh(R_BLOOD, 0.35f + 0.25f * (1.0f - fabsf(o) / (wdt + 0.01f)) - t * 0.15f));
        }
        grid_stamp(KNIGHT_HELM, 15, 16, 25, 2, R_IRON, R_BLOOD, (uint8_t)(R_FLAME + 9), 0);
        iron_crown(32, 6, 7.5f, state == 1);
    } else {                                                         /* the helm gone: the crowned skull, a light in it */
        skull_stamp(32, 10, 0, 0, (uint8_t)(R_FLAME + 12));
        iron_crown(32, 4, 7.0f, 1);
    }
    cv_bone_decay(611);
    cv_outline(0);
    cv_store(id);
}

/* ---- The Black Deacon's own (docs/InitialArtPass.md, part three; the owner chose a profane bishop) ---- */

/* a bone hand at (hx, hy), its fingers closed round something going up and down (grip) or reaching (a: the way) */
static void bishop_hand(float hx, float hy, int s) {
    cv_ellipse(hx, hy, 1.9f, 1.7f, R_BONE, 0.25f, 0.85f);
    for (int f = 0; f < 3; f++) cv_px((int)(hx - (float)s * 1.5f), (int)(hy - 1.0f + (float)f), sh(R_BONE, 0.7f - 0.1f * (float)f));   /* knuckles round the grip */
}

/* the mitre over a brow at (cx, by): tall and pointed, black, a crimson band up its front and another round its foot,
 * worn gilt at its edges, an inverted cross on its front; tilt leans it (the fallen one) */
static void bishop_mitre(float cx, float by, float h, float hw) {
    for (int y = (int)(by - h); y <= (int)by + 2; y++)
        for (int x = (int)(cx - hw) - 1; x <= (int)(cx + hw) + 1; x++) {
            float t = clampf(((float)y - (by - h)) / h, 0.0f, 1.0f), w = hw * powf(t, 0.65f) + 0.4f;
            float dx = (float)x + 0.5f - cx;
            if (fabsf(dx) > w) continue;
            float u = dx / (w + 0.01f), v = 0.1f + 0.22f * (1.0f - u * u) * (0.6f + 0.4f * (1.0f - t)) - (u > 0.3f ? 0.05f : 0.0f);
            uint8_t c = sh(R_IRON, v);
            if (fabsf(dx) < 1.6f && t > 0.12f) c = sh(R_BLOOD, 0.22f + 0.12f * (dx < 0));   /* the band up its front */
            if (y >= (int)by) c = sh(R_BLOOD, 0.2f + 0.12f * (1.0f - fabsf(u)) + (y == (int)by ? 0.08f : 0.0f));   /* the band round its foot */
            if (fabsf(u) > 0.84f && ((x + y) & 1) == 0) c = (uint8_t)(R_GOLD + 3 + (int)(3.0f * (1.0f - t) * (dx < 0)));   /* the gilt, worn */
            if (y == (int)by - 1 || y == (int)by + 3) { if ((x & 1) == 0) c = (uint8_t)(R_GOLD + 4); }
            cv_px(x, y, c);
        }
    int cy = (int)(by - h * 0.62f);                                  /* the cross on its front, upside down */
    for (int k = -3; k <= 2; k++) cv_px((int)cx, cy + k, (uint8_t)(R_GOLD + 7 + (k < 0)));
    for (int k = -2; k <= 2; k++) cv_px((int)cx + k, cy + 1, (uint8_t)(R_GOLD + 6 + (k == 0) * 3));
}

/* his crozier: a staff of black iron banded in gilt from (x0, y0) up to (x1, y1), crowned with a skull, its crook
 * arched over it, chains hung from the crook's end */
static void bishop_crozier(float x0, float y0, float x1, float y1, float sway) {
    cv_capsule(x0, y0, x1, y1, 1.1f, R_IRON, 0.12f, 0.55f);
    float L = hypotf(x1 - x0, y1 - y0);
    for (int b = 1; b <= 4; b++) {                                   /* gilt bands */
        float t = (float)b / 5.0f, x = x0 + (x1 - x0) * t, y = y0 + (y1 - y0) * t;
        cv_px((int)x - 1, (int)y, (uint8_t)(R_GOLD + 5)); cv_px((int)x, (int)y, (uint8_t)(R_GOLD + 8)); cv_px((int)x + 1, (int)y, (uint8_t)(R_GOLD + 4));
    }
    (void)L;
    float sx = x1, sy = y1 - 4.0f;                                   /* the skull on its head */
    for (int k = 0; k <= 40; k++) {                                  /* the crook: a thick volute, up from behind the skull, over it, curling in */
        float t = (float)k / 40.0f, a = PI_F * 0.95f + t * PI_F * 1.75f, r = 6.6f - 3.4f * t * t;
        float x = sx + 1.0f + cosf(a) * r, y = sy - 0.5f + sinf(a) * r;
        float lv = 0.35f + 0.35f * (-cosf(a) * 0.5f - sinf(a) * 0.6f);
        cv_px((int)x, (int)y, sh(R_IRON, lv)); cv_px((int)(x - cosf(a)), (int)(y - sinf(a)), sh(R_IRON, lv * 0.6f));
        if (k % 8 == 4) cv_px((int)(x + cosf(a) * 0.8f), (int)(y + sinf(a) * 0.8f), (uint8_t)(R_GOLD + 6));   /* crockets of gilt along it */
    }
    grid_stamp(SKULL_SMALL, 9, 10, (int)sx - 4, (int)sy - 4, R_BONE, R_BONE, 0, 0);
    cv_px((int)sx - 2, (int)sy - 1, (uint8_t)(R_MOSS + 9)); cv_px((int)sx + 2, (int)sy - 1, (uint8_t)(R_MOSS + 8));   /* a green spark in its sockets */
    for (int c = 0; c < 2; c++) {                                    /* chains hung from the crook, links light and dark */
        float cx0 = c ? sx - 5.5f : sx + 5.0f, cy0 = c ? sy + 2.0f : sy + 3.0f;
        for (int k = 0; k < 9 - 2 * c; k++) cv_px((int)(cx0 + sway * 0.12f * (float)k), (int)cy0 + k, sh(R_IRON, (k & 1) ? 0.62f : 0.25f));
    }
}

/* his censer on its chains from a hand at (hx, hy), the bowl at (cx, cy): brass gone dark, its pierced holes glowing green,
 * the smoke going up from it in strands (more: how much) */
static void bishop_censer(float hx, float hy, float cx, float cy, float more) {   /* more: unused, the smoke is bishop_smoke's */
    for (int c = -1; c <= 1; c++) cv_line(hx, hy, cx + (float)c * 2.2f, cy - 3.0f, sh(R_IRON, 0.45f + 0.15f * (float)c));
    cv_ellipse(cx, cy - 3.5f, 2.0f, 1.4f, R_GOLD, 0.2f, 0.6f);         /* its lid */
    cv_px((int)cx, (int)cy - 5, (uint8_t)(R_GOLD + 6));
    cv_ellipse(cx, cy, 3.4f, 3.0f, R_GOLD, 0.08f, 0.5f);              /* the bowl */
    for (int k = 0; k < 5; k++) cv_px((int)(cx - 2.0f + (float)k), (int)cy - (k & 1), (uint8_t)(R_MOSS + 13 + (k & 1) * 2));   /* holes, green inside */
    cv_px((int)cx, (int)cy + 3, (uint8_t)(R_GOLD + 3));
    (void)more;
}

/* the censer's smoke going up from a bowl at (cx, cy) in thin puffs, drifting off as it climbs and thinning (more: how
 * much). Drawn after the outline, so that it has none */
static void bishop_smoke(float cx, float cy, float more) {
    int n = (int)(18.0f * more);
    for (int s = 0; s < 2; s++)
        for (int k = 0; k < n; k++) {
            float t = (float)k / 18.0f, x = cx + 1.0f + sinf(t * 4.5f + (float)s * 2.6f) * (0.8f + 2.5f * t) + (5.0f + 3.0f * (float)s) * t * t * 1.4f;
            float y = cy - 5.0f - (float)k * 1.5f, r = 0.9f + 1.8f * t;
            for (int j = (int)(-r); j <= (int)r; j++)
                for (int i = (int)(-r); i <= (int)r; i++) {
                    int px = (int)x + i, py = (int)y + j;
                    if ((float)(i * i + j * j) > r * r || h01(px, py, 601 + (uint32_t)s) > 0.6f - 0.45f * t) continue;
                    cv_px(px, py, sh(R_MOSS, 0.55f - 0.3f * t + (j < 0 ? 0.06f : 0.0f)));
                }
        }
}

/* The Black Deacon's own (docs/InitialArtPass.md, part three; the owner chose a profane bishop): the Order's bishop gone
 * to bone and kept on saying the rites. Towering in black vestments with a crimson orphrey and an inverted cross, a tall
 * mitre, crimson lappets; a skull face lit green from below; a crozier crowned with a skull and hung with chains in one
 * hand, a censer swinging green smoke from the other. His bone ward is drawn by the game, wheeling about him. pose 0-1
 * walking (the censer swinging), 2 chanting (the crozier and censer raised, the smoke billowing), 3 fallen. */
static void spr_bdeacon(int id, float w, int pose) {
    cv_begin(cv_sprite, TS, TS);
    if (pose == 3) {                                                 /* fallen: the vestments in a heap, the mitre rolled off */
        cv_flat(32, 61.5f, 26, 2.5f, R_WARM + 1);
        float m[12] = { 6, 62, 12, 51, 26, 46, 42, 48, 54, 55, 58, 62 };
        cv_poly(m, 6, R_IRON, 0.3f, 0.05f, 46, 16);
        for (int y = 47; y < 63; y++) for (int k = -2; k <= 2; k++) cv_px(30 + (y - 47) / 3 + k, y, sh(R_BLOOD, 0.25f - 0.03f * (float)abs(k)));   /* the orphrey, twisted */
        bishop_crozier(4, 61, 50, 49, 1.0f);
        skull_stamp(24, 50, 1, 1, 0);                                /* the skull on its side, its light gone */
        bishop_mitre(49, 58, 9, 5.5f);
        float cx = 14, cy = 58;                                      /* the censer spilled, its coals still green */
        cv_ellipse(cx, cy, 3.2f, 2.4f, R_GOLD, 0.08f, 0.45f);
        for (int k = 0; k < 6; k++) cv_px((int)(cx + 3 + (float)k * 1.3f), (int)cy + 1 + (k & 1), (uint8_t)(R_MOSS + 10 + (k % 3) * 2));
        cv_bone_decay(589);
        cv_outline(0);
        cv_store(id);
        return;
    }
    int chant = pose == 2;
    cv_flat(32, 62, 17, 1.8f, R_WARM + 1);
    for (int s = -1; s <= 1; s += 2)                                 /* the lappets, hanging behind from the mitre */
        for (int y = 11; y < 30; y++) {
            float x = 32 + (float)s * (5.5f + (float)(y - 11) * 0.12f) + w * 0.3f * (float)(y - 11) / 19.0f;
            cv_px((int)x, y, sh(R_BLOOD, 0.18f + 0.08f * (s < 0))); cv_px((int)x + s, y, sh(R_BLOOD, 0.12f));
            if (y >= 27) { cv_px((int)x, y + 1, (uint8_t)(R_GOLD + 4)); cv_px((int)x + s, y + 2, (uint8_t)(R_GOLD + 3)); }   /* gilt fringe */
        }
    for (int y = 20; y < 63; y++) {                                  /* the chasuble: black, belled, its folds falling straight */
        float t = (float)(y - 20) / 42.0f, hw = 9.5f + 8.0f * t, sway = w * 1.4f * t;
        for (int x = (int)(32 - hw + sway) - 1; x <= (int)(32 + hw + sway) + 1; x++) {
            float dx = (float)x + 0.5f - 32.0f - sway;
            if (fabsf(dx) > hw) continue;
            if (y > 59 && h01(x, y, 591 + (w > 0)) < (float)(y - 59) / 4.0f) continue;   /* the hem dragging, frayed */
            float u = dx / hw, v = 0.05f + 0.26f * (1.0f - u * u) * (1.0f - 0.25f * t) + (u < -0.3f ? 0.04f : 0.0f);
            if (sinf(dx * 1.5f + (float)y * 0.03f) > 0.6f) v -= 0.07f;
            uint8_t c = sh(R_IRON, v);
            if (fabsf(u) > 0.93f) c = sh(R_BLOOD, 0.22f);           /* its crimson lining, turned at the edge */
            if (y >= 59 && ((x + y) & 1) == 0 && y <= 60) c = (uint8_t)(R_GOLD + 3);   /* the hem's gilt, worn */
            cv_px(x, y, c);
        }
    }
    for (int y = 21; y < 62; y++) {                                  /* the orphrey: a crimson Y from the shoulders, gilt at its edges */
        float sway = w * 1.4f * (float)(y - 20) / 42.0f;
        if (y < 33) {
            for (int s = -1; s <= 1; s += 2) {
                float cx = 32.0f + (float)s * (8.0f - (float)(y - 21) * 0.62f) + sway;
                for (int k = -1; k <= 1; k++) cv_px((int)cx + k, y, sh(R_BLOOD, 0.3f + 0.08f * (s < 0) - 0.04f * (float)abs(k)));
                cv_px((int)cx + 2 * s, y, (uint8_t)(R_GOLD + 4)); 
            }
        } else {
            for (int k = -2; k <= 2; k++) cv_px((int)(32.0f + sway) + k, y, abs(k) == 2 ? (((y & 1) == 0) ? (uint8_t)(R_GOLD + 4) : sh(R_BLOOD, 0.15f)) : sh(R_BLOOD, 0.3f + 0.06f * (k < 0)));
        }
    }
    {                                                                /* the inverted cross on it, in bone */
        int cx = (int)(32.0f + w * 1.4f * 0.45f);
        for (int y = 37; y <= 47; y++) cv_px(cx, y, sh(R_BONE, 0.8f - 0.02f * (float)(y - 37)));
        for (int k = -2; k <= 2; k++) cv_px(cx + k, 44, sh(R_BONE, 0.75f - 0.08f * (float)abs(k)));
    }
    for (int x = 25; x <= 39; x++)                                   /* the amice: a high dark collar, crimson in its fold */
        for (int y = 20; y <= 23; y++) {
            float u = ((float)x + 0.5f - 32.0f) / 7.5f;
            if (u * u + ((float)y - 23.5f) * ((float)y - 23.5f) / 12.0f > 1.0f) continue;
            cv_px(x, y, y == 20 ? sh(R_BLOOD, 0.2f) : sh(R_IRON, 0.1f + 0.1f * (1.0f - fabsf(u))));
        }
    /* the crozier in his right hand (to our left), the censer in his left */
    float chx = chant ? 15.0f : 16.0f, chy = chant ? 22.0f : 35.0f;
    float sx = chant ? 12.0f : 13.5f, sy0 = chant ? 50.0f : 61.0f, sy1 = chant ? 4.0f : 12.0f;
    bishop_crozier(sx, sy0, sx + (chant ? 1.0f : 0.0f), sy1, w);
    float hx = chant ? 51.0f : 46.0f, hy = chant ? 10.0f : 31.0f;
    float bx = chant ? 55.0f : 46.0f + 4.0f * w, by = chant ? 24.0f : 46.0f;
    for (int s = -1; s <= 1; s += 2) {                               /* sleeves, belled, black lined with crimson */
        float shx = 32.0f + (float)s * 9.0f, shy = 23.0f, tx = s < 0 ? chx : hx, ty = s < 0 ? chy : hy;
        float ex = (shx + tx) * 0.5f + (float)s * 2.5f, ey = (shy + ty) * 0.5f + (chant ? 2.0f : 3.0f);
        if (chant) { cv_bone(ex, ey, tx, ty + 1.5f, 1.0f, 0.25f, 0.85f); }   /* raised: the sleeve fallen back from the bone */
        cv_capsule(shx, shy, ex, ey, 3.4f, R_IRON, 0.06f, 0.36f);
        if (!chant) {
            cv_capsule(ex, ey, tx + (float)s * 0.5f, ty - 2.0f, 3.2f, R_IRON, 0.06f, 0.34f);
            cv_ellipse(tx + (float)s * 0.6f, ty - 1.5f, 3.6f, 2.0f, R_BLOOD, 0.12f, 0.3f);   /* the cuff's lining */
        } else cv_ellipse(ex, ey + 1.0f, 3.8f, 2.4f, R_BLOOD, 0.12f, 0.3f);
        bishop_hand(tx, ty, s);
    }
    if (chant) cv_capsule(sx - 1.0f, chy - 3.0f, sx + 2.0f, chy + 3.0f, 0.9f, R_IRON, 0.12f, 0.55f);   /* the staff again, in front of the hand */
    bishop_censer(hx, hy + 1.0f, bx, by, 0.0f);
    bishop_mitre(32, 10, 11, 7.5f);
    skull_stamp(32, 17, 0, 0, (uint8_t)(chant ? R_GLOW + 14 : C_WHITE));   /* his face, under it */
    static uint8_t face[TS * TS];                                   /* lit green from below: the lower planes catch it, the brow and dome stay dark */
    memcpy(face, cv_sprite, sizeof face);
    for (int y = 11; y < 25; y++)
        for (int x = 24; x < 41; x++) {
            uint8_t c = face[y * TS + x];
            if (c <= R_BONE || c >= R_BONE + 16) continue;
            float f = clampf(((float)y - 13.0f) / 10.0f, 0.0f, 1.0f), b = (float)(c - R_BONE) / 15.0f;
            int below = y + 1 < TS && (face[(y + 1) * TS + x] == 0 || face[(y + 1) * TS + x] == C_CLEAR);   /* a lower edge, over a hollow: the light's catch */
            float g = 0.06f + 0.8f * f * b * b + (below ? 0.3f : 0.0f) + (chant ? 0.08f : 0.0f);
            cv_px(x, y, f > 0.25f || below ? sh(R_MOSS, g) : sh(R_BONE, 0.12f + b * 0.3f));
        }
    if (chant)                                                       /* the green rising round him */
        for (int i = 0; i < 18; i++) cv_px((int)(32.0f + (h01(i, 1, 595) - 0.5f) * 40.0f), (int)(28.0f + 30.0f * h01(i, 2, 595)), (uint8_t)(R_MOSS + 10 + (i % 5)));
    cv_bone_decay(597);
    cv_outline(0);
    bishop_smoke(bx, by, chant ? 1.4f : 0.85f);
    cv_store(id);
}

