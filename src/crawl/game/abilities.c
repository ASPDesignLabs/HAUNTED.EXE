/* A part of scene_crawl.c (docs/PLAN-tidy.md): it is #included there in its place and never compiled on its own.
 * The balance sheet (crawl_combat_sheet); a creature's blow; the bat's latch; the caster's boss abilities (web, Grave Call and allies, Wraithwalk, Swift Dodge); runes raised by hand; doors (update_doors). */

/* ---- the caster's boss abilities (docs/PLAN-b7.md, section 8), from the Hexed Totem ---- */
static int hz_soft(int x, int y);
static int hz_danger(int x, int y);
static int walk_clear(float x0, float y0, float x1, float y1);

#ifdef CRAWL_DEBUG
/* The balance sheet (docs/PLAN-combat.md, section 3; `make combat`, `build/host_debug combat`): the game's own numbers, level by
 * level. THE DEAD: each kind met by then at rank I and at its run's highest rank, its health, each of its attacks' harm before
 * anything turns it, how often it comes, what gets through a reference caster's armour, and how long that attack alone needs to
 * kill them. THE CASTER: each class's line at a reference Level of Atonement (the dream's: 16 at level 11, 30 at 21, 44 at 31)
 * with every Alignment point in it and nothing worn, its damage, cycle and damage a second, and how many casts a rank-I kind
 * needs (the kind's weakness before its rank's strengths, and no class's gifts). The reference caster is 100 health and eight a
 * level, with a Common robe's armour (0.8 a level). */
void crawl_combat_sheet(FILE *f);
void crawl_combat_sheet(FILE *f) {
    static const int LV[] = { 1, 3, 5, 7, 10, 13, 15, 17, 20, 23, 25, 27, 30, 33, 35, 37, 40 };
    static const float AT_L[] = { 1, 11, 21, 31, 40 }, AT_A[] = { 1, 16, 30, 44, 50 };
    static const char *const AK_WORD[] = { "blow", "bite", "throw", "leap", "touch", "cast", "spew", "charge" };
    int keep_level = G.level, keep_retries = G.retries, keep_theme = G.ptheme;
    G.retries = 0; G.ptheme = 0;
    Gear gear;
    fprintf(f, "THE DEAD: kind, rank, health; each attack: what, element, harm, its whole round (wind-up and wait, s), harm\n"
               "through the reference armour, seconds it alone needs to kill the reference caster\n");
    for (size_t li = 0; li < sizeof LV / sizeof LV[0]; li++) {
        int L = LV[li];
        G.level = L;
        float php = 100.0f + 8.0f * (float)L, parm = 0.8f * (float)L;
        int set = crawl_set(L), top = imin(RK_V, set + (crawl_step(L) >= 4 ? 1 : 0));
        fprintf(f, "\nLEVEL %d (run %d): reference caster %.0f health, %.0f armour; throws every %.2f s\n", L, set + 1, php, parm, throw_interval());
        for (int k = 0; k < NKINDS; k++) {
            if (KINDS[k].debut > L || KINDS[k].hp >= 1e8f || fa_main(k) < 0) continue;
            for (int ri = 0; ri < 2; ri++) {
                int r = ri ? imin(top, KINDS[k].top) : RK_I;
                if (ri && r == RK_I) continue;
                Mob m;
                memset(&m, 0, sizeof m);
                m.type = k; m.dmgk = RANK_DMG[r];
                float hp = k == MOB_WSKULL ? 1.0f : mob_base_hp(k) * powf(1.04f, (float)(L - 1)) * RANK_HP[r];
                fprintf(f, "  %-16s %-4s hp %6.0f", KINDS[k].name, r == RK_I ? "I" : r == RK_II ? "II" : r == RK_III ? "III" : r == RK_IV ? "IV" : "V", hp);
                for (int fa = 0; fa < NFA; fa++) {
                    const FoeAtk *a = &ATTACKS[fa];
                    if (a->type != k || a->k <= 0.0f || (fa == FA_WBOSS)) continue;
                    int phys = a->el == EL_PHYS;
                    float harm = mob_damage(&m) * a->k + a->add, every = (a->cool < 0.0f ? throw_interval() : a->cool) + a->wind;   /* the whole round */
                    float ar = parm * (phys ? 1.0f : 0.5f), ref = 4.0f + 0.8f * (float)L;   /* as hurt_ex takes it (armour_keep) */
                    float got = fmaxf(1.0f, old_harm() ? harm - ar : harm * ref / (ref + ar));
                    fprintf(f, " | %s %s %.1f /%.2fs ->%.1f kill %.0fs", AK_WORD[a->ak], ELEM_NAME[a->el], harm, every, got, php / got * every);
                }
                fprintf(f, "\n");
            }
        }
    }
    fprintf(f, "\nTHE CASTER: each class's line at the reference Level of Atonement, every point in it, nothing worn: damage a cast,\n"
               "cycle, damage a second; then for each kind at its run's highest rank, the casts each class needs, by its weakness\n");
    static const char *const CL2[NELEM] = { "WF", "AN", "GA", "PD", "SE", "HE" };
    for (size_t li = 0; li < sizeof LV / sizeof LV[0]; li++) {
        int L = LV[li];
        G.level = L;
        float at = 50.0f, dmg[NELEM];
        for (int i = 0; i + 1 < 5; i++) if (L <= AT_L[i + 1]) { at = AT_A[i] + (AT_A[i + 1] - AT_A[i]) * ((float)L - AT_L[i]) / (AT_L[i + 1] - AT_L[i]); break; }
        int atone = (int)(at + 0.5f), rank = imin(29, atone), set = crawl_set(L), top = imin(RK_V, set + (crawl_step(L) >= 4 ? 1 : 0));
        spell_set_atone(atone);
        for (int x = 0; x < NELEM; x++) spell_set_line(x, 0.0f, 0.0f, 0.0f);
        fprintf(f, "\nLEVEL %d: Atonement %d, line rank %d\n", L, atone, rank);
        for (int e = 0; e < NELEM; e++) {
            gear_reset(&gear);
            int sp = LINE_ATK[e];
            gear.atk[sp] = (int8_t)rank;
            float cyc = spell_cycle(&gear, sp);
            dmg[e] = spell_damage_at(&gear, sp, rank);
            fprintf(f, "  %-13s %-12s dmg %6.1f  cycle %.2fs  dps %6.1f\n", CLASS_SHORT[e], ATK_NAME[sp], dmg[e], cyc, dmg[e] / cyc);
        }
        for (int k = 0; k < NKINDS; k++) {
            if (KINDS[k].debut > L || KINDS[k].hp >= 1e8f || (KINDS[k].flags & KF_SPIRIT) || !(KINDS[k].flags & KF_PAGE) || k == MOB_WSKULL) continue;
            int r = imin(top, KINDS[k].top);
            float hp = mob_base_hp(k) * powf(1.04f, (float)(L - 1)) * RANK_HP[r];
            fprintf(f, "    %-16s %-3s hp %6.0f |", KINDS[k].name, r == RK_I ? "I" : r == RK_II ? "II" : r == RK_III ? "III" : r == RK_IV ? "IV" : "V", hp);
            for (int e = 0; e < NELEM; e++) {
                float w = kind_weak(k, e);                     /* the dead's track's: CRAWL_ELEM=0 gives 1.0.0b18's */
                if (w <= 0.0f) fprintf(f, " %s    -", CL2[e]); else fprintf(f, " %s %4.1f", CL2[e], hp / (dmg[e] * w));
            }
            fprintf(f, "\n");
        }
    }
    G.level = keep_level; G.retries = keep_retries; G.ptheme = keep_theme;
}
#endif
/* a creature's usual blow and how often it comes: what it gives a web, one of yours, or one of theirs */
static float mob_blow(const Mob *m, float *every) {               /* its usual attack's row (docs/PLAN-combat.md, section 3) */
    int fa = fa_main(m->type);
    if (fa < 0 || ATTACKS[fa].k <= 0.0f) { *every = 2.5f; return mob_damage(m); }   /* those that never strike: as they were */
    *every = fa_every(fa);
    return mob_damage(m) * ATTACKS[fa].k + ATTACKS[fa].add;
}

/* Soul Sap (docs/PLAN-b11.md, section 6): a bat of rank II or more that stays on your face latches on instead of biting:
 * 3 s at rank II, 2.4 at III, 1.8 at IV, 1.2 at V. Latched it saps a fifth of its bite's health a second and half a soul
 * (a soul at rank V), and you walk at 70%; Resist Spirit takes its share off each. Turning the view hard one way and back,
 * or strafing left and right, loosens it: each reversal within 0.4 s counts the time since the last, and it lets go at its
 * rank's need; stop, and the grip creeps back. Ten seconds and it lets go by itself; Blink and Swift Dodge throw it off */
static const float LATCH_T[NRANK] = { 99.0f, 3.0f, 2.4f, 1.8f, 1.2f, 99.0f }, SHAKE_NEED[NRANK] = { 0.0f, 0.6f, 0.9f, 1.2f, 1.6f, 0.0f };
static int latch_begin(Mob *m) {
    if (G.latch || G.dead || m->rank < RK_II || m->rank == RK_GILDED || m->contact < LATCH_T[clampi(m->rank, 0, NRANK - 1)] * soft_pace() || m->webbed > 0.0f) return 0;   /* Survival: rarer */
    G.latch = (int)(m - G.mobs) + 1; G.latch_t = G.shake = G.sap_acc = G.slip_k = 0.0f;
    G.shake_dir = G.strafe_dir = 0; G.shake_last = G.strafe_last = -9.0f; G.shake_ang = G.ang;
    m->contact = 0.0f;
    sfx_at(SFX_BAT, m->x, m->y, 1.3f); snd_sfx_ex(SFX_WHISPER, 0.7f, 0.5f);
    if (G.player && !RUN.latch_said) { RUN.latch_said = 1; say("THE BAT FEEDS. SHAKE IT OFF."); }
    RUN.cx_more |= CXM_SAP;                                        /* and into the Codex */
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT latch rank %d by %s t=%.2f\n", m->rank, G.player ? "player" : "autopilot", G.t);
#endif
    return 1;
}
static void latch_release(int thrown) {                           /* it lets go: off a few cells, and it comes again */
    if (!G.latch) return;
    Mob *m = &G.mobs[G.latch - 1];
    G.latch = 0; G.slip_k = 0.0f;
    if (!m->alive || m->state != MS_WALK) return;
    m->feared = 1.6f; m->wfeared = 0; m->attacking = 0; m->contact = 0.0f; m->cool = 1.0f;
    m->x = G.px - cosf(G.ang) * 0.4f; m->y = G.py - sinf(G.ang) * 0.4f;
    sfx_at(SFX_BAT, m->x, m->y, 1.4f);
    if (thrown != 2) por_bump(MOB_BAT);                            /* shaken or thrown off, not fed: SHAKEN OFF (the portraits') */
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT latch_off %s after %.1f s t=%.2f\n", thrown == 2 ? "itself" : thrown ? "thrown" : "shaken", G.latch_t, G.t);
#endif
}
static float sap_slow(void) { return G.latch ? 1.0f - 0.3f * (1.0f - gear_total(&RUN.gear, ST_RES_SPIRIT) / 100.0f) : 1.0f; }
static void update_latch(float dt) {
    if (!G.latch) return;
    Mob *m = &G.mobs[G.latch - 1];
    if (!m->alive || m->state != MS_WALK || G.dead || G.shop) { G.latch = 0; return; }
    m->x = G.px + cosf(G.ang) * 0.3f; m->y = G.py + sinf(G.ang) * 0.3f; m->cx = m->tx = (int)m->x; m->cy = m->ty = (int)m->y;
    m->contact = 0.0f;
    if ((G.latch_t += dt * soft_pace()) >= 10.0f) { latch_release(2); return; }   /* fed enough; Survival, sooner */
    float res = 1.0f - gear_total(&RUN.gear, ST_RES_SPIRIT) / 100.0f, t = (float)G.t;
    float every, bite = mob_blow(m, &every);
    G.sap_acc += 0.2f * bite * res * dt;                           /* its drain: health, a fifth of its bite a second, */
    if (G.sap_acc >= 1.0f) {
        float d = floorf(G.sap_acc); G.sap_acc -= d; harm_next = EL_SPIRIT; hurt_by(m, d, HURT_POISON);
        G.face_side = 0; G.face_t = fmaxf(G.face_t, FACE_T * 0.6f);   /* the portrait winces */
    }
    if (G.dead || !G.latch) return;
    spend_spirit(fminf(spirit(), (m->rank >= RK_V ? 1.0f : 0.5f) * res * dt));   /* and souls */
    int shook = 0;                                                 /* the view turned hard one way and back, */
    float turn = wrap_pi(G.ang - G.shake_ang) / fmaxf(dt, 1e-3f);
    G.shake_ang = G.ang;
    if (fabsf(turn) > 2.5f) {
        int d = turn > 0.0f ? 1 : -1;
        if (d != G.shake_dir) { if (G.shake_dir && t - G.shake_last < 0.4f) { G.shake += t - G.shake_last; shook = 1; } G.shake_dir = d; G.shake_last = t; }
    }
    float lat = G.pvx * -sinf(G.ang) + G.pvy * cosf(G.ang);       /* or the feet, strafing left and right */
    if (fabsf(lat) > 0.9f) {
        int d = lat > 0.0f ? 1 : -1;
        if (d != G.strafe_dir) { if (G.strafe_dir && t - G.strafe_last < 0.4f) { G.shake += t - G.strafe_last; shook = 1; } G.strafe_dir = d; G.strafe_last = t; }
    }
    if (!G.player) G.shake += dt * 0.32f;                          /* the dream shakes it too, a little slower */
    if (shook) { G.slip_k = 1.0f; rumble(0.3f, 0.2f, 0.12f); sfx_at(SFX_BAT, m->x, m->y, 0.9f); DBG_COUNT(shakes); }
    else if (G.player && t - fmaxf(G.shake_last, G.strafe_last) > 0.6f) G.shake = fmaxf(0.0f, G.shake - 0.15f * dt);   /* the grip creeping back */
    G.slip_k = fmaxf(0.0f, G.slip_k - dt * 3.0f);
    if (G.shake >= SHAKE_NEED[clampi(m->rank, 0, NRANK - 1)]) latch_release(0);
}

/* Tangled Web: a glob spat at what you aim at, eight cells a second and eight cells' reach. It holds what
 * walks or flies and has a body (not a spirit, a shade, a wall skull, a sac or a web spider, nor a boss,
 * from which it falls away torn), and the held thing claws at the web, at half its usual rate, till it
 * has done as much to it as the web holds: a third of your health, two thirds, or all of it, by rank.
 * One web out a rank; the wait is 2 s after a miss, the whole of it after a hit. */
static int web_holds(const Mob *m) {
    if (!m->alive || m->state != MS_WALK || m->boss || m->ally || buried(m)) return 0;
    if (KIND_IS(m, KF_SPIRIT) || m->type == MOB_SHADE || m->type == MOB_WSKULL || m->type == MOB_SAC || m->type == MOB_SPIDER) return 0;
    return !(m->type == MOB_GARGOYLE && m->sub == 0);
}
static int webs_out(void) {
    int n = 0;
    for (int i = 0; i < MAXMOBS; i++) n += G.mobs[i].alive && G.mobs[i].state == MS_WALK && G.mobs[i].webbed > 0.0f;
    return n;
}
static int web_spit(int t) {
    if (webs_out() >= RUN.abil[BOSS_SPIDER]) {
        if (G.player) { say(RUN.abil[BOSS_SPIDER] > 1 ? "ALL YOUR WEBS ARE OUT" : "YOUR WEB IS OUT"); snd_sfx_ex(SFX_KNOCK, 0.4f, 0.5f); }
        return 0;
    }
    float a = t >= 0 && !G.player ? atan2f(G.mobs[t].y - G.py, G.mobs[t].x - G.px) : G.ang;   /* a person's goes where they look */
    Shot *s = new_shot(SHOT_WEB, G.px + cosf(a) * 0.35f, G.py + sinf(a) * 0.35f, a, 8.0f, 0.0f);
    if (!s) return 0;
    s->life = 1.0f; s->tier = 0;
    G.ab_cd[BOSS_SPIDER] = 2.0f;                                   /* what a miss costs; a hit, the whole wait */
    snd_sfx_ex(SFX_THROW, 0.8f, 0.5f); snd_sfx_ex(SFX_SKITTER, 0.6f, 0.5f);
    if (G.player) G.grab_t = fmaxf(G.grab_t, 0.7f);
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT web_spit at %d t=%.2f\n", t, G.t);
#endif
    return 1;
}
static void web_miss(float x, float y) {
    sfx_at(SFX_SKITTER, x, y, 0.5f);
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT web_miss t=%.2f\n", G.t);
#endif
}
static void web_land(Shot *s, Mob *m) {
    s->alive = 0;
    if (!web_holds(m)) {                                           /* it falls away, torn */
        sfx_at(SFX_SKITTER, m->x, m->y, 1.0f); sfx_at(SFX_SHATTER, m->x, m->y, 0.4f);
        if (G.player && m->boss) say("THE WEB TEARS AWAY FROM IT");
        web_miss(m->x, m->y);
        return;
    }
    float every;
    mob_blow(m, &every);
    m->webbed = m->webmax = (float)G.max_hp * (float)RUN.abil[BOSS_SPIDER] / 3.0f;
    m->cool = every; m->claw = 0.0f; m->swing = 0.0f; m->throw_t = 0.0f; m->attacking = 0;
    if (m->type == MOB_BAT) m->contact = 0.0f;
    if (m->type == MOB_DEACON) m->deco = -1;                       /* its chant broken off */
    G.ab_cd[BOSS_SPIDER] = ab_wait(BOSS_SPIDER);
    sfx_at(SFX_DIRT, m->x, m->y, 0.9f); sfx_at(SFX_SKITTER, m->x, m->y, 0.8f);
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT web_hold %s web=%.0f t=%.2f\n", KINDS[m->type].name, m->webbed, G.t);
#endif
}
static void web_claw(Mob *m, float dt) {                           /* held: it claws at the web instead of you */
    m->claw = fmaxf(0.0f, m->claw - dt);
    if ((m->cool -= dt) > 0.0f) return;
    float every, b = mob_blow(m, &every);
    m->cool = 2.0f * every; m->claw = 0.3f;
    m->webbed -= b;
    sfx_at(SFX_SKITTER, m->x, m->y, 0.5f);
    if (m->webbed > 0.0f) return;
    m->webbed = 0.0f; m->claw = 0.0f; m->cool = 0.4f;
    m->alert = fmaxf(m->alert, 20.0f);                             /* free, and it comes for you at once */
    sfx_at(SFX_SHATTER, m->x, m->y, 0.5f);
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT web_free %s t=%.2f\n", KINDS[m->type].name, G.t);
#endif
}

/* Grave Call: the remains of something you slew, within four cells and in sight, get up and fight for you
 * for 12, 16 or 20 s, one, two or three at once, by rank. Creatures turn on it as they would on Blink's
 * double, and it strikes whatever is near; then it falls back into what it was. */
static void note_remains(const Mob *m, int deco) {
    if (m->type == MOB_SPIDER || m->type == MOB_SAC || m->type == MOB_LING) return;
    Remains *r = &G.remains[G.nremains++ % MAXREMAINS];
    r->deco = deco; r->type = m->type; r->rank = m->rank; r->t = (float)G.level_t;
}
static int remains_ok(const Remains *r) {
    return r->type >= 0 && r->deco >= 0 && r->deco < G.ndecos && G.decos[r->deco].spr == KINDS[r->type].remains;
}
/* the remains to raise: those aimed at (a person), or the nearest fresh ones (the autopilot); -1 if none */
static int remains_near(int aimed) {
    int best = -1; float bs = 1e9f;
    for (int i = 0; i < MAXREMAINS && i < G.nremains; i++) {
        const Remains *r = &G.remains[i];
        if (!remains_ok(r)) continue;
        const Deco *dc = &G.decos[r->deco];
        float d = hypotf(dc->x - G.px, dc->y - G.py), sc = d;
        if (d > 4.0f || !line_clear(G.px, G.py, dc->x, dc->y)) continue;
        if (aimed) {
            float off = fabsf(wrap_pi(atan2f(dc->y - G.py, dc->x - G.px) - G.ang));
            if (off > 0.3f + 0.45f / fmaxf(d, 0.6f)) continue;
            sc = off * 4.0f + d * 0.2f;
        } else if ((float)G.level_t - r->t > 20.0f) continue;
        if (sc < bs) { bs = sc; best = i; }
    }
    return best;
}
static int allies_out(void) {
    int n = 0;
    for (int i = 0; i < MAXMOBS; i++) n += G.mobs[i].alive && G.mobs[i].ally && G.mobs[i].state == MS_WALK;
    return n;
}
static int grave_call(int ri) {
    int rank = RUN.abil[BOSS_DEACON];
    if (allies_out() >= rank) {
        if (G.player) { say(rank > 1 ? "ALL WHO WOULD ANSWER STAND ALREADY" : "ONE STANDS FOR YOU ALREADY"); snd_sfx_ex(SFX_KNOCK, 0.4f, 0.5f); }
        return 0;
    }
    Remains *r = &G.remains[ri];
    Deco *dc = &G.decos[r->deco];
    int x = (int)dc->x, y = (int)dc->y;
    if (G.map[y][x].kind != K_FLOOR || occupied(x, y, NULL)) {      /* the floor beside it, if something stands on it */
        int d = 0;
        for (; d < 4; d++) if (cell(x + DX[d], y + DY[d])->kind == K_FLOOR && !occupied(x + DX[d], y + DY[d], NULL)) break;
        if (d == 4) { if (G.player) say("NO ROOM FOR IT TO RISE"); return 0; }
        x += DX[d]; y += DY[d];
    }
    Mob *m = spawn_mob(x, y, r->type);
    if (!m) return 0;
    G.spawned[r->type]--;                                          /* no newcomer, and no kill to be had of it */
    if (!KIND_IS(m, KF_UNCOUNTED)) G.kills_total--;
    set_rank(m, r->rank);
    m->ally = 1; m->ally_t = 8.0f + 4.0f * (float)rank; m->home = -1; m->alert = 0.0f;
    m->z = -0.55f; m->face = G.ang + PI_F;
    if (m->type == MOB_HUNTER || m->type == MOB_GARGOYLE) m->sub = 2;   /* on its feet (or wings), not overhead or a statue */
    dc->spr = -1; r->type = -1;
    G.ab_cd[BOSS_DEACON] = ab_wait(BOSS_DEACON);
    sfx_at(SFX_DIRT, m->x, m->y, 1.1f); sfx_at(SFX_BONES, m->x, m->y, 0.9f); snd_sfx_ex(SFX_RUNE, 0.7f, 0.5f);
    if (G.player) G.grab_t = fmaxf(G.grab_t, 0.7f);
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT gravecall %s rank %d for %.0f s t=%.2f\n", KINDS[m->type].name, m->rank, m->ally_t, G.t);
#endif
    return 1;
}
static void ally_fall(Mob *m) {                                    /* its time is up, or they have beaten it down */
    m->state = MS_DIE; m->t = 0.0f; m->sub = 0;
    sfx_at(KINDS[m->type].sfx_die, m->x, m->y, 0.8f);
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT ally_falls %s hp=%.0f t=%.2f\n", KINDS[m->type].name, m->hp, G.t);
#endif
}
static void ally_hurt(Mob *a, float dmg) {
    a->hp -= dmg;
    if (a->hp <= 0.0f && a->state == MS_WALK) ally_fall(a);
}
static Mob *ally_near(const Mob *m, float r) {                     /* one of yours for this creature to turn on */
    Mob *best = NULL; float bd = r;
    for (int i = 0; i < MAXMOBS; i++) {
        Mob *a = &G.mobs[i];
        if (!a->alive || !a->ally || a->state != MS_WALK || a->z < -0.05f) continue;
        float d = hypotf(a->x - m->x, a->y - m->y);
        if (d < bd && line_clear(m->x, m->y, a->x, a->y)) { bd = d; best = a; }
    }
    return best;
}
static int targetable(const Mob *m);
static void update_ally(Mob *m, float dt) {
    m->t += dt;
    float zt = m->type == MOB_BAT ? 0.2f : (m->type == MOB_GARGOYLE ? GARG_Z : 0.0f);
    if (m->z < zt - 0.01f) { m->z = fminf(zt, m->z + dt); return; }   /* getting up out of what it left */
    m->z = m->type == MOB_BAT ? 0.2f + 0.07f * sinf(m->t * 7.0f) : zt;
    if ((m->ally_t -= dt) <= 0.0f) { ally_fall(m); return; }
    m->claw = fmaxf(0.0f, m->claw - dt);
    m->acool -= dt;
    Mob *t = NULL;
    float bd = 8.0f;
    for (int i = 0; i < MAXMOBS; i++) {                            /* the nearest of them in its sight */
        Mob *o = &G.mobs[i];
        if (o == m || !targetable(o)) continue;
        float d = hypotf(o->x - m->x, o->y - m->y);
        if (d < bd && line_clear(m->x, m->y, o->x, o->y)) { bd = d; t = o; }
    }
    if (t) m->face = atan2f(t->y - m->y, t->x - m->x);
    if (t && bd < 1.35f + (t->boss ? 0.5f : 0.0f)) {               /* at it */
        if (m->acool <= 0.0f) {
            float every, b = mob_blow(m, &every);
            m->acool = every; m->claw = 0.3f;
            sfx_at(m->type == MOB_SHIELD || m->type == MOB_KNIGHT ? SFX_SWORD : SFX_BONES, m->x, m->y, 0.8f);
            damage_mob(t, b, DM_ARC, m->x, m->y, 0.0f);
        }
        return;
    }
    if (!t && hypotf(G.px - m->x, G.py - m->y) < 2.0f) return;     /* nothing to fight: it keeps by you */
    m->anim += dt * m->speed * 3.0f;
    float gx, gy; cell_pass(m->tx, m->ty, &gx, &gy);
    float dx = gx - m->x, dy = gy - m->y, d = hypotf(dx, dy), step = m->speed * dt;
    if (d > step) { m->x += dx / d * step; m->y += dy / d * step; return; }
    m->x = gx; m->y = gy; m->cx = m->tx; m->cy = m->ty;
    int best = -1, here = hunt[m->cy][m->cx];
    float bs = t ? hypotf(m->cx + 0.5f - t->x, m->cy + 0.5f - t->y) - 0.01f : 1e9f;
    for (int k = 0; k < 4; k++) {
        int nx = m->cx + DX[k], ny = m->cy + DY[k];
        if ((m->type == MOB_GARGOYLE ? blocks_flight(nx, ny) : blocks_move(nx, ny)) || occupied(nx, ny, m)) continue;
        float sc;
        if (t) sc = hypotf(nx + 0.5f - t->x, ny + 0.5f - t->y) + rndf() * 0.2f;   /* straight at it */
        else { int h = hunt[ny][nx]; if (h < 0 || (here >= 0 && h >= here)) continue; sc = (float)h + rndf() * 0.3f; }   /* after you, round corners */
        if (sc < bs) { bs = sc; best = k; }
    }
    if (best >= 0) { m->tx = m->cx + DX[best]; m->ty = m->cy + DY[best]; }
}

/* Wraithwalk: for 1.5, 2 or 2.5 s you pass through creatures and their missiles, and they lose you */
static void wraithwalk(void) {
    G.wwalk_t = 1.0f + 0.5f * (float)RUN.abil[BOSS_WRAITH];
    G.ab_cd[BOSS_WRAITH] = ab_wait(BOSS_WRAITH);
    for (int i = 0; i < MAXMOBS; i++) {                            /* they lose you, and must find you again */
        Mob *m = &G.mobs[i];
        if (!m->alive || m->ally) continue;
        m->alert = 0.0f; m->lost = 9.0f; m->attacking = 0;
        if (m->type == MOB_SKEL) { m->seen = 0; m->throw_t = 0.0f; }
        if (m->swing > 0.0f && m->type != MOB_DEACON) m->swing = 0.0f;   /* a blow on its way finds nothing */
    }
    snd_sfx_ex(SFX_WAIL, 0.45f, 0.5f); snd_sfx_ex(SFX_SHIMMER, 0.7f, 0.5f);
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT wraithwalk %.1f s t=%.2f by %s\n", G.wwalk_t, G.t, G.player ? "player" : "autopilot");
#endif
}

/* Swift Dodge: a dart of two to five cells, a quarter of a second, untouchable; it stops before walls,
 * doors, props and creatures, and with less than two cells clear nothing happens and nothing is spent */
static int body_blocked(float x, float y, int *bx, int *by);
static float dash_reach(float a, float most) {
    float ca = cosf(a), sa = sinf(a), d = 0.0f;
    int bx, by;
    while (d + 0.1f <= most) {
        float nx = G.px + ca * (d + 0.1f), ny = G.py + sa * (d + 0.1f);
        if (body_blocked(nx, ny, &bx, &by) || cell((int)nx, (int)ny)->kind == K_DOOR) break;
        int mob = 0;
        for (int i = 0; i < MAXMOBS && !mob; i++) { const Mob *m = &G.mobs[i]; mob = solid_mob(m) && m->state == MS_WALK && !m->ally && hypotf(m->x - nx, m->y - ny) < 0.55f; }
        if (mob) break;
        d += 0.1f;
    }
    return d;
}
static void dash_begin(float vx, float vy) {
    G.dash_t = 0.25f; G.dash_vx = vx; G.dash_vy = vy;
    G.ab_cd[BOSS_KNIGHT] = ab_wait(BOSS_KNIGHT);
    snd_sfx_ex(SFX_WHOOSH, 1.0f, 0.5f);
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT dodge %.1f cells t=%.2f by %s\n", hypotf(vx, vy) * 0.25f, G.t, G.player ? "player" : "autopilot");
#endif
}
static int swift_dodge(float a) {
    if (RUN.abil[BOSS_KNIGHT] <= 0 || G.dash_t > 0.0f || G.web_hp > 0.0f || G.shop) return 0;
    if (no_cast()) { hush_say(); return 0; }
    if (G.ab_cd[BOSS_KNIGHT] > 0.0f) return 0;                     /* quietly: a double tap is easily made by chance */
    float r = dash_reach(a, 5.0f);
    if (r < 2.0f) return 0;
    dash_begin(cosf(a) * r / 0.25f, sinf(a) * r / 0.25f);
    latch_release(1);
    return 1;
}
/* the autopilot's dart keeps to the lanes: two to five cells to a lane cell, clear all the way, out of
 * any warning, across the way a missile comes if one does */
static int auto_dodge(void) {
    int bx = (int)G.px, by = (int)G.py, best = -1, bk = 0;
    float bs = -1e9f, mvx = 0.0f, mvy = 0.0f;
    for (int i = 0; i < MAXBONES; i++) if (G.bones[i].alive && hypotf(G.bones[i].x - G.px, G.bones[i].y - G.py) < 4.0f) { mvx = G.bones[i].vx; mvy = G.bones[i].vy; }
    float mv = hypotf(mvx, mvy);
    for (int d = 0; d < 4; d++)
        for (int k = 5; k >= 2; k--) {
            int lx = bx + DX[d] * k, ly = by + DY[d] * k, ok = apass(lx, ly) && !hz_danger(lx, ly);
            for (int j = 1; j <= k && ok; j++) ok = passable(bx + DX[d] * j, by + DY[d] * j) && cell(bx + DX[d] * j, by + DY[d] * j)->kind != K_DOOR && !occupied(bx + DX[d] * j, by + DY[d] * j, NULL);
            if (!ok || !walk_clear(G.px, G.py, lx + 0.5f, ly + 0.5f)) continue;
            float sc = (float)k * 0.3f + rndf() * 0.2f;
            if (mv > 0.1f) sc += 2.0f * fabsf((float)DX[d] * mvy - (float)DY[d] * mvx) / mv;   /* across its way */
            for (int i = 0; i < MAXMOBS; i++) {
                const Mob *m = &G.mobs[i];
                if (m->alive && m->state == MS_WALK && !m->ally && !KIND_IS(m, KF_SPIRIT)) sc += 0.15f * fminf(5.0f, hypotf(m->x - lx - 0.5f, m->y - ly - 0.5f));
            }
            if (sc > bs) { bs = sc; best = d; bk = k; }
            break;                                                 /* the farthest that will do, that way */
        }
    if (best < 0) return 0;
    G.tx = bx + DX[best] * bk; G.ty = by + DY[best] * bk;
    dash_begin((G.tx + 0.5f - G.px) / 0.25f, (G.ty + 0.5f - G.py) / 0.25f);
    return 1;
}

/* the right mouse: the readied ability; Q readies the next */
static int aim_target(void);
static void use_readied(void) {
    int b = ab_readied();
    if (b < 0) return;
    if (no_cast()) { hush_say(); return; }
    if (G.ab_cd[b] > 0.0f) { say("%s IN %.0f S", BA_NAME[b], ceilf(G.ab_cd[b])); snd_sfx_ex(SFX_KNOCK, 0.35f, 0.5f); return; }
    show_bar();
    if (b == BOSS_SPIDER) web_spit(aim_target());
    else if (b == BOSS_DEACON) {
        int ri = remains_near(1);
        if (ri < 0) { say("NO REMAINS OF YOURS IN REACH"); snd_sfx_ex(SFX_KNOCK, 0.35f, 0.5f); }
        else grave_call(ri);
    } else wraithwalk();
}
static int hand_runes(int *out) {                                  /* the runes held that a person raises */
    int n = 0;
    for (int k = 0; k < NDEF; k++) if (RUN.gear.def[k] > 0 && rune_by_hand(k)) out[n++] = k;
    return n;
}
static int rune_readied(void) {                                    /* RB's: the one chosen, else the first held, else -1 */
    int h[NDEF], n = hand_runes(h);
    for (int i = 0; i < n; i++) if (h[i] == RUN.rune_sel) return h[i];
    return n ? h[0] : -1;
}
static void rune_hand(int k) {                                     /* a rune raised by hand, its cooldown as ever */
    if (k < 0) { say("NO RUNE TO RAISE BY HAND"); snd_sfx_ex(SFX_KNOCK, 0.35f, 0.5f); return; }
    if (rune_barred(k)) { hush_say(); return; }
    if (G.def_cd[k] > 0.0f) { say("%s IN %.0f S", DEF_NAME[k], ceilf(G.def_cd[k])); snd_sfx_ex(SFX_KNOCK, 0.35f, 0.5f); return; }
    show_bar();
    cast_defense(k);
}
/* Q (the pad's X) readies the next for the right mouse: the runes raised by hand, then the bosses' gifts */
static void ready_next(void) {
    int h[NDEF], nr = hand_runes(h), L[NDEF + NBOSS], n = 0, cur = -1;
    for (int i = 0; i < nr; i++) L[n++] = h[i];
    for (int b = BOSS_SPIDER; b < NBOSS; b++) if (ab_on_mouse(b)) L[n++] = 100 + b;
    if (n <= 1) return;
    int now = RUN.mouse_rune && rune_readied() >= 0 ? rune_readied() : ab_readied() >= 0 ? 100 + ab_readied() : (nr ? h[0] : -1);
    for (int i = 0; i < n; i++) if (L[i] == now) cur = i;
    int nx = L[(cur + 1) % n];
    if (nx >= 100) { RUN.abil_sel = nx - 100; RUN.mouse_rune = 0; say("READIED: %s", BA_NAME[nx - 100]); }
    else { RUN.rune_sel = nx; RUN.mouse_rune = 1; say("READIED: %s", DEF_NAME[nx]); }
    snd_sfx_ex(SFX_TICK, 0.7f, 0.5f); show_bar();
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT ready_next %s %d t=%.2f\n", nx >= 100 ? "gift" : "rune", nx >= 100 ? nx - 100 : nx, G.t);
#endif
}
static void use_right(void) {                                      /* the right mouse (the pad's LT): whichever was readied last */
    int r = rune_readied();
    if (r >= 0 && (RUN.mouse_rune || ab_readied() < 0)) rune_hand(r);
    else use_readied();
}

/* the autopilot, if the page has them: Swift Dodge out of a missile's way or a warning, Tangled Web on the
 * nearest creature that comes to blows, Grave Call on fresh remains in a fight, Wraithwalk when cornered */
static void auto_abilities(void) {
    if (G.dead || no_cast() || G.shop || G.exiting || G.dash_t > 0.0f) return;
    int st = G.state;
    if (RUN.abil[BOSS_KNIGHT] && G.ab_cd[BOSS_KNIGHT] <= 0.0f && G.web_hp <= 0.0f
        && (st == PS_MOVE || st == PS_TURN || st == PS_AIM || st == PS_CAST || st == PS_HOLD || st == PS_UNAIM)
        && (incoming(0.45f) || hz_danger((int)G.px, (int)G.py)) && auto_dodge()) return;
    if (RUN.abil[BOSS_SPIDER] && G.ab_cd[BOSS_SPIDER] <= 0.0f && webs_out() < RUN.abil[BOSS_SPIDER]) {
        int best = -1; float bd = 2.2f;
        for (int i = 0; i < MAXMOBS; i++) {
            const Mob *m = &G.mobs[i];
            if (!web_holds(m) || !targetable(m) || !KIND_IS(m, KF_MELEE) || m->alert <= 0.0f || m->webbed > 0.0f) continue;
            float d = hypotf(m->x - G.px, m->y - G.py);
            if (d < bd && shot_clear(m->x, m->y)) { bd = d; best = i; }
        }
        if (best >= 0 && web_spit(best)) return;
    }
    if (RUN.abil[BOSS_DEACON] && G.ab_cd[BOSS_DEACON] <= 0.0f && allies_out() < RUN.abil[BOSS_DEACON] && in_combat()) {
        int ri = remains_near(0);
        if (ri >= 0 && grave_call(ri)) return;
    }
    if (RUN.abil[BOSS_WRAITH] && G.ab_cd[BOSS_WRAITH] <= 0.0f && G.wwalk_t <= 0.0f && G.hp < G.max_hp * 0.35f) {
        int near = enemies_within(2.2f, 0);
        if (near >= 2 || (near >= 1 && incoming(0.8f))) wraithwalk();
    }
}

/* every frame: the waits running down, a soft tick as one comes round, Wraithwalk wearing off */
static void ab_update(float dt) {
    int ready = 0;
    for (int b = 0; b < NBOSS; b++) {
        if (G.ab_cd[b] > 0.0f) G.ab_cd[b] -= dt;
        if (RUN.abil[b] && G.ab_cd[b] <= 0.0f) ready |= 1 << b;
    }
    if (G.player && (ready & ~G.ab_ready)) snd_sfx_ex(SFX_TICK, 0.45f, 0.3f);
    G.ab_ready = ready;
    if (G.wwalk_t > 0.0f && (G.wwalk_t -= dt) <= 0.0f) { G.wwalk_t = 0.0f; snd_sfx_ex(SFX_WHOOSH, 0.45f, 0.5f); }
}

/* a door's sound, opening or shut, by the run (docs/PLAN-ai.md, section 29); her hall's are run I's */
static int door_sfx(int shut) {
    static const int S[4][2] = { { SFX_DOOR1_OPEN, SFX_DOOR1_SHUT }, { SFX_DOOR2_OPEN, SFX_DOOR2_SHUT },
                                 { SFX_DOOR3_OPEN, SFX_DOOR3_SHUT }, { SFX_DOOR4_OPEN, SFX_DOOR4_SHUT } };
    return S[G.shop ? 0 : clampi(crawl_set(G.level), 0, 3)][shut != 0];
}
/* the caster's girth over a door's cell, as body_blocked would find it: a door that seals itself never shuts on it */
static int door_body(const Door *d) {
    return G.px + BODY + 0.02f >= (float)d->x && G.px - BODY - 0.02f < (float)(d->x + 1)
        && G.py + BODY + 0.02f >= (float)d->y && G.py - BODY - 0.02f < (float)(d->y + 1);
}
static void update_doors(float dt) {
    for (int i = 0; i < G.ndoors; i++) {
        Door *d = &G.doors[i];
        float pdist = hypotf(d->x + 0.5f - G.px, d->y + 0.5f - G.py);
        if (d->opening && !d->was) {                               /* a door opening is heard (S3), and sounds like its run */
            noise(d->x + 0.5f, d->y + 0.5f, 4, NZ_DOOR, -1);
            sfx_at(door_sfx(0), d->x + 0.5f, d->y + 0.5f, 0.95f);
        }
        d->was = d->opening;
        if (d->opening) {
            d->open = fminf(1.0f, d->open + dt / 0.85f);
            if (d->open >= 1.0f) { d->opening = 0; d->idle = 0.0f; }
        } else if (d->open > 0.0f && !(g_drawn == 2 && i == IC.gate_door && !d->locked)) {   /* the tutorial's gate, risen, stays up */
            int busy = pdist < 1.6f || (d->locked == 2 && vault_of_room((int)G.px, (int)G.py) == d->color);
            if (d->locked == SEALED || d->locked == TRIALED || d->locked == GATED) busy = door_body(d);   /* it shuts behind you, not on you */
            for (int m = 0; m < MAXMOBS; m++)
                if (G.mobs[m].alive && ((G.mobs[m].cx == d->x && G.mobs[m].cy == d->y) || (G.mobs[m].tx == d->x && G.mobs[m].ty == d->y))) busy = 1;
            d->idle = busy ? 0.0f : d->idle + dt;
            if (d->idle > 2.5f) {
                d->open = fmaxf(0.0f, d->open - dt / 0.85f);
                if (d->open <= 0.0f) sfx_at(door_sfx(1), d->x + 0.5f, d->y + 0.5f, 0.8f);   /* shut */
            }
        }
    }
}

