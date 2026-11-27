/* The side passages' art (docs/PLAN-side.md): their tiles and the caster as a figure, made at startup in CATACOMB's palette.
 * A draft, to see the side passages working; each run's look and the gear worn come later, as sheets first. */
#ifndef SIDE_ART_H
#define SIDE_ART_H

#include <stdint.h>

#define STS 16                      /* a tile, square in pixels (tall on the screen, as mode 13h's pixels are) */
#define SF_W 20                     /* a frame of the figure: the feet at the foot, the middle at SF_W / 2 (half height since 26 November) */
#define SF_H 18

enum {                              /* the tiles, row-major, C_CLEAR where the backdrop shows */
    ST_STONE0, ST_STONE1, ST_STONE2, ST_STONE_TOP,                 /* the stone: three looks, and with dust and moss on top */
    ST_BRICK, ST_BRICK_TOP,
    ST_LEDGE,                                                      /* stood on, jumped up through: a timber on iron corbels */
    ST_SPIKES,
    ST_WATER0, ST_WATER1, ST_WATER2, ST_WATER3, ST_WATER_DEEP,     /* the toxic water: its surface, four frames; under it */
    ST_FALL, ST_FALL_CRACK,                                        /* a falling block, whole and about to go */
    ST_BACK0, ST_BACK1, ST_BACK_NICHE, ST_BACK_SKULLS,             /* the back wall: drawn darker, not solid */
    ST_FAR,                                                        /* the far wall, behind everything, slower */
    ST_PLAT_L, ST_PLAT_M, ST_PLAT_R,                               /* a moving platform: a slab hung on chains */
    ST_DOOR_T, ST_DOOR_B, ST_GOAL_T, ST_GOAL_B,                    /* a doorway two tiles high; the goal's with its gold */
    ST_DOOR_LIT,                                                   /* the light going away inside one, over its dark */
    ST_SWITCH0, ST_SWITCH1,                                        /* a lever off and thrown */
    ST_KEY0, ST_KEY1, ST_KEY2,                                     /* gold, silver, crimson */
    ST_LIFE,                                                       /* a life: a votive lamp */
    ST_TORCH0, ST_TORCH1, ST_TORCH2,                               /* a sconce on the back wall, burning */
    ST_CHAIN,                                                      /* a chain hanging, for the platforms and the dressing */
    ST_HZ_SLAB, ST_HZ_SLAB_OPEN, ST_HZ_COLD,                       /* 2e: the crypt's breath slab, cracked open, and its cold crack, as tiles */
    ST_HZ_GRATE, ST_HZ_SLUICE, ST_HZ_SLUICE_OPEN, ST_HZ_SAINT, ST_HZ_SAINT_LIT,   /* run II's: the bile's grate, the sluice shut and up, the saint */
    /* the builder's kit (27 November, side_kit.c; drafts for the owner's sheet) */
    ST_SLATE, ST_SLATE_TOP,                                        /* wet slate, and with its gloss on top */
    ST_WEB,                                                        /* a cobweb, over the back wall */
    ST_CRACK,                                                      /* cracked masonry */
    ST_STONE_A, ST_STONE_B,                                        /* the bell stones: a gold bell cut in A's, a violet in B's */
    ST_GHOST,                                                      /* ghost stone */
    ST_BELT0, ST_BELT1, ST_BELT2, ST_BELT3,                        /* the bone-mill belt, four frames of its run */
    ST_PRESS,                                                      /* a press's head */
    ST_TOMB_L, ST_TOMB_R,                                          /* a sarcophagus, its two halves */
    ST_SPRING0, ST_SPRING1,                                        /* the bone springboard, and thrown down */
    ST_BELL0, ST_BELL1, ST_BELL2,                                  /* a bell on its bracket: at rest, swung one way, the other */
    ST_CLIMB,                                                      /* a chain to climb: heavier than the platforms' */
    NST
};

enum {                              /* the figure's frames */
    SF_IDLE0, SF_IDLE1,
    SF_WALK0, SF_WALK5 = SF_WALK0 + 5,
    SF_RUN0, SF_RUN5 = SF_RUN0 + 5,
    SF_RISE, SF_APEX, SF_FALL,
    SF_CROUCH, SF_CREEP0, SF_CREEP1,
    SF_CAST0, SF_CAST1,
    SF_HURT,
    SF_DIE0, SF_DIE4 = SF_DIE0 + 4,
    NSF
};

enum { FP_NONE, FP_HOOD, FP_FACE, FP_ROBE, FP_TRIM, FP_SKIN, FP_BOOT, FP_BELT, FP_GLOW, FP_LINE };   /* what each pixel is, for the gear */

extern uint8_t side_tile[NST][STS * STS];                      /* row-major, C_CLEAR clear */
extern uint8_t side_fig[NSF][SF_W * SF_H];                     /* row-major, facing right, C_CLEAR clear */
extern uint8_t side_fig_part[NSF][SF_W * SF_H];                /* FP_* of each pixel */
extern const char *const SIDE_FRAME_NAME[NSF];
void side_art_traps(void);                                     /* the tiles taken from the crypt's walls, as the run looks now */

void side_art_init(void);                                      /* once, after crawl_art_init (it reads the palette's ramps only) */

#endif
