/* A part of scene_crawl.c (docs/PLAN-tidy.md): it is #included there in its place and never compiled on its own.
 * Side passages (docs/PLAN-side.md, 2e): what pours, drifts and creeps in a passage, as particles from emitters the World Editor
 * tunes; the censer on its chain; and run I's traps with their signs while idle: the rockfall's crack and the rubble it leaves, the
 * votive sconce that tips and pours its wax, the embalmers' slab and its breath, the cold vault's crack and its rime, the hungry
 * reliquary's haze. Wax and rime settle as a falling sand does, on a grid of two pixels: down, then aside while they are warm. */

enum { PK_WAX, PK_VAPOUR, PK_RIME, PK_HAZE, PK_SMOKE, PK_GRIT, NPK };
#define SDP_MAX 900
#define SD_CENSERS 4
#define SDC_LINKS 10
#define SDF_OCC_W (SD_W * SD_TILE / 2)
#define SDF_OCC_H (SD_H * SD_TILE / 2)
typedef struct { float x, y, vx, vy, age, life, heat; uint8_t kind, h, rest, harm; int8_t dir; } SdPart;
typedef struct { int h, i; float px, py, r, a0, a1, hang, swing, t, bx, by, bvx, smoke, lx[SDC_LINKS + 1], ly[SDC_LINKS + 1], ox[SDC_LINKS + 1], oy[SDC_LINKS + 1]; int side; } SdCenser;
static struct {
    SdPart p[SDP_MAX]; int n;
    uint8_t occ[SDF_OCC_W * SDF_OCC_H / 8];                        /* a bit for each two pixels square a settled particle fills */
    float acc[SD_HAZ], bite[SD_HAZ], tip[SD_HAZ], ca_t;           /* each emitter's particles owed; bites; a sconce's tilt, 0 to 1 */
    int thing[SD_HAZ], vdir[SD_HAZ], vn[SD_HAZ];                  /* the trap's thing; a sconce's way and how many cells were painted */
    uint8_t rubble[SD_HZC];                                        /* the rockfall's rubble lying on a painted cell */
    uint8_t danger[SD_H][SD_W];                                    /* harmful particles in each tile, for the dream */
    SdCenser c[SD_CENSERS]; int nc;
} FX;
static int sd_hz_cell(const SdHaz *z, int c, int *x, int *y);     /* side_works.c */
static void sd_hz_strike(int i, int h, float share);
static float sd_hz_warn(const SdHaz *z);
static int sd_fx_kind(int k) { return k == HZ_VOTIVE || k == HZ_CENSER || k == HZ_BREATH || k == HZ_COLD || k == HZ_RELIQ; }

/* ---- each kind's own emitter, where the editor leaves it alone ---- */
static float sdz(float v, float d) { return v >= 0.0f ? v : d; }
static float sd_fx_rate(const SdHaz *z, int h) {
    switch (z->kind) {
    case HZ_VOTIVE: return sdz(z->e_rate, fminf(90.0f, 18.0f * (float)imax(1, FX.vn[h])));   /* more painted, a heavier pour */
    case HZ_BREATH: return sdz(z->e_rate, 26.0f);
    case HZ_COLD: return sdz(z->e_rate, 20.0f);
    case HZ_RELIQ: return sdz(z->e_rate, 34.0f);
    default: return sdz(z->e_rate, 0.0f);
    }
}
static float sd_fx_speed(const SdHaz *z) { return sdz(z->e_speed, z->kind == HZ_VOTIVE ? 1.3f : z->kind == HZ_BREATH ? 0.7f : z->kind == HZ_COLD ? 0.9f : 1.6f) * SD_TILE; }
static float sd_fx_spread(const SdHaz *z) { return sdz(z->e_spread, z->kind == HZ_VOTIVE ? 14.0f : z->kind == HZ_BREATH ? 55.0f : z->kind == HZ_COLD ? 35.0f : 180.0f); }
static float sd_fx_life(const SdHaz *z) { return sdz(z->e_life, z->kind == HZ_VOTIVE ? 7.0f : z->kind == HZ_BREATH ? 3.5f : z->kind == HZ_COLD ? 4.5f : 2.0f); }
static float sd_fx_radial(const SdHaz *z) { return sdz(z->radial, 1.0f); }
static float sd_fx_on_for(const SdHaz *z) { return z->kind == HZ_VOTIVE ? fminf(z->hold, 2.2f) : fminf(z->hold, 4.0f); }

/* ---- the grid settled particles fill ---- */
static int sdf_solid_px(int px, int py) {
    if (px < 0 || py < 0 || px >= SD.L->w * SD_TILE) return 1;
    if (py >= SD.L->h * SD_TILE) return 0;
    int tx = px / SD_TILE, ty = py / SD_TILE;
    if (sd_solid(tx, ty)) return 1;
    return sd_tile(tx, ty) == '=' && (py & 15) < 2;                /* a ledge: its top */
}
static int sdf_occ(int cx, int cy) { return cx >= 0 && cy >= 0 && cx < SDF_OCC_W && cy < SDF_OCC_H && (FX.occ[(cy * SDF_OCC_W + cx) >> 3] >> (cx & 7) & 1); }
static void sdf_mark(int cx, int cy, int on) {
    if (cx < 0 || cy < 0 || cx >= SDF_OCC_W || cy >= SDF_OCC_H) return;
    uint8_t *b = &FX.occ[(cy * SDF_OCC_W + cx) >> 3];
    if (on) *b |= (uint8_t)(1 << (cx & 7)); else *b &= (uint8_t)~(1 << (cx & 7));
}
static int sdf_free(int cx, int cy) { return !sdf_occ(cx, cy) && !sdf_solid_px(cx * 2, cy * 2) && !sdf_solid_px(cx * 2 + 1, cy * 2 + 1); }

static void sd_fx_spawn(int kind, int h, float x, float y, float ang, float spread, float speed, float life, int harm) {
    if (FX.n >= SDP_MAX) return;
    SdPart *p = &FX.p[FX.n++];
    float a = (ang + (sd_rndf() * 2.0f - 1.0f) * spread) * (PI_F / 180.0f), s = speed * (0.7f + 0.6f * sd_rndf());
    memset(p, 0, sizeof *p);
    p->x = x; p->y = y; p->vx = cosf(a) * s; p->vy = -sinf(a) * s;
    p->life = life * (0.8f + 0.4f * sd_rndf()); p->heat = 1.0f; p->kind = (uint8_t)kind; p->h = (uint8_t)h; p->harm = (uint8_t)harm;
    p->dir = (int8_t)(p->vx < 0.0f ? -1 : 1);
}
static void sd_fx_kill(int k) {
    SdPart *p = &FX.p[k];
    if (p->rest) sdf_mark((int)(p->x * 0.5f), (int)(p->y * 0.5f), 0);
    FX.p[k] = FX.p[--FX.n];
}
static void sd_fx_settle(SdPart *p) {                              /* come to rest where it struck, or the nearest free above */
    int cx = (int)(p->x * 0.5f), cy = (int)(p->y * 0.5f);
    for (int k = 0; k < 8 && !sdf_free(cx, cy); k++) cy--;
    if (!sdf_free(cx, cy)) { p->age = p->life; return; }
    p->rest = 1; p->x = (float)(cx * 2) + 1.0f; p->y = (float)(cy * 2) + 1.0f; p->vx = p->vy = 0.0f;
    sdf_mark(cx, cy, 1);
}
static int sdf_try(SdPart *p, int dx, int dy) {
    int cx = (int)(p->x * 0.5f), cy = (int)(p->y * 0.5f);
    if (!sdf_free(cx + dx, cy + dy)) return 0;
    sdf_mark(cx, cy, 0); sdf_mark(cx + dx, cy + dy, 1);
    p->x += (float)(dx * 2); p->y += (float)(dy * 2);
    return 1;
}
static void sd_fx_ca(void) {                                       /* the settled: down if they can, aside while warm */
    for (int k = 0; k < FX.n; k++) {
        SdPart *p = &FX.p[k];
        if (!p->rest) continue;
        int cx = (int)(p->x * 0.5f), cy = (int)(p->y * 0.5f);
        if (sdf_solid_px(cx * 2, cy * 2)) { p->age = p->life; continue; }   /* a gate came down on it */
        if (sdf_try(p, 0, 1)) continue;
        float flow = p->kind == PK_WAX ? p->heat : (p->age < p->life * 0.6f ? 0.45f : 0.0f);
        if (flow <= 0.15f) continue;
        if (sdf_try(p, p->dir, 1) || sdf_try(p, -p->dir, 1)) continue;
        if (sd_rndf() < flow * 0.8f && !sdf_try(p, p->dir, 0)) p->dir = (int8_t)-p->dir;
    }
}

/* ---- the censer: its swing and its chain ---- */
static void sdc_at(const SdCenser *c, float t, float *x, float *y, float *th) {
    float cyc = 2.0f * (c->hang + c->swing), u = fmodf(t, cyc), a;
    if (u < c->hang) a = c->a0;
    else if (u < c->hang + c->swing) a = c->a0 + (c->a1 - c->a0) * (0.5f - 0.5f * cosf(PI_F * (u - c->hang) / c->swing));
    else if (u < 2.0f * c->hang + c->swing) a = c->a1;
    else a = c->a1 + (c->a0 - c->a1) * (0.5f - 0.5f * cosf(PI_F * (u - 2.0f * c->hang - c->swing) / c->swing));
    *x = c->px + c->r * sinf(a); *y = c->py + c->r * cosf(a);
    if (th) *th = a;
}
static void sdc_build(SdCenser *c, const SdHaz *z, const SdThing *t) {
    c->px = (float)(t->x * SD_TILE) + 8.0f; c->py = (float)(t->y * SD_TILE) + 2.0f;
    float a[2] = { -0.7854f, 0.7854f }, r = 0.0f; int got = 0;
    for (int k = 0; k < 2 && k < z->nc; k++) {                     /* its two ends, the first two cells painted */
        int x, y; sd_hz_cell(z, k, &x, &y);
        float dx = (float)(x * SD_TILE) + 8.0f - c->px, dy = (float)(y * SD_TILE) + 8.0f - c->py;
        a[k] = atan2f(dx, fmaxf(dy, 1.0f)); r += hypotf(dx, dy); got++;
    }
    r = got ? r / (float)got : 4.0f * SD_TILE;
    if (z->len > 0.0f) r = z->len * SD_TILE;
    c->r = fmaxf(r, 3.0f * SD_TILE);                                /* three tiles at the least (the owner) */
    c->a0 = clampf(fminf(a[0], a[1]), -1.40f, 1.40f); c->a1 = clampf(fmaxf(a[0], a[1]), -1.40f, 1.40f);
    if (c->a1 - c->a0 < 0.2f) { c->a0 -= 0.4f; c->a1 += 0.4f; }
    c->hang = sdz(z->hang, 0.7f); c->swing = fmaxf(0.2f, sdz(z->swing, 1.5f));
    c->t = z->first > 0.0f ? z->first : 0.0f;
    sdc_at(c, c->t, &c->bx, &c->by, NULL);
    for (int k = 0; k <= SDC_LINKS; k++) {
        float u = (float)k / SDC_LINKS;
        c->lx[k] = c->ox[k] = c->px + (c->bx - c->px) * u; c->ly[k] = c->oy[k] = c->py + (c->by - c->py) * u;
    }
}
static void sdc_step(SdCenser *c, int live, float dt) {
    if (live) c->t += dt;
    float nx, ny; sdc_at(c, c->t, &nx, &ny, NULL);
    c->bvx = (nx - c->bx) / dt; c->bx = nx; c->by = ny;
    float rest = c->r * 1.02f / SDC_LINKS;                          /* a little slack: it bows on the swing, near straight at rest */
    for (int k = 1; k < SDC_LINKS; k++) {
        float vx = (c->lx[k] - c->ox[k]) * 0.995f, vy = (c->ly[k] - c->oy[k]) * 0.995f;
        c->ox[k] = c->lx[k]; c->oy[k] = c->ly[k];
        c->lx[k] += vx; c->ly[k] += vy + 420.0f * dt * dt;
    }
    c->lx[0] = c->px; c->ly[0] = c->py; c->lx[SDC_LINKS] = c->bx; c->ly[SDC_LINKS] = c->by;
    for (int it = 0; it < 6; it++)
        for (int k = 0; k < SDC_LINKS; k++) {
            float dx = c->lx[k + 1] - c->lx[k], dy = c->ly[k + 1] - c->ly[k], d = fmaxf(0.001f, hypotf(dx, dy)), e = (d - rest) / d * 0.5f;
            if (d < rest) continue;                                /* a chain: it pulls, it doesn't push */
            if (k > 0) { c->lx[k] += dx * e; c->ly[k] += dy * e; }
            if (k + 1 < SDC_LINKS) { c->lx[k + 1] -= dx * e; c->ly[k + 1] -= dy * e; }
        }
}

/* ---- a trap's emitter: where it is and which way ---- */
static int sd_fx_open_dir(int ax, int ay, float *dir) {             /* from a wall-set thing: toward its open side */
    static const int D[4][3] = { { 1, 0, 0 }, { -1, 0, 180 }, { 0, 1, -90 }, { 0, -1, 90 } };
    int solid = sd_solid(ax, ay);
    for (int k = 0; k < 4; k++) {
        int open = !sd_solid(ax + D[k][0], ay + D[k][1]);
        if (solid ? open : sd_solid(ax - D[k][0], ay - D[k][1])) { *dir = (float)D[k][2]; return 1; }
    }
    *dir = -90.0f;
    return 0;
}
static void sd_fx_mouth(const SdHaz *z, int h, float *x, float *y, float *dir) {
    const SdThing *t = &SD.L->th[FX.thing[h]];
    float d = -90.0f;
    *x = (float)(t->x * SD_TILE) + 8.0f; *y = (float)(t->y * SD_TILE) + 8.0f;
    if (z->kind == HZ_VOTIVE) {                                    /* the candle's lip, as far over as it is */
        float a = FX.tip[h] * (0.6f + 0.25f * (float)imin(4, FX.vn[h])) * (float)FX.vdir[h];
        *x += sinf(a) * 9.0f; *y += 2.0f - cosf(a) * 9.0f;        /* its foot on the bracket, two pixels below the tile's middle */
        d = 90.0f - a * 57.29578f;                                 /* out along the candle; it falls from there */
    } else if (z->kind == HZ_RELIQ) { *y = (float)((t->y + 1) * SD_TILE) - 10.0f; d = 0.0f; }
    else {
        sd_fx_open_dir(t->x, t->y, &d);
        if (sd_solid(t->x, t->y)) { *x += cosf(d * PI_F / 180.0f) * 9.0f; *y -= sinf(d * PI_F / 180.0f) * 9.0f; }
    }
    if (z->e_dir > -900.0f) d = z->e_dir;
    *dir = d;
}
static void sd_fx_emit(const SdHaz *z, int h, float rate, int harm, float dt) {
    float x, y, d;
    sd_fx_mouth(z, h, &x, &y, &d);
    FX.acc[h] += rate * dt;
    int kind = z->kind == HZ_VOTIVE ? PK_WAX : z->kind == HZ_BREATH ? PK_VAPOUR : z->kind == HZ_COLD ? PK_RIME : PK_HAZE;
    float speed = sd_fx_speed(z), spread = sd_fx_spread(z), life = sd_fx_life(z);
    if (kind == PK_HAZE) life = fminf(life, (sd_fx_radial(z) + 0.6f) * SD_TILE / fmaxf(8.0f, speed) * 1.6f);
    if (!harm) { life *= 0.6f; speed *= 0.4f; }
    while (FX.acc[h] >= 1.0f) {
        FX.acc[h] -= 1.0f;
        float jx = (sd_rndf() - 0.5f) * (kind == PK_WAX ? 2.0f : 6.0f), jy = (sd_rndf() - 0.5f) * (kind == PK_WAX ? 2.0f : 6.0f);
        sd_fx_spawn(kind, h, x + jx, y + jy, d, spread, speed, life, harm);
    }
}

/* ---- the particles, a step ---- */
static int sd_fx_in_cells(const SdHaz *z, int tx, int ty) {        /* a breath or a rime within what was painted for it (all, unpainted) */
    if (!z->nc) return 1;
    for (int c = 0; c < z->nc; c++) { int x, y; sd_hz_cell(z, c, &x, &y); if (abs(x - tx) <= 0 && abs(y - ty) <= 1) return 1; }
    return 0;
}
static void sd_fx_particles(float dt) {
    const SdLevel *L = SD.L;
    if ((FX.ca_t += dt) >= 1.0f / 60.0f) { FX.ca_t = 0.0f; sd_fx_ca(); }
    for (int k = 0; k < FX.n; k++) {
        SdPart *p = &FX.p[k];
        p->age += dt;
        if (p->age >= p->life) { sd_fx_kill(k--); continue; }
        if (p->kind == PK_WAX) p->heat = fmaxf(0.0f, p->heat - dt / 2.6f);
        if (p->rest) continue;
        float g = 0.0f, drag = 0.0f;
        switch (p->kind) {
        case PK_WAX: g = 520.0f; break;
        case PK_RIME: g = 260.0f; drag = 1.0f; break;
        case PK_VAPOUR: g = 14.0f; drag = 1.6f; p->vx += (sd_rndf() - 0.5f) * 90.0f * dt; p->vy += (sd_rndf() - 0.5f) * 60.0f * dt; break;
        case PK_HAZE: drag = 1.2f; break;
        case PK_SMOKE: g = -26.0f; drag = 2.2f; p->vx += (sd_rndf() - 0.5f) * 40.0f * dt; break;
        case PK_GRIT: g = 420.0f; break;
        }
        p->vy += g * dt; p->vx -= p->vx * fminf(1.0f, drag * dt); p->vy -= p->vy * fminf(1.0f, drag * 0.5f * dt);
        float nx = p->x + p->vx * dt, ny = p->y + p->vy * dt;
        int hit = sdf_solid_px((int)nx, (int)ny) || ((p->kind == PK_WAX || p->kind == PK_RIME) && sdf_occ((int)(nx * 0.5f), (int)(ny * 0.5f)));
        if (hit) {
            if (p->kind == PK_WAX || p->kind == PK_RIME) { sd_fx_settle(p); continue; }
            if (p->kind == PK_GRIT) { p->age = p->life; continue; }
            if (!sdf_solid_px((int)nx, (int)p->y)) { p->x = nx; p->vy *= -0.2f; }   /* a gas slides along what it meets */
            else if (!sdf_solid_px((int)p->x, (int)ny)) { p->y = ny; p->vx *= -0.2f; }
            else { p->vx = p->vy = 0.0f; }
            continue;
        }
        p->x = nx; p->y = ny;
        if (p->kind == PK_HAZE && p->h < L->nhz) {                 /* the reliquary's haze keeps to its reach */
            const SdThing *t = &L->th[FX.thing[p->h]];
            float r = (sd_fx_radial(&L->hz[p->h]) + 0.5f) * SD_TILE, dx = p->x - ((float)(t->x * SD_TILE) + 8.0f), dy = p->y - ((float)((t->y + 1) * SD_TILE) - 10.0f);
            if (dx * dx + dy * dy > r * r) p->age = fmaxf(p->age, p->life - 0.15f);
        }
        if ((p->kind == PK_VAPOUR || p->kind == PK_RIME) && p->h < L->nhz && !sd_fx_in_cells(&L->hz[p->h], (int)(p->x / SD_TILE), (int)(p->y / SD_TILE)))
            p->age += dt * 3.0f;                                   /* outside its painted cells: it thins out fast */
    }
    /* what harms: the tiles the dream keeps out of, and the caster in them */
    memset(FX.danger, 0, sizeof FX.danger);
    int cnt[SD_HAZ] = { 0 };
    float bx0 = SD.x - SD_HW - 1.0f, bx1 = SD.x + SD_HW + 1.0f, by0 = SD.y - sd_box_h() - 1.0f, by1 = SD.y + 1.0f;
    for (int k = 0; k < FX.n; k++) {
        const SdPart *p = &FX.p[k];
        if (!p->harm || p->h >= L->nhz || (p->kind == PK_WAX && p->heat < 0.4f) || (p->kind == PK_RIME && p->age > p->life * 0.8f)) continue;
        int tx = (int)(p->x / SD_TILE), ty = (int)(p->y / SD_TILE);
        if (tx >= 0 && ty >= 0 && tx < L->w && ty < L->h && FX.danger[ty][tx] < 255) FX.danger[ty][tx]++;
        if (p->x >= bx0 && p->x <= bx1 && p->y >= by0 && p->y <= by1) cnt[p->h]++;
    }
    for (int h = 0; h < L->nhz; h++) {
        FX.bite[h] = fmaxf(0.0f, FX.bite[h] - dt);
        const SdHaz *z = &L->hz[h];
        int need = z->kind == HZ_VOTIVE ? 1 : z->kind == HZ_BREATH ? 3 : 2;
        if (cnt[h] < need || FX.bite[h] > 0.0f || SD.dying) continue;
        float share = z->harm >= 0 ? (float)z->harm / 100.0f : z->kind == HZ_VOTIVE ? 0.12f : z->kind == HZ_RELIQ ? 0.05f : 0.06f;
        sd_hz_strike(FX.thing[h], h, share);
        if (z->kind == HZ_COLD) SD.vx *= 0.5f;                      /* the rime: it slows */
        if (z->kind == HZ_RELIQ && RUN.souls > 0) { RUN.souls = imax(0, RUN.souls - 1); }   /* the reliquary drinks a soul */
        FX.bite[h] = SDT[TU_BITE_EVERY];
    }
}

/* ---- the traps of run I, a step each (sd_traps hands them over once it has seen what sets them off) ---- */
static void sd_fx_trap(int i, int h, float dt) {
    const SdHaz *z = &SD.L->hz[h];
    int ph = SD.hz_ph[h];
    if (z->kind == HZ_CENSER) return;                              /* it swings on its own, below */
    float w = sd_hz_warn(z), on = sd_fx_on_for(z);
    if (ph) SD.hz_k[h] += dt;
    float k = SD.hz_k[h];
    if (ph == 1 && k >= w) { SD.hz_ph[h] = ph = 2; SD.hz_k[h] = k = 0.0f;
        snd_sfx_ex(z->kind == HZ_VOTIVE ? SFX_IGNITE : z->kind == HZ_RELIQ ? SFX_SHIMMER : SFX_HISS, 0.7f, 0.5f); }
    else if (ph == 2 && (k >= on || !SD.st[i])) { SD.hz_ph[h] = ph = 5; SD.hz_k[h] = k = 0.0f; }
    else if (ph == 5 && k >= (z->kind == HZ_VOTIVE ? 1.0f : 0.6f)) { SD.hz_ph[h] = ph = 0; SD.hz_k[h] = k = 0.0f; }
    if (z->kind == HZ_VOTIVE) FX.tip[h] = ph == 1 ? fminf(1.0f, k / w) : ph == 2 ? 1.0f : ph == 5 ? fmaxf(0.0f, 1.0f - k) : 0.0f;
    if (ph == 2) sd_fx_emit(z, h, sd_fx_rate(z, h), 1, dt);
    else if (ph == 1 && z->kind != HZ_VOTIVE) sd_fx_emit(z, h, sd_fx_rate(z, h) * 0.2f, 0, dt);   /* the warning: a thin breath first */
    else if (!ph && SD.st[i] && z->kind != HZ_VOTIVE) sd_fx_emit(z, h, 1.5f, 0, dt);   /* idle: a wisp now and then, that it is there */
}
static void sd_fx_censers(float dt) {
    for (int k = 0; k < FX.nc; k++) {
        SdCenser *c = &FX.c[k];
        int live = SD.st[c->i];
        float ox = c->bx;
        sdc_step(c, live, dt);
        int side = c->bx > c->px ? 1 : -1;
        if (live && side != c->side && fabsf(c->bvx) > 60.0f) snd_sfx_ex(SFX_WHOOSH, 0.35f, 0.5f);
        c->side = side;
        float sp = fabsf(c->bx - ox) / dt;
        if ((c->smoke += (live ? 3.0f + sp * 0.12f : 1.0f) * dt) >= 1.0f) {   /* a thin trail behind it, and a thread while it hangs */
            c->smoke -= 1.0f;
            sd_fx_spawn(PK_SMOKE, c->h, c->bx + (sd_rndf() - 0.5f) * 3.0f, c->by - 3.0f, 90.0f, 25.0f, 10.0f, 1.4f, 0);
        }
        if (!live || SD.dying || FX.bite[c->h] > 0.0f) continue;   /* the bowl struck: knocked the way it swings */
        float nx = clampf(c->bx, SD.x - SD_HW, SD.x + SD_HW), ny = clampf(c->by, SD.y - sd_box_h(), SD.y);
        if (hypotf(c->bx - nx, c->by - ny) > 5.0f) continue;
        const SdHaz *z = &SD.L->hz[c->h];
        sd_hz_strike(c->i, c->h, z->harm >= 0 ? (float)z->harm / 100.0f : 0.20f);
        SD.vx = (c->bvx >= 0.0f ? 1.0f : -1.0f) * 240.0f; SD.vy = -150.0f; SD.ground = 0;
        FX.bite[c->h] = 0.9f;
    }
}
static int sd_fx_censer_danger(int tx, int ty) {                   /* where the bowl will be within half a second */
    for (int k = 0; k < FX.nc; k++) {
        const SdCenser *c = &FX.c[k];
        if (!SD.st[c->i]) continue;
        for (int s = 0; s <= 5; s++) {
            float x, y; sdc_at(c, c->t + 0.1f * (float)s, &x, &y, NULL);
            if ((int)(x / SD_TILE) == tx && ((int)(y / SD_TILE) == ty || (int)((y + 6.0f) / SD_TILE) == ty)) return 1;
        }
    }
    return 0;
}
static int sd_fx_danger(int tx, int ty) {
    if (tx < 0 || ty < 0 || tx >= SD.L->w || ty >= SD.L->h) return 0;
    return FX.danger[ty][tx] >= 2 || sd_fx_censer_danger(tx, ty);
}
static void sd_fx_grit(int h, float dt) {                           /* the rockfall's warning: grit from its crack */
    const SdHaz *z = &SD.L->hz[h];
    if ((FX.acc[h] += 14.0f * (float)SD.hz_n[h] * dt) < 1.0f) return;
    FX.acc[h] -= 1.0f;
    int c = SD.hz_at[h] + (int)(sd_rndf() * (float)SD.hz_n[h]), x, y;
    if (!sd_hz_cell(z, c, &x, &y)) return;
    int top = y; while (top > 0 && !sd_solid(x, top - 1) && y - top < 10) top--;
    sd_fx_spawn(PK_GRIT, h, (float)(x * SD_TILE) + 3.0f + sd_rndf() * 10.0f, (float)(top * SD_TILE) + 1.0f, -90.0f, 5.0f, 10.0f, 2.0f, 0);
}
static void sd_fx_rubble(int h) {                                   /* it came down: rubble lies where it fell */
    const SdHaz *z = &SD.L->hz[h];
    for (int c = 0; c < SD.hz_n[h]; c++) { int idx = z->c0 + SD.hz_at[h] + c; if (idx >= 0 && idx < SD_HZC) FX.rubble[idx] = 1; }
}

/* ---- laid as the passage opens, and again after a life lost ---- */
static void sd_fx_build(void) {
    const SdLevel *L = SD.L;
    FX.n = 0; FX.nc = 0; FX.ca_t = 0.0f;
    memset(FX.occ, 0, sizeof FX.occ); memset(FX.acc, 0, sizeof FX.acc); memset(FX.bite, 0, sizeof FX.bite); memset(FX.tip, 0, sizeof FX.tip);
    memset(FX.rubble, 0, sizeof FX.rubble); memset(FX.danger, 0, sizeof FX.danger);
    for (int i = 0; i < L->nth; i++) {
        const SdThing *t = &L->th[i];
        if (t->kind != SDT_HAZ) continue;
        int h = t->a;
        const SdHaz *z = &L->hz[h];
        FX.thing[h] = i;
        if (z->kind == HZ_VOTIVE) {                                /* its way: the side more cells are painted on */
            int l = 0, r = 0;
            for (int c = 0; c < z->nc; c++) { int x, y; sd_hz_cell(z, c, &x, &y); if (x < t->x) l++; else if (x > t->x) r++; }
            FX.vdir[h] = r >= l ? 1 : -1; FX.vn[h] = imax(1, imax(l, r));
        }
        if (z->kind == HZ_CENSER && FX.nc < SD_CENSERS) { SdCenser *c = &FX.c[FX.nc++]; memset(c, 0, sizeof *c); c->h = h; c->i = i; sdc_build(c, z, t); }
    }
}
static void sd_fx_step(float dt) {
    sd_fx_censers(dt); sd_fx_particles(dt);
#ifdef CRAWL_DEBUG
    static float tr;                                               /* CRAWL_SIDETRACE: how many, by kind, twice a second */
    if (getenv("CRAWL_SIDETRACE") && (tr += dt) >= 0.5f) {
        int n[NPK] = { 0 }, rest = 0;
        for (int k = 0; k < FX.n; k++) { n[FX.p[k].kind]++; rest += FX.p[k].rest; }
        tr = 0.0f;
        fprintf(stderr, "SIDE fx %d: wax %d vapour %d rime %d haze %d smoke %d grit %d, %d settled t=%.2f\n", FX.n, n[0], n[1], n[2], n[3], n[4], n[5], rest, SD.t);
    }
#endif
}
