/* The lessons of the first two levels (docs/PLAN-tutorial.md): their words, and the page they are cut on. The words mark
 * each key in brackets, [E], as the keyboard has it; the game puts the pad's button or the stone touch shows in its place
 * before the page is drawn. {CLASS} and {SPELL} are the caster's past and its spell. */
#ifndef CRAWL_TUTOR_H
#define CRAWL_TUTOR_H

#include "core.h"

#define TUT_LINES 8                                /* a page's lines, at most */
#define TUT_PAGES 3                                /* a lesson's pages, at most */
#define TUT_COLS 32                                /* a line's letters, at most, the brackets not counted */

/* the lessons, by beat: level 1's, her room's, then level 2's */
enum { TL_WALK, TL_FIRE, TL_THROUGH, TL_CLASS, TL_SOULS, TL_EVERY, TL_KEY, TL_NOTYET, TL_HOARD, TL_GOLDDOOR,
       TL_WARES,
       TL_L2, TL_CRYPT, TL_GATE, TL_ICE, TL_MAP, TL_WAYON, TL_CRACKS, TL_CRIMSON, TL_NOMAP, NTL };

int tut_pages(int lesson);                         /* how many pages it has */
/* a page's heading and lines, for a past (0 the Witchfinder ... 5 the Headsman, as the elements); alt: its shorter words for a
 * second time (a turning back), or B4's opening for a caster who walked on unmended. The words are the table's, filled */
const char *tut_head(int lesson, int page, int cls);
int tut_lines(int lesson, int page, int cls, int alt, const char **out);
const char *tut_class_name(int cls);               /* THE WITCHFINDER */
const char *tut_spell_name(int cls);               /* FIRE BOLT */

/* the page: a tablet in the run's stone over the dimmed crypt, the heading inscribed, the lines with each key on a cap
 * ([E]; a '\1' before a letter within a cap marks a pad's face button), a pip a page, the keys to go on at its foot */
typedef struct {
    const char *head;
    const char *line[TUT_LINES];
    int nline, page, npage;
    const char *keys;                              /* the foot's hint, as ui_keys draws it */
    float t;
} UiTutor;
void ui_tutor(Frame *f, const UiTutor *p);
int ui_tutor_width(const char *s);                 /* a line's width in pixels, its caps included */
/* lines whose keys were put in another voice (a pad's buttons, touch's stones) can outgrow a line: kept as written while
 * each fits, else set again as one paragraph, a bracketed key never broken; the lines made, at most TUT_LINES */
int tut_wrap(const char *const *in, int n, char out[TUT_LINES][96], int cols);

#endif
