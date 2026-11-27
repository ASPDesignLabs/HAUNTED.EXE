/* A part of crawl_art.c (docs/PLAN-tidy.md): it is #included there in its place and never compiled on its own.
 * Small things that move (docs/PLAN-vermin.md): the skull hopper, the bile leech and the coin crab, drafts for the owner's sheet. Each
 * is drawn in the canvas's lower half, a tile at the side passages' half size: a skull bounding (settled, and stretched in its leap),
 * a leech (lying, and arched in its leap), a crab (still, a coin as it passes for one; and running, its legs out). */

static void spr_hopper(int id, int frame) {                        /* frame 0 settled, its jaw shut; 1 in the air, stretched, its jaw open */
    cv_begin(cv_sprite, TS, TS);
    float cy = frame ? 42.0f : 46.0f, s = frame ? 2.2f : 2.4f;
    cv_ellipse(32, cy, 6.2f * s, (frame ? 6.4f : 5.6f) * s, R_BONE, 0.12f, 0.95f);
    cv_ellipse(32, cy + 4.6f * s, 4.4f * s, 2.7f * s, R_BONE, 0.1f, 0.8f);
    for (int e = -1; e <= 1; e += 2) {                             /* the sockets, and the eyes lit in them */
        cv_flat(32 + e * 2.5f * s, cy + 0.6f * s, 1.9f * s, 2.0f * s, (uint8_t)(R_STONE + 1));
        cv_flat(32 + e * 2.5f * s, cy + 0.8f * s, 0.9f * s, 0.9f * s, (uint8_t)(R_FLAME + 9));
        cv_eye_mark((int)(32 + e * 2.5f * s), (int)(cy + 0.8f * s));
    }
    cv_flat(32, cy + 3.2f * s, 0.8f * s, 1.1f * s, (uint8_t)(R_STONE + 2));   /* the nose */
    float jaw = cy + (frame ? 7.6f : 6.4f) * s;                    /* the teeth, the jaw hanging open in the leap */
    for (int i = -2; i <= 2; i++) { cv_px((int)(32 + i * 1.4f * s), (int)(cy + 5.6f * s), sh(R_BONE, 0.9f)); cv_px((int)(32 + i * 1.4f * s), (int)jaw, sh(R_BONE, 0.85f)); }
    if (frame) for (int x = -4; x <= 4; x++) cv_px((int)(32 + x * s * 0.9f), (int)(jaw - 1), (uint8_t)(R_STONE + 1));
    for (int k = 0; k < 3; k++) cv_line(24 + k * 6, cy - 5.2f * s + (k & 1), 26 + k * 5, cy - 3.6f * s, sh(R_BONE, 0.35f));   /* cracks */
    cv_outline(0);
    cv_store(id);
}
static void spr_leech(int id, int frame) {                         /* frame 0 lying low, 1 arched in the leap; its round mouth to the right */
    cv_begin(cv_sprite, TS, TS);
    for (int i = 0; i < 9; i++) {                                  /* its rings, tail to mouth */
        float t = (float)i / 8.0f, x = 18.0f + t * 28.0f, y = frame ? 54.0f - 14.0f * sinf(t * PI_F) : 56.0f - 1.5f * sinf(t * 6.0f);
        float r = 4.0f + 3.0f * sinf(t * PI_F * 0.9f + 0.2f);
        cv_ellipse(x, y, r + 1.0f, r, R_MOSS, 0.04f, 0.4f);
        if (i & 1) cv_line(x, y - r + 1, x, y + r - 1, sh(R_MOSS, 0.06f));
    }
    float mx = 47.0f, my = frame ? 52.0f : 56.0f;
    cv_flat(mx, my, 3.4f, 3.4f, (uint8_t)(R_BLOOD + 6));
    cv_flat(mx, my, 1.8f, 1.8f, (uint8_t)(R_STONE + 1));
    for (int k = 0; k < 6; k++) { float a = (float)k * PI_F / 3.0f; cv_px((int)(mx + cosf(a) * 2.6f), (int)(my + sinf(a) * 2.6f), sh(R_BONE, 0.9f)); }   /* its ring of teeth */
    cv_flat(40, frame ? 44.0f : 51.0f, 1.2f, 1.2f, (uint8_t)(R_MOSS + 13)); cv_eye_mark(40, frame ? 44 : 51);   /* an eyespot, sallow */
    for (int k = 0; k < 4; k++) cv_px(22 + k * 6, (int)(frame ? 50.0f - 8.0f * sinf((float)k * 0.8f) : 52.0f), (uint8_t)(R_MOSS + 11));   /* the bile's sheen */
    cv_outline(0);
    cv_store(id);
}
static void spr_crab(int id, int pose) {                           /* pose 0: still, a coin; 1 and 2: running, its legs at one step and the next */
    cv_begin(cv_sprite, TS, TS);
    if (pose) for (int s = -1; s <= 1; s += 2) for (int l = 0; l < 3; l++) {   /* six legs from under the coin, bent, tipped dark */
        float ph = ((l + (pose == 2)) & 1) ? 3.0f : -1.0f, kx = 32 + s * (12 + l * 2), ky = 50 + l * 2 - ph * 0.6f, fx = 32 + s * (17 + l * 2.5f);
        cv_capsule(32 + s * 7, 52 + l, kx, ky, 1.6f, R_IRON, 0.1f, 0.5f);
        cv_capsule(kx, ky, fx, 61, 1.4f, R_IRON, 0.08f, 0.45f);
    }
    if (pose) for (int s = -1; s <= 1; s += 2) {                   /* its claws, small, up before it */
        cv_capsule(32 + s * 8, 50, 32 + s * 13, 44, 1.6f, R_IRON, 0.1f, 0.5f);
        cv_ellipse(32 + s * 14, 43, 2.4f, 1.8f, R_IRON, 0.15f, 0.6f);
    }
    float cy = pose ? 50.0f : 58.0f, ry = pose ? 7.0f : 3.2f;      /* the coin: its shell, a face stamped on it */
    for (int y = (int)(cy - ry - 1); y <= (int)(cy + ry + 1); y++)
        for (int x = 19; x <= 45; x++) {
            float nx = (x + 0.5f - 32.0f) / 12.5f, ny = (y + 0.5f - cy) / ry, d = nx * nx + ny * ny;
            if (d > 1.0f) continue;
            int k = d > 0.78f ? 4 + (nx < 0.0f) * 3 : 7 + (int)(3.0f * lit(nx, ny, sqrtf(fmaxf(0.0f, 1.0f - d))));
            if (d < 0.35f && ((x * 3 + y * 5) % 7 == 0)) k -= 3;   /* the worn head on it */
            cv_px(x, y, (uint8_t)(R_GOLD + clampi(k, 0, NGOLD - 1)));
        }
    if (pose) for (int s = -1; s <= 1; s += 2) {                   /* eyes on stalks over its rim */
        cv_line(32 + s * 3, cy - ry + 1, 32 + s * 4, cy - ry - 4, sh(R_IRON, 0.5f));
        cv_flat(32 + s * 4, cy - ry - 5, 1.3f, 1.3f, (uint8_t)(R_GLOW + 11)); cv_eye_mark(32 + s * 4, (int)(cy - ry - 5));
    }
    cv_outline(0);
    cv_store(id);
}
static void vermin_sprites(void) {
    spr_hopper(SP_HOP_A, 0); spr_hopper(SP_HOP_B, 1);
    spr_leech(SP_LEECH_A, 0); spr_leech(SP_LEECH_B, 1);
    spr_crab(SP_CRAB_COIN, 0); spr_crab(SP_CRAB_A, 1); spr_crab(SP_CRAB_B, 2);
}
