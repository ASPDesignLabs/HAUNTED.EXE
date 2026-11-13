/* A part of scene_crawl.c (docs/PLAN-tidy.md): it is #included there in its place and never compiled on its own.
 * What a level is given: the arena stamped, treasure, keys and the exit, and the creatures that wait in their places (wall skulls, spiders, the buried, imps, gargoyles, guardians). */

/* ================================================================== */
/* Treasure, the key, a potion                                         */
/* ================================================================== */

/* the boss's strip of the map: nothing of the level's own is put there */
static int in_band(int y) { return G.boss >= 0 && !PK.depth && y >= BAND_Y0; }

/* the boss's antechamber and arena, from its map, into the strip of the map the plan kept for them; it
 * is stamped last, while the strip is still solid, so nothing of the level spills into it */
static void stamp_arena(void) {
    const BossDef *B = &BOSSES[G.boss];
    static const int THEME[NBOSS] = { TH_CRYPT, TH_MOSS, TH_OSSUARY, TH_CORRIDOR };
    G.bnfont = G.bnspawn = 0; G.barch = -1; G.bhome_x = G.bhome_y = G.bstart_x = G.bstart_y = -1;
    G.exit_fx = G.exit_fy = -1; G.exit_in_vault = -1;
    for (int r = 0; r < BAND_H; r++)
        for (int x = 0; x < MAPN; x++) {
            int y = BAND_Y0 + r;
            char ch = B->map[r][x];
            Cell *c = &G.map[y][x];
            memset(c, 0, sizeof *c);
            c->region = -1; c->door = -1; c->visits = 1;           /* nothing here for the autopilot to explore */
            G.fog[y][x] = 0.0f;
            if (ch == '#' || ch == 'E' || ch == 'G' || ch == 'h' || ch == 'F' || ch == 'v') {
                c->kind = K_WALL;
                c->tex = (uint8_t)(ch == 'E' ? T_EXIT0 : ch == 'G' ? T_GATE : ch == 'h' ? T_NICHE : ch == 'F' ? T_FONT_DRY : ch == 'v' ? T_GRATE
                                   : theme_tex(THEME[G.boss], hash2(x, y)));
                if (ch == 'E') { G.exit_wx = x; G.exit_wy = y; }
                if (ch == 'F' && G.bnfont < MAXFONTS) { G.bfont_x[G.bnfont] = x; G.bfont_y[G.bnfont] = y; G.bfont_full[G.bnfont++] = 0; }
                if ((ch == 'G' || ch == 'h') && G.bnspawn < MAXSPAWN) { G.bspawn_x[G.bnspawn] = x; G.bspawn_y[G.bnspawn++] = y; }
                continue;
            }
            c->kind = K_FLOOR; c->region = ARENA_REGION; c->theme = (uint8_t)THEME[G.boss];
            switch (ch) {
            case '@': c->kind = K_DOOR; break;
            case 'a': G.bstart_x = x; G.bstart_y = y; break;
            case 'O': solid_prop(x, y, B->pillar, 1, K_PILLAR); break;
            case '=': case ':': solid_prop(x, y, B->low, 1, K_PROP); break;
            case '%': add_deco(x + 0.5f, y + 0.5f, G.boss == BOSS_DEACON ? SP_BONES2 : SP_SKULLPILE); if (G.bnspawn < MAXSPAWN) { G.bspawn_x[G.bnspawn] = x; G.bspawn_y[G.bnspawn++] = y; } break;
            case 'w': add_deco_ex(x + 0.5f, y + 0.5f, SP_WEBSHEET, 1, DF_SWAY); break;
            case 'K': case 'S': case 'U': case 'W': G.bhome_x = x; G.bhome_y = y; break;
            default: break;
            }
        }
    /* what stands in the walls is used from the floor before it: the exit, the fonts, the gates and burrows */
    #define FRONT(wx, wy, fx, fy, dir) do { for (int d_ = 0; d_ < 4; d_++) { int nx_ = (wx) + DX[d_], ny_ = (wy) + DY[d_]; \
        if (ny_ >= BAND_Y0 && nx_ >= ARENA_X0 && G.map[ny_][nx_].kind == K_FLOOR) { fx = nx_; fy = ny_; dir = (d_ + 2) & 3; break; } } } while (0)
    { int fx = -1, fy = -1, dir = 0; FRONT(G.exit_wx, G.exit_wy, fx, fy, dir); G.exit_fx = fx; G.exit_fy = fy; G.exit_dir = dir; }
    for (int i = 0; i < G.bnfont; i++) { int dir = 0; G.bfont_fx[i] = G.bfont_fy[i] = -1; FRONT(G.bfont_x[i], G.bfont_y[i], G.bfont_fx[i], G.bfont_fy[i], dir); (void)dir; }
    for (int i = 0; i < G.bnspawn; i++)
        if (G.map[G.bspawn_y[i]][G.bspawn_x[i]].kind == K_WALL) { int fx = G.bspawn_x[i], fy = G.bspawn_y[i], dir = 0; FRONT(G.bspawn_x[i], G.bspawn_y[i], fx, fy, dir); (void)dir; G.bspawn_x[i] = fx; G.bspawn_y[i] = fy; }
    #undef FRONT
    for (int y = BAND_Y0; y < MAPN; y++)                            /* the arch: a door that seals behind you */
        for (int x = 0; x < MAPN; x++)
            if (G.map[y][x].kind == K_DOOR && G.ndoors < MAXDOORS) {
                G.map[y][x].door = (int16_t)G.ndoors; G.barch = G.ndoors;
                G.doors[G.ndoors++] = (Door){ x, y, 1, 0.0f, 0.0f, 0, 0, 2, 0 };
            }
    /* light: a candle in the antechamber; braziers; torches along the arena's long walls, but for the
     * Wraith's, which has only its braziers */
    add_deco(6.5f, 31.35f, SP_CANDLES); light_static(6.5f, 31.6f, 1.0f, 4.2f);
    for (int r = 0; r < BAND_H; r++)
        for (int x = 0; x < MAPN; x++) {
            if (B->map[r][x] != 'B') continue;
            if (G.boss != BOSS_WRAITH || G.bbz_n >= 8) { brazier(x, BAND_Y0 + r); continue; }
            solid_prop(x, BAND_Y0 + r, SP_BRAZIER_A, 3, K_PROP);   /* the Wraith's: lit each frame, so they can go out */
            G.bbz_x[G.bbz_n] = x; G.bbz_y[G.bbz_n] = BAND_Y0 + r; G.bbz_lit[G.bbz_n] = 1; G.bbz_deco[G.bbz_n++] = G.ndecos - 1;
        }
    if (G.boss != BOSS_WRAITH)
        for (int x = ARENA_X0 + 5; x < MAPN - 6; x += 8)
            for (int k = 0; k < 2; k++) {
                int wy = k ? MAPN - 2 : BAND_Y0, fy = k ? wy - 1 : wy + 1;
                Cell *w = &G.map[wy][x];
                if (w->kind != K_WALL || w->tex == T_GATE || w->tex == T_NICHE || w->tex == T_FONT_DRY || G.map[fy][x].kind != K_FLOOR) continue;
                w->tex = T_TORCH0;
                light_static(x + 0.5f, wy + 0.5f + (k ? -0.42f : 0.42f), G.boss == BOSS_SPIDER ? 1.1f : 1.55f, 4.6f);
            }
    light_static(ARENA_X0 + 0.6f, 34.5f, 0.9f, 3.6f);               /* the arch, seen from inside */
    G.bnout = 0;                                                   /* the outlets the bile comes from: drains in the walls, and the well */
    for (int r = 0; r < BAND_H && G.bnout < MAXOUT; r++)
        for (int x = 0; x < MAPN && G.bnout < MAXOUT; x++) {
            char ch = B->map[r][x];
            int y = BAND_Y0 + r, sx = -1, sy = -1;
            if (ch == 'v') { for (int d = 0; d < 4; d++) if (in_arena((float)(x + DX[d]), (float)(y + DY[d])) && G.map[y + DY[d]][x + DX[d]].kind == K_FLOOR) { sx = x + DX[d]; sy = y + DY[d]; } }
            else if (ch == ':' && B->map[r - 1][x] != ':' && B->map[r][x - 1] != ':') { sx = x - 1; sy = y + 1; }   /* the well: from its west side */
            if (sx < 0) continue;
            G.bout_n[G.bnout] = (uint8_t)hz_patch(sx, sy, ch == ':' ? 2 : 3, G.bout_cx[G.bnout], G.bout_cy[G.bnout], G.bout_cd[G.bnout], ch == ':' ? 14 : 18);
            G.bout_haz[G.bnout++] = -1;
        }
    if (G.boss == BOSS_KNIGHT) { chandelier(21, 36); chandelier(31, 36); }   /* the lists under their candles, so it can be seen */
    else if (G.boss == BOSS_SPIDER) { lantern(19, 36); lantern(33, 36); }    /* the cistern, dimly */
    else if (G.boss == BOSS_DEACON) {                              /* the chapel, lit for the rite */
        static const int CH[5][2] = { { 17, 33 }, { 35, 33 }, { 26, 36 }, { 20, 39 }, { 32, 39 } };
        for (int i = 0; i < 5; i++) { chandelier(CH[i][0], CH[i][1]); light_static(CH[i][0] + 0.5f, CH[i][1] + 0.5f, 0.9f, 7.0f); }
    }
}

static int item_at(int x, int y) {
    for (int i = 0; i < G.nitems; i++) if (G.items[i].alive && G.items[i].cx == x && G.items[i].cy == y) return 1;
    return 0;
}

static int free_floor(int x, int y) {
    if (cell(x, y)->kind != K_FLOOR || cell(x, y)->secret || G.bulky[y][x] || item_at(x, y) || lane_dist(x, y) > 1 || in_band(y)) return 0;   /* within reach of a lane */
    for (int d = 0; d < 4; d++) if (cell(x + DX[d], y + DY[d])->kind == K_DOOR) return 0;
    return 1;
}

/* treasure is worth more the deeper it lies */
static int treasure_value(int type) { return (int)(ITEM_VALUE[type] * (1.0f + 0.2f * (float)(G.level - 1)) + 0.5f); }

static int vault_of_room(int x, int y) {
    for (int k = 0; k < G.nvaults; k++) if (G.vault_room[k] >= 0 && in_room(&G.rooms[G.vault_room[k]], x, y)) return k;
    return -1;
}

static Item *add_item(int x, int y, int type) {
    if (G.nitems >= MAXITEMS) return NULL;
    Item *it = &G.items[G.nitems++];
    memset(it, 0, sizeof *it);
    it->alive = 1; it->type = type; it->cx = x; it->cy = y;
    it->x = x + 0.5f + (rndf() - 0.5f) * 0.16f; it->y = y + 0.5f + (rndf() - 0.5f) * 0.16f;
    it->phase = rndf() * 10.0f;
    it->lx = x; it->ly = y;                                        /* the lane it is taken from: one running past it */
    int best = -1;
    for (int yy = y - 1; yy <= y + 1; yy++)
        for (int xx = x - 1; xx <= x + 1; xx++) {
            if (!lane(xx, yy) || !is_open(xx, yy)) continue;
            int through = 0;
            for (int d = 0; d < 4; d++) through += lane(xx + DX[d], yy + DY[d]) && is_open(xx + DX[d], yy + DY[d]);
            int sc = through * 4 - abs(xx - x) - abs(yy - y);
            if (sc > best) { best = sc; it->lx = xx; it->ly = yy; }
        }
    if (type < IT_KEY) { G.items_total++; G.value_total += treasure_value(type); }
    return it;
}

static int random_treasure(void) { int k = rnd(20); return k < 9 ? IT_COINS : (k < 14 ? IT_CHALICE : (k < 18 ? IT_GEMS : IT_CROWN)); }

/* The final door: set in a wall at the far end of the level, or now and then inside the vault. */
static void place_exit(int sx, int sy) {
    static int dist[MAPN][MAPN], qx[MAPN * MAPN], qy[MAPN * MAPN];
    for (int y = 0; y < MAPN; y++) for (int x = 0; x < MAPN; x++) dist[y][x] = -1;
    int qh = 0, qt = 0, maxd = 0;
    dist[sy][sx] = 0; qx[qt] = sx; qy[qt] = sy; qt++;
    while (qh < qt) {
        int x = qx[qh], y = qy[qh]; qh++;
        if (dist[y][x] > maxd) maxd = dist[y][x];
        for (int d = 0; d < 4; d++) {
            int nx = x + DX[d], ny = y + DY[d];
            if (!is_open(nx, ny) || dist[ny][nx] >= 0) continue;
            dist[ny][nx] = dist[y][x] + 1; qx[qt] = nx; qy[qt] = ny; qt++;
        }
    }
    int want_vault = G.nvaults > 0 && rnd(100) < 45 && !G.pz_exit, xv = G.nvaults > 0 ? rnd(G.nvaults) : -1;
    for (int pass = 0; pass < 2; pass++) {
        int best = -1, bd = 0; float bs = -1e9f;
        for (int y = 1; y < MAPN - 1; y++)
            for (int x = 1; x < MAPN - 1; x++) {
                if (G.map[y][x].kind != K_FLOOR || dist[y][x] < 0 || G.bulky[y][x] || (x == sx && y == sy) || !lane(x, y)) continue;
                int inv = vault_of_room(x, y);
                if (G.pz_exit ? !in_wing(x, y) : (in_wing(x, y) || (want_vault ? inv != xv : inv >= 0))) continue;   /* the wing's, or none of it */
                if (in_trial(x, y)) continue;                      /* never in a trial's room */
                for (int d = 0; d < 4; d++) {
                    int wx = x + DX[d], wy = y + DY[d];
                    Cell *w = cell(wx, wy);
                    if (w->kind != K_WALL || (w->tex >= T_TORCH0 && w->tex <= T_TORCH2) || wx < 0 || wy < 0 || wx >= MAPN || wy >= MAPN) continue;
                    if (DX[d] ? (y & 3) != 2 : (x & 3) != 2) continue;     /* the lane runs into it */
                    int open = 0;
                    for (int e = 0; e < 4; e++) open += is_open(wx + DX[e], wy + DY[e]) || cell(wx + DX[e], wy + DY[e])->kind >= K_PILLAR;
                    if (open != 1) continue;                           /* only one face, so it can't be seen from behind */
                    float sc = want_vault ? rndf() : (float)dist[y][x] + rndf() * 5.0f;
                    if (sc > bs) { bs = sc; best = (y * MAPN + x) * 4 + d; bd = d; }
                }
            }
        if (best < 0) { want_vault = 0; if (G.pz_exit && pass == 0) { G.pz_exit = 0; } continue; }
        int x = (best / 4) % MAPN, y = (best / 4) / MAPN;
        G.exit_fx = x; G.exit_fy = y; G.exit_dir = bd;
        G.exit_wx = x + DX[bd]; G.exit_wy = y + DY[bd];
        G.exit_in_vault = want_vault ? xv : -1;
        G.map[G.exit_wy][G.exit_wx].tex = T_EXIT0;
        light_static(x + 0.5f + DX[bd] * 0.35f, y + 0.5f + DY[bd] * 0.35f, 0.9f, 3.2f);   /* its runes light the stone */
        return;
    }
    G.exit_fx = G.exit_fy = -1;
}

/* a small thing in what a prop leaves free of its cell (1.0.0b18.1): half of it or more always is. Beside a lane, beside floor
 * reached without a key (dist, at least dmin steps from the start), no door by it; the thing lies at the free part's middle */
static int part_spot(int x, int y, int dist[MAPN][MAPN], int dmin) {
    if (!foot_of(x, y) || G.map[y][x].secret || G.bulky[y][x] || item_at(x, y) || lane_dist(x, y) > 1 || in_band(y)) return 0;
    int reach = 0;
    for (int d = 0; d < 4; d++) {
        const Cell *n = cell(x + DX[d], y + DY[d]);
        if (n->kind == K_DOOR) return 0;
        reach |= n->kind == K_FLOOR && dist[y + DY[d]][x + DX[d]] >= dmin;
    }
    return reach;
}
static void part_lay(Item *it) {
    if (!it) return;
    float gx, gy;
    cell_pass(it->cx, it->cy, &gx, &gy);
    it->x = gx; it->y = gy;
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT part_item %d at %.2f,%.2f\n", it->type, it->x, it->y);
#endif
}
static int room_cell(int room, int *ox, int *oy) {                 /* a free floor cell in a room */
    const Room *R = &G.rooms[room];
    for (int tries = 0; tries < 120; tries++) {
        int x = R->x + rnd(R->w), y = R->y + rnd(R->h);
        if (G.map[y][x].kind != K_FLOOR || G.map[y][x].secret || G.bulky[y][x] || item_at(x, y) || lane_dist(x, y) > 1) continue;
        *ox = x; *oy = y;
        return 1;
    }
    return 0;
}

static void place_items(int sx, int sy) {
    G.nitems = 0; G.items_total = 0; G.items_got = 0; G.value_total = G.value_got = 0;
    static int dist[MAPN][MAPN], qx[MAPN * MAPN], qy[MAPN * MAPN];
    for (int y = 0; y < MAPN; y++) for (int x = 0; x < MAPN; x++) dist[y][x] = -1;
    int qh = 0, qt = 0, maxd = 0;
    dist[sy][sx] = 0; qx[qt] = sx; qy[qt] = sy; qt++;
    while (qh < qt) {                                              /* reachable without any key */
        int x = qx[qh], y = qy[qh]; qh++;
        if (dist[y][x] > maxd) maxd = dist[y][x];
        for (int d = 0; d < 4; d++) {
            int nx = x + DX[d], ny = y + DY[d];
            if (!passable(nx, ny) || dist[ny][nx] >= 0) continue;
            dist[ny][nx] = dist[y][x] + 1; qx[qt] = nx; qy[qt] = ny; qt++;
        }
    }
    for (int r = 1; r < G.nrooms; r++) {                           /* a little in most rooms, more deeper down */
        if (vault_of_room(G.rooms[r].x, G.rooms[r].y) >= 0) continue;
        Room *R = &G.rooms[r];
        int n = rnd(3) + (G.level >= 4);
        for (int k = 0, tries = 0; k < n && tries < 30; tries++) {
            int x = R->x + rnd(R->w), y = R->y + rnd(R->h);
            if (part_spot(x, y, dist, 0)) { part_lay(add_item(x, y, random_treasure())); k++; continue; }   /* by a prop (1.0.0b18.1) */
            if (dist[y][x] < 0 || !free_floor(x, y)) continue;
            add_item(x, y, random_treasure()); k++;
        }
    }
    for (int i = 0, placed = 0; i < 400 && placed < 7 + G.level / 2; i++) {   /* and along the halls */
        int x = 1 + rnd(MAPN - 2), y = 1 + rnd(MAPN - 2);
        if (G.map[y][x].theme != TH_CORRIDOR || dist[y][x] < 4 || !free_floor(x, y)) continue;
        add_item(x, y, rnd(3) ? IT_COINS : IT_CHALICE); placed++;
    }
    static const int HOARD[10] = { IT_CHEST, IT_CROWN, IT_GEMS, IT_COINS, IT_COINS, IT_CHALICE, IT_GEMS, IT_CHALICE, IT_CROWN, IT_GEMS };
    for (int k = 0; k < G.nvaults; k++) {                          /* the hoards: richer behind silver, richer still behind crimson */
        int n = k == 0 ? 6 : (k == 1 ? 8 : 10), x, y;
        for (int i = 0; i < n; i++) if (room_cell(G.vault_room[k], &x, &y)) add_item(x, y, HOARD[i]);
        for (int i = 0; i < k + 1 && k >= 1; i++) if (room_cell(G.vault_room[k], &x, &y)) add_item(x, y, IT_URN);
        if (k == 2 && rnd(4) == 0 && room_cell(G.vault_room[k], &x, &y)) add_item(x, y, IT_RING);
    }
    /* the keys: usually out in the halls, a short walk from their door; now and then locked in the vault before */
    static int ddist[MAPN][MAPN];
    for (int k = 0; k < G.nvaults; k++) {
        int x, y;
        if (k > 0 && rnd(100) < 35 && room_cell(G.vault_room[k - 1], &x, &y)) { Item *it = add_item(x, y, IT_KEY); if (it) it->color = k; continue; }
        const Door *vd = &G.doors[G.vault_door[k]];
        for (int yy = 0; yy < MAPN; yy++) for (int xx = 0; xx < MAPN; xx++) ddist[yy][xx] = -1;
        qh = qt = 0;
        for (int d = 0; d < 4; d++) {
            int nx = vd->x + DX[d], ny = vd->y + DY[d];
            if (passable(nx, ny) && dist[ny][nx] >= 0) { ddist[ny][nx] = 1; qx[qt] = nx; qy[qt] = ny; qt++; }
        }
        while (qh < qt) {
            int xx = qx[qh], yy = qy[qh]; qh++;
            for (int d = 0; d < 4; d++) {
                int nx = xx + DX[d], ny = yy + DY[d];
                if (!passable(nx, ny) || ddist[ny][nx] >= 0) continue;
                ddist[ny][nx] = ddist[yy][xx] + 1; qx[qt] = nx; qy[qt] = ny; qt++;
            }
        }
        int best = -1; float bs = -1e9f;
        for (int yy = 1; yy < MAPN - 1; yy++)
            for (int xx = 1; xx < MAPN - 1; xx++) {
                if (dist[yy][xx] < maxd / 5 || ddist[yy][xx] < 0 || !free_floor(xx, yy) || in_trial(xx, yy)) continue;
                float sc = -fabsf(ddist[yy][xx] - 20.0f) + (G.map[yy][xx].theme != TH_CORRIDOR ? 4.0f : 0.0f) + rndf() * 6.0f
                        - (dist[yy][xx] > maxd * 3 / 5 ? 10.0f : 0.0f);
                if (sc > bs) { bs = sc; best = yy * MAPN + xx; }
            }
        if (best >= 0) { Item *it = add_item(best % MAPN, best / MAPN, IT_KEY); if (it) it->color = k; }
    }
    for (int i = 0; i < 300; i++) {                                /* one healing potion */
        int x = 1 + rnd(MAPN - 2), y = 1 + rnd(MAPN - 2);
        if (part_spot(x, y, dist, 6) && G.map[y][x].theme != TH_CORRIDOR) { part_lay(add_item(x, y, IT_POTION)); break; }   /* by a prop */
        if (dist[y][x] < 6 || !free_floor(x, y)) continue;
        add_item(x, y, IT_POTION);
        break;
    }
    /* and in each secret room, off the stone's path and clear of its doorway, within reach of a lane:
     * treasure, more for a bigger room, and often something to heal with. None of it counts towards the
     * level's treasure, so a secret is pure gain */
    static const int LOOT[6] = { IT_CHEST, IT_CROWN, IT_GEMS, IT_GEMS, IT_CROWN, IT_CHALICE };
    for (int k = 0; k < G.nsecrets; k++) {
        const Secret *sc = &G.secrets[k];
        static int spots[64][2];
        int ns = 0;
        for (int y = sc->y0; y <= sc->y1; y++)
            for (int x = sc->x0; x <= sc->x1; x++) {
                int along = (x - sc->sx) * DX[sc->d] + (y - sc->sy) * DY[sc->d], across = (x - sc->sx) * DY[sc->d] + (y - sc->sy) * DX[sc->d];
                if (across == 0 || lane_dist(x, y) > 1 || ns >= 64) continue;   /* the stone's way, and out of reach */
                if (sc->door_x >= 0 && abs(x - sc->door_x) + abs(y - sc->door_y) <= 1) continue;
                if (along < 2 && sc->len > 3) continue;            /* a big room keeps its front clear */
                spots[ns][0] = x; spots[ns][1] = y; ns++;
            }
        for (int i = ns - 1; i > 0; i--) { int j = rnd(i + 1); int t0 = spots[i][0], t1 = spots[i][1]; spots[i][0] = spots[j][0]; spots[i][1] = spots[j][1]; spots[j][0] = t0; spots[j][1] = t1; }
        int big = (sc->x1 - sc->x0 + 1) * (sc->y1 - sc->y0 + 1), n = 2 + (G.level >= 4) + (rnd(3) == 0) + (big > 9) + (big > 21);
        for (int i = 0; i < n && i < ns; i++) {
            int type = (i == n - 1 && rnd(10) < 6) ? IT_POTION : LOOT[rnd(6)];
            Item *it = add_item(spots[i][0], spots[i][1], type);
            if (!it) break;
            it->secret = 1;
            if (type < IT_KEY) { G.items_total--; G.value_total -= treasure_value(type); }
        }
    }
}

/* wall skulls, from level 3: on a wall the caster will walk straight at, at the end of a run of hall */
static void watch_dead(int sx, int sy, int cap);
static void nest_make(int cap);
static int pack_new(int kind);
static void pack_join(int pk, Mob *m);
static int mind_on(void);
static void place_wall_skulls(int sx, int sy) {
    if (G.level < KINDS[MOB_WSKULL].debut) return;
    int want = G.level == KINDS[MOB_WSKULL].debut ? 1 : 2 + (G.level >= 5) + rnd(2), placed = 0, px[6], py[6];
    want = imin(want, set_cap);
    for (int tries = 0; tries < 3000 && placed < want; tries++) {
        int x = 1 + rnd(MAPN - 2), y = 1 + rnd(MAPN - 2), d = rnd(4);
        if (G.map[y][x].kind != K_FLOOR || G.map[y][x].secret || G.bulky[y][x] || vault_of_room(x, y) >= 0 || abs(x - sx) + abs(y - sy) < 6 || in_band(y)) continue;
        Cell *w = cell(x + DX[d], y + DY[d]);
        if (w->kind != K_WALL || w->tex >= T_TORCH0 || w->secret) continue;
        int run = 1;
        for (int k = 1; k <= 3; k++) run &= cell(x - DX[d] * k, y - DY[d] * k)->kind == K_FLOOR;
        if (!run) continue;
        int far = 1;
        for (int i = 0; i < placed; i++) if (abs(px[i] - x) + abs(py[i] - y) < 6) far = 0;
        if (!far) continue;
        Mob *m = spawn_mob(x, y, MOB_WSKULL);
        if (!m) break;
        m->x = x + 0.5f + DX[d] * 0.42f; m->y = y + 0.5f + DY[d] * 0.42f; m->z = 0.3f;
        m->face = heading_angle_raw(d);
        px[placed] = x; py[placed] = y; placed++;
    }
    if (placed && mind_on() && since(3)) watch_dead(px[0], py[0], set_cap - placed);   /* the watch and the dead (step 8) */
}

/* spiders, from level 6: in some of the corner webs */
static void place_spiders(int sx, int sy) {
    if (G.level < KINDS[MOB_SPIDER].debut) return;
    int want = G.level == KINDS[MOB_SPIDER].debut ? 1 : 2 + (G.level >= 8) + rnd(2), placed = 0;
    want = imin(want, set_cap);
    for (int tries = 0; tries < 60 && placed < want && G.ndecos; tries++) {
        const Deco *dc = &G.decos[rnd(G.ndecos)];
        if (dc->spr != SP_COBWEB || hypotf(dc->x - sx, dc->y - sy) < 5.0f) continue;
        int clash = 0;
        for (int i = 0; i < MAXMOBS; i++) if (G.mobs[i].alive && G.mobs[i].type == MOB_SPIDER && hypotf(G.mobs[i].ax - dc->x, G.mobs[i].ay - dc->y) < 4.0f) clash = 1;
        if (clash) continue;
        Mob *m = spawn_mob((int)dc->x, (int)dc->y, MOB_SPIDER);
        if (!m) break;
        m->ax = m->x = dc->x; m->ay = m->y = dc->y; m->z = 0.62f;
        placed++;
    }
    if (placed && mind_on() && since(15) && may_spawn(MOB_HUNTER)) nest_make(set_cap - placed);   /* the nest (step 8) */
}

static int mind_on(void);
static int room_of_region(int rg);
static int pick_cell_at_depth(int sx, int sy, int dmin, int dmax, int need_hidden, int *ox, int *oy);
static void pack_call(Mob *m);
static int pack_new(int kind);
static void pack_join(int pk, Mob *m);
static Mob *riser_make(int type, int x, int y, int at) {
    Mob *m = spawn_mob(x, y, type);
    if (!m) return NULL;
    m->sub = 1; m->home = G.map[y][x].region;
    if (at >= 0) { m->deco = at; G.decos[at].spr = SP_PUDDLE_B; }
    else { m->deco = G.ndecos; add_deco(x + 0.5f, y + 0.5f, SP_MOUND); }
    return m;
}
static int riser_spot(int pass, int di, int d0, int *ox, int *oy) {   /* where one waits by this grave or in this puddle */
    int sp = G.decos[di].spr, x = (int)G.decos[di].x, y = (int)G.decos[di].y;
    if (pass) {                                                    /* in the puddle itself */
        if ((sp != SP_PUDDLE_A && sp != SP_PUDDLE_B && sp != SP_SIGN_DRAG) || G.map[y][x].kind != K_FLOOR) return 0;
    } else {                                                       /* on the floor before the stone */
        if (sp < SP_TOMB_ROUND || sp > SP_TOMB_BROKEN) return 0;
        int d = d0, k = 0;
        for (; k < 4; k++, d = (d + 1) & 3) if (G.map[y + DY[d]][x + DX[d]].kind == K_FLOOR && !G.map[y + DY[d]][x + DX[d]].secret) break;
        if (k == 4) return 0;
        x += DX[d]; y += DY[d];
    }
    if (occupied(x, y, NULL) || abs(x - G.cx) + abs(y - G.cy) < 8) return 0;
    *ox = x; *oy = y;
    return 1;
}
/* The watch and the dead (docs/PLAN-ai.md, section 7; from 3, step 8): a wall skull over a hall, and two or three of the
 * Order's dead holding the nearest room its scream reaches; meet its eye and they come */
static int16_t (*mind_field(int tx, int ty))[MAPN];
static int set_clear(int x, int y);
static int free_near(int x, int y, int *ox, int *oy);
static void watch_dead(int sx, int sy, int cap) {
    if (cap < 2) return;
    int16_t (*f)[MAPN] = mind_field(sx, sy);
    int bx = -1, by = -1, bh = 99;
    for (int y = 1; y < MAPN - 1; y++)
        for (int x = 1; x < MAPN - 1; x++) {
            const Cell *c = &G.map[y][x];
            int h = f[y][x];
            if (c->kind != K_FLOOR || c->secret || h < 3 || h > 9 || h >= bh || room_of_region(c->region) <= 0) continue;
            if (vault_of_room(x, y) >= 0 || in_wing(x, y) || in_trial(x, y) || in_band(y) || abs(x - G.cx) + abs(y - G.cy) < 10 || !set_clear(x, y) || occupied(x, y, NULL)) continue;
            bx = x; by = y; bh = h;
        }
    if (bx < 0) return;
    int pk = pack_new(PACK_FILE), rg = G.map[by][bx].region, n = imin(2 + (G.level >= 7 && rnd(2)), cap), made = 0;
    if (pk < 0) return;
    for (int i = 0; i < n; i++) {
        int mx = bx, my = by;
        if (i && (!free_near(bx, by, &mx, &my) || G.map[my][mx].region != rg)) break;
        Mob *m = spawn_mob(mx, my, MOB_SKEL);
        if (!m) break;
        m->home = rg;
        pack_join(pk, m); made++;
    }
#ifdef CRAWL_DEBUG
    fprintf(stderr, "PACKS level %d: the watch's dead, %d, %d steps from a wall skull at (%d,%d)\n", G.level, made, bh, sx, sy);
#endif
}
/* The nest (section 6; from 15): a web spider's corner (a gallery's or a stope's first), and two or three hunting spiders
 * overhead waiting on it; the web spider dropping is the tripwire that brings them */
static void nest_make(int cap) {
    if (cap < 2) return;
    int best = -1, bs = 0;
    for (int i = 0; i < MAXMOBS; i++) {
        const Mob *w = &G.mobs[i];
        if (!w->alive || w->type != MOB_SPIDER || w->guard || w->pack) continue;
        int r = room_of_region(G.map[(int)w->y][(int)w->x].region);   /* the webs hang in the halls' corners, and a gallery's */
        if (r == 0 || abs((int)w->x - G.cx) + abs((int)w->y - G.cy) < 10 || !set_clear((int)w->x, (int)w->y)) continue;
        int rt = r > 0 ? G.rooms[r].rtype : RT_HALL, sc = rt == RT_SPINNERS || rt == RT_STOPE || rt == RT_GALLERY ? 2 : 1;
        if (sc > bs) { bs = sc; best = i; }
    }
    if (best < 0) return;
    int pk = pack_new(PACK_SPIDERS);
    if (pk < 0) return;
    Mob *w = &G.mobs[best];
    int wx = (int)w->x, wy = (int)w->y, rg = G.map[wy][wx].region, n = imin(2 + (crawl_set(G.level) >= 3), cap), made = 0;
    pack_join(pk, w);
    for (int i = 0; i < MAXMOBS; i++)                              /* the other webs of the room are its tripwires too */
        if (i != best && G.mobs[i].alive && G.mobs[i].type == MOB_SPIDER && !G.mobs[i].guard && G.map[(int)G.mobs[i].y][(int)G.mobs[i].x].region == rg) pack_join(pk, &G.mobs[i]);
    for (int i = 0; i < n; i++) {
        int mx, my;
        if (!free_near(wx, wy, &mx, &my) || G.map[my][mx].region != rg) break;
        Mob *m = spawn_mob(mx, my, MOB_HUNTER);
        if (!m) break;
        m->sub = 0; m->home = rg;                                  /* overhead, upside down */
        pack_join(pk, m); made++;
    }
#ifdef CRAWL_DEBUG
    fprintf(stderr, "PACKS level %d: a nest of %d hunting spiders over the webs at (%d,%d)\n", G.level, made, wx, wy);
#endif
}
/* The wake (docs/PLAN-ai.md, section 6; from 13): three or four of them under one yard or one flooded room, as a pack:
 * when one rises the rest stir after it, the nearest to you first */
static int place_wake(int pass, int type) {
    if (!mind_on() || G.level < 13 || G.level == KINDS[type].debut || (G.level != 13 && rnd(3) == 0)) return 0;
    int cx[64], cy[64], cdi[64], n = 0;
    for (int di = 0; di < G.ndecos && n < 64; di++) {
        int x, y, dup = 0;
        if (!riser_spot(pass, di, 0, &x, &y) || in_band(y)) continue;
        for (int i = 0; i < n; i++) dup |= cx[i] == x && cy[i] == y;
        if (dup) continue;
        cx[n] = x; cy[n] = y; cdi[n] = di; n++;
    }
    int best = -1, bc = 2;
    for (int i = 0; i < n; i++) {                                  /* the room with the most of them */
        int rg = G.map[cy[i]][cx[i]].region, c = 0;
        for (int j = 0; j < n; j++) c += G.map[cy[j]][cx[j]].region == rg;
        if (c > bc || (c == bc && best >= 0 && rnd(2))) { bc = c; best = rg; }
    }
    if (best < 0) return 0;
    int pk = pack_new(PACK_WAKE), made = 0, want = imin(3 + rnd(2), set_cap);
    if (pk < 0) return 0;
    for (int i = 0; i < n && made < want; i++) {
        if (G.map[cy[i]][cx[i]].region != best || occupied(cx[i], cy[i], NULL)) continue;
        Mob *m = riser_make(type, cx[i], cy[i], pass ? cdi[i] : -1);
        if (!m) break;
        pack_join(pk, m); made++;
    }
#ifdef CRAWL_DEBUG
    fprintf(stderr, "PACKS level %d: wake of %d %s in room %d\n", G.level, made, KINDS[type].name, best);
#endif
    return made;
}

/* The buried, from level 2: a grave whose earth has been turned over from below, with something under
 * it that climbs out when you come near. The Drowned, from 7: a puddle that stirs. */
static void place_risers(void) {
    int set = crawl_set(G.level);
    for (int pass = 0; pass < 2; pass++) {
        int type = pass ? MOB_DROWNED : MOB_SHAMBLER;
        if ((pass ? set != 1 : set > 1) || !may_spawn(type)) continue;
        int want = G.level == KINDS[type].debut ? 1 : 2 + rnd(2), placed = place_wake(pass, type);
        want = imin(want, set_cap);
        for (int tries = 0; tries < 300 && placed < want && G.ndecos; tries++) {
            int di = rnd(G.ndecos), x, y;
            if (!riser_spot(pass, di, rnd(4), &x, &y)) continue;
            if (!riser_make(type, x, y, pass ? di : -1)) return;
            placed++;
        }
    }
}

/* Ember imps, from 17: born of the magma, they keep near it till they know you are there */
static void place_imps(void) {
    if (!may_spawn(MOB_IMP)) return;
    int want = G.level == KINDS[MOB_IMP].debut ? 1 : 2 + rnd(2), placed = 0, brood = mind_on() && since(23), pk = -1, vent = -1;
    want = imin(want, set_cap);
    for (int tries = 0; tries < 200 && placed < want && G.ndecos; tries++) {
        int di = vent >= 0 ? vent : rnd(G.ndecos);                 /* a brood of the magma (step 8, from 23): all of it by one vent */
        const Deco *dc = &G.decos[di];
        int d = rnd(4), x = (int)dc->x + DX[d], y = (int)dc->y + DY[d];
        if (dc->spr != SP_MAGMA_A || G.map[y][x].kind != K_FLOOR || G.map[y][x].secret || occupied(x, y, NULL) || abs(x - G.cx) + abs(y - G.cy) < 8) continue;
        Mob *m = spawn_mob(x, y, MOB_IMP);
        if (!m) return;
        m->home = G.map[y][x].region;
        placed++;
        if (brood) { if (pk < 0) { pk = pack_new(PACK_IMPS); vent = di; } pack_join(pk, m); }
    }
#ifdef CRAWL_DEBUG
    if (pk >= 0) fprintf(stderr, "PACKS level %d: a brood of %d imps by a vent\n", G.level, G.packs[pk].n0);
#endif
}

/* Gargoyles, from 19: some of the statues are not statues. If the level has too few, one stands in a
 * room's corner, on a plinth like theirs. */
static void place_gargoyles(void) {
    if (!may_spawn(MOB_GARGOYLE)) return;
    int want = G.level == KINDS[MOB_GARGOYLE].debut ? 1 : 2 + rnd(2), placed = 0;
    want = imin(want, set_cap);
    for (int tries = 0; tries < 400 && placed < want; tries++) {
        int x, y, di = -1;
        if (tries < 200) {                                         /* a statue already standing */
            if (!G.ndecos) continue;
            di = rnd(G.ndecos);
            if (G.decos[di].spr != SP_GARG_PERCH) continue;
            x = (int)G.decos[di].x; y = (int)G.decos[di].y;
        } else {                                                   /* or a corner of a room */
            const Room *R = &G.rooms[rnd(G.nrooms)];
            x = R->x + rnd(R->w); y = R->y + rnd(R->h);
            if (G.map[y][x].kind != K_FLOOR || G.map[y][x].secret || !block_corner(x, y) || lane(x, y) || by_way(x, y) || occupied(x, y, NULL) || in_band(y)) continue;
        }
        if (abs(x - G.cx) + abs(y - G.cy) < 10) continue;
        int taken = 0;
        for (int i = 0; i < MAXMOBS; i++) taken |= G.mobs[i].alive && G.mobs[i].cx == x && G.mobs[i].cy == y;
        if (taken) continue;
        Mob *m = spawn_mob(x, y, MOB_GARGOYLE);
        if (!m) return;
        if (di >= 0) G.decos[di].spr = -1;                         /* it is drawn as itself now */
        G.map[y][x].kind = K_PROP; G.foot[y][x] = 0;              /* its plinth whole while it stands there (1.0.0b18.1) */
        m->home = G.map[y][x].region;
        placed++;
    }
    if (placed >= 2 && mind_on() && since(27)) {                   /* sentinels (step 8, from 27): one wakes, and the others with it */
        int pk = pack_new(PACK_SENTINELS);
        for (int i = 0; i < MAXMOBS && pk >= 0; i++) if (G.mobs[i].alive && G.mobs[i].type == MOB_GARGOYLE && !G.mobs[i].pack && !G.mobs[i].amb) pack_join(pk, &G.mobs[i]);
#ifdef CRAWL_DEBUG
        if (pk >= 0) fprintf(stderr, "PACKS level %d: %d sentinels\n", G.level, G.packs[pk].n0);
#endif
    }
}

/* the silver and crimson rooms are guarded */
static void place_guardians(void) {
    for (int k = 1; k < G.nvaults; k++) {
        int x, y;
        if (!room_cell(G.vault_room[k], &x, &y)) continue;
        if (may_spawn(MOB_SPIDER) && G.level > KINDS[MOB_SPIDER].debut && rnd(5) < 2) {   /* a spider in a corner of the room */
            const Room *R = &G.rooms[G.vault_room[k]];
            int cx = rnd(2) ? R->x : R->x + R->w - 1, cy = rnd(2) ? R->y : R->y + R->h - 1;
            if (G.map[cy][cx].kind != K_FLOOR) continue;
            float wx = cx + (cx == R->x ? 0.16f : 0.84f), wy = cy + (cy == R->y ? 0.16f : 0.84f);
            add_deco(wx, wy, SP_COBWEB);
            Mob *m = spawn_mob(cx, cy, MOB_SPIDER);
            if (m) { m->ax = m->x = wx; m->ay = m->y = wy; m->z = 0.62f; m->guard = 1; }
        } else {
            Mob *m = spawn_mob(x, y, may_spawn(MOB_SHIELD) ? MOB_SHIELD : MOB_SKEL);
            if (m) m->guard = 1;
        }
    }
}

