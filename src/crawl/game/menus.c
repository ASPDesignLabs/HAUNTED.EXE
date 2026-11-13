/* A part of scene_crawl.c (docs/PLAN-tidy.md): it is #included there in its place and never compiled on its own.
 * The menus' logic: the Alignment and its gifts and augments, the class, the bell and font, confessing and ascending, her menu, a stone, the L list, the altar, the shrine. */

/* ---- the Alignment's screen (docs/PLAN-b11.md, section 8): at the Lampstand to buy, Tab's fourth page to look ----
 * The tree: the opening; Efficiency and Insight in either order, both full; the ability; one path, its nodes in
 * order, the ultimate last. A node is lit when bought, outlined when it can be bought now, dim when it can't yet,
 * chained over when the other path was taken. The first node of a path asks twice. Unbinding a line costs two
 * Cursed Coins and gives back half its points; a line that came open stays open. */
static const int ND_ROW[NNODE] = { 0, 1, 1, 2, 3, 4, 5, 3, 4, 5 };
static float al_buy_t = -9.0f;                                     /* the last node bought, for its groove to fill */
static int al_buy_e = -1, al_buy_n = -1;
static const int ND_COL[NNODE] = { 1, 0, 2, 1, 0, 0, 0, 2, 2, 2 };
static int align_point(int e, int n) { int k = ALIGN[e].n[n].kind; return k == NK_EFF || k == NK_INS || k == NK_STEP; }   /* bought a point at a time */
static int align_price(int e, int n) { return align_point(e, n) ? 1 : ALIGN[e].n[n].cost; }
static int align_other(int e, int n) { return n >= ND_B1 ? ND_A1 : ND_B1; }   /* the first node of the other path */
static int align_ready(int e, int n) {                             /* may be bought now, points aside */
    if (n == ND_OPEN) return !align_open(e) && !(e == EL_PHYS && phys_locked());
    if (!align_open(e) || align_full(e, n)) return 0;
    switch (n) {
    case ND_EFF: case ND_INS: return 1;
    case ND_ABIL: return align_full(e, ND_EFF) && align_full(e, ND_INS);
    case ND_A1: case ND_B1: return align_full(e, ND_ABIL) && !RUN.al[e][align_other(e, n)];
    default: return align_full(e, n - 1);                          /* the path's nodes in order */
    }
}
static int align_chained(int e, int n) { return n >= ND_A1 && RUN.al[e][align_other(e, n)]; }
static int align_spent(int e) { int t = 0; for (int n = 0; n < NNODE; n++) t += RUN.al[e][n]; return t; }
static int align_funds(int e) { return RUN.al_points + (e == EL_FIRE ? RUN.cls_pts : 0); }   /* the Witchfinder's go only into Fire */
static int align_refund(int e) {                                   /* what unbinding gives back: half of what was paid */
    return (align_spent(e) - ((RUN.al_gift >> e & 1) || (RUN.gifted >> e & 1) ? ALIGN[e].n[ND_OPEN].cost : 0) - (e == EL_FIRE ? RUN.cls_spent : 0)) / 2;
}
static void align_buy(int e, int n) {
    int price = align_price(e, n), given = e == EL_FIRE ? imin(RUN.cls_pts, price) : 0;
    RUN.al[e][n] = (uint8_t)imin(ALIGN[e].n[n].cost, RUN.al[e][n] + (align_point(e, n) ? 1 : ALIGN[e].n[n].cost));
    RUN.cls_pts -= given; RUN.cls_spent += given;
    RUN.al_points -= price - given;
    if (n == ND_OPEN && e == EL_PHYS) RUN.gear.bones = (int16_t)imin(bones_max(), RUN.gear.bones + SPEAR_BONES);   /* the spear comes with bones */
    caster_sync();
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT align_buy %s %s %d/%d points %d by %s t=%.2f\n", ALIGN[e].name, ALIGN[e].n[n].name, RUN.al[e][n], ALIGN[e].n[n].cost,
            RUN.al_points, G.player ? "player" : "autopilot", G.t);
#endif
}
/* a gift taken: the line's opening, free */
static void gift_take(int e) {
    if (align_open(e)) return;
    RUN.al[e][ND_OPEN] = (uint8_t)ALIGN[e].n[ND_OPEN].cost; RUN.gifted |= (uint8_t)(1u << e); RUN.gifts++;
    if (e == EL_PHYS) RUN.gear.bones = (int16_t)imin(bones_max(), RUN.gear.bones + SPEAR_BONES);   /* the spear comes with bones */
    caster_sync();
    G.gift_glow = 2.0f;
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT gift %d %s level %d by %s t=%.2f\n", RUN.gifts, ALIGN[e].name, G.level, G.player ? "player" : "autopilot", G.t);
#endif
}
static int gift_closed(void) { int n = 0; for (int e = 0; e < NELEM; e++) n += !align_open(e); return n; }
/* in her room: a gift owed with every line open comes as points; the dream takes its own at once (the line its Alignment
 * would buy next: Fire, then its second, then any), but not on a page it may not choose for */
static void gift_settle(void) {
    while (gift_due() && !gift_closed()) {
        RUN.gifts++; RUN.al_points += ALIGN[EL_FIRE].n[ND_OPEN].cost;
        if (G.player) say("EVERY LINE IS OPEN: THE GIFT COMES AS POINTS");
    }
    if (G.player || LEDGER.yours || LEDGER.kept) return;
#ifdef CRAWL_DEBUG
    if (getenv("CRAWL_NOGIFT")) return;                            /* testing: the gift left for a person */
#endif
    uint32_t h = RUN.run_id * 2654435761u;
    int own = cls_el() >= 0 ? cls_el() : (int)(h % NELEM), second = (own + 1 + (int)((h >> 8) % (NELEM - 1))) % NELEM;   /* as auto_align */
    if (second == EL_FIRE) second = (second + 1) % NELEM;
    if (second == own) second = (own + 2) % NELEM;
    while (gift_due() && gift_closed()) {
        int e = !align_open(EL_FIRE) ? EL_FIRE : !align_open(second) ? second : -1;
        for (int k = 0; e < 0 && k < NELEM; k++) { int c = (int)((h >> 12) + (uint32_t)k) % NELEM; if (!align_open(c)) e = c; }
        gift_take(e);
    }
}
static void align_unbind(int e) {                                  /* every node goes; half the points come back, rounded down */
    int back = align_refund(e);
    if (e == EL_FIRE) { if (cls_el() == EL_FIRE) RUN.cls_pts += RUN.cls_spent; RUN.cls_spent = 0; }   /* the Witchfinder's come back whole */
    memset(RUN.al[e], 0, sizeof RUN.al[e]);
    aug_forget(e);                                                 /* and its augments with it (A5) */
    RUN.al_gift &= (uint8_t)~(1u << e);                            /* an old class's line, unbound, closes; */
    if (RUN.gifted >> e & 1) RUN.al[e][ND_OPEN] = (uint8_t)ALIGN[e].n[ND_OPEN].cost;   /* a gift's stays open */
    RUN.al_points += back; RUN.coins -= 2;
    caster_sync();
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT align_unbind %s back %d points %d t=%.2f\n", ALIGN[e].name, back, RUN.al_points, G.t);
#endif
}
#define AL_GIFT 100                         /* al_arm: a gift's line awaiting its second Enter, AL_GIFT + the line */
#define AL_AUG 200                          /* and an augment awaiting its second, AL_AUG + its step x 2 + which */
static int al_aug_step(int e, int n) {                             /* the augment's step a node carries (1.0.0b21), or -1 */
    if (n == ND_OPEN) return AUG_OPEN;
    if (n == ND_ABIL) return AUG_ABIL;
    return (n == ND_AULT || n == ND_BULT) && align_full(e, n) ? AUG_ULT : -1;   /* the ultimate held, whichever path */
}
static const char *const *aug_mark(int e, int t, int w) { return UI_AUGMARK[e == EL_FIRE && w >= 1 && w <= 2 ? t * 2 + w - 1 : 6]; }
static void update_augpair(int e) {                                /* the pair at a step: A/D the one, Enter twice takes it (A5: for good) */
    int t = clampi(G.al_aug - 1, 0, NAUGT - 1);
    if (IN.hit[KEY_ESC] || IN.hit[KEY_SPACE] || IN.hit[KEY_BACK] || IN.hit[KEY_TAB]) { G.al_aug = 0; G.al_arm = -1; snd_sfx_ex(SFX_KEY, 0.6f, 0.5f); return; }
    if (RUN.aug[e][t]) return;                                     /* chosen: there to be looked at */
    int lr = (IN.hit[KEY_D] || IN.hit[KEY_RIGHT] || IN.hit[KEY_S] || IN.hit[KEY_DOWN]) - (IN.hit[KEY_A] || IN.hit[KEY_LEFT] || IN.hit[KEY_W] || IN.hit[KEY_UP]);
    if (lr) { G.al_augs = !G.al_augs; G.al_arm = -1; snd_sfx_ex(SFX_TICK, 0.7f, 0.5f); return; }
    if (!(IN.hit[KEY_ENTER] || IN.hit[KEY_E] || IN.hit[KEY_MOUSE1])) return;
    int w = clampi(G.al_augs, 0, 1);
    if (G.al_arm != AL_AUG + t * 2 + w) { G.al_arm = AL_AUG + t * 2 + w; snd_sfx_ex(SFX_KEY, 0.7f, 0.5f); return; }
    G.al_arm = -1;
    if (!aug_choose(e, t, w + 1)) { snd_sfx_ex(SFX_KNOCK, 0.5f, 0.5f); return; }
    snd_sfx_ex(SFX_RUNE, 0.8f, 0.5f); snd_sfx_ex(SFX_SHIMMER, 0.6f, 0.5f);
    say("%s IS YOURS", aug_name(e, t, w + 1));
}
static void update_align(int view) {
    if (!view && G.al_aug) { update_augpair(clampi(G.al_el, 0, NELEM - 1)); return; }   /* the pair's tablet has the keys while it is up */
    if (!view && (IN.hit[KEY_ESC] || IN.hit[KEY_TAB])) { G.ui = UI_NONE; snd_sfx_ex(SFX_KEY, 0.6f, 0.5f); return; }
    int e = G.al_el = clampi(G.al_el, 0, NELEM - 1), was = e;
    for (int k = 0; k < NELEM; k++) if (IN.hit[KEY_1 + k]) e = k;
    e = (e + NELEM + (IN.hit[KEY_PGDN] - IN.hit[KEY_PGUP])) % NELEM;
    if (e != was) { G.al_el = e; G.al_node = ND_OPEN; G.al_arm = -1; snd_sfx_ex(SFX_TICK, 0.8f, 0.5f); return; }
    int n = G.al_node = clampi(G.al_node, 0, NNODE - 1), row = ND_ROW[n], col = ND_COL[n];
    int ud = (IN.hit[KEY_S] || IN.hit[KEY_DOWN]) - (IN.hit[KEY_W] || IN.hit[KEY_UP]), lr = view ? 0 : (IN.hit[KEY_D] || IN.hit[KEY_RIGHT]) - (IN.hit[KEY_A] || IN.hit[KEY_LEFT]);
    if (ud || lr) {                                                /* to the nearest node that way */
        int best = n, bd = 99;
        for (int m = 0; m < NNODE; m++) {
            int dr = ND_ROW[m] - row, dc = ND_COL[m] - col;
            if ((ud && (dr * ud <= 0 || (view && dr * ud > 1))) || (lr && (dc * lr <= 0 || dr))) continue;
            int d = abs(dr) * 4 + abs(dc) + (ud && ND_COL[m] != col && col != 1 ? 2 : 0);
            if (d < bd) { bd = d; best = m; }
        }
        if (best != n) { G.al_node = best; G.al_arm = -1; snd_sfx_ex(SFX_TICK, 0.6f, 0.5f); }
        return;
    }
    if (view) return;
    if (aug_on() && IN.hit[KEY_SPACE]) {                           /* Space (Y): the step's augments (1.0.0b21, A6) */
        int at = al_aug_step(e, n);
        if (at < 0 || !aug_built(e) || !aug_step_open(e, at)) { snd_sfx_ex(SFX_KNOCK, 0.5f, 0.5f); return; }
        G.al_aug = 1 + at; G.al_augs = RUN.aug[e][at] ? RUN.aug[e][at] - 1 : 0; G.al_arm = -1;
        snd_sfx_ex(SFX_PAGE, 0.7f, 0.5f);
        return;
    }
    if (aug_on() ? IN.hit[KEY_BACK] : IN.hit[KEY_SPACE] || IN.hit[KEY_BACK]) {   /* unbinding, for two coins, asked twice (Back, with the augments) */
        if (!align_spent(e)) { snd_sfx_ex(SFX_KNOCK, 0.5f, 0.5f); return; }
        if (RUN.coins < 2) { G.al_arm = -3; snd_sfx_ex(SFX_KNOCK, 0.5f, 0.5f); return; }
        if (G.al_arm != -2) { G.al_arm = -2; snd_sfx_ex(SFX_KEY, 0.7f, 0.5f); return; }
        G.al_arm = -1; align_unbind(e);
        snd_sfx_ex(SFX_COIN, 0.8f, 0.5f); snd_sfx_ex(SFX_WAIL, 0.4f, 0.5f);
        return;
    }
    if (!(IN.hit[KEY_ENTER] || IN.hit[KEY_E] || IN.hit[KEY_MOUSE1])) return;
    if (gift_due() && n == ND_OPEN && !align_open(e)) {           /* the gift: any closed line, Physical too, asked twice */
        if (G.al_arm != AL_GIFT + e) { G.al_arm = AL_GIFT + e; snd_sfx_ex(SFX_KEY, 0.7f, 0.5f); return; }
        G.al_arm = -1;
        gift_take(e);
        al_buy_t = G.ui_t; al_buy_e = e; al_buy_n = n;
        snd_sfx_ex(SFX_ATONE, 0.6f, 0.5f); snd_sfx_ex(SFX_SHIMMER, 0.6f, 0.5f); snd_sfx_ex(SFX_WHISPER, 0.7f, 0.5f);
        say("%s IS YOURS: A GIFT", ALIGN[e].name);
        return;
    }
    if (!align_ready(e, n) || align_funds(e) < align_price(e, n)) { G.al_arm = -1; snd_sfx_ex(SFX_KNOCK, 0.5f, 0.5f); return; }
    if ((n == ND_A1 || n == ND_B1) && !RUN.al[e][n] && G.al_arm != n) { G.al_arm = n; snd_sfx_ex(SFX_KEY, 0.7f, 0.5f); return; }   /* the other path will close */
    G.al_arm = -1;
    align_buy(e, n);
    al_buy_t = G.ui_t; al_buy_e = e; al_buy_n = n;                 /* its groove fills (render_align) */
    snd_sfx_ex(SFX_RUNE, 0.7f, 0.5f); snd_sfx_ex(SFX_SHIMMER, 0.4f, 0.5f);
}

/* Her question (section 11), WHAT WERE YOU?: A/D or W/S through the six, Enter twice takes the vow. Asked, it must be
 * answered (Esc is the question of leaving); another past costs five Cursed Coins, from Level of Atonement 20 */
#define VOW_COINS 5
#define VOW_ATONE 20
static int class_page;                                             /* her question shows the past (0) or what they bring (1) */
static void update_class(void) {
    if (IN.hit[KEY_SPACE]) { class_page = !class_page; snd_sfx_ex(SFX_PAGE, 0.6f, 0.5f); return; }   /* (Y on a pad) */
    if (IN.hit[KEY_ESC] || IN.hit[KEY_TAB]) {
        if (G.vow_change) { G.ui = UI_NONE; snd_sfx_ex(SFX_KEY, 0.6f, 0.5f); }
        else if (IN.hit[KEY_ESC]) open_ui(UI_QUIT);
        return;
    }
    int d = (IN.hit[KEY_D] || IN.hit[KEY_RIGHT] || IN.hit[KEY_S] || IN.hit[KEY_DOWN]) - (IN.hit[KEY_A] || IN.hit[KEY_LEFT] || IN.hit[KEY_W] || IN.hit[KEY_UP]);
    for (int k = 0; k < NELEM; k++) if (IN.hit[KEY_1 + k]) { G.vow_sel = k; G.vow_arm = -1; snd_sfx_ex(SFX_TICK, 0.8f, 0.5f); }
    if (d) { G.vow_sel = (G.vow_sel + NELEM + d) % NELEM; G.vow_arm = -1; snd_sfx_ex(SFX_TICK, 0.8f, 0.5f); }
    int c = G.vow_sel = clampi(G.vow_sel, 0, NELEM - 1);
    if (!(IN.hit[KEY_ENTER] || IN.hit[KEY_E] || IN.hit[KEY_MOUSE1])) return;
    if (G.vow_change && (c == cls_el() || RUN.coins < VOW_COINS)) { snd_sfx_ex(SFX_KNOCK, 0.5f, 0.5f); G.vow_arm = -1; return; }
    if (!class_offered(c)) { snd_sfx_ex(SFX_KNOCK, 0.5f, 0.5f); G.vow_arm = -1; return; }   /* the Headsman, before the first gift */
    if (G.vow_arm != c) { G.vow_arm = c; snd_sfx_ex(SFX_KEY, 0.7f, 0.5f); return; }
    if (G.vow_change) RUN.coins -= VOW_COINS;
    class_take(c + 1);
    G.ui = UI_NONE; G.vow_arm = -1; G.vow_change = 0;
    snd_sfx_ex(SFX_ATONE, 0.6f, 0.5f); snd_sfx_ex(SFX_WHISPER, 0.7f, 0.5f); sfx_at(SFX_TRILL, CAT_X, CAT_Y, 0.7f); G.cat_slow = 1.4f;
    say("YOU WERE %s", CLASSES[c].name);
}

/* The Dead Bell and the Weeping Font (docs/PLAN-b11.md, section 12), in the room north of hers. The bell: a Cursed Coin
 * corrupts it, once; then souls toll it for the next level attempt, and on a level cleared before, more souls show
 * its secrets. The font: a coin curses it, once; then souls, once a visit, for a tenth more experience on the next
 * level. Every choice is asked twice */
static int bell_price(void) { return 3 * (10 + G.level); }
static int bell_shown_price(void) { return bell_price() / 2; }
static int font_price(void) { return 10 + G.level; }
static int cleared_before(int level) { return level >= 1 && (RUN.rec.cleared >> ((level - 1) % DESCENT_LEN) & 1); }
static int bell_tolled(void) { return RUN.bell_next == G.level; }
static void bell_toll(int shown) {                                 /* the souls spent, the next level marked */
    spend_spirit((float)(shown ? bell_shown_price() : bell_price()));
    RUN.bell_next = G.level; RUN.bell_secrets |= shown;
    snd_sfx_ex(SFX_DEEPCHIME, 0.45f, 0.5f); snd_sfx_ex(SFX_DEEPCHIME, 0.6f, 0.5f); snd_sfx_ex(SFX_WAIL, 0.5f, 0.5f);
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT bell_toll level %d shown %d souls %d by %s t=%.2f\n", G.level, RUN.bell_secrets, RUN.souls, G.player ? "player" : "autopilot", G.t);
#endif
}
static void wfont_drink(void) {                                    /* the Weeping Font's (a boss arena's fonts are font_drink) */
    spend_spirit((float)font_price());
    RUN.font_next = G.level; G.font_drunk = 1;
    snd_sfx_ex(SFX_SPLASH, 0.7f, 0.5f); snd_sfx_ex(SFX_WHISPER, 0.8f, 0.5f);
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT font_drink level %d souls %d by %s t=%.2f\n", G.level, RUN.souls, G.player ? "player" : "autopilot", G.t);
#endif
}
static int hall2_rows(int *row) {                                  /* what the menu offers: 0 curse/corrupt, 1 toll/drink, 2 show, 3 leave */
    int n = 0;
    if (G.h2_obj == 0) {
        if (!RUN.bell_cursed) row[n++] = 0;
        else {
            row[n++] = 1;
            if (cleared_before(G.level)) row[n++] = 2;
        }
    } else row[n++] = RUN.font_cursed ? 1 : 0;
    row[n++] = 3;
    return n;
}
static void update_hall2(void) {
    if (IN.hit[KEY_ESC] || IN.hit[KEY_TAB]) { G.ui = UI_NONE; snd_sfx_ex(SFX_KEY, 0.6f, 0.5f); return; }
    int row[4], n = hall2_rows(row);
    int ud = (IN.hit[KEY_S] || IN.hit[KEY_DOWN]) - (IN.hit[KEY_W] || IN.hit[KEY_UP]);
    if (ud) { G.ui_sel = (G.ui_sel + n + ud) % n; G.h2_arm = -1; snd_sfx_ex(SFX_TICK, 0.6f, 0.5f); }
    int r = row[clampi(G.ui_sel, 0, n - 1)];
    if (!(IN.hit[KEY_ENTER] || IN.hit[KEY_E] || IN.hit[KEY_SPACE] || IN.hit[KEY_MOUSE1])) return;
    if (r == 3) { G.ui = UI_NONE; return; }
    int bell = G.h2_obj == 0, ok;
    if (r == 0) ok = RUN.coins >= 1;
    else if (bell && r == 1) ok = !bell_tolled() && spirit() >= (float)bell_price();
    else if (bell) ok = bell_tolled() && !RUN.bell_secrets && spirit() >= (float)bell_shown_price();
    else ok = !G.font_drunk && RUN.font_next != G.level && spirit() >= (float)font_price();
    if (!ok) { snd_sfx_ex(SFX_KNOCK, 0.5f, 0.5f); G.h2_arm = -1; return; }
    if (G.h2_arm != r) { G.h2_arm = r; snd_sfx_ex(SFX_KEY, 0.7f, 0.5f); return; }
    G.h2_arm = -1;
    if (r == 0) {                                                  /* a coin, and it is cursed for good */
        RUN.coins--;
        if (bell) RUN.bell_cursed = 1; else RUN.font_cursed = 1;
        int dc = bell ? G.bell_deco : G.font_deco;
        if (dc >= 0 && dc < G.ndecos) G.decos[dc].spr = bell ? SP_DBELL_C : SP_FONT_C;
        snd_sfx_ex(SFX_COIN, 0.9f, 0.5f); snd_sfx_ex(SFX_WAIL, 0.6f, 0.5f); snd_sfx_ex(SFX_GROAN, 0.5f, 0.5f);
        say(bell ? "THE BELL GOES BLACK" : "THE FONT FILLS WITH WHAT IT WEEPS");
#ifdef CRAWL_DEBUG
        fprintf(stderr, "EVENT %s t=%.2f\n", bell ? "bell_corrupted" : "font_cursed", G.t);
#endif
        return;
    }
    if (bell) { bell_toll(r == 2); say(r == 2 ? "ITS SECRETS WILL SHOW THEMSELVES" : "THE NEXT LEVEL IS CORRUPTED"); }
    else { wfont_drink(); say("THE NEXT LEVEL WILL TEACH YOU MORE"); }
    G.ui = UI_NONE;
}

static void update_stones(void) {
    if (IN.hit[KEY_ESC] || IN.hit[KEY_TAB]) { G.ui = UI_ASCEND; G.ui_sel = 0; snd_sfx_ex(SFX_KEY, 0.6f, 0.5f); return; }
    update_stones_core();
}

/* Her offering for a Second Wind spent: a Cursed Coin */
static void wind_offer(void) {
    if (!wind_restorable()) { snd_sfx_ex(SFX_KNOCK, 0.5f, 0.5f); return; }
    if (RUN.coins <= 0) { say("YOU HAVE NO CURSED COIN"); snd_sfx_ex(SFX_KNOCK, 0.5f, 0.5f); G.cat_ear = 0.32f; G.cat_ear_side = rnd(2); return; }
    RUN.coins--; RUN.wind_spent = 0;
    snd_sfx_ex(SFX_COIN, 0.9f, 0.5f); snd_sfx_ex(SFX_WAIL, 0.4f, 0.5f); sfx_at(SFX_TRILL, CAT_X, CAT_Y, 1.0f); G.cat_slow = 1.4f;
    say("YOUR SECOND WIND IS RESTORED");
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT wind_restored coins %d t=%.2f\n", RUN.coins, G.t);
#endif
}

/* The caster's screen (docs/PLAN-b9.md, section 4): a row for each line points buy. W/S choose, D or Enter
 * buys a step (a second press when it is at its cap: more is wasted), A or Backspace takes back a step bought
 * since it opened, Tab the cornerstones, Esc leaves. */
static int conf_capped(int cf, int *all);
static void update_confess(void) {                                 /* W/S chooses, D confesses a point (held, ten a second after a
                                                                      breath), A takes one back, Space an overlevel on one at V */
    if (IN.hit[KEY_ESC]) { G.ui = UI_NONE; snd_sfx_ex(SFX_KEY, 0.6f, 0.5f); return; }
    if (IN.hit[KEY_TAB]) { open_ui(UI_STONES); return; }
    int ud = (IN.hit[KEY_S] || IN.hit[KEY_DOWN]) - (IN.hit[KEY_W] || IN.hit[KEY_UP]);
    if (ud) { G.ui_sel = (G.ui_sel + NCONF + ud) % NCONF; G.cf_arm = -1; G.cf_rep0 = -1.0f; snd_sfx_ex(SFX_TICK, 0.6f, 0.5f); }
    int cf = G.ui_sel = clampi(G.ui_sel, 0, NCONF - 1);
    if (IN.hit[KEY_SPACE] && over_due() > 0) {
        if (RUN.conf[cf] < CONF_TOP) { snd_sfx_ex(SFX_KNOCK, 0.5f, 0.5f); return; }
        if (G.cf_arm != 100 + cf) { G.cf_arm = 100 + cf; snd_sfx_ex(SFX_KEY, 0.7f, 0.5f); return; }
        conf_overlevel(cf, 1); G.cf_arm = -1;
        snd_sfx_ex(SFX_ATONE, 0.7f, 0.5f); snd_sfx_ex(SFX_WHISPER, 0.6f, 0.5f);
        return;
    }
    int fwd = IN.hit[KEY_D] || IN.hit[KEY_RIGHT] || IN.hit[KEY_ENTER] || IN.hit[KEY_E] || IN.hit[KEY_MOUSE1];
    int back = IN.hit[KEY_A] || IN.hit[KEY_LEFT] || IN.hit[KEY_BACK];
    int held = IN.down[KEY_D] || IN.down[KEY_RIGHT] ? 1 : IN.down[KEY_A] || IN.down[KEY_LEFT] ? -1 : 0, step = fwd ? 1 : back ? -1 : 0, rep = 0;
    if (step) { G.cf_rep0 = G.ui_t; G.cf_repn = 0; }
    else if (held && G.cf_rep0 >= 0.0f) {                          /* held: a point a tenth of a second after the first breath */
        int want = G.ui_t - G.cf_rep0 > 0.45f ? 1 + (int)((G.ui_t - G.cf_rep0 - 0.45f) / 0.1f) : 0;
        if (want > G.cf_repn) { G.cf_repn = want; step = held; rep = 1; }
    } else if (!held) G.cf_rep0 = -1.0f;
    if (step > 0) {
        if (RUN.points <= 0 || RUN.conf[cf] >= CONF_TOP) { if (!rep) snd_sfx_ex(SFX_KNOCK, 0.5f, 0.5f); return; }
        int all = 0;
        conf_capped(cf, &all);
        if (all && G.cf_arm != cf) { if (!rep) { G.cf_arm = cf; snd_sfx_ex(SFX_KNOCK, 0.7f, 0.5f); } G.cf_rep0 = -1.0f; return; }   /* its note warns; again confesses */
        int r0 = conf_rank(cf);
        conf_buy(cf, 1); G.cf_buy[cf]++; G.cf_arm = all ? cf : -1;
        snd_sfx_ex(SFX_TICK, rep ? 0.5f : 0.9f, 0.5f);
        if (conf_rank(cf) != r0) { snd_sfx_ex(SFX_RUNE, 0.6f, 0.5f); snd_sfx_ex(SFX_WHISPER, 0.5f, 0.5f); }   /* a rank whole */
    } else if (step < 0) {
        if (G.cf_buy[cf] <= 0) { if (!rep) snd_sfx_ex(SFX_KNOCK, 0.5f, 0.5f); return; }
        G.cf_buy[cf]--; RUN.conf[cf]--; RUN.points++; G.cf_arm = -1;
        caster_sync(); G.max_hp = gear_max_hp(&RUN.gear); G.hp = imin(G.hp, G.max_hp);
        snd_sfx_ex(SFX_TICK, rep ? 0.4f : 0.6f, 0.5f);
    }
}
static void update_ascend(void) {
    if (conf_on()) { update_confess(); return; }
    if (IN.hit[KEY_ESC]) { G.ui = UI_NONE; snd_sfx_ex(SFX_KEY, 0.6f, 0.5f); return; }
    if (IN.hit[KEY_TAB]) { open_ui(UI_STONES); return; }
    int ud = (IN.hit[KEY_S] || IN.hit[KEY_DOWN]) - (IN.hit[KEY_W] || IN.hit[KEY_UP]);
    if (ud) { G.ui_sel = (G.ui_sel + NLINE + ud) % NLINE; G.asc_arm = -1; snd_sfx_ex(SFX_TICK, 0.6f, 0.5f); }
    int l = G.ui_sel = clampi(G.ui_sel, 0, NLINE - 1);
    if (IN.hit[KEY_SPACE] && over_due() > 0) {                     /* an overlevel: past the cap, five more (Luck one), asked twice */
        if (!line_overs(l)) { snd_sfx_ex(SFX_KNOCK, 0.5f, 0.5f); return; }
        if (G.asc_arm != 100 + l) { G.asc_arm = 100 + l; snd_sfx_ex(SFX_KEY, 0.7f, 0.5f); return; }
        RUN.over[l]++; RUN.al_points++; G.asc_arm = -1; caster_sync();
        snd_sfx_ex(SFX_ATONE, 0.7f, 0.5f); snd_sfx_ex(SFX_WHISPER, 0.6f, 0.5f);
#ifdef CRAWL_DEBUG
        fprintf(stderr, "EVENT overlevel %s %d t=%.2f\n", LINES[l].name, RUN.over[l], G.t);
#endif
        return;
    }
    if (IN.hit[KEY_D] || IN.hit[KEY_RIGHT] || IN.hit[KEY_ENTER] || IN.hit[KEY_E] || IN.hit[KEY_MOUSE1]) {
        if (RUN.points < LINES[l].cost) { snd_sfx_ex(SFX_KNOCK, 0.5f, 0.5f); return; }
        if (line_capped(l) && G.asc_arm != l) { G.asc_arm = l; snd_sfx_ex(SFX_KNOCK, 0.7f, 0.5f); return; }   /* its note warns; again buys */
        line_buy(l); G.asc_buy[l]++;
        snd_sfx_ex(SFX_TICK, 0.9f, 0.5f); snd_sfx_ex(SFX_RUNE, 0.35f, 0.5f);
#ifdef CRAWL_DEBUG
        fprintf(stderr, "EVENT line_buy %s step %d points %d t=%.2f\n", LINES[l].name, RUN.steps[l], RUN.points, G.t);
#endif
    }
    if ((IN.hit[KEY_A] || IN.hit[KEY_LEFT] || IN.hit[KEY_BACK]) && G.asc_buy[l] > 0) {
        G.asc_buy[l]--; RUN.steps[l]--; RUN.points += LINES[l].cost; G.asc_arm = -1;
        caster_sync();
        G.max_hp = gear_max_hp(&RUN.gear); G.hp = imin(G.hp, G.max_hp);
        snd_sfx_ex(SFX_TICK, 0.6f, 0.5f);
    }
}

#define OFFER_SOULS 10
static float offer_xp(void) { return 20.0f * xp_level(); }        /* what ten souls offered bring: most of a secret's, by the level */
static void update_merchantmenu(void) {
    if (IN.hit[KEY_ESC] || IN.hit[KEY_TAB]) { G.ui = UI_NONE; return; }
    int up = IN.hit[KEY_W] || IN.hit[KEY_UP], down = IN.hit[KEY_S] || IN.hit[KEY_DOWN];
    if (up || down) { G.ui_sel = (G.ui_sel + NMM + down - up) % NMM; snd_sfx_ex(SFX_TICK, 0.6f, 0.5f); }
    G.ui_sel = clampi(G.ui_sel, 0, NMM - 1);
    if (!(IN.hit[KEY_ENTER] || IN.hit[KEY_E] || IN.hit[KEY_SPACE] || IN.hit[KEY_MOUSE1])) return;
    if (G.ui_sel == MM_SIGN) { open_ledger(0); return; }
    if (G.ui_sel == MM_ASCEND) {
        int n = ascend();
        if (n > 0) {
            snd_sfx_ex(SFX_ATONE, 1.0f, 0.5f); sfx_at(SFX_TRILL, CAT_X, CAT_Y, 0.8f); G.cat_trill = 0.6f; G.cat_slow = 1.6f;
            say("LEVEL OF ATONEMENT %d", RUN.atone);
        }
        if (n > 0 || RUN.points > 0 || corner_due() > 0 || over_due() > 0) {
            open_ui(UI_ASCEND);
            if (corner_due() > 0) open_ui(UI_STONES);              /* a cornerstone to choose first; Tab or Esc goes on to the points */
            return;
        }
        snd_sfx_ex(SFX_KNOCK, 0.5f, 0.5f); G.cat_ear = 0.32f; G.cat_ear_side = rnd(2);   /* its note says how far off it is */
        return;
    }
    if (G.ui_sel == MM_WIND) { wind_offer(); return; }
    if (G.ui_sel == MM_OFFER) {                                    /* ten souls for experience, by the level (section 12, S4) */
        if (spirit() < (float)OFFER_SOULS) { snd_sfx_ex(SFX_KNOCK, 0.5f, 0.5f); G.cat_ear = 0.32f; G.cat_ear_side = rnd(2); return; }
        spend_spirit((float)OFFER_SOULS);
        xp_gain(offer_xp(), "AN OFFERING");
        snd_sfx_ex(SFX_WHISPER, 0.9f, 0.5f); sfx_at(SFX_TRILL, CAT_X, CAT_Y, 0.6f); G.cat_slow = 1.2f;
#ifdef CRAWL_DEBUG
        fprintf(stderr, "EVENT offer_souls xp %.0f souls %d by %s t=%.2f\n", offer_xp(), RUN.souls, G.player ? "player" : "autopilot", G.t);
#endif
        return;
    }
    if (G.ui_sel == MM_VOW) {                                      /* what you were; another past, from Atonement 20, for five coins */
        if (RUN.cls && (RUN.atone < VOW_ATONE || RUN.coins < VOW_COINS)) { snd_sfx_ex(SFX_KNOCK, 0.5f, 0.5f); G.cat_ear = 0.32f; G.cat_ear_side = rnd(2); return; }
        G.vow_change = RUN.cls != 0; open_ui(UI_CLASS);
        return;
    }
    if (G.ui_sel == MM_PED) {
        if (RUN.peds >= NPED) { snd_sfx_ex(SFX_KNOCK, 0.5f, 0.5f); return; }
        if (RUN.treasure < ped_price()) { snd_sfx_ex(SFX_KNOCK, 0.5f, 0.5f); G.cat_ear = 0.32f; G.cat_ear_side = rnd(2); return; }
        G.ui = UI_NONE; ped_buy();
        return;
    }
    G.ui = UI_NONE;
    if (G.ui_sel == MM_PAT) { G.cat_slow = 1.4f; sfx_at(SFX_TRILL, CAT_X, CAT_Y, 0.9f); }
}

/* a stone's page: A/D the next stone, W/S what was carried (the spells and runes, each worn piece, the bag's) */
static int stone_rows(const Stone *st) { return 1 + NSLOT + st->bag_size; }
static void update_stonemenu(void) {
    int n = RUN.nstones;
    if (IN.hit[KEY_ESC] || IN.hit[KEY_TAB] || n <= 0) { G.ui = UI_NONE; return; }
    int lr = (IN.hit[KEY_D] || IN.hit[KEY_RIGHT]) - (IN.hit[KEY_A] || IN.hit[KEY_LEFT]);
    if (lr) { G.stone_sel = (G.stone_sel + n + lr) % n; G.ui_sel = 0; snd_sfx_ex(SFX_TICK, 0.8f, 0.5f); }
    G.stone_sel = clampi(G.stone_sel, 0, n - 1);
    int rows = stone_rows(&RUN.stones[G.stone_sel]), ud = (IN.hit[KEY_S] || IN.hit[KEY_DOWN]) - (IN.hit[KEY_W] || IN.hit[KEY_UP]);
    if (ud) { G.ui_sel = (G.ui_sel + rows + ud) % rows; snd_sfx_ex(SFX_TICK, 0.6f, 0.5f); }
    G.ui_sel = clampi(G.ui_sel, 0, rows - 1);
}

/* the L menu: W/S choose, Enter casts, Space the toggle, Esc or L closes */
static void update_wispmenu(void) {
    WispChoice c[WISP_CHOICES];
    int n = wisp_choices(c), rows = n + lantern_held();             /* the lantern's row under the wisps */
    if (IN.hit[KEY_ESC] || IN.hit[KEY_L]) { G.ui = UI_NONE; return; }
    if (IN.hit[KEY_W] || IN.hit[KEY_UP]) { G.ui_sel = (G.ui_sel + rows - 1) % rows; snd_sfx_ex(SFX_TICK, 0.6f, 0.5f); }
    if (IN.hit[KEY_S] || IN.hit[KEY_DOWN]) { G.ui_sel = (G.ui_sel + 1) % rows; snd_sfx_ex(SFX_TICK, 0.6f, 0.5f); }
    G.ui_sel = clampi(G.ui_sel, 0, rows - 1);
    if (G.ui_sel == n) {                                           /* the lantern: Enter shutters or lights it */
        if (IN.hit[KEY_ENTER] || IN.hit[KEY_E] || IN.hit[KEY_MOUSE1] || IN.hit[KEY_SPACE]) lantern_toggle();
        return;
    }
    if (IN.hit[KEY_SPACE]) {
        RUN.wisp_auto ^= 1;
        if (RUN.wisp_auto) { RUN.wisp_kind = c[G.ui_sel].kind; RUN.wisp_color = c[G.ui_sel].color; }
        snd_sfx_ex(SFX_KEY, 0.7f, 0.5f);
    }
    if (IN.hit[KEY_ENTER] || IN.hit[KEY_E] || IN.hit[KEY_MOUSE1]) { G.ui = UI_NONE; cast_choice(&c[G.ui_sel]); }
}

/* the altar's menu: W/S choose, Enter takes the gift, Esc closes */
static void update_altarmenu(void) {
    AltarRow r[3];
    altar_rows(r);
    if (IN.hit[KEY_ESC]) { G.ui = UI_NONE; return; }
    if (IN.hit[KEY_W] || IN.hit[KEY_UP]) { G.ui_sel = (G.ui_sel + 2) % 3; snd_sfx_ex(SFX_TICK, 0.6f, 0.5f); }
    if (IN.hit[KEY_S] || IN.hit[KEY_DOWN]) { G.ui_sel = (G.ui_sel + 1) % 3; snd_sfx_ex(SFX_TICK, 0.6f, 0.5f); }
    G.ui_sel = clampi(G.ui_sel, 0, 2);
    if (!(IN.hit[KEY_ENTER] || IN.hit[KEY_E] || IN.hit[KEY_SPACE] || IN.hit[KEY_MOUSE1])) return;
    if (!r[G.ui_sel].ok) { snd_sfx_ex(SFX_KNOCK, 0.5f, 0.5f); return; }   /* its note says why */
    G.ui = UI_NONE;
    if (G.ui_sel == 0) { if (G.altar_state == ALTAR_DORMANT) altar_wake(); else altar_send(); }
    else if (G.ui_sel == 1) altar_bless(); else altar_anoint();
}

/* the shrine's screen: W/S the actions, A/D their values, Enter; among the slots W/A/S/D and Enter;
 * Esc goes back, and from the actions leaves (asking first if a take would be lost) */
static int shrine_rows(void) { return RUN.shrine ? NSR : 2; }
static void shrine_leave(void) {
    if (G.sh_takes > 0 && shrine_count(1) > 0) { G.sh_mode = SM_LEAVE; G.ui_sel = 0; snd_sfx_ex(SFX_KNOCK, 0.6f, 0.5f); return; }
    G.ui = UI_NONE;
}
static void shrine_slots(int mode, int row) {                      /* into the slots, on one that holds a rune */
    G.sh_mode = mode; G.sh_arm = -1; G.ui_sel = row;
    if (!RUN.slot_def[G.sh_slot]) for (int i = 0; i < SHRINE_SLOTS; i++) if (RUN.slot_def[i]) { G.sh_slot = i; break; }
    snd_sfx_ex(SFX_TICK, 0.6f, 0.5f);
}
static void update_shrinemenu(void) {
    int enter = IN.hit[KEY_ENTER] || IN.hit[KEY_E] || IN.hit[KEY_SPACE] || IN.hit[KEY_MOUSE1];
    int up = IN.hit[KEY_W] || IN.hit[KEY_UP], down = IN.hit[KEY_S] || IN.hit[KEY_DOWN];
    int left = IN.hit[KEY_A] || IN.hit[KEY_LEFT], right = IN.hit[KEY_D] || IN.hit[KEY_RIGHT];
    if (G.sh_mode == SM_PULL) return;                              /* the runes are coming */
    if (G.sh_mode == SM_TAKE || G.sh_mode == SM_LETGO) {
        if (IN.hit[KEY_ESC]) { G.sh_mode = SM_MENU; G.sh_arm = -1; return; }
        int mx = right - left, my = down - up;
        if (mx || my) { G.sh_slot = (G.sh_slot % 3 + mx + 3) % 3 + (G.sh_slot / 3 + my + 3) % 3 * 3; G.sh_arm = -1; snd_sfx_ex(SFX_TICK, 0.6f, 0.5f); }
        if (!enter) return;
        int i = G.sh_slot, k = RUN.slot_def[i] - 1, q = RUN.slot_q[i];
        if (k < 0) { snd_sfx_ex(SFX_KNOCK, 0.5f, 0.5f); return; }
        if (G.sh_mode == SM_TAKE) {
            if (RUN.gear.def[k] >= q && G.sh_arm != i) { G.sh_arm = i; shrine_note("YOURS IS AS GOOD. ENTER AGAIN TAKES IT ANYWAY"); snd_sfx_ex(SFX_KNOCK, 0.7f, 0.5f); return; }
            shrine_take(i);
            if (G.sh_takes <= 0 || !shrine_count(1)) G.sh_mode = SM_MENU;
        } else {
            if (G.sh_arm != i) { G.sh_arm = i; shrine_note("ENTER AGAIN LETS IT GO FOR GOOD"); snd_sfx_ex(SFX_KNOCK, 0.7f, 0.5f); return; }
            shrine_letgo(i);
            if (!shrine_count(1)) G.sh_mode = SM_MENU;
        }
        G.sh_arm = -1;
        return;
    }
    if (G.sh_mode == SM_FULL || G.sh_mode == SM_LEAVE) {             /* a question */
        int full = G.sh_mode == SM_FULL, n = full ? 3 : 2;
        if (IN.hit[KEY_ESC]) { G.sh_mode = SM_MENU; G.ui_sel = full ? SR_PULL : SR_LEAVE; return; }
        if (up || down) { G.ui_sel = (G.ui_sel + n + down - up) % n; snd_sfx_ex(SFX_TICK, 0.6f, 0.5f); }
        G.ui_sel = clampi(G.ui_sel, 0, n - 1);
        if (!enter) return;
        if (full) {
            if (G.ui_sel == 0) shrine_slots(SM_LETGO, SR_PULL);
            else if (G.ui_sel == 1) { shrine_pull(G.sh_coins, G.sh_fav); G.sh_mode = SM_PULL; G.ui_sel = SR_PULL; }
            else { G.sh_mode = SM_MENU; G.ui_sel = SR_PULL; }
        } else if (G.ui_sel == 0) shrine_slots(SM_TAKE, SR_TAKE);
        else G.ui = UI_NONE;
        return;
    }
    int n = shrine_rows();
    if (IN.hit[KEY_ESC]) { shrine_leave(); return; }
    if (up || down) { G.ui_sel = (G.ui_sel + n + down - up) % n; G.sh_arm = -1; snd_sfx_ex(SFX_TICK, 0.6f, 0.5f); }
    G.ui_sel = clampi(G.ui_sel, 0, n - 1);
    if (!RUN.shrine) {                                             /* sanctified: it can only be cursed */
        if (!enter) return;
        if (G.ui_sel == 1) { G.ui = UI_NONE; return; }
        if (RUN.coins < SHRINE_COST[0]) { shrine_note("YOU HAVE NO CURSED COINS"); snd_sfx_ex(SFX_KNOCK, 0.5f, 0.5f); return; }
        shrine_curse(); G.ui_sel = SR_PULL; G.sh_coins = clampi(RUN.coins, 1, 3);
        return;
    }
    if (left || right) {
        if (G.ui_sel == SR_PULL) G.sh_coins = clampi(G.sh_coins + right - left, 1, 3);
        else if (G.ui_sel == SR_FAVOUR) G.sh_fav = fav_step(G.sh_fav, right - left);
        snd_sfx_ex(SFX_TICK, 0.6f, 0.5f);
    }
    if (!enter) return;
    switch (G.ui_sel) {
    case SR_PULL: {
        int cost = G.sh_coins + fav_coins(G.sh_fav);
        if (cost > RUN.coins) { shrine_note("THAT PULL TAKES %d COIN%s; YOU HAVE %d", cost, cost == 1 ? "" : "S", RUN.coins); snd_sfx_ex(SFX_KNOCK, 0.5f, 0.5f); return; }
        if (pull_most(G.sh_coins) > shrine_count(0)) { G.sh_mode = SM_FULL; G.ui_sel = 0; snd_sfx_ex(SFX_KNOCK, 0.6f, 0.5f); return; }
        shrine_pull(G.sh_coins, G.sh_fav); G.sh_mode = SM_PULL;
        return;
    }
    case SR_FAVOUR: G.sh_fav = fav_step(G.sh_fav, 1); snd_sfx_ex(SFX_TICK, 0.6f, 0.5f); return;
    case SR_RAISE:
        if (RUN.shrine >= NQUAL) { shrine_note("IT CAN RISE NO HIGHER"); return; }
        if (RUN.coins < SHRINE_COST[RUN.shrine]) { shrine_note("RAISING IT TAKES %d COINS", SHRINE_COST[RUN.shrine]); snd_sfx_ex(SFX_KNOCK, 0.5f, 0.5f); return; }
        if (G.sh_arm != SR_RAISE) { G.sh_arm = SR_RAISE; shrine_note("ENTER AGAIN SPENDS %d COINS", SHRINE_COST[RUN.shrine]); snd_sfx_ex(SFX_KNOCK, 0.7f, 0.5f); return; }
        shrine_raise(); G.sh_arm = -1;
        return;
    case SR_TAKE:
        if (G.sh_takes <= 0) { shrine_note("PULL FIRST: EACH PULL EARNS ONE TAKE"); snd_sfx_ex(SFX_KNOCK, 0.5f, 0.5f); return; }
        if (!shrine_count(1)) { shrine_note("THE SLOTS ARE EMPTY"); return; }
        shrine_slots(SM_TAKE, SR_TAKE);
        return;
    case SR_LETGO:
        if (!shrine_count(1)) { shrine_note("THE SLOTS ARE EMPTY"); return; }
        shrine_slots(SM_LETGO, SR_LETGO);
        return;
    default: shrine_leave(); return;
    }
}

/* the Totem: W/S a boss, A/D raise or sell, Enter takes (a sale asked twice), Esc closes */
