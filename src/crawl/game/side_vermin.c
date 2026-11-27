/* A part of scene_crawl.c (docs/PLAN-tidy.md): it is #included there in its place and never compiled on its own.
 * Vermin in a side passage (docs/PLAN-vermin.md): rats and spiderlings, a tile each, not kinds: no record, no souls, nothing saved.
 * A rat runs its floor back and forth, turning at walls and edges; a spiderling keeps to the stone, along floors, up walls and over
 * ceilings, turning every corner, and drops on the caster passing under it. Each bites on touch, a nudge; each dies to a spell, a
 * flask's splash or lightning, struck by its pixels as it is drawn. The dream is not taught them. */

#define SD_VERM 32
#define VERM_SPEED_RAT 62.0f
#define VERM_SPEED_LING 46.0f
typedef struct {
    int alive, kind, thing, state;                                 /* kind 0 rat, 1 spiderling; state: 0 going, 1 falling, 2 dying */
    float x, y, vy, t, cool, die_t;                                /* a rat: its feet; a spiderling: the middle of its cell, drawn hugging its stone */
    int face;                                                      /* a rat's way, -1 or 1 */
    int cx, cy, dx, dy, sx, sy, wrap;                              /* a spiderling: its cell, its way along the stone, the stone's side; turning a corner */
    float k;                                                       /* how far to the next cell, 0 to 1 */
} SdVerm;
static SdVerm VRM[SD_VERM];

static int verm_solid(int tx, int ty) { return sd_solid(tx, ty); }
static void verm_says(const SdVerm *v, const char *what) {
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT side_vermin %s %s at %.0f,%.0f t=%.2f\n", v->kind ? "spiderling" : "rat", what, v->x, v->y, SD.t);
#else
    (void)v; (void)what;
#endif
}
/* a spiderling's place drawn: its cell's middle moved toward the next, and pressed to its stone */
static void verm_pos(const SdVerm *v, float *x, float *y) {
    if (v->kind == 0 || v->state == 1) { *x = v->x; *y = v->y; return; }
    *x = (float)(v->cx * SD_TILE) + 8.0f + (float)v->dx * v->k * SD_TILE + (float)v->sx * 4.0f;
    *y = (float)(v->cy * SD_TILE) + 8.0f + (float)v->dy * v->k * SD_TILE + (float)v->sy * 4.0f;
}
/* how it is drawn: the tile, its turn (0 upright, 1 its feet to the right, 2 upside down, 3 its feet to the left) and whether flipped */
static void verm_pose(const SdVerm *v, int *tile, int *rot, int *flip, int *ox, int *oy) {
    float x, y; verm_pos(v, &x, &y);
    *tile = (v->kind ? ST_LING0 : ST_RAT0) + (((int)(v->t * 8.0f)) & 1);
    if (v->kind == 0 || v->state == 1) { *rot = 0; *flip = v->face < 0 || (v->kind && v->state == 1 && v->dx < 0); *ox = (int)x - 8; *oy = (int)y - 16 + (v->kind ? 8 : 0); return; }
    *rot = v->sy > 0 ? 0 : v->sx > 0 ? 1 : v->sy < 0 ? 2 : 3;
    int fwd = *rot == 0 ? v->dx > 0 : *rot == 1 ? v->dy < 0 : *rot == 2 ? v->dx < 0 : v->dy > 0;   /* going the way its picture faces */
    *flip = !fwd; *ox = (int)x - 8; *oy = (int)y - 8;
}
/* a pixel of the tile at (u, v) on the screen, turned and flipped as drawn */
static void verm_map(int rot, int flip, int u, int w, int *x, int *y) {
    if (flip) u = STS - 1 - u;
    switch (rot) {
    case 1: *x = w; *y = STS - 1 - u; break;
    case 2: *x = STS - 1 - u; *y = STS - 1 - w; break;
    case 3: *x = STS - 1 - w; *y = u; break;
    default: *x = u; *y = w; break;
    }
}
static int verm_px(const SdVerm *v, int wx, int wy) {               /* one of its pixels at (wx, wy) in the passage */
    if (!v->alive || v->state == 2) return 0;
    int tile, rot, flip, ox, oy;
    verm_pose(v, &tile, &rot, &flip, &ox, &oy);
    if (wx < ox || wy < oy || wx >= ox + STS || wy >= oy + STS) return 0;
    for (int w = 0; w < STS; w++) for (int u = 0; u < STS; u++) {   /* (small: each of its pixels where it lands) */
        int x, y; verm_map(rot, flip, u, w, &x, &y);
        if (ox + x == wx && oy + y == wy) return side_tile[tile][w * STS + u] != C_CLEAR;
    }
    return 0;
}
static int verm_touches(const SdVerm *v) {                          /* its pixels over the caster's */
    if (SD.dying || !v->alive || v->state == 2) return 0;
    float x, y; verm_pos(v, &x, &y);
    if (fabsf(x - SD.x) > 16.0f || fabsf(y - (SD.y - 8.0f)) > 20.0f) return 0;
    int tile, rot, flip, ox, oy;
    verm_pose(v, &tile, &rot, &flip, &ox, &oy);
    for (int w = 0; w < STS; w++) for (int u = 0; u < STS; u++) {
        if (side_tile[tile][w * STS + u] == C_CLEAR) continue;
        int px, py; verm_map(rot, flip, u, w, &px, &py);
        if (sd_fig_px(ox + px, oy + py)) return 1;
    }
    return 0;
}
static void verm_kill(SdVerm *v, const char *how) {
    if (v->state == 2) return;
    v->state = 2; v->die_t = 0.0f;
    float x, y; verm_pos(v, &x, &y); v->x = x; v->y = y;
    sd_sfx_at(v->kind ? SFX_SKITTER : SFX_RAT_SQUEAL, 0.6f, x, y);
    verm_says(v, how);
}

/* ---- laid as the passage opens ---- */
static void sd_verm_build(void) {
    memset(VRM, 0, sizeof VRM);
    const SdLevel *L = SD.L;
    int n = 0;
    for (int i = 0; i < L->nth && n < SD_VERM; i++) {
        const SdThing *t = &L->th[i];
        if (t->kind != SDT_VERMIN) continue;
        SdVerm *v = &VRM[n++];
        v->alive = 1; v->kind = t->a; v->thing = i; v->face = (i & 1) ? -1 : 1; v->t = (float)i * 0.37f;
        v->x = (float)(t->x * SD_TILE) + 8.0f; v->y = (float)((t->y + 1) * SD_TILE);
        if (v->kind == 1) {                                        /* a spiderling: the stone nearest, below first */
            v->cx = t->x; v->cy = t->y;
            static const int S4[4][2] = { { 0, 1 }, { 1, 0 }, { -1, 0 }, { 0, -1 } };
            v->state = 1; v->x = (float)(t->x * SD_TILE) + 8.0f; v->y = (float)(t->y * SD_TILE) + 8.0f;
            for (int k = 0; k < 4; k++) if (verm_solid(t->x + S4[k][0], t->y + S4[k][1])) {
                v->sx = S4[k][0]; v->sy = S4[k][1]; v->state = 0;
                v->dx = -v->sy * v->face; v->dy = v->sx * v->face;   /* along it, one way or the other */
                if (!v->dx && !v->dy) v->dx = 1;
                break;
            }
        }
    }
}

/* ---- each step ---- */
static void verm_ling(SdVerm *v, float dt) {
    if (v->state == 1) {                                           /* falling: onto the floor, then along it toward where the caster was */
        v->vy = fminf(SD_FALL_MAX, v->vy + SD_G * dt);
        float ny = v->y + v->vy * dt;
        int tx = sd_floor_px(v->x), ty = sd_floor_px(ny + 4.0f);
        if (verm_solid(tx, ty)) {
            v->state = 0; v->cx = tx; v->cy = ty - 1; v->sx = 0; v->sy = 1; v->k = 0.0f;
            v->dx = SD.x >= v->x ? 1 : -1; v->dy = 0; v->wrap = 0;
            verm_says(v, "lands");
        } else v->y = ny;
        if (v->y > (float)(SD.L->h * SD_TILE + 64)) v->alive = 0;
        return;
    }
    if (!verm_solid(v->cx + v->sx, v->cy + v->sy) && !v->wrap) {    /* its stone gone (a stone sunk, masonry broken): it falls */
        v->state = 1; v->x = (float)(v->cx * SD_TILE) + 8.0f; v->y = (float)(v->cy * SD_TILE) + 8.0f; v->vy = 0.0f;
        return;
    }
    if (v->sy < 0 && !SD.dying) {                                  /* on a ceiling, over the caster: it lets go */
        float x, y; verm_pos(v, &x, &y);
        if (fabsf(SD.x - x) < 8.0f && SD.y > y && SD.y - y < 9.0f * SD_TILE && sd_line_clear(x, y + 6.0f, SD.x, SD.y - SD_HT)) {
            v->state = 1; v->x = x; v->y = y; v->vy = 0.0f; sd_sfx_at(SFX_SKITTER, 0.6f, x, y); verm_says(v, "drops");
            return;
        }
    }
    v->k += VERM_SPEED_LING * dt / SD_TILE;
    while (v->k >= 1.0f) {                                         /* into the next cell: then which way on */
        v->k -= 1.0f; v->cx += v->dx; v->cy += v->dy;
        if (v->wrap == 1) {                                        /* round a corner it was going over: its way down the stone's side */
            int ndx = v->sx, ndy = v->sy; v->sx = -v->dx; v->sy = -v->dy; v->dx = ndx; v->dy = ndy; v->wrap = 2;   /* (2: on its way round) */
            continue;
        }
        v->wrap = 0;
        for (int turn = 0; turn < 4; turn++) {
            if (verm_solid(v->cx + v->dx, v->cy + v->dy)) {         /* stone ahead: up it (the stone's side becomes its way) */
                int ndx = -v->sx, ndy = -v->sy; v->sx = v->dx; v->sy = v->dy; v->dx = ndx; v->dy = ndy;
                continue;
            }
            if (!verm_solid(v->cx + v->dx + v->sx, v->cy + v->dy + v->sy)) v->wrap = 1;   /* its stone ends ahead: over the corner */
            break;
        }
    }
}
static void verm_rat(SdVerm *v, float dt) {
    int ground = sd_floor_at(v->x, v->y, 1) && fmodf(v->y, (float)SD_TILE) < 0.5f;
    if (!ground) {                                                 /* off its floor: it falls */
        v->vy = fminf(SD_FALL_MAX, v->vy + SD_G * dt);
        float ny = v->y + v->vy * dt;
        int r = sd_floor_px(ny - 0.001f);
        if (verm_solid(sd_floor_px(v->x), r) || sd_tile(sd_floor_px(v->x), r) == '=') { v->y = (float)(r * SD_TILE); v->vy = 0.0f; }
        else v->y = ny;
        if (v->y > (float)(SD.L->h * SD_TILE + 64)) v->alive = 0;
        return;
    }
    float nx = v->x + (float)v->face * VERM_SPEED_RAT * dt, lead = nx + (float)v->face * 6.0f;
    int wall = verm_solid(sd_floor_px(lead), sd_floor_px(v->y - 4.0f)), edge = !sd_floor_at(lead, v->y, 1), bad = sd_tile(sd_floor_px(lead), sd_floor_px(v->y - 1.0f)) == '^';
    if (wall || edge || bad) v->face = -v->face; else v->x = nx;   /* a wall, an edge, spikes: about */
}
static void sd_verm(float dt) {
#ifdef CRAWL_DEBUG
    static int trace = -1;
    if (trace < 0) trace = getenv("CRAWL_SIDEMOBS") != NULL;
    if (trace && (int)(SD.t * 2.0f) != (int)((SD.t - dt) * 2.0f))  /* CRAWL_SIDEMOBS: each of them too, twice a second */
        for (int i = 0; i < SD_VERM; i++) {
            const SdVerm *v = &VRM[i];
            float x, y; verm_pos(v, &x, &y);
            if (v->alive) fprintf(stderr, "SIDE vermin %d %s at %.0f,%.0f stone %d,%d way %d,%d state %d t=%.2f\n", i, v->kind ? "spiderling" : "rat", x, y, v->sx, v->sy, v->dx, v->dy, v->state, SD.t);
        }
#endif
    for (int i = 0; i < SD_VERM; i++) {
        SdVerm *v = &VRM[i];
        if (!v->alive) continue;
        v->t += dt; v->cool = fmaxf(0.0f, v->cool - dt);
        if (v->state == 2) { if ((v->die_t += dt) > 0.6f) v->alive = 0; continue; }
        if (v->kind) verm_ling(v, dt); else verm_rat(v, dt);
        if (v->cool <= 0.0f && verm_touches(v)) {                  /* a bite, a nudge */
            float x, y; verm_pos(v, &x, &y);
            G.blame = (Blame){ BL_MOB, v->kind ? MOB_LING : MOB_RAT, 0 };
            sd_knock = 0.3f;
            sd_hurt(2.0f + 0.4f * (float)G.level, EL_PHYS, x);
            v->cool = 0.9f;
            verm_says(v, "bites");
        }
    }
}

/* ---- what strikes them: the caster's shots, a flask's splash, lightning ---- */
static int sd_verm_shot(const SdShot *s) {                         /* a pixel of the shot over one: it dies, and the shot is spent */
    for (int i = 0; i < SD_VERM; i++) {
        SdVerm *v = &VRM[i];
        if (!v->alive || v->state == 2) continue;
        float x, y; verm_pos(v, &x, &y);
        if (fabsf(s->x - x) > 10.0f || fabsf(s->y - y) > 18.0f) continue;
        int dx[SD_SHOT_PX], dy[SD_SHOT_PX], n = sd_shot_shape(s, dx, dy), cx = (int)floorf(s->x), cy = (int)floorf(s->y);
        for (int k = 0; k < n; k++) if (verm_px(v, cx + dx[k], cy + dy[k])) { verm_kill(v, "struck"); return 1; }
    }
    return 0;
}
static void sd_verm_burst(float bx, float by) {
    for (int i = 0; i < SD_VERM; i++) {
        SdVerm *v = &VRM[i];
        if (!v->alive || v->state == 2) continue;
        float x, y; verm_pos(v, &x, &y);
        if (hypotf(x - bx, y - by) < 26.0f) verm_kill(v, "splashed");
    }
}
static void sd_verm_beam(float x0, float y0, float x1) {           /* lightning: any along its line, within three pixels */
    for (int i = 0; i < SD_VERM; i++) {
        SdVerm *v = &VRM[i];
        if (!v->alive || v->state == 2) continue;
        float x, y; verm_pos(v, &x, &y);
        if (x < fminf(x0, x1) - 8.0f || x > fmaxf(x0, x1) + 8.0f || fabsf(y - y0) > 12.0f) continue;
        for (int yy = (int)y0 - 3; yy <= (int)y0 + 3; yy++)
            for (int xx = (int)fminf(x0, x1); xx <= (int)fmaxf(x0, x1); xx++) if (verm_px(v, xx, yy)) { verm_kill(v, "struck by lightning"); goto next; }
    next:;
    }
}
