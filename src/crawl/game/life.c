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

static void life_level(void) {
    G.nlife = 0; memset(G.life, 0, sizeof G.life);
    G.lrs = ((G.rs ^ 0x1F3D5Bu) * 2654435769u + (uint32_t)G.level * 40503u) | 1u;   /* its own dice: G.rs read, never drawn on */
    lf_n_puddles = 0;
    if (life_off() || !life_level_ok()) return;
    life_floor();
#ifdef CRAWL_DEBUG
    int c[LF_NKINDS] = { 0 };
    for (int i = 0; i < G.nlife; i++) if (G.life[i].alive) c[G.life[i].kind]++;
    fprintf(stderr, "EVENT life level %d puddles %d beetles %d lice %d toads %d\n", G.level, c[LF_PUDDLE], c[LF_BEETLE], c[LF_LOUSE], c[LF_TOAD]);
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

#ifdef CRAWL_DEBUG
static void human_begin(void);
/* CRAWL_TPLIFE=1: two and a half cells from the first puddle, facing it, a beetle, a woodlouse and (if there is one) a toad set about it;
 * the player may then be moved on by HOST_INPUT. 2: as 1, with the beetle and louse on the floor a step before you. */
static void life_tp(void) {
    int pu = -1;
    for (int i = 0; i < G.nlife && pu < 0; i++) if (G.life[i].alive && G.life[i].kind == LF_PUDDLE) pu = i;
    if (pu < 0) return;
    float lx = G.life[pu].x, ly = G.life[pu].y;
    mo_bfs((int)lx, (int)ly, 0);
    for (int y = 1; y < MAPN - 1; y++)
        for (int x = 1; x < MAPN - 1; x++)
            if (mo_dist[y][x] == 3 && G.map[y][x].kind == K_FLOOR && line_clear(x + 0.5f, y + 0.5f, lx, ly)) {
                G.cx = x; G.cy = y; G.px = x + 0.5f; G.py = y + 0.5f; G.ang = atan2f(ly - G.py, lx - G.px);
                int k = 0;
                for (int i = 0; i < G.nlife; i++) {                  /* the crawlers nearest the puddle are brought to it */
                    Life *o = &G.life[i];
                    if (!o->alive || (o->kind != LF_BEETLE && o->kind != LF_LOUSE) || k >= 3) continue;
                    float ax = lx + 0.3f * (k - 1), ay = ly + 0.55f + 0.2f * k;
                    if (mo_pass((int)ax, (int)ay, 0)) { o->x = ax; o->y = ay; o->st = 1; o->wait = 30.0f; o->head = G.ang + PI_F; k++; }
                }
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
        default: break;
        }
    }
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
