/* A part of scene_crawl.c (docs/PLAN-tidy.md): it is #included there in its place and never compiled on its own.
 * A drawn run's walls by face: materials by the stretch, blends, what stands on the wall (lay_faces). */

/* ---- a drawn run's walls by face (docs/PLAN-b10.md) ----
 * Each straight stretch of wall, on one side, between corners and doorways, takes its room type's
 * materials; where two meet along it, a blend sits between them; what stands on a wall is put along it by
 * rule. A face's texture runs left to right as you look at it: along +y, -x, -y, +x for faces looking
 * east, south, west and north (the raycaster's own flip), and the stretches are walked that way. */
static int faceable(int x, int y) {                                /* an ordinary wall (not an exit, a gate, a doorway, a sign) */
    const Cell *c = &G.map[y][x];
    if (c->kind != K_WALL) return 0;
    int t = c->tex;
    return t == T_STONE || t == T_RUBBLE || t == T_SKULLS || t == T_BRICK || t == T_MOSS || t == T_BANNER || t == T_TORCH0 || t == T_NICHE;
}
static int plain_tex(int t) { return t <= T_BANNER || t == T_TORCH0; }   /* what a face may stand for */
static int face_open(int x, int y, int d) {
    int nx = x + DX[d], ny = y + DY[d];
    if (nx < 0 || ny < 0 || nx >= MAPN || ny >= MAPN) return 0;
    int k = G.map[ny][nx].kind;
    return k == K_FLOOR || k == K_PILLAR || k == K_PROP;
}
static int cell_rtype(int x, int y) {                              /* the room type a floor cell lies in */
    const Cell *n = cell(x, y);
    static const uint8_t CLOSET[4] = { RT_UNDER, RT_NAVE, RT_STOPE, RT_CINDER }, ARENA[4] = { RT_KNIGHTS, RT_SPINNERS, RT_CHAPEL, RT_THRONE };
    if (n->secret && !G.shop) return CLOSET[G.look];                 /* a secret's closet: rough work */
    if (n->region == ARENA_REGION) return ARENA[G.look];              /* the knight's lists, the spider's gallery, the Deacon's chapel, the Wraith's throne */
    if (n->theme != TH_CORRIDOR)
        for (int r = 0; r < G.nrooms; r++) if (G.rooms[r].region == n->region) return G.rooms[r].rtype;
    return RT_HALL;
}
static int face_key(int x, int y, int d) {                         /* which place a face looks into: a room, the halls, a closet, the arena */
    const Cell *n = cell(x + DX[d], y + DY[d]);
    if (n->secret) return -2;
    if (n->region == ARENA_REGION) return -3;
    return n->theme == TH_CORRIDOR ? -1 : n->region;
}
static int blend_of(int a, int b, int *flip) {                     /* the blend from a (left) to b (right), and whether to mirror it */
    const RunLook *L = &crawl_looks[G.look];
    for (int k = 0; k < NBLEND; k++) {
        if (L->blend[k][0] == a && L->blend[k][1] == b) { *flip = 0; return k; }
        if (L->blend[k][0] == b && L->blend[k][1] == a) { *flip = 1; return k; }
    }
    return -1;
}
static int on_water(int x, int y) { return G.faced && G.look == 1 && cell(x, y)->flo == T_FLOORS + FL2_WATER; }   /* the Catacombs' standing water */
static uint8_t room_shrine[12];                                     /* an undercroft's shrine, once */
static int wall_lights;                                             /* the saints' candles and the geodes' glow lit this level */
static float mat_field(int x, int y, uint32_t seed) {              /* smooth over six cells: where the halls are dressed, patched or raw */
    float fx = x / 6.0f, fy = y / 6.0f;
    int x0 = (int)floorf(fx), y0 = (int)floorf(fy);
    float tx = smoothstepf(0.0f, 1.0f, fx - x0), ty = smoothstepf(0.0f, 1.0f, fy - y0);
    #define HV(i, j) ((float)(hash2((x0 + (i)) * 7 + (int)seed, (y0 + (j)) * 13 + (int)(seed >> 3)) & 0xFFFF) / 65535.0f)
    float v = lerpf(lerpf(HV(0, 0), HV(1, 0), tx), lerpf(HV(0, 1), HV(1, 1), tx), ty);
    #undef HV
    return v;
}
static void lay_stretch(int d, const int *xs, const int *ys, int n, int rt) {
    int8_t mat[MAPN], feat[MAPN], bl[MAPN], blf[MAPN];
    uint32_t h0 = hash2(xs[0] * 7 + d, ys[0] * 13 + n * 5);
    #define HR(i, k) (hash2(xs[i] * 31 + (k), ys[i] * 17 + d) % 100u)
    for (int i = 0; i < n; i++) { feat[i] = -1; bl[i] = -1; blf[i] = 0; }
    int look = G.look == 1, step = crawl_step(G.level);
    switch (rt) {
    case RT_HALL: case RT_UNDER: case RT_PLOT: case RT_NAVE: case RT_CISTERN: case RT_STOPE: case RT_VENT: {   /* by the field, so whole stretches of the crypt agree */
        uint32_t fs = (uint32_t)G.level * 7919u + (rt == RT_HALL ? 0u : 4111u);
        for (int i = 0; i < n; i++) {
            float v = mat_field(xs[i] + DX[d], ys[i] + DY[d], fs);
            mat[i] = (int8_t)(rt == RT_UNDER ? (v < 0.5f ? M1_RUBBLE : M1_BEDROCK)
                            : rt == RT_PLOT ? (v < 0.55f ? M1_ASHLAR : M1_RUBBLE)
                            : rt == RT_NAVE ? (v < 0.42f ? M2_SILT : v < 0.72f ? M2_SLATE : M2_LINING)
                            : rt == RT_CISTERN ? (v < 0.55f ? M2_FLOW : M2_LINING)
                            : rt == RT_STOPE ? (v < 0.7f ? M3_FACE : M3_GRANITE)
                            : rt == RT_VENT ? (v < 0.72f ? M4_BASALT : M4_ASH)
                            : G.look == 3 ? (v < 0.42f ? M4_ASH : v < 0.75f ? M4_OBSIDIAN : M4_BASALT)   /* the Deep's halls: ash, glass, columns */
                            : G.look == 2 ? (v < 0.5f ? M3_GRANITE : v < 0.78f ? M3_FACE : M3_TIMBER)   /* the Hollows' halls: cave, cut, shored */
                            : look ? (v < 0.58f ? M2_SLATE : v < 0.8f ? M2_LINING : M2_SILT)   /* the Catacombs' halls: the slate as cut, lined, silted */
                            : (v < 0.48f ? M1_ASHLAR : v < 0.72f ? M1_RUBBLE : M1_BEDROCK));
        }
        for (int i = 1; i + 1 < n; i++)                             /* no material for a single cell */
            if (mat[i] != mat[i - 1] && mat[i] != mat[i + 1]) mat[i] = mat[i - 1];
        if (n >= 2 && mat[0] != mat[1]) mat[0] = mat[1];
        if (n >= 2 && mat[n - 1] != mat[n - 2]) mat[n - 1] = mat[n - 2];
        break;
    }
    case RT_OSSUARY:
        for (int i = 0; i < n; i++) mat[i] = M1_BONES;
        if (n >= 5) mat[0] = mat[n - 1] = M1_ASHLAR;                /* dressed stone at the ends */
        break;
    case RT_CHAPTER:
        for (int i = 0; i < n; i++) mat[i] = M1_PLASTER;
        break;
    case RT_SAINTS: case RT_CANAL:
        for (int i = 0; i < n; i++) mat[i] = M2_LINING;
        break;
    case RT_SPINNERS:
        for (int i = 0; i < n; i++) mat[i] = M2_BONES;
        if (n >= 5) mat[0] = mat[n - 1] = M2_SILT;                  /* plain silt at the ends */
        break;
    case RT_GALLERY: for (int i = 0; i < n; i++) mat[i] = M3_TIMBER; break;
    case RT_FOSSIL: for (int i = 0; i < n; i++) mat[i] = M3_GRANITE; break;
    case RT_CHAPEL: for (int i = 0; i < n; i++) mat[i] = M3_BASALT; break;
    case RT_CHARNEL:
        for (int i = 0; i < n; i++) mat[i] = M3_FILL;
        if (n >= 5) mat[0] = mat[n - 1] = M3_GRANITE;               /* the pit's rock at the ends */
        break;
    case RT_GEODE: for (int i = 0; i < n; i++) mat[i] = M4_CRYSTAL; break;
    case RT_GLASS: for (int i = 0; i < n; i++) mat[i] = M4_OBSIDIAN; break;
    case RT_CINDER: for (int i = 0; i < n; i++) mat[i] = M4_ASH; break;
    case RT_THRONE:
        for (int i = 0; i < n; i++) mat[i] = M4_BONES;
        if (n >= 5) mat[0] = mat[n - 1] = M4_OBSIDIAN;              /* plain glass at the ends */
        break;
    default:
        for (int i = 0; i < n; i++) mat[i] = M1_ASHLAR;
        break;
    }
    for (int i = 1; i < n; i++) {                                   /* a blend where one material gives way to the next */
        if (mat[i] == mat[i - 1]) continue;
        int f, k = blend_of(mat[i - 1], mat[i], &f);
        if (k < 0) { mat[i] = mat[i - 1]; continue; }
        int at = i - 1 >= 1 && mat[i - 2] == mat[i - 1] ? i - 1 : (i + 1 < n && mat[i + 1] == mat[i] ? i : -1);
        if (at < 0 || bl[at] >= 0) continue;
        bl[at] = (int8_t)k; blf[at] = (int8_t)f;
    }
    int mid = n / 2, lo = n > 2 ? 1 : 0, hi = n > 2 ? n - 2 : n - 1;
    for (int i = lo; i <= hi; i++) {                                /* what stands on the walls */
        if (bl[i] >= 0) continue;
        switch (rt) {
        case RT_BURIAL: if (n >= 2 && (i - lo) % 4 != 3) feat[i] = F1_NICHE; break;
        case RT_KNIGHTS: if (n >= 5 && i == mid) feat[i] = F1_BANNER; else if (((i - lo) & 1) == 0) feat[i] = F1_SLAB; break;
        case RT_CHAPTER: case RT_MERCHANT:
            if (n >= 3 && (i - mid) % 4 == 0) feat[i] = F1_BANNER;
            else if (rt == RT_CHAPTER && n >= 4 && i == lo + (int)(h0 % (uint32_t)(hi - lo + 1)) && (i - mid) % 4 != 0) feat[i] = F1_TABLET;
            break;
        case RT_HALL:
            if (G.look == 3) { if (mat[i] == M4_OBSIDIAN ? HR(i, 3) < 3 : HR(i, 3) < 4) feat[i] = mat[i] == M4_OBSIDIAN ? F4_FUSED : F4_GOLD; }
            else if (G.look == 2) { if (n >= 4 && mat[i] != M3_TIMBER && (i - lo) % 6 == 3) feat[i] = F3_FRAME; }
            else if (look) { if (n >= 4 && mat[i] != M2_SLATE && HR(i, 3) < 5) feat[i] = F2_HANDS; }
            else if (n >= 6 && mat[i] == M1_ASHLAR && HR(i, 3) < 6) feat[i] = F1_TABLET;
            break;
        case RT_SAINTS: feat[i] = n >= 3 && (i - mid) % 3 == 0 ? F2_SAINT : ((i - lo) % 4 != 3 ? F2_NICHE : -1); break;
        case RT_CISTERN: if (mat[i] == M2_LINING && (i - lo) % 4 == 1) feat[i] = F2_CULVERT; break;
        case RT_CANAL: if ((i - lo) % 4 == 1) feat[i] = F2_CULVERT; break;
        case RT_NAVE: if (HR(i, 5) < 16) feat[i] = F2_HANDS; break;
        case RT_SPINNERS: if (HR(i, 6) < 40) feat[i] = F2_WEB; break;
        case RT_GALLERY: if ((i - lo) % 4 == 2) feat[i] = F3_FRAME; break;
        case RT_STOPE:
            if (n >= 4 && i == mid && mat[i] == M3_FACE) feat[i] = F3_CHUTE;
            else if (mat[i] == M3_FACE && HR(i, 4) < 24) feat[i] = F3_ORE;
            break;
        case RT_FOSSIL: if (HR(i, 5) < 30) feat[i] = F3_FOSSIL; break;
        case RT_CHAPEL: if ((i - mid) % 3 == 0) feat[i] = F3_MARKS; break;
        case RT_GEODE: if (HR(i, 4) < 28) feat[i] = F4_GEODE; break;
        case RT_GLASS: if (HR(i, 4) < 18) feat[i] = F4_SEAM; else if (HR(i, 5) < 10) feat[i] = F4_FUSED; break;
        case RT_CINDER: if (HR(i, 4) < 14) feat[i] = F4_FUSED; break;
        case RT_VENT: if (HR(i, 4) < 20) feat[i] = F4_GOLD; break;
        case RT_THRONE: if (n >= 3 && i == mid) feat[i] = F4_FRIEZE; else if (HR(i, 5) < 10) feat[i] = F4_FUSED; break;
        default: break;
        }
        if (look && feat[i] < 0 && step >= 5 && (int)HR(i, 7) < 3 * (step - 4)) feat[i] = F2_WEB;   /* then webs, more by the run's end */
    }
    if (rt == RT_UNDER && n >= 3) {                                 /* the undercroft's shrine, once a room */
        int r = cell(xs[mid] + DX[d], ys[mid] + DY[d])->region;
        if (r >= 0 && r < 12 && !room_shrine[r] && bl[mid] < 0) { room_shrine[r] = 1; feat[mid] = F1_SHRINE; }
    }
    for (int i = 0; i < n; i++) {
        Cell *c = &G.map[ys[i]][xs[i]];
        int v = mat[i] == M1_BONES ? (hash2(xs[i] * 3 + d, ys[i] * 5) & 3) == 0 : (int)(hash2(xs[i] * 3 + d, ys[i] * 5) & 1);
        c->face[d] = (uint8_t)(bl[i] >= 0 ? T_BLEND + bl[i] : T_MATV(mat[i], v));
        if (bl[i] >= 0 && blf[i]) c->fflip |= (uint8_t)(1 << d); else c->fflip &= (uint8_t)~(1 << d);
        c->fover[d] = 0;
        if (c->secret == 2) continue;                               /* a secret's cracked wall shows only its stone */
        if (c->tex == T_TORCH0 && c->ftorch == d + 1) c->fover[d] = (uint8_t)(T_FEAT + F1_TORCH);
        else if (c->tex == T_NICHE && in_band(ys[i])) {             /* an arena's burrow: the adds come out of it */
            static const uint8_t BURROW[4] = { F1_NICHE, F2_NICHE, F3_CHUTE, F4_FUSED };
            c->fover[d] = (uint8_t)(T_FEAT + BURROW[G.look]);
        }
        else if (feat[i] >= 0) c->fover[d] = (uint8_t)(T_FEAT + feat[i]);
        if (((look && feat[i] == F2_SAINT) || (G.look == 3 && feat[i] == F4_GEODE)) && wall_lights < 10 && G.nlights < MAXLIGHTS - 8 && !in_band(ys[i])) {
            light_static(xs[i] + 0.5f + DX[d] * 0.62f, ys[i] + 0.5f + DY[d] * 0.62f, 0.55f, 2.4f);   /* the candles at a saint's feet; a geode's glow */
            wall_lights++;
        }
    }
    #undef HR
}
static void lay_faces(void) {
    static int xs[MAPN], ys[MAPN];
    memset(room_shrine, 0, sizeof room_shrine); wall_lights = 0;
    for (int y = 0; y < MAPN; y++)                                  /* where a torch stands on a wall: the face by its light */
        for (int x = 0; x < MAPN; x++) {
            Cell *c = &G.map[y][x];
            if (c->kind != K_WALL || c->tex != T_TORCH0 || c->ftorch) continue;
            float best = 9.0f;
            for (int d = 0; d < 4; d++) {
                if (!face_open(x, y, d)) continue;
                float fx = x + 0.5f + DX[d] * 0.5f, fy = y + 0.5f + DY[d] * 0.5f;
                for (int l = 0; l < G.nlights; l++) {
                    float dd = hypotf(G.lights[l].x - fx, G.lights[l].y - fy);
                    if (dd < best) { best = dd; c->ftorch = (uint8_t)(d + 1); }
                }
            }
        }
    for (int d = 0; d < 4; d++)
        for (int line = 0; line < MAPN; line++) {
            int n = 0, key = 0;
            for (int i = 0; i <= MAPN; i++) {
                int x = 0, y = 0, ok = 0;
                if (i < MAPN) {
                    x = d == 0 || d == 2 ? line : (d == 3 ? i : MAPN - 1 - i);
                    y = d == 1 || d == 3 ? line : (d == 0 ? i : MAPN - 1 - i);
                    ok = faceable(x, y) && face_open(x, y, d);
                }
                int k = ok ? face_key(x, y, d) : 0;
                if (n && (!ok || k != key)) { lay_stretch(d, xs, ys, n, cell_rtype(xs[0] + DX[d], ys[0] + DY[d])); n = 0; }
                if (ok) { if (!n) key = k; xs[n] = x; ys[n] = y; n++; }
            }
        }
    for (int y = 0; y < MAPN; y++)                                  /* floors and ceilings by the room */
        for (int x = 0; x < MAPN; x++) {
            Cell *c = &G.map[y][x];
            if (c->kind == K_WALL && c->secret != 2) continue;      /* a secret's wall: floor once it has slid */
            int rt = c->kind == K_DOOR ? RT_HALL : cell_rtype(x, y), fl = RTYPES[rt].floor;
            if (rt == RT_CISTERN && c->kind != K_DOOR) {                /* the water in the middle, a walk of flags round it */
                int open = 1;
                for (int j = -1; j <= 1 && open; j++) for (int i = -1; i <= 1; i++) if (cell(x + i, y + j)->kind == K_WALL || cell(x + i, y + j)->kind == K_DOOR) { open = 0; break; }
                if (open) fl = FL2_WATER;
            }
            int turn = -1;
            if ((rt == RT_CANAL || rt == RT_GALLERY) && c->kind != K_DOOR)   /* the canal's water down its length; the gallery's rails */
                for (int r = 0; r < G.nrooms; r++) {
                    const Room *R = &G.rooms[r];
                    if (R->region != c->region) continue;
                    int along = R->w >= R->h, off = along ? y - (R->y + R->h / 2) : x - (R->x + R->w / 2), half = (along ? R->h : R->w) >= 7;
                    if (rt == RT_CANAL && abs(off) <= half) fl = FL2_WATER;
                    if (rt == RT_GALLERY && !off) { fl = FL3_RAILS; turn = 16 | (along ? 4 : 0) << 5; }   /* drawn along y: turned for a room along x */
                    break;
                }
            if (fl == FL1_FLAGS && (float)(hash2(x * 5 + 3, y * 7 + 1) % 1000u) < 380.0f * decay()) fl = FL1_BROKEN;   /* either run's floor 0 goes to its floor 4 with the rot */
            c->flo = (uint8_t)(T_FLOORS + fl); c->cei = (uint8_t)(T_CEILS + RTYPES[rt].ceil);
            if (turn < 0) turn = fl == FL1_CHAPTER ? 16 : (int)(hash2(x, y) & 7) << 5;   /* floor 1 lies as laid (the Hollows' rails along their room) */
            c->fflip = (uint8_t)((c->fflip & 15) | turn);   /* how it is turned, kept */
        }
    for (int i = 0; i < G.nsecrets; i++) {                          /* the sliding wall wears the face it shows the hall */
        Secret *sc = &G.secrets[i];
        Cell *w = &G.map[sc->sy][sc->sx];
        for (int d = 0; d < 4; d++)
            if (w->face[d] && face_open(sc->sx, sc->sy, d) && !cell(sc->sx + DX[d], sc->sy + DY[d])->secret) { sc->tex = w->face[d]; break; }
    }
#ifdef CRAWL_DEBUG
    fprintf(stderr, "ROOMS level %d:", G.level);
    for (int r = 0; r < G.nrooms; r++) fprintf(stderr, " %s (%d,%d %dx%d)", RTYPES[G.rooms[r].rtype].name, G.rooms[r].x, G.rooms[r].y, G.rooms[r].w, G.rooms[r].h);
    int wet = 0;
    for (int y = 0; y < MAPN; y++) for (int x = 0; x < MAPN; x++) wet += on_water(x, y);
    fprintf(stderr, " | water %d cells, lights in the walls %d\n", wet, wall_lights);
#endif
}

