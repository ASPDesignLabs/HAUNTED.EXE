/* Mock-ups of the title screen (docs/PLAN-title.md): src/crawl_title.c's scenes driven round loops with made-up pages,
 * written as animated GIFs (each frame with the palette it was drawn in, twice size) and sheets of stills.
 *
 *   titlemock <dir> [which]     which: a, b, c (each title, and its graves), menu (the three menus), logo, parts, overview,
 *                               flash (the lightning's frames); all but flash by default. TM_SCALE=n: the GIFs at n times
 *   titlemock <dir> s <A|Ag|Am|B|Bg|Bm|C|Cg|Cm> <seconds>      one still, doubled (TM_PAD=1: the pad's words)
 *
 *   make build/titlemock
 */
#include "../src/crawl.h"
#include "../src/crawl_title.h"
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
#define PI 3.14159265f

static Frame F;
static float T, LOOP;                                   /* the clock, and the loop the mock-up comes round in */
static int PADMODE;
static float smooth(float a, float b, float v) { float t = clampf((v - a) / (b - a), 0, 1); return t * t * (3 - 2 * t); }

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

/* ---- the mock-ups' timelines: what a person would be doing, round a loop ---- */
static TitleView V;
static void mock_pages(void) {                         /* made up: two runs signed, one page blank */
    title_page_set(&V.page[0], "I", 1, 1, "WITCHFINDER", "LEVEL 14", "CATACOMBS", "7 NOV 2026");
    title_page_set(&V.page[1], "II", 1, 2, "ANATOMIST", "LEVEL 1", "AT A DEATH", "9 NOV 2026");
    title_page_set(&V.page[2], "III", 0, 0, "UNMARKED", "NOTHING", "WRITTEN", "");
}
static void view_begin(int scene) { memset(&V, 0, sizeof V); mock_pages(); V.scene = scene; V.pad = PADMODE; V.version = "1.0.0B21.1"; }
/* the graves: each chosen in turn, coming up over a third of a second and going as fast */
static float sel_k(float t, int k) {
    float per = LOOP / 3, u = t - k * per;
    if (k == 0 && t > LOOP - 0.35f) return smooth(LOOP - 0.35f, LOOP, t);
    if (u < 0 || u > per + 0.35f) return 0;
    return smooth(0, 0.35f, u) * (1 - smooth(per, per + 0.35f, u));
}
/* the menu: the title a second, the key, the menu coming in, the choice going down the four, then Esc and the menu going,
 * so the loop comes back to the title */
#define T_PRESS 1.0f
#define T_IN 0.7f
static void menu_time(float t) {
    V.press = t >= T_PRESS && t < T_PRESS + 0.35f ? 1 - (t - T_PRESS) / 0.35f : 0;
    V.menu = smooth(T_PRESS, T_PRESS + T_IN, t) * (1 - smooth(LOOP - 0.6f, LOOP - 0.05f, t));
    float t0 = T_PRESS + T_IN, per = (LOOP - 0.6f - t0) / 4;
    for (int k = 0; k < 4; k++) {
        float u = t - t0 - k * per;
        float in = k == 0 ? 1 : smooth(0, 0.3f, u), out = k == 3 ? 1 : 1 - smooth(per, per + 0.3f, u);
        V.mk[k] = (k == 0 ? (t >= T_PRESS ? 1 : 0) : (u >= 0 ? 1 : 0)) * in * out;
    }
}
static void scene_a(float t) { view_begin(0); title_draw(&F, t, &V); }
static void scene_b(float t) { view_begin(1); title_draw(&F, t, &V); }
static void scene_c(float t) { view_begin(2); title_draw(&F, t, &V); }
static void graves_of(int sc, float t) { view_begin(sc); V.graves = 1; for (int g = 0; g < 3; g++) V.gk[g] = sel_k(t, g); title_draw(&F, t, &V); }
static void a_graves(float t) { graves_of(0, t); }
static void scene_b_graves(float t) { graves_of(1, t); }
static void scene_c_graves(float t) { graves_of(2, t); }
static void scene_a_menu(float t) { view_begin(0); menu_time(t); title_draw(&F, t, &V); }
static void scene_b_menu(float t) { view_begin(1); menu_time(t); title_draw(&F, t, &V); }
static void scene_c_menu(float t) { view_begin(2); menu_time(t); title_draw(&F, t, &V); }

/* ---- the sheets ---- */
static void logo_sheet(const char *dir) {
    char p[512];
    sw = W * 2 + 32; sh = (H * 2 + 40) * 2 + 16;
    sheet = calloc((size_t)sw * sh, 3);
    for (int k = 0; k < 2; k++) {
        title_debug_frame(&F, k, k ? 2.5f : 0.7f);
        sheet_label(16, 8 + k * (H * 2 + 40), k ? "CUT IN STONE" : "IN BLOOD");
        sheet_put(16, 32 + k * (H * 2 + 40));
    }
    snprintf(p, sizeof p, "%s/logo.png", dir);
    write_png(p, sheet, sw, sh);
    free(sheet);
}
static void parts_sheet(const char *path) {
    view_begin(0);
    title_draw(&F, 0, &V);                              /* the pages handed over */
    title_debug_frame(&F, 2, 0);
    sw = W * 3; sh = H * 3;
    sheet = calloc((size_t)sw * sh, 3);
    for (int y = 0; y < sh; y++) for (int x = 0; x < sw; x++) { const uint8_t *q = F.pal[F.pix[(y / 3) * W + x / 3]]; for (int c = 0; c < 3; c++) sheet[((size_t)y * sw + x) * 3 + c] = (uint8_t)(q[c] * 255 / 63); }
    write_png(path, sheet, sw, sh);
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
/* the overview: each scene's title, its menu and its graves, a still each, labelled; and a sheet of the pad's words */
static void overview(const char *dir) {
    static const struct { const char *label; void (*fn)(float); float loop, t; } VV[9] = {
        { "A  BENEATH", scene_a, 12, 4.0f }, { "A  ITS MENU", scene_a_menu, 12, 3.0f }, { "A  ITS GRAVES", a_graves, 12, 1.2f },
        { "B  THE WALK", scene_b, 8, 2.0f }, { "B  ITS MENU", scene_b_menu, 16, 5.4f }, { "B  ITS GRAVES", scene_b_graves, 16, 8.0f },
        { "C  THE RISEN", scene_c, 8, 2.2f }, { "C  ITS MENU", scene_c_menu, 16, 10.5f }, { "C  ITS GRAVES", scene_c_graves, 16, 1.4f } };
    char p[512];
    for (int pass = 0; pass < 2; pass++) {
        int n = pass ? 3 : 9;
        sw = (W * 2 + 16) * 3 + 16; sh = (H * 2 + 40) * (n / 3) + 16;
        uint8_t *big = calloc((size_t)sw * sh, 3);
        PADMODE = pass;
        for (int i = 0; i < n; i++) {
            int v = pass ? (i == 0 ? 6 : i == 1 ? 4 : 5) : i;
            LOOP = VV[v].loop; T = VV[v].t;
            VV[v].fn(T);
            sheet = big;
            int ox = 16 + (i % 3) * (W * 2 + 16), oy = 16 + (i / 3) * (H * 2 + 40);
            sheet_label(ox, oy, VV[v].label);
            sheet_put(ox, oy + 24);
        }
        PADMODE = 0;
        snprintf(p, sizeof p, pass ? "%s/pad.png" : "%s/overview.png", dir);
        write_png(p, big, sw, sh);
        free(big);
    }
}

int main(int argc, char **argv) {
    if (argc < 2) { fprintf(stderr, "usage: titlemock <dir> [a|b|c|menu|logo|parts|overview|flash] | titlemock <dir> s <name> <seconds>\n"); return 2; }
    crawl_art_init();
    crawl_art_level(3);
    ui_face_init();
    ui_ramps(0);
    F.w = W; F.h = H;
    const char *which = argc > 2 ? argv[2] : "all";
    int all = !strcmp(which, "all");
    if (!strcmp(which, "logo") || all) logo_sheet(argv[1]);
    if (!strcmp(which, "a") || all) { run(argv[1], "A_beneath", scene_a, 12.0f); run(argv[1], "A_graves", a_graves, 12.0f); }
    if (!strcmp(which, "b") || all) { run(argv[1], "B_walk", scene_b, 8.0f); run(argv[1], "B_graves", scene_b_graves, 16.0f); }
    if (!strcmp(which, "c") || all) { run(argv[1], "C_risen", scene_c, 8.0f); run(argv[1], "C_graves", scene_c_graves, 16.0f); }
    if (!strcmp(which, "menu") || all) {
        run(argv[1], "A_menu", scene_a_menu, 12.0f);
        run(argv[1], "B_menu", scene_b_menu, 16.0f);
        run(argv[1], "C_menu", scene_c_menu, 16.0f);
    }
    if (!strcmp(which, "overview") || all) overview(argv[1]);
    if (!strcmp(which, "parts") || all) { char p[512]; snprintf(p, sizeof p, "%s/parts.png", argv[1]); parts_sheet(p); }
    if (!strcmp(which, "s") && argc > 4) {                              /* one still */
        static const struct { const char *n; void (*fn)(float); float loop; } SS[9] = {
            { "A", scene_a, 12 }, { "Ag", a_graves, 12 }, { "Am", scene_a_menu, 12 }, { "B", scene_b, 8 }, { "Bg", scene_b_graves, 16 },
            { "Bm", scene_b_menu, 16 }, { "C", scene_c, 8 }, { "Cg", scene_c_graves, 16 }, { "Cm", scene_c_menu, 16 } };
        PADMODE = getenv("TM_PAD") != NULL;
        for (int i = 0; i < 9; i++) if (!strcmp(argv[3], SS[i].n)) {
            char p[512];
            snprintf(p, sizeof p, "%s/s_%s_%s.png", argv[1], argv[3], argv[4]);
            still(p, SS[i].fn, SS[i].loop, (float)atof(argv[4]));
        }
    }
    if (!strcmp(which, "bench")) {                                      /* how long a frame takes to draw, each title and its menu */
        static void (*const FN[6])(float) = { scene_a, scene_b, scene_c, scene_a_menu, scene_b_menu, scene_c_menu };
        static const char *const NM[6] = { "A", "B", "C", "A menu", "B menu", "C menu (its dissolve at 1.0-1.7 s)" };
        for (int i = 0; i < 6; i++) {
            LOOP = i < 3 ? (i ? 8.0f : 12.0f) : 16.0f;
            clock_t c0 = clock();
            for (int k = 0; k < 160; k++) FN[i](k * LOOP / 160.0f);
            printf("%s: %.2f ms a frame\n", NM[i], (double)(clock() - c0) * 1000.0 / CLOCKS_PER_SEC / 160.0);
        }
    }
    if (!strcmp(which, "flash")) {                                      /* the lightning's frames, to check them */
        char p[512];
        snprintf(p, sizeof p, "%s/flash_a.png", argv[1]); still(p, scene_a, 12, 12 * 0.55f + 0.04f);
        snprintf(p, sizeof p, "%s/flash_c.png", argv[1]); still(p, scene_c, 8, 8 * 0.62f + 0.24f);
    }
    return 0;
}
