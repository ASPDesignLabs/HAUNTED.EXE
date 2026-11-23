/* The unlit torch's sheet (docs/PLAN-events.md, step 2): the sconce every drawn run hangs on its walls, as the game draws it lit,
 * beside what an event makes of it: guttering, just snuffed, and cold. Each row is a run, on that run's own wall and palette; the
 * lit frames sit in a torch's light, the others in the dark it leaves. Nothing here is in the game until the owner has seen it.
 *
 *   build/torchmock out.png
 */
#include "crawl.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>

#define SC 3                                /* sheet pixels per texel */
#define CELL (TS * SC)
#define PAD 10
#define TXT 2
#define HEAD 150
#define ROWLAB 150
#define NCOL 7
#define LIT_LIGHT 26                        /* a wall by a burning torch, as sheet.c shows sprites */
#define DARK_LIGHT 9                        /* the same wall with it out */

static int iw, ih;
static uint8_t *img;
static void put(int x, int y, int r, int g, int b) { if (x < 0 || y < 0 || x >= iw || y >= ih) return; uint8_t *p = img + ((size_t)y * iw + x) * 3; p[0] = r; p[1] = g; p[2] = b; }
static void rect(int x, int y, int w, int h, int r, int g, int b) { for (int yy = y; yy < y + h; yy++) for (int xx = x; xx < x + w; xx++) put(xx, yy, r, g, b); }
static void text(int x, int y, const char *s, int r, int g, int b) {
    for (; *s; s++, x += 8 * TXT)
        for (int row = 0; row < 8; row++)
            for (int col = 0; col < 8; col++)
                if (font_bios8[(uint8_t)*s][row] >> (7 - col) & 1) rect(x + col * TXT, y + row * TXT, TXT, TXT, r, g, b);
}

static uint8_t shd(int ramp, float v) { int k = (int)(v * 15.0f + 0.5f); return (uint8_t)(ramp + (k < 0 ? 0 : k > 15 ? 15 : k)); }
static float vnoise(float x, float y, uint32_t seed) {          /* value noise, smooth enough for a flame's edge */
    int ix = (int)floorf(x), iy = (int)floorf(y); float fx = x - ix, fy = y - iy;
    #define HH(a, b) ((float)(((uint32_t)(a) * 374761393u + (uint32_t)(b) * 668265263u + seed * 2246822519u) * 3266489917u >> 8 & 0xFFFF) / 65535.0f)
    float a = HH(ix, iy), b = HH(ix + 1, iy), c = HH(ix, iy + 1), d = HH(ix + 1, iy + 1);
    #undef HH
    fx = fx * fx * (3 - 2 * fx); fy = fy * fy * (3 - 2 * fy);
    return a + (b - a) * fx + (c - a) * fy + (a - b - c + d) * fx * fy;
}

/* What an event makes of the lit sconce (T_FEAT + F1_TORCH): the same iron and wood, the flame taken away; then a small flame
 * (guttering), embers and a thread of smoke (just snuffed), or a charred head and nothing (cold). Column-major, as the game's. */
enum { V_GUTTER0, V_GUTTER1, V_SNUFFED, V_COLD };
static void make_unlit(const uint8_t *lit, uint8_t *out, int v) {
    memcpy(out, lit, TS * TS);
    for (int i = 0; i < TS * TS; i++) if (out[i] >= R_FLAME && out[i] < R_FLAME + 16) out[i] = C_CLEAR;
    for (int y = 19; y <= 23; y++)                                  /* the head, burnt: charcoal, a crack of lighter ash */
        for (int x = 30; x <= 33; x++) {
            uint8_t c = shd(R_IRON, 0.06f + 0.05f * ((x + y) & 1));
            if (y == 19 && (x == 30 || x == 33)) continue;          /* its rounded top */
            if ((x == 31 && y == 21) || (x == 32 && y == 22)) c = shd(R_STONE, 0.35f);
            out[x * TS + y] = c;
        }
    if (v == V_SNUFFED) {
        static const int EMB[][2] = { { 31, 19 }, { 32, 20 }, { 30, 21 }, { 33, 22 } };
        for (int k = 0; k < 4; k++) out[EMB[k][0] * TS + EMB[k][1]] = (uint8_t)(R_FLAME + 5 + (k & 1) * 3);
        for (int y = 2; y <= 18; y++) {                             /* smoke, rising and leaning, thinning as it goes */
            float u = (18 - y) / 16.0f, cx = 31.5f + 2.2f * sinf(y * 0.42f) * u;
            for (int x = 27; x <= 37; x++) {
                float d = fabsf(x + 0.5f - cx);
                if (d > 1.1f + u * 1.4f || ((x + y) & 1) == (y > 10)) continue;
                if (u > 0.5f && ((x * 7 + y * 3) % 5) < 2) continue;
                out[x * TS + y] = shd(R_STONE, 0.62f - 0.25f * u);
            }
        }
    }
    if (v == V_GUTTER0 || v == V_GUTTER1) {                         /* a low flame, blue at the root, as a torch near out */
        float tall = v == V_GUTTER0 ? 0.42f : 0.55f, sway = v == V_GUTTER0 ? 1.0f : -0.8f;
        for (int y = 6; y <= 20; y++)
            for (int x = 24; x <= 40; x++) {
                float u = (20 - y) / (14.0f * tall);
                if (u > 1.0f) continue;
                float cx = 31.5f + sway * u * u * 2.0f;
                float w = 3.6f * powf(fmaxf(0.0f, sinf(3.14159f * (0.18f + 0.82f * (1.0f - u)))), 0.8f) * (1.0f - 0.35f * u);
                if (w < 0.3f) continue;
                float d = fabsf(x + 0.5f - cx) / (w + 0.01f), n = vnoise(x / 3.0f, y / 3.0f + v * 5.0f, 77u) * 0.5f;
                if (d > 1.0f - 0.25f * n) continue;
                int heat = (int)((0.75f - d * 0.6f - u * 0.5f + n * 0.3f) * 15.0f);
                out[x * TS + y] = u < 0.18f && d < 0.6f ? shd(R_GHOST, 0.75f) : (uint8_t)(R_FLAME + (heat < 1 ? 1 : heat > 11 ? 11 : heat));
            }
    }
}

/* The other runs' lights (the Catacombs' lantern, the Hollows' lamp, the Deep's seam of fire) keep their own shapes: the flame's
 * hottest pixels stay, low, for guttering; the rest go to what the light leaves behind: dark glass where the flame is held between
 * metal, the bare wall where it stood in the air, and for the Deep a cooled crack with a dull red vein. */
static void make_unlit_run(const uint8_t *lit, uint8_t *out, int v, int run) {
    memcpy(out, lit, TS * TS);
    int top = TS, bx = 0, nb = 0;
    for (int x = 0; x < TS; x++) for (int y = 0; y < TS; y++) { uint8_t c = lit[x * TS + y]; if (c >= R_FLAME && c < R_FLAME + 16) { if (y < top) top = y; bx += x; nb++; } }
    if (!nb) return;
    bx /= nb;
    for (int x = 0; x < TS; x++)
        for (int y = 0; y < TS; y++) {
            uint8_t c = lit[x * TS + y];
            if (c < R_FLAME || c >= R_FLAME + 16) continue;
            int h = c - R_FLAME;
            int keep = (v == V_GUTTER0 && h >= 12) || (v == V_GUTTER1 && h >= 10);
            if (keep) { out[x * TS + y] = (uint8_t)(R_FLAME + h - (v == V_GUTTER0 ? 7 : 5)); continue; }
            if (run == 3) { out[x * TS + y] = h >= 11 ? shd(R_BLOOD, 0.22f + 0.06f * ((x + y) & 1)) : shd(R_IRON, 0.05f + 0.04f * (h > 6)); continue; }
            int l = 0, r = 0;                                       /* held between metal? */
            for (int k = 1; k <= 6; k++) {
                if (x - k >= 0) { uint8_t q = lit[(x - k) * TS + y]; if (q != C_CLEAR && (q < R_FLAME || q >= R_FLAME + 16)) l = 1; }
                if (x + k < TS) { uint8_t q = lit[(x + k) * TS + y]; if (q != C_CLEAR && (q < R_FLAME || q >= R_FLAME + 16)) r = 1; }
            }
            out[x * TS + y] = l && r ? shd(R_IRON, 0.10f + 0.08f * (h > 8) + 0.05f * ((x * 3 + y) % 4 == 0)) : C_CLEAR;
        }
    if (v == V_SNUFFED) {
        int k = 0;
        for (int x = 0; x < TS && k < 3; x++) for (int y = 0; y < TS && k < 3; y++) { uint8_t c = lit[x * TS + y]; if (c >= R_FLAME + 13 && c < R_FLAME + 16 && ((x + y) % 3 == 0)) { out[x * TS + y] = (uint8_t)(R_FLAME + 5 + (k & 1) * 2); k++; } }
        for (int y = top - 14; y < top; y++) {                      /* the thread of smoke from where it burned */
            if (y < 1) continue;
            float u = (top - y) / 14.0f, cx = bx + 0.5f + 2.0f * sinf(y * 0.45f) * u;
            for (int x = bx - 4; x <= bx + 4; x++) {
                if (x < 0 || x >= TS || fabsf(x + 0.5f - cx) > 0.9f + u * 1.2f || ((x + y) & 1) == (u > 0.5f)) continue;
                out[x * TS + y] = shd(R_STONE, 0.6f - 0.25f * u);
            }
        }
    }
}

static void tile(int ox, int oy, const uint8_t *wall, const uint8_t *over, int light) {
    for (int y = 0; y < CELL; y++)
        for (int x = 0; x < CELL; x++) {
            int k = (x / SC) * TS + y / SC;
            uint8_t c = over && over[k] != C_CLEAR ? over[k] : wall[k];
            c = crawl_cmap[light][c];
            put(ox + x, oy + y, crawl_pal[c][0] * 255 / 63, crawl_pal[c][1] * 255 / 63, crawl_pal[c][2] * 255 / 63);
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

int main(int argc, char **argv) {
    if (argc < 2) { fprintf(stderr, "usage: %s out.png\n", argv[0]); return 1; }
    static const int LEVELS[4] = { 1, 11, 21, 31 };
    static const char *RUN[4] = { "THE CRYPTS", "THE CATACOMBS", "THE HOLLOWS", "THE DEEP" };
    static const char *COL[NCOL] = { "LIT", "LIT", "LIT", "GUTTER", "GUTTER", "SNUFFED", "COLD" };
    iw = ROWLAB + NCOL * (CELL + PAD) + PAD; ih = HEAD + 4 * (CELL + PAD + 24) + 110;
    img = calloc((size_t)iw * ih * 3, 1);
    rect(0, 0, iw, ih, 16, 14, 20);
    text(PAD, 14, "THE UNLIT TORCH: FOR THE OWNER'S LOOK (PLAN-EVENTS, STEP 2)", 233, 194, 76);
    text(PAD, 44, "THE FIRST THREE ARE EACH RUN'S OWN LIGHT, LIT. THE REST ARE NEW: LOW", 216, 208, 189);
    text(PAD, 64, "AS IT GUTTERS, EMBERS AND SMOKE THE MOMENT IT DIES, THEN COLD: A", 216, 208, 189);
    text(PAD, 84, "CHARRED TORCH, DARK GLASS, A BARE SPOUT, A COOLED CRACK.", 216, 208, 189);
    for (int c = 0; c < NCOL; c++) text(ROWLAB + PAD + c * (CELL + PAD) + 4, HEAD - 26, COL[c], c < 3 ? 233 : 199, c < 3 ? 194 : 155, c < 3 ? 76 : 240);
    crawl_art_init();
    static uint8_t un[TS * TS];
    for (int r = 0; r < 4; r++) {
        crawl_art_level(LEVELS[r]);
        int oy = HEAD + r * (CELL + PAD + 24);
        text(PAD, oy + CELL / 2 - 8, RUN[r], 216, 208, 189);
        const uint8_t *wall = crawl_tex[T_MATV(0, 0)];
        for (int c = 0; c < NCOL; c++) {
            int ox = ROWLAB + PAD + c * (CELL + PAD);
            if (c < 3) tile(ox, oy, wall, crawl_tex[T_FEAT + F1_TORCH + c], LIT_LIGHT);
            else {
                int v = c == 3 ? V_GUTTER0 : c == 4 ? V_GUTTER1 : c == 5 ? V_SNUFFED : V_COLD;
                if (r == 0) make_unlit(crawl_tex[T_FEAT + F1_TORCH], un, v); else make_unlit_run(crawl_tex[T_FEAT + F1_TORCH], un, v, r);
                tile(ox, oy, wall, un, c < 5 ? 18 : DARK_LIGHT);
            }
        }
    }
    int fy = HEAD + 4 * (CELL + PAD + 24) + 6;
    text(PAD, fy, "FLICKER TO LIFE:  COLD, GUTTER, COLD, GUTTER, LIT ... OVER ABOUT 0.6 S", 216, 208, 189);
    text(PAD, fy + 24, "FLICKER OUT:      LIT, GUTTER, LIT, GUTTER, SNUFFED 1 S, THEN COLD", 216, 208, 189);
    text(PAD, fy + 48, "ITS LIGHT FOLLOWS: FULL, A THIRD, NONE (A GLOW OF EMBERS WHEN SNUFFED)", 216, 208, 189);
    if (!write_png(argv[1])) { fprintf(stderr, "cannot write %s\n", argv[1]); return 1; }
    printf("%s: %dx%d\n", argv[1], iw, ih);
    return 0;
}
