/* A part of scene_crawl.c (docs/PLAN-tidy.md): it is #included there in its place and never compiled on its own.
 * The dream at a trial (tri_next, tri_goal); omens; a boss's hazards. */

/* the dream at a trial: in, when it has found the door and two thirds of its health; then what each asks */
static int tri_next(int *a) {
    *a = -1;
    switch (G.tri) {
    case TRI_VIGIL: {                                              /* a candle out, with none of the dead at it */
        int best = -1; float bd = 1e9f;
        for (int k = 0; k < 3; k++) {
            int i = G.tri_a[1 + k];
            const Thing *c = &G.thing[i];
            if (c->state) continue;
            float d = hypotf(c->x - G.px, c->y - G.py);
            if (i == G.tri_a[12] - 1) d -= 3.0f;                   /* the one it was going for */
            if (d < bd) { bd = d; best = i; }
        }
        if (best >= 0) { *a = best; G.tri_a[12] = best + 1; return PA_USE; }
        return PA_NONE;
    }
    case TRI_RITES: {
        if (G.carry) { *a = G.tri_a[1]; return PA_USE; }           /* to the bier */
        int best = -1; float bd = 1e9f;
        for (int i = G.tri_n0; i < G.nthing; i++) {
            const Thing *t = &G.thing[i];
            if (t->role != TR_ITEM || t->in >= 0) continue;
            float d = hypotf(t->x - G.px, t->y - G.py);
            if (i == G.tri_a[12] - 1) d -= 3.0f;                   /* the one it was going for */
            if (d < bd) { bd = d; best = i; }
        }
        if (best >= 0) { *a = best; G.tri_a[12] = best + 1; return PA_USE; }
        return PA_NONE;
    }
    case TRI_SLUICES: case TRI_CAVEIN: case TRI_FIREDAMP: case TRI_LAMPS: case TRI_DARK: {   /* the nearest not yet done (the roof's: the
                                                                      one it will reach first) */
        int best = -1; float bd = 1e9f;
        for (int i = G.tri_n0; i < G.nthing; i++) {
            const Thing *t = &G.thing[i];
            if (t->role == TR_READ || t->state) continue;
            float d = hypotf(t->x - G.px, t->y - G.py);
            if (G.tri == TRI_CAVEIN) d = (float)tri_strip_of(t->cx, t->cy) * 4.0f + d * 0.25f;
            if (i == G.tri_a[12] - 1) d -= 3.0f;                   /* the one it was going for */
            if (d < bd) { bd = d; best = i; }
        }
        if (best >= 0) { *a = best; G.tri_a[12] = best + 1; return PA_USE; }
        return PA_NONE;
    }
    case TRI_SINGING: {                                            /* the lit one, if there is time to reach it */
        if (G.tri_a[2] >= 0) { *a = G.tri_a[2]; return PA_USE; }
        return PA_NONE;
    }
    case TRI_TOLL: {                                               /* to a bell, and under the toll, ring it */
        int best = -1; float bd = 1e9f;
        for (int i = G.tri_n0; i < G.nthing; i++) {
            const Thing *t = &G.thing[i];
            if (t->role != TR_STRIKE || t->state) continue;
            float d = hypotf(t->x - G.px, t->y - G.py);
            if (i == G.tri_a[12] - 1) d -= 3.0f;                   /* the one it was going for */
            if (d < bd) { bd = d; best = i; }
        }
        if (best < 0) return PA_NONE;
        *a = best; G.tri_a[12] = best + 1;
        return PA_USE;                                             /* there, its hand on it till the toll (tri_touched) */
    }
    default: return PA_NONE;
    }
}
static int tri_goal(int x, int y, int *gx, int *gy) {
    if (G.player || G.tri < 0 || PK.depth || G.shop) return 0;
    if (G.tri_state == TRS_SHUT) {
        const Door *d = &G.doors[G.tri_door];
        if (G.hp * 3 < G.max_hp * 2 || G.leaving || G.tri_ix < 0) return 0;
        bfs_from(x, y);
        int dd = bfs_dist[G.tri_iy][G.tri_ix];                     /* found: its door seen, or its red lights a short walk off */
        if (dd < 0 || (!(G.mapped[d->y][d->x] & 4) && dd > 16)) return 0;
        *gx = G.tri_ix; *gy = G.tri_iy;
        return 1;
    }
    if (G.tri_state != TRS_ON || !in_trial(x, y)) return 0;
    int a, act = tri_next(&a);
    if (act == PA_STEP && a >= 0) { int sx = a % MAPN, sy = a / MAPN; if (sx == x && sy == y) return 0; *gx = sx; *gy = sy; G.use_kind = USE_NONE; return 1; }
    if (act == PA_USE && a >= 0 && !G.work_i) {
        const Thing *t = &G.thing[a];
        int sx, sy;
        if (use_spot(t->cx, t->cy, &sx, &sy)) {                    /* (it fights what comes in front of it on the way) */
            bfs_from(x, y);
            if (bfs_dist[sy][sx] >= 0) { G.use_x = t->cx; G.use_y = t->cy; G.use_kind = USE_THING; G.use_thing = a; *gx = sx; *gy = sy; return 1; }
        }
    }
    int best = -1, hunt = 0; float bd = 1e9f;                      /* the nearest of the trial's dead, or the room's middle; what
                                                                      keeps its distance (deacons, imps, the crowned one) first */
    for (int pass = 0; pass < 2 && best < 0; pass++)
        for (int i = 0; i < MAXMOBS; i++) {
            const Mob *m = &G.mobs[i];
            if (!m->alive || !m->tri || m->state != MS_WALK) continue;
            int far = m->type == MOB_DEACON || m->type == MOB_IMP;
            if (pass == 0 && !(G.tri == TRI_COURT ? m->tri == 3 : far)) continue;
            float d = hypotf(m->x - G.px, m->y - G.py);
            if (d < bd) { bd = d; best = i; hunt = pass == 0; }
        }
    const Room *R = &G.rooms[G.tri_room];
    float tx = best >= 0 ? G.mobs[best].x : R->x + R->w * 0.5f, ty = best >= 0 ? G.mobs[best].y : R->y + R->h * 0.5f;
    if (best >= 0 && bd < (hunt ? 2.0f : 3.5f)) return 0;           /* close enough: it fights from here */
    int bx = -1, by = -1, bs = 1 << 20;
    for (int yy = R->y; yy < R->y + R->h; yy++)
        for (int xx = R->x; xx < R->x + R->w; xx++) {
            if (!apass(xx, yy)) continue;
            int s = (int)(hypotf(xx + 0.5f - tx, yy + 0.5f - ty) * 4.0f);
            if (s < bs) { bs = s; bx = xx; by = yy; }
        }
    if (bx < 0 || (bx == x && by == y)) return 0;
    *gx = bx; *gy = by; G.use_kind = USE_NONE;
    return 1;
}
#ifdef CRAWL_DEBUG
static void tp_trial(void) {                                       /* testing: two steps from its door, outside, facing it */
    if (G.tri < 0 || G.tri_wx < 0) return;
    const Door *d = &G.doors[G.tri_door];
    int x = G.tri_wx + (G.tri_wx - d->x), y = G.tri_wy + (G.tri_wy - d->y);
    if (!hz_floor(x, y)) { x = G.tri_wx; y = G.tri_wy; }
    G.px = x + 0.5f; G.py = y + 0.5f; G.ang = atan2f(d->y + 0.5f - G.py, d->x + 0.5f - G.px); human_begin();
}
#endif


/* the omens: in the three levels before a boss, and in its own level's halls, its hazard comes by
 * itself near the dweller, at half the harm: a stone from the ceiling in the Crypts, bile from a drain in
 * the Catacombs, the floor splitting in the Hollows, the void in the Deep */
static float omen_every(void) { static const float E[4] = { 90.0f, 60.0f, 40.0f, 30.0f }; int s = crawl_step(G.level) - (RUN_LEN - 4); return s >= 0 ? E[s] : 0.0f; }
static void update_omens(float dt) {
    float every = omen_every();
#ifdef CRAWL_DEBUG
    if (getenv("CRAWL_OMEN")) { every = (float)atof(getenv("CRAWL_OMEN")); G.omen_t = fminf(G.omen_t, every); }   /* testing: omens on any level, this often */
#endif
    if (PK.depth || G.shop || G.tally || G.dead || G.exiting || G.level_t < 8.0 || every <= 0.0f || in_arena(G.px, G.py)) return;
    if ((G.omen_t -= dt) > 0.0f) return;
    G.omen_t = every * (0.75f + 0.5f * rndf());
    int x, y;
    switch (crawl_set(G.level)) {
    case 0: if (hz_near(1.0f, 5.0f, &x, &y)) hz_rock(x + 0.5f, y + 0.5f, 1); break;
    case 1:                                                        /* a drain in a wall near you, onto the lane */
        for (int tries = 0; tries < 40; tries++) {
            if (!hz_near(2.0f, 6.0f, &x, &y)) break;
            int d = rnd(4), wx = x + DX[d], wy = y + DY[d];
            Cell *w = &G.map[wy][wx];
            if (w->kind != K_WALL || w->secret || (w->tex >= T_TORCH0 && w->tex <= T_TORCH2) || w->tex == T_EXIT0 || wx <= 0 || wy <= 0 || wx >= MAPN - 1 || wy >= MAPN - 1) continue;
            int8_t cx[HZ_CELLS], cy[HZ_CELLS]; uint8_t cd[HZ_CELLS];
            int n = hz_patch(x, y, 2, cx, cy, cd, 12);
            Hazard *h = n ? hz_bile(cx, cy, cd, n, 1) : NULL;
            if (h) { h->wx = wx; h->wy = wy; h->wtex = w->tex; w->tex = T_GRATE; }
            break;
        }
        break;
    case 2:
        if (hz_near(1.0f, 4.0f, &x, &y)) { Hazard *h = hz_fire(1); float a = rndf() * TAU_F; if (h) { hz_line(h, x + 0.5f, y + 0.5f, x + 0.5f + cosf(a), y + 0.5f + sinf(a), 4 + rnd(3), 0); hz_fire_done(h, x + 0.5f, y + 0.5f); } }
        break;
    default: if (hz_near(1.5f, 5.0f, &x, &y)) hz_void(x + 0.5f, y + 0.5f, 1.2f, 1); break;
    }
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT omen set %d t=%.2f\n", crawl_set(G.level), G.t);
#endif
}

/* a boss's own hazard, over its fight: how often and how much grow with the challenge's steps */
static void boss_hazards(float dt) {
    const Mob *m = boss_mob();
    int step = G.bstep, wave = G.bstate == BS_WAVE || G.bstate == BS_RISE;
    float pace = 1.0f + 0.2f * (float)step;
    if (G.bcast_t > 0.0f) G.bcast_t -= dt;
    switch (G.boss) {
    case BOSS_KNIGHT:                                              /* Rockfall: three to five about you, one on your cell */
        if (G.bdash_t > 0.0f || G.bpose_t > 0.0f) break;            /* one thing at a time */
        if (m && !wave && (G.bz2_t -= dt * pace) <= 0.0f && hypotf(G.px - m->x, G.py - m->y) < 3.0f) {   /* the Sundering Arc, at you */
            G.bz2_t = 7.0f + 3.0f * rndf();
            hz_arc(m->x, m->y, atan2f(G.py - m->y, G.px - m->x), mob_damage(m) * 1.3f);
            G.bpose = BP_RAISE; G.bpose_t = hz_warn(0.8f);         /* the sword up while the arc shows, then the sweep */
        }
        if ((G.bz_t -= dt * pace * (wave ? 0.5f : 1.0f)) > 0.0f) break;
        G.bz_t = 10.0f + 4.0f * rndf();
        G.bcast_t = 0.5f;
        if (!wave) { G.bpose = BP_PLANT; G.bpose_t = 0.5f; }        /* it plants its sword with a shout */
        if (m) sfx_at(SFX_GROAN, m->x, m->y, 1.0f);
        hz_rock(G.px, G.py, 0);
        for (int i = 0, n = 2 + rnd(3) + step / 2; i < n; i++) {
            float a = rndf() * TAU_F, d = 1.5f + 2.5f * rndf();
            int x = (int)floorf(G.px + cosf(a) * d), y = (int)floorf(G.py + sinf(a) * d);
            if (in_arena((float)x, (float)y) && hz_floor(x, y)) hz_rock(x + 0.5f, y + 0.5f, 0);
        }
        break;
    case BOSS_SPIDER: {                                            /* the outlets: so many open at once, one changing every 20 s */
        if (m && !wave && (G.bz2_t -= dt * pace) <= 0.0f) { G.bz2_t = 14.0f + 4.0f * rndf(); egg_shot(m); }   /* Egg Shot */
        int want = imin(G.bnout, 2 + (step >= 2) + (step >= 4) + wave), open = 0;
        for (int o = 0; o < G.bnout; o++) {
            int hi = G.bout_haz[o];
            if (hi >= 0 && (!G.haz[hi].alive || G.haz[hi].src != o)) G.bout_haz[o] = hi = -1;
            open += hi >= 0 && hz_phase(&G.haz[hi]) < 2;
        }
        int change = (G.bz_t -= dt * pace) <= 0.0f;
        if (change) G.bz_t = 20.0f;
        if (change || open > want) {                               /* shut one: it drains back */
            int c[MAXOUT], nc = 0;
            for (int o = 0; o < G.bnout; o++) if (G.bout_haz[o] >= 0 && hz_phase(&G.haz[G.bout_haz[o]]) < 2) c[nc++] = o;
            if (nc) { Hazard *h = &G.haz[G.bout_haz[c[rnd(nc)]]]; h->on = fmaxf(0.0f, h->t - h->warn); if (h->t < h->warn) h->warn = h->t; open--; }
        }
        for (int tries = 0; open < want && tries < 8; tries++) {   /* and another opens */
            int o = rnd(G.bnout);
            if (G.bout_haz[o] >= 0) continue;
            Hazard *h = hz_bile(G.bout_cx[o], G.bout_cy[o], G.bout_cd[o], G.bout_n[o], 0);
            if (h) { h->src = o; G.bout_haz[o] = (int)(h - G.haz); open++; }
        }
        break;
    }
    case BOSS_DEACON:                                              /* Hellrent: a line at you, a ring round him, a cross */
        if (!wave && m && m->swing <= 0.0f && G.bpose_t <= 0.0f && (G.bz2_t -= dt * pace) <= 0.0f) {   /* Litany */
            G.bz2_t = 18.0f + 6.0f * rndf();
            G.bpose = BP_LITANY; G.bpose_t = 2.0f;
            sfx_at(SFX_WAIL, m->x, m->y, 0.9f);
        }
        if (wave || !m || (G.bz_t -= dt * pace) > 0.0f) break;
        G.bz_t = 9.0f + 3.0f * rndf();
        G.bcast_t = 0.6f;
        for (int k = 0; k < 1 + (step >= 2); k++) {
            Hazard *h = hz_fire(0);
            if (!h) break;
            int shape = rnd(3);
            if (shape == 0) hz_line(h, m->x, m->y, G.px, G.py, 10, 0);
            else if (shape == 1) for (int a = 0; a < 8; a++) hz_line(h, m->x + cosf(a * TAU_F / 8.0f) * 2.0f, m->y + sinf(a * TAU_F / 8.0f) * 2.0f, m->x + cosf(a * TAU_F / 8.0f) * 3.0f, m->y + sinf(a * TAU_F / 8.0f) * 3.0f, 0, a & 1);
            else { hz_line(h, G.px - 4.0f, G.py, G.px + 4.0f, G.py, 8, 0); hz_line(h, G.px, G.py - 4.0f, G.px, G.py + 4.0f, 8, 0); }
            hz_fire_done(h, m->x, m->y);
        }
        break;
    case BOSS_WRAITH:                                              /* Void Energies: one on you, the rest across your ways out */
        if ((G.bz2_t -= dt * pace) <= 0.0f) {                      /* Grasping Dark, wherever no fire is near */
            G.bz2_t = 6.0f + 3.0f * rndf();
            int lit = 0;
            for (int l = 0; l < G.nlights; l++) lit |= hypotf(G.lights[l].x - G.px, G.lights[l].y - G.py) < 3.5f;
            if (!lit) hz_grasp(G.px, G.py);
        }
        if ((G.bz_t -= dt * pace) > 0.0f) break;
        G.bz_t = 8.0f + 4.0f * rndf();
        G.bcast_t = 0.6f;
        hz_void(G.px, G.py, 1.4f, 0);
        for (int i = 0, n = rnd(3) + (step >= 3) + (step >= 5); i < n; i++) {
            float a = rndf() * TAU_F, d = 3.0f + 2.0f * rndf();
            int x = (int)floorf(G.px + cosf(a) * d), y = (int)floorf(G.py + sinf(a) * d);
            if (in_arena((float)x, (float)y) && hz_floor(x, y)) hz_void(x + 0.5f, y + 0.5f, 1.2f + 0.5f * rndf(), 0);
        }
        break;
    }
}

