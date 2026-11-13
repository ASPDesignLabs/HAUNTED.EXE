/* A part of scene_crawl.c (docs/PLAN-tidy.md): it is #included there in its place and never compiled on its own.
 * The spells' augments: the other five lines' (augd_*), then the frame and Fire's (aug_*, fire_*). */

/* ---- the dead's track (D) owns these bodies (docs/PLAN-combat.md, 17.2 c and 17.3 for 1.0.0b21): the augments of Frost,
 * Lightning, Poison, Spirit and Physical (section 11.4), e the line; stubs that change nothing till D7 ---- */
static int spell_tier(int sp);
static float spell_effect(void);
static float permafrost(void);
static int targetable(const Mob *m);
static int damage_mob(Mob *m, float dmg, int flags, float fx, float fy, float slow);
static void add_beam(int kind, float x0, float y0, float z0, float x1, float y1, float z1);
static void mob_chill(Mob *m, int stacks, int need);
static int chill_need(void);
static void mob_stun(Mob *m, float secs);
static float dk(int e, int t, int w) {
#ifdef CRAWL_DEBUG
    static int ov = -1, ch[3], rk = 3;                             /* CRAWL_AUGD=abc: the choice at each step (0 none, 1, 2) on every one
                                                                      of the five lines; CRAWL_AUGDR=r its rank, 1-3 (3) */
    if (ov < 0) {
        const char *a = getenv("CRAWL_AUGD"), *r = getenv("CRAWL_AUGDR");
        ov = a != NULL;
        for (int i = 0; i < 3 && a && a[i]; i++) ch[i] = a[i] - '0';
        if (r) rk = clampi(atoi(r), 1, 3);
    }
    if (ov && e > EL_FIRE && e < NELEM && t >= 0 && t < 3) return ch[t] == w ? (rk >= 3 ? 1.0f : rk == 2 ? 0.75f : 0.5f) : 0.0f;
#endif
    return aug_k(e, t, w);
}
static int aug_show(int k) {                                       /* CRAWL_AUGSHOW=k in the debug build, for the sheet: 1 everything in
                                                                      sight bleeding, 2 Static Build's three bolts over it, 3 the bursts
                                                                      and the cloud before you every two seconds, 9 all of them */
#ifdef CRAWL_DEBUG
    static int v = -1;
    if (v < 0) v = getenv("CRAWL_AUGSHOW") ? atoi(getenv("CRAWL_AUGSHOW")) : 0;
    return v && (v == k || v == 9);
#else
    (void)k; return 0;
#endif
}
static void d7_boom(float x, float y, int big, float r);
static void aug_show_tick(float dt) {                              /* CRAWL_AUGSHOW=3: Cold Snap's burst on the left, Thunderclap's on the
                                                                      right, Miasma's cloud between, before the caster */
    static float t;
    if (!aug_show(3) || G.shop || (t -= dt) > 0.0f) return;
    t = 2.0f;
    float ax = cosf(G.ang), ay = sinf(G.ang), lx = -ay, ly = ax;
    d7_boom(G.px + ax * 1.9f + lx * 0.8f, G.py + ay * 1.9f + ly * 0.8f, 3, 1.9f);
    d7_boom(G.px + ax * 1.9f - lx * 0.8f, G.py + ay * 1.9f - ly * 0.8f, 4, 1.5f);
    for (int i = 0; i < MAXVENOM; i++)
        if (!G.venoms[i].alive) { G.venoms[i] = (Venom){ 1, G.px + ax * 2.2f, G.py + ay * 2.2f, 0.9f, 0.0f, 1.5f, 0.0f, 0.0f, 2 }; break; }
}
static Mob *d7_near(const Mob *from, float x, float y, float reach, uint64_t skip) {   /* the nearest other within reach and in sight */
    Mob *best = NULL; float bd = reach;
    for (int k = 0; k < MAXMOBS; k++) {
        Mob *o = &G.mobs[k];
        if (o == from || (skip >> k & 1) || !targetable(o) || o->ally) continue;
        float d = hypotf(o->x - x, o->y - y);
        if (d < bd && line_clear(x, y, o->x, o->y)) { bd = d; best = o; }
    }
    return best;
}
static void d7_bolt(const Mob *from, float dmg, const char *what) {   /* Forked, Grounding: the bolt leaps on to one more */
    if (dmg <= 0.0f) return;
    Mob *o = d7_near(from, from->x, from->y, 3.0f, 0);
    if (!o) return;
    add_beam(AT_CHAIN, from->x, from->y, from->z + 0.5f, o->x, o->y, o->z + 0.5f);
    sfx_at(SFX_ZAP, o->x, o->y, 0.7f);
    damage_mob(o, dmg, DM_ARC | DM_ATK(AT_CHAIN), from->x, from->y, 0.0f);
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT aug_%s t=%.2f\n", what, G.t);
#else
    (void)what;
#endif
}
static void d7_boom(float x, float y, int big, float r) {
    for (int b = 0; b < MAXBOOMS; b++)
        if (!G.booms[b].alive) { G.booms[b] = (Boom){ 1, x, y, 0.0f, r, big }; return; }
}
static void thunderclap(Mob *m) {                                  /* the stunned struck: all within a cell and a half stunned */
    d7_boom(m->x, m->y, 4, 1.5f);
    sfx_at(SFX_CRACKLE, m->x, m->y, 1.0f);
    for (int k = 0; k < MAXMOBS; k++) {
        Mob *o = &G.mobs[k];
        if (o != m && o->alive && o->state == MS_WALK && hypotf(o->x - m->x, o->y - m->y) < 1.5f) mob_stun(o, 1.2f);
    }
    DBG_COUNT(aug_claps);
}
static void cold_snap(Mob *m) {                                    /* the frozen burst: a chill on all within a cell and a half */
    static int snapping;
    if (snapping) return;                                          /* one snap doesn't set off another */
    snapping = 1;
    d7_boom(m->x, m->y, 3, 1.9f);
    sfx_at(SFX_SHATTER, m->x, m->y, 0.7f);
    for (int k = 0; k < MAXMOBS; k++) {
        Mob *o = &G.mobs[k];
        if (o != m && o->alive && o->state == MS_WALK && hypotf(o->x - m->x, o->y - m->y) < 1.5f) mob_chill(o, 1, chill_need());
    }
    snapping = 0;
    DBG_COUNT(aug_snaps);
}
static void frost_splinter(const Shot *s, const Mob *mb, float dmg) {   /* Splinterfrost: three shards on behind what it struck */
    float a0 = atan2f(s->vy, s->vx), r = KINDS[mb->type].rad + 0.25f;
    float x = mb->x + cosf(a0) * r, y = mb->y + sinf(a0) * r;
    if (dmg <= 0.0f || blocks_light((int)x, (int)y)) return;
    for (int i = -1; i <= 1; i++) {
        Shot *n = new_shot(AT_FROST, x, y, a0 + 0.45f * (float)i, 8.0f, dmg);
        if (n) { n->tier = s->tier; n->target = -2; n->life = 0.45f; n->scale = 0.5f; }
    }
    DBG_COUNT(aug_splinters);
}
static void bone_rain(const Mob *m, float dmg) {                   /* Bone Rain: three splinters at the nearest */
    if (dmg <= 0.0f) return;
    Mob *t[3] = { NULL, NULL, NULL };
    uint64_t skip = 0;
    for (int i = 0; i < 3; i++) { t[i] = d7_near(m, m->x, m->y, 6.0f, skip); if (t[i]) skip |= 1ull << (int)(t[i] - G.mobs); }
    for (int i = 0; i < 3; i++) {
        const Mob *o = t[i] ? t[i] : t[0];
        float a = o ? atan2f(o->y - m->y, o->x - m->x) + (t[i] ? 0.0f : 0.25f * (float)(i - 1)) : rndf() * TAU_F;
        Shot *n = new_shot(SHOT_SPLINTER, m->x, m->y, a, 11.0f, dmg);
        if (n) n->tier = spell_tier(AT_SPEAR);
    }
    DBG_COUNT(aug_rains);
}
static void d7_bone(const Mob *m) {                                /* a bone where the spear's kill fell (Recovery) */
    if (G.map[m->cy][m->cx].kind != K_FLOOR) return;
    Item *it = add_item(m->cx, m->cy, IT_BONE);
    if (it) { it->secret = 1; it->seen = 1; DBG_COUNT(aug_bones); }
}
static float augd_cost(int e, int sp) {
    return e == EL_LIGHT && sp == AT_CHAIN && dk(EL_LIGHT, 0, 2) > 0.0f ? 0.25f : 0.0f;   /* Overcharge: a quarter more spirit */
}
static float augd_dmg(int e, int sp) {
    float k = 1.0f;
    if (e == EL_FROST && sp == AT_FROST && dk(EL_FROST, 0, 1) > 0.0f) k *= 0.8f;     /* Hoarbite: a fifth weaker */
    if (e == EL_LIGHT && sp == AT_CHAIN && dk(EL_LIGHT, 0, 2) > 0.0f) k *= 1.0f + 0.3f * dk(EL_LIGHT, 0, 2);   /* Overcharge */
    if (e == EL_SPIRIT && sp == AT_DRAIN && dk(EL_SPIRIT, 0, 1) > 0.0f) k *= 0.85f;  /* Siphon: it hurts less */
    return k;
}
static void augd_cast(int e, int sp, Shot *s, int t) {
    float k;
    switch (e) {
    case EL_FROST:
        if (s && sp == AT_FROST && (k = dk(EL_FROST, 1, 2)) > 0.0f) {   /* Glacial Weight: slower, heavier, through the first */
            s->vx *= 0.7f; s->vy *= 0.7f; s->life *= 1.45f; s->scale = 1.35f; s->target = -3;
        }
        return;
    case EL_LIGHT: {
        if (sp != AT_CHAIN || t < 0) return;
        const Mob *m = &G.mobs[t];
        if ((k = dk(EL_LIGHT, 0, 1)) > 0.0f) d7_bolt(m, G.d7_zap * 0.5f * k, "fork");   /* Forked */
        if ((k = dk(EL_LIGHT, 1, 2)) > 0.0f && (on_water(m->cx, m->cy) || m->pois_t > 0.0f)) d7_bolt(m, G.d7_zap * 0.7f * k, "ground");   /* Grounding */
        return;
    }
    case EL_POISON:
        if (s && sp == AT_FLASK && s->target < 1000 && (k = dk(EL_POISON, 0, 2)) > 0.0f) {   /* Straight Throw: flat and fast */
            float sp1 = 1.0f + 0.6f * k;
            s->vx *= sp1; s->vy *= sp1; s->arc = 0.0f; s->z = 0.0f; s->life = 1.2f;
        }
        return;
    case EL_SPIRIT: {
        if (sp != AT_DRAIN || t < 0) { G.d7_drained = 0.0f; return; }
        const Gear *g = &RUN.gear;
        int tier = spell_tier(AT_DRAIN);
        if ((k = dk(EL_SPIRIT, 0, 1)) > 0.0f && G.d7_drained > 0.0f && G.hp < G.max_hp) {   /* Siphon: more of it back */
            float heal = fminf(G.d7_drained * fminf(0.8f, 0.5f + 0.05f * tier) * spell_effect()
                               * (1.0f + ALIGN[EL_SPIRIT].n[ND_A2].val * (float)RUN.al[EL_SPIRIT][ND_A2]) * 0.3f * k, (float)(G.max_hp - G.hp));
            if (heal > 0.0f && spend_spirit(heal * heal_cost(g, tier))) G.hp = clampi(G.hp + (int)(heal + 0.5f), 0, G.max_hp);
        }
        if ((k = dk(EL_SPIRIT, 1, 1)) > 0.0f && G.d7_drained > 0.0f && G.mobs[t].alive && G.mobs[t].state == MS_WALK) {   /* Lingering Hunger */
            G.d7_lg = t + 1; G.d7_lg_t = 0.6f * k; G.d7_lg_dps = G.d7_drained * 0.5f / 0.6f; G.d7_lg_acc = 0.0f; G.d7_lg_beam = 0.0f;
            DBG_COUNT(aug_lingers);
        }
        G.d7_drained = 0.0f;
        return;
    }
    case EL_PHYS:
        if (s && sp == AT_SPEAR && (k = dk(EL_PHYS, 1, 1)) > 0.0f) {   /* Twin Throw: two side by side for one bone */
            float a = atan2f(s->vy, s->vx), v = hypotf(s->vx, s->vy), ox = -sinf(a) * 0.16f, oy = cosf(a) * 0.16f;
            s->dmg *= 0.3f + 0.3f * k;
            Shot *n = new_shot(AT_SPEAR, s->x + ox, s->y + oy, a, v, s->dmg);
            if (n) { n->tier = s->tier; n->target = s->target; }
            s->x -= ox; s->y -= oy;
            DBG_COUNT(aug_twins);
        }
        return;
    }
}
static void augd_hit(int e, Mob *m, int atk, int flags, float *dmg) {
    float k;
    if (flags & DM_TICK) return;
    switch (e) {
    case EL_FROST:
        if (atk == AT_FROST && (flags & DM_SLOW) && (k = dk(EL_FROST, 0, 1)) > 0.0f && rndf() < k) mob_chill(m, 1, chill_need());   /* Hoarbite */
        return;
    case EL_LIGHT:
        if (atk != AT_CHAIN) return;
        if (dk(EL_LIGHT, 1, 1) > 0.0f) {                           /* Static Build: each strike within three seconds counts */
            if (m->stb_t > 3.0f) m->stb_n = 0;
            m->stb_n = imin(m->stb_n + 1, 4); m->stb_t = 0.0f;
        }
        if ((k = dk(EL_LIGHT, 2, 1)) > 0.0f && m->stun_t > 0.0f && rndf() < k) thunderclap(m);   /* Thunderclap */
        return;
    case EL_SPIRIT:
        if (atk == AT_DRAIN) G.d7_drained = fminf(*dmg, fmaxf(m->hp, 0.0f));   /* what the drain takes, for Siphon and the hunger */
        return;
    case EL_PHYS:
        if (atk == AT_SPEAR && !G.d7_splint && (k = dk(EL_PHYS, 0, 1)) > 0.0f && *dmg > 0.0f) {   /* Barbed: half the hit again, bled */
            float left = m->bld_t > 0.0f ? m->bld_dps * m->bld_t : 0.0f;
            m->bld_t = 3.0f; m->bld_dps = (left + *dmg * 0.5f * k) / 3.0f;
        }
        return;
    }
}
static float augd_af(int e, const Mob *m, int atk, int q) {
    if (q != AUGQ_CHANCE) return 1.0f;
    if (e == EL_LIGHT && atk == AT_CHAIN)                          /* Overcharge doubles it; Static Build raises it a half a strike */
        return (1.0f + dk(EL_LIGHT, 0, 2)) * (1.0f + 0.5f * dk(EL_LIGHT, 1, 1) * (float)imax(0, m->stb_n - 1));
    if (e == EL_SPIRIT && atk == AT_DRAIN) return 1.0f + dk(EL_SPIRIT, 0, 2);   /* Hollowing */
    return 1.0f;
}
static void augd_kill(int e, Mob *m, int atk, int flags) {
    float k;
    switch (e) {
    case EL_SPIRIT:
        if (atk != AT_DRAIN) return;
        if ((k = dk(EL_SPIRIT, 1, 2)) > 0.0f) {                    /* Soul Echo: the cast's spirit back */
            gain_souls(spell_cost(&RUN.gear, AT_DRAIN, spell_tier(AT_DRAIN)) * (1.0f + aug_cost(AT_DRAIN)) * k);
            DBG_COUNT(aug_echoes);
        }
        if ((k = dk(EL_SPIRIT, 2, 2)) > 0.0f) G.d7_shep = 5.0f * k;   /* Shepherd: its motes come to you (spawn_mote) */
        return;
    case EL_PHYS:
        if (atk != AT_SPEAR || (flags & DM_TICK) || G.d7_splint) return;
        if ((k = dk(EL_PHYS, 0, 2)) > 0.0f && rndf() < 0.5f * k) d7_bone(m);   /* Recovery */
        if ((k = dk(EL_PHYS, 2, 2)) > 0.0f) bone_rain(m, G.d7_spear * 0.35f * k);   /* Bone Rain */
        return;
    }
}
static void augd_tick(Mob *m, float dt) {
    if (m->stb_n) { m->stb_t += dt; if (m->stb_t > 3.0f) m->stb_n = 0; }   /* Static Build fades */
    if (m->bld_t > 0.0f) {                                         /* Barbed's bleed */
        m->bld_t -= dt; m->bld_acc += m->bld_dps * dt;
        if (m->bld_acc >= 1.0f || m->bld_t <= 0.0f) {
            float d = m->bld_acc; m->bld_acc = 0.0f;
            if (m->bld_t <= 0.0f) m->bld_t = 0.0f;
            if (d > 0.0f && damage_mob(m, d, DM_ARC | DM_TICK | DM_ATK(AT_SPEAR), m->x, m->y, 0.0f)) return;
        }
    }
    if (G.d7_lg == (int)(m - G.mobs) + 1 && G.d7_lg_t > 0.0f) {   /* Lingering Hunger: the beam holds on it */
        if (G.dead || hypotf(m->x - G.px, m->y - G.py) > 9.0f || !line_clear(G.px, G.py, m->x, m->y)) { G.d7_lg = 0; return; }
        G.d7_lg_t -= dt; G.d7_lg_acc += G.d7_lg_dps * dt;
        if ((G.d7_lg_beam -= dt) <= 0.0f) {
            G.d7_lg_beam = 0.12f;
            add_beam(AT_DRAIN, G.px + cosf(G.ang) * 0.35f, G.py + sinf(G.ang) * 0.35f, 0.42f, m->x, m->y, m->z + 0.5f);
        }
        if (G.d7_lg_acc >= 1.0f || G.d7_lg_t <= 0.0f) {
            float d = G.d7_lg_acc; G.d7_lg_acc = 0.0f;
            if (G.d7_lg_t <= 0.0f) G.d7_lg = 0;
            if (d > 0.0f) damage_mob(m, d, DM_ARC | DM_TICK | DM_ATK(AT_DRAIN), G.px, G.py, 0.0f);
        }
    }
}
static void augd_fly(int e, Shot *s, float dt) {
    float k;
    if (e != EL_FROST || s->kind != AT_FROST || s->target == -2 || (k = dk(EL_FROST, 1, 1)) <= 0.0f) return;   /* Rime Trail */
    int i = (int)(s - G.shots);
    if (hypotf(s->x - G.lay_x[i], s->y - G.lay_y[i]) < 0.6f || G.map[clampi((int)s->y, 0, MAPN - 1)][clampi((int)s->x, 0, MAPN - 1)].kind != K_FLOOR) return;
    G.lay_x[i] = s->x; G.lay_y[i] = s->y;
    for (int v = 0; v < MAXVENOM; v++)
        if (!G.venoms[v].alive) { G.venoms[v] = (Venom){ 1, s->x, s->y, 0.4f, 0.0f, 2.0f * k, 0.0f, 0.6f * permafrost(), 1 }; break; }
    (void)dt;
}
static int d7_on(void) {                                           /* the five lines' augments offered at the Lampstand wherever C6's frame is
                                                                      (their sheet approved, 9 November); CRAWL_D7=0 in the debug build takes
                                                                      them out, for the balance runs */
#ifdef CRAWL_DEBUG
    static int on = -1;
    if (on < 0) on = !(getenv("CRAWL_D7") && atoi(getenv("CRAWL_D7")) == 0);
    return on;
#else
    return 1;
#endif
}
static int augd_built(int e) { return e > EL_FIRE && e < NELEM && d7_on(); }   /* which of the five have their augments built (the
                                                                      Lampstand offers only those) */
/* the five lines' augments (section 11.4, the drafts the owner took till theirs come, A15): a pair at each of the three steps */
static const char *const AUGD_NAME[NELEM][3][2] = {
    [EL_FROST]  = { { "HOARBITE", "SPLINTERFROST" }, { "RIME TRAIL", "GLACIAL WEIGHT" }, { "COLD SNAP", "BRITTLE" } },
    [EL_LIGHT]  = { { "FORKED", "OVERCHARGE" }, { "STATIC BUILD", "GROUNDING" }, { "THUNDERCLAP", "CONDUCTOR" } },
    [EL_POISON] = { { "THICK BREW", "STRAIGHT THROW" }, { "CLUSTER", "MIASMA" }, { "PESTILENCE", "ROT" } },
    [EL_SPIRIT] = { { "SIPHON", "HOLLOWING" }, { "LINGERING HUNGER", "SOUL ECHO" }, { "EXORCISM", "SHEPHERD" } },
    [EL_PHYS]   = { { "BARBED", "RECOVERY" }, { "TWIN THROW", "HEAVY HAFT" }, { "SKEWER", "BONE RAIN" } },
};
static const char *augd_name(int e, int t, int w) {
    if (e <= EL_FIRE || e >= NELEM || t < 0 || t > 2 || w < 1 || w > 2) return "";
    return AUGD_NAME[e][t][w - 1];
}
static void augd_does(int e, int t, int w, float k, char *buf, int n) {
    if (k <= 0.0f) k = 1.0f;
    int p = (int)(100.0f * k + 0.5f);
    *buf = 0;
    if (e <= EL_FIRE || e >= NELEM || t < 0 || t > 2 || w < 1 || w > 2) return;
    switch (e * 6 + t * 2 + (w - 1)) {
    case EL_FROST * 6 + 0: snprintf(buf, (size_t)n, "A FIFTH WEAKER, AND %d%% OF ITS HITS LAY A SECOND CHILL.", p); break;
    case EL_FROST * 6 + 1: snprintf(buf, (size_t)n, "THE SHARD BREAKS INTO THREE BEHIND WHAT IT STRIKES, EACH AT %d%%.", (int)(30.0f * k + 0.5f)); break;
    case EL_FROST * 6 + 2: snprintf(buf, (size_t)n, "THE SHARD RIMES THE FLOOR IT CROSSES FOR %.1f S, SLOWING WHAT WALKS THERE.", 2.0f * k); break;
    case EL_FROST * 6 + 3: snprintf(buf, (size_t)n, "SLOWER AND HEAVIER: IT GOES THROUGH THE FIRST IT STRIKES, AT %d%% AFTER.", (int)(70.0f * k + 0.5f)); break;
    case EL_FROST * 6 + 4: snprintf(buf, (size_t)n, "WHAT FREEZES BURSTS %d TIMES IN A HUNDRED, A CHILL ON ALL WITHIN A CELL AND A HALF.", p); break;
    case EL_FROST * 6 + 5: snprintf(buf, (size_t)n, "THE FROZEN TAKE %d%% MORE FROM EVERYTHING.", (int)(25.0f * k + 0.5f)); break;
    case EL_LIGHT * 6 + 0: snprintf(buf, (size_t)n, "A SECOND BOLT LEAPS TO ANOTHER WITHIN THREE CELLS, AT %d%%.", (int)(50.0f * k + 0.5f)); break;
    case EL_LIGHT * 6 + 1: snprintf(buf, (size_t)n, "%d%% HARDER, AND ITS STUN %d%% LIKELIER; A QUARTER MORE SPIRIT.", (int)(30.0f * k + 0.5f), p); break;
    case EL_LIGHT * 6 + 2: snprintf(buf, (size_t)n, "EACH HIT ON ONE CREATURE WITHIN THREE SECONDS RAISES ITS STUN %d%%, THREE TIMES.", (int)(50.0f * k + 0.5f)); break;
    case EL_LIGHT * 6 + 3: snprintf(buf, (size_t)n, "WHAT STANDS IN WATER OR IS POISONED PASSES IT TO ONE MORE, AT %d%%.", (int)(70.0f * k + 0.5f)); break;
    case EL_LIGHT * 6 + 4: snprintf(buf, (size_t)n, "STRIKE THE STUNNED, AND %d TIMES IN A HUNDRED ALL WITHIN A CELL AND A HALF ARE STUNNED.", p); break;
    case EL_LIGHT * 6 + 5: snprintf(buf, (size_t)n, "IONIZATION'S TRAIL LASTS %.1f TIMES AS LONG, AND STUNS WHAT CROSSES IT %d TIMES IN A HUNDRED.", 1.0f + 2.0f * k, p); break;
    case EL_POISON * 6 + 0: snprintf(buf, (size_t)n, "YOUR POISON LASTS %d%% LONGER, AND HURTS AS MUCH MORE.", (int)(50.0f * k + 0.5f)); break;
    case EL_POISON * 6 + 1: snprintf(buf, (size_t)n, "THROWN FLAT AND %d%% FASTER, IT BURSTS ON THE FIRST IT MEETS.", (int)(60.0f * k + 0.5f)); break;
    case EL_POISON * 6 + 2: snprintf(buf, (size_t)n, "THE BURST THROWS THREE SMALL FLASKS, EACH AT %d%%.", (int)(40.0f * k + 0.5f)); break;
    case EL_POISON * 6 + 3: snprintf(buf, (size_t)n, "THE BURST LEAVES A CLOUD A SECOND AND A HALF THAT POISONS WHAT PASSES, AT %d%%.", p); break;
    case EL_POISON * 6 + 4: snprintf(buf, (size_t)n, "THE POISONED LEAVE A POOL WHERE THEY DIE, OF %d%% OF WHAT WAS LEFT IN THEM.", p); break;
    case EL_POISON * 6 + 5: snprintf(buf, (size_t)n, "THE POISONED TAKE %d%% MORE FROM EVERY OTHER ELEMENT.", (int)(15.0f * k + 0.5f)); break;
    case EL_SPIRIT * 6 + 0: snprintf(buf, (size_t)n, "THE DRAIN HEALS %d%% MORE, AND HURTS 15%% LESS.", (int)(30.0f * k + 0.5f)); break;
    case EL_SPIRIT * 6 + 1: snprintf(buf, (size_t)n, "THE DRAIN LEAVES THE DEAD UNBOUND %d%% MORE OFTEN.", p); break;
    case EL_SPIRIT * 6 + 2: snprintf(buf, (size_t)n, "THE BEAM HOLDS %.1f S AFTER THE CAST, DRAINING %d%% OF THE HIT AGAIN.", 0.6f * k, (int)(50.0f * k + 0.5f)); break;
    case EL_SPIRIT * 6 + 3: snprintf(buf, (size_t)n, "A KILL BY THE DRAIN GIVES BACK %d%% OF ITS SPIRIT.", p); break;
    case EL_SPIRIT * 6 + 4: snprintf(buf, (size_t)n, "THE UNBOUND TAKE %d%% MORE FROM EVERYTHING.", (int)(30.0f * k + 0.5f)); break;
    case EL_SPIRIT * 6 + 5: snprintf(buf, (size_t)n, "THE SOULS OF WHAT THE DRAIN KILLS FLY TO YOU, %s.", k >= 0.99f ? "QUICKLY" : k >= 0.74f ? "STEADILY" : "SLOWLY"); break;
    case EL_PHYS * 6 + 0: snprintf(buf, (size_t)n, "WHAT IT STRIKES BLEEDS %d%% OF THE HIT OVER THREE SECONDS.", (int)(50.0f * k + 0.5f)); break;
    case EL_PHYS * 6 + 1: snprintf(buf, (size_t)n, "A SPEAR THAT KILLS LEAVES ITS BONE %d TIMES IN A HUNDRED.", (int)(50.0f * k + 0.5f)); break;
    case EL_PHYS * 6 + 2: snprintf(buf, (size_t)n, "TWO SPEARS SIDE BY SIDE FOR ONE BONE, EACH AT %d%%.", (int)(30.0f + 30.0f * k + 0.5f)); break;
    case EL_PHYS * 6 + 3: snprintf(buf, (size_t)n, "THE FIRST IT STRIKES STAGGERS %.2f S.", 0.5f * k); break;
    case EL_PHYS * 6 + 4: snprintf(buf, (size_t)n, "EACH CREATURE IT GOES THROUGH ADDS %d%% FOR THE NEXT.", (int)(15.0f * k + 0.5f)); break;
    case EL_PHYS * 6 + 5: snprintf(buf, (size_t)n, "A SPEAR THAT KILLS BURSTS INTO THREE SPLINTERS THAT SEEK THE NEAREST, EACH AT %d%%.", (int)(35.0f * k + 0.5f)); break;
    }
}
/* ---- the dead's track's augment stubs end ---- */

/* ---- the spells' augments (1.0.0b21, PLAN-combat section 11): the caster's track's frame and its dispatchers. Each hook hands a
 * line's spell to its line's own: Fire's here, the other five's to the dead's track (augd_*).
 * A line's three steps (its opening, its ability, its ultimate) each offer a pair; one is chosen at the Lampstand (Space), for good
 * till the line is unbound (A5). The line's credits (a kill by it 1, a thing struck with it 2, a boss 10: A3) bank by the line from
 * its opening and go at once to the newest chosen augment not yet at rank III, then the next (A4); an augment's ranks come at 20,
 * 50 and 100 credits of its own (AUG_CR). CRAWL_AUG=0 in the debug build takes it all out ---- */
static const uint16_t AUG_CR[3] = { 20, 50, 100 };                 /* the credits that make ranks I, II and III */
static const char *const FIRE_AUG[NAUGT][2] = {                 /* Fire's (section 11.2, the guide's); the other five lines' are the
                                                                      dead's track's (augd_name, augd_does) */
    { "BURNING INTENSITY", "FIRECRACKER" }, { "MULTIPLY", "SWIFT FLIGHT" }, { "VOLATILITY", "PYRE" } };
static const float MULTIPLY_K[4] = { 0.5f, 0.3f, 0.4f, 0.5f };     /* a split bolt's share of the cast, by rank (11.3; III's with none) */
static const float VOLATILE_K[4] = { 0.33f, 0.17f, 0.25f, 0.33f }; /* a burst's share of the dead's health, by rank */
static void fire_does(int t, int w, float k, char *buf, int n) {   /* what Fire's augment w at step t does at a rank's share k (0 as III) */
    if (k <= 0.0f) k = 1.0f;
    int r = k >= 0.99f ? 3 : k >= 0.74f ? 2 : 1;
    switch (t * 2 + w - 1) {
    case 0: snprintf(buf, (size_t)n, "HELD, THE BOLT GROWS: A FIFTH WEAKER AT ONCE, %d%% STRONGER AND %.1f TIMES ITS SIZE AT THREE SECONDS, UP TO 30%% MORE SPIRIT.",
                     (int)(25.0f * k + 0.5f), 1.0f + 1.5f * k); break;
    case 1: snprintf(buf, (size_t)n, "THE BOLT IS A SPARK: A QUARTER OF ITS DAMAGE, ALL BUT SURE TO SET ITS MARK ALIGHT, THE BURNING %.1f TIMES AS FIERCE.", 1.0f + 2.0f * k); break;
    case 2: snprintf(buf, (size_t)n, "THE BOLT SPLITS IN AN ARC OF THREE, EACH %d%% OF THE CAST, FOR 30%% MORE SPIRIT.", (int)(100.0f * MULTIPLY_K[r] + 0.5f)); break;
    case 3: snprintf(buf, (size_t)n, "THE BOLT FLIES %d%% FASTER AND %d%% FURTHER.", (int)(100.0f * k + 0.5f), (int)(50.0f * k + 0.5f)); break;
    case 4: snprintf(buf, (size_t)n, "WHAT DIES OF YOUR BURNING BURSTS, SETTING ALL NEAR IT ALIGHT FOR %d%% OF ITS HEALTH. 30%% MORE SPIRIT.",
                     (int)(100.0f * VOLATILE_K[r] + 0.5f)); break;
    default: snprintf(buf, (size_t)n, "WHAT BURNS SPREADS %d%% OF ITS BURNING TO ONE CREATURE WITHIN A CELL, ONCE.", (int)(100.0f * k + 0.5f)); break;
    }
}
static const char *aug_name(int e, int t, int w) {                 /* the augment's name, whichever track's line it is */
    if (w < 1 || w > 2 || t < 0 || t >= NAUGT) return "";
    return e == EL_FIRE ? FIRE_AUG[t][w - 1] : augd_name(e, t, w);
}
static void aug_does(int e, int t, int w, float k, char *buf, int n) {
    *buf = 0;
    if (w < 1 || w > 2 || t < 0 || t >= NAUGT) return;
    if (e == EL_FIRE) fire_does(t, w, k, buf, n); else augd_does(e, t, w, k, buf, n);
}
static int aug_on(void) {                                          /* the augments in play (the sheet approved, 9 November); CRAWL_AUG=0 in
                                                                      the debug build takes them out, for the balance runs */
#ifdef CRAWL_DEBUG
    static int v = -1;
    if (v < 0) v = !(getenv("CRAWL_AUG") && atoi(getenv("CRAWL_AUG")) == 0);
    return v;
#else
    return 1;
#endif
}
static int aug_built(int e) { return aug_on() && (e == EL_FIRE ? 1 : augd_built(e)); }
static int aug_step_open(int e, int t) {                           /* the step reached: the line open, its ability, an ultimate */
    if (e < 0 || e >= NELEM || !align_open(e)) return 0;
    return t == AUG_OPEN ? 1 : t == AUG_ABIL ? align_full(e, ND_ABIL) : align_full(e, ND_AULT) || align_full(e, ND_BULT);
}
static int aug_of(int e, int t) {
    if (e < 0 || e >= NELEM || t < 0 || t >= NAUGT || !aug_built(e) || !aug_step_open(e, t)) return 0;
    return RUN.aug[e][t];
}
static int aug_rank_at(int cr) { return cr >= AUG_CR[2] ? 3 : cr >= AUG_CR[1] ? 2 : cr >= AUG_CR[0] ? 1 : 0; }
static int aug_rank(int e, int t) { return aug_of(e, t) ? aug_rank_at(RUN.aug_cr[e][t]) : 0; }
static inline float aug_k(int e, int t, int w) {
    int r = w && aug_of(e, t) == w ? aug_rank(e, t) : 0;
    return r >= 3 ? 1.0f : r == 2 ? 0.75f : r == 1 ? 0.5f : 0.0f;
}
static const char *const AUG_STEP[NAUGT] = { "OPENING", "ABILITY", "ULTIMATE" };
static void aug_feed(int e) {                                      /* the bank to the newest chosen not yet at III, then the next (A4) */
    for (int guard = 0; guard < NAUGT && RUN.aug_bank[e] > 0; guard++) {
        int best = -1;
        for (int t = 0; t < NAUGT; t++)
            if (RUN.aug[e][t] && aug_step_open(e, t) && RUN.aug_cr[e][t] < AUG_CR[2] && (best < 0 || RUN.aug_ord[e][t] > RUN.aug_ord[e][best])) best = t;
        if (best < 0) return;                                      /* nothing to feed: it waits in the bank */
        int was = aug_rank_at(RUN.aug_cr[e][best]), give = imin(RUN.aug_bank[e], AUG_CR[2] - RUN.aug_cr[e][best]);
        RUN.aug_cr[e][best] = (uint16_t)(RUN.aug_cr[e][best] + give); RUN.aug_bank[e] = (uint16_t)(RUN.aug_bank[e] - give);
        int now = aug_rank_at(RUN.aug_cr[e][best]);
        if (now > was) {
            static const char *const RN[4] = { "", "I", "II", "III" };
            if (G.player) { say("%s: RANK %s", aug_name(e, best, RUN.aug[e][best]), RN[now]); snd_sfx_ex(SFX_RUNE, 0.9f, 0.5f); }
#ifdef CRAWL_DEBUG
            fprintf(stderr, "EVENT aug %s %s %s rank %d t=%.2f by %s\n", ELEM_NAME[e], AUG_STEP[best], aug_name(e, best, RUN.aug[e][best]), now, G.t, G.player ? "player" : "autopilot");
#endif
        }
    }
}
static void aug_credit(int e, int n) {                             /* the line's credits, from its opening (11.1) */
    if (e < 0 || e >= NELEM || n <= 0 || !aug_on() || !align_open(e)) return;
    RUN.aug_bank[e] = (uint16_t)imin(60000, RUN.aug_bank[e] + n);
    aug_feed(e);
}
static int aug_choose(int e, int t, int w) {                       /* final, till the line is unbound (A5) */
    if (!aug_built(e) || !aug_step_open(e, t) || RUN.aug[e][t] || w < 1 || w > 2) return 0;
    int ord = 0;
    for (int k = 0; k < NAUGT; k++) ord = imax(ord, RUN.aug_ord[e][k]);
    RUN.aug[e][t] = (uint8_t)w; RUN.aug_ord[e][t] = (uint8_t)(ord + 1);
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT aug_choose %s %s %s t=%.2f by %s\n", ELEM_NAME[e], AUG_STEP[t], aug_name(e, t, w), G.t, G.player ? "player" : "autopilot");
#endif
    aug_feed(e);                                                   /* a pair chosen late ranks up from the bank at once */
    return 1;
}
static void aug_forget(int e) {                                    /* the line unbound, or a death that costs: its augments go (A5, A14) */
    if (e < 0 || e >= NELEM) return;
    memset(RUN.aug[e], 0, sizeof RUN.aug[e]); memset(RUN.aug_ord[e], 0, sizeof RUN.aug_ord[e]);
    memset(RUN.aug_cr[e], 0, sizeof RUN.aug_cr[e]); RUN.aug_bank[e] = 0;
}
static void auto_augment(void) {                                   /* the dream's: at random within each pair, as it opens (A15) */
    if (LEDGER.yours || LEDGER.kept) return;                       /* a person's run: theirs to choose */
#ifdef CRAWL_DEBUG
    if (getenv("CRAWL_AUGAUTO") && !atoi(getenv("CRAWL_AUGAUTO"))) return;   /* CRAWL_AUGAUTO=0: the dream leaves them, for frames */
#endif
    for (int e = 0; e < NELEM; e++)
        for (int t = 0; t < NAUGT; t++)
            if (aug_built(e) && aug_step_open(e, t) && !RUN.aug[e][t]) aug_choose(e, t, 1 + rnd(2));
}
#ifdef CRAWL_DEBUG
static void aug_debug_set(void) {                                  /* CRAWL_AUGSET: eighteen digits, a line's three steps each in ELEM order:
                                                                      0 none, 1-3 the first at that rank, 4-6 the second */
    const char *v = getenv("CRAWL_AUGSET");
    if (!v || !aug_on()) return;
    for (int i = 0; v[i] && i < NELEM * NAUGT; i++) {
        int d = v[i] - '0', e = i / NAUGT, t = i % NAUGT;
        if (d < 1 || d > 6) continue;
        RUN.aug[e][t] = (uint8_t)(d > 3 ? 2 : 1); RUN.aug_ord[e][t] = (uint8_t)(t + 1);
        RUN.aug_cr[e][t] = AUG_CR[(d - 1) % 3];
    }
}
#endif
/* Fire's six (section 11.2), each felt only at rank I or more (aug_k is 0 till then). The opening's pair changes the bolt itself:
 * Burning Intensity holds it (human_cast, the dream's PS_CAST), a fifth weaker tapped, growing over three seconds held, and past
 * three with the Firewall the Firewall comes (A9); Firecracker makes it a spark. The ability's: Multiply splits it in three,
 * Swift Flight speeds it. The ultimate's: Volatility bursts what dies of your burning, Pyre passes burning on. Only the bolt
 * (AT_FIRE) takes the first four; the burning (AT_HELL's ticks, whatever lit it) the last two */
static float weak_el(const Mob *m, int el);
static float el_chance(const Mob *m, int el);
static void mob_ignite(Mob *m, float dmg);
static int mob_takes_af(const Mob *m);
static void explode_big(float x, float y, float r);
static float g_fire_hold = -1.0f;                                  /* Burning Intensity: the held bolt's share of three seconds as it
                                                                      goes; below 0, a bolt not held (a tap, a splash, the dream's) */
#ifdef CRAWL_DEBUG
static int fire_held, fire_splits, fire_bursts, fire_spreads;
#endif
#define FIRE_GROW_T 3.0f                                           /* Burning Intensity's full growth, and the Firewall's hold with it */
static int fire_grows(int sp) { return sp == AT_FIRE && aug_k(EL_FIRE, AUG_OPEN, 1) > 0.0f; }
static float fire_cost(int sp) {                                   /* the increases add (A7) */
    if (sp != AT_FIRE) return 0.0f;
    float c = 0.0f;
    if (fire_grows(sp) && g_fire_hold > 0.0f) c += 0.3f * g_fire_hold;
    if (aug_k(EL_FIRE, AUG_ABIL, 1) > 0.0f) c += 0.3f;             /* Multiply */
    if (aug_k(EL_FIRE, AUG_ULT, 1) > 0.0f) c += 0.3f;              /* Volatility */
    return c;
}
static float fire_dmg(int sp) {
    if (sp != AT_FIRE) return 1.0f;
    float d = 1.0f, k = aug_k(EL_FIRE, AUG_OPEN, 1);
    if (k > 0.0f) d *= 0.8f + (0.2f + 0.25f * k) * fmaxf(0.0f, g_fire_hold);   /* a fifth weaker at once, a quarter stronger at three s */
    else if (aug_k(EL_FIRE, AUG_OPEN, 2) > 0.0f) d *= 0.25f;       /* the spark */
    if (aug_k(EL_FIRE, AUG_ABIL, 1) > 0.0f) d *= MULTIPLY_K[aug_rank(EL_FIRE, AUG_ABIL)];   /* each of the three */
    return d;
}
static void fire_cast(int sp, Shot *s) {
    if (sp != AT_FIRE || !s) return;
    float k = aug_k(EL_FIRE, AUG_OPEN, 1);
    if (k > 0.0f) s->scale = 1.0f + 1.5f * k * fmaxf(0.0f, g_fire_hold);
    else if (aug_k(EL_FIRE, AUG_OPEN, 2) > 0.0f) s->scale = 0.55f;
    if ((k = aug_k(EL_FIRE, AUG_ABIL, 2)) > 0.0f) {                /* Swift Flight: faster by k, further by half k */
        s->vx *= 1.0f + k; s->vy *= 1.0f + k; s->life *= (1.0f + 0.5f * k) / (1.0f + k);
    }
    if (aug_k(EL_FIRE, AUG_ABIL, 1) > 0.0f) {                      /* Multiply: two more, a little either side */
        float a = atan2f(s->vy, s->vx), v = hypotf(s->vx, s->vy);
        for (int i = -1; i <= 1; i += 2) {
            Shot *o = new_shot(AT_FIRE, s->x, s->y, a + 0.25f * (float)i, v, s->dmg);
            if (o) { o->tier = s->tier; o->target = s->target; o->scale = s->scale; o->life = s->life; }
        }
#ifdef CRAWL_DEBUG
        DBG_COUNT(fire_splits);
#endif
    }
}
static float fire_af(const Mob *m, int atk, int q) {               /* Firecracker: all but sure to set its mark alight, and fiercer */
    float k = atk == AT_FIRE ? aug_k(EL_FIRE, AUG_OPEN, 2) : 0.0f;
    if (k <= 0.0f) return 1.0f;
    if (q == AUGQ_HARM) return 1.0f + 2.0f * k;                    /* three times the spark's damage over four seconds at III (A11) */
    float c = el_chance(m, EL_FIRE);
    return c > 0.0f ? fmaxf(1.0f, (weak_el(m, EL_FIRE) < 0.75f ? 0.6f : 0.95f) / c) : 1.0f;   /* less sure of what shrugs fire off */
}
static void fire_kill(Mob *m, int atk, int flags) {                /* Volatility: what dies of your burning bursts */
    if (!(flags & DM_TICK) || (atk != AT_HELL && atk != AT_FIRE) || aug_k(EL_FIRE, AUG_ULT, 1) <= 0.0f) return;
    float harm = m->maxhp * VOLATILE_K[aug_rank(EL_FIRE, AUG_ULT)];
    explode_big(m->x, m->y, 1.5f);
    for (int i = 0; i < MAXMOBS; i++) {
        Mob *o = &G.mobs[i];
        if (o == m || !mob_takes_af(o) || hypotf(o->x - m->x, o->y - m->y) >= 1.6f || !line_clear(m->x, m->y, o->x, o->y)) continue;
        mob_ignite(o, harm * (o->boss ? 0.1f : 1.0f));             /* a boss a tenth (A12); it dies once, so it bursts once */
    }
#ifdef CRAWL_DEBUG
    DBG_COUNT(fire_bursts);
#endif
}
static void fire_tick(Mob *m) {                                    /* Pyre: what burns passes its burning to one within a cell, once */
    if (m->pyre || m->burn_t <= 0.0f) return;
    float k = aug_k(EL_FIRE, AUG_ULT, 2);
    if (k <= 0.0f) return;
    Mob *best = NULL; float bd = 1.25f;                            /* a cell between them, near enough */
    for (int i = 0; i < MAXMOBS; i++) {
        Mob *o = &G.mobs[i];
        if (o == m || o->burn_t > 0.0f || !mob_takes_af(o)) continue;
        float d = hypotf(o->x - m->x, o->y - m->y);
        if (d < bd && line_clear(m->x, m->y, o->x, o->y)) { bd = d; best = o; }
    }
    if (!best) return;
    m->pyre = 1;
    mob_ignite(best, k * m->burn_dps * m->burn_t);                 /* k of what is left to it */
    sfx_at(SFX_IGNITE, best->x, best->y, 0.6f);
#ifdef CRAWL_DEBUG
    DBG_COUNT(fire_spreads);
#endif
}
static int aug_line(int atk) { return atk >= 0 && atk < NATK ? ATK_ELEM[atk] : -1; }   /* a spell's line (Hellfire's burning is Fire's) */
static float aug_cost(int sp) { int e = aug_line(sp); return e < 0 ? 0.0f : e == EL_FIRE ? fire_cost(sp) : augd_cost(e, sp); }
static float aug_dmg(int sp) { int e = aug_line(sp); return e < 0 ? 1.0f : e == EL_FIRE ? fire_dmg(sp) : augd_dmg(e, sp); }
static void aug_cast(int sp, Shot *s, int t) { int e = aug_line(sp); if (e == EL_FIRE) fire_cast(sp, s); else if (e >= 0) augd_cast(e, sp, s, t); }
static void aug_hit(Mob *m, int atk, int flags, float *dmg) { int e = aug_line(atk); if (e >= 0 && e != EL_FIRE) augd_hit(e, m, atk, flags, dmg); }
static float aug_af(const Mob *m, int atk, int q) { int e = aug_line(atk); return e < 0 ? 1.0f : e == EL_FIRE ? fire_af(m, atk, q) : augd_af(e, m, atk, q); }
static void aug_kill(Mob *m, int atk, int flags) {
    int e = aug_line(atk);
    if (e < 0) return;
    if (atk != AT_BFS) aug_credit(e, 1);                           /* a kill by the line: a credit (A3) */
    if (e == EL_FIRE) fire_kill(m, atk, flags); else augd_kill(e, m, atk, flags);
}
static void aug_tick(Mob *m, float dt) { fire_tick(m); augd_tick(m, dt); }
static void aug_fly(Shot *s, float dt) { int e = aug_line(s->kind); if (e >= 0 && e != EL_FIRE) augd_fly(e, s, dt); }

