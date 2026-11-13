/* A part of scene_crawl.c (docs/PLAN-tidy.md): it is #included there in its place and never compiled on its own.
 * The carved stone's hooks; cards (the level's, a boss's, her claim), the web, the boss's bar, the trial's glass, the tally, the readout, the statuses, sockets, the sprint, the view's tint, what is carried and read, messages. */

/* ---- the carved stone (docs/PLAN-ui.md): menus, cards and the tally cut from marble ----
 * A menu lends the frame's floor and ceiling ramps to the marble (ui_lend: the crypt drawn so far keeps its look in
 * the run's stone); a card over the live crypt is cut from the run's stone instead, so nothing behind it changes.
 * While the stone is up every glyph goes through the kit: the faces of our own, inlaid, the run's stone greys turned
 * to the marble's. */
static int ui_lent;
static void ui_hook8(Frame *f, int x, int y, uint8_t ch, int fg, int sc) {
    if (fg >= R_STONE && fg < R_STONE + 16) fg = UI_M(fg - R_STONE);
    ui_glyph8x(f, x, y, ch, FACE_BODY, fg, TX_INLAID, sc);
}
static void ui_hook16(Frame *f, int x, int y, uint8_t ch, int fg, int sc) {
    if (fg >= R_STONE && fg < R_STONE + 16) fg = UI_M(fg - R_STONE);
    ui_glyph16x(f, x, y, ch, FACE_INSC, fg, TX_INLAID, sc);
}
static void ui_on(Frame *f, int marble, const char *where) {
    ui_face_init();
    if (ui_marble_lv != G.level) { ui_marble_make(0x51ABu ^ (unsigned)G.level * 7919u); ui_marble_lv = G.level; }
    if (marble && !ui_lent) { ui_lend(f); ui_lent = 1; }
    ui_ramps(marble && ui_lent);
    ui_where = where;
    gfx_hook8 = ui_hook8; gfx_hook16 = ui_hook16;
}
static void ui_off(void) { gfx_hook8 = 0; gfx_hook16 = 0; ui_box_none(); }

static void card_box(Frame *f, int bw, int bh, int by) {           /* a headstone (docs/PLAN-ui.md section 6.2) */
    ui_slab(f, (W - bw) / 2, by, bw, bh, SLAB_HEAD | SLAB_CALM, ORN_NONE, G.ui_t);
}

static void card_text(Frame *f, int y, const char *s, int c) { gfx_text16(f, (W - gfx_text_width(s, 1)) / 2, y, s, c, 1); }

/* her card, when a page marked in danger was opened */
static void dim_view(Frame *f);
static int text_centred(Frame *f, int y, const char *s, int cols, int col, int maxlines);
static void render_claim_in(Frame *f);
static void render_claim(Frame *f) { ui_on(f, 1, "claim"); render_claim_in(f); ui_off(); }
static void render_claim_in(Frame *f) {
    dim_view(f);
    card_box(f, 292, 128, 32);
    card_text(f, 44, "CAUGHT CHEATING THE DEAD", R_BLOOD + 13);
    text_centred(f, 66, G.claim_text, 34, R_STONE + 13, 7);
    if (G.player && G.claim_t > 1.0f) text_centred(f, 145, pkp("E: SO BE IT"), 34, R_STONE + 9, 1);
}

/* small text centred on the screen, wrapped at word ends; the lines it took */
static int text_centred(Frame *f, int y, const char *s, int cols, int col, int maxlines) {
    int lines = 0;
    while (*s && lines < maxlines) {
        int n = (int)strlen(s);
        if (n > cols) { n = cols; while (n > 0 && s[n] != ' ') n--; if (n == 0) n = cols; }
        char ln[64];
        n = imin(n, (int)sizeof ln - 1);
        memcpy(ln, s, (size_t)n); ln[n] = 0;
        gfx_text8(f, (W - gfx_text_width(ln, 1)) / 2, y + lines * 10, ln, (uint8_t)col, 1);
        lines++;
        s += n; while (*s == ' ') s++;
    }
    return lines;
}

static void render_card_in(Frame *f);
static void render_card(Frame *f) { ui_on(f, 0, "card"); render_card_in(f); ui_off(); }
static void render_card_in(Frame *f) {
    char line[48];
    if (G.dead && G.dcost) {                                       /* a death that costs: what took you, what went with you */
        if (G.dead_t < 1.6 || G.dead_t > 4.6) return;
        char by[64], tk[96];
        blame_text(&G.killer, by, sizeof by);
        card_box(f, 284, 112, 42);
        snprintf(line, sizeof line, "YOU DIED ON LEVEL %d", G.level);
        card_text(f, 54, line, R_BLOOD + 13);
        snprintf(tk, sizeof tk, "TAKEN BY %s", by);
        int y = 76;
        y += text_centred(f, y, tk, 33, C_WHITE, 2) * 10 + 4;
        y += text_centred(f, y, G.dlost, 33, R_STONE + 12, 2) * 10 + 4;
        text_centred(f, y, "A NEW STONE IN THE PLOT", 33, R_GOLD + 11, 1);
        if (G.player && G.dcard_t > 0.8f) text_centred(f, 140, pkp("E: WAKE BY THE MERCHANT"), 33, R_STONE + 9, 1);
        return;
    }
    if (G.dead && G.dead_t > 1.1 && G.dead_t < 4.6) {
        card_box(f, 232, 64, 68);
        card_text(f, 80, "YOU HAVE PERISHED", R_BLOOD + 13);
        snprintf(line, sizeof line, "LEVEL %d BEGINS AGAIN", G.level);
        card_text(f, 104, line, C_WHITE);
        return;
    }
    if (G.boss >= 0 && !PK.depth && G.bcard > 0.3f && G.bcard < 4.6f) {   /* a boss's name, and a line about it */
        const BossDef *B = &BOSSES[G.boss];
        int bw = gfx_text_width(B->name, 1) + 32;
        card_box(f, bw > 250 ? bw : 250, 78, 60);
        card_text(f, 73, B->name, R_BLOOD + 13);
        text_wrap(f, (W - 236) / 2, 98, B->lore, 29, R_STONE + 13, 3);
        return;
    }
    if (G.card < 0.3f || G.card > 3.6f) return;
    char name[48];
    snprintf(line, sizeof line, "LEVEL %d", G.level);
    level_name(G.level, name, sizeof name);
    int bw = gfx_text_width(name, 1) + 32;
    card_box(f, imax(bw, G.cshown ? 244 : 210), 64, G.corrupt || G.fonted ? 78 : 68);
    card_text(f, 80, line, C_WHITE);
    card_text(f, 104, name, R_FLAME + 11);
    if (G.corrupt) {                                               /* the bell's mark by its name */
        const char *c = G.cshown ? "CORRUPTED, ITS SECRETS SHOWN" : "CORRUPTED";
        gfx_text8(f, (W - gfx_text_width(c, 1)) / 2, 121, c, R_MOSS + 13, 1);
    } else if (G.fonted) gfx_text8(f, (W - gfx_text_width("THE FONT'S TENTH", 1)) / 2, 121, "THE FONT'S TENTH", R_BLOOD + 12, 1);
}

/* a boss's bar along the top of the view: its name, its health with a notch at each wave's mark, grey
 * while it is untouchable, and how many of its adds are still standing */
/* held in the web: strands in from the edges, fewer as it tears */
static void render_web(Frame *f) {
    float k = G.web_max > 0.0f ? G.web_hp / G.web_max : 0.0f;
    for (int i = 0; i < 28; i++) {
        if (hash2(i, 77) % 100 >= (uint32_t)(25 + 75 * k)) continue;   /* torn away */
        float t = (float)(hash2(i, 3) % 1000) / 1000.0f;
        int side = i & 3, ex, ey;
        if (side == 0) { ex = (int)(t * W); ey = 0; } else if (side == 1) { ex = (int)(t * W); ey = H - 1; }
        else if (side == 2) { ex = 0; ey = (int)(t * H); } else { ex = W - 1; ey = (int)(t * H); }
        float reach = (0.18f + 0.22f * (float)(hash2(i, 9) % 100) / 100.0f) * (0.5f + 0.5f * k);
        int ix = ex + (int)((W / 2 - ex) * reach), iy = ey + (int)((H / 2 - ey) * reach);
        gfx_line(f, ex, ey, ix, iy, (uint8_t)(R_GHOST + 10 + (i % 4)));
        if (i > 0 && (hash2(i, 5) & 1)) {                          /* a cross thread to the last strand's middle */
            int j = i - 1, s2 = j & 3; float t2 = (float)(hash2(j, 3) % 1000) / 1000.0f;
            int ex2 = s2 == 2 ? 0 : (s2 == 3 ? W - 1 : (int)(t2 * W)), ey2 = s2 == 0 ? 0 : (s2 == 1 ? H - 1 : (int)(t2 * H));
            if (s2 == side) gfx_line(f, (ex + ix) / 2, (ey + iy) / 2, (ex2 + W / 2) / 2 + (ex2 - W / 2) / 3, (ey2 + H / 2) / 2 + (ey2 - H / 2) / 3, (uint8_t)(R_GHOST + 9));
        }
    }
}

static void render_bossbar(Frame *f) {
    if (G.boss < 0 || PK.depth || G.shop || G.bstate < BS_FIGHT || (G.bstate == BS_DEAD && G.bstate_t > 3.0f)) return;
    const BossDef *B = &BOSSES[G.boss];
    int off = G.bstate != BS_FIGHT, bw = 180, bx = (W - bw) / 2, by = 13;
    int nw = gfx_text_width(B->name, 1);
    ui_band(f, (W - nw) / 2 - 3, 2, nw + 6, 10);
    gfx_text8(f, (W - nw) / 2, 3, B->name, off ? (uint8_t)(R_STONE + 12) : (uint8_t)(R_BLOOD + 13), 1);
    ui_band(f, bx - 2, by - 2, bw + 4, 9);
    for (int i = -1; i <= bw; i++) { gfx_px(f, bx + i, by - 1, R_STONE + 9); gfx_px(f, bx + i, by + 5, R_STONE + 5); }
    for (int j = -1; j <= 5; j++) { gfx_px(f, bx - 1, by + j, R_STONE + 9); gfx_px(f, bx + bw, by + j, R_STONE + 5); }
    int fill = (int)(clampf(G.bbar, 0.0f, 1.0f) * bw + 0.5f), ring = G.bring > 0.0f && ((int)(G.t * 20.0) & 1);
    for (int y = 0; y < 5; y++)
        for (int x = 0; x < bw; x++) {
            uint8_t c = x < fill ? (uint8_t)((off ? R_STONE + 7 : R_BLOOD + 10) + (y == 0 ? 4 : 0)) : (uint8_t)(R_BLOOD + 2);
            if (ring && x < fill) c = (uint8_t)(R_STONE + 13);
            gfx_px(f, bx + x, by + y, c);
        }
    for (int w = 0; w < NWAVES; w++) {                             /* the marks: bright until passed */
        int x = bx + (int)(WAVE_MARK[w] * bw + 0.5f);
        for (int y = -1; y <= 5; y++) gfx_px(f, x, by + y, w < G.bwave ? (uint8_t)(R_STONE + 6) : C_WHITE);
    }
    char line[32];
    if (G.bstate == BS_WAVE) {
        if (G.badds < 0) snprintf(line, sizeof line, "IT CALLS"); else snprintf(line, sizeof line, "%d REMAIN", G.badds);
        int lw = gfx_text_width(line, 1);
        ui_band(f, (W - lw) / 2 - 3, by + 7, lw + 6, 10);
        gfx_text8(f, (W - lw) / 2, by + 8, line, R_GOLD + 11, 1);
    }
}

/* a trial's hourglass at the top of the view, where a boss's bar goes: its name over it, how it stands under it; it
 * breaks when won, cracks when the sand runs out, and goes a few seconds after */
static int trial_shown(void) {
    if (G.tri < 0 || PK.depth || G.shop || G.tri_state == TRS_SHUT || (G.tri_state != TRS_ON && G.tri_end > 3.0f)) return 0;
    return G.tri_state != TRS_ON || in_trial((int)G.px, (int)G.py);
}
static void render_trial(Frame *f) {
    if (!trial_shown()) return;
    const char *nm = TRI_NAME[G.tri];
    int nw = gfx_text_width(nm, 1), x0 = W / 2 - 10, y0 = 13;
    ui_band(f, (W - nw) / 2 - 3, 2, nw + 6, 10);
    gfx_text8(f, (W - nw) / 2, 3, nm, G.tri_state == TRS_LOST ? (uint8_t)(R_STONE + 11) : (uint8_t)(R_GOLD + 12), 1);
    ui_band(f, x0 - 2, y0 - 1, 25, 35);
    crawl_hourglass(f->pix, W, x0, y0, G.tri_t / TRI_TIME, G.tri_state == TRS_WON ? 1 : G.tri_state == TRS_LOST ? 2 : 0, (float)G.t);
    char line[40];
    line[0] = 0;
    if (G.tri_state == TRS_WON) snprintf(line, sizeof line, "WON");
    else if (G.tri_state == TRS_LOST) snprintf(line, sizeof line, "THE SAND HAS RUN OUT");
    else switch (G.tri) {
    case TRI_VIGIL: {
        int lit = 0;
        for (int k = 0; k < 3; k++) lit += G.thing[G.tri_a[1 + k]].state;
        snprintf(line, sizeof line, "%d OF 3 BURN", lit);
        break;
    }
    case TRI_RITES: snprintf(line, sizeof line, "%d OF %d ON THE BIER", pile_count(G.tri_a[1]), G.tri_a[2]); break;
    case TRI_TOLL: {
        int rung = 0, bells = 0;
        for (int i = G.tri_n0; i < G.nthing; i++) if (G.thing[i].role == TR_STRIKE) { bells++; rung += G.thing[i].state; }
        snprintf(line, sizeof line, "%d OF %d RUNG", rung, bells);
        break;
    }
    case TRI_SLUICES: case TRI_CAVEIN: case TRI_FIREDAMP: case TRI_LAMPS: case TRI_DARK: {
        int done = 0, n = 0;
        for (int i = G.tri_n0; i < G.nthing; i++) if (G.thing[i].role != TR_READ) { n++; done += G.thing[i].state; }
        static const char *const WORD[NTRI] = { [TRI_SLUICES] = "TURNED", [TRI_CAVEIN] = "BRACED", [TRI_FIREDAMP] = "OPEN", [TRI_LAMPS] = "LIT", [TRI_DARK] = "BURN" };
        snprintf(line, sizeof line, "%d OF %d %s", done, n, WORD[G.tri]);
        break;
    }
    case TRI_SINGING: snprintf(line, sizeof line, "%d OF 6 STRUCK", G.tri_a[3]); break;
    case TRI_COURT: snprintf(line, sizeof line, "THE CROWNED ONE STANDS"); break;
    default: if (tri_alive(0)) snprintf(line, sizeof line, "%d REMAIN", tri_alive(0)); break;
    }
    if (line[0]) {
        int lw = gfx_text_width(line, 1);
        ui_band(f, (W - lw) / 2 - 3, y0 + 35, lw + 6, 10);
        gfx_text8(f, (W - lw) / 2, y0 + 36, line, G.tri_state == TRS_LOST ? (uint8_t)(R_BLOOD + 12) : (uint8_t)(R_GOLD + 11), 1);
    }
}

/* Wolfenstein-style intermission: the numbers run up one row at a time */
static void render_tally_in(Frame *f);
static void render_tally(Frame *f) { ui_on(f, 1, "tally"); render_tally_in(f); ui_off(); }
static void render_tally_in(Frame *f) {
    memset(f->pix, 0, (size_t)W * H);
    float t = (float)G.tally_t;
    ui_slab(f, 4, 4, 312, 192, SLAB_TABLET, ORN_NONE, G.ui_t);       /* a memorial tablet (docs/PLAN-ui.md section 6.2) */
    char line[48];
    snprintf(line, sizeof line, "LEVEL %d COMPLETE!", G.level);
    int flash = t < 0.6f && ((int)(t * 12.0f) & 1), tw = gfx_text_width(line, 2);
    ui_plate(f, (W - tw) / 2 - 8, 6, tw + 16, 36);                   /* its head on a smooth plate */
    gfx_text16(f, (W - tw) / 2, 8, line, flash ? C_WHITE : R_GOLD + 12, 2);
    char name[48];
    level_name(G.level, name, sizeof name);
    card_text(f, 43, name, R_FLAME + 9);                            /* the level's name for its epitaph */
    int top = 62;
    if (G.corrupt) {                                               /* the bell's row (section 12), under the epitaph */
        const char *c = "CORRUPTED: HALF EXPERIENCE";
        gfx_text8(f, (W - gfx_text_width(c, 1)) / 2, 61, c, R_MOSS + 13, 1);
        top = 72;
    }
    static const char *LABEL[TALLY_ROWS] = { "TIME", "KILLS", "TREASURE", "SOULS", "SECRETS", "EXPERIENCE", "BOSS" };
    int totals[TALLY_ROWS] = { 0, G.kills_total, G.items_total, 0, G.nlinks, 0, 0 }, nr = tally_rows();
    char note[TALLY_ROWS][48];                                     /* each row's note, on a line of its own under it */
    int nn = 0;
    for (int r = 0; r < nr; r++) {
        note[r][0] = 0;
        if (r == 5) {                                              /* the bell's half, the font's tenth, the trial's boon or curse */
            const char *a = G.corrupt && G.fonted ? "HALF, +10%" : G.corrupt ? "HALF" : G.fonted ? "+10%" : "";
            const char *b = G.tri_boon > 0 ? "BOON +20%" : G.tri_boon < 0 ? "CURSE -20%" : "";
            snprintf(note[r], sizeof note[r], "%s%s%s", a, *a && *b ? "  " : "", b);
        }
        if (r == 4 && G.trials_failed) snprintf(note[r], sizeof note[r], "%d TRIAL%s FAILED", G.trials_failed, G.trials_failed > 1 ? "S" : "");
        if (r == 2 && G.tri >= 0 && G.tri_state == TRS_WON)       /* the trial's hoard, out of the count (1.0.0b18.1) */
            snprintf(note[r], sizeof note[r], tri_reward_at(IT_CHEST) >= 0 ? "THE TRIAL'S HOARD, LEFT IN ITS ROOM" : "THE TRIAL'S HOARD, TAKEN");
        nn += note[r][0] != 0;
    }
    int pitch = clampi((176 - top - nn * 9) / imax(1, nr), 12, 15), y = top;   /* spaced to fit, closer with more rows and notes */
    for (int r = 0; r < nr; r++) {
        int shown = t >= TALLY_ROW0 + r * TALLY_ROW_GAP - 0.3f, v = G.tally_shown[r];
        if (shown) {
            gfx_text16(f, 30, y, LABEL[r], r == 5 ? (uint8_t)(R_PURPLE + 13) : (uint8_t)C_WHITE, 1);
            if (r == 0 || r == 6) snprintf(line, sizeof line, "%d:%02d", v / 60, v % 60);
            else if (r == 3 || r == 5) snprintf(line, sizeof line, "+%d", v);
            else if (r == 4 && !totals[r]) snprintf(line, sizeof line, "NONE HERE");   /* a drawn level with none (docs/PLAN-tutorial.md) */
            else snprintf(line, sizeof line, "%d / %d", v, totals[r]);
            int done = t >= TALLY_ROW0 + r * TALLY_ROW_GAP + TALLY_COUNT, lw = gfx_text_width(line, 1);
            gfx_text16(f, 290 - lw, y, line, done ? R_GOLD + 12 : R_GOLD + 8, 1);
            int x0 = 30 + gfx_text_width(LABEL[r], 1) + 6, x1 = 290 - lw - 6;   /* the leader: a line cut in the stone */
            for (int x = x0; x < x1; x++) { gfx_px(f, x, y + 11, UI_M(1)); gfx_px(f, x, y + 12, UI_M(11)); }
            if (r == 4 && note[r][0]) gfx_text8(f, 46, y + 13, note[r], R_BLOOD + 12, 1);   /* under the label's foot, each part its colour */
            if (r == 2 && note[r][0]) gfx_text8(f, 46, y + 13, note[r], tri_reward_at(IT_CHEST) >= 0 ? (uint8_t)(R_BLOOD + 12) : (uint8_t)(R_GOLD + 12), 1);
            if (r == 5 && note[r][0]) {
                const char *a = G.corrupt && G.fonted ? "HALF, +10%" : G.corrupt ? "HALF" : G.fonted ? "+10%" : "";
                int nx = 46;
                if (*a) { gfx_text8(f, nx, y + 13, a, G.corrupt ? (uint8_t)(R_MOSS + 12) : (uint8_t)(R_BLOOD + 12), 1); nx += gfx_text_width(a, 1) + 16; }
                if (G.tri_boon) gfx_text8(f, nx, y + 13, G.tri_boon > 0 ? "BOON +20%" : "CURSE -20%", G.tri_boon > 0 ? (uint8_t)(R_GOLD + 12) : (uint8_t)(R_BLOOD + 12), 1);
            }
        }
        y += note[r][0] ? imax(pitch, 13) + 8 : pitch;
    }
    if (t > TALLY_ROW0 + nr * TALLY_ROW_GAP + 0.2f) {
        int perfect = G.items_total > 0 && G.items_got == G.items_total;
        int all_dead = G.kills_total > 0 && G.kills == G.kills_total;
        const char *msg = level_perfect() ? "PERFECT!" : (perfect ? "ALL TREASURE!" : (all_dead ? "NONE LEFT STANDING" : ""));
        if (*msg && ((int)(t * 3.0f) & 1)) card_text(f, imin(y + 2, 174), msg, perfect ? R_GOLD + 12 : R_BLOOD + 13);
    }
}

/* HP, gold, souls and keys: slides up from the bottom edge when something changes, then away */
#define SOCK 19                                                    /* an ability's socket and the gap after it */
static int sockets_shown(void);
static const char *const KEY_ART[5] = { ".##.....", "#..#####", "#..#..#.", ".##...#.", "........" };
static void dead_eye(Frame *f, int x, int y) {                     /* the page is marked: the dead are watching */
    static const char *const EYE[5] = { "..####..", ".#....#.", "#..##..#", ".#....#.", "..####.." };
    int pulse = (int)(G.t * 2.0) & 1;
    for (int r = 0; r < 5; r++)
        for (int c = 0; c < 8; c++)
            if (EYE[r][c] == '#') gfx_px(f, x + c, y + r, (uint8_t)(R_BLOOD + (r == 2 && c >= 3 && c <= 4 ? 15 : 10 + pulse)));
}
static void key_icon(Frame *f, int kx, int ky, int color) {
    static const int RAMP[NKEYS] = { R_GOLD, R_IRON, R_BLOOD };
    for (int y = 0; y < 5; y++)
        for (int x = 0; x < 8; x++)
            if (KEY_ART[y][x] == '#') gfx_px(f, kx + x, ky + y, (uint8_t)(RAMP[color] + (color == 0 ? (y < 2 ? 12 : 9) : (y < 2 ? 14 : 11))));
}
static void render_readout(Frame *f) {
    if (G.readout <= 0.0f || G.dead) return;
    float shown = READOUT_T - G.readout, k = fminf(smoothstepf(0.0f, 0.25f, shown), 1.0f - smoothstepf(0.0f, 0.35f, 0.35f - G.readout));
    if (G.readout > READOUT_T) k = smoothstepf(READOUT_T + 1.5f, READOUT_T + 1.25f, G.readout);
    if (k <= 0.0f) return;
    char num[16], gold[16], souls[16];
    snprintf(gold, sizeof gold, "%d", RUN.treasure);
    spirit_text(souls, sizeof souls);
    int nk = 0;
    for (int c = 0; c < NKEYS; c++) nk += G.has_key[c] > 0;
    int show_souls = RUN.souls > 0 || RUN.gear.atk[AT_BFS] >= 0;
    int gx = 84, sx = gx + 10 + gfx_text_width(gold, 1) + 8;
    int kx = show_souls ? sx + 12 + gfx_text_width(souls, 1) + 8 : sx;
    char coins[12];
    snprintf(coins, sizeof coins, "%d", RUN.coins);
    int ex = kx + nk * 11 + (nk ? 2 : 0), cx = ex + (LEDGER.marked ? 11 : 0);
    int pw = cx + (RUN.coins > 0 ? 10 + gfx_text_width(coins, 1) + 6 : 0) + 4, ph = 18, px = 6 + sockets_shown() * SOCK, py = H - 4 - (int)(k * (ph + 4)) + 4 - (int)(G.lift * 40.0f + 0.5f);   /* raised over HOLD H */
    ui_band(f, px, py, pw, ph);                                    /* a band of the run's stone */
    int low = G.hp <= G.max_hp / 4 && ((int)(G.t * 4.0) & 1);
    uint8_t hcol = G.poison_t > 0.0f ? (uint8_t)(R_MOSS + 13) : (low ? (uint8_t)(R_BLOOD + 15) : (uint8_t)(R_BLOOD + 12));
    gfx_glyph8(f, px + 5, py + 5, 3, hcol, -1, 1);
    snprintf(num, sizeof num, "%3d", G.hp);
    gfx_text8(f, px + 14, py + 5, num, C_WHITE, 1);
    int bx = px + 42, bw = 34, fill = (G.hp * bw + G.max_hp / 2) / G.max_hp;
    for (int y = 0; y < 4; y++)
        for (int x = 0; x < bw; x++) {
            uint8_t c = x < fill ? (uint8_t)((G.poison_t > 0.0f ? R_MOSS : R_BLOOD) + (y == 0 ? 14 : 11)) : (uint8_t)(R_BLOOD + 3);
            if (G.ward_hp > 0.0f && y == 0 && x < (int)(G.ward_hp * bw / 40.0f)) c = (uint8_t)(R_BONE + 14);   /* the ward, a pale line over it */
            gfx_px(f, bx + x, py + 7 + y, c);
        }
    gfx_glyph8(f, px + gx, py + 5, '$', R_GOLD + 12, -1, 1);
    gfx_text8(f, px + gx + 10, py + 5, gold, R_GOLD + 10, 1);
    if (show_souls) {                                              /* a soul: a little wisp */
        int wx = px + sx, wy = py + 4;
        static const char *SOUL[8] = { "..#..", ".###.", "#####", "#####", ".###.", "..##.", "...#.", "....#" };
        for (int y = 0; y < 8; y++) for (int x = 0; x < 5; x++) if (SOUL[y][x] == '#') gfx_px(f, wx + x, wy + y, (uint8_t)(R_GLOW + (y < 4 ? 13 : 9)));
        int ready = RUN.gear.atk[AT_BFS] >= 0 && RUN.souls >= bfs_cost(RUN.gear.atk[AT_BFS]);
        gfx_text8(f, wx + 8, py + 5, souls, ready && ((int)(G.t * 3.0) & 1) ? C_WHITE : (uint8_t)(R_GLOW + 11), 1);
    }
    for (int c = 0, i = 0; c < NKEYS; c++) if (G.has_key[c] > 0) key_icon(f, px + kx + (i++) * 11, py + 6, c);
    if (LEDGER.marked) dead_eye(f, px + ex, py + 6);
    if (RUN.coins > 0) { draw_coin(f, px + cx, py + 5); gfx_text8(f, px + cx + 10, py + 5, coins, R_GOLD + 10, 1); }
}

/* The caster's statuses shown (docs/PLAN-combat.md, section 4.2). The row: a small stone for each, centred over the portrait, just
 * above the bar while it is up, else on a band of its own at the foot (over the readout while that shows). The worst first */
static void render_af_row(Frame *f) {
    static const int ORDER[NAF] = { AF_DOWN, AF_FROZEN, AF_STUN, AF_UNBOUND, AF_CHILL, AF_FIRE, AF_POISON };
    static const int RAMP[NAF] = { R_FLAME, R_GLOW, R_GLOW, R_GOLD, R_MOSS, R_PURPLE, R_BLOOD };   /* by AF_* */
    int on[NAF], n = 0, tall = 0;
    for (int i = 0; i < NAF; i++) if (af_on(ORDER[i])) { on[n++] = ORDER[i]; tall |= ORDER[i] == AF_CHILL; }
    if (!n || G.tally || G.shop) return;
    int w = n * 15 - 2, h = tall ? 17 : 13, x = 182 - w / 2, y;
    int bar = G.player && G.bar > 0.0f;
    if (bar) y = H - 44 + (int)((1.0f - smoothstepf(0.0f, 1.0f, G.bar)) * 46.0f + 0.5f) - h - 2;   /* sits on the bar's top edge */
    else {
        y = H - h - 4 - (G.readout > 0.0f ? 22 : 0);
        ui_band(f, x - 3, y - 2, w + 6, h + 4);
    }
    for (int i = 0; i < n; i++) {
        int k = on[i];
        float left = G.af0[k] > 0.0f ? G.af[k] / G.af0[k] : 0.0f;
        int ramp = k == AF_DOWN && G.af_shake > 0.0f ? R_BONE : RAMP[k];   /* the grapple, bone: no fall */
        ui_status(f, x + i * 15, y + (h - (k == AF_CHILL ? 17 : 13)), UI_AFMARK[k], ramp, left, k == AF_CHILL ? G.af_n : 0, k == AF_CHILL ? 3 : 0, (float)G.t, 1);
    }
}
/* knocked down (section 4.2): the crypt drawn so far leans as you go over, each column slid by its distance from the middle, the
 * gap left black: a roll the raycaster can't draw */
static void af_roll(Frame *f, float k) {
    static uint8_t col[H];
    float lean = 0.16f * k;
    for (int x = 0; x < W; x++) {
        int s = (int)lroundf(((float)x - W * 0.5f) * lean);
        if (!s) continue;
        for (int y = 0; y < H; y++) col[y] = f->pix[y * W + x];
        for (int y = 0; y < H; y++) { int sy = y - s; f->pix[y * W + x] = sy >= 0 && sy < H ? col[sy] : 0; }
    }
}
static uint32_t af_hash(int x, int y) { uint32_t h = (uint32_t)x * 374761393u + (uint32_t)y * 668265263u; h = (h ^ (h >> 13)) * 1274126177u; return h ^ (h >> 16); }
/* What a status does to the view (section 4.2), drawn after the hand: frost from the corners by Chilled's stacks and all round
 * when Frozen, flames licking up the foot when On Fire, sparks wheeling while Stunned, a violet shadow at the edges while
 * Unbound, the dark closing in while knocked down, and a pulse at the edges while grappled */
static void render_af_view(Frame *f) {
    if (G.shop || G.tally) return;
    float tt = (float)G.t;
    float vig = af_on(AF_UNBOUND) ? 0.3f : 0.0f;                   /* the edges darken first: violet, unbound; red, grappled; black, down */
    int vcol = R_PURPLE;
    if (G.af_shake > 0.0f && af_on(AF_DOWN)) { vig = 0.3f + 0.2f * sinf(tt * 12.0f); vcol = R_BLOOD; }
    if (G.af_k > 0.0f) { vig = fmaxf(vig, 0.7f * G.af_k); vcol = 0; }
    if (vig > 0.0f) {
        for (int y = 0; y < H; y++) for (int x = 0; x < W; x++) {
            float dx = fabsf((float)x - W * 0.5f) / (W * 0.5f), dy = fabsf((float)y - H * 0.5f) / (H * 0.5f);
            float d = fmaxf(dx * dx, dy * dy) + 0.4f * (dx * dx + dy * dy);   /* 0 in the middle, 1 to 1.4 at the edges */
            float v = (d - (vcol == R_PURPLE ? 1.15f : 1.05f - 0.45f * vig)) * 2.2f;
            if (v <= 0.0f) continue;
            uint32_t hh = af_hash(x, y);
            if (vcol == R_PURPLE) v = fminf(v, 0.4f);             /* unbound: a shadow, never a wall; grappled, a throb */
            if (vcol == R_BLOOD) v = fminf(v, 0.7f);
            if ((float)(hh & 255) / 255.0f > v) continue;          /* dithered, thicker to the edge */
            f->pix[y * W + x] = vcol ? (uint8_t)(vcol + (vcol == R_PURPLE ? 1 : v > 0.8f ? 2 : 4)) : 0;
        }
    }
    int frozen = af_on(AF_FROZEN), stacks = frozen ? 4 : af_on(AF_CHILL) ? G.af_n : 0;
    if (stacks) {                                                  /* rime, from the corners in; frozen, a crust all round */
        float reach = frozen ? 74.0f : 14.0f + 13.0f * (float)stacks, band = frozen ? 16.0f : 0.0f;
        for (int y = 0; y < H; y++) for (int x = 0; x < W; x++) {
            float dx = (float)(x < W / 2 ? x : W - 1 - x), dy = (float)(y < H / 2 ? y : H - 1 - y);
            float d = sqrtf(dx * dx * 0.6f + dy * dy * 1.4f), e = fminf(dx, dy);
            uint32_t hh = af_hash(x, y);
            float v = fmaxf(reach - d, band - e) + (float)(hh & 15) * 0.6f - 4.5f;
            if (v <= 0.0f) continue;
            if (v < 2.0f) { if (hh & 0x30) f->pix[y * W + x] = (uint8_t)(R_GLOW + 14 - (int)(hh >> 8 & 1)); continue; }   /* its glittering rim */
            if (v < 9.0f && ((x + y) & 1) && (hh & 0x100)) continue;   /* thin, the crypt through it */
            int k = 8 + (int)fminf(5.0f, v * 0.35f) + (int)(hh >> 9 & 1);
            f->pix[y * W + x] = (uint8_t)(R_GLOW + clampi(k, 0, 15));
            if (!(hh & 0x7c00)) f->pix[y * W + x] = C_WHITE;       /* a glint */
        }
    }
    if (af_on(AF_FIRE)) {                                          /* flames along the foot */
        for (int x = 0; x < W; x++) {
            float fh = 7.0f + 6.0f * sinf((float)x * 0.21f + tt * 9.0f) * sinf((float)x * 0.07f - tt * 5.0f) + (float)(af_hash(x, (int)(tt * 20.0f)) & 7);
            for (int j = 0; j < (int)fh; j++) {
                int y = H - 1 - j;
                float k = 1.0f - (float)j / fh;
                if (k < 0.35f && ((x + y + (int)(tt * 30.0f)) & 1)) continue;
                f->pix[y * W + x] = (uint8_t)(R_FLAME + clampi((int)(4.0f + 11.0f * k), 0, 15));
            }
        }
    }
    if (af_on(AF_STUN)) {                                          /* sparks wheeling over your head, trailing */
        for (int i = 0; i < 5; i++) {
            for (int tr = 6; tr >= 0; tr--) {
                float a = tt * 3.2f - (float)tr * 0.09f + (float)i * TAU_F / 5.0f;
                int cx = W / 2 + (int)(cosf(a) * 46.0f), cy = 24 + (int)(sinf(a) * 9.0f);
                if (tr) { if (!(tr & 1)) gfx_px(f, cx, cy, (uint8_t)(R_GOLD + 9 - tr)); continue; }
                int r = sinf(a) > 0.0f ? 4 : 3;                    /* the near ones larger */
                for (int k = -r; k <= r; k++) {
                    uint8_t c = (uint8_t)(R_GOLD + (abs(k) <= 1 ? 12 : 8));
                    gfx_px(f, cx + k, cy, c); gfx_px(f, cx, cy + k, c);
                    if (abs(k) <= r / 2) { gfx_px(f, cx + k, cy + k, (uint8_t)(R_GOLD + 7)); gfx_px(f, cx + k, cy - k, (uint8_t)(R_GOLD + 7)); }
                }
                gfx_px(f, cx, cy, C_WHITE);
            }
        }
    }

}

/* The ability sockets (docs/PLAN-b7.md, section 8), at the bottom left: each unlocked ability's mark, a
 * line along its foot filling as it comes round (bright when ready), a pip over it for each web or risen
 * dead it may have out (lit while one is), the right mouse's one outlined in gold, all greyed and struck
 * through in the void. On the status bar's top edge while it is up; down in the corner when it is away. */
static const char *const BA_MARK[NBOSS][11] = {
    { "...........", "##...##....", ".##...##...", "..##...##..", "...##...##.", "....##...##",
      "...##...##.", "..##...##..", ".##...##...", "##...##....", "..........." },   /* a dart, twice over */
    { "#....#....#", ".#...#...#.", "..#######..", "..##.#.##..", "..#.###.#..", "#####.#####",
      "..#.###.#..", "..##.#.##..", "..#######..", ".#...#...#.", "#....#....#" },   /* a web */
    { "...#####...", "..#######..", ".##..#..##.", ".##..#..##.", ".#########.", "..##.#.##..",
      "...#####...", "....#.#....", "#.#.....#.#", "###########", ".#########." },   /* a skull up out of the earth */
    { "....###....", "...#####...", "..##...##..", "..#.#.#.#..", "..#.....#..", "..#.....#..",
      "..##...##..", ".#.#...#.#.", ".#.#...#.#.", "#..#.#.#..#", "...#.#.#..." },   /* a hooded shape, going thin */
};
static const int BA_RAMP[NBOSS] = { R_GLOW, R_GHOST, R_BONE, R_PURPLE };
static void ab_mark(Frame *f, int x, int y, int b, uint8_t c) {
    for (int r = 0; r < 11; r++) for (int k = 0; k < 11; k++) if (BA_MARK[b][r][k] == '#') gfx_px(f, x + k, y + r, c);
}
static int sockets_shown(void) {                                   /* how many: only for a person, in the crypt */
    int n = 0;
    if (!G.player || G.shop || G.tally || G.dead) return 0;
    for (int b = 0; b < NBOSS; b++) n += RUN.abil[b] > 0;
    return n;
}
/* the sprint's meter (docs/PLAN-ai.md, SP4): a slim bar over the middle of the status bar's top edge, or the screen's
 * foot while it is away, only while it isn't full: dull through the wait, bright as it fills */
static void render_sprint(Frame *f) {
    if (!G.player || G.dead || G.ui || G.tally || G.spr_show <= 0.0f) return;
    int bar_top = H - 44 + (int)((1.0f - smoothstepf(0.0f, 1.0f, G.bar)) * 46.0f + 0.5f);
    int w = 64, x = (W - w) / 2, y = imin(bar_top, H) - 5;
    float k = clampf(G.spr / fmaxf(0.1f, sprint_max()), 0.0f, 1.0f);
    int fill = (int)(k * (float)(w - 2) + 0.5f), waiting = G.spr_wait > 0.0f && !G.spr_on;
    uint8_t lit = G.spr_on ? (uint8_t)(R_BONE + 14) : waiting ? (uint8_t)(R_STONE + 8) : (uint8_t)(R_BONE + 12);
    if (G.spr_show < 0.6f && ((int)(G.spr_show * 20.0f) & 1)) return;   /* full: it flickers out */
    gfx_rect(f, x, y, w, 4, 0);
    for (int i = 0; i < w - 2; i++) {
        gfx_px(f, x + 1 + i, y + 1, i < fill ? lit : (uint8_t)(R_STONE + 3));
        gfx_px(f, x + 1 + i, y + 2, i < fill ? (uint8_t)(lit - 2) : (uint8_t)(R_STONE + 2));
    }
}
static void render_sockets(Frame *f) {
    if (!sockets_shown()) return;
    int bar_top = H - 44 + (int)((1.0f - smoothstepf(0.0f, 1.0f, G.bar)) * 46.0f + 0.5f);
    int y = imin(bar_top - 20, H - 21), x = 4, sel = ab_readied(), hush = no_cast();   /* the void, or a status: struck through */
    for (int b = 0; b < NBOSS; b++) {
        if (!RUN.abil[b]) continue;
        float wait = ab_wait(b), k = wait > 0.0f ? 1.0f - clampf(G.ab_cd[b] / wait, 0.0f, 1.0f) : 1.0f;
        int ready = k >= 1.0f, ramp = BA_RAMP[b];
        if (b == sel && ab_on_mouse(b)) gfx_rect(f, x - 1, y - 1, 19, 19, (uint8_t)(R_GOLD + 11));   /* the right mouse's */
        bevel(f, x, y, 17, 17, 0);
        uint8_t mc = hush ? (uint8_t)(R_STONE + 7) : (ready ? ramp_at(ramp, 14) : ramp_at(ramp, 8));
        ab_mark(f, x + 3, y + 2, b, mc);
        for (int i = 0; i < 13; i++)                               /* coming round */
            gfx_px(f, x + 2 + i, y + 14, i < (int)(k * 13.0f + 0.5f) ? (ready && !hush ? (uint8_t)(R_GOLD + 12) : (uint8_t)(R_STONE + 9)) : (uint8_t)(R_STONE + 3));
        if (hush) for (int i = 1; i < 16; i++) gfx_px(f, x + i, y + 16 - i, R_BLOOD + 11);   /* struck through */
        if (b == BOSS_SPIDER || b == BOSS_DEACON) {                /* a pip for each it may have out, lit while one is */
            int out = b == BOSS_SPIDER ? webs_out() : allies_out();
            for (int p = 0; p < RUN.abil[b]; p++) gfx_rect(f, x + 1 + p * 4, y - 4, 2, 2, p < out ? (uint8_t)C_WHITE : (uint8_t)(R_STONE + 7));
        }
        x += SOCK;
    }
}

/* the runes' sockets, at the lower right (section 3): a bevel edged in the rune's quality, its mark in its
 * colour, a line along its foot coming round; struck through in the void. Over the status bar for a person,
 * in the corner when it is away and in the dream */
static int runes_shown(void) {
    if (G.shop || G.tally || G.dead || (G.player && G.ui)) return 0;
    for (int k = 0; k < NDEF; k++) if (G.rs_slot[k] && G.rs_a[k] > 0.0f) return 1;
    return 0;
}
static void render_runesockets(Frame *f) {
    if (!runes_shown()) return;
    int bar_top = H - 44 + (int)((1.0f - smoothstepf(0.0f, 1.0f, G.bar)) * 46.0f + 0.5f);
    int y0 = G.player ? imin(bar_top - 20, H - 21) : H - 21 - (int)(G.lift * 40.0f + 0.5f);
    for (int k = 0; k < NDEF; k++) {
        if (!G.rs_slot[k] || G.rs_a[k] <= 0.0f) continue;
        int q = clampi(RUN.gear.def[k], 1, RUNE_EXQ), ph = rune_phase(k), ramp = gear_color(GK_DEF, k), qr = QUAL_RAMP[q - 1], hush = rune_barred(k);
        int x = W - 21 - (G.rs_slot[k] - 1) * SOCK, y = y0 + (int)((1.0f - smoothstepf(0.0f, 1.0f, G.rs_a[k])) * 24.0f + 0.5f);
        ui_well(f, x, y, 17, 17);                                  /* a socket sunk in the stone */
        for (int i = 0; i < 17; i++) {                             /* the edge in its quality's colour */
            gfx_px(f, x + i, y, ramp_at(qr, 13)); gfx_px(f, x, y + i, ramp_at(qr, 13));
            gfx_px(f, x + i, y + 16, ramp_at(qr, 7)); gfx_px(f, x + 16, y + i, ramp_at(qr, 7));
        }
        int glow = ph == RS_FIRED && rune_work(k) > 0.0f, lit = ph == RS_READY || (ph == RS_FLASH && ((int)(G.t * 8.0) & 1));
        for (int r = 0; r < 11; r++) for (int c = 0; c < 11; c++) {
            char ch = RUNE_MARK[k][r][c];
            if (ch != '#' && ch != 'o') continue;
            int v = hush ? 6 : glow ? (ch == 'o' ? 15 : 13 + (int)(1.5f * sinf((float)G.t * 9.0f))) : (ch == 'o' ? 14 : (lit ? 12 : 10));
            gfx_px(f, x + 3 + c, y + 2 + r, hush ? (uint8_t)(R_STONE + v) : ramp_at(ramp, v));
        }
        float wait = def_cooldown(q), fill = wait > 0.0f ? 1.0f - clampf(G.def_cd[k] / wait, 0.0f, 1.0f) : 1.0f;
        for (int i = 0; i < 13; i++)                               /* coming round */
            gfx_px(f, x + 2 + i, y + 14, i < (int)(fill * 13.0f + 0.5f) ? (fill >= 1.0f && !hush ? (uint8_t)(R_GOLD + 12) : (uint8_t)(R_STONE + 9)) : (uint8_t)(R_STONE + 3));
        if (hush) for (int i = 1; i < 16; i++) gfx_px(f, x + i, y + 16 - i, R_BLOOD + 11);   /* the void, or a status: no rune fires */
    }
}

/* Swift Dodge, seen from inside it: streaks at the edges of sight */
static void render_dash(Frame *f) {
    if (G.dash_t <= 0.0f || !G.player) return;
    for (int k = 0; k < 28; k++) {
        uint32_t h = hash2(k, (int)(G.t * 30.0));
        int y = (int)(h % H), len = 18 + (int)((h >> 8) % 70), side = (h >> 16) & 1;
        for (int i = 0; i < len; i++) { int x = side ? W - 1 - i : i; if (!((x + y) & 1)) f->pix[y * W + x] = (uint8_t)(R_GHOST + 10 + (i < len / 3 ? 3 : 0)); }
    }
}

/* screen tints: red when hurt or dying, gold for treasure; index 0 stays black for the OLED */
static void tint(Frame *f) {
    float red = 0.30f * G.hurt, gold = 0.22f * G.gold, dark = 0.0f, chill = 0.3f * G.chill;
    float green = G.poison_t > 0.0f ? 0.20f + 0.07f * sinf((float)G.t * 4.0f) : 0.0f;          /* poisoned */
    float blue = G.slow_t > 0.0f && G.poison_t <= 0.0f ? 0.10f * fminf(1.0f, G.slow_t) : 0.0f;  /* webbed and slowed */
    float violet = G.slip_t > 0.0f ? 0.10f * fminf(1.0f, G.slip_t) : 0.0f;                      /* Time Slip */
    if (G.dead) { float k = smoothstepf(0.0f, 1.4f, (float)G.dead_t); red = fmaxf(red, 0.45f * k); dark = 0.35f * k; }
    else if (G.hp <= G.max_hp / 4) red = fmaxf(red, 0.08f * (0.5f + 0.5f * sinf((float)G.t * 5.7f)));
    if (G.dread_t > 0.0f) { float k = sinf(PI_F * G.dread_t / 0.8f); dark = fmaxf(dark, 0.45f * k); violet = fmaxf(violet, 0.18f * k); }   /* Dread: a shadow falls */
    if (G.bfs_t > 0.0f) dark = fmaxf(dark, G.bfs_t < 0.12f ? 0.97f : 0.9f * (1.0f - smoothstepf(0.12f, 1.3f, G.bfs_t)));   /* the light goes out of the room */
    float ghost = G.wwalk_t > 0.0f ? 0.6f * fminf(1.0f, G.wwalk_t * 4.0f) : 0.0f;   /* Wraithwalk: the world grey and far */
    if (af_on(AF_POISON)) green = fmaxf(green, 0.20f + 0.07f * sinf((float)G.t * 4.0f));   /* the statuses (PLAN-combat, 4.2) */
    if (af_on(AF_FROZEN)) { blue = fmaxf(blue, 0.34f); chill = fmaxf(chill, 0.25f); }
    else if (af_on(AF_CHILL)) blue = fmaxf(blue, 0.06f * (float)G.af_n);   /* bluer by the stack */
    if (af_on(AF_FIRE)) { red = fmaxf(red, 0.06f + 0.03f * sinf((float)G.t * 13.0f)); gold = fmaxf(gold, 0.05f); }
    if (af_on(AF_UNBOUND)) violet = fmaxf(violet, 0.05f);
    if (af_on(AF_STUN)) ghost = fmaxf(ghost, 0.18f);
    if (G.af_k > 0.0f) dark = fmaxf(dark, 0.22f * G.af_k);
    if (red <= 0.0f && gold <= 0.0f && chill <= 0.0f && green <= 0.0f && blue <= 0.0f && violet <= 0.0f && dark <= 0.0f && ghost <= 0.0f) return;
    for (int i = 1; i < 256; i++) {
        int glow = G.bfs_t > 0.0f && i >= R_GLOW && i < R_GLOW + 16;   /* the imploding keep their light */
        float lum = 0.3f * f->pal[i][0] + 0.59f * f->pal[i][1] + 0.11f * f->pal[i][2];
        for (int c = 0; c < 3; c++) {
            float v = f->pal[i][c] * (glow || i == C_WHITE ? 1.0f : 1.0f - dark);
            v += ((c == 1 ? 0.7f : 0.85f) * lum + (c == 1 ? 0.0f : 4.0f) - v) * ghost;
            v += (50.0f * (c == 0) - v) * red * (c == 0 ? 0.6f : 1.0f);
            if (c < 2) v += ((c ? 50.0f : 63.0f) - v) * gold; else v -= v * gold;
            v += ((c == 0 ? 30.0f : 52.0f) - v) * chill;                 /* a wraith's cold breath */
            v += ((c == 1 ? 40.0f : 8.0f) - v) * green;
            v += ((c == 2 ? 50.0f : 20.0f) - v) * blue;
            v += ((c == 1 ? 10.0f : 44.0f) - v) * violet;
            f->pal[i][c] = (uint8_t)clampi((int)(v + 0.5f), 0, 63);
        }
    }
}

/* what is carried (1.0.0b13): small at the lower right, above the runes' sockets, in its own colours; and what
 * was read last, a panel under the news */
static void ui_sprite(Frame *f, int spr, int x0, int y0, int size, const uint8_t *lut, int shadow);
static void render_carry(Frame *f) {
    if (!G.carry || !G.player || G.ui || G.shop || G.dead || G.tally) return;
    const Thing *t = &G.thing[G.carry - 1];
    int bar_top = H - 44 + (int)((1.0f - smoothstepf(0.0f, 1.0f, G.bar)) * 46.0f + 0.5f);
    int x = W - 34, y = imin(bar_top - 20, H - 21) - 34;
    bevel(f, x - 2, y - 2, 32, 32, 0);
    if (t->spr >= 0) ui_sprite(f, t->spr, x, y, 28, NULL, 0);
    if (t->mark > 0) ui_sprite(f, t->mark >= 100 ? SP_PZ_LETTER0 + t->mark - 100 : t->mark > 16 ? SP_PIP1 + t->mark - 17 : SP_MARK0 + t->mark - 1, x, y + (int)(t->mz * -28.0f), 28, NULL, 0);
}
static void render_read(Frame *f) {
    if (G.read_t <= 0.0f || !G.read_text[0] || (G.player && G.ui)) return;
    int y = toast_showing() ? 66 : 30;
    ui_band(f, 14, y, W - 28, 40);
    text_wrap(f, 20, y + 4, G.read_text, 35, R_BONE + 13, 4);
}
/* a line of news at the top of the view, and the first-time hints */
static int target_shown(void);
static int target_y(void);
static void render_messages(Frame *f) {
    render_read(f);
    if (G.msg_t > 0.0f && G.msg[0] && !(G.player && G.ui)) {
        if (G.msg_t > 0.3f || ((int)(G.msg_t * 20.0f) & 1)) {
            int w = gfx_text_width(G.msg, 1), x = (W - w) / 2, y = toast_showing() ? 50 : (trial_shown() ? 60 : boss_fighting() ? 34 : 14);   /* below a pop-up,
                                                                      the hourglass, or a boss's bar */
            if (target_shown()) y = imax(y, target_y() + 34);         /* or what you are looking at, and what it does */
            ui_band(f, x - 5, y, w + 10, 14);
            gfx_text8(f, x, y + 3, G.msg, R_GOLD + 11, 1);
        }
    }
    if (G.hint_t > 0.0f && G.player && !G.ui && !G.dead && !G.claim_card && !shop_carded() && WH.k <= 0.0f) {   /* never over death's card, hers, a ware's or a wheel */
        static const char *const HINT[4] = { WEB ? "WASD WALK  MOUSE TURN  SPACE CASTS" : "WASD WALK  MOUSE TURN  CLICK CASTS", "1-9 SPELLS  E OPENS  TAB INVENTORY", "B BESTIARY  ESC LEAVE OR SLEEPWALK",
                                             "L WISPS  C THE CODEX  H THE STATUS BAR" };
        static char PADHINT[4][48], pm[6][48];                     /* the pad's lines, by the layout (1.0.0b18.1) */
        #define PW(j) PAD_WORD[pl_on(j)]
        snprintf(PADHINT[0], 48, "%s WALK  %s TURN  %s CASTS", PL.swap ? "R-STICK" : "L-STICK", PL.swap ? "L-STICK" : "R-STICK", PW(PAD_RT));
        snprintf(PADHINT[1], 48, "%s HELD: SPELLS  %s OPENS", PW(PAD_DOWN), PW(PAD_A));
        snprintf(PADHINT[2], 48, "MENU LEAVE  VIEW INVENTORY  %s WISPS", PW(PAD_Y));
        if (pl_on(PAD_LEFT) == PAD_LEFT && pl_on(PAD_RIGHT) == PAD_RIGHT && pl_on(PAD_DOWN) == PAD_DOWN) snprintf(PADHINT[3], 48, "D-PAD: BESTIARY, CODEX, STATUS BAR");
        else snprintf(PADHINT[3], 48, "%s BESTIARY  %s CODEX", pad_short(pl_on(PAD_LEFT)), pad_short(pl_on(PAD_RIGHT)));
        snprintf(pm[0], 48, "%s DODGES THE WAY THE STICK LEANS", PW(PAD_B));
        snprintf(pm[1], 48, "%s A GIFT  %s A RUNE  %s HELD CHOOSES", pad_short(pl_on(PAD_LB)), pad_short(pl_on(PAD_RB)), pl_on(PAD_UP) == PAD_UP ? "UP" : pad_short(pl_on(PAD_UP)));
        snprintf(pm[2], 48, "%s TAPPED: THE MAP", PW(PAD_UP));
        int ults = 0;                                              /* only Burning Intensity's bolt held: it grows (1.0.0b21) */
        for (int sp = 0; sp < AT_BFS; sp++) ults |= RUN.gear.atk[sp] >= 0 && (spell_charges(sp) || spell_tethers(sp));
        snprintf(pm[3], 48, ults ? "HOLD %s TO CHARGE AN ULTIMATE" : "HOLD %s: THE FIRE BOLT GROWS", PW(PAD_RT));
        snprintf(pm[4], 48, "SWING THE %s TO SHAKE OFF A BAT", PL.swap ? "L-STICK" : "R-STICK");
        #undef PW
        int charges = 0, bats = G.level >= 5;                      /* bats of rank II come in from the first run's fifth level */
        for (int sp = 0; sp < AT_BFS; sp++) charges |= RUN.gear.atk[sp] >= 0 && spell_holds(sp);
        const char *more[7] = { PADX.used ? pad_sprint_hint() : "SHIFT SPRINTS FORWARD, A WHILE", RUN.abil[BOSS_KNIGHT] ? (PADX.used ? pm[0] : "TAP A DIRECTION TWICE TO DODGE") : NULL,
                                ab_readied() >= 0 || rune_readied() >= 0 ? (PADX.used ? pm[1] : "RIGHT CLICK USES  Q NEXT, HOLD: WHEEL") : NULL,
                                RUN.has_map ? (PADX.used ? pm[2] : "M THE MAP") : NULL,
                                charges ? (PADX.used ? pm[3] : ults ? "HOLD A SPELL TO CHARGE ITS ULTIMATE" : "HOLD THE FIRE BOLT AND IT GROWS") : NULL,   /* 1.0.0b11 */
                                bats ? (PADX.used ? pm[4] : "TURN OR STRAFE HARD TO SHAKE OFF A BAT") : NULL,
                                G.shop || RUN.al_points > 0 ? "THE LAMPSTAND NORTH OF HER: THE LINES" : NULL };
        int touch = TCH.used && !PADX.used;                        /* by touch, its own lines (docs/PLAN-mobile.md) */
        const char *TL[7] = { "LEFT THUMB WALKS, RIGHT THUMB TURNS", "THE RIGHT STONE CASTS; HELD, AGAIN", "TAP THE LEFT STONE TO USE",
                              "HOLD THE TOP LEFT TO PAUSE", "HOLD THE TOP RIGHT FOR THE WHEELS", "PUSH THE WALK PAST ITS RING: SPRINT",
                              RUN.abil[BOSS_KNIGHT] ? "TOUCH TWICE AND PUSH: DODGE" : NULL };
        #define HLINE(i) (touch ? ((i) < 7 ? TL[i] : NULL) : (i) < 4 ? (PADX.used ? PADHINT[i] : HINT[i]) : more[(i) - 4])
        int rows = 0, widest = 0;                                  /* one band of stone behind them all */
        for (int i = 0; i < 11; i++) { const char *h = HLINE(i); if (h) { rows++; widest = imax(widest, hint_width(h)); } }
        ui_band(f, (W - widest) / 2 - 6, 30, widest + 12, rows * 11 + 5);
        for (int i = 0, row = 0; i < 11; i++) {
            const char *h = HLINE(i);
            if (!h) continue;
            int w = hint_width(h), x = (W - w) / 2, y = 34 + row++ * 11;
            hint8(f, x, y, h, R_STONE + 13);
        }
        #undef HLINE
    }
}

