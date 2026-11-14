/* A part of scene_crawl.c (docs/PLAN-tidy.md): it is #included there in its place and never compiled on its own.
 * The mice (docs/PLAN-mice.md): their holes, their days, scurrying, the holes as ways through, the tell; the charnel rats told. */

/* Not creatures: a list of their own (G.mice), outside MAXMOBS, the hunt, the budget, the tally, the map and the autopilot's aim. They
 * draw on their own dice (G.mrs, seeded from the level, never G.rs), and before level 15 nothing they do reaches the simulation:
 * the dream's log stays the build before's, line for line, their own EVENT lines aside. */

static int tu_trig(int cx, int cy);
static int mrnd(int n) { return (int)(rng_next(&G.mrs) % (uint32_t)(n > 0 ? n : 1)); }
static float mrndf(void) { return rng_f(&G.mrs); }
static int mice_off(void) {
#ifdef CRAWL_DEBUG
    if (getenv("CRAWL_MICE") && atoi(getenv("CRAWL_MICE")) == 0) return 1;
#endif
    return 0;
}
static float mo_walk_pace(void) { return 0.9f + 0.12f * (float)G.mstage; }
static float mo_run_pace(void) { return 3.0f + 0.25f * (float)G.mstage; }
static float mo_pitch(void) { return 1.0f - 0.09f * (float)G.mstage; }   /* their chirps fall as they grow */
static void mo_sfx(int id, float x, float y, float gain) {         /* as sfx_at, at their size's pitch */
    float dx = x - G.px, dy = y - G.py, d = sqrtf(dx * dx + dy * dy);
    float lat = dx * -sinf(G.ang) + dy * cosf(G.ang), g = gain / (1.0f + 0.08f * d * d);
    if (g > 0.03f) snd_sfx_pitch(id, g, 0.5f + 0.4f * clampf(lat / (d + 0.5f), -1.0f, 1.0f), mo_pitch());
}

/* where a mouse may go: the floor, a door standing open (they open none); all doors, for the holes' own distances */
static int mo_pass(int x, int y, int doors) {
    if (x <= 0 || y <= 0 || x >= MAPN - 1 || y >= MAPN - 1) return 0;
    const Cell *c = &G.map[y][x];
    if (c->kind == K_FLOOR) return 1;
    if (c->kind != K_DOOR) return 0;
    return doors || G.doors[c->door].open > 0.85f;
}
static int16_t mo_dist[MAPN][MAPN], mo_par[MAPN][MAPN];
static void mo_bfs(int sx, int sy, int doors) {                    /* steps from (sx, sy) to every cell a mouse may reach; -1 none */
    static int16_t q[MAPN * MAPN];
    memset(mo_dist, 0xFF, sizeof mo_dist);
    if (!mo_pass(sx, sy, 1)) return;
    int qh = 0, qt = 0;
    mo_dist[sy][sx] = 0; mo_par[sy][sx] = -1; q[qt++] = (int16_t)(sy * MAPN + sx);
    while (qh < qt) {
        int c = q[qh++], x = c % MAPN, y = c / MAPN;
        for (int d = 0; d < 4; d++) {
            int nx = x + DX[d], ny = y + DY[d];
            if (mo_dist[ny][nx] >= 0 || !mo_pass(nx, ny, doors)) continue;
            mo_dist[ny][nx] = (int16_t)(mo_dist[y][x] + 1); mo_par[ny][nx] = (int16_t)c; q[qt++] = (int16_t)(ny * MAPN + nx);
        }
    }
}
static int mo_cell_ok(int x, int y) {                               /* a cell to wander to or begin in */
    if (!mo_pass(x, y, 0) || G.map[y][x].kind != K_FLOOR) return 0;
    if (g_drawn && tu_trig(x, y)) return 0;                        /* kept off the lessons' triggers (M4) */
    return !in_trial(x, y);
}
/* its way to (tx, ty) as things stand: 1 if it has one */
static int mo_path(Mouse *o, int tx, int ty) {
    int sx = (int)o->x, sy = (int)o->y;
    mo_bfs(sx, sy, 0);
    if (mo_dist[ty][tx] < 0) return 0;
    int n = mo_dist[ty][tx], c = ty * MAPN + tx;
    while (n > MPATH) { c = mo_par[c / MAPN][c % MAPN]; n--; }   /* too far: the first part of it, and again from there */
    o->npath = n; o->ip = 0;
    for (int i = n - 1; i >= 0; i--) { o->path[i] = (int16_t)c; c = mo_par[c / MAPN][c % MAPN]; }
    return 1;
}
static void mo_go(Mouse *o, int tx, int ty, int st, float pace) {
    if (!mo_path(o, tx, ty)) { o->st = MO_IDLE; o->wait = 0.5f + mrndf(); o->t = 0.0f; return; }
    o->st = st; o->pace = pace; o->t = 0.0f;
    o->side = (mrndf() - 0.5f) * 0.5f;                             /* off the middle, as they run along a wall */
}
static int mo_nearest_hole(const Mouse *o) {                       /* by foot as things stand, -1 none */
    mo_bfs((int)o->x, (int)o->y, 0);
    int best = -1, bd = 1 << 20;
    for (int h = 0; h < G.nmholes; h++) {
        int d = mo_dist[G.mholes[h].fy][G.mholes[h].fx];
        if (d >= 0 && d < bd) { bd = d; best = h; }
    }
    return best;
}
static int mo_out_now(const Mouse *o) { return o->alive && o->st != MO_WALL && o->st != MO_PEEK && o->st != MO_DEAD; }
static int mo_calm(const Mouse *o) { return o->alive && (o->st == MO_IDLE || o->st == MO_WALK || o->st == MO_PREEN) && !o->msg && o->cower <= 0.0f; }
static int mo_carriers(void) { int n = 0; for (int i = 0; i < G.nmice; i++) n += G.mice[i].alive && G.mice[i].msg; return n; }

/* running for its nearest hole, to stay in a while */
static void mo_flee(Mouse *o, float hide) {
    o->msg = 0; o->tgt = -1; o->rat = 0; o->role = 0; o->light = -1;
    o->hide = hide;
    int h = mo_nearest_hole(o);
    if (h < 0) { o->st = MO_IDLE; o->wait = 2.0f; o->cower = fminf(hide, 15.0f); return; }   /* nowhere to go: it freezes where it is */
    o->hole = h;
    mo_go(o, G.mholes[h].fx, G.mholes[h].fy, MO_FLEE, mo_run_pace());
}

/* a calm rat to tell (from level 15): 1 + its index, 0 none */
static int mo_rat_calm(const Mob *m) {
    return m->alive && m->type == MOB_RAT && m->state == MS_WALK && m->alert <= 0.0f && m->told_t <= 0.0f
        && m->aware != MA_HUNT && m->aware != MA_TRACK;
}
/* whom to tell, and how: the nearest calm one by its quickest way, on foot or through two holes (a tenth of the walk between
 * them). A rat first, from level 15; else a calm mouse. Returns 1 if there is one. */
static int16_t mo_hf[MAXMHOLES][MAPN][MAPN];
static int mo_plan(Mouse *o) {
    int self = (int)(o - G.mice);
    mo_bfs((int)o->x, (int)o->y, 0);
    static int16_t d0[MAPN][MAPN];
    memcpy(d0, mo_dist, sizeof d0);
    for (int h = 0; h < G.nmholes; h++) { mo_bfs(G.mholes[h].fx, G.mholes[h].fy, 0); memcpy(mo_hf[h], mo_dist, sizeof mo_dist); }
    float best = 1e9f; int bt = -1, br = 0, b1 = -1, b2 = -1;
    for (int pass = 0; pass < 2; pass++) {                         /* pass 0 the rats, 1 the mice */
        if (pass == 0 && G.level < MOUSE_DEBUT) continue;
        if (pass == 1 && bt >= 0) break;                           /* a rat to tell comes first */
        int n = pass ? G.nmice : MAXMOBS;
        for (int i = 0; i < n; i++) {
            int tx, ty;
            if (pass == 0) { const Mob *m = &G.mobs[i]; if (!mo_rat_calm(m)) continue; tx = m->cx; ty = m->cy; }
            else {                                                 /* a mouse away from where you were seen: those near it know */
                const Mouse *t = &G.mice[i];
                if (i == self || !mo_calm(t) || t->heard == o->word || hypotf(t->x - o->ax, t->y - o->ay) < 4.5f) continue;
                tx = (int)t->x; ty = (int)t->y;
            }
            if (tx <= 0 || ty <= 0 || tx >= MAPN - 1 || ty >= MAPN - 1) continue;
            float c = d0[ty][tx] >= 0 ? (float)d0[ty][tx] : 1e9f; int h1 = -1, h2 = -1;
            for (int a = 0; a < G.nmholes; a++) {
                int da = d0[G.mholes[a].fy][G.mholes[a].fx];
                if (da < 0) continue;
                for (int b = 0; b < G.nmholes; b++) {
                    if (b == a || mo_hf[b][ty][tx] < 0) continue;
                    float cb = (float)da + 0.1f * (float)G.mh_dist[a][b] + (float)mo_hf[b][ty][tx] + 1.5f;   /* and the moment at each end */
                    if (cb < c) { c = cb; h1 = a; h2 = b; }
                }
            }
            if (c < best) { best = c; bt = i; br = pass == 0; b1 = h1; b2 = h2; }
        }
    }
    if (bt < 0) return 0;
    o->rat = br ? 1 + bt : 0; o->tgt = br ? -1 : bt; o->h1 = b1; o->h2 = b2; o->plan_t = 1.5f;
    if (b1 >= 0) { o->hole = b1; mo_go(o, G.mholes[b1].fx, G.mholes[b1].fy, MO_RUN, mo_run_pace()); }
    else if (br) mo_go(o, G.mobs[bt].cx, G.mobs[bt].cy, MO_RUN, mo_run_pace());
    else mo_go(o, (int)G.mice[bt].x, (int)G.mice[bt].y, MO_RUN, mo_run_pace());
    if (o->st != MO_RUN) return 0;
    return 1;
}
/* it carries word of you: where you were seen, to the next one */
static int mo_carry(Mouse *o, float ax, float ay, int word) {    /* word 0: a new alarm */
    if (mo_carriers() >= 2) return 0;                              /* two at once at most */
    o->word = o->heard = word ? word : ++G.malarm;
    o->msg = 1; o->msg_t = 0.0f; o->ax = ax; o->ay = ay; o->role = 0; o->light = -1;
    if (!mo_plan(o)) { o->msg = 0; return 0; }
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT mouse_tell %d at %.1f,%.1f to %s %d via %d,%d t=%.2f\n", (int)(o - G.mice), o->x, o->y, o->rat ? "rat" : "mouse",
            o->rat ? o->rat - 1 : o->tgt, o->h1, o->h2, G.t);
#endif
    mo_sfx(SFX_SQUEAK, o->x, o->y, 0.8f);
    return 1;
}
/* scurried: the first of them to know carries word, if any calm one is out to tell; the rest run */
static void mo_scare(Mouse *o) {
    if (!mo_calm(o)) return;
    if (G.mboss) { mo_flee(o, 1e9f); return; }
    int known = 0;                                                 /* word of this already going: it only runs */
    for (int i = 0; i < G.nmice; i++) { const Mouse *c = &G.mice[i]; known |= c->alive && c->msg && hypotf(c->ax - G.px, c->ay - G.py) < 6.0f; }
    if (known || !mo_carry(o, G.px, G.py, 0)) { mo_flee(o, 18.0f + 20.0f * mrndf()); mo_sfx(SFX_SQUEAK, o->x, o->y, 0.5f); }
}

/* into its hole: out of another, a tenth of the walk later, and after a while in there if it is hiding */
static void mo_into(Mouse *o) {
    int h = o->hole, out = -1;
    if (o->msg && o->h2 >= 0) out = o->h2;
    else if (G.nmholes > 1) {                                      /* running: the hole furthest from you; else any other */
        float bd = -1.0f;
        for (int k = 0; k < G.nmholes; k++) {
            if (k == h) continue;
            float d = o->hide > 0.0f ? hypotf(G.mholes[k].mx - G.px, G.mholes[k].my - G.py) + 4.0f * mrndf() : mrndf();
            if (d > bd) { bd = d; out = k; }
        }
    }
    if (out < 0) out = h;
    o->out = out; o->st = MO_WALL; o->t = 0.0f;
    o->wait = 0.1f * (float)(h == out ? 0 : G.mh_dist[h][out]) / fmaxf(0.5f, o->pace) + (o->msg ? 0.0f : o->hide);
    o->x = G.mholes[h].mx; o->y = G.mholes[h].my;
}
static void mo_emerge(Mouse *o) {
    const MHole *hh = &G.mholes[o->out];
    o->x = hh->fx + 0.5f - DX[hh->d] * 0.2f; o->y = hh->fy + 0.5f - DY[hh->d] * 0.2f;   /* just out of the hole */
    o->face = atan2f((float)DY[hh->d], (float)DX[hh->d]);
    o->hide = 0.0f; o->hole = -1;
    if (o->msg) {                                                  /* on to whom it carries word to */
        o->h1 = o->h2 = -1;
        if (o->rat) { const Mob *m = &G.mobs[o->rat - 1]; if (mo_rat_calm(m)) { mo_go(o, m->cx, m->cy, MO_RUN, mo_run_pace()); o->plan_t = 1.5f; return; } }
        else if (o->tgt >= 0 && mo_calm(&G.mice[o->tgt])) { mo_go(o, (int)G.mice[o->tgt].x, (int)G.mice[o->tgt].y, MO_RUN, mo_run_pace()); o->plan_t = 1.5f; return; }
        if (!mo_plan(o)) mo_flee(o, 15.0f + 15.0f * mrndf());
        return;
    }
    o->st = MO_IDLE; o->wait = 1.0f + 2.0f * mrndf(); o->t = 0.0f;
}

/* along its path; 1 when there */
static int mo_step(Mouse *o, float dt) {
    if (o->ip >= o->npath) return 1;
    int c = o->path[o->ip], cx = c % MAPN, cy = c / MAPN;
    if (!mo_pass(cx, cy, 0)) { o->npath = o->ip; return 1; }       /* a door shut in its way */
    float gx = cx + 0.5f, gy = cy + 0.5f, dx = gx - o->x, dy = gy - o->y, d = sqrtf(dx * dx + dy * dy);
    if (o->ip + 1 < o->npath) {                                    /* off the middle of the way, along its side */
        float ux = d > 1e-3f ? dx / d : 0.0f, uy = d > 1e-3f ? dy / d : 0.0f;
        gx += -uy * o->side; gy += ux * o->side;
        dx = gx - o->x; dy = gy - o->y; d = sqrtf(dx * dx + dy * dy);
    }
    float s = o->pace * dt;
    if (d <= s || d < 0.05f) { o->x = gx; o->y = gy; o->ip++; return o->ip >= o->npath; }
    o->x += dx / d * s; o->y += dy / d * s;
    o->face = atan2f(dy, dx);
    return 0;
}

/* a warm light near enough to go and sit by: its index in G.mlit, -1 none */
static int mo_find_light(const Mouse *o) {
    if (!G.nmlit) return -1;
    mo_bfs((int)o->x, (int)o->y, 0);
    int pick = -1, seen = 0;
    for (int i = 0; i < G.nmlit; i++) {
        int d = mo_dist[G.mlit[i][1]][G.mlit[i][0]];
        if (d >= 0 && d <= 12 && mrnd(++seen) == 0) pick = i;
    }
    return pick;
}
static void mo_wander(Mouse *o) {                                  /* a few cells off, at random */
    mo_bfs((int)o->x, (int)o->y, 0);
    int px = -1, py = -1, seen = 0, cx = (int)o->x, cy = (int)o->y;
    for (int y = cy - 7; y <= cy + 7; y++)
        for (int x = cx - 7; x <= cx + 7; x++) {
            if (x <= 0 || y <= 0 || x >= MAPN - 1 || y >= MAPN - 1) continue;
            int d = mo_dist[y][x];
            if (d >= 2 && d <= 7 && mo_cell_ok(x, y) && mrnd(++seen) == 0) { px = x; py = y; }
        }
    if (px < 0) { o->wait = 1.0f + mrndf(); return; }
    mo_go(o, px, py, MO_WALK, mo_walk_pace());
}

/* it tells: they sit up nose to nose and chirp; then the one told carries it on */
static void mo_tell_begin(Mouse *o, Mouse *t) {
    o->st = t->st = MO_TELL; o->t = t->t = 0.0f; o->role = 1; t->role = 2;
    t->msg = 0; t->ax = o->ax; t->ay = o->ay; t->light = -1; t->word = t->heard = o->word;
    o->face = atan2f(t->y - o->y, t->x - o->x); t->face = o->face + PI_F;
    mo_sfx(SFX_MTELL, o->x, o->y, 0.9f);
}
static void mo_tell_rat(Mouse *o, Mob *m) {                         /* from level 15: the rat rears, squeals, and goes where you were seen */
    m->told_t = 1.0f;
    if (mind_on() && minded(m)) mind_track(m, o->ax, o->ay); else m->alert = fmaxf(m->alert, 8.0f);
    sfx_at(SFX_RAT_SQUEAL, m->x, m->y, 1.0f);
    mo_sfx(SFX_MTELL, o->x, o->y, 0.9f);
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT mouse_told_rat %d rat %d at %.1f,%.1f t=%.2f\n", (int)(o - G.mice), (int)(m - G.mobs), m->x, m->y, G.t);
#endif
    mo_flee(o, 15.0f + 15.0f * mrndf());
}

/* a messenger slain (from level 15): one hit, and the word dies with it; not a kill */
static void mo_slay(Mouse *o) {
    o->st = MO_DEAD; o->t = 0.0f; o->msg = 0;
    mo_sfx(SFX_SQUEAK, o->x, o->y, 1.0f);
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT mouse_slain %d at %.1f,%.1f t=%.2f\n", (int)(o - G.mice), o->x, o->y, G.t);
#endif
}
static int mo_open(const Mouse *o) { return G.level >= MOUSE_DEBUT && o->alive && o->msg && (o->st == MO_RUN || o->st == MO_TELL); }
static void mice_shot(float x, float y) {                         /* a spell going by (update_shots) */
    for (int i = 0; i < G.nmice; i++) { Mouse *o = &G.mice[i]; if (mo_open(o) && hypotf(o->x - x, o->y - y) < 0.35f) mo_slay(o); }
}
static void mice_blast(float x, float y, float r) {                /* a blast (explode): it scatters them; a messenger in it dies */
    for (int i = 0; i < G.nmice; i++) {
        Mouse *o = &G.mice[i];
        if (!mo_out_now(o)) continue;
        float d = hypotf(o->x - x, o->y - y);
        if (mo_open(o) && d < r + 0.4f) mo_slay(o);
        else if (d < r + 2.0f) mo_scare(o);
    }
}
static void mice_hear(float x, float y, int steps, int what) {    /* a noise the dead would hear (noise) */
    if (what == NZ_STEP) return;                                   /* footsteps: they see you instead */
    for (int i = 0; i < G.nmice; i++) {
        Mouse *o = &G.mice[i];
        if (mo_calm(o) && hypotf(o->x - x, o->y - y) <= (float)steps * 0.6f) mo_scare(o);
    }
}

/* ---- the level's: holes, warm lights, the mice placed ---- */
static int mo_warm(const Light *L) {                               /* fire, not crystal, a geode or a steady glow */
    if (L->phase < -100.0f) return 0;
    for (int i = 0; i < G.ndecos; i++) {
        int s = G.decos[i].spr;
        if ((s == SP_CRYSTAL_A || s == SP_CRYSTAL_B) && fabsf(G.decos[i].x - L->x) < 0.9f && fabsf(G.decos[i].y - L->y) < 0.9f) return 0;
    }
    if (crawl_set(G.level) == 3 && L->power < 0.6f) return 0;      /* the Deep's geodes glow cold */
    return 1;
}
static void mice_level(void) {
    G.nmice = G.nmholes = G.nmlit = 0; G.mboss = 0;
    memset(G.mice, 0, sizeof G.mice);
    G.mstage = crawl_mouse_stage(G.level);
#ifdef CRAWL_DEBUG
    if (getenv("CRAWL_MICESTAGE")) G.mstage = clampi(atoi(getenv("CRAWL_MICESTAGE")), 0, MOUSE_STAGES - 1);
#endif
    G.mrs = (G.rs * 2654435761u) ^ (0x6D1CE5u + (uint32_t)G.level * 7919u);   /* their own dice: G.rs read, never drawn on */
    G.mrs |= 1u;
    if (mice_off() || G.shop || PK.depth || !G.faced) return;
    int step = crawl_step(G.level), boss = G.boss >= 0;
    int want_h = g_drawn ? 3 : boss ? 4 : 4 + step / 2, want_m = g_drawn ? 3 : boss ? 4 : 3 + step + (step >= 6);
    /* the holes: on a plain stretch of the run's own wall facing a floor cell, at least six steps apart, spread far as they go */
    static int16_t cand[MAPN * MAPN][3];
    int nc = 0;
    for (int y = 1; y < MAPN - 1; y++)
        for (int x = 1; x < MAPN - 1; x++) {
            if (!mo_cell_ok(x, y)) continue;
            for (int d = 0; d < 4; d++) {
                int wx = x - DX[d], wy = y - DY[d];
                const Cell *w = &G.map[wy][wx];
                if (w->kind != K_WALL || w->secret || w->por || w->fover[d] || w->ftorch == d + 1) continue;
                if (w->face[d] < T_MAT || w->face[d] >= T_MAT + NMAT * NVAR) continue;
                if (nc < MAPN * MAPN) { cand[nc][0] = (int16_t)x; cand[nc][1] = (int16_t)y; cand[nc][2] = (int16_t)d; nc++; }
            }
        }
    static int16_t near[MAPN][MAPN];
    for (int y = 0; y < MAPN; y++) for (int x = 0; x < MAPN; x++) near[y][x] = 0x7FFF;
    for (int k = 0; k < want_h && nc > 0; k++) {
        int pick = -1, bd = -1;
        if (k == 0) pick = mrnd(nc);
        else for (int i = 0; i < nc; i++) { int d = near[cand[i][1]][cand[i][0]]; if (d >= 6 && d < 0x7FFF && d + mrnd(3) > bd) { bd = d + mrnd(3); pick = i; } }
        if (pick < 0) break;
        MHole *hh = &G.mholes[G.nmholes++];
        hh->fx = cand[pick][0]; hh->fy = cand[pick][1]; hh->d = cand[pick][2]; hh->wx = hh->fx - DX[hh->d]; hh->wy = hh->fy - DY[hh->d];
        hh->mx = hh->wx + 0.5f + DX[hh->d] * 0.5f; hh->my = hh->wy + 0.5f + DY[hh->d] * 0.5f;
        Cell *w = &G.map[hh->wy][hh->wx];
        w->face[hh->d] = (uint8_t)(T_MHOLE + (w->face[hh->d] - T_MAT));   /* the same stone, a hole at its foot */
        mo_bfs(hh->fx, hh->fy, 1);
        for (int y = 0; y < MAPN; y++) for (int x = 0; x < MAPN; x++) if (mo_dist[y][x] >= 0 && mo_dist[y][x] < near[y][x]) near[y][x] = mo_dist[y][x];
    }
    if (!G.nmholes) return;                                        /* no wall to live in: none here */
    for (int a = 0; a < G.nmholes; a++) {
        mo_bfs(G.mholes[a].fx, G.mholes[a].fy, 1);
        for (int b = 0; b < G.nmholes; b++) { int d = mo_dist[G.mholes[b].fy][G.mholes[b].fx]; G.mh_dist[a][b] = (uint8_t)(d < 0 || d > 255 ? 255 : d); }
    }
    for (int i = 0; i < G.nlights && G.nmlit < MAXMLIT; i++) {     /* the warm lights, and a cell beside each to sit in */
        const Light *L = &G.lights[i];
        if (!mo_warm(L)) continue;
        int lx = (int)L->x, ly = (int)L->y, fx = -1, fy = -1;
        if (mo_cell_ok(lx, ly)) { fx = lx; fy = ly; }
        else for (int d = 0; d < 4 && fx < 0; d++) if (mo_cell_ok(lx + DX[d], ly + DY[d])) { fx = lx + DX[d]; fy = ly + DY[d]; }
        if (fx < 0) continue;
        G.mlit[G.nmlit][0] = (int16_t)fx; G.mlit[G.nmlit][1] = (int16_t)fy; G.nmlit++;
    }
    /* the mice: anywhere a mouse can be, away from where you begin; a third sat by a warm light */
    mo_bfs(G.cx, G.cy, 1);
    static int16_t from_start[MAPN][MAPN];
    memcpy(from_start, mo_dist, sizeof from_start);
    for (int k = 0; k < want_m && G.nmice < MAXMICE; k++) {
        int px = -1, py = -1, seen = 0, lit = -1;
        if (k % 3 == 1 && G.nmlit) {
            lit = mrnd(G.nmlit);
            if (from_start[G.mlit[lit][1]][G.mlit[lit][0]] >= 7) { px = G.mlit[lit][0]; py = G.mlit[lit][1]; } else lit = -1;
        }
        if (px < 0)
            for (int y = 1; y < MAPN - 1; y++)
                for (int x = 1; x < MAPN - 1; x++)
                    if (from_start[y][x] >= 7 && mo_cell_ok(x, y) && mrnd(++seen) == 0) { px = x; py = y; }
        if (px < 0) break;
        Mouse *o = &G.mice[G.nmice++];
        o->alive = 1; o->hole = o->out = o->tgt = o->h1 = o->h2 = -1; o->light = lit;
        o->x = px + 0.2f + 0.6f * mrndf(); o->y = py + 0.2f + 0.6f * mrndf(); o->face = mrndf() * TAU_F;
        o->st = lit >= 0 ? MO_PREEN : MO_IDLE; o->wait = lit >= 0 ? 6.0f + 18.0f * mrndf() : 3.0f * mrndf();
        o->sing_t = 4.0f + 20.0f * mrndf();
    }
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT mice level %d stage %d mice %d holes %d lights %d\n", G.level, G.mstage, G.nmice, G.nmholes, G.nmlit);
#endif
}

#ifdef CRAWL_DEBUG
static void human_begin(void);
static void mice_tp(void) {                                        /* CRAWL_TPMICE: three of them preening by the nearest warm light, you three */
    if (atoi(getenv("CRAWL_TPMICE")) == 2 && G.nmholes && G.nmice) {   /* steps off facing it, once you take over; 2: two cells */
        const MHole *mh = &G.mholes[0];                            /* before a hole, a mouse about to look out of it */
        int x = mh->fx + DX[mh->d], y = mh->fy + DY[mh->d];
        if (G.map[y][x].kind != K_FLOOR) { x = mh->fx; y = mh->fy; }
        G.cx = x; G.cy = y; G.px = x + 0.5f; G.py = y + 0.5f; G.ang = atan2f(mh->my - G.py, mh->mx - G.px);
        Mouse *o = &G.mice[0];
        o->st = MO_WALL; o->out = 0; o->hole = 0; o->t = 0.0f; o->wait = 10.0f; o->msg = 0; o->x = mh->mx; o->y = mh->my;
        human_begin();
        return;
    }
    if (!G.nmlit || G.nmice < 1) return;
    mo_bfs(G.cx, G.cy, 0);
    int li = -1, bd = 1 << 20;
    for (int i = 0; i < G.nmlit; i++) { int d = mo_dist[G.mlit[i][1]][G.mlit[i][0]]; if (d > 0 && d < bd) { bd = d; li = i; } }
    if (li < 0) return;
    int lx = G.mlit[li][0], ly = G.mlit[li][1];
    for (int k = 0; k < 3 && k < G.nmice; k++) {
        Mouse *o = &G.mice[k];
        o->x = lx + 0.3f + 0.2f * (float)k; o->y = ly + 0.35f + 0.15f * (float)(k & 1); o->st = MO_PREEN; o->light = li; o->wait = 60.0f;
        o->face = TAU_F * 0.3f * (float)k; o->msg = 0; o->hole = -1;
    }
    mo_bfs(lx, ly, 0);
    for (int y = 1; y < MAPN - 1; y++)
        for (int x = 1; x < MAPN - 1; x++)
            if (mo_dist[y][x] == 3 && G.map[y][x].kind == K_FLOOR && line_clear(x + 0.5f, y + 0.5f, lx + 0.5f, ly + 0.5f)) {
                G.cx = x; G.cy = y; G.px = x + 0.5f; G.py = y + 0.5f;
                G.ang = atan2f(ly + 0.5f - G.py, lx + 0.5f - G.px);
                human_begin();
                return;
            }
}
#endif

/* ---- each frame ---- */
static void mice_update(float dt) {
    for (int i = 0; i < MAXMOBS; i++) if (G.mobs[i].told_t > 0.0f) G.mobs[i].told_t -= dt;   /* a told rat's rearing */
    if (!G.nmice || G.shop || PK.depth) return;
    if (G.boss >= 0 && !G.mboss && (G.bstate == BS_FIGHT || G.bstate == BS_WAVE || G.bstate == BS_RISE)) {   /* the fight: in, for good */
        G.mboss = 1;
        for (int i = 0; i < G.nmice; i++) {
            Mouse *o = &G.mice[i];
            if (!o->alive) continue;
            if (o->st == MO_WALL || o->st == MO_PEEK) { o->st = MO_WALL; o->wait = 1e9f; } else mo_flee(o, 1e9f);
        }
#ifdef CRAWL_DEBUG
        fprintf(stderr, "EVENT mice_hide boss t=%.2f\n", G.t);
#endif
    }
    float cd = 0.0f;
    for (int i = 0; i < G.nmice; i++) {
        Mouse *o = &G.mice[i];
        if (!o->alive) continue;
        o->t += dt;
        if (o->msg) o->msg_t += dt;
        if (o->cower > 0.0f) o->cower -= dt;
        if (mo_calm(o)) {                                          /* you near, or a fight */
            float d = hypotf(o->x - G.px, o->y - G.py);
            if (!G.dead && (d < 2.5f || (d < 4.0f && G.spd > 0.4f && line_clear(o->x, o->y, G.px, G.py)))) { mo_scare(o); continue; }
            cd = 0.0f;
            for (int k = 0; k < MAXMOBS && cd == 0.0f; k++) {
                const Mob *m = &G.mobs[k];
                if (m->alive && m->state == MS_WALK && m->alert > 0.0f && fabsf(m->x - o->x) < 3.0f && fabsf(m->y - o->y) < 3.0f) cd = 1.0f;
            }
            if (cd > 0.0f) { mo_scare(o); continue; }
            if ((o->sing_t -= dt) <= 0.0f) {                       /* a chirp now and then, heard close */
                o->sing_t = 8.0f + 22.0f * mrndf();
                if (d < 9.0f) mo_sfx(SFX_SQUEAK, o->x, o->y, 0.35f);
            }
        }
        switch (o->st) {
        case MO_IDLE:
            if ((o->wait -= dt) > 0.0f) {
                if (mrnd(90) == 0) o->face += (mrndf() - 0.5f) * 2.0f;   /* sniffing about */
                break;
            }
            {
                float r = mrndf();
                int li = r < 0.35f ? mo_find_light(o) : -1;
                if (li >= 0) { o->light = li; mo_go(o, G.mlit[li][0], G.mlit[li][1], MO_WALK, mo_walk_pace()); }
                else if (r > 0.88f) { int h = mo_nearest_hole(o); o->hide = 0.0f; if (h >= 0) { o->hole = h; mo_go(o, G.mholes[h].fx, G.mholes[h].fy, MO_FLEE, mo_walk_pace() * 1.5f); } else o->wait = 1.0f; }
                else mo_wander(o);
            }
            break;
        case MO_WALK:
            if (mo_step(o, dt)) {
                if (o->light >= 0) { o->st = MO_PREEN; o->wait = 8.0f + 17.0f * mrndf(); o->t = 0.0f; }
                else { o->st = MO_IDLE; o->wait = 0.5f + 2.5f * mrndf(); }
            }
            break;
        case MO_PREEN:
            if (o->light >= 0) {                                   /* turned to the warmth */
                float lx = G.mlit[o->light][0] + 0.5f, ly = G.mlit[o->light][1] + 0.5f;
                if (hypotf(lx - o->x, ly - o->y) > 0.2f) o->face = atan2f(ly - o->y, lx - o->x) + 0.6f * sinf(o->t * 0.3f + (float)i);
            }
            if ((o->wait -= dt) <= 0.0f) { o->st = MO_IDLE; o->light = -1; o->wait = 0.5f + mrndf(); }
            break;
        case MO_FLEE:
            if (mo_step(o, dt)) {
                const MHole *hh = o->hole >= 0 ? &G.mholes[o->hole] : NULL;
                if (hh && (int)o->x == hh->fx && (int)o->y == hh->fy) mo_into(o);
                else { int h = mo_nearest_hole(o); if (h >= 0) { o->hole = h; mo_go(o, G.mholes[h].fx, G.mholes[h].fy, MO_FLEE, o->pace); } else { o->st = MO_IDLE; o->wait = 2.0f; } }
            }
            break;
        case MO_WALL:
            if (o->t >= o->wait) {
                if (G.mboss) { o->wait = 1e9f; break; }
                o->st = MO_PEEK; o->t = 0.0f;                      /* its eyes at the hole, a moment */
                o->x = G.mholes[o->out].mx; o->y = G.mholes[o->out].my;
            }
            break;
        case MO_PEEK:
            if (o->t >= 0.6f) {
                const MHole *hh = &G.mholes[o->out];
                if (!o->msg && hypotf(hh->fx + 0.5f - G.px, hh->fy + 0.5f - G.py) < 3.0f) { o->st = MO_WALL; o->t = 0.0f; o->wait = 4.0f + 6.0f * mrndf(); break; }   /* you there: not yet */
                mo_emerge(o);
            }
            break;
        case MO_RUN:
            if (o->msg_t > 25.0f) { mo_flee(o, 15.0f); break; }     /* it gives it up */
            if (o->rat) {
                Mob *m = &G.mobs[o->rat - 1];
                if (!mo_rat_calm(m)) { if (!mo_plan(o)) mo_flee(o, 15.0f); break; }
                if (hypotf(m->x - o->x, m->y - o->y) < 1.0f) { mo_tell_rat(o, m); break; }
            } else if (o->tgt >= 0) {
                Mouse *t = &G.mice[o->tgt];
                if (!mo_calm(t)) { if (!mo_plan(o)) mo_flee(o, 15.0f); break; }
                if (hypotf(t->x - o->x, t->y - o->y) < 0.7f) { mo_tell_begin(o, t); break; }
            }
            if (mo_step(o, dt)) {
                if (o->h1 >= 0 && o->hole == o->h1 && (int)o->x == G.mholes[o->h1].fx && (int)o->y == G.mholes[o->h1].fy) { mo_into(o); break; }
                if (!mo_plan(o)) mo_flee(o, 15.0f);                /* there, and it has moved on: again */
            } else if ((o->plan_t -= dt) <= 0.0f && o->h1 < 0) {  /* on foot after one that moves: its way again now and then */
                o->plan_t = 1.5f;
                if (o->rat) mo_go(o, G.mobs[o->rat - 1].cx, G.mobs[o->rat - 1].cy, MO_RUN, o->pace);
                else if (o->tgt >= 0) mo_go(o, (int)G.mice[o->tgt].x, (int)G.mice[o->tgt].y, MO_RUN, o->pace);
            }
            break;
        case MO_TELL:
            if (o->t >= 1.3f) {
                int told = o->role == 2;
                o->role = 0;
                if (told) { if (!mo_carry(o, o->ax, o->ay, o->word)) mo_flee(o, 20.0f + 20.0f * mrndf()); }   /* and on it goes */
                else mo_flee(o, 20.0f + 20.0f * mrndf());
            }
            break;
        case MO_DEAD:
            if (o->t >= 1.5f) o->alive = 0;
            break;
        }
    }
}
