/* A part of scene_crawl.c (docs/PLAN-tidy.md): it is #included there in its place and never compiled on its own.
 * Drawn levels (the tutorial's two): the maps, gen_drawn, their items, dead and marks; the burning floor; the ice. */

/* ---- Drawn levels (docs/PLAN-tutorial.md): levels 1 and 2 of the first descent are the owner's drawings, the same every
 * time, for everyone, the dream too. Each is a map of letters, its rooms by type, and what stands in it, in the drawing's own
 * cells (x east from its corner, y south, as media/sheets/tutorial/level*-read.png has them), laid at (ox, oy) in the level's map.
 * The look, the dressing and the torches are the generator's, laid on a seed of the drawing's own, so they lie the same each
 * time too. The letters: # stone, . floor, + floor on a lane, F the burning floor, f on a lane, D a door, G a gold door,
 * C a crimson door, = the one-way gate, S the start (on a lane), h the place to mend and X the exit (walls, used from their open
 * side, north first), I the ice, i on a lane, c a secret's wall, q its room, Q on a lane. Doors are on lanes. ---- */
typedef struct { int x, y, w, h, rtype; } DrawnRoom;
enum { DT_GOLD, DT_KEY, DT_DEAD, DT_SGOLD, DT_MARK, DT_SWITCH, DT_GSEAL, DT_MAP };   /* SGOLD: in the secret; MARK's kind 0 a coffer,
                                                                      1 a cage (any line), 2 the class's own */
typedef struct { int what, kind; float x, y; } DrawnThing;          /* kind: an IT_* for gold, a key's colour, a MOB_* */
typedef struct {
    const char *const *map; int w, h, ox, oy, face;                 /* face: the heading at the start, as G.h */
    const DrawnRoom *room; int nroom, vault[NKEYS];                /* vault: the room each colour's doors keep, -1 none */
    const DrawnThing *thing; int nthing;
    uint32_t seed;                                                 /* the dice its look is laid with */
} Drawn;
static const char *const DRAWN1_MAP[32] = {
    "##S++#################",    /*  0 */
    "####D#################",    /*  1 */
    "##Fff#################",    /*  2 */
    "##FfF#################",    /*  3 */
    "##FfF#################",    /*  4 */
    "##FfF#################",    /*  5 */
    "##.+.#################",    /*  6 */
    "##.+.#################",    /*  7 */
    "##.+.#################",    /*  8 */
    "##.+.............#####",    /*  9 */
    "##.++++++++++++++#####",    /* 10 */
    "##.+...........+.#####",    /* 11 */
    "###h##########.+.#.+..",    /* 12 */
    "##############.+.#++++",    /* 13 */
    "##############.+.#.+..",    /* 14 */
    "##############.++D++++",    /* 15 */
    "##########.+.#.+.#####",    /* 16 */
    "##########+++D++.#####",    /* 17 */
    "##########.+.#.+.#####",    /* 18 */
    "##############.+.#####",    /* 19 */
    "####...........+.#####",    /* 20 */
    "####+++++++++++++#####",    /* 21 */
    "####+............#####",    /* 22 */
    ".++D++################",    /* 23 */
    ".+.#.+####.+.#########",    /* 24 */
    ".+##.+####.+.#########",    /* 25 */
    ".+G+#+#####D##########",    /* 26 */
    ".+#+#+##...+...#######",    /* 27 */
    ".+#+#+##+++++++#######",    /* 28 */
    ".+#+#+##...+...#####X#",    /* 29 */
    ".+#+#+.....+...#....+.",    /* 30 */
    ".+#+G++++++++++D++++++",    /* 31 */
};
static const DrawnThing DRAWN1_THING[] = {
    { DT_GOLD, IT_COINS, 4.01f, 7.02f }, { DT_GOLD, IT_COINS, 6.01f, 10.02f }, { DT_GOLD, IT_COINS, 12.03f, 10.02f },
    { DT_GOLD, IT_CHALICE, 16.04f, 10.02f }, { DT_GOLD, IT_COINS, 3.01f, 11.03f }, { DT_GOLD, IT_COINS, 9.09f, 11.09f },
    { DT_GOLD, IT_GEMS, 20.98f, 14.03f }, { DT_GOLD, IT_COINS, 14.97f, 14.10f }, { DT_GOLD, IT_CHALICE, 20.11f, 15.03f },
    { DT_GOLD, IT_COINS, 20.98f, 15.03f }, { DT_GOLD, IT_GEMS, 11.02f, 17.10f }, { DT_GOLD, IT_CHALICE, 12.67f, 18.04f },
    { DT_GOLD, IT_COINS, 16.03f, 18.04f }, { DT_GOLD, IT_CHALICE, 16.04f, 19.04f }, { DT_GOLD, IT_COINS, 8.08f, 21.04f },
    { DT_GOLD, IT_CHALICE, 13.03f, 22.05f }, { DT_GOLD, IT_CHALICE, 0.94f, 24.05f }, { DT_GOLD, IT_COINS, 4.95f, 24.05f },
    { DT_GOLD, IT_GEMS, 10.96f, 25.05f }, { DT_GOLD, IT_CHALICE, 12.03f, 25.12f }, { DT_GOLD, IT_COINS, 5.31f, 25.90f },
    { DT_GOLD, IT_CHEST, 3.44f, 26.54f }, { DT_GOLD, IT_CROWN, 3.35f, 26.96f }, { DT_GOLD, IT_COINS, 0.94f, 27.06f },
    { DT_GOLD, IT_GEMS, 3.63f, 27.37f }, { DT_GOLD, IT_CHALICE, 9.08f, 28.06f }, { DT_GOLD, IT_COINS, 13.10f, 28.06f },
    { DT_GOLD, IT_GEMS, 3.43f, 28.41f }, { DT_GOLD, IT_GEMS, 1.00f, 29.06f }, { DT_GOLD, IT_CHALICE, 3.67f, 29.22f },
    { DT_GOLD, IT_COINS, 3.33f, 29.82f }, { DT_GOLD, IT_COINS, 1.01f, 30.06f }, { DT_GOLD, IT_COINS, 12.03f, 30.06f },
    { DT_GOLD, IT_COINS, 19.98f, 31.00f }, { DT_GOLD, IT_COINS, 7.02f, 31.06f }, { DT_GOLD, IT_CHALICE, 14.06f, 31.06f },
    { DT_GOLD, IT_CHALICE, 18.95f, 31.09f }, { DT_GOLD, IT_GEMS, 21.11f, 31.13f }, { DT_KEY, 0, 11.20f, 24.57f },
    { DT_DEAD, MOB_SKEL, 15.10f, 12.96f }, { DT_DEAD, MOB_SKEL, 20.98f, 13.10f }, { DT_DEAD, MOB_SKEL, 19.04f, 14.97f },
    { DT_DEAD, MOB_SKEL, 16.04f, 16.04f }, { DT_DEAD, MOB_SKEL, 11.03f, 18.04f }, { DT_DEAD, MOB_SKEL, 14.97f, 20.04f },
    { DT_DEAD, MOB_SKEL, 5.08f, 21.98f }, { DT_DEAD, MOB_SKEL, 1.00f, 28.12f }, { DT_DEAD, MOB_SKEL, 5.95f, 31.06f },
    { DT_DEAD, MOB_SKEL, 9.09f, 31.07f }, { DT_DEAD, MOB_SKEL, 1.00f, 31.13f },
};
static const DrawnRoom DRAWN1_ROOM[] = {
    { 18, 12, 4, 4, RT_BURIAL }, { 10, 16, 3, 3, RT_OSSUARY }, { 10, 24, 3, 2, RT_OSSUARY },   /* the east room, the side room, the key room */
    { 0, 23, 3, 9, RT_UNDER }, { 8, 27, 7, 3, RT_CHAPTER }, { 3, 26, 1, 6, RT_KNIGHTS },     /* the west room, the big room, the vault */
};
static const char *const DRAWN2_MAP[35] = {
    "##S++.#########",    /*  0 */
    "####+.#########",    /*  1 */
    "###.+.#########",    /*  2 */
    "##..+..########",    /*  3 */
    "#+++++++#######",    /*  4 */
    "#.+.#.+.......#",    /*  5 */
    "#.+###++++++++#",    /*  6 */
    "#.+.#.+......+#",    /*  7 */
    "#.++#.+.####.+#",    /*  8 */
    "##.+#++#####.+#",    /*  9 */
    "###+#+##X####=#",    /* 10 */
    "++G+#+#.+.#.++.",    /* 11 */
    ".+###+#.+.#.++.",    /* 12 */
    ".+#+++#.+.#.++.",    /* 13 */
    ".+#+#+#.+.#.++.",    /* 14 */
    ".+#+#+#.+.#.++.",    /* 15 */
    ".+#+++#.+.#IiiI",    /* 16 */
    ".+#++##.+.#IiiI",    /* 17 */
    ".+#+h##.+.#IiiI",    /* 18 */
    ".+#+#+C+++#IiiI",    /* 19 */
    ".+#+#+######ii#",    /* 20 */
    "##.+#+.#####ii#",    /* 21 */
    "#..+#+..####ii#",    /* 22 */
    "#+++#+++++++++#",    /* 23 */
    "#.+###...+....#",    /* 24 */
    "#.+###...+....#",    /* 25 */
    "#+++#+++++++++#",    /* 26 */
    "#+++++++#####D#",    /* 27 */
    "##..+..##....+#",    /* 28 */
    "###.+.##++++++#",    /* 29 */
    "###.+.#+++++++#",    /* 30 */
    "###.+.#+.....+#",    /* 31 */
    "#######c#######",    /* 32 */
    "#######QQQ#####",    /* 33 */
    "#######qqq#####",    /* 34 */
};
static const DrawnThing DRAWN2_THING[] = {
    { DT_GOLD, IT_COINS, 5.30f, 2.50f }, { DT_GOLD, IT_CHALICE, 2.50f, 3.50f },                                     /* the start */
    { DT_GOLD, IT_COINS, 1.40f, 7.00f }, { DT_GOLD, IT_GEMS, 1.30f, 7.90f },                                        /* the west passage */
    { DT_GOLD, IT_COINS, 11.10f, 6.40f },                                                                           /* the east hall */
    { DT_GOLD, IT_CHALICE, 7.40f, 8.70f }, { DT_GOLD, IT_COINS, 5.60f, 12.50f }, { DT_GOLD, IT_COINS, 3.30f, 13.60f },
    { DT_GOLD, IT_GEMS, 5.70f, 15.50f },                                                                            /* the middle way */
    { DT_GOLD, IT_CHALICE, 11.30f, 13.90f }, { DT_GOLD, IT_GEMS, 11.30f, 17.40f },                                  /* the map room, the ice */
    { DT_GOLD, IT_COINS, 7.60f, 11.30f }, { DT_GOLD, IT_CHALICE, 9.30f, 11.40f }, { DT_GOLD, IT_CROWN, 9.60f, 18.60f },   /* the exit room */
    { DT_GOLD, IT_COINS, 1.60f, 14.90f }, { DT_GOLD, IT_CHEST, 0.40f, 20.60f }, { DT_GOLD, IT_GEMS, 1.60f, 20.60f },     /* the west room */
    { DT_GOLD, IT_COINS, 9.60f, 24.10f }, { DT_GOLD, IT_CHALICE, 12.20f, 25.40f },                                  /* the great hall */
    { DT_GOLD, IT_COINS, 1.30f, 27.50f }, { DT_GOLD, IT_COINS, 7.70f, 27.70f }, { DT_GOLD, IT_CHALICE, 2.80f, 28.80f },
    { DT_GOLD, IT_COINS, 4.00f, 30.50f }, { DT_GOLD, IT_GEMS, 4.90f, 31.70f },                                      /* the south-west lobe */
    { DT_GOLD, IT_COINS, 9.40f, 28.50f }, { DT_GOLD, IT_CHALICE, 8.60f, 29.30f },                                   /* the south-east room */
    { DT_SGOLD, IT_CHEST, 9.40f, 33.50f }, { DT_SGOLD, IT_GEMS, 8.50f, 33.60f }, { DT_SGOLD, IT_CROWN, 8.50f, 34.50f },
    { DT_SGOLD, IT_GEMS, 9.50f, 34.50f },                                                                           /* the secret */
    { DT_KEY, 0, 7.60f, 30.80f }, { DT_KEY, 2, 1.00f, 19.60f }, { DT_MAP, 0, 12.80f, 15.40f },
    { DT_DEAD, MOB_SKEL, 2.30f, 5.00f }, { DT_DEAD, MOB_BAT, 13.00f, 5.70f }, { DT_DEAD, MOB_SKEL, 3.40f, 9.40f },
    { DT_DEAD, MOB_SKEL, 11.70f, 12.60f }, { DT_DEAD, MOB_SHAMBLER, 7.90f, 13.10f }, { DT_DEAD, MOB_SKEL, 3.50f, 15.00f },
    { DT_DEAD, MOB_SKEL, 9.50f, 16.40f }, { DT_DEAD, MOB_BAT, 14.30f, 17.60f }, { DT_DEAD, MOB_SHAMBLER, 1.40f, 18.20f },
    { DT_DEAD, MOB_SKEL, 7.10f, 22.60f }, { DT_DEAD, MOB_SHAMBLER, 3.10f, 22.90f }, { DT_DEAD, MOB_SKEL, 11.60f, 26.30f },
    { DT_DEAD, MOB_BAT, 12.90f, 28.80f }, { DT_DEAD, MOB_SKEL, 9.30f, 30.80f }, { DT_DEAD, MOB_SHAMBLER, 3.60f, 31.30f },
    /* the things that want a spell (T22 a): a, b, g, j any line on the main way; c, d, e, f, h, i the class's own. a is moved a
     * row north and f, g, h set to their walls, off the lanes they were drawn on */
    { DT_MARK, 0, 5.50f, 3.50f }, { DT_MARK, 1, 11.80f, 7.60f }, { DT_MARK, 2, 2.40f, 9.50f }, { DT_MARK, 2, 7.30f, 14.80f },
    { DT_MARK, 2, 0.30f, 16.20f }, { DT_MARK, 2, 3.20f, 16.50f }, { DT_MARK, 0, 11.70f, 23.20f }, { DT_MARK, 2, 3.70f, 26.20f },
    { DT_MARK, 2, 5.50f, 31.40f }, { DT_MARK, 1, 11.10f, 31.60f },
    { DT_SWITCH, 0, 14.30f, 11.60f }, { DT_GSEAL, 0, 13.50f, 11.06f },
};
static const DrawnRoom DRAWN2_ROOM[] = {
    { 0, 11, 2, 10, RT_OSSUARY }, { 11, 11, 4, 5, RT_KNIGHTS }, { 7, 11, 3, 9, RT_CHAPTER },   /* the west room, the map room, the exit room */
    { 7, 28, 7, 4, RT_BURIAL }, { 5, 23, 9, 4, RT_UNDER }, { 1, 27, 5, 5, RT_UNDER },          /* the south-east room, the great hall, the lobe */
};
#define NDRAWN 2
static const Drawn DRAWN[NDRAWN] = {
    { DRAWN1_MAP, 22, 32, 11, 6, 0, DRAWN1_ROOM, (int)(sizeof DRAWN1_ROOM / sizeof DRAWN1_ROOM[0]), { 5, -1, -1 },
      DRAWN1_THING, (int)(sizeof DRAWN1_THING / sizeof DRAWN1_THING[0]), 0x7D1A0001u },
    { DRAWN2_MAP, 15, 35, 15, 5, 0, DRAWN2_ROOM, (int)(sizeof DRAWN2_ROOM / sizeof DRAWN2_ROOM[0]), { 0, -1, 2 },
      DRAWN2_THING, (int)(sizeof DRAWN2_THING / sizeof DRAWN2_THING[0]), 0x7D1A0002u },
};
#define DRAWN_HALL 40                                              /* the halls' region on a drawn level */
static int drawn_level(int level) {                                /* which drawing a level is, 1-based; 0 none */
#ifdef CRAWL_DEBUG
    static int off = -1;
    if (off < 0) { const char *e = getenv("CRAWL_DRAWN"); off = e && atoi(e) == 0; }   /* 0: the generated levels, as before */
    if (off) return 0;
#endif
    return level >= 1 && level <= NDRAWN && !PK.depth ? level : 0;
}
static const Drawn *drawn(void) { return &DRAWN[g_drawn - 1]; }
static char drawn_ch(int x, int y) {                               /* the letter at a cell of the level's map */
    const Drawn *D = drawn();
    x -= D->ox; y -= D->oy;
    return x < 0 || y < 0 || x >= D->w || y >= D->h ? '#' : D->map[y][x];
}
static int drawn_front(int wx, int wy, int *fx, int *fy) {         /* a wall's open side (north, south, west, east first): the cell
                                                                      it is used from, and the way */
    static const int ORDER[4] = { 3, 1, 2, 0 };
    for (int k = 0; k < 4; k++) {
        int d = ORDER[k];
        if (is_open(wx + DX[d], wy + DY[d]) && !G.map[wy + DY[d]][wx + DX[d]].secret) { *fx = wx + DX[d]; *fy = wy + DY[d]; return (d + 2) & 3; }
    }
    *fx = *fy = -1;
    return 0;
}
static void drawn_clear(int x, int y) {                            /* the dressing taken off a cell the drawing wants */
    if (x < 1 || y < 1 || x >= MAPN - 1 || y >= MAPN - 1) return;
    if (G.map[y][x].kind == K_PROP) G.map[y][x].kind = K_FLOOR;
    G.foot[y][x] = 0; G.bulky[y][x] = 0;
    for (int d = 0; d < G.ndecos; d++) if ((int)G.decos[d].x == x && (int)G.decos[d].y == y) G.decos[d].spr = -1;
}
static void gen_drawn(void) {
    const Drawn *D = drawn();
    G.faced = crawl_drawn(G.level); G.look = crawl_set(G.level);
    memset(nr_type, 0, sizeof nr_type);
    memset(G.map, 0, sizeof G.map);
    for (int y = 0; y < MAPN; y++)
        for (int x = 0; x < MAPN; x++) { G.map[y][x].kind = K_WALL; G.map[y][x].region = -1; G.map[y][x].door = -1; }
    memset(MM, 0, sizeof MM);                                      /* no plan: the dressing's crossings find none */
    for (int y = 0; y < MG; y++) for (int x = 0; x < MG; x++) { MM[y][x].kind = K_WALL; MM[y][x].region = -1; MM[y][x].room = -1; }
    G.nrooms = 0; G.ndoors = 0; G.nlights = 0; G.nlc = 0; G.ndecos = 0;
    memset(G.bulky, 0, sizeof G.bulky); memset(G.mapped, 0, sizeof G.mapped); memset(G.foot, 0, sizeof G.foot);
    G.nvaults = 0;
    for (int k = 0; k < NKEYS; k++) G.vault_room[k] = G.vault_door[k] = -1;
    G.pz = -1; G.pz_room = G.pz_door = -1; G.pz_exit = 0;           /* no puzzle, trial, lair or secret */
    G.tri = -1; G.tri_room = G.tri_door = -1; G.lairs = 0;
    G.nsecrets = G.secrets_found = G.nlinks = G.links_found = G.trials_failed = 0; G.slide = -1;
    memset(g_lane, 0, sizeof g_lane);
    memset(&DF, 0, sizeof DF);
    memset(&IC, 0, sizeof IC); IC.sw = IC.gate = IC.gate_door = -1; IC.live = 1; IC.t = 99.0f;   /* the ice, laid */
    for (int r = 0; r < D->nroom && r < 12; r++) {
        const DrawnRoom *dr = &D->room[r];
        G.rooms[r] = (Room){ D->ox + dr->x, D->oy + dr->y, dr->w, dr->h, r, RTYPES[dr->rtype].theme, dr->rtype };
        mroom[r][0] = mroom[r][1] = 0; mroom[r][2] = mroom[r][3] = 1;   /* a block each, so nothing laid by the plan's blocks */
        nr_type[dr->rtype]++; G.nrooms++;
    }
    for (int y = 0; y < D->h; y++)
        for (int x = 0; x < D->w; x++) {
            char ch = D->map[y][x];
            int mx = D->ox + x, my = D->oy + y, rg = DRAWN_HALL, th = TH_CORRIDOR, door = ch == 'D' || ch == 'G' || ch == 'C' || ch == '=';
            if (ch == 'c') { G.map[my][mx].secret = 2; g_lane[my][mx] = 1; }   /* a secret's wall: a lane once it has slid */
            if (ch == '#' || ch == 'h' || ch == 'X' || ch == 'c') continue;
            if (ch == 'q' || ch == 'Q') {                          /* the secret's room, carved as a secret's is */
                carve(mx, my, -2, TH_CORRIDOR); G.map[my][mx].secret = 1;
                if (ch == 'Q') g_lane[my][mx] = 1;
                continue;
            }
            if (!door)
                for (int r = 0; r < G.nrooms; r++) if (in_room(&G.rooms[r], mx, my)) { rg = r; th = G.rooms[r].theme; }
            carve(mx, my, rg, th);
            if (door) G.map[my][mx].kind = K_DOOR;
            if (ch == '+' || ch == 'f' || ch == 'i' || door || ch == 'S') g_lane[my][mx] = 1;
            if ((ch == 'I' || ch == 'i') && IC.n < ICE_CELLS) { IC.cx[IC.n] = mx; IC.cy[IC.n] = my; IC.n++; IC.at[my][mx] = (uint8_t)IC.n; }
            if ((ch == 'F' || ch == 'f') && DF.n < DFIRE_CELLS) {
                DF.cx[DF.n] = mx; DF.cy[DF.n] = my; DF.n++; DF.at[my][mx] = 1;
                DF.fade = 1.0f; DF.past_y = (float)my + 3.0f;      /* the last row's far edge and two cells more (south: the drawing's way on) */
            }
        }
    for (int y = 0; y < MAPN; y++)                                 /* the doors; a gold door is the vault's */
        for (int x = 0; x < MAPN; x++) {
            if (G.map[y][x].kind != K_DOOR || G.ndoors >= MAXDOORS) continue;
            char ch = drawn_ch(x, y);
            int lk = ch == 'G' || ch == 'C', col = ch == 'C' ? 2 : 0, ax = is_open(x - 1, y) && is_open(x + 1, y);
            G.map[y][x].door = (int16_t)G.ndoors;
            G.doors[G.ndoors++] = (Door){ x, y, ax, 0.0f, 0.0f, 0, ch == '=' ? GATED : lk, col, 0 };
            if (lk) G.vault_door[col] = G.ndoors - 1;
            if (ch == '=') IC.gate_door = G.ndoors - 1;            /* the one-way gate: barred till its seal is struck from the south */
        }
    for (int k = 0; k < NKEYS; k++) if (D->vault[k] >= 0) { G.vault_room[k] = D->vault[k]; G.nvaults = k + 1; }
    for (int y = 0; y < D->h; y++)                                 /* the secret, laid by hand (7.5) */
        for (int x = 0; x < D->w && G.nsecrets < MAXSECRETS; x++) {
            if (D->map[y][x] != 'c') continue;
            int sx = D->ox + x, sy = D->oy + y, in = -1, len = 0, x0 = MAPN, y0 = MAPN, x1 = -1, y1 = -1;
            for (int d = 0; d < 4 && in < 0; d++) if (is_open(sx - DX[d], sy - DY[d]) && !G.map[sy - DY[d]][sx - DX[d]].secret) in = d;
            if (in < 0) continue;
            while (G.map[sy + DY[in] * (len + 1)][sx + DX[in] * (len + 1)].secret == 1) len++;
            for (int yy = 0; yy < D->h; yy++)
                for (int xx = 0; xx < D->w; xx++)
                    if (D->map[yy][xx] == 'q' || D->map[yy][xx] == 'Q') { x0 = imin(x0, D->ox + xx); y0 = imin(y0, D->oy + yy); x1 = imax(x1, D->ox + xx); y1 = imax(y1, D->oy + yy); }
            G.secrets[G.nsecrets++] = (Secret){ sx - DX[in], sy - DY[in], in, sx, sy, 0, 0, 0, len, 1, -1, -1, x0, y0, x1, y1, D->seed | 1u, 0 };
            G.nlinks++;
        }
    gen_dress();
    for (int i = 0; i < D->nthing; i++)                            /* what the drawing sets down, and the ice, clear of the dressing */
        if (D->thing[i].what >= DT_SGOLD) drawn_clear(D->ox + (int)D->thing[i].x, D->oy + (int)D->thing[i].y);
    for (int i = 0; i < IC.n; i++) drawn_clear(IC.cx[i], IC.cy[i]);
    for (int y = 0; y < MAPN; y++)                                 /* and no prop on a lane: the dressing doesn't know the drawn ones */
        for (int x = 0; x < MAPN; x++) if (g_lane[y][x] && G.map[y][x].kind == K_PROP) drawn_clear(x, y);
    for (int i = 0; i < IC.n; i++) {                               /* each cell's wait as the ice turns: by the walls first */
        int open = 0;
        for (int d = 0; d < 4; d++) open += IC.at[IC.cy[i] + DY[d]][IC.cx[i] + DX[d]] != 0;
        IC.wait[i] = 0.3f * (float)open + 0.15f * (float)(hash2(IC.cx[i], IC.cy[i]) & 3);
    }
    static int qx[MAPN * MAPN], qy[MAPN * MAPN];                   /* each cell's steps to a lane, for what lies within reach of one */
    int qh = 0, qt = 0;
    for (int y = 0; y < MAPN; y++)
        for (int x = 0; x < MAPN; x++) {
            g_ldist[y][x] = g_lane[y][x] ? 0 : 9;
            if (g_lane[y][x]) { qx[qt] = x; qy[qt] = y; qt++; }
        }
    while (qh < qt) {
        int x = qx[qh], y = qy[qh]; qh++;
        for (int d = 0; d < 4; d++) {
            int nx = x + DX[d], ny = y + DY[d];
            if (!is_open(nx, ny) || g_ldist[ny][nx] <= g_ldist[y][x] + 1) continue;
            g_ldist[ny][nx] = (uint8_t)(g_ldist[y][x] + 1); qx[qt] = nx; qy[qt] = ny; qt++;
        }
    }
}
static void drawn_start(void) {
    const Drawn *D = drawn();
    for (int y = 0; y < D->h; y++) for (int x = 0; x < D->w; x++) if (D->map[y][x] == 'S') { G.cx = D->ox + x; G.cy = D->oy + y; }
}
static void drawn_items(void) {                                    /* the gold and the keys where drawn: no potion */
    const Drawn *D = drawn();
    G.nitems = 0; G.items_total = 0; G.items_got = 0; G.value_total = G.value_got = 0;
    for (int i = 0; i < D->nthing; i++) {
        const DrawnThing *t = &D->thing[i];
        if (t->what != DT_GOLD && t->what != DT_KEY && t->what != DT_SGOLD && t->what != DT_MAP) continue;
        float x = D->ox + t->x, y = D->oy + t->y;
        int type = t->what == DT_KEY ? IT_KEY : t->what == DT_MAP ? (RUN.has_map ? IT_CHALICE : IT_MAP) : t->kind;   /* the map held: gold (T18 a) */
        Item *it = add_item((int)x, (int)y, type);
        if (!it) continue;
        it->x = x; it->y = y;
        if (t->what == DT_KEY) it->color = t->kind;
        if (t->what == DT_SGOLD) { it->secret = 1; G.items_total--; G.value_total -= treasure_value(type); }   /* the secret's, out of the count */
        if (type == IT_MAP) { int nl = G.nlights; light_static(x, y, 0.55f, 2.2f); if (G.nlights > nl) G.lights[nl].phase = -1000.0f; }   /* its glow */
    }
}
static void drawn_walls(void) {                                    /* the exit and the places to mend, in the walls drawn */
    const Drawn *D = drawn();
    G.exit_fx = G.exit_fy = -1; G.exit_in_vault = -1;
    G.nrest = 0; G.rest_on = 0; G.rest_goal = -1;
    for (int y = 0; y < D->h; y++)
        for (int x = 0; x < D->w; x++) {
            char ch = D->map[y][x];
            int wx = D->ox + x, wy = D->oy + y, fx, fy, d;
            if (ch == 'X') {
                d = drawn_front(wx, wy, &fx, &fy);
                if (fx < 0) continue;
                G.exit_fx = fx; G.exit_fy = fy; G.exit_dir = d; G.exit_wx = wx; G.exit_wy = wy;
                G.exit_in_vault = vault_of_room(fx, fy);           /* level 2's is behind the crimson door */
                G.map[wy][wx].tex = T_EXIT0;
                light_static(fx + 0.5f + DX[d] * 0.35f, fy + 0.5f + DY[d] * 0.35f, 0.9f, 3.2f);   /* its runes light the stone */
            } else if (ch == 'h' && G.nrest < 2) {
                d = drawn_front(wx, wy, &fx, &fy);
                if (fx < 0) continue;
                G.map[wy][wx].tex = T_REST;
                G.rest_x[G.nrest] = wx; G.rest_y[G.nrest] = wy; G.rest_fx[G.nrest] = fx; G.rest_fy[G.nrest] = fy; G.nrest++;
                light_static(fx + 0.5f + DX[d] * 0.38f, fy + 0.5f + DY[d] * 0.38f, 0.75f, 2.8f);   /* its candles */
            }
        }
#ifdef CRAWL_DEBUG
    fprintf(stderr, "DRAWN level %d: exit (%d,%d) before (%d,%d), %d to mend, %d rooms, %d doors\n", G.level, G.exit_wx, G.exit_wy, G.exit_fx, G.exit_fy, G.nrest, G.nrooms, G.ndoors);
#endif
}
static void drawn_dead(void) {                                     /* the dead where drawn, rank I, each at its post */
    const Drawn *D = drawn();
    for (int i = 0; i < D->nthing; i++) {
        const DrawnThing *t = &D->thing[i];
        if (t->what != DT_DEAD) continue;
        float x = D->ox + t->x, y = D->oy + t->y;
        Mob *m = t->kind == MOB_SHAMBLER ? riser_make(t->kind, (int)x, (int)y, -1) : spawn_mob((int)x, (int)y, t->kind);   /* under turned earth */
        if (!m) continue;
        m->x = x; m->y = y;
        set_rank(m, RK_I);
        m->home = G.map[m->cy][m->cx].region;
    }
}
static void drawn_marks(void) {                                    /* level 2's things that want a spell, its switch and its gate's seal */
    const Drawn *D = drawn();
    G.nmk = 0; G.ward_said = 0;
    int own = cls_el() >= 0 ? cls_el() : LEDGER.lent - 1;          /* the line held: the class's, or one lent */
    for (int e = 0; e < NELEM && (own < 0 || !align_open(own)); e++) if (align_open(e)) own = e;
    if (own < 0 || !align_open(own)) own = -1;
    for (int i = 0, n = 0; i < D->nthing; i++) {
        const DrawnThing *t = &D->thing[i];
        if (t->what != DT_MARK && t->what != DT_SWITCH && t->what != DT_GSEAL) continue;
        float x = D->ox + t->x, y = D->oy + t->y;
        int kind = t->what == DT_SWITCH ? MKK_RSEAL : t->what == DT_GSEAL ? MKK_GSEAL : t->kind == 0 ? MKK_COFFER : t->kind == 1 ? MKK_CAGE : -1;
        int el = own >= 0 ? own : EL_PHYS;                         /* the switch and the gate take any spell, whatever is held */
        if (kind < 0) { if (own < 0) continue; kind = own == EL_FIRE && (n++ & 1) ? MKK_RACK : MK_OWN[own]; }
        else if (t->what == DT_MARK && own < 0) continue;
        Marked *m = mk_add(kind, el, (int)x, (int)y);
        if (!m) continue;
        m->x = m->hx = m->vx = x; m->y = m->hy = m->vy = y;
        if (m->deco >= 0) mk_deco(m, MKK[kind].s0, 1);
        if (kind == MKK_RSEAL) { m->pips = 3; IC.sw = G.nmk - 1; }
        if (kind == MKK_GSEAL) { m->pips = 1; m->door = IC.gate_door; IC.gate = G.nmk - 1; }
    }
#ifdef CRAWL_DEBUG
    fprintf(stderr, "DRAWN level %d: %d things that want a spell (line %d), the switch %d, the gate's seal %d (door %d), %d ice\n", G.level, G.nmk, own, IC.sw, IC.gate, IC.gate_door, IC.n);
    if (getenv("CRAWL_ICE") && !atoi(getenv("CRAWL_ICE"))) IC.live = 0;   /* testing: the ice down from the start */
#endif
}

static void drawn_fire_lights(void) {                              /* a light a row, flickering, low as it goes out */
    if (!g_drawn || !DF.n || DF.fade <= 0.02f) return;
    for (int i = 0; i < DF.n; i++) {
        if (i && DF.cy[i] == DF.cy[i - 1]) continue;               /* the first cell of each row */
        float sx = 0.0f; int k = 0;
        for (int j = i; j < DF.n && DF.cy[j] == DF.cy[i]; j++) { sx += (float)DF.cx[j] + 0.5f; k++; }
        light_dynamic(sx / (float)k, (float)DF.cy[i] + 0.5f, 1.6f * DF.fade * (0.82f + 0.18f * sinf((float)G.t * (13.0f + (float)i) + (float)i)), 3.6f);
    }
}
static void drawn_fire_tick(float dt) {                            /* every frame, whoever plays */
    if (!g_drawn || !DF.n || DF.fade <= 0.0f) return;
    if (DF.out_t > 0.0f) {                                         /* past: burning low, then out */
        DF.out_t -= dt; DF.fade = fmaxf(0.0f, DF.out_t / DFIRE_OUT);
#ifdef CRAWL_DEBUG
        if (DF.fade <= 0.0f) fprintf(stderr, "EVENT drawn_fire out t=%.2f\n", G.t);
#endif
    } else if (!G.dead && G.py >= DF.past_y) {
        DF.out_t = DFIRE_OUT;
#ifdef CRAWL_DEBUG
        fprintf(stderr, "EVENT drawn_fire past hp=%d/%d burn=%.1f t=%.2f\n", G.hp, G.max_hp, G.af_burn, G.t);
#endif
    }
    if ((DF.crackle -= dt) <= 0.0f) {                              /* heard near it */
        DF.crackle = 0.8f + 0.6f * rndf();
        int i = rnd(DF.n);
        sfx_at(SFX_CRACKLE, (float)DF.cx[i] + 0.5f, (float)DF.cy[i] + 0.5f, 0.9f * DF.fade);
    }
    int cx = (int)floorf(G.px), cy = (int)floorf(G.py);
    int in = cx >= 0 && cy >= 0 && cx < MAPN && cy < MAPN && DF.at[cy][cx];
    if (!in || G.dead || DF.fade < 0.35f || G.shop) { DF.inside = 0; return; }
    float floor_hp = DFIRE_FLOOR * (float)G.max_hp;
    if (!DF.inside) {                                              /* the first touch: alight */
        DF.inside = 1; DF.tick = 0.0f;
        G.blame = (Blame){ BL_HAZARD, HZ_FIRE, 0 };
        if (af_can(AF_FIRE)) af_ignite(fminf(DFIRE_AFTER * (float)G.max_hp, fmaxf(0.0f, (float)G.hp - floor_hp)));
    }
    if ((DF.tick -= dt) > 0.0f) return;
    DF.tick += DFIRE_TICK;
    float room = (float)G.hp - floor_hp - G.af_burn, dmg = fminf(DFIRE_SHARE * (float)G.max_hp * DF.fade, room);
    if (dmg >= 1.0f) { G.blame = (Blame){ BL_HAZARD, HZ_FIRE, 0 }; harm_next = EL_FIRE; hurt_ex(dmg, HURT_ELEM, G.px, G.py); }
    G.af_burn = fminf(G.af_burn, fmaxf(0.0f, (float)G.hp - floor_hp));   /* nor the burning after, below the floor */
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT drawn_fire hurt %.1f hp=%d/%d burn=%.1f %s t=%.2f\n", dmg, G.hp, G.max_hp, G.af_burn, G.player ? "player" : "autopilot", G.t);
#endif
}

/* level 2's ice (docs/PLAN-tutorial.md 7.1) */
static float ice_k(int i) {                                        /* how far a cell is laid: 0 bare, 1 ice */
    float u = clampf((IC.t - (IC.live ? IC.wait[i] : 1.2f - IC.wait[i])) / ICE_TURN, 0.0f, 1.0f);
    return IC.live ? u : 1.0f - u;
}
static int ice_at(int x, int y) {                                  /* ice to stand on here */
    return g_drawn == 2 && x >= 0 && y >= 0 && x < MAPN && y < MAPN && IC.at[y][x] && ice_k(IC.at[y][x] - 1) > 0.5f;
}
static void ice_down(void) {                                       /* the switch struck: it thaws */
    IC.live = 0; IC.t = 0.0f;
    if (IC.n) { sfx_at(SFX_HISS, (float)IC.cx[IC.n / 2] + 0.5f, (float)IC.cy[IC.n / 2] + 0.5f, 1.1f); sfx_at(SFX_SHATTER, (float)IC.cx[0] + 0.5f, (float)IC.cy[0] + 0.5f, 0.5f); }
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT ice down by %s t=%.2f\n", G.player ? "player" : "autopilot", G.t);
#endif
}
static void ice_up(void) {                                         /* the map taken: it creeps back in, and the switch is spent */
    if (!IC.n || IC.spent) return;
    IC.live = 1; IC.spent = 1; IC.t = 0.0f;
    if (IC.sw >= 0 && IC.sw < G.nmk) { Marked *m = &G.mk[IC.sw]; m->done = 1; m->done_t = 9.0f; mk_deco(m, SP_MK_RSEAL2, 1); sfx_at(SFX_SHATTER, m->x, m->y, 0.8f); }
    sfx_at(SFX_FROST, (float)IC.cx[IC.n / 2] + 0.5f, (float)IC.cy[IC.n / 2] + 0.5f, 1.2f); snd_sfx_ex(SFX_DEEPCHIME, 0.5f, 0.5f);
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT ice up t=%.2f\n", G.t);
#endif
}
static void ice_tick(float dt) {                                   /* every frame, whoever plays */
    if (g_drawn != 2 || !IC.n) return;
    IC.t += dt;
    if (IC.live && (IC.breath -= dt) <= 0.0f) {                    /* a cold breath, heard near it */
        IC.breath = 2.5f + 2.0f * rndf();
        int i = rnd(IC.n);
        sfx_at(SFX_FROST, (float)IC.cx[i] + 0.5f, (float)IC.cy[i] + 0.5f, 0.3f);
    }
    for (int j = 0; j < MAXMOBS; j++) {                            /* what stands on it is held in it */
        Mob *m = &G.mobs[j];
        if (m->alive && m->state == MS_WALK && !KIND_IS(m, KF_SPIRIT) && !buried(m) && ice_at((int)m->x, (int)m->y)) m->frozen = fmaxf(m->frozen, 0.3f);
    }
    if (G.dead || G.shop || tu_taken() || !ice_at((int)floorf(G.px), (int)floorf(G.py))) { IC.stack = 0.0f; return; }   /* none while led */
    if ((IC.stack -= dt) > 0.0f) return;
    IC.stack = ICE_STACK;
    if (!af_can(AF_CHILL)) return;
    if (G.af_n < 3) af_chill(1, 0);                                /* held at three on it: it never freezes a person */
    else G.af[AF_CHILL] = G.af0[AF_CHILL];
}
