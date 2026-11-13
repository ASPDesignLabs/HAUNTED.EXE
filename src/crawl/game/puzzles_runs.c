/* A part of scene_crawl.c (docs/PLAN-tidy.md): it is #included there in its place and never compiled on its own.
 * The Catacombs' puzzles, the Hollows', the Deep's, and the two of any run. */

/* ---- the Catacombs' ---- */
/* channels set in the floor, turning a quarter (the Culverts, and the Deep's Gold Veins): open sides as bits, east 1, south 2,
 * west 4, north 8 (DX/DY's order); a straight piece or an elbow, turned */
static int pipe_mask(int shape, int rot) { int m = shape ? 3 : 5; for (int r = 0; r < (rot & 3); r++) m = ((m << 1) | (m >> 3)) & 15; return m; }
/* water (or gold) from the spout into the first piece, along whatever joins, out of the last to the wheel: how far it gets */
static int pipe_flow(uint32_t *wet) {
    int w = G.pz_a[0], h = G.pz_a[1], r = G.pz_a[2], x = 0, y = r, from = 2, n = 0;   /* in from the west */
    if (wet) *wet = 0;
    while (x >= 0 && y >= 0 && x < w && y < h && n < 16) {
        const Thing *t = &G.thing[G.pz_a[4 + y * w + x]];
        int m = pipe_mask(t->val, t->state);
        if (!(m >> from & 1)) return 0;
        if (wet) *wet |= 1u << (y * w + x);
        int out = -1;
        for (int d = 0; d < 4; d++) if (d != from && (m >> d & 1)) out = d;
        if (out < 0) return 0;
        x += DX[out]; y += DY[out]; from = (out + 2) & 3; n++;
        if (x == w && y == G.pz_a[3] && out == 0) return 1;       /* out to the wheel */
    }
    return 0;
}
static int pipe_ok(int i) { const Thing *t = &G.thing[i]; return t->want < 0 || pipe_mask(t->val, t->state) == pipe_mask(t->val, t->want); }
static int pipes_next(int *a) {
    for (int k = 0; k < G.pz_a[0] * G.pz_a[1]; k++) { int i = G.pz_a[4 + k]; if (!pipe_ok(i)) { *a = i; return PA_USE; } }
    return PA_NONE;
}
/* the grid and its path: from the spout's row on the west to the wheel's on the east, wandering; each piece of the path is
 * the shape its turns want, the rest any */
static int pipes_lay(int w, int h, int look) {
    int bx = -1, by = -1, bd = 1 << 20;
    for (int y = 1; y < MAPN - h - 1; y++)
        for (int x = 2; x < MAPN - w - 2; x++) {
            if (pz_dist[y][x] < 1 || pz_dist[y][x] >= bd) continue;
            int ok = 1;
            for (int k = 0; k < w * h && ok; k++) ok = pz_free(x + k % w, y + k / w) && room_kind_at(x + k % w, y + k / w) >= 0;
            for (int k = 0; k < h && ok; k++) ok = hz_floor(x - 1, y + k) && hz_floor(x + w, y + k);   /* room for the spout and the wheel */
            if (ok) { bx = x; by = y; bd = pz_dist[y][x]; }
        }
    if (bx < 0) return 0;
    int path[16], np = 0, cx = 0, cy = rnd(h), seen = 0;
    path[np++] = cy * w;
    seen |= 1 << (cy * w);
    while (cx < w - 1 || (np > 1 && rnd(3) == 0 && 0)) {          /* east, with a turn up or down now and then */
        int dirs[3] = { 0, 1, 3 }, ok = 0;
        shuffle_int(dirs, 3);
        if (rnd(2)) dirs[0] = 0;
        for (int k = 0; k < 3 && !ok; k++) {
            int nx = cx + DX[dirs[k]], ny = cy + DY[dirs[k]];
            if (nx < 0 || ny < 0 || nx >= w || ny >= h || (seen >> (ny * w + nx) & 1)) continue;
            cx = nx; cy = ny; ok = 1;
        }
        if (!ok) break;
        path[np++] = cy * w + cx; seen |= 1 << (cy * w + cx);
    }
    G.pz_a[0] = w; G.pz_a[1] = h; G.pz_a[2] = path[0] / w; G.pz_a[3] = path[np - 1] / w;
    for (int k = 0; k < w * h; k++) {
        int x = bx + k % w, y = by + k / w;
        pz_claim(x, y);
        int i = thing_new(TR_TURN, x + 0.5f, y + 0.5f, -1);
        if (i < 0) return 0;
        Thing *t = &G.thing[i];
        t->grp = 1; t->nstate = 4; t->val = rnd(2); t->want = -1; t->state = (uint8_t)rnd(4); t->look = look;
        snprintf(t->name, sizeof t->name, "%s", look ? "A TILE OF GOLD VEINS" : "A CULVERT");
        G.pz_a[4 + k] = i;
    }
    for (int p = 0; p < np; p++) {                                 /* the path's pieces: the shape and turn its ends need */
        int k = path[p], x = k % w, y = k / w;
        int in = p == 0 ? 2 : -1, out = p == np - 1 ? 0 : -1;
        for (int d = 0; d < 4; d++) {
            if (p > 0 && path[p - 1] == (y + DY[d]) * w + x + DX[d] && x + DX[d] >= 0 && x + DX[d] < w) in = d;
            if (p < np - 1 && path[p + 1] == (y + DY[d]) * w + x + DX[d] && x + DX[d] >= 0 && x + DX[d] < w) out = d;
        }
        int need = (1 << in) | (1 << out), shape = (in + 2) % 4 == out ? 0 : 1, rot = 0;
        while (pipe_mask(shape, rot) != need && rot < 4) rot++;
        Thing *t = &G.thing[G.pz_a[4 + k]];
        t->val = shape; t->want = rot;
        if (pipe_mask(shape, t->state) == need) t->state = (uint8_t)((rot + 1) & 3);   /* never set right to begin with */
    }
    int sp = thing_new(TR_READ, bx - 0.5f + 0.35f, by + G.pz_a[2] + 0.5f, look ? SP_PZ_IDOL : SP_PZ_SPOUT);   /* the spout (or the idol) on the west */
    if (sp >= 0) { snprintf(G.thing[sp].name, sizeof G.thing[sp].name, "%s", look ? "THE IDOL'S EYES" : "THE SPOUT"); snprintf(G.thing[sp].text, sizeof G.thing[sp].text, "%s", look ? "GOLD RUNS FROM THE IDOL'S EYES INTO THE VEINS, AND MUST REACH THE LOCK." : "WATER POURS FROM THE SPOUT INTO THE CULVERTS. IT MUST REACH THE MILL WHEEL."); G.thing[sp].scale = 0.7f; }
    int wh = thing_new(TR_READ, bx + w + 0.5f - 0.35f, by + G.pz_a[3] + 0.5f, look ? SP_PZ_LOCK : SP_PZ_MILL0);
    if (wh >= 0) { snprintf(G.thing[wh].name, sizeof G.thing[wh].name, "%s", look ? "THE DOOR'S LOCK" : "THE MILL WHEEL"); snprintf(G.thing[wh].text, sizeof G.thing[wh].text, "%s", look ? "A LOCK WITH A CHANNEL CUT INTO IT, DRY." : "A MILL WHEEL THAT LIFTS THE SLUICE GATE, WAITING ON THE WATER."); G.thing[wh].frames = 2; G.thing[wh].scale = 0.8f; G.pz_a[30] = wh; }
    return 1;
}
/* the Three Cisterns: eight, five and three measures, the first full; three sluices pour round */
static int cis_cap(int k) { static const int C[3] = { 8, 5, 3 }; return C[k]; }
static void cis_show(void) {                                       /* what each says, read */
    static const char *const NUM[9] = { "NOTHING", "ONE MEASURE", "TWO MEASURES", "THREE MEASURES", "FOUR MEASURES", "FIVE MEASURES", "SIX MEASURES", "SEVEN MEASURES", "EIGHT MEASURES" };
    static const char *const NM[3] = { "THE GREAT CISTERN, OF EIGHT", "THE MIDDLE CISTERN, OF FIVE", "THE LEAST CISTERN, OF THREE" };
    for (int k = 0; k < 3; k++) {
        Thing *t = &G.thing[G.pz_a[k]];
        snprintf(t->text, sizeof t->text, "%s, HOLDS %s.", NM[k], NUM[clampi(G.pz_a[3 + k], 0, 8)]);
        t->state = (uint8_t)clampi((G.pz_a[3 + k] * 8 + cis_cap(k) / 2) / cis_cap(k), 0, 8);
    }
}
static int cis_goal(const int *v) { return G.pz_a[6] ? v[0] == 4 && v[1] == 4 : v[0] == 4 || v[1] == 4; }   /* harder: four and four */
static int cisterns_next(int *a) {
    int seen[9 * 6 * 4], q[200], qh = 0, qt = 0, first[200];
    memset(seen, -1, sizeof seen);
    int s0 = G.pz_a[3] * 24 + G.pz_a[4] * 4 + G.pz_a[5];
    seen[s0] = 0; q[qt] = s0; first[qt++] = -1;
    while (qh < qt) {
        int st = q[qh], f = first[qh++], v[3] = { st / 24, (st / 4) % 6, st % 4 };
        if (cis_goal(v)) { if (f < 0) return PA_NONE; *a = G.pz_a[7 + f]; return PA_USE; }
        for (int l = 0; l < 3; l++) {
            int from = l, to = (l + 1) % 3, w[3] = { v[0], v[1], v[2] }, m = imin(w[from], cis_cap(to) - w[to]);
            w[from] -= m; w[to] += m;
            int ns = w[0] * 24 + w[1] * 4 + w[2];
            if (seen[ns] >= 0 || qt >= 200) continue;
            seen[ns] = 1; q[qt] = ns; first[qt++] = f < 0 ? l : f;
        }
    }
    return PA_NONE;
}
/* the saints and their emblems */
static const char *const SAINT_LINE[6] = { "SHE KEPT THE GATE, AND ITS KEY", "BROKEN ON THE WHEEL", "SHOT FOR THE FAITH", "SHUT IN A TOWER",
                                           "SHE WENT AS GENTLY AS THE LAMB TO HER DEATH", "SHE KEPT HER LAMP TRIMMED THROUGH THE NIGHT" };
static const char *const SAINT_NAME[6] = { "SAINT AGNES", "SAINT CATHERINE", "SAINT SEBASTIAN", "SAINT BARBARA", "SAINT AGATHA", "SAINT URSULA" };
/* a strip of floor three across and some long, on a lane, for stepping on */
static int strip_find(int len, int *ox, int *oy, int *oax) {
    int best = -1, bd = 1 << 20, bax = 0;
    for (int y = 2; y < MAPN - 2; y++)
        for (int x = 2; x < MAPN - 2; x++) {
            if (pz_dist[y][x] < 2 || pz_dist[y][x] >= bd || !lane(x, y)) continue;
            for (int ax = 0; ax < 2; ax++) {                       /* ax: the way along it */
                int bxx = ax ^ 1, ok = 1;
                if (ax == 0 ? (y & 3) != 2 : (x & 3) != 2) continue;   /* its middle row on a lane */
                for (int i = 0; i < len && ok; i++)
                    for (int j = -1; j <= 1 && ok; j++) { int sx = x + DX[ax] * i + DX[bxx] * j, sy = y + DY[ax] * i + DY[bxx] * j; ok = pz_free(sx, sy) && room_kind_at(sx, sy) >= 0; }
                int ex = x - DX[ax], ey = y - DY[ax], fx = x + DX[ax] * len, fy = y + DY[ax] * len;   /* a way on and off at each end */
                ok = ok && hz_floor(ex, ey) && apass(ex, ey) && hz_floor(fx, fy) && apass(fx, fy);
                if (ok) { best = y * MAPN + x; bd = pz_dist[y][x]; bax = ax; }
            }
        }
    if (best < 0) return 0;
    *ox = best % MAPN; *oy = best / MAPN; *oax = bax;
    return 1;
}
static void pz_make_run2(int kind, int diff) {
    char *ans = G.pz_answer, *hint = G.pz_hint;
    switch (kind) {
    case PZ_CISTERNS: {
        static const char *const NM[3] = { "THE GREAT CISTERN", "THE MIDDLE CISTERN", "THE LEAST CISTERN" };
        static const char *const LN[3] = { "THE SLUICE FROM THE GREAT TO THE MIDDLE", "THE SLUICE FROM THE MIDDLE TO THE LEAST", "THE SLUICE FROM THE LEAST TO THE GREAT" };
        for (int k = 0; k < 3; k++) {
            int c = pz_wallthing(TR_READ, SP_PZ_CISTERN0, 1, NM[k]);
            if (c < 0) return;
            G.thing[c].frames = 9; G.thing[c].grp = 0; G.thing[c].mark = (int16_t)(17 + cis_cap(k) - 1); G.thing[c].mz = 0.3f;
            G.pz_a[k] = c;
        }
        for (int k = 0; k < 3; k++) {
            int l = pz_wallthing(TR_LEVER, SP_PZ_LEVER0, 1, LN[k]);
            if (l < 0) return;
            G.thing[l].frames = 2; G.thing[l].grp = 0; G.pz_a[7 + k] = l;
        }
        G.pz_a[3] = 8; G.pz_a[4] = G.pz_a[5] = 0; G.pz_a[6] = diff >= 2;
        cis_show();
        pz_reading(SP_PZ_TABLET, 1, "A CARVING BY THE CULVERT", G.pz_a[6] ? "THE CULVERT OPENS WHEN THE GREAT CISTERN AND THE MIDDLE EACH HOLD FOUR MEASURES."
                                                                                : "THE CULVERT OPENS WHEN A CISTERN HOLDS FOUR MEASURES, NEITHER MORE NOR LESS.");
        snprintf(ans, 200, "%s", G.pz_a[6] ? "POUR GREAT TO MIDDLE, MIDDLE TO LEAST, LEAST TO GREAT, AND ROUND AGAIN, TILL THE GREAT AND THE MIDDLE HOLD FOUR EACH."
                                         : "POUR GREAT TO MIDDLE, MIDDLE TO LEAST, LEAST TO GREAT, AND ROUND AGAIN, TILL ONE HOLDS FOUR.");
        snprintf(hint, 100, "EACH SLUICE POURS TILL ONE IS FULL OR THE OTHER EMPTY");
        break;
    }
    case PZ_SAINTS: {
        int ord[6] = { 0, 1, 2, 3, 4, 5 }, n = imin(6, 3 + diff);
        shuffle_int(ord, 6);
        snprintf(ans, 200, "EACH SAINT HER EMBLEM:");
        for (int k = 0; k < n; k++) {
            int e = ord[k], p = pz_wallthing(TR_PLACE, SP_PZ_SAINT, 1, SAINT_NAME[e]);
            if (p < 0) break;
            Thing *t = &G.thing[p];
            t->grp = 1; t->want = e; t->hold = 0.32f;
            snprintf(t->text, sizeof t->text, "%s: %s", SAINT_NAME[e], SAINT_LINE[e]);
            char nm[40];
            snprintf(nm, sizeof nm, "AN EMBLEM: A %s", MARK_WORD[4 + e]);
            int it = pz_lying(TR_ITEM, SP_PZ_EMBLEM, 4, nm);
            if (it < 0) break;
            G.thing[it].grp = 2; G.thing[it].val = e; G.thing[it].mark = (int16_t)(4 + e + 1); G.thing[it].mz = -0.28f;
            pz_append(ans, 200, " THE %s TO %s,", MARK_WORD[4 + e], SAINT_NAME[e] + 6);
        }
        snprintf(hint, 100, "READ HOW EACH SAINT DIED; THE EMBLEM SAYS THE SAME");
        break;
    }
    case PZ_CULVERTS: case PZ_VEINS: {
        int veins = kind == PZ_VEINS;
        if (!pipes_lay(3 + (diff >= 2), 3, veins)) return;
        snprintf(ans, 200, "%s", veins ? "TURN THE TILES SO ONE VEIN OF GOLD RUNS UNBROKEN FROM THE IDOL'S EYES TO THE LOCK. A GLINT SHOWS WHERE THE GOLD HAS REACHED."
                                       : "TURN THE CULVERTS SO ONE CHANNEL RUNS UNBROKEN FROM THE SPOUT TO THE MILL WHEEL. THE WATER SHOWS HOW FAR IT HAS COME.");
        snprintf(hint, 100, "FOLLOW THE WATER: TURN THE PIECE WHERE IT STOPS");
        if (veins) snprintf(hint, 100, "FOLLOW THE GOLD: TURN THE TILE WHERE IT STOPS");
        break;
    }
    case PZ_FERRY: case PZ_GLASS: {
        int glass = kind == PZ_GLASS, len = glass ? 4 : 4 + diff, x0, y0, ax;   /* the glass: three by four, so a walk over all of it can begin in the middle */
        if (!strip_find(len, &x0, &y0, &ax)) return;
        int bxx = ax ^ 1, path[24], np = 0, j = 0;
        G.pz_a[0] = len; G.pz_a[1] = ax; G.pz_a[2] = x0; G.pz_a[3] = y0;
        if (glass) {                                               /* a walk over every tile once, from the near middle, found by search */
            int seen = 1 << 1, st[24], sd[24], depth = 0;
            st[0] = 1; sd[0] = 0;
            while (depth >= 0 && depth < 3 * len - 1) {
                int c = st[depth], moved = 0;
                while (sd[depth] < 4 && !moved) {
                    int d = sd[depth]++, ci = c / 3 + DX[d], ck = c % 3 + DY[d];   /* (column, cell across) */
                    if (ci < 0 || ci >= len || ck < 0 || ck > 2 || (seen >> (ci * 3 + ck) & 1)) continue;
                    depth++; st[depth] = ci * 3 + ck; sd[depth] = (int)(rndf() * 0.0f); seen |= 1 << st[depth]; moved = 1;
                }
                if (!moved) { seen &= ~(1 << c); depth--; }
            }
            for (int k = 0; k <= depth && depth >= 0; k++) path[np++] = st[k];
        } else {                                                   /* a way across, stepping aside now and then */
            for (int i = 0; i < len; i++) {
                path[np++] = i * 3 + j + 1;
                if (i < len - 1 && rnd(2)) { int nj = clampi(j + (rnd(2) ? 1 : -1), -1, 1); if (nj != j) { j = nj; path[np++] = i * 3 + j + 1; } }
            }
        }
        for (int i = 0; i < len; i++)
            for (int k = 0; k < 3; k++) {
                int x = x0 + DX[ax] * i + DX[bxx] * (k - 1), y = y0 + DY[ax] * i + DY[bxx] * (k - 1);
                pz_claim(x, y);
                int s = thing_new(TR_PLATE, x + 0.5f, y + 0.5f, -1);
                if (s < 0) return;
                Thing *t = &G.thing[s];
                t->grp = 1; t->val = i * 3 + k; t->look = glass ? 2 : 1;
                t->want = 0;
                for (int p = 0; p < np; p++) if (path[p] == i * 3 + k) t->want = 1 + p;   /* safe, and its place on the way */
                t->mark = glass ? 0 : (int16_t)(1 + (4 + rnd(6)));
                G.pz_a[4 + i * 3 + k] = s;
            }
        G.pz_a[29] = np; for (int p = 0; p < np && p < 24; p++) G.pz_seq[p % 16] = path[p];
        G.pz_nseq = imin(np, 16); G.pz_step = 0;
        if (glass) {
            snprintf(ans, 200, "STEP ON EVERY TILE ONCE: GO UP ONE ROW AND DOWN THE NEXT, AS A PLOUGH GOES.");
            pz_reading(SP_PZ_TABLET, 1, "A GOLD TABLET", "EVERY TILE OF THE GLASS FLOOR LIT, AND NONE TROD TWICE: SO THE COURT WALKED BEFORE ITS KING.");
            snprintf(hint, 100, "PLAN THE WHOLE WALK BEFORE THE FIRST STEP");
        } else {
            char txt[160];
            snprintf(txt, sizeof txt, "THE FERRYMAN'S WAY, STONE BY STONE:");
            snprintf(ans, 200, "THE STONES THAT HOLD, FROM THE NEAR SIDE:");
            for (int p = 0; p < np; p++) {
                int s = G.pz_a[4 + path[p]];
                pz_append(txt, 160, " %s", MARK_WORD[G.thing[s].mark - 1]);
                pz_append(ans, 200, " %s", MARK_WORD[G.thing[s].mark - 1]);
            }
            pz_reading(SP_PZ_TABLET, 1, "A DROWNED TABLET", txt);
            snprintf(hint, 100, "ONLY THE SIGNS ON THE TABLET HOLD, IN ITS ORDER");
        }
        break;
    }
    case PZ_SLUICELOCK: {                                          /* three wheels of the saints' signs, set by the flood line */
        int ord[6] = { 0, 1, 2, 3, 4, 5 };
        shuffle_int(ord, 6);
        char txt[160];
        snprintf(txt, sizeof txt, "THE FLOOD LINES ON THE SAINTS' ALCOVES, THE HIGHEST WATER FIRST:");
        snprintf(ans, 200, "THE WHEELS, FIRST TO LAST:");
        for (int k = 0; k < 3; k++) {
            char nm[40];
            snprintf(nm, sizeof nm, "THE %s WHEEL", NTH[k]);
            int w = pz_wallthing(TR_TURN, SP_PZ_WHEEL, 1, nm);
            if (w < 0) return;
            Thing *t = &G.thing[w];
            t->grp = 1; t->nstate = 6; t->want = ord[k]; t->state = (uint8_t)((ord[k] + 1 + rnd(5)) % 6); t->mark = (int16_t)(4 + t->state + 1); t->mz = 0.0f;
            pz_append(txt, 160, " %s%s", MARK_WORD[4 + ord[k]], k < 2 ? "," : ".");
            pz_append(ans, 200, " %s", MARK_WORD[4 + ord[k]]);
        }
        if (diff >= 1) {                                           /* deeper: the saints' marks, not their signs */
            snprintf(txt, sizeof txt, "THE FLOOD LINES, THE HIGHEST WATER FIRST:");
            for (int k = 0; k < 3; k++) pz_append(txt, 160, " %s%s", SAINT_NAME[G.thing[G.nthing - 3 + k].want], k < 2 ? "," : ".");
        }
        pz_reading(SP_PZ_TABLET, 1, "THE FLOOD LINES", txt);
        if (diff >= 1) pz_reading(SP_PZ_TABLET, 1, "THE SAINTS' SIGNS", "AGNES THE KEY, CATHERINE THE WHEEL, SEBASTIAN THE ARROWS, BARBARA THE TOWER, AGATHA THE LAMB, URSULA THE LAMP.");
        snprintf(hint, 100, "THE WHEELS GO IN THE ORDER OF THE WATER, HIGHEST FIRST");
        break;
    }
    default: break;
    }
}
static void pz_touch_run2(int i, int how) {
    if (i < 0) {                                                   /* each frame: the stepping puzzles' lights, the water */
        if (G.pz == PZ_CULVERTS && G.pz_a[30] && pipe_flow(NULL)) { G.thing[G.pz_a[30]].state = (uint8_t)(((int)(G.t * 3.0)) & 1); }
        return;
    }
    Thing *t = &G.thing[i];
    switch (G.pz) {
    case PZ_CISTERNS:
        for (int l = 0; l < 3; l++) if (i == G.pz_a[7 + l] && how == TU_PULL) {
            int from = l, to = (l + 1) % 3, m = imin(G.pz_a[3 + from], cis_cap(to) - G.pz_a[3 + to]);
            G.pz_a[3 + from] -= m; G.pz_a[3 + to] += m;
            sfx_at(SFX_SPLASH, t->x, t->y, 1.2f); sfx_at(SFX_DRIP, t->x, t->y, 0.8f);
            cis_show();
            int v[3] = { G.pz_a[3], G.pz_a[4], G.pz_a[5] };
            if (cis_goal(v)) pz_solve();
            t->state = 0;
        }
        break;
    case PZ_SAINTS:
        if (how == TU_SET && t->grp == 2) { if (match_done()) pz_solve(); else if (match_full()) { say("THE SAINTS DO NOT STIR"); pz_wrong(); } }
        break;
    case PZ_CULVERTS:
        if (how == TU_TURN && pipe_flow(NULL)) { sfx_at(SFX_SPLASH, t->x, t->y, 1.4f); pz_solve(); }
        break;
    case PZ_SLUICELOCK:
        if (how == TU_TURN) {
            t->mark = (int16_t)(4 + t->state + 1);
            say("%s: THE %s", t->name, MARK_WORD[4 + t->state]);
            if (settings_done()) pz_solve();
        }
        break;
    case PZ_FERRY:
        if (how == TU_STEP && t->grp == 1) {
            if (!t->want) {                                        /* it sinks: back to the near side, cold but unhurt */
                say("THE STONE SINKS UNDER YOU");
                sfx_at(SFX_SPLASH, t->x, t->y, 1.6f);
                int ax = G.pz_a[1];
                float bx = G.pz_a[2] - DX[ax] + 0.5f, by = G.pz_a[3] - DY[ax] + 0.5f;
                pz_return(bx, by);
                G.slow_t = fmaxf(G.slow_t, 1.5f);
                pz_wrong();
            } else if (t->val / 3 == G.pz_a[0] - 1) pz_solve();    /* the far row reached */
        }
        break;
    case PZ_GLASS:
        if (how == TU_STEP && t->grp == 1) {
            if (t->state) {                                        /* a lit tile again: it cracks, and the floor goes dark */
                say("THE GLASS CRACKS, AND THE FLOOR GOES DARK");
                sfx_at(SFX_SHATTER, t->x, t->y, 1.0f);
                for (int k = 0; k < G.nthing; k++) if (G.thing[k].role == TR_PLATE) G.thing[k].state = 0;
                pz_wrong();
            } else {
                t->state = 1;
                snd_tone(1, 72 + (t->val % 12), 0.4f, 0.5f);
                int all = 1;
                for (int k = 0; k < G.nthing; k++) if (G.thing[k].role == TR_PLATE && !G.thing[k].state) all = 0;
                if (all) pz_solve();
            }
        }
        break;
    default: break;
    }
}
/* the next stone on the way, from where the dream stands; off the strip, the first */
static int strip_next(int *a, int glass) {
    int px = pz_atx >= 0 ? pz_atx : (int)G.px, py = pz_atx >= 0 ? pz_aty : (int)G.py, at = -1;
    for (int k = 0; k < G.nthing; k++) if (G.thing[k].role == TR_PLATE && G.thing[k].cx == px && G.thing[k].cy == py) at = k;
    int ex = G.pz_a[2] - DX[G.pz_a[1]], ey = G.pz_a[3] - DY[G.pz_a[1]];   /* the near end, off the stones */
    if (at < 0 && (px != ex || py != ey)) {
        int lit = 0;
        if (glass) for (int k = 0; k < G.nthing; k++) lit += G.thing[k].role == TR_PLATE && G.thing[k].state;
        if (!lit) { *a = ey * MAPN + ex; return PA_STEP; }
    }
    int step = at >= 0 ? G.thing[at].want : 0, lit = 0, stray = -1;
    if (glass) for (int k = 0; k < G.nthing; k++) if (G.thing[k].role == TR_PLATE && G.thing[k].state) lit++;
    if (glass) for (int k = 0; k < G.nthing; k++) if (G.thing[k].role == TR_PLATE && G.thing[k].state && G.thing[k].want > lit + 1) stray = k;   /* lit out of the walk's order */
    if (glass && stray >= 0 && at != stray) { *a = G.thing[stray].cy * MAPN + G.thing[stray].cx; return PA_STEP; }   /* out of order: start again */
    if (glass) step = at >= 0 ? G.thing[at].want : lit;            /* on the walk: what follows the tile it is stepping onto */
    if (at >= 0 && !G.thing[at].want && !glass) step = 0;
    for (int k = 0; k < G.nthing; k++)
        if (G.thing[k].role == TR_PLATE && G.thing[k].want == step + 1) {
            if (glass && G.thing[k].state) continue;
            *a = G.thing[k].cy * MAPN + G.thing[k].cx;
            return PA_STEP;
        }
    return PA_NONE;
}
/* the glass floor: from the tile it stands on (or is stepping onto), a walk over every tile still dark, found by search; if none
 * can be, a lit tile beside it to start again */
static int glass_walk(int from, int dark, int left, int *first, int depth) {
    if (!left) return 1;
    int len = G.pz_a[0];
    for (int d = 0; d < 4; d++) {
        int ci = from / 3 + DX[d], ck = from % 3 + DY[d], c = ci * 3 + ck;
        if (ci < 0 || ci >= len || ck < 0 || ck > 2 || !(dark >> c & 1)) continue;
        if (glass_walk(c, dark & ~(1 << c), left - 1, first, depth + 1)) { if (!depth) *first = c; return 1; }
    }
    return 0;
}
static int glass_next(int *a) {
    int px = pz_atx >= 0 ? pz_atx : (int)G.px, py = pz_atx >= 0 ? pz_aty : (int)G.py, at = -1, dark = 0, n = 0, len = G.pz_a[0];
    for (int k = 0; k < 3 * len; k++) {
        const Thing *t = &G.thing[G.pz_a[4 + k]];
        if (t->cx == px && t->cy == py) at = k;
        if (!t->state) { dark |= 1 << k; n++; }
    }
    if (at < 0) {                                                  /* off the glass: to the near end's tile */
        int k = 1;
        *a = G.thing[G.pz_a[4 + k]].cy * MAPN + G.thing[G.pz_a[4 + k]].cx;
        return PA_STEP;
    }
    if (dark >> at & 1) { dark &= ~(1 << at); n--; }               /* the one it is stepping onto, as good as lit */
    int first = -1;
    if (n > 0 && glass_walk(at, dark, n, &first, 0)) { *a = G.thing[G.pz_a[4 + first]].cy * MAPN + G.thing[G.pz_a[4 + first]].cx; return PA_STEP; }
    for (int d = 0; d < 4 && n > 0; d++) {                         /* no way on: a lit tile beside it, and the floor goes dark */
        int ci = at / 3 + DX[d], ck = at % 3 + DY[d], c = ci * 3 + ck;
        if (ci < 0 || ci >= len || ck < 0 || ck > 2 || (dark >> c & 1)) continue;
        *a = G.thing[G.pz_a[4 + c]].cy * MAPN + G.thing[G.pz_a[4 + c]].cx;
        return PA_STEP;
    }
    return PA_NONE;
}
static int pz_next_run2(int *a) {
    switch (G.pz) {
    case PZ_CISTERNS: return cisterns_next(a);
    case PZ_SAINTS: return match_next(a);
    case PZ_CULVERTS: return pipes_next(a);
    case PZ_SLUICELOCK: return settings_next(a);
    case PZ_FERRY: return strip_next(a, 0);
    default: return PA_NONE;
    }
}


/* ---- the Hollows' ---- */
static const char *const MINER[10] = { "ALDOUS", "BREN", "COLL", "DUNN", "EDRIC", "FENN", "GRIM", "HOBB", "IVO", "JOSS" };
static const char *const ROMAN[16] = { "", "I", "II", "III", "IV", "V", "VI", "VII", "VIII", "IX", "X", "XI", "XII", "XIII", "XIV", "XV" };
static int set_done(void) {                                        /* the board's hooks hold the dead's tags, one each, any order */
    for (int i = 0; i < G.nthing; i++) {
        if (!pz_is(i, TR_PLACE, 1)) continue;
        if (G.thing[i].in < 0 || !G.thing[G.thing[i].in].want) return 0;
    }
    return 1;
}
static int set_next(int *a) {
    if (G.carry) {
        if (!G.thing[G.carry - 1].want) return PA_DROP;
        for (int i = 0; i < G.nthing; i++) if (pz_is(i, TR_PLACE, 1) && G.thing[i].in < 0) { *a = i; return PA_USE; }
        return PA_DROP;
    }
    for (int i = 0; i < G.nthing; i++) if (pz_is(i, TR_PLACE, 1) && G.thing[i].in >= 0 && !G.thing[G.thing[i].in].want) { *a = i; return PA_USE; }
    for (int i = 0; i < G.nthing; i++) if (pz_is(i, TR_ITEM, 2) && G.thing[i].want && G.thing[i].in < 0) { *a = i; return PA_USE; }
    return PA_NONE;
}
static int hoist_next(int *a) {                                    /* the box: the answer's sacks in, the rest out */
    int box = G.pz_a[0];
    if (G.carry) { *a = G.thing[G.carry - 1].want ? box : -1; return G.thing[G.carry - 1].want ? PA_USE : PA_DROP; }
    if (G.thing[box].in >= 0 && !G.thing[G.thing[box].in].want) { *a = box; return PA_USE; }
    for (int i = 0; i < G.nthing; i++) if (pz_is(i, TR_ITEM, 2) && G.thing[i].want && !pile_has(box, i)) { *a = G.thing[i].in >= 0 ? G.thing[i].in : i; return PA_USE; }
    return PA_NONE;
}
static void pz_make_run3(int kind, int diff) {
    char *ans = G.pz_answer, *hint = G.pz_hint;
    switch (kind) {
    case PZ_POINTS: {                                              /* three switches, four buffers, the brake; the plan chalked on a board */
        int t = rnd(4), flip[3] = { diff >= 1 && rnd(2), diff >= 2 && rnd(2), diff >= 1 && rnd(2) };
        static const char *const SW[3] = { "THE HEAD SWITCH", "THE LEFT SWITCH", "THE RIGHT SWITCH" };
        for (int k = 0; k < 3; k++) {
            int s = pz_wallthing(TR_LEVER, SP_PZ_LEVER0, 1, SW[k]);
            if (s < 0) return;
            Thing *th = &G.thing[s];
            th->grp = 1; th->frames = 2; th->state = (uint8_t)rnd(2);
            int way = k == 0 ? t >> 1 : (k == 1 + (t >> 1) ? t & 1 : -1);   /* 0 left, 1 right; the switch off the way, any */
            th->want = way < 0 ? -1 : way ^ flip[k];
            G.pz_a[k] = s;
        }
        int brake = pz_wallthing(TR_LEVER, SP_PZ_LEVER0, 1, "THE CART'S BRAKE");
        if (brake < 0) return;
        G.thing[brake].frames = 2; G.pz_a[3] = brake;
        pz_standing(TR_READ, SP_CART, 1, 0, "THE CART AT THE HEAD OF THE RAILS");
        static const char *const BUF[4] = { "THE FIRST", "THE SECOND", "THE THIRD", "THE FOURTH" };
        char txt[160];
        snprintf(txt, sizeof txt, "THE MINE'S PLAN: UP SENDS THE HEAD SWITCH %s, THE LEFT %s, THE RIGHT %s. THE WINDING GEAR IS AT %s BUFFER FROM THE LEFT.",
                 flip[0] ? "RIGHT" : "LEFT", flip[1] ? "RIGHT" : "LEFT", flip[2] ? "RIGHT" : "LEFT", BUF[t]);
        pz_reading(SP_PZ_BOARD, 1, "THE MINE'S PLAN, CHALKED", txt);
        snprintf(ans, 200, "THE HEAD SWITCH %s, THE %s SWITCH %s, THEN LET OFF THE BRAKE.", G.thing[G.pz_a[0]].want ? "DOWN" : "UP", t >> 1 ? "RIGHT" : "LEFT",
                 G.thing[G.pz_a[1 + (t >> 1)]].want ? "DOWN" : "UP");
        snprintf(hint, 100, "THE CART GOES LEFT OR RIGHT AT EACH SWITCH, AS THE PLAN SAYS");
        break;
    }
    case PZ_HOIST: {                                               /* sacks tallied I to V, a box to load to the shaft's number */
        int box = pz_standing(TR_PLACE, SP_PZ_BOX, 1, 1, "THE COUNTERWEIGHT BOX");
        if (box < 0) return;
        G.thing[box].cap = 6; G.thing[box].hold = 0.18f; G.thing[box].grp = 0; G.pz_a[0] = box;
        int n = 5 + (diff >= 1), want = 2 + (diff >= 2), sum = 0;
        for (int k = 0; k < n; k++) {
            int v = 1 + rnd(5);
            char nm[40];
            snprintf(nm, sizeof nm, "A SACK OF ORE TALLIED %s", ROMAN[v]);
            int s = pz_lying(TR_ITEM, SP_PZ_SACK, 2, nm);
            if (s < 0) return;
            Thing *th = &G.thing[s];
            th->grp = 2; th->val = v; th->mark = (int16_t)(16 + v); th->mz = -0.3f; th->want = k < want;
            if (k < want) sum += v;
        }
        G.pz_a[1] = sum;
        char txt[160];
        snprintf(txt, sizeof txt, "CHALKED ON THE SHAFT: THE CAGE RISES ON %s, AND NOT ONE MORE.", ROMAN[clampi(sum, 1, 15)]);
        pz_reading(SP_PZ_BOARD, 1, "THE HOIST SHAFT", txt);
        snprintf(ans, 200, "LOAD THE BOX TO %s:", ROMAN[clampi(sum, 1, 15)]);
        for (int k = 0; k < G.nthing; k++) if (pz_is(k, TR_ITEM, 2) && G.thing[k].want) pz_append(ans, 200, " %s", ROMAN[G.thing[k].val]);
        snprintf(hint, 100, "ADD UP THE TALLIES IN THE BOX");
        break;
    }
    case PZ_TALLY: {                                               /* the shift list, the board of those who came up; the dead's tags lie with them */
        int nd = 2 + (diff >= 2), nup = 3, idx[10];
        for (int k = 0; k < 10; k++) idx[k] = k;
        shuffle_int(idx, 10);
        char list[160], up[160];
        snprintf(list, sizeof list, "THE SHIFT LIST, NAILED BY THE BOARD: DOWN WENT");
        snprintf(up, sizeof up, "TAGS HANG ON THE BOARD FOR");
        for (int k = 0; k < nd + nup; k++) pz_append(list, 160, " %s%s", MINER[idx[k]], k < nd + nup - 1 ? "," : ".");
        for (int k = nd; k < nd + nup; k++) pz_append(up, 160, " %s%s", MINER[idx[k]], k < nd + nup - 1 ? "," : ", WHO CAME UP.");
        pz_reading(SP_PZ_BOARD, 1, "THE SHIFT LIST", list);
        pz_reading(SP_PZ_BOARD, 1, "THE TALLY BOARD", up);
        snprintf(ans, 200, "HANG THE TAGS OF");
        for (int k = 0; k < nd; k++) {
            char nm[40];
            snprintf(nm, sizeof nm, "A HOOK ON THE BOARD");
            int h = pz_wallthing(TR_PLACE, SP_PZ_HOOK, 1, nm);
            if (h < 0) return;
            G.thing[h].grp = 1; G.thing[h].want = -1; G.thing[h].hold = 0.15f;
        }
        for (int k = 0; k < nd + 1 + (diff >= 1); k++) {          /* the dead's tags, and one or two of men not on the list */
            int m = k < nd ? idx[k] : idx[nd + nup + k - nd];
            char nm[40];
            snprintf(nm, sizeof nm, "THE TAG OF %s", MINER[m]);
            int tg = pz_lying(TR_ITEM, SP_PZ_TAG, 5, nm);
            if (tg < 0) return;
            G.thing[tg].grp = 2; G.thing[tg].val = m; G.thing[tg].want = k < nd;
            if (k < nd) { pz_append(ans, 200, " %s", MINER[m]); add_deco(G.thing[tg].x + 0.2f, G.thing[tg].y, SP_CARCASS); }   /* still held by its man */
        }
        pz_append(ans, 200, ", WHO NEVER CAME UP.");
        snprintf(hint, 100, "WHO WENT DOWN, AND DID NOT COME UP?");
        break;
    }
    case PZ_FOSSIL: {                                              /* a great fossil's bones, each fitting one socket by its shape */
        static const int SHAPE[6] = { 11, 15, 13, 14, 6, 10 };     /* a claw, a skull, a rib, a star of vertebra, a fork, an eye-socket */
        static const char *const BN[6] = { "A CLAW", "A GREAT SKULL", "A RIB", "A VERTEBRA", "A FORKED BONE", "A JAW" };
        int n = 3 + diff;
        if (n > 6) n = 6;
        for (int k = 0; k < n; k++) {
            int s = pz_wallthing(TR_PLACE, SP_PZ_SOCKET, 1, "A SOCKET IN THE ROCK");
            if (s < 0) return;
            Thing *th = &G.thing[s];
            th->grp = 1; th->want = k; th->mark = (int16_t)(SHAPE[k] + 1); th->mz = 0.05f; th->hold = 0.05f;
            snprintf(th->text, sizeof th->text, "A SOCKET IN THE ROCK, SHAPED FOR %s.", BN[k]);
            char nm[40];
            snprintf(nm, sizeof nm, "%s OF THE FOSSIL", BN[k]);
            int b = pz_lying(TR_ITEM, SP_PZ_FOSSIL, 3, nm);
            if (b < 0) return;
            G.thing[b].grp = 2; G.thing[b].val = k; G.thing[b].mark = (int16_t)(SHAPE[k] + 1); G.thing[b].mz = -0.3f;
        }
        snprintf(ans, 200, "EACH BONE TO THE SOCKET OF ITS SHAPE; ONLY ITS OWN WILL TAKE IT.");
        snprintf(hint, 100, "A BONE GOES ONLY WHERE ITS SHAPE IS CUT");
        break;
    }
    case PZ_RING: {                                                /* standing stones, a mark on each that turns; the litany gives their order */
        static const int MK[4] = { 10, 11, 3, 13 };                /* the eye, the hand, the crown, the moon */
        int n = imin(7, 4 + diff);
        char txt[160];
        snprintf(txt, sizeof txt, "THE CULT'S LITANY, ON THE CHAPEL WALL: FROM THE DOOR, ");
        snprintf(ans, 200, "THE STONES FROM THE WING'S DOOR:");
        int ids[7];
        for (int k = 0; k < n; k++) {
            char nm[40];
            snprintf(nm, sizeof nm, "A STANDING STONE");
            ids[k] = pz_standing(TR_TURN, SP_PZ_STONE, 1, 1, nm);
            if (ids[k] < 0) return;
        }
        for (int k = 0; k < n; k++) {                              /* nearest the door first */
            for (int j = k + 1; j < n; j++) if (pz_dist[G.thing[ids[j]].cy][G.thing[ids[j]].cx] < pz_dist[G.thing[ids[k]].cy][G.thing[ids[k]].cx]) { int q = ids[k]; ids[k] = ids[j]; ids[j] = q; }
        }
        for (int k = 0; k < n; k++) {
            Thing *th = &G.thing[ids[k]];
            th->grp = 1; th->nstate = 4; th->want = rnd(4); th->state = (uint8_t)((th->want + 1 + rnd(3)) & 3); th->mark = (int16_t)(MK[th->state] + 1); th->mz = 0.05f;
            snprintf(th->name, sizeof th->name, "THE %s STONE", NTH[k]);
            pz_append(txt, 160, "%s%s", MARK_WORD[MK[th->want]], k < n - 1 ? ", " : ".");
            pz_append(ans, 200, " %s", MARK_WORD[MK[th->want]]);
        }
        G.pz_a[0] = n;
        pz_reading(SP_PZ_TABLET, 1, "THE CULT'S LITANY", txt);
        snprintf(hint, 100, "COUNT THE STONES FROM THE DOOR, NEAREST FIRST");
        break;
    }
    case PZ_LAMPS: {                                               /* a lamp for each gallery, lit for those never reopened */
        static const char *const GAL[5] = { "I", "II", "III", "IV", "V" };
        char txt[160];
        snprintf(txt, sizeof txt, "CHALKED ON THE PROPS:");
        snprintf(ans, 200, "LIGHT THE LAMPS OF GALLERIES");
        int any = 0;
        for (int k = 0; k < 5; k++) {
            char nm[40];
            snprintf(nm, sizeof nm, "THE LAMP OF GALLERY %s", GAL[k]);
            int l = pz_wallthing(TR_LIGHT, SP_PZ_LAMP0, 1, nm);
            if (l < 0) return;
            Thing *th = &G.thing[l];
            th->grp = 1; th->frames = 2; th->want = rnd(5) < 2; th->state = (uint8_t)(rnd(3) == 0);
            if (k == 4 && !any) th->want = 1;
            any |= th->want;
            if (th->want) pz_append(ans, 200, " %s", GAL[k]);
            pz_append(txt, 160, " %s %s.", GAL[k], th->want ? (diff >= 1 ? "SHUT, '03" : "SHUT FOR GOOD") : (diff >= 1 ? "SHUT, '03; OPEN, '04" : "SHUT, REOPENED"));
        }
        pz_reading(SP_PZ_BOARD, 1, "THE PROPS' CHALKINGS", txt);
        snprintf(hint, 100, "A LAMP FOR EACH GALLERY THAT NEVER OPENED AGAIN");
        break;
    }
    default: break;
    }
}
static void pz_touch_run3(int i, int how) {
    if (i < 0) return;
    Thing *t = &G.thing[i];
    switch (G.pz) {
    case PZ_POINTS:
        if (how == TU_PULL && i == G.pz_a[3]) {
            sfx_at(SFX_RUMBLE, t->x, t->y, 1.0f);
            if (settings_done()) pz_solve();
            else { say("THE CART CLANGS INTO A DEAD END AND ROLLS BACK"); sfx_at(SFX_BOOM, t->x, t->y, 0.6f); pz_wrong(); }
            t->state = 0;
        }
        break;
    case PZ_HOIST:
        if (how == TU_SET || how == TU_TAKE) {
            int s = pile_sum(G.pz_a[0]);
            if (s == G.pz_a[1]) pz_solve();
            else if (how == TU_SET && s > G.pz_a[1]) { say("THE CAGE GROANS AND STAYS DOWN"); pz_wrong(); }
        }
        break;
    case PZ_TALLY:
        if (how == TU_SET && t->grp == 2) { if (set_done()) pz_solve(); else if (match_full()) { say("THE BOARD IS FULL, AND NOTHING STIRS"); pz_wrong(); } }
        break;
    case PZ_FOSSIL:
        if (how == TU_SET && match_done()) { sfx_at(SFX_BONES, t->x, t->y, 1.0f); pz_solve(); }
        break;
    case PZ_RING: {
        static const int MK[4] = { 10, 11, 3, 13 };
        if (how == TU_TURN) { t->mark = (int16_t)(MK[t->state & 3] + 1); if (settings_done()) pz_solve(); }
        break;
    }
    case PZ_LAMPS:
        if (how == TU_LIGHT && settings_done()) pz_solve();
        break;
    default: break;
    }
}
static int pz_next_run3(int *a) {
    switch (G.pz) {
    case PZ_POINTS: { int r = settings_next(a); if (r != PA_NONE) return r; *a = G.pz_a[3]; return PA_USE; }
    case PZ_HOIST: return hoist_next(a);
    case PZ_TALLY: return set_next(a);
    case PZ_FOSSIL: return match_next(a);
    case PZ_RING: case PZ_LAMPS: return settings_next(a);
    default: return PA_NONE;
    }
}

/* ---- the Deep's, and the two of any run ---- */
/* Prisms: a beam of the magma's light from the west; a prism sends it on the way it is turned; it must reach the geode */
static int prism_trace(float *px, float *py, int *n) {             /* the beam's corners, and whether it reaches */
    int w = G.pz_a[0], h = G.pz_a[1], x = 0, y = G.pz_a[2], d = 0, k = 0, x0 = G.pz_a[4], y0 = G.pz_a[5];
    px[k] = x0 - 0.4f; py[k] = y0 + y + 0.5f; k++;
    for (int steps = 0; steps < 40; steps++) {
        if (x < 0 || y < 0 || x >= w || y >= h) break;
        int p = G.pz_a[8 + y * w + x];
        if (p > 0) { d = G.thing[p - 1].state & 3; px[k] = x0 + x + 0.5f; py[k] = y0 + y + 0.5f; if (k < 30) k++; }
        x += DX[d]; y += DY[d];
    }
    px[k] = x0 + x + 0.5f - DX[d] * 0.5f; py[k] = y0 + y + 0.5f - DY[d] * 0.5f; k++;
    *n = k;
    return x == w && y == G.pz_a[3] && d == 0;
}
static int crystals_next(int *a) {
    if (G.pz_a[0]) return PA_WAIT;                                 /* it listens while the phrase plays */
    int note = G.pz_a[2] ? G.pz_seq[G.pz_step] : 0;
    for (int i = 0; i < G.nthing; i++) if (G.thing[i].grp == 1 && G.thing[i].val == note) { *a = i; return PA_USE; }
    return PA_NONE;
}
static const char *const COURT[5] = { "THE KING", "THE QUEEN", "THE PRIEST", "THE HEADSMAN", "THE FOOL" };
static const int COURT_MARK[5] = { 3, 14, 0, 15, 13 };
/* a court's clue holds of an arrangement (pos[who] = its plinth, 0 nearest the throne) */
typedef struct { int kind, a, b; } CClue;
static int cclue_holds(const CClue *c, const int *pos) {
    switch (c->kind) {
    case 0: return pos[c->a] == 4;                                 /* farthest from the crown */
    case 1: return pos[c->a] == pos[c->b] + 1;                     /* at the other's right hand */
    case 2: return abs(pos[c->a] - pos[c->b]) != 1;                /* never beside */
    case 3: return pos[c->a] == c->b;                              /* on the nth */
    default: return pos[c->a] < pos[c->b];                         /* nearer the crown */
    }
}
static void cclue_text(const CClue *c, char *buf, int n) {
    switch (c->kind) {
    case 0: snprintf(buf, (size_t)n, "%s STANDS FARTHEST FROM THE CROWN", COURT[c->a] + 4); break;
    case 1: snprintf(buf, (size_t)n, "%s AT %s'S RIGHT HAND", COURT[c->a] + 4, COURT[c->b] + 4); break;
    case 2: snprintf(buf, (size_t)n, "%s NEVER BESIDE %s", COURT[c->a] + 4, COURT[c->b] + 4); break;
    case 3: snprintf(buf, (size_t)n, "%s ON THE %s PLINTH", COURT[c->a] + 4, NTH[c->b]); break;
    default: snprintf(buf, (size_t)n, "%s NEARER THE CROWN THAN %s", COURT[c->a] + 4, COURT[c->b] + 4); break;
    }
}
static const char *const WORDS[10] = { "BONES", "GRAVE", "SOULS", "CRYPT", "SHADE", "ASHES", "TOMB", "DUST", "KNELL", "DREAD" };
static void pz_make_run4(int kind, int diff) {
    char *ans = G.pz_answer, *hint = G.pz_hint;
    switch (kind) {
    case PZ_GLASS: case PZ_VEINS: pz_make_run2(kind, diff); return;
    case PZ_PRISMS: {                                              /* the grid's corners hold prisms; the beam turns at them */
        int w = 3 + (diff >= 2), h = 3, bx = -1, by = -1, bd = 1 << 20;
        for (int y = 1; y < MAPN - h - 1; y++)
            for (int x = 2; x < MAPN - w - 2; x++) {
                if (pz_dist[y][x] < 1 || pz_dist[y][x] >= bd) continue;
                int ok = 1;
                for (int k = 0; k < w * h && ok; k++) ok = pz_free(x + k % w, y + k / w) && room_kind_at(x + k % w, y + k / w) >= 0;
                for (int k = 0; k < h && ok; k++) ok = hz_floor(x - 1, y + k) && hz_floor(x + w, y + k);
                if (ok) { bx = x; by = y; bd = pz_dist[y][x]; }
            }
        if (bx < 0) return;
        for (int k = 0; k < w * h; k++) pz_claim(bx + k % w, by + k / w);
        int r0 = rnd(h), r1 = rnd(h), mid = 1 + rnd(w - 2);       /* east along r0, to the column mid, across to r1, east out */
        G.pz_a[0] = w; G.pz_a[1] = h; G.pz_a[2] = r0; G.pz_a[3] = r1; G.pz_a[4] = bx; G.pz_a[5] = by;
        for (int k = 0; k < 16; k++) G.pz_a[8 + k] = 0;
        int corners[2][3] = { { mid, r0, r1 > r0 ? 1 : 3 }, { mid, r1, 0 } }, nc = r0 == r1 ? 0 : 2;
        if (!nc) { corners[0][0] = mid; corners[0][1] = r0; corners[0][2] = 0; nc = 1; }   /* straight across: one prism, set to pass it on */
        for (int c = 0; c < nc; c++) {
            int x = corners[c][0], y = corners[c][1];
            if (c == 1 && x == corners[0][0] && y == corners[0][1]) continue;
            int p = thing_new(TR_TURN, bx + x + 0.5f, by + y + 0.5f, SP_PZ_PRISM0);
            if (p < 0) return;
            Thing *th = &G.thing[p];
            th->grp = 1; th->nstate = 4; th->facing = 1; th->want = corners[c][2]; th->state = (uint8_t)((th->want + 1 + rnd(3)) & 3);
            snprintf(th->name, sizeof th->name, "A PRISM");
            G.map[by + y][bx + x].kind = K_PROP;
            G.pz_a[8 + y * w + x] = p + 1;
        }
        for (int k = 0; k < diff && k < 2; k++) {                  /* deeper, a prism that is no part of it */
            int x = rnd(w), y = rnd(h);
            if (G.pz_a[8 + y * w + x] || y == r0 || y == r1) continue;
            int p = thing_new(TR_TURN, bx + x + 0.5f, by + y + 0.5f, SP_PZ_PRISM0);
            if (p < 0) break;
            G.thing[p].grp = 1; G.thing[p].nstate = 4; G.thing[p].facing = 1; G.thing[p].want = -1; G.thing[p].state = (uint8_t)rnd(4);
            snprintf(G.thing[p].name, sizeof G.thing[p].name, "A PRISM");
            G.map[by + y][bx + x].kind = K_PROP;
            G.pz_a[8 + y * w + x] = p + 1;
        }
        int src = thing_new(TR_READ, bx - 0.5f + 0.4f, by + r0 + 0.5f, SP_PZ_SPOUT);
        if (src >= 0) { snprintf(G.thing[src].name, sizeof G.thing[src].name, "THE FISSURE'S LIGHT"); snprintf(G.thing[src].text, sizeof G.thing[src].text, "THE MAGMA'S LIGHT COMES THROUGH A FISSURE HERE, AND GOES WHERE THE PRISMS SEND IT."); G.thing[src].spr = SP_PZ_GEODE; G.thing[src].frames = 2; G.thing[src].state = 1; }
        int geo = thing_new(TR_READ, bx + w + 0.5f - 0.4f, by + r1 + 0.5f, SP_PZ_GEODE);
        if (geo >= 0) { snprintf(G.thing[geo].name, sizeof G.thing[geo].name, "A DARK GEODE"); snprintf(G.thing[geo].text, sizeof G.thing[geo].text, "A GEODE, DARK, WAITING ON THE LIGHT."); G.thing[geo].frames = 2; G.pz_a[6] = geo; }
        snprintf(ans, 200, "TURN THE PRISMS SO THE BEAM GOES EAST, TURNS %s AT THE %s COLUMN, AND TURNS EAST AGAIN INTO THE GEODE.", r1 > r0 ? "SOUTH" : r1 < r0 ? "NORTH" : "NOT AT ALL", NTH[mid]);
        snprintf(hint, 100, "FOLLOW THE BEAM: TURN THE PRISM WHERE IT GOES ASTRAY");
        break;
    }
    case PZ_COURT: {                                               /* five statues, five plinths, a frieze of rules that allows one way */
        int perm[5] = { 0, 1, 2, 3, 4 }, pos[5];
        shuffle_int(perm, 5);                                      /* perm[plinth] = who */
        for (int p = 0; p < 5; p++) pos[perm[p]] = p;
        CClue cl[10];
        int nc = 0;
        for (int tries = 0; tries < 200 && nc < 10; tries++) {
            CClue c = { rnd(5), rnd(5), rnd(5) };
            if (c.kind == 3) c.b = rnd(5);
            if (c.kind != 3 && c.kind != 0 && c.a == c.b) continue;
            if (c.kind == 3 && diff >= 1 && rnd(2)) continue;      /* deeper, fewer given outright */
            if (!cclue_holds(&c, pos)) continue;
            cl[nc++] = c;
            int count = 0, all[5] = { 0, 1, 2, 3, 4 };             /* how many arrangements the clues allow so far */
            for (int q = 0; q < 120; q++) {
                int pp[5], used = 0, x = q;
                for (int k = 0; k < 5; k++) { int m = x % (5 - k); x /= 5 - k; int j = -1; for (int s = 0; s < 5; s++) if (!(used >> s & 1) && m-- == 0) { j = s; break; } pp[k] = j; used |= 1 << j; }
                int ok = 1;
                for (int z = 0; z < nc && ok; z++) ok = cclue_holds(&cl[z], pp);
                count += ok;
            }
            (void)all;
            if (count == 1) break;
        }
        char frieze[160];
        snprintf(frieze, sizeof frieze, "THE FRIEZE ABOVE THE THRONE:");
        for (int z = 0; z < nc; z++) { char c[60]; cclue_text(&cl[z], c, sizeof c); pz_append(frieze, 160, " %s%s", c, z < nc - 1 ? ";" : "."); }
        pz_standing(TR_READ, SP_PZ_THRONE, 1, 1, "THE THRONE");
        for (int p = 0; p < 5; p++) {
            char nm[40];
            snprintf(nm, sizeof nm, "THE %s PLINTH FROM THE THRONE", NTH[p]);
            int pl = pz_standing(TR_PLACE, SP_PZ_PLINTH, 1, 1, nm);
            if (pl < 0) return;
            G.thing[pl].grp = 1; G.thing[pl].want = perm[p]; G.thing[pl].hold = 0.2f;
        }
        for (int s = 0; s < 5; s++) {
            int st = pz_lying(TR_ITEM, SP_PZ_STATUE, 2, COURT[s]);
            if (st < 0) return;
            G.thing[st].grp = 2; G.thing[st].val = s; G.thing[st].mark = (int16_t)(COURT_MARK[s] + 1); G.thing[st].mz = -0.05f;
            snprintf(G.thing[st].name, sizeof G.thing[st].name, "THE STATUE OF %s", COURT[s] + 4);
        }
        pz_reading(SP_PZ_TABLET, 1, "THE FRIEZE", frieze);
        snprintf(ans, 200, "FROM THE THRONE OUTWARD:");
        for (int p = 0; p < 5; p++) pz_append(ans, 200, " %s%s", COURT[perm[p]] + 4, p < 4 ? "," : ".");
        snprintf(hint, 100, "SET THE ONE THE FRIEZE PINS DOWN FIRST, THEN THE REST");
        break;
    }
    case PZ_CRYSTALS: {                                            /* they sing a phrase in light and sound; strike it back, a note longer each time */
        for (int k = 0; k < 4; k++) {
            int c = pz_standing(TR_STRIKE, SP_CRYSTAL_A, 1, 1, "A SINGING CRYSTAL");
            if (c < 0) return;
            G.thing[c].grp = 1; G.thing[c].val = k; G.thing[c].scale = 0.75f + 0.08f * (float)k;
        }
        G.pz_nseq = 5; G.pz_step = 0;
        for (int k = 0; k < 5; k++) G.pz_seq[k] = rnd(4);
        G.pz_a[0] = 0; G.pz_a[1] = 0; G.pz_a[2] = 0;               /* playing; its note; how long the phrase is so far (0: not begun) */
        pz_reading(SP_PZ_TABLET, 1, "A GOLD TABLET", "STRIKE A CRYSTAL AND THEY SING. SING IT BACK TO THEM, AND THEY SING AGAIN, LONGER, FIVE NOTES IN ALL.");
        snprintf(ans, 200, "THE CRYSTALS' PHRASE, BY THEIR SIZE, SMALLEST FIRST AS ONE:");
        for (int k = 0; k < 5; k++) pz_append(ans, 200, " %d", G.pz_seq[k] + 1);
        snprintf(hint, 100, "LISTEN TO THE WHOLE PHRASE BEFORE YOU STRIKE");
        break;
    }
    case PZ_VENTS: {                                               /* vents to open in the calendar's order, before the first crust cools no more */
        static const char *const MOON[4] = { "THE NEW MOON", "THE WAXING MOON", "THE FULL MOON", "THE WANING MOON" };
        int n = 4, start = rnd(4);
        for (int k = 0; k < n; k++) {
            char nm[40];
            snprintf(nm, sizeof nm, "THE VENT OF %s", MOON[k]);
            int v = pz_wallthing(TR_LEVER, SP_PZ_VENT0, 1, nm);
            if (v < 0) return;
            G.thing[v].grp = 1; G.thing[v].frames = 2; G.thing[v].val = k;
        }
        G.pz_nseq = n; G.pz_step = 0;
        for (int k = 0; k < n; k++) G.pz_seq[k] = (start + (diff >= 2 ? n - k : k)) % 4;
        char txt[160];
        snprintf(txt, sizeof txt, "THE GOLD CALENDAR: THE CRUST WAS LAID FROM %s, %s, A VENT EACH, BEFORE THE FIRST COOLED NO MORE.", MOON[start], diff >= 2 ? "BACKWARD THROUGH THE MONTH" : "ROUND THE MONTH");
        pz_reading(SP_PZ_TABLET, 1, "A GOLD CALENDAR", txt);
        snprintf(ans, 200, "OPEN THE VENTS IN THIS ORDER, QUICKLY:");
        for (int k = 0; k < n; k++) pz_append(ans, 200, " %s%s", MOON[G.pz_seq[k]] + 4, k < n - 1 ? "," : ".");
        snprintf(hint, 100, "THE MOONS GO NEW, WAXING, FULL, WANING");
        break;
    }
    case PZ_SQUARE: {                                              /* nine stones in a three by three, every line fifteen, the corners given */
        static const int MAGIC[9] = { 2, 7, 6, 9, 5, 1, 4, 3, 8 };
        int sq[9], turn = rnd(4), mirror = rnd(2);
        for (int k = 0; k < 9; k++) {
            int r = k / 3, c = k % 3;
            for (int t = 0; t < turn; t++) { int q = r; r = c; c = 2 - q; }
            if (mirror) c = 2 - c;
            sq[r * 3 + c] = MAGIC[k];
        }
        int bx = -1, by = -1, bd = 1 << 20;
        for (int y = 1; y < MAPN - 3; y++)
            for (int x = 1; x < MAPN - 3; x++) {
                if ((x & 3) != 1 || (y & 3) != 1 || pz_dist[y + 1][x + 1] < 1 || pz_dist[y + 1][x + 1] >= bd) continue;
                int ok = 1;
                for (int k = 0; k < 9 && ok; k++) ok = pz_free(x + k % 3, y + k / 3) && room_kind_at(x + k % 3, y + k / 3) >= 0;
                if (ok) { bx = x; by = y; bd = pz_dist[y + 1][x + 1]; }
            }
        if (bx < 0) return;
        int slots[9];
        for (int k = 0; k < 9; k++) {
            pz_claim(bx + k % 3, by + k / 3);
            slots[k] = thing_new(TR_PLACE, bx + k % 3 + 0.5f, by + k / 3 + 0.5f, SP_PZ_SLAB);
            if (slots[k] < 0) return;
            G.thing[slots[k]].grp = 1; G.thing[slots[k]].want = sq[k]; G.thing[slots[k]].hold = 0.04f;
            snprintf(G.thing[slots[k]].name, sizeof G.thing[slots[k]].name, "A SQUARE OF THE GRID");
        }
        for (int k = 0; k < 9; k++) {
            char nm[40];
            snprintf(nm, sizeof nm, "A STONE OF %s", ROMAN[sq[k]]);
            int corner = k == 0 || k == 2 || k == 6 || k == 8;
            int st = corner ? thing_new(TR_ITEM, 0.0f, 0.0f, SP_PZ_NUMSTONE) : pz_lying(TR_ITEM, SP_PZ_NUMSTONE, 2, nm);
            if (st < 0) return;
            Thing *th = &G.thing[st];
            th->grp = 2; th->val = sq[k]; th->mark = (int16_t)(16 + sq[k]); th->mz = -0.28f;
            snprintf(th->name, sizeof th->name, "%s", nm);
            if (corner) thing_put(st, slots[k]);
        }
        pz_reading(SP_PZ_TABLET, 1, "THE SQUARE'S RULE", "NINE STONES, ONE TO NINE: EVERY LINE OF THREE, ACROSS, DOWN AND CORNER TO CORNER, MAKES FIFTEEN. THE CORNERS ARE GIVEN.");
        snprintf(ans, 200, "THE GRID, ROW BY ROW:");
        for (int r = 0; r < 3; r++) pz_append(ans, 200, " %d %d %d%s", sq[r * 3], sq[r * 3 + 1], sq[r * 3 + 2], r < 2 ? ";" : ".");
        snprintf(hint, 100, "THE MIDDLE OF THE GRID IS FIVE");
        break;
    }
    case PZ_LETTERS: {                                             /* dials of letters, set to a word cut in pieces round the room */
        const char *word = WORDS[rnd(10)];
        int n = (int)strlen(word);
        snprintf(ans, 200, "SET THE DIALS TO SPELL %s.", word);
        for (int k = 0; k < n; k++) {
            char nm[40];
            snprintf(nm, sizeof nm, "THE %s DIAL", NTH[k]);
            int d = pz_wallthing(TR_TURN, SP_PZ_DIAL0, 1, nm);
            if (d < 0) return;
            Thing *th = &G.thing[d];
            int letters[6], ok = 0;
            letters[0] = word[k] - 'A';
            for (int j = 1; j < 6; j++) { int l; do l = rnd(26); while (l == letters[0]); letters[j] = l; }
            shuffle_int(letters, 6);
            for (int j = 0; j < 6; j++) if (letters[j] == word[k] - 'A') ok = j;
            th->grp = 1; th->nstate = 6; th->want = ok; th->state = (uint8_t)((ok + 1 + rnd(5)) % 6);
            for (int j = 0; j < 6; j++) G.pz_a[k * 6 + j < 32 ? k * 6 + j : 31] = letters[j];
            th->val = k; th->mark = (int16_t)(100 + letters[th->state]); th->mz = 0.0f;
        }
        for (int k = 0; k < n; k++) {                              /* the pieces of the word, cut round the room */
            char nm[40], txt[160];
            snprintf(nm, sizeof nm, "A WORD CUT IN THE WALL");
            snprintf(txt, sizeof txt, "THE %s LETTER IS %c.", NTH[k], word[k]);
            if (diff >= 1 && k > 0) snprintf(txt, sizeof txt, "THE %s LETTER COMES AFTER %c IN THE ALPHABET, BY %d.", NTH[k], word[k] - 1 - (k % 2), 1 + (k % 2));
            pz_reading(SP_PZ_TABLET, 1, nm, txt);
        }
        snprintf(hint, 100, "EACH DIAL IS ONE LETTER OF THE WORD, IN ORDER");
        break;
    }
    default: break;
    }
}
static void pz_touch_run4(int i, int how) {
    if (G.pz == PZ_GLASS || G.pz == PZ_VEINS) {
        if (i < 0) { if (G.pz == PZ_VEINS && G.pz_a[30] && pipe_flow(NULL)) G.thing[G.pz_a[30]].state = 1; return; }
        if (G.pz == PZ_VEINS && how == TU_TURN && pipe_flow(NULL)) { sfx_at(SFX_UNLOCK, G.thing[i].x, G.thing[i].y, 1.0f); pz_solve(); return; }
        pz_touch_run2(i, how);
        return;
    }
    if (i < 0) {                                                   /* the clocks: the crystals' phrase playing, the crust cooling */
        if (G.pz == PZ_CRYSTALS && G.pz_a[0]) {
            G.pz_t += pz_dt;
            if (G.pz_t >= 0.65f) {
                G.pz_t = 0.0f;
                if (G.pz_a[1] >= G.pz_a[2]) { G.pz_a[0] = 0; G.pz_step = 0; return; }
                int note = G.pz_seq[G.pz_a[1]++];
                for (int k = 0; k < G.nthing; k++) if (G.thing[k].grp == 1 && G.thing[k].val == note) { G.thing[k].t = 0.0f; snd_tone(1, 76 + note * 3, 0.6f, 0.5f); }
            }
        }
        if (G.pz == PZ_VENTS && G.pz_step > 0 && (G.pz_t += pz_dt) > 20.0f) {   /* too slow: the first crust melts */
            G.pz_step = 0; G.pz_t = 0.0f;
            for (int k = 0; k < G.nthing; k++) if (G.thing[k].grp == 1) G.thing[k].state = 0;
            say("THE FIRST CRUST HAS MELTED AGAIN");
            pz_wrong();
        }
        return;
    }
    Thing *t = &G.thing[i];
    switch (G.pz) {
    case PZ_PRISMS:
        if (how == TU_TURN) {
            float px[32], py[32]; int n;
            if (prism_trace(px, py, &n)) { if (G.pz_a[6]) G.thing[G.pz_a[6]].state = 1; sfx_at(SFX_HUM, t->x, t->y, 1.0f); pz_solve(); }
        }
        break;
    case PZ_COURT: case PZ_SQUARE:
        if (how == TU_SET && t->grp == 2) {
            if (match_done()) pz_solve();
            else if (match_full()) { say(G.pz == PZ_COURT ? "THE COURT STANDS, AND NOTHING MOVES" : "NOT EVERY LINE MAKES FIFTEEN"); pz_wrong(); }
        }
        break;
    case PZ_CRYSTALS:
        if (how != TU_STRIKE || t->grp != 1) break;
        snd_tone(1, 76 + t->val * 3, 0.6f, 0.5f);
        if (G.pz_a[0]) break;                                      /* they are singing: listen */
        if (!G.pz_a[2]) { G.pz_a[2] = 1; G.pz_a[0] = 1; G.pz_a[1] = 0; G.pz_t = -0.6f; break; }   /* the first strike: they sing one note */
        if (t->val == G.pz_seq[G.pz_step]) {
            if (++G.pz_step >= G.pz_a[2]) {
                if (G.pz_a[2] >= G.pz_nseq) { pz_solve(); break; }
                G.pz_a[2]++; G.pz_a[0] = 1; G.pz_a[1] = 0; G.pz_t = -0.8f;   /* longer, and again */
            }
        } else { say("THE CRYSTALS FALL SILENT, THEN SING AGAIN"); G.pz_a[0] = 1; G.pz_a[1] = 0; G.pz_t = -1.0f; G.pz_step = 0; pz_wrong(); }
        break;
    case PZ_VENTS:
        if (how == TU_PULL && t->grp == 1) {
            if (!t->state) { t->state = 1; break; }                /* already open: it stays so */
            sfx_at(SFX_HISS, t->x, t->y, 0.8f);
            G.pz_t = 0.0f;
            if (t->val == G.pz_seq[G.pz_step]) { if (++G.pz_step >= G.pz_nseq) pz_solve(); }
            else {
                G.pz_step = 0;
                for (int k = 0; k < G.nthing; k++) if (G.thing[k].grp == 1) G.thing[k].state = 0;
                say("THE CRUST CRACKS AND SINKS");
                pz_wrong();
            }
        }
        break;
    case PZ_LETTERS:
        if (how == TU_TURN) {
            int k = t->val;
            t->mark = (int16_t)(100 + G.pz_a[clampi(k * 6 + t->state, 0, 31)]);
            say("%s: %c", t->name, 'A' + G.pz_a[clampi(k * 6 + t->state, 0, 31)]);
            if (settings_done()) pz_solve();
        }
        break;
    default: break;
    }
}
static int pz_next_run4(int *a) {
    switch (G.pz) {
    case PZ_GLASS: return glass_next(a);
    case PZ_VEINS: return pipes_next(a);
    case PZ_PRISMS: case PZ_LETTERS: return settings_next(a);
    case PZ_COURT: case PZ_SQUARE: return match_next(a);
    case PZ_CRYSTALS: return crystals_next(a);
    case PZ_VENTS: {
        for (int i = 0; i < G.nthing; i++) if (G.thing[i].grp == 1 && G.thing[i].val == G.pz_seq[G.pz_step] && !G.thing[i].state) { *a = i; return PA_USE; }
        return PA_NONE;
    }
    default: return PA_NONE;
    }
}

#ifdef CRAWL_DEBUG
static void tp_puzzle(void) {                                      /* testing: before the puzzle's first piece, facing it */
    for (int i = 0; i < G.nthing; i++) {
        const Thing *t = &G.thing[i];
        if (!t->grp) continue;
        for (int r = 2; r >= 1; r--)
            for (int d = 0; d < 4; d++) {
                int x = t->cx + DX[d] * r, y = t->cy + DY[d] * r;
                if (!hz_floor(x, y) || in_wing(x, y) || !line_clear(x + 0.5f, y + 0.5f, t->x, t->y)) continue;
                G.px = x + 0.5f; G.py = y + 0.5f; G.ang = atan2f(t->y - G.py, t->x - G.px); human_begin();
                return;
            }
    }
}
#endif

