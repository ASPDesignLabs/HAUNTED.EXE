/* A part of scene_crawl.c (docs/PLAN-tidy.md): it is #included there in its place and never compiled on its own.
 * Tab's and her hall's frames; a piece's card, the full bag's question, the buying, Tab's pages (elements, the caster, documents, the wardrobe), the Black Shrine, Esc's question. */

static void dim_view(Frame *f) {                                   /* the crypt goes dim behind a menu */
    for (int y = 0; y < H; y++)
        for (int x = (y & 1); x < W; x += 2) f->pix[y * W + x] = 0;
}

/* a page of Tab's, or the bestiary's (docs/PLAN-ui.md section 6): the slab, its title, a figure at the right, and its keys
 * on caps under the title, where they were; its wells are the page's own */
static void tab_frame(Frame *f, const char *title, int title_col, const char *right, int right_col, const char *keys) {
    ui_slab(f, 4, 4, 312, 192, SLAB_LEDGER, ORN_NONE, G.ui_t);
    ui_text16(f, 12, 8, title, FACE_INSC, title_col, TX_INLAID, 1);
    if (right) ui_text8(f, 308 - 8 * (int)strlen(right), 12, right, FACE_INSC, right_col, TX_INLAID);
    if (keys) ui_keys(f, 15, 27, pk(keys), UI_M(13));
}

/* a screen of her hall's (docs/PLAN-ui.md section 6.2): the slab, its title and a figure at its right; the keys on caps along
 * the foot, or a passing word in their place */
static void hall_frame(Frame *f, const char *title, int title_col, const char *right, int right_col) {
    ui_slab(f, 4, 4, 312, 192, SLAB_LEDGER, ORN_NONE, G.ui_t);
    ui_text16(f, 12, 8, title, FACE_INSC, title_col, TX_INLAID, 1);
    if (right) ui_text8(f, 308 - 8 * (int)strlen(right), 12, right, FACE_INSC, right_col, TX_INLAID);
}
static void hall_keys(Frame *f, int y, const char *keys) { const char *k = pk(keys); ui_keys(f, 160 - ui_keys_width(k) / 2, y, k, UI_M(13)); }
static void hall_word(Frame *f, int y, const char *s, int col) { ui_text8(f, 160 - 4 * (int)strlen(s), y, s, FACE_BODY, col, TX_INLAID); }

static int stat_over(int st);
static void eerie_text(Frame *f, int x, int y, const char *s);
static void eerie_eye(Frame *f, int x, int y);

/* the caster's total of a stat, in its units: health as the whole of it */
static void total_text(const Gear *g, int st, char *buf, int n) {
    float t = gear_total(g, st);
    if (st == ST_HP) snprintf(buf, (size_t)n, "%d", gear_max_hp(g));
    else if (st == ST_ARMOR) snprintf(buf, (size_t)n, "%.0f", t);
    else if (st == ST_REGEN) snprintf(buf, (size_t)n, "%.2f/S", t);
    else if (st == ST_THRIFT && t >= 0.5f) snprintf(buf, (size_t)n, "-%.0f%%", t);
    else if (st == ST_LUCK) snprintf(buf, (size_t)n, "%.1f%%", t);
    else if (st == ST_VESSEL) snprintf(buf, (size_t)n, "+%.0f", t);
    else if (st == ST_SPRINT) snprintf(buf, (size_t)n, "%.1fS", SPRINT_BASE + t);   /* the whole of it */
    else snprintf(buf, (size_t)n, "%.0f%%", t);
}

/* a piece's card: its name in its quality's colour, its stats (each beside the caster's total, green
 * or red against when the wardrobe opened), its ability; the bottom it reached */
static int piece_card(Frame *f, int x, int y, int w, const Piece *p, const Gear *g, const Gear *g0) {
    if (p->kind == IK_NONE) { gfx_text8(f, x, y, "NOTHING", R_STONE + 8, 1); return y + 8; }
    char l1[48], l2[48], line[100];
    piece_name(p, l1, l2, sizeof l1);
    int col = qual_text(p->quality);
    snprintf(line, sizeof line, "%s%s%s", l1, *l1 ? " " : "", l2);
    y += text_wrap_ex(f, x, y, line, w / 8, col, 4, 8) * 8 + 2;
    if (p->kind == IK_BAG) {
        snprintf(line, sizeof line, "HOLDS %d", bag_capacity(p->quality));
        gfx_text8(f, x, y, line, R_STONE + 12, 1);
        return y + 8;
    }
    for (int i = 0; i < p->nstat; i++) {
        piece_stat_text(p->stat[i], p->val[i], line, sizeof line);
        gfx_text8(f, x, y, line, R_STONE + 12, 1);
        int sw = gfx_text_width(line, 1);
        if (g) {
            float v = gear_total(g, p->stat[i]), v0 = gear_total(g0, p->stat[i]);
            int c = v > v0 + 1e-3f ? R_MOSS + 13 : (v < v0 - 1e-3f ? R_BLOOD + 13 : R_STONE + 9);
            total_text(g, p->stat[i], line, sizeof line);
            if (sw + 6 + gfx_text_width(line, 1) <= w) gfx_text8(f, x + w - gfx_text_width(line, 1), y, line, c, 1);   /* if there is room */
        }
        y += 8;
    }
    if (p->ability) {
        piece_ability_text(p, line, sizeof line);
        y += 2 + text_wrap_ex(f, x, y + 2, line, w / 8, R_GOLD + 11, 3, 8) * 8;
    }
    return y;
}

/* the bag is full and a ware is bought for a slot already worn: which spare to let go */
static void render_letgo(Frame *f) {
    dim_view(f);
    const Gear *g = &RUN.gear;
    hall_frame(f, "THE BAG IS FULL", R_GOLD + 12, 0, 0);
    int i = G.shop_letgo;
    if (i >= 0) {
        const Piece *np = &G.offers[i].piece;
        int sl = piece_slot(g, np->kind);
        char line[48];
        snprintf(line, sizeof line, "TO MAKE ROOM FOR YOUR OLD %s", KIND_NAME[g->worn[sl].kind]);
        gfx_text8(f, 14, 29, line, R_STONE + 12, 1);
    }
    int n = g->bag_size, sel = clampi(G.ui_sel, 0, n - 1);
    ui_well(f, 9, 40, 108, 141);                                   /* the bag's places, and the chosen spare's card */
    ui_well(f, 120, 40, 191, 141);
    for (int r = 0; r < n; r++) {
        int y = 45 + r * 10;
        const Piece *p = &g->bag[r];
        ui_row(f, 11, y - 1, 104, 10, r == sel);
        gfx_text8(f, 14, y, p->kind ? KIND_NAME[p->kind] : "-", p->kind ? qual_text(p->quality) : R_STONE + 7, 1);
        if (p->anointed) gfx_glyph8(f, 14 + gfx_text_width(KIND_NAME[p->kind], 1), y, '*', R_GOLD + 12, -1, 1);
    }
    piece_card(f, 125, 45, 311 - 5 - 125, &g->bag[sel], NULL, NULL);
    hall_keys(f, 184, "W/S PICK  ENTER LET GO  ESC NOT NOW");
}

/* a ware's buying (after 1.0.0b21.2): the ware and its price, what is worn there now, each of the caster's numbers that would change
 * (now and after, green where it rises, red where it falls), the abilities gained and lost, and what becomes of the old piece */
static void buy_stats(const Piece *p, char *b, int n) {            /* a piece's stats as a line, its ability apart */
    b[0] = 0;
    if (p->kind == IK_BAG) { snprintf(b, (size_t)n, "HOLDS %d", bag_capacity(p->quality)); return; }
    for (int k = 0; k < p->nstat; k++) {
        char st[40];
        piece_stat_text(p->stat[k], p->val[k], st, sizeof st);
        snprintf(b + strlen(b), (size_t)n - strlen(b), "%s%s", k ? ", " : "", st);
    }
}
static int abil_count(const Gear *g, int ab) { int c = 0; for (int s = 0; s < NSLOT; s++) c += g->worn[s].kind != IK_NONE && g->worn[s].ability == ab; return c; }
static const char *stat_short(int st);
static void render_buy(Frame *f) {
    dim_view(f);
    int i = G.shop_buy, more = buy_live(BUY_MAP);
    for (int j = 0; j < G.ped_n; j++) more += buy_live(j);
    char line[400], keys[96], right[32];
    snprintf(right, sizeof right, "YOU HAVE $%d", RUN.treasure);
    hall_frame(f, "HER WARES", R_GOLD + 12, right, R_GOLD + 11);
    if (i == BUY_MAP) {                                            /* the map, at her left */
        ui_well(f, 9, 26, 302, 153);
        gfx_text8(f, 14, 31, "A MAP OF THE CRYPT", R_BONE + 13, 1);
        snprintf(line, sizeof line, "$%d", MAP_PRICE);
        gfx_text8(f, 306 - gfx_text_width(line, 1), 31, line, MAP_PRICE <= RUN.treasure ? (uint8_t)(R_GOLD + 12) : (uint8_t)(R_BLOOD + 12), 1);
        snprintf(line, sizeof line, "A SCROLL THAT FILLS IN AS YOU WALK: THE HALLS AND ROOMS YOU HAVE SEEN, WHERE YOU HAVE WALKED, THE "
                 "DOORS, THE KEYS AND THE WAY OUT, AND THE LEVEL'S NUMBERS ACROSS ITS FOOT. %s OPENS IT, AND THE CRYPT RUNS ON UNDER "
                 "IT. IT OUTLASTS DEATH AND HER CLAIM.", PADX.used ? "D-PAD UP" : "M");
        text_wrap_ex(f, 14, 45, line, 36, R_STONE + 13, 8, 9);
        if (G.sh_note_t > 0.0f) gfx_text8(f, 14, 166, G.sh_note, R_GOLD + 11, 1);
        else if (MAP_PRICE > RUN.treasure) { snprintf(line, sizeof line, "YOU ARE $%d SHORT", MAP_PRICE - RUN.treasure); gfx_text8(f, 14, 166, line, R_BLOOD + 13, 1); }
        snprintf(keys, sizeof keys, "E BUY  %sESC CLOSE", more > 1 ? "A/D WARES  " : "");
        hall_keys(f, 184, keys);
        return;
    }
    const Offer *o = &G.offers[i];
    const Piece *p = &o->piece;
    int piece = o->kind == GK_PIECE, bag = piece && p->kind == IK_BAG, sl = piece && !bag ? piece_slot(&RUN.gear, p->kind) : -1;
    const Piece *old = sl >= 0 && RUN.gear.worn[sl].kind != IK_NONE ? &RUN.gear.worn[sl] : NULL;
    char name[100], stats[200], ab[80] = "", oname[100] = "", ostats[200] = "", price[16];
    if (piece) { piece_full_name(p, name, sizeof name); buy_stats(p, stats, sizeof stats); if (p->ability) piece_ability_text(p, ab, sizeof ab); }
    else { gear_name(o, name, sizeof name); snprintf(stats, sizeof stats, "%s", gear_blurb(o)); }
    if (old) { piece_full_name(old, oname, sizeof oname); buy_stats(old, ostats, sizeof ostats); }
    if (bag) {                                                     /* a bag: the one carried now, if any */
        if (RUN.gear.bag_size) snprintf(oname, sizeof oname, "%s %s", QUAL_NAME[RUN.gear.bag_quality], bag_name(RUN.gear.bag_size));
        snprintf(ostats, sizeof ostats, RUN.gear.bag_size ? "HOLDS %d" : "", RUN.gear.bag_size);
    }
    if (o->price) snprintf(price, sizeof price, "$%d", o->price); else snprintf(price, sizeof price, "FREE");
    int pw = gfx_text_width(price, 1), ncols = (292 - pw - 8) / 8;
    /* what changes: each number the caster has, before and after, and the abilities */
    Gear after = RUN.gear;
    if (piece) { if (bag) gear_take(&after, o); else gear_take_wear(&after, o); }
    struct { int st; const char *what; int good; } ch[NSTAT + NABIL + 1];
    int nch = 0;
    if (piece && !bag) {                                           /* the abilities first, then the numbers in the caster's page's order */
        static const int FIRST[] = { ST_ARMOR, ST_DODGE, ST_HP, ST_REGEN, ST_PHYSIC, ST_RES_POISON, ST_RES_BURN, ST_RES_SLOW, ST_RES_SPIRIT,
                                     ST_RES_LIGHT, ST_MOVE, ST_SPRINT, ST_QUIET, ST_HASTE, ST_THRIFT, ST_DAMAGE, ST_FIRE, ST_FROST, ST_SHOCK,
                                     ST_EFFECT, ST_TREASURE, ST_LUCK, ST_VESSEL };
        enum { NFIRST = sizeof FIRST / sizeof FIRST[0] };
        for (int a = 1; a < NABIL; a++) {
            int c0 = abil_count(&RUN.gear, a), c1 = abil_count(&after, a);
            if (c0 != c1) { ch[nch].st = -1 - a; ch[nch].what = ABIL_NAME[a]; ch[nch++].good = c1 > c0; }
        }
        for (int q = 0; q < NSTAT; q++) {
            int st = q < NFIRST ? FIRST[q] : -1;
            if (q >= NFIRST) {                                     /* the rest, in their own order */
                int k = 0, n = q - NFIRST;
                for (st = 0; st < NSTAT; st++) {
                    int listed = 0;
                    for (int j = 0; j < NFIRST; j++) listed |= FIRST[j] == st;
                    if (!listed && k++ == n) break;
                }
                if (st >= NSTAT) break;
            }
            float a = st == ST_HP ? (float)gear_max_hp(&RUN.gear) : gear_total(&RUN.gear, st), b = st == ST_HP ? (float)gear_max_hp(&after) : gear_total(&after, st);
            if (fabsf(b - a) < (st == ST_REGEN ? 0.005f : 0.05f)) continue;
            ch[nch].st = st; ch[nch].what = NULL; ch[nch++].good = b > a;
        }
    }
    if (bag && bag_capacity(p->quality) != RUN.gear.bag_size) { ch[nch].st = -1000; ch[nch].what = NULL; ch[nch++].good = bag_capacity(p->quality) > RUN.gear.bag_size; }
    /* the ware's lines, the old's, and the rows: what fits */
    int ln = text_wrap_ex(NULL, 0, 0, name, ncols, 0, 2, 9), ls = text_wrap_ex(NULL, 0, 0, stats, 36, 0, 3, 9);
    const char *lab = bag ? "YOUR BAG: " : "WORN NOW: ";
    int lw = gfx_text_width(lab, 1), ocols = (292 - lw) / 8;
    int la = ab[0] ? text_wrap_ex(NULL, 0, 0, ab, 36, 0, 2, 9) : 0, los = ostats[0] ? text_wrap_ex(NULL, 0, 0, ostats, 36, 0, 2, 9) : 0;
    int lo = oname[0] ? text_wrap_ex(NULL, 0, 0, oname, ocols, 0, 2, 9) : 1;
    if (!piece) lo = los = 0;
    int rows_fit, ha, hb;
    for (;;) {                                                     /* the old's stats give way first, then its name's second line */
        ha = 9 + 9 * (ln + ls + la) + (lo ? 5 + 9 * (lo + los) : 0);
        hb = 179 - (26 + ha + 3);
        rows_fit = (hb - 38) / 8;                                  /* from under the heading to over the two lines at its foot */
        if (rows_fit >= nch || (los == 0 && lo <= 1 && ls <= 2)) break;
        if (los > 0) los--; else if (lo > 1) lo--; else ls--;
    }
    ui_well(f, 9, 26, 302, ha);
    int y = 31;
    text_wrap_ex(f, 14, y, name, ncols, piece ? qual_text(p->quality) : ramp_at(offer_color(o), 13), ln, 9);
    gfx_text8(f, 306 - pw, y, price, o->price <= RUN.treasure ? (uint8_t)(R_GOLD + 12) : (uint8_t)(R_BLOOD + 12), 1);
    y += 9 * ln;
    text_wrap_ex(f, 14, y, stats, 36, R_STONE + 13, ls, 9); y += 9 * ls;
    if (la) { text_wrap_ex(f, 14, y, ab, 36, R_GOLD + 11, la, 9); y += 9 * la; }
    if (lo) {                                                      /* what it takes the place of */
        y += 3;
        ui_rule(f, 14, 305, y - 2, 0);
        y += 2;
        gfx_text8(f, 14, y, lab, R_STONE + 10, 1);
        if (oname[0]) {
            snprintf(line, sizeof line, "%s", oname);
            if (lo == 1 && (int)strlen(line) > ocols) { line[ocols - 2] = '.'; line[ocols - 1] = '.'; line[ocols] = 0; }   /* no room: cut short */
            text_wrap_ex(f, 14 + lw, y, line, ocols, old ? qual_text(old->quality) : (uint8_t)(R_STONE + 12), lo, 9);
        } else gfx_text8(f, 14 + lw, y, bag ? "NONE" : "NOTHING", R_STONE + 8, 1);
        y += 9 * lo;
        if (los) text_wrap_ex(f, 14, y, ostats, 36, R_STONE + 10, los, 9);
    }
    int yb = 26 + ha + 3;
    ui_well(f, 9, yb, 302, 179 - yb);
    y = yb + 4;
    if (piece) {
        gfx_text8(f, 14, y, "WHAT CHANGES", R_GOLD + 10, 1);
        gfx_text8(f, 236 - gfx_text_width("NOW", 1), y, "NOW", R_STONE + 10, 1);
        gfx_text8(f, 300 - gfx_text_width("AFTER", 1), y, "AFTER", R_STONE + 10, 1);
        y += 11;
        if (!nch) { gfx_text8(f, 14, y, "NOTHING THE CASTER HAS", R_STONE + 9, 1); y += 8; }
        for (int k = 0; k < nch && k < rows_fit; k++, y += 8) {
            int c = ch[k].good ? R_MOSS + 13 : R_BLOOD + 13;
            if (k == rows_fit - 1 && nch > rows_fit) { snprintf(line, sizeof line, "AND %d MORE", nch - k); gfx_text8(f, 14, y, line, R_STONE + 10, 1); break; }
            if (ch[k].st == -1000) {
                gfx_text8(f, 14, y, "THE BAG HOLDS", R_STONE + 12, 1);
                snprintf(line, sizeof line, "%d", RUN.gear.bag_size); gfx_text8(f, 236 - gfx_text_width(line, 1), y, line, R_STONE + 12, 1);
                snprintf(line, sizeof line, "%d", bag_capacity(p->quality)); gfx_text8(f, 300 - gfx_text_width(line, 1), y, line, c, 1);
            } else if (ch[k].st < 0) {
                snprintf(line, sizeof line, "%s %s", ch[k].good ? "GAINS" : "LOSES", ch[k].what);
                gfx_text8(f, 14, y, line, ch[k].good ? (uint8_t)(R_GOLD + 11) : (uint8_t)(R_BLOOD + 13), 1);
            } else {
                char a[24], b[24];
                total_text(&RUN.gear, ch[k].st, a, sizeof a); total_text(&after, ch[k].st, b, sizeof b);
                gfx_text8(f, 14, y, stat_short(ch[k].st), R_STONE + 12, 1);
                gfx_text8(f, 236 - gfx_text_width(a, 1), y, a, R_STONE + 12, 1);
                gfx_text8(f, 300 - gfx_text_width(b, 1), y, b, c, 1);
            }
        }
    } else text_wrap_ex(f, 14, y, "A SPELL OF ITS OWN: NOTHING WORN CHANGES", 36, R_STONE + 11, 2, 9);
    /* what becomes of the old piece, and how the visit stands */
    char f1[96] = "", f2[96] = "";
    int c1 = R_STONE + 11, c2 = R_GOLD + 11, worn = buy_worn(i);
    if (piece && !bag) {
        if (worn < 0) snprintf(f1, sizeof f1, "NOTHING IS WORN THERE YET");
        else if (!RUN.gear.bag_size) {
            int pl = old->kind == IK_BOOTS || old->kind == IK_ROBE;   /* BOOTS and ROBES are plural */
            snprintf(f1, sizeof f1, G.shop_confirm == i ? (pl ? "E AGAIN: YOUR OLD %s ARE LOST" : "E AGAIN: YOUR OLD %s IS LOST") : "NO BAG: YOUR OLD %s WILL BE LOST", KIND_NAME[old->kind]);
            c1 = R_BLOOD + 13;
        }
        else if (!gear_bag_free(&RUN.gear)) { snprintf(f1, sizeof f1, "BAG FULL: A SPARE MUST GO FOR IT"); c1 = R_GOLD + 10; }
        else snprintf(f1, sizeof f1, old->kind == IK_BOOTS || old->kind == IK_ROBE ? "YOUR OLD %s GO INTO YOUR BAG" : "YOUR OLD %s GOES INTO YOUR BAG", KIND_NAME[old->kind]);
    } else if (bag) snprintf(f1, sizeof f1, RUN.gear.bag_size ? "YOUR SPARES GO INTO IT" : "SPARES ARE KEPT IN IT, NOT LOST");
    if (G.sh_note_t > 0.0f) snprintf(f2, sizeof f2, "%s", G.sh_note);
    else if (G.shop_left <= 0) { snprintf(f2, sizeof f2, "NO MORE WARES THIS VISIT"); c2 = R_BLOOD + 13; }
    else if (o->price > RUN.treasure) { snprintf(f2, sizeof f2, "YOU ARE $%d SHORT", o->price - RUN.treasure); c2 = R_BLOOD + 13; }
    else if (G.shop_held == i) snprintf(f2, sizeof f2, "HELD FOR YOUR NEXT VISIT");
    else if (G.offer_back[i]) snprintf(f2, sizeof f2, "HELD FOR YOU LAST TIME");
    else snprintf(f2, sizeof f2, G.shop_left == 1 ? "ONE MORE WARE THIS VISIT" : "%d MORE WARES THIS VISIT", G.shop_left);
    hint8(f, 14, 179 - 4 - 18, f1, c1);                           /* (its E the controller's A, asked twice) */
    gfx_text8(f, 14, 179 - 4 - 9, f2, (uint8_t)c2, 1);
    int hold = G.shop_left > 0 && o->price > 0 && G.shop_held < 0;
    if (hold) {                                                    /* what holding it costs, at the right of that line */
        snprintf(line, sizeof line, "HOLD $%d", (o->price + 3) / 4);
        if (gfx_text_width(f2, 1) + 16 + gfx_text_width(line, 1) <= 292) gfx_text8(f, 306 - gfx_text_width(line, 1), 179 - 4 - 9, line, R_STONE + 11, 1);
    }
    keys[0] = 0;
    if (G.shop_left > 0) snprintf(keys, sizeof keys, "E BUY  ");
    if (hold) snprintf(keys + strlen(keys), sizeof keys - strlen(keys), "%s HOLD  ", PADX.used ? "SPACE" : "Q");
    if (more > 1) snprintf(keys + strlen(keys), sizeof keys - strlen(keys), "A/D WARES  ");
    snprintf(keys + strlen(keys), sizeof keys - strlen(keys), "ESC CLOSE");   /* (not BACK: a controller's X) */
    hall_keys(f, 184, keys);
}

/* Tab, the fifth page (1.0.0b11): each element's Power, Efficiency and Insight, the line's and what is worn together,
 * blue where what is worn is at its cap; and what each means */
static void render_elements(Frame *f) {
    const Gear *g = &RUN.gear;
    tab_frame(f, "THE ELEMENTS", R_GOLD + 12, 0, 0, "A/D PAGE  TAB CLOSE");
    ui_well(f, 9, 37, 302, 154);
    static const char *const HEAD[5] = { "POWER", "EFF.", "INS.", "ADDS", "ODDS" };   /* 1.0.0b21: what each adds, and its status */
    static const int HX[5] = { 146, 186, 226, 266, 306 };
    for (int c = 0; c < 5; c++) gfx_text8(f, HX[c] - gfx_text_width(HEAD[c], 1), 42, HEAD[c], R_GOLD + 10, 1);
    char v[24];
    for (int e = 0; e < NELEM; e++) {
        int y = 54 + e * 12, r = align_rank(e), worn[5] = { ST_POW + e, ST_EFF + e, ST_INS + e, st_adds(e), e < EL_PHYS ? ST_CHANCE + e : -1 };
        float val[5] = { (spell_line(e, 2) + gear_total(g, ST_POW + e) / 100.0f) * 100.0f, spell_efficiency(g, e) * 100.0f, spell_insight(g, e) * 100.0f,
                         gear_total(g, st_adds(e)) + (e != EL_POISON ? align_add(e) * 100.0f : 0.0f),
                         (st_odds(e) + (e == EL_FIRE || e == EL_LIGHT || e == EL_SPIRIT ? cls_ab(e, 0) / 100.0f : 0.0f)) * 100.0f };
        elem_glyph(f, 14, y, e, r == 0);
        gfx_text8(f, 26, y, ELEM_NAME[e], r ? ramp_at(ELEM_RAMP[e], 13) : (uint8_t)(R_STONE + 8), 1);
        for (int c = 0; c < 5; c++) {
            if (worn[c] < 0) { gfx_text8(f, HX[c] - gfx_text_width("--", 1), y, "--", R_STONE + 7, 1); continue; }   /* Physical: no status */
            snprintf(v, sizeof v, c == 1 || c == 2 ? "%.0f%%" : "+%.0f%%", val[c]);
            int capped = gear_capped(g, worn[c]), some = gear_total(g, worn[c]) > 0.0f || (c == 4 && val[c] > 0.5f);
            gfx_text8(f, HX[c] - gfx_text_width(v, 1), y, v, capped ? (uint8_t)(R_GLOW + 12) : some ? (uint8_t)(R_GOLD + 11) : val[c] > 0.5f ? (uint8_t)(R_STONE + 12) : (uint8_t)(R_STONE + 7), 1);
        }
    }
    ui_rule(f, 14, 305, 129, 0);
    text_wrap(f, 14, 134, "POWER: MORE OF ITS DAMAGE. EFF.: LESS SPIRIT, OR NO BONE SPENT. INS.: A SHORTER WIND-UP. "
                          "ADDS: SO MUCH MORE OF EVERY SPELL, AS IT. ODDS: ITS MARK ON THE DEAD MORE OFTEN. GOLD: WORN. "
                          "BLUE: AT ITS CAP.", 36, R_STONE + 11, 6);
}

/* a stat's name, short enough for a column of the caster's numbers */
static const char *stat_short(int st) {
    if (st == ST_PHYSIC) return "PHYSIC";
    if (st == ST_SPRINT) return "SPRINT";
    static const char *const SHORT[ST_POW] = { "ARMOR", "DODGE", "HEALTH", "RESTORES", "POISON WARD", "BURN WARD", "SLOW WARD",
                                              "MOVEMENT", "QUIET", "SPELL HASTE", "SPIRIT COST", "DAMAGE", "FIRE", "FROST",
                                              "LIGHTNING", "EFFECT", "TREASURE", "LUCK", "SPIRIT WARD", "VESSEL" };
    return st == ST_RES_LIGHT ? "LIGHTNING WARD" : st >= 0 && st < ST_POW ? SHORT[st] : STAT_NAME[clampi(st, 0, NSTAT - 1)];
}
/* Tab, the second page: the caster's numbers (what abilities add in gold beside them) and the spellbook */
static void render_caster(Frame *f) {
    static const int ROWS[] = { ST_ARMOR, ST_DODGE, ST_HP, ST_REGEN, ST_PHYSIC, ST_RES_POISON, ST_RES_BURN, ST_RES_SLOW, ST_RES_SPIRIT,
                                ST_RES_LIGHT, ST_MOVE, ST_SPRINT, ST_QUIET, ST_HASTE, ST_THRIFT, ST_DAMAGE, ST_FIRE, ST_FROST, ST_SHOCK,
                                ST_EFFECT, ST_TREASURE, ST_LUCK, ST_VESSEL };   /* twelve down the left, eleven down the right */
    enum { NROWS = sizeof ROWS / sizeof ROWS[0], LEFT = 12 };
    const Gear *g = &RUN.gear, *g0 = &G.inv0;
    tab_frame(f, "THE CASTER", R_GOLD + 12, 0, 0, "A/D PAGE  TAB CLOSE");
    ui_well(f, 9, 37, 302, 154);
    {
        char at[24];
        snprintf(at, sizeof at, "ATONEMENT %d", RUN.atone);
        int ax = 12 + gfx_text_width("THE CASTER", 1) + 10;
        gfx_text8(f, ax, 12, at, R_PURPLE + 13, 1);
        if (cls_el() >= 0) class_mark(f, ax + gfx_text_width(at, 1) + 8, 10, cls_el(), 0);   /* what the caster was */
    }
    {                                                              /* the Cursed Coins, for the shrine */
        char cc[24];
        snprintf(cc, sizeof cc, "%d COIN%s", RUN.coins, RUN.coins == 1 ? "" : "S");
        int cw = gfx_text_width(cc, 1);
        draw_coin(f, 308 - cw - 10, 12);
        gfx_text8(f, 308 - cw, 12, cc, RUN.coins ? (uint8_t)(R_GOLD + 10) : (uint8_t)(R_STONE + 8), 1);
    }
    char v[24], extra[16];
    int capped = 0;
    for (int i = 0; i < NROWS; i++) {                              /* the elements' own are on a page of their own */
        int st = ROWS[i], x = i < LEFT ? 14 : 162, y = 39 + (i % LEFT) * 8, w = i < LEFT ? 140 : 144;
        float t = gear_total(g, st), t0 = gear_total(g0, st), add = 0.0f;
        if (st == ST_MOVE || st == ST_THRIFT) add = fminf(gear_total_raw(g, st) * (gear_ability(g, st == ST_MOVE ? AB_SWIFT : AB_ARCANE, 0) / 100.0f), t);   /* Swiftness, Arcane Knowledge */
        total_text(g, st, v, sizeof v);
        int c = t > t0 + 1e-3f ? R_MOSS + 13 : (t < t0 - 1e-3f ? R_BLOOD + 13 : (t > 0.0f ? R_STONE + 13 : R_STONE + 7));
        if (gear_capped(g, st)) { c = t < t0 - 1e-3f ? c : R_GLOW + 12; capped = 1; }   /* at its cap */
        gfx_text8(f, x, y, stat_short(st), t > 0.0f ? (uint8_t)(R_STONE + 11) : (uint8_t)(R_STONE + 7), 1);
        int vw = gfx_text_width(v, 1);
        if (stat_over(st) > 0) { eerie_text(f, x + w - vw, y, v); eerie_eye(f, x + w - vw - 11, y + 1); continue; }   /* overlevelled */
        gfx_text8(f, x + w - vw, y, v, c, 1);
        if (add >= 0.5f) { snprintf(extra, sizeof extra, "+%.0f", add); gfx_text8(f, x + w - vw - 4 - gfx_text_width(extra, 1), y, extra, R_GOLD + 11, 1); }
    }
    if (capped) gfx_text8(f, 308 - gfx_text_width("BLUE: AT ITS CAP", 1), 27, "BLUE: AT ITS CAP", R_GLOW + 12, 1);
    /* the abilities, summed and capped */
    int n = 0, y = 136;
    for (int ab = 1; ab < NABIL; ab++) {
        float a = gear_ability(g, ab, 0), b = gear_ability(g, ab, 1);
        if (a <= 0.0f) continue;
        switch (ab) {
        case AB_SWIFT: snprintf(v, sizeof v, "SWIFT +%.0f%%", a); break;
        case AB_ARCANE: snprintf(v, sizeof v, "ARCANE +%.0f%%", a); break;
        case AB_TREASURE: snprintf(v, sizeof v, "TREASURE +%.0f%%", a); break;
        case AB_KEYMAKER: snprintf(v, sizeof v, "KEYMAKER %.0f CELLS", a); break;
        case AB_COMMUNE: snprintf(v, sizeof v, "COMMUNE %.0f, %.1f S", a, b / 10.0f); break;
        case AB_THORNS: snprintf(v, sizeof v, "THORNS %.0f", a); break;
        case AB_MIRRORED: snprintf(v, sizeof v, "MIRROR %.0f%% AT %.0f%%", a, b); break;
        default: abil_short(ab, a, v, sizeof v); break;            /* the classes' (D6) */
        }
        gfx_text8(f, n % 2 ? 162 : 14, y + (n / 2) * 9, v, R_GOLD + 11, 1);
        n++;
    }
    if (!n) gfx_text8(f, 14, y, "NO ABILITIES WORN", R_STONE + 8, 1);
    /* the spellbook: number keys for the attack spells, the runes as they stand */
    y += ((n + 1) / 2 > 1 ? (n + 1) / 2 : 1) * 9 + 4;
    for (int i = 10; i < W - 10; i++) gfx_px(f, i, y, R_STONE + 5);
    y += 5;
    int x = 14;
    gfx_text8(f, x, y, "SPELLS", R_GOLD + 10, 1);
    x = 66;
    for (int k = 0; k < 7; k++) {                                  /* by their keys: each line open, at its rank */
        int sp = KEY_SPELL[k];
        if (g->atk[sp] < 0) continue;
        char e[24];
        if (sp == AT_BFS) snprintf(e, sizeof e, "%d BFS", k + 1); else snprintf(e, sizeof e, "%d   %d", k + 1, g->atk[sp]);
        int w = gfx_text_width(e, 1);
        if (x + w > W - 10) { x = 66; y += 9; }
        gfx_text8(f, x, y, e, sp == G.spell && !G.rst_sel ? (uint8_t)(R_GOLD + 12) : (uint8_t)(R_STONE + 11), 1);
        if (sp != AT_BFS) elem_glyph(f, x + 12, y, ATK_ELEM[sp], 0);
        x += w + 8;
    }
    if (restore_open()) {                                          /* Restoration on 8, once confessed (D5's spell), an ankh for its glyph */
        static const char *const ANKH[7] = { "..###..", ".#...#.", "..#.#..", "#######", "...#...", "...#...", "...#..." };
        char e[24];
        snprintf(e, sizeof e, "8   %d", imax(1, conf_rank(CF_RESTORE)));
        int w = gfx_text_width(e, 1);
        if (x + w > W - 10) { x = 66; y += 9; }
        gfx_text8(f, x, y, e, G.rst_sel ? (uint8_t)(R_GOLD + 12) : (uint8_t)(R_STONE + 11), 1);
        for (int r = 0; r < 7; r++) for (int c = 0; c < 7; c++) if (ANKH[r][c] == '#') gfx_px(f, x + 12 + c, y + r, ramp_at(R_GHOST, 13));
    }
    y += 10; x = 14;
    gfx_text8(f, x, y, "RUNES", R_GOLD + 10, 1);
    x = 66;
    int any = 0;
    for (int k = 0; k < NDEF; k++) {
        int q = g->def[k];
        if (q <= 0) continue;
        any = 1;
        int w = gfx_text_width(DEF_SHORT[k], 1), wt = gfx_text_width(Q_TAG[q - 1], 1);
        if (x + w + 5 + wt > W - 10) { x = 66; y += 9; }
        if (y > H - 17) { ui_overflow("caster runes", DEF_SHORT[k], 1); break; }   /* the well's foot */
        int ramp = QUAL_RAMP[q - 1], cool = G.def_cd[k] > 0.0f;   /* dimmer while it comes round again */
        gfx_text8(f, x, y, DEF_SHORT[k], (uint8_t)(ramp + (cool ? 7 : 12)), 1);
        gfx_text8(f, x + w + 5, y, Q_TAG[q - 1], (uint8_t)(ramp + (cool ? 5 : 9)), 1);
        x += w + 5 + wt + 9;
    }
    if (!any) gfx_text8(f, x, y, "NONE YET", R_STONE + 8, 1);
}

/* Tab's sixth page (1.0.0b13): what has been read and found on the level, the newest first; the message on its own page */
static int doc_wrap(const char *s, int cols0, int cols1, int nfirst, char out[][48], int max) {   /* lines, the first few narrower */
    int n = 0;
    while (*s && n < max) {
        int cols = n < nfirst ? cols0 : cols1, len = (int)strlen(s), cut = len;
        if (len > cols) { cut = cols; while (cut > 0 && s[cut] != ' ') cut--; if (!cut) cut = cols; }
        snprintf(out[n++], 48, "%.*s", cut, s);
        s += cut;
        while (*s == ' ') s++;
    }
    return n;
}
static void render_documents(Frame *f) {
    tab_frame(f, "DOCUMENTS", R_GOLD + 12, 0, 0, DOCS.n > 1 ? "W/S CHOOSE  A/D PAGE  TAB CLOSE" : "A/D PAGE  TAB CLOSE");
    if (!DOCS.n) {
        ui_well(f, 9, 37, 302, 154);
        text_wrap(f, 14, 46, "NOTHING READ OR FOUND ON THIS LEVEL. WHAT IS READ WITH E, AND WHAT IS FOUND, IS KEPT HERE UNTIL THE LEVEL ENDS.", 36, R_STONE + 10, 6);
        return;
    }
    int sel = clampi(DOCS.sel, 0, DOCS.n - 1);
    for (int r = 0; r < DOCS.n && r < 15; r++) {                   /* the newest first */
        int k = DOCS.n - 1 - r, y = 40 + r * 10;
        char nm[16];
        if (DOCS.d[k].msg) snprintf(nm, sizeof nm, "%s", DOC_SHORT[DOCS.d[k].run & 3]);
        else { snprintf(nm, sizeof nm, "%.12s", DOCS.d[k].where); char *c = strchr(nm, ','); if (c) *c = 0; }
        if (r == 0) ui_well(f, 9, 37, 99, 154);                     /* the list's well, the page beside it on the slab */
        if (r == sel) for (int yy = y - 1; yy < y + 9; yy++) for (int x = 11; x < 106; x++) if ((x + yy) & 1) gfx_px(f, x, yy, UI_W(6));
        gfx_text8(f, 14, y, nm, DOCS.d[k].msg ? (uint8_t)(R_GOLD + 11) : r == sel ? (uint8_t)C_WHITE : (uint8_t)(R_STONE + 11), 1);
    }
    const int k = DOCS.n - 1 - sel, px = 112, py = 36, pw = 198, ph = 154;
    char ln[16][48];
    if (!DOCS.d[k].msg) {                                          /* a clue: where it was read, and what it said */
        ui_well(f, 110, 37, 201, 154);
        int n = doc_wrap(DOCS.d[k].where, 23, 23, 0, ln, 3), y = py + 4;
        for (int i = 0; i < n; i++, y += 9) gfx_text8(f, px + 4, y, ln[i], R_GOLD + 10, 1);
        y += 5;
        n = doc_wrap(DOCS.d[k].text, 23, 23, 0, ln, 13);
        for (int i = 0; i < n; i++, y += 9) gfx_text8(f, px + 4, y, ln[i], R_STONE + 13, 1);
        return;
    }
    int run = DOCS.d[k].run & 3;
    unsigned seed = (unsigned)(G.level * 977 + run * 31 + 5);
    UI_PLAIN_BEGIN;                                                /* the page in its own ink */
    uint8_t ink = crawl_doc_page(f->pix, W, px, py, pw, ph, run, seed);
    const char *s = DOCS.d[k].text;
    int y = py + 12, x = px + 12, first = 0;
    if (run == 0 && *s) {                                          /* the first letter rubricated, in the gilt square */
        char c[2] = { *s, 0 };
        gfx_text16(f, px + 14, py + 9, c, R_BLOOD + 11, 1);
        s++; first = 2;
    }
    int n = doc_wrap(s, (pw - 40) / 8, (pw - 22) / 8, first, ln, 15);
    for (int i = 0; i < n && y < py + ph - 12; i++, y += 9) gfx_text8(f, i < first ? x + 16 : x, y, ln[i], ink, 1);
    crawl_doc_ink(f->pix, W, px, py, pw, ph, run, ink, seed);
    UI_PLAIN_END;
}

/* the Black Shrine's one row */
static void render_bshrine(Frame *f) {
    dim_view(f);
    int pw = 296, ph = 104, px = (W - pw) / 2, py = 30;
    ui_slab(f, px, py, pw, ph, SLAB_TABLET, ORN_NONE, G.ui_t);       /* a tablet: its one gift in a well, what it means under it */
    gfx_text16(f, px + 10, py + 6, "THE BLACK SHRINE", R_GOLD + 12, 1);
    char sp[24], line[48];
    spirit_text(sp, sizeof sp);
    snprintf(line, sizeof line, "SPIRIT %s", sp);
    gfx_text8(f, px + pw - 10 - gfx_text_width(line, 1), py + 10, line, R_GLOW + 12, 1);
    int done = G.msg_found || G.msg_deco < 0, price = bsh_price(), pay = spirit() + 1e-4f >= (float)price, y = py + 29;
    ui_well(f, px + 6, py + 24, pw - 12, 18);
    ui_well(f, px + 6, py + 45, pw - 12, 40);
    ui_row(f, px + 8, y - 3, pw - 16, 13, 1);
    for (int e = 0; e < 2; e++) { gfx_px(f, px + 14 + e * 5, y + 3, done ? (uint8_t)(R_PURPLE + 6) : (uint8_t)(R_PURPLE + 13)); gfx_px(f, px + 14 + e * 5, y + 2, R_PURPLE + 8); }
    gfx_text8(f, px + 28, y, "SHOW WHERE IT IS HIDDEN", done ? (uint8_t)(R_STONE + 8) : (uint8_t)C_WHITE, 1);
    snprintf(line, sizeof line, "%d SOULS", price);
    if (!done) gfx_text8(f, px + pw - 12 - gfx_text_width(line, 1), y, line, pay ? (uint8_t)(R_GOLD + 11) : (uint8_t)(R_BLOOD + 13), 1);
    const char *note = done ? "IT HAS NOTHING MORE TO SHOW" : !pay ? "IT WANTS MORE SOULS THAN YOU HAVE"
                     : G.bsh_arm ? "ENTER AGAIN TO PAY" : G.bsh_show > 0.0f ? "IT IS SHOWING NOW. PAYING AGAIN STARTS IT OVER"
                     : "FOR TWO AND A HALF MINUTES: THROUGH THE WALLS, AND ON THE MAP";
    text_wrap(f, px + 12, py + 50, note, 34, G.bsh_arm ? R_GOLD + 12 : R_STONE + 12, 2);
    if (G.bsh_show > 0.0f && !done) {
        snprintf(line, sizeof line, "%d:%02d LEFT", (int)G.bsh_show / 60, (int)G.bsh_show % 60);
        gfx_text8(f, px + pw - 12 - gfx_text_width(line, 1), py + 72, line, R_PURPLE + 12, 1);
    }
    hall_keys(f, py + ph - 14, "ENTER PAYS  ESC CLOSES");
}

/* Tab: what you wear and the bag (Enter moves between them) and the selected piece; A/D turns the page */
static void render_stones(Frame *f, int tabbed);
static void render_inventory(Frame *f) {
    dim_view(f);
    if (G.inv_page == 2) { render_stones(f, 1); return; }
    if (G.inv_page == 3) { render_align(f, 1); return; }
    if (G.inv_page == 4) { render_elements(f); return; }
    if (G.inv_page == 5) { render_documents(f); return; }
    if (G.inv_page) { render_caster(f); return; }
    /* the wardrobe (section 6.1): the seven worn as sockets on a carved figure, the bag's places under it, the piece in the pane */
    static const int KIND_ICON[NIK] = { -1, IC_BRACER, IC_BOOTS, IC_AMULET, IC_HOOD, IC_ROBES, IC_RING, IC_BAG };
    static const int SLOT_KIND[NSLOT] = { IK_WRIST, IK_WRIST, IK_BOOTS, IK_NECK, IK_HOOD, IK_ROBE, IK_RING };
    static const int SX[NSLOT] = { 32, 108, 70, 70, 70, 70, 124 }, SY[NSLOT] = { 104, 104, 152, 76, 52, 104, 130 };
    const Gear *g = &RUN.gear;
    int n = inv_rows(), sel = G.ui_sel < n ? G.ui_sel : n - 1;
    char line[48];
    if (g->bag_size) snprintf(line, sizeof line, "BAG %d/%d", g->bag_size - gear_bag_free(g), g->bag_size); else snprintf(line, sizeof line, "NO BAG");
    tab_frame(f, "WARDROBE", R_GOLD + 12, line, R_GOLD + 10, "W/S PICK  ENTER MOVE  A/D PAGE");
    ui_well(f, 9, 37, 150, 154);
    ui_well(f, 163, 37, 148, 154);
    bevel(f, 118, 40, 38, 46, 0);                                  /* the hood frames the face */
    draw_face(f, 121, 43, 0);
    static const int UP[NSLOT] = { SL_ROBE, SL_ROBE, SL_ROBE, SL_HOOD, -1, SL_NECK, SL_WRIST_R };   /* the figure's grooves */
    for (int r = 0; r < NSLOT; r++) {
        int u = UP[r];
        if (u < 0) continue;
        int dx = SX[r] - SX[u], dy = SY[r] - SY[u];
        float len = sqrtf((float)(dx * dx + dy * dy));
        int ox = (int)(dx / len * 11.0f), oy = (int)(dy / len * 11.0f);
        ui_groove(f, SX[u] + ox, SY[u] + oy, SX[r] - ox, SY[r] - oy, R_BONE, g->worn[r].kind && g->worn[u].kind ? 1.0f : 0.0f);
    }
    for (int r = 0; r < n; r++) {
        const Piece *p = inv_piece(r);
        int worn = r < NSLOT, kind = p->kind ? p->kind : worn ? SLOT_KIND[r] : IK_NONE;
        int cx = worn ? SX[r] : 24 + (r - NSLOT) * 20, cy = worn ? SY[r] : 176, rad = worn ? 10 : 7;
        int ramp = p->kind ? QUAL_RAMP[p->quality] : R_BONE;
        ui_socket(f, cx, cy, rad, worn || p->kind ? KIND_ICON[kind] : -1, ramp, p->kind ? SK_HELD : SK_REACH, r == sel, p->kind && p->anointed, G.ui_t);
    }
    if (!g->bag_size) ui_text8(f, 16, 172, "NO BAG", FACE_BODY, UI_M(9), TX_PLAIN);
    /* the pane: the slot, what Enter does, the piece's card */
    const Piece *p = inv_piece(sel);
    int kind = p->kind ? p->kind : sel < NSLOT ? SLOT_KIND[sel] : IK_NONE;
    ui_socket(f, 186, 62, 19, kind ? KIND_ICON[kind] : -1, p->kind ? QUAL_RAMP[p->quality] : R_BONE, p->kind ? SK_HELD : SK_REACH, 0, p->kind && p->anointed, G.ui_t);
    snprintf(line, sizeof line, "%s", sel < NSLOT ? SLOT_NAME[sel] : "IN THE BAG");
    ui_wrap(f, 210, 46, line, 12, 9, 2, FACE_INSC, UI_M(13), TX_INLAID);
    static const char *const WHY[] = { "", "NOTHING THERE", "NO BAG TO HOLD IT", "THE BAG IS FULL" };
    int act = inv_action(sel);
    const char *what = act == INV_TO_BAG ? "ENTER: INTO THE BAG" : (act == INV_TO_WEAR ? "ENTER: WEAR IT" : WHY[-act]);
    ui_wrap(f, 210, 66, pkp(what), 12, 9, 2, FACE_BODY, act > 0 ? R_GOLD + 11 : UI_M(9), TX_INLAID);
    ui_rule(f, 168, 305, 87, 0);
    piece_card(f, 168, 92, 138, p, g, &G.inv0);
}

/* Esc: the crypt makes its case for staying */
static void render_quit(Frame *f) {
    static const char *const LINE[] = {
        "THE MERCHANT WILL MISS YOU. THE MERCHANT WILL NEVER SAY SO.",
        "LEAVING? THE DEAD HAVE BEEN SO PATIENT WITH YOU.",
        "YOU STILL HAVE %d GOLD. IT SPENDS NOTHING UP THERE.",
        "OUT THERE IT IS JUST EMAIL. DOWN HERE YOU ARE A WIZARD.",
        "THE SKELETONS WERE JUST STARTING TO LIKE YOU.",
        "THE TORCHES WILL GO OUT WITHOUT YOU. ALL OF THEM.",
        "THE CAT IS WATCHING. THE CAT IS ALWAYS WATCHING.",
        "ONE MORE LEVEL. EVERYONE SAYS ONE MORE LEVEL.",
        "THE WRAITHS WILL TELL EVERYONE YOU RAN.",
        "YOUR DESK IS THE REAL TOMB, YOU KNOW.",
        "SOMEWHERE ON LEVEL %d A KEY IS STILL WAITING FOR YOU.",
        "WHO WILL PET THE MERCHANT WHILE YOU ARE GONE?",
        "THE SPIDERS SAY STAY AS LONG AS YOU LIKE. THE SPIDERS INSIST.",
        "GO, THEN. THE CRYPT WILL KEEP YOUR PLACE. AND YOUR NAME.",
        "REAL WIZARDS DO NOT HAVE MEETINGS.",
        "%d SOULS. SO CLOSE TO SOMETHING WONDERFUL.",
    };
    enum { NLINE = (int)(sizeof LINE / sizeof LINE[0]) };
    dim_view(f);
    int bw = 280, bh = 180, bx = (W - bw) / 2, by = (H - bh) / 2 - 4;   /* a headstone: the question in its arch */
    ui_slab(f, bx, by, bw, bh, SLAB_HEAD, ORN_NONE, G.ui_t);
    gfx_text16(f, (W - gfx_text_width("LEAVE THE CRYPT?", 1)) / 2, by + 10, "LEAVE THE CRYPT?", R_BLOOD + 13, 1);
    char msg[96];
    int li = G.quit_line % NLINE, arg = li == 2 ? RUN.treasure : (li == 10 ? G.level : RUN.souls);
    snprintf(msg, sizeof msg, LINE[li], arg);
    int y = by + 32;                                               /* wrapped at word ends, 32 to a line */
    for (const char *p = msg; *p && y < by + 52; y += 10) {
        int n = (int)strlen(p);
        if (n > 32) { n = 32; while (n > 0 && p[n] != ' ') n--; if (n == 0) n = 32; }
        char ln[40]; memcpy(ln, p, (size_t)n); ln[n] = 0;
        gfx_text8(f, (W - gfx_text_width(ln, 1)) / 2, y, ln, R_STONE + 13, 1);
        p += n; while (*p == ' ') p++;
    }
    char tt[24], ts[24];
    snprintf(tt, sizeof tt, "TURNING  %d%%", PL.tturn * 10); snprintf(ts, sizeof ts, "SMOOTHING  %d", PL.tsmooth);
    const char *OPT[Q_OPTIONS + 1] = { "FLEE TO THE SURFACE", "STAY IN THE DARK", "SLEEPWALK", "OPEN THE LEDGER", "BEGIN A NEW RUN",
                                       quit_touch() ? tt : "THE CONTROLLER", ts };
    static const char *const NOTE[Q_OPTIONS + 1] = { "NOTHING IS KEPT BUT WHAT IS SIGNED", "BACK TO THE CRYPT", "THE AUTOPILOT WALKS ON WITHOUT YOU",
                                                     "A RUN FROM A PAGE SHE HAS SIGNED", "FROM NOTHING. SIGNED PAGES STAY",
                                                     "EACH BUTTON'S JOB, AND THE STICKS", "SIDEWAYS: STEADIER, OR QUICKER" };
    const char *const TNOTE = "SIDEWAYS: HOW FAR A SLIDE TURNS";
    int nq = quit_rows(), step = nq > Q_OPTIONS ? 13 : nq > Q_PAD ? 15 : 17, top = 62;   /* the rows in a well, spaced to fit */
    ui_well(f, bx + 10, by + 56, bw - 20, 104);
    ui_rule(f, bx + 22, bx + bw - 23, by + top + Q_OPEN * step, 1);   /* leaving, then the ledger and a new run */
    for (int i = 0; i < nq; i++) {
        int oy = by + top + i * step + (i >= Q_OPEN ? 4 : 0) - (i < Q_OPEN ? 1 : 0), sel = i == G.ui_sel;
        int w = gfx_text_width(OPT[i], 1), ox = (W - w) / 2;
        ui_row(f, bx + 12, oy - 1, bw - 24, 16, sel);
        gfx_text16(f, ox, oy, OPT[i], sel ? (uint8_t)C_WHITE : (uint8_t)(R_STONE + 9), 1);
        if (sel) {                                                 /* a candle flame for a cursor */
            int fl = (int)(G.t * 10.0) & 1;
            for (int k = 0; k < 2; k++) {
                int cx = k ? ox + w + 8 : ox - 12;
                for (int yy = 0; yy < 9; yy++)
                    for (int xx = -2; xx <= 2; xx++)
                        if (abs(xx) * 3 <= yy + fl) gfx_px(f, cx + xx, oy + 3 + yy, (uint8_t)(R_FLAME + 15 - yy));
            }
        }
    }
    const char *note = quit_touch() && G.ui_sel == Q_TTURN ? TNOTE : NOTE[clampi(G.ui_sel, 0, nq - 1)];
    int ncol = R_GOLD + 9;
    if (LEDGER.marked && (G.ui_sel == Q_FLEE || G.ui_sel == Q_OPEN || G.ui_sel == Q_NEW)) { note = "THE DEAD ARE WATCHING"; ncol = R_BLOOD + 13; }
    if (CON.used && G.ui_sel == Q_FLEE) note = "THE DEAD TOOK NOTE: NOTHING KEPT";
    if (G.ui_sel == Q_NEW && G.quit_arm) { note = "ENTER AGAIN: THE UNSIGNED IS LOST"; ncol = R_BLOOD + 13; }
    hint8(f, (W - hint_width(note)) / 2, by + bh - 15, note, ncol);   /* its own line, inside the stone */
}

