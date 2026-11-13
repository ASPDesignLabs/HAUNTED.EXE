/* A part of scene_crawl.c (docs/PLAN-tidy.md): it is #included there in its place and never compiled on its own.
 * Menus drawn: the L list, the altar, the shrine, the Compendium, the Repository, the Codex, the Totem, the Portal. */

/* the L menu: a small plate over the crypt, a wisp of each kind that can be cast now and its price */
static void render_wispmenu(Frame *f) {
    static const char *const KEYW[NKEYS] = { "THE GOLD KEY", "THE SILVER KEY", "THE CRIMSON KEY" };
    WispChoice c[WISP_CHOICES];
    int n = wisp_choices(c), rows = n + lantern_held(), sel = clampi(G.ui_sel, 0, rows - 1);
    dim_view(f);
    int pw = 296, ph = 24 + rows * 13 + 5 + (RUN.wisp_auto ? 26 : 16) + 18, px = (W - pw) / 2, py = 24;
    ui_slab(f, px, py, pw, ph, SLAB_TABLET, ORN_NONE, G.ui_t);       /* a tablet: the wisps in a well, the choice under it, the keys at its foot */
    gfx_text16(f, px + 10, py + 6, "WISPS", R_GOLD + 12, 1);
    char sp[24], line[40];
    spirit_text(sp, sizeof sp);
    snprintf(line, sizeof line, "SPIRIT %s", sp);
    gfx_text8(f, px + pw - 10 - gfx_text_width(line, 1), py + 10, line, R_GLOW + 12, 1);
    ui_well(f, px + 6, py + 24, pw - 12, rows * 13 + 5);
    for (int r = 0; r < n; r++) {
        int y = py + 29 + r * 13, ok = c[r].cost <= 0.0f || spirit() + 1e-4f >= c[r].cost;
        ui_row(f, px + 8, y - 3, pw - 16, 13, r == sel);
        Wisp probe;
        probe.kind = c[r].kind; probe.color = c[r].color;
        wisp_ramp = wisp_ramp_of(&probe);                          /* a little wisp of its colour */
        float t = (float)G.t * 9.0f + r;
        for (int pass = 0; pass < 2; pass++) wisp_ball(f, px + 17, y + 3.5f, 3.2f + 0.3f * sinf(t), 0.8f, pass, 0.0f);
        const char *what = c[r].kind == WK_EXIT ? "THE WAY OUT" : (c[r].kind == WK_KEY ? KEYW[c[r].color] : "WHAT THE ALTAR HIDES");
        gfx_text8(f, px + 28, y, what, r == sel ? (uint8_t)C_WHITE : (uint8_t)(R_STONE + 12), 1);
        const char *cost = c[r].cost <= 0.0f ? "FREE" : "1 SOUL";
        gfx_text8(f, px + pw - 12 - gfx_text_width(cost, 1), y, cost, ok ? (uint8_t)(R_GOLD + 11) : (uint8_t)(R_BLOOD + 13), 1);
    }
    if (lantern_held()) {                                          /* the lantern's row */
        int y = py + 29 + n * 13;
        ui_row(f, px + 8, y - 3, pw - 16, 13, sel == n);
        lantern_mark(f, px + 14, y - 1, lantern_lit());
        gfx_text8(f, px + 28, y, lantern_lit() ? "THE LANTERN: LIT" : "THE LANTERN: SHUTTERED", sel == n ? (uint8_t)C_WHITE : (uint8_t)(R_STONE + 12), 1);
        const char *hold = PADX.used ? "HOLD Y" : "HOLD L";
        gfx_text8(f, px + pw - 12 - gfx_text_width(hold, 1), y, hold, R_STONE + 9, 1);
    }
    int ty = py + 24 + rows * 13 + 5 + 6;
    ui_well(f, px + 11, ty - 1, 10, 10);                           /* the toggle: a cup cut in the stone, a gold cross in it when on */
    if (RUN.wisp_auto) for (int k = 1; k < 7; k++) { gfx_px(f, px + 12 + k, ty + k, R_GOLD + 12); gfx_px(f, px + 19 - k, ty + k, R_GOLD + 12); }
    hint8(f, px + 28, ty, "L ALWAYS CASTS THIS  (SPACE)", R_STONE + 12);
    if (RUN.wisp_auto) gfx_text8(f, px + 28, ty + 10, lantern_held() ? "TAP L TWICE FOR THIS LIST" : "HOLD L FOR THIS LIST", R_STONE + 9, 1);
    hall_keys(f, py + ph - 14, "W/S CHOOSE  ENTER CASTS  ESC CLOSES");
}

/* the altar's menu: its gifts, a little fire of each one's colour, the price, and a note on the one chosen */
static void render_altarmenu(Frame *f) {
    static const int RAMP[3] = { R_PURPLE, R_GLOW, R_GOLD };
    AltarRow r[3];
    altar_rows(r);
    int sel = clampi(G.ui_sel, 0, 2);
    dim_view(f);
    int pw = 296, ph = 122, px = (W - pw) / 2, py = 24;
    ui_slab(f, px, py, pw, ph, SLAB_TABLET, ORN_NONE, G.ui_t);       /* a tablet: the gifts in a well, the chosen one's note in another */
    gfx_text16(f, px + 10, py + 6, "THE BLACK ALTAR", R_GOLD + 12, 1);
    char sp[24], line[40];
    spirit_text(sp, sizeof sp);
    snprintf(line, sizeof line, "SPIRIT %s", sp);
    gfx_text8(f, px + pw - 10 - gfx_text_width(line, 1), py + 10, line, R_GLOW + 12, 1);
    ui_well(f, px + 6, py + 24, pw - 12, 3 * 13 + 5);
    ui_well(f, px + 6, py + 72, pw - 12, 30);
    for (int i = 0; i < 3; i++) {
        int y = py + 29 + i * 13, pay = r[i].price <= 0 || spirit() + 1e-4f >= (float)r[i].price;
        ui_row(f, px + 8, y - 3, pw - 16, 13, i == sel);
        float t = (float)G.t * 9.0f + i;
        if (i == 1) { wisp_ramp = R_GOLD + 4; wisp_ball(f, px + 17, y + 3.5f, 3.4f, 0.6f, 0, 0.0f); }   /* the blessing's halo */
        wisp_ramp = RAMP[i];
        for (int pass = 0; pass < 2; pass++) wisp_ball(f, px + 17, y + 3.5f, (r[i].ok ? 3.2f : 2.2f) + 0.3f * sinf(t), 0.8f, pass, 0.0f);
        uint8_t c = !r[i].ok ? (uint8_t)(R_STONE + 8) : (i == sel ? (uint8_t)C_WHITE : (uint8_t)(R_STONE + 12));
        gfx_text8(f, px + 28, y, r[i].what, c, 1);
        if (r[i].cost[0])
            gfx_text8(f, px + pw - 12 - gfx_text_width(r[i].cost, 1), y, r[i].cost, pay ? (uint8_t)(R_GOLD + 11) : (uint8_t)(R_BLOOD + 13), 1);
    }
    gfx_text8(f, px + 12, py + 77, r[sel].note, r[sel].ok ? (uint8_t)(R_STONE + 12) : (uint8_t)(R_STONE + 10), 1);
    if (G.blessed) gfx_text8(f, px + 12, py + 89, "YOUR EXIT WISPS ARE BLESSED", R_GOLD + 10, 1);
    hall_keys(f, py + ph - 14, "W/S CHOOSE  ENTER TAKES  ESC CLOSES");
}

/* the shrine's screen: the nine slots on the left, its actions (or a question) on the right with a note
 * under them, and the runes you hold along the bottom */
static void render_shrinemenu(Frame *f) {
    dim_view(f);
    int px = 6, pw = W - 12;
    char line[96];
    snprintf(line, sizeof line, "CURSED COINS %d", RUN.coins);
    int lw = gfx_text_width(line, 1);
    hall_frame(f, "THE RUNE SHRINE", R_GOLD + 12, line, R_GOLD + 11);
    draw_coin(f, 308 - 11 - lw, 12);
    int rank = RUN.shrine, ramp = rank ? QUAL_RAMP[rank - 1] : R_STONE;
    if (!rank) gfx_text8(f, 14, 28, "SANCTIFIED: IT GIVES NOTHING", R_STONE + 11, 1);
    else { snprintf(line, sizeof line, "CURSED: RUNES UP TO %s", QUAL_NAME[rank - 1]); gfx_text8(f, 14, 28, line, ramp_at(ramp, 12), 1); }
    ui_well(f, 9, 39, 115, 107);                                   /* the slots; what to do with them; what you hold */
    ui_well(f, 127, 39, 184, 107);
    ui_well(f, 9, 149, 302, 32);
    /* the slots */
    int slots = G.sh_mode == SM_TAKE || G.sh_mode == SM_LETGO;
    for (int i = 0; i < SHRINE_SLOTS; i++) {
        int x = 14 + i % 3 * 36, y = 43 + i / 3 * 34, k = RUN.slot_def[i] - 1, q = RUN.slot_q[i];
        int fl = G.sh_flash[i] > 0.0f && ((int)(G.sh_flash[i] * 16.0f) & 1);
        ui_well(f, x, y, 32, 30);                                  /* a cup cut in the stone; lit round when chosen or filling */
        if (fl) for (int yy = 2; yy < 28; yy++) for (int xx = 2; xx < 30; xx++) if ((xx + yy) & 1) gfx_px(f, x + xx, y + yy, UI_M(9));
        int chosen = slots && i == G.sh_slot;
        if (chosen || fl) {
            uint8_t rim = chosen ? (uint8_t)(G.sh_arm == i ? R_BLOOD + 13 : R_GOLD + 12) : (uint8_t)C_WHITE;
            for (int j = -1; j < 33; j++) { gfx_px(f, x + j, y - 1, rim); gfx_px(f, x + j, y + 30, rim); }
            for (int j = -1; j < 31; j++) { gfx_px(f, x - 1, y + j, rim); gfx_px(f, x + 32, y + j, rim); }
        }
        if (k < 0) continue;
        int r = QUAL_RAMP[q - 1];
        draw_glyph(f, x + 11, y + 4, k, 2, ramp_at(r, 13));
        gfx_text8(f, x + (32 - gfx_text_width(Q_TAG[q - 1], 1)) / 2, y + 20, Q_TAG[q - 1], ramp_at(r, 10), 1);
    }
    /* the actions, or a question */
    int rx = 133, rr = 305, ry = 44, ny;
    char note[160] = "";
    if (G.sh_mode == SM_FULL || G.sh_mode == SM_LEAVE) {
        static const char *const FULL[3] = { "LET GO OF SOME FIRST", "PULL ANYWAY", "NOT NOW" };
        static const char *const LEAVE[2] = { "CHOOSE A RUNE", "CLOSE FOR NOW" };
        int full = G.sh_mode == SM_FULL, n = full ? 3 : 2;
        if (full) snprintf(line, sizeof line, "THIS PULL MAY BRING %d, AND %d SLOT%s EMPTY. WHAT DOESN'T FIT FADES.",
                           pull_most(G.sh_coins), shrine_count(0), shrine_count(0) == 1 ? " IS" : "S ARE");
        else snprintf(line, sizeof line, "%d TAKE%s LEFT. UNUSED, %s LOST WHEN YOU LEAVE THE ROOM.", G.sh_takes, G.sh_takes == 1 ? "" : "S", G.sh_takes == 1 ? "IT IS" : "THEY ARE");
        int nl = text_wrap(f, rx, ry, line, 22, R_STONE + 13, 4);
        for (int i = 0; i < n; i++) {
            int y = ry + nl * 9 + 6 + i * 13;
            ui_row(f, 129, y - 3, 180, 13, i == G.ui_sel);
            gfx_text8(f, rx, y, full ? FULL[i] : LEAVE[i], i == G.ui_sel ? (uint8_t)C_WHITE : (uint8_t)(R_STONE + 12), 1);
        }
        ny = ry + nl * 9 + 10 + n * 13;
    } else {
        int n = shrine_rows();
        for (int i = 0; i < n; i++) {
            int y = ry + i * 11, sel = !slots && i == G.ui_sel, ok = 1;
            const char *what = "";
            char val[32] = "";
            if (!rank) { what = i ? "LEAVE" : "CURSE IT"; if (!i) { snprintf(val, sizeof val, "1 COIN"); ok = RUN.coins >= 1; } }
            else switch (i) {
            case SR_PULL: what = "PULL"; snprintf(val, sizeof val, "< %d COIN%s >", G.sh_coins, G.sh_coins == 1 ? "" : "S"); ok = G.sh_coins + fav_coins(G.sh_fav) <= RUN.coins; break;
            case SR_FAVOUR:
                what = "FAVOUR";
                if (!RUN.attuned) { snprintf(val, sizeof val, "NONE ATTUNED"); ok = 0; }
                else if (!G.sh_fav) snprintf(val, sizeof val, "< NONE >");
                else snprintf(val, sizeof val, "< %s +%d >", DEF_SHORT[fav_kind(G.sh_fav)], fav_coins(G.sh_fav));
                break;
            case SR_RAISE:
                what = "RAISE";
                if (rank >= NQUAL) { snprintf(val, sizeof val, "AT ITS HIGHEST"); ok = 0; }
                else { snprintf(val, sizeof val, "%s: %d", QUAL_NAME[rank], SHRINE_COST[rank]); ok = RUN.coins >= SHRINE_COST[rank]; }
                break;
            case SR_TAKE: what = "TAKE A RUNE"; snprintf(val, sizeof val, "%d LEFT", G.sh_takes); ok = G.sh_takes > 0 && shrine_count(1); break;
            case SR_LETGO: what = "LET GO OF ONE"; ok = shrine_count(1) > 0; break;
            default: what = "LEAVE"; break;
            }
            ui_row(f, 129, y - 2, 180, 11, sel);
            gfx_text8(f, rx, y, what, sel ? (uint8_t)C_WHITE : (uint8_t)(R_STONE + 12), 1);
            if (val[0]) gfx_text8(f, rr - gfx_text_width(val, 1), y, val, ok ? (uint8_t)(R_GOLD + 11) : (uint8_t)(R_STONE + 9), 1);
        }
        ny = ry + n * 11 + 3;
        if (slots) {                                               /* the rune in the slot chosen */
            int k = RUN.slot_def[G.sh_slot] - 1, q = RUN.slot_q[G.sh_slot];
            if (k < 0) snprintf(note, sizeof note, "AN EMPTY SLOT");
            else if (RUN.gear.def[k] > 0) snprintf(note, sizeof note, "%s %s. YOU HOLD %s.", QUAL_NAME[q - 1], DEF_NAME[k], QUAL_NAME[RUN.gear.def[k] - 1]);
            else snprintf(note, sizeof note, "%s %s. YOU HAVE NONE.", QUAL_NAME[q - 1], DEF_NAME[k]);
        } else if (!rank) snprintf(note, sizeof note, RUN.coins ? "ONE CURSED COIN WAKES IT, AND THEN IT GIVES RUNES." : "CURSED COINS LIE DEEP IN THE SECRETS.");
        else switch (G.ui_sel) {
        case SR_PULL: { int lo = G.sh_coins == 3 ? 3 : G.sh_coins, hi = pull_most(G.sh_coins);
            snprintf(note, sizeof note, "SHOWS %d TO %d RUNES. ONE TAKE A PULL.", lo, hi); break; }
        case SR_FAVOUR: snprintf(note, sizeof note, RUN.attuned ? "EACH COIN, ADDED TO THE PULL, MAKES IT A THIRD LIKELIER." : "IT LEARNS A KIND WHEN YOU HOLD ONE AT RARE."); break;
        case SR_RAISE: snprintf(note, sizeof note, "RAISED, IT GIVES BETTER RUNES."); break;
        case SR_TAKE: snprintf(note, sizeof note, "UNUSED TAKES ARE LOST WHEN YOU LEAVE THE ROOM."); break;
        case SR_LETGO: snprintf(note, sizeof note, "FREES A SLOT. THE RUNE IS GONE."); break;
        default: snprintf(note, sizeof note, "THE SLOTS KEEP THEIR RUNES FOR NEXT TIME."); break;
        }
    }
    if (G.sh_note_t > 0.0f && G.sh_mode != SM_FULL && G.sh_mode != SM_LEAVE) text_wrap(f, rx, ny + 2, G.sh_note, 22, R_GOLD + 11, 3);
    else if (note[0]) text_wrap(f, rx, ny + 2, note, 22, R_STONE + 11, 3);
    /* what you hold, under its own line */
    int y = 152, x = px + 8;
    gfx_text8(f, x, y, "YOURS", R_GOLD + 10, 1);
    y += 10;
    int any = 0;
    for (int k = 0; k < NDEF; k++) {
        int q = RUN.gear.def[k];
        if (q <= 0) continue;
        any = 1;
        int w = gfx_text_width(DEF_SHORT[k], 1), wt = gfx_text_width(Q_TAG[q - 1], 1);
        if (x + w + 5 + wt > px + pw - 10) { x = px + 8; y += 9; }
        gfx_text8(f, x, y, DEF_SHORT[k], ramp_at(QUAL_RAMP[q - 1], 12), 1);
        gfx_text8(f, x + w + 5, y, Q_TAG[q - 1], ramp_at(QUAL_RAMP[q - 1], 9), 1);
        x += w + 5 + wt + 9;
    }
    if (!any) gfx_text8(f, x, y, "NO RUNES YET", R_STONE + 9, 1);
    const char *hint = slots ? (G.sh_mode == SM_TAKE ? "WASD A SLOT  ENTER TAKES  ESC BACK" : "WASD A SLOT  ENTER LETS GO  ESC BACK")
                     : (G.sh_mode == SM_PULL ? "THE RUNES ARE COMING..." : "W/S CHOOSE  A/D CHANGE  ESC LEAVES");
    if (G.sh_mode == SM_PULL) hall_word(f, 184, hint, UI_M(11)); else hall_keys(f, 184, hint);
}

/* the Totem's screen: a row for each boss beaten, its ability's mark, its rank, the scrip held, what the
 * next rank costs and, with scrip to spare, a sale; a card on the one chosen */
/* the Compendium's page: its entries in their qualities' colours with prices, and the card of the one chosen */
static int piece_card(Frame *f, int x, int y, int w, const Piece *p, const Gear *g, const Gear *g0);
static void render_compmenu(Frame *f) {
    dim_view(f);
    int page = clampi(G.comp_page, 0, NCP - 1);
    char line[96];
    snprintf(line, sizeof line, "$%d", RUN.treasure);
    hall_frame(f, "THE CORPUS COMPENDIUM", R_GOLD + 12, line, R_GOLD + 11);
    snprintf(line, sizeof line, "%s  %d OF %d", CP_NAME[page], page + 1, NCP);
    gfx_text8(f, 14, 28, line, R_STONE + 13, 1);
    const char *k = pk("A/D  W/S  ENTER  ESC");
    ui_keys(f, 306 - ui_keys_width(k), 28, k, UI_M(13));
    CompRow r[NQUAL * COMP_MAX + NATK];
    int n = comp_rows(page, r), sel = clampi(G.ui_sel, 0, n - 1), rows = 7;
    ui_well(f, 9, 39, 302, rows * 9 + 6);                          /* the entries, and the chosen one's card under them */
    ui_well(f, 9, 39 + rows * 9 + 9, 302, 181 - (39 + rows * 9 + 9));
    if (!n) {
        text_wrap(f, 14, 44, "NOTHING OF THIS KIND IS WRITTEN HERE YET. WHAT YOU WEAR, IT KEEPS.", 36, R_STONE + 11, 3);
        return;
    }
    int top = clampi(sel - rows / 2, 0, imax(0, n - rows)), y = 43;
    for (int j = top; j < n && j < top + rows; j++, y += 9) {
        const CompRow *c = &r[j];
        int price = comp_price(page, c), col;
        {
            char a[48], b[48];
            piece_name(&RUN.comp[CP_KIND[page]][c->q][c->i], a, b, sizeof a);
            snprintf(line, sizeof line, "%s", page == CP_BAG ? bag_name(bag_capacity(c->q)) : b);
            col = qual_text(c->q);
        }
        ui_row(f, 11, y - 1, 292, 9, j == sel);
        gfx_text8(f, 14, y, line, (uint8_t)col, 1);
        char pr[16]; snprintf(pr, sizeof pr, "$%d", price);
        gfx_text8(f, 300 - gfx_text_width(pr, 1), y, pr, price <= RUN.treasure ? (uint8_t)(R_GOLD + 11) : (uint8_t)(R_BLOOD + 13), 1);
    }
    ui_scroll(f, 305, 41, rows * 9 + 2, top, rows, n);              /* where in the list, in place of a count */
    const CompRow *c = &r[sel];
    piece_card(f, 14, 39 + rows * 9 + 13, 292, &RUN.comp[CP_KIND[page]][c->q][c->i], NULL, NULL);
    if (G.sh_note_t > 0.0f) hall_word(f, 184, G.sh_note, G.comp_arm >= 0 ? R_BLOOD + 13 : R_GOLD + 11);
    else {
        snprintf(line, sizeof line, "AT THE SHOP'S PRICE x%.2f", comp_factor());
        hall_word(f, 184, line, UI_M(11));
    }
}

/* the Repository's screen: its places down the left, the gold and coins on the right, and under them the
 * card of what lies in the place chosen, or what the row will do */
static void render_repomenu(Frame *f) {
    static const char *const AMT[4] = { "ALL", "HALF", "A QUARTER", "A TENTH" };
    dim_view(f);
    int px = 6, pw = W - 12, cx = 165;
    hall_frame(f, "THE WRETCHED REPOSITORY", R_IRON + 13, 0, 0);
    char line[96];
    if (G.rep_mode) {                                              /* which piece goes in */
        RepFrom fr[NSLOT + BAG_MAX + 1];
        int n = repo_froms(fr), sel = clampi(G.ui_sel, 0, n - 1);
        snprintf(line, sizeof line, "INTO PLACE %d: WHICH?", G.rep_place + 1);
        gfx_text8(f, 14, 28, line, R_STONE + 13, 1);
        ui_well(f, 9, 39, 152, 142);                               /* what might go in, and its card */
        ui_well(f, 161, 39, 150, 142);
        for (int j = 0; j < n; j++) {
            int y = 43 + j * 9;
            Piece p = repo_from_piece(&fr[j]);
            ui_row(f, 11, y - 1, 148, 9, j == sel);
            if (fr[j].slot >= 0) snprintf(line, sizeof line, "%s", SLOT_NAME[fr[j].slot]);
            else if (fr[j].bag >= 0) snprintf(line, sizeof line, "%s, BAGGED", KIND_NAME[p.kind]);
            else snprintf(line, sizeof line, "THE BAG ITSELF");
            gfx_text8(f, px + 10, y, line, (uint8_t)qual_text(p.quality), 1);
        }
        if (n) {
            Piece p = repo_from_piece(&fr[sel]);
            int tax = repo_tax(G.rep_place, &p);
            if (tax) snprintf(line, sizeof line, "IN FOR $%d", tax); else snprintf(line, sizeof line, "IN FOR NOTHING");
            gfx_text8(f, cx, 43, line, tax > RUN.treasure ? (uint8_t)(R_BLOOD + 13) : (uint8_t)(R_GOLD + 11), 1);
            piece_card(f, cx, 56, 306 - cx, &p, NULL, NULL);
        }
        if (G.sh_note_t > 0.0f) hall_word(f, 184, G.sh_note, R_GOLD + 11); else hall_keys(f, 184, "ENTER SHUTS IT AWAY  ESC BACK");
        return;
    }
    ui_well(f, 9, 28, 152, 153);                                   /* the places down the left; the money, and what the row does */
    ui_well(f, 161, 28, 150, 153);
    RepRow r[REPO_MAX + 6];
    int n = repo_rows(r), sel = clampi(G.ui_sel, 0, n - 1), ly = 32, ry = 32;
    for (int j = 0; j < n; j++) {
        const RepRow *c = &r[j];
        if (c->kind <= RR_BUY) {                                   /* the places, down the left */
            ui_row(f, 11, ly - 1, 148, 9, j == sel);
            if (c->kind == RR_BUY) {
                snprintf(line, sizeof line, "BUY A PLACE");
                gfx_text8(f, px + 10, ly, line, j == sel ? (uint8_t)C_WHITE : (uint8_t)(R_STONE + 11), 1);
                int pr = PLACE_PRICE[clampi(RUN.repo_n - 2, 0, REPO_MAX - 3)];
                snprintf(line, sizeof line, "$%d", pr);
                gfx_text8(f, px + 150 - gfx_text_width(line, 1), ly, line, pr <= RUN.treasure ? (uint8_t)(R_GOLD + 11) : (uint8_t)(R_BLOOD + 13), 1);
            } else {
                const Piece *p = &RUN.repo[c->i];
                snprintf(line, sizeof line, "%2d %s", c->i + 1, p->kind ? KIND_NAME[p->kind] : "EMPTY");
                gfx_text8(f, px + 10, ly, line, p->kind ? (uint8_t)qual_text(p->quality) : (uint8_t)(R_STONE + 8), 1);
                if (c->i == 0) gfx_text8(f, px + 150 - gfx_text_width("UNTAXED", 1), ly, "UNTAXED", R_STONE + 9, 1);
            }
            ly += 9;
            continue;
        }
        if (ry == 32) {                                            /* the money, on the right */
            snprintf(line, sizeof line, "ON YOU: $%d", RUN.treasure);
            gfx_text8(f, cx, ry, line, R_GOLD + 9, 1);
            snprintf(line, sizeof line, "IN IT:  $%d", RUN.repo_gold);
            gfx_text8(f, cx, ry + 9, line, R_GOLD + 11, 1);
            draw_coin(f, cx, ry + 19);
            snprintf(line, sizeof line, "%d IN, %d ON YOU", RUN.repo_coins, RUN.coins);
            gfx_text8(f, cx + 10, ry + 19, line, R_GOLD + 10, 1);
            ry += 32;
        }
        static const char *const MR[4] = { "PUT GOLD IN", "TAKE GOLD OUT", "PUT A COIN IN", "TAKE A COIN OUT" };
        ui_row(f, 163, ry - 1, 146, 9, j == sel);
        gfx_text8(f, cx, ry, MR[c->kind - RR_GOLD_IN], j == sel ? (uint8_t)C_WHITE : (uint8_t)(R_STONE + 12), 1);
        if (c->kind <= RR_GOLD_OUT) gfx_text8(f, px + pw - 10 - gfx_text_width(AMT[clampi(G.rep_amt, 0, 3)], 1), ry, AMT[clampi(G.rep_amt, 0, 3)], R_STONE + 10, 1);
        ry += 10;
    }
    ry += 4;
    ui_rule(f, 164, 306, ry - 3, 0);
    const RepRow *c = &r[sel];
    if (c->kind == RR_PLACE && RUN.repo[c->i].kind != IK_NONE) piece_card(f, cx, ry + 2, 306 - cx, &RUN.repo[c->i], NULL, NULL);
    else {
        int a;
        switch (c->kind) {
        case RR_PLACE: snprintf(line, sizeof line, c->i == 0 ? "EMPTY. WHAT GOES IN HERE GOES IN FOR NOTHING." : "EMPTY. A PIECE GOES IN FOR A TENTH OF WHAT THE SHOP WOULD ASK."); break;
        case RR_BUY: snprintf(line, sizeof line, "ONE MORE PLACE, KEPT THROUGH DEATH. TWELVE AT THE MOST."); break;
        case RR_GOLD_IN: a = gold_amount(RUN.treasure); snprintf(line, sizeof line, "PUT IN $%d, AND $%d IS KEPT. A QUARTER GOES TO THE DEAD.", a, a - a / 4); break;
        case RR_GOLD_OUT: snprintf(line, sizeof line, "TAKE OUT $%d, FREE.", gold_amount(RUN.repo_gold)); break;
        case RR_COIN_IN: snprintf(line, sizeof line, "A CURSED COIN IN, FREE. NOTHING IN HERE IS LOST TO DEATH OR TO HER CLAIM."); break;
        default: snprintf(line, sizeof line, "A CURSED COIN OUT, FOR $%d.", COIN_OUT); break;
        }
        text_wrap(f, cx, ry + 2, line, 17, R_STONE + 12, 6);
    }
    if (G.sh_note_t > 0.0f) hall_word(f, 184, G.sh_note, R_GOLD + 11); else hall_keys(f, 184, "W/S CHOOSE  A/D HOW MUCH  ENTER  ESC");
}

/* a page of the Codex: what it is, what it does, its numbers by quality or rank, where it comes from, and
 * what you have of it now */
static void total_text(const Gear *g, int st, char *buf, int n);
static void stat_does(int st, char *buf, int n) {                  /* the elements' stats, in the Codex's words */
    if (st == ST_RES_LIGHT) { snprintf(buf, (size_t)n, "LIGHTNING HURTS LESS, AND A BOLT OF IT BREAKS A SPELL BEING GATHERED LESS OFTEN."); return; }
    if (st == ST_SPRINT) { snprintf(buf, (size_t)n, "MORE SECONDS OF SPRINT, PAST THE TWO AND A HALF EVERYONE HAS, TO TEN IN ALL."); return; }
    if (st == ST_PHYSIC) { snprintf(buf, (size_t)n, "A POTION RESTORES MORE THAN ITS TWENTY-FIVE: HALF AS MUCH AGAIN AT FIFTY, FIVE TIMES AT FOUR HUNDRED."); return; }
    if (st == ST_ADD_POISON || st == ST_ADD_SPIRIT || st == ST_ADD_PHYS) {   /* 1.0.0b21's (docs/PLAN-combat.md section 12) */
        const char *el = STAT_NAME[st];
        snprintf(buf, (size_t)n, "A SHARE OF EVERY SPELL'S DAMAGE IS ADDED AS %s, AS MUCH AS %s BITES WHAT IT HITS. THE SIX SHARE ONE CAP.", el, el);
        return;
    }
    if (st == ST_STEADY) { snprintf(buf, (size_t)n, "KNOCKED DOWN, STUNNED AND FROZEN COME LESS OFTEN AND END SOONER, WITH AGILITY'S SHARE, NINE TENTHS AT MOST."); return; }
    if (st >= ST_CHANCE && st < ST_CHANCE + EL_PHYS) {
        static const char *const ODDS[EL_PHYS] = { "YOUR FIRE SETS THE DEAD ALIGHT MORE OFTEN, BY THIS SHARE.",
            "YOUR FROST LAYS A SECOND CHILL THIS SHARE OF THE TIME, SO THE DEAD FREEZE SOONER.",
            "YOUR LIGHTNING STUNS THE DEAD MORE OFTEN, BY THIS SHARE.",
            "THIS SHARE OF YOUR POISON'S DOSES RUN HALF AGAIN AS LONG, AND HURT HALF AGAIN AS MUCH.",
            "YOUR SPIRIT LEAVES THE DEAD UNBOUND MORE OFTEN, BY THIS SHARE." };
        snprintf(buf, (size_t)n, "%s", ODDS[st - ST_CHANCE]);
        return;
    }
    int e = st >= ST_INS ? st - ST_INS : st >= ST_EFF ? st - ST_EFF : st - ST_POW;
    if (st >= ST_INS) snprintf(buf, (size_t)n, "%s SPELLS GATHER QUICKER, AFTER WHAT THE LINE'S INSIGHT TAKES OFF.", ELEM_NAME[e]);
    else if (st >= ST_EFF && e == EL_PHYS) snprintf(buf, (size_t)n, "A SPEAR SPENDS NO BONE MORE OFTEN, WITH THE LINE'S EFFICIENCY; NINE TIMES IN TEN AT MOST.");
    else if (st >= ST_EFF) snprintf(buf, (size_t)n, "%s SPELLS TAKE LESS SPIRIT, WITH THE LINE'S EFFICIENCY; NINE TENTHS AT MOST IN ALL.", ELEM_NAME[e]);
    else snprintf(buf, (size_t)n, "%s SPELLS HIT HARDER, WITH WHAT THE LINE'S MASTERY GIVES.", ELEM_NAME[e]);
}
static void render_codex(Frame *f) {
    static const char *const ABIL_DOES[NABIL] = { "",
        "YOUR MOVEMENT, RAISED ONCE MORE BY THIS SHARE OF IT.",
        "THE SPIRIT YOUR SPELLS SAVE, RAISED ONCE MORE BY THIS SHARE OF IT.",
        "MORE GOLD FROM TREASURE YOU PICK UP, AND A WHISPER FROM AN UNOPENED SECRET WALL WITHIN THREE CELLS.",
        "KEYS WITHIN ITS REACH SPARKLE THROUGH THE WALLS, AND THE L MENU OFFERS A WISP TO EACH.",
        "EVERY FIFTEEN SECONDS, CREATURES WITHIN ITS REACH SHOW AS RED SHAPES THROUGH THE WALLS FOR A MOMENT.",
        "WHATEVER STRIKES OR BITES YOU TAKES SO MUCH BACK, WHATEVER YOUR ARMOUR TOOK.",
        "A CHANCE THAT A MISSILE IS SENT BACK, HOMING ON ITS THROWER ROUND CORNERS.",
        "YOUR FIRE SETS THE DEAD ALIGHT MORE OFTEN, BY THIS SHARE, AND THEY BURN A SECOND LONGER.",   /* the classes' (1.0.0b21) */
        "THE CHILL YOU LAY ON THE DEAD LASTS A SECOND LONGER, AND THE FROZEN TAKE THIS SHARE MORE FROM YOU.",
        "YOUR LIGHTNING STUNS MORE OFTEN, BY THIS SHARE; STRIKE A STUNNED ONE AND A FIFTH OF THE HIT ARCS TO THE NEAREST WITHIN TWO CELLS.",
        "WHAT YOU HAVE POISONED TAKES THIS SHARE MORE FROM EVERY ELEMENT.",
        "YOUR SPIRIT LEAVES THE DEAD UNBOUND MORE OFTEN, BY THIS SHARE; WHAT DIES UNBOUND LEAVES A SOUL MORE.",
        "KNOCKED DOWN LESS OFTEN AND NOT AS LONG, WITH AGILITY. A SPEAR THAT KILLS THE STAGGERED OR PINNED LEAVES ITS BONE." };
    static const char *const EXQ_DOES[NDEF] = { "IT BURSTS INTO SHARDS WHEN IT BREAKS.", "IT LEAVES A DOUBLE BEHIND, AND THE DEAD NEARBY GO FOR THAT.",
        "IT SENDS MISSILES HOME TO THEIR THROWER AT TWICE ITS STRENGTH.", "HEALING PAST FULL BECOMES A SMALL WARD.",
        "IT MENDS TWO HUNDREDTHS OF YOUR HEALTH A SECOND WHILE IT LASTS.", "THE FROZEN TAKE DOUBLE.", "IT REACHES THE LIVING TOO.",
        "WHAT IT SLOWS TAKES A QUARTER MORE.", "EACH KILL HEALS A TWENTIETH OF YOUR HEALTH." };
    static const char *const STAT_DOES[ST_POW] = { "TAKEN OFF EVERY BLOW, HALF OFF FIRE AND MAGIC.", "A CHANCE THAT A BLOW PASSES THROUGH YOU.",
        "MORE HEALTH.", "HEALTH BACK EVERY SECOND.", "POISON LASTS SHORTER AND BITES LESS.", "FIRE AND BURNING HURT LESS.",
        "SLOWING AND BEING HELD LAST SHORTER.", "YOU WALK FASTER.", "YOUR STEPS WAKE FEWER OF THE DEAD.", "YOUR SPELLS COME ROUND SOONER.",
        "YOUR SPELLS TAKE LESS SPIRIT.", "EVERY SPELL HITS HARDER.",
        "A SHARE OF EVERY SPELL'S DAMAGE IS ADDED AS FIRE, AS MUCH AS FIRE BITES WHAT IT HITS. THE SIX SHARE ONE CAP.",
        "A SHARE OF EVERY SPELL'S DAMAGE IS ADDED AS FROST, AS MUCH AS FROST BITES WHAT IT HITS. THE SIX SHARE ONE CAP.",
        "A SHARE OF EVERY SPELL'S DAMAGE IS ADDED AS LIGHTNING, AS MUCH AS LIGHTNING BITES WHAT IT HITS. THE SIX SHARE ONE CAP.",
        "SOUL DRAIN'S HEALING, FROST'S SLOW, HELLFIRE'S BURNING FLOOR AND EVERY RUNE, STRONGER.",
        "MORE FOR EVERY GOLD PIECE YOU PICK UP.", "THE CHANCE THAT WHAT YOU SLAY DROPS GOLD OF ITS RANK, AND NOW AND THEN SOMETHING MORE.",
        "WHAT IS DONE TO YOU BY SPIRIT HURTS LESS, AND WHAT DRAINS SOULS TAKES FEWER.", "YOUR VESSEL HOLDS MORE SPIRIT." };
    char does[160];                                                /* an element's Power, Efficiency, Insight */
    #define STAT_WORDS(st) (st < ST_POW ? STAT_DOES[st] : (stat_does(st, does, sizeof does), does))
    static const char *const HZ_NAME[NHZ] = { "THE KNIGHT'S ROCKFALL", "THE DEACON'S HELLRENT", "THE CISTERN'S BILE", "THE VOID", "THE SUNDERING ARC", "THE GRASPING DARK",
        "THE SWINGING CENSER", "THE EMBALMERS' BREATH", "VOTIVE SPILL", "THE COLD VAULT", "THE HUNGRY RELIQUARY",
        "SLUICE BURST", "THE GALVANIC POOL", "WEB SNARE", "SILT GAS", "THE WEEPING SAINT",
        "TIMBER COLLAPSE", "FIREDAMP", "THE RUNAWAY CART", "THE LODESTONE SEAM", "THE TITHE CIRCLE",
        "FISSURE BREATH", "ASHFALL", "SINGING CRYSTAL", "SHATTERGLASS", "THE DARK COLD", "THE BLACK MIRROR" };
    static const char *const HZ_DOES[NHZ] = {
        "THE CEILING GROANS AND A SHADOW GROWS ON THE FLOOR, THEN STONE FALLS THERE. IT HURTS ONCE, AND NEVER BLOCKS THE WAY.",
        "CRACKS SPREAD ACROSS THE FLOOR, THEN FIRE BURSTS FROM THEM AND BURNS WHOEVER STANDS ON THEM.",
        "THE DRAINS SPILL A SLUDGE THAT POISONS AND SLOWS WHOEVER WADES IN IT.",
        "PURPLE ZONES SHOW ON THE FLOOR AND AS A SHADED VOLUME BEFORE THEY COME. INSIDE, THEY SAP HEALTH, SLOW YOU AND SILENCE YOU: NO SPELLS, NO ABILITIES, NO RUNES.",
        "THE KNIGHT RAISES ITS SWORD, AND ITS SWEEP CUTS ALL BEFORE IT IN AN ARC.",
        "THE FLOOR DARKENS, THEN HANDS OF THE DARK RISE THERE. THEY STRIKE AND HOLD.",
        "A GREAT IRON CENSER SWINGS ACROSS A HALL ON A SLOW ARC, CREAKING AT EACH END, ITS SHADOW AND ITS SMOKE BEFORE IT. STRUCK, YOU ARE KNOCKED A CELL ASIDE.",
        "A BURIAL SLAB CRACKS WITH A HISS AND A SEAM OF LIGHT, AND A GREY-GREEN VAPOUR SETTLES BEFORE ITS NICHE FOR FOUR SECONDS. IT POISONS.",
        "A RACK OF VOTIVE CANDLES GOES OVER NEAR YOU. THE WAX RUNS ALONG THE FLOOR, THEN BURNS THERE A WHILE.",
        "A CRACK IN THE WALL BREATHES COLD. RIME CREEPS OVER THE FLOOR BEFORE IT, AND ON IT YOU ARE CHILLED AND SLOWED.",
        "A GILDED RELIQUARY LIFTS ITS LID WHEN YOU COME NEAR. ITS VIOLET HAZE DRINKS A SOUL A SECOND AND A LITTLE HEALTH, AND SLOWS YOU, TILL YOU LEAVE IT OR IT HAS FED.",
        "A SLUICE CLANKS OPEN IN THE WALL AND A SURGE RUNS DOWN THE HALL. IT CHILLS, SLOWS AND CARRIES YOU TWO CELLS ALONG.",
        "LIGHTNING COMES DOWN INTO STANDING WATER. THE WATER GLOWS AND CRACKLES, THEN EVERYONE IN IT IS SHOCKED, AND A SPELL BEING GATHERED MAY BREAK.",
        "A WEB ACROSS A DOORWAY, ALL BUT UNSEEN. WALK INTO IT AND IT HOLDS YOU A MOMENT, AND THE SPIDERS NEAR WAKE.",
        "BUBBLES RISE FROM THE SILT AND A CLOUD HANGS THERE, POISONING. A FIRE SPELL THROUGH IT SENDS IT UP IN FLAME.",
        "A SAINT WEEPS IN HER ALCOVE. THE POOL SPREADING BEFORE HER DRAWS YOUR SOULS WHILE YOU STAND IN IT, AND HER EYES BURN BRIGHTER.",
        "THE PROPS CREAK AND DUST SIFTS DOWN, THEN THE ROOF COMES DOWN ALONG A STRIP, AND THE RUBBLE CLOSES IT A WHILE.",
        "GAS POOLS IN THE STOPES, A SHIMMER OVER THE FLOOR. BREATHING IT POISONS. FIRE SENDS IT UP, AND IT GATHERS AGAIN.",
        "A CART COMES DOWN THE RAILS WITH A RUMBLE BEFORE IT. IT STRIKES WHATEVER STANDS IN ITS WAY, AND THROWS YOU ASIDE.",
        "A GREEN-VEINED SEAM IN THE ROCK CRACKLES, THEN ARCS TO ANYTHING NEAR IT. A SPELL BEING GATHERED MAY BREAK.",
        "A RING OF THE CULT'S CHALK MARKS. STEP IN, AND IT GLOWS AND TAKES ITS TITHE OF SOULS FOR EVERY SECOND YOU STAY.",
        "A FISSURE IN THE WALL GLOWS BRIGHTER, THEN BREATHES FLAME ACROSS THE FLOOR BEFORE IT, ON A RHYTHM.",
        "ASH COMES DOWN IN A ROOM. THE AIR THICKENS UNDER IT, AND YOU CHOKE WHILE YOU STAND THERE.",
        "A CRYSTAL HUMS HIGHER AND BRIGHTER, THEN DISCHARGES IN ARCS TO THE THREE NEAREST THINGS, YOU OR THE DEAD.",
        "THE GLASS FLOOR CRACKS IN A STAR, THEN SPIKES STAND UP THERE FOR A MOMENT.",
        "WHERE THE FIRE HAS GONE OUT OF THE ROCK, RIME LIES OVER THE FLOOR. ON IT YOU ARE CHILLED AND SLOWED.",
        "A SHEET OF OBSIDIAN IN THE WALL. WHILE YOU STAND IN ITS SIGHT YOUR REFLECTION GROWS IN IT, AND IT DRINKS YOUR SOULS." };
    static const int HZ_BOSS[6] = { BOSS_KNIGHT, BOSS_DEACON, BOSS_SPIDER, BOSS_WRAITH, BOSS_KNIGHT, BOSS_WRAITH };
    CxEntry e[CX_MAX];
    int n = codex_entries(e), pg = clampi(G.cx_page, 0, n - 1);
    const CxEntry *c = &e[pg];
    dim_view(f);
    char line[240], name[48];
    static char count[24];
    snprintf(count, sizeof count, "%d OF %d", pg + 1, n);
    switch (c->sec) {
    case CX_ABIL: snprintf(name, sizeof name, "%s", ABIL_NAME[c->i]); break;
    case CX_RUNE: snprintf(name, sizeof name, "%s", DEF_NAME[c->i]); break;
    case CX_BOSS: snprintf(name, sizeof name, "%s", BA_NAME[c->i]); break;
    case CX_STAT: snprintf(name, sizeof name, "%s", STAT_NAME[c->i]); break;
    case CX_HAZ: snprintf(name, sizeof name, "%s", HZ_NAME[c->i]); break;
    case CX_PUZZLE: snprintf(name, sizeof name, "%s", PZ_NAME[c->i]); break;
    case CX_TRIAL: snprintf(name, sizeof name, "%s", TRI_NAME[c->i]); break;
    case CX_WAYS: snprintf(name, sizeof name, "%s", WAY_NAME[clampi(c->i, 0, NWAY - 1)]); break;
    case CX_ELEM: snprintf(name, sizeof name, c->i ? "THE DEAD'S ELEMENTS" : "WHAT HURTS WHAT"); break;
    case CX_LINE: snprintf(name, sizeof name, "%s", ALIGN[c->i].name); break;
    case CX_CLASS: snprintf(name, sizeof name, "%s", CLASSES[c->i % NELEM].name); break;
    case CX_SOUL: {
        static const char *const SN[NCXS] = { "THE VESSEL", "THE DEAD BELL", "THE WEEPING FONT", "LEVELS REPLAYED", "SOUL SAP" };
        snprintf(name, sizeof name, "%s", SN[clampi(c->i, 0, NCXS - 1)]);
        break;
    }
    case CX_CASTER: snprintf(name, sizeof name, "%s", c->i == CXC_ATONE ? "LEVELS OF ATONEMENT" : c->i == CXC_LUCK ? "LUCK" : c->i == CXC_OVER ? "OVERLEVELS"
                                                    : CS_NAME[clampi(c->i - CXC_CORNER, 0, NCS - 1)]); break;
    case CX_AUG: snprintf(name, sizeof name, "%s", aug_name(c->i / NAUGT, c->i % NAUGT, aug_of(c->i / NAUGT, c->i % NAUGT))); break;
    default: snprintf(name, sizeof name, "HER CLAIM"); break;
    }
    /* the stone (docs/PLAN-ui.md section 6.2): the entry's name across the top, its part and the parts as lozenges, the page in a well */
    ui_screen(f, name, R_GOLD + 12, count, UI_M(11), 0, G.ui_t);
    gfx_text8(f, 14, 31, CX_PART[c->sec], R_STONE + 10, 1);
    {
        int has[NCX] = { 0 };
        for (int i = 0; i < n; i++) has[e[i].sec] = 1;
        for (int sct = 0; sct < NCX; sct++) {                       /* small, so the longest part's name (to x 230) clears them */
            int lx = 305 - (NCX - 1 - sct) * 5, ly = 34, on = sct == c->sec;
            for (int d = 0; d < 2; d++) for (int q = -d; q <= d; q++) {
                uint8_t col = on ? (uint8_t)(R_GOLD + 12) : has[sct] ? UI_M(9) : UI_W(1);
                gfx_px(f, lx + q, ly - 1 + d, col); gfx_px(f, lx + q, ly + 1 - d, col);
            }
        }
    }
    int y = 40;
    const Gear *g = &RUN.gear;
    switch (c->sec) {
    case CX_ABIL: {
        int a = c->i;
        y += text_wrap(f, 14, y, ABIL_DOES[a], 36, R_STONE + 13, a >= AB_CLASS ? 4 : 3) * 9 + 4;
        for (int q = Q_UNCOMMON; q < NQUAL; q++, y += 9) {           /* never on a Common piece */
            int v0 = piece_grade(a, q, G.level, 0), v1 = piece_grade(a, q, G.level, 1);
            switch (a) {
            case AB_KEYMAKER: snprintf(line, sizeof line, "%d CELLS", v0); break;
            case AB_COMMUNE: snprintf(line, sizeof line, "%d CELLS, %d.%d S", v0, v1 / 10, v1 % 10); break;
            case AB_THORNS: snprintf(line, sizeof line, "%d BACK, AT LEVEL %d", v0, G.level); break;
            case AB_MIRRORED: snprintf(line, sizeof line, "%d%% OF THEM, AT %d%%", v0, v1); break;
            case AB_BLOCK: snprintf(line, sizeof line, "%d%% LESS", v0); break;
            default: snprintf(line, sizeof line, "+%d%%", v0); break;
            }
            gfx_text8(f, 14, y, QUAL_NAME[q], (uint8_t)qual_text(q), 1);
            gfx_text8(f, 124, y, line, R_STONE + 12, 1);
        }
        float cap = gear_ability_cap(a);
        if (cap > 0.0f) { snprintf(line, sizeof line, "SUMMED OVER WHAT YOU WEAR, TO %.0f%s AT MOST.", cap, a == AB_KEYMAKER || a == AB_COMMUNE ? " CELLS" : "%"); y += 2 + text_wrap(f, 14, y + 2, line, 36, R_STONE + 10, 2) * 9; }
        y += 4;
        if (a >= AB_CLASS) {                                       /* a class's (1.0.0b21): rolled for it alone */
            snprintf(line, sizeof line, "THE %s'S ALONE: ONE ABILITY IN THREE ON A PIECE OF UNCOMMON OR BETTER, WHILE YOU ARE ONE. ANYONE ELSE'S DOES NOTHING.", CLASS_SHORT[a - AB_CLASS]);
            y += text_wrap(f, 14, y, line, 36, R_BONE + 11, 4) * 9 + 4;
        } else
        y += text_wrap(f, 14, y, "ON ANY PIECE BUT A BAG, UNCOMMON OR BETTER: ONE IN TEN, A QUARTER AT RARE, HALF AT ULTRA-RARE, EVERY EXQUISITE ONE.", 36, R_BONE + 11, 4) * 9 + 4;
        float have = gear_ability(g, a, 0);
        if (have > 0.0f && a >= AB_CLASS && cls_el() != a - AB_CLASS) snprintf(line, sizeof line, "YOU WEAR IT, BUT IT IS NOT YOURS");
        else if (have > 0.0f) snprintf(line, sizeof line, "YOU WEAR IT: %.0f%s", have, a == AB_KEYMAKER || a == AB_COMMUNE ? " CELLS" : (a == AB_THORNS ? " BACK" : "%"));
        else snprintf(line, sizeof line, "YOU WEAR NONE OF IT NOW");
        gfx_text8(f, 14, y, line, have > 0.0f ? (uint8_t)(R_GOLD + 11) : (uint8_t)(R_STONE + 9), 1);
        break;
    }
    case CX_RUNE: {
        int k = c->i;
        Offer o; memset(&o, 0, sizeof o); o.kind = GK_DEF; o.id = k;
        snprintf(line, sizeof line, "%s. CAST BY ITSELF WHEN IT IS NEEDED.", gear_blurb(&o));
        y += text_wrap(f, 14, y, line, 36, R_STONE + 13, 3) * 9 + 4;
        for (int q = 1; q <= RUNE_EXQ; q++, y += 9) {
            snprintf(line, sizeof line, "AT TIER %d, EVERY %.0f S", rune_tier(q), def_cooldown(q));
            gfx_text8(f, 14, y, QUAL_NAME[q - 1], (uint8_t)qual_text(q - 1), 1);
            gfx_text8(f, 124, y, line, R_STONE + 12, 1);
        }
        if (RUN.cx_exq >> k & 1) {
            snprintf(line, sizeof line, "EXQUISITE, IT DOES MORE: %s", EXQ_DOES[k]);
            y += 2 + text_wrap(f, 14, y + 2, line, 36, R_GOLD + 10, 3) * 9;
        }
        y += 4;
        gfx_text8(f, 14, y, "FROM THE RUNE SHRINE", R_BONE + 11, 1);
        y += 13;
        if (g->def[k] > 0) snprintf(line, sizeof line, "YOU HOLD IT: %s", QUAL_NAME[clampi(g->def[k], 1, NQUAL) - 1]);
        else snprintf(line, sizeof line, "NOT HELD NOW");
        gfx_text8(f, 14, y, line, g->def[k] > 0 ? (uint8_t)(R_GOLD + 11) : (uint8_t)(R_STONE + 9), 1);
        break;
    }
    case CX_BOSS: {
        int b = c->i;
        static const char *const RK[ABIL_TOP] = { "I", "II", "III" };
        for (int r = 1; r <= ABIL_TOP; r++) {
            char d[200];
            ab_text(b, r, d, sizeof d);
            gfx_text8(f, 14, y, RK[r - 1], ramp_at(BA_RAMP[b], 13), 1);
            y += text_wrap(f, 46, y, d, 32, R_STONE + 12, 4) * 9 + 3;
        }
        y += 2;
        snprintf(line, sizeof line, "THE HEXED TOTEM, FOR %s", BOSSES[b].scrip);
        y += text_wrap(f, 14, y, line, 36, R_BONE + 11, 2) * 9 + 4;
        if (RUN.abil[b]) snprintf(line, sizeof line, "YOURS AT RANK %s OF III", RK[clampi(RUN.abil[b], 1, ABIL_TOP) - 1]);
        else snprintf(line, sizeof line, "NOT YET UNLOCKED: %d SCRIP HELD", RUN.scrip[b]);
        gfx_text8(f, 14, y, line, RUN.abil[b] ? (uint8_t)(R_GOLD + 11) : (uint8_t)(R_STONE + 9), 1);
        break;
    }
    case CX_STAT: {
        int st = c->i;
        y += text_wrap(f, 14, y, STAT_WORDS(st), 36, R_STONE + 13, 4) * 9 + 4;
        float cap = gear_stat_cap(st);
        if (cap > 0.0f) snprintf(line, sizeof line, "IT COUNTS TO %.0f%s AT MOST", cap, st == ST_ARMOR || st == ST_HP ? "" : "%");
        else snprintf(line, sizeof line, "NO CAP: AS MUCH AS YOU CAN WEAR");
        gfx_text8(f, 14, y, line, R_STONE + 11, 1);
        y += 13;
        char kinds[96] = "";
        for (int k = IK_WRIST; k < IK_BAG; k++)
            if (stat_on_kind(st, k)) { size_t l = strlen(kinds); snprintf(kinds + l, sizeof kinds - l, "%s%s", l ? ", " : "", KIND_NAME[k]); }
        snprintf(line, sizeof line, "ON %s", kinds);
        y += text_wrap(f, 14, y, line, 36, R_BONE + 11, 2) * 9 + 4;
        char t[24];
        total_text(g, st, t, sizeof t);
        snprintf(line, sizeof line, "YOUR TOTAL: %s%s", t, stat_over(st) > 0 ? ", PAST ITS CAP" : gear_capped(g, st) ? ", AT ITS CAP" : "");
        if (stat_over(st) > 0) { eerie_text(f, 14, y, line); eerie_eye(f, 14 + gfx_text_width(line, 1) + 4, y + 1); }
        else gfx_text8(f, 14, y, line, gear_total(g, st) > 0.0f ? (uint8_t)(R_GOLD + 11) : (uint8_t)(R_STONE + 9), 1);
        break;
    }
    case CX_CASTER: {
        char a[16], b[16];
        if (c->i == CXC_ATONE) {
            y += text_wrap(f, 14, y, "WHAT YOU SLAY, THE SECRETS YOU FIND, THE TRIALS YOU END, A BOSS, A LEVEL CLEARED AND EACH ROOM FIRST ENTERED "
                           "ALL COUNT AS EXPERIENCE. THE MERCHANT TURNS IT INTO LEVELS OF ATONEMENT, TEN POINTS EACH TO THE FIFTIETH, "
                           "TO SPEND ON WHAT YOU ARE: ADDED TO WHAT YOU WEAR, UNDER THE SAME CAPS. A DEATH TAKES IT ALL.", 36, R_STONE + 13, 9) * 9 + 4;
            commas(RUN.xp, a, sizeof a); commas(atone_need(RUN.atone + 1), b, sizeof b);
            snprintf(line, sizeof line, "YOURS: %d, %d POINTS. EXPERIENCE %s, THE NEXT AT %s", RUN.atone, RUN.points, a, b);
            text_wrap(f, 14, y, line, 36, R_GOLD + 11, 3);
        } else if (c->i == CXC_LUCK) {
            y += text_wrap(f, 14, y, STAT_DOES[ST_LUCK], 36, R_STONE + 13, 3) * 9 + 4;
            y += text_wrap(f, 14, y, conf_on() ? "TWO IN A HUNDRED FOR EVERYONE AT THE START. NO PIECE CARRIES IT: ONLY THE CONFESSION OF SURVIVAL RAISES IT."
                                               : "TWO IN A HUNDRED FOR EVERYONE AT THE START. NO PIECE CARRIES IT: ONLY YOUR LEVELS OF ATONEMENT RAISE IT, A TENTH A POINT.", 36, R_BONE + 11, 4) * 9 + 4;
            snprintf(line, sizeof line, "YOURS: %.1f%% OF %.0f%%", gear_total(g, ST_LUCK), gear_stat_cap(ST_LUCK));
            gfx_text8(f, 14, y, line, R_GOLD + 11, 1);
        } else if (c->i == CXC_OVER) {
            y += text_wrap(f, 14, y, conf_on() ? "PAST THE FIFTIETH LEVEL OF ATONEMENT THERE ARE NO MORE POINTS. AT EVERY FIFTH LEVEL AFTER, ONE CONFESSION "
                                                 "WHOLE AT V GOES ON TO VI, VII AND BEYOND, AND THE CAPS OF WHAT IT RAISES WITH IT. THE DEAD SEE WHAT YOU HAVE BECOME."
                                               : "PAST THE FIFTIETH LEVEL OF ATONEMENT THERE ARE NO MORE POINTS. AT EVERY FIFTH LEVEL AFTER, ONE LINE OF YOUR CHOOSING "
                                                 "GOES PAST ITS CAP: FIVE MORE, AND ITS CAP WITH IT (LUCK ONE). THE DEAD SEE WHAT YOU HAVE BECOME.", 36, R_STONE + 13, 7) * 9 + 4;
            int tot = 0;
            for (int l = 0; l < NLINE; l++) tot += RUN.over[l];
            for (int cf = 0; cf < NCONF; cf++) tot += RUN.conf_over[cf];
            snprintf(line, sizeof line, "YOURS: %d CHOSEN, %d TO CHOOSE", tot, imax(0, over_due()));
            gfx_text8(f, 14, y, line, R_PURPLE + 13, 1);
        } else {
            int cs = clampi(c->i - CXC_CORNER, 0, NCS - 1);
            y += text_wrap(f, 14, y, CS_DOES[cs], 36, R_STONE + 13, 5) * 9 + 4;
            y += text_wrap(f, 14, y, "A CORNERSTONE: ONE CHOSEN, FREE, AT EACH TENTH LEVEL OF ATONEMENT, ON THE PLAYER ABILITIES SCREEN. A DEATH TAKES IT.", 36, R_BONE + 11, 4) * 9 + 4;
            if (cs == CS_WIND) gfx_text8(f, 14, y, RUN.wind_spent ? "YOURS, SPENT FOR THIS RUN" : "YOURS, READY", RUN.wind_spent ? (uint8_t)(R_STONE + 9) : (uint8_t)(R_GOLD + 11), 1);
            else gfx_text8(f, 14, y, "YOURS", R_GOLD + 11, 1);
        }
        break;
    }
    case CX_ELEM: {                                                /* docs/PLAN-b11.md, sections 2 and 4 */
        if (c->i == 1) {                                           /* the dead's (docs/PLAN-combat.md, section 7) */
            y += text_wrap_ex(f, 14, y, "EVERY KIND IS WEAK TO ONE ELEMENT AND SHRUGS OFF ONE.", 36, R_STONE + 13, 2, 9) * 9 + 5;
            glyph_marked(f, 14, y, EL_FIRE, 2);
            gfx_text8(f, 40, y, "BITES HALF AGAIN AS HARD", R_MOSS + 13, 1);
            y += 11;
            glyph_marked(f, 14, y, EL_FROST, -2);
            gfx_text8(f, 40, y, "BITES HALF AS HARD", R_BLOOD + 13, 1);
            y += 15;
            y += text_wrap_ex(f, 14, y, "FROM RANK III IT SHRUGS OFF ONE OF YOUR LINES MORE, FROM V ANOTHER: NEVER WHAT IT IS WEAK TO, "
                              "NEVER THE LAST OF YOURS THAT BITES IT. A PACK SHARES THEM.", 36, R_STONE + 13, 5, 9) * 9 + 5;
            text_wrap(f, 14, y, "STRUCK WITH ONE, IT RINGS DULL AND SHOWS IT, BARRED. A FIFTH OF ITS HEALTH IN THAT ELEMENT "
                      "CRACKS IT.", 36, R_BONE + 11, 3);
            break;
        }
        static const char *const NATURE[NELEM] = { "IT BURNS; SETS ALIGHT", "IT CHILLS; FOUR FREEZE", "AT ONCE, AND STUNS",
                                                   "IT SICKENS, AND SLOWS", "IT HEALS YOU; UNBINDS", "IT PIERCES" };   /* 1.0.0b19: and
                                                                      what each leaves on the dead (docs/PLAN-combat.md section 6) */
        static const char *const FROM[NELEM] = { "SKULLS, IMPS, HELLRENT", "THE WRAITH'S TOUCH", "NOTHING YET",
                                                 "SPIDERS, DROWNED, BILE", "SHADES; BATS FROM II", "BLOWS, BITES, BONES, ROCK" };
        y -= 2;
        for (int el = 0; el < NELEM; el++, y += 18) {
            elem_glyph(f, 14, y + 1, el, 0);
            gfx_text8(f, 26, y, ELEM_NAME[el], ramp_at(ELEM_RAMP[el], 13), 1);
            gfx_text8(f, 110, y, NATURE[el], R_STONE + 13, 1);
            gfx_text8(f, 110, y + 9, FROM[el], R_STONE + 9, 1);
        }
        text_wrap(f, 14, y + 1, "LEARNT OF EACH KIND: AN ELEMENT FROM FIVE SLAIN OR TWELVE HITS, ITS HARM TO YOU FROM THREE HURTS.", 36, R_BONE + 11, 3);
        break;
    }
    case CX_LINE: {                                                /* its nodes in order, those bought in gold, the closed path chained */
        int el = c->i;
        for (int nd = 0; nd < NNODE; nd++, y += 9) {
            const ANode *a = &ALIGN[el].n[nd];
            int got = align_full(el, nd) && (nd != ND_OPEN || align_open(el)), chained = align_chained(el, nd);
            snprintf(line, sizeof line, "%s%s", nd >= ND_B1 ? "B  " : nd >= ND_A1 ? "A  " : "", a->name);
            gfx_text8(f, 14, y, line, got ? (uint8_t)(R_GOLD + 11) : chained ? (uint8_t)(R_STONE + 6) : ramp_at(ELEM_RAMP[el], 11), 1);
            if (align_point(el, nd)) snprintf(line, sizeof line, "%d OF %d", RUN.al[el][nd], a->cost);
            else if (nd == ND_OPEN && align_open(el) && RUN.al[el][nd] < a->cost) snprintf(line, sizeof line, "OPEN");   /* Fire's, or the class's */
            else snprintf(line, sizeof line, "%d", a->cost);
            gfx_text8(f, W - 14 - gfx_text_width(line, 1), y, line, got ? (uint8_t)(R_GOLD + 11) : (uint8_t)(R_STONE + 9), 1);
        }
        snprintf(line, sizeof line, "RANK %d. THE LAMPSTAND, NORTH OF HER ROOM, SAYS WHAT EACH DOES.", align_rank(el));
        text_wrap(f, 14, y + 4, line, 36, R_BONE + 11, 2);
        break;
    }
    case CX_AUG: {                                                 /* C6: its line and step, what it does now and at III, its credits */
        static const char *const STEP[NAUGT] = { "ITS OPENING", "ITS ABILITY", "ITS ULTIMATE" };
        int el = c->i / NAUGT, t = c->i % NAUGT, w = aug_of(el, t), r = aug_rank(el, t), cr = RUN.aug_cr[el][t];
        static const char *const RN[4] = { "", "I", "II", "III" };
        snprintf(line, sizeof line, "%s, %s", ALIGN[el].name, STEP[t]);
        gfx_text8(f, 14, y, line, ramp_at(ELEM_RAMP[el], 12), 1); y += 11;
        char does[200];
        aug_does(el, t, w, r >= 2 ? (r >= 3 ? 1.0f : 0.75f) : 0.5f, does, sizeof does);
        snprintf(line, sizeof line, "%s %s", r ? "NOW:" : "AT RANK I:", does);
        y += text_wrap(f, 14, y, line, 36, R_STONE + 13, 5) * 9 + 3;
        if (r < 3) {
            aug_does(el, t, w, 1.0f, does, sizeof does);
            snprintf(line, sizeof line, "AT RANK III: %s", does);
            y += text_wrap(f, 14, y, line, 36, R_STONE + 10, 5) * 9 + 3;
        }
        if (r >= 3) snprintf(line, sizeof line, "RANK III, WHOLE. %d BANKED.", RUN.aug_bank[el]);
        else snprintf(line, sizeof line, "%s%s, %d OF %d CREDITS. %d BANKED.", r ? "RANK " : "", r ? RN[r] : "UNRANKED", cr, AUG_CR[r], RUN.aug_bank[el]);
        gfx_text8(f, 14, y, line, R_GLOW + 12, 1); y += 11;
        text_wrap(f, 14, y, "A KILL BY THE LINE'S SPELL IS A CREDIT, A THING STRUCK TWO, A BOSS TEN; THEY FEED THE NEWEST CHOSEN. "
                  "IT IS YOURS TILL THE LINE IS UNBOUND.", 36, R_BONE + 11, 4);
        break;
    }
    case CX_CLASS: {
        if (c->i >= NELEM) {                                       /* its second page: what they bring (D5, section 9), the caps it
                                                                      lifts beside them and the rest after (docs/BRIEF-caps.md), its pair */
            int e = c->i - NELEM, st, used[3] = { 0, 0, 0 };
            char nm[40], v[16], cv[16], cn[40];
            gfx_text8(f, 14, y, "WHAT THEY BRING", R_GOLD + 11, 1); y += 11;
            for (int i = 0; class_sheet_row(e, i, nm, sizeof nm, v, sizeof v, &st) && i < 10; i++, y += 9) {
                gfx_text8(f, 14, y, nm, R_STONE + 13, 1);
                gfx_text8(f, 224 - gfx_text_width(v, 1), y, v, R_GLOW + 12, 1);
                for (int j = 0; st >= 0 && j < 3 && class_cap_row(e, j, cn, sizeof cn, cv, sizeof cv); j++)
                    if (!used[j] && !strcmp(cn, STAT_NAME[st])) { used[j] = 1; gfx_text8(f, 232, y, cv, R_GHOST + 12, 1); }
            }
            for (int j = 0; j < 4 && class_cap_row(e, j, cn, sizeof cn, cv, sizeof cv); j++) {
                if (j < 3 && used[j]) continue;
                gfx_text8(f, 14, y, cn, R_STONE + 13, 1); gfx_text8(f, 232, y, cv, R_GHOST + 12, 1); y += 9;
            }
            snprintf(line, sizeof line, "%s HURTS THEM LESS, %s MORE. EACH MAX IS THEIR OWN CAP, OVER EVERYONE'S; "
                     "A RESIST BELOW NOTHING LETS ITS ELEMENT HURT THE MORE.", ELEM_NAME[CLASS_OWN[e].strong], ELEM_NAME[CLASS_OWN[e].weak]);
            text_wrap(f, 14, y + 4, line, 36, R_BONE + 11, 5);
            break;
        }
        int n2 = text_wrap_ex(f, 14, y, CLASSES[c->i].story, 36, R_STONE + 13, 10, 8);
        y += n2 * 8 + 4;
        y += text_wrap_ex(f, 14, y, CLASSES[c->i].gift, 36, R_GOLD + 11, 3, 8) * 8 + 4;
        text_wrap(f, 14, y, "A LIFE KEEPS ONE; ANOTHER FROM ATONEMENT 20, FOR FIVE CURSED COINS. WHAT THEY BRING IS ON THE NEXT PAGE.", 36, R_BONE + 11, 3);
        break;
    }
    case CX_SOUL: {
        static const char *const SD[NCXS] = {
            "SOULS ARE YOUR SPIRIT, AND SPELLS SPEND IT BY THEIR RANK. THE VESSEL HOLDS TWENTY AND TWO A LEVEL OF ATONEMENT, MORE "
            "FROM THE SIN-EATER, THE VESSEL WORN, COMMUNION AND SPIRIT MASTERY; WHAT IS OVER IT SLIPS AWAY. THE DEAD LEAVE THEIR SOULS "
            "WHERE THEY FALL, FOR SIX SECONDS: WALK THROUGH THEM. SHE TAKES TEN FOR EXPERIENCE.",
            "A CURSED COIN TURNS IT BLACK, FOR GOOD. THEN SOULS TOLL IT FOR THE NEXT LEVEL: TWO FIFTHS MORE OF THE DEAD AND OF THEIR "
            "GOLD, HALF THE EXPERIENCE, A BOSS'S WAVES A THIRD LARGER AND A SCRIP MORE. ONE ATTEMPT. ON A LEVEL CLEARED BEFORE, MORE "
            "SOULS SHOW ITS SECRETS FROM THE START.",
            "A CURSED COIN AND IT WEEPS RED, FOR GOOD. THEN, ONCE A VISIT, SOULS FOR A TENTH MORE EXPERIENCE ON THE NEXT LEVEL.",
            "AT THE ACCURSED PORTAL, A LEVEL CLEARED BEFORE, AGAIN, FOR FIVE SOULS AND ITS NUMBER: HER ROOM BEFORE IT, THE LEVEL NEW, "
            "THEN BACK. GOLD, SOULS AND EXPERIENCE AS EVER; NO CURSED COINS.",
            "BATS FROM RANK II LATCH ON INSTEAD OF BITING, AND FEED: HEALTH AND SOULS, AND YOU WALK SLOWER. TURN HARD ONE WAY AND BACK, "
            "OR STRAFE LEFT AND RIGHT, AGAIN AND AGAIN, TO SHAKE IT OFF. BLINK AND SWIFT DODGE THROW IT OFF. RESIST SPIRIT TAKES ITS SHARE." };
        y += text_wrap(f, 14, y, SD[clampi(c->i, 0, NCXS - 1)], 36, R_STONE + 13, 12) * 9 + 4;
        if (c->i == CXS_VESSEL) { snprintf(line, sizeof line, "YOURS: %d.%d OF %d", RUN.souls, clampi((int)(RUN.soul_frac * 10.0f), 0, 9), vessel()); gfx_text8(f, 14, y, line, R_GLOW + 12, 1); }
        break;
    }
    case CX_PUZZLE: {                                              /* 1.0.0b13: what it asks, and the tally */
        static const char *const PZ_DOES[NPZK] = {
            "SKULLS MARKED WITH THE ORDER'S RANKS, EACH TO THE NICHE CUT WITH ITS OWN.", "FOUR BELLS, RUNG IN THE ORDER THE BANNER GIVES.",
            "FIVE KNIGHTS ON THEIR TOMBS, EACH HEAD TURNED TO WHOM ITS EPITAPH SAYS.", "RELIQUARIES OF BEADS ON A GREAT SCALE, LAID UNTIL IT HANGS LEVEL.",
            "NINE VOTIVE CANDLES, LIT AS THE WINDOW SHOWS THEM.", "THREE URNS MOVED PLINTH TO PLINTH, NEVER A LARGER ON A SMALLER.",
            "A DIAL OF THE CANONICAL HOURS, SET TO THE HOUR THE BOOK NAMES.", "THREE CISTERNS POURED ONE INTO ANOTHER UNTIL THE MEASURE IS RIGHT.",
            "SAINTS WITH EMPTY HANDS, EACH GIVEN HER OWN EMBLEM.", "CULVERTS TURNED TO CARRY THE WATER FROM THE SPOUT TO THE MILL WHEEL.",
            "STEPPING STONES OVER THE FLOOD: SOME HOLD, SOME SINK.", "THREE WHEELS OF SAINTS' SIGNS, TURNED IN THE ORDER THEY DIED.",
            "BELLS AT THE WATER LINE, RUNG DEEPEST OR SHALLOWEST AS THE CARVING SAYS.", "SWITCH LEVERS SET SO THE CART RUNS TO THE RIGHT BUFFER.",
            "SACKS OF ORE IN THE HOIST'S BOX, TO THE WEIGHT THE BOARD ASKS.", "THE MINERS' TAGS HUNG FOR THOSE WHO CAME UP FROM THE LAST SHIFT.",
            "A FOSSIL'S BONES, EACH TO THE SOCKET OF ITS SHAPE.", "SEVEN STANDING STONES, THEIR MARKS TURNED AS THE CULT'S LITANY RUNS.",
            "LAMPS DOWN A SHAFT, LIT FOR THE GALLERIES THAT LOST MEN.", "PRISMS TURNED TO BEND A BEAM OF THE MAGMA'S LIGHT INTO A GEODE.",
            "STATUES OF A DEAD COURT, EACH SET IN ITS PLACE FROM THE THRONE.", "A FLOOR OF GLASS: EVERY TILE LIT, NONE STEPPED ON TWICE.",
            "CRYSTALS THAT SING A PHRASE, STRUCK BACK IN THE SAME ORDER.", "TILES OF GOLD VEIN TURNED TO JOIN THE IDOL'S EYES TO THE LOCK.",
            "VENTS OPENED QUICKLY, IN AN ORDER THAT CRUSTS A PATH OVER THE MAGMA.", "NINE NUMBERED STONES LAID SO EVERY LINE ADDS TO THE SAME.",
            "DIALS OF LETTERS SET TO A WORD CUT IN PIECES ROUND THE ROOM." };
        y += text_wrap(f, 14, y, PZ_DOES[c->i], 36, R_STONE + 13, 4) * 9 + 4;
        y += text_wrap(f, 14, y, "IT OPENS A SEALED WING, A HOARD BEHIND IT; ON ONE PUZZLE LEVEL IN THREE, THE WAY ON. ONLY THOUGHT OPENS IT, "
                       "NEVER A SPELL. THE ANSWER IS WRITTEN DOWN SOMEWHERE ON THE LEVEL.", 36, R_STONE + 11, 7) * 9 + 4;
        snprintf(line, sizeof line, "PUZZLES SOLVED: %d", RUN.deed_pz);
        text_wrap(f, 14, y, line, 36, R_BONE + 11, 1);
        break;
    }
    case CX_WAYS: {                                                /* 1.0.0b18: what it does, its tell, how it is answered */
        static const char *const DOES[NWAY] = {
            "THE ORDER'S DEAD IN A LINE: ONE OR TWO SHIELDS AT THE HEAD, THE REST BEHIND, WALKING A ROUND OR HOLDING A ROOM.",
            "BONE HOUNDS, TWO TO FOUR. THEY TAKE TURNS AT YOU AND CIRCLE WHILE THEY WAIT; ONE GOES FOR YOUR BACK.",
            "BATS ASLEEP ON THE CEILING OF THE DIMMEST ROOM. A NOISE, A STEP CLOSE BY OR A LIGHT, AND ALL OF IT DROPS.",
            "THE BURIED, THREE OR FOUR UNDER ONE ROOM'S GRAVES OR PUDDLES. ONE RISING STIRS THE REST, NEAREST YOU FIRST.",
            "GHOULS HEAR A DEATH THREE TIMES AS FAR AS OTHERS DO, AND COME TO FEED ON IT, ONE TO THREE TOGETHER.",
            "THE WALL SKULL SCREAMS AS IT WAKES, THE GARGOYLE SHRIEKS AS IT COMES LOOSE, THE DEACON CALLS WHEN HE FIRST SEES YOU. "
            "WHAT HEARS IT COMES TO LOOK.",
            "A BAND WAITS AT THE FAR END OF THE LEVEL AND IS LET LOOSE LATER: THE WATCH GOES TO EVERY LOUD NOISE, HOUNDS AND "
            "SPIDERS TO WHERE YOU WERE A MINUTE AGO, AND ON ALONG YOUR TRAIL.",
            "A KILL ROOM: THE TEETH BY THE INSIDE OF THE DOOR, OFTEN A BAIT IN THE HALL THAT SHOWS ITSELF AND BACKS AWAY TO THE FAR SIDE.",
            "A WALL SKULL WATCHES A HALL, AND TWO OR THREE SKELETONS HOLD THE ROOM ITS SCREAM REACHES.",
            "HUNTING SPIDERS OVERHEAD BY A WEB SPIDER'S CORNER. ITS WEB IS THEIR TRIPWIRE.",
            "MORE THAN ONE KIND TOGETHER: A KNIGHT AND HIS HOUNDS, A DEACON AND HIS CONGREGATION, A MOTHER AND HER GUARD.",
            "EMBER IMPS BY A VENT. THEY SPLIT TO EITHER SIDE TO LOB, AND FALL BACK INTO THE HEAT.",
            "GARGOYLES ON THEIR PLINTHS. WAKE ONE AND EVERY STATUE WITHIN TWELVE CELLS WAKES WITH IT.",
            "THE HUNTSMAN AND THREE OR FOUR HOUNDS. HE KEEPS BACK AND SENDS THEM IN ONE BY ONE.",
            "HE RINGS ONCE AT EACH TURN OF HIS ROUND. SEEING YOU, HE RAISES THE BELL AND RINGS IT HARD, AND EVERYTHING THAT HEARS "
            "GOES TO HIS PLACE.",
            "AT THE WRAITH'S COMING AND AT EACH WAIL, PACKS NEAR IT KEEP TO THEIR POSTS FOR FIFTEEN SECONDS.",
            "LATE IN THE DEEP, WHAT YOU WALKED PAST GATHERS IN THE EXIT'S ROOM ONCE YOU HAVE SEEN IT." };
        static const char *const TELL[NWAY] = {
            "BONES CLACKED IN TIME, LIKE A DRUM.", "A LONG RISING HOWL.", "A RUSTLE OF WINGS AND A SQUEAK.",
            "EARTH BREAKING, OR WATER STIRRING, NEAR A GRAVE.", "GROANING, AND THE SOUND OF CHEWING.", "THE CALL ITSELF.",
            "SOMETHING FAR OFF: A HOWL, A DRUM, A SKITTER, A HORN.", "A SIGN BY THE DOOR: BONES, A PUDDLE, RUBBLE; NOW AND THEN A TORCH "
            "SNUFFED, OR A SPECTRE HANGING THERE.", "THE SKULL'S SCREAM, THEN THEIR CLACKING.", "A WEB THRUMMING.",
            "THE LEADER'S SOUND: A SWORD AND A HOWL, A WHISPERED CHANT.", "HISSING, AND A POP OF EMBERS.", "GRIT FALLING FROM THEM.",
            "HIS HORN, AND THE CRACK OF THE WHIP.", "THE HANDBELL.", "THE WAIL.", "NOTHING: THEY ARE THERE WHEN YOU COME." };
        static const char *const ANSWER[NWAY] = {
            "SILENCE THE DRUM, OR STRIKE AT THE SHIELDS FROM THE SIDE. THE THROWERS KEEP A CELL BEHIND.",
            "KEEP A WALL AT YOUR BACK. FELL THE LEADER AND THE PACK BREAKS, BUT ONE RUNS TO FETCH OTHERS.",
            "GO THROUGH THE DIM ROOMS IN THE DARK, AND QUIETLY.", "BURN THE FIRST, AND STAND BACK FROM THE REST.",
            "CATCH ONE FEEDING: IT IS OPEN, AND TAKES TWICE THE HARM.", "KILL THE CALLER BEFORE IT CALLS: THEN NOTHING HEARS.",
            "BREAK YOUR TRAIL: WATER CARRIES NO SCENT, AND A SHUT DOOR STOPS A HUNTER.",
            "READ THE SIGN, AND SEND A SPELL THROUGH THE DOOR BEFORE YOU GO IN. KILL THE BAIT FROM AFAR AND THE TEETH COME OUT TO LOOK.",
            "THE SKULL FIRST.", "LOOK UP BEFORE YOU WALK UNDER A WEB.", "FELL THE LEADER: HOUNDS RUN, THE CONGREGATION LOSES ITS VOICE.",
            "COLD, AND KEEP OUT OF THE VENT'S HEAT.", "PASS WIDE OF THE PLINTHS, OR WAKE THEM ONE AT A TIME.",
            "FELL HIM, AND HIS HOUNDS RUN WILD: QUICKER, BUT CARELESS AND ALONE.", "FROST STOPS THE BELL. KILL HIM WITH IT RAISED, AND NO ONE HEARS.",
            "A BREATH TO FIGHT WHAT IS LEFT OUT OF THEIR ORDER.", "LEAVE NOTHING BEHIND YOU, OR COME IN READY." };
        int w = clampi(c->i, 0, NWAY - 1);
        y += text_wrap(f, 14, y, DOES[w], 36, R_STONE + 13, 6) * 9 + 4;
        gfx_text8(f, 14, y, "ITS TELL", R_GOLD + 10, 1); y += 10;
        y += text_wrap(f, 14, y, TELL[w], 36, R_STONE + 11, 3) * 9 + 4;
        gfx_text8(f, 14, y, "HOW IT IS ANSWERED", R_GOLD + 10, 1); y += 10;
        y += text_wrap(f, 14, y, ANSWER[w], 36, R_STONE + 11, 4) * 9 + 4;
        line[0] = 0;
        if (w == WY_AMBUSH) snprintf(line, sizeof line, "AMBUSHES COME THROUGH: %d", RUN.deed_amb);
        else if (w == WY_HUNT) snprintf(line, sizeof line, "HUNTS SHAKEN OFF: %d", RUN.deed_shaken);
        else if (w == WY_CALLER || w == WY_BELL) snprintf(line, sizeof line, "CALLERS SILENCED: %d", RUN.deed_hush);
        else if (w == WY_FILE) snprintf(line, sizeof line, "FILES BEATEN UNTOUCHED: %d", RUN.deed_file);
        else if (w == WY_HOUNDS || w == WY_BAND || w == WY_WHIP) snprintf(line, sizeof line, "PACKS BROKEN AT THE HEAD: %d", RUN.deed_heads);
        if (line[0]) text_wrap(f, 14, y, line, 36, R_BONE + 11, 1);
        break;
    }
    case CX_TRIAL: {
        static const char *const RUN_OF[4] = { "THE CRYPTS", "THE CATACOMBS", "THE HOLLOWS", "THE DEEP" };
        snprintf(line, sizeof line, "%s. %s.", RUN_OF[tri_run_of(c->i)], TRI_TASK[c->i]);
        y += text_wrap(f, 14, y, line, 36, R_STONE + 13, 4) * 9 + 4;
        y += text_wrap(f, 14, y, TRI_HOLD[c->i] ? "HELD TO THE END OF THE SAND, IT IS WON." : "DONE BEFORE THE SAND RUNS OUT, IT IS WON.", 36, R_STONE + 11, 2) * 9 + 4;
        y += text_wrap(f, 14, y, "A SKULL AND CROSSED BONES ON THE DOOR. A STEP AND A HALF IN, IT SEALS: SIXTY SECONDS. WON, IT LEAVES A HOARD AND "
                       "A DRAUGHT TO MAKE YOU WHOLE, SPIRIT TO HALF THE VESSEL, AND THE BOON: A FIFTH MORE FROM WHAT YOU SLAY. RUN OUT, THE "
                       "CURSE TAKES A FIFTH OF EVERYTHING.", 36, R_STONE + 11, 9) * 9 + 4;   /* (1.0.0b18.1: the step, the reward) */
        snprintf(line, sizeof line, "TRIALS WON: %d   LOST: %d", RUN.deed_tri, RUN.deed_lost);
        text_wrap(f, 14, y, line, 36, R_BONE + 11, 1);
        break;
    }
    case CX_HAZ: {
        int h = c->i;
        y += text_wrap(f, 14, y, HZ_DOES[h], 36, R_STONE + 13, 6) * 9 + 4;
        if (h < HZ_CENSER) {
            y += text_wrap(f, 14, y, "IT ALWAYS WARNS BEFORE IT HARMS, AND HARMS ONLY YOU.", 36, R_STONE + 11, 2) * 9 + 4;
            snprintf(line, sizeof line, "IN THE HALL OF %s, AND AS AN OMEN ON THE THREE LEVELS BEFORE IT, AT HALF ITS HARM.", BOSSES[HZ_BOSS[h]].name);
        } else {
            static const char *const RUN_OF[4] = { "THE CRYPTS", "THE CATACOMBS", "THE HOLLOWS", "THE DEEP" };
            int sap = h == HZ_RELIQ || h == HZ_SAINT || h == HZ_TITHE || h == HZ_MIRROR, fixed = !(h == HZ_VOTIVE || h == HZ_GALV || h == HZ_SILT || h == HZ_ASH || h == HZ_GLASS);
            snprintf(line, sizeof line, "%s: %s. %s, NOW AND THEN, IN THE RUNS AFTER.", ELEM_NAME[HZ_ELEM[h]], sap ? "IT DRINKS SOULS"
                     : fixed ? "IT KEEPS A RHYTHM THAT CAN BE LEARNT" : "IT ARISES NEAR YOU", RUN_OF[HZ_SET[h]]);
            y += text_wrap(f, 14, y, line, 36, R_STONE + 11, 3) * 9 + 4;
            snprintf(line, sizeof line, "%s", sap ? "TWO OR THREE A LEVEL, FROM THE SIXTH OF ITS RUN." : "NEVER MORE THAN A QUARTER OF YOUR HEALTH AT A STROKE.");
        }
        text_wrap(f, 14, y, line, 36, R_BONE + 11, 3);
        break;
    }
    default:
        text_wrap(f, 14, y, "ONLY SHE SIGNS THE LEDGER. WHILE YOU ARE IN DANGER (A CREATURE HUNTING YOU, A MISSILE COMING, HEALTH UNDER A QUARTER, "
                  "POISON, FIRE, SOMETHING HOLDING YOU, A HAZARD, A BOSS'S HALL) YOUR PAGE IS MARKED, AND A RED EYE SHOWS BY YOUR KEYS. "
                  "IT CLEARS EIGHT SECONDS AFTER. OPEN A MARKED PAGE, OR START AGAIN FROM ONE, AND SHE TAKES THE DEAD'S CLAIM: A QUARTER "
                  "TO A HALF OF YOUR GOLD, AND ONE THING YOU WEAR OR THE LAST NODE BOUGHT IN A LINE. WHATEVER YOU MEANT BY IT.", 36, R_STONE + 13, 15);
        break;
    }
    const char *keys = pk("A/D PAGES  W/S PARTS  C CLOSES");
    ui_keys(f, 160 - ui_keys_width(keys) / 2, 183, keys, UI_M(13));
}

/* the Hexed Totem's stone (docs/PLAN-ui.md section 6.1): a row a boss in a well across the top, its mark, its three ranks as
 * sockets along a groove, the scrip held, the sale on a tag; under it a band for what the next rank does (too many words
 * for a pane at the side) */
static void render_totemmenu(Frame *f) {
    static const char *const RANK[ABIL_TOP + 1] = { "NOT KNOWN", "RANK I", "RANK II", "RANK III" };
    static char right[24], cost[40], does[200], warn[64];
    dim_view(f);
    snprintf(right, sizeof right, "$%d", RUN.treasure);
    ui_slab(f, 4, 4, 312, 192, SLAB_LEDGER, ORN_NONE, G.ui_t);
    ui_text16(f, 12, 8, "THE HEXED TOTEM", FACE_INSC, R_GOLD + 12, TX_INLAID, 1);
    ui_text8(f, 308 - 8 * (int)strlen(right), 12, right, FACE_INSC, R_GOLD + 11, TX_INLAID);
    ui_well(f, 9, 28, 302, 62);
    ui_well(f, 9, 94, 302, 86);
    int r[NBOSS], n = totem_rows(r), sel = n ? clampi(G.ui_sel, 0, n - 1) : -1, bs = n ? r[sel] : -1;
    for (int b = 0; b < NBOSS; b++) {
        int y = 31 + b * 14, beaten = boss_beaten(b), rk = RUN.abil[b], on = b == bs, ramp = BA_RAMP[b];
        if (on) {                                                  /* the chosen row: lit in its groove, gilt at either end */
            for (int yy = y; yy < y + 13; yy++) for (int x = 11; x < 309; x++) if ((x + yy) & 1) gfx_px(f, x, yy, UI_W(5));
            for (int k = 0; k < 4; k++) { gfx_px(f, 11, y + 1 + k * 3, R_GOLD + 11); gfx_px(f, 308, y + 1 + k * 3, R_GOLD + 11); }
        }
        ui_mark(f, 14, y + 1, BA_MARK[b], ramp, rk ? IM_INLAID : IM_CUT, 1);
        ui_text8(f, 30, y + 3, BA_NAME[b], FACE_BODY, !beaten ? UI_M(7) : on ? C_WHITE : rk ? ramp + 13 : UI_M(13), TX_INLAID);
        for (int k = 0; k < ABIL_TOP; k++) {                        /* the ranks, I to III */
            int cx = 132 + k * 18, cy = y + 6;
            int state = !beaten ? SK_REACH : k < rk ? SK_HELD : k == rk ? (RUN.scrip[b] >= rk + 1 ? SK_READY : SK_SHORT) : SK_REACH;
            if (k) ui_groove(f, cx - 18 + 6, cy, cx - 6, cy, ramp, k < rk ? 1.0f : 0.0f);
            ui_socket(f, cx, cy, 5, -1, ramp, state, on && !G.tot_col && k == imin(rk, ABIL_TOP - 1), 0, G.ui_t);
        }
        char sc[16];
        if (beaten) { snprintf(sc, sizeof sc, "%d SCRIP", RUN.scrip[b]); ui_text8(f, 186, y + 3, sc, FACE_BODY, R_BONE + 11, TX_INLAID); }
        else ui_text8(f, 186, y + 3, "NOT FALLEN", FACE_BODY, UI_M(7), TX_PLAIN);
        if (beaten && totem_extra(b) > 0) {                         /* the sale, on a tag of its own */
            int tx = 268, ty = y + 1, lit = on && G.tot_col;
            for (int yy = 0; yy < 11; yy++) for (int xx = 0; xx < 36; xx++)
                gfx_px(f, tx + xx, ty + yy, yy == 0 || xx == 0 ? UI_M(12) : yy == 10 || xx == 35 ? UI_M(2) : UI_M(lit ? 10 : 7));
            ui_text8(f, tx + 3, ty + 2, "SELL", FACE_BODY, G.tot_arm == b ? R_BLOOD + 13 : lit ? C_WHITE : R_GOLD + 9, TX_INLAID);
        }
    }
    if (!n) {
        UiPane pn = { -1, 0, R_GOLD, SK_REACH, 0, "NOTHING TO TEACH YET", 0, 0, 0,
                      "IT WANTS A BOSS'S ODIOUS SCRIP, AND NONE HAS FALLEN TO YOU YET. BEAT ONE ON THE WAY DOWN AND IT WILL TEACH YOU WHAT THAT ONE KNEW.",
                      0, 0, 0, 0, 0 };
        ui_pane_wide(f, 9, 94, 302, 86, &pn);
        const char *k = pk("ESC CLOSES");
        ui_keys(f, 160 - ui_keys_width(k) / 2, 183, k, UI_M(13));
        return;
    }
    int b = bs, rk = RUN.abil[b], top = rk >= ABIL_TOP, need = rk + 1, extra_n = totem_extra(b);
    if (G.tot_col && extra_n > 0) {
        snprintf(cost, sizeof cost, "SELL ONE, $%d", SCRIP_PRICE);
        snprintf(does, sizeof does, "YOU HOLD %d MORE OF ITS SCRIP THAN THE TOTEM WILL EVER ASK. EACH SELLS FOR $%d.", extra_n, SCRIP_PRICE);
    } else {
        if (top) snprintf(cost, sizeof cost, "HIGHEST"); else snprintf(cost, sizeof cost, rk ? "RAISE: %d SCRIP" : "UNLOCK: %d SCRIP", need);
        char d[160];
        int shown = rk < ABIL_TOP ? rk + 1 : rk;
        ab_text(b, shown, d, sizeof d);
        snprintf(does, sizeof does, "%s %s: %s", rk < ABIL_TOP ? (rk ? "NEXT," : "TO LEARN,") : "AT", RANK[shown], d);
    }
    int ok = G.tot_col ? 1 : (!top && RUN.scrip[b] >= need);
    if (G.tot_arm == b) snprintf(warn, sizeof warn, "ENTER AGAIN SELLS ONE FOR $%d.", SCRIP_PRICE);
    UiPane pn = { -1, BA_MARK[b], BA_RAMP[b], top ? SK_HELD : rk ? SK_PART : ok ? SK_READY : SK_SHORT, 0, BA_NAME[b], RANK[rk], cost,
                  ok ? R_GOLD + 12 : UI_M(9), does, 0, 0, G.sh_note_t > 0.0f ? G.sh_note : BOSSES[b].scrip, G.tot_arm == b ? warn : 0, 0 };
    ui_pane_wide(f, 9, 94, 302, 86, &pn);
    const char *k = pk(extra_n > 0 ? "W/S CHOOSE  A/D SELL  ENTER TAKES" : "W/S CHOOSE  ENTER TAKES");
    ui_keys(f, 160 - ui_keys_width(k) / 2, 183, k, UI_M(13));
}

/* the Portal's screen: the runs it opens, the challenges it offers with each one's step, a note on the one chosen */
static void render_portalmenu(Frame *f) {
    dim_view(f);
    int px = 6, pw = W - 12;
    hall_frame(f, "THE ACCURSED PORTAL", R_GOLD + 12, 0, 0);
    PortRow r[16];
    int n = portal_rows(r), sel = clampi(G.ui_sel, 0, n - 1), y = 33, here = crawl_run(G.level);
    char line[200];
    int heads = n <= 9, li[32], nl = 0, at = 0;                    /* the headings, while there is room for them: a list of lines,
                                                                   * -1 - k a heading, else a row */
    for (int i = 0; i < n; i++) {
        int lv = r[i].run < 0 && r[i].boss < 0;
        if (heads && (i == 0 || (r[i].boss >= 0 && r[i - 1].boss < 0) || (lv && !(r[i - 1].run < 0 && r[i - 1].boss < 0)))) li[nl++] = lv ? -3 : r[i].boss < 0 ? -1 : -2;
        if (i == sel) at = nl;
        li[nl++] = i;
    }
    enum { SHOWN = 9 };                                            /* nine lines show; past them the list scrolls by the chosen row */
    int first = clampi(at - SHOWN / 2, 0, imax(0, nl - SHOWN));
    ui_well(f, 9, 28, 302, SHOWN * 10 + 6);                        /* the rows, and what the chosen one does under them */
    ui_well(f, 9, 28 + SHOWN * 10 + 9, 302, 181 - (28 + SHOWN * 10 + 9));
    ui_scroll(f, 307, 30, SHOWN * 10 + 2, first, SHOWN, nl);
    for (int q = first; q < nl && q < first + SHOWN; q++, y += 10) {
        if (li[q] < 0) { gfx_text8(f, px + 8, y, li[q] == -3 ? "THE LEVELS CLEARED" : li[q] == -1 ? "THE RUNS" : "THE CHALLENGES", R_GOLD + 10, 1); continue; }
        int i = li[q];
        const PortRow *p = &r[i];
        int lv = p->run < 0 && p->boss < 0;
        ui_row(f, 11, y - 1, 294, 10, i == sel);
        uint8_t c = i == sel ? (uint8_t)C_WHITE : (uint8_t)(R_STONE + 12);
        if (lv) {                                                  /* the level to replay, A/D through those cleared */
            int l = G.port_lvl >= 1 && (RUN.rec.cleared >> (G.port_lvl - 1) & 1) ? G.port_lvl : replay_step(1, -1);
            snprintf(line, sizeof line, "< LEVEL %d >", replay_level(l));
            gfx_text8(f, px + 12, y, line, c, 1);
            snprintf(line, sizeof line, "%d SOULS", replay_price(l));
            gfx_text8(f, px + pw - 8 - gfx_text_width(line, 1), y, line, spirit() >= (float)replay_price(l) ? (uint8_t)(R_GLOW + 12) : (uint8_t)(R_BLOOD + 13), 1);
        } else if (p->boss < 0) {
            snprintf(line, sizeof line, "RUN %s  %s", RUN_ROMAN[p->run % 8], SET_NAME[p->run % 4]);
            gfx_text8(f, px + 12, y, line, c, 1);
            snprintf(line, sizeof line, p->run == here ? "HERE  LEVEL %d" : "LEVEL %d", 1 + RUN_LEN * p->run);
            gfx_text8(f, px + pw - 8 - gfx_text_width(line, 1), y, line, p->run == here ? (uint8_t)(R_GOLD + 11) : (uint8_t)(R_STONE + 10), 1);
        } else {
            gfx_text8(f, px + 12, y, BOSSES[p->boss].short_name, c, 1);
            snprintf(line, sizeof line, "STEP %d", RUN.bstep[p->boss]);
            gfx_text8(f, px + pw - 8 - gfx_text_width(line, 1), y, line, ramp_at(R_BLOOD, 9 + RUN.bstep[p->boss]), 1);
        }
    }
    y = 28 + SHOWN * 10 + 14;
    const PortRow *p = &r[sel];
    if (p->run < 0 && p->boss < 0) {
        int l = G.port_lvl >= 1 && (RUN.rec.cleared >> (G.port_lvl - 1) & 1) ? G.port_lvl : replay_step(1, -1);
        char nm[40];
        level_name(replay_level(l), nm, sizeof nm);
        snprintf(line, sizeof line, "%s, AGAIN, FOR SOULS: HER ROOM BEFORE IT, THEN THE LEVEL, NEW. GOLD, SOULS AND EXPERIENCE AS EVER; "
                 "NO CURSED COINS. THEN BACK HERE.", nm);
    }
    else if (p->boss < 0) snprintf(line, sizeof line, "THROUGH, AS YOU ARE, TO THE MERCHANT'S ROOM BEFORE LEVEL %d. %s", 1 + RUN_LEN * p->run,
                              p->run < here ? "BACK UP IS NO LOSS." : (p->run > here ? "ON, PAST WHAT LIES BETWEEN." : "THE START OF THIS ONE."));
    else step_text(RUN.bstep[p->boss], line, sizeof line);
    int room = (178 - y) / 9, need = ui_wrap_count(line, 36);
    if (need > room) ui_overflow("portal words", line, need - room);
    text_wrap(f, px + 10, y, line, 36, R_STONE + 13, room);
    hall_keys(f, 184, "W/S  A/D A LEVEL  ENTER GOES  ESC");
}

