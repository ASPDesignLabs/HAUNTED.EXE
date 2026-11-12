/* The carved stone's sheet, for approval (docs/PLAN-ui.md step 1): the Alignment laid out by the kit for each line in
 * its states, the marble by run, the faces in their ways, the kit's pieces and the icons. Each panel is a 320x200 frame
 * drawn as the game will draw it, over a dimmed stretch of the run's wall, doubled.
 *
 *   uisheet <out.png> [level]       level picks the run's stone (default 1)
 *   uisheet all <dir>               ui.png at level 1, ui_runs.png the marble's lean in each run
 *   uisheet fit                     every node in the pane with the longest foot; fails on any overflow
 *   uisheet tutor <dir>             the lessons' pages (docs/PLAN-tutorial.md): pages1.png level 1's and her room's,
 *                                   pages2.png level 2's, a few in a pad's and touch's words; fails on any overflow
 */
#include "../src/crawl.h"
#include "../src/crawl_ui.h"
#include "../src/crawl_tutor.h"
#include "../src/gfx.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>

static Frame F;
static uint8_t *img;
static int iw, ih;

static void put(int x, int y, const uint8_t *p6) {
    if ((unsigned)x >= (unsigned)iw || (unsigned)y >= (unsigned)ih) return;
    uint8_t *p = &img[((size_t)y * iw + x) * 3];
    for (int c = 0; c < 3; c++) p[c] = (uint8_t)(p6[c] * 255 / 63);
}
static void blit(int ox, int oy, int sc) {                 /* the frame, through its own palette, sc times */
    for (int y = 0; y < F.h * sc; y++) for (int x = 0; x < F.w * sc; x++) put(ox + x, oy + y, F.pal[F.pix[(y / sc) * F.w + x / sc]]);
}
static void label(int x, int y, const char *s) {           /* the sheet's own words, in the BIOS face, twice size */
    static const uint8_t W[3] = { 52, 52, 52 };
    for (; *s; s++, x += 16)
        for (int r = 0; r < 8; r++) for (int c = 0; c < 8; c++)
            if (font_bios8[(uint8_t)*s][r] >> (7 - c) & 1) { put(x + c * 2, y + r * 2, W); put(x + c * 2 + 1, y + r * 2, W); put(x + c * 2, y + r * 2 + 1, W); put(x + c * 2 + 1, y + r * 2 + 1, W); }
}

/* ---- PNG ---- */
static void put32(FILE *f, uint32_t v) { uint8_t b[4] = { v >> 24, v >> 16, v >> 8, v }; fwrite(b, 1, 4, f); }
static void chunk(FILE *f, const char *type, const uint8_t *data, uint32_t len) {
    put32(f, len); fwrite(type, 1, 4, f); if (len) fwrite(data, 1, len, f);
    uint32_t crc = crc32(0, (const Bytef *)type, 4); if (len) crc = crc32(crc, data, len); put32(f, crc);
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
    chunk(f, "IHDR", ihdr, 13); chunk(f, "IDAT", comp, (uint32_t)clen); chunk(f, "IEND", NULL, 0);
    fclose(f); free(raw); free(comp);
    return 1;
}

/* a frame begun: the run's wall, lit low, dimmed every other pixel as a menu dims it; then the marble lent */
static void frame_begin(int dim) {
    F.w = 320; F.h = 200;
    memcpy(F.pal, crawl_pal, sizeof F.pal);
    for (int y = 0; y < 200; y++) for (int x = 0; x < 320; x++) {
        uint8_t c = crawl_tex[T_MATV(0, (x / 64 + y / 64) % NVAR)][(x % 64) * TS + y % 64];
        F.pix[y * 320 + x] = crawl_cmap[18 + (y > 120 ? 0 : 4)][c];
    }
    if (dim) for (int y = 0; y < 200; y++) for (int x = (y & 1); x < 320; x += 2) F.pix[y * 320 + x] = 0;
    ui_lend(&F);
}

/* ---- the Alignment, as the game knows it ---- */
static const struct { const char *name; int cost; const char *does; } LINES_[6][10] = {
    {
        { "FIRE", 0, "THE FIREBALL, YOURS FROM THE START." },
        { "EFFICIENCY", 2, "FIRE COSTS 5% LESS SPIRIT A POINT." },
        { "INSIGHT", 2, "FIRE GATHERS 15% QUICKER A POINT." },
        { "FIRESTARTER", 3, "THE FIREBALL SETS THE FLOOR ALIGHT ALONG ITS FLIGHT; WHAT WALKS THROUGH IT CATCHES FIRE." },
        { "EFFICIENCY II", 2, "FIRE COSTS 10% LESS SPIRIT A POINT." },
        { "BURNING INTENSITY", 2, "WHAT BURNS BURNS HALF AS LONG AGAIN, A POINT." },
        { "FLAMEWALKER", 5, "WALKING THROUGH YOUR FLAMES GIVES YOU HALF YOUR ARMOUR AGAIN FOR FIVE SECONDS." },
        { "INSIGHT II", 2, "FIRE GATHERS 15% QUICKER A POINT." },
        { "FIRE MASTERY", 5, "FIRE HITS A QUARTER HARDER." },
        { "FIREWALL", 5, "HOLD TO CALL THREE FLAME SPIRITS ROUND YOU; WHAT THEY TOUCH BURNS." },
    },
    {
        { "FROST", 3, "FROST SHARD: A SHARD THAT SLOWS WHAT IT HITS." },
        { "EFFICIENCY", 2, "FROST COSTS 5% LESS SPIRIT A POINT." },
        { "INSIGHT", 2, "FROST GATHERS 15% QUICKER A POINT." },
        { "DEEP CHILL", 3, "EVERY SHARD CHILLS; THE THIRD ON ONE CREATURE IN FOUR SECONDS FREEZES IT." },
        { "EFFICIENCY II", 2, "FROST COSTS 10% LESS SPIRIT A POINT." },
        { "PERMAFROST", 2, "SLOWING AND FREEZING LAST HALF AS LONG AGAIN, A POINT." },
        { "WINTER'S LANCE", 5, "HOLD TO CHARGE A LANCE THROUGH EVERYTHING IN ITS LINE; IT FREEZES, AND RIMES THE FLOOR." },
        { "INSIGHT II", 2, "FROST GATHERS 20% QUICKER A POINT." },
        { "FROST MASTERY", 5, "FROST HITS A QUARTER HARDER, AND A TENTH OF IT GOES WITH EVERY OTHER SPELL." },
        { "SHATTER", 5, "A FROZEN CREATURE STRUCK SHATTERS FOR DOUBLE, ITS SHARDS CUTTING ALL ROUND IT." },
    },
    {
        { "LIGHTNING", 3, "A BOLT FROM THE HAND THAT STRIKES WHAT YOU AIM AT, AT ONCE." },
        { "EFFICIENCY", 2, "LIGHTNING COSTS 5% LESS SPIRIT A POINT." },
        { "INSIGHT", 2, "LIGHTNING GATHERS 15% QUICKER A POINT." },
        { "IONIZATION", 3, "LIGHTNING LEAVES A TRAIL THAT HURTS WHAT CROSSES IT, HALF A SECOND." },
        { "EFFICIENCY II", 2, "LIGHTNING COSTS 10% LESS SPIRIT A POINT." },
        { "LIGHTNING MASTERY", 5, "LIGHTNING HITS 30% HARDER, AND A FIFTH OF IT GOES WITH EVERY OTHER SPELL." },
        { "ARC", 5, "THE BOLT LEAPS ON TO THE NEAREST, UP TO FOUR LEAPS, EACH A QUARTER WEAKER." },
        { "INSIGHT II", 2, "LIGHTNING GATHERS 20% QUICKER A POINT." },
        { "LIGHTNING MASTERY", 5, "LIGHTNING HITS A FIFTH HARDER, AND A TENTH OF IT GOES WITH EVERY OTHER SPELL." },
        { "AMPERAGE", 5, "HOLD TO CHARGE AN ORB THAT GOES THROUGH ALL IT MEETS, ARCING TO ONE MORE FROM EACH." },
    },
    {
        { "POISON", 3, "THE BLIGHT FLASK: LOBBED, IT BURSTS AND SICKENS WHAT IT SPLASHES." },
        { "EFFICIENCY", 2, "POISON COSTS 5% LESS SPIRIT A POINT." },
        { "INSIGHT", 2, "POISON GATHERS 15% QUICKER A POINT." },
        { "SEEP", 3, "THE SPLASH LEAVES A POOL OF VENOM TWO SECONDS; WHAT WALKS IN IT IS POISONED AGAIN." },
        { "EFFICIENCY II", 2, "POISON COSTS 10% LESS SPIRIT A POINT." },
        { "LINGERING", 2, "THE POOL LASTS HALF AS LONG AGAIN, A POINT." },
        { "PLAGUE PIT", 5, "HOLD TO CHARGE: THE POOL GROWS WITH IT, LASTS SIX SECONDS AND SLOWS WHAT WADES IN." },
        { "INSIGHT II", 2, "POISON GATHERS 20% QUICKER A POINT." },
        { "POISON MASTERY", 5, "POISON HITS A QUARTER HARDER, AND A TENTH OF EVERY OTHER SPELL GOES IN AS POISON." },
        { "CONTAGION", 5, "WHAT DIES POISONED PASSES IT ON TO ALL WITHIN TWO CELLS." },
    },
    {
        { "SPIRIT", 3, "SOUL DRAIN: A BEAM THAT HURTS, AND HEALS YOU FOR SOME OF IT." },
        { "EFFICIENCY", 2, "SPIRIT COSTS 5% LESS SPIRIT A POINT." },
        { "INSIGHT", 2, "SPIRIT GATHERS 15% QUICKER A POINT." },
        { "HARVEST", 3, "WHAT SOUL DRAIN SLAYS LEAVES A SOUL MORE, TWO FROM RANK III." },
        { "EFFICIENCY II", 2, "SPIRIT COSTS 10% LESS SPIRIT A POINT." },
        { "COMMUNION", 2, "SOUL DRAIN HEALS A QUARTER MORE, A POINT." },
        { "SOUL TETHER", 5, "HOLD TO KEEP THE BEAM ON, DRAINING AND SLOWING; IT LEAPS ON WHEN WHAT IT HOLDS DIES." },
        { "INSIGHT II", 2, "SPIRIT GATHERS 20% QUICKER A POINT." },
        { "SPIRIT MASTERY", 5, "SPIRIT HITS A QUARTER HARDER, A TENTH OF IT GOES WITH EVERY OTHER SPELL, AND IT HARMS SPECTRES." },
        { "WISP HOST", 5, "EVERY DRAIN LOOSES THREE WISPS THAT HUNT THE NEAREST ROUND CORNERS." },
    },
    {
        { "PHYSICAL", 3, "THE BONE SPEAR: THROUGH ALL IN A LINE, A BONE A SPEAR, NO SPIRIT." },
        { "EFFICIENCY", 2, "5% A POINT THAT A SPEAR SPENDS NO BONE; FIVE MORE BONES CARRIED." },
        { "INSIGHT", 2, "THE SPEAR GATHERS 15% QUICKER A POINT." },
        { "IMPALE", 3, "THE FIRST A SPEAR STRIKES IS PINNED, AND CAN'T MOVE OR STRIKE, 0.6 S." },
        { "BONE PLATE", 2, "10% MORE ARMOUR A POINT." },
        { "OSSIFIED", 5, "SPEARS ADD HALF YOUR ARMOUR TO THEIR DAMAGE, AND EACH THAT STRIKES GIVES A POINT OF ARMOUR." },
        { "PHALANX", 5, "HOLD TO CHARGE: A FAN OF SPEARS, THREE AND THEN FIVE, FOR ONE BONE." },
        { "INSIGHT II", 2, "THE SPEAR GATHERS 20% QUICKER A POINT." },
        { "PHYSICAL MASTERY", 5, "SPEARS HIT 30% HARDER." },
        { "BALLISTA", 5, "HOLD TO CHARGE A HEAVY SPEAR THAT KNOCKS BACK AND STUNS ALL IT PASSES." },
    },
};
static const int RAMP[6] = { R_FLAME, R_GLOW, R_GOLD, R_MOSS, R_PURPLE, R_BONE };
static const char *const ENAME[6] = { "FIRE", "FROST", "LIGHTNING", "POISON", "SPIRIT", "PHYSICAL" };
static const char *const KIND[10] = { "OPENING", "PASSIVE", "PASSIVE", "ABILITY", "PATH A", "PATH A", "ULTIMATE",
                                      "PATH B", "PATH B", "ULTIMATE" };

/* a line in a state: held, the node chosen, points to spend; part bought, closed paths */
typedef struct { int e, sel, points, rank; unsigned held, part, closed; int cls; const char *warn; } Case;

static void tree_case(const Case *k, float t) {
    UiTree tr;
    memset(&tr, 0, sizeof tr);
    static char title[40];
    snprintf(title, sizeof title, "%s  RANK %d", ENAME[k->e], k->rank);
    tr.title = title; tr.ramp = RAMP[k->e]; tr.elem = k->e; tr.cls = k->cls;
    for (int i = 0; i < 6; i++) { tr.ramps[i] = RAMP[i]; tr.ranks[i] = i == k->e ? k->rank : (i * 7 + k->e * 3) % 5 == 0 ? 0 : 1 + (i * 5 + k->e) % 9; }
    tr.sel_el = k->e; tr.points = k->points; tr.sel = k->sel; tr.t = t; tr.warn = k->warn;
    tr.keys = "1-6  WASD  ENTER BUYS  SPACE UNBINDS";
    static const int UP[10] = { -1, 0, 0, 1, 3, 4, 5, 3, 7, 8 };
    for (int m = 0; m < 10; m++) {
        UiNode *n = &tr.n[m];
        n->name = LINES_[k->e][m].name; n->does = LINES_[k->e][m].does; n->kind = KIND[m];
        n->cost = LINES_[k->e][m].cost < 1 ? 1 : LINES_[k->e][m].cost;
        n->steps = LINES_[k->e][m].cost == 2;                       /* every node bought a point at a time costs two */
        int held = k->held >> m & 1, up = m == 0 ? 1 : m == 3 ? ((k->held >> 1 & 1) || (k->held >> 2 & 1)) : (k->held >> UP[m] & 1);
        if (k->closed >> m & 1) { n->state = SK_CLOSED; n->note = "THE OTHER PATH IS CHOSEN."; }
        else if (held) { n->state = SK_HELD; n->got = n->cost; }
        else if (k->part >> m & 1) { n->state = SK_PART; n->got = 1; }
        else if (!up) { n->state = SK_REACH; n->note = m == 3 ? "NEEDS EFFICIENCY OR INSIGHT." : "NEEDS THE NODE ABOVE IT."; }
        else if (k->points >= n->cost) n->state = SK_READY;
        else { n->state = SK_SHORT; n->note = n->cost - k->points == 1 ? "ONE POINT SHORT." : "NOT POINTS ENOUGH."; }
    }
    ui_tree(&F, &tr);
}

static const Case CASES[6] = {
    { 4, 6, 0, 12, 0x03F, 0, 0x380, 4, 0 },                                   /* Spirit as the owner saw it: Soul Tether chosen */
    { 0, 1, 2, 1, 0x001, 0, 0, 0, 0 },                                        /* Fire at the start: Efficiency to buy */
    { 1, 2, 3, 4, 0x003, 0x004, 0, -1, 0 },                                   /* Frost: Insight half bought */
    { 2, 5, 5, 9, 0x01F, 0, 0, -1, "THE OTHER PATH WILL CLOSE. AGAIN TO TAKE THIS ONE." },
    { 3, 9, 0, 22, 0x38F, 0, 0x070, 3, 0 },                                   /* Poison: path B to its ultimate */
    { 5, 0, 1, 15, 0x07F, 0, 0x380, -1,
      "UNBIND PHYSICAL FOR TWO CURSED COINS: EVERY NODE GOES, AND 34 POINTS COME BACK. AGAIN TO UNBIND." },
};

/* ---- the other panels ---- */

static void faces_panel(void) {                             /* the faces, each way, on the slab and in a well */
    frame_begin(1);
    ui_slab(&F, 4, 4, 312, 192, SLAB_LEDGER, ORN_ROSE, 0.0f);
    ui_plate(&F, 20, 9, 196, 18);
    ui_text16(&F, 30, 10, "THE FACES OF THE STONE", FACE_INSC, 0, TX_CUT, 1);
    ui_rule(&F, 12, 307, 28, 1);
    static const char *const NM[3] = { "BIOS", "INSCR", "BODY" };
    static const char *const SAMPLE = "THE QUICK SEXTON JUMPS OVER 39 GRAVES.";
    int y = 34;
    for (int fc = 0; fc < 3; fc++) {
        ui_text8(&F, 12, y + 2, NM[fc], FACE_BODY, UI_M(9), TX_PLAIN);
        ui_text8(&F, 60, y, "PLAIN", fc, UI_M(14), TX_PLAIN);
        ui_text8(&F, 116, y, "CUT", fc, 0, TX_CUT);
        ui_text8(&F, 156, y, "INLAID", fc, R_GOLD + 12, TX_INLAID);
        ui_text8(&F, 220, y, "INLAID", fc, R_PURPLE + 13, TX_INLAID);
        y += 12;
    }
    ui_well(&F, 10, y, 300, 62);
    for (int fc = 0; fc < 3; fc++) {
        ui_text8(&F, 15, y + 5 + fc * 19, SAMPLE, fc, UI_M(14), TX_INLAID);
        ui_text8(&F, 15, y + 14 + fc * 19, "0123456789 +-%$/:,.!?'()<>=*#&", fc, R_GLOW + 13, TX_INLAID);
    }
    y += 66;
    ui_text16(&F, 14, y, "LEVEL 11 COMPLETE!", FACE_INSC, R_GOLD + 12, TX_INLAID, 1);
    ui_text16(&F, 170, y, "THE DROWNED", FACE_BIOS, R_GOLD + 12, TX_INLAID, 1);
    ui_plate(&F, 10, y + 17, 140, 17);
    ui_text16(&F, 16, y + 18, "CUT, ON A PLATE", FACE_INSC, 0, TX_CUT, 1);
    ui_text16(&F, 170, y + 18, "ABCDEFGHIJKLM", FACE_INSC, UI_M(14), TX_INLAID, 1);
    ui_text16(&F, 170, y + 36, "NOPQRSTUVWXYZ", FACE_INSC, UI_M(14), TX_INLAID, 1);
    ui_text16(&F, 14, y + 36, "0123456789", FACE_INSC, R_FLAME + 12, TX_INLAID, 1);
}

static void kit_panel(float t) {                            /* the stones and the sockets */
    frame_begin(1);
    ui_slab(&F, 4, 4, 150, 112, SLAB_LEDGER, ORN_PAW, t);
    ui_text8(&F, 14, 12, "A LEDGER STONE", FACE_INSC, UI_M(14), TX_INLAID);
    ui_well(&F, 12, 24, 134, 40);
    static const char *const ROW[4] = { "SIGN THE LEDGER", "ASCEND", "OFFER SOULS", "PAT HER" };
    for (int i = 0; i < 4; i++) {
        if (i == 1) {                                      /* the chosen row: lit in its groove, gilt at either end */
            for (int x = 14; x < 144; x++) for (int yy = 0; yy < 9; yy++) gfx_px(&F, x, 34 + yy, UI_W(5 + ((x + yy) & 1)));
            for (int k = 0; k < 3; k++) { gfx_px(&F, 14, 35 + k * 3, R_GOLD + 11); gfx_px(&F, 143, 35 + k * 3, R_GOLD + 11); }
        }
        ui_text8(&F, 20, 26 + i * 9, ROW[i], FACE_BODY, i == 1 ? C_WHITE : UI_M(11), TX_INLAID);
    }
    ui_text8(&F, 118, 44, "$500", FACE_BODY, R_GOLD + 11, TX_INLAID);
    ui_rule(&F, 12, 146, 70, 1);
    ui_wrap(&F, 14, 76, "SHE KEEPS YOUR RUN AS IT STANDS, ON A PAGE OF YOUR CHOOSING.", 16, 9, 4, FACE_BODY, UI_M(13), TX_INLAID);
    ui_slab(&F, 160, 4, 156, 112, SLAB_HEAD, ORN_CANDLE, t);
    ui_text8(&F, 174, 30, "LEAVE THE CRYPT?", FACE_INSC, R_BLOOD + 13, TX_INLAID);
    ui_wrap(&F, 174, 43, "WHO WILL PET THE MERCHANT WHILE YOU ARE GONE?", 16, 9, 3, FACE_BODY, UI_M(13), TX_INLAID);
    ui_well(&F, 168, 72, 140, 36);
    ui_text8(&F, 174, 75, "STAY IN THE DARK", FACE_INSC, C_WHITE, TX_INLAID);
    ui_text8(&F, 202, 86, "SLEEPWALK", FACE_INSC, UI_M(9), TX_INLAID);
    ui_text8(&F, 174, 97, "BACK TO THE CRYP", FACE_BODY, R_GOLD + 9, TX_INLAID);
    ui_slab(&F, 4, 122, 132, 34, SLAB_TABLET, 0, t);
    ui_text8(&F, 12, 128, "ACHIEVEMENT", FACE_INSC, R_GOLD + 11, TX_INLAID);
    ui_text8(&F, 12, 138, "THE OSSUARY", FACE_BODY, UI_M(14), TX_INLAID);
    ui_text8(&F, 12, 147, "LEVEL 1 CLEARED", FACE_BODY, UI_M(9), TX_PLAIN);
    static const char *const SN[6] = { "FAR", "BUY", "FEW", "1/2", "OWN", "SHUT" };
    for (int s = 0; s < 6; s++) {
        int cx = 150 + s * 28;
        ui_socket(&F, cx, 142, 10, IC_FIREWALL, R_FLAME, s, s == 1, 0, t);
        ui_text8(&F, cx - 4 * (int)strlen(SN[s]) + 1, 158, SN[s], FACE_BODY, UI_M(12), TX_INLAID);
    }
    ui_socket(&F, 26, 178, 11, IC_WISPHOST, R_PURPLE, SK_HELD, 1, 1, t);
    ui_socket(&F, 60, 178, 11, IC_SHATTER, R_GLOW, SK_READY, 0, 1, t);
    ui_socket(&F, 94, 178, 11, IC_BALLISTA, R_BONE, SK_CLOSED, 0, 1, t);
    ui_text8(&F, 116, 175, "ULTIMATES, AND GROOVES:", FACE_BODY, UI_M(12), TX_INLAID);
    ui_groove(&F, 116, 188, 200, 188, R_MOSS, 1.0f);
    ui_groove(&F, 210, 188, 300, 188, R_GOLD, 0.5f);
}

static void icons_panel(int inlaid, float t) {               /* the forty-one, each line in its colour */
    frame_begin(1);
    ui_slab(&F, 4, 4, 312, 192, SLAB_LEDGER, ORN_EYE, t);
    ui_text8(&F, 14, 11, inlaid ? "THE ICONS, BOUGHT: INLAID" : "THE ICONS, UNBOUGHT: CUT IN THE STONE", FACE_INSC, UI_M(14), TX_INLAID);
    for (int e = 0; e < 6; e++) {
        int y = 26 + e * 28;
        ui_well(&F, 10, y - 2, 300, 26);
        ui_text8(&F, 14, y + 7, ENAME[e], FACE_BODY, RAMP[e] + (RAMP[e] == R_GOLD ? 12 : 13), TX_INLAID);
        for (int m = 0; m < 10; m++) {
            int ic = UI_NODE_ICON[e][m], x = 96 + m * 21;
            if (inlaid) for (int j = 0; j < 20; j++) for (int i = 0; i < 20; i++) if ((i + j) & 1) gfx_px(&F, x - 2 + i, y + j, (uint8_t)(RAMP[e] + 2));
            ui_icon(&F, x, y + 2, ic, RAMP[e], inlaid ? IM_INLAID : IM_CUT, 1);
        }
    }
}

static void runs_panel(int level) {                         /* the marble as each run leans it, beside the run's stone */
    crawl_art_level(level);
    crawl_art_run(level);
    ui_marble_make(7u + (unsigned)level);
    frame_begin(1);
    ui_slab(&F, 4, 4, 312, 192, SLAB_LEDGER, ORN_CANDLE, 0.0f);
    char b[48];
    snprintf(b, sizeof b, "LEVEL %d", level);
    ui_text16(&F, 14, 10, b, FACE_INSC, UI_M(14), TX_INLAID, 1);
    ui_well(&F, 10, 32, 300, 90);
    for (int i = 0; i < 16; i++) {
        gfx_rect(&F, 16 + i * 18, 38, 16, 16, UI_M(i));
        gfx_rect(&F, 16 + i * 18, 58, 16, 16, UI_W(i));
        gfx_rect(&F, 16 + i * 18, 78, 16, 16, (uint8_t)(R_STONE + i));
    }
    ui_text8(&F, 16, 100, "THE SLAB, THE WELLS, THE RUN'S STONE", FACE_BODY, UI_M(13), TX_INLAID);
    for (int e = 0; e < 6; e++) ui_text8(&F, 16 + (e % 3) * 96, 126 + (e / 3) * 10, ENAME[e], FACE_INSC, RAMP[e] + (RAMP[e] == R_GOLD ? 12 : 13), TX_INLAID);
    ui_wrap(&F, 16, 146, "THE SLAB LEANS A TENTH TOWARDS THE RUN'S STONE, AND IS AS BRIGHT IN THE DEEP AS IN THE CRYPTS.", 36, 9, 4, FACE_BODY, UI_M(14), TX_INLAID);
}

/* ---- the controller (docs/PLAN-b18-1.md section 7): its screen and the look, with today's layout ---- */
enum { JB_USE, JB_DODGE, JB_NEXT, JB_WISPS, JB_GIFT, JB_RUNE, JB_READIED, JB_CAST, JB_SPRINT, JB_NONE, JB_MAP, JB_BAR, JB_BEAST,
       JB_CODEX, NJOB };
static const char *const JOB[NJOB] = { "USE", "DODGE", "NEXT RUNE", "WISPS", "THE GIFT", "THE RUNE", "READIED", "CAST", "SPRINT",
                                       0, "THE MAP", "THE BAR", "BESTIARY", "CODEX" };
static const char *const JOB_LIST[NJOB] = { "USE", "SWIFT DODGE", "NEXT RUNE", "WISPS", "THE GIFT", "THE RUNE", "READIED",
                                            "CAST", "SPRINT", "NOTHING", "THE MAP", "THE BAR", "BESTIARY", "CODEX" };
static const char *const JOB_HOLD[NJOB] = { 0, 0, "RUNES", 0, 0, 0, 0, 0, 0, 0, "RUNES", "SPELLS", 0, 0 };
static const char *const JOB_DOES[NJOB] = {
    "USE: OPEN, TAKE, PULL, RING, OFFER A SOUL.",
    "SWIFT DODGE, THE WAY THE LEFT STICK LEANS.",
    "THE NEXT RUNE OR GIFT READIED. HELD: THE WHEEL OF RUNES AND GIFTS.",
    "THE WISPS' LIST, OR THE ONE ALWAYS CAST; HELD, THE LIST.",
    "THE BOSS'S GIFT READIED, USED.",
    "THE RUNE READIED, RAISED BY HAND: THE MIRROR'S PARRY.",
    "THE RIGHT HAND: THE RUNE OR GIFT READIED, USED.",
    "CAST THE SPELL IN HAND; HELD, CHARGE IT.",
    "THE SPRINT, ON AND OFF; IT ENDS AS THE STICK STOPS LEANING.",
    "NOTHING. TRADED HERE, IT LEAVES THAT BUTTON IDLE.",
    "THE MAP. HELD: THE WHEEL OF RUNES AND GIFTS.",
    "THE STATUS BAR. HELD: THE WHEEL OF SPELLS.",
    "THE BESTIARY.",
    "THE CODEX OF AFFIXATIONS." };
static const int JOB_DEFAULT[PAD_COUNT] = { JB_USE, JB_DODGE, JB_NEXT, JB_WISPS, JB_GIFT, JB_RUNE, -1, -1, JB_SPRINT, JB_NONE,
                                            JB_MAP, JB_BAR, JB_BEAST, JB_CODEX, JB_READIED, JB_CAST };
static void pad_fill(UiPad *u, const int *jobs) {              /* a layout into the drawing's words */
    memset(u, 0, sizeof *u);
    for (int b = 0; b < PAD_COUNT; b++) if (jobs[b] >= 0) { u->job[b] = JOB[jobs[b]]; u->hold[b] = JOB_HOLD[jobs[b]]; }
    u->job[PAD_VIEW] = "PAGES"; u->job[PAD_MENU] = "PAUSE";
    u->fixed = 1 << PAD_VIEW | 1 << PAD_MENU;
    u->sel = u->opt_sel = -1;
    u->title = "THE CONTROLLER";
}
static void pad_panel(int mode, float t) {
    static char about[120];
    static const char *list[NJOB];
    static int on[NJOB];
    UiPad u;
    pad_fill(&u, JOB_DEFAULT);
    u.t = t;
    static const char *const OPT[5] = { "STICKS: SAME", "TURNING: 1.0", "MENUS: LEFT", "RUMBLE: ON", "THE DEFAULTS" };
    if (mode < 4) for (int i = 0; i < 5; i++) u.opt[i] = OPT[i];
    u.keys = "D-PAD CHOOSES  \1A CHANGES  \1B BACK";
    switch (mode) {
    case 0:                                                        /* a button chosen */
        u.sel = PAD_X;
        snprintf(about, sizeof about, "X: %s", JOB_DOES[JB_NEXT]);
        u.about = about;
        break;
    case 1:                                                        /* the jobs open for it: the map chosen, to trade */
        u.sel = PAD_X;
        for (int j = 0; j < NJOB; j++) { list[j] = JOB_LIST[j]; on[j] = -1; for (int b = 0; b < PAD_COUNT; b++) if (JOB_DEFAULT[b] == j) on[j] = b; }
        u.list = list; u.list_on = on; u.nlist = NJOB; u.list_sel = JB_MAP;
        u.about = "X TAKES THE MAP; THE D-PAD'S UP TAKES THE NEXT RUNE.";
        u.keys = "\1A TRADES  \1B BACK";
        break;
    case 2:                                                        /* an option */
        u.opt_sel = 1; u.opt[1] = "TURNING: 1.2";
        u.about = "HOW FAST THE RIGHT STICK TURNS YOU: HALF TO TWICE TODAY'S, BY TENTHS.";
        u.keys = "D-PAD \x1b\x1a CHANGES  \1B BACK";
        break;
    case 3:                                                        /* the defaults, asked twice */
        u.opt_sel = 4;
        u.about = "AGAIN, AND EVERY BUTTON GOES BACK TO ITS FIRST JOB.";
        u.about_col = R_GOLD + 11;
        u.keys = "\1A RESTORES  \1B BACK";
        break;
    default:                                                       /* the look: as bound, nothing to change */
        u.title = "THE CONTROLLER, AS IT IS";
        u.about = "TO CHANGE THEM: MENU, THEN THE CONTROLLER.";
        u.keys = "VIEW+MENU  \1B CLOSES";
        break;
    }
    ui_pad(&F, &u);
}
static int pad_sheet(const char *path) {
    const int S = 2, P = 640, Q = 400, G = 12, H = 30;
    iw = G + 3 * (P + G); ih = 2 * (Q + H + G) + 40;
    img = calloc((size_t)iw * ih, 3);
    for (size_t i = 0; i < (size_t)iw * ih * 3; i++) img[i] = 12;
    label(G, 10, "THE CONTROLLER, FOR APPROVAL (PLAN-B18-1 STEP 7): TODAY'S LAYOUT; EACH PANEL A 320X200 FRAME, DOUBLED");
    static const char *const CL[6] = { "THE SCREEN: X CHOSEN", "A: THE JOBS, TO TRADE", "AN OPTION: THE TURNING",
                                       "THE DEFAULTS, ASKED TWICE", "THE LOOK: VIEW AND MENU", "THE LOOK, IN THE DEEP" };
    for (int i = 0; i < 6; i++) {
        int lv = i == 5 ? 35 : 1, x = G + (i % 3) * (P + G), y = 40 + (i / 3) * (Q + H + G);
        crawl_art_level(lv); crawl_art_run(lv); ui_marble_make(7u + (unsigned)lv);
        frame_begin(1);
        ui_where = CL[i]; ui_check_frame();
        pad_panel(i < 4 ? i : 4, 0.3f + 0.2f * i);
        blit(x, y + H, S);
        label(x, y + 6, CL[i]);
    }
    int ok = write_png(path);
    free(img);
    return ok;
}

static int sheet(const char *path, int level) {
    const int S = 2, P = 640, Q = 400, G = 12, H = 30;
    iw = G + 3 * (P + G); ih = 4 * (Q + H + G) + 40;
    img = calloc((size_t)iw * ih, 3);
    for (size_t i = 0; i < (size_t)iw * ih * 3; i++) img[i] = 12;
    crawl_art_level(level);
    crawl_art_run(level);
    ui_marble_make(7u + (unsigned)level);
    label(G, 10, "THE CARVED STONE, FOR APPROVAL (PLAN-UI STEP 1): EACH PANEL A 320X200 FRAME, DOUBLED");
    static const char *const CL[6] = { "SPIRIT AS SEEN: TETHER CHOSEN", "FIRE AT THE START", "FROST: INSIGHT HALF BOUGHT",
                                       "LIGHTNING: A PATH CLOSING", "POISON: PATH B TO THE END", "PHYSICAL: UNBINDING ASKED" };
    for (int i = 0; i < 6; i++) {
        int x = G + (i % 3) * (P + G), y = 40 + (i / 3) * (Q + H + G);
        frame_begin(1);
        tree_case(&CASES[i], 0.3f + i * 0.2f);
        blit(x, y + H, S);
        label(x, y + 6, CL[i]);
    }
    int y = 40 + 2 * (Q + H + G);
    faces_panel(); blit(G, y + H, S); label(G, y + 6, "THE FACES: BIOS, INSCRIPTION, BODY");
    kit_panel(0.4f); blit(G + P + G, y + H, S); label(G + P + G, y + 6, "STONES, ROWS, SOCKETS, GROOVES");
    icons_panel(0, 0.0f); blit(G + 2 * (P + G), y + H, S); label(G + 2 * (P + G), y + 6, "THE ICONS, CUT");
    y += Q + H + G;
    icons_panel(1, 0.0f); blit(G, y + H, S); label(G, y + 6, "THE ICONS, INLAID");
    runs_panel(level); blit(G + P + G, y + H, S); label(G + P + G, y + 6, "THE MARBLE AT THIS LEVEL");
    { Case k = CASES[0]; k.sel = 3; frame_begin(0); tree_case(&k, 1.0f); blit(G + 2 * (P + G), y + H, S); label(G + 2 * (P + G), y + 6, "SPIRIT: HARVEST, UNDIMMED"); }
    int ok = write_png(path);
    free(img);
    return ok;
}

static int runs_sheet(const char *path) {
    const int S = 2, P = 640, Q = 400, G = 12, H = 30;
    static const int LV[4] = { 1, 15, 25, 38 };
    iw = G + 2 * (P + G); ih = 2 * (Q + H + G) + 40;
    img = calloc((size_t)iw * ih, 3);
    for (size_t i = 0; i < (size_t)iw * ih * 3; i++) img[i] = 12;
    label(G, 10, "THE MARBLE IN EACH RUN: THE CRYPTS, THE CATACOMBS, THE HOLLOWS, THE DEEP");
    for (int i = 0; i < 4; i++) {
        int x = G + (i % 2) * (P + G), y = 40 + (i / 2) * (Q + H + G);
        crawl_art_level(LV[i]); crawl_art_run(LV[i]); ui_marble_make(7u + (unsigned)LV[i]);
        frame_begin(1);
        Case k = CASES[i == 0 ? 0 : i == 1 ? 2 : i == 2 ? 4 : 3];
        tree_case(&k, 0.5f);
        blit(x, y + H, S);
        char b[40]; snprintf(b, sizeof b, "LEVEL %d", LV[i]);
        label(x, y + 6, b);
    }
    int ok = write_png(path);
    free(img);
    return ok;
}

/* the caster's statuses (docs/PLAN-combat.md, section 4.2, for approval): the seven marks on their stones three times size; under
 * them On Fire's edge running out, Chilled's stacks one to three, and the row as the game sets it */
static int status_sheet(const char *path) {
    static const char *const NAME[7] = { "ON FIRE", "CHILLED", "FROZEN", "STUNNED", "POISONED", "UNBOUND", "KNOCKED DOWN" };
    static const int RAMP[7] = { R_FLAME, R_GLOW, R_GLOW, R_GOLD, R_MOSS, R_PURPLE, R_BLOOD };
    const int S = 2, P = 640, Q = 400, G = 12, H = 30;
    iw = G + P + G; ih = 40 + Q + H + G + 7 * 20 + 20;
    img = calloc((size_t)iw * ih, 3);
    for (size_t i = 0; i < (size_t)iw * ih * 3; i++) img[i] = 12;
    crawl_art_level(3); crawl_art_run(3); ui_marble_make(10u);
    label(G, 10, "THE CASTER'S STATUSES (PLAN-COMBAT C2)");
    frame_begin(1);
    ui_ramps(0);
    for (int i = 0; i < 7; i++) ui_status(&F, 6 + i * 44, 12, UI_AFMARK[i], RAMP[i], 1.0f, i == 1 ? 2 : 0, i == 1 ? 3 : 0, 0.0f, 3);
    static const float LEFT[4] = { 1.0f, 0.66f, 0.33f, 0.12f };
    for (int k = 0; k < 4; k++) ui_status(&F, 10 + k * 34, 92, UI_AFMARK[0], R_FLAME, LEFT[k], 0, 0, 0.0f, 2);
    for (int k = 0; k < 3; k++) ui_status(&F, 170 + k * 34, 92, UI_AFMARK[1], R_GLOW, 0.8f, k + 1, 3, 0.0f, 2);
    for (int k = 0; k < 5; k++) {                             /* the row, as the game sets it */
        static const int ROW[5] = { 6, 3, 5, 1, 0 };
        int i = ROW[k];
        ui_status(&F, 124 + k * 15, 160 + (i == 1 ? 0 : 4), UI_AFMARK[i], RAMP[i], 0.9f - 0.15f * (float)k, i == 1 ? 3 : 0, i == 1 ? 3 : 0, 0.0f, 1);
    }
    blit(G, 40 + H, S);
    label(G, 40 + 6, "MARKS X3; RUNNING OUT; STACKS; THE ROW");
    for (int i = 0; i < 7; i++) { char b[40]; snprintf(b, sizeof b, "%d  %s", i + 1, NAME[i]); label(G, 40 + H + Q + G + i * 20, b); }
    int ok = write_png(path);
    free(img);
    return ok;
}

/* every node of every line in the pane, with the longest foot it can have: the fit check */
static int fit_all(void) {
    static const char *const WARN[3] = { 0, "THE OTHER PATH WILL CLOSE. AGAIN TO TAKE THIS ONE.",
                                         "UNBIND LIGHTNING FOR TWO CURSED COINS: EVERY NODE GOES, AND 134 POINTS COME BACK. AGAIN TO UNBIND." };
    ui_overflows = 0;
    for (int e = 0; e < 6; e++) for (int m = 0; m < 10; m++) for (int k = 0; k < 3; k++) {
        Case c = { e, m, 0, 30, k == 0 ? 0x001u : 0x000u, 0, 0, -1, WARN[k] };   /* nothing held: every note at its longest */
        frame_begin(0);
        tree_case(&c, 0.0f);
    }
    return ui_overflows;
}

/* ---- the lessons' pages (docs/PLAN-tutorial.md), for approval: each as the game cuts it over the dimmed crypt ---- */
typedef struct { int lesson, page, cls, alt; const char *label; const char *const *voice; int nvoice; } TutCase;
static void tutor_panel(const TutCase *c, float t) {
    const char *in[TUT_LINES];
    char wr[TUT_LINES][96];
    int n = c->voice ? c->nvoice : tut_lines(c->lesson, c->page, c->cls, c->alt, in);
    if (c->voice) for (int i = 0; i < n; i++) in[i] = c->voice[i];
    n = tut_wrap(in, n, wr, 34);
    UiTutor u;
    memset(&u, 0, sizeof u);
    u.head = tut_head(c->lesson, c->page, c->cls);
    for (int i = 0; i < n; i++) u.line[i] = wr[i];
    u.nline = n; u.page = c->page; u.npage = tut_pages(c->lesson); u.t = t;
    u.keys = c->voice && strstr(c->label, "PAD") ? "D-PAD PAGES  \1A ON" : c->voice ? "SLIDE PAGES  TAP ON" : u.npage < 2 ? "E DONE" : u.page + 1 < u.npage ? "A/D PAGES  E ON" : "A/D PAGES  E DONE";
    frame_begin(1);
    ui_where = c->label; ui_check_frame();
    ui_tutor(&F, &u);
}
static int tutor_sheet(const char *path, const TutCase *cs, int n, const char *title) {
    const int S = 2, P = 640, Q = 400, G = 12, H = 30, cols = 4, rows = (n + cols - 1) / cols;
    iw = G + cols * (P + G); ih = rows * (Q + H + G) + 40;
    img = calloc((size_t)iw * ih, 3);
    for (size_t i = 0; i < (size_t)iw * ih * 3; i++) img[i] = 12;
    label(G, 10, title);
    for (int i = 0; i < n; i++) {
        int x = G + (i % cols) * (P + G), y = 40 + (i / cols) * (Q + H + G);
        tutor_panel(&cs[i], 0.3f + 0.2f * i);
        blit(x, y + H, S);
        label(x, y + 6, cs[i].label);
    }
    int ok = write_png(path);
    free(img);
    return ok;
}
static int tutor_sheets(const char *dir) {
    static const char *const PAD_WALK[4] = { "[L-STICK] WALKS. [R-STICK] TURNS YOU.", "[L-STICK] PRESSED SPRINTS FORWARD, A",
                                             "WHILE: A SLIM BAR OVER THE TABLET", "SAYS HOW LONG, AND FILLS AGAIN AFTER." };
    static const char *const PAD_HEALTH[5] = { "THE TABLET UNDER THE VIEW HOLDS", "YOUR HEALTH, YOUR SOULS, YOUR",
                                               "KEYS AND THE SPELL IN HAND. IT", "SLIDES AWAY WHEN NOTHING",
                                               "CHANGES; [D-PAD DOWN] BRINGS IT BACK." };
    static const char *const TOUCH_WALK[4] = { "[THE WALKING STICK] WALKS. A FINGER", "SLID ON THE RIGHT TURNS YOU. THE",
                                               "STICK PUSHED PAST ITS RING SPRINTS,", "A WHILE: A SLIM BAR SAYS HOW LONG." };
    static const char *const PAD_MAP[5] = { "THE MAP IS YOURS, AND STAYS", "YOURS, THROUGH DEATH AND ALL.",
                                            "[D-PAD UP] UNROLLS IT. YOU WALK, TURN", "AND CAST WITH IT OPEN: THE",
                                            "CRYPT DOES NOT WAIT FOR YOU." };
    static const TutCase L1[] = {
        { TL_WALK, 0, 0, 0, "B1 WALKING", 0, 0 }, { TL_WALK, 1, 0, 0, "B1 DOORS", 0, 0 },
        { TL_WALK, 0, 0, 0, "B1 WALKING, BY A PAD", PAD_WALK, 4 }, { TL_WALK, 0, 0, 0, "B1 WALKING, BY TOUCH", TOUCH_WALK, 4 },
        { TL_FIRE, 0, 0, 0, "B2 FIRE", 0, 0 }, { TL_THROUGH, 0, 0, 0, "B3 HEALTH", 0, 0 },
        { TL_THROUGH, 0, 0, 0, "B3 HEALTH, BY A PAD", PAD_HEALTH, 5 }, { TL_THROUGH, 1, 0, 0, "B3 BURNING", 0, 0 },
        { TL_THROUGH, 2, 0, 0, "B3 MENDING", 0, 0 }, { TL_CLASS, 0, 0, 0, "B4 THE WITCHFINDER", 0, 0 },
        { TL_CLASS, 0, 2, 0, "B4 THE GALVANIST", 0, 0 }, { TL_CLASS, 0, 4, 1, "B4 THE SIN-EATER, UNMENDED", 0, 0 },
        { TL_CLASS, 1, 3, 0, "B4 THE DEAD", 0, 0 }, { TL_SOULS, 0, 3, 0, "B5 SOULS (THE PLAGUE DOCTOR)", 0, 0 },
        { TL_EVERY, 0, 0, 0, "B6 EVERYWHERE", 0, 0 }, { TL_EVERY, 1, 0, 0, "B6 LOCKS", 0, 0 },
        { TL_KEY, 0, 0, 0, "B7 THE GOLD KEY", 0, 0 }, { TL_NOTYET, 0, 0, 0, "B8 NOT YET", 0, 0 },
        { TL_NOTYET, 0, 0, 1, "B8 NOT YET, AGAIN", 0, 0 }, { TL_HOARD, 0, 0, 0, "B9 THE HOARD", 0, 0 },
        { TL_GOLDDOOR, 0, 0, 0, "B10 THE GOLD DOOR", 0, 0 }, { TL_WARES, 0, 0, 0, "B10A HER WARES", 0, 0 },
    };
    static const TutCase L2[] = {
        { TL_L2, 0, 1, 0, "B11 THE ANATOMIST", 0, 0 }, { TL_CRYPT, 0, 3, 0, "B12 THE CRYPT ANSWERS", 0, 0 },
        { TL_GATE, 0, 0, 0, "B13 BEYOND THE GATE", 0, 0 }, { TL_ICE, 0, 0, 0, "B14 THE ICE", 0, 0 },
        { TL_ICE, 0, 0, 1, "B14 THE ICE, AGAIN", 0, 0 }, { TL_MAP, 0, 0, 0, "B16 THE MAP", 0, 0 },
        { TL_MAP, 0, 0, 0, "B16 THE MAP, BY A PAD", PAD_MAP, 5 }, { TL_MAP, 1, 0, 0, "B16 WHAT IT SHOWS", 0, 0 },
        { TL_MAP, 2, 0, 0, "B16 THE WAY ON", 0, 0 }, { TL_WAYON, 0, 0, 0, "B17 THE WAY ON", 0, 0 },
        { TL_CRACKS, 0, 0, 0, "B18 CRACKS", 0, 0 }, { TL_CRIMSON, 0, 0, 0, "B19 A SECOND KEY", 0, 0 },
        { TL_NOMAP, 0, 0, 0, "B20 NOT WITHOUT THE MAP", 0, 0 }, { TL_NOMAP, 0, 0, 1, "B20 AGAIN", 0, 0 },
    };
    char p[512];
    ui_check = 1; ui_overflows = 0;
    crawl_art_level(1); crawl_art_run(1); ui_marble_make(8u);
    snprintf(p, sizeof p, "%s/pages1.png", dir);
    int ok = tutor_sheet(p, L1, (int)(sizeof L1 / sizeof L1[0]), "THE LESSONS, LEVEL 1 AND HER ROOM, FOR APPROVAL (PLAN-TUTORIAL STEP 4)");
    crawl_art_level(2); crawl_art_run(2); ui_marble_make(9u);
    snprintf(p, sizeof p, "%s/pages2.png", dir);
    ok &= tutor_sheet(p, L2, (int)(sizeof L2 / sizeof L2[0]), "THE LESSONS, LEVEL 2, FOR APPROVAL (PLAN-TUTORIAL STEP 4)");
    printf("UI fit (tutor): %d overflows\n", ui_overflows);
    return ok && !ui_overflows;
}

int main(int argc, char **argv) {
    if (argc < 2) { fprintf(stderr, "usage: uisheet <out.png> [level] | uisheet all <dir>\n"); return 2; }
    crawl_art_init();
    ui_face_init();
    if (!strcmp(argv[1], "fit")) { int n = fit_all(); printf("UI fit: %d overflows\n", n); return n ? 1 : 0; }
    if (!strcmp(argv[1], "status")) return status_sheet(argc > 2 ? argv[2] : "status.png") ? 0 : 1;
    if (!strcmp(argv[1], "all")) {
        char p[512];
        const char *d = argc > 2 ? argv[2] : ".";
        snprintf(p, sizeof p, "%s/ui.png", d);
        if (!sheet(p, 1)) return 1;
        snprintf(p, sizeof p, "%s/ui_runs.png", d);
        return runs_sheet(p) ? 0 : 1;
    }
    if (!strcmp(argv[1], "tutor")) return tutor_sheets(argc > 2 ? argv[2] : ".") ? 0 : 1;
    if (!strcmp(argv[1], "pad")) { ui_check = 1; ui_overflows = 0; int ok = pad_sheet(argc > 2 ? argv[2] : "pad.png"); printf("UI fit (pad): %d overflows\n", ui_overflows); return ok && !ui_overflows ? 0 : 1; }
    return sheet(argv[1], argc > 2 ? atoi(argv[2]) : 1) ? 0 : 1;
}
