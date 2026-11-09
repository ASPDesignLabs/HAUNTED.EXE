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

#endif
