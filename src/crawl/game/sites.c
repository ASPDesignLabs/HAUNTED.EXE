/* A part of scene_crawl.c (docs/PLAN-tidy.md): it is #included there in its place and never compiled on its own.
 * The fixed hazards: where they stand (place_sites), update_sites, the arising ones, their lights. */

/* ---- the fixed ones: where they stand ---- */
static int site_near(int x, int y, int d) {
    for (int i = 0; i < G.nsite; i++) if (abs(G.site[i].x - x) + abs(G.site[i].y - y) < d) return 1;
    return 0;
}
static int site_ok(int x, int y) {                                 /* a floor cell a fixed hazard may hold */
    if (!hz_floor(x, y) || in_band(y) || vault_of_room(x, y) >= 0 || in_wing(x, y) || in_trial(x, y) || G.bulky[y][x] || item_at(x, y)) return 0;
    if (G.pz_door >= 0 && abs(x - G.doors[G.pz_door].x) + abs(y - G.doors[G.pz_door].y) < 5) return 0;   /* nor at a puzzle's door */
    int near = 8;
#ifdef CRAWL_DEBUG
    if (getenv("CRAWL_HZ")) near = 3;                              /* testing: close by */
#endif
    if (abs(x - G.cx) + abs(y - G.cy) < near) return 0;
    if (G.exit_fx >= 0 && abs(x - G.exit_fx) + abs(y - G.exit_fy) < 3) return 0;
    if (G.altar_x >= 0 && abs(x - G.altar_x) + abs(y - G.altar_y) < 3) return 0;
    for (int d = 0; d < 4; d++) if (cell(x + DX[d], y + DY[d])->kind == K_DOOR) return 0;
    return !site_near(x, y, 6);
}
/* a wall beside (x, y), plain and seen only from this side, for something set in it */
static int hz_wall(int x, int y, int *dir) {
    int k0 = rnd(4);
    for (int k = 0; k < 4; k++) {
        int d = (k0 + k) & 3, wx = x + DX[d], wy = y + DY[d];
        if (wx <= 0 || wy <= 0 || wx >= MAPN - 1 || wy >= MAPN - 1) continue;
        const Cell *w = &G.map[wy][wx];
        if (w->kind != K_WALL || w->secret || !plain_tex(w->tex) || w->tex == T_TORCH0 || w->ftorch) continue;
        int open = 0;
        for (int e = 0; e < 4; e++) open += is_open(wx + DX[e], wy + DY[e]) || cell(wx + DX[e], wy + DY[e])->kind == K_PILLAR || cell(wx + DX[e], wy + DY[e])->kind == K_PROP;
        if (open != 1) continue;
        *dir = d;
        return 1;
    }
    return 0;
}
static HzSite *site_new(int kind, int x, int y) {
    if (G.nsite >= MAXSITE) return NULL;
    HzSite *s = &G.site[G.nsite++];
    memset(s, 0, sizeof *s);
    s->kind = kind; s->x = x; s->y = y; s->haz = -1; s->wx = -1;
    s->wait = 3.0f + 6.0f * rndf(); s->t = rndf() * 10.0f;
    return s;
}
static HzSite *site_wall(int kind, int x, int y, int d, int tex) { /* set in the wall to the d of (x, y) */
    HzSite *s = site_new(kind, x, y);
    if (!s) return NULL;
    s->d = d; s->wx = x + DX[d]; s->wy = y + DY[d];
    s->ax = x + 0.5f + DX[d] * 0.48f; s->ay = y + 0.5f + DY[d] * 0.48f;
    G.map[s->wy][s->wx].tex = (uint8_t)tex;
    return s;
}
static int site_line(HzSite *s, int x, int y, int d, int most) {   /* cells from (x, y) the d way, in order */
    s->n = 0;
    for (int k = 0; k < most && s->n < HZ_CELLS; k++) {
        int cx = x + DX[d] * k, cy = y + DY[d] * k;
        if (!hz_floor(cx, cy) || in_band(cy)) break;
        s->cx[s->n] = (int8_t)cx; s->cy[s->n] = (int8_t)cy; s->cd[s->n] = (uint8_t)k; s->n++;
    }
    return s->n;
}
static int room_kind_at(int x, int y) {                            /* the room type a cell lies in, -1 a hall */
    for (int r = 0; r < G.nrooms; r++) if (in_room(&G.rooms[r], x, y)) return G.rooms[r].rtype;
    return -1;
}
static int place_site(int kind) {
    for (int tries = 0; tries < 500; tries++) {
        int x = 1 + rnd(MAPN - 2), y = 1 + rnd(MAPN - 2), d, rt;
        if (!site_ok(x, y)) continue;
        rt = room_kind_at(x, y);
        HzSite *s;
        switch (kind) {
        case HZ_CENSER: {                                          /* across a wide hall or a room, over its lane: there is a way past it */
            if (!lane(x, y)) continue;
            int ax = -1;
            for (int a = 0; a < 2 && ax < 0; a++) {                /* a: the way it swings; the lane runs the other way */
                int ok = 1, b = a ^ 1;
                for (int k = -1; k <= 1 && ok; k++) ok = hz_floor(x + DX[a] * k, y + DY[a] * k) && !G.bulky[y + DY[a] * k][x + DX[a] * k];
                for (int k = -2; k <= 2 && ok; k++) ok = hz_floor(x + DX[b] * k, y + DY[b] * k);
                if (ok && (x & 3) == 2 && (y & 3) == 2) ok = 0;    /* not over a crossing */
                if (ok) ax = a;
            }
            if (ax < 0 || !(s = site_new(kind, x, y))) continue;
            s->d = ax;
            return 1;
        }
        case HZ_BREATH: case HZ_COLD: case HZ_SAINT: case HZ_LODE: case HZ_MIRROR: case HZ_SLUICE: case HZ_FISSURE: {
            if (!hz_wall(x, y, &d)) continue;
            static const int TEX[NHZ] = { [HZ_BREATH] = T_HZ_SLAB, [HZ_COLD] = T_HZ_COLD, [HZ_SAINT] = T_HZ_SAINT, [HZ_LODE] = T_HZ_SEAM,
                                          [HZ_MIRROR] = T_HZ_MIRROR, [HZ_SLUICE] = T_HZ_SLUICE, [HZ_FISSURE] = T_HZ_FISSURE0 };
            if (kind == HZ_SLUICE || kind == HZ_FISSURE) {         /* a run of floor away from the wall */
                HzSite tmp;
                if (site_line(&tmp, x, y, (d + 2) & 3, kind == HZ_SLUICE ? 7 : 5) < 4) continue;
                if (!(s = site_wall(kind, x, y, d, TEX[kind]))) continue;
                site_line(s, x, y, (d + 2) & 3, kind == HZ_SLUICE ? 7 : 5);
                return 1;
            }
            if (!(s = site_wall(kind, x, y, d, TEX[kind]))) continue;
            if (kind != HZ_LODE && kind != HZ_MIRROR) s->n = hz_patch(x, y, kind == HZ_BREATH ? 1 : 2, s->cx, s->cy, s->cd, kind == HZ_BREATH ? 5 : (kind == HZ_SAINT ? 10 : 9));
            return 1;
        }
        case HZ_SNARE: {                                           /* in a doorway, or where a room opens on the hall */
            int door = 0;
            for (int e = 0; e < 4; e++) door |= cell(x + DX[e], y + DY[e])->kind == K_DOOR;
            if (!lane(x, y) || (!door && !by_way(x, y))) continue;
            if (abs(x - G.cx) + abs(y - G.cy) < 8 || site_near(x, y, 6) || (G.exit_fx >= 0 && abs(x - G.exit_fx) + abs(y - G.exit_fy) < 3)) continue;
            if (!(s = site_new(kind, x, y))) continue;
            return 1;
        }
        case HZ_TIMBER: {                                          /* across a room: three or four cells in a row */
            if (rt < 0 || block_corner(x, y)) continue;
            d = rnd(2);
            if (!(s = site_new(kind, x, y))) continue;
            if (site_line(s, x, y, d, 3 + rnd(2)) < 3) { G.nsite--; continue; }
            int whole = 1;
            for (int i = 0; i < s->n && whole; i++) whole = room_kind_at(s->cx[i], s->cy[i]) == rt && !item_at(s->cx[i], s->cy[i]) && !G.bulky[s->cy[i]][s->cx[i]];
            if (!whole) { G.nsite--; continue; }
            return 1;
        }
        case HZ_DAMP: case HZ_DCOLD: {                             /* a patch of floor, in a room if it can be (a stope, for the gas) */
            if (kind == HZ_DAMP && rt < 0 && tries < 300) continue;
            if (kind == HZ_DAMP && rt != RT_STOPE && tries < 120 && G.look == 2) continue;
            if (!(s = site_new(kind, x, y))) continue;
            s->n = hz_patch(x, y, 2, s->cx, s->cy, s->cd, kind == HZ_DCOLD ? 14 : 12);
            return 1;
        }
        case HZ_CART: {                                            /* down a straight run of rails */
            if (cell(x, y)->flo != T_FLOORS + FL3_RAILS) continue;
            for (int a = 0; a < 2; a++) {
                int x0 = x, y0 = y;
                while (cell(x0 - DX[a], y0 - DY[a])->flo == T_FLOORS + FL3_RAILS && hz_floor(x0 - DX[a], y0 - DY[a])) { x0 -= DX[a]; y0 -= DY[a]; }
                HzSite tmp;
                int n = 0;
                for (int k = 0; k < 14; k++) { int cx = x0 + DX[a] * k, cy = y0 + DY[a] * k; if (cell(cx, cy)->flo != T_FLOORS + FL3_RAILS || !hz_floor(cx, cy)) break; n = k + 1; }
                if (n < 5) continue;
                if (!(s = site_new(kind, x0, y0))) continue;
                s->d = a;
                site_line(s, x0, y0, a, n);
                (void)tmp;
                return 1;
            }
            continue;
        }
        case HZ_CRYSTAL: case HZ_RELIQ: {                          /* a thing of its own, at a block's corner in a room */
            if (rt < 0 || !block_corner(x, y) || lane(x, y) || by_way(x, y)) continue;
            if (!(s = site_new(kind, x, y))) continue;
            G.map[y][x].kind = K_PROP;
            return 1;
        }
        case HZ_TITHE: {                                           /* a ring of chalk on the floor of a room, all of it floor */
            if (rt < 0) continue;
            int ok = 1;
            for (int dy = -1; dy <= 1 && ok; dy++) for (int dx = -1; dx <= 1 && ok; dx++) ok = hz_floor(x + dx, y + dy) && !G.bulky[y + dy][x + dx];
            if (!ok || !(s = site_new(kind, x, y))) continue;
            return 1;
        }
        default: return 0;
        }
    }
    return 0;
}
/* the level's fixed hazards: their number by the run's step, their kinds by the run (and an earlier one's now and then) */
static void place_sites(void) {
    G.nsite = 0; G.snare_t = 0.0f;
    G.arise_t = 25.0f + 20.0f * rndf();
    if (PK.depth || G.shop) return;
    int step = crawl_step(G.level), n = imin(5, 1 + step / 2), forced = -1;
#ifdef CRAWL_DEBUG
    if (getenv("CRAWL_HZ")) forced = atoi(getenv("CRAWL_HZ"));    /* testing: this one, three times over */
#endif
    if (forced >= 0) {
        int fixed = 0;
        for (int s = 0; s < 4; s++) { for (int k = 0; k < 4; k++) fixed |= HZ_FIXED[s][k] == forced; fixed |= HZ_SAPPER[s] == forced; }
        if (fixed) for (int i = 0; i < 3; i++) place_site(forced);
        n = 0;
    }
    for (int i = 0; i < n; i++) {
        int set = hz_from_set(), kinds[4], nk = 0;
        for (int k = 0; k < 4; k++) {
            int kd = HZ_FIXED[set][k];
            if (kd < 0) continue;
            if (kd == HZ_SNARE && step < 5) continue;              /* the web from the run's sixth level */
            if (kd == HZ_DCOLD && step < 6) continue;              /* the dark cold on its last */
            kinds[nk++] = kd;
        }
        if (!nk) continue;
        int kd = kinds[rnd(nk)];
        if (!place_site(kd) && kd == HZ_CART) place_site(HZ_TIMBER);   /* no rails: the props instead */
    }
    if (step >= 5 && forced < 0)                                   /* the soul-sappers, from the sixth level */
        for (int i = 0, ns = 2 + rnd(2); i < ns; i++) place_site(HZ_SAPPER[hz_from_set()]);
#ifdef CRAWL_DEBUG
    fprintf(stderr, "SITES level %d:", G.level);
    for (int i = 0; i < G.nsite; i++) fprintf(stderr, " %d@(%d,%d)", G.site[i].kind, G.site[i].x, G.site[i].y);
    fprintf(stderr, "\n");
#endif
}

/* a fixed one's turn: warning, harm and fading, as a hazard of its own */
static Hazard *site_fire(HzSite *s, int i) {
    Hazard *h = NULL;
    hz_quiet = 1;
    switch (s->kind) {
    case HZ_BREATH: h = hz_cells(HZ_BREATH, s->cx, s->cy, s->cd, s->n, 1.2f, 4.0f, 1.5f, 0.5f); if (h) sfx_at(SFX_HISS, s->ax, s->ay, 0.9f); break;
    case HZ_COLD: h = hz_cells(HZ_COLD, s->cx, s->cy, s->cd, s->n, 1.4f, 4.0f, 2.0f, 0.02f); if (h) sfx_at(SFX_FROST, s->ax, s->ay, 0.9f); break;
    case HZ_SLUICE:
        h = hz_cells(HZ_SLUICE, s->cx, s->cy, s->cd, s->n, 1.3f, (float)s->n / 6.0f + 0.3f, 1.5f, 0.08f);
        if (h) { h->ang = atan2f((float)-DY[s->d], (float)-DX[s->d]); sfx_at(SFX_UNLOCK, s->ax, s->ay, 0.9f); }
        break;
    case HZ_FISSURE: h = hz_cells(HZ_FISSURE, s->cx, s->cy, s->cd, s->n, 1.3f, 1.2f, 1.5f, 0.1f); if (h) sfx_at(SFX_GROAN, s->ax, s->ay, 0.4f); break;
    case HZ_TIMBER: h = hz_cells(HZ_TIMBER, s->cx, s->cy, s->cd, s->n, 2.0f, 0.05f, 20.0f, 0.18f); if (h) sfx_at(SFX_CREAK, s->x + 0.5f, s->y + 0.5f, 1.2f); break;
    case HZ_DAMP: h = hz_cells(HZ_DAMP, s->cx, s->cy, s->cd, s->n, 4.0f, 1e6f, 1.0f, 0.35f); if (h) sfx_at(SFX_HISS, s->x + 0.5f, s->y + 0.5f, 0.4f); break;
    case HZ_SAINT: h = hz_cells(HZ_SAINT, s->cx, s->cy, s->cd, s->n, 3.0f, 10.0f, 3.0f, 1.0f); if (h) sfx_at(SFX_MOAN, s->ax, s->ay, 0.5f); break;
    case HZ_CART: {                                                /* from whichever end it rests at, to the other */
        int8_t cx[HZ_CELLS], cy[HZ_CELLS]; uint8_t cd[HZ_CELLS];
        for (int k = 0; k < s->n; k++) { int j = s->state ? s->n - 1 - k : k; cx[k] = s->cx[j]; cy[k] = s->cy[j]; cd[k] = (uint8_t)k; }
        h = hz_cells(HZ_CART, cx, cy, cd, s->n, 1.5f, (float)(s->n - 1) / 6.0f, 0.1f, 0.16f);
        if (h) { h->ang = s->state ? atan2f((float)-DY[s->d], (float)-DX[s->d]) : atan2f((float)DY[s->d], (float)DX[s->d]); s->state ^= 1; sfx_at(SFX_RUMBLE, h->x, h->y, 1.0f); }
        break;
    }
    case HZ_LODE: h = hz_disc(HZ_LODE, s->ax, s->ay, 1.5f, 1.2f, 0.15f, 0.5f, 0.14f); if (h) sfx_at(SFX_CRACKLE, s->ax, s->ay, 0.8f); break;
    case HZ_CRYSTAL: h = hz_disc(HZ_CRYSTAL, s->x + 0.5f, s->y + 0.5f, 3.0f, 1.6f, 0.15f, 0.5f, 0.14f); if (h) sfx_at(SFX_HUM, s->x + 0.5f, s->y + 0.5f, 0.9f); break;
    default: break;
    }
    hz_quiet = 0;
    if (h) { h->site = i; s->haz = (int)(h - G.haz); }
    return h;
}
static float site_rest(int kind) {                                 /* the wait between its turns */
    switch (kind) {
    case HZ_BREATH: return 9.0f + 4.0f * rndf();
    case HZ_COLD: return 8.0f + 4.0f * rndf();
    case HZ_SLUICE: return 12.0f + 4.0f * rndf();
    case HZ_FISSURE: return 5.0f + 3.0f * rndf();
    case HZ_TIMBER: return 25.0f + 10.0f * rndf();
    case HZ_DAMP: return 20.0f + 10.0f * rndf();
    case HZ_SAINT: return 8.0f + 4.0f * rndf();
    case HZ_CART: return 14.0f + 6.0f * rndf();
    case HZ_LODE: return 7.0f + 3.0f * rndf();
    default: return 8.0f + 3.0f * rndf();                          /* the crystal */
    }
}
static void censer_bob(const HzSite *s, float t, float *bx, float *by, float *v) {
    float per = 5.0f - 0.08f * (float)crawl_step(G.level), ph = t * TAU_F / per;
    *bx = s->x + 0.5f + DX[s->d] * 1.4f * sinf(ph); *by = s->y + 0.5f + DY[s->d] * 1.4f * sinf(ph);
    if (v) *v = cosf(ph);
}
static int mirror_sees(const HzSite *s) {                          /* you, within three cells before its face */
    float vx = G.px - s->ax, vy = G.py - s->ay;
    return hypotf(vx, vy) < 3.0f && vx * DX[s->d] + vy * DY[s->d] < -0.2f && line_clear(s->ax - DX[s->d] * 0.1f, s->ay - DY[s->d] * 0.1f, G.px, G.py);
}
static void update_sites(float dt) {
    if (G.snare_t > 0.0f) G.snare_t = fmaxf(0.0f, G.snare_t - dt * soft_pace());
    if (PK.depth || G.shop || G.tally || !G.nsite) return;
    int px = (int)G.px, py = (int)G.py;
    for (int i = 0; i < G.nsite; i++) {
        HzSite *s = &G.site[i];
        float cx = s->x + 0.5f, cy = s->y + 0.5f, wx = s->wx >= 0 ? s->ax : cx, wy = s->wx >= 0 ? s->ay : cy;
        float d = hypotf(G.px - wx, G.py - wy);
        if (!G.dead && d < 9.0f && line_clear(G.px, G.py, wx - (s->wx >= 0 ? DX[s->d] * 0.1f : 0.0f), wy - (s->wx >= 0 ? DY[s->d] * 0.1f : 0.0f))) hz_name(s->kind);
        Hazard *h = s->haz >= 0 && G.haz[s->haz].alive && G.haz[s->haz].site == i ? &G.haz[s->haz] : NULL;
        if (!h) s->haz = -1;
        int drink = 0;
        s->t += dt; s->hit_t = fmaxf(0.0f, s->hit_t - dt);
        switch (s->kind) {
        case HZ_CENSER: {                                          /* it swings, always: its shadow and smoke ahead of it */
            float bx, by, v;
            censer_bob(s, s->t, &bx, &by, &v);
            if ((s->k > 0.0f) != (v > 0.0f)) sfx_at(SFX_CREAK, bx, by, 0.5f);   /* at each end of its swing */
            s->k = v;
            for (float u = 0.0f; u <= 0.9f; u += 0.15f) { float ux, uy; censer_bob(s, s->t + u, &ux, &uy, NULL); hz_mark((int)ux, (int)uy, u < 0.3f ? 2 : 1); }   /* the
                                                                      dream's map: not Premonition's further second, which kept the lane
                                                                      under it marked the whole swing, so the dream never crossed */
            if (!G.dead && G.level_t > 2.0 && s->hit_t <= 0.0f && hypotf(G.px - bx, G.py - by) < 0.55f) {
                s->hit_t = 1.2f;
                sfx_at(SFX_BOOM, bx, by, 0.5f);
                if (hz_strike(HZ_CENSER, 0.12f, bx, by) > 0) hz_shove(DX[s->d] * v, DY[s->d] * v, 1.0f);
            }
            break;
        }
        case HZ_SNARE:                                             /* torn by whoever walks into it first */
            if (s->state) break;
            hz_mark(s->x, s->y, HZ_SOFT);
            if (!G.dead && px == s->x && py == s->y) {
                s->state = 1;
                G.snare_t = 1.5f * (1.0f - 0.5f * gear_slow_resist(&RUN.gear));
                sfx_at(SFX_SKITTER, cx, cy, 1.0f); sfx_at(SFX_CREAK, cx, cy, 0.4f);
                for (int j = 0; j < MAXMOBS; j++) {                /* and the spiders near wake to it */
                    Mob *m = &G.mobs[j];
                    if (m->alive && (m->type == MOB_SPIDER || m->type == MOB_HUNTER || m->type == MOB_BROOD || m->type == MOB_LING) && hypotf(m->x - cx, m->y - cy) < 10.0f) m->alert = fmaxf(m->alert, 10.0f);
                }
#ifdef CRAWL_DEBUG
                fprintf(stderr, "EVENT snared at (%d,%d) by %s t=%.2f\n", s->x, s->y, G.player ? "player" : "autopilot", G.t);
#endif
            }
            break;
        case HZ_DCOLD:                                             /* rime where the fire went out: it is always there */
            for (int k = 0; k < s->n; k++) hz_mark(s->cx[k], s->cy[k], HZ_SOFT);
            for (int k = 0; k < s->n; k++) if (!G.dead && s->cx[k] == px && s->cy[k] == py) { if (af_off()) hz_chill(0.65f); else af_cold(dt); hz_seep(HZ_DCOLD, 0.012f, dt); break; }
            break;
        case HZ_TITHE:                                             /* the chalk takes its tithe while you stand in it */
            hz_mark_disc(cx, cy, 1.4f, HZ_SOFT);
            if (!G.dead && d < 1.4f) { drink = 1; hz_sap(HZ_TITHE, 0.8f, 0.0f, dt, cx, cy, &s->took); }
            break;
        case HZ_MIRROR:
            hz_mark_disc(s->ax - DX[s->d] * 1.5f, s->ay - DY[s->d] * 1.5f, 1.6f, HZ_SOFT);
            if (!G.dead && mirror_sees(s)) { drink = 1; hz_sap(HZ_MIRROR, 1.0f, 0.0f, dt, s->ax, s->ay, &s->took); }
            break;
        case HZ_RELIQ:                                             /* shut; its lid lifting; open and drinking; shut again a while */
            hz_mark_disc(cx, cy, 2.0f, HZ_SOFT);                   /* it drinks, it doesn't strike: hurried past, not fled */
            if (s->state == 0) {
                if ((s->wait -= dt) <= 0.0f && !G.dead && d < 2.0f && line_clear(G.px, G.py, cx, cy)) { s->state = 1; s->k = 0.0f; sfx_at(SFX_CREAK, cx, cy, 0.8f); sfx_at(SFX_WHISPER, cx, cy, 0.6f); }
            } else if (s->state == 1) {
                if ((s->k += dt) >= 0.9f) { s->state = 2; s->k = 0.0f; }
            } else {
                s->k += dt;
                if (G.dead || d >= 2.0f || s->k >= 8.0f) { s->state = 0; s->wait = 6.0f; sfx_at(SFX_KNOCK, cx, cy, 0.8f); }
                else { drink = 1; hz_sap(HZ_RELIQ, 1.0f, 0.006f, dt, cx, cy, &s->took); }
            }
            break;
        default:                                                   /* the rhythmic ones: rest, then a turn */
            if (!h && G.level_t > 4.0 && (s->wait -= dt) <= 0.0f) { h = site_fire(s, i); s->wait = site_rest(s->kind); }
            if (h && s->kind == HZ_SAINT && hz_phase(h) < 2 && !G.dead) {
                int k = hz_in_cells(h, px, py);
                if (k && hz_bile_wet(h, k - 1) > 0.5f) { drink = 1; hz_sap(HZ_SAINT, 1.0f, 0.004f, dt, s->ax, s->ay, &s->took); }
            }
            break;
        }
        s->drink = drink;
        if (s->wx >= 0) {                                          /* what the wall shows */
            int ph = h ? hz_phase(h) : 3, tex = G.map[s->wy][s->wx].tex;
            switch (s->kind) {
            case HZ_BREATH: tex = ph < 2 ? T_HZ_SLAB_OPEN : T_HZ_SLAB; break;
            case HZ_SLUICE: tex = ph == 1 || ph == 2 ? T_HZ_SLUICE_OPEN : T_HZ_SLUICE; break;
            case HZ_SAINT: tex = drink ? T_HZ_SAINT_LIT : T_HZ_SAINT; break;
            case HZ_LODE: tex = ph == 1 || (ph == 0 && fmodf((float)G.t * (4.0f + 10.0f * h->t / h->warn), 1.0f) < 0.5f) ? T_HZ_SEAM_LIT : T_HZ_SEAM; break;
            case HZ_FISSURE: tex = ph <= 1 ? T_HZ_FISSURE1 : T_HZ_FISSURE0; break;
            case HZ_MIRROR: tex = drink ? T_HZ_MIRROR_SEEN : T_HZ_MIRROR; break;
            default: break;
            }
            G.map[s->wy][s->wx].tex = (uint8_t)tex;
        }
    }
}
/* the arising ones: one near you every so often, from the run's third level */
static void update_arising(float dt) {
    int step = crawl_step(G.level), forced = -1;
    float every = step >= 2 ? 120.0f - 80.0f * fminf(1.0f, (float)(step - 2) / 6.0f) : 0.0f;
#ifdef CRAWL_DEBUG
    if (getenv("CRAWL_HZ")) {
        int k = atoi(getenv("CRAWL_HZ"));
        for (int s = 0; s < 4; s++) for (int j = 0; j < 3; j++) if (HZ_ARISE[s][j] == k) { forced = k; every = 8.0f; G.arise_t = fminf(G.arise_t, every); }
    }
#endif
    if (PK.depth || G.shop || G.tally || G.dead || G.exiting || G.level_t < 8.0 || every <= 0.0f || in_arena(G.px, G.py)) return;
    if ((G.arise_t -= dt) > 0.0f) return;
    G.arise_t = every * (0.75f + 0.5f * rndf());
    int kind = forced;
    if (kind < 0) {
        int set = hz_from_set(), nk = 0, ks[3];
        for (int j = 0; j < 3; j++) if (HZ_ARISE[set][j] >= 0) ks[nk++] = HZ_ARISE[set][j];
        kind = ks[rnd(nk)];
    }
    int x, y;
    switch (kind) {
    case HZ_ROCK: if (hz_near(1.0f, 5.0f, &x, &y)) hz_rock(x + 0.5f, y + 0.5f, 0); break;
    case HZ_FIRE:
        if (hz_near(1.0f, 4.0f, &x, &y)) { Hazard *h = hz_fire(0); float a = rndf() * TAU_F; if (h) { hz_line(h, x + 0.5f, y + 0.5f, x + 0.5f + cosf(a), y + 0.5f + sinf(a), 4 + rnd(3), 0); hz_fire_done(h, x + 0.5f, y + 0.5f); } }
        break;
    case HZ_VOID: if (hz_near(1.5f, 5.0f, &x, &y)) hz_void(x + 0.5f, y + 0.5f, 1.2f, 0); break;
    case HZ_BILE:                                                  /* a drain in a wall near you, as the omen's */
        for (int tries = 0; tries < 40; tries++) {
            if (!hz_near(2.0f, 6.0f, &x, &y)) break;
            int d = rnd(4), gx = x + DX[d], gy = y + DY[d];
            Cell *w = &G.map[gy][gx];
            if (w->kind != K_WALL || w->secret || !plain_tex(w->tex) || w->tex == T_TORCH0 || gx <= 0 || gy <= 0 || gx >= MAPN - 1 || gy >= MAPN - 1) continue;
            int8_t cx[HZ_CELLS], cy[HZ_CELLS]; uint8_t cd[HZ_CELLS];
            int n = hz_patch(x, y, 2, cx, cy, cd, 12);
            Hazard *h = n ? hz_bile(cx, cy, cd, n, 0) : NULL;
            if (h) { h->on = 8.0f; h->share = 0.7f; h->wx = gx; h->wy = gy; h->wtex = w->tex; w->tex = T_GRATE; }
            break;
        }
        break;
    case HZ_VOTIVE:                                                /* a rack goes over: burning wax down a line */
        if (hz_near(1.5f, 4.5f, &x, &y)) {
            Hazard *h = hz_new(HZ_VOTIVE, 0);
            float a = rndf() * TAU_F;
            if (!h) break;
            hz_line(h, x + 0.5f, y + 0.5f, x + 0.5f + cosf(a), y + 0.5f + sinf(a), 3 + rnd(2), 0);
            if (!h->n) { h->alive = 0; break; }
            h->x = x + 0.5f; h->y = y + 0.5f; h->r = 4.0f; h->warn = hz_warn(1.0f); h->on = 3.0f; h->off = 1.5f; h->share = 0.07f;
            sfx_at(SFX_SHATTER, h->x, h->y, 0.6f); sfx_at(SFX_IGNITE, h->x, h->y, 0.6f);
        }
        break;
    case HZ_GALV: case HZ_SILT: case HZ_ASH: case HZ_GLASS: {
        if (!hz_near(kind == HZ_GLASS ? 0.0f : 1.5f, kind == HZ_GLASS ? 3.0f : 5.0f, &x, &y)) break;
        int want = kind == HZ_GALV ? FL2_WATER : kind == HZ_SILT ? FL2_SILT : kind == HZ_GLASS ? FL4_GLASS : -1;
        for (int t = 0; t < 12 && want >= 0 && cell(x, y)->flo != T_FLOORS + want; t++) { int ax, ay; if (hz_near(1.0f, 6.0f, &ax, &ay) && cell(ax, ay)->flo == T_FLOORS + want) { x = ax; y = ay; } }   /* its own floor, if near */
        if (kind == HZ_ASH) hz_disc(HZ_ASH, x + 0.5f, y + 0.5f, 2.0f, 1.5f, 6.0f, 2.0f, 0.03f);
        else if (kind == HZ_GLASS) { if (hz_disc(HZ_GLASS, x + 0.5f, y + 0.5f, 0.9f, 1.0f, 1.0f, 0.8f, 0.15f)) sfx_at(SFX_SHATTER, x + 0.5f, y + 0.5f, 0.5f); }
        else {
            int8_t cx[HZ_CELLS], cy[HZ_CELLS]; uint8_t cd[HZ_CELLS];
            int n;
            if (kind == HZ_GALV && cell(x, y)->flo == T_FLOORS + FL2_WATER) {   /* the water near it */
                n = hz_patch(x, y, 4, cx, cy, cd, HZ_CELLS);
                int m = 0;
                for (int k = 0; k < n; k++) if (cell(cx[k], cy[k])->flo == T_FLOORS + FL2_WATER) { cx[m] = cx[k]; cy[m] = cy[k]; cd[m] = cd[k]; m++; }
                n = m;
            } else n = hz_patch(x, y, kind == HZ_SILT ? 1 + rnd(2) : 1, cx, cy, cd, kind == HZ_SILT ? 9 : 6);
            Hazard *h = kind == HZ_GALV ? hz_cells(HZ_GALV, cx, cy, cd, n, 1.5f, 0.15f, 0.6f, 0.14f) : hz_cells(HZ_SILT, cx, cy, cd, n, 1.4f, 5.0f, 1.5f, 0.4f);
            if (h) sfx_at(kind == HZ_GALV ? SFX_CRACKLE : SFX_HISS, h->x, h->y, 0.7f);
        }
        break;
    }
    default: break;
    }
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT arising %d level %d t=%.2f\n", kind, G.level, G.t);
#endif
}
/* the lights they give: a fissure growing hot, a crystal singing, the reliquary open, the chalk lit */
static void hz_lights(void) {
    if (PK.depth || G.shop) return;
    for (int i = 0; i < G.nsite; i++) {
        const HzSite *s = &G.site[i];
        const Hazard *h = s->haz >= 0 && G.haz[s->haz].alive ? &G.haz[s->haz] : NULL;
        float k = h && h->pre <= 0.0f ? clampf(h->t / fmaxf(0.1f, h->warn), 0.0f, 1.0f) : 0.0f;
        int ph = h ? hz_phase(h) : 3;
        if (s->kind == HZ_FISSURE) light_dynamic(s->ax - DX[s->d] * 0.3f, s->ay - DY[s->d] * 0.3f, 0.5f + (ph == 0 ? 1.4f * k : ph == 1 ? 2.0f : 0.0f), 3.6f);
        else if (s->kind == HZ_CRYSTAL) light_dynamic(s->x + 0.5f, s->y + 0.5f, 0.6f + (ph == 0 ? 1.6f * k : 0.0f) + 0.1f * sinf((float)G.t * 3.0f + i), 3.4f);
        else if (s->kind == HZ_RELIQ && s->state) light_dynamic(s->x + 0.5f, s->y + 0.5f, 0.9f, 2.8f);
        else if (s->kind == HZ_TITHE && s->drink) light_dynamic(s->x + 0.5f, s->y + 0.5f, 0.7f + 0.2f * sinf((float)G.t * 6.0f), 2.4f);
        else if (s->kind == HZ_LODE && ph <= 1) light_dynamic(s->ax - DX[s->d] * 0.3f, s->ay - DY[s->d] * 0.3f, 0.4f + 1.4f * k, 3.0f);
    }
    for (int i = 0; i < G.nthing; i++) {                           /* a puzzle's lights, lit */
        const Thing *t = &G.thing[i];
        if (t->role == TR_LIGHT && t->state) light_dynamic(t->x, t->y, 0.8f + 0.1f * sinf((float)G.t * 7.0f + i), 2.6f);
        else if (t->role == TR_STRIKE && t->t < 1.2f) light_dynamic(t->x, t->y, 0.9f * (1.0f - t->t / 1.2f), 2.4f);   /* a note's glow */
        else if (t->tri && t->role == TR_STRIKE && t->state) light_dynamic(t->x, t->y, 0.35f, 1.8f);   /* a trial's bell, rung */
    }
}


static int pz_takes(int place, int item);
static void pz_touched(int i, int how);
static int pile_count(int place);
static void doc_add(const char *where, const char *text, int msg);
