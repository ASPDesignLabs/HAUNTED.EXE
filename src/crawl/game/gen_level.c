/* A part of scene_crawl.c (docs/PLAN-tidy.md): it is #included there in its place and never compiled on its own.
 * gen_level itself (with con_carve and gen_dress), then the dressing: props and their footprints, crystals, fires, litter (dress_level). */

static void con_carve(int x, int y, int a, int b) {             /* a wall between two regions opened, a door into a room most times */
    int room_side = mc(x - 1, y)->room >= 0 || mc(x + 1, y)->room >= 0 || mc(x, y - 1)->room >= 0 || mc(x, y + 1)->room >= 0;
    int hall_side = (mopen(x - 1, y) && mc(x - 1, y)->room < 0) || (mopen(x + 1, y) && mc(x + 1, y)->room < 0)
                 || (mopen(x, y - 1) && mc(x, y - 1)->room < 0) || (mopen(x, y + 1) && mc(x, y + 1)->room < 0);
    mcarve(x, y, room_side && !hall_side ? a : (mc(x - 1, y)->room < 0 && mopen(x - 1, y) ? MM[y][x - 1].region
                : (mc(x, y - 1)->room < 0 && mopen(x, y - 1) ? MM[y - 1][x].region : b)), TH_CORRIDOR, -1);
    if (room_side && rnd(10) < 7) MM[y][x].kind = K_DOOR;
}
static int mind_on(void);
static int since(int level);
/* what follows the plan on every level: the walls' stone by the room beside them, colonnades, torches, the dressing, what
 * the autopilot can walk, fog, a boss's arena and the faces; a drawn level (docs/PLAN-tutorial.md) shares it */
static void gen_dress(void) {
    /* wall textures from the neighbouring room's theme, or the corridor zone */
    for (int y = 0; y < MAPN; y++)
        for (int x = 0; x < MAPN; x++) {
            Cell *c = &G.map[y][x];
            if (c->kind != K_WALL) continue;
            int theme = -1;
            for (int d = 0; d < 4; d++) {
                Cell *n = cell(x + DX[d], y + DY[d]);
                if (n->kind == K_FLOOR && n->theme != TH_CORRIDOR) theme = n->theme;
            }
            if (theme < 0) {
                uint32_t z = hash2(x / 9 + 101, y / 9 + 57) % 10;
                theme = z < 6 ? TH_CORRIDOR : (z < 8 ? TH_MOSS : TH_OSSUARY);
            }
            c->tex = (uint8_t)theme_tex(theme, hash2(x, y));
        }
    for (int i = 0; i < G.nsecrets; i++) {                       /* a plain stone for the cracks to show in */
        Secret *sc = &G.secrets[i];
        Cell *w = &G.map[sc->sy][sc->sx];
        if (w->tex == T_NICHE || w->tex == T_BANNER || w->tex == T_SKULLS) w->tex = T_STONE;
        sc->tex = w->tex;
        if (sc->door_x >= 0) G.map[sc->door_y][sc->door_x].tex = T_PASSAGE;
    }

    /* colonnades down some of the big rooms: a pillar where four of their blocks meet */
    for (int r = 0; r < G.nrooms; r++) {
        if (mroom[r][2] < 3 || mroom[r][3] < 3 || rnd(10) >= 4) continue;
        for (int my = mroom[r][1] + 1; my < mroom[r][1] + mroom[r][3] - 1; my += 2)
            for (int mx = mroom[r][0] + 1; mx < mroom[r][0] + mroom[r][2] - 1; mx += 2) {
                int x = mstart(mx), y = mstart(my);
                G.map[y][x].kind = K_PILLAR;
                add_deco(x + 0.5f, y + 0.5f, SP_PILLAR);
            }
    }

    /* torches along the halls */
    static int order[MAPN * MAPN];
    int no = 0;
    for (int i = 0; i < MAPN * MAPN; i++) order[no++] = i;
    for (int i = no - 1; i > 0; i--) { int j = rnd(i + 1), t = order[i]; order[i] = order[j]; order[j] = t; }
    for (int i = 0; i < no && G.nlights < MAXLIGHTS - 40; i++) {
        int x = order[i] % MAPN, y = order[i] / MAPN;
        if (G.map[y][x].kind != K_FLOOR || G.map[y][x].secret) continue;
        int near = 0;
        for (int l = 0; l < G.nlights; l++) if (abs(G.lights[l].cx - x) + abs(G.lights[l].cy - y) < 6) near = 1;
        if (near || rndf() > 0.68f - 0.22f * depth()) continue;     /* fewer torches the deeper you go */
        int d = rnd(4);
        for (int k = 0; k < 4; k++, d = (d + 1) & 3) {
            int wx = x + DX[d], wy = y + DY[d];
            Cell *w = cell(wx, wy);
            if (w->kind != K_WALL || w->secret || wx <= 0 || wy <= 0 || wx >= MAPN - 1 || wy >= MAPN - 1) continue;
            int bydoor = 0;
            for (int e = 0; e < 4; e++) if (cell(wx + DX[e], wy + DY[e])->kind == K_DOOR) bydoor = 1;
            if (bydoor) continue;
            w->tex = T_TORCH0; w->ftorch = (uint8_t)(((d + 2) & 3) + 1);   /* on the face toward the hall */
            light_static(x + 0.5f + DX[d] * 0.42f, y + 0.5f + DY[d] * 0.42f, 1.55f, 4.6f);
            break;
        }
    }

    dress_level();

    G.ncells = 0;                                                  /* what the autopilot can walk */
    for (int y = 0; y < MAPN; y++) for (int x = 0; x < MAPN; x++) if (G.map[y][x].kind == K_FLOOR && lane(x, y) && !G.map[y][x].secret) G.ncells++;

    /* fog banks: smooth blobs from a coarse random lattice; a few wisps at first, thick in the Deep */
    #define FOGL (MAPN * 10 / 48 + 3)
    float lat[FOGL][FOGL];
    for (int j = 0; j < FOGL; j++) for (int i = 0; i < FOGL; i++) lat[j][i] = rndf();
    float dk = clampf(0.45f * decay() + 0.55f * depth(), 0.0f, 1.0f), thr = 0.72f - 0.32f * dk, amt = 0.4f + 0.45f * dk;
    for (int y = 0; y < MAPN; y++)
        for (int x = 0; x < MAPN; x++) {
            float fx = x / 4.8f, fy = y / 4.8f;
            int ix = (int)fx, iy = (int)fy;
            float ax = fx - ix, ay = fy - iy;
            ax = ax * ax * (3 - 2 * ax); ay = ay * ay * (3 - 2 * ay);
            float n = lerpf(lerpf(lat[iy][ix], lat[iy][ix + 1], ax), lerpf(lat[iy + 1][ix], lat[iy + 1][ix + 1], ax), ay);
            G.fog[y][x] = clampf((n - thr) * 3.2f, 0.0f, 1.0f) * amt;
        }
    #undef FOGL
    if (G.boss >= 0) stamp_arena();
    if (G.faced) lay_faces();
}

static void gen_level(void) {
    G.faced = crawl_drawn(G.level); G.look = crawl_set(G.level);
    memset(nr_type, 0, sizeof nr_type);
    memset(G.map, 0, sizeof G.map);
    for (int y = 0; y < MAPN; y++)
        for (int x = 0; x < MAPN; x++) { G.map[y][x].kind = K_WALL; G.map[y][x].region = -1; G.map[y][x].door = -1; }
    memset(MM, 0, sizeof MM);
    for (int y = 0; y < MG; y++) for (int x = 0; x < MG; x++) { MM[y][x].kind = K_WALL; MM[y][x].region = -1; MM[y][x].room = -1; }
    if (G.boss >= 0)                                               /* the boss's strip of the plan: kept out of the rest */
        for (int y = ARENA_PLAN; y < MG; y++) for (int x = 0; x < MG; x++) MM[y][x].kind = K_PILLAR;
    G.nrooms = 0; G.ndoors = 0; G.nlights = 0; G.nlc = 0; G.ndecos = 0;
    memset(G.bulky, 0, sizeof G.bulky); memset(G.mapped, 0, sizeof G.mapped); memset(G.foot, 0, sizeof G.foot);
    int nreg = 0;

    /* rooms: chambers of one to four blocks a side, a few of them big halls */
    static const int SPAN[10] = { 1, 1, 3, 3, 3, 3, 5, 5, 5, 7 };
    for (int tries = 0; tries < 400 && G.nrooms < 9; tries++) {
        int w = SPAN[rnd(10)], h = SPAN[rnd(10)];
        if (w * h > 35 || (w == 1 && h == 1 && rnd(2))) continue;
        int x = 1 + 2 * rnd((MG - w - 1) / 2 + 1), y = 1 + 2 * rnd((MG - h - 1) / 2 + 1);
        if (x + w >= MG - 1 || y + h >= MG - 1) continue;
        int ok = 1;
        for (int yy = y - 1; yy <= y + h && ok; yy++)
            for (int xx = x - 1; xx <= x + w; xx++) if (MM[yy][xx].kind != K_WALL) { ok = 0; break; }
        if (!ok) continue;
        int theme = 1 + rnd(TH_COUNT - 1), rtype = 0;
        if (G.faced) { rtype = pick_rtype(w, h); theme = RTYPES[rtype].theme; }
        for (int yy = y; yy < y + h; yy++) for (int xx = x; xx < x + w; xx++) mcarve(xx, yy, nreg, theme, G.nrooms);
        mroom[G.nrooms][0] = x; mroom[G.nrooms][1] = y; mroom[G.nrooms][2] = w; mroom[G.nrooms][3] = h;
        G.rooms[G.nrooms++] = (Room){ 0, 0, 0, 0, nreg, theme, rtype };
        nreg++;
    }
    if (G.boss >= 0) {                                             /* the start as far from the arena as the rooms allow */
        int far = 0, fd = -1;
        for (int r = 0; r < G.nrooms; r++) {
            int d = abs(mroom[r][0] + mroom[r][2] / 2 - ARENA_DOOR_MX) + abs(mroom[r][1] + mroom[r][3] / 2 - (ARENA_PLAN - 2));
            if (d > fd) { fd = d; far = r; }
        }
        if (far) swap_rooms(0, far);
    }
    for (int y = 1; y < MG - 1; y += 2)
        for (int x = 1; x < MG - 1; x += 2)
            if (MM[y][x].kind == K_WALL && nreg < 1000) mz_grow(x, y, nreg++);
    if (G.boss >= 0) {                                             /* and a way down into its antechamber, kept open */
        MCell *a = &MM[ARENA_PLAN - 2][ARENA_DOOR_MX], *d = &MM[ARENA_PLAN - 1][ARENA_DOOR_MX];
        d->kind = K_DOOR; d->region = a->region; d->theme = TH_CORRIDOR; d->room = -1; d->keep = a->keep = 1;
    }

    /* connect every region, mostly as a tree, with a few extra loops; doors into most rooms */
    static int con[MG * MG][4];
    int nc = 0;
    for (int y = 1; y < MG - 1; y++)
        for (int x = 1; x < MG - 1; x++) {
            if (MM[y][x].kind != K_WALL || !((x ^ y) & 1)) continue;   /* only a wall between two blocks */
            int a = -1, b = -1;
            if (mopen(x - 1, y) && mopen(x + 1, y)) { a = MM[y][x - 1].region; b = MM[y][x + 1].region; }
            else if (mopen(x, y - 1) && mopen(x, y + 1)) { a = MM[y - 1][x].region; b = MM[y + 1][x].region; }
            if (a >= 0 && b >= 0 && a != b) { con[nc][0] = x; con[nc][1] = y; con[nc][2] = a; con[nc][3] = b; nc++; }
        }
    for (int i = nc - 1; i > 0; i--) {
        int j = rnd(i + 1);
        for (int k = 0; k < 4; k++) { int t = con[i][k]; con[i][k] = con[j][k]; con[j][k] = t; }
    }
    for (int i = 0; i < nreg; i++) uf[i] = i;
    /* D2 (docs/PLAN-ai.md, section 13; step 7): with the new minds, two or three rooms where a pack may lie (G.lairs) get a
     * second way in where the plan allows, and the extra loops within two blocks of them come one time in six, not one in
     * twelve: a pack then has a way round to you */
    static uint8_t lairnear[MG][MG];
    memset(lairnear, 0, sizeof lairnear);
    G.lairs = 0;
    if (mind_on() && since(25)) {                                  /* with flanking, the reds' cunning (section 23) */
        int cand[12], n = 0, want = 2 + (crawl_set(G.level) >= 2);
        for (int r = 1; r < G.nrooms; r++) if (mroom[r][2] * mroom[r][3] >= 3) cand[n++] = r;
        for (int i = n - 1; i > 0; i--) { int j = rnd(i + 1), t = cand[i]; cand[i] = cand[j]; cand[j] = t; }
        for (int i = 0; i < n && i < want; i++) {
            int r = cand[i];
            G.lairs |= 1u << r;
            for (int y = 0; y < MG; y++)
                for (int x = 0; x < MG; x++) {
                    int dx = x < mroom[r][0] ? mroom[r][0] - x : (x >= mroom[r][0] + mroom[r][2] ? x - mroom[r][0] - mroom[r][2] + 1 : 0);
                    int dy = y < mroom[r][1] ? mroom[r][1] - y : (y >= mroom[r][1] + mroom[r][3] ? y - mroom[r][1] - mroom[r][3] + 1 : 0);
                    if (dx + dy <= 4) lairnear[y][x] = 1;
                }
        }
    }
    for (int pass = 0; pass < 2; pass++)
        for (int i = 0; i < nc; i++) {
            int x = con[i][0], y = con[i][1], a = con[i][2], b = con[i][3];
            if (MM[y][x].kind != K_WALL) continue;
            int ra = uf_find(a), rb = uf_find(b);
            int extra = pass == 1 && rnd(100) < (lairnear[y][x] ? 16 : 8);
            if (ra == rb && !extra) continue;
            if (extra) {
                int near = 0;
                for (int d = 0; d < 4; d++) if (mc(x + DX[d] * 2, y + DY[d] * 2)->kind == K_DOOR) near = 1;
                if (near) continue;
            }
            uf[ra] = rb;
            con_carve(x, y, a, b);
        }
    for (int r = 1; r < G.nrooms; r++) {                          /* a lair with one way in: another, if the plan allows */
        if (!(G.lairs >> r & 1)) continue;
        int ways = 0, cand[64], n = 0;
        for (int i = 0; i < nc; i++) {
            int x = con[i][0], y = con[i][1];
            if (mc(x - 1, y)->room != r && mc(x + 1, y)->room != r && mc(x, y - 1)->room != r && mc(x, y + 1)->room != r) continue;
            if (MM[y][x].kind != K_WALL) { ways++; continue; }
            int near = 0;
            for (int d = 0; d < 4; d++) if (mc(x + DX[d] * 2, y + DY[d] * 2)->kind == K_DOOR) near = 1;
            if (!near && n < 64) cand[n++] = i;
        }
#ifdef CRAWL_DEBUG
        fprintf(stderr, "LAIRS level %d: room %d (%d,%d %dx%d plan) ways %d%s\n", G.level, r, mroom[r][0], mroom[r][1], mroom[r][2], mroom[r][3], ways,
                ways < 2 && n ? ", one more made" : "");
#endif
        if (ways >= 2 || !n) continue;
        int i = cand[rnd(n)];
        con_carve(con[i][0], con[i][1], con[i][2], con[i][3]);
    }
    mz_fill_dead_ends();

    /* the locked rooms: gold always, silver from level 3, crimson from level 5 */
    int want = G.level >= 5 ? 3 : (G.level >= 3 ? 2 : 1), vkx[NKEYS], vky[NKEYS];
    G.nvaults = 0;
    for (int k = 0; k < NKEYS; k++) G.vault_room[k] = G.vault_door[k] = -1;
    for (int k = 0; k < want; k++) {
        int room, px, py;
        if (!mz_vault(&px, &py, &room)) break;
        G.vault_room[k] = room; G.nvaults++;
        vkx[k] = mstart(px) + (px & 1); vky[k] = mstart(py) + (py & 1);
        mz_fill_dead_ends();
    }
    int wkx = -1, wky = -1;                                        /* a puzzle's sealed wing (1.0.0b13): a vault the puzzle holds */
    G.pz = pz_level(G.level) ? pz_kind_of(G.level) : -1; G.pz_room = G.pz_door = -1; G.pz_exit = 0;
    if (G.pz >= 0) {
        int room, px, py;
        mz_min = 3;
        if (mz_vault(&px, &py, &room)) { G.pz_room = room; wkx = mstart(px) + (px & 1); wky = mstart(py) + (py & 1); mz_fill_dead_ends(); }
        else G.pz = -1;
        mz_min = 0;
        G.pz_exit = G.pz >= 0 && ((uint32_t)G.level * 2654435761u >> 7) % 3 == 0;   /* P2: on one puzzle level in three the way on lies within */
#ifdef CRAWL_DEBUG
        if (G.pz >= 0 && getenv("CRAWL_WINGEXIT")) G.pz_exit = 1;
#endif
    }
    int tkx = -1, tky = -1;                                        /* a trial's room (1.0.0b13): one way in, its door marked */
    G.tri = tri_level(G.level) ? tri_kind_of(G.level) : -1; G.tri_room = G.tri_door = -1;
    if (G.tri >= 0) {
        int room, px, py, ok = 0;
        mz_min = 3;
        for (mz_max = 15; mz_max <= 35 && !ok; mz_max += 10)       /* a small room if one is free, else a larger */
            if ((ok = mz_vault(&px, &py, &room))) { G.tri_room = room; tkx = mstart(px) + (px & 1); tky = mstart(py) + (py & 1); mz_fill_dead_ends(); }
        if (!ok) G.tri = -1;
        mz_min = 0; mz_max = 15;
    }
    mz_widths();
    mz_draw();
    for (int r = 0; r < G.nrooms; r++) {                          /* the rooms, in cells */
        Room *R = &G.rooms[r];
        R->x = mstart(mroom[r][0]); R->y = mstart(mroom[r][1]);
        R->w = mstart(mroom[r][0] + mroom[r][2] - 1) + 3 - R->x; R->h = mstart(mroom[r][1] + mroom[r][3] - 1) + 3 - R->y;
    }

    /* doors: only where they sit between two walls, never next to each other */
    for (int y = 1; y < MAPN - 1; y++)
        for (int x = 1; x < MAPN - 1; x++) {
            Cell *c = &G.map[y][x];
            if (c->kind != K_DOOR) continue;
            int ax = is_open(x - 1, y) && is_open(x + 1, y) && !is_open(x, y - 1) && !is_open(x, y + 1);
            int ay = is_open(x, y - 1) && is_open(x, y + 1) && !is_open(x - 1, y) && !is_open(x + 1, y);
            int adj = 0, vault = -1, wing = x == wkx && y == wky, trial = x == tkx && y == tky;
            for (int k = 0; k < G.nvaults; k++) if (x == vkx[k] && y == vky[k]) vault = k;
            for (int d = 0; d < 4; d++) if (cell(x + DX[d], y + DY[d])->kind == K_DOOR) adj = 1;
            if (vault < 0 && !wing && !trial && ((!ax && !ay) || adj || G.ndoors >= MAXDOORS - 1)) { c->kind = K_FLOOR; continue; }
            c->door = (int16_t)G.ndoors;
            G.doors[G.ndoors++] = (Door){ x, y, ax, 0.0f, 0.0f, 0, wing ? WINGED : vault >= 0, vault >= 0 ? vault : 0, 0 };
            if (vault >= 0) G.vault_door[vault] = G.ndoors - 1;
            if (wing) G.pz_door = G.ndoors - 1;
            if (trial) G.tri_door = G.ndoors - 1;
        }

    place_secrets();
    gen_dress();
}

/* set dressing: bones, webs, chains and roots in the halls, a totem at some crossings; each room a
 * graveyard, a fire hall, a gibbet room or a plain crypt, the big ones under a chandelier */
static int wall_toward(int x, int y) {                             /* a wall beside (x,y): which way, or -1 */
    for (int d = 0, k = rnd(4); d < 4; d++, k = (k + 1) & 3) if (cell(x + DX[k], y + DY[k])->kind == K_WALL) return k;
    return -1;
}
static int by_way(int x, int y) {                                  /* next to a door, or to where a room opens */
    for (int d = 0; d < 4; d++) {
        const Cell *n = cell(x + DX[d], y + DY[d]);
        if (n->kind == K_DOOR || (n->kind == K_FLOOR && n->theme != G.map[y][x].theme)) return 1;
    }
    return 0;
}
static int block_corner(int x, int y) { return ((x & 3) == 1 || (x & 3) == 3) && ((y & 3) == 1 || (y & 3) == 3); }
/* Footprints (1.0.0b18.1): while dress_level lays a level (g_foot), a low prop takes the part of its cell its art gives it
 * (crawl_prop_foot, crawl_foot_part): the whole from 0.7 of a cell, and always with no wall beside it; the half against a
 * wall it stands by, its long side along it, from 0.45; the quarter in its corner under that, the corner's open side the
 * one away from the block's middle. Choices go by the cell, not the generator's dice, so the level is laid as before. */
static int g_foot, g_foot_n[4];                                    /* laying; and how many whole, half, a quarter, whole in the open */
static float prop_lx, prop_ly;                                     /* where the last prop stands: its light goes there */
static int foot_on(void) {
#ifdef CRAWL_DEBUG
    static int v = -1;
    if (v < 0) { const char *e = getenv("CRAWL_FOOT"); v = !(e && atoi(e) == 0); }   /* 0: every prop whole, as before */
    return v;
#else
    return 1;
#endif
}
static void solid_prop(int x, int y, int spr, int nfr, int kind) {
    G.map[y][x].kind = (uint8_t)kind;
    prop_lx = x + 0.5f; prop_ly = y + 0.5f;
    int part = FOOT_WHOLE, walls = 0;
    if (g_foot && kind == K_PROP && foot_on() && !lane(x, y) && !in_band(y)) {
        part = crawl_foot_part(crawl_prop_foot(spr, nfr));
        for (int d = 0; d < 4; d++) walls |= (cell(x + DX[d], y + DY[d])->kind == K_WALL) << d;
        if (part != FOOT_WHOLE && !walls) { part = FOOT_WHOLE; g_foot_n[3]++; }
    }
    if (part != FOOT_WHOLE) {
        uint32_t h = hash2(x * 7 + 3, y * 13 + G.level);
        int nw = 0, ws[4];
        for (int d = 0; d < 4; d++) if (walls >> d & 1) ws[nw++] = d;
        int d = ws[h % (uint32_t)nw], sx = DX[d], sy = DY[d];       /* the wall it stands by */
        if (part == FOOT_HALF) {
            G.foot[y][x] = (uint8_t)(sx > 0 ? 10 : sx < 0 ? 5 : sy > 0 ? 12 : 3);
            prop_lx = x + 0.5f + 0.25f * sx; prop_ly = y + 0.5f + 0.25f * sy;
        } else {                                                    /* its corner: by a second wall, or away from the block's middle */
            if (!sx) sx = (walls & 1) ? 1 : (walls & 4) ? -1 : (x & 3) == 1 ? -1 : (x & 3) == 3 ? 1 : ((h >> 8) & 1 ? 1 : -1);
            if (!sy) sy = (walls & 2) ? 1 : (walls & 8) ? -1 : (y & 3) == 1 ? -1 : (y & 3) == 3 ? 1 : ((h >> 9) & 1 ? 1 : -1);
            G.foot[y][x] = (uint8_t)(1 << ((sx > 0) | (sy > 0) << 1));
            prop_lx = x + 0.5f + 0.25f * sx; prop_ly = y + 0.5f + 0.25f * sy;
        }
    }
    if (g_foot && kind == K_PROP) g_foot_n[part]++;
#ifdef CRAWL_DEBUG
    if (part != FOOT_WHOLE && getenv("CRAWL_FOOTLIST")) fprintf(stderr, "FOOT at %d,%d mask %d sprite %d foot %d\n", x, y, G.foot[y][x], spr, crawl_prop_foot(spr, nfr));
#endif
    add_deco_ex(prop_lx, prop_ly, spr, nfr, 0);
}
static void fire_pit(int x, int y) { solid_prop(x, y, SP_FIREPIT_A, 3, K_PROP); light_static(prop_lx, prop_ly, 1.7f, 5.6f); }
static void totem(int x, int y) { solid_prop(x, y, SP_TOTEM_A, 2, K_PILLAR); light_static(x + 0.5f, y + 0.5f, 1.0f, 3.8f); }
static void chandelier(int x, int y) { add_deco_ex(x + 0.5f, y + 0.5f, SP_CHANDELIER_A, 2, DF_SWAY); light_static(x + 0.5f, y + 0.5f, 1.35f, 6.0f); }
static void candlestand(int x, int y) { solid_prop(x, y, SP_CANDLESTAND_A, 2, K_PROP); light_static(prop_lx, prop_ly, 0.95f, 3.6f); }
static void lantern(int x, int y) { add_deco_ex(x + 0.5f, y + 0.5f, SP_LANTERN_A, 2, DF_SWAY); light_static(x + 0.5f, y + 0.5f, 1.1f, 5.0f); }
static void brazier(int x, int y) { solid_prop(x, y, SP_BRAZIER_A, 3, K_PROP); light_static(prop_lx, prop_ly, 1.5f, 5.2f); }
static void minelamp(int x, int y) { solid_prop(x, y, SP_MINELAMP_A, 2, K_PILLAR); light_static(x + 0.5f, y + 0.5f, 0.95f, 4.2f); }
static void crystals(int x, int y) {                               /* a steady glow, not a flame */
    int n = G.nlights;
    solid_prop(x, y, SP_CRYSTAL_A, 2, K_PROP); light_static(prop_lx, prop_ly, 1.15f, 4.6f);
    if (G.nlights > n) G.lights[n].phase = -1000.0f;
}
static void magma(int x, int y) { solid_prop(x, y, SP_MAGMA_A, 3, K_PROP); light_static(prop_lx, prop_ly, 1.6f, 5.4f); }

/* The dressing comes in as you go down (PLAN-bestiary, section 8): each set lights and furnishes
 * itself in its own way, and some things show a level or two before what they warn of. In the second
 * time down and after, everything a set has is there from its first level. */
static int since(int level) { return G.level >= level || crawl_loop(G.level) > 0; }
static int set_cap = 1 << 20;                                      /* step 8: the heads a set piece may place, from what is left of the
                                                                      level's budget (the old placements leave it be) */
/* the set pieces (docs/PLAN-ai.md, section 16): what a creature came with */
enum { SET_NONE, SET_WANDER, SET_NEW, SET_POST, SET_GUARD, SET_WATCH, SET_WEBS, SET_BURIED, SET_IMPS, SET_GARGS, SET_BROOD,
       SET_FILE, SET_ROOST, SET_HUNT, SET_AMBUSH, SET_BAND, NSET };
static int last_set;                                               /* the set piece of what last struck you */
static int short_lv = -1, short_n[NSET], short_hunt;               /* a level's deaths: the set pieces that killed you there, which come
                                                                      back a head short each time; and a hunt that did isn't let loose again */

/* a light for a hall crossing, by the set */
static void crossing_light(int x, int y) {
    switch (crawl_set(G.level)) {
    case 0: if (since(3)) totem(x, y); else candlestand(x, y); break;
    case 1: if (rnd(2)) totem(x, y); else lantern(x, y); break;
    case 2: if (rnd(2)) minelamp(x, y); else brazier(x, y); break;
    default: if (rnd(3)) crystals(x, y); else magma(x, y); break;
    }
}
/* a room's own light, in a corner of one of its blocks */
static void room_light(int x, int y, int vault) {
    switch (crawl_set(G.level)) {
    case 0: if (vault || rnd(3) || !since(3)) chandelier(x, y); else totem(x, y); break;
    case 1: if (vault || rnd(3)) chandelier(x, y); else lantern(x, y); break;
    case 2: if (rnd(2)) brazier(x, y); else minelamp(x, y); break;
    default: if (vault || rnd(3)) crystals(x, y); else magma(x, y); break;
    }
}
/* a fire hall's fire */
static void hall_fire(int x, int y) {
    int set = crawl_set(G.level);
    if (set <= 1) fire_pit(x, y); else if (set == 2) brazier(x, y); else magma(x, y);
}

/* something on the floor of a hall, by a wall: mostly bones, now and then a sign of what is coming */
static int hall_litter(void) {
    int set = crawl_set(G.level), k = rnd(100);
    if (set <= 1 && since(3) && k < 6) return SP_BROKEN_ARMS;      /* shield-bearers from 4 */
    if (set >= 1 && set <= 2 && since(7) && k < 12) return SP_COLLAR;   /* hounds from 8 */
    if (set >= 2 && since(11) && k < 18) return SP_CARCASS;         /* ghouls from 12 */
    if (set == 2 && since(12) && k < 23) return SP_HELM;            /* knights from 13 */
    return rnd(4) ? SP_BONES1 : SP_BONES2;
}

static void dress_level_props(void);
static void dress_level(void) {                                    /* the props laid with their footprints (1.0.0b18.1) */
    g_foot = 1; memset(g_foot_n, 0, sizeof g_foot_n);
    dress_level_props();
    g_foot = 0;
#ifdef CRAWL_DEBUG
    if (!PK.depth) fprintf(stderr, "FOOT level %d: %d whole (%d of them in the open), %d half, %d a quarter\n", G.level, g_foot_n[0], g_foot_n[3], g_foot_n[1], g_foot_n[2]);
#endif
}
static void dress_level_props(void) {
    float dk = decay();
    int set = crawl_set(G.level);
    /* the halls */
    for (int y = 1; y < MAPN - 1; y++)
        for (int x = 1; x < MAPN - 1; x++) {
            Cell *c = &G.map[y][x];
            if (c->kind != K_FLOOR || c->theme != TH_CORRIDOR || c->secret) continue;
            int o[4];
            for (int d = 0; d < 4; d++) o[d] = is_open(x + DX[d], y + DY[d]);
            int n = o[0] + o[1] + o[2] + o[3];
            float cx = x + 0.5f, cy = y + 0.5f;
            if (n == 2 && !(o[0] && o[2]) && !(o[1] && o[3])) {      /* a corner: cobweb in the dead corner */
                if (rndf() < 0.45f + 0.35f * dk) add_deco(cx + (o[0] ? -0.34f : 0.34f), cy + (o[1] ? -0.34f : 0.34f), SP_COBWEB);
                continue;
            }
            int wd = wall_toward(x, y);
            if (lane(x, y)) {                                      /* on a lane: only things kept to the side */
                if (wd < 0 || n > 2) continue;
                if (rndf() < 0.09f + 0.08f * dk) add_deco(cx + DX[wd] * 0.31f + DY[wd] * (rndf() - 0.5f) * 0.3f, cy + DY[wd] * 0.31f + DX[wd] * (rndf() - 0.5f) * 0.3f, hall_litter());
                else if (rndf() < 0.05f) add_deco(cx + DX[wd] * 0.34f, cy + DY[wd] * 0.34f, SP_CHAINS);
                else if (since(8) && rndf() < 0.025f) add_deco(cx + DX[wd] * 0.3f, cy + DY[wd] * 0.3f, SP_ICICLES);   /* the Wraith has been by */
                continue;
            }
            float k = rndf();                                      /* beside the lane, in the wide halls */
            if (k < 0.08f) add_deco_ex(cx, cy, set <= 1 && since(3) && rnd(3) ? SP_HOOKS : SP_CHAINS, 1, DF_SWAY);
            else if (k < 0.13f + 0.06f * dk) { if (since(6) && set <= 2) add_deco_ex(cx, cy, SP_ROOTS, 1, DF_SWAY); }
            else if (k < 0.17f + 0.06f * dk) { if (since(4)) add_deco_ex(cx, cy, SP_WEBSHEET, 1, DF_SWAY); }
            else if (set == 1 && k < 0.24f + 0.06f * dk) add_deco(cx, cy, rnd(4) ? SP_PUDDLE_A : SP_PUDDLE_B);   /* water underfoot */
            else if (set >= 1 && set <= 2 && wd >= 0 && k < 0.27f + 0.04f * dk) solid_prop(x, y, SP_STALAG, 1, K_PILLAR);   /* flowstone */
            else if (set == 2 && k < 0.31f) add_deco(cx, cy, SP_FUNGUS);
            else if (set == 3 && wd >= 0 && k < 0.31f) solid_prop(x, y, SP_OBSIDIAN, 1, K_PROP);
            else if (wd >= 0 && k < 0.40f + 0.1f * dk) add_deco(cx + DX[wd] * 0.3f, cy + DY[wd] * 0.3f, hall_litter());
        }
    /* a light at some crossings of the wide halls */
    for (int my = 1; my < MG; my += 2)
        for (int mx = 1; mx < MG; mx += 2) {
            if (!mopen(mx, my) || MM[my][mx].room >= 0 || mdeg(mx, my) < 3 || rnd(10) >= 4) continue;
            int x0 = mstart(mx), y0 = mstart(my), k = rnd(4);
            for (int t = 0; t < 4; t++, k = (k + 1) & 3) {
                int x = x0 + (k & 1) * 2, y = y0 + (k >> 1) * 2;
                if (G.map[y][x].kind != K_FLOOR || by_way(x, y)) continue;
                crossing_light(x, y);
                break;
            }
        }
    /* the rooms: a plain crypt, a graveyard, a fire hall or a gibbet room, or what the deeper sets
     * make of them (a cave, a mine, a web room; a crystal cave, an altar room, a field of glass) */
    for (int r = 0; r < G.nrooms; r++) {
        Room *R = &G.rooms[r];
        int vault = 0;
        for (int k = 0; k < G.nvaults; k++) vault |= G.vault_room[k] == r;
        int big = mroom[r][2] >= 3 && mroom[r][3] >= 3, style = vault ? 0 : rnd(10);
        style = style < 3 ? 0 : (style < 6 ? 1 : (style < 8 ? 2 : 3));   /* crypt, graveyard, fire hall, gibbets */
        if (G.faced && !vault) style = R->rtype == RT_UNDER ? 2 + (int)(hash2(r, G.level) & 1) : RTYPES[R->rtype].style;   /* as its type has it */
        if (style == 3 && set <= 1 && !since(3)) style = 0;        /* no gibbets on the first two levels */
        if (r == 0 && style == 1) style = 2;                       /* the first room has a fire to start by */
        int lit = 0;
        if (big && !vault && set <= 2) {                           /* lights where four blocks meet, hung */
            for (int my = mroom[r][1] + 1; my < mroom[r][1] + mroom[r][3] - 1; my += 2)
                for (int mx = mroom[r][0] + 1; mx < mroom[r][0] + mroom[r][2] - 1; mx += 2) {
                    int x = mstart(mx), y = mstart(my);
                    if (G.map[y][x].kind != K_FLOOR || rnd(10) >= (style == 2 ? 4 : 7)) continue;
                    if (style == 2 && !lit && set <= 1) fire_pit(x, y);
                    else if (set <= 1) chandelier(x, y);
                    else lantern(x, y);
                    lit++;
                }
        }
        if (style == 2 && !lit) {                                  /* a fire hall's fire, in a corner of its middle block */
            for (int tries = 0; tries < 20; tries++) {
                int x = R->x + rnd(R->w), y = R->y + rnd(R->h);
                if (G.map[y][x].kind != K_FLOOR || !block_corner(x, y) || by_way(x, y)) continue;
                hall_fire(x, y); lit = 1;
                break;
            }
        }
        for (int tries = 0; tries < 40 && lit < (big ? 2 : 1); tries++) {   /* every room some light of its own; a big one two */
            int x = R->x + rnd(R->w), y = R->y + rnd(R->h);
            if (G.map[y][x].kind != K_FLOOR || !block_corner(x, y) || by_way(x, y)) continue;
            room_light(x, y, vault);
            lit++;
        }
        int pools = 0;
        for (int y = R->y; y < R->y + R->h; y++)
            for (int x = R->x; x < R->x + R->w; x++) {
                if (G.map[y][x].kind != K_FLOOR || lane(x, y)) continue;
                int corner = block_corner(x, y) && !by_way(x, y);
                if (G.faced && R->rtype == RT_VENT && corner && pools < 3 && G.nlights < MAXLIGHTS - 4 && rnd(3) == 0) { magma(x, y); pools++; continue; }   /* magma welling up */
                if (style == 1 && corner && rndf() < 0.6f) {       /* rows of graves; a mine's carts and props; an altar room */
                    if (set <= 1) solid_prop(x, y, SP_TOMB_ROUND + rnd(4), 1, K_PROP);
                    else if (set == 2 && G.faced && R->rtype == RT_CHAPEL) {   /* the cult's altars, candles between */
                        if (rnd(4) == 0) { solid_prop(x, y, SP_ALTAR_A, 2, K_PROP); light_static(prop_lx, prop_ly, 0.7f, 3.0f); }
                        else if (G.nlights < MAXLIGHTS - 4 && rnd(2)) { add_deco(x + 0.5f, y + 0.5f, SP_CANDLES); light_static(x + 0.5f, y + 0.5f, 0.8f, 2.6f); }
                    }
                    else if (set == 2) { if (rnd(3)) solid_prop(x, y, SP_CART, 1, K_PROP); else minelamp(x, y); }
                    else if (since(17) && rnd(2)) { solid_prop(x, y, SP_ALTAR_A, 2, K_PROP); light_static(prop_lx, prop_ly, 0.8f, 3.2f); }
                    else if (since(18)) solid_prop(x, y, SP_GARG_PERCH, 1, K_PROP);   /* a statue: gargoyles from 19 */
                    else solid_prop(x, y, SP_OBSIDIAN, 1, K_PROP);
                    continue;
                }
                if (style == 3 && corner && rndf() < 0.3f) {       /* cages over the floor; deeper, webs and their sacs, or glass */
                    if (set <= 1) add_deco_ex(x + 0.5f, y + 0.5f, since(4) && rnd(3) ? SP_GIBBET_SKEL : SP_GIBBET, 1, DF_SWAY);
                    else if (set == 2) add_deco_ex(x + 0.5f, y + 0.5f, since(13) && rnd(2) ? SP_SAC_A : SP_WEBSHEET, 1, DF_SWAY);
                    else solid_prop(x, y, SP_OBSIDIAN, 1, K_PROP);
                    continue;
                }
                if (set == 1 && style == 1 && rndf() < 0.1f) { add_deco(x + 0.5f, y + 0.5f, rnd(3) ? SP_PUDDLE_A : SP_PUDDLE_B); continue; }   /* the graves have flooded */
                if (set == 1 && since(9) && style == 0 && corner && rnd(30) == 0) { add_deco_ex(x + 0.5f, y + 0.5f, SP_BELL_DEAD, 1, 0); continue; }   /* dead, still: not one to ring (1.0.0b18.1) */
                if (set == 2 && style == 0 && corner && since(12) && rnd(12) == 0) { solid_prop(x, y, SP_ARMOURSTAND, 1, K_PROP); continue; }
                if (set == 3 && style == 0 && corner && rnd(4) == 0) { crystals(x, y); continue; }
                if ((style == 1 || style == 3) && since(4) && rndf() < 0.04f + 0.05f * dk) { add_deco_ex(x + 0.5f, y + 0.5f, since(6) && set <= 2 && rnd(2) ? SP_ROOTS : SP_WEBSHEET, 1, DF_SWAY); continue; }
                int d = wall_toward(x, y);
                if (d < 0 || by_way(x, y) || rndf() > (style == 0 ? 0.3f : 0.12f) + 0.1f * dk) continue;
                int spr, k = rnd(10);
                if (k == 0 && G.nlights < MAXLIGHTS - 4) {
                    spr = SP_CANDLES;
                    light_static(x + 0.5f + DX[d] * 0.3f, y + 0.5f + DY[d] * 0.3f, 0.95f, 3.0f);
                } else if (R->theme == TH_OSSUARY) spr = k < 5 ? SP_SKULLPILE : SP_BONES2;
                else if (set == 2 && k < 3) spr = since(13) && k == 1 ? SP_SAC_A : (since(11) && k == 2 ? SP_CARCASS : SP_FUNGUS);
                else if (set == 3 && k < 3) spr = k == 1 ? SP_SKULLPILE : SP_BONES2;
                else spr = k < 3 ? SP_URN : (k < 6 ? SP_BONES2 : (k < 8 ? SP_SKULLPILE : hall_litter()));
                add_deco(x + 0.5f + DX[d] * 0.3f, y + 0.5f + DY[d] * 0.3f, spr);
            }
    }
}

