/* A part of scene_crawl.c (docs/PLAN-tidy.md): it is #included there in its place and never compiled on its own.
 * Hazards: the bosses' six, the runs' own by element, what they mark for the dream, their harm to the caster and to the dead, update_hazards, af_tick. */

/* ---- hazards (docs/PLAN-b7.md, section 6) ----
 * Rockfall: grit and a growing shadow, then a rock that strikes once and lies as rubble. Hellrent:
 * cracks running along a line, glowing up to the burst, then fire for a moment and embers. Bile: a wet
 * sheen creeping from an outlet a cell at a time, foul (poison, and slow) once it has settled, draining
 * back when the outlet shuts. The void: a ring and a thin column, then a purple volume that saps health,
 * slows and silences. The Sundering Arc: a pale arc on the floor before the knight, then its sweep.
 * Grasping Dark: a black ring under you, then hands. Omens are the same at half the harm. */
static uint8_t hz_dmap[MAPN][MAPN];                                 /* warned or harmful cells, for the autopilot */
#define HZ_SOFT 3                                                  /* a cell always so (a sapper's haze, rime, a puzzle's stones): a little out of the way */
static int hz_quiet;                                               /* a fixed one's turn: named when first seen, not when it fires */
static void hz_name(int kind);
static int hz_step(void) { return G.boss >= 0 ? G.bstep : 0; }
static float hz_warn(float base) { return fmaxf(0.9f, base - 0.06f * (float)hz_step()); }   /* never under 0.9 s */
static Hazard *hz_new(int kind, int omen) {
    for (int i = 0; i < MAXHAZ; i++)
        if (!G.haz[i].alive) {
            Hazard *h = &G.haz[i];
            memset(h, 0, sizeof *h);
            h->alive = 1; h->kind = kind; h->omen = omen; h->src = -1; h->wx = -1; h->site = -1;
            if ((RUN.corner >> CS_OMEN & 1) && kind != HZ_ARC) h->pre = 1.0f;   /* Premonition: a second's more notice (the arc keeps the blade's time) */
#ifdef CRAWL_DEBUG
            fprintf(stderr, "EVENT hazard %d%s t=%.2f\n", kind, omen ? " omen" : "", G.t);
#endif
            if (!hz_quiet) hz_name(kind);
            return h;
        }
    return NULL;
}
static int hz_phase(const Hazard *h) { return h->t < h->warn ? 0 : (h->t < h->warn + h->on ? 1 : 2); }   /* warning, harm, fading */
static int hz_floor(int x, int y) { return x > 0 && y > 0 && x < MAPN - 1 && y < MAPN - 1 && G.map[y][x].kind == K_FLOOR && !G.map[y][x].secret; }

static void hz_rock(float x, float y, int omen) {
    Hazard *h = hz_new(HZ_ROCK, omen);
    if (!h) return;
    h->x = x; h->y = y; h->r = 0.55f; h->warn = hz_warn(1.2f); h->on = 0.05f; h->off = 3.0f; h->share = omen ? 0.06f : 0.12f;
    sfx_at(SFX_CREAK, x, y, 0.7f);
}
/* cells for hellrent: a line from (x,y) towards (tx,ty), so many long, in order */
static int hz_line(Hazard *h, float x, float y, float tx, float ty, int len, int from) {
    float a = atan2f(ty - y, tx - x), cx = cosf(a), cy = sinf(a);
    int n = h->n;
    for (int i = 0; i < len * 3 && n < HZ_CELLS; i++) {
        int px = (int)floorf(x + cx * i / 3.0f), py = (int)floorf(y + cy * i / 3.0f), dup = 0;
        if (!hz_floor(px, py)) { if (G.map[clampi(py, 0, MAPN - 1)][clampi(px, 0, MAPN - 1)].kind == K_WALL) break; continue; }
        for (int k = 0; k < n; k++) dup |= h->cx[k] == px && h->cy[k] == py;
        if (dup) continue;
        h->cx[n] = (int8_t)px; h->cy[n] = (int8_t)py; h->cd[n] = (uint8_t)(from + i / 3); n++;
        if (i / 3 >= len) break;
    }
    return h->n = n;
}
static Hazard *hz_fire(int omen) {
    Hazard *h = hz_new(HZ_FIRE, omen);
    if (!h) return NULL;
    h->warn = hz_warn(1.5f); h->on = 1.2f; h->off = 2.0f; h->share = omen ? 0.04f : 0.08f;
    return h;
}
static void hz_fire_done(Hazard *h, float sx, float sy) {
    if (!h) return;
    if (!h->n) { h->alive = 0; return; }
    int maxd = 1;
    for (int i = 0; i < h->n; i++) if (h->cd[i] > maxd) maxd = h->cd[i];
    h->r = (float)maxd;
    h->x = sx; h->y = sy;
    sfx_at(SFX_PUSHWALL, sx, sy, 0.6f);
}
static Hazard *hz_bile(const int8_t *cx, const int8_t *cy, const uint8_t *cd, int n, int omen) {
    Hazard *h = hz_new(HZ_BILE, omen);
    if (!h) return NULL;
    h->n = imin(n, HZ_CELLS);
    int maxd = 1;
    for (int i = 0; i < h->n; i++) { h->cx[i] = cx[i]; h->cy[i] = cy[i]; h->cd[i] = cd[i]; if (cd[i] > maxd) maxd = cd[i]; }
    h->r = (float)maxd; h->warn = 2.0f; h->on = omen ? 8.0f : 1e6f; h->off = 3.0f; h->share = omen ? 0.5f : 1.0f;
    if (h->n) { h->x = h->cx[0] + 0.5f; h->y = h->cy[0] + 0.5f; }
    sfx_at(SFX_DRIP, h->x, h->y, 1.0f); sfx_at(SFX_GROAN, h->x, h->y, 0.3f);
    return h;
}
static void hz_void(float x, float y, float r, int omen) {
    Hazard *h = hz_new(HZ_VOID, omen);
    if (!h) return;
    h->x = x; h->y = y; h->r = r; h->warn = hz_warn(1.5f); h->on = omen ? 5.0f : 6.0f + 2.0f * rndf(); h->off = 1.0f; h->share = omen ? 0.02f : 0.04f;
    sfx_at(SFX_SHIMMER, x, y, 0.5f);
}
static void hz_arc(float x, float y, float ang, float share) {
    Hazard *h = hz_new(HZ_ARC, 0);
    if (!h) return;
    h->x = x; h->y = y; h->ang = ang; h->r = 3.2f; h->warn = hz_warn(0.8f); h->on = 0.05f; h->off = 0.4f; h->share = share;
}
static void hz_grasp(float x, float y) {
    Hazard *h = hz_new(HZ_GRASP, 0);
    if (!h) return;
    h->x = x; h->y = y; h->r = 0.8f; h->warn = hz_warn(1.0f); h->on = 0.05f; h->off = 0.7f; h->share = 0.10f;
    sfx_at(SFX_WHISPER, x, y, 0.8f);
}


/* ---- the runs' hazards (1.0.0b13, docs/PLAN-b13.md, part B) ----
 * Each run has its own. The fixed ones are placed with the level, in a room or hall of their own, with a
 * rhythm that can be learnt; the arising ones come up near you, as the omens do. One fixed on a run's first
 * level, about five by its ninth; arising ones from its third level, every 120 s, down to every 40 by its
 * ninth; the soul-sappers two or three a level from its sixth. Now and then an earlier run's comes back.
 * None strikes for more than a quarter of your health, none twice inside its own warning, and each is of
 * an element, resisted as that element is. None in pockets, her hall, or a boss's arena. */
static const int8_t HZ_FIXED[4][4] = { { HZ_CENSER, HZ_BREATH, HZ_COLD, -1 }, { HZ_SLUICE, HZ_SNARE, -1, -1 },
                                       { HZ_TIMBER, HZ_DAMP, HZ_CART, HZ_LODE }, { HZ_FISSURE, HZ_CRYSTAL, HZ_DCOLD, -1 } };
static const int8_t HZ_ARISE[4][3] = { { HZ_ROCK, HZ_VOTIVE, -1 }, { HZ_BILE, HZ_GALV, HZ_SILT }, { HZ_FIRE, -1, -1 }, { HZ_VOID, HZ_ASH, HZ_GLASS } };
static const int8_t HZ_SAPPER[4] = { HZ_RELIQ, HZ_SAINT, HZ_TITHE, HZ_MIRROR };
static const int8_t HZ_ELEM[NHZ] = { EL_PHYS, EL_FIRE, EL_POISON, -1, EL_PHYS, EL_PHYS,
                                     EL_PHYS, EL_POISON, EL_FIRE, EL_FROST, EL_SPIRIT, EL_FROST, EL_LIGHT, EL_PHYS, EL_POISON, EL_SPIRIT,
                                     EL_PHYS, EL_POISON, EL_PHYS, EL_LIGHT, EL_SPIRIT, EL_FIRE, EL_POISON, EL_LIGHT, EL_PHYS, EL_FROST, EL_SPIRIT };
static const int8_t HZ_SET[NHZ] = { 0, 2, 1, 3, 0, 3, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3 };   /* whose it is */
static int hz_cellkind(int k) {                                    /* covers a list of cells, not a disc */
    return k == HZ_FIRE || k == HZ_BILE || k == HZ_BREATH || k == HZ_VOTIVE || k == HZ_COLD || k == HZ_SLUICE || k == HZ_GALV
        || k == HZ_SILT || k == HZ_SAINT || k == HZ_TIMBER || k == HZ_DAMP || k == HZ_CART || k == HZ_FISSURE;
}
static int hz_from_set(void) {                                     /* the run's own, or an earlier run's: one in 7, 15, 30 */
    static const float P[4] = { 0.0f, 1.0f / 7.0f, 1.0f / 15.0f, 1.0f / 30.0f };
    int set = crawl_set(G.level);
    float r = rndf(), acc = 0.0f;
    for (int back = 1; back <= set; back++) { acc += P[back]; if (r < acc) return set - back; }
    return set;
}
static float elem_resist(int el) {                                 /* the share of an element's harm shrugged off */
    const Gear *g = &RUN.gear;
    switch (el) {
    case EL_FIRE: return gear_magic_resist(g);
    case EL_FROST: return gear_slow_resist(g);
    case EL_LIGHT: return gear_light_resist(g);
    case EL_POISON: return gear_poison_resist(g);
    case EL_SPIRIT: return gear_total(g, ST_RES_SPIRIT) / 100.0f;
    default: return 0.0f;
    }
}
/* a stroke: a share of your health of an element, a quarter at most, nothing to dodge; lightning may break a
 * spell being gathered, unless Resist Lightning holds it */
static int hz_strike_el(int kind, int el, float share, float sx, float sy) {
    if (G.dead) return 0;
    int mhp = G.max_hp > 0 ? G.max_hp : 100;
    G.blame = (Blame){ BL_HAZARD, (uint8_t)kind, 0 };
    harm_next = el;
    int d = hurt_ex(fminf(share, 0.25f) * (float)mhp, HURT_ELEM, sx, sy);
    if (d > 0 && el == EL_LIGHT && winding_up() && rndf() >= gear_light_resist(&RUN.gear)) windup_break();
    if (d > 0) af_stroke(el, (float)d);                            /* its element's status (S9, S10) */
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT hz_hit %d %d t=%.2f\n", kind, d, G.t);
#endif
    return d;
}
static int hz_strike(int kind, float share, float sx, float sy) { return hz_strike_el(kind, HZ_ELEM[kind], share, sx, sy); }
static void hz_seep(int kind, float share_s, float dt) {           /* harm a second, a point at a time, resisted as it comes */
    int el = HZ_ELEM[kind], mhp = G.max_hp > 0 ? G.max_hp : 100;
    if (el < 0) return;
    G.hz_el_acc[el] += share_s * (float)mhp * dt * (1.0f - elem_resist(el));
    G.hz_el_by[el] = (uint8_t)kind;
}
static void hz_poison(int kind, float share) {                     /* poisoned as bile poisons: Purify keeps it off */
    if (G.purify_t > 0.0f) return;
    float r = 1.0f - gear_poison_resist(&RUN.gear), was = G.poison_t;
    G.poison_t = fmaxf(G.poison_t, 2.5f * r); G.poison_by = (Blame){ BL_HAZARD, (uint8_t)kind, 0 };
    af_poison(G.poison_t - was);
    G.poison_dps = fmaxf(G.poison_dps, (1.0f + (float)imax(0, (G.level - 4) / 2)) * share * (0.5f + 0.5f * r));
}
static void hz_chill(float slow) { G.hz_slow = fminf(G.hz_slow, 1.0f - (1.0f - slow) * (1.0f - 0.5f * gear_slow_resist(&RUN.gear))); }
/* drunk: souls a second (fewer with Resist Spirit), and a little health; a violet thread from you to it */
static void hz_sap(int kind, float souls_s, float hp_s, float dt, float sx, float sy, float *took) {
    float res = 1.0f - gear_total(&RUN.gear, ST_RES_SPIRIT) / 100.0f, c = fminf(spirit(), souls_s * res * dt);
    if (c > 0.0f && spend_spirit(c)) *took += c;
    if (hp_s > 0.0f) hz_seep(kind, hp_s, dt);
    G.hz_slow = fminf(G.hz_slow, 0.9f);
    if (fmodf((float)G.t, 0.25f) < dt) add_beam(AT_DRAIN, sx, sy, 0.45f, G.px, G.py, 0.4f);
}
static void hz_mark(int x, int y, int v) {
    if (x < 0 || y < 0 || x >= MAPN || y >= MAPN) return;
    if (v == HZ_SOFT) { if (!hz_dmap[y][x]) hz_dmap[y][x] = HZ_SOFT; return; }
    if (hz_dmap[y][x] == HZ_SOFT || hz_dmap[y][x] < v) hz_dmap[y][x] = (uint8_t)v;
}
static void hz_mark_disc(float x, float y, float r, int v) {
    for (int yy = (int)floorf(y - r); yy <= (int)floorf(y + r); yy++)
        for (int xx = (int)floorf(x - r); xx <= (int)floorf(x + r); xx++)
            if (hypotf(xx + 0.5f - x, yy + 0.5f - y) < r + 0.35f) hz_mark(xx, yy, v);
}
/* what else stands in it takes its share of its own health, by its weakness to the element; not a boss, an ally or a spirit */
static void hz_hurt_mob(Mob *m, float share, int el, float sx, float sy) {
    if (!m->alive || m->state != MS_WALK || m->boss || m->ally || KIND_IS(m, KF_SPIRIT) || buried(m)) return;
    damage_mob(m, share * 1.5f * m->maxhp * (el >= 0 ? KINDS[m->type].weak[el] : 1.0f), DM_TICK, sx, sy, 0.0f);
}
/* thrown: a person by the push's own way, the dream to a lane cell that way, as a dart is */
static void hz_shove(float vx, float vy, float cells) {
    float len = hypotf(vx, vy);
    if (len < 1e-3f || G.dash_t > 0.0f || G.dead) return;
    vx /= len; vy /= len;
    if (G.player) {
        float r = dash_reach(atan2f(vy, vx), cells);
        if (r < 0.3f) return;
        G.dash_t = 0.25f; G.dash_vx = vx * r / 0.25f; G.dash_vy = vy * r / 0.25f;
        return;
    }
    int bx = (int)G.px, by = (int)G.py, d = fabsf(vx) > fabsf(vy) ? (vx > 0.0f ? 0 : 2) : (vy > 0.0f ? 1 : 3), best = 0;
    for (int k = 1; k <= (int)ceilf(cells) + 1; k++) {
        int lx = bx + DX[d] * k, ly = by + DY[d] * k;
        if (!passable(lx, ly) || cell(lx, ly)->kind == K_DOOR || occupied(lx, ly, NULL)) break;
        if (apass(lx, ly) && walk_clear(G.px, G.py, lx + 0.5f, ly + 0.5f)) best = k;
    }
    if (!best) return;
    G.tx = bx + DX[d] * best; G.ty = by + DY[d] * best;
    G.dash_t = 0.25f; G.dash_vx = (G.tx + 0.5f - G.px) / 0.25f; G.dash_vy = (G.ty + 0.5f - G.py) / 0.25f;
}
static void hz_name(int kind) {                                    /* the first of each kind is named */
    static const char *const NAME[NHZ] = { "THE CEILING GROANS ABOVE YOU", "THE FLOOR SPLITS AND BURNS", "BILE RISES FROM THE DRAINS",
        "THE VOID OPENS", "MIND THE BLADE'S ARC", "THE DARK REACHES UP",
        "A CENSER SWINGS ACROSS THE WAY", "A SLAB BREATHES IN ITS NICHE", "THE VOTIVES GO OVER", "THE VAULT BREATHES COLD",
        "A RELIQUARY HUNGERS", "A SLUICE GROANS IN THE WALL", "LIGHTNING IN THE WATER", "A WEB ACROSS THE WAY",
        "THE SILT GIVES UP ITS GAS", "A SAINT WEEPS", "THE TIMBERS GROAN", "FIREDAMP POOLS HERE", "A CART WAITS ON THE RAILS",
        "THE SEAM CRACKLES", "A CIRCLE OF CHALK", "THE FISSURE BREATHES", "ASH COMES DOWN", "A CRYSTAL SINGS",
        "THE GLASS SPLITS UNDERFOOT", "THE COLD WHERE THE FIRE WAS", "A BLACK MIRROR" };
    if (kind < 0 || kind >= NHZ || (RUN.omens >> kind & 1)) return;
    RUN.omens |= 1 << kind;
    say("%s", NAME[kind]);
}
static Hazard *hz_cells(int kind, const int8_t *cx, const int8_t *cy, const uint8_t *cd, int n, float warn, float on, float off, float share) {
    Hazard *h = hz_new(kind, 0);
    if (!h) return NULL;
    h->n = imin(n, HZ_CELLS);
    int maxd = 1;
    for (int i = 0; i < h->n; i++) { h->cx[i] = cx[i]; h->cy[i] = cy[i]; h->cd[i] = cd[i]; if (cd[i] > maxd) maxd = cd[i]; }
    h->r = (float)maxd; h->warn = hz_warn(warn); h->on = on; h->off = off; h->share = share;
    if (h->n) { h->x = h->cx[0] + 0.5f; h->y = h->cy[0] + 0.5f; }
    return h;
}
static Hazard *hz_disc(int kind, float x, float y, float r, float warn, float on, float off, float share) {
    Hazard *h = hz_new(kind, 0);
    if (!h) return NULL;
    h->x = x; h->y = y; h->r = r; h->warn = hz_warn(warn); h->on = on; h->off = off; h->share = share;
    return h;
}
static int hz_in_cells(const Hazard *h, int x, int y) { for (int i = 0; i < h->n; i++) if (h->cx[i] == x && h->cy[i] == y) return i + 1; return 0; }
/* a fire spell, a burning floor or an exploding fireball in its cells: silt gas and firedamp go up */
static int hz_fire_in(const Hazard *h) {
    for (int i = 0; i < MAXSHOTS; i++) {
        const Shot *s = &G.shots[i];
        if (s->alive && s->kind < NATK && ATK_ELEM[s->kind] == EL_FIRE && hz_in_cells(h, (int)s->x, (int)s->y)) return 1;
    }
    for (int i = 0; i < MAXBURNS; i++) if (G.burns[i].alive && hz_in_cells(h, (int)G.burns[i].x, (int)G.burns[i].y)) return 1;
    for (int i = 0; i < MAXBOOMS; i++) if (G.booms[i].alive && G.booms[i].t < 0.1f && hz_in_cells(h, (int)G.booms[i].x, (int)G.booms[i].y)) return 1;
    return 0;
}
static void hz_burst(Hazard *h) {                                  /* the gas goes up: Fire to whatever stands in it */
    int px = (int)G.px, py = (int)G.py;
    for (int i = 0; i < h->n; i += 2) explode(h->cx[i] + 0.5f, h->cy[i] + 0.5f);
    if (!G.dead && hz_in_cells(h, px, py)) hz_strike_el(h->kind, EL_FIRE, h->kind == HZ_DAMP ? 0.15f : 0.12f, h->x, h->y);
    for (int i = 0; i < MAXMOBS; i++) if (G.mobs[i].alive && hz_in_cells(h, (int)G.mobs[i].x, (int)G.mobs[i].y)) hz_hurt_mob(&G.mobs[i], 0.15f, EL_FIRE, h->x, h->y);
    h->t = h->warn + h->on;                                        /* and is gone */
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT hz_burst %d t=%.2f\n", h->kind, G.t);
#endif
}
/* the dream's way, and everything that hunts, still through if these cells were shut: so the timbers may close them */
static int hz_may_close(const Hazard *h) {
    static uint8_t seen[MAPN][MAPN];
    static int qx[MAPN * MAPN], qy[MAPN * MAPN];
    for (int i = 0; i < h->n; i++) {
        int x = h->cx[i], y = h->cy[i];
        if ((int)G.px == x && (int)G.py == y) return 0;
        for (int j = 0; j < MAXMOBS; j++) if (G.mobs[j].alive && (int)G.mobs[j].x == x && (int)G.mobs[j].y == y) return 0;
    }
    int total = 0, reach = 0;
    for (int y = 0; y < MAPN; y++) for (int x = 0; x < MAPN; x++) total += passable(x, y) && !hz_in_cells(h, x, y);
    memset(seen, 0, sizeof seen);
    int qh = 0, qt = 0;
    qx[qt] = (int)G.px; qy[qt++] = (int)G.py; seen[(int)G.py][(int)G.px] = 1;
    while (qh < qt) {
        int x = qx[qh], y = qy[qh++];
        reach++;
        for (int d = 0; d < 4; d++) {
            int nx = x + DX[d], ny = y + DY[d];
            if (nx < 0 || ny < 0 || nx >= MAPN || ny >= MAPN || seen[ny][nx] || !passable(nx, ny) || hz_in_cells(h, nx, ny)) continue;
            seen[ny][nx] = 1; qx[qt] = nx; qy[qt++] = ny;
        }
    }
    return reach >= total;
}
static void hz_timber_close(Hazard *h, int shut) {
    for (int i = 0; i < h->n; i++) {
        Cell *c = &G.map[h->cy[i]][h->cx[i]];
        if (shut && c->kind == K_FLOOR) c->kind = K_PROP;
        else if (!shut && c->kind == K_PROP) c->kind = K_FLOOR;
    }
    h->hit = shut ? 2 : 0;
    hunt_cx = -1;                                                  /* the dead find their way again */
}


/* how far a bile cell has come: 0 dry, up to 1 settled and foul (the sheen creeps out, then drains back) */
static float hz_bile_wet(const Hazard *h, int i) {
    float creep = h->t / h->warn * (h->r + 1.0f) - (float)h->cd[i];
    if (h->t >= h->warn + h->on) creep = fminf(creep, (1.0f - (h->t - h->warn - h->on) / h->off) * (h->r + 1.0f) - (float)h->cd[i]);
    return clampf(creep, 0.0f, 1.0f);
}
static int hz_in_arc(const Hazard *h, float x, float y) {
    float d = hypotf(x - h->x, y - h->y);
    return d <= h->r && d > 0.2f && fabsf(wrap_pi(atan2f(y - h->y, x - h->x) - h->ang)) <= 0.8f;
}
/* how much a hazard has a cell: 2 harming, 1 warning (or fading fire), 0 not at all */
static float cart_at(const Hazard *h) { return clampf((h->t - h->warn) / fmaxf(0.05f, h->on), 0.0f, 1.0f) * (float)(h->n - 1); }   /* how far down its rails */
static int hz_covers(const Hazard *h, int x, int y) {
    int ph = hz_phase(h);
    float cx = x + 0.5f, cy = y + 0.5f;
    if (h->kind >= HZ_CENSER) {                                    /* the runs' own (1.0.0b13) */
        if (hz_cellkind(h->kind)) {
            int k = hz_in_cells(h, x, y);
            if (!k) return 0;
            switch (h->kind) {
            case HZ_SAINT: return ph < 2 && hz_bile_wet(h, k - 1) > 0.0f ? 1 : 0;
            case HZ_CART: { if (ph == 0) return 1; if (ph != 1) return 0; float dk = (float)h->cd[k - 1] - cart_at(h); return dk >= -0.6f && dk < 2.5f ? 2 : (dk >= -0.6f ? 1 : 0); }
            case HZ_DAMP: if (h->src == -7 || h->on > 100.0f) return ph < 2 ? HZ_SOFT : 0;   /* a trial's gas, or a stope's that hangs till
                                                                      fire sets it off, only chokes: crossed when it must be (it had held
                                                                      the dream at a doorway till it gave the level up) */
                return ph == 1 ? 2 : (ph == 0 && h->t > h->warn * 0.5f ? 1 : 0);
            default: return ph == 1 ? 2 : (ph == 0 ? 1 : 0);
            }
        }
        if (hypotf(cx - h->x, cy - h->y) >= h->r + 0.35f) return 0;
        return ph == 1 ? 2 : (ph == 0 ? 1 : 0);
    }
    switch (h->kind) {
    case HZ_ROCK: return ph == 0 && hypotf(cx - h->x, cy - h->y) < h->r + 0.45f ? 1 : 0;
    case HZ_FIRE: for (int i = 0; i < h->n; i++) if (h->cx[i] == x && h->cy[i] == y) return ph == 1 ? 2 : (ph == 0 ? 1 : 0); return 0;
    case HZ_BILE: for (int i = 0; i < h->n; i++) if (h->cx[i] == x && h->cy[i] == y) { float w = hz_bile_wet(h, i); return w >= 1.0f && ph < 2 ? 2 : (w > 0.0f ? 1 : 0); } return 0;
    case HZ_VOID: return ph < 2 && hypotf(cx - h->x, cy - h->y) < h->r + 0.35f ? (ph == 1 ? 2 : 1) : 0;
    case HZ_ARC: return ph == 0 && hz_in_arc(h, cx, cy) ? 1 : 0;
    default: return ph == 0 && hypotf(cx - h->x, cy - h->y) < h->r + 0.35f ? 1 : 0;   /* HZ_GRASP */
    }
}

static int hz_pre_covers(const Hazard *h, int x, int y) {          /* what Premonition marks before it warns */
    if (hz_cellkind(h->kind)) { for (int i = 0; i < h->n; i++) if (h->cx[i] == x && h->cy[i] == y) return 1; return 0; }
    float cx = x + 0.5f, cy = y + 0.5f;
    if (h->kind == HZ_ARC) return hz_in_arc(h, cx, cy);
    return hypotf(cx - h->x, cy - h->y) < h->r + 0.45f;
}
/* D3 (docs/PLAN-ai.md, section 15; step 7): the hazards that hurt only you before now hurt the dead too, at half the share;
 * the runs' own that struck them already (the galvanic water, the timbers, the carts, the arcs, the glass) do as they did.
 * Blue and up walk round them (hz_shuns); green and white blunder in. Not a boss or its adds, nor what is over a floor's harm.
 * Harm, not a blow: nothing learns where you are from it */
static void hz_harm(Mob *m, float dmg, int el) {
    if (el >= 0) dmg *= KINDS[m->type].weak[el];
    if (dmg <= 0.0f) return;
    m->hp -= dmg;
    knight_shed(m);
    if (m->hp <= 0.0f) kill_mob(m);
}
static void hz_mobs(Hazard *h, int struck, int ph, float dt) {
    if (!mind_on() || !since(31)) return;                          /* from 31, in section 23's rollout */
    int k = h->kind, floor = k == HZ_FIRE || k == HZ_BILE || k == HZ_SLUICE || k == HZ_VOTIVE, strike = k == HZ_ROCK || k == HZ_SLUICE;
    switch (k) {
    case HZ_ROCK: if (!struck) return; break;
    case HZ_FIRE: case HZ_VOID: case HZ_VOTIVE: case HZ_FISSURE: case HZ_COLD: case HZ_ASH: case HZ_SLUICE: if (ph != 1) return; break;
    case HZ_BREATH: case HZ_DAMP: case HZ_SILT: if (ph != 1 || h->src == -7) return; break;   /* a trial's gas only chokes you */
    case HZ_BILE: if (ph >= 2) return; break;
    default: return;
    }
    int el = HZ_ELEM[k];
    for (int j = 0; j < MAXMOBS; j++) {
        Mob *m = &G.mobs[j];
        if (!m->alive || m->state != MS_WALK || m->boss || m->add || m->ally || KIND_IS(m, KF_SPIRIT) || buried(m) || m->sleep) continue;
        if (floor && (m->type == MOB_BAT || (m->type == MOB_GARGOYLE && m->sub >= 1) || (m->type == MOB_HUNTER && m->sub < 2) || m->sub == 3)) continue;
        int mx = (int)m->x, my = (int)m->y, in = 0;
        switch (k) {
        case HZ_ROCK: in = hypotf(m->x - h->x, m->y - h->y) < h->r + KINDS[m->type].rad; break;
        case HZ_FIRE: case HZ_BILE:
            for (int i = 0; i < h->n; i++) if (h->cx[i] == mx && h->cy[i] == my) { in = k == HZ_FIRE || hz_bile_wet(h, i) >= 1.0f; break; }
            break;
        case HZ_VOID: in = hypotf(m->x - h->x, m->y - h->y) < h->r; break;
        case HZ_SLUICE: { int c = hz_in_cells(h, mx, my); in = c && (float)h->cd[c - 1] <= (h->t - h->warn) * 6.0f + 0.5f; break; }
        default: in = hz_covers(h, mx, my) == 2; break;
        }
        if (!in) continue;
        uint64_t bit = 1ull << j;
        if (strike) {
            if (k == HZ_SLUICE && (h->hitmask & bit)) continue;
            h->hitmask |= bit;
            DBG_COUNT(hz_mob_hits);
            hz_harm(m, 0.5f * h->share * m->maxhp, el);
        } else {
            if (!(h->hitmask & bit)) { h->hitmask |= bit; DBG_COUNT(hz_mob_hits); }
            if (k == HZ_COLD) m->slowed = fmaxf(m->slowed, 0.5f);
            hz_harm(m, 0.5f * h->share * m->maxhp * dt, el);
        }
    }
}
static void hz_dput(int x, int y, int c) {                         /* the worse of two marks; what is always there yields to a warning or harm */
    uint8_t *d = &hz_dmap[y][x];
    if (c == HZ_SOFT) { if (!*d) *d = HZ_SOFT; }
    else if (c && (*d == HZ_SOFT || c > *d)) *d = (uint8_t)c;
}
static void drawn_fire_dmarks(void) {                              /* the dream crosses it, there being no other way */
    if (!g_drawn || !DF.n || DF.fade < 0.35f) return;
    for (int i = 0; i < DF.n; i++) hz_dput(DF.cx[i], DF.cy[i], HZ_SOFT);
}
static void update_hazards(float dt) {
    int was = G.silenced;
    G.silenced = 0; G.hz_slow = 1.0f;
    memset(hz_dmap, 0, sizeof hz_dmap);
    if (PK.depth || G.shop || G.tally) { if (was) snd_set_muffle(0); return; }
    mk_dmarks();                                                   /* the steam vents (1.0.0b16) */
    c4_dmarks();                                                   /* a charge's lane, a spew's cone (C4) */
    drawn_fire_tick(dt);                                           /* a drawn level's burning floor (docs/PLAN-tutorial.md) */
    drawn_fire_dmarks();
    ice_tick(dt);                                                  /* and level 2's ice: the dream crosses it if it must */
    for (int i = 0; i < IC.n && g_drawn == 2 && IC.live; i++) hz_dput(IC.cx[i], IC.cy[i], HZ_SOFT);
    int px = (int)G.px, py = (int)G.py, mhp = G.max_hp > 0 ? G.max_hp : 100;
    for (int i = 0; i < MAXHAZ; i++) {
        Hazard *h = &G.haz[i];
        if (!h->alive) continue;
        if (h->pre > 0.0f) {                                       /* Premonition: shown, and the autopilot keeps out, before it warns */
            h->pre -= dt;
            for (int y = imax(1, (int)(h->y - h->r - 2.0f)); y <= imin(MAPN - 2, (int)(h->y + h->r + 2.0f)); y++)
                for (int x = imax(1, (int)(h->x - h->r - 2.0f)); x <= imin(MAPN - 2, (int)(h->x + h->r + 2.0f)); x++)
                    if (!hz_dmap[y][x] && hz_pre_covers(h, x, y)) hz_dmap[y][x] = 1;
            for (int k = 0; k < h->n; k++) if (!hz_dmap[h->cy[k]][h->cx[k]]) hz_dmap[h->cy[k]][h->cx[k]] = 1;
            continue;
        }
        float pt = h->t;
        h->t += dt;
        if (h->t >= h->warn + h->on + h->off) {                    /* done: an omen's grate is a wall again */
            if (h->wx >= 0) G.map[h->wy][h->wx].tex = (uint8_t)h->wtex;
            if (h->kind == HZ_TIMBER && h->hit == 2) hz_timber_close(h, 0);   /* the rubble cleared */
            h->alive = 0; continue;
        }
        int ph = hz_phase(h), struck = pt < h->warn && h->t >= h->warn;
        switch (h->kind) {
        case HZ_ROCK:
            if (struck) {
                sfx_at(SFX_BOOM, h->x, h->y, 0.9f); sfx_at(SFX_SHATTER, h->x, h->y, 0.5f);
                if (hypotf(G.px - h->x, G.py - h->y) < 4.0f) rumble(0.45f, 0.1f, 0.22f);   /* a rock lands near */
                for (int j = 0; j < MAXBOOMS; j++) if (!G.booms[j].alive) { memset(&G.booms[j], 0, sizeof G.booms[j]); G.booms[j].alive = 1; G.booms[j].x = h->x; G.booms[j].y = h->y; break; }
                if (!G.dead && hypotf(G.px - h->x, G.py - h->y) < h->r + BODY) { G.blame = (Blame){ BL_HAZARD, HZ_ROCK, 0 }; int d = hurt_ex(h->share * (float)mhp, HURT_STONE, h->x, h->y); if (d > 0) af_stroke(EL_PHYS, (float)d);
#ifdef CRAWL_DEBUG
                    fprintf(stderr, "EVENT hz_hit rock %d t=%.2f\n", d, G.t);
#endif
                    (void)d; }
            }
            break;
        case HZ_FIRE:
            if (struck) { sfx_at(SFX_IGNITE, h->x, h->y, 1.0f); sfx_at(SFX_EXPLODE, h->x, h->y, 0.5f); }
            if (ph == 1) for (int k = 0; k < h->n; k++) if (h->cx[k] == px && h->cy[k] == py) { G.hz_acc += h->share * (float)mhp * dt; break; }
            break;
        case HZ_BILE:
            if (ph < 2)
                for (int k = 0; k < h->n; k++)
                    if (h->cx[k] == px && h->cy[k] == py && hz_bile_wet(h, k) >= 1.0f && G.purify_t <= 0.0f) {   /* Purify keeps it off */
                        float r = 1.0f - gear_poison_resist(&RUN.gear), was = G.poison_t;
                        G.poison_t = fmaxf(G.poison_t, 2.5f * r); G.poison_by = (Blame){ BL_HAZARD, HZ_BILE, 0 };
                        af_poison(G.poison_t - was);
                        G.poison_dps = fmaxf(G.poison_dps, (1.0f + (float)imax(0, (G.level - 4) / 2)) * h->share * (0.5f + 0.5f * r));
                        G.hz_slow = fminf(G.hz_slow, 0.8f);
                        break;
                    }
            break;
        case HZ_VOID:
            if (struck) sfx_at(SFX_WAIL, h->x, h->y, 0.5f);
            if (ph == 1 && hypotf(G.px - h->x, G.py - h->y) < h->r) { G.hz_acc += h->share * (float)mhp * dt; G.hz_slow = fminf(G.hz_slow, 0.6f); G.silenced = 1; }
            break;
        case HZ_ARC:
            if (struck) { snd_sfx_ex(SFX_SWORD, 1.2f, 0.5f); if (!G.dead && hz_in_arc(h, G.px, G.py)) { G.blame = (Blame){ BL_HAZARD, HZ_ARC, 0 }; hurt_ex(h->share, HURT_BLOW, h->x, h->y); } }
            break;
        case HZ_BREATH: case HZ_DAMP: case HZ_SILT:                /* breathed: poison; a fire through the gas sends it up */
            if (h->kind != HZ_BREATH && h->src != -7 && ph == 1 && hz_fire_in(h)) { hz_burst(h); break; }   /* (a trial's gas only chokes) */
            if (ph == 1 && !G.dead && hz_in_cells(h, px, py)) hz_poison(h->kind, h->share);
            break;
        case HZ_VOTIVE: case HZ_FISSURE:                           /* burning wax, the fissure's breath */
            if (struck) { sfx_at(SFX_IGNITE, h->x, h->y, 0.8f); if (h->kind == HZ_FISSURE) sfx_at(SFX_EXPLODE, h->x, h->y, 0.4f); }
            if (ph == 1 && !G.dead && hz_in_cells(h, px, py)) hz_seep(h->kind, h->share, dt);
            break;
        case HZ_COLD:
            if (ph == 1 && !G.dead && hz_in_cells(h, px, py)) { if (af_off()) hz_chill(0.6f); else af_cold(dt); hz_seep(HZ_COLD, h->share, dt); }
            break;
        case HZ_SLUICE:                                            /* the surge runs down its cells, and throws you on */
            if (struck) sfx_at(SFX_SPLASH, h->x, h->y, 1.5f);
            if (ph == 1 && !h->hit && !G.dead) {
                int k = hz_in_cells(h, px, py);
                if (k && (float)h->cd[k - 1] <= (h->t - h->warn) * 6.0f + 0.5f) {
                    h->hit = 1;
                    if (hz_strike(HZ_SLUICE, h->share, h->x, h->y) > 0) { if (af_off()) G.slow_t = fmaxf(G.slow_t, 2.0f * (1.0f - gear_slow_resist(&RUN.gear))); hz_shove(cosf(h->ang), sinf(h->ang), 2.0f); }
                }
            }
            break;
        case HZ_GALV:                                              /* everyone in the water, at once */
            if (struck) {
                sfx_at(SFX_ZAP, h->x, h->y, 1.0f); sfx_at(SFX_CRACKLE, h->x, h->y, 1.0f);
                if (!G.dead && hz_in_cells(h, px, py)) hz_strike(HZ_GALV, h->share, h->x, h->y);
                for (int j = 0; j < MAXMOBS; j++) if (G.mobs[j].alive && hz_in_cells(h, (int)G.mobs[j].x, (int)G.mobs[j].y)) hz_hurt_mob(&G.mobs[j], h->share, EL_LIGHT, h->x, h->y);
                for (int c = 0; c < h->n; c += 3) add_beam(AT_CHAIN, h->cx[c] + 0.5f, h->cy[c] + 0.5f, 0.95f, h->cx[c] + 0.4f + 0.2f * rndf(), h->cy[c] + 0.4f + 0.2f * rndf(), 0.02f);
            }
            break;
        case HZ_TIMBER:                                            /* the roof comes down along the strip */
            if (struck) {
                sfx_at(SFX_BOOM, h->x, h->y, 1.0f); sfx_at(SFX_SHATTER, h->x, h->y, 0.6f);
                if (hypotf(G.px - h->x, G.py - h->y) < 5.0f) rumble(0.5f, 0.15f, 0.3f);
                if (!G.dead && hz_in_cells(h, px, py)) hz_strike(HZ_TIMBER, h->share, h->x, h->y);
                for (int j = 0; j < MAXMOBS; j++) if (G.mobs[j].alive && hz_in_cells(h, (int)G.mobs[j].x, (int)G.mobs[j].y)) hz_hurt_mob(&G.mobs[j], h->share, EL_PHYS, h->x, h->y);
                if (hz_may_close(h)) hz_timber_close(h, 1);
            }
            break;
        case HZ_CART:                                              /* down the rails, and whatever stands on them */
            if (ph == 1) {
                float u = cart_at(h);
                int k = imin(h->n - 1, (int)u), k2 = imin(h->n - 1, k + 1);
                float f = u - (float)k, cx = lerpf(h->cx[k], h->cx[k2], f) + 0.5f, cy = lerpf(h->cy[k], h->cy[k2], f) + 0.5f;
                if (!h->hit && !G.dead && hypotf(G.px - cx, G.py - cy) < 0.6f) {
                    h->hit = 1;
                    float side = (G.px - cx) * -sinf(h->ang) + (G.py - cy) * cosf(h->ang) >= 0.0f ? 1.0f : -1.0f;
                    if (hz_strike(HZ_CART, h->share, cx, cy) > 0) hz_shove(-sinf(h->ang) * side, cosf(h->ang) * side, 1.0f);
                }
                for (int j = 0; j < MAXMOBS && j < 64; j++)
                    if (G.mobs[j].alive && !(h->hitmask >> j & 1) && hypotf(G.mobs[j].x - cx, G.mobs[j].y - cy) < 0.6f) { h->hitmask |= 1ull << j; hz_hurt_mob(&G.mobs[j], h->share, EL_PHYS, cx, cy); }
            }
            break;
        case HZ_LODE: case HZ_CRYSTAL:                             /* arcs: the seam to all within reach, the crystal to the three nearest */
            if (struck) {
                float tx[4], ty[4], td[4];
                int who[4], nt = 0, most = h->kind == HZ_CRYSTAL ? 3 : 4;
                float z0 = h->kind == HZ_CRYSTAL ? 0.55f : 0.5f;
                sfx_at(SFX_ZAP, h->x, h->y, 1.0f);
                if (!G.dead && hypotf(G.px - h->x, G.py - h->y) < h->r && line_clear(h->x, h->y, G.px, G.py)) { tx[nt] = G.px; ty[nt] = G.py; td[nt] = hypotf(G.px - h->x, G.py - h->y); who[nt++] = -1; }
                for (int j = 0; j < MAXMOBS; j++) {
                    const Mob *m = &G.mobs[j];
                    float dm = hypotf(m->x - h->x, m->y - h->y);
                    if (!m->alive || m->state != MS_WALK || m->boss || m->ally || dm >= h->r || !line_clear(h->x, h->y, m->x, m->y)) continue;
                    int at = nt < most ? nt++ : most;
                    if (at == most) { int far = 0; for (int q = 1; q < nt; q++) if (td[q] > td[far]) far = q; if (td[far] <= dm) continue; at = far; }
                    tx[at] = m->x; ty[at] = m->y; td[at] = dm; who[at] = j;
                }
                for (int q = 0; q < nt; q++) {
                    add_beam(AT_CHAIN, h->x, h->y, z0, tx[q], ty[q], 0.45f);
                    if (who[q] < 0) hz_strike(h->kind, h->share, h->x, h->y);
                    else hz_hurt_mob(&G.mobs[who[q]], h->share, EL_LIGHT, h->x, h->y);
                }
            }
            break;
        case HZ_ASH:
            if (ph == 1 && !G.dead && hypotf(G.px - h->x, G.py - h->y) < h->r) hz_seep(HZ_ASH, h->share, dt);   /* you choke */
            break;
        case HZ_GLASS:
            if (struck) {
                sfx_at(SFX_SHATTER, h->x, h->y, 1.0f);
                if (!G.dead && hypotf(G.px - h->x, G.py - h->y) < h->r + BODY) hz_strike(HZ_GLASS, h->share, h->x, h->y);
                for (int j = 0; j < MAXMOBS; j++) if (G.mobs[j].alive && hypotf(G.mobs[j].x - h->x, G.mobs[j].y - h->y) < h->r) hz_hurt_mob(&G.mobs[j], h->share, EL_PHYS, h->x, h->y);
            }
            break;
        case HZ_SAINT: break;                                      /* her site drinks */
        case HZ_GRASP:
            if (struck) {
                sfx_at(SFX_DIRT, h->x, h->y, 0.8f);
                if (!G.dead && hypotf(G.px - h->x, G.py - h->y) < h->r + BODY) { G.blame = (Blame){ BL_HAZARD, HZ_GRASP, 0 }; hurt_ex(h->share * (float)mhp, HURT_BLOW, h->x, h->y); G.slow_t = fmaxf(G.slow_t, 1.0f); }
            }
            break;
        }
        hz_mobs(h, struck, ph, dt);
        int x0 = imax(1, (int)(h->x - h->r - 2.0f)), x1 = imin(MAPN - 2, (int)(h->x + h->r + 2.0f));
        int y0 = imax(1, (int)(h->y - h->r - 2.0f)), y1 = imin(MAPN - 2, (int)(h->y + h->r + 2.0f));
        if (hz_cellkind(h->kind))
            for (int k = 0; k < h->n; k++) hz_dput(h->cx[k], h->cy[k], hz_covers(h, h->cx[k], h->cy[k]));
        else
            for (int y = y0; y <= y1; y++) for (int x = x0; x <= x1; x++) hz_dput(x, y, hz_covers(h, x, y));
    }
    while (G.hz_acc >= 1.0f && !G.dead) {                          /* fire and void, a point at a time */
        G.hz_acc -= 1.0f;
#ifdef CRAWL_DEBUG
        static int burns; if ((++burns & 15) == 1) fprintf(stderr, "EVENT hz_hit %s x16 t=%.2f\n", G.silenced ? "void" : "fire", G.t);
#endif
        G.blame = (Blame){ BL_HAZARD, (uint8_t)(G.silenced ? HZ_VOID : HZ_FIRE), 0 };
        hurt_ex(1.0f, G.silenced ? HURT_VOID : HURT_SPELL, G.px, G.py);
    }
    for (int e = 0; e < NELEM; e++)                                /* the runs' own, by element (1.0.0b13) */
        while (G.hz_el_acc[e] >= 1.0f && !G.dead) { G.hz_el_acc[e] -= 1.0f; G.blame = (Blame){ BL_HAZARD, G.hz_el_by[e], 0 }; harm_next = e; hurt_ex(1.0f, HURT_POISON, G.px, G.py); }
    if (G.dead) { G.hz_acc = 0.0f; memset(G.hz_el_acc, 0, sizeof G.hz_el_acc); }
    if (G.silenced != was) snd_set_muffle(G.silenced);
    G.sil_k += ((float)G.silenced - G.sil_k) * fminf(1.0f, dt * 8.0f);
}
/* The caster's statuses (docs/PLAN-combat.md, section 4): their clocks, what ends as they run out (a hard control can't land
 * again for 3 s, nor any for 1 s; the ice breaks and takes the stacks with it), On Fire's harm a point at a time, and how far a
 * knock-down has the view down. The debug build: CRAWL_AFSHOW (a status's look, for its sheet; nothing else then) and CRAWL_AF
 * (0 none of them; fire, chill, frozen, stun, poison, unbound, down or all: that one put on the caster every ten seconds) */
static void af_tick(float dt) {
    int shown = 0;
#ifdef CRAWL_DEBUG
    static int show = -2, stacks = 0, put = -2;
    static double put_t;
    if (show == -2) {
        const char *e = getenv("CRAWL_AFSHOW");                    /* k (AF_*), k:stacks for Chilled, 7 the grapple, "all" the row */
        show = !e ? -1 : !strcmp(e, "all") ? 100 : clampi(atoi(e), 0, NAF);
        if (e && strchr(e, ':')) stacks = clampi(atoi(strchr(e, ':') + 1), 0, 3);
        const char *a = getenv("CRAWL_AF");
        put = -1;
        if (a && !strcmp(a, "all")) put = NAF;
        else if (a) for (int k = 0; k < NAF; k++) if (!strcmp(a, AF_NAME[k])) put = k;
    }
    if (show >= 0 && G.level_t > 2.0 && !G.dead) {
        float t = (float)fmod(G.level_t, 6.0);
        int grapple = show == NAF, all = show == 100;
        for (int k = 0; k < NAF; k++) {
            if (!all && k != show && !(grapple && k == AF_DOWN)) continue;
            if (all && (k == AF_FROZEN || k == AF_STUN || k == AF_DOWN)) continue;   /* the row's sheet: four at once */
            if (k == AF_DOWN && grapple) { G.af0[k] = 1.2f; G.af[k] = 1.2f - (float)fmod(G.level_t, 1.0); continue; }
            if (k == AF_DOWN) { float c = (float)fmod(G.level_t, 3.0); G.af0[k] = 1.5f; G.af[k] = c < 1.5f ? 1.5f - c : 0.0f; continue; }
            G.af0[k] = k == AF_POISON ? 42.0f : 6.0f; G.af[k] = G.af0[k] - (k == AF_POISON ? t * 2.0f : t);
        }
        G.af_n = show == AF_CHILL || all ? (stacks ? stacks : 2) : 0;
        G.af_shake = grapple ? 1.0f : 0.0f;
        shown = 1;
    }
    if (put >= 0 && !G.dead && G.level_t > 3.0 && G.level_t - put_t >= 10.0) {   /* one every ten seconds, on whatever stands there */
        int k = put == NAF ? (int)(G.level_t / 10.0) % NAF : put;
        put_t = G.level_t;
        G.blame = (Blame){ BL_HAZARD, 0, 0 };
        if (k == AF_FROZEN) { if (af_can(AF_CHILL)) af_chill(4, 0); }
        else if (af_can(k)) af_put(k, 0.15f * (float)G.max_hp, 0);
    }
    if (G.level_t < put_t) put_t = 0.0;                            /* a new level */
#endif
    if (G.af[AF_FIRE] > 0.0f && !shown) {                          /* On Fire: what is left, spread over the time left */
        float part = G.af_burn * fminf(1.0f, dt / G.af[AF_FIRE]);
        G.af_burn -= part; G.af_acc += part;
        while (G.af_acc >= 1.0f && !G.dead) { G.af_acc -= 1.0f; G.blame = G.af_fire_by; harm_next = EL_FIRE; hurt_ex(1.0f, HURT_POISON, G.px, G.py); }
    }
    for (int k = 0; k < NAF; k++) {
        G.af_lock[k] = fmaxf(0.0f, G.af_lock[k] - dt);
        if (G.af[k] <= 0.0f) continue;
        if ((G.af[k] -= dt) > 0.0f) continue;
        G.af[k] = 0.0f;
        if (shown) continue;
        if (af_hardk(k)) { G.af_lock[k] = 3.0f; G.af_hardt = 1.0f; }   /* S1 */
        if (k == AF_FROZEN) {                                      /* the ice breaks, and the stacks go with it */
            if (G.af[AF_CHILL] > 0.0f) af_event(AF_CHILL, 0);
            G.af_n = 0; G.af[AF_CHILL] = 0.0f;
            snd_sfx_ex(SFX_AF_THAW, 0.8f, 0.5f); rumble(0.25f, 0.6f, 0.2f);
        }
        if (k == AF_FIRE) G.af_burn = G.af_acc = 0.0f;
        if (k == AF_DOWN) { G.af_by = 0; G.af_shake = 0.0f; }   /* up, or the grip let go */
        af_event(k, 0);
    }
    G.af_hardt = fmaxf(0.0f, G.af_hardt - dt);
    if (G.af[AF_CHILL] <= 0.0f) G.af_n = 0;
    float want = G.af[AF_DOWN] > 0.0f && G.af_shake <= 0.0f ? 1.0f : 0.0f;   /* down in a quarter second, up in four tenths; a grapple holds you up */
    G.af_k = want > G.af_k ? fminf(want, G.af_k + dt / 0.25f) : fmaxf(want, G.af_k - dt / 0.4f);
    if (G.dead) af_clear();
}
static int hz_danger(int x, int y) { return x >= 0 && y >= 0 && x < MAPN && y < MAPN && hz_dmap[y][x] < HZ_SOFT ? hz_dmap[y][x] : 0; }
static int hz_soft(int x, int y) { return x >= 0 && y >= 0 && x < MAPN && y < MAPN && hz_dmap[y][x] == HZ_SOFT; }   /* what is always there: walked round if it can be */
static void hz_clear(void) {                                       /* a new place: none of them come along */
    for (int i = 0; i < MAXHAZ; i++) if (G.haz[i].alive && G.haz[i].wx >= 0) G.map[G.haz[i].wy][G.haz[i].wx].tex = (uint8_t)G.haz[i].wtex;
    memset(G.haz, 0, sizeof G.haz); memset(hz_dmap, 0, sizeof hz_dmap);
    G.hz_acc = 0.0f; G.hz_slow = 1.0f;
    G.nsite = 0; G.snare_t = 0.0f; memset(G.hz_el_acc, 0, sizeof G.hz_el_acc);
    if (G.silenced) { G.silenced = 0; snd_set_muffle(0); }
}

/* a floor cell near the dweller, in sight, for an omen: from d0 to d1 off, ahead of you if it can be,
 * so that it is seen */
static int hz_near(float d0, float d1, int *ox, int *oy) {
    for (int tries = 0; tries < 60; tries++) {
        float a = tries < 40 ? G.ang + (rndf() - 0.5f) * 2.0f : rndf() * TAU_F, d = d0 + (d1 - d0) * rndf();
        int x = (int)floorf(G.px + cosf(a) * d), y = (int)floorf(G.py + sinf(a) * d);
        if (!hz_floor(x, y) || in_band(y) || vault_of_room(x, y) >= 0 || !line_clear(G.px, G.py, x + 0.5f, y + 0.5f)) continue;
        *ox = x; *oy = y;
        return 1;
    }
    return 0;
}
/* bile's patch: the floor within reach of a cell, nearest first */
static int hz_patch(int sx, int sy, int reach, int8_t *cx, int8_t *cy, uint8_t *cd, int max) {
    static int qx[MAPN * MAPN], qy[MAPN * MAPN], qd[MAPN * MAPN];
    static uint8_t seen[MAPN][MAPN];
    memset(seen, 0, sizeof seen);
    int qh = 0, qt = 0, n = 0;
    if (!hz_floor(sx, sy)) return 0;
    qx[qt] = sx; qy[qt] = sy; qd[qt++] = 0; seen[sy][sx] = 1;
    while (qh < qt && n < max) {
        int x = qx[qh], y = qy[qh], d = qd[qh++];
        cx[n] = (int8_t)x; cy[n] = (int8_t)y; cd[n] = (uint8_t)d; n++;
        if (d >= reach) continue;
        for (int k = 0; k < 4; k++) {
            int nx = x + DX[k], ny = y + DY[k];
            if (seen[ny][nx] || !hz_floor(nx, ny)) continue;
            seen[ny][nx] = 1; qx[qt] = nx; qy[qt] = ny; qd[qt++] = d + 1;
        }
    }
    return n;
}

