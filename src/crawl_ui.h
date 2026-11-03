/* The carved stone: CATACOMB's screens cut from one marble (docs/PLAN-ui.md). Everything here draws on a Frame
 * whose R_CEIL and R_FLOOR ramps are the marble's (ui_marble_pal); the game lends them while a menu is up. */
#ifndef CRAWL_UI_H
#define CRAWL_UI_H

#include "core.h"
#include "crawl.h"

extern int ui_mr, ui_wr;                           /* the ramps the stone is cut from: the marble's, or the run's stone */
#define UI_M(k) ((uint8_t)(ui_mr + (k)))           /* the slab's sixteen greys, groove to vein */
#define UI_W(k) ((uint8_t)(ui_wr + (k)))           /* the wells' darker stone */
void ui_ramps(int marble);                         /* 1: R_CEIL and R_FLOOR, lent; 0: R_STONE, for what lies over the live crypt */

void ui_marble_pal(uint8_t pal[256][3], const uint8_t stone_mid[3]);   /* the two ramps, a tenth towards the run's stone */
void ui_marble_make(unsigned seed);                /* the whole screen's marble, once a level */
void ui_lend(Frame *f);                            /* the crypt drawn so far keeps its looks in R_STONE; the marble takes R_FLOOR, R_CEIL */

enum { SLAB_LEDGER, SLAB_HEAD, SLAB_TABLET, SLAB_CALM = 16 };   /* CALM: or'ed in, the inside quieter for text set on it */
enum { ORN_NONE, ORN_CANDLE, ORN_PAW, ORN_BONES, ORN_KEY, ORN_ROSE, ORN_EYE, NORN };
void ui_slab(Frame *f, int x, int y, int w, int h, int kind, int orn, float t);
void ui_well(Frame *f, int x, int y, int w, int h);
void ui_rule(Frame *f, int x0, int x1, int y, int orn);
void ui_orn(Frame *f, int x, int y, int orn, float t);
void ui_plate(Frame *f, int x, int y, int w, int h);
void ui_keys(Frame *f, int x, int y, const char *s, int col);   /* a hint, each group's first word on a cap */
int  ui_keys_width(const char *s);

enum { FACE_BIOS, FACE_INSC, FACE_BODY, NFACE };   /* the 8x8 faces; the 8x16 ones are the BIOS's VGA and FACE_INSC's */
enum { TX_PLAIN, TX_CUT, TX_INLAID };
void ui_face_init(void);
void ui_glyph8(Frame *f, int x, int y, uint8_t ch, int face, int col, int style);
void ui_glyph8x(Frame *f, int x, int y, uint8_t ch, int face, int col, int style, int scale);
void ui_glyph16x(Frame *f, int x, int y, uint8_t ch, int face, int col, int style, int scale);
void ui_text8(Frame *f, int x, int y, const char *s, int face, int col, int style);
void ui_text16(Frame *f, int x, int y, const char *s, int face, int col, int style, int scale);
int  ui_wrap(Frame *f, int x, int y, const char *s, int cols, int lh, int maxl, int face, int col, int style);   /* lines used */
int  ui_wrap_count(const char *s, int cols);
extern int ui_overflows;                           /* the fit checks' count; the debug build prints each, once */
void ui_overflow(const char *where, const char *what, int by);
/* the fit checks on what is drawn (section 2.4): set ui_check, name the screen, and each frame's glyphs are stamped;
 * a glyph's ink over another's, or outside the last stone cut (ui_box), is told */
extern int ui_check;
extern const char *ui_where;
void ui_check_frame(void);                         /* a new frame: nothing stamped yet, no stone */
void ui_box_none(void);

/* the icons, 16 by 16 (section 5.4), in ui_art.c's order; UI_NODE_ICON gives each line's ten */
enum { IC_EFF, IC_INS, IC_EFF2, IC_INS2, IC_FIRE, IC_FIRESTARTER, IC_INTENSITY, IC_FLAMEWALKER, IC_FIREMASTER,
      IC_FIREWALL, IC_FROST, IC_DEEPCHILL, IC_PERMAFROST, IC_LANCE, IC_FROSTMASTER, IC_SHATTER, IC_LIGHTNING,
      IC_IONIZATION, IC_LMASTERA, IC_ARC, IC_LMASTERB, IC_AMPERAGE, IC_POISON, IC_SEEP, IC_LINGERING, IC_PLAGUEPIT,
      IC_PMASTER, IC_CONTAGION, IC_SPIRIT, IC_HARVEST, IC_COMMUNION, IC_TETHER, IC_SMASTER, IC_WISPHOST, IC_SPEAR,
      IC_IMPALE, IC_BONEPLATE, IC_OSSIFIED, IC_PHALANX, IC_PHMASTER, IC_BALLISTA, NICON };
extern const int UI_NODE_ICON[6][10];
const char *ui_icon_row(int icon, int r);
const char *ui_icon_name(int icon);
enum { IM_CUT, IM_DIM, IM_GLOW, IM_HALF, IM_INLAID };
void ui_icon(Frame *f, int x, int y, int icon, int ramp, int mode, int scale);

/* a socket and its states (section 5.3) */
enum { SK_REACH, SK_READY, SK_SHORT, SK_PART, SK_HELD, SK_CLOSED };
void ui_socket(Frame *f, int cx, int cy, int r, int icon, int ramp, int state, int sel, int ult, float t);
/* the same with a mark of the game's own in it (rows of '#' and 'o', 7 or 11 square), twice its size when there is room */
void ui_socket_mark(Frame *f, int cx, int cy, int r, const char *const *mark, int ramp, int state, int sel, int ult, float t);
void ui_mark(Frame *f, int x, int y, const char *const *mark, int ramp, int mode, int scale);
extern const char *const UI_LMARK[11][7];         /* the caster's lines' small marks (ui_art.c), by UI_LM_* */
enum { UI_LM_HEALTH, UI_LM_DAMAGE, UI_LM_RESIST, UI_LM_HASTE, UI_LM_DODGE, UI_LM_TREASURE, UI_LM_MOVE, UI_LM_LUCK,
       UI_LM_ATTUNE, UI_LM_SPRINT, UI_LM_PHYSIC };

/* a screen of the kit's: the slab, a title across the top and a figure at its right, a well on the left and the pane's
 * well from split on (split 0: one well across) */
void ui_screen(Frame *f, const char *title, int title_col, const char *right, int right_col, int split, float t);
/* the pane (section 5.1): the chosen thing large in its socket, its name and kind, a cost line, what it does, a figure of its
 * own, its note at the foot; a warning takes the description's place */
typedef struct {
    int icon;                                      /* an icon, or -1 and a mark */
    const char *const *mark;
    int ramp, state, ult;
    const char *name, *kind, *cost;
    int cost_col;                                  /* 0: gold */
    const char *does;
    const char *extra; int extra_col;              /* a line or two under what it does, or 0 */
    const char *note, *warn;
    int warn_col;                                  /* 0: gold */
} UiPane;
void ui_pane(Frame *f, int x, int y, int w, int h, const UiPane *p, float t);
void ui_pane_wide(Frame *f, int x, int y, int w, int h, const UiPane *p);   /* a band across the stone: no socket, the words wide */
void ui_groove(Frame *f, int x0, int y0, int x1, int y1, int ramp, float fill);

/* the Alignment's stone (section 5): what the game or the sheet knows of a line, laid out */
typedef struct {
    const char *name, *kind, *does;
    int cost, got, state;                          /* got: points in it; state SK_* */
    int steps;                                     /* bought a point at a time: its points as pips under it */
    float grow;                                    /* 0: its groove by its state; else filled this far, 0..1 (a purchase) */
    const char *note;                              /* its state's words at the pane's foot, or 0 */
} UiNode;
typedef struct {
    const char *title;                             /* the line and its rank */
    int ramp, elem, cls;                           /* the element's ramp, which, the class's element or -1 */
    int ranks[6], ramps[6], sel_el, points;        /* the six seals */
    UiNode n[10];
    int sel;                                       /* the chosen node, ND_* order */
    const char *warn;                              /* gold words that take the pane's foot, or 0 */
    const char *keys;                              /* the hint line */
    float t;
} UiTree;
void ui_tree(Frame *f, const UiTree *tr);

#endif
