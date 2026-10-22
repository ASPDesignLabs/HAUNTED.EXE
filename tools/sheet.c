/* Sprite sheets for approval: CATACOMB's sprites as the game shades them, in each rank they come
 * in, labelled in the VGA font and written as PNG. New creatures and decorations are shown on these
 * before they go into play (docs/PLAN-bestiary.md).
 *
 *   sheet list                          the groups
 *   sheet <group> out.png [level]       one group; level picks the palette and decay (default 1)
 *   sheet all <dir> [level]             every group, as <dir>/<group>.png
 */
#include "../src/crawl.h"
#include <stdio.h>
#include <stdlib.h>
#include <zlib.h>

#define SC 3                                /* sprite pixels per texel */
#define CELL (TS * SC)
#define PAD 8
#define TXT 2                               /* font scale */
#define LABEL_H (8 * TXT + 8)
#define ROWHEAD (13 * 8 * TXT)
#define LIGHT 26                            /* a little brighter than unlit, as near a torch */

typedef struct { int spr; const char *label; } Frame1;
typedef struct {
    const char *name, *title;
    int family;                             /* PF_*, or -1: drawn as it is (spirits, decorations) */
    unsigned ranks;                         /* bit per rank shown */
    Frame1 fr[16];
} Group;

#define ALL_RANKS 0x3Fu
#define NO_V (ALL_RANKS & ~(1u << RK_V))

static const Group GROUPS[] = {
    { "skeleton", "SKELETONS (TODAY)", PF_BONE, ALL_RANKS,
      { { SP_SKEL_A, "WALK A" }, { SP_SKEL_B, "WALK B" }, { SP_SKEL_T, "THROW" }, { SP_SKEL_D1, "DIE 1" }, { SP_SKEL_D3, "DIE 3" }, { -1, 0 } } },
    { "shield", "SHIELD-BEARERS (TODAY)", PF_BONE, ALL_RANKS,
      { { SP_SHIELD_A, "WALK" }, { SP_SHIELD_S, "SWING" }, { SP_SHIELD_XA, "NO SHIELD" }, { SP_SHIELD_XS, "SWING" }, { -1, 0 } } },
    { "wskull", "WALL SKULLS (TODAY)", PF_BONE, ALL_RANKS,
      { { SP_WSKULL0, "ASLEEP" }, { SP_WSKULL1, "WAKING" }, { SP_WSKULL2, "BLAZING" }, { SP_WSKULL_D, "BROKEN" }, { -1, 0 } } },
    { "spider", "WEB SPIDERS (TODAY)", PF_CHITIN, ALL_RANKS,
      { { SP_SPIDER_A, "A" }, { SP_SPIDER_B, "B" }, { SP_SPIDER_BITE, "BITE" }, { SP_SPIDER_D, "DEAD" }, { -1, 0 } } },
    { "bat", "BATS (TODAY)", PF_WING, NO_V,
      { { SP_BAT_A, "UP" }, { SP_BAT_B, "DOWN" }, { SP_BAT_D, "DEAD" }, { -1, 0 } } },
    { "spectre", "SPECTRES (TODAY'S WRAITHS)", -1, 1,
      { { SP_GHOST_A, "A" }, { SP_GHOST_B, "B" }, { SP_GHOST_D1, "SHRIEK" }, { SP_GHOST_D2, "SHRIEK" }, { -1, 0 } } },
    { "shambler", "SHAMBLER (ZOMBIE, FROM LEVEL 2)", PF_FLESH, ALL_RANKS,
      { { SP_SHAMB_A, "WALK A" }, { SP_SHAMB_B, "WALK B" }, { SP_SHAMB_GRAB, "GRAB" }, { SP_SHAMB_RISE1, "RISE 1" },
        { SP_SHAMB_RISE2, "RISE 2" }, { SP_SHAMB_D1, "DIE 1" }, { SP_SHAMB_D2, "DIE 2" }, { -1, 0 } } },
    { "drowned", "THE DROWNED (FROM LEVEL 7)", PF_STONE, ALL_RANKS,
      { { SP_DROWN_A, "WADE A" }, { SP_DROWN_B, "WADE B" }, { SP_DROWN_GRAB, "GRAB" }, { SP_DROWN_RISE, "RISE" },
        { SP_DROWN_BURST, "BURST" }, { SP_DROWN_POOL, "FOUL POOL" }, { -1, 0 } } },
    { "ghoul", "GHOUL (FROM LEVEL 12)", PF_FLESH, ALL_RANKS,
      { { SP_GHOUL_A, "LOPE A" }, { SP_GHOUL_B, "LOPE B" }, { SP_GHOUL_LEAP, "LEAP" }, { SP_GHOUL_FEED, "FEED" }, { SP_GHOUL_D, "DEAD" }, { -1, 0 } } },
    { "hound", "BONE HOUND (FROM LEVEL 8)", PF_BONE, ALL_RANKS,
      { { SP_HOUND_A, "RUN A" }, { SP_HOUND_B, "RUN B" }, { SP_HOUND_BITE, "BITE" }, { SP_HOUND_D, "DEAD" }, { -1, 0 } } },
    { "knight", "SKELETON KNIGHT (FROM LEVEL 13)", PF_ARMOUR, ALL_RANKS,
      { { SP_KNIGHT_A, "WALK" }, { SP_KNIGHT_W, "WIND-UP" }, { SP_KNIGHT_S, "STRIKE" }, { SP_KNIGHT2_A, "HALF PLATE" }, { SP_KNIGHT2_W, "WIND-UP" },
        { SP_KNIGHT3_A, "STRIPPED" }, { SP_KNIGHT3_S, "STRIKE" }, { SP_KNIGHT_D, "DEAD" }, { -1, 0 } } },
    { "deacon", "DEACON (FROM LEVEL 18)", PF_ROBE, ALL_RANKS,
      { { SP_DEACON_A, "WALK A" }, { SP_DEACON_B, "WALK B" }, { SP_DEACON_CHANT, "RAISING" }, { SP_DEACON_D, "DEAD" }, { -1, 0 } } },
    { "hunter", "HUNTING SPIDER (FROM LEVEL 11)", PF_HIDE, ALL_RANKS,
      { { SP_HUNTER_A, "RUN A" }, { SP_HUNTER_B, "RUN B" }, { SP_HUNTER_LEAP, "LEAP" }, { SP_HUNTER_D, "DEAD" }, { -1, 0 } } },
    { "brood", "BROOD MOTHER, SACS, SPIDERLINGS (FROM 14)", PF_CHITIN, ALL_RANKS,
      { { SP_BROOD_A, "WALK A" }, { SP_BROOD_B, "WALK B" }, { SP_BROOD_LAY, "LAYING" }, { SP_BROOD_D, "DEAD" },
        { SP_SAC_A, "SAC" }, { SP_SAC_B, "STIRRING" }, { SP_SAC_OPEN, "HATCHED" }, { SP_LING_A, "SPIDERLING" }, { -1, 0 } } },
    { "wraith", "THE WRAITH (FROM LEVEL 9; SHOWN 1.6X TALL)", -1, 1,
      { { SP_WRAITH_A, "GLIDE A" }, { SP_WRAITH_B, "GLIDE B" }, { SP_WRAITH_REACH, "REACH" }, { SP_WRAITH_BACK, "DRIVEN BACK" }, { -1, 0 } } },
    { "shade", "SHADE (FROM LEVEL 16; ONLY SEEN IN LIGHT)", -1, 1,
      { { SP_SHADE_A, "DRIFT A" }, { SP_SHADE_B, "DRIFT B" }, { SP_SHADE_TOUCH, "TOUCH" }, { SP_SHADE_D, "UNMADE" }, { -1, 0 } } },
    { "imp", "EMBER IMP (FROM LEVEL 17; SHOWN HALF SIZE)", PF_CLAY, ALL_RANKS,
      { { SP_IMP_A, "HOP A" }, { SP_IMP_B, "HOP B" }, { SP_IMP_THROW, "THROW" }, { SP_IMP_D, "CRUMBLED" }, { SP_EMBER_A, "EMBER" }, { -1, 0 } } },
    { "gargoyle", "GARGOYLE (FROM LEVEL 19)", PF_STONE, ALL_RANKS,
      { { SP_GARG_PERCH, "PERCHED" }, { SP_GARG_A, "FLY A" }, { SP_GARG_B, "FLY B" }, { SP_GARG_SWOOP, "SWOOP" }, { SP_GARG_D, "BROKEN" }, { -1, 0 } } },
};
#define NGROUPS ((int)(sizeof GROUPS / sizeof GROUPS[0]))

static const char *const RANK_NAME[NRANK] = { "I", "II GREEN", "III BLUE", "IV RED", "V BLACK", "GILDED" };

/* ---- an RGB canvas ---- */

static uint8_t *img;
static int iw, ih;

static void put(int x, int y, int r, int g, int b) {
    if ((unsigned)x >= (unsigned)iw || (unsigned)y >= (unsigned)ih) return;
    uint8_t *p = &img[((size_t)y * iw + x) * 3];
    p[0] = (uint8_t)r; p[1] = (uint8_t)g; p[2] = (uint8_t)b;
}
static void rect(int x, int y, int w, int h, int r, int g, int b) {
    for (int yy = y; yy < y + h; yy++) for (int xx = x; xx < x + w; xx++) put(xx, yy, r, g, b);
}
static void text(int x, int y, const char *s, int r, int g, int b) {
    for (; *s; s++, x += 8 * TXT)
        for (int row = 0; row < 8; row++)
            for (int col = 0; col < 8; col++)
                if (font_bios8[(uint8_t)*s][row] >> (7 - col) & 1) rect(x + col * TXT, y + row * TXT, TXT, TXT, r, g, b);
}

/* a sprite, shaded as the game shades it, through a rank's table */
static void sprite(int ox, int oy, int spr, const uint8_t *lut) {
    rect(ox, oy, CELL, CELL, 22, 21, 24);
    for (int y = 0; y < CELL; y++)
        for (int x = 0; x < CELL; x++) {
            uint8_t c = crawl_spr[spr][(x / SC) * TS + y / SC];
            if (c == C_CLEAR) continue;
            c = crawl_cmap[LIGHT][lut ? lut[c] : c];
            put(ox + x, oy + y, crawl_pal[c][0] * 255 / 63, crawl_pal[c][1] * 255 / 63, crawl_pal[c][2] * 255 / 63);
        }
}

/* ---- PNG ---- */

static void put32(FILE *f, uint32_t v) { uint8_t b[4] = { v >> 24, v >> 16, v >> 8, v }; fwrite(b, 1, 4, f); }
static void chunk(FILE *f, const char *type, const uint8_t *data, uint32_t len) {
    put32(f, len);
    fwrite(type, 1, 4, f);
    if (len) fwrite(data, 1, len, f);
    uint32_t crc = crc32(0, (const Bytef *)type, 4);
    if (len) crc = crc32(crc, data, len);
    put32(f, crc);
}
static int write_png(const char *path) {
    size_t rowlen = 1 + (size_t)iw * 3;
    uint8_t *raw = malloc(rowlen * ih);
    for (int y = 0; y < ih; y++) { raw[rowlen * y] = 0; memcpy(raw + rowlen * y + 1, img + (size_t)y * iw * 3, (size_t)iw * 3); }
    uLongf clen = compressBound((uLong)(rowlen * ih));
    uint8_t *comp = malloc(clen);
    compress2(comp, &clen, raw, (uLong)(rowlen * ih), 9);
    FILE *f = fopen(path, "wb");
    if (!f) { free(raw); free(comp); return 0; }
    static const uint8_t sig[8] = { 0x89, 'P', 'N', 'G', 13, 10, 26, 10 };
    fwrite(sig, 1, 8, f);
    uint8_t ihdr[13] = { iw >> 24, iw >> 16, iw >> 8, iw, ih >> 24, ih >> 16, ih >> 8, ih, 8, 2, 0, 0, 0 };
    chunk(f, "IHDR", ihdr, 13);
    chunk(f, "IDAT", comp, (uint32_t)clen);
    chunk(f, "IEND", NULL, 0);
    fclose(f);
    free(raw); free(comp);
    return 1;
}

/* ---- a group's sheet: frames across, ranks down ---- */

static int draw_group(const Group *g, const char *path, int level) {
    int nf = 0, nr = 0;
    while (nf < 16 && g->fr[nf].spr >= 0) nf++;
    for (int r = 0; r < NRANK; r++) nr += g->ranks >> r & 1;
    iw = PAD + ROWHEAD + nf * (CELL + PAD);
    ih = PAD + LABEL_H + 8 * TXT + PAD + nr * (CELL + PAD) + 8 * TXT + PAD;
    img = calloc((size_t)iw * ih, 3);
    rect(0, 0, iw, ih, 10, 10, 12);
    char head[96];
    snprintf(head, sizeof head, "%s - LEVEL %d", g->title, level);
    text(PAD, PAD, head, 230, 230, 230);
    int y = PAD + LABEL_H;
    for (int f = 0; f < nf; f++) text(PAD + ROWHEAD + f * (CELL + PAD), y, g->fr[f].label, 190, 190, 190);
    y += 8 * TXT + PAD;
    for (int r = 0; r < NRANK; r++) {
        if (!(g->ranks >> r & 1)) continue;
        if (g->family >= 0) text(PAD, y + CELL / 2 - 4 * TXT, RANK_NAME[r], 190, 190, 190);
        for (int f = 0; f < nf; f++) sprite(PAD + ROWHEAD + f * (CELL + PAD), y, g->fr[f].spr, g->family >= 0 ? crawl_rank_lut[g->family][r] : NULL);
        y += CELL + PAD;
    }
    text(PAD, y, "SHADED AS NEAR A TORCH. SIZES ARE NOT TO SCALE.", 120, 120, 120);
    int ok = write_png(path);
    free(img);
    return ok;
}

int main(int argc, char **argv) {
    if (argc >= 2 && !strcmp(argv[1], "list")) {
        for (int i = 0; i < NGROUPS; i++) printf("%-12s %s\n", GROUPS[i].name, GROUPS[i].title);
        return 0;
    }
    if (argc < 3) { fprintf(stderr, "usage: sheet list | sheet <group|all> <out.png|dir> [level]\n"); return 2; }
    int level = argc > 3 ? atoi(argv[3]) : 1;
    crawl_art_init();
    crawl_art_level(level < 1 ? 1 : level);
    int done = 0;
    for (int i = 0; i < NGROUPS; i++) {
        if (strcmp(argv[1], "all") && strcmp(argv[1], GROUPS[i].name)) continue;
        char path[512];
        if (!strcmp(argv[1], "all")) snprintf(path, sizeof path, "%s/%s.png", argv[2], GROUPS[i].name);
        else snprintf(path, sizeof path, "%s", argv[2]);
        if (!draw_group(&GROUPS[i], path, level)) { fprintf(stderr, "can't write %s\n", path); return 1; }
        done++;
    }
    if (!done) { fprintf(stderr, "no group '%s' (sheet list)\n", argv[1]); return 2; }
    return 0;
}
