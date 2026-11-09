/* Mock-ups of a title screen for CATACOMB, for the owner's eye: not in the game. Each is drawn as the game draws, in mode
 * 13h's terms (320x200, one palette of 256 six-bit colours, the DAC rewritten for lightning, art shaded inside its ramps)
 * from the game's own palette, textures and faces, and written as an animated GIF (twice size, each frame with the
 * palette it was drawn in) and a contact sheet of stills.
 *
 *   titlemock <dir> [which]     which: a, b, c (the title and its graves), logo, overview, flash (the lightning's frames);
 *                               all but flash by default. TM_SCALE=n: the GIFs at n times (default 2)
 *
 * Built alone, from the art's sources and the carved stone's (nothing of the game's scene):
 *   gcc -O2 -std=gnu11 -o build/titlemock src/crawl_art.c src/hand_img.c src/face_img.c src/cat_img.c src/font_vga.c \
 *       src/crawl_ui.c src/ui_art.c src/gfx.c tools/titlemock.c -lm -lz
 *
 * A, BENEATH: a graveyard in cross-section, a walker with a lantern in the crypt buried under it.
 * B, THE WALK: the crypt itself in the first person, seen through an arch whose lintel carries the name.
 * C, THE RISEN: one headstone close, the name cut in it and bleeding, a hand clawing out of its grave.
 */
#include "../src/crawl.h"
#include "../src/crawl_ui.h"
#include "../src/gfx.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>

#define W 320
#define H 200
#define FPS 20
#define PI 3.14159265f

static Frame F;
static float T, LOOP;                                   /* the clock, and the loop it comes round in */
static uint8_t BASEPAL[256][3];

static float frac(float v) { return v - floorf(v); }
static float smooth(float a, float b, float v) { float t = clampf((v - a) / (b - a), 0, 1); return t * t * (3 - 2 * t); }
static uint32_t hsh(int x, int y, uint32_t s) {
    uint32_t h = (uint32_t)x * 374761393u + (uint32_t)y * 668265263u + s * 2246822519u;
    h = (h ^ (h >> 13)) * 1274126177u;
    return h ^ (h >> 16);
}
static float h01(int x, int y, uint32_t s) { return (float)(hsh(x, y, s) & 0xFFFF) / 65535.0f; }
static float vnoise(float x, float y, uint32_t s) {
    int x0 = (int)floorf(x), y0 = (int)floorf(y);
    float fx = x - x0, fy = y - y0;
    fx = fx * fx * (3 - 2 * fx); fy = fy * fy * (3 - 2 * fy);
    float a = h01(x0, y0, s), b = h01(x0 + 1, y0, s), c = h01(x0, y0 + 1, s), d = h01(x0 + 1, y0 + 1, s);
    return (a + (b - a) * fx) * (1 - fy) + (c + (d - c) * fx) * fy;
}
static float fbm(float x, float y, uint32_t s) {
    float v = 0, amp = 0.5f;
    for (int o = 0; o < 4; o++) { v += amp * vnoise(x, y, s + (uint32_t)o * 77u); x *= 2.03f; y *= 2.03f; amp *= 0.5f; }
    return v / 0.9375f;
}
/* periodic in x with period p (cells of noise), for things that drift and must come round with the loop */
static float vnoise_px(float x, float y, int p, uint32_t s) {
    int x0 = (int)floorf(x), y0 = (int)floorf(y);
    float fx = x - x0, fy = y - y0;
    fx = fx * fx * (3 - 2 * fx); fy = fy * fy * (3 - 2 * fy);
    int ax = ((x0 % p) + p) % p, bx = (ax + 1) % p;
    float a = h01(ax, y0, s), b = h01(bx, y0, s), c = h01(ax, y0 + 1, s), d = h01(bx, y0 + 1, s);
    return (a + (b - a) * fx) * (1 - fy) + (c + (d - c) * fx) * fy;
}
static float fbm_px(float x, float y, int p, uint32_t s) {
    float v = 0, amp = 0.5f;
    for (int o = 0; o < 4; o++) { v += amp * vnoise_px(x, y, p, s + (uint32_t)o * 77u); x *= 2; y *= 2; p *= 2; amp *= 0.5f; }
    return v / 0.9375f;
}
static const uint8_t BAYER[4][4] = { { 0, 8, 2, 10 }, { 12, 4, 14, 6 }, { 3, 11, 1, 9 }, { 15, 7, 13, 5 } };
static float bay(int x, int y) { return (BAYER[y & 3][x & 3] + 0.5f) / 16.0f; }

/* ---- the frame as materials and light (13h has no light: a colour is a step in a ramp, so the light picks the step) ---- */
enum { MF_FLAT = 1, MF_DIRECT = 2, MF_NOWARM = 4 };
static uint8_t MR[H][W], MF[H][W], MD[H][W];             /* the ramp; flags; a colour as it is (MF_DIRECT) */
static float MV[H][W], LC[H][W], LW[H][W];               /* the stuff's own value 0..1; cold light and warm, 1 as drawn */
static int warm_of(int ramp) { return ramp == R_STONE ? R_WARM : ramp == R_CEIL ? R_WARM : ramp == R_IRON ? R_WARM : ramp; }
static void matf(int x, int y, int ramp, float v, int flags) { if ((unsigned)x < W && (unsigned)y < H) { MR[y][x] = (uint8_t)ramp; MV[y][x] = v; MF[y][x] = (uint8_t)flags; } }
static void direct(int x, int y, uint8_t c) { if ((unsigned)x < W && (unsigned)y < H) { MF[y][x] = MF_DIRECT; MD[y][x] = c; } }
static void light_clear(float cold, float warm) { for (int y = 0; y < H; y++) for (int x = 0; x < W; x++) { LC[y][x] = cold; LW[y][x] = warm; } }
static void light_add(float cx, float cy, float r, float k, int warm) {   /* a soft round light */
    int x0 = clampi((int)(cx - r), 0, W - 1), x1 = clampi((int)(cx + r), 0, W - 1), y0 = clampi((int)(cy - r), 0, H - 1), y1 = clampi((int)(cy + r), 0, H - 1);
    for (int y = y0; y <= y1; y++) for (int x = x0; x <= x1; x++) {
        float d = sqrtf((x - cx) * (x - cx) + (y - cy) * (y - cy)) / r;
        if (d >= 1) continue;
        float a = (1 - d) * (1 - d) * k;
        if (warm) LW[y][x] += a; else LC[y][x] += a;
    }
}
static void resolve(void) {
    for (int y = 0; y < H; y++) for (int x = 0; x < W; x++) {
        int fl = MF[y][x];
        if (fl & MF_DIRECT) { F.pix[y * W + x] = MD[y][x]; continue; }
        int r = MR[y][x];
        if (r == 0) { F.pix[y * W + x] = 0; continue; }
        float lc = LC[y][x], lw = LW[y][x], l = lc + lw;
        float v = MV[y][x] * l * 15.0f;
        float d = fl & MF_FLAT ? 0.5f : bay(x, y);
        int ramp = r;
        if (!(fl & MF_NOWARM) && lw > 0.02f && warm_of(r) != r && lw / l > 0.25f + 0.5f * bay(x + 2, y + 1)) ramp = warm_of(r);
        int n = ramp == R_GOLD ? NGOLD : 16;
        int k = (int)(v * (n - 1) / 15.0f + d);
        if (k <= 0 && v < 0.35f) { F.pix[y * W + x] = 0; continue; }
        F.pix[y * W + x] = (uint8_t)(ramp + clampi(k, 0, n - 1));
    }
}

/* ---- the palette: the game's, at level 3, with ramps lent as a menu lends them ---- */
static void ramp3(uint8_t pal[256][3], int base, int n, const int a[3], const int m[3], const int b[3]) {
    for (int i = 0; i < n; i++) {
        float t = i / (float)(n - 1), u;
        const int *p, *q;
        if (t < 0.5f) { p = a; q = m; u = t * 2; } else { p = m; q = b; u = (t - 0.5f) * 2; }
        for (int c = 0; c < 3; c++) pal[base + i][c] = (uint8_t)clampi((int)(p[c] + (q[c] - p[c]) * u + 0.5f), 0, 63);
    }
}
static void pal_night(void) {                          /* R_CEIL the night sky, R_FLOOR the earth */
    memcpy(F.pal, BASEPAL, sizeof F.pal);
    static const int SA[3] = { 0, 0, 3 }, SM[3] = { 7, 8, 19 }, SB[3] = { 30, 33, 47 };
    static const int EA[3] = { 2, 1, 1 }, EM[3] = { 15, 10, 7 }, EB[3] = { 38, 29, 21 };
    ramp3(F.pal, R_CEIL, 16, SA, SM, SB);
    ramp3(F.pal, R_FLOOR, 16, EA, EM, EB);
}
static void pal_flash(float k) {                       /* lightning: the sky's ramp and the stone's raised in the DAC */
    if (k <= 0) return;
    for (int i = 0; i < 16; i++) for (int c = 0; c < 3; c++) {
        int v = F.pal[R_CEIL + i][c];
        F.pal[R_CEIL + i][c] = (uint8_t)clampi((int)(v + (54 - v) * k * (0.25f + 0.04f * i) + (c == 2 ? 4 * k : 0)), 0, 63);
        v = F.pal[R_STONE + i][c];
        F.pal[R_STONE + i][c] = (uint8_t)clampi((int)(v + (56 - v) * k * 0.18f), 0, 63);
    }
}

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
/* the sheet: stills of each mock-up, doubled, in rows, with labels in the BIOS face */
static uint8_t *sheet; static int sw, sh;
static void sheet_put(int ox, int oy) {
    for (int y = 0; y < H * 2; y++) for (int x = 0; x < W * 2; x++) {
        int X = ox + x, Y = oy + y;
        if (X < 0 || Y < 0 || X >= sw || Y >= sh) continue;
        const uint8_t *p = F.pal[F.pix[(y / 2) * W + x / 2]];
        uint8_t *q = &sheet[((size_t)Y * sw + X) * 3];
        for (int c = 0; c < 3; c++) q[c] = (uint8_t)(p[c] * 255 / 63);
    }
}
static void sheet_label(int x, int y, const char *s) {
    for (; *s; s++, x += 16)
        for (int r = 0; r < 8; r++) for (int c = 0; c < 8; c++)
            if (font_bios8[(uint8_t)*s][r] >> (7 - c) & 1)
                for (int j = 0; j < 2; j++) for (int i = 0; i < 2; i++) {
                    int X = x + c * 2 + i, Y = y + r * 2 + j;
                    if (X < sw && Y < sh) { uint8_t *q = &sheet[((size_t)Y * sw + X) * 3]; q[0] = q[1] = q[2] = 200; }
                }
}

/* ======================================================================================================================
 * The name: HAUNTED.EXE in a broken gothic hand. Each letter is a few strokes of a broad nib held at 35 degrees (the
 * stems come out thick with their ends cut on the slant, what climbs to the right thin), with thorns and hairlines;
 * filled at four by four under each pixel and kept where half is covered.
 * ====================================================================================================================== */
typedef struct { float L; int n; float p[14]; } Stroke;          /* the nib's length (0: a polygon), points as x, y */
typedef struct { char ch; float w; Stroke s[9]; } Glyph;
static const Glyph GLYPHS[] = {
    { 'H', 29, { { 9, 2, { 6, 1, 6, 40 } }, { 9, 2, { 22, 5, 22, 40 } }, { 6, 2, { 6, 22, 22, 19 } },
                 { 2.2f, 3, { 6, 2, 2, 5, 0, 10 } }, { 0, 3, { 2.4f, 24, -1.8f, 27.5f, 2.4f, 30 } },
                 { 0, 3, { 25.6f, 12, 29.5f, 15, 25.6f, 18 } }, { 2.2f, 3, { 22, 6, 25, 2, 27, 0 } } } },
    { 'A', 29, { { 6, 2, { 2.5f, 39, 14, 2 } }, { 9, 3, { 13, 2, 22, 8, 22, 40 } }, { 5.5f, 2, { 8, 26, 22, 23 } },
                 { 2.2f, 2, { -1, 40, 7, 40 } }, { 0, 3, { 25.6f, 16, 29.5f, 19, 25.6f, 22 } },
                 { 2.2f, 2, { 22, 40, 28, 37 } } } },
    { 'U', 29, { { 9, 4, { 6, 3, 6, 32, 12, 40, 18, 37 } }, { 9, 2, { 22, 1, 22, 40 } },
                 { 2.2f, 3, { 6, 4, 2, 6, 0, 11 } }, { 0, 3, { 2.4f, 18, -1.8f, 21.5f, 2.4f, 24 } },
                 { 2.2f, 2, { 22, 40, 28, 37 } } } },
    { 'N', 29, { { 9, 2, { 6, 5, 6, 40 } }, { 5, 2, { 7, 3, 21, 36 } }, { 9, 2, { 22, 1, 22, 37 } },
                 { 2.2f, 3, { 6, 6, 2, 8, 0, 13 } }, { 0, 3, { 25.6f, 22, 29.5f, 25, 25.6f, 28 } },
                 { 2.2f, 2, { -1, 40, 6, 40 } } } },
    { 'T', 30, { { 7, 3, { 0.5f, 6, 14, 2, 29.5f, 5 } }, { 9, 2, { 15, 3, 15, 40 } },
                 { 2.2f, 2, { 0.5f, 6, -0.5f, 13 } }, { 2.2f, 2, { 29.5f, 5, 30.5f, 12 } },
                 { 0, 3, { 18.6f, 20, 22.5f, 23, 18.6f, 26 } }, { 0, 3, { 11.4f, 26, 7.5f, 29, 11.4f, 32 } } } },
    { 'E', 27, { { 9, 6, { 22, 4, 13, 1, 6, 8, 6, 33, 13, 40, 23, 36 } }, { 6, 2, { 6, 21, 18, 19 } },
                 { 2.2f, 2, { 22, 4, 24, 10 } }, { 0, 3, { 18, 19.5f, 22.5f, 18, 18, 23.5f } } } },
    { 'D', 29, { { 9, 2, { 6, 1, 6, 40 } }, { 7.5f, 6, { 6, 1, 15, 2, 22.5f, 9, 22.5f, 32, 15, 39, 6, 40 } },
                 { 2.2f, 3, { 6, 2, 2, 5, 0, 10 } }, { 0, 3, { 2.4f, 24, -1.8f, 27.5f, 2.4f, 30 } } } },
    { 'X', 28, { { 9, 2, { 4, 1, 23, 40 } }, { 4, 2, { 23, 1, 4, 40 } }, { 2.2f, 2, { -1, 40, 8, 40 } },
                 { 2.2f, 2, { 19, 1, 28, 1 } } } },
    { '.', 12, { { 0, 4, { 6, 29, 10.5f, 34, 6, 39, 1.5f, 34 } } } },
};
static const Glyph *glyph_of(char c) { for (size_t i = 0; i < sizeof GLYPHS / sizeof GLYPHS[0]; i++) if (GLYPHS[i].ch == c) return &GLYPHS[i]; return NULL; }

static int in_convex(const float *p, int n, float x, float y) {
    int pos = 0, neg = 0;
    for (int i = 0; i < n; i++) {
        float ax = p[2 * i], ay = p[2 * i + 1], bx = p[2 * ((i + 1) % n)], by = p[2 * ((i + 1) % n) + 1];
        float c = (bx - ax) * (y - ay) - (by - ay) * (x - ax);
        if (c > 1e-6f) pos = 1; else if (c < -1e-6f) neg = 1;
        if (pos && neg) return 0;
    }
    return 1;
}
#define LGW 320
#define LGH 96
typedef struct {
    int w, h;                                   /* the logo's box */
    uint8_t m[LGH][LGW];                        /* in it */
    uint8_t letter[LGH][LGW];                   /* which letter, 1 on */
    int8_t d[LGH][LGW];                         /* steps in from its edge */
    float nx[LGH][LGW], ny[LGH][LGW];           /* which way its surface leans */
    int ndrip;
    struct { int x, y, len, per, big; float ph; } drip[96];
} Logo;
static Logo LG;
/* the name laid out: line 1 at scale s1, then line 2 (or on the same line when same is set) at s2 */
static void logo_build(const char *l1, float s1, const char *l2, float s2, int same, uint32_t seed) {
    memset(&LG, 0, sizeof LG);
    const int SS = 4;
    float gap = 2.5f;
    float w1 = 0, w2 = 0;
    for (const char *c = l1; *c; c++) w1 += (glyph_of(*c)->w + gap) * s1;
    for (const char *c = l2; *c; c++) w2 += (glyph_of(*c)->w + gap) * s2;
    w1 -= gap * s1; w2 -= gap * s2;
    float ox1, oy1 = 3, ox2, oy2;
    if (same) { LG.w = (int)(w1 + 4 * s1 + w2) + 8; ox1 = 4; ox2 = ox1 + w1 + 4 * s1; oy2 = oy1 + (40 - 40 * s2 / s1) * s1 * 0 + 40 * (s1 - s2); LG.h = (int)(40 * s1) + 8; }
    else { LG.w = (int)fmaxf(w1, w2) + 8; ox1 = 4 + (LG.w - 8 - w1) / 2; ox2 = LG.w - 4 - w2 - 6 * s1; oy2 = oy1 + 40 * s1 + 4; LG.h = (int)(oy2 + 40 * s2) + 5; }
    if (LG.w > LGW) LG.w = LGW;
    if (LG.h > LGH) LG.h = LGH;
    for (int line = 0; line < 2; line++) {
        const char *s = line ? l2 : l1;
        float sc = line ? s2 : s1, x = line ? ox2 : ox1, y = line ? oy2 : oy1;
        int li = line ? (int)strlen(l1) : 0;
        for (const char *c = s; *c; c++, li++) {
            const Glyph *g = glyph_of(*c);
            const float na = 35.0f * PI / 180.0f, nx = cosf(na), ny = -sinf(na);
            for (int k = 0; k < 9 && g->s[k].n; k++) {
                const Stroke *st = &g->s[k];
                float poly[16];
                int segs = st->L > 0 ? st->n - 1 : 1;
                for (int sgi = 0; sgi < segs; sgi++) {
                    int np;
                    if (st->L > 0) {
                        float ax = st->p[2 * sgi], ay = st->p[2 * sgi + 1], bx = st->p[2 * sgi + 2], by = st->p[2 * sgi + 3], hl = st->L * 0.5f;
                        float ex = nx * hl, ey = ny * hl;
                        if (st->L < 3) {                    /* a hairline: a pen of its own width across the way it goes */
                            float dx = bx - ax, dy = by - ay, l = sqrtf(dx * dx + dy * dy) + 1e-6f;
                            ex = -dy / l * hl; ey = dx / l * hl;
                        }
                        float q[8] = { ax - ex, ay - ey, ax + ex, ay + ey, bx + ex, by + ey, bx - ex, by - ey };
                        memcpy(poly, q, sizeof q); np = 4;
                    } else { memcpy(poly, st->p, sizeof(float) * 2 * st->n); np = st->n; }
                    float bx0 = 1e9f, by0 = 1e9f, bx1 = -1e9f, by1 = -1e9f;
                    for (int i = 0; i < np; i++) {
                        poly[2 * i] = x + poly[2 * i] * sc; poly[2 * i + 1] = y + poly[2 * i + 1] * sc;
                        bx0 = fminf(bx0, poly[2 * i]); bx1 = fmaxf(bx1, poly[2 * i]); by0 = fminf(by0, poly[2 * i + 1]); by1 = fmaxf(by1, poly[2 * i + 1]);
                    }
                    for (int py = (int)floorf(by0); py <= (int)ceilf(by1); py++)
                        for (int px = (int)floorf(bx0); px <= (int)ceilf(bx1); px++) {
                            if (px < 0 || py < 0 || px >= LG.w || py >= LG.h) continue;
                            int cov = 0;
                            for (int j = 0; j < SS; j++) for (int i = 0; i < SS; i++)
                                cov += in_convex(poly, np, px + (i + 0.5f) / SS, py + (j + 0.5f) / SS);
                            if (cov * 2 >= SS * SS) { LG.m[py][px] = 1; LG.letter[py][px] = (uint8_t)(li + 1); }
                        }
                }
            }
            x += (g->w + gap) * sc;
        }
    }
    /* steps in from the edge, and the lean of a surface rounded over two of them */
    for (int y = 0; y < LG.h; y++) for (int x = 0; x < LG.w; x++) LG.d[y][x] = LG.m[y][x] ? 9 : 0;
    for (int it = 0; it < 9; it++)
        for (int y = 0; y < LG.h; y++) for (int x = 0; x < LG.w; x++) {
            if (!LG.m[y][x]) continue;
            int best = 9;
            for (int j = -1; j <= 1; j++) for (int i = -1; i <= 1; i++) {
                int X = x + i, Y = y + j;
                int v = (X < 0 || Y < 0 || X >= LG.w || Y >= LG.h) ? 0 : LG.d[Y][X];
                if (v + 1 < best) best = v + 1;
            }
            LG.d[y][x] = (int8_t)best;
        }
    for (int y = 0; y < LG.h; y++) for (int x = 0; x < LG.w; x++) {
        if (!LG.m[y][x]) continue;
        float hx = 0, hy = 0;
        for (int j = -1; j <= 1; j++) for (int i = -1; i <= 1; i++) {
            int X = x + i, Y = y + j;
            float v = (X < 0 || Y < 0 || X >= LG.w || Y >= LG.h) ? 0 : fminf((float)LG.d[Y][X], 3.0f);
            hx += v * i; hy += v * j;
        }
        float l = sqrtf(hx * hx + hy * hy);
        LG.nx[y][x] = l > 0 ? -hx / l : 0; LG.ny[y][x] = l > 0 ? -hy / l : 0;
        if (LG.d[y][x] >= 3) LG.nx[y][x] = LG.ny[y][x] = 0;
    }
    /* drips: from the lowest edges, the feet first */
    LG.ndrip = 0;
    for (int x = 1; x < LG.w - 1 && LG.ndrip < 96; x++)
        for (int y = LG.h - 2; y > 0; y--) {
            if (!LG.m[y][x] || LG.m[y + 1][x]) continue;
            if (!LG.m[y][x - 1] || !LG.m[y][x + 1] || LG.m[y + 1][x - 1] || LG.m[y + 1][x + 1]) continue;
            float r = h01(x, y, seed);
            int foot = y > LG.h * 0.6f || (y > 30 && y < 50);
            if (r < (foot ? 0.42f : 0.12f)) {
                int k = LG.ndrip++;
                LG.drip[k].x = x; LG.drip[k].y = y + 1;
                LG.drip[k].big = r < (foot ? 0.16f : 0.03f);
                LG.drip[k].len = (LG.drip[k].big ? 9 : 3) + (int)(h01(x, y, seed + 1) * (LG.drip[k].big ? 24 : 9));
                LG.drip[k].per = 1 + (int)(h01(x, y, seed + 2) * 3);
                LG.drip[k].ph = h01(x, y, seed + 3);
                x += 3;
            }
            break;
        }
}

/* the name drawn: style 0 letters of blood; 1 cut into stone with blood in the cuts. lit: how much light falls on it;
 * fall: how far a drop may fall below it before it is lost (or lands); ground: the row it lands on, or -1 */
enum { LS_BLOOD, LS_CARVED };
static void logo_draw(int ox, int oy, int style, float lit, int fall_to, float t) {
    /* the shadow behind */
    if (style == LS_BLOOD)
        for (int y = 0; y < LG.h; y++) for (int x = 0; x < LG.w; x++) {
            if (!LG.m[y][x]) continue;
            for (int k = 1; k <= 3; k++) {
                int X = ox + x + k, Y = oy + y + k;
                if ((unsigned)X < W && (unsigned)Y < H && !(y + k < LG.h && x + k < LG.w && LG.m[y + k][x + k])) direct(X, Y, 0);
            }
        }
    for (int y = 0; y < LG.h; y++) for (int x = 0; x < LG.w; x++) {
        int X = ox + x, Y = oy + y;
        if (!LG.m[y][x]) {
            int edge = 0;
            for (int j = -1; j <= 1; j++) for (int i = -1; i <= 1; i++) {
                int a = x + i, b = y + j;
                if (a >= 0 && b >= 0 && a < LG.w && b < LG.h && LG.m[b][a]) edge = 1;
            }
            if (edge && style == LS_BLOOD) direct(X, Y, 0);
            if (style == LS_CARVED && edge) {                       /* the stone's lip: lit below and right of a cut */
                int lip = (y > 0 && LG.m[y - 1][x]) || (x > 0 && LG.m[y][x - 1]);
                if (lip && (unsigned)X < W && (unsigned)Y < H) MV[Y][X] = fminf(1.0f, MV[Y][X] + 0.22f);
            }
            continue;
        }
        float lx = -0.6f, ly = -0.8f;                                /* light from above, a little to the left */
        float dotl = LG.nx[y][x] * lx + LG.ny[y][x] * ly;
        int dd = LG.d[y][x];
        uint8_t c;
        if (style == LS_BLOOD) {
            float v = 0.42f + 0.30f * dotl - 0.05f * (float)(dd > 3);
            float sweep = frac(t / LOOP) * (LG.w + 160) - 80;        /* a gleam crossing the wet once a loop */
            float g = 1.0f - fabsf((x + y * 0.6f) - sweep) / 14.0f;
            if (g > 0) v += 0.28f * g * g;
            v *= lit;
            if (dd == 1 && dotl > 0.55f) c = (uint8_t)(R_SKIN + clampi((int)(9 + 6 * lit + (g > 0.6f ? 2 : 0)), 0, 15));   /* the wet catches it */
            else if (dd == 2 && dotl > 0.6f && h01(x, y, 5) < 0.25f) c = (uint8_t)(R_BLOOD + 15);
            else c = (uint8_t)(R_BLOOD + clampi((int)(v * 15 + bay(X, Y) * 0.8f), 1, 14));
            direct(X, Y, c);
        } else {
            /* the cut: its upper and left walls in shadow, the lower and right lit; blood lies in the bottom of it */
            float wallv = dotl > 0.15f ? 0.03f : dotl < -0.15f ? 0.2f : 0.1f;
            int blood = dd >= 2 || (dd == 1 && LG.d[y][x] && dotl < -0.5f && 0);
            if (blood) {
                float v = (0.5f + 0.2f * (-dotl) * 0.5f) * lit;
                float sweep = frac(t / LOOP) * (LG.w + 160) - 80;
                float g = 1.0f - fabsf((x + y * 0.6f) - sweep) / 12.0f;
                if (g > 0) v += 0.25f * g * g;
                if (dd == 2 && dotl > 0.4f) c = (uint8_t)(R_BLOOD + 4);
                else if (dd == 2 && dotl < -0.4f) c = (uint8_t)(R_BLOOD + clampi((int)(13 * lit), 3, 14));
                else c = (uint8_t)(R_BLOOD + clampi((int)(v * 15 + bay(X, Y) * 0.8f), 1, 13));
                direct(X, Y, c);
            } else if ((unsigned)X < W && (unsigned)Y < H) { MV[Y][X] = wallv; MF[Y][X] &= (uint8_t)~MF_DIRECT; }
        }
    }
    /* the drips: grow, swell, let go a drop, draw back a little, again; each comes round a whole number of times a loop */
    for (int k = 0; k < LG.ndrip; k++) {
        int dx = ox + LG.drip[k].x, dy = oy + LG.drip[k].y;
        float u = frac(t / LOOP * LG.drip[k].per + LG.drip[k].ph);
        float L = (float)LG.drip[k].len, len;
        if (u < 0.7f) len = L * (0.35f + 0.65f * smooth(0, 0.7f, u));
        else if (u < 0.78f) len = L;
        else len = L * (1.0f - 0.65f * smooth(0.78f, 1.0f, u));
        int il = (int)len, bulb = u > 0.55f && u < 0.79f ? 2 : 1;
        for (int j = 0; j < il; j++) {
            direct(dx, dy + j, (uint8_t)(R_BLOOD + clampi((int)(7 * lit) - (j > il - 2 ? 0 : 1), 2, 12)));
            if (LG.drip[k].big && j < il - 3) direct(dx + 1, dy + j, (uint8_t)(R_BLOOD + clampi((int)(5 * lit), 1, 10)));
            if (j == 0 || (j < il / 2 && LG.drip[k].big)) direct(dx - 1, dy + j, (uint8_t)(R_BLOOD + clampi((int)(9 * lit), 2, 13)));
        }
        if (il > 0) {                                               /* the bead at its end, and its gleam */
            int by = dy + il;
            for (int j = 0; j < bulb + 1; j++) for (int i = -bulb + 1; i <= bulb - (bulb > 1 ? 0 : 1); i++)
                direct(dx + i, by + j - 1, (uint8_t)(R_BLOOD + clampi((int)(8 * lit), 2, 12)));
            direct(dx - (bulb > 1), by - 1, (uint8_t)(R_SKIN + clampi((int)(10 * lit), 4, 15)));
            if (style == LS_BLOOD) direct(dx, by + bulb, 0);
        }
        if (u >= 0.78f && fall_to > 0) {                           /* the drop let go */
            float ft = (u - 0.78f) * LOOP / LG.drip[k].per;
            float fy = dy + L + 0.5f * 260.0f * ft * ft;
            if (fy < fall_to) {
                int fyi = (int)fy;
                direct(dx, fyi, (uint8_t)(R_BLOOD + clampi((int)(9 * lit), 3, 13)));
                direct(dx, fyi - 1, (uint8_t)(R_BLOOD + clampi((int)(6 * lit), 2, 10)));
                if (ft > 0.12f) direct(dx, fyi - 2, (uint8_t)(R_BLOOD + clampi((int)(4 * lit), 1, 8)));
            } else {                                                /* a splash where it lands, a moment */
                float land = sqrtf(2.0f * (fall_to - dy - L) / 260.0f);
                float st = ft - land;
                if (st < 0.25f) {
                    int r = 1 + (int)(st * 12);
                    direct(dx - r, fall_to - (r > 1), (uint8_t)(R_BLOOD + 6)); direct(dx + r, fall_to - (r > 1), (uint8_t)(R_BLOOD + 6));
                    direct(dx, fall_to, (uint8_t)(R_BLOOD + 4));
                }
            }
        }
    }
}

/* the prompt: inlaid words that breathe in and out slowly (no blinking), on a band of dark stone if asked */
static void prompt(int cx, int y, const char *s, int band, float t, int ramp) {
    int w = (int)strlen(s) * 8;
    if (band) ui_band(&F, cx - w / 2 - 8, y - 4, w + 16, 16);
    float b = 0.5f + 0.5f * sinf(2 * PI * t / (LOOP / roundf(LOOP / 3.0f)));
    int col = ramp + clampi((int)(8 + 6 * b), 0, 15);
    ui_text8(&F, cx - w / 2, y, s, FACE_INSC, col, TX_INLAID);
}

/* ---- the night sky, its moon and clouds ---- */
static float flash_k(float t, float at) {                /* lightning: a flash, a pause, a second, and its fading */
    float u = t - at;
    if (u < 0) u += LOOP;
    if (u < 0.08f) return 1.0f;
    if (u < 0.18f) return 0.15f;
    if (u < 0.30f) return 0.75f;
    if (u < 0.9f) return 0.75f * (1 - smooth(0.3f, 0.9f, u));
    return 0;
}
static void sky(int y0, int y1, float t, float mx, float my, float mr, float flash) {
    for (int y = y0; y < y1; y++) for (int x = 0; x < W; x++) {
        float v = 0.06f + 0.30f * (float)(y - y0) / (float)(y1 - y0);
        float d = sqrtf((x - mx) * (x - mx) + (y - my) * (y - my));
        if (d < mr * 4.5f) v += 0.32f * powf(1 - d / (mr * 4.5f), 2);   /* the moon's halo */
        matf(x, y, R_CEIL, v, MF_NOWARM);
    }
    for (int i = 0; i < 70; i++) {                                     /* stars, a few breathing */
        int sx = (int)(h01(i, 1, 77) * W), sy = y0 + (int)(h01(i, 2, 77) * (y1 - y0) * 0.8f);
        float tw = h01(i, 3, 77) < 0.4f ? 0.5f + 0.5f * sinf(2 * PI * (t / LOOP * (1 + i % 3) + h01(i, 4, 77))) : 1;
        float dm = sqrtf((sx - mx) * (sx - mx) + (sy - my) * (sy - my));
        if (dm < mr * 2.5f) continue;
        matf(sx, sy, R_CEIL, 0.55f + 0.45f * tw * h01(i, 5, 77), MF_NOWARM | MF_FLAT);
    }
    for (int y = (int)(my - mr - 1); y <= (int)(my + mr + 1); y++)      /* the moon: craters, lit from the right */
        for (int x = (int)(mx - mr - 1); x <= (int)(mx + mr + 1); x++) {
            float dx = (x - mx) / mr, dy = (y - my) / mr, r2 = dx * dx + dy * dy;
            if (r2 > 1 || y < y0) continue;
            float n = fbm(x * 0.18f, y * 0.18f, 41);
            float v = 0.72f + 0.25f * n + 0.18f * dx - 0.25f * r2 * r2;
            if (n > 0.62f) v -= 0.18f;
            matf(x, y, R_BONE, clampf(v, 0.3f, 1.0f), MF_NOWARM | MF_FLAT);
        }
    for (int layer = 0; layer < 2; layer++) {                          /* clouds drifting left, coming round with the loop */
        int per = layer ? 5 : 4;
        float sp = (float)(W) / LOOP * (layer ? 1 : 2) / 1.0f;
        for (int y = y0; y < y1; y++) for (int x = 0; x < W; x++) {
            /* drifting, and seamless round the loop: two drifts a loop apart, crossfaded */
            float fy = y * 0.045f + layer * 3, a1 = (x + t * sp * 0.25f) / (float)W * per, a0 = (x + (t - LOOP) * sp * 0.25f) / (float)W * per;
            float c = fbm_px(a1, fy, per, 91 + layer * 13) * (1 - t / LOOP) + fbm_px(a0, fy, per, 91 + layer * 13) * (t / LOOP);
            float band = 1 - fabsf((float)(y - (y0 + (y1 - y0) * (layer ? 0.55f : 0.3f))) / ((y1 - y0) * 0.25f));
            c = c * clampf(band, 0, 1) - 0.36f;
            if (c <= 0) continue;
            float a = clampf(c * 4, 0, 1);
            if (a < bay(x, y) * 0.9f) continue;
            float d = sqrtf((x - mx) * (x - mx) + (y - my) * (y - my));
            float rim = d < mr * 5 ? 0.25f * (1 - d / (mr * 5)) : 0;
            matf(x, y, R_CEIL, 0.14f + 0.16f * layer + rim + 0.1f * (1 - a), MF_NOWARM);
        }
    }
    (void)flash;
}

/* fog: dithered drifting bands of pale stone */
static void fog(int y0, int y1, float t, float dens, uint32_t seed) {
    for (int y = y0; y < y1; y++) for (int x = 0; x < W; x++) {
        float fy = y * 0.08f, f1 = fbm_px((x + t * 9) / (float)W * 3, fy, 3, seed), f0 = fbm_px((x + (t - LOOP) * 9) / (float)W * 3, fy, 3, seed);
        float f = (f1 * (1 - t / LOOP) + f0 * (t / LOOP)) * smooth((float)y0, (float)y0 + (y1 - y0) * 0.5f, (float)y) * (1 - smooth((float)y1 - 6, (float)y1, (float)y));
        f = f * dens - 0.5f;
        if (f > bay(x, y) * 0.5f && !(MF[y][x] & MF_DIRECT)) { MR[y][x] = R_STONE; MV[y][x] = fmaxf(MV[y][x], 0.3f + 0.4f * f); MF[y][x] = MF_NOWARM; }
    }
}

/* ---- shapes in the materials ---- */
static void mdisc(float cx, float cy, float r, int ramp, float v, int flags) {
    for (int y = (int)floorf(cy - r); y <= (int)ceilf(cy + r); y++) for (int x = (int)floorf(cx - r); x <= (int)ceilf(cx + r); x++)
        if ((x + 0.5f - cx) * (x + 0.5f - cx) + (y + 0.5f - cy) * (y + 0.5f - cy) <= r * r) matf(x, y, ramp, v, flags);
}
static void mline(float x0, float y0, float x1, float y1, float w, int ramp, float v, int flags) {
    float dx = x1 - x0, dy = y1 - y0, l = sqrtf(dx * dx + dy * dy);
    int n = (int)(l * 2) + 1;
    for (int i = 0; i <= n; i++) { float u = (float)i / n; mdisc(x0 + dx * u, y0 + dy * u, w * 0.5f, ramp, v, flags); }
}
static void mrect(int x, int y, int w, int h, int ramp, float v, int flags) { for (int j = 0; j < h; j++) for (int i = 0; i < w; i++) matf(x + i, y + j, ramp, v, flags); }

/* a headstone seen from the side of the plot: shape 0 rounded, 1 a cross, 2 pointed, 3 broken; lit from the moon's side */
static void small_stone(int cx, int base, int w, int h, int shape, float lean, float lit_right, uint32_t seed) {
    for (int y = base - h; y < base; y++) for (int x = cx - w / 2 - 3; x <= cx + w / 2 + 3; x++) {
        float fy = (float)(base - y) / h, fx = x - cx - lean * (base - y);
        int in = 0;
        float hw = w * 0.5f;
        if (shape == 0) { float r = hw, top = h - r; in = fabsf(fx) <= hw && ((base - y) <= top || fx * fx + ((base - y) - top) * ((base - y) - top) <= r * r); }
        else if (shape == 1) { in = (fabsf(fx) <= hw * 0.35f) || (fabsf(fx) <= hw && fy > 0.62f && fy < 0.8f); }
        else if (shape == 2) { in = fabsf(fx) <= hw * (fy < 0.7f ? 1 : (1 - fy) / 0.3f); }
        else { in = fabsf(fx) <= hw && fy < 0.75f - 0.35f * (fx / hw + 1) * 0.5f + 0.1f * (h01(x, 0, seed) - 0.5f); }
        if (!in) continue;
        float v = 0.34f + 0.16f * fbm(x * 0.3f, y * 0.3f, seed) + lit_right * 0.22f * (fx / hw);
        int edge_r = 0;
        if (fx > hw - 1.5f) edge_r = 1;
        if (edge_r) v += 0.18f * lit_right;
        if (fx < -hw + 1.2f) v -= 0.12f;
        int moss = fbm(x * 0.5f, y * 0.5f, seed + 5) > 0.66f && fy < 0.5f;
        matf(x, y, moss ? R_MOSS : R_STONE, v, 0);
    }
}

/* a dead tree: a trunk and its branches, forked again and again */
static void branch(float x, float y, float ang, float len, float w, int depth, uint32_t seed) {
    float x1 = x + cosf(ang) * len, y1 = y - sinf(ang) * len;
    mline(x, y, x1, y1, w, R_WOOD, 0.12f, 0);
    if (depth <= 0 || w < 0.9f) return;
    int n = 2 + (h01(depth, (int)len, seed) < 0.3f);
    for (int i = 0; i < n; i++) {
        float da = (h01(i, depth, seed) - 0.5f) * 1.3f + (i - (n - 1) * 0.5f) * 0.55f;
        branch(x1, y1, ang + da, len * (0.62f + 0.2f * h01(i + 7, depth, seed)), w * 0.62f, depth - 1, seed * 31u + (uint32_t)i);
    }
}
/* a root going down into the earth, forking */
static void root(float x, float y, float ang, float len, float w, int depth, uint32_t seed) {
    float px = x, py = y;
    int n = (int)(len / 3) + 1;
    for (int i = 1; i <= n; i++) {
        float u = (float)i / n;
        float a = ang + 0.5f * sinf(u * 5 + seed % 7);
        float nx = px + cosf(a) * 3, ny = py + sinf(a) * 3;
        mline(px, py, nx, ny, w * (1 - 0.6f * u), R_WOOD, 0.3f - 0.1f * u, 0);
        px = nx; py = ny;
    }
    if (depth > 0) for (int i = 0; i < 2; i++) root(px, py, ang + (i ? 0.6f : -0.6f) + (h01(i, depth, seed) - 0.5f), len * 0.6f, w * 0.6f, depth - 1, seed * 17u + (uint32_t)i);
}
/* a small skull in the earth, or a bone */
static void skull_small(int x, int y, float v) {
    static const char *const S[6] = { ".###.", "#####", "#.#.#", "#####", ".#.#.", ".###." };
    for (int j = 0; j < 6; j++) for (int i = 0; i < 5; i++) if (S[j][i] == '#') matf(x + i, y + j, R_BONE, v - 0.04f * j + (i == 0 ? -0.05f : 0), 0);
}
static void bone_small(int x, int y, int len, int dir, float v) {
    for (int i = 0; i < len; i++) matf(x + i, y + dir * i / 3, R_BONE, v, 0);
    matf(x - 1, y - 1, R_BONE, v, 0); matf(x - 1, y + 1, R_BONE, v, 0);
    matf(x + len, y + dir * len / 3 - 1, R_BONE, v, 0); matf(x + len, y + dir * len / 3 + 1, R_BONE, v, 0);
}
/* a coffin, from the side: a long six-sided box, its lid's edge */
static void coffin_side(int x, int y, int w, int h, float v, float open) {
    for (int j = 0; j < h; j++) for (int i = 0; i < w; i++) {
        float u = (float)i / (w - 1);
        int taper = (int)(h * 0.25f * (u < 0.3f ? (0.3f - u) / 0.3f : u > 0.85f ? (u - 0.85f) / 0.15f : 0));
        if (j < taper || j >= h - taper) continue;
        float vv = v + (j == 0 || j == 1 ? 0.08f : 0) - (j == h - 1 ? 0.08f : 0) + 0.05f * h01(i / 4, j, 3);
        int lid = j == 2;
        matf(x + i, y + j - (int)(open * (u > 0.4f ? (u - 0.4f) * 6 : 0)) * (j < 3), R_WOOD, lid ? vv - 0.1f : vv, 0);
    }
}
/* a bat across the sky: two wings flapping */
static void bat(float x, float y, float ph) {
    int up = sinf(ph) > 0;
    mrect((int)x - 1, (int)y, 3, 2, R_IRON, 0.05f, MF_FLAT);
    for (int i = 1; i <= 4; i++) {
        int dy = up ? -(i / 2) : (i / 2);
        matf((int)x - 1 - i, (int)y + dy, R_IRON, 0.05f, MF_FLAT);
        matf((int)x + 1 + i, (int)y + dy, R_IRON, 0.05f, MF_FLAT);
    }
}
/* a flame and its flicker: a few pixels at full brightness, and the light round it */
static float flick(float t, int seed) {
    return 0.82f + 0.1f * sinf(t * 17.0f + seed) + 0.06f * sinf(t * 29.0f + seed * 3) + 0.05f * sinf(t * 7.0f + seed * 5);
}
static void flame(int x, int y, float h, float t, int seed) {
    int hh = (int)(h * (0.85f + 0.2f * sinf(t * 13 + seed)));
    for (int j = 0; j < hh; j++) {
        float u = (float)j / hh;
        int w = u < 0.35f ? 1 : u < 0.75f ? 1 : 0;
        int sway = (int)roundf(sinf(t * 9 + seed + j * 0.7f) * u * 1.2f);
        for (int i = -w; i <= w; i++) direct(x + i + sway, y - j, (uint8_t)(R_FLAME + clampi((int)(15 - u * 9 - abs(i) * 4), 2, 15)));
    }
}

/* ======================================================================================================================
 * A, BENEATH: the graveyard under the moon; under it the earth, its dead and its roots; under that the crypt, and in it
 * a walker with a lantern going by
 * ====================================================================================================================== */
#define A_GROUND 96
#define A_CRYPT0 140
#define A_FLOOR 184
static float a_lantern_x, a_lantern_y, a_flick;
/* the walker, side on, going right: a hooded cloak, a lantern held out on a short chain */
static void walker(float x, int foot, float t, int lantern_out) {
    float step = t / 0.5f * PI;                                       /* a step each half second */
    float bob = fabsf(sinf(step)) * 1.2f;
    int top = foot - 30 + (int)roundf(-bob);
    int cx = (int)roundf(x);
    /* the cloak: shoulders to a hem that swings */
    float sw = sinf(step) * 2.0f;
    for (int y = top + 8; y < foot - 1; y++) {
        float u = (float)(y - top - 8) / (foot - 1 - top - 8);
        float back = 5 + 4 * u + (u > 0.6f ? -sw * (u - 0.6f) * 2 : 0), front = 4 + 3 * u + (u > 0.6f ? sw * (u - 0.6f) * 2 : 0);
        for (int i = -(int)back; i <= (int)front; i++) {
            float v = 0.07f + 0.03f * (fmodf(i + y * 0.3f, 4) < 1) + (i >= (int)front - 1 ? 0.3f : i > 0 ? 0.06f : 0);
            matf(cx + i, y, R_IRON, v, 0);
        }
    }
    /* the feet, in turn */
    int fa = (int)roundf(sinf(step) * 3), fb = -fa;
    mrect(cx + fa, foot - 2, 4, 2, R_IRON, 0.18f, 0);
    mrect(cx + fb - 1, foot - 2, 4, 2, R_IRON, 0.12f, 0);
    /* the hood: round, a point behind, the face in its shadow */
    for (int y = top; y < top + 11; y++) for (int i = -5; i <= 5; i++) {
        float dx = i - 0.5f, dy = y - (top + 5.5f);
        int in = dx * dx + dy * dy <= 26 || (i < -2 && y < top + 4 && y > top - 1 && i > -7 + (y - top));
        if (!in) continue;
        int face = i >= 1 && y >= top + 3 && y <= top + 8 && dx * dx + dy * dy < 14;
        matf(cx + i, y, face ? R_SKIN : R_IRON, face ? (i >= 3 ? 0.22f : 0.04f) : 0.08f + 0.26f * (i > 2), 0);
    }
    /* the arm out, the lantern hanging from the hand, swinging after the step */
    int hx = cx + 9, hy = top + 13;
    mline((float)cx + 2, (float)top + 10, (float)hx, (float)hy, 2.6f, R_IRON, 0.2f, 0);
    matf(hx, hy, R_SKIN, 0.4f, 0);
    float swing = sinf(step - 0.8f) * 0.18f;
    float lx = hx + sinf(swing) * 6, ly = hy + cosf(swing) * 6;
    mline((float)hx, (float)hy, lx, ly - 2, 1, R_IRON, 0.3f, 0);
    mrect((int)lx - 2, (int)ly - 2, 5, 1, R_IRON, 0.35f, 0);
    mrect((int)lx - 2, (int)ly + 5, 5, 1, R_IRON, 0.3f, 0);
    for (int j = -1; j <= 4; j++) { matf((int)lx - 2, (int)ly + j, R_IRON, 0.3f, 0); matf((int)lx + 2, (int)ly + j, R_IRON, 0.3f, 0); }
    for (int j = -1; j <= 4; j++) for (int i = -1; i <= 1; i++) direct((int)lx + i, (int)ly + j, (uint8_t)(R_FLAME + clampi(9 + (j > 1) * 3 - abs(i) * 2, 0, 15)));
    if (lantern_out) { a_lantern_x = lx; a_lantern_y = ly + 2; }
}
static void a_world(float t, float flash, int graves) {
    float mx = graves ? 34 : 196, my = graves ? 22 : 70, mr = graves ? 11 : 14;
    pal_night();
    pal_flash(flash * 0.55f);
    light_clear(1.0f + 0.4f * flash, 0);
    sky(0, A_GROUND, t, mx, my, mr, flash);
    /* the far hills and a church's spire, flat against the sky */
    for (int x = 0; x < W; x++) {
        float hy = 80 + 6 * sinf(x * 0.021f + 1.3f) + 3 * sinf(x * 0.063f) + 2 * fbm(x * 0.05f, 0, 8);
        if (x > 44 && x < 70) hy = fminf(hy, 72);
        if (x > 52 && x < 62) hy = fminf(hy, 64);
        if (x >= 55 && x <= 58) hy = fminf(hy, 50 + (float)abs(x - 56) * 4);
        for (int y = (int)hy; y < A_GROUND; y++) matf(x, y, R_CEIL, 0.1f + 0.02f * (y > 88), MF_NOWARM);
    }
    for (int x = 53; x <= 59; x++) mrect(x, 64, 1, 3, R_CEIL, 0.1f, MF_NOWARM);
    /* the tree, its crows */
    branch(28, A_GROUND, 1.5f, 30, 8, 6, 7);
    branch(28, A_GROUND - 18, 2.5f, 18, 4, 4, 9);
    branch(30, A_GROUND - 24, 0.7f, 16, 3.5f, 4, 13);
    mrect(40, 52, 3, 2, R_IRON, 0.04f, MF_FLAT); matf(43, 51, R_IRON, 0.04f, MF_FLAT); matf(39, 51, R_IRON, 0.04f, MF_FLAT);
    mrect(17, 47, 3, 2, R_IRON, 0.04f, MF_FLAT); matf(16, 46, R_IRON, 0.04f, MF_FLAT);
    /* the stones, an iron fence and the mausoleum */
    static const struct { int x, w, h, shape; float lean; } ST[] = {
        { 84, 11, 16, 0, 0.05f }, { 108, 9, 19, 1, -0.06f }, { 133, 12, 14, 2, 0.0f }, { 160, 10, 17, 0, -0.12f },
        { 186, 13, 12, 3, 0.04f }, { 211, 9, 18, 1, 0.08f }, { 232, 11, 15, 0, 0.0f } };
    for (int i = 0; i < 7; i++) small_stone(ST[i].x, A_GROUND, ST[i].w, ST[i].h, ST[i].shape, ST[i].lean, 1, 30 + i);
    for (int x = 250; x < 318; x += 4) { mrect(x, A_GROUND - 13, 1, 13, R_IRON, 0.12f, 0); matf(x, A_GROUND - 14, R_IRON, 0.2f, 0); }
    mrect(250, A_GROUND - 11, 68, 1, R_IRON, 0.12f, 0); mrect(250, A_GROUND - 4, 68, 1, R_IRON, 0.12f, 0);
    {
        int mx0 = 262, mw = 42, base = A_GROUND;
        for (int y = base - 34; y < base; y++) for (int x = mx0; x < mx0 + mw; x++) {
            int ped = y < base - 24, roof = y < base - 30 + abs(x - (mx0 + mw / 2)) * 6 / (mw / 2);
            if (ped && roof) continue;
            float v = 0.3f + 0.12f * fbm(x * 0.4f, y * 0.4f, 21) + 0.18f * (x > mx0 + mw - 4);
            int col = !ped && ((x - mx0) % 9 < 3) && y > base - 23 && y < base - 2;
            int door = !ped && x > mx0 + 16 && x < mx0 + 26 && y > base - 18;
            if (door) v = 0.03f;
            else if (!ped && !col && y > base - 23 && y < base - 2) v -= 0.12f;
            if (y == base - 24 || y == base - 23) v += 0.15f;
            matf(x, y, R_STONE, v, 0);
        }
    }
    /* the earth: strata, darker going down */
    for (int y = A_GROUND; y < H; y++) for (int x = 0; x < W; x++) {
        float n = fbm(x * 0.07f, y * 0.11f, 51), strata = 0.05f * sinf(y * 0.55f + 2 * fbm(x * 0.02f, y * 0.02f, 52));
        float v = 0.3f + 0.26f * (n - 0.5f) * 1.6f + strata - 0.2f * (float)(y - A_GROUND) / (H - A_GROUND);
        if (y < A_GROUND + 3) v -= 0.1f;
        if (h01(x, y, 53) < 0.03f) v += 0.14f;
        if (h01(x / 2, y / 2, 54) < 0.04f) v -= 0.12f;
        for (int g = 0; g < 3; g++) {                                   /* the graves' shafts: earth turned, looser */
            static const int GX[3] = { 70, 146, 197 }, GY[3] = { 110, 116, 108 };
            if (x >= GX[g] - 2 && x < GX[g] + 30 && y < GY[g] + 10) { v += 0.06f * (h01(x, y, 55 + g) - 0.3f) + 0.03f; if (x == GX[g] - 2 || x == GX[g] + 29) v -= 0.08f; }
        }
        matf(x, y, R_FLOOR, v, 0);
    }
    for (int i = 0; i < 26; i++) {                                     /* stones in the earth */
        int sx = (int)(h01(i, 1, 61) * W), sy = A_GROUND + 8 + (int)(h01(i, 2, 61) * 40);
        float r = 1.5f + h01(i, 3, 61) * 2.5f;
        for (int y = (int)(sy - r); y <= sy + r; y++) for (int x = (int)(sx - r * 1.4f); x <= sx + r * 1.4f; x++)
            if ((x - sx) * (x - sx) / 1.96f + (y - sy) * (y - sy) <= r * r) matf(x, y, R_STONE, 0.22f + 0.12f * (y < sy) + 0.04f * h01(x, y, 4), 0);
    }
    root(28, A_GROUND + 1, 1.6f, 36, 3.5f, 3, 3); root(22, A_GROUND + 1, 2.3f, 28, 2.4f, 3, 4); root(35, A_GROUND + 1, 0.9f, 32, 2.4f, 3, 5);
    /* the buried: coffins under some of the stones, bones loose in the earth */
    coffin_side(70, 110, 28, 8, 0.42f, 0); coffin_side(146, 116, 28, 8, 0.38f, 0); coffin_side(197, 108, 28, 8, 0.4f, 0);
    skull_small(118, 124, 0.42f); skull_small(240, 113, 0.38f); skull_small(52, 128, 0.36f);
    bone_small(124, 128, 6, 1, 0.38f); bone_small(30, 124, 7, -1, 0.34f); bone_small(258, 120, 6, 1, 0.36f); bone_small(176, 130, 5, -1, 0.35f);
    for (int x = 0; x < W; x += 2) {                                    /* grass along the edge */
        int g = (int)(h01(x, 0, 71) * 3);
        for (int j = 1; j <= g; j++) matf(x, A_GROUND - j, R_MOSS, 0.18f + 0.06f * j, 0);
    }
    (void)graves;
}
/* the crypt: arches between piers, its back wall of blocks, its niches; returns nothing, lit later */
static int a_open(int x, int y) {                                       /* in the passage's air (seen against its back wall) */
    if (y >= A_FLOOR || y < A_CRYPT0) return 0;
    int bay = ((x % 64) + 64) % 64;
    if (bay < 6) return y > A_CRYPT0 + 16;                              /* a pier */
    float c = (bay - 35) / 29.0f;
    float top = A_CRYPT0 + 4 + 12 * (1 - sqrtf(fmaxf(0, 1 - c * c)));
    return y >= top;
}
static void a_crypt(float t) {
    for (int y = A_CRYPT0 - 6; y < A_FLOOR + 3; y++) for (int x = 0; x < W; x++) {
        int bay = ((x % 64) + 64) % 64;
        if (a_open(x, y)) {                                             /* the back wall: courses of blocks */
            int row = (y - A_CRYPT0) / 6, off = row & 1 ? 8 : 0;
            int mort = (y - A_CRYPT0) % 6 == 0 || ((x + off) % 16) == 0;
            float v = mort ? 0.12f : 0.3f + 0.12f * fbm(x * 0.2f, y * 0.2f, 81) + 0.05f * h01((x + off) / 16, row, 82);
            matf(x, y, R_STONE, v, 0);
        } else if (y < A_FLOOR) {
            int near = 0;                                               /* the vault's ring of voussoirs round the arch */
            for (int d = 1; d <= 4 && !near; d++) if (a_open(x, y + d)) near = d;
            if (near) matf(x, y, R_STONE, 0.42f - near * 0.04f + ((x / 5) % 2 ? 0.04f : 0) - ((x % 5) == 0 ? 0.12f : 0), 0);
            else if (bay < 6 && y > A_CRYPT0 + 16) matf(x, y, R_STONE, 0.38f + (bay == 5 ? 0.1f : bay == 0 ? -0.12f : 0) + 0.06f * ((y / 6) % 2), 0);
        } else matf(x, y, R_STONE, 0.36f - 0.1f * (y - A_FLOOR) + ((x % 12) == 0 ? -0.1f : 0), 0);
    }
    /* niches in alternate bays, with skulls; a coffin on a ledge in another */
    for (int b = 0; b < 6; b++) {
        int bx = b * 64 + 20;
        if (b % 2 == 0) {
            for (int k = 0; k < 2; k++) {
                int nx = bx + k * 16, ny = A_FLOOR - 26;
                mrect(nx, ny, 12, 9, R_STONE, 0.04f, 0);
                skull_small(nx + 3, ny + 3, 0.5f);
                mrect(nx, ny + 9, 12, 1, R_STONE, 0.45f, 0);
            }
        } else {
            mrect(bx - 2, A_FLOOR - 9, 32, 9, R_STONE, 0.32f, 0);
            mrect(bx - 2, A_FLOOR - 9, 32, 1, R_STONE, 0.46f, 0);
            for (int i = 0; i < 30; i += 6) mrect(bx + i, A_FLOOR - 6, 3, 4, R_STONE, 0.22f, 0);
        }
    }
    /* cobwebs in the corners of the arches */
    for (int b = 0; b <= 5; b++) {
        int px = b * 64 + 6;
        for (int r = 3; r <= 9; r += 3) for (int a = 0; a < 10; a++) {
            float an = a / 9.0f * PI * 0.5f;
            matf(px + (int)(cosf(an) * r), A_CRYPT0 + 17 + (int)(sinf(an) * r) - r, R_BONE, 0.3f, MF_FLAT);
        }
        for (int a = 0; a < 3; a++) mline((float)px, A_CRYPT0 + 8.0f, px + cosf(a * 0.7f) * 10, A_CRYPT0 + 8 + sinf(a * 0.7f) * 10, 1, R_BONE, 0.26f, MF_FLAT);
    }
    /* dust coming down out of the cracks */
    for (int i = 0; i < 14; i++) {
        float x = h01(i, 1, 91) * W, ph = h01(i, 2, 91), sp = 1 + (i % 2);
        float u = frac(t / LOOP * sp + ph);
        float y = A_CRYPT0 + 18 + u * (A_FLOOR - A_CRYPT0 - 18);
        if (a_open((int)x, (int)y)) matf((int)x, (int)y, R_BONE, 0.65f, MF_FLAT);
    }
}
static void scene_a(float t) {
    float flash = flash_k(t, LOOP * 0.55f);
    a_world(t, flash, 0);
    a_crypt(t);
    /* the walker crosses once a loop */
    float wx = -24 + (W + 48) * (t / LOOP);
    walker(wx, A_FLOOR, t, 1);
    /* eyes in the dark of a niche, shut while the light is near */
    float ex = 2 * 64 + 22 + 16 + 3, ey = A_FLOOR - 22;
    int eyes = fabsf(a_lantern_x - ex) > 60 && frac(t / 3.1f) > 0.06f;
    /* light: the moon on the ground, little in the earth, nothing in the crypt but the lantern */
    for (int y = A_GROUND; y < H; y++) for (int x = 0; x < W; x++) {
        float k = 0.85f - 0.55f * smooth(A_GROUND, A_CRYPT0 - 4, (float)y) - 0.24f * smooth(A_CRYPT0 - 10, A_CRYPT0 + 2, (float)y);
        LC[y][x] = k * (1.0f + 0.35f * flash * (y < A_CRYPT0 - 6));
    }
    a_flick = flick(t, 3);
    light_add(a_lantern_x, a_lantern_y, 78 * a_flick, 1.6f, 1);
    light_add(a_lantern_x, a_lantern_y, 20, 0.9f, 1);
    /* the bats over the moon, twice a loop */
    for (int i = 0; i < 2; i++) {
        float u = frac(t / LOOP * 2 + i * 0.37f);
        bat(W + 10 - u * (W + 30), 30 + 12 * sinf(u * 9 + i) + i * 10, t * 26 + i);
    }
    fog(A_GROUND - 22, A_GROUND + 2, t, 1.5f, 33);
    /* lightning on the hills */
    if (flash > 0.7f) {
        float bx = 118, by = 0;
        for (int j = 0; j < 14; j++) {
            float nx = bx + (h01(j, 1, 5) - 0.5f) * 10, ny = by + 6;
            mline(bx, by, nx, ny, 1, R_GLOW, 1.0f, MF_FLAT | MF_NOWARM);
            bx = nx; by = ny;
            if (by > 82) break;
        }
    }
    logo_build("HAUNTED", 0.95f, ".EXE", 0.84f, 1, 11);
    logo_draw((W - LG.w) / 2, 10, LS_BLOOD, 1.0f + 0.25f * flash, A_GROUND - 1, t);
    resolve();
    if (eyes) { F.pix[(int)ey * W + (int)ex] = R_BLOOD + 15; F.pix[(int)ey * W + (int)ex + 2] = R_BLOOD + 15; }
    for (int y = 0; y < H; y++) for (int x = 0; x < W; x++) if (MR[y][x] == R_GLOW && (MF[y][x] & MF_FLAT) && y < A_GROUND) F.pix[y * W + x] = C_WHITE;
    prompt(W / 2, 189, "PRESS ANY BUTTON", 0, t, R_BONE);
    gfx_text8(&F, W - 2 - 8 * 8, 191, "1.0.0B21", R_STONE + 4, 1);
}

/* ---- the graves to choose from: what the ledger's pages would say (made up for the mock-ups) ---- */
typedef struct { const char *num, *who, *l1, *l2, *l3; int used; } Grave;
static const Grave GR[3] = {
    { "I", "WITCHFINDER", "LEVEL 14", "CATACOMBS", "7 NOV 2026", 1 },
    { "II", "ANATOMIST", "LEVEL 1", "AT A DEATH", "9 NOV 2026", 1 },
    { "III", "UNMARKED", "NOTHING", "WRITTEN", "", 0 } };
#define NCHOICE 5                                       /* the three, the unsigned run, a new grave */
static float sel_k(float t, int k) {                     /* how chosen k is: coming up over a third of a second, going as fast */
    float per = LOOP / NCHOICE, u = t - k * per;
    if (k == 0 && t > LOOP - 0.35f) return smooth(LOOP - 0.35f, LOOP, t);   /* the loop comes back round to the first */
    if (u < 0 || u > per + 0.35f) return 0;
    return smooth(0, 0.35f, u) * (1 - smooth(per, per + 0.35f, u));
}
static void text_c(int cx, int y, const char *s, int col) { ui_text8(&F, cx - (int)strlen(s) * 4, y, s, FACE_INSC, col, TX_INLAID); }
static void text16_c(int cx, int y, const char *s, int col) { ui_text16(&F, cx - (int)strlen(s) * 4, y, s, FACE_INSC, col, TX_INLAID, 1); }
static void hints(int y, const char *s) { ui_keys(&F, (W - ui_keys_width(s)) / 2, y, s, R_STONE + 11); }

/* a great headstone: shape 0 round-topped, 1 a pointed gothic arch; a moulded edge, a sunk panel for the words */
static void big_stone(int cx, int base, int w, int h, int shape, uint32_t seed) {
    int x0 = cx - w / 2;
    for (int y = base - h; y < base; y++) for (int x = x0; x < x0 + w; x++) {
        int i = x - x0, j = y - (base - h);
        int in;
        if (shape == 0) { float r = w * 0.5f, cyy = r; in = j >= cyy || ((i - r + 0.5f) * (i - r + 0.5f) + (j - cyy) * (j - cyy) <= r * r); }
        else {                                                          /* a drop arch: two arcs meeting at a point */
            float R = w * 0.75f, sq = 0.6f, spring = sqrtf(R * R - (w * 0.25f) * (w * 0.25f)) * sq, px = i + 0.5f, py = j;
            float d1 = (px - (w - R)) * (px - (w - R)) + (py - spring) * (py - spring) / (sq * sq), d2 = (px - R) * (px - R) + (py - spring) * (py - spring) / (sq * sq);
            in = j >= spring || (d1 <= R * R && d2 <= R * R);
        }
        if (!in) continue;
        float v = 0.38f + 0.18f * fbm(x * 0.12f, y * 0.12f, seed) + 0.05f * (h01(x, y, seed) - 0.5f);
        /* the edge: lit on the right (the moon), shadowed left */
        int e = 0;
        for (int d = 1; d <= 3 && !e; d++) {
            int ii = i - d, jj = j;
            (void)ii; (void)jj;
            if (x - d < x0 || x + d >= x0 + w) e = d;
        }
        if (i < 2) v -= 0.14f; else if (i > w - 3) v += 0.16f;
        /* the sunk panel */
        int pj0 = shape == 0 ? (int)(w * 0.42f) : (int)(w * 0.52f), pj1 = h - 8;
        if (j >= pj0 && j <= pj1 && i >= 6 && i <= w - 7) {
            if (j == pj0 || i == 6) v -= 0.14f;
            else if (j == pj1 || i == w - 7) v += 0.1f;
            else v -= 0.05f;
        }
        int moss = fbm(x * 0.25f, y * 0.25f, seed + 9) > 0.68f && j > h * 0.6f;
        int crack = fabsf(fbm(x * 0.07f, y * 0.07f, seed + 3) - 0.5f) < 0.012f;
        if (crack) v -= 0.2f;
        matf(x, y, moss ? R_MOSS : R_STONE, v, 0);
    }
    for (int x = x0 - 3; x < x0 + w + 3; x++) for (int y = base - 3; y < base; y++)   /* its foot set in turf */
        if (x < x0 || x >= x0 + w || y == base - 1) matf(x, y, R_STONE, 0.3f - 0.05f * (base - y), 0);
}
static void board(int cx, int G) {                                     /* a post and a board nailed across it, for a plot unmarked */
    mline((float)cx, (float)G, (float)cx, (float)G - 70, 6, R_WOOD, 0.3f, 0);
    for (int y = G - 64; y < G - 20; y++) for (int x = cx - 42; x <= cx + 42; x++) {
        float v = 0.4f + 0.1f * sinf(y * 1.3f + fbm(x * 0.1f, y, 4) * 4) - 0.12f * (y == G - 64 || y == G - 21) + (x == cx + 42 ? 0.1f : 0);
        matf(x, y, R_WOOD, v, 0);
    }
    matf(cx - 36, G - 58, R_IRON, 0.5f, 0); matf(cx + 36, G - 58, R_IRON, 0.5f, 0);
}
static void board_words(int cx, int G, float k) {
    text_c(cx, G - 60, GR[2].num, k > 0.5f ? R_GOLD + 11 : R_WOOD + 1);
    text_c(cx, G - 49, GR[2].who, k > 0.5f ? R_BONE + 13 : R_WOOD + 1);
    text_c(cx, G - 39, "NOTHING", k > 0.5f ? R_BONE + 10 : R_WOOD + 1);
    text_c(cx, G - 30, "WRITTEN", k > 0.5f ? R_BONE + 10 : R_WOOD + 1);
}
static void candle(int x, int base, int lit, float t, int seed) {
    mrect(x - 1, base - 7, 3, 7, R_BONE, 0.62f, 0);
    matf(x - 1, base - 7, R_BONE, 0.75f, 0);
    if (lit) flame(x, base - 8, 6, t, seed);
    else matf(x, base - 8, R_IRON, 0.2f, MF_FLAT);
}
static void mist(int cx, int base, float k, float t, uint32_t seed) {    /* a breath of cold mist off a grave */
    if (k <= 0) return;
    for (int y = base - 40; y < base + 1; y++) for (int x = cx - 46; x < cx + 46; x++) {
        float f = fbm((x - cx) * 0.05f, (y + t * 9) * 0.07f, seed) * (1 - fabsf((float)(x - cx)) / 46) * smooth((float)base - 40, (float)base - 4, (float)y);
        if (f * k > 0.36f + bay(x, y) * 0.3f && !(MF[y][x] & MF_DIRECT)) { MR[y][x] = R_GHOST; MV[y][x] = fmaxf(MV[y][x], 0.2f + 0.3f * f * k); MF[y][x] = MF_NOWARM; }
    }
}
static void a_graves(float t) {
    const int G = 124;
    pal_night();
    light_clear(0.75f, 0);
    sky(0, G, t, 34, 26, 11, 0);
    for (int x = 0; x < W; x++) {                                      /* the hills behind */
        float hy = 96 + 5 * sinf(x * 0.025f + 0.4f) + 3 * sinf(x * 0.07f) + 2 * fbm(x * 0.05f, 0, 8);
        for (int y = (int)hy; y < G; y++) matf(x, y, R_CEIL, 0.09f, MF_NOWARM);
    }
    branch(306, G, 1.7f, 26, 6, 5, 12);
    for (int y = G; y < H; y++) for (int x = 0; x < W; x++) {
        float n = fbm(x * 0.07f, y * 0.11f, 51), strata = 0.05f * sinf(y * 0.55f + 2 * fbm(x * 0.02f, y * 0.02f, 52));
        float v = 0.28f + 0.26f * (n - 0.5f) * 1.6f + strata - 0.16f * (float)(y - G) / (H - G);
        if (y < G + 3) v -= 0.1f;
        if (h01(x, y, 53) < 0.03f) v += 0.14f;
        matf(x, y, R_FLOOR, v, 0);
    }
    static const int CX[3] = { 57, 160, 263 };
    for (int g = 0; g < 3; g++) {
        float k = sel_k(t, g);
        if (GR[g].used) {
            big_stone(CX[g], G, 96, g == 0 ? 82 : 92, g, 100 + g);
            coffin_side(CX[g] - 34, G + 10, 68, 12, 0.36f, 0.25f * k);
        } else {                                                        /* the empty plot: a wooden cross, its board blank */
            board(CX[g], G);
            for (int x = CX[g] - 30; x <= CX[g] + 30; x++) {             /* its mound */
                int mh = (int)(6 * sqrtf(fmaxf(0, 1 - powf((x - CX[g]) / 30.0f, 2))));
                for (int y = G - mh; y < G; y++) matf(x, y, R_FLOOR, 0.36f + 0.1f * h01(x, y, 6), 0);
            }
        }
        for (int c = 0; c < 2; c++) candle(CX[g] + (c ? 30 : -30), G + 1, k > 0.5f, t, g * 2 + c);
    }
    for (int x = 0; x < W; x += 2) {                                    /* the turf */
        int gg = (int)(h01(x, 0, 71) * 3);
        for (int j = 1; j <= gg; j++) matf(x, G - j, R_MOSS, 0.2f + 0.06f * j, 0);
    }
    for (int g = 0; g < 3; g++) mist(CX[g], G, sel_k(t, g), t, 300 + g);
    fog(G - 16, G + 1, t, 1.2f, 34);
    /* light: the moon, a candle's warmth on the chosen, the coffin's lid lit from within */
    for (int y = G; y < H; y++) for (int x = 0; x < W; x++) LC[y][x] = 0.55f - 0.25f * (y - G) / (H - G);
    for (int g = 0; g < 3; g++) {
        float k = sel_k(t, g);
        if (k <= 0) continue;
        float fl = flick(t, g);
        light_add(CX[g], G - 10, 70 * fl, 1.0f * k, 1);
        light_add(CX[g], G - 40, 60, 0.35f * k, 0);
        if (GR[g].used) light_add(CX[g], G + 14, 40, 0.8f * k, 0);
    }
    for (int k = 3; k < 5; k++) { float kk = sel_k(t, k); if (kk > 0) light_add(k == 3 ? 85 : 235, 166, 60, 0.5f * kk, 1); }
    resolve();
    /* the coffins' cracks of light while chosen */
    for (int g = 0; g < 2; g++) {
        float k = sel_k(t, g);
        if (k < 0.3f) continue;
        for (int i = 4; i < 64; i += 1) if (h01(i, g, 7) < 0.7f) F.pix[(G + 12) * W + CX[g] - 34 + i] = (uint8_t)(R_GLOW + clampi((int)(6 + 8 * k), 0, 15));
    }
    /* the words: carved on the stones, written on the board; bright on the chosen */
    text16_c(W / 2, 4, "RISE FROM WHICH GRAVE?", R_BONE + 13);
    for (int g = 0; g < 3; g++) {
        float k = sel_k(t, g);
        int top = GR[g].used ? G - (g == 0 ? 82 : 92) + (g == 0 ? 6 : 14) : G - 60;
        int hi = R_BONE + clampi((int)(5 + 10 * k), 0, 15), lo = R_STONE + clampi((int)(6 + 7 * k), 0, 15);
        if (GR[g].used) {
            text16_c(CX[g], top + 4, GR[g].num, k > 0.5f ? R_GOLD + 11 : R_STONE + 8);
            int y = G - 48 + (g == 1 ? 2 : 0);
            text_c(CX[g], y, GR[g].who, hi);
            text_c(CX[g], y + 10, GR[g].l1, lo);
            text_c(CX[g], y + 19, GR[g].l2, g == 1 ? R_BLOOD + clampi((int)(7 + 7 * k), 0, 15) : lo);
            text_c(CX[g], y + 28, GR[g].l3, lo);
        } else {
            (void)top;
            board_words(CX[g], G, k);
        }
    }
    /* the two more: the run the dream was in, and a fresh one */
    static const char *const MORE[2] = { "UNSIGNED: LEVEL 3", "DIG A NEW GRAVE" };
    for (int k = 0; k < 2; k++) {
        float kk = sel_k(t, 3 + k);
        int x = k ? 166 : 6, w = 148;
        ui_row(&F, x, 160, w, 15, kk > 0.5f);
        text_c(x + w / 2, 164, MORE[k], kk > 0.5f ? C_WHITE : R_STONE + 11);
    }
    hints(187, "A/D CHOOSES  ENTER RISES  ESC BACK");
}

/* ======================================================================================================================
 * B, THE WALK: run I's hall in the first person, walked slowly towards its dark, the game's own textures and light
 * table; seen through an arch whose lintel carries the name
 * ====================================================================================================================== */
#define B_PERIOD 4                                       /* the hall's furniture comes round every four cells */
#define B_PROJ 186.0f
#define B_EYE 0.5f
static float b_camx, b_hor;
static float b_torch(float t, int k) { return flick(t, k * 7 + 3); }
static float b_light(float wx, float wy, float wz, float dist, float t) {   /* torches each side, the walker's own glow, fog */
    float l = 0.1f;
    for (int c = (int)floorf(b_camx) - 2; c <= (int)floorf(b_camx) + 14; c++) {
        int k = ((c % B_PERIOD) + B_PERIOD) % B_PERIOD;
        if (k != 0 && k != 2) continue;
        float tx = c + 0.5f, ty = k == 0 ? 1.08f : 2.92f, tz = 0.62f;
        float d2 = (wx - tx) * (wx - tx) + (wy - ty) * (wy - ty) + (wz - tz) * (wz - tz);
        l += 1.5f * b_torch(t, c) / (1 + 3.2f * d2);
    }
    float d2c = (wx - b_camx) * (wx - b_camx) + (wy - 2) * (wy - 2);
    l += 0.35f / (1 + 2.5f * d2c);
    return l * expf(-dist / 5.5f);
}
static uint8_t b_lit(uint8_t texel, float l) {
    if (texel == C_CLEAR) return 0;
    return crawl_cmap[clampi((int)(l * LIGHT_UNIT), 0, NLIGHT - 1)][texel];
}
static int b_wall_tex(int cell, int left, int texx, int texy, float t) {   /* a wall's texel: its stone, what stands on it */
    int k = ((cell % B_PERIOD) + B_PERIOD) % B_PERIOD;
    uint8_t c = crawl_tex[T_MATV(M1_ASHLAR, (cell >> 1) & 1)][texx * TS + texy];
    int over = -1;
    if (left && k == 0) over = T_FEAT + F1_TORCH + ((int)(t * 9) + cell) % 3;
    if (!left && k == 2) over = T_FEAT + F1_TORCH + ((int)(t * 9) + cell + 1) % 3;
    if (left && k == 2) over = T_FEAT + F1_NICHE;
    if (!left && k == 0) over = T_FEAT + F1_SLAB;
    if (!left && k == 1 && (cell / B_PERIOD) % 2) over = T_FEAT + F1_BANNER;
    if (over >= 0) { uint8_t o = crawl_tex[over][texx * TS + texy]; if (o != C_CLEAR) c = o; }
    return c;
}
static float b_depth[W];
static void b_hall(float t, int y0, int y1) {
    float step = b_camx * 2 * PI;                                       /* two steps a cell */
    b_hor = 100 + sinf(step) * 1.2f;
    float camy = 2.0f + sinf(step * 0.5f) * 0.03f;
    for (int x = 0; x < W; x++) {
        float cxs = (2.0f * x / W - 1) * 0.66f, rx = 1, ry = cxs;
        float d = ry < 0 ? (1 - camy) / ry : ry > 0 ? (3 - camy) / ry : 1e9f;
        if (d > 60) d = 60;
        b_depth[x] = d;
        float wx = b_camx + d;
        int cell = (int)floorf(wx), left = ry < 0;
        int texx = (int)(frac(wx) * TS); if (!left) texx = TS - 1 - texx;
        float top = b_hor - B_PROJ * (1 - B_EYE) / d, bot = b_hor + B_PROJ * B_EYE / d;
        for (int y = y0; y < y1; y++) {
            uint8_t c;
            if (y >= top && y < bot) {                                    /* the wall */
                float v = (y - top) / (bot - top);
                int texy = clampi((int)(v * TS), 0, TS - 1);
                uint8_t tx = (uint8_t)b_wall_tex(cell, left, texx, texy, t);
                float wz = 1 - v;
                c = IS_FULLBRIGHT(tx) ? tx : b_lit(tx, b_light(wx, left ? 1.0f : 3.0f, wz, d, t));
            } else {
                int floor = y >= bot;
                float dist = floor ? B_PROJ * B_EYE / (y - b_hor + 0.5f) : B_PROJ * (1 - B_EYE) / (b_hor - y + 0.5f);
                float fx = b_camx + dist * rx, fy = camy + dist * ry;
                int tu = (int)(frac(fx) * TS) & 63, tv = (int)(frac(fy) * TS) & 63;
                uint8_t tx = crawl_tex[floor ? T_FLOORS + FL1_FLAGS : T_CEILS + CE1_RIB][tu * TS + tv];
                c = b_lit(tx, b_light(fx, fy, floor ? 0 : 1, dist, t) * (floor ? 1.0f : 0.8f));
            }
            direct(x, y, c);
        }
    }
    /* dust in the torches' light, drifting */
    for (int i = 0; i < 90; i++) {
        float px = (floorf(b_camx / B_PERIOD) + (i % 3)) * B_PERIOD + h01(i, 1, 13) * B_PERIOD;
        float py = 1.15f + h01(i, 2, 13) * 1.7f, pz = frac(h01(i, 3, 13) + t / LOOP * (1 + i % 2)) ;
        pz = 0.15f + 0.8f * (0.5f + 0.5f * sinf(2 * PI * (h01(i, 3, 13) + t / LOOP * (1 + i % 2))));
        float dx = px - b_camx, dy = py - camy;
        if (dx < 0.3f) continue;
        int sx = (int)((dy / (dx * 0.66f) + 1) * W / 2), sy = (int)(b_hor - (pz - B_EYE) * B_PROJ / dx);
        if (sx < 0 || sx >= W || sy < y0 || sy >= y1 || dx > b_depth[sx]) continue;
        float l = b_light(px, py, pz, dx, t);
        if (l > 0.55f) direct(sx, sy, (uint8_t)(R_BONE + clampi((int)(l * 8), 3, 14)));
    }
}
/* the arch it is seen through: two piers and a lintel of great blocks; the name cut in the lintel */
static int b_arch(int x, int y) {                                       /* 0 the opening, 1 a pier, 2 the lintel */
    if (y < 62) return 2;
    if (x < 24 || x >= W - 24) return 1;
    float c = (x - W * 0.5f) / (W * 0.5f - 24);
    float top = 62 + 10 * (1 - sqrtf(fmaxf(0, 1 - c * c)));               /* the opening's segmental head */
    return y < top ? 2 : 0;
}
static void b_frame(float t, float glow) {
    for (int y = 0; y < H; y++) for (int x = 0; x < W; x++) {
        int a = b_arch(x, y);
        if (!a) continue;
        int row = a == 2 ? y / 31 : (y - 62) / 23, off = row & 1 ? 21 : 0;
        int bw = a == 2 ? 64 : 24;
        int bx = a == 2 ? (x + off) % bw : (x < 24 ? x : x - (W - 24));
        int by = a == 2 ? y % 31 : (y - 62) % 23;
        int joint = by == 0 || bx == 0 || (a == 1 && (bx == 23));
        float v = 0.36f + 0.16f * fbm(x * 0.09f, y * 0.09f, 61) + 0.06f * (h01((x + off) / bw, row, 62) - 0.5f);
        if (joint) v = 0.12f;
        else if (by == 1 || bx == 1) v += 0.1f;
        else if (by >= (a == 2 ? 29 : 21)) v -= 0.08f;
        /* the opening's edge: chamfered, lit from inside */
        int inner = 0;
        for (int d = 1; d <= 3 && !inner; d++) if (!b_arch(x + (x < W / 2 ? d : -d), y) || !b_arch(x, y + d)) inner = d;
        if (inner == 1) v += 0.16f; else if (inner == 2) v -= 0.1f;
        matf(x, y, R_STONE, v, 0);
    }
    light_clear(0.34f, 0);
    for (int y = 0; y < H; y++) for (int x = 0; x < W; x++) {          /* the hall's glow on the arch's inside */
        if (!b_arch(x, y)) continue;
        float dx = fminf((float)abs(x - 24), (float)abs(x - (W - 24))), dy = 72.0f - y;
        float dd = b_arch(x, y) == 2 ? fmaxf(0, dy) : dx;
        LW[y][x] = glow * 0.9f * expf(-dd / 22.0f);
    }
    (void)t;
}
static void scene_b(float t) {
    memcpy(F.pal, BASEPAL, sizeof F.pal);
    b_camx = 0.5f + t / LOOP * B_PERIOD * 2;
    for (int y = 0; y < H; y++) for (int x = 0; x < W; x++) MF[y][x] = 0;
    b_hall(t, 50, H);
    b_frame(t, 0.85f * flick(t, 11));
    logo_build("HAUNTED", 0.95f, ".EXE", 0.84f, 1, 21);
    logo_draw((W - LG.w) / 2, 6, LS_CARVED, 0.95f + 0.1f * flick(t, 4), H - 2, t);
    /* the threshold: a step of dark stone, the prompt cut in it */
    for (int y = 184; y < H; y++) for (int x = 24; x < W - 24; x++) matf(x, y, R_STONE, (y == 184 ? 0.42f : 0.24f) + 0.08f * fbm(x * 0.2f, y * 0.2f, 63), 0);
    resolve();
    prompt(W / 2, 189, "PRESS ANY BUTTON", 0, t, R_BONE);
}

/* the graves in B: a wall of the crypt with three loculi, their slabs inscribed; the chosen one's candle lit, its slab
 * drawn a little aside and a cold light from behind it */
static void scene_b_graves(float t) {
    memcpy(F.pal, BASEPAL, sizeof F.pal);
    static const int CX[3] = { 58, 160, 262 };
    const int NY0 = 42, NY1 = 150;
    float lights[5][4];                                                 /* x, y, strength, warm */
    int nl = 0;
    for (int g = 0; g < 3; g++) { float k = sel_k(t, g); if (k > 0) { lights[nl][0] = CX[g]; lights[nl][1] = NY1 + 4; lights[nl][2] = 1.4f * k * flick(t, g); lights[nl][3] = 1; nl++; } }
    lights[nl][0] = 4; lights[nl][1] = 70; lights[nl][2] = 0.9f * flick(t, 9); lights[nl][3] = 1; nl++;
    lights[nl][0] = W - 4; lights[nl][1] = 70; lights[nl][2] = 0.9f * flick(t, 10); lights[nl][3] = 1; nl++;
    for (int y = 0; y < H; y++) for (int x = 0; x < W; x++) {
        float l = 0.16f;
        for (int i = 0; i < nl; i++) { float dx = x - lights[i][0], dy = (y - lights[i][1]) * 1.2f; l += lights[i][2] / (1 + (dx * dx + dy * dy) / 2600.0f); }
        int tx = ((x % 64) + 64) % 64, ty = y % 64;
        uint8_t c = crawl_tex[T_MATV(M1_ASHLAR, ((x / 64) + (y / 64)) & 1)][tx * TS + ty];
        direct(x, y, b_lit(c, l));
        LC[y][x] = l * 0.5f; LW[y][x] = l * 0.5f;
    }
    for (int g = 0; g < 3; g++) {
        float k = sel_k(t, g);
        int x0 = CX[g] - 44, x1 = CX[g] + 44;
        /* the niche: an arched recess, its rim of dressed stone */
        for (int y = NY0 - 6; y < NY1 + 6; y++) for (int x = x0 - 6; x < x1 + 6; x++) {
            float c = (x - CX[g]) / 50.0f, top = NY0 - 6 + 20 * (1 - sqrtf(fmaxf(0, 1 - c * c)));
            if (y < top) continue;
            float ci = (x - CX[g]) / 44.0f, itop = NY0 + 20 * (1 - sqrtf(fmaxf(0, 1 - ci * ci)));
            int inside = x >= x0 && x < x1 && y >= itop && y < NY1;
            if (!inside) { matf(x, y, R_STONE, 0.5f + 0.1f * (y < NY0 + 10) - 0.15f * (y >= NY1) + 0.06f * fbm(x * 0.2f, y * 0.2f, 70), 0); continue; }
            matf(x, y, R_STONE, 0.05f, 0);
        }
        if (GR[g].used) {                                               /* the slab, drawn aside a little while chosen */
            int dxs = (int)(10 * k);
            for (int y = NY0 + 16; y < NY1 - 2; y++) for (int x = x0 + 4; x < x1 - 4; x++) {
                int X = x + dxs;
                if (X >= x1 - 1) continue;
                int e = x == x0 + 4 || y == NY0 + 16 ? 1 : x == x1 - 5 || y == NY1 - 3 ? -1 : 0;
                float v = 0.52f + 0.12f * fbm(x * 0.15f, y * 0.15f, 80 + g) + 0.12f * e;
                if (fabsf(fbm(x * 0.05f, y * 0.05f, 90 + g) - 0.5f) < 0.01f) v -= 0.2f;
                matf(X, y, R_STONE, v, 0);
            }
            for (int y = NY0 + 16; y < NY1 - 2 && dxs > 0; y++) for (int x = x0 + 4; x < x0 + 4 + dxs; x++)   /* the cold behind it */
                if (x >= x0 + 2 + dxs - 2 || h01(x, y + (int)(t * 6), 3) < 0.04f * k) matf(x, y, R_GLOW, 0.12f + 0.2f * k, MF_NOWARM | MF_FLAT);
                else matf(x, y, R_STONE, 0.02f, 0);
            for (int i = 0; i < 3; i++) mline((float)x0 + 6, NY0 + 2.0f + i * 3, x0 + 18.0f - i * 2, NY0 + 12.0f, 1, R_BONE, 0.35f, MF_FLAT);
        } else {                                                        /* empty: a cobweb, bones of no one */
            for (int r = 4; r <= 16; r += 4) for (int a = 0; a <= 12; a++) { float an = a / 12.0f * PI * 0.5f; matf(x1 - 1 - (int)(cosf(an) * r), NY0 + 14 + (int)(sinf(an) * r) - 2, R_BONE, 0.4f, MF_FLAT); }
            bone_small(CX[g] - 10, NY1 - 4, 8, 0, 0.4f); skull_small(CX[g] + 6, NY1 - 8, 0.45f);
        }
        mrect(x0 - 8, NY1, 104, 4, R_STONE, 0.56f, 0);                   /* the ledge, a candle on it */
        mrect(x0 - 8, NY1 + 4, 104, 2, R_STONE, 0.3f, 0);
        candle(CX[g], NY1, k > 0.5f, t, g);
    }
    for (int y = 0; y < H; y++) for (int x = 0; x < W; x++) if (!(MF[y][x] & MF_DIRECT)) { float l = LC[y][x] + LW[y][x]; LC[y][x] = l * 0.45f; LW[y][x] = l * 0.55f; }
    for (int g = 0; g < 3; g++) { float k = sel_k(t, g); if (k > 0 && GR[g].used) light_add(CX[g] - 30, (NY0 + NY1) * 0.5f, 40, 0.8f * k, 0); }
    ui_band(&F, 0, 0, 0, 0);
    resolve();
    ui_band(&F, 60, 4, 200, 20);
    text16_c(W / 2, 6, "RISE FROM WHICH GRAVE?", R_BONE + 13);
    for (int g = 0; g < 3; g++) {
        float k = sel_k(t, g);
        int dxs = (int)(10 * k), y = NY0 + 22;
        if (GR[g].used) {
            int hi = R_BONE + clampi((int)(6 + 9 * k), 0, 15), lo = R_STONE + clampi((int)(4 + 8 * k), 0, 15);
            ui_text16(&F, CX[g] + dxs - (int)strlen(GR[g].num) * 4, y, GR[g].num, FACE_INSC, 0, TX_CUT, 1);
            text_c(CX[g] + dxs, y + 22, GR[g].who, hi);
            text_c(CX[g] + dxs, y + 33, GR[g].l1, lo);
            text_c(CX[g] + dxs, y + 43, GR[g].l2, g == 1 ? R_BLOOD + clampi((int)(7 + 7 * k), 0, 15) : lo);
            text_c(CX[g] + dxs, y + 53, GR[g].l3, lo);
        } else {
            text16_c(CX[g], y, GR[g].num, R_STONE + clampi((int)(6 + 6 * k), 0, 15));
            text_c(CX[g], y + 22, GR[g].who, R_STONE + clampi((int)(5 + 8 * k), 0, 15));
            text_c(CX[g], y + 33, "NOTHING", R_STONE + clampi((int)(4 + 6 * k), 0, 15));
            text_c(CX[g], y + 43, "WRITTEN", R_STONE + clampi((int)(4 + 6 * k), 0, 15));
        }
    }
    static const char *const MORE[2] = { "UNSIGNED: LEVEL 3", "DIG A NEW GRAVE" };
    for (int k = 0; k < 2; k++) {
        float kk = sel_k(t, 3 + k);
        int x = k ? 166 : 6, w = 148;
        ui_row(&F, x, 164, w, 15, kk > 0.5f);
        text_c(x + w / 2, 168, MORE[k], kk > 0.5f ? C_WHITE : R_STONE + 11);
    }
    hints(189, "A/D CHOOSES  ENTER RISES  ESC BACK");
}

/* ======================================================================================================================
 * C, THE RISEN: one stone close, the name cut in it and bleeding; candles at its foot; out of the grave before it a hand
 * claws at the air, earth falling off it
 * ====================================================================================================================== */
/* a limb's piece: a capsule from a to b, r0 to r1 thick, shaded round its length, lit from above left; nothing under
 * the ground line gy */
static void capsule(float ax, float ay, float bx, float by, float r0, float r1, int ramp, float v, int gy, uint32_t seed) {
    float dx = bx - ax, dy = by - ay, l2 = dx * dx + dy * dy + 1e-6f;
    int x0 = (int)floorf(fminf(ax, bx) - fmaxf(r0, r1) - 1), x1 = (int)ceilf(fmaxf(ax, bx) + fmaxf(r0, r1) + 1);
    int y0 = (int)floorf(fminf(ay, by) - fmaxf(r0, r1) - 1), y1 = (int)ceilf(fmaxf(ay, by) + fmaxf(r0, r1) + 1);
    float l = sqrtf(l2), nx = -dy / l, ny = dx / l;
    if (nx * -0.6f + ny * -0.8f < 0) { nx = -nx; ny = -ny; }        /* the normal that faces the light */
    for (int y = y0; y <= y1; y++) for (int x = x0; x <= x1; x++) {
        if (y >= gy) continue;
        float px = x + 0.5f - ax, py = y + 0.5f - ay;
        float u = clampf((px * dx + py * dy) / l2, 0, 1);
        float r = r0 + (r1 - r0) * u;
        float qx = px - dx * u, qy = py - dy * u, d = sqrtf(qx * qx + qy * qy);
        if (d > r) continue;
        float side = (qx * nx + qy * ny) / (r + 1e-6f);                 /* +1 the lit edge, -1 the dark */
        float vv = v * (0.72f + 0.38f * side) + 0.05f * (h01(x, y, seed) - 0.5f);
        if (d > r - 0.8f && side < -0.2f) vv *= 0.55f;                  /* its outline in shadow */
        int rot = fbm(x * 0.35f, y * 0.35f, seed) > 0.64f;
        matf(x, y, rot ? R_MOSS : ramp, rot ? vv * 0.9f : vv, 0);
    }
}
/* the hand: out of the earth at (gx, gy), its wrist rise above it; fingers curled by curl (0 open, 1 a fist) */
static float hand_tips[5][2];
static void hand(float gx, int gy, float sc, float rise, float curl, float t, uint32_t seed) {
    if (rise <= 0) return;
    float lean = -0.18f + 0.06f * sinf(t * 1.3f);
    float wx = gx + sinf(lean) * 40 * sc * rise, wy = gy - cosf(lean) * 40 * sc * rise;
    capsule(gx + 2 * sc, gy + 6, wx, wy, 6.0f * sc, 5.2f * sc, R_WARM, 0.36f, gy, seed);   /* the forearm, rags at the wrist */
    for (int i = 0; i < 6; i++) {
        float a = lean - PI / 2 + (i - 2.5f) * 0.32f;
        float rx = wx + cosf(a + PI) * 4 * sc * 0, ry = wy + 6 * sc;
        mline(rx + (i - 2.5f) * 2 * sc, ry, rx + (i - 2.5f) * 2.6f * sc + sinf(t * 3 + i) * sc, ry + 7 * sc + h01(i, 0, seed) * 5 * sc, 1.4f * sc, R_FLOOR, 0.3f, 0);
    }
    float ux = sinf(lean), uy = -cosf(lean), rx = cosf(lean), ry = sinf(lean);   /* up the hand, and across it */
    float kx = wx + ux * 13 * sc, ky = wy + uy * 13 * sc;               /* the knuckles */
    /* the palm: a wedge from the wrist out to the knuckles */
    for (int k = 0; k <= 6; k++) {
        float u = k / 6.0f;
        float cx = wx + ux * 13 * sc * u, cy = wy + uy * 13 * sc * u, half = (4.6f + 2.4f * u) * sc;
        capsule(cx - rx * half * 0.6f, cy - ry * half * 0.6f, cx + rx * half * 0.6f, cy + ry * half * 0.6f, 2.4f * sc, 2.4f * sc, R_WARM, 0.4f + 0.04f * u, gy, seed + 1);
    }
    static const float BASE[4] = { -5.2f, -1.8f, 1.6f, 4.8f }, LEN[4] = { 0.92f, 1.06f, 1.0f, 0.78f }, SPREAD[4] = { -0.34f, -0.11f, 0.1f, 0.3f };
    for (int f = 0; f < 4; f++) {
        float x = kx + rx * BASE[f] * sc, y = ky + ry * BASE[f] * sc;
        float a = lean + SPREAD[f] * (1 - 0.5f * curl), segs[3] = { 8, 6, 4.5f };
        float c = curl * (0.85f + 0.3f * h01(f, 3, seed)) + 0.08f * sinf(t * 7 + f * 1.7f);
        for (int j = 0; j < 3; j++) {
            a += c * (0.55f + 0.25f * j);                               /* the claw bends forward, to the right */
            float l = segs[j] * LEN[f] * sc;
            float nx = x + sinf(a) * l, ny = y - cosf(a) * l;
            float r = (2.0f - 0.35f * j) * sc;
            capsule(x, y, nx, ny, r, r * 0.88f, j == 2 ? R_BONE : R_WARM, j == 2 ? 0.5f : 0.42f, gy, seed + 3 + f);
            x = nx; y = ny;
        }
        matf((int)x, (int)y, R_BONE, 0.75f, 0);                         /* the nail gone, the bone's tip */
        hand_tips[f][0] = x; hand_tips[f][1] = y;
    }
    float tx = wx + ux * 5 * sc + rx * 6 * sc, ty = wy + uy * 5 * sc + ry * 6 * sc, ta = lean + 0.9f - 0.5f * curl;   /* the thumb */
    for (int j = 0; j < 2; j++) {
        float l = (j ? 5.5f : 7) * sc, nx = tx + sinf(ta) * l, ny = ty - cosf(ta) * l;
        capsule(tx, ty, nx, ny, 2.3f * sc, 2.0f * sc, j ? R_BONE : R_WARM, j ? 0.5f : 0.42f, gy, seed + 9);
        tx = nx; ty = ny; ta -= 0.5f * curl + 0.2f;
    }
    hand_tips[4][0] = tx; hand_tips[4][1] = ty;
}
/* earth off the hand, falling back: little clods dropping from its fingers and wrist, a whole number of times a loop */
static void clods(float gx, int gy, float t, float sc, int n) {
    for (int i = 0; i < n; i++) {
        float u = frac(t / LOOP * (2 + i % 3) + h01(i, 1, 5));
        int f = i % 5;
        float sx = hand_tips[f][0] + (h01(i, 2, 5) - 0.5f) * 6 * sc, sy = hand_tips[f][1] + 4 * sc;
        if (i % 4 == 3) { sx = gx + (h01(i, 3, 5) - 0.5f) * 12 * sc; sy = gy - 30 * sc; }
        float y = sy + 0.5f * 200 * (u * 0.6f) * (u * 0.6f) * 3;
        if (y >= gy) continue;
        matf((int)sx, (int)y, R_FLOOR, 0.4f, MF_FLAT);
        if (i % 2) matf((int)sx + 1, (int)y, R_FLOOR, 0.3f, MF_FLAT);
    }
}
static void leaf(float x, float y, float ph, int col) {               /* an autumn leaf turning over as it goes */
    int w = (int)(1.5f + 1.5f * fabsf(sinf(ph)));
    for (int i = -w; i <= w; i++) matf((int)x + i, (int)y + (i * (sinf(ph * 0.5f) > 0 ? 1 : -1)) / 2, col, 0.55f + 0.1f * (i == 0), MF_FLAT);
}
static void scene_c(float t) {
    float flash = flash_k(t, LOOP * 0.62f);
    const int G = 160;
    pal_night();
    pal_flash(flash * 0.5f);
    light_clear(0.62f + 0.4f * flash, 0);
    sky(0, G, t, 46, 34, 16, flash);
    /* the far graveyard, flat against the hills */
    for (int x = 0; x < W; x++) {
        float hy = 128 + 5 * sinf(x * 0.03f + 2) + 2 * fbm(x * 0.06f, 0, 18);
        for (int y = (int)hy; y < G; y++) matf(x, y, R_CEIL, 0.1f, MF_NOWARM);
    }
    for (int i = 0; i < 16; i++) {
        int x = 8 + i * 20 + (int)(h01(i, 1, 8) * 8), h = 5 + (int)(h01(i, 2, 8) * 7), sh = (int)(h01(i, 3, 8) * 3);
        if (x > 50 && x < 270) continue;
        for (int y = G - 26 - h; y < G - 26; y++) for (int dx = -2; dx <= 2; dx++) {
            int in = sh == 1 ? (abs(dx) <= 0 || (y == G - 26 - h + 2 && abs(dx) <= 2)) : abs(dx) <= 2 - (y < G - 26 - h + 1 && abs(dx) == 2);
            if (in) matf(x + dx, y, R_CEIL, 0.04f, MF_NOWARM);
        }
    }
    branch(24, G, 1.45f, 34, 7, 6, 31);
    branch(24, G - 24, 0.6f, 18, 3.5f, 4, 37);
    /* the ground */
    for (int y = G - 6; y < H; y++) for (int x = 0; x < W; x++) {
        float n = fbm(x * 0.04f, y * 0.08f, 51);
        float v = 0.16f + 0.1f * n + 0.04f * (y - G) / 40.0f + 0.04f * (h01(x, y, 52) - 0.5f);
        if (y < G) { if (h01(x, 0, 3) * 6 < G - y) continue; matf(x, y, R_MOSS, 0.12f + 0.04f * (G - y), 0); continue; }
        matf(x, y, n > 0.68f ? R_FLOOR : R_MOSS, n > 0.68f ? v + 0.04f : v, 0);
    }
    /* the stone: a great round-headed slab, a skull carved in its head */
    const int SW = 210, SH = 132, SX = W / 2 - SW / 2, SB = G + 2;
    for (int y = SB - SH; y < SB; y++) for (int x = SX; x < SX + SW; x++) {
        int i = x - SX, j = y - (SB - SH);
        float r = SW * 0.5f;
        float ey = 34;                                                   /* its head: a flattened half-round */
        int in = j >= ey || ((i - r + 0.5f) * (i - r + 0.5f) / (r * r) + (j - ey) * (j - ey) / (ey * ey) <= 1);
        if (!in) continue;
        float v = 0.3f + 0.18f * fbm(x * 0.07f, y * 0.07f, 101) + 0.05f * (h01(x, y, 102) - 0.5f);
        int edge = 0;
        for (int d = 1; d <= 3 && !edge; d++) {
            int ii = i - d, jj = j - d;
            int up = jj >= ey || ((ii - r + 0.5f) * 0 + 1) ? 0 : 0; (void)up;
            float e1 = (i - d - r + 0.5f), e2 = (i + d - r + 0.5f);
            int o1 = !(j >= ey || (e1 * e1 / (r * r) + (j - ey) * (j - ey) / (ey * ey) <= 1)) || i - d < 0;
            int o2 = !(j >= ey || (e2 * e2 / (r * r) + (j - ey) * (j - ey) / (ey * ey) <= 1)) || i + d >= SW;
            int o3 = jj < 0 || !(jj >= ey || ((i - r + 0.5f) * (i - r + 0.5f) / (r * r) + (jj - ey) * (jj - ey) / (ey * ey) <= 1));
            if (o1 || o3) edge = d; else if (o2) edge = -d;
        }
        if (edge > 0) v += edge == 1 ? 0.2f : 0.1f;
        else if (edge < 0) v -= edge == -1 ? 0.18f : 0.08f;
        if (fabsf(fbm(x * 0.04f, y * 0.04f, 103) - 0.5f) < 0.008f) v -= 0.22f;   /* cracks */
        int moss = fbm(x * 0.2f, y * 0.2f, 104) > 0.66f && (j > SH - 30 || i < 14 || i > SW - 14);
        if (j >= SH - 3) v -= 0.1f;
        matf(x, y, moss ? R_MOSS : R_STONE, moss ? v * 0.7f : v, 0);
    }
    /* the skull carved in its head, sunk and shadowed */
    {
        static const char *const SK[] = {
            "....#######....", "..###########..", ".#############.", "###############", "###...###...###", "##.....#.....##",
            "##....###....##", "###..##.##..###", ".######.######.", "..####...####..", "...#.#.#.#.#...", "...#########...", "....#.#.#.#...." };
        int kx = W / 2 - 15, ky = SB - SH + 9;
        for (int j = 0; j < 13; j++) for (int i = 0; i < 15; i++) for (int b = 0; b < 4; b++) {
            int x = kx + i * 2 + (b & 1), y = ky + j * 2 + (b >> 1);
            if (SK[j][i] == '#') {
                int lit = j > 0 && SK[j - 1][i] != '#' ? -1 : (j < 12 && SK[j + 1][i] != '#') ? 1 : 0;
                MV[y][x] = clampf(MV[y][x] - 0.06f + 0.08f * lit, 0, 1);
            } else if ((j > 0 && SK[j - 1][i] == '#') || (i > 0 && SK[j][i - 1] == '#')) MV[y][x] = 0.12f;   /* the hollows: the eyes, the nose */
        }
    }
    /* the mound before it, and the hand out of it */
    for (int x = 200; x < 320; x++) {
        int mh = (int)(9 * sqrtf(fmaxf(0, 1 - powf((x - 284) / 60.0f, 2))));
        for (int y = G + 14 - mh; y < G + 16; y++) matf(x, y, R_FLOOR, 0.3f + 0.14f * fbm(x * 0.3f, y * 0.3f, 7) + 0.1f * (y == G + 14 - mh), 0);
    }
    float u = t / LOOP;
    float curl = 0.2f + 0.62f * (0.5f - 0.5f * cosf(2 * PI * u * 2)), rise = 0.92f + 0.08f * sinf(2 * PI * u);
    const float HX = 288;
    hand(HX, G + 12, 1.05f, rise, curl, t, 77);
    clods(HX, G + 12, t, 1.05f, 14);
    for (int i = 0; i < 9; i++) {                                       /* the earth cracked open round it */
        float a = i / 9.0f * PI;
        mline(HX - cosf(a) * 7, G + 12.0f - sinf(a) * 2, HX - cosf(a) * (14 + 6 * h01(i, 1, 2)), G + 12.0f - sinf(a) * 3, 1, R_FLOOR, 0.08f, 0);
    }
    /* candles at the stone's foot */
    static const int CA[6] = { 62, 70, 78, 242, 250, 258 }, CH[6] = { 0, 3, 1, 2, 0, 3 };
    for (int i = 0; i < 6; i++) { mrect(CA[i] - 1, G + 2 - 9 - CH[i], 3, 9 + CH[i], R_BONE, 0.6f, 0); matf(CA[i] - 1, G + 2 - 9 - CH[i], R_BONE, 0.75f, 0); }
    /* light: the moon, the candles' warmth on the stone's foot */
    for (int y = 0; y < H; y++) for (int x = 0; x < W; x++) if (y > G) LC[y][x] *= 0.75f;
    for (int i = 0; i < 6; i += 2) { float fl = flick(t, i); light_add(CA[i] + 8, G - 8, 110 * fl, 0.75f, 1); }
    for (int i = 3; i < 6; i += 2) { float fl = flick(t, i); light_add(CA[i] - 8, G - 8, 110 * fl, 0.75f, 1); }
    /* the leaves, the bats */
    for (int i = 0; i < 7; i++) {
        float lu = frac(t / LOOP * (1 + i % 2) + h01(i, 1, 44));
        float lx = W + 8 - lu * (W + 16), ly = 20 + h01(i, 2, 44) * 120 + lu * 30 + 6 * sinf(lu * 12 + i);
        static const int LC_[3] = { R_BRICK, R_WOOD, R_GOLD };
        leaf(lx, ly, lu * 30 + i, LC_[i % 3]);
    }
    { float bu = frac(t / LOOP + 0.2f); bat(W + 10 - bu * (W + 30), 24 + 10 * sinf(bu * 8), t * 26); }
    fog(G - 16, G + 6, t, 1.2f, 35);
    logo_build("HAUNTED", 0.8f, ".EXE", 0.76f, 0, 31);
    logo_draw((W - LG.w) / 2, SB - SH + 40, LS_CARVED, 1.0f + 0.2f * flash, G + 1, t);
    resolve();
    for (int i = 0; i < 6; i++) flame(CA[i], G + 2 - 10 - CH[i], 6, t, i);
    for (int y = 0; y < H; y++) for (int x = 0; x < W; x++) if (MF[y][x] & MF_DIRECT) F.pix[y * W + x] = MD[y][x];
    prompt(W / 2, 186, "PRESS ANY BUTTON", 1, t, R_BONE);
}

/* the graves in C: three stones close, a mound before each; out of the chosen one's a hand comes up, its candles lit; the
 * empty plot an open pit, a spade in its earth */
static void scene_c_graves(float t) {
    const int G = 112, M = G + 40;                                      /* the stones' foot; the mounds' */
    pal_night();
    light_clear(0.55f, 0);
    sky(0, G, t, 290, 26, 11, 0);
    for (int x = 0; x < W; x++) {
        float hy = 100 + 5 * sinf(x * 0.03f + 2) + 2 * fbm(x * 0.06f, 0, 18);
        for (int y = (int)hy; y < G; y++) matf(x, y, R_CEIL, 0.1f, MF_NOWARM);
    }
    for (int y = G - 6; y < H; y++) for (int x = 0; x < W; x++) {
        float n = fbm(x * 0.04f, y * 0.08f, 51);
        float v = 0.16f + 0.1f * n + 0.04f * (h01(x, y, 52) - 0.5f);
        if (y < G) { if (h01(x, 0, 3) * 6 < G - y) continue; matf(x, y, R_MOSS, 0.12f + 0.04f * (G - y), 0); continue; }
        matf(x, y, n > 0.68f ? R_FLOOR : R_MOSS, n > 0.68f ? v + 0.04f : v, 0);
    }
    static const int CX[3] = { 57, 160, 263 };
    for (int g = 0; g < 3; g++) {
        float k = sel_k(t, g);
        if (GR[g].used) big_stone(CX[g], G, 96, g == 0 ? 80 : 88, g, 100 + g);
        else board(CX[g], G);
        if (GR[g].used) for (int x = CX[g] - 46; x <= CX[g] + 46; x++) {  /* the grave's mound before the stone */
            float m = fmaxf(0, 1 - powf((x - CX[g]) / 46.0f, 2));
            int mh = (int)(8 * sqrtf(m));
            for (int y = M - mh; y < M + 2; y++) matf(x, y, R_FLOOR, 0.26f + 0.12f * fbm(x * 0.3f, y * 0.3f, 7 + g) + 0.1f * (y == M - mh), 0);
        } else {                                                        /* the open pit and its earth, the spade */
            for (int x = CX[g] - 46; x < CX[g] + 46; x++) {
                int mh = (int)(10 * sqrtf(fmaxf(0, 1 - powf((x - CX[g]) / 46.0f, 2))));
                for (int y = M - 12 - mh; y < M - 12; y++) matf(x, y, R_FLOOR, 0.3f + 0.14f * fbm(x * 0.25f, y * 0.25f, 19) + 0.08f * (y == M - 12 - mh), 0);
            }
            for (int y = M - 12; y < M + 1; y++) for (int x = CX[g] - 34; x < CX[g] + 34; x++) {
                float e = fminf(fminf((float)(x - CX[g] + 34), (float)(CX[g] + 33 - x)), (float)(M - y));
                matf(x, y, R_FLOOR, y == M - 12 ? 0.42f : 0.03f + 0.02f * fminf(e, 3), 0);
            }
            mline(CX[g] + 30.0f, M - 16.0f, CX[g] + 38.0f, M - 52.0f, 2, R_WOOD, 0.4f, 0);   /* the spade's haft, its blade in the heap */
            mrect(CX[g] + 34, M - 56, 8, 2, R_WOOD, 0.4f, 0);
            for (int y = M - 18; y < M - 10; y++) for (int x = CX[g] + 26; x < CX[g] + 34; x++) if (y >= M - 14 + (x - CX[g] - 26) / 3) matf(x, y, R_IRON, 0.45f + 0.1f * (x == CX[g] + 27), 0);
        }
        float rise = smooth(0.1f, 0.9f, k);
        if (GR[g].used && rise > 0) {
            float curl = 0.15f + 0.5f * (0.5f - 0.5f * cosf(2 * PI * t * 0.8f));
            hand(CX[g] + 4, M + 1, 0.82f, rise * 0.72f, curl, t, 70 + g);
            clods(CX[g] + 4, M + 1, t, 0.82f, 8 * (rise > 0.6f));
        }
        for (int c = 0; c < 2; c++) candle(CX[g] + (c ? 40 : -40), M - 2, k > 0.5f, t, g * 2 + c);
    }
    for (int g = 0; g < 3; g++) mist(CX[g], M, sel_k(t, g), t, 300 + g);
    for (int y = G; y < H; y++) for (int x = 0; x < W; x++) LC[y][x] = 0.42f;
    for (int g = 0; g < 3; g++) {
        float k = sel_k(t, g);
        if (k <= 0) continue;
        float fl = flick(t, g);
        light_add(CX[g], M - 8, 76 * fl, 1.0f * k, 1);
        light_add(CX[g], G - 40, 60, 0.3f * k, 0);
    }
    for (int k = 3; k < 5; k++) { float kk = sel_k(t, k); if (kk > 0) light_add(k == 3 ? 85 : 235, 168, 60, 0.5f * kk, 1); }
    resolve();
    text16_c(W / 2, 4, "RISE FROM WHICH GRAVE?", R_BONE + 13);
    for (int g = 0; g < 3; g++) {
        float k = sel_k(t, g);
        int hi = R_BONE + clampi((int)(5 + 10 * k), 0, 15), lo = R_STONE + clampi((int)(6 + 7 * k), 0, 15);
        if (GR[g].used) {
            int top = G - (g == 0 ? 80 : 88) + (g == 0 ? 5 : 12);
            text16_c(CX[g], top + 4, GR[g].num, k > 0.5f ? R_GOLD + 11 : R_STONE + 8);
            int y = G - 48 + (g == 1 ? 2 : 0);
            text_c(CX[g], y, GR[g].who, hi);
            text_c(CX[g], y + 10, GR[g].l1, lo);
            text_c(CX[g], y + 19, GR[g].l2, g == 1 ? R_BLOOD + clampi((int)(7 + 7 * k), 0, 15) : lo);
            text_c(CX[g], y + 28, GR[g].l3, lo);
        } else board_words(CX[g], G, k);
    }
    static const char *const MORE[2] = { "UNSIGNED: LEVEL 3", "DIG A NEW GRAVE" };
    for (int k = 0; k < 2; k++) {
        float kk = sel_k(t, 3 + k);
        int x = k ? 166 : 6, w = 148;
        ui_row(&F, x, 165, w, 15, kk > 0.5f);
        text_c(x + w / 2, 169, MORE[k], kk > 0.5f ? C_WHITE : R_STONE + 11);
    }
    hints(189, "A/D CHOOSES  ENTER RISES  ESC BACK");
}

/* ======================================================================================================================
 * The logo alone, at its sizes and styles: for checking the letters
 * ====================================================================================================================== */
static void logo_sheet(const char *dir) {
    char p[512];
    LOOP = 6;
    sw = W * 2 + 32; sh = (H * 2 + 40) * 2 + 16;
    sheet = calloc((size_t)sw * sh, 3);
    for (int k = 0; k < 2; k++) {
        T = k ? 2.5f : 0.7f;
        pal_night();
        light_clear(1, 0);
        for (int y = 0; y < H; y++) for (int x = 0; x < W; x++) matf(x, y, k ? R_STONE : R_CEIL, k ? 0.35f + 0.15f * fbm(x * 0.1f, y * 0.1f, 3) : 0.15f, 0);
        logo_build("HAUNTED", 0.95f, ".EXE", 0.84f, 1, 11);
        logo_draw((W - LG.w) / 2, 20, k ? LS_CARVED : LS_BLOOD, 1, k ? 0 : 150, T);
        logo_build("HAUNTED", 1.3f, ".EXE", 0.95f, 0, 12);
        logo_draw((W - LG.w) / 2, 80, k ? LS_CARVED : LS_BLOOD, 1, k ? 0 : 196, T);
        resolve();
        sheet_label(16, 8 + k * (H * 2 + 40), k ? "CUT IN STONE" : "IN BLOOD");
        sheet_put(16, 32 + k * (H * 2 + 40));
    }
    snprintf(p, sizeof p, "%s/logo.png", dir);
    write_png(p, sheet, sw, sh);
    free(sheet);
}

/* a mock-up run round its loop into a GIF, with four stills from it into <name>.png */
static void run(const char *dir, const char *name, void (*fn)(float), float loop) {
    char p[512];
    LOOP = loop;
    snprintf(p, sizeof p, "%s/%s.gif", dir, name);
    const char *sc = getenv("TM_SCALE");
    if (!gif_begin(p, sc ? clampi(atoi(sc), 1, 4) : 2)) { fprintf(stderr, "can't write %s\n", p); return; }
    int n = (int)roundf(loop * FPS);
    sw = (W * 2 + 8) * 2 + 8; sh = (H * 2 + 8) * 2 + 8;
    sheet = calloc((size_t)sw * sh, 3);
    for (int i = 0; i < n; i++) {
        T = (float)i / FPS;
        fn(T);
        gif_frame();
        int k = i * 4 / n;
        if (i == k * n / 4 + n / 8) sheet_put(8 + (k % 2) * (W * 2 + 8), 8 + (k / 2) * (H * 2 + 8));
    }
    gif_end();
    snprintf(p, sizeof p, "%s/%s.png", dir, name);
    write_png(p, sheet, sw, sh);
    free(sheet);
    printf("%s: %d frames\n", name, n);
}

static void still(const char *path, void (*fn)(float), float loop, float t) {
    LOOP = loop; T = t;
    fn(t);
    sw = W * 2; sh = H * 2;
    sheet = calloc((size_t)sw * sh, 3);
    sheet_put(0, 0);
    write_png(path, sheet, sw, sh);
    free(sheet);
}
/* the six side by side, a still each, labelled: the overview */
static void overview(const char *dir) {
    static const struct { const char *label; void (*fn)(float); float loop, t; } V[6] = {
        { "A  BENEATH", scene_a, 12, 4.0f }, { "A  ITS GRAVES", a_graves, 12, 1.2f },
        { "B  THE WALK", scene_b, 8, 2.0f }, { "B  ITS GRAVES", scene_b_graves, 12, 3.6f },
        { "C  THE RISEN", scene_c, 8, 2.2f }, { "C  ITS GRAVES", scene_c_graves, 12, 1.4f } };
    sw = (W * 2 + 16) * 2 + 16; sh = (H * 2 + 40) * 3 + 16;
    uint8_t *big = calloc((size_t)sw * sh, 3);
    for (int i = 0; i < 6; i++) {
        LOOP = V[i].loop; T = V[i].t;
        V[i].fn(T);
        sheet = big;
        int ox = 16 + (i % 2) * (W * 2 + 16), oy = 16 + (i / 2) * (H * 2 + 40);
        sheet_label(ox, oy, V[i].label);
        sheet_put(ox, oy + 24);
    }
    char p[512];
    snprintf(p, sizeof p, "%s/overview.png", dir);
    write_png(p, big, sw, sh);
    free(big);
}

int main(int argc, char **argv) {
    if (argc < 2) { fprintf(stderr, "usage: titlemock <dir> [a|b|c|logo]\n"); return 2; }
    crawl_art_init();
    crawl_art_level(3);
    ui_face_init();
    ui_ramps(0);
    memcpy(BASEPAL, crawl_pal, sizeof BASEPAL);
    F.w = W; F.h = H;
    const char *which = argc > 2 ? argv[2] : "all";
    if (!strcmp(which, "logo") || !strcmp(which, "all")) logo_sheet(argv[1]);
    if (!strcmp(which, "a") || !strcmp(which, "all")) run(argv[1], "A_beneath", scene_a, 12.0f);
    if (!strcmp(which, "a") || !strcmp(which, "all")) run(argv[1], "A_graves", a_graves, 12.0f);
    if (!strcmp(which, "b") || !strcmp(which, "all")) run(argv[1], "B_walk", scene_b, 8.0f);
    if (!strcmp(which, "b") || !strcmp(which, "all")) run(argv[1], "B_graves", scene_b_graves, 12.0f);
    if (!strcmp(which, "c") || !strcmp(which, "all")) run(argv[1], "C_risen", scene_c, 8.0f);
    if (!strcmp(which, "c") || !strcmp(which, "all")) run(argv[1], "C_graves", scene_c_graves, 12.0f);
    if (!strcmp(which, "overview") || !strcmp(which, "all")) overview(argv[1]);
    if (!strcmp(which, "flash")) {                                      /* the lightning's frames, to check them */
        char p[512];
        snprintf(p, sizeof p, "%s/flash_a.png", argv[1]); still(p, scene_a, 12, 12 * 0.55f + 0.04f);
        snprintf(p, sizeof p, "%s/flash_c.png", argv[1]); still(p, scene_c, 8, 8 * 0.62f + 0.24f);
    }
    return 0;
}
