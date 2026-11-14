/* A part of scene_crawl.c (docs/PLAN-tidy.md): it is #included there in its place and never compiled on its own.
 * The wall spiders (docs/PLAN-mice.md, section 8): set dressing that crawls the walls and runs to tell a web or hunting spider of you. */

/* Not creatures, as the mice are not: their own list (G.skit) on their own dice (G.srs), outside MAXMOBS and the autopilot's aim. A web
 * spider told only shows it (its eyes stop blinking, its web thrums) unless it is a nest's, which it calls; a hunting spider told comes
 * to where you were seen. Before level 11 neither can come, so nothing they do reaches the simulation there. */

static int srnd(int n) { return (int)(rng_next(&G.srs) % (uint32_t)(n > 0 ? n : 1)); }
static float srndf(void) { return rng_f(&G.srs); }
#define SK_SCALE 0.14f                                             /* slain and falling: two thirds of a spiderling's */
#define SK_SIZE 0.17f                                              /* on the stone, its back to you: legs and all */
static int sk_tame;                                                /* CRAWL_TPSKIT=2, 3: the first minds nothing (looked at close) */

/* The stone's ways (section 8.1). Every face where open floor meets a wall (K_WALL) is traced with the wall on the left into closed
 * loops: one round each room's and corridor's edge, out through every doorway along its jambs, and one round each block standing
 * free. A face is the open cell (x, y) and the side d its wall is on; it runs along t = (d + 1) & 3, and its joints with the faces
 * before and after it go straight on (WJ_ON), into a corner (WJ_IN) or round the wall's edge (WJ_OUT). A spider on it is WF_IN off
 * the stone, so its way stops that short of an inner corner and goes that far past an outer one: lo, hi, how far along the face
 * (0 to 1 its cell's width) it runs. The way is then unbroken all round, and nothing on it is ever off the stone. */
enum { WJ_ON, WJ_IN, WJ_OUT };
#define WF_IN 0.02f
#define MAXWF (MAPN * MAPN * 4)
typedef struct { int8_t x, y, d, jp, jn; int16_t prev, next, loop; float s0, lo, hi; } WFace;   /* s0: how far round its loop it begins */
static WFace WF[MAXWF];
static float wf_len[MAXWF];                                        /* each loop's length */
static int16_t wf_at[MAPN][MAPN][4];                               /* the face of (x, y) toward d, -1 none */
static int nwf;
static uint32_t wf_hash;                                           /* the walls they were traced from */

static int wf_solid(int x, int y) { return x < 0 || y < 0 || x >= MAPN || y >= MAPN || G.map[y][x].kind == K_WALL; }
static uint32_t wf_layout(void) {
    uint32_t h = 2166136261u;
    for (int y = 0; y < MAPN; y++)
        for (int x = 0; x < MAPN; x++) h = (h ^ (uint32_t)(G.map[y][x].kind == K_WALL)) * 16777619u;
    return h;
}
static int wf_after(int x, int y, int d, int *nx, int *ny, int *nd) {   /* the face after (x, y, d), and how it joins */
    int t = (d + 1) & 3;
    if (wf_solid(x + DX[t], y + DY[t])) { *nx = x; *ny = y; *nd = t; return WJ_IN; }
    int x1 = x + DX[t], y1 = y + DY[t];
    if (wf_solid(x1 + DX[d], y1 + DY[d])) { *nx = x1; *ny = y1; *nd = d; return WJ_ON; }
    *nx = x1 + DX[d]; *ny = y1 + DY[d]; *nd = (t + 2) & 3;        /* round the wall's edge, onto its other side */
    return WJ_OUT;
}
static void wf_build(void) {
    nwf = 0;
    memset(wf_at, 0xFF, sizeof wf_at);
    for (int y = 0; y < MAPN; y++)
        for (int x = 0; x < MAPN; x++) {
            if (wf_solid(x, y)) continue;
            for (int d = 0; d < 4; d++)
                if (wf_solid(x + DX[d], y + DY[d]) && nwf < MAXWF) {
                    WF[nwf] = (WFace){ (int8_t)x, (int8_t)y, (int8_t)d, 0, 0, -1, -1, -1, 0.0f, 0.0f, 1.0f };
                    wf_at[y][x][d] = (int16_t)nwf++;
                }
        }
    for (int f = 0; f < nwf; f++) {
        int nx, ny, nd, j = wf_after(WF[f].x, WF[f].y, WF[f].d, &nx, &ny, &nd), g = wf_at[ny][nx][nd];
        WF[f].next = (int16_t)g; WF[f].jn = (int8_t)j;
        if (g >= 0) { WF[g].prev = (int16_t)f; WF[g].jp = (int8_t)j; }
    }
    for (int f = 0; f < nwf; f++) {
        WFace *w = &WF[f];
        w->lo = w->jp == WJ_IN ? WF_IN : w->jp == WJ_OUT ? -WF_IN : 0.0f;
        w->hi = w->jn == WJ_IN ? 1.0f - WF_IN : w->jn == WJ_OUT ? 1.0f + WF_IN : 1.0f;
    }
    int nl = 0, bad = 0;
    for (int f = 0; f < nwf; f++) {
        if (WF[f].loop >= 0) continue;
        float s = 0.0f;
        int g = f, k = 0;
        do { WF[g].loop = (int16_t)nl; WF[g].s0 = s; s += WF[g].hi - WF[g].lo; g = WF[g].next; } while (g >= 0 && g != f && ++k < MAXWF);
        if (g != f) bad++;
        wf_len[nl++] = s;
    }
    wf_hash = wf_layout();
#ifdef CRAWL_DEBUG
    if (bad) fprintf(stderr, "EVENT skitter_ways_broken %d loops level %d\n", bad, G.level);
#endif
}
static void wf_pos(int f, float u, float *x, float *y) {           /* where (f, u) is: WF_IN off the stone */
    const WFace *w = &WF[f];
    int t = (w->d + 1) & 3;
    *x = w->x + 0.5f + DX[w->d] * (0.5f - WF_IN) + DX[t] * (u - 0.5f);
    *y = w->y + 0.5f + DY[w->d] * (0.5f - WF_IN) + DY[t] * (u - 0.5f);
}
static void wf_move(int *f, float *u) {                            /* past a face's end: on to the next, or back to the one before */
    for (int k = 0; k < 64; k++) {
        const WFace *w = &WF[*f];
        if (*u > w->hi) { float over = *u - w->hi; *f = w->next; *u = WF[*f].lo + over; }
        else if (*u < w->lo) { float under = w->lo - *u; *f = w->prev; *u = WF[*f].hi - under; }
        else return;
    }
}
static float wf_gap(int f, float u, int g, float v) {             /* from (f, u) to (g, v) round their loop, the shorter way, signed;
                                                                      1e9 on another loop */
    if (WF[f].loop != WF[g].loop) return 1e9f;
    float L = wf_len[WF[f].loop], d = (WF[g].s0 + v - WF[g].lo) - (WF[f].s0 + u - WF[f].lo);
    if (d > L * 0.5f) d -= L; else if (d < -L * 0.5f) d += L;
    return d;
}
/* the place on the stone nearest (px, py) that can be walked to from it, within reach steps: its face, -1 none. corner: into a
 * corner a step off (a web's) */
static int wf_near(float px, float py, float *u, int reach, int corner) {
    static int16_t q[MAPN * MAPN];
    static uint16_t seen[MAPN][MAPN], stamp;
    int sx = (int)px, sy = (int)py;
    if (wf_solid(sx, sy)) return -1;
    if (++stamp == 0) { memset(seen, 0, sizeof seen); stamp = 1; }
    int qh = 0, qt = 0, lend = 1, layer = 0, found = -1, best = -1;
    float bd = 1e9f, bu = 0.0f;
    q[qt++] = (int16_t)(sy * MAPN + sx); seen[sy][sx] = stamp;
    while (qh < qt && layer <= reach) {
        int c = q[qh++], x = c % MAPN, y = c / MAPN;
        for (int d = 0; d < 4; d++) {
            int g = wf_at[y][x][d];
            if (g >= 0) {
                const WFace *w = &WF[g];
                int t = (w->d + 1) & 3;
                float v = clampf((px - (w->x + 0.5f)) * DX[t] + (py - (w->y + 0.5f)) * DY[t] + 0.5f, w->lo, w->hi), qx, qy;
                wf_pos(g, v, &qx, &qy);
                float dd = hypotf(qx - px, qy - py);
                if (dd < bd) { bd = dd; best = g; bu = v; }
                if (found < 0) found = layer;
            }
            int nx = x + DX[d], ny = y + DY[d];
            if (!wf_solid(nx, ny) && seen[ny][nx] != stamp) { seen[ny][nx] = stamp; q[qt++] = (int16_t)(ny * MAPN + nx); }
        }
        if (qh == lend) { layer++; lend = qt; if (found >= 0 && layer > found + 1) break; }
    }
    if (best >= 0 && corner) {
        const WFace *w = &WF[best];
        if (w->jn == WJ_IN && w->hi - bu < 0.35f) bu = w->hi; else if (w->jp == WJ_IN && bu - w->lo < 0.35f) bu = w->lo;
    }
    *u = bu;
    return best;
}

static void sk_go(Skitter *o, int f, float u, float h, int st, float pace) {
    if (f < 0 || WF[f].loop != WF[o->f].loop) { o->tf = -1; o->st = SK_IDLE; o->wait = 1.0f + srndf(); return; }
    o->tf = f; o->tu = u; o->ht = clampf(h, 0.06f, 0.94f); o->st = st; o->pace = pace; o->t = 0.0f;
}
static float sk_ang(float a) { while (a > PI_F) a -= TAU_F; while (a < -PI_F) a += TAU_F; return a; }
static int sk_crawl(Skitter *o, float dt) {                        /* along the stone and up or down it toward its mark; 1 there */
    if (o->tf < 0) return 1;
    float ds = wf_gap(o->f, o->u, o->tf, o->tu);
    if (ds > 1e8f) { o->tf = -1; return 1; }
    float dh = o->ht - o->h, dist = sqrtf(ds * ds + dh * dh), s = o->pace * dt;
    if (dist > 1e-4f) o->head += sk_ang(atan2f(dh, ds) - o->head) * fminf(1.0f, dt * 10.0f);
    if (dist <= s) { o->f = o->tf; o->u = o->tu; o->h = o->ht; o->tf = -1; return 1; }
    o->u += ds / dist * s; o->h += dh / dist * s;
    wf_move(&o->f, &o->u);
    return 0;
}
static int sk_calm(const Skitter *o) { return o->alive && (o->st == SK_IDLE || o->st == SK_CRAWL); }
static int sk_spider_calm(const Mob *m) {                         /* a spider not yet told, and not about you already */
    if (!m->alive || m->state != MS_WALK || m->boss || m->add || m->ally || m->told_t > 0.0f) return 0;
    if (m->type == MOB_SPIDER) return m->sub == 0;                 /* in its web */
    return m->type == MOB_HUNTER && m->alert <= 0.0f && m->aware != MA_HUNT && m->aware != MA_TRACK;
}
static int sk_mark(const Mob *m, float *u) {                      /* where on the stone a spider is told from: a web spider's corner,
                                                                      the wall nearest a hunter; its face, -1 none */
    int web = m->type == MOB_SPIDER;
    return wf_near(web ? m->ax : m->x, web ? m->ay : m->y, u, 6, web);
}
static float sk_height(const Mob *m) { return m->type == MOB_SPIDER ? 0.8f : 0.25f; }   /* up by its web; low, by a hunter */
static int sk_find_spider(const Skitter *o) {                     /* the nearest calm spider round its loop, within 36 cells' crawl */
    int best = 0;                                                  /* (about fourteen seconds' run): 1 + it, 0 none */
    float bd = 36.0f;
    for (int i = 0; i < MAXMOBS; i++) {
        const Mob *m = &G.mobs[i];
        float u;
        if (!sk_spider_calm(m)) continue;
        int g = sk_mark(m, &u);
        if (g < 0) continue;
        float d = fabsf(wf_gap(o->f, o->u, g, u));
        if (d < bd) { bd = d; best = 1 + i; }
    }
    return best;
}
static int sk_find_web(const Skitter *o, float reach, int *wf, float *wu) {   /* a corner web within reach round its loop: 1 + its deco */
    int best = 0;
    float bd = reach;
    for (int i = 0; i < G.ndecos; i++) {
        const Deco *dc = &G.decos[i];
        float u;
        if (dc->spr != SP_COBWEB) continue;
        int g = wf_near(dc->x, dc->y, &u, 2, 1);
        if (g < 0) continue;
        float d = fabsf(wf_gap(o->f, o->u, g, u));
        if (d < bd) { bd = d; best = 1 + i; *wf = g; *wu = u; }
    }
    return best;
}
static int sk_for_web(Skitter *o, float reach, float pace) {      /* off to the nearest web within reach: 1 if there is one */
    int g = -1; float u = 0.0f;
    int w = sk_find_web(o, reach, &g, &u);
    if (!w) return 0;
    o->web = w; o->spider = 0;
    sk_go(o, g, u, 0.8f, SK_RUN, pace);
    return o->st == SK_RUN;
}
static void sk_to_web(Skitter *o, int web) {                       /* into the corner behind its web, a while */
    o->st = SK_WEB; o->web = web; o->spider = 0; o->seek = 0; o->t = 0.0f; o->tf = -1;
    o->wait = G.mboss ? 1e9f : 30.0f + 30.0f * srndf();
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT skitter_web %d at %.1f,%.1f t=%.2f\n", (int)(o - G.skit), o->x, o->y, G.t);
#endif
}
/* scurried: to tell the nearest spider; with none, off to find a web; it carries word only to a spider */
static void sk_scare(Skitter *o) {
    if (!sk_calm(o)) return;
    o->ax = G.px; o->ay = G.py;
    int sp = G.mboss ? 0 : sk_find_spider(o);
    if (sp) {
        const Mob *m = &G.mobs[sp - 1];
        float u;
        int g = sk_mark(m, &u);
        o->spider = sp; o->web = 0; o->seek = 0; o->seek_t = 0.0f; o->plan_t = 1.5f;   /* seek_t: how long it has run, here */
        sk_go(o, g, u, sk_height(m), SK_RUN, 2.6f);
        if (o->st == SK_RUN) {
            sfx_at(SFX_SKITTER, o->x, o->y, 0.35f);
#ifdef CRAWL_DEBUG
            fprintf(stderr, "EVENT skitter_run %d at %.1f,%.1f to spider %d t=%.2f\n", (int)(o - G.skit), o->x, o->y, sp - 1, G.t);
#endif
            return;
        }
    }
    o->spider = 0;
    if (sk_for_web(o, 40.0f, 2.2f)) return;
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
    if (!sk_for_web(o, 12.0f, 2.2f)) { o->st = SK_IDLE; o->wait = 3.0f; o->tf = -1; }
}
static void sk_slay(Skitter *o) {                                   /* one carrying word, hit: the word dies with it; not a kill. It */
    const WFace *w = &WF[o->f];                                     /* drops off the stone */
    o->st = SK_DEAD; o->t = 0.0f; o->spider = 0; o->tf = -1; o->vz = 0.0f;
    o->x -= DX[w->d] * 0.12f; o->y -= DY[w->d] * 0.12f;
    sfx_at(SFX_SKITTER, o->x, o->y, 0.6f);
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT skitter_slain %d t=%.2f\n", (int)(o - G.skit), G.t);
#endif
}
static int sk_open(const Skitter *o) { return o->alive && o->st == SK_RUN && o->spider; }
static void skit_shot(float x, float y) {
    for (int i = 0; i < G.nskit; i++) { Skitter *o = &G.skit[i]; if (sk_open(o) && hypotf(o->x - x, o->y - y) < 0.3f) sk_slay(o); }
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

static void sk_wander(Skitter *o, int reach) {                    /* a few faces on round its loop, up or down the stone */
    int g = o->f, k = 1 + srnd(reach), fwd = srnd(2);
    for (int i = 0; i < k; i++) g = fwd ? WF[g].next : WF[g].prev;
    float lo = WF[g].lo, hi = WF[g].hi;
    sk_go(o, g, lo + (hi - lo) * srndf(), 0.1f + 0.75f * srndf(), SK_CRAWL, o->seek ? 2.0f : 0.5f + 0.3f * srndf());
}

static void skit_level(void) {
    G.nskit = 0; memset(G.skit, 0, sizeof G.skit); sk_tame = 0;
    G.srs = ((G.rs ^ 0x5B1D3Au) * 2246822519u + (uint32_t)G.level * 104729u) | 1u;   /* their own dice: G.rs read, never drawn on */
#ifdef CRAWL_DEBUG
    if (getenv("CRAWL_SKIT") && atoi(getenv("CRAWL_SKIT")) == 0) return;
#endif
    if (mice_off() || G.shop || PK.depth || !G.faced) return;
    wf_build();
    int step = crawl_step(G.level), want = g_drawn ? 2 : G.boss >= 0 ? 2 : 2 + step / 3 + 2 * (crawl_set(G.level) == 1);
    mo_bfs(G.cx, G.cy, 1);
    static int16_t from_start[MAPN][MAPN];
    memcpy(from_start, mo_dist, sizeof from_start);
    for (int k = 0; k < want && G.nskit < MAXSKIT; k++) {
        int pick = -1, seen = 0;
        for (int f = 0; f < nwf; f++) {
            const WFace *w = &WF[f];
            if (from_start[w->y][w->x] >= 6 && mo_cell_ok(w->x, w->y) && wf_len[w->loop] >= 12.0f && srnd(++seen) == 0) pick = f;
        }
        if (pick < 0) break;
        Skitter *o = &G.skit[G.nskit++];
        o->alive = 1; o->f = pick; o->tf = -1;
        o->u = WF[pick].lo + (WF[pick].hi - WF[pick].lo) * srndf(); o->h = 0.15f + 0.6f * srndf();
        o->head = srndf() * TAU_F - PI_F; o->lz = -9.0f;
        wf_pos(o->f, o->u, &o->x, &o->y); o->z = o->h;
        o->st = SK_IDLE; o->wait = 4.0f * srndf();
    }
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT skitters level %d count %d faces %d\n", G.level, G.nskit, nwf);
#endif
}

#ifdef CRAWL_DEBUG
static int skit_tp_corner(int jt) {                                /* CRAWL_TPSKIT=2, 3: one put a little short of the nearest-lit
                                                                      corner of that joint (WJ_OUT round a wall's edge, WJ_IN into a
                                                                      corner), crawling slowly round it; you where both faces show */
    int bf = -1, bx = 0, by = 0;
    float bl = -1.0f;
    for (int f = 0; f < nwf; f++) {
        const WFace *w = &WF[f];
        if (w->jn != jt || G.map[w->y][w->x].kind != K_FLOOR || wf_len[w->loop] < 12.0f) continue;
        float px, py, ax, ay, cx, cy;
        wf_pos(f, w->hi, &px, &py); wf_pos(f, w->hi - 0.5f, &ax, &ay); wf_pos(w->next, WF[w->next].lo + 0.5f, &cx, &cy);
        float l = lm_at(px, py);
        if (l <= bl) continue;
        int sx = -1, sy = -1;
        float sd = 9.0f;
        for (int y = (int)py - 3; y <= (int)py + 3; y++)
            for (int x = (int)px - 3; x <= (int)px + 3; x++) {
                if (x <= 0 || y <= 0 || x >= MAPN - 1 || y >= MAPN - 1 || G.map[y][x].kind != K_FLOOR) continue;
                float d = hypotf(x + 0.5f - px, y + 0.5f - py);
                if (d < 1.4f || d > 2.4f || !line_clear(x + 0.5f, y + 0.5f, ax, ay) || !line_clear(x + 0.5f, y + 0.5f, cx, cy)) continue;
                int hid = 0;                                       /* nothing standing between */
                for (int k = 0; k < G.ndecos && !hid; k++) {
                    float vx = px - (x + 0.5f), vy = py - (y + 0.5f), wx = G.decos[k].x - (x + 0.5f), wy = G.decos[k].y - (y + 0.5f);
                    float tt = clampf((wx * vx + wy * vy) / (vx * vx + vy * vy), 0.0f, 1.0f);
                    hid = hypotf(wx - vx * tt, wy - vy * tt) < 0.7f && G.decos[k].spr != SP_COBWEB;
                }
                if (hid) continue;
                if (fabsf(d - 1.8f) < sd) { sd = fabsf(d - 1.8f); sx = x; sy = y; }
            }
        if (sx < 0) continue;
        bl = l; bf = f; bx = sx; by = sy;
    }
    if (bf < 0 || !G.nskit) return 0;
    Skitter *o = &G.skit[0];
    o->f = bf; o->u = WF[bf].hi - 0.6f; o->h = 0.45f; o->head = 0.0f; o->lz = -9.0f;
    int g = WF[bf].next;
    float u = WF[g].lo + 0.6f;
    sk_go(o, g, u, 0.6f, SK_CRAWL, 0.3f);
    wf_pos(o->f, o->u, &o->x, &o->y); o->z = o->h;
    float px, py;
    wf_pos(bf, WF[bf].hi, &px, &py);
    G.cx = bx; G.cy = by; G.px = bx + 0.5f; G.py = by + 0.5f; G.ang = atan2f(py - G.py, px - G.px);
    fprintf(stderr, "TPSKIT corner %s face %d (%d,%d d%d) you at %d,%d\n", jt == WJ_OUT ? "outer" : "inner", bf, WF[bf].x, WF[bf].y, WF[bf].d, bx, by);
    human_begin();
    G.hint_t = 0.0f; sk_tame = 1;                                  /* the card put away; it minds you not, to be looked at close */
    return 1;
}
static void skit_tp(void) {                                        /* CRAWL_TPSKIT: two cells from the best lit on a wall, facing it */
    int mode = atoi(getenv("CRAWL_TPSKIT"));
    if (mode >= 2 && skit_tp_corner(mode == 2 ? WJ_OUT : WJ_IN)) return;
    int order[MAXSKIT], n = 0;
    for (int i = 0; i < G.nskit; i++) if (G.skit[i].alive) order[n++] = i;
    for (int a = 1; a < n; a++) { int v = order[a], b = a - 1; while (b >= 0 && lm_at(G.skit[order[b]].x, G.skit[order[b]].y) < lm_at(G.skit[v].x, G.skit[v].y)) { order[b + 1] = order[b]; b--; } order[b + 1] = v; }
    for (int j = 0; j < n; j++) {
        Skitter *o = &G.skit[order[j]];
        const WFace *w = &WF[o->f];
        for (int back = 2; back >= 1; back--) {
            int x = w->x - DX[w->d] * back, y = w->y - DY[w->d] * back;
            if (x <= 0 || y <= 0 || x >= MAPN - 1 || y >= MAPN - 1 || G.map[y][x].kind != K_FLOOR || !line_clear(x + 0.5f, y + 0.5f, o->x, o->y)) continue;
            G.cx = x; G.cy = y; G.px = x + 0.5f; G.py = y + 0.5f; G.ang = atan2f(o->y - G.py, o->x - G.px);
            o->st = SK_IDLE; o->wait = getenv("CRAWL_SKITGO") ? 0.0f : 20.0f;   /* CRAWL_SKITGO: let it crawl off at once */
            human_begin();
            return;
        }
    }
}
#endif

static void skit_rebuild(void) {                                   /* the walls moved (a secret's block): trace them again, and put
                                                                      each spider back on the face it was on, or the nearest */
    int fx[MAXSKIT], fy[MAXSKIT], fd[MAXSKIT], tx[MAXSKIT], ty[MAXSKIT], td[MAXSKIT];
    for (int i = 0; i < G.nskit; i++) {
        const Skitter *o = &G.skit[i];
        fx[i] = WF[o->f].x; fy[i] = WF[o->f].y; fd[i] = WF[o->f].d;
        tx[i] = o->tf >= 0 ? WF[o->tf].x : 0; ty[i] = o->tf >= 0 ? WF[o->tf].y : 0; td[i] = o->tf >= 0 ? WF[o->tf].d : 0;
    }
    wf_build();
    for (int i = 0; i < G.nskit; i++) {
        Skitter *o = &G.skit[i];
        if (!o->alive || o->st == SK_DEAD) continue;
        int g = wf_at[fy[i]][fx[i]][fd[i]];
        if (g >= 0) o->u = clampf(o->u, WF[g].lo, WF[g].hi); else g = wf_near(o->x, o->y, &o->u, 4, 0);
        if (g < 0) { o->alive = 0; continue; }
        o->f = g;
        if (o->tf >= 0) {
            int h = wf_at[ty[i]][tx[i]][td[i]];
            if (h >= 0 && WF[h].loop == WF[g].loop) { o->tf = h; o->tu = clampf(o->tu, WF[h].lo, WF[h].hi); }
            else { o->tf = -1; if (o->st != SK_WEB) { o->st = SK_IDLE; o->wait = 0.5f; o->spider = o->web = 0; } }
        }
    }
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT skitter_ways level %d faces %d t=%.2f\n", G.level, nwf, G.t);
#endif
}

static void skit_update(float dt) {
    if (!G.nskit || G.shop || PK.depth) return;
    if (wf_layout() != wf_hash) skit_rebuild();
    for (int i = 0; i < G.nskit; i++) {
        Skitter *o = &G.skit[i];
        if (!o->alive) continue;
        o->t += dt;
        if (G.mboss && o->st != SK_WEB && o->st != SK_DEAD && !o->web) {   /* a boss's fight: into the nearest web, for good */
            if (sk_for_web(o, 60.0f, 2.4f)) continue;
        }
        if (sk_calm(o) && !(sk_tame && i == 0)) {                  /* you near, or a fight */
            float d = hypotf(o->x - G.px, o->y - G.py);
            if (!G.dead && (d < 2.0f || (d < 3.5f && G.spd > 0.4f && line_clear(o->x, o->y, G.px, G.py)))) { sk_scare(o); continue; }
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
            if (o->seek) {                                         /* looking for a web: one a few faces off, or on */
                o->seek_t += dt;
                if (sk_for_web(o, 6.0f, 2.2f)) break;
                if (o->seek_t > 25.0f) { o->seek = 0; o->wait = 3.0f; break; }   /* it settles where it is */
            }
            sk_wander(o, o->seek ? 6 : 5);
            break;
        case SK_CRAWL:
            if (o->seek) o->seek_t += dt;
            if (sk_crawl(o, dt) && sk_tame && i == 0) { int g = WF[o->f].prev; sk_go(o, WF[g].prev, WF[WF[g].prev].lo + 0.5f, 0.35f, SK_CRAWL, 0.3f); break; }   /* and back */
            else if (o->tf < 0) { o->st = SK_IDLE; o->wait = o->seek ? 0.2f : 2.0f + 6.0f * srndf(); }
            break;
        case SK_RUN:
            if (o->spider) {
                Mob *m = &G.mobs[o->spider - 1];
                if (!sk_spider_calm(m)) { o->spider = 0; o->st = SK_IDLE; o->wait = 0.0f; o->seek = 1; o->seek_t = 0.0f; o->tf = -1; break; }
                float sx = m->type == MOB_SPIDER ? m->ax : m->x, sy = m->type == MOB_SPIDER ? m->ay : m->y, sd = hypotf(sx - o->x, sy - o->y);
                if (m->type == MOB_HUNTER && sd < 2.2f && line_clear(o->x, o->y, m->x, m->y)) { sk_tell(o, m); break; }   /* near enough to
                                                                      be heard from the wall */
                if ((o->seek_t += dt) > 20.0f) {                   /* it gives it up, on a clock of its own */
#ifdef CRAWL_DEBUG
                    fprintf(stderr, "EVENT skitter_gaveup %d to %s %.1f away at %.1f,%.1f t=%.2f\n", i, m->type == MOB_HUNTER ? "hunter" : "web", sd, o->x, o->y, G.t);
#endif
                    o->spider = 0; o->st = SK_IDLE; o->wait = 2.0f; o->tf = -1; break;
                }
                int there = sk_crawl(o, dt);
                if (there && m->type == MOB_SPIDER) { sk_tell(o, m); break; }   /* at its web */
                if (there || (m->type == MOB_HUNTER && (o->plan_t -= dt) <= 0.0f)) {   /* after a hunter as it goes: the wall nearest it */
                    float u;
                    int g = sk_mark(m, &u);
                    o->plan_t = 1.5f;
                    sk_go(o, g, u, sk_height(m), SK_RUN, o->pace);
                    if (o->st != SK_RUN) o->spider = 0;
                }
            } else if (o->web) {
                if (sk_crawl(o, dt)) sk_to_web(o, o->web);         /* there: into the corner behind it */
            } else { o->st = SK_IDLE; o->wait = 1.0f; o->tf = -1; }
            break;
        case SK_WEB:
            if (o->t >= o->wait && !G.mboss) { o->web = 0; o->seek = 0; o->st = SK_IDLE; o->wait = 2.0f; }   /* out again, about its business */
            break;
        case SK_DEAD:                                              /* off the stone, to the floor */
            o->vz -= 9.0f * dt; o->z = fmaxf(0.0f, o->z + o->vz * dt);
            if (o->t >= 1.0f) o->alive = 0;
            break;
        }
    }
    for (int i = 0; i < G.nskit; i++) {                            /* where each is now: on its face, always */
        Skitter *o = &G.skit[i];
        if (!o->alive || o->st == SK_DEAD) continue;
        wf_pos(o->f, o->u, &o->x, &o->y); o->z = o->h;
#ifdef CRAWL_DEBUG
        if (o->lz > -5.0f && sqrtf((o->x - o->lx) * (o->x - o->lx) + (o->y - o->ly) * (o->y - o->ly) + (o->z - o->lz) * (o->z - o->lz)) > 0.3f)
            fprintf(stderr, "EVENT skitter_jump %d from %.2f,%.2f,%.2f to %.2f,%.2f,%.2f t=%.2f\n", i, o->lx, o->ly, o->lz, o->x, o->y, o->z, G.t);
#endif
        o->lx = o->x; o->ly = o->y; o->lz = o->z;
    }
}
