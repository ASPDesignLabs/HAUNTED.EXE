/* A part of scene_crawl.c (docs/PLAN-tidy.md): it is #included there in its place and never compiled on its own.
 * The caster's defences: runes, each waiting for its moment (def_trigger, cast_defense), update_status. */

/* ---- the caster's defences: tattooed runes, each waiting for its moment ---- */

static int c4_tells(float within);
static int incoming(float within) {                                 /* missiles that will reach the caster soon */
    int n = c4_tells(within);                                      /* and a Bone Thrower's flash, as good as one (C4, 8.6) */
    for (int i = 0; i < MAXBONES; i++) {
        const Bone *b = &G.bones[i];
        if (!b->alive) continue;
        float dx = G.px - b->x, dy = G.py - b->y, sp = hypotf(b->vx, b->vy) + 1e-3f;
        float along = (dx * b->vx + dy * b->vy) / sp, off = fabsf(dx * b->vy - dy * b->vx) / sp;
        if (along > 0.0f && off < 0.6f && along / sp < within) n++;
    }
    return n;
}
static int enemies_within(float r, int undead_only) {
    int n = 0;
    for (int i = 0; i < MAXMOBS; i++) {
        const Mob *m = &G.mobs[i];
        if (!m->alive || m->state != MS_WALK || m->type == MOB_GHOST || m->type == MOB_WSKULL || m->type == MOB_SAC || m->ally || m->webbed > 0.0f) continue;
        if ((m->type == MOB_SPIDER || m->type == MOB_GARGOYLE) && m->sub == 0) continue;
        if (buried(m)) continue;                                   /* still under the earth: nothing to fight yet (the owner's word, 9 November) */
        if (undead_only && !KIND_IS(m, KF_UNDEAD)) continue;
        if (hypotf(m->x - G.px, m->y - G.py) < r && line_clear(m->x, m->y, G.px, G.py)) n++;
    }
    return n;
}
static int skull_charging(float r, float after) {
    for (int i = 0; i < MAXMOBS; i++) {
        const Mob *m = &G.mobs[i];
        if (m->alive && m->type == MOB_WSKULL && m->state == MS_WALK && m->sub == 1 && m->sub_t >= after
            && hypotf(m->x - G.px, m->y - G.py) < r && line_clear(m->x, m->y, G.px, G.py)) return 1;
    }
    return 0;
}
static int melee_close(void) {                                      /* a sword raised, or a spider coming down */
    for (int i = 0; i < MAXMOBS; i++) {
        const Mob *m = &G.mobs[i];
        if (!m->alive || m->state != MS_WALK || m->ally || m->webbed > 0.0f) continue;
        float d = hypotf(m->x - G.px, m->y - G.py);
        if ((m->type == MOB_SHIELD && d < 1.5f) || (m->type == MOB_SPIDER && m->sub >= 1 && m->sub <= 2 && d < 1.4f)) return 1;
    }
    return 0;
}
static int in_combat(void) { return enemies_within(7.0f, 0) > 0 || incoming(2.0f) > 0 || skull_charging(9.0f, 0.0f); }

static int def_trigger(int k) {
    float hpf = (float)G.hp / (float)G.max_hp;
    switch (k) {
    case DF_WARD: return G.ward_hp <= 0.0f && (incoming(0.7f) || skull_charging(7.0f, 1.5f) || melee_close());
    case DF_BLINK: return (melee_close() && hpf < 0.6f) || (incoming(0.4f) && G.ward_hp <= 0.0f && G.mirror_t <= 0.0f && hpf < 0.5f)
                          || (af_rooted() && G.af[af_on(AF_FROZEN) ? AF_FROZEN : AF_DOWN] > 0.6f && in_combat());   /* off the floor, out of the ice */
    case DF_MIRROR: return G.mirror_t <= 0.0f && (incoming(1.5f) >= 2 || skull_charging(9.0f, 2.0f));
    case DF_MEND: return G.mend_t <= 0.0f && hpf < 0.45f;
    case DF_PURIFY: return G.poison_t > 2.0f || G.slow_t > 1.5f || af_on(AF_FROZEN) || G.af_n >= 2 || G.af_burn > 0.1f * (float)G.max_hp || G.af[AF_POISON] > 8.0f;
    case DF_NOVA: return enemies_within(1.8f, 0) >= 2 || melee_close();
    case DF_DREAD: return enemies_within(4.0f, 1) >= 2;
    case DF_SLIP: return G.slip_t <= 0.0f && (enemies_within(5.0f, 0) + incoming(2.0f) >= 3 || (hpf < 0.35f && in_combat()));
    default: return G.reap_t <= 0.0f && enemies_within(7.0f, 0) >= 2;
    }
}

static void begin_turn(float target);
static float heading_angle(int h);
static int decide(int x, int y, int h);

/* Blink: two to six steps to wherever is furthest from trouble, out of sight if it can */
#define DECOY_T 3.0f                        /* how long an Exquisite Blink's double holds them */
static void latch_release(int thrown);
static void blink(void) {
    latch_release(1);                                              /* a bat on your face is thrown off */
    static int dist[MAPN][MAPN], qx[MAPN * MAPN], qy[MAPN * MAPN];
    int range = clampi(2 + rune_tier(RUN.gear.def[DF_BLINK]) / 3, 2, 6);
    for (int y = 0; y < MAPN; y++) for (int x = 0; x < MAPN; x++) dist[y][x] = -1;
    int qh = 0, qt = 0, bx = -1, by = -1; float bs = -1e9f;
    dist[G.cy][G.cx] = 0; qx[qt] = G.cx; qy[qt] = G.cy; qt++;
    while (qh < qt) {
        int x = qx[qh], y = qy[qh]; qh++;
        if (dist[y][x] >= 2 && G.map[y][x].kind == K_FLOOR && !occupied(x, y, NULL)) {
            float s = dist[y][x] * 0.3f;
            for (int i = 0; i < MAXMOBS; i++) {
                const Mob *m = &G.mobs[i];
                if (!m->alive || m->state != MS_WALK || m->type == MOB_GHOST || m->ally) continue;
                float d = hypotf(m->x - x - 0.5f, m->y - y - 0.5f);
                s += fminf(d, 6.0f) * 0.5f - (line_clear(m->x, m->y, x + 0.5f, y + 0.5f) ? 1.5f : 0.0f);
            }
            if (s > bs) { bs = s; bx = x; by = y; }
        }
        if (dist[y][x] >= range) continue;
        for (int d = 0; d < 4; d++) {
            int nx = x + DX[d], ny = y + DY[d];
            if (!passable(nx, ny) || dist[ny][nx] >= 0 || cell(nx, ny)->kind == K_DOOR) continue;
            dist[ny][nx] = dist[y][x] + 1; qx[qt] = nx; qy[qt] = ny; qt++;
        }
    }
    if (bx < 0) return;
    if (RUN.gear.def[DF_BLINK] >= RUNE_EXQ) {                      /* Exquisite: a double stays */
        G.decoy_x = G.px; G.decoy_y = G.py; G.decoy_t = DECOY_T;
#ifdef CRAWL_DEBUG
        int nl = 0;
        for (int i = 0; i < MAXMOBS; i++) nl += G.mobs[i].alive && G.mobs[i].state == MS_WALK && hypotf(G.mobs[i].x - G.px, G.mobs[i].y - G.py) < 6.0f;
        fprintf(stderr, "EVENT decoy near=%d t=%.2f\n", nl, G.t);
#endif
    }
    G.cx = G.tx = bx; G.cy = G.ty = by; G.px = bx + 0.5f; G.py = by + 0.5f; G.u = 1.0f;
    G.spd = 0.0f;
    if (!G.player) {
        G.hand = 0.0f;
        G.next_h = decide(G.cx, G.cy, G.h);
        begin_turn(heading_angle(G.next_h)); G.state = PS_TURN;
    }
    G.chill = fmaxf(G.chill, 0.5f);
    snd_sfx_ex(SFX_WHOOSH, 1.0f, 0.5f);
}

/* A rune a person raises by hand (1.0.0b16: the Mirror; the rest come later, docs/PLAN-play.md Q9); the dream's all rise
 * by themselves as before */
static int rune_by_hand(int k) { return G.player && k == DF_MIRROR; }
#define MIRROR_TRUE 0.6f                    /* the parry: what the Mirror meets this soon after it is raised goes home, twice as hard */
static void cast_defense(int k) {
    int q = RUN.gear.def[k], t = rune_tier(q);                     /* by its quality, at about the tier it once was */
    if (q <= 0) return;
    G.def_cd[k] = def_cooldown(q);
    G.rune_glow[k] = 1.4f; G.rune_lift = 1.4f;
    G.rs_on[k] = 1; G.rs_since[k] = 0.0f;                          /* its socket comes up */
    snd_sfx_ex(SFX_RUNE, 0.9f, 0.5f);
#ifdef CRAWL_DEBUG
    defs[k]++;
    fprintf(stderr, "EVENT defense %s q=%d t=%.2f hp=%d\n", DEF_NAME[k], q, G.t, G.hp);
#endif
    float eff = spell_effect();                                    /* the runes' strength */
    switch (k) {
    case DF_WARD: G.ward_hp = (10.0f + 5.0f * t) * eff; G.ward_t = 12.0f; G.ward_shards = q >= RUNE_EXQ ? G.ward_hp * 0.5f : 0.0f; break;
    case DF_BLINK: af_break(); blink(); break;
    case DF_MIRROR: G.mirror_t = (3.0f + 0.5f * t) * eff; if (G.player) G.mirror_true = MIRROR_TRUE;
#ifdef CRAWL_DEBUG
        if (G.player && getenv("CRAWL_PARRYT")) G.mirror_true = (float)atof(getenv("CRAWL_PARRYT"));   /* testing: the parry's window */
#endif
        break;
    case DF_MEND: G.mend_t = 4.0f; G.mend_rate = (15.0f + 5.0f * t) / 4.0f * eff; break;
    case DF_PURIFY: G.poison_t = G.slow_t = 0.0f; G.purify_t = (4.0f + t) * eff; af_purify(); break;
    case DF_NOVA:
        for (int i = 0; i < MAXMOBS; i++) {
            Mob *m = &G.mobs[i];
            if (m->alive && m->state == MS_WALK && !KIND_IS(m, KF_SPIRIT) && !m->ally && hypotf(m->x - G.px, m->y - G.py) < 1.8f + 0.1f * t)
                { m->frozen = (2.0f + 0.3f * t) * eff; m->swing = 0.0f; m->throw_t = 0.0f; }
        }
        snd_sfx_ex(SFX_FROST, 1.3f, 0.5f);
        G.nova_t = 0.001f; G.chill = fmaxf(G.chill, 0.55f);
        break;
    case DF_DREAD:
        for (int i = 0; i < MAXMOBS; i++) {
            Mob *m = &G.mobs[i];
            int dead = KIND_IS(m, KF_UNDEAD) || m->type == MOB_WSKULL || m->type == MOB_WRAITH;
            int any = q >= RUNE_EXQ && !KIND_IS(m, KF_SPIRIT) && !buried(m);   /* Exquisite: the living too */
            if (m->alive && m->state == MS_WALK && (dead || any) && !m->ally && hypotf(m->x - G.px, m->y - G.py) < 5.0f)
                { m->feared = (3.0f + 0.5f * t) * eff; m->wfeared = 0; m->dreaded = !dead; if (!dead) DBG_COUNT(exq_dread); }
        }
        G.dread_t = 0.001f;
        snd_sfx_ex(SFX_MOAN, 1.2f, 0.5f);
        break;
    case DF_SLIP: G.slip_t = (3.0f + 0.4f * t) * eff; break;
    default: G.reap_t = (15.0f + t) * eff; break;
    }
}

/* The runes in sight (docs/PLAN-b11.md, section 3): fired, a rune's socket comes up for three seconds, or
 * while its work lasts if that is longer, the mark glowing until it ends; two seconds before it is ready it
 * comes back, flashing; ready, it holds a second, its line bright, and goes. The newest stands outermost,
 * and each keeps its place while it shows. */
enum { RS_HIDDEN, RS_FIRED, RS_FLASH, RS_READY };
static float rune_work(int k) {                                    /* how long the rune's work still lasts */
    switch (k) {
    case DF_WARD: return G.ward_hp > 0.0f ? G.ward_t : 0.0f;
    case DF_BLINK: return G.decoy_t;
    case DF_MIRROR: return G.mirror_t;
    case DF_MEND: return G.mend_t;
    case DF_PURIFY: return G.purify_t;
    case DF_SLIP: return G.slip_t;
    case DF_REAP: return G.reap_t;
    default: return 0.0f;                                          /* the nova and Dread are done as they go off */
    }
}
static int rune_phase(int k) {
    if (!G.rs_on[k] || RUN.gear.def[k] <= 0) return RS_HIDDEN;
    if (G.rs_since[k] < 3.0f || rune_work(k) > 0.0f) return RS_FIRED;
    if (G.def_cd[k] > 0.0f) return G.def_cd[k] <= 2.0f ? RS_FLASH : RS_HIDDEN;
    return G.def_cd[k] > -1.0f ? RS_READY : RS_HIDDEN;
}
static void rune_sockets_tick(float dt) {
    int top = 0;
    for (int k = 0; k < NDEF; k++) top = imax(top, G.rs_slot[k]);
    for (int k = 0; k < NDEF; k++) {
        if (!G.rs_on[k]) { G.rs_a[k] = 0.0f; G.rs_slot[k] = 0; continue; }
        G.rs_since[k] += dt;
        int ph = rune_phase(k);
        G.rs_a[k] = ph ? fminf(1.0f, G.rs_a[k] + dt * 6.0f) : fmaxf(0.0f, G.rs_a[k] - dt * 4.0f);
        if (G.rs_a[k] > 0.0f && !G.rs_slot[k]) G.rs_slot[k] = (uint8_t)++top;   /* the newest outermost */
        if (G.rs_a[k] <= 0.0f && !ph) { G.rs_slot[k] = 0; if (G.def_cd[k] <= -1.0f) G.rs_on[k] = 0; }
    }
}

/* each rune watches for its moment; the caster is only human: sometimes slow, sometimes not at all */
static void update_defense(float dt) {
    const Gear *g = &RUN.gear;
    int fighting = in_combat();
    for (int k = 0; k < NDEF; k++) {
        if (g->def[k] <= 0) continue;
        G.def_cd[k] -= dt; G.def_hush[k] -= dt;
        if (rune_by_hand(k)) { G.def_wait[k] = 0.0f; continue; }   /* a person's to raise */
        if (G.def_wait[k] > 0.0f) {
            G.def_wait[k] -= dt;
            if (G.def_wait[k] <= 0.0f && G.def_cd[k] <= 0.0f && !G.dead && !rune_barred(k)) cast_defense(k);
            continue;
        }
        if (G.def_cd[k] > 0.0f || G.def_hush[k] > 0.0f || G.dead || rune_barred(k)) continue;   /* the void, or a status: no rune fires */
        if (def_trigger(k)) {
            if (rndf() < 0.12f) { G.def_hush[k] = 1.5f; continue; }   /* missed the moment */
            G.def_wait[k] = 0.15f + 0.55f * rndf() + (rndf() < 0.1f ? 0.8f : 0.0f);
        } else if (fighting && rndf() < dt * 0.004f) cast_defense(k);   /* a nervous, wasted cast */
    }
    rune_sockets_tick(dt);
}

/* poison, slow and the timed spells wearing off; mending and blessed regeneration */
static void update_status(float dt) {
    splint_tick(dt);                                               /* splinters grow back */
    if (G.poison_t > 0.0f) {
        G.poison_t -= dt * soft_pace();                            /* Survival runs it down sooner */
        G.poison_acc += G.poison_dps * dt;
        while (G.poison_acc >= 1.0f && !G.dead) { G.poison_acc -= 1.0f; G.blame = G.poison_by; hurt_ex(1.0f, HURT_POISON, G.px, G.py); }
    } else G.poison_acc = 0.0f;
    G.slow_t = fmaxf(0.0f, G.slow_t - dt * soft_pace());          /* grabs, webs, bile: Survival runs them down sooner */
    G.purify_t = fmaxf(0.0f, G.purify_t - dt);
    if (G.ward_t > 0.0f && (G.ward_t -= dt) <= 0.0f) G.ward_hp = G.ward_shards = 0.0f;   /* it fades unbroken */
    G.decoy_t = fmaxf(0.0f, G.decoy_t - dt);
    G.mirror_t = fmaxf(0.0f, G.mirror_t - dt); G.mirror_flash = fmaxf(0.0f, G.mirror_flash - dt); G.mirror_true = fmaxf(0.0f, G.mirror_true - dt);
    G.slip_t = fmaxf(0.0f, G.slip_t - dt);
    G.reap_t = fmaxf(0.0f, G.reap_t - dt);
    for (int i = 0; i < 10; i++) G.oss_t[i] = fmaxf(0.0f, G.oss_t[i] - dt);
    update_ultimates(dt);
    update_latch(dt);
    for (int k = 0; k < NDEF; k++) G.rune_glow[k] = fmaxf(0.0f, G.rune_glow[k] - dt);
    G.rune_lift = fmaxf(0.0f, G.rune_lift - dt);
    if (G.nova_t > 0.0f && (G.nova_t += dt) > 0.6f) G.nova_t = 0.0f;
    if (G.dread_t > 0.0f && (G.dread_t += dt) > 0.8f) G.dread_t = 0.0f;
    float heal = 0.0f, mend = 0.0f;
    if (G.mend_t > 0.0f) { G.mend_t -= dt; mend = G.mend_rate * dt; heal += mend; }
    if (G.purify_t > 0.0f && RUN.gear.def[DF_PURIFY] >= RUNE_EXQ) heal += (float)G.max_hp * 0.02f * dt;   /* Exquisite Purify mends too */
    heal += gear_regen(&RUN.gear) * dt;
    if (mend > 0.0f && G.hp >= G.max_hp && !G.dead && RUN.gear.def[DF_MEND] >= RUNE_EXQ) {   /* Exquisite Mend: past full, a small ward */
        float cap = (float)G.max_hp * 0.25f;
        if (G.ward_hp < cap) {
            if (G.ward_hp <= 0.0f) DBG_COUNT(exq_mendward);
            G.ward_hp = fminf(cap, G.ward_hp + mend); G.ward_t = fmaxf(G.ward_t, 8.0f);
        }
    }
    if (heal > 0.0f && G.hp < G.max_hp && !G.dead) {
        G.regen_acc += heal;
        if (G.regen_acc >= 1.0f) { int h = (int)G.regen_acc; G.regen_acc -= h; G.hp = clampi(G.hp + h, 0, G.max_hp); if (G.mend_t > 0.0f) pop_readout(); }
    }
}

