/* The side passages' draft art as a sheet (docs/PLAN-side.md, step 1): every tile, every frame of the caster's figure, and a
 * screen as it might look, at twice its pixels, in CATACOMB's palette and light.
 *
 *   make build/sidemock && build/sidemock out.png
 */
#include "crawl.h"
#include "side_art.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>

static int iw, ih;
static uint8_t *img;
static void put(int x, int y, int r, int g, int b) { if (x < 0 || y < 0 || x >= iw || y >= ih) return; uint8_t *p = img + ((size_t)y * iw + x) * 3; p[0] = r; p[1] = g; p[2] = b; }
static void rect(int x, int y, int w, int h, int r, int g, int b) { for (int yy = y; yy < y + h; yy++) for (int xx = x; xx < x + w; xx++) put(xx, yy, r, g, b); }
static void text(int x, int y, const char *s, int sc, int r, int g, int b) {
    for (; *s; s++, x += 8 * sc)
        for (int row = 0; row < 8; row++)
            for (int col = 0; col < 8; col++)
                if (font_bios8[(uint8_t)*s][row] >> (7 - col) & 1) rect(x + col * sc, y + row * sc, sc, sc, r, g, b);
}
static void pal_px(int x, int y, int sc, uint8_t c, int light) {
    if (c == C_CLEAR) return;
    c = crawl_cmap[light][c];
    rect(x, y, sc, sc, crawl_pal[c][0] * 255 / 63, crawl_pal[c][1] * 255 / 63, crawl_pal[c][2] * 255 / 63);
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
    fclose(f); free(raw); free(comp);
    return 1;
}

static const char *const TILE_NAME[NST] = {
    "STONE", "STONE", "STONE", "STONE TOP", "BRICK", "BRICK TOP", "LEDGE", "SPIKES",
    "WATER 1", "WATER 2", "WATER 3", "WATER 4", "WATER DEEP", "FALLING", "FALLING!",
    "BACK", "BACK", "NICHE", "SKULLS", "FAR WALL", "PLATFORM", "PLATFORM", "PLATFORM",
    "DOOR TOP", "DOOR FOOT", "GOAL TOP", "GOAL FOOT", "DOOR LIGHT", "LEVER", "LEVER ON",
    "GOLD KEY", "SILVER KEY", "CRIMSON KEY", "A LIFE", "TORCH 1", "TORCH 2", "TORCH 3", "CHAIN",
    "BREATH", "BREATH!", "COLD", "GRATE", "SLUICE", "SLUICE!", "SAINT", "SAINT!",
};
_Static_assert(sizeof TILE_NAME / sizeof TILE_NAME[0] == NST, "a name for each tile");

/* ---- the mock-up: a screen of a side passage, 320 by 156 over the status bar ---- */
static uint8_t scr[200][320];
static int lit[200][320];
static void sblit(const uint8_t *t, int w, int h, int ox, int oy, int flip) {
    for (int y = 0; y < h; y++)
        for (int x = 0; x < w; x++) {
            uint8_t c = t[y * w + (flip ? w - 1 - x : x)];
            int sx = ox + x, sy = oy + y;
            if (c == C_CLEAR || sx < 0 || sy < 0 || sx >= 320 || sy >= 156) continue;
            scr[sy][sx] = c;
            lit[sy][sx] = -1;                                      /* lit by the scene's light, as the foreground */
        }
}
static void stile(int t, int ox, int oy, int light) {
    for (int y = 0; y < STS; y++)
        for (int x = 0; x < STS; x++) {
            uint8_t c = side_tile[t][y * STS + x];
            int sx = ox + x, sy = oy + y;
            if (c == C_CLEAR || sx < 0 || sy < 0 || sx >= 320 || sy >= 156) continue;
            scr[sy][sx] = c; lit[sy][sx] = light;
        }
}
static void ssprite(int spr, int ox, int oy, int flip) {           /* a crawl sprite at half its size: what stands in for a creature's own */
    for (int y = 0; y < TS / 2; y++)
        for (int x = 0; x < TS / 2; x++) {
            int n = 0; uint8_t c = C_CLEAR;
            for (int k = 0; k < 4; k++) {
                int px = x * 2 + (k & 1), py = y * 2 + (k >> 1);
                uint8_t v = crawl_spr[spr][(flip ? TS - 1 - px : px) * TS + py];
                if (v != C_CLEAR) { n++; c = v; }
            }
            int sx = ox + x, sy = oy + y;
            if (n >= 2 && sx >= 0 && sy >= 0 && sx < 320 && sy < 156) { scr[sy][sx] = c; lit[sy][sx] = -1; }
        }
}
static const char *const MOCK[10] = {
    "#######################",
    "#,,,,,,....,,,,,,,,,,,#",
    "#,n,,,,.....,,,,s,,,,,#",
    "#,,,,,,......,,,,,,,,G#",
    "#,,,,=====...,,,,BBB,g#",
    "#,D,,,.........,,bbbBB#",
    "TTdT,,.....FF..TTTbbbb#",
    "####^^^TTTwwwwTT#######",
    "#######WWWWWWWW########",
    "#######################",
};
static int mock_tile(char ch, int x, int y, int *light) {
    *light = 24;
    switch (ch) {
    case '#': return ST_STONE0 + (int)((unsigned)(x * 7 + y * 3) % 3);
    case 'T': return ST_STONE_TOP;
    case 'b': return ST_BRICK;
    case 'B': return ST_BRICK_TOP;
    case ',': *light = 14; return (x + y) % 3 ? ST_BACK0 : ST_BACK1;
    case 'n': *light = 14; return ST_BACK_NICHE;
    case 's': *light = 14; return ST_BACK_SKULLS;
    case '=': return ST_LEDGE;
    case '^': return ST_SPIKES;
    case 'w': return ST_WATER0 + x % 4;
    case 'W': return ST_WATER_DEEP;
    case 'F': return ST_FALL + (x & 1);
    case 'D': return ST_DOOR_T;
    case 'd': return ST_DOOR_B;
    case 'G': return ST_GOAL_T;
    case 'g': return ST_GOAL_B;
    default: return -1;
    }
}
static void mock_scene(void) {
    const int scroll = 8;                                          /* the camera a half tile into the map */
    for (int y = 0; y < 156; y++) for (int x = 0; x < 320; x++) {  /* the far wall, at half the camera's pace, in the dark */
        int fx = (x + scroll / 2) % STS, fy = y % STS;
        scr[y][x] = side_tile[ST_FAR][fy * STS + fx]; lit[y][x] = 7;
    }
    for (int ty = 0; ty < 10; ty++)
        for (int tx = 0; tx < 23; tx++) {
            char ch = MOCK[ty][tx];
            int light, t = mock_tile(ch, tx, ty, &light);
            if (ch == 'D' || ch == 'd' || ch == 'G' || ch == 'g') {    /* a doorway stands on the back wall */
                stile(ST_BACK0, tx * STS - scroll, ty * STS, 14);
            }
            if (t >= 0) stile(t, tx * STS - scroll, ty * STS, light);
        }
    stile(ST_TORCH1, 4 * STS - scroll, 3 * STS, 24);                  /* the dressing */
    stile(ST_TORCH0, 15 * STS - scroll, 3 * STS, 24);
    for (int y = STS; y < 3 * STS + 8; y += STS) stile(ST_CHAIN, 9 * STS + 2 - scroll, y - 8, 22), stile(ST_CHAIN, 11 * STS + 10 - scroll, y - 8, 22);
    int py = 3 * STS + 8;                                          /* a platform on its chains */
    stile(ST_PLAT_L, 8 * STS + 8 - scroll, py, 24); stile(ST_PLAT_M, 9 * STS + 8 - scroll, py, 24); stile(ST_PLAT_R, 10 * STS + 8 - scroll, py, 24);
    stile(ST_KEY1, 9 * STS + 8 - scroll, py - STS, 24);
    stile(ST_LIFE, 18 * STS - scroll, 3 * STS, 24);
    stile(ST_SWITCH0, 5 * STS - scroll, 5 * STS, 24);
    sblit(side_fig[SF_RUN0 + 2], SF_W, SF_H, 2 * STS + 6 - scroll, 4 * STS, 0);   /* the caster, running */
    sblit(side_fig[SF_CAST1], SF_W, SF_H, 12 * STS - scroll, 2 * STS + 10, 0);   /* and again, casting from the ledge's end */
    ssprite(SP_SKEL_SA, 17 * STS - scroll, 2 * STS + 8, 1);         /* a skeleton coming, from the bestiary's side view */
    ssprite(SP_BAT_SA, 14 * STS - scroll, STS, 1);
    for (int i = 0; i < 6; i++) {                                  /* a firebolt in flight, and its trail */
        int bx = 12 * STS + 26 + i * 4 - scroll, by = 3 * STS + 2;
        for (int dy = -1; dy <= 1; dy++) for (int dx = -1; dx <= 1; dx++)
            if (dx * dx + dy * dy <= (i == 5 ? 2 : 1)) { scr[by + dy][bx + dx] = (uint8_t)(R_FLAME + 6 + i * 2); lit[by + dy][bx + dx] = 24; }
    }
    /* the light: the torches' and the doorways', on everything but the far wall */
    const float LX[] = { 4 * STS + 8, 15 * STS + 8, 2 * STS + 8, 21 * STS + 8 }, LY[] = { 3 * STS + 4, 3 * STS + 4, 5 * STS + 8, 4 * STS + 8 };
    for (int y = 0; y < 156; y++)
        for (int x = 0; x < 320; x++) {
            float g = 0.0f;
            for (int k = 0; k < 4; k++) { float d = hypotf((float)(x + scroll) - LX[k], (float)y - LY[k]); g += fmaxf(0.0f, 1.0f - d / (k < 2 ? 90.0f : 50.0f)); }
            int base = lit[y][x] < 0 ? 18 : lit[y][x];
            if (lit[y][x] != 7) lit[y][x] = (int)fminf(NLIGHT - 1, (float)base - 6.0f + g * 10.0f);
            else lit[y][x] = (int)fminf(12, 5 + g * 3.0f);
        }
}

int main(int argc, char **argv) {
    const char *out = argc > 1 ? argv[1] : "build/side_draft.png";
    crawl_art_init();
    crawl_art_level(1);
    crawl_art_run(1);
    side_art_init(); side_art_traps();

    const int TSC = 4, FSC = 5, PAD = 16;
    const int tcol = 13, tcell = STS * TSC + 18;
    const int fcol = 9, fcell = SF_W * FSC + 14;
    int trows = (NST + tcol - 1) / tcol, frows = (NSF + fcol - 1) / fcol;
    iw = PAD * 2 + fcol * fcell;
    int y_t = 60, y_f = y_t + trows * (tcell + 14) + 40, y_m = y_f + frows * (SF_H * FSC + 30) + 40;
    ih = y_m + 400 + 60;
    img = calloc((size_t)iw * ih, 3);
    rect(0, 0, iw, ih, 22, 20, 26);
    text(PAD, 16, "SIDE PASSAGES: THE DRAFT ART (DOCS/PLAN-SIDE.MD, STEP 1)", 2, 230, 200, 120);
    text(PAD, 38, "16 X 16 TILES IN RUN I'S STONE AND THE CRYPT'S PALETTE; THE CASTER FROM A RIG, EACH PIXEL KEPT BY ITS PART FOR THE GEAR", 1, 170, 160, 150);

    text(PAD, y_t - 4, "THE TILES, AT FOUR TIMES", 1, 230, 200, 120);
    for (int i = 0; i < NST; i++) {
        int cx = PAD + (i % tcol) * tcell, cy = y_t + 10 + (i / tcol) * (tcell + 14);
        rect(cx - 1, cy - 1, STS * TSC + 2, STS * TSC + 2, 40, 38, 46);
        for (int y = 0; y < STS; y++) for (int x = 0; x < STS; x++) pal_px(cx + x * TSC, cy + y * TSC, TSC, side_tile[i][y * STS + x], 24);
        text(cx, cy + STS * TSC + 4, TILE_NAME[i], 1, 190, 180, 170);
    }

    text(PAD, y_f - 4, "THE CASTER: IDLE, WALK, RUN, JUMP, CROUCH AND CREEP, CAST, HURT, FALLING OVER DEAD (AT FIVE TIMES)", 1, 230, 200, 120);
    for (int i = 0; i < NSF; i++) {
        int cx = PAD + (i % fcol) * fcell, cy = y_f + 10 + (i / fcol) * (SF_H * FSC + 30);
        rect(cx - 1, cy - 1, SF_W * FSC + 2, SF_H * FSC + 2, 46, 42, 50);
        for (int y = 0; y < SF_H; y++) for (int x = 0; x < SF_W; x++) pal_px(cx + x * FSC, cy + y * FSC, FSC, side_fig[i][y * SF_W + x], 24);
        char lab[24];
        int n = i >= SF_WALK0 && i <= SF_WALK5 ? i - SF_WALK0 + 1 : i >= SF_RUN0 && i <= SF_RUN5 ? i - SF_RUN0 + 1 : i >= SF_DIE0 ? i - SF_DIE0 + 1 : i <= SF_IDLE1 ? i + 1 : i >= SF_CREEP0 && i <= SF_CREEP1 ? i - SF_CREEP0 + 1 : i >= SF_CAST0 && i <= SF_CAST1 ? i - SF_CAST0 + 1 : 0;
        if (n) snprintf(lab, sizeof lab, "%s %d", SIDE_FRAME_NAME[i], n); else snprintf(lab, sizeof lab, "%s", SIDE_FRAME_NAME[i]);
        text(cx, cy + SF_H * FSC + 6, lab, 1, 190, 180, 170);
    }

    text(PAD, y_m - 4, "A SCREEN AS IT MIGHT BE, AT TWICE ITS PIXELS: THE FAR WALL SLOWER, THE BACK WALL DARKER, TORCHLIGHT; THE DEAD IN THEIR BESTIARY VIEWS FOR NOW", 1, 230, 200, 120);
    mock_scene();
    int mx = PAD, my = y_m + 10;
    for (int y = 0; y < 156; y++) for (int x = 0; x < 320; x++) pal_px(mx + x * 2, my + y * 2, 2, scr[y][x], lit[y][x]);
    rect(mx, my + 312, 640, 88, 30, 28, 34);
    text(mx + 150, my + 350, "THE CRYPT'S STATUS BAR, AS IT IS", 1, 150, 140, 130);
    if (!write_png(out)) { fprintf(stderr, "can't write %s\n", out); return 1; }
    printf("%s %dx%d\n", out, iw, ih);
    return 0;
}
