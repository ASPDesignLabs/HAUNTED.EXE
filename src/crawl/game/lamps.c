/* A part of scene_crawl.c (docs/PLAN-tidy.md): it is #included there in its place and never compiled on its own.
 * The soul-lamps: laid with the level, lit by souls, ward_tick. */

/* ---- the soul-lamps, laid with the level and lit by souls (1.0.0b18.1, docs/PLAN-b18-1.md section 6) ---- */
static int ward_wraith(void) {                                     /* the Wraith on the level: 1 + its place, or 0 */
    for (int i = 0; i < MAXMOBS; i++) if (G.mobs[i].alive && G.mobs[i].type == MOB_WRAITH && !G.mobs[i].boss) return i + 1;
    return 0;
}
static int ward_level(void) {                                      /* a level the Wraith comes to */
    return !G.shop && !PK.depth && G.boss < 0 && !crawl_boss(G.level) && (G.level >= KINDS[MOB_WRAITH].debut || crawl_loop(G.level) > 0);
}
static float ward_due(void) {                                      /* seconds till it comes: 0 come, 1e9 not this level */
    if (ward_wraith()) return 0.0f;
    if (G.wraith_done || !ward_level()) return 1e9f;
    float at = 240.0f;
#ifdef CRAWL_DEBUG
    if (getenv("CRAWL_WRAITH")) at = (float)atof(getenv("CRAWL_WRAITH"));
#endif
    return fmaxf(0.0f, at - (float)G.level_t);
}
static void ward_spread(int bit, int x0, int y0, int reach) {      /* every open cell within reach steps of a place */
    static int16_t d[MAPN][MAPN];
    static int q[MAPN * MAPN];
    if (reach < 0 || x0 < 1 || y0 < 1 || x0 >= MAPN - 1 || y0 >= MAPN - 1) return;
    if (!is_open(x0, y0)) {                                        /* a thing in the wall: from the floor before it */
        for (int k = 0; k < 4; k++) if (is_open(x0 + DX[k], y0 + DY[k])) ward_spread(bit, x0 + DX[k], y0 + DY[k], reach - 1);
        return;
    }
    memset(d, 0xff, sizeof d);
    int qh = 0, qt = 0;
    d[y0][x0] = 0; q[qt++] = y0 * MAPN + x0;
    while (qh < qt) {
        int x = q[qh] % MAPN, y = q[qh] / MAPN; qh++;
        WD.cell[y][x] |= (uint8_t)bit;
        if (d[y][x] >= reach) continue;
        for (int k = 0; k < 4; k++) {
            int nx = x + DX[k], ny = y + DY[k];
            if (nx < 1 || ny < 1 || nx >= MAPN - 1 || ny >= MAPN - 1 || d[ny][nx] >= 0 || !is_open(nx, ny)) continue;
            d[ny][nx] = (int16_t)(d[y][x] + 1); q[qt++] = ny * MAPN + nx;
        }
    }
}
static int ward_area_n(int bit) { int n = 0; for (int y = 0; y < MAPN; y++) for (int x = 0; x < MAPN; x++) n += (WD.cell[y][x] & bit) != 0; return n; }
static void ward_ring(int bit) {                                   /* the cells about it, corners too, through floor and doorways */
    static uint8_t add[MAPN][MAPN];
    memset(add, 0, sizeof add);
    for (int y = 1; y < MAPN - 1; y++)
        for (int x = 1; x < MAPN - 1; x++) {
            if (!(WD.cell[y][x] & bit)) continue;
            for (int j = -1; j <= 1; j++) for (int i = -1; i <= 1; i++) if (is_open(x + i, y + j) && !(WD.cell[y + j][x + i] & bit)) add[y + j][x + i] = 1;
        }
    for (int y = 0; y < MAPN; y++) for (int x = 0; x < MAPN; x++) if (add[y][x]) WD.cell[y][x] |= (uint8_t)bit;
}
/* its lamp, hung on a wall near a place: want steps from it if it can */
static int ward_hang(int kind, int ax, int ay, int want) {
    static int16_t d[MAPN][MAPN];
    static int q[MAPN * MAPN];
    if (WD.n >= MAXWL || G.ndecos >= MAXDECO || ax < 1 || ay < 1 || ax >= MAPN - 1 || ay >= MAPN - 1) return 0;
    memset(d, 0xff, sizeof d);
    int qh = 0, qt = 0, bx = -1, by = -1, bk = 0;
    float bs = 1e9f;
    d[ay][ax] = 0; q[qt++] = ay * MAPN + ax;
    while (qh < qt) {
        int x = q[qh] % MAPN, y = q[qh] / MAPN; qh++;
        if (cell(x, y)->kind == K_FLOOR && !in_band(y) && !in_trial(x, y) && !in_wing(x, y))   /* never shut away behind the door it guards */
            for (int k = 0; k < 4; k++) {
                const Cell *w = cell(x + DX[k], y + DY[k]);
                if (w->kind != K_WALL || w->secret || w->ftorch || w->fover[(k + 2) & 3]) continue;   /* bare stone, not a secret's */
                float px = x + 0.5f + DX[k] * 0.40f, py = y + 0.5f + DY[k] * 0.40f, s = (float)(abs(d[y][x] - want) * 4 + d[y][x]) + k * 0.01f;
                int clash = 0;
                for (int i = 0; i < G.ndecos && !clash; i++) clash = G.decos[i].spr >= 0 && hypotf(G.decos[i].x - px, G.decos[i].y - py) < 0.7f;
                for (int i = 0; i < WD.n && !clash; i++) clash = hypotf(G.decos[WD.l[i].deco].x - px, G.decos[WD.l[i].deco].y - py) < 1.5f;
                if (!clash && s < bs) { bs = s; bx = x; by = y; bk = k; }
            }
        if (d[y][x] >= 4) continue;
        for (int k = 0; k < 4; k++) {
            int nx = x + DX[k], ny = y + DY[k];
            if (nx < 1 || ny < 1 || nx >= MAPN - 1 || ny >= MAPN - 1 || d[ny][nx] >= 0 || !is_open(nx, ny)) continue;
            d[ny][nx] = (int16_t)(d[y][x] + 1); q[qt++] = ny * MAPN + nx;
        }
    }
    if (bx < 0) return 0;
    WLamp *L = &WD.l[WD.n++];
    memset(L, 0, sizeof *L);
    L->kind = kind; L->cx = bx; L->cy = by; L->d = bk; L->deco = G.ndecos;
    G.decos[G.ndecos++] = (Deco){ bx + 0.5f + DX[bk] * 0.40f, by + 0.5f + DY[bk] * 0.40f, 0.0f, SP_WLAMP0 + 3 * (crawl_set(G.level) & 3), 1, 0 };   /* no draw on the dice */
    return 1;
}
static void ward_lights(void) {                                    /* a lit lamp's light, for the lightmap */
    for (int i = 0; i < WD.n && !G.shop && !PK.depth; i++)
        if (WD.l[i].banked > 0.0f) light_dynamic(G.decos[WD.l[i].deco].x, G.decos[WD.l[i].deco].y, 0.5f + 0.06f * sinf((float)G.t * 5.0f + (float)i), 2.6f);
}
static int seq_kind(int k) { return k == MKK_TABLET || k == MKK_PLINTH || k == MKK_RELIQ || k == MKK_WSTONE || k == MKK_RELAY; }
static int seq_key(int i) { return G.mk[i].grp ? G.mk[i].grp : i + 1; }
/* each challenge's ground, a ring round it and its lamp, once the level is made; nothing yet lit */
static void ward_make(void) {
    memset(&WD, 0, sizeof WD);
#ifdef CRAWL_DEBUG
    if (getenv("CRAWL_LAMPS") && !atoi(getenv("CRAWL_LAMPS"))) return;   /* (CRAWL_WARD is the warded creatures') */
#endif
    if (!ward_level()) return;                                     /* the Wraith never comes here: no lamps to light */
    for (int c = 0; c < 2 + MAXMK && WD.n < MAXWL; c++) {
        int bit = 1 << WD.n, kind = -1, ax = -1, ay = -1, want = 1;
        if (c == 0 && G.tri >= 0 && G.tri_room >= 0 && G.tri_wx >= 0) {   /* a trial: its room; the lamp beside its lights */
            for (int y = 0; y < MAPN; y++) for (int x = 0; x < MAPN; x++) if (in_trial(x, y) && is_open(x, y)) WD.cell[y][x] |= (uint8_t)bit;
            kind = WLK_TRIAL; ax = G.tri_wx; ay = G.tri_wy; want = 2;
        } else if (c == 1 && G.pz >= 0 && G.pz_room >= 0 && G.pz_door >= 0 && G.pz_wx >= 0) {   /* a puzzle: its wing, the ground its pieces lie on */
            for (int y = 0; y < MAPN; y++) for (int x = 0; x < MAPN; x++) if (in_wing(x, y) && is_open(x, y)) WD.cell[y][x] |= (uint8_t)bit;
            ward_spread(bit, G.doors[G.pz_door].x, G.doors[G.pz_door].y, 2);
            for (int i = 0; i < G.nthing; i++) if (!G.thing[i].tri) ward_spread(bit, G.thing[i].cx, G.thing[i].cy, 2);
            kind = WLK_PUZZLE; ax = G.pz_wx; ay = G.pz_wy; want = 1;
        } else if (c >= 2 && c - 2 < G.nmk && seq_kind(G.mk[c - 2].kind)) {   /* a sequence: about its pieces, the lamp by its first */
            int i = c - 2, key = seq_key(i), seen = 0;
            for (int j = 0; j < i; j++) seen |= seq_kind(G.mk[j].kind) && seq_key(j) == key;
            if (seen) continue;
            for (int j = 0; j < G.nmk; j++) if (seq_key(j) == key) ward_spread(bit, (int)G.mk[j].x, (int)G.mk[j].y, 2);
            kind = WLK_SEQ; ax = (int)G.mk[i].x; ay = (int)G.mk[i].y; want = 1;
            if (!is_open(ax, ay)) for (int k = 0; k < 4; k++) if (is_open((int)G.mk[i].x + DX[k], (int)G.mk[i].y + DY[k])) { ax = (int)G.mk[i].x + DX[k]; ay = (int)G.mk[i].y + DY[k]; break; }
        }
        if (kind < 0) continue;
        ward_ring(bit);
        if (!ward_area_n(bit) || !ward_hang(kind, ax, ay, want))   /* nowhere to hang it: no ward there */
            for (int y = 0; y < MAPN; y++) for (int x = 0; x < MAPN; x++) WD.cell[y][x] &= (uint8_t)~bit;
    }
#ifdef CRAWL_DEBUG
    static const char *const WK[3] = { "trial", "puzzle", "sequence" };
    for (int i = 0; i < WD.n; i++)
        fprintf(stderr, "WARD level %d: lamp %d for the %s at (%d,%d) on the wall %d, %d cells\n", G.level, i, WK[WD.l[i].kind], WD.l[i].cx, WD.l[i].cy, WD.l[i].d, ward_area_n(1 << i));
    if (getenv("CRAWL_WARDMAP"))                                   /* the ward's cells by lamp, the lamps, the things */
        for (int y = 0; y < MAPN; y++) {
            char row[MAPN + 1];
            for (int x = 0; x < MAPN; x++) {
                int b = WD.cell[y][x], c = cell(x, y)->kind;
                row[x] = c == K_WALL ? '#' : c == K_DOOR ? '+' : b ? (char)('0' + __builtin_ctz((unsigned)b)) : '.';
                for (int i = 0; i < WD.n; i++) if (WD.l[i].cx == x && WD.l[i].cy == y) row[x] = 'L';
                for (int i = 0; i < G.nthing; i++) if (G.thing[i].cx == x && G.thing[i].cy == y) row[x] = 'T';
                for (int i = 0; i < G.nmk; i++) if ((int)G.mk[i].x == x && (int)G.mk[i].y == y) row[x] = 'M';
            }
            row[MAPN] = 0;
            fprintf(stderr, "WARDMAP %2d %s\n", y, row);
        }
    if (getenv("CRAWL_WARDLIT"))                                   /* testing: every lamp lit with n souls, none paid */
        for (int i = 0; i < WD.n; i++) { WD.l[i].banked = WARD_SOUL_T * (float)clampi(atoi(getenv("CRAWL_WARDLIT")), 1, WARD_MAX); WD.mask |= 1 << i; G.decos[WD.l[i].deco].spr++; G.decos[WD.l[i].deco].nfr = 2; }
#endif
}
static void ward_light(int i, int on) {                            /* its flame, and the bit that bars the Wraith */
    Deco *dc = &G.decos[WD.l[i].deco];
    int base = SP_WLAMP0 + 3 * (crawl_set(G.level) & 3);
    dc->spr = on ? base + 1 : base; dc->nfr = on ? 2 : 1;
    if (on) WD.mask |= 1 << i; else WD.mask &= ~(1 << i);
    WD.field_t = 0.0f;
}
static int ward_offer(int i, int dream) {                          /* a soul for two minutes of it */
    WLamp *L = &WD.l[i];
    const Deco *dc = &G.decos[L->deco];
    if (L->banked > (float)(WARD_MAX - 1) * WARD_SOUL_T + 0.5f) { if (!dream) { say("THE LAMP HOLDS NO MORE"); snd_sfx_ex(SFX_KNOCK, 0.35f, 0.5f); } return 0; }
    if (spirit() < 1.0f || !spend_spirit(1.0f)) { if (!dream) { say("THE LAMP WANTS A WHOLE SOUL"); snd_sfx_ex(SFX_KNOCK, 0.35f, 0.5f); } return 0; }
    int was = L->banked > 0.0f;
    L->banked += WARD_SOUL_T;
    if (dream) L->offered++;
    if (!was) ward_light(i, 1);
    sfx_at(SFX_IGNITE, dc->x, dc->y, 0.7f);
    if (!dream) {
        int min = (int)ceilf(L->banked / 60.0f - 1e-3f);
        if (ward_wraith()) say("THE WARD HOLDS: %d MINUTES", min); else say("%d MINUTES, FROM THE WRAITH'S COMING", min);
    }
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT ward_offer lamp %d %s banked %.0f s spirit %.1f t=%.2f\n", i, dream ? "dream" : "person", L->banked, spirit(), G.t);
    if (!was) fprintf(stderr, "EVENT ward_lit lamp %d t=%.2f\n", i, G.t);
#endif
    return 1;
}
static int ward_facing(void) {                                     /* the lamp a person stands before and looks at */
    int best = -1; float ba = 0.6f;
    for (int i = 0; i < WD.n; i++) {
        const Deco *dc = &G.decos[WD.l[i].deco];
        if (hypotf(dc->x - G.px, dc->y - G.py) > 1.7f) continue;
        float a = fabsf(wrap_pi(atan2f(dc->y - G.py, dc->x - G.px) - G.ang));
        if (a < ba) { ba = a; best = i; }
    }
    return best;
}
static int ward_undone(int i) {                                    /* its challenge still waits to be met */
    const WLamp *L = &WD.l[i];
    if (L->kind == WLK_TRIAL) return G.tri >= 0 && G.tri_state == TRS_SHUT;
    if (L->kind == WLK_PUZZLE) return G.pz >= 0 && !G.pz_solved;
    for (int j = 0; j < G.nmk; j++) if (seq_kind(G.mk[j].kind) && (WD.cell[(int)G.mk[j].y][(int)G.mk[j].x] >> i & 1) && !G.mk[j].done) return 1;
    for (int j = 0; j < G.nmk; j++)                                /* a piece in the wall: by the floor before it */
        if (seq_kind(G.mk[j].kind) && !G.mk[j].done && hypotf(G.mk[j].x - (L->cx + 0.5f), G.mk[j].y - (L->cy + 0.5f)) < 2.5f) return 1;
    return 0;
}
/* the clock, the field, a person's offering, the dream's, the Wraith held at the edge */
static void ward_tick(float dt) {
    if (PK.depth || G.shop || !WD.n || dt <= 0.0f) return;
    int wi = ward_wraith();
    for (int i = 0; i < WD.n; i++) {                               /* the clock burns only while the Wraith walks the level */
        WLamp *L = &WD.l[i];
        if (L->banked <= 0.0f || !wi) continue;
        if ((L->banked -= dt) > 0.0f) continue;
        L->banked = 0.0f;
        ward_light(i, 0);
        sfx_at(SFX_WIND, G.decos[L->deco].x, G.decos[L->deco].y, 0.6f);
        if (G.player) say("A SOUL-LAMP GOES OUT");
#ifdef CRAWL_DEBUG
        fprintf(stderr, "EVENT ward_out lamp %d t=%.2f\n", i, G.t);
#endif
    }
    if (WD.mask && (WD.field_t -= dt) <= 0.0f) { ward_field(); WD.field_t = 0.5f; }
    WD.flare_t = fmaxf(0.0f, WD.flare_t - dt / 1.2f);
    int held = 0;
    if (wi && WD.mask && !G.dead) {                                /* held at the edge: a hiss, the line flaring where it stands */
        Mob *m = &G.mobs[wi - 1];
        int hx = -1, hy = -1;
        for (int k = 0; k < 4 && hx < 0; k++) if (wraith_barred(m->cx + DX[k], m->cy + DY[k])) { hx = m->cx + DX[k]; hy = m->cy + DY[k]; }
        held = hx >= 0 && m->sub != 1 && !wraith_barred(m->cx, m->cy) && hypotf(m->x - G.px, m->y - G.py) < 8.0f;
        if (held && !WD.at_edge) {
            WD.hiss_t = 0.0f;
            if (G.player && !WD.said_held) { WD.said_held = 1; say("THE WRAITH CANNOT PASS THE WARD"); }
#ifdef CRAWL_DEBUG
            fprintf(stderr, "EVENT ward_held at (%d,%d) t=%.2f\n", m->cx, m->cy, G.t);
#endif
        }
        if (held && (WD.hiss_t -= dt) <= 0.0f) {
            WD.hiss_t = 3.0f;
            sfx_at(SFX_HISS, m->x, m->y, 0.45f);
            WD.flare_t = 1.0f; WD.flare_x = (m->cx + hx) * 0.5f + 0.5f; WD.flare_y = (m->cy + hy) * 0.5f + 0.5f;
        }
    }
    WD.at_edge = held;
#ifdef CRAWL_DEBUG
    if (wi) {                                                      /* a check: it never walks into a lit ward from outside */
        static int lx = -1, ly = -1, lw = -1;
        const Mob *m = &G.mobs[wi - 1];
        if (lw == wi && (lx != m->cx || ly != m->cy) && lx >= 0 && !wraith_barred(lx, ly) && wraith_barred(m->cx, m->cy))
            fprintf(stderr, "EVENT ward_breach (%d,%d) to (%d,%d) t=%.2f\n", lx, ly, m->cx, m->cy, G.t);
        lx = m->cx; ly = m->cy; lw = wi;
        static double tr_t;
        if (getenv("CRAWL_WARDTRACE") && G.t - tr_t >= 1.0) {
            tr_t = G.t;
            fprintf(stderr, "WARD trace t=%.1f wraith (%.2f,%.2f) sub %d field %d, you (%.2f,%.2f) ang %.2f inside %d, apart %.2f, mask %d banked %.0f hp %d/%d\n", G.t, m->x, m->y,
                    m->sub, wraith_h(m->cx, m->cy), G.px, G.py, G.ang, wraith_barred((int)G.px, (int)G.py), hypotf(m->x - G.px, m->y - G.py), WD.mask, WD.n ? WD.l[0].banked : 0.0f, G.hp, G.max_hp);
        }
    }
#endif
    if (G.player && !G.dead) {
        if (IN.hit[KEY_E] && thing_facing() < 0) { int i = ward_facing(); if (i >= 0) { IN.hit[KEY_E] = 0; ward_offer(i, 0); } }   /* E at it: a soul (a puzzle's piece first) */
        for (int i = 0; i < WD.n && G.msg_t <= 0.4f; i++) {        /* its words, once a level, when first seen near */
            WLamp *L = &WD.l[i];
            const Deco *dc = &G.decos[L->deco];
            if (L->said || hypotf(dc->x - G.px, dc->y - G.py) > 3.0f || fabsf(wrap_pi(atan2f(dc->y - G.py, dc->x - G.px) - G.ang)) > 0.8f
                || !line_clear(G.px, G.py, L->cx + 0.5f, L->cy + 0.5f)) continue;
            L->said = 1;
            say("%s", pkp("A SOUL-LAMP   E: OFFER A SOUL"));
        }
        return;
    }
    if (G.dead || (WD.offer_t -= dt) > 0.0f) return;               /* the dream (W3 a): what the challenge wants, as it comes to it */
    WD.offer_t = 0.5f;
    float due = ward_due();
    int ccx = (int)G.px, ccy = (int)G.py;
    for (int i = 0; i < WD.n; i++) {
        WLamp *L = &WD.l[i];
        const Deco *dc = &G.decos[L->deco];
        if (!ward_undone(i)) continue;
        int inside = WD.cell[ccy][ccx] >> i & 1, near = hypotf(dc->x - G.px, dc->y - G.py) < 2.5f && line_clear(G.px, G.py, L->cx + 0.5f, L->cy + 0.5f);
        int coming = L->kind == WLK_TRIAL ? near : inside || near;
        if (L->banked <= 0.0f && !L->offered && coming && due < 60.0f && spirit() >= 15.0f) {
            int want = L->kind == WLK_TRIAL ? 1 : 2;
            for (int k = 0; k < want && spirit() >= 11.0f; k++) ward_offer(i, 1);   /* never under ten */
        } else if (wi && inside && L->banked > 0.0f && L->banked < 60.0f && L->offered < WARD_MAX && spirit() >= 11.0f)
            ward_offer(i, 1);                                      /* and one more as it burns low */
    }
}
