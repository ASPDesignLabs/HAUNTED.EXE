/* A part of scene_crawl.c (docs/PLAN-tidy.md): it is #included there in its place and never compiled on its own.
 * Lines of sight, ranks picked, spawn_mob, what comes on a level (random_type), wanderers, the file and the roost placed, the readout's pop-up. */

/* ================================================================== */
/* Mobs, shots, explosions                                             */
/* ================================================================== */

static int line_clear(float x0, float y0, float x1, float y1) {
    float dx = x1 - x0, dy = y1 - y0, d = sqrtf(dx * dx + dy * dy);
    int n = (int)(d / 0.1f);
    for (int i = 1; i < n; i++) {
        float t = (float)i / n;
        int cx = (int)(x0 + dx * t), cy = (int)(y0 + dy * t);
        Cell *c = cell(cx, cy);
        if (c->kind == K_WALL || c->kind == K_PILLAR || (c->kind == K_DOOR && G.doors[c->door].open < 0.7f)) return 0;
    }
    return 1;
}

/* would a fireball thrown from the caster get there? (finer than line_clear, and from where it leaves the hand) */
static int shot_clear(float x1, float y1) {
    float a = atan2f(y1 - G.py, x1 - G.px), x0 = G.px + cosf(a) * 0.35f, y0 = G.py + sinf(a) * 0.35f;
    float dx = x1 - x0, dy = y1 - y0, d = sqrtf(dx * dx + dy * dy);
    int n = (int)(d / 0.04f) + 1;
    for (int i = 0; i <= n; i++) {
        float t = (float)i / n;
        int cx = (int)(x0 + dx * t), cy = (int)(y0 + dy * t);
        if (blocks_light(cx, cy) || cell(cx, cy)->kind == K_PILLAR) return 0;
    }
    return 1;
}

static void sfx_at(int id, float x, float y, float gain) {
    float dx = x - G.px, dy = y - G.py, d = sqrtf(dx * dx + dy * dy);
    float rx = -sinf(G.ang), ry = cosf(G.ang);
    float lat = dx * rx + dy * ry;
    float g = gain / (1.0f + 0.05f * d * d);
    if (g > 0.03f) snd_sfx_ex(id, g, 0.5f + 0.4f * clampf(lat / (d + 0.5f), -1.0f, 1.0f));
}

/* creatures that stand in a cell and get in the way (not wraiths, wall skulls or spiders in their webs) */
static int buried(const Mob *m);
static int solid_mob(const Mob *m) { return m->alive && KIND_IS(m, KF_SOLID) && !buried(m); }

static int occupied(int x, int y, const Mob *self) {
    if ((x == G.cx && y == G.cy) || (x == G.tx && y == G.ty)) return 1;
    for (int i = 0; i < MAXMOBS; i++) {
        const Mob *m = &G.mobs[i];
        if (m != self && solid_mob(m) && ((m->tx == x && m->ty == y) || (m->cx == x && m->cy == y))) return 1;
    }
    return 0;
}

static float mob_base_hp(int type) { return KINDS[type].hp; }
static float shield_hp(void) { return 10.0f + 6.0f * (float)(G.level - 3); }   /* grows tougher each level */

/* a creature takes a rank: its colour, and its toughness, blow, pace and worth with it */
static void set_rank(Mob *m, int rank) {
    m->rank = rank;
    m->speed = KINDS[m->type].speed * RANK_SPEED[rank];
    m->hp = m->maxhp = m->type == MOB_WSKULL ? 1.0f : mob_base_hp(m->type) * powf(1.04f, (float)(G.level - 1)) * RANK_HP[rank];
    if (m->type == MOB_SHIELD) m->shield = shield_hp() * sqrtf(RANK_HP[rank]);
    m->dmgk = RANK_DMG[rank];
}

/* which rank, by the set's mix (PLAN-bestiary, section 4): the set's newest rung comes in on its run's
 * fifth level; each time round again the mix climbs a rung, up to black */
static int pick_rank(int type) {
    if (KINDS[type].family < 0) return RK_I;
    if (CON.rank >= 0) return CON.rank;
#ifdef CRAWL_DEBUG
    if (getenv("CRAWL_RANK")) return clampi(atoi(getenv("CRAWL_RANK")), 0, NRANK - 1);
#endif
    static const int MIX[4][5] = { { 85, 15, 0, 0, 0 }, { 55, 35, 10, 0, 0 }, { 20, 40, 30, 10, 0 }, { 0, 25, 35, 30, 10 } };
    int set = crawl_set(G.level), step = crawl_step(G.level), loop = crawl_loop(G.level), share[5];
    if (type == MOB_BAT && set == 3) return RK_IV;                 /* the Deep's bats: embers, all of them */
    for (int i = 0; i < 5; i++) share[i] = MIX[set][i];
    if (step < 4 && loop == 0) { share[set] += share[set + 1]; share[set + 1] = 0; }
    for (int l = 0; l < loop; l++) { share[4] += share[3]; for (int i = 3; i > 0; i--) share[i] = share[i - 1]; share[0] = 0; }
    int r = rnd(100), k = 0;
    while (k < 4 && r >= share[k]) r -= share[k++];
    return k < KINDS[type].top ? k : KINDS[type].top;              /* a black bat is lost in the dark: they stop at red */
}

/* may one of these come now? not before its first level; and on that level, only the one */
static int may_spawn(int type) {
    if (G.level < KINDS[type].debut) return 0;
    return G.level > KINDS[type].debut || G.spawned[type] == 0;
}

static Mob *spawn_mob(int x, int y, int type) {
    for (int i = 0; i < MAXMOBS; i++) {
        Mob *m = &G.mobs[i];
        if (m->alive) continue;
        memset(m, 0, sizeof *m);
        m->alive = 1; m->type = type; m->state = MS_WALK;
        m->x = x + 0.5f; m->y = y + 0.5f; m->cx = m->tx = x; m->cy = m->ty = y;
        m->home = -1; m->last_atk = -1; m->deco = -1;
        set_rank(m, pick_rank(type));
        G.spawned[type]++;
        m->face = rndf() * TAU_F;
        if (!(KINDS[type].flags & KF_UNCOUNTED)) G.kills_total++;
        m->t = rndf() * 10.0f; m->orbit = rndf() * TAU_F;
        return m;
    }
    return NULL;
}

/* Spectres (docs/PLAN-b11.md, SP2): on every level, one or two on a run's first three; after that their number rests
 * on Luck: about one level in two at the 2% everyone starts with, two a level at 10%, four at 25%. A boss's level has
 * none below 10% Luck, then one or two. They drift in one at a time through the level, so they are met */
static int spectre_count(void) {
#ifdef CRAWL_DEBUG
    if (getenv("CRAWL_SPECTRES")) return atoi(getenv("CRAWL_SPECTRES"));
#endif
    float luck = gear_total(&RUN.gear, ST_LUCK);
    if (crawl_boss(G.level)) return luck < 10.0f ? 0 : 1 + rnd(2);
    if (crawl_step(G.level) < 3) return 1 + rnd(2);
    float e = luck <= 10.0f ? 0.5f + (luck - 2.0f) * 0.1875f : fminf(6.0f, 2.0f + (luck - 10.0f) * 2.0f / 15.0f);
    int n = (int)e;
    return n + (rndf() < e - (float)n);
}
static void spectres_set(void) { G.spec_n = spectre_count(); G.spec_t = 15.0f + 45.0f * rndf(); }
static float spectre_souls(void) { int set = crawl_set(G.level); return set == 0 ? 2.0f : set == 1 ? 1.5f : 1.0f; }   /* thinning deeper */

/* what wanders the halls: skeletons, spectres and bats, shield-bearers from level 4; the spectres
 * give way to the Wraith after the first run, and bats grow rare in the Deep */
static int random_type(void) {
    int w[NKINDS] = { 0 }, total = 0;
    w[MOB_SKEL] = 10;
    w[MOB_GHOST] = 0;                                              /* spectres come by their own count now (spectre_count) */
    w[MOB_BAT] = crawl_set(G.level) == 3 ? 2 : 5;
    w[MOB_SHIELD] = may_spawn(MOB_SHIELD) ? 4 : 0;
    w[MOB_SHAMBLER] = may_spawn(MOB_SHAMBLER) ? (crawl_set(G.level) <= 1 ? 5 : 3) : 0;
    w[MOB_HOUND] = may_spawn(MOB_HOUND) ? 4 : 0;
    w[MOB_HUNTER] = may_spawn(MOB_HUNTER) ? 3 : 0;
    w[MOB_GHOUL] = may_spawn(MOB_GHOUL) ? 4 : 0;
    w[MOB_KNIGHT] = may_spawn(MOB_KNIGHT) ? 2 : 0;
    w[MOB_SHADE] = may_spawn(MOB_SHADE) ? 3 : 0;
    w[MOB_IMP] = may_spawn(MOB_IMP) ? 1 : 0;
    w[MOB_DEACON] = may_spawn(MOB_DEACON) ? 2 : 0;
    w[MOB_WHIPPER] = mind_on() && may_spawn(MOB_WHIPPER) ? 1 : 0;   /* the new kinds (step 9): only with the new minds, for they */
    w[MOB_SEXTON] = mind_on() && may_spawn(MOB_SEXTON) ? 2 : 0;     /* are made for them */
    w[MOB_LIGHTLESS] = mind_on() && may_spawn(MOB_LIGHTLESS) ? 2 : 0;
    for (int i = 0; i < NKINDS; i++) total += w[i];
    int r = rnd(total), k = 0;
    while (r >= w[k]) r -= w[k++];
    return k;
}

static int free_near(int x, int y, int *ox, int *oy);
/* a wanderer, or a few: hounds hunt in pairs (except the first, which comes alone), a knight keeps two,
 * and a deacon walks with its dead */
static void spawn_wanderer_type(int x, int y, int type) {
    Mob *m = spawn_mob(x, y, type);
    int escort = type == MOB_GHOUL && mind_on() && G.level > KINDS[MOB_GHOUL].debut ? rnd(3)   /* scavengers, one to three */
               : type == MOB_HOUND && G.level > KINDS[MOB_HOUND].debut ? 1
               : type == MOB_KNIGHT && G.level > KINDS[MOB_KNIGHT].debut ? 2
               : type == MOB_DEACON ? (G.level > KINDS[MOB_DEACON].debut ? 3 : 2)
               : type == MOB_WHIPPER ? 3 + (crawl_set(G.level) >= 3) : 0;   /* never without three or four hounds */
    int pk = -1;
    if (type == MOB_WHIPPER && m && (pk = pack_new(PACK_WHIP)) >= 0) pack_join(pk, m);
    if (type == MOB_HOUND && escort && m && (pk = pack_new(PACK_HOUNDS)) >= 0) {   /* the new minds: a pack, two to four by the run */
        int set = crawl_set(G.level);
        escort = 1 + (set >= 2) + (set >= 3);
        pack_join(pk, m);
    }
    if (type == MOB_GHOUL && escort && m && (pk = pack_new(PACK_SCAV)) >= 0) pack_join(pk, m);
    if ((type == MOB_KNIGHT && since(17) && escort) || type == MOB_DEACON) {   /* the knight and his hounds, the deacon's congregation:
                                                                      mixed bands (section 7) */
        if (m && (pk = pack_new(PACK_BAND)) >= 0) pack_join(pk, m);
    }
    int shield = type == MOB_DEACON && pk >= 0 && may_spawn(MOB_SHIELD);   /* a shield-bearer in front of his dead */
    for (int d = 0; m && d < (type == MOB_WHIPPER ? 12 : 4) && escort; d++) {   /* the huntsman's, a step further if need be */
        int ex = x + DX[d & 3], ey = y + DY[d & 3];
        if (d >= 4 && !free_near(x, y, &ex, &ey)) break;
        if (G.map[ey][ex].kind == K_FLOOR && !occupied(ex, ey, NULL)) {
            Mob *e = spawn_mob(ex, ey, type == MOB_DEACON ? (shield ? MOB_SHIELD : MOB_SKEL) : type == MOB_GHOUL ? MOB_GHOUL : MOB_HOUND);   /* the
                                                                      huntsman's are hounds */
            shield = 0;
            if (pk >= 0) pack_join(pk, e);
            escort--;
        }
    }
#ifdef CRAWL_DEBUG
    if (pk >= 0) fprintf(stderr, "PACKS level %d: %s %d at (%d,%d)\n", G.level, type == MOB_GHOUL ? "scavengers" : type == MOB_KNIGHT ? "the knight and his hounds"
                         : type == MOB_DEACON ? "the deacon's congregation" : type == MOB_WHIPPER ? "the huntsman and his hounds" : "hounds", G.packs[pk].n0, x, y);
#endif
}

/* a free floor cell a few steps from (x, y), for a pack's members */
static int free_near(int x, int y, int *ox, int *oy) {
    int qx[64], qy[64], qh = 0, qt = 0;
    uint8_t seen[MAPN][MAPN / 8 + 1];
    memset(seen, 0, sizeof seen);
    qx[qt] = x; qy[qt] = y; qt++; seen[y][x >> 3] |= 1 << (x & 7);
    while (qh < qt) {
        int cx = qx[qh], cy = qy[qh]; qh++;
        int taken = occupied(cx, cy, NULL);                       /* nor one overhead or under the earth there */
        for (int j = 0; j < MAXMOBS && !taken; j++) taken = G.mobs[j].alive && G.mobs[j].cx == cx && G.mobs[j].cy == cy;
        if ((cx != x || cy != y) && G.map[cy][cx].kind == K_FLOOR && !G.map[cy][cx].secret && !taken && !in_band(cy)) { *ox = cx; *oy = cy; return 1; }
        for (int d = 0; d < 4 && qt < 64; d++) {
            int nx = cx + DX[d], ny = cy + DY[d];
            if (nx < 1 || ny < 1 || nx >= MAPN - 1 || ny >= MAPN - 1 || (seen[ny][nx >> 3] >> (nx & 7) & 1) || G.map[ny][nx].kind != K_FLOOR) continue;
            seen[ny][nx >> 3] |= 1 << (nx & 7); qx[qt] = nx; qy[qt] = ny; qt++;
        }
    }
    return 0;
}
/* The file of the Order (from 5): its shields in front, its throwers behind; on a round of the halls, or now and then
 * holding a room as its residents. Two to five: three in run I, four in II, four or five deeper. */
static int place_file(void) {
    if (!mind_on() || G.level < 5 || (G.level != 5 && rnd(2))) return 0;
    int x, y;
    if (!pick_cell_at_depth(G.cx, G.cy, 10, 40, 1, &x, &y) || in_band(y) || in_wing(x, y) || in_trial(x, y) || vault_of_room(x, y) >= 0) return 0;
    int pk = pack_new(PACK_FILE);
    if (pk < 0) return 0;
    int set = crawl_set(G.level), n = imin(3 + (set >= 1) + (set >= 2 && rnd(2)), set_cap), shields = may_spawn(MOB_SHIELD) ? 1 + (n >= 4) : 0;
    int held = rnd(5) < 2, made = 0;
    if (n < 2) { G.npacks--; return 0; }
    for (int t = 0; held && t < 60; t++) {                         /* holding a room: one of its own (section 18) first, then a lair (D2) */
        int r = rnd(G.nrooms), rt = G.rooms[r].rtype;
        if (t < 30 ? rt != RT_OSSUARY && rt != RT_CHAPTER && rt != RT_FOSSIL : !(G.lairs >> r & 1)) continue;
        const Room *R = &G.rooms[r];
        int lx = R->x + rnd(R->w), ly = R->y + rnd(R->h);
        if (G.map[ly][lx].kind != K_FLOOR || G.map[ly][lx].secret || occupied(lx, ly, NULL) || in_band(ly) || in_wing(lx, ly) || in_trial(lx, ly)
            || vault_of_room(lx, ly) >= 0 || abs(lx - G.cx) + abs(ly - G.cy) < 12) continue;
        x = lx; y = ly; break;
    }
    int rg = G.map[y][x].region;
    held = held && room_of_region(rg) >= 0;
    for (int i = 0; i < n; i++) {
        int mx = x, my = y;
        if (i && !free_near(x, y, &mx, &my)) break;
        Mob *m = spawn_mob(mx, my, i < shields ? MOB_SHIELD : MOB_SKEL);
        if (!m) break;
        if (held) m->home = G.map[my][mx].region;
        pack_join(pk, m); made++;
    }
#ifdef CRAWL_DEBUG
    fprintf(stderr, "PACKS level %d: file of %d (%d shields) at (%d,%d) %s\n", G.level, made, shields, x, y, held ? "holding a room" : "on a round");
#endif
    return made > 0;
}
/* The roost (from 11): three to six bats asleep on the ceiling of the dimmest room, waking all at once */
static int place_roost(void) {
    if (!mind_on() || G.level < 11 || (G.level != 11 && rnd(2))) return 0;
    int best = -1; float bl = 1e9f;
    for (int r = 1; r < G.nrooms; r++) {
        const Room *R = &G.rooms[r];
        float mx = R->x + R->w * 0.5f, my = R->y + R->h * 0.5f, lit = 0.0f;
        if (R->w * R->h < 9 || vault_of_room(R->x, R->y) >= 0 || in_band(R->y) || in_band(R->y + R->h - 1) || r == G.pz_room || r == G.tri_room
            || abs((int)mx - G.cx) + abs((int)my - G.cy) < 10) continue;
        for (int l = 0; l < G.nlights; l++) { float d = hypotf(G.lights[l].x - mx, G.lights[l].y - my); lit += G.lights[l].power / (1.0f + d * d); }
        if (!(G.lairs >> r & 1)) lit += 1000.0f;                   /* a lair first, with its two ways in (D2) */
        if (R->rtype == RT_UNDER || R->rtype == RT_GEODE) lit -= 2000.0f;   /* and before that the rooms that are its own (section 18) */
        if (lit < bl) { bl = lit; best = r; }
    }
    if (best < 0) return 0;
    int pk = pack_new(PACK_ROOST);
    if (pk < 0) return 0;
    const Room *R = &G.rooms[best];
    int n = imin(clampi(2 + crawl_set(G.level) + rnd(2), 3, 6), set_cap), made = 0;
    if (n < 2) { G.npacks--; return 0; }
    for (int tries = 0; tries < 60 && made < n; tries++) {
        int x = R->x + rnd(R->w), y = R->y + rnd(R->h);
        if (G.map[y][x].kind != K_FLOOR || G.map[y][x].secret || occupied(x, y, NULL)) continue;
        Mob *m = spawn_mob(x, y, MOB_BAT);
        if (!m) break;
        m->sleep = 1; m->home = G.map[y][x].region; m->x += (rndf() - 0.5f) * 0.4f; m->y += (rndf() - 0.5f) * 0.4f;
        pack_join(pk, m); made++;
    }
#ifdef CRAWL_DEBUG
    fprintf(stderr, "PACKS level %d: roost of %d in room %d at (%d,%d)\n", G.level, made, best, R->x, R->y);
#endif
    if (made && since(21) && may_spawn(MOB_SHADE) && set_cap - made >= 2) {   /* the roost and the shade (section 7; from 21): a dark room
                                                                      the shade keeps, the cloud at your face while its touch comes */
        for (int tries = 0; tries < 40; tries++) {
            int x = R->x + rnd(R->w), y = R->y + rnd(R->h);
            if (G.map[y][x].kind != K_FLOOR || G.map[y][x].secret || occupied(x, y, NULL)) continue;
            Mob *m = spawn_mob(x, y, MOB_SHADE);
            if (m) m->home = G.map[y][x].region;
#ifdef CRAWL_DEBUG
            fprintf(stderr, "PACKS level %d: and a shade keeps the roost's room\n", G.level);
#endif
            break;
        }
    }
    return made > 0;
}
static void spawn_wanderer(int x, int y) { spawn_wanderer_type(x, y, random_type()); }

/* cells reachable from (sx,sy) at a BFS depth in [dmin, dmax] */
static int pick_cell_at_depth(int sx, int sy, int dmin, int dmax, int need_hidden, int *ox, int *oy) {
    static int dist[MAPN][MAPN], qx[MAPN * MAPN], qy[MAPN * MAPN], cand[MAPN * MAPN];
    for (int y = 0; y < MAPN; y++) for (int x = 0; x < MAPN; x++) dist[y][x] = -1;
    int qh = 0, qt = 0, nc = 0;
    dist[sy][sx] = 0; qx[qt] = sx; qy[qt] = sy; qt++;
    while (qh < qt) {
        int x = qx[qh], y = qy[qh]; qh++;
        int d = dist[y][x];
        if (d >= dmin && d <= dmax && G.map[y][x].kind == K_FLOOR && !occupied(x, y, NULL) && !in_band(y)
            && (!need_hidden || !line_clear(G.px, G.py, x + 0.5f, y + 0.5f))) cand[nc++] = y * MAPN + x;
        if (d >= dmax) continue;
        for (int k = 0; k < 4; k++) {
            int nx = x + DX[k], ny = y + DY[k];
            if (!passable(nx, ny) || dist[ny][nx] >= 0) continue;
            dist[ny][nx] = d + 1; qx[qt] = nx; qy[qt] = ny; qt++;
        }
    }
    if (!nc) return 0;
    int c = cand[rnd(nc)];
    *ox = c % MAPN; *oy = c / MAPN;
    return 1;
}

static void pop_readout(void) {                                    /* in from the bottom; if it is up already, it stays */
    if (G.readout <= 0.0f || G.readout > READOUT_T) G.readout = READOUT_T;
    else if (G.readout < READOUT_T - 0.25f) G.readout = READOUT_T - 0.25f;
}
static void show_bar(void) { G.bar_t = BAR_T; }

#ifdef CRAWL_DEBUG
static int hits_bone, hits_bat, throws, shrieks, hits_fire, hits_sword, bites, casts[NATK], defs[NDEF], bfs_kills, dodges, wskull_wakes, spider_drops,
           grabs, hound_bites, wraith_touches, wraith_drives, leaps, knight_blows, brood_bites, ling_bites, sacs_laid,
           shade_touches, embers, ember_hits, ember_burns, ember_scorch, raised, garg_wakes, mirrored, turned,
           exq_shatter, exq_bruise, exq_reaped, exq_mendward, exq_dread, exq_mirror, parries,   /* Exquisite runes' second effects */
           knight_turned, turned_hits,                                                /* the Ill-Fated Knight's Mirror Rune */
           spider_mends, spider_ups, webbed, web_torn, egg_shots,                     /* the Wandering Spider */
           ward_broken, ward_grown, chant_broken, litany_hits, litany_missed,         /* the Black Deacon */
           brazier_out, brazier_lit, wraith_slips, wraithwalks,                       /* the Wraith Unbound */
           orb_hits, shatters, contagions, flamewalks, wisp_host, tether_ticks,       /* the ultimates (1.0.0b11) */
           souls_slipped, shakes;
#define DBG_COUNT(v) ((v)++, fprintf(stderr, "EVENT %s t=%.2f\n", #v, G.t))
static float dbg_soul_in, dbg_soul_out, dbg_soul_last = -1.0f;   /* the souls that came and went this level */
static int dbg_weak;
static void foot_watch(void) {                                     /* footprints (1.0.0b18.1): the dead through part-filled cells */
    static int was[MAXMOBS];
    static float brush_t[MAXMOBS];
    for (int i = 0; i < MAXMOBS; i++) {
        const Mob *m = &G.mobs[i];
        int in = m->alive && foot_of(m->cx, m->cy) != 0;
        if (in && !was[i]) fprintf(stderr, "EVENT foot_pass %d at %d,%d t=%.2f\n", m->type, m->cx, m->cy, G.t);
        was[i] = in;
        if (m->alive && m->state == MS_WALK && foot_hit(m->x, m->y) && G.t - brush_t[i] > 1.0f) {
            brush_t[i] = (float)G.t;
            fprintf(stderr, "EVENT foot_brush %d at %.2f,%.2f t=%.2f\n", m->type, m->x, m->y, G.t);
        }
    }
}
static void dbg_souls(void) {
    float cur = (float)RUN.souls + RUN.soul_frac;
    if (dbg_soul_last >= 0.0f) { float d = cur - dbg_soul_last; if (d > 0.0f) dbg_soul_in += d; else dbg_soul_out -= d; }
    dbg_soul_last = cur;
}
#else
#define DBG_COUNT(v) ((void)0)
#endif

static int in_combat(void);

/* The caster takes a blow. Shadow robes may make it miss, a ward soaks it up, armor blunts it.
 * Returns what got through. (sx, sy): where it came from, for the portrait's flinch. */
static void ward_burst(void);
static int passing(void) { return G.dash_t > 0.0f || G.wwalk_t > 0.0f; }   /* darting, or wraithwalking: missiles go through */
static Blame blame_of(const Mob *m) { Blame b = { (uint8_t)(m->boss ? BL_BOSS : BL_MOB), (uint8_t)(m->boss ? m->boss - 1 : m->type), (uint8_t)m->rank }; return b; }
static void death_fall(void);
static void death_wake(void);
static void thing_drop_home(void);
/* The element of a hurt (docs/PLAN-b11.md, section 4): by how it is resisted, unless the one about to deal
 * it says otherwise (harm_next: the shade's touch is Spirit, the Wraith's Frost). Once it gets through, the
 * kind that dealt it is learnt to deal it, three times and known (rec_harm) */
static int harm_next = -1;
