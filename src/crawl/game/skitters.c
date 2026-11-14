/* A part of scene_crawl.c (docs/PLAN-tidy.md): it is #included there in its place and never compiled on its own.
 * The wall spiders (docs/PLAN-mice.md, section 8): set dressing that crawls the walls and runs to tell a web or hunting spider of you. */

/* Not creatures, as the mice are not: their own list (G.skit) on their own dice (G.srs), outside MAXMOBS and the autopilot's aim. A web
 * spider told only shows it (its eyes stop blinking, its web thrums) unless it is a nest's, which it calls; a hunting spider told comes
 * to where you were seen. Before level 11 neither can come, so nothing they do reaches the simulation there. */

static int srnd(int n) { return (int)(rng_next(&G.srs) % (uint32_t)(n > 0 ? n : 1)); }
static float srndf(void) { return rng_f(&G.srs); }
#define SK_SCALE 0.14f                                             /* two thirds of a spiderling's */

static int sk_side(int cx, int cy, int pref) {                    /* a wall beside the cell to cling to, the one it is on if it can */
    if (pref >= 0 && G.map[cy + DY[pref]][cx + DX[pref]].kind == K_WALL) return pref;
    int pick = -1, seen = 0;
    for (int d = 0; d < 4; d++) if (G.map[cy + DY[d]][cx + DX[d]].kind == K_WALL && srnd(++seen) == 0) pick = d;
    return pick;
}
static int sk_path(Skitter *o, int tx, int ty) {
    mo_bfs((int)o->x, (int)o->y, 0);
    if (mo_dist[ty][tx] < 0) return 0;
    int n = mo_dist[ty][tx], c = ty * MAPN + tx;
    while (n > MPATH) { c = mo_par[c / MAPN][c % MAPN]; n--; }
    o->npath = n; o->ip = 0; o->stall = 0.0f; o->stall_ip = 0;
    for (int i = n - 1; i >= 0; i--) { o->path[i] = (int16_t)c; c = mo_par[c / MAPN][c % MAPN]; }
    return 1;
}
static void sk_go(Skitter *o, int tx, int ty, int st, float pace) {
    if (!sk_path(o, tx, ty)) { o->st = SK_IDLE; o->wait = 1.0f + srndf(); return; }
    o->st = st; o->pace = pace; o->t = 0.0f;
}
static int sk_step(Skitter *o, float dt) {                        /* cell to cell along its way, along the walls as it goes; 1 there */
    if (o->ip >= o->npath) return 1;
    if (o->ip != o->stall_ip) { o->stall_ip = o->ip; o->stall = 0.0f; }
    int c = o->path[o->ip], cx = c % MAPN, cy = c / MAPN;
    if (!mo_pass(cx, cy, 0)) { o->npath = o->ip; return 1; }
    float gx = cx + 0.5f, gy = cy + 0.5f, dx = gx - o->x, dy = gy - o->y, d = sqrtf(dx * dx + dy * dy), s = o->pace * dt;
    if (d <= s || (o->stall += dt) > 2.0f) {
        o->x = gx; o->y = gy; o->ip++;
        o->side = sk_side(cx, cy, o->side);
        if (o->side >= 0 && srnd(3) == 0) o->zt = 0.15f + 0.6f * srndf();   /* up the stone, or down it */
        return o->ip >= o->npath;
    }
    o->x += dx / d * s; o->y += dy / d * s; o->face = atan2f(dy, dx);
    return 0;
}
static int sk_calm(const Skitter *o) { return o->alive && (o->st == SK_IDLE || o->st == SK_CRAWL); }
static int sk_spider_calm(const Mob *m) {                         /* a spider not yet told, and not about you already */
    if (!m->alive || m->state != MS_WALK || m->boss || m->add || m->ally || m->told_t > 0.0f) return 0;
    if (m->type == MOB_SPIDER) return m->sub == 0;                 /* in its web */
    return m->type == MOB_HUNTER && m->alert <= 0.0f && m->aware != MA_HUNT && m->aware != MA_TRACK;
}
static int sk_cell(const Mob *m, int *x, int *y) {                /* where a spider is reached from: a web spider's corner, a hunter's cell */
    *x = m->type == MOB_SPIDER ? (int)m->ax : m->cx; *y = m->type == MOB_SPIDER ? (int)m->ay : m->cy;
    return mo_pass(*x, *y, 0);
}
static int sk_find_spider(const Skitter *o) {                     /* the nearest calm spider by its way there: 1 + it, 0 none */
    mo_bfs((int)o->x, (int)o->y, 0);
    int best = 0, bd = 1 << 20;
    for (int i = 0; i < MAXMOBS; i++) {
        const Mob *m = &G.mobs[i];
        int x, y;
        if (!sk_spider_calm(m) || !sk_cell(m, &x, &y)) continue;
        int d = mo_dist[y][x];
        if (d >= 0 && d < bd) { bd = d; best = 1 + i; }
    }
    return best;
}
static int sk_find_web(const Skitter *o, int reach) {             /* a corner web within reach of its way: 1 + its deco, 0 none */
    mo_bfs((int)o->x, (int)o->y, 0);
    int best = 0, bd = reach + 1;
    for (int i = 0; i < G.ndecos; i++) {
        const Deco *dc = &G.decos[i];
        if (dc->spr != SP_COBWEB) continue;
        int d = mo_dist[(int)dc->y][(int)dc->x];
        if (d >= 0 && d < bd) { bd = d; best = 1 + i; }
    }
    return best;
}
static void sk_to_web(Skitter *o, int web) {                       /* up into a corner web, a while */
    const Deco *dc = &G.decos[web - 1];
    o->st = SK_WEB; o->web = web; o->spider = 0; o->seek = 0; o->t = 0.0f;
    o->x = dc->x; o->y = dc->y; o->ox = o->oy = 0.0f; o->zt = 0.62f; o->side = -1;
    o->wait = G.mboss ? 1e9f : 30.0f + 30.0f * srndf();
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT skitter_web %d at %.1f,%.1f t=%.2f\n", (int)(o - G.skit), o->x, o->y, G.t);
#endif
}
/* scurried: to tell the nearest spider; with none, off to find a web; it carries word only to a spider */
static void sk_scare(Skitter *o) {
    if (!sk_calm(o)) return;
    o->ax = G.px; o->ay = G.py;
    int sp = G.mboss ? 0 : sk_find_spider(o), x, y;
    if (sp && sk_cell(&G.mobs[sp - 1], &x, &y)) {
        o->spider = sp; o->web = 0; o->seek = 0; o->seek_t = 0.0f;   /* seek_t: how long it has run, here */
        sk_go(o, x, y, SK_RUN, 2.6f);
        if (o->st == SK_RUN) {
            sfx_at(SFX_SKITTER, o->x, o->y, 0.35f);
#ifdef CRAWL_DEBUG
            fprintf(stderr, "EVENT skitter_run %d at %.1f,%.1f to spider %d t=%.2f\n", (int)(o - G.skit), o->x, o->y, sp - 1, G.t);
#endif
            return;
        }
    }
    o->spider = 0;
    int w = sk_find_web(o, 40);
    if (w) { o->web = w; sk_go(o, (int)G.decos[w - 1].x, (int)G.decos[w - 1].y, SK_RUN, 2.2f); return; }
    o->seek = 1; o->web = 0; o->seek_t = 0.0f;                     /* none known: it wanders, looking for one */
    o->st = SK_IDLE; o->wait = 0.0f;
}
static void sk_tell(Skitter *o, Mob *m) {                          /* the word given: a hunter comes; a web spider watches, a nest's calls */
    if (m->type == MOB_HUNTER) {
        m->told_t = 1.0f;
        if (mind_on() && minded(m)) mind_track(m, o->ax, o->ay); else m->alert = fmaxf(m->alert, 8.0f);
        sfx_at(SFX_SKITTER, m->x, m->y, 0.9f);
    } else {
        m->told_t = 30.0f;
        sfx_at(SFX_THRUM, m->ax, m->ay, 0.8f);
        if (m->pack) pack_call(m);
    }
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT skitter_told %d %s %d t=%.2f\n", (int)(o - G.skit), m->type == MOB_HUNTER ? "hunter" : "web", (int)(m - G.mobs), G.t);
#endif
    o->spider = 0;
    int w = sk_find_web(o, 12);
    if (w) { o->web = w; sk_go(o, (int)G.decos[w - 1].x, (int)G.decos[w - 1].y, SK_RUN, 2.2f); }
    else { o->st = SK_IDLE; o->wait = 3.0f; }
}
static void sk_slay(Skitter *o) {                                   /* one carrying word, hit: the word dies with it; not a kill */
    o->st = SK_DEAD; o->t = 0.0f; o->spider = 0;
    sfx_at(SFX_SKITTER, o->x, o->y, 0.6f);
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT skitter_slain %d t=%.2f\n", (int)(o - G.skit), G.t);
#endif
}
static int sk_open(const Skitter *o) { return o->alive && o->st == SK_RUN && o->spider; }
static void skit_shot(float x, float y) {
    for (int i = 0; i < G.nskit; i++) { Skitter *o = &G.skit[i]; if (sk_open(o) && hypotf(o->x + o->ox - x, o->y + o->oy - y) < 0.3f) sk_slay(o); }
}
static void skit_blast(float x, float y, float r) {
    for (int i = 0; i < G.nskit; i++) {
        Skitter *o = &G.skit[i];
        float d = hypotf(o->x - x, o->y - y);
        if (sk_open(o) && d < r + 0.4f) sk_slay(o); else if (d < r + 2.0f) sk_scare(o);
    }
}
static void skit_hear(float x, float y, int steps, int what) {
    if (what == NZ_STEP) return;
    for (int i = 0; i < G.nskit; i++) { Skitter *o = &G.skit[i]; if (sk_calm(o) && hypotf(o->x - x, o->y - y) <= (float)steps * 0.5f) sk_scare(o); }
}

static void sk_wander(Skitter *o, int reach) {                    /* a few cells off along the walls */
    mo_bfs((int)o->x, (int)o->y, 0);
    int px = -1, py = -1, seen = 0, cx = (int)o->x, cy = (int)o->y;
    for (int y = cy - reach; y <= cy + reach; y++)
        for (int x = cx - reach; x <= cx + reach; x++) {
            if (x <= 0 || y <= 0 || x >= MAPN - 1 || y >= MAPN - 1) continue;
            int d = mo_dist[y][x];
            if (d >= 2 && d <= reach && mo_cell_ok(x, y) && sk_side(x, y, -1) >= 0 && srnd(++seen) == 0) { px = x; py = y; }
        }
    if (px < 0) { o->st = SK_IDLE; o->wait = 2.0f + 2.0f * srndf(); return; }
    sk_go(o, px, py, SK_CRAWL, o->seek ? 2.0f : 0.6f + 0.3f * srndf());
}

static void skit_level(void) {
    G.nskit = 0; memset(G.skit, 0, sizeof G.skit);
    G.srs = ((G.rs ^ 0x5B1D3Au) * 2246822519u + (uint32_t)G.level * 104729u) | 1u;   /* their own dice: G.rs read, never drawn on */
#ifdef CRAWL_DEBUG
    if (getenv("CRAWL_SKIT") && atoi(getenv("CRAWL_SKIT")) == 0) return;
#endif
    if (mice_off() || G.shop || PK.depth || !G.faced) return;
    int step = crawl_step(G.level), want = g_drawn ? 2 : G.boss >= 0 ? 2 : 2 + step / 3 + 2 * (crawl_set(G.level) == 1);
    mo_bfs(G.cx, G.cy, 1);
    static int16_t from_start[MAPN][MAPN];
    memcpy(from_start, mo_dist, sizeof from_start);
    for (int k = 0; k < want && G.nskit < MAXSKIT; k++) {
        int px = -1, py = -1, seen = 0;
        for (int y = 1; y < MAPN - 1; y++)
            for (int x = 1; x < MAPN - 1; x++)
                if (from_start[y][x] >= 6 && mo_cell_ok(x, y) && sk_side(x, y, -1) >= 0 && srnd(++seen) == 0) { px = x; py = y; }
        if (px < 0) break;
        Skitter *o = &G.skit[G.nskit++];
        o->alive = 1; o->x = px + 0.5f; o->y = py + 0.5f; o->side = sk_side(px, py, -1);
        o->ox = DX[o->side] * 0.42f; o->oy = DY[o->side] * 0.42f; o->z = o->zt = 0.15f + 0.6f * srndf();
        o->st = SK_IDLE; o->wait = 4.0f * srndf();
    }
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT skitters level %d count %d\n", G.level, G.nskit);
#endif
}

#ifdef CRAWL_DEBUG
static void skit_tp(void) {                                        /* CRAWL_TPSKIT: two cells from the best lit on a wall, facing it */
    int order[MAXSKIT], n = 0;
    for (int i = 0; i < G.nskit; i++) if (G.skit[i].alive && G.skit[i].side >= 0) order[n++] = i;
    for (int a = 1; a < n; a++) { int v = order[a], b = a - 1; while (b >= 0 && lm_at(G.skit[order[b]].x, G.skit[order[b]].y) < lm_at(G.skit[v].x, G.skit[v].y)) { order[b + 1] = order[b]; b--; } order[b + 1] = v; }
    for (int j = 0; j < n; j++) {
        Skitter *o = &G.skit[order[j]];
        int cx = (int)o->x, cy = (int)o->y;
        for (int back = 2; back >= 1; back--) {
            int x = cx - DX[o->side] * back, y = cy - DY[o->side] * back;
            if (x <= 0 || y <= 0 || x >= MAPN - 1 || y >= MAPN - 1 || G.map[y][x].kind != K_FLOOR || !line_clear(x + 0.5f, y + 0.5f, o->x, o->y)) continue;
            G.cx = x; G.cy = y; G.px = x + 0.5f; G.py = y + 0.5f; G.ang = atan2f(o->y + o->oy - G.py, o->x + o->ox - G.px);
            o->st = SK_IDLE; o->wait = 20.0f;
            human_begin();
            return;
        }
    }
}
#endif

static void skit_update(float dt) {
    if (!G.nskit || G.shop || PK.depth) return;
    for (int i = 0; i < G.nskit; i++) {
        Skitter *o = &G.skit[i];
        if (!o->alive) continue;
        o->t += dt;
        float wx = o->side >= 0 && o->st != SK_WEB ? DX[o->side] * 0.42f : 0.0f, wy = o->side >= 0 && o->st != SK_WEB ? DY[o->side] * 0.42f : 0.0f;
        float zt = o->st == SK_WEB ? 0.62f : o->side >= 0 ? o->zt : 0.0f, k = fminf(1.0f, dt * 4.0f);
        o->ox += (wx - o->ox) * k; o->oy += (wy - o->oy) * k; o->z += (zt - o->z) * fminf(1.0f, dt * 2.5f);   /* onto the wall, up it, down */
        if (G.mboss && o->st != SK_WEB && o->st != SK_DEAD && !o->web) {   /* a boss's fight: into the nearest web, for good */
            int w = sk_find_web(o, 60);
            if (w) { o->web = w; o->spider = 0; sk_go(o, (int)G.decos[w - 1].x, (int)G.decos[w - 1].y, SK_RUN, 2.4f); }
        }
        if (sk_calm(o)) {                                          /* you near, or a fight */
            float d = hypotf(o->x - G.px, o->y - G.py);
            if (!G.dead && (d < 2.0f || (d < 3.5f && G.spd > 0.4f && line_clear(o->x + o->ox * 0.8f, o->y + o->oy * 0.8f, G.px, G.py)))) { sk_scare(o); continue; }
            int fight = 0;
            for (int k2 = 0; k2 < MAXMOBS && !fight; k2++) {
                const Mob *m = &G.mobs[k2];
                fight = m->alive && m->state == MS_WALK && m->alert > 0.0f && fabsf(m->x - o->x) < 3.0f && fabsf(m->y - o->y) < 3.0f;
            }
            if (fight) { sk_scare(o); continue; }
        }
        switch (o->st) {
        case SK_IDLE:
            if ((o->wait -= dt) > 0.0f) break;
            if (o->seek) {                                         /* looking for a web: one in sight of its way, or on */
                o->seek_t += dt;
                int w = sk_find_web(o, 6);
                if (w) { o->web = w; sk_go(o, (int)G.decos[w - 1].x, (int)G.decos[w - 1].y, SK_RUN, 2.2f); break; }
                if (o->seek_t > 25.0f) { o->seek = 0; o->wait = 3.0f; break; }   /* it settles where it is */
            }
            sk_wander(o, o->seek ? 6 : 5);
            break;
        case SK_CRAWL:
            if (o->seek) o->seek_t += dt;
            if (sk_step(o, dt)) { o->st = SK_IDLE; o->wait = o->seek ? 0.2f : 2.0f + 6.0f * srndf(); }
            break;
        case SK_RUN:
            if (o->spider) {
                Mob *m = &G.mobs[o->spider - 1];
                int x, y;
                if (!sk_spider_calm(m) || !sk_cell(m, &x, &y)) { o->spider = 0; o->st = SK_IDLE; o->wait = 0.0f; o->seek = 1; o->seek_t = 0.0f; break; }
                float sx = m->type == MOB_SPIDER ? m->ax : m->x, sy = m->type == MOB_SPIDER ? m->ay : m->y, sd = hypotf(sx - o->x, sy - o->y);
                if (sd < 0.8f) { sk_tell(o, m); break; }
                if ((o->seek_t += dt) > 20.0f) {                   /* it gives it up, on a clock of its own */
#ifdef CRAWL_DEBUG
                    fprintf(stderr, "EVENT skitter_gaveup %d t=%.2f\n", i, G.t);
#endif
                    o->spider = 0; o->st = SK_IDLE; o->wait = 2.0f; break;
                }
                if (o->ip >= o->npath && sd < 1.6f) {              /* its way done, the spider a step off: straight to it */
                    float s = fminf(sd, o->pace * dt);
                    o->x += (sx - o->x) / sd * s; o->y += (sy - o->y) / sd * s;
                } else if (sk_step(o, dt) || (m->type == MOB_HUNTER && (o->plan_t -= dt) <= 0.0f)) {   /* after a hunter as it goes, its */
                    o->plan_t = 1.5f;                              /* way again now and then; and when its way is done */
                    sk_go(o, x, y, SK_RUN, o->pace);
                }
            } else if (o->web) {
                if (sk_step(o, dt)) {
                    const Deco *dc = &G.decos[o->web - 1];
                    if ((int)o->x == (int)dc->x && (int)o->y == (int)dc->y) sk_to_web(o, o->web);   /* there: up into it */
                    else if ((o->plan_t -= dt) <= 0.0f) { o->plan_t = 0.5f; sk_go(o, (int)dc->x, (int)dc->y, SK_RUN, o->pace); }   /* not yet: on */
                }
            } else { o->st = SK_IDLE; o->wait = 1.0f; }
            break;
        case SK_WEB:
            if (o->t >= o->wait && !G.mboss) {                     /* down again, and about its business */
                const Deco *dc = &G.decos[o->web - 1];
                o->x = (int)dc->x + 0.5f; o->y = (int)dc->y + 0.5f; o->web = 0; o->seek = 0;
                o->side = sk_side((int)o->x, (int)o->y, -1); o->zt = 0.5f;
                o->st = SK_IDLE; o->wait = 2.0f;
            }
            break;
        case SK_DEAD:
            if (o->t >= 1.0f) o->alive = 0;
            break;
        }
    }
}
