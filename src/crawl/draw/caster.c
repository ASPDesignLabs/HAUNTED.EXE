/* A part of scene_crawl.c (docs/PLAN-tidy.md): it is #included there in its place and never compiled on its own.
 * Experience and Second Wind shown; the target's health; the cornerstones, the Alignment, the bell and font, the class; the Confessions and ascending. */

/* ---- experience shown (docs/PLAN-b9.md, section 3): each gain a line at the lower left, newest lowest,
 * gone after three seconds; the moment another Level of Atonement is banked, its name across the view ---- */
static void render_xp(Frame *f) {
    if (G.tally || (G.player && G.ui)) return;
    int bar = (G.shop || G.player) ? H - 44 + (int)((1.0f - smoothstepf(0.0f, 1.0f, G.bar)) * 46.0f + 0.5f) : H;
    int dream = !G.player && !G.shop;                              /* the dream: at the lower right, clear of the face */
    int base = dream ? H - 30 - (int)(G.lift * 40.0f + 0.5f) - (runes_shown() ? 6 : 0) : imin(bar, H - 24) - 12 - (sockets_shown() && bar < H ? 14 : 0);
    for (int i = 0; i < XPF.n; i++) {
        float a = XPF.t[i];
        int y = base - i * 10;
        if (y < 40) break;
        char v[16];
        snprintf(v, sizeof v, "+%d EXP", XPF.v[i]);
        int vw = gfx_text_width(v, 1), ww = gfx_text_width(XPF.what[i], 1), dim = a > 2.4f, x = dream ? W - 10 - vw - ww - 6 : 7;
        ui_band(f, x - 3, y - 1, vw + ww + 14, 10);
        gfx_text8(f, x, y, v, dim ? (uint8_t)(R_PURPLE + 9) : (uint8_t)(R_PURPLE + 13), 1);
        gfx_text8(f, x + vw + 6, y, XPF.what[i], dim ? (uint8_t)(R_STONE + 8) : (uint8_t)(R_STONE + 12), 1);
    }
    if (XPF.banner > 0.0f && !G.dead) {                            /* another level banked: she will hear it */
        float k = 3.2f - XPF.banner;
        if (k < 0.5f && ((int)(k * 16.0f) & 1)) return;            /* it flickers in */
        char l1[40];
        snprintf(l1, sizeof l1, "LEVEL OF ATONEMENT %d", XPF.banner_lvl);
        const char *l2 = "THE MERCHANT WILL HEAR IT";
        int w1 = gfx_text_width(l1, 1), w2 = gfx_text_width(l2, 1), wb = imax(w1, w2) + 12, y = 62;
        ui_band(f, (W - wb) / 2, y - 4, wb, 34);
        for (int i = 0; i < wb; i++) { gfx_px(f, (W - wb) / 2 + i, y - 4, R_PURPLE + 8); gfx_px(f, (W - wb) / 2 + i, y + 29, R_PURPLE + 5); }
        int sway = (int)(1.5f * sinf(k * 5.0f));
        gfx_text16(f, (W - w1) / 2 + sway + 1, y + 1, l1, R_PURPLE + 4, 1);
        gfx_text16(f, (W - w1) / 2 + sway, y, l1, (int)(k * 6.0f) % 5 == 0 ? (uint8_t)C_WHITE : (uint8_t)(R_PURPLE + 14), 1);
        gfx_text8(f, (W - w2) / 2, y + 18, l2, R_STONE + 12, 1);
    }
}

/* a Second Wind: a white flash, and its name across the view while nothing can touch you */
static void render_wind(Frame *f) {
    if (G.wind_t <= 0.0f || G.tally) return;
    int lb = lastblow_shown();                                     /* or a killing blow lived (D5): three seconds, and its word */
    float k = (lb ? 3.0f : 2.0f) - G.wind_t;
    if (k < 0.3f) {                                                /* the flash, thinning */
        int lim = (int)(16.0f * (1.0f - k / 0.3f));
        for (int y = 0; y < H; y++) for (int x = 0; x < W; x++) if (BAYER[y & 3][x & 3] < lim) f->pix[y * W + x] = C_WHITE;
    }
    const char *t = lb ? "UNBROKEN" : "SECOND WIND";
    int w = gfx_text_width(t, 2), y = 54 + (int)(4.0f * sinf(k * 3.0f));
    gfx_text16(f, (W - w) / 2 + 2, y + 2, t, R_STONE + 3, 2);
    gfx_text16(f, (W - w) / 2, y, t, (int)(k * 10.0f) & 1 ? (uint8_t)C_WHITE : (uint8_t)(R_GHOST + 14), 2);
}

/* ---- the target's health (docs/PLAN-b9.md, section 2) ----
 * At the top, under a boss's bar if one is up: its name and rank, its health as a bar and in figures, the
 * spell it is learnt to take worst on the left and the one it shrugs off on the right. ? while some spell
 * is still to be learnt on it (three kills each, as the bestiary has it) and none learnt says more; -- when
 * every one is learnt and none differs. A tie takes turns, a second each. */
static int trial_shown(void);
static int target_y(void) { return trial_shown() ? 60 : boss_fighting() ? 30 : 2; }   /* under the hourglass, or a boss's bar */
static int target_shown(void) {
    if (!G.player || G.shop || G.ui || G.dead || G.tally || G.look_t <= 0.0f || G.map_on) return 0;
    const Mob *m = &G.mobs[clampi(G.look_mob, 0, MAXMOBS - 1)];
    return m->alive && !m->ally && !m->boss;
}
/* One side of the target's readout: what it takes more from (more), or less, learnt, the most nearest the
 * bar; up to four. Returns how many, with each element in el[] and its grade in gr[]; *unknown if some
 * element is still to be learnt on this kind */
static float shown_weak(const Mob *m, int e) {                    /* what the readout may say: its kind's, and its rank's
                                                                      strengths only once seen struck (B11) */
    if (!elem_mode() || m->type == MOB_GHOST || (m->type == MOB_WRAITH && m->boss)) return weak_el(m, e);
    float w = kind_weak(m->type, e);
    for (int i = 0; i < 2; i++) if (m->ward[i] == e + 1 && strength_shown(m, i)) w = fminf(w, STRENGTH_K);
    if (m->elem == e + 1) w = fminf(w, STRENGTH_K);                /* born to it: its eyes say so */
    return w;
}
static int weak_side(const Mob *m, int more, int *el, int *gr, int *unknown) {
    int n = 0;
    *unknown = 0;
    for (int e = 0; e < NELEM; e++) {
        if (!el_learnt(m->type, e)) { *unknown = 1; continue; }
        int g = weak_grade(shown_weak(m, e));
        if (more ? g <= 0 : g >= 0) continue;
        int i = n++;
        while (i > 0 && (more ? gr[i - 1] < g : gr[i - 1] > g)) { el[i] = el[i - 1]; gr[i] = gr[i - 1]; i--; }   /* the most first */
        el[i] = e; gr[i] = g;
    }
    return imin(n, 4);
}
static void render_target(Frame *f) {
    if (!target_shown()) return;
    const Mob *m = &G.mobs[clampi(G.look_mob, 0, MAXMOBS - 1)];
    int k = m->type, y0 = target_y(), spirit = KIND_IS(m, KF_SPIRIT) != 0;
    char name[48];
    snprintf(name, sizeof name, "%s%s", m->rank == RK_I || spirit ? "" : rank_word(k, m->rank), KINDS[k].name);
    int nw = gfx_text_width(name, 1);
    ui_band(f, (W - nw) / 2 - 3, y0, nw + 6, 10);
    gfx_text8(f, (W - nw) / 2, y0 + 1, name, spirit ? (uint8_t)(R_STONE + 12) : (uint8_t)RANK_COL[clampi(m->rank, 0, NRANK - 1)], 1);
    if (spirit) {
        const char *t = "NOTHING YOU CARRY TOUCHES IT";
        int tw = gfx_text_width(t, 1);
        ui_band(f, (W - tw) / 2 - 3, y0 + 11, tw + 6, 10);
        gfx_text8(f, (W - tw) / 2, y0 + 12, t, R_STONE + 10, 1);
        return;
    }
    int harms = kind_harms(k, m->rank), nh = 0, hel[NELEM];      /* what it does to you, under its name: up to two */
    for (int e = 0; e < NELEM && nh < 2; e++) if (harms >> e & 1) hel[nh++] = e;
    int by = y0 + (nh ? 22 : 13);
    if (nh) {
        int hw = nh * 7 + (nh - 1) * 5, hx = (W - hw) / 2;
        ui_band(f, hx - 3, y0 + 10, hw + 6, 10);
        for (int i = 0; i < nh; i++) {
            int x = hx + i * 12;
            if (harm_learnt(k, hel[i])) elem_glyph(f, x, y0 + 11, hel[i], 0);
            else gfx_text8(f, x, y0 + 11, "?", R_STONE + 8, 1);       /* a dim ? until it has hurt you with it three times */
        }
    }
    int bw = 120, bx = (W - bw) / 2;
    ui_band(f, bx - 2, by - 2, bw + 4, 11);
    for (int i = -1; i <= bw; i++) { gfx_px(f, bx + i, by - 1, R_STONE + 9); gfx_px(f, bx + i, by + 7, R_STONE + 5); }
    for (int j = -1; j <= 7; j++) { gfx_px(f, bx - 1, by + j, R_STONE + 9); gfx_px(f, bx + bw, by + j, R_STONE + 5); }
    float frac = m->state == MS_WALK && m->maxhp > 0.0f ? clampf(m->hp / m->maxhp, 0.0f, 1.0f) : 0.0f;
    int fill = (int)(frac * bw + 0.5f);
    for (int y = 0; y < 7; y++) for (int x = 0; x < bw; x++) gfx_px(f, bx + x, by + y, x < fill ? (uint8_t)(R_BLOOD + 9 + (y == 0 ? 4 : 0)) : (uint8_t)(R_BLOOD + 2));
    int hp = m->state == MS_WALK ? (int)ceilf(fmaxf(0.0f, m->hp)) : 0, mx = (int)ceilf(m->maxhp);
    int wa = small_digits(NULL, 0, 0, hp, 0), wb = small_digits(NULL, 0, 0, mx, 0), tx = bx + (bw - (wa + 5 + wb)) / 2;
    for (int sh = 1; sh >= 0; sh--) {                              /* 34/60, shadowed */
        uint8_t c = sh ? (uint8_t)0 : (uint8_t)C_WHITE;
        small_digits(f, tx + sh, by + 1 + sh, hp, c);
        static const int SL[5][2] = { { 3, 0 }, { 3, 1 }, { 2, 2 }, { 1, 3 }, { 1, 4 } };
        for (int i = 0; i < 5; i++) gfx_px(f, tx + wa + SL[i][0] + sh, by + 1 + SL[i][1] + sh, c);
        small_digits(f, tx + wa + 5 + sh, by + 1 + sh, mx, c);
    }
    for (int side = 0; side < 2; side++) {                         /* left what it takes more from, right less */
        int el[NELEM], gr[NELEM], unknown, n = weak_side(m, side == 0, el, gr, &unknown), w = unknown ? 8 : 0;
        for (int i = 0; i < n; i++) w += glyph_width(gr[i]) + 3;
        if (!w) continue;
        int x0 = side == 0 ? bx - 6 - w : bx + bw + 6, x = side == 0 ? bx - 6 : x0;
        ui_band(f, x0 - 2, by - 2, w + 3, 11);
        for (int i = 0; i < n; i++) {                              /* outward from the bar, the most first */
            int gw = glyph_width(gr[i]);
            if (side == 0) x -= gw + 3;
            glyph_marked(f, x, by, el[i], gr[i]);
            if (side == 1) x += gw + 3;
        }
        if (unknown) gfx_text8(f, side == 0 ? x0 : x, by, "?", R_STONE + 8, 1);   /* still to be learnt: a dim ? at the outer end */
    }
    for (int w = 0, nb = 0; w < 2; w++) {                          /* its rank's strengths seen: their glyphs barred, under the
                                                                      bar's right end */
        if (!strength_shown(m, w)) continue;
        int x = bx + bw - 9 - nb++ * 11, y = by + 10;
        ui_band(f, x - 2, y - 1, 11, 9);
        elem_glyph(f, x, y, m->ward[w] - 1, 0);
        for (int i = -1; i <= 7; i++) gfx_px(f, x + i, y + 3, C_WHITE);
    }
}

/* ---- the cornerstones' screen ---- */
static const char *const CS_MARK[NCS][11] = {
    { "....###....", "..##...##..", ".#.......#.", "#...###...#", "#..#...#..#", "#..#.#.#..#",
      "#...#..#..#", ".#....#...#", "..####...#.", "........#..", "......##..." },   /* breath, coming back */
    { "...........", "....###....", "..##...##..", ".#..###..#.", "#..#####..#", "#..##.##..#",
      "#..#####..#", ".#..###..#.", "..##...##..", "....###....", "..........." },   /* an eye */
    { ".....#.....", ".....#.....", "....###....", "...#.#.#...", "..#..#..#..", "#####.#####",
      "..#..#..#..", "...#.#.#...", "....###....", ".....#.....", ".....#....." },   /* a compass rose */
    { "....###....", "...#...#...", "..#######..", "..#.###.#..", "..#.###.#..", "..#..#..#..",
      "..#.###.#..", "..#######..", "...#####...", "....###....", "..........." },   /* a lantern */
    { "#....#....#", "...#####...", "..#######..", ".##..#..##.", ".##..#..##.", ".#########.",
      "..##.#.##..", "...#####...", "#...#.#...#", "...........", "....#.#...." },   /* a skull, sensed */
};
/* the cornerstones' stone (docs/PLAN-ui.md section 6.1): five sockets down a carved bow, the pane wide for their words */
static void render_stones(Frame *f, int tabbed) {
    static char right[24], kind[24], cost[32], note[96], warn[160];
    int due = corner_due(), sel = clampi(G.cs_sel, 0, NCS - 1), count = corner_count();
    if (due > 0) snprintf(right, sizeof right, "%d TO CHOOSE", due); else snprintf(right, sizeof right, "%d OF %d HELD", count, NCS);
    ui_screen(f, "CORNERSTONES", R_PURPLE + 13, right, due > 0 && ((int)(G.ui_t * 2.0f) & 1) ? R_PURPLE + 14 : R_PURPLE + 10, 84, G.ui_t);
    static const int CX[NCS] = { 36, 46, 50, 46, 36 }, CY[NCS] = { 46, 74, 102, 130, 158 };
    int st[NCS];
    for (int c = 0; c < NCS; c++) {
        int held = RUN.corner >> c & 1;
        st[c] = held ? SK_HELD : cs_takeable(c) ? SK_READY : (c == CS_CARTO && !RUN.has_map && due > 0) ? SK_SHORT : SK_REACH;
    }
    for (int c = 1; c < NCS; c++)                                  /* the bow's groove, lit through what is held */
        ui_groove(f, CX[c - 1] + (CX[c] > CX[c - 1] ? 3 : -4), CY[c - 1] + 12, CX[c] + (CX[c] > CX[c - 1] ? -3 : 3), CY[c] - 12, R_PURPLE,
                  st[c - 1] == SK_HELD && st[c] == SK_HELD ? 1.0f : 0.0f);
    for (int c = 0; c < NCS; c++) ui_socket_mark(f, CX[c], CY[c], 12, CS_MARK[c], R_PURPLE, st[c], c == sel, 0, G.ui_t);
    int held = RUN.corner >> sel & 1;
    note[0] = 0;
    if (held && sel == CS_WIND && RUN.wind_spent) {
        snprintf(kind, sizeof kind, "HELD, SPENT");
        snprintf(note, sizeof note, "SPENT: IT COMES BACK WITH THE NEXT RUN OF TEN, OR FOR A COIN AT THE MERCHANT'S.");
    } else snprintf(kind, sizeof kind, "%s", held ? "HELD" : st[sel] == SK_READY ? "TO CHOOSE" : "A CORNERSTONE");
    if (held) snprintf(cost, sizeof cost, "YOURS");
    else if (st[sel] == SK_READY) snprintf(cost, sizeof cost, "ENTER CHOOSES IT");
    else if (st[sel] == SK_SHORT) snprintf(cost, sizeof cost, "NEEDS THE MAP");
    else if (count < NCS) snprintf(cost, sizeof cost, "AT ATONEMENT %d", (count + 1) * 10);
    else cost[0] = 0;
    if (!note[0]) snprintf(note, sizeof note, "ONE IS CHOSEN AT EACH TENTH LEVEL OF ATONEMENT; A DEATH TAKES THEM ALL.");
    if (G.cs_arm == sel) snprintf(warn, sizeof warn, "%s, UNTIL A DEATH TAKES IT. ENTER AGAIN TO TAKE IT.", CS_NAME[sel]);
    UiPane pn = { -1, CS_MARK[sel], R_PURPLE, st[sel], 0, CS_NAME[sel], kind, cost[0] ? cost : 0, held ? R_GOLD + 12 : R_GOLD + 10,
                  CS_DOES[sel], 0, 0, note, G.cs_arm == sel ? warn : 0, 0 };
    ui_pane(f, 84, 28, 227, 152, &pn, G.ui_t);
    const char *k = pk(tabbed ? "A/D PAGE  ENTER CHOOSES  TAB CLOSE" : "W/S  ENTER CHOOSES  TAB BACK");
    ui_keys(f, 160 - ui_keys_width(k) / 2, 183, k, UI_M(13));
}

/* the Alignment's screen: the six lines across the top, the chosen line's tree, the node's card at the foot */
/* the Alignment's stone (docs/PLAN-ui.md section 5): the kit lays it, from what the line holds now */
static void render_align(Frame *f, int view) {
    static const char *const KIND[NNODE] = { "OPENING", "PASSIVE", "PASSIVE", "ABILITY", "PATH A", "PATH A", "ULTIMATE",
                                             "PATH B", "PATH B", "ULTIMATE" };
    static char title[40], note[NNODE][56], warn[200], augn[NNODE][80];
    int e = clampi(G.al_el, 0, NELEM - 1), n = clampi(G.al_node, 0, NNODE - 1), funds = align_funds(e);
    UiTree tr;
    memset(&tr, 0, sizeof tr);
    int r = align_rank(e);
    if (r) snprintf(title, sizeof title, "%s  RANK %d", ALIGN[e].name, r); else snprintf(title, sizeof title, "%s  NOT OPEN", ALIGN[e].name);
    tr.title = title; tr.ramp = ELEM_RAMP[e]; tr.elem = e; tr.cls = cls_el();
    for (int k = 0; k < NELEM; k++) { tr.ranks[k] = align_rank(k); tr.ramps[k] = ELEM_RAMP[k]; }
    tr.sel_el = e; tr.points = funds; tr.sel = n; tr.t = G.ui_t;
    for (int m = 0; m < NNODE; m++) {
        const ANode *nd = &ALIGN[e].n[m];
        UiNode *u = &tr.n[m];
        u->name = nd->name; u->does = nd->does; u->kind = KIND[m];
        u->cost = imax(1, nd->cost); u->got = RUN.al[e][m]; u->steps = align_point(e, m);
        note[m][0] = 0;
        int held = m == ND_OPEN ? align_open(e) : align_full(e, m);
        if (held) {
            u->state = SK_HELD; u->got = u->cost;
            if (m == ND_OPEN && RUN.al[e][m] < nd->cost) snprintf(note[m], sizeof note[m], LEDGER.lent == e + 1 ? "LENT, TILL YOU SAY WHAT YOU WERE." : "YOURS BY WHAT YOU WERE.");
        } else if (align_chained(e, m)) { u->state = SK_CLOSED; snprintf(note[m], sizeof note[m], "THE OTHER PATH IS CHOSEN."); }
        else if (m == ND_OPEN && !view && gift_due()) { u->state = SK_READY; u->cost = 0; snprintf(note[m], sizeof note[m], "A GIFT: FREE."); }
        else if (m == ND_OPEN && e == EL_PHYS && phys_locked()) { u->state = SK_CLOSED; snprintf(note[m], sizeof note[m], "SHUT TILL THE LAMPSTAND'S FIRST GIFT."); }
        else if (align_ready(e, m)) {
            int short_by = align_price(e, m) - funds;
            u->state = u->got ? SK_PART : short_by > 0 ? SK_SHORT : SK_READY;
            if (view) snprintf(note[m], sizeof note[m], "BOUGHT AT THE LAMPSTAND.");
            else if (short_by == 1) snprintf(note[m], sizeof note[m], "ONE POINT SHORT.");
            else if (short_by > 1) snprintf(note[m], sizeof note[m], "%d POINTS SHORT.", short_by);
        } else {
            u->state = u->got ? SK_PART : SK_REACH;
            if (!align_open(e)) snprintf(note[m], sizeof note[m], "OPEN %s FIRST.", ALIGN[e].name);
            else if (m == ND_ABIL) snprintf(note[m], sizeof note[m], "NEEDS EFFICIENCY AND INSIGHT, BOTH FULL.");
            else if (m == ND_A1 || m == ND_B1) snprintf(note[m], sizeof note[m], "NEEDS %s.", ALIGN[e].n[ND_ABIL].name);
            else snprintf(note[m], sizeof note[m], "NEEDS %s.", ALIGN[e].n[m - 1].name);
        }
        u->note = note[m][0] ? note[m] : 0;
        int at = al_aug_step(e, m);                                /* 1.0.0b21: its augment's slot, once the step is reached */
        if (at >= 0 && aug_built(e) && aug_step_open(e, at)) {
            int w = RUN.aug[e][at], r = aug_rank(e, at), cr = RUN.aug_cr[e][at];
            static const char *const RN[4] = { "NO RANK YET", "RANK I", "RANK II", "RANK III" };
            u->aug = w ? 2 : 1; u->aug_rank = r; u->aug_mark = w ? aug_mark(e, at, w) : 0;
            static const char *const RV[4] = { "", " I", " II", " III" };
            (void)RN; (void)cr;
            if (!w) snprintf(augn[m], sizeof augn[m], view ? "AN AUGMENT TO CHOOSE." : "%s: ITS AUGMENTS.", pk("SPACE"));
            else snprintf(augn[m], sizeof augn[m], "%s%s", aug_name(e, at, w), RV[r]);
            u->aug_note = augn[m];
        }
        if (e == al_buy_e && m == al_buy_n && G.ui_t - al_buy_t < 0.33f && G.ui_t >= al_buy_t)
            u->grow = fmaxf(0.02f, (G.ui_t - al_buy_t) / 0.33f);   /* the colour runs down its groove */
    }
    if (!view && G.al_arm == -2) snprintf(warn, sizeof warn, "UNBIND %s FOR TWO CURSED COINS: EVERY NODE GOES, AND %d POINTS COME BACK. AGAIN TO UNBIND.", ALIGN[e].name, align_refund(e));
    else if (!view && G.al_arm == -3) snprintf(warn, sizeof warn, "UNBINDING A LINE TAKES TWO CURSED COINS. YOU HAVE %d.", RUN.coins);
    else if (!view && G.al_arm == n) snprintf(warn, sizeof warn, "THE OTHER PATH WILL CLOSE. AGAIN TO TAKE THIS ONE.");
    else if (!view && G.al_arm == AL_GIFT + e) snprintf(warn, sizeof warn, "%s, THE LAMPSTAND'S GIFT. ENTER AGAIN TO TAKE IT.", ALIGN[e].name);
    else if (!view && gift_due()) snprintf(warn, sizeof warn, "THE DEAD ASK YOU TO CARRY ANOTHER: ANY LINE NOT YET OPEN, FREE. ITS OPENING, ENTER TWICE.");
    else warn[0] = 0;
    tr.warn = warn[0] ? warn : 0;
    tr.keys = pk(view ? "A/D PAGE  1-6 LINE  W/S NODE  TAB" : !aug_on() ? "1-6  WASD  ENTER BUYS  SPACE UNBINDS"
                 : al_aug_step(e, n) >= 0 && aug_built(e) && aug_step_open(e, al_aug_step(e, n)) ? "ENTER BUYS  SPACE AUGMENTS"
                 : "1-6  WASD  ENTER BUYS  BACK UNBINDS");      /* a pad's buttons marked; Space where it means something */
    if (!view && G.al_aug) tr.keys = 0;                            /* the pair's tablet has its own */
    ui_tree(f, &tr);
    if (view || !G.al_aug) return;
    int t = clampi(G.al_aug - 1, 0, NAUGT - 1), w = RUN.aug[e][t];   /* the pair at a step (1.0.0b21) */
    static char head[48], aw[160];
    UiAugPair ap;
    memset(&ap, 0, sizeof ap);
    snprintf(head, sizeof head, "%s: %s", ALIGN[e].name, AUG_STEP[t]);
    ap.title = head; ap.sub = w ? "CHOSEN" : "CHOOSE ONE"; ap.ramp = ELEM_RAMP[e]; ap.t = G.ui_t;
    static char does[2][200];                                      /* each at the rank it is at, or would be (III's numbers, unchosen) */
    for (int k = 0; k < 2; k++) {
        aug_does(e, t, k + 1, w == k + 1 ? (aug_rank(e, t) ? aug_k(e, t, k + 1) : 0.5f) : 0.0f, does[k], sizeof does[k]);   /* unranked: I's */
        ap.o[k].name = aug_name(e, t, k + 1); ap.o[k].does = does[k]; ap.o[k].mark = aug_mark(e, t, k + 1);
    }
    ap.sel = w ? w - 1 : clampi(G.al_augs, 0, 1); ap.chosen = w;
    ap.rank = aug_rank(e, t); ap.credits = RUN.aug_cr[e][t]; ap.next = ap.rank < 3 ? AUG_CR[ap.rank] : 0; ap.bank = RUN.aug_bank[e];
    if (G.al_arm == AL_AUG + t * 2 + ap.sel) {
        snprintf(aw, sizeof aw, "AGAIN, AND IT IS YOURS FOR GOOD.");
        ap.warn = aw;
    }
    ap.keys = pk(w ? "SPACE BACK" : "A/D CHOOSE  ENTER TAKES  SPACE BACK");
    ui_check_frame();                                              /* a tablet over the tree: its own words are what fit or don't */
    ui_augpair(f, &ap);
}

/* the bell's or the font's menu: what it is now, its rows and their prices, the second Enter's warning */
static void render_hall2(Frame *f) {
    dim_view(f);
    int row[4], n = hall2_rows(row), bell = G.h2_obj == 0, sel = clampi(G.ui_sel, 0, n - 1);
    int pw = 280, ph = 132, px = (W - pw) / 2, py = 24;
    ui_slab(f, px, py, pw, ph, SLAB_TABLET, ORN_NONE, G.ui_t);       /* a tablet: the rows in a well, what the chosen one does in another */
    ui_well(f, px + 6, py + 25, pw - 12, n * 12 + 5);
    ui_well(f, px + 6, py + 25 + n * 12 + 8, pw - 12, ph - 6 - (25 + n * 12 + 8));
    gfx_text16(f, px + 10, py + 6, bell ? "THE DEAD BELL" : "THE WEEPING FONT", bell ? (uint8_t)(R_MOSS + 12) : (uint8_t)(R_BLOOD + 12), 1);
    char line[160], pr[24];
    snprintf(pr, sizeof pr, "%d.%d SOULS", RUN.souls, clampi((int)(RUN.soul_frac * 10.0f), 0, 9));
    gfx_text8(f, px + pw - 10 - gfx_text_width(pr, 1), py + 10, pr, R_GLOW + 12, 1);
    for (int i = 0; i < n; i++) {
        int y = py + 30 + i * 12, r = row[i];
        const char *l = r == 0 ? (bell ? "CORRUPT IT" : "CURSE IT") : r == 1 ? (bell ? "TOLL IT FOR THE NEXT LEVEL" : "DRINK") : r == 2 ? "AND SHOW ITS SECRETS" : "LEAVE";
        int off = (r == 1 && bell && bell_tolled()) || (r == 2 && (!bell_tolled() || RUN.bell_secrets)) || (r == 1 && !bell && (G.font_drunk || RUN.font_next == G.level));
        ui_row(f, px + 8, y - 3, pw - 16, 12, i == sel);
        gfx_text8(f, px + 14, y, l, off ? (uint8_t)(R_STONE + 8) : i == sel ? (uint8_t)C_WHITE : (uint8_t)(R_STONE + 12), 1);
        if (r == 0) snprintf(pr, sizeof pr, "1 COIN");
        else if (r == 1) snprintf(pr, sizeof pr, "%d", bell ? bell_price() : font_price());
        else if (r == 2) snprintf(pr, sizeof pr, "+%d", bell_shown_price());
        else continue;
        int afford = r == 0 ? RUN.coins >= 1 : spirit() >= (float)(r == 1 ? (bell ? bell_price() : font_price()) : bell_shown_price());
        gfx_text8(f, px + pw - 14 - gfx_text_width(pr, 1), y, pr, off ? (uint8_t)(R_STONE + 8) : afford ? (uint8_t)(R_GOLD + 11) : (uint8_t)(R_BLOOD + 13), 1);
    }
    int ty = py + 25 + n * 12 + 13, r = row[sel];
    if (G.h2_arm == r) snprintf(line, sizeof line, r == 0 ? "A CURSED COIN, AND IT IS SO FOR GOOD. AGAIN TO GIVE IT." : "THE SOULS ARE SPENT FOR GOOD. AGAIN TO GIVE THEM.");
    else if (r == 0 && bell) snprintf(line, sizeof line, "CORRUPTED, IT WILL TAKE SOULS TO CURSE THE LEVEL YOU GO TO NEXT: MORE OF THE DEAD, MORE GOLD, HALF THE EXPERIENCE.");
    else if (r == 0) snprintf(line, sizeof line, "CURSED, IT WILL TAKE SOULS ONCE A VISIT, AND THE NEXT LEVEL WILL TEACH YOU A TENTH MORE.");
    else if (r == 1 && bell) snprintf(line, sizeof line, "LEVEL %d, ONE ATTEMPT: TWO FIFTHS MORE OF THE DEAD AND OF ITS GOLD, HALF ITS EXPERIENCE; A BOSS'S WAVES A THIRD LARGER.", G.level);
    else if (r == 1) snprintf(line, sizeof line, "A TENTH MORE EXPERIENCE ON LEVEL %d.", G.level);
    else if (r == 2) snprintf(line, sizeof line, "YOU HAVE CLEARED IT BEFORE. EVERY SECRET ON IT WILL SHOW ITSELF FROM THE MOMENT IT OPENS.");
    else snprintf(line, sizeof line, "%s", bell ? "IT HANGS STILL." : "IT WEEPS ON.");
    int room = (py + ph - 9 - ty) / 9, need = ui_wrap_count(line, 32);
    if (need > room) ui_overflow("hall2 words", line, need - room);
    text_wrap(f, px + 12, ty, line, 32, G.h2_arm == r ? R_BLOOD + 12 : R_STONE + 12, room);
}

/* her question: the six marks across the top, the chosen one's name, its story and its gifts */
/* her question's stone (docs/PLAN-ui.md section 6.1): the six pasts as medallions across the top, the chosen one's name in
 * the band, its story and its gifts in a well across the stone (too many words for a pane at the side) */
static void render_class(Frame *f) {
    dim_view(f);
    int c = clampi(G.vow_sel, 0, NELEM - 1), mine = cls_el();
    const char *title = G.vow_change ? "ANOTHER PAST" : "WHAT WERE YOU?";
    ui_screen(f, title, R_GOLD + 12, CLASSES[c].name, ramp_at(ELEM_RAMP[c], 13), 0, G.ui_t);
    for (int k = 0; k < NELEM; k++) {
        int state = k == mine ? SK_HELD : !class_offered(k) ? SK_CLOSED : k == c ? (G.vow_change && RUN.coins < VOW_COINS ? SK_SHORT : SK_READY) : SK_REACH;
        ui_socket_mark(f, 35 + k * 50, 46, 13, CLASS_MARK[k], ELEM_RAMP[k], state, k == c, 0, G.ui_t);
    }
    ui_rule(f, 14, 305, 65, 1);
    const char *turn = pk(class_page ? "SPACE THEIR PAST" : "SPACE WHAT THEY BRING");   /* the page's other side (D5) */
    ui_keys(f, 305 - ui_keys_width(turn), 70, turn, UI_M(10));
    int n;
    if (!class_page) {
        n = ui_wrap(f, 14, 81, CLASSES[c].story, 36, 8, 10, FACE_BODY, UI_M(13), TX_INLAID) + 1;
        int need = ui_wrap_count(CLASSES[c].story, 36);
        if (need > 10) ui_overflow("class story", CLASSES[c].name, need - 10);
    } else {                                                       /* the starting sheet (section 9), a row each, and the caps it */
        char nm[40], v[16], cv[16], cn[40];                        /* lifts beside them (docs/BRIEF-caps.md), the rest after; its pair */
        int st, used[3] = { 0, 0, 0 };
        for (n = 0; class_sheet_row(c, n, nm, sizeof nm, v, sizeof v, &st) && n < 10; n++) {
            ui_text8(f, 22, 81 + n * 8, nm, FACE_BODY, UI_M(13), TX_INLAID);
            ui_text8(f, 224 - 8 * (int)strlen(v), 81 + n * 8, v, FACE_BODY, R_GLOW + 12, TX_INLAID);
            for (int j = 0; st >= 0 && j < 3 && class_cap_row(c, j, cn, sizeof cn, cv, sizeof cv); j++)
                if (!used[j] && !strcmp(cn, STAT_NAME[st])) { used[j] = 1; ui_text8(f, 232, 81 + n * 8, cv, FACE_BODY, R_GHOST + 12, TX_INLAID); }
        }
        for (int j = 0; j < 4 && class_cap_row(c, j, cn, sizeof cn, cv, sizeof cv); j++) {
            if (j < 3 && used[j]) continue;
            ui_text8(f, 22, 81 + n * 8, cn, FACE_BODY, UI_M(13), TX_INLAID);
            ui_text8(f, 232, 81 + n * 8, cv, FACE_BODY, R_GHOST + 12, TX_INLAID);
            n++;
        }
        snprintf(cn, sizeof cn, "%s HURTS LESS, %s MORE", ELEM_NAME[CLASS_OWN[c].strong], ELEM_NAME[CLASS_OWN[c].weak]);
        ui_text8(f, 22, 81 + n * 8, cn, FACE_BODY, ramp_at(ELEM_RAMP[c], 12), TX_INLAID);
        n += 2;
    }
    int gy = 71 + n * 8 + 5, gl = ui_wrap_count(CLASSES[c].gift, 36);
    if (gy + gl * 8 > 177) { ui_overflow("class gift", CLASSES[c].name, (gy + gl * 8 - 177 + 7) / 8); gy = 177 - gl * 8; }
    ui_wrap(f, 14, gy, CLASSES[c].gift, 36, 8, 3, FACE_BODY, R_GOLD + 11, TX_INLAID);
    const char *note;
    char buf[96];
    int keys = 0;
    if (G.vow_change && c == mine) note = "WHAT YOU ARE NOW";
    else if (!class_offered(c)) note = "SHUT TILL THE LAMPSTAND'S FIRST GIFT";
    else if (G.vow_change && RUN.coins < VOW_COINS) { snprintf(buf, sizeof buf, "FIVE CURSED COINS. YOU HAVE %d", RUN.coins); note = buf; }
    else if (G.vow_arm == c) note = G.vow_change ? "ENTER AGAIN: FIVE COINS, AND THE VOW" : "ENTER AGAIN TO TAKE THE VOW";
    else { note = pk(G.vow_change ? "A/D CHOOSE  ENTER TAKES  ESC KEEPS" : "A/D CHOOSE  ENTER TAKES THE VOW"); keys = 1; }
    if (keys) ui_keys(f, 160 - ui_keys_width(note) / 2, 183, note, UI_M(13));
    else ui_text8(f, 160 - 4 * (int)strlen(pkp(note)), 183, pkp(note), FACE_BODY, G.vow_arm == c ? R_BLOOD + 13 : UI_M(13), TX_INLAID);
}

/* An overlevelled number (docs/PLAN-b9.md, section 7): pale violet, breathing, now and then slipping a
 * pixel or two with a red ghost the other way, the dead's eye beside it in their colour */
static int stat_over(int st) {
    int n = 0;
    for (int l = 0; l < NLINE; l++) if (LINES[l].stat == st) n += RUN.over[l];
    for (int cf = 0; cf < NCONF; cf++) for (int i = 0; i < CONF[cf].n; i++) if (CONF_STAT[cf][i] == st) n += RUN.conf_over[cf];
    return n;
}
static void eerie_text(Frame *f, int x, int y, const char *s) {
    float t = G.ui_t;
    int slip = fmodf(t + (float)x * 0.013f + (float)y * 0.007f, 2.3f) < 0.09f ? 1 + ((int)(t * 40.0f) & 1) : 0;
    if (slip) gfx_text8(f, x - slip, y, s, R_BLOOD + 9, 1);
    gfx_text8(f, x + slip, y, s, (uint8_t)(R_PURPLE + 11 + (int)(2.5f + 2.5f * sinf(t * 1.6f))), 1);
}
static void eerie_eye(Frame *f, int x, int y) {
    static const char *const EYE[5] = { "..####..", ".#....#.", "#..##..#", ".#....#.", "..####.." };
    int open = fmodf(G.ui_t * 0.7f + (float)x * 0.01f, 4.0f) > 0.15f;   /* it blinks */
    for (int r = 0; r < 5; r++)
        for (int c = 0; c < 8; c++)
            if (EYE[r][c] == '#' && (open || r == 2)) gfx_px(f, x + c, y + r, (uint8_t)(r == 2 && c >= 3 && c <= 4 ? R_GHOST + 15 : R_PURPLE + 12));
}

/* ---- the caster's screen: Levels of Atonement and what points buy (docs/PLAN-b9.md, section 4) ---- */
static void line_value(int l, float v, char *buf, int n) {        /* a step's worth or a total, in its units */
    if (l == LN_HEALTH) snprintf(buf, (size_t)n, "%+.0f", v);
    else if (l == LN_LUCK) snprintf(buf, (size_t)n, "%+.1f%%", v);
    else if (l == LN_SPRINT) snprintf(buf, (size_t)n, "%+.1fS", v);
    else snprintf(buf, (size_t)n, "%+.0f%%", v);
}
static void line_total(int l, char *buf, int n, int *capped) {
    const Gear *g = &RUN.gear;
    *capped = line_capped(l);
    if (l == LN_HEALTH) { snprintf(buf, (size_t)n, "%d", gear_max_hp(g)); return; }
    if (l == LN_RESIST) {
        static const int RS[5] = { ST_RES_POISON, ST_RES_BURN, ST_RES_SLOW, ST_RES_SPIRIT, ST_RES_LIGHT };   /* the five: lowest to highest */
        float lo = 1e9f, hi = 0.0f;
        for (int i = 0; i < 5; i++) { lo = fminf(lo, gear_total(g, RS[i])); hi = fmaxf(hi, gear_total(g, RS[i])); }
        if (hi - lo < 0.5f) snprintf(buf, (size_t)n, "%.0f%% EACH", lo); else snprintf(buf, (size_t)n, "%.0f-%.0f%%", lo, hi);
        return;
    }
    int st = LINES[l].stat;
    if (l == LN_LUCK) snprintf(buf, (size_t)n, "%.1f/%.0f%%", gear_total(g, st), gear_stat_cap(st));
    else if (l == LN_SPRINT) snprintf(buf, (size_t)n, "%.1f/%.0fS", SPRINT_BASE + gear_total(g, st), SPRINT_BASE + gear_stat_cap(st));
    else snprintf(buf, (size_t)n, "%.0f/%.0f%%", gear_total(g, st), gear_stat_cap(st));
}
/* ascend's stone (docs/PLAN-ui.md section 6.1): the caster's lines down the left, each its mark, a thumb for the scroll; the
 * pane the chosen line's numbers: what is yours, the total and its cap, one step more */
static const char *const *line_mark(int l) {
    switch (l) {
    case LN_POISON: return ELEM_GLYPH[EL_POISON];
    case LN_BURN: return ELEM_GLYPH[EL_FIRE];
    case LN_SLOW: return ELEM_GLYPH[EL_FROST];
    case LN_SPIRIT: return ELEM_GLYPH[EL_SPIRIT];
    case LN_LIGHT: return ELEM_GLYPH[EL_LIGHT];
    case LN_HEALTH: return UI_LMARK[UI_LM_HEALTH];
    case LN_DAMAGE: return UI_LMARK[UI_LM_DAMAGE];
    case LN_RESIST: return UI_LMARK[UI_LM_RESIST];
    case LN_HASTE: return UI_LMARK[UI_LM_HASTE];
    case LN_DODGE: return UI_LMARK[UI_LM_DODGE];
    case LN_TREASURE: return UI_LMARK[UI_LM_TREASURE];
    case LN_MOVE: return UI_LMARK[UI_LM_MOVE];
    case LN_LUCK: return UI_LMARK[UI_LM_LUCK];
    case LN_ATTUNE: return UI_LMARK[UI_LM_ATTUNE];
    case LN_SPRINT: return UI_LMARK[UI_LM_SPRINT];
    default: return UI_LMARK[UI_LM_PHYSIC];
    }
}
/* ---- the Unholy Confessions' screen (1.0.0b20, docs/PLAN-combat.md section 10): the six down the left, each its rank and
 * a groove that fills to V with the ranks notched in it; the chosen one's numbers on the right, as you have them and as the
 * next rank has them, what it does under them. The class's weight is in the numbers and never named ---- */
enum { CVF_PCT, CVF_PCT1, CVF_SHARE, CVF_CUT, CVF_LESS, CVF_NUM, CVF_SEC, CVF_HEAL, CVF_SOULS, CVF_CHANCE, CVF_CELLS };
typedef struct { const char *lbl; int fmt; int af[3]; } ConfLbl;   /* a word, or up to three statuses' marks (1 + AF_*) */
static const ConfLbl CONF_LBL[NCONF][5] = {
    [CF_RESTORE]  = { { "MENDING", CVF_HEAL, { 0 } }, { "SOULS/S", CVF_SOULS, { 0 } }, { "PHYSIC", CVF_PCT, { 0 } } },
    [CF_AGILITY]  = { { 0, CVF_CUT, { 1 + AF_DOWN, 1 + AF_STUN, 1 + AF_FROZEN } }, { "DODGE", CVF_PCT1, { 0 } },
                      { "MOVEMENT", CVF_PCT1, { 0 } }, { "QUIET", CVF_PCT, { 0 } } },
    [CF_SPIRIT]   = { { "SOULS", CVF_SHARE, { 0 } }, { 0, CVF_CUT, { 1 + AF_UNBOUND } }, { "HASTE", CVF_PCT1, { 0 } },
                      { "COST", CVF_LESS, { 0 } }, { "EFFECT", CVF_PCT, { 0 } } },
    [CF_CONST]    = { { "RECOVERY", CVF_SHARE, { 0 } }, { "SPRINT", CVF_SEC, { 0 } }, { "HEALTH", CVF_NUM, { 0 } },
                      { "REPRIEVE", CVF_CHANCE, { 0 } } },
    [CF_SURVIVAL] = { { 0, CVF_CUT, { 1 + AF_POISON, 1 + AF_CHILL, 1 + AF_FIRE } }, { "TREASURE", CVF_PCT, { 0 } }, { "LUCK", CVF_PCT1, { 0 } } },
    [CF_TOUGH]    = { { "ARMOUR", CVF_NUM, { 0 } }, { "OF WORN", CVF_SHARE, { 0 } }, { "CRACKS", CVF_CELLS, { 0 } } },
};
static const char *const CONF_DOES[NCONF] = {
    "A SPELL TO MEND BY, CHOSEN AS THE OTHERS ARE, PAID IN SOULS AS IT RUNS. POTIONS DO MORE.",
    "KNOCKED DOWN, STUNNED, FROZEN: RARER AND SHORTER. HARDER TO HIT; QUICKER, QUIETER.",
    "MORE SOULS FROM THE DEAD; UNBOUND RARER AND SHORTER; BETTER SPELLS.",
    "MORE HEALTH, A LONGER SPRINT AND A QUICKER BREATH. NOW AND THEN A KILLING BLOW IS LIVED.",
    "POISON, CHILL, FIRE, GRABS, WEBS, BILE AND SAP RARER AND SHORTER. MORE GOLD, MORE LUCK.",
    "MORE ARMOUR, AND MORE FROM WHAT YOU WEAR. A SHUT SECRET'S CRACKS SHOW FROM FURTHER OFF.",
};
static const int AF_LBL_RAMP[NAF] = { R_FLAME, R_GLOW, R_GLOW, R_GOLD, R_MOSS, R_PURPLE, R_BLOOD };   /* as the row over the bar */
static void conf_fmt(int fmt, float v, int open, char *b, int n) {  /* a number, five characters at most */
    if (!open) { snprintf(b, (size_t)n, "--"); return; }
    switch (fmt) {
    case CVF_PCT: snprintf(b, (size_t)n, "+%.0f%%", v); break;
    case CVF_PCT1: if (v < 9.95f) snprintf(b, (size_t)n, "+%.1f%%", v); else snprintf(b, (size_t)n, "+%.0f%%", v); break;
    case CVF_SHARE: snprintf(b, (size_t)n, "+%.0f%%", v * 100.0f); break;
    case CVF_CUT: snprintf(b, (size_t)n, "-%.0f%%", v * 100.0f); break;
    case CVF_LESS: snprintf(b, (size_t)n, "-%.0f%%", v); break;
    case CVF_NUM: if (v < 9.95f) snprintf(b, (size_t)n, "+%.1f", v); else snprintf(b, (size_t)n, "+%.0f", v); break;
    case CVF_SEC: snprintf(b, (size_t)n, "+%.1fS", v); break;
    case CVF_HEAL: snprintf(b, (size_t)n, "%.2f%%", v * 100.0f); break;
    case CVF_SOULS: snprintf(b, (size_t)n, "%.2f", v); break;
    case CVF_CHANCE: snprintf(b, (size_t)n, "%.0f%%", v * 100.0f); break;
    default: snprintf(b, (size_t)n, "%.1f", v); break;
    }
}
static void af_mark1(Frame *f, int x, int y, int k) {              /* a status's mark at its own size, nine square */
    for (int r = 0; r < 9; r++)
        for (int c = 0; c < 9; c++) {
            char ch = UI_AFMARK[k][r][c];
            if (ch == '#') gfx_px(f, x + c, y + r, (uint8_t)(AF_LBL_RAMP[k] + 12));
            else if (ch == 'o') gfx_px(f, x + c, y + r, (uint8_t)(AF_LBL_RAMP[k] + 15));
        }
}
/* a number of a Confession as the caster has it in all (what is worn, the class's own and every Confession), now and at np points
 * (and no overlevels), under its cap; the stats' totals, the rest as they stand. Returns its cap, 0 none */
#define CONF_CAP_K 0.9f                                            /* a status cut's cap, and the killing blow's 0.5 (conf_at) */
static float conf_total(int cf, int i, int np, int no, float *now, float *next) {
    *now = *next = 0.0f;
    if (cf < 0 || cf >= NCONF || i < 0 || i >= CONF[cf].n) return 0.0f;
    int st = CONF_STAT[cf][i];
    float d = conf_at(cf, i, np, no) - conf_value(cf, i);
    if (st < 0) {                                                  /* felt where it is used: as it stands */
        float extra = cf == CF_SPIRIT && i == CV_SOULS ? class_extra(KX_SOULS) : 0.0f;
        *now = conf_value(cf, i) + extra; *next = conf_at(cf, i, np, no) + extra;
        if (cf == CF_TOUGH && i == CV_ARMOUR_WORN) return 0.0f;
        return (cf == CF_AGILITY && i == CV_HARD) || (cf == CF_SPIRIT && i == CV_UNBOUND) || (cf == CF_SURVIVAL && i == CV_SOFT) ? CONF_CAP_K
             : cf == CF_CONST && i == CV_LASTBLOW ? 0.3f : 0.0f;
    }
    const Gear *g = &RUN.gear;
    if (st == ST_ARMOR) d += (conf_at(CF_TOUGH, CV_ARMOUR_WORN, np, no) - conf_value(CF_TOUGH, CV_ARMOUR_WORN)) * gear_total_raw(g, ST_ARMOR);
    float cap = gear_stat_cap(st), up = 0.0f;
    if (cap > 0.0f && no > RUN.conf_over[cf]) up = (conf_at(cf, i, np, no) - conf_at(cf, i, np, 0)) - (conf_value(cf, i) - conf_at(cf, i, RUN.conf[cf], 0));
    float free = gear_total_free(g, st);
    *now = cap > 0.0f ? fminf(free, cap) : free;
    *next = cap > 0.0f ? fminf(free + d, cap + up) : free + d;
    if (st == ST_HP) { *now += 100.0f; *next += 100.0f; }         /* as health is shown: the whole of it */
    if (st == ST_SPRINT) { *now += SPRINT_BASE; *next += SPRINT_BASE; cap += SPRINT_BASE; }
    return cap > 0.0f ? cap + (st == ST_SPRINT ? 0.0f : 0.0f) : 0.0f;
}
static void conf_tfmt(int cf, int i, float v, char *b, int n) {   /* a total, five characters at most */
    int st = CONF_STAT[cf][i];
    if (st < 0) { conf_fmt(CONF_LBL[cf][i].fmt, v, 1, b, n); return; }
    if (st == ST_HP) snprintf(b, (size_t)n, "%.0f", v);
    else if (st == ST_SPRINT) snprintf(b, (size_t)n, "%.1fS", v);
    else if (st == ST_THRIFT) snprintf(b, (size_t)n, "-%.0f%%", v);
    else if (st == ST_ARMOR) { if (v < 9.95f) snprintf(b, (size_t)n, "%.1f", v); else snprintf(b, (size_t)n, "%.0f", v); }
    else if (v < 9.95f && (st == ST_DODGE || st == ST_MOVE || st == ST_HASTE || st == ST_LUCK)) snprintf(b, (size_t)n, "%.1f%%", v);
    else snprintf(b, (size_t)n, "%.0f%%", v);
}
static int conf_at_cap(float v, float cap) { return cap > 0.0f && v >= cap - 0.05f * (cap < 2.0f ? 0.01f : 1.0f); }
static int conf_capped(int cf, int *all) {                         /* how many of its numbers are at their cap now; *all: every one */
    int n = 0, k = 0;
    for (int i = 0; i < CONF[cf].n; i++) {
        float now, next, cap = conf_total(cf, i, RUN.conf[cf], RUN.conf_over[cf], &now, &next);
        if (cap <= 0.0f) continue;
        k++; n += conf_at_cap(now, cap);
    }
    if (all) *all = k > 0 && n == k && k == CONF[cf].n;
    return n;
}
static void cap_lid(Frame *f, int x, int y, int w) {               /* a lid over a number at its cap */
    for (int i = 0; i < w - 1; i++) gfx_px(f, x + i, y - 2, R_GLOW + 12);
    gfx_px(f, x, y - 1, R_GLOW + 10); gfx_px(f, x + w - 2, y - 1, R_GLOW + 10);
}
static void render_confess(Frame *f) {
    static char title[32], right[24], buf[40];
    dim_view(f);
    int od = over_due(), due = corner_due() + od, sel = clampi(G.ui_sel, 0, NCONF - 1);
    snprintf(title, sizeof title, "LEVEL OF ATONEMENT %d", RUN.atone);
    if (due > 0 && ((int)(G.ui_t * 2.0f) & 1)) snprintf(right, sizeof right, "%d TO CHOOSE", due);
    else snprintf(right, sizeof right, "%d POINT%s", RUN.points, RUN.points == 1 ? "" : "S");
    ui_screen(f, title, R_PURPLE + 13, right, due > 0 && ((int)(G.ui_t * 2.0f) & 1) ? R_PURPLE + 14 : RUN.points ? R_GOLD + 12 : UI_M(9), 136, G.ui_t);
    char a[16], b[16];
    commas(RUN.xp, a, sizeof a); commas(atone_need(RUN.atone + 1), b, sizeof b);
    snprintf(buf, sizeof buf, "EXP %s/%s", a, b);
    if (strlen(buf) > 16) snprintf(buf, sizeof buf, "EXP %s", a);
    ui_text8(f, 14, 32, buf, FACE_BODY, UI_M(11), TX_PLAIN);
    for (int cf = 0; cf < NCONF; cf++) {                           /* the six: name and rank, the groove under them */
        int y = 46 + cf * 22, p = RUN.conf[cf], r = conf_rank(cf);
        ui_row(f, 11, y - 2, 120, 20, cf == sel);
        ui_text8(f, 16, y, CONF[cf].name, FACE_BODY, cf == sel ? C_WHITE : p ? R_PURPLE + 13 : UI_M(12), TX_INLAID);
        const char *num = r > 0 ? ROMAN[imin(r, 15)] : "-";
        if (RUN.conf_over[cf] > 0) eerie_text(f, 127 - 8 * (int)strlen(num), y + 9, num);   /* beside its groove */
        else ui_text8(f, 127 - 8 * (int)strlen(num), y + 9, num, FACE_INSC, r > 0 ? R_GOLD + 11 : UI_M(10), TX_INLAID);
        if (conf_capped(cf, 0)) cap_lid(f, 127 - 8 * (int)strlen(num), y + 9, 8 * (int)strlen(num));   /* something of it at its cap */
        ui_groove(f, 16, y + 11, 93, y + 11, R_PURPLE, (float)p / (float)CONF_TOP);
        for (int k = 1; k < 5; k++) {                              /* the ranks notched in it, gilt once reached */
            int x = 16 + 77 * CONF_CUM[k] / CONF_TOP;
            gfx_px(f, x, y + 9, p >= CONF_CUM[k] ? R_GOLD + 12 : UI_M(13)); gfx_px(f, x, y + 10, p >= CONF_CUM[k] ? R_GOLD + 10 : UI_M(11));
        }
        if (G.cf_buy[cf] > 0) for (int k = 0; k < 2; k++) { gfx_px(f, 125, y + 3 + k, R_PURPLE + 13); gfx_px(f, 126, y + 3 + k, R_PURPLE + 13); }   /* confessed just now */
    }
    /* the pane: its name, its rank and the points into the next, the numbers now and at the next rank, what it does */
    int lx = 142, p = RUN.conf[sel], r = conf_rank(sel), whole = p >= CONF_TOP;
    int nr = whole ? (od > 0 ? r + 1 : 0) : r + 1;                 /* the rank the right-hand column shows: none when whole */
    ui_text8(f, lx, 33, CONF[sel].name, FACE_INSC, p ? R_PURPLE + 14 : UI_M(13), TX_INLAID);
    if (r == 0) snprintf(buf, sizeof buf, p ? "TOWARDS I" : "UNCONFESSED");
    else snprintf(buf, sizeof buf, "RANK %s", ROMAN[imin(r, 15)]);
    ui_text8(f, lx, 44, buf, FACE_BODY, UI_M(11), TX_PLAIN);
    if (!whole) {
        int k = r + 1;
        snprintf(buf, sizeof buf, "%d/%d", p - CONF_CUM[k - 1], CONF_CUM[k] - CONF_CUM[k - 1]);
    } else snprintf(buf, sizeof buf, "WHOLE");
    ui_text8(f, 305 - 8 * (int)strlen(buf), 44, buf, FACE_BODY, R_GOLD + 10, TX_PLAIN);
    ui_rule(f, lx, 304, 54, 0);
    ui_text8(f, 256 - 24, 58, "NOW", FACE_BODY, UI_M(10), TX_PLAIN);
    if (nr) ui_text8(f, 305 - 8 * (int)strlen(ROMAN[imin(nr, 15)]), 58, ROMAN[imin(nr, 15)], FACE_INSC, R_GOLD + 11, TX_INLAID);
    const char *capw = 0, *capn = 0;                               /* the first at its cap now, and the first the next rank would cap */
    for (int i = 0; i < CONF[sel].n; i++) {
        int y = 69 + i * 10;
        const ConfLbl *l = &CONF_LBL[sel][i];
        if (l->lbl) ui_text8(f, lx, y, l->lbl, FACE_BODY, UI_M(13), TX_INLAID);
        else for (int k = 0; k < 3 && l->af[k]; k++) af_mark1(f, lx + k * 11, y - 1, l->af[k] - 1);
        if (!l->lbl && sel == CF_SURVIVAL) ui_text8(f, lx + 33, y, "+", FACE_BODY, UI_M(12), TX_PLAIN);   /* and the rest, as it says below */
        int from1 = CONF[sel].v[i].from1, st = CONF_STAT[sel][i], np = nr <= 5 ? CONF_CUM[imax(nr, 1)] : CONF_TOP, no = nr > 5 ? nr - 5 : 0;
        float now, next, cap = conf_total(sel, i, nr ? np : p, nr ? no : RUN.conf_over[sel], &now, &next);
        int open = st >= 0 || (p > 0 && (!from1 || p >= CONF_CUM[1])), cn = open && conf_at_cap(now, cap);
        if (st >= 0) conf_tfmt(sel, i, now, buf, sizeof buf);      /* a stat: all you have of it; the rest: what this gives */
        else conf_fmt(l->fmt, now, open, buf, sizeof buf);
        int w = 8 * (int)strlen(buf);
        ui_text8(f, 256 - w, y, buf, FACE_BODY, cn ? R_GLOW + 12 : open ? C_WHITE : UI_M(11), TX_PLAIN);
        if (cn) cap_lid(f, 256 - w, y, w);
        if (nr) {
            int cx = conf_at_cap(next, cap);
            if (st >= 0) conf_tfmt(sel, i, next, buf, sizeof buf); else conf_fmt(l->fmt, next, 1, buf, sizeof buf);
            w = 8 * (int)strlen(buf);
            ui_text8(f, 305 - w, y, buf, FACE_BODY, cx ? R_GLOW + 12 : R_GOLD + 11, TX_PLAIN);
            if (cx) cap_lid(f, 305 - w, y, w);
            if (cx && !cn) capn = capn ? capn : l->lbl ? l->lbl : "THE CUT";   /* the next rank would reach it */
        }
        if (cn && !capw) capw = l->lbl ? l->lbl : "THE CUT";
    }
    int dy = 69 + CONF[sel].n * 10 + 5;
    ui_rule(f, lx, 304, dy - 3, 0);
    const char *note = 0;
    if (G.cf_arm == 100 + sel) note = "AN OVERLEVEL TAKES IT PAST V, FOR GOOD. AGAIN TO CONFESS IT.";
    else if (whole && od > 0) note = PADX.used ? "\1Y: AN OVERLEVEL TAKES IT ON." : "SPACE: AN OVERLEVEL TAKES IT ON.";
    if (G.cf_arm == sel) note = "ALL IT GIVES IS AT ITS CAP: MORE IS WASTED. AGAIN TO CONFESS IT ANYWAY.";
    const char *say = note ? pkp(note) : CONF_DOES[sel];           /* an overlevel's word or a warning stands in for what it does */
    int bottom = 178;
    if (!note && (capw || capn)) {                                 /* the caps, at the foot: lids mark them above */
        snprintf(buf, sizeof buf, capw ? "CAPPED: %s" : "%s CAPS %s", capw ? capw : ROMAN[imin(nr, 15)], capw ? "" : capn);
        if (capw) snprintf(buf, sizeof buf, "AT ITS CAP: %s", capw);
        ui_text8(f, lx, 169, buf, FACE_BODY, R_GLOW + 12, TX_INLAID);
        bottom = 167;
    }
    int dl = (bottom - dy) / 9;
    if (ui_wrap_count(say, 20) > dl) ui_overflow("confession", CONF[sel].name, ui_wrap_count(say, 20) - dl);
    ui_wrap(f, lx, dy, say, 20, 9, dl, FACE_BODY, !note ? UI_M(14) : G.cf_arm == 100 + sel || G.cf_arm == sel ? R_BLOOD + 12 : R_PURPLE + 13, TX_INLAID);
    const char *k = PADX.used ? "\1A CONFESSES  LB BACK  VIEW CORNERSTONES" : "D CONFESSES  A BACK  TAB CORNERSTONES";
    ui_keys(f, 160 - ui_keys_width(k) / 2, 183, k, UI_M(13));
}
static void render_ascend(Frame *f) {
    static char title[32], right[24], kind[32], cost[40], extra[96], note[120], warn[120];
    if (conf_on()) { render_confess(f); return; }
    dim_view(f);
    int due = corner_due() + over_due(), od = over_due(), sel = clampi(G.ui_sel, 0, NLINE - 1);
    snprintf(title, sizeof title, "LEVEL OF ATONEMENT %d", RUN.atone);
    if (due > 0 && ((int)(G.ui_t * 2.0f) & 1)) snprintf(right, sizeof right, "%d TO CHOOSE", due);
    else snprintf(right, sizeof right, "%d POINT%s", RUN.points, RUN.points == 1 ? "" : "S");
    ui_screen(f, title, R_PURPLE + 13, right, due > 0 && ((int)(G.ui_t * 2.0f) & 1) ? R_PURPLE + 14 : RUN.points ? R_GOLD + 12 : UI_M(9), 172, G.ui_t);
    char a[16], b[16], line[48];
    commas(RUN.xp, a, sizeof a); commas(atone_need(RUN.atone + 1), b, sizeof b);
    snprintf(line, sizeof line, "EXP %s/%s", a, b);
    if (strlen(line) > 18) snprintf(line, sizeof line, "EXP %s", a);   /* in the millions: the next level's need is in the bar's edge */
    ui_text8(f, 14, 32, line, FACE_BODY, UI_M(11), TX_PLAIN);
    enum { SHOWN = 14 };                                           /* fourteen at a time; the list scrolls (1.0.0b15) */
    int first = clampi(sel - SHOWN / 2, 0, NLINE - SHOWN);
    for (int l = first; l < first + SHOWN; l++) {
        int y = 44 + (l - first) * 9, own = line_own(l) > 0.0f;
        if (l == sel) {                                            /* the chosen row: lit in its groove, gilt at either end */
            for (int yy = y - 1; yy < y + 8; yy++) for (int x = 11; x < 157; x++) if ((x + yy) & 1) gfx_px(f, x, yy, UI_W(6));
            for (int k = 0; k < 3; k++) { gfx_px(f, 11, y + k * 3, R_GOLD + 11); gfx_px(f, 156, y + k * 3, R_GOLD + 11); }
        }
        if (RUN.over[l] > 0) eerie_eye(f, 14, y);                  /* past its cap: the dead see it */
        else ui_mark(f, 14, y, line_mark(l), R_PURPLE, own ? IM_INLAID : IM_CUT, 1);
        ui_text8(f, 25, y, LINES[l].name, FACE_BODY, l == sel ? C_WHITE : own ? R_PURPLE + 13 : UI_M(12), TX_INLAID);
        if (G.asc_buy[l] > 0) for (int k = 0; k < 2; k++) { gfx_px(f, 153, y + 3 + k, R_PURPLE + 13); gfx_px(f, 154, y + 3 + k, R_PURPLE + 13); }   /* bought just now */
    }
    for (int y = 44; y < 44 + SHOWN * 9; y++) { gfx_px(f, 160, y, UI_W(0)); gfx_px(f, 161, y, UI_M(10)); }   /* the scroll's groove and its thumb */
    int th = SHOWN * 9 * SHOWN / NLINE, ty = 44 + (SHOWN * 9 - th) * first / imax(1, NLINE - SHOWN);
    for (int y = ty; y < ty + th; y++) { gfx_px(f, 160, y, UI_M(12)); gfx_px(f, 161, y, UI_M(6)); }
    /* the pane */
    int capped;
    char tot[24], one[24], yours[24];
    line_total(sel, tot, sizeof tot, &capped);
    line_value(sel, line_own(sel), yours, sizeof yours);
    line_value(sel, line_own(sel) + LINES[sel].step, one, sizeof one);
    snprintf(kind, sizeof kind, "COSTS %d", LINES[sel].cost);
    snprintf(cost, sizeof cost, "YOURS %s", yours);
    snprintf(extra, sizeof extra, capped ? "TOTAL %s, AT ITS CAP." : "TOTAL %s; NEXT %s.", tot, one);
    note[0] = 0;
    if (od > 0 && !line_overs(sel)) snprintf(note, sizeof note, "%d OVERLEVEL%s TO CHOOSE, BUT NOT FOR THIS LINE.", od, od == 1 ? "" : "S");
    else if (od > 0) snprintf(note, sizeof note, "%d OVERLEVEL%s TO CHOOSE: %s PUTS %s %s PAST ITS CAP.", od, od == 1 ? "" : "S", PADX.used ? "Y" : "SPACE",
                              sel == LN_LUCK ? "ONE" : "FIVE", LINES[sel].name);
    warn[0] = 0;
    if (G.asc_arm == sel) snprintf(warn, sizeof warn, "IT IS AT ITS CAP: MORE IS WASTED. AGAIN TO BUY IT ANYWAY.");
    else if (G.asc_arm == 100 + sel) snprintf(warn, sizeof warn, "%s PAST ITS CAP, FOR GOOD: %s AGAIN TO OVERLEVEL IT.", LINES[sel].name, PADX.used ? "Y" : "SPACE");
    int afford = RUN.points >= LINES[sel].cost;
    UiPane pn = { -1, RUN.over[sel] > 0 ? UI_LMARK[UI_LM_LUCK] : line_mark(sel), R_PURPLE,
                  capped ? SK_HELD : line_own(sel) > 0.0f ? SK_PART : afford ? SK_READY : SK_SHORT, 0,
                  LINES[sel].name, kind, cost, line_own(sel) > 0.0f ? R_PURPLE + 13 : UI_M(10), LINES[sel].does, extra,
                  capped ? R_GLOW + 12 : UI_M(12), 0, warn[0] ? warn : note[0] ? note : 0, warn[0] ? R_BLOOD + 12 : R_PURPLE + 13 };
    ui_pane(f, 172, 28, 139, 152, &pn, G.ui_t);                    /* an overlevel's word or a warning stands in for what it does, as before */
    const char *k = od > 0 ? (PADX.used ? "\1A BUYS  \1Y OVERLEVELS  \1B LEAVES" : "D BUYS  SPACE OVERLEVELS  ESC LEAVES")
                           : (PADX.used ? "\1A BUYS  LB UNDOES  VIEW CORNERSTONES" : "D BUYS  A UNDOES  TAB CORNERSTONES");
    ui_keys(f, 160 - ui_keys_width(k) / 2, 183, k, UI_M(13));
}

