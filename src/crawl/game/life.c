/* A part of scene_crawl.c (docs/PLAN-tidy.md): it is #included there in its place and never compiled on its own.
 * The crypt's small life (docs/PLAN-ambience.md): the list, its dice, the settings' gates, where it may be; the kinds are added below as
 * they are built. None of it is saved, hurt, counted or in the way, and none of it draws on G.rs: before it is switched on, the dream's
 * log is the build before's, line for line. */

extern int g_life_move, g_life_sound;                              /* core/app.h */
#ifdef CRAWL_DEBUG
#define LEVENT(...) do { fprintf(stderr, "EVENT life "); fprintf(stderr, __VA_ARGS__); fprintf(stderr, " t=%.1f\n", G.level_t); } while (0)
#else
#define LEVENT(...) ((void)0)
#endif
#define LIFE_SPARE __attribute__((unused))                         /* until each kind that uses it is built */
LIFE_SPARE static int lrnd(int n) { return (int)(rng_next(&G.lrs) % (uint32_t)(n > 0 ? n : 1)); }
LIFE_SPARE static float lrndf(void) { return rng_f(&G.lrs); }

/* the owner's settings (core/app.h): movement and sound, 0 off, 1 reduced (what keeps still), 2 on */
static int life_move(void) { return g_life_move; }
LIFE_SPARE static int life_sound(void) { return g_life_sound; }

static int life_off(void) {                                        /* CRAWL_LIFE=0: none, for the proof and for the frames before */
#ifdef CRAWL_DEBUG
    if (getenv("CRAWL_LIFE") && atoi(getenv("CRAWL_LIFE")) == 0) return 1;
#endif
    return 0;
}

/* a level it may be on (not the shop, nor a pocket: the Backrooms, Severance, her rooms), and a cell it may begin in (the mice's: not a
 * lesson's trigger nor a trial's floor) */
static int life_level_ok(void) { return !G.shop && !PK.depth && G.faced; }
LIFE_SPARE static int life_cell_ok(int x, int y) { return mo_cell_ok(x, y); }

LIFE_SPARE static int life_add(int kind, float x, float y, float z) {         /* a new one, or -1 when the list is full */
    for (int i = 0; i < MAXLIFE; i++)
        if (!G.life[i].alive) {
            Life *o = &G.life[i];
            memset(o, 0, sizeof *o);
            o->alive = 1; o->kind = kind; o->x = x; o->y = y; o->z = z; o->ref = -1;
            if (i >= G.nlife) G.nlife = i + 1;
            return i;
        }
    return -1;
}

/* where a floor creature may be set: a floor cell, six steps and more from where you begin (and from you, when it comes back) */
static int life_spot(float *x, float *y, float far) {
    for (int k = 0; k < 300; k++) {
        int cx = 1 + lrnd(MAPN - 2), cy = 1 + lrnd(MAPN - 2);
        if (!life_cell_ok(cx, cy) || G.map[cy][cx].kind != K_FLOOR) continue;
        float fx = cx + 0.15f + 0.7f * lrndf(), fy = cy + 0.15f + 0.7f * lrndf();
        if (hypotf(fx - (G.cx + 0.5f), fy - (G.cy + 0.5f)) < far) continue;
        if (far < 6.0f + 0.5f && hypotf(fx - G.px, fy - G.py) < far) continue;
        *x = fx; *y = fy;
        return 1;
    }
    return 0;
}
static void life_sfx(int id, float x, float y, float gain, float pitch, int need) {   /* as mo_sfx: by distance and side, at the setting's leave */
    if (life_sound() < need) return;
    float dx = x - G.px, dy = y - G.py, d = sqrtf(dx * dx + dy * dy);
    float lat = dx * -sinf(G.ang) + dy * cosf(G.ang), g = gain / (1.0f + 0.08f * d * d);
    if (life_sound() == 1) g *= 0.5f;
    if (g > 0.03f) snd_sfx_pitch(id, g, 0.5f + 0.4f * clampf(lat / (d + 0.5f), -1.0f, 1.0f), pitch);
}

/* what is on each level: puddles that drip (and the toads by them), beetles and woodlice on the floor; fewer on a boss level, two and
 * one on the drawn ones */
static int lf_n_puddles, lf_wet;                                   /* how many; how far your boots are wet yet */
static float lf_pr_x, lf_pr_y; static int lf_pr_side;
static void life_floor(void) {
    int step = crawl_step(G.level), thin = g_drawn || G.boss >= 0;
    int np = thin ? 2 : clampi(3 + step / 2, 3, 8), nb = thin ? 3 : clampi(5 + step, 5, 12), nl = thin ? 2 : clampi(3 + step / 2, 3, 8);
    lf_n_puddles = 0; lf_wet = 0; lf_pr_x = lf_pr_y = -99.0f; lf_pr_side = 0;
    for (int k = 0; k < np; k++) {
        float x, y;
        if (!life_spot(&x, &y, 6.0f)) break;
        int i = life_add(LF_PUDDLE, x, y, 0.0f);
        if (i < 0) break;
        G.life[i].a = 0.55f + 0.4f * lrndf(); G.life[i].b = 1.0f + 5.0f * lrndf();
        lf_n_puddles++;
    }
    for (int k = 0; k < nb + nl; k++) {
        float x, y;
        if (!life_spot(&x, &y, 6.0f)) break;
        int i = life_add(k < nb ? LF_BEETLE : LF_LOUSE, x, y, 0.0f);
        if (i < 0) break;
        Life *o = &G.life[i];
        o->head = TAU_F * lrndf(); o->st = 1; o->wait = 3.0f * lrndf(); o->a = 0.8f + 0.4f * lrndf();   /* a: its pace's share */
    }
    if (G.level >= 6 && !thin)                                     /* a toad by a puddle or two, from the sixth level */
        for (int i = 0, made = 0; i < G.nlife && made < 2; i++)
            if (G.life[i].alive && G.life[i].kind == LF_PUDDLE && lrnd(2) == 0) {
                float a = TAU_F * lrndf(), r = 0.3f * G.life[i].a + 0.1f;
                float x = G.life[i].x + cosf(a) * r, y = G.life[i].y + sinf(a) * r;
                if (!mo_pass((int)x, (int)y, 0)) continue;
                int t = life_add(LF_TOAD, x, y, 0.0f);
                if (t < 0) break;
                G.life[t].ref = i; G.life[t].wait = 4.0f * lrndf(); made++;
            }
}

/* the air: moths and flies round the warm lights, bats roosting in the corners of the first ten levels' rooms (from the eleventh a roost of
 * the dead's own is the warning, and ours would blur it), and the torches, which gutter when you pass or a door swings */
static float lf_gut[MAPN][MAPN];                                   /* a torch's wall cell: how long its flame is guttering yet */
static int lf_ntorch; static int8_t lf_torch[64][2];
static int lf_nroost; static float lf_roost[16][2];                /* where a bat may hang: a floor cell's edge by a wall */
static float lf_door_was[MAXDOORS];
static int torch_frame(int mx, int my, int tframe) {               /* a flame's frame: its own cycle, or in a gutter the two leaning ones, quick */
    if (g_life_move == 2 && (unsigned)mx < MAPN && (unsigned)my < MAPN && lf_gut[my][mx] > 0.0f) return 1 + (((int)(G.t * 15.0) + (int)hash2(mx, my)) & 1);
    return (int)((tframe + hash2(mx, my)) % 3);
}
/* the stone: eyes in some niches' dark, sleepers (a hand) on some niches' sills, both set on the niche's face; and the hanging
 * things' swing kicked when you pass (lf_kick). Niches are slot 0 of runs I and II's features (look 0 and 1). */
static float lf_kick[MAXDECO];
static int lf_watch;                                               /* the saints' eyes (draw/walls.c): on while the small life is, and moving */
static float lf_kick_of(int i) { return g_life_move == 2 && i >= 0 && i < MAXDECO ? lf_kick[i] : 0.0f; }
static void life_stone(void) {
    memset(lf_kick, 0, sizeof lf_kick);
    if (!G.faced || G.look != 0 || g_drawn) return;                 /* run I's burial niches: four to a face, two by two, each cut 18 wide and 19 tall at
                                                                      (7, 6), (39, 6), (7, 38), (39, 38) with a skull or a shroud in it (gen_feat_niche) */
    int ne = 0, ns = 0, maxe = G.boss >= 0 ? 2 : 5, maxs = G.boss >= 0 ? 2 : 6;
    for (int y = 1; y < MAPN - 1; y++)
        for (int x = 1; x < MAPN - 1; x++) {
            const Cell *c = &G.map[y][x];
            if (c->kind != K_WALL || c->secret) continue;
            for (int d = 0; d < 4; d++) {
                if (c->fover[d] != T_FEAT + 0) continue;
                int ox = x + DX[d], oy = y + DY[d];
                if (!life_cell_ok(ox, oy) || G.map[oy][ox].kind != K_FLOOR) continue;
                if (hypotf(ox + 0.5f - (G.cx + 0.5f), oy + 0.5f - (G.cy + 0.5f)) < 5.0f) continue;
                float r = lrndf();
                int nx = lrnd(2), ny = lrnd(2), flip = c->fflip >> d & 1;
                float u = ((float)(nx * 32 + 16) / (float)TS - 0.5f) * (flip ? -1.0f : 1.0f);   /* along the face: its tangent runs (-DY, DX) as the texture's x does */
                float wx = x + 0.5f + DX[d] * 0.53f - DY[d] * u, wy = y + 0.5f + DY[d] * 0.53f + DX[d] * u;
                if (r < 0.16f && ne < maxe) {                      /* eyes: in the dark above the skull in its niche */
                    int i = life_add(LF_NEYES, wx, wy, 0.0f);
                    if (i < 0) return;
                    Life *o = &G.life[i];
                    o->vx = (float)DX[d]; o->vy = (float)DY[d]; o->c = 0.28f;
                    o->z = (1.0f - (float)(ny * 32 + 12) / (float)TS) - (1.0f - 33.0f / 64.0f) * o->c;
                    o->st = 0; o->wait = 3.0f * lrndf(); o->a = 0.0f; o->b = 2.0f + 4.0f * lrndf();
                    ne++;
                } else if (r < 0.34f && ns < maxs) {               /* a sleeper: a hand lying on the niche's sill */
                    int i = life_add(LF_SLEEP, wx, wy, 0.0f);
                    if (i < 0) return;
                    Life *o = &G.life[i];
                    o->vx = (float)DX[d]; o->vy = (float)DY[d]; o->c = 0.24f; o->ref = 1;
                    o->z = (1.0f - (float)(ny * 32 + 26) / (float)TS) - (1.0f - 56.0f / 64.0f) * o->c;
                    o->st = 0; o->wait = 0.0f;
                    ns++;
                }
                break;
            }
        }
}
static void lf_stoned(Life *o, float dt) {
    float dx = G.px - o->x, dy = G.py - o->y, d = hypotf(dx, dy);
    int front = dx * o->vx + dy * o->vy > 0.0f;
    o->t += dt;
    if (o->kind == LF_NEYES) {
        float bear = fabsf(wrap_pi(atan2f(o->y - G.py, o->x - G.px) - G.ang));   /* how squarely you face it */
        if (o->st == 0) {                                          /* unseen: waits for you to be in view, a few cells off */
            o->a = 0.0f;
            if ((o->wait -= dt) <= 0.0f && !G.dead && front && d > 3.8f && d < 10.0f && bear < 1.1f) { o->st = 1; o->t = 0.0f; LEVENT("niche eyes open d %.1f", d); }
        } else if (o->st == 1) {                                   /* open, blinking now and then */
            o->a = fminf(1.0f, o->a + dt * 1.3f);
            if (G.dead || !front || d < 3.4f || (bear < 0.2f && d < 6.5f) || d > 12.0f) { o->st = 2; o->t = 0.0f; LEVENT("niche eyes shut d %.1f", d); }
        } else {                                                   /* withdrawing: shut, fading; gone a good while */
            o->a -= dt * 2.0f;
            if (o->a <= 0.0f) { o->a = 0.0f; o->st = 0; o->wait = 25.0f + 25.0f * lrndf(); }
        }
        return;
    }
    if (o->wait > 0.0f) o->wait -= dt;                             /* a sleeper: settles, once, as you pass near */
    if (o->wait <= 0.0f && !G.dead && G.spd > 0.3f && d < 2.4f && front) {
        o->st ^= 1; o->wait = 8.0f;
        life_sfx(SFX_BONES, o->x, o->y, 0.25f, 1.8f, 2); LEVENT("sleeper settles %s", o->ref ? "hand" : "skull");
    }
}
static void lf_swing(float dt) {                                   /* hanging things: a kick when you pass, dying away */
    for (int i = 0; i < G.ndecos && i < MAXDECO; i++) {
        const Deco *dc = &G.decos[i];
        if (lf_kick[i] > 0.0f) lf_kick[i] = lf_kick[i] > 0.02f ? lf_kick[i] * expf(-0.9f * dt) : 0.0f;
        if (!(dc->flags & DF_SWAY) || dc->spr < 0 || lf_kick[i] > 0.25f || G.dead) continue;
        if (G.spd > 0.4f && fabsf(dc->x - G.px) < 1.2f && fabsf(dc->y - G.py) < 1.2f) { lf_kick[i] = 1.0f; LEVENT("hanging swings"); }
    }
}

static void life_air(void) {
    int step = crawl_step(G.level), thin = g_drawn || G.boss >= 0;
    memset(lf_gut, 0, sizeof lf_gut); memset(lf_door_was, 0, sizeof lf_door_was); lf_ntorch = 0; lf_nroost = 0;
    for (int y = 1; y < MAPN - 1; y++)
        for (int x = 1; x < MAPN - 1; x++) {
            const Cell *c = &G.map[y][x];
            if (c->kind != K_WALL || lf_ntorch >= 64) continue;
            int torch = c->tex == T_TORCH0;
            for (int d = 0; d < 4; d++) if (c->fover[d] == T_FEAT + F1_TORCH) torch = 1;
            if (torch) { lf_torch[lf_ntorch][0] = (int8_t)x; lf_torch[lf_ntorch][1] = (int8_t)y; lf_ntorch++; }
        }
    int nm = 0, nlit = 0;
    float decay = crawl_decay(G.level);
    for (int l = 0; l < G.nlights && nlit < 10; l++) {             /* moths, and flies where it is foul, about each warm light */
        const Light *L = &G.lights[l];
        if (!mo_warm(L)) continue;
        float cx = L->x, cy = L->y;
        int lx = (int)L->x, ly = (int)L->y;
        if (G.map[ly][lx].kind == K_WALL) {                        /* a torch in its wall: the air before it, on its open side */
            int got = 0;
            for (int d = 0; d < 4 && !got; d++) if (mo_pass(lx + DX[d], ly + DY[d], 0)) { cx = lx + 0.5f + DX[d] * 0.75f; cy = ly + 0.5f + DY[d] * 0.75f; got = 1; }
            if (!got) continue;
        }
        nlit++;
        int moths = thin ? 1 : 1 + lrnd(3), flies = decay > 0.35f && !thin ? lrnd(3) : 0;
        for (int k = 0; k < moths + flies && nm < 40; k++) {
            int i = life_add(k < moths ? LF_MOTH : LF_FLY, cx, cy, 0.5f);
            if (i < 0) break;
            Life *o = &G.life[i];
            o->vx = cx; o->vy = cy; o->ref = l; o->a = TAU_F * lrndf();
            o->b = (k < moths ? 0.2f : 0.28f) + 0.14f * lrndf();   /* its orbit's radius, its speed, its height, its own time */
            o->c = (k < moths ? 1.3f : 2.6f) * (0.8f + 0.5f * lrndf()) * (lrnd(2) ? 1.0f : -1.0f);
            o->d = 0.42f + 0.22f * lrndf(); o->vz = TAU_F * lrndf();
            nm++;
        }
    }
    if (G.level <= 10 && !thin) {                                  /* the bats: a few roosts, each by a wall */
        int want = clampi(2 + step / 3, 2, 4);
        for (int k = 0; k < 120 && lf_nroost < want; k++) {
            int cx = 1 + lrnd(MAPN - 2), cy = 1 + lrnd(MAPN - 2);
            if (!life_cell_ok(cx, cy) || G.map[cy][cx].kind != K_FLOOR) continue;
            int d = lrnd(4);
            if (G.map[cy + DY[d]][cx + DX[d]].kind != K_WALL) continue;
            float x = cx + 0.5f + DX[d] * 0.4f, y = cy + 0.5f + DY[d] * 0.4f;
            if (hypotf(x - (G.cx + 0.5f), y - (G.cy + 0.5f)) < 7.0f) continue;
            lf_roost[lf_nroost][0] = x; lf_roost[lf_nroost][1] = y; lf_nroost++;
        }
        for (int r = 0; r < lf_nroost; r++)
            for (int k = 0, n = 1 + lrnd(3); k < n; k++) {
                int i = life_add(LF_BAT, lf_roost[r][0] + (lrndf() - 0.5f) * 0.18f * (float)(DX[0] == 0), lf_roost[r][1] + (lrndf() - 0.5f) * 0.18f, 0.84f);
                if (i < 0) break;
                G.life[i].st = 0; G.life[i].a = (float)r; G.life[i].wait = 10.0f * lrndf();   /* a: the roost it hangs in */
            }
    }
}

static void life_level(void) {
#ifdef CRAWL_DEBUG
    if (getenv("CRAWL_LIFEMOVE")) g_life_move = atoi(getenv("CRAWL_LIFEMOVE"));     /* the settings, from the harness: 0 off, 1 reduced, 2 on */
    if (getenv("CRAWL_LIFESOUND")) g_life_sound = atoi(getenv("CRAWL_LIFESOUND"));
#endif
    G.nlife = 0; memset(G.life, 0, sizeof G.life); memset(lf_kick, 0, sizeof lf_kick); memset(lf_gut, 0, sizeof lf_gut); lf_ntorch = 0; lf_nroost = 0;
    G.lrs = ((G.rs ^ 0x1F3D5Bu) * 2654435769u + (uint32_t)G.level * 40503u) | 1u;   /* its own dice: G.rs read, never drawn on */
    lf_n_puddles = 0; lf_watch = 0;
    if (life_off() || !life_level_ok()) return;
    lf_watch = 1;
    life_floor();
    life_air();
    life_stone();
#ifdef CRAWL_DEBUG
    int c[LF_NKINDS] = { 0 };
    for (int i = 0; i < G.nlife; i++) if (G.life[i].alive) c[G.life[i].kind]++;
    fprintf(stderr, "EVENT life level %d puddles %d beetles %d lice %d toads %d moths %d flies %d bats %d roosts %d torches %d\n", G.level, c[LF_PUDDLE], c[LF_BEETLE], c[LF_LOUSE], c[LF_TOAD], c[LF_MOTH], c[LF_FLY], c[LF_BAT], lf_nroost, lf_ntorch);
#endif
}

/* a creature of the floor takes a step if it can: along x and along y apart, so it slides along a wall; 0 if it was stopped */
static int lf_step_to(Life *o, float nx, float ny) {
    int ok = 0;
    if (mo_pass((int)nx, (int)o->y, 0)) { o->x = nx; ok = 1; }
    if (mo_pass((int)o->x, (int)ny, 0)) { o->y = ny; ok = 1; }
    return ok;
}
static void lf_crawler(Life *o, int i, float dt) {                 /* a beetle or a woodlouse: wanders, stops, runs from you, slips into the stone */
    int beetle = o->kind == LF_BEETLE;
    float pace = (beetle ? 0.34f : 0.17f) * o->a, flee = (beetle ? 1.7f : 1.05f) * o->a, d = hypotf(o->x - G.px, o->y - G.py);
    o->t += dt;
    if (o->st == 4) {                                              /* gone into the stone: back after a while, elsewhere */
        if ((o->wait -= dt) <= 0.0f) {
            float x, y;
            if (life_spot(&x, &y, 7.0f)) { o->x = x; o->y = y; o->st = 1; o->wait = 1.0f; o->t = 0.0f; o->c = 0.0f; }
            else o->wait = 5.0f;
        }
        return;
    }
    if (o->st == 3) {                                              /* sliding into a crack: fading, then gone */
        o->c += dt * 2.5f;
        if (o->c >= 1.0f) { o->st = 4; o->wait = 18.0f + 30.0f * lrndf(); }
        return;
    }
    if (!G.dead && d < 0.22f && G.spd > 0.1f) {                    /* stepped on: a crunch, and gone for a long while */
        life_sfx(SFX_CHEW, o->x, o->y, 0.5f, beetle ? 1.7f : 1.3f, 2); LEVENT("crunch %s", beetle ? "beetle" : "louse");
        o->st = 4; o->wait = 60.0f + 60.0f * lrndf();
        return;
    }
    int near = !G.dead && d < (beetle ? 2.0f : 1.6f);
    if (near && o->st != 2) { LEVENT("flee %s d %.1f", beetle ? "beetle" : "louse", d); o->st = 2; o->wait = 1.5f + 2.5f * lrndf(); o->head = atan2f(o->y - G.py, o->x - G.px) + (lrndf() - 0.5f) * 1.2f; }
    if (o->st == 2) {                                              /* away from you, till it is clear of you or cornered */
        float h = atan2f(o->y - G.py, o->x - G.px);
        o->head += wrap_pi(h - o->head) * fminf(1.0f, 3.0f * dt);
        if (!lf_step_to(o, o->x + cosf(o->head) * flee * dt, o->y + sinf(o->head) * flee * dt)) { o->st = 3; o->c = 0.0f; LEVENT("crack %s", beetle ? "beetle" : "louse"); return; }
        if ((o->wait -= dt) <= 0.0f && !near) { o->st = 1; o->wait = 2.0f; }
        return;
    }
    if (o->st == 1) { if ((o->wait -= dt) <= 0.0f) { o->st = 0; o->wait = 1.5f + 3.5f * lrndf(); o->head += (lrndf() - 0.5f) * 2.4f; } return; }
    o->head += (lrndf() - 0.5f) * 1.6f * dt;                       /* wandering */
    if (!lf_step_to(o, o->x + cosf(o->head) * pace * dt, o->y + sinf(o->head) * pace * dt)) o->head += 1.2f + lrndf();
    if ((o->wait -= dt) <= 0.0f) { o->st = 1; o->wait = 1.0f + 2.5f * lrndf(); }
    (void)i;
}
static int lf_toad_hop(Life *o) {                                  /* begin a hop away from you, if there is floor for it: 1 if begun */
    float h = atan2f(o->y - G.py, o->x - G.px) + (lrndf() - 0.5f) * 1.0f, L = 0.55f + 0.3f * lrndf();
    float nx = o->x + cosf(h) * L, ny = o->y + sinf(h) * L;
    if (!mo_pass((int)nx, (int)ny, 0) || !mo_pass((int)((o->x + nx) * 0.5f), (int)((o->y + ny) * 0.5f), 0)) return 0;
    o->st = 1; o->t = 0.0f; o->vx = o->x; o->vy = o->y; o->c = nx; o->d = ny; o->head = h;
    return 1;
}
static void lf_toady(Life *o, float dt) {                          /* a toad: sits by its water, and hops off with a croak when you come near */
    float d = hypotf(o->x - G.px, o->y - G.py);
    o->t += dt;
    if (o->st == 1) {                                              /* in a hop: up, and along, and down */
        float f = o->t / 0.55f;
        if (f < 1.0f) { o->x = o->vx + (o->c - o->vx) * f; o->y = o->vy + (o->d - o->vy) * f; o->z = 0.16f * sinf(f * PI_F); return; }
        o->x = o->c; o->y = o->d; o->z = 0.0f;
        if (--o->b > 0.0f && !G.dead && d < 2.4f && lf_toad_hop(o)) return;
        o->st = 0; o->wait = 6.0f + 6.0f * lrndf();
        return;
    }
    if ((o->wait -= dt) > 0.0f || G.dead || d >= 2.0f) return;
    o->b = 3.0f;                                                   /* off: three hops at most, away from you */
    if (lf_toad_hop(o)) LEVENT("hop toad d %.1f", d);
    if (o->st == 1) life_sfx(SFX_SQUEAK, o->x, o->y, 0.35f, 0.42f, 2);   /* a croak: a chirp, low and slow */
    else o->wait = 1.0f;
}

/* a moth or a fly: round its light, wobbling; from you, when you come near it, out into the dark, then back */
static void lf_flier(Life *o, float dt) {
    int fly = o->kind == LF_FLY;
    float d = hypotf(o->vx - G.px, o->vy - G.py), pd = hypotf(o->x - G.px, o->y - G.py);
    o->t += dt;
    if (o->st == 0) {                                              /* in its orbit */
        o->a += o->c * dt;
        float wob = fly ? 0.07f * sinf(o->t * 9.0f + o->vz) : 0.03f * sinf(o->t * 3.0f + o->vz);
        float r = o->b + wob + (fly ? 0.05f * sinf(o->t * 17.0f) : 0.0f);
        o->x = o->vx + cosf(o->a) * r; o->y = o->vy + sinf(o->a) * r * 0.8f;
        o->z = o->d + 0.1f * sinf(o->t * (fly ? 4.0f : 1.7f) + o->vz);
        if (!mo_pass((int)o->x, (int)o->y, 0)) { o->x = o->vx; o->y = o->vy; }
        if (!G.dead && (d < 1.5f || (pd < 2.4f && G.spd > 0.5f))) {   /* you near it: off, away from you, into the dark */
            float h = atan2f(o->vy - G.py, o->vx - G.px) + (lrndf() - 0.5f) * 1.6f;
            o->st = 1; o->t = 0.0f; o->head = h; o->wait = 1.4f + 1.4f * lrndf();
        }
        return;
    }
    if (o->st == 1) {                                              /* fleeing */
        float sp = fly ? 3.2f : 2.1f;
        float nx = o->x + cosf(o->head) * sp * dt, ny = o->y + sinf(o->head) * sp * dt + 0.0f;
        if (mo_pass((int)nx, (int)ny, 0)) { o->x = nx; o->y = ny; } else o->head += 1.9f + lrndf();
        o->head += (lrndf() - 0.5f) * 5.0f * dt;
        o->z = fminf(0.85f, o->z + 0.25f * dt) + 0.02f * sinf(o->t * 20.0f);
        if ((o->wait -= dt) <= 0.0f) { o->st = 2; o->wait = 2.5f + 3.5f * lrndf(); }
        return;
    }
    if (o->st == 2) {                                              /* waiting out of sight of you, in the dark */
        o->x += (lrndf() - 0.5f) * 0.4f * dt; o->z = o->d + 0.2f * sinf(o->t * 2.0f);
        if ((o->wait -= dt) <= 0.0f && (G.dead || d > 2.4f)) o->st = 3;
        return;
    }
    {                                                              /* going back to its light */
        float dx = o->vx - o->x, dy = o->vy - o->y, L = hypotf(dx, dy) + 0.001f, sp = fly ? 1.8f : 1.2f;
        float nx = o->x + dx / L * sp * dt, ny = o->y + dy / L * sp * dt;
        if (mo_pass((int)nx, (int)ny, 0)) { o->x = nx; o->y = ny; } else { o->x = o->vx; o->y = o->vy; }
        o->z += (o->d - o->z) * fminf(1.0f, 1.5f * dt);
        if (L < o->b + 0.1f) { o->st = 0; o->a = atan2f(o->y - o->vy, o->x - o->vx); }
        if (!G.dead && (d < 1.5f)) { o->st = 1; o->t = 0.0f; o->head = atan2f(o->y - G.py, o->x - G.px); o->wait = 1.0f; }
    }
}
/* a bat: hangs by its roost; some of those near it drop when you come within two and a half cells, circle the room for a few
 * seconds and settle in another roost (or the same). It never comes near you: it flies off from you and from the walls. */
static void lf_batty(Life *o, float dt) {
    float d = hypotf(o->x - G.px, o->y - G.py);
    o->t += dt;
    if (o->st == 0) {                                              /* roosting */
        o->wait -= dt;
        if (o->wait <= 0.0f && !G.dead && d < 2.6f) {
            if (lrnd(3) == 0) { o->wait = 20.0f + 20.0f * lrndf(); return; }   /* this one stays */
            o->st = 1; o->t = 0.0f; o->c = 4.0f + 3.0f * lrndf();   /* c: how long it flies */
            o->head = atan2f(o->y - G.py, o->x - G.px) + (lrndf() - 0.5f) * 1.4f; o->z = 0.84f; o->vz = 0.0f;
            life_sfx(SFX_BAT, o->x, o->y, 0.5f, 1.4f, 2); LEVENT("bat drops d %.1f", d);
        }
        return;
    }
    if (o->st == 1) {                                              /* on the wing */
        float sp = 1.9f, ph = o->t;
        o->z += (0.58f + 0.12f * sinf(ph * 1.7f) - o->z) * fminf(1.0f, 2.0f * dt);
        o->head += (lrndf() - 0.5f) * 3.0f * dt + 0.35f * dt * (o->vz < 0.0f ? -1.0f : 1.0f);
        if (!G.dead && d < 1.4f) o->head += wrap_pi(atan2f(o->y - G.py, o->x - G.px) - o->head) * fminf(1.0f, 5.0f * dt);   /* from you, never at you */
        float nx = o->x + cosf(o->head) * sp * dt, ny = o->y + sinf(o->head) * sp * dt;
        if (mo_pass((int)nx, (int)ny, 0)) { o->x = nx; o->y = ny; }
        else { o->head += 1.5f + 1.2f * lrndf(); o->vz = lrndf() < 0.5f ? -1.0f : 1.0f; }
        if ((o->c -= dt) <= 0.0f) {                                 /* to a roost: the nearest of the others in sight, else its own */
            int best = (int)o->a; float bd = 1e9f;
            for (int r = 0; r < lf_nroost; r++) {
                float rd = hypotf(lf_roost[r][0] - o->x, lf_roost[r][1] - o->y);
                if (rd > 1.0f && rd < bd && line_clear(o->x, o->y, lf_roost[r][0], lf_roost[r][1]) && lrnd(2)) { bd = rd; best = r; }
            }
            o->a = (float)best; o->st = 2; o->t = 0.0f;
        }
        return;
    }
    {                                                              /* coming home */
        float tx = lf_roost[(int)o->a][0], ty = lf_roost[(int)o->a][1], dx = tx - o->x, dy = ty - o->y, L = hypotf(dx, dy) + 0.001f;
        float nx = o->x + dx / L * 1.7f * dt, ny = o->y + dy / L * 1.7f * dt;
        if (mo_pass((int)nx, (int)ny, 0) || L < 0.6f) { o->x = nx; o->y = ny; } else { o->x = tx; o->y = ty; }
        o->z += (0.84f - o->z) * fminf(1.0f, 2.5f * dt);
        if (L < 0.12f || o->t > 12.0f) { o->x = tx + (lrndf() - 0.5f) * 0.15f; o->y = ty + (lrndf() - 0.5f) * 0.15f; o->z = 0.84f; o->st = 0; o->wait = 12.0f + 18.0f * lrndf(); }
    }
}
/* the torches: gutter when you pass near and moving, and when a door close to them swings */
static void lf_torches(float dt) {
    for (int t = 0; t < lf_ntorch; t++) {
        int x = lf_torch[t][0], y = lf_torch[t][1];
        float *g = &lf_gut[y][x];                                   /* guttering while above nought; below it, a rest of five seconds first */
        if (*g > 0.0f) { *g -= dt; if (*g <= 0.0f) *g = -5.0f; }
        else if (*g < 0.0f) { *g += dt; if (*g > 0.0f) *g = 0.0f; }
        else if (!G.dead && G.spd > 0.3f && hypotf(x + 0.5f - G.px, y + 0.5f - G.py) < 2.3f) { *g = 0.9f; LEVENT("torch gutters"); }
    }
    for (int k = 0; k < G.ndoors; k++) {
        float o = G.doors[k].open;
        if (fabsf(o - lf_door_was[k]) > 0.002f && o > 0.01f && o < 0.99f)   /* a door moving: every torch within five cells gutters, harder */
            for (int t = 0; t < lf_ntorch; t++)
                if (hypotf(lf_torch[t][0] - G.doors[k].x, lf_torch[t][1] - G.doors[k].y) < 5.0f && lf_gut[(int)lf_torch[t][1]][(int)lf_torch[t][0]] < 1.3f && lf_gut[(int)lf_torch[t][1]][(int)lf_torch[t][0]] >= 0.0f)
                    lf_gut[(int)lf_torch[t][1]][(int)lf_torch[t][0]] = 1.3f;
        lf_door_was[k] = o;
    }
}

#ifdef CRAWL_DEBUG
static void human_begin(void);
/* CRAWL_TPLIFE=1: three cells from the first puddle, facing it, a beetle, a woodlouse and (if there is one) a toad set about it;
 * the player may then be moved on by HOST_INPUT. 2: as 1, with the beetle and louse on the floor a step before you. */
static void life_tp(void) {
    if (atoi(getenv("CRAWL_TPLIFE")) == 7) {                         /* 7: two cells before a saint, run II's, a little to one side of her */
        for (int y = 1; y < MAPN - 1; y++)
            for (int x = 1; x < MAPN - 1; x++)
                for (int d = 0; d < 4; d++) {
                    const Cell *c = &G.map[y][x];
                    if (c->kind != K_WALL || c->fover[d] != T_FEAT + F2_SAINT || G.look != 1) continue;
                    int ox = x + DX[d] * 2, oy = y + DY[d] * 2;
                    if (!mo_pass(ox, oy, 0) || !mo_pass(x + DX[d], y + DY[d], 0)) continue;
                    float side = mo_pass(ox - DY[d], oy + DX[d], 0) ? 0.35f : 0.0f;
                    G.cx = ox; G.cy = oy; G.px = ox + 0.5f - DY[d] * side; G.py = oy + 0.5f + DX[d] * side;
                    G.ang = atan2f(y + 0.5f - G.py, x + 0.5f - G.px);
                    human_begin(); G.hint_t = 0.0f;
                    return;
                }
        return;
    }
    int mode = atoi(getenv("CRAWL_TPLIFE")), pu = -1, want = mode == 3 ? LF_MOTH : mode == 4 ? LF_BAT : mode == 5 ? LF_NEYES : mode == 6 ? LF_SLEEP : LF_PUDDLE;   /* 3: before a light's moths; 4: a roost; 5: eyes in a niche; 6: a sleeper */
    for (int i = 0; i < G.nlife && pu < 0; i++) if (G.life[i].alive && G.life[i].kind == want) pu = i;
    if (pu < 0) return;
    float lx = want == LF_MOTH ? G.life[pu].vx : G.life[pu].x, ly = want == LF_MOTH ? G.life[pu].vy : G.life[pu].y;
    mo_bfs((int)lx, (int)ly, 0);
    for (int y = 1; y < MAPN - 1; y++)
        for (int x = 1; x < MAPN - 1; x++)
            if (mo_dist[y][x] == (mode == 5 ? 5 : 3) && G.map[y][x].kind == K_FLOOR && line_clear(x + 0.5f, y + 0.5f, lx, ly)) {
                G.cx = x; G.cy = y; G.px = x + 0.5f; G.py = y + 0.5f; G.ang = atan2f(ly - G.py, lx - G.px);
                int k = 0;
                for (int i = 0; i < G.nlife; i++) {                  /* the crawlers nearest the puddle are brought to it */
                    Life *o = &G.life[i];
                    if (mode >= 3 || !o->alive || (o->kind != LF_BEETLE && o->kind != LF_LOUSE) || k >= 3) continue;
                    float ax = lx + 0.3f * (k - 1), ay = ly + 0.55f + 0.2f * k;
                    if (mo_pass((int)ax, (int)ay, 0)) { o->x = ax; o->y = ay; o->st = 1; o->wait = 30.0f; o->head = G.ang + PI_F; k++; }
                }
                if (mode == 5) { G.life[pu].st = 1; G.life[pu].a = 1.0f; G.life[pu].b = 0.0f; G.ang += 0.45f; }   /* a little off: face it squarely and it shuts */
                human_begin(); G.hint_t = 0.0f;                      /* the controls card put away, to see */
                return;
            }
}
#endif

static void life_update(float dt) {
    if (!G.nlife || !life_level_ok() || life_move() == 0) return;
    int mobile = life_move() == 2;
    float wet_r2 = 0.0f;
    for (int i = 0; i < G.nlife; i++) {
        Life *o = &G.life[i];
        if (!o->alive) continue;
        switch (o->kind) {
        case LF_PUDDLE:
            if (hypotf(G.px - o->x, G.py - o->y) < 0.46f * o->a) { lf_wet = 8; wet_r2 = 1.0f; }   /* you are in it: wet boots for some steps */
            if (!mobile && life_sound() == 0) break;
            if ((o->b -= dt) <= 0.0f && hypotf(o->x - G.px, o->y - G.py) < 14.0f) {   /* a drop forms above it, and falls */
                int d = mobile ? life_add(LF_DRIP, o->x + (lrndf() - 0.5f) * 0.4f * o->a, o->y + (lrndf() - 0.5f) * 0.25f * o->a, 0.95f) : -1;
                if (d >= 0) G.life[d].ref = i;
                else if (!mobile) life_sfx(SFX_DRIP, o->x, o->y, 0.4f, 1.0f, 1);
                o->b = 2.5f + 5.5f * lrndf();
            } else if (o->b <= 0.0f) o->b = 2.5f + 5.5f * lrndf();
            break;
        case LF_DRIP:
            o->vz -= 7.6f * dt; o->z += o->vz * dt;
            if (o->z <= 0.0f) {                                    /* in: a ring, and a plink */
                const Life *pu = &G.life[o->ref];
                int r = life_add(LF_RING, o->x, o->y, 0.0f);
                if (r >= 0) { G.life[r].a = pu->a; G.life[r].ref = o->ref; }
                life_sfx(SFX_DRIP, o->x, o->y, 0.5f, 0.9f + 0.3f * lrndf(), 1); LEVENT("drip");
                o->alive = 0;
            }
            break;
        case LF_RING: if ((o->t += dt) > 1.35f) o->alive = 0; break;
        case LF_PRINT: if ((o->t += dt) > 14.0f) o->alive = 0; break;
        case LF_BEETLE: case LF_LOUSE: if (mobile) lf_crawler(o, i, dt); break;
        case LF_TOAD: if (mobile) lf_toady(o, dt); break;
        case LF_MOTH: case LF_FLY: if (mobile) lf_flier(o, dt); break;
        case LF_BAT: if (mobile) lf_batty(o, dt); break;
        case LF_NEYES: case LF_SLEEP: if (mobile) lf_stoned(o, dt); break;
        default: break;
        }
    }
    if (mobile) { lf_torches(dt); lf_swing(dt); }
    (void)wet_r2;
    if (lf_wet > 0 && !G.dead) {                                   /* wet boots: a print each half-cell, a little paler each, till they are dry */
        float mv = hypotf(G.px - lf_pr_x, G.py - lf_pr_y);
        if (mv >= 0.5f) {
            float sx = -sinf(G.ang) * (lf_pr_side ? 0.1f : -0.1f), sy = cosf(G.ang) * (lf_pr_side ? 0.1f : -0.1f);
            if (lf_pr_x > -90.0f && mo_pass((int)(G.px + sx), (int)(G.py + sy), 0)) {
                int p = life_add(LF_PRINT, G.px + sx, G.py + sy, 0.0f);
                if (p >= 0) { LEVENT("print wet %d", lf_wet); G.life[p].a = (float)lf_wet / 8.0f; G.life[p].b = (float)lf_pr_side; G.life[p].c = (float)((int)(G.ang * 100.0f)); G.life[p].t = 0.0f; }
            }
            lf_pr_x = G.px; lf_pr_y = G.py; lf_pr_side ^= 1; lf_wet--;
        }
    } else if (lf_wet <= 0) { lf_pr_x = lf_pr_y = -99.0f; }
}
