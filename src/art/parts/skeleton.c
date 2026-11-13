/* A part of crawl_art.c (docs/PLAN-tidy.md): it is #included there in its place and never compiled on its own.
 * Flame; the skeleton and the shield-bearer; the dressing a rank puts on a creature (kit_*); their backs; the wall skull; the web spider. */

/* ------------------------------------------------------------------ */
/* Sprites                                                             */
/* ------------------------------------------------------------------ */





static void flame(float cx, float by, float hgt, float w, float sway) {
    for (int y = (int)(by - hgt); y <= (int)by; y++)
        for (int x = (int)(cx - w - 2); x <= (int)(cx + w + 2); x++) {
            float u = (by - y) / hgt;
            float c = cx + sway * u * u;
            float ww = w * fmaxf(0.0f, sinf(PI_F * (0.15f + 0.85f * (1.0f - u)))) * (1.0f - 0.3f * u);
            if (ww < 0.3f) continue;
            float d = fabsf(x + 0.5f - c) / (ww + 0.01f);
            if (d > 1.0f) continue;
            cv_px(x, y, (uint8_t)(R_FLAME + clampi((int)((1.0f - d * 0.6f - u * 0.5f) * 15.0f), 2, 15)));
        }
}

/* the skeleton: a list of bones that can collapse into a heap */
typedef struct { int kind; float x0, y0, x1, y1, r; } Part;   /* kind 0 bone, 1 skull, 2 pelvis */

static float g_decay;                                   /* 0 on the first level, 1 eight levels down */
static void gem(float cx, float cy, float r, int ramp);
static void skel_draw(float walk, float k, uint32_t seed, int raise) {
    Part P[32]; int n = 0;
    g_skbox[0] = g_skbox[2] = -1;
    #define BONE(a, b, c, d, r) (P[n++] = (Part){ 0, a, b, c, d, r })
    float w = walk;
    BONE(29, 42, 27 + w * 3, 52, 1.6f); BONE(27 + w * 3, 52, 27 + w * 4.5f, 61, 1.4f); BONE(27 + w * 4.5f, 62, 23 + w * 4.5f, 62.5f, 1.0f);
    BONE(35, 42, 37 - w * 3, 52, 1.6f); BONE(37 - w * 3, 52, 37 - w * 4.5f, 61, 1.4f); BONE(37 - w * 4.5f, 62, 41 - w * 4.5f, 62.5f, 1.0f);
    P[n++] = (Part){ 2, 32, 40, 0, 0, 0 };
    BONE(32, 19, 32, 38, 1.4f);
    for (int i = 0; i < 4; i++) {
        float y = 23.5f + i * 3.2f;
        BONE(31, y, 25 + i * 0.6f, y + 2, 0.9f); BONE(25 + i * 0.6f, y + 2, 27 + i * 0.5f, y + 4.2f, 0.8f);
        BONE(33, y, 39 - i * 0.6f, y + 2, 0.9f); BONE(39 - i * 0.6f, y + 2, 37 - i * 0.5f, y + 4.2f, 0.8f);
    }
    BONE(24.5f, 21, 39.5f, 21.5f, 1.1f);
    BONE(24.5f, 21, 21, 31, 1.3f); BONE(21, 31, 19 - w * 2, 41, 1.1f);
    if (raise == 1) { BONE(39.5f, 21.5f, 46, 13, 1.3f); BONE(46, 13, 45, 5, 1.1f); BONE(39, 6, 51, 3, 1.0f); }   /* bone held high */
    else if (raise == 2) { BONE(39.5f, 21.5f, 47, 14, 1.3f); BONE(47, 14, 48, 6, 1.1f); }   /* arm up, to strike */
    else { BONE(39.5f, 21.5f, 43, 31, 1.3f); BONE(43, 31, 48, 24, 1.1f); }
    BONE(32, 17, 32, 20, 1.2f);
    P[n++] = (Part){ 1, 32, 10, 0, 0, 1.0f };
    #undef BONE

    if (!g_keep) cv_begin(cv_sprite, TS, TS);
    if (k > 0.5f) cv_flat(32, 61.5f, 20, 2.5f, R_WARM + 1);
    for (int i = 0; i < n; i++) {
        Part p = P[i];
        if (p.kind == 0 && p.r < 1.3f && h01(i, 41, 77) < g_decay * 0.3f) continue;   /* deeper down, bones go missing */
        float cx = p.kind ? p.x0 : (p.x0 + p.x1) / 2, cy = p.kind ? p.y0 : (p.y0 + p.y1) / 2;
        float ty = 57.0f + 5.0f * h01(i, 1, seed), tx = cx + (h01(i, 2, seed) - 0.5f) * 26.0f;
        float kk = k * k * (3 - 2 * k);
        float ncx = cx + (tx - cx) * kk, ncy = cy + (ty - cy) * powf(kk, 0.8f);
        float rot = kk * (h01(i, 3, seed) - 0.5f) * 3.0f + kk * 1.2f;
        float cr = cosf(rot), sr = sinf(rot);
        if (p.kind == 0) {
            float ax = p.x0 - cx, ay = p.y0 - cy, bx = p.x1 - cx, by = p.y1 - cy;
            float x0 = ncx + ax * cr - ay * sr, y0 = ncy + ax * sr + ay * cr;
            float x1 = ncx + bx * cr - by * sr, y1 = ncy + bx * sr + by * cr;
            if (p.r >= 1.3f) cv_bone(x0, y0, x1, y1, p.r * 0.9f, 0.25f, 0.95f);
            else cv_capsule(x0, y0, x1, y1, p.r, R_BONE, g_back ? 0.15f : 0.25f, g_back ? 0.6f : 0.9f);   /* from behind the ribs fall back */
        } else if (p.kind == 1) {
            uint8_t eye = g_decay > 0.55f ? (uint8_t)(R_GLOW + 13) : (uint8_t)(R_FLAME + 10);   /* the eyes go cold */
            skull_stamp(ncx, ncy - (kk > 0.9f ? 1.5f : 0.0f), kk > 0.6f, 0, k < 0.3f ? eye : 0);   /* fallen, it rolls aside */
            if (g_decay > 0.3f) { cv_line(ncx + 2, ncy - 8, ncx + 3, ncy - 3, R_BONE + 3); cv_px((int)ncx + 4, (int)ncy - 3, R_BONE + 2); }   /* a cracked skull */
        } else {
            cv_ellipse(ncx, ncy, 7, 3.4f, R_BONE, 0.25f, 0.9f);
            if (!g_back) { cv_flat(ncx - 3, ncy, 1.2f, 1.2f, R_BONE); cv_flat(ncx + 3, ncy, 1.2f, 1.2f, R_BONE); }
        }
    }
    if (g_back && k < 0.3f) {                                      /* from behind: the spine's knobs over the ribs, the shoulder blades, the sacrum */
        for (int s2 = -1; s2 <= 1; s2 += 2) {
            float bl[8] = { 32 + s2 * 2.5f, 21.5f, 32 + s2 * 8.5f, 20.8f, 32 + s2 * 7.5f, 24.5f, 32 + s2 * 4.0f, 31.0f };
            for (int yy = 20; yy <= 32; yy++) for (int xx = 22; xx <= 42; xx++) {   /* its edge, dark against the ribs behind */
                int in = 0;
                for (int q = 0, j = 3; q < 4; j = q++) if (((bl[q * 2 + 1] > yy + 0.5f) != (bl[j * 2 + 1] > yy + 0.5f)) && (xx + 0.5f < (bl[j * 2] - bl[q * 2]) * (yy + 0.5f - bl[q * 2 + 1]) / (bl[j * 2 + 1] - bl[q * 2 + 1]) + bl[q * 2])) in = !in;
                if (in) for (int d = 0; d < 4; d++) { int ax = xx + (d == 0) - (d == 1), ay = yy + (d == 2) - (d == 3); cv_px(ax, ay, sh(R_BONE, 0.08f)); }
            }
            cv_poly(bl, 4, R_BONE, 0.45f, 0.95f, 21, 10);
            cv_line(32 + s2 * 3.0f, 23.0f, 32 + s2 * 8.0f, 21.8f, sh(R_BONE, s2 < 0 ? 0.92f : 0.7f));   /* the blade's ridge */
            cv_px(32 + s2 * 4, 28, sh(R_BONE, 0.2f));
        }
        for (int i = 0; i < 10; i++) {
            int y = 19 + i * 2;
            cv_px(32, y, sh(R_BONE, 0.88f)); cv_px(31, y, sh(R_BONE, 0.62f)); cv_px(33, y, sh(R_BONE, 0.4f)); cv_px(32, y + 1, sh(R_BONE, 0.18f));
        }
        float sc[6] = { 29.0f, 38.5f, 35.0f, 38.5f, 32.0f, 45.5f };
        cv_poly(sc, 3, R_BONE, 0.25f, 0.75f, 38, 7);
        cv_px(31, 40, 0); cv_px(33, 40, 0); cv_px(31, 42, 0); cv_px(33, 42, 0); cv_px(32, 44, sh(R_BONE, 0.2f));
    }
    if (g_decay > 0.0f)                                            /* grime, rot and moss on the bones */
        for (int y = 0; y < TS; y++)
            for (int x = 0; x < TS; x++) {
                uint8_t c = cv_sprite[y * TS + x];
                if (c < R_BONE || c >= R_BONE + 16) continue;
                float n = fbm(x / 5.0f, y / 5.0f, 16, seed + 31);
                if (x >= g_skbox[0] && x < g_skbox[2] && y >= g_skbox[1] && y < g_skbox[3]) {   /* the skull keeps its face */
                    if (n < 0.2f + 0.25f * g_decay && c > R_BONE + 4) cv_sprite[y * TS + x] = (uint8_t)(c - 1);
                    continue;
                }
                if (n < 0.28f + 0.32f * g_decay) cv_sprite[y * TS + x] = (uint8_t)(c - 3 < R_BONE + 1 ? R_BONE + 1 : c - 3);
                if (g_decay > 0.35f && n > 0.78f - 0.12f * g_decay) cv_sprite[y * TS + x] = sh(R_MOSS, 0.3f + 0.3f * n);
            }
}

/* ---- the dressing (docs/InitialArtPass.md section 6): what a rank has done to a creature, as a layer over its sprite ----
 * The marks add up rank by rank: rags from II, damage from III, wounds from IV, regalia at V; the condition is the
 * rank's own: II moss and mould, III frost, IV old blood, V char; the gilded are dressed, not hurt. A creature's
 * function draws its sprite, stores it, then for each rank begins a layer (dress_begin), draws its kit and stores it. */
uint8_t crawl_dress[NDRESS][TS * TS];
int16_t crawl_dress_ix[NSPR][NRANK];
static int g_dress_n;
static const uint8_t *g_base;                           /* the body being dressed, column-major, for the kit's masks */
static int base_on(int x, int y) { return (unsigned)x < (unsigned)TS && (unsigned)y < (unsigned)TS && g_base[x * TS + y] != C_CLEAR; }
static uint8_t base_c(int x, int y) { return base_on(x, y) ? g_base[x * TS + y] : (uint8_t)C_CLEAR; }
static void dress_begin(int id) { g_base = crawl_spr[id]; cv_begin(cv_sprite, TS, TS); }
/* keep the layer where it shows the body (ramp) or hangs in the air, not over what the creature carries; a dark rim
 * where it stands out of the body */
static int g_dress_ramp2 = -1, g_dress_ramp3 = -1;         /* other ramps the layer may lie on (the bat's fur; the brood's eggs) */
static int dress_on(uint8_t b, int r) { return r >= 0 && b >= r && b < r + 16; }
static void dress_store(int id, int rank, int ramp) {
    static uint8_t tmp[TS * TS];
    for (int y = 0; y < TS; y++)
        for (int x = 0; x < TS; x++) {
            uint8_t b = base_c(x, y);
            if (b != C_CLEAR && b != 0 && !dress_on(b, ramp) && !dress_on(b, g_dress_ramp2) && !dress_on(b, g_dress_ramp3)) cv_sprite[y * TS + x] = C_CLEAR;
        }
    memcpy(tmp, cv_sprite, sizeof tmp);
    for (int y = 0; y < TS; y++)
        for (int x = 0; x < TS; x++) {
            if (tmp[y * TS + x] != C_CLEAR || base_on(x, y)) continue;
            int n = (x > 0 && tmp[y * TS + x - 1] != C_CLEAR && !IS_FULLBRIGHT(tmp[y * TS + x - 1]))
                  + (x < TS - 1 && tmp[y * TS + x + 1] != C_CLEAR && !IS_FULLBRIGHT(tmp[y * TS + x + 1]))
                  + (y < TS - 1 && tmp[(y + 1) * TS + x] != C_CLEAR && !IS_FULLBRIGHT(tmp[(y + 1) * TS + x]));
            if (n) cv_sprite[y * TS + x] = 0;
        }
    int ix = crawl_dress_ix[id][rank];
    if (ix < 0) { if (g_dress_n >= NDRESS) return; ix = g_dress_n++; crawl_dress_ix[id][rank] = (int16_t)ix; }
    for (int x = 0; x < TS; x++) for (int y = 0; y < TS; y++) crawl_dress[ix][x * TS + y] = cv_sprite[y * TS + x];
}
/* a stored sprite turned about, with its dressing (1.0.0b18): a back is drawn and dressed as the front is, then turned */
static void spr_turn(int id) {
    uint8_t *sp = crawl_spr[id];
    for (int x = 0; x < TS / 2; x++) for (int y = 0; y < TS; y++) { uint8_t t = sp[x * TS + y]; sp[x * TS + y] = sp[(TS - 1 - x) * TS + y]; sp[(TS - 1 - x) * TS + y] = t; }
    for (int r = 0; r < NRANK; r++) {
        int ix = crawl_dress_ix[id][r];
        if (ix < 0) continue;
        uint8_t *d = crawl_dress[ix];
        for (int x = 0; x < TS / 2; x++) for (int y = 0; y < TS; y++) { uint8_t t = d[x * TS + y]; d[x * TS + y] = d[(TS - 1 - x) * TS + y]; d[(TS - 1 - x) * TS + y] = t; }
    }
}
/* rags hanging from a line, len long, ragged and torn, swung by w; behind: only where the body isn't (a cloak) */
static void kit_rags(float x0, float y0, float x1, float y1, float len, float w, int ramp, float lo, float hi, int behind, uint32_t seed) {
    int n = (int)hypotf(x1 - x0, y1 - y0) + 1;
    for (int i = 0; i <= n; i++) {
        float t = (float)i / (float)n, x = x0 + (x1 - x0) * t, y = y0 + (y1 - y0) * t;
        float l = len * (0.5f + 0.5f * h01(i, 1, seed));
        if (h01(i, 2, seed) < 0.15f) l *= 0.35f;                    /* torn away */
        for (int k = 0; k < (int)l; k++) {
            float u = (float)k / len;
            int px = (int)(x + w * u * 1.6f + sinf((float)k * 0.6f + (float)i) * 0.35f), py = (int)(y + (float)k);
            if (behind && base_on(px, py)) continue;
            float v = lo + (hi - lo) * (1.0f - 0.7f * u) * (0.65f + 0.35f * h01(i, k, seed + 3)) - (((i + (int)(seed & 3)) % 4) == 0 ? 0.07f : 0.0f);
            cv_px(px, py, sh(ramp, v));
        }
    }
}
/* frost along the tops of the body and icicles under it; amount 0-1 */
static void kit_frost(float amount, uint32_t seed) {
    for (int y = 0; y < TS; y++)
        for (int x = 0; x < TS; x++) {
            if (!base_on(x, y) || base_c(x, y) == 0) continue;
            float r = h01(x, y, seed);
            if (!base_on(x, y - 1) && r < amount) cv_px(x, y, r < amount * 0.15f ? (uint8_t)C_WHITE : (uint8_t)(R_GHOST + 12 + (int)(r * 7.0f) % 4));
            else if (!base_on(x, y + 1) && r < amount * 0.5f) {   /* an icicle */
                int l = 1 + (int)(h01(x, y, seed + 1) * 3.5f * amount);
                for (int k = 1; k <= l; k++) cv_px(x, y + k, (uint8_t)(R_GHOST + 14 - k * 2));
            } else if (r < amount * 0.12f) cv_px(x, y, (uint8_t)(R_GHOST + 10));   /* rime */
        }
}
/* moss and mould on the body, and a few pale caps of fungus */
static void kit_moss(float amount, int caps, uint32_t seed) {
    for (int y = 0; y < TS; y++)
        for (int x = 0; x < TS; x++) {
            if (!base_on(x, y) || base_c(x, y) == 0) continue;
            float n = fbm(x / 4.0f, y / 4.0f, 16, seed);
            if (n > 1.0f - amount) cv_px(x, y, sh(R_MOSS, 0.15f + 0.5f * (n - 1.0f + amount) / amount + 0.15f * h01(x, y, seed)));
        }
    for (int i = 0, made = 0; i < 200 && made < caps; i++) {
        int x = 16 + (int)(h01(i, 1, seed) * 32.0f), y = 20 + (int)(h01(i, 2, seed) * 30.0f);
        if (!base_on(x, y) || base_on(x, y - 1)) continue;          /* on a top edge */
        cv_px(x, y - 1, sh(R_SKIN, 0.85f)); cv_px(x + 1, y - 1, sh(R_SKIN, 0.7f)); cv_px(x, y, sh(R_SKIN, 0.4f));
        made++;
    }
}
/* old blood run down from a wound, following the body */
static void kit_blood(float cx, float cy, int len, int width, uint32_t seed) {
    for (int c = 0; c < width; c++) {
        int x = (int)cx + c - width / 2, l = (int)((float)len * (0.5f + 0.5f * h01(c, 1, seed)));
        for (int k = 0; k < l; k++) {
            int y = (int)cy + k;
            if (!base_on(x, y)) { if (k > 2) break; continue; }
            cv_px(x, y, (uint8_t)(R_BLOOD + 2 + (int)(4.0f * (1.0f - (float)k / (float)(l + 1))) + (int)(h01(x, y, seed) * 2.0f)));
        }
    }
}
/* char and soot on the body, an ember or two still in it */
static void kit_char(float amount, uint32_t seed) {
    for (int y = 0; y < TS; y++)
        for (int x = 0; x < TS; x++) {
            if (!base_on(x, y) || base_c(x, y) == 0) continue;
            float n = fbm(x / 3.0f, y / 3.0f, 16, seed);
            if (n < amount) cv_px(x, y, (uint8_t)(R_IRON + 1 + (int)(n / amount * 4.0f)));
            else if (h01(x, y, seed + 5) > 0.992f) cv_px(x, y, (uint8_t)(R_FLAME + 4));
        }
}
/* gold leaf laid on the body's lit edges */
static void kit_gold(int ramp, float amount, uint32_t seed) {
    for (int y = 0; y < TS; y++)
        for (int x = 0; x < TS; x++) {
            uint8_t b = base_c(x, y);
            if (b < ramp || b >= ramp + 16) continue;
            int v = b - ramp;
            if (v >= 9 && fbm(x / 3.0f, y / 3.0f, 16, seed) > 1.0f - amount) cv_px(x, y, (uint8_t)(R_GOLD + clampi(v - 3, 4, NGOLD - 1)));
        }
}
/* an arrow in it: the shaft out at angle a, fletched */
static void kit_arrow(float x, float y, float a, float len) {
    float dx = cosf(a), dy = sinf(a);
    for (int k = 0; k <= (int)len; k++) cv_px((int)(x + dx * k), (int)(y + dy * k), sh(R_WOOD, 0.55f - 0.2f * (float)k / len));
    float ex = x + dx * len, ey = y + dy * len;
    for (int k = 0; k < 3; k++) { cv_px((int)(ex - dx * k - dy), (int)(ey - dy * k + dx), sh(R_GHOST, 0.75f)); cv_px((int)(ex - dx * k + dy), (int)(ey - dy * k - dx), sh(R_GHOST, 0.55f)); }
}
/* an iron circlet, broken: a band with spikes, one snapped */
static void kit_crown(float cx, float cy, float w, uint32_t seed) {
    for (int x = (int)(cx - w / 2); x <= (int)(cx + w / 2); x++) {
        float u = (x - cx) / (w / 2);
        cv_px(x, (int)cy, sh(R_IRON, 0.55f - 0.25f * u)); cv_px(x, (int)cy + 1, sh(R_IRON, 0.3f - 0.15f * u));
    }
    for (int k = 0; k < 4; k++) {
        float x = cx - w / 2 + 1.5f + (float)k * (w - 3.0f) / 3.0f;
        int h = k == 2 ? 1 : 3 + (k == 1);
        for (int j = 1; j <= h; j++) cv_px((int)x, (int)cy - j, sh(R_IRON, 0.7f - 0.1f * j - 0.15f * (x > cx)));
    }
    cv_px((int)(cx + 1), (int)cy + 1, sh(R_BRICK, 0.45f));               /* rust */
}
/* a chain sagging from (x0,y0) to (x1,y1), links light and dark; a shackle at the first end */
static void kit_chain(float x0, float y0, float x1, float y1, float sag, int ramp) {
    int n = (int)(hypotf(x1 - x0, y1 - y0) * 1.2f) + 1;
    for (int i = 0; i <= n; i++) {
        float t = (float)i / (float)n;
        int x = (int)(x0 + (x1 - x0) * t), y = (int)(y0 + (y1 - y0) * t + sag * 4.0f * t * (1.0f - t));
        cv_px(x, y, ramp == R_GOLD ? (uint8_t)(R_GOLD + ((i & 1) ? 11 : 6)) : sh(ramp, (i & 1) ? 0.7f : 0.3f));
    }
    if (ramp != R_GOLD) { cv_flat(x0, y0, 1.6f, 1.6f, sh(ramp, 0.5f)); cv_px((int)x0, (int)y0, 0); }
}
/* the skull's face turned to gold: a death-mask, the sockets left dark */
static void kit_mask(int x0, int y0, int x1, int y1) {
    for (int y = y0; y < y1; y++)
        for (int x = x0; x < x1; x++) {
            uint8_t b = base_c(x, y);
            if (b > R_BONE && b < R_BONE + 16) cv_px(x, y, (uint8_t)(R_GOLD + clampi((b - R_BONE) - 2, 2, NGOLD - 1)));
        }
}

/* the skeleton's dressing (and the shield-bearer's): hx, hy its right hand, w its stride */
static void skel_dress(int id, float w, float hx, float hy) {
    uint32_t sd = (uint32_t)id * 7u + 101u;
    for (int r = RK_II; r < NRANK; r++) {
        dress_begin(id);
        int gild = r == RK_GILDED;
        if (gild) {                                                /* the honoured dead: a clean shroud, gold on it */
            kit_rags(26, 42, 38, 42.5f, 8, w * 0.3f, R_WARM, 0.45f, 0.8f, 0, sd);
            for (int x = 26; x <= 38; x++) cv_px(x, 42, (uint8_t)(R_GOLD + 9 + (x & 1)));
            kit_gold(R_BONE, 0.55f, sd + 1);
            kit_mask(25, 3, 40, 14);
            kit_chain(26, 24, 38, 25, 2.5f, R_GOLD);
        } else {
            /* rags from II, longer each rank; the dead's cloak at V, behind them */
            float len = r == RK_II ? 9.0f : r == RK_III ? 10.0f : 11.0f;
            int cr = r == RK_IV ? R_BLOOD : r == RK_V ? R_IRON : R_WARM;
            kit_rags(26, 42, 38, 43, len, w * (r == RK_III ? 0.25f : 1.0f), cr, cr == R_WARM ? 0.2f : 0.08f, cr == R_WARM ? 0.55f : 0.35f, 0, sd);
            kit_rags(23, 20.5f, 28, 21.5f, r == RK_II ? 5.0f : 7.0f, w * 0.5f, cr, cr == R_WARM ? 0.2f : 0.08f, cr == R_WARM ? 0.5f : 0.3f, 0, sd + 9);
            if (r == RK_V) kit_rags(22, 20, 42, 21, 30, w * 0.6f, R_IRON, 0.03f, 0.2f, 1, sd + 17);
            /* the condition: the rank's own */
            if (r == RK_II) kit_moss(0.3f, 3, sd + 2);
            if (r == RK_III) kit_frost(0.75f, sd + 3);
            if (r == RK_V) kit_char(0.32f, sd + 4);
            /* damage from III: a notch out of the crown, the crack wider */
            if (r >= RK_III) { cv_px(35, 3, 0); cv_px(36, 4, 0); cv_px(36, 3, 0); cv_line(35, 5, 34, 9, sh(R_BONE, 0.15f)); }
            /* wounds from IV: an arrow in the ribs, a cut across the brow, and what ran from them */
            if (r >= RK_IV) {
                kit_blood(34, 29, 12, 2, sd + 5); kit_blood(28, 8, 5, 1, sd + 6);
                kit_arrow(34, 28, -0.6f, 9.0f);
                cv_line(27, 6, 30, 8, 0); cv_px(28, 6, sh(R_BLOOD, 0.3f));
            }
            /* regalia at V: an iron crown, broken, and a shackle on the wrist with its chain */
            if (r == RK_V) { kit_crown(32, 3, 12, sd); kit_chain(hx, hy, hx + 3, hy + 14, 1.5f, R_IRON); }
        }
        dress_store(id, r, R_BONE);
    }
}

static void spr_skeleton(int id, float walk, float k, uint32_t seed, int raise) {
    skel_draw(walk, k, seed, raise);
    cv_outline(0);
    cv_store(id);
    if (k < 0.3f) skel_dress(id, walk, raise == 1 ? 46.0f : raise == 2 ? 48.0f : 48.0f, raise == 1 ? 13.0f : raise == 2 ? 6.0f : 24.0f);
}

/* A skeleton with a round wooden shield on its arm and a sword: it closes in and strikes */
static void spr_shieldskel(int id, float walk, int swing, int shield) {
    skel_draw(walk, 0.0f, 13, swing ? 2 : 0);
    if (swing) {                                                   /* the blade raised over its head */
        cv_capsule(48, 6, 46, -18, 1.0f, R_IRON, 0.5f, 1.0f);
        cv_capsule(44, 7, 52, 5, 0.9f, R_WOOD, 0.3f, 0.6f);
    } else {                                                       /* held low and forward */
        cv_capsule(48, 24, 55, 9, 1.0f, R_IRON, 0.5f, 1.0f);
        cv_capsule(45, 26, 51, 22, 0.9f, R_WOOD, 0.3f, 0.6f);
    }
    if (shield) {                                                  /* the shield on the left arm, in front of the ribs */
        float cx = 22.5f, cy = 32.0f;
        for (int y = 17; y < 48; y++)
            for (int x = 7; x < 39; x++) {
                float nx = (x + 0.5f - cx) / 13.0f, ny = (y + 0.5f - cy) / 14.5f, d = nx * nx + ny * ny;
                if (d > 1.0f) continue;
                float v = 0.3f + 0.45f * lit(nx * 0.4f, ny * 0.4f, sqrtf(1.0f - d * 0.6f));
                uint8_t c;
                if (d > 0.8f) c = sh(R_IRON, v + 0.25f);                                 /* iron rim */
                else if (d < 0.05f) c = sh(R_IRON, v + 0.45f);                           /* the boss */
                else {
                    int plank = (int)((x - 7) / 5.5f);
                    c = sh(R_WOOD, v + 0.08f * h01(plank, 3, 51) - (((x - 7) % 6) == 0 ? 0.12f : 0.0f) - g_decay * 0.15f * h01(x / 3, y / 3, 52));
                    if (fabsf(y + 0.5f - cy) < 1.2f) c = sh(R_IRON, v + 0.1f);          /* a band across */
                }
                cv_px(x, y, c);
            }
        cv_px(18, 26, sh(R_IRON, 0.9f)); cv_px(27, 38, sh(R_IRON, 0.9f));
    } else {                                                       /* what is left of it: a splintered strap */
        cv_capsule(19, 28, 26, 35, 1.0f, R_WOOD, 0.2f, 0.45f);
        cv_line(24, 29, 21, 24, sh(R_WOOD, 0.4f)); cv_line(26, 33, 29, 29, sh(R_WOOD, 0.35f));
    }
    cv_outline(0);
    cv_store(id);
    skel_dress(id, walk, 48.0f, swing ? 6.0f : 24.0f);
}

/* ---- 1.0.0b18: the backs (docs/PLAN-ai.md, Part H; S1 a). What turns its back on you shows it: the skull from behind, the
 * spine over the ribs, the shoulder blades; what it carries on the other side. Drawn as from the front, then turned about. ---- */
static void cv_mirror(void) {
    for (int y = 0; y < TS; y++)
        for (int x = 0; x < TS / 2; x++) { uint8_t t = cv_sprite[y * TS + x]; cv_sprite[y * TS + x] = cv_sprite[y * TS + TS - 1 - x]; cv_sprite[y * TS + TS - 1 - x] = t; }
}
/* the dressing seen from behind: no death-mask or cut brow; the dead's cloak at V hangs over the back, an arrow's fletching
 * stands out of it, the crown's notch on the other side */
static void skel_dress_back(int id, float w, float hx, float hy) {
    uint32_t sd = (uint32_t)id * 7u + 101u;
    for (int r = RK_II; r < NRANK; r++) {
        dress_begin(id);
        if (r == RK_GILDED) {
            kit_rags(26, 42, 38, 42.5f, 8, w * 0.3f, R_WARM, 0.45f, 0.8f, 0, sd);
            for (int x = 26; x <= 38; x++) cv_px(x, 42, (uint8_t)(R_GOLD + 9 + (x & 1)));
            kit_gold(R_BONE, 0.55f, sd + 1);
            kit_chain(26, 22, 38, 23, 1.5f, R_GOLD);               /* the chain's clasp at the nape */
        } else {
            float len = r == RK_II ? 9.0f : r == RK_III ? 10.0f : 11.0f;
            int cr = r == RK_IV ? R_BLOOD : r == RK_V ? R_IRON : R_WARM;
            kit_rags(26, 42, 38, 43, len, w * (r == RK_III ? 0.25f : 1.0f), cr, cr == R_WARM ? 0.2f : 0.08f, cr == R_WARM ? 0.55f : 0.35f, 0, sd);
            kit_rags(36, 20.5f, 41, 21.5f, r == RK_II ? 5.0f : 7.0f, w * 0.5f, cr, cr == R_WARM ? 0.2f : 0.08f, cr == R_WARM ? 0.5f : 0.3f, 0, sd + 9);
            if (r == RK_II) kit_moss(0.3f, 3, sd + 2);
            if (r == RK_III) kit_frost(0.75f, sd + 3);
            if (r == RK_V) kit_char(0.32f, sd + 4);
            if (r >= RK_III) { cv_px(28, 3, 0); cv_px(27, 4, 0); cv_px(27, 3, 0); }
            if (r >= RK_IV) {                                      /* the arrow through it from in front: its fletching out of the back */
                kit_blood(30, 30, 9, 1, sd + 5);
                cv_line(30, 29, 26, 25, sh(R_WOOD, 0.45f));
                cv_px(25, 24, sh(R_BLOOD, 0.4f)); cv_px(26, 23, sh(R_BLOOD, 0.55f)); cv_px(24, 25, sh(R_BLOOD, 0.3f)); cv_px(25, 26, sh(R_BLOOD, 0.45f));
            }
            if (r == RK_V) { kit_rags(22, 20, 42, 21, 30, w * 0.6f, R_IRON, 0.03f, 0.2f, 0, sd + 17); kit_crown(32, 3, 12, sd); kit_chain(hx, hy, hx - 3, hy + 14, 1.5f, R_IRON); }
        }
        dress_store(id, r, R_BONE);
    }
}
static void spr_skel_back(int id, float walk) {
    cv_begin(cv_sprite, TS, TS);
    g_back = 1; g_keep = 1;
    skel_draw(walk, 0.0f, 3, 0);
    g_back = 0; g_keep = 0;
    cv_mirror();
    cv_outline(0);
    cv_store(id);
    skel_dress_back(id, -walk, 16.0f, 24.0f);
}
/* the shield-bearer from behind: the shield's back on its arm, planks and two straps, the rim's rivets; the blade low on the
 * other side; the body in front of both */
static void spr_shield_back(int id, float walk, int shield) {
    cv_begin(cv_sprite, TS, TS);
    cv_capsule(48, 24, 55, 9, 1.0f, R_IRON, 0.25f, 0.6f);
    cv_capsule(45, 26, 51, 22, 0.9f, R_WOOD, 0.2f, 0.45f);
    if (shield) {
        float cx = 22.5f, cy = 32.0f;
        for (int y = 17; y < 48; y++)
            for (int x = 7; x < 39; x++) {
                float nx = (x + 0.5f - cx) / 13.0f, ny = (y + 0.5f - cy) / 14.5f, d = nx * nx + ny * ny;
                if (d > 1.0f) continue;
                float v = 0.2f + 0.3f * lit(-nx * 0.4f, ny * 0.4f, sqrtf(1.0f - d * 0.6f));   /* hollow: lit the other way */
                uint8_t c;
                if (d > 0.82f) c = sh(R_IRON, v + 0.15f);
                else {
                    int plank = (int)((x - 7) / 5.5f);
                    c = sh(R_WOOD, v - 0.04f + 0.07f * h01(plank, 5, 51) - (((x - 7) % 6) == 0 ? 0.12f : 0.0f) - g_decay * 0.15f * h01(x / 3, y / 3, 53));
                    if (h01(x, y / 4, 57) < 0.04f) c = sh(R_WOOD, 0.05f);   /* the grain's splits */
                }
                cv_px(x, y, c);
            }
        for (int k = 0; k < 2; k++) {                              /* the straps, the forearm through them */
            float sy = 27.0f + k * 9.0f;
            for (int x = 14; x <= 31; x++) { cv_px(x, (int)sy, sh(R_WARM, 0.32f - 0.01f * (x - 14))); cv_px(x, (int)sy + 1, sh(R_WARM, 0.18f)); }
            cv_px(14, (int)sy, sh(R_IRON, 0.8f)); cv_px(31, (int)sy, sh(R_IRON, 0.6f));
        }
        for (int k = 0; k < 10; k++) {                             /* the rim's rivets, from behind */
            float a = k * 0.628f; cv_px((int)(cx + cosf(a) * 11.6f), (int)(cy + sinf(a) * 13.0f), sh(R_IRON, 0.75f));
        }
    } else {
        cv_capsule(19, 28, 26, 35, 1.0f, R_WOOD, 0.15f, 0.4f);
        cv_line(24, 29, 21, 24, sh(R_WOOD, 0.35f));
    }
    g_back = 1; g_keep = 1;
    skel_draw(walk, 0.0f, 13, 0);
    g_back = 0; g_keep = 0;
    cv_mirror();
    cv_outline(0);
    cv_store(id);
    skel_dress_back(id, -walk, 16.0f, 24.0f);
}

/* The wall skull (docs/InitialArtPass.md section 8): the master skull three times over (Scale3x keeps its corners), grain
 * and cracks at the finer scale, set in the stone to watch the halls. Asleep, embers deep in its sockets; woken, fire
 * gathers in them; blazing, it shows through the teeth and the cracks; broken, it splits and the right half falls. */
static uint8_t g_big[64 * 64];
static int g_bigw, g_bigh;
static void grid_scale3(const char *const *g, int w, int h, int mirror) {   /* into g_big: palette indices, C_CLEAR, 0 the sockets */
    uint8_t src[SKULL_H][SKULL_W];
    for (int y = 0; y < h; y++)
        for (int x = 0; x < w; x++) {
            char ch = g[y][mirror ? w - 1 - x : x];
            src[y][x] = ch == '.' ? (uint8_t)C_CLEAR : (ch == '0' || ch == '@') ? 0 : (uint8_t)(R_BONE + (ch <= '9' ? ch - '0' : ch - 'a' + 10));
        }
    #define S3(yy, xx) ((yy) < 0 || (yy) >= h || (xx) < 0 || (xx) >= w ? (uint8_t)C_CLEAR : src[yy][xx])
    g_bigw = w * 3; g_bigh = h * 3;
    for (int y = 0; y < h; y++)
        for (int x = 0; x < w; x++) {
            uint8_t A = S3(y - 1, x - 1), B = S3(y - 1, x), C = S3(y - 1, x + 1), D = S3(y, x - 1), E = src[y][x], F = S3(y, x + 1);
            uint8_t G = S3(y + 1, x - 1), H = S3(y + 1, x), I = S3(y + 1, x + 1), o[9];
            int db = D == B && D != H && B != F, bf = B == F && B != D && F != H, dh = D == H && D != B && H != F, hf = H == F && D != H && B != F;
            o[0] = db ? D : E; o[1] = (db && E != C) || (bf && E != A) ? B : E; o[2] = bf ? F : E;
            o[3] = (db && E != G) || (dh && E != A) ? D : E; o[4] = E; o[5] = (bf && E != I) || (hf && E != C) ? F : E;
            o[6] = dh ? D : E; o[7] = (dh && E != I) || (hf && E != G) ? H : E; o[8] = hf ? F : E;
            for (int k = 0; k < 9; k++) g_big[(y * 3 + k / 3) * 64 + x * 3 + k % 3] = o[k];
        }
    #undef S3
}
static void spr_wskull(int id, int heat) {
    cv_begin(cv_sprite, TS, TS);
    grid_scale3(SKULL_FRONT, SKULL_W, SKULL_H, 0);
    int x0 = 32 - g_bigw / 2, y0 = 6;
    for (int y = 0; y < g_bigh; y++)                               /* the bone, with a grain finer than its pixels */
        for (int x = 0; x < g_bigw; x++) {
            uint8_t c = g_big[y * 64 + x];
            if (c == C_CLEAR) continue;
            if (c > R_BONE && c < R_BONE + 16) {
                float r = h01(x, y, 71), n = fbm(x / 5.0f, y / 5.0f, 16, 73);
                int v = c - R_BONE + (r < 0.12f ? -1 : r > 0.93f ? 1 : 0) - (n < 0.3f ? 1 : 0);
                c = (uint8_t)(R_BONE + clampi(v, 1, 15));
            }
            cv_px(x0 + x, y0 + y, c);
        }
    static const float CR[3][4] = { { 33, 6, 1.1f, 13 }, { 15, 18, 0.5f, 9 }, { 40, 34, 1.9f, 8 } };   /* cracks wandering through it */
    for (int k = 0; k < 3; k++) {
        float x = CR[k][0], y = CR[k][1], a = CR[k][2];
        for (int i = 0; i < (int)CR[k][3]; i++) {
            a += (h01(k, i, 79) - 0.5f) * 0.9f; x += cosf(a); y += sinf(a);
            uint8_t c = cv_sprite[(int)y * TS + (int)x];
            if (c > R_BONE && c < R_BONE + 16) cv_px((int)x, (int)y, heat == 2 ? (uint8_t)(R_FLAME + 8 + (i & 3)) : (uint8_t)(R_BONE + 1));
        }
    }
    /* its eyes: the sockets' middles, 3 by 3 from the grid's @ */
    float ex[2], ey[2]; int ne = 0;
    for (int y = 0; y < SKULL_H; y++) for (int x = 0; x < SKULL_W && ne < 2; x++) if (SKULL_FRONT[y][x] == '@') { ex[ne] = x0 + x * 3 + 1.5f; ey[ne] = y0 + y * 3 + 1.5f; ne++; }
    if (heat == 3) {                                               /* broken: split, the right half falling away */
        static uint8_t tmp[TS * TS];
        memcpy(tmp, cv_sprite, sizeof tmp);
        for (int y = 0; y < TS; y++)
            for (int x = 0; x < TS; x++) {
                int jag = 33 + (int)(3.0f * sinf(y * 0.7f)) + (y & 1);
                if (x >= jag - 1 && x <= jag) { cv_sprite[y * TS + x] = C_CLEAR; continue; }
                if (x > jag) { cv_sprite[y * TS + x] = C_CLEAR; int sy = y - 6, sx = x - 3; if (sy >= 0 && sx > jag) cv_sprite[y * TS + x] = tmp[sy * TS + sx]; }
            }
        for (int i = 0; i < 7; i++) cv_ellipse(14 + h01(i, 1, 81) * 36, 57 + h01(i, 2, 81) * 5, 1.5f + h01(i, 3, 81) * 2, 1.2f, R_BONE, 0.2f, 0.8f);   /* shards */
        cv_outline(0); cv_store(id); return;
    }
    for (int k = 0; k < ne; k++) {
        cv_eye_mark((int)ex[k], (int)ey[k]);
        if (!heat) { cv_px((int)ex[k], (int)ey[k], (uint8_t)(R_FLAME + 3)); continue; }   /* a last ember */
        float r = heat == 1 ? 3.0f : 5.5f;
        for (int y = (int)(ey[k] - 7); y <= (int)(ey[k] + 7); y++)   /* fire gathering in the socket, and only there */
            for (int x = (int)(ex[k] - 7); x <= (int)(ex[k] + 7); x++) {
                if ((unsigned)x >= TS || (unsigned)y >= TS || cv_sprite[y * TS + x] != 0) continue;
                float d = hypotf(x + 0.5f - ex[k], (y + 0.5f - ey[k]) * 0.9f) / r;
                if (d < 1.0f) cv_px(x, y, (uint8_t)(R_FLAME + clampi((int)((1.1f - d * 0.8f) * 15), 2, 15)));
            }
        if (heat == 2) for (int i = 0; i < 6; i++) cv_px((int)(ex[k] + (h01(i, k + 2, 83) - 0.5f) * 7), (int)(ey[k] - 5 - h01(i, k + 5, 83) * 6), (uint8_t)(R_FLAME + 9 + (i & 3)));
    }
    if (heat)                                                      /* and in its mouth, and up its nose when it blazes */
        for (int y = y0 + 27; y < y0 + 48; y++)
            for (int x = x0; x < x0 + g_bigw; x++) {
                if (cv_sprite[y * TS + x] != 0) continue;
                int mouth = y >= y0 + 39, nose = y < y0 + 39 && x > x0 + 15 && x < x0 + 30;
                if (mouth) cv_px(x, y, (uint8_t)(R_FLAME + (heat == 2 ? 7 + (int)(h01(x, y, 85) * 6.0f) : 2 + (int)(h01(x, y, 85) * 3.0f))));
                else if (nose && heat == 2) cv_px(x, y, (uint8_t)(R_FLAME + 4 + (int)(h01(x, y, 87) * 4.0f)));
            }
    cv_outline(0);
    cv_store(id);
}

/* the wall skull's dressing: what its rank has done to it, as the skeletons' */
static void wskull_dress(int id) {
    for (int r = RK_II; r < NRANK; r++) {
        dress_begin(id);
        if (r == RK_II) kit_moss(0.35f, 2, 401);
        if (r == RK_III) kit_frost(0.8f, 403);
        if (r == RK_IV) { kit_blood(23, 37, 14, 2, 405); kit_blood(41, 37, 12, 2, 407); }   /* tears of old blood */
        if (r == RK_V) { kit_char(0.35f, 409); kit_crown(32, 7, 26, 411); }
        if (r == RK_GILDED) { kit_gold(R_BONE, 0.6f, 413); kit_mask(10, 6, 55, 42); }
        dress_store(id, r, R_BONE);
    }
}

/* a small skull by hand, for the deacon's hood (dim, in shadow) and his staff, and the web spider's mark at rank V: hex R_BONE, @ the eyes */
static const char *const SKULL_SMALL[10] = {
    "..56775..", ".5799975.", "4799a9974", "40@090@04", "450090054", ".5770775.", ".4a0a0a4.", "..30003..", "..4a0a4..", "...353...",
};
/* the small skull from behind (1.0.0b18): the crown, the bump of the occiput, the jaw's back corners, the neck below */
static const char *const SKULL_SMALL_BACK[10] = {
    "..56775..", ".5799975.", "4799a9974", "4789a9874", "4679a9764", ".5678765.", ".4567654.", "..45054..", "..3.0.3..", "...353...",
};

/* The web spider's head from the front (docs/InitialArtPass.md, section 8): a cluster of eight eyes like coals, two
 * great ones low in front, two above them, two smaller still, two at the sides. Chitin in R_IRON (hex), the eyes fire. */
static const char *const SPIDER_HEAD[8] = {
    "...2345432...", "..34H676H43..", ".3456J7J6543.", "2H567888765H2",
    "3567@N7N@7653", "2456NM6MN6542", ".34567876543.", "..345666543..",
};

/* a spider's leg in two long joints, from its body to the knee and down to the foot: banded where the joints' plates
 * meet (paler, or dark in the band ramp if one is given), bristles standing out at the knee */
static void chitin_leg(float ax, float ay, float kx, float ky, float fx, float fy, float r, int ramp, float lo, float hi, int band, uint32_t seed) {
    cv_capsule(ax, ay, kx, ky, r, ramp, lo, hi);
    cv_capsule(kx, ky, fx, fy, r * 0.75f, ramp, lo, hi * 0.9f);
    for (int seg = 0; seg < 2; seg++) {                              /* bands across each joint */
        float x0 = seg ? kx : ax, y0 = seg ? ky : ay, x1 = seg ? fx : kx, y1 = seg ? fy : ky, L = hypotf(x1 - x0, y1 - y0);
        float nx = -(y1 - y0) / L, ny = (x1 - x0) / L;
        for (int k = 1; k <= 2; k++) {
            float t = k / 3.0f, bx = x0 + (x1 - x0) * t, by = y0 + (y1 - y0) * t;
            for (int q = -1; q <= 1; q++) cv_px((int)(bx + nx * q * 0.8f), (int)(by + ny * q * 0.8f), band >= 0 ? (uint8_t)(band + 1 + (q == 0)) : sh(ramp, hi + 0.12f));
        }
    }
    for (int q = 0; q < 4; q++) {                                    /* bristles at the knee */
        float an = h01(q, 1, seed) * TAU_F, l = 1.5f + 1.5f * h01(q, 2, seed);
        cv_line(kx, ky, kx + cosf(an) * l, ky + sinf(an) * l, sh(ramp, hi * 0.8f));
    }
    cv_px((int)kx, (int)ky, sh(ramp, hi + 0.15f));
}

/* a spider's abdomen: a swollen shell pitted all over, bristles round its rim, lit from above */
static void spider_abdomen(float cx, float cy, float rx, float ry, int ramp, uint32_t seed) {
    for (int y = (int)(cy - ry) - 1; y <= (int)(cy + ry) + 1; y++)
        for (int x = (int)(cx - rx) - 1; x <= (int)(cx + rx) + 1; x++) {
            float nx = (x + 0.5f - cx) / rx, ny = (y + 0.5f - cy) / ry, d = nx * nx + ny * ny;
            if (d > 1.0f) continue;
            float v = 0.05f + 0.3f * (1.0f - d) * (0.6f + 0.4f * (-nx * 0.5f - ny * 0.7f + 0.5f));
            int gx = (int)floorf((x + 64) / 4.0f), gy = (int)floorf((y + 64) / 4.0f);   /* pits, one to a few pixels */
            float px = gx * 4.0f - 64.0f + 1.0f + 2.0f * h01(gx, gy, seed), py = gy * 4.0f - 64.0f + 1.0f + 2.0f * h01(gx, gy, seed + 1);
            float pd = hypotf(x + 0.5f - px, y + 0.5f - py);
            if (pd < 0.9f) v -= 0.12f; else if (pd < 1.7f && x + 0.5f > px && y + 0.5f > py) v += 0.06f;
            cv_px(x, y, sh(ramp, v));
        }
    for (int q = 0; q < 30; q++) {                                   /* bristles round its rim */
        float an = TAU_F * q / 30.0f + h01(q, 3, seed) * 0.15f, l = 1.0f + 1.6f * h01(q, 4, seed);
        float ex = cx + cosf(an) * rx, ey = cy + sinf(an) * ry;
        cv_line(ex, ey, ex + cosf(an) * l, ey + sinf(an) * l, sh(ramp, 0.28f));
    }
}

/* fangs: curved down and in from the jaws' bulges, pale at the tips, a drop of venom on each (falling, biting) */
static void spider_fangs(float cx, float y0, float len, int biting) {
    for (int s = -1; s <= 1; s += 2) {
        cv_ellipse(cx + s * 2.2f, y0, 1.8f, 2.2f, R_IRON, 0.12f, 0.34f);   /* the jaw's bulge */
        float spread = biting ? 1.8f : 0.0f, px = cx + s * (2.4f + spread), py = y0 + 1.0f;
        for (int k = 0; k <= (int)len; k++) {
            float t = k / len, x = cx + s * (2.4f + spread - t * (1.6f + spread * 0.6f) + sinf(t * PI_F) * 0.8f), y = y0 + 1.0f + k;
            cv_px((int)x, (int)y, sh(R_BONE, 0.45f + 0.45f * t)); px = x; py = y;
        }
        cv_px((int)px, (int)py + 1, (uint8_t)(R_MOSS + 14)); cv_px((int)px, (int)py + 2, (uint8_t)(R_MOSS + 10));   /* the drop (no glint: a pale pair there reads as a second pair of eyes) */
        if (biting) cv_px((int)px, (int)py + 5, (uint8_t)(R_MOSS + 11));   /* another falling */
    }
}

/* The web spider: a black shell pitted all over and a red mark, the head before it with its eight coals of eyes, pale
 * fangs with venom on them, banded legs bristling at the knee. Its red eyes are all you see of it in its web until it
 * drops. frame 0-1 walking, 2 biting (the front legs up), 3 dead on its back. Chitin R_IRON (PF_CHITIN). */
static void spr_spider(int id, int frame) {
    cv_begin(cv_sprite, TS, TS);
    if (frame == 3) {                                              /* dead on its back, its legs curled over it */
        spider_abdomen(32, 52, 10.0f, 7.0f, R_IRON, 421);
        for (int s = -1; s <= 1; s += 2)
            for (int l = 0; l < 4; l++)
                chitin_leg(32 + s * (3 + l), 48, 32 + s * (9 + l * 2.5f), 38 - l * 2.0f, 32 + s * (5 + l * 1.5f), 33 - l, 1.1f, R_IRON, 0.05f, 0.3f, -1, 423 + (uint32_t)(l * 2 + s + 1));
        cv_ellipse(32, 46, 6.0f, 4.0f, R_IRON, 0.06f, 0.3f);
        cv_outline(0);
        cv_store(id);
        return;
    }
    float bob = frame == 1 ? 1.0f : 0.0f;
    spider_abdomen(32, 28 + bob, 10.5f, 9.5f, R_IRON, 431);         /* the abdomen, looming behind */
    for (int k = 0; k < 7; k++) {                                   /* its mark: an hourglass in blood */
        int w = k < 3 ? 3 - k : (k > 3 ? k - 3 : 0);
        for (int q = -w; q <= w; q++) cv_px(32 + q, (int)(24 + bob) + k, (uint8_t)(R_BLOOD + 8 + (q == 0) * 2));
    }
    for (int s = -1; s <= 1; s += 2)
        for (int l = 3; l >= 0; l--) {                              /* legs: up to a high knee and down, the back ones first */
            float sw = frame == 1 ? (((l + (s > 0)) & 1) ? 2.0f : -2.0f) : 0.0f;
            float ax = 32 + s * (4.0f + l * 0.8f), ay = 40 + l * 0.6f;
            float kx = 32 + s * (12.0f + l * 4.0f) + sw, ky = 29 - l * 0.8f + sw * 0.5f;
            float fx = 32 + s * (14.0f + l * 5.0f) + sw * 1.5f, fy = 59 - l * 2.5f;
            if (frame == 2 && l == 0) { kx = 32 + s * 11.0f; ky = 22; fx = 32 + s * 7.0f; fy = 14; }   /* the front legs raised */
            if (frame == 2 && l == 1) { ky -= 4; fy -= 6; }
            chitin_leg(ax, ay, kx, ky, fx, fy, 1.25f, R_IRON, 0.05f, 0.36f, -1, 441 + (uint32_t)(l * 2 + s + 1));
        }
    cv_ellipse(32, 40, 7.0f, 5.0f, R_IRON, 0.06f, 0.4f);            /* the head's shell */
    grid_stamp(SPIDER_HEAD, 13, 8, 26, 35, R_IRON, R_IRON, (uint8_t)(R_FLAME + 14), 0);
    spider_fangs(32, 44, frame == 2 ? 6.0f : 4.0f, frame == 2);
    cv_outline(0);
    cv_store(id);
}

/* the web spider's dressing by rank: II silk clinging in strands and mould on its shell, III frost and its shell
 * cracked, IV old blood on its fangs and a pale scar across its back, V charred, the pale skull on its back as the
 * Wandering Spider wears it, gilded gold in its bands and along its shell */
static void spider_dress(int id) {
    for (int r = RK_II; r < NRANK; r++) {
        g_dress_ramp2 = r == RK_V ? R_BLOOD : R_BONE;               /* its fangs too; at V over its mark, which the skull takes */
        dress_begin(id);
        if (r == RK_II) {
            kit_moss(0.25f, 0, 451);
            for (int q = 0; q < 5; q++) {                            /* silk, clinging */
                float x0 = 10 + 44 * h01(q, 1, 453), y0 = 24 + 10 * h01(q, 2, 453), x1 = x0 + (h01(q, 3, 453) - 0.5f) * 16, y1 = y0 + 6 + 10 * h01(q, 4, 453);
                for (int k = 0; k <= 12; k++) { float t = k / 12.0f; int x = (int)(x0 + (x1 - x0) * t), y = (int)(y0 + (y1 - y0) * t + sinf(t * PI_F) * 2.0f); if (base_on(x, y) || (k & 1)) cv_px(x, y, sh(R_GHOST, 0.65f)); }
            }
        }
        if (r == RK_III) {
            kit_frost(0.7f, 455);
            float x = 26, y = 22;                                    /* the shell cracked */
            for (int k = 0; k < 12; k++) { cv_px((int)x, (int)y, 0); cv_px((int)x + 1, (int)y, sh(R_IRON, 0.5f)); x += h01(k, 1, 457) < 0.5f ? 1.0f : 0.0f; y += 1.0f; }
        }
        if (r >= RK_IV && r != RK_GILDED) {
            kit_blood(30, 45, 6, 2, 459); kit_blood(34, 45, 6, 2, 461);
            for (int k = 0; k < 13; k++) cv_px(24 + k, 31 - k / 3 + (k & 1), sh(R_BONE, 0.7f - 0.2f * (k & 1)));   /* a scar */
        }
        if (r == RK_V) {
            kit_char(0.3f, 463);
            cv_flat(32.5f, 25.8f, 6.2f, 6.6f, 0);                 /* the skull's ground, dark */
            grid_stamp(SKULL_SMALL, 9, 10, 28, 21, R_BONE, R_BONE, 0, 0);
        }
        if (r == RK_GILDED)
            for (int y = 0; y < TS; y++)
                for (int x = 0; x < TS; x++) {
                    uint8_t b = base_c(x, y);
                    if (b >= R_IRON + 6 && b < R_IRON + 16 && h01(x, y, 465) < 0.8f) cv_px(x, y, (uint8_t)(R_GOLD + clampi(b - R_IRON, 4, NGOLD - 1)));
                    else if (b >= R_IRON + 4 && b < R_IRON + 6 && !base_on(x, y - 1)) cv_px(x, y, (uint8_t)(R_GOLD + 6));
                }
        dress_store(id, r, R_IRON);
    }
    g_dress_ramp2 = -1;
}

