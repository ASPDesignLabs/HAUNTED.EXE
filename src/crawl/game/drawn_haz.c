/* A part of scene_crawl.c (docs/PLAN-tidy.md): it is #included there in its place and never compiled on its own.
 * A drawn level's hazards (docs/PLAN-events.md, 2c): the game's own kinds at the drawing's anchors and cells. Each is live or not, set
 * off by its clock, by its box walked into, or by an event; the fixed kinds stand as sites (G.site, update_sites runs them), the rest
 * arise as the game lays them (G.haz, update_hazards). Their state is DH, in drawn.c: the level's, never saved. */

enum { DW_CLOCK, DW_ENTER, DW_TRIGGER };
#ifdef CRAWL_DEBUG
static const char *const DW_NAME[3] = { "clock", "enter", "trigger" };
#endif
static int dh_on_level(void) { return g_drawn && DH.n && !PK.depth && !G.shop; }
static int dh_site_kind(int k) {                                   /* the fixed kinds: a site of their own */
    return k == HZ_CENSER || k == HZ_BREATH || k == HZ_COLD || k == HZ_SLUICE || k == HZ_SNARE || k == HZ_TIMBER || k == HZ_DAMP
        || k == HZ_CART || k == HZ_LODE || k == HZ_FISSURE || k == HZ_CRYSTAL || k == HZ_DCOLD || k == HZ_RELIQ || k == HZ_SAINT
        || k == HZ_TITHE || k == HZ_MIRROR;
}
static int dh_wall_kind(int k) {                                   /* set in a wall: the anchor is the stone */
    return k == HZ_BREATH || k == HZ_COLD || k == HZ_SAINT || k == HZ_LODE || k == HZ_MIRROR || k == HZ_SLUICE || k == HZ_FISSURE;
}
static int dh_by_itself(int k) {                                   /* sites with no turns: they run while live */
    return k == HZ_CENSER || k == HZ_SNARE || k == HZ_DCOLD || k == HZ_TITHE || k == HZ_MIRROR || k == HZ_RELIQ;
}
/* its painted cells in the level's map, in the order painted, those that are open floor (not a door, not a secret's) */
static int dh_cells(int i, int *cx, int *cy, int most) {
    const Drawn *D = drawn();
    const DrawnHaz *z = &D->haz[i];
    int n = 0;
    for (int c = z->cell; c < z->cell + z->ncell && c < D->nhzcell && n < most; c++) {
        int x = D->ox + D->hzcell[c].x, y = D->oy + D->hzcell[c].y, dup = 0;
        if (!hz_floor(x, y)) continue;
        for (int k = 0; k < n; k++) dup |= cx[k] == x && cy[k] == y;
        if (!dup) { cx[n] = x; cy[n] = y; n++; }
    }
    return n;
}
static int dh_ax(int i) { return drawn()->ox + drawn()->haz[i].ax; }
static int dh_ay(int i) { return drawn()->oy + drawn()->haz[i].ay; }

/* ---- laid with the level: the fixed ones' sites, the anchor's wall dressed as the game dresses it ---- */
static int dh_site(int i) {
    const DrawnHaz *z = &drawn()->haz[i];
    int ax = dh_ax(i), ay = dh_ay(i), x = ax, y = ay, d = 0, k = z->kind;
    int cx[HZ_CELLS], cy[HZ_CELLS], n = dh_cells(i, cx, cy, HZ_CELLS);
    if (dh_wall_kind(k)) {                                         /* the floor before it: a painted cell beside it, else the side */
        if (ax < 1 || ay < 1 || ax >= MAPN - 1 || ay >= MAPN - 1 || G.map[ay][ax].kind != K_WALL || G.map[ay][ax].secret) return -1;   /* nearest the first */
        int best = -1, bd = 1 << 30;
        for (int e = 0; e < 4; e++) {
            int fx = ax - DX[e], fy = ay - DY[e];
            if (!hz_floor(fx, fy)) continue;
            int dd = n ? abs(fx - cx[0]) + abs(fy - cy[0]) : 0;
            for (int c = 0; c < n; c++) if (cx[c] == fx && cy[c] == fy) dd = -1000 + c;
            if (dd < bd) { bd = dd; best = e; }
        }
        if (best < 0) return -1;
        d = best; x = ax - DX[d]; y = ay - DY[d];
    } else if (!hz_floor(ax, ay)) return -1;
    HzSite *s = site_new(k, x, y);
    if (!s) return -1;
    s->dh = i + 1; s->idle = !z->on;
    if (dh_wall_kind(k)) {
        static const int TEX[NHZ] = { [HZ_BREATH] = T_HZ_SLAB, [HZ_COLD] = T_HZ_COLD, [HZ_SAINT] = T_HZ_SAINT, [HZ_LODE] = T_HZ_SEAM,
                                      [HZ_MIRROR] = T_HZ_MIRROR, [HZ_SLUICE] = T_HZ_SLUICE, [HZ_FISSURE] = T_HZ_FISSURE0 };
        s->d = d; s->wx = ax; s->wy = ay;
        s->ax = x + 0.5f + DX[d] * 0.48f; s->ay = y + 0.5f + DY[d] * 0.48f;
        G.map[ay][ax].tex = (uint8_t)TEX[k];
    }
    int line = k == HZ_SLUICE || k == HZ_FISSURE || k == HZ_TIMBER || k == HZ_CART;
    int cells = line || k == HZ_BREATH || k == HZ_COLD || k == HZ_SAINT || k == HZ_DAMP || k == HZ_DCOLD;
    if (cells) {                                                   /* a line's cells in their order; a patch's by its step from the anchor */
        s->n = n;
        for (int c = 0; c < n; c++) { s->cx[c] = (int8_t)cx[c]; s->cy[c] = (int8_t)cy[c]; s->cd[c] = (uint8_t)(line ? c : imin(255, abs(cx[c] - x) + abs(cy[c] - y))); }
    }
    switch (k) {
    case HZ_CENSER: {                                              /* it swings along the way its arc was painted, as far as it was */
        float far = 0.0f;
        int ex = 0, ey = 0;
        for (int c = 0; c < n; c++) { ex = imax(ex, abs(cx[c] - x)); ey = imax(ey, abs(cy[c] - y)); }
        s->d = ey > ex ? 1 : 0;
        for (int c = 0; c < n; c++) far = fmaxf(far, (float)(s->d ? abs(cy[c] - y) : abs(cx[c] - x)));
        if (n) s->amp = clampf(far, 0.5f, 4.0f);
        if ((z->how & HH_CLOCK) && z->every > 0.0f) s->per = fmaxf(1.0f, z->every);
        s->t = 0.0f;                                               /* from the middle */
        break;
    }
    case HZ_CART:                                                  /* down its rails, as painted; the Hollows' rails under them */
        if (n < 2) { G.nsite--; return -1; }
        s->x = cx[0]; s->y = cy[0];
        s->d = abs(cy[n - 1] - cy[0]) > abs(cx[n - 1] - cx[0]) ? 1 : 0;
        if (G.look == 2)
            for (int c = 0; c < n; c++) {
                Cell *f = &G.map[cy[c]][cx[c]];
                int along = c + 1 < n ? cy[c + 1] == cy[c] : cy[c - 1] == cy[c];
                f->flo = (uint8_t)(T_FLOORS + FL3_RAILS); f->fflip = (uint8_t)((f->fflip & 15) | (16 | (along ? 4 : 0) << 5));
            }
        break;
    case HZ_LODE: case HZ_CRYSTAL: {                               /* it reaches as far as its cells */
        float ox = k == HZ_LODE ? s->ax : x + 0.5f, oy = k == HZ_LODE ? s->ay : y + 0.5f, far = 0.0f;
        for (int c = 0; c < n; c++) far = fmaxf(far, hypotf(cx[c] + 0.5f - ox, cy[c] + 0.5f - oy));
        if (n) s->amp = clampf(far + 0.25f, 1.0f, 4.0f);
        if (k == HZ_CRYSTAL) G.map[y][x].kind = K_PROP;
        break;
    }
    case HZ_RELIQ: G.map[y][x].kind = K_PROP; break;               /* the plinth */
    case HZ_SNARE: s->state = s->idle; break;                      /* no web till it is live */
    default: break;
    }
    if (!dh_by_itself(k) && (z->how & (HH_CLOCK | HH_ENTER))) s->wait = 1e9f;   /* its turns are its clock's and its box's, not its own */
    return (int)(s - G.site);
}
static void dh_lay(void) {                                         /* start_level: where a drawn level's sites would be */
    const Drawn *D = drawn();
    memset(&DH, 0, sizeof DH);
    DH.n = imin(D->nhaz, MAXSITE);
    int cells = 0;
    for (int i = 0; i < DH.n; i++) {                               /* the dressing off what each covers, and off where it stands */
        const DrawnHaz *z = &D->haz[i];
        for (int c = z->cell; c < z->cell + z->ncell && c < D->nhzcell; c++) drawn_clear(D->ox + D->hzcell[c].x, D->oy + D->hzcell[c].y);
        if (!dh_wall_kind(z->kind) && z->kind != HZ_BILE) drawn_clear(dh_ax(i), dh_ay(i));
    }
    for (int i = 0; i < DH.n; i++) {
        const DrawnHaz *z = &D->haz[i];
        DhOne *o = &DH.h[i];
        o->live = z->on != 0; o->next = z->first; o->last = -1e9f; o->site = -1;
        cells += z->ncell;
        if (dh_site_kind(z->kind)) o->site = dh_site(i);
#ifdef CRAWL_DEBUG
        if (dh_site_kind(z->kind) && o->site < 0) fprintf(stderr, "DRAWN level %d: hazard %d (%s) has nowhere to stand at (%d,%d)\n", G.level, i, HZ_TAG[z->kind], z->ax, z->ay);
#endif
    }
#ifdef CRAWL_DEBUG
    if (DH.n) fprintf(stderr, "DRAWN level %d: %d hazards, %d cells\n", G.level, DH.n, cells);
#else
    (void)cells;
#endif
}

/* ---- set off: a fixed one's turn, or one that arises laid as the game lays it, on all its cells or one place in them ---- */
static int dh_pick(const int *cx, const int *cy, int n) {          /* a zone's place: near you and in sight if it can be, the run's dice */
    int near[DH_CELLS], m = 0;
    for (int c = 0; c < n; c++)
        if (hypotf(cx[c] + 0.5f - G.px, cy[c] + 0.5f - G.py) < 6.0f && line_clear(G.px, G.py, cx[c] + 0.5f, cy[c] + 0.5f)) near[m++] = c;
    return m ? near[rnd(m)] : rnd(n);
}
static int dh_patch(const int *cx, const int *cy, int n, int from, int reach, int most, int8_t *ox, int8_t *oy, uint8_t *od) {
    static uint8_t in[MAPN][MAPN];                                 /* the cells within reach of one, through the painted ones, nearest first */
    static int qx[DH_CELLS], qy[DH_CELLS], qd[DH_CELLS];
    memset(in, 0, sizeof in);
    for (int c = 0; c < n; c++) in[cy[c]][cx[c]] = 1;
    int qh = 0, qt = 0, m = 0;
    qx[qt] = cx[from]; qy[qt] = cy[from]; qd[qt++] = 0; in[cy[from]][cx[from]] = 2;
    while (qh < qt && m < most) {
        int x = qx[qh], y = qy[qh], d = qd[qh++];
        ox[m] = (int8_t)x; oy[m] = (int8_t)y; od[m] = (uint8_t)d; m++;
        if (d >= reach) continue;
        for (int e = 0; e < 4 && qt < DH_CELLS; e++) {
            int nx = x + DX[e], ny = y + DY[e];
            if (nx < 0 || ny < 0 || nx >= MAPN || ny >= MAPN || in[ny][nx] != 1) continue;
            in[ny][nx] = 2; qx[qt] = nx; qy[qt] = ny; qd[qt++] = d + 1;
        }
    }
    return m;
}
static int dh_run(const int *cx, const int *cy, int n, int from, int len, int8_t *ox, int8_t *oy, uint8_t *od) {
    int bd = 0, bl = 0, d0 = rnd(4);                               /* a line within the cells from one: the way the most of them run */
    for (int k = 0; k < 4; k++) {
        int d = (d0 + k) & 3, l = 1;
        for (; l < len; l++) {
            int x = cx[from] + DX[d] * l, y = cy[from] + DY[d] * l, here = 0;
            for (int c = 0; c < n && !here; c++) here = cx[c] == x && cy[c] == y;
            if (!here) break;
        }
        if (l > bl) { bl = l; bd = d; }
    }
    for (int l = 0; l < bl; l++) { ox[l] = (int8_t)(cx[from] + DX[bd] * l); oy[l] = (int8_t)(cy[from] + DY[bd] * l); od[l] = (uint8_t)l; }
    return bl;
}
static int dh_arise(int i, int *rx, int *ry) {
    const DrawnHaz *z = &drawn()->haz[i];
    static int cx[DH_CELLS], cy[DH_CELLS];
    int n = dh_cells(i, cx, cy, DH_CELLS), k = z->kind, ax = dh_ax(i), ay = dh_ay(i), dh = i + 1, laid = 0;
    if (!n) return 0;
    int at = z->zone ? dh_pick(cx, cy, n) : 0;
    int8_t hx[HZ_CELLS], hy[HZ_CELLS]; uint8_t hd[HZ_CELLS];
    *rx = z->zone ? cx[at] : ax; *ry = z->zone ? cy[at] : ay;
    switch (k) {
    case HZ_ROCK: case HZ_VOID: case HZ_GLASS:                     /* a disc on each cell, or on one: placed, each kept to its own cell */
        for (int c = z->zone ? at : 0; c < (z->zone ? at + 1 : n); c++) {
            float x = cx[c] + 0.5f, y = cy[c] + 0.5f;
            int h0 = -1;
            for (int j = 0; j < MAXHAZ && h0 < 0; j++) if (!G.haz[j].alive) h0 = j;
            if (h0 < 0) break;
            if (k == HZ_ROCK) hz_rock(x, y, 0);
            else if (k == HZ_VOID) hz_void(x, y, z->zone ? 1.2f : 0.5f, 0);
            else if (hz_disc(HZ_GLASS, x, y, z->zone ? 0.9f : 0.5f, 1.0f, 1.0f, 0.8f, 0.15f)) sfx_at(SFX_SHATTER, x, y, z->zone || !c ? 0.5f : 0.15f);
            if (G.haz[h0].alive) { hz_dapply(&G.haz[h0], dh); laid++; }
        }
        break;
    case HZ_ASH: {                                                 /* placed: one fall over all its cells; a zone: the game's own, on one */
        float x = cx[at] + 0.5f, y = cy[at] + 0.5f, r = 2.0f;
        if (!z->zone) {
            float sx = 0.0f, sy = 0.0f, far = 0.0f;
            for (int c = 0; c < n; c++) { sx += cx[c] + 0.5f; sy += cy[c] + 0.5f; }
            x = sx / (float)n; y = sy / (float)n;
            for (int c = 0; c < n; c++) far = fmaxf(far, hypotf(cx[c] + 0.5f - x, cy[c] + 0.5f - y));
            r = fmaxf(0.8f, far + 0.3f);
        }
        Hazard *h = hz_disc(HZ_ASH, x, y, r, 1.5f, 6.0f, 2.0f, 0.03f);
        if (h) { hz_dapply(h, dh); laid++; }
        break;
    }
    case HZ_FIRE: case HZ_VOTIVE: {                                /* a line: as painted, or a run within the cells */
        int m = 0;
        if (k == HZ_VOTIVE && !z->zone && !(cx[0] == ax && cy[0] == ay) && hz_floor(ax, ay)) { hx[m] = (int8_t)ax; hy[m] = (int8_t)ay; hd[m] = 0; m++; }   /* the rack first */
        if (z->zone) m = dh_run(cx, cy, n, at, k == HZ_FIRE ? 4 + rnd(3) : 3 + rnd(2), hx, hy, hd);
        else for (int c = 0; c < n && m < HZ_CELLS; c++) { hx[m] = (int8_t)cx[c]; hy[m] = (int8_t)cy[c]; hd[m] = (uint8_t)m; m++; }
        Hazard *h = k == HZ_FIRE ? hz_fire(0) : hz_new(HZ_VOTIVE, 0);
        if (!h) break;
        h->n = m;
        for (int c = 0; c < m; c++) { h->cx[c] = hx[c]; h->cy[c] = hy[c]; h->cd[c] = hd[c]; }
        float sx = z->zone ? hx[0] + 0.5f : ax + 0.5f, sy = z->zone ? hy[0] + 0.5f : ay + 0.5f;
        if (k == HZ_FIRE) hz_fire_done(h, sx, sy);
        else {
            h->x = hx[0] + 0.5f; h->y = hy[0] + 0.5f; h->r = 4.0f; h->warn = hz_warn(1.0f); h->on = 3.0f; h->off = 1.5f; h->share = 0.07f;
            sfx_at(SFX_SHATTER, h->x, h->y, 0.6f); sfx_at(SFX_IGNITE, h->x, h->y, 0.6f);
        }
        if (h->alive) { hz_dapply(h, dh); laid++; }
        break;
    }
    case HZ_BILE: case HZ_GALV: case HZ_SILT: {                    /* a patch: all its cells, or the game's patch within them */
        int m, from = at;
        if (!z->zone && k == HZ_BILE)                              /* from the grate: the painted cell beside it, else the first */
            for (int c = 0; c < n; c++) if (abs(cx[c] - ax) + abs(cy[c] - ay) == 1) { from = c; break; }
        int reach = z->zone ? (k == HZ_BILE ? 2 : k == HZ_GALV ? 4 : 1 + rnd(2)) : 1 << 20, most = z->zone ? (k == HZ_BILE ? 12 : k == HZ_SILT ? 9 : HZ_CELLS) : HZ_CELLS;
        m = dh_patch(cx, cy, n, from, reach, most, hx, hy, hd);
        if (!z->zone && m < imin(n, HZ_CELLS))                     /* what the painting does not join, after it, in its order */
            for (int c = 0; c < n && m < HZ_CELLS; c++) {
                int dup = 0;
                for (int j = 0; j < m && !dup; j++) dup = hx[j] == cx[c] && hy[j] == cy[c];
                if (!dup) { hx[m] = (int8_t)cx[c]; hy[m] = (int8_t)cy[c]; hd[m] = (uint8_t)imin(255, hd[m ? m - 1 : 0] + 1); m++; }
            }
        Hazard *h = NULL;
        if (k == HZ_BILE) {
            h = hz_bile(hx, hy, hd, m, 0);
            if (h) {
                h->on = 8.0f; h->share = 0.7f;
                Cell *w = ax > 0 && ay > 0 && ax < MAPN - 1 && ay < MAPN - 1 ? &G.map[ay][ax] : NULL;
                if (w && w->kind == K_WALL && !w->secret && w->tex != T_GRATE && !(g_events && EV.at_torch[ay][ax])) { h->wx = ax; h->wy = ay; h->wtex = w->tex; w->tex = T_GRATE; }
            }
        } else {
            h = k == HZ_GALV ? hz_cells(HZ_GALV, hx, hy, hd, m, 1.5f, 0.15f, 0.6f, 0.14f) : hz_cells(HZ_SILT, hx, hy, hd, m, 1.4f, 5.0f, 1.5f, 0.4f);
            if (h && !z->zone) { h->x = ax + 0.5f; h->y = ay + 0.5f; }   /* where it strikes; its heart */
            if (h) sfx_at(k == HZ_GALV ? SFX_CRACKLE : SFX_HISS, h->x, h->y, 0.7f);
        }
        if (h) { hz_dapply(h, dh); laid++; }
        break;
    }
    default: break;
    }
    return laid;
}
static int dh_busy(int i) {                                        /* a fixed one's turn still running */
    int si = DH.h[i].site;
    if (si < 0 || si >= G.nsite) return 0;
    const HzSite *s = &G.site[si];
    return s->haz >= 0 && G.haz[s->haz].alive && G.haz[s->haz].site == si;
}
/* 1 gone off, 0 nothing to set off, -1 not yet (a fixed one's turn still running) */
static int dh_go(int i, int why) {
    const DrawnHaz *z = &drawn()->haz[i];
    DhOne *o = &DH.h[i];
    int rx = z->ax, ry = z->ay;
    if (G.dead || G.exiting || G.tally) return -1;
    if (dh_site_kind(z->kind)) {
        if (o->site < 0 || o->site >= G.nsite) return 0;
        HzSite *s = &G.site[o->site];
        if (z->kind == HZ_SNARE) { if (!s->state) return 0; s->state = 0; }   /* the web set again */
        else if (dh_by_itself(z->kind)) return 0;                  /* it runs while live: nothing to set off */
        else {
            if (dh_busy(i)) return -1;
            if (!site_fire(s, o->site)) return -1;
            if (!(z->how & (HH_CLOCK | HH_ENTER))) s->wait = site_rest(s->kind);   /* and then at its own pace */
        }
    } else {
        int x, y;
        if (!dh_arise(i, &x, &y)) return 0;
        rx = x - drawn()->ox; ry = y - drawn()->oy;
    }
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT dhaz %d %s %s at (%d,%d) t=%.2f\n", i, HZ_TAG[z->kind], DW_NAME[why], rx, ry, G.t);
#else
    (void)why; (void)rx; (void)ry;
#endif
    return 1;
}
static void dh_wake(int i, int on) {                               /* TA_HAZ_ON, TA_HAZ_OFF */
    if (!g_drawn || i < 0 || i >= DH.n) return;
    const DrawnHaz *z = &drawn()->haz[i];
    DhOne *o = &DH.h[i];
    HzSite *s = o->site >= 0 && o->site < G.nsite ? &G.site[o->site] : NULL;
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT dhaz_%s %d t=%.2f\n", on ? "on" : "off", i, G.t);
#endif
    if (on) {
        int was = o->live;
        o->live = 1;
        if (s) { if (s->idle && z->kind == HZ_SNARE) s->state = 0; s->idle = 0; }
        if (!(z->how & (HH_CLOCK | HH_ENTER))) dh_go(i, DW_TRIGGER);   /* neither clock nor box: it goes off now */
        else if (!was && (z->how & HH_CLOCK)) o->next = DH.now + z->first;
        return;
    }
    o->live = 0;
    for (int j = 0; j < MAXHAZ; j++) {                             /* it stops, its warning and all */
        Hazard *h = &G.haz[j];
        if (!h->alive || h->dh != i + 1) continue;
        if (h->wx >= 0) G.map[h->wy][h->wx].tex = (uint8_t)h->wtex;
        if (h->kind == HZ_TIMBER && h->hit == 2) hz_timber_close(h, 0);
        h->alive = 0;
    }
    if (s) {
        s->idle = 1; s->haz = -1; s->drink = 0;
        if (z->kind == HZ_SNARE) s->state = 1;
        if (z->kind == HZ_RELIQ) { s->state = 0; s->k = 0.0f; }
    }
}
/* every frame, whoever plays (update_hazards): the clocks, and the boxes walked into */
static void dh_tick(float dt) {
    if (!dh_on_level()) return;
    const Drawn *D = drawn();
    DH.now += dt;
    int cx = (int)floorf(G.px) - D->ox, cy = (int)floorf(G.py) - D->oy;
    for (int i = 0; i < DH.n; i++) {
        const DrawnHaz *z = &D->haz[i];
        DhOne *o = &DH.h[i];
        int in = !G.dead && (z->how & HH_ENTER) && z->x1 >= z->x0 && z->y1 >= z->y0 && cx >= z->x0 && cx <= z->x1 && cy >= z->y0 && cy <= z->y1;
        if (o->live && in && !o->inside && (z->every <= 0.0f || DH.now - o->last >= z->every) && dh_go(i, DW_ENTER) > 0) o->last = DH.now;
        o->inside = in;
        if (o->live && (z->how & HH_CLOCK) && DH.now >= o->next && dh_go(i, DW_CLOCK) >= 0)
            o->next = DH.now + fmaxf(0.5f, z->every + (z->give > 0.0f ? z->give * (2.0f * rndf() - 1.0f) : 0.0f));
#ifdef CRAWL_DEBUG
        static int drank[MAXSITE];                                 /* a sapper's drinking begun, for the logs */
        int dr = o->site >= 0 && o->site < G.nsite && G.site[o->site].drink;
        if (dr && !drank[i]) fprintf(stderr, "EVENT dhaz_drink %d %s t=%.2f\n", i, HZ_TAG[z->kind], G.t);
        drank[i] = dr;
#endif
    }
}
static int dh_spare(int kind) {                                    /* hurt_ex: every drawn hazard of the kind may not kill */
    if (!g_drawn || !DH.n) return 0;
    int any = 0;
    for (int i = 0; i < DH.n; i++) {
        if (drawn()->haz[i].kind != kind) continue;
        if (drawn()->haz[i].kills != 0) return 0;
        any = 1;
    }
    return any;
}
