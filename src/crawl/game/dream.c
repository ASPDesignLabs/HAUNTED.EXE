/* A part of scene_crawl.c (docs/PLAN-tidy.md): it is #included there in its place and never compiled on its own.
 * The dream (the autopilot): its counters, pick_goal, decide, its steps and turns, find_target, choose_spell, aim, update_player. */

/* ---- The dream's counters (docs/PLAN-ai.md, Part G; step 10). What the dead do from behind and round corners, the dream
 * answers: a glance back at a sound behind it (a hunt come up on its trail, a pack's call, a scavenger at its meal); an
 * ambush's sign read, its way in taken only if there is no other, and a spell sent through it first; callers and leaders
 * aimed at first (find_target); and hurt, with a pack on it, a corner it has walked, to fight from. Never a person's. ---- */
static void dream_glance(float x, float y) {
    if (G.player || G.dead || G.shop || G.tally || G.level_t - G.glance_last < 6.0) return;
    float vx = x - G.px, vy = y - G.py, d = hypotf(vx, vy);
    if (d > 12.0f || d < 0.5f || vx * cosf(G.ang) + vy * sinf(G.ang) > -0.2f * d) return;   /* ahead or beside: it sees to that */
    G.glance = 1; G.glance_x = x; G.glance_y = y;
}
static int amb_soft(int x, int y) {                                /* by the way into an ambush whose sign it has read */
    for (int k = 0; k < G.namb; k++) {
        const Ambush *a = &G.amb[k];
        if (!(G.amb_read >> k & 1) || a->sprung) continue;
        if ((abs(x - a->ox) <= 1 && abs(y - a->oy) <= 1) || (abs(x - a->ix) <= 1 && abs(y - a->iy) <= 1)) return 1;
    }
    return 0;
}
static void amb_read_tick(void) {                                  /* the sign by a door, seen within seven cells */
    for (int k = 0; k < G.namb && !G.player; k++) {
        const Ambush *a = &G.amb[k];
        if ((G.amb_read >> k & 1) || a->sprung) continue;
        if (hypotf(a->ox + 0.5f - G.px, a->oy + 0.5f - G.py) < 7.0f && line_clear(G.px, G.py, a->ox + 0.5f, a->oy + 0.5f)) { G.amb_read |= 1 << k; DBG_COUNT(amb_read); }
    }
}
static int pack_on_me(void) {                                      /* a pack hunting it, one of it within ten cells */
    for (int k = 0; k < G.npacks; k++) {
        const Pack *p = &G.packs[k];
        if (p->broken || p->state != PAS_HUNT) continue;
        for (int i = 0; i < MAXMOBS; i++) {
            const Mob *o = &G.mobs[i];
            if ((p->mem >> i & 1) && o->alive && o->state == MS_WALK && o->alert > 0.0f && hypotf(o->x - G.px, o->y - G.py) < 10.0f) return 1;
        }
    }
    return 0;
}
static int lane_corner(int x, int y) {                             /* a bend or a dead end in the lanes: half the ways in */
    int n = 0, m = 0;
    for (int d = 0; d < 4; d++) if (apass(x + DX[d], y + DY[d])) { n++; m |= 1 << d; }
    return n == 1 || (n == 2 && m != 5 && m != 10);
}
static int corner_goal(int x, int y, int *gx, int *gy) {           /* hurt, a pack on it: the nearest corner it has walked */
    if (!G.corner_on) {
        if (G.player || G.dead || G.shop || PK.depth || boss_fighting() || in_trial(G.cx, G.cy) || G.corner_cd > (float)G.level_t
            || G.hp >= G.max_hp / 2 || !pack_on_me()) return 0;
        bfs_from(x, y);
        int best = -1, bd = 9;
        for (int cy = 1; cy < MAPN - 1; cy++)
            for (int cx = 1; cx < MAPN - 1; cx++) {
                int d = bfs_dist[cy][cx];
                if (d < 0 || d >= bd || !apass(cx, cy) || !G.map[cy][cx].visits || !lane_corner(cx, cy) || hz_danger(cx, cy)) continue;
                bd = d; best = cy * MAPN + cx;
            }
        if (best < 0) { G.corner_cd = (float)G.level_t + 10.0f; return 0; }
        G.corner_on = 1; G.corner_x = best % MAPN; G.corner_y = best / MAPN; G.guard_t = 0.0f;
        DBG_COUNT(corners);
    }
    if (x == G.corner_x && y == G.corner_y) return 0;
    *gx = G.corner_x; *gy = G.corner_y;
    return 1;
}
static int corner_keep(void) {                                     /* it holds the corner while it is hurt and the pack is on it */
    return !G.dead && G.guard_t < 30.0f && G.hp < G.max_hp * 0.6f && !hz_danger(G.cx, G.cy) && pack_on_me();
}
static void corner_leave(void) {
    G.corner_on = 0; G.corner_cd = (float)G.level_t + 20.0f;
    G.next_h = decide(G.cx, G.cy, G.h);
    begin_turn(heading_angle(G.next_h)); G.state = PS_TURN;
}
static int guard_target(void) {                                    /* from the corner: the nearest that knows it is there */
    int best = -1; float bd = 9.0f;
    for (int i = 0; i < MAXMOBS; i++) {
        const Mob *m = &G.mobs[i];
        if (!targetable(m) || (m->alert <= 0.0f && !m->seen) || m->ignore > (float)G.t) continue;
        float d = hypotf(m->x - G.px, m->y - G.py);
        if (d < bd && line_clear(G.px, G.py, m->x, m->y) && shot_clear(m->x, m->y)) { bd = d; best = i; }
    }
    return best;
}
static int at_corner(void) {
    if (!G.corner_on || G.player || G.cx != G.corner_x || G.cy != G.corner_y) return 0;
    G.state = PS_GUARD; G.st = 0.0f; G.spd = 0.0f; G.u = 1.0f; G.hand = 0.0f;
    DBG_COUNT(corners_held);
    return 1;
}
static int dream_looks(void) {                                     /* at a lane cell: a glance back, or a spell through a door */
    if (G.player) return 0;
    if (G.glance) {
        G.glance = 0; G.glance_last = (float)G.level_t;
        begin_turn(atan2f(G.glance_y - G.py, G.glance_x - G.px));
        G.state = PS_LOOK; G.st = 0.0f; G.spd = 0.0f; G.u = 1.0f;
        DBG_COUNT(glances);
        return 1;
    }
    for (int k = 0; k < G.namb; k++) {
        const Ambush *a = &G.amb[k];
        if (!(G.amb_read >> k & 1) || (G.amb_probed >> k & 1) || a->sprung || no_cast()) continue;
        if (abs(G.cx - a->ix) > 2 || abs(G.cy - a->iy) > 2 || in_room(&G.rooms[a->room], G.cx, G.cy)) continue;
        G.amb_probed |= 1 << k;
        begin_turn(atan2f(a->fy + 0.5f - G.py, a->fx + 0.5f - G.px));
        G.state = PS_PROBE; G.st = 0.0f; G.spd = 0.0f; G.u = 1.0f; G.fired = 0; G.probe_mk = 0;
        DBG_COUNT(amb_probes);
        return 1;
    }
    for (int k = 0; k < G.nmk && !no_cast(); k++) {               /* a thing that wants a spell it holds, close by the lane (1.0.0b16) */
        Marked *m = &G.mk[k];
        int next_plinth = m->kind == MKK_PLINTH && !m->got && G.mk[m->grp - 1].st < 3 && G.mk[m->grp - 1].els[G.mk[m->grp - 1].st] == m->el;
        if (m->kind == MKK_TABLET) continue;                       /* the tablet only tells: the dream goes by the plinth next in its order */
        if (m->done || m->tried >= 2 || (m->grp && m->grp != k + 1 && !next_plinth) || (m->kind == MKK_PLINTH && !next_plinth)
            || (m->kind == MKK_WINCH && m->times) || MKK[m->kind].tier == 4) continue;   /* the bells
                                                                      as one; things for fights are a person's */
        if (m->kind == MKK_HSEAL && (RUN.gear.atk[LINE_ATK[EL_FIRE]] < 0 || RUN.gear.atk[LINE_ATK[EL_FROST]] < 0)) continue;
        float d = hypotf(m->x - G.px, m->y - G.py);
        if (d > (m->kind == MKK_RSEAL ? 13.0f : 6.5f) || d < 1.0f || !line_clear(G.px, G.py, m->x, m->y)) continue;   /* the switch from the ice's edge */
        int sp = LINE_ATK[m->el];
        if (RUN.gear.atk[sp] < 0 || (sp == AT_SPEAR && !spear_ready()) || in_combat()) continue;
        m->tried++;
        G.probe_mk = k + 1; G.probe_sp = sp; G.probe_n = 0; G.probe_last = 0.0f;
        begin_turn(atan2f(m->y - G.py, m->x - G.px));
        G.state = PS_PROBE; G.st = 0.0f; G.spd = 0.0f; G.u = 1.0f; G.fired = 0;
#ifdef CRAWL_DEBUG
        fprintf(stderr, "EVENT mk_dream %s at %.1f t=%.2f\n", MKK[m->kind].name, d, G.t);
#endif
        return 1;
    }
    return 0;
}

static int pick_goal(int x, int y, int *gx, int *gy) {
    if (boss_goal(x, y, gx, gy)) return 1;
    if (tri_goal(x, y, gx, gy)) return 1;                          /* a trial: in, and through it */
    if (corner_goal(x, y, gx, gy)) return 1;                       /* hurt, a pack on it: a corner to fight from */
    if (chain_goal(x, y, gx, gy)) return 1;
    if (pz_goal(x, y, gx, gy)) return 1;                           /* a puzzle, when there is time or the way on lies beyond */
    if (rest_goal(x, y, gx, gy)) return 1;                         /* hurt and nothing after it: a place to mend */
    int saving = RUN.goal > RUN.treasure;
    if (G.leaving && G.exit_fx >= 0) {                              /* time to go: the exit, by way of key and vault if need be */
        if (G.exit_in_vault >= 0 && key_goal(G.exit_in_vault, 0, gx, gy)) return 1;
        int hurt = G.hp < G.max_hp * 0.35f;                         /* badly hurt, it stops only for a potion */
        if (G.level_t < (G.hp >= G.max_hp * 0.6f ? 350.0 : 320.0)) {   /* but first, treasure it remembers seeing */
            for (int k = 0; k < G.nvaults && !hurt; k++)            /* a key in hand opens a hoard on the way out */
                if (G.has_key[k] && G.vault_door[k] >= 0 && G.doors[G.vault_door[k]].locked == 1) {
                    *gx = G.doors[G.vault_door[k]].x; *gy = G.doors[G.vault_door[k]].y;
                    return 1;
                }
            bfs_from(x, y);
            int best = -1, bd = 31;
            for (int i = 0; i < G.nitems; i++) {
                const Item *it = &G.items[i];
                if (!it->alive || !worth_taking(it) || (it->lx == x && it->ly == y) || (hurt && it->type != IT_POTION)) continue;
                int d = bfs_dist[it->ly][it->lx];
                if (it->type == IT_KEY) { if (d < 0 || d > 22 || !key_calls(it)) continue; d = imax(0, d - 8); }   /* a vault's worth: worth the walk */
                else if (!it->seen) continue;
                if (d >= 0 && d < bd) { bd = d; best = i; }
            }
            if (best >= 0) { *gx = G.items[best].lx; *gy = G.items[best].ly; return 1; }
        }
        *gx = G.exit_fx; *gy = G.exit_fy;
        return 1;
    }
    for (int k = 0; k < G.nvaults; k++)                             /* a key in hand: straight to its door */
        if (G.has_key[k] && G.vault_door[k] >= 0 && G.doors[G.vault_door[k]].locked == 1) {
            *gx = G.doors[G.vault_door[k]].x; *gy = G.doors[G.vault_door[k]].y;
            return 1;
        }
    bfs_from(x, y);
    if (mote_goal(gx, gy)) return 1;                               /* souls lying close by */
    int ax, ay;
    if (altar_wanted() && altar_spot(&ax, &ay) && bfs_dist[ay][ax] > 0
        && bfs_dist[ay][ax] <= (G.altar_state == ALTAR_READY ? 30 : 24)) { *gx = ax; *gy = ay; return 1; }   /* the altar, when it passes */
    if (!G.player && G.altar_state == ALTAR_AWAKE && G.altar_wisp == AW_SEEK && G.altar_seek >= 0 && !G.secrets[G.altar_seek].state
        && G.level_t < ALTAR_TIME) {                               /* where the violet wisp leads */
        const Secret *sc = &G.secrets[G.altar_seek];
        if (bfs_dist[sc->fy][sc->fx] > 0) { G.seek_secret = G.altar_seek; *gx = sc->fx; *gy = sc->fy; return 1; }
    }
    int best = -1; float bs = 1e9f;
    int reach = saving ? 30 : 24;                                   /* gold buys things now: it goes out of its way */
    for (int i = 0; i < G.nitems; i++) {
        const Item *it = &G.items[i];
        if (!it->alive || (it->lx == x && it->ly == y) || !worth_taking(it)) continue;
        int d = bfs_dist[it->ly][it->lx];
        if (!it->seen && !(it->type == IT_KEY && d >= 0 && d <= 14 && key_calls(it))) continue;   /* a key's jingle carries */
        if (d < 0 || d > reach) continue;
        float sc = (float)d - (it->type == IT_KEY ? 8.0f : 0.0f) - (it->type == IT_POTION ? 6.0f : 0.0f)
                 - (it->type == IT_CROWN || it->type == IT_CHEST || it->type == IT_RING ? 4.0f : 0.0f);
        if (sc < bs) { bs = sc; best = i; }
    }
    if (best < 0) return 0;
    *gx = G.items[best].lx; *gy = G.items[best].ly;
    return 1;
}

/* which way to leave (x,y): towards a goal if there is one, else new ground first,
 * otherwise towards the nearest unexplored cell */
static int hz_danger(int x, int y);
static int pz_plate_at(int x, int y) {                             /* a puzzle's stone or tile there */
    for (int i = 0; i < G.nthing; i++) if (G.thing[i].role == TR_PLATE && G.thing[i].cx == x && G.thing[i].cy == y) return 1;
    return 0;
}
static int decide(int x, int y, int h) {
    int gx, gy;
#ifdef CRAWL_DEBUG
    if (getenv("CRAWL_GOALS")) { int ok = pick_goal(x, y, &gx, &gy); fprintf(stderr, "GOAL at (%d,%d) -> %d (%d,%d) t=%.2f\n", x, y, ok, ok ? gx : -1, ok ? gy : -1, G.t); }
#endif
    if (pick_goal(x, y, &gx, &gy)) {
        if (!G.player && G.pz_stepx >= 0)                          /* a puzzle's stone beside it: off the lane, onto it */
            for (int d = 0; d < 4; d++) if (x + DX[d] == G.pz_stepx && y + DY[d] == G.pz_stepy && passable(G.pz_stepx, G.pz_stepy)) return d;
        bfs_from(gx, gy);
        int gbest = -1, gs = 1 << 30;
        for (int d = 0; d < 4; d++) {
            int nx = x + DX[d], ny = y + DY[d];
            if (!apass(nx, ny) || bfs_dist[ny][nx] < 0) continue;
            int sc = bfs_dist[ny][nx] * 4 + (d == h ? 0 : 1) + (d == ((h + 2) & 3) ? 2 : 0) + (G.bulky[ny][nx] ? 5 : 0) + 400 * hz_danger(nx, ny) + 9 * (hz_soft(nx, ny) | amb_soft(nx, ny))
                   + (bfs_dist[ny][nx] >= bfs_dist[y][x] ? 40 : 0);   /* not into a warning; a pile or a soft mark only chooses between ways on,
                                                                       never turns it back (it paced before a sapper's doorway) */
            if (sc < gs) { gs = sc; gbest = d; }
        }
        if (gbest >= 0 && bfs_dist[y][x] > 0) return gbest;
    }
    int best = -1; float bs = -1e9f;
    for (int d = 0; d < 4; d++) {
        if (d == ((h + 2) & 3)) continue;
        int nx = x + DX[d], ny = y + DY[d];
        if (!apass(nx, ny)) continue;
        float s;
        if (G.map[ny][nx].visits == 0) s = 100.0f + (d == h ? 1.3f : 0.0f) + rndf() * 1.6f;
        else s = -(float)dist_to_unexplored(nx, ny, x, y) + (d == h ? 0.4f : 0.0f) + rndf() * 0.8f;
        if (G.bulky[ny][nx]) s -= 2.5f;                           /* walk past piles, not through them */
        s -= 400.0f * (float)hz_danger(nx, ny) + 2.0f * (float)(hz_soft(nx, ny) | amb_soft(nx, ny));
        if (s > bs) { bs = s; best = d; }
    }
    return best < 0 ? ((h + 2) & 3) : best;
}

static void start_move(void) {
    G.tx = G.cx + DX[G.h]; G.ty = G.cy + DY[G.h];
    if (!apass(G.tx, G.ty) && !(pz_plate_at(G.tx, G.ty) && passable(G.tx, G.ty))) {   /* should not happen: turn instead (a puzzle's stones may be off the lanes) */
        G.next_h = decide(G.cx, G.cy, G.h);
        G.tx = G.cx; G.ty = G.cy; G.u = 1.0f;
        G.state = PS_MOVE;
        return;
    }
    G.next_h = decide(G.tx, G.ty, G.h);
    G.u = 0.0f;
    Cell *c = &G.map[G.ty][G.tx];
    if (c->kind == K_DOOR && G.doors[c->door].open < 0.95f) {
        Door *d = &G.doors[c->door];
        if (d->locked == 1) { d->locked = 2; G.has_key[d->color] = 0; G.unlock_t = G.level_t; snd_sfx(SFX_UNLOCK); pop_readout(); }
        if (!d->opening) d->opening = 1;
        G.state = PS_DOOR;
        G.spd = 0.0f;
        return;
    }
    G.state = PS_MOVE;
}

/* returns 1 when this step brought the caster to the exit, which takes over from walking */
static int arrive(void) {
    G.cx = G.tx; G.cy = G.ty;
    G.px = G.cx + 0.5f; G.py = G.cy + 0.5f;
    if (G.leaving && !G.exiting && G.cx == G.exit_fx && G.cy == G.exit_fy && !boss_sealed()) {   /* the way out: face it, open it, go */
        G.exiting = 1; G.exit_t = 0.0f; G.u = 1.0f;
        begin_turn(heading_angle(G.exit_dir)); G.adur = 0.4f;
        G.state = PS_EXIT; G.spd = 0.0f; G.hand = 0.0f;
        return 1;
    }
    for (int i = 0; i < G.bnfont && boss_fighting(); i++)          /* before a full font, badly hurt: a draught */
        if (G.cx == G.bfont_fx[i] && G.cy == G.bfont_fy[i] && G.bfont_full[i] && G.hp < G.max_hp * 0.4f) font_drink(i);
    if (G.map[G.cy][G.cx].visits < 250) {
        if (G.map[G.cy][G.cx].visits == 0) G.explored++;
        G.map[G.cy][G.cx].visits++;
    }
    for (int y = G.cy - 1; y <= G.cy + 1; y++)                     /* the lanes beside it are seen in passing */
        for (int x = G.cx - 1; x <= G.cx + 1; x++)
            if (apass(x, y) && G.map[y][x].visits == 0) { G.map[y][x].visits = 1; G.explored++; }
    for (int i = 0; i < G.nsecrets && G.seek_secret < 0 && !G.leaving; i++) {   /* the cracks catch its eye: every one while it */
        Secret *sc = &G.secrets[i];                                /* has time, a third of them after */
        if (sc->state || sc->rolled || abs(G.cx - sc->fx) + abs(G.cy - sc->fy) > 1) continue;
        sc->rolled = 1;
        if (G.level_t < 180.0 || rndf() < 0.35f) G.seek_secret = i;
    }
    if (G.seek_secret >= 0) {                                      /* one step across to stand before it */
        const Secret *sc = &G.secrets[G.seek_secret];
        for (int d = 0; d < 4; d++) if (G.cx + DX[d] == sc->fx && G.cy + DY[d] == sc->fy) G.next_h = d;
    }
    return 0;
}

/* at the lane beside a doorway or the visportal it means to use: face it, and go through or ask */
static int at_use(void) {
    int sx, sy;
    if (G.use_kind == USE_NONE || G.player || !use_spot(G.use_x, G.use_y, &sx, &sy) || G.cx != sx || G.cy != sy) return 0;
    begin_turn(atan2f(G.use_y + 0.5f - G.py, G.use_x + 0.5f - G.px));
    G.state = PS_USE; G.spd = 0.0f; G.fired = 0; G.u = 1.0f;
    return 1;
}

/* beside a place to mend, hurt and nothing after it: face it, and kneel */
static int at_rest(void) {
    int sx, sy, i = G.rest_goal;
    if (i < 0 || i >= G.nrest || !rest_wanted() || !use_spot(G.rest_x[i], G.rest_y[i], &sx, &sy) || G.cx != sx || G.cy != sy) return 0;
    begin_turn(atan2f(G.rest_y[i] + 0.5f - G.py, G.rest_x[i] + 0.5f - G.px));
    G.state = PS_MEND; G.spd = 0.0f; G.fired = 0; G.u = 1.0f;
    return 1;
}

/* beside the altar with something to ask of it: face it, and stop a moment */
static int at_altar(void) {
    int ax, ay;
    if (!altar_wanted() || !altar_spot(&ax, &ay) || G.cx != ax || G.cy != ay) return 0;
    begin_turn(atan2f(G.altar_y + 0.5f - G.py, G.altar_x + 0.5f - G.px));
    G.state = PS_ALTAR; G.spd = 0.0f; G.fired = 0; G.u = 1.0f;
    return 1;
}

/* standing before the secret it means to press: face it, press, wait for the stone to move */
static int at_secret(void) {
    if (G.seek_secret < 0) return 0;
    const Secret *sc = &G.secrets[G.seek_secret];
    if (G.cx != sc->fx || G.cy != sc->fy) return 0;
    begin_turn(heading_angle(sc->d));
    G.state = PS_PUSH; G.spd = 0.0f; G.fired = 0; G.u = 1.0f;
    return 1;
}

static void advance(float du) {                     /* move along the current step, with head bob and footsteps */
    G.u += du;
    float k = fminf(G.u, 1.0f);
    G.px = lerpf(G.cx + 0.5f, G.tx + 0.5f, k);
    G.py = lerpf(G.cy + 0.5f, G.ty + 0.5f, k);
    float ph0 = G.bobph;
    G.bobph += du * TAU_F;
    if (floorf(ph0 / PI_F) != floorf(G.bobph / PI_F)) footstep();
}

static void begin_turn(float target) {
    G.ang0 = G.ang;
    G.ang1 = G.ang + wrap_pi(target - G.ang);
    float delta = fabsf(G.ang1 - G.ang0);
    G.adur = delta > 2.0f ? 0.55f : (delta > 0.2f ? 0.30f : 0.05f);
    G.st = 0.0f;
}

/* can the caster pick this one out? a spider in its web shows only its eyes, and a wall skull
 * asleep is only a carving: neither, until it stirs */
static int targetable(const Mob *m) {
    if (G.latch == (int)(m - G.mobs) + 1) return 0;                /* a bat on your face: no spell reaches it */
    if (!m->alive || m->state != MS_WALK || (m->type == MOB_GHOST && !spirit_mastery()) || buried(m) || shade_hidden(m) || m->ally) return 0;
    if (m->type == MOB_GARGOYLE && m->sub == 0) return 0;          /* only a statue, as far as anyone can tell */
    if ((m->type == MOB_SPIDER || m->type == MOB_WSKULL) && m->sub == 0) return 0;
    if (m->boss && G.bstate != BS_FIGHT) return 0;                 /* untouchable: its adds first */
    if (m->boss && m->type == MOB_HUNTER && (m->sub == 0 || m->sub == 5)) return 0;   /* overhead */
    if (m->boss && !wraith_open(m)) return 0;                      /* the Wraith in the dark, or in the stone */
    return 1;
}

/* what to throw at: anything ahead, plus whatever is attacking from the side or behind
 * (a skeleton that has spotted the caster, a bat at its face, a sword, a spider, a skull's fire) */
static int find_target(void) {
    float dx = cosf(G.ang), dy = sinf(G.ang), rx = -dy, ry = dx;
    int best = -1; float bd = 1e9f;
    for (int i = 0; i < MAXMOBS; i++) {
        Mob *m = &G.mobs[i];
        if (!targetable(m)) continue;
        if (m->type == MOB_WRAITH && !m->boss && m->sub) continue;    /* driven back already: more fire only holds it */
        float vx = m->x - G.px, vy = m->y - G.py, dist = sqrtf(vx * vx + vy * vy);
        float depth = vx * dx + vy * dy, lat = vx * rx + vy * ry, score;
        const Pack *mp = pack_of(m);                               /* about to call (step 10): the caller first, from any side */
        int calling = m->call_t > 0.0f || (m->type == MOB_SEXTON && m->rings == 1) || (mp && !mp->broken && mp->state == PAS_CALL && mp->caller == i);
        int leads = mp && !mp->broken && mp->state == PAS_HUNT && mp->leader == i;
        if (depth >= 0.35f && depth <= 7.5f && fabsf(lat) <= fmaxf(depth * 0.42f, 0.3f)) score = depth;
        else if (calling && dist < 9.0f && line_clear(G.px, G.py, m->x, m->y)) score = dist + 1.5f;
        else if ((m->type == MOB_SKEL && m->seen && dist < 8.0f) || (m->type == MOB_BAT && dist < 1.4f) || (m->type == MOB_SHIELD && m->seen && dist < 3.0f)
                 || (m->type == MOB_SPIDER && m->sub >= 1 && dist < 2.0f) || (m->type == MOB_WSKULL && m->sub >= 1 && dist < 9.0f)
                 || (m->type == MOB_WRAITH && m->sub == 0 && dist < 7.0f) || (KIND_IS(m, KF_MELEE) && m->alert > 0.0f && dist < 4.0f)
                 || ((m->boss || m->add) && dist < 9.0f)) score = dist + 1.5f;   /* in a boss's fight, whatever is in it */
        else continue;
        if (m->type == MOB_WSKULL) score -= m->sub == 1 ? 4.0f : 1.5f;   /* one hit ends it: first, before it fires */
        if (m->type == MOB_DEACON || m->type == MOB_SEXTON) score -= 2.0f;   /* the priest first, before it raises more; the bell-ringer */
        if (calling) score -= 3.0f; else if (leads) score -= 1.5f;   /* a caller before it calls; a pack's leader, and it breaks */
        if (m->ignore > (float)G.t || !shot_clear(m->x, m->y)) continue;
        if (score < bd) { bd = score; best = i; }
    }
    return best;
}

/* which spell for this target: a crowd, a shield, a bat, a skull, or low on health */
static int choose_spell(int t) {
    const Gear *g = &RUN.gear;
    const Mob *m = &G.mobs[t];
    if (m->type == MOB_WRAITH && !m->boss)                         /* nothing kills it: fire or spirit drive it longest */
        return g->atk[AT_FIRE] >= 0 ? AT_FIRE : g->atk[AT_DRAIN] >= 0 ? AT_DRAIN : own_spell();
    if (m->type == MOB_GHOST) return AT_DRAIN;                     /* only Spirit reaches a spectre */
    float hpf = (float)G.hp / (float)G.max_hp;
    int vis = 0, close = 0, near = 0, packed = 0;
    for (int i = 0; i < MAXMOBS; i++) {
        const Mob *o = &G.mobs[i];
        if (!targetable(o)) continue;
        float d = hypotf(o->x - G.px, o->y - G.py);
        if (d < 10.0f && line_clear(G.px, G.py, o->x, o->y)) { vis++; if (d < 4.0f) close++; }
        if (o != m && hypotf(o->x - m->x, o->y - m->y) < 3.5f) { near++; if (hypotf(o->x - m->x, o->y - m->y) < 1.5f) packed++; }
    }
    if (g->atk[AT_BFS] >= 0 && RUN.souls >= bfs_cost(g->atk[AT_BFS]) && (vis >= 4 || (hpf < 0.3f && close >= 2))) return AT_BFS;
#ifdef CRAWL_DEBUG
    if (getenv("CRAWL_BFSANY") && g->atk[AT_BFS] >= 0 && RUN.souls >= bfs_cost(g->atk[AT_BFS]) && vis >= 2) return AT_BFS;
#endif
    int owned[NATK], no = 0, pick = own_spell();
    float best = -1.0f;
    int shielded = m->type == MOB_SHIELD && m->shield > 0.0f;
    float have = spirit();
    for (int sp = 0; sp < AT_BFS; sp++) {
        if (g->atk[sp] < 0 || (sp == AT_SPEAR && !spear_ready())) continue;   /* no bones nor splinters, no spears */
        owned[no++] = sp;
        int tier = g->atk[sp];
        float cost = spell_cost(g, sp, tier);
        if (cost > have) { tier = 1; cost = 0.0f; }                 /* what it would really get */
        float v = spell_damage_at(g, sp, tier) / cast_cycle(sp);
        if (sp == AT_SPEAR && spear_bare()) v *= SPLINTER_DMG;     /* a splinter: lighter, and through nothing */
        if (sp == AT_WISPS) v *= (float)wisp_count(tier);
        if (m->type == MOB_WSKULL) v = 1.0f / spell_cycle(g, sp);   /* any hit will do: the quickest */
        switch (sp) {
        case AT_SPEAR: if (!spear_bare()) { if (shielded) v *= 2.2f; if (near) v *= 1.3f; } else if (shielded) v *= 0.5f; break;
        case AT_CHAIN: v *= 1.0f + 0.6f * (float)(near < 3 ? near : 3); if (shielded) v *= 1.8f; break;
        case AT_FLASK: v *= 1.0f + 0.6f * (float)(packed < 3 ? packed : 3); break;   /* a splash, and the poison after */
        case AT_DRAIN: if (hpf < 0.45f && have > 5.0f) v *= 2.2f; break;   /* the healing costs spirit too */
        case AT_FROST: if (m->type == MOB_BAT || m->type == MOB_SPIDER || m->type == MOB_SHIELD) v *= 1.2f; break;   /* slowing what closes in */
        default: break;
        }
        if (m->type != MOB_WSKULL) v *= hit_mult(m, sp);           /* what it is weak to, the elements worn too */
        if (m->boss == 1 + BOSS_KNIGHT && sp != AT_CHAIN && sp != AT_DRAIN) v *= 0.8f;   /* its Mirror Rune turns missiles */
        if (sp == AT_WISPS) v *= 0.75f;                            /* not every wisp gets there */
        v /= 1.0f + cost * (have < 15.0f ? 1.0f : 0.25f);          /* thriftier as spirit runs low */
        if (sp == G.spell) v *= 0.8f;                              /* and a little variety */
        v *= 0.85f + 0.3f * rndf();
        if (v > best) { best = v; pick = sp; }
    }
    if (no > 1 && rnd(100) < 12) pick = owned[rnd(no)];             /* a whim */
    return pick;
}

static void cast_windup(void) {                                    /* the autopilot's cast: it gathers, then goes */
    G.cast_wind = spell_windup(&RUN.gear, G.spell, spell_tier(G.spell)) * af_slow_cast();
    G.cast_len = G.cast_wind + 0.3f;
}
/* The autopilot's ultimates (docs/PLAN-b11.md, section 14): it charges one when three or more are in reach (in sight
 * within seven cells), or for a boss; calls Firewall when something comes to blows; holds Soul Tether on the strongest it sees. Only
 * with the spirit for a full charge (or the bones) */
static int spell_charges(int sp);
static int spell_tethers(int sp);
static void auto_charge_pick(void) {
    G.auto_chg = 0;
    int sp = G.spell, t = G.target, tier = spell_tier(sp);
    if (t < 0 || !targetable(&G.mobs[t]) || no_cast()) return;
    const Mob *m = &G.mobs[t];
    if (spell_tethers(sp)) {                                       /* Soul Tether: on the strongest in sight */
        if (spirit() < 3.0f) return;
        int best = t;
        for (int i = 0; i < MAXMOBS; i++) {
            const Mob *o = &G.mobs[i];
            if (targetable(o) && hypotf(o->x - G.px, o->y - G.py) < 8.0f && line_clear(G.px, G.py, o->x, o->y) && o->hp > G.mobs[best].hp) best = i;
        }
        G.target = best; G.auto_chg = 1; G.auto_hold = 2.5f;
        return;
    }
    if (fire_grows(sp) && !spell_charges(sp)) {                    /* Burning Intensity: held while what it aims at is far, longer the
                                                                      further, if nothing has come to blows */
        float d = hypotf(m->x - G.px, m->y - G.py), hold = clampf((d - 4.0f) * 0.6f, 0.0f, FIRE_GROW_T - 0.25f);
        for (int i = 0; i < MAXMOBS && hold > 0.0f; i++) {
            const Mob *o = &G.mobs[i];
            if (targetable(o) && hypotf(o->x - G.px, o->y - G.py) < 2.5f) hold = 0.0f;
        }
        if (hold >= 0.5f && spirit() >= spell_cost(&RUN.gear, sp, tier) * 1.3f) { G.auto_chg = 1; G.auto_hold = hold; }
        return;
    }
    if (!spell_charges(sp)) return;
    if (sp == AT_SPEAR ? RUN.gear.bones <= 0 : spirit() < spell_cost(&RUN.gear, sp, tier) * 2.5f) return;
    int near = 0, blows = 0;
    for (int i = 0; i < MAXMOBS; i++) {
        const Mob *o = &G.mobs[i];
        if (!targetable(o)) continue;
        float d = hypotf(o->x - G.px, o->y - G.py);
        if (d < 7.0f && line_clear(G.px, G.py, o->x, o->y)) near++;   /* in reach: in sight within seven cells */
        if (hypotf(o->x - G.px, o->y - G.py) < 1.7f && !KIND_IS(o, KF_SPIRIT)) blows++;
    }
    if (sp == AT_FIRE) G.auto_chg = blows > 0 && G.fwall_t <= 0.0f; /* Firewall: when something has come to blows */
    else G.auto_chg = near >= 3 || m->boss;
}
static void aim_at(int t) {
    G.target = t; G.resume_state = PS_MOVE;
    G.spell = choose_spell(t);
    cast_windup();
    auto_charge_pick();
    begin_turn(atan2f(G.mobs[t].y - G.py, G.mobs[t].x - G.px));
    G.adur = 0.18f + 0.13f * fabsf(G.ang1 - G.ang0);
    G.state = PS_AIM; G.spd = 0.0f;
}

static void pocket_return(int died);
static void auto_abilities(void);
static void update_player(float dt) {
    if (G.dash_t > 0.0f) {                                         /* Swift Dodge: darting to a lane cell */
        float k = fminf(dt, G.dash_t);
        G.px += G.dash_vx * k; G.py += G.dash_vy * k;
        if ((G.dash_t -= dt) > 0.0f) return;
        G.dash_t = 0.0f;
        G.cx = G.tx; G.cy = G.ty; G.px = G.cx + 0.5f; G.py = G.cy + 0.5f; G.u = 1.0f; G.hand = 0.0f; G.spd = 0.0f;
        if (G.map[G.cy][G.cx].visits < 250) { if (!G.map[G.cy][G.cx].visits) G.explored++; G.map[G.cy][G.cx].visits++; }
        G.next_h = decide(G.cx, G.cy, G.h);
        begin_turn(heading_angle(G.next_h)); G.state = PS_TURN;
        return;
    }
    if (af_rooted() && !G.dead) {                                   /* frozen or on the floor: it waits it out, its Blink ready (PLAN-combat, 4) */
        G.cooldown -= dt; G.spd = 0.0f; G.hand = fmaxf(0.0f, G.hand - dt * 4.0f);
        return;
    }
    auto_abilities();
    if (G.dash_t > 0.0f) return;
    if (G.web_hp > 0.0f && !G.dead && (G.web_idle += dt) > 1.2f) {   /* held, and nothing to aim at: it tears at the web */
        G.web_idle = 0.0f;
        cast_spell(own_spell(), -1);
    }
    G.cooldown -= dt;
    G.calm += dt;
    amb_read_tick();                                               /* an ambush's sign, read in passing (step 10) */
    dream_restore(dt);                                             /* Restoration, out of a fight (D5, 10.3) */
    if ((G.state == PS_AIM || G.state == PS_HOLD || G.state == PS_UNAIM || (G.state == PS_CAST && G.fired) || (G.state == PS_CAST && no_cast()))
        && hz_danger((int)G.px, (int)G.py)) {                      /* the floor warns: it stops aiming and goes on */
        G.hand = 0.0f; G.state = PS_MOVE; G.ang = wrap_pi(G.ang);
        begin_turn(heading_angle(G.h)); G.state = PS_TURN; G.st = 0.0f; G.adur = 0.15f;
    }
    switch (G.state) {
    case PS_MOVE: {
        int keyed = G.has_key[0] || G.has_key[1] || G.has_key[2];
        float target = (keyed ? CRUISE * 1.25f : CRUISE) * gear_haste(&RUN.gear) * slow_factor();   /* a key in hand: hurry */
        if (G.next_h != G.h && G.u > 0.62f) target = CRUISE * 0.45f;
        int blocker = -1;                                          /* something in the way: stop and deal with it */
        for (int i = 0; i < MAXMOBS && G.blocked < 3.0f; i++) {
            Mob *m = &G.mobs[i];
            if (!solid_mob(m) || m->state != MS_WALK || m->ally) continue;
            float vx = m->x - G.px, vy = m->y - G.py;
            float ahead = vx * cosf(G.ang) + vy * sinf(G.ang), side = -vx * sinf(G.ang) + vy * cosf(G.ang);
            if (ahead > 0.0f && ahead < 1.3f && fabsf(side) < 0.55f && line_clear(G.px, G.py, m->x, m->y)) blocker = i;
        }
        if (blocker >= 0) {
            target = 0.0f; G.spd = fminf(G.spd, 0.3f); G.blocked += dt;
            if (G.cooldown <= 0.0f && !no_cast()) { aim_at(blocker); break; }
        } else G.blocked = 0.0f;
        G.spd += clampf(target - G.spd, -6.0f * dt, 6.0f * dt);
        advance(G.spd * dt);
        if (G.next_h != G.h && G.u >= 0.78f) {
            /* corner ahead: start the quick turn while finishing the step */
            begin_turn(heading_angle(G.next_h));
            G.state = PS_TURN;
            break;
        }
        if (G.u >= 1.0f) {
            if (arrive() || at_secret() || at_altar() || at_use() || at_rest() || at_corner() || dream_looks()) break;
            if (G.next_h != G.h) {
                begin_turn(heading_angle(G.next_h));
                G.state = PS_TURN;
            } else start_move();
        }
        if (G.cooldown <= 0.0f && G.state == PS_MOVE && !no_cast()) {
            int t = find_target();
            if (t >= 0) aim_at(t);
        }
        break;
    }
    case PS_TURN: {
        G.st += dt;
        if (G.u < 1.0f) {                                         /* still finishing the step into the corner */
            advance(fmaxf(G.spd, CRUISE * 0.4f) * dt);
            if (G.u >= 1.0f && (arrive() || at_secret() || at_altar() || at_use() || at_rest() || at_corner())) break;
        } else G.spd *= 0.85f;
        float k = smoothstepf(0.0f, 1.0f, G.st / G.adur);
        G.ang = lerpf(G.ang0, G.ang1, k);
        if (G.st >= G.adur) {
            if (G.u < 1.0f) { advance(1.0f - G.u); if (arrive() || at_secret() || at_altar() || at_use() || at_rest() || at_corner()) break; }
            G.ang = wrap_pi(G.ang1);
            G.h = G.next_h;
            G.spd = CRUISE * 0.6f;
            start_move();
        }
        break;
    }
    case PS_DOOR: {
        Cell *c = cell(G.tx, G.ty);
        if (c->kind != K_DOOR || G.doors[c->door].open >= 0.95f) G.state = PS_MOVE;
        break;
    }
    case PS_AIM: {
        G.st += dt;
        Mob *m = &G.mobs[G.target];
        if (m->alive) G.ang1 = G.ang0 + wrap_pi(atan2f(m->y - G.py, m->x - G.px) - G.ang0);
        G.ang = lerpf(G.ang0, G.ang1, smoothstepf(0.0f, 1.0f, G.st / G.adur));
        if (G.st >= G.adur) { G.state = PS_CAST; G.st = 0.0f; G.fired = 0; }
        break;
    }
    case PS_CAST: {
        G.st += dt;
        if (G.auto_chg && !G.fired) {                              /* charging an ultimate, or holding the tether */
            Mob *m = &G.mobs[G.target];
            G.chg_on = 1; G.chg_sp = G.spell; G.chg_t = G.st;
            G.hand = fminf(1.0f, G.st / 0.2f);
            if (targetable(m)) G.ang += wrap_pi(atan2f(m->y - G.py, m->x - G.px) - G.ang) * fminf(1.0f, dt * 8.0f);
            int done = 0;
            if (spell_tethers(G.spell)) {
                if (G.st >= 0.25f) { if (!G.tether && targetable(m)) G.tether = G.target + 1; tether_hold(dt); }
                done = G.st >= 0.25f + G.auto_hold || (G.st > 0.6f && !G.tether) || !G.chg_on;
            } else if (fire_grows(G.spell) && !spell_charges(G.spell)) {   /* Burning Intensity's bolt, held its while */
                if (G.st >= 0.25f + G.auto_hold || !targetable(m)) { cast_held(G.spell, targetable(m) ? G.target : -1, G.st); done = 1; }
            } else if (G.st >= hold_span(G.spell) + 0.05f) { cast_charged(G.spell, targetable(m) ? G.target : -1, 1.0f); done = 1; }
            if (done) {
                G.chg_on = 0; G.auto_chg = 0; tether_end(); G.fired = 1;
                G.hand = 0.0f; G.cooldown = cast_cycle(G.spell) * 1.5f;
                begin_turn(heading_angle(G.h)); G.adur = 0.18f + 0.13f * fabsf(G.ang1 - G.ang0); G.state = PS_UNAIM;
            }
            break;
        }
        float L = G.cast_len, fire = G.cast_wind, up = fminf(0.2f, fire), down = fire + 0.1f;   /* up while it gathers */
        G.hand = G.st < up ? G.st / up : (G.st > down ? fmaxf(0.0f, 1.0f - (G.st - down) / fmaxf(0.05f, L - down)) : 1.0f);
        if (!G.fired && G.st >= fire) {
            G.fired = 1;
            if (G.spell == AT_BFS && RUN.souls < bfs_cost(RUN.gear.atk[AT_BFS])) G.spell = own_spell();
            cast_spell(G.spell, G.target);
            G.calm = 0.0f;
        }
        if (G.st >= L) {
            Mob *m = &G.mobs[G.target];
            G.hand = 0.0f; G.cooldown = G.fired == 2 ? 0.1f : fmaxf(0.15f, cast_cycle(G.spell) - L);   /* 2: a blow broke it */
            if (m->alive && ++m->misses >= 3) m->ignore = (float)G.t + 5.0f;   /* three throws and no hit: leave it */
            int stand = m->type == MOB_SKEL || m->type == MOB_SHIELD || m->type == MOB_WSKULL || (m->type == MOB_SPIDER && m->sub >= 1)
                        || m->type == MOB_SHAMBLER || m->type == MOB_DROWNED;   /* the slow ones: stand and finish them */
            if (targetable(m) && stand && hypotf(m->x - G.px, m->y - G.py) < 8.0f
                && m->ignore < (float)G.t && shot_clear(m->x, m->y)) { G.state = PS_HOLD; G.st = 0.0f; break; }   /* stand and finish it */
            begin_turn(heading_angle(G.h)); G.adur = 0.18f + 0.13f * fabsf(G.ang1 - G.ang0); G.state = PS_UNAIM;
        }
        break;
    }
    case PS_HOLD: {
        Mob *m = &G.mobs[G.target];
        G.st += dt;
        int ok = targetable(m) && shot_clear(m->x, m->y) && G.st < 6.0f;
        if (ok) G.ang += wrap_pi(atan2f(m->y - G.py, m->x - G.px) - G.ang) * fminf(1.0f, dt * 10.0f);
        if (!ok) { begin_turn(heading_angle(G.h)); G.adur = 0.18f + 0.13f * fabsf(G.ang1 - G.ang0); G.state = PS_UNAIM; }
        else if (G.cooldown <= 0.0f) {
            G.spell = choose_spell(G.target);
            cast_windup();
            auto_charge_pick();
            G.state = PS_CAST; G.st = 0.0f; G.fired = 0;
        }
        break;
    }
    case PS_EXIT: {
        G.st += dt;
        float pt = G.exit_t;
        G.exit_t += dt;
        G.ang = G.st < G.adur ? lerpf(G.ang0, G.ang1, smoothstepf(0.0f, 1.0f, G.st / G.adur)) : G.ang1;
        if (pt < 0.4f && G.exit_t >= 0.4f) { snd_sfx(SFX_DOOR); snd_sfx_ex(SFX_SHIMMER, 0.8f, 0.5f); }
        G.exit_open = clampf((G.exit_t - 0.4f) / 0.7f, 0.0f, 1.0f);
        if (G.exit_t > 1.2f && G.exit_t < 2.1f) { G.px += cosf(G.ang) * dt * 0.5f; G.py += sinf(G.ang) * dt * 0.5f; G.bobph += dt * 5.0f; }
        if (G.exit_t >= 2.4f && !G.tally) {
            G.tally = 1; G.tally_t = 0.0; G.clear_time = G.level_t; G.retries = 0;
            snd_sfx_ex(SFX_FANFARE, 1.0f, 0.5f);
            rec_cleared();
            tu_cleared();                                          /* a drawn level's lessons given, for good */
            char lc[36]; snprintf(lc, sizeof lc, "LEVEL %d CLEARED", G.level);
            xp_gain(30.0f * xp_level(), lc);
#ifdef CRAWL_DEBUG
            fprintf(stderr, "EVENT cleared level %d t=%.1f by %s\n", G.level, G.level_t, G.player ? "player" : "autopilot");
            fprintf(stderr, "EVENT souls level %d in=%.1f out=%.1f held=%.1f weak=%d\n", G.level, dbg_soul_in, dbg_soul_out, RUN.souls + RUN.soul_frac, dbg_weak);
            dbg_soul_in = dbg_soul_out = 0.0f; dbg_weak = 0;
#endif
        }
        break;
    }
    case PS_PUSH: {
        G.st += dt;
        G.ang = lerpf(G.ang0, G.ang1, smoothstepf(0.0f, 1.0f, fminf(1.0f, G.st / G.adur)));
        float k = G.st - G.adur;
        G.hand = k < 0.0f ? 0.0f : (k < 0.25f ? k / 0.25f * 0.7f : (k < 0.8f ? 0.7f : fmaxf(0.0f, 0.7f - (k - 0.8f) * 2.5f)));
        if (k >= 0.3f && !G.fired) { G.fired = 1; open_secret(G.seek_secret); }
        if (k >= 1.2f && G.slide < 0) {                             /* the way is open: in it goes */
            G.ang = wrap_pi(G.ang1); G.hand = 0.0f;
            G.h = G.next_h = G.secrets[G.seek_secret].d; G.seek_secret = -1;
            G.state = PS_MOVE; start_move();
        }
        break;
    }
    case PS_USE: {                                                 /* a doorway in a chain, or the visportal's gift */
        G.st += dt;
        G.ang = lerpf(G.ang0, G.ang1, smoothstepf(0.0f, 1.0f, fminf(1.0f, G.st / G.adur)));
        float k = G.st - G.adur;
        G.hand = G.use_kind == USE_VIS ? (k < 0.0f ? 0.0f : (k < 0.25f ? k / 0.25f * 0.7f : (k < 0.9f ? 0.7f : fmaxf(0.0f, 0.7f - (k - 0.9f) * 2.5f)))) : 0.0f;
        if (k >= 0.3f && !G.fired) {
            G.fired = 1;
#ifdef CRAWL_DEBUG
            fprintf(stderr, "EVENT chain_use %s (%d,%d) link %d t=%.2f\n", G.use_kind == USE_DOOR ? "door" : G.use_kind == USE_THING ? "thing" : "vis", G.use_x, G.use_y, PK.depth, G.t);
#endif
            if (G.use_kind == USE_THING) {                         /* a puzzle's piece, touched */
                if (G.use_thing >= 0 && G.use_thing < G.nthing) thing_use(G.use_thing);
                G.pz_wait = G.use_thing >= 0 && (G.thing[G.use_thing].role == TR_STRIKE || G.pz == PZ_VENTS) ? 0.3f + 0.4f * rndf() : 0.8f + 1.4f * rndf();   /* slowly, as if thinking; a phrase, quicker */
                if (G.use_thing >= 0 && G.use_thing < G.nthing && G.thing[G.use_thing].tri) G.pz_wait = 0.0f;   /* a trial's: no time to think */
            } else if (G.use_kind == USE_DOOR) {                   /* through it: a new place, and G with it */
                if (!PK.depth) for (int i = 0; i < G.nsecrets; i++) if (G.secrets[i].door_x == G.use_x && G.secrets[i].door_y == G.use_y) PK.auto_done[i] = 1;
                if (pocket_go(G.use_x, G.use_y)) break;
            } else { visportal_use(); PK.auto_back = 1; }
        }
        if (k >= (G.use_kind == USE_VIS ? 1.2f : 0.5f) && !G.work_i && G.tri_hold <= 0.0f) {   /* (a slow thing: until the work is done;
                                                                      a trial's bell, till the toll) */
            G.ang = wrap_pi(G.ang1); G.hand = 0.0f; G.use_kind = USE_NONE;
            G.next_h = decide(G.cx, G.cy, G.h);
            begin_turn(heading_angle(G.next_h));
            G.state = PS_TURN;
        }
        break;
    }
    case PS_ALTAR: {                                               /* a hand to the altar: anointed, woken, sent on, blessed */
        G.st += dt;
        G.ang = lerpf(G.ang0, G.ang1, smoothstepf(0.0f, 1.0f, fminf(1.0f, G.st / G.adur)));
        float k = G.st - G.adur;
        G.hand = k < 0.0f ? 0.0f : (k < 0.25f ? k / 0.25f * 0.7f : (k < 0.9f ? 0.7f : fmaxf(0.0f, 0.7f - (k - 0.9f) * 2.5f)));
        if (k >= 0.4f && !G.fired) {
            G.fired = 1;
            if (wants_anoint()) altar_anoint();
            if (wants_wake()) altar_wake();
            if (wants_send()) altar_send();
            if (wants_bless()) altar_bless();
        }
        if (k >= 1.2f) {
            G.ang = wrap_pi(G.ang1); G.hand = 0.0f;
            G.next_h = decide(G.cx, G.cy, G.h);
            begin_turn(heading_angle(G.next_h));
            G.state = PS_TURN;
        }
        break;
    }
    case PS_MEND: {                                                /* kneeling: up when it can mend no further, struck, or something comes */
        G.st += dt;
        G.ang = lerpf(G.ang0, G.ang1, smoothstepf(0.0f, 1.0f, fminf(1.0f, G.st / G.adur)));
        if (!G.fired && G.st >= G.adur) { G.fired = 1; if (G.rest_goal >= 0) rest_begin(G.rest_goal); }
        int come = in_combat();
        if ((G.fired && (!G.rest_on || come)) || G.st > 40.0f) {
            rest_end(come ? "something came" : "done");
            G.ang = wrap_pi(G.ang1);
            G.next_h = decide(G.cx, G.cy, G.h);
            begin_turn(heading_angle(G.next_h));
            G.state = PS_TURN;
        }
        break;
    }
    case PS_LOOK: {                                                /* a glance back: a look, and on, unless something is there */
        G.st += dt;
        G.ang = lerpf(G.ang0, G.ang1, smoothstepf(0.0f, 1.0f, fminf(1.0f, G.st / G.adur)));
        if (G.st < G.adur) break;
        int t = G.cooldown <= 0.0f && !no_cast() ? find_target() : -1;
        if (t >= 0) { aim_at(t); DBG_COUNT(glance_finds); break; }
        if (G.st >= G.adur + 0.5f) { begin_turn(heading_angle(G.next_h)); G.state = PS_TURN; }
        break;
    }
    case PS_PROBE: {                                               /* a spell through an ambush's door before going by it */
        G.st += dt;
        G.ang = lerpf(G.ang0, G.ang1, smoothstepf(0.0f, 1.0f, fminf(1.0f, G.st / G.adur)));
        if (G.probe_mk > 0 && G.probe_mk <= G.nmk) {               /* or at a thing that wants a spell, till it is answered */
            Marked *m = &G.mk[G.probe_mk - 1], *a = m;
            int need = m->kind == MKK_BELL || m->kind == MKK_TABLET ? 3 : m->kind == MKK_HSEAL ? 2 : m->kind == MKK_RELAY ? 4 : m->kind == MKK_RELIQ ? 6 : m->pips - m->got;
            if (m->kind == MKK_PLINTH) need = 1;
            else if (m->kind == MKK_TABLET) {                      /* the plinth next in the tablet's order */
                for (int i = m->grp; i < G.nmk; i++) if (G.mk[i].grp == m->grp && G.mk[i].kind == MKK_PLINTH && !G.mk[i].got && m->st < 3 && G.mk[i].el == m->els[m->st]) { a = &G.mk[i]; break; }
            } else if (m->grp && m->kind != MKK_PLINTH)            /* the bells: the next not ringing; the relay: the first not lit */
                for (int i = m->grp - 1; i < G.nmk; i++)
                    if (G.mk[i].grp == m->grp && (m->kind == MKK_BELL ? !G.mk[i].got : G.mk[i].st != 1)) { a = &G.mk[i]; break; }
            int sp = LINE_ATK[a->kind == MKK_COIL ? EL_LIGHT : a->kind == MKK_RELAY ? EL_FIRE : a->el];
            int wait = m->kind == MKK_RELIQ && m->st == 1;          /* turning: wait for the face */
            int more = !m->done && !(m->kind == MKK_PLINTH && m->got) && G.probe_n < need + 2 && !in_combat() && !no_cast() && RUN.gear.atk[sp] >= 0;
            if (more && !wait && G.st >= G.adur + 0.1f && G.cooldown <= 0.0f) {
                G.ang = atan2f(a->y - G.py, a->x - G.px);          /* it may have moved */
                G.spell = sp; G.hand = 1.0f; cast_spell(sp, -1);
                G.probe_n++; G.cooldown = cast_cycle(sp); G.calm = 0.0f; G.probe_last = G.st;
            }
            G.hand = fmaxf(0.0f, G.hand - dt * 2.0f);
            if (!more && G.st >= G.probe_last + 0.6f && G.st >= G.adur + 0.2f) { G.probe_mk = 0; G.hand = 0.0f; begin_turn(heading_angle(G.next_h)); G.state = PS_TURN; }
            break;
        }
        if (G.st >= G.adur + 0.1f && !G.fired) { G.fired = 1; G.hand = 1.0f; cast_spell(own_missile(), -1); G.calm = 0.0f; }
        if (G.fired) G.hand = fmaxf(0.0f, G.hand - dt * 2.0f);
        if (G.st >= G.adur + 0.7f) { G.hand = 0.0f; begin_turn(heading_angle(G.next_h)); G.state = PS_TURN; }
        break;
    }
    case PS_GUARD: {                                               /* a corner held: what comes, it fights from here */
        G.st += dt; G.guard_t += dt; G.spd = 0.0f;
        if (!corner_keep()) { corner_leave(); break; }
        int t = guard_target();
        if (t >= 0) {
            if (G.cooldown <= 0.0f && !no_cast()) { aim_at(t); G.resume_state = PS_GUARD; break; }
            G.ang += wrap_pi(atan2f(G.mobs[t].y - G.py, G.mobs[t].x - G.px) - G.ang) * fminf(1.0f, dt * 6.0f);   /* facing it */
        }
        break;
    }
    case PS_UNAIM:
        G.st += dt;
        G.ang = lerpf(G.ang0, G.ang1, smoothstepf(0.0f, 1.0f, G.st / G.adur));
        if (G.st >= G.adur) { G.ang = wrap_pi(G.ang1); G.state = G.resume_state; }
        break;
    case PS_SETTLE: {                                              /* handed back: to the middle of the cell, square to the walls */
        G.st += dt;
        float k = smoothstepf(0.0f, 1.0f, fminf(1.0f, G.st / 0.45f));
        G.px = lerpf(G.sx0, G.cx + 0.5f, k); G.py = lerpf(G.sy0, G.cy + 0.5f, k);
        G.ang = lerpf(G.ang0, G.ang1, k);
        G.hand = fmaxf(0.0f, G.hand - dt * 2.0f);
        if (G.st >= 0.45f) { G.ang = wrap_pi(G.ang1); G.u = 1.0f; G.tx = G.cx; G.ty = G.cy; start_move(); }
        break;
    }
    }
    float amp = 0.014f * clampf(G.spd / CRUISE, 0.0f, 1.0f);
    G.camz = 0.5f + amp * sinf(G.bobph);
}

