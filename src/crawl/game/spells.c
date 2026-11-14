/* A part of scene_crawl.c (docs/PLAN-tidy.md): it is #included there in its place and never compiled on its own.
 * The caster's spells: shots, lightning, the BFS9000, the flask, cast_spell; charging and the ultimates; update_shots. */

/* ---- the caster's spells ---- */

static int wisp_field[MAPN][MAPN];                                  /* steps to the wisps' target, for going round corners */
static int reflect_field[MAPN][MAPN];                               /* the same, to a reflected missile's thrower */
static void field_bfs(int f[MAPN][MAPN], int tx, int ty) {
    static int qx[MAPN * MAPN], qy[MAPN * MAPN];
    for (int y = 0; y < MAPN; y++) for (int x = 0; x < MAPN; x++) f[y][x] = -1;
    int qh = 0, qt = 0;
    f[ty][tx] = 0; qx[qt] = tx; qy[qt] = ty; qt++;
    while (qh < qt) {
        int x = qx[qh], y = qy[qh]; qh++;
        if (f[y][x] > 14) continue;
        for (int d = 0; d < 4; d++) {
            int nx = x + DX[d], ny = y + DY[d];
            if (blocks_light(nx, ny) || cell(nx, ny)->kind == K_PILLAR || f[ny][nx] >= 0) continue;
            f[ny][nx] = f[y][x] + 1; qx[qt] = nx; qy[qt] = ny; qt++;
        }
    }
}
static void wisp_bfs(int tx, int ty) { field_bfs(wisp_field, tx, ty); }

static Shot *new_shot(int kind, float x, float y, float a, float speed, float dmg) {
    for (int i = 0; i < MAXSHOTS; i++) {
        Shot *s = &G.shots[i];
        if (s->alive) continue;
        *s = (Shot){ 1, kind, -1, x, y, 0.0f, cosf(a) * speed, sinf(a) * speed, kind == AT_SPEAR || kind == SHOT_SPLINTER ? 1.0f : 1.6f, dmg, 0, 1, 0.0f, 0, 0.0f };
        G.lay_x[i] = x; G.lay_y[i] = y;
        return s;
    }
    return NULL;
}

static void add_beam(int kind, float x0, float y0, float z0, float x1, float y1, float z1) {
    for (int i = 0; i < MAXBEAMS; i++)
        if (!G.beams[i].alive) { G.beams[i] = (Beam){ 1, kind, x0, y0, z0, x1, y1, z1, 0.0f }; return; }
}

static int spell_tier(int sp) { return RUN.gear.atk[sp] < 0 ? 0 : RUN.gear.atk[sp]; }

/* Splinters (docs/PLAN-play.md, N2): with the bones gone the spear's line throws splinters, its own ammunition: eight
 * at most, one growing back every six seconds, thrown slower and striking lighter, through nothing. Bones are where
 * the line's strength is. */
#define SPLINTER_MAX 8
#define SPLINTER_T 6.0f                     /* a splinter grows back */
#define SPLINTER_DMG 0.4f                   /* of the spear's blow, at its rank */
#define SPLINTER_SLOW 1.6f                  /* times the spear's time between throws */
static int splint_left(void) { return SPLINTER_MAX - clampi(RUN.splint_used, 0, SPLINTER_MAX); }
static int spear_bare(void) { return RUN.gear.bones <= 0; }       /* the next throw a splinter */
static int spear_ready(void) { return !spear_bare() || splint_left() > 0; }
static float af_slow_cast(void) { return af_on(AF_POISON) ? 1.5f : 1.0f; }   /* Poisoned: wind-up and cycle half again (S5) */
static float cast_cycle(int sp) { float c = spell_cycle(&RUN.gear, sp) * af_slow_cast(); return sp == AT_SPEAR && spear_bare() ? c * SPLINTER_SLOW : c; }
static void splint_tick(float dt) {
    if (RUN.splint_used <= 0 || RUN.gear.atk[AT_SPEAR] < 0) { RUN.splint_t = 0.0f; return; }
    if ((RUN.splint_t += dt) >= SPLINTER_T) { RUN.splint_t -= SPLINTER_T; RUN.splint_used--; }
}
/* the caster's own spell, for whatever reaches for one without choosing: the class's line, else the line lent, else
 * the first open one that can go now (1.0.0b16: the fireball is no longer everyone's) */
static int own_spell(void) {
    int e = cls_el() >= 0 ? cls_el() : LEDGER.lent - 1;
    if (e >= 0 && RUN.gear.atk[LINE_ATK[e]] >= 0 && (LINE_ATK[e] != AT_SPEAR || spear_ready())) return LINE_ATK[e];
    for (int k = 0; k < NELEM; k++) {
        int a = LINE_ATK[k];
        if (RUN.gear.atk[a] >= 0 && (a != AT_SPEAR || spear_ready())) return a;
    }
    return e >= 0 ? LINE_ATK[e] : AT_FIRE;                         /* nothing to throw: it fizzles and says why */
}
static int own_missile(void) {                                     /* the same, but something that flies with no target */
    int a = own_spell();
    if (a != AT_CHAIN && a != AT_DRAIN) return a;
    for (int k = 0; k < NELEM; k++) {
        int b = LINE_ATK[k];
        if (b != AT_CHAIN && b != AT_DRAIN && RUN.gear.atk[b] >= 0 && (b != AT_SPEAR || spear_ready())) return b;
    }
    return a;
}

/* lightning: a bolt that strikes the target at once; with Arc it leaps on to the nearest, up to four leaps,
 * each a quarter weaker (docs/PLAN-b11.md, section 9) */
static void chain_lightning(int t, float dmg, int tier) {
    uint64_t hit = 0;
    G.d7_zap = dmg;                                                /* the bolt's harm, for Forked and Grounding (D7) */
    Mob *m = &G.mobs[t];
    float hx = G.px + cosf(G.ang) * 0.35f, hy = G.py + sinf(G.ang) * 0.35f;
    add_beam(AT_CHAIN, hx, hy, 0.42f, m->x, m->y, m->z + 0.5f);
    hit |= 1ull << t;
    float tx0 = hx, ty0 = hy;
    damage_mob(m, dmg, DM_ARC | DM_ATK(AT_CHAIN), G.px, G.py, 0.0f);
    int hops = align_has(EL_LIGHT, ND_AULT) ? 4 : 0;
    (void)tier;
    float cx = m->x, cy = m->y, cz = m->z;
    for (int h = 0; h < hops; h++) {
        int best = -1; float bd = 3.5f;
        for (int i = 0; i < MAXMOBS; i++) {
            Mob *o = &G.mobs[i];
            if ((hit >> i & 1) || !o->alive || o->state != MS_WALK || KIND_IS(o, KF_SPIRIT) || o->ally) continue;
            float d = hypotf(o->x - cx, o->y - cy);
            if (d < bd && line_clear(cx, cy, o->x, o->y)) { bd = d; best = i; }
        }
        if (best < 0) break;
        Mob *o = &G.mobs[best];
        add_beam(AT_CHAIN, cx, cy, cz + 0.5f, o->x, o->y, o->z + 0.5f);
        hit |= 1ull << best;
        dmg *= 0.75f;
        damage_mob(o, dmg, DM_ARC | DM_ATK(AT_CHAIN), cx, cy, 0.0f);
        cx = o->x; cy = o->y; cz = o->z;
    }
    ionize(tx0, ty0, m->x, m->y, dmg, hit);                        /* Ionization: the way it went hangs a moment */
    cx = m->x; cy = m->y;
    snd_sfx_ex(SFX_ZAP, 1.2f, 0.5f);
}

/* the BFS9000: everything the caster can see collapses into a point of light */
static void bfs9000(void) {
    int tier = RUN.gear.atk[AT_BFS];
    RUN.souls -= bfs_cost(tier);
    G.bfs_t = 0.001f;
    rumble(1.0f, 1.0f, 0.7f);
    snd_sfx_ex(SFX_IMPLODE, 1.3f, 0.5f);
    for (int i = 0; i < MAXMOBS; i++) {
        Mob *m = &G.mobs[i];
        if (!m->alive || m->state != MS_WALK || (KIND_IS(m, KF_SPIRIT) && !m->boss) || m->ally) continue;
        if (hypotf(m->x - G.px, m->y - G.py) > 14.0f || !line_clear(G.px, G.py, m->x, m->y)) continue;
        if (m->boss) { damage_mob(m, m->maxhp * 0.08f, 0, G.px, G.py, 0.0f); continue; }   /* a boss takes a twelfth or so */
        m->hp = 0.0f;
        kill_mob(m);
        m->sub = 77;                                               /* drawn imploding */
#ifdef CRAWL_DEBUG
        bfs_kills++;
#endif
    }
}

/* The Blight Flask bursts (section 10): a splash a cell across, a quarter of its harm at once and the rest as poison
 * over four seconds */
static void flask_burst(Shot *s) {
    s->alive = 0;
    mk_burst(s->x, s->y, EL_POISON, s->mkhit);
    for (int i = 0; i < MAXBOOMS; i++)
        if (!G.booms[i].alive) { G.booms[i] = (Boom){ 1, s->x, s->y, 0.0f, 0.9f, 2 }; break; }
    sfx_at(SFX_SHATTER, s->x, s->y, 0.6f); sfx_at(SFX_SPLASH, s->x, s->y, 1.0f);
    for (int i = 0; i < MAXMOBS; i++) {
        Mob *m = &G.mobs[i];
        if (!m->alive || m->state != MS_WALK || KIND_IS(m, KF_SPIRIT) || m->ally || buried(m)) continue;
        if (hypotf(m->x - s->x, m->y - s->y) > 0.6f + KINDS[m->type].rad || !line_clear(s->x, s->y, m->x, m->y)) continue;
        float hit = s->dmg * 0.25f * (cls_el() == EL_POISON ? PLAGUE_STRIKE : 1.0f);   /* the Plague Doctor's flask strikes harder (theirs alone) */
        if (damage_mob(m, hit, DM_ARC | DM_ATK(AT_FLASK), s->x, s->y, 0.0f)) continue;
        mob_poison(m, s->dmg * 0.75f, 4.0f);
    }
    if (s->target >= 1000) {                                       /* Plague Pit: the pool grows with the charge, six seconds, slowing */
        float k = clampf((float)(s->target - 1000) / 100.0f, 0.0f, 1.0f);
        for (int i = 0; i < MAXVENOM; i++)
            if (!G.venoms[i].alive) { G.venoms[i] = (Venom){ 1, s->x, s->y, 0.55f + 0.7f * k, 0.0f, 6.0f, s->dmg * 0.75f, 0.7f, 0 }; break; }
#ifdef CRAWL_DEBUG
        fprintf(stderr, "EVENT plague_pit r=%.2f t=%.2f\n", 0.55f + 0.7f * k, G.t);
#endif
        return;
    }
    seep(s->x, s->y, s->dmg * 0.75f);
    float k;
    if ((k = dk(EL_POISON, 1, 1)) > 0.0f && s->target != -3) {   /* Cluster: three small flasks thrown out (D7) */
        float a0 = rndf() * TAU_F;
        for (int i = 0; i < 3; i++) {
            Shot *c = new_shot(AT_FLASK, s->x, s->y, a0 + (float)i * TAU_F / 3.0f, 4.5f, s->dmg * 0.4f * k);
            if (c) { c->tier = s->tier; c->arc = c->life = 0.35f; c->target = -3; c->scale = 0.6f; }
        }
        DBG_COUNT(aug_clusters);
    }
    if ((k = dk(EL_POISON, 1, 2)) > 0.0f)                          /* Miasma: a cloud where it burst (D7) */
        for (int i = 0; i < MAXVENOM; i++)
            if (!G.venoms[i].alive) { G.venoms[i] = (Venom){ 1, s->x, s->y, 0.9f, 0.0f, 1.5f, s->dmg * 0.75f * k, 0.0f, 2 }; DBG_COUNT(aug_miasmas); break; }
}

/* let the chosen spell go at target t (or straight ahead if it is gone) */
static void fizzle(int sp);
static void say(const char *fmt, ...);
static void cast_spell(int sp, int t) {
    const Gear *g = &RUN.gear;
    Mob *m = t >= 0 ? &G.mobs[t] : &G.mobs[0];
    if (af_hushed()) return;                                       /* stunned, unbound, frozen or down: nothing comes (PLAN-combat, 4) */
    if (G.rest_on) rest_end("cast");                                /* a cast gets you up */
    int have = t >= 0 && m->alive && m->state == MS_WALK;
    float tx = have ? m->x : G.px + cosf(G.ang) * 4, ty = have ? m->y : G.py + sinf(G.ang) * 4;
    int seal = !have && (sp == AT_CHAIN || sp == AT_DRAIN) ? mk_aimed() : -1;   /* a seal: the way to it, and spirit spent as on a creature */
    if (seal >= 0) { tx = G.mk[seal].x; ty = G.mk[seal].y; }
    float a = atan2f(ty - G.py, tx - G.px);
    if (G.player && sp != AT_CHAIN && sp != AT_DRAIN) a = G.ang;   /* a person's shots go where they look */
    if (G.af_n > 0 && sp != AT_CHAIN && sp != AT_DRAIN) a += (rndf() * 2.0f - 1.0f) * 0.0262f * (float)G.af_n;   /* Chilled: 1.5 degrees astray a stack (S4) */
    /* what it costs: a bone for every spear; spirit above tier 1, and with too little, it goes at tier-1
     * strength (and heals nothing) */
    int splinter = sp == AT_SPEAR && spear_bare();                 /* no bones: a splinter, if one has grown */
    if (splinter && splint_left() <= 0) { say("NO BONES, AND NO SPLINTERS YET"); fizzle(sp); return; }
    int tier = spell_tier(sp), weak = 0;
    if (sp != AT_BFS && !((sp == AT_CHAIN || sp == AT_DRAIN) && !have && seal < 0) && !spend_spirit(spell_cost(g, sp, tier) * (1.0f + aug_cost(sp)))) {   /* its augments' */
        tier = 1; weak = 1;
        if (!G.spirit_said) { G.spirit_said = 1; say("LOW ON SPIRIT: SPELLS CAST WEAK"); }
#ifdef CRAWL_DEBUG
        fprintf(stderr, "EVENT weak_%s t=%.2f\n", ATK_NAME[sp], G.t);
        dbg_weak++;
#endif
    }
    if (sp == AT_SPEAR && !splinter && rndf() >= bone_spared()) RUN.gear.bones--;   /* Physical Efficiency: now and then none */
    float dmg = spell_damage_at(g, sp, tier) * aug_dmg(sp);       /* its augments' share (1.0.0b21) */
    if (sp == AT_SPEAR && !splinter && align_has(EL_PHYS, ND_A2)) dmg += 0.5f * (float)gear_armor(g);   /* Ossified: half your armour behind it */
    if (splinter) {
        dmg *= SPLINTER_DMG; RUN.splint_used++;
        if (!G.splint_said) { G.splint_said = 1; say("NO BONES: SPLINTERS THROWN"); }
    }
    if (web_strike(sp, dmg * (sp == AT_WISPS ? (float)wisp_count(tier) : 1.0f))) return;   /* held: it strikes the web */
    float hx = G.px + cosf(a) * 0.35f, hy = G.py + sinf(a) * 0.35f;
    int bat = have && m->type == MOB_BAT && !G.player;
    noise(G.px, G.py, sp == AT_BFS ? HUNT_FAR : (sp == AT_SPEAR ? (splinter ? 4 : 5) : (sp == AT_HELL ? 10 : 8)), NZ_CAST, -1);   /* a spell is heard down the halls */
    G.flare_t = 1.0f;                                              /* and it lights the caster a moment (S2) */
#ifdef CRAWL_DEBUG
    casts[sp]++;
    fprintf(stderr, "EVENT cast_%s t=%.2f\n", splinter ? "SPLINTER" : ATK_NAME[sp], G.t);
#endif
    if (splinter) {
        Shot *s = new_shot(SHOT_SPLINTER, hx, hy, a + (bat ? (rndf() - 0.5f) * 0.6f : 0.0f), 11.0f, dmg);
        if (s) s->tier = tier;
        snd_sfx_ex(SFX_WHOOSH, 0.45f, 0.5f);
        return;
    }
    switch (sp) {
    case AT_CHAIN:
        if (have) chain_lightning(t, dmg, tier);
        else { int k = mk_aimed(); if (k >= 0) { add_beam(AT_CHAIN, hx, hy, 0.42f, G.mk[k].x, G.mk[k].y, 0.6f); mk_strike(k, EL_LIGHT); } else fizzle(sp); }   /* a seal */
        aug_cast(sp, NULL, have ? t : -1);
        return;
    case AT_DRAIN:
        if (have) {
            add_beam(AT_DRAIN, hx, hy, 0.42f, m->x, m->y, m->z + 0.5f);
            float before = m->hp;
            damage_mob(m, dmg, DM_ARC | DM_ATK(AT_DRAIN), G.px, G.py, 0.0f);
            float dealt = before - fmaxf(m->hp, 0.0f);
            float heal = fminf(dealt * fminf(0.8f, 0.5f + 0.05f * tier) * spell_effect() * (1.0f + ALIGN[EL_SPIRIT].n[ND_A2].val * (float)RUN.al[EL_SPIRIT][ND_A2]),
                               (float)(G.max_hp - G.hp));                  /* Communion heals more */
            if (heal > 0.0f && (weak || !spend_spirit(heal * heal_cost(g, tier)))) {   /* healing costs spirit too */
                heal = 0.0f;
                if (!G.spirit_said) { G.spirit_said = 1; say("LOW ON SPIRIT: SPELLS CAST WEAK"); }
            }
            G.hp = clampi(G.hp + (int)(heal + 0.5f), 0, G.max_hp);
            pop_readout();
            snd_sfx_ex(SFX_WHISPER, 0.8f, 0.5f);
            if (align_has(EL_SPIRIT, ND_BULT)) {                   /* Wisp Host: three wisps, each for 40% of the drain */
                wisp_bfs(m->cx, m->cy);
                for (int i = 0; i < 3; i++) {
                    Shot *w = new_shot(AT_WISPS, hx, hy, a + (float)(i - 1) * 0.6f, 4.5f, dmg * 0.4f);
                    if (w) { w->target = -1; w->life = 3.0f; w->tier = tier; }
                }
                DBG_COUNT(wisp_host);
            }
        } else { int k = mk_aimed(); if (k >= 0) { add_beam(AT_DRAIN, hx, hy, 0.42f, G.mk[k].x, G.mk[k].y, 0.6f); mk_strike(k, EL_SPIRIT); } else fizzle(sp); }   /* a seal */
        aug_cast(sp, NULL, have ? t : -1);
        return;
    case AT_BFS: bfs9000(); return;
    case AT_FLASK: {                                               /* lobbed to where you aim, over what is in the way */
        float d = clampf(have ? hypotf(tx - G.px, ty - G.py) : 4.5f, 1.2f, 9.0f);
        Shot *s = new_shot(AT_FLASK, hx, hy, a, 7.0f, dmg);
        if (s) { s->tier = tier; s->arc = s->life = d / 7.0f; aug_cast(sp, s, have ? t : -1); }
        snd_sfx_ex(SFX_WHOOSH, 0.6f, 0.5f);
        return;
    }
    case AT_WISPS: {
        int n = wisp_count(tier);
        if (have) wisp_bfs(m->cx, m->cy);
        for (int i = 0; i < n; i++) {
            Shot *s = new_shot(AT_WISPS, hx, hy, a + (i - (n - 1) * 0.5f) * 0.45f, 4.5f, dmg);
            if (s) { s->target = have ? t : -1; s->life = 3.0f; s->tier = tier; aug_cast(sp, s, have ? t : -1); }
        }
        snd_sfx_ex(SFX_SHIMMER, 0.6f, 0.5f);
        return;
    }
    default: {
        float speed = sp == AT_SPEAR ? 12.0f : (sp == AT_HELL ? 6.0f : (sp == AT_FROST ? 9.0f : 8.0f));
        if (bat) a += (rndf() - 0.5f) * (sp == AT_FROST ? 0.3f : 0.6f);   /* bats are hard to hit */
        Shot *s = new_shot(sp, hx, hy, a, speed, dmg);
        if (s) { s->tier = tier; aug_cast(sp, s, have ? t : -1); }
        snd_sfx_ex(sp == AT_FROST ? SFX_FROST : SFX_CAST, sp == AT_HELL ? 1.3f : 1.0f, 0.5f);
        return;
    }
    }
}

static int targetable(const Mob *m);
static int aim_target(void);
/* ---- charging and the ultimates (docs/PLAN-b11.md, sections 7, 9 and 10) ----
 * A line with a charging ultimate: its spell held charges over a second, let go releases it, a tap casts the spell.
 * Soul Tether's spell held keeps the beam on. The rest of the ultimates work by themselves. */
static int spell_charges(int sp) {
    switch (sp) {
    case AT_FIRE: return align_has(EL_FIRE, ND_BULT);               /* Firewall */
    case AT_FROST: return align_has(EL_FROST, ND_AULT);             /* Winter's Lance */
    case AT_CHAIN: return align_has(EL_LIGHT, ND_BULT);             /* Amperage */
    case AT_FLASK: return align_has(EL_POISON, ND_AULT);            /* Plague Pit */
    case AT_SPEAR: return align_has(EL_PHYS, ND_AULT) || align_has(EL_PHYS, ND_BULT);   /* the Phalanx, the Ballista */
    default: return 0;
    }
}
static int spell_tethers(int sp) { return sp == AT_DRAIN && align_has(EL_SPIRIT, ND_AULT); }
static int spell_holds(int sp) { return spell_charges(sp) || spell_tethers(sp) || fire_grows(sp); }   /* and Burning Intensity's bolt */
static float hold_span(int sp) { return fire_grows(sp) ? FIRE_GROW_T : 1.0f; }   /* a full charge: three seconds with it, the Firewall too */
static void cast_held(int sp, int t, float held) {                 /* Burning Intensity's bolt let go: grown by the hold */
    g_fire_hold = clampf(held / FIRE_GROW_T, 0.0f, 1.0f);
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT fire_held k=%.2f t=%.2f by %s\n", g_fire_hold, G.t, G.player ? "player" : "autopilot"); fire_held++;
#endif
    cast_spell(sp, t);
    g_fire_hold = -1.0f;
}
static void mob_stun(Mob *m, float secs);
static void daze(Mob *m) {                                         /* Amperage: no blow or throw a while; 1.0.0b19, a stun (section 5) */
    if (elem_mode()) { mob_stun(m, 1.5f); return; }
    m->cool = fmaxf(m->cool, 1.5f); m->throw_t = 0.0f; m->swing = 0.0f;
}
static void rime(float x, float y) {                               /* Winter's Lance: the floor rimed four seconds, slowing what crosses */
    for (int i = 0; i < MAXVENOM; i++)
        if (!G.venoms[i].alive) { G.venoms[i] = (Venom){ 1, x, y, 0.45f, 0.0f, 4.0f, 0.0f, 1.0f * permafrost(), 1 }; return; }
}
static void cast_charged(int sp, int t, float k) {
    const Gear *g = &RUN.gear;
    Mob *m = t >= 0 ? &G.mobs[t] : NULL;
    int have = m && m->alive && m->state == MS_WALK, tier = spell_tier(sp);
    if ((sp == AT_FIRE && k < 0.98f) || G.web_hp > 0.0f) { cast_spell(sp, t); return; }   /* Firewall let go early: an ordinary fireball;
                                                                                              held in a web: it goes at the web */
    if (sp == AT_SPEAR && spear_bare()) { cast_spell(sp, t); return; }   /* no bones for the Phalanx or the Ballista: a splinter */
    if (!spend_spirit(spell_cost(g, sp, tier) * (1.0f + 1.5f * k))) { cast_spell(sp, t); return; }   /* a full charge, two and a half casts */
    float a = G.player || !have ? G.ang : atan2f(m->y - G.py, m->x - G.px), dmg = spell_damage_at(g, sp, tier);
    float hx = G.px + cosf(a) * 0.35f, hy = G.py + sinf(a) * 0.35f;
    noise(G.px, G.py, 10, NZ_CAST, -1); G.flare_t = 1.0f;          /* an ultimate is heard further */
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT ultimate %s k=%.2f t=%.2f by %s\n", ATK_NAME[sp], k, G.t, G.player ? "player" : "autopilot");
#endif
    Shot *s;
    switch (sp) {
    case AT_FIRE:                                                  /* Firewall: three flame spirits round you, five seconds */
        G.fwall_t = 5.0f; G.fwall_dps = dmg * 0.5f / 5.0f;
        snd_sfx_ex(SFX_EXPLODE, 0.8f, 0.5f); snd_sfx_ex(SFX_WHOOSH, 1.0f, 0.5f);
        return;
    case AT_FROST:                                                 /* Winter's Lance */
        if ((s = new_shot(SHOT_LANCE, hx, hy, a, 14.0f, dmg * (1.0f + k)))) { s->tier = tier; s->life = 1.4f; }
        snd_sfx_ex(SFX_FROST, 1.4f, 0.5f);
        return;
    case AT_CHAIN:                                                 /* Amperage: the orb, twice the bolt at a full charge */
        if ((s = new_shot(SHOT_ORB, hx, hy, a, 3.5f, dmg * (1.0f + k)))) { s->tier = tier; s->life = 6.0f; }
        snd_sfx_ex(SFX_ZAP, 1.5f, 0.5f); snd_sfx_ex(SFX_SHIMMER, 0.8f, 0.5f);
        return;
    case AT_FLASK: {                                               /* Plague Pit: the flask, and its pool grows with the charge */
        float tx = have ? m->x : G.px + cosf(a) * 4.5f, ty = have ? m->y : G.py + sinf(a) * 4.5f, d = clampf(hypotf(tx - G.px, ty - G.py), 1.2f, 9.0f);
        if ((s = new_shot(AT_FLASK, hx, hy, a, 7.0f, dmg))) { s->tier = tier; s->arc = s->life = d / 7.0f; s->target = 1000 + (int)(k * 100.0f); }
        snd_sfx_ex(SFX_WHOOSH, 0.8f, 0.5f);
        return;
    }
    case AT_SPEAR:
        if (rndf() >= bone_spared()) RUN.gear.bones--;
        if (align_has(EL_PHYS, ND_BULT)) {                         /* the Ballista: a heavy spear, twice as fast, up to thrice the blow */
            if ((s = new_shot(SHOT_BALLISTA, hx, hy, a, 24.0f, (dmg + (align_has(EL_PHYS, ND_A2) ? 0.5f * (float)gear_armor(g) : 0.0f)) * (1.0f + 2.0f * k)))) { s->tier = tier; s->life = 0.8f; }
            snd_sfx_ex(SFX_BONES, 1.3f, 0.5f); snd_sfx_ex(SFX_WHOOSH, 1.2f, 0.5f);
        } else {                                                   /* the Phalanx: a fan of spears, three, five at full, for one bone */
            int n = k < 0.75f ? 3 : 5;
            float sd = dmg + (align_has(EL_PHYS, ND_A2) ? 0.5f * (float)gear_armor(g) : 0.0f);
            for (int i = 0; i < n; i++)
                if ((s = new_shot(AT_SPEAR, hx, hy, a + ((float)i - (float)(n - 1) * 0.5f) * 0.16f, 12.0f, sd))) s->tier = tier;
            snd_sfx_ex(SFX_BONES, 1.1f, 0.5f); snd_sfx_ex(SFX_CAST, 1.0f, 0.5f);
        }
        return;
    default: cast_spell(sp, t); return;
    }
}
/* Soul Tether: the beam kept on what it holds, draining four times a second and slowing it; when that dies it
 * leaps to the next within four cells. Spirit by the second: a cast's worth a second */
static int web_strike(int sp, float dmg);
static void tether_end(void) { G.tether = 0; }
static void tether_hold(float dt) {
    const Gear *g = &RUN.gear;
    if (G.web_hp > 0.0f) {                                         /* held in a web: the beam tears at it */
        if ((G.tether_tick -= dt) <= 0.0f) { G.tether_tick = 0.25f; web_strike(AT_DRAIN, spell_damage_at(g, AT_DRAIN, spell_tier(AT_DRAIN)) * 0.3f); }
        return;
    }
    Mob *m = G.tether ? &G.mobs[G.tether - 1] : NULL;
    if (!m || !m->alive || m->state != MS_WALK || !line_clear(G.px, G.py, m->x, m->y) || hypotf(m->x - G.px, m->y - G.py) > 10.0f) {
        float fx = m ? m->x : G.px, fy = m ? m->y : G.py, bd = m ? 4.0f : 99.0f;
        int t = m ? -1 : aim_target();
        if (m) for (int i = 0; i < MAXMOBS; i++) {                 /* the held one gone: the next within four cells of it */
            const Mob *o = &G.mobs[i];
            if (o == m || !targetable(o)) continue;
            float d = hypotf(o->x - fx, o->y - fy);
            if (d < bd && line_clear(G.px, G.py, o->x, o->y)) { bd = d; t = i; }
        }
        G.tether = t >= 0 ? t + 1 : 0;
        if (!G.tether) return;
        m = &G.mobs[t];
    }
    if ((G.tether_tick -= dt) > 0.0f) return;
    G.tether_tick = 0.25f;
    DBG_COUNT(tether_ticks);
    int tier = spell_tier(AT_DRAIN);
    if (!spend_spirit(spell_cost(g, AT_DRAIN, tier) * 0.25f)) { tether_end(); G.chg_on = 0; say("LOW ON SPIRIT: THE TETHER SLIPS"); return; }
    float hx = G.px + cosf(G.ang) * 0.35f, hy = G.py + sinf(G.ang) * 0.35f, before = m->hp;
    add_beam(AT_DRAIN, hx, hy, 0.42f, m->x, m->y, m->z + 0.5f);
    damage_mob(m, spell_damage_at(g, AT_DRAIN, tier) * 0.3f, DM_ARC | DM_ATK(AT_DRAIN), G.px, G.py, 0.0f);
    if (m->alive && m->state == MS_WALK) m->slowed = fmaxf(m->slowed, 0.5f);
    float dealt = before - fmaxf(m->hp, 0.0f), heal = fminf(dealt * fminf(0.8f, 0.5f + 0.05f * tier) * spell_effect()
                                                             * (1.0f + ALIGN[EL_SPIRIT].n[ND_A2].val * (float)RUN.al[EL_SPIRIT][ND_A2]), (float)(G.max_hp - G.hp));   /* Communion */
    if (heal > 0.0f && spend_spirit(heal * heal_cost(g, tier))) { G.hp = clampi(G.hp + (int)(heal + 0.5f), 0, G.max_hp); pop_readout(); }
    if (rnd(3) == 0) snd_sfx_ex(SFX_WHISPER, 0.5f, 0.3f);
}
/* Firewall's spirits circling you, just inside a blow's reach; Flamewalker's armour wearing off and coming round again */
#define FWALL_R 1.2f
static void update_ultimates(float dt) {
    if (G.fwall_t > 0.0f) {
        G.fwall_t -= dt;
        for (int i = 0; i < 3; i++) {
            float an = (float)G.t * 2.2f + (float)i * TAU_F / 3.0f, sx = G.px + cosf(an) * FWALL_R, sy = G.py + sinf(an) * FWALL_R;
            for (int k = 0; k < MAXMOBS; k++) {
                Mob *m = &G.mobs[k];
                if (m->alive && m->state == MS_WALK && !KIND_IS(m, KF_SPIRIT) && !m->ally && !buried(m) && hypotf(m->x - sx, m->y - sy) < 0.3f + KINDS[m->type].rad)
                    mob_burn(m, G.fwall_dps, 5.0f);                /* a burn that doesn't stack with itself */
            }
        }
    }
    if (G.fwalk_t > 0.0f) { if ((G.fwalk_t -= dt) <= 0.0f) G.fwalk_cd = fmaxf(0.0f, 10.0f * (1.0f - 1.4f * (spell_effect() - 1.0f))); }
    else G.fwalk_cd = fmaxf(0.0f, G.fwalk_cd - dt);
    if (G.tether && !G.chg_on) tether_end();
}

static void web_miss(float x, float y);
static void web_land(Shot *s, Mob *m);
static void mice_shot(float x, float y);
static void update_shots(float dt) {
    const Gear *g = &RUN.gear;
    aug_show_tick(dt);                                             /* CRAWL_AUGSHOW's, for the sheet (D7; nothing otherwise) */
    for (int i = 0; i < MAXSHOTS; i++) {
        Shot *s = &G.shots[i];
        if (!s->alive) continue;
        s->life -= dt;
        aug_fly(s, dt);                                            /* its line's augments, in flight (17.3, 1.0.0b21) */
        if (s->kind == AT_WISPS) {                                 /* homing, round corners if need be */
            Mob *m = s->target >= 0 ? &G.mobs[s->target] : NULL;
            if (m && (!m->alive || m->state != MS_WALK)) { m = NULL; s->target = -1; }
            if (!m) {                                              /* find another */
                float bd = 6.0f;
                for (int k = 0; k < MAXMOBS; k++) {
                    Mob *o = &G.mobs[k];
                    if (!o->alive || o->state != MS_WALK || (o->type == MOB_GHOST && !spirit_mastery()) || o->ally) continue;
                    float d = hypotf(o->x - s->x, o->y - s->y);
                    if (d < bd && line_clear(s->x, s->y, o->x, o->y)) { bd = d; m = o; s->target = k; }
                }
            }
            if (m) {
                float gx = m->x, gy = m->y;
                int cx = (int)s->x, cy = (int)s->y;
                if (!line_clear(s->x, s->y, m->x, m->y) && wisp_field[cy][cx] > 0) {
                    for (int d = 0; d < 4; d++) {
                        int nx = cx + DX[d], ny = cy + DY[d];
                        if (wisp_field[ny][nx] >= 0 && wisp_field[ny][nx] < wisp_field[cy][cx]) { gx = nx + 0.5f; gy = ny + 0.5f; break; }
                    }
                }
                float want = atan2f(gy - s->y, gx - s->x), have = atan2f(s->vy, s->vx);
                have += clampf(wrap_pi(want - have), -6.0f * dt, 6.0f * dt);
                s->vx = cosf(have) * 4.5f; s->vy = sinf(have) * 4.5f;
            }
        }
        if (s->kind == SHOT_REFLECT && s->target >= 0) {          /* Mirrored: home on the thrower, round corners */
            Mob *m = &G.mobs[s->target];
            if (!m->alive || m->state != MS_WALK) s->target = -1;
            else {
                float gx = m->x, gy = m->y, sp = hypotf(s->vx, s->vy);
                int cx = (int)s->x, cy = (int)s->y;
                if (!line_clear(s->x, s->y, m->x, m->y)) {
                    field_bfs(reflect_field, m->cx, m->cy);
                    for (int d = 0; d < 4 && reflect_field[cy][cx] > 0; d++) {
                        int nx = cx + DX[d], ny = cy + DY[d];
                        if (reflect_field[ny][nx] >= 0 && reflect_field[ny][nx] < reflect_field[cy][cx]) { gx = nx + 0.5f; gy = ny + 0.5f; break; }
                    }
                }
                float want = atan2f(gy - s->y, gx - s->x), have = atan2f(s->vy, s->vx);
                have += clampf(wrap_pi(want - have), -7.0f * dt, 7.0f * dt);
                s->vx = cosf(have) * sp; s->vy = sinf(have) * sp;
            }
        }
        if (s->arc > 0.0f) { float u = clampf(1.0f - s->life / s->arc, 0.0f, 1.0f); s->z = (0.3f + 0.06f * s->arc * 7.0f) * 4.0f * u * (1.0f - u) - 0.42f * u; }   /* a lob */
        if (s->kind == AT_FIRE && s->tier >= 0 && align_has(EL_FIRE, ND_ABIL)) {   /* Firestarter: the floor alight along its flight */
            int step = (int)((1.6f - s->life) * 8.0f / 0.6f);
            if (step > (int)s->pierced && G.map[clampi((int)s->y, 0, MAPN - 1)][clampi((int)s->x, 0, MAPN - 1)].kind == K_FLOOR) {
                s->pierced = (uint64_t)step;
                for (int b = 0; b < MAXBURNS; b++)
                    if (!G.burns[b].alive) { G.burns[b] = (Burn){ 1, s->x, s->y, 0.0f, s->dmg * 0.25f, 2, 1.5f }; break; }
            }
        }
        if ((s->kind == SHOT_ORB || s->kind == SHOT_LANCE) && hypotf(s->x - G.lay_x[i], s->y - G.lay_y[i]) > 0.7f) {
            if (s->kind == SHOT_ORB) ionize(G.lay_x[i], G.lay_y[i], s->x, s->y, spell_damage_at(g, AT_CHAIN, s->tier), s->pierced);   /* EL4 */
            else rime(s->x, s->y);
            G.lay_x[i] = s->x; G.lay_y[i] = s->y;
        }
        float sdt = G.bslip_t > 0.0f && hypotf(s->x - G.bslip_x, s->y - G.bslip_y) < 3.0f ? dt * 0.2f : dt;   /* the Wraith's Time Slip */
        int n = 1 + (int)(sqrtf(s->vx * s->vx + s->vy * s->vy) * sdt / 0.08f);
        for (int k = 0; k < n && s->alive; k++) {
            float nx = s->x + s->vx * sdt / n, ny = s->y + s->vy * sdt / n;
            if (blocks_light((int)nx, (int)ny) || cell((int)nx, (int)ny)->kind == K_PILLAR) {
                if (s->kind == AT_WISPS) {                         /* wisps slide along the stone */
                    int bx = blocks_light((int)nx, (int)s->y) || cell((int)nx, (int)s->y)->kind == K_PILLAR;
                    int by = blocks_light((int)s->x, (int)ny) || cell((int)s->x, (int)ny)->kind == K_PILLAR;
                    if (bx) s->vx = -s->vx * 0.5f;
                    if (by || !bx) s->vy = -s->vy * 0.5f;
                    break;
                }
                s->alive = 0;
                if (s->kind == SHOT_WEB) web_miss(s->x, s->y);
                else if (s->kind == AT_HELL) { explode_big(s->x, s->y, 1.6f); splash(s->x, s->y, 1.5f, s->dmg, NULL, AT_HELL); }
                else if (s->kind == AT_FROST) sfx_at(SFX_FROST, s->x, s->y, 0.6f);
                else if (s->kind == SHOT_REFLECT && s->z < 0.5f) sfx_at(SFX_BONES, s->x, s->y, 0.4f);
                else if (s->kind == AT_FLASK) { s->alive = 1; flask_burst(s); }
                else if (s->kind == SHOT_ORB) {                    /* the orb against the stone: a gold flash */
                    sfx_at(SFX_ZAP, s->x, s->y, 1.0f);
                    for (int b = 0; b < MAXBOOMS; b++)
                        if (!G.booms[b].alive) { G.booms[b] = (Boom){ 1, s->x, s->y, 0.0f, 1.3f, 4 }; break; }
                }
                else if (s->kind == SHOT_LANCE) sfx_at(SFX_SHATTER, s->x, s->y, 0.7f);
                else if (s->kind == SHOT_BALLISTA) sfx_at(SFX_BONES, s->x, s->y, 0.9f);
                else if (s->kind == SHOT_SPLINTER) sfx_at(SFX_BONES, s->x, s->y, 0.25f);
                else if (s->kind != AT_SPEAR) explode(s->x, s->y);
                break;
            }
            s->x = nx; s->y = ny;
            if (s->tier >= 0) mice_shot(s->x, s->y);                /* from level 15 a messenger in its way dies (docs/PLAN-mice.md) */
            if (mk_shot(s)) {                                      /* a thing that wants a spell takes it */
                s->alive = 0;
                if (s->kind == AT_FIRE || s->kind == AT_HELL) explode(s->x, s->y); else sfx_at(s->kind == AT_FROST || s->kind == SHOT_LANCE ? SFX_FROST : SFX_BONES, s->x, s->y, 0.5f);
                break;
            }
            for (int mi = 0; mi < MAXMOBS && s->tier >= 0 && (s->kind != AT_FLASK || s->arc <= 0.0f); mi++) {   /* tier -1: a broken ward's
                                                                                                shards, only for show; a lobbed flask goes over
                                                                                                them, one thrown flat (Straight Throw) doesn't */
                Mob *mb = &G.mobs[mi];
                if (!mb->alive || mb->state != MS_WALK || mb->ally) continue;   /* your own pass */
                if (mb->type == MOB_GHOST && !(s->kind == AT_WISPS && spirit_mastery())) continue;   /* fire passes through spectres; Wisp Host's don't */
                int pierce = s->kind == AT_SPEAR || s->kind == SHOT_ORB || s->kind == SHOT_LANCE || s->kind == SHOT_BALLISTA || s->kind == AT_FROST;   /* a frost
                                                                      shard: only Glacial Weight's ever passes one */
                if (pierce && (s->pierced >> mi & 1)) continue;
                float hr = KINDS[mb->type].rad * (mb->boss ? BOSSES[mb->boss - 1].scale : 1.0f) + (s->kind == SHOT_ORB ? 0.3f : 0.0f);
                if (hypotf(mb->x - s->x, mb->y - s->y) >= hr) continue;
                if (mb->boss && (G.bdash_t > 0.0f || (mb->type == MOB_HUNTER && (mb->sub == 0 || mb->sub == 5)) || !wraith_open(mb))) continue;   /* darting, or
                                                                                                                         overhead: under it */
                if (knight_turns(mb, s)) break;
                float fx = s->x - s->vx * 0.1f, fy = s->y - s->vy * 0.1f;
                switch (s->kind) {
                case AT_SPEAR: {                                   /* through everything in the line, shields too */
                    int first = !s->pierced;
                    if (!s->pierced && align_has(EL_PHYS, ND_ABIL) && mb->alive) {   /* Impale: the first it strikes, pinned */
                        if (mb->boss) mb->stagger = fmaxf(mb->stagger, 0.4f); else mb->pin = fmaxf(mb->pin, 0.6f);
                    }
                    s->pierced |= 1ull << mi;
                    G.d7_spear = s->dmg;                           /* for Bone Rain (D7) */
                    damage_mob(mb, s->dmg, DM_PIERCE | DM_ATK(AT_SPEAR), fx, fy, 0.0f);
                    float k;
                    if (first && (k = dk(EL_PHYS, 1, 2)) > 0.0f && mb->alive && mb->state == MS_WALK) mb->stagger = fmaxf(mb->stagger, 0.5f * k);   /* Heavy Haft */
                    if ((k = dk(EL_PHYS, 2, 1)) > 0.0f) s->dmg *= 1.0f + 0.15f * k;   /* Skewer: more for the next */
                    if (align_has(EL_PHYS, ND_A2)) {                   /* Ossified: a point of armour, six seconds */
                        int o = 0;
                        for (int k = 1; k < 10; k++) if (G.oss_t[k] < G.oss_t[o]) o = k;
                        G.oss_t[o] = 6.0f;
                    }
                    sfx_at(SFX_BONES, mb->x, mb->y, 0.5f);
                    continue;
                }
                case AT_HELL:
                    s->alive = 0; explode_big(s->x, s->y, 1.6f);
                    damage_mob(mb, s->dmg, DM_ATK(AT_HELL), fx, fy, 0.0f); splash(s->x, s->y, 1.5f, s->dmg, mb, AT_HELL);
                    for (int b = 0; b < MAXBURNS; b++)
                        if (!G.burns[b].alive) { G.burns[b] = (Burn){ 1, s->x, s->y, 0.0f, (4.0f + s->tier) * spell_effect(), 0, 2.5f }; break; }
                    break;
                case AT_FIRE:
                    s->alive = 0; explode(s->x, s->y);
                    damage_mob(mb, s->dmg, DM_ATK(AT_FIRE), fx, fy, 0.0f);
                    if (s->tier >= 2) splash(s->x, s->y, 0.75f, s->dmg * 0.5f, mb, AT_FIRE);
                    break;
                case AT_FROST: {
                    int heavy = s->target == -3;                   /* Glacial Weight's, through the first it strikes (D7) */
                    float d0 = s->dmg, k;
                    if (!heavy) s->alive = 0;
                    sfx_at(SFX_FROST, s->x, s->y, 0.8f);
                    if (!damage_mob(mb, s->dmg, DM_SLOW | DM_ATK(AT_FROST), fx, fy, (2.5f + 0.25f * s->tier) * spell_effect() * permafrost())) deep_chill(mb);
                    if (s->target != -2 && (k = dk(EL_FROST, 0, 2)) > 0.0f) frost_splinter(s, mb, d0 * 0.3f * k);   /* Splinterfrost */
                    if (heavy) { s->pierced |= 1ull << mi; s->target = -1; s->dmg *= 0.7f * dk(EL_FROST, 1, 2); continue; }
                    break;
                }
                case SHOT_WEB: web_land(s, mb); break;             /* your Tangled Web */
                case AT_FLASK: flask_burst(s); break;              /* thrown flat (Straight Throw, D7): it bursts on what it meets */
                case SHOT_SPLINTER:                                /* a splinter: the first it meets, and a shield stops it */
                    s->alive = 0;
                    G.d7_splint = 1;                               /* no bleed, bone or rain from it (D7) */
                    damage_mob(mb, s->dmg, DM_ATK(AT_SPEAR), fx, fy, 0.0f);
                    G.d7_splint = 0;
                    sfx_at(SFX_BONES, mb->x, mb->y, 0.3f);
                    break;
                case SHOT_ORB: {                                   /* Amperage: through it, and an arc to one more not yet struck */
                    s->pierced |= 1ull << mi;
                    damage_mob(mb, s->dmg, DM_ARC | DM_ATK(AT_CHAIN), fx, fy, 0.0f);
                    if (mb->alive && mb->state == MS_WALK) daze(mb);
                    sfx_at(SFX_ZAP, mb->x, mb->y, 0.8f);
                    int best = -1; float bd = 3.5f;
                    for (int k = 0; k < MAXMOBS; k++) {
                        const Mob *o = &G.mobs[k];
                        if ((s->pierced >> k & 1) || !targetable(o)) continue;
                        float d = hypotf(o->x - mb->x, o->y - mb->y);
                        if (d < bd && d > 0.3f && line_clear(mb->x, mb->y, o->x, o->y)) { bd = d; best = k; }
                    }
                    if (best >= 0) {                               /* EL1: one, and only one */
                        Mob *o = &G.mobs[best];
                        s->pierced |= 1ull << best;
                        add_beam(AT_CHAIN, mb->x, mb->y, mb->z + 0.5f, o->x, o->y, o->z + 0.5f);
                        damage_mob(o, s->dmg * 0.5f, DM_ARC | DM_ATK(AT_CHAIN), mb->x, mb->y, 0.0f);
                        if (o->alive && o->state == MS_WALK) daze(o);
                        ionize(mb->x, mb->y, o->x, o->y, spell_damage_at(g, AT_CHAIN, s->tier), s->pierced);
                    }
                    DBG_COUNT(orb_hits);
                    continue;
                }
                case SHOT_LANCE:                                   /* Winter's Lance: through every one, each frozen */
                    s->pierced |= 1ull << mi;
                    damage_mob(mb, s->dmg, DM_PIERCE | DM_SLOW | DM_ATK(AT_FROST), fx, fy, 2.5f * spell_effect() * permafrost());
                    if (mb->alive && mb->state == MS_WALK) {
                        if (mb->boss) mb->slowed = fmaxf(mb->slowed, 3.0f * permafrost());
                        else { mb->frozen = fmaxf(mb->frozen, 2.0f * permafrost()); mb->swing = 0.0f; mb->throw_t = 0.0f; }
                    }
                    sfx_at(SFX_FROST, mb->x, mb->y, 1.0f);
                    continue;
                case SHOT_BALLISTA: {                              /* the Ballista: through each, a cell back and stunned */
                    s->pierced |= 1ull << mi;
                    damage_mob(mb, s->dmg, DM_PIERCE | DM_ATK(AT_SPEAR), fx, fy, 0.0f);
                    if (mb->alive && mb->state == MS_WALK) {
                        if (mb->boss) mb->stagger = fmaxf(mb->stagger, 0.5f);
                        else {
                            float sp = hypotf(s->vx, s->vy) + 1e-4f, ux = s->vx / sp, uy = s->vy / sp;
                            for (int st = 0; st < 10; st++) {      /* a cell back, short of the stone */
                                float nx = mb->x + ux * 0.1f, ny = mb->y + uy * 0.1f;
                                if (blocks_move((int)nx, (int)ny) || occupied((int)nx, (int)ny, mb) || (G.map[(int)ny][(int)nx].kind == K_DOOR && !passable((int)nx, (int)ny))) break;
                                mb->x = nx; mb->y = ny;
                            }
                            mb->cx = mb->tx = (int)mb->x; mb->cy = mb->ty = (int)mb->y;
                            mb->pin = fmaxf(mb->pin, 1.0f); mb->swing = 0.0f; mb->throw_t = 0.0f;
                        }
                    }
                    sfx_at(SFX_BONES, mb->x, mb->y, 0.9f);
                    continue;
                }
                default:                                           /* sparks, wisps, reflected missiles */
                    s->alive = 0;
                    if (s->kind == SHOT_REFLECT) explode(s->x, s->y); else sfx_at(SFX_ZAP, s->x, s->y, 0.35f);
                    damage_mob(mb, s->dmg, s->kind == SHOT_REFLECT ? 0 : DM_ATK(s->kind), fx, fy, 0.0f);
                    break;
                }
                break;
            }
        }
        if (s->alive && s->life <= 0) { s->alive = 0; if (s->kind == AT_FIRE || s->kind == AT_HELL) explode(s->x, s->y); if (s->kind == SHOT_WEB) web_miss(s->x, s->y);
                                         if (s->kind == AT_FLASK) { s->alive = 1; flask_burst(s); } }
    }
    for (int i = 0; i < MAXBOOMS; i++) if (G.booms[i].alive && (G.booms[i].t += dt) >= 0.6f) G.booms[i].alive = 0;
    for (int i = 0; i < MAXBEAMS; i++) if (G.beams[i].alive && (G.beams[i].t += dt) >= 0.3f) G.beams[i].alive = 0;
    update_trails(dt);
    update_venoms(dt);
    update_motes(dt);
    for (int i = 0; i < MAXBURNS; i++) {                           /* the floor burns a while: hellfire, embers, Firestarter */
        Burn *b = &G.burns[i];
        if (!b->alive) continue;
        float pt = b->t; b->t += dt;
        if (b->t >= b->life) { b->alive = 0; continue; }
        if (b->foe == 2 && G.fwalk_t <= 0.0f && G.fwalk_cd <= 0.0f && align_has(EL_FIRE, ND_AULT) && hypotf(G.px - b->x, G.py - b->y) < 0.5f) {
            G.fwalk_t = 5.0f;                                      /* Flamewalker: half your armour again, five seconds */
            snd_sfx_ex(SFX_WHOOSH, 0.7f, 0.4f);
            DBG_COUNT(flamewalks);
        }
        if (b->foe == 2) {                                         /* Firestarter's: what walks through catches fire */
            for (int k = 0; k < MAXMOBS; k++) {
                Mob *m = &G.mobs[k];
                if (m->alive && m->state == MS_WALK && !KIND_IS(m, KF_SPIRIT) && m->type != MOB_WSKULL && !m->ally && !buried(m)
                    && hypotf(m->x - b->x, m->y - b->y) < 0.45f + KINDS[m->type].rad * 0.5f) mob_burn(m, b->dps, burning_time());
            }
            continue;
        }
        if (b->foe) {                                              /* an ember's: it burns whoever stands in it */
            if ((int)(pt * 2.0f) != (int)(b->t * 2.0f) && hypotf(G.px - b->x, G.py - b->y) < 0.6f) {
                G.blame = (Blame){ BL_FIRE, MOB_IMP, 0 };
                if (hurt_ex(b->dps * 0.5f, HURT_SPELL, b->x, b->y) > 0) DBG_COUNT(ember_scorch);
            }
            continue;
        }
        if ((int)(pt * 4.0f) != (int)(b->t * 4.0f))
            for (int k = 0; k < MAXMOBS; k++) {
                Mob *m = &G.mobs[k];
                if (m->alive && m->state == MS_WALK && !KIND_IS(m, KF_SPIRIT) && m->type != MOB_WSKULL && !m->ally && hypotf(m->x - b->x, m->y - b->y) < 0.7f)
                    damage_mob(m, b->dps * 0.25f, DM_ARC | DM_ATK(AT_HELL), b->x, b->y, 0.0f);
            }
    }
    (void)g;
}

