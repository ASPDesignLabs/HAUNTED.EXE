/* A part of scene_crawl.c (docs/PLAN-tidy.md): it is #included there in its place and never compiled on its own.
 * Harm that reaches the caster: its one road (el_resist, armour_keep), the caster's statuses (af_*), hurt_ex, the dead's blows and bones (foe_hit, throw_to, update_bones); then souls: the vessel, motes, spirit spent. */

static int harm_of(int kind) {
    int e = harm_next;
    harm_next = -1;
    if (e >= 0) return e;
    return kind == HURT_POISON ? EL_POISON : kind == HURT_SPELL ? EL_FIRE : kind == HURT_VOID ? -1 : EL_PHYS;
}
static void rec_harm(Blame b, int el) {
    int k = b.what == BL_MOB || b.what == BL_POISON || b.what == BL_FIRE || b.what == BL_POOL ? b.type : -1;
    if (k < 0 || k >= REC_KINDS || el < 0 || RUN.rec.harms[k][el] >= 255) return;
    RUN.rec.harms[k][el]++;
}
/* A blow that lands while the hand is gathering a spell may break it (docs/PLAN-b11.md, section 7, K3): the
 * spell is lost and nothing spent. A quarter, and twice the share of your health it took, at most nine in ten;
 * kept back by Dodge in full, three fifths of what Armour took off, two fifths of your resistance to its
 * element, and a quarter of Luck, together nine tenths at most. What seeps and what burns underfoot never do */
static int winding_up(void) { return G.player ? (G.pc_t >= 0.0f && !G.pc_fired) || G.chg_on : G.state == PS_CAST && !G.fired; }
static void windup_break(void) {                                   /* the spell gathered is lost, and nothing spent */
    if (G.player) { G.pc_t = -1.0f; G.cooldown = 0.0f; G.hand_base = fminf(G.hand_base, 0.3f); G.chg_on = 0; G.tether = 0; }   /* begin again at once; a charge lost */
    else { G.st = G.cast_len; G.fired = 2; G.auto_chg = 0; G.chg_on = 0; G.tether = 0; }
    snd_sfx_ex(SFX_KNOCK, 0.5f, 0.5f);
}
/* an element's own resist (docs/PLAN-combat.md, section 3): Fire by Resist Burn, Frost by Resist Slow, Poison, Spirit and
 * Lightning by theirs; Physical by none */
static float el_resist(const Gear *g, int el) {
    switch (el) {
    case EL_FIRE: return gear_magic_resist(g);
    case EL_FROST: return gear_slow_resist(g);
    case EL_POISON: return gear_poison_resist(g);
    case EL_SPIRIT: return gear_total(g, ST_RES_SPIRIT) / 100.0f;
    case EL_LIGHT: return gear_light_resist(g);
    default: return 0.0f;
    }
}
/* the share armour keeps off (P2): armour as great as a rank-I blow of the level keeps off half of any blow, whatever the
 * level, as a flat number never could (it grew as the blow did, till one robe left a blow at 1); half as much against the
 * elements */
static float armour_cap(void);
static float armour_keep(const Gear *g, int phys) {
    float a = (float)gear_armor(g) * (phys ? 1.0f : 0.5f), ref = 4.0f + 0.8f * (float)G.level;
    return a > 0.0f ? fminf(a / (a + ref), armour_cap()) : 0.0f;  /* never more than its cap (the owner's caps by class; D's) */
}
static void windup_blow(int d, float raw, float after, int el) {
    if (!winding_up() || G.max_hp <= 0) return;
    if (G.blame.what == BL_FIRE || (G.blame.what == BL_HAZARD && (G.blame.type == HZ_FIRE || G.blame.type == HZ_BILE || G.blame.type == HZ_VOID))) return;
    const Gear *g = &RUN.gear;
    float share = (float)d / (float)G.max_hp, chance = fminf(0.9f, 0.25f + 2.0f * share);
    float res = el_resist(g, el);
    float keep = gear_dodge(g) + 0.6f * (raw > 0.0f ? fmaxf(0.0f, raw - after) / raw : 0.0f) + 0.4f * res + 0.25f * gear_total(g, ST_LUCK) / 100.0f;
    if (rndf() >= chance * (1.0f - fminf(0.9f, keep))) return;
    windup_break();
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT windup_broken %s chance %.2f keep %.2f t=%.2f\n", G.player ? "player" : "autopilot", chance, keep, G.t);
#endif
}
static void rest_end(const char *why);
static void say(const char *fmt, ...);
/* ---- The caster's statuses (docs/PLAN-combat.md, section 4; track C). G.af[k] the time left, G.af0[k] what it began at,
 * G.af_n Chilled's stacks. The hard ones (Stunned, Unbound, Frozen, Knocked Down) hold a person from spells, runes and the
 * gifts; Frozen and Knocked Down from walking too. None lands back to back (S1), and an element's own resist halves its
 * status's chance (S11). CRAWL_AF=0 (the debug build) takes them all out */
static int af_on(int k) { return k >= 0 && k < NAF && G.af[k] > 0.0f; }
static int af_hardk(int k) { return k == AF_STUN || k == AF_UNBOUND || k == AF_FROZEN || k == AF_DOWN; }
static int af_rooted(void) { return af_on(AF_FROZEN) || af_on(AF_DOWN); }            /* not a step */
static int af_hushed(void) { return af_on(AF_STUN) || af_on(AF_UNBOUND) || af_rooted(); }   /* no spell, rune or gift */
static int no_cast(void) { return G.silenced || af_hushed(); }                        /* the void's silence, or a status's */
static int rune_barred(int k) {                                    /* Blink breaks a knock-down or a freeze; Purify a freeze (S2) */
    if (G.silenced || af_on(AF_STUN) || af_on(AF_UNBOUND)) return 1;
    if (af_on(AF_FROZEN)) return k != DF_BLINK && k != DF_PURIFY;
    return af_on(AF_DOWN) && k != DF_BLINK;
}
static const char *hush_word(void) {
    return G.silenced ? "THE VOID TAKES THE WORDS FROM YOU" : af_on(AF_FROZEN) ? "FROZEN FAST: NOTHING WILL COME"
         : af_on(AF_DOWN) ? "ON THE FLOOR: NOTHING WILL COME" : af_on(AF_UNBOUND) ? "UNBOUND: THE WORDS WON'T COME"
         : "STUNNED: NOTHING WILL GATHER";
}
static void hush_say(void) { say("%s", hush_word()); snd_sfx_ex(SFX_KNOCK, 0.4f, 0.5f); }
static int af_off(void) {
#ifdef CRAWL_DEBUG
    static int off = -1;
    if (off < 0) { const char *e = getenv("CRAWL_AF"); off = e && !strcmp(e, "0"); }
    return off;
#else
    return 0;
#endif
}
static const int8_t AF_EL[NAF] = { EL_FIRE, EL_FROST, EL_FROST, EL_LIGHT, EL_POISON, EL_SPIRIT, EL_PHYS };   /* whose resist keeps it off */
#ifdef CRAWL_DEBUG
static const char *const AF_NAME[NAF] = { "fire", "chill", "frozen", "stun", "poison", "unbound", "down" };
static int af_count[NAF];
#endif
static float conf_cut(int k);                                     /* 1.0.0b20: the Confessions' share off a status (C5) */
static float soft_pace(void);
static void af_event(int k, int on) {
#ifdef CRAWL_DEBUG
    if (on) af_count[k]++;
    if (on) fprintf(stderr, "EVENT af %s on by %d/%d/%d n=%d %s t=%.2f\n", AF_NAME[k], G.blame.what, G.blame.type, G.blame.rank,
                    G.af_n, G.player ? "player" : "autopilot", G.t);
    else fprintf(stderr, "EVENT af %s off t=%.2f\n", AF_NAME[k], G.t);
#else
    (void)k; (void)on;
#endif
}
#define AF_BASE 0.12f                       /* a hazard's stroke (S10): M2's rank II */
static float af_rank_chance(int rank) { return 0.08f + 0.04f * (float)imin(rank, RK_V); }   /* M2: 12% at rank II, 4% more a rank */
static int af_boss_fight(void) { return G.boss >= 0 && (G.bstate == BS_FIGHT || G.bstate == BS_WAVE || G.bstate == BS_RISE); }
static int af_can(int k) {
    if (af_off() || G.dead || G.ending || G.exiting || G.tally || G.shop || G.level_t < 1.5 || G.wind_t > 0.0f || G.dash_t > 0.0f) return 0;
    if (G.purify_t > 0.0f && (k == AF_FIRE || k == AF_CHILL || k == AF_POISON)) return 0;   /* Purify keeps them off */
    if (af_hardk(k) && (af_on(k) || G.af_lock[k] > 0.0f || G.af_hardt > 0.0f)) return 0;  /* S1: never back to back */
    return 1;
}
static int af_rolls(int k, float chance) {                        /* S11: the element's own resist halves it; the class's own (D5) */
    return rndf() < chance * (1.0f - 0.5f * el_resist(&RUN.gear, AF_EL[k])) * (1.0f - class_extra(KX_AF_CUT)) * (1.0f - conf_cut(k));
}
static void af_hold(int k, float secs, int by) {                  /* a hard control lands: whatever was gathering is lost */
    secs *= 1.0f - conf_cut(k);                                    /* Agility, Spirituality: not as long */
    G.af[k] = G.af0[k] = secs;
    if (k == AF_DOWN) G.af_by = by;
    if (winding_up()) windup_break();
    if (G.rest_on) rest_end("struck");
    switch (k) {
    case AF_STUN: snd_sfx_ex(SFX_AF_STUN, 0.8f, 0.5f); rumble(0.15f, 0.55f, 0.18f); break;   /* a short buzz */
    case AF_UNBOUND: snd_sfx_ex(SFX_AF_HUSH, 0.8f, 0.5f); break;
    case AF_FROZEN: snd_sfx_ex(SFX_AF_FREEZE, 0.9f, 0.5f); rumble(0.55f, 0.25f, 0.45f); break;
    case AF_DOWN: snd_sfx_ex(SFX_AF_DOWN, 0.9f, 0.5f); snd_sfx_ex(SFX_HEART, 0.6f, 0.5f); rumble(0.9f, 0.4f, 0.35f); break;
    }
    pop_readout(); show_bar();
    af_event(k, 1);
}
static void af_ignite(float dmg) {                                 /* the hit's harm again over four seconds, added to what is left (E2) */
    if (dmg <= 0.0f) return;
    int was = af_on(AF_FIRE);
    float cut = conf_cut(AF_FIRE);                                 /* Survival: shorter, and so less of it */
    G.af_burn += dmg * (1.0f - cut); G.af[AF_FIRE] = G.af0[AF_FIRE] = 4.0f * (1.0f - cut); G.af_fire_by = G.blame;
    snd_sfx_ex(SFX_IGNITE, was ? 0.45f : 0.8f, 0.5f);
    if (!was) af_event(AF_FIRE, 1);
}
static void af_chill(int n, int by) {                              /* stacks; four make Frozen; all fall 3 s after the last (S3) */
    if (af_on(AF_FROZEN)) return;                                  /* frozen: they are spent as it ends */
    int was = G.af_n;
    G.af_n = imin(G.af_n + n, 4); G.af[AF_CHILL] = G.af0[AF_CHILL] = 3.0f * (1.0f - conf_cut(AF_CHILL));
    snd_sfx_ex(SFX_FROST, 0.35f + 0.1f * (float)G.af_n, 0.5f);
    if (!was) af_event(AF_CHILL, 1);
    if (G.af_n < 4) return;
    if (af_can(AF_FROZEN)) af_hold(AF_FROZEN, 3.0f, by);
    else G.af_n = 3;                                               /* not again so soon: it waits at three */
}
static void af_poison(float secs) {                                /* each dose adds its time, to a minute (S5) */
    if (secs <= 0.0f || !af_can(AF_POISON)) return;
    secs *= 1.0f - conf_cut(AF_POISON);                            /* Survival */
    int was = af_on(AF_POISON);
    G.af[AF_POISON] = G.af0[AF_POISON] = fminf(60.0f, G.af[AF_POISON] + secs);
    if (!was) { snd_sfx_ex(SFX_AF_SICK, 0.7f, 0.5f); af_event(AF_POISON, 1); }
}
static void af_put(int k, float d, int by) {                       /* status k, landed, from a hit of d */
    switch (k) {
    case AF_FIRE: af_ignite(d); break;
    case AF_CHILL: af_chill(1, by); break;
    case AF_POISON: af_poison(6.0f); break;
    case AF_STUN: af_hold(AF_STUN, 1.2f, by); break;
    case AF_UNBOUND: af_hold(AF_UNBOUND, 1.5f, by); break;
    case AF_DOWN: af_hold(AF_DOWN, 1.5f, by); break;
    default: break;
    }
}
/* a creature's elemental hit got through: its status, by its rank (M2). Bosses keep their own ways (S12); Physical knocks down
 * only by the attacks that say so (C3, C4) */
static void af_struck(int el, Blame b, float d, int by, float mult) {
    int k = af_of(el);
    if (k < 0 || el == EL_PHYS || b.what == BL_BOSS || af_boss_fight() || !af_can(k) || !af_rolls(k, af_rank_chance(b.rank) * mult)) return;
    af_put(k, d, by);
}
static void af_stroke(int el, float d) {                           /* a hazard's stroke: a frost one a stack (S9), the rest at the base chance (S10) */
    int k = af_of(el);
    if (k < 0 || af_boss_fight() || !af_can(k)) return;
    if (k == AF_CHILL) { if (af_rolls(k, 1.0f)) af_chill(1, 0); return; }
    if (af_rolls(k, AF_BASE)) af_put(k, d, 0);
}
static void af_cold(float dt) {                                    /* standing in the cold: a stack a second and a half (S9) */
    if (G.level_t - G.af_cold_t > 0.5 || G.level_t < G.af_cold_t) G.af_cold = 0.5f;   /* come in fresh: the first after a second */
    G.af_cold_t = (float)G.level_t;
    if ((G.af_cold += dt) < 1.5f) return;
    G.af_cold -= 1.5f;
    if (af_can(AF_CHILL) && af_rolls(AF_CHILL, 1.0f)) af_chill(1, 0);
}
static void af_purify(void) {                                      /* Purify: Poisoned, Chilled and On Fire cured, and a freeze broken */
    G.af[AF_POISON] = G.af[AF_CHILL] = G.af[AF_FIRE] = 0.0f; G.af_n = 0; G.af_burn = G.af_acc = 0.0f;
    if (af_on(AF_FROZEN)) { G.af[AF_FROZEN] = 0.01f; }             /* af_tick ends it, with its sound */
}
static void af_break(void) {                                       /* Blink: up off the floor, out of the ice */
    if (af_on(AF_DOWN)) G.af[AF_DOWN] = 0.01f;
    if (af_on(AF_FROZEN)) G.af[AF_FROZEN] = 0.01f;
}
static void af_clear(void) {                                       /* a death, a new run, a reset: none of them */
    memset(G.af, 0, sizeof G.af); memset(G.af0, 0, sizeof G.af0); memset(G.af_lock, 0, sizeof G.af_lock);
    G.af_n = G.af_by = 0; G.af_k = G.af_shake = G.af_hardt = G.af_burn = G.af_acc = G.af_cold = 0.0f;
}
static int harm_from;                                              /* 1 + the creature whose harm hurt_ex is about to take */
/* a person's last two minutes, in tens of seconds: health lost, kills, and how long since a hurt (docs/PLAN-ai.md, section 27) */
static struct { float hp[12]; int k[12], i; float t, calm, over_t, quiet; int n0; } WELL;
static int hurt_ex(float dmg, int kind, float sx, float sy) {
    int el = harm_of(kind);
    float raw = dmg, armoured = dmg;
    if (G.dead || G.ending || G.exiting || G.tally || G.shop || G.level_t < 1.5) return 0;
    if (G.dash_t > 0.0f) return 0;                                 /* Swift Dodge: nothing touches you while you dart */
    if (G.wind_t > 0.0f) return 0;                                 /* a Second Wind: a breath when nothing touches you */
    if (G.player && tu_taken()) return 0;                          /* nor while the lessons hold you (docs/PLAN-tutorial.md 8.3) */
    if (G.rest_on) rest_end("struck");                             /* a blow ends the mending */
    const Gear *g = &RUN.gear;
    int seeping = kind == HURT_POISON || kind == HURT_VOID;        /* nothing turns these: no dodge, armour or ward */
    int from = harm_from; harm_from = 0;
    if (af_on(AF_FROZEN)) dmg *= 1.33f;                            /* frozen: everything a third more, before the killing blow is judged */
    if (!seeping) {
        if (kind != HURT_STONE && kind != HURT_ELEM && rndf() < gear_dodge(g)) { snd_sfx_ex(SFX_WHOOSH, 0.5f, 0.5f); DBG_COUNT(dodges); return 0; }   /* it passed through a shadow */
        int phys = el < 0 || el == EL_PHYS;
        if (old_harm()) {                                          /* 1.0.0b18's: every spell resisted as fire */
            if (kind == HURT_SPELL) dmg *= 1.0f - gear_magic_resist(g);
            if (kind == HURT_ELEM && el != EL_SPIRIT && el != EL_LIGHT) dmg *= 1.0f - el_resist(g, el);
            if (el == EL_SPIRIT || el == EL_LIGHT) dmg *= 1.0f - el_resist(g, el);
        } else if (!phys) dmg *= 1.0f - el_resist(g, el);          /* each element by its own resist alone (PLAN-combat, section 3) */
        raw = dmg;
        if (old_harm()) dmg -= kind == HURT_SPELL || (kind == HURT_ELEM && el != EL_PHYS) ? gear_armor(g) * 0.5f : (float)gear_armor(g);
        else dmg *= 1.0f - armour_keep(g, phys);                  /* a share kept off (P2), half of it against the elements */
        if (af_on(AF_DOWN) && from > 0 && from != G.af_by) dmg *= 0.5f;   /* on the floor: every other creature's harm halved */
        if (dmg < 1.0f) dmg = 1.0f;
        armoured = dmg;
        if (G.ward_hp > 0.0f) {                                    /* the bone ward takes it */
            float ab = fminf(G.ward_hp, dmg);
            G.ward_hp -= ab; dmg -= ab;
            sfx_at(SFX_BONES, sx, sy, 0.5f);
            if (G.ward_hp <= 0.0f) { G.ward_t = 0.0f; snd_sfx_ex(SFX_SHATTER, 0.8f, 0.5f); if (G.ward_shards > 0.0f) ward_burst(); }
            if (dmg < 0.5f) return 0;
        }
    }
    int d = (int)(dmg + 0.5f);
    if (d < 1) d = 1;
    G.hp -= d; G.hurts++;
    if (G.player) { WELL.hp[WELL.i] += (float)d; WELL.calm = 0.0f; WELL.over_t = 0.0f; }   /* hurt: the cap is the run's again */
    rec_harm(G.blame, el);
    if (!seeping) windup_blow(d, raw, armoured, el);
    if (!seeping) {
        G.hurt = 1.0f;
        G.kick = (rnd(2) ? 1.0f : -1.0f) * 0.07f;
        float lat = (sx - G.px) * -sinf(G.ang) + (sy - G.py) * cosf(G.ang);
        G.face_side = fabsf(lat) < 0.3f ? 0 : (lat < 0.0f ? -1 : 1);
        G.face_t = FACE_T;
    } else G.hurt = fmaxf(G.hurt, 0.25f);
    pop_readout(); show_bar();
    float share = G.max_hp > 0 ? (float)d / (float)G.max_hp : 0.0f;  /* the pad shakes by how much it hurt */
    if (G.hp > 0) { if (!seeping) { snd_sfx(SFX_HURT); rumble(0.25f + 2.0f * share, 0.15f + 1.5f * share, 0.15f + share); } return d; }
    if ((RUN.corner >> CS_WIND & 1) && !RUN.wind_spent) {          /* Second Wind: not yet */
        RUN.wind_spent = 1;
        G.hp = imax(1, G.max_hp / 2); G.wind_t = 2.0f; G.poison_t = 0.0f; G.hz_acc = 0.0f; af_clear();   /* and nothing on you */
        snd_sfx_ex(SFX_WAIL, 0.9f, 0.5f); snd_sfx_ex(SFX_SHIMMER, 1.0f, 0.5f); snd_sfx_ex(SFX_WHOOSH, 0.8f, 0.5f);
        rumble(0.8f, 0.5f, 0.6f); pop_readout(); show_bar();
#ifdef CRAWL_DEBUG
        fprintf(stderr, "EVENT second_wind level %d t=%.2f by %s\n", G.level, G.t, G.player ? "player" : "autopilot");
#endif
        return d;
    }
    if (!seeping && blow_survived(kind)) return d;                 /* Constitution's killing blow survived (D5, section 10.4) */
    rumble(1.0f, 0.7f, 0.9f);
    G.hp = 0; G.dead = 1; G.dead_t = 0.0; G.retries++; G.killer = G.blame;
    if (mind_on() && !PK.depth) {                                  /* no death loop (section 21): what killed you comes back a head short */
        if (short_lv != G.level) { memset(short_n, 0, sizeof short_n); short_hunt = 0; short_lv = G.level; }
        int k = G.blame.what == BL_MOB ? last_set : 0;
        if (k > 0 && k < NSET) { short_n[k]++; short_hunt |= k == SET_HUNT; }
    }
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT died level %d t=%.1f by %d/%d/%d\n", G.level, G.level_t, G.killer.what, G.killer.type, G.killer.rank);
#endif
    G.hand = 0.0f; G.spd = 0.0f; G.face_t = 1e9f;
    snd_sfx(SFX_DEATH);
    snd_set_boss(0, 0);
    death_fall();                                                  /* a person's run: written at once */
    return d;
}
enum { MA_CALM, MA_WARY, MA_HUNT, MA_TRACK, MA_SEARCH };          /* how the dead think (docs/PLAN-ai.md, Part A) */
enum { NZ_STEP, NZ_CAST, NZ_FIGHT, NZ_MEND, NZ_LAND, NZ_DOOR, NZ_SECRET, NZ_DEATH, NZ_CALL, NZ_BELL, NNZ };   /* the first four are the caster's own, where it stands */
static void noise(float x, float y, int steps, int what, int kind);
static void alarm_at(Mob *m, int steps);
static void way_met(int w);
static void deed_note(int which);
static int pack_way(const Pack *p);
static void ways_ends(void);
static void sexton_ring(Mob *m, int hard);
static void dream_glance(float x, float y);
static void wisp_out(Wisp *w);
static void hunt_tick(void);
static void amb_tick(void);
static int shade_hidden(const Mob *m);
static void amb_spring(int k, const char *why);
static void amb_out(int k, float x, float y, int hunting);
static int minded(const Mob *m);
static int in_view(const Mob *m, float px, float py);
static Pack *pack_of(const Mob *m);
#ifdef CRAWL_DEBUG
static int g_mind = -1;
static int wary, noticed, searches, gave_up, unaware_hits, tracked, lost_you, routes, posted;
static int pack_calls, pack_cuts, pack_hunts, packs_broken, fetches, roosts_woken, wakes_rise, turns;
static int alarms, alarms_heard, scavenge, feeds;
static int hunt_scent, hunt_lost, shade_fades, shade_strikes;
static int ambushes_sprung, ambushes_out, ambushes_refused, bait_seen, bait_run;
static int dodged, flank, spread, feints, fell_back, covers, corner_throws, door_waits, charge_reads, hz_mob_hits, cap_over;
static int sentinels_woke, chills, nest_calls, gathered, leashed;
static int glances, glance_finds, amb_read, amb_probes, corners, corners_held;
static int ways_blocked, lantern_felt, horns, whip_cracks, whip_lashes, wild_hounds, bell_rounds, bell_alarms, bell_heard, bells_frozen, light_felt, lightless_lost, lightless_hits, wisps_struck;
/* docs/PLAN-combat.md section 17: each track adds its counters only above its own end mark */
static int elem_hits, elem_throws;                                 /* C3: blows and throws that carried a creature's element */
static int c4_volleys, c4_charges, c4_charge_hits, c4_downs, c4_spews, c4_spew_hits, c4_grapples, c4_souls;   /* C4's */
/* the caster's track (C) ends */
/* the dead's track (D): */
static int wraith_cornered_n;                                      /* the Wraith gone into the stone from a corner */
static int wire_arcs, sin_souls, block_bones;                      /* D6: Live Wire's sparks, Swallowed Sin's souls, the Block's bones */
static int aug_claps, aug_snaps, aug_splinters, aug_rains, aug_bones, aug_lingers, aug_twins, aug_echoes, aug_conducts,
           aug_clusters, aug_miasmas, aug_pests;                   /* D7: the five lines' augments at work */
/* the dead's track (D) ends */
#endif
static int hurt_by(const Mob *m, float dmg, int kind) {
    G.blame = blame_of(m); last_set = m->set;
    harm_from = 1 + (int)(m - G.mobs);
    int r = hurt_ex(dmg, kind, m->x, m->y);
    if (r > 0) noise(G.px, G.py, 5, NZ_FIGHT, -1);                /* blows: heard five steps off (S3) */
    return r;
}
/* One road for the dead's harm (docs/PLAN-combat.md, section 3): an attack's row, times k, as its element. A blow, a bite, a
 * leap's landing or a touch; what flies goes by bone_hurt */
/* An elemental creature's attack (docs/PLAN-combat.md, section 8, M1): one in three carries its element, decided as its wind-up
 * begins (elem_arm; its eyes flare while it holds) or, for a quick bite with none, as it lands; an Unbound one carries none (E5).
 * elem_take spends it */
static void elem_arm(Mob *m, float wind) {
    if (!m->elem || m->boss || m->add || m->ally || af_off()) return;
    m->elem_t = wind + 0.35f;
    m->elem_on = !mob_unbound(m) && rnd(3) == 0;
#ifdef CRAWL_DEBUG
    static int every = -1;
    if (every < 0) every = getenv("CRAWL_ELEMON") != NULL;       /* every attack carries it, for its frames */
    if (every && !mob_unbound(m)) m->elem_on = 1;
#endif
}
static int elem_take(Mob *m) {
    if (!m->elem || m->boss || m->add || m->ally || af_off()) return -1;
    if (m->elem_t <= 0.0f) elem_arm(m, 0.0f);                       /* a bite with no wind-up: decided now */
    int e = m->elem_on && !mob_unbound(m) ? m->elem - 1 : -1;
    m->elem_on = 0; m->elem_t = 0.0f;
    return e >= 0 && e < NELEM && e != EL_PHYS ? e : -1;
}
static void elem_tick(Mob *m, float dt) {                          /* the decision held only as long as its wind-up */
    if (m->elem_t > 0.0f && (m->elem_t -= dt) <= 0.0f) { m->elem_t = 0.0f; m->elem_on = 0; }
}
static int c4_on(void);
static float spirit(void);
static int spend_spirit(float c);
static int foe_el;                                                 /* the element foe_hit's last blow went as */
static int foe_hit(Mob *m, int fa, float k) {
    const FoeAtk *a = &ATTACKS[fa];
    int el = a->el;
    if (el == EL_PHYS) { int e = elem_take(m); if (e >= 0) el = e; }   /* its element, one blow in three */
    if (el != EL_PHYS) harm_next = el;
    foe_el = el;
    int d = hurt_by(m, mob_damage(m) * a->k * k + a->add, el == EL_PHYS ? HURT_BLOW : HURT_SPELL);
    if (d > 0 && m->type == MOB_SHAMBLER && el == EL_SPIRIT && c4_on()) {   /* a Spirit shambler's grip takes souls (M6), not the words */
        if (af_can(AF_UNBOUND) && af_rolls(AF_UNBOUND, af_rank_chance(m->rank) * 2.5f) && spirit() > 0.0f) {
            spend_spirit(fminf(spirit(), 2.0f + (float)imin(m->rank, RK_V))); show_bar(); pop_readout();
            snd_sfx_ex(SFX_WHISPER, 0.7f, 0.5f); DBG_COUNT(c4_souls);
        }
    } else if (d > 0 && m->type != MOB_WRAITH)                     /* the Wraith's chills (S9); a shambler's grip two and a half times (M2) */
        af_struck(el, blame_of(m), (float)d, 1 + (int)(m - G.mobs), m->type == MOB_SHAMBLER ? 2.5f : 1.0f);
    if (d > 0 && el != a->el) DBG_COUNT(elem_hits);
    return d;
}
static int bone_el(const Bone *b) { return b->el1 - 1; }           /* what a missile carries: -1 nothing but itself */
static int bone_hurt(const Bone *b) {
    int el = bone_el(b);
    G.blame = b->by;
    if (el >= 0 && el != EL_PHYS) harm_next = el;
    harm_from = b->from >= 0 && b->from < MAXMOBS ? 1 + b->from : 0;
    int d = hurt_ex(b->dmg, el >= 0 && el != EL_PHYS ? HURT_SPELL : HURT_BLOW, b->x - b->vx, b->y - b->vy);
    if (d > 0 && el >= 0 && b->kind != MIS_TURNED) af_struck(el, b->by, (float)d, harm_from, 1.0f);   /* a turned spell is the knight's own (S12) */
    return d;
}

/* a throw at the caster, leading it a little; or (lob) one lobbed to a place, landing there: over a corner (step 7) */
static void throw_to(const Mob *m, int kind, float tx, float ty, int lob) {
    for (int i = 0; i < MAXBONES; i++) {
        Bone *b = &G.bones[i];
        if (b->alive) continue;
        float sp = kind == MIS_FIRE ? FIRE_SPEED : (kind == MIS_EMBER ? EMBER_SPEED : BONE_SPEED);
        float tt = hypotf(tx - m->x, ty - m->y) / sp;
        if (kind != MIS_FIRE && !lob) { tx += G.pvx * tt * 0.6f; ty += G.pvy * tt * 0.6f; }   /* leads the caster a little */
        float far = lob ? 0.0f : fmaxf(0.0f, hypotf(G.px - m->x, G.py - m->y) - 4.0f);   /* across a great hall a bone goes wide */
        float a = atan2f(ty - m->y, tx - m->x) + (rndf() - 0.5f) * (kind == MIS_FIRE ? 0.05f : (lob ? 0.06f : 0.14f + 0.05f * far));
        int fa = kind == MIS_FIRE ? FA_FIREBALL : (kind == MIS_EMBER ? FA_EMBER : FA_BONE);
        int el = kind == MIS_BONE ? elem_take(&G.mobs[m - G.mobs]) : -1;   /* an elemental skeleton's, one throw in three (M1) */
        if (el >= 0) DBG_COUNT(elem_throws);
        *b = (Bone){ 1, kind, m->x + cosf(a) * 0.3f, m->y + sinf(a) * 0.3f, cosf(a) * sp, sinf(a) * sp, 4.0f, rndf() * 4.0f,
                     mob_damage(m) * ATTACKS[fa].k, 0.0f, (int)(m - G.mobs), 0, blame_of(m), 1 + (el >= 0 ? el : ATTACKS[fa].el) };
        if (kind == MIS_EMBER || lob) b->life = b->life0 = fminf(4.0f, hypotf(tx - m->x, ty - m->y) / sp);   /* lobbed to where the caster will be */
        if (kind == MIS_BONE) G.mobs[m - G.mobs].feat |= FT_THREW;   /* its feat spoilt (the portraits') */
        sfx_at(kind == MIS_BONE ? SFX_THROW : SFX_CAST, m->x, m->y, kind == MIS_EMBER ? 0.7f : 1.0f);
        return;
    }
}
static void throw_missile(const Mob *m, int kind) { throw_to(m, kind, G.px, G.py, 0); }
static void throw_bone(const Mob *m) { throw_missile(m, MIS_BONE); }
static void bone_land(const Bone *b) {                             /* a bone lobbed over a corner comes down where you went */
    if (!G.dead && !passing() && hypotf(b->x - G.px, b->y - G.py) < 0.9f) {
        bone_hurt(b); DBG_COUNT(hits_bone);
    } else sfx_at(SFX_BONES, b->x, b->y, 0.5f);
}

static void explode(float x, float y);
static Shot *new_shot(int kind, float x, float y, float a, float speed, float dmg);
static void say(const char *fmt, ...);

static void ember_land(const Bone *b) {
    for (int i = 0; i < MAXBURNS; i++)
        if (!G.burns[i].alive) { G.burns[i] = (Burn){ 1, b->x, b->y, 0.0f, b->dmg * 0.8f, 1, 2.5f }; break; }
    sfx_at(SFX_EXPLODE, b->x, b->y, 0.4f);
    DBG_COUNT(ember_burns);
}

static void update_bones(float dt) {
    for (int i = 0; i < MAXBONES; i++) {
        Bone *b = &G.bones[i];
        if (!b->alive) continue;
        float k = G.slip_t > 0.0f && hypotf(b->x - G.px, b->y - G.py) < 7.0f ? 0.3f : 1.0f;   /* Time Slip */
        b->life -= dt; b->spin += dt * 14.0f * k;
        for (int s = 0; s < 4 && b->alive; s++) {
            float nx = b->x + b->vx * dt * k / 4, ny = b->y + b->vy * dt * k / 4;
            if (blocks_light((int)nx, (int)ny) || cell((int)nx, (int)ny)->kind == K_PILLAR) {
                b->alive = 0;
                if (b->kind == MIS_FIRE || (b->kind == MIS_TURNED && (b->atk == AT_FIRE || b->atk == AT_HELL))) explode(b->x, b->y);
                else if (b->kind == MIS_EMBER) ember_land(b);
                else if (b->kind == MIS_TURNED) sfx_at(b->atk == AT_FROST ? SFX_FROST : SFX_ZAP, b->x, b->y, 0.5f);
                else if (b->kind == MIS_WEB) sfx_at(SFX_DIRT, b->x, b->y, 0.4f);
                else sfx_at(SFX_BONES, b->x, b->y, 0.4f);
                break;
            }
            b->x = nx; b->y = ny;
            float d = hypotf(b->x - G.px, b->y - G.py);
            if (passing()) continue;                               /* it goes through you */
            if (G.mirror_t > 0.0f && d < 0.7f && !G.dead) {           /* the Mirror Veil sends it back */
                b->alive = 0;
                int exq = RUN.gear.def[DF_MIRROR] >= RUNE_EXQ || G.mirror_true > 0.0f;   /* Exquisite, or the parry: home to its thrower, twice as hard */
#ifdef CRAWL_DEBUG
                if (G.mirror_true > 0.0f) DBG_COUNT(parries);
#endif
                Shot *sh = new_shot(SHOT_REFLECT, b->x, b->y, atan2f(-b->vy, -b->vx), b->kind == MIS_FIRE ? 6.0f : 7.0f, b->dmg * (exq ? 3.0f : 1.5f));
                if (sh) { sh->z = (float)b->kind; if (exq) { sh->target = b->from; sh->life = 3.0f; DBG_COUNT(exq_mirror); } }
                snd_sfx_ex(SFX_SHIMMER, 0.35f, 0.5f);
                break;
            }
            if (!G.dead && d < 0.36f && rndf() * 100.0f < gear_ability(&RUN.gear, AB_MIRRORED, 0)) {   /* Mirrored: back to its thrower */
                b->alive = 0;
                Shot *sh = new_shot(SHOT_REFLECT, b->x, b->y, atan2f(-b->vy, -b->vx), b->kind == MIS_FIRE ? 6.0f : 7.0f,
                                    b->dmg * gear_ability(&RUN.gear, AB_MIRRORED, 1) / 100.0f);
                if (sh) { sh->z = (float)b->kind; sh->target = b->from; sh->life = 3.0f; }
                snd_sfx_ex(SFX_SHIMMER, 0.5f, 0.5f); G.mirror_flash = 0.35f;
                DBG_COUNT(mirrored);
                break;
            }
            if (!G.dead && d < 0.36f) {
                b->alive = 0; G.blame = b->by; last_set = b->from >= 0 && b->from < MAXMOBS ? G.mobs[b->from].set : 0;
                if (b->kind == MIS_FIRE) { bone_hurt(b); explode(b->x, b->y); DBG_COUNT(hits_fire); }
                else if (b->kind == MIS_EMBER) { bone_hurt(b); ember_land(b); DBG_COUNT(ember_hits); }
                else if (b->kind == MIS_WEB) {                     /* held fast */
                    G.web_max = G.web_hp = (float)G.max_hp / 3.0f; G.web_idle = 0.0f;
                    snd_sfx_ex(SFX_DIRT, 0.9f, 0.5f); say("CAUGHT IN THE WEB: CAST TO TEAR IT");
                    DBG_COUNT(webbed);
                }
                else if (b->kind == MIS_TURNED) {                  /* your own spell, at half strength */
                    bone_hurt(b);                                   /* as its own element (it was always fire before) */
                    if (b->atk == AT_FIRE || b->atk == AT_HELL) explode(b->x, b->y); else sfx_at(b->atk == AT_FROST ? SFX_FROST : SFX_ZAP, b->x, b->y, 0.6f);
                    DBG_COUNT(turned_hits);
                }
                else { bone_hurt(b); DBG_COUNT(hits_bone); }
            }
        }
        if (b->life <= 0.0f && b->alive) { if (b->kind == MIS_EMBER) ember_land(b); else if (b->kind == MIS_BONE && b->life0 > 0.0f) bone_land(b); b->alive = 0; }
    }
}

/* souls: one or more a kill, by its rank, three times over while Soul Reaping runs */
/* The vessel (docs/PLAN-b11.md, section 12, S1): 20 and two a Level of Atonement, the Sin-Eater's twenty, the Vessel worn,
 * and ten each for Communion and Spirit Mastery. Souls past it slip away */
static int spirit_mastery(void);
static int cls_el(void);
static int align_open(int e);
static int align_full(int e, int n);
static int vessel(void) {
    float v = 20.0f + 2.0f * (float)RUN.atone + (cls_el() == EL_SPIRIT ? 20.0f : 0.0f) + gear_total(&RUN.gear, ST_VESSEL)
            + (align_open(EL_SPIRIT) && align_full(EL_SPIRIT, ND_A2) ? 10.0f : 0.0f) + (spirit_mastery() ? 10.0f : 0.0f);
    return (int)v;
}
static void soul_cap(int quiet) {                                  /* what is over the vessel slips away */
    int cap = vessel();
    if (RUN.souls < cap) return;
    int over = RUN.souls > cap || RUN.soul_frac > 0.0f;
    RUN.souls = cap; RUN.soul_frac = 0.0f;
    if (quiet || !over) return;
    G.vslip_t = 1.0f;
    if (!G.full_said && G.player) { G.full_said = 1; say("THE VESSEL IS FULL"); }
#ifdef CRAWL_DEBUG
    DBG_COUNT(souls_slipped);
#endif
}
static void gain_souls(float n) {
    float mult = G.reap_t > 0.0f ? 3.0f : 1.0f;
    RUN.soul_frac += n * mult;
    int whole = (int)RUN.soul_frac;
    if (whole > 0) { RUN.souls += whole; RUN.soul_frac -= (float)whole; G.level_souls += whole; snd_sfx_ex(SFX_GHOST_TICK, 0.5f, 0.5f); if (G.player) pop_readout(); }
    soul_cap(0);
}
static void soul_add(int n) { RUN.souls += n; G.level_souls += n; if (G.player) pop_readout(); soul_cap(0); }   /* whole souls: an urn's, a visportal's */
static void spawn_mote(float x, float y, float souls) {            /* left where it fell; a wall's spectre leaves it on the floor nearby */
    int cx = clampi((int)x, 0, MAPN - 1), cy = clampi((int)y, 0, MAPN - 1);
    if (G.map[cy][cx].kind != K_FLOOR)
        for (int d = 0; d < 8; d++) {
            static const int OX[8] = { 1, -1, 0, 0, 1, 1, -1, -1 }, OY[8] = { 0, 0, 1, -1, 1, -1, 1, -1 };
            int nx = clampi(cx + OX[d], 0, MAPN - 1), ny = clampi(cy + OY[d], 0, MAPN - 1);
            if (G.map[ny][nx].kind == K_FLOOR) { x = nx + 0.5f; y = ny + 0.5f; break; }
        }
    int k = 0;
    for (int i = 0; i < MAXMOTES; i++) { if (!G.motes[i].alive) { k = i; break; } if (G.motes[i].t > G.motes[k].t) k = i; }   /* or the oldest goes */
    G.motes[k] = (SoulMote){ 1, x, y, 0.0f, souls, G.d7_shep };
}
static void update_motes(float dt) {
    for (int i = 0; i < MAXMOTES; i++) {
        SoulMote *m = &G.motes[i];
        if (!m->alive) continue;
        if ((m->t += dt) >= MOTE_LIFE) { m->alive = 0; continue; }
        if (m->fly > 0.0f && m->t > 0.25f && !G.dead) {               /* Shepherd's: it comes to you (D7) */
            float vx = G.px - m->x, vy = G.py - m->y, d = hypotf(vx, vy) + 1e-4f, st = fminf(d, m->fly * dt);
            m->x += vx / d * st; m->y += vy / d * st; m->t = fminf(m->t, MOTE_LIFE - 2.5f);
        }
        if (G.dead || hypotf(m->x - G.px, m->y - G.py) > MOTE_REACH) continue;
        m->alive = 0;
        gain_souls(m->souls);
        sfx_at(SFX_WHISPER, m->x, m->y, 0.5f);
    }
    G.vslip_t = fmaxf(0.0f, G.vslip_t - dt);
}

/* Spirit is the souls, spent on spells to the tenth (PLAN-items: spirit and bones). Spending never
 * touches what the level has given (G.level_souls), so the wisp still comes after ten gained. */
static float spirit(void) { return (float)RUN.souls + RUN.soul_frac; }
static int potion_heal(void) { return (int)(25.0f * (1.0f + gear_total(&RUN.gear, ST_PHYSIC) / 100.0f) + 0.5f); }   /* PHYSIC (docs/PLAN-ai.md, 25) */
static void spirit_text(char *buf, int n) {                         /* to the tenth, while it fits */
    if (RUN.souls < 1000) snprintf(buf, (size_t)n, "%d.%d", RUN.souls, clampi((int)(RUN.soul_frac * 10.0f), 0, 9));
    else snprintf(buf, (size_t)n, "%d", RUN.souls);
}
static int spend_spirit(float c) {
    if (c <= 0.0f) return 1;
    if (spirit() + 1e-4f < c) return 0;
    RUN.soul_frac -= c;
    while (RUN.soul_frac < -1e-5f) { RUN.soul_frac += 1.0f; RUN.souls--; }
    if (RUN.soul_frac < 0.0f) RUN.soul_frac = 0.0f;
    if (G.player) pop_readout();                                    /* a person sees it go */
    return 1;
}

