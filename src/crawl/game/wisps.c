/* A part of scene_crawl.c (docs/PLAN-tidy.md): it is #included there in its place and never compiled on its own.
 * The map's noting (map_note), items and the tally, the hand raised, wisps (targets, routes, summon_wisp, update_wisps), the L list's choices, the altar's rows, undead turned. */

/* ---- the map (docs/PLAN-b9.md, section 1): bought from the merchant, kept through death ---- */
#define MAP_PRICE 1000
#define MAP_X 10.75f
#define MAP_Y 14.1f                         /* where it floats in her room, at her left */
#define MAP_REACH 1.5f                      /* what lies this near the caster fills in, through walls (3.2 until 1.0.0b11) */
#define MAP_SIGHT 16.0f                     /* and what is seen, out to where the dark closes */
static void map_carto(void) {                                      /* the Cartographer: the shape of it all, known at once */
    if (RUN.corner >> CS_CARTO & 1) for (int y = 0; y < MAPN; y++) for (int x = 0; x < MAPN; x++) G.mapped[y][x] |= 1;
}
static void map_note(void) {                                       /* its shape known, been near it, and walked */
    int cx = (int)G.px, cy = (int)G.py;
    for (int y = cy - 2; y <= cy + 2; y++)
        for (int x = cx - 2; x <= cx + 2; x++)
            if (x >= 0 && y >= 0 && x < MAPN && y < MAPN && hypotf(x + 0.5f - G.px, y + 0.5f - G.py) <= MAP_REACH) G.mapped[y][x] |= 3;
    for (int k = 0; k < 4; k++) {                                  /* every cell the body passes through */
        int x = (int)(G.px + (k & 1 ? BODY : -BODY)), y = (int)(G.py + (k & 2 ? BODY : -BODY));
        if (x >= 0 && y >= 0 && x < MAPN && y < MAPN && G.map[y][x].kind != K_WALL) G.mapped[y][x] |= 8;
    }
    for (int r = 0; r < 64; r++) {                                 /* and what is seen: the cells the view's rays cross */
        float cam = 2.0f * ((float)r + 0.5f) / 64.0f - 1.0f, dx = cosf(G.ang), dy = sinf(G.ang);
        float rdx = dx - dy * PLANE_LEN * cam, rdy = dy + dx * PLANE_LEN * cam, l = hypotf(rdx, rdy);
        rdx /= l; rdy /= l;
        int mx = (int)G.px, my = (int)G.py, sx = rdx < 0 ? -1 : 1, sy = rdy < 0 ? -1 : 1;
        float ddx = rdx == 0.0f ? 1e30f : fabsf(1.0f / rdx), ddy = rdy == 0.0f ? 1e30f : fabsf(1.0f / rdy);
        float sdx = rdx < 0 ? (G.px - mx) * ddx : (mx + 1.0f - G.px) * ddx, sdy = rdy < 0 ? (G.py - my) * ddy : (my + 1.0f - G.py) * ddy;
        while (fminf(sdx, sdy) < MAP_SIGHT) {
            if (sdx < sdy) { sdx += ddx; mx += sx; } else { sdy += ddy; my += sy; }
            if (mx < 0 || my < 0 || mx >= MAPN || my >= MAPN) break;
            const Cell *c = &G.map[my][mx];
            if (c->kind == K_WALL || (G.slide >= 0 && slide_covers(mx, my))) break;   /* a shut secret still looks like stone */
            G.mapped[my][mx] |= 5;
            if (c->kind == K_DOOR && c->door >= 0 && G.doors[c->door].open < 0.9f) break;   /* a shut door stops sight */
        }
    }
}

static void update_items(void) {
    for (int i = 0; i < G.nitems; i++) {
        Item *it = &G.items[i];
        if (!it->alive) continue;
        float d = hypotf(it->x - G.px, it->y - G.py);
        if (!it->seen && d < (it->type == IT_KEY ? 13.0f : 8.5f) && line_clear(G.px, G.py, it->x, it->y)) it->seen = 1;
        if (it->type == IT_BONE && RUN.gear.bones >= bones_max()) continue;   /* no room for it */
        if (it->type == IT_POTION && it->color == TRI_REWARD && G.hp >= G.max_hp) {   /* the trial's draught waits while you are whole */
            if (G.player && !it->value && d < 0.9f) { it->value = 1; say("THE DRAUGHT WAITS: YOU ARE WHOLE"); }   /* (value: said, once) */
            continue;
        }
        if (d < (G.player ? 0.5f : 1.6f) && G.level_t > 1.0 && !G.ending) collect(it);   /* the autopilot reaches out, to a corner */
    }
}

/* the tally counts up row by row: time, kills, treasure, souls, secrets */
#define TALLY_ROWS 7
static const float TALLY_ROW0 = 1.6f, TALLY_ROW_GAP = 1.0f, TALLY_COUNT = 0.75f, TALLY_END = 9.0f;
static int tally_rows(void) { return G.boss >= 0 && G.bstate == BS_DEAD ? 7 : 6; }   /* a boss's level: how long its fight took */
static int tally_value(int row, float t, int final) {
    float k = smoothstepf(0.0f, 1.0f, (t - TALLY_ROW0 - row * TALLY_ROW_GAP) / TALLY_COUNT);
    return (int)floorf(final * k);
}
static void update_tally(double dt) {
    float pt = (float)G.tally_t;
    G.tally_t += dt;
    float t = (float)G.tally_t;
    int finals[TALLY_ROWS] = { (int)G.clear_time, G.kills, G.items_got, G.level_souls, G.links_found, G.level_xp, (int)G.bfight_t };
    for (int r = 0; r < tally_rows(); r++) {
        int v = tally_value(r, t, finals[r]);
        if (v != G.tally_shown[r]) { G.tally_shown[r] = v; snd_sfx_ex(SFX_TICK, 0.7f, 0.5f); }
        float done = TALLY_ROW0 + r * TALLY_ROW_GAP + TALLY_COUNT;
        if (pt < done && t >= done) snd_sfx_ex(r == 2 ? SFX_COIN : (r == 4 && G.nlinks && G.links_found == G.nlinks ? SFX_SECRET : SFX_ENTER), 1.0f, 0.5f);
    }
    float perfect = TALLY_ROW0 + tally_rows() * TALLY_ROW_GAP + 0.2f;
    if (pt < perfect && t >= perfect && G.items_total > 0 && G.items_got == G.items_total) snd_sfx_ex(SFX_KEYGET, 1.0f, 0.5f);
    if (t >= TALLY_END) {                                          /* on, or after a challenge back to the room it was begun from */
        if (G.bchallenge) G.level = RUN.chal_from;
        else if (G.replay && RUN.replay_from) { G.level = RUN.replay_from; RUN.replay_from = 0; }   /* a replay: back where it was begun */
        else G.level++;
        RUN.level = G.level; RUN.hp = G.hp; begin_shop();
    }
}

static void update_shop(double dt);

static void update_ui(void);
static void open_ui(int which);
static void update_human(float dt);
/* how far up the hand is: a cast, or a rune shown off */
static float hand_raise(void) {
    float lift = G.rune_lift > 0.0f ? 0.8f * smoothstepf(1.4f, 1.25f, G.rune_lift) * smoothstepf(0.0f, 0.3f, G.rune_lift) : 0.0f;
    if (G.af_shake > 0.0f && G.af[AF_DOWN] > 0.0f) lift = fmaxf(lift, 0.75f);   /* grappled: up against what holds you (PLAN-combat) */
    lift = fmaxf(lift, 0.72f * G.rst_k);                            /* Restoration channelled: up, glowing (D5) */
    return fmaxf(G.hand, lift);
}
/* the hand rises from the bottom right, reaching up and over, Catacomb 3-D / Doom style */
static void hand_origin(float hand, float *ox, float *oy) {
    float walk = G.player ? clampf(G.spd / WALK, 0.0f, 1.0f) : 0.0f;
    *ox = 170.0f + (int)(2.0f * sinf((float)G.t * 3.0f) + 5.0f * walk * sinf(G.bobph * 0.5f));
    *oy = H - (int)(hand * 130.0f) + (int)(2.0f * sinf((float)G.t * 6.0f) + 5.0f * walk * fabsf(sinf(G.bobph * 0.5f))) - (G.shop || G.player ? 40 : 0);
}

/* a key still lying in the level, or -1 */
static int key_lying(int color) {
    for (int i = 0; i < G.nitems; i++) if (G.items[i].alive && G.items[i].type == IT_KEY && G.items[i].color == color) return i;
    return -1;
}

/* a goal in a locked vault: first the key to it (and to the vault that key lies in) */
static void past_vaults(int *gx, int *gy, int k) {
    for (int depth = 0; depth < 4 && k >= 0 && G.vault_door[k] >= 0 && G.doors[G.vault_door[k]].locked == 1 && !G.has_key[k]; depth++) {
        int found = -1;
        for (int i = 0; i < G.nitems && found < 0; i++) if (G.items[i].alive && G.items[i].type == IT_KEY && G.items[i].color == k) found = i;
        if (found < 0) return;
        *gx = G.items[found].cx; *gy = G.items[found].cy;
        int v = vault_of_room(*gx, *gy);
        k = v != k ? v : -1;
    }
}

/* room for a body to walk straight there: past no wall, door, pillar or stone, nor a corner of one */
static int walk_clear(float x0, float y0, float x1, float y1) {
    float dx = x1 - x0, dy = y1 - y0, d = sqrtf(dx * dx + dy * dy);
    if (d < 0.01f) return 1;
    float nx = -dy / d * 0.3f, ny = dx / d * 0.3f;
    int n = (int)(d / 0.1f);
    for (int side = -1; side <= 1; side++)
        for (int i = 1; i <= n; i++) {
            float t = (float)i / n;
            Cell *c = cell((int)(x0 + dx * t + nx * side), (int)(y0 + dy * t + ny * side));
            if (c->kind == K_WALL || c->kind == K_PILLAR || c->kind == K_PROP || (c->kind == K_DOOR && G.doors[c->door].open < 0.7f)) return 0;
        }
    return 1;
}

static const char *const KEY_COLOR[NKEYS];
/* Will-o'-wisps (PLAN-b5). Summoned for a soul, or woken from a Black Altar, a wisp goes on ahead along
 * the way to what it seeks, in that thing's colour, and hovers there till you come. It leaves a trail of
 * motes hanging in the air, to follow round a corner. */
static const int KEY_RAMP[NKEYS] = { R_GOLD, R_BONE, R_BLOOD };
static int wisp_ramp_of(const Wisp *w) { return w->kind == WK_ALTAR ? R_PURPLE : w->kind == WK_MEND ? R_GOLD : (w->kind == WK_KEY ? KEY_RAMP[w->color] : R_GLOW); }

/* what a wisp of this kind seeks now: the cell, where it hovers there, and whether that is the end of
 * it (or a key it must fetch first); 0 if there is nothing left to seek */
static int wisp_target(int kind, int color, int *gx, int *gy, float *hx, float *hy, int *final) {
    int x, y;
    float px, py;
    if (kind == WK_EXIT && PK.depth) {                             /* in a pocket, the way out is the way back */
        int d = 0;
        for (; d < 4; d++) if (cell(PK.back_x + DX[d], PK.back_y + DY[d])->kind == K_FLOOR) break;
        if (d == 4) return 0;
        x = PK.back_x + DX[d]; y = PK.back_y + DY[d];
        px = x + 0.5f - DX[d] * 0.3f; py = y + 0.5f - DY[d] * 0.3f;   /* before the doorway */
        *gx = x; *gy = y;
    } else if (kind == WK_EXIT) {
        if (G.exit_fx < 0) return 0;
        x = G.exit_fx; y = G.exit_fy;
        px = x + 0.5f + DX[G.exit_dir] * 0.3f; py = y + 0.5f + DY[G.exit_dir] * 0.3f;   /* before the door */
        if (G.pz >= 0 && G.pz_exit && !G.pz_solved && G.pz_wx >= 0) { x = G.pz_wx; y = G.pz_wy; px = x + 0.5f; py = y + 0.5f; }   /* the wing's door, and it waits */
        *gx = x; *gy = y; past_vaults(gx, gy, G.exit_in_vault);
    } else if (kind == WK_KEY) {
        int i = key_lying(color);
        if (i < 0) return 0;
        x = G.items[i].cx; y = G.items[i].cy; px = G.items[i].x; py = G.items[i].y;   /* over the key */
        *gx = x; *gy = y; past_vaults(gx, gy, vault_of_room(x, y));
    } else if (kind == WK_MEND) {                                  /* the lessons': before the place to mend */
        if (G.nrest <= 0) return 0;
        x = G.rest_fx[0]; y = G.rest_fy[0];
        px = x + 0.5f + (G.rest_x[0] - x) * 0.3f; py = y + 0.5f + (G.rest_y[0] - y) * 0.3f;
        *gx = x; *gy = y;
    } else if (G.altar_wisp == AW_HOME || G.altar_wisp == AW_REST) {   /* home: over the altar */
        if (G.altar_x < 0) return 0;
        int d = 0;
        for (; d < 4; d++) if (is_open(G.altar_x + DX[d], G.altar_y + DY[d])) break;
        if (d == 4) return 0;
        x = G.altar_x + DX[d]; y = G.altar_y + DY[d];
        px = x + 0.5f - DX[d] * 0.45f; py = y + 0.5f - DY[d] * 0.45f;   /* at the altar's edge, over its fire */
        *gx = x; *gy = y; past_vaults(gx, gy, vault_of_room(x, y));
    } else {
        if (G.altar_state != ALTAR_AWAKE || G.altar_seek < 0) return 0;
        const Secret *sc = &G.secrets[G.altar_seek];
        x = sc->fx; y = sc->fy;
        px = x + 0.5f + DX[sc->d] * 0.3f; py = y + 0.5f + DY[sc->d] * 0.3f;   /* at its door, till you have been in and out */
        *gx = x; *gy = y; past_vaults(gx, gy, vault_of_room(x, y));
    }
    *final = *gx == x && *gy == y;
    *hx = *final ? px : *gx + 0.5f; *hy = *final ? py : *gy + 0.5f;
    return 1;
}

/* the way to it: open floor and doors, but not through a vault you have no key for */
static void wisp_route(Wisp *w) {
    static int q[MAPN * MAPN];
    memset(w->field, 0xff, sizeof w->field);
    if (w->gx < 0) return;
    int qh = 0, qt = 0;
    w->field[w->gy][w->gx] = 0; q[qt++] = w->gy * MAPN + w->gx;
    while (qh < qt) {
        int x = q[qh] % MAPN, y = q[qh] / MAPN; qh++;
        for (int d = 0; d < 4; d++) {
            int nx = x + DX[d], ny = y + DY[d];
            if (nx < 0 || ny < 0 || nx >= MAPN || ny >= MAPN || w->field[ny][nx] >= 0 || !is_open(nx, ny)) continue;
            const Cell *c = &G.map[ny][nx];
            if (c->kind == K_DOOR && ((G.doors[c->door].locked == 1 && !G.has_key[G.doors[c->door].color]) || G.doors[c->door].locked == SEALED || G.doors[c->door].locked == WINGED || G.doors[c->door].locked == GATED)) continue;
            w->field[ny][nx] = (int16_t)(w->field[y][x] + 1); q[qt++] = ny * MAPN + nx;
        }
    }
}

/* a wisp's line of flight: no wall or pillar in the way, nor a corner of one (doors it goes through) */
static int wisp_line(float x0, float y0, float x1, float y1) {
    float dx = x1 - x0, dy = y1 - y0, d = sqrtf(dx * dx + dy * dy);
    if (d < 0.01f) return 1;
    float nx = -dy / d * 0.15f, ny = dx / d * 0.15f;
    int n = (int)(d / 0.1f) + 1;
    for (int side = -1; side <= 1; side++)
        for (int i = 1; i <= n; i++) {
            float t = (float)i / n;
            int k = cell((int)(x0 + dx * t + nx * side), (int)(y0 + dy * t + ny * side))->kind;
            if (k == K_WALL || k == K_PILLAR) return 0;
        }
    return 1;
}

/* a wisp of this kind sets out: from the hand, or from the altar; a soul (cost) unless it is free */
static int summon_wisp(int kind, int color, float cost) {
    int gx, gy, fin;
    float hx, hy;
    if (!wisp_target(kind, color, &gx, &gy, &hx, &hy, &fin)) {
        say(kind == WK_KEY ? "THAT KEY LIES NOWHERE NOW" : "THERE IS NOTHING LEFT FOR IT TO FIND");
        return 0;
    }
    if (cost > 0.0f && !spend_spirit(cost)) { say("A WISP COSTS A SOUL"); snd_sfx_ex(SFX_KNOCK, 0.6f, 0.5f); return 0; }
    Wisp *w = &G.wisps[kind == WK_ALTAR ? 1 : 0];                  /* casting another sends the first out */
    memset(w, 0, sizeof *w);
    w->on = 1; w->kind = kind; w->color = color; w->state = WS_KINDLE; w->gx = -1;
    if (kind == WK_ALTAR) { w->x = G.altar_x + 0.5f; w->y = G.altar_y + 0.5f; w->z = 0.7f; }
    else { w->x = G.px + cosf(G.ang) * 0.3f; w->y = G.py + sinf(G.ang) * 0.3f; w->z = 0.4f; }
    if (!is_open((int)w->x, (int)w->y) && kind != WK_ALTAR) { w->x = G.px; w->y = G.py; }
    snd_sfx_ex(SFX_SHIMMER, 0.7f, 0.5f);
    if (kind == WK_EXIT) say("A WISP KINDLES. IT KNOWS THE WAY OUT");
    else if (kind == WK_MEND) say("A WISP KINDLES. IT KNOWS WHERE TO MEND");
    else if (kind == WK_KEY) say("A WISP GOES FOR THE %s KEY", KEY_COLOR[color]);
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT wisp %d %d t=%.2f\n", kind, color, G.t);
#endif
    return 1;
}

/* the L menu: what can be cast now, and what each costs */
typedef struct { int kind, color; float cost; } WispChoice;
#define WISP_CHOICES (2 + NKEYS)
/* the Lantern of the Dead, shuttered or lit (docs/PLAN-ai.md, 26): its light and the Wraith kept off go together */
static void lantern_toggle(void) {
    RUN.lantern_off ^= 1;
    say(RUN.lantern_off ? "THE LANTERN SHUTTERED" : "THE LANTERN LIT");
    if (RUN.lantern_off) snd_sfx_ex(SFX_KNOCK, 0.45f, 0.5f); else snd_sfx_ex(SFX_IGNITE, 0.6f, 0.5f);
    show_bar();
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT lantern %s t=%.2f\n", RUN.lantern_off ? "shuttered" : "lit", G.t);
#endif
}
static int wisp_choices(WispChoice *c) {
    int n = 0;
    c[n++] = (WispChoice){ WK_EXIT, 0, WISP_COST };
    if (gear_ability(&RUN.gear, AB_KEYMAKER, 0) > 0.0f)            /* Keymaker: a wisp to each key still lying */
        for (int k = 0; k < NKEYS; k++) if (key_lying(k) >= 0) c[n++] = (WispChoice){ WK_KEY, k, WISP_COST };
    if (G.altar_state == ALTAR_AWAKE && G.altar_wisp == AW_SEEK && G.altar_seek >= 0) c[n++] = (WispChoice){ WK_ALTAR, 0, 0.0f };   /* the altar's, back */
    return n;
}
static int wisp_choice_at(const WispChoice *c, int n, int kind, int color) {
    for (int i = 0; i < n; i++) if (c[i].kind == kind && (kind != WK_KEY || c[i].color == color)) return i;
    return -1;
}
static void cast_choice(const WispChoice *c) {
    if (summon_wisp(c->kind, c->color, c->cost)) { RUN.wisp_kind = c->kind; RUN.wisp_color = c->color; }
}

/* the altar's menu: its three gifts, what each costs, and why one can't be had */
typedef struct { const char *what, *cost, *note; int ok, price; } AltarRow;
static void altar_rows(AltarRow *r) {
    static char found[40];
    static char price[24];
    if (G.altar_state == ALTAR_DORMANT) r[0] = (AltarRow){ "WAKE ITS WISP", "10 SOULS", "IT LEADS YOU TO A SECRET", 1, ALTAR_SOULS };
    else if (G.altar_state == ALTAR_AWAKE && G.altar_wisp == AW_REST && nearest_shut(G.px, G.py) >= 0) {
        snprintf(price, sizeof price, "%d SOULS", altar_price());
        r[0] = (AltarRow){ "SEND ITS WISP ON", price, "TO THE NEAREST SECRET STILL SHUT", 1, altar_price() };
    }
    else if (G.altar_state == ALTAR_AWAKE && G.altar_wisp == AW_REST) r[0] = (AltarRow){ "ITS WISP RESTS", "", "NO SECRET WALL IS LEFT SHUT", 0, 0 };
    else if (G.altar_state == ALTAR_AWAKE) r[0] = (AltarRow){ "ITS WISP IS OUT", "", "IN, OUT, THEN E SENDS IT HOME", 0, 0 };
    else r[0] = (AltarRow){ "ITS WISP HAS DONE ITS WORK", "", "EVERY SECRET HERE IS FOUND", 0, 0 };
    r[1] = G.blessed ? (AltarRow){ "YOUR EXIT WISPS ARE BLESSED", "", "UNTIL YOU LEAVE THIS LEVEL", 0, 0 }
                     : (AltarRow){ "BLESS YOUR EXIT WISPS", "25 SOULS", "THEY TURN THE UNDEAD, THIS LEVEL", 1, ALTAR_BLESS };
    if (G.altar_state == ALTAR_READY) r[2] = (AltarRow){ "ANOINT WHAT YOU WEAR", "FREE", "ONE WORN PIECE GROWS STRONGER", 1, 0 };
    else if (G.altar_state == ALTAR_SPENT) r[2] = (AltarRow){ "IT HAS ANOINTED ALREADY", "", "IT HAS GIVEN WHAT IT HAD", 0, 0 };
    else {
        snprintf(found, sizeof found, "FIND EVERY SECRET: %d OF %d SO FAR", G.links_found, G.nlinks);
        r[2] = (AltarRow){ "ANOINT WHAT YOU WEAR", "", found, 0, 0 };
    }
}

static void wisp_out(Wisp *w) { w->state = WS_GONE; w->flare = 0.0f; sfx_at(SFX_SHIMMER, w->x, w->y, 0.6f); }

/* a blessed exit wisp: the undead about it flee from it, and go on fleeing for three seconds after it
 * has gone by; the Wraith is driven back */
static void turn_undead(const Wisp *w) {
    for (int i = 0; i < MAXMOBS; i++) {
        Mob *m = &G.mobs[i];
        if (!m->alive || m->state != MS_WALK || m->ally || !(KIND_IS(m, KF_UNDEAD) || m->type == MOB_WSKULL || m->type == MOB_WRAITH)) continue;
        if (hypotf(m->x - w->x, m->y - w->y) > BLESS_REACH || !line_clear(w->x, w->y, m->x, m->y)) continue;
        if (m->type == MOB_WRAITH) { if (m->sub != 1) wraith_back(m); continue; }
        if (m->feared <= 0.0f || !m->wfeared) { sfx_at(SFX_MOAN, m->x, m->y, 0.6f); DBG_COUNT(turned); }
        m->feared = fmaxf(m->feared, 3.0f); m->wfeared = 1; m->wfx = w->x; m->wfy = w->y;
    }
}

static void update_wisps(float dt) {
    for (int i = 0; i < MAXWISPS; i++) {
        Wisp *w = &G.wisps[i];
        if (!w->on || dt <= 0.0f) continue;
        w->t += dt;
        if (w->state == WS_GONE) {                                 /* it flares and goes out; its trail lingers a while */
            w->flare += dt;
            w->a = w->flare < 0.15f ? 1.0f + w->flare * 4.0f : fmaxf(0.0f, 1.6f * (1.0f - (w->flare - 0.15f) / 0.6f));
            if (w->flare > 0.75f + WISP_MOTE_T) w->on = 0;
            continue;
        }
        if ((w->retarget -= dt) <= 0.0f) {                         /* a key taken, a secret opened: where now */
            int gx, gy, fin;
            float hx, hy;
            w->retarget = 0.35f;
            if (!wisp_target(w->kind, w->color, &gx, &gy, &hx, &hy, &fin)) { wisp_out(w); continue; }
            if (gx != w->gx || gy != w->gy) { w->gx = gx; w->gy = gy; wisp_route(w); if (w->state == WS_HOVER) w->state = WS_FLY; }
            w->hx = hx; w->hy = hy; w->final = fin;
        }
        float ox = w->x, oy = w->y;
        if (w->state == WS_KINDLE) {                               /* a spark that grows, then goes */
            w->a = fminf(1.0f, w->a + dt / 0.45f);
            w->z += dt * 0.2f;
            if (w->a >= 1.0f) w->state = WS_FLY;
        } else if (w->state == WS_FLY) {
            int cx = (int)w->x, cy = (int)w->y;
            int f = cx >= 0 && cy >= 0 && cx < MAPN && cy < MAPN ? w->field[cy][cx] : -1;
            float tx = w->hx, ty = w->hy, pace = WISP_PACE;
            if (f > 0) {                                           /* down the way, as far along it as it can see */
                int x = cx, y = cy;
                for (int k = 0; k < 3 && w->field[y][x] > 0; k++) {
                    int bx = -1, by = -1;
                    float best = 1e9f;
                    for (int d = 0; d < 4; d++) {
                        int nx = x + DX[d], ny = y + DY[d];
                        if (nx < 0 || ny < 0 || nx >= MAPN || ny >= MAPN || w->field[ny][nx] != w->field[y][x] - 1) continue;
                        float e = hypotf((float)(nx - w->gx), (float)(ny - w->gy));   /* of equal ways, the straightest */
                        if (e < best) { best = e; bx = nx; by = ny; }
                    }
                    if (bx < 0 || (k > 0 && !wisp_line(w->x, w->y, bx + 0.5f, by + 0.5f))) break;
                    x = bx; y = by;
                }
                if (w->field[y][x] > 0) { tx = x + 0.5f; ty = y + 0.5f; }
            } else if (f < 0 && cx >= 0 && cy >= 0 && cx < MAPN && cy < MAPN) {   /* strayed off its way (over a prop): back onto it */
                int best = -1;
                for (int d = 0; d < 4; d++) {
                    int nx = cx + DX[d], ny = cy + DY[d];
                    if (nx < 0 || ny < 0 || nx >= MAPN || ny >= MAPN || w->field[ny][nx] < 0) continue;
                    if (best < 0 || w->field[ny][nx] < w->field[cy + DY[best]][cx + DX[best]]) best = d;
                }
                if (best >= 0) { tx = cx + DX[best] + 0.5f; ty = cy + DY[best] + 0.5f; }
            }
            if (f >= 0 && f < 2) pace *= 0.45f + 0.55f * fminf(1.0f, hypotf(w->hx - w->x, w->hy - w->y));   /* slowing as it comes there */
            float dx = tx - w->x, dy = ty - w->y, d = hypotf(dx, dy);
            if (f == 0 && d < 0.05f) w->state = WS_HOVER;
            else if (d > 1e-4f) { float st = fminf(d, pace * dt); w->x += dx / d * st; w->y += dy / d * st; }
            w->z += (0.5f + 0.05f * sinf(w->t * 2.1f) - w->z) * fminf(1.0f, dt * 3.0f);
        } else {                                                   /* there: it hovers, bobbing, till you come */
            w->x += (w->hx - w->x) * fminf(1.0f, dt * 4.0f); w->y += (w->hy - w->y) * fminf(1.0f, dt * 4.0f);
            w->z += (0.55f + 0.08f * sinf(w->t * 2.6f) - w->z) * fminf(1.0f, dt * 3.0f);
            if (w->final && w->kind != WK_ALTAR && hypotf(G.px - w->hx, G.py - w->hy) < 1.5f) { wisp_out(w); continue; }
        }
        w->vx = (w->x - ox) / dt; w->vy = (w->y - oy) / dt;
        if (w->kind == WK_EXIT && G.blessed && w->a > 0.5f) turn_undead(w);
        if ((w->mote_d += hypotf(w->x - ox, w->y - oy)) >= 0.22f) {   /* a mote of it left hanging */
            Mote *m = &w->motes[w->mote_head];
            w->mote_head = (w->mote_head + 1) % WISP_MOTES;
            m->x = w->x; m->y = w->y; m->z = w->z; m->t = (float)G.t;
            w->mote_d = 0.0f;
        }
    }
}

/* the way a wisp went, from where the caster stands, as far down it as there is a clear walk */
static int wisp_lean(const Wisp *w, float *out) {
    int x = (int)G.px, y = (int)G.py;
    if (!w->on || w->gx < 0 || x < 0 || y < 0 || x >= MAPN || y >= MAPN) return 0;
    float tx = w->hx, ty = w->hy;
    if (w->field[y][x] < 0) { *out = atan2f(ty - G.py, tx - G.px); return 1; }
    for (int k = 0; k < 7 && w->field[y][x] > 0; k++) {
        int bx = -1, by = -1;
        float best = 1e9f;
        for (int d = 0; d < 4; d++) {
            int nx = x + DX[d], ny = y + DY[d];
            if (nx < 0 || ny < 0 || nx >= MAPN || ny >= MAPN || w->field[ny][nx] != w->field[y][x] - 1) continue;
            float e = hypotf((float)(nx - w->gx), (float)(ny - w->gy));
            if (e < best) { best = e; bx = nx; by = ny; }
        }
        if (bx < 0 || (k > 0 && !walk_clear(G.px, G.py, bx + 0.5f, by + 0.5f))) break;
        x = bx; y = by; tx = x + 0.5f; ty = y + 0.5f;
    }
    *out = atan2f(ty - G.py, tx - G.px);
    return 1;
}

