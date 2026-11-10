/* The portraits of the slain on her walls (docs/PLAN-portraits.md): a mock-up's art, not yet in the game. Each is drawn as
 * a wall face's overlay (C_CLEAR where the face's own stone shows), at s pixels a texel: 1 for the walls' own 64 x 64, 2
 * for 128 x 128 (the creature at its full drawing) or a card up close. */
#ifndef CRAWL_PORTRAIT_H
#define CRAWL_PORTRAIT_H

#include "crawl.h"

/* where its candle stands, so the walls aren't all alike: a sconce at its right or left, a pair, a lamp hung over it,
 * a ledge of candles under it, a skull on a bracket with a candle on its crown, a stand of three on the floor */
enum { PL_SCONCE_R, PL_SCONCE_L, PL_PAIR, PL_LAMP, PL_LEDGE, PL_SKULL, PL_STAND, NPLAYOUT };
extern const char *const PL_NAME[NPLAYOUT];

typedef struct {
    int spr[4];                 /* its pictures, as the bestiary has them: towards you walking (two), its side (two) */
    int family;                 /* PF_*, -1 off the ladder */
    int rank;                   /* the rank painted; -1 its silhouette only */
    int frame;                  /* the frame's colour, the highest rank unlocked: RK_*, -1 bare old wood */
    int eye_core, eye_glow;     /* its eyes at the rank painted (palette indices); 0 none */
    const uint8_t *wall, *floor;   /* the run behind it: its material and its floor, 64 x 64 column-major */
    int layout;                 /* PL_* */
    int ranks[6], nranks;       /* the ranks it comes in, for the gems on the frame's foot */
    int unlocked, unpaid;       /* bits by RK_*: the ranks unlocked; of those, the ones whose gold waits this life */
    float veil;                 /* the tapestry: 1 over it, 0.15 drawn back to its side, then fading out; 0 gone */
    float t;                    /* the clock: its walk, the candles */
    float walk0;                /* when its walk began (the bestiary's turn about starts here) */
} PortraitView;

/* the overlay into pix (a byte a pixel, rows of stride), its top left at ox, oy */
void portrait_face(uint8_t *pix, int stride, int w, int h, int ox, int oy, int s, const PortraitView *v);
/* its flames as the last portrait_face drew them, in texels of the face (x, y) and their strength: at most four; how many */
int portrait_flames(const PortraitView *v, float out[4][3]);
/* the frame's rectangle on the face, in texels */
void portrait_frame_rect(int layout, int r[4]);

#endif
