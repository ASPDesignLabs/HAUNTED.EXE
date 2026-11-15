/* Stamps (docs/PLAN-stamps.md): small drawn maps of cells, kept as text, that the generator lays down whole instead of carving
 * rectangles: a secret's room, a joiner where halls meet, a place built for a purpose. This is the interface between the two
 * tracks that build it: track A writes the parser behind it (crawl_stamp.c), track B writes the stamps' data
 * (crawl_stamp_data.c) and the sheet tool. Pure data and arithmetic, no game state: nothing here includes core.h or reads G, and
 * the caller brings its own random numbers, so tools/stampsheet.c links it without the game.
 *
 * One glyph is one cell. Rows are joined with '\n' in StampDef.rows; every row is exactly w glyphs, there are h rows.
 *
 *   #  stone                       .  floor
 *   P  a port that must connect    p  a port that may be walled off: a floor cell on the stamp's edge, at a block centre
 *   E  a secret's entrance         D  a secret's doorway on into the pockets: a stone cell outside the room, touching floor
 *   a b c s t   floor that asks for an altar, a brazier, loot, a spawn group, a torch on its nearest wall (provisional)
 *   0-9 floor with a number the stamp's own rule may use (a plate, a trial's door); the game ignores them at first
 *
 * Directions are the game's (DX/DY): 0 east, 1 south, 2 west, 3 north; x grows east and y south, row 0 is the top. */
#ifndef CRAWL_STAMP_H
#define CRAWL_STAMP_H

#include <stddef.h>
#include <stdint.h>

enum { ST_SECRET, ST_JOINER, ST_PLACE, ST_KINDS };      /* what a stamp is for */

#define STAMP_MAXW 40                                   /* cells a side, at most (the great nave is 35 wide) */
#define STAMP_MAXH 40
#define STAMP_MAXPORTS 16
#define STAMP_MAXENTR 4
#define STAMP_MAXMARKS 48
#define STAMP_NAME 24

/* what track B writes (crawl_stamp_data.c): one of these per drawing, in the order drawn. A joiner's and a place's w and h are 3, 7, 11
 * or 15 (so (w + 1) and (h + 1) divide by 4), which is a whole number of the plan's blocks; a secret's need not be. */
typedef struct {
    const char *name;                                   /* "secret_c", "joiner_plus"; unique, at most STAMP_NAME - 1 */
    uint8_t kind;                                       /* ST_* */
    uint8_t w, h;                                       /* in cells */
    uint8_t minstep, maxstep;                           /* the run's levels it may come on, 0..9 (crawl_step) */
    uint8_t weight;                                     /* 1..9, against the others of its kind */
    uint8_t runs;                                       /* a bit per run, I (1) to IV (8); 0 means all four */
    const char *rows;                                   /* h rows of w glyphs, each ended with '\n' but the last */
} StampDef;
extern const StampDef STAMP_DEFS[];                     /* crawl_stamp_data.c */
extern const int NSTAMP_DEFS;

/* per-cell flags in Stamp.fl */
enum { SF_FLOOR = 1,                                    /* walkable: '.', a port, a mark or a number */
       SF_LANE = 2,                                     /* floor with x or y = 2 (mod 4) from the stamp's origin: the hall lane */
       SF_PORT = 4,                                     /* on a P or p */
       SF_ENTRY = 8,                                    /* an E */
       SF_DOOR = 16 };                                  /* a D */

typedef struct { int8_t x, y; uint8_t d; uint8_t must; } StampPort;   /* d: the way out of the stamp; must: a 'P' */
typedef struct { int8_t x, y; char glyph; } StampMark;               /* a, b, c, s, t or a digit */
typedef struct { int8_t x, y, d; } StampSpot;                        /* an entrance or a doorway; d: the way out of the room into it */

/* a stamp as laid: the glyphs as drawn, what was found in them, and where it came from */
typedef struct {
    char name[STAMP_NAME];
    uint8_t kind, w, h;
    uint8_t minstep, maxstep, weight, runs;
    int16_t def;                                        /* its index in STAMP_DEFS */
    uint8_t xf;                                         /* 0..7: bit 0 mirror in x (applied first), bits 1-2 quarter turns clockwise */
    uint8_t norient;                                    /* set by stamp_set: how many distinct orientations this stamp has (1..8) */
    char g[STAMP_MAXH][STAMP_MAXW];                     /* the glyphs, g[y][x] */
    uint8_t fl[STAMP_MAXH][STAMP_MAXW];                 /* SF_* */
    StampPort port[STAMP_MAXPORTS]; int nport;
    StampSpot entry[STAMP_MAXENTR]; int nentry;         /* alternatives: one is chosen per level */
    StampSpot door[STAMP_MAXENTR]; int ndoor;
    StampMark mark[STAMP_MAXMARKS]; int nmark;
    int nfloor;                                         /* the floor cells */
} Stamp;

/* Reads a definition into *out as drawn (xf 0). Returns 0, or -1 with a line in why (may be NULL) saying what is wrong with its
 * shape: a row of the wrong length, an unknown glyph, more than the limits above. It does not judge the layout: see stamp_valid. */
int stamp_parse(const StampDef *def, int index, Stamp *out, char *why, size_t whylen);

/* *out is *in turned and mirrored by t (0..7, as Stamp.xf), glyphs, flags, ports, entrances, doorways and marks with it. t 0 copies.
 * w and h swap for a quarter turn. Never fails for a stamp that parsed. */
void stamp_xform(const Stamp *in, int t, Stamp *out);

/* The rules of PLAN-stamps.md section 4, for any orientation: the floor is one connected region; every port is on the stamp's edge at
 * a block centre (its x or y is 1 mod 4) and faces out; a 2-wide arm includes its block's middle cell; every E and D is stone, outside
 * the room, touching floor and not on a port; a joiner or place is on the lattice; a secret has an entrance and at least one floor
 * cell; a joiner has at least two ports. Returns 0 for a good stamp, else how many things were wrong, the first described in why. */
int stamp_valid(const Stamp *s, char *why, size_t whylen);

/* 1 if a and b have the same glyphs (as a symmetric stamp's turns do), so a set drops the duplicate; 0 if not. */
int stamp_same(const Stamp *a, const Stamp *b);

/* Every stamp of a kind (ST_*, or -1 for all), in the eight orientations, duplicates dropped, into out[0..max): the count, or -1 if
 * max was too small. Stamps that fail stamp_parse or stamp_valid are left out; the sheet tool's `check` is where they are reported. */
int stamp_set(int kind, Stamp *out, int max);

/* How heavily one orientation counts on a level: 0 if step (0..9) or run (0..3) is outside the stamp's limits, else
 * weight * 840 / norient (840 divides by 1 to 8), so all of a stamp's orientations together weigh the same whether it is symmetric or
 * not. Needs norient, so it is for stamps from stamp_set. The caller picks by these with its own random number. */
int stamp_weight(const Stamp *s, int step, int run);

#endif
