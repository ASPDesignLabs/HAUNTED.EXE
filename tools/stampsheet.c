/* The stamps' sheet (docs/PLAN-stamps.md, track B): every stamp in src/crawl/crawl_stamp_data.c drawn in each of its distinct
 * orientations, for the owner to approve before the game uses any of them, and a check that runs the parser's own rules on all eight.
 *
 *   stampsheet <dir> [secrets|joiners|places|extras|all|check]
 *
 * secrets, joiners and places each write <dir>/stamps_<kind>.png (a kind with no stamps writes nothing); all (the default) writes
 * those and then runs check; check prints each stamp that breaks a rule, and the counts, and exits 1 if any did.
 *
 *   make build/stampsheet
 */
#include "crawl_stamp.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>

extern const unsigned char font_vga16[256][16];

/* ---- the canvas ---- */
typedef struct { uint8_t r, g, b; } Col;
static const Col C_BG = { 34, 34, 38 }, C_STONE = { 78, 78, 86 }, C_WALL = { 66, 66, 74 }, C_FLOOR = { 155, 130, 110 },
                 C_GRID = { 52, 82, 88 }, C_TEXT = { 224, 224, 224 }, C_DIM = { 150, 150, 158 }, C_E = { 255, 0, 255 },
                 C_D = { 0, 230, 70 }, C_P = { 40, 200, 255 }, C_PO = { 120, 170, 200 }, C_BOX = { 110, 110, 120 };
static uint8_t *px; static int cw, ch;

static void dot(int x, int y, Col c) { if (px && x >= 0 && y >= 0 && x < cw && y < ch) { uint8_t *p = px + ((size_t)y * cw + x) * 3; p[0] = c.r; p[1] = c.g; p[2] = c.b; } }
static void rect(int x, int y, int w, int h, Col c) { for (int j = 0; j < h; j++) for (int i = 0; i < w; i++) dot(x + i, y + j, c); }
static void frame(int x, int y, int w, int h, int t, Col c) { rect(x, y, w, t, c); rect(x, y + h - t, w, t, c); rect(x, y, t, h, c); rect(x + w - t, y, t, h, c); }
static void text(int x, int y, const char *s, Col c) {
    for (; *s; s++, x += 8)
        for (int j = 0; j < 16; j++) for (int i = 0; i < 8; i++) if (font_vga16[(unsigned char)*s][j] & (0x80 >> i)) dot(x + i, y + j, c);
}

/* ---- PNG ---- */
static void put32(FILE *f, uint32_t v) { uint8_t b[4] = { v >> 24, v >> 16, v >> 8, v }; fwrite(b, 1, 4, f); }
static void chunk(FILE *f, const char *type, const uint8_t *data, uint32_t len) {
    put32(f, len); fwrite(type, 1, 4, f); if (len) fwrite(data, 1, len, f);
    uint32_t crc = crc32(0, (const Bytef *)type, 4); if (len) crc = crc32(crc, data, len); put32(f, crc);
}
static int write_png(const char *path) {
    size_t rowlen = 1 + (size_t)cw * 3;
    uint8_t *raw = malloc(rowlen * ch);
    for (int y = 0; y < ch; y++) { raw[rowlen * y] = 0; memcpy(raw + rowlen * y + 1, px + (size_t)y * cw * 3, (size_t)cw * 3); }
    uLongf clen = compressBound((uLong)(rowlen * ch));
    uint8_t *comp = malloc(clen);
    compress2(comp, &clen, raw, (uLong)(rowlen * ch), 9);
    FILE *f = fopen(path, "wb");
    if (!f) { free(raw); free(comp); return 0; }
    static const uint8_t sig[8] = { 0x89, 'P', 'N', 'G', 13, 10, 26, 10 };
    fwrite(sig, 1, 8, f);
    uint8_t ihdr[13] = { cw >> 24, cw >> 16, cw >> 8, cw, ch >> 24, ch >> 16, ch >> 8, ch, 8, 2, 0, 0, 0 };
    chunk(f, "IHDR", ihdr, 13); chunk(f, "IDAT", comp, (uint32_t)clen); chunk(f, "IEND", NULL, 0);
    fclose(f); free(raw); free(comp);
    return 1;
}

/* ---- what the owner is told about each stamp: where it came from and what was done to it. Same order and names as the data. ---- */
typedef struct { const char *name, *note; } Note;
static const Note NOTES[] = {
    { "secret_box", "S1, as drawn. Floor 3x3." },
    { "secret_wide", "S2, as drawn. Floor 4x3." },
    { "secret_strip", "S3, as drawn. Floor 7x2." },
    { "secret_l", "S4, as drawn. Floor: a leg of 3 and a foot of 4x2." },
    { "secret_l_turned", "S5, as drawn." },
    { "secret_arrow", "S6, as drawn. Stepped, doorway at the point." },
    { "secret_c", "S7, as drawn. The doorway is in the notch, on the lower arm." },
    { "secret_spiral", "S8, as drawn. Doorway at the far foot." },
    { "secret_j", "S9, as drawn. One entrance, at the top of the long leg." },
    { "secret_long", "S10, as drawn. One entrance; the doorway is on the raised part of the foot." },
    { "joiner_cross_19", "J1, drawn 16x16. Regridded to 19: the stem is 1 wide, the crossbar 5. Ports N and S must connect." },
    { "joiner_cross_11", "J1 at 11: the crossbar is 3." },
    { "joiner_ring_19", "J2, drawn 16x16. Regridded to 19: the core is a cross 9 across with arms 5 wide." },
    { "joiner_ring_11", "J2 at 11: a cross 5 across with arms 3 wide." },
    { "joiner_pillars_19", "J3, drawn 16x16. Regridded to 19: aisles 3 wide, pillars of 7 cells, the pockets by the walls kept." },
    { "joiner_pillars_11", "J3 at 11: aisles 1 wide, pillars of 3 cells." },
    { "joiner_chevron_11", "J4, drawn 12x7. Trimmed to 11 by its centre column. Both feet must connect." },
    { "joiner_transept_19", "J5, drawn 16x16. Regridded to 19; each arm ends 3 wide so its port lies in one block." },
    { "joiner_transept_11", "J5 at 11." },
    { "joiner_star_19", "J6, drawn 16x16. Regridded to 19: arms 3 wide, the core a 5-cell cross." },
    { "joiner_star_11", "J6 at 11: arms 1 wide, the thin star as drawn." },
    { "joiner_tee_19", "J7, drawn 22x7. Regridded to 19x7: the stub is 3 wide. Wingtips must connect; the stub may be walled off." },
    { "joiner_fork_11", "J8, drawn 14x10. Regridded to 11x11. Feet must connect; the apex may be walled off." },
    { "joiner_nave_35", "J9, drawn 35x10, kept 35 wide; one row added (11 tall) so its end ports sit on a block's middle. At most one a level." },
    { "joiner_gallery_15", "J10, drawn 3x16. Trimmed to 15 long. Both ends must connect." },
    { "joiner_ladder_15", "Ours: two halls (ports at the top must connect) joined by two 1-wide rungs." },
    { "joiner_horseshoe_15", "Ours: a bend round solid stone, 3 wide; both feet must connect." },
    { "joiner_hourglass_11", "Ours: two chambers and a 1-wide neck, for a boss, a vault or a trial. Both ends must connect." },
    { "joiner_dogleg_15", "Ours: a 3-wide diagonal joining halls a block-width apart." },
    { "joiner_broken_ring_19", "Ours: a ring road pinched to 1 wide at two corners, four spokes (N and S must connect)." },
    { "joiner_pinwheel_11", "Ours: four blades turning the way in to a core room (N and S must connect)." },
    { "joiner_trident_11", "Ours: three prongs from one wall; the middle prong must connect." },
    { "joiner_octagon_7", "Ours: small octagon with a pillar; the two ports off its axis must connect." },
    { "joiner_octagon_11", "Ours: small octagon with a central pillar (N and S must connect)." },
};
static const char *note_for(const char *name) {
    for (size_t i = 0; i < sizeof NOTES / sizeof NOTES[0]; i++) if (!strcmp(NOTES[i].name, name)) return NOTES[i].note;
    return "";
}

static int FILTER;                                      /* 0: the owner's drawings, 1: ours (their notes begin "Ours") */
static int is_ours(const char *name) { return !strncmp(note_for(name), "Ours", 4); }

/* ---- drawing one stamp ---- */
static Col mark_col(char g) {
    switch (g) {
        case 'a': return (Col){ 170, 90, 230 };         /* altar */
        case 'b': return (Col){ 240, 140, 40 };         /* brazier */
        case 'c': return (Col){ 240, 210, 60 };         /* loot */
        case 's': return (Col){ 220, 60, 60 };          /* spawn */
        case 't': return (Col){ 250, 250, 250 };        /* torch */
        default:  return (Col){ 80, 210, 110 };         /* a number */
    }
}
static int has_floor_nb(const Stamp *s, int x, int y, int *dir) {
    static const int dx[4] = { 1, 0, -1, 0 }, dy[4] = { 0, 1, 0, -1 };
    for (int k = 0; k < 4; k++) {
        int nx = x + dx[k], ny = y + dy[k];
        if (nx >= 0 && ny >= 0 && nx < s->w && ny < s->h && (s->fl[ny][nx] & SF_FLOOR)) { *dir = k; return 1; }
    }
    return 0;
}
static void draw_stamp(const Stamp *s, int ox, int oy, int cs) {
    int lat = s->kind != ST_SECRET;
    rect(ox, oy, s->w * cs, s->h * cs, C_STONE);
    for (int y = 0; y < s->h; y++)
        for (int x = 0; x < s->w; x++) {
            int X = ox + x * cs, Y = oy + y * cs;
            char g = s->g[y][x];
            if (lat && (s->fl[y][x] & SF_FLOOR) == 0 && ((x & 3) == 3 || (y & 3) == 3)) rect(X, Y, cs, cs, C_WALL);   /* the plan's walls between blocks */
            if (s->fl[y][x] & SF_FLOOR) rect(X, Y, cs, cs, C_FLOOR);
            if (g == 'E') rect(X, Y, cs, cs, C_FLOOR);
            if (g == 'a' || g == 'b' || g == 'c' || g == 's' || g == 't' || (g >= '0' && g <= '9')) rect(X + cs / 4, Y + cs / 4, cs - cs / 2, cs - cs / 2, mark_col(g));
        }
    /* gridlines */
    for (int y = 0; y < s->h; y++)
        for (int x = 0; x < s->w; x++) {
            int X = ox + x * cs, Y = oy + y * cs;
            Col gc = (s->fl[y][x] & SF_FLOOR) ? (Col){ 138, 116, 98 } : C_GRID;
            rect(X, Y, cs, 1, gc); rect(X, Y, 1, cs, gc);
        }
    for (int y = 0; y < s->h; y++)
        for (int x = 0; x < s->w; x++) {
            int X = ox + x * cs, Y = oy + y * cs, d;
            char g = s->g[y][x];
            if (g == 'E') frame(X, Y, cs, cs, 2, C_E);
            else if (g == 'D' && has_floor_nb(s, x, y, &d)) {                       /* a green bar on the cell's edge that touches the floor */
                int t = cs > 6 ? 3 : 2;
                if (d == 0) rect(X + cs - t, Y, t, cs, C_D); else if (d == 1) rect(X, Y + cs - t, cs, t, C_D);
                else if (d == 2) rect(X, Y, t, cs, C_D); else rect(X, Y, cs, t, C_D);
                frame(X, Y, cs, cs, 1, C_D);
            } else if (g == 'P') { int m = cs / 3 > 1 ? cs / 3 : 1; rect(X + m, Y + m, cs - 2 * m, cs - 2 * m, C_P); }
            else if (g == 'p') frame(X + 1, Y + 1, cs - 2, cs - 2, 2, C_PO);
        }
    frame(ox - 1, oy - 1, s->w * cs + 2, s->h * cs + 2, 1, C_BOX);
}

/* ---- the sheet ---- */
#define SHW 1800
#define MARGIN 16
#define GAP 18
static const char *KIND_NAME[ST_KINDS] = { "secret rooms", "joiners", "places" };
static const char *kind_title(int kind) { return kind < 0 ? "ours, for approval" : KIND_NAME[kind]; }

static void legend(int y, int kind) {
    int x = MARGIN;
    struct { int k; const char *w; } L[] = { { 0, "floor" }, { 1, "entrance (E)" }, { 2, "doorway (D)" }, { 3, "port, must connect (P)" }, { 4, "port, may be walled off (p)" }, { 5, "wall between blocks" } };
    int nl = kind == ST_SECRET ? 3 : 6;
    if (kind == ST_SECRET) { L[0] = (typeof(L[0])){ 0, "floor" }; L[1] = (typeof(L[0])){ 1, "entrance (E)" }; L[2] = (typeof(L[0])){ 2, "doorway (D)" }; }
    for (int i = 0; i < nl; i++) {
        switch (L[i].k) {
            case 0: rect(x, y, 14, 14, C_FLOOR); break;
            case 1: rect(x, y, 14, 14, C_FLOOR); frame(x, y, 14, 14, 2, C_E); break;
            case 2: rect(x, y, 14, 14, C_STONE); rect(x + 10, y, 4, 14, C_D); break;
            case 3: rect(x, y, 14, 14, C_FLOOR); rect(x + 4, y + 4, 6, 6, C_P); break;
            case 4: rect(x, y, 14, 14, C_FLOOR); frame(x + 1, y + 1, 12, 12, 2, C_PO); break;
            default: rect(x, y, 14, 14, C_WALL); frame(x, y, 14, 14, 1, C_GRID); break;
        }
        text(x + 20, y - 1, L[i].w, C_DIM);
        x += 20 + 8 * (int)strlen(L[i].w) + 24;
    }
}

/* lays the sheet out and, if px is set, draws it; returns the height it needs */
static int sheet(int kind) {
    Stamp all[400];
    int n = stamp_set(kind, all, 400);
    int y = MARGIN;
    char line[256];
    snprintf(line, sizeof line, "STAMPS: %s. Each shape in every orientation it has (a turn or a mirror that looks the same is not shown twice).", kind_title(kind));
    text(MARGIN, y, line, C_TEXT); y += 22;
    legend(y + 2, kind); y += 24;
    text(MARGIN, y, "Nothing here is in the game until the sheet is approved.", C_DIM); y += 28;
    for (int i = 0; i < NSTAMP_DEFS; i++) {
        if ((kind >= 0 && STAMP_DEFS[i].kind != kind) || is_ours(STAMP_DEFS[i].name) != FILTER) continue;
        int first = -1, count = 0;
        for (int k = 0; k < n; k++) if (all[k].def == i) { if (first < 0) first = k; count++; }
        const StampDef *d = &STAMP_DEFS[i];
        snprintf(line, sizeof line, "%s   %dx%d   %s", d->name, d->w, d->h, count ? "" : "(INVALID: see check)");
        text(MARGIN, y, line, C_TEXT);
        snprintf(line, sizeof line, "steps %d-%d, weight %d, %d orientation%s", d->minstep, d->maxstep, d->weight, count, count == 1 ? "" : "s");
        text(SHW - MARGIN - 8 * (int)strlen(line), y, line, C_DIM);
        y += 18;
        text(MARGIN, y, note_for(d->name), C_DIM); y += 22;
        if (!count) { y += 8; continue; }
        int big = d->w > d->h ? d->w : d->h;
        int cs = d->kind == ST_SECRET ? 14 : (big > 20 ? 8 : 12);
        int x = MARGIN, rowh = 0;
        for (int k = first; k < first + count; k++) {
            const Stamp *s = &all[k];
            if (x + s->w * cs > SHW - MARGIN) { x = MARGIN; y += rowh + GAP; rowh = 0; }
            draw_stamp(s, x, y, cs);
            snprintf(line, sizeof line, "t%d", s->xf);
            text(x, y + s->h * cs + 3, line, C_DIM);
            if (s->h * cs + 20 > rowh) rowh = s->h * cs + 20;
            x += s->w * cs + GAP;
        }
        y += rowh + GAP + 6;
    }
    return y + MARGIN;
}

static int write_sheet(const char *dir, int kind) {
    int have = 0;
    for (int i = 0; i < NSTAMP_DEFS; i++) have |= (kind < 0 || STAMP_DEFS[i].kind == kind) && is_ours(STAMP_DEFS[i].name) == FILTER;
    if (!have) return 1;
    px = NULL; cw = SHW; ch = 0;
    int h = sheet(kind);                                    /* measure */
    ch = h; px = malloc((size_t)cw * ch * 3);
    for (int i = 0; i < cw * ch; i++) { px[i * 3] = C_BG.r; px[i * 3 + 1] = C_BG.g; px[i * 3 + 2] = C_BG.b; }
    sheet(kind);                                            /* draw */
    char path[512]; snprintf(path, sizeof path, "%s/stamps_%s.png", dir, kind < 0 ? "extras" : kind == ST_SECRET ? "secrets" : kind == ST_JOINER ? "joiners" : "places");
    int ok = write_png(path);
    free(px); px = NULL;
    if (ok) printf("%s  %dx%d\n", path, cw, ch); else fprintf(stderr, "cannot write %s\n", path);
    return ok;
}

/* ---- the check: every stamp, every orientation, the parser's own rules ---- */
static int check(void) {
    int bad = 0, kinds[ST_KINDS] = { 0 };
    char why[300];
    for (int i = 0; i < NSTAMP_DEFS; i++) {
        const StampDef *d = &STAMP_DEFS[i];
        Stamp base;
        for (int j = 0; j < i; j++) if (!strcmp(STAMP_DEFS[j].name, d->name)) { printf("FAIL %s: the name is used twice\n", d->name); bad++; break; }
        if (stamp_parse(d, i, &base, why, sizeof why)) { printf("FAIL %s\n", why); bad++; continue; }
        int failed = 0;
        for (int t = 0; t < 8 && !failed; t++) {
            Stamp u;
            stamp_xform(&base, t, &u);
            u.xf = (uint8_t)t;
            if (stamp_valid(&u, why, sizeof why)) { printf("FAIL t%d %s\n", t, why); bad++; failed = 1; }
        }
        if (!failed) kinds[d->kind]++;
    }
    static Stamp set[ST_KINDS][400];
    int total = 0;
    printf("%d stamp definitions;", NSTAMP_DEFS);
    for (int k = 0; k < ST_KINDS; k++) {
        int n = stamp_set(k, set[k], 400);
        printf(" %s %d (%d oriented)%s", KIND_NAME[k], kinds[k], n, k + 1 < ST_KINDS ? "," : "");
        if (n > 0) total += n;
    }
    printf("; %d oriented in all. %s\n", total, bad ? "FAILED" : "clean");
    return bad;
}

int main(int argc, char **argv) {
    if (argc < 2) { fprintf(stderr, "usage: stampsheet <dir> [secrets|joiners|places|extras|all|check]\n"); return 2; }
    const char *dir = argv[1], *which = argc > 2 ? argv[2] : "all";
    if (!strcmp(which, "check")) return check() ? 1 : 0;
    int all = !strcmp(which, "all"), ok = 1;
    if (!all && strcmp(which, "secrets") && strcmp(which, "joiners") && strcmp(which, "places") && strcmp(which, "extras")) { fprintf(stderr, "stampsheet: unknown '%s'\n", which); return 2; }
    if (!strcmp(which, "extras") || all) { FILTER = 1; ok &= write_sheet(dir, -1); FILTER = 0; }
    if (all || !strcmp(which, "secrets")) ok &= write_sheet(dir, ST_SECRET);
    if (all || !strcmp(which, "joiners")) ok &= write_sheet(dir, ST_JOINER);
    if (all || !strcmp(which, "places")) ok &= write_sheet(dir, ST_PLACE);
    if (all && check()) return 1;
    return ok ? 0 : 1;
}
