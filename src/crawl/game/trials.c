/* A part of scene_crawl.c (docs/PLAN-tidy.md): it is #included there in its place and never compiled on its own.
 * Trials: kinds, waves, tri_begin, tri_run, the reward, tri_end, slow work, where a trial is placed. */

/* ---- trials (1.0.0b13, docs/PLAN-b13.md part D) ----
 * On every level from the third, a boss's too (outside its arena), one room off the way with one door, marked with the
 * run's skull and crossed bones, two red lights by it. A cell and a half past the door it seals behind you and the room turns:
 * its light, its walls, a quicker march, and an hourglass at the top of the view with sixty seconds of sand and no
 * numbers. Win, and the hourglass breaks, bones fill, spirit goes to half the vessel if it was less, a hoard and a draught
 * that makes you whole are left in the room (tri_reward, 1.0.0b18.1; health had come back at once), and THE TRIAL'S BOON
 * gives a fifth more experience from combat for the rest of the level; let the sand run
 * out and its creatures crumble, and THE TRIAL'S CURSE takes a fifth from all experience instead. Falling there is a
 * death as anywhere. The dream takes every trial it finds with two thirds of its health. */
static const char *const TRI_NAME[NTRI] = { "THE OSSUARY WAKES", "THE VIGIL", "LAST RITES", "THE KNIGHT'S WAGER", "THE TOLL",
    "RISING WATER", "THE BROOD", "SHUT THE SLUICES", "THE SAINTS' LAMPS", "THE CAVE-IN", "THE CHARNEL FEAST", "THE RITE", "FIREDAMP",
    "THE MOLTEN RISE", "DARK HOURS", "THE GLASS COURT", "SINGING GLASS" };
static const char *const TRI_TASK[NTRI] = { "PUT EVERY ONE DOWN", "KEEP A CANDLE BURNING", "BRING THE RELICS TO THE BIER", "BEST THE KNIGHT",
    "RING THE FOUR BELLS", "SLAY THE DROWNED BEFORE THE WATER RISES", "BREAK THE SACS BEFORE THEY HATCH", "TURN THE FIVE WHEELS",
    "LIGHT THE FIVE LAMPS", "BRACE THE FOUR PROPS", "SLAY THE GHOULS", "SILENCE THE FOUR DEACONS", "OPEN THE THREE VENTS",
    "SLAY THE IMPS", "KEEP FOUR BRAZIERS BURNING", "STRIKE DOWN THE CROWNED ONE", "STRIKE THE CRYSTALS AS THEY LIGHT" };
static const int8_t TRI_RUN[4][5] = { { TRI_OSSUARY, TRI_VIGIL, TRI_RITES, TRI_WAGER, TRI_TOLL },
                                      { TRI_WATER, TRI_BROOD, TRI_SLUICES, TRI_LAMPS, -1 },
                                      { TRI_CAVEIN, TRI_FEAST, TRI_RITE, TRI_FIREDAMP, -1 },
                                      { TRI_MOLTEN, TRI_DARK, TRI_COURT, TRI_SINGING, -1 } };
static int tri_run_of(int kind) { for (int r = 0; r < 4; r++) for (int k = 0; k < 5; k++) if (TRI_RUN[r][k] == kind) return r; return 0; }
static const uint8_t TRI_FOG[NTRI] = { R_MOSS, R_WARM, R_PURPLE, R_BLOOD, R_GHOST, R_GHOST, R_MOSS, R_GHOST, R_IRON, R_WARM, R_BLOOD, R_PURPLE,
                                       R_MOSS, R_BRICK, R_PURPLE, R_IRON, R_GHOST };   /* the room's light, while it is turned */
static const uint8_t TRI_HOLD[NTRI] = { [TRI_VIGIL] = 1, [TRI_DARK] = 1 };   /* won by holding to the end, not by finishing */
static int in_trial(int x, int y) { return G.tri >= 0 && G.tri_room >= 0 && !PK.depth && !G.shop && in_room(&G.rooms[G.tri_room], x, y); }
static int tri_level(int level) { return level >= 3; }                   /* T2: one on every level from the third */
static int tri_kind_of(int level) {                                /* T4: the run's, each in turn on the way down, then round again */
#ifdef CRAWL_DEBUG
    if (getenv("CRAWL_TRIAL")) return clampi(atoi(getenv("CRAWL_TRIAL")), 0, NTRI - 1);
#endif
    int set = crawl_set(level), n = 0, order[5];
    for (int k = 0; k < 5 && TRI_RUN[set][k] >= 0; k++) order[n++] = TRI_RUN[set][k];
    uint32_t h = (uint32_t)crawl_run(level) * 2654435761u ^ RUN.run_id * 40503u;
    for (int i = n - 1; i > 0; i--) { h = h * 1664525u + 1013904223u; int j = (int)((h >> 8) % (uint32_t)(i + 1)), t = order[i]; order[i] = order[j]; order[j] = t; }
    int first = set == 0 && crawl_loop(level) == 0 ? 2 : 0;          /* the step its first trial falls on (steps count from 0) */
    return order[imax(0, crawl_step(level) - first) % n];
}
static int tri_alive(int which) {                                  /* the trial's creatures standing (which: 0 all, else that mark) */
    int n = 0;
    for (int i = 0; i < MAXMOBS; i++) { const Mob *m = &G.mobs[i]; n += m->alive && m->state == MS_WALK && m->tri && (!which || m->tri == which); }
    return n;
}
/* a cell in the trial's room for something to stand: off the lanes with a lane beside it, away from the door, from
 * you and from the room's other things; in a small room, on a lane or nearer the rest if it must */
static int tri_spot(int *ox, int *oy, float from_you) {
    const Room *R = &G.rooms[G.tri_room];
    const Door *dr = &G.doors[G.tri_door];
    for (int pass = 0; pass < 3; pass++)
        for (int tries = 0; tries < 300; tries++) {
            int x = R->x + rnd(R->w), y = R->y + rnd(R->h), lanes = 0;
            if (!hz_floor(x, y) || (lane(x, y) && pass == 0) || G.bulky[y][x] || abs(x - dr->x) + abs(y - dr->y) < 3 - (pass == 2)) continue;
            if (hypotf(x + 0.5f - G.px, y + 0.5f - G.py) < from_you * (pass == 2 ? 0.5f : 1.0f)) continue;
            for (int d = 0; d < 4; d++) lanes += apass(x + DX[d], y + DY[d]);
            int near = 0;
            for (int i = G.tri_n0; i < G.nthing; i++) near |= abs(G.thing[i].cx - x) + abs(G.thing[i].cy - y) < (pass == 2 ? 1 : 2);
            if ((!lanes && !apass(x, y)) || near) continue;
            *ox = x; *oy = y;
            return 1;
        }
    return 0;
}
static Mob *tri_spawn(int type, float from_you) {                  /* one of the trial's, out of the room's floor */
    const Room *R = &G.rooms[G.tri_room];
    for (int tries = 0; tries < 200; tries++) {
        int x = R->x + rnd(R->w), y = R->y + rnd(R->h);
        if (!hz_floor(x, y) || occupied(x, y, NULL) || hypotf(x + 0.5f - G.px, y + 0.5f - G.py) < from_you) continue;
        Mob *m = spawn_mob(x, y, type);
        if (!m) return NULL;
        m->tri = 1; m->alert = 999.0f; m->home = -1;
        m->dmgk *= 0.7f;                                           /* a room you can't leave: its creatures strike lighter */
        sfx_at(SFX_DIRT, m->x, m->y, 0.6f);
        return m;
    }
    return NULL;
}
static int tri_thing(int role, int spr, const char *name, float from_you) {   /* one of its things, standing in the room */
    int x, y;
    if (!tri_spot(&x, &y, from_you)) return -1;
    int i = thing_new(role, x + 0.5f, y + 0.5f, spr);
    if (i < 0) return -1;
    G.thing[i].tri = 1;
    snprintf(G.thing[i].name, sizeof G.thing[i].name, "%s", name);
    return i;
}
/* the room's walls take on the trial's look while it lasts, and give it back after */
static void tri_turn(int on) {
    const Room *R = &G.rooms[G.tri_room];
    if (!on) {
        for (int k = 0; k < G.tri_nface; k++) { Cell *c = &G.map[G.tri_fy[k]][G.tri_fx[k]]; c->face[G.tri_fd[k]] = G.tri_face[k]; c->fover[G.tri_fd[k]] = G.tri_fover[k]; }
        G.tri_nface = 0;
        return;
    }
    static const uint8_t MAT[4] = { M1_BONES, M2_FLOW, M3_FILL, M4_CRYSTAL };
    int mat = MAT[crawl_set(G.level)];
    G.tri_nface = 0;
    for (int y = R->y - 1; y <= R->y + R->h; y++)
        for (int x = R->x - 1; x <= R->x + R->w; x++) {
            Cell *c = cell(x, y);
            if (c->kind != K_WALL || !G.faced) continue;
            for (int d = 0; d < 4; d++) {
                int nx = x + DX[d], ny = y + DY[d];
                if (!in_room(R, nx, ny) || cell(nx, ny)->kind == K_WALL || G.tri_nface >= (int)(sizeof G.tri_fx / sizeof G.tri_fx[0])) continue;
                int k = G.tri_nface++;
                G.tri_fx[k] = (int16_t)x; G.tri_fy[k] = (int16_t)y; G.tri_fd[k] = (uint8_t)d; G.tri_face[k] = c->face[d]; G.tri_fover[k] = c->fover[d];
                c->face[d] = (uint8_t)T_MATV(mat, (x + y) & 1); c->fover[d] = 0;
            }
        }
}
/* the room's floor: water, while the Catacombs' trials run (0 gives it back) */
static void tri_floor(int flo) {
    const Room *R = &G.rooms[G.tri_room];
    if (R->w * R->h > (int)sizeof G.tri_flo) return;
    for (int y = 0; y < R->h; y++)
        for (int x = 0; x < R->w; x++) {
            Cell *c = &G.map[R->y + y][R->x + x];
            if (c->kind != K_FLOOR) continue;
            if (flo && !G.tri_floored) G.tri_flo[y * R->w + x] = c->flo;
            c->flo = flo ? (uint8_t)flo : G.tri_flo[y * R->w + x];
        }
    G.tri_floored = flo != 0;
}
/* the dark of the Saints' Lamps: the room's light all but gone but for the lamps lit and your own glow (in build_lightmap) */
static void tri_dark(void) {
    if (G.tri != TRI_LAMPS || G.tri_state != TRS_ON || PK.depth || G.shop) return;
    const Room *R = &G.rooms[G.tri_room];
    for (int ty = imax(0, (R->y - 1) * LMR); ty < imin(LMN, (R->y + R->h + 1) * LMR); ty++)
        for (int tx = imax(0, (R->x - 1) * LMR); tx < imin(LMN, (R->x + R->w + 1) * LMR); tx++) G.lm[ty * LMN + tx] *= 0.18f;
}
/* the trial's water: a step higher every fifteen seconds, a tenth slower each (it never drowns you) */
static float tri_wading(void) {
    if ((G.tri != TRI_WATER && G.tri != TRI_SLUICES) || G.tri_state != TRS_ON || !in_trial((int)G.px, (int)G.py)) return 1.0f;
    return 1.0f - 0.1f * (float)imin(G.tri_a[0], 4);
}
/* the next strip of the room from its far end (from the door), a column or row of cells, in the direction it runs */
static int tri_strip(int k, int8_t *cx, int8_t *cy, int max) {
    const Room *R = &G.rooms[G.tri_room];
    const Door *dr = &G.doors[G.tri_door];
    int along_x = R->w >= R->h, n = 0, len = along_x ? R->w : R->h;
    if (k < 0 || k >= len) return 0;
    int far_lo = along_x ? abs(dr->x - R->x) > abs(dr->x - (R->x + R->w - 1)) : abs(dr->y - R->y) > abs(dr->y - (R->y + R->h - 1));
    int s = far_lo ? k : len - 1 - k;                              /* from the end away from the door */
    for (int j = 0; j < (along_x ? R->h : R->w) && n < max; j++) {
        int x = along_x ? R->x + s : R->x + j, y = along_x ? R->y + j : R->y + s;
        if (G.map[y][x].kind != K_FLOOR) continue;
        cx[n] = (int8_t)x; cy[n] = (int8_t)y; n++;
    }
    return n;
}
static int tri_strips(void) { const Room *R = &G.rooms[G.tri_room]; return R->w >= R->h ? R->w : R->h; }
static int tri_strip_of(int x, int y) {                            /* which strip a cell lies in, counted from the far end */
    for (int k = 0; k < tri_strips(); k++) {
        int8_t cx[24], cy[24];
        int n = tri_strip(k, cx, cy, 24);
        for (int i = 0; i < n; i++) if (cx[i] == x && cy[i] == y) return k;
    }
    return -1;
}
static void add_burn(float x, float y, int foe) {                  /* magma on the floor a while (the Molten Rise) */
    for (int i = 0; i < MAXBURNS; i++)
        if (!G.burns[i].alive) { G.burns[i] = (Burn){ 1, x, y, 0.0f, (4.0f + 0.8f * (float)G.level) * 0.06f, foe, 8.0f }; return; }
}
/* the creatures the trial lets loose, by kind */
static void tri_wave(void) {
    int lv = G.level;
    switch (G.tri) {
    case TRI_OSSUARY: {                                            /* the bones in the walls come down, a wave at a time; the half
                                                                      of them that stand up whole grapple rather than throw */
        int n = 3 + G.tri_a[0] / 2 + lv / 12;
        for (int k = 0; k < n; k++) tri_spawn(k & 1 ? MOB_SHAMBLER : k % 4 == 2 && may_spawn(MOB_SHIELD) ? MOB_SHIELD : MOB_SKEL, 3.0f);
        G.tri_a[0]++; G.tri_f[0] = 0.0f;
        snd_sfx_ex(SFX_BONES, 0.9f, 0.5f);
        break;
    }
    case TRI_VIGIL: case TRI_RITES: {                              /* the dead come to snuff the candles; or wake about you */
        Mob *m = tri_spawn(MOB_SKEL, 3.0f);
        if (m && G.tri == TRI_VIGIL) m->tri = 2;
        G.tri_f[0] = 0.0f;
        break;
    }
    case TRI_TOLL:
        for (int k = 0; k < 3 + lv / 5; k++) tri_spawn(MOB_SKEL, 3.0f);
        break;
    case TRI_WATER: case TRI_SLUICES:                              /* the drowned, out of the water */
        for (int k = 0; k < (G.tri == TRI_WATER ? 3 : 1); k++) { Mob *m = tri_spawn(MOB_DROWNED, 3.0f); if (m) { m->sub = 2; m->sub_t = 0.0f; } }
        G.tri_f[2] = 0.0f;
        break;
    case TRI_LAMPS:                                                /* shades, seen only in light */
        for (int k = 0; k < 4; k++) tri_spawn(MOB_SHADE, 4.0f);
        break;
    case TRI_FEAST: {                                              /* ghouls at their meal on the heap */
        for (int k = 0; k < 3; k++) {
            Mob *m = tri_spawn(MOB_GHOUL, 3.0f);
            if (!m) break;
            m->x = G.tri_f[3] + (rndf() - 0.5f) * 0.8f; m->y = G.tri_f[4] + (rndf() - 0.5f) * 0.8f; m->cx = m->tx = (int)m->x; m->cy = m->ty = (int)m->y;
            m->sub = 4; m->alert = 0.0f;                           /* feeding: mending, and taking double */
        }
        break;
    }
    case TRI_RITE: {                                               /* four deacons in the corners, each with a heap to chant over */
        const Room *R = &G.rooms[G.tri_room];
        for (int c = 0; c < 4; c++) {
            int x = c & 1 ? R->x + R->w - 1 : R->x, y = c & 2 ? R->y + R->h - 1 : R->y;
            if (!hz_floor(x, y)) continue;
            Mob *m = spawn_mob(x, y, MOB_DEACON);
            if (!m) break;
            m->tri = 1; m->alert = 999.0f; m->home = -1; m->dmgk *= 0.7f; m->raised = 3;   /* one each, then only its chant */
            m->hp = m->maxhp = m->maxhp * 4.0f;
            add_deco(x + 0.5f + (c & 1 ? -0.6f : 0.6f), y + 0.5f, SP_BONES2);
        }
        break;
    }
    case TRI_FIREDAMP:
        for (int k = 0; k < 3; k++) tri_spawn(MOB_BAT, 3.0f);
        break;
    case TRI_MOLTEN:                                               /* the imps that feed the vents */
        for (int k = 0; k < 3; k++) {
            Mob *m = tri_spawn(MOB_IMP, 3.5f);
            if (!m) break;
            G.tri_a[4 + k] = (int)(m - G.mobs) + 1;
            G.tri_f[4 + (k & 3)] = 0.0f;
            m->hp = m->maxhp = m->maxhp * 2.5f;
        }
        for (int k = 0; k < 2; k++) { Mob *m = tri_spawn(MOB_IMP, 3.5f); if (m) m->hp = m->maxhp = m->maxhp * 2.5f; }
        break;
    case TRI_DARK:                                                 /* a shade of the Wraith, after the braziers */
        { Mob *m = tri_spawn(MOB_SHADE, 4.0f); if (m) m->tri = 2; }
        break;
    case TRI_COURT: {                                              /* black-rank dead about a crowned one, who keeps behind them */
        for (int k = 0; k < 3; k++) { Mob *m = tri_spawn(MOB_SKEL, 3.0f); if (m) { set_rank(m, RK_V); m->hp = m->maxhp = m->maxhp * 0.7f; m->dmgk *= 0.7f; } }
        Mob *c = tri_spawn(MOB_DEACON, 5.0f);
        if (!c) c = tri_spawn(MOB_DEACON, 2.0f);
        if (c) { c->tri = 3; set_rank(c, RK_GILDED); c->raised = 0; c->hp = c->maxhp = c->maxhp * 5.0f; add_deco(c->x + 0.6f, c->y, SP_BONES2); add_deco(c->x - 0.6f, c->y, SP_BONES2); }
        else G.tri_a[1] = 1;                                       /* no room for it: the court has nobody to raise them */
        break;
    }
    case TRI_CAVEIN: case TRI_SINGING:
        for (int k = 0; k < (G.tri == TRI_CAVEIN ? 2 : 1); k++) tri_spawn(G.tri == TRI_CAVEIN ? MOB_SKEL : MOB_IMP, 3.5f);
        break;
    }
}
static void tri_begin(void) {
    Door *d = &G.doors[G.tri_door];
    d->locked = TRIALED; d->opening = 0; d->idle = 99.0f;
    G.tri_state = TRS_ON; G.tri_t = TRI_TIME; G.tri_end = 0.0f; G.tri_n0 = G.nthing; G.work_i = 0;
    memset(G.tri_a, 0, sizeof G.tri_a); memset(G.tri_f, 0, sizeof G.tri_f);
    tri_turn(1);
    snd_set_trial(1);
    say("%s: %s", TRI_NAME[G.tri], TRI_TASK[G.tri]);
    snd_sfx_ex(SFX_DOOR, 0.9f, 0.5f); snd_sfx_ex(SFX_STINGER, 0.7f, 0.5f);
    if (G.player) rumble(0.5f, 0.3f, 0.4f);
    switch (G.tri) {
    case TRI_OSSUARY: case TRI_TOLL: tri_wave(); break;
    case TRI_VIGIL: {                                              /* three candles on a bier, burning */
        int b = tri_thing(TR_READ, SP_TR_BIER, "THE BIER", 0.0f);
        if (b < 0) break;
        G.thing[b].scale = 0.9f;
        snprintf(G.thing[b].text, sizeof G.thing[b].text, "KEEP ONE OF THE CANDLES BURNING TILL THE SAND RUNS OUT.");
        for (int k = 0; k < 3; k++) {
            int i = thing_new(TR_LIGHT, G.thing[b].x - 0.3f + 0.3f * (float)k, G.thing[b].y, SP_PZ_CANDLE0);
            if (i < 0) break;
            Thing *t = &G.thing[i];
            t->tri = 1; t->frames = 2; t->state = 1; t->z = 0.22f; t->scale = 0.55f; t->cx = G.thing[b].cx; t->cy = G.thing[b].cy;
            snprintf(t->name, sizeof t->name, "A CANDLE ON THE BIER");
            G.tri_a[1 + k] = i;
        }
        tri_wave(); G.tri_f[0] = 6.0f;
        break;
    }
    case TRI_RITES: {                                              /* a bier; four relics about the room */
        int b = tri_thing(TR_PLACE, SP_TR_BIER, "THE BIER", 0.0f);
        if (b < 0) break;
        G.thing[b].cap = 4; G.thing[b].hold = 0.24f; G.thing[b].scale = 0.9f; G.thing[b].grp = 0; G.tri_a[1] = b;
        for (int k = 0; k < 4; k++) {
            int i = tri_thing(TR_ITEM, SP_PZ_CASKET, "A RELIC", 1.5f);
            if (i >= 0) { G.thing[i].scale = 0.7f; G.tri_a[2]++; }
        }
        tri_wave();
        break;
    }
    case TRI_WAGER: {                                              /* one knight, two ranks above the level's best, alone */
        int best = 0;
        for (int i = 0; i < MAXMOBS; i++) if (G.mobs[i].alive && !G.mobs[i].tri && KINDS[G.mobs[i].type].family >= 0) best = imax(best, imin(G.mobs[i].rank, RK_V));
        Mob *m = tri_spawn(MOB_KNIGHT, 4.0f);
        if (m) { set_rank(m, imin(RK_V, best + 1)); m->hp = m->maxhp = m->maxhp * 3.0f; }
        break;
    }
    case TRI_WATER: tri_wave(); break;
    case TRI_BROOD: {                                              /* sacs on the walls, each ripening on its own time */
        for (int k = 0; k < 4; k++) { Mob *m = tri_spawn(MOB_SAC, 2.0f); if (m) m->sub_t = -2.0f - 4.0f * rndf(); }   /* (a sac hatches at eight) */
        break;
    }
    case TRI_SLUICES:
        for (int k = 0; k < 5; k++) {
            int i = tri_thing(TR_TURN, SP_PZ_WHEEL, "A SLUICE WHEEL", 1.5f);
            if (i >= 0) { G.thing[i].slow = 35; G.thing[i].nstate = 2; G.thing[i].scale = 0.8f; }   /* a long heave */
        }
        tri_wave();
        break;
    case TRI_LAMPS:
        for (int k = 0; k < 5; k++) {
            int i = tri_thing(TR_LIGHT, SP_PZ_LAMP0, "A SAINT'S LAMP", 1.5f);
            if (i >= 0) { G.thing[i].frames = 2; G.thing[i].scale = 0.8f; G.thing[i].slow = 15; }
        }
        tri_wave();
        break;
    case TRI_CAVEIN:
        for (int k = 0; k < 4; k++) {
            int i = tri_thing(TR_TURN, SP_TR_PROP0, "A LOOSE PROP", 1.5f);
            if (i < 0) continue;
            G.thing[i].slow = 30; G.thing[i].nstate = 2; G.thing[i].frames = 2;   /* hammered home */
            if (tri_strip_of(G.thing[i].cx, G.thing[i].cy) < tri_strips() / 3) {   /* not where the roof comes first */
                int x, y;
                for (int t = 0; t < 40; t++) if (tri_spot(&x, &y, 1.0f) && tri_strip_of(x, y) >= tri_strips() / 3) { G.thing[i].x = x + 0.5f; G.thing[i].y = y + 0.5f; G.thing[i].cx = x; G.thing[i].cy = y; break; }
            }
        }
        tri_wave();
        break;
    case TRI_FEAST: {
        int x, y;
        if (tri_spot(&x, &y, 3.0f)) { G.tri_f[3] = x + 0.5f; G.tri_f[4] = y + 0.5f; add_deco(x + 0.5f, y + 0.5f, SP_BONES2); add_deco(x + 0.3f, y + 0.7f, SP_SKULLPILE); }
        else { G.tri_f[3] = G.rooms[G.tri_room].x + G.rooms[G.tri_room].w * 0.5f; G.tri_f[4] = G.rooms[G.tri_room].y + G.rooms[G.tri_room].h * 0.5f; }
        tri_wave();
        break;
    }
    case TRI_RITE: case TRI_MOLTEN: case TRI_COURT: tri_wave(); break;
    case TRI_FIREDAMP: {                                           /* the vents at the high end, away from where the gas comes in */
        for (int k = 0; k < 3; k++) {
            int i = tri_thing(TR_LEVER, SP_PZ_VENT0, "A VENT", 1.5f);
            if (i < 0) continue;
            G.thing[i].frames = 2; G.thing[i].slow = 50; G.thing[i].scale = 0.8f;   /* rusted shut */
            int s = tri_strip_of(G.thing[i].cx, G.thing[i].cy);   /* not in the first strips the gas takes */
            if (s >= 0 && s < tri_strips() / 2) { int x, y; for (int t = 0; t < 30; t++) if (tri_spot(&x, &y, 1.0f) && tri_strip_of(x, y) >= tri_strips() / 2) { G.thing[i].x = x + 0.5f; G.thing[i].y = y + 0.5f; G.thing[i].cx = x; G.thing[i].cy = y; break; } }
        }
        tri_wave();
        break;
    }
    case TRI_DARK:
        for (int k = 0; k < 6; k++) {
            int i = tri_thing(TR_LIGHT, SP_TR_BRAZ0, "A BRAZIER", 1.0f);
            if (i >= 0) { G.thing[i].frames = 2; G.thing[i].state = 1; G.thing[i].slow = 1; G.thing[i].scale = 0.8f; }
        }
        tri_wave();
        break;
    case TRI_SINGING:
        for (int k = 0; k < 5; k++) {
            int i = tri_thing(TR_STRIKE, SP_PZ_GEODE, "A CRYSTAL", 1.5f);
            if (i >= 0) { G.thing[i].frames = 2; G.thing[i].val = k; G.thing[i].scale = 0.8f; }
        }
        G.tri_a[2] = -1; G.tri_f[2] = 2.0f;                       /* none lit yet: the first in two seconds */
        tri_wave();
        break;
    }
    if (G.tri == TRI_TOLL)
        for (int k = 0; k < 4; k++) {
            int i = tri_thing(TR_STRIKE, SP_BELL, "A SMALL BELL", 1.5f);
            if (i >= 0) { G.thing[i].scale = 0.55f; G.thing[i].z = 0.25f; G.thing[i].val = k; }
        }
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT trial_begin %d %s level %d by %s t=%.2f\n", G.tri, TRI_NAME[G.tri], G.level, G.player ? "player" : "autopilot", G.t);
#endif
}
#ifdef CRAWL_DEBUG
static int tri_done(int *n) {                                      /* of the trial's things, those done, and how many */
    int d = 0;
    *n = 0;
    for (int i = G.tri_n0; i < G.nthing; i++) if (G.thing[i].role != TR_READ && G.thing[i].role != TR_PLACE) { (*n)++; d += G.thing[i].state || G.thing[i].in >= 0; }
    return d;
}
#endif
/* 1.0.0b18.1: a trial won leaves its reward in its room, in place of the restoring it gave at once: a hoard worth three of
 * the level's chests, glowing, kept out of the level's treasure count, and a draught that makes you whole, which waits while
 * you are whole. Both are marked in Item.color, which only keys and scrip read. */
static int tri_reward_at(int type) {                               /* a won trial's hoard or draught still lying, or -1 */
    for (int i = 0; i < G.nitems; i++) if (G.items[i].alive && G.items[i].type == type && G.items[i].color == TRI_REWARD) return i;
    return -1;
}
static int tri_reward_spot(int nx, int ny, int *ox, int *oy) {     /* free floor near the room's middle, in sight of its door,
                                                                      not under the caster; the second near the first (nx >= 0) */
    const Room *R = &G.rooms[G.tri_room];
    const Door *d = &G.doors[G.tri_door];
    int ix = 2 * d->x - G.tri_wx, iy = 2 * d->y - G.tri_wy;        /* the cell inside its door */
    float cx = (float)R->x + (float)R->w * 0.5f, cy = (float)R->y + (float)R->h * 0.5f;
    for (int pass = 0; pass < 2; pass++) {                         /* in sight of the door first, then anywhere in it */
        int bx = -1, by = -1; float bd = 1e9f;
        for (int y = R->y; y < R->y + R->h; y++)
            for (int x = R->x; x < R->x + R->w; x++) {
                if (!free_floor(x, y) || hypotf(x + 0.5f - G.px, y + 0.5f - G.py) < 1.4f) continue;
                if (pass == 0 && !line_clear(ix + 0.5f, iy + 0.5f, x + 0.5f, y + 0.5f)) continue;
                float s = nx >= 0 ? hypotf((float)(x - nx), (float)(y - ny)) : hypotf(x + 0.5f - cx, y + 0.5f - cy);
                if (s < bd) { bd = s; bx = x; by = y; }
            }
        if (bx >= 0) { *ox = bx; *oy = by; return 1; }
    }
    return 0;
}
static void tri_reward(void) {
    int hx, hy, px, py;
    if (!tri_reward_spot(-1, -1, &hx, &hy)) { hx = (int)G.px; hy = (int)G.py; }   /* no room for it: at your feet */
    Item *h = add_item(hx, hy, IT_CHEST);
    if (h) {
        h->secret = 1; h->seen = 1; h->color = TRI_REWARD; h->value = 3 * treasure_value(IT_CHEST);
        G.items_total--; G.value_total -= treasure_value(IT_CHEST);   /* out of the level's count, as a secret's loot is */
    }
    if (!tri_reward_spot(hx, hy, &px, &py)) { px = (int)G.px; py = (int)G.py; }
    Item *p = add_item(px, py, IT_POTION);
    if (p) { p->secret = 1; p->seen = 1; p->color = TRI_REWARD; }
    sfx_at(SFX_SHIMMER, hx + 0.5f, hy + 0.5f, 0.9f); sfx_at(SFX_DEEPCHIME, hx + 0.5f, hy + 0.5f, 0.6f);
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT trial_reward gold %d at (%d,%d) draught at (%d,%d) caster (%.1f,%.1f) t=%.2f\n", h ? h->value : 0, hx, hy, px, py, G.px, G.py, G.t);
#endif
}
static void tri_end(int won) {
#ifdef CRAWL_DEBUG
    { int n, d = tri_done(&n); fprintf(stderr, "TRIAL end %s: %d of %d done, %d standing, hp %d/%d, hold %.1f at (%.1f,%.1f)", won ? "won" : "lost", d, n, tri_alive(0), G.hp, G.max_hp, G.tri_hold, G.px, G.py);
      for (int i = 0; i < MAXMOBS; i++) { const Mob *m = &G.mobs[i]; if (m->alive && m->tri && m->state == MS_WALK) fprintf(stderr, " [%d at (%.1f,%.1f) hp %.0f/%.0f %s%s]", m->type, m->x, m->y, m->hp, m->maxhp,
          in_trial(m->cx, m->cy) ? "" : "OUT ", line_clear(G.px, G.py, m->x, m->y) ? "seen" : "hid"); }
      fprintf(stderr, "\n"); }
#endif
    G.tri_state = won ? TRS_WON : TRS_LOST; G.tri_end = 0.0f; G.work_i = 0;
    Door *d = &G.doors[G.tri_door];
    d->locked = 0; d->opening = 1;
    tri_turn(0);
    if (G.tri_floored) tri_floor(0);
    for (int i = 0; i < MAXHAZ; i++) if (G.haz[i].alive && (G.haz[i].kind == HZ_DAMP || G.haz[i].kind == HZ_GLASS) && in_trial((int)G.haz[i].x, (int)G.haz[i].y)) G.haz[i].alive = 0;   /* its own air and glass */
    snd_set_trial(0);
    if (G.carry > G.tri_n0) G.carry = 0;                           /* what it brought goes with it */
    G.nthing = G.tri_n0;
    for (int i = 0; i < MAXMOBS; i++) {                            /* what is left of its dead crumbles */
        Mob *m = &G.mobs[i];
        if (!m->alive || !m->tri || m->state != MS_WALK) continue;
        m->state = MS_DIE; m->t = 0.0f;
        if (!KIND_IS(m, KF_UNCOUNTED)) G.kills_total--;
        sfx_at(SFX_BONES, m->x, m->y, 0.5f);
    }
    if (won) {                                                     /* the record: a trial won, and its kind */
        if (RUN.deed_tri < 65535) RUN.deed_tri++;
        RUN.tri_kinds |= 1u << G.tri;
        char d[32];
        for (int k = 0; k < 3; k++) if (RUN.deed_tri >= TRI_STEP[k]) { snprintf(d, sizeof d, "%d TRIAL%s WON", TRI_STEP[k], TRI_STEP[k] > 1 ? "S" : ""); ach_earn(ACH_DEED0 + 3 + k, SP_TRLIGHT0, -1, 0, TRI_ACH[k], d); }
        for (int r = 0; r < 4; r++) {
            int all = 1;
            for (int k = 0; k < 5 && TRI_RUN[r][k] >= 0; k++) all &= (int)(RUN.tri_kinds >> TRI_RUN[r][k] & 1);
            if (all) ach_earn(ACH_DEED0 + 6 + r, SP_TRLIGHT0 + r, -1, 0, TRI_RUN_ACH[r], "EVERY ONE WON");
        }
        G.tri_boon = 1;
        tri_reward();                                              /* its hoard and its draught, in place of health at once (1.0.0b18.1) */
        RUN.gear.bones = imax(RUN.gear.bones, bones_max());
        float half = vessel() * 0.5f;
        if (spirit() < half) { RUN.souls = (int)half; RUN.soul_frac = half - (float)RUN.souls; }
        say("THE TRIAL IS WON: THE TRIAL'S BOON, A FIFTH MORE FROM WHAT YOU SLAY");
        snd_sfx_ex(SFX_SHATTER, 1.0f, 0.5f); snd_sfx_ex(SFX_FANFARE, 0.6f, 0.5f);
        show_bar();
    } else {
        if (RUN.deed_lost < 65535) RUN.deed_lost++;
        G.tri_boon = -1;
        say("THE SAND HAS RUN OUT: THE TRIAL'S CURSE, A FIFTH LESS OF EVERYTHING");
        snd_sfx_ex(SFX_SHATTER, 0.5f, 0.5f); snd_sfx_ex(SFX_GHOST_DIE, 0.8f, 0.5f);
    }
    if (G.player) rumble(won ? 0.4f : 0.7f, 0.4f, 0.5f);
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT trial_%s %d %s level %d left %.1f by %s t=%.2f\n", won ? "won" : "lost", G.tri, TRI_NAME[G.tri], G.level, G.tri_t,
            G.player ? "player" : "autopilot", G.t);
#endif
}
/* what the trial's things answer */
static void tri_touched(int i, int how) {
    Thing *t = &G.thing[i];
    if (G.tri_state != TRS_ON) return;
    switch (G.tri) {
    case TRI_VIGIL: if (how == TU_LIGHT && t->state) sfx_at(SFX_IGNITE, t->x, t->y, 0.6f); break;
    case TRI_SLUICES: case TRI_CAVEIN:                            /* turned, or braced: it stays so */
        if (how == TU_TURN) { t->state = 1; t->nstate = 1; sfx_at(G.tri == TRI_CAVEIN ? SFX_KNOCK : SFX_UNLOCK, t->x, t->y, 0.8f); }
        break;
    case TRI_FIREDAMP:
        if (how == TU_PULL) { t->state = 1; sfx_at(SFX_HISS, t->x, t->y, 0.8f); }
        break;
    case TRI_LAMPS: case TRI_DARK:
        if (how == TU_LIGHT && !t->state) t->state = 1;            /* lit stays lit: E doesn't put it out */
        break;
    case TRI_SINGING:
        if (how != TU_STRIKE) break;
        if (G.tri_a[2] == i) { G.tri_a[3]++; t->state = 0; G.tri_a[2] = -1; G.tri_f[2] = 1.2f; snd_tone(1, 76 + t->val * 2, 0.8f, 0.5f); }   /* in time */
        else sfx_at(SFX_KNOCK, t->x, t->y, 0.5f);
        break;
    case TRI_TOLL:
        if (how != TU_STRIKE || t->state) break;
        if (G.tri_f[1] <= 0.0f) {
            if (!G.player) { G.tri_hold = TOLL_T + 0.2f - G.tri_f[0]; break; }   /* the dream waits with its hand on it */
            sfx_at(SFX_KNOCK, t->x, t->y, 0.6f); say("IT ANSWERS ONLY WHILE THE GREAT BELL TOLLS");
            break;
        }
        t->state = 1; snd_tone(0, 72 + t->val * 3, 0.7f, 0.5f);   /* rung under the toll: it answers */
        break;
    }
}
static void tri_run(float dt) {
    switch (G.tri) {
    case TRI_OSSUARY:
        G.tri_f[0] += dt;
        if (G.tri_a[0] < 4 && ((tri_alive(0) <= 1 && G.tri_f[0] > 5.0f) || G.tri_f[0] > 15.0f)) tri_wave();
        if (G.tri_a[0] >= 4 && !tri_alive(0)) tri_end(1);
        break;
    case TRI_VIGIL: {
        int lit = 0;
        for (int k = 0; k < 3; k++) lit += G.tri_a[1 + k] < G.nthing && G.thing[G.tri_a[1 + k]].state;
        G.tri_f[1] = lit ? 0.0f : G.tri_f[1] + dt;
        if (G.tri_f[1] > 6.0f) { tri_end(0); break; }              /* all of them dark too long */
        if ((G.tri_f[0] += dt) > 8.0f && tri_alive(2) < 3) tri_wave();
        for (int i = 0; i < MAXMOBS; i++) {                        /* the dead go for the lit candles; near you, for you */
            Mob *m = &G.mobs[i];
            if (!m->alive || m->state != MS_WALK || m->tri != 2) continue;
            int best = -1; float bd = 1e9f;
            for (int k = 0; k < 3; k++) {
                const Thing *c = &G.thing[G.tri_a[1 + k]];
                float dd = hypotf(c->x - m->x, c->y - m->y);
                if (c->state && dd < bd) { bd = dd; best = G.tri_a[1 + k]; }
            }
            m->lure = best >= 0 && hypotf(G.px - m->x, G.py - m->y) > 2.2f;
            if (!m->lure) continue;
            m->lx = G.thing[best].x; m->ly = G.thing[best].y;
            if (bd >= 1.1f) m->sub_t = 0.0f;                       /* a few seconds bent over it to put it out */
            else if ((m->sub_t += dt) > 2.5f) { G.thing[best].state = 0; m->sub_t = 0.0f; sfx_at(SFX_WHOOSH, m->lx, m->ly, 0.6f); }
        }
        break;
    }
    case TRI_RITES:
        if ((G.tri_f[0] += dt) > 12.0f && tri_alive(0) < 2) tri_wave();
        if (pile_count(G.tri_a[1]) >= G.tri_a[2]) tri_end(1);   /* every relic laid out */
        break;
    case TRI_WAGER:
        if (!tri_alive(0)) tri_end(1);
        break;
    case TRI_TOLL: {
        G.tri_f[1] = fmaxf(0.0f, G.tri_f[1] - dt);                /* the toll hanging in the air */
        if ((G.tri_f[0] += dt) >= TOLL_T) {                        /* the great bell: the dead stand still, the small bells answer */
            G.tri_f[0] = 0.0f; G.tri_f[1] = 3.5f;
            snd_tone(0, 33, 1.0f, 0.5f); snd_sfx_ex(SFX_DEEPCHIME, 0.8f, 0.5f);
            for (int i = 0; i < MAXMOBS; i++) if (G.mobs[i].alive && G.mobs[i].tri) G.mobs[i].stagger = fmaxf(G.mobs[i].stagger, 2.0f);
        }
        int rung = 0, bells = 0;
        for (int i = G.tri_n0; i < G.nthing; i++) if (G.thing[i].role == TR_STRIKE) { bells++; rung += G.thing[i].state; }
        if (rung >= bells) tri_end(1);                             /* (as many as found room: four, nearly always) */
        break;
    }
    case TRI_WATER: case TRI_SLUICES: {
        if ((G.tri_f[0] += dt) >= 14.0f && G.tri_a[0] < 4) {     /* the water: a step higher */
            G.tri_f[0] = 0.0f; G.tri_a[0]++;
            if (G.tri_a[0] == 1) tri_floor(T_FLOORS + FL2_WATER);
            if (G.tri == TRI_WATER && G.tri_a[0] <= 3) tri_wave();     /* more of the drowned with the water */
            snd_sfx_ex(SFX_SPLASH, 1.0f, 0.5f); snd_sfx_ex(SFX_DRIP, 0.8f, 0.5f);
            if (G.player && G.tri_a[0] < 4) say("THE WATER RISES");
        }
        if (G.tri == TRI_WATER) { if (!tri_alive(0) && G.tri_a[0] >= 3) tri_end(1); break; }   /* (the last of them come at forty-five) */
        if ((G.tri_f[2] += dt) > 12.0f && tri_alive(0) < 2) tri_wave();
        int turned = 0, n = 0;
        for (int i = G.tri_n0; i < G.nthing; i++) if (G.thing[i].role == TR_TURN) { n++; turned += G.thing[i].state; }
        if (n && turned >= n) tri_end(1);
        break;
    }
    case TRI_BROOD:                                                /* more sacs on the walls every fifteen seconds, to forty-five */
        if ((G.tri_f[0] += dt) >= 13.0f && G.tri_a[0] < 3) {
            G.tri_f[0] = 0.0f; G.tri_a[0]++;
            for (int k = 0; k < 3; k++) { Mob *m = tri_spawn(MOB_SAC, 2.0f); if (m) m->sub_t = -2.0f - 4.0f * rndf(); }
            sfx_at(SFX_SKITTER, G.px, G.py, 0.7f);
        }
        if (!tri_alive(0) && G.tri_a[0] >= 3) tri_end(1);         /* the sacs, and whatever has hatched from them */
        for (int i = 0; i < MAXMOBS; i++) if (G.mobs[i].alive && G.mobs[i].type == MOB_LING && !G.mobs[i].tri && in_trial(G.mobs[i].cx, G.mobs[i].cy)) G.mobs[i].tri = 1;
        break;
    case TRI_LAMPS: {                                              /* all five burning at once; the shades put them out */
        int lit = 0, n = 0;
        for (int i = G.tri_n0; i < G.nthing; i++) if (G.thing[i].role == TR_LIGHT) { n++; lit += G.thing[i].state; }
        if (n && lit >= n) { tri_end(1); break; }
        for (int i = 0; i < MAXMOBS; i++) {
            Mob *m = &G.mobs[i];
            if (!m->alive || m->state != MS_WALK || !m->tri) continue;
            int best = -1; float bd = 1e9f;
            for (int j = G.tri_n0; j < G.nthing; j++) { const Thing *c = &G.thing[j]; float dd = hypotf(c->x - m->x, c->y - m->y); if (c->role == TR_LIGHT && c->state && dd < bd) { bd = dd; best = j; } }
            m->lure = best >= 0 && hypotf(G.px - m->x, G.py - m->y) > 2.2f;
            if (!m->lure) continue;
            m->lx = G.thing[best].x; m->ly = G.thing[best].y;
            if (bd >= 1.1f) m->sub_t = 0.0f;
            else if ((m->sub_t += dt) > 1.8f) { G.thing[best].state = 0; m->sub_t = 0.0f; sfx_at(SFX_WHOOSH, m->lx, m->ly, 0.6f); }
        }
        break;
    }
    case TRI_CAVEIN: {                                             /* the roof comes down from the far end, a strip at a time */
        int strips = tri_strips(), braced = 0, n = 0;
        float every = (TRI_TIME - 15.0f) / (float)imax(2, strips);   /* (it holds fifteen seconds, then comes down) */
        if ((G.tri_f[0] += dt) >= (G.tri_a[0] ? every : 15.0f) && G.tri_a[0] < strips) {
            G.tri_f[0] = 0.0f;
            int8_t cx[24], cy[24];
            int m = tri_strip(G.tri_a[0], cx, cy, 24);
            for (int i = 0; i < m; i++) if (rnd(3) == 0 || i == m / 2) hz_rock(cx[i] + 0.5f, cy[i] + 0.5f, 0);   /* rocks where the roof is coming */
            G.tri_a[0]++;
            snd_sfx_ex(SFX_RUMBLE, 0.7f, 0.5f);
            for (int i = G.tri_n0; i < G.nthing; i++)              /* it reached a prop not braced: the roof has won */
                if (G.thing[i].role == TR_TURN && !G.thing[i].state && tri_strip_of(G.thing[i].cx, G.thing[i].cy) < G.tri_a[0]) { tri_end(0); return; }
        }
        for (int i = G.tri_n0; i < G.nthing; i++) if (G.thing[i].role == TR_TURN) { n++; braced += G.thing[i].state; }
        if (n && braced >= n) tri_end(1);
        break;
    }
    case TRI_FEAST:                                                /* more come to the feast */
        if ((G.tri_f[0] += dt) >= 18.0f && G.tri_a[0] < 2) { G.tri_f[0] = 0.0f; G.tri_a[0]++; tri_wave(); }
        if (!tri_alive(0) && G.tri_a[0] >= 2) tri_end(1);
        break;
    case TRI_RITE:                                                 /* the deacons: when they are silent, what they raised falls */
        {
            int deacons = 0;
            for (int i = 0; i < MAXMOBS; i++) deacons += G.mobs[i].alive && G.mobs[i].state == MS_WALK && G.mobs[i].tri && G.mobs[i].type == MOB_DEACON;
            for (int i = 0; i < MAXMOBS; i++) if (G.mobs[i].alive && G.mobs[i].raised == 0 && !G.mobs[i].tri && G.mobs[i].type == MOB_SKEL && in_trial(G.mobs[i].cx, G.mobs[i].cy)) G.mobs[i].tri = 1;
            if (!deacons) tri_end(1);
        }
        break;
    case TRI_FIREDAMP: {                                           /* the gas in from the low end, a strip at a time */
        if ((G.tri_f[0] += dt) >= 6.0f && G.tri_a[0] < tri_strips()) {
            G.tri_f[0] = 0.0f;
            int8_t cx[HZ_CELLS], cy[HZ_CELLS];
            int m = tri_strip(tri_strips() - 1 - G.tri_a[0], cx, cy, HZ_CELLS);
            Hazard *h = m ? hz_new(HZ_DAMP, 0) : NULL;
            if (h) {
                for (int i = 0; i < m; i++) { h->cx[i] = cx[i]; h->cy[i] = cy[i]; h->cd[i] = 0; }
                h->n = m; h->x = cx[m / 2] + 0.5f; h->y = cy[m / 2] + 0.5f; h->r = 3.0f; h->warn = hz_warn(1.0f); h->on = TRI_TIME; h->off = 1.0f; h->share = 0.04f; h->src = -7;
            }
            G.tri_a[0]++;
            sfx_at(SFX_HISS, cx[0] + 0.5f, cy[0] + 0.5f, 0.6f);
        }
        int open = 0, n = 0;
        for (int i = G.tri_n0; i < G.nthing; i++) if (G.thing[i].role == TR_LEVER) { n++; open += G.thing[i].state; }
        if (n && open >= n) {                                      /* the vents draw it off */
            for (int i = 0; i < MAXHAZ; i++) if (G.haz[i].alive && G.haz[i].kind == HZ_DAMP && in_trial((int)G.haz[i].x, (int)G.haz[i].y)) G.haz[i].on = fminf(G.haz[i].on, G.haz[i].t + 0.5f);
            tri_end(1);
        }
        break;
    }
    case TRI_MOLTEN: {                                             /* magma creeping from each vent while its imp lives */
        int left = 0;
        for (int k = 0; k < 4; k++) {
            int mi = G.tri_a[4 + k] - 1;
            if (mi < 0) continue;
            const Mob *m = &G.mobs[mi];
            if (!m->alive || m->state != MS_WALK || !m->tri) { if (G.tri_a[4 + k] > 0) { G.tri_a[4 + k] = -G.tri_a[4 + k]; sfx_at(SFX_FROST, m->x, m->y, 0.5f); } continue; }   /* slain: its vent crusts */
            left++;
            if ((G.tri_f[4 + k] += dt) > 4.5f) {                  /* a burn where it leads, spreading */
                G.tri_f[4 + k] = 0.0f;
                float a = rndf() * TAU_F, r = 0.5f + 1.5f * rndf();
                int x = (int)(m->x + cosf(a) * r), y = (int)(m->y + sinf(a) * r);
                if (in_trial(x, y) && hz_floor(x, y)) add_burn(x + 0.5f, y + 0.5f, 1);
            }
        }
        if (!left && !tri_alive(0)) tri_end(1);
        break;
    }
    case TRI_DARK: {                                               /* the braziers gutter, one by one; the shade goes for them */
        int lit = 0, n = 0, out = -1;
        for (int i = G.tri_n0; i < G.nthing; i++) if (G.thing[i].role == TR_LIGHT) { n++; lit += G.thing[i].state; if (G.thing[i].state && (out < 0 || rnd(2))) out = i; }
        if ((G.tri_f[0] += dt) >= 9.0f && out >= 0) { G.tri_f[0] = 0.0f; G.thing[out].state = 0; sfx_at(SFX_WHOOSH, G.thing[out].x, G.thing[out].y, 0.6f); }
        G.tri_f[1] = lit >= imin(4, n) ? 0.0f : G.tri_f[1] + dt;
        if (G.tri_f[1] > 8.0f) { tri_end(0); break; }             /* too few burning too long */
        for (int i = 0; i < MAXMOBS; i++) {
            Mob *m = &G.mobs[i];
            if (!m->alive || m->state != MS_WALK || m->tri != 2) continue;
            int best = -1; float bd = 1e9f;
            for (int j = G.tri_n0; j < G.nthing; j++) { const Thing *c = &G.thing[j]; float dd = hypotf(c->x - m->x, c->y - m->y); if (c->role == TR_LIGHT && c->state && dd < bd) { bd = dd; best = j; } }
            m->lure = best >= 0 && hypotf(G.px - m->x, G.py - m->y) > 2.2f;
            if (!m->lure) continue;
            m->lx = G.thing[best].x; m->ly = G.thing[best].y;
            if (bd >= 1.1f) m->sub_t = 0.0f;
            else if ((m->sub_t += dt) > 2.0f) { G.thing[best].state = 0; m->sub_t = 0.0f; sfx_at(SFX_WHOOSH, m->lx, m->ly, 0.6f); }
        }
        if (!tri_alive(2) && (G.tri_f[2] += dt) > 6.0f) { G.tri_f[2] = 0.0f; tri_wave(); }   /* struck down, it comes again */
        break;
    }
    case TRI_COURT: {                                              /* the crowned one; what it raised falls with it */
        int crowned = 0;
        for (int i = 0; i < MAXMOBS; i++) {
            Mob *m = &G.mobs[i];
            crowned |= m->alive && m->state == MS_WALK && m->tri == 3;
            if (m->alive && !m->tri && m->type == MOB_SKEL && in_trial(m->cx, m->cy)) m->tri = 1;   /* raised again: the court's */
        }
        if (!crowned && (!G.tri_a[1] || !tri_alive(0))) tri_end(1);
        break;
    }
    case TRI_SINGING: {                                            /* a crystal lights; strike it before its light goes out */
        int n = 0;
        for (int i = G.tri_n0; i < G.nthing; i++) n += G.thing[i].role == TR_STRIKE;
        if (G.tri_a[3] >= 6) { tri_end(1); break; }
        if ((G.tri_f[2] -= dt) <= 0.0f && n) {
            if (G.tri_a[2] >= 0) { G.thing[G.tri_a[2]].state = 0; sfx_at(SFX_KNOCK, G.thing[G.tri_a[2]].x, G.thing[G.tri_a[2]].y, 0.3f); }   /* gone out, unstruck */
            int k = G.tri_n0 + rnd(n), j = 0;
            for (int i = G.tri_n0; i < G.nthing; i++) if (G.thing[i].role == TR_STRIKE && j++ == k - G.tri_n0) { k = i; break; }
            G.tri_a[2] = k; G.thing[k].state = 1; G.tri_f[2] = 8.0f;
            snd_tone(1, 76 + G.thing[k].val * 2, 0.4f, 0.5f);
        }
        if ((G.tri_f[1] += dt) > 6.0f) {                           /* the glass spikes under you */
            G.tri_f[1] = 0.0f;
            if (hz_disc(HZ_GLASS, G.px, G.py, 0.9f, 1.0f, 1.0f, 0.8f, 0.08f)) sfx_at(SFX_SHATTER, G.px, G.py, 0.4f);
        }
        break;
    }
    default:
        if (!tri_alive(0)) tri_end(1);
        break;
    }
}
/* a slow thing under the hand: a second's work, which walking off or turning away stops */
static void update_work(float dt) {
    if (!G.work_i) return;
    int i = G.work_i - 1;
    const Thing *t = &G.thing[clampi(i, 0, MAXTHING - 1)];
    float d = hypotf(t->x - G.px, t->y - G.py), a = fabsf(wrap_pi(atan2f(t->y - G.py, t->x - G.px) - G.ang));
    if (i >= G.nthing || G.dead || G.ui || (G.player && (d > 2.0f || a > 1.2f))) { G.work_i = 0; return; }
    G.grab_t = fmaxf(G.grab_t, 0.4f);
    if ((G.work_t -= dt) <= 0.0f) thing_use(i);                    /* done: it does what it does */
}
static void update_trial(float dt) {
    if (G.tri < 0 || G.shop || PK.depth) return;
    update_work(dt);
    G.tri_hold = G.tri_state == TRS_ON && !G.player ? fmaxf(0.0f, G.tri_hold - dt) : 0.0f;
    if (G.tri_state == TRS_SHUT) {
        const Door *d = &G.doors[G.tri_door];
        int x = (int)G.px, y = (int)G.py;
        if (!G.dead && in_trial(x, y) && !door_body(d) && hypotf(G.px - (d->x + 0.5f), G.py - (d->y + 0.5f)) >= TRI_IN) tri_begin();   /* well past the door: a peek begins nothing */
        return;
    }
    if (G.tri_state != TRS_ON) {
        float was = G.tri_end;
        G.tri_end += dt;
        if (G.player && G.tri_state == TRS_WON && was < 2.4f && G.tri_end >= 2.4f && tri_reward_at(IT_CHEST) >= 0)
            say("ITS HOARD AND A DRAUGHT LIE IN THE ROOM");           /* once the boon's words have had their time (1.0.0b18.1) */
        return;
    }
    if (G.dead) {
#ifdef CRAWL_DEBUG
        if (!G.tri_a[15]++) { int n, d = tri_done(&n); fprintf(stderr, "EVENT trial_died %d %s level %d left %.1f by %s t=%.2f (%d of %d done, %d standing, by %d/%d/%d)\n", G.tri, TRI_NAME[G.tri], G.level, G.tri_t,
                                       G.player ? "player" : "autopilot", G.t, d, n, tri_alive(0), G.blame.what, G.blame.type, G.blame.rank); }
#endif
        return;
    }
    float was = G.tri_t;
    G.tri_t -= dt;
#ifdef CRAWL_DEBUG
    if (getenv("CRAWL_TRIWIN") && TRI_TIME - G.tri_t >= (float)atof(getenv("CRAWL_TRIWIN"))) { tri_end(1); return; }   /* testing: won, s seconds in */
#endif
    if (G.tri_t < 10.0f && floorf(was) != floorf(G.tri_t)) snd_sfx_ex(SFX_TICK, 0.5f + 0.05f * (10.0f - G.tri_t), 0.5f);   /* the last ten */
    tri_run(dt);
    if (G.tri_state == TRS_ON && G.tri_t <= 0.0f) {
        if (!TRI_HOLD[G.tri]) { tri_end(0); return; }
        int ok = 0;                                                /* held to the end */
        if (G.tri == TRI_VIGIL) for (int k = 0; k < 3; k++) ok |= G.thing[G.tri_a[1 + k]].state;
        if (G.tri == TRI_DARK) { int lit = 0, n = 0; for (int i = G.tri_n0; i < G.nthing; i++) if (G.thing[i].role == TR_LIGHT) { n++; lit += G.thing[i].state; } ok = lit >= imin(4, n); }
        tri_end(ok);
    }
}
/* the trial, placed with its level: its door's lights, and where the dream would go in */
static void tri_place(void) {
    G.tri_state = TRS_SHUT; G.tri_boon = 0; G.tri_nface = 0; G.work_i = 0; G.tri_wx = G.tri_wy = G.tri_ix = G.tri_iy = -1;
    if (G.tri < 0 || G.tri_door < 0 || G.tri_room < 0) { G.tri = -1; return; }
    const Door *dr = &G.doors[G.tri_door];
    for (int d = 0; d < 4; d++) { int x = dr->x + DX[d], y = dr->y + DY[d]; if (is_open(x, y) && !in_trial(x, y)) { G.tri_wx = x; G.tri_wy = y; } }
    if (G.tri_wx < 0) { G.tri = -1; return; }
    int ox = G.tri_wx - dr->x, oy = G.tri_wy - dr->y, run = crawl_set(G.level);
    for (int s = -1; s <= 1; s += 2) {                             /* its lights, either side of it in the hall */
        int x = dr->x + oy * s, y = dr->y + ox * s;
        if (cell(x, y)->kind == K_WALL) { x += ox; y += oy; }
        if (!hz_floor(x, y)) { x = G.tri_wx; y = G.tri_wy; }
        add_deco(x + 0.5f - ox * 0.3f + oy * s * 0.35f, y + 0.5f - oy * 0.3f + ox * s * 0.35f, SP_TRLIGHT0 + run);
    }
    light_static(G.tri_wx + 0.5f, G.tri_wy + 0.5f, 0.75f, 3.6f);
#ifdef CRAWL_DEBUG
    fprintf(stderr, "TRIAL lights at (%.2f,%.2f) and (%.2f,%.2f)\n", G.decos[G.ndecos - 2].x, G.decos[G.ndecos - 2].y, G.decos[G.ndecos - 1].x, G.decos[G.ndecos - 1].y);
#endif
    const Room *R = &G.rooms[G.tri_room];                          /* inside, two steps past the door, on a lane */
    int bd = 1 << 20;
    for (int y = R->y; y < R->y + R->h; y++)
        for (int x = R->x; x < R->x + R->w; x++) {
            int dd = abs(x - dr->x) + abs(y - dr->y);
            if (!apass(x, y) || dd < 2) continue;
            if (dd < bd) { bd = dd; G.tri_ix = x; G.tri_iy = y; }
        }
#ifdef CRAWL_DEBUG
    fprintf(stderr, "TRIAL level %d: %s room %d door (%d,%d) in at (%d,%d)\n", G.level, TRI_NAME[G.tri], G.tri_room, dr->x, dr->y, G.tri_ix, G.tri_iy);
#endif
}
