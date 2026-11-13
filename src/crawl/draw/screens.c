/* A part of scene_crawl.c (docs/PLAN-tidy.md): it is #included there in its place and never compiled on its own.
 * The controller's screen (its logic too) and the look; the ledger, her menu, a stone's page. */

/* ---- the controller (1.0.0b18.1, docs/PLAN-b18-1.md section 7): its screen from Esc's question, and the look ----
 * A job is named by the button it began on (PAD_*); PL.job[b] is the job button b has now. */
static const char *const JOB_SHORT[PAD_COUNT] = { "USE", "DODGE", "NEXT RUNE", "WISPS", "THE GIFT", "THE RUNE", "PAGES", "PAUSE",
                                                  "SPRINT", 0, "THE MAP", "THE BAR", "BESTIARY", "CODEX", "READIED", "CAST" };
static const char *const JOB_LONG[PAD_COUNT] = { "USE", "SWIFT DODGE", "NEXT RUNE", "WISPS", "THE GIFT", "THE RUNE", "PAGES", "PAUSE",
                                                 "SPRINT", "NOTHING", "THE MAP", "THE BAR", "BESTIARY", "CODEX", "READIED", "CAST" };
static const char *const JOB_HOLD[PAD_COUNT] = { 0, 0, "RUNES", 0, 0, 0, 0, 0, 0, 0, "RUNES", "SPELLS", 0, 0, 0, 0 };
static const char *const JOB_DOES[PAD_COUNT] = {
    "USE (OPEN, TAKE, PULL, RING, OFFER A SOUL).", "SWIFT DODGE, THE WAY THE STICK LEANS.",
    "THE NEXT RUNE OR GIFT. HELD: THE RUNES' WHEEL.", "THE WISPS' LIST, OR THE ONE ALWAYS CAST.",
    "THE BOSS'S GIFT READIED, USED.", "THE RUNE READIED, RAISED BY HAND.",
    "TAB'S PAGES. FIXED: WITH MENU, IT IS THE LOOK.", "THE QUESTION, AND BACK. FIXED: WITH VIEW, THE LOOK.",
    "THE SPRINT, ON AND OFF.", "NOTHING: A BUTTON LEFT IDLE.",
    "THE MAP. HELD: THE RUNES' WHEEL.", "THE STATUS BAR. HELD: THE SPELLS' WHEEL.",
    "THE BESTIARY.", "THE CODEX OF AFFIXATIONS.",
    "THE RIGHT HAND: THE RUNE OR GIFT READIED.", "CAST THE SPELL IN HAND; HELD, CHARGE IT." };
static const char *const PAD_SAY[PAD_COUNT] = { "A", "B", "X", "Y", "LB", "RB", "VIEW", "MENU", "THE L-STICK", "THE R-STICK",
                                                "D-PAD UP", "D-PAD DOWN", "D-PAD LEFT", "D-PAD RIGHT", "LT", "RT" };
static const int8_t PJ_LIST[14] = { PAD_A, PAD_B, PAD_X, PAD_Y, PAD_LB, PAD_RB, PAD_LT, PAD_RT, PAD_LS, PAD_RS, PAD_UP, PAD_DOWN,
                                    PAD_LEFT, PAD_RIGHT };      /* the jobs to trade, in the sheet's order */
enum { PO_STICKS = PAD_COUNT, PO_TURN, PO_MENUS, PO_RUMBLE, PO_DEFAULTS };   /* in the order ui_pad lays the options out */
#define PS_ROWS 11
static const int8_t PS_GRID[PS_ROWS][2] = { { PAD_LT, PAD_RT }, { PAD_LB, PAD_RB }, { PAD_LS, PAD_Y }, { PAD_UP, PAD_X }, { PAD_LEFT, PAD_B },
                                            { PAD_RIGHT, PAD_A }, { PAD_DOWN, PAD_RS }, { PAD_VIEW, PAD_MENU }, { PO_STICKS, PO_RUMBLE },
                                            { PO_TURN, PO_DEFAULTS }, { PO_MENUS, -1 } };   /* as drawn: the left hand's down the left,
                                                                      the options under; -1 nothing there */
static struct { int sel, r, c, list, lsel, arm; } PS;
static void pad_menu_open(void) {
    open_ui(UI_PAD);
    PS.sel = PAD_A; PS.r = 5; PS.c = 1; PS.list = PS.arm = 0;
    snd_sfx_ex(SFX_PAGE, 0.6f, 0.5f);
}
static void pad_fill(UiPad *u) {                                   /* the layout as the drawing wants it */
    memset(u, 0, sizeof *u);
    pl_read();
    for (int b = 0; b < PAD_COUNT; b++) { u->job[b] = JOB_SHORT[PL.job[b]]; u->hold[b] = JOB_HOLD[PL.job[b]]; }
    u->fixed = 1 << PAD_VIEW | 1 << PAD_MENU;
    u->sel = u->opt_sel = -1;
}
static void update_padmenu(void) {
    int up = IN.hit[KEY_W] || IN.hit[KEY_UP], down = IN.hit[KEY_S] || IN.hit[KEY_DOWN];
    int left = IN.hit[KEY_A] || IN.hit[KEY_LEFT], right = IN.hit[KEY_D] || IN.hit[KEY_RIGHT];
    int ok = IN.hit[KEY_ENTER] || IN.hit[KEY_E] || IN.hit[KEY_SPACE], back = IN.hit[KEY_ESC];
    pl_read();
    if (PS.list) {                                                 /* the jobs: one chosen is traded onto this button */
        if (up || down) { PS.lsel = (PS.lsel + (down ? 1 : 13)) % 14; snd_sfx_ex(SFX_TICK, 0.6f, 0.5f); }
        if (back) { PS.list = 0; snd_sfx_ex(SFX_KEY, 0.6f, 0.5f); return; }
        if (ok) {
            int job = PJ_LIST[PS.lsel], from = pl_on(job);
            if (from != PS.sel) { pl_trade(PS.sel, from); pl_write(); snd_sfx_ex(SFX_RUNE, 0.6f, 0.5f); }
            else snd_sfx_ex(SFX_KEY, 0.6f, 0.5f);
            PS.list = 0;
#ifdef CRAWL_DEBUG
            if (from != PS.sel) fprintf(stderr, "EVENT pad_trade %s takes %s, %s takes %s t=%.2f\n", PAD_SAY[PS.sel], JOB_LONG[job], PAD_SAY[from], JOB_LONG[PL.job[from]], G.t);
            else fprintf(stderr, "EVENT pad_keep %s %s t=%.2f\n", PAD_SAY[PS.sel], JOB_LONG[job], G.t);
#endif
        }
        return;
    }
    if (back) { G.ui = UI_QUIT; G.ui_sel = Q_PAD; PS.arm = 0; snd_sfx_ex(SFX_KEY, 0.6f, 0.5f); return; }   /* back to the question */
    if (up || down) {                                              /* down the column, past an empty place, round */
        do PS.r = (PS.r + (down ? 1 : PS_ROWS - 1)) % PS_ROWS; while (PS_GRID[PS.r][PS.c] < 0);
        PS.sel = PS_GRID[PS.r][PS.c]; PS.arm = 0; snd_sfx_ex(SFX_TICK, 0.6f, 0.5f);
    } else if ((left || right) && PS.sel == PO_TURN) {             /* the turning, by tenths */
        int t = clampi(PL.turn + (right ? 1 : -1), 5, 20);
        if (t != PL.turn) { PL.turn = t; pl_write(); snd_sfx_ex(SFX_TICK, 0.6f, 0.5f); } else snd_sfx_ex(SFX_KNOCK, 0.35f, 0.5f);
    } else if ((left || right) && PS.sel == PO_MENUS) {            /* the menus' stick: left, right, either */
        PL.mstick = (PL.mstick + (right ? 1 : 2)) % 3; pl_write(); snd_sfx_ex(SFX_TICK, 0.6f, 0.5f);
    } else if (left || right) {                                    /* across, or the nearest above where nothing is */
        PS.c = 1 - PS.c;
        while (PS_GRID[PS.r][PS.c] < 0) PS.r--;
        PS.sel = PS_GRID[PS.r][PS.c]; PS.arm = 0; snd_sfx_ex(SFX_TICK, 0.6f, 0.5f);
    }
    if (!ok) return;
    if (PS.sel < PAD_COUNT) {
        if (pl_fixed(PS.sel)) { snd_sfx_ex(SFX_KNOCK, 0.4f, 0.5f); return; }   /* View and Menu stay */
        PS.list = 1; PS.lsel = 0;
        for (int i = 0; i < 14; i++) if (PJ_LIST[i] == PL.job[PS.sel]) PS.lsel = i;
        snd_sfx_ex(SFX_PAGE, 0.5f, 0.5f);
        return;
    }
    if (PS.sel == PO_STICKS) PL.swap ^= 1;
    else if (PS.sel == PO_MENUS) PL.mstick = (PL.mstick + 1) % 3;
    else if (PS.sel == PO_RUMBLE) { PL.quiet ^= 1; if (!PL.quiet) pad_rumble(0.4f, 0.4f, 0.25f); }   /* a shudder, to say so */
    else if (PS.sel == PO_DEFAULTS) {                              /* asked twice */
        if (!PS.arm) { PS.arm = 1; snd_sfx_ex(SFX_KNOCK, 0.6f, 0.5f); return; }
        pl_default(); PS.arm = 0;
    } else return;
    pl_write();
    snd_sfx_ex(SFX_RUNE, 0.5f, 0.5f);
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT pad_option %d swap %d quiet %d turn %d menus %d t=%.2f\n", PS.sel - PAD_COUNT, PL.swap, PL.quiet, PL.turn, PL.mstick, G.t);
#endif
}
static void render_padmenu(Frame *f) {
    static char about[160], o0[24], o1[24], o2[24], o3[24];
    static const char *const MSTICK[3] = { "LEFT", "RIGHT", "BOTH" };
    static const char *list[14];
    static int on[14];
    UiPad u;
    pad_fill(&u);
    u.t = G.ui_t; u.title = "THE CONTROLLER";
    snprintf(o0, sizeof o0, "STICKS: %s", PL.swap ? "SWAP" : "SAME");
    snprintf(o1, sizeof o1, "TURNING: %d.%d", PL.turn / 10, PL.turn % 10);
    snprintf(o2, sizeof o2, "MENUS: %s", MSTICK[PL.mstick]);
    snprintf(o3, sizeof o3, "RUMBLE: %s", PL.quiet ? "OFF" : "ON");
    u.opt[0] = o0; u.opt[1] = o1; u.opt[2] = o2; u.opt[3] = o3; u.opt[4] = "THE DEFAULTS";
    if (PS.sel < PAD_COUNT) u.sel = PS.sel; else u.opt_sel = PS.sel - PAD_COUNT;
    u.keys = PS.sel == PO_TURN || PS.sel == PO_MENUS ? "D-PAD \x1b\x1a CHANGES  \1B BACK" : "D-PAD CHOOSES  \1A CHANGES  \1B BACK";
    if (PS.sel < PAD_COUNT) snprintf(about, sizeof about, "%s: %s", PAD_SAY[PS.sel], JOB_DOES[PL.job[PS.sel]]);
    else if (PS.sel == PO_STICKS) snprintf(about, sizeof about, "WHICH STICK WALKS AND WHICH TURNS. NOW THE %s WALKS.", PL.swap ? "RIGHT" : "LEFT");
    else if (PS.sel == PO_TURN) snprintf(about, sizeof about, "HOW FAST THE TURNING STICK TURNS YOU: HALF TO TWICE THE FIRST.");
    else if (PS.sel == PO_MENUS) snprintf(about, sizeof about, "WHICH STICK MOVES IN MENUS. A WHEEL TAKES THE FREE HAND'S STICK.");
    else if (PS.sel == PO_RUMBLE) snprintf(about, sizeof about, "THE MOTORS: A HURT, A ROCK FALLING, A BOSS'S FALL.");
    else snprintf(about, sizeof about, PS.arm ? "AGAIN, AND EVERY BUTTON GOES BACK TO ITS FIRST JOB." : "EVERY BUTTON'S FIRST JOB, THE STICKS AS THEY CAME, RUMBLE ON.");
    u.about = about;
    if (PS.sel == PO_DEFAULTS && PS.arm) { u.about_col = R_GOLD + 11; u.keys = "\1A RESTORES  \1B BACK"; }
    if (PS.list) {
        int job = PJ_LIST[PS.lsel], from = pl_on(job);
        for (int i = 0; i < 14; i++) { list[i] = JOB_LONG[PJ_LIST[i]]; on[i] = pl_on(PJ_LIST[i]); }
        u.list = list; u.list_on = on; u.nlist = 14; u.list_sel = PS.lsel;
        if (from == PS.sel) snprintf(about, sizeof about, "%s KEEPS %s.", PAD_SAY[PS.sel], JOB_LONG[job]);
        else snprintf(about, sizeof about, "%s TAKES %s; %s TAKES %s.", PAD_SAY[PS.sel], JOB_LONG[job], PAD_SAY[from], JOB_LONG[PL.job[PS.sel]]);
        u.keys = "\1A TRADES  \1B BACK";
    }
    dim_view(f);
    ui_pad(f, &u);
}
static int pad_look_tick(float dt) {                               /* the look up: the crypt waits; Esc on the keys puts it away */
    if (!PADX.look) return 0;
    if (!G.player || !IN.pad) { PADX.look = 0; return 0; }
    PADX.look_t += dt;
    if (IN.hit[KEY_ESC]) { PADX.look = 0; snd_sfx_ex(SFX_KEY, 0.6f, 0.5f); }
    return 1;
}
static void pad_look_render(Frame *f) {
    if (!PADX.look || !G.player) return;
    UiPad u;
    pad_fill(&u);
    u.t = PADX.look_t; u.title = "THE CONTROLLER, AS IT IS";
    u.about = "TO CHANGE THEM: MENU, THEN THE CONTROLLER.";
    u.keys = "VIEW+MENU  \1B CLOSES";
    ui_on(f, 1, "look");
    dim_view(f);
    ui_pad(f, &u);
    ui_off();
}

/* a page of the ledger, 24 wide and 27 tall, its corner turned down and its numeral at the foot; gold for
 * this run's */
static void ledger_leaf(Frame *f, int x, int y, int i, int used, int mine) {
    int w = 24, h = 27, fold = 6;
    for (int yy = 0; yy < h; yy++)
        for (int xx = 0; xx < w; xx++) {
            int c = w - 1 - xx + yy;                                /* the turned corner */
            if (c < fold) continue;
            int edge = xx == 0 || yy == h - 1 || xx == w - 1 || yy == 0 || c == fold;
            int k = used ? (edge ? 7 : 12 - (xx + 2 * yy) % 5 / 3) : (edge ? 5 : 7);
            gfx_px(f, x + xx, y + yy, (uint8_t)((used ? R_BONE : R_STONE) + k));
        }
    for (int k = 0; k < fold; k++) gfx_px(f, x + w - fold + k, y + k, (uint8_t)((used ? R_BONE : R_STONE) + 6));   /* the fold's crease */
    if (used)                                                       /* her hand: lines of something written */
        for (int ln = 0; ln < 4; ln++)
            for (int xx = 3; xx < w - 3 - (ln == 0 ? fold - 1 : 0) - (ln == 3 ? 6 : 0); xx++)
                if ((xx * 7 + ln * 3) % 5) gfx_px(f, x + xx, y + 4 + ln * 3, R_BONE + 5);
    int tw = gfx_text_width(RUN_ROMAN[i], 1);
    gfx_text8(f, x + (w - tw) / 2, y + h - 10, RUN_ROMAN[i], mine ? (uint8_t)(R_GOLD + 9) : (uint8_t)(used ? R_BONE + 4 : R_STONE + 3), 1);
}

/* SIGN THE LEDGER / OPEN THE LEDGER: her three pages */
static void render_ledger(Frame *f) {
    static const char *const MONTH[12] = { "JAN", "FEB", "MAR", "APR", "MAY", "JUN", "JUL", "AUG", "SEP", "OCT", "NOV", "DEC" };
    static const char *const WHY[3] = { "SIGNED", "AT A DEATH", "HER CLAIM" };
    dim_view(f);
    int bw = 288, bh = 150, bx = (W - bw) / 2, by = (H - bh) / 2 - 6;
    ui_slab(f, bx, by, bw, bh, SLAB_LEDGER, ORN_PAW, G.ui_t);       /* her book on a ledger stone: the paw is hers */
    const char *title = G.led_mode ? "OPEN THE LEDGER" : "SIGN THE LEDGER";
    gfx_text16(f, (W - gfx_text_width(title, 1)) / 2, by + 8, title, G.led_mode ? R_BLOOD + 13 : R_GOLD + 12, 1);
    ui_well(f, bx + 6, by + 26, bw - 12, SLOT_COUNT * 34 + 3);
    Page sel_pg;
    memset(&sel_pg, 0, sizeof sel_pg);
    for (int i = 0; i < SLOT_COUNT; i++) {
        int y = by + 30 + i * 34, sel = i == G.ui_sel;
        Page g = page_info(i);
        ui_row(f, bx + 8, y - 3, bw - 16, 33, sel);
        if (sel) sel_pg = g;
        ledger_leaf(f, bx + 13, y - 1, i, g.used, g.used && g.mine);
        char l1[48], l2[48];
        const char *tag = "";
        if (g.used) {
            snprintf(l1, sizeof l1, "LEVEL %d  $%d", g.level, g.gold);
            time_t when = (time_t)g.when;
            struct tm *tm = g.when > 0 ? localtime(&when) : NULL;
            if (tm) snprintf(l2, sizeof l2, "%d %s %d  %02d:%02d", tm->tm_mday, MONTH[tm->tm_mon % 12], tm->tm_year + 1900, tm->tm_hour, tm->tm_min);
            else snprintf(l2, sizeof l2, "%d SOULS", g.souls);
            tag = g.old ? "CARVED" : WHY[clampi(g.why, 0, 2)];
        } else {
            snprintf(l1, sizeof l1, "A BLANK PAGE");
            snprintf(l2, sizeof l2, "NOTHING WRITTEN");
        }
        gfx_text8(f, bx + 46, y + 5, l1, sel ? (uint8_t)C_WHITE : (uint8_t)(g.used ? R_STONE + 12 : R_STONE + 9), 1);
        gfx_text8(f, bx + 46, y + 16, l2, R_STONE + 9, 1);
        gfx_text8(f, bx + bw - 12 - gfx_text_width(tag, 1), y + 5, tag, g.why == PG_SIGNED ? (uint8_t)(R_STONE + 10) : (uint8_t)(R_BLOOD + 12), 1);
        if (g.used && g.mine) gfx_text8(f, bx + bw - 12 - gfx_text_width("THIS RUN", 1), y + 16, "THIS RUN", R_GOLD + 10, 1);
    }
    const char *note = G.led_mode ? "ENTER OPENS   ESC GOES BACK" : "ENTER SIGNS   ESC GOES BACK";
    int col = R_GOLD + 9, keys = 1;
    if (!G.led_mode && sel_pg.used && !sel_pg.mine) { note = "ANOTHER RUN'S PAGE"; keys = 0; }
    if (G.led_msg == LM_EMPTY) { note = "NOTHING IS WRITTEN HERE"; col = R_STONE + 10; keys = 0; }
    else if (G.led_msg == LM_ARMED && G.led_arm == G.ui_sel) {
        note = G.led_mode ? "ENTER AGAIN. THE UNSIGNED IS LOST" : "ENTER AGAIN TO SIGN OVER IT";
        col = R_BLOOD + 13; keys = 0;
    }
    if (keys) hall_keys(f, by + bh - 15, note);                    /* the keys on caps; a word of hers in their place */
    else hall_word(f, by + bh - 15, pkp(note), col);
}

/* E at the merchant: her menu, and a note on the row chosen */
static void render_merchantmenu(Frame *f) {
    static const char *const ROW[NMM] = { "SIGN THE LEDGER", "ASCEND", "OFFER SOULS", "ANOTHER PAST", "ANOTHER PEDESTAL", "OFFER A COIN", "PAT HER", "LEAVE" };
    int sel = clampi(G.ui_sel, 0, NMM - 1);
    dim_view(f);
    char line[160];
    if (LEDGER.page) snprintf(line, sizeof line, "PAGE %s", RUN_ROMAN[LEDGER.page - 1]); else snprintf(line, sizeof line, "UNSIGNED");
    hall_frame(f, "THE MERCHANT", R_GOLD + 12, line, LEDGER.page ? R_BONE + 12 : R_BLOOD + 13);
    ui_well(f, 9, 28, 302, NMM * 11 + 6);                          /* her rows in one well, what the chosen one means in another */
    for (int i = 0; i < NMM; i++) {
        int y = 32 + i * 11, rx = 304;
        ui_row(f, 11, y - 2, 298, 11, i == sel);
        int off = (i == MM_OFFER && spirit() < (float)OFFER_SOULS) || (i == MM_PED && RUN.peds >= NPED) || (i == MM_WIND && !wind_restorable()) || (i == MM_ASCEND && !ascend_open())
               || (i == MM_VOW && RUN.cls && RUN.atone < VOW_ATONE);
        gfx_text8(f, 16, y, i == MM_VOW && !RUN.cls ? "WHAT YOU WERE" : ROW[i], off ? (uint8_t)(R_STONE + 8) : (i == sel ? (uint8_t)C_WHITE : (uint8_t)(R_STONE + 12)), 1);
        if (i == MM_ASCEND) {                                      /* how far it goes, or the points still to spend */
            char ar[24];
            int to = atone_reach();
            if (to > RUN.atone) snprintf(ar, sizeof ar, "TO %d", to);
            else if (RUN.points > 0) snprintf(ar, sizeof ar, "%d POINTS", RUN.points);
            else snprintf(ar, sizeof ar, "%d", RUN.atone);
            gfx_text8(f, rx - gfx_text_width(ar, 1), y, ar, to > RUN.atone || RUN.points > 0 ? (uint8_t)(R_PURPLE + 13) : (uint8_t)(R_STONE + 8), 1);
        }
        if (i == MM_WIND && wind_restorable()) gfx_text8(f, rx - gfx_text_width("1 COIN", 1), y, "1 COIN", RUN.coins > 0 ? (uint8_t)(R_GOLD + 11) : (uint8_t)(R_BLOOD + 13), 1);
        if (i == MM_OFFER) {
            char os[16];
            snprintf(os, sizeof os, "%d SOULS", OFFER_SOULS);
            gfx_text8(f, rx - gfx_text_width(os, 1), y, os, spirit() >= (float)OFFER_SOULS ? (uint8_t)(R_GLOW + 12) : (uint8_t)(R_STONE + 8), 1);
        }
        if (i == MM_VOW && RUN.cls && RUN.atone >= VOW_ATONE)
            gfx_text8(f, rx - gfx_text_width("5 COINS", 1), y, "5 COINS", RUN.coins >= VOW_COINS ? (uint8_t)(R_GOLD + 11) : (uint8_t)(R_BLOOD + 13), 1);
        if (i == MM_PED && RUN.peds < NPED) {                       /* its price, red if it is beyond you */
            char pr[16];
            snprintf(pr, sizeof pr, "$%d", ped_price());
            gfx_text8(f, rx - gfx_text_width(pr, 1), y, pr, RUN.treasure >= ped_price() ? (uint8_t)(R_GOLD + 11) : (uint8_t)(R_BLOOD + 13), 1);
        }
    }
    int ty = 28 + NMM * 11 + 10;
    ui_well(f, 9, ty - 4, 302, 181 - (ty - 4));
    if (sel == MM_SIGN && CON.used) snprintf(line, sizeof line, "THE DEAD TOOK NOTE. SHE WILL SIGN NOTHING WHILE THIS LASTS.");
    else if (sel == MM_SIGN && LEDGER.page) snprintf(line, sizeof line, "SHE KEEPS YOUR RUN AS IT STANDS, ON A PAGE OF YOUR CHOOSING. YOU CAME FROM PAGE %s.", RUN_ROMAN[LEDGER.page - 1]);
    else if (sel == MM_SIGN) snprintf(line, sizeof line, "SHE KEEPS YOUR RUN AS IT STANDS, ON A PAGE OF YOUR CHOOSING. THIS RUN IS UNSIGNED: FLEE NOW AND IT IS LOST.");
    else if (sel == MM_ASCEND && atone_reach() > RUN.atone)
        snprintf(line, sizeof line, "WHAT YOU HAVE DONE BUYS LEVEL OF ATONEMENT %d: TEN POINTS A LEVEL, TO SPEND AS YOU WILL. A DEATH TAKES IT ALL.", atone_reach());
    else if (sel == MM_ASCEND && (RUN.points > 0 || corner_due() > 0 || over_due() > 0)) snprintf(line, sizeof line, "LEVEL OF ATONEMENT %d. %d POINTS STILL TO SPEND.", RUN.atone, RUN.points);
    else if (sel == MM_ASCEND) snprintf(line, sizeof line, "LEVEL OF ATONEMENT %d. %d MORE EXPERIENCE FOR THE NEXT.", RUN.atone, atone_need(RUN.atone + 1) - RUN.xp);
    else if (sel == MM_OFFER) snprintf(line, sizeof line, "TEN SOULS, AND SHE GIVES YOU %.0f EXPERIENCE FOR THEM. YOU HOLD %d OF %d.", offer_xp(), RUN.souls, vessel());
    else if (sel == MM_VOW && !RUN.cls) snprintf(line, sizeof line, "SHE ASKS WHAT YOU WERE BEFORE. A LIFE KEEPS ONE ANSWER.");
    else if (sel == MM_VOW && RUN.atone < VOW_ATONE) snprintf(line, sizeof line, "YOU WERE %s. AT LEVEL OF ATONEMENT %d SHE WILL HEAR ANOTHER PAST, FOR FIVE CURSED COINS.", CLASSES[cls_el()].name, VOW_ATONE);
    else if (sel == MM_VOW) snprintf(line, sizeof line, "YOU WERE %s. FIVE CURSED COINS AND SHE WILL HEAR ANOTHER PAST; WHAT YOUR LINE HOLDS STAYS, NO LONGER FREE.", CLASSES[cls_el()].name);
    else if (sel == MM_WIND && wind_restorable()) snprintf(line, sizeof line, "YOUR SECOND WIND IS SPENT. A CURSED COIN AND IT IS YOURS AGAIN.");
    else if (sel == MM_WIND && (RUN.corner >> CS_WIND & 1)) snprintf(line, sizeof line, "YOUR SECOND WIND IS WAITING. SHE WANTS NOTHING FOR IT YET.");
    else if (sel == MM_WIND) snprintf(line, sizeof line, "SHE TAKES A CURSED COIN TO RESTORE A SECOND WIND, ONCE YOU HAVE ONE.");
    else if (sel == MM_PED && RUN.peds >= NPED) snprintf(line, sizeof line, "ALL SEVEN STAND. SHE HAS NO MORE TO SET OUT.");
    else if (sel == MM_PED) snprintf(line, sizeof line, "A %s PEDESTAL, AND A WARE ON IT FROM NOW ON, EVERY VISIT. KEPT THROUGH DEATH.",
                                     RUN.peds == 2 ? "THIRD" : RUN.peds == 3 ? "FOURTH" : RUN.peds == 4 ? "FIFTH" : RUN.peds == 5 ? "SIXTH" : "SEVENTH");
    else snprintf(line, sizeof line, sel == MM_PAT ? "SHE ALLOWS IT." : "SHE WILL BE HERE. SHE IS ALWAYS HERE.");
    int need = ui_wrap_count(line, 36);
    if (need > 5) ui_overflow("merchant note", line, need - 5);
    text_wrap(f, 14, ty, line, 36, sel == MM_SIGN && !LEDGER.page && !CON.used ? R_BLOOD + 12 : R_STONE + 12, 5);
    hall_keys(f, 184, "W/S CHOOSE  ENTER TAKES  ESC CLOSES");
}

/* a stone's page, laid out like the bestiary's: the epitaph and when, then what was carried */
static void numeral(int v, char *buf, int n) {                     /* a tier in Roman numerals */
    static const char *const ONES[10] = { "", "I", "II", "III", "IV", "V", "VI", "VII", "VIII", "IX" };
    static const char *const TENS[4] = { "", "X", "XX", "XXX" };
    snprintf(buf, (size_t)n, "%s%s", TENS[clampi(v / 10, 0, 3)], ONES[clampi(v % 10, 0, 9)]);
}
static void render_stonemenu(Frame *f) {
    static const char *const MONTH[12] = { "JANUARY", "FEBRUARY", "MARCH", "APRIL", "MAY", "JUNE", "JULY", "AUGUST", "SEPTEMBER", "OCTOBER", "NOVEMBER", "DECEMBER" };
    static const char *const ORD[8] = { "FIRST", "SECOND", "THIRD", "FOURTH", "FIFTH", "SIXTH", "SEVENTH", "EIGHTH" };
    int n = RUN.nstones;
    if (n <= 0) return;
    int si = clampi(G.stone_sel, 0, n - 1);
    const Stone *st = &RUN.stones[si];
    dim_view(f);
    ui_slab(f, 4, 4, 312, 192, SLAB_HEAD, ORN_ROSE, G.ui_t);       /* a headstone, as the plot's are */
    ui_text16(f, 160 - 4 * 19, 10, "HERE LIES A DWELLER", FACE_INSC, R_STONE + 13, TX_INLAID, 1);
    char line[200], by[64], lname[48], num[8], ep[4][48];
    blame_text(&st->by, by, sizeof by);
    level_name(st->level, lname, sizeof lname);
    int run = crawl_run(st->level);
    if (run < 8) snprintf(line, sizeof line, "OF THE %s RUN, TAKEN BY %s ON LEVEL %d, %s.", ORD[run], by, st->level, lname);
    else snprintf(line, sizeof line, "OF RUN %d, TAKEN BY %s ON LEVEL %d, %s.", run + 1, by, st->level, lname);
    int el = doc_wrap(line, 32, 32, 0, ep, 4);                     /* the epitaph, each line centred, clear of the roses */
    if (el > 3) ui_overflow("stone epitaph", line, el - 3);
    for (int i = 0; i < el && i < 3; i++) gfx_text8(f, 160 - gfx_text_width(ep[i], 1) / 2, 30 + i * 9, ep[i], R_STONE + 13, 1);
    time_t when = (time_t)st->when;
    struct tm *tm = st->when > 0 ? localtime(&when) : NULL;
    if (tm) snprintf(line, sizeof line, "%d %s %d, %02d:%02d", tm->tm_mday, MONTH[tm->tm_mon % 12], tm->tm_year + 1900, tm->tm_hour, tm->tm_min);
    else snprintf(line, sizeof line, "A DATE WORN AWAY");
    gfx_text8(f, 14, 59, line, R_STONE + 10, 1);
    snprintf(line, sizeof line, "%d OF %d", si + 1, n);
    gfx_text8(f, 306 - gfx_text_width(line, 1), 59, line, R_STONE + 10, 1);
    int y0 = 74, rows = stone_rows(st), sel = clampi(G.ui_sel, 0, rows - 1);
    ui_well(f, 9, 70, 112, 111);                                   /* what was carried down the left, the chosen one on the right */
    ui_well(f, 124, 70, 187, 111);
    for (int r = 0; r < rows; r++) {                               /* the list: spells and runes, the worn, the bag's */
        int ry = y0 + r * 8 + (r > 0 ? 1 : 0) + (r > NSLOT ? 1 : 0);
        const Piece *p = r == 0 ? NULL : (r <= NSLOT ? &st->worn[r - 1] : &st->bag[r - 1 - NSLOT]);
        ui_row(f, 11, ry - 1, 108, 9, r == sel);
        if (!p) { gfx_text8(f, 14, ry, st->lined ? "THE LINES" : "SPELLBOOK", r == sel ? (uint8_t)C_WHITE : (uint8_t)(R_STONE + 12), 1); continue; }
        snprintf(line, sizeof line, "%s", r <= NSLOT ? SLOT_NAME[r - 1] : (p->kind ? KIND_NAME[p->kind] : "IN THE BAG"));
        gfx_text8(f, 14, ry, line, p->kind ? (uint8_t)qual_text(p->quality) : (uint8_t)(R_STONE + 7), 1);
        if (p->anointed) gfx_glyph8(f, 14 + gfx_text_width(line, 1), ry, '*', R_GOLD + 12, -1, 1);
    }
    int cx = 128, cw = 311 - 4 - cx, rx = cx + 108;                /* the runes' column clear of the longest class's name */
    if (sel > 0) {
        const Piece *p = sel <= NSLOT ? &st->worn[sel - 1] : &st->bag[sel - 1 - NSLOT];
        gfx_text8(f, cx, y0, sel <= NSLOT ? SLOT_NAME[sel - 1] : "IN THE BAG", R_STONE + 10, 1);
        piece_card(f, cx, y0 + 12, cw, p, NULL, NULL);
    } else {
        gfx_text8(f, cx, y0, st->lined ? "LINES" : "SPELLS", R_STONE + 10, 1);   /* the lines (the spellbook, before) and the runes */
        gfx_text8(f, rx, y0, "RUNES", R_STONE + 10, 1);
        int sy = y0 + 11, ry = y0 + 11;
        if (st->lined) {
            if (st->cls >= 1 && st->cls <= NELEM) {                /* what they were */
                class_mark(f, cx, sy - 1, st->cls - 1, 0);
                gfx_text8(f, cx + 14, sy + 1, CLASS_SHORT[st->cls - 1], ramp_at(ELEM_RAMP[st->cls - 1], 12), 1);
                sy += 13;
            }
            for (int e = 0; e < NELEM; e++) {                      /* each line open: its glyph, its rank, its path */
                int r = st->atk[LINE_ATK[e]];
                if (r < 0) continue;
                elem_glyph(f, cx, sy, e, 0);
                snprintf(line, sizeof line, "RANK %d%s", r, st->path[e] == 1 ? " A" : st->path[e] == 2 ? " B" : "");
                gfx_text8(f, cx + 10, sy, line, ramp_at(ELEM_RAMP[e], 12), 1);
                sy += 9;
            }
            if (st->atk[AT_BFS] >= 0) { gfx_text8(f, cx, sy, "BFS9000", ramp_at(gear_color(GK_ATK, AT_BFS), 12), 1); sy += 8; }
        } else for (int k = 0; k < NATK; k++) {                    /* an older stone: its spellbook, by the old names */
            static const char *const OLD_SHORT[NATK] = { "FIRE", "FROST", "CHAIN", "SPEAR", "DRAIN", "HELL", "WISPS", "SPARKS", "BFS" };
            if (st->atk[k] < 0) continue;
            numeral(st->atk[k], num, sizeof num);
            snprintf(line, sizeof line, "%s %s", OLD_SHORT[k], st->atk[k] ? num : "");
            gfx_text8(f, cx, sy, line, ramp_at(gear_color(GK_ATK, k), 12), 1);
            sy += 8;
        }
        for (int k = 0; k < NDEF; k++) {
            if (st->def[k] <= 0) continue;
            int q = clampi(st->def[k], 1, NQUAL) - 1;
            snprintf(line, sizeof line, "%s %s", DEF_SHORT[k], Q_TAG[q]);
            gfx_text8(f, rx, ry, line, (uint8_t)qual_text(q), 1);
            ry += 8;
        }
        if (ry == y0 + 11) gfx_text8(f, rx, ry, "NONE", R_STONE + 8, 1);
        int y = imax(sy, ry) + 6;
        snprintf(line, sizeof line, "$%d  %d SOULS", st->gold, st->souls);
        gfx_text8(f, cx, y, line, R_GOLD + 11, 1);
        if (st->coins) {
            snprintf(line, sizeof line, "%d", st->coins);
            draw_coin(f, cx, y + 11);
            gfx_text8(f, cx + 10, y + 11, line, R_GOLD + 11, 1);
        }
    }
    hall_keys(f, 184, "W/S CHOOSE  A/D STONES  ESC CLOSES");
}

