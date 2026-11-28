/* A part of scene_crawl.c (docs/PLAN-tidy.md): it is #included there in its place and never compiled on its own.
 * The dead in a side passage (docs/PLAN-side.md, 2h): their pictures as masks, so that what strikes them, and what they strike,
 * strikes where it is drawn and not a box about it; each kind's way in the side view, from the crypt's own attacks (ATTACKS, with
 * their warnings, waits and strengths); what they throw, and how it flies. */

static int sd_verm_shot(const SdShot *s);                          /* side_vermin.c */
/* ---- the pictures as masks: the drawing's own rule (draw/side.c, sd_draw_spr: a sprite at half its size, a pixel shown where two
 * of its four are), so a pixel here is a pixel there ---- */
typedef struct { uint32_t row[32]; int8_t x0, x1, y0, y1; uint8_t made; } SdMask;
static SdMask SDMK[NSPR];
static const SdMask *sd_mask(int spr) {
    SdMask *k = &SDMK[clampi(spr, 0, NSPR - 1)];
    if (k->made) return k;
    k->made = 1; k->x0 = 31; k->y0 = 31; k->x1 = 0; k->y1 = 0;
    int any = 0;
    for (int y = 0; y < TS / 2; y++)
        for (int x = 0; x < TS / 2; x++) {
            int n = 0;
            for (int q = 0; q < 4; q++) n += crawl_spr[spr][(x * 2 + (q & 1)) * TS + y * 2 + (q >> 1)] != C_CLEAR;
            if (n < 2) continue;
            k->row[y] |= 1u << x; any = 1;
            if (x < k->x0) k->x0 = (int8_t)x;
            if (x > k->x1) k->x1 = (int8_t)x;
            if (y < k->y0) k->y0 = (int8_t)y;
            if (y > k->y1) k->y1 = (int8_t)y;
        }
    if (!any) { k->x0 = k->x1 = 16; k->y0 = k->y1 = 31; }
    return k;
}
static int sd_way_of(int k);
static int sd_mob_spr(const SdMob *m) {                            /* the picture shown now (draw/side.c draws this one) */
    switch (m->kind) {                                             /* small things that move (docs/PLAN-vermin.md): by what they are doing */
    case MOB_HOPPER: return m->ground ? SP_HOP_A : SP_HOP_B;
    case MOB_LEECH: return m->ground || m->vy == 0.0f ? SP_LEECH_A : SP_LEECH_B;
    case MOB_CRAB: return m->act == 22 ? SP_CRAB_COIN : ((int)(m->t * 10.0f) & 1) ? SP_CRAB_B : SP_CRAB_A;   /* (22: SA_COIN) */
    default: break;
    }
    int spr = ((int)(m->t * 5.0f) & 1) ? KINDS[m->kind].side2 : KINDS[m->kind].side;
    return spr > 0 ? spr : KINDS[m->kind].spr;
}
static int sd_mob_ox(const SdMob *m) { return (int)m->x - TS / 4; }   /* the picture's top left, in the passage's pixels */
static int sd_mob_oy(const SdMob *m) { return (int)m->y - TS / 2; }
static int sd_mask_at(const SdMask *k, int flip, int c, int r) {
    if (c < 0 || c >= TS / 2 || r < 0 || r >= TS / 2) return 0;
    return k->row[r] >> (flip ? TS / 2 - 1 - c : c) & 1;
}
static int sd_mob_px(const SdMob *m, int wx, int wy) {
    if (m->kind == MOB_LEECH && m->act == 21) return 0;            /* (21: SA_HIDE) under the poison: nothing to strike or be struck by */
    return sd_mask_at(sd_mask(sd_mob_spr(m)), m->face < 0, wx - sd_mob_ox(m), wy - sd_mob_oy(m)); }
static void sd_mob_box(const SdMob *m, int *l, int *t, int *r, int *b) {   /* the picture's pixels' bounds now, inclusive */
    const SdMask *k = sd_mask(sd_mob_spr(m));
    int ox = sd_mob_ox(m), oy = sd_mob_oy(m);
    if (m->face < 0) { *l = ox + TS / 2 - 1 - k->x1; *r = ox + TS / 2 - 1 - k->x0; } else { *l = ox + k->x0; *r = ox + k->x1; }
    *t = oy + k->y0; *b = oy + k->y1;
}
static void sd_mob_span(const SdMob *m, int *top, int *bot) {      /* both its pictures' rows, above its feet: for sight and aim, steady */
    const SdMask *a = sd_mask(KINDS[m->kind].side > 0 ? KINDS[m->kind].side : KINDS[m->kind].spr);
    const SdMask *b = sd_mask(KINDS[m->kind].side2 > 0 ? KINDS[m->kind].side2 : KINDS[m->kind].spr);
    *top = imin(a->y0, b->y0) - TS / 2; *bot = imax(a->y1, b->y1) - TS / 2;
}
static float sd_mob_h(const SdMob *m) { int t, b; sd_mob_span(m, &t, &b); return (float)(b - t + 1); }
static float sd_mob_cy(const SdMob *m) { int t, b; sd_mob_span(m, &t, &b); return (float)((int)m->y) + (float)(t + b + 1) * 0.5f; }
static float sd_mob_top(const SdMob *m) { int t, b; sd_mob_span(m, &t, &b); return (float)((int)m->y + t); }
static int sd_mob_line(const SdMob *m, float x0, float y0, float x1, float y1, int band, float *hx, float *hy) {   /* the first pixel of
                                                                      the picture a line crosses, within band pixels of it */
    int l, t, r, b;
    sd_mob_box(m, &l, &t, &r, &b);
    if (fmaxf(x0, x1) < (float)l || fminf(x0, x1) > (float)(r + 1) || fmaxf(y0, y1) + (float)band < (float)t || fminf(y0, y1) - (float)band > (float)(b + 1)) return 0;
    int n = imax(1, (int)ceilf(fmaxf(fabsf(x1 - x0), fabsf(y1 - y0))));
    for (int i = 0; i <= n; i++) {
        float k = (float)i / (float)n, x = x0 + (x1 - x0) * k, y = y0 + (y1 - y0) * k;
        for (int j = 0; j <= band * 2; j++) {
            int dy = (j & 1) ? -(j + 1) / 2 : j / 2;               /* the line itself first, then a pixel either side, and on */
            if (sd_mob_px(m, (int)floorf(x), (int)floorf(y) + dy)) { *hx = x; *hy = y + (float)dy; return 1; }
        }
    }
    return 0;
}
static float sd_mob_near(const SdMob *m, float wx, float wy) {     /* how far the nearest of its pixels is */
    int l, t, r, b;
    float best = 1e9f;
    sd_mob_box(m, &l, &t, &r, &b);
    for (int y = t; y <= b; y++)
        for (int x = l; x <= r; x++)
            if (sd_mob_px(m, x, y)) best = fminf(best, hypotf((float)x + 0.5f - wx, (float)y + 0.5f - wy));
    return best;
}
/* the caster's figure as it was last drawn (SD.frame) */
static int sd_fig_ox(void) { return (int)floorf(SD.x) - SF_W / 2; }
static int sd_fig_oy(void) { return (int)floorf(SD.y) - SF_H; }
static int sd_fig_px(int wx, int wy) {
    int c = wx - sd_fig_ox(), r = wy - sd_fig_oy();
    if (c < 0 || c >= SF_W || r < 0 || r >= SF_H) return 0;
    return side_fig[clampi(SD.frame, 0, NSF - 1)][r * SF_W + (SD.face < 0 ? SF_W - 1 - c : c)] != C_CLEAR;
}
static int sd_mob_touches(const SdMob *m, int ahead) {             /* its picture, moved ahead pixels the way it faces, over the caster's */
    if (SD.dying) return 0;
    int l, t, r, b, sh = ahead * (m->face < 0 ? -1 : 1);
    sd_mob_box(m, &l, &t, &r, &b);
    int x0 = imax(l + sh, sd_fig_ox()), x1 = imin(r + sh, sd_fig_ox() + SF_W - 1), y0 = imax(t, sd_fig_oy()), y1 = imin(b, sd_fig_oy() + SF_H - 1);
    for (int y = y0; y <= y1; y++)
        for (int x = x0; x <= x1; x++)
            if (sd_fig_px(x, y) && sd_mob_px(m, x - sh, y)) return 1;
    return 0;
}
/* a shot's pixels, the same for its drawing and what it strikes: the dead's bone a short stick turning end over end; the rest round */
#define SD_SHOT_PX 32
static float sd_shot_r(const SdShot *s) {
    switch (s->kind) {
    case SS_FLASK: return 2.2f;
    case SS_EMBER: return 1.6f;
    case SS_FIREBALL: return 3.0f;
    case SS_BONE: return 0.0f;
    default: return 2.4f;                                          /* the caster's bolts: their head (the tail behind is its glow) */
    }
}
static int sd_shot_shape(const SdShot *s, int *dx, int *dy) {
    int n = 0;
    if (s->kind == SS_BONE) {
        float a = s->life * 11.0f, c = cosf(a), sn = sinf(a);
        for (int k = -2; k <= 2; k++) { dx[n] = (int)lroundf(c * (float)k); dy[n] = (int)lroundf(sn * (float)k * 0.8f); n++; }
        return n;
    }
    float r = sd_shot_r(s);
    int ri = (int)ceilf(r);
    for (int y = -ri; y <= ri; y++) for (int x = -ri; x <= ri; x++) if ((float)(x * x + y * y) <= r * r && n < SD_SHOT_PX) { dx[n] = x; dy[n] = y; n++; }
    return n;
}

/* ---- each kind's way ---- */
enum { SW_WALK, SW_THROW, SW_LEAP, SW_BAT, SW_DRIFT, SW_FLY, SW_IMP, SW_SKULL, SW_DROP, SW_SAC, SW_BROOD, SW_DEACON, SW_SEXTON, SW_WHIP, SW_GARG,
       SW_HOP, SW_LEECH, SW_CRAB };                                /* small things that move (docs/PLAN-vermin.md) */
enum { SA_NONE, SA_WIND, SA_STRIKE, SA_THROW, SA_LEAP, SA_LAND, SA_CHARGE, SA_CIRCLE, SA_DIVE, SA_CLIMB, SA_HANG, SA_DROP, SA_RISE, SA_RING,
       SA_CHANT, SA_HORN, SA_LASH, SA_STONE, SA_SHAKE, SA_RECOIL, SA_FLEE,
       SA_HIDE, SA_COIN, SA_RUN, SA_CORNER };                      /* the leech under the poison; the crab still, running, cornered */
_Static_assert(SA_HIDE == 21 && SA_COIN == 22, "sd_mob_spr and sd_mob_px name them by number, above");
#define SD_KIDS 3                                                  /* a brood mother's young at once */
#define SD_DEACON_RAISES 4
static int sd_way_of(int k) {
    switch (k) {
    case MOB_SKEL: return SW_THROW;
    case MOB_HUNTER: case MOB_GHOUL: return SW_LEAP;
    case MOB_BAT: return SW_BAT;
    case MOB_GHOST: return SW_DRIFT;
    case MOB_SHADE: case MOB_WRAITH: return SW_FLY;
    case MOB_IMP: return SW_IMP;
    case MOB_WSKULL: return SW_SKULL;
    case MOB_SPIDER: return SW_DROP;
    case MOB_SAC: return SW_SAC;
    case MOB_BROOD: return SW_BROOD;
    case MOB_DEACON: return SW_DEACON;
    case MOB_SEXTON: return SW_SEXTON;
    case MOB_WHIPPER: return SW_WHIP;
    case MOB_GARGOYLE: return SW_GARG;
    case MOB_HOPPER: return SW_HOP;
    case MOB_LEECH: return SW_LEECH;
    case MOB_CRAB: return SW_CRAB;
    default: return SW_WALK;                                       /* the shambler, guard, drowned, hound, knight, ling, lightless, rat */
    }
}
static int sd_agile(int k) { return k == MOB_HOUND || k == MOB_GHOUL || k == MOB_HUNTER || k == MOB_RAT || k == MOB_LING || k == MOB_LIGHTLESS; }
static void sd_mob_way(SdMob *m) {                                 /* sd_mob_spawn: its way, its attack, and where a spider hangs */
    m->way = sd_way_of(m->kind);
    m->fa = fa_main(m->kind);
    m->act = SA_NONE; m->ground = 1;
    if (m->way == SW_DROP) {                                       /* up to the roof over it, on a thread */
        int tx = sd_floor_px(m->x), ty = sd_floor_px(m->y - 1.0f), up = 0;
        while (up < 10 && !sd_solid(tx, ty - up - 1)) up++;
        m->aim_y = (float)((ty - up) * SD_TILE);                   /* where the thread is tied */
        int t, b; sd_mob_span(m, &t, &b);
        m->y = m->home_y = m->aim_y - (float)t + 1.0f;
        m->act = SA_HANG; m->ground = 0;
    }
    if (m->way == SW_SKULL) m->ground = 0;
    if (m->way == SW_GARG) m->act = SA_STONE;
    if (m->way == SW_LEECH) {                                      /* under the poison it is set in: its surface the top of that pool */
        int tx = sd_floor_px(m->x), ty = sd_floor_px(m->y - 1.0f);
        while (ty > 0 && sd_tile(tx, ty - 1) == '~') ty--;
        m->home_y = (float)(ty * SD_TILE) + 4.0f; m->y = m->home_y + 10.0f; m->act = SA_HIDE; m->ground = 0; m->cool = 1.0f;
    }
    if (m->way == SW_CRAB) { m->act = SA_COIN; m->sub_t = 0.0f; }   /* still, a coin; sub_t: how far along its way */
    if (m->way == SW_HOP) m->face = 1;
}
/* the coin crab's way: its own place, then the cells painted, each a point at its foot */
static int sd_crab_points(const SdMob *m, float *px, float *py, int max) {
    int n = 0;
    px[n] = m->home_x; py[n] = m->home_y; n++;
    if (m->thing < 0) return n;
    const SdThing *t = &SD.L->th[m->thing];
    for (int i = 0; i < t->h && n < max; i++) { px[n] = (float)(SD.L->cpc[t->w + i][0] * SD_TILE) + 8.0f; py[n] = (float)((SD.L->cpc[t->w + i][1] + 1) * SD_TILE); n++; }
    return n;
}
static float sd_wind_of(const SdMob *m, float least) {             /* the warning before its blow: the crypt's, never less than least */
    float w = m->fa >= 0 ? ATTACKS[m->fa].wind : 0.4f;
    return fmaxf(least, w) * SDT[TU_DEAD_WIND];
}
static float sd_hit_k(const SdMob *m) { return m->fa >= 0 && ATTACKS[m->fa].k > 0.0f ? ATTACKS[m->fa].k : 0.6f; }
static int sd_hit_el(const SdMob *m) {
    if (m->fa >= 0) return ATTACKS[m->fa].el;
    for (int e = 0; e < NELEM; e++) if (KINDS[m->kind].harm >> e & 1) return e;
    return EL_PHYS;
}
static void sd_mob_strikes(SdMob *m, float k) {                    /* its blow landing on the caster; then it draws back a moment */
    G.blame = (Blame){ BL_MOB, m->kind, m->rank };
    sd_foe_set((int)(m - SD.mob), 3.0f);                           /* the panel shows what struck (2j) */
    if (m->kind == MOB_BAT || m->kind == MOB_LING || m->kind == MOB_RAT) sd_knock = 0.3f;   /* a small biter: a nudge, not a throw */
    sd_hurt(sd_blow(m) * k, sd_hit_el(m), m->x);
    if (!m->fly && m->way != SW_DROP && m->way != SW_SKULL && m->way != SW_SAC && m->way != SW_HOP && m->way != SW_LEECH && m->way != SW_CRAB) {
        m->act = SA_LAND; m->act_t = 0.0f; m->vx = 0.0f;           /* (in the air: it drops) */
    }
}
static void sd_wake(SdMob *m, float secs) {                        /* told where the caster is (a bell, a horn, a deacon's call) */
    m->alert = fmaxf(m->alert, secs); m->mem = fmaxf(m->mem, secs); m->lx = SD.x; m->ly = SD.y;
    if (m->act == SA_STONE) { m->act = SA_SHAKE; m->act_t = 0.0f; }
}
static void sd_call(const SdMob *from, float tiles, float secs, int only) {   /* everything near that hears it; only: one kind, -1 any */
    for (int i = 0; i < SD_MOBS; i++) {
        SdMob *o = &SD.mob[i];
        if (o == from || !o->alive || o->state == SDM_DIE || (only >= 0 && o->kind != only)) continue;
        if (hypotf(o->x - from->x, o->y - from->y) < tiles * SD_TILE) sd_wake(o, secs);
    }
}

static void sd_dead_says(const SdMob *m, const char *what) {      /* EVENT side_dead: a creature's act worth a line */
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT side_dead %s %s t=%.2f\n", KINDS[m->kind].name, what, SD.t);
#else
    (void)m; (void)what;
#endif
}
/* the fallen, for a deacon: the last few, where they fell */
#define SD_FALLEN 8
static struct { float x, y; int kind, rank, up; } SDF[SD_FALLEN];
static int sdf_n;
static void sd_dead_build(void) { sdf_n = 0; }
static void sd_fallen(const SdMob *m) {
    if (KINDS[m->kind].family != PF_BONE) return;                  /* bones get up again; nothing else does */
    int i = sdf_n < SD_FALLEN ? sdf_n++ : (int)(SD.t * 7.0f) % SD_FALLEN;
    SDF[i].x = m->x; SDF[i].y = m->y; SDF[i].kind = m->kind; SDF[i].rank = m->rank; SDF[i].up = 0;
}

/* ---- moving: walkers on the floor, fliers in the air ---- */
#define SD_MOB_HW 5.0f                                             /* the walkers' feet's half width against the stone, as the caster's */
static float sd_mob_bh(const SdMob *m) { return clampf(sd_mob_h(m) - 2.0f, 8.0f, 28.0f); }   /* how tall it is against the stone */
static int sd_floor_at(float x, float y, int ledges) {             /* stone, or a ledge, at the row under y */
    int c = sd_floor_px(x), r = sd_floor_px(y + 0.5f);
    return sd_solid(c, r) || (ledges && sd_tile(c, r) == '=');
}
static int sd_bad_at(const SdMob *m, float x, float y) {           /* what a walker won't step into: spikes, poison (the drowned wade it) */
    int c = sd_floor_px(x), r = sd_floor_px(y - 1.0f), below = sd_floor_px(y + 0.5f);
    if (sd_tile(c, below) == '^' || sd_tile(c, r) == '^') return 1;
    if (m->kind == MOB_DROWNED) return 0;
    return sd_tile(c, r) == '~' || sd_tile(c, below) == '~';
}
static int sd_mob_blocked(const SdMob *m, float x) {               /* the stone in the way at x, from its feet to its head */
    float bh = sd_mob_bh(m);
    int c = sd_floor_px(x);
    for (int r = sd_floor_px(m->y - bh); r <= sd_floor_px(m->y - 1.0f); r++) if (sd_solid(c, r)) return 1;
    return 0;
}
static int sd_mob_step_up(const SdMob *m, float x) {               /* a step a tile high, with room over it */
    float bh = sd_mob_bh(m);
    int c = sd_floor_px(x), foot = sd_floor_px(m->y - 1.0f);
    if (!sd_solid(c, foot) || sd_solid(c, foot - 1)) return 0;
    for (int r = sd_floor_px(m->y - SD_TILE - bh); r < foot; r++) if (sd_solid(c, r) || sd_solid(sd_floor_px(m->x), r)) return 0;
    return 1;
}
static void sd_mob_jump(SdMob *m, float h, float vx) { m->vy = -sqrtf(2.0f * SD_G * h); m->vx = vx; m->ground = 0; }
/* the walker's body: along the floor at vx, falling when there is nothing under it, landing on stone, a ledge or a platform */
static int sd_mob_body(SdMob *m, float dt) {                       /* 1: stopped by the stone */
    int stopped = 0;
    if (m->ground >= 2) {                                          /* stood on a platform: it carries it */
        int pi = m->ground - 2;
        const SdPlat *p = &SD.plat[pi];
        const SdThing *t = &SD.L->th[pi];
        m->x += p->x - p->px; m->y = p->y;
        if (m->x + SD_MOB_HW <= p->x || m->x - SD_MOB_HW >= p->x + (float)(t->w * SD_TILE)) m->ground = 0;
    }
    if (m->vx != 0.0f) {
        float nx = m->x + m->vx * dt, lead = nx + (m->vx > 0.0f ? SD_MOB_HW : -SD_MOB_HW);
        if (sd_mob_blocked(m, lead)) { m->vx = 0.0f; stopped = 1; } else m->x = nx;
    }
    if (m->ground == 1 && !sd_floor_at(m->x - SD_MOB_HW + 1.0f, m->y, 1) && !sd_floor_at(m->x + SD_MOB_HW - 1.0f, m->y, 1)) m->ground = 0;
    if (!m->ground) {
        float prev = m->y;
        m->vy = fminf(SD_FALL_MAX, m->vy + SD_G * dt);
        m->y += m->vy * dt;
        if (m->vy < 0.0f) {                                        /* rising: the roof */
            int r = sd_floor_px(m->y - sd_mob_bh(m));
            if (sd_solid(sd_floor_px(m->x), r)) { m->y = (float)((r + 1) * SD_TILE) + sd_mob_bh(m); m->vy = 0.0f; }
        } else {
            int r = sd_floor_px(m->y - 0.001f);
            float top = (float)(r * SD_TILE);
            for (int s = -1; s <= 1; s += 2) {
                int c = sd_floor_px(m->x + (float)s * (SD_MOB_HW - 1.0f)), ch = sd_tile(c, r);
                if (sd_solid(c, r) || (ch == '=' && prev <= top + 0.5f)) { m->y = top; m->vy = 0.0f; m->ground = 1; break; }
            }
            if (!m->ground) {
                const SdLevel *L = SD.L;
                for (int i = 0; i < L->nth; i++) {
                    if (L->th[i].kind != SDT_PLAT) continue;
                    const SdPlat *p = &SD.plat[i];
                    if (m->x + SD_MOB_HW <= p->x || m->x - SD_MOB_HW >= p->x + (float)(L->th[i].w * SD_TILE)) continue;
                    if (prev <= p->py + 0.5f && m->y >= p->y) { m->y = p->y; m->vy = 0.0f; m->ground = 2 + i; break; }
                }
            }
        }
        if (m->y > (float)(SD.L->h * SD_TILE + 64)) m->alive = 0;  /* down the drop */
    }
    return stopped;
}
static void sd_mob_fly(SdMob *m, float tx, float ty, float sp, float dt, int through) {   /* a flier to (tx, ty), its picture's middle */
    float cy = sd_mob_cy(m), ax = tx - m->x, ay = ty - cy, d = fmaxf(1.0f, hypotf(ax, ay));
    m->vx += (ax / d * sp - m->vx) * fminf(1.0f, dt * 2.5f);
    m->vy += (ay / d * sp - m->vy) * fminf(1.0f, dt * 2.5f);
    float nx = m->x + m->vx * dt, ny = m->y + m->vy * dt;
    if (through) { m->x = nx; m->y = ny; return; }
    float half = sd_mob_h(m) * 0.5f;
    if (!sd_solid(sd_floor_px(nx + (m->vx > 0.0f ? 6.0f : -6.0f)), sd_floor_px(cy))) m->x = nx; else m->vx = -m->vx * 0.5f;
    float edge = ny - m->y + (m->vy > 0.0f ? cy + half - 2.0f : cy - half + 2.0f);
    if (!sd_solid(sd_floor_px(m->x), sd_floor_px(edge))) m->y = ny; else m->vy = -m->vy * 0.5f;
}

/* ---- what they throw ---- */
static float sd_headroom(float x, float y) {                       /* how far up from (x, y) before the stone, 96 at the most */
    for (float h = 0.0f; h < 96.0f; h += 4.0f) if (sd_solid(sd_floor_px(x), sd_floor_px(y - h))) return h;
    return 96.0f;
}
static void sd_throw_bone(SdMob *m) {                              /* a skeleton's bone: a high, slow lob to where you stood, not ahead */
    float hx = m->x + (float)m->face * 3.0f, hy = sd_mob_top(m) + 2.0f;
    float spread = SDT[TU_BONE_AIM] * fmaxf(0.25f, 1.15f - 0.2f * (float)m->rank);   /* the higher ranks throw truer */
    float tx = m->aim_x + (sd_rndf() * 2.0f - 1.0f) * spread, ty = m->aim_y - 6.0f;
    float dx = tx - hx, dy = ty - hy, g = SDT[TU_BONE_GRAV];
    float T = SDT[TU_BONE_TIME] * (0.8f + 0.2f * clampf(fabsf(dx) / 112.0f, 0.0f, 2.0f));   /* never quick, even near: a high arc */
    float vy = (dy - 0.5f * g * T * T) / T;
    float room = sd_headroom(hx, hy) - 6.0f;                       /* under a low roof, flatter and quicker */
    if (vy < 0.0f && vy * vy / (2.0f * g) > room) {
        vy = -sqrtf(2.0f * g * fmaxf(4.0f, room));
        float disc = vy * vy + 2.0f * g * dy;
        T = disc > 0.0f ? (-vy + sqrtf(disc)) / g : -vy / g;
        T = fmaxf(0.3f, T);
    }
    SdShot *s = sd_shot_new(SS_BONE, hx, hy, dx / T, vy, sd_blow(m) * sd_hit_k(m), T + 2.0f, g, 1, -1, 0);
    if (s) { s->from = m->kind; s->el = EL_PHYS; }
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT side_bone from %.0f,%.0f to %.0f,%.0f (%.0f off) in %.2f s, rising %.0f px t=%.2f\n", hx, hy, tx, ty, tx - m->aim_x, T,
            vy < 0.0f ? vy * vy / (2.0f * g) : 0.0f, SD.t);
#endif
    sd_sfx_at(SFX_THROW, 0.4f, m->x, m->y - 8.0f);
}
static void sd_throw_at(SdMob *m, int kind, float speed, float g, float k) {   /* an ember or a fireball, at the caster's middle now */
    float hx = m->x + (float)m->face * 5.0f, hy = sd_mob_cy(m) - 2.0f;
    float tx = SD.x, ty = SD.y - SD_HT * 0.5f, d = fmaxf(1.0f, hypotf(tx - hx, ty - hy)), T = d / speed;
    SdShot *s = sd_shot_new(kind, hx, hy, (tx - hx) / T, (ty - hy - 0.5f * g * T * T) / T, sd_blow(m) * k, 5.0f, g, 1, -1, 0);
    if (s) { s->from = m->kind; s->el = EL_FIRE; }
    sd_sfx_at(kind == SS_FIREBALL ? SFX_IGNITE : SFX_WHOOSH, 0.5f, m->x, m->y - 8.0f);
}

/* ---- the frame: each creature ---- */
static void sd_mob_walker(SdMob *m, int i, float dt, float sp, int knows, int sees, float dx, float dy) {
    int want = 0;
    float keep_lo = 0.0f, keep_hi = 0.0f;                          /* a band it keeps from you: those that fight from further off */
    if (m->way == SW_DEACON || m->way == SW_SEXTON) { keep_lo = 56.0f; keep_hi = 110.0f; }
    if (m->way == SW_THROW) { keep_lo = 40.0f; keep_hi = 110.0f; }   /* a thrower keeps off, and doesn't come to blows */
    if (knows) {
        float gx = m->lx - m->x;
        want = gx > 4.0f ? 1 : gx < -4.0f ? -1 : 0;
        if (keep_hi > 0.0f && sees) {
            float ad = fabsf(dx);
            want = ad < keep_lo ? (dx > 0.0f ? -1 : 1) : ad > keep_hi ? (dx > 0.0f ? 1 : -1) : 0;
            if (ad < keep_lo && sd_mob_blocked(m, m->x + (float)want * (SD_MOB_HW + 4.0f))) want = 0;   /* backed to the wall: stands */
        }
        if (want) m->face = want; else if (sees) m->face = dx > 0.0f ? 1 : -1;
    } else {                                                       /* its round: along the floor to its edge, and back */
        want = m->face;
        if (fabsf(m->x - m->home_x) > 6.0f * SD_TILE) want = m->face = m->x < m->home_x ? 1 : -1;
    }
    float pace = (float)want * sp * (knows ? 1.0f : 0.5f);
    if (m->ground && want) {                                       /* what is ahead: a step to hop, an edge, spikes or poison */
        float lead = m->x + (float)want * (SD_MOB_HW + 3.0f), leap = (float)want * fmaxf(sp * 1.6f, 90.0f);
        if (sd_mob_blocked(m, lead)) {
            if (sd_mob_step_up(m, lead)) { sd_mob_jump(m, SD_TILE + 4.0f, pace); pace = m->vx; }
            else { pace = 0.0f; if (!knows) m->face = -m->face; }
        } else if (sd_bad_at(m, lead, m->y)) { pace = 0.0f; if (!knows) m->face = -m->face; }
        else if (m->ground == 1 && !sd_floor_at(lead, m->y, 1)) {  /* an edge: down it after you, if not far; the quick jump a gap */
            int c = sd_floor_px(lead), r = sd_floor_px(m->y + 0.5f), drop = 0;
            while (drop < 5 && !sd_solid(c, r + drop) && sd_tile(c, r + drop) != '=') drop++;
            int pit_bad = drop >= 5 || sd_tile(c, r + drop) == '^' || (m->kind != MOB_DROWNED && sd_tile(c, r + drop) == '~');
            int land = 0;
            for (int k = 2; k <= 3 && !land; k++) land = sd_floor_at(m->x + (float)(want * k * SD_TILE), m->y, 1) && !sd_bad_at(m, m->x + (float)(want * k * SD_TILE), m->y);
            if (knows && m->ly > m->y + 8.0f && !pit_bad) { }    /* over the edge, after you */
            else if (knows && sd_agile(m->kind) && land && fabsf(m->lx - m->x) > 24.0f) { sd_mob_jump(m, 22.0f, leap); pace = m->vx; }
            else { pace = 0.0f; if (!knows) m->face = -m->face; }
        }
        if (m->ground && knows && sd_agile(m->kind) && m->ly < m->y - 20.0f && fabsf(m->lx - m->x) < 40.0f && m->ly > m->y - 60.0f) {
            sd_mob_jump(m, m->y - m->ly + 10.0f, pace); pace = m->vx;   /* you above it, on a ledge: up after you */
        }
    }
    if (!m->ground) pace = m->vx;                                  /* in the air it keeps its way */
    m->vx = pace;
    if (sd_mob_body(m, dt) && !knows) m->face = -m->face;
    (void)i;
}
static void sd_mob_melee(SdMob *m, float dt, int sees) {           /* its blow: a warning, then a lunge that lands only on the pixels it reaches */
    if (m->fa < 0 || m->act != SA_NONE || m->cool > 0.0f || !sees || !m->ground) return;
    const FoeAtk *a = &ATTACKS[m->fa];
    if (a->ak != AK_BLOW && a->ak != AK_BITE && a->ak != AK_TOUCH) return;
    int l, t, r, b;
    sd_mob_box(m, &l, &t, &r, &b);
    float gap = SD.x > m->x ? (SD.x - SD_HW) - (float)r : (float)l - (SD.x + SD_HW);
    float vy_over = fminf((float)b, SD.y) - fmaxf((float)t, SD.y - sd_box_h());
    if (gap > a->go * 12.0f || vy_over < -2.0f) return;
    m->face = SD.x > m->x ? 1 : -1;
    m->act = SA_WIND; m->next = SA_STRIKE; m->act_t = 0.0f; m->wind = sd_wind_of(m, 0.25f);
    (void)dt;
}
static void sd_mob_acts(SdMob *m, float dt, int sees, float dx, float dy) {   /* the blows and leaps once begun, any way */
    const FoeAtk *a = m->fa >= 0 ? &ATTACKS[m->fa] : NULL;
    switch (m->act) {
    case SA_WIND:                                                  /* the warning: it stands (or hangs), and then what it warned of */
        m->vx = 0.0f;
        if (!m->fly && m->way != SW_SKULL) sd_mob_body(m, dt);
        if (m->act_t < m->wind) return;
        m->act_t = 0.0f; m->act = SA_NONE;
        switch (m->next) {
        case SA_THROW:
            if (m->way == SW_THROW) { sd_throw_bone(m); m->cool = fmaxf(1.4f, 2.6f - 0.03f * (float)G.level) + sd_rndf(); }
            else if (m->way == SW_IMP) { sd_dead_says(m, "throws an ember"); sd_throw_at(m, SS_EMBER, SDT[TU_DEAD_SHOT], 60.0f, sd_hit_k(m)); m->cool = (a ? a->cool : 2.4f) + sd_rndf(); }
            else { sd_dead_says(m, "spits fire"); sd_throw_at(m, SS_FIREBALL, SDT[TU_DEAD_SHOT] * 0.8f, 0.0f, sd_hit_k(m)); m->cool = (a ? a->cool : 3.0f) + sd_rndf(); }
            return;
        case SA_LEAP: {                                            /* off the ground at where you stood */
            float gx = m->aim_x - m->x, h = 22.0f + fmaxf(0.0f, m->y - m->aim_y);
            sd_mob_jump(m, fminf(h, 56.0f), 0.0f); sd_dead_says(m, "leaps");
            float T = 2.0f * -m->vy / SD_G;
            m->vx = clampf(gx / fmaxf(0.25f, T), -190.0f, 190.0f);
            m->act = SA_LEAP;
            sd_sfx_at(KINDS[m->kind].sfx_die == SFX_SKITTER ? SFX_SKITTER : SFX_WHOOSH, 0.4f, m->x, m->y - 8.0f);
            return;
        }
        case SA_HORN:                                              /* the horn wound: his hounds set on you */
            sd_sfx_at(SFX_HORN, 1.0f, m->x, m->y - 16.0f);
            sd_call(m, 30.0f, 10.0f, MOB_HOUND); sd_dead_says(m, "winds his horn");
            m->cool = 0.6f;
            return;
        case SA_LASH:
            m->act = SA_LASH; sd_dead_says(m, "lashes");
            sd_sfx_at(SFX_WHIPCRACK, 0.9f, m->x, m->y - 16.0f);
            return;
        case SA_CHARGE: m->act = SA_CHARGE; sd_dead_says(m, "charges"); return;
        default:
            m->act = SA_STRIKE; sd_dead_says(m, "strikes");
            sd_sfx_at(m->kind == MOB_KNIGHT || m->kind == MOB_SHIELD ? SFX_SWORD : SFX_WHOOSH, 0.4f, m->x, m->y - 10.0f);
            return;
        }
    case SA_STRIKE: {                                              /* a short lunge; what it reaches it strikes */
        float reach = a ? fmaxf(2.0f, (a->reach - a->go) * 14.0f) + 3.0f : 4.0f;
        if (!sd_mob_blocked(m, m->x + (float)m->face * (SD_MOB_HW + 2.0f))) m->x += (float)m->face * 70.0f * dt;
        m->vx = 0.0f; if (!m->fly) sd_mob_body(m, dt);
        if (sd_mob_touches(m, (int)reach)) { sd_mob_strikes(m, sd_hit_k(m)); m->cool = a ? a->cool : 1.2f; m->touch_t = 0.8f; return; }
        if (m->act_t > 0.14f) { m->act = SA_NONE; m->cool = (a ? a->cool : 1.2f) * 0.6f; }
        return;
    }
    case SA_LASH: {                                                /* the whip's line, out from his hand: the pixels on it strike */
        float k = fminf(1.0f, m->act_t / 0.08f), hx = m->x + (float)m->face * 6.0f, hy = sd_mob_top(m) + 10.0f;
        m->aim_x = hx + (float)m->face * 34.0f * k; m->aim_y = hy + 6.0f * k;
        int n = (int)(34.0f * k);
        for (int p = 0; p <= n && m->act_t < 0.2f; p++) {
            float x = hx + (m->aim_x - hx) * (float)p / (float)imax(1, n), y = hy + (m->aim_y - hy) * (float)p / (float)imax(1, n);
            if (sd_fig_px((int)floorf(x), (int)floorf(y))) { sd_mob_strikes(m, sd_hit_k(m)); m->act_t = 0.2f; break; }
        }
        if (m->act_t > 0.24f) { m->act = SA_NONE; m->cool = a ? a->cool : 1.8f; }
        return;
    }
    case SA_CHARGE: {                                              /* head down, at three times its pace, till it strikes, stops or tires */
        m->vx = (float)m->face * m->speed * SDT[TU_DEAD_PACE] * 3.2f;
        int stop = sd_mob_body(m, dt) || !sd_floor_at(m->x + (float)m->face * (SD_MOB_HW + 3.0f), m->y, 1) || sd_bad_at(m, m->x + (float)m->face * 8.0f, m->y);
        if (sd_mob_touches(m, 2)) { sd_mob_strikes(m, 1.2f); m->cool = 1.6f; m->touch_t = 0.8f; return; }
        if (stop || m->act_t > 0.8f) { m->act = SA_NONE; m->cool = 1.2f; m->vx = 0.0f; }
        return;
    }
    case SA_LEAP:                                                  /* in the air: whatever of you it meets */
        if (sd_mob_touches(m, 0) && m->touch_t <= 0.0f) { sd_mob_strikes(m, sd_hit_k(m)); m->touch_t = 0.8f; }
        if (sd_mob_body(m, dt)) m->vx = 0.0f;
        if (m->ground) { m->act = SA_LAND; m->act_t = 0.0f; m->vx = 0.0f; }
        return;
    case SA_LAND:                                                  /* come down, or struck home: a step back, a moment to gather itself */
        m->vx = m->act_t < 0.3f && m->ground ? -(float)m->face * 70.0f : 0.0f;   /* some twenty pixels: room to get away */
        if (m->vx != 0.0f && (!sd_floor_at(m->x + m->vx * 0.2f, m->y, 1) || sd_bad_at(m, m->x + m->vx * 0.2f, m->y))) m->vx = 0.0f;
        sd_mob_body(m, dt);
        if (m->act_t > 0.55f) { m->act = SA_NONE; m->cool = fmaxf(m->cool, a ? a->cool * 0.6f : 1.5f); }
        return;
    default: return;
    }
    (void)sees; (void)dx; (void)dy;
}
static void sd_mobs(float dt) {
    for (int i = 0; i < SD_MOBS; i++) {
        SdMob *m = &SD.mob[i];
        if (!m->alive) continue;
        m->t += dt; m->act_t += dt; m->hurt_t = fmaxf(0.0f, m->hurt_t - dt); m->slow_t = fmaxf(0.0f, m->slow_t - dt);
        m->touch_t = fmaxf(0.0f, m->touch_t - dt); m->sub_t -= dt; m->mem -= dt; m->alert -= dt; m->cool -= dt;
        if (m->state == SDM_DIE) { if (m->t > 0.8f) m->alive = 0; continue; }
        if (m->pois_t > 0.0f) { m->pois_t -= dt; m->hp -= m->pois_dps * dt; if (m->hp <= 0.0f) { sd_mob_slain(m, AT_FLASK); continue; } }
        float sp = m->speed * SDT[TU_DEAD_PACE] * (m->slow_t > 0.0f ? 0.5f : 1.0f);   /* the crypt's cells a second as pixels, a little slower */
        if (m->kind == MOB_HOUND && m->alert > 0.0f) sp *= 1.3f;   /* set on by the horn */
        float cy = sd_mob_cy(m), dx = SD.x - m->x, dy = (SD.y - SD_HT * 0.5f) - cy;
        int through = m->kind == MOB_WRAITH || m->kind == MOB_GHOST;
        int sees = !SD.dying && fabsf(dx) < SDT[TU_DEAD_SIGHT] && fabsf(dy) < 80.0f
                && (through || sd_line_clear(m->x, cy, SD.x, SD.y - SD_HT * 0.6f));
        if (sees || (m->alert > 0.0f && !SD.dying)) { m->lx = SD.x; m->ly = SD.y; m->mem = fmaxf(m->mem, SDT[TU_DEAD_MEMORY]); }
        int knows = !SD.dying && m->mem > 0.0f;
        if (m->hurt_t > 0.15f && !knows) { m->lx = SD.x; m->ly = SD.y; m->mem = SDT[TU_DEAD_MEMORY]; knows = 1; }   /* struck: it knows from where */
        switch (m->way) {
        case SW_WALK: case SW_THROW: case SW_LEAP: case SW_BROOD: case SW_DEACON: case SW_SEXTON: case SW_WHIP:
            if (m->act != SA_NONE && m->act != SA_RING && m->act != SA_CHANT) { sd_mob_acts(m, dt, sees, dx, dy); break; }
            if (m->way == SW_THROW) {                               /* a thrower stands off, and throws high and slow */
                if (sees && m->cool <= 0.0f && fabsf(dx) < 150.0f && fabsf(dx) > 20.0f && m->ground) {
                    m->face = dx > 0.0f ? 1 : -1; m->act = SA_WIND; m->next = SA_THROW; m->act_t = 0.0f; m->wind = sd_wind_of(m, 0.5f);
                    m->aim_x = SD.x; m->aim_y = SD.y;              /* where you stand as it winds up: step off it */
                    break;
                }
                if (sees && fabsf(dx) < 110.0f && fabsf(dx) > 36.0f) { m->vx = 0.0f; m->face = dx > 0.0f ? 1 : -1; sd_mob_body(m, dt); break; }
            }
            if (m->way == SW_LEAP && sees && m->ground && m->cool <= 0.0f && m->fa >= 0) {
                const FoeAtk *a = &ATTACKS[m->fa];
                float ad = fabsf(dx);
                if (ad > a->go * 12.0f && ad < a->go2 * 30.0f && dy > -56.0f && dy < 24.0f) {
                    m->face = dx > 0.0f ? 1 : -1; m->act = SA_WIND; m->next = SA_LEAP; m->act_t = 0.0f; m->wind = sd_wind_of(m, 0.35f);
                    m->aim_x = SD.x; m->aim_y = SD.y;
                    break;
                }
            }
            if (m->kind == MOB_SHIELD && sees && m->ground && m->cool <= 0.0f && m->sub_t < 0.0f && fabsf(dx) > 40.0f && fabsf(dx) < 96.0f && fabsf(dy) < 20.0f) {
                m->face = dx > 0.0f ? 1 : -1; m->act = SA_WIND; m->next = SA_CHARGE; m->act_t = 0.0f; m->wind = 0.5f * SDT[TU_DEAD_WIND]; m->sub_t = 8.0f;
                break;
            }
            if (m->way == SW_WHIP && sees && m->ground) {          /* the huntsman: his horn first, if he has hounds; then his whip */
                int hounds = 0;
                for (int k = 0; k < SD_MOBS; k++) hounds += SD.mob[k].alive && SD.mob[k].kind == MOB_HOUND && SD.mob[k].state != SDM_DIE;
                if (hounds && !m->kids) { m->kids = 1; m->act = SA_WIND; m->next = SA_HORN; m->act_t = 0.0f; m->wind = 1.1f; m->vx = 0.0f; break; }
                if (m->cool <= 0.0f && fabsf(dx) < ATTACKS[FA_LASH].go * 14.0f && fabsf(dy) < 24.0f) {
                    m->face = dx > 0.0f ? 1 : -1; m->act = SA_WIND; m->next = SA_LASH; m->act_t = 0.0f; m->wind = sd_wind_of(m, 0.35f);
                    break;
                }
            }
            if (m->way == SW_BROOD && sees && m->sub_t < 0.0f) {   /* she lays: a spiderling beside her, three at once at the most */
                int young = 0;
                for (int k = 0; k < SD_MOBS; k++) young += SD.mob[k].alive && SD.mob[k].state != SDM_DIE && SD.mob[k].parent == i;
                SdMob *y = young >= SD_KIDS ? NULL : sd_mob_spawn(MOB_LING, 0, m->x - (float)m->face * 8.0f, m->y, -1);
                if (y) { y->parent = i; y->mem = 4.0f; y->lx = SD.x; y->ly = SD.y; sd_dead_says(m, "lays"); sd_sfx_at(SFX_SKITTER, 0.6f, m->x, m->y - 6.0f); }
                m->sub_t = 5.0f;
            }
            if (m->way == SW_SEXTON) {                             /* he doesn't fight: he rings, and the dead near know where you are */
                if (m->act == SA_RING) {
                    m->vx = 0.0f; sd_mob_body(m, dt);
                    if ((int)(m->act_t / 0.16f) != (int)((m->act_t - dt) / 0.16f)) sd_sfx_at(SFX_HANDBELL, 0.9f, m->x, m->y - 16.0f);
                    if (m->act_t > 0.4f && m->act_t - dt <= 0.4f) { sd_call(m, 14.0f, 8.0f, -1); sd_dead_says(m, "rings"); }
                    if (m->act_t > 1.6f || m->slow_t > 0.0f) { m->act = SA_NONE; m->sub_t = 8.0f; }   /* frost stops the bell */
                    break;
                }
                if (sees && m->sub_t < 0.0f && m->slow_t <= 0.0f) { m->act = SA_RING; m->act_t = 0.0f; break; }
            }
            if (m->way == SW_DEACON) {                             /* he keeps back, calls the dead the first time, and raises the fallen */
                if (m->act == SA_CHANT) {
                    m->vx = 0.0f; sd_mob_body(m, dt);
                    if (m->hurt_t > 0.15f) { m->act = SA_NONE; m->sub_t = 3.0f; break; }   /* struck: the chant broken */
                    if (m->act_t > 2.0f) {
                        int f = (int)m->aim_x;
                        if (f >= 0 && f < sdf_n && !SDF[f].up) {
                            SdMob *r = sd_mob_spawn(MOB_SKEL, imax(0, m->rank - 1), SDF[f].x, SDF[f].y, -1);
                            if (r) { SDF[f].up = 1; r->mem = 8.0f; r->lx = SD.x; r->ly = SD.y; m->raised++; sd_sfx_at(SFX_BONES, 1.0f, r->x, r->y - 8.0f); sd_dead_says(m, "raises the fallen"); }
                        }
                        m->act = SA_NONE; m->sub_t = 6.0f;
                    }
                    break;
                }
                if (sees && m->kids == 0) { m->kids = 1; sd_call(m, 14.0f, 6.0f, -1); sd_dead_says(m, "calls the dead"); sd_sfx_at(SFX_MOAN, 1.0f, m->x, m->y - 16.0f); }
                if (knows && m->raised < SD_DEACON_RAISES && m->sub_t < 0.0f) {
                    m->sub_t = 1.0f;                               /* it looks again in a moment */
                    for (int f = 0; f < sdf_n; f++)
                        if (!SDF[f].up && fabsf(SDF[f].x - m->x) < 6.0f * SD_TILE && fabsf(SDF[f].y - m->y) < 3.0f * SD_TILE && sd_line_clear(m->x, cy, SDF[f].x, SDF[f].y - 8.0f)) {
                            m->act = SA_CHANT; m->act_t = 0.0f; m->aim_x = (float)f; sd_sfx_at(SFX_WHISPER, 0.8f, m->x, m->y - 16.0f);
                            break;
                        }
                    if (m->act == SA_CHANT) break;
                }
            }
            sd_mob_walker(m, i, dt, sp, knows, sees, dx, dy);
            sd_mob_melee(m, dt, sees);
            break;
        case SW_BAT: case SW_GARG: {                               /* round about you, then down at where you were, through, and up again */
            if (m->way == SW_GARG && !m->fly) {
                if (m->act == SA_STONE) {                          /* stone till you pass */
                    if (sees && fabsf(dx) < 3.0f * SD_TILE) { m->act = SA_SHAKE; m->act_t = 0.0f; }
                    else if (m->hurt_t > 0.0f) { m->act = SA_SHAKE; m->act_t = 0.0f; }
                    break;
                }
                if (m->act == SA_SHAKE) {                          /* it shakes itself loose: shaking where it stands */
                    m->x = m->home_x + ((int)(m->act_t * 30.0f) & 1 ? 0.6f : -0.6f);
                    if (m->act_t > 0.8f) { m->x = m->home_x; m->fly = 1; m->act = SA_NONE; m->act_t = 0.0f; sd_dead_says(m, "wakes"); sd_sfx_at(SFX_SHRIEK, 0.8f, m->x, m->y - 16.0f); sd_call(m, 8.0f, 4.0f, -1); }
                    break;
                }
            }
            float side = m->x < SD.x ? -1.0f : 1.0f, ty_over = SD.y - SD_HAND - 8.0f;   /* its middle a little over the hand: a straight
                                                                      bolt catches it as it bobs */
            if (!knows) { m->act = SA_NONE; sd_mob_fly(m, m->home_x + 30.0f * sinf(m->t * 0.7f + (float)i), m->home_y - 12.0f + 10.0f * sinf(m->t * 1.3f), sp * 0.8f, dt, 0); break; }
            if (m->act == SA_NONE) { m->act = SA_CIRCLE; m->act_t = 0.0f; m->wind = 1.0f + sd_rndf() * 0.8f; }
            if (m->act == SA_CIRCLE) {
                sd_mob_fly(m, SD.x + side * 48.0f + 10.0f * sinf(m->t * 3.0f), ty_over - 6.0f + 8.0f * sinf(m->t * 4.0f + (float)i), sp * 1.2f, dt, 0);
                if (m->act_t > m->wind && sees && m->cool <= 0.0f) { m->act = SA_WIND; m->next = SA_DIVE; m->act_t = 0.0f; m->wind = 0.3f * SDT[TU_DEAD_WIND]; if (m->way == SW_BAT) sd_sfx_at(SFX_BAT, 0.6f, m->x, m->y - 16.0f); }
                break;
            }
            if (m->act == SA_WIND) {                               /* a flutter in place: it is coming */
                m->vx *= 0.9f; m->vy *= 0.9f;
                if (m->act_t > m->wind) {
                    m->act = SA_DIVE; m->act_t = 0.0f; sd_dead_says(m, "dives");
                    float ax = SD.x - m->x, ay = (SD.y - SD_HT * 0.5f) - sd_mob_cy(m), d = fmaxf(1.0f, hypotf(ax, ay));
                    m->aim_x = ax / d; m->aim_y = ay / d;           /* where you were: it holds to that line */
                }
                break;
            }
            if (m->act == SA_DIVE) {
                float v = sp * 2.6f + 40.0f, nx = m->x + m->aim_x * v * dt, ny = m->y + m->aim_y * v * dt;
                m->vx = m->aim_x * v; m->vy = m->aim_y * v;
                int wall = sd_solid(sd_floor_px(nx + m->aim_x * 6.0f), sd_floor_px(sd_mob_cy(m) + (ny - m->y) + m->aim_y * 6.0f));
                if (!wall) { m->x = nx; m->y = ny; }
                if (sd_mob_touches(m, 0) && m->touch_t <= 0.0f) { sd_mob_strikes(m, m->way == SW_GARG ? ATTACKS[FA_SWOOP].k : sd_hit_k(m)); m->touch_t = 0.8f; m->act = SA_CLIMB; m->act_t = 0.0f; }
                else if (wall || m->act_t > 0.9f) { m->act = SA_CLIMB; m->act_t = 0.0f; }
                break;
            }
            if (m->act == SA_CLIMB) {                              /* away up, and round again */
                sd_mob_fly(m, m->x + (m->vx >= 0.0f ? 40.0f : -40.0f), ty_over - 24.0f, sp * 1.4f, dt, 0);
                if (m->act_t > 0.7f) { m->act = SA_CIRCLE; m->act_t = 0.0f; m->wind = 0.8f + sd_rndf(); m->cool = 0.4f; }
                break;
            }
            m->act = SA_NONE;
            break;
        }
        case SW_DRIFT:                                             /* a spectre: drifts to you through the stone, shrieks, reels, melts away */
            if (m->act == SA_NONE) {
                if (knows) sd_mob_fly(m, SD.x + 20.0f * sinf(m->t * 0.6f + (float)i), SD.y - SD_HT * 0.5f + 10.0f * cosf(m->t * 0.8f), sp * 1.0f, dt, 1);
                else sd_mob_fly(m, m->home_x + 20.0f * sinf(m->t * 0.5f), m->home_y - 16.0f + 8.0f * sinf(m->t), sp * 0.6f, dt, 1);
                if (sees && hypotf(dx, dy) < 26.0f) { m->act = SA_RECOIL; m->act_t = 0.0f; sd_dead_says(m, "shrieks"); sd_sfx_at(SFX_SHRIEK, 1.0f, m->x, m->y - 16.0f); }
            } else if (m->act == SA_RECOIL) {
                sd_mob_fly(m, m->x - dx, cy - dy, sp * 1.2f, dt, 1);
                if (m->act_t > 0.55f) { m->act = SA_FLEE; m->act_t = 0.0f; }
            } else {
                sd_mob_fly(m, m->x - dx * 4.0f, cy - 40.0f, sp * 3.0f, dt, 1);
                if (m->act_t > 3.0f) m->alive = 0;                 /* gone, for now: not slain */
            }
            break;
        case SW_FLY:                                               /* the shade and the wraith: come at you; the wraith through the stone */
            if (m->act == SA_WIND || m->act == SA_STRIKE) {
                m->vx *= 0.9f; m->vy *= 0.9f;
                if (m->act == SA_WIND && m->act_t > m->wind) { m->act = SA_STRIKE; m->act_t = 0.0f; }
                if (m->act == SA_STRIKE) {
                    m->x += (float)m->face * 60.0f * dt;
                    if (sd_mob_touches(m, 3)) { sd_mob_strikes(m, sd_hit_k(m)); m->act = SA_NONE; m->cool = ATTACKS[m->fa].cool; m->touch_t = 0.8f; }
                    else if (m->act_t > 0.16f) { m->act = SA_NONE; m->cool = 0.8f; }
                }
                break;
            }
            if (knows) sd_mob_fly(m, m->lx - (float)(dx > 0.0f ? 1 : -1) * 10.0f, m->ly - SD_HT * 0.5f, sp * 1.1f, dt, through);
            else sd_mob_fly(m, m->home_x + 24.0f * sinf(m->t * 0.6f + (float)i), m->home_y - 14.0f + 8.0f * sinf(m->t * 1.1f), sp * 0.6f, dt, through);
            if (sees && m->fa >= 0 && m->cool <= 0.0f && fabsf(dx) < 26.0f && fabsf(dy) < 18.0f) {
                m->face = dx > 0.0f ? 1 : -1; m->act = SA_WIND; m->next = SA_STRIKE; m->act_t = 0.0f; m->wind = sd_wind_of(m, 0.3f);
            }
            break;
        case SW_IMP:                                               /* an imp: hangs off, high, and throws its embers */
            if (m->act == SA_WIND) {
                m->vx *= 0.92f; m->vy *= 0.92f; m->x += m->vx * dt; m->y += m->vy * dt;
                sd_mob_acts(m, dt, sees, dx, dy);
                break;
            }
            if (knows) {
                float side = m->x < SD.x ? -1.0f : 1.0f;
                sd_mob_fly(m, SD.x + side * 72.0f + 12.0f * sinf(m->t * 1.3f), SD.y - SD_HT - 30.0f + 8.0f * sinf(m->t * 2.1f), sp, dt, 0);
                m->face = dx > 0.0f ? 1 : -1;
                if (sees && m->cool <= 0.0f && fabsf(dx) < 140.0f) { m->act = SA_WIND; m->next = SA_THROW; m->act_t = 0.0f; m->wind = sd_wind_of(m, 0.45f); }
            } else sd_mob_fly(m, m->home_x + 30.0f * sinf(m->t * 0.7f + (float)i), m->home_y - 14.0f + 10.0f * sinf(m->t * 1.3f), sp * 0.7f, dt, 0);
            break;
        case SW_SKULL:                                             /* a wall skull: fixed where it is set; it glows, then spits fire at you */
            m->x = m->home_x; m->y = m->home_y;
            if (dx != 0.0f) m->face = dx > 0.0f ? 1 : -1;
            if (m->act == SA_WIND) {
                if (!sees) { m->act = SA_NONE; break; }            /* out of its sight: it waits */
                sd_mob_acts(m, dt, sees, dx, dy);
                break;
            }
            if (sees && m->cool <= 0.0f && fabsf(dx) < 9.0f * SD_TILE) { m->act = SA_WIND; m->next = SA_THROW; m->act_t = 0.0f; m->wind = 1.1f * SDT[TU_DEAD_WIND]; sd_sfx_at(SFX_CRACKLE, 0.5f, m->x, m->y - 12.0f); }
            break;
        case SW_DROP: {                                            /* a spider: hangs from the roof, drops on you passing under, climbs back */
            int t, b; sd_mob_span(m, &t, &b);
            float rest = m->aim_y - (float)t + 1.0f;
            if (m->act == SA_HANG) {
                m->y = rest; m->x = m->home_x + 1.5f * sinf(m->t * 1.2f + (float)i);
                if (!SD.dying && fabsf(dx) < 20.0f && SD.y > m->y && SD.y - m->y < 11.0f * SD_TILE && sd_line_clear(m->x, m->y, SD.x, SD.y - 4.0f)) {
                    m->act = SA_WIND; m->next = SA_DROP; m->act_t = 0.0f; m->wind = 0.25f * SDT[TU_DEAD_WIND]; sd_sfx_at(SFX_SKITTER, 0.6f, m->x, m->y - 8.0f);
                }
            } else if (m->act == SA_WIND) {
                m->x = m->home_x + ((int)(m->act_t * 30.0f) & 1 ? 0.8f : -0.8f);
                if (m->act_t > m->wind) { m->act = SA_DROP; m->act_t = 0.0f; m->vy = 0.0f; m->x = m->home_x; sd_dead_says(m, "drops"); }
            } else if (m->act == SA_DROP) {
                m->vy = fminf(260.0f, m->vy + SD_G * 0.8f * dt); m->y += m->vy * dt;
                if (sd_mob_touches(m, 0) && m->touch_t <= 0.0f) { sd_mob_strikes(m, 0.7f); m->touch_t = 1.0f; m->act = SA_RISE; m->act_t = 0.0f; }
                else if (sd_solid(sd_floor_px(m->x), sd_floor_px(m->y)) || m->y - rest > 11.0f * SD_TILE) {
                    if (sd_solid(sd_floor_px(m->x), sd_floor_px(m->y))) m->y = (float)(sd_floor_px(m->y) * SD_TILE);
                    m->act = SA_RISE; m->act_t = -0.6f;            /* (a moment on the floor first) */
                }
            } else {                                               /* up its thread */
                if (m->act_t > 0.0f) m->y = fmaxf(rest, m->y - 50.0f * dt);
                if (m->y <= rest) { m->act = SA_HANG; m->cool = 1.5f; }
            }
            break;
        }
        case SW_HOP: {                                             /* the skull hopper: bounds between its two places, high; settles a moment */
            float span = (m->thing >= 0 && SD.L->th[m->thing].f[0] > 0.0f ? SD.L->th[m->thing].f[0] : 4.0f) * SD_TILE;
            if (m->ground) {
                m->vx = 0.0f; sd_mob_body(m, dt);
                if (m->act_t < 0.45f || !m->ground) break;
                float lo = m->home_x - span, hi = m->home_x + span, h = 2.5f * SD_TILE, T = 2.0f * sqrtf(2.0f * h / SD_G);
                if ((m->face > 0 && m->x >= hi - 3.0f) || (m->face < 0 && m->x <= lo + 3.0f)) m->face = -m->face;
                float along = fminf(2.0f * SD_TILE, fabsf((m->face > 0 ? hi : lo) - m->x));
                if (along < 6.0f) { m->face = -m->face; along = fminf(2.0f * SD_TILE, span); }
                float lx = m->x + (float)m->face * along;
                if (!sd_floor_at(lx, m->y, 1) || sd_mob_blocked(m, lx + (float)m->face * SD_MOB_HW)) {   /* no floor there, or stone: the other way */
                    m->face = -m->face; lx = m->x + (float)m->face * along;
                    if (!sd_floor_at(lx, m->y, 1)) { m->act_t = 0.0f; break; }
                }
                sd_mob_jump(m, h, (float)m->face * along / T);
                m->act_t = 0.0f;
            } else {
                sd_mob_body(m, dt);
                if (m->ground) { m->act_t = 0.0f; m->vx = 0.0f; sd_sfx_at(SFX_BONES, 0.35f, m->x, m->y - 6.0f); }
            }
            break;
        }
        case SW_LEECH: {                                           /* under the poison; up at what is over it, and back in */
            float surf = m->home_y;
            if (m->act == SA_HIDE) {
                m->y = surf + 10.0f; m->vx = m->vy = 0.0f;
                float ax = fabsf(SD.x - m->x);
                if (!SD.dying && m->cool <= 0.0f && ax < 5.0f * SD_TILE && SD.y < surf + 2.0f && SD.y > surf - 9.0f * SD_TILE) {
                    m->act = SA_WIND; m->act_t = 0.0f; m->wind = 0.4f * SDT[TU_DEAD_WIND];
                    sd_sfx_at(SFX_DRIP, 0.6f, m->x, surf);
                }
            } else if (m->act == SA_WIND) {                        /* bubbles at the surface: then up */
                m->y = surf + 10.0f;
                if (m->act_t >= m->wind) {
                    float hgt = clampf(surf - (SD.y - SD_HT) + 10.0f, 24.0f, 110.0f), vy = sqrtf(2.0f * SD_G * hgt), up = vy / SD_G;
                    m->vy = -vy; m->vx = clampf((SD.x - m->x) / up, -150.0f, 150.0f); m->face = m->vx >= 0.0f ? 1 : -1;
                    m->y = surf; m->act = SA_LEAP; m->ground = 0; m->act_t = 0.0f;
                    sd_sfx_at(SFX_SPLASH, 0.7f, m->x, surf); sd_dead_says(m, "leaps");
                }
            } else if (m->act == SA_LEAP) {                        /* its arc; back into the poison, or stranded on stone */
                m->vy = fminf(SD_FALL_MAX, m->vy + SD_G * dt);
                float nx = m->x + m->vx * dt, ny = m->y + m->vy * dt;
                if (sd_solid(sd_floor_px(nx + (m->vx > 0.0f ? 5.0f : -5.0f)), sd_floor_px(m->y - 4.0f))) m->vx = 0.0f; else m->x = nx;
                if (m->vy < 0.0f && sd_solid(sd_floor_px(m->x), sd_floor_px(ny - 10.0f))) m->vy = 0.0f;
                else m->y = ny;
                int tx = sd_floor_px(m->x), ty = sd_floor_px(m->y - 1.0f);
                if (m->vy > 0.0f && (sd_tile(tx, ty) == '~' || sd_tile(tx, ty + 1) == '~') && m->y >= (float)(ty * SD_TILE) + 4.0f) {
                    int top = ty; while (top > 0 && sd_tile(tx, top - 1) == '~') top--;
                    m->home_y = (float)(top * SD_TILE) + 4.0f; m->act = SA_HIDE; m->cool = 1.8f; m->vy = 0.0f;
                    sd_sfx_at(SFX_SPLASH, 0.6f, m->x, m->home_y);
                } else if (m->vy > 0.0f && sd_floor_at(m->x, m->y, 1)) {
                    m->y = (float)(sd_floor_px(m->y - 0.001f) * SD_TILE); m->vy = m->vx = 0.0f; m->ground = 1; m->act = SA_LAND; m->act_t = 0.0f;
                }
                if (m->y > (float)(SD.L->h * SD_TILE + 64)) m->alive = 0;
            } else {                                               /* stranded: it flops toward its pool */
                if (!m->ground) { m->act = SA_LEAP; m->vx = 0.0f; break; }   /* (not on the stone: it falls, as in its leap) */
                if (m->act_t > 0.7f && m->ground) {
                    float d = m->home_x - m->x;
                    m->vx = 0.0f; m->vy = -sqrtf(2.0f * SD_G * 12.0f); m->vx = clampf(d, -40.0f, 40.0f); m->ground = 0; m->act = SA_LEAP; m->act_t = 0.0f;
                }
            }
            break;
        }
        case SW_CRAB: {                                            /* still, a coin, till you come near; then along its way, to its place */
            if (m->act == SA_COIN) {
                if (!SD.dying && hypotf(SD.x - m->x, SD.y - m->y) < 4.0f * SD_TILE) {
                    m->act = SA_RUN; m->act_t = 0.0f; sd_sfx_at(SFX_SKITTER, 0.7f, m->x, m->y - 4.0f); sd_dead_says(m, "runs");
                }
            } else if (m->act == SA_RUN) {
                float px[64], py[64];
                int n = sd_crab_points(m, px, py, 64);
                float go = sp * 1.7f * dt;                         /* quick: a coin's worth is a chase */
                while (go > 0.0f && m->sub_t < (float)(n - 1)) {
                    int k = (int)m->sub_t;
                    float f = m->sub_t - (float)k, sx = px[k + 1] - px[k], sy = py[k + 1] - py[k], len = fmaxf(0.01f, hypotf(sx, sy)), left = (1.0f - f) * len;
                    if (go >= left) { go -= left; m->sub_t = (float)(k + 1); }
                    else { m->sub_t += go / len; go = 0.0f; }
                }
                int k = imin((int)m->sub_t, n - 1);
                float f = k < n - 1 ? m->sub_t - (float)k : 0.0f, ox = m->x;
                m->x = k < n - 1 ? px[k] + (px[k + 1] - px[k]) * f : px[n - 1];
                m->y = k < n - 1 ? py[k] + (py[k + 1] - py[k]) * f : py[n - 1];
                if (m->x != ox) m->face = m->x > ox ? 1 : -1;
                if (m->sub_t >= (float)(n - 1)) { m->act = SA_CORNER; m->act_t = 0.0f; sd_dead_says(m, "is cornered"); }
            }
            if (!SD.dying && sd_mob_touches(m, 0)) {               /* caught: it is yours */
                sd_dead_says(m, "is caught");
                m->hp = 0.0f; sd_mob_slain(m, -1);
            }
            break;
        }
        case SW_SAC:                                               /* an egg sac: still, till you come close; then the spiderlings */
            if (!SD.dying && fabsf(dx) < 34.0f && fabsf(dy) < 30.0f) {
                for (int k = 0; k < 3; k++) {
                    SdMob *y = sd_mob_spawn(MOB_LING, 0, m->x + (float)(k - 1) * 6.0f, m->y, -1);
                    if (y) { y->mem = 5.0f; y->lx = SD.x; y->ly = SD.y; y->face = k == 0 ? -1 : 1; }
                }
                sd_sfx_at(SFX_SHATTER, 0.8f, m->x, m->y - 8.0f); sd_dead_says(m, "hatches");
                m->state = SDM_DIE; m->t = 0.0f;
                if (m->thing >= 0) SDP.slain[SD.side][m->thing] = 1;
            }
            break;
        }
        if (!m->alive || m->state == SDM_DIE) continue;
        /* its body against yours: a bump, pixel on pixel (the harmless do nothing) */
        if (KINDS[m->kind].harm && m->touch_t <= 0.0f && m->act != SA_STONE && m->way != SW_SKULL && sd_mob_touches(m, 0)) {
            sd_mob_strikes(m, 0.5f);
            m->touch_t = 1.0f;
        }
    }
#ifdef CRAWL_DEBUG
    static int trace = -1;
    if (trace < 0) trace = getenv("CRAWL_SIDEMOBS") != NULL;
    if (trace && (int)(SD.t * 2.0f) != (int)((SD.t - dt) * 2.0f))  /* CRAWL_SIDEMOBS: each of the dead, twice a second */
        for (int i = 0; i < SD_MOBS; i++) {
            const SdMob *m = &SD.mob[i];
            if (m->alive && m->state != SDM_DIE)
                fprintf(stderr, "SIDE mob %d %s at %.0f,%.0f act %d next %d %s hp %.0f t=%.2f\n", i, KINDS[m->kind].name, m->x, m->y, m->act, m->next,
                        m->mem > 0.0f ? "knows" : "-", m->hp, SD.t);
        }
#endif
}
/* ---- shots: the caster's spells and what the dead throw; each moved a pixel at a time, each pixel of it tried ---- */
static void sd_burst(SdShot *s) {                                  /* the flask: a splash a tile and a half round, to the pixels it reaches */
    for (int i = 0; i < SD_MOBS; i++) {
        SdMob *m = &SD.mob[i];
        if (!m->alive || m->state == SDM_DIE) continue;
        if (sd_mob_near(m, s->x, s->y) < 26.0f) sd_mob_hit(m, s->dmg, s->sp, EL_POISON);
    }
    sd_sfx_at(SFX_SPLASH, 0.7f, s->x, s->y);
    sd_kit_burst(s->x, s->y);
    sd_verm_burst(s->x, s->y);                                      /* masonry broken, a bell rung (side_kit.c) */
    s->alive = 0;
}
static int sd_shot_meets(SdShot *s) {                              /* 1: it struck something (and is spent, or burst) */
    int dx[SD_SHOT_PX], dy[SD_SHOT_PX], n = sd_shot_shape(s, dx, dy);
    int cx = (int)floorf(s->x), cy = (int)floorf(s->y);
    if (s->foe) {
        if (SD.dying || SD.inv_t > 0.0f) return 0;
        for (int k = 0; k < n; k++)
            if (sd_fig_px(cx + dx[k], cy + dy[k])) {
                G.blame = (Blame){ BL_MOB, s->from >= 0 ? s->from : MOB_SKEL, 0 };
                if (s->from >= 0) sd_foe_kind(s->from, s->x - s->vx * 0.2f, s->y);   /* the panel: who threw it (2j) */
                sd_hurt(s->dmg, s->el, s->x - s->vx);
                s->alive = 0;
                return 1;
            }
        return 0;
    }
    for (int i = 0; i < SD_MOBS; i++) {
        SdMob *m = &SD.mob[i];
        if (!m->alive || m->state == SDM_DIE) continue;
        int l, t, r, b;
        sd_mob_box(m, &l, &t, &r, &b);
        if (cx + 4 < l || cx - 4 > r || cy + 4 < t || cy - 4 > b) continue;
        for (int k = 0; k < n; k++)
            if (sd_mob_px(m, cx + dx[k], cy + dy[k])) {
                if (s->kind == SS_FLASK) { sd_burst(s); return 1; }
                sd_mob_hit(m, s->dmg, s->sp, s->el);
                s->alive = 0;
                return 1;
            }
    }
    return 0;
}
static void sd_shots(float dt) {
    for (int i = 0; i < SD_SHOTS; i++) {
        SdShot *s = &SD.shot[i];
        if (!s->alive) continue;
        s->vy += s->g * dt;
        float slow = sd_kit_web_at(s->x, s->y) ? SDT[TU_WEB] : 1.0f;   /* a cobweb holds what flies through it */
        float ox = s->x, oy = s->y, mx = s->vx * dt * slow, my = s->vy * dt * slow;
        int n = imax(1, (int)ceilf(fmaxf(fabsf(mx), fabsf(my))));  /* a pixel a time, so nothing thin is passed through */
        s->life -= dt;
        for (int k = 1; k <= n && s->alive; k++) {
            float px = s->x, py = s->y;
            s->x = ox + mx * (float)k / (float)n; s->y = oy + my * (float)k / (float)n;
            if (!s->foe && s->el == EL_FIRE) { sd_fx_ignite(s->x, s->y); sd_kit_burn(s->x, s->y); }   /* fire through silt gas sets it alight
                                                                      (side_fx.c, 2e); through a cobweb, burns it */
            int body = KB.n ? sd_body_at(s->x, s->y, s->x, s->y, -1) : -1, bell = !s->foe && sd_kit_bell_hit(s->x, s->y);
            if (body >= 0 || bell) {                               /* a press's head, a sarcophagus; a bell, rung */
                if (s->kind == SS_FLASK) { s->x = px; s->y = py; sd_burst(s); } else s->alive = 0;
                break;
            }
            if (sd_solid(sd_floor_px(s->x), sd_floor_px(s->y))) {
                if (!s->foe) sd_kit_struck(sd_floor_px(s->x), sd_floor_px(s->y));   /* cracked masonry: broken */
                if (s->kind == SS_FLASK) { s->x = px; s->y = py; sd_burst(s); }
                else {
                    if (s->kind == SS_BONE) sd_sfx_at(SFX_BONES, 0.25f, s->x, s->y);
                    s->alive = 0;
                }
                break;
            }
            if (sd_shot_meets(s)) break;
            if (!s->foe && sd_verm_shot(s)) { if (s->kind == SS_FLASK) sd_burst(s); else s->alive = 0; break; }   /* vermin (side_vermin.c) */
        }
        if (s->alive && s->life <= 0.0f) { if (s->kind == SS_FLASK) sd_burst(s); else s->alive = 0; }
        if (s->alive && s->y > (float)(SD.L->h * SD_TILE + 32)) s->alive = 0;
    }
}
