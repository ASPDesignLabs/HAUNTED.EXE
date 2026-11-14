/* A part of scene_crawl.c (docs/PLAN-tidy.md): it is #included there in its place and never compiled on its own.
 * Harm that reaches the dead: kill_mob, weaknesses and strengths, elemental creatures, damage_mob, the dead's statuses and their gate (mob_gate), trails, venoms, blasts; the spectre; the hunt field (build_hunt, alert_near). */

static void xp_rooms(void) {                                       /* a room first set foot in */
    if (G.shop || PK.depth) return;
    int cx = (int)G.px, cy = (int)G.py;
    for (int i = 0; i < G.nrooms && i < 31; i++)
        if (!(G.rooms_in >> i & 1) && in_room(&G.rooms[i], cx, cy)) { G.rooms_in |= 1 << i; xp_gain(3.0f * xp_level(), "A ROOM FOUND"); }
}

static void mob_poison(Mob *m, float total, float secs);
static float permafrost(void);
static int caller_hushed(const Mob *m);
static void kill_mob(Mob *m) {
    if (caller_hushed(m)) deed_note(DEED_HUSH);                    /* slain before its call went out */
    m->state = MS_DIE; m->t = 0.0f; m->sleep = 0; m->hold = 0; m->engage = 0;
    if (m->amb && m->bait_st && m->bait_st != B_GILD && m->amb <= G.namb && !G.amb[m->amb - 1].sprung) {   /* the bait slain */
        const Ambush *a = &G.amb[m->amb - 1];
        if (in_room(&G.rooms[a->room], (int)m->x, (int)m->y)) amb_spring(m->amb - 1, "bait_slain"); else amb_out(m->amb - 1, m->x, m->y, 0);
    }
    if (!KIND_IS(m, KF_UNCOUNTED)) { if (!PK.depth) G.kills++; if (!m->boss) { rec_kill(m); por_feat_kill(m); } if (G.player) WELL.k[WELL.i]++; }   /* a pocket's are in the
                                                                      record, not the level's tally */
    if (PK.depth && m->gen > 0 && m->gen <= 32) PK.slain[PK.chain][clampi(PK.depth, 0, POCKET_LINKS - 1)] |= 1u << (m->gen - 1);
    sfx_at(KINDS[m->type].sfx_die, m->x, m->y, 1.0f);
    noise(m->x, m->y, 4, NZ_DEATH, m->type);                        /* heard four steps off, eight by its own kind (S3) */
    if (m->type == MOB_SPIDER && m->z > 0.3f) m->sub = 9;           /* it falls from the web */
    if (m->type == MOB_DROWNED) { add_pool(m->x, m->y); sfx_at(SFX_EXPLODE, m->x, m->y, 0.6f); }   /* it bursts */
    if (m->type == MOB_BROOD) {                                    /* her sacs die with the last of the mothers */
        int others = 0;
        for (int i = 0; i < MAXMOBS; i++) others += G.mobs[i].alive && G.mobs[i].state == MS_WALK && G.mobs[i].type == MOB_BROOD && &G.mobs[i] != m;
        if (!others) for (int i = 0; i < MAXMOBS; i++) if (G.mobs[i].alive && G.mobs[i].type == MOB_SAC && G.mobs[i].state == MS_WALK) { G.mobs[i].state = MS_DIE; G.mobs[i].t = 0.0f; }
    }
    if (m->pois_t > 0.0f && align_has(EL_POISON, ND_BULT)) {       /* Contagion: what is left of its poison, a fresh dose at 60% */
        float dose = m->pois_dps * 4.0f * 0.6f;
        int n = 0;
        for (int i = 0; i < MAXMOBS; i++) {
            Mob *o = &G.mobs[i];
            if (o == m || !o->alive || o->state != MS_WALK || hypotf(o->x - m->x, o->y - m->y) > 2.0f) continue;
            mob_poison(o, dose, 4.0f); n++;
        }
        m->pois_t = 0.0f;
        if (n) { sfx_at(SFX_SPLASH, m->x, m->y, 0.7f); DBG_COUNT(contagions); }
    }
    if (m->boss) return;                                           /* a boss leaves only its scrip */
    if (m->type == MOB_GHOST) {                                    /* a spectre, slain by Spirit: its souls, nothing more */
        spawn_mote(m->x, m->y, spectre_souls() * soul_share());
        por_bump(MOB_GHOST);                                       /* laid to rest, for its portrait */
#ifdef CRAWL_DEBUG
        fprintf(stderr, "EVENT spectre slain t=%.2f\n", G.t);
#endif
        return;
    }
    xp_kill(m);
    spawn_mote(m->x, m->y, (RANK_SOULS[m->rank] + (m->last_atk == AT_DRAIN && align_has(EL_SPIRIT, ND_ABIL) ? (m->rank >= RK_III ? 2.0f : 1.0f) : 0.0f))   /* Harvest */
                           * soul_share());                        /* Spirituality, and the class's own */
    if (m->unb_t > 0.0f && cls_ab(EL_SPIRIT, 0) > 0.0f) { spawn_mote(m->x, m->y, 1.0f); DBG_COUNT(sin_souls); }   /* Swallowed Sin: Unbound, a soul
                                                                      more (G5) */
    if (m->pois_t > 0.0f && dk(EL_POISON, 2, 1) > 0.0f && G.map[m->cy][m->cx].kind == K_FLOOR)   /* Pestilence: the poisoned leave a pool (D7) */
        for (int i = 0; i < MAXVENOM; i++)
            if (!G.venoms[i].alive) {
                G.venoms[i] = (Venom){ 1, m->x, m->y, 0.6f, 0.0f, 3.0f, fmaxf(1.0f, m->pois_dps * m->pois_t) * dk(EL_POISON, 2, 1), 0.0f, 0 };
                DBG_COUNT(aug_pests); break;
            }
    if (m->last_atk == AT_SPEAR && (m->stagger > 0.0f || m->pin > 0.0f) && cls_ab(EL_PHYS, 0) > 0.0f && G.map[m->cy][m->cx].kind == K_FLOOR) {
        Item *it = add_item(m->cx, m->cy, IT_BONE);                /* the Block: the spear's bone left where it fell (G5) */
        if (it) { it->secret = 1; it->seen = 1; DBG_COUNT(block_bones); }
    }
    if (G.reap_t > 0.0f && RUN.gear.def[DF_REAP] >= RUNE_EXQ && !G.dead && !KIND_IS(m, KF_UNCOUNTED)) {   /* Exquisite: a little life back */
        G.hp = clampi(G.hp + (G.max_hp + 19) / 20, 0, G.max_hp);
        DBG_COUNT(exq_reaped);
    }
    /* what it leaves: the gilded always something good, the higher ranks now and then coin or a potion.
     * Like a closet's loot it isn't counted in the level's treasure, and it glints a little */
    int drop = -1;
    if (m->add) ;                                                  /* a boss's adds leave no treasure, nor potions */
    else if (m->rank == RK_GILDED) drop = rnd(3) ? IT_CROWN : IT_CHEST;
    else if (m->rank >= RK_IV && rnd(10) == 0) drop = IT_POTION;
    else if (m->rank >= RK_II && rndf() < 0.08f * (float)m->rank) drop = m->rank >= RK_IV ? IT_GEMS : IT_COINS;
    if (drop >= 0 && G.map[m->cy][m->cx].kind == K_FLOOR) {
        Item *it = add_item(m->cx, m->cy, drop);
        if (it) {
            it->secret = 1; it->seen = 1;
            if (drop < IT_KEY) { G.items_total--; G.value_total -= treasure_value(drop); }
        }
    }
    /* the bony ones leave a bone for the spear now and then: about one in eight, more at higher ranks */
    int bony = m->type == MOB_SKEL || m->type == MOB_SHIELD || m->type == MOB_KNIGHT || m->type == MOB_HOUND;
    if (bony && G.map[m->cy][m->cx].kind == K_FLOOR && rndf() < 0.125f + 0.03f * (float)m->rank) {
        Item *it = add_item(m->cx, m->cy, IT_BONE);
        if (it) { it->secret = 1; it->seen = 1; }
    }
    /* Luck (docs/PLAN-b9.md, section 5): a chance of gold of its rank, a share of what the level's treasure
     * comes to, and half the time something with it: a bone from the bony, a potion from rank III, else souls */
    if (!m->add && !KIND_IS(m, KF_UNCOUNTED) && G.map[m->cy][m->cx].kind == K_FLOOR && rndf() < gear_total(&RUN.gear, ST_LUCK) / 100.0f) {
        static const int TIER_ITEM[NRANK] = { IT_COINS, IT_CHALICE, IT_GEMS, IT_CROWN, IT_CHEST, IT_CHEST };
        static const float TIER_K[NRANK] = { 0.6f, 0.8f, 1.0f, 1.3f, 1.6f, 3.0f };
        int r = clampi(m->rank, 0, NRANK - 1);
        Item *it = add_item(m->cx, m->cy, TIER_ITEM[r]);
        if (it) {
            it->secret = 1; it->seen = 1;
            it->value = imax(1, (int)(gear_haul(G.level) * 0.12f * TIER_K[r] + 0.5f));
            G.items_total--; G.value_total -= treasure_value(TIER_ITEM[r]);   /* not the level's treasure */
            sfx_at(SFX_COIN, m->x, m->y, 0.45f);
#ifdef CRAWL_DEBUG
            fprintf(stderr, "EVENT lucky %s rank %d worth %d t=%.2f\n", KINDS[m->type].name, r, it->value, G.t);
#endif
        }
        if (rnd(2) == 0) {
            int res = bony ? IT_BONE : (r >= RK_III ? IT_POTION : IT_URN);
            Item *e = add_item(m->cx, m->cy, res);
            if (e) { e->secret = 1; e->seen = 1; }
        }
    }
}

/* the knight's plate comes off as it is hurt: 0 whole, 1 half, 2 down to the breastplate */
static int knight_stage(const Mob *m) {
    if (m->boss) return G.bwave <= 0 ? 0 : (G.bwave <= 2 ? 1 : 2);   /* the Ill-Fated Knight sheds plate as its waves come */
    return m->hp > m->maxhp * 0.66f ? 0 : (m->hp > m->maxhp * 0.33f ? 1 : 2);
}
static void knight_shed(Mob *m) {                                  /* standing, with plate gone: its feat spoilt (the portraits') */
    if (m->type == MOB_KNIGHT && !m->boss && m->hp > 0.0f && knight_stage(m)) m->feat |= FT_SHED;
}
static float knight_pace(const Mob *m, int st) {                   /* quicker as the plate comes off */
    if (m->boss) return KINDS[MOB_KNIGHT].speed * RANK_SPEED[m->rank] * BOSSES[m->boss - 1].speed * (1.0f + 0.15f * st);
    return KINDS[MOB_KNIGHT].speed * RANK_SPEED[m->rank] * (1.0f + 0.25f * st);
}

/* The bestiary's elements (1.0.0b19, docs/PLAN-combat.md section 7): every kind weak to one element (x1.5) and strong
 * against one (x0.5), the rest plain, as KINDS has it; from rank III one more strength, from V another, among the lines
 * the caster holds (the wards' way, folded in: they still crack). CRAWL_ELEM=0 in the debug build puts the old shaded
 * table and the old wards back, for the balance runs (the hazards' harm to the dead reads KINDS itself, and keeps the new
 * table either way). */
static int elem_mode(void) {                                       /* 0 the old table and wards (CRAWL_ELEM=0), 1 as made */
#ifdef CRAWL_DEBUG
    static int mode = -1;
    if (mode < 0) mode = !(getenv("CRAWL_ELEM") && atoi(getenv("CRAWL_ELEM")) == 0);
    return mode;
#else
    return 1;
#endif
}
#define STRENGTH_K 0.5f                                            /* what a strength lets through */
#ifdef CRAWL_DEBUG
static const float OLD_WEAK[][NELEM] = {                           /* 1.0.0b18's, for CRAWL_ELEM=0 */
    [MOB_SKEL] = { 1.0f, 0.75f, 1.25f, 0.5f, 0.75f, 1.25f },     [MOB_GHOST] = { 0 },
    [MOB_BAT] = { 1.0f, 1.5f, 1.25f, 1.5f, 1.0f, 1.0f },         [MOB_WSKULL] = { 1.0f, 1.0f, 1.0f, 0.5f, 1.0f, 1.0f },
    [MOB_SHIELD] = { 0.75f, 1.0f, 1.5f, 0.5f, 1.0f, 1.5f },      [MOB_SPIDER] = { 1.25f, 1.5f, 1.0f, 0.5f, 1.0f, 1.0f },
    [MOB_SHAMBLER] = { 1.5f, 0.75f, 1.0f, 0.75f, 1.25f, 1.0f },  [MOB_DROWNED] = { 1.5f, 0.5f, 1.25f, 0.75f, 1.0f, 1.0f },
    [MOB_HOUND] = { 1.0f, 1.5f, 1.25f, 0.5f, 0.75f, 1.0f },      [MOB_WRAITH] = { 0 },
    [MOB_HUNTER] = { 1.25f, 1.5f, 1.0f, 0.5f, 1.0f, 1.0f },      [MOB_GHOUL] = { 1.0f, 1.5f, 1.0f, 0.75f, 1.25f, 1.0f },
    [MOB_KNIGHT] = { 0.75f, 1.0f, 1.5f, 0.25f, 1.0f, 1.5f },     [MOB_BROOD] = { 1.5f, 1.0f, 1.25f, 0.5f, 1.0f, 1.0f },
    [MOB_LING] = { 1.5f, 1.5f, 1.5f, 0.75f, 1.0f, 1.0f },        [MOB_SAC] = { 1.5f, 1.0f, 1.0f, 0.5f, 0.0f, 1.0f },
    [MOB_SHADE] = { 1.25f, 0.5f, 1.0f, 0.0f, 1.5f, 0.75f },      [MOB_IMP] = { 0.5f, 1.5f, 1.0f, 1.0f, 1.0f, 1.0f },
    [MOB_DEACON] = { 1.25f, 1.0f, 1.0f, 0.75f, 0.75f, 1.25f },   [MOB_GARGOYLE] = { 0.5f, 0.5f, 1.5f, 0.0f, 0.0f, 1.5f },
    [MOB_WHIPPER] = { 1.0f, 0.75f, 1.25f, 0.5f, 0.75f, 1.25f },  [MOB_SEXTON] = { 1.5f, 1.0f, 1.0f, 0.75f, 1.0f, 1.0f },
    [MOB_LIGHTLESS] = { 0.75f, 1.25f, 1.0f, 0.0f, 0.75f, 1.5f }, [MOB_RAT] = { 1.5f, 1.0f, 1.0f, 0.5f, 1.0f, 1.0f },
};
_Static_assert(sizeof OLD_WEAK / sizeof OLD_WEAK[0] == sizeof KINDS / sizeof KINDS[0], "a row for every kind");
#endif
static float kind_weak(int k, int el) {                            /* what an element does to a kind, before its rank's strengths */
#ifdef CRAWL_DEBUG
    if (!elem_mode()) return OLD_WEAK[k][el];
#endif
    return KINDS[k].weak[el];
}
/* How much a spell bites a creature: its kind's weakness or strength in the spell's element, its rank's strengths, and
 * each element worn added in by how much that element hurts it. The BFS9000 is no element's: it hurts anything that can
 * be hurt the same. */
static int spirit_mastery(void);
static float ward_str(void);
static float weak_el(const Mob *m, int el) {
    if (m->type == MOB_GHOST) return el == EL_SPIRIT && spirit_mastery() ? 1.0f : 0.0f;   /* a spectre: Spirit Mastery's alone */
    if (m->type == MOB_WRAITH && m->boss) return el == EL_FIRE ? 1.2f : 1.0f;   /* the Wraith Unbound, in light */
    if (g_drawn) return 1.0f;                                      /* the drawn levels' dead take every element alike (T6 a) */
    int strong = m->ward[0] == el + 1 || m->ward[1] == el + 1;
    if (!elem_mode()) {                                            /* the old: a knight down to its breastplate has nothing left
                                                                      to keep fire off; a ward keeps 60% to 90% off (1.0.0b16) */
        float w = m->type == MOB_KNIGHT && el == EL_FIRE && knight_stage(m) == 2 ? 1.0f : kind_weak(m->type, el);
        return strong ? w * (1.0f - ward_str()) : w;
    }
    float w = kind_weak(m->type, el);
    return strong || m->elem == el + 1 ? fminf(w, STRENGTH_K) : w;   /* its rank's strength, or born to it (7.4, B6), never more than its kind's own */
}
static float weak_to(const Mob *m, int atk) {
    return ATK_ELEM[atk] < 0 ? (KIND_IS(m, KF_SPIRIT) ? 0.0f : 1.0f) : weak_el(m, ATK_ELEM[atk]);
}
static float hit_mult(const Mob *m, int atk) {
    const Gear *g = &RUN.gear;
    float k = weak_to(m, atk);
    if (atk == AT_BFS) return k;
    float adds = 0.0f;                                             /* "ADDS %", all six since 1.0.0b21 (G4) */
    for (int e = 0; e < NELEM; e++) adds += gear_total(g, st_adds(e)) / 100.0f * weak_el(m, e);
    k += adds;
    for (int e = 0; e < NELEM; e++)                                /* the Masteries' Additions (docs/PLAN-b11.md, sections 9-10) */
        if (e != ATK_ELEM[atk] && e != EL_POISON) k += align_add(e) * weak_el(m, e);
    return k;
}
/* Warded creatures (1.0.0b16, docs/PLAN-play.md section 6): some come warded against a line the caster holds, which then
 * does a small share of its damage (60% kept off on level 5, rising evenly to 90% by level 30); a fifth of its health
 * taken in that element cracks it. Packs share one. Given the first time it moves (ward_give), never saved. */
static float ward_str(void) { return 0.6f + 0.3f * clampf((float)(G.level - 5) / 25.0f, 0.0f, 1.0f); }
static int ward_mode(void) {                                       /* 0 none, 1 as made, 2 every creature (CRAWL_WARD) */
#ifdef CRAWL_DEBUG
    if (getenv("CRAWL_WARD")) return clampi(atoi(getenv("CRAWL_WARD")), 0, 2);
#endif
    return 1;
}
static int ward_pick(const Mob *m, int ward[2]) {                  /* which elements, from the lines held: 0 none */
    int held[NELEM], nh = 0, own = cls_el(), weakest = 0;
    for (int e = 0; e < NELEM; e++) if (align_open(e)) held[nh++] = e;
    if (nh < 2) return 0;                                          /* never every line held */
    float wk[NELEM];
    for (int e = 0; e < NELEM; e++) wk[e] = kind_weak(m->type, e);
    for (int e = 1; e < NELEM; e++) if (wk[e] > wk[weakest]) weakest = e;
    int cand[NELEM], nc = 0;
    for (int i = 0; i < nh; i++) {
        int e = held[i];
        if (wk[e] <= 0.05f || (e == weakest && wk[e] > 1.01f)) continue;   /* nothing to keep off; never what it is weakest to */
        if (nh == 2 && e == own) continue;                         /* with two lines the second is the one a ward teaches */
        cand[nc++] = e;
    }
    if (!nc) return 0;
    for (int i = 0; i < nc; i++) { int j = i + rnd(nc - i), t = cand[i]; cand[i] = cand[j]; cand[j] = t; }   /* ties at random */
    for (int i = 1; i < nc; i++) for (int j = i; j > 0 && wk[cand[j]] < wk[cand[j - 1]] - 0.01f; j--) { int t = cand[j]; cand[j] = cand[j - 1]; cand[j - 1] = t; }
    ward[0] = cand[0] + 1; ward[1] = 0;                            /* the one its nature already shrugs off most */
    if (nh >= 3 && nc >= 2 && nh - 2 >= 1 && (m->rank == RK_IV || m->rank == RK_V) && rnd(2)) ward[1] = cand[1] + 1;   /* two, from three lines */
    return 1;
}
/* A rank's strengths (docs/PLAN-combat.md 7.3, B3-B5): rank III and IV one, V two, the gilded one; up to n of the lines
 * held, never its weakness nor what its kind already shrugs off, with two lines never the class's own (the second is the
 * one a strength teaches), and always leaving a held line that bites it at least as usual. Those its pack chose come
 * first (pref), so a pack shares them where its kinds allow. Returns how many. */
static int rank_strengths(int rank) { return rank >= RK_V && rank != RK_GILDED ? 2 : rank >= RK_III ? 1 : 0; }
static int strength_pick(const Mob *m, int n, int born, const int *pref, int out[2]) {   /* born: 1 + the element it was born to */
    out[0] = out[1] = 0;
    int held[NELEM], nh = 0, own = cls_el(), cand[NELEM], nc = 0, bites = 0;
    for (int e = 0; e < NELEM; e++) if (align_open(e)) held[nh++] = e;
    if (nh < 2 || n <= 0) return 0;                                /* never against the only line held */
    for (int i = 0; i < nh; i++) {
        int e = held[i];
        float w = born == e + 1 ? fminf(kind_weak(m->type, e), STRENGTH_K) : kind_weak(m->type, e);
        if (w > STRENGTH_K + 0.01f) bites++;                       /* a held line it doesn't shrug off */
        if (w <= STRENGTH_K + 0.01f || w > 1.01f) continue;        /* shrugged off already, or what it is weak to */
        if (nh == 2 && e == own) continue;
        cand[nc++] = e;
    }
    for (int i = 0; i < nc; i++) { int j = i + rnd(nc - i), t = cand[i]; cand[i] = cand[j]; cand[j] = t; }
    for (int q = 1; pref && q >= 0; q--)                           /* the pack's to the front, its first first */
        for (int i = 0; i < nc; i++)
            if (pref[q] == cand[i] + 1) { int t = cand[i]; memmove(cand + 1, cand, (size_t)i * sizeof cand[0]); cand[0] = t; break; }
    int k = 0;
    for (int i = 0; i < nc && k < n && bites > 1; i++, bites--) out[k++] = cand[i] + 1;
    return k;
}
static void strength_give(Mob *m) {
    Pack *p = m->pack ? pack_of(m) : NULL;
    if (p && !p->wset) {                                           /* the pack's, chosen once for its leader, two deep */
        const Mob *lead = p->leader >= 0 ? &G.mobs[p->leader] : m;
        p->wset = 1;
        strength_pick(lead, 2, p->elem > 0 ? p->elem : 0, NULL, p->ward);
    }
    strength_pick(m, rank_strengths(m->rank), m->elem, p ? p->ward : NULL, m->ward);
#ifdef CRAWL_DEBUG
    if (m->ward[0]) fprintf(stderr, "EVENT strengths %s rank %d against %s%s%s t=%.2f\n", KINDS[m->type].name, m->rank, ELEM_NAME[m->ward[0] - 1],
                            m->ward[1] ? " and " : "", m->ward[1] ? ELEM_NAME[m->ward[1] - 1] : "", G.t);
#endif
}
/* Elemental creatures (docs/PLAN-combat.md 7.4, B6-B10): past rank I one may be born to an element; its attacks carry it
 * some of the time (the caster's track's, C3) and its eyes show it; it shrugs that element off, and its weakness stays. Every
 * kind with eyes and a blow (B7 b), never a spirit, the shade, the Lightless, the deacon, the Sexton, a boss or its adds, an
 * ally; the imp and the wall skull are fire by nature and born to nothing else. Rank II 15%, III 25%, IV 35%, V 45%, the
 * gilded never; any element but Physical, the run's two thrice as likely as the rest, never its kind's weakness. A pack shares
 * its leader's; a brood's sacs and spiderlings take their mother's. Given the first time it moves, with its strengths (so a
 * pack's ranks are settled), never saved. CRAWL_ELEM=e (1 Fire ... 5 Spirit): every one that can be, of that element. */
static int elem_kind(int type) {
    switch (type) {
    case MOB_SKEL: case MOB_BAT: case MOB_SHIELD: case MOB_SPIDER: case MOB_SHAMBLER: case MOB_DROWNED: case MOB_HOUND:
    case MOB_HUNTER: case MOB_GHOUL: case MOB_KNIGHT: case MOB_BROOD: case MOB_SAC: case MOB_LING: case MOB_GARGOYLE: case MOB_WHIPPER:
    case MOB_RAT:
        return 1;
    default: return 0;
    }
}
static int elem_may(const Mob *m, int e) {                         /* may m be born to element e (0-based) */
    return e >= 0 && e < EL_PHYS && elem_kind(m->type) && !m->boss && !m->add && !m->ally && kind_weak(m->type, e) <= 1.01f;
}
static int elem_roll(const Mob *m) {                               /* 1 + the element m is born to, or 0 */
    static const float CHANCE[NRANK] = { 0.0f, 0.15f, 0.25f, 0.35f, 0.45f, 0.0f };
    static const int8_t FAV[4][2] = { { EL_FIRE, EL_SPIRIT }, { EL_FROST, EL_POISON }, { EL_LIGHT, EL_POISON }, { EL_FIRE, EL_SPIRIT } };
    float ch = CHANCE[clampi(m->rank, 0, NRANK - 1)];
    if (ch <= 0.0f) return 0;
#ifdef CRAWL_DEBUG
    if (getenv("CRAWL_ELEM")) { int e = atoi(getenv("CRAWL_ELEM")) - 1; return elem_may(m, e) ? e + 1 : 0; }
#endif
    if (rndf() >= ch) return 0;
    const int8_t *fav = FAV[clampi(crawl_set(G.level), 0, 3)];
    float w[EL_PHYS], tot = 0.0f;
    for (int e = 0; e < EL_PHYS; e++) { w[e] = elem_may(m, e) ? (e == fav[0] || e == fav[1] ? 3.0f : 1.0f) : 0.0f; tot += w[e]; }
    float r = rndf() * tot;
    for (int e = 0; e < EL_PHYS; e++) if (w[e] > 0.0f && (r -= w[e]) < 0.0f) return e + 1;
    return 0;
}
#ifdef CRAWL_DEBUG
static int elem_born;
#endif
static void elem_give(Mob *m) {
    m->elem = 0;
    if (!elem_mode() || !elem_kind(m->type) || m->boss || m->add || m->ally) return;
    int forced = 0;                                                /* CRAWL_ELEM=e: every one that can be, mother or none */
#ifdef CRAWL_DEBUG
    forced = getenv("CRAWL_ELEM") != NULL;
#endif
    if ((m->type == MOB_SAC || m->type == MOB_LING) && !forced) {  /* her children: the nearest mother's */
        const Mob *mo = NULL;
        float bd = 12.0f;
        for (int i = 0; i < MAXMOBS; i++) {
            const Mob *o = &G.mobs[i];
            float d = hypotf(o->x - m->x, o->y - m->y);
            if (o->alive && o->type == MOB_BROOD && o->state == MS_WALK && o->wset && d < bd) { bd = d; mo = o; }
        }
        if (mo && elem_may(m, mo->elem - 1)) m->elem = mo->elem;
    } else if (m->pack && pack_of(m)) {                            /* a pack's: its leader's, rolled once */
        Pack *p = (Pack *)pack_of(m);
        if (!p->elem) {
            const Mob *lead = p->leader >= 0 && elem_kind(G.mobs[p->leader].type) ? &G.mobs[p->leader] : m;
            int e = elem_roll(lead);
            p->elem = e ? e : -1;
        }
        if (p->elem > 0 && m->rank > RK_I && m->rank != RK_GILDED && elem_may(m, p->elem - 1)) m->elem = p->elem;
    } else m->elem = m->rank != RK_GILDED ? elem_roll(m) : 0;
#ifdef CRAWL_DEBUG
    if (m->elem) { elem_born++; fprintf(stderr, "EVENT born %s rank %d of %s t=%.2f\n", KINDS[m->type].name, m->rank, ELEM_NAME[m->elem - 1], G.t); }
#endif
}
static void ward_give(Mob *m) {
    m->wset = 1; m->ward[0] = m->ward[1] = 0;
    elem_give(m);                                                  /* born to an element, or not (1.0.0b19) */
    int mode = ward_mode();
    if (!mode || m->boss || m->add || m->ally || m->tri || KIND_IS(m, KF_SPIRIT)
        || m->type == MOB_GHOST || m->type == MOB_WSKULL || m->type == MOB_SAC || m->type == MOB_LING || m->type == MOB_WRAITH) return;
    if (elem_mode()) { strength_give(m); return; }                 /* 1.0.0b19: by rank, in place of wards */
    if (G.level < 5 && crawl_loop(G.level) == 0) return;
    Pack *p = m->pack ? pack_of(m) : NULL;
    if (p && p->wset) { m->ward[0] = p->ward[0]; m->ward[1] = m->rank >= RK_IV && m->rank != RK_GILDED ? p->ward[1] : 0; if (!m->ward[1] && p->ward[1] && !m->ward[0]) m->ward[0] = p->ward[1]; return; }
    const Mob *lead = p && p->leader >= 0 ? &G.mobs[p->leader] : m;
    static const int ONE_IN[4] = { 6, 4, 3, 3 };                   /* about one in six on run I, four on II, three on III and IV */
    float chance = 1.0f / (float)ONE_IN[clampi(crawl_set(G.level), 0, 3)];
    if (p || lead->rank >= RK_III) chance *= 1.5f;                 /* leaders, rank III and up and the gilded more often */
    int w[2] = { 0, 0 };
    if ((mode == 2 || rndf() < chance) && ward_pick(lead, w)) { m->ward[0] = w[0]; m->ward[1] = m->rank >= RK_IV && m->rank != RK_GILDED ? w[1] : 0; }
    if (p) { p->wset = 1; p->ward[0] = w[0]; p->ward[1] = w[1]; }
#ifdef CRAWL_DEBUG
    if (m->ward[0]) fprintf(stderr, "EVENT warded %s rank %d against %s%s%s t=%.2f\n", KINDS[m->type].name, m->rank, ELEM_NAME[m->ward[0] - 1],
                            m->ward[1] ? " and " : "", m->ward[1] ? ELEM_NAME[m->ward[1] - 1] : "", G.t);
#endif
}
#ifdef CRAWL_DEBUG
static int ward_hits, wards_cracked;
#endif
/* a rank's strength is shown (its rim, the barred glyph over it, the readout) once it has been seen struck (B11): on it,
 * and on those of its pack that share it */
static int strength_shown(const Mob *m, int i) { return m->ward[i] && (!elem_mode() || (m->wseen >> i & 1)); }
static void strength_seen(Mob *m, int i) {
    for (int j = 0; j < MAXMOBS; j++) {
        Mob *o = &G.mobs[j];
        if (o != m && (!m->pack || o->pack != m->pack || !o->alive)) continue;
        for (int q = 0; q < 2; q++) if (o->ward[q] == m->ward[i]) o->wseen |= 1 << q;
    }
}
static void ward_struck(Mob *m, int el, float dmg, int tick) {
    for (int i = 0; i < 2; i++) {
        if (m->ward[i] != el + 1) continue;
        m->wdmg[i] += dmg;
        if (!tick) {
            if (m->wring <= 0.0f) sfx_at(SFX_KNOCK, m->x, m->y, 0.9f);   /* a dull ring, as a boss's shell gives */
            m->wring = 0.35f;
#ifdef CRAWL_DEBUG
            ward_hits++;
#endif
            if (elem_mode()) strength_seen(m, i);
            if (G.player && !(G.ward_said >> el & 1)) { G.ward_said |= 1 << el; say(elem_mode() ? "THIS ONE SHRUGS OFF %s" : "WARDED AGAINST %s", ELEM_NAME[el]); }
        }
        if (m->wdmg[i] >= 0.2f * m->maxhp) {                       /* cracked (Q7 a): only slower, never locked */
            m->ward[i] = 0; m->wring = 0.0f;
            sfx_at(SFX_GLASS, m->x, m->y, 1.2f); sfx_at(SFX_SHATTER, m->x, m->y, 0.6f);
            if (elem_mode() && G.player && !(G.ward_said >> (el + 8) & 1)) {   /* once a level an element (the upper bits) */
                G.ward_said |= 1 << (el + 8);
                say("CRACKED: %s BITES IT AGAIN", ELEM_NAME[el]);
            }
#ifdef CRAWL_DEBUG
            fprintf(stderr, "EVENT ward_cracked %s %s t=%.2f\n", KINDS[m->type].name, ELEM_NAME[el], G.t);
            wards_cracked++;
#endif
        }
    }
}
static float sap_slow(void);
static float tri_wading(void);
static float slow_factor(void) {                                   /* slowed, the caster walks at 60%; resisted, less slowly; */
    if (G.web_hp > 0.0f) return 0.0f;                              /* held in the Wandering Spider's web: not at all */
    if (G.snare_t > 0.0f) return 0.0f;                             /* or a snare's (1.0.0b13) */
    if (af_rooted()) return 0.0f;                                  /* frozen, or on the floor */
    return sap_slow() * (G.slow_t > 0.0f ? 1.0f - 0.4f * (1.0f - 0.5f * gear_slow_resist(&RUN.gear)) : 1.0f) * (G.hz_slow > 0.0f ? G.hz_slow : 1.0f)   /* bile and void besides, */
         * tri_wading()                                               /* and a trial's water, */
         * (1.0f - 0.08f * (float)G.af_n)                             /* and the cold: Chilled, 8% a stack (S4), */
         * (G.rst_on ? 0.5f : 1.0f);                                   /* and Restoration channelled: half pace (D5, 10.3) */
}
static float spell_effect(void) { return 1.0f + gear_total(&RUN.gear, ST_EFFECT) / 100.0f; }   /* all a spell does besides damage */

/* A hit on a creature, from (fx, fy) (shields only stop what comes from in front). Returns 1 if it died. */
static int boss_hurt(Mob *m, float dmg);
static int wraith_open(const Mob *m);
static void ward_struck(Mob *m, int el, float dmg, int tick);
static void mob_chill(Mob *m, int stacks, int need);
static int chill_need(void);
static void el_status(Mob *m, int atk, float dmg);
static void mk_wet_hit(const Mob *o);
static void add_beam(int kind, float x0, float y0, float z0, float x1, float y1, float z1);
static int damage_mob(Mob *m, float dmg, int flags, float fx, float fy, float slow);
static void wire_pass(Mob *m, float dmg) {                         /* Live Wire (G5): a stunned creature struck passes a fifth of the hit
                                                                      to the nearest within two cells, as a spark that rolls nothing */
    static int passing;
    if (passing || cls_ab(EL_LIGHT, 0) <= 0.0f || dmg <= 0.0f) return;
    int best = -1; float bd = 2.0f;
    for (int i = 0; i < MAXMOBS; i++) {
        Mob *o = &G.mobs[i];
        if (o == m || !o->alive || o->state != MS_WALK || o->ally || KIND_IS(o, KF_SPIRIT) || buried(o) || o->boss) continue;
        float d = hypotf(o->x - m->x, o->y - m->y);
        if (d < bd && line_clear(m->x, m->y, o->x, o->y)) { bd = d; best = i; }
    }
    if (best < 0) return;
    Mob *o = &G.mobs[best];
    add_beam(AT_CHAIN, m->x, m->y, m->z + 0.5f, o->x, o->y, o->z + 0.5f);
    sfx_at(SFX_CRACKLE, o->x, o->y, 1.4f);
    passing = 1;
    damage_mob(o, dmg * 0.2f, DM_ARC | DM_TICK, m->x, m->y, 0.0f);
    passing = 0;
    DBG_COUNT(wire_arcs);
}
static int damage_mob(Mob *m, float dmg, int flags, float fx, float fy, float slow) {
    if (m->ally) return 0;                                         /* yours: your spells pass it by */
    if (G.latch == (int)(m - G.mobs) + 1) return 0;                /* nor what is latched on your face */
    if (m->boss && m->alive && m->state == MS_WALK && dmg > 0.0f) {   /* a boss: its weaknesses, then its marks */
        if (G.bdash_t > 0.0f) return 0;                            /* darting: nothing touches it */
        if (m->type == MOB_HUNTER && (m->sub == 0 || m->sub == 5)) return 0;   /* overhead, out of reach */
        if (!wraith_open(m)) return 0;                             /* the Wraith: in the dark, through it */
        int a = (flags >> 8) - 1;
        if (a >= 0 && a < NATK) { dmg *= hit_mult(m, a); m->last_atk = a; }
        if ((flags & DM_SLOW) && slow > 0.0f) m->slowed = fmaxf(m->slowed, slow * 0.5f);
        m->misses = 0;
        if (m->type == MOB_DEACON && G.bstate == BS_FIGHT && G.bward > 0.0f) {   /* the Bone Ward takes the first blows */
            float ab = fminf(G.bward, dmg);
            G.bward -= ab; dmg -= ab;
            if (G.bward <= 0.0f) { G.bward = 0.0f; G.bward_cd = def_cooldown(Q_RARE + 1); sfx_at(SFX_SHATTER, m->x, m->y, 1.3f); DBG_COUNT(ward_broken); }
            if (dmg <= 0.0f) { sfx_at(SFX_BONES, m->x, m->y, 0.5f); return 0; }
        }
        if (m->type == MOB_DEACON && G.bstate == BS_FIGHT && m->swing > 0.0f && (G.bchant_dmg += dmg) >= 0.1f * m->maxhp) {
            m->swing = 0.0f; m->deco = -1; m->contact = 3.0f;         /* a tenth of his health mid-chant breaks it off */
            sfx_at(SFX_GROAN, m->x, m->y, 1.0f);
            DBG_COUNT(chant_broken);
        }
        if (G.bstate == BS_FIGHT) G.bhurt += dmg;
        return boss_hurt(m, dmg);
    }
    if (m->type == MOB_WRAITH && m->alive && m->state == MS_WALK) {   /* nothing kills it, but a spell drives it back */
        int a = (flags >> 8) - 1;
        if (a == AT_FIRE || a == AT_HELL || a == AT_DRAIN || a == AT_WISPS) wraith_back(m);   /* fire, and spirit, five seconds; */
        else if (a >= 0) wraith_back_for(m, 3.0f);                 /* any other spell three (1.0.0b16: no one has fire by right) */
        m->misses = 0;
        return 0;
    }
    int spectre = m->type == MOB_GHOST && spirit_mastery() && ((flags >> 8) - 1 == AT_DRAIN || (flags >> 8) - 1 == AT_WISPS);   /* Spirit slays spectres */
    if (!m->alive || m->state != MS_WALK || (KIND_IS(m, KF_SPIRIT) && !spectre) || dmg <= 0.0f || buried(m)) return 0;
    int moment = (m->type == MOB_GHOUL && m->sub == 4) || (m->type == MOB_HUNTER && m->sub <= 1);   /* at its meal, or overhead: a feat if
                                                                      this blow is the last (the portraits') */
    if (G.player && !(flags & DM_TICK)) { G.look_mob = (int)(m - G.mobs); G.look_t = fmaxf(G.look_t, 3.0f); }   /* what you hit is your target a while */
    if (m->type == MOB_SHIELD && m->shield > 0.0f && !(flags & (DM_PIERCE | DM_ARC))) {
        float a = atan2f(fy - m->y, fx - m->x);
        if (fabsf(wrap_pi(a - m->face)) < 1.25f) {
            float ab = fminf(m->shield, dmg);
            m->shield -= ab; dmg -= ab;
            if (m->shield <= 0.0f) { sfx_at(SFX_SHATTER, m->x, m->y, 1.0f); m->feat |= FT_BROKE; }   /* splinters */
            else sfx_at(SFX_STEP, m->x, m->y, 1.4f);                       /* a thud on the wood */
            m->misses = 0;                                         /* a blow on the shield still counts */
            if (dmg <= 0.0f) { m->stagger = fmaxf(m->stagger, 0.2f); return 0; }
        }
    }
    if ((flags & DM_SLOW) && slow > 0.0f) {                        /* Frost's slow: a Chilled stack (D4, section 6) */
        if (elem_mode()) mob_chill(m, 1 + (st_odds(EL_FROST) > 0.0f && rndf() < st_odds(EL_FROST)), chill_need());   /* Rime: a second (G3) */
        else m->slowed = fmaxf(m->slowed, slow);
    }
    int atk = (flags >> 8) - 1;
    if (atk >= 0 && atk < NATK && !(flags & DM_TICK) && atk != AT_BFS) {   /* the Masteries' Additions that aren't damage */
        if (ATK_ELEM[atk] != EL_FROST && align_add(EL_FROST) > 0.0f) m->slowed = fmaxf(m->slowed, 0.6f);   /* Frost's: a little slowing */
        if (ATK_ELEM[atk] != EL_POISON && align_add(EL_POISON) > 0.0f) mob_poison(m, dmg * align_add(EL_POISON), 4.0f);   /* Poison's: a dose */
    }
    if (atk >= 0 && atk < NATK) { dmg *= hit_mult(m, atk); m->last_atk = atk; if (!(flags & DM_TICK)) rec_hit(m, atk); }   /* what it is weak to, or shrugs off */
    if (atk >= 0 && atk < NATK) aug_hit(m, atk, flags, &dmg);      /* the line's augments (17.3, 1.0.0b21) */
    if (m->pois_t > 0.0f && cls_ab(EL_POISON, 0) > 0.0f) dmg *= 1.0f + cls_ab(EL_POISON, 0) / 100.0f;   /* Fever: the poisoned take more (G5) */
    if (m->pois_t > 0.0f && atk >= 0 && atk < NATK && ATK_ELEM[atk] != EL_POISON && dk(EL_POISON, 2, 2) > 0.0f) dmg *= 1.0f + 0.15f * dk(EL_POISON, 2, 2);   /* Rot (D7) */
    if (m->unb_t > 0.0f && dk(EL_SPIRIT, 2, 1) > 0.0f) dmg *= 1.0f + 0.3f * dk(EL_SPIRIT, 2, 1);   /* Exorcism: the Unbound take more (D7) */
    if (atk >= 0 && atk < NATK && ATK_ELEM[atk] >= 0) ward_struck(m, ATK_ELEM[atk], dmg, flags & DM_TICK);   /* a ward: a dull ring, and it may crack */
    if (atk >= 0 && atk < NATK && ATK_ELEM[atk] == EL_LIGHT && !(flags & DM_TICK)) mk_wet_hit(m);   /* standing in the water */
    if (m->frozen > 0.0f && elem_mode()) dmg *= 1.33f;             /* Frozen: a third more (docs/PLAN-combat.md section 5) */
    if (m->frozen > 0.0f && cls_ab(EL_FROST, 0) > 0.0f) dmg *= 1.0f + cls_ab(EL_FROST, 0) / 100.0f;   /* the Slab: more yet (G5) */
    if (m->frozen > 0.0f && dk(EL_FROST, 2, 2) > 0.0f) dmg *= 1.0f + 0.25f * dk(EL_FROST, 2, 2);   /* Brittle (D7) */
    if (m->frozen > 0.0f && RUN.gear.def[DF_NOVA] >= RUNE_EXQ) { dmg *= 2.0f; DBG_COUNT(exq_shatter); }   /* Exquisite runes: the frozen */
    static int shattering;                                         /* Shatter: the frozen struck by a spell break, and their shards cut */
    int shards = 0;
    if (m->frozen > 0.0f && !shattering && atk >= 0 && atk < NATK && atk != AT_BFS && !(flags & DM_TICK) && align_has(EL_FROST, ND_BULT)) {
        dmg *= 2.0f; m->frozen = 0.0f; shards = 1;
    }
    if (G.slip_t > 0.0f && RUN.gear.def[DF_SLIP] >= RUNE_EXQ && hypotf(m->x - G.px, m->y - G.py) < 7.0f) { dmg *= 1.25f; DBG_COUNT(exq_bruise); }   /* shatter, the slowed bruise */
    if (m->type == MOB_GHOUL && m->sub == 4) { dmg *= 2.0f; m->sub = 0; }   /* caught feeding */
    if (m->type == MOB_GARGOYLE) garg_wake(m);                     /* it was never a statue */
    if (m->type == MOB_HUNTER && m->sub == 0) { m->sub = 1; m->sub_t = 0.0f; }   /* hit, it drops from the ceiling */
    m->misses = 0;
    if (minded(m) && !(flags & DM_TICK) && m->alert <= 0.0f && (m->aware == MA_CALM || m->aware == MA_WARY) && !in_view(m, G.px, G.py)) {
        dmg *= 1.5f;                                               /* caught unaware, from where it wasn't looking (S5) */
        if (m->type != MOB_BAT && m->type != MOB_GARGOYLE && m->type != MOB_HUNTER && m->type != MOB_BROOD) {
            m->stagger = fmaxf(m->stagger, 0.6f); m->throw_t = 0.0f; m->swing = 0.0f;
        }
        DBG_COUNT(unaware_hits);
        if (m->type == MOB_SEXTON) { m->ring_cd = fmaxf(m->ring_cd, 2.5f); if (m->rings == 1) { m->ring_t = 0.0f; m->rings = 0; } }   /* caught
                                                                      unaware, the bell is lowered a moment */
    }
    if (m->type == MOB_SEXTON) {
        if (atk >= 0 && atk < NATK && ATK_ELEM[atk] == EL_FROST && !(flags & DM_TICK)) {   /* a frost hit: the bell frozen six seconds */
            if (m->ice_t <= 0.0f) DBG_COUNT(bells_frozen);
            m->ice_t = 6.0f; m->ring_t = 0.0f; m->rings = 0;
        } else if (minded(m) && m->ring_cd <= 0.0f && m->stagger <= 0.0f) { m->seen_t = (float)G.level_t; sexton_ring(m, 1); }   /* struck: he
                                                                      rings for them */
    }
    m->alert = fmaxf(m->alert, minded(m) ? 6.0f : 12.0f);          /* hurt, it knows where you are (a while, as the dead think now) */
    if (m->sleep == 1) { m->sleep = 2; m->sub_t = 0.0f; }          /* a roost's bat, struck: down it comes */
    if (m->pack) { Pack *cp = (Pack *)pack_of(m); if (cp) cp->chill_t = 0.0f; }   /* struck, it forgets the Wraith */
    if (m->pack && !m->fetch) pack_call(m);                        /* and it calls its pack */
    if (m->amb && m->amb <= G.namb && !G.amb[m->amb - 1].sprung) {   /* an ambush's */
        if (!m->bait_st) amb_spring(m->amb - 1, "struck");
        else if (m->bait_st == B_WAIT || m->bait_st == B_SHOW || (m->bait_st == B_GILD && m->feared > 0.0f)) {
            m->bait_st = B_LURE; m->feared = 0.0f; m->alert = 0.0f; m->bait_t = 0.0f; DBG_COUNT(bait_run);   /* struck, it backs off to the room */
        }
    }
    if (m->rank == RK_GILDED && m->hp > m->maxhp * 0.5f && m->hp - dmg <= m->maxhp * 0.5f) { m->feared = 6.0f; m->wfeared = 0; }   /* the gilded run, with their gold */
    m->hp -= dmg;
    knight_shed(m);
    if (m->stun_t > 0.0f && atk >= 0 && atk < NATK && !(flags & DM_TICK)) wire_pass(m, dmg);   /* Live Wire: a fifth arcs on (G5) */
    if (shards) {                                                  /* everything within a cell and a half: half a shard, and a chill */
        shattering = 1;
        float sd = spell_damage(&RUN.gear, AT_FROST) * 0.5f;
        for (int i = 0; i < MAXMOBS; i++) {
            Mob *o = &G.mobs[i];
            if (o == m || !o->alive || o->state != MS_WALK || KIND_IS(o, KF_SPIRIT) || o->ally || buried(o)) continue;
            if (hypotf(o->x - m->x, o->y - m->y) < 1.5f && line_clear(m->x, m->y, o->x, o->y))
                damage_mob(o, sd, DM_ARC | DM_SLOW | DM_ATK(AT_FROST), m->x, m->y, 1.5f * permafrost());
        }
        shattering = 0;
        sfx_at(SFX_SHATTER, m->x, m->y, 1.2f);
        for (int b = 0; b < MAXBOOMS; b++)
            if (!G.booms[b].alive) { G.booms[b] = (Boom){ 1, m->x, m->y, 0.0f, 1.0f, 3 }; break; }
        DBG_COUNT(shatters);
    }
    if (m->hp <= 0.0f) {                                           /* (the augments, 17.3; Shepherd's pace for its motes spent, and a beam
                                                                      held on it let go) */
        if (atk >= 0 && atk < NATK) aug_kill(m, atk, flags);
        if (moment) m->feat |= FT_NOW;
        kill_mob(m); G.d7_shep = 0.0f;
        if (G.d7_lg == (int)(m - G.mobs) + 1) G.d7_lg = 0;
        return 1;
    }
    if (atk >= 0 && atk < NATK && !(flags & DM_TICK)) el_status(m, atk, dmg);   /* its element's status, by chance (D4) */
    if (m->type == MOB_SKEL || m->type == MOB_SHIELD) {            /* knocked back a step */
        m->stagger = 0.6f; m->throw_t = 0.0f; m->swing = 0.0f; m->cool = fmaxf(m->cool, 0.9f);
        sfx_at(SFX_BONES, m->x, m->y, 0.45f);
    }
    if (m->type == MOB_SPIDER && m->sub == 0) { m->sub = 1; m->sub_t = 0.0f; }   /* disturbed: it comes down */
    return 0;
}

/* Poison and burning on a creature (docs/PLAN-b11.md, sections 9-10): neither stacks; a stronger dose, or flames
 * walked through again, start it over */
#define PLAGUE_BITE 1.5f                    /* the Plague Doctor's poison, a tick, against anyone else's */
#define PLAGUE_STRIKE 1.25f                 /* and their flask's blow as it bursts (the owner, 1.0.0b16) */
static void mob_poison(Mob *m, float total, float secs) {
    if (!m->alive || m->state != MS_WALK || KIND_IS(m, KF_SPIRIT) || m->ally || secs <= 0.0f) return;
    if (st_odds(EL_POISON) > 0.0f && rndf() < st_odds(EL_POISON)) { total *= 1.5f; secs *= 1.5f; }   /* Blight: a dose half again (G3) */
    if (dk(EL_POISON, 0, 1) > 0.0f) { float b = 1.0f + 0.5f * dk(EL_POISON, 0, 1); total *= b; secs *= b; }   /* Thick Brew (D7) */
    if (cls_el() == EL_POISON) { total *= 1.2f * PLAGUE_BITE; secs *= 1.2f; }   /* the Plague Doctor's bites harder each tick (1.0.0b16:
                                                                                    the owner's, to set the class apart) and lasts a fifth longer */
    if (total <= m->pois_dps * fmaxf(0.0f, m->pois_t)) return;
    m->pois_t = secs; m->pois_dps = total / secs;
}
static void mob_burn(Mob *m, float dps, float secs) {
    if (!m->alive || m->state != MS_WALK || KIND_IS(m, KF_SPIRIT) || m->ally || secs <= 0.0f) return;
    if (dps * secs < m->burn_dps * fmaxf(0.0f, m->burn_t)) return;
    m->burn_t = secs; m->burn_dps = dps;
}
static void mob_afflict(Mob *m, float dt) {                        /* what wears on it, in small bites */
    if (m->state != MS_WALK) { m->pois_t = m->burn_t = 0.0f; return; }
    if (m->pois_t > 0.0f) {
        m->pois_t -= dt; m->pois_acc += m->pois_dps * dt;
        if (m->pois_acc >= 1.0f || m->pois_t <= 0.0f) { float d = m->pois_acc; m->pois_acc = 0.0f; if (d > 0.0f) damage_mob(m, d, DM_ARC | DM_TICK | DM_ATK(AT_FLASK), m->x, m->y, 0.0f); }
    }
    if (m->burn_t > 0.0f && m->state == MS_WALK) {
        m->burn_t -= dt; m->burn_acc += m->burn_dps * dt;
        if (m->burn_acc >= 1.0f || m->burn_t <= 0.0f) { float d = m->burn_acc; m->burn_acc = 0.0f; if (d > 0.0f) damage_mob(m, d, DM_ARC | DM_TICK | DM_ATK(AT_HELL), m->x, m->y, 0.0f); }
    }
    if (m->state == MS_WALK) aug_tick(m, dt);                      /* the augments' (17.3, 1.0.0b21) */
}

/* The dead's statuses (1.0.0b19, docs/PLAN-combat.md section 5): the caster's seven, as far as they mean anything to a
 * creature. On Fire is its burning (E2: a new ignite adds to what is left and starts the four seconds again). Chilled stacks
 * slow its whole clock, 18% a stack, and all fall off three seconds after the last; four freeze it three seconds (m->frozen:
 * Frozen, and it takes a third more, damage_mob), and thawing takes the stacks. Stunned (1.2 s, a boss a third as long)
 * breaks a wind-up, holds its blows, throws and calls, and halves its pace. Poisoned (its poison running, mob_poison)
 * doubles its wind-ups and waits. Unbound (1.5 s) holds its calls; the caster's track's attacks ask mob_unbound for the
 * rest (its element, the Bone Thrower, the charge). When a stun or a freeze ends the same can't land for three seconds
 * (S1, for the dead too: a default). Spectres, the Wraith, sacs, wall skulls, spirits, allies and the buried take none; a
 * boss only a shortened stun and the chill's slowing. Never saved. Knocked Down is the caster's alone. */
#define CHILL_SLOW 0.18f                    /* each stack of Chilled off a creature's clock */
#define CHILL_FREEZE 3.0f                   /* frozen by stacks, and how long a stack lasts after the last chilling */
#define AF_RELOCK 3.0f                      /* after a stun or a freeze, before the same can land again */
#ifdef CRAWL_DEBUG
static int maf_count[NAF];                                         /* statuses landed on the dead, by AF_* */
#endif
static int mob_takes_af(const Mob *m) {
    return m->alive && m->state == MS_WALK && !m->ally && !KIND_IS(m, KF_SPIRIT) && !buried(m) && m->type != MOB_GHOST
        && m->type != MOB_SAC && m->type != MOB_WSKULL && !(m->type == MOB_WRAITH && !m->boss) && elem_mode();
}
static void maf_note(const Mob *m, int af) {
#ifdef CRAWL_DEBUG
    static const char *const NM[NAF] = { "fire", "chill", "frozen", "stun", "poison", "unbound", "down" };
    maf_count[af]++;
    fprintf(stderr, "EVENT maf %s %s rank %d t=%.2f\n", NM[af], KINDS[m->type].name, m->rank, G.t);
#else
    (void)m; (void)af;
#endif
}
static void mob_ignite(Mob *m, float dmg) {                        /* On Fire: the hit again over four seconds, on what is left */
    if (!mob_takes_af(m) || dmg <= 0.0f) return;
    float left = m->burn_t > 0.0f ? m->burn_dps * m->burn_t : 0.0f;
    m->burn_t = 4.0f + (cls_ab(EL_FIRE, 0) > 0.0f ? 1.0f : 0.0f); m->burn_dps = (left + dmg) / 4.0f;   /* Pyrebrand: a second longer (G5) */
    maf_note(m, AF_FIRE);
}
static void mob_freeze(Mob *m, float secs) {                       /* Frozen: its clock stopped (mob_time), a third more taken */
    if (!mob_takes_af(m) || m->frz_cd > 0.0f || m->frozen > 0.0f || secs <= 0.0f) return;   /* never lengthened: no locks */
    if (m->boss) { m->slowed = fmaxf(m->slowed, secs); return; }   /* a boss is slowed instead, as Deep Chill has it */
    m->frozen = fmaxf(m->frozen, secs); m->swing = 0.0f; m->throw_t = 0.0f;
    sfx_at(SFX_FROST, m->x, m->y, 1.2f);
    maf_note(m, AF_FROZEN);
}
static int chill_need(void) { return align_has(EL_FROST, ND_ABIL) ? 3 : 4; }   /* Deep Chill freezes at three (D4, section 5) */
static void mob_chill(Mob *m, int stacks, int need) {              /* Chilled: stacks, frozen at need (four) */
    if (!mob_takes_af(m) || stacks <= 0 || m->frozen > 0.0f) return;
    m->chl_n += stacks; m->chl_t = 0.0f;
    maf_note(m, AF_CHILL);
    if (m->chl_n >= need) {
        if (m->frz_cd > 0.0f) m->chl_n = need - 1;                 /* thawed too lately: it stays at the edge */
        else {
            mob_freeze(m, CHILL_FREEZE * permafrost()); if (m->boss) m->chl_n = 0;   /* Permafrost longer, as its slowing was */
            if (m->frozen > 0.0f && dk(EL_FROST, 2, 1) > 0.0f && rndf() < dk(EL_FROST, 2, 1)) cold_snap(m);   /* Cold Snap (D7) */
        }
    }
}
static void mob_stun(Mob *m, float secs) {                         /* Stunned */
    if (!mob_takes_af(m) || m->stun_cd > 0.0f || m->stun_t > 0.0f || secs <= 0.0f) return;   /* never lengthened: no locks */
    if (m->boss) secs /= 3.0f;                                     /* a boss for a third as long, its ways kept */
    else { m->swing = 0.0f; m->throw_t = 0.0f; }                   /* a wind-up broken */
    sfx_at(SFX_CRACKLE, m->x, m->y, 1.3f);
    m->stun_t = secs;
    maf_note(m, AF_STUN);
}
static void mob_unbind(Mob *m, float secs) {                       /* Unbound: no element, no call */
    if (!mob_takes_af(m) || m->boss || secs <= 0.0f) return;
    if (m->unb_t <= 0.0f) sfx_at(SFX_WHISPER, m->x, m->y, 0.9f);
    m->unb_t = fmaxf(m->unb_t, secs);
    maf_note(m, AF_UNBOUND);
}
/* The elements on the caster's spells (1.0.0b19, docs/PLAN-combat.md section 6, E1): each hit that leaves it standing rolls its
 * element's status, 10% at the line's rank 1 rising evenly to 25% by rank 20, times Spell Effect, halved on what is strong
 * against the element and half again on what is weak to it. Reconciled, not doubled: Frost's slow is already a Chilled stack
 * on every hit, and the flask's poison Poisoned, so neither rolls; Physical gives the dead nothing (the spear's stagger and
 * Impale stay). Fire: the hit again over four seconds; Lightning: stunned 1.2 s; Spirit: Unbound 1.5 s. */
static float el_chance(const Mob *m, int el) {
    float r = (float)align_rank(el), ch = (0.10f + 0.15f * clampf((r - 1.0f) / 19.0f, 0.0f, 1.0f)) * spell_effect();
    ch *= 1.0f + st_odds(el) + cls_ab(el, 0) / 100.0f;            /* Ignition, Static, Hush; Pyrebrand, Live Wire, Swallowed Sin (1.0.0b21) */
    float w = weak_el(m, el);
    return w <= STRENGTH_K + 0.01f ? ch * 0.5f : w >= 1.49f ? ch * 1.5f : ch;
}
static void el_status(Mob *m, int atk, float dmg) {
    int el = ATK_ELEM[atk];
    if (!elem_mode() || el < 0 || el == EL_FROST || el == EL_POISON || el == EL_PHYS || !mob_takes_af(m)) return;
    if (rndf() >= el_chance(m, el) * aug_af(m, atk, AUGQ_CHANCE)) return;   /* the line's augments (17.3, 1.0.0b21) */
    if (el == EL_FIRE) mob_ignite(m, dmg * aug_af(m, atk, AUGQ_HARM));
    else if (el == EL_LIGHT) mob_stun(m, 1.2f);
    else if (el == EL_SPIRIT) mob_unbind(m, 1.5f);
}
static float mob_clock(const Mob *m) {                             /* what mob_time will make of the gate's dt, as things stand */
    if (m->frozen > 0.0f || m->pin > 0.0f) return 0.0f;
    float k = m->slowed > 0.0f ? 0.5f : 1.0f;
    if (G.slip_t > 0.0f && hypotf(m->x - G.px, m->y - G.py) < 7.0f) k *= 0.3f;
    return k;
}
/* the gate at the head of update_mob, before the kinds' own ways: the statuses run down, and it returns the dt the rest of
 * the creature's step runs on. A hold adds back what the kinds' own branches will take off (cool, swing, throw_t, a call) */
static float mob_gate(Mob *m, float dt) {
    if (m->state != MS_WALK || !elem_mode()) { m->stun_t = m->unb_t = 0.0f; m->chl_n = 0; return dt; }
#ifdef CRAWL_DEBUG
    if (getenv("CRAWL_MAF")) {                                     /* CRAWL_MAF=k: status k (AF_*) on what is in sight within nine
                                                                      cells, every 2.5 seconds (a stack of Chilled every 0.7) */
        int k = clampi(atoi(getenv("CRAWL_MAF")), 0, NAF - 1);
        float every = k == AF_CHILL ? 0.7f : 2.5f;
        static int last[MAXMOBS];                                  /* the period each was last given it in */
        int now = (int)floor(G.t / every), mi = (int)(m - G.mobs);
        if (now != last[mi] && hypotf(m->x - G.px, m->y - G.py) < 9.0f && line_clear(G.px, G.py, m->x, m->y) && (last[mi] = now, 1))
            switch (k) {
            case AF_FIRE: mob_ignite(m, 0.1f * m->maxhp); break;
            case AF_CHILL: mob_chill(m, 1, chill_need()); break;
            case AF_FROZEN: mob_freeze(m, 3.0f); break;
            case AF_STUN: mob_stun(m, 1.2f); break;
            case AF_POISON: mob_poison(m, 0.1f * m->maxhp, 4.0f); if (mob_takes_af(m)) maf_note(m, AF_POISON); break;
            case AF_UNBOUND: mob_unbind(m, 1.5f); break;
            default: break;
            }
    }
#endif
    if (m->stun_cd > 0.0f) m->stun_cd -= dt;
    if (m->frz_cd > 0.0f) m->frz_cd -= dt;
    if (m->frozen > 0.0f) m->frz_was = 1;
    else if (m->frz_was) { m->frz_was = 0; m->chl_n = 0; m->frz_cd = AF_RELOCK; }   /* thawed: the stacks are spent */
    if (m->chl_n > 0 && m->frozen <= 0.0f && (m->chl_t += dt) >= CHILL_FREEZE + (cls_ab(EL_FROST, 0) > 0.0f ? 1.0f : 0.0f)) m->chl_n = 0;   /* they
                                                                      fall off together; the Slab holds them a second more */
    float k = 1.0f;
    if (m->chl_n > 0 && m->frozen <= 0.0f) k *= fmaxf(0.2f, 1.0f - CHILL_SLOW * (float)m->chl_n);
    int stun = m->stun_t > 0.0f, unb = m->unb_t > 0.0f;
    if (stun) {
        k *= 0.5f;                                                 /* half pace */
        m->cool = fmaxf(m->cool, m->stun_t + 0.1f);                /* no blow or throw begun */
        if ((m->stun_t -= dt) <= 0.0f) { m->stun_t = 0.0f; m->stun_cd = AF_RELOCK; }
    }
    if (unb && (m->unb_t -= dt) <= 0.0f) m->unb_t = 0.0f;
    float after = dt * k * mob_clock(m);                           /* what the branches will count down by */
    if (stun || unb) {                                             /* no call: the deacon's, the horn, the bell, a pack's, held */
        if (m->call_t > 0.0f) m->call_t += after;
        if (m->type == MOB_SEXTON && m->rings == 1 && m->ring_t > 0.0f) m->ring_t += after;
        Pack *p = m->pack ? pack_of(m) : NULL;
        if (p && p->state == PAS_CALL && p->caller == (int)(m - G.mobs) && p->call_t > 0.0f) p->call_t += dt;   /* pack_frame's own dt */
    }
    if (m->pois_t > 0.0f && !m->boss) {                            /* Poisoned: its wind-ups and waits at half speed */
        if (m->cool > 0.0f) m->cool += 0.5f * after;
        if (m->swing > 0.0f) m->swing += 0.5f * after;
        if (m->throw_t > 0.0f) m->throw_t += 0.5f * after;
        if (m->call_t > 0.0f) m->call_t += 0.5f * after;
    }
    return dt * k;
}

/* The lines' abilities (docs/PLAN-b11.md, sections 9-10, step 7) */
static float burning_time(void) {                                  /* Firestarter's burning: two seconds, Burning Intensity more, Spell Effect */
    return 2.0f * (1.0f + ALIGN[EL_FIRE].n[ND_A2].val * (float)RUN.al[EL_FIRE][ND_A2]) * spell_effect();
}
static float permafrost(void) {                                    /* slowing and freezing longer: Permafrost, the Anatomist */
    return (1.0f + ALIGN[EL_FROST].n[ND_A2].val * (float)RUN.al[EL_FROST][ND_A2]) * (cls_el() == EL_FROST ? 1.2f : 1.0f);
}
static void deep_chill(Mob *m) {                                   /* Deep Chill: the third shard in four seconds freezes it a second */
    if (!align_has(EL_FROST, ND_ABIL) || !m->alive || m->state != MS_WALK) return;
    if (elem_mode()) return;                                       /* 1.0.0b19: Deep Chill freezes at three stacks instead (chill_need) */
    if ((float)G.t - m->chill_t > 4.0f) { m->chill_n = 0; m->chill_t = (float)G.t; }
    if (++m->chill_n < 3) return;
    m->chill_n = 0;
    if (m->boss) m->slowed = fmaxf(m->slowed, 3.0f * permafrost());   /* a boss is slowed harder instead */
    else { m->frozen = fmaxf(m->frozen, 1.0f * permafrost()); m->swing = 0.0f; m->throw_t = 0.0f; }
    sfx_at(SFX_FROST, m->x, m->y, 1.2f);
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT deep_chill %s t=%.2f\n", KINDS[m->type].name, G.t);
#endif
}
static float trail_life(void) { return 0.5f * (1.0f + 2.0f * dk(EL_LIGHT, 2, 2)); }   /* half a second; Conductor's, three times (D7) */
static void ionize(float x0, float y0, float x1, float y1, float dmg, uint64_t struck) {   /* Ionization: a trail half a second */
    if (!align_has(EL_LIGHT, ND_ABIL)) return;
    for (int i = 0; i < MAXTRAILS; i++)
        if (!G.trails[i].alive) { G.trails[i] = (Trail){ 1, x0, y0, x1, y1, 0.0f, dmg * 0.5f, struck }; return; }
}
static void update_trails(float dt) {                              /* what crosses a trail is hurt once by it */
    for (int i = 0; i < MAXTRAILS; i++) {
        Trail *tr = &G.trails[i];
        if (!tr->alive) continue;
        if ((tr->t += dt) >= trail_life()) { tr->alive = 0; continue; }
        float vx = tr->x1 - tr->x0, vy = tr->y1 - tr->y0, l2 = vx * vx + vy * vy + 1e-4f;
        for (int k = 0; k < MAXMOBS; k++) {
            Mob *m = &G.mobs[k];
            if ((tr->hit >> k & 1) || !m->alive || m->state != MS_WALK || KIND_IS(m, KF_SPIRIT) || m->ally || buried(m)) continue;
            float u = clampf(((m->x - tr->x0) * vx + (m->y - tr->y0) * vy) / l2, 0.0f, 1.0f);
            if (hypotf(tr->x0 + vx * u - m->x, tr->y0 + vy * u - m->y) > 0.25f + KINDS[m->type].rad) continue;
            tr->hit |= 1ull << k;
            damage_mob(m, tr->dmg, DM_ARC | DM_ATK(AT_CHAIN), m->x, m->y, 0.0f);
            if (m->alive && m->state == MS_WALK && dk(EL_LIGHT, 2, 2) > 0.0f && rndf() < dk(EL_LIGHT, 2, 2)) { mob_stun(m, 1.2f); DBG_COUNT(aug_conducts); }   /* Conductor */
        }
    }
}
static void seep(float x, float y, float dose) {                   /* Seep: the splash leaves a pool two seconds, Lingering longer */
    if (!align_has(EL_POISON, ND_ABIL)) return;
    float life = 2.0f * (1.0f + ALIGN[EL_POISON].n[ND_A2].val * (float)RUN.al[EL_POISON][ND_A2]);
    for (int i = 0; i < MAXVENOM; i++)
        if (!G.venoms[i].alive) { G.venoms[i] = (Venom){ 1, x, y, 0.55f, 0.0f, life, dose, 0.0f, 0 }; return; }
}
static void update_venoms(float dt) {                              /* what wades in it is poisoned again */
    for (int i = 0; i < MAXVENOM; i++) {
        Venom *v = &G.venoms[i];
        if (!v->alive) continue;
        float pt = v->t;
        if ((v->t += dt) >= v->life) { v->alive = 0; continue; }
        if ((int)(pt * 2.0f) == (int)(v->t * 2.0f)) continue;      /* twice a second */
        for (int k = 0; k < MAXMOBS; k++) {
            Mob *m = &G.mobs[k];
            if (!m->alive || m->state != MS_WALK || KIND_IS(m, KF_SPIRIT) || m->ally || buried(m) || m->type == MOB_GARGOYLE) continue;
            if (hypotf(m->x - v->x, m->y - v->y) > v->r + KINDS[m->type].rad * 0.5f) continue;
            if (v->dose > 0.0f) mob_poison(m, v->dose, 4.0f);
            if (v->slow > 0.0f) m->slowed = fmaxf(m->slowed, v->slow);
        }
    }
}

static void thorns(Mob *m) {
    int t = gear_thorns(&RUN.gear);
    if (t > 0 && m->alive) damage_mob(m, (float)t, DM_ARC, G.px, G.py, 0.0f);
}

/* an Exquisite Bone Ward, broken: its shards fly out and cut everything within two cells */
static void ward_burst(void) {
    float dmg = G.ward_shards;
    G.ward_shards = 0.0f;
    for (int i = 0; i < MAXMOBS; i++) {
        Mob *m = &G.mobs[i];
        if (!m->alive || m->state != MS_WALK || KIND_IS(m, KF_SPIRIT) || buried(m) || m->ally) continue;
        if (hypotf(m->x - G.px, m->y - G.py) < 2.0f && line_clear(G.px, G.py, m->x, m->y)) damage_mob(m, dmg, DM_ARC, G.px, G.py, 0.0f);
    }
    for (int k = 0; k < 6; k++) {                                   /* to be seen going; they touch nothing */
        Shot *sh = new_shot(SHOT_REFLECT, G.px, G.py, G.ang + 0.3f + (float)k * (2.0f * PI_F / 6.0f), 7.0f, 0.0f);
        if (sh) { sh->tier = -1; sh->life = 0.3f; }
    }
    sfx_at(SFX_BONES, G.px, G.py, 1.4f);
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT ward_burst %.0f t=%.2f\n", dmg, G.t);
#endif
}

static void mice_blast(float x, float y, float r);
static void explode(float x, float y) {
    for (int i = 0; i < MAXBOOMS; i++)
        if (!G.booms[i].alive) { G.booms[i] = (Boom){ 1, x, y, 0.0f, 1.0f, 0 }; break; }
    sfx_at(SFX_EXPLODE, x, y, 1.0f);
    mice_blast(x, y, 1.0f);                                        /* it scatters the mice (docs/PLAN-mice.md) */
}
static void explode_big(float x, float y, float r) {
    for (int i = 0; i < MAXBOOMS; i++)
        if (!G.booms[i].alive) { G.booms[i] = (Boom){ 1, x, y, 0.0f, r, 1 }; break; }
    sfx_at(SFX_EXPLODE, x, y, 1.4f); sfx_at(SFX_BOOM, x, y, 0.6f);
    mice_blast(x, y, r);
}

/* everything within r of (x, y) takes dmg, but not `skip` (it took the direct hit) */
static void splash(float x, float y, float r, float dmg, const Mob *skip, int atk) {
    for (int i = 0; i < MAXMOBS; i++) {
        Mob *m = &G.mobs[i];
        if (m == skip || !m->alive || m->state != MS_WALK || KIND_IS(m, KF_SPIRIT) || m->ally) continue;
        if (hypotf(m->x - x, m->y - y) < r && line_clear(x, y, m->x, m->y)) damage_mob(m, dmg, DM_ARC | DM_ATK(atk), x, y, 0.0f);
    }
}

/* Wraiths drift through stone towards the caster, looming, until it comes close; then they recoil
 * with a shriek and flee back through the walls, fading as they go. Nothing can touch them, and
 * they touch nothing. */
static void update_ghost(Mob *m, float dt) {
    m->t += dt; m->st += dt; m->anim += dt * 1.6f;
    float dx = G.px - m->x, dy = G.py - m->y, d = sqrtf(dx * dx + dy * dy) + 1e-4f;
    m->z = 0.10f + 0.06f * sinf(m->t * 1.7f);
    if (m->state == MS_WALK && m->sub == 7 && d > 5.0f && m->st < 28.0f) {   /* hanging about a kill room's door, harmless */
        float tx = m->bx - m->x, ty = m->by - m->y, td = sqrtf(tx * tx + ty * ty) + 1e-4f, sp = td > 0.6f ? 0.6f : 0.0f;
        float weave = sinf(m->t * 0.6f + m->orbit) * 0.3f;
        m->x += (tx / td * sp - ty / td * weave) * dt; m->y += (ty / td * sp + tx / td * weave) * dt;
        if (d < 8.0f && rnd(300) == 0) sfx_at(SFX_MOAN, m->x, m->y, 0.7f);
    } else if (m->state == MS_WALK) {
        float sp = (d > 6.0f ? 0.75f : 0.5f) * (G.dead || G.exiting ? 0.0f : 1.0f);
        float weave = sinf(m->t * 0.6f + m->orbit) * 0.4f;
        m->x += (dx / d * sp - dy / d * weave) * dt;
        m->y += (dy / d * sp + dx / d * weave) * dt;
        if ((d < 2.1f && line_clear(m->x, m->y, G.px, G.py)) || d < 1.2f) {
            m->state = MS_RECOIL; m->st = 0.0f;
            sfx_at(SFX_SHRIEK, m->x, m->y, 1.4f);
            G.chill = 1.0f;
            DBG_COUNT(shrieks);
        } else if (m->st > 30.0f) { m->state = MS_FLEE; m->st = 0.0f; }   /* tired of waiting: melts away */
        if (d < 6.0f && rnd(500) == 0) sfx_at(SFX_MOAN, m->x, m->y, 0.9f);
    } else if (m->state == MS_RECOIL) {
        m->x -= dx / d * 0.6f * dt; m->y -= dy / d * 0.6f * dt;     /* reeling back */
        if (m->st > 0.55f) { m->state = MS_FLEE; m->st = 0.0f; }
    } else {                                                       /* fleeing, through whatever is in the way */
        m->x -= dx / d * 4.0f * dt; m->y -= dy / d * 4.0f * dt;
        m->fade += dt / 0.9f;
        if (m->fade >= 1.0f) m->alive = 0;
    }
    m->x = clampf(m->x, 0.5f, MAPN - 0.5f); m->y = clampf(m->y, 0.5f, MAPN - 0.5f);
    m->cx = m->tx = (int)m->x; m->cy = m->ty = (int)m->y;
}

/* The hunt: steps from every cell to the caster, rebuilt as it moves, which creatures that know where
 * it is follow round corners and through doors (a closed door counts: they open it; a locked one
 * doesn't). -1 where it is too far, or can't be walked. */
#define HUNT_FAR 64
static int16_t hunt[MAPN][MAPN];
static int hunt_cx = -1, hunt_cy = -1;
static float hunt_t;
static int hunt_pass(int x, int y) {
    const Cell *c = cell(x, y);
    if (c->kind == K_FLOOR) return !(G.slide >= 0 && slide_covers(x, y));
    if (c->kind == K_PROP) return foot_of(x, y) != 0;              /* round a prop that takes part of its cell (1.0.0b18.1) */
    return c->kind == K_DOOR && G.doors[c->door].locked != 1 && G.doors[c->door].locked != SEALED && G.doors[c->door].locked != WINGED && G.doors[c->door].locked != TRIALED && G.doors[c->door].locked != GATED;
}
static void build_hunt(float dt) {
    int cx = (int)G.px, cy = (int)G.py;
    hunt_t -= dt;
    if (cx == hunt_cx && cy == hunt_cy && hunt_t > 0.0f) return;   /* doors and walls move too: now and then, anyway */
    hunt_cx = cx; hunt_cy = cy; hunt_t = 0.5f;
    static int q[MAPN * MAPN];
    memset(hunt, 0xff, sizeof hunt);
    int qh = 0, qt = 0;
    hunt[cy][cx] = 0; q[qt++] = cy * MAPN + cx;
    while (qh < qt) {
        int x = q[qh] % MAPN, y = q[qh] / MAPN; qh++;
        if (hunt[y][x] >= HUNT_FAR) continue;
        for (int d = 0; d < 4; d++) {
            int nx = x + DX[d], ny = y + DY[d];
            if (nx < 1 || ny < 1 || nx >= MAPN - 1 || ny >= MAPN - 1 || hunt[ny][nx] >= 0 || !hunt_pass(nx, ny)) continue;
            hunt[ny][nx] = (int16_t)(hunt[y][x] + 1); q[qt++] = ny * MAPN + nx;
        }
    }
}
/* only the nearest few hunt at once, so the whole level doesn't arrive together */
#define MAX_HUNTERS 3
static void pick_hunters(void) {
    int best[MAX_HUNTERS], bd[MAX_HUNTERS], n = 0;
    for (int i = 0; i < MAXMOBS; i++) {
        Mob *m = &G.mobs[i];
        m->hunts = m->type == MOB_LING;                            /* a swarm comes all at once */
        if (!m->alive || m->state != MS_WALK || m->alert <= 0.0f || m->type == MOB_LING || m->ally || m->webbed > 0.0f) continue;
        if (minded(m)) { m->hunts = 1; continue; }                 /* the new minds: any number hunt; the fight is capped (engage_pick) */
        int h = hunt[m->cy][m->cx];
        if (h < 0) continue;
        int k = n < MAX_HUNTERS ? n++ : MAX_HUNTERS;
        if (k == MAX_HUNTERS) { if (h >= bd[MAX_HUNTERS - 1]) continue; k = MAX_HUNTERS - 1; }
        while (k > 0 && bd[k - 1] > h) { best[k] = best[k - 1]; bd[k] = bd[k - 1]; k--; }
        best[k] = i; bd[k] = h;
    }
    for (int i = 0; i < n; i++) G.mobs[best[i]].hunts = 1;
    for (int i = 0; i < MAXMOBS; i++) if (G.mobs[i].alive && (G.mobs[i].boss || G.mobs[i].add)) G.mobs[i].hunts = 1;   /* a boss's fight is all at once */
}

/* a noise: whatever is within so many steps knows where the caster is now */
static void alert_near(int steps, float secs) {
    if (G.wwalk_t > 0.0f) return;                                  /* Wraithwalk: they hear nothing of you */
    for (int i = 0; i < MAXMOBS; i++) {
        Mob *m = &G.mobs[i];
        if (!m->alive || m->state != MS_WALK) continue;
        int h = hunt[m->cy][m->cx];
        if (h >= 0 && h <= steps) m->alert = fmaxf(m->alert, secs);
    }
}

