/* The lessons of the first two levels (docs/PLAN-tutorial.md section 9): their words, and the page. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "crawl_tutor.h"
#include "crawl_ui.h"
#include "gfx.h"

typedef struct { const char *head; const char *l[TUT_LINES]; } TutPage;
typedef struct { int n; TutPage p[TUT_PAGES]; const char *again; } TutLesson;   /* again: the words for a second time */

static const TutLesson TUT[NTL] = {
    [TL_WALK] = { 2, { { "WALKING", { "[WASD] WALKS. [THE MOUSE] TURNS YOU.", "[SHIFT] SPRINTS FORWARD, A WHILE:",
                                      "A SLIM BAR OVER THE TABLET SAYS", "HOW LONG, AND FILLS AGAIN AFTER." } },
                       { "DOORS", { "[E] OPENS DOORS, TAKES WHAT LIES", "ABOUT AND USES WHAT CAN BE USED.",
                                    "THIS PASSAGE ENDS IN A DOOR ON", "YOUR RIGHT. OPEN IT." } } }, 0 },
    [TL_FIRE] = { 1, { { "FIRE", { "THE ORDER'S LAMP OIL HAS SPILLED", "AND CAUGHT. THERE IS NO WAY",
                                   "ROUND. GO THROUGH IT SPRINTING:", "THE LESS TIME YOU GIVE IT, THE",
                                   "LESS IT TAKES. IT WILL HURT.", "IT WILL NOT KILL YOU." } } }, 0 },
    [TL_THROUGH] = { 3, { { "HEALTH", { "THE TABLET UNDER THE VIEW HOLDS", "YOUR HEALTH, YOUR SOULS, YOUR",
                                        "KEYS AND THE SPELL IN HAND. IT", "SLIDES AWAY WHEN NOTHING",
                                        "CHANGES; [H] BRINGS IT BACK." } },
                          { "BURNING", { "THE FLAME ON THE TABLET'S EDGE", "SAYS YOU ARE STILL ALIGHT. IT",
                                         "BURNS ON A FEW SECONDS MORE.", "COLD, POISON AND A BLOW TO THE",
                                         "HEAD LEAVE MARKS OF THEIR OWN." } },
                          { "MENDING", { "THE WISP KNOWS A PLACE TO MEND.", "FOLLOW IT, FACE THE NICHE AND",
                                         "PRESS [E] TO KNEEL. STAND STILL", "AND IT MENDS YOU, TO THREE",
                                         "QUARTERS. THE DEAD HEAR IT.", "BE READY." } } }, 0 },
    [TL_CLASS] = { 2, { { "{CLASS}", { 0 } },                      /* filled by the past: tut_lines */
                        { "THE DEAD", { "THEY THROW WHAT THEY ARE MADE", "OF. STEP ASIDE FROM WHAT COMES,",
                                        "AND KEEP THEM IN FRONT OF YOU. A", "BLOW WHILE YOU CAST CAN BREAK",
                                        "THE SPELL." } } }, 0 },
    [TL_SOULS] = { 1, { { "SOULS", { "WHAT YOU SEND BACK LEAVES ITS", "SOUL WHERE IT FELL, A FEW",
                                     "SECONDS: WALK THROUGH IT. SOULS", "PAY FOR STRONGER SPELLS, FOR",
                                     "WISPS ([L], A SOUL EACH) AND FOR", "MUCH IN HER HALL. URNS HOLD THEM",
                                     "TOO. YOUR {SPELL}, AS IT IS", "NOW, COSTS NOTHING." } } }, 0 },
    [TL_EVERY] = { 2, { { "EVERYWHERE", { "THE WAY ON IS SELDOM THE ONLY", "WAY. HALLS BRANCH, AND ROOMS",
                                          "KEEP GOLD; GOLD BUYS HER WARES", "BETWEEN LEVELS. DEEPER DOWN,",
                                          "CRACKED STONE GIVES TO [E] AND", "OPENS ON SECRETS." } },
                        { "LOCKS", { "A DOOR BANDED IN GOLD, SILVER", "OR CRIMSON IS LOCKED. ITS KEY",
                                     "LIES SOMEWHERE ON THE SAME", "LEVEL. FIND IT, AND REMEMBER",
                                     "WHERE THE DOOR WAS." } } }, 0 },
    [TL_KEY] = { 1, { { "THE GOLD KEY", { "DO YOU REMEMBER A DOOR BANDED", "IN GOLD? IT WILL OPEN FOR YOU",
                                          "NOW. WHAT IS LOCKED AWAY IS", "WORTH THE WALK BACK." } } }, 0 },
    [TL_NOTYET] = { 1, { { "NOT YET", { "NOT YET. THERE IS A KEY ON THIS", "LEVEL, AND IT IS NEAR: BEHIND A",
                                        "DOOR YOU HAVE NOT OPENED." } } }, "THE KEY IS STILL OUT THERE." },
    [TL_HOARD] = { 1, { { "THE HOARD, AND THE WAY OUT", { "WHAT WAS LOCKED AWAY IS YOURS.", "THE WAY OUT IS EAST, PAST THE",
                                                          "LAST DOOR: RUNES IN THE WALL.", "FACE THEM AND PRESS [E]. WHAT YOU",
                                                          "LEAVE BEHIND IS SHOWN YOU", "BEFORE YOU GO." } } }, 0 },
    [TL_GOLDDOOR] = { 1, { { "THE GOLD DOOR", { "YOU HOLD THE GOLD KEY AND HAVE", "NOT USED IT. GO BACK: THE DOOR",
                                                "IT OPENS IS WEST OF HERE." } } }, "THE GOLD DOOR IS STILL SHUT." },
    [TL_WARES] = { 1, { { "HER WARES", { "GOLD BUYS WHAT STANDS ON HER", "PEDESTALS. FACE ONE AND PRESS [E]",
                                         "TO SEE WHAT IT WOULD CHANGE.", "THE WAY ON IS THE FAR DOOR." } } }, 0 },
    [TL_L2] = { 1, { { "{CLASS}", { "THE DEAD ARE MORE HERE, AND THE", "WAY IS LESS PLAIN. LOOK IN EVERY",
                                    "ROOM: SOMEWHERE ON THIS LEVEL", "LIES A MAP, AND YOU WILL NOT",
                                    "LEAVE WITHOUT IT." } } }, 0 },
    [TL_CRYPT] = { 1, { { "THE CRYPT ANSWERS", { "SOME THINGS HERE WANT A SPELL.", "A SEAL SHOWS ON THEM, AND ITS",
                                                 "PIPS SAY HOW MANY STRIKES. TRY", "YOUR {SPELL} ON ONE AND SEE",
                                                 "WHAT IT GIVES." } } }, 0 },
    [TL_GATE] = { 1, { { "BEYOND THE GATE", { "THE MAP LIES BEYOND THIS GATE,", "AND IT OPENS ONLY FROM THE FAR",
                                              "SIDE. THE WAY IN IS FROM THE", "SOUTH, ACROSS THE ICE." } } }, 0 },
    [TL_ICE] = { 1, { { "THE ICE", { "THE ICE FREEZES WHAT STANDS ON", "IT. IN THE FAR CORNER OF THE",
                                     "ROOM BEYOND IT IS A SWITCH:", "STRIKE IT THREE TIMES FROM HERE",
                                     "AND THE ICE GOES DOWN." } } }, "STRIKE THE SWITCH FIRST." },
    [TL_MAP] = { 3, { { "THE MAP", { "THE MAP IS YOURS, AND STAYS", "YOURS, THROUGH DEATH AND ALL.",
                                     "[M] UNROLLS IT. YOU WALK, TURN", "AND CAST WITH IT OPEN: THE",
                                     "CRYPT DOES NOT WAIT FOR YOU." } },
                      { "WHAT IT SHOWS", { "IT DRAWS WHAT YOU SEE, AND", "DARKENS WHERE YOU HAVE WALKED.",
                                           "IT MOVES WITH THE [ARROWS], AND", "COMES BACK TO YOU AFTER. AT ITS",
                                           "FOOT: THE DEAD, THE GOLD AND THE", "SECRETS LEFT ON THE LEVEL." } },
                      { "THE WAY ON", { "THE ICE HAS CLOSED BEHIND YOU.", "THE GATE TO THE NORTH OPENS",
                                        "FROM THIS SIDE: STRIKE ITS SEAL." } } }, 0 },
    [TL_WAYON] = { 1, { { "THE WAY ON", { "THE ICE HAS CLOSED. THE WAY ON", "IS THE GATE TO THE NORTH." } } }, 0 },
    [TL_CRACKS] = { 1, { { "CRACKS", { "LOOK AT THE WALL BEFORE YOU.", "CRACKS IN THE STONE MEAN A",
                                       "SECRET: WALK UP TO IT AND PRESS", "[E], AND IT GIVES. THEY SHOW ONLY",
                                       "WHEN YOU ARE CLOSE." } } }, 0 },
    [TL_CRIMSON] = { 1, { { "A SECOND KEY", { "A CRIMSON KEY. A KEY IS SPENT", "ON THE DOOR IT OPENS, SO THE",
                                              "GOLD ONE IS GONE. THIS ONE IS", "FOR A DOOR BANDED IN CRIMSON." } } }, 0 },
    [TL_NOMAP] = { 1, { { "NOT WITHOUT THE MAP", { "NOT YET. YOU WILL NOT LEAVE", "WITHOUT THE MAP. IT LIES IN THE",
                                                   "ROOM PAST THE ICE." } } }, "THE MAP IS STILL OUT THERE." },
};

/* the pasts, in the elements' order: their names, their spells, and B4's lines on the spell */
static const char *const CLASS_NAME[6] = { "THE WITCHFINDER", "THE ANATOMIST", "THE GALVANIST", "THE PLAGUE DOCTOR",
                                           "THE SIN-EATER", "THE HEADSMAN" };
static const char *const SPELL_NAME[6] = { "FIRE BOLT", "FROST SHARD", "LIGHTNING", "BLIGHT FLASK", "SOUL DRAIN", "BONE SPEAR" };
static const char *const CLASS_LINES[6][5] = {
    { "YOUR FIRE BOLT FLIES WHERE YOU", "LOOK AND BURSTS ON WHAT IT", "MEETS. NOW AND THEN IT SETS", "THEM ALIGHT." },
    { "YOUR FROST SHARD FLIES WHERE", "YOU LOOK. EACH ONE CHILLS WHAT", "IT STRIKES AND SLOWS IT; FOUR", "AND IT FREEZES." },
    { "YOUR LIGHTNING STRIKES AT ONCE", "WHAT STANDS NEAREST THE MIDDLE", "OF YOUR VIEW. WITH NOTHING", "THERE, IT FIZZLES. NOW AND",
      "THEN IT STUNS." },
    { "YOUR BLIGHT FLASK IS LOBBED", "OVER WHAT IS IN THE WAY TO", "WHAT YOU LOOK AT, AND BURSTS,", "POISONING WHAT IT SPLASHES." },
    { "YOUR SOUL DRAIN STRIKES AT ONCE", "WHAT STANDS NEAREST THE MIDDLE", "OF YOUR VIEW, AND MENDS YOU BY", "SOME OF WHAT IT TAKES, FOR A",
      "LITTLE OF YOUR SOULS." },
    { "YOUR BONE SPEAR FLIES FAST AND", "STRAIGHT, AND TAKES A BONE A", "THROW. WITH NONE LEFT, YOU", "THROW SPLINTERS." },
};

static int cls_ok(int cls) { return cls < 0 || cls > 5 ? 0 : cls; }
const char *tut_class_name(int cls) { return CLASS_NAME[cls_ok(cls)]; }
const char *tut_spell_name(int cls) { return SPELL_NAME[cls_ok(cls)]; }
int tut_pages(int lesson) { return lesson < 0 || lesson >= NTL ? 0 : TUT[lesson].n; }

static const char *fill(char *buf, size_t n, const char *s, int cls) {   /* {CLASS} and {SPELL} put in */
    size_t o = 0;
    while (*s && o + 1 < n) {
        const char *w = !strncmp(s, "{CLASS}", 7) ? tut_class_name(cls) : !strncmp(s, "{SPELL}", 7) ? tut_spell_name(cls) : NULL;
        if (w) { while (*w && o + 1 < n) buf[o++] = *w++; s += 7; continue; }
        buf[o++] = *s++;
    }
    buf[o] = 0;
    return buf;
}
const char *tut_head(int lesson, int page, int cls) {
    static char buf[48];
    if (lesson < 0 || lesson >= NTL || page < 0 || page >= TUT[lesson].n) return "";
    return fill(buf, sizeof buf, TUT[lesson].p[page].head, cls);
}
int tut_lines(int lesson, int page, int cls, int alt, const char **out) {
    static char buf[TUT_LINES][64];
    int n = 0;
    if (lesson < 0 || lesson >= NTL || page < 0 || page >= TUT[lesson].n) return 0;
    cls = cls_ok(cls);
    if (alt && TUT[lesson].again) { out[0] = TUT[lesson].again; return 1; }
    if (lesson == TL_CLASS && page == 0) {                         /* the past's own: who heard you, the spell, how to cast it */
        out[n++] = alt ? "SOMETHING WAITS AHEAD." : "SOMETHING HEARD YOU MEND.";
        for (int i = 0; i < 5 && CLASS_LINES[cls][i]; i++) out[n++] = CLASS_LINES[cls][i];
        out[n++] = "[CLICK] CASTS.";
        return n;
    }
    for (int i = 0; i < TUT_LINES && TUT[lesson].p[page].l[i]; i++) out[n++] = fill(buf[i], sizeof buf[i], TUT[lesson].p[page].l[i], cls);
    return n;
}

int tut_wrap(const char *const *in, int n, char out[TUT_LINES][96], int cols) {
    int fits = 1;
    for (int i = 0; i < n; i++) fits &= ui_tutor_width(in[i]) <= cols * 8;
    if (fits) { for (int i = 0; i < n && i < TUT_LINES; i++) snprintf(out[i], 96, "%s", in[i]); return n < TUT_LINES ? n : TUT_LINES; }
    char para[TUT_LINES * 96];                                     /* one paragraph, then words onto lines */
    size_t o = 0;
    for (int i = 0; i < n; i++) o += (size_t)snprintf(para + o, sizeof para - o, "%s%s", i ? " " : "", in[i]);
    int l = 0, len = 0, w0 = 0;
    out[0][0] = 0;
    for (const char *s = para; *s && l < TUT_LINES; ) {
        while (*s == ' ') s++;
        if (!*s) break;
        const char *e = s;                                         /* a word, or a bracketed key and what clings to it */
        int br = 0;
        while (*e && (br || *e != ' ')) { if (*e == '[') br = 1; else if (*e == ']') br = 0; e++; }
        char word[96];
        snprintf(word, sizeof word, "%.*s", (int)(e - s), s);
        int w = ui_tutor_width(word) / 8;
        if (len && len + 1 + w > cols) { if (++l >= TUT_LINES) break; out[l][0] = 0; len = 0; }
        w0 = (int)strlen(out[l]);
        snprintf(out[l] + w0, (size_t)(96 - w0), "%s%s", len ? " " : "", word);
        len += (len ? 1 : 0) + w;
        s = e;
    }
    return l + (l < TUT_LINES && out[l][0] ? 1 : 0);
}

/* ---- the page ---- */

int ui_tutor_width(const char *s) { int n = 0; for (; *s; s++) n += *s != '[' && *s != ']' && *s != '\1'; return n * 8; }
static void tut_text(Frame *f, int x, int y, const char *s, int col) {   /* the words, each [key] on a cap as ui_keys sets one */
    int cap = 0, mark = 0;
    for (; *s; s++) {
        if (*s == '[') { cap = 1; continue; }
        if (*s == ']') { cap = 0; continue; }
        if (*s == '\1') { mark = 1; continue; }
        if (cap) {
            int first = s[-1] == '[' || (s[-1] == '\1' && s[-2] == '['), last = s[1] == ']';
            for (int yy = -1; yy < 9; yy++)
                for (int xx = first ? -2 : 0; xx < (last ? 9 : 8); xx++) {
                    int edge_t = yy == -1, edge_b = yy == 8, edge_l = first && xx == -2, edge_r = last && xx == 8;
                    gfx_px(f, x + xx, y + yy, edge_t || edge_l ? UI_M(12) : edge_b || edge_r ? UI_M(2) : UI_M(8));
                }
            int c = !mark ? UI_W(1) : *s == 'A' ? R_MOSS + 9 : *s == 'B' ? R_BLOOD + 9 : *s == 'X' ? R_GLOW + 8 : R_GOLD + 6;
            ui_glyph8(f, x, y, (uint8_t)*s, FACE_BODY, c, TX_PLAIN);
        } else ui_glyph8(f, x, y, (uint8_t)*s, FACE_BODY, col, TX_INLAID);
        x += 8; mark = 0;
    }
}
void ui_tutor(Frame *f, const UiTutor *p) {
    int n = p->nline < 0 ? 0 : p->nline > TUT_LINES ? TUT_LINES : p->nline, wide = 0;
    for (int i = 0; i < n; i++) { int w = ui_tutor_width(p->line[i]); if (w > wide) wide = w; }
    int bw = 296, bh = 34 + n * 12 + 24, bx = (320 - bw) / 2, by = (200 - bh) / 2 - 14;
    if (by < 4) by = 4;
    ui_slab(f, bx, by, bw, bh, SLAB_TABLET | SLAB_CALM, ORN_CANDLE, p->t);
    int hw = (int)strlen(p->head) * 8;
    ui_text16(f, (320 - hw) / 2, by + 7, p->head, FACE_INSC, R_GOLD + 12, TX_INLAID, 1);
    ui_rule(f, bx + 24, bx + bw - 24, by + 26, ORN_NONE);
    int lx = (320 - wide) / 2;                                     /* the lines as one block, its left edge straight */
    for (int i = 0; i < n; i++) tut_text(f, lx, by + 33 + i * 12, p->line[i], R_STONE + 13);
    int fy = by + bh - 15;
    if (p->npage > 1)                                              /* a pip a page, this one gold */
        for (int k = 0; k < p->npage; k++) {
            int px = bx + 14 + k * 9;
            for (int yy = 0; yy < 5; yy++)
                for (int xx = 0; xx < 5; xx++) {
                    if (abs(xx - 2) + abs(yy - 2) > 2) continue;   /* a lozenge */
                    gfx_px(f, px + xx, fy + 2 + yy, k == p->page ? (uint8_t)(R_GOLD + 11) : UI_M(xx + yy < 4 ? 4 : 2));
                }
        }
    if (p->keys) ui_keys(f, bx + bw - 12 - ui_keys_width(p->keys), fy, p->keys, R_STONE + 12);
}
