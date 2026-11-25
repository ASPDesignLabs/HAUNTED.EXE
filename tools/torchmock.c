/* The unlit torch's sheet (docs/PLAN-events.md, step 2): the sconce every drawn run hangs on its walls, as the game draws it lit,
 * beside what an event makes of it: guttering, just snuffed, and cold. Each row is a run, on that run's own wall and palette; the
 * lit frames sit in a torch's light, the others in the dark it leaves. The owner approved it; the frames are the art's now
 * (src/art/parts/unlit.c, T_UNLIT), and this draws them from there.
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
    for (int r = 0; r < 4; r++) {
        crawl_art_level(LEVELS[r]);
        int oy = HEAD + r * (CELL + PAD + 24);
        text(PAD, oy + CELL / 2 - 8, RUN[r], 216, 208, 189);
        const uint8_t *wall = crawl_tex[T_MATV(0, 0)];
        for (int c = 0; c < NCOL; c++) {
            int ox = ROWLAB + PAD + c * (CELL + PAD);
            if (c < 3) tile(ox, oy, wall, crawl_tex[T_FEAT + F1_TORCH + c], LIT_LIGHT);
            else {
                int v = c == 3 ? UL_GUTTER0 : c == 4 ? UL_GUTTER1 : c == 5 ? UL_SNUFFED : UL_COLD;
                tile(ox, oy, wall, crawl_tex[T_UNLIT + v], c < 5 ? 18 : DARK_LIGHT);   /* the art's own (src/art/parts/unlit.c) */
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
