/* A part of scene_crawl.c (docs/PLAN-tidy.md): it is #included there in its place and never compiled on its own.
 * A boss's fight: fonts, boss_begin, waves and adds, boss_fall, update_boss. */

/* ---- a boss's fight (docs/PLAN-b7.md) ----
 * It waits, untouchable, until you are a step into its arena; then the arch seals behind you, its name
 * comes up and its bar along the top of the view. At 75%, 50%, 25% and 10% of its health it stops (the
 * blow that would take it past the mark is lost), turns untouchable and calls a wave; two seconds later
 * the adds come in where its map says, and once the last of them is down it rises and fights on. When it
 * falls, its scrip drops, the way out opens, and on the way down the run is done. */
static Mob *boss_mob(void) {
    if (G.boss < 0 || G.bmob < 0 || PK.depth) return NULL;
    Mob *m = &G.mobs[G.bmob];
    return m->alive && m->boss ? m : NULL;
}
static int boss_sealed(void) { return G.boss >= 0 && !PK.depth && G.bstate != BS_DEAD; }   /* its way out shut till it falls */
static int boss_fighting(void) { return G.boss >= 0 && !PK.depth && !G.shop && G.bstate >= BS_FIGHT && G.bstate <= BS_RISE; }
static int in_arena(float x, float y) { return G.boss >= 0 && !PK.depth && y >= BAND_Y0 && x >= ARENA_X0 && x < MAPN - 1 && y < MAPN - 1; }   /* the map's rim is stone */
static void font_fill(int i) {
    if (i < 0 || i >= G.bnfont || G.bfont_full[i]) return;
    G.bfont_full[i] = 1; G.map[G.bfont_y[i]][G.bfont_x[i]].tex = T_FONT;
    sfx_at(SFX_DRIP, G.bfont_x[i] + 0.5f, G.bfont_y[i] + 0.5f, 1.0f);
}
static int font_drink(int i) {                                     /* E at a font: a third of your health, once */
    if (!G.bfont_full[i]) { say("THE FONT IS DRY"); snd_sfx_ex(SFX_KNOCK, 0.5f, 0.5f); return 1; }
    G.bfont_full[i] = 0; G.map[G.bfont_y[i]][G.bfont_x[i]].tex = T_FONT_DRY;
    G.hp = imin(G.max_hp, G.hp + (G.max_hp + 2) / 3);
    snd_sfx_ex(SFX_POTION, 1.0f, 0.5f); G.grab_t = 1.0f; show_bar(); pop_readout();
    say("THE FONT GIVES WHAT IT HOLDS");
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT font %d hp=%d t=%.2f\n", i, G.hp, G.t);
#endif
    return 1;
}
static int font_facing(void) {                                     /* the font the dweller faces, near enough to drink */
    int cx = (int)floorf(G.px + cosf(G.ang) * 0.8f), cy = (int)floorf(G.py + sinf(G.ang) * 0.8f);
    for (int i = 0; i < G.bnfont && !PK.depth; i++)
        if (cx == G.bfont_x[i] && cy == G.bfont_y[i] && hypotf(G.bfont_fx[i] + 0.5f - G.px, G.bfont_fy[i] + 0.5f - G.py) < 1.2f) return i;
    return -1;
}

static void boss_spawn(void) {
    const BossDef *B = &BOSSES[G.boss];
    G.bmob = -1;
    if (G.bhome_x < 0) return;
    Mob *m = spawn_mob(G.bhome_x, G.bhome_y, B->kind);
    if (!m) return;
    m->boss = 1 + G.boss;
    set_rank(m, RK_I);
    float hp = B->hp * KINDS[MOB_KNIGHT].hp * powf(1.04f, (float)(G.level - 1)) * (1.0f + 0.25f * (float)G.bstep);
#ifdef CRAWL_DEBUG
    if (getenv("CRAWL_BOSSHP")) hp *= (float)atof(getenv("CRAWL_BOSSHP"));
#endif
    m->hp = m->maxhp = hp;
    m->speed *= B->speed;
    m->dmgk *= B->blow * (1.0f + 0.1f * (float)G.bstep);
    m->home = -1; m->face = PI_F;                                  /* facing the arch */
    if (m->type == MOB_HUNTER) m->sub = 0;                         /* overhead, waiting */
    G.bmob = (int)(m - G.mobs);
}

static void boss_begin(void) {
    G.bstate = BS_FIGHT; G.bstate_t = 0.0f; G.bfight_t = 0.0f; G.bcard = 0.001f;
    if (G.barch >= 0) { Door *d = &G.doors[G.barch]; d->locked = SEALED; d->opening = 0; d->idle = 99.0f; }
    Mob *m = boss_mob();
    if (m) { m->alert = 99.0f; m->seen = 1; rec_seen(m); }
    if (m && G.boss == BOSS_DEACON) G.bward = 0.04f * m->maxhp;   /* his Bone Ward, whole */
    RUN.rec.boss[G.boss].met = 1;
    if (RUN.rec.boss[G.boss].tries < 65535) RUN.rec.boss[G.boss].tries++;
    font_fill(0);
    snd_sfx_ex(SFX_DOOR, 1.2f, 0.5f); snd_sfx_ex(SFX_STINGER, 1.0f, 0.5f);
    snd_set_boss(G.boss + 1, 0);                                   /* its waltz */
    show_bar();
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT boss_begin %s step %d hp=%.0f t=%.2f by %s\n", BOSSES[G.boss].name, G.bstep, m ? m->maxhp : 0.0f, G.t, G.player ? "player" : "autopilot");
#endif
}

static void boss_wave(Mob *m) {
    G.bstate = BS_WAVE; G.bstate_t = 0.0f; G.badds = -1; G.bwave++;
    G.bdash_t = 0.0f; G.bpose = BP_NONE; G.bpose_t = 0.0f;
    if (m->type == MOB_KNIGHT && knight_stage(m) != m->sub) {      /* plate falls away */
        m->sub = knight_stage(m); m->speed = knight_pace(m, m->sub);
        sfx_at(SFX_SHATTER, m->x, m->y, 1.3f);
    }
    m->swing = 0.0f; m->throw_t = 0.0f; m->frozen = m->slowed = m->feared = 0.0f; m->stagger = 0.0f;
    snd_sfx_ex(SFX_SHRIEK, 0.8f, 0.5f); snd_sfx_ex(SFX_STINGER, 0.7f, 0.5f);
    snd_set_boss(G.boss + 1, 1);                                   /* turned up while the adds are out */
    G.kick = 0.05f;
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT boss_wave %d hp=%.0f/%.0f t=%.2f\n", G.bwave, m->hp, m->maxhp, G.t);
#endif
}

/* the rank its adds come at: the highest of each kind met since the last death, a rank more at the
 * challenge's second step and another at its fourth; never under what the level would bring */
static int add_rank(int type) {
    int r = pick_rank(type), top = type < REC_KINDS ? RUN.top_rank[type] - 1 : -1;
    if (top > r && top < RK_GILDED) r = top;
    r += (G.bstep >= 2) + (G.bstep >= 4);
    return clampi(r, 0, KINDS[type].top < RK_GILDED ? KINDS[type].top : RK_V);
}

/* a wave's adds, out of the gates, burrows or heaps in turn, or, in the Wraith's arena, out of the dark
 * well away from you; each comes with a puff of dust */
static void boss_adds(void) {
    const BossDef *B = &BOSSES[G.boss];
    const AddGroup *wave = B->waves[clampi(G.bwave - 1, 0, NWAVES - 1)];
    int n = 0, k = rnd(G.bnspawn > 0 ? G.bnspawn : 1);
    for (int g = 0; g < WAVE_GROUPS; g++)
        for (int i = 0; i < wave[g].n + (G.corrupt ? (wave[g].n + 2) / 3 : 0); i++) {   /* the bell's: a third larger */
            int x = -1, y = -1;
            for (int tries = 0; tries < 40 && x < 0; tries++) {
                int sx, sy;
                if (G.bnspawn > 0) { sx = G.bspawn_x[k % G.bnspawn]; sy = G.bspawn_y[k % G.bnspawn]; k++; }
                else { sx = ARENA_X0 + rnd(MAPN - ARENA_X0 - 1); sy = BAND_Y0 + 1 + rnd(BAND_H - 3); if (hypotf(sx + 0.5f - G.px, sy + 0.5f - G.py) < 7.0f) continue; }
                for (int d = -1; d < 4 && x < 0; d++) {
                    int cx = sx + (d < 0 ? 0 : DX[d]), cy = sy + (d < 0 ? 0 : DY[d]);
                    if (in_arena((float)cx, (float)cy) && G.map[cy][cx].kind == K_FLOOR && !occupied(cx, cy, NULL)) { x = cx; y = cy; }
                }
            }
            if (x < 0) continue;
            Mob *m = spawn_mob(x, y, wave[g].type);
            if (!m) continue;
            set_rank(m, add_rank(wave[g].type));
            m->add = 1; m->alert = 30.0f; m->seen = 1; m->home = -1;
            if (wave[g].small) { m->small = 1; m->hp = m->maxhp = m->maxhp * 0.5f; }
            if (m->type == MOB_HUNTER) m->sub = 1;                 /* down and coming */
            if (m->type == MOB_SHAMBLER || m->type == MOB_DROWNED) { m->sub = 2; m->sub_t = 0.0f; m->deco = -1; }   /* up at once */
            for (int j = 0; j < MAXBOOMS; j++)
                if (!G.booms[j].alive) { memset(&G.booms[j], 0, sizeof G.booms[j]); G.booms[j].alive = 1; G.booms[j].x = m->x; G.booms[j].y = m->y; break; }
            n++;
        }
    G.badds = n;
    snd_sfx_ex(SFX_DOOR, 0.9f, 0.5f);
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT boss_adds %d wave %d t=%.2f ranks", n, G.bwave, G.t);   /* each add's kind and rank, I as 1 */
    for (int i = 0; i < MAXMOBS; i++) if (G.mobs[i].alive && G.mobs[i].add == 1 && G.mobs[i].state == MS_WALK) fprintf(stderr, " %d/%d", G.mobs[i].type, G.mobs[i].rank + 1);
    fputc('\n', stderr);
#endif
}

static void boss_fall(Mob *m) {
    const BossDef *B = &BOSSES[G.boss];
    G.bstate = BS_DEAD; G.bstate_t = 0.0f;
    if (m->last_atk >= 0 && m->last_atk < NATK && m->last_atk != AT_BFS) aug_credit(ATK_ELEM[m->last_atk], 10);   /* the line that felled it (A3) */
    G.web_hp = 0.0f;                                               /* its web goes slack */
    rumble(0.9f, 0.5f, 1.4f);
    Item *it = add_item(m->cx, m->cy, IT_SCRIP);
    if (it) { it->secret = 1; it->seen = 1; it->color = G.boss; }
    if (G.barch >= 0) G.doors[G.barch].locked = 0;
    for (int i = 0; i < MAXMOBS; i++) {                            /* what it called crumbles with it */
        Mob *o = &G.mobs[i];
        if (o != m && o->alive && o->state != MS_DIE && in_arena(o->x, o->y)) { o->state = MS_DIE; o->t = 0.0f; }
    }
    Record *r = &RUN.rec;
    int secs = clampi((int)(G.bfight_t + 0.5f), 1, 65535);
    if (r->boss[G.boss].slain < 65535) r->boss[G.boss].slain++;
    if (!r->boss[G.boss].best || secs < r->boss[G.boss].best) r->boss[G.boss].best = (uint16_t)secs;
    char d[32];
    if (!G.bchallenge) { snprintf(d, sizeof d, "SLAIN"); ach_earn(ACH_BOSS0 + G.boss * 6, SP_SCRIP, -1, 0, B->short_name, d); }
    else { snprintf(d, sizeof d, "SLAIN AT STEP %d", G.bstep); ach_earn(ACH_BOSS0 + G.boss * 6 + clampi(G.bstep, 1, 5), SP_SCRIP, -1, 0, B->short_name, d); }
    if (!G.bchallenge) { RUN.runs_done++; RUN.run_top = imax(RUN.run_top, G.level / RUN_LEN); }   /* the run is done, the next opened */
    else if (RUN.bstep[G.boss] < 5) RUN.bstep[G.boss]++;           /* the next challenge a step harder */
    snd_sfx_ex(SFX_FANFARE, 0.9f, 0.5f); snd_sfx_ex(SFX_SHATTER, 1.0f, 0.5f);
    snd_boss_fall();
    say("THE WAY OUT IS OPEN");
    xp_gain(40.0f * (float)BOSS_XP[G.boss] * xp_level() * (G.tri_boon > 0 ? 1.2f : 1.0f), B->short_name);
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT boss_fall %s in %d s t=%.2f by %s\n", B->name, secs, G.t, G.player ? "player" : "autopilot");
#endif
}

/* a blow on the boss: nothing while it is untouchable; a mark stops it and calls the next wave */
static int boss_hurt(Mob *m, float dmg) {
    if (G.bstate != BS_FIGHT) { G.bring = 0.35f; sfx_at(SFX_KNOCK, m->x, m->y, 0.7f); return 0; }   /* a dull ring */
    float hp = m->hp - dmg;
    if (G.bwave < NWAVES && hp <= m->maxhp * WAVE_MARK[G.bwave]) { m->hp = m->maxhp * WAVE_MARK[G.bwave]; boss_wave(m); return 0; }
    m->hp = hp;
    if (m->hp > 0.0f) return 0;
    m->hp = 0.0f;
    kill_mob(m);
    boss_fall(m);
    return 1;
}

static void update_boss(float dt) {
    if (G.boss < 0 || PK.depth || G.shop || G.tally) return;
    if (G.bring > 0.0f) G.bring -= dt;
    if (G.bmirror_t > 0.0f) G.bmirror_t -= dt;
    if (G.bmirror_cd > 0.0f) G.bmirror_cd -= dt;
    G.bdash_age += dt;
    if (G.bpose_t > 0.0f && (G.bpose_t -= dt) <= 0.0f) {            /* a raised sword comes down */
        if (G.bpose == BP_RAISE) { G.bpose = BP_STRIKE; G.bpose_t = 0.35f; Mob *k = boss_mob(); if (k) sfx_at(SFX_SWORD, k->x, k->y, 1.5f); }
        else if (G.bpose == BP_LITANY) {                           /* the Litany ends: if he sees you, it strikes */
            Mob *k = boss_mob();
            G.bpose = BP_NONE;
            if (k && G.bstate == BS_FIGHT) {
                int seen = !G.dead;
                float lx = G.px - k->x, ly = G.py - k->y, ld = hypotf(lx, ly);
                for (float u = 0.0f; u < ld && seen; u += 0.1f) {  /* the timber props hide you; the pews don't */
                    int cx = (int)(k->x + lx / ld * u), cy = (int)(k->y + ly / ld * u);
                    if (blocks_light(cx, cy) || G.map[cy][cx].kind == K_PILLAR) seen = 0;
                }
                if (seen) { G.blame = blame_of(k); harm_next = EL_SPIRIT; hurt_ex((float)G.max_hp * 0.2f, HURT_SPELL, k->x, k->y); sfx_at(SFX_ZAP, G.px, G.py, 1.2f); DBG_COUNT(litany_hits); }
                else { sfx_at(SFX_WHISPER, k->x, k->y, 0.5f); DBG_COUNT(litany_missed); }
            }
        }
        else G.bpose = BP_NONE;
    }
    if (G.bcard > 0.0f) G.bcard += dt;
#ifdef CRAWL_DEBUG
    if (getenv("CRAWL_HOLDCARD") && G.bcard > 1.0f) G.bcard = 1.0f;   /* and a boss's */
#endif
    G.bstate_t += dt;
    Mob *m = boss_mob();
    if (m && m->maxhp > 0.0f) G.bbar += (m->hp / m->maxhp - G.bbar) * fminf(1.0f, dt * 6.0f);
    if (G.bstate == BS_WAIT) {
        if (!G.dead && in_arena(G.px, G.py) && G.px >= ARENA_X0 + 0.6f) boss_begin();
        return;
    }
    if (G.bstate == BS_DEAD) return;
    G.bfight_t += dt;
#ifdef CRAWL_DEBUG
    if (m && G.bstate == BS_FIGHT && (int)((G.bfight_t - dt) / 10.0f) != (int)(G.bfight_t / 10.0f))   /* how the fight goes */
        fprintf(stderr, "BOSS fight t=%.1f hp=%.0f/%.0f ward=%.0f d=%.1f sight=%d target=%d aim=%d state=%d pose=%d swing=%.1f\n", G.t, m->hp, m->maxhp, G.bward,
                hypotf(m->x - G.px, m->y - G.py), line_clear(m->x, m->y, G.px, G.py), targetable(m), G.target == G.bmob, G.state, G.bpose, m->swing);
#endif
    boss_hazards(dt);
    if (G.boss == BOSS_WRAITH) {
        if (G.bslip_t > 0.0f) G.bslip_t -= dt;
        if (G.bbz_at < 0 && !G.player && !G.dead)                 /* the autopilot kindles what it stands by */
            for (int b = 0; b < G.bbz_n; b++)
                if (!G.bbz_lit[b] && hypotf(G.bbz_x[b] + 0.5f - G.px, G.bbz_y[b] + 0.5f - G.py) < 2.0f) { G.bbz_at = b; G.bbz_t = 1.0f; break; }
        if (G.bbz_at >= 0) {
            int b = G.bbz_at;
            if (G.dead || G.bbz_lit[b] || hypotf(G.bbz_x[b] + 0.5f - G.px, G.bbz_y[b] + 0.5f - G.py) > 2.3f) G.bbz_at = -1;   /* walked off */
            else if ((G.bbz_t -= dt) <= 0.0f) { brazier_set(b, 1); G.bbz_at = -1; }
        }
        for (int b = 0; b < G.bbz_n; b++) {
            float bx = G.bbz_x[b] + 0.5f, by = G.bbz_y[b] + 0.5f;
            if (!G.bbz_lit[b])                                     /* a wisp close by kindles it at once */
                for (int i = 0; i < MAXWISPS; i++)
                    if (G.wisps[i].on && G.wisps[i].a > 0.5f && hypotf(G.wisps[i].x - bx, G.wisps[i].y - by) < 1.5f) brazier_set(b, 1);
            if (G.bbz_lit[b])                                      /* the void puts it out */
                for (int i = 0; i < MAXHAZ; i++)
                    if (G.haz[i].alive && G.haz[i].kind == HZ_VOID && hz_covers(&G.haz[i], G.bbz_x[b], G.bbz_y[b]) == 2) { brazier_set(b, 0); break; }
        }
    }
    if (G.boss == BOSS_DEACON && m && G.bward <= 0.0f && (G.bward_cd -= dt) <= 0.0f) {   /* his Bone Ward grows back */
        G.bward = 0.04f * m->maxhp; G.bward_cd = 0.0f;
        sfx_at(SFX_RUNE, m->x, m->y, 1.0f);
        DBG_COUNT(ward_grown);
    }
    if (G.boss == BOSS_SPIDER && G.bnspawn > 0 && (G.btrickle_t += dt) >= 15.0f) {   /* every fifteen seconds, one more out of a burrow */
        int out = 0;
        G.btrickle_t = 0.0f;
        for (int i = 0; i < MAXMOBS; i++) out += G.mobs[i].alive && G.mobs[i].add == 2 && G.mobs[i].state != MS_DIE;
        int k = rnd(G.bnspawn), x = G.bspawn_x[k], y = G.bspawn_y[k];
        for (int d = 0; d < 4 && out < 4; d++) {
            int cx = x + DX[d], cy = y + DY[d];
            if (!in_arena((float)cx, (float)cy) || G.map[cy][cx].kind != K_FLOOR || occupied(cx, cy, NULL)) continue;
            Mob *a = spawn_mob(cx, cy, MOB_HUNTER);
            if (a) { set_rank(a, add_rank(MOB_HUNTER)); a->add = 2; a->small = 1; a->hp = a->maxhp = a->maxhp * 0.5f; a->sub = 2; a->alert = 30.0f; a->seen = 1; a->home = -1; }
            break;
        }
    }
    for (int i = 0; i < MAXMOBS; i++) if (G.mobs[i].alive && (G.mobs[i].boss || G.mobs[i].add)) G.mobs[i].alert = fmaxf(G.mobs[i].alert, 20.0f);
    if (G.bstate == BS_WAVE) {
        if (G.badds < 0) { if (G.bstate_t >= 2.0f) boss_adds(); return; }
        int left = 0;
        for (int i = 0; i < MAXMOBS; i++) left += G.mobs[i].alive && G.mobs[i].add == 1 && G.mobs[i].state != MS_DIE;
        G.badds = left;
#ifdef CRAWL_DEBUG
        if (left && G.bstate_t > 20.0f && (int)((G.bstate_t - dt) / 10.0f) != (int)(G.bstate_t / 10.0f)) {   /* adds that linger */
            fprintf(stderr, "BOSS adds left %d t=%.1f:", left, G.t);
            for (int i = 0; i < MAXMOBS; i++) {
                const Mob *o = &G.mobs[i];
                if (o->alive && o->add == 1 && o->state != MS_DIE)
                    fprintf(stderr, " %s@%.1f,%.1f sub%d alert%.0f hunts%d seen%d", KINDS[o->type].name, o->x, o->y, o->sub, o->alert, o->hunts, o->seen);
            }
            fprintf(stderr, " caster@%.1f,%.1f\n", G.px, G.py);
        }
#endif
        if (left) return;
        G.bstate = BS_RISE; G.bstate_t = 0.0f;
        snd_set_boss(G.boss + 1, 0);                               /* and settling back */
        if (G.bwave == 1) font_fill(1);                            /* a font fills after the first wave, and the third */
        if (G.bwave == 3) font_fill(2);
        if (m) sfx_at(SFX_GROAN, m->x, m->y, 1.0f);
#ifdef CRAWL_DEBUG
        fprintf(stderr, "EVENT boss_rise wave %d t=%.2f\n", G.bwave, G.t);
#endif
    } else if (G.bstate == BS_RISE && G.bstate_t >= 1.2f) { G.bstate = BS_FIGHT; G.bstate_t = 0.0f; }
}

/* Abilities that run by themselves. Treasure Hunter: an eerie whisper from an unopened secret wall
 * within three cells, louder as you near it. Commune: every 15 s, the creatures within its reach show
 * as red shapes through the walls for a moment (render_commune). */
static int summon_wisp(int kind, int color, float cost);
static void wisp_out(Wisp *w);
