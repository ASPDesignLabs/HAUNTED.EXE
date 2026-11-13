/* A part of scene_crawl.c (docs/PLAN-tidy.md): it is #included there in its place and never compiled on its own.
 * Puzzles: which a level has, where the pieces go, solved and wrong, the shapes they take; the Crypts' (pz_make_run1); pz_make, pz_touched; the dream at a puzzle. */

/* ---- puzzles (1.0.0b13, docs/PLAN-b13.md, part C) ----
 * On even levels from the sixth, never a boss's, one puzzle: a sealed wing (a room with one way in, its door
 * shut by the puzzle, a hoard behind it as rich as a vault's, and on one puzzle level in three the exit), and
 * near its door the pieces and their clues. Only thought opens it: nothing a spell or an ability does, nothing
 * carried or learnt or what you were. It answers with sound: a click, a chime, a dull knock for a wrong move,
 * and when it is solved a rumble, dust, a choir and the door grinding open. After three wrong tries a whisper
 * gives a nudge, once. The dream solves it slowly, now and then wrongly, reading the answer from the game. */
static const char *const PZ_NAME[NPZK] = { "THE OSSUARY LEDGER", "THE KNELL", "THE EFFIGIES' GAZE", "THE BALANCE OF SOULS", "VOTIVE LIGHTS",
    "THE ABBOT'S URNS", "THE BOOK OF HOURS", "THREE CISTERNS", "THE SAINTS' ATTRIBUTES", "THE CULVERTS", "THE FERRYMAN'S STONES",
    "THE SLUICE LOCK", "THE DROWNED BELLS", "THE POINTS", "THE HOIST", "THE TALLY BOARD", "THE FOSSIL HALL", "THE CULT'S RING",
    "THE LAMPS OF THE LOST", "PRISMS", "THE COURT OF GLASS", "THE GLASS FLOOR", "SINGING CRYSTALS", "GOLD VEINS", "THE COOLING VENTS",
    "THE MAGIC SQUARE", "THE SEALED LETTERS" };
static const int8_t PZ_RUN[4][7] = { { PZ_LEDGER, PZ_KNELL, PZ_EFFIGY, PZ_BALANCE, PZ_VOTIVES, PZ_URNS, PZ_HOURS },
                                     { PZ_CISTERNS, PZ_SAINTS, PZ_CULVERTS, PZ_FERRY, PZ_SLUICELOCK, PZ_DBELLS, -1 },
                                     { PZ_POINTS, PZ_HOIST, PZ_TALLY, PZ_FOSSIL, PZ_RING, PZ_LAMPS, -1 },
                                     { PZ_PRISMS, PZ_COURT, PZ_GLASS, PZ_CRYSTALS, PZ_VEINS, PZ_VENTS, -1 } };
static int pz_level(int level) { return level >= 6 && level % 2 == 0 && !crawl_boss(level); }   /* P1: even levels from 6 */
/* which kind a puzzle level draws: one of its run's not yet met on this way down, the next descent the rest; now and
 * then (one in eight) the Magic Square or the Sealed Letters, in any run (P8) */
static int pz_kind_of(int level) {
#ifdef CRAWL_DEBUG
    if (getenv("CRAWL_PUZZLE")) return clampi(atoi(getenv("CRAWL_PUZZLE")), 0, NPZK - 1);
#endif
    int set = crawl_set(level), nk = 0, idx = (crawl_step(level) - 1) / 2 - (set == 0 ? 2 : 0), per = set == 0 ? 2 : 4;
    while (nk < 7 && PZ_RUN[set][nk] >= 0) nk++;
    uint32_t h = (uint32_t)level * 2654435761u ^ 0x5bd1e995u;
    h ^= h >> 15;
    if (h % 8 == 0) return h & 16 ? PZ_SQUARE : PZ_LETTERS;
    return PZ_RUN[set][(crawl_loop(level) * per + imax(0, idx)) % nk];
}
static int pz_diff(void) { return clampi((crawl_step(G.level) - 1) / 2 - (crawl_set(G.level) == 0 ? 2 : 0), 0, 3) + imin(2, crawl_loop(G.level)); }   /* P9 */
static int in_wing(int x, int y) { return G.pz >= 0 && G.pz_room >= 0 && in_room(&G.rooms[G.pz_room], x, y); }

/* ---- where the pieces go: found outward from the wing's door, never in a vault, the wing, a secret or the boss's band ---- */
static int16_t pz_dist[MAPN][MAPN];
static uint8_t pz_used[MAPN][MAPN];
static void pz_spread(void) {
    static int q[MAPN * MAPN];
    memset(pz_dist, 0xff, sizeof pz_dist); memset(pz_used, 0, sizeof pz_used);
    if (G.pz_wx < 0) return;
    int qh = 0, qt = 0;
    pz_dist[G.pz_wy][G.pz_wx] = 0; q[qt++] = G.pz_wy * MAPN + G.pz_wx;
    while (qh < qt) {
        int x = q[qh] % MAPN, y = q[qh] / MAPN; qh++;
        for (int d = 0; d < 4; d++) {
            int nx = x + DX[d], ny = y + DY[d];
            if (nx < 1 || ny < 1 || nx >= MAPN - 1 || ny >= MAPN - 1 || pz_dist[ny][nx] >= 0) continue;
            const Cell *c = &G.map[ny][nx];
            if ((c->kind != K_FLOOR && c->kind != K_DOOR) || c->secret || in_wing(nx, ny) || in_trial(nx, ny)) continue;
            if (c->kind == K_DOOR && G.doors[c->door].locked) continue;
            pz_dist[ny][nx] = (int16_t)(pz_dist[y][x] + 1); q[qt++] = ny * MAPN + nx;
        }
    }
}
static int pz_free(int x, int y) {                                 /* a cell nothing else of the puzzle's or the level's holds */
    if (!hz_floor(x, y) || pz_dist[y][x] < 0 || pz_used[y][x] || G.bulky[y][x] || item_at(x, y) || in_band(y) || vault_of_room(x, y) >= 0) return 0;
    if (abs(x - G.cx) + abs(y - G.cy) < 2 || (G.exit_fx >= 0 && abs(x - G.exit_fx) + abs(y - G.exit_fy) < 2)) return 0;
    if (G.altar_x >= 0 && abs(x - G.altar_x) + abs(y - G.altar_y) < 2) return 0;
    for (int i = 0; i < G.nsite; i++) if (abs(G.site[i].x - x) + abs(G.site[i].y - y) < 2) return 0;
    for (int d = 0; d < 4; d++) if (cell(x + DX[d], y + DY[d])->kind == K_DOOR) return 0;
    return 1;
}
static void pz_claim(int x, int y) { if (x >= 0 && y >= 0 && x < MAPN && y < MAPN) pz_used[y][x] = 1; }
/* the nearest free cell from dmin on that is: 0 against a wall (and which way), 1 a block's corner off the lanes for
 * something that stands, 2 any floor for something to lie, 3 a lane cell */
static int pz_spot(int what, int dmin, int *ox, int *oy, int *od) {
    int best = -1, bd = 1 << 20, bdir = 0;
    for (int y = 1; y < MAPN - 1; y++)
        for (int x = 1; x < MAPN - 1; x++) {
            int d = pz_dist[y][x], dir = -1;
            if (d < dmin || d >= bd || !pz_free(x, y)) continue;
            if (what == 0) { dir = wall_toward(x, y); if (dir < 0 || lane_dist(x, y) > 1 || cell(x + DX[dir], y + DY[dir])->secret || cell(x + DX[dir], y + DY[dir])->tex == T_EXIT0) continue; }
            else if (what == 1) { if (!block_corner(x, y) || lane(x, y) || by_way(x, y)) continue; }
            else if (what == 2) { if (lane_dist(x, y) > 1) continue; }
            else if (!lane(x, y)) continue;
            best = y * MAPN + x; bd = d; bdir = dir;
        }
    if (best < 0) return 0;
    *ox = best % MAPN; *oy = best / MAPN; if (od) *od = bdir;
    pz_claim(*ox, *oy);
    return 1;
}
/* a piece against a wall, a piece standing, an item lying: with its name, near the door (or, for an item, farther) */
static int pz_wallthing(int role, int spr, int dmin, const char *name) {
    int x, y, d;
    if (!pz_spot(0, dmin, &x, &y, &d)) return -1;
    int i = thing_wall(role, x, y, d, spr);
    if (i >= 0) snprintf(G.thing[i].name, sizeof G.thing[i].name, "%s", name);
    return i;
}
static int pz_standing(int role, int spr, int dmin, int solid, const char *name) {
    int x, y;
    if (!pz_spot(1, dmin, &x, &y, NULL) && !pz_spot(2, dmin, &x, &y, NULL)) return -1;
    int i = thing_new(role, x + 0.5f, y + 0.5f, spr);
    if (i < 0) return -1;
    if (solid && !lane(x, y)) G.map[y][x].kind = K_PROP;
    snprintf(G.thing[i].name, sizeof G.thing[i].name, "%s", name);
    return i;
}
static int pz_lying(int role, int spr, int dmin, const char *name) {
    int x, y;
    if (!pz_spot(2, dmin, &x, &y, NULL)) return -1;
    int i = thing_new(role, x + 0.3f + 0.4f * rndf(), y + 0.3f + 0.4f * rndf(), spr);
    if (i >= 0) snprintf(G.thing[i].name, sizeof G.thing[i].name, "%s", name);
    return i;
}
static int pz_reading(int spr, int dmin, const char *name, const char *text) {   /* a clue: on a wall if it can be */
    int i = pz_wallthing(TR_READ, spr, dmin, name);
    if (i < 0) i = pz_standing(TR_READ, spr, dmin, 0, name);
    if (i >= 0) snprintf(G.thing[i].text, sizeof G.thing[i].text, "%s", text);
    return i;
}
/* a pile: a place that holds more than one, each laid on the last */
static int pile_count(int place) { int n = 0; for (int it = G.thing[place].in; it >= 0 && n < MAXTHING; it = G.thing[it].below) n++; return n; }
static int pile_has(int place, int item) { for (int it = G.thing[place].in, n = 0; it >= 0 && n < MAXTHING; it = G.thing[it].below, n++) if (it == item) return 1; return 0; }
static int pile_sum(int place) { int s = 0, n = 0; for (int it = G.thing[place].in; it >= 0 && n < MAXTHING; it = G.thing[it].below, n++) s += G.thing[it].val; return s; }

static const char *const MARK_WORD[16] = { "MITRE", "HELM", "TONSURE", "CROWN", "KEY", "WHEEL", "ARROWS", "TOWER", "LAMB", "LAMP", "EYE", "HAND",
                                           "PICKS", "MOON", "STAR", "SKULL" };
static const char *const NTH[10] = { "FIRST", "SECOND", "THIRD", "FOURTH", "FIFTH", "SIXTH", "SEVENTH", "EIGHTH", "NINTH", "TENTH" };
static void pz_append(char *buf, int n, const char *fmt, ...) {
    size_t l = strlen(buf);
    if ((int)l >= n - 1) return;
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf + l, (size_t)n - l, fmt, ap);
    va_end(ap);
}
static void shuffle_int(int *a, int n) { for (int i = n - 1; i > 0; i--) { int j = rnd(i + 1), t = a[i]; a[i] = a[j]; a[j] = t; } }

/* ---- solved, and wrong ---- */
static void pz_solve(void) {
    if (G.pz < 0 || G.pz_solved) return;
    G.pz_solved = 1;
    if (G.pz_door >= 0) { Door *d = &G.doors[G.pz_door]; d->locked = 0; d->opening = 1; }
    float dx = G.pz_door >= 0 ? G.doors[G.pz_door].x + 0.5f : G.px, dy = G.pz_door >= 0 ? G.doors[G.pz_door].y + 0.5f : G.py;
    snd_sfx_ex(SFX_ATONE, 0.7f, 0.5f); sfx_at(SFX_PUSHWALL, dx, dy, 1.2f); sfx_at(SFX_BOOM, dx, dy, 0.5f);
    rumble(0.5f, 0.2f, 0.5f);
    for (int i = 0; i < MAXBOOMS; i++) if (!G.booms[i].alive) { G.booms[i] = (Boom){ 1, dx, dy, 0.0f, 1.0f, 0 }; break; }   /* dust */
    say("THE WAY IS OPEN");
    xp_gain(60.0f * xp_level(), PZ_NAME[G.pz]);                     /* P7: a chain's end at the level */
    rec_puzzle();
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT pz_solved %d level %d tries %d by %s t=%.2f level_t=%.1f\n", G.pz, G.level, G.pz_tries, G.player ? "player" : "autopilot", G.t, G.level_t);
#endif
}
static void pz_wrong(void) {
    G.pz_tries++;
    snd_sfx_ex(SFX_KNOCK, 0.7f, 0.5f);
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT pz_wrong %d try %d by %s t=%.2f\n", G.pz, G.pz_tries, G.player ? "player" : "autopilot", G.t);
#endif
    if (G.pz_tries >= 3 && !G.pz_hinted && G.pz_hint[0]) {         /* P5: a nudge, once */
        G.pz_hinted = 1;
        say("A WHISPER: %s", G.pz_hint);
        snd_sfx_ex(SFX_WHISPER, 0.8f, 0.5f);
    }
}

/* ---- the shapes a puzzle takes: things into their places, an order kept, every piece set right ---- */
enum { PA_NONE, PA_USE, PA_DROP, PA_STEP, PA_WAIT };
static int pz_is(int i, int role, int grp) { return G.thing[i].role == role && G.thing[i].grp == grp; }
static int match_done(void) {                                      /* every place (group 1) holds the item it wants */
    int any = 0;
    for (int i = 0; i < G.nthing; i++) {
        if (!pz_is(i, TR_PLACE, 1)) continue;
        any = 1;
        if (G.thing[i].in < 0 || G.thing[G.thing[i].in].val != G.thing[i].want) return 0;
    }
    return any;
}
static int match_full(void) { for (int i = 0; i < G.nthing; i++) if (pz_is(i, TR_PLACE, 1) && G.thing[i].in < 0) return 0; return 1; }
static int match_next(int *a) {                                    /* the dream's next move at it */
    if (G.carry) {
        int c = G.carry - 1;
        for (int i = 0; i < G.nthing; i++) if (pz_is(i, TR_PLACE, 1) && G.thing[i].want == G.thing[c].val && G.thing[i].in < 0) { *a = i; return PA_USE; }
        return PA_DROP;
    }
    for (int i = 0; i < G.nthing; i++) if (pz_is(i, TR_PLACE, 1) && G.thing[i].in >= 0 && G.thing[G.thing[i].in].val != G.thing[i].want) { *a = i; return PA_USE; }
    for (int i = 0; i < G.nthing; i++) {
        if (!pz_is(i, TR_ITEM, 2) || (G.thing[i].in >= 0 && G.thing[G.thing[i].in].want == G.thing[i].val)) continue;
        for (int p = 0; p < G.nthing; p++)
            if (pz_is(p, TR_PLACE, 1) && G.thing[p].want == G.thing[i].val && G.thing[p].in < 0) { *a = G.thing[i].in >= 0 ? G.thing[i].in : i; return PA_USE; }
    }
    return PA_NONE;
}
static int setting(int i) { const Thing *t = &G.thing[i]; return t->grp == 1 && t->want >= 0 && (t->role == TR_TURN || t->role == TR_LIGHT || t->role == TR_LEVER); }   /* one that matters */
static int settings_done(void) { int any = 0; for (int i = 0; i < G.nthing; i++) if (setting(i)) { any = 1; if (G.thing[i].state != G.thing[i].want) return 0; } return any; }
static int settings_next(int *a) { for (int i = 0; i < G.nthing; i++) if (setting(i) && G.thing[i].state != G.thing[i].want) { *a = i; return PA_USE; } return PA_NONE; }
static int seq_next(int *a) {                                      /* the piece the order wants next */
    for (int i = 0; i < G.nthing; i++) if (G.thing[i].grp == 1 && G.thing[i].val == G.pz_seq[G.pz_step]) { *a = i; return PA_USE; }
    return PA_NONE;
}
static void seq_touch(int i, const char *fail) {                   /* in the order, or back to its beginning */
    if (G.thing[i].grp != 1) return;
    if (G.thing[i].val == G.pz_seq[G.pz_step]) {
        if (++G.pz_step >= G.pz_nseq) pz_solve();
        return;
    }
    G.pz_step = 0;
    if (fail) say("%s", fail);
    pz_wrong();
}
/* the hours, the knell's bells and the like: words for small numbers */
static const char *const HOURS[8] = { "MATINS", "LAUDS", "PRIME", "TERCE", "SEXT", "NONE", "VESPERS", "COMPLINE" };
static const char *const BELL_WORD[4] = { "THE LEAST", "THE LESSER", "THE GREATER", "THE GREAT" };
static const int BELL_NOTE[4] = { 79, 74, 67, 60 };

/* ---- the Crypts' ---- */
/* the Abbot's Urns: n urns on three plinths, never a larger on a smaller. Each state is where each urn stands, a digit
 * in base three; distances from the goal (all on the third) found once */
static int8_t urn_far[81];
static int urn_state(void) {
    int st = 0, p3 = 1;
    for (int u = 0; u < G.pz_a[0]; u++, p3 *= 3) {
        int peg = 0;
        for (int k = 0; k < 3; k++) if (pile_has(G.pz_a[1 + k], G.pz_a[4 + u])) peg = k;
        st += peg * p3;
    }
    return st;
}
static int urn_top(int st, int peg, int n) { int p3 = 1; for (int u = 0; u < n; u++, p3 *= 3) if ((st / p3) % 3 == peg) return u; return n; }   /* smallest there */
static void urn_solve(int n) {
    int total = 1;
    for (int u = 0; u < n; u++) total *= 3;
    memset(urn_far, -1, sizeof urn_far);
    int q[81], qh = 0, qt = 0, goal = total - 1;
    urn_far[goal] = 0; q[qt++] = goal;
    while (qh < qt) {
        int st = q[qh++];
        for (int a = 0; a < 3; a++)
            for (int b = 0; b < 3; b++) {
                int u = urn_top(st, a, n);
                if (a == b || u >= n || urn_top(st, b, n) < u) continue;
                int p3 = 1;
                for (int k = 0; k < u; k++) p3 *= 3;
                int ns = st + (b - a) * p3;
                if (urn_far[ns] < 0) { urn_far[ns] = (int8_t)(urn_far[st] + 1); q[qt++] = ns; }
            }
    }
}
static int urns_next(int *a) {
    int n = G.pz_a[0], st = urn_state();
    if (G.carry) {                                                 /* where the urn in hand does most good */
        int u = -1;
        for (int k = 0; k < n; k++) if (G.pz_a[4 + k] == G.carry - 1) u = k;
        if (u < 0) return PA_DROP;
        int best = -1, bd = 99, p3 = 1;
        for (int k = 0; k < u; k++) p3 *= 3;
        int base = st - ((st / p3) % 3) * p3;
        for (int b = 0; b < 3; b++) {
            if (urn_top(base, b, n) < u) continue;
            int d = urn_far[base + b * p3];
            if (d >= 0 && d < bd) { bd = d; best = b; }
        }
        if (best < 0) return PA_DROP;
        *a = G.pz_a[1 + best];
        return PA_USE;
    }
    int bd = urn_far[st], from = -1;
    for (int x = 0; x < 3; x++)
        for (int b = 0; b < 3; b++) {
            int u = urn_top(st, x, n);
            if (x == b || u >= n || urn_top(st, b, n) < u) continue;
            int p3 = 1;
            for (int k = 0; k < u; k++) p3 *= 3;
            if (urn_far[st + (b - x) * p3] < bd) { bd = urn_far[st + (b - x) * p3]; from = x; }
        }
    if (from < 0) return PA_NONE;
    *a = G.pz_a[1 + from];
    return PA_USE;
}
/* Votive Lights: nine on a rack, three by three; touching one lights or snuffs it and the ones beside it */
static int votive_flip(int cell) { int r = cell / 3, c = cell % 3, m = 1 << cell; if (r > 0) m |= 1 << (cell - 3); if (r < 2) m |= 1 << (cell + 3); if (c > 0) m |= 1 << (cell - 1); if (c < 2) m |= 1 << (cell + 1); return m; }
static int votive_lit(void) { int m = 0; for (int k = 0; k < 9; k++) if (G.thing[G.pz_a[k]].state) m |= 1 << k; return m; }
static int votives_next(int *a) {
    int now = votive_lit(), want = G.pz_a[9], best = -1, bn = 99;
    for (int s = 0; s < 512; s++) {
        int m = now, n = 0;
        for (int k = 0; k < 9; k++) if (s >> k & 1) { m ^= votive_flip(k); n++; }
        if (m == want && n < bn) { bn = n; best = s; }
    }
    for (int k = 0; k < 9 && best > 0; k++) if (best >> k & 1) { *a = G.pz_a[k]; return PA_USE; }
    return PA_NONE;
}
/* the Balance of Souls: the marked reliquary in the left pan, and the pans hanging level */
static int balance_done(void) { int L = G.pz_a[0], R = G.pz_a[1]; return pile_has(L, G.pz_a[2]) && G.thing[R].in >= 0 && pile_sum(L) == pile_sum(R); }
static int balance_next(int *a) {                                  /* pz_a[3..]: each reliquary's pan in the answer (0 left, 1 right, 2 neither) */
    int L = G.pz_a[0], R = G.pz_a[1], n = G.pz_a[10];
    if (G.carry) {
        for (int k = 0; k < n; k++) if (G.pz_a[11 + k] == G.carry - 1) { int w = G.pz_a[3 + k]; if (w < 2) { *a = w ? R : L; return PA_USE; } }
        return PA_DROP;
    }
    for (int p = 0; p < 2; p++) {                                  /* the top of a pan that is wrong comes off */
        int top = G.thing[p ? R : L].in;
        for (int k = 0; k < n && top >= 0; k++) if (G.pz_a[11 + k] == top && G.pz_a[3 + k] != p) { *a = p ? R : L; return PA_USE; }
    }
    for (int k = 0; k < n; k++) {
        int it = G.pz_a[11 + k], w = G.pz_a[3 + k];
        if (w < 2 && !pile_has(w ? R : L, it)) {
            if (G.thing[it].in >= 0) { int p = G.thing[it].in; int top = G.thing[p].in; if (top != it) { *a = p; return PA_USE; } }   /* dig it out */
            *a = G.thing[it].in >= 0 ? G.thing[it].in : it; return PA_USE;
        }
    }
    return PA_NONE;
}
/* the Effigies' Gaze: which way a head looks, from a knight to whom it looks to */
static int dir_to(float x0, float y0, float x1, float y1) { float dx = x1 - x0, dy = y1 - y0; return fabsf(dx) >= fabsf(dy) ? (dx >= 0.0f ? 0 : 2) : (dy >= 0.0f ? 1 : 3); }
static const char *const KNIGHT_NAME[6] = { "ALDHELM", "BEDE", "CUTHBERT", "DUNSTAN", "EADBERT", "FELIX" };

static void pz_make_run1(int kind, int diff) {
    char *ans = G.pz_answer, *hint = G.pz_hint;
    int n;
    switch (kind) {
    case PZ_LEDGER: {                                              /* skulls with the Order's ranks on their brows, and niches cut with them */
        static const int POOL[6] = { 0, 1, 2, 3, 13, 14 };
        int m[6];
        n = imin(6, 3 + diff);
        for (int k = 0; k < 6; k++) m[k] = POOL[k];
        shuffle_int(m, 6);
        snprintf(ans, 200, "THE SKULLS, EACH TO THE NICHE CUT WITH ITS MARK:");
        for (int k = 0; k < n; k++) {
            char nm[40];
            snprintf(nm, sizeof nm, "THE NICHE OF THE %s", MARK_WORD[m[k]]);
            int p = pz_wallthing(TR_PLACE, SP_PZ_NICHE, 1, nm);
            if (p < 0) break;
            Thing *t = &G.thing[p];
            t->grp = 1; t->want = m[k]; t->mark = (int16_t)(m[k] + 1); t->mz = 0.18f; t->hold = 0.02f;
            snprintf(nm, sizeof nm, "A SKULL MARKED WITH A %s", MARK_WORD[m[k]]);
            int s = pz_lying(TR_ITEM, SP_PZ_SKULL, 5, nm);
            if (s < 0) break;
            G.thing[s].grp = 2; G.thing[s].val = m[k]; G.thing[s].mark = (int16_t)(m[k] + 1); G.thing[s].mz = -0.3f;
            pz_append(ans, 200, " THE %s IN THE %s,", MARK_WORD[m[k]], NTH[k]);
        }
        snprintf(hint, 100, "EACH SKULL TO THE NICHE CUT WITH ITS OWN MARK");
        break;
    }
    case PZ_KNELL: case PZ_DBELLS: {                               /* bells to ring in an order a banner (or a carving of ripples) gives */
        int dbells = kind == PZ_DBELLS;
        static const char *const DEPTH_WORD[4] = { "THE SHALLOWEST", "THE SHALLOW", "THE DEEP", "THE DEEPEST" };
        static const char *const ROMAN[4] = { "I", "II", "III", "IV" };   /* cut in each bell, and in the words (1.0.0b18.1) */
        n = imin(7, 4 + diff);
        for (int k = 0; k < 4; k++) {
            char nm[40];
            snprintf(nm, sizeof nm, "%s BELL (%s)", dbells ? DEPTH_WORD[k] : BELL_WORD[k], ROMAN[k]);
            int b = pz_standing(TR_STRIKE, SP_PBELL0 + k, 1, 0, nm);   /* each its own: shape, bands, metal, numeral */
            if (b < 0) break;
            G.thing[b].grp = 1; G.thing[b].val = k; G.thing[b].scale = 0.6f + 0.17f * (float)k;
            G.thing[b].z = dbells ? -0.08f * (float)k : 0.0f;
        }
        G.pz_nseq = n; G.pz_step = 0;
        for (int k = 0; k < n; k++) { int v; do v = rnd(4); while (k && v == G.pz_seq[k - 1]); G.pz_seq[k] = v; }
        char txt[160];
        snprintf(txt, sizeof txt, "%s", dbells ? "THE RIPPLES, AS THE BELLS SOUND:" : "THE ORDER'S KNELL, IN BELLS:");
        for (int k = 0; k < n; k++) pz_append(txt, 160, " %s (%s)%s", dbells ? DEPTH_WORD[G.pz_seq[k]] + 4 : BELL_WORD[G.pz_seq[k]] + 4, ROMAN[G.pz_seq[k]], k < n - 1 ? "," : "");
        pz_reading(dbells ? SP_PZ_TABLET : SP_PZ_BANNER, 1, dbells ? "A CARVING OF RIPPLES" : "THE BANNER OVER THE BELLS", txt);
        snprintf(ans, 200, "RING THEM SO:%s", strchr(txt, ':') + 1);
        snprintf(hint, 100, dbells ? "THE DEEPER IT HANGS, THE LOWER IT SOUNDS" : "THE BANNER'S BELLS, FROM ITS LEFT");
        break;
    }
    case PZ_EFFIGY: {                                              /* knights on their tombs, heads to turn, each as its epitaph says */
        int ids[6], ne = imin(6, 4 + (diff + 1) / 2);
        for (n = 0; n < ne; n++) {
            char nm[40];
            snprintf(nm, sizeof nm, "SIR %s'S EFFIGY", KNIGHT_NAME[n]);
            ids[n] = pz_standing(TR_TURN, SP_PZ_EFFIGY0, 1, 1, nm);
            if (ids[n] < 0) break;
            Thing *t = &G.thing[ids[n]];
            t->grp = 1; t->nstate = 4; t->frames = 4; t->facing = 1;
        }
        char tab[160];
        snprintf(tab, sizeof tab, "FROM THE ORDER'S BOOK:");
        int order[6];
        for (int k = 0; k < n; k++) order[k] = k;
        shuffle_int(order, n);
        int k = 0;
        snprintf(ans, 200, "THE HEADS:");
        while (k < n) {                                             /* brothers look to each other; the betrayed turn from their betrayer; the rest watch the door */
            int i = order[k], kind2 = k + 1 < n ? rnd(3) : 2;
            Thing *a = &G.thing[ids[i]];
            if (kind2 == 0 && k + 1 < n) {
                int j = order[k + 1];
                Thing *b = &G.thing[ids[j]];
                a->want = dir_to(a->x, a->y, b->x, b->y); b->want = dir_to(b->x, b->y, a->x, a->y);
                snprintf(a->text, sizeof a->text, "SIR %s: I LOOK TO MY BROTHER", KNIGHT_NAME[i]);
                snprintf(b->text, sizeof b->text, "SIR %s: I LOOK TO MY BROTHER", KNIGHT_NAME[j]);
                pz_append(tab, 160, " %s AND %s WERE BROTHERS.", KNIGHT_NAME[i], KNIGHT_NAME[j]);
                k += 2;
            } else if (kind2 == 1 && k + 1 < n) {
                int j = order[k + 1];
                Thing *b = &G.thing[ids[j]];
                a->want = (dir_to(a->x, a->y, b->x, b->y) + 2) & 3;
                snprintf(a->text, sizeof a->text, "SIR %s: I TURN FROM THE ONE WHO BETRAYED ME", KNIGHT_NAME[i]);
                b->want = dir_to(b->x, b->y, G.pz_wx + 0.5f, G.pz_wy + 0.5f);
                snprintf(b->text, sizeof b->text, "SIR %s: I WATCH THE DOOR", KNIGHT_NAME[j]);
                pz_append(tab, 160, " %s BETRAYED %s.", KNIGHT_NAME[j], KNIGHT_NAME[i]);
                k += 2;
            } else {
                a->want = dir_to(a->x, a->y, G.pz_wx + 0.5f, G.pz_wy + 0.5f);
                snprintf(a->text, sizeof a->text, "SIR %s: I WATCH THE DOOR", KNIGHT_NAME[i]);
                k++;
            }
        }
        static const char *const WAY[4] = { "EAST", "SOUTH", "WEST", "NORTH" };
        for (int i = 0; i < n; i++) {
            Thing *t = &G.thing[ids[i]];
            do t->state = (uint8_t)rnd(4); while (n > 1 && t->state == t->want && rnd(3));
            pz_append(ans, 200, " %s %s,", KNIGHT_NAME[i], WAY[t->want]);
        }
        pz_reading(SP_PZ_TABLET, 1, "A TABLET OF THE ORDER", tab);
        snprintf(hint, 100, "EACH EPITAPH SAYS WHERE ITS KNIGHT LOOKS");
        break;
    }
    case PZ_BALANCE: {                                             /* reliquaries of one to five beads; level, with the marked in the left */
        int L = pz_standing(TR_PLACE, SP_PZ_PAN, 1, 1, "THE LEFT PAN"), R = pz_standing(TR_PLACE, SP_PZ_PAN, 1, 1, "THE RIGHT PAN");
        int sc = pz_standing(TR_READ, SP_PZ_SCALE0, 1, 1, "THE GREAT SCALE");
        if (L < 0 || R < 0) break;
        G.thing[L].cap = G.thing[R].cap = 5; G.thing[L].hold = G.thing[R].hold = 0.22f; G.thing[L].mark = 14; G.thing[R].mark = 15;
        G.thing[L].mz = G.thing[R].mz = -0.2f;
        if (sc >= 0) { G.thing[sc].frames = 3; G.thing[sc].state = 1; snprintf(G.thing[sc].text, sizeof G.thing[sc].text, "A GREAT SCALE: WHAT IS JUDGED HANGS IN THE LEFT PAN, AND THE PANS MUST HANG LEVEL"); }
        int w0 = 2 + rnd(3), left = diff >= 1 ? 1 + rnd(3) : 0, right = w0 + left, want[3], wn = right > 5 || (right >= 2 && rnd(2)) ? 2 : 1;
        if (wn == 1) want[0] = right;                              /* the right pan: one, or two that make it up */
        else { want[0] = clampi(1 + rnd(right - 1), right - 5, 5); want[1] = right - want[0]; }
        int lw[1] = { left };
        int decoys = 1 + (diff >= 2);
        n = 0;
        G.pz_a[0] = L; G.pz_a[1] = R;
        snprintf(ans, 200, "IN THE LEFT PAN THE MARKED RELIQUARY (%d)", w0);
        for (int k = 0; k < 1 + (left > 0) + wn + decoys && n < 7; k++) {
            int w, side, marked = k == 0;
            if (k == 0) { w = w0; side = 0; }
            else if (left > 0 && k == 1) { w = lw[0]; side = 0; pz_append(ans, 200, " AND ONE OF %d", w); }
            else if (k < 1 + (left > 0) + wn) { w = want[k - 1 - (left > 0)]; side = 1; }
            else { w = 1 + rnd(5); side = 2; }
            char nm[40];
            snprintf(nm, sizeof nm, "%s RELIQUARY OF %d BEAD%s", marked ? "THE MARKED" : "A", w, w == 1 ? "" : "S");
            int it = pz_lying(TR_ITEM, SP_PZ_CASKET, 2, nm);
            if (it < 0) break;
            Thing *t = &G.thing[it];
            t->grp = 2; t->val = w; t->mark = (int16_t)(16 + w); t->mz = -0.32f; if (marked) t->ramp = R_BLOOD;
            G.pz_a[3 + n] = side; G.pz_a[11 + n] = it; n++;
        }
        G.pz_a[2] = G.pz_a[11]; G.pz_a[10] = n;
        pz_append(ans, 200, "; IN THE RIGHT,");
        for (int k = 0; k < n; k++) if (G.pz_a[3 + k] == 1) pz_append(ans, 200, " %d", G.thing[G.pz_a[11 + k]].val);
        snprintf(hint, 100, "COUNT THE BEADS: THE PANS HANG LEVEL WHEN THEY ARE EQUAL");
        break;
    }
    case PZ_VOTIVES: {                                             /* nine on the floor three by three, and the window's pattern */
        int bx = -1, by = -1, bd = 1 << 20;                        /* a whole block of a room, the nearest to the door */
        for (int y = 1; y < MAPN - 3; y++)
            for (int x = 1; x < MAPN - 3; x++) {
                if ((x & 3) != 1 || (y & 3) != 1 || pz_dist[y + 1][x + 1] < 1 || pz_dist[y + 1][x + 1] >= bd) continue;
                int ok = 1;
                for (int k = 0; k < 9 && ok; k++) ok = pz_free(x + k % 3, y + k / 3) && room_kind_at(x + k % 3, y + k / 3) >= 0;
                if (ok) { bx = x; by = y; bd = pz_dist[y + 1][x + 1]; }
            }
        if (bx < 0) break;
        int want = 0, start;
        while (!want) want = rnd(512);
        start = want;
        for (int k = 0, presses = 2 + diff; k < presses; k++) start ^= votive_flip(rnd(9));
        if (start == want) start ^= votive_flip(4);
        for (int k = 0; k < 9; k++) {
            int x = bx + k % 3, y = by + k / 3;
            pz_claim(x, y);
            int c = thing_new(TR_LIGHT, x + 0.5f, y + 0.5f, SP_PZ_CANDLE0);
            if (c < 0) break;
            Thing *t = &G.thing[c];
            t->grp = 1; t->frames = 2; t->state = (uint8_t)(start >> k & 1); t->want = want >> k & 1; t->val = k;
            snprintf(t->name, sizeof t->name, "A VOTIVE CANDLE");
            G.pz_a[k] = c;
        }
        G.pz_a[9] = want;
        char txt[160];
        snprintf(txt, sizeof txt, "STAINED GLASS OVER THE RACK, ROW BY ROW FROM THE DOOR'S SIDE:");
        for (int r = 0; r < 3; r++) { pz_append(txt, 160, r ? ";" : ""); for (int c = 0; c < 3; c++) pz_append(txt, 160, " %s", want >> (r * 3 + c) & 1 ? "LIT" : "DARK"); }
        pz_reading(SP_PZ_TABLET, 1, "A STAINED-GLASS WINDOW", txt);
        snprintf(ans, 200, "THE CANDLES AS THE WINDOW HAS THEM:%s", strchr(txt, ':') + 1);
        snprintf(hint, 100, "EACH CANDLE YOU TOUCH TURNS ITS NEIGHBOURS TOO");
        break;
    }
    case PZ_URNS: {                                                /* the urns from the moon's plinth to the star's, as the old puzzle */
        static const int PM[3] = { 13, 10, 14 };
        static const char *const PN[3] = { "THE PLINTH OF THE MOON", "THE PLINTH OF THE EYE", "THE PLINTH OF THE STAR" };
        n = diff >= 2 ? 4 : 3;
        for (int k = 0; k < 3; k++) {
            int p = pz_standing(TR_PLACE, SP_PZ_PLINTH, 1, 1, PN[k]);
            if (p < 0) return;
            G.thing[p].grp = 1; G.thing[p].cap = (uint8_t)n; G.thing[p].hold = 0.2f; G.thing[p].mark = (int16_t)(PM[k] + 1); G.thing[p].mz = -0.25f; G.thing[p].want = -1;
            G.pz_a[1 + k] = p;
        }
        G.pz_a[0] = n;
        for (int u = n - 1; u >= 0; u--) {                         /* the largest at the bottom */
            int it = thing_new(TR_ITEM, 0.0f, 0.0f, SP_URN);
            if (it < 0) return;
            Thing *t = &G.thing[it];
            t->grp = 2; t->val = u; t->scale = 0.55f + 0.15f * (float)u;
            snprintf(t->name, sizeof t->name, "THE %s URN", u == 0 ? "SMALLEST" : u == n - 1 ? "LARGEST" : (u == 1 ? "SMALLER" : "LARGER"));
            G.pz_a[4 + u] = it;
            thing_pile(it, G.pz_a[1]);
        }
        urn_solve(n);
        snprintf(ans, 200, "ALL THE URNS TO THE STAR'S PLINTH: THE SMALLEST MOVES EVERY OTHER TIME, ALWAYS THE SAME WAY ROUND; BETWEEN, MAKE THE ONE MOVE THAT CAN BE MADE.");
        pz_reading(SP_PZ_TABLET, 1, "THE ABBOT'S RULE", "THE ABBOT'S URNS GO FROM THE MOON TO THE STAR, ONE AT A TIME, AND NEVER THE LARGER ON THE SMALLER.");
        snprintf(hint, 100, "THE SMALLEST URN MOVES EVERY OTHER TIME");
        break;
    }
    case PZ_HOURS: {                                               /* a dial of the eight hours, a lectern's line, the sundial's stones, the bell rope */
        int dial = pz_standing(TR_TURN, SP_PZ_DIAL0, 1, 1, "THE DIAL OF THE HOURS");
        int rope = pz_wallthing(TR_LEVER, SP_PZ_LEVER0, 1, "THE BELL ROPE");
        if (dial < 0 || rope < 0) break;
        int stone = rnd(8), before = diff >= 1 ? rnd(3) : 0, h = (stone - before + 8) % 8;
        Thing *d = &G.thing[dial];
        d->grp = 0; d->nstate = 8; d->frames = 8; d->state = (uint8_t)((h + 1 + rnd(7)) % 8); d->want = h;
        G.thing[rope].grp = 0; G.thing[rope].frames = 2;
        G.pz_a[0] = dial; G.pz_a[1] = rope;
        char txt[160];
        if (before) snprintf(txt, sizeof txt, "WE SANG %s HOUR%s BEFORE THE SHADOW FELL ON THE %s STONE.", before == 1 ? "ONE" : "TWO", before == 1 ? "" : "S", NTH[stone]);
        else snprintf(txt, sizeof txt, "WE SANG WHEN THE SHADOW FELL ON THE %s STONE.", NTH[stone]);
        pz_reading(SP_LECTERN, 1, "A BOOK OF HOURS ON A LECTERN", txt);
        pz_reading(SP_PZ_TABLET, 1, "THE SUNDIAL'S STONES", "STONES NUMBERED FOR THE HOURS IN TURN: MATINS ON THE FIRST, LAUDS, PRIME, TERCE, SEXT, NONE, VESPERS, AND COMPLINE ON THE EIGHTH.");
        snprintf(ans, 200, "SET THE DIAL TO %s, AND PULL THE BELL ROPE.", HOURS[h]);
        snprintf(hint, 100, "THE STONES COUNT THE HOURS FROM MATINS");
        break;
    }
    default: break;
    }
}
static void pz_touch_run1(int i, int how) {
    Thing *t = &G.thing[i];
    switch (G.pz) {
    case PZ_LEDGER:
        if (how == TU_SET && t->grp == 2) {
            if (match_done()) pz_solve();
            else if (match_full()) { say("THE NICHES ARE FULL, AND NOTHING MOVES"); pz_wrong(); }
        }
        break;
    case PZ_KNELL: case PZ_DBELLS:
        if (how == TU_STRIKE) {
            snd_tone(0, BELL_NOTE[clampi(t->val, 0, 3)] - (G.pz == PZ_DBELLS ? 5 : 0), 0.8f, 0.5f);
            seq_touch(i, G.pz == PZ_KNELL ? "ALL FOUR HUM DOWN TO SILENCE" : "THE WATER STILLS THE BELLS");
        }
        break;
    case PZ_EFFIGY:
        if (how == TU_TURN) {
            if (t->text[0]) { say("%s", t->text); doc_add(t->name, t->text, 0); }
            if (settings_done()) pz_solve();
        }
        break;
    case PZ_BALANCE:
        if (how == TU_SET || how == TU_TAKE) {
            int L = pile_sum(G.pz_a[0]), R = pile_sum(G.pz_a[1]);
            for (int k = 0; k < G.nthing; k++) if (G.thing[k].role == TR_READ && G.thing[k].frames == 3) G.thing[k].state = (uint8_t)(L > R ? 0 : L == R ? 1 : 2);
            if (how == TU_SET && balance_done()) pz_solve();
            else if (how == TU_SET && L == R && L > 0) { say("LEVEL, BUT THE JUDGED IS NOT IN THE LEFT PAN"); pz_wrong(); }
        }
        break;
    case PZ_VOTIVES:
        if (how == TU_LIGHT && t->grp == 1) {
            t->state ^= 1;                                         /* the one touched is already turned: put it back, then flip the cross */
            int m = votive_lit() ^ votive_flip(t->val);
            for (int k = 0; k < 9; k++) G.thing[G.pz_a[k]].state = (uint8_t)(m >> k & 1);
            if (m == G.pz_a[9]) pz_solve();
        }
        break;
    case PZ_URNS:
        if (how == TU_SET && pile_count(G.pz_a[3]) == G.pz_a[0]) pz_solve();
        break;
    case PZ_HOURS:
        if (how == TU_TURN && i == G.pz_a[0]) say("THE DIAL: %s", HOURS[t->state & 7]);
        if (how == TU_PULL && i == G.pz_a[1]) {
            snd_tone(0, 55, 0.9f, 0.5f);
            if (G.thing[G.pz_a[0]].state == G.thing[G.pz_a[0]].want) pz_solve();
            else { say("THE BELL TOLLS, AND NOTHING ANSWERS"); pz_wrong(); }
            t->state = 0;
        }
        break;
    default: break;
    }
}
static int pz_next_run1(int *a) {
    switch (G.pz) {
    case PZ_LEDGER: return match_next(a);
    case PZ_KNELL: case PZ_DBELLS: return seq_next(a);
    case PZ_EFFIGY: return settings_next(a);
    case PZ_BALANCE: return balance_next(a);
    case PZ_VOTIVES: return votives_next(a);
    case PZ_URNS: return urns_next(a);
    case PZ_HOURS: { const Thing *d = &G.thing[G.pz_a[0]]; *a = d->state != d->want ? G.pz_a[0] : G.pz_a[1]; return PA_USE; }
    default: return PA_NONE;
    }
}

static void pz_make_run2(int kind, int diff);
static float pz_dt;
static void pz_make_run3(int kind, int diff);
static void pz_make_run4(int kind, int diff);
static void pz_touch_run2(int i, int how);
static void pz_touch_run3(int i, int how);
static void pz_touch_run4(int i, int how);
static int pz_next_run2(int *a);
static int pz_next_run3(int *a);
static int pz_next_run4(int *a);
static int pz_wants(void);
static int pz_atx = -1, pz_aty = -1;                               /* where the dream will stand when it next moves: it decides a cell ahead */
static void pz_return(float x, float y) {                          /* sent back (a stone sank): a person, or the dream set going again */
    G.px = x; G.py = y;
    if (G.player) return;
    G.cx = G.tx = (int)x; G.cy = G.ty = (int)y; G.u = 1.0f; G.pz_stepx = -1;
    G.next_h = decide(G.cx, G.cy, G.h);
    begin_turn(heading_angle(G.next_h)); G.state = PS_TURN;
}
static int pz_set_of(int kind) { for (int s = 0; s < 4; s++) for (int k = 0; k < 7; k++) if (PZ_RUN[s][k] == kind) return s; return 3; }
/* the level's puzzle, laid out once the level is made: its pieces near the wing's door, its hoard behind it */
static void pz_make(void) {
    G.pz_solved = G.pz_tries = G.pz_hinted = 0; G.pz_step = G.pz_nseq = 0; G.pz_wait = 0.0f; G.pz_t = 0.0f;
    G.pz_answer[0] = G.pz_hint[0] = 0;
    memset(G.pz_a, 0, sizeof G.pz_a);
    if (G.pz < 0 || G.pz_door < 0) { G.pz = -1; return; }
    const Door *dr = &G.doors[G.pz_door];
    G.pz_wx = G.pz_wy = -1;
    for (int d = 0; d < 4; d++) { int x = dr->x + DX[d], y = dr->y + DY[d]; if (is_open(x, y) && !in_wing(x, y)) { G.pz_wx = x; G.pz_wy = y; } }
    if (G.pz_wx < 0) { G.pz = -1; return; }
    pz_spread();
    int diff = pz_diff(), set = pz_set_of(G.pz);
    int n0 = G.nthing;
    if (G.pz == PZ_SQUARE || G.pz == PZ_LETTERS) pz_make_run4(G.pz, diff);
    else if (set == 0 || G.pz == PZ_DBELLS) pz_make_run1(G.pz, diff);
    else if (set == 1) pz_make_run2(G.pz, diff);
    else if (set == 2) pz_make_run3(G.pz, diff);
    else pz_make_run4(G.pz, diff);
    if (G.nthing == n0) { G.pz = -1; G.doors[G.pz_door].locked = 0; return; }   /* no room for it: the wing stands open */
    static const int HOARD[8] = { IT_CHEST, IT_CROWN, IT_GEMS, IT_GEMS, IT_CHALICE, IT_COINS, IT_CROWN, IT_GEMS };   /* as a vault's (P7) */
    int x, y;
    for (int i = 0; i < 6 + diff; i++) if (room_cell(G.pz_room, &x, &y)) { Item *it = add_item(x, y, HOARD[i % 8]); if (it) { it->secret = 1; G.items_total--; G.value_total -= treasure_value(HOARD[i % 8]); } }
    if (rnd(3) == 0 && room_cell(G.pz_room, &x, &y)) { Item *it = add_item(x, y, IT_POTION); if (it) it->secret = 1; }   /* now and then health */
#ifdef CRAWL_DEBUG
    fprintf(stderr, "PUZZLE level %d: %s diff %d room %d door (%d,%d) exit inside %d, %d things: %s\n", G.level, PZ_NAME[G.pz], diff, G.pz_room,
            dr->x, dr->y, G.pz_exit, G.nthing - n0, G.pz_answer);
    if (getenv("CRAWL_PZTHINGS"))                                  /* each of its things, where (1.0.0b18.1) */
        for (int i = n0; i < G.nthing; i++) fprintf(stderr, "PUZZLE thing %d %s at (%.2f,%.2f) z %.2f scale %.2f\n", i, G.thing[i].name, G.thing[i].x, G.thing[i].y, G.thing[i].z, G.thing[i].scale);
#endif
}
/* stacking: an item laid on top of a pile */
static void thing_pile(int item, int place) {
    Thing *it = &G.thing[item], *p = &G.thing[place];
    int below = p->in, n = pile_count(place);
    it->below = (int8_t)below; it->in = (int8_t)place; p->in = (int8_t)item;
    it->x = p->x; it->y = p->y; it->cx = p->cx; it->cy = p->cy;
    float h = 0.0f;
    for (int k = below, c = 0; k >= 0 && c < n; k = G.thing[k].below, c++) h += 0.16f * G.thing[k].scale;
    it->z = p->z + p->hold + h;
}
static int pz_takes(int place, int item) {
    const Thing *p = &G.thing[place], *it = &G.thing[item];
    if (p->tri || it->tri) return p->tri && it->tri;               /* a trial's go only with a trial's */
    if (G.pz == PZ_URNS && p->in >= 0 && G.thing[p->in].val < it->val) return 0;   /* never a larger on a smaller */
    if (G.pz == PZ_FOSSIL && p->want != it->val) return 0;        /* a bone fits only its own socket */
    if (p->grp == 1 && it->grp != 2) return 0;
    return 1;
}
static void pz_touched(int i, int how) {
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT thing %d role %d how %d state %d by %s t=%.2f\n", i, G.thing[i].role, how, G.thing[i].state, G.player ? "player" : "autopilot", G.t);
#endif
    if (G.thing[i].tri) { tri_touched(i, how); return; }           /* a trial's thing */
    if (G.pz < 0 || G.pz_solved) return;
    int set = pz_set_of(G.pz);
    if (G.pz == PZ_SQUARE || G.pz == PZ_LETTERS || set == 3) pz_touch_run4(i, how);
    else if (set == 0 || G.pz == PZ_DBELLS) pz_touch_run1(i, how);
    else if (set == 1) pz_touch_run2(i, how);
    else pz_touch_run3(i, how);
}
static int pz_next(int *a) {
    *a = -1;
    if (G.pz < 0 || G.pz_solved) return PA_NONE;
    int set = pz_set_of(G.pz);
    if (G.pz == PZ_SQUARE || G.pz == PZ_LETTERS || set == 3) return pz_next_run4(a);
    if (set == 0 || G.pz == PZ_DBELLS) return pz_next_run1(a);
    if (set == 1) return pz_next_run2(a);
    return pz_next_run3(a);
}
static void pz_update(float dt) {                                  /* what runs by itself: a phrase playing, a crust cooling */
    if (G.pz < 0 || G.pz_solved || PK.depth || G.shop) return;
    pz_dt = dt;
    for (int i = 0; i < G.nthing; i++) {                           /* the dream keeps off stones that sink and glass already lit */
        const Thing *t = &G.thing[i];
        if (t->role != TR_PLATE) continue;
        if (pz_wants()) break;                                     /* working it: it goes by the way it knows */
        if (G.pz == PZ_GLASS || (G.pz == PZ_FERRY && !t->want)) hz_mark(t->cx, t->cy, HZ_SOFT);
    }
    if (G.pz_wait > 0.0f) G.pz_wait -= dt;
    if (pz_set_of(G.pz) == 3 || G.pz == PZ_SQUARE || G.pz == PZ_LETTERS) pz_touch_run4(-1, -1);   /* their clocks */
    else if (pz_set_of(G.pz) == 1) pz_touch_run2(-1, -1);
}

/* ---- the dream at a puzzle (P6): when it has time, or when the way on lies beyond, it works the puzzle out, a move at
 * a time with a pause between, and now and then a wrong one; it reads the answer from the game, never the clues ---- */
static int pz_wants(void) {
    if (G.player || G.pz < 0 || G.pz_solved || PK.depth || G.shop || G.pz_wx < 0) return 0;
    if (G.pz_exit) return 1;                                       /* the way on: whatever its clock says */
    return G.level_t < 210.0 && G.hp * 3 >= G.max_hp * 2;
}
static int pz_goal(int x, int y, int *gx, int *gy) {
    G.pz_stepx = -1;
    if (!pz_wants() || G.pz_wait > 0.0f) return 0;
    pz_atx = x; pz_aty = y;
    int a, act = pz_next(&a);
    pz_atx = pz_aty = -1;
    if (act == PA_NONE || act == PA_WAIT) { if (G.pz_exit && act == PA_WAIT) { *gx = G.pz_wx; *gy = G.pz_wy; return apass(*gx, *gy); } return 0; }
    if (act == PA_DROP) { thing_set_down(); G.pz_wait = 0.6f; return 0; }
    if (act == PA_STEP) { *gx = a % MAPN; *gy = a / MAPN; G.use_kind = USE_NONE; G.pz_stepx = *gx; G.pz_stepy = *gy; return 1; }
    if (a < 0) return 0;
    if (rnd(11) == 0 && !G.carry) {                                /* now and then, a wrong one */
        int k = rnd(G.nthing);
        if (G.thing[k].role != TR_ITEM && G.thing[k].role != TR_PLATE && G.thing[k].role != TR_READ && G.thing[k].grp) a = k;
    }
    const Thing *t = &G.thing[a];
    int sx, sy, tx = t->cx, ty = t->cy;                            /* from the floor it stands on (a wall's thing: the cell before the wall) */
    if (!use_spot(tx, ty, &sx, &sy) && !(apass(t->cx, t->cy) && (sx = t->cx, sy = t->cy, 1))) return 0;
    bfs_from(x, y);
    if (bfs_dist[sy][sx] < 0) return 0;
    G.use_x = tx; G.use_y = ty; G.use_kind = USE_THING; G.use_thing = a;
    *gx = sx; *gy = sy;
    return 1;
}

