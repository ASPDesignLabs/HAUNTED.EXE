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
#include <string.h>
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
    { "dress1", "DRESSING, SET I: THE CRYPTS", -1, 1,
      { { SP_CANDLESTAND_A, "CANDLE STAND" }, { SP_BROKEN_ARMS, "BROKEN ARMS" }, { SP_MOUND, "TURNED EARTH" }, { SP_PIT, "OPEN GRAVE" }, { -1, 0 } } },
    { "dress2", "DRESSING, SET II: THE CATACOMBS", -1, 1,
      { { SP_PUDDLE_A, "PUDDLE" }, { SP_PUDDLE_B, "STIRRING" }, { SP_STALAG, "FLOWSTONE" }, { SP_LANTERN_A, "LANTERN" }, { SP_BRAZIER_A, "BRAZIER" },
        { SP_COLLAR, "COLLAR" }, { SP_ICICLES, "ICICLES" }, { SP_BELL, "BELL" }, { -1, 0 } } },
    { "dress3", "DRESSING, SET III: THE HOLLOWS", -1, 1,
      { { SP_CARCASS, "CARCASS" }, { SP_ARMOURSTAND, "ARMOUR STAND" }, { SP_HELM, "FALLEN HELM" }, { SP_TIMBER, "MINE PROP" }, { SP_MINELAMP_A, "MINER'S LAMP" },
        { SP_CART, "ORE CART" }, { SP_FUNGUS, "FUNGUS" }, { SP_SAC_A, "EGG SAC" }, { -1, 0 } } },
    { "dress4", "DRESSING, SET IV: THE DEEP", -1, 1,
      { { SP_CRYSTAL_A, "CRYSTALS" }, { SP_CRYSTAL_B, "PULSE" }, { SP_MAGMA_A, "MAGMA VENT" }, { SP_MAGMA_B, "FLARE" }, { SP_OBSIDIAN, "OBSIDIAN" },
        { SP_ALTAR_A, "ALTAR" }, { SP_GARG_PERCH, "STATUE" }, { -1, 0 } } },
    { "sides", "SIDE VIEWS, WALKING (LEFT IS RIGHT, MIRRORED)", -1, 1,
      { { SP_SKEL_SA, "SKELETON A" }, { SP_SKEL_SB, "B" }, { SP_SHIELD_SA, "SHIELD A" }, { SP_SHIELD_SB, "B" }, { SP_BAT_SA, "BAT UP" }, { SP_BAT_SB, "DOWN" },
        { SP_SPIDER_SA, "SPIDER A" }, { SP_SPIDER_SB, "B" }, { SP_GHOST_SA, "SPECTRE A" }, { SP_GHOST_SB, "B" }, { SP_WSKULL_S, "WALL SKULL" }, { -1, 0 } } },
    { "sides2", "SIDE VIEWS: SETS I AND II", -1, 1,
      { { SP_SHAMB_SA, "SHAMBLER A" }, { SP_SHAMB_SB, "B" }, { SP_DROWN_SA, "DROWNED A" }, { SP_DROWN_SB, "B" }, { SP_HOUND_SA, "HOUND A" }, { SP_HOUND_SB, "B" },
        { SP_WRAITH_SA, "WRAITH A" }, { SP_WRAITH_SB, "B" }, { -1, 0 } } },
    { "sides3", "SIDE VIEWS: SET III", -1, 1,
      { { SP_HUNTER_SA, "HUNTER A" }, { SP_HUNTER_SB, "B" }, { SP_GHOUL_SA, "GHOUL A" }, { SP_GHOUL_SB, "B" }, { SP_KNIGHT_SA, "KNIGHT A" }, { SP_KNIGHT_SB, "B" },
        { SP_BROOD_SA, "BROOD A" }, { SP_BROOD_SB, "B" }, { -1, 0 } } },
    { "sides4", "SIDE VIEWS: SET IV", -1, 1,
      { { SP_SHADE_SA, "SHADE A" }, { SP_SHADE_SB, "B" }, { SP_IMP_SA, "IMP A" }, { SP_IMP_SB, "B" }, { SP_DEACON_SA, "DEACON A" }, { SP_DEACON_SB, "B" },
        { SP_GARG_SA, "GARGOYLE A" }, { SP_GARG_SB, "B" }, { -1, 0 } } },
    { "b6", "1.0.0B6: FOR APPROVAL", -1, 1,
      { { SP_DOUBLE, "BLINK'S DOUBLE" }, { -1, 0 } } },
    { "b7", "1.0.0B7: FOR APPROVAL", -1, 1,
      { { SP_HEXTOTEM, "TOTEM" }, { SP_SCRIP, "SCRIP" }, { SP_BSPIDER_A, "SPIDER" }, { SP_KNIGHT_P0, "KNIGHT" },
        { SP_BRAZIER_OUT, "PUT OUT" }, { SP_TOMB_FRESH, "NEWEST" }, { SP_TOMB_ROUND, "OLDER" }, { SP_LECTERN, "COMPENDIUM" }, { -1, 0 } } },
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

/* ---- the line-up: everything at the size it is drawn in play, on one floor ---- */

typedef struct { int spr, family, rank; float scale, z; const char *label; } Stand;
static const Stand LINEUP[] = {                 /* today's sizes from scene_crawl.c; the new ones are proposals */
    { SP_SKEL_A, -1, 0, 1.0f, 0.0f, "SKELETON" }, { SP_SHIELD_A, PF_BONE, RK_IV, 1.0f, 0.0f, "SHIELD IV" },
    { SP_GHOST_A, -1, 0, 1.35f, 0.1f, "SPECTRE" }, { SP_BAT_A, -1, 0, 1.0f, 0.2f, "BAT" },
    { SP_WSKULL1, -1, 0, 0.62f, 0.3f, "WALL SKULL" }, { SP_SPIDER_A, -1, 0, 0.42f, 0.62f, "WEB SPIDER" },
    { SP_SHAMB_A, -1, 0, 1.0f, 0.0f, "SHAMBLER" }, { SP_DROWN_A, -1, 0, 1.1f, 0.0f, "DROWNED" },
    { SP_HOUND_A, PF_BONE, RK_III, 0.75f, 0.0f, "HOUND III" }, { SP_WRAITH_A, -1, 0, 1.6f, 0.0f, "THE WRAITH" },
    { SP_HUNTER_A, -1, 0, 0.6f, 0.0f, "HUNTER" }, { SP_GHOUL_A, PF_FLESH, RK_II, 0.85f, 0.0f, "GHOUL II" },
    { SP_KNIGHT_A, PF_ARMOUR, RK_IV, 1.2f, 0.0f, "KNIGHT IV" }, { SP_BROOD_A, -1, 0, 1.0f, 0.0f, "BROOD" },
    { SP_LING_A, -1, 0, 0.22f, 0.0f, "LINGS" }, { SP_SHADE_A, -1, 0, 1.1f, 0.0f, "SHADE" },
    { SP_IMP_A, -1, 0, 0.55f, 0.0f, "IMP" }, { SP_DEACON_A, PF_ROBE, RK_V, 1.05f, 0.0f, "DEACON V" },
    { SP_GARG_A, PF_STONE, RK_GILDED, 0.9f, 0.4f, "GARGOYLE G" },
};
#define NSTAND ((int)(sizeof LINEUP / sizeof LINEUP[0]))
#define UNIT 150                                /* pixels to a cell */

static int draw_lineup(const char *path, int level) {
    int step = 112, top = PAD + LABEL_H + 16;
    iw = PAD * 2 + (NSTAND / 2 + 1) * step + UNIT;
    int floor1 = top + (int)(UNIT * 1.75f), floor2 = floor1 + LABEL_H * 2 + (int)(UNIT * 1.75f);
    ih = floor2 + LABEL_H * 3 + PAD;
    img = calloc((size_t)iw * ih, 3);
    rect(0, 0, iw, ih, 10, 10, 12);
    char head[96];
    snprintf(head, sizeof head, "THE LINE-UP, AT THE SIZE THEY ARE DRAWN - LEVEL %d", level);
    text(PAD, PAD, head, 230, 230, 230);
    for (int row = 0; row < 2; row++) {
        int fl = row ? floor2 : floor1;
        rect(PAD, fl, iw - PAD * 2, 2, 60, 58, 64);
        for (int x = PAD; x < iw - PAD; x += 6) { put(x, fl - UNIT / 2, 70, 50, 50); put(x, fl - UNIT, 45, 45, 50); }   /* the eye, and a cell up */
    }
    text(PAD, floor1 - UNIT / 2 - 8 * TXT - 2, "EYE", 110, 70, 70);
    text(PAD, floor1 - UNIT - 8 * TXT - 2, "1 CELL", 80, 80, 90);
    for (int i = 0; i < NSTAND; i++) {
        const Stand *st = &LINEUP[i];
        int row = i >= (NSTAND + 1) / 2, col = row ? i - (NSTAND + 1) / 2 : i;
        int fl = row ? floor2 : floor1, size = (int)(UNIT * st->scale);
        int cx = PAD + 70 + col * step + step / 2, x0 = cx - size / 2, y0 = fl - (int)(st->z * UNIT) - size;
        const uint8_t *lut = st->family >= 0 ? crawl_rank_lut[st->family][st->rank] : NULL;
        for (int y = 0; y < size; y++)
            for (int x = 0; x < size; x++) {
                uint8_t c = crawl_spr[st->spr][(x * TS / size) * TS + y * TS / size];
                if (c == C_CLEAR) continue;
                c = crawl_cmap[LIGHT][lut ? lut[c] : c];
                put(x0 + x, y0 + y, crawl_pal[c][0] * 255 / 63, crawl_pal[c][1] * 255 / 63, crawl_pal[c][2] * 255 / 63);
            }
        text(cx - (int)strlen(st->label) * 4 * TXT, fl + 6 + (col & 1) * (8 * TXT + 4), st->label, 190, 190, 190);   /* staggered, so they don't collide */
    }
    text(PAD, ih - PAD - 8 * TXT, "SCALES FOR THE NEW ONES ARE PROPOSALS. SOME SHOWN IN A RANK.", 120, 120, 120);
    int ok = write_png(path);
    free(img);
    return ok;
}

/* ---- a group's sheet: frames across, ranks down ---- */

static int draw_group(const Group *g, const char *path, int level) {
    int nf = 0, nr = 0;
    while (nf < 16 && g->fr[nf].spr >= 0) nf++;
    for (int r = 0; r < NRANK; r++) nr += g->ranks >> r & 1;
    char head[96];
    snprintf(head, sizeof head, "%s - LEVEL %d", g->title, level);
    iw = PAD + ROWHEAD + nf * (CELL + PAD);
    int tw = PAD * 2 + (int)strlen(head) * 8 * TXT, fw = PAD * 2 + 47 * 8 * TXT;   /* room for the title and the note under it */
    if (iw < tw) iw = tw;
    if (iw < fw) iw = fw;
    ih = PAD + LABEL_H + 8 * TXT + PAD + nr * (CELL + PAD) + 8 * TXT + PAD;
    img = calloc((size_t)iw * ih, 3);
    rect(0, 0, iw, ih, 10, 10, 12);
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

/* ---- the Rune Shrine (1.0.0b6), its states, and its runes in each quality's colour ---- */

static const int QRAMP[5] = { R_BONE, R_MOSS, R_GLOW, R_PURPLE, R_GOLD };   /* as QUAL_RAMP: white, green, blue, purple, gold */
static const char *const QNAME[5] = { "COMMON", "UNCOMMON", "RARE", "ULTRA-RARE", "EXQUISITE" };
static uint8_t ramp_to(int ramp, int i) {                         /* step i of 16 in another ramp (gold has 13) */
    return ramp == R_GOLD ? (uint8_t)(R_GOLD + i * 12 / 15) : (uint8_t)(ramp + i);
}
/* a sprite at size `size`: shaded as near a torch, or at full brightness with R_PURPLE turned into `remap` */
static void blit(int ox, int oy, int size, int spr, int bright, int remap) {
    for (int y = 0; y < size; y++)
        for (int x = 0; x < size; x++) {
            uint8_t c = crawl_spr[spr][(x * TS / size) * TS + y * TS / size];
            if (c == C_CLEAR) continue;
            if (remap && c >= R_PURPLE && c < R_PURPLE + 16) c = ramp_to(remap, c - R_PURPLE);
            if (!bright) c = crawl_cmap[LIGHT][c];
            put(ox + x, oy + y, crawl_pal[c][0] * 255 / 63, crawl_pal[c][1] * 255 / 63, crawl_pal[c][2] * 255 / 63);
        }
}
static int draw_shrine(const char *path, int level) {
    int cw = CELL + PAD, top = PAD + LABEL_H + 8;
    iw = PAD * 2 + ROWHEAD + 10 * (CELL / 2 + PAD + 18) + 40;
    ih = top + 4 * (CELL + LABEL_H + PAD) + 8 * TXT + PAD;
    img = calloc((size_t)iw * ih, 3);
    rect(0, 0, iw, ih, 10, 10, 12);
    char head[96];
    snprintf(head, sizeof head, "1.0.0B6: THE RUNE SHRINE, FOR APPROVAL - LEVEL %d", level);
    text(PAD, PAD, head, 230, 230, 230);
    int y = top, x0 = PAD + ROWHEAD;
    /* the shrine: sanctified, cursed and unlit, and cursed and burning at each rank */
    text(PAD, y + CELL / 2, "THE SHRINE", 200, 200, 200);
    rect(x0, y, CELL, CELL, 22, 21, 24); blit(x0, y, CELL, SP_SHRINE_S, 0, 0);
    text(x0, y + CELL + 4, "SANCTIFIED", 190, 190, 190);
    for (int q = 0; q < 5; q++) {
        int x = x0 + (q + 1) * cw;
        rect(x, y, CELL, CELL, 22, 21, 24);
        blit(x, y, CELL, SP_SHRINE_C, 0, 0); blit(x, y, CELL, q & 1 ? SP_SHRINE_G1 : SP_SHRINE_G0, 1, QRAMP[q]);
        text(x, y + CELL + 4, QNAME[q], 190, 190, 190);
    }
    y += CELL + LABEL_H + PAD;
    /* the shrine in play: runes circling it, small, at full brightness (two attuned, at Rare) */
    text(PAD, y + CELL / 2, "CIRCLING", 200, 200, 200);
    for (int v = 0; v < 2; v++) {
        int x = x0 + v * 2 * cw, big = CELL * 2;
        rect(x, y, big + PAD, CELL, 22, 21, 24);
        int sx = x + big / 2 - CELL / 2 + PAD / 2;
        blit(sx, y, CELL, SP_SHRINE_C, 0, 0); blit(sx, y, CELL, SP_SHRINE_G0, 1, v ? QRAMP[2] : QRAMP[0]);
        int n = v ? 2 : 1;
        for (int i = 0; i < n; i++) {
            float a = 0.8f + i * 3.14159f;
            int rs = CELL / 4, rx = sx + CELL / 2 + (int)(cosf(a) * CELL * 0.62f) - rs / 2, ry = y + CELL / 3 + (int)(sinf(a) * CELL * 0.12f) - rs / 2;
            blit(rx, ry, rs, v ? (i ? SP_SRUNE3 : SP_SRUNE0) : SP_SRUNE_X, 1, v ? QRAMP[2] : QRAMP[0]);
        }
        text(x, y + CELL + 4, v ? "ATTUNED: WARD, MEND" : "JUST CURSED", 190, 190, 190);
    }
    y += CELL + LABEL_H + PAD;
    /* every rune's mark */
    text(PAD, y + CELL / 4, "ITS RUNES", 200, 200, 200);
    static const char *const RN[10] = { "WARD", "BLINK", "MIRROR", "MEND", "PURIFY", "NOVA", "DREAD", "SLIP", "REAP", "UNATTUNED" };
    int rs = CELL / 2;
    for (int k = 0; k < 10; k++) {
        int x = x0 + k * (rs + PAD + 18);
        rect(x, y, rs, rs, 22, 21, 24); blit(x, y, rs, k < 9 ? SP_SRUNE0 + k : SP_SRUNE_X, 1, QRAMP[k % 5]);
        text(x, y + rs + 4 + (k & 1) * (8 * TXT + 2), RN[k], 170, 170, 170);
    }
    y += CELL + LABEL_H / 2;
    /* the coin */
    text(PAD, y + CELL / 2, "CURSED COIN", 200, 200, 200);
    rect(x0, y, CELL, CELL, 22, 21, 24); blit(x0, y, CELL, SP_CCOIN, 0, 0);
    text(PAD, ih - PAD - 8 * TXT, "SHADED AS NEAR A TORCH; THE GLOW AND RUNES AT FULL BRIGHTNESS.", 120, 120, 120);
    int ok = write_png(path);
    free(img);
    return ok;
}

/* ---- the Backrooms (1.0.0b6): the pocket's textures in its palette, and a glitch proposed ---- */

static void tex(int ox, int oy, int t, int scale, int light) {    /* a wall, floor or ceiling texture, shaded */
    for (int y = 0; y < TS * scale; y++)
        for (int x = 0; x < TS * scale; x++) {
            uint8_t c = crawl_cmap[light][crawl_tex[t][(x / scale) * TS + y / scale]];
            put(ox + x, oy + y, crawl_pal[c][0] * 255 / 63, crawl_pal[c][1] * 255 / 63, crawl_pal[c][2] * 255 / 63);
        }
}
static uint32_t gl_hash(uint32_t v) { v ^= v >> 16; v *= 0x7feb352du; v ^= v >> 15; v *= 0x846ca68bu; return v ^ (v >> 16); }
/* a creature as the Backrooms would show it: bands of rows torn sideways, and its colours slipping,
 * a red copy one way and a blue the other; `seed` stands for the moment */
static void glitched(int ox, int oy, int spr, uint32_t seed) {
    rect(ox, oy, CELL, CELL, 22, 21, 24);
    int shift[TS], slip[TS];
    for (int y = 0; y < TS;) {
        uint32_t h = gl_hash(seed * 977u + (uint32_t)y);
        int band = 2 + (int)(h % 6), torn = (h >> 8) % 3 == 0;
        int dx = torn ? (int)((h >> 12) % 9) - 4 : 0, sl = (h >> 20) % 4 == 0;
        for (int k = 0; k < band && y < TS; k++, y++) { shift[y] = dx; slip[y] = sl; }
    }
    for (int pass = 0; pass < 3; pass++)                           /* the red copy, the blue, then the creature */
        for (int y = 0; y < TS; y++) {
            if (pass < 2 && !slip[y]) continue;
            for (int x = 0; x < TS; x++) {
                int sx = x - shift[y] + (pass == 0 ? 2 : pass == 1 ? -2 : 0);
                if (sx < 0 || sx >= TS) continue;
                uint8_t c = crawl_spr[spr][sx * TS + y];
                if (c == C_CLEAR) continue;
                if (pass < 2) {
                    int lum = crawl_pal[c][0] + crawl_pal[c][1] + crawl_pal[c][2];
                    c = (uint8_t)((pass == 0 ? R_BLOOD : R_GLOW) + clampi(lum / 12, 4, 15));
                } else c = crawl_cmap[LIGHT][c];
                rect(ox + x * SC, oy + y * SC, SC, SC, crawl_pal[c][0] * 255 / 63, crawl_pal[c][1] * 255 / 63, crawl_pal[c][2] * 255 / 63);
            }
        }
}
static int draw_backrooms(const char *path, int level) {
    int cw = CELL + PAD, top = PAD + LABEL_H + 8, x0 = PAD + ROWHEAD, row = CELL + 2 * LABEL_H + PAD;
    iw = x0 + 5 * cw + PAD;
    ih = top + 5 * row + 8 * TXT + PAD;
    img = calloc((size_t)iw * ih, 3);
    rect(0, 0, iw, ih, 10, 10, 12);
    char head[96];
    snprintf(head, sizeof head, "1.0.0B6: THE BACKROOMS, FOR APPROVAL - LEVEL %d", level);
    text(PAD, PAD, head, 230, 230, 230);
    crawl_art_theme(PT_BACKROOMS, level);
    int y = top;
    #define LBL(x_, s_) text((x_), y + CELL + 4, (s_), 190, 190, 190)
    #define LBL2(x_, s_) text((x_), y + CELL + 6 + 8 * TXT, (s_), 150, 150, 150)
    text(PAD, y + CELL / 2, "WALLS", 200, 200, 200);
    static const int WALLS[4] = { T_PAPER, T_PAPER_DAMP, T_PAPER_OUTLET, T_BDOOR };
    static const char *const WN[4][2] = { { "WALLPAPER", "" }, { "A LEAK", "" }, { "A SOCKET", "" }, { "DOORWAY", "TO THE NEXT" } };
    for (int k = 0; k < 4; k++) { tex(x0 + k * cw, y, WALLS[k], SC, LIGHT); LBL(x0 + k * cw, WN[k][0]); LBL2(x0 + k * cw, WN[k][1]); }
    y += row;
    text(PAD, y + CELL / 2, "THE END", 200, 200, 200);
    text(PAD, y + CELL / 2 + 8 * TXT + 4, "OF A CHAIN", 200, 200, 200);
    for (int f = 0; f < 4; f++) { tex(x0 + f * cw, y, T_VISPORT0 + f, SC, LIGHT); }
    LBL(x0, "VISPORTAL, FOUR FRAMES"); LBL2(x0, "DRIFTING IN, OVER AND OVER");
    y += row;
    text(PAD, y + CELL / 2, "UNDERFOOT", 200, 200, 200);
    text(PAD, y + CELL / 2 + 8 * TXT + 4, "AND ABOVE", 200, 200, 200);
    static const int FC[5] = { T_CARPET, T_CEILTILE, T_CEILSTAIN, T_CEILLAMP, T_CEILDEAD };
    static const char *const FN[5][2] = { { "CARPET", "" }, { "CEILING", "TILES" }, { "A STAINED", "TILE" }, { "A TUBE", "PANEL, LIT" }, { "AND DEAD", "" } };
    for (int k = 0; k < 5; k++) { tex(x0 + k * cw, y, FC[k], SC, LIGHT); LBL(x0 + k * cw, FN[k][0]); LBL2(x0 + k * cw, FN[k][1]); }
    y += row;
    text(PAD, y + CELL / 2, "AS THEY TILE", 200, 200, 200);
    for (int j = 0; j < 3; j++)                                    /* three by three cells of each, at a third the size */
        for (int i = 0; i < 3; i++) {
            tex(x0 + i * TS, y + j * TS, T_CARPET, 1, LIGHT);
            int t = (i == 1 && j == 1) ? T_CEILLAMP : (i == 0 && j == 2) ? T_CEILDEAD : (i == 2 && j == 0) ? T_CEILSTAIN : T_CEILTILE;
            tex(x0 + cw + i * TS, y + j * TS, t, 1, LIGHT);
            tex(x0 + 2 * cw + i * TS, y + j * TS, (i == 2 && j == 1) ? T_PAPER_DAMP : T_PAPER, 1, LIGHT);
        }
    LBL(x0, "CARPET"); LBL(x0 + cw, "CEILING"); LBL(x0 + 2 * cw, "WALLPAPER");
    y += row;
    crawl_art_level(level);                                        /* back to the crypt's own colours */
    text(PAD, y + CELL / 2, "FROM THE", 200, 200, 200);
    text(PAD, y + CELL / 2 + 8 * TXT + 4, "CRYPT", 200, 200, 200);
    tex(x0, y, T_PASSAGE, SC, LIGHT); LBL(x0, "THE WAY IN"); LBL2(x0, "LEVELS 1-10");
    glitched(x0 + cw, y, SP_SKEL_A, 3); glitched(x0 + 2 * cw, y, SP_SKEL_B, 8); glitched(x0 + 3 * cw, y, SP_BAT_A, 5); glitched(x0 + 4 * cw, y, SP_SPIDER_A, 11);
    LBL(x0 + cw, "GLITCHED: TORN ROWS,"); LBL2(x0 + cw, "COLOURS SLIPPING; A FEW MOMENTS");
    #undef LBL
    #undef LBL2
    text(PAD, ih - PAD - 8 * TXT, "SHADED AS NEAR A LIGHT; THE TUBES AND THE VISPORTAL'S HEART AT FULL BRIGHTNESS.", 120, 120, 120);
    int ok = write_png(path);
    free(img);
    return ok;
}

/* ---- Severance (1.0.0b6): the pocket's textures and things in its palette, and its brighter creatures ---- */

static void shaded(int ox, int oy, int spr, int light) {          /* a sprite at a given light, on the cell's ground */
    rect(ox, oy, CELL, CELL, 22, 21, 24);
    for (int y = 0; y < CELL; y++)
        for (int x = 0; x < CELL; x++) {
            uint8_t c = crawl_spr[spr][(x / SC) * TS + y / SC];
            if (c == C_CLEAR) continue;
            c = crawl_cmap[light][c];
            put(ox + x, oy + y, crawl_pal[c][0] * 255 / 63, crawl_pal[c][1] * 255 / 63, crawl_pal[c][2] * 255 / 63);
        }
}
static int draw_severance(const char *path, int level) {
    int cw = CELL + PAD, top = PAD + LABEL_H + 8, x0 = PAD + ROWHEAD, row = CELL + 2 * LABEL_H + PAD;
    iw = x0 + 5 * cw + PAD;
    ih = top + 5 * row + 8 * TXT + PAD;
    img = calloc((size_t)iw * ih, 3);
    rect(0, 0, iw, ih, 10, 10, 12);
    char head[96];
    snprintf(head, sizeof head, "1.0.0B6: SEVERANCE, FOR APPROVAL - LEVEL %d", level);
    text(PAD, PAD, head, 230, 230, 230);
    crawl_art_theme(PT_SEVERANCE, level);
    int y = top;
    #define LBL(x_, s_) text((x_), y + CELL + 4, (s_), 190, 190, 190)
    #define LBL2(x_, s_) text((x_), y + CELL + 6 + 8 * TXT, (s_), 150, 150, 150)
    text(PAD, y + CELL / 2, "WALLS", 200, 200, 200);
    static const int WALLS[4] = { T_WHITE, T_WHITE_SIGN, T_WHITE_ART, T_SDOOR };
    static const char *const WN[4][2] = { { "WHITE", "" }, { "A PLACARD", "" }, { "A PICTURE", "" }, { "DOORWAY", "TO THE NEXT" } };
    for (int k = 0; k < 4; k++) { tex(x0 + k * cw, y, WALLS[k], SC, LIGHT); LBL(x0 + k * cw, WN[k][0]); LBL2(x0 + k * cw, WN[k][1]); }
    y += row;
    text(PAD, y + CELL / 2, "UNDERFOOT", 200, 200, 200);
    text(PAD, y + CELL / 2 + 8 * TXT + 4, "AND ABOVE", 200, 200, 200);
    tex(x0, y, T_SCARPET, SC, LIGHT); LBL(x0, "CARPET");
    tex(x0 + cw, y, T_SCEIL, SC, LIGHT); LBL(x0 + cw, "PANELS");
    tex(x0 + 2 * cw, y, T_SCEILLAMP, SC, LIGHT); LBL(x0 + 2 * cw, "AND LIT");
    for (int j = 0; j < 3; j++)                                    /* as they tile, a third the size */
        for (int i = 0; i < 3; i++) {
            tex(x0 + 3 * cw + i * TS, y + j * TS, (i == 1 && j == 1) ? T_SCEILLAMP : T_SCEIL, 1, LIGHT);
            tex(x0 + 4 * cw + i * TS, y + j * TS, (i == 1 && j == 0) ? T_SDOOR : (i == 2 && j == 2) ? T_WHITE_SIGN : T_WHITE, 1, LIGHT);
        }
    LBL(x0 + 3 * cw, "CEILING"); LBL(x0 + 4 * cw, "WALLS"); LBL2(x0 + 3 * cw, "AS THEY TILE");
    y += row;
    text(PAD, y + CELL / 2, "THINGS", 200, 200, 200);
    shaded(x0, y, SP_DESKS, LIGHT); LBL(x0, "FOUR DESKS"); LBL2(x0, "AND SCREENS");
    shaded(x0 + cw, y, SP_COOLER, LIGHT); LBL(x0 + cw, "THE WATER"); LBL2(x0 + cw, "COOLER");
    shaded(x0 + 2 * cw, y, SP_COOLER_LOW, LIGHT); LBL(x0 + 2 * cw, "AFTER ITS"); LBL2(x0 + 2 * cw, "ONE DRINK");
    y += row;
    text(PAD, y + CELL / 2, "BRIGHTER", 200, 200, 200);
    text(PAD, y + CELL / 2 + 8 * TXT + 4, "CREATURES", 200, 200, 200);
    static const int CR[4] = { SP_SKEL_A, SP_SHIELD_A, SP_GHOUL_A, SP_KNIGHT_A };
    for (int k = 0; k < 2; k++) {                                  /* as in the crypt, then as Severance shows them */
        shaded(x0 + (2 * k) * cw, y, CR[k * 2], 22); shaded(x0 + (2 * k + 1) * cw, y, CR[k * 2], 31);
    }
    shaded(x0 + 4 * cw, y, CR[3], 31);
    LBL(x0, "IN THE CRYPT,"); LBL2(x0, "THEN UNDER THE PANELS");
    y += row;
    crawl_art_level(level);
    text(PAD, y + CELL / 2, "FROM THE", 200, 200, 200);
    text(PAD, y + CELL / 2 + 8 * TXT + 4, "CRYPT", 200, 200, 200);
    tex(x0, y, T_PASSAGE, SC, LIGHT); LBL(x0, "THE WAY IN"); LBL2(x0, "LEVELS 11-20");
    #undef LBL
    #undef LBL2
    text(PAD, ih - PAD - 8 * TXT, "SHADED AS NEAR A LIGHT; THE LIT PANELS AT FULL BRIGHTNESS.", 120, 120, 120);
    int ok = write_png(path);
    free(img);
    return ok;
}

/* ---- 1.0.0b7's walls: the Accursed Portal, and a chain's doorway sealed after a failed trial ---- */
static int draw_b7walls(const char *path, int level) {
    int cw = CELL + PAD, top = PAD + LABEL_H + 8, x0 = PAD + ROWHEAD, row = CELL + 2 * LABEL_H + PAD;
    iw = x0 + 4 * cw + PAD;
    ih = top + 2 * row + 8 * TXT + PAD;
    img = calloc((size_t)iw * ih, 3);
    rect(0, 0, iw, ih, 10, 10, 12);
    char head[96];
    snprintf(head, sizeof head, "1.0.0B7: WALLS, FOR APPROVAL - LEVEL %d", level);
    text(PAD, PAD, head, 230, 230, 230);
    int y = top;
    #define LBL(x_, s_) text((x_), y + CELL + 4, (s_), 190, 190, 190)
    #define LBL2(x_, s_) text((x_), y + CELL + 6 + 8 * TXT, (s_), 150, 150, 150)
    text(PAD, y + CELL / 2, "THE PORTAL", 200, 200, 200);
    for (int f = 0; f < 4; f++) tex(x0 + f * cw, y, T_PORTAL0 + f, SC, LIGHT);
    LBL(x0, "THE ACCURSED PORTAL, FOUR FRAMES"); LBL2(x0, "A SLOW RED-VIOLET SWIRL");
    y += row;
    text(PAD, y + CELL / 2, "WALLS", 200, 200, 200);
    tex(x0, y, T_PASSAGE, SC, LIGHT); LBL(x0, "THE WAY IN"); LBL2(x0, "AS IT WAS");
    tex(x0 + cw, y, T_SEALED, SC, LIGHT); LBL(x0 + cw, "SEALED"); LBL2(x0 + cw, "FOR THE REST OF THE LEVEL");
    tex(x0 + 2 * cw, y, T_REPOSITORY, SC, LIGHT); LBL(x0 + 2 * cw, "THE REPOSITORY"); LBL2(x0 + 2 * cw, "");
    #undef LBL
    #undef LBL2
    text(PAD, ih - PAD - 8 * TXT, "SHADED AS NEAR A TORCH.", 120, 120, 120);
    int ok = write_png(path);
    free(img);
    return ok;
}

/* ---- a run's own look (docs/PLAN-b10.md): its parts, and its rooms laid out as the generator would ---- */

typedef struct { uint8_t tex[NTEX][TS * TS]; uint8_t pal[256][3]; uint8_t cmap[NLIGHT][256]; } Snap;
static Snap g_snap[2];                                              /* the run's first level and its last */
static void snap_take(Snap *s, int level) {
    crawl_art_level(level);
    crawl_art_run(level);
    memcpy(s->tex, crawl_tex, sizeof crawl_tex); memcpy(s->pal, crawl_pal, sizeof crawl_pal); memcpy(s->cmap, crawl_cmap, sizeof crawl_cmap);
}
static void textk(int x, int y, const char *s, int k, int r, int g, int b) {   /* the VGA font at any scale */
    for (; *s; s++, x += 8 * k)
        for (int row = 0; row < 8; row++)
            for (int col = 0; col < 8; col++)
                if (font_bios8[(uint8_t)*s][row] >> (7 - col) & 1) rect(x + col * k, y + row * k, k, k, r, g, b);
}
/* a texture as the game would draw it near a torch: flip for a wall seen from the other side, an overlay
 * standing on it (-1 none), rot for a floor cell turned about as the game turns them */
static void rtex(const Snap *s, int ox, int oy, int t, int scale, int flip, int over, int rot) {
    for (int y = 0; y < TS * scale; y++)
        for (int x = 0; x < TS * scale; x++) {
            int tx = x / scale, ty = y / scale;
            if (flip) tx = TS - 1 - tx;
            if (rot & 1) tx = TS - 1 - tx;
            if (rot & 2) ty = TS - 1 - ty;
            if (rot & 4) { int q = tx; tx = ty; ty = q; }
            uint8_t c = s->tex[t][tx * TS + ty];
            if (over >= 0 && s->tex[over][tx * TS + ty] != C_CLEAR) c = s->tex[over][tx * TS + ty];
            c = s->cmap[LIGHT][c];
            put(ox + x, oy + y, s->pal[c][0] * 255 / 63, s->pal[c][1] * 255 / 63, s->pal[c][2] * 255 / 63);
        }
}
static void rfloor(const Snap *s, int ox, int oy, int t, int scale, int n, uint32_t seed, int broken) {   /* n by n cells */
    for (int j = 0; j < n; j++)
        for (int i = 0; i < n; i++) {
            uint32_t h = (uint32_t)(i * 7 + j * 13 + 1) * 2654435761u ^ seed;
            int tt = broken && ((h >> 9) % 4 == 0) ? T_FLOORS + 4 : t;
            rtex(s, ox + i * TS * scale, oy + j * TS * scale, tt, scale, 0, -1, t == T_FLOORS + 1 ? 0 : (int)((h >> 5) & 7));   /* a pattern lies as laid */
        }
}

typedef struct { int t, flip, over; } Face;                       /* one wall cell in a strip */
#define MV(m, v) T_MATV(m, v)
#define F_(t) { (t), 0, -1 }
#define FO(t, o) { (t), 0, T_FEAT + (o) }
#define FB(k, fl) { T_BLEND + (k), (fl), -1 }
typedef struct { const char *name, *what; Face f[8]; int floor, ceil, broken; } RoomMock;
enum { A_, R_, P_, B_, O_ };                                        /* the Crypts' materials, as crawl_art.c numbers them */
enum { NI, BA, SL, SR, TA, TO };
static const RoomMock CRYPT_ROOMS[] = {
    { "THE HALLS", "ASHLAR, PATCHED WITH RUBBLE, CUT INTO THE ROCK AT THE FAR END; TORCHES",
      { F_(MV(A_, 0)), FO(MV(A_, 1), TO), F_(MV(A_, 0)), FB(0, 0), F_(MV(R_, 1)), FO(MV(R_, 0), TO), FB(4, 0), F_(MV(B_, 0)) }, 2, 1, 0 },
    { "OSSUARY", "BONE WALLS FACING IN, DRESSED STONE AT THE ENDS",
      { F_(MV(A_, 0)), FB(5, 1), F_(MV(O_, 0)), F_(MV(O_, 1)), F_(MV(O_, 0)), F_(MV(O_, 0)), FB(5, 0), F_(MV(A_, 1)) }, 3, 1, 0 },
    { "BURIAL VAULT", "NICHES IN THE ASHLAR, A TORCH BETWEEN",
      { F_(MV(A_, 0)), FO(MV(A_, 0), NI), FO(MV(A_, 1), NI), FO(MV(A_, 0), TO), FO(MV(A_, 0), NI), FO(MV(A_, 0), NI), F_(MV(A_, 1)), FO(MV(A_, 0), NI) }, 0, 0, 1 },
    { "CHAPTER HOUSE", "PLASTERED AND PAINTED, THE ORDER'S BANNERS, A TABLET",
      { F_(MV(P_, 0)), FO(MV(P_, 0), BA), F_(MV(P_, 1)), FO(MV(P_, 0), TA), FO(MV(P_, 0), TO), FO(MV(P_, 0), BA), FB(1, 0), F_(MV(A_, 0)) }, 1, 0, 0 },
    { "KNIGHTS' VAULT", "SLABS OF THE ORDER'S KNIGHTS IN THE ASHLAR, A BANNER OVER THEM",
      { F_(MV(A_, 0)), FO(MV(A_, 0), SL), FO(MV(A_, 1), TO), FO(MV(A_, 0), SL), FO(MV(A_, 0), BA), FO(MV(A_, 1), SL), FO(MV(A_, 0), TO), F_(MV(A_, 1)) }, 0, 0, 1 },
    { "UNDERCROFT", "RUBBLE GIVING WAY TO THE ROCK, A SHRINE CUT IN IT",
      { F_(MV(R_, 0)), FO(MV(R_, 1), TO), FB(4, 0), F_(MV(B_, 0)), F_(MV(B_, 1)), FO(MV(B_, 0), SR), FB(3, 1), F_(MV(A_, 0)) }, 2, 2, 0 },
};
enum { SL2, LN2, SI2, FW2, BS2 };                                   /* the Catacombs' */
enum { NC, SA, CU, HA, WE, LA };
static const RoomMock CATA_ROOMS[] = {
    { "THE HALLS", "THE SLATE AS IT WAS CUT, LINED IN PLACES, PATCHED WITH SILT; LANTERNS",
      { F_(MV(SL2, 0)), FO(MV(SL2, 1), LA), F_(MV(SL2, 0)), FB(0, 1), F_(MV(LN2, 0)), FO(MV(LN2, 1), LA), FB(1, 0), F_(MV(SI2, 0)) }, 2, 1, 0 },
    { "SAINTS' CATACOMB", "SLATE LINING, SAINTS IN THEIR ALCOVES, THE FLOOD STANDING IN THE NICHES",
      { F_(MV(LN2, 0)), FO(MV(LN2, 0), NC), FO(MV(LN2, 1), SA), FO(MV(LN2, 0), LA), FO(MV(LN2, 0), SA), FO(MV(LN2, 0), NC), F_(MV(LN2, 1)), FO(MV(LN2, 0), SA) }, 0, 0, 1 },
    { "CISTERN", "FLOWSTONE OVER THE LINING, CULVERTS POURING IN, WATER UNDERFOOT",
      { F_(MV(LN2, 0)), FB(4, 1), F_(MV(FW2, 0)), F_(MV(FW2, 1)), FB(4, 0), FO(MV(LN2, 0), CU), FO(MV(LN2, 1), LA), FO(MV(LN2, 0), CU) }, 3, 2, 0 },
    { "SILTED NAVE", "SILT BANKED UP THE SLATE, HANDPRINTS WHERE THE WATER CAME",
      { F_(MV(SI2, 0)), FO(MV(SI2, 1), HA), F_(MV(SI2, 0)), FB(2, 1), F_(MV(SL2, 0)), FB(0, 1), FO(MV(LN2, 0), HA), F_(MV(LN2, 1)) }, 1, 0, 0 },
    { "CANAL", "SLATE LINING BOTH SIDES, CULVERTS, LANTERNS OVER THE WATER",
      { F_(MV(LN2, 0)), FO(MV(LN2, 1), CU), F_(MV(LN2, 0)), FO(MV(LN2, 0), LA), F_(MV(LN2, 1)), FO(MV(LN2, 0), CU), F_(MV(LN2, 0)), FO(MV(LN2, 1), LA) }, 3, 1, 0 },
    { "SPINNERS' GALLERY", "BONE IN SILT, WEBS ACROSS IT",
      { F_(MV(SI2, 0)), FB(5, 1), F_(MV(BS2, 0)), FO(MV(BS2, 1), WE), FO(MV(BS2, 0), WE), F_(MV(BS2, 0)), FB(5, 0), FO(MV(SI2, 0), WE) }, 1, 2, 0 },
};
/* each run's sheets: its levels, its title, which material each feature is shown on (first level, last) */
typedef struct { const char *title, *rooms_title, *treasure; const RoomMock *rooms; int nrooms; int ft[6][3]; } RunSheet;
static const RunSheet RUNSHEETS[2] = {
    { "RUN I, THE CRYPTS: THE ORDER'S CRYPTS, FOR APPROVAL", "RUN I'S ROOMS: A STRETCH OF EACH AT LEVEL 1, THEN 10", "AS IT WAS",
      CRYPT_ROOMS, (int)(sizeof CRYPT_ROOMS / sizeof CRYPT_ROOMS[0]),
      { { NI, A_, B_ }, { BA, P_, A_ }, { SL, A_, A_ }, { SR, B_, R_ }, { TA, P_, A_ }, { TO, R_, A_ } } },
    { "RUN II, THE CATACOMBS: THE DROWNED CATACOMBS, FOR APPROVAL", "RUN II'S ROOMS: A STRETCH OF EACH AT LEVEL 11, THEN 20", "SILTED, AS THE WATER LEFT IT",
      CATA_ROOMS, (int)(sizeof CATA_ROOMS / sizeof CATA_ROOMS[0]),
      { { NC, LN2, SL2 }, { SA, LN2, LN2 }, { CU, LN2, SL2 }, { HA, SI2, LN2 }, { WE, BS2, SL2 }, { LA, SL2, LN2 } } },
};

static void spr_at(const Snap *s, int ox, int oy, int id, int scale) {   /* a sprite as near a torch, on a dark plate */
    rect(ox, oy, TS * scale, TS * scale, 22, 21, 24);
    for (int yy = 0; yy < TS * scale; yy++)
        for (int xx = 0; xx < TS * scale; xx++) {
            uint8_t k = crawl_spr[id][(xx / scale) * TS + yy / scale];
            if (k == C_CLEAR) continue;
            k = s->cmap[LIGHT][k];
            put(ox + xx, oy + yy, s->pal[k][0] * 255 / 63, s->pal[k][1] * 255 / 63, s->pal[k][2] * 255 / 63);
        }
}

static int draw_runlook(const char *path, int run) {
    const RunLook *L = &crawl_looks[run];
    const RunSheet *R = &RUNSHEETS[run];
    int first = run * RUN_LEN + 1, last = run * RUN_LEN + RUN_LEN;
    char lvs[2][16];
    snprintf(lvs[0], sizeof lvs[0], "LEVEL %d", first); snprintf(lvs[1], sizeof lvs[1], "LEVEL %d", last);
    snap_take(&g_snap[1], last);
    snap_take(&g_snap[0], first);                                   /* last, so the keys and treasure are the first level's */
    const int S2 = 2, T2 = TS * S2, G = 10, x0 = PAD + ROWHEAD, lab = 8 * TXT + 6;
    iw = x0 + NMAT * (2 * T2 + 3 * G) + PAD;
    ih = 3200;
    img = calloc((size_t)iw * ih, 3);
    rect(0, 0, iw, ih, 10, 10, 12);
    textk(PAD, PAD, R->title, 3, 230, 230, 230);
    int y = PAD + 24 + 18;
    #define HEAD(s) textk(PAD, y + 4, (s), TXT, 230, 200, 120)
    #define SUB(xx, yy, s) textk((xx), (yy), (s), 1, 160, 160, 160)
    for (int lv = 0; lv < 2; lv++) {                                /* the materials, both looks side by side */
        HEAD("MATERIALS"); SUB(PAD, y + 24, lvs[lv]); SUB(PAD, y + 36, "TWO LOOKS EACH");
        for (int m = 0; m < NMAT; m++) {
            int x = x0 + m * (2 * T2 + 3 * G);
            rtex(&g_snap[lv], x, y, MV(m, 0), S2, 0, -1, 0); rtex(&g_snap[lv], x + T2, y, MV(m, 1), S2, 0, -1, 0);
            if (!lv) textk(x, y + T2 + 4, L->mat[m], TXT, 190, 190, 190);
        }
        y += T2 + (lv ? G * 2 : lab + G);
    }
    y += G;
    for (int lv = 0; lv < 2; lv++) {                                /* each blend between its two materials */
        for (int half = 0; half < 2; half++) {
            if (!half) { HEAD("BLENDS"); SUB(PAD, y + 24, lvs[lv]); SUB(PAD, y + 36, "THE FIRST IN FRONT"); }
            for (int k = half * 3; k < half * 3 + 3; k++) {
                int x = x0 + (k - half * 3) * (3 * T2 + 3 * G), a = L->blend[k][0], b = L->blend[k][1];
                rtex(&g_snap[lv], x, y, MV(a, 0), S2, 0, -1, 0); rtex(&g_snap[lv], x + T2, y, T_BLEND + k, S2, 0, -1, 0); rtex(&g_snap[lv], x + 2 * T2, y, MV(b, 0), S2, 0, -1, 0);
                if (!lv) {
                    char s[64]; snprintf(s, sizeof s, "%s INTO %s", L->mat[a], L->mat[b]);
                    SUB(x, y + T2 + 4, s);
                }
            }
            y += T2 + (lv ? G : 16 + G);
        }
        y += G;
    }
    HEAD("ON THE WALLS"); SUB(PAD, y + 24, "OVER ANY MATERIAL"); SUB(PAD, y + 36, "FIRST LEVEL, THEN LAST");
    for (int row = 0; row < 2; row++) {                             /* what stands on the walls, on the materials it would stand on */
        for (int i = row * 3; i < row * 3 + 3; i++) {
            int x = x0 + (i - row * 3) * (2 * T2 + 4 * G), f = R->ft[i][0];
            for (int lv = 0; lv < 2; lv++) rtex(&g_snap[lv], x + lv * T2, y, MV(R->ft[i][1 + lv], 0), S2, 0, T_FEAT + f, 0);
            SUB(x, y + T2 + 4, L->feat[f]);
            char s[48]; snprintf(s, sizeof s, "ON %s, %s", L->mat[R->ft[i][1]], L->mat[R->ft[i][2]]);
            SUB(x, y + T2 + 14, s);
        }
        y += T2 + 26 + G;
    }
    y += G;
    HEAD("FLOORS"); SUB(PAD, y + 24, "THREE BY THREE CELLS"); SUB(PAD, y + 36, "TURNED AS IN PLAY");
    for (int lv = 0; lv < 2; lv++) {
        for (int i = 0; i < NFLOORS - 1; i++) {
            int x = x0 + i * (3 * TS + 2 * G);
            rfloor(&g_snap[lv], x, y, T_FLOORS + i, 1, 3, (uint32_t)(i * 31 + 5), lv && (i == 0));
            if (!lv) SUB(x, y + 3 * TS + 4, L->floor[i]);
        }
        char s[64];
        if (lv) snprintf(s, sizeof s, "%s: SOME %s", lvs[1], L->floor[0]); else snprintf(s, sizeof s, "%s", lvs[0]);
        SUB(x0 + 4 * (3 * TS + 2 * G), y + 4, s);
        if (lv) { snprintf(s, sizeof s, "%s BY THEN", L->floor[NFLOORS - 1]); SUB(x0 + 4 * (3 * TS + 2 * G), y + 16, s); }
        y += 3 * TS + (lv ? G : 16 + G);
    }
    y += G;
    HEAD("CEILINGS"); SUB(PAD, y + 24, "FIRST LEVEL, THEN LAST");
    for (int lv = 0; lv < 2; lv++)
        for (int i = 0; i < NCEILS; i++) {
            int x = x0 + (lv * NCEILS + i) * (2 * TS + 3 * G) + lv * 3 * G;
            rfloor(&g_snap[lv], x, y, T_CEILS + i, 1, 2, (uint32_t)(i * 17 + 3), 0);
            SUB(x, y + 2 * TS + 4, L->ceil[i]);
        }
    y += 2 * TS + 16 + 2 * G;
    HEAD("DOORS"); SUB(PAD, y + 24, "THE VAULTS IN THEIR");  SUB(PAD, y + 36, "KEYS' METALS");
    static const int DT[6] = { T_RDOOR, T_RJAMB, T_RLOCKED, T_RLOCKED_S, T_RLOCKED_C, T_RDOOR };
    char dn5[32]; snprintf(dn5, sizeof dn5, "DOOR, %s", lvs[1]);
    const char *DN[6] = { "DOOR", "ITS JAMB", "GOLD VAULT", "SILVER VAULT", "CRIMSON VAULT", dn5 };
    for (int i = 0; i < 6; i++) {
        int x = x0 + i * (T2 + 3 * G);
        rtex(&g_snap[i == 5], x, y, DT[i], S2, 0, -1, 0);
        SUB(x, y + T2 + 4, DN[i]);
    }
    y += T2 + 16 + 2 * G;
    HEAD("KEYS"); SUB(PAD, y + 24, "TURNING, FOUR FRAMES");
    static const int KB[3] = { SP_KEY0, SP_KEYS0, SP_KEYC0 };
    for (int c = 0; c < 3; c++)
        for (int f = 0; f < 4; f++) {
            if (c && f > 1) continue;                              /* every frame of the gold; the others face on and turning */
            spr_at(&g_snap[0], x0 + (c ? 4 + (c - 1) * 2 + f : f) * (T2 + 6) + c * 2 * G, y, KB[c] + f, S2);
        }
    y += T2 + 2 * G;
    HEAD("TREASURE"); SUB(PAD, y + 24, R->treasure);
    static const int TR[6] = { SP_COINS_A, SP_CHALICE, SP_GEMS, SP_CROWN, SP_CHEST, SP_POTION };
    for (int i = 0; i < 5; i++) spr_at(&g_snap[0], x0 + i * (T2 + 6), y, TR[i], S2);
    y += T2 + 2 * G;
    #undef HEAD
    #undef SUB
    ih = y + PAD;
    int ok = write_png(path);
    free(img);
    return ok;
}

static int draw_runrooms(const char *path, int run) {
    const RunLook *L = &crawl_looks[run];
    const RunSheet *RS = &RUNSHEETS[run];
    snap_take(&g_snap[1], run * RUN_LEN + RUN_LEN);
    snap_take(&g_snap[0], run * RUN_LEN + 1);
    const int S2 = 2, T2 = TS * S2, G = 10, x0 = PAD;
    int nr = RS->nrooms;
    iw = x0 + 8 * T2 + 2 * G + 2 * TS + G + TS * 2 + PAD;
    ih = 80 + nr * (2 * T2 + 3 * G + 40);
    img = calloc((size_t)iw * ih, 3);
    rect(0, 0, iw, ih, 10, 10, 12);
    textk(PAD, PAD, RS->rooms_title, 2, 230, 230, 230);
    int y = PAD + 16 + 20;
    for (int r = 0; r < nr; r++) {
        const RoomMock *R = &RS->rooms[r];
        textk(x0, y, R->name, TXT, 230, 200, 120);
        textk(x0 + (int)strlen(R->name) * 8 * TXT + 16, y + 6, R->what, 1, 170, 170, 170);
        y += 22;
        for (int lv = 0; lv < 2; lv++) {
            for (int i = 0; i < 8; i++) rtex(&g_snap[lv], x0 + i * T2, y + lv * (T2 + G), R->f[i].t, S2, R->f[i].flip, R->f[i].over, 0);
            int fx = x0 + 8 * T2 + 2 * G;
            rfloor(&g_snap[lv], fx, y + lv * (T2 + G), T_FLOORS + R->floor, 1, 2, (uint32_t)(r * 11 + 2), lv && R->broken);
            rfloor(&g_snap[lv], fx + 2 * TS + G, y + lv * (T2 + G), T_CEILS + R->ceil, 1, 2, (uint32_t)(r * 13 + 7), 0);
        }
        int fx = x0 + 8 * T2 + 2 * G;
        textk(fx, y + 2 * T2 + G + 4, L->floor[R->floor], 1, 170, 170, 170);
        textk(fx + 2 * TS + G, y + 2 * T2 + G + 4, L->ceil[R->ceil], 1, 170, 170, 170);
        y += 2 * T2 + G + 18 + 2 * G;
    }
    ih = y + PAD;
    int ok = write_png(path);
    free(img);
    return ok;
}

int main(int argc, char **argv) {
    if (argc >= 2 && !strcmp(argv[1], "list")) {
        for (int i = 0; i < NGROUPS; i++) printf("%-12s %s\n", GROUPS[i].name, GROUPS[i].title);
        printf("%-12s %s\n", "lineup", "EVERY CREATURE AT ITS SIZE IN PLAY");
        printf("%-12s %s\n", "shrine", "THE RUNE SHRINE, ITS RUNES AND THE CURSED COIN");
        printf("%-12s %s\n", "backrooms", "THE BACKROOMS: WALLS, FLOOR, CEILING, THE VISPORTAL, THE GLITCH");
        printf("%-12s %s\n", "severance", "SEVERANCE: WALLS, DOORS, FLOOR, CEILING, DESKS, THE WATER COOLER");
        printf("%-12s %s\n", "b7walls", "1.0.0B7: THE PORTAL, A SEALED DOORWAY, THE REPOSITORY");
        printf("%-12s %s\n", "crypts", "RUN I'S OWN LOOK: MATERIALS, BLENDS, FEATURES, FLOORS, CEILINGS, DOORS, KEYS");
        printf("%-12s %s\n", "cryptrooms", "RUN I'S ROOMS, LAID AS THE GENERATOR WOULD LAY THEM");
        printf("%-12s %s\n", "catacombs", "RUN II'S OWN LOOK, THE SAME WAY");
        printf("%-12s %s\n", "catarooms", "RUN II'S ROOMS");
        return 0;
    }
    if (argc < 3) { fprintf(stderr, "usage: sheet list | sheet <group|all> <out.png|dir> [level]\n"); return 2; }
    int level = argc > 3 ? atoi(argv[3]) : 1;
    crawl_art_init();
    crawl_art_level(level < 1 ? 1 : level);
    int done = 0;
    if (!strcmp(argv[1], "all") || !strcmp(argv[1], "lineup")) {
        char path[512];
        if (!strcmp(argv[1], "all")) snprintf(path, sizeof path, "%s/lineup.png", argv[2]);
        else snprintf(path, sizeof path, "%s", argv[2]);
        if (!draw_lineup(path, level)) { fprintf(stderr, "can't write %s\n", path); return 1; }
        done++;
    }
    if (!strcmp(argv[1], "all") || !strcmp(argv[1], "shrine")) {
        char path[512];
        if (!strcmp(argv[1], "all")) snprintf(path, sizeof path, "%s/shrine.png", argv[2]);
        else snprintf(path, sizeof path, "%s", argv[2]);
        if (!draw_shrine(path, level)) { fprintf(stderr, "can't write %s\n", path); return 1; }
        done++;
    }
    if (!strcmp(argv[1], "all") || !strcmp(argv[1], "backrooms")) {
        char path[512];
        if (!strcmp(argv[1], "all")) snprintf(path, sizeof path, "%s/backrooms.png", argv[2]);
        else snprintf(path, sizeof path, "%s", argv[2]);
        if (!draw_backrooms(path, level)) { fprintf(stderr, "can't write %s\n", path); return 1; }
        done++;
    }
    if (!strcmp(argv[1], "all") || !strcmp(argv[1], "severance")) {
        char path[512];
        if (!strcmp(argv[1], "all")) snprintf(path, sizeof path, "%s/severance.png", argv[2]);
        else snprintf(path, sizeof path, "%s", argv[2]);
        if (!draw_severance(path, level)) { fprintf(stderr, "can't write %s\n", path); return 1; }
        done++;
    }
    if (!strcmp(argv[1], "all") || !strcmp(argv[1], "b7walls")) {
        char path[512];
        if (!strcmp(argv[1], "all")) snprintf(path, sizeof path, "%s/b7walls.png", argv[2]);
        else snprintf(path, sizeof path, "%s", argv[2]);
        if (!draw_b7walls(path, level)) { fprintf(stderr, "can't write %s\n", path); return 1; }
        done++;
    }
    static const char *const RUNSHEET_NAMES[2][2] = { { "crypts", "cryptrooms" }, { "catacombs", "catarooms" } };
    for (int run = 0; run < 2; run++)
        for (int k = 0; k < 2; k++) {
            const char *nm = RUNSHEET_NAMES[run][k];
            char path[512];
            if (strcmp(argv[1], "all") && strcmp(argv[1], nm)) continue;
            if (!strcmp(argv[1], "all")) snprintf(path, sizeof path, "%s/%s.png", argv[2], nm);
            else snprintf(path, sizeof path, "%s", argv[2]);
            if (!(k ? draw_runrooms(path, run) : draw_runlook(path, run))) { fprintf(stderr, "can't write %s\n", path); return 1; }
            crawl_art_level(level < 1 ? 1 : level);
            done++;
        }
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
