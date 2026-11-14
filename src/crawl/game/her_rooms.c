/* A part of scene_crawl.c (docs/PLAN-tidy.md): it is #included there in its place and never compiled on its own.
 * A person in her rooms: the portraits in play, shop_human, the full bag's question, the buying; taking over and letting go (human_begin, human_end); Tab's actions; Esc's question and a new run; the debug build's teleports. */

/* ---- the portraits of the slain in play (docs/PLAN-portraits.md, step 4): a tapestry drawn back, a portrait looked at and its card,
 * and the gold each rank unlocked pays once a life ---- */

/* the bestiary's pages in three, the spiderling with its mother: 7, 8 and 7 (0-2); -1 not a page */
static int por_third(int k) {
    int order[NKINDS], nk = por_kinds(order);
    for (int i = 0; i < nk; i++) if (order[i] == k) return i < 7 ? 0 : i < 15 ? 1 : 2;
    return -1;
}
static int por_slayable(int k) { for (int r = 0; r < NRANK; r++) if (slayer_need(k, r)) return 1; return 0; }
/* its strength: its worth in experience (KIND_XP) against the others of its third that can be slain, 0 the least to 1 the most, to the
 * hundredth (the Spectre 0, the Wraith 1) */
static double por_strength(int k) {
    if (k == MOB_GHOST) return 0.0;
    if (k == MOB_WRAITH) return 1.0;
    int t = por_third(k), lo = 1 << 30, hi = -1;
    for (int j = 0; j < NKINDS; j++) if (por_third(j) == t && por_slayable(j)) { lo = imin(lo, KIND_XP[j]); hi = imax(hi, KIND_XP[j]); }
    return hi > lo ? floor((double)(KIND_XP[k] - lo) / (double)(hi - lo) * 100.0 + 0.5) / 100.0 : 0.0;
}
/* the gold a rank unlocked pays, once a life (section 4, the owner's Sheet): its third's start (100, 200, 300), and 60% more of it for
 * each rank's step (I 0 to V 4, gilded 3.5) and for its strength, to the nearest ten; a kind of one rank pays as its third's rank V */
static int por_gold(int k, int r) {
    static const double START[3] = { 100.0, 200.0, 300.0 }, STEP[NRANK] = { 0.0, 1.0, 2.0, 3.0, 4.0, 3.5 };
    int t = por_third(k);
    double step;
    if (t < 0 || r < 0 || r > POR_FEAT || (k == MOB_WRAITH && r == RK_IV)) return 0;   /* the Wraith's fourth: its boss pays (Q14) */
    if (r == POR_FEAT) { if (feat_of(k) < 0) return 0; step = STEP[RK_III]; }   /* a feat pays as its rank III (5 B) */
    else step = STEP[KINDS[k].family < 0 && por_slayable(k) ? RK_V : r];   /* the shade's one rank as its third's V (Q9) */
    return (int)(START[t] * (1.0 + 0.6 * (step + por_strength(k))) / 10.0 + 0.5 + 1e-9) * 10;
}
static int por_drawable(int k) {                                   /* one of its kind slain (or laid, or driven): its tapestry may come down */
    if (k == MOB_GHOST || k == MOB_WRAITH) return RUN.por_feat[k] > 0 || por_unlocked(k);
    for (int r = 0; r < NRANK; r++) if (RUN.rec.kills[k][r]) return 1;
    return 0;
}
static int por_can_draw(int i) {                                   /* a portrait's tapestry may come down: one of its kind slain, its boss fallen */
    int b = por_bossof(i);
    return b >= 0 ? boss_unlocked(b) != 0 : por_drawable(POR.at[i].kind);
}
static int por_is_drawn(int i) { return RUN.por_drawn >> por_bit(i) & 1; }
static int por_wait_gold(int k) {                                  /* what waits on it now */
    int w = por_waiting(k), g = 0;
    for (int r = 0; r < POR_GEMS; r++) if (w >> r & 1) g += por_gold(k, r);
    return g;
}
static void por_face_mid(int i, float *x, float *y) {              /* the middle of its face, on the wall's line */
    const PorAt *a = &POR.at[i];
    *x = (float)a->x + 0.5f + 0.5f * (float)DX[a->fd]; *y = (float)a->y + 0.5f + 0.5f * (float)DY[a->fd];
}
static int por_facing(void) {                                      /* 1 + the portrait a person faces, near enough to reach; 0 none */
    int best = 0;
    float bd = 2.8f;
    for (int i = 0; i < POR.n; i++) {
        const PorAt *a = &POR.at[i];
        float fx, fy;
        por_face_mid(i, &fx, &fy);
        float dx = fx - G.px, dy = fy - G.py, d = hypotf(dx, dy);
        if (d > bd || -(dx * (float)DX[a->fd] + dy * (float)DY[a->fd]) < 0.2f) continue;   /* on the room's side of it */
        if (!line_clear(G.px, G.py, fx + 0.1f * (float)DX[a->fd], fy + 0.1f * (float)DY[a->fd])) continue;   /* and in sight */
        if (fabsf(wrap_pi(atan2f(dy, dx) - G.ang)) < 0.3f + 0.35f / fmaxf(d, 0.3f)) { best = i + 1; bd = d; }
    }
    return best;
}
static void por_draw_back(int i, int dream) {                      /* its tapestry gathered to its side and tied there, for good (a
                                                                      boss's two, parted) */
    i = por_head(i);
    int k = POR.at[i].kind, b = por_bossof(i);
    (void)k;
    float fx, fy;
    por_face_mid(i, &fx, &fy);
    RUN.por_drawn |= 1u << por_bit(i);
    if (!dream) {
        POR.veil_t[i] = (float)G.t; if (b >= 0) POR.veil_t[i + 1] = (float)G.t;
        sfx_at(SFX_RUSTLE, fx, fy, 1.0f); snd_sfx_ex(SFX_WHOOSH, 0.25f, 0.5f);
    }
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT portrait_drawn %s by %s t=%.2f\n", b >= 0 ? POR_BOSS_NAME[b] : KINDS[k].plural, dream ? "dream" : "player", G.t);
#endif
}
static int por_take(int i) {                                       /* the gold that waits on portrait i, all of it; how much */
    i = por_head(i);
    int b = por_bossof(i);
    if (b >= 0) {                                                  /* a boss's: its deeds, once a life */
        int w = boss_waiting(b), g = boss_wait_gold(b);
        if (!g) return 0;
        RUN.treasure += g; RUN.por_btaken[b] |= (uint8_t)w;
        say("THE PORTRAIT PAYS %d", g);
        snd_sfx_ex(SFX_COIN, 1.1f, 0.5f); snd_sfx_ex(SFX_SHIMMER, 0.6f, 0.5f);
        if (G.player) pop_readout();
#ifdef CRAWL_DEBUG
        fprintf(stderr, "EVENT portrait_paid %s gold %d steps %x gold now %d t=%.2f\n", POR_BOSS_NAME[b], g, w, RUN.treasure, G.t);
#endif
        return g;
    }
    int k = POR.at[i].kind, w = por_waiting(k), g = por_wait_gold(k);
    if (!g) return 0;
    RUN.treasure += g; RUN.por_taken[k] |= (uint8_t)w;
    say("THE PORTRAIT PAYS %d", g);
    snd_sfx_ex(SFX_COIN, 1.1f, 0.5f); snd_sfx_ex(SFX_SHIMMER, 0.6f, 0.5f);
    if (G.player) pop_readout();
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT portrait_paid %s gold %d ranks %x gold now %d t=%.2f\n", KINDS[k].plural, g, w, RUN.treasure, G.t);
#endif
    return g;
}
/* the dream draws back a tapestry now and then (the owner's Q5): one visit in two, one at most, a moment after it comes in; never on a
 * page read from the ledger, and not with the dice, so its runs keep step */
static void por_dream(void) {
    if (LEDGER.yours || LEDGER.kept || !POR.n) return;
    uint32_t h = RUN.run_id * 2654435761u ^ (uint32_t)G.level * 40503u;
    h ^= h >> 15; h *= 2246822519u; h ^= h >> 13;
    if (h & 1) return;
    for (int j = 0; j < POR.n; j++) {
        int i = por_head((int)((h >> 1) % (uint32_t)POR.n + (uint32_t)j) % POR.n);
        if (por_is_drawn(i) || !por_can_draw(i)) continue;
        por_draw_back(i, 1);
        POR.veil_t[i] = (float)G.t + 4.0f + (float)(h >> 8 & 7);   /* while it looks about */
        if (POR.at[i].boss) POR.veil_t[i + 1] = POR.veil_t[i];
        return;
    }
}
static void update_portrait(void) {                                /* its card: A/D through every portrait, E draws back or takes */
    if (!POR.n || IN.hit[KEY_ESC] || IN.hit[KEY_TAB]) { G.ui = UI_NONE; snd_sfx_ex(SFX_KEY, 0.6f, 0.5f); return; }
    int lr = (IN.hit[KEY_D] || IN.hit[KEY_RIGHT]) - (IN.hit[KEY_A] || IN.hit[KEY_LEFT]);
    if (lr) {                                                      /* a boss's right half is passed over: its left stands for it */
        do G.pt_sel = (G.pt_sel + POR.n + lr) % POR.n; while (POR.at[G.pt_sel].boss && POR.at[G.pt_sel].half);
        snd_sfx_ex(SFX_TICK, 0.7f, 0.5f);
    }
    G.pt_sel = por_head(clampi(G.pt_sel, 0, POR.n - 1));
    if (!(IN.hit[KEY_E] || IN.hit[KEY_ENTER] || IN.hit[KEY_SPACE] || IN.hit[KEY_MOUSE1])) return;
    int i = G.pt_sel, k = POR.at[i].kind, b = por_bossof(i);
    if (!por_is_drawn(i)) {
        if (por_can_draw(i)) por_draw_back(i, 0);
        else { say(b >= 0 ? "WHAT HANGS HERE IS NOT YET SLAIN" : "NONE OF ITS KIND SLAIN YET"); snd_sfx_ex(SFX_KNOCK, 0.5f, 0.5f); }
    } else if (!por_take(i)) {
        snd_sfx_ex(SFX_KNOCK, 0.5f, 0.5f);
        if (b >= 0) say(!RUN.por_open ? "IT PAYS AFTER A LEVEL IS CLEARED" : "ITS GOLD IS TAKEN THIS LIFE");
        else if (!por_unlocked(k)) say("NO RANK OF IT UNLOCKED YET");
        else if (!RUN.por_open) say("IT PAYS AFTER A LEVEL IS CLEARED");
        else say("ITS GOLD IS TAKEN THIS LIFE");
    }
}

#define BUY_MAP (-2)                        /* the buying screen's map, in place of a pedestal */
static void buy_open(int i);
static void shop_human(float dt) {
    if (!RUN.cls) { G.vow_change = 0; open_ui(UI_CLASS); return; }   /* her question, before anything else */
    tu_show(); if (G.ui) return;                                   /* a lesson waiting: her wares (docs/PLAN-tutorial.md) */
    spell_keys();
    if (IN.hit[KEY_TAB]) { open_ui(UI_INVENTORY); return; }
    if (IN.hit[KEY_B]) { open_ui(UI_BESTIARY); return; }
    if (IN.hit[KEY_C]) { open_ui(UI_CODEX); return; }
    if (IN.hit[KEY_ESC]) { open_ui(UI_QUIT); return; }
    if (IN.hit[KEY_M]) say(RUN.has_map ? "NOTHING HERE TO MAP" : "YOU HAVE NO MAP. SHE SELLS ONE");
    if (gift_due() && !G.gift_said) { G.gift_said = 1; say("THE LAMPSTAND IS LIT FOR YOU: A GIFT WAITS"); }
    human_walk(dt);
    if (gift_due() && G.px > SHOP_DOOR - 0.55f && G.py > 13.0f && G.py < 18.0f) {   /* the way on is held till the gift is taken */
        G.px = SHOP_DOOR - 0.55f; G.cx = (int)G.px;
        if (G.t - G.gift_bump > 3.0) { G.gift_bump = G.t; say("NOT YET: THE LAMPSTAND IS LIT FOR YOU"); snd_sfx_ex(SFX_KNOCK, 0.4f, 0.5f); }
    }
    int was = G.shop_look;
    G.shop_look = -1;
    float bd = 3.2f;
    for (int i = 0; i < G.ped_n; i++) {
        if (G.offer_gone[i]) continue;
        float d = hypotf(G.ped_x[i] - G.px, G.ped_y[i] - G.py);
        if (d < bd && fabsf(wrap_pi(atan2f(G.ped_y[i] - G.py, G.ped_x[i] - G.px) - G.ang)) < 0.3f + 0.25f / d) { bd = d; G.shop_look = i; }
    }
    if (G.shop_look != was) G.shop_confirm = -1;                   /* a warning is for the ware it was given over */
    float sd = hypotf(SHRINE_X - G.px, SHRINE_Y - G.py);
    G.sh_look = G.shop_look < 0 && sd < 3.0f && fabsf(wrap_pi(atan2f(SHRINE_Y - G.py, SHRINE_X - G.px) - G.ang)) < 0.3f + 0.3f / sd;
    float td = hypotf(TOTEM_X - G.px, TOTEM_Y - G.py);
    G.tot_look = G.shop_look < 0 && !G.sh_look && td < 3.6f && fabsf(wrap_pi(atan2f(TOTEM_Y - G.py, TOTEM_X - G.px) - G.ang)) < 0.3f + 0.3f / td;
    float lx = LAMP_X + 0.5f - G.px, ly = LAMP_Y + 0.5f - G.py, ld = hypotf(lx, ly);   /* the Lampstand, in the room north of hers */
    G.lamp_look = G.shop_look < 0 && !G.sh_look && !G.tot_look && ld < 3.4f && fabsf(wrap_pi(atan2f(ly, lx) - G.ang)) < 0.3f + 0.35f / ld;
    float cdx = LECTERN_X + 0.5f - G.px, cdy = LECTERN_Y + 0.5f - G.py, cd = hypotf(cdx, cdy);
    G.comp_look = G.shop_look < 0 && !G.sh_look && !G.tot_look && cd < 2.6f && fabsf(wrap_pi(atan2f(cdy, cdx) - G.ang)) < 0.3f + 0.35f / cd;
    float bdx = BELL_X + 0.5f - G.px, bdy = BELL_Y + 0.5f - G.py, bd2 = hypotf(bdx, bdy), fdx = FONT_X + 0.5f - G.px, fdy = FONT_Y + 0.5f - G.py, fd = hypotf(fdx, fdy);
    G.bell_look = G.shop_look < 0 && !G.lamp_look && bd2 < 2.8f && fabsf(wrap_pi(atan2f(bdy, bdx) - G.ang)) < 0.3f + 0.35f / bd2;   /* the bell, */
    G.font_look = G.shop_look < 0 && !G.lamp_look && !G.bell_look && fd < 2.8f && fabsf(wrap_pi(atan2f(fdy, fdx) - G.ang)) < 0.3f + 0.35f / fd;   /* the font */
    G.rep_look = G.shop_look < 0 && !G.sh_look && !G.tot_look && !G.comp_look && G.px > 3.9f && G.px < 6.6f && G.py > 12.6f && G.py < 17.4f
                 && fabsf(wrap_pi(G.ang - PI_F)) < 0.55f;          /* facing the niches across the back room */
    float pdx = PORTAL_X + 0.5f - G.px, pdy = PORTAL_Y + 0.1f - G.py, pd = hypotf(pdx, pdy);
    G.por_look = G.shop_look < 0 && !G.sh_look && !G.tot_look && !G.comp_look && !G.rep_look && pd < 3.4f && fabsf(wrap_pi(atan2f(pdy, pdx) - G.ang)) < 0.3f + 0.35f / pd;
    G.stone_look = -1;
    for (int k = 0; k < RUN.nstones && G.shop_look < 0 && !G.sh_look && !G.tot_look && !G.comp_look && !G.rep_look && !G.por_look; k++) {   /* a stone in the plot */
        float sx = STONE_AT[k][0] + 0.5f, sy = STONE_AT[k][1] + 0.5f, d = hypotf(sx - G.px, sy - G.py);
        if (d < 2.4f && fabsf(wrap_pi(atan2f(sy - G.py, sx - G.px) - G.ang)) < 0.25f + 0.3f / d
            && (G.stone_look < 0 || d < hypotf(STONE_AT[G.stone_look][0] + 0.5f - G.px, STONE_AT[G.stone_look][1] + 0.5f - G.py))) G.stone_look = k;
    }
    float mdx = MAP_X - G.px, mdy = MAP_Y - G.py, md = hypotf(mdx, mdy);   /* the map at her left, until it is yours */
    G.map_look = !RUN.has_map && !g_map_kept && G.shop_look < 0 && !G.sh_look && !G.tot_look && !G.comp_look && !G.rep_look && !G.por_look && G.stone_look < 0
                 && md < 2.6f && fabsf(wrap_pi(atan2f(mdy, mdx) - G.ang)) < 0.25f + 0.25f / fmaxf(md, 0.3f);
    G.pt_look = G.shop_look < 0 && !G.sh_look && !G.tot_look && !G.comp_look && !G.rep_look && !G.por_look && G.stone_look < 0 && !G.map_look
                && !G.lamp_look && !G.bell_look && !G.font_look ? por_facing() : 0;   /* a portrait of the slain */
    G.mer_look = G.shop_look < 0 && !G.sh_look && !G.tot_look && !G.comp_look && !G.rep_look && !G.por_look && G.stone_look < 0 && !G.map_look
                 && !G.pt_look && hypotf(G.px - CAT_X, G.py - CAT_Y) < 1.8f;
#ifdef CRAWL_DEBUG
    if (G.shop_look != was) fprintf(stderr, "EVENT shop_look %d t=%.2f at (%.2f,%.2f) ang %.3f\n", G.shop_look, G.t, G.px, G.py, G.ang);
#endif
    int i = G.shop_look;
    if (CAST_UP) G.click_spent = 0;
    if (IN.hit[KEY_E] || IN.hit[KEY_SPACE] || IN.hit[KEY_MOUSE1]) {
        if (i >= 0) { G.click_spent = CAST_HIT; buy_open(i); }    /* a ware: its buying, on a screen of its own (after 1.0.0b21.2) */
        else if (G.sh_look) { open_ui(UI_SHRINE); G.click_spent = CAST_HIT; G.grab_t = 1.0f; }
        else if (G.tot_look) { open_ui(UI_TOTEM); G.click_spent = CAST_HIT; G.grab_t = 1.0f; }
        else if (G.lamp_look) { open_ui(UI_ALIGN); G.click_spent = CAST_HIT; }
        else if (G.bell_look || G.font_look) { G.h2_obj = G.font_look; open_ui(UI_HALL2); G.click_spent = CAST_HIT; }
        else if (G.comp_look) { open_ui(UI_COMP); G.click_spent = CAST_HIT; }
        else if (G.rep_look) { open_ui(UI_REPO); G.click_spent = CAST_HIT; }
        else if (G.por_look) { open_ui(UI_PORTAL); G.click_spent = CAST_HIT; }
        else if (G.stone_look >= 0) { G.stone_sel = G.stone_look; open_ui(UI_STONE); G.click_spent = CAST_HIT; }
        else if (G.pt_look) {                                      /* a portrait: its tapestry drawn back, or its card */
            int p = por_head(G.pt_look - 1);
            G.click_spent = CAST_HIT;
            if (!por_is_drawn(p) && por_can_draw(p)) por_draw_back(p, 0);
            else { G.pt_sel = p; open_ui(UI_PORTRAIT); snd_sfx_ex(SFX_PAGE, 0.5f, 0.5f); }
        }
        else if (G.map_look) { G.click_spent = CAST_HIT; buy_open(BUY_MAP); }
        else if (!human_use() && G.mer_look) { open_ui(UI_MERCHANT); G.click_spent = CAST_HIT; }   /* her menu: the ledger, a pat */
    }
    human_cast(dt, !G.click_spent);
}

/* the bag is full and a ware is bought for a slot already worn: let something go to make room, or not */
static void update_letgo(void) {
    Gear *g = &RUN.gear;
    int n = g->bag_size;
    if (IN.hit[KEY_ESC] || IN.hit[KEY_TAB]) { G.ui = UI_NONE; G.shop_letgo = -1; say("THOUGHT BETTER OF IT"); snd_sfx_ex(SFX_KEY, 0.6f, 0.5f); return; }
    if (IN.hit[KEY_W] || IN.hit[KEY_UP]) { G.ui_sel = (G.ui_sel + n - 1) % n; snd_sfx_ex(SFX_TICK, 0.6f, 0.5f); }
    if (IN.hit[KEY_S] || IN.hit[KEY_DOWN]) { G.ui_sel = (G.ui_sel + 1) % n; snd_sfx_ex(SFX_TICK, 0.6f, 0.5f); }
    if (IN.hit[KEY_ENTER] || IN.hit[KEY_E] || IN.hit[KEY_SPACE]) {
        int i = G.shop_letgo;
        memset(&g->bag[G.ui_sel], 0, sizeof g->bag[G.ui_sel]);    /* gone, and nothing for it */
        G.ui = UI_NONE; G.shop_letgo = -1;
        if (i >= 0 && !G.offer_gone[i] && G.offers[i].price <= RUN.treasure && G.shop_left > 0) { shop_purchase(i); G.grab_t = 1.0f; }
    }
}

/* A ware's buying (after 1.0.0b21.2, at the owner's word): E at a pedestal, or at the map, opens a screen of its own with what it is,
 * what is worn there now, what changes in the caster's numbers and what becomes of the old piece. E buys (twice where the old
 * would be lost), Q holds it, A and D go round the rest of her wares, Esc puts it away. */
static void buy_open(int i) {
    G.shop_buy = i; G.shop_confirm = -1; G.sh_note_t = 0.0f;
    open_ui(UI_BUY);
    snd_sfx_ex(SFX_PAGE, 0.5f, 0.5f);
}
static int buy_live(int i) { return i == BUY_MAP ? !RUN.has_map && !g_map_kept : i >= 0 && i < G.ped_n && !G.offer_gone[i]; }
static int buy_next(int from, int dir) {                           /* the next ware still to be had, round the pedestals and the map */
    int n = G.ped_n + 1, at = from == BUY_MAP ? G.ped_n : from;
    for (int k = 1; k <= n; k++) {
        int j = ((at + dir * k) % n + n) % n, w = j == G.ped_n ? BUY_MAP : j;
        if (buy_live(w)) return w;
    }
    return from;
}
static void map_buy(void) {
    if (RUN.treasure < MAP_PRICE) { shop_say("YOU CAN'T AFFORD THAT"); G.cat_ear = 0.32f; G.cat_ear_side = rnd(2); return; }
    RUN.treasure -= MAP_PRICE; RUN.has_map = 1; G.map_look = 0; G.grab_t = 1.0f;
    G.ui = UI_NONE;
    say(PADX.used ? "THE MAP IS YOURS: D-PAD UP OPENS IT" : "THE MAP IS YOURS: M OPENS IT");
    snd_sfx_ex(SFX_COIN, 1.1f, 0.5f); snd_sfx_ex(SFX_SHIMMER, 0.8f, 0.5f);
    sfx_at(SFX_TRILL, CAT_X, CAT_Y, 1.2f); G.cat_trill = 0.5f; G.cat_slow = 1.4f; G.grin_t = 2.0f;
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT map_bought left %d t=%.2f\n", RUN.treasure, G.t);
#endif
}
static int buy_worn(int i) {                                       /* the slot a ware would be worn in that is worn already; -1 none */
    const Offer *o = &G.offers[i];
    if (o->kind != GK_PIECE || o->piece.kind == IK_BAG) return -1;
    int sl = piece_slot(&RUN.gear, o->piece.kind);
    return RUN.gear.worn[sl].kind != IK_NONE ? sl : -1;
}
static void update_buy(void) {
    int i = G.shop_buy;
    if (!buy_live(i)) { G.ui = UI_NONE; G.shop_confirm = -1; return; }
    if (IN.hit[KEY_ESC] || IN.hit[KEY_TAB] || IN.hit[KEY_MOUSE2]) { G.ui = UI_NONE; G.shop_confirm = -1; snd_sfx_ex(SFX_KEY, 0.6f, 0.5f); return; }
    if (IN.hit[KEY_A] || IN.hit[KEY_LEFT] || IN.hit[KEY_D] || IN.hit[KEY_RIGHT]) {
        int j = buy_next(i, IN.hit[KEY_A] || IN.hit[KEY_LEFT] ? -1 : 1);
        if (j != i) { G.shop_buy = j; G.shop_confirm = -1; G.sh_note_t = 0.0f; snd_sfx_ex(SFX_TICK, 0.6f, 0.5f); }
        return;
    }
    if (i == BUY_MAP) { if (IN.hit[KEY_E] || IN.hit[KEY_ENTER]) map_buy(); return; }
    if (IN.hit[KEY_Q] || IN.hit[KEY_SPACE] || IN.hit[KEY_BACK]) {  /* held for the next visit (Y or X on a controller) */
        if (G.shop_left <= 0) shop_say("NO MORE WARES THIS VISIT"); else shop_hold(i);
        return;
    }
    if (!(IN.hit[KEY_E] || IN.hit[KEY_ENTER])) return;
    const Offer *o = &G.offers[i];
    int worn = buy_worn(i);
    if (G.shop_left <= 0) shop_say("NO MORE WARES THIS VISIT");
    else if (o->price > RUN.treasure) { shop_say("YOU CAN'T AFFORD THAT"); G.cat_ear = 0.32f; G.cat_ear_side = rnd(2); }
    else if (worn >= 0 && !RUN.gear.bag_size && G.shop_confirm != i) { G.shop_confirm = i; snd_sfx_ex(SFX_KNOCK, 0.7f, 0.5f); }   /* the old would be lost: a second E buys */
    else if (worn >= 0 && RUN.gear.bag_size && !gear_bag_free(&RUN.gear)) { G.shop_letgo = i; G.ui = UI_LETGO; G.ui_sel = 0; snd_sfx_ex(SFX_KEY, 0.7f, 0.5f); }
    else { G.ui = UI_NONE; shop_purchase(i); G.grab_t = 1.0f; }
}

/* the person takes over, from wherever the autopilot was */
static void human_begin(void) {
    if (g_drawn && !TU.on && !G.shop) tu_level_begin();          /* taken over on a drawn level: its lessons too */
    G.state = PS_PLAYER; G.pc_t = -1.0f; G.u = 1.0f; G.spd = 0.0f; G.seek_secret = -1; G.dash_t = 0.0f; G.tap_key = -1;
    G.cx = G.tx = (int)G.px; G.cy = G.ty = (int)G.py;
    if (RUN.gear.atk[G.spell] < 0) G.spell = own_spell();
}

/* back to the autopilot: the merchant's routine picks up where it fits; in the crypt, settle into the grid */
static void human_end(void) {
    G.player = 0; G.ui = UI_NONE; G.status_ret = PLAY_AUTO; G.pc_t = -1.0f; G.dash_t = 0.0f; G.map_on = 0;
    if (G.shop) {
        if (G.shop_left <= 0) { G.shop_step = SH_LEAVE; G.shop_t = 0.0; shop_walk(SHOP_DOOR - 0.8f, 15.5f, 1.8f); }
        else { G.shop_step = SH_THINK; G.shop_t = 0.0; shop_turn(face_of(SHOP_MID, 13.0f), 0.5f); }
        return;
    }
    if (G.exiting || G.tally || G.dead) return;
    if (PK.depth) { pocket_return(0); return; }                    /* sleepwalking in a pocket: the feet find the way back out */
    if (G.bchallenge) G.leaving = 1;                               /* in a challenge: on into the fight */
    G.cx = G.tx = (int)G.px; G.cy = G.ty = (int)G.py;
    if (!apass(G.cx, G.cy)) {                                      /* onto the nearest lane */
        int bd = 99, bx = G.cx, by = G.cy;
        for (int y = G.cy - 3; y <= G.cy + 3; y++)
            for (int x = G.cx - 3; x <= G.cx + 3; x++) {
                int dd = abs(x - G.cx) + abs(y - G.cy);
                if (apass(x, y) && dd < bd && line_clear(G.px, G.py, x + 0.5f, y + 0.5f)) { bd = dd; bx = x; by = y; }
            }
        G.cx = G.tx = bx; G.cy = G.ty = by;
    }
    G.sx0 = G.px; G.sy0 = G.py;
    int h = ((int)lroundf(wrap_pi(G.ang) / (PI_F * 0.5f)) + 4) & 3;
    G.h = G.next_h = h;
    G.ang0 = G.ang; G.ang1 = G.ang + wrap_pi(heading_angle(h) - G.ang);
    G.state = PS_SETTLE; G.st = 0.0f;
}

/* ---- the inventory (Tab): what is worn, the bag, and the spellbook ---- */

static const char *const SLOT_NAME[NSLOT] = { "LEFT WRIST", "RIGHT WRIST", "BOOTS", "NECK", "HOOD", "ROBES", "RING" };

static int inv_rows(void) { return NSLOT + RUN.gear.bag_size; }
static const Piece *inv_piece(int r) { return r < NSLOT ? &RUN.gear.worn[r] : &RUN.gear.bag[r - NSLOT]; }

/* what Enter would do with the row: 1 into the bag, 2 worn; or why not (as a negative) */
enum { INV_NOTHING = -1, INV_NO_BAG = -2, INV_BAG_FULL = -3, INV_TO_BAG = 1, INV_TO_WEAR = 2 };
static int inv_action(int r) {
    const Piece *p = inv_piece(r);
    if (p->kind == IK_NONE) return INV_NOTHING;
    if (r >= NSLOT) return INV_TO_WEAR;
    if (!RUN.gear.bag_size) return INV_NO_BAG;
    return gear_bag_free(&RUN.gear) ? INV_TO_BAG : INV_BAG_FULL;
}

static void update_stones_core(void);
static void update_align(int view);
static void render_align(Frame *f, int view);
static void update_inventory(void) {
    spell_keys();
    if (IN.hit[KEY_TAB] || IN.hit[KEY_ESC]) { G.ui = UI_NONE; snd_sfx_ex(SFX_KEY, 0.6f, 0.5f); return; }
    int lr = (IN.hit[KEY_D] || IN.hit[KEY_RIGHT]) - (IN.hit[KEY_A] || IN.hit[KEY_LEFT]);
    if (lr) {                                                      /* the wardrobe, the caster, the cornerstones */
        G.inv_page = (G.inv_page + NINV + lr) % NINV; G.cs_arm = -1; snd_sfx_ex(SFX_TICK, 0.8f, 0.5f);
        if (G.inv_page == 1) for (int l = 0; l < NLINE + NCONF; l++) if (l < NLINE ? RUN.over[l] : RUN.conf_over[l - NLINE]) { snd_sfx_ex(SFX_WHISPER, 0.3f, 0.2f); break; }
    }
    if (G.inv_page == 2) { update_stones_core(); return; }
    if (G.inv_page == 3) { update_align(1); return; }               /* the Alignment, to look at */
    if (G.inv_page == 4) return;                                   /* the elements: only to look at */
    if (G.inv_page == 5) { docs_keys(); return; }                  /* the documents (1.0.0b13) */
    if (G.inv_page) return;
    int n = inv_rows();
    if (G.ui_sel >= n) G.ui_sel = n - 1;
    if (IN.hit[KEY_W] || IN.hit[KEY_UP]) { G.ui_sel = (G.ui_sel + n - 1) % n; snd_sfx_ex(SFX_TICK, 0.6f, 0.5f); }
    if (IN.hit[KEY_S] || IN.hit[KEY_DOWN]) { G.ui_sel = (G.ui_sel + 1) % n; snd_sfx_ex(SFX_TICK, 0.6f, 0.5f); }
    if (!(IN.hit[KEY_ENTER] || IN.hit[KEY_E] || IN.hit[KEY_SPACE])) return;
    Gear *g = &RUN.gear;
    int r = G.ui_sel, act = inv_action(r);
    if (act == INV_TO_BAG) {
        for (int i = 0; i < g->bag_size; i++)
            if (g->bag[i].kind == IK_NONE) { g->bag[i] = g->worn[r]; memset(&g->worn[r], 0, sizeof g->worn[r]); break; }
    } else if (act == INV_TO_WEAR) {
        int bi = r - NSLOT, slot = piece_slot(g, g->bag[bi].kind);
        gear_wear(g, slot, bi);
        if (g->worn[slot].ability == AB_KEYMAKER) say("KEYMAKER: KEY WISPS IN THE L MENU");   /* and every time it is put on */
    } else { snd_sfx_ex(SFX_KNOCK, 0.5f, 0.5f); return; }
    G.max_hp = gear_max_hp(g);
    if (G.hp > G.max_hp) G.hp = G.max_hp;
    snd_sfx_ex(SFX_STEP, 1.2f, 0.5f);
}

/* ---- Esc: leave the crypt? ---- */

enum { Q_FLEE, Q_STAY, Q_SLEEP, Q_OPEN, Q_NEW, Q_PAD, Q_OPTIONS };   /* Q_PAD only while a pad is in play */
enum { Q_TTURN = Q_PAD, Q_TSMOOTH = Q_OPTIONS };                   /* by touch, its turning and smoothing there instead */
static int quit_touch(void) { return IN.touch && IN.stage_w && !IN.pad; }
static int quit_rows(void) { return quit_touch() ? Q_OPTIONS + 1 : IN.pad ? Q_OPTIONS : Q_PAD; }
static void quit_tune(int smooth, int d) {                         /* A and D (a slide by touch) a step; Enter steps round */
    int *v = smooth ? &PL.tsmooth : &PL.tturn, lo = smooth ? 0 : 3, hi = smooth ? 10 : 30;
    *v = d > 1 ? (*v >= hi ? lo : *v + 1) : clampi(*v + d, lo, hi);
    pl_write();
    snd_sfx_ex(SFX_TICK, 0.6f, 0.5f);
}
static void open_ledger(int mode);
/* a new run from nothing, a person's from the start, so she asks what you were; the pages stay in the ledger,
 * and what was not signed is lost, as in fleeing */
static void run_new(void) {
    uint32_t seed = G.rs * 1664525u + 1013904223u;
    pl_read();
    if (PL.tut & TUT_AGAIN) { PL.tut = 0; pl_write(); }            /* the lessons asked for again: this run has them */
    int ql = G.quit_line;
    RUN.active = 0;                                                /* crawl_enter makes it fresh */
    pocket_reset_level();                                          /* out of any pocket, its hum with it */
    LEDGER.page = 0; LEDGER.has_visit = 0; LEDGER.kept = 0; LEDGER.claim_due = 0;
    LEDGER.marked = 0;                                             /* a page left behind keeps its own mark */
    crawl_enter(seed);
    G.player = 1; G.status_ret = PLAY_PLAYER; G.ui = UI_NONE; G.quit_line = ql;
    G.sdur = 0.0f; G.shop_look = -1; G.pc_t = -1.0f;               /* her room; no cast left half-made */
    say("A NEW RUN BEGINS");
    snd_sfx_ex(SFX_WHOOSH, 0.8f, 0.5f); snd_sfx_ex(SFX_WHISPER, 0.8f, 0.5f);
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT new_run run %08x level %d shop %d t=%.2f\n", RUN.run_id, G.level, G.shop, G.t);
#endif
}
static void quit_choose(int c) {
    if (c == Q_OPEN) { open_ledger(1); return; }
    if (c == Q_PAD) { pad_menu_open(); return; }                   /* the controller's screen (1.0.0b18.1) */
    if (c == Q_NEW) {                                              /* asked twice: what is unsigned is lost */
        if (!G.quit_arm) { G.quit_arm = 1; snd_sfx_ex(SFX_KNOCK, 0.6f, 0.5f); return; }
        run_new();
        return;
    }
    if (c == Q_FLEE) { G.status_ret = PLAY_QUIT; snd_sfx_ex(SFX_GHOST_DIE, 0.8f, 0.5f); }
    else if (c == Q_STAY) { G.ui = UI_NONE; snd_sfx_ex(SFX_KEY, 0.6f, 0.5f); }
    else { G.ui = UI_NONE; human_end(); snd_sfx_ex(SFX_WHISPER, 0.7f, 0.5f); }
}
static void update_quit(void) {
    if (G.status_ret == PLAY_QUIT) return;
    int nq = quit_rows();
    G.ui_sel = clampi(G.ui_sel, 0, nq - 1);
    if (IN.hit[KEY_W] || IN.hit[KEY_UP]) { G.ui_sel = (G.ui_sel + nq - 1) % nq; G.quit_arm = 0; snd_sfx_ex(SFX_TICK, 0.6f, 0.5f); }
    if (IN.hit[KEY_S] || IN.hit[KEY_DOWN]) { G.ui_sel = (G.ui_sel + 1) % nq; G.quit_arm = 0; snd_sfx_ex(SFX_TICK, 0.6f, 0.5f); }
    if (quit_touch() && G.ui_sel >= Q_TTURN) {
        int d = (IN.hit[KEY_D] || IN.hit[KEY_RIGHT]) - (IN.hit[KEY_A] || IN.hit[KEY_LEFT]);
        if (IN.hit[KEY_ENTER] || IN.hit[KEY_SPACE] || IN.hit[KEY_E]) d = 2;
        if (d) { quit_tune(G.ui_sel == Q_TSMOOTH, d); return; }
    }
    if (IN.hit[KEY_ENTER] || IN.hit[KEY_SPACE] || IN.hit[KEY_E] || IN.hit[KEY_MOUSE1]) { quit_choose(G.ui_sel); return; }
    if (IN.hit[KEY_Y]) { quit_choose(Q_FLEE); return; }
    if (IN.hit[KEY_N] || IN.hit[KEY_ESC]) { quit_choose(Q_STAY); return; }
}

/* where the console's codes and the test switches put the caster */
static void tp_exit(void) { G.px = G.exit_fx + 0.5f; G.py = G.exit_fy + 0.5f; G.ang = heading_angle(G.exit_dir); human_begin(); }
static void tp_secret(int i) {                                     /* three steps back from it, facing it */
    const Secret *sc = &G.secrets[i];
    G.px = sc->fx + 0.5f - DX[sc->d] * 2.5f; G.py = sc->fy + 0.5f - DY[sc->d] * 2.5f; G.ang = heading_angle(sc->d); human_begin();
}
#ifdef CRAWL_DEBUG
/* CRAWL_TPHOLD=k (step 7): a doorway held. In a room three cells in from an open door and facing it, a creature of kind k
 * (the MOB_* number) three cells beyond it, after you */
static void tp_hold(int k) {
    for (int i = 0; i < G.ndoors; i++) {
        Door *dr = &G.doors[i];
        if (dr->locked) continue;
        for (int s = -1; s <= 1; s += 2) {
            int dx = dr->along_x ? s : 0, dy = dr->along_x ? 0 : s, ok = 1;
            for (int j = 1; j <= 3 && ok; j++)
                ok = G.map[dr->y + dy * j][dr->x + dx * j].kind == K_FLOOR && G.map[dr->y - dy * j][dr->x - dx * j].kind == K_FLOOR
                  && !occupied(dr->x - dx * j, dr->y - dy * j, NULL);
            int ix = dr->x + dx * 3, iy = dr->y + dy * 3;
            if (!ok || room_of_region(G.map[iy][ix].region) < 0 || in_band(iy)) continue;
            G.px = ix + 0.5f; G.py = iy + 0.5f; G.ang = atan2f((float)-dy, (float)-dx); human_begin();
            dr->open = 1.0f;
            Mob *m = spawn_mob(dr->x - dx * 3, dr->y - dy * 3, clampi(k, 0, NKINDS - 1));
            if (m) { m->alert = 30.0f; m->aware = MA_HUNT; m->kx = G.px; m->ky = G.py; m->face = atan2f((float)dy, (float)dx); }
            fprintf(stderr, "TPHOLD door %d at (%d,%d), you at (%d,%d), %s at (%d,%d)\n", i, dr->x, dr->y, ix, iy, m ? KINDS[k].name : "nothing", dr->x - dx * 3, dr->y - dy * 3);
            return;
        }
    }
}
static void tp_room(int k) {                                       /* in the first room of type k, in a corner, looking across to the far one */
    for (int r = 0; r < G.nrooms; r++) {
        const Room *R = &G.rooms[r];
        if (R->rtype != k) continue;
        for (int c = 0; c < 4; c++) {
            int x = c & 1 ? R->x + R->w - 1 : R->x, y = c & 2 ? R->y + R->h - 1 : R->y;
            if (G.map[y][x].kind != K_FLOOR) continue;
            G.px = x + 0.5f; G.py = y + 0.5f; G.cx = x; G.cy = y;
            G.ang = atan2f((float)((c & 2 ? -1 : 1) * (R->h - 1)), (float)((c & 1 ? -1 : 1) * (R->w - 1)));
            human_begin();
            return;
        }
    }
}
/* stand CRAWL_TPPROPD cells (2.2) from (tx, ty), facing it, on floor with a clear line to just short of it (a pillar is solid):
 * the distance asked, then nearer, then further */
static int tp_before(float tx, float ty) {
    float want = getenv("CRAWL_TPPROPD") ? (float)atof(getenv("CRAWL_TPPROPD")) : 2.2f;
    for (int pass = 0; pass < 3; pass++)
        for (int a = 0; a < 24; a++) {
            float ang = TAU_F * (float)((a * 7) % 24) / 24.0f, d = want * (pass == 0 ? 1.0f : pass == 1 ? 0.7f : 1.4f);
            float x = tx + cosf(ang) * d, y = ty + sinf(ang) * d;
            if (x < 1.0f || y < 1.0f || x >= MAPN - 1 || y >= MAPN - 1 || G.map[(int)y][(int)x].kind != K_FLOOR
                || !line_clear(x, y, tx + cosf(ang) * 0.6f, ty + sinf(ang) * 0.6f)) continue;
            G.px = x; G.py = y; G.cx = (int)x; G.cy = (int)y; G.ang = atan2f(ty - y, tx - x);
            human_begin();
            fprintf(stderr, "TPPROP at %.2f,%.2f from %.2f,%.2f\n", tx, ty, x, y);
            return 1;
        }
    return 0;
}
/* before the first decoration drawn as spr (a look counts as itself), or with CRAWL_TPITEM=k the first thing to pick up of type
 * k (IT_*); CRAWL_TPPROPN=n the n-th such (from 0) */
static void tp_prop(int spr) {
    int nth = getenv("CRAWL_TPPROPN") ? atoi(getenv("CRAWL_TPPROPN")) : 0, item = getenv("CRAWL_TPITEM") ? atoi(getenv("CRAWL_TPITEM")) : -1;
    if (item >= 0) {
        for (int i = 0; i < G.nitems; i++)
            if (G.items[i].alive && G.items[i].type == item && nth-- <= 0 && tp_before(G.items[i].x, G.items[i].y)) return;
    } else
        for (int i = 0; i < G.ndecos; i++) {
            const Deco *dc = &G.decos[i];
            if (dc->spr < 0 || deco_look(dc) != spr || nth-- > 0) continue;
            if (tp_before(dc->x, dc->y)) return;
        }
    fprintf(stderr, "TPPROP spr %d item %d: none\n", spr, item);
}
static void tp_door(void) {                                        /* a chained room opened, and you before its doorway */
    for (int i = 0; i < G.nsecrets; i++) {
        Secret *sc = &G.secrets[i];
        if (sc->door_x < 0) continue;
        if (!sc->state) { open_secret(i); while (G.slide >= 0) update_slide(0.5f); }
        for (int n = 0; n < 4; n++) {
            int fx = sc->door_x - DX[n], fy = sc->door_y - DY[n];
            if (fx < sc->x0 || fx > sc->x1 || fy < sc->y0 || fy > sc->y1 || G.map[fy][fx].kind != K_FLOOR) continue;
            int bx = fx - DX[n], by = fy - DY[n];
            float back = (bx >= sc->x0 && bx <= sc->x1 && by >= sc->y0 && by <= sc->y1 && G.map[by][bx].kind == K_FLOOR) ? 1.0f : 0.0f;
            G.px = fx + 0.5f - DX[n] * back; G.py = fy + 0.5f - DY[n] * back; G.ang = heading_angle(n); human_begin();
            int k = clampi(atoi(getenv("CRAWL_TPDOOR")), 1, sc->links);   /* 2 on: straight into that link of the chain */
            if (k < 2) return;
            G.cx = fx; G.cy = fy;
            pocket_door(sc->door_x, sc->door_y);
            while (PK.depth < k) pocket_build(PK.depth + 1, 0);
            if (PK.on_x >= 0) { stand_at_door(PK.on_x, PK.on_y); G.ang = wrap_pi(G.ang + PI_F); }   /* facing the way on */
            if (getenv("CRAWL_TPTHING")) {                          /* or a few steps from the k-th thing out of place, facing it */
                int want = atoi(getenv("CRAWL_TPTHING")), seen = 0;
                for (int i = 0; i < G.ndecos; i++) {
                    const Deco *dc = &G.decos[i];
                    if (!(dc->flags & (DF_FLIP | DF_FLOAT | DF_SINK | DF_LOOP)) && dc->spr != SP_DESKS) continue;
                    if (seen++ != want) continue;
                    int tx = (int)dc->x, ty = (int)dc->y;
                    for (int d = 0, done = 0; d < 4 && !done; d++)
                        for (int r = 3; r >= 2 && !done; r--) {
                            int ok = 1;
                            for (int k = 1; k <= r; k++) ok &= G.map[ty + DY[d] * k][tx + DX[d] * k].kind == K_FLOOR;
                            if (!ok) continue;
                            G.px = tx + DX[d] * r + 0.5f; G.py = ty + DY[d] * r + 0.5f; G.ang = heading_angle((d + 2) & 3); done = 1;
                        }
                    fprintf(stderr, "THING %d: sprite %d flags %d at (%.1f,%.1f)\n", want, dc->spr, dc->flags, dc->x, dc->y);
                }
            }
            if (getenv("CRAWL_TPVIS") && G.vis_x >= 0) {            /* or a few steps from the visportal (or cooler), facing it */
                int bd = -1, bfar = 0;
                for (int d = 0; d < 4; d++) {                       /* from the side with the longest clear run */
                    int far = 0;
                    while (far < 3 && G.map[G.vis_y + DY[d] * (far + 1)][G.vis_x + DX[d] * (far + 1)].kind == K_FLOOR) far++;
                    if (far > bfar) { bfar = far; bd = d; }
                }
                if (bd >= 0) {
                    int k = bfar > 1 ? bfar : 1;
                    G.px = G.vis_x + DX[bd] * k + 0.5f; G.py = G.vis_y + DY[bd] * k + 0.5f; G.ang = heading_angle((bd + 2) & 3);
                }
            }
            return;
        }
    }
}
#endif
#ifdef CRAWL_DEBUG
static int tp_hz(void) {                                           /* testing: before the level's first fixed hazard, facing it */
    if (!G.nsite) return 0;
    const HzSite *s = &G.site[0];
    float tx = s->wx >= 0 ? s->ax : s->x + 0.5f, ty = s->wx >= 0 ? s->ay : s->y + 0.5f;
    for (int r = 4; r >= 2; r--)                                    /* as far back as can be, up to three cells */
        for (int d = 0; d < 4; d++) {
            int x = (s->wx >= 0 ? s->x - DX[s->d] * (r - 2) : s->x) + DX[d] * (s->wx >= 0 ? 0 : r), y = (s->wx >= 0 ? s->y - DY[s->d] * (r - 2) : s->y) + DY[d] * (s->wx >= 0 ? 0 : r);
            if (!hz_floor(x, y) || !line_clear(x + 0.5f, y + 0.5f, tx, ty)) continue;
            G.px = x + 0.5f; G.py = y + 0.5f; G.ang = atan2f(ty - G.py, tx - G.px); human_begin();
            if (s->wx >= 0) { G.px -= DX[s->d] * 0.3f; G.py -= DY[s->d] * 0.3f; }
            return 1;
        }
    return 0;
}
#endif
static int tp_altar(void) {                                        /* before the Black Altar, facing it */
    for (int d = 0; d < 4 && G.altar_x >= 0; d++) {
        int x = G.altar_x + DX[d], y = G.altar_y + DY[d];
        if (!is_open(x, y)) continue;
        int k = vault_of_room(x, y);                               /* in a vault: its door unlocked, as if by its key */
        if (k >= 0 && G.vault_door[k] >= 0 && G.doors[G.vault_door[k]].locked == 1) G.doors[G.vault_door[k]].locked = 2;
        G.px = x + 0.5f; G.py = y + 0.5f; G.ang = heading_angle((d + 2) & 3); human_begin();
        return 1;
    }
    return 0;
}

