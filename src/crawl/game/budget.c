/* A part of scene_crawl.c (docs/PLAN-tidy.md): it is #included there in its place and never compiled on its own.
 * A level's creatures placed: the caster's state reset, the debug map, the Black Altar's place, the old placements, bands, and the encounter budget (place_creatures). */

/* ================================================================== */
/* Level flow                                                          */
/* ================================================================== */

static void begin_shop(void);
static void human_begin(void);

static void reset_caster_state(void) {
    G.poison_t = G.poison_dps = G.poison_acc = G.slow_t = G.purify_t = G.regen_acc = 0.0f;
    af_clear();
    G.ward_hp = G.ward_t = G.mirror_t = G.slip_t = G.reap_t = G.mend_t = G.mend_rate = G.mirror_flash = G.mirror_true = 0.0f;
    G.ward_shards = G.decoy_t = 0.0f;
    memset(G.def_cd, 0, sizeof G.def_cd); memset(G.def_wait, 0, sizeof G.def_wait);
    memset(G.def_hush, 0, sizeof G.def_hush); memset(G.rune_glow, 0, sizeof G.rune_glow);
    G.bfs_t = 0.0f; G.face_t = G.grin_t = 0.0f;
    G.l_down = 0; G.l_t = 0.0f; G.pilgrim = 0;
    memset(G.ab_cd, 0, sizeof G.ab_cd); G.dash_t = G.wwalk_t = 0.0f; G.tap_key = -1; G.ab_ready = 0;   /* the abilities ready again */
    for (int b = 0; b < NBOSS; b++) if (RUN.abil[b]) G.ab_ready |= 1 << b;
    G.max_hp = gear_max_hp(&RUN.gear);
    if (G.hp > G.max_hp) G.hp = G.max_hp;
}

#ifdef CRAWL_DEBUG
/* the level from above, for checking the generator; seen: the lanes the autopilot has counted as seen */
static void debug_map(int seen) {
    {
        static char rows[MAPN][MAPN + 1];
        for (int y = 0; y < MAPN; y++) {
            for (int x = 0; x < MAPN; x++) {
                const Cell *c = &G.map[y][x];
                char ch = c->kind == K_WALL ? '#' : c->kind == K_PILLAR ? 'O' : c->kind == K_PROP ? (foot_of(x, y) ? 'f' : 'p') : c->kind == K_DOOR ? (G.doors[c->door].locked ? 'L' : 'D') : (lane(x, y) ? (seen && c->visits ? 'o' : '.') : ' ');
                if (c->kind == K_WALL && c->tex == T_TORCH0) ch = 't';
                if (c->secret) ch = c->secret == 2 ? '?' : (c->secret == 3 ? '&' : ':');
                rows[y][x] = ch;
            }
            rows[y][MAPN] = 0;
        }
        for (int i = 0; i < G.nitems; i++) rows[G.items[i].cy][G.items[i].cx] = G.items[i].type == IT_KEY ? 'k' : G.items[i].type == IT_POTION ? '+' : '$';
        for (int i = 0; i < MAXMOBS; i++) if (G.mobs[i].alive) rows[(int)G.mobs[i].y][(int)G.mobs[i].x] = 'm';
        if (G.exit_fx >= 0) { rows[G.exit_fy][G.exit_fx] = 'x'; rows[G.exit_wy][G.exit_wx] = 'X'; }
        rows[G.cy][G.cx] = seen ? '@' : 'S';
        fprintf(stderr, "MAP level %d rooms %d vaults %d doors %d lights %d decos %d items %d lanes %d\n", G.level, G.nrooms, G.nvaults, G.ndoors, G.nlights, G.ndecos, G.nitems, G.ncells);
        for (int y = 0; y < MAPN; y++) fprintf(stderr, "MAP %s\n", rows[y]);
        if (!seen) {
            for (int y = 0; y < MG; y++) {                          /* and the plan it was drawn from */
                char pr[MG + 1];
                for (int x = 0; x < MG; x++) pr[x] = MM[y][x].kind == K_WALL ? '#' : MM[y][x].kind == K_DOOR ? 'D' : MM[y][x].room >= 0 ? (char)('a' + MM[y][x].room) : (char)('0' + MM[y][x].w);
                pr[MG] = 0;
                fprintf(stderr, "PLAN %s\n", pr);
            }
            static int st[MAPN][MAPN], qx[MAPN * MAPN], qy[MAPN * MAPN];   /* can every lane be reached, keys aside? */
            memset(st, 0, sizeof st);
            int qh = 0, qt = 0, total = 0;
            st[G.cy][G.cx] = 1; qx[qt] = G.cx; qy[qt] = G.cy; qt++;
            while (qh < qt) {
                int x = qx[qh], y = qy[qh]; qh++;
                for (int d = 0; d < 4; d++) {
                    int nx = x + DX[d], ny = y + DY[d];
                    if (st[ny][nx] || !is_open(nx, ny) || !lane(nx, ny)) continue;
                    if (G.map[ny][nx].kind == K_DOOR && G.doors[G.map[ny][nx].door].locked == GATED) continue;   /* a barred gate is shut */
                    st[ny][nx] = 1; qx[qt] = nx; qy[qt] = ny; qt++;
                }
            }
            for (int y = 0; y < MAPN; y++) for (int x = 0; x < MAPN; x++) total += is_open(x, y) && lane(x, y);
            fprintf(stderr, "MAP lanes reachable %d of %d\n", qt, total);
            for (int y = 0, n = 0; y < MAPN && n < 16; y++)        /* and the first few that aren't */
                for (int x = 0; x < MAPN && n < 16; x++)
                    if (is_open(x, y) && lane(x, y) && !st[y][x]) { fprintf(stderr, "MAP lane cut off (%d,%d)%s\n", x, y, G.map[y][x].secret ? " secret" : ""); n++; }
        }
    }
}
#endif

/* A Black Altar, from level 6, on a level with secrets to find: in the exit room, clear of the way,
 * else in the room farthest from the start (PLAN-items: Black Altars). */
static void place_altar(void) {
    G.altar_x = -1; G.altar_state = ALTAR_DORMANT; G.altar_seek = -1; G.blessed = 0; G.altar_wisp = AW_NONE; G.altar_inside = 0;
    if (G.level < 6 || G.nsecrets == 0) return;
    int room = -1;
    for (int r = 0; r < G.nrooms && G.exit_fx >= 0 && room < 0; r++) if (in_room(&G.rooms[r], G.exit_fx, G.exit_fy)) room = r;
    for (int pass = 0; pass < 2; pass++) {
        if (pass == 1 || room <= 0) {                               /* the room farthest from the start */
            bfs_from(G.cx, G.cy);
            int bd = -1;
            room = -1;
            for (int r = 1; r < G.nrooms; r++) {
                const Room *Rm = &G.rooms[r];
                int d = bfs_dist[Rm->y + Rm->h / 2][Rm->x + Rm->w / 2];
                if (vault_of_room(Rm->x, Rm->y) < 0 && r != G.tri_room && d > bd) { bd = d; room = r; }
            }
            if (room < 0) return;
        }
        const Room *Rm = &G.rooms[room];
        for (int tries = 0; tries < 300; tries++) {
            int x = Rm->x + rnd(Rm->w), y = Rm->y + rnd(Rm->h);
            if (G.map[y][x].kind != K_FLOOR || G.map[y][x].secret || !block_corner(x, y) || lane(x, y) || by_way(x, y) || G.bulky[y][x] || item_at(x, y) || in_band(y)) continue;
            if (G.exit_fx >= 0 && abs(x - G.exit_fx) + abs(y - G.exit_fy) < 2) continue;   /* not in the way out */
            solid_prop(x, y, SP_BALTAR, 1, K_PROP);
            G.altar_x = x; G.altar_y = y;
#ifdef CRAWL_DEBUG
            fprintf(stderr, "EVENT altar at (%d,%d) in %s room\n", x, y, pass == 0 && in_room(Rm, G.exit_fx, G.exit_fy) ? "the exit" : "the farthest");
#endif
            return;
        }
    }
}

static void pocket_reset_level(void);
static void place_sites(void);
static void pz_make(void);
static void pz_hide(void);
static void docs_clear(void);
static void tri_place(void);
#ifdef CRAWL_DEBUG
static void things_test(void);
#endif
static void map_carto(void);
static void map_up(void);
static void map_keys(float dt);
static void map_follow(float dt);
/* Today's placements, kept for the old minds (CRAWL_MIND=0) */
static void spawn_wanderer(int x, int y);
static void place_creatures_old(void) {
    place_wall_skulls(G.cx, G.cy);
    place_spiders(G.cx, G.cy);
    place_guardians();
    int x, y;
    place_risers();
    place_imps();
    place_gargoyles();
    for (int i = 0; i < 3 + (G.level >= 4); i++) if (pick_cell_at_depth(G.cx, G.cy, 7, 40, 1, &x, &y)) spawn_wanderer(x, y);
    static const int WANDERS[] = { MOB_SHIELD, MOB_SHAMBLER, MOB_HOUND, MOB_HUNTER, MOB_GHOUL, MOB_KNIGHT, MOB_SHADE, MOB_IMP, MOB_DEACON };
    for (int i = 0; i < (int)(sizeof WANDERS / sizeof WANDERS[0]); i++)   /* a newcomer's first level always brings one, alone */
        if (G.level == KINDS[WANDERS[i]].debut && !G.spawned[WANDERS[i]] && pick_cell_at_depth(G.cx, G.cy, 8, 30, 1, &x, &y)) spawn_wanderer_type(x, y, WANDERS[i]);
    for (int r = 1; r < G.nrooms; r++) {                           /* residents: the dead keep to their rooms till they know you are there */
        const Room *R = &G.rooms[r];
        if (vault_of_room(R->x, R->y) >= 0 || (R->theme != TH_OSSUARY && R->theme != TH_CRYPT) || rnd(100) >= 45) continue;
        if (!room_cell(r, &x, &y) || abs(x - G.cx) + abs(y - G.cy) < 8) continue;
        int type = R->theme == TH_CRYPT && rnd(3) == 0 ? (may_spawn(MOB_KNIGHT) && rnd(2) ? MOB_KNIGHT : (may_spawn(MOB_SHIELD) ? MOB_SHIELD : MOB_SKEL)) : MOB_SKEL;
        Mob *m = spawn_mob(x, y, type);
        if (m) m->home = G.map[y][x].region;
    }
    if (may_spawn(MOB_BROOD) && (G.level == KINDS[MOB_BROOD].debut || rnd(10) < 6))   /* a brood mother in a room of her own, a sac or two about her */
        for (int tries = 0; tries < 20; tries++) {
            int r = 1 + rnd(G.nrooms > 1 ? G.nrooms - 1 : 1);
            if (r >= G.nrooms || vault_of_room(G.rooms[r].x, G.rooms[r].y) >= 0 || !room_cell(r, &x, &y) || abs(x - G.cx) + abs(y - G.cy) < 10) continue;
            Mob *m = spawn_mob(x, y, MOB_BROOD);
            if (!m) break;
            m->home = G.map[y][x].region; m->contact = 3.0f;
            int sx, sy;
            for (int k = 0; k < 1 + rnd(2); k++) if (room_cell(r, &sx, &sy)) { Mob *e = spawn_mob(sx, sy, MOB_SAC); if (e) e->rank = m->rank; }
            break;
        }
    if (G.corrupt && G.boss < 0) {                                 /* the bell's: two in five more of the dead */
        int n = 0;
        for (int i = 0; i < MAXMOBS; i++) n += G.mobs[i].alive && !KIND_IS(&G.mobs[i], KF_UNCOUNTED) && G.mobs[i].type != MOB_GHOST;
        for (int i = 0; i < (2 * n + 2) / 5; i++) if (pick_cell_at_depth(G.cx, G.cy, 8, 40, 1, &x, &y)) spawn_wanderer(x, y);
    }
}

/* Bands of every kind met (section 23; from 29), and of three kinds from 33: a leader (a knight, a deacon or a ghoul) and
 * what it has gathered, in a room of their own (a fossil hall, the throne) if there is one, holding it one time in two */
static int set_clear(int x, int y);
static int place_band(void) {
    if (!since(29) || PK.depth || set_cap < 3) return 0;
    static const int LEAD[] = { MOB_KNIGHT, MOB_DEACON, MOB_GHOUL }, ESC[] = { MOB_SKEL, MOB_SHIELD, MOB_HOUND, MOB_HUNTER, MOB_GHOUL, MOB_IMP, MOB_BAT };
    int lead = LEAD[rnd(3)];
    if (!may_spawn(lead)) return 0;
    int kinds[2] = { -1, -1 }, nk = 1 + since(33);
    for (int k = 0; k < nk; k++)
        for (int t = 0; t < 20 && kinds[k] < 0; t++) {
            int e = ESC[rnd(7)];
            if (e != lead && e != kinds[0] && may_spawn(e)) kinds[k] = e;
        }
    if (kinds[0] < 0) return 0;
    int x = -1, y = -1;
    for (int t = 0; t < 80 && x < 0; t++) {
        int r = 1 + rnd(G.nrooms > 1 ? G.nrooms - 1 : 1), rt = G.rooms[r].rtype, cx, cy;
        if (r >= G.nrooms || (t < 40 && rt != RT_FOSSIL && rt != RT_THRONE) || G.rooms[r].w * G.rooms[r].h < 9 || vault_of_room(G.rooms[r].x, G.rooms[r].y) >= 0
            || r == G.pz_room || r == G.tri_room || !room_cell(r, &cx, &cy) || in_band(cy) || abs(cx - G.cx) + abs(cy - G.cy) < 12 || !set_clear(cx, cy)) continue;
        x = cx; y = cy;
    }
    if (x < 0) return 0;
    int pk = pack_new(PACK_BAND);
    if (pk < 0) return 0;
    Mob *L = spawn_mob(x, y, lead);
    if (!L) return 0;
    int rg = G.map[y][x].region, held = rnd(2), n = imin(2 + since(33), set_cap - 2), made = 1;
    pack_join(pk, L);
    for (int i = 0; i < n; i++) {
        int mx, my;
        if (!free_near(x, y, &mx, &my) || G.map[my][mx].region != rg) break;
        Mob *e = spawn_mob(mx, my, kinds[i % nk] >= 0 ? kinds[i % nk] : kinds[0]);
        if (!e) break;
        if (e->type == MOB_HUNTER) e->sub = 2;                     /* on the floor with the rest */
        pack_join(pk, e); made++;
    }
    for (int i = 0; i < MAXMOBS; i++) if (G.packs[pk].mem >> i & 1) G.mobs[i].home = held ? rg : -1;
#ifdef CRAWL_DEBUG
    fprintf(stderr, "PACKS level %d: a band of %d, a %s and %s%s%s, %s\n", G.level, made, KINDS[lead].name, KINDS[kinds[0]].plural,
            kinds[1] >= 0 ? " and " : "", kinds[1] >= 0 ? KINDS[kinds[1]].plural : "", held ? "holding a room" : "on a round");
#endif
    return made;
}

/* ---- The encounter budget (docs/PLAN-ai.md, Part E; step 8). Each level has a budget of threat, spent on set pieces:
 * today's placements among them (the vaults' guards, a newcomer's first showing, the watch of wall skulls, the webs, the
 * buried, the imps, the gargoyles, the brood mother, the posts) as well as the packs, the hunt and the ambushes, and
 * wanderers to fill what is left. A head costs by its kind and a fifth more a rank (the red and the black are dearer, so
 * a level of them holds fewer). The budget follows what the levels held before the packs came: the creatures on a level
 * stay near today's numbers (section 21). A boss's level spends half outside its arena; a corrupted one two in five more.
 * Rules (section 17): the creatures placed stay within 48 of the 64 places; nothing is set within six steps of the
 * altar, a puzzle's or a trial's door or a place to mend; what killed you on a level comes back a head short. ---- */
#ifdef CRAWL_DEBUG
static const char *PIECE_NAME[NSET] = { "none", "wanderers", "newcomer", "posts", "guards", "watch", "webs", "buried", "imps",
                                      "gargoyles", "brood", "file", "roost", "hunt", "ambush", "band" };
#endif
static const float KIND_COST[NKINDS] = { [MOB_SKEL] = 1.0f, [MOB_BAT] = 0.75f, [MOB_WSKULL] = 0.75f, [MOB_SHIELD] = 1.25f,
    [MOB_SPIDER] = 0.5f, [MOB_SHAMBLER] = 0.75f, [MOB_DROWNED] = 1.0f, [MOB_HOUND] = 1.0f, [MOB_HUNTER] = 1.0f, [MOB_GHOUL] = 1.25f,
    [MOB_KNIGHT] = 2.0f, [MOB_BROOD] = 2.0f, [MOB_SAC] = 0.25f, [MOB_SHADE] = 1.5f, [MOB_IMP] = 1.0f, [MOB_DEACON] = 1.5f,
    [MOB_GARGOYLE] = 1.25f, [MOB_WHIPPER] = 1.5f, [MOB_SEXTON] = 1.0f, [MOB_LIGHTLESS] = 2.0f };
static float rank_cost(int rank) { return rank == RK_GILDED ? 1.4f : 1.0f + 0.2f * (float)imin(rank, RK_V); }
static float set_spent(int *heads) {
    float c = 0.0f; int n = 0;
    for (int i = 0; i < MAXMOBS; i++) {
        const Mob *m = &G.mobs[i];
        if (!m->alive) continue;
        n++;
        if (!m->boss && !m->add && !m->ally) c += KIND_COST[m->type] * rank_cost(m->rank);
    }
    if (heads) *heads = n;
    return c;
}
static float budget_of(int level) {                                /* fitted to what the levels held before (the old minds' runs), by the
                                                                      run and its step; a boss's level is its ninth's */
    static const float RUNB[4][10] = {
        { 4.3f, 5.4f, 7.5f, 9.8f, 12.3f, 13.3f, 13.2f, 14.2f, 14.0f, 14.0f },
        { 16.5f, 17.1f, 17.8f, 18.4f, 19.1f, 19.7f, 20.4f, 21.0f, 21.7f, 21.7f },
        { 19.5f, 19.7f, 19.9f, 20.1f, 20.3f, 20.5f, 20.7f, 20.9f, 21.1f, 21.1f },
        { 27.0f, 27.2f, 27.4f, 27.6f, 27.8f, 28.0f, 28.2f, 28.4f, 28.6f, 28.6f } };
    return RUNB[crawl_set(level)][clampi(crawl_step(level), 0, 9)];
}
static void set_tag(int k) { for (int i = 0; i < MAXMOBS; i++) if (G.mobs[i].alive && !G.mobs[i].set) G.mobs[i].set = k; }
static int set_clear(int x, int y) {                               /* rule 2: six steps from the places that have their own */
    int near = G.altar_x >= 0 && abs(x - G.altar_x) + abs(y - G.altar_y) < 6;
    if (G.pz_door >= 0) near |= abs(x - G.doors[G.pz_door].x) + abs(y - G.doors[G.pz_door].y) < 6;
    if (G.tri_door >= 0) near |= abs(x - G.doors[G.tri_door].x) + abs(y - G.doors[G.tri_door].y) < 6;
    for (int i = 0; i < G.nrest; i++) near |= abs(x - G.rest_fx[i]) + abs(y - G.rest_fy[i]) < 6;
    return !near;
}
static int set_first(int k) {                                      /* section 23: the level a set piece is first met on */
    switch (k) {
    case SET_WATCH: return 3;
    case SET_FILE: return 5;
    case SET_AMBUSH: return 7;
    case SET_ROOST: return 11;
    case SET_BAND: return 29;
    default: return 0;
    }
}
static void place_creatures(void) {
    int x, y, heads;
    float B = budget_of(G.level) * (G.corrupt ? 1.4f : 1.0f) * (G.boss >= 0 ? 0.5f : 1.0f);
#ifdef CRAWL_DEBUG
    if (getenv("CRAWL_BUDGET")) B *= (float)atof(getenv("CRAWL_BUDGET"));   /* the budget times f */
#endif
    if (short_lv != G.level) { memset(short_n, 0, sizeof short_n); short_hunt = 0; short_lv = G.level; }
    set_cap = 1 << 20;
    place_guardians(); set_tag(SET_GUARD);                         /* the vaults keep their guards */
    static const int WANDERS[] = { MOB_SHIELD, MOB_SHAMBLER, MOB_HOUND, MOB_HUNTER, MOB_GHOUL, MOB_KNIGHT, MOB_SHADE, MOB_IMP, MOB_DEACON,
                                   MOB_WHIPPER, MOB_SEXTON, MOB_LIGHTLESS };   /* the new kinds too (step 9): with the new minds only */
    for (int i = 0; i < (int)(sizeof WANDERS / sizeof WANDERS[0]); i++)   /* a newcomer's first level always brings one, alone */
        if (G.level == KINDS[WANDERS[i]].debut && !G.spawned[WANDERS[i]] && pick_cell_at_depth(G.cx, G.cy, 8, 30, 1, &x, &y)) spawn_wanderer_type(x, y, WANDERS[i]);
    set_tag(SET_NEW);
    /* the set pieces: the one this level shows first, then the rest in a shuffled order while the budget lasts */
    int order[NSET], n = 0;
    static const int PIECES[] = { SET_WATCH, SET_WEBS, SET_BURIED, SET_IMPS, SET_GARGS, SET_BROOD, SET_FILE, SET_ROOST, SET_AMBUSH, SET_HUNT, SET_BAND };
    for (int i = 0; i < (int)(sizeof PIECES / sizeof PIECES[0]); i++) order[n++] = PIECES[i];
    for (int i = n - 1; i > 0; i--) { int j = rnd(i + 1), t = order[i]; order[i] = order[j]; order[j] = t; }
    for (int i = 0; i < n; i++)
        if (set_first(order[i]) == G.level) { int t = order[i]; for (int j = i; j > 0; j--) order[j] = order[j - 1]; order[0] = t; }
    if (G.corrupt)                                                 /* a corrupted level always has its hunt (section 19): it goes early */
        for (int i = 0; i < n; i++) if (order[i] == SET_HUNT || order[i] == SET_AMBUSH) { int t = order[i]; for (int j = i; j > 1; j--) order[j] = order[j - 1]; order[1] = t; }
    int amb_at = -1, hunt_at = -1;                                 /* the ambush before the hunt (section 23, level 37) */
    for (int i = 0; i < n; i++) { if (order[i] == SET_AMBUSH) amb_at = i; if (order[i] == SET_HUNT) hunt_at = i; }
    if (amb_at > hunt_at) { order[amb_at] = SET_HUNT; order[hunt_at] = SET_AMBUSH; }
    for (int i = 0; i < n; i++) {
        int k = order[i];
        float left = B - set_spent(&heads), per = 1.0f + 0.2f * (float)crawl_set(G.level);   /* a head of the level's usual rank */
        if (left <= 0.0f || heads >= 48) break;
        if (set_first(k) == G.level) left = fmaxf(left, 4.0f * per);   /* its first showing comes, whatever else */
        float kc = k == SET_WATCH ? 0.75f : k == SET_WEBS ? 0.5f : k == SET_BURIED ? 0.85f : k == SET_GARGS ? 1.25f : k == SET_BROOD ? 2.5f : k == SET_BAND ? 1.4f : 1.1f;
        set_cap = imin((int)(left / (kc * per) + 0.5f), 48 - heads) - short_n[k];
        if (set_cap < 1) continue;
        switch (k) {
        case SET_WATCH: place_wall_skulls(G.cx, G.cy); break;
        case SET_WEBS: place_spiders(G.cx, G.cy); break;
        case SET_BURIED: place_risers(); break;
        case SET_IMPS: place_imps(); break;
        case SET_GARGS: place_gargoyles(); break;
        case SET_FILE: place_file(); break;
        case SET_ROOST: place_roost(); break;
        case SET_AMBUSH: place_ambushes(); break;
        case SET_HUNT: if (!short_hunt) place_hunt(); break;
        case SET_BAND: place_band(); break;
        case SET_BROOD:                                            /* a brood mother in a room of her own, a sac or two about her */
            if (may_spawn(MOB_BROOD) && (G.level == KINDS[MOB_BROOD].debut || rnd(10) < 6))
                for (int tries = 0; tries < 20; tries++) {
                    int r = 1 + rnd(G.nrooms > 1 ? G.nrooms - 1 : 1);
                    if (r >= G.nrooms || vault_of_room(G.rooms[r].x, G.rooms[r].y) >= 0 || !room_cell(r, &x, &y) || abs(x - G.cx) + abs(y - G.cy) < 10
                        || in_wing(x, y) || in_trial(x, y) || !set_clear(x, y)) continue;
                    Mob *m = spawn_mob(x, y, MOB_BROOD);
                    if (!m) break;
                    m->home = G.map[y][x].region; m->contact = 3.0f;
                    int sx, sy;
                    for (int s2 = 0; s2 < 1 + rnd(2); s2++) if (room_cell(r, &sx, &sy)) { Mob *e = spawn_mob(sx, sy, MOB_SAC); if (e) e->rank = m->rank; }
                    int pk;                                        /* the brood's guard (section 7; from 19): two hunting spiders that */
                    if (since(19) && may_spawn(MOB_HUNTER) && set_cap >= 4 && (pk = pack_new(PACK_BAND)) >= 0) {   /* never stray far from her */
                        pack_join(pk, m);
                        for (int g = 0; g < 2; g++) {
                            if (!room_cell(r, &sx, &sy) || occupied(sx, sy, NULL)) continue;
                            Mob *h = spawn_mob(sx, sy, MOB_HUNTER);
                            if (h) { h->sub = 0; h->home = m->home; pack_join(pk, h); }
                        }
#ifdef CRAWL_DEBUG
                        fprintf(stderr, "PACKS level %d: the brood's guard, %d with her in room %d\n", G.level, G.packs[pk].n0 - 1, r);
#endif
                    }
                    break;
                }
            break;
        }
        set_tag(k);
    }
    set_cap = 1 << 20;
    for (int r = 1; r < G.nrooms; r++) {                           /* posts: the dead keep to their rooms till they know you are there */
        const Room *R = &G.rooms[r];
        if (set_spent(&heads) >= B - 0.5f || heads >= 48) break;
        if (vault_of_room(R->x, R->y) >= 0 || (R->theme != TH_OSSUARY && R->theme != TH_CRYPT) || rnd(100) >= 45) continue;
        if (!room_cell(r, &x, &y) || abs(x - G.cx) + abs(y - G.cy) < 8 || in_wing(x, y) || in_trial(x, y) || !set_clear(x, y)) continue;
        int type = R->theme == TH_CRYPT && rnd(3) == 0 ? (may_spawn(MOB_KNIGHT) && rnd(2) ? MOB_KNIGHT : (may_spawn(MOB_SHIELD) ? MOB_SHIELD : MOB_SKEL)) : MOB_SKEL;
        if (R->rtype == RT_KNIGHTS && may_spawn(MOB_SHIELD)) type = crawl_loop(G.level) > 0 && may_spawn(MOB_KNIGHT) ? MOB_KNIGHT : MOB_SHIELD;   /* section 18 */
        Mob *m = spawn_mob(x, y, type);
        if (m) m->home = G.map[y][x].region;
    }
    set_tag(SET_POST);
    for (int tries = 0; tries < 40; tries++) {                     /* and wanderers with what is left */
        if (set_spent(&heads) >= B - 0.5f || heads >= 48) break;
        if (!pick_cell_at_depth(G.cx, G.cy, 7, 40, 1, &x, &y) || in_wing(x, y) || in_trial(x, y) || !set_clear(x, y)) continue;
        spawn_wanderer(x, y);
    }
    set_tag(SET_WANDER);
#ifdef CRAWL_DEBUG
    float cs[NSET] = { 0 }; int hs[NSET] = { 0 };
    for (int i = 0; i < MAXMOBS; i++) { const Mob *m = &G.mobs[i]; if (m->alive && !m->boss && m->set < NSET) { cs[m->set] += KIND_COST[m->type] * rank_cost(m->rank); hs[m->set]++; } }
    fprintf(stderr, "SETS level %d: budget %.1f spent %.1f heads %d:", G.level, B, set_spent(&heads), heads);
    for (int k = 1; k < NSET; k++) if (hs[k]) fprintf(stderr, " %s %d (%.1f)", PIECE_NAME[k], hs[k], cs[k]);
    for (int k = 1; k < NSET; k++) if (short_n[k]) fprintf(stderr, " [%s %d short]", PIECE_NAME[k], short_n[k]);
    fprintf(stderr, "\n");
#endif
}

