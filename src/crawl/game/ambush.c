/* A part of scene_crawl.c (docs/PLAN-tidy.md): it is #included there in its place and never compiled on its own.
 * Lures and ambushes: kill rooms, teeth, bait, the spring. */

/* ---- Lures and ambushes (docs/PLAN-ai.md, section 12; step 6). A kill room (one or two ways in, off the way from the
 * start to the exit and the keys, twelve steps from both), its teeth waiting beside the inside of its doorway or as
 * their kind hides (under the earth, in the water, on the ceiling, on plinths, in the dark), and a bait: a creature
 * in a hall six to twelve steps off that shows itself and backs away to the room's far side at your pace, looking
 * back; or treasure in plain view of the door (section 30); or now and then the level's gilded, running there when
 * hurt (L2). The teeth spring when you are two cells in, see one of them, strike one, or kill the bait in the room.
 * Refuse three times and the set piece comes hunting; kill the bait outside and the teeth come out to search. Each
 * carries its run's tell by the door, a run's first two, and the spectres drift to it now and then (L3). From level 7,
 * one at most a level, two now and then from run III, never on a boss's level or in a pocket (L4). ---- */
enum { TEETH_SHIELDS, TEETH_BURIED, TEETH_DROWNED, TEETH_KNIGHTS, TEETH_SPIDERS, TEETH_GHOULS, TEETH_GARGS, TEETH_SHADES };
#ifdef CRAWL_DEBUG
static const char *TEETH_NAME[] = { "shields", "buried", "drowned", "knights", "spiders", "ghouls", "gargoyles", "shades" };
#endif
static int amb_cell_ok(int x, int y) { return x > 0 && y > 0 && x < MAPN - 1 && y < MAPN - 1 && G.map[y][x].kind == K_FLOOR && !G.map[y][x].secret && !occupied(x, y, NULL); }
static void amb_bfs(int sx, int sy, int16_t f[MAPN][MAPN], int br) {   /* steps over floor and doors, room br shut */
    static int q[MAPN * MAPN];
    for (int y = 0; y < MAPN; y++) for (int x = 0; x < MAPN; x++) f[y][x] = -1;
    if (sx < 0) return;
    int qh = 0, qt = 0;
    f[sy][sx] = 0; q[qt++] = sy * MAPN + sx;
    while (qh < qt) {
        int x = q[qh] % MAPN, y = q[qh] / MAPN; qh++;
        for (int d = 0; d < 4; d++) {
            int nx = x + DX[d], ny = y + DY[d];
            if (nx < 1 || ny < 1 || nx >= MAPN - 1 || ny >= MAPN - 1 || f[ny][nx] >= 0) continue;
            int k = G.map[ny][nx].kind;
            if (k != K_FLOOR && k != K_DOOR) continue;
            if (br >= 0 && in_room(&G.rooms[br], nx, ny)) continue;
            f[ny][nx] = (int16_t)(f[y][x] + 1); q[qt++] = ny * MAPN + nx;
        }
    }
}
static int amb_light_near(int x, int y) {                          /* the nearest light within three cells, 1 + its index */
    int best = 0; float bd = 3.0f;
    for (int l = 0; l < G.nlights; l++) { float d = hypotf(G.lights[l].x - x - 0.5f, G.lights[l].y - y - 0.5f); if (d < bd) { bd = d; best = l + 1; } }
    return best;
}
static int amb_teeth(int r) {
    int set = crawl_set(G.level), lv = G.level > DESCENT_LEN ? 99 : G.level, rt = G.rooms[r].rtype;
#ifdef CRAWL_DEBUG
    if (getenv("CRAWL_AMBT")) return clampi(atoi(getenv("CRAWL_AMBT")), 0, TEETH_SHADES);
#endif
    switch (set) {
    case 0: return rt == RT_BURIAL && lv >= KINDS[MOB_SHAMBLER].debut ? TEETH_BURIED : TEETH_SHIELDS;
    case 1: return (rt == RT_CISTERN || rt == RT_CANAL) ? TEETH_DROWNED : (lv > KINDS[MOB_KNIGHT].debut && rnd(2)) ? TEETH_KNIGHTS : TEETH_SHIELDS;
    case 2: return rt == RT_CHARNEL ? TEETH_GHOULS : (rt == RT_GALLERY || rt == RT_STOPE || rnd(2)) ? TEETH_SPIDERS : TEETH_GHOULS;
    default: return rt == RT_GLASS ? TEETH_GARGS : TEETH_SHADES;
    }
}
/* one ambush in room r, its way in the opening nearest the start; 1 if made */
static int amb_make(int r, const int16_t ds[MAPN][MAPN]) {
    const Room *R = &G.rooms[r];
    int ox = -1, oy = -1, bd = 1 << 30;
    for (int y = R->y - 1; y <= R->y + R->h; y++)                 /* the way in: of the ring's open cells, the nearest the start */
        for (int x = R->x - 1; x <= R->x + R->w; x++) {
            if (in_room(R, x, y) || x < 1 || y < 1 || x >= MAPN - 1 || y >= MAPN - 1) continue;
            int k = G.map[y][x].kind;
            if ((k != K_FLOOR && k != K_DOOR) || ds[y][x] < 0 || ds[y][x] >= bd) continue;
            int in = 0;
            for (int d = 0; d < 4; d++) in |= in_room(R, x + DX[d], y + DY[d]);
            if (!in) continue;
            bd = ds[y][x]; ox = x; oy = y;
        }
    if (ox < 0) return 0;
    int ix = ox, iy = oy, dx = 0, dy = 0;
    for (int d = 0; d < 4; d++) if (in_room(R, ox + DX[d], oy + DY[d])) { ix = ox + DX[d]; iy = oy + DY[d]; dx = DX[d]; dy = DY[d]; break; }
    int fx = ix, fy = iy; float fd = -1.0f;                         /* the far side */
    for (int y = R->y; y < R->y + R->h; y++)
        for (int x = R->x; x < R->x + R->w; x++)
            if (G.map[y][x].kind == K_FLOOR && hypotf((float)(x - ix), (float)(y - iy)) > fd) { fd = hypotf((float)(x - ix), (float)(y - iy)); fx = x; fy = y; }
    int k = G.namb, teeth = amb_teeth(r), set = crawl_set(G.level), lv = G.level > DESCENT_LEN ? 99 : G.level;
    Ambush *a = &G.amb[k];
    memset(a, 0, sizeof *a);
    a->room = r; a->ox = ox; a->oy = oy; a->ix = ix; a->iy = iy; a->fx = fx; a->fy = fy; a->teeth = teeth;
    int pk = pack_new(teeth == TEETH_SPIDERS ? PACK_SPIDERS : teeth == TEETH_GHOULS ? PACK_SCAV : teeth == TEETH_BURIED || teeth == TEETH_DROWNED ? PACK_WAKE : PACK_FILE);
    int n = teeth == TEETH_SHIELDS ? 2 + (lv >= 9) : teeth == TEETH_KNIGHTS ? 2 : teeth == TEETH_GARGS || teeth == TEETH_SHADES ? 2
          : teeth == TEETH_SPIDERS ? 2 + (set >= 3) : 3;
    n = imax(2, imin(n, set_cap - 1));                             /* the budget's (step 8): its bait counts too */
    int px = -dy, py = dx, made = 0;                                /* along the wall, either side of the way in */
    for (int ring = 0; ring < 12 && made < n; ring++) {
        int along = ring / 4, side = (ring % 4 < 2 ? 1 : 2) * ((ring & 1) ? -1 : 1);
        int x = ix + dx * along + px * side, y = iy + dy * along + py * side;
        if (teeth == TEETH_BURIED || teeth == TEETH_DROWNED) { x = ix + dx * (1 + along) + px * (side / 2 + (ring & 1)); y = iy + dy * (1 + along) + py * (side / 2 + (ring & 1)); }
        if (!in_room(R, x, y) || !amb_cell_ok(x, y) || (x == ix && y == iy)) continue;
        Mob *m = NULL;
        switch (teeth) {
        case TEETH_BURIED: m = riser_make(MOB_SHAMBLER, x, y, -1); break;
        case TEETH_DROWNED: { int di = G.ndecos; add_deco(x + 0.5f, y + 0.5f, SP_PUDDLE_A); if (G.ndecos > di) m = riser_make(MOB_DROWNED, x, y, di); break; }
        case TEETH_KNIGHTS: m = spawn_mob(x, y, made == 0 ? MOB_KNIGHT : MOB_SHIELD); break;
        case TEETH_SPIDERS: m = spawn_mob(x, y, MOB_HUNTER); break;
        case TEETH_GHOULS: m = spawn_mob(x, y, MOB_GHOUL); break;
        case TEETH_GARGS: m = spawn_mob(x, y, MOB_GARGOYLE); if (m) G.map[y][x].kind = K_PROP; break;
        case TEETH_SHADES: m = spawn_mob(x, y, MOB_SHADE); break;
        default: m = spawn_mob(x, y, made == 2 ? MOB_SKEL : MOB_SHIELD); break;
        }
        if (!m) break;
        m->amb = k + 1; m->home = G.map[y][x].region; m->role = RL_POST; m->hx = x; m->hy = y; m->route = -1;
        m->pf = m->face = atan2f(iy + 0.5f - m->y, ix + 0.5f - m->x);   /* turned to the doorway */
        if (pk >= 0) pack_join(pk, m);
        made++;
    }
    if (!made) return 0;
    /* the bait: treasure in plain view of the door one time in three; else one of the run's own, in a hall six to twelve
       steps off */
    int bx = -1, by = -1;
    if (rnd(3)) {
        static int16_t fo[MAPN][MAPN];
        amb_bfs(ox, oy, fo, r);
        for (int tries = 0; tries < 400 && bx < 0; tries++) {
            int x = 1 + rnd(MAPN - 2), y = 1 + rnd(MAPN - 2);
            if (fo[y][x] < 6 || fo[y][x] > 12 || !apass(x, y) || !amb_cell_ok(x, y) || room_of_region(G.map[y][x].region) >= 0 || in_band(y)
                || ds[y][x] < 10) continue;
            bx = x; by = y;
        }
    }
    if (bx >= 0) {
        int type = set == 0 ? MOB_SKEL : set == 1 ? MOB_HOUND : set == 2 ? MOB_BAT : (lv >= KINDS[MOB_IMP].debut ? MOB_IMP : MOB_BAT);
        Mob *b = spawn_mob(bx, by, type);
        if (b) {
            b->amb = k + 1; b->bait_st = B_WAIT; b->role = RL_POST; b->hx = bx; b->hy = by; b->route = -1;
            b->pf = b->face = atan2f(oy + 0.5f - b->y, ox + 0.5f - b->x) + PI_F;   /* its back to the room it means to take you to */
            a->bait = (int)(b - G.mobs) + 1;
        }
    }
    if (!a->bait) {                                                /* a glint: a hoard or a potion, two or three cells in */
        int tx = ix + dx * 2, ty = iy + dy * 2;
        if (!in_room(R, tx, ty) || !amb_cell_ok(tx, ty)) { tx = ix + dx; ty = iy + dy; }
        int type = rnd(3) == 0 ? IT_POTION : IT_CHEST;
        Item *it = in_room(R, tx, ty) && amb_cell_ok(tx, ty) ? add_item(tx, ty, type) : NULL;
        if (it) { it->secret = 1; if (type < IT_KEY) { G.items_total--; G.value_total -= treasure_value(type); } }
    }
    /* its tell, by the door outside; a run's first two (the second a torch by the door snuffed) */
    static const int TELL[4] = { SP_SIGN_RAKE, SP_SIGN_DRAG, SP_SIGN_SCREE, SP_SIGN_CINDER };   /* 1.0.0b18: each run's own */
    add_deco(ox + 0.5f + px * 0.3f, oy + 0.5f + py * 0.3f, TELL[clampi(set, 0, 3)]);
    int first = !(RUN.amb_runs >> clampi(crawl_run(G.level) % 32, 0, 31) & 1);
    if (first && !(a->snuff = amb_light_near(ox, oy))) add_deco(ox + 0.5f - px * 0.3f, oy + 0.5f - py * 0.3f, TELL[clampi(set, 0, 3)]);   /* or a second sign */
    else if (a->snuff && G.ndecos < MAXDECO) {                     /* the torch snuffed: eyes in the dark inside, till it springs (drawn only) */
        int ex = ix + (ix - ox), ey = iy + (iy - oy);
        if (!in_room(R, ex, ey) || G.map[ey][ex].kind != K_FLOOR) { ex = ix; ey = iy; }
        a->eyes = G.ndecos + 1;
        G.decos[G.ndecos++] = (Deco){ ex + 0.5f, ey + 0.5f, 0.0f, SP_SIGN_EYES, 1, 0 };
    }
    G.namb++;
#ifdef CRAWL_DEBUG
    fprintf(stderr, "AMBUSH level %d: room %d (type %d) way in (%d,%d) far (%d,%d), %d %s, bait %s, %s\n", G.level, r, R->rtype, ox, oy, fx, fy,
            made, TEETH_NAME[teeth], a->bait ? KINDS[G.mobs[a->bait - 1].type].name : "treasure", a->snuff ? "a torch snuffed" : first ? "two signs" : "one sign");
#endif
    return 1;
}
static int place_ambushes(void) {
    G.namb = 0;
    if (!mind_on() || PK.depth || G.boss >= 0) return 0;
    int lv = G.level > DESCENT_LEN ? 99 : G.level, set = crawl_set(G.level), want = 0;
    float chance = lv == 7 ? 1.0f : set <= 1 ? 0.5f : 0.67f;
#ifdef CRAWL_DEBUG
    if (getenv("CRAWL_AMBUSH")) { lv = 99; chance = 1.0f; }
#endif
    if (lv < 7 || rndf() >= chance) return 0;
    want = 1 + (set >= 2 && rnd(4) == 0);
    static int16_t ds[MAPN][MAPN], de[MAPN][MAPN], dk[MAPN][MAPN];
    amb_bfs(G.cx, G.cy, ds, -1);
    amb_bfs(G.exit_fx, G.exit_fy, de, -1);
    int order[64], no = 0;
    for (int r = 1; r < G.nrooms && no < 64; r++) order[no++] = r;
    for (int i = no - 1; i > 0; i--) { int j = rnd(i + 1), t = order[i]; order[i] = order[j]; order[j] = t; }
    int why[6] = { 0 };
    for (int oi = 0; oi < no && G.namb < want; oi++) {
        int r = order[oi];
        const Room *R = &G.rooms[r];
        if (R->w * R->h < 6 || vault_of_room(R->x, R->y) >= 0 || r == G.pz_room || r == G.tri_room || in_band(R->y) || in_band(R->y + R->h - 1)) { why[0]++; continue; }
        int bad = 0, opens = 0;
        for (int i = 0; i < G.namb; i++) bad |= G.amb[i].room == r;
        for (int y = R->y; y < R->y + R->h && !bad; y++)          /* twelve steps from the start and the exit, and nothing of note in it */
            for (int x = R->x; x < R->x + R->w && !bad; x++) {
                if ((ds[y][x] >= 0 && ds[y][x] < 10) || (de[y][x] >= 0 && de[y][x] < 8)) bad = 1;   /* clear of the start and the exit */
                if ((x == G.exit_fx && y == G.exit_fy) || (x == G.exit_wx && y == G.exit_wy) || (x == G.altar_x && y == G.altar_y)) bad = 1;
                for (int i = 0; i < G.nrest; i++) if (x == G.rest_fx[i] && y == G.rest_fy[i]) bad = 1;
            }
        for (int i = 0; i < G.nitems && !bad; i++) if (G.items[i].alive && G.items[i].type == IT_KEY && in_room(R, (int)G.items[i].x, (int)G.items[i].y)) bad = 1;
        if (bad) { why[1]++; continue; }
        for (int y = R->y - 1; y <= R->y + R->h; y++)             /* one or two ways in */
            for (int x = R->x - 1; x <= R->x + R->w; x++) {
                if (in_room(R, x, y) || x < 1 || y < 1 || x >= MAPN - 1 || y >= MAPN - 1) continue;
                int k = G.map[y][x].kind, in = 0;
                if (k != K_FLOOR && k != K_DOOR) continue;
                for (int d = 0; d < 4; d++) in |= in_room(R, x + DX[d], y + DY[d]);
                if (!in) continue;
                int nb = 0;                                        /* an opening is a run of open cells: count its first */
                for (int d = 0; d < 4; d++) { int k2 = G.map[y + DY[d]][x + DX[d]].kind; nb += !in_room(R, x + DX[d], y + DY[d]) && (k2 == K_FLOOR || k2 == K_DOOR) && (DX[d] < 0 || DY[d] < 0); }
                if (!nb) opens++;
            }
        if (opens < 1 || opens > 2) { why[2]++; continue; }
        amb_bfs(G.cx, G.cy, dk, r);                                /* rule 3: nothing needed lies past it */
        if (G.exit_fx >= 0 && dk[G.exit_fy][G.exit_fx] < 0) { why[3]++; continue; }
        for (int i = 0; i < G.nitems && !bad; i++) if (G.items[i].alive && G.items[i].type == IT_KEY && dk[(int)G.items[i].y][(int)G.items[i].x] < 0) bad = 1;
        if (bad) { why[4]++; continue; }
        if (!amb_make(r, ds)) why[5]++;
    }
#ifdef CRAWL_DEBUG
    if (G.namb < want) fprintf(stderr, "AMBUSH level %d: none of %d rooms (small/vault %d, near or holding %d, ways in %d, the exit past it %d, a key %d, no teeth %d)\n",
                               G.level, no, why[0], why[1], why[2], why[3], why[4], why[5]);
#endif
    return G.namb > 0;
}
/* the level's gilded creature, one time in two, runs to a kill room when it is hurt (L2) */
static void amb_gild(void) {
    if (!G.namb || rnd(2)) return;
    for (int i = 0; i < MAXMOBS; i++) {
        Mob *m = &G.mobs[i];
        if (m->alive && m->rank == RK_GILDED && !m->amb && minded(m)) { m->amb = 1; m->bait_st = B_GILD; return; }
    }
}
static void amb_said(void) {                                       /* the first sprung on a run is named */
    int bit = clampi(crawl_run(G.level) % 32, 0, 31);
    RUN.amb_runs |= 1u << bit;
    way_met(WY_AMBUSH);
}
static void amb_release(Mob *m) {                                  /* a tooth stirs: out of the earth, off the plinth, down from the ceiling */
    if ((m->type == MOB_SHAMBLER || m->type == MOB_DROWNED) && m->sub == 1) {
        m->sub = 2; m->sub_t = 0.0f;
        sfx_at(m->type == MOB_DROWNED ? SFX_DRIP : SFX_DIRT, m->x, m->y, 1.2f);
        if (m->deco >= 0) G.decos[m->deco].spr = m->type == MOB_DROWNED ? SP_PUDDLE_A : SP_PIT;
    }
    if (m->type == MOB_GARGOYLE && m->sub == 0) garg_wake(m);
    if (m->type == MOB_HUNTER && m->sub == 0) { m->sub = 1; m->sub_t = 0.0f; }
}
static void amb_spring(int k, const char *why) {
    Ambush *a = &G.amb[k];
    if (a->sprung) return;
    a->sprung = 1; a->deaths0 = G.retries;
    if (a->eyes) G.decos[a->eyes - 1].spr = -1;                    /* the eyes come out of the dark */
    for (int i = 0; i < MAXMOBS; i++) {
        Mob *m = &G.mobs[i];
        if (!m->alive || m->state != MS_WALK || m->amb != k + 1) continue;
        if (m->bait_st == B_GILD) continue;
        amb_release(m);
        m->bait_st = m->bait_st ? B_HOME : B_NONE;
        m->alert = fmaxf(m->alert, MIND_LIVE + 2.0f); m->aware = MA_HUNT; m->kx = G.px; m->ky = G.py;
    }
    snd_sfx_ex(SFX_STINGER, 0.8f, 0.5f);
    amb_said();
    DBG_COUNT(ambushes_sprung);
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT ambush_sprung %s %s t=%.2f\n", TEETH_NAME[a->teeth], why, G.t);
#endif
}
static void amb_out(int k, float x, float y, int hunting) {        /* the bait slain outside: the teeth come out to look; refused: to hunt */
    Ambush *a = &G.amb[k];
    if (a->sprung) return;
    a->sprung = hunting ? 3 : 2;
    if (a->eyes) G.decos[a->eyes - 1].spr = -1;
    for (int i = 0; i < MAXMOBS; i++) {
        Mob *m = &G.mobs[i];
        if (!m->alive || m->state != MS_WALK || m->amb != k + 1 || m->bait_st == B_GILD) continue;
        amb_release(m);
        m->bait_st = m->bait_st ? B_HOME : B_NONE;
        m->home = -1;
        if (hunting) { m->alert = fmaxf(m->alert, MIND_LIVE + 3.0f); m->aware = MA_HUNT; m->kx = G.px; m->ky = G.py; }
        else mind_track(m, x, y);
    }
    if (hunting) DBG_COUNT(ambushes_refused); else DBG_COUNT(ambushes_out);
}
/* once a frame: you two cells into a kill room, or a tooth in your sight, and it springs */
static void amb_tick(void) {
    if (PK.depth || G.dead) return;
    for (int k = 0; k < G.namb; k++) {
        Ambush *a = &G.amb[k];
        if (a->sprung) continue;
        const Room *R = &G.rooms[a->room];
        if (in_room(R, (int)G.px, (int)G.py) && hypotf(G.px - a->ix - 0.5f, G.py - a->iy - 0.5f) >= 1.5f) { amb_spring(k, "walked_in"); continue; }
        for (int i = 0; i < MAXMOBS; i++) {
            const Mob *m = &G.mobs[i];
            if (!m->alive || m->state != MS_WALK || m->amb != k + 1 || m->bait_st || buried(m) || (m->type == MOB_SHADE && shade_hidden(m))) continue;
            float d = hypotf(m->x - G.px, m->y - G.py);
            if (m->type == MOB_GARGOYLE) { if (d < 2.5f) { amb_spring(k, "near"); break; } continue; }   /* a statue, to look at */
            if (d < 9.0f && fabsf(wrap_pi(atan2f(m->y - G.py, m->x - G.px) - G.ang)) < 0.6f && line_clear(G.px, G.py, m->x, m->y)) { amb_spring(k, "seen"); break; }
        }
    }
}
/* the bait's part, from mind_tick: 1 while it handles the creature itself */
static int bait_tick(Mob *m, float dt, int sees) {
    if (m->amb < 1 || m->amb > G.namb) { m->bait_st = B_NONE; return 0; }
    Ambush *a = &G.amb[m->amb - 1];
    if (a->sprung) { m->bait_st = B_HOME; return 0; }
    float pd = hypotf(G.px - m->x, G.py - m->y);
    if (sees) { m->kx = G.px; m->ky = G.py; }
    m->alert = 0.0f;
    switch (m->bait_st) {
    case B_WAIT:
        if (sees && G.level_t > 1.5 && pd < 12.0f) {
            m->bait_st = B_SHOW; m->bait_t = 0.0f; mind_tell(m);
            if (m->type == MOB_HOUND) sfx_at(SFX_YELP, m->x, m->y, 1.0f);   /* a yelp */
            if (m->type == MOB_SKEL && !m->bthrew) { throw_bone(m); m->bthrew = 1; }   /* the straggler throws once */
            DBG_COUNT(bait_seen);
        }
        return 1;
    case B_SHOW:                                                   /* it waits to be seen; come near, and it backs off */
        m->bait_t += dt;
        if (sees && pd < 6.0f) { m->bait_st = B_LURE; m->bait_t = 0.0f; DBG_COUNT(bait_run); return 1; }
        if (!sees && m->bait_t > 8.0f) { m->bait_st = B_WAIT; a->teases++; m->hx = clampi((int)m->kx, 1, MAPN - 2); m->hy = clampi((int)m->ky, 1, MAPN - 2); }   /* to tease again */
        break;
    case B_LURE:
        if (hypotf(a->fx + 0.5f - m->x, a->fy + 0.5f - m->y) < 1.3f) {   /* the far side: it turns */
            m->bait_st = B_HOME; m->alert = MIND_LIVE; m->aware = MA_HUNT; m->kx = G.px; m->ky = G.py;
            return 0;
        }
        if (!sees && pd > 12.0f) { if ((m->bait_t += dt) > 6.0f) { m->bait_st = B_WAIT; a->teases++; m->hx = clampi((int)m->kx, 1, MAPN - 2); m->hy = clampi((int)m->ky, 1, MAPN - 2); } }
        else m->bait_t = 0.0f;
        break;
    default: return 0;
    }
    if (a->teases >= 3) amb_out(m->amb - 1, G.px, G.py, 1);       /* refused: the set piece gives up its trap and comes */
    return 1;
}

