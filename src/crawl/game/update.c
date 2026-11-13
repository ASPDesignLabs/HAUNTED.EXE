/* A part of scene_crawl.c (docs/PLAN-tidy.md): it is #included there in its place and never compiled on its own.
 * The debug build's stages (CRAWL_MEET, CRAWL_PACK, CRAWL_STAGE), and crawl_update: the frame's order of things. */

/* one of a kind a few cells ahead, in view (the console, CRAWL_MEET); n of them for a test (CRAWL_MEETN), spread
 * about the line of sight */
static int meet_creature(int k, int n) {
    int far = 0;
    for (int d = 1; d <= 7; d++) {                                 /* the farthest open cell in view, up to seven */
        int mx = (int)(G.px + cosf(G.ang) * d), my = (int)(G.py + sinf(G.ang) * d);
        if (G.map[my][mx].kind != K_FLOOR || !line_clear(G.px, G.py, mx + 0.5f, my + 0.5f)) break;
        if (d >= 3 && !occupied(mx, my, NULL)) far = d;
    }
    if (!far) return 0;
    int made = 0;
    for (int j = 0; j < n * 3 && made < n; j++) {
        float d = (float)far - (float)(j / 3) * 0.9f, side = (float)((j % 3) == 1 ? 1 : (j % 3) == 2 ? -1 : 0);
        int mx = (int)(G.px + cosf(G.ang) * d - sinf(G.ang) * side), my = (int)(G.py + sinf(G.ang) * d + cosf(G.ang) * side);
        if (d < 2.0f || G.map[my][mx].kind != K_FLOOR || occupied(mx, my, NULL)) continue;
        Mob *m = spawn_mob(mx, my, k);
        if (m && k == MOB_HUNTER) m->sub = 0;
        made += m != NULL;
    }
    return made > 0;
}

#ifdef CRAWL_DEBUG
/* CRAWL_PACK=k: a pack a few cells ahead, five seconds in: 1 a file of the Order, 2 hounds, 3 a roost asleep, 4 a wake, 5 the
 * Whipper-In and his hounds */
static int pack_meet(int k) {
    static const int KIND[6] = { 0, PACK_FILE, PACK_HOUNDS, PACK_ROOST, PACK_WAKE, PACK_WHIP };
    if (k < 1 || k > 5) return 1;
    int far = 0;
    for (int d = 1; d <= 7; d++) {
        int mx = (int)(G.px + cosf(G.ang) * d), my = (int)(G.py + sinf(G.ang) * d);
        if (G.map[my][mx].kind != K_FLOOR || !line_clear(G.px, G.py, mx + 0.5f, my + 0.5f)) break;
        if (d >= 3 && !occupied(mx, my, NULL)) far = d;
    }
    if (!far) return 0;
    int x = (int)(G.px + cosf(G.ang) * far), y = (int)(G.py + sinf(G.ang) * far), pk = pack_new(KIND[k]);
    if (pk < 0) return 1;
    int n = k == 1 ? 4 : k == 2 ? 3 : 4;
    for (int i = 0; i < n; i++) {
        int mx = x, my = y;
        if (i && !free_near(x, y, &mx, &my)) break;
        int type = k == 1 ? (i == 0 ? MOB_SHIELD : MOB_SKEL) : k == 2 ? MOB_HOUND : k == 3 ? MOB_BAT : k == 5 ? (i == 0 ? MOB_WHIPPER : MOB_HOUND) : MOB_SHAMBLER;
        Mob *m = k == 4 ? riser_make(type, mx, my, -1) : spawn_mob(mx, my, type);
        if (!m) break;
        if (k == 3) { m->sleep = 1; m->home = G.map[my][mx].region; }
        pack_join(pk, m);
    }
    fprintf(stderr, "PACKS meet %s of %d at (%d,%d) t=%.2f\n", PACK_NAME[KIND[k]], G.packs[pk].n0, x, y, G.t);
    return 1;
}

static void stage_set(int k) {                                     /* CRAWL_STAGE (see g_stage) */
    static const int DX[4] = { 1, 0, -1, 0 }, DY[4] = { 0, 1, 0, -1 };
    int bx = -1, by = -1, bd = 0, blen = 0;
    for (int y = 1; y < MAPN - 1; y++)
        for (int x = 1; x < MAPN - 1; x++) {
            if (G.map[y][x].kind != K_FLOOR) continue;
            for (int d = 0; d < 4; d++) {
                int n = 0;
                for (;;) {
                    int cx = x + DX[d] * (n + 1), cy = y + DY[d] * (n + 1);
                    if (n >= 24 || cx < 0 || cy < 0 || cx >= MAPN || cy >= MAPN || G.map[cy][cx].kind != K_FLOOR) break;
                    n++;
                }
                if (n > blen) { blen = n; bx = x; by = y; bd = d; }
            }
        }
    if (bx < 0) return;
    G.px = bx + 0.5f; G.py = by + 0.5f; G.ang = atan2f((float)DY[bd], (float)DX[bd]);
    for (int i = 0; i < MAXMOBS; i++) G.mobs[i].alive = 0;
    float D[7] = { 2.5f, 4.0f, 6.0f, 9.0f, 13.0f, 18.0f, 23.0f };
    int nd = 7;
    if (getenv("CRAWL_STAGEDIST")) { D[0] = (float)atof(getenv("CRAWL_STAGEDIST")); nd = 1; }   /* just one, at that distance */
    for (int i = 0; i < nd && D[i] <= blen + 0.4f; i++) {
        float x = G.px + DX[bd] * D[i], y = G.py + DY[bd] * D[i];
        Mob *m = spawn_mob((int)x, (int)y, k);
        if (!m) break;
        m->x = x; m->y = y; m->face = G.ang + PI_F; m->t = 0.0f;
        if (getenv("CRAWL_STAGEFACE")) m->face += (float)atof(getenv("CRAWL_STAGEFACE")) * PI_F / 180.0f;   /* turned (1.0.0b15): its side */
        if (k == MOB_GARGOYLE) m->sub = 2;
        if (k == MOB_HUNTER) m->sub = 2;
        if (getenv("CRAWL_STAGESUB")) m->sub = atoi(getenv("CRAWL_STAGESUB"));
        if (getenv("CRAWL_STAGEBOSS")) m->boss = (uint8_t)(1 + atoi(getenv("CRAWL_STAGEBOSS")));   /* drawn as that boss, for its frames */
        if (k == MOB_BAT && getenv("CRAWL_STAGESLEEP")) { m->sleep = 1; m->z = 1.0f - 0.64f * KINDS[k].scale * RANK_SIZE[m->rank]; }   /* a roost (1.0.0b18) */
        if (k == MOB_HUNTER && m->sub == 0)                         /* on the ceiling, as it hangs there */
            m->z = 1.0f - KINDS[k].scale * (m->boss ? BOSSES[m->boss - 1].scale : RANK_SIZE[m->rank]);
    }
    if (getenv("CRAWL_STAGEREAR")) G.web_rear = 1e6f;              /* the Wandering Spider rearing to spit, held */
    if (getenv("CRAWL_STAGECHANT"))                                /* a deacon chanting, held */
        for (int i = 0; i < MAXMOBS; i++) if (G.mobs[i].alive) G.mobs[i].swing = DEACON_CHANT * 0.5f;
    if (getenv("CRAWL_STAGEDARK")) g_stage_lm = (float)atof(getenv("CRAWL_STAGEDARK"));
    g_stage_fog = getenv("CRAWL_STAGEFOG") != NULL;
    fprintf(stderr, "EVENT stage kind=%d hall=%d at %.1f,%.1f\n", k, blen, G.px, G.py);
}
#endif

static int con_update(float dt);
static void challenge_begin(int b);
#ifdef CRAWL_DEBUG
static void tp_room(int k);
static void tp_prop(int spr);
static int deco_look(const Deco *dc);
#endif
static void update_mark(float dt);
static void comp_note(int level);
static void crawl_update(double dt) {
    float fdt = (float)dt;
    if (title_tick(fdt)) return;                                   /* the title holds the screen: the crypt waits behind it */
    foot_settle();                                                 /* one born by a prop, into what it leaves free (1.0.0b18.1) */
#ifdef CRAWL_DEBUG
    dbg_souls(); foot_watch();
#endif
    if (con_update(fdt)) return;                                   /* the console is down: the crypt waits, the music plays */
    if (pad_look_tick(fdt)) return;                                /* and so it does under the controller's look (1.0.0b18.1) */
    if (RUN.active) comp_note(G.level);                            /* what is worn and known goes into the Compendium */
    caster_sync();                                                 /* and the caster's own joins what is worn */
    G.msg_t = fmaxf(0.0f, G.msg_t - fdt); G.hint_t = fmaxf(0.0f, G.hint_t - fdt); G.knock_t = fmaxf(0.0f, G.knock_t - fdt);
    G.ui_t += fdt;
    if (!(G.player && G.ui == UI_CLASS)) update_toasts(fdt);      /* the news waits while she asks */
    if (!(G.player && G.ui)) {                                     /* the experience shown: it waits with a menu */
        for (int i = 0; i < XPF.n; i++) XPF.t[i] += fdt;
        while (XPF.n > 0 && XPF.t[XPF.n - 1] > 3.0f) XPF.n--;
        XPF.banner = fmaxf(0.0f, XPF.banner - fdt);
    }
    if (G.player && G.ui && !G.shop) { update_ui(); return; }     /* a menu is up: the crypt holds its breath */
    if (G.player && !G.ui && !G.dead && !G.tally && !G.exiting) wheel_input(fdt);   /* a wheel: real time for it (her rooms too,
                                                                      1.0.0b18.1, but never over one of her menus), */
    else if (WH.open) wheel_close(0);
    WH.k = WH.open ? fminf(1.0f, WH.k + fdt / 0.12f) : fmaxf(0.0f, WH.k - fdt / 0.12f);
    if (WH.open) { dt *= WHEEL_SLOW; fdt = (float)dt; }            /* and the world slow under it */
    if (G.player && (G.tally || G.dead) && IN.hit[KEY_ESC]) { open_ui(UI_QUIT); return; }
    if (G.player && G.dead && G.dcost && G.dcard_t > 0.8f && (IN.hit[KEY_E] || IN.hit[KEY_ENTER] || IN.hit[KEY_SPACE] || IN.hit[KEY_MOUSE1])) G.dcard_done = 1;
    if (G.player && (G.tally || G.dead) && IN.hit[KEY_B]) { open_ui(UI_BESTIARY); return; }
    if (G.player && (G.tally || G.dead) && IN.hit[KEY_C]) { open_ui(UI_CODEX); return; }
    G.t += dt; G.level_t += dt; G.card += fdt;
#ifdef CRAWL_DEBUG
    if (getenv("CRAWL_HOLDCARD") && G.card > 1.0f && G.card < 90.0f) G.card = 1.0f;   /* the level's card held up, for the shots */
#endif
    if (G.shop || G.dead || G.tally) G.bar_t = BAR_T;               /* always up at the merchant's, and at the end */
    else G.bar_t = fmaxf(0.0f, G.bar_t - fdt);
    G.bar = G.bar_t > 0.0f ? fminf(1.0f, G.bar + fdt / BAR_SLIDE) : fmaxf(0.0f, G.bar - fdt / BAR_SLIDE);
    G.peek = fmaxf(0.0f, G.peek - fdt);                            /* the readout rises over the HOLD H plate, then settles */
    G.lift += ((G.peek > 0.0f ? 1.0f : 0.0f) - G.lift) * fminf(1.0f, fdt * 8.0f);
    update_mark(fdt);
    if (G.claim_card) {                                            /* her card: nothing moves until it is read */
        G.claim_t += fdt;
        if ((G.player && G.claim_t > 1.0f && (IN.hit[KEY_E] || IN.hit[KEY_ENTER] || IN.hit[KEY_SPACE] || IN.hit[KEY_MOUSE1])) || (!G.player && G.claim_t > 9.0f)) {
            G.claim_card = 0; G.click_spent = CAST_HIT;
        }
        if (G.player) return;
    }
    if (G.shop) { update_shop(dt); return; }
    if (G.tally) {                                                 /* the level-complete screen; the crypt waits */
        update_tally(dt);
        if (G.tally) G.fade = smoothstepf(0.0f, 0.4f, (float)G.tally_t) * (1.0f - smoothstepf(TALLY_END - 0.7f, TALLY_END, (float)G.tally_t));
        return;
    }
    ward_tick(fdt);                                                /* the soul-lamps (1.0.0b18.1) */
    float ox = G.px, oy = G.py;
    int still = 0;
#ifdef CRAWL_DEBUG
    still = g_stage >= 0 && !G.player;                            /* CRAWL_STAGE: the caster stands and looks (a person may act) */
#endif
    if (G.level_t > 1.0 && !G.dead && !still) { if (G.player && !G.exiting) { if (!WH.open) update_human(fdt); } else update_player(fdt); }
    if (fdt > 0.0f) { G.pvx = (G.px - ox) / fdt; G.pvy = (G.py - oy) / fdt; }
    build_hunt(fdt);
    pick_hunters();
    minds_frame(fdt);                                              /* packs, and how many may be in the fight (docs/PLAN-ai.md, step 3) */
#ifdef CRAWL_DEBUG
    if (getenv("CRAWL_KILLAT") && !G.dead && G.t >= atof(getenv("CRAWL_KILLAT")) && G.t - dt < atof(getenv("CRAWL_KILLAT"))) {   /* testing: struck down */
        G.level_t = fmax(G.level_t, 2.0); G.blame = (Blame){ BL_NONE, 0, 0 }; hurt_ex(99999.0f, HURT_SPELL, G.px, G.py);
    }
    static int stage_level = -1;                                   /* CRAWL_STAGE: a kind stood down a hall */
    if (getenv("CRAWL_STAGE") && !G.shop && !PK.depth && stage_level != G.level && G.level_t > 0.3) {
        g_stage = clampi(atoi(getenv("CRAWL_STAGE")), 0, NKINDS - 1); stage_level = G.level;
        if (getenv("CRAWL_PROPROOM") || getenv("CRAWL_TPPROP") || getenv("CRAWL_TPITEM")) {   /* for the props: no one about, and you in a room of that type, or before one */
            for (int i = 0; i < MAXMOBS; i++) G.mobs[i].alive = 0;
            if (getenv("CRAWL_TPPROP") || getenv("CRAWL_TPITEM")) tp_prop(getenv("CRAWL_TPPROP") ? atoi(getenv("CRAWL_TPPROP")) : -1); else tp_room(atoi(getenv("CRAWL_PROPROOM")));
        } else stage_set(g_stage);
    }
    if (g_stage >= 0) for (int i = 0; i < MAXMOBS; i++) if (G.mobs[i].alive && G.mobs[i].type != g_stage) G.mobs[i].alive = 0;
    static int meet_level = -1;                                    /* one of a kind, ahead, once the level's card has gone (and in each pocket) */
    if (getenv("CRAWL_MEET") && meet_level != G.level * 16 + PK.depth && G.level_t > 5.0 && !G.dead
        && meet_creature(clampi(atoi(getenv("CRAWL_MEET")), 0, NKINDS - 1), getenv("CRAWL_MEETN") ? atoi(getenv("CRAWL_MEETN")) : 1)) meet_level = G.level * 16 + PK.depth;
    static double ktrace_t;                                        /* CRAWL_KINDTRACE=k: each of kind k once a second */
    if (getenv("CRAWL_KINDTRACE") && G.t - ktrace_t >= 1.0) {
        ktrace_t = G.t;
        int k = atoi(getenv("CRAWL_KINDTRACE"));
        for (int i = 0; i < MAXMOBS; i++) {
            const Mob *m = &G.mobs[i];
            if (!m->alive || m->type != k) continue;
            if (m->route >= 0 && m->route < G.nroute) { int w = m->wp % G.route[m->route].n; fprintf(stderr, "KIND waypoint (%d,%d) occupied %d h %d\n", G.route[m->route].x[w], G.route[m->route].y[w], occupied(G.route[m->route].x[w], G.route[m->route].y[w], m), mind_field(G.route[m->route].x[w], G.route[m->route].y[w])[m->cy][m->cx]); }
            fprintf(stderr, "KIND %s #%d at (%d,%d) %s aware %d alert %.1f role %d route %d wp %d wait %.1f hp %.0f/%.0f pack %d ring %.1f/%d ice %.1f wild %d dist %.1f t=%.1f\n",
                    KINDS[k].name, i, m->cx, m->cy, m->state == MS_WALK ? "walking" : "dying", m->aware, m->alert, m->role, m->route, m->wp, m->wait,
                    m->hp, m->maxhp, m->pack, m->ring_t, m->rings, m->ice_t, m->wild, hypotf(m->x - G.px, m->y - G.py), G.level_t);
        }
    }
    static int pack_level = -1;                                    /* a pack ahead, five seconds into each level (and pocket) */
    if (getenv("CRAWL_PACK") && pack_level != G.level * 16 + PK.depth && G.level_t > 5.0 && !G.dead && pack_meet(atoi(getenv("CRAWL_PACK")))) pack_level = G.level * 16 + PK.depth;
    static int latch_level = -1;                                   /* a rank-II bat latched five seconds into each level (and pocket) */
    if (getenv("CRAWL_LATCH") && latch_level != G.level * 16 + PK.depth && G.level_t > 5.0 && !G.dead && !G.latch) {
        Mob *b = spawn_mob(G.cx, G.cy, MOB_BAT);
        if (b) { set_rank(b, RK_II); b->contact = LATCH_T[RK_II]; b->attacking = 1; latch_begin(b); latch_level = G.level * 16 + PK.depth; }
    }
#endif
    for (int i = 0; i < MAXMOBS; i++) if (G.mobs[i].alive) { mob_afflict(&G.mobs[i], fdt); if (G.mobs[i].alive) update_mob(&G.mobs[i], fdt); }
    if (G.spec_n > 0 && !G.tally && !G.exiting && !G.dead && (G.spec_t -= fdt) <= 0.0f) {   /* a spectre drifts in */
        int x, y;
        G.spec_t = 5.0f;
        Mob *sp = pick_cell_at_depth(G.cx, G.cy, 6, 16, 1, &x, &y) ? spawn_mob(x, y, MOB_GHOST) : NULL;
        if (sp) {
            for (int k = 0; k < G.namb; k++)                       /* now and then it drifts to a set piece's door first (L3) */
                if (!G.amb[k].sprung && rnd(2)) { sp->sub = 7; sp->bx = G.amb[k].ox + 0.5f; sp->by = G.amb[k].oy + 0.5f; break; }
            G.spec_n--; G.spec_t = 40.0f + 50.0f * rndf();
#ifdef CRAWL_DEBUG
            fprintf(stderr, "EVENT spectre comes, %d to come t=%.1f\n", G.spec_n, G.level_t);
#endif
        }
    }
    update_pools(fdt);
    /* the Wraith comes, once, after about four minutes on a level, from the far end of it */
    float wraith_at = 240.0f;
#ifdef CRAWL_DEBUG
    if (getenv("CRAWL_WRAITH")) wraith_at = (float)atof(getenv("CRAWL_WRAITH"));
#endif
    if (!G.wraith_done && !PK.depth && (G.wraith_now || ((G.level >= KINDS[MOB_WRAITH].debut || crawl_loop(G.level) > 0) && G.level_t > wraith_at && !crawl_boss(G.level)))
        && !G.tally && !G.exiting && (G.wraith_try -= fdt) <= 0.0f) {
        int x, y;
        G.wraith_try = 5.0f;
        if (pick_cell_at_depth(G.cx, G.cy, 20, 400, 1, &x, &y) && !wraith_barred(x, y)) {   /* (never inside a soul-lamp's ward) */
            Mob *m = spawn_mob(x, y, MOB_WRAITH);
            if (m) { m->alert = 1e9f; G.wraith_done = 1; G.wraith_now = 0; sfx_at(SFX_WAIL, x + 0.5f, y + 0.5f, 3.0f); wraith_chill(m); }
#ifdef CRAWL_DEBUG
            fprintf(stderr, "EVENT wraith comes t=%.1f\n", G.level_t);
#endif
        }
    }
    update_shots(fdt);
    mk_tick(fdt);
    update_bones(fdt);
    update_doors(fdt);
    update_slide(fdt);
    update_wisps(fdt);
#ifdef CRAWL_DEBUG
    static int wisp_level = -1;                                    /* testing: an exit wisp, free, as the level opens */
    if (getenv("CRAWL_BLESS") && !G.shop) G.blessed = 1;          /* testing: the altar's blessing on every level */
    if (getenv("CRAWL_WISP") && wisp_level != G.level && G.level_t > 1.5 && !G.dead) {
        int c = atoi(getenv("CRAWL_WISP"));
        summon_wisp(c >= 2 ? WK_KEY : WK_EXIT, c >= 2 ? (c - 2) % NKEYS : 0, 0.0f);
        wisp_level = G.level;
    }
#endif
    if (!G.dead) { map_note(); map_follow(fdt); xp_rooms(); update_items(); update_abilities(fdt); ab_update(fdt); update_altar(); update_status(fdt); if (G.level_t > 1.0) update_defense(fdt); }
    update_boss(fdt);
    update_hazards(fdt);
    af_tick(fdt);                                                  /* what is on the caster (PLAN-combat, section 4) */
    update_sites(fdt);
    update_things(fdt);
    pz_update(fdt);
    update_search(fdt);
    update_bshrine_t(fdt);
    update_trial(fdt);
    update_omens(fdt);
    update_arising(fdt);
    if (G.bfs_t > 0.0f && (G.bfs_t += fdt) > 1.6f) G.bfs_t = 0.0f;
    G.hurt = fmaxf(0.0f, G.hurt - fdt / 0.35f);
    G.gold = fmaxf(0.0f, G.gold - fdt / 0.45f);
    G.chill = fmaxf(0.0f, G.chill - fdt / 0.6f);
    G.wind_t = fmaxf(0.0f, G.wind_t - fdt);
    G.face_t = fmaxf(0.0f, G.face_t - fdt);
    G.grin_t = fmaxf(0.0f, G.grin_t - fdt);
    G.kick *= expf(-fdt * 9.0f);
    if (G.readout > 0.0f) G.readout -= fdt;
    if (G.flare_t > 0.0f) G.flare_t -= fdt;
    mind_frame();
    if (!G.dead && !G.shop && G.level_t > 0.0) {                  /* the trail (section 11): water keeps none */
        int sx = (int)G.px, sy = (int)G.py;
        if (sx > 0 && sy > 0 && sx < MAPN - 1 && sy < MAPN - 1 && !on_water(sx, sy)) G.scent[sy][sx] = (float)G.level_t + 0.01f;
    }
    G.still_t = G.pvx * G.pvx + G.pvy * G.pvy < 0.09f ? G.still_t + fdt : 0.0f;
    rest_tick(fdt);
    lull_tick(fdt);                                                /* a person's long quiet walk: something offered (1.0.0b17) */
    if (G.hp <= G.max_hp / 4 && !G.dead && (G.heartbeat -= fdt) <= 0.0f) { snd_sfx_ex(SFX_HEART, 0.9f, 0.5f); G.heartbeat = 1.1f; }

    name_newcomers();

    /* keep the action coming: if nothing has happened for a while, wake something up ahead */
    if (G.calm > (G.player ? 35.0f : 11.0f) && !PK.depth && !g_drawn) {   /* a person clears a level for good, mostly; a drawn
                                                                      level holds what was drawn */
        int x, y, alive = 0;
        for (int i = 0; i < MAXMOBS; i++) { const Mob *m = &G.mobs[i]; alive += m->alive && !m->guard && !m->ally && !KIND_IS(m, KF_UNCOUNTED) && m->type != MOB_WSKULL && m->type != MOB_SPIDER && m->type != MOB_GARGOYLE && m->type != MOB_LING; }
        int old_calm = !mind_on();
#ifdef CRAWL_DEBUG
        if (getenv("CRAWL_OLDCALM")) old_calm = 1;                 /* the calm as it was, for comparing */
#endif
        if (old_calm) { if (alive < 8 && pick_cell_at_depth(G.cx, G.cy, 4, 8, 1, &x, &y)) spawn_wanderer(x, y); }
        else if (!G.player && alive < 8 && pick_cell_at_depth(G.cx, G.cy, 16, 30, 1, &x, &y)) {   /* H2: for a person the hunt stands in
                                                                      for this; the dream's comes from beyond sixteen steps and walks in */
            uint64_t was = 0;
            for (int i = 0; i < MAXMOBS; i++) was |= (uint64_t)(G.mobs[i].alive != 0) << i;
            spawn_wanderer(x, y);
            for (int i = 0; i < MAXMOBS; i++) if (G.mobs[i].alive && !(was >> i & 1) && minded(&G.mobs[i])) mind_track(&G.mobs[i], G.px, G.py);
        }
        G.calm = 4.0f;
    }

    G.mood_t -= fdt;
    if (G.mood_t <= 0.0f) {                                        /* torch crackle follows nearby torches */
        float m = 0.0f;
        for (int l = 0; l < G.nlights; l++) {
            float d = hypotf(G.lights[l].x - G.px, G.lights[l].y - G.py);
            if (d < 5.0f) m += 1.2f / (1.0f + d * d);
        }
        snd_set_mood(clampf(m, 0.0f, 1.0f));
        G.mood_t = 0.15f;
    }

#ifdef CRAWL_DEBUG
    if (getenv("CRAWL_MAPAT") && G.level_t >= atof(getenv("CRAWL_MAPAT")) && G.level_t - dt < atof(getenv("CRAWL_MAPAT"))) debug_map(1);
    {
        static double next;
        if (G.t >= next) {
            next = G.t + 1.0;
            int alive = 0;
            for (int i = 0; i < MAXMOBS; i++) alive += G.mobs[i].alive;
            fprintf(stderr, "lvl=%d t=%5.1f state=%d cell=(%d,%d)->(%d,%d) h=%d next=%d spd=%.2f mobs=%d explored=%d/%d hp=%d gold=%d items=%d/%d key=%d vault=%d dead=%d souls=%d val=%d/%d\n",
                    G.level, G.t, G.state, G.cx, G.cy, G.tx, G.ty, G.h, G.next_h, G.spd, alive, G.explored, G.ncells,
                    G.hp, RUN.treasure, G.items_got, G.items_total, G.has_key[0] | G.has_key[1] << 1 | G.has_key[2] << 2,
                    G.nvaults ? G.doors[G.vault_door[0]].locked : -1, G.dead, RUN.souls, G.value_got, G.value_total);
            fprintf(stderr, "   throws=%d bonehits=%d bathits=%d firehits=%d swordhits=%d bites=%d dodges=%d exit=(%d,%d) seen=%d leaving=%d kills=%d/%d wardhits=%d cracked=%d\n",
                    throws, hits_bone, hits_bat, hits_fire, hits_sword, bites, dodges, G.exit_fx, G.exit_fy, G.exit_seen, G.leaving, G.kills, G.kills_total, ward_hits, wards_cracked);
            if (mind_on()) {                                       /* how the dead think, counted */
                int aw[5] = { 0 }, eng = 0, held = 0, asleep = 0;
                for (int i = 0; i < MAXMOBS; i++) {
                    const Mob *o = &G.mobs[i];
                    if (!minded(o)) continue;
                    aw[clampi(o->aware, 0, 4)]++; eng += o->engage; held += o->hold != 0; asleep += o->sleep != 0;
                }
                int trail = 0;
                for (int i = 0; i < MAXMOBS; i++) trail += G.mobs[i].alive && G.mobs[i].hunter >= 2;
                if (getenv("CRAWL_HUNTTRACE"))                     /* each hunter: where, how it knows you, the scent under it */
                    for (int i = 0; i < MAXMOBS; i++) {
                        const Mob *o = &G.mobs[i];
                        if (o->alive && o->type == MOB_SHADE && o->state == MS_WALK)
                            fprintf(stderr, "   SHADE at %.1f cells aw%d alert%.1f hold%d shy%.1f hidden%d still%.1f strike%d\n", hypotf(o->x - G.px, o->y - G.py),
                                    o->aware, o->alert, o->hold, o->shy, shade_hidden(o), G.still_t, shade_may_strike(o));
                        if (!o->alive || !o->hunter) continue;
                        float sc = G.scent[o->cy][o->cx];
                        int16_t (*pf)[MAPN] = mind_field(G.hunt_nx, G.hunt_ny);
                        fprintf(stderr, "   HUNTER %s h%d at (%d,%d) aw%d alert%.1f scent %.0f s old, caster (%d,%d) %d steps, pick-up (%d,%d) %d steps, role %d wait %.1f\n",
                                KINDS[o->type].name, o->hunter, o->cx, o->cy, o->aware, o->alert, sc > 0.0f ? (float)G.level_t - sc : -1.0f, (int)G.px, (int)G.py,
                                hunt[o->cy][o->cx], G.hunt_nx, G.hunt_ny, pf[o->cy][o->cx], o->role, o->wait);
                    }
                fprintf(stderr, "   minds calm=%d wary=%d hunting=%d tracking=%d searching=%d engaged=%d/%d held=%d asleep=%d packs=%d hunt=%d/%d/%d\n",
                        aw[MA_CALM], aw[MA_WARY], aw[MA_HUNT], aw[MA_TRACK], aw[MA_SEARCH], eng, engage_cap(), held, asleep, G.npacks, G.hunt_kind, G.hunt_on, trail);
            }
            {                                                      /* the dream stood within a cell of one place 40 s: what is about */
                static float sx, sy; static double st0; static int told, slvl;
                if (G.level != slvl || hypotf(G.px - sx, G.py - sy) > 1.0f || G.player) { sx = G.px; sy = G.py; st0 = G.t; told = 0; slvl = G.level; }
                else if (G.t - st0 > 40.0 + 80.0 * told && told < 2 && !PK.depth) {
                    told++;
                    fprintf(stderr, "STALL level %d at (%.1f,%.1f) %.0f s: state %d resume %d target %d blocked %.1f cooldown %.1f leaving %d\n",
                            G.level, G.px, G.py, G.t - st0, G.state, G.resume_state, G.target, G.blocked, G.cooldown, G.leaving);
                    for (int i = 0; i < MAXMOBS; i++) {
                        const Mob *o = &G.mobs[i];
                        float d = hypotf(o->x - G.px, o->y - G.py);
                        if (!o->alive || d > 12.0f) continue;
                        const Pack *op = pack_of(o);
                        fprintf(stderr, "   STALL #%d %s r%d at (%.1f,%.1f) %.1f cells st%d sub%d aw%d alert%.1f hold%d eng%d hp%.0f/%.0f solid%d tgt%d clear%d/%d "
                                "misses%d ign%.1f call%.1f pack%d/%d/%d/%d amb%d hunter%d role%d face%.2f\n",
                                i, KINDS[o->type].name, o->rank, o->x, o->y, d, o->state, o->sub, o->aware, o->alert, o->hold, o->engage, o->hp, o->maxhp,
                                solid_mob(o), targetable(o), line_clear(G.px, G.py, o->x, o->y), shot_clear(o->x, o->y), o->misses,
                                o->ignore - (float)G.t, o->call_t, op ? op->kind : -1, op ? op->state : -1, op ? op->broken : -1, op ? op->caller : -1,
                                o->amb, o->hunter, o->role, o->face);
                    }
                }
            }
        }
    }
#endif
    /* Explore first and gather treasure: head for the exit (via its key and vault if need be) once
     * most of the treasure is in hand and most of the level seen, or after four and a half minutes,
     * or at five and a half whatever happens. Not while a key is in hand for a vault that doesn't
     * hold the exit, or while a freshly opened vault is still full. */
    if (!G.exit_seen && G.exit_fx >= 0 && hypotf(G.exit_fx + 0.5f - G.px, G.exit_fy + 0.5f - G.py) < 7.0f
        && line_clear(G.px, G.py, G.exit_fx + 0.5f, G.exit_fy + 0.5f)) {
        G.exit_seen = 1;
        snd_sfx_ex(SFX_SHIMMER, 0.35f, 0.5f);                       /* a faint call from the runes */
#ifdef CRAWL_DEBUG
        fprintf(stderr, "EVENT exit_seen t=%.2f\n", G.t);
#endif
    }
    int pending = 0;
    for (int k = 0; k < G.nvaults; k++) {
        if (G.vault_door[k] < 0) continue;                         /* a drawn level's gold and crimson, and no silver */
        const Door *d = &G.doors[G.vault_door[k]];
        if (k == G.exit_in_vault) continue;
        if (G.has_key[k] && d->locked == 1) pending = 1;
        if (d->locked == 2 && G.level_t - G.unlock_t < 30.0) {
            for (int i = 0; i < G.nitems; i++)
                if (G.items[i].alive && G.items[i].type != IT_KEY && vault_of_room(G.items[i].cx, G.items[i].cy) == k) pending = 1;
        }
    }
    float got = G.value_total ? (float)G.value_got / (float)G.value_total : 1.0f;
    float want = RUN.goal > RUN.treasure ? 0.9f : 0.75f;            /* saving up: it wants nearly all of it */
    double stay = G.hp >= G.max_hp * 0.6f ? 30.0 : 0.0;            /* in good health it lingers for the treasure */
    if (!G.leaving && !G.dead && G.exit_fx >= 0 && !G.player
        && ((G.level_t > 270.0 + stay && !pending) || G.level_t > 330.0 + stay
            || (got >= want && G.explored > G.ncells * 0.7f && G.level_t > 150.0 && !pending))) {
        G.leaving = 1;
        if (spirit() >= 2.0f && !(G.wisps[0].on && G.wisps[0].kind == WK_EXIT && G.wisps[0].state != WS_GONE))
            summon_wisp(WK_EXIT, 0, WISP_COST);                    /* a soul to spare: a wisp to light the way out */
#ifdef CRAWL_DEBUG
        fprintf(stderr, "EVENT leaving t=%.2f in_vault=%d got=%.2f\n", G.t, G.exit_in_vault, got);
        if (G.tri >= 0 && G.tri_state == TRS_SHUT && G.tri_ix >= 0) {     /* a trial left untried: why */
            const Door *td = &G.doors[G.tri_door];
            fprintf(stderr, "EVENT trial_passed %d %s level %d seen=%d cells=%.0f hp=%d/%d\n", G.tri, TRI_NAME[G.tri], G.level,   /* no bfs here: it would move the dream */
                    !!(G.mapped[td->y][td->x] & 4), hypotf(G.tri_ix + 0.5f - G.px, G.tri_iy + 0.5f - G.py), G.hp, G.max_hp);
        }
#endif
    }
    if (!G.tally && !G.dead && !G.player && !PK.depth && (G.exit_fx < 0 || G.level_t > (boss_sealed() ? 1500.0 : 600.0))) {   /* no exit, or hopelessly lost: call it done (a boss gets longer) */
        G.tally = 1; G.tally_t = 0.0; G.clear_time = G.level_t; G.retries = 0; snd_sfx_ex(SFX_FANFARE, 1.0f, 0.5f);
        rec_cleared();
    }
    float fout = 1.0f;
    if (G.dead) {                                                  /* down to the floor, a red haze, then the level again */
        G.dead_t += dt;
        if (G.dcost) {                                             /* a death that costs: its card holds the haze until let go */
            if (G.dead_t > 1.6) G.dcard_t += fdt;
            if (!G.player && G.dcard_t > 6.0f) G.dcard_done = 1;
            if (!G.dcard_done && G.dead_t > 3.0) G.dead_t = 3.0;
        }
        G.camz = lerpf(0.5f, 0.1f, smoothstepf(0.0f, 0.9f, (float)G.dead_t));
        fout = 1.0f - smoothstepf(3.8f, 4.8f, (float)G.dead_t);
        if (G.dead_t >= 5.0) {
            if (G.dcost) death_wake();                             /* by the merchant, before level 1 */
            else if (PK.depth) pocket_return(1);                   /* in a pocket: thrown back out, the trial failed */
            else { RUN.treasure -= G.level_treasure; G.hp = gear_max_hp(&RUN.gear); if (G.bchallenge) challenge_begin(G.boss); else start_level(); }
        }
    }
    PK.since += fdt;
    if (PK.depth && !G.player) PK.auto_t += fdt;                    /* the autopilot's time in the level's chains */
    float fin = fminf(smoothstepf(0.0f, 1.2f, (float)G.level_t), smoothstepf(0.0f, 0.6f, PK.since));   /* and through a doorway */
    if (G.exiting) fout = 1.0f - smoothstepf(1.6f, 2.4f, G.exit_t);  /* into the light */
    G.fade = fin * fout;
    build_lightmap();
}

