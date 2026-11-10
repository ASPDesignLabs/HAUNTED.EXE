/* The portraits of the slain on her walls (docs/PLAN-portraits.md), the game's and the mock-ups' art. Each is drawn as
 * a wall face's overlay (C_CLEAR where the face's own stone shows), at s pixels a texel: 1 for the walls' own 64 x 64, 2
 * for 128 x 128 (the creature at its full drawing) or a card up close. */
#ifndef CRAWL_PORTRAIT_H
#define CRAWL_PORTRAIT_H

#include "crawl.h"

/* where its candle stands, so the walls aren't all alike: a sconce at its right or left, a pair, a lamp hung over it,
 * a ledge of candles under it, a skull on a bracket with a candle on its crown, a stand of three on the floor */
enum { PL_SCONCE_R, PL_SCONCE_L, PL_PAIR, PL_LAMP, PL_LEDGE, PL_SKULL, PL_STAND, NPLAYOUT };
#define PORTRAIT_FEAT 6         /* a feat's bit in a view's unlocked and unpaid, after the ranks */
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
    int unlocked, unpaid;       /* bits by RK_*: the ranks unlocked; of those, the ones whose gold waits this life; bit
                                   PORTRAIT_FEAT its feat, a medallion under its frame */
    float veil;                 /* the tapestry: 1 over it, 0 drawn back and tied at its side; below 0 none */
    float t;                    /* the clock: its walk, the candles */
    float walk0;                /* when its walk began (the bestiary's turn about starts here) */
} PortraitView;

/* a boss's portrait (step 5): two faces wide, 128 by 64 texels, on the grave plot's far wall by the Accursed Portal. Its gems
 * are its achievements: slain, then each challenge step; the frame takes the highest's colour, bone for
 * slain, then green, blue, red, gold and black for the steps, as a kind's frame takes its ranks' */
typedef struct {
    int spr[5];                 /* walking (two), raising its blow, striking, and a pose of its own */
    int prop[2];                /* its arena behind it: what stands (a pillar), and what lies (on the floor) */
    int prop_n;                 /* what stands, its frames after prop[0] (a brazier's three burning); 0 or 1 still */
    const uint8_t *wall, *floor;   /* its run's stone and floor, 64 x 64 column-major */
    int eye_core, eye_glow;     /* its eyes (palette indices); 0 none */
    int unlocked, unpaid;       /* bits: slain, then challenge steps 1 to 5; of those, the ones whose gold waits */
    float veil;                 /* its tapestries: 1 drawn over it, 0 parted and tied at either side */
    float t, scale;             /* the clock; its size in the picture, 1 its own */
} BossPortraitView;
void portrait_boss_face(uint8_t *pix, int stride, int w, int h, int ox, int oy, int s, const BossPortraitView *v);
/* its stands' six flames at rest, as portrait_flames gives a kind's, in texels of the two faces (128 by 64); how many */
int portrait_boss_flames(float out[6][3]);

/* the overlay into pix (a byte a pixel, rows of stride), its top left at ox, oy */
void portrait_face(uint8_t *pix, int stride, int w, int h, int ox, int oy, int s, const PortraitView *v);
/* its flames as the last portrait_face drew them, in texels of the face (x, y) and their strength: at most four; how many */
int portrait_flames(const PortraitView *v, float out[4][3]);
/* the side its tapestry is gathered to once drawn back, as seen: 0 its left, 1 its right */
int portrait_drape_side(int layout);
/* a layout's flames at rest, as portrait_flames gives them, without a portrait to draw */
int portrait_layout_flames(int layout, float out[4][3]);
/* the frame's rectangle on the face, in texels */
void portrait_frame_rect(int layout, int r[4]);
/* each run's main stone and first floor, made once (at its fifth level, a little worn) and kept: it draws over two of the
 * walls' textures, so the caller draws its own level's art again after it */
void portrait_keep_runs(void);
const uint8_t *portrait_run_wall(int run);
const uint8_t *portrait_run_floor(int run);

#endif
