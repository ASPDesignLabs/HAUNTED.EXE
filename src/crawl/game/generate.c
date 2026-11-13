/* A part of scene_crawl.c (docs/PLAN-tidy.md): it is #included there in its place and never compiled on its own.
 * Level generation: carving, themes and room types, decorations and static lights; the plan (maze, vaults, widths); secrets placed; the sliding wall's box. */

/* ================================================================== */
/* Level generation: rooms and mazes                                   */
/* ================================================================== */

static void carve(int x, int y, int region, int theme) {
    Cell *c = &G.map[y][x];
    c->kind = K_FLOOR; c->region = (int16_t)region; c->theme = (uint8_t)theme;
}

static int theme_tex(int theme, uint32_t h) {
    int k = (int)(h % 10);
    switch (theme) {
    case TH_OSSUARY: return k < 6 ? T_SKULLS : (k < 9 ? T_NICHE : T_STONE);
    case TH_CRYPT:   return k < 5 ? T_STONE : (k < 8 ? T_NICHE : T_BANNER);
    case TH_BRICK:   return k < 8 ? T_BRICK : T_BANNER;
    case TH_MOSS:    return k < 6 ? T_MOSS : T_RUBBLE;
    default:         return k < 6 ? T_STONE : (k < 9 ? T_RUBBLE : T_SKULLS);
    }
}

/* Each run's rooms (docs/PLAN-b10.md): their floor and ceiling (in their run's slots), their dressing's
 * style (0 a crypt, 1 graves, 2 a fire, 3 cages), and the old theme (TH_*) their dressing and dwellers
 * still follow (style 1 and 3 mean what the run makes of them: carts in the Hollows, altars in the Deep).
 * What their walls are made of and carry is in lay_stretch. The Crypts, her hall (always in the Crypts'
 * stone), the Catacombs, the Hollows and the Deep. The halls are each run's own, in the same slots. */
enum { RT_HALL, RT_OSSUARY, RT_BURIAL, RT_CHAPTER, RT_KNIGHTS, RT_UNDER, RT_MERCHANT, RT_PLOT,
       RT_SAINTS, RT_CISTERN, RT_NAVE, RT_CANAL, RT_SPINNERS,
       RT_GALLERY, RT_STOPE, RT_CHARNEL, RT_FOSSIL, RT_CHAPEL,
       RT_GEODE, RT_GLASS, RT_CINDER, RT_VENT, RT_THRONE, NRT };
typedef struct { const char *name; uint8_t theme, style, floor, ceil; } RType;
static const RType RTYPES[NRT] = {
    [RT_HALL]     = { "THE HALLS",      TH_CORRIDOR, 0, FL1_SETTS,   CE1_VAULT },
    [RT_OSSUARY]  = { "OSSUARY",        TH_OSSUARY,  0, FL1_EARTH,   CE1_VAULT },
    [RT_BURIAL]   = { "BURIAL VAULT",   TH_CRYPT,    1, FL1_FLAGS,   CE1_RIB },
    [RT_CHAPTER]  = { "CHAPTER HOUSE",  TH_BRICK,    0, FL1_CHAPTER, CE1_RIB },   /* no dwellers kept here */
    [RT_KNIGHTS]  = { "KNIGHTS' VAULT", TH_CRYPT,    1, FL1_FLAGS,   CE1_RIB },
    [RT_UNDER]    = { "UNDERCROFT",     TH_MOSS,     2, FL1_SETTS,   CE1_ROCK },
    [RT_MERCHANT] = { "HER HALL",       TH_BRICK,    0, FL1_FLAGS,   CE1_RIB },
    [RT_PLOT]     = { "THE GRAVE PLOT", TH_CRYPT,    0, FL1_EARTH,   CE1_VAULT },
    [RT_SAINTS]   = { "SAINTS' CATACOMB", TH_CRYPT,  1, FL2_FLAGS,   CE2_VAULT },
    [RT_CISTERN]  = { "CISTERN",        TH_MOSS,     0, FL2_FLAGS,   CE2_DRIP },   /* water in the middle, flags round it */
    [RT_NAVE]     = { "SILTED NAVE",    TH_MOSS,     1, FL2_SILT,    CE2_VAULT },   /* its graves, but no dwellers kept */
    [RT_CANAL]    = { "CANAL",          TH_BRICK,    0, FL2_FLAGS,   CE2_ROCK },   /* water down the middle */
    [RT_SPINNERS] = { "SPINNERS' GALLERY", TH_OSSUARY, 0, FL2_SILT,  CE2_DRIP },
    [RT_GALLERY]  = { "TIMBERED GALLERY", TH_CRYPT,  1, FL3_GRAVEL,  CE3_LAGGING },   /* rails down the middle, carts by them */
    [RT_STOPE]    = { "STOPE",          TH_MOSS,     2, FL3_GRAVEL,  CE3_ROCK },
    [RT_CHARNEL]  = { "CHARNEL PIT",    TH_OSSUARY,  0, FL3_CHARNEL, CE3_ROOTS },
    [RT_FOSSIL]   = { "FOSSIL HALL",    TH_CRYPT,    0, FL3_EARTH,   CE3_ROOTS },
    [RT_CHAPEL]   = { "THE CHAPEL",     TH_CRYPT,    1, FL3_EARTH,   CE3_ROCK },   /* the cult's altars and candles */
    [RT_GEODE]    = { "GEODE",          TH_MOSS,     0, FL4_GLASS,   CE4_CRYSTAL },   /* crystals in its corners */
    [RT_GLASS]    = { "GLASS HALL",     TH_CRYPT,    3, FL4_GLASS,   CE4_ROCK },
    [RT_CINDER]   = { "CINDER FIELD",   TH_OSSUARY,  2, FL4_CINDER,  CE4_SOOT },
    [RT_VENT]     = { "MAGMA VENT",     TH_BRICK,    2, FL4_CINDER,  CE4_SOOT },   /* magma welling up in its corners */
    [RT_THRONE]   = { "THE THRONE",     TH_CRYPT,    1, FL4_THRONE,  CE4_CRYSTAL },
};
static int nr_type[NRT];                                            /* this level's rooms of each type so far */
/* a room's type by what it is: its size in blocks, and how far into its run the level lies */
static int pick_rtype(int w, int h) {
    int bw = (w + 1) / 2, bh = (h + 1) / 2, area = bw * bh, step = crawl_step(G.level);
    float wt[NRT] = { 0 }, sum = 0.0f;
    if (G.look == 3) {                                              /* the Deep */
        wt[RT_GLASS] = 2.6f / (1.0f + 0.5f * nr_type[RT_GLASS]);
        wt[RT_GEODE] = (area <= 2 ? 2.4f : 1.2f) / (1.0f + 0.6f * nr_type[RT_GEODE]);
        wt[RT_CINDER] = area >= 2 ? (1.6f + 0.15f * step) / (1.0f + 0.5f * nr_type[RT_CINDER]) : 0.0f;
        wt[RT_VENT] = area >= 2 && nr_type[RT_VENT] < 1 + (step >= 4) ? 1.8f : 0.0f;
        wt[RT_THRONE] = step >= 4 && area >= 4 && !nr_type[RT_THRONE] ? 1.5f + 0.3f * step : 0.0f;
    } else if (G.look == 2) {                                       /* the Hollows */
        int lng = bw > bh ? bw : bh, shrt = bw > bh ? bh : bw, long_room = lng >= 3 && lng >= 2 * shrt;
        wt[RT_GALLERY] = (long_room ? 3.2f : 1.0f) / (1.0f + 0.5f * nr_type[RT_GALLERY]);
        wt[RT_STOPE] = (area >= 4 ? 2.8f : 1.4f) / (1.0f + 0.5f * nr_type[RT_STOPE]);
        wt[RT_CHARNEL] = nr_type[RT_CHARNEL] < 1 + (step >= 5) ? 1.6f + 0.1f * step : 0.0f;
        wt[RT_FOSSIL] = area >= 2 && nr_type[RT_FOSSIL] < 1 + (step >= 6) ? 1.8f : 0.0f;
        wt[RT_CHAPEL] = step >= 6 && area >= 4 && !nr_type[RT_CHAPEL] ? 1.5f + 0.3f * step : 0.0f;   /* the cult's, at the run's end */
        wt[RT_STOPE] += area <= 2 ? 1.0f : 0.0f;
    } else if (G.look == 1) {                                       /* the Catacombs */
        int lng = bw > bh ? bw : bh, shrt = bw > bh ? bh : bw, long_room = lng >= 3 && lng >= 2 * shrt;
        wt[RT_SAINTS] = 2.6f / (1.0f + 0.6f * nr_type[RT_SAINTS]);     /* the commonest, but not every room */
        wt[RT_NAVE] = ((area <= 2 ? 2.4f : 1.8f) + (long_room ? 1.0f : 0.0f)) / (1.0f + 0.3f * nr_type[RT_NAVE]);
        wt[RT_CANAL] = long_room && nr_type[RT_CANAL] < 1 + step / 4 ? 3.0f : 0.0f;   /* only a long room */
        wt[RT_CISTERN] = area >= 4 && !long_room && nr_type[RT_CISTERN] < 1 + (step >= 5) ? 2.6f : 0.0f;
        wt[RT_SPINNERS] = step >= 2 && area >= 2 && nr_type[RT_SPINNERS] < 1 + (step >= 6) ? 1.0f + 0.2f * step : 0.0f;
    } else {
    wt[RT_OSSUARY] = nr_type[RT_OSSUARY] < 2 + (step >= 6) ? 1.0f + 0.1f * step : 0.0f;   /* a display: two a level, three late in the run */
    wt[RT_BURIAL] = 3.0f / (1.0f + 0.45f * nr_type[RT_BURIAL]);         /* the commonest, but not every room */
    wt[RT_CHAPTER] = area >= 4 && !nr_type[RT_CHAPTER] ? 3.5f : 0.0f;
    wt[RT_KNIGHTS] = area >= 2 && step >= 2 && nr_type[RT_KNIGHTS] < 1 + step / 5 ? 1.6f + 0.15f * step : 0.0f;
    wt[RT_UNDER] = area <= 2 ? 3.0f : 1.0f;
    }
    for (int k = 0; k < NRT; k++) sum += wt[k];
    float r = rndf() * sum;
    int k = G.look == 3 ? RT_GLASS : G.look == 2 ? RT_STOPE : G.look == 1 ? RT_SAINTS : RT_BURIAL;
    for (int i = 0; i < NRT; i++) { if (wt[i] > 0.0f && r < wt[i]) { k = i; break; } r -= wt[i]; }
    nr_type[k]++;
    return k;
}

static void add_deco_ex(float x, float y, int spr, int nfr, int flags) {
    if (G.ndecos < MAXDECO) G.decos[G.ndecos++] = (Deco){ x, y, rndf() * 10.0f, spr, nfr, flags };
}
static void add_deco(float x, float y, int spr) {
    add_deco_ex(x, y, spr, 1, spr == SP_CHAINS ? DF_SWAY : 0);
    if (spr == SP_SKULLPILE || spr == SP_BONES2 || spr == SP_URN || spr == SP_CANDLES || spr == SP_CHAINS || spr == SP_BROKEN_ARMS
        || spr == SP_COLLAR || spr == SP_CARCASS || spr == SP_HELM || spr == SP_FUNGUS || spr == SP_SAC_A)
        G.bulky[clampi((int)y, 0, MAPN - 1)][clampi((int)x, 0, MAPN - 1)] = 1;
}

/* static light: record its contribution to every light-map texel it reaches
 * through open space (and the faces of the walls around that space) */
static void light_static_ph(float lx, float ly, float power, float radius, float phase);
static void light_static(float lx, float ly, float power, float radius) {
    if (G.nlights >= MAXLIGHTS) return;
    light_static_ph(lx, ly, power, radius, rndf() * 10.0f);
}
/* a light whose flicker's phase is given, so the dice aren't touched (the portraits' candles: the dream's runs keep step) */
static void light_static_ph(float lx, float ly, float power, float radius, float phase) {
    if (G.nlights >= MAXLIGHTS) return;
    Light *L = &G.lights[G.nlights++];
    L->x = lx; L->y = ly; L->power = power; L->radius = radius; L->phase = phase;
    L->cx = (int)lx; L->cy = (int)ly; L->first = G.nlc; L->count = 0;
    static int stamp[MAPN][MAPN], qx[MAPN * MAPN], qy[MAPN * MAPN], dist[MAPN][MAPN];
    static int gen;
    gen++;
    int qh = 0, qt = 0, maxd = (int)ceilf(radius) + 1;
    qx[qt] = L->cx; qy[qt] = L->cy; qt++; stamp[L->cy][L->cx] = gen; dist[L->cy][L->cx] = 0;
    while (qh < qt) {
        int x = qx[qh], y = qy[qh]; qh++;
        if (dist[y][x] >= maxd) continue;
        for (int d = 0; d < 4; d++) {
            int nx = x + DX[d], ny = y + DY[d];
            if (nx < 0 || ny < 0 || nx >= MAPN || ny >= MAPN || stamp[ny][nx] == gen) continue;
            stamp[ny][nx] = gen; dist[ny][nx] = dist[y][x] + 1;
            if (G.map[ny][nx].kind != K_WALL) { qx[qt] = nx; qy[qt] = ny; qt++; }
            else { qx[qt] = nx; qy[qt] = ny; qt++; dist[ny][nx] = maxd; }   /* lit face, no further */
        }
    }
    for (int i = 0; i < qt; i++) {
        int x = qx[i], y = qy[i];
        for (int ty = 0; ty < LMR; ty++)
            for (int tx = 0; tx < LMR; tx++) {
                float wx = x + (tx + 0.5f) / LMR, wy = y + (ty + 0.5f) / LMR;
                float dd = sqrtf((wx - lx) * (wx - lx) + (wy - ly) * (wy - ly));
                if (dd >= radius || G.nlc >= MAXLC) continue;
                float w = power / (1.0f + 0.9f * dd * dd) * (1.0f - smoothstepf(radius * 0.55f, radius, dd));
                G.lc_idx[G.nlc] = (uint16_t)((y * LMR + ty) * LMN + x * LMR + tx);
                G.lc_w[G.nlc] = w;
                G.nlc++; L->count++;
            }
    }
}

static int in_room(const Room *R, int x, int y) { return x >= R->x && y >= R->y && x < R->x + R->w && y < R->y + R->h; }

static void dress_level(void);
/* ---- the plan ----
 * A level is laid out first as NB x NB blocks of 3x3 cells with a one-cell wall between
 * neighbouring blocks: on the plan (MG x MG) odd rows and columns are blocks, even ones the walls
 * between them. Rooms span several blocks; the halls between them are a maze of blocks. The plan is
 * then drawn into the cell map, where a stretch of hall is 1, 2 or 3 cells wide but always keeps
 * its middle lane, and the autopilot walks those lanes: every cell with x or y = 2 (mod 4). */
#define NB 11
#define MG (2 * NB + 1)
typedef struct { uint8_t kind, theme, w, lo, trunk; int8_t room; int16_t region; uint8_t keep; } MCell;   /* keep: not filled as a dead end */
static MCell MM[MG][MG];
static int mroom[12][4];                                         /* the rooms on the plan: x, y, w, h */
static int mstart(int m) { return (m & 1) ? 2 * m - 1 : 2 * m; }  /* the first cell of plan row/column m */
static MCell *mc(int x, int y) {
    static MCell solid = { K_WALL, 0, 0, 0, 0, -1, -1, 0 };
    return (x < 0 || y < 0 || x >= MG || y >= MG) ? &solid : &MM[y][x];
}
static int mopen(int x, int y) { int k = mc(x, y)->kind; return k == K_FLOOR || k == K_DOOR; }
static int mdeg(int x, int y) { int n = 0; for (int d = 0; d < 4; d++) n += mopen(x + DX[d], y + DY[d]); return n; }
static void mcarve(int x, int y, int region, int theme, int room) {
    MCell *c = &MM[y][x];
    c->kind = K_FLOOR; c->region = (int16_t)region; c->theme = (uint8_t)theme; c->room = (int8_t)room;
}
static int in_mroom(int r, int x, int y) { return x >= mroom[r][0] && y >= mroom[r][1] && x < mroom[r][0] + mroom[r][2] && y < mroom[r][1] + mroom[r][3]; }
/* a drawn level (docs/PLAN-tutorial.md) isn't on the plan's grid: its lanes are drawn with it, and each cell's steps to one kept */
static int g_drawn;                                                /* 1 or 2: the drawn level in play; 0 any other place */
static uint8_t g_lane[MAPN][MAPN], g_ldist[MAPN][MAPN];
/* level 1's burning floor (docs/PLAN-tutorial.md, section 6): lamp oil spilled across the hall and alight, crossed to go on */
#define DFIRE_CELLS 16
#define DFIRE_OUT 20.0f                                            /* it burns low and goes out this long after you are past */
#define DFIRE_TICK 0.5f                                            /* its harm comes this often while you stand in it */
#define DFIRE_SHARE 0.17f                                          /* a share of your health each time */
#define DFIRE_AFTER 0.20f                                          /* and On Fire's harm after, at the first touch */
#define DFIRE_FLOOR 0.25f                                          /* never under this share of health, the burning after included */
static struct {
    int n, inside, cx[DFIRE_CELLS], cy[DFIRE_CELLS];
    float fade, out_t, tick, crackle, past_y;                      /* past_y: two cells beyond it, where it starts to go out */
    uint8_t at[MAPN][MAPN];
} DF;
static void drawn_fire_lights(void);
/* level 2's ice field (docs/PLAN-tutorial.md, section 7.1): it freezes what stands on it and holds the caster at three stacks; the
 * rimed seal in the far room takes it down, and it comes back, for good, as the map is taken */
#define ICE_CELLS 24
#define ICE_TURN 0.8f                                              /* a cell turns over this long, after its wait (up to 1.2 s) */
#define ICE_STACK 0.4f                                             /* a Chilled stack this often on it */
static struct {
    int n, live, spent, sw, gate, gate_door;                       /* sw, gate: the switch's and the gate's things; gate_door, -1 none */
    int cx[ICE_CELLS], cy[ICE_CELLS];
    float wait[ICE_CELLS], t, stack, breath;                       /* each cell's wait as it turns; since it last turned */
    uint8_t at[MAPN][MAPN];                                        /* 1 + its index */
} IC;
static void ice_down(void);
static void ice_up(void);
static int tu_taken(void);                                         /* the lessons have the body (a turning, a hold, a lead) */
static int tu_exit_barred(void);                                   /* level 2's way out, shut while the lessons want the map */
static void tu_map_taken(void);
static int g_map_kept;                                             /* her room keeps the map off her wares (level 2's lessons to come) */
static void tu_level_begin(void);                                 /* the lessons (docs/PLAN-tutorial.md), further down */
static void tu_shop_begin(void);
static void tu_cleared(void);
static int tu_mercy(void);
static void tu_show(void);
static int lane(int x, int y) {
    if (g_drawn) return x >= 0 && y >= 0 && x < MAPN && y < MAPN && g_lane[y][x];
    return (x & 3) == 2 || (y & 3) == 2;
}
static int lane_dist(int x, int y) {
    if (g_drawn) return x >= 0 && y >= 0 && x < MAPN && y < MAPN ? g_ldist[y][x] : 9;
    int a = abs((x & 3) - 2), b = abs((y & 3) - 2); return a < b ? a : b;
}
/* where the autopilot may walk: the middle lanes */
static int apass(int x, int y) { return lane(x, y) && passable(x, y); }

static int uf[1024];
static int uf_find(int a) { while (uf[a] != a) a = uf[a] = uf[uf[a]]; return a; }

static void mz_grow(int sx, int sy, int region) {
    static int st[MG * MG][2];
    int sp = 0, last = -1;
    mcarve(sx, sy, region, TH_CORRIDOR, -1);
    st[sp][0] = sx; st[sp][1] = sy; sp++;
    while (sp) {
        int x = st[sp - 1][0], y = st[sp - 1][1], avail[4], na = 0;
        for (int d = 0; d < 4; d++) {
            int nx = x + DX[d] * 2, ny = y + DY[d] * 2;
            if (nx >= 1 && ny >= 1 && nx < MG - 1 && ny < MG - 1 && MM[ny][nx].kind == K_WALL) avail[na++] = d;
        }
        if (!na) { sp--; last = -1; continue; }
        int d = avail[rnd(na)];
        for (int i = 0; i < na; i++) if (avail[i] == last && rndf() < 0.5f) d = last;   /* long straight runs */
        mcarve(x + DX[d], y + DY[d], region, TH_CORRIDOR, -1);
        mcarve(x + DX[d] * 2, y + DY[d] * 2, region, TH_CORRIDOR, -1);
        st[sp][0] = x + DX[d] * 2; st[sp][1] = y + DY[d] * 2; sp++;
        last = d;
    }
}

static void mz_fill_dead_ends(void) {                            /* so the walk rarely has to turn back */
    for (int changed = 1; changed; ) {
        changed = 0;
        for (int y = 1; y < MG - 1; y++)
            for (int x = 1; x < MG - 1; x++) {
                if (!mopen(x, y) || MM[y][x].room >= 0 || MM[y][x].keep || mdeg(x, y) > 1) continue;
                MM[y][x].kind = K_WALL; MM[y][x].region = -1; changed = 1;
            }
    }
}

/* Make one room a vault with a single way in. Its other entrances are walled up, but only if the
 * rest of the level stays connected without the room. */
static int mz_min, mz_max = 15;                                     /* the least a vault may be (the wing: two blocks), and the most */
static int in_wing(int x, int y);
static int in_trial(int x, int y);
static int tri_level(int level);
static int tri_kind_of(int level);
static void tri_touched(int i, int how);
static int thing_use(int i);
static int pz_level(int level);
static int pz_kind_of(int level);
static int mz_vault(int *kx, int *ky, int *room) {
    int order[12], no = 0;
    for (int r = 1; r < G.nrooms; r++) {
        int taken = 0;
        for (int k = 0; k < G.nvaults; k++) taken |= G.vault_room[k] == r;
        taken |= r == G.pz_room || r == G.tri_room;                 /* nor the wing, nor a trial's room */
        if (!taken) order[no++] = r;
    }
    for (int i = no - 1; i > 0; i--) { int j = rnd(i + 1), t = order[i]; order[i] = order[j]; order[j] = t; }
    int sx = mroom[0][0], sy = mroom[0][1];
    for (int oi = 0; oi < no; oi++) {
        int r = order[oi], *R = mroom[r];
        if (R[2] * R[3] > mz_max || R[2] * R[3] < mz_min) continue;   /* the big halls stay open */
        int ex[24], ey[24], ed[24], ne = 0;
        for (int y = R[1]; y < R[1] + R[3]; y++)
            for (int x = R[0]; x < R[0] + R[2]; x++)
                for (int d = 0; d < 4; d++) {
                    int nx = x + DX[d], ny = y + DY[d];
                    if (in_mroom(r, nx, ny) || !mopen(nx, ny) || ne >= 24) continue;
                    ex[ne] = nx; ey[ne] = ny; ed[ne] = d; ne++;
                }
        if (!ne || ne > 8) continue;
        int kept = 0;                                              /* the way down to a boss's arena is never sealed */
        for (int i = 0; i < ne; i++) kept |= MM[ey[i]][ex[i]].keep;
        if (kept) continue;
        int keep = -1;
        for (int i = 0; i < ne; i++) {
            if (!mopen(ex[i] + DX[ed[i]], ey[i] + DY[ed[i]])) continue;
            if (keep < 0 || MM[ey[i]][ex[i]].kind == K_DOOR) keep = i;
        }
        if (keep < 0) continue;
        uint8_t saved[24];
        for (int i = 0; i < ne; i++) { saved[i] = MM[ey[i]][ex[i]].kind; if (i != keep) MM[ey[i]][ex[i]].kind = K_WALL; }
        static uint8_t seen[MG][MG]; static int qx[MG * MG], qy[MG * MG];   /* the rest must still be reachable */
        memset(seen, 0, sizeof seen);
        int qh = 0, qt = 0, total = 0;
        for (int y = 0; y < MG; y++)
            for (int x = 0; x < MG; x++)
                if (mopen(x, y) && !in_mroom(r, x, y) && !(x == ex[keep] && y == ey[keep])) total++;
        seen[sy][sx] = 1; qx[qt] = sx; qy[qt] = sy; qt++;
        while (qh < qt) {
            int x = qx[qh], y = qy[qh]; qh++;
            for (int d = 0; d < 4; d++) {
                int nx = x + DX[d], ny = y + DY[d];
                if (seen[ny][nx] || !mopen(nx, ny) || in_mroom(r, nx, ny) || (nx == ex[keep] && ny == ey[keep])) continue;
                seen[ny][nx] = 1; qx[qt] = nx; qy[qt] = ny; qt++;
            }
        }
        if (qt == total) {
            for (int i = 0; i < ne; i++) if (i != keep) MM[ey[i]][ex[i]].region = -1;
            MM[ey[keep]][ex[keep]].kind = K_DOOR;
            *room = r; *kx = ex[keep]; *ky = ey[keep];
            return 1;
        }
        for (int i = 0; i < ne; i++) MM[ey[i]][ex[i]].kind = saved[i];
    }
    return 0;
}

/* how wide each stretch of hall is: the trunk (from the first room to the farthest) three cells,
 * the rest one, two or three, a whole stretch between junctions alike; junctions are open blocks */
static void mz_widths(void) {
    static int dist[MG][MG], from[MG][MG], qx[MG * MG], qy[MG * MG];
    for (int y = 0; y < MG; y++) for (int x = 0; x < MG; x++) dist[y][x] = -1;
    int sx = mroom[0][0], sy = mroom[0][1], qh = 0, qt = 0, far = -1, fd = -1;
    dist[sy][sx] = 0; qx[qt] = sx; qy[qt] = sy; qt++;
    while (qh < qt) {
        int x = qx[qh], y = qy[qh]; qh++;
        if (MM[y][x].room > 0 && dist[y][x] > fd) { fd = dist[y][x]; far = y * MG + x; }
        for (int d = 0; d < 4; d++) {
            int nx = x + DX[d], ny = y + DY[d];
            if (!mopen(nx, ny) || dist[ny][nx] >= 0) continue;
            dist[ny][nx] = dist[y][x] + 1; from[ny][nx] = d; qx[qt] = nx; qy[qt] = ny; qt++;
        }
    }
    for (int p = far; p >= 0 && dist[p / MG][p % MG] > 0; ) {
        int x = p % MG, y = p / MG, d = from[y][x];
        MM[y][x].trunk = 1;
        p = (y - DY[d]) * MG + (x - DX[d]);
    }
    #define BI(x, y) (((y) / 2) * NB + (x) / 2)
    #define HALL(x, y) (mopen(x, y) && mc(x, y)->room < 0)
    for (int i = 0; i < NB * NB; i++) uf[i] = i;
    for (int y = 1; y < MG; y += 2)
        for (int x = 1; x < MG; x += 2) {
            if (!HALL(x, y) || mdeg(x, y) != 2) continue;
            for (int d = 0; d < 2; d++) {                          /* east and south: each pair once */
                int nx = x + 2 * DX[d], ny = y + 2 * DY[d];
                if (mopen(x + DX[d], y + DY[d]) && mc(x + DX[d], y + DY[d])->kind == K_FLOOR && HALL(nx, ny) && mdeg(nx, ny) == 2)
                    uf[uf_find(BI(x, y))] = uf_find(BI(nx, ny));
            }
        }
    static int rw[NB * NB], rlo[NB * NB], rtrunk[NB * NB];
    for (int i = 0; i < NB * NB; i++) { rw[i] = -1; rtrunk[i] = 0; }
    for (int y = 1; y < MG; y += 2)
        for (int x = 1; x < MG; x += 2)
            if (HALL(x, y) && (MM[y][x].trunk || MM[y][x + 1].trunk || MM[y + 1][x].trunk || MM[y][x - 1].trunk || MM[y - 1][x].trunk)) rtrunk[uf_find(BI(x, y))] = 1;
    for (int y = 1; y < MG; y += 2)
        for (int x = 1; x < MG; x += 2) {
            if (!HALL(x, y)) continue;
            MCell *m = &MM[y][x];
            if (mdeg(x, y) != 2) { m->w = 3; m->lo = 0; continue; }  /* a junction: open */
            int r = uf_find(BI(x, y));
            if (rw[r] < 0) {
                int k = rnd(20);
                rw[r] = rtrunk[r] ? 3 : (k < 4 ? 1 : (k < 12 ? 2 : 3));
                rlo[r] = rw[r] == 2 ? rnd(2) : (rw[r] == 1 ? 1 : 0);
            }
            m->w = (uint8_t)rw[r]; m->lo = (uint8_t)rlo[r];
        }
    #undef BI
    #undef HALL
}

/* the lanes of hall block (mx,my) on its way out in direction d: lo, and how many */
static void mz_arm(int mx, int my, int d, int *lo, int *w) {
    const MCell *conn = mc(mx + DX[d], my + DY[d]), *nb = mc(mx + 2 * DX[d], my + 2 * DY[d]), *self = mc(mx, my), *src = self;
    if (conn->kind == K_DOOR) { *lo = 1; *w = 1; return; }
    if (self->room >= 0) src = nb;                                 /* a room's archway takes the hall's width */
    else if (nb->room < 0 && mdeg(mx, my) != 2) src = nb;          /* a junction meets each hall at its own width */
    if (src->room >= 0) { *lo = 0; *w = 3; return; }
    *lo = src->lo; *w = src->w;
}

/* the plan, drawn into the cell map */
static void mz_draw(void) {
    for (int my = 0; my < MG; my++)
        for (int mx = 0; mx < MG; mx++) {
            const MCell *m = &MM[my][mx];
            if (!mopen(mx, my)) continue;
            int fx = mstart(mx), fy = mstart(my), bw = (mx & 1) ? 3 : 1, bh = (my & 1) ? 3 : 1;
            if (m->room >= 0) {                                    /* a room: all of it, walls between its blocks too */
                for (int y = fy; y < fy + bh; y++) for (int x = fx; x < fx + bw; x++) carve(x, y, m->region, m->theme);
                continue;
            }
            if ((mx & 1) && (my & 1)) {                            /* a hall block: its middle, and an arm each way out */
                carve(fx + 1, fy + 1, m->region, TH_CORRIDOR);
                for (int d = 0; d < 4; d++) {
                    if (!mopen(mx + DX[d], my + DY[d])) continue;
                    int lo, w;
                    mz_arm(mx, my, d, &lo, &w);
                    for (int l = lo; l < lo + w; l++)
                        for (int k = 0; k < 2; k++) {
                            int lx = DX[d] ? 1 + DX[d] * k : l, ly = DX[d] ? l : 1 + DY[d] * k;
                            carve(fx + lx, fy + ly, m->region, TH_CORRIDOR);
                        }
                }
            } else if ((mx & 1) != (my & 1)) {                     /* the wall between two blocks, opened */
                int horiz = !(mx & 1), d = horiz ? 0 : 1, ax = horiz ? mx - 1 : mx, ay = horiz ? my : my - 1;
                if (m->kind == K_DOOR) {
                    int x = horiz ? fx : fx + 1, y = horiz ? fy + 1 : fy;
                    carve(x, y, m->region, TH_CORRIDOR); G.map[y][x].kind = K_DOOR;
                    continue;
                }
                const MCell *A = mc(ax, ay), *B = mc(ax + 2 * DX[d], ay + 2 * DY[d]);
                int lo = 0, w = 3;
                if (A->room < 0) mz_arm(ax, ay, d, &lo, &w);
                else if (B->room < 0) mz_arm(ax + 2 * DX[d], ay + 2 * DY[d], d + 2, &lo, &w);
                for (int l = lo; l < lo + w; l++) carve(horiz ? fx : fx + l, horiz ? fy + l : fy, m->region, TH_CORRIDOR);
            }
        }
}

/* Secrets (docs/PLAN-b6.md): a set's levels hold 3, 4, 4, 5 and 6 of them, every link of a chain
 * counted, split into chains that grow longer deeper into the set (the first always two or more, so
 * every level has a way into a pocket). Each chain begins in a room behind a cracked wall beside a hall,
 * made of blocks the plan left solid: one, two side by side, or two by two where there is the room;
 * the wall slides right through it and comes to rest at the back. A chain of two or more has a doorway
 * in the back wall, beside where the stone stops. */
static int imin(int a, int b) { return a < b ? a : b; }
static int imax(int a, int b) { return a > b ? a : b; }
static int secret_block_free(int mx, int my) { return mx >= 1 && my >= 1 && mx < MG - 1 && my < MG - 1 && MM[my][mx].kind == K_WALL && !MM[my][mx].trunk; }
/* a doorway into a room's pockets: in its back wall if it can, else a side wall, beside where the stone
 * stops and never behind it, with solid rock beyond (kept so) for the doorway not to show elsewhere */
static int secret_door(Secret *sc) {
    int rx = sc->sx + DX[sc->d] * sc->len, ry = sc->sy + DY[sc->d] * sc->len, best = -1; float bs = -1e9f;
    for (int y = sc->y0 - 1; y <= sc->y1 + 1; y++)
        for (int x = sc->x0 - 1; x <= sc->x1 + 1; x++) {
            int inside = x >= sc->x0 && x <= sc->x1 && y >= sc->y0 && y <= sc->y1;
            if (inside || x < 1 || y < 1 || x >= MAPN - 1 || y >= MAPN - 1) continue;
            for (int n = 0; n < 4; n++) {                          /* n: outwards from the room */
                int ix = x - DX[n], iy = y - DY[n], bx = x + DX[n], by = y + DY[n];
                if (ix < sc->x0 || ix > sc->x1 || iy < sc->y0 || iy > sc->y1 || (ix == rx && iy == ry)) continue;
                if (n == ((sc->d + 2) & 3) || G.map[y][x].kind != K_WALL || G.map[y][x].secret) continue;   /* not the front */
                if (bx < 1 || by < 1 || bx >= MAPN - 1 || by >= MAPN - 1 || G.map[by][bx].kind != K_WALL || G.map[by][bx].secret) continue;
                int along = (ix - sc->sx) * DX[sc->d] + (iy - sc->sy) * DY[sc->d];
                float sc0 = (n == sc->d ? 20.0f : 0.0f) + (float)along + rndf() * 2.0f;
                if (sc0 > bs) { bs = sc0; best = (y * MAPN + x) * 4 + n; }
            }
        }
    if (best < 0) return 0;
    int n = best % 4, x = best / 4 % MAPN, y = best / 4 / MAPN, bx = x + DX[n], by = y + DY[n];
    if ((bx & 3) && (by & 3)) MM[(by - 1) / 4 * 2 + 1][(bx - 1) / 4 * 2 + 1].trunk = 1;
    sc->door_x = x; sc->door_y = y;
    G.map[y][x].secret = 3;
    return 1;
}
static void place_secrets(void) {
    static const int LINKS[RUN_LEN] = { 3, 3, 4, 4, 4, 5, 5, 5, 6, 6 };
    G.nsecrets = G.secrets_found = G.nlinks = G.links_found = G.trials_failed = 0; G.slide = -1;
    int step = crawl_step(G.level), left = LINKS[step], chain[MAXSECRETS], nch = 0;
    float sk = (float)step / (float)(RUN_LEN - 1), longer = 0.3f + 0.4f * sk;   /* longer chains deeper into the run */
    while (left > 0 && nch < MAXSECRETS) {
        int len = 1;
        while (len < left && rndf() < longer) len++;
        if (!nch && len < 2 && left >= 2) len = 2;
        chain[nch++] = len; left -= len;
    }
    left = 0;                                                      /* now: links that found no room, to go to a chain that has one */
    for (int i = 1; i < nch; i++) { int v = chain[i], j = i - 1; while (j >= 0 && chain[j] < v) { chain[j + 1] = chain[j]; j--; } chain[j + 1] = v; }   /* the longest first */
    static int cand[MG * MG * 4];
    int nc = 0;
    for (int my = 1; my < MG; my += 2)
        for (int mx = 1; mx < MG; mx += 2) {
            if (mopen(mx, my) || MM[my][mx].kind != K_WALL) continue;   /* (K_PILLAR on the plan: the arena's) */
            for (int d = 0; d < 4; d++) {                          /* d: from the room towards where it is pressed */
                int cx = mx + DX[d], cy = my + DY[d], nx = mx + 2 * DX[d], ny = my + 2 * DY[d];
                if (nx < 1 || ny < 1 || nx >= MG - 1 || ny >= MG - 1 || mopen(cx, cy) || !mopen(nx, ny)) continue;
                int sx = mstart(cx) + (cx & 1), sy = mstart(cy) + (cy & 1), fx = sx + DX[d], fy = sy + DY[d];
                if (G.map[fy][fx].kind != K_FLOOR || !lane(fx, fy) || in_trial(fx, fy)) continue;
                int doorish = 0;
                for (int e = 0; e < 4; e++) doorish |= cell(fx + DX[e], fy + DY[e])->kind == K_DOOR;
                if (doorish) continue;
                cand[nc++] = (my * MG + mx) * 4 + d;
            }
        }
    for (int i = nc - 1; i > 0; i--) { int j = rnd(i + 1), t = cand[i]; cand[i] = cand[j]; cand[j] = t; }
    int placed = 0;
    for (int i = 0; i < nc && placed < nch; i++) {
        int b = cand[i] / 4, d = cand[i] % 4, mx = b % MG, my = b / MG, in = (d + 2) & 3;
        int cx = mx + DX[d], cy = my + DY[d], sx = mstart(cx) + (cx & 1), sy = mstart(cy) + (cy & 1);
        int clash = MM[my][mx].trunk;                              /* trunk marks a room already taken */
        for (int k = 0; k < G.nsecrets; k++) clash |= abs(G.secrets[k].sx - sx) + abs(G.secrets[k].sy - sy) < 8;
        if (clash) continue;
        /* how big: two by two, two side by side, or the one block, as the plan allows */
        int px = DY[in], py = DX[in], side = rnd(2) ? 1 : -1, bx[4] = { mx }, by[4] = { my }, nb = 1;
        float r = rndf();
        if (r < 0.15f + 0.4f * sk) {
            for (int sd = 0; sd < 2 && nb == 1; sd++, side = -side) {
                int ax = mx + 2 * px * side, ay = my + 2 * py * side, dx2 = mx + 2 * DX[in], dy2 = my + 2 * DY[in];
                if (secret_block_free(ax, ay) && secret_block_free(dx2, dy2) && secret_block_free(ax + 2 * DX[in], ay + 2 * DY[in])) {
                    bx[1] = ax; by[1] = ay; bx[2] = dx2; by[2] = dy2; bx[3] = ax + 2 * DX[in]; by[3] = ay + 2 * DY[in]; nb = 4;
                }
            }
        }
        if (nb == 1 && r < 0.65f)
            for (int sd = 0; sd < 2 && nb == 1; sd++, side = -side)
                if (secret_block_free(mx + 2 * px * side, my + 2 * py * side)) { bx[1] = mx + 2 * px * side; by[1] = my + 2 * py * side; nb = 2; }
        int x0 = MAPN, y0 = MAPN, x1 = -1, y1 = -1;
        for (int k = 0; k < nb; k++) {
            MM[by[k]][bx[k]].trunk = 1;
            x0 = imin(x0, mstart(bx[k])); y0 = imin(y0, mstart(by[k])); x1 = imax(x1, mstart(bx[k]) + 2); y1 = imax(y1, mstart(by[k]) + 2);
        }
        for (int y = y0; y <= y1; y++)
            for (int x = x0; x <= x1; x++) { carve(x, y, -2, TH_CORRIDOR); G.map[y][x].secret = 1; }
        G.map[sy][sx].secret = 2;
        Secret *sc = &G.secrets[G.nsecrets++];
        *sc = (Secret){ sx + DX[d], sy + DY[d], in, sx, sy, 0, 0, 0, nb == 4 ? 7 : 3, chain[placed++], -1, -1, x0, y0, x1, y1, rng_next(&G.rs) | 1u, 0 };
        if (sc->links >= 2 && !secret_door(sc)) { left += sc->links - 1; sc->links = 1; }   /* nowhere for a doorway: alone after all */
    }
    for (int k = placed; k < nch; k++) left += chain[k];
    for (int k = 0; k < G.nsecrets && left > 0; k++)               /* what found no room goes deeper into a chain */
        if (G.secrets[k].door_x >= 0) { G.secrets[k].links += left; left = 0; }
    for (int k = 0; k < G.nsecrets && left > 0; k++)               /* or makes one of a room alone */
        if (secret_door(&G.secrets[k])) { G.secrets[k].links += left; left = 0; }
    for (int k = 0; k < G.nsecrets; k++) G.nlinks += G.secrets[k].links;
#ifdef CRAWL_DEBUG
    fprintf(stderr, "SECRETS level %d: %d links in %d rooms:", G.level, G.nlinks, G.nsecrets);
    for (int k = 0; k < G.nsecrets; k++)
        fprintf(stderr, " [%dx%d chain %d door (%d,%d)]", G.secrets[k].x1 - G.secrets[k].x0 + 1, G.secrets[k].y1 - G.secrets[k].y0 + 1, G.secrets[k].links, G.secrets[k].door_x, G.secrets[k].door_y);
    fprintf(stderr, "\n");
#endif
}

/* the sliding wall: a block of stone at its cell plus how far it has come */
static void slide_box(float *bx, float *by) {
    const Secret *sc = &G.secrets[G.slide];
    *bx = sc->sx + DX[sc->d] * G.slide_t; *by = sc->sy + DY[sc->d] * G.slide_t;
}
static int slide_covers(int x, int y) {
    float bx, by;
    slide_box(&bx, &by);
    return bx < x + 1 - 0.02f && bx + 1 > x + 0.02f && by < y + 1 - 0.02f && by + 1 > y + 0.02f;
}

#define ARENA_PLAN 15                       /* plan rows from here on are a boss's (cells from BAND_Y0) */
#define ARENA_DOOR_MX 3                     /* and the way down to it: plan column 3, cell x 6 */
static void stamp_arena(void);
static int in_band(int y);
static float decay(void);
static int hz_patch(int sx, int sy, int reach, int8_t *cx, int8_t *cy, uint8_t *cd, int max);
static void swap_rooms(int a, int b) {                             /* rooms a and b change places in the lists */
    for (int y = 0; y < MG; y++)
        for (int x = 0; x < MG; x++) {
            if (MM[y][x].room == a) MM[y][x].room = (int8_t)b;
            else if (MM[y][x].room == b) MM[y][x].room = (int8_t)a;
        }
    for (int k = 0; k < 4; k++) { int t = mroom[a][k]; mroom[a][k] = mroom[b][k]; mroom[b][k] = t; }
    Room t = G.rooms[a]; G.rooms[a] = G.rooms[b]; G.rooms[b] = t;
}

