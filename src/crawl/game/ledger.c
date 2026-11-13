/* A part of scene_crawl.c (docs/PLAN-tidy.md): it is #included there in its place and never compiled on its own.
 * The ledger's pages (signing, opening); the title screen's logic (TI); death's cost; her claim and the mark; the ledger's and the cornerstones' menus. */

/* ---- the merchant's ledger: three pages, signed at her menu and opened from Esc ---- */

typedef struct { int used, level, hp, gold, souls, why, mine, old, cls; int64_t when; } Page;
static Page page_info(int i) {
    static uint8_t buf[SLOT_MAX];
    Page g;
    memset(&g, 0, sizeof g);
    int n = slot_read(i, buf, sizeof buf), v = save_version(buf, n), base = save_end(v), k;
    if (!v) return g;
    g.used = 1; g.level = get32(buf + 8); g.hp = get32(buf + 12); g.gold = get32(buf + 16); g.souls = get32(buf + 20) / (v >= 3 ? 10 : 1);
    const uint8_t *c = page_section(buf, n, LG_PAGE, &k);
    if (c && k >= LG_PAGE_BYTES) {
        uint32_t id = (uint32_t)get32(c);
        g.mine = (id && id == RUN.run_id) || i + 1 == LEDGER.page;
        g.when = (int64_t)((uint64_t)(uint32_t)get32(c + 8) | (uint64_t)(uint32_t)get32(c + 12) << 32); g.why = c[16];
    } else {
        g.mine = i + 1 == LEDGER.page; g.old = 1;
        if (n >= base + 8) g.when = (int64_t)((uint64_t)(uint32_t)get32(buf + base) | (uint64_t)(uint32_t)get32(buf + base + 4) << 32);   /* 1.0.0b6's graves: when carved */
    }
    c = page_section(buf, n, LG_ALIGN, &k);                        /* its class, for the title's graves (1.0.0b21.1) */
    if (c && k >= LG_ALIGN_B11) g.cls = c[NELEM * NNODE + 2] <= NELEM ? c[NELEM * NNODE + 2] : 0;
    return g;
}

static void open_ledger(int mode) {
    G.ui = UI_LEDGER; G.led_mode = mode; G.led_arm = -1; G.led_msg = 0; G.ui_sel = 0;
    int64_t newest = -1;
    int empty = -1;
    for (int i = SLOT_COUNT - 1; i >= 0; i--) {
        Page g = page_info(i);
        if (!g.used) empty = i;
        if (mode && g.used && g.when >= newest) { newest = g.when; G.ui_sel = i; }   /* opening: the newest */
    }
    if (!mode) G.ui_sel = LEDGER.page ? LEDGER.page - 1 : (empty >= 0 ? empty : 0);   /* signing: the page you came from */
    snd_sfx_ex(SFX_KEY, 0.7f, 0.5f);
}

static void ledger_sign(int i) {
    static uint8_t buf[SLOT_MAX];
    G.ui = UI_NONE;
    if (CON.used) { say("THE DEAD TOOK NOTE: SHE SIGNS NOTHING"); snd_sfx_ex(SFX_KNOCK, 0.6f, 0.5f); return; }
    RUN.signs++;
    int n = run_pack(buf, sizeof buf, PG_SIGNED);
    if (n <= 0) { RUN.signs--; return; }
    slot_write(i, buf, n);
    LEDGER.page = i + 1;
    progress_point();
    say("SHE SIGNS PAGE %s OF HER LEDGER", RUN_ROMAN[i]);
    snd_sfx_ex(SFX_KNOCK, 0.8f, 0.5f); snd_sfx_ex(SFX_RUNE, 0.6f, 0.5f);
    sfx_at(SFX_TRILL, CAT_X, CAT_Y, 1.0f); G.cat_trill = 0.5f; G.cat_slow = 1.2f;
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT ledger_sign page %d level %d gold %d run %08x signs %u bytes %d t=%.2f\n", i + 1, G.level, RUN.treasure, RUN.run_id, RUN.signs, n, G.t);
#endif
}

/* the page's run comes back in her room (or, for 1.0.0b6's graves, at the start of its level), still in your hands */
static void ledger_open(int i) {
    static uint8_t buf[SLOT_MAX];
    int n = slot_read(i, buf, sizeof buf), ql = G.quit_line;
    if (!save_version(buf, n)) return;
    uint32_t seed = G.rs * 1664525u + 1013904223u;
    run_unpack(buf, n);
    LEDGER.page = i + 1;
    progress_point();
    LEDGER.claim_due = !CON.used && mark_fits();
    LEDGER.marked = 0;                                             /* the page left behind keeps its own mark */
    crawl_enter(seed);
    G.player = 1; G.status_ret = PLAY_PLAYER; G.ui = UI_NONE; G.quit_line = ql;
    if (G.shop) { G.sdur = 0.0f; G.shop_look = -1; G.pc_t = -1.0f; }   /* no cast left half-made */
    else human_begin();
    say("THE LEDGER OPENS AT PAGE %s: LEVEL %d", RUN_ROMAN[i], G.level);
    snd_sfx_ex(SFX_WHOOSH, 0.8f, 0.5f); snd_sfx_ex(SFX_WHISPER, 0.8f, 0.5f);
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT ledger_open page %d level %d gold %d shop %d t=%.2f\n", i + 1, G.level, RUN.treasure, G.shop, G.t);
#endif
}


/* ---- the title screen (1.0.0b21.1, docs/PLAN-title.md section 7): it comes up when a person takes over, before anything
 * is theirs. Any key or button (not a stick) brings its menu into the middle while the scene goes on and the lament ground
 * plays; a grave rises from the ledger, a new run begins, the controls or help are shown. Left untouched on its prompt it
 * gives the screen back to the dream. The art is src/art/crawl_title.c. ---- */
enum { TS_PROMPT, TS_MENU, TS_GRAVES, TS_CONTROLS, TS_HELP };
#define TITLE_IDLE 90.0f                                           /* untouched this long on its prompt: back to the dream */
#define TITLE_LEVEL 3                                              /* its art is run I's, at level 3, as it was drawn */
static struct {
    int on, st, scene, sel, gsel, pad;
    float t, grace, idle, press, menu, mk[NTM], gk[3], foot_t, sx, sy;
    const char *foot;
    TitlePage page[3];
} TI;
static int title_enabled(void) {
#ifdef CRAWL_DEBUG
    if (getenv("CRAWL_TITLE") && !atoi(getenv("CRAWL_TITLE"))) return 0;   /* testing: H gives the dream's run, as before */
#endif
    return 1;
}
static int title_next_scene(void) {                                /* A, B and C in turn, once each start of the program */
    static int chosen = -1;
    if (chosen < 0) { pl_read(); chosen = (PL.title + 1) % 3; PL.title = chosen; pl_write(); }
#ifdef CRAWL_DEBUG
    if (getenv("CRAWL_TITLESCENE")) return clampi(atoi(getenv("CRAWL_TITLESCENE")), 0, 2);   /* testing: this one */
#endif
    return chosen;
}
static void title_pages(void) {                                    /* the three pages' words, from the ledger */
    static const char *const SETS[4] = { "CRYPTS", "CATACOMBS", "HOLLOWS", "THE DEEP" };
    static const char *const MONTH[12] = { "JAN", "FEB", "MAR", "APR", "MAY", "JUN", "JUL", "AUG", "SEP", "OCT", "NOV", "DEC" };
    for (int i = 0; i < 3; i++) {
        Page g = page_info(i);
        if (!g.used) { title_page_set(&TI.page[i], RUN_ROMAN[i], 0, 0, "UNMARKED", "NOTHING", "WRITTEN", ""); continue; }
        char lv[16], when[16] = "", who[16], who2[16];
        const char *why = g.why == PG_DEATH ? "AT A DEATH" : g.why == PG_CLAIM ? "HER CLAIM" : SETS[crawl_set(imax(1, g.level))];
        snprintf(lv, sizeof lv, "LEVEL %d", g.level);
        time_t w = (time_t)g.when;
        struct tm *tm = g.when > 0 ? localtime(&w) : NULL;
        if (tm) snprintf(when, sizeof when, "%d %s %d", tm->tm_mday, MONTH[tm->tm_mon % 12], tm->tm_year + 1900);
        const char *name = g.cls >= 1 && g.cls <= NELEM ? CLASS_SHORT[g.cls - 1] : "NO PAST YET";
        const char *sp = strlen(name) > 11 ? strchr(name, ' ') : NULL;   /* PLAGUE DOCTOR: on two lines, the date given up */
        if (sp) {
            snprintf(who, sizeof who, "%.*s", (int)(sp - name), name); snprintf(who2, sizeof who2, "%s", sp + 1);
            title_page_set(&TI.page[i], RUN_ROMAN[i], 1, g.cls, who, who2, lv, why);
        } else title_page_set(&TI.page[i], RUN_ROMAN[i], 1, g.cls, name, lv, why, when);
    }
}
static void title_open(void) {
    memset(&TI, 0, sizeof TI);
    TI.on = 1; TI.st = TS_PROMPT; TI.scene = title_next_scene(); TI.grace = 0.5f; TI.pad = PADX.used;
    G.player = 1; G.status_ret = PLAY_PLAYER; G.ui = UI_NONE; G.map_on = 0;
    PADX.fresh = 1; TCH.fresh = 1;
    if (!G.quit_line) G.quit_line = 1 + rnd(97);
    title_pages();
    crawl_art_level(TITLE_LEVEL);
    snd_sfx_ex(SFX_WHOOSH, 0.6f, 0.5f);
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT title open scene %c t=%.2f\n", 'A' + TI.scene, G.t);
#endif
}
static void title_to_dream(void) {                                 /* untouched: the screen back to the dream, as it was */
    TI.on = 0;
    crawl_art_level(G.shop ? 1 : imax(1, G.level));
    if (PK.depth) crawl_art_theme(G.ptheme, G.level);
    G.player = 0; G.status_ret = PLAY_AUTO; G.ui = UI_NONE;
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT title dream t=%.2f\n", G.t);
#endif
}
static void title_to_play(int page) {                              /* a grave risen (page 0-2), or a new run (-1) */
    TI.on = 0;
    LEDGER.yours = 1;                                              /* the dream is over: from now on the run is yours */
    if (page >= 0) ledger_open(page); else run_new();
    show_bar(); G.hint_t = 6.0f; PADX.fresh = 1; TCH.fresh = 1;
    snd_sfx_ex(SFX_RUNE, 1.0f, 0.5f);
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT title %s %d level %d shop %d\n", page >= 0 ? "rise" : "new", page + 1, G.level, G.shop);
#endif
}
static int title_any(void) {                                       /* a key or a button: not a stick, nor the mouse moving */
    for (int b = 0; b < PAD_COUNT; b++) if (IN.pad_hit[b]) { TI.pad = 1; return 1; }
    for (int k = 0; k < KEY_COUNT; k++) if (IN.hit[k]) { TI.pad = 0; return 1; }
    return 0;
}
static void title_nav(int *dx, int *dy) {                          /* the arrows, WASD, the D-pad, the wheel, a stick pushed */
    *dx = *dy = 0;
    if (IN.hit[KEY_W] || IN.hit[KEY_UP] || IN.wheel > 0) *dy = -1;
    if (IN.hit[KEY_S] || IN.hit[KEY_DOWN] || IN.wheel < 0) *dy = 1;
    if (IN.hit[KEY_A] || IN.hit[KEY_LEFT]) *dx = -1;
    if (IN.hit[KEY_D] || IN.hit[KEY_RIGHT]) *dx = 1;
    int pad = IN.pad_hit[PAD_UP] || IN.pad_hit[PAD_DOWN] || IN.pad_hit[PAD_LEFT] || IN.pad_hit[PAD_RIGHT];
    if (IN.pad_hit[PAD_UP]) *dy = -1;
    if (IN.pad_hit[PAD_DOWN]) *dy = 1;
    if (IN.pad_hit[PAD_LEFT]) *dx = -1;
    if (IN.pad_hit[PAD_RIGHT]) *dx = 1;
    float sx = fabsf(IN.lx) > fabsf(IN.rx) ? IN.lx : IN.rx, sy = fabsf(IN.ly) > fabsf(IN.ry) ? IN.ly : IN.ry;   /* either stick, y up */
    if (sy > 0.6f && TI.sy <= 0.6f) { *dy = -1; pad = 1; }
    if (sy < -0.6f && TI.sy >= -0.6f) { *dy = 1; pad = 1; }
    if (sx > 0.6f && TI.sx <= 0.6f) { *dx = 1; pad = 1; }
    if (sx < -0.6f && TI.sx >= -0.6f) { *dx = -1; pad = 1; }
    TI.sx = sx; TI.sy = sy;
    if (*dx || *dy) TI.pad = pad;
}
static void title_pick(int k) {
    TI.sel = k;
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT title pick %d t=%.2f\n", k, G.t);
#endif
    if (k == TM_NEW) { title_to_play(-1); return; }
    snd_sfx_ex(SFX_KEY, 0.7f, 0.5f);
    if (k == TM_GRAVE) {
        title_pages();
        TI.st = TS_GRAVES; TI.gsel = 0;
        for (int i = 2; i >= 0; i--) if (TI.page[i].used) TI.gsel = i;   /* the first written */
        for (int g = 0; g < 3; g++) TI.gk[g] = 0;
    } else TI.st = k == TM_CONTROLS ? TS_CONTROLS : TS_HELP;
}
static int title_tick(float dt) {                                  /* the title's frame: 1 while it holds the screen */
    if (!TI.on) return 0;
    TI.t += dt; TI.grace -= dt;
    if (TI.t < 0.2f) {                                             /* what took over says which words to use */
        for (int b = 0; b < PAD_COUNT; b++) if (IN.pad_down[b]) TI.pad = 1;
        if (IN.down[KEY_H]) TI.pad = 0;
    }
    int dx = 0, dy = 0, any = 0, sel = 0, back = 0;
    if (TI.grace <= 0.0f) {
        title_nav(&dx, &dy);
        sel = IN.hit[KEY_ENTER] || IN.hit[KEY_SPACE] || IN.hit[KEY_E] || IN.hit[KEY_MOUSE1] || IN.pad_hit[PAD_A];
        back = IN.hit[KEY_ESC] || IN.hit[KEY_BACK] || IN.hit[KEY_MOUSE2] || IN.pad_hit[PAD_B] || IN.pad_hit[PAD_MENU];
        any = title_any();
    }
    int busy = any || dx || dy || fabsf(IN.lx) + fabsf(IN.ly) + fabsf(IN.rx) + fabsf(IN.ry) > 0.3f || IN.mdx != 0.0f || IN.mdy != 0.0f;
    TI.idle = busy ? 0.0f : TI.idle + dt;
    switch (TI.st) {
    case TS_PROMPT:
        if (any) {
            TI.st = TS_MENU; TI.press = 1.0f; TI.sel = TM_GRAVE;
            snd_sfx_ex(SFX_RUNE, 0.8f, 0.5f); snd_sfx_ex(SFX_WHOOSH, 0.7f, 0.5f);
#ifdef CRAWL_DEBUG
            fprintf(stderr, "EVENT title menu t=%.2f\n", G.t);
#endif
        } else if (TI.idle > TITLE_IDLE) { title_to_dream(); return 1; }
        break;
    case TS_MENU:
        if (TI.menu < 0.4f) break;                                 /* let it come in first */
        if (dy) { TI.sel = (TI.sel + dy + NTM) % NTM; snd_sfx_ex(SFX_TICK, 0.6f, 0.5f); }
        else if (sel) title_pick(TI.sel);
        else if (back) { TI.st = TS_PROMPT; snd_sfx_ex(SFX_KEY, 0.6f, 0.5f); }
        break;
    case TS_GRAVES:
        if (dx || dy) { int to = clampi(TI.gsel + (dx ? dx : dy), 0, 2); if (to != TI.gsel) { TI.gsel = to; snd_sfx_ex(SFX_TICK, 0.6f, 0.5f); } }
        else if (sel) {
            if (TI.page[TI.gsel].used) { title_to_play(TI.gsel); return 1; }
            TI.foot = "NOTHING IS WRITTEN HERE"; TI.foot_t = 2.0f; snd_sfx_ex(SFX_KNOCK, 0.6f, 0.5f);
        } else if (back) { TI.st = TS_MENU; snd_sfx_ex(SFX_KEY, 0.6f, 0.5f); }
        break;
    default:                                                       /* the controls, help: back to the menu */
        if (TI.st == TS_HELP && sel && !back) {                    /* the lessons on a new run, once they have been given */
            pl_read();
            if (PL.tut & TUT_L1) { PL.tut ^= TUT_AGAIN; pl_write(); snd_sfx_ex(SFX_TICK, 0.7f, 0.5f); }
            else snd_sfx_ex(SFX_KNOCK, 0.5f, 0.5f);
            break;
        }
        if (sel || back) { TI.st = TS_MENU; snd_sfx_ex(SFX_KEY, 0.6f, 0.5f); }
        break;
    }
    float to = TI.st == TS_PROMPT ? 0.0f : 1.0f;                   /* the menu comes in over 0.7 s, goes in 0.6 */
    TI.menu = to > TI.menu ? fminf(to, TI.menu + dt / 0.7f) : fmaxf(to, TI.menu - dt / 0.6f);
    TI.press = fmaxf(0.0f, TI.press - dt / 0.35f);
    for (int k = 0; k < NTM; k++) { float g = k == TI.sel ? 1.0f : 0.0f; TI.mk[k] = g > TI.mk[k] ? fminf(g, TI.mk[k] + dt / 0.3f) : fmaxf(g, TI.mk[k] - dt / 0.3f); }
    for (int i = 0; i < 3; i++) { float g = TI.st == TS_GRAVES && i == TI.gsel ? 1.0f : 0.0f; TI.gk[i] = g > TI.gk[i] ? fminf(g, TI.gk[i] + dt / 0.35f) : fmaxf(g, TI.gk[i] - dt / 0.35f); }
    if ((TI.foot_t -= dt) <= 0.0f) TI.foot = NULL;
    return 1;
}
static void dim_view(Frame *f);
static void pad_fill(UiPad *u);
static void ui_on(Frame *f, int marble, const char *where);
static void ui_off(void);
static void title_controls(Frame *f) {                             /* THE CONTROLS: the keys' whole list, or the controller's own screen */
    if (TI.pad) {
        UiPad u;
        pad_fill(&u);
        u.t = TI.t; u.title = "THE CONTROLLER";
        u.about = "TO CHANGE THEM: IN PLAY, MENU, THEN THE CONTROLLER.";
        u.keys = "\1B BACK";
        ui_on(f, 0, "title pad"); dim_view(f); ui_pad(f, &u); ui_off();
        return;
    }
    static const char *const L[] = {
        WEB ? "WASD WALK  MOUSE TURNS  SPACE CASTS" : "WASD WALK  MOUSE TURNS  CLICK CASTS", "SHIFT SPRINTS FORWARD, A WHILE",
        "1-9 SPELLS  E OPENS, TAKES, SPEAKS", "HOLD A SPELL TO CHARGE ITS ULTIMATE", "RIGHT CLICK USES  Q NEXT, HOLD: WHEEL",
        "TAP A DIRECTION TWICE TO DODGE", "TAB INVENTORY  B BESTIARY  C THE CODEX", "M THE MAP  L WISPS  H THE STATUS BAR",
        "TURN OR STRAFE HARD TO SHAKE OFF A BAT", "ESC LEAVE, SLEEPWALK, A NEW RUN" };
    int n = (int)(sizeof L / sizeof L[0]), widest = 0;
    for (int i = 0; i < n; i++) widest = imax(widest, hint_width(L[i]));
    ui_on(f, 0, "title controls");
    dim_view(f);
    int bw = imin(W - 8, widest + 28), bh = n * 12 + 50, bx = (W - bw) / 2, by = (H - bh) / 2;
    ui_slab(f, bx, by, bw, bh, SLAB_LEDGER | SLAB_CALM, ORN_NONE, TI.t);
    ui_text16(f, (W - 12 * 8) / 2, by + 7, "THE CONTROLS", FACE_INSC, R_GOLD + 12, TX_INLAID, 1);
    for (int i = 0; i < n; i++) hint8(f, (W - hint_width(L[i])) / 2, by + 28 + i * 12, L[i], R_STONE + 13);
    ui_keys(f, (W - ui_keys_width("ESC BACK")) / 2, by + bh - 14, "ESC BACK", R_STONE + 12);
    ui_off();
}
static void title_help(Frame *f) {                                 /* HELP: the lessons (docs/PLAN-tutorial.md 8.7) */
    ui_on(f, 0, "title help");
    dim_view(f);
    pl_read();
    int given = PL.tut & TUT_L1, on = !given || (PL.tut & TUT_AGAIN);
    int bw = 296, bh = 110, bx = (W - bw) / 2, by = (H - bh) / 2;
    ui_slab(f, bx, by, bw, bh, SLAB_LEDGER | SLAB_CALM, ORN_NONE, TI.t);
    ui_text16(f, (W - 4 * 8) / 2, by + 7, "HELP", FACE_INSC, R_GOLD + 12, TX_INLAID, 1);
    static const char *const L[3] = { "THE FIRST TWO LEVELS OF A RUN TEACH", "AS YOU GO, ON YOUR FIRST RUN HERE.",
                                      "THEIR LESSONS, AGAIN ON A NEW RUN:" };
    for (int i = 0; i < 3; i++) ui_text8(f, (W - (int)strlen(L[i]) * 8) / 2, by + 30 + i * 12, L[i], FACE_INSC, R_STONE + 13, TX_INLAID);
    const char *v = !given ? "THEY COME ON YOUR FIRST RUN" : on ? "YES" : "NO";
    ui_text8(f, (W - (int)strlen(v) * 8) / 2, by + 70, v, FACE_INSC, on ? R_GOLD + 12 : R_STONE + 11, TX_INLAID);
    const char *k = !given ? (TI.pad ? "\1B BACK" : "ESC BACK") : TI.pad ? "\1A CHANGES  \1B BACK" : "E CHANGES  ESC BACK";
    ui_keys(f, (W - ui_keys_width(k)) / 2, by + bh - 16, k, R_STONE + 12);
    ui_off();
}
static int title_up(void) { return TI.on; }
static int title_at_prompt(void) { return TI.on && TI.st == TS_PROMPT; }
static int title_render(Frame *f) {
    if (!TI.on) return 0;
    static char ver[24];
    snprintf(ver, sizeof ver, "%s", HVS_VERSION);
    for (char *c = ver; *c; c++) if (*c >= 'a' && *c <= 'z') *c = (char)(*c - 32);
    TitleView v;
    memset(&v, 0, sizeof v);
    v.scene = TI.scene; v.pad = TI.pad ? TI.pad : IN.touch && !PADX.used ? 2 : 0; v.graves = TI.st == TS_GRAVES;   /* 2: by touch */ v.menu = TI.menu; v.press = TI.press;
    memcpy(v.mk, TI.mk, sizeof v.mk); memcpy(v.gk, TI.gk, sizeof v.gk); memcpy(v.page, TI.page, sizeof v.page);
    v.foot = TI.foot; v.version = ver;
    title_draw(f, TI.t, &v);
    if (TI.st == TS_CONTROLS) title_controls(f);
    else if (TI.st == TS_HELP) title_help(f);
    return 1;
}

/* ---- death's cost (docs/PLAN-b7.md, section 1) ---- */

/* what took the dweller, as a stone or the card would put it */
static void blame_text(const Blame *b, char *buf, int n) {
    static const char *const HZ[NHZ] = { "THE KNIGHT'S ROCKFALL", "THE DEACON'S HELLRENT", "THE CISTERN'S BILE", "THE VOID",
                                         "THE KNIGHT'S SUNDERING ARC", "THE GRASPING DARK",
                                         "THE SWINGING CENSER", "THE EMBALMERS' BREATH", "A VOTIVE SPILL", "THE COLD VAULT", "THE HUNGRY RELIQUARY",
                                         "A SLUICE BURST", "THE GALVANIC POOL", "A WEB SNARE", "SILT GAS", "THE WEEPING SAINT",
                                         "A TIMBER COLLAPSE", "FIREDAMP", "A RUNAWAY CART", "THE LODESTONE SEAM", "THE TITHE CIRCLE",
                                         "THE FISSURE'S BREATH", "THE ASHFALL", "A SINGING CRYSTAL", "SHATTERGLASS", "THE DARK COLD", "THE BLACK MIRROR" };
    int t = b->type < NKINDS ? b->type : MOB_SKEL;
    const char *nm = KINDS[t].name, *w = b->what == BL_MOB ? rank_word(t, clampi(b->rank, 0, NRANK - 1)) : "";
    const char *art = (KINDS[t].flags & KF_UNIQUE) ? "THE" : (strchr("AEIOU", *w ? *w : *nm) ? "AN" : "A");
    switch (b->what) {
    case BL_MOB: snprintf(buf, (size_t)n, "%s %s%s", art, w, nm); break;
    case BL_BOSS: snprintf(buf, (size_t)n, "%s", BOSSES[clampi(b->type, 0, NBOSS - 1)].name); break;
    case BL_HAZARD: snprintf(buf, (size_t)n, "%s", HZ[clampi(b->type, 0, NHZ - 1)]); break;
    case BL_POISON: snprintf(buf, (size_t)n, "%s %s'S POISON", art, nm); break;
    case BL_FIRE: snprintf(buf, (size_t)n, "%s %s'S FIRE", art, nm); break;
    case BL_POOL: snprintf(buf, (size_t)n, "A DROWNED ONE'S POOL"); break;
    case BL_MIRROR: snprintf(buf, (size_t)n, "THE KNIGHT'S MIRROR"); break;
    default: snprintf(buf, (size_t)n, "THE DARK"); break;
    }
}

static int death_costs(void) { return LEDGER.yours && !CON.used && !PK.depth && !tu_mercy(); }   /* nor while the lessons run (docs/PLAN-tutorial.md, T4 a) */   /* in the dream, with a code used, or in a pocket: the old mercy */
static void death_write(void) {                                    /* the page it came from, or the unsigned run */
    static uint8_t buf[SLOT_MAX];
    RUN.signs++;
    int n = run_pack(buf, sizeof buf, PG_DEATH);
    if (n <= 0) return;
    if (LEDGER.page) { slot_write(LEDGER.page - 1, buf, n); progress_point(); }
    else slot_write(SLOT_PROGRESS, buf, n);
    if (LEDGER.marked) mark_write(0);                              /* the dead have had theirs */
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT death_written %s%s bytes %d stones %d t=%.2f\n", LEDGER.page ? "page " : "the unsigned run", LEDGER.page ? RUN_ROMAN[LEDGER.page - 1] : "", n, RUN.nstones, G.t);
#endif
}

/* the fall: a stone for the plot with what was carried, then all of it gone but the runes and what the hall
 * keeps, and the page written before the haze has cleared */
static void death_fall(void) {
    thing_drop_home();                                             /* a puzzle's thing back where it was taken from */
    G.dcost = death_costs(); G.dcard_done = 0; G.dcard_t = 0.0f; G.dlost[0] = 0;
    if (!G.dcost) return;
    Gear *g = &RUN.gear;
    Stone st;
    memset(&st, 0, sizeof st);
    st.when = (int64_t)time(NULL); st.level = G.level; st.gold = RUN.treasure; st.souls = RUN.souls; st.coins = RUN.coins; st.by = G.killer;
    memcpy(st.atk, g->atk, sizeof st.atk); memcpy(st.def, g->def, sizeof st.def);
    st.lined = 1; st.cls = RUN.cls;                                /* the class, and each line's rank and path */
    for (int e = 0; e < NELEM; e++) st.path[e] = (uint8_t)align_path(e);
    memcpy(st.worn, g->worn, sizeof st.worn); memcpy(st.bag, g->bag, sizeof st.bag);
    st.bag_size = g->bag_size; st.bag_quality = g->bag_quality;
    memmove(&RUN.stones[1], &RUN.stones[0], sizeof RUN.stones[0] * (STONES - 1));   /* an eleventh takes the oldest away */
    RUN.stones[0] = st; RUN.nstones = imin(RUN.nstones + 1, STONES);
    int pieces = g->bag_size > 0, spells = g->atk[AT_BFS] >= 0, lines = 0;
    for (int i = 0; i < NSLOT; i++) pieces += g->worn[i].kind != IK_NONE;
    for (int i = 0; i < g->bag_size; i++) pieces += g->bag[i].kind != IK_NONE;
    for (int e = 0; e < NELEM; e++) lines += align_open(e);
    char *d = G.dlost;
    int left = (int)sizeof G.dlost;
    #define LOST(...) do { int w_ = snprintf(d, (size_t)left, __VA_ARGS__); if (w_ > 0 && w_ < left) { d += w_; left -= w_; } } while (0)
    if (RUN.treasure) LOST("$%d", RUN.treasure);
    if (pieces) LOST("%s%d PIECE%s", d > G.dlost ? ", " : "", pieces, pieces > 1 ? "S" : "");
    if (lines) LOST("%s%d LINE%s", d > G.dlost ? ", " : "", lines, lines > 1 ? "S" : "");
    if (spells) LOST("%sTHE BFS9000", d > G.dlost ? ", " : "");
    if (RUN.souls) LOST("%s%d SOULS", d > G.dlost ? ", " : "", RUN.souls);
    if (RUN.coins) LOST("%s%d COIN%s", d > G.dlost ? ", " : "", RUN.coins, RUN.coins > 1 ? "S" : "");
    if (RUN.atone > 1) LOST("%sATONEMENT %d", d > G.dlost ? ", " : "", RUN.atone);
    #undef LOST
    if (d > G.dlost) snprintf(d, (size_t)left, " GONE WITH YOU"); else snprintf(G.dlost, sizeof G.dlost, "YOU HAD NOTHING TO LOSE");
#ifdef CRAWL_DEBUG
    char by[64]; blame_text(&G.killer, by, sizeof by);
    fprintf(stderr, "EVENT death_cost level %d by %s: %s t=%.2f\n", G.level, by, G.dlost, G.t);
#endif
    RUN.treasure = 0; RUN.souls = 0; RUN.soul_frac = 0.0f; RUN.coins = 0; RUN.goal = 0;
    g->atk[AT_BFS] = -1;                                           /* the lines' spells follow them (align_sync) */
    memset(RUN.al, 0, sizeof RUN.al); RUN.al_points = 0;           /* the lines go, every one; */
    RUN.cls = RUN.cls_pts = RUN.cls_spent = 0; RUN.al_gift = 0; RUN.gifts = 0; RUN.gifted = 0;   /* the next life is asked what it was, and given again */
    RUN.replay_from = 0; RUN.latch_said = 0;
    memset(g->worn, 0, sizeof g->worn); memset(g->bag, 0, sizeof g->bag); g->bag_size = g->bag_quality = 0;
    g->bones = 0; g->has_held = 0; memset(&g->held, 0, sizeof g->held);
    memset(RUN.top_rank, 0, sizeof RUN.top_rank);                  /* the adds' ranks start again */
    RUN.atone = 1; RUN.xp = 0; RUN.points = 0; RUN.corner = 0; RUN.wind_spent = 0;   /* the Level of Atonement and all it bought; */
    memset(RUN.steps, 0, sizeof RUN.steps); memset(RUN.over, 0, sizeof RUN.over);       /* the map stays */
    memset(RUN.conf, 0, sizeof RUN.conf); memset(RUN.conf_over, 0, sizeof RUN.conf_over); RUN.ap_overs = 0;   /* the Confessions with them (U13) */
    for (int e = 0; e < NELEM; e++) aug_forget(e);                 /* and the augments with the lines (A14) */
    por_life();                                                    /* the portraits owe their gold again (docs/PLAN-portraits.md) */
    XPF.n = 0; XPF.banner = 0.0f;
    G.level_treasure = G.level_souls = 0;
    RUN.level = 1; RUN.shop_next = 1; RUN.hp = gear_max_hp(g);
    death_write();
}

/* the haze clears in the merchant's room, before level 1 */
static void death_wake(void) {
    G.dead = 0; G.dcost = 0; G.retries = 0;
    G.level = RUN.level; G.max_hp = G.hp = gear_max_hp(&RUN.gear);
    begin_shop();
    if (G.player) { G.sdur = 0.0f; G.shop_look = -1; G.pc_t = -1.0f; }
    say("YOU WAKE BY THE MERCHANT");
    snd_sfx_ex(SFX_WHOOSH, 0.8f, 0.5f); sfx_at(SFX_TRILL, CAT_X, CAT_Y, 1.0f);
}

/* ---- her claim: a marked page opened ---- */

static void numeral(int v, char *buf, int n);

static void commas(int v, char *buf, int n) {                      /* 1,240 */
    char t[16];
    snprintf(t, sizeof t, "%d", v < 0 ? 0 : v);
    int len = (int)strlen(t), o = 0;
    for (int i = 0; i < len && o < n - 1; i++) {
        if (i && (len - i) % 3 == 0 && o < n - 2) buf[o++] = ',';
        buf[o++] = t[i];
    }
    buf[o] = 0;
}

/* a quarter to a half of the gold, and one thing equipped at random (a worn piece taken, or a spell a tier
 * down, lost at tier 1, the Fire Bolt never below the fireball); never a rune, the bag or the Totem's. She
 * signs the page herself, and her card says so. */
static void claim_take(void) {
    static uint8_t buf[SLOT_MAX];
    Gear *g = &RUN.gear;
    LEDGER.claim_due = 0;
    int gold = RUN.treasure * (25 + rnd(26)) / 100, cand[NSLOT + NELEM], nc = 0;
    for (int sl = 0; sl < NSLOT; sl++) if (g->worn[sl].kind != IK_NONE) cand[nc++] = sl;
    for (int e = 0; e < NELEM; e++) if (align_last(e) >= 0) cand[nc++] = NSLOT + e;   /* a line's last node, if it may go */
    char what[96] = "", nm[64], g1[24];
    if (nc) {
        int c = cand[rnd(nc)];
        if (c < NSLOT) {
            piece_full_name(&g->worn[c], nm, sizeof nm);
            snprintf(what, sizeof what, "THE %s", nm);
            memset(&g->worn[c], 0, sizeof g->worn[c]);
        } else {
            int e = c - NSLOT, n = align_last(e);
            snprintf(what, sizeof what, "%s FROM %s", ALIGN[e].n[n].name, ALIGN[e].name);
            RUN.al[e][n] = 0;                                      /* its points lost with it */
        }
    }
    RUN.treasure -= gold;
    commas(gold, g1, sizeof g1);
    char took[140];
    if (gold > 0 && *what) snprintf(took, sizeof took, "%s GOLD, AND %s", g1, what);
    else if (gold > 0) snprintf(took, sizeof took, "%s GOLD", g1);
    else if (*what) snprintf(took, sizeof took, "%s", what);
    else snprintf(took, sizeof took, "NOTHING, FOR YOU HAD NOTHING");
    snprintf(G.claim_text, sizeof G.claim_text, "YOU TRIED TO CHEAT THE DEAD OF THEIR BOUNTY. WHATEVER YOU MEANT BY IT, SHE HAS TAKEN THEIR CLAIM: %s. THE LEDGER IS SIGNED.", took);
    G.claim_card = 1; G.claim_t = 0.0f;
    G.max_hp = gear_max_hp(g); if (G.hp > G.max_hp) G.hp = G.max_hp;
    RUN.hp = imin(RUN.hp, G.max_hp);
    if (G.shop) G.shop_gold = RUN.treasure;
    RUN.signs++;                                                   /* she signs it herself */
    int n = run_pack(buf, sizeof buf, PG_CLAIM);
    if (n > 0) {
        if (LEDGER.page) { slot_write(LEDGER.page - 1, buf, n); progress_point(); }
        else slot_write(SLOT_PROGRESS, buf, n);
    }
    mark_write(0);
    snd_sfx_ex(SFX_STINGER, 0.8f, 0.5f); snd_sfx_ex(SFX_WHISPER, 0.9f, 0.5f);
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT claim gold %d took %s page %d t=%.2f\n", gold, *what ? what : "nothing", LEDGER.page, G.t);
#endif
}

/* danger, for the mark: a creature hunting within eight cells or a missile on its way; health under a
 * quarter; poisoned, burning, held in a web, held fast, or in a hazard's warning or harm; a boss's arena
 * from the doors shutting till it falls */
static int in_danger(void) {
    if (G.shop || G.tally || G.dead || G.exiting || PK.depth) return 0;
    if (G.boss >= 0 && (G.bstate == BS_FIGHT || G.bstate == BS_WAVE || G.bstate == BS_RISE)) return 1;
    if (G.hp * 4 < G.max_hp || G.poison_t > 0.0f || G.slow_t > 0.0f || G.web_hp > 0.0f || hz_danger(G.cx, G.cy)) return 1;
    for (int i = 0; i < MAXBURNS; i++) if (G.burns[i].alive && G.burns[i].foe && hypotf(G.px - G.burns[i].x, G.py - G.burns[i].y) < 0.8f) return 1;
    if (incoming(2.0f) > 0 || skull_charging(9.0f, 0.0f)) return 1;
    for (int i = 0; i < MAXMOBS; i++) {
        const Mob *m = &G.mobs[i];
        if (m->alive && m->state == MS_WALK && m->alert && !m->ally && !(KINDS[m->type].flags & KF_SPIRIT) && hypotf(m->x - G.px, m->y - G.py) < 8.0f) return 1;
    }
    return 0;
}
static void update_mark(float dt) {
    if (!LEDGER.yours || CON.used || !RUN.active) return;
    if (tu_mercy()) { if (LEDGER.marked) mark_write(0); return; }   /* the lessons' levels: a death costs nothing, so she has no claim */
    if (in_danger()) { LEDGER.calm_t = 8.0f; if (!LEDGER.marked) mark_write(1); }
    else if (LEDGER.marked && (LEDGER.calm_t -= dt) <= 0.0f) mark_write(0);
}

enum { LM_NONE, LM_ARMED, LM_EMPTY };
static void update_ledger(void) {
    if (IN.hit[KEY_ESC] || IN.hit[KEY_TAB] || IN.hit[KEY_N]) {        /* back to where it was opened from */
        if (G.led_mode) { G.ui = UI_QUIT; G.ui_sel = Q_OPEN; } else { G.ui = UI_MERCHANT; G.ui_sel = MM_SIGN; }
        snd_sfx_ex(SFX_KEY, 0.6f, 0.5f);
        return;
    }
    int mv = (IN.hit[KEY_S] || IN.hit[KEY_DOWN]) - (IN.hit[KEY_W] || IN.hit[KEY_UP]), pick = -1;
    for (int k = 0; k < SLOT_COUNT; k++) if (IN.hit[KEY_1 + k]) pick = k;
    if (mv || pick >= 0) {
        G.ui_sel = pick >= 0 ? pick : (G.ui_sel + mv + SLOT_COUNT) % SLOT_COUNT;
        G.led_arm = -1; G.led_msg = LM_NONE;
        snd_sfx_ex(SFX_TICK, 0.6f, 0.5f);
    }
    if (!(IN.hit[KEY_ENTER] || IN.hit[KEY_SPACE] || IN.hit[KEY_E] || IN.hit[KEY_MOUSE1])) return;
    int i = G.ui_sel;
    Page g = page_info(i);
    if (G.led_mode && !g.used) { G.led_msg = LM_EMPTY; snd_sfx_ex(SFX_ZAP, 0.4f, 0.5f); return; }
    if ((G.led_mode || (g.used && !g.mine)) && G.led_arm != i) {   /* losing this run, or signing over another's: ask twice */
        G.led_arm = i; G.led_msg = LM_ARMED; snd_sfx_ex(SFX_KNOCK, 0.5f, 0.5f);
        return;
    }
    if (G.led_mode) ledger_open(i); else ledger_sign(i);
}

/* her menu: the ledger, a pat, and leaving her be (another pedestal comes with the hall) */
/* The cornerstones (the Player Abilities screen; docs/PLAN-b9.md, section 6): the five, those held, and the
 * choices waiting, one at each tenth Level of Atonement. W/S look, Enter takes one (Enter again: it is yours
 * until a death). From the caster's screen (Tab, back to it with Tab or Esc) and as Tab's third page. */
static const char *const CS_NAME[NCS] = { "SECOND WIND", "PREMONITION", "CARTOGRAPHER", "LANTERN OF THE DEAD", "GRAVE SENSE" };
static const char *const CS_DOES[NCS] = {
    "WHEN A BLOW WOULD KILL YOU, YOU RISE AGAIN AT HALF YOUR HEALTH. ONCE A RUN OF TEN LEVELS; A CURSED COIN TO THE MERCHANT RESTORES IT SOONER.",
    "A PURPLE ZONE ON THE FLOOR WARNS OF WHAT THE PLACE WILL DO BEFORE IT DOES IT: FALLING STONE, FIRE, BILE, THE VOID, WHERE AN EMBER WILL LAND.",
    "YOUR MAP COMES DRAWN WHOLE, BUT FOR KEYS AND SECRETS. A LOCKED DOOR OR A SECRET SHOWS ON IT AS YOU PASS, SEEN OR NOT.",
    "A FAINT LIGHT OF YOUR OWN, TWO CELLS ROUND YOU. SHADES SHOW IN IT, AND THE WRAITH KEEPS A CELL FURTHER OFF. HOLD L TO SHUTTER IT.",
    "WHILE THE MAP IS OPEN, THE DEAD WITHIN TEN CELLS OF YOU SHOW ON IT AS RED MARKS." };
static int cs_takeable(int c) {
    if (RUN.corner >> c & 1 || corner_due() <= 0) return 0;
    return c != CS_CARTO || RUN.has_map;                           /* the Cartographer needs a map to draw on */
}
static void corner_take(int c) {
    RUN.corner |= 1 << c;
    if (c == CS_WIND) { RUN.wind_spent = 0; RUN.wind_run = crawl_run(G.level); }
    if (c == CS_CARTO && !G.shop) map_carto();
    snd_sfx_ex(SFX_ATONE, 0.8f, 0.5f); snd_sfx_ex(SFX_RUNE, 0.6f, 0.5f);
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT cornerstone %s by %s t=%.2f\n", CS_NAME[c], G.player ? "player" : "autopilot", G.t);
#endif
}
static void update_stones_core(void) {
    int ud = (IN.hit[KEY_S] || IN.hit[KEY_DOWN]) - (IN.hit[KEY_W] || IN.hit[KEY_UP]);
    if (ud) { G.cs_sel = (G.cs_sel + NCS + ud) % NCS; G.cs_arm = -1; snd_sfx_ex(SFX_TICK, 0.6f, 0.5f); }
    int c = G.cs_sel = clampi(G.cs_sel, 0, NCS - 1);
    if (!(IN.hit[KEY_ENTER] || IN.hit[KEY_E] || IN.hit[KEY_SPACE] || IN.hit[KEY_MOUSE1])) return;
    if (!cs_takeable(c)) { snd_sfx_ex(SFX_KNOCK, 0.5f, 0.5f); return; }
    if (G.cs_arm != c) { G.cs_arm = c; snd_sfx_ex(SFX_KEY, 0.7f, 0.5f); return; }
    G.cs_arm = -1;
    corner_take(c);
}
