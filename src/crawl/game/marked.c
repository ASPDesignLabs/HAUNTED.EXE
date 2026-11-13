/* A part of scene_crawl.c (docs/PLAN-tidy.md): it is #included there in its place and never compiled on its own.
 * Things that want a spell: their kinds, where they go, what a strike does, sequences and gates, things for fights; the lull's watch. */

/* ---- Things that want a spell (1.0.0b16, docs/PLAN-play.md sections 4 and 5): set along the way from the start to the
 * exit where it runs longest with nothing in it, each wanting a line the caster holds, marked with its seal (crawl_mark).
 * Struck by its element a pip lights; by another, a dull ring; every pip lit, it is done and gives what it gives. ---- */
static void hz_dput(int x, int y, int c);
static int hz_strike_el(int kind, int el, float share, float sx, float sy);
static const int MK_OWN[NELEM] = { MKK_PRICKET, MKK_URN, MKK_COIL, MKK_VINE, MKK_LANTERN, MKK_BARRICADE };   /* each line's own */
#define MK_STRETCH 14                       /* hall steps on the way with nothing in them before a thing is set */
#define MK_COIN 6                           /* the Cursed Coin's bit for every seal on a level answered (one level in five) */
static int mk_level_ok(void) { return G.boss < 0 && !PK.depth && !G.bchallenge; }
static const char *mk_name(const Marked *m) {                      /* some take their run's name */
    int run = crawl_set(G.level);
    if (m->kind == MKK_WINCH && run == 1) return "A SLUICE WINCH";
    if (m->kind == MKK_HSEAL && run == 3) return "AN OBSIDIAN SEAL";
    return MKK[m->kind].name;
}
static int mk_answered(const Marked *m) { return m->done == 1 || m->times > 0; }
static void mk_deco(const Marked *m, int spr, int nfr) {
    if (m->deco < 0 || m->deco >= G.ndecos) return;
    G.decos[m->deco].spr = spr; G.decos[m->deco].nfr = nfr; G.decos[m->deco].x = m->x; G.decos[m->deco].y = m->y;
}
static int mk_at(int x, int y, int r) {                            /* a thing already within r cells */
    for (int i = 0; i < G.nmk; i++) if (abs((int)G.mk[i].x - x) <= r && abs((int)G.mk[i].y - y) <= r) return 1;
    return 0;
}
static int mk_free(int x, int y, int lanes) {                      /* floor clear of what is there; off the lanes unless asked */
    if (x < 1 || y < 1 || x >= MAPN - 1 || y >= MAPN - 1 || G.map[y][x].kind != K_FLOOR || (!lanes && lane(x, y)) || G.map[y][x].secret) return 0;
    if (in_band(y) || (x == G.exit_fx && y == G.exit_fy) || G.bulky[y][x]) return 0;
    for (int d = 0; d < G.ndecos; d++) if (fabsf(G.decos[d].x - x - 0.5f) < 0.9f && fabsf(G.decos[d].y - y - 0.5f) < 0.9f) return 0;
    for (int i = 0; i < G.nitems; i++) if (G.items[i].alive && G.items[i].cx == x && G.items[i].cy == y) return 0;
    for (int d = 0; d < 4; d++) if (G.map[y + DY[d]][x + DX[d]].kind == K_DOOR) return 0;
    return 1;
}
static int mk_spot_sep(int px, int py, int *ox, int *oy, int sep) {   /* beside the way: floor off the lanes, clear of what is there */
    for (int r = 1; r <= 2; r++)
        for (int k = 0; k < 16; k++) {
            int x = px + (int)lroundf(cosf((float)k * 0.3927f) * (float)r), y = py + (int)lroundf(sinf((float)k * 0.3927f) * (float)r);
            if (!mk_free(x, y, 0) || mk_at(x, y, sep)) continue;
            *ox = x; *oy = y;
            return 1;
        }
    return 0;
}
static int mk_spot(int px, int py, int *ox, int *oy) { return mk_spot_sep(px, py, ox, oy, 4); }
static Marked *mk_add(int kind, int el, int x, int y) {
    if (G.nmk >= MAXMK) return NULL;
    Marked *m = &G.mk[G.nmk++];
    memset(m, 0, sizeof *m);
    m->kind = kind; m->el = el; m->x = x + 0.5f; m->y = y + 0.5f; m->door = -1;
    m->hx = m->vx = m->x; m->hy = m->vy = m->y;
    m->pips = G.level <= 4 ? imin(MKK[kind].pips, 2) : MKK[kind].pips;   /* the first levels generous */
    m->deco = MKK[kind].s0 < 0 ? -1 : G.ndecos;                   /* the water, the floor and the gas wear only the seal */
    if (MKK[kind].s0 < 0) ;
    else if (kind == MKK_CLIGHT || kind == MKK_CART) add_deco_ex(m->x, m->y, MKK[kind].s0, 2, 0);
    else if (kind == MKK_WEB) add_deco_ex(m->x, m->y, SP_WEBSHEET, 1, DF_SWAY);
    else add_deco(m->x, m->y, MKK[kind].s0);
    if (kind == MKK_CART && m->deco >= 0) G.decos[m->deco].nfr = 2;
    if (kind == MKK_BARRICADE || kind == MKK_GEODE)                /* R3: Physical brings its own bones */
        for (int d = 0, n = 0; d < 4 && n < 2; d++) {
            int bx = x + DX[d], by = y + DY[d];
            if (G.map[by][bx].kind != K_FLOOR) continue;
            Item *it = add_item(bx, by, IT_BONE);
            if (it) { it->secret = 1; n++; }
        }
    return m;
}

/* The winch's and the hearth's gate: an ordinary door that is a short cut (shut, every cell is still reached the long way
 * round), barred while it waits (GATED), its thing on the side nearer the start */
static int16_t mk_d0[MAPN][MAPN], mk_d1[MAPN][MAPN];
static int mk_flood(int bx, int by, int16_t d[MAPN][MAPN]) {
    static int q[MAPN * MAPN];
    for (int y = 0; y < MAPN; y++) for (int x = 0; x < MAPN; x++) d[y][x] = -1;
    int h = 0, t = 0;
    d[G.cy][G.cx] = 0; q[t++] = G.cy * MAPN + G.cx;
    while (h < t) {
        int c = q[h++], x = c % MAPN, y = c / MAPN;
        for (int k = 0; k < 4; k++) {
            int nx = x + DX[k], ny = y + DY[k];
            if (nx < 1 || ny < 1 || nx >= MAPN - 1 || ny >= MAPN - 1 || d[ny][nx] >= 0 || !is_open(nx, ny) || (nx == bx && ny == by)) continue;
            if (G.map[ny][nx].kind == K_DOOR && (G.doors[G.map[ny][nx].door].locked == SEALED || G.doors[G.map[ny][nx].door].locked == GATED)) continue;
            d[ny][nx] = (int16_t)(d[y][x] + 1); q[t++] = ny * MAPN + nx;
        }
    }
    return t;
}
static int mk_gated(int kind, int el) {
    for (int i = 0; i < G.nmk; i++) if (G.mk[i].door >= 0) return 0;   /* one a level */
    int all = mk_flood(-1, -1, mk_d0), best = -1, bs = -9999;
    for (int i = 0; i < G.ndoors; i++) {
        const Door *d = &G.doors[i];
        if (d->locked || i == G.pz_door || i == G.tri_door || mk_d0[d->y][d->x] < 8 || in_band(d->y)) continue;
        int sc = -mk_d0[d->y][d->x] + rnd(24);
        if (sc > bs) { bs = sc; best = i; }
    }
    for (int tries = 0; tries < 6 && best >= 0; tries++) {
        Door *d = &G.doors[best];
        if (mk_flood(d->x, d->y, mk_d1) == all - 1) {
            int nx = -1, ny = -1, nd = 9999;                       /* the side nearer the start */
            for (int k = 0; k < 4; k++) {
                int x = d->x + DX[k], y = d->y + DY[k];
                if (G.map[y][x].kind == K_FLOOR && mk_d0[y][x] >= 0 && mk_d0[y][x] < nd) { nd = mk_d0[y][x]; nx = x; ny = y; }
            }
            for (int r = 1; r <= 3 && nx >= 0; r++)
                for (int k = 0; k < 16; k++) {
                    int x = nx + (int)lroundf(cosf((float)k * 0.3927f) * (float)r), y = ny + (int)lroundf(sinf((float)k * 0.3927f) * (float)r);
                    if (!mk_free(x, y, 0) || mk_at(x, y, 3) || mk_d1[y][x] < 0 || mk_d1[y][x] != mk_d0[y][x]) continue;
                    Marked *m = mk_add(kind, el, x, y);
                    if (!m) return 0;
                    m->door = best;
                    d->locked = GATED; d->open = 0.0f; d->opening = 0;
                    return 1;
                }
        }
        best = -1; bs = -9999;                                     /* not a short cut, or no room by it: another */
        for (int i = 0; i < G.ndoors; i++) {
            const Door *e = &G.doors[i];
            if (e->locked || i == G.pz_door || i == G.tri_door || mk_d0[e->y][e->x] < 8 || in_band(e->y)) continue;
            int sc = rnd(100);
            if (sc > bs) { bs = sc; best = i; }
        }
    }
    return 0;
}

/* the way from the start to the exit, for the placing */
static int mk_px[MAPN * MAPN], mk_py[MAPN * MAPN], mk_n;
static int mk_hall(int i) {                                        /* a step of the way in a hall, not a room or a door */
    if (i < 0 || i >= mk_n) return 0;
    int x = mk_px[i], y = mk_py[i];
    if (G.map[y][x].kind != K_FLOOR || G.map[y][x].secret || G.bulky[y][x] || mk_at(x, y, 3) || (x == G.exit_fx && y == G.exit_fy)) return 0;
    for (int r = 0; r < G.nrooms; r++) if (in_room(&G.rooms[r], x, y)) return 0;
    for (int j = 0; j < G.nitems; j++) if (G.items[j].alive && G.items[j].cx == x && G.items[j].cy == y) return 0;
    return 1;
}
static int mk_straight(int i, int n) {                             /* n steps of the way from i in a straight hall */
    for (int j = 0; j < n; j++) if (!mk_hall(i + j)) return 0;
    for (int j = 2; j < n; j++)
        if (mk_px[i + j] - mk_px[i + j - 1] != mk_px[i + 1] - mk_px[i] || mk_py[i + j] - mk_py[i + j - 1] != mk_py[i + 1] - mk_py[i]) return 0;
    return 1;
}
static int mk_bells(int i, int el) {                               /* three bells down a hall, one behind another */
    if (G.nmk + 3 > MAXMK) return 0;
    for (int o = 0; o < 12; o++) {
        int j = i + ((o & 1) ? -(o + 1) / 2 : o / 2);
        if (!mk_straight(j, 3)) continue;
        int first = G.nmk;
        for (int b = 0; b < 3; b++) { Marked *m = mk_add(MKK_BELL, el, mk_px[j + b], mk_py[j + b]); if (m) m->grp = first + 1; }
        return 1;
    }
    return 0;
}
static int mk_cart(int i, int el) {                                /* run III: a cart on its rails along five cells of hall */
    for (int o = 0; o < 14; o++) {
        int j = i + ((o & 1) ? -(o + 1) / 2 : o / 2);
        if (!mk_straight(j, 5)) continue;
        Marked *m = mk_add(MKK_CART, el, mk_px[j + 2], mk_py[j + 2]);
        if (!m) return 0;
        m->hx = mk_px[j] + 0.5f; m->hy = mk_py[j] + 0.5f;
        m->vx = (float)(mk_px[j + 1] - mk_px[j]); m->vy = (float)(mk_py[j + 1] - mk_py[j]);
        m->t = 2.0f;
        return 1;
    }
    return 0;
}
static int mk_can(int k, int *el) {                                /* may kind k be set here, and in which line */
    const MkKind *K = &MKK[k];
    int run = clampi(crawl_set(G.level), 0, 3), step = crawl_step(G.level) + 1;
    if (!(K->runs >> run & 1) || (K->tier == 2 && step < MK_FROM && G.level < 11) || K->tier >= 4) return 0;
    int nheld = 0;
    for (int e = 0; e < NELEM; e++) nheld += align_open(e);
    if (K->tier == 3 && G.level < 13 && crawl_loop(G.level) == 0) return 0;   /* the sequences from level 13: three lines or more */
    if ((k == MKK_TABLET || k == MKK_RELIQ) && (nheld < 3 || G.nmk + 4 > MAXMK)) return 0;
    if (k == MKK_WSTONE && nheld < 2) return 0;
    if (k == MKK_RELAY && G.nmk + 3 > MAXMK) return 0;
    if (k == MKK_HSEAL && !(align_open(EL_FIRE) && align_open(EL_FROST))) return 0;
    if (k == MKK_BELL && G.nmk + 3 > MAXMK) return 0;
    int e = K->el;
    if (e >= 0) { if (!align_open(e)) return 0; *el = e; return 1; }
    int open[NELEM], no = 0;
    for (int i = 0; i < NELEM; i++) if (align_open(i)) open[no++] = i;
    if (!no) return 0;
    *el = open[rnd(no)];
    return 1;
}
static int mk_pick(int first, int el_out[1]) {                     /* what to set: the class's own first, then what the lines held allow */
    int any = 0, own = cls_el() >= 0 ? cls_el() : LEDGER.lent - 1;
    for (int e = 0; e < NELEM; e++) any |= align_open(e);
    if (!any) return -1;
#ifdef CRAWL_DEBUG
    if (getenv("CRAWL_MARKK")) { int k = clampi(atoi(getenv("CRAWL_MARKK")), 0, NMKK - 1), e; if (mk_can(k, &e)) { el_out[0] = e; return k; } }   /* that kind, where it can be */
#endif
    if (first && own >= 0 && align_open(own)) { el_out[0] = own; return own == EL_FIRE && rnd(2) ? MKK_RACK : MK_OWN[own]; }
    int run = clampi(crawl_set(G.level), 0, 3), late = crawl_step(G.level) + 1 >= MK_FROM || G.level >= 11;
    int want_own = rnd(2), want_new = late && rnd(5) < 3;          /* the run's own half the time; section 4.3's three times in five */
    int want_seq = (G.level >= 13 || crawl_loop(G.level) > 0) && rnd(4) == 0;   /* a sequence one time in four, from level 13 */
    for (int tries = 0; tries < 40; tries++) {
        int k = rnd(NMKK), e;
        if (tries < 20 && want_seq && MKK[k].tier != 3) continue;
        if (tries < 24 && want_own && !(MKK[k].own >> run & 1)) continue;
        if (tries < 32 && want_new && !want_seq && MKK[k].tier != 2) continue;
        if (!mk_can(k, &e)) continue;
        int seen = 0;
        for (int i = 0; i < G.nmk; i++) seen += G.mk[i].kind == k;
        if (seen && tries < 36) continue;                          /* a level's things unlike, when they can be */
        el_out[0] = e;
        return k;
    }
    int e;
    if (!mk_can(MKK_COFFER, &e)) return -1;
    el_out[0] = e;
    return MKK_COFFER;
}
static int mk_count(void) {                                        /* things set: three bells are one */
    int n = 0;
    for (int i = 0; i < G.nmk; i++) n += !G.mk[i].grp || G.mk[i].grp == i + 1;
    return n;
}
static int mk_held(int out[NELEM]) {                               /* the lines held, shuffled */
    int n = 0;
    for (int e = 0; e < NELEM; e++) if (align_open(e)) out[n++] = e;
    for (int i = 0; i < n; i++) { int j = i + rnd(n - i), t = out[i]; out[i] = out[j]; out[j] = t; }
    return n;
}
static int mk_tablet(int i) {                                      /* the tablet, and three plinths on down the way, each its own line */
    int held[NELEM], nh = mk_held(held), tx, ty, px[3], py[3];
    if (nh < 3 || G.nmk + 4 > MAXMK || !mk_spot(mk_px[i], mk_py[i], &tx, &ty)) return 0;
    for (int k = 0; k < 3; k++) {
        int j = i + 3 + 3 * k, ok = 0;
        for (int o = 0; o < 4 && !ok && j + o < mk_n; o++) {
            int x, y;
            if (!mk_spot_sep(mk_px[j + o], mk_py[j + o], &x, &y, 1) || (abs(x - tx) <= 1 && abs(y - ty) <= 1)) continue;
            int clash = 0;
            for (int q = 0; q < k; q++) clash |= abs(px[q] - x) <= 1 && abs(py[q] - y) <= 1;
            if (clash) continue;
            px[k] = x; py[k] = y; ok = 1;
        }
        if (!ok) return 0;
    }
    int lead = G.nmk;
    Marked *t = mk_add(MKK_TABLET, held[0], tx, ty);
    if (!t) return 0;
    t->grp = lead + 1; t->pips = 3;
    int ord[3] = { 0, 1, 2 };
    for (int a = 0; a < 3; a++) { int b = a + rnd(3 - a), q = ord[a]; ord[a] = ord[b]; ord[b] = q; }
    for (int k = 0; k < 3; k++) {                                  /* the plinths down the hall hold the lines; the tablet their order */
        Marked *p = mk_add(MKK_PLINTH, held[k], px[k], py[k]);
        if (p) p->grp = lead + 1;
        t->els[ord[k]] = held[k];
    }
    t->el = t->els[0];
    return 1;
}
static int mk_relay(int i) {                                       /* three lamps on down the way; fire passes from one to the next */
    int held[NELEM], nh = mk_held(held), px[3], py[3], light = align_open(EL_LIGHT);
    (void)held; (void)nh;
    if (G.nmk + 3 > MAXMK) return 0;
    for (int k = 0; k < 3; k++) {
        int j = i + 3 * k, ok = 0;
        for (int o = 0; o < 3 && !ok && j + o < mk_n; o++) {
            int x, y;
            if (!mk_spot_sep(mk_px[j + o], mk_py[j + o], &x, &y, 1)) continue;
            int clash = 0;
            for (int q = 0; q < k; q++) clash |= abs(px[q] - x) <= 1 && abs(py[q] - y) <= 1;
            if (clash) continue;
            px[k] = x; py[k] = y; ok = 1;
        }
        if (!ok) return 0;
    }
    int lead = G.nmk, mid = rnd(2) ? 1 : 0;                        /* the middle one frozen, or a dead coil if lightning is held */
    for (int k = 0; k < 3; k++) {
        Marked *m;
        if (k == 1 && mid && light && rnd(2)) { m = mk_add(MKK_COIL, EL_LIGHT, px[k], py[k]); if (m) m->pips = 1; }
        else {
            m = mk_add(MKK_RELAY, EL_FIRE, px[k], py[k]);
            if (m && k == 1 && mid) { m->st = 2; mk_deco(m, SP_MK_RELAY2, 1); }   /* frozen: thawed, then lit */
        }
        if (m) m->grp = lead + 1;
    }
    return 1;
}
static int mk_place(int i, int first) {                            /* a thing near step i of the way */
    for (int tries = 0; tries < 5; tries++) {
        int el, x, y, k = mk_pick(first && tries == 0, &el);
        if (k < 0) return 0;
        switch (k) {
        case MKK_BELL: if (mk_bells(i, el)) return 1; break;
        case MKK_CART: if (mk_cart(i, el)) return 1; break;
        case MKK_WINCH: case MKK_HEARTH: case MKK_WSTONE: if (mk_gated(k, el)) return 1; break;
        case MKK_TABLET: if (mk_tablet(i)) return 1; break;
        case MKK_RELAY: if (mk_relay(i)) return 1; break;
        default: {
            if (!mk_spot(mk_px[i], mk_py[i], &x, &y)) return 0;
            Marked *m = mk_add(k, el, x, y);
            if (m && k == MKK_RELIQ) { int held[NELEM]; mk_held(held); for (int q = 0; q < 3; q++) m->els[q] = held[q]; m->el = m->els[0]; m->t = 3.5f; }
            return 1;
        }
        }
    }
    return 0;
}
static void place_marked(void) {
    G.nmk = 0; G.ward_said = 0;
    if (!mk_level_ok()) return;
    static int16_t prev[MAPN][MAPN];
    static int qx[MAPN * MAPN], qy[MAPN * MAPN];
    for (int y = 0; y < MAPN; y++) for (int x = 0; x < MAPN; x++) prev[y][x] = -1;
    int qh = 0, qt = 0, tx = G.exit_fx, ty = G.exit_fy;
    prev[G.cy][G.cx] = (int16_t)(G.cy * MAPN + G.cx); qx[qt] = G.cx; qy[qt] = G.cy; qt++;
    while (qh < qt) {                                              /* the way from the start to the exit */
        int x = qx[qh], y = qy[qh]; qh++;
        if (x == tx && y == ty) break;
        for (int d = 0; d < 4; d++) {
            int nx = x + DX[d], ny = y + DY[d];
            if (nx < 0 || ny < 0 || nx >= MAPN || ny >= MAPN || prev[ny][nx] >= 0 || !is_open(nx, ny)) continue;
            prev[ny][nx] = (int16_t)(y * MAPN + x); qx[qt] = nx; qy[qt] = ny; qt++;
        }
    }
    if (tx < 0 || ty < 0 || prev[ty][tx] < 0) {
#ifdef CRAWL_DEBUG
        fprintf(stderr, "MARKED level %d: no way to the exit (%d,%d)\n", G.level, tx, ty);
#endif
        return;
    }
    int n = 0;
    static int rx[MAPN * MAPN], ry[MAPN * MAPN];
    for (int c = ty * MAPN + tx; n < MAPN * MAPN; c = prev[c / MAPN][c % MAPN]) {
        rx[n] = c % MAPN; ry[n] = c / MAPN; n++;
        if (c == G.cy * MAPN + G.cx) break;
    }
    for (int i = 0; i < n; i++) { mk_px[i] = rx[n - 1 - i]; mk_py[i] = ry[n - 1 - i]; }   /* from the start */
    mk_n = n;
    int want = G.level <= 4 ? 4 + rnd(3) : 3 + rnd(3), quiet = 0;   /* Q11: three to six, more early */
    for (int i = 0; i < n && mk_count() < want; i++) {             /* from the start: where it runs longest with nothing */
        int in = 0;
        for (int r = 0; r < G.nrooms && !in; r++) in = in_room(&G.rooms[r], mk_px[i], mk_py[i]);
        if (in || G.map[mk_py[i]][mk_px[i]].kind == K_DOOR) { quiet = 0; continue; }
        if (++quiet < MK_STRETCH) continue;
        if (mk_place(i, G.nmk == 0)) quiet = 0;
    }
    for (int tries = 0; tries < 40 && mk_count() < want && n > 8; tries++)   /* too few long stretches: anywhere along it */
        mk_place(4 + rnd(n - 8), G.nmk == 0);
#ifdef CRAWL_DEBUG
    fprintf(stderr, "MARKED level %d: %d on a way of %d:", G.level, mk_count(), n);
    for (int i = 0; i < G.nmk; i++) fprintf(stderr, " %s/%s/%d at (%d,%d)%s", MKK[G.mk[i].kind].name, ELEM_NAME[G.mk[i].el], G.mk[i].pips, (int)G.mk[i].x, (int)G.mk[i].y, G.mk[i].door >= 0 ? " gated" : "");
    fprintf(stderr, "\n");
#endif
}
/* things for fights (section 4.5): one or two a level from 5, in rooms (where the fights are), only in a line held */
static int mk_room_spot(int *ox, int *oy, int water) {
    for (int tries = 0; tries < 200; tries++) {
        if (!G.nrooms) return 0;
        const Room *R = &G.rooms[rnd(G.nrooms)];
        if (vault_of_room(R->x + R->w / 2, R->y + R->h / 2) >= 0 || in_room(R, G.cx, G.cy) || in_room(R, G.exit_fx, G.exit_fy)) continue;
        int x = R->x + 1 + rnd(imax(1, R->w - 2)), y = R->y + 1 + rnd(imax(1, R->h - 2));
        if (water ? !(on_water(x, y) && G.map[y][x].kind == K_FLOOR && !mk_at(x, y, 4)) : (!mk_free(x, y, 0) || mk_at(x, y, 4))) continue;
        if (in_wing(x, y) || in_trial(x, y) || abs(x - G.cx) + abs(y - G.cy) < 8) continue;
        *ox = x; *oy = y;
        return 1;
    }
    return 0;
}
static void place_fights(void) {
    if (!mk_level_ok() || (G.level < 5 && crawl_loop(G.level) == 0)) return;
    int want = 1 + (G.level >= 13 || crawl_loop(G.level) > 0), run = clampi(crawl_set(G.level), 0, 3), placed = 0;
    for (int tries = 0; tries < 30 && placed < want && G.nmk < MAXMK; tries++) {
        int k = MKK_CASK + rnd(MKK_DAMP - MKK_CASK + 1), x, y;
#ifdef CRAWL_DEBUG
        if (getenv("CRAWL_FIGHTK")) k = clampi(atoi(getenv("CRAWL_FIGHTK")), MKK_CASK, MKK_DAMP);   /* that kind, where it can be */
#endif
        int el = MKK[k].el;
        if (!(MKK[k].runs >> run & 1) || !align_open(el)) continue;
        int dup = 0;
        for (int i = 0; i < G.nmk; i++) dup |= G.mk[i].kind == k;
        if (dup) continue;
        if (k == MKK_DAMP) {                                       /* on a stope's firedamp, where it hangs */
            int s0 = -1;
            for (int i = 0; i < G.nsite && s0 < 0; i++) if (G.site[i].kind == HZ_DAMP) s0 = i;
            if (s0 < 0) continue;
            x = G.site[s0].x; y = G.site[s0].y;
        } else if (!mk_room_spot(&x, &y, k == MKK_WATER)) continue;
        Marked *m = mk_add(k, el, x, y);
        if (m) placed++;
    }
#ifdef CRAWL_DEBUG
    if (placed) {
        fprintf(stderr, "FIGHTS level %d:", G.level);
        for (int i = 0; i < G.nmk; i++) if (MKK[G.mk[i].kind].tier == 4) fprintf(stderr, " %s at (%d,%d)", MKK[G.mk[i].kind].name, (int)G.mk[i].x, (int)G.mk[i].y);
        fprintf(stderr, "\n");
    }
#endif
}
static int shot_elem(const Shot *s) {
    if (s->kind < NATK) return ATK_ELEM[s->kind];
    return s->kind == SHOT_ORB ? EL_LIGHT : s->kind == SHOT_LANCE ? EL_FROST : (s->kind == SHOT_BALLISTA || s->kind == SHOT_SPLINTER) ? EL_PHYS : -1;
}
static Item *mk_item(Marked *m, int type) {                        /* a find, out of the level's totals */
    Item *it = add_item((int)m->x, (int)m->y, type);
    if (it) { it->secret = 1; it->seen = 1; if (it->type < IT_KEY) { G.items_total--; G.value_total -= treasure_value(it->type); } }
    return it;
}
static void mk_reward(Marked *m) {
    if (m->grp && m->grp != (int)(m - G.mk) + 1 && m->kind != MKK_BELL) return;   /* a group's gift is its first's */
    if (MKK[m->kind].tier == 4) return;                            /* things for fights give the fight */
    switch (m->kind) {
    case MKK_TABLET: case MKK_RELIQ: mk_item(m, IT_CHALICE); mk_item(m, IT_GEMS); break;
    case MKK_WSTONE: case MKK_RELAY: mk_item(m, rnd(2) ? IT_COINS : IT_GEMS); break;
    case MKK_COFFER: mk_item(m, rnd(3) ? IT_COINS : IT_CHALICE); if (rnd(3) == 0) mk_item(m, IT_POTION); break;
    case MKK_CAGE: mk_item(m, rnd(2) ? IT_COINS : IT_GEMS); break;
    case MKK_URN: mk_item(m, IT_POTION); break;
    case MKK_VINE: case MKK_RSKULL: case MKK_HEARTH: mk_item(m, IT_COINS); break;
    case MKK_BARRICADE: mk_item(m, IT_COINS); { Item *b = add_item((int)m->x, (int)m->y, IT_BONE); if (b) b->secret = 1; } break;
    case MKK_LANTERN: for (int k = 0; k < 3; k++) spawn_mote(m->x + (rndf() - 0.5f) * 0.6f, m->y + (rndf() - 0.5f) * 0.6f, 1.0f); break;
    case MKK_COIL: spawn_mote(m->x, m->y, 1.5f); break;
    case MKK_CLIGHT: case MKK_PRISM: for (int k = 0; k < 2; k++) spawn_mote(m->x + (rndf() - 0.5f) * 0.6f, m->y + (rndf() - 0.5f) * 0.6f, 1.0f); break;
    case MKK_HSEAL: mk_item(m, rnd(2) ? IT_GEMS : IT_CHALICE); break;
    case MKK_BELL: if (m->grp == (int)(m - G.mk) + 1) mk_item(m, rnd(2) ? IT_COINS : IT_GEMS); break;   /* the first of the three */
    case MKK_WEB: mk_item(m, rnd(3) ? IT_COINS : IT_POTION); break;
    case MKK_CART: case MKK_LODE: mk_item(m, IT_GEMS); break;
    case MKK_GEODE: mk_item(m, IT_GEMS); mk_item(m, IT_GEMS); break;
    case MKK_WINCH: case MKK_VENT: spawn_mote(m->x, m->y, 0.5f); break;
    default: spawn_mote(m->x, m->y, 0.5f); break;                  /* the lights: a little light, and a little soul */
    }
}
static void mk_coin_check(void) {
    int all = 1;
    for (int i = 0; i < G.nmk; i++) all &= mk_answered(&G.mk[i]);
    if (all && mk_count() >= 3 && hash2(G.level, 4441) % 5 == 0) gain_coin(MK_COIN, "EVERY SEAL HERE ANSWERED");   /* Q11 */
}
static void add_burn(float x, float y, int foe);
static void mk_water_shock(Marked *w);
static void mk_fight(int k) {                                      /* section 4.5: the fight made different */
    Marked *m = &G.mk[k];
    switch (m->kind) {
    case MKK_CASK:                                                 /* the oil runs out burning: eight seconds of floor */
        for (int q = 0; q < 9; q++) {
            float a = q * 0.698f, r = q ? 1.1f : 0.0f;
            int x = (int)(m->x + cosf(a) * r), y = (int)(m->y + sinf(a) * r);
            if (G.map[y][x].kind == K_FLOOR) add_burn(x + 0.5f, y + 0.5f, 0);
        }
        explode(m->x, m->y); sfx_at(SFX_IGNITE, m->x, m->y, 1.3f);
        break;
    case MKK_PILLAR:                                               /* down on whatever stands by it */
        for (int j = 0; j < MAXMOBS; j++) {
            Mob *o = &G.mobs[j];
            if (!o->alive || o->state != MS_WALK || o->boss || o->ally || KIND_IS(o, KF_SPIRIT) || buried(o) || hypotf(o->x - m->x, o->y - m->y) > 1.8f) continue;
            o->stagger = fmaxf(o->stagger, 1.2f);
            damage_mob(o, 0.5f * o->maxhp, DM_ARC, m->x, m->y, 0.0f);
#ifdef CRAWL_DEBUG
            fprintf(stderr, "EVENT mk_crush %s t=%.2f\n", KINDS[o->type].name, G.t);
#endif
        }
        add_deco(m->x + 0.6f, m->y + 0.3f, SP_RUBBLE);
        sfx_at(SFX_RUMBLE, m->x, m->y, 1.4f); sfx_at(SFX_BONES, m->x, m->y, 1.0f);
        break;
    case MKK_PFONT: m->t = 30.0f; sfx_at(SFX_HISS, m->x, m->y, 0.9f); break;   /* brimming, half a minute */
    case MKK_DRUM:                                                 /* the file loses its order */
        for (int p = 0; p < G.npacks; p++) {
            Pack *pk = &G.packs[p];
            if (pk->kind != PACK_FILE || pk->broken) continue;
            int near = 0;
            for (int j = 0; j < MAXMOBS && !near; j++) near = G.mobs[j].alive && G.mobs[j].pack == p + 1 && hypotf(G.mobs[j].x - m->x, G.mobs[j].y - m->y) < 16.0f;
            if (!near) continue;
            pack_break(pk);
            for (int j = 0; j < MAXMOBS; j++) if (G.mobs[j].alive && G.mobs[j].pack == p + 1) G.mobs[j].stagger = fmaxf(G.mobs[j].stagger, 1.5f);
#ifdef CRAWL_DEBUG
            fprintf(stderr, "EVENT mk_drum file broken t=%.2f\n", G.t);
#endif
        }
        sfx_at(SFX_GROAN, m->x, m->y, 1.0f);
        break;
    case MKK_WATER: mk_water_shock(m); break;
    case MKK_ICEF: {                                               /* the floor about it rimed, twenty seconds */
        m->els[0] = G.ndecos; m->els[1] = 0; m->t = 20.0f;
        for (int dy = -1; dy <= 1; dy++) for (int dx = -1; dx <= 1; dx++) {
            int x = (int)m->x + dx, y = (int)m->y + dy;
            if (G.map[y][x].kind != K_FLOOR || G.ndecos >= MAXDECO) continue;
            add_deco_ex(x + 0.5f, y + 0.5f, SP_MK_ICE, 1, 0); m->els[1]++;
        }
        sfx_at(SFX_FROST, m->x, m->y, 1.2f);
        break;
    }
    default: break;
    }
}
static void mk_finish(int k) {
    Marked *m = &G.mk[k];
    int first = !m->times;
    m->done = 1; m->done_t = 0.0f; m->times++;
    switch (m->kind) {
    case MKK_CLIGHT: case MKK_WEB: mk_deco(m, -1, 1); break;       /* gone into motes; burnt away */
    case MKK_RSKULL: m->st = 2; mk_deco(m, MKK[m->kind].s0, 1); break;
    case MKK_CART: mk_deco(m, SP_MK_CART0, 1); break;
    case MKK_HEARTH: if (m->door >= 0) { Door *d = &G.doors[m->door]; d->locked = 0; d->opening = 1; sfx_at(SFX_RUMBLE, d->x + 0.5f, d->y + 0.5f, 1.0f); } mk_deco(m, MKK[m->kind].s1, 1); break;
    case MKK_VENT: m->t = 0.0f; mk_deco(m, MKK[m->kind].s1, 1); break;
    case MKK_WSTONE: if (m->door >= 0) { Door *d = &G.doors[m->door]; d->locked = 0; d->opening = 1; sfx_at(SFX_RUMBLE, d->x + 0.5f, d->y + 0.5f, 1.0f); } mk_deco(m, MKK[m->kind].s1, 1); break;
    case MKK_RSEAL: mk_deco(m, MKK[m->kind].s1, 1); ice_down(); break;   /* the tutorial's: the ice goes down */
    case MKK_GSEAL: if (m->door >= 0) { Door *d = &G.doors[m->door]; d->locked = 0; d->opening = 1; sfx_at(SFX_RUMBLE, d->x + 0.5f, d->y + 0.5f, 1.2f); } break;   /* up for good */
    default: mk_deco(m, MKK[m->kind].s1, 1); break;
    }
    mk_fight(k);                                                   /* what a thing for fights does */
    if (first) {
        mk_reward(m);
        xp_gain((4.0f + 2.0f * (float)m->pips) * xp_level(), mk_name(m));
        aug_credit(m->el, 2);                                      /* a thing struck with the line: two of its credits (A3) */
    }
    sfx_at(SFX_SHIMMER, m->x, m->y, 1.2f);
    sfx_at(m->kind == MKK_COFFER || m->kind == MKK_CAGE || m->kind == MKK_CART ? SFX_COIN : m->kind == MKK_HSEAL || m->kind == MKK_GEODE ? SFX_SHATTER
           : m->kind == MKK_VENT ? SFX_FROST : m->kind == MKK_WEB ? SFX_IGNITE : SFX_RUNE, m->x, m->y, 1.0f);
    if (m->kind == MKK_CAGE || m->kind == MKK_BARRICADE || m->kind == MKK_RSKULL) sfx_at(SFX_BONES, m->x, m->y, 1.2f);
    if (G.player && (!m->grp || m->grp == k + 1)) say("%s", MKK[m->kind].done);
    mk_coin_check();
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT mk_done %s %s by %s t=%.2f\n", MKK[m->kind].name, ELEM_NAME[m->el], G.player ? "player" : "autopilot", G.t);
#endif
}
static void mk_gate_up(int k) {                                    /* the winch: the portcullis rises for a while */
    Marked *m = &G.mk[k];
    Door *d = &G.doors[m->door];
    if (!m->times) { mk_reward(m); xp_gain((4.0f + 2.0f * (float)m->pips) * xp_level(), mk_name(m)); if (G.player) say("%s", MKK[m->kind].done); }
    m->times++; m->st = 1; m->t = GATE_UP; m->pips = 8; m->got = 8; m->hit_t = 0.5f;
    d->locked = 0; d->opening = 1;
    mk_deco(m, MKK[m->kind].s1, 1);
    sfx_at(SFX_RUMBLE, d->x + 0.5f, d->y + 0.5f, 1.0f); sfx_at(SFX_ZAP, m->x, m->y, 0.8f);
    mk_coin_check();
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT mk_gate up %s by %s t=%.2f\n", mk_name(m), G.player ? "player" : "autopilot", G.t);
#endif
}
static void mk_wrong(Marked *m, int el, const char *why) {          /* the dull ring, the sparks thrown back */
    m->wrong_t = 0.7f; m->hit_t = 0.0f;
    sfx_at(SFX_KNOCK, m->x, m->y, 0.8f);
    if (G.player && !(m->said & 2)) { m->said |= 2; if (why) say("%s", why); else say("IT WANTS %s", ELEM_NAME[m->el]); }
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT mk_wrong %s wants %s got %s t=%.2f\n", MKK[m->kind].name, ELEM_NAME[m->el], el >= 0 ? ELEM_NAME[el] : "none", G.t);
#endif
}
static void mk_finish(int k);
static void relay_check(int lead) {                                /* every lamp of the relay lit: done */
    for (int i = lead; i < G.nmk; i++) if (G.mk[i].grp == lead + 1 && G.mk[i].st != 1) return;
    for (int i = lead; i < G.nmk; i++) if (G.mk[i].grp == lead + 1) mk_finish(i);
}
static void mk_strike(int k, int el) {
    Marked *m = &G.mk[k];
    G.lull_t = 0.0f;                                               /* a thing struck is something happening */
    if (m->done || m->kind == MKK_TABLET) return;
    if (m->kind == MKK_RSEAL || m->kind == MKK_GSEAL) el = m->el;  /* the tutorial's switch and gate: any spell answers them */
    if (m->kind == MKK_PLINTH) {                                   /* the tablet's order */
        if (el != m->el) { mk_wrong(m, el, NULL); return; }
        Marked *t = &G.mk[m->grp - 1];
        if (m->got) { m->hit_t = 0.3f; return; }                 /* already lit: it stays so */
        if (t->st < 3 && t->els[t->st] == m->el) {
            m->got = 1; m->hit_t = 0.5f; t->st++; t->got = t->st; t->hit_t = 0.5f;
            mk_deco(m, MKK[m->kind].s1, 1); sfx_at(SFX_RUNE, m->x, m->y, 1.0f);
#ifdef CRAWL_DEBUG
            fprintf(stderr, "EVENT mk_hit TABLET %d/3 t=%.2f\n", t->st, G.t);
#endif
            if (t->st >= 3) for (int i = m->grp - 1; i < G.nmk; i++) if (G.mk[i].grp == m->grp) mk_finish(i);
            return;
        }
        for (int i = m->grp - 1; i < G.nmk; i++)                   /* out of turn: all of it again */
            if (G.mk[i].grp == m->grp) { G.mk[i].got = 0; if (G.mk[i].kind == MKK_PLINTH) mk_deco(&G.mk[i], MKK[MKK_PLINTH].s0, 1); }
        t->st = 0; t->got = 0;
        sfx_at(SFX_GROAN, m->x, m->y, 1.0f);
        mk_wrong(m, el, "OUT OF TURN: THE STONE GROANS");
        return;
    }
    if (m->kind == MKK_RELIQ) {                                    /* the face towards you, three right running */
        if (m->st == 1) return;                                    /* turning: it glances off */
        if (el != m->el) { m->got = 0; mk_wrong(m, el, NULL); return; }
        m->got++; m->hit_t = 0.5f; sfx_at(SFX_RUNE, m->x, m->y, 0.9f);
        if (m->got >= m->pips) mk_finish(k);
        return;
    }
    if (m->kind == MKK_WSTONE) {                                   /* a new glyph after each right hit */
        if (el != m->el) { mk_wrong(m, el, NULL); return; }
        m->got++; m->hit_t = 0.5f; sfx_at(SFX_RUNE, m->x, m->y, 0.9f);
#ifdef CRAWL_DEBUG
        fprintf(stderr, "EVENT mk_hit %s %d/%d t=%.2f\n", MKK[m->kind].name, m->got, m->pips, G.t);
#endif
        if (m->got >= m->pips) { mk_finish(k); return; }
        int held[NELEM], nh = mk_held(held);
        for (int i = 0; i < nh; i++) if (held[i] != m->el) { m->el = held[i]; break; }
        return;
    }
    if (m->grp && (m->kind == MKK_RELAY || m->kind == MKK_COIL)) {   /* the relay's lamps and its coil */
        int want = m->kind == MKK_COIL ? EL_LIGHT : EL_FIRE;
        if (el != want) { mk_wrong(m, el, NULL); return; }
        m->hit_t = 0.5f;
#ifdef CRAWL_DEBUG
        fprintf(stderr, "EVENT mk_hit RELAY %s st %d t=%.2f\n", MKK[m->kind].name, m->st, G.t);
#endif
        if (m->st == 2) { m->st = 0; mk_deco(m, SP_MK_RELAY0, 1); sfx_at(SFX_HISS, m->x, m->y, 0.8f); return; }   /* thawed */
        if (m->st == 0) { m->st = 1; m->got = 1; mk_deco(m, MKK[m->kind].s1, 1); sfx_at(m->kind == MKK_COIL ? SFX_ZAP : SFX_IGNITE, m->x, m->y, 0.9f); }
        relay_check(m->grp - 1);
        return;
    }
    if (m->kind == MKK_HSEAL && el == EL_FIRE) {                   /* heated: frost now, while it glows */
        if (!m->st) { m->st = 1; m->el = EL_FROST; mk_deco(m, SP_MK_HSEAL1, 1); sfx_at(SFX_IGNITE, m->x, m->y, 0.9f); }
        m->t = HSEAL_HOT; m->hit_t = 0.5f; m->wrong_t = 0.0f;
        if (G.player && !(m->said & 4)) { m->said |= 4; say("IT GLOWS: NOW FROST"); }
#ifdef CRAWL_DEBUG
        fprintf(stderr, "EVENT mk_heat %s t=%.2f\n", MKK[m->kind].name, G.t);
#endif
        return;
    }
    if (m->kind == MKK_WINCH && m->st == 1 && el == EL_LIGHT) { m->t = GATE_UP; m->got = 8; m->hit_t = 0.5f; sfx_at(SFX_ZAP, m->x, m->y, 0.7f); return; }
    if (el != m->el || (m->kind == MKK_WINCH && m->st == 1)) {     /* the wrong spell: a dull ring, the sparks thrown back */
        m->wrong_t = 0.7f; m->hit_t = 0.0f;
        sfx_at(SFX_KNOCK, m->x, m->y, 0.8f);
        if (G.player && !(m->said & 2)) { m->said |= 2; say(m->kind == MKK_HSEAL ? "IT WANTS FIRE FIRST" : "IT WANTS %s", ELEM_NAME[m->el]); }
#ifdef CRAWL_DEBUG
        fprintf(stderr, "EVENT mk_wrong %s wants %s got %s t=%.2f\n", MKK[m->kind].name, ELEM_NAME[m->el], el >= 0 ? ELEM_NAME[el] : "none", G.t);
#endif
        return;
    }
    m->hit_t = 0.5f; m->wrong_t = 0.0f;
    if (m->kind == MKK_BELL) {                                     /* it rings a while: the three at once answer them */
        m->got = 1; m->t = BELL_RING;
        mk_deco(m, MKK[m->kind].s1, 1);
        sfx_at(SFX_HANDBELL, m->x, m->y, 0.9f);
        int g = m->grp - 1, ringing = 0;
        for (int i = g; i < g + 3 && i < G.nmk; i++) ringing += G.mk[i].grp == m->grp && G.mk[i].got > 0;
#ifdef CRAWL_DEBUG
        fprintf(stderr, "EVENT mk_hit %s %d/3 t=%.2f\n", MKK[m->kind].name, ringing, G.t);
#endif
        if (ringing >= 3) for (int i = g; i < g + 3 && i < G.nmk; i++) mk_finish(i);
        return;
    }
    m->got++; m->t = 0.0f;
    sfx_at(m->kind == MKK_HEARTH ? SFX_CRACKLE : SFX_RUNE, m->x, m->y, 0.9f);
    if (m->kind == MKK_HEARTH) mk_deco(m, MKK[m->kind].s1, 1);
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT mk_hit %s %d/%d t=%.2f\n", MKK[m->kind].name, m->got, m->pips, G.t);
#endif
    if (m->got >= m->pips) { if (m->kind == MKK_WINCH) mk_gate_up(k); else mk_finish(k); }
}
static float mk_z(const Marked *m) {                                /* the seal's height */
    const MkKind *K = &MKK[m->kind];
    float z = 1.0f - (float)(m->done ? K->dy : K->sy) / 64.0f;
    if (m->kind == MKK_CLIGHT) z += 0.05f * sinf((float)G.t * 2.3f + m->hx);   /* it bobs */
    return z;
}
static int mk_aimed(void) {                                        /* the thing a cast with no creature to strike goes at */
    int best = -1; float bd = 13.0f;
    for (int i = 0; i < G.nmk; i++) {
        const Marked *m = &G.mk[i];
        if (m->done || m->kind == MKK_TABLET || (m->kind == MKK_BELL && m->got)) continue;
        float d = hypotf(m->x - G.px, m->y - G.py);
        if (d >= bd || fabsf(wrap_pi(atan2f(m->y - G.py, m->x - G.px) - G.ang)) > atanf(0.45f / fmaxf(d, 0.3f)) + 0.06f) continue;
        if (!line_clear(G.px, G.py, m->x, m->y)) continue;
        bd = d; best = i;
    }
    return best;
}
static int mk_shot(Shot *s) {                                      /* a missile meeting a thing: it stops there, unless it pierces */
    if (s->tier < 0 || s->kind == SHOT_REFLECT || s->kind == SHOT_WEB) return 0;
    if (s->kind == AT_FLASK) {                                     /* the flask goes over: what wants poison takes it as it passes */
        for (int i = 0; i < G.nmk; i++) {
            Marked *m = &G.mk[i];
            if (m->done || m->el != EL_POISON || (s->mkhit >> i & 1) || hypotf(m->x - s->x, m->y - s->y) > 0.6f) continue;
            s->mkhit |= (uint16_t)(1u << i);
            mk_strike(i, EL_POISON);
        }
        return 0;
    }
    int pierce = s->kind == AT_SPEAR || s->kind == SHOT_LANCE || s->kind == SHOT_BALLISTA;
    for (int i = 0; i < G.nmk; i++) {
        Marked *m = &G.mk[i];
        if (m->done || m->kind == MKK_TABLET || (s->mkhit >> i & 1) || (m->kind == MKK_BELL && m->got) || hypotf(m->x - s->x, m->y - s->y) > 0.42f) continue;   /* a bell swinging: it goes under */
        s->mkhit |= (uint16_t)(1u << i);
        mk_strike(i, shot_elem(s));
        if (!pierce) return 1;
    }
    return 0;
}
static void mk_water_shock(Marked *w) {                            /* lightning in the standing water: all that stands wet, stunned */
    static int busy;
    if (busy) return;
    busy = 1;
    int n = 0;
    for (int j = 0; j < MAXMOBS; j++) {
        Mob *o = &G.mobs[j];
        if (!o->alive || o->state != MS_WALK || o->ally || o->boss || KIND_IS(o, KF_SPIRIT) || buried(o) || o->type == MOB_BAT || o->type == MOB_GARGOYLE) continue;
        if (!on_water((int)o->x, (int)o->y) || hypotf(o->x - w->x, o->y - w->y) > 8.0f) continue;
        o->stagger = fmaxf(o->stagger, 1.0f); o->slowed = fmaxf(o->slowed, 1.5f);
        damage_mob(o, 0.12f * o->maxhp, DM_ARC | DM_ATK(AT_CHAIN), w->x, w->y, 0.0f);
        n++;
    }
    sfx_at(SFX_ZAP, w->x, w->y, 1.3f);
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT mk_water_shock %d t=%.2f\n", n, G.t);
#endif
    busy = 0;
}
static void mk_wet_hit(const Mob *o) {                            /* lightning on a creature standing in the water: the water takes it */
    if (!on_water((int)o->x, (int)o->y)) return;
    for (int i = 0; i < G.nmk; i++)
        if (G.mk[i].kind == MKK_WATER && !G.mk[i].done && hypotf(G.mk[i].x - o->x, G.mk[i].y - o->y) < 8.0f) { mk_strike(i, EL_LIGHT); return; }
}
static void mk_burst(float x, float y, int el, unsigned skip) {   /* a flask's burst reaches what is within a cell of it */
    for (int i = 0; i < G.nmk; i++) if (!G.mk[i].done && !(skip >> i & 1) && hypotf(G.mk[i].x - x, G.mk[i].y - y) < 1.1f) mk_strike(i, el);
}
static int mk_door_busy(const Door *d) {
    if (door_body(d)) return 1;                                    /* the caster anywhere in its doorway (1.0.0b18.1) */
    for (int j = 0; j < MAXMOBS; j++)
        if (G.mobs[j].alive && ((G.mobs[j].cx == d->x && G.mobs[j].cy == d->y) || (G.mobs[j].tx == d->x && G.mobs[j].ty == d->y))) return 1;
    return 0;
}
static void mk_roll_next(Marked *m) {                              /* the rolling skull's next cell: away from you, never back */
    int cx = (int)m->x, cy = (int)m->y, bx = -1, by = -1; float bs = -1e9f;
    for (int k = 0; k < 4; k++) {
        int x = cx + DX[k], y = cy + DY[k];
        if (G.map[y][x].kind != K_FLOOR || (x == (int)m->hx && y == (int)m->hy)) continue;
        float sc = hypotf(x + 0.5f - G.px, y + 0.5f - G.py) + rndf() * 0.4f;
        if (sc > bs) { bs = sc; bx = x; by = y; }
    }
    m->hx = cx + 0.5f; m->hy = cy + 0.5f;                          /* where it came from */
    if (bx < 0) { m->vx = -1.0f; return; }
    m->vx = bx + 0.5f; m->vy = by + 0.5f;
}
static void mk_tick(float dt) {
    for (int i = 0; i < G.nmk; i++) {
        Marked *m = &G.mk[i];
        m->hit_t = fmaxf(0.0f, m->hit_t - dt); m->wrong_t = fmaxf(0.0f, m->wrong_t - dt);
        float pd = hypotf(m->x - G.px, m->y - G.py);
        if (m->done) {
            m->done_t += dt;
            if (m->kind == MKK_VENT && m->done_t >= VENT_ICE) {     /* the ice gives: it vents again */
                m->done = 0; m->got = 0; m->t = 0.0f; mk_deco(m, MKK[m->kind].s0, 1);
                sfx_at(SFX_HISS, m->x, m->y, 0.7f);
            }
            if (m->kind == MKK_PFONT && m->t > 0.0f) {             /* the font brimming: what comes near it sickens */
                float was = m->t; m->t -= dt;
                if ((int)was != (int)m->t)
                    for (int j = 0; j < MAXMOBS; j++) {
                        Mob *o = &G.mobs[j];
                        if (o->alive && o->state == MS_WALK && !o->ally && !KIND_IS(o, KF_SPIRIT) && !buried(o) && hypotf(o->x - m->x, o->y - m->y) < 1.7f) mob_poison(o, 0.05f * o->maxhp, 3.0f);
                    }
                if (m->t <= 0.0f) mk_deco(m, MKK[m->kind].s0, 1);   /* it drains away */
            }
            if (m->kind == MKK_ICEF && m->t > 0.0f) {              /* the rime: what crosses it slides and slows */
                m->t -= dt;
                for (int j = 0; j < MAXMOBS; j++) {
                    Mob *o = &G.mobs[j];
                    if (o->alive && o->state == MS_WALK && !o->ally && fabsf(o->x - m->x) < 1.5f && fabsf(o->y - m->y) < 1.5f) o->slowed = fmaxf(o->slowed, 0.6f);
                }
                if (m->t <= 0.0f) {                                /* thawed: it may be rimed again */
                    for (int q = 0; q < m->els[1]; q++) if (m->els[0] + q < G.ndecos) G.decos[m->els[0] + q].spr = -1;
                    m->done = 0; m->got = 0;
                }
            }
            if ((m->kind == MKK_WATER && m->done_t >= 6.0f) || (m->kind == MKK_DAMP && m->done_t >= 20.0f)) { m->done = 0; m->got = 0; }   /* again */
            continue;
        }
        if (G.player && !(m->said & 1) && pd < 3.5f && line_clear(G.px, G.py, m->x, m->y) && (!m->grp || m->grp == i + 1)) {
            m->said |= 1;                                          /* a person's first look at it */
            if (m->kind == MKK_HSEAL) say("%s: FIRE, THEN FROST", mk_name(m));
            else if (m->kind == MKK_TABLET) say("A CARVED TABLET: STRIKE THE PLINTHS IN ITS ORDER");
            else if (m->kind == MKK_RELIQ) say("A TURNING RELIQUARY: THE FACE TOWARDS YOU, THREE RIGHT");
            else if (m->kind == MKK_WSTONE) say("A WARD-STONE: ITS GLYPH CHANGES AS IT IS STRUCK");
            else if (m->kind == MKK_RELAY) say("DEAD LAMPS: FIRE PASSES DOWN THEM");
            else if (m->kind == MKK_BELL) say("THREE BELLS: ALL THREE AT ONCE, WITH %s", ELEM_NAME[m->el]);
            else say("%s: IT WANTS %s", mk_name(m), ELEM_NAME[m->el]);
        }
        switch (m->kind) {
        case MKK_CLIGHT: {                                         /* it drifts about where it was, and shies off */
            float gx, gy, sp;
            if (pd < 3.0f && pd > 0.01f) { gx = m->x + (m->x - G.px) / pd; gy = m->y + (m->y - G.py) / pd; sp = 1.5f; }
            else {
                if ((m->t -= dt) <= 0.0f) { m->t = 2.0f + 2.0f * rndf(); m->vx = m->hx + (rndf() - 0.5f) * 4.0f; m->vy = m->hy + (rndf() - 0.5f) * 4.0f; }
                gx = m->vx; gy = m->vy; sp = 0.5f;
            }
            float ex = gx - m->x, ey = gy - m->y, e = hypotf(ex, ey);
            if (e > 0.05f) {
                float st = fminf(e, sp * dt), nx = m->x + ex / e * st, ny = m->y + ey / e * st;
                if (G.map[(int)ny][(int)nx].kind == K_FLOOR && hypotf(nx - m->hx, ny - m->hy) < 3.5f) { m->x = nx; m->y = ny; }
                else m->t = 0.0f;
                mk_deco(m, SP_MK_CLIGHT0, 2);
            }
            break;
        }
        case MKK_RSKULL:                                           /* seen, it rolls away; seven cells on, it drops */
            if (m->st == 0 && pd < 7.0f && pd > 1.2f && line_clear(G.px, G.py, m->x, m->y)) {
                m->st = 1; m->t = 0.0f; m->hx = m->x; m->hy = m->y; mk_roll_next(m);
                mk_deco(m, SP_MK_RSKULL0, 2);
                sfx_at(SFX_BONES, m->x, m->y, 0.6f);
            } else if (m->st == 1) {
                if (m->vx < 0.0f) m->t = 99.0f;
                else {
                    float ex = m->vx - m->x, ey = m->vy - m->y, e = hypotf(ex, ey), st = 1.5f * dt;
                    if (e > st) { m->x += ex / e * st; m->y += ey / e * st; }
                    else { m->x = m->vx; m->y = m->vy; m->t += 1.0f; if (m->t < 7.0f) mk_roll_next(m); }
                    mk_deco(m, SP_MK_RSKULL0, 2);
                }
                if (m->t >= 7.0f) {                                /* into a crack, and gone */
                    m->done = 2; m->done_t = 99.0f; mk_deco(m, -1, 1);
                    sfx_at(SFX_KNOCK, m->x, m->y, 0.6f);
                    if (G.player && pd < 10.0f) say("THE SKULL DROPS INTO A CRACK");
#ifdef CRAWL_DEBUG
                    fprintf(stderr, "EVENT mk_lost %s t=%.2f\n", MKK[m->kind].name, G.t);
#endif
                }
            }
            break;
        case MKK_HSEAL:
            if (m->st == 1 && (m->t -= dt) <= 0.0f) { m->st = 0; m->el = EL_FIRE; mk_deco(m, SP_MK_HSEAL0, 1); sfx_at(SFX_HISS, m->x, m->y, 0.5f); }
            break;
        case MKK_BELL:
            if (m->got && (m->t -= dt) <= 0.0f) { m->got = 0; mk_deco(m, MKK[m->kind].s0, 1); }
            break;
        case MKK_WINCH:                                            /* the gate up: it counts down, then drops once the way is clear */
            if (m->st == 1) {
                m->t -= dt; m->got = imax(0, (int)ceilf(m->t));
                Door *d = &G.doors[m->door];
                if (m->t <= 0.0f && !mk_door_busy(d)) {
                    d->locked = GATED; d->opening = 0; d->idle = 99.0f;
                    m->st = 0; m->pips = 1; m->got = 0; mk_deco(m, MKK[m->kind].s0, 1);
                    sfx_at(SFX_RUMBLE, d->x + 0.5f, d->y + 0.5f, 0.8f);
                }
            }
            break;
        case MKK_HEARTH: case MKK_LODE:                            /* fed, or charged: it falls back if left */
            if (m->got > 0 && (m->t += dt) >= (m->kind == MKK_HEARTH ? 10.0f : 6.0f)) {
                m->got--; m->t = 0.0f;
                sfx_at(m->kind == MKK_HEARTH ? SFX_HISS : SFX_ZAP, m->x, m->y, 0.4f);
                if (m->kind == MKK_HEARTH && !m->got) mk_deco(m, MKK[m->kind].s0, 1);
            }
            break;
        case MKK_VENT: {                                           /* quiet, a hiss, then scalding steam beside it */
            float was = m->t;
            m->t = fmodf(m->t + dt, VENT_CYCLE);
            if (was < VENT_WARN && m->t >= VENT_WARN) sfx_at(SFX_HISS, m->x, m->y, 0.8f);
            if (m->t < was) m->st = 0;                             /* a new round: it may scald again */
            if (m->t >= VENT_ON && !m->st && !G.dead && pd < 1.15f) { m->st = 1; hz_strike_el(HZ_FISSURE, EL_FIRE, 0.06f, m->x, m->y); }
            break;
        }
        case MKK_PRICKET: case MKK_COIL:                           /* the lull's: lit a moment, then out */
            if (m->st == 3 && (m->t -= dt) <= 0.0f) { m->st = 0; mk_deco(m, MKK[m->kind].s0, 1); sfx_at(SFX_HISS, m->x, m->y, 0.5f); }
            if (m->kind == MKK_COIL && m->grp && m->st == 1) {       /* a relay's coil passes the flame, below */
                for (int q = i + 1; q < G.nmk; q++) {
                    Marked *n2 = &G.mk[q];
                    if (n2->grp != m->grp) continue;
                    if (n2->kind == MKK_RELAY && n2->st == 0 && !n2->done && (n2->t += dt) >= 0.8f) {
                        n2->st = 1; n2->got = 1; n2->t = 0.0f; mk_deco(n2, MKK[MKK_RELAY].s1, 1); sfx_at(SFX_IGNITE, n2->x, n2->y, 0.8f);
                        relay_check(m->grp - 1);
                    }
                    break;
                }
            }
            break;
        case MKK_RELIQ:                                            /* it turns: a face, then the next */
            if ((m->t -= dt) <= 0.0f) {
                if (m->st == 0) { m->st = 1; m->t = 0.7f; mk_deco(m, SP_MK_RELIQ1, 1); }
                else { m->st = 0; m->t = 3.5f; m->hx += 1.0f; m->el = m->els[(int)m->hx % 3]; mk_deco(m, SP_MK_RELIQ0, 1); sfx_at(SFX_PUSHWALL, m->x, m->y, 0.3f); }
            }
            break;
        case MKK_RELAY:                                            /* lit, it passes the flame to the next */
            if (!m->grp || m->st != 1) break;
            for (int q = i + 1; q < G.nmk; q++) {
                Marked *n2 = &G.mk[q];
                if (n2->grp != m->grp) continue;
                if (n2->kind == MKK_RELAY && n2->st == 0 && !n2->done && (n2->t += dt) >= 0.8f) {
                    n2->st = 1; n2->got = 1; n2->t = 0.0f; mk_deco(n2, MKK[MKK_RELAY].s1, 1); sfx_at(SFX_IGNITE, n2->x, n2->y, 0.8f);
                    relay_check(m->grp - 1);
                }
                break;                                             /* only the next in line */
            }
            break;
        case MKK_CART:                                             /* to and fro on its rails */
            m->t += (m->st ? -0.9f : 0.9f) * dt;
            if (m->t >= 4.0f) { m->t = 4.0f; m->st = 1; } else if (m->t <= 0.0f) { m->t = 0.0f; m->st = 0; }
            m->x = m->hx + m->vx * m->t; m->y = m->hy + m->vy * m->t;
            mk_deco(m, SP_MK_CART0, 2);
            break;
        }
    }
}
static void mk_dmarks(void) {                                      /* the vents' steam on the autopilot's map: a hiss warns, steam harms */
    for (int i = 0; i < G.nmk; i++) {
        const Marked *m = &G.mk[i];
        if (m->kind != MKK_VENT || m->done || m->t < VENT_WARN - 0.4f) continue;
        int c = m->t >= VENT_ON ? 2 : 1, x = (int)m->x, y = (int)m->y;
        hz_dput(x, y, c);
        for (int d = 0; d < 4; d++) if (G.map[y + DY[d]][x + DX[d]].kind == K_FLOOR) hz_dput(x + DX[d], y + DY[d], c);
    }
}

/* The lull's watch (docs/PLAN-play.md section 5, step 9): a person who has walked thirty seconds with no fight, no find,
 * nothing struck and no creature seen is offered something, a minute apart at least, never the dream: a corpse-light
 * drifting in ahead, a cold pricket (or a coil) that lights itself a moment further on and goes out, or a far-off bell
 * and something coming by. */
#define LULL_T 30.0f
#define LULL_CD 60.0f
#ifdef CRAWL_DEBUG
static int lulls;
#endif
static int lull_spot(int dmin, int dmax, int seen, int *ox, int *oy) {   /* ahead if it can be; out of sight, or in it */
    float fx = cosf(G.ang), fy = sinf(G.ang), best = seen ? 0.6f : -2.0f;
    int found = 0;
    for (int tries = 0; tries < (seen ? 40 : 10); tries++) {
        int x, y;
        if (!pick_cell_at_depth(G.cx, G.cy, dmin, dmax, !seen, &x, &y)) continue;
        if (!mk_free(x, y, 1) || mk_at(x, y, 3) || in_wing(x, y) || in_trial(x, y)) continue;
        if (seen && !line_clear(G.px, G.py, x + 0.5f, y + 0.5f)) continue;
        float dx = x + 0.5f - G.px, dy = y + 0.5f - G.py, d = hypotf(dx, dy), dot = d > 0.1f ? (dx * fx + dy * fy) / d : 0.0f;
        if (dot > best) { best = dot; *ox = x; *oy = y; found = 1; }
    }
    return found;
}
static void lull_offer(void) {
    int held[NELEM], nh = mk_held(held), x, y, kind = rnd(3);
    if (!nh) return;
#ifdef CRAWL_DEBUG
    if (getenv("CRAWL_LULLK")) kind = clampi(atoi(getenv("CRAWL_LULLK")), 0, 2);   /* 0 the corpse-light, 1 the pricket, 2 the bell */
#endif
    if (kind == 1 && !align_open(EL_FIRE) && !align_open(EL_LIGHT)) kind = 0;
    if (kind < 2 && G.nmk >= MAXMK) kind = 2;
    if (kind == 1 && !lull_spot(5, 12, 1, &x, &y)) kind = 0;        /* the pricket in sight, further on */
    if (kind == 2 && (!mind_on() || !lull_spot(16, 26, 0, &x, &y))) kind = 0;
    if (kind == 0 && !lull_spot(6, 12, 0, &x, &y)) return;
    G.lull_cd = LULL_CD; G.lull_t = 0.0f;
    if (kind == 0) {                                               /* a corpse-light drifts in ahead */
        Marked *m = mk_add(MKK_CLIGHT, held[0], x, y);
        if (m) sfx_at(SFX_WHISPER, m->x, m->y, 0.7f);
    } else if (kind == 1) {                                        /* a cold pricket lights itself a moment, and goes out */
        int fire = align_open(EL_FIRE);
        Marked *m = mk_add(fire ? MKK_PRICKET : MKK_COIL, fire ? EL_FIRE : EL_LIGHT, x, y);
        if (m) { m->st = 3; m->t = 1.6f; mk_deco(m, MKK[m->kind].s1, 1); sfx_at(fire ? SFX_IGNITE : SFX_ZAP, m->x, m->y, 0.8f); }
    } else {                                                       /* a far-off bell, and something comes by */
        sfx_at(SFX_HANDBELL, x + 0.5f, y + 0.5f, 1.0f);
        int alive = 0;
        for (int i = 0; i < MAXMOBS; i++) alive += G.mobs[i].alive && !KIND_IS(&G.mobs[i], KF_UNCOUNTED);
        if (alive < 12) {
            uint64_t was = 0;
            for (int i = 0; i < MAXMOBS; i++) was |= (uint64_t)(G.mobs[i].alive != 0) << i;
            spawn_wanderer(x, y);
            for (int i = 0; i < MAXMOBS; i++) if (G.mobs[i].alive && !(was >> i & 1) && minded(&G.mobs[i])) mind_track(&G.mobs[i], G.px, G.py);
        }
    }
#ifdef CRAWL_DEBUG
    lulls++;
    fprintf(stderr, "EVENT lull %s at (%d,%d) t=%.2f\n", kind == 0 ? "corpse-light" : kind == 1 ? "pricket" : "bell", x, y, G.t);
#endif
}
static void lull_tick(float dt) {
#ifdef CRAWL_DEBUG
    static double lull_why;
    if (getenv("CRAWL_LULLTRACE") && G.t >= lull_why) { lull_why = G.t + 5.0; fprintf(stderr, "LULL gate player=%d dead=%d ui=%d exiting=%d ok=%d\n", G.player, G.dead, G.ui, G.exiting, mk_level_ok()); }
#endif
    if (!G.player || G.dead || G.ui || G.exiting || !mk_level_ok() || g_drawn) { G.lull_t = 0.0f; return; }   /* not on a drawn level */
    G.lull_cd = fmaxf(0.0f, G.lull_cd - dt);
    int quiet = !in_combat() && G.items_got == G.lull_items && G.links_found == G.lull_links;
    if (quiet && (G.lull_look -= dt) <= 0.0f) {                    /* a creature in sight ends it too */
        G.lull_look = 0.5f;
        for (int i = 0; i < MAXMOBS && quiet; i++) {
            const Mob *m = &G.mobs[i];
            if (!m->alive || m->state != MS_WALK || buried(m) || KIND_IS(m, KF_UNCOUNTED) || hypotf(m->x - G.px, m->y - G.py) > 9.0f) continue;
            if (m->alert > 0.0f || line_clear(G.px, G.py, m->x, m->y)) quiet = 0;
        }
    }
#ifdef CRAWL_DEBUG
    static double lull_q;
    if (getenv("CRAWL_LULLTRACE") && G.t >= lull_q) { lull_q = G.t + 2.0; fprintf(stderr, "LULL quiet=%d combat=%d items=%d/%d links=%d/%d\n", quiet, in_combat(), G.items_got, G.lull_items, G.links_found, G.lull_links); }
#endif
    G.lull_items = G.items_got; G.lull_links = G.links_found;
    if (!quiet) { G.lull_t = 0.0f; return; }
    if (G.pvx * G.pvx + G.pvy * G.pvy > 0.09f) G.lull_t += dt;     /* walking: standing still only waits */
    float need = LULL_T;
#ifdef CRAWL_DEBUG
    if (getenv("CRAWL_LULL")) need = (float)atof(getenv("CRAWL_LULL"));
#endif
    if (G.lull_t >= need && G.lull_cd <= 0.0f) lull_offer();
#ifdef CRAWL_DEBUG
    static double lull_say;
    if (getenv("CRAWL_LULLTRACE") && G.t >= lull_say) { lull_say = G.t + 2.0; fprintf(stderr, "LULL t=%.1f quiet=%d lull=%.1f cd=%.1f v=%.2f\n", G.t, quiet, G.lull_t, G.lull_cd, hypotf(G.pvx, G.pvy)); }
#endif
}

