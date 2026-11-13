/* A part of scene_crawl.c (docs/PLAN-tidy.md): it is #included there in its place and never compiled on its own.
 * Noise and who hears it, alarms, the Sexton's bell, searching, mind_tick, mind_goal, mind_frame, footsteps. */

/* A noise at a place, heard so many steps off down the halls (S3). The caster's own come from where it stands; a
 * creature's death is heard twice as far by its own kind; Wraithwalk makes no sound of you. With the old minds, only
 * the caster's own are heard, and they tell where it is (alert_near). */
static void noise(float x, float y, int steps, int what, int kind) {
    int caster = what < NZ_LAND;
    if (caster && G.wwalk_t > 0.0f) return;
    if (caster && what != NZ_STEP) { G.noise_x = x; G.noise_y = y; G.noise_t = (float)G.level_t; }   /* what the watch goes to */
    if (!mind_on()) { if (caster) alert_near(steps, what == NZ_STEP ? 4.0f : 8.0f); return; }
    int ix = clampi((int)x, 0, MAPN - 1), iy = clampi((int)y, 0, MAPN - 1);
    int16_t (*f)[MAPN] = caster && ix == hunt_cx && iy == hunt_cy ? hunt : mind_field(ix, iy);
    for (int i = 0; i < MAXMOBS; i++) {
        Mob *m = &G.mobs[i];
        if (!m->alive || m->state != MS_WALK) continue;
        int h = f[m->cy][m->cx];
        if (m->sleep == 1) { if (h >= 0 && h <= steps) pack_call(m); continue; }   /* a roost: anything it hears, it calls */
        if (m->sleep || m->fetch || m->flee > 0.0f) continue;
        if (!minded(m)) {                                          /* the rest hear the caster as they always have */
            int hh = hunt[m->cy][m->cx];
            if (caster && hh >= 0 && hh <= steps) m->alert = fmaxf(m->alert, what == NZ_STEP ? 4.0f : 8.0f);
            continue;
        }
        int reach = what == NZ_DEATH && m->type == kind ? steps * 2 : steps;
        if (m->type == MOB_LIGHTLESS) reach /= 2;                  /* it hears at half the reach the others do */
        if (what == NZ_DEATH && m->type == MOB_GHOUL) reach = steps * 3;   /* a scavenger hears a death from far off */
        if (m->edgy > 0.0f) reach += reach / 2;                    /* after a search it listens harder */
        if (h < 0 || h > reach) continue;
        if (what == NZ_DEATH && m->type == MOB_GHOUL && m->alert <= 0.0f && m->sub != 4 && h > 1) {   /* it goes to feed */
            if (!m->scav) DBG_COUNT(scavenge);
            m->scav = 1; m->scx = x; m->scy = y; m->wait = 0.0f;
            continue;
        }
        if (what == NZ_BELL) {                                     /* the Sexton's bell rung hard: all of them, to his place */
            if (m->alert <= 0.0f && m->type != MOB_SEXTON) { mind_track(m, x, y); DBG_COUNT(bell_heard); }
            continue;
        }
        if (what == NZ_CALL && m->alert <= 0.0f) {                 /* an alarm: a pack goes there hunting, the rest to look */
            const Pack *p = pack_of(m);
            if ((p && !p->broken) || m->aware != MA_CALM) mind_track(m, x, y); else mind_wary(m, x, y);
            DBG_COUNT(alarms_heard);
            continue;
        }
        if (what == NZ_MEND) { m->aware = MA_HUNT; m->alert = fmaxf(m->alert, MIND_LIVE); continue; }   /* it knows where you are */
        if (m->alert > 0.0f) continue;                             /* it already has you */
        if (m->aware == MA_HUNT || m->aware == MA_TRACK || m->aware == MA_SEARCH) { mind_track(m, x, y); DBG_COUNT(tracked); }
        else mind_wary(m, x, y);
    }
}

/* A caller (docs/PLAN-ai.md, section 10): its alarm, heard so many steps off; whatever hears it is wary of the place, a
 * pack goes there hunting. With the old minds nothing hears it. */
static void alarm_at(Mob *m, int steps) {
    if (m->called || G.shop || G.dead) return;
    m->called = 1;
    noise(m->x, m->y, steps, NZ_CALL, m->type);
    if (m->type == MOB_WSKULL || m->type == MOB_GARGOYLE || m->type == MOB_DEACON) way_met(WY_CALLER);
    DBG_COUNT(alarms);
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT alarm %s steps %d t=%.2f\n", KINDS[m->type].name, steps, G.t);
#endif
}

/* The Sexton's bell (section 22): one stroke at each turn of his round, so a person knows about where he is; hard, when he sees
 * you or comes on round having seen you lately: raised half a second (kill him then, and nothing hears it), then rung: the
 * widest call on the level, fourteen steps, and everything that hears it goes to his place. Frost stops it six seconds */
static void sexton_ring(Mob *m, int hard) {
    if (m->ice_t > 0.0f) { sfx_at(SFX_SHATTER, m->x, m->y, 0.3f); return; }   /* frozen: a dull clink */
    if (m->ring_t > 0.0f) return;
    m->rings = hard ? 1 : 0;
    m->ring_t = hard ? 1.7f : 0.45f;
    if (!hard) { sfx_at(SFX_HANDBELL, m->x, m->y, 0.7f); DBG_COUNT(bell_rounds); }
#ifdef CRAWL_DEBUG
    else fprintf(stderr, "EVENT bell_raised t=%.2f\n", G.t);
#endif
}

/* the rooms where a search lost you, which the next search looks into first (section 30) */
static void lost_note(int rg) {
    if (rg < 0) return;
    for (int i = 0; i < G.lost_n; i++) if (G.lost_rg[i] == rg) return;
    memmove(&G.lost_rg[1], &G.lost_rg[0], sizeof G.lost_rg[0] * 7);
    G.lost_rg[0] = rg; if (G.lost_n < 8) G.lost_n++;
}
static int search_point(Mob *m, int *wx, int *wy) {                 /* somewhere within six to ten steps of where you were lost */
    int16_t (*f)[MAPN] = mind_field(clampi((int)m->kx, 0, MAPN - 1), clampi((int)m->ky, 0, MAPN - 1));
    int here = cell(m->cx, m->cy)->region;
    for (int pass = 0; pass < 2; pass++)
        for (int tries = 0; tries < 60; tries++) {
            int x = clampi((int)m->kx + rnd(23) - 11, 1, MAPN - 2), y = clampi((int)m->ky + rnd(23) - 11, 1, MAPN - 2);
            int h = f[y][x];
            if (h < 0 || cell(x, y)->kind != K_FLOOR || occupied(x, y, m)) continue;
            if (pass == 0) {                                       /* first a room it lost you in before, if near */
                int rg = cell(x, y)->region, known = 0;
                for (int i = 0; i < G.lost_n; i++) known |= G.lost_rg[i] == rg;
                if (!known || rg == here || h > 14) continue;
            } else if (h < 3 || h > 10) continue;
            *wx = x; *wy = y; return 1;
        }
    return 0;
}

/* The Lightless (section 22): where light is near you, within sixteen cells of it. 2 your own (the Lantern of the Dead lit,
 * a cast's flare): it knows where you are; 1 a wisp or a fire on the floor, within ten cells of you: somewhere to go */
static int light_feel(const Mob *m, float *lx, float *ly) {
    if (G.dead || G.wwalk_t > 0.0f || G.shop) return 0;
    float d = hypotf(G.px - m->x, G.py - m->y), best = 16.0f;
    if (d < 16.0f && (lantern_lit() || G.flare_t > 0.0f)) { *lx = G.px; *ly = G.py; return 2; }
    int k = 0;
    const Wisp *w = &G.wisps[0];
    if (w->on && w->state != WS_GONE && hypotf(G.px - w->x, G.py - w->y) < 10.0f && (d = hypotf(w->x - m->x, w->y - m->y)) < best) { best = d; *lx = w->x; *ly = w->y; k = 1; }
    for (int i = 0; i < MAXBURNS; i++) {
        const Burn *b = &G.burns[i];
        if (!b->alive || hypotf(G.px - b->x, G.py - b->y) > 10.0f || (d = hypotf(b->x - m->x, b->y - m->y)) >= best) continue;
        best = d; *lx = b->x; *ly = b->y; k = 1;
    }
    return k;
}
/* each frame, for a creature that thinks this way: what it knows of the caster, by its sight */
static void mind_tick(Mob *m, float dt, int sees) {
    m->aw_t += dt;
    if (m->edgy > 0.0f) m->edgy -= dt;
    if (m->wait > 0.0f) m->wait -= dt;
    if (!m->role) role_give(m);                                    /* its post, its round, or its roaming (step 2) */
    const Pack *cp = pack_of(m);
    if (cp && cp->chill_t > 0.0f && !m->hunter) { m->alert = 0.0f; if (m->aware != MA_CALM) { m->aware = MA_CALM; m->wait = 0.0f; } return; }   /* the
                                                                      Wraith's wail: back to its post */
    if (m->type == MOB_SEXTON) {                                   /* he never hunts: he rings, and walks on round (step 9) */
        if (sees && G.level_t > 1.5) { m->seen_t = (float)G.level_t; m->kx = G.px; m->ky = G.py; }
        m->alert = 0.0f; m->aware = MA_CALM;
        return;
    }
    if (m->type == MOB_LIGHTLESS && !sees) {                       /* it feels light: your lantern or a cast's flare within sixteen */
        float lx, ly;                                              /* cells is as good as sight; a wisp or a fire, a place to go */
        int k = light_feel(m, &lx, &ly);
        if (k == 2) { if (m->aware != MA_HUNT) DBG_COUNT(lantern_felt); sees = 1; }
        else if (k == 1 && m->alert <= 0.0f && (m->aware != MA_TRACK || hypotf(m->kx - lx, m->ky - ly) > 1.5f)) {
            if (m->aware != MA_TRACK) DBG_COUNT(light_felt);
            mind_track(m, lx, ly); m->wait = 0.0f;
        }
    }
    if (m->fetch) { fetch_tick(m, dt); return; }                   /* a broken pack's hound, gone for help */
    hunter_tick(m);                                                /* a hunter come up on you by the trail */
    if (m->bait_st >= B_WAIT && m->bait_st <= B_LURE && bait_tick(m, dt, sees)) return;   /* a bait leads, it doesn't hunt */
    if (sees && G.level_t > 1.5) {
        if (m->pack) pack_call(m);                                 /* what one sees, all of it will know (step 3) */
        m->scav = 0;
        if (m->aware != MA_HUNT) DBG_COUNT(noticed);
        m->aware = MA_HUNT; m->aw_t = 0.0f;
        m->alert = fmaxf(m->alert, MIND_LIVE);
        m->kx = G.px; m->ky = G.py;
        m->kh = G.pvx * G.pvx + G.pvy * G.pvy > 0.01f ? atan2f(G.pvy, G.pvx) : G.ang;
        m->lost = 0.0f;
        return;
    }
    m->lost += dt;
    if (m->alert > 0.0f) {                                         /* the live field: it knows where you are a moment yet */
        m->alert -= dt;
        m->aware = MA_HUNT; m->kx = G.px; m->ky = G.py;
        if (G.pvx * G.pvx + G.pvy * G.pvy > 0.01f) m->kh = atan2f(G.pvy, G.pvx);
        if (m->alert <= 0.0f) {                                    /* lost: to where you were, a few cells on the way you went */
            float x = m->kx, y = m->ky;
            for (int k = 0; k < 3; k++) {
                float nx = x + cosf(m->kh), ny = y + sinf(m->kh);
                if (blocks_move((int)nx, (int)ny)) break;
                x = nx; y = ny;
            }
            mind_track(m, x, y);
            DBG_COUNT(lost_you);
        }
        return;
    }
    if (m->aware == MA_HUNT) mind_track(m, m->kx, m->ky);           /* told it knew (a blow, a call) and the time ran out */
    if (m->aware == MA_WARY && m->aw_t > 8.0f) { m->aware = MA_CALM; m->edgy = fmaxf(m->edgy, 10.0f); }
    if (m->aware == MA_TRACK && m->aw_t > 25.0f) { m->aware = MA_SEARCH; m->aw_t = 0.0f; m->wn = 0; }   /* it got no nearer */
    if (m->aware == MA_SEARCH && m->aw_t > 15.0f + 2.0f * (float)imin(m->rank, RK_V) + 5.0f * (float)((m - G.mobs) % 3) / 2.0f) {
        m->aware = MA_CALM; m->edgy = 30.0f; m->wn = 0;
        DBG_COUNT(gave_up);
    }
}
/* where it means to go, if anywhere: 1 and the cell, or 0 to wander as before */
static int mind_goal(Mob *m, int *gx, int *gy) {
    if (m->fetch) { const Mob *o = &G.mobs[m->fetch - 1]; *gx = o->cx; *gy = o->cy; return 1; }
    if (m->bait_st == B_LURE && m->amb) { *gx = G.amb[m->amb - 1].fx; *gy = G.amb[m->amb - 1].fy; return 1; }   /* to the kill room */
    if (m->bait_st == B_SHOW) return 0;                            /* it waits to be seen */
    if (m->scav) { *gx = (int)m->scx; *gy = (int)m->scy; return 1; }   /* to where something died */
    if (m->hunter == 2) {                                          /* a hunter: on the trail it follows the scent (update_mob); */
        if (on_trail(m)) return 0;                                 /* off it, to where it picks it up */
        *gx = G.hunt_nx; *gy = G.hunt_ny; return 1;
    }
    switch (m->aware) {
    case MA_WARY:
        if (m->aw_t < 0.8f) return 0;                              /* it stops and listens first */
        *gx = (int)m->kx; *gy = (int)m->ky; return 1;
    case MA_TRACK:
        *gx = (int)m->kx; *gy = (int)m->ky; return 1;
    case MA_SEARCH:
        if (m->wait > 0.0f) return 0;
        if (m->wn == 0) {
            if (!search_point(m, &m->wx, &m->wy)) { m->wait = 1.5f; return 0; }
            m->wn = 1;
        }
        *gx = m->wx; *gy = m->wy; return 1;
    default:                                                       /* calm: its post, its round, its roaming */
        if (m->wait > 0.0f) return 0;
        if (m->role == RL_POST) { if (m->cx == m->hx && m->cy == m->hy) return 0; *gx = m->hx; *gy = m->hy; return 1; }
        if (m->role == RL_PATROL && m->route >= 0 && m->route < G.nroute) {
            int n = G.route[m->route].n, w = m->wp % n;
            *gx = G.route[m->route].x[w]; *gy = G.route[m->route].y[w]; return 1;
        }
        if (m->role == RL_ROAM) { *gx = m->wx; *gy = m->wy; return 1; }
        if (m->role == RL_FOLLOW) {                                /* a pack on its round: each a step or two behind the one before */
            const Mob *l = follow_of(m);
            if (!l) { m->role = RL_PATROL; m->route = route_make(m->cx, m->cy); m->wp = 0; return 0; }
            if (hypotf(l->x - m->x, l->y - m->y) < 2.2f) return 0;
            *gx = l->cx; *gy = l->cy; return 1;
        }
        return 0;
    }
}
static void scav_feed(Mob *m) {                                    /* the scavenger at its meal: it mends, and is open to a blow */
    m->scav = 0; m->sub = 4; m->sub_t = 5.0f + 3.0f * rndf();
    dream_glance(m->x, m->y);                                      /* its chewing, heard */
    m->face = atan2f(m->scy - m->y, m->scx - m->x) + (rndf() - 0.5f);   /* bent over it */
    if (hypotf(m->x - G.px, m->y - G.py) < 12.0f) way_met(WY_SCAV);
    DBG_COUNT(feeds);
}
static void mind_arrived(Mob *m) {                                 /* at the place it meant to go */
    if (m->scav) { scav_feed(m); return; }
    if (m->hunter == 2) { if (!on_trail(m)) hunter_lost(m); return; }   /* the trail gone cold where it should begin */
    if (m->aware == MA_WARY) { m->look = 0.0f; return; }          /* it looks about, and settles if nothing comes */
    if (m->aware == MA_TRACK) {
        lost_note(cell((int)m->kx, (int)m->ky)->region);
        m->aware = MA_SEARCH; m->aw_t = 0.0f; m->wn = 0; m->wait = 1.0f;
        if (m->type == MOB_LIGHTLESS) { m->wait = 5.0f + 2.0f * rndf(); DBG_COUNT(lightless_lost); }   /* in the dark, lost: it stops
                                                                      and turns its head, and listens */
        DBG_COUNT(searches);
        return;
    }
    if (m->aware == MA_SEARCH) { m->wn = 0; m->wait = 1.0f + rndf(); return; }   /* a look round, and on */
    if (m->role == RL_PATROL) { m->wp++; m->wait = 1.5f + 1.5f * rndf(); }   /* a pause at each waypoint */
    if (m->type == MOB_SEXTON && m->role == RL_PATROL)            /* his bell at every turn; hard again, if he saw you lately */
        sexton_ring(m, m->seen_t > 0.0f && (float)G.level_t - m->seen_t < 25.0f && m->ring_cd <= 0.0f);
    else if (m->role == RL_ROAM) { roam_next(m); m->wait = 1.0f + 2.0f * rndf(); }
}
/* the head: towards you while it sees you, towards a sound while wary, about it while searching, idly while calm */
static void mind_face(Mob *m, float dt, int sees, int moving, float mdx, float mdy) {
    if (sees) { m->face = turn_to(m->face, atan2f(G.py - m->y, G.px - m->x), 6.0f * dt); m->look *= 1.0f - fminf(1.0f, dt * 4.0f); return; }
    if (moving) { m->face = turn_to(m->face, atan2f(mdy, mdx), 5.0f * dt); m->look *= 1.0f - fminf(1.0f, dt * 3.0f); return; }
    if (m->aware == MA_WARY) { m->face = turn_to(m->face, atan2f(m->ky - m->y, m->kx - m->x), 3.0f * dt); m->look *= 1.0f - fminf(1.0f, dt * 3.0f); return; }
    if (m->aware == MA_CALM && m->role == RL_POST && m->cx == m->hx && m->cy == m->hy) m->face = turn_to(m->face, m->pf, 2.0f * dt);   /* at its post */
    float sweep = m->aware == MA_SEARCH ? 1.1f : 0.8f, rate = m->aware == MA_SEARCH ? 1.4f : 0.45f;
    m->look = sweep * sinf(m->t * rate + (float)(m - G.mobs));    /* turning its head */
}

/* once a frame: a spell landing is heard where it lands (a blast new since the last frame); in the debug build, a pool
 * of effects or creatures that fills is told, for sizing them (docs/PLAN-ai.md, 27) */
static void mind_frame(void) {
    static uint8_t was[MAXBOOMS]; static float wx[MAXBOOMS], wy[MAXBOOMS];
    for (int i = 0; i < MAXBOOMS; i++) {
        const Boom *b = &G.booms[i];
        int fresh = b->alive && (!was[i] || wx[i] != b->x || wy[i] != b->y);
        was[i] = (uint8_t)b->alive; wx[i] = b->x; wy[i] = b->y;
        if (fresh && !G.shop) noise(b->x, b->y, 6, NZ_LAND, -1);
    }
#ifdef CRAWL_DEBUG
    #define POOL(name, arr, max) { int n = 0; for (int i = 0; i < (max); i++) n += G.arr[i].alive != 0; \
        static int full; if (n >= (max) && !full) fprintf(stderr, "EVENT pool_full %s %d t=%.2f\n", name, n, G.t); full = n >= (max); }
    POOL("mobs", mobs, MAXMOBS) POOL("shots", shots, MAXSHOTS) POOL("booms", booms, MAXBOOMS) POOL("bones", bones, MAXBONES)
    POOL("burns", burns, MAXBURNS) POOL("venoms", venoms, MAXVENOM) POOL("trails", trails, MAXTRAILS) POOL("motes", motes, MAXMOTES)
    #undef POOL
#endif
}

/* A footstep: heard three steps off down the halls, five sprinting or in water (S3), by anything there that isn't
 * already after you. Quiet boots soften it (80% Quiet: only the next cell hears). */
static void footstep(void) {
    int wet = on_water((int)G.px, (int)G.py);
    snd_sfx_ex(wet ? SFX_SPLASH : SFX_STEP, 1.0f, ((int)(G.bobph / PI_F) & 1) ? 0.44f : 0.56f);
    float loud = mind_on() && (wet || G.spr_on) ? 5.0f : 3.0f;
    noise(G.px, G.py, (int)(loud * (1.0f - gear_total(&RUN.gear, ST_QUIET) / 100.0f) + 0.5f), NZ_STEP, -1);
}

/* how fast a creature's clock runs: frozen stops it, frost slows it, Time Slip slows everything near */
static float mob_time(Mob *m, float dt) {
    if (m->frozen > 0.0f) { m->frozen -= dt; return 0.0f; }
    if (m->pin > 0.0f) { m->pin -= dt; return 0.0f; }              /* Impale: pinned where it stands */
    float k = 1.0f;
    if (m->slowed > 0.0f) { m->slowed -= dt; k *= 0.5f; }
    if (G.slip_t > 0.0f && hypotf(m->x - G.px, m->y - G.py) < 7.0f) k *= 0.3f;
    return dt * k;
}

