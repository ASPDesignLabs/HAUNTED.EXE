/* The title screen (docs/PLAN-title.md): three scenes that come round in turn, the name in a gothic, bloody hand; after
 * the key, a menu in the middle while the scene goes on; and a screen of the ledger's three pages as graves. */
#ifndef CRAWL_TITLE_H
#define CRAWL_TITLE_H

#include "crawl.h"

typedef struct {
    int used, cls;                  /* a page written; its class, 1 + the element, 0 none */
    char num[4];                    /* its numeral */
    char line[4][16];               /* the words cut in its stone */
} TitlePage;
typedef struct {
    int scene;                      /* 0 A, beneath; 1 B, the walk; 2 C, the risen */
    int pad;                        /* the pad is the input: its words */
    int graves;                     /* the grave screen, not the title */
    float menu, press;              /* the menu come in, 0..1; the key's flare, 1 going to 0 */
    float mk[4], gk[3];             /* how chosen each of the menu's four is, and each grave, 0..1 */
    TitlePage page[3];
    const char *foot;               /* a word in place of the hints, or 0 */
    const char *version;            /* the build, small in a corner of A, or 0 */
} TitleView;
enum { TM_GRAVE, TM_NEW, TM_CONTROLS, TM_HELP, NTM };

float title_loop(int scene);                                       /* how long a scene takes to come round */
void title_draw(Frame *f, float t, const TitleView *v);            /* the whole frame and its palette */
void title_page_set(TitlePage *p, const char *num, int used, int cls, const char *who, const char *l1, const char *l2, const char *l3);
void title_debug_frame(Frame *f, int which, float t);              /* the mock-ups' sheets: 0, 1 the name; 2 the built pieces */

/* Touch's stones (docs/PLAN-mobile.md 6.5), cut with the graves' kit: a phone's margins hold a monument for each control,
 * weathered, broken and grown over, with places cut in it for what the game has to say. Each is built once for its size and
 * state and kept; the game fills its places each frame. */
enum { TSK_USE, TSK_CAST, TSK_CORNER, TSK_TABLET };                /* the left strip, the right, a corner's, a small tablet */
enum { TSM_NONE, TSM_HAND, TSM_WISP, TSM_SCROLL, TSM_ROBE, TSM_ENTER, TSM_ESC, TSM_BACK, TSM_SPACE, NTSM };   /* inlaid in the socket */
typedef struct {
    int kind, w, h, right;          /* right: in the right-hand column, laid out the other way */
    int blank, pressed;             /* blank: nothing to do, its socket empty and its words worn; pressed: gilt */
    int mark;                       /* TSM_*: inlaid in its socket; TSM_NONE leaves the socket for what is in hand */
    unsigned seed;                  /* its own breaks and cracks */
    const char *word, *foot;        /* cut at its head, and on its plinth (a corner's hold), or 0 */
    int foot_mark;                  /* before the foot's word: 0 none, 1 an hourglass (held), 2 a wheel */
} TStone;
typedef struct {
    int sx, sy, sr;                 /* the socket: its middle and radius; sr 0 none */
    int ring;                       /* the groove round it for a time: its radius; 0 none */
    int px, py, pw, ph;             /* the tablet sunk for a word or a number; pw 0 none */
    int gx, gy, gw;                 /* the groove for a fill, three rows from gy; gw 0 none */
} TsPlaces;
typedef struct {
    int sock, icon;                 /* in the socket: 0 nothing, 1 an icon (the kit's), 2 a mark */
    const char *const *mark;
    int ramp, state, sel;           /* the socket's ramp and SK_* state; sel: gilt brackets */
    float ring; int ring_ramp;      /* the ring's time, 0..1 (1 ready); below 0 left dark */
    const char *text; int text_col; /* in the tablet */
    const char *const *glyph; int glyph_ramp;   /* a small mark before the words (5 square), or 0 */
    int pips, maxpips, pip_ramp;    /* pips in the tablet, when there are no words */
    float fill; int fill_ramp;      /* the groove's fill 0..1; below 0 left dark */
} TsInfo;
void tstone_places(const TStone *s, TsPlaces *p);                  /* where its places are, on the stone as built */
int  tstone_word_fits(const TStone *s);                           /* its word fits where it is cut */
void tstone_draw(Frame *f, int x, int y, const TStone *s, const TsInfo *in, float t);   /* the stone, sunk a pixel if pressed, and
                                                                      what fills its places (in may be 0) */
void tstone_ring(Frame *f, float bx, float by, float fx, float fy, int ring, int lit);   /* a stick down: its roundel and knob */
extern const char *const TS_GLYPH_SOUL[5], *const TS_GLYPH_SECRET[5], *const TS_GLYPH_BAG[5];

#endif
