/* Procedural "ASCII art" set pieces for HAUNT.EXE, drawn into an 80x25
 * character grid using only CP437 characters (blocks, shades, lines). */
#ifndef HAUNT_ART_H
#define HAUNT_ART_H

#include "core.h"

#define TCOLS 80
#define TROWS 25
#define ART_ROWS 22                 /* rows 22..24 stay free for captions */

typedef struct { uint8_t ch, fg, bg; } ArtCell;
extern ArtCell art[TROWS][TCOLS];   /* ch == 0: nothing here */

enum { ART_PUMPKIN, ART_SKULL, ART_HOUSE, ART_SPIDER, ART_GHOST, ART_ZOMBIE, ART_SKELETON, ART_COUNT };

/* DAC entries the art relies on (set up by the scene). 0-15 are the standard
 * VGA text colours. */
enum {
    PAL_GHOST0 = 16,                /* 16..23: spectral ramp, dim -> bright */
    PAL_OR_HI = 32, PAL_OR_MID, PAL_OR_LO,
    PAL_CANDLE = 35,                /* flickers */
    PAL_BONE_HI = 40, PAL_BONE_MID, PAL_BONE_LO,
    PAL_EYEGLOW = 43,               /* pulses */
    /* 64..127 belong to whichever full-screen piece is showing (see art_palette) */
};

void art_draw(int kind, double t, float talk);   /* clear + draw one frame (the full-screen pieces fill all 25 rows) */
void art_palette(int kind, uint8_t pal[256][3]); /* DAC entries 64..127 for pieces that need them */
enum {
    ART_EV_LIGHTNING = 1, ART_EV_DOOR = 2, ART_EV_SPIDER = 4, ART_EV_DIRT = 8, ART_EV_GROAN = 16,
    ART_EV_CAW = 32, ART_EV_XYLO = 64, ART_EV_BONES = 128, ART_EV_WAIL = 256, ART_EV_BOO = 512,
    ART_EV_IGNITE = 1024, ART_EV_WIND = 2048, ART_EV_CACKLE = 4096, ART_EV_BAT = 8192, ART_EV_CHATTER = 16384,
    ART_EV_SETTLE = 32768
};
int  art_events(void);                           /* events since the last call */

#endif
