/* A part of scene_crawl.c (docs/PLAN-tidy.md): it is #included there in its place and never compiled on its own.
 * Each kind's own ways: the Wraith (and the wards' field it walks by), the buried, pools, wall skulls, spiders and leaps, sacs, shades, gargoyles; then the bosses' own: the knight, the spider, the Wraith Unbound. */

/* ---- the soul-lamps' ward (1.0.0b18.1, docs/PLAN-b18-1.md section 6) ----
 * A lamp by each challenge (a trial's room, a puzzle's ground and wing, a sequence of things that want a spell); a soul
 * offered buys two minutes of it, five at most banked, burning only while the Wraith is on the level. While one burns its
 * ground, and a ring of a cell round it, is stone to the Wraith: it goes round, or, with you inside, to the edge nearest you
 * and along it as you move. Kept outside G, so a pocket leaves it as it was, and made again with each level. */
#define MAXWL 8
#define WARD_SOUL_T 120.0f                  /* a soul's worth of ward */
#define WARD_MAX 5                          /* souls a lamp holds */
enum { WLK_TRIAL, WLK_PUZZLE, WLK_SEQ };
typedef struct {
    int kind, cx, cy, d, deco;              /* what it guards; the floor cell before it, the wall it hangs on (DX, DY); its decoration */
    float banked;                           /* seconds of ward left in it */
    int said, offered;                      /* its words said once; souls the dream has put in */
} WLamp;
static struct {
    int n, mask;                            /* lamps; the bits of those burning */
    WLamp l[MAXWL];
    uint8_t cell[MAPN][MAPN];               /* a bit for each lamp's ground, its ring with it */
    int16_t field[MAPN][MAPN], out[MAPN][MAPN];   /* while one burns: the Wraith's way to you round them; steps out of them */
    float field_t, hiss_t, flare_t, flare_x, flare_y, offer_t;   /* the field's next making; the hiss; the edge's flare and where; the dream's look */
    int at_edge, said_held;                 /* the Wraith held at an edge; that said once */
} WD;
static int wraith_barred(int x, int y) { return !PK.depth && x >= 0 && y >= 0 && x < MAPN && y < MAPN && (WD.cell[y][x] & WD.mask); }
static int wraith_h(int x, int y) {                                /* the field the Wraith goes down: the ward's while one burns */
    if (x < 0 || y < 0 || x >= MAPN || y >= MAPN) return -1;
    return WD.mask && !PK.depth ? WD.field[y][x] : hunt[y][x];
}
static void ward_field(void) {                                     /* from you, round what is warded; from its edge, with you inside it */
    static int qx[MAPN * MAPN], qy[MAPN * MAPN], sv[MAPN * MAPN], sx[MAPN * MAPN], sy[MAPN * MAPN];
    int qh = 0, qt = 0, ns = 0;
    for (int y = 0; y < MAPN; y++) for (int x = 0; x < MAPN; x++) { WD.field[y][x] = -1; WD.out[y][x] = wraith_barred(x, y) ? -1 : 0; }
    int cx = (int)G.px, cy = (int)G.py;
    if (!wraith_barred(cx, cy)) { sx[0] = cx; sy[0] = cy; sv[0] = 0; ns = 1; }
    else
        for (int y = 1; y < MAPN - 1; y++)                         /* every open cell at the edge, by its distance from you */
            for (int x = 1; x < MAPN - 1; x++) {
                if (wraith_barred(x, y) || !hunt_pass(x, y)) continue;
                int edge = 0;
                for (int k = 0; k < 4; k++) edge |= wraith_barred(x + DX[k], y + DY[k]);
                if (!edge) continue;
                sx[ns] = x; sy[ns] = y; sv[ns] = (int)(hypotf(x + 0.5f - G.px, y + 0.5f - G.py) + 0.5f); ns++;
            }
    for (int a = 1; a < ns; a++)                                   /* nearest first */
        for (int b = a; b > 0 && sv[b] < sv[b - 1]; b--) { int t = sv[b]; sv[b] = sv[b - 1]; sv[b - 1] = t; t = sx[b]; sx[b] = sx[b - 1]; sx[b - 1] = t; t = sy[b]; sy[b] = sy[b - 1]; sy[b - 1] = t; }
    int si = 0;
    for (;;) {                                                     /* a breadth-first walk, the seeds let in as their distance comes */
        while (si < ns && (qh == qt || sv[si] <= WD.field[qy[qh]][qx[qh]])) {
            if (WD.field[sy[si]][sx[si]] < 0) { WD.field[sy[si]][sx[si]] = (int16_t)sv[si]; qx[qt] = sx[si]; qy[qt] = sy[si]; qt++; }
            si++;
        }
        if (qh == qt) break;
        int x = qx[qh], y = qy[qh]; qh++;
        for (int k = 0; k < 4; k++) {
            int nx = x + DX[k], ny = y + DY[k];
            if (nx < 1 || ny < 1 || nx >= MAPN - 1 || ny >= MAPN - 1 || WD.field[ny][nx] >= 0 || !hunt_pass(nx, ny) || wraith_barred(nx, ny)) continue;
            WD.field[ny][nx] = (int16_t)(WD.field[y][x] + 1); qx[qt] = nx; qy[qt] = ny; qt++;
        }
    }
    qh = qt = 0;                                                   /* and inside: steps to the nearest cell outside */
    for (int y = 0; y < MAPN; y++) for (int x = 0; x < MAPN; x++) if (WD.out[y][x] == 0 && hunt_pass(x, y)) { qx[qt] = x; qy[qt] = y; qt++; }
    while (qh < qt) {
        int x = qx[qh], y = qy[qh]; qh++;
        for (int k = 0; k < 4; k++) {
            int nx = x + DX[k], ny = y + DY[k];
            if (nx < 1 || ny < 1 || nx >= MAPN - 1 || ny >= MAPN - 1 || WD.out[ny][nx] >= 0 || !hunt_pass(nx, ny)) continue;
            WD.out[ny][nx] = (int16_t)(WD.out[y][x] + 1); qx[qt] = nx; qy[qt] = ny; qt++;
        }
    }
}
static int ward_step_out(Mob *m) {                                 /* the Wraith caught inside as a lamp is lit: out, the shortest way */
    if (!wraith_barred(m->cx, m->cy) || WD.out[m->cy][m->cx] < 0) return 0;
    int best = -1, bo = WD.out[m->cy][m->cx];
    for (int k = 0; k < 4; k++) {
        int nx = m->cx + DX[k], ny = m->cy + DY[k];
        if (nx < 1 || ny < 1 || nx >= MAPN - 1 || ny >= MAPN - 1 || !hunt_pass(nx, ny) || WD.out[ny][nx] < 0 || WD.out[ny][nx] >= bo) continue;
        bo = WD.out[ny][nx]; best = k;
    }
    if (best < 0) return 0;
    if (m->sub != 1) wraith_back_for(m, 1.6f);                     /* driven as by fire, a recoil once, */
    else m->sub_t = fmaxf(0.0f, m->sub_t - 1.2f);                  /* and kept driven a step at a time till it is out */
    const Cell *c = cell(m->cx + DX[best], m->cy + DY[best]);
    if (c->kind == K_DOOR && G.doors[c->door].open < 0.9f) { if (!G.doors[c->door].opening) G.doors[c->door].opening = 1; return 1; }
    m->tx = m->cx + DX[best]; m->ty = m->cy + DY[best];
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT ward_driven to (%d,%d) t=%.2f\n", m->tx, m->ty, G.t);
#endif
    return 1;
}

static void ward_make(void);
static void ward_tick(float dt);
/* The Wraith: from level 9, once a level, after about four minutes there, from the far end. It walks
 * the halls as you do, solid, down the hunt field, and pulls doors open. It can't be killed: fire,
 * the wisp's light close by, and Dread drive it back for a while; then it comes on again. Its touch
 * drains and chills. Torches gutter as it passes (build_lightmap). */
#define WRAITH_BACK_T 5.0f                  /* how long a drive lasts at most */
static void wraith_back_for(Mob *m, float secs) {
    float t0 = WRAITH_BACK_T - secs;                               /* it counts up to WRAITH_BACK_T */
    if (m->sub == 1) { m->sub_t = fminf(m->sub_t, t0); return; }
    m->sub = 1; m->sub_t = t0;
    sfx_at(SFX_SHRIEK, m->x, m->y, 1.3f);
    G.chill = fmaxf(G.chill, 0.4f);
    if (!m->boss) por_bump(MOB_WRAITH);                            /* for its portrait */
    DBG_COUNT(wraith_drives);
}
static void wraith_back(Mob *m) { wraith_back_for(m, WRAITH_BACK_T); }
/* Driven back into a corner it can't leave (nowhere further from you, or a door it can't open: a vault's still locked, a
 * sealed, winged, trialed or gated one), a moment, then it goes into the stone and comes again from the far end of the
 * level, as it first came. It had stood there, and the dream had stood driving it back for minutes at a time (a vault's
 * locked door at its back and the dream's way out through the vault). */
static void wraith_cornered(Mob *m, float dt) {
    if (m->sub != 1) { m->corner_t = 0.0f; return; }
    if ((m->corner_t += dt) < 1.0f) return;
    int x, y;
    if (!pick_cell_at_depth((int)G.px, (int)G.py, 20, 400, 1, &x, &y) || wraith_barred(x, y)) return;   /* never into a lit ward */
    m->corner_t = 0.0f;
    sfx_at(SFX_WAIL, m->x, m->y, 1.1f);
    m->x = x + 0.5f; m->y = y + 0.5f; m->cx = m->tx = x; m->cy = m->ty = y;
    m->sub = 0;                                                    /* and it comes again */
#ifdef CRAWL_DEBUG
    wraith_cornered_n++;
    fprintf(stderr, "EVENT wraith_cornered to (%d,%d) t=%.2f\n", x, y, G.t);
#endif
}
static void update_wraith(Mob *m, float dt) {
    m->t += dt;
    dt = mob_time(m, dt);
    if (dt <= 0.0f) return;
    m->anim += dt * 2.0f; m->cool -= dt; m->sub_t += dt;
    float pd = hypotf(G.px - m->x, G.py - m->y);
    if (m->feared > 0.0f) { m->feared = 0.0f; wraith_back(m); }    /* Dread */
    for (int i = 0; i < MAXWISPS && m->sub != 1; i++)              /* a wisp's light, close by */
        if (G.wisps[i].on && G.wisps[i].a > 0.5f && hypotf(G.wisps[i].x - m->x, G.wisps[i].y - m->y) < 3.0f) wraith_back(m);
    if (m->sub == 1 && m->sub_t > WRAITH_BACK_T) m->sub = 0;
    if (m->sub == 0 && !G.dead && G.wwalk_t <= 0.0f && pd < ATTACKS[FA_WTOUCH].reach && m->cool <= 0.0f) {  /* its touch */
        m->cool = ATTACKS[FA_WTOUCH].cool;
        sfx_at(SFX_SHRIEK, m->x, m->y, 1.0f);
        if (foe_hit(m, FA_WTOUCH, 1.0f) > 0) {
            G.chill = 1.0f;
            if (af_off()) G.slow_t = fmaxf(G.slow_t, ATTACKS[FA_WTOUCH].slow * (1.0f - gear_slow_resist(&RUN.gear)));
            else if (af_can(AF_CHILL) && af_rolls(AF_CHILL, 1.0f)) af_chill(2, 1 + (int)(m - G.mobs));   /* its cold: two stacks (S9) */
            DBG_COUNT(wraith_touches);
        }
    }
    if (rnd(900) == 0) { sfx_at(SFX_WAIL, m->x, m->y, 0.9f); wraith_chill(m); }
    if ((m->tx != m->cx || m->ty != m->cy) && wraith_barred(m->tx, m->ty) && !wraith_barred(m->cx, m->cy)) {
        m->tx = m->cx; m->ty = m->cy;                              /* a lamp lit while it stepped in: back the way it came */
    }
    float gx, gy; cell_pass(m->tx, m->ty, &gx, &gy);
    float dx = gx - m->x, dy = gy - m->y, d = sqrtf(dx * dx + dy * dy);
    float step = m->speed * dt * (m->sub == 1 ? 1.6f : 1.0f);
    if (lantern_lit() && m->sub == 0 && pd < 2.1f && hypotf(gx - G.px, gy - G.py) < pd) return;   /* the Lantern: it keeps a cell further off */
    if (d > step) { m->x += dx / d * step; m->y += dy / d * step; return; }
    m->x = gx; m->y = gy; m->cx = m->tx; m->cy = m->ty;
    if (G.dead || G.exiting) return;
    if (ward_step_out(m)) return;                                  /* a soul-lamp lit with it inside: out (1.0.0b18.1) */
    int here = wraith_h(m->cx, m->cy), best = -1; float bs = -1e9f;   /* (the ward's field while a lamp burns) */
    for (int k = 0; k < 4; k++) {
        int nx = m->cx + DX[k], ny = m->cy + DY[k], h = wraith_h(nx, ny);
        if (h < 0 || occupied(nx, ny, m) || wraith_barred(nx, ny)) continue;
        if (here >= 0 && (m->sub == 1 ? h <= here : h >= here)) continue;   /* down the field coming, up it going */
        float s = rndf() * 0.3f + (here < 0 ? -hypotf(nx + 0.5f - G.px, ny + 0.5f - G.py) : 0.0f);
        if (s > bs) { bs = s; best = k; }
    }
    if (best < 0) { wraith_cornered(m, dt); return; }
    int nx = m->cx + DX[best], ny = m->cy + DY[best];
    const Cell *c = cell(nx, ny);
    if (c->kind == K_DOOR && G.doors[c->door].open < 0.9f) {        /* a door opening by itself: it is coming */
        Door *dr = &G.doors[c->door];
        if (dr->locked == 1 || dr->locked >= SEALED) { wraith_cornered(m, dt); return; }   /* one it can't open */
        if (!dr->opening) dr->opening = 1;                          /* its sound in update_doors, by the run */
        return;
    }
    m->corner_t = 0.0f;
    m->tx = nx; m->ty = ny;
}

/* the buried and the drowned: under the earth of a grave or in a puddle, until you come near */
static int buried(const Mob *m) { return (m->type == MOB_SHAMBLER || m->type == MOB_DROWNED) && m->sub == 1; }
static int update_riser(Mob *m, float dt) {                       /* 1 while it is not yet up and about */
    if (m->state != MS_WALK || !m->sub || (m->type != MOB_SHAMBLER && m->type != MOB_DROWNED)) return 0;
    m->t += dt;
    if (m->sub == 1) {
        float d = hypotf(G.px - m->x, G.py - m->y);
        if (m->amb && m->amb <= G.namb && !G.amb[m->amb - 1].sprung) return 1;   /* an ambush's: it rises when it springs */
        int stirred = m->sub_t > 0.0f && (m->sub_t -= dt) <= 0.0f;    /* the wake: another of it rose, and now this one */
        if (m->pack && d < 9.0f && rnd(700) == 0) sfx_at(m->type == MOB_DROWNED ? SFX_DRIP : SFX_DIRT, m->x, m->y, 0.3f);   /* where nothing moves */
        if (stirred || (!G.dead && G.level_t > 1.5 && d < (m->type == MOB_DROWNED ? 3.5f : 4.5f) && line_clear(m->x, m->y, G.px, G.py))) {
            if (!stirred) wake_stir(m);
            m->sub = 2; m->sub_t = 0.0f;
            sfx_at(m->type == MOB_DROWNED ? SFX_DRIP : SFX_DIRT, m->x, m->y, 1.4f); sfx_at(SFX_GROAN, m->x, m->y, 1.0f);
            if (m->deco >= 0) G.decos[m->deco].spr = m->type == MOB_DROWNED ? SP_PUDDLE_A : SP_PIT;   /* the earth opened, the water stilled */
        }
        return 1;
    }
    if ((m->sub_t += dt) >= 1.6f) { m->sub = 0; m->alert = 12.0f; }
    return 1;
}

/* where a drowned one burst: a foul pool that poisons for a few seconds */
static void add_pool(float x, float y) {
    for (int i = 0; i < MAXPOOLS; i++) if (!G.pools[i].alive) { G.pools[i] = (Pool){ 1, x, y, 0.0f, 1 }; return; }
}
static void update_pools(float dt) {
    for (int i = 0; i < MAXPOOLS; i++) {
        Pool *p = &G.pools[i];
        if (!p->alive) continue;
        if ((p->t += dt) > 5.0f) { p->alive = 0; if (p->feat) por_bump(MOB_DROWNED); continue; }   /* NOT A DROP */
        if (!G.dead && hypotf(G.px - p->x, G.py - p->y) < 0.9f) p->feat = 0;
        if (!G.dead && G.purify_t <= 0.0f && hypotf(G.px - p->x, G.py - p->y) < 0.9f) {
            float k = 1.0f - gear_poison_resist(&RUN.gear);
            float was = G.poison_t;
            G.poison_t = fmaxf(G.poison_t, 2.5f * k); G.poison_by = (Blame){ BL_POOL, MOB_DROWNED, 0 };
            af_poison(G.poison_t - was);                           /* Poisoned while you stand in it */
            G.poison_dps = fmaxf(G.poison_dps, (1.0f + (float)(G.level > 4 ? (G.level - 4) / 2 : 0)) * (0.5f + 0.5f * k));
        }
    }
}

/* The wall skull: asleep until the caster comes close, then a scream and three seconds of fire
 * gathering, then a fireball; again and again while the caster stays near. One hit ends it. */
static void update_wskull(Mob *m, float dt) {
    m->t += dt;
    if (m->state == MS_DIE) {                                      /* falls from the wall and breaks */
        m->z = fmaxf(0.0f, m->z - dt * 1.6f);
        if (m->t >= 0.55f) { add_deco(m->x, m->y, SP_BONES1); m->alive = 0; }
        return;
    }
    dt = mob_time(m, dt);
    m->sub_t += dt;
    if (m->feared > 0.0f) { m->feared -= dt; m->sub = 0; m->sub_t = 0.0f; return; }   /* Dread: it cowers silent */
    float d = hypotf(G.px - m->x, G.py - m->y);
    int seen = !G.dead && !G.exiting && G.wwalk_t <= 0.0f && line_clear(m->x, m->y, G.px, G.py);
    switch (m->sub) {
    case 0: {                                                      /* asleep, until the caster walks up to it, eyes on it */
        float vx = (G.px - m->x) / (d + 1e-4f), vy = (G.py - m->y) / (d + 1e-4f);
        int facing = vx * cosf(m->face) + vy * sinf(m->face) > 0.6f && -(vx * cosf(G.ang) + vy * sinf(G.ang)) > 0.8f;
        if (seen && facing && d < 3.0f) {
            m->sub = 1; m->sub_t = 0.0f; sfx_at(SFX_SCREAM, m->x, m->y, 1.5f); DBG_COUNT(wskull_wakes);
#ifdef CRAWL_DEBUG
            fprintf(stderr, "   skull at (%.2f,%.2f) caster (%.2f,%.2f) ang %.2f\n", m->x, m->y, G.px, G.py, G.ang);
#endif
            m->ignore = (float)G.t + 0.4f + 1.2f * rndf() + (rndf() < 0.2f ? 1.6f : 0.0f);   /* startled */
            G.kick = 0.05f; G.chill = fmaxf(G.chill, 0.35f);
        }
        break;
    }
    case 1:                                                        /* the fire gathers: three seconds */
        if (m->sub_t >= 0.6f) alarm_at(m, 10);                     /* its scream down the halls: the watch (docs/PLAN-ai.md, 10) */
        if (m->sub_t >= ATTACKS[FA_FIREBALL].wind) {
            if (seen && d < ATTACKS[FA_FIREBALL].go2) throw_missile(m, MIS_FIRE);
            m->sub = 2; m->sub_t = 0.0f;
        }
        break;
    default:                                                       /* spent; it gathers again if you linger */
        if (m->sub_t >= ATTACKS[FA_FIREBALL].cool) {
            m->sub = seen && d < 4.5f ? 1 : 0; m->sub_t = 0.0f;   /* awake now: it needs no second look */
            if (m->sub == 1) sfx_at(SFX_SCREAM, m->x, m->y, 0.9f);
        }
        break;
    }
}

/* The spider: hidden in its corner web with only its eyes showing; it drops on its thread when the
 * caster comes within two steps, bites (poison and a slow), and climbs back to wait. */
static void bite(Mob *m) {
    const Gear *g = &RUN.gear;
    sfx_at(SFX_SKITTER, m->x, m->y, 1.4f); snd_sfx_ex(SFX_HURT, 0.7f, 0.5f);
    G.face_t = FACE_T; G.face_side = 0; G.hurt = fmaxf(G.hurt, 0.6f);
    thorns(m);
    DBG_COUNT(bites);
    if (G.purify_t > 0.0f) return;                                  /* Purify's protection */
    float k = 1.0f - gear_poison_resist(g);                        /* resisted, it is shorter and weaker */
    G.poison_t = fmaxf(G.poison_t, 6.0f * k); G.poison_by = (Blame){ BL_POISON, (uint8_t)m->type, (uint8_t)m->rank };
    af_poison(6.0f * k);                                           /* Poisoned: each bite adds its time (S5, S9) */
    G.poison_dps = (1.0f + (float)((G.level - 4) / 2 > 0 ? (G.level - 4) / 2 : 0)) * m->dmgk * (0.5f + 0.5f * k);
    G.slow_t = fmaxf(G.slow_t, ATTACKS[FA_WEB].slow * (1.0f - gear_slow_resist(g)));
    if (m->type == MOB_SPIDER) {                                   /* an elemental one's bite, one in three, carries it too (C3) */
        int e = elem_take(m);
        if (e >= 0) { G.blame = blame_of(m); af_struck(e, G.blame, mob_damage(m), 1 + (int)(m - G.mobs), 1.0f); DBG_COUNT(elem_hits); }
    }
}

static void update_spider(Mob *m, float dt) {
    m->t += dt;
    if (m->state == MS_DIE) {
        m->z = fmaxf(0.0f, m->z - dt * 1.4f);
        if (m->t >= 0.9f) { add_deco(m->x, m->y, SP_SPIDER_D); m->alive = 0; }
        return;
    }
    dt = mob_time(m, dt);
    m->sub_t += dt; m->cool -= dt;
    if (m->feared > 0.0f) m->feared -= dt;                         /* Exquisite Dread: it keeps to its web */
    float dax = G.px - m->ax, day = G.py - m->ay, da = sqrtf(dax * dax + day * day) + 1e-4f;
    float reach = fminf(da - 0.45f, 0.75f);                         /* how far out from the corner it drops */
    switch (m->sub) {
    case 0:
        m->x = m->ax; m->y = m->ay; m->z = 0.62f;
        if (!G.dead && G.wwalk_t <= 0.0f && m->cool <= 0.0f && m->feared <= 0.0f && da < ATTACKS[FA_WEB].go && line_clear(m->ax, m->ay, G.px, G.py)) { m->sub = 1; m->sub_t = 0.0f; m->feat |= FT_DROPPED; elem_arm(m, 1.0f); sfx_at(SFX_SKITTER, m->x, m->y, 1.0f); DBG_COUNT(spider_drops);
            if (m->pack) { pack_call(m); sfx_at(SFX_THRUM, m->ax, m->ay, 1.1f); DBG_COUNT(nest_calls); } }   /* a nest's tripwire: the web thrums */
        break;
    case 1: {                                                      /* down the thread */
        float k = fminf(1.0f, m->sub_t / 0.35f);
        m->z = lerpf(0.62f, 0.12f, k);
        m->x = m->ax + dax / da * reach * k; m->y = m->ay + day / da * reach * k;
        if (k >= 1.0f) { m->sub = 2; m->sub_t = 0.0f; }
        break;
    }
    case 2:                                                        /* the bite */
        if (m->sub_t >= ATTACKS[FA_WEB].wind) {
            if (!G.dead && G.wwalk_t <= 0.0f && hypotf(G.px - m->x, G.py - m->y) < ATTACKS[FA_WEB].reach) bite(m);
            m->sub = 3; m->sub_t = 0.0f;
            m->bx = m->x; m->by = m->y;                             /* where it climbs back from */
        }
        break;
    default: {                                                     /* back up to the web */
        float k = fminf(1.0f, m->sub_t / 1.2f);
        m->z = lerpf(0.12f, 0.62f, k);
        m->x = lerpf(m->bx, m->ax, k); m->y = lerpf(m->by, m->ay, k);
        if (k >= 1.0f) { m->sub = 0; m->cool = ATTACKS[FA_WEB].cool; }
        break;
    }
    }
    m->cx = m->tx = (int)m->x; m->cy = m->ty = (int)m->y;
}

/* A leap at the caster, for hunting spiders and ghouls: from where it is to just short of where the
 * caster stands, in an arc; a blow, or a bite, if it lands close. */
static void start_leap(Mob *m) {
    float a = atan2f(G.py - m->y, G.px - m->x), d = fmaxf(0.0f, hypotf(G.px - m->x, G.py - m->y) - 0.7f);
    m->sub = 3; m->sub_t = 0.0f;
    m->bx = m->x; m->by = m->y; m->ax = m->x + cosf(a) * d; m->ay = m->y + sinf(a) * d;
    elem_arm(m, ATTACKS[m->type == MOB_HUNTER ? FA_HLEAP : m->type == MOB_GARGOYLE ? FA_SWOOP : FA_GLEAP].wind);
    sfx_at(m->type == MOB_GHOUL ? SFX_GROAN : (m->type == MOB_GARGOYLE ? SFX_WHOOSH : SFX_SKITTER), m->x, m->y, 1.2f);
}
static int update_leap(Mob *m, float dt) {
    if (m->sub != 3) return 0;
    m->sub_t += dt;
    int garg = m->type == MOB_GARGOYLE, fa = m->type == MOB_HUNTER ? FA_HLEAP : garg ? FA_SWOOP : FA_GLEAP;
    float k = fminf(1.0f, m->sub_t / ATTACKS[fa].wind);
    m->x = lerpf(m->bx, m->ax, k); m->y = lerpf(m->by, m->ay, k); m->z = garg ? GARG_Z - 0.1f * sinf(PI_F * k) : 0.35f * sinf(PI_F * k);
    if (k < 1.0f) return 1;
    m->sub = m->type == MOB_HUNTER || garg ? 2 : 0; m->z = garg ? GARG_Z : 0.0f; m->cool = ATTACKS[fa].cool;
    m->cx = m->tx = (int)m->x; m->cy = m->ty = (int)m->y;
    if (!G.dead && G.wwalk_t <= 0.0f && hypotf(G.px - m->x, G.py - m->y) < ATTACKS[fa].reach) {
        if (foe_hit(m, fa, 1.0f) > 0) thorns(m);
        if (m->type == MOB_HUNTER && rnd(3) == 0) bite(m);         /* now and then the fangs go in */
        DBG_COUNT(leaps);
    }
    return 1;
}

/* an egg sac: it waits while the caster is far, then stirs, and splits into spiderlings */
static void update_sac(Mob *m, float dt) {
    m->t += dt;
    if (m->state == MS_DIE) { if (m->t >= 0.4f) { add_deco(m->x, m->y, SP_SAC_OPEN); m->alive = 0; } return; }
    if (hypotf(G.px - m->x, G.py - m->y) < 10.0f) m->sub_t += dt;
    if (m->sub_t < 8.0f) return;
    for (int k = 0; k < (m->small ? 4 : 3); k++) {                  /* it hatches; the Wandering Spider's, small hunters */
        Mob *l = spawn_mob(m->cx, m->cy, m->small ? MOB_HUNTER : MOB_LING);
        if (!l) break;
        l->x = m->x + (rndf() - 0.5f) * 0.6f; l->y = m->y + (rndf() - 0.5f) * 0.6f; l->alert = 30.0f;
        set_rank(l, m->rank < RK_V ? m->rank : RK_V);              /* their mother's colour */
        if (m->small) { l->small = 1; l->hp = l->maxhp = l->maxhp * 0.5f; l->sub = 2; l->home = -1; }
    }
    sfx_at(SFX_SKITTER, m->x, m->y, 1.4f);
    add_deco(m->x, m->y, SP_SAC_OPEN);
    m->alive = 0;
    if (!m->small) {                                               /* the nearest mother's: NONE HATCHED is spoilt for her (the portraits') */
        Mob *mo = NULL; float bd = 1e9f;
        for (int i = 0; i < MAXMOBS; i++) {
            Mob *o = &G.mobs[i];
            if (o->alive && o->state == MS_WALK && o->type == MOB_BROOD && hypotf(o->x - m->x, o->y - m->y) < bd) { bd = hypotf(o->x - m->x, o->y - m->y); mo = o; }
        }
        if (mo) mo->feat |= FT_HATCHED;
    }
}

/* A shade shows only in light: torches, fires, the wisp's pool, a spell going by. For a moment after
 * its touch the cold of it shows too. */
static float shade_alpha(const Mob *m) {
    if (m->state != MS_WALK) return 0.8f;
    float a = smoothstepf(0.3f, 0.6f, lm_at(m->x, m->y));
    return fmaxf(a, m->swing > 0.0f || m->sub_t > 0.0f ? 0.75f : 0.0f);
}
static int shade_hidden(const Mob *m) { return m->type == MOB_SHADE && shade_alpha(m) < 0.5f; }
static int shade_may_strike(const Mob *m) {                       /* you stand still, or fight something else, or something has you */
    if (G.still_t > 1.5f || G.latch) return 1;
    for (int i = 0; i < MAXMOBS; i++) {
        const Mob *o = &G.mobs[i];
        if (o != m && o->alive && o->state == MS_WALK && o->type != MOB_SHADE && (o->engage || o->boss || o->add) && hypotf(o->x - G.px, o->y - G.py) < 6.0f) return 1;
    }
    return 0;
}

/* a gargoyle wakes: the statue moves, and its plinth is floor again */
static void garg_wake(Mob *m) {
    if (m->sub != 0) return;
    m->sub = 1; m->sub_t = 0.0f;
    m->feat_t = (float)G.level_t + 1e-3f;                          /* when (the portraits' STILL STONE) */
    sfx_at(SFX_DIRT, m->x, m->y, 1.2f);
    if (G.map[m->cy][m->cx].kind == K_PROP) G.map[m->cy][m->cx].kind = K_FLOOR;
    DBG_COUNT(garg_wakes);
    static int waking;                                             /* sentinels: the others within twelve cells wake with it */
    const Pack *p = pack_of(m);
    if (!p || p->kind != PACK_SENTINELS || waking) return;
    waking = 1;
    for (int i = 0; i < MAXMOBS; i++) {
        Mob *o = &G.mobs[i];
        if ((p->mem >> i & 1) && o != m && o->alive && o->sub == 0 && hypotf(o->x - m->x, o->y - m->y) < 12.0f) { garg_wake(o); o->sub_t = -0.3f * rndf(); }
    }
    waking = 0;
    DBG_COUNT(sentinels_woke);
}
/* what stops a flier: walls and shut doors, not the things on the floor */
static int blocks_flight(int x, int y) { return cell(x, y)->kind != K_PROP && blocks_move(x, y); }

/* The Ill-Fated Knight's Swift Dodge (docs/PLAN-b7.md): half a second untouchable, darting two to five
 * cells along clear floor, with after-images, a whoosh and a clank of plate. Three reasons, three waits:
 * to close in when you are four cells off and in its sight (5 s), to get away when it has lost a tenth of
 * its health lately or stands in fire (9 s), to step aside from a shot about to strike it (16 s). Never
 * while a wave holds it: the hold comes first. */
static int knight_blocked(float x, float y) {
    for (int k = 0; k < 4; k++)
        if (blocks_move((int)(x + ((k & 1) ? 0.28f : -0.28f)), (int)(y + ((k & 2) ? 0.28f : -0.28f)))) return 1;
    return 0;
}
static float dash_room(const Mob *m, float a, float most) {
    float d = 0.0f, ca = cosf(a), sa = sinf(a);
    while (d + 0.25f <= most && in_arena(m->x + ca * (d + 0.25f), m->y + sa * (d + 0.25f)) && !knight_blocked(m->x + ca * (d + 0.25f), m->y + sa * (d + 0.25f)))
        d += 0.25f;
    return d;
}
static void knight_dash(Mob *m, float a, float dist, const char *why) {
    G.bdash_t = 0.5f; G.bdash_age = 0.0f;
    G.bdash_x0 = G.bdash_x1 = m->x; G.bdash_y0 = G.bdash_y1 = m->y;
    G.bdash_vx = cosf(a) * dist / 0.5f; G.bdash_vy = sinf(a) * dist / 0.5f;
    m->swing = 0.0f;
    sfx_at(SFX_WHOOSH, m->x, m->y, 0.9f); sfx_at(SFX_SHATTER, m->x, m->y, 0.3f);
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT knight_dash %s %.1f cells t=%.2f\n", why, dist, G.t);
#else
    (void)why;
#endif
}
static int knight_boss(Mob *m, float dt, float pd0, int sees) {
    for (int k = 0; k < 3; k++) if (G.bdash_cd[k] > 0.0f) G.bdash_cd[k] -= dt;
    G.bhurt *= expf(-dt / 3.0f);
    if (G.bdash_t > 0.0f) {                                        /* on its way */
        float nx = m->x + G.bdash_vx * dt, ny = m->y + G.bdash_vy * dt;
        G.bdash_t -= dt;
        if (!in_arena(nx, ny) || knight_blocked(nx, ny)) G.bdash_t = 0.0f;
        else { m->x = nx; m->y = ny; }
        m->cx = m->tx = (int)m->x; m->cy = m->ty = (int)m->y;
        G.bdash_x1 = m->x; G.bdash_y1 = m->y;
        m->face = atan2f(G.py - m->y, G.px - m->x);
        return 1;
    }
    if (m->swing > 0.0f || G.dead) return 0;
    int burning = 0;
    for (int i = 0; i < MAXBURNS; i++) burning |= G.burns[i].alive && !G.burns[i].foe && hypotf(G.burns[i].x - m->x, G.burns[i].y - m->y) < 0.7f;
    if (G.bdash_cd[1] <= 0.0f && (burning || G.bhurt >= m->maxhp * 0.1f)) {   /* away, as far as it can */
        static const float TURN[5] = { 0.0f, 0.6f, -0.6f, 1.2f, -1.2f };
        float away = atan2f(m->y - G.py, m->x - G.px), best = 0.0f, ba = away;
        for (int k = 0; k < 5; k++) { float r = dash_room(m, away + TURN[k], 5.0f); if (r > best + 0.5f) { best = r; ba = away + TURN[k]; } }
        if (best >= 2.0f) { G.bdash_cd[1] = 9.0f; G.bhurt = 0.0f; knight_dash(m, ba, best, "flee"); return 1; }
    }
    if (G.bdash_cd[2] <= 0.0f)                                     /* a shot about to strike: aside */
        for (int i = 0; i < MAXSHOTS; i++) {
            const Shot *sh = &G.shots[i];
            if (!sh->alive || sh->tier < 0) continue;
            float sp = hypotf(sh->vx, sh->vy) + 1e-3f, dx = m->x - sh->x, dy = m->y - sh->y;
            float along = (dx * sh->vx + dy * sh->vy) / sp, off = fabsf(dx * sh->vy - dy * sh->vx) / sp;
            if (along <= 0.0f || along > 3.5f || off > 0.7f) continue;
            float a = atan2f(sh->vy, sh->vx), l = dash_room(m, a + PI_F * 0.5f, 3.0f), r = dash_room(m, a - PI_F * 0.5f, 3.0f);
            if (fmaxf(l, r) < 2.0f) break;
            G.bdash_cd[2] = 16.0f;
            knight_dash(m, l >= r ? a + PI_F * 0.5f : a - PI_F * 0.5f, fmaxf(l, r), "dodge");
            return 1;
        }
    if (G.bdash_cd[0] <= 0.0f && sees && pd0 >= 4.0f) {             /* to a cell short of you, ready to strike */
        float a = atan2f(G.py - m->y, G.px - m->x), r = dash_room(m, a, fminf(5.0f, pd0 - 1.2f));
        if (r >= 2.0f) { G.bdash_cd[0] = 5.0f; knight_dash(m, a, r, "close"); m->cool = 0.0f; return 1; }
    }
    return 0;
}

/* Its Mirror Rune: one missile in seven that strikes it goes back at you, never two within two seconds,
 * at half the spell's damage and slower, so it can be dodged; its plate flashes silver in a ring and a
 * struck bell rings. Chain Lightning, Soul Drain and the BFS9000 aren't missiles. */
static int knight_turns(Mob *mb, Shot *s) {
    if (mb->boss != 1 + BOSS_KNIGHT || G.bstate != BS_FIGHT || G.bmirror_cd > 0.0f || s->tier < 0) return 0;
    if (s->kind != AT_FIRE && s->kind != AT_FROST && s->kind != AT_SPEAR && s->kind != AT_HELL && s->kind != AT_WISPS) return 0;   /* a flask goes over it */
    if (rnd(7)) return 0;
    G.bmirror_cd = 2.0f; G.bmirror_t = 0.35f;
    s->alive = 0;
    for (int i = 0; i < MAXBONES; i++) {
        Bone *b = &G.bones[i];
        if (b->alive) continue;
        float a = atan2f(G.py - mb->y, G.px - mb->x), sp = fminf(hypotf(s->vx, s->vy) * 0.8f, 6.0f);
        *b = (Bone){ 1, MIS_TURNED, mb->x + cosf(a) * 0.5f, mb->y + sinf(a) * 0.5f, cosf(a) * sp, sinf(a) * sp, 4.0f, 0.0f,
                     fminf(s->dmg * 0.5f, (float)G.max_hp * 0.2f), 0.0f, (int)(mb - G.mobs), s->kind, { BL_MIRROR, 0, 0 },
                     s->kind >= 0 && s->kind < NATK ? 1 + ATK_ELEM[s->kind] : 0 };
        break;
    }
    sfx_at(SFX_MIRRORBELL, mb->x, mb->y, 1.0f);
    DBG_COUNT(knight_turned);
    return 1;
}

/* The Wandering Spider (docs/PLAN-b7.md): up to the ceiling every four to seven seconds and across it,
 * out of reach, to a spot two cells short of you, then down and a leap. Its Mend Rune mends an eighth of
 * its health over four seconds once it has lost a fifth since it last mended, at most every twenty and
 * never past the last wave mark, glowing green. Its Tangled Web: it rears with a hiss and spits a glob
 * that can be sidestepped; if it lands you can't walk until your spells have torn a third of your health
 * out of it (fire twice over); the next comes 12 s after you are free. */
static int web_flying(void) {
    for (int i = 0; i < MAXBONES; i++) if (G.bones[i].alive && G.bones[i].kind == MIS_WEB) return 1;
    return 0;
}
static int spider_boss(Mob *m, float dt, float pd0, int sees) {
    float top = 1.0f - KINDS[MOB_HUNTER].scale * BOSSES[BOSS_SPIDER].scale;
    if (G.bmend_base > m->maxhp) G.bmend_base = m->maxhp;
    if (G.bmend_cd > 0.0f) G.bmend_cd -= dt;
    if (G.bmend_t > 0.0f) {
        float cap = G.bwave > 0 ? m->maxhp * WAVE_MARK[G.bwave - 1] : m->maxhp;
        m->hp = fminf(fmaxf(cap, m->hp), m->hp + m->maxhp * 0.125f / 4.0f * dt);
        if ((G.bmend_t -= dt) <= 0.0f) G.bmend_base = m->hp;
    } else if (G.bmend_cd <= 0.0f && m->hp <= G.bmend_base - 0.2f * m->maxhp) {
        G.bmend_t = 4.0f; G.bmend_cd = 20.0f;
        sfx_at(SFX_RUNE, m->x, m->y, 1.2f); sfx_at(SFX_POTION, m->x, m->y, 0.8f);
        DBG_COUNT(spider_mends);
    }
    if (m->sub == 3 || m->sub == 1) return 0;                      /* a leap, or the drop: the hunter's own */
    if (m->sub == 5) {                                             /* up to the ceiling */
        m->sub_t += dt;
        m->z = top * fminf(1.0f, m->sub_t / 0.4f);
        if (m->sub_t >= 0.4f) { m->sub = 0; m->sub_t = 0.0f; }
        return 1;
    }
    if (m->sub == 0) {                                             /* across it, to two cells short of you, and down */
        m->z = top; m->sub_t += dt; m->t += dt;
        float d = fmaxf(pd0, 0.01f), gx = G.px + (m->x - G.px) / d * 2.0f, gy = G.py + (m->y - G.py) / d * 2.0f;
        float vx = gx - m->x, vy = gy - m->y, vd = hypotf(vx, vy), step = m->speed * 1.4f * dt;
        if (vd > 0.3f) {
            float nx = m->x + vx / vd * fminf(step, vd), ny = m->y + vy / vd * fminf(step, vd);
            if (in_arena(nx, ny) && !knight_blocked(nx, ny)) { m->x = nx; m->y = ny; m->cx = m->tx = (int)nx; m->cy = m->ty = (int)ny; }
            else vd = 0.0f;
        }
        if ((vd <= 0.3f || m->sub_t > 3.0f) && G.bstate == BS_FIGHT) { m->sub = 1; m->sub_t = 0.0f; m->cool = 0.0f; sfx_at(SFX_SKITTER, m->x, m->y, 1.3f); }
        return 1;
    }
    if (G.web_rear > 0.0f) {                                       /* rearing: then the glob */
        m->face = atan2f(G.py - m->y, G.px - m->x);
        if ((G.web_rear -= dt) <= 0.0f)
            for (int i = 0; i < MAXBONES; i++) {
                Bone *b = &G.bones[i];
                if (b->alive) continue;
                float a = atan2f(G.py - m->y, G.px - m->x);
                *b = (Bone){ 1, MIS_WEB, m->x + cosf(a) * 0.5f, m->y + sinf(a) * 0.5f, cosf(a) * 3.5f, sinf(a) * 3.5f, 4.0f, 0.0f, 0.0f, 0.0f, (int)(m - G.mobs), 0, blame_of(m), 0 };
                sfx_at(SFX_THROW, m->x, m->y, 1.0f);
                break;
            }
        return 1;
    }
    if (m->swing > 0.0f || G.dead) return 0;
    if (G.web_hp <= 0.0f && !web_flying() && (G.web_cd -= dt) <= 0.0f && sees && pd0 > 2.5f && pd0 < 9.0f) {
        G.web_rear = 0.6f; G.web_cd = 12.0f;
        sfx_at(SFX_SKITTER, m->x, m->y, 1.5f); sfx_at(SFX_WHISPER, m->x, m->y, 0.6f);   /* a hiss */
        return 1;
    }
    if ((G.bsp_t -= dt) <= 0.0f) {                                 /* up it goes */
        G.bsp_t = 4.0f + 3.0f * rndf();
        m->sub = 5; m->sub_t = 0.0f;
        sfx_at(SFX_SKITTER, m->x, m->y, 1.1f);
        DBG_COUNT(spider_ups);
        return 1;
    }
    return 0;
}
/* a spell cast while held strikes the web instead; fire tears it twice as fast */
static int web_strike(int sp, float dmg) {
    if (G.web_hp <= 0.0f) return 0;
    G.web_hp -= dmg * (sp == AT_FIRE || sp == AT_HELL ? 2.0f : 1.0f);
    G.web_idle = 0.0f;
    sfx_at(sp == AT_FIRE || sp == AT_HELL ? SFX_IGNITE : SFX_SKITTER, G.px, G.py, 0.8f);
    if (G.web_hp <= 0.0f) { G.web_hp = 0.0f; G.web_cd = 12.0f; snd_sfx_ex(SFX_SHATTER, 0.6f, 0.5f); DBG_COUNT(web_torn); }
    return 1;
}
/* Egg Shot: a sac stuck to the stone in sight of you, which hatches four small spiders in five seconds
 * unless it is burned or broken first */
static void egg_shot(const Mob *m) {
    for (int tries = 0; tries < 60; tries++) {
        float a = rndf() * TAU_F, d = 2.0f + 4.0f * rndf();
        int x = (int)floorf(G.px + cosf(a) * d), y = (int)floorf(G.py + sinf(a) * d), wall = 0;
        if (!in_arena(x + 0.5f, y + 0.5f) || G.map[y][x].kind != K_FLOOR || occupied(x, y, NULL)) continue;
        for (int k = 0; k < 4; k++) wall |= G.map[y + DY[k]][x + DX[k]].kind == K_WALL;
        if (!wall || !line_clear(G.px, G.py, x + 0.5f, y + 0.5f)) continue;
        Mob *e = spawn_mob(x, y, MOB_SAC);
        if (!e) return;
        e->small = 1; e->sub_t = 3.0f; e->rank = m->rank; e->alert = 30.0f;
        sfx_at(SFX_THROW, m->x, m->y, 1.0f); sfx_at(SFX_SKITTER, e->x, e->y, 1.0f);
        DBG_COUNT(egg_shots);
        return;
    }
}

/* The Wraith Unbound (docs/PLAN-b7.md): here it can be hurt, but only in light, from its braziers or your
 * wisp; it puts the braziers out as it passes, and a void over one does too; E kindles one again in a
 * second, and a wisp close by does it at once. Its Time Slip Rune slows your missiles near it with a grey
 * ripple and a deep chime while it steps aside; Wraithwalk takes it into the stone and out of a wall
 * near you. */
static int wraith_lit(float x, float y) {
    for (int b = 0; b < G.bbz_n; b++) if (G.bbz_lit[b] && hypotf(G.bbz_x[b] + 0.5f - x, G.bbz_y[b] + 0.5f - y) < 4.0f) return 1;
    for (int i = 0; i < MAXWISPS; i++) if (G.wisps[i].on && G.wisps[i].a > 0.5f && hypotf(G.wisps[i].x - x, G.wisps[i].y - y) < 3.0f) return 1;
    return 0;
}
static int wraith_open(const Mob *m) { return m->type != MOB_WRAITH || !m->boss || (!G.bww && wraith_lit(m->x, m->y)); }
static void brazier_set(int b, int lit) {
    if (G.bbz_lit[b] == lit) return;
    G.bbz_lit[b] = lit;
    Deco *dc = &G.decos[G.bbz_deco[b]];
    dc->spr = lit ? SP_BRAZIER_A : SP_BRAZIER_OUT; dc->nfr = lit ? 3 : 1;
    sfx_at(lit ? SFX_IGNITE : SFX_WHOOSH, G.bbz_x[b] + 0.5f, G.bbz_y[b] + 0.5f, lit ? 1.0f : 0.6f);
#ifdef CRAWL_DEBUG
    if (lit) DBG_COUNT(brazier_lit); else DBG_COUNT(brazier_out);
#endif
}
static int brazier_facing(void) {                                   /* an unlit brazier before the dweller */
    for (int b = 0; b < G.bbz_n; b++) {
        float d = hypotf(G.bbz_x[b] + 0.5f - G.px, G.bbz_y[b] + 0.5f - G.py);
        if (!G.bbz_lit[b] && d < 1.8f && fabsf(wrap_pi(atan2f(G.bbz_y[b] + 0.5f - G.py, G.bbz_x[b] + 0.5f - G.px) - G.ang)) < 0.8f) return b;
    }
    return -1;
}
static void wraith_boss(Mob *m, float dt) {
    m->t += dt;
    dt = mob_time(m, dt);
    if (dt <= 0.0f) return;
    m->anim += dt * 2.0f; m->cool -= dt; m->sub = 0;               /* nothing drives it back here */
    if (G.bgutter_cd > 0.0f) G.bgutter_cd -= dt;
    for (int b = 0; b < G.bbz_n && G.bgutter_cd <= 0.0f; b++)      /* it gutters a brazier it passes close by, one in eight seconds */
        if (G.bbz_lit[b] && !G.bww && hypotf(G.bbz_x[b] + 0.5f - m->x, G.bbz_y[b] + 0.5f - m->y) < 1.05f) { brazier_set(b, 0); G.bgutter_cd = 8.0f; }
    float pd = hypotf(G.px - m->x, G.py - m->y);
    if (G.bww) {                                                   /* Wraithwalk */
        G.bww_t += dt;
        if (G.bww == 1 && G.bww_t >= 0.6f) { G.bww = 2; G.bww_t = 0.0f; }
        else if (G.bww == 2 && G.bww_t >= 1.5f) {                  /* out of a wall near you */
            for (int tries = 0; tries < 60; tries++) {
                float a = rndf() * TAU_F, d = 2.0f + 1.5f * rndf();
                int x = (int)floorf(G.px + cosf(a) * d), y = (int)floorf(G.py + sinf(a) * d), wall = 0;
                if (!in_arena(x + 0.5f, y + 0.5f) || G.map[y][x].kind != K_FLOOR || occupied(x, y, m)) continue;
                for (int k = 0; k < 4; k++) wall |= G.map[y + DY[k]][x + DX[k]].kind == K_WALL;
                if (!wall) continue;
                m->x = x + 0.5f; m->y = y + 0.5f; m->cx = m->tx = x; m->cy = m->ty = y;
                break;
            }
            G.bww = 3; G.bww_t = 0.0f;
            sfx_at(SFX_SHRIEK, m->x, m->y, 0.9f);
        } else if (G.bww == 3 && G.bww_t >= 0.5f) { G.bww = 0; G.bww_cd = 16.0f + 4.0f * rndf(); m->cool = fminf(m->cool, 0.3f); }
        return;
    }
    if ((G.bww_cd -= dt) <= 0.0f && pd > 2.0f && !G.dead) {
        G.bww = 1; G.bww_t = 0.0f;
        sfx_at(SFX_WAIL, m->x, m->y, 0.9f);
        DBG_COUNT(wraithwalks);
        return;
    }
    if (G.bslip_cd > 0.0f) G.bslip_cd -= dt;
    if (G.bslip_cd <= 0.0f && wraith_lit(m->x, m->y))              /* its Time Slip Rune: a missile about to strike */
        for (int i = 0; i < MAXSHOTS; i++) {
            const Shot *sh = &G.shots[i];
            if (!sh->alive || sh->tier < 0) continue;
            float sp = hypotf(sh->vx, sh->vy) + 1e-3f, dx = m->x - sh->x, dy = m->y - sh->y;
            float along = (dx * sh->vx + dy * sh->vy) / sp, off = fabsf(dx * sh->vy - dy * sh->vx) / sp;
            if (along <= 0.0f || along > 2.5f || off > 0.7f) continue;
            G.bslip_t = 1.5f; G.bslip_cd = def_cooldown(Q_RARE + 1); G.bslip_x = m->x; G.bslip_y = m->y;
            float a = atan2f(sh->vy, sh->vx) + (rnd(2) ? 0.5f : -0.5f) * PI_F;   /* and it steps out of the way */
            int nx = (int)(m->x + cosf(a) * 1.2f), ny = (int)(m->y + sinf(a) * 1.2f);
            if (in_arena(nx + 0.5f, ny + 0.5f) && G.map[ny][nx].kind == K_FLOOR) { m->tx = nx; m->ty = ny; }
            sfx_at(SFX_DEEPCHIME, m->x, m->y, 1.0f);
            DBG_COUNT(wraith_slips);
            break;
        }
    if (!G.dead && G.wwalk_t <= 0.0f && pd < ATTACKS[FA_WBOSS].reach && m->cool <= 0.0f) {   /* its touch */
        m->cool = ATTACKS[FA_WBOSS].cool;
        sfx_at(SFX_SHRIEK, m->x, m->y, 1.0f);
        if (foe_hit(m, FA_WBOSS, 1.0f) > 0) {
            G.chill = 1.0f;
            if (af_off()) G.slow_t = fmaxf(G.slow_t, ATTACKS[FA_WBOSS].slow * (1.0f - gear_slow_resist(&RUN.gear)));
            else if (af_can(AF_CHILL) && af_rolls(AF_CHILL, 1.0f)) af_chill(2, 1 + (int)(m - G.mobs));
            DBG_COUNT(wraith_touches);
        }
    }
    float gx = m->tx + 0.5f, gy = m->ty + 0.5f, dx = gx - m->x, dy = gy - m->y, d = sqrtf(dx * dx + dy * dy), step = m->speed * dt;
    if (d > step) { m->x += dx / d * step; m->y += dy / d * step; return; }
    m->x = gx; m->y = gy; m->cx = m->tx; m->cy = m->ty;
    if (G.dead) return;
    int best = -1; float bs = 1e9f;
    for (int k = 0; k < 4; k++) {                                  /* straight for you, round what stands in the way */
        int nx = m->cx + DX[k], ny = m->cy + DY[k];
        if (!in_arena(nx + 0.5f, ny + 0.5f) || blocks_move(nx, ny) || occupied(nx, ny, m)) continue;
        float sc = hypotf(nx + 0.5f - G.px, ny + 0.5f - G.py) + (hunt[ny][nx] >= 0 ? 0.0f : 2.0f) + rndf() * 0.3f;
        if (sc < bs) { bs = sc; best = k; }
    }
    if (best >= 0) { m->tx = m->cx + DX[best]; m->ty = m->cy + DY[best]; }
}

static void update_ally(Mob *m, float dt);
static void web_claw(Mob *m, float dt);
static void note_remains(const Mob *m, int deco);
static Mob *ally_near(const Mob *m, float r);
static float mob_blow(const Mob *m, float *every);
static void ally_hurt(Mob *a, float dmg);
static int latch_begin(Mob *m);
static void update_latch(float dt);
