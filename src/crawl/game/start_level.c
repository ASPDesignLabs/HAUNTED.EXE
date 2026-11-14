/* A part of scene_crawl.c (docs/PLAN-tidy.md): it is #included there in its place and never compiled on its own.
 * start_level: a level begun; the debug build's gear and gifts. */

static void start_level(void) {
    line_lend();
    pocket_reset_level();
    G.replay = RUN.replay_from != 0;                               /* a level replayed through the Portal */
    RUN.perfect = 0;                                               /* her hall's piece is the habanera again unless this one is perfect */
    G.corrupt = RUN.bell_next == G.level; G.cshown = G.corrupt && RUN.bell_secrets; G.fonted = RUN.font_next == G.level;   /* the bell's and */
    RUN.bell_next = RUN.bell_secrets = RUN.font_next = 0;          /* the font's, for one attempt: a death or fleeing ends them */
#ifdef CRAWL_DEBUG
    if (getenv("CRAWL_CORRUPT")) { G.corrupt = 1; G.cshown = atoi(getenv("CRAWL_CORRUPT")) >= 2; }   /* every level corrupted; 2 shown */
    if (G.corrupt || G.fonted) fprintf(stderr, "EVENT corrupted level %d shown %d fonted %d t=%.2f\n", G.level, G.cshown, G.fonted, G.t);
#endif
    G.rooms_in = 0;                                                /* a new level's rooms, each worth finding */
    G.lb_spent = 0; G.lb_at = 0.0;                                 /* Constitution's killing blow, once a level (D5) */
    G.d7_lg = 0; G.d7_shep = 0.0f; G.d7_splint = 0;                /* the augments' (D7): nothing held over from the level before */
    G.glance = G.corner_on = G.amb_read = G.amb_probed = 0; G.glance_last = -99.0f; G.corner_cd = 0.0f;   /* the dream's counters */
    G.map_on = 0;                                                  /* rolled up between levels */
    memset(G.has_key, 0, sizeof G.has_key);
    G.shop = 0;
    hz_clear(); snd_set_boss(0, 0);
    G.boss = boss_of_level(G.level); G.bstate = G.boss >= 0 ? BS_WAIT : BS_NONE; G.bwave = G.badds = 0; G.bmob = -1;
    G.bz_t = 6.0f; G.bz2_t = 9.0f; G.bcast_t = 0.0f; G.omen_t = 30.0f + 20.0f * rndf();
    G.bfight_t = G.bstate_t = G.bcard = G.bring = 0.0f; G.bbar = 1.0f;
    G.bpose = BP_NONE; G.bpose_t = G.bdash_t = G.bmirror_t = G.bmirror_cd = G.bhurt = 0.0f; G.bdash_age = 99.0f;
    memset(G.bdash_cd, 0, sizeof G.bdash_cd);
    G.bward = G.bward_cd = G.bchant_dmg = 0.0f;
    G.bbz_n = 0; G.bbz_at = -1; G.bbz_t = G.bslip_t = G.bslip_cd = G.bgutter_cd = 0.0f; G.bww = 0; G.bww_t = 0.0f; G.bww_cd = 12.0f;
    G.bsp_t = 5.0f; G.bmend_t = G.bmend_cd = G.web_hp = G.web_rear = G.web_idle = G.btrickle_t = 0.0f; G.web_cd = 8.0f; G.bmend_base = 1e9f;
    if (G.boss < 0) G.bchallenge = 0;
    G.bstep = G.bchallenge && G.boss >= 0 ? RUN.bstep[G.boss] : 0;
    crawl_art_level(G.level);
    ui_marble_make(0x51ABu ^ (unsigned)G.level * 7919u); ui_marble_lv = G.level;   /* the menus' marble, made now, not on a frame in play */
    g_drawn = drawn_level(G.level);                                /* the owner's drawing (docs/PLAN-tutorial.md), laid on its own dice */
    uint32_t run_rs = G.rs;
    if (g_drawn) { G.rs = drawn()->seed; gen_drawn(); } else gen_level();
    memset(G.mobs, 0, sizeof G.mobs); G.nroute = 0; G.npacks = 0; G.gathered = 0;   /* and the patrols' rounds, the packs */
    memset(G.scent, 0, sizeof G.scent); G.namb = 0; G.hunt_kind = G.hunt_on = G.hunt_said = 0; G.noise_t = -1.0f; G.still_t = 0.0f;   /* the trail */
    memset(G.shots, 0, sizeof G.shots);
    memset(G.booms, 0, sizeof G.booms);
    memset(G.bones, 0, sizeof G.bones);
    memset(G.beams, 0, sizeof G.beams);
    memset(G.burns, 0, sizeof G.burns); memset(G.trails, 0, sizeof G.trails); memset(G.venoms, 0, sizeof G.venoms);
    memset(G.motes, 0, sizeof G.motes); G.full_said = 0; soul_cap(1);   /* an older run's souls, over the vessel, go quietly */
    memset(G.remains, 0, sizeof G.remains); G.nremains = 0;
    reset_caster_state();
    const Room *R = &G.rooms[0];
    G.cx = R->x + R->w / 2; G.cy = R->y + R->h / 2;
    G.cx -= ((G.cx - 2) % 4 + 4) % 4; G.cy -= ((G.cy - 2) % 4 + 4) % 4;   /* where two lanes cross */
    if (G.map[G.cy][G.cx].kind != K_FLOOR) { G.cx = R->x + 1; G.cy = R->y + 1; }
    if (g_drawn) drawn_start();
    G.exit_seen = G.leaving = G.exiting = 0; G.exit_open = G.exit_t = 0.0f; G.seek_secret = -1;
    G.kills = G.kills_total = 0; G.tally = 0; G.tally_t = 0.0; G.tally_done = 0;
    memset(G.tally_shown, 0, sizeof G.tally_shown);
    if (g_drawn) { drawn_items(); drawn_walls(); }                 /* where drawn: no potion, sites, things or spectres */
    else {
        place_items(G.cx, G.cy);
        if (G.boss < 0) place_exit(G.cx, G.cy);                    /* a boss level's way out is beyond its boss */
    }
    place_altar();
    if (g_drawn) { G.nsite = 0; G.snare_t = 0.0f; G.arise_t = 1e9f; }
    else place_sites();                                            /* the run's fixed hazards (1.0.0b13) */
    G.nthing = 0; G.carry = 0; G.read_t = 0.0f;                    /* a puzzle's things are its level's */
    docs_clear();                                                  /* and what was read on the last */
    pz_make();
    pz_hide();                                                     /* its answer, written down and hidden; the Black Shrine */
    tri_place(); snd_set_trial(0);                                 /* the trial's door, its lights */
    if (!g_drawn) {
        place_rests();                                             /* the places to mend (1.0.0b15) */
        place_marked();                                            /* things that want a spell (1.0.0b16) */
        place_fights();                                            /* and things for fights (step 8) */
    } else drawn_marks();                                         /* level 2's (docs/PLAN-tutorial.md 7.2, 7.3, 7.6); level 1 has none */
    ward_make();                                                   /* a soul-lamp by each challenge, out (1.0.0b18.1) */
#ifdef CRAWL_DEBUG
    if (getenv("CRAWL_THINGS")) things_test();
#endif
    G.level_treasure = 0; G.level_souls = 0; G.dead = 0; G.dead_t = 0.0; G.spirit_said = G.splint_said = 0;
    G.wraith_done = G.wraith_now = 0; G.wraith_try = 0.0f;          /* the Wraith comes once a level, and again after a death */
    if (g_drawn) { G.spec_n = 0; G.spec_t = 1e9f; } else spectres_set();
    memset(G.wisps, 0, sizeof G.wisps);                            /* wisps don't go down the stairs with you */
    G.hurt = G.gold = G.kick = G.chill = 0.0f; G.pvx = G.pvy = 0.0f;
    G.px = G.cx + 0.5f; G.py = G.cy + 0.5f;
    G.h = rnd(4);
    for (int k = 0; k < 4 && !apass(G.cx + DX[G.h], G.cy + DY[G.h]); k++) G.h = (G.h + 1) & 3;
    if (g_drawn) G.h = drawn()->face;
    G.ang = heading_angle(G.h);
    G.map[G.cy][G.cx].visits = 1; G.explored = 1;
    G.spd = 0.0f; G.bobph = 0.0f; G.camz = 0.5f; G.hand = 0.0f; G.cooldown = 2.0f; G.calm = 0.0f;
    start_move();
    memset(G.spawned, 0, sizeof G.spawned); memset(G.named, 0, sizeof G.named);
    if (g_drawn) drawn_dead();
    else if (mind_on()) place_creatures(); else place_creatures_old();   /* step 8: one budget spent on set pieces */
    if (G.corrupt) snd_sfx_ex(SFX_DEEPCHIME, 0.45f, 0.5f);         /* and the bell heard as you come */
    if (G.boss >= 0) boss_spawn();
    hunt_cx = -1;
    if (G.level >= 7 && rnd(3) == 0) {                             /* now and then one of them is gilded, and carries treasure */
        int pick[MAXMOBS], np = 0;
        for (int i = 0; i < MAXMOBS; i++) {
            const Mob *m = &G.mobs[i];
            if (m->alive && !m->guard && !m->boss && KINDS[m->type].gild) pick[np++] = i;
        }
        if (np) set_rank(&G.mobs[pick[rnd(np)]], RK_GILDED);
        amb_gild();                                                /* and one time in two it runs to a kill room when hurt (L2) */
    }
    if (g_drawn) G.rs = run_rs;                                    /* the drawing laid: the run's own dice again */
    mice_level();                                                  /* the mice and their holes, on their own dice (docs/PLAN-mice.md) */
#ifdef CRAWL_DEBUG
    { int nt[NKINDS] = { 0 }, nr[NRANK] = { 0 }; for (int i = 0; i < MAXMOBS; i++) if (G.mobs[i].alive) { nt[G.mobs[i].type]++; nr[G.mobs[i].rank]++; }
      fprintf(stderr, "LEVEL %d mobs:", G.level);
      for (int k = 0; k < NKINDS; k++) if (nt[k]) fprintf(stderr, " %s %d", KINDS[k].name, nt[k]);
      fprintf(stderr, "; ranks %d %d %d %d %d gilded %d\n", nr[0], nr[1], nr[2], nr[3], nr[4], nr[5]); }
#endif
#ifdef CRAWL_DEBUG
    if (getenv("CRAWL_TPBOSS") && atoi(getenv("CRAWL_TPBOSS")) == 2 && G.boss >= 0 && G.bstart_x >= 0) {   /* testing: the autopilot */
        G.cx = G.bstart_x; G.cy = G.bstart_y; G.px = G.cx + 0.5f; G.py = G.cy + 0.5f; G.h = 0; G.ang = 0.0f;   /* in the antechamber, going */
        G.leaving = 1; start_move();
    }
    if (getenv("CRAWL_TPTRIAL") && atoi(getenv("CRAWL_TPTRIAL")) == 2 && G.tri >= 0 && G.tri_wx >= 0) {   /* testing: the autopilot a few */
        int bd = 99;                                               /* steps from a trial's door, on the lane nearest it */
        for (int y = G.tri_wy - 4; y <= G.tri_wy + 4; y++)
            for (int x = G.tri_wx - 4; x <= G.tri_wx + 4; x++) {
                int dd = abs(x - G.tri_wx) + abs(y - G.tri_wy);
                if (x > 0 && y > 0 && x < MAPN - 1 && y < MAPN - 1 && apass(x, y) && !in_trial(x, y) && dd < bd) { bd = dd; G.cx = x; G.cy = y; }
            }
        G.px = G.cx + 0.5f; G.py = G.cy + 0.5f; G.tx = G.cx; G.ty = G.cy;
    }
#endif
    G.level_t = 0.0; G.card = 0.0f; G.ending = 0;
    if ((RUN.corner >> CS_WIND & 1) && crawl_run(G.level) != RUN.wind_run) { RUN.wind_spent = 0; RUN.wind_run = crawl_run(G.level); }   /* a new run: a Second Wind again */
    map_carto();
    G.readout = READOUT_T + 1.5f;                                  /* show where things stand as the level opens */
    G.bar = 1.0f; show_bar();
    if (G.player) human_begin();
    tu_level_begin();                                              /* the lessons, on a drawn level (docs/PLAN-tutorial.md) */
#ifdef CRAWL_DEBUG
    if (getenv("CRAWL_MAP")) debug_map(0);
#endif
}

/* every rune at the quality tier t comes to in an old save, and an Uncommon piece of level 2t in every slot
 * (about what tier t used to wear); for the console and the test switches alike. Spells are the lines' now */
#ifdef CRAWL_DEBUG
/* testing: each line, in the elements' order, to a node: 0 closed, 1 opened, 2 with its Efficiency and Insight, 3 and
 * its ability; a, b, c path A to its first, second or last node; x, y, z the same of path B */
static void align_debug(const char *s) {
    memset(RUN.al, 0, sizeof RUN.al);
    for (int e = 0; e < NELEM && s[e]; e++) {
        char ch = s[e];
        int to = ch == '0' ? -1 : ch == '1' ? ND_OPEN : ch == '2' ? ND_INS : ch == '3' ? ND_ABIL
               : ch >= 'a' && ch <= 'c' ? ND_A1 + (ch - 'a') : ch >= 'x' && ch <= 'z' ? ND_B1 + (ch - 'x') : -1;
        for (int n = 0; n <= ND_ABIL && n <= to; n++) RUN.al[e][n] = (uint8_t)ALIGN[e].n[n].cost;
        int p0 = to >= ND_B1 ? ND_B1 : ND_A1;
        for (int n = p0; n <= to && to >= ND_A1; n++) RUN.al[e][n] = (uint8_t)ALIGN[e].n[n].cost;
    }
    if (align_open(EL_PHYS)) RUN.gear.bones = (int16_t)bones_max();   /* and the spear's bones */
}
#endif
static void give_gear(int t) {
    uint32_t rs = 0x9e3779b9u ^ (uint32_t)t;
    static const int KIND_OF[NSLOT] = { IK_WRIST, IK_WRIST, IK_BOOTS, IK_NECK, IK_HOOD, IK_ROBE, IK_RING };
    for (int i = 0; i < NDEF; i++) RUN.gear.def[i] = (int8_t)rune_from_tier(t);
    RUN.gear.bones = (int16_t)clampi(SPEAR_BONES * t, 0, BONES_MAX);   /* as if the spear were bought t times */
    for (int s = 0; s < NSLOT && t > 0; s++) RUN.gear.worn[s] = piece_roll(KIND_OF[s], 2 * t, Q_UNCOMMON, &rs);
}
static int give_ability(int a) {                                   /* every worn piece with that ability */
    int n = 0;
    for (int sl = 0; sl < NSLOT; sl++) if (RUN.gear.worn[sl].kind) { RUN.gear.worn[sl].ability = (uint8_t)clampi(a, 0, NABIL - 1); n++; }
    return n;
}
static void give_bag(int q) {                                      /* a bag of that quality, all but one place filled */
    uint32_t rs = 0x85ebca6bu ^ (uint32_t)q;
    q = clampi(q, 0, NQUAL - 1);
    RUN.gear.bag_size = (uint8_t)bag_capacity(q); RUN.gear.bag_quality = (uint8_t)q;
    for (int i = 0; i + 1 < RUN.gear.bag_size; i++) RUN.gear.bag[i] = piece_roll(IK_WRIST + (int)(rs % 6), 3 + i * 2, i % NQUAL, &rs);
}

#ifdef CRAWL_DEBUG
/* for testing: CRAWL_GEAR=n (give_gear), CRAWL_RUNES=q (every rune at quality q, 1 Common to 5
 * Exquisite), CRAWL_ABIL, CRAWL_BAG, CRAWL_GOLD, CRAWL_SOULS, CRAWL_COINS (Cursed Coins), CRAWL_SHRINE=r
 * (the Rune Shrine's rank, 0 sanctified to 5) */
static void debug_gear(void) {
    if (RUN.level > 1) RUN.peds = gear_shop_pedestals(RUN.level);  /* started deeper: as an old save would come */
    if (getenv("CRAWL_PEDS")) RUN.peds = clampi(atoi(getenv("CRAWL_PEDS")), 2, NPED);
    if (getenv("CRAWL_GEAR")) give_gear(atoi(getenv("CRAWL_GEAR")));
    if (getenv("CRAWL_ALIGN")) align_debug(getenv("CRAWL_ALIGN"));   /* the lines opened, to a node and a path */
    if (getenv("CRAWL_CONF")) conf_debug(getenv("CRAWL_CONF"));    /* each Confession's rank (1.0.0b20) */
    aug_debug_set();                                               /* CRAWL_AUGSET: the augments chosen, at a rank (1.0.0b21) */
    if (getenv("CRAWL_ALPTS")) RUN.al_points = atoi(getenv("CRAWL_ALPTS"));
    if (getenv("CRAWL_CLASS")) { RUN.cls = 0; if (atoi(getenv("CRAWL_CLASS")) > 0) class_take(atoi(getenv("CRAWL_CLASS"))); }   /* 0: asked */
    if (getenv("CRAWL_ATONE")) RUN.atone = clampi(atoi(getenv("CRAWL_ATONE")), 1, 999);   /* the Level of Atonement, nothing bought with it */
    if (getenv("CRAWL_XPBANK")) RUN.xp = imax(0, atoi(getenv("CRAWL_XPBANK")));   /* experience banked at a fresh start: the dream ascends on it */
    if (getenv("CRAWL_CLEARED")) for (int l = 1; l <= imin(atoi(getenv("CRAWL_CLEARED")), DESCENT_LEN); l++) RUN.rec.cleared |= 1ull << (l - 1);   /* levels 1-n cleared */
    if (getenv("CRAWL_RUNES")) for (int i = 0; i < NDEF; i++) RUN.gear.def[i] = (int8_t)clampi(atoi(getenv("CRAWL_RUNES")), 0, RUNE_EXQ);
    if (getenv("CRAWL_COINS")) RUN.coins = atoi(getenv("CRAWL_COINS"));
    if (getenv("CRAWL_SHRINE")) RUN.shrine = clampi(atoi(getenv("CRAWL_SHRINE")), 0, NQUAL);
    if (getenv("CRAWL_ABIL")) give_ability(atoi(getenv("CRAWL_ABIL")));
    if (getenv("CRAWL_BAG")) give_bag(atoi(getenv("CRAWL_BAG")));
    if (getenv("CRAWL_GOLD")) RUN.treasure = atoi(getenv("CRAWL_GOLD"));
    if (getenv("CRAWL_WAYS")) { RUN.ways = (1u << NWAY) - 1; RUN.deed_amb = 3; RUN.deed_hush = 2; }   /* 1.0.0b18: every way met, for the shots */
    if (getenv("CRAWL_SEENALL")) for (int k = 0; k < NKINDS && k < REC_KINDS; k++) {   /* every kind met, and some slain, for the shots */
        RUN.rec.seen[k] = 0x1f;
        for (int r = 0; r < NRANK; r++) RUN.rec.kills[k][r] = (uint16_t)((k * 7 + r * 5) % 23 + (r == 0 ? 30 : 0));
    }
    if (getenv("CRAWL_STONE")) for (int k = 0; k < atoi(getenv("CRAWL_STONE")) && k < STONES; k++) {   /* stones in the plot, from what is held */
        Stone *st = &RUN.stones[k];
        memset(st, 0, sizeof *st);
        st->when = (int64_t)time(NULL) - 86400 * (k + 1); st->level = 3 + k * 4; st->gold = 1200 + k * 300; st->souls = 14; st->coins = k;
        st->by.what = BL_MOB; st->by.type = MOB_SKEL; st->by.rank = (uint8_t)(k % 3);
        memcpy(st->atk, RUN.gear.atk, sizeof st->atk); memcpy(st->def, RUN.gear.def, sizeof st->def);
        st->lined = 1; st->cls = RUN.cls ? RUN.cls : 1;
        memcpy(st->worn, RUN.gear.worn, sizeof st->worn); memcpy(st->bag, RUN.gear.bag, sizeof st->bag);
        st->bag_size = RUN.gear.bag_size; st->bag_quality = RUN.gear.bag_quality;
        RUN.nstones = k + 1;
    }
    if (getenv("CRAWL_SOULS")) RUN.souls = atoi(getenv("CRAWL_SOULS"));
    const char *tot = getenv("CRAWL_TOTEM");                       /* each boss ability's rank, a digit each: 3300 */
    for (int b = 0; tot && b < NBOSS && tot[b]; b++) { RUN.abil[b] = (uint8_t)clampi(tot[b] - '0', 0, ABIL_TOP); if (RUN.abil[b]) RUN.rec.boss[b].slain = 1; }
    if (getenv("CRAWL_SCRIP")) for (int b = 0; b < NBOSS; b++) { RUN.scrip[b] = atoi(getenv("CRAWL_SCRIP")); RUN.rec.boss[b].slain = 1; }   /* and the bosses beaten */
    if (getenv("CRAWL_RUNTOP")) RUN.run_top = atoi(getenv("CRAWL_RUNTOP"));   /* the runs the Portal opens */
    if (getenv("CRAWL_BSTEP")) for (int b = 0; b < NBOSS; b++) RUN.bstep[b] = clampi(atoi(getenv("CRAWL_BSTEP")), 0, 5);   /* each challenge's step */
}
#endif

/* A piece comes to the caster, bought or found. Found, or bought by the autopilot: worn if it beats
 * what is on, otherwise into the bag. Bought by a person: worn at once (wear). What has nowhere to go
 * is lost. A caption says which. */
static void take_piece(const Offer *o, int wear) {
    const Piece *p = &o->piece;
    char a[48], b[48];
    piece_name(p, a, b, sizeof a);
    int slot = piece_slot(&RUN.gear, p->kind);
    const char *was = slot >= 0 ? KIND_NAME[RUN.gear.worn[slot].kind] : "";
    int old = G.max_hp;
    switch (wear ? gear_take_wear(&RUN.gear, o) : gear_take(&RUN.gear, o)) {
    case TAKE_WORN: say("NOW WEARING %s", b); break;
    case TAKE_WORN_OLD_BAGGED: say("%s ON; THE OLD %s IN THE BAG", KIND_NAME[p->kind], was); break;
    case TAKE_WORN_OLD_LOST: say("%s ON; THE OLD %s LOST", KIND_NAME[p->kind], was); break;
    case TAKE_BAGGED: say("INTO THE BAG: %s", b); break;
    case TAKE_LOST: say("NO ROOM FOR IT: %s LEFT", KIND_NAME[p->kind]); break;
    case TAKE_BAG: say("A %s: ROOM FOR %d", bag_name(RUN.gear.bag_size), RUN.gear.bag_size); break;
    default: break;
    }
    if (p->ability == AB_KEYMAKER && G.player) say("KEYMAKER: KEY WISPS IN THE L MENU");   /* on every purchase */
    G.max_hp = gear_max_hp(&RUN.gear);
    if (G.max_hp > old) G.hp += G.max_hp - old;                    /* new strength comes with it */
    if (G.hp > G.max_hp) G.hp = G.max_hp;
}

/* The run carries on across scene switches: the level, HP, gold, souls and gear are kept, and the
 * current level is laid out afresh (without a second visit to the merchant). */

