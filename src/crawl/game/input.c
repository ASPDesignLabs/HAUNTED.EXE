/* A part of scene_crawl.c (docs/PLAN-tidy.md): it is #included there in its place and never compiled on its own.
 * IN and the pad (PADX), touch's and the wheels' state, the pad's layout (PL) and its saving, rumble, the hints in the pad's words (pk). */

static Input IN;                            /* their keys and mouse, this step */
static double IN_DT;                        /* and how long the step is */
static struct {                             /* their game controller, if they have one */
    int used;                               /* the pad, not the keys, was touched last: the hints speak its language */
    int fresh;                              /* just taken over: what is held now counts once it is let go */
    uint32_t held;                          /* those buttons */
    float rep[4];                           /* a direction held in a menu: when it repeats (up, down, left, right) */
    uint8_t on[4];
    int menu;                               /* a menu was open last step */
    int run;                                /* the sprint's button pressed: sprinting until pressed again, the stick
                                               stops leaning forward or the meter runs dry */
    int dodge;                              /* B: Swift Dodge, at dodge_a */
    float dodge_a;
    int look;                               /* the look (View and Menu together, 1.0.0b18.1): the controller as it is */
    float look_t, vm_t;                     /* how long it has been up; since View or Menu went down */
    int vm_p, vm_s;                         /* View (1) and Menu (2) down and waiting for their letting go; spent on the look */
    float plx, ply, prx, pry;               /* the sticks as they are, before any swap */
    int w_hand, q_hand;                     /* the hand holding a wheel open, from the D-pad's jobs and from X's: 1 left, 2 right,
                                               0 a shoulder; the wheel is pointed with the other hand's stick */
    float up_t, dn_t; int up_w, dn_w;       /* the D-pad's up and down held, and whether that opened a wheel (1.0.0b16) */
    int wheel_req, wheel_rel, wheel_cancel; /* this step: a wheel to open (WH_*), the one let go, B or Menu */
    int rune_hit, abil_hit;                 /* RB, the rune readied; LB, the boss's gift readied */
} PADX;
/* Touch (docs/PLAN-mobile.md, Part C): the fingers turned into keys and sticks by where they land and what is up, as PADX does
 * for the pad. The stones are laid out by touch_geo; touch_keys reads the fingers; crawl_stage draws the stones. */
enum { TB_STRIP_L, TB_STRIP_R, TB_CORNER_L, TB_CORNER_R, TB_WISPS, TB_MAP, TB_WARD, NTB };
enum { TF_NONE, TF_PEND, TF_WALK, TF_TURN, TF_CAST, TF_BTN, TF_TAP };   /* a finger's part: pending in the left strip (a tap or the
                                                                      walk), the sticks, the cast, a stone, a tap anywhere else */
typedef struct { int x, y, w, h; } TRect;
static struct {
    int fresh, used, wheel;                 /* just taken over: fingers down count once lifted; a finger was last; a wheel asked */
    struct { int id, role, btn, moved, spent, held; float x0, y0, px, py, bx, by, t, sx, sv; } f[TOUCH_MAX];   /* sx, sv: where
                                               the turning has followed it to, and its speed, smoothed */
    int nf;
    float walk_up, rep[4]; uint8_t on[4];   /* since the walking finger last lifted; a menu's direction held and its repeat */
    int dodge_arm, press[NTB];              /* a second touch may dodge; the stones pressed now, to draw */
    int walk; float wbx, wby, wx, wy;       /* the walking stick down, its base and finger, to draw */
} TCH;
/* The wheels (1.0.0b16, docs/PLAN-play.md section 7a): held open, the world goes grey and slow, the stick (or the mouse)
 * points, and letting go takes what it points at. Spells on the D-pad held down; runes and the bosses' gifts on it held up
 * (or Q held), the runes on the right half (RB), the gifts on the left (LB). */
enum { WH_NONE, WH_SPELLS, WH_GIFTS };
enum { WS_SPELL, WS_RUNE, WS_GIFT };
#define WHEEL_HOLD 0.30f                    /* held this long, a wheel opens; let go sooner, it was a tap */
#define WHEEL_SLOW 0.08f                    /* the world's pace while one is open */
#define WHEEL_MAX 16
typedef struct { int kind, id, ok; float ang; } WSlot;
static struct {
    int open, src, n, hi;                   /* the wheel open (WH_*), opened by Q (1) or the pad (0), its slots, the one pointed at */
    float k, t, mx, my, q_t; int q_w;       /* how far up it is (0..1), how long open, the mouse's point, Q held and whether it opened */
    WSlot s[WHEEL_MAX];
} WH;
static void wheel_input(float dt);
static void wheel_close(int take);
/* the pad's sprint button (docs/PLAN-ai.md, SP3): kept as a person's own setting beside the ledger */
static const char *const PAD_WORD[PAD_COUNT] = { "\1A", "\1B", "\1X", "\1Y", "LB", "RB", "VIEW", "MENU", "L-STICK", "R-STICK",
                                                 "D-PAD UP", "D-PAD DOWN", "D-PAD LEFT", "D-PAD RIGHT", "LT", "RT" };
/* The pad's layout (1.0.0b18.1, docs/PLAN-b18-1.md section 7): each button's job, a job named by the button that has it at
 * first (job[b] == b is today's layout), View and Menu never moved; the sticks swapped, no rumble, the turning in tenths.
 * A person's own, in SLOT_PREFS: "HVQ1", a version, the sixteen jobs, a byte of flags (1 swapped, 2 quiet), the turning.
 * 1.0.0b15's single byte (the sprint's button) reads as today's layout with the sprint traded onto that button. */
#define PL_BYTES 23                                                /* the least a layout is; 1.0.0b21.1 writes one more: */
#define PL_BYTES_TITLE 24                                          /* the title screen last shown (docs/PLAN-title.md) */
#define PL_BYTES_TOUCH 26                                          /* and touch's turning and its smoothing (docs/PLAN-mobile.md) */
#define PL_BYTES_TUT 27                                            /* and the lessons, done or asked again (docs/PLAN-tutorial.md) */
enum { TUT_L1 = 1, TUT_L2 = 2, TUT_HER = 4, TUT_AGAIN = 8 };      /* PL.tut: level 1's, level 2's and her room's lessons given; again */
static struct { uint8_t job[PAD_COUNT]; int swap, quiet, turn, mstick, read, title, tturn, tsmooth, tut; } PL;   /* mstick: the menus' stick, 0 left, 1 right, 2 either */
static void pl_default(void) { for (int b = 0; b < PAD_COUNT; b++) PL.job[b] = (uint8_t)b; PL.swap = PL.quiet = PL.mstick = 0; PL.turn = 10; PL.title = 2; PL.tturn = 10; PL.tsmooth = 5; PL.tut = 0; }
static int pl_fixed(int b) { return b == PAD_VIEW || b == PAD_MENU; }
static void pl_trade(int a, int b) { if (a == b || pl_fixed(a) || pl_fixed(b)) return; uint8_t t = PL.job[a]; PL.job[a] = PL.job[b]; PL.job[b] = t; }
static void pl_read(void) {
    if (PL.read) return;
    PL.read = 1; pl_default();
    uint8_t buf[64];
    int n = slot_read(SLOT_PREFS, buf, sizeof buf), ok = n >= PL_BYTES && !memcmp(buf, "HVQ1", 4) && buf[4] >= 1, seen = 0;
    for (int b = 0; b < PAD_COUNT && ok; b++) { ok = buf[5 + b] < PAD_COUNT && !(seen >> buf[5 + b] & 1); if (ok) seen |= 1 << buf[5 + b]; }
    if (ok && buf[5 + PAD_VIEW] == PAD_VIEW && buf[5 + PAD_MENU] == PAD_MENU) {
        memcpy(PL.job, buf + 5, PAD_COUNT); PL.swap = buf[21] & 1; PL.quiet = buf[21] >> 1 & 1; PL.mstick = clampi(buf[21] >> 2 & 3, 0, 2);
        PL.turn = clampi(buf[22], 5, 20);
        if (n >= PL_BYTES_TITLE) PL.title = buf[23] % 3;
        if (n >= PL_BYTES_TOUCH) { PL.tturn = clampi(buf[24], 3, 30); PL.tsmooth = clampi(buf[25], 0, 10); }
        if (n >= PL_BYTES_TUT) PL.tut = buf[26] & 15;
    } else if (n == 1 && buf[0] < PAD_COUNT) pl_trade(PAD_LS, buf[0]);
    if (n < PL_BYTES_TUT) {                                        /* no lessons byte yet (before 1.0.0b21.5): someone who has played here
                                                                      already, with settings of their own or a page in the ledger, is past
                                                                      their first run; HELP gives the lessons back (docs/PLAN-tutorial.md T2 a) */
        static uint8_t pg[SLOT_MAX];
        int played = n > 0;
        for (int k = 0; k <= SLOT_PROGRESS && !played; k++) played = slot_read(k, pg, sizeof pg) > 0;
        if (played) PL.tut = TUT_L1 | TUT_L2 | TUT_HER;
    }
}
static void pl_write(void) {
    uint8_t b[PL_BYTES_TUT];
    memcpy(b, "HVQ1", 4); b[4] = 1;
    memcpy(b + 5, PL.job, PAD_COUNT);
    b[21] = (uint8_t)(PL.swap | PL.quiet << 1 | PL.mstick << 2); b[22] = (uint8_t)PL.turn;
    b[23] = (uint8_t)PL.title; b[24] = (uint8_t)PL.tturn; b[25] = (uint8_t)PL.tsmooth; b[26] = (uint8_t)PL.tut;
    slot_write(SLOT_PREFS, b, PL_BYTES_TUT);
}
static int pl_on(int job) { pl_read(); for (int b = 0; b < PAD_COUNT; b++) if (PL.job[b] == job) return b; return job; }   /* the button a job is on */
static int pad_sprint(void) { return pl_on(PAD_LS); }
static const char *pad_short(int b) { return b >= PAD_UP && b <= PAD_RIGHT ? UI_PAD_NAME[b] : PAD_WORD[b]; }   /* the D-pad as arrows */
static int pad_hand(int b) {                                       /* which thumb a button is under: 1 the left, 2 the right, 0 a finger */
    if ((b >= PAD_UP && b <= PAD_RIGHT) || b == PAD_LS || b == PAD_VIEW) return 1;
    if (b <= PAD_Y || b == PAD_RS || b == PAD_MENU) return 2;
    return 0;
}
static void pad_menu_open(void);                                   /* its screen, from Esc's question, and the look */
static void update_padmenu(void);
static void render_padmenu(Frame *f);
static int pad_look_tick(float dt);
static void pad_look_render(Frame *f);
static int title_enabled(void);                                    /* the title screen (1.0.0b21.1), further down */
static void title_open(void);
static int title_tick(float dt);
static int title_render(Frame *f);
static int title_up(void);
static const char *pad_sprint_hint(void) {
    static char h[48];
    snprintf(h, sizeof h, "%s SPRINTS FORWARD, A WHILE", PAD_WORD[pad_sprint()]);
    return h;
}
#define PAD_TURN 3.0f                       /* radians a second with the right stick all the way over */

/* The hints in the pad's words, when it was touched last: each key a hint names becomes its button, a
 * face button marked (\1) for its colour. Only hints go through here: elsewhere a lone C or E is prose. */
/* the controller's motors, for a person playing with it */
static void rumble(float low, float high, float secs) {
    pl_read();
    if (!G.player || !PADX.used || PL.quiet) return;           /* and not when a person turned it off */
    pad_rumble(low, high, secs);
}
/* The browser's build (docs/PLAN-web.md): Space casts, as the left button does, since Ctrl+W and Ctrl+1-9 are the
 * browser's; E alone uses things in play. The Windows build keeps Ctrl. */
#ifdef HVS_WEB
#define WEB 1
#define KEY_CAST KEY_SPACE
#else
#define WEB 0
#define KEY_CAST KEY_CTRL
#endif
#define CAST_HIT (IN.hit[KEY_MOUSE1] || (WEB && IN.hit[KEY_SPACE]))     /* what spends a press: no cast till it is let go */
#define CAST_UP (!IN.down[KEY_MOUSE1] && !(WEB && IN.down[KEY_SPACE]))
static const char *const PK_KEY[] = { "ENTER", "ESC", "E", "W/S", "WASD", "A/D", "TAB", "SPACE", "Q", "B", "C", "L", "1-9", "CLICK", "M", "RIGHT-CLICK",
                                      "BACK" };
static const char *const PK_PAD[] = { "\1A", "\1B", "\1A", "D-PAD", "D-PAD", "LB/RB", "VIEW", "\1Y", "\1X", "\1B", "\1B", "\1Y", "D-PAD DOWN", "RT", "D-PAD UP", "RB",
                                      "\1X" };   /* BACK: X in a menu (1.0.0b21) */
static const char *pk(const char *s) {
    static char buf[8][240];
    static int at;
    int touch = !PADX.used && TCH.used && G.ui == UI_NONE;           /* by touch, in play: the stones' own words (docs/PLAN-mobile.md) */
    if ((!PADX.used && !touch) || !s) return s;
    char *o = buf[at = (at + 1) & 7];
    const char *s0 = s;
    int n = 0;
    while (*s && n < 230) {
        if (*s == ' ') { o[n++] = *s++; continue; }
        const char *e = s, *a = s, *b;
        while (*e && *e != ' ') e++;
        if (*a == '(') a++;                                        /* the key, less a ( before or a : , . ) after */
        for (b = e; b > a && (b[-1] == ':' || b[-1] == ')' || b[-1] == ',' || b[-1] == '.'); b--) {}
        int key = -1;
        for (int i = 0; i < (int)(sizeof PK_KEY / sizeof PK_KEY[0]); i++)
            if ((int)strlen(PK_KEY[i]) == (int)(b - a) && !strncmp(a, PK_KEY[i], (size_t)(b - a))) key = i;
        if (key == 16 && !(s == s0 || (s - s0 >= 2 && s[-1] == ' ' && s[-2] == ' '))) key = -1;   /* BACK is a key only where a
                                                                      group begins, as in "BACK UNBINDS": not "ESC BACK", "DRAW BACK" */
        if (key < 0) { while (s < e && n < 230) o[n++] = *s++; continue; }
        while (s < a) o[n++] = *s++;
        static const int8_t PK_JOB[17] = { -1, -1, PAD_A, -1, -1, -1, -1, -1, PAD_X, -1, -1, PAD_Y, PAD_DOWN, PAD_RT, PAD_UP, PAD_RB, -1 };
        static const char *const PK_TOUCH[17] = { "USE", "PAUSE", "USE", "WALK", "WALK", "WALK", "WARDROBE", "CAST", "WHEELS", "PAUSE",
                                                  "PAUSE", "WISPS", "WHEELS", "CAST", "MAP", "GIFT", "BACK" };
        const char *pw = PK_PAD[key];                              /* in play, the button that has the key's job now (1.0.0b18.1) */
        if (touch) pw = PK_TOUCH[key];
        else if (G.ui == UI_NONE && PK_JOB[key] >= 0) pw = PAD_WORD[pl_on(PK_JOB[key])];
        for (const char *p = pw; *p && n < 230; p++) o[n++] = *p;
        for (s = b; s < e && n < 230; ) o[n++] = *s++;
    }
    o[n] = 0;
    return o;
}
static const char *pkp(const char *s) {                            /* the same, plain, for text drawn as it is */
    static char buf[4][240];
    static int at;
    const char *t = pk(s);
    if (t == s) return s;
    char *o = buf[at = (at + 1) & 3];
    int n = 0;
    for (; *t && n < 239; t++) if (*t != '\1') o[n++] = *t;
    o[n] = 0;
    return o;
}
static int hint_width(const char *s) {
    int n = 0;
    for (s = pk(s); *s; s++) n += *s != '\1';
    return n * 8;
}
static void hint8(Frame *f, int x, int y, const char *s, int col) {   /* a hint, its buttons in their colours */
    int mark = 0;
    for (s = pk(s); *s; s++) {
        if (*s == '\1') { mark = 1; continue; }
        char c[2] = { *s, 0 };
        int k = !mark ? col : (*s == 'A' ? R_MOSS + 14 : (*s == 'B' ? R_BLOOD + 14 : (*s == 'X' ? R_GLOW + 13 : R_GOLD + 10)));
        gfx_text8(f, x, y, c, k, 1);
        x += 8; mark = 0;
    }
}

