/* A part of scene_crawl.c (docs/PLAN-tidy.md): it is #included there in its place and never compiled on its own.
 * Side passages (docs/PLAN-side.md): a platformer behind a doorway in the crypt. In by a doorway and out by one, the crypt
 * waiting as it is meanwhile (G untouched but for the caster's health, keys and the run's purse, which are the caster's own);
 * the caster's box and how it moves; the tiles, platforms, falling blocks, flows and spikes; doorways, lives and their end;
 * the spells by element; the run's creatures; what is found. The dream's hands are side_dream.c's, the drawing draw/side.c's. */

#define SD_MOBS 40
#define SD_SHOTS 40
#define SD_STEP (1.0f / 240.0f)                                    /* the physics' fixed step, so a run comes out the same */
#define SD_VIEW_H 156                                              /* the view over the status bar */
#define SD_LIVES 3
#define SD_LIFE_PRICE 500                                          /* at the merchant's (step 11) */
#define SD_HW 5.0f                                                 /* the caster's box: half its width, its height, crouched */
#define SD_HT 13.0f                                                /* half the first figure's height (the owner, 26 November) */
#define SD_HC 8.0f
static float SDT[NTU];                                             /* the tuned numbers this passage plays with (TUNE, side_text.c) */
#define SD_WALK SDT[TU_WALK]
#define SD_RUN SDT[TU_RUN]
#define SD_CREEP SDT[TU_CREEP]
#define SD_JUMP SDT[TU_JUMP]
#define SD_G_HELD SDT[TU_GRAV_HELD]
#define SD_G SDT[TU_GRAV]
#define SD_FALL_MAX SDT[TU_FALL_MAX]
#define SD_COYOTE SDT[TU_COYOTE]
#define SD_JBUF SDT[TU_JUMP_BUF]
#define SD_HAND (SD.crouch ? 5.0f : 9.0f)                          /* where a spell leaves the hand, above the feet */
#define SD_SIDES 8                                                 /* side passages a run's levels can name: SIDE_TEXT's */

enum { SS_BOLT, SS_FLASK, SS_BONE };
enum { SDM_WALK, SDM_WIND, SDM_DIE };
typedef struct { int alive, kind, rank, thing, face, fly, state; float x, y, vx, vy, hp, maxhp, dmgk, speed, t, cool, hurt_t, home_x, home_y, pois_t, pois_dps, slow_t; } SdMob;
typedef struct { int alive, kind, sp, el, tier, foe; float x, y, vx, vy, life, dmg, g; } SdShot;
typedef struct { float x, y, px, py, s; int dir; float wait; } SdPlat;   /* where it is (its top's left), where it was; s along its way */
typedef struct { int state; float t, dy, vy; } SdFall;             /* 0 in place, 1 shaking, 2 falling, 3 gone */
typedef struct { int left, right, run, jump, jump_hit, down, up_hit, cast, cast_hit; } SdIn;

static SdLevel SDL[SD_SIDES];                                      /* each read once, when first walked into */
static int sdl_read[SD_SIDES];
static struct {
    int on, side, link_in, cx, cy;                                 /* in one; which; by which link; the crypt doorway walked into */
    SdLevel *L;
    float t;                                                       /* its own clock: the crypt's waits */
    uint32_t rs;                                                   /* its own dice: the crypt's are left as they were */
    float x, y, vx, vy;                                            /* the caster: the middle of its feet, in pixels */
    int face, ground, crouch, on_plat, drop;
    float coyote, jbuf, anim, cast_t, cd, inv_t, hurt_t, pois_k, held, in_t;
    int frame, dying, leaving, out_link, out_goal, holding;
    float die_t, fade, mark_x, mark_y;
    float beam_t, bx0, by0, bx1, by1;                              /* lightning, a moment */
    SdPlat plat[SD_THINGS]; SdFall fall[SD_THINGS];
    uint8_t taken[SD_THINGS], thrown[SD_THINGS];
    SdMob mob[SD_MOBS]; SdShot shot[SD_SHOTS];
    float camx, camy;
    SdIn in;
    int dream_on;
    /* the works (side_works.c): each thing's state, on or off; a gate's openness; a flow's clock and fill; what is loose, where;
     * each trigger's state; each trap's turn */
    uint8_t st[SD_THINGS];
    float gk[SD_THINGS], fc[SD_THINGS], fk[SD_THINGS];
    float ix[SD_THINGS], iy[SD_THINGS], ivx[SD_THINGS], ivy[SD_THINGS];
    uint8_t tr_on[SD_TRIGS], tr_was[SD_TRIGS]; float tr_t[SD_TRIGS];
    int hz_ph[SD_HAZ], hz_at[SD_HAZ], hz_n[SD_HAZ], hz_in[SD_HAZ]; float hz_t[SD_HAZ], hz_k[SD_HAZ], hz_bite[SD_HAZ];
    float silence_t, hold_t;
} SD;
static struct {                                                    /* what stays done for the rest of the crypt level's visit */
    int stamp;
    uint8_t taken[SD_SIDES][SD_THINGS], thrown[SD_SIDES][SD_THINGS], slain[SD_SIDES][SD_THINGS], done[SD_SIDES], tried[SD_SIDES];   /* tried: the dream went in */
} SDP;
static struct { uint8_t taken[SD_THINGS], thrown[SD_THINGS], slain[SD_THINGS]; int keys[NKEYS], treasure, level_treasure; } SD0;   /* as it was when you came in */
#define SDX_DOORS 16
static struct { int n; struct { int x, y, side, link; } d[SDX_DOORS]; } SDX;   /* the crypt level's side doorways */
static int sd_stamp;                                               /* a level laid */
static int sd_dream_lost = -1;                                     /* the level the dream lost its last life on: not again there */

static int sd_lives(void) { return SD_LIVES - clampi(RUN.sd_lost, 0, SD_LIVES); }
static float sd_rndf(void) { return (float)(rng_next(&SD.rs) & 0xFFFFFF) / 16777216.0f; }

/* ---- the crypt's side: its doorways, laid with the level ---- */
static void sd_add_door(int x, int y, int side, int link) {
    if (SDX.n >= SDX_DOORS || x <= 0 || y <= 0 || x >= MAPN - 1 || y >= MAPN - 1) return;
    Cell *c = &G.map[y][x];
    if (c->kind != K_WALL) return;
    c->tex = T_PASSAGE; memset(c->face, 0, sizeof c->face); memset(c->fover, 0, sizeof c->fover);   /* the draft's look: a pocket's way in */
    SDX.d[SDX.n].x = x; SDX.d[SDX.n].y = y; SDX.d[SDX.n].side = side; SDX.d[SDX.n].link = link; SDX.n++;
#ifdef CRAWL_DEBUG
    fprintf(stderr, "SIDE door (%d,%d) to side %d link %d\n", x, y, side, link);
#endif
}
static void sd_level_laid(void) {                                  /* start_level, after the level's things: no dice are thrown here */
    SDX.n = 0; sd_stamp++;
    memset(&SDP, 0, sizeof SDP); SDP.stamp = sd_stamp;
    SD.on = 0;
    if (g_drawn && !G.shop) {                                      /* the drawing's doorways (the World Editor's) */
        const Drawn *D = drawn();
        for (int i = 0; i < D->nthing; i++)
            if (D->thing[i].what == DT_SIDEDOOR) sd_add_door(D->ox + (int)D->thing[i].x, D->oy + (int)D->thing[i].y, D->thing[i].kind / 16, D->thing[i].kind % 16);
    }
#ifdef CRAWL_DEBUG
    const char *tp = getenv("CRAWL_TPSIDE");                       /* testing: a doorway in the first wall ahead of the start */
    if (tp && !G.shop) {
        int d = G.h;
        for (int k = 1; k < 12; k++) {
            int x = G.cx + DX[d] * k, y = G.cy + DY[d] * k;
            if (x <= 0 || y <= 0 || x >= MAPN - 1 || y >= MAPN - 1) break;
            if (G.map[y][x].kind == K_WALL) { for (int l = 0; l < 16; l++) sd_add_door(x, y, atoi(tp), l); break; }
            if (G.map[y][x].kind != K_FLOOR) break;
        }
    }
#endif
}
static int sd_crypt_door(int x, int y) {                           /* the side doorway at (x, y), -1 none */
    for (int i = 0; i < SDX.n; i++) if (SDX.d[i].x == x && SDX.d[i].y == y) return i;
    return -1;
}

/* ---- the crypt's dream and the doorways (step 7): the nearest doorway to a passage not yet tried this visit, while it is early,
 * whole and has two lives or more; it stands on the lane beside it, turns and goes in, as into a chain's doorway (PS_USE) ---- */
static int sd_dream_goal(int x, int y, int *gx, int *gy) {         /* pick_goal; bfs_dist is from (x, y) already (chain_goal) */
    if (!SDX.n || G.player || G.shop || PK.depth || G.level == sd_dream_lost || G.leaving || G.level_t > 200.0 || G.hp * 3 < G.max_hp * 2 || sd_lives() < 2) return 0;
    int best = -1, bd = 61, sx, sy;
    for (int i = 0; i < SDX.n; i++) {
        int s = SDX.d[i].side;
        if (s < 0 || s >= SD_SIDES || SDP.done[s] || SDP.tried[s] || !use_spot(SDX.d[i].x, SDX.d[i].y, &sx, &sy)) continue;
        int d = bfs_dist[sy][sx];
        if (d >= 0 && d < bd) { bd = d; best = i; *gx = sx; *gy = sy; }
    }
    if (best < 0) return 0;
    G.use_x = SDX.d[best].x; G.use_y = SDX.d[best].y; G.use_kind = USE_SIDE;
    return 1;
}
static int sd_walk_in(int x, int y);
static int sd_dream_in(void) {                                     /* PS_USE: through it */
    int k = sd_crypt_door(G.use_x, G.use_y);
    G.use_kind = USE_NONE;
    if (k < 0) return 0;
    if (SDX.d[k].side >= 0 && SDX.d[k].side < SD_SIDES) SDP.tried[SDX.d[k].side] = 1;
    return sd_walk_in(G.use_x, G.use_y);
}

/* ---- the world as it stands ---- */
static int sd_tile(int tx, int ty) {                               /* the letter, the dark outside the sides and under the foot */
    if (tx < 0 || tx >= SD.L->w) return '#';
    if (ty < 0) return '#';
    if (ty >= SD.L->h) return '.';
    return SD.L->t[ty][tx];
}
static int sd_fall_at(int tx, int ty) {                            /* a falling block standing there, -1 none */
    const SdLevel *L = SD.L;
    for (int i = 0; i < L->nth; i++)
        if (L->th[i].kind == SDT_FALL && L->th[i].x == tx && L->th[i].y == ty && SD.fall[i].state <= 1) return i;
    return -1;
}
static int sd_gate_at(int tx, int ty);                             /* side_works.c */
static int sd_in_current_poison(float x, float y);
static void sd_works_build(void); static void sd_waters(float dt); static void sd_items(float dt); static void sd_trigs(float dt); static void sd_traps(float dt);
static int sd_solid(int tx, int ty) { return sd_solid_ch(sd_tile(tx, ty)) || sd_fall_at(tx, ty) >= 0 || sd_gate_at(tx, ty); }
static int sd_floor_px(float px) { return (int)floorf(px / SD_TILE); }
static float sd_flow_k(const SdThing *t) { return SD.fk[t - SD.L->th]; }   /* how full a flow is now (side_works.c keeps it) */
static float sd_flow_top(const SdThing *t) { return (float)((t->y + t->h) * SD_TILE) - sd_flow_k(t) * (float)(t->h * SD_TILE); }
static int sd_in_poison(float x, float y, float top) {             /* any of the box in standing poison or a flow that is up */
    const SdLevel *L = SD.L;
    for (int ty = sd_floor_px(top); ty <= sd_floor_px(y - 1.0f); ty++)
        for (int tx = sd_floor_px(x - SD_HW); tx <= sd_floor_px(x + SD_HW - 1.0f); tx++)
            if (sd_tile(tx, ty) == '~' && y - 1.0f >= (float)(ty * SD_TILE) + 4.0f) return 1;
    if (sd_in_current_poison(x, y - 1.0f) || sd_in_current_poison(x, top + 1.0f)) return 1;
    for (int i = 0; i < L->nth; i++) {
        const SdThing *t = &L->th[i];
        if (t->kind != SDT_FLOW || sd_flow_k(t) <= 0.05f) continue;
        float x0 = (float)(t->x * SD_TILE), x1 = (float)((t->x + t->w) * SD_TILE), y1 = (float)((t->y + t->h) * SD_TILE);
        if (x + SD_HW > x0 + 2.0f && x - SD_HW < x1 - 2.0f && y > sd_flow_top(t) + 2.0f && top < y1) return 1;
    }
    return 0;
}
static float sd_box_h(void) { return SD.crouch ? SD_HC : SD_HT; }

/* the platforms' places along their ways */
static void sd_plat_place(int i) {
    const SdThing *t = &SD.L->th[i];
    SdPlat *p = &SD.plat[i];
    float len = t->f[0] * SD_TILE, s = clampf(p->s, 0.0f, len), n = hypotf((float)t->a, (float)t->b);
    p->x = (float)(t->x * SD_TILE) + (float)t->a / n * s;
    p->y = (float)(t->y * SD_TILE) + (float)t->b / n * s;
}
static void sd_plats(float dt) {
    const SdLevel *L = SD.L;
    for (int i = 0; i < L->nth; i++) {
        const SdThing *t = &L->th[i];
        if (t->kind != SDT_PLAT) continue;
        SdPlat *p = &SD.plat[i];
        p->px = p->x; p->py = p->y;
        float len = t->f[0] * SD_TILE;
        if (!SD.st[i]) continue;                                   /* stopped by a trigger, or OFF till one */
        if (p->wait > 0.0f) p->wait -= dt;
        else {
            p->s += (float)p->dir * t->f[1] * SD_TILE * dt;
            if (p->s >= len) {
                p->s = len; p->wait = t->f[2];
                if (t->flags & SDF_LOOP) { p->s = 0.0f; } else p->dir = -1;
            } else if (p->s <= 0.0f) { p->s = 0.0f; p->dir = 1; p->wait = t->f[2]; }
        }
        sd_plat_place(i);
    }
}
static int sd_on_plat(float x, float y, float prev_y, int *which, float *top) {   /* landing on a platform's top from above */
    const SdLevel *L = SD.L;
    for (int i = 0; i < L->nth; i++) {
        const SdThing *t = &L->th[i];
        if (t->kind != SDT_PLAT) continue;
        const SdPlat *p = &SD.plat[i];
        float w = (float)(t->w * SD_TILE);
        if (x + SD_HW <= p->x || x - SD_HW >= p->x + w) continue;
        if (prev_y <= p->py + 0.5f && y >= p->y) { *which = i; *top = p->y; return 1; }
    }
    return 0;
}

/* ---- the caster ---- */
static void sd_life_lost(void);
static void sd_hurt(float dmg, int el, float from_x) {
    if (SD.dying || SD.inv_t > 0.0f || dmg <= 0.0f) return;
    const Gear *g = &RUN.gear;
    int phys = el == EL_PHYS;
    float d = dmg;
    if (!phys) d *= 1.0f - el_resist(g, el);
    d *= 1.0f - armour_keep(g, phys);
    int n = (int)(d + 0.5f);
    if (n < 1) n = 1;
    G.hp -= n;
    SD.hurt_t = 0.35f; SD.inv_t = SDT[TU_HURT_SAFE];
    SD.vx = (SD.x < from_x ? -1.0f : 1.0f) * SDT[TU_KNOCK_X]; SD.vy = -SDT[TU_KNOCK_Y]; SD.ground = 0; SD.on_plat = -1;
    rumble(0.3f, 0.2f, 0.2f);
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT side_hurt %d el %d hp=%d/%d t=%.2f\n", n, el, G.hp, G.max_hp, SD.t);
#endif
    if (G.hp > 0) { snd_sfx(SFX_HURT); return; }
    G.hp = 0; SD.dying = 1; SD.die_t = 0.0f; SD.vx *= 0.4f;
    snd_sfx(SFX_DEATH);
}
static void sd_move_x(float dx) {
    float h = sd_box_h();
    SD.x += dx;
    int r0 = sd_floor_px(SD.y - h), r1 = sd_floor_px(SD.y - 0.5f);
    if (dx > 0.0f) {
        int c = sd_floor_px(SD.x + SD_HW - 0.01f);
        for (int r = r0; r <= r1; r++) if (sd_solid(c, r)) { SD.x = (float)(c * SD_TILE) - SD_HW; SD.vx = 0.0f; break; }
    } else if (dx < 0.0f) {
        int c = sd_floor_px(SD.x - SD_HW);
        for (int r = r0; r <= r1; r++) if (sd_solid(c, r)) { SD.x = (float)((c + 1) * SD_TILE) + SD_HW; SD.vx = 0.0f; break; }
    }
}
static void sd_move_y(float dy) {
    float prev = SD.y, h = sd_box_h();
    SD.y += dy;
    int c0 = sd_floor_px(SD.x - SD_HW + 0.01f), c1 = sd_floor_px(SD.x + SD_HW - 0.01f);
    if (dy > 0.0f) {
        int r = sd_floor_px(SD.y - 0.001f);
        float top = (float)(r * SD_TILE);
        for (int c = c0; c <= c1; c++) {
            int ch = sd_tile(c, r);
            if (sd_solid(c, r) || (ch == '=' && !SD.drop && prev <= top + 0.5f)) {
                if (prev <= top + 0.5f || sd_solid(c, r)) { SD.y = top; SD.vy = 0.0f; SD.ground = 1; SD.on_plat = -1; break; }
            }
        }
        int pi; float pt;
        if (!SD.ground && sd_on_plat(SD.x, SD.y, prev, &pi, &pt)) { SD.y = pt; SD.vy = 0.0f; SD.ground = 1; SD.on_plat = pi; }
    } else if (dy < 0.0f) {
        int r = sd_floor_px(SD.y - h);
        for (int c = c0; c <= c1; c++) if (sd_solid(c, r)) { SD.y = (float)((r + 1) * SD_TILE) + h; SD.vy = 0.0f; break; }
    }
}
static int sd_stands(void) {                                       /* something under the feet now */
    int c0 = sd_floor_px(SD.x - SD_HW + 0.01f), c1 = sd_floor_px(SD.x + SD_HW - 0.01f), r = sd_floor_px(SD.y + 0.5f);
    if (fmodf(SD.y, (float)SD_TILE) < 0.6f || fmodf(SD.y, (float)SD_TILE) > SD_TILE - 0.6f)
        for (int c = c0; c <= c1; c++) if (sd_solid(c, r) || (sd_tile(c, r) == '=' && !SD.drop)) return 1;
    if (SD.on_plat >= 0) {
        const SdThing *t = &SD.L->th[SD.on_plat]; const SdPlat *p = &SD.plat[SD.on_plat];
        if (SD.x + SD_HW > p->x && SD.x - SD_HW < p->x + (float)(t->w * SD_TILE) && fabsf(SD.y - p->y) < 1.5f) return 1;
    }
    return 0;
}
static void sd_cast(int held_release);
static void sd_caster(float dt) {
    SdIn *in = &SD.in;
    if (SD.dying) {                                                /* falling over, sliding to a stop */
        SD.vx *= expf(-dt * 4.0f); SD.vy = fminf(SD_FALL_MAX, SD.vy + SD_G * dt);
        sd_move_x(SD.vx * dt); SD.ground = 0; sd_move_y(SD.vy * dt);
        return;
    }
    /* riding: carried by what is under you */
    if (SD.ground && SD.on_plat >= 0) { const SdPlat *p = &SD.plat[SD.on_plat]; sd_move_x(p->x - p->px); SD.y += p->y - p->py; }
    int want_crouch = in->down && SD.ground;
    if (!want_crouch && SD.crouch) {                               /* up again, if there is room */
        int r = sd_floor_px(SD.y - SD_HT), c0 = sd_floor_px(SD.x - SD_HW + 0.01f), c1 = sd_floor_px(SD.x + SD_HW - 0.01f), room = 1;
        for (int c = c0; c <= c1; c++) if (sd_solid(c, r)) room = 0;
        if (room) SD.crouch = 0;
    } else if (want_crouch) SD.crouch = 1;
    int dir = SD.hold_t > 0.0f ? 0 : (in->right ? 1 : 0) - (in->left ? 1 : 0);   /* held by a snare: still */
    if (SD.hold_t > 0.0f) in->jump_hit = 0;
    if (dir) SD.face = dir;
    float top = SD.crouch ? SD_CREEP : in->run ? SD_RUN : SD_WALK;
    if (SD.pois_k > 0.0f) top *= SDT[TU_WADE];                      /* wading in poison */
    float want = (float)dir * top, acc = SD.ground ? (dir ? SDT[TU_ACCEL] : SDT[TU_BRAKE]) : SDT[TU_AIR];
    if (SD.vx < want) SD.vx = fminf(want, SD.vx + acc * dt); else if (SD.vx > want) SD.vx = fmaxf(want, SD.vx - acc * dt);
    /* the jump: a short grace off an edge, and one pressed just before landing still counts */
    SD.coyote = SD.ground ? SD_COYOTE : SD.coyote - dt;
    SD.jbuf = in->jump_hit ? SD_JBUF : SD.jbuf - dt;
    SD.drop = 0;
    if (SD.jbuf > 0.0f && in->down && SD.ground && SD.on_plat < 0) {  /* down and jump on a ledge: through it */
        int r = sd_floor_px(SD.y + 0.5f), c = sd_floor_px(SD.x);
        if (sd_tile(c, r) == '=') { SD.drop = 1; SD.ground = 0; SD.y += 2.0f; SD.jbuf = 0.0f; }
    }
    if (SD.jbuf > 0.0f && SD.coyote > 0.0f && !SD.crouch) {
        SD.vy = -SD_JUMP; SD.ground = 0; SD.on_plat = -1; SD.coyote = 0.0f; SD.jbuf = 0.0f;
        snd_sfx_ex(SFX_WHOOSH, 0.25f, 0.5f);
    }
    if (!SD.ground) SD.vy = fminf(SD_FALL_MAX, SD.vy + (in->jump && SD.vy < 0.0f ? SD_G_HELD : SD_G) * dt);
    int was = SD.ground;
    sd_move_x(SD.vx * dt);
    SD.ground = 0;
    if (SD.vy != 0.0f || !was) sd_move_y(SD.vy * dt);
    if (!SD.ground && was && SD.vy >= 0.0f) {                      /* walking: still on something? */
        if (sd_stands()) SD.ground = 1; else if (SD.vy == 0.0f) SD.vy = 1.0f;
    }
    if (!SD.ground) SD.on_plat = -1;
    if (SD.ground && !was && SD.vy == 0.0f) snd_sfx_ex(SFX_STEP, 0.3f, 0.5f);
}

/* ---- harm from the place ---- */
static void sd_place_harm(float dt) {
    if (SD.dying) return;
    float h = sd_box_h(), top = SD.y - h;
    for (int ty = sd_floor_px(top); ty <= sd_floor_px(SD.y - 1.0f); ty++)   /* spikes: the lower part of their tile */
        for (int tx = sd_floor_px(SD.x - SD_HW + 1.0f); tx <= sd_floor_px(SD.x + SD_HW - 1.0f); tx++)
            if (sd_tile(tx, ty) == '^' && SD.y > (float)(ty * SD_TILE) + 7.0f) {
                G.blame = (Blame){ BL_HAZARD, 0, 0 };
                sd_hurt(SDT[TU_SPIKES] * (float)G.max_hp, EL_PHYS, SD.x - (float)SD.face);
                if (!SD.dying) { SD.vy = -200.0f; SD.ground = 0; }
                return;
            }
    int in = sd_in_poison(SD.x, SD.y, top);
    SD.pois_k = in ? 1.0f : 0.0f;
    if (in) {
        SD.held += dt;
        if (SD.held >= SDT[TU_BITE_EVERY]) {                       /* a share of your health each while */
            SD.held = 0.0f;
            int n = imax(1, (int)(SDT[TU_BITE] * (float)G.max_hp * (1.0f - el_resist(&RUN.gear, EL_POISON)) + 0.5f));
            G.hp -= n; snd_sfx_ex(SFX_HURT, 0.5f, 0.5f);
            if (G.hp <= 0) { G.hp = 0; SD.dying = 1; SD.die_t = 0.0f; snd_sfx(SFX_DEATH); }
        }
    } else SD.held = SDT[TU_BITE_EVERY] * 0.8f;                   /* the first bite comes quick */
    if (SD.y > (float)(SD.L->h * SD_TILE + 48)) {                  /* down past the foot: gone */
        G.hp = imax(0, G.hp); SD.dying = 1; SD.die_t = 1.0f;
#ifdef CRAWL_DEBUG
        fprintf(stderr, "EVENT side_pit t=%.2f\n", SD.t);
#endif
    }
}

/* ---- the falling blocks ---- */
static void sd_falls(float dt) {
    const SdLevel *L = SD.L;
    for (int i = 0; i < L->nth; i++) {
        const SdThing *t = &L->th[i];
        if (t->kind != SDT_FALL) continue;
        SdFall *f = &SD.fall[i];
        switch (f->state) {
        case 0:                                                    /* in place: stood on, it starts to go */
            if (SD.ground && SD.on_plat < 0 && fabsf(SD.y - (float)(t->y * SD_TILE)) < 1.0f
                && SD.x + SD_HW > (float)(t->x * SD_TILE) && SD.x - SD_HW < (float)((t->x + 1) * SD_TILE)) {
                f->state = 1; f->t = 0.0f;
                snd_sfx_ex(SFX_BONES, 0.4f, 0.5f);
            }
            break;
        case 1: if ((f->t += dt) >= t->f[0]) { f->state = 2; f->t = 0.0f; f->vy = 0.0f; snd_sfx_ex(SFX_KNOCK, 0.5f, 0.5f); } break;
        case 2:
            f->vy = fminf(420.0f, f->vy + SD_G * dt); f->dy += f->vy * dt; f->t += dt;
            if (f->t > 2.5f) { f->state = 3; f->t = 0.0f; }
            break;
        case 3: {                                                  /* gone: back when its time is up and nothing stands in its place */
            float x0 = (float)(t->x * SD_TILE), y0 = (float)(t->y * SD_TILE);
            int clear = !(SD.x + SD_HW > x0 && SD.x - SD_HW < x0 + SD_TILE && SD.y > y0 && SD.y - sd_box_h() < y0 + SD_TILE);
            if ((f->t += dt) >= t->f[1] && clear) { f->state = 0; f->dy = 0.0f; f->vy = 0.0f; }
            break;
        }
        }
    }
}

/* ---- the creatures: the run's own, from the crypt's numbers ---- */
static int sd_flier(int k) { return k == MOB_BAT || k == MOB_GHOST || k == MOB_SHADE || k == MOB_WRAITH || k == MOB_IMP || k == MOB_WSKULL || k == MOB_LIGHTLESS; }
static int sd_thrower(int k) { return k == MOB_SKEL || k == MOB_HUNTER; }
static void sd_mob_make(int thing) {
    const SdThing *t = &SD.L->th[thing];
    int k = t->a;
    if (k < 0) {                                                   /* the run's choice, on the side's own dice */
        uint32_t keep = G.rs; G.rs = SD.rs;
        k = random_type();
        SD.rs = G.rs; G.rs = keep;
    }
    for (int i = 0; i < SD_MOBS; i++) {
        SdMob *m = &SD.mob[i];
        if (m->alive) continue;
        memset(m, 0, sizeof *m);
        Mob tmp; memset(&tmp, 0, sizeof tmp); tmp.type = k;
        uint32_t keep = G.rs; G.rs = SD.rs;
        int rank = t->b >= 0 ? clampi(t->b, 0, KINDS[k].top) : pick_rank(k);
        SD.rs = G.rs; G.rs = keep;
        set_rank(&tmp, rank);
        m->alive = 1; m->kind = k; m->rank = rank; m->thing = thing;
        m->hp = m->maxhp = tmp.maxhp; m->dmgk = tmp.dmgk; m->speed = tmp.speed;
        m->fly = sd_flier(k);
        m->x = m->home_x = (float)(t->x * SD_TILE) + 8.0f; m->y = m->home_y = (float)((t->y + 1) * SD_TILE);
        if (m->fly) m->y -= 4.0f;
        m->face = -1; m->cool = 1.0f + sd_rndf();
        return;
    }
}
static float sd_mob_h(const SdMob *m) { return m->kind == MOB_BAT || m->kind == MOB_RAT || m->kind == MOB_SPIDER ? 14.0f : m->kind == MOB_HOUND ? 18.0f : 26.0f; }
static float sd_blow(const SdMob *m) { return (4.0f + 0.8f * (float)G.level) * m->dmgk; }
static void sd_shot_new(int kind, float x, float y, float vx, float vy, float dmg, float life, float g, int foe, int sp, int tier) {
    for (int i = 0; i < SD_SHOTS; i++) {
        SdShot *s = &SD.shot[i];
        if (s->alive) continue;
        *s = (SdShot){ 1, kind, sp, sp >= 0 && sp < NATK ? ATK_ELEM[sp] : EL_PHYS, tier, foe, x, y, vx, vy, life, dmg, g };
        return;
    }
}
static void sd_mob_slain(SdMob *m, int sp) {
    m->state = SDM_DIE; m->t = 0.0f;
    SDP.slain[SD.side][m->thing] = 1;
    Mob tmp; memset(&tmp, 0, sizeof tmp);
    tmp.type = m->kind; tmp.rank = m->rank; tmp.last_atk = sp; tmp.x = G.px; tmp.y = G.py;
    if (!KINDS[m->kind].flags || !(KINDS[m->kind].flags & KF_UNCOUNTED)) { rec_kill(&tmp); xp_kill(&tmp); }
    gain_souls(RANK_SOULS[m->rank] * soul_share());
    snd_sfx_ex(KINDS[m->kind].sfx_die, 0.8f, 0.5f);
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT side_slain %s rank %d by %s t=%.2f\n", KINDS[m->kind].name, m->rank, sp >= 0 && sp < NATK ? ATK_NAME[sp] : "?", SD.t);
#endif
}
static void sd_mob_hit(SdMob *m, float dmg, int sp, int el) {
    if (m->state == SDM_DIE || !m->alive) return;
    if ((KINDS[m->kind].flags & KF_SPIRIT) && el != EL_SPIRIT) return;   /* nothing carried hurts a spirit but spirit */
    if (el >= 0 && el < NELEM) dmg *= KINDS[m->kind].weak[el];
    if (el == EL_FROST) m->slow_t = 2.0f;
    if (el == EL_POISON) { m->pois_t = 4.0f; m->pois_dps = dmg * 0.75f / 4.0f; dmg *= 0.25f; }
    m->hp -= dmg; m->hurt_t = 0.2f;
    m->vx += (m->x < SD.x ? -1.0f : 1.0f) * 40.0f;
    if (m->hp <= 0.0f) sd_mob_slain(m, sp);
}
static int sd_line_clear(float x0, float y0, float x1, float y1) {
    float d = hypotf(x1 - x0, y1 - y0);
    int n = (int)(d / 4.0f) + 1;
    for (int i = 1; i < n; i++) { float k = (float)i / (float)n; if (sd_solid(sd_floor_px(x0 + (x1 - x0) * k), sd_floor_px(y0 + (y1 - y0) * k))) return 0; }
    return 1;
}
static void sd_mobs(float dt) {
    for (int i = 0; i < SD_MOBS; i++) {
        SdMob *m = &SD.mob[i];
        if (!m->alive) continue;
        m->t += dt; m->hurt_t = fmaxf(0.0f, m->hurt_t - dt); m->slow_t = fmaxf(0.0f, m->slow_t - dt);
        if (m->state == SDM_DIE) { if (m->t > 0.8f) m->alive = 0; continue; }
        if (m->pois_t > 0.0f) { m->pois_t -= dt; m->hp -= m->pois_dps * dt; if (m->hp <= 0.0f) { sd_mob_slain(m, AT_FLASK); continue; } }
        float sp = m->speed * SDT[TU_DEAD_PACE] * (m->slow_t > 0.0f ? 0.5f : 1.0f);   /* the crypt's cells a second as pixels, a little slower */
        float mh = sd_mob_h(m), dx = SD.x - m->x, dy = (SD.y - SD_HT * 0.5f) - (m->y - mh * 0.5f);
        int sees = !SD.dying && fabsf(dx) < SDT[TU_DEAD_SIGHT] && fabsf(dy) < 80.0f && sd_line_clear(m->x, m->y - mh * 0.6f, SD.x, SD.y - SD_HT * 0.6f);
        m->cool -= dt;
        if (m->fly) {                                              /* drift about home; seen, come at you and swoop (its middle at the caster's) */
            float tx = sees ? SD.x : m->home_x + 30.0f * sinf(m->t * 0.7f + (float)i), ty = sees ? SD.y - SD_HT * 0.5f + mh * 0.5f : m->home_y + 10.0f * sinf(m->t * 1.3f);
            float ax = tx - m->x, ay = ty - m->y, d = fmaxf(1.0f, hypotf(ax, ay));
            m->vx += (ax / d * sp * 1.2f - m->vx) * fminf(1.0f, dt * 2.5f);
            m->vy += (ay / d * sp * 1.2f - m->vy) * fminf(1.0f, dt * 2.5f);
            float nx = m->x + m->vx * dt, ny = m->y + m->vy * dt;
            if (!sd_solid(sd_floor_px(nx), sd_floor_px(m->y - mh * 0.5f))) m->x = nx; else m->vx = -m->vx * 0.5f;
            if (!sd_solid(sd_floor_px(m->x), sd_floor_px(ny - mh * 0.5f))) m->y = ny; else m->vy = -m->vy * 0.5f;
        } else {                                                   /* walk the floor: along it to its edge, at you when seen */
            int want = sees ? (dx > 0.0f ? 1 : -1) : m->face;
            if (sees && sd_thrower(m->kind) && fabsf(dx) < 110.0f && fabsf(dx) > 36.0f) want = 0;   /* a thrower stands off */
            if (want) m->face = want;
            m->vx = (float)want * sp * (sees ? 1.0f : 0.5f);
            float nx = m->x + m->vx * dt + (float)m->face * 7.0f;
            int c = sd_floor_px(nx), r = sd_floor_px(m->y - 1.0f), below = sd_floor_px(m->y + 1.0f);
            int ahead_floor = sd_solid(c, below) || sd_tile(c, below) == '=';
            int ahead_bad = sd_tile(c, below) == '^' || sd_tile(c, r) == '~' || sd_tile(c, below) == '~';
            if (sd_solid(c, r) || sd_solid(c, sd_floor_px(m->y - mh)) || !ahead_floor || ahead_bad) {
                if (!sees) m->face = -m->face;
                m->vx = 0.0f;
            } else m->x += m->vx * dt;
            if (!(sd_solid(sd_floor_px(m->x), below) || sd_tile(sd_floor_px(m->x), below) == '=')) {   /* fallen off something */
                m->vy = fminf(SD_FALL_MAX, m->vy + SD_G * dt); m->y += m->vy * dt;
                if (sd_solid(sd_floor_px(m->x), sd_floor_px(m->y))) { m->y = (float)(sd_floor_px(m->y) * SD_TILE); m->vy = 0.0f; }
                if (m->y > (float)(SD.L->h * SD_TILE + 64)) m->alive = 0;
            }
            if (sees && sd_thrower(m->kind) && m->cool <= 0.0f && fabsf(dx) < 140.0f) {   /* a bone, lobbed */
                float t = fmaxf(0.5f, fabsf(dx) / 150.0f);
                sd_shot_new(SS_BONE, m->x, m->y - mh * 0.7f, dx / t, (dy - 0.5f * SD_G * 0.6f * t * t) / t, sd_blow(m), 3.0f, SD_G * 0.6f, 1, -1, 0);
                m->cool = fmaxf(0.9f, 2.2f - 0.03f * (float)G.level) + sd_rndf();
                snd_sfx_ex(SFX_WHOOSH, 0.3f, 0.5f);
            }
        }
        /* a touch: its blow */
        if (!SD.dying && fabsf(SD.x - m->x) < SD_HW + 6.0f && SD.y > m->y - mh && SD.y - sd_box_h() < m->y && m->cool <= 0.0f) {
            G.blame = (Blame){ BL_MOB, m->kind, m->rank };
            int el = EL_PHYS;
            for (int e = 0; e < NELEM; e++) if (KINDS[m->kind].harm >> e & 1) { el = e; break; }
            sd_hurt(sd_blow(m), el, m->x);
            m->cool = 1.0f;
        }
    }
}

/* ---- shots: the caster's spells and the dead's bones ---- */
static void sd_burst(SdShot *s) {                                  /* the flask: a splash a tile and a half round */
    for (int i = 0; i < SD_MOBS; i++) {
        SdMob *m = &SD.mob[i];
        if (!m->alive || m->state == SDM_DIE) continue;
        if (hypotf(m->x - s->x, (m->y - sd_mob_h(m) * 0.5f) - s->y) < 26.0f) sd_mob_hit(m, s->dmg, s->sp, EL_POISON);
    }
    snd_sfx_ex(SFX_SPLASH, 0.7f, 0.5f);
    s->alive = 0;
}
static void sd_shots(float dt) {
    for (int i = 0; i < SD_SHOTS; i++) {
        SdShot *s = &SD.shot[i];
        if (!s->alive) continue;
        s->vy += s->g * dt;
        float ox = s->x, oy = s->y;
        s->x += s->vx * dt; s->y += s->vy * dt;
        s->life -= dt;
        if (s->life <= 0.0f) { if (s->kind == SS_FLASK) sd_burst(s); else s->alive = 0; continue; }
        if (sd_solid(sd_floor_px(s->x), sd_floor_px(s->y))) {
            if (s->kind == SS_FLASK) { s->x = ox; s->y = oy; sd_burst(s); } else s->alive = 0;
            continue;
        }
        if (s->foe) {                                              /* a bone at the caster */
            if (!SD.dying && fabsf(s->x - SD.x) < SD_HW + 2.0f && s->y < SD.y && s->y > SD.y - sd_box_h()) {
                G.blame = (Blame){ BL_MOB, MOB_SKEL, 0 };
                sd_hurt(s->dmg, EL_PHYS, s->x - s->vx);
                s->alive = 0;
            }
            continue;
        }
        for (int k = 0; k < SD_MOBS; k++) {
            SdMob *m = &SD.mob[k];
            if (!m->alive || m->state == SDM_DIE) continue;
            float mh = sd_mob_h(m);
            if (fabsf(s->x - m->x) < 8.0f && s->y < m->y + 2.0f && s->y > m->y - mh) {
                if (s->kind == SS_FLASK) { sd_burst(s); break; }
                sd_mob_hit(m, s->dmg, s->sp, s->el);
                s->alive = 0;
                break;
            }
        }
    }
}
/* How far each element goes, in tiles (the owner, 1.2): fire short, ice slower and a fifth further, lightning at once to five,
 * the bone till it strikes, spirit three for now; the flask where its mark says. */
static float sd_reach(int el) {
    switch (el) {
    case EL_FIRE: return SDT[TU_FIRE_REACH];
    case EL_FROST: return SDT[TU_FROST_REACH];
    case EL_LIGHT: return SDT[TU_LIGHT_REACH];
    case EL_SPIRIT: return SDT[TU_SPIRIT_REACH];
    default: return 40.0f;
    }
}
static float sd_speed(int el) { return el == EL_FIRE ? SDT[TU_FIRE_SPEED] : el == EL_FROST ? SDT[TU_FROST_SPEED] : el == EL_SPIRIT ? SDT[TU_SPIRIT_SPEED] : SDT[TU_BOLT_SPEED]; }
static float sd_flask_k(void) { return clampf(SD.held / SDT[TU_FLASK_HOLD], 0.2f, 1.0f); }   /* the throw grows as it is held, over a second */
static void sd_flask_way(float k, float *vx, float *vy) { *vx = (float)SD.face * (SDT[TU_FLASK_NEAR] + (SDT[TU_FLASK_FAR] - SDT[TU_FLASK_NEAR]) * k); *vy = -SDT[TU_FLASK_LIFT] - 60.0f * k; }
static void sd_flask_mark(void) {                                  /* where the flask would land now: its arc tried */
    float vx, vy, x = SD.x + (float)SD.face * 6.0f, y = SD.y - SD_HAND;
    sd_flask_way(sd_flask_k(), &vx, &vy);
    for (int i = 0; i < 400; i++) {
        float nx = x + vx * 0.01f, ny = y + vy * 0.01f;
        vy += SD_G * 0.01f;
        if (sd_solid(sd_floor_px(nx), sd_floor_px(ny)) || ny > (float)(SD.L->h * SD_TILE)) break;
        x = nx; y = ny;
    }
    SD.mark_x = x; SD.mark_y = y;
}
static void sd_cast(int release) {
    int sp = G.spell;
    if (sp < 0 || sp >= NATK || RUN.gear.atk[sp] < 0) { sp = own_spell(); if (sp < 0) return; G.spell = sp; }
    if (SD.cd > 0.0f || SD.dying) return;
    if (SD.silence_t > 0.0f) { if (G.msg_t <= 0.0f) say("THE VOID TAKES THE SPELL FROM YOUR HAND"); SD.cd = 0.3f; return; }
    const Gear *g = &RUN.gear;
    int el = ATK_ELEM[sp];
    if (sp == AT_BFS) {                                            /* the BFS9000: everything on the screen */
        if (RUN.souls < bfs_cost(RUN.gear.atk[AT_BFS])) { say("THE BFS9000 NEEDS %d SOULS", bfs_cost(RUN.gear.atk[AT_BFS])); return; }
        RUN.souls -= bfs_cost(RUN.gear.atk[AT_BFS]);
        for (int i = 0; i < SD_MOBS; i++) {
            SdMob *m = &SD.mob[i];
            if (m->alive && m->state != SDM_DIE && fabsf(m->x - SD.camx - 160.0f) < 170.0f && fabsf(m->y - SD.camy - 78.0f) < 90.0f
                && !(KINDS[m->kind].flags & KF_SPIRIT)) { m->hp = 0.0f; sd_mob_slain(m, AT_BFS); }
        }
        snd_sfx_ex(SFX_IMPLODE, 1.3f, 0.5f); rumble(1.0f, 1.0f, 0.7f);
        SD.cd = cast_cycle(sp); SD.cast_t = 0.3f;
        return;
    }
    int splinter = sp == AT_SPEAR && spear_bare();
    if (splinter && splint_left() <= 0) { say("NO BONES, AND NO SPLINTERS YET"); SD.cd = 0.3f; return; }
    int tier = spell_tier(sp);
    if (!spend_spirit(spell_cost(g, sp, tier) * (1.0f + aug_cost(sp)))) {
        tier = 1;
        if (!G.spirit_said) { G.spirit_said = 1; say("LOW ON SPIRIT: SPELLS CAST WEAK"); }
    }
    if (sp == AT_SPEAR) { if (splinter) RUN.splint_used++; else if (rndf() >= bone_spared()) RUN.gear.bones--; }
    float dmg = spell_damage_at(g, sp, tier) * aug_dmg(sp) * (splinter ? SPLINTER_DMG : 1.0f);
    float hx = SD.x + (float)SD.face * 6.0f, hy = SD.y - SD_HAND;
    SD.cd = cast_cycle(sp); SD.cast_t = 0.3f;
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT side_cast %s tier %d t=%.2f\n", ATK_NAME[sp], tier, SD.t);
#endif
    switch (el) {
    case EL_LIGHT: {                                               /* at once, five tiles: the first in the way, and two more near it */
        float reach = sd_reach(el) * SD_TILE, end = hx + (float)SD.face * reach;
        for (float x = hx; fabsf(x - hx) < reach; x += (float)SD.face * 2.0f) if (sd_solid(sd_floor_px(x), sd_floor_px(hy))) { end = x; break; }
        SdMob *first = NULL;
        float best = 1e9f;
        for (int i = 0; i < SD_MOBS; i++) {
            SdMob *m = &SD.mob[i];
            if (!m->alive || m->state == SDM_DIE) continue;
            float d = (m->x - hx) * (float)SD.face;
            if (d < -4.0f || d > fabsf(end - hx) + 6.0f || hy < m->y - sd_mob_h(m) - 4.0f || hy > m->y + 4.0f) continue;
            if (d < best) { best = d; first = m; }
        }
        SD.beam_t = 0.14f; SD.bx0 = hx; SD.by0 = hy; SD.bx1 = first ? first->x : end; SD.by1 = first ? first->y - sd_mob_h(first) * 0.5f : hy;
        if (first) {
            float fx = first->x, fy = first->y;
            sd_mob_hit(first, dmg, sp, el);
            for (int hop = 0, done = 0; hop < SD_MOBS && done < 2; hop++) {
                SdMob *m = &SD.mob[hop];
                if (m == first || !m->alive || m->state == SDM_DIE || hypotf(m->x - fx, m->y - fy) > 48.0f) continue;
                sd_mob_hit(m, dmg * 0.6f, sp, el); done++;
            }
        }
        snd_sfx_ex(SFX_ZAP, 0.8f, 0.5f);
        break;
    }
    case EL_POISON: {                                              /* lobbed, where the mark showed */
        float vx, vy; sd_flask_way(release ? sd_flask_k() : 0.6f, &vx, &vy);
        sd_shot_new(SS_FLASK, hx, hy, vx, vy, dmg, 4.0f, SD_G, 0, sp, tier);
        snd_sfx_ex(SFX_WHOOSH, 0.5f, 0.5f);
        break;
    }
    case EL_SPIRIT:
        if (sp == AT_DRAIN) {                                      /* the drain: the nearest before you, three tiles */
            SdMob *best = NULL; float bd = sd_reach(el) * SD_TILE;
            for (int i = 0; i < SD_MOBS; i++) {
                SdMob *m = &SD.mob[i];
                if (!m->alive || m->state == SDM_DIE) continue;
                float d = (m->x - SD.x) * (float)SD.face;
                if (d < -4.0f || fabsf(m->y - SD.y) > 30.0f || d > bd) continue;
                bd = d; best = m;
            }
            if (best) {
                float before = best->hp;
                sd_mob_hit(best, dmg, sp, el);
                float heal = fminf((before - fmaxf(best->hp, 0.0f)) * 0.5f, (float)(G.max_hp - G.hp));
                G.hp += (int)heal;
                SD.beam_t = 0.18f; SD.bx0 = hx; SD.by0 = hy; SD.bx1 = best->x; SD.by1 = best->y - sd_mob_h(best) * 0.5f;
            }
            snd_sfx_ex(SFX_ZAP, 0.5f, 0.5f);
            break;
        }
        /* the wisps: fly, as the rest */
        /* fallthrough */
    default: {
        float reach = sd_reach(el) * SD_TILE, v = sd_speed(el);
        sd_shot_new(SS_BOLT, hx, hy, (float)SD.face * v, 0.0f, dmg * (sp == AT_WISPS ? (float)wisp_count(tier) : 1.0f), reach / v, 0.0f, 0, sp, tier);
        snd_sfx_ex(el == EL_FIRE ? SFX_IGNITE : el == EL_FROST ? SFX_FROST : SFX_CAST, 0.6f, 0.5f);
        break;
    }
    }
}

/* ---- what is found ---- */
static void sd_take(int i) {
    const SdThing *t = &SD.L->th[i];
    SD.taken[i] = 1; SDP.taken[SD.side][i] = 1;
    switch (t->kind) {
    case SDT_GOLD: { Item it; memset(&it, 0, sizeof it); it.alive = 1; it.type = t->a; it.secret = 1; collect(&it); break; }
    case SDT_KEY: G.has_key[t->a] = 1; snd_sfx(SFX_KEYGET); say("A %s KEY", t->a == 0 ? "GOLD" : t->a == 1 ? "SILVER" : "CRIMSON"); break;
    case SDT_LIFE:
        if (RUN.sd_lost > 0) { RUN.sd_lost--; say("A LIFE: %d NOW", sd_lives()); } else { gain_souls(5.0f); say("LIVES FULL: FIVE SOULS"); }
        snd_sfx_ex(SFX_SHIMMER, 0.9f, 0.5f);
        break;
    default: break;
    }
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT side_take %d kind %d t=%.2f\n", i, t->kind, SD.t);
#endif
}
static void sd_throw(int i) {
    const SdThing *t = &SD.L->th[i];
    int on = (t->flags & SDF_TOGGLE) ? !SD.thrown[i] : 1;
    if (!(t->flags & SDF_TOGGLE) && SD.thrown[i]) return;
    SD.thrown[i] = (uint8_t)on; SDP.thrown[SD.side][i] = (uint8_t)on;
    snd_sfx_ex(SFX_KNOCK, 0.7f, 0.5f); snd_sfx_ex(SFX_DEEPCHIME, 0.5f, 0.5f);
    if (t->a >= 0 && ev_live() && t->a < drawn()->nthing) ev_touch(TG_USE, t->a);   /* the crypt's own triggers on that thing */
    say(on ? "SOMEWHERE IN THE CRYPT, SOMETHING MOVES" : "SOMETHING IN THE CRYPT GOES BACK");
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT side_switch %d thing %d %s t=%.2f\n", i, t->a, on ? "on" : "off", SD.t);
#endif
}
static int sd_at_thing(int i, float rx, float ry) {                /* the caster's box over a thing, give or take */
    const SdThing *t = &SD.L->th[i];
    float cx = (float)(t->x * SD_TILE) + 8.0f, cy = (float)(t->y * SD_TILE) + 8.0f;
    if (t->kind == SDT_GOLD || t->kind == SDT_KEY || t->kind == SDT_LIFE) { cx = SD.ix[i]; cy = SD.iy[i]; }   /* (where it lies now: loose things move) */
    return fabsf(SD.x - cx) < rx && SD.y - sd_box_h() * 0.5f > cy - ry && SD.y - sd_box_h() * 0.5f < cy + ry + 8.0f;
}
static int sd_door_here(void) {                                    /* the doorway stood before, -1 none */
    const SdLevel *L = SD.L;
    for (int i = 0; i < L->nth; i++)
        if (L->th[i].kind == SDT_DOOR && fabsf(SD.x - (float)(L->th[i].x * SD_TILE + 8)) < 8.0f && fabsf(SD.y - (float)((L->th[i].y + 1) * SD_TILE)) < 4.0f) return i;
    return -1;
}

/* ---- in and out ---- */
static void sd_build(void) {                                       /* the place as SDP and the entry left it */
    const SdLevel *L = SD.L;
    memset(SD.plat, 0, sizeof SD.plat); memset(SD.fall, 0, sizeof SD.fall);
    sd_works_build();
    memset(SD.mob, 0, sizeof SD.mob); memset(SD.shot, 0, sizeof SD.shot);
    memcpy(SD.taken, SDP.taken[SD.side], sizeof SD.taken); memcpy(SD.thrown, SDP.thrown[SD.side], sizeof SD.thrown);
    SD.rs = 0x51DE0000u ^ (uint32_t)(G.level * 7919 + SD.side * 104729) ^ ((uint32_t)G.retries << 20);
    for (int i = 0; i < L->nth; i++) {
        const SdThing *t = &L->th[i];
        if (t->kind == SDT_PLAT) { SdPlat *p = &SD.plat[i]; p->dir = 1; p->s = clampf(t->f[3], 0.0f, 1.0f) * t->f[0] * SD_TILE; sd_plat_place(i); p->px = p->x; p->py = p->y; }
        if (t->kind == SDT_MOB && !SDP.slain[SD.side][i]) sd_mob_make(i);
    }
}
static void sd_place_at(int link) {
    const SdLevel *L = SD.L;
    int d = -1;
    for (int i = 0; i < L->nth; i++) if (L->th[i].kind == SDT_DOOR && (d < 0 || L->th[i].a == link)) { if (L->th[i].a == link) { d = i; break; } if (d < 0) d = i; }
    const SdThing *t = &L->th[d];
    SD.x = (float)(t->x * SD_TILE) + 8.0f; SD.y = (float)((t->y + 1) * SD_TILE);
    SD.vx = SD.vy = 0.0f; SD.ground = 1; SD.on_plat = -1; SD.crouch = 0; SD.face = t->x < L->w / 2 ? 1 : -1;
    SD.dying = 0; SD.die_t = 0.0f; SD.inv_t = 1.0f; SD.cd = 0.4f; SD.in_t = 0.0f; SD.held = 0.0f; SD.holding = 0;
    SD.camx = clampf(SD.x - 160.0f, 0.0f, fmaxf(0.0f, (float)(L->w * SD_TILE - W)));
    SD.camy = clampf(SD.y - 100.0f, 0.0f, fmaxf(0.0f, (float)(L->h * SD_TILE - SD_VIEW_H)));
}
static int sd_enter(int side, int link, int cx, int cy) {
    if (side < 0 || side >= SD_SIDES || (side >= NSIDE_TEXT && !sdl_read[side])) return 0;   /* built in, or handed over by PLAY */
    side_art_init();                                               /* the tiles and the figure, made once */
    if (!sdl_read[side]) {
        char err[160];
        if (!sd_read(SIDE_TEXT[side], &SDL[side], err, sizeof err)) {
#ifdef CRAWL_DEBUG
            fprintf(stderr, "SIDE %d unreadable: %s\n", side, err);
#endif
            return 0;
        }
        sdl_read[side] = 1;
    }
    if (SDP.stamp != sd_stamp) { memset(&SDP, 0, sizeof SDP); SDP.stamp = sd_stamp; }
    SD.on = 1; SD.side = side; SD.link_in = link; SD.cx = cx; SD.cy = cy; SD.L = &SDL[side];
    for (int k = 0; k < NTU; k++) SDT[k] = (SD.L->tuned >> k & 1) ? SD.L->tune[k] : TUNE[k].def;   /* its own numbers, else the game's */
    SD.leaving = 0; SD.fade = 1.0f; SD.t = 0.0f;
    memcpy(SD0.taken, SDP.taken[side], sizeof SD0.taken); memcpy(SD0.thrown, SDP.thrown[side], sizeof SD0.thrown);
    memcpy(SD0.slain, SDP.slain[side], sizeof SD0.slain);
    memcpy(SD0.keys, G.has_key, sizeof SD0.keys); SD0.treasure = RUN.treasure; SD0.level_treasure = G.level_treasure;
    sd_build();
    sd_place_at(link);
    thing_drop_home();                                             /* what you carried stays in the crypt */
    snd_set_room(1);                                               /* the pockets' echo; the run's piece goes on under it */
    snd_sfx_ex(SFX_WHOOSH, 0.9f, 0.5f); snd_sfx_ex(SFX_SHIMMER, 0.5f, 0.5f);
    say("%s", SD.L->name);
    show_bar();
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT side_in %d link %d lives %d hp=%d/%d player=%d t=%.2f\n", side, link, sd_lives(), G.hp, G.max_hp, G.player, G.t);
#endif
    return 1;
}
static void sd_leave(int link, int goal) {
    SD.on = 0;
    snd_set_room(0);
    int k = -1;
    for (int i = 0; i < SDX.n; i++) if (SDX.d[i].side == SD.side && SDX.d[i].link == link) { k = i; break; }
    int x = k >= 0 ? SDX.d[k].x : SD.cx, y = k >= 0 ? SDX.d[k].y : SD.cy;
    stand_at_door(x, y);
    G.camz = 0.5f; G.spd = 0.0f; G.hurt = 0.0f;
    hunt_cx = -1;
    if (G.player) human_begin(); else { G.state = PS_MOVE; G.u = 1.0f; }
    if (goal && !SDP.done[SD.side]) {
        SDP.done[SD.side] = 1;
        if (RUN.sd_lost > 0) RUN.sd_lost--;
        xp_gain(60.0f * xp_level(), "A SIDE PASSAGE DONE");
        say("THE PASSAGE IS DONE: %d %s", sd_lives(), sd_lives() == 1 ? "LIFE" : "LIVES");
    }
    snd_sfx_ex(SFX_WHOOSH, 0.8f, 0.5f);
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT side_out %d link %d goal %d at (%d,%d) hp=%d/%d lives %d t=%.2f\n", SD.side, link, goal, x, y, G.hp, G.max_hp, sd_lives(), G.t);
#endif
}
static void sd_life_lost(void) {
    RUN.sd_lost = imin(SD_LIVES, RUN.sd_lost + 1);
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT side_life lost, %d left at %d,%d by %s t=%.2f\n", sd_lives(), sd_floor_px(SD.x), sd_floor_px(SD.y - 1.0f),
            SD.y > (float)(SD.L->h * SD_TILE) ? "the drop" : SD.pois_k > 0.0f ? "poison" : G.blame.what == BL_MOB ? KINDS[G.blame.type].name : "spikes", SD.t);
#endif
    if (sd_lives() <= 0) {                                         /* the last: the run ends, as a death in the crypt */
#ifdef CRAWL_DEBUG
        fprintf(stderr, "EVENT side_over t=%.2f\n", SD.t);
#endif
        SD.on = 0; snd_set_room(0);
        stand_at_door(SD.cx, SD.cy);
        RUN.sd_lost = 0;                                           /* a new run's three */
        if (!G.player) sd_dream_lost = G.level;
        G.blame = (Blame){ BL_NONE, 0, 0 };
        int wind = RUN.wind_spent; RUN.wind_spent = 1; G.lb_spent = 1;
        G.hp = 1; G.level_t = fmax(G.level_t, 2.0);
        hurt_ex(99999.0f, HURT_SPELL, G.px, G.py);
        RUN.wind_spent = wind;
        return;
    }
    /* back as it was when you came in: what was taken goes back, the switches too, and you whole at the doorway */
    memcpy(SDP.taken[SD.side], SD0.taken, sizeof SD0.taken); memcpy(SDP.thrown[SD.side], SD0.thrown, sizeof SD0.thrown);
    memcpy(SDP.slain[SD.side], SD0.slain, sizeof SD0.slain);
    memcpy(G.has_key, SD0.keys, sizeof SD0.keys); RUN.treasure = SD0.treasure; G.level_treasure = SD0.level_treasure;
    G.hp = G.max_hp; G.poison_t = 0.0f;
    sd_build();
    sd_place_at(SD.link_in);
    SD.fade = 1.0f;
    say("A LIFE LOST: %d LEFT", sd_lives());
}

/* ---- the frame ---- */
static void sd_dream(float dt);                                    /* side_dream.c */
static void sd_person(void) {
    SdIn *in = &SD.in;
    memset(in, 0, sizeof *in);
    in->left = IN.down[KEY_A] || IN.down[KEY_LEFT];
    in->right = IN.down[KEY_D] || IN.down[KEY_RIGHT];
    in->run = IN.down[KEY_SHIFT];
    in->jump = IN.down[KEY_SPACE]; in->jump_hit = IN.hit[KEY_SPACE];
    in->down = IN.down[KEY_S] || IN.down[KEY_DOWN];
    in->up_hit = IN.hit[KEY_W] || IN.hit[KEY_UP] || (IN.hit[KEY_E] && !(IN.pad && IN.pad_hit[PAD_A]));
    in->cast = IN.down[KEY_MOUSE1] || IN.down[KEY_CTRL]; in->cast_hit = IN.hit[KEY_MOUSE1] || IN.hit[KEY_CTRL];
    spell_keys();
    if (IN.pad) {                                                  /* a controller (the owner, 26 November): read as it is, not by the crypt's layout */
        static int up_was, lb_was, rb_was;
        const uint8_t *d = IN.pad_down, *h = IN.pad_hit;
        int up = IN.ly > 0.6f || d[PAD_UP];
        in->left |= IN.lx < -0.3f || d[PAD_LEFT];                  /* the left stick or the pad to walk; pushed hard, or B held, to run */
        in->right |= IN.lx > 0.3f || d[PAD_RIGHT];
        in->run |= d[PAD_B] || fabsf(IN.lx) > 0.92f;
        in->jump |= d[PAD_A]; in->jump_hit |= h[PAD_A];             /* A to jump */
        in->down |= IN.ly < -0.6f || d[PAD_DOWN];                   /* down to crouch, and with A to drop through a ledge */
        in->up_hit |= (up && !up_was) || h[PAD_Y];                  /* up, or Y, through a doorway or at a lever */
        in->cast |= d[PAD_RT] || d[PAD_X]; in->cast_hit |= h[PAD_RT] || h[PAD_X];   /* the right trigger, or X, to cast */
        int lb = d[PAD_LB], rb = d[PAD_RB];                          /* the bumpers turn through the spells held */
        if ((lb && !lb_was) || (rb && !rb_was)) {
            static const int ORDER[7] = { AT_FIRE, AT_FROST, AT_CHAIN, AT_FLASK, AT_DRAIN, AT_SPEAR, AT_BFS };
            int at = 0;
            for (int k = 0; k < 7; k++) if (ORDER[k] == G.spell) at = k;
            for (int n = 0; n < 7; n++) { at = (at + (rb ? 1 : 6)) % 7; if (RUN.gear.atk[ORDER[at]] >= 0) { pick_spell(ORDER[at]); break; } }
        }
        up_was = up; lb_was = lb; rb_was = rb;
    }
    if (IN.touch && TCH.used) {                                    /* touch (step 12): the left stick walks, a finger on the right of the
                                                                      picture jumps, held for higher; the right strip casts, a tap at the
                                                                      left uses (E), as in the crypt */
        static int up_was, jump_was;
        int up = IN.ly > 0.6f, jump = 0;
        in->left |= IN.lx < -0.3f; in->right |= IN.lx > 0.3f;
        in->run |= fabsf(IN.lx) > 0.92f;
        in->down |= IN.ly < -0.6f;
        in->up_hit |= up && !up_was;
        for (int k = 0; k < TCH.nf; k++) jump |= TCH.f[k].role == TF_TURN && !TCH.f[k].spent;
        in->jump |= jump; in->jump_hit |= jump && !jump_was;
        up_was = up; jump_was = jump;
    }
}
static void sd_update(float dt) {
    if (!SD.on) return;
    if (G.player && IN.hit[KEY_ESC]) { open_ui(UI_QUIT); return; }
    if (G.player) sd_person(); else sd_dream(dt);
#ifdef CRAWL_DEBUG
    static float trace_t;                                          /* CRAWL_SIDETRACE: where the caster is, twice a second */
    if (getenv("CRAWL_SIDETRACE") && (trace_t += dt) >= 0.5f) {
        trace_t = 0.0f;
        fprintf(stderr, "SIDE at %.1f,%.1f tile %d,%d v %.0f,%.0f ground %d plat %d crouch %d in L%d R%d J%d D%d C%d hp %d t=%.2f\n", SD.x, SD.y, sd_floor_px(SD.x), sd_floor_px(SD.y - 1.0f),
                SD.vx, SD.vy, SD.ground, SD.on_plat, SD.crouch, SD.in.left, SD.in.right, SD.in.jump, SD.in.down, SD.in.cast, G.hp, SD.t);
    }
#endif
    SD.fade = fmaxf(0.0f, SD.fade - dt / 0.4f);
    G.fade = 1.0f - SD.fade * 0.8f;
    G.bar = 1.0f; G.bar_t = BAR_T;                                  /* the status bar stays up in here */
    G.msg_t = fmaxf(0.0f, G.msg_t - dt);
    update_toasts(dt);
    SdIn *in = &SD.in;
    /* the cast: the flask is held to aim and let go to throw; the rest as pressed */
    int sp = G.spell >= 0 && G.spell < NATK ? G.spell : AT_FIRE;
    if (ATK_ELEM[sp] == EL_POISON && sp != AT_BFS) {
        if (in->cast && !SD.dying) { if (!SD.holding) { SD.holding = 1; SD.held = 0.0f; } SD.held += dt; sd_flask_mark(); }
        else if (SD.holding) { SD.holding = 0; sd_cast(1); }
    } else {
        SD.holding = 0;
        if (in->cast && (in->cast_hit || SD.cd <= 0.0f)) sd_cast(0);
    }
    /* the world, in fixed steps */
    for (float left = fminf(dt, 0.1f); left > 1e-6f; left -= SD_STEP) {
        float st = fminf(SD_STEP, left);
        SD.t += st;
        sd_plats(st);
        sd_falls(st);
        sd_waters(st);                                             /* gates, flows, currents (side_works.c) */
        sd_caster(st);
        sd_items(st);
        sd_trigs(st);
        sd_place_harm(st);
        sd_traps(st);
        sd_mobs(st);
        sd_shots(st);
        if (!SD.on) return;                                        /* (the last life went in a step) */
        in->jump_hit = 0; in->cast_hit = 0;
    }
    SD.cd = fmaxf(0.0f, SD.cd - dt); SD.cast_t = fmaxf(0.0f, SD.cast_t - dt); SD.inv_t = fmaxf(0.0f, SD.inv_t - dt);
    SD.hurt_t = fmaxf(0.0f, SD.hurt_t - dt); SD.beam_t = fmaxf(0.0f, SD.beam_t - dt); SD.in_t += dt;
    if (SD.dying) {
        SD.die_t += dt;
        if (SD.die_t > 2.0f) { sd_life_lost(); return; }
    } else {
        for (int i = 0; i < SD.L->nth; i++) {                       /* what is found: walked over */
            const SdThing *t = &SD.L->th[i];
            if (SD.taken[i] || (t->kind != SDT_GOLD && t->kind != SDT_KEY && t->kind != SDT_LIFE)) continue;
            if (sd_at_thing(i, 10.0f, 10.0f)) sd_take(i);
        }
        if (in->up_hit) {                                          /* up: a lever, or a doorway */
            int done = 0;
            for (int i = 0; i < SD.L->nth && !done; i++)
                if (SD.L->th[i].kind == SDT_SWITCH && sd_at_thing(i, 12.0f, 10.0f)) { sd_throw(i); done = 1; }
            int d = done ? -1 : sd_door_here();
            if (d >= 0 && SD.in_t > 0.3f) { sd_leave(SD.L->th[d].a, (SD.L->th[d].flags & SDF_GOAL) != 0); return; }
        }
    }
    /* the camera: a little ahead of where you face, kept in the map */
    float tx = SD.x - 160.0f + (float)SD.face * SDT[TU_CAM_LEAD], ty = SD.y - 96.0f;
    SD.camx += (tx - SD.camx) * fminf(1.0f, dt * 5.0f);
    SD.camy += (ty - SD.camy) * fminf(1.0f, dt * 4.0f);
    SD.camx = clampf(SD.camx, 0.0f, fmaxf(0.0f, (float)(SD.L->w * SD_TILE - W)));
    SD.camy = clampf(SD.camy, 0.0f, fmaxf(0.0f, (float)(SD.L->h * SD_TILE - SD_VIEW_H)));
    /* the figure's frame */
    SD.anim += dt * (fabsf(SD.vx) / 14.0f);
    if (SD.dying) SD.frame = SF_DIE0 + imin(4, (int)(SD.die_t / 0.18f));
    else if (SD.hurt_t > 0.0f) SD.frame = SF_HURT;
    else if (SD.cast_t > 0.0f || SD.holding) SD.frame = SD.cast_t > 0.15f ? SF_CAST1 : SF_CAST0;
    else if (!SD.ground) SD.frame = SD.vy < -60.0f ? SF_RISE : SD.vy < 60.0f ? SF_APEX : SF_FALL;
    else if (SD.crouch) SD.frame = fabsf(SD.vx) > 4.0f ? SF_CREEP0 + ((int)SD.anim & 1) : SF_CROUCH;
    else if (fabsf(SD.vx) > SD_WALK + 8.0f) SD.frame = SF_RUN0 + (int)SD.anim % 6;
    else if (fabsf(SD.vx) > 4.0f) SD.frame = SF_WALK0 + (int)SD.anim % 6;
    else SD.frame = SF_IDLE0 + ((int)(SD.t * 1.5f) & 1);
}

/* ---- the hooks: the crypt's doorway walked into; the frame handed over ---- */
static int sd_walk_in(int x, int y) {                              /* human_bump, human_use */
    int k = sd_crypt_door(x, y);
    if (k < 0 || G.shop || PK.depth) return 0;
    return sd_enter(SDX.d[k].side, SDX.d[k].link, x, y);
}
static int sd_frame(float dt) {                                    /* crawl_update: 1 while a side passage has the frame */
#ifdef CRAWL_DEBUG
    static int went = -1;                                          /* CRAWL_SIDE=n: straight in as the level opens, once a level */
    const char *s = getenv("CRAWL_SIDE");
    if (s && !SD.on && went != sd_stamp && !G.shop && !G.dead && !G.tally && G.level_t > 0.5) {
        went = sd_stamp;
        sd_enter(atoi(s), 0, G.cx + DX[G.h], G.cy + DY[G.h]);
    }
#endif
    static int went_now = -1;                                      /* the World Editor's PLAY of a passage: straight in, once a level */
    if (g_side_now >= 0 && !SD.on && went_now != sd_stamp && !G.shop && !G.dead && !G.tally && G.level_t > 0.5) {
        went_now = sd_stamp;
        sd_enter(g_side_now, 0, G.cx + DX[G.h], G.cy + DY[G.h]);
    }
    if (!SD.on) return 0;
    G.ui_t += dt;
    sd_update(dt);
    return 1;
}

/* ---- the World Editor's PLAY (docs/PLAN-side.md, 2c): passages handed over as text, before the game starts ---- */
int crawl_side_text(int n, const char *text, char *err, int nerr);   /* 0: read into slot n, in place of what is built in; else why not */
int crawl_side_text(int n, const char *text, char *err, int nerr) {
    if (n < 0 || n >= SD_SIDES) { snprintf(err, (size_t)nerr, "a side passage's number is 0 to %d", SD_SIDES - 1); return 1; }
    if (!sd_read(text, &SDL[n], err, nerr)) { sdl_read[n] = 0; return 1; }
    sdl_read[n] = 1;
    return 0;
}
void crawl_side_now(int n, int level);                             /* a new run begins on level, and goes straight into passage n */
void crawl_side_now(int n, int level) { g_side_now = n >= 0 && n < SD_SIDES ? n : -1; g_side_level = level < 1 ? 1 : level; }
void crawl_side_tune(int k, float v);                              /* the World Editor's sliders while it plays: one number, now */
void crawl_side_tune(int k, float v) {
    if (k < 0 || k >= NTU) return;
    SDT[k] = clampf(v, TUNE[k].lo, TUNE[k].hi);
    for (int i = 0; i < SD_SIDES; i++) { SDL[i].tune[k] = SDT[k]; SDL[i].tuned |= 1ull << k; }   /* and kept through a life lost */
}
