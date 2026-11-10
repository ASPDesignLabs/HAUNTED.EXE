/* Mock-ups of the portraits of the slain on her walls (docs/PLAN-portraits.md): src/crawl_portrait.c's faces set in her north
 * wall, seen straight on with the crypt's own textures, palette and light, written as animated GIFs (each frame with its own
 * palette) and sheets of stills.
 *
 *   portraitmock <dir> [all|skeleton|veil|room|sheets|card|bench|check]     PM_SCALE=n: the GIFs at n times (3)
 *   (bench: what a portrait costs to draw; check: the runs' stone made alone against the whole art)
 *
 *   make build/portraitmock
 */
#include "../src/crawl.h"
#include "../src/crawl_portrait.h"
#include "../src/crawl_ui.h"
#include "../src/gfx.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <zlib.h>

#define W 320
#define H 200
#define FPS 20
#define FOCAL ((W / 2) / 0.75f)                         /* the crypt's own (PLANE_LEN 0.75) */

static Frame F;
static float clampf_(float v, float a, float b) { return v < a ? a : v > b ? b : v; }
static int clampi_(int v, int a, int b) { return v < a ? a : v > b ? b : v; }
static float smooth(float a, float b, float v) { float t = clampf_((v - a) / (b - a), 0, 1); return t * t * (3 - 2 * t); }
/* ---- GIF: each frame its own palette; only what changed is sent, the rest left as it was (index 255, the sprites'
 * clear, is never on the screen, so it stands for "unchanged") ---- */
static FILE *gf;
static int gw, gh, gsc, gframes;
static uint8_t gprev[H * W], gpal_prev[256][3];
static void gput16(int v) { fputc(v & 255, gf); fputc(v >> 8 & 255, gf); }
static uint8_t gbuf[256]; static int gbn;
static uint32_t gbits; static int gnbits;
static void gbyte(int b) { gbuf[gbn++] = (uint8_t)b; if (gbn == 255) { fputc(255, gf); fwrite(gbuf, 1, 255, gf); gbn = 0; } }
static void gcode(int code, int size) { gbits |= (uint32_t)code << gnbits; gnbits += size; while (gnbits >= 8) { gbyte(gbits & 255); gbits >>= 8; gnbits -= 8; } }
#define HSZ 8209
static int32_t hkey[HSZ]; static int16_t hval[HSZ];
static void lzw(const uint8_t *d, int n) {
    fputc(8, gf);
    gbn = 0; gbits = 0; gnbits = 0;
    int clear = 256, eoi = 257, next = 258, size = 9;
    memset(hkey, 0xff, sizeof hkey);
    gcode(clear, size);
    int pre = d[0];
    for (int i = 1; i < n; i++) {
        int c = d[i];
        int32_t key = pre << 8 | c;
        int h = (int)(((uint32_t)key * 2654435761u) % HSZ);
        while (hkey[h] != -1 && hkey[h] != key) h = (h + 1) % HSZ;
        if (hkey[h] == key) { pre = hval[h]; continue; }
        gcode(pre, size);
        if (next < 4096) { hkey[h] = key; hval[h] = (int16_t)next++; if (next > (1 << size) && size < 12) size++; }
        else { gcode(clear, size); memset(hkey, 0xff, sizeof hkey); next = 258; size = 9; }
        pre = c;
    }
    gcode(pre, size);
    gcode(eoi, size);
    if (gnbits > 0) gbyte(gbits & 255);
    if (gbn) { fputc(gbn, gf); fwrite(gbuf, 1, (size_t)gbn, gf); }
    fputc(0, gf);
}
static int gif_begin(const char *path, int sc) {
    gf = fopen(path, "wb");
    if (!gf) return 0;
    gsc = sc; gw = W * sc; gh = H * sc; gframes = 0;
    fwrite("GIF89a", 1, 6, gf);
    gput16(gw); gput16(gh);
    fputc(0x70, gf); fputc(0, gf); fputc(0, gf);         /* no global table */
    static const uint8_t NS[] = { 0x21, 0xFF, 11, 'N', 'E', 'T', 'S', 'C', 'A', 'P', 'E', '2', '.', '0', 3, 1, 0, 0, 0 };
    fwrite(NS, 1, sizeof NS, gf);                       /* round and round */
    return 1;
}
static void gif_frame(void) {
    int full = gframes == 0 || memcmp(gpal_prev, F.pal, sizeof gpal_prev) != 0;
    int x0 = 0, y0 = 0, x1 = W - 1, y1 = H - 1;
    if (!full) {
        x0 = W; y0 = H; x1 = -1; y1 = -1;
        for (int y = 0; y < H; y++) for (int x = 0; x < W; x++) if (F.pix[y * W + x] != gprev[y * W + x]) {
            if (x < x0) x0 = x; if (x > x1) x1 = x; if (y < y0) y0 = y; if (y > y1) y1 = y;
        }
        if (x1 < 0) { x0 = y0 = 0; x1 = y1 = 0; }
    }
    int rw = (x1 - x0 + 1) * gsc, rh = (y1 - y0 + 1) * gsc;
    uint8_t *d = malloc((size_t)rw * rh);
    for (int y = 0; y < rh; y++) for (int x = 0; x < rw; x++) {
        int sx = x0 + x / gsc, sy = y0 + y / gsc;
        uint8_t c = F.pix[sy * W + sx];
        d[y * rw + x] = !full && c == gprev[sy * W + sx] ? 255 : (c == 255 ? 0 : c);
    }
    static const uint8_t GCE[] = { 0x21, 0xF9, 4, 0x05, 100 / FPS, 0, 255, 0 };   /* leave it; 255 clear; the delay */
    fwrite(GCE, 1, sizeof GCE, gf);
    fputc(0x2C, gf); gput16(x0 * gsc); gput16(y0 * gsc); gput16(rw); gput16(rh);
    fputc(0x87, gf);                                    /* a local table of 256 */
    for (int i = 0; i < 256; i++) for (int c = 0; c < 3; c++) fputc(F.pal[i][c] * 255 / 63, gf);
    lzw(d, rw * rh);
    free(d);
    memcpy(gprev, F.pix, sizeof gprev);
    memcpy(gpal_prev, F.pal, sizeof gpal_prev);
    gframes++;
}
static void gif_end(void) { fputc(0x3B, gf); fclose(gf); gf = NULL; }

/* ---- PNG, for the sheet ---- */
static void put32(FILE *f, uint32_t v) { uint8_t b[4] = { v >> 24, v >> 16, v >> 8, v }; fwrite(b, 1, 4, f); }
static void chunk(FILE *f, const char *type, const uint8_t *data, uint32_t len) {
    put32(f, len); fwrite(type, 1, 4, f); if (len) fwrite(data, 1, len, f);
    uint32_t crc = crc32(0, (const Bytef *)type, 4); if (len) crc = crc32(crc, data, len); put32(f, crc);
}
static int write_png(const char *path, const uint8_t *rgb, int iw, int ih) {
    size_t rowlen = 1 + (size_t)iw * 3;
    uint8_t *raw = malloc(rowlen * ih);
    for (int y = 0; y < ih; y++) { raw[rowlen * y] = 0; memcpy(raw + rowlen * y + 1, rgb + (size_t)y * iw * 3, (size_t)iw * 3); }
    uLongf clen = compressBound((uLong)(rowlen * ih));
    uint8_t *comp = malloc(clen);
    compress2(comp, &clen, raw, (uLong)(rowlen * ih), 9);
    FILE *f = fopen(path, "wb");
    if (!f) { free(raw); free(comp); return 0; }
    static const uint8_t sig[8] = { 0x89, 'P', 'N', 'G', 13, 10, 26, 10 };
    fwrite(sig, 1, 8, f);
    uint8_t ihdr[13] = { iw >> 24, iw >> 16, iw >> 8, iw, ih >> 24, ih >> 16, ih >> 8, ih, 8, 2, 0, 0, 0 };
    chunk(f, "IHDR", ihdr, 13); chunk(f, "IDAT", comp, (uint32_t)clen); chunk(f, "IEND", NULL, 0);
    fclose(f); free(raw); free(comp);
    return 1;
}
static uint8_t *sheet; static int sw, sh_;
static void sheet_put(int ox, int oy, int x0, int y0, int w, int h, int sc) {   /* a part of F, sc times */
    for (int y = 0; y < h * sc; y++) for (int x = 0; x < w * sc; x++) {
        int X = ox + x, Y = oy + y;
        if (X < 0 || Y < 0 || X >= sw || Y >= sh_) continue;
        const uint8_t *p = F.pal[F.pix[(y0 + y / sc) * W + x0 + x / sc]];
        uint8_t *q = &sheet[((size_t)Y * sw + X) * 3];
        for (int c = 0; c < 3; c++) q[c] = (uint8_t)(p[c] * 255 / 63);
    }
}
static void sheet_label(int x, int y, const char *s, int sc, int grey) {
    for (; *s; s++, x += 8 * sc)
        for (int r = 0; r < 8; r++) for (int c = 0; c < 8; c++)
            if (font_bios8[(uint8_t)*s][r] >> (7 - c) & 1)
                for (int j = 0; j < sc; j++) for (int i = 0; i < sc; i++) {
                    int X = x + c * sc + i, Y = y + r * sc + j;
                    if (X < sw && Y < sh_) { uint8_t *q = &sheet[((size_t)Y * sw + X) * 3]; q[0] = q[1] = (uint8_t)grey; q[2] = (uint8_t)(grey * 9 / 10); }
                }
}

/* ---- the art the mock-ups borrow: each run's stone and floor, her hall's own ---- */
static uint8_t HALL[2][TS * TS], HFLOOR[TS * TS], HCEIL[TS * TS], TORCH[3][TS * TS], SIGN[TS * TS];
static void art_setup(void) {
    crawl_art_init();
    portrait_keep_runs();                                           /* each run's stone and floor, as the game keeps them */
    crawl_art_level(1);                                             /* her hall: always level 1's look (begin_shop) */
    memcpy(HALL[0], crawl_tex[T_MATV(M1_ASHLAR, 0)], TS * TS); memcpy(HALL[1], crawl_tex[T_MATV(M1_ASHLAR, 1)], TS * TS);
    memcpy(HFLOOR, crawl_tex[T_FLOORS + FL1_FLAGS], TS * TS); memcpy(HCEIL, crawl_tex[T_CEILS + CE1_VAULT], TS * TS);
    for (int k = 0; k < 3; k++) memcpy(TORCH[k], crawl_tex[T_FEAT + F1_TORCH + k], TS * TS);
    memcpy(SIGN, crawl_tex[T_SIGN], TS * TS);
    F.w = W; F.h = H; F.fade = 1.0f;
    memcpy(F.pal, crawl_pal, sizeof F.pal);
    ui_face_init();
    ui_ramps(0);
}

/* ---- the creatures, as the bestiary has them ---- */
typedef struct { const char *name; int spr[4], family, eye, ranks[6], nranks; } Kindish;
enum { K_SKEL, K_BAT, K_SHAMB, K_SHIELD, K_DROWN, K_HOUND, K_GHOUL, K_KNIGHT, K_DEACON, K_GARG, NK };
static const Kindish KD[NK] = {                         /* eye: the kind's own (render_eyes' EYE_OWN), as a core and glow */
    { "SKELETONS", { SP_SKEL_A, SP_SKEL_B, SP_SKEL_SA, SP_SKEL_SB }, PF_BONE, 0, { RK_I, RK_II, RK_III, RK_IV, RK_V, RK_GILDED }, 6 },
    { "BATS", { SP_BAT_A, SP_BAT_B, SP_BAT_SA, SP_BAT_SB }, PF_WING, 3, { RK_I, RK_II, RK_III, RK_IV }, 4 },
    { "SHAMBLERS", { SP_SHAMB_A, SP_SHAMB_B, SP_SHAMB_SA, SP_SHAMB_SB }, PF_FLESH, 2, { RK_I, RK_II, RK_III, RK_IV, RK_V, RK_GILDED }, 6 },
    { "SHIELD-BEARERS", { SP_SHIELD_A, SP_SHIELD_B, SP_SHIELD_SA, SP_SHIELD_SB }, PF_BONE, 0, { RK_I, RK_II, RK_III, RK_IV, RK_V, RK_GILDED }, 6 },
    { "DROWNED", { SP_DROWN_A, SP_DROWN_B, SP_DROWN_SA, SP_DROWN_SB }, PF_DROWN, 1, { RK_I, RK_II, RK_III, RK_IV, RK_V, RK_GILDED }, 6 },
    { "BONE HOUNDS", { SP_HOUND_A, SP_HOUND_B, SP_HOUND_SA, SP_HOUND_SB }, PF_BONE, 0, { RK_I, RK_II, RK_III, RK_IV, RK_V }, 5 },
    { "GHOULS", { SP_GHOUL_A, SP_GHOUL_B, SP_GHOUL_SA, SP_GHOUL_SB }, PF_FLESH, 2, { RK_I, RK_II, RK_III, RK_IV, RK_V, RK_GILDED }, 6 },
    { "SKELETON KNIGHTS", { SP_KNIGHT_A, SP_KNIGHT_B, SP_KNIGHT_SA, SP_KNIGHT_SB }, PF_ARMOUR, 5, { RK_I, RK_II, RK_III, RK_IV, RK_V, RK_GILDED }, 6 },
    { "DEACONS", { SP_DEACON_A, SP_DEACON_B, SP_DEACON_SA, SP_DEACON_SB }, PF_ROBE, 6, { RK_I, RK_II, RK_III, RK_IV, RK_V, RK_GILDED }, 6 },
    { "GARGOYLES", { SP_GARG_A, SP_GARG_B, SP_GARG_SA, SP_GARG_SB }, PF_STONE, 4, { RK_I, RK_II, RK_III, RK_IV, RK_V }, 5 },
};
/* the eyes' colours (render_eyes): each kind's own at rank I, the rank's from II */
static const uint8_t OWN_CORE[7] = { R_FLAME + 12, R_GLOW + 15, R_FLAME + 15, R_BLOOD + 15, C_WHITE, R_BLOOD + 15, C_WHITE };
static const uint8_t OWN_GLOW[7] = { R_FLAME + 7, R_GLOW + 10, R_MOSS + 13, R_BLOOD + 10, R_FLAME + 12, R_BLOOD + 11, R_GHOST + 11 };
static const uint8_t RANK_CORE[NRANK] = { 0, R_MOSS + 15, R_GLOW + 15, R_FLAME + 8, C_WHITE, R_GOLD + 12 };
static const uint8_t RANK_GLOW[NRANK] = { 0, R_MOSS + 11, R_GLOW + 11, R_BLOOD + 13, R_PURPLE + 14, R_GOLD + 8 };
static PortraitView pv_of(int k, int rank, int frame, int run, int layout, unsigned unlocked, unsigned unpaid, float veil, float t) {
    PortraitView v;
    memset(&v, 0, sizeof v);
    const Kindish *K = &KD[k];
    memcpy(v.spr, K->spr, sizeof v.spr);
    v.family = K->family; v.rank = rank; v.frame = frame; v.layout = layout;
    v.eye_core = rank <= RK_I ? OWN_CORE[K->eye] : RANK_CORE[rank]; v.eye_glow = rank <= RK_I ? OWN_GLOW[K->eye] : RANK_GLOW[rank];
    v.wall = portrait_run_wall(run); v.floor = portrait_run_floor(run);
    memcpy(v.ranks, K->ranks, sizeof v.ranks); v.nranks = K->nranks;
    v.unlocked = (int)unlocked; v.unpaid = (int)unpaid; v.veil = veil; v.t = t;
    return v;
}

/* ---- her north wall, straight on: faces a cell each, the floor and ceiling going away, lit as the crypt lights a cell (a
 * light's power over 1 + 0.9 d squared, out to its reach, on her room's 0.06) and, on a portrait's own face, its candles'
 * glow as well ---- */
static const uint8_t BAY[4][4] = { { 0, 8, 2, 10 }, { 12, 4, 14, 6 }, { 3, 11, 1, 9 }, { 15, 7, 13, 5 } };
enum { FC_STONE, FC_PORTRAIT, FC_TORCH, FC_SIGN, FC_OVERLAY, FC_PORTAL };   /* OVERLAY: drawn already (half a boss's) */
#define MAXF 12
#define OVR 192                                          /* the largest overlay: three a texel */
typedef struct { int kind, mat, res; PortraitView pv, pv2; float mix; uint8_t ov[OVR * OVR]; int nfl; float fl[4][3]; } Face;   /* mix: a
                                                                      dissolve to pv2 */
static Face FACES[MAXF];
static int NF, FX0;                                     /* how many, and the world x of the first */
static float AMB = 0.06f, GLOW = 1.0f;
typedef struct { float x, y, z, power, reach, glow; } Lamp;   /* glow: a portrait's candle's, flickering; its cell's light steady */
static Lamp LAMPS[48], XL[16];
static int NL, NXL;                                     /* XL: lamps of the faces drawn already, kept across faces_prepare */
static void faces_begin(int x0, int n) { memset(FACES, 0, sizeof FACES); FX0 = x0; NF = n; NXL = 0; for (int i = 0; i < n; i++) { FACES[i].mat = (x0 + i) & 1; FACES[i].res = TS; } }
static void face_portrait(int wx, PortraitView pv, int res) { Face *f = &FACES[wx - FX0]; f->kind = FC_PORTRAIT; f->pv = pv; f->res = res; }
static void faces_prepare(float t) {
    NL = 0;
    for (int i = 0; i < NF; i++) {
        Face *f = &FACES[i];
        if (f->kind == FC_TORCH) {
            float fl = 1.0f + 0.10f * sinf(t * 7.3f + (float)i) + 0.05f * sinf(t * 17.0f + (float)i * 3.0f);
            LAMPS[NL++] = (Lamp){ (float)(FX0 + i) + 0.5f, 0.08f, 0.62f, 1.35f * fl, 4.6f, 0.0f };
        }
        if (f->kind != FC_PORTRAIT) continue;
        memset(f->ov, C_CLEAR, sizeof f->ov);
        f->pv.t = t;
        int s = f->res / TS;
        portrait_face(f->ov, f->res, f->res, f->res, 0, 0, s, &f->pv);
        f->nfl = portrait_flames(&f->pv, f->fl);
        if (f->mix > 0.0f) {                                        /* the next rank coming through, dithered */
            static uint8_t o2[OVR * OVR];
            memset(o2, C_CLEAR, sizeof o2);
            f->pv2.t = t;
            portrait_face(o2, f->res, f->res, f->res, 0, 0, s, &f->pv2);
            for (int y = 0; y < f->res; y++) for (int x = 0; x < f->res; x++)
                if ((float)BAY[(y / s) & 3][(x / s) & 3] < f->mix * 16.0f) f->ov[y * f->res + x] = o2[y * f->res + x];
            if (f->mix > 0.5f) f->nfl = portrait_flames(&f->pv2, f->fl);
        }
        for (int k = 0; k < f->nfl && NL < 48; k++)
            LAMPS[NL++] = (Lamp){ (float)(FX0 + i) + f->fl[k][0] / TS, 0.15f, 1.0f - f->fl[k][1] / TS, 0.45f / (float)(f->nfl > 1 ? 2 : 1), 2.4f, f->fl[k][2] };
    }
    for (int i = 0; i < NXL && NL < 48; i++) LAMPS[NL++] = XL[i];
}
static float light_at(float x, float y, float z, int wallface) {
    float l = AMB;
    for (int i = 0; i < NL; i++) {
        const Lamp *L = &LAMPS[i];
        float dd = hypotf(x - L->x, y - L->y);
        if (dd >= L->reach) continue;
        l += L->power / (1.0f + 0.9f * dd * dd) * (1.0f - smooth(L->reach * 0.55f, L->reach, dd));
        if (wallface && L->glow > 0.0f) {                          /* a portrait's candle on the wall by it: its glow */
            float d3 = hypotf(x - L->x, (z - L->z) * 1.1f), rg = 0.8f;
            if (d3 < rg) l += GLOW * 1.5f * L->power * L->glow * (1.0f - d3 / rg) * (1.0f - d3 / rg);
        }
    }
    return l;
}
static uint8_t shade(uint8_t c, float l, int x, int y) {
    if (IS_FULLBRIGHT(c)) return c;
    int lv = (int)(l * LIGHT_UNIT + BAY[y & 3][x & 3] * (1.0f / 16.0f));
    return crawl_cmap[clampi_(lv, 0, NLIGHT - 1)][c];
}
static void view_wall(float d, float cx, float t) {
    faces_prepare(t);
    float hz = H * 0.5f, top = hz - FOCAL * 0.5f / d, bot = hz + FOCAL * 0.5f / d;
    int tf = (int)(t * 8.0f);
    for (int y = 0; y < H; y++)
        for (int x = 0; x < W; x++) {
            float sx = (float)x + 0.5f - W * 0.5f, sy = (float)y + 0.5f;
            uint8_t c; float l;
            if (sy >= top && sy < bot) {                            /* the wall */
                float wx = cx + sx * d / FOCAL, v = (sy - top) / (bot - top);
                int fi = (int)floorf(wx) - FX0;
                float u = wx - floorf(wx);
                Face *f = fi >= 0 && fi < NF ? &FACES[fi] : NULL;
                int mi = f ? f->mat : ((int)floorf(wx) & 1);
                c = HALL[mi][clampi_((int)(u * TS), 0, TS - 1) * TS + clampi_((int)(v * TS), 0, TS - 1)];
                if (f && f->kind == FC_PORTAL) c = crawl_tex[T_PORTAL0 + (tf / 2) % 4][clampi_((int)(u * TS), 0, TS - 1) * TS + clampi_((int)(v * TS), 0, TS - 1)];
                else if (f && (f->kind == FC_PORTRAIT || f->kind == FC_OVERLAY)) {
                    uint8_t o = f->ov[clampi_((int)(v * f->res), 0, f->res - 1) * f->res + clampi_((int)(u * f->res), 0, f->res - 1)];
                    if (o != C_CLEAR) c = o;
                } else if (f && f->kind == FC_TORCH) {
                    uint8_t o = TORCH[(tf + fi) % 3][clampi_((int)(u * TS), 0, TS - 1) * TS + clampi_((int)(v * TS), 0, TS - 1)];
                    if (o != C_CLEAR) c = o;
                } else if (f && f->kind == FC_SIGN) c = SIGN[clampi_((int)(u * TS), 0, TS - 1) * TS + clampi_((int)(v * TS), 0, TS - 1)];
                l = light_at(wx, 0.04f, 1.0f - v, 1);
            } else {                                                /* the floor and the ceiling, going away */
                int fl = sy >= bot;
                float z = FOCAL * 0.5f / fabsf(sy - hz), wx = cx + sx * z / FOCAL, wy = d - z;
                if (wy < 0.0f) wy = 0.0f;
                int tu = (int)((wx - floorf(wx)) * TS) & 63, tv = (int)((wy - floorf(wy)) * TS) & 63;
                c = fl ? HFLOOR[tu * TS + tv] : HCEIL[tu * TS + tv];
                l = light_at(wx, wy + 0.02f, fl ? 0.0f : 1.0f, 0) * (fl ? 1.0f : 0.8f);
            }
            F.pix[y * W + x] = shade(c, l, x, y);
        }
}

/* ---- the mock-ups ---- */
static void gif_run(const char *dir, const char *name, void (*fn)(float), float loop, int sc) {
    char p[512];
    snprintf(p, sizeof p, "%s/%s.gif", dir, name);
    if (!gif_begin(p, sc)) { fprintf(stderr, "can't write %s\n", p); return; }
    int n = (int)(loop * FPS + 0.5f);
    for (int i = 0; i < n; i++) { fn((float)i / FPS); gif_frame(); }
    gif_end();
    printf("%s: %d frames\n", p, n);
}
static int g_res = 128;
/* the skeleton, ranks I and II unlocked: five seconds of each walking (and turning about, as in the bestiary), each on the
 * run it was unlocked in, a dissolve between; the frame green for II, its highest; II's gold waiting */
static void skel_at(float t) {
    faces_begin(13, 7);
    float tl = fmodf(t, 10.0f);
    int second = tl >= 5.0f;
    unsigned un = 1u << RK_I | 1u << RK_II, wait = 1u << RK_II;
    PortraitView a = pv_of(K_SKEL, RK_I, RK_II, 0, PL_SCONCE_R, un, wait, 0.0f, t), b = pv_of(K_SKEL, RK_II, RK_II, 1, PL_SCONCE_R, un, wait, 0.0f, t);
    a.walk0 = 0.0f; b.walk0 = 5.0f;
    face_portrait(16, second ? b : a, g_res);
    Face *f = &FACES[16 - FX0];
    float u = second ? tl - 5.0f : tl;                              /* a dissolve over the last 0.4 s of each */
    if (u > 4.6f) { f->pv2 = second ? a : b; f->pv2.walk0 = second ? 10.0f : 5.0f; f->mix = smooth(4.6f, 5.0f, u); }
    view_wall(FOCAL / 128.0f, 16.5f, t);
}
/* the tapestry: hung over it, a prompt; drawn back; one slain, so its shadow walks there */
static void veil_at(float t) {
    faces_begin(13, 7);
    float k = 1.0f - smooth(1.6f, 2.9f, t);                     /* gathered aside, and tied there */
    PortraitView v = pv_of(K_SHAMB, -1, -1, 0, PL_PAIR, 0, 0, k, t);
    v.walk0 = 2.2f;
    face_portrait(16, v, g_res);
    view_wall(FOCAL / 128.0f, 16.5f, t);
    if (t < 1.6f) {                                                 /* the prompt, as the crypt words it */
        const char *s = "E DRAW BACK THE TAPESTRY";
        int w = ui_keys_width(s), x = (W - w) / 2;
        ui_band(&F, x - 6, 178, w + 12, 14);
        ui_keys(&F, x, 181, s, R_STONE + 13);
    }
}
/* her north wall from the middle of her room, where the second third hangs: the torches, the sign, four portraits in four states */
static void room_at(float t) {
    faces_begin(11, 9);
    FACES[12 - FX0].kind = FC_TORCH; FACES[18 - FX0].kind = FC_TORCH; FACES[15 - FX0].kind = FC_SIGN;
    PortraitView a = pv_of(K_DROWN, RK_II, RK_II, 1, PL_SCONCE_L, 3, 2, 0.0f, t);
    PortraitView b = pv_of(K_HOUND, RK_I, -1, 1, PL_LEDGE, 0, 0, 1.0f, t);
    PortraitView c = pv_of(K_KNIGHT, RK_IV, RK_IV, 3, PL_LAMP, 15, 0, 0.0f, t);
    PortraitView d = pv_of(K_GHOUL, -1, -1, 1, PL_PAIR, 0, 0, 0.0f, t);
    a.walk0 = 0.0f; c.walk0 = 0.8f; d.walk0 = 2.4f;
    face_portrait(13, a, g_res); face_portrait(14, b, g_res); face_portrait(16, c, g_res); face_portrait(17, d, g_res);
    view_wall(3.6f, 15.5f, t);
}

/* ---- the sheets: stills, each a portrait seen straight on from a cell and two thirds, a label over it ---- */
#define TW 200
#define TH 176
static void tile_at(int sx, int sy, PortraitView v, const char *label, const char *label2) {
    faces_begin(13, 7);
    face_portrait(16, v, g_res);
    view_wall(FOCAL / 128.0f, 16.5f, v.t);
    sheet_put(sx, sy + 40, (W - TW) / 2, (H - TH) / 2, TW, TH, 2);
    sheet_label(sx, sy, label, 2, 220);
    if (label2) sheet_label(sx, sy + 20, label2, 2, 150);
}
static void sheet_begin(int cols, int rows) {
    sw = cols * (TW * 2 + 24) + 24; sh_ = rows * (TH * 2 + 64) + 72;
    free(sheet); sheet = calloc((size_t)sw * sh_, 3);
    for (size_t i = 0; i < (size_t)sw * sh_ * 3; i++) sheet[i] = 18;
}
static void sheet_end(const char *dir, const char *name) {
    char p[512];
    snprintf(p, sizeof p, "%s/%s.png", dir, name);
    write_png(p, sheet, sw, sh_);
    printf("%s\n", p);
}
#define CX(c) (24 + (c) * (TW * 2 + 24))
#define RY(r) (72 + (r) * (TH * 2 + 64))
static void sheet_states(const char *dir) {
    sheet_begin(4, 3);
    sheet_label(24, 20, "THE TAPESTRY, THEN THE FRAME BY THE HIGHEST RANK UNLOCKED", 3, 235);
    float t = 1.15f;
    unsigned all = 0;
    tile_at(CX(0), RY(0), pv_of(K_SKEL, -1, -1, 0, PL_SCONCE_R, 0, 0, 1.0f, t), "NONE SLAIN", "THE TAPESTRY OVER IT");
    tile_at(CX(1), RY(0), pv_of(K_SKEL, -1, -1, 0, PL_SCONCE_R, 0, 0, 0.5f, t), "ONE SLAIN", "E DRAWS IT BACK");
    tile_at(CX(2), RY(0), pv_of(K_SKEL, -1, -1, 0, PL_SCONCE_R, 0, 0, 0.0f, t), "ITS SHADOW", "BARE WOOD: NONE UNLOCKED");
    static const int RK[7] = { RK_I, RK_II, RK_III, RK_IV, RK_GILDED, RK_V }, RUN[7] = { 0, 1, 2, 3, 2, 3 };
    static const char *const NM[7] = { "RANK I: BONE", "RANK II: GREEN", "RANK III: BLUE", "RANK IV: RED", "GILDED: GOLD", "RANK V: BLACK" };
    static const char *const RN[4] = { "ON THE CRYPTS", "ON THE CATACOMBS", "ON THE MINE", "ON THE BLACK GLASS" };
    for (int i = 0; i < 6; i++) {
        all |= 1u << RK[i];
        int c = (i + 3) % 4, r = (i + 3) / 4;
        tile_at(CX(c), RY(r), pv_of(K_SKEL, RK[i], RK[i], RUN[i], PL_SCONCE_R, all, 1u << RK[i], 0.0f, t + 0.2f * (float)i), NM[i], RN[RUN[i]]);
    }
    sheet_end(dir, "states");
}
static void sheet_candles(const char *dir) {
    sheet_begin(4, 2);
    sheet_label(24, 20, "THE CANDLES: SEVEN WAYS, SO NO TWO NEIGHBOURS ARE ALIKE", 3, 235);
    static const int K[NPLAYOUT] = { K_SKEL, K_GHOUL, K_DEACON, K_KNIGHT, K_DROWN, K_HOUND, K_GARG };
    static const int RK[NPLAYOUT] = { RK_II, RK_I, RK_III, RK_IV, RK_II, RK_I, RK_III }, RUN[NPLAYOUT] = { 1, 1, 2, 3, 1, 0, 2 };
    for (int i = 0; i < NPLAYOUT; i++) {
        unsigned un = (2u << RK[i]) - 1u;
        tile_at(CX(i % 4), RY(i / 4), pv_of(K[i], RK[i], RK[i], RUN[i], i, un, 0, 0.0f, 0.7f + 0.37f * (float)i), PL_NAME[i], KD[K[i]].name);
    }
    sheet_end(dir, "candles");
}
static void sheet_runs(const char *dir) {
    sheet_begin(4, 2);
    sheet_label(24, 20, "BEHIND IT, THE RUN ITS RANK WAS UNLOCKED IN; THE WALLS' 64 AGAINST 128", 3, 235);
    static const char *const RN[4] = { "THE CRYPTS: ASHLAR", "THE CATACOMBS: SLATE", "THE MINE: GRANITE", "THE GLASS: OBSIDIAN" };
    for (int r = 0; r < 4; r++)
        tile_at(CX(r), RY(0), pv_of(K_SKEL, r, r, r, PL_SCONCE_R, (2u << r) - 1u, 0, 0.0f, 0.9f + 0.3f * (float)r), RN[r], NULL);
    for (int k = 0; k < 2; k++) {                                   /* the walls' own 64, and 128 */
        g_res = k ? 128 : 64;
        tile_at(CX(k), RY(1), pv_of(K_SHIELD, RK_IV, RK_IV, 3, PL_LAMP, 15, 0, 0.0f, 2.1f), k ? "128: FINER" : "64: THE WALLS' OWN", "1.7 CELLS OFF");
    }
    for (int k = 0; k < 2; k++) {                                   /* and from further off */
        g_res = k ? 128 : 64;
        faces_begin(13, 7);
        face_portrait(16, pv_of(K_SHIELD, RK_IV, RK_IV, 3, PL_LAMP, 15, 0, 0.0f, 2.1f), g_res);
        view_wall(3.2f, 16.5f, 2.1f);
        sheet_put(CX(2 + k), RY(1) + 40, (W - TW) / 2, (H - TH) / 2, TW, TH, 2);
        sheet_label(CX(2 + k), RY(1), k ? "128, FROM 3 CELLS" : "64, FROM 3 CELLS", 2, 220);
    }
    g_res = 128;
    sheet_end(dir, "runs");
}
/* a feat earned (step 5): its medallion on a ribbon under the frame, at the side away from the tapestry; on four layouts, and up
 * close */
static void sheet_medals(const char *dir) {
    sheet_begin(4, 2);
    sheet_label(24, 20, "A FEAT EARNED: A MEDALLION ON ITS RIBBON, UNDER THE FRAME", 3, 235);
    static const int K[4] = { K_SKEL, K_KNIGHT, K_GHOUL, K_DEACON }, L[4] = { PL_SCONCE_R, PL_LAMP, PL_LEDGE, PL_SKULL };
    static const int RK[4] = { RK_V, RK_IV, RK_II, RK_III }, RUN[4] = { 3, 3, 1, 2 };
    static const char *const NM[4] = { "IT NEVER THREW", "IN FULL PLATE", "AT ITS MEAL", "NONE RISE" };
    for (int i = 0; i < 4; i++) {
        unsigned un = ((2u << RK[i]) - 1u) | 1u << PORTRAIT_FEAT, wait = i == 0 ? 1u << PORTRAIT_FEAT : 0u;
        tile_at(CX(i), RY(0), pv_of(K[i], RK[i], RK[i], RUN[i], L[i], un, wait, 0.0f, 1.1f + 0.3f * (float)i), NM[i], i == 0 ? "ITS GOLD WAITING" : PL_NAME[L[i]]);
    }
    for (int i = 0; i < 2; i++) {                                   /* up close: the frame's foot, as the card shows it, three times */
        static uint8_t ov[128 * 128];
        int k = i * 2;
        PortraitView v = pv_of(K[k], RK[k], RK[k], RUN[k], L[k], ((2u << RK[k]) - 1u) | 1u << PORTRAIT_FEAT, 0, 0.0f, 1.1f);
        memset(ov, C_CLEAR, sizeof ov);
        portrait_face(ov, 128, 128, 128, 0, 0, 2, &v);
        float fl[4][3]; int nf = portrait_flames(&v, fl);
        for (int y = 0; y < H; y++) for (int x = 0; x < W; x++) F.pix[y * W + x] = 0;
        for (int y = 0; y < 64; y++) for (int x = 0; x < 128; x++) {
            int oy = 64 + y;
            uint8_t c = ov[oy * 128 + x], base = HALL[0][(x / 2) * TS + oy / 2];
            float l = 0.30f;
            for (int q = 0; q < nf; q++) { float d = hypotf((float)x / 2 - fl[q][0], (float)oy / 2 - fl[q][1]) / 64.0f; l += 0.9f * fl[q][2] / (1.0f + 30.0f * d * d); }
            F.pix[y * W + x] = shade(c != C_CLEAR ? c : base, l, x, y);
        }
        sheet_put(CX(i * 2), RY(1) + 40, 0, 0, 128, 64, 6);
        sheet_label(CX(i * 2), RY(1), i ? "UP CLOSE: THE LEDGE'S" : "UP CLOSE: A SCONCE'S", 2, 220);
    }
    sheet_end(dir, "medals");
}
/* ---- the bosses' portraits (step 5, a mock-up of the knight's for the owner): two faces wide on the grave plot's far wall, the
 * Accursed Portal between them ---- */
static BossPortraitView bv_of(int boss, unsigned un, unsigned wait, float veil, float t) {
    static const int SPR[4][5] = {                                  /* walking, raising its blow, striking, a pose of its own */
        { SP_BKNIGHT_A, SP_BKNIGHT_B, SP_BKNIGHT_W, SP_BKNIGHT_S, SP_BKNIGHT_P0 },
        { SP_BSPIDER_A, SP_BSPIDER_B, SP_BSPIDER_REAR, SP_BSPIDER_LEAP, SP_BSPIDER_REAR },
        { SP_BDEACON_A, SP_BDEACON_B, SP_BDEACON_CHANT, SP_BDEACON_CHANT, SP_BDEACON_A },
        { SP_WRAITH_A, SP_WRAITH_B, SP_WRAITH_REACH, SP_WRAITH_REACH, SP_WRAITH_A } };
    static const int PROP[4][2] = { { SP_PILLAR, SP_BROKEN_ARMS }, { SP_PILLAR, SP_PUDDLE_A }, { SP_TIMBER, SP_TOMB_BROKEN }, { SP_BRAZIER_A, SP_OBSIDIAN } };
    static const int EYE[4] = { 5, 3, 6, 1 };                      /* the knight's crimson, the spider's ember, the deacon's pale, the Wraith's cold */
    BossPortraitView v;
    memset(&v, 0, sizeof v);
    boss &= 3;
    memcpy(v.spr, SPR[boss], sizeof v.spr); v.prop[0] = PROP[boss][0]; v.prop[1] = PROP[boss][1]; v.prop_n = boss == 3 ? 3 : 1;
    v.eye_core = OWN_CORE[EYE[boss]]; v.eye_glow = OWN_GLOW[EYE[boss]];
    v.wall = portrait_run_wall(boss); v.floor = portrait_run_floor(boss);
    v.unlocked = (int)un; v.unpaid = (int)wait; v.veil = veil; v.t = t; v.scale = 1.25f;
    return v;
}
static void face_boss(int wx, BossPortraitView bv) {              /* drawn whole, then halved onto its two faces; its candles' light */
    static uint8_t ov[128 * 256];
    memset(ov, C_CLEAR, sizeof ov);
    portrait_boss_face(ov, 256, 256, 128, 0, 0, 2, &bv);
    for (int h = 0; h < 2; h++) {
        Face *f = &FACES[wx + h - FX0];
        f->kind = FC_OVERLAY; f->res = 128;
        for (int y = 0; y < 128; y++) memcpy(&f->ov[y * 128], &ov[y * 256 + h * 128], 128);
    }
    for (int k = 0; k < 2 && NXL < 16; k++)                         /* a stand of three either side */
        XL[NXL++] = (Lamp){ (float)wx + (k ? 121.0f : 7.0f) / TS, 0.15f, 1.0f - 25.0f / TS, 0.5f, 2.6f, 1.0f };
}
static void plot_wall(float t, const unsigned un[4], const unsigned wt[4], const float veil[4]) {   /* the far wall: the knight's and the
                                                                      spider's, the Portal, the Deacon's and the Wraith's */
    static const int WX[4] = { 12, 14, 17, 19 };
    faces_begin(10, 12);
    FACES[16 - FX0].kind = FC_PORTAL;
    XL[NXL++] = (Lamp){ 16.5f, 0.3f, 0.45f, 0.9f, 3.2f, 0.0f };     /* its swirl's glow */
    for (int b = 0; b < 4; b++) face_boss(WX[b], bv_of(b, un[b], wt[b], veil[b], t + 0.9f * (float)b));
}
static void knight_at(float t) {                                   /* the tapestries parting, then the knight in its lists */
    float k = 1.0f - smooth(1.4f, 2.7f, t);
    static const unsigned UN[4] = { 0x07u, 0, 0, 0 }, WT[4] = { 0x04u, 0, 0, 0 };
    float V[4] = { k, 1.0f, 1.0f, 1.0f };
    plot_wall(t - 2.7f, UN, WT, V);
    view_wall(1.9f, 13.0f, t);
}
static void sheet_bosses(const char *dir) {
    sw = 24 + 2 * (640 + 24); sh_ = 72 + 3 * (400 + 64);
    free(sheet); sheet = calloc((size_t)sw * sh_, 3);
    for (size_t i = 0; i < (size_t)sw * sh_ * 3; i++) sheet[i] = 18;
    sheet_label(24, 20, "THE BOSSES' FOUR, BY THE PORTAL", 3, 235);
    static const unsigned UN[4] = { 0x3Fu, 0x01u, 0x0Fu, 0x07u }, WT[4] = { 0x00u, 0x01u, 0x08u, 0x04u };
    static const float CX4[4] = { 13.0f, 15.0f, 18.0f, 20.0f };
    static const char *const L1[4] = { "THE ILL-FATED KNIGHT", "THE WANDERING SPIDER", "THE BLACK DEACON", "THE WRAITH UNBOUND" };
    static const char *const L2[4] = { "EVERY STEP: BLACK", "SLAIN: BONE, ITS GOLD WAITING", "THREE STEPS: RED, THE THIRD'S WAITING", "TWO STEPS: BLUE" };
    float V[4] = { 0, 0, 0, 0 };
    for (int i = 0; i < 4; i++) {
        int x = 24 + (i & 1) * (640 + 24), y = 72 + (i >> 1) * (400 + 64);
        plot_wall(1.3f + 0.4f * (float)i, UN, WT, V);
        view_wall(2.1f, CX4[i], 1.3f + 0.4f * (float)i);
        sheet_put(x, y + 40, 0, 0, W, H, 2);
        sheet_label(x, y, L1[i], 2, 220); sheet_label(x, y + 20, L2[i], 2, 150);
    }
    int y = 72 + 2 * (400 + 64);
    plot_wall(2.0f, UN, WT, V);
    view_wall(6.2f, 16.0f, 2.0f);
    sheet_put(24, y + 40, 0, 0, W, H, 2);
    sheet_label(24, y, "THE PLOT'S FAR WALL, FROM ACROSS IT", 2, 220); sheet_label(24, y + 20, "ALL FOUR DRAWN BACK", 2, 150);
    static const unsigned U0[4] = { 0x01u, 0, 0, 0 }, W0[4] = { 0, 0, 0, 0 };
    float V0[4] = { 0.0f, 1.0f, 1.0f, 1.0f };
    plot_wall(2.0f, U0, W0, V0);
    view_wall(3.4f, 15.0f, 2.0f);
    sheet_put(24 + 640 + 24, y + 40, 0, 0, W, H, 2);
    sheet_label(24 + 640 + 24, y, "THE KNIGHT SLAIN, THE REST NOT YET", 2, 220); sheet_label(24 + 640 + 24, y + 20, "THEIR TAPESTRIES DRAWN", 2, 150);
    sheet_end(dir, "bosses");
}
/* the card that E opens before it: the portrait up close in its niche, and its ranks with what each has cost and pays */
static void card(const char *dir) {
    room_at(1.0f);
    for (int i = 0; i < W * H; i++) F.pix[i] = crawl_cmap[9][F.pix[i]];   /* the room dimmed under it */
    ui_slab(&F, 6, 8, 308, 184, SLAB_TABLET, ORN_CANDLE, 1.0f);
    ui_text16(&F, 16, 14, "SKELETONS", FACE_INSC, R_GOLD + 12, 0, 1);
    ui_text8(&F, 304 - ui_keys_width("RANK II") - 0, 18, "RANK II", FACE_BODY, R_MOSS + 13, 0);
    ui_well(&F, 12, 34, 124, 140);
    static uint8_t ov[128 * 128];                                   /* the portrait, drawn straight onto the card */
    memset(ov, C_CLEAR, sizeof ov);
    PortraitView v = pv_of(K_SKEL, RK_II, RK_II, 1, PL_SCONCE_R, 3, 2, 0.0f, 1.0f);
    portrait_face(ov, 128, 128, 128, 0, 0, 2, &v);
    float fl[4][3]; int nf = portrait_flames(&v, fl);
    for (int y = 0; y < 128; y++) for (int x = 0; x < 118; x++) {
        uint8_t c = ov[y * 128 + x + 2], base = HALL[0][((x + 2) / 2) * TS + y / 2];
        float l = 0.30f;
        for (int k = 0; k < nf; k++) { float d = hypotf((float)(x + 2) / 2 - fl[k][0], (float)y / 2 - fl[k][1]) / 64.0f; l += 0.9f * fl[k][2] / (1.0f + 30.0f * d * d); }
        F.pix[(40 + y) * W + 15 + x] = shade(c != C_CLEAR ? c : base, l, x, y);
    }
    ui_well(&F, 140, 34, 168, 104);
    static const char *const RN[6] = { "I", "II", "III", "IV", "V", "G" };
    static const char *const HAVE[6] = { "60/60", "40/40", "13/25", " 2/15", " 0/8", " 1/3" };
    static const char *const GOLD[6] = { " 250", " 440", " 630", " 810", "1000", " 910" };
    static const char *const STATE[6] = { "TAKEN", "WAITS", "", "", "", "" };
    static const int RC[6] = { R_BONE + 13, R_MOSS + 13, R_GHOST + 13, R_BLOOD + 13, R_IRON + 11, R_GOLD + 11 };
    ui_text8(&F, 146, 39, "RANK SLAIN  GOLD", FACE_BODY, R_GOLD + 10, 0);
    for (int i = 0; i < 6; i++) {
        int y = 53 + i * 13, on = i < 2;
        if (i == 1) ui_row(&F, 142, y - 3, 164, 13, 1);
        ui_text8(&F, 146, y, RN[i], FACE_BODY, on ? RC[i] : R_STONE + 9, 0);
        ui_text8(&F, 178, y, HAVE[i], FACE_BODY, on ? (int)C_WHITE : R_STONE + 11, 0);
        ui_text8(&F, 226, y, GOLD[i], FACE_BODY, on ? R_GOLD + 12 : R_STONE + 8, 0);
        if (STATE[i][0]) ui_text8(&F, 266, y, STATE[i], FACE_BODY, i == 1 ? R_GOLD + 12 : R_STONE + 10, 0);
    }
    ui_wrap(&F, 144, 144, "EACH RANK PAYS ONCE A LIFE, AFTER A LEVEL IS CLEARED.", 20, 10, 3, FACE_BODY, R_STONE + 12, 0);
    const char *keys = "E TAKE 440  A/D ANOTHER  ESC LEAVE";
    ui_keys(&F, (W - ui_keys_width(keys)) / 2, 179, keys, R_STONE + 12);
    char p[512];
    snprintf(p, sizeof p, "%s/card.png", dir);
    sw = W * 2; sh_ = H * 2; free(sheet); sheet = calloc((size_t)sw * sh_, 3);
    sheet_put(0, 0, 0, 0, W, H, 2);
    write_png(p, sheet, sw, sh_);
    printf("%s\n", p);
}
static void still(const char *dir, const char *name, void (*fn)(float), float t) {
    char p[512];
    fn(t);
    snprintf(p, sizeof p, "%s/%s.png", dir, name);
    sw = W * 2; sh_ = H * 2; free(sheet); sheet = calloc((size_t)sw * sh_, 3);
    sheet_put(0, 0, 0, 0, W, H, 2);
    write_png(p, sheet, sw, sh_);
    printf("%s\n", p);
}

int main(int argc, char **argv) {
    if (argc < 2) { fprintf(stderr, "usage: portraitmock <dir> [all|skeleton|veil|room|sheets|card|medals|bosses|bench|check]\n"); return 2; }
    art_setup();
    const char *which = argc > 2 ? argv[2] : "all";
    int all = !strcmp(which, "all"), sc = getenv("PM_SCALE") ? atoi(getenv("PM_SCALE")) : 3;
    if (all || !strcmp(which, "skeleton")) {
        gif_run(argv[1], "skeleton", skel_at, 10.0f, sc);
        g_res = 64; gif_run(argv[1], "skeleton_64", skel_at, 10.0f, sc); g_res = 128;
    }
    if (all || !strcmp(which, "veil")) gif_run(argv[1], "tapestry", veil_at, 8.0f, sc);
    if (all || !strcmp(which, "room")) { gif_run(argv[1], "room", room_at, 6.4f, sc); still(argv[1], "room", room_at, 1.0f); }
    if (all || !strcmp(which, "sheets")) { sheet_states(argv[1]); sheet_candles(argv[1]); sheet_runs(argv[1]); }
    if (all || !strcmp(which, "card")) card(argv[1]);
    if (all || !strcmp(which, "medals")) sheet_medals(argv[1]);
    if (all || !strcmp(which, "bosses")) { sheet_bosses(argv[1]); gif_run(argv[1], "knight", knight_at, 10.7f, sc); }
    if (!strcmp(which, "check")) {                                      /* the runs' stone made alone, against the whole art */
        int bad = 0;
        for (int r = 0; r < 4; r++) {
            static uint8_t w[TS * TS], fl[TS * TS];
            static const int MAT[4] = { M1_ASHLAR, M2_SLATE, M3_GRANITE, M4_OBSIDIAN };
            crawl_art_level(5 + 10 * r);
            crawl_art_stone(5 + 10 * r, w, fl);
            crawl_art_level(5 + 10 * r);
            int dw = 0, df = 0;
            for (int i = 0; i < TS * TS; i++) { dw += w[i] != crawl_tex[T_MATV(MAT[r], 0)][i]; df += fl[i] != crawl_tex[T_FLOORS][i]; }
            printf("run %d: stone %d texels apart, floor %d\n", r + 1, dw, df);
            bad += dw + df;
        }
        return bad != 0;
    }
    if (!strcmp(which, "bench")) {                                      /* what a portrait costs to draw, at 128 and at 64 */
        static uint8_t ov[OVR * OVR];
        for (int res = 128; res >= 64; res /= 2)
            for (int part = 0; part < 3; part++) {
                int n = 0; clock_t c0 = clock();
                for (; n < 400; n++) {
                    PortraitView v = pv_of(n % NK, part == 0 ? -1 : RK_II, RK_II, n & 3, n % NPLAYOUT, 3, 2, part == 2 ? 1.0f : 0.0f, (float)n * 0.05f);
                    memset(ov, C_CLEAR, sizeof ov);
                    portrait_face(ov, res, res, res, 0, 0, res / TS, &v);
                }
                printf("bench %d texels, %s: %.3f ms a portrait\n", res, part == 0 ? "its shadow" : part == 1 ? "a rank, drawn back" : "veiled",
                       (double)(clock() - c0) * 1000.0 / CLOCKS_PER_SEC / n);
            }
    }
    if (!strcmp(which, "s") && argc > 3) {                              /* one still of the skeleton's, for looking at */
        still(argv[1], "s_skeleton", skel_at, (float)atof(argv[3]));
    }
    return 0;
}
