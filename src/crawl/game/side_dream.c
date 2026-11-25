/* A part of scene_crawl.c (docs/PLAN-side.md): it is #included there in its place and never compiled on its own.
 * The dream in a side passage (step 7): where it can stand (a tile with floor under it and room over it), how it gets from one
 * to the next (a walk, a hop up, a leap over, a drop), the platforms where they are now; a way through to the goal, found again
 * every few tenths; held keys to follow it; the dead before it cast at. A first version: it waits for what moves rather than
 * timing it. */

#define SDD_NODES (SD_W * SD_H)
static struct {
    int16_t from[SDD_NODES];
    int path[256], npath, at;
    float plan_t, stuck_t, wait_t, best_x, give_t, hold_t;
    int goal, last_node, jumping, jdir, target_node;
} SDD;

static int sdd_hazard_floor(int tx, int ty) { int b = sd_tile(tx, ty + 1); return b == '^' || b == '~'; }
static int sdd_plat_under(int tx, int ty) {                        /* a platform's top is this tile's floor, now */
    const SdLevel *L = SD.L;
    for (int i = 0; i < L->nth; i++) {
        const SdThing *t = &L->th[i];
        if (t->kind != SDT_PLAT) continue;
        const SdPlat *p = &SD.plat[i];
        int pty = (int)floorf((p->y + 0.5f) / SD_TILE), ptx0 = (int)floorf((p->x + 4.0f) / SD_TILE), ptx1 = (int)floorf((p->x + (float)(t->w * SD_TILE) - 4.0f) / SD_TILE);
        if (ty + 1 != pty || tx < ptx0 || tx > ptx1 || fabsf(p->y - (float)(pty * SD_TILE)) >= 6.0f) continue;
        if (p->wait > 0.12f || (SD.on_plat == i && sd_floor_px(SD.x) == tx)) return 1;   /* only while it waits at an end, or under you */
    }
    return 0;
}
static int sdd_room(int tx, int ty) {                              /* 2 room to stand, 1 to creep, 0 none */
    if (sd_solid(tx, ty) || sd_tile(tx, ty) == '~' || sd_tile(tx, ty) == '^') return 0;
    return sd_solid(tx, ty - 1) ? 1 : 2;
}
static int sdd_stand(int tx, int ty) {
    if (tx < 0 || ty < 1 || tx >= SD.L->w || ty >= SD.L->h - 0) return 0;
    if (!sdd_room(tx, ty)) return 0;
    int b = sd_tile(tx, ty + 1);
    if (sd_solid(tx, ty + 1) || b == '=' || sdd_plat_under(tx, ty)) return !sdd_hazard_floor(tx, ty);
    return 0;
}
static int sdd_air(int tx, int ty) { return tx >= 0 && tx < SD.L->w && !sd_solid(tx, ty); }
static int sdd_clear_col(int tx, int y0, int y1) { for (int y = y0; y <= y1; y++) if (!sdd_air(tx, y)) return 0; return 1; }
/* can it get from a to b with one move? a rough test of the arc's room */
static int sdd_link(int ax, int ay, int bx, int by) {
    int dx = bx - ax, dy = by - ay, adx = abs(dx), s = dx > 0 ? 1 : dx < 0 ? -1 : 0;
    if (adx == 1 && dy == 0) return sdd_room(ax, ay) && sdd_room(bx, by);   /* a step, creeping if it must */
    if (sdd_room(ax, ay) < 2) return 0;                             /* every other move stands up */
    if (dy < 0) {                                                  /* up: three at most, across four */
        if (-dy > 3 || adx > 4 || (-dy == 3 && adx > 3)) return 0;
        if (!sdd_clear_col(ax, ay + dy - 1, ay)) return 0;
        for (int x = ax + s; x != bx + s && s; x += s) if (!sdd_clear_col(x, by - 1, by)) return 0;
        return sdd_room(bx, by) == 2;
    }
    if (dy == 0) {                                                 /* over a gap: five at most */
        if (adx < 2 || adx > 5) return 0;
        for (int x = ax + s; x != bx; x += s) if (!sdd_clear_col(x, ay - 2, ay)) return 0;
        return 1;
    }
    if (adx > 4 + dy / 2 || dy > 10) return 0;                     /* down */
    for (int x = ax + s; x != bx + s && s; x += s) if (!sdd_clear_col(x, imin(ay, by) - 1, by)) return 0;
    if (!s && !sdd_clear_col(ax, ay + 1, by)) return 0;
    return 1;
}
static int sdd_node(int tx, int ty) { return ty * SD_W + tx; }
static void sdd_plan(void) {
    const SdLevel *L = SD.L;
    static int q[SDD_NODES];
    for (int i = 0; i < SDD_NODES; i++) SDD.from[i] = -2;
    int sx = sd_floor_px(SD.x), sy = sd_floor_px(SD.y - 1.0f);
    if (!sdd_stand(sx, sy)) { SDD.npath = 0; return; }
    int gx = L->th[SDD.goal].x, gy = L->th[SDD.goal].y, start = sdd_node(sx, sy), goal = sdd_node(gx, gy);
    int qh = 0, qt = 0;
    q[qt++] = start; SDD.from[start] = -1;
    while (qh < qt && SDD.from[goal] == -2) {
        int n = q[qh++], ax = n % SD_W, ay = n / SD_W;
        for (int dy = -3; dy <= 10; dy++)
            for (int dx = -5; dx <= 5; dx++) {
                if (!dx && dy <= 0) continue;
                int bx = ax + dx, by = ay + dy;
                if (bx < 0 || by < 0 || bx >= L->w || by >= L->h) continue;
                int m = sdd_node(bx, by);
                if (SDD.from[m] != -2 || !sdd_stand(bx, by) || !sdd_link(ax, ay, bx, by)) continue;
                if ((dy || abs(dx) > 1) && sdd_plat_under(bx, by) && !sd_solid(bx, by + 1) && sd_tile(bx, by + 1) != '=') continue;   /* onto a platform: a step, no leap */
                SDD.from[m] = (int16_t)(n > 32767 ? -1 : n);
                q[qt++] = m;
            }
    }
    SDD.npath = 0; SDD.at = 0;
#ifdef CRAWL_DEBUG
    if (getenv("CRAWL_SIDETRACE")) {
        int far = start;
        for (int i = 0; i < qt; i++) if (q[i] % SD_W > far % SD_W) far = q[i];
        fprintf(stderr, "SIDE plan from %d,%d to %d,%d: %d reached, the furthest %d,%d, %s\n", sx, sy, gx, gy, qt, far % SD_W, far / SD_W, SDD.from[goal] == -2 ? "no way" : "a way");
    }
#endif
    if (SDD.from[goal] == -2) {                                    /* no whole way now: the nearest it can reach to the goal, to wait there */
        int best = start, bd = abs(gx - sx) * 2 + abs(gy - sy);
        for (int i = 1; i < qt; i++) { int d = abs(gx - q[i] % SD_W) * 2 + abs(gy - q[i] / SD_W); if (d < bd) { bd = d; best = q[i]; } }
        if (best == start) return;
        goal = best;
    }
    int rev[256], k = 0;
    for (int n = goal; n != start && n >= 0 && k < 256; n = SDD.from[n]) rev[k++] = n;
    for (int i = 0; i < k; i++) SDD.path[i] = rev[k - 1 - i];
    SDD.npath = k;
}
static int sdd_flow_soon(int tx, int ty, float within) {           /* a flow over this tile now, or before it can be crossed (within < 0) */
    const SdLevel *L = SD.L;
    for (int i = 0; i < L->nth; i++) {
        const SdThing *t = &L->th[i];
        if (t->kind != SDT_FLOW || tx < t->x || tx >= t->x + t->w || ty < t->y || ty >= t->y + t->h) continue;
        float per = t->f[0] + t->f[1], c = fmodf(SD.t + t->f[2], per);
        if (within < 0.0f) {                                       /* the time to the far side of it, creeping some of the way */
            float far = SD.x < (float)(t->x * SD_TILE) ? (float)((t->x + t->w) * SD_TILE) : (float)(t->x * SD_TILE);
            within = fabsf(far - SD.x) / 45.0f + 0.4f;
        }
        if (c < t->f[0] + 0.6f) return 1;                          /* up, or still draining */
        if (per - c < within) return 1;                            /* about to rise */
    }
    return 0;
}
static void sdd_fight(void) {                                      /* the nearest of the dead before it, in reach: face it and cast */
    SdIn *in = &SD.in;
    int sp = G.spell >= 0 && G.spell < NATK && RUN.gear.atk[G.spell] >= 0 ? G.spell : own_spell();
    if (sp < 0) return;
    if (sp == AT_BFS || sp == AT_FLASK) { int o = own_missile(); if (o >= 0) sp = o; }   /* the dream keeps to what flies straight */
    G.spell = sp;
    float reach = sd_reach(ATK_ELEM[sp]) * SD_TILE, best = 1e9f;
    const SdMob *t = NULL;
    for (int i = 0; i < SD_MOBS; i++) {
        const SdMob *m = &SD.mob[i];
        if (!m->alive || m->state == SDM_DIE || ((KINDS[m->kind].flags & KF_SPIRIT) && ATK_ELEM[sp] != EL_SPIRIT)) continue;
        float d = fabsf(m->x - SD.x), dy = (m->y - sd_mob_h(m) * 0.5f) - (SD.y - 17.0f);
        if (d > fminf(reach, 150.0f) || fabsf(dy) > 14.0f + sd_mob_h(m) * 0.5f) continue;
        if (!sd_line_clear(SD.x, SD.y - 17.0f, m->x, m->y - sd_mob_h(m) * 0.5f)) continue;
        if (d < best) { best = d; t = m; }
    }
    if (!t) return;
    int dir = t->x > SD.x ? 1 : -1;
    if (SD.face != dir) { in->left = dir < 0; in->right = dir > 0; }
    else if (SD.cd <= 0.0f) { in->cast = 1; in->cast_hit = 1; }
}
static void sdd_steer(SdIn *in, float tx) {                         /* in the air: to the tile, letting go in time to come down on it */
    float gap = tx - SD.x, stop = SD.vx * SD.vx / (2.0f * 560.0f);
    int toward = (gap > 0.0f) == (SD.vx > 0.0f), dir = gap > 0.0f ? 1 : -1;
    if (toward && fabsf(gap) <= stop + 2.0f) dir = 0;
    else if (fabsf(gap) <= 2.0f) dir = 0;
    in->left = dir < 0; in->right = dir > 0;
    in->run = fabsf(gap) > 40.0f;
}
static void sd_dream(float dt) {
    SdIn *in = &SD.in;
    memset(in, 0, sizeof *in);
    if (SD.dying) return;
    const SdLevel *L = SD.L;
    SDD.give_t += dt;
    if (SD.in_t < 0.05f) { memset(&SDD, 0, sizeof SDD); SDD.goal = -1; }
    if (SDD.goal < 0 || SDD.goal >= L->nth || L->th[SDD.goal].kind != SDT_DOOR) {   /* the goal doorway, else the furthest other way */
        SDD.goal = -1;
        for (int i = 0; i < L->nth; i++) if (L->th[i].kind == SDT_DOOR && (L->th[i].flags & SDF_GOAL)) SDD.goal = i;
        if (SDD.goal < 0 || SDP.done[SD.side]) {
            float far = -1.0f;
            for (int i = 0; i < L->nth; i++)
                if (L->th[i].kind == SDT_DOOR && L->th[i].a != SD.link_in && fabsf(SD.x - (float)(L->th[i].x * SD_TILE)) > far) { far = fabsf(SD.x - (float)(L->th[i].x * SD_TILE)); SDD.goal = i; }
        }
        if (SDD.goal < 0) return;
    }
    if (SDD.give_t > 180.0f) {                                     /* three minutes and no way through: back out */
#ifdef CRAWL_DEBUG
        fprintf(stderr, "EVENT side_dream gives up t=%.2f\n", SD.t);
#endif
        sd_leave(SD.link_in, 0);
        return;
    }
    if (sd_door_here() == SDD.goal && SD.ground) { in->up_hit = 1; return; }
    int in_flow = sdd_flow_soon(sd_floor_px(SD.x), sd_floor_px(SD.y - 1.0f), 1e9f);   /* (inside a flow's box: keep going) */
    if (!in_flow) sdd_fight();
    if (in->cast || in->left || in->right) return;                 /* turning to cast, or casting: a moment */
    if ((SDD.plan_t -= dt) <= 0.0f || SDD.at >= SDD.npath) {
        if (SD.ground) { sdd_plan(); SDD.plan_t = 0.25f; }
    }
    int cx = sd_floor_px(SD.x), cy = sd_floor_px(SD.y - 1.0f);
    if (SD.ground && !sdd_stand(cx, cy) && SD.on_plat < 0) {       /* somewhere it shouldn't be (the poison): out, to the nearest place to stand */
        int bx = -1, by = -1, bd = 99;
        for (int y = cy - 4; y <= cy + 1; y++) for (int x = cx - 6; x <= cx + 6; x++) if (sdd_stand(x, y) && abs(x - cx) + abs(y - cy) < bd) { bd = abs(x - cx) + abs(y - cy); bx = x; by = y; }
        if (bx >= 0) { in->left = bx < cx; in->right = bx > cx; in->jump = in->jump_hit = 1; in->run = 1; SDD.jumping = 1; SDD.jdir = bx < cx ? -1 : 1; (void)by; }
        return;
    }
    if (SD.ground) while (SDD.at < SDD.npath && SDD.path[SDD.at] == sdd_node(cx, cy)) SDD.at++;
    if (SDD.at >= SDD.npath) {                                     /* no way now: wait where it stands (a platform, a block coming back) */
        SDD.wait_t += dt; SDD.stuck_t = 0.0f; SDD.best_x = SD.x;
        if (!SD.ground && SDD.target_node > 0) { in->jump = SD.vy < 0.0f && SDD.jumping; sdd_steer(in, (float)((SDD.target_node % SD_W) * SD_TILE) + 8.0f); }
        return;
    }
    int n = SDD.path[SDD.at], nx = n % SD_W, ny = n / SD_W;
    float tx = (float)(nx * SD_TILE) + 8.0f;
    if (SD.ground && !sdd_stand(nx, ny)) {                         /* gone since the plan (a platform moved on, a block fell): wait, look again */
        float mid = (float)(cx * SD_TILE) + 8.0f;
        in->left = SD.x > mid + 1.0f; in->right = SD.x < mid - 1.0f;
        SDD.plan_t = 0.0f; SDD.stuck_t = 0.0f; SDD.best_x = SD.x;
        return;
    }
    int dir = tx > SD.x + 1.5f ? 1 : tx < SD.x - 1.5f ? -1 : 0;
    int ddx = nx - cx, ddy = ny - cy;
#ifdef CRAWL_DEBUG
    if (getenv("CRAWL_SIDETRACE") && n != SDD.target_node) fprintf(stderr, "SIDE next %d,%d from %d,%d ground %d t=%.2f\n", nx, ny, cx, cy, SD.ground, SD.t);
#endif
    SDD.target_node = n;
    /* the poison's clock: wait at its edge until it is down and will stay down a while */
    if (SD.ground && sdd_flow_soon(nx, ny, -1.0f) && !sdd_flow_soon(cx, cy, 0.0f)) {   /* back from its edge, to the middle of the tile */
        float mid = (float)(cx * SD_TILE) + 8.0f;
        in->left = SD.x > mid + 1.0f; in->right = SD.x < mid - 1.0f;
        SDD.stuck_t = 0.0f; SDD.best_x = SD.x;
        return;
    }
    if (sdd_room(nx, ny) == 1 || sdd_room(cx, cy) == 1) in->down = 1;   /* a low way: creep */
    if (!SD.ground) {                                              /* in the air: steer to the tile it is going to */
        in->jump = SD.vy < 0.0f && SDD.jumping && (ny < cy || SD.y > (float)((ny + 1) * SD_TILE) - 8.0f);
        sdd_steer(in, tx);
        return;
    }
    SDD.jumping = 0;
    if (fabsf(SD.x - SDD.best_x) > 2.0f) { SDD.best_x = SD.x; SDD.stuck_t = 0.0f; }   /* stuck: a hop, then a fresh look */
    else if ((SDD.stuck_t += dt) > 1.2f) {
        in->jump = in->jump_hit = 1; in->left = dir < 0; in->right = dir > 0; SDD.jumping = 1; SDD.jdir = dir;
        if (SDD.stuck_t > 3.0f) { SDD.stuck_t = 0.0f; SDD.plan_t = 0.0f; }
        return;
    }
    int need_jump = ddy < 0 || (ddy == 0 && abs(ddx) >= 2);
    if (need_jump) {
        float edge = (float)(cx * SD_TILE) + (ddx > 0 ? (float)SD_TILE - SD_HW - 0.5f : ddx < 0 ? SD_HW + 0.5f : 8.0f);   /* take off from the edge toward it */
        if ((ddx > 0 && SD.x < edge) || (ddx < 0 && SD.x > edge)) { in->left = ddx < 0; in->right = ddx > 0; in->run = abs(ddx) >= 3; return; }
        if (!ddx && fabsf(SD.x - edge) > 3.0f) { in->left = SD.x > edge; in->right = SD.x < edge; return; }
        in->jump = 1; in->jump_hit = 1; in->left = dir < 0; in->right = dir > 0; in->run = abs(ddx) >= 3;
        SDD.jumping = 1; SDD.jdir = dir;
        return;
    }
    in->left = dir < 0; in->right = dir > 0;
    in->run = abs(ddx) >= 2 || (ddy > 0 && abs(ddx) >= 2);
}
