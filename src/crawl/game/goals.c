/* A part of scene_crawl.c (docs/PLAN-tidy.md): it is #included there in its place and never compiled on its own.
 * The dream's goals: secrets pushed, keys, the altar, chains, a boss, motes; places to mend. */

/* ================================================================== */
/* The caster's walk                                                   */
/* ================================================================== */

static float heading_angle(int h) { return h * (PI_F * 0.5f); }
static void begin_turn(float target);

static void say(const char *fmt, ...);
static void open_secret(int i) {
    Secret *sc = &G.secrets[i];
    if (sc->state || G.slide >= 0) return;
    sc->state = 1; G.slide = i; G.slide_t = 0.0f;
    rumble(0.22f, 0.0f, SLIDE_TIME);                               /* stone grinding on stone */
    noise(sc->fx + 0.5f, sc->fy + 0.5f, 8, NZ_SECRET, -1);         /* heard eight steps off (S3) */
    G.map[sc->sy][sc->sx].kind = K_FLOOR;                          /* the block is drawn by itself while it moves */
    int nl = G.nlights;
    light_static(sc->sx + 0.5f + DX[sc->d] * 1.5f, sc->sy + 0.5f + DY[sc->d] * 1.5f, 0.75f, 3.4f);   /* the hoard's own glow */
    if (sc->len > 3 || sc->x1 - sc->x0 + sc->y1 - sc->y0 > 4)     /* a bigger room: the far end too */
        light_static((sc->x0 + sc->x1 + 1) * 0.5f + DX[sc->d] * 1.0f, (sc->y0 + sc->y1 + 1) * 0.5f + DY[sc->d] * 1.0f, 0.7f, 4.0f);
    if (sc->door_x >= 0)                                           /* what comes through the doorway */
        light_static(sc->door_x + 0.5f - (sc->door_x - imin(imax(sc->door_x, sc->x0), sc->x1)) * 0.6f,
                     sc->door_y + 0.5f - (sc->door_y - imin(imax(sc->door_y, sc->y0), sc->y1)) * 0.6f, 0.55f, 2.6f);
    for (int l = nl; l < G.nlights; l++) G.lights[l].phase = -1000.0f;   /* steady, not flames */
    G.secrets_found++; G.links_found++;
    rec_secret();
    say("A SECRET!   %d OF %d", G.links_found, G.nlinks);
    xp_gain(25.0f * xp_level(), "A SECRET");
    if (G.links_found >= G.nlinks) gain_coin(COIN_ALL, "EVERY SECRET HERE FOUND");
    sfx_at(SFX_PUSHWALL, sc->sx + 0.5f, sc->sy + 0.5f, 1.0f);
    snd_sfx_ex(SFX_SECRET, 1.0f, 0.5f);
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT secret %d of %d (room %d, chain of %d) t=%.2f by %s\n", G.links_found, G.nlinks, i, sc->links, G.t, G.player ? "player" : "autopilot");
#endif
}
static void update_slide(float dt) {
    if (G.slide < 0) return;
    Secret *sc = &G.secrets[G.slide];
    G.slide_t += dt * (float)sc->len / SLIDE_TIME;
    if (G.slide_t < (float)sc->len) return;
    int ex = sc->sx + DX[sc->d] * sc->len, ey = sc->sy + DY[sc->d] * sc->len;   /* it comes to rest at the back */
    G.map[ey][ex].kind = K_WALL; G.map[ey][ex].tex = (uint8_t)sc->tex;
    G.map[sc->sy][sc->sx].secret = 0;
    for (int y = sc->y0; y <= sc->y1; y++) for (int x = sc->x0; x <= sc->x1; x++) G.map[y][x].secret = 0;
    sc->state = 2; G.slide = -1;
    for (int i = 0; i < MAXWISPS; i++) G.wisps[i].gx = -1;        /* the closet is part of the crypt now: map the ways again */
}

/* steps from (sx,sy) to the nearest unexplored cell, not passing back through (bx,by) */
static int dist_to_unexplored(int sx, int sy, int bx, int by) {
    static int stamp[MAPN][MAPN], dist[MAPN][MAPN], qx[MAPN * MAPN], qy[MAPN * MAPN], gen;
    gen++;
    int qh = 0, qt = 0;
    stamp[sy][sx] = gen; dist[sy][sx] = 0; qx[qt] = sx; qy[qt] = sy; qt++;
    stamp[by][bx] = gen;
    while (qh < qt) {
        int x = qx[qh], y = qy[qh]; qh++;
        if (G.map[y][x].visits == 0) return dist[y][x];
        for (int d = 0; d < 4; d++) {
            int nx = x + DX[d], ny = y + DY[d];
            if (!apass(nx, ny) || stamp[ny][nx] == gen) continue;
            stamp[ny][nx] = gen; dist[ny][nx] = dist[y][x] + 1; qx[qt] = nx; qy[qt] = ny; qt++;
        }
    }
    return 999;
}

static int bfs_dist[MAPN][MAPN];
static void bfs_from(int sx, int sy) {                             /* along the lanes */
    static int qx[MAPN * MAPN], qy[MAPN * MAPN];
    for (int y = 0; y < MAPN; y++) for (int x = 0; x < MAPN; x++) bfs_dist[y][x] = -1;
    int qh = 0, qt = 0;
    bfs_dist[sy][sx] = 0; qx[qt] = sx; qy[qt] = sy; qt++;
    while (qh < qt) {
        int x = qx[qh], y = qy[qh]; qh++;
        for (int d = 0; d < 4; d++) {
            int nx = x + DX[d], ny = y + DY[d];
            if (!apass(nx, ny) || bfs_dist[ny][nx] >= 0) continue;
            bfs_dist[ny][nx] = bfs_dist[y][x] + 1; qx[qt] = nx; qy[qt] = ny; qt++;
        }
    }
}

/* the way to open vault k: its key (which may be locked up in another vault), then its door */
static int key_goal(int k, int depth, int *gx, int *gy) {
    if (depth > 3 || k < 0 || G.vault_door[k] < 0) return 0;
    const Door *d = &G.doors[G.vault_door[k]];
    if (d->locked != 1) return 0;
    if (G.has_key[k]) { *gx = d->x; *gy = d->y; return 1; }
    for (int i = 0; i < G.nitems; i++) {
        const Item *it = &G.items[i];
        if (!it->alive || it->type != IT_KEY || it->color != k) continue;
        int v = vault_of_room(it->cx, it->cy);
        if (v >= 0 && v != k && G.doors[G.vault_door[v]].locked == 1) return key_goal(v, depth + 1, gx, gy);
        *gx = it->lx; *gy = it->ly;
        return 1;
    }
    return 0;
}

/* a key whose vault is still shut: the caster can hear it chime from a little way off */
static int key_calls(const Item *it) {
    if (it->type != IT_KEY || it->color < 0 || it->color >= G.nvaults || G.vault_door[it->color] < 0) return 0;
    return G.doors[G.vault_door[it->color]].locked == 1;
}

static int worth_taking(const Item *it) {
    if (it->type == IT_SCRIP) return 1;
    if (it->type == IT_POTION) return G.hp < G.max_hp * 4 / 5;      /* saved for when it is needed */
    if (it->type == IT_BONE) return RUN.gear.atk[AT_SPEAR] >= 0 && RUN.gear.bones < bones_max();
    return 1;
}

/* The autopilot and the Black Altar: it wakes one with 25 spirit or more (keeping 15 for its spells)
 * while there is time to follow its wisp, blesses its exit wisps with 40 or more when the level has
 * undead, and anoints when it burns gold. It asks from the lane beside it. */
#define ALTAR_TIME 240.0                    /* past this, it leaves the altar and its secrets be */
#define WAKE_TIME 180.0                     /* and past this, it is too late to wake one */
static int altar_wake(void), altar_bless(void), altar_anoint(void), altar_send(void), nearest_shut(float x, float y), altar_price(void);
static int altar_spot(int *sx, int *sy) {
    if (G.shop || G.altar_x < 0) return 0;
    for (int d = 0; d < 4; d++) {
        int x = G.altar_x + DX[d], y = G.altar_y + DY[d];
        if (apass(x, y)) { *sx = x; *sy = y; return 1; }
    }
    return 0;
}
static int undead_about(void) {
    for (int i = 0; i < MAXMOBS; i++) if (G.mobs[i].alive && KIND_IS(&G.mobs[i], KF_UNDEAD)) return 1;
    return 0;
}
static int wants_wake(void) { return G.altar_state == ALTAR_DORMANT && G.level_t <= WAKE_TIME && spirit() >= 25.0f && G.secrets_found < G.nsecrets; }
static int wants_bless(void) { return !G.blessed && G.level_t <= ALTAR_TIME && spirit() >= 40.0f && undead_about(); }
static int wants_send(void) {                                      /* its wisp home and resting: on, if 15 spirit is left after */
    return G.altar_state == ALTAR_AWAKE && G.altar_wisp == AW_REST && G.level_t <= ALTAR_TIME
        && nearest_shut(G.altar_x + 0.5f, G.altar_y + 0.5f) >= 0 && spirit() - (float)altar_price() >= 15.0f;
}
static int wants_anoint(void) {
    if (G.altar_state == ALTAR_READY)
        for (int s = 0; s < NSLOT; s++) if (RUN.gear.worn[s].kind && !RUN.gear.worn[s].anointed) return 1;
    return 0;
}
static int altar_wanted(void) {
    if (G.player || G.leaving || G.shop || G.altar_x < 0) return 0;
    return wants_anoint() || wants_wake() || wants_send() || wants_bless();
}

/* The autopilot in a chain (docs/PLAN-b6.md): it goes through an open chained room's doorway while it
 * has two thirds of its health and before 180 s, and in the pockets takes the treasure it sees near,
 * then the visportal's gift (or the cooler's drink) at the end, or the way on; it turns back when its
 * health falls under two thirds, when it has the gift, or after CHAIN_TIME in a level's chains. It
 * stands on the lane beside the cell before a doorway or the visportal and turns to use it (PS_USE). */
enum { USE_NONE, USE_DOOR, USE_VIS, USE_THING };   /* USE_THING: a puzzle's piece (1.0.0b13) */
#define CHAIN_TIME 150.0f
static int pocket_go(int x, int y);
static int visportal_use(void);
static int use_spot(int tx, int ty, int *sx, int *sy) {
    for (int d = 0; d < 4; d++) {
        int fx = tx + DX[d], fy = ty + DY[d];
        if (fx < 1 || fy < 1 || fx >= MAPN - 1 || fy >= MAPN - 1 || !passable(fx, fy)) continue;
        if (apass(fx, fy)) { *sx = fx; *sy = fy; return 1; }
        for (int e = 0; e < 4; e++) if (apass(fx + DX[e], fy + DY[e])) { *sx = fx + DX[e]; *sy = fy + DY[e]; return 1; }
    }
    return 0;
}
static int chain_goal(int x, int y, int *gx, int *gy) {
    if (G.player || G.shop) return 0;
    int sx, sy;
    G.use_kind = USE_NONE;
    bfs_from(x, y);
    if (!PK.depth) {
        if (G.leaving || G.level_t > 180.0 || G.hp * 3 < G.max_hp * 2 || PK.auto_t >= CHAIN_TIME) return 0;
        int best = -1, bd = 41;
        for (int i = 0; i < G.nsecrets; i++) {
            const Secret *sc = &G.secrets[i];
            if (sc->state != 2 || sc->door_x < 0 || PK.auto_done[i] || !use_spot(sc->door_x, sc->door_y, &sx, &sy)) continue;
            int d = bfs_dist[sy][sx];
            if (d >= 0 && d < bd) { bd = d; best = i; *gx = sx; *gy = sy; }
        }
        if (best < 0) return 0;
        G.use_x = G.secrets[best].door_x; G.use_y = G.secrets[best].door_y; G.use_kind = USE_DOOR;
        return 1;
    }
    if (!PK.auto_back && (G.hp * 3 < G.max_hp * 2 || PK.auto_t >= CHAIN_TIME || (G.vis_x >= 0 && PK.vis_used[PK.chain]))) {
        PK.auto_back = 1;
#ifdef CRAWL_DEBUG
        fprintf(stderr, "EVENT chain_back %s link %d hp=%d/%d chain_t=%.1f t=%.2f\n", G.hp * 3 < G.max_hp * 2 ? "hurt" : PK.auto_t >= CHAIN_TIME ? "time" : "done",
                PK.depth, G.hp, G.max_hp, PK.auto_t, G.t);
#endif
    }
    int tx = PK.back_x, ty = PK.back_y, kind = USE_DOOR;
    if (!PK.auto_back) {
        int bi = -1, bd = 17;                                      /* treasure it has seen, close by */
        for (int i = 0; i < G.nitems; i++) {
            const Item *it = &G.items[i];
            if (!it->alive || !it->seen || !worth_taking(it) || (it->lx == x && it->ly == y)) continue;
            int d = bfs_dist[it->ly][it->lx];
            if (d >= 0 && d < bd) { bd = d; bi = i; }
        }
        if (bi >= 0) { *gx = G.items[bi].lx; *gy = G.items[bi].ly; return 1; }
        if (G.vis_x >= 0) { tx = G.vis_x; ty = G.vis_y; kind = USE_VIS; }
        else if (PK.on_x >= 0) { tx = PK.on_x; ty = PK.on_y; }
    }
#ifdef CRAWL_DEBUG
    { static double next; if (G.t >= next) { next = G.t + 3.0; int ok = use_spot(tx, ty, &sx, &sy);
      fprintf(stderr, "CHAIN at (%d,%d) to (%d,%d) kind %d spot %d (%d,%d) dist %d back %d t=%.1f\n", x, y, tx, ty, kind, ok, sx, sy, ok ? bfs_dist[sy][sx] : -9, PK.auto_back, G.t); } }
#endif
    if (!use_spot(tx, ty, &sx, &sy) || bfs_dist[sy][sx] < 0) return 0;
    G.use_x = tx; G.use_y = ty; G.use_kind = kind;
    *gx = sx; *gy = sy;
    return 1;
}

/* something worth walking to from (x,y): a vault door once its key is held, else treasure it has seen */
/* the autopilot in a boss's arena: its scrip once it falls; a full font when badly hurt; otherwise a
 * crossing of lanes three to six cells from the boss and away from its adds, changed every few seconds
 * so that what is thrown or swung finds nothing standing */
static int boss_goal(int x, int y, int *gx, int *gy) {
    if (G.boss < 0 || PK.depth || !in_arena(x + 0.5f, y + 0.5f)) return 0;
    if (G.bstate == BS_DEAD) {
        for (int i = 0; i < G.nitems; i++)
            if (G.items[i].alive && G.items[i].type == IT_SCRIP) { *gx = G.items[i].lx; *gy = G.items[i].ly; return 1; }
        return 0;
    }
    if (!boss_fighting()) return 0;
    if (G.hp < G.max_hp * 0.4f)
        for (int i = 0; i < G.bnfont; i++)
            if (G.bfont_full[i] && G.bfont_fx[i] >= 0 && apass(G.bfont_fx[i], G.bfont_fy[i])) { *gx = G.bfont_fx[i]; *gy = G.bfont_fy[i]; return 1; }
    const Mob *b = boss_mob();
    float bx = b ? b->x : (float)G.exit_fx, by = b ? b->y : (float)G.exit_fy;
    if (G.bstate == BS_WAVE && G.badds > 0) {                      /* adds out and none near: go and find the nearest */
        float nd = 1e9f; const Mob *na = NULL;
        for (int i = 0; i < MAXMOBS; i++) {
            const Mob *o = &G.mobs[i];
            float d = hypotf(o->x - G.px, o->y - G.py);
            if (o->alive && o->add == 1 && o->state == MS_WALK && d < nd) { nd = d; na = o; }
        }
        if (na && nd > 5.0f) { bx = na->x; by = na->y; }
    }
    if (G.boss == BOSS_WRAITH && G.bstate == BS_FIGHT) {           /* the Wraith's: under four lit, kindle the nearest out */
        int lit = 0, nb = -1; float nd = 1e9f;
        for (int k = 0; k < G.bbz_n; k++) {
            float d = hypotf(G.bbz_x[k] + 0.5f - G.px, G.bbz_y[k] + 0.5f - G.py);
            lit += G.bbz_lit[k];
            if (!G.bbz_lit[k] && d < nd) { nd = d; nb = k; }
        }
        if (lit < 4 && nb >= 0) {
            int bc = -1; float bdd = 1e9f;
            for (int cy = G.bbz_y[nb] - 2; cy <= G.bbz_y[nb] + 2; cy++)
                for (int cx = G.bbz_x[nb] - 2; cx <= G.bbz_x[nb] + 2; cx++) {
                    float dd = hypotf(cx - G.bbz_x[nb], cy - G.bbz_y[nb]);
                    if (cx < 0 || cy < 0 || cx >= MAPN || cy >= MAPN || dd > 1.9f || !apass(cx, cy) || !in_arena(cx + 0.5f, cy + 0.5f)) continue;
                    if (dd < bdd) { bdd = dd; bc = cy * MAPN + cx; }
                }
            if (bc >= 0 && bc != y * MAPN + x) { *gx = bc % MAPN; *gy = bc / MAPN; return 1; }
            if (bc >= 0) return 0;                                 /* there: stand and kindle it */
        }
    }
    static int keep_x = -1, keep_y = -1; static double keep_t = -1.0;
    if (keep_x >= 0 && G.t < keep_t && apass(keep_x, keep_y) && in_arena(keep_x + 0.5f, keep_y + 0.5f)
        && fabsf(hypotf(keep_x + 0.5f - bx, keep_y + 0.5f - by) - 4.5f) < 2.0f && (keep_x != x || keep_y != y)) { *gx = keep_x; *gy = keep_y; return 1; }
    int best = -1; float bs = 1e9f;
    for (int cy = BAND_Y0; cy < MAPN; cy++)
        for (int cx = ARENA_X0; cx < MAPN; cx++) {
            if ((cx & 3) != 2 || (cy & 3) != 2 || !apass(cx, cy) || (cx == x && cy == y)) continue;
            float d = hypotf(cx + 0.5f - bx, cy + 0.5f - by), sc = fabsf(d - 4.5f) * 2.0f + 0.25f * (float)(abs(cx - x) + abs(cy - y)) + rndf();
            for (int i = 0; i < MAXMOBS; i++)
                if (G.mobs[i].alive && G.mobs[i].add && G.mobs[i].state == MS_WALK && hypotf(G.mobs[i].x - cx - 0.5f, G.mobs[i].y - cy - 0.5f) < 2.5f) sc += 4.0f;
            if (G.boss == BOSS_WRAITH && !wraith_lit(cx + 0.5f, cy + 0.5f)) sc += 3.0f;   /* in the light, where it can be hurt */
            if (sc < bs) { bs = sc; best = cy * MAPN + cx; }
        }
    if (best < 0) return 0;
    keep_x = best % MAPN; keep_y = best / MAPN; keep_t = G.t + 3.0 + rndf() * 2.0;
    *gx = keep_x; *gy = keep_y;
    return 1;
}

/* the autopilot's motes (section 12): one within three steps of its lane, while nothing hunts it near */
static int mote_goal(int *gx, int *gy) {
    for (int i = 0; i < MAXMOBS; i++) {
        const Mob *m = &G.mobs[i];
        if (m->alive && m->state == MS_WALK && m->alert > 0.0f && !KIND_IS(m, KF_SPIRIT) && !m->ally && hypotf(m->x - G.px, m->y - G.py) < 6.0f) return 0;
    }
    int best = 4, bx = -1, by = -1;
    for (int i = 0; i < MAXMOTES; i++) {
        const SoulMote *m = &G.motes[i];
        if (!m->alive || m->t > MOTE_LIFE - 1.0f) continue;
        int mx = (int)m->x, my = (int)m->y;
        for (int yy = my - 1; yy <= my + 1; yy++)
            for (int xx = mx - 1; xx <= mx + 1; xx++) {
                if (!apass(xx, yy) || hypotf(xx + 0.5f - m->x, yy + 0.5f - m->y) > MOTE_REACH - 0.1f) continue;
                int d = bfs_dist[yy][xx];
                if (d >= 1 && d < best) { best = d; bx = xx; by = yy; }
            }
    }
    if (bx < 0) return 0;
    *gx = bx; *gy = by;
    return 1;
}
static int pz_goal(int x, int y, int *gx, int *gy);
static int tri_goal(int x, int y, int *gx, int *gy);
/* ---- Places to mend (docs/PLAN-ai.md, section 28; "rests" in the code, since the Mend rune has G.mend_*): one a level,
 * two from run III, in a room's wall facing its way in, so whoever mends there has their back to the door; never the
 * start's room or the exit's, a vault, the wing, a trial's room, an arena or a pocket. E facing one begins; standing
 * there, health rises towards three quarters of the whole, from nothing in twenty seconds; moving, turning away,
 * casting, E again or a blow ends it. PHYSIC raises how far (a point for each 16%) and how fast (by half of it). Every
 * second of it is a noise sixteen steps off, and what hears it knows where you are. ---- */
#define REST_T 20.0f
static float rest_cap(void) { return fminf(1.0f, 0.75f + gear_total(&RUN.gear, ST_PHYSIC) / 1600.0f); }
static int rest_facing(float reach) {                              /* the one the caster stands before and faces */
    for (int i = 0; i < G.nrest; i++) {
        if (hypotf(G.rest_fx[i] + 0.5f - G.px, G.rest_fy[i] + 0.5f - G.py) > reach) continue;
        if (fabsf(wrap_pi(atan2f(G.rest_y[i] + 0.5f - G.py, G.rest_x[i] + 0.5f - G.px) - G.ang)) < 0.7f) return i;
    }
    return -1;
}
static void rest_end(const char *why) {
    if (!G.rest_on) return;
    G.rest_on = 0;
    snd_sfx_ex(SFX_WIND, 0.35f, 0.5f);
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT mend_end %s hp=%d/%d t=%.2f\n", why, G.hp, G.max_hp, G.t);
#else
    (void)why;
#endif
}
static int rest_begin(int i) {
    if (G.hp >= (int)(rest_cap() * (float)G.max_hp)) { say("IT CAN MEND YOU NO FURTHER"); snd_sfx_ex(SFX_KNOCK, 0.5f, 0.5f); return 1; }
    G.rest_on = i + 1; G.rest_acc = 0.0f; G.rest_beat = 0.0f;
    snd_sfx_ex(SFX_WHISPER, 0.6f, 0.5f); sfx_at(SFX_DRIP, G.rest_x[i] + 0.5f, G.rest_y[i] + 0.5f, 0.8f);
    if (!G.rest_said) { G.rest_said = 1; say("YOU KNEEL AND MEND. THE DEAD WILL HEAR IT"); }
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT mend_begin %d hp=%d/%d by %s t=%.2f\n", i, G.hp, G.max_hp, G.player ? "player" : "autopilot", G.t);
#endif
    return 1;
}
static void rest_tick(float dt) {                                  /* every frame, whoever plays */
    if (!G.rest_on) return;
    int i = G.rest_on - 1;
    if (G.dead || G.shop || PK.depth || i >= G.nrest) { rest_end("gone"); return; }
    if (G.player && G.spd > 0.2f) { rest_end("moved"); return; }
    if (G.player && rest_facing(1.5f) != i) { rest_end("turned"); return; }
    float phys = gear_total(&RUN.gear, ST_PHYSIC), cap = rest_cap() * (float)G.max_hp;
    G.rest_acc += 0.75f * (float)G.max_hp / REST_T * (1.0f + phys / 200.0f) * dt;
    while (G.rest_acc >= 1.0f && G.hp < (int)cap) { G.hp++; G.rest_acc -= 1.0f; }
    if ((G.rest_beat -= dt) <= 0.0f) {                              /* the dead hear it */
        G.rest_beat = 1.0f;
        noise(G.px, G.py, 16, NZ_MEND, -1);
        sfx_at(SFX_DRIP, G.rest_x[i] + 0.5f, G.rest_y[i] + 0.5f, 0.5f);
    }
    if (G.hp >= (int)cap) { rest_end("full"); say("IT CAN MEND YOU NO FURTHER"); }
}
static void place_rests(void) {
    G.nrest = 0; G.rest_on = 0; G.rest_goal = -1;
    if (G.shop || PK.depth || G.nrooms <= 0) return;
    int want = crawl_set(G.level) >= 2 ? 2 : 1;
    uint32_t seed = hash2(G.level * 31 + 7, 977);
    int start_rg = cell((int)G.px, (int)G.py)->region, exit_rg = G.exit_fx >= 0 ? cell(G.exit_fx, G.exit_fy)->region : -1000;
    for (int pass = 0; pass < G.nrooms && G.nrest < want; pass++) {
        int r = (pass + (int)(seed % (uint32_t)G.nrooms)) % G.nrooms, vault = 0;
        const Room *R = &G.rooms[r];
        for (int k = 0; k < G.nvaults; k++) vault |= G.vault_room[k] == r;
        if (vault || R->region == start_rg || R->region == exit_rg || r == G.pz_room || r == G.tri_room || in_band(R->y + R->h)) continue;
        float ex = 0.0f, ey = 0.0f; int ne = 0;                    /* the way in: what is open on the ring about it */
        for (int y = R->y - 1; y <= R->y + R->h; y++)
            for (int x = R->x - 1; x <= R->x + R->w; x++) {
                if (y > R->y - 1 && y < R->y + R->h && x > R->x - 1 && x < R->x + R->w) continue;
                int k = cell(x, y)->kind;
                if (k == K_FLOOR || k == K_DOOR) { ex += (float)x; ey += (float)y; ne++; }
            }
        if (!ne) continue;
        ex /= (float)ne; ey /= (float)ne;
        float cx = (float)R->x + (float)R->w * 0.5f - 0.5f, cy = (float)R->y + (float)R->h * 0.5f - 0.5f, bs = -1e9f;
        int bx = -1, by = -1, bfx = 0, bfy = 0;
        for (int y = R->y - 1; y <= R->y + R->h; y++)               /* the wall it faces: as far from the way in as can be */
            for (int x = R->x - 1; x <= R->x + R->w; x++) {
                const Cell *w = cell(x, y);
                if (x < 1 || y < 1 || x >= MAPN - 1 || y >= MAPN - 1) continue;   /* the map's rim is stone */
                if (w->kind != K_WALL || w->secret || w->ftorch || !plain_tex(w->tex)) continue;
                int open = 0, fx = 0, fy = 0;
                for (int d = 0; d < 4; d++) if (face_open(x, y, d)) { open++; fx = x + DX[d]; fy = y + DY[d]; }
                if (open != 1 || cell(fx, fy)->kind != K_FLOOR || cell(fx, fy)->region != R->region) continue;
                float s = -((float)x - cx) * (ex - cx) - ((float)y - cy) * (ey - cy) + 0.3f * (float)(hash2(x, y + (int)seed) & 7);
                if (s > bs) { bs = s; bx = x; by = y; bfx = fx; bfy = fy; }
            }
        if (bx < 0) continue;
        G.map[by][bx].tex = T_REST;
        G.rest_x[G.nrest] = bx; G.rest_y[G.nrest] = by; G.rest_fx[G.nrest] = bfx; G.rest_fy[G.nrest] = bfy; G.nrest++;
    }
#ifdef CRAWL_DEBUG
    fprintf(stderr, "MEND level %d:", G.level);
    for (int i = 0; i < G.nrest; i++) fprintf(stderr, " (%d,%d) before (%d,%d)", G.rest_x[i], G.rest_y[i], G.rest_fx[i], G.rest_fy[i]);
    fprintf(stderr, "\n");
#endif
}
/* the dream: under two fifths of its health, nothing after it, one within thirty steps: there, and it mends */
static int rest_wanted(void) {
    return !G.player && !G.shop && !PK.depth && G.nrest > 0 && !G.rest_on && G.hp * 5 < G.max_hp * 2 && !in_combat()
        && G.hp < (int)(rest_cap() * (float)G.max_hp);
}
static int rest_goal(int x, int y, int *gx, int *gy) {
    G.rest_goal = -1;
    if (!rest_wanted()) return 0;
    bfs_from(x, y);
    int bd = 31;
    for (int i = 0; i < G.nrest; i++) {
        int sx, sy;
        if (!use_spot(G.rest_x[i], G.rest_y[i], &sx, &sy)) continue;
        int d = bfs_dist[sy][sx];
        if (d >= 0 && d < bd) { bd = d; G.rest_goal = i; *gx = sx; *gy = sy; }
    }
    return G.rest_goal >= 0;
}
static void begin_turn(float target);
static int targetable(const Mob *m);
