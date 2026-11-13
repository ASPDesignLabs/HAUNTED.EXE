/* A part of scene_crawl.c (docs/PLAN-tidy.md): it is #included there in its place and never compiled on its own.
 * crawl_render: the frame's order; the music's side (music_frame); touch's stones (crawl_stage); the scene itself. */

static void crawl_render(Frame *f) {
    f->w = W; f->h = H;
    memcpy(f->pal, crawl_pal, sizeof crawl_pal);
    f->fade = G.fade;
    ui_lent = 0; ui_ramps(0);                                     /* the HUD in the run's stone; a menu lends the marble (ui_on) */
#ifdef CRAWL_DEBUG
    ui_check = 1; ui_check_frame();
#endif
    if (title_render(f)) return;                                   /* the title screen, the whole frame (1.0.0b21.1) */
    if (G.tally) { render_tally(f); if (G.player && G.ui) render_ui(f); pad_look_render(f); con_render(f); return; }   /* toasts wait for the next level */
    g_camfog = fog_at(G.px, G.py);
    fog_tint = G.tri >= 0 && G.tri_state == TRS_ON && in_trial((int)G.px, (int)G.py) ? TRI_FOG[G.tri] : 0;   /* a trial's room, turned */
    float stun = af_on(AF_STUN) ? 1.0f : 0.0f;                    /* stunned, the view swims; knocked down, it lies on the floor */
    float ang = G.ang + G.kick + stun * 0.035f * sinf((float)G.t * 4.3f);
    float dx = cosf(ang), dy = sinf(ang);
    float plx = -dy * PLANE_LEN, ply = dx * PLANE_LEN;
    float down = smoothstepf(0.0f, 1.0f, G.af_k);
    float horizon = H / 2.0f + G.hurt * 3.0f + down * 40.0f + stun * 3.0f * sinf((float)G.t * 6.1f);
    float camz = G.camz;
    G.camz -= 0.03f * G.hurt + 0.40f * down;
    fm_any = hz_marks();
    por_frame();                                                   /* her rooms' portraits, as they move */
    render_floor(f, dx, dy, plx, ply, horizon);
    render_walls(f, dx, dy, plx, ply, horizon);
    render_sprites(f, dx, dy, horizon);
    render_beams(f, dx, dy, horizon);
    render_wisps(f, dx, dy, horizon);
    render_marks(f, dx, dy, horizon);
    render_warded(f, dx, dy, horizon);
    G.camz = camz;
    if (down > 0.0f) af_roll(f, down);                             /* on the floor, on your side */
    if (G.web_hp > 0.0f && !G.dead && !G.shop) render_web(f);
    if (G.shop) render_shop_tags(f, dx, dy, horizon);
    if (!G.dead) { render_wards(f); render_hand(f); render_vslip(f); render_latch(f); render_af_view(f); }
    wheel_grey(f);                                                 /* a wheel up: the world grey, what is drawn after keeps its colours */
    if (map_shown()) render_map(f);                                /* the map, as the crypt goes on under it */
    if (G.shop && G.ui != UI_MERCHANT) render_shop_card(f);   /* her own card would show under her menu */
    if (G.shop || G.player) render_statusbar(f);                  /* a person gets the Wolfenstein bar, */
    render_sprint(f);
    render_sockets(f);
    render_runesockets(f);
    render_carry(f);
    render_dash(f);
    if (G.player && !G.shop && G.bar <= 0.0f) render_readout(f);   /* and the small one while it is away */
    if (!G.shop && !G.player) { render_readout(f); render_face_popup(f); }
    if (!G.dead) render_af_row(f);                                 /* what is on you (PLAN-combat, section 4.2) */
    if (!G.shop) { render_bossbar(f); render_trial(f); render_target(f); if (!(G.player && G.ui)) render_card(f); }   /* a menu covers a card */
    render_xp(f);
    render_wind(f);
    render_messages(f);
    if (G.player) wheel_render(f);
    if (G.claim_card) render_claim(f);
    if (G.player && G.ui) render_ui(f);
    if (!(G.player && G.ui)) render_toasts(f);                     /* never over a menu: they wait for it */
    tint(f);
    pad_look_render(f);
    con_render(f);
}

/* The music follows the crypt (docs/PLAN-music.md): the run's piece, her hall and its back rooms, a menu, a
 * fight that grows and the failing. Each setter does nothing when nothing has changed, so all of it is said
 * every frame, the dream's as a person's. */
static int her_screen(int ui) {                                    /* her hall's own screens keep her piece */
    return ui == UI_MERCHANT || ui == UI_LEDGER || ui == UI_LETGO || ui == UI_SHRINE || ui == UI_TOTEM || ui == UI_PORTAL ||
           ui == UI_STONE || ui == UI_COMP || ui == UI_REPO || ui == UI_ASCEND || ui == UI_STONES || ui == UI_ALIGN ||
           ui == UI_CLASS || ui == UI_HALL2 || ui == UI_PORTRAIT || ui == UI_BUY;
}
static void music_frame(float dt) {
    if (title_up()) {                                              /* the title's piece, the lament ground (1.0.0b21.1) */
        snd_set_menu(3); snd_set_fight(0); snd_set_frail(0);
        MUS.menu = 3;
        return;
    }
    int lv = G.level > 0 ? G.level : imax(1, RUN.level);
    snd_set_track(MU_CRYPTS + crawl_set(lv));
    int place = 0;                                                 /* her room (and the way on), or her back rooms */
    if (G.shop) place = (G.cx >= 10 && G.cx <= 20 && G.cy >= 12 && G.cy <= 17) || G.cx > 20 ? 1 : 2;
    snd_set_shop(RUN.perfect ? MU_PERFECT : MU_MERCHANT);          /* after a perfect level, the alley cat (docs/PLAN-music-work.md) */
#ifdef CRAWL_DEBUG
    if (place && !MUS.place) fprintf(stderr, "EVENT music hall %s t=%.2f\n", RUN.perfect ? "perfect" : "merchant", G.t);
#endif
    snd_set_place(place);
#ifdef CRAWL_DEBUG
    if (place != MUS.place) fprintf(stderr, "EVENT music place %d t=%.2f\n", place, G.t);
#endif
    MUS.place = place;
    int menu = 0;                                                  /* not the console, nor the map: the crypt runs under those */
    if (G.ui == UI_LEDGER && G.led_mode) menu = 3;                 /* a page to open, here or in her hall: the lament ground */
    else if (G.ui == UI_ALIGN || G.ui == UI_ASCEND || G.ui == UI_STONES) menu = 4;   /* the Lampstand, or ascending: the estampie */
    else if (G.ui != UI_NONE && !(G.shop && her_screen(G.ui))) menu = G.ui == UI_WISPS ? 2 : 1;
    snd_set_menu(menu);
#ifdef CRAWL_DEBUG
    if (menu != MUS.menu) fprintf(stderr, "EVENT music menu %d t=%.2f\n", menu, G.t);
#endif
    MUS.menu = menu;

    int near = 0, engaged = 0;                                     /* a fight: something that knows you, close, or a missile coming */
    int boss = G.boss >= 0 && (G.bstate == BS_FIGHT || G.bstate == BS_WAVE || G.bstate == BS_RISE);
    if (!G.shop && !G.tally && !G.dead && !PK.depth && !boss) {
        for (int i = 0; i < MAXMOBS; i++) {
            const Mob *m = &G.mobs[i];
            if (!m->alive || m->state != MS_WALK || m->ally || (KINDS[m->type].flags & KF_SPIRIT)) continue;
            if (m->engage) engaged++;
            if (m->alert > 0.0f && hypotf(m->x - G.px, m->y - G.py) < 9.0f) near++;
        }
        if (incoming(2.0f) > 0 || skull_charging(9.0f, 0.0f)) near++;
        if (G.flare_t > 0.9f) MUS.cast_t = 3.0f;                   /* casting at something in sight that hasn't noticed yet */
        if (MUS.cast_t > 0.0f && enemies_within(9.0f, 0) > 0) near++;
    }
    MUS.cast_t = fmaxf(0.0f, MUS.cast_t - dt);
    if (G.shop || PK.depth || boss) { MUS.fight = 0; MUS.fight_t = 0.0f; }
    else if (near) {                                               /* in at once; it grows with time, numbers and hurt, never back */
        int lvl = 1;
        MUS.calm_t = 0.0f; MUS.fight_t += dt;
        if (MUS.fight_t > 20.0f || engaged >= 3) lvl = 2;
        if (MUS.fight_t > 45.0f || engaged >= 5 || G.hp * 5 < G.max_hp * 2) lvl = 3;
        MUS.fight = imax(MUS.fight, lvl);
    } else if (MUS.fight && (MUS.calm_t += dt) >= 6.0f) { MUS.fight = 0; MUS.fight_t = 0.0f; }   /* out after six quiet seconds */
#ifdef CRAWL_DEBUG
    static int said_fight;
    if (MUS.fight != said_fight) fprintf(stderr, "EVENT music fight %d level %d t=%.2f engaged=%d hp=%d/%d\n", MUS.fight, G.level, G.t, engaged, G.hp, G.max_hp);
    said_fight = MUS.fight;
#endif
    snd_set_fight(MUS.fight);

    if (G.shop || G.max_hp <= 0) MUS.frail = 0;                    /* the failing: under a fifth, lifted over a quarter */
    else if (G.hp * 5 < G.max_hp) MUS.frail = 1;
    else if (G.hp * 4 > G.max_hp) MUS.frail = 0;
#ifdef CRAWL_DEBUG
    static int said_frail;
    if (MUS.frail != said_frail) fprintf(stderr, "EVENT music frail %d t=%.2f hp=%d/%d\n", MUS.frail, G.t, G.hp, G.max_hp);
    said_frail = MUS.frail;
#endif
    snd_set_frail(MUS.frail);
}
static void crawl_frame(double dt) { crawl_update(dt); music_frame((float)dt); }

/* ---- touch's stones on the stage (docs/PLAN-mobile.md, 6.5): a monument for each control in the margins, cut with the
 * graves' kit (tstone_draw in crawl_title.c), in the run's stone; what each has to say goes in the places cut for it: the
 * spell, rune or gift in hand in its socket, its time round the ring, a word or a number in the tablet with handles, a fill
 * in the groove. Pressed, a stone sinks a pixel and its words go to gold ---- */
static void stone_info_rune(TsInfo *in, int k) {                   /* a rune in a socket: its quality's tag, its time */
    int q = clampi(RUN.gear.def[k], 1, NQUAL), hush = no_cast() || rune_barred(k);
    float cd = def_cooldown(q);
    in->sock = 2; in->mark = RUNE_MARK[k]; in->ramp = gear_color(GK_DEF, k);
    in->state = hush ? SK_CLOSED : G.def_cd[k] > 0.0f ? SK_PART : SK_READY;
    in->ring = cd > 0.0f ? 1.0f - clampf(G.def_cd[k] / cd, 0.0f, 1.0f) : 1.0f; in->ring_ramp = in->ramp;
    in->text = Q_TAG[q - 1]; in->text_col = qual_text(q - 1);
}
static void crawl_stage(Frame *st, int px, int py) {
    (void)px; (void)py;
    if (!IN.touch || !IN.stage_w || CON.open) return;
    TRect R[NTB];
    touch_geo(R);
    int ctx = touch_ctx(), run = crawl_set(imax(1, G.level));
    int rr = rune_readied(), gr = ab_readied();
    int rmark = RUN.mouse_rune && rr >= 0;                         /* the top right uses what was readied last */
    static char num[3][16];
    static const char *const MENU_W[NTB] = { "ENTER", "ESC", "BACK", "SPACE", "", "", "TAB" };
    static const int MENU_M[NTB] = { TSM_ENTER, TSM_ESC, TSM_BACK, TSM_SPACE, TSM_NONE, TSM_NONE, TSM_ROBE };
    for (int k = 0; k < NTB; k++) {
        if (!touch_shown(k, ctx) || R[k].w < 20 || R[k].h < 12) continue;
        static const int KIND[NTB] = { TSK_USE, TSK_CAST, TSK_CORNER, TSK_CORNER, TSK_TABLET, TSK_TABLET, TSK_TABLET };
        static const int RIGHT[NTB] = { 0, 1, 0, 1, 0, 0, 1 };
        TStone s = { KIND[k], R[k].w, R[k].h, RIGHT[k], 0, TCH.press[k], TSM_NONE, (unsigned)(k * 101 + run * 17), NULL, NULL, 0 };
        TsInfo in;
        memset(&in, 0, sizeof in); in.ring = in.fill = -1.0f;
        int y = R[k].y;
        if (s.h > 200) { y += s.h - 200; s.h = 200; }                  /* a tall stage: the stone at its foot */
        if (ctx != TC_PLAY) {                                      /* a menu, the title or a wheel: the stones are the keys they press */
            s.word = MENU_W[k]; s.mark = MENU_M[k];
            if (k == TB_STRIP_L) { in.text = ctx == TC_TITLE ? "CHOOSE" : "ACCEPT"; in.text_col = R_BONE + 14; }
            if (k == TB_STRIP_R) { in.text = ctx == TC_WHEEL ? "AWAY" : "BACK"; in.text_col = R_BONE + 14; }
            if (k == TB_WARD) { s.word = "TAB"; }
            tstone_draw(st, R[k].x, y, &s, &in, G.ui_t);
            continue;
        }
        switch (k) {
        case TB_STRIP_L: {                                         /* E: what it would do now; the sprint's breath in the groove */
            s.word = "USE"; s.mark = TSM_HAND;
            const char *v = use_word();
            if (v) { in.text = v; in.text_col = R_GOLD + 12; }
            float mx = fmaxf(0.1f, sprint_max());
            in.fill = clampf(G.spr_set ? G.spr / mx : 1.0f, 0.0f, 1.0f);
            in.fill_ramp = G.spr_on ? R_GOLD : G.spr_wait > 0.0f && !G.spr_on ? R_STONE : R_BONE;
            break;
        }
        case TB_STRIP_R: {                                         /* the spell in hand, its time, its price; a charge in the groove */
            s.word = "CAST";
            int sp = G.spell, hush = no_cast();
            if (G.rst_sel) { in.sock = 2; in.mark = RESTORE_MARK; in.ramp = R_GHOST; in.state = hush ? SK_CLOSED : SK_READY; in.ring_ramp = R_GHOST; }
            else if (sp >= 0 && sp < NATK) {
                int el = ATK_ELEM[sp];
                in.ramp = el >= 0 ? ELEM_RAMP[el] : R_PURPLE;
                if (el >= 0) { in.sock = 1; in.icon = UI_NODE_ICON[el][ND_OPEN]; } else { in.sock = 2; in.mark = BFS_MARK; }
                float cyc = fmaxf(0.05f, cast_cycle(sp));
                in.state = hush ? SK_CLOSED : SK_READY;
                in.ring = 1.0f - clampf(G.cooldown / cyc, 0.0f, 1.0f); in.ring_ramp = in.ramp;
                if (sp == AT_SPEAR) {                              /* bones, or splinters when there are none */
                    snprintf(num[0], sizeof num[0], "%d", spear_bare() ? splint_left() : RUN.gear.bones);
                    in.text = num[0]; in.text_col = spear_bare() ? R_STONE + 12 : R_BONE + 14;
                    if (spear_bare()) { in.fill = (float)splint_left() / SPLINTER_MAX; in.fill_ramp = R_BONE; }
                } else if (sp != AT_BFS) {
                    float c = spell_cost(&RUN.gear, sp, spell_tier(sp)) * (1.0f + aug_cost(sp));
                    if (c < 0.05f) { in.text = "FREE"; in.text_col = R_BONE + 14; }
                    else {
                        snprintf(num[0], sizeof num[0], c < 10.0f ? "%.1f" : "%.0f", c);
                        in.text = num[0]; in.glyph = TS_GLYPH_SOUL; in.glyph_ramp = R_GHOST;
                        in.text_col = spirit() >= c ? R_GHOST + 13 : R_BLOOD + 12;
                    }
                }
                if (G.chg_on) { in.fill = clampf(G.chg_t / hold_span(G.chg_sp), 0.0f, 1.0f); in.fill_ramp = R_GOLD; in.ring = 1.0f; in.ring_ramp = R_GOLD; }
            }
            break;
        }
        case TB_CORNER_L:                                          /* the rune readied; held, the pause */
            s.word = "RUNE"; s.foot = "PAUSE"; s.foot_mark = 1;
            if (rr >= 0) stone_info_rune(&in, rr);
            else { in.text = "NONE"; in.text_col = R_STONE + 10; }
            break;
        case TB_CORNER_R:                                          /* the right mouse's: a gift, or a rune; held, the wheels */
            s.word = rmark ? "RUNE" : "GIFT"; s.foot = "WHEELS"; s.foot_mark = 2;
            if (rmark) stone_info_rune(&in, rr);
            else if (gr >= 0) {
                float wait = ab_wait(gr);
                in.sock = 2; in.mark = BA_MARK[gr]; in.ramp = BA_RAMP[gr];
                in.state = no_cast() ? SK_CLOSED : G.ab_cd[gr] > 0.0f ? SK_PART : SK_READY;
                in.ring = wait > 0.0f ? 1.0f - clampf(G.ab_cd[gr] / wait, 0.0f, 1.0f) : 1.0f; in.ring_ramp = in.ramp;
                in.pips = RUN.abil[gr]; in.maxpips = ABIL_TOP; in.pip_ramp = R_GOLD;
            } else { in.text = "NONE"; in.text_col = R_STONE + 10; }
            break;
        case TB_WISPS:                                             /* a soul a wisp: the souls held */
            s.word = "WISPS"; s.mark = TSM_WISP;
            spirit_text(num[1], sizeof num[1]);
            in.text = num[1]; in.glyph = TS_GLYPH_SOUL; in.glyph_ramp = R_GHOST;
            in.text_col = spirit() >= WISP_COST ? R_GHOST + 13 : R_BLOOD + 12;
            break;
        case TB_MAP:                                               /* the level's secrets found; worn blank till it is bought */
            s.word = "MAP"; s.mark = TSM_SCROLL; s.blank = !RUN.has_map;
            if (!G.shop && G.nlinks > 0) {
                snprintf(num[2], sizeof num[2], "%d/%d", imin(G.links_found, G.nlinks), G.nlinks);
                in.text = num[2]; in.glyph = TS_GLYPH_SECRET; in.glyph_ramp = R_BONE;
                in.text_col = G.links_found >= G.nlinks ? R_GOLD + 12 : R_BONE + 14;
            }
            break;
        case TB_WARD: {                                            /* the bag, how full */
            s.word = "WARDROBE"; s.mark = TSM_ROBE;
            if (!tstone_word_fits(&s)) s.word = "GEAR";
            int n = RUN.gear.bag_size;
            if (n > 0) {
                static char bag[24];
                int used = n - gear_bag_free(&RUN.gear);
                snprintf(bag, sizeof bag, "%d/%d", used, n);
                in.text = bag; in.glyph = TS_GLYPH_BAG; in.glyph_ramp = R_BONE;
                in.text_col = used >= n ? R_BLOOD + 12 : R_BONE + 14;
            }
            break;
        }
        }
        tstone_draw(st, R[k].x, y, &s, &in, G.ui_t);
    }
    if (ctx == TC_PLAY && TCH.walk) tstone_ring(st, TCH.wbx, TCH.wby, TCH.wx, TCH.wy, touch_ring(), G.spr_on);
}

const Scene scene_crawl = { "CATACOMB", W, H, SND_PROG_CRAWL, crawl_enter, crawl_frame, crawl_render,
                            crawl_control, crawl_status, crawl_input, crawl_stage };   /* the stage: touch's stones (docs/PLAN-mobile.md) */
