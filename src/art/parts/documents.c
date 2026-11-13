/* A part of crawl_art.c (docs/PLAN-tidy.md): it is #included there in its place and never compiled on its own.
 * A document's page and its ink; the bosses' hazards (rock, rubble, the eruption, the void, the hands); the scrip; the Hexed Totem. */

/* ---- a document's page (1.0.0b13): a leaf of the Order's book of hours, a letter come in a bottle, a foreman's
 * scrap of sacking, a strip of beaten gold leaf ---- */
static uint8_t doc_shade(int run, float v) {
    if (run == 3) return sh(R_GOLD, v);
    return sh(run == 2 ? R_WOOD : R_BONE, v);
}
int crawl_doc_paper(int run, unsigned char c) {
    if (run == 3) return c >= R_GOLD && c < R_GOLD + NGOLD;
    if (run == 0 && c >= R_BLOOD && c < R_BLOOD + 16) return 1;   /* the vellum's red rules are page too */
    int base = run == 2 ? R_WOOD : R_BONE;
    return c >= base && c < base + 16;
}
unsigned char crawl_doc_page(unsigned char *pix, int stride, int x0, int y0, int w, int h, int run, unsigned seed) {
    int plain = run & 4;                                           /* (a slip: no square for a first letter) */
    run &= 3;
    for (int y = 0; y < h; y++)
        for (int x = 0; x < w; x++) {
            float e = 99.0f;                                       /* how far in from a ragged edge */
            float rag = run == 0 ? 2.5f : run == 1 ? 1.0f : run == 2 ? 2.0f : 1.6f;
            float dl = x - rag * (run == 0 ? 2.0f * fbm(y / 3.0f, 1.0f, 64, seed) + 1.0f : fbm(y / 4.0f, 2.0f, 64, seed));   /* torn from its binding */
            float dr = (w - 1 - x) - rag * fbm(y / 4.0f, 3.0f, 64, seed + 1);
            float dt = y - rag * fbm(x / 4.0f, 4.0f, 64, seed + 2), db = (h - 1 - y) - rag * fbm(x / 4.0f, 5.0f, 64, seed + 3);
            e = fminf(fminf(dl, dr), fminf(dt, db));
            if (e < 0.0f) continue;
            if (run == 2 && e < 3.0f && h01(x, y, seed + 9) < 0.35f) continue;   /* frayed */
            if (run == 3 && fbm(x / 16.0f, y / 12.0f, 64, seed + 5) > 0.7f) continue;   /* beaten through */
            float v;
            uint8_t c;
            if (run == 0) {
                v = 0.66f + 0.12f * (fbm(x / 13.0f, y / 11.0f, 64, seed + 6) - 0.5f) + 0.03f * (h01(x, y, seed) - 0.5f);
                if (e < 2.0f) v -= 0.18f;
                c = doc_shade(0, v);
                int ix = x - 8, iy = y - 6, rx = w - 7 - x, ry = h - 7 - y;
                if ((ix == 0 || ix == 2 || rx == 0 || rx == 2 || iy == 0 || iy == 2 || ry == 0 || ry == 2) && ix >= 0 && iy >= 0 && rx >= 0 && ry >= 0) c = sh(R_BLOOD, 0.5f);   /* ruled in red */
                if (x < 4 && (y % 22) == 11) c = sh(R_WOOD, 0.1f);  /* its sewing */
            } else if (run == 1) {
                v = 0.86f + 0.06f * (fbm(x / 17.0f, y / 13.0f, 64, seed + 6) - 0.5f);
                float tide = (0.14f + 0.12f * fbm(x / 40.0f, 9.0f, 64, seed + 7)) * (float)h, wet = (float)(h - y) - tide;   /* where the water came up to */
                if (wet < 0.0f) v -= 0.08f + 0.06f * fbm(x / 9.0f, y / 7.0f, 64, seed + 10);
                else if (wet < 1.2f) v -= 0.2f;
                else if (wet > 6.0f && wet < 7.0f && h01(x, 0, seed + 11) < 0.6f) v -= 0.08f;   /* and once higher, fainter */
                if (x == w / 2 || y == h / 3) v += 0.05f; else if (x == w / 2 + 1 || y == h / 3 + 1) v -= 0.07f;   /* its folds */
                c = doc_shade(1, v);
                float sx = (float)(x - (w - 22)), sy = (float)(y - (h - 20));
                float sd = hypotf(sx, sy) + 1.4f * (fbm(atan2f(sy, sx) * 1.5f + 3.0f, 1.0f, 64, seed + 8) - 0.5f);
                if (sd < 9.0f) c = sh(R_BLOOD, 0.35f + 0.25f * (sd < 6.5f && sd > 5.0f) + 0.15f * (sx < -1 && sy < -1));   /* the seal */
            } else if (run == 2) {
                int warp = (x & 1) ^ (y & 1), thread = (y % 3 == 0) || (x % 3 == 0);
                v = 0.52f + 0.1f * warp + 0.06f * thread + 0.1f * (fbm(x / 9.0f, y / 9.0f, 64, seed + 6) - 0.5f);
                if (hypotf((float)(x - w * 0.7f), (float)(y - h * 0.25f) * 1.3f) < 11.0f + 3.0f * fbm(x / 5.0f, y / 5.0f, 64, seed + 7)) v -= 0.12f;   /* grease */
                c = doc_shade(2, v);
            } else {
                float r = fbm(x / 5.0f, y / 3.0f, 64, seed + 6), ridge = 1.0f - fabsf(r - 0.5f) * 2.0f;   /* crinkled */
                v = 0.55f + 0.3f * ridge + 0.08f * (h01(x, y, seed) - 0.5f);
                if (e < 1.5f) v -= 0.2f;
                c = doc_shade(3, v);
            }
            pix[(y0 + y) * stride + x0 + x] = c;
        }
    if (run == 0 && !plain)                                        /* a gilt square for the first letter */
        for (int y = 10; y < 22 && y < h; y++) for (int x = 12; x < 24 && x < w; x++) pix[(y0 + y) * stride + x0 + x] = sh(R_GOLD, 0.55f + 0.25f * ((x + y) % 5 == 0));
    return run == 0 ? sh(R_WOOD, 0.03f) : run == 1 ? sh(R_IRON, 0.12f) : run == 2 ? sh(R_STONE, 0.08f) : sh(R_GOLD, 0.2f);
}
void crawl_doc_ink(unsigned char *pix, int stride, int x0, int y0, int w, int h, int run, unsigned char ink, unsigned seed) {
    if (run != 1 && run != 3) return;
    for (int y = h - 2; y >= 0; y--)
        for (int x = w - 2; x >= 0; x--) {
            uint8_t *p = &pix[(y0 + y) * stride + x0 + x];
            if (*p != ink) continue;
            if (run == 1) {                                        /* run a little, downward, where it was wet */
                uint8_t *q = p + stride;
                if (*q != ink && crawl_doc_paper(1, *q) && h01(x, y, seed) < 0.3f) *q = sh(R_IRON, 0.4f);
            } else {                                               /* scratched: a bright edge below and to the right */
                uint8_t *q = p + stride + 1;
                if (*q != ink && crawl_doc_paper(3, *q)) *q = sh(R_GOLD, 1.0f);
            }
        }
}

/* a rock, broken from the vault: drawn at the bottom of its square, so it can be dropped from above */
static void spr_rock(void) {
    cv_begin(cv_sprite, TS, TS);
    for (int y = 44; y < 64; y++)
        for (int x = 20; x < 44; x++) {
            float dx = (x + 0.5f - 32.0f) / 10.5f, dy = (y + 0.5f - 54.0f) / 8.5f, d = dx * dx + dy * dy;
            d += (fbm(x / 3.0f, y / 3.0f, 16, 77) - 0.5f) * 0.6f;
            if (d > 1.0f) continue;
            cv_px(x, y, sh(R_STONE, 0.25f + 0.45f * lit(dx, dy, sqrtf(fmaxf(0.0f, 1.0f - d))) + 0.1f * h01(x, y, 5)));
        }
    cv_outline(0);
    cv_store(SP_ROCK);
}
static void spr_rubble(void) {
    cv_begin(cv_sprite, TS, TS);
    cv_flat(32, 62, 13.0f, 1.6f, sh(R_STONE, 0.05f));
    for (int k = 0; k < 6; k++) {
        float cx = 22.0f + 20.0f * h01(k, 1, 31), cy = 59.0f + 3.0f * h01(k, 2, 31), r = 2.0f + 3.0f * h01(k, 3, 31);
        cv_ellipse(cx, cy, r * 1.2f, r, R_STONE, 0.2f, 0.65f);
    }
    for (int i = 0; i < 40; i++) cv_px(18 + (int)(h01(i, 5, 31) * 28.0f), 60 + (int)(h01(i, 6, 31) * 4.0f), sh(R_BONE, 0.3f));   /* dust */
    cv_outline(0);
    cv_store(SP_RUBBLE);
}

/* fire bursting from the floor's cracks: a narrow tongue the height of a man, flickering */
static void spr_erupt(int id, int frame) {
    cv_begin(cv_sprite, TS, TS);
    for (int y = 4; y < TS; y++)
        for (int x = 18; x < 46; x++) {
            float k = (float)(TS - y) / 60.0f;                      /* 0 at the floor, 1 at the top */
            float w = 11.0f * (1.0f - k * 0.75f) + 2.0f * sinf(y * 0.45f + frame * 2.1f);
            float dx = fabsf(x + 0.5f - 32.0f - 3.0f * sinf(y * 0.2f + frame * 1.7f) * k);
            if (dx > w) continue;
            float v = 1.0f - dx / w * 0.6f - k * 0.45f + 0.2f * (fbm(x / 4.0f, (y + frame * 9) / 4.0f, 16, 99) - 0.5f);
            if (h01(x, y + frame * 31, 5) > 0.9f - k * 0.2f && k > 0.5f) continue;   /* ragged at the top */
            cv_px(x, y, sh(R_FLAME, v));
        }
    cv_store(id);
}

/* the void's column: purple dark, thickest at the floor, with motes in it; the renderer draws it see-through */
static void spr_voidcol(void) {
    cv_begin(cv_sprite, TS, TS);
    for (int y = 0; y < TS; y++)
        for (int x = 14; x < 50; x++) {
            float k = (float)y / (TS - 1), dx = fabsf(x + 0.5f - 32.0f) / 18.0f;
            if (dx > 1.0f || h01(x, y, 61) > 0.35f + 0.6f * k * (1.0f - dx)) continue;
            cv_px(x, y, sh(R_PURPLE, 0.15f + 0.35f * k * (1.0f - dx)));
        }
    for (int i = 0; i < 26; i++) cv_px(18 + (int)(h01(i, 1, 63) * 28.0f), (int)(h01(i, 2, 63) * 60.0f), sh(R_PURPLE, 0.85f + 0.15f * h01(i, 3, 63)));
    cv_store(SP_VOIDCOL);
}

/* hands of the dark, rising: black, long-fingered, reaching */
static void spr_hands(void) {
    cv_begin(cv_sprite, TS, TS);
    for (int h = 0; h < 3; h++) {
        float bx = 20.0f + h * 12.0f, top = 34.0f + 6.0f * h01(h, 1, 71), lean = (h - 1) * 1.6f;
        cv_capsule(bx, 63, bx + lean, top + 8.0f, 2.6f, R_IRON, 0.02f, 0.2f);   /* the wrist */
        for (int f = 0; f < 4; f++) {
            float a = -0.9f + 0.6f * f + 0.15f * h, len = 7.0f + 2.0f * h01(h, f, 73);
            cv_capsule(bx + lean, top + 8.0f, bx + lean + sinf(a) * len, top + 8.0f - cosf(a) * len, 1.0f, R_IRON, 0.05f, 0.3f);
        }
    }
    cv_outline(0);
    cv_store(SP_HANDS);
}

/* a boss's Named Odious Scrip: a curl of old parchment on the floor, tied with a red cord under a
 * black seal pressed with a skull */
static void spr_scrip(void) {
    cv_begin(cv_sprite, TS, TS);
    cv_flat(32, 60.5f, 14.0f, 1.8f, sh(R_WARM, 0.05f));
    cv_capsule(19, 53, 45, 53, 5.0f, R_BONE, 0.35f, 0.85f);        /* the roll */
    for (int k = 0; k < 2; k++) {                                   /* its curled ends */
        float ex = k ? 45.5f : 18.5f;
        cv_ellipse(ex, 53, 2.2f, 5.2f, R_BONE, 0.2f, 0.7f);
        cv_flat(ex, 53, 0.9f, 2.2f, sh(R_WARM, 0.15f));
    }
    for (int y = 47; y <= 59; y++) { cv_px(30, y, sh(R_BLOOD, 0.55f)); cv_px(31, y, sh(R_BLOOD, 0.75f)); }   /* the cord */
    cv_ellipse(31, 53, 4.6f, 4.4f, R_IRON, 0.02f, 0.3f);           /* the seal, black wax */
    cv_flat(31, 52.6f, 2.0f, 1.8f, sh(R_IRON, 0.38f));             /* a skull pressed in it */
    cv_px(30, 52, sh(R_IRON, 0.05f)); cv_px(32, 52, sh(R_IRON, 0.05f));
    cv_px(31, 54, sh(R_IRON, 0.45f));
    cv_outline(0);
    cv_store(SP_SCRIP);
}

/* The Hexed Totem (1.0.0b7), in the merchant's back room: a black pole stacked with skulls whose eyes
 * burn cold, under an iron crossbar with four hooks. Each boss's scrip hangs from a hook once its ability
 * is unlocked; the game draws those on. */
static void spr_hextotem(void) {
    cv_begin(cv_sprite, TS, TS);
    float cx = 32;
    cv_flat(cx, 62, 11, 1.8f, R_WARM + 1);
    cv_flat(cx, 61.5f, 5, 1.6f, sh(R_IRON, 0.1f));                /* its foot, driven into the floor */
    cv_capsule(cx, 7, cx, 61, 2.5f, R_IRON, 0.04f, 0.38f);         /* the pole, black and old */
    cv_capsule(cx - 15, 17, cx + 15, 17, 1.3f, R_IRON, 0.08f, 0.5f);   /* the crossbar */
    for (int k = 0; k < 4; k++) {                                  /* its hooks */
        int hx = (int)(cx - 13 + k * 8.67f);
        for (int y = 18; y <= 22; y++) cv_px(hx, y, sh(R_IRON, 0.5f));
        cv_px(hx + 1, 23, sh(R_IRON, 0.45f)); cv_px(hx + 2, 22, sh(R_IRON, 0.4f));
    }
    for (int k = 0; k < 4; k++) cv_skull(cx, 55 - k * 9.0f, 0.78f - 0.04f * k, 0.18f, 0.72f);   /* stacked, smaller going up */
    cv_skull(cx, 8.5f, 0.62f, 0.22f, 0.8f);                       /* and one on top, over the bar */
    for (int k = 0; k < 5; k++) {                                  /* their eyes, burning cold */
        float y = k < 4 ? 55 - k * 9.0f + 0.7f : 9.1f, s = k < 4 ? 0.78f - 0.04f * k : 0.62f;
        uint8_t c = (uint8_t)(R_GLOW + 12 + (k & 1) * 2);
        cv_px((int)(cx - 2.4f * s), (int)y, c); cv_px((int)(cx + 2.4f * s), (int)y, c);
    }
    for (int y = 24; y < 60; y += 7) cv_px((int)cx - 3, y, sh(R_BLOOD, 0.4f));   /* old marks cut in the wood */
    cv_outline(0);
    cv_store(SP_HEXTOTEM);
}

