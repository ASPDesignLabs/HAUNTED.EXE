/* A part of scene_crawl.c (docs/PLAN-tidy.md): it is #included there in its place and never compiled on its own.
 * Side passages (docs/PLAN-side.md, 2e): what pours, drifts and creeps in a passage, as particles from emitters the World Editor
 * tunes; the censer on its chain; and run I's traps with their signs while idle: the rockfall's crack and the rubble it leaves, the
 * votive sconce that tips and pours its wax, the embalmers' slab and its breath, the cold vault's crack and its rime, the hungry
 * reliquary's haze. Wax and rime settle as a falling sand does, on a grid of two pixels: down, then aside while they are warm.
 * Run II's (the owner, 26 November): the grate's bile, the sluice's surge, the galvanic pool, silt gas that a fire spell sets alight,
 * the weeping saint's tears, and the snare's web. */

enum { PK_WAX, PK_VAPOUR, PK_RIME, PK_HAZE, PK_SMOKE, PK_GRIT, PK_BILE, PK_WATER, PK_GAS, PK_FLAME, PK_TEAR, PK_SPARK, PK_STRAND, NPK };
#define PK_FLUID(k) ((k) == PK_WAX || (k) == PK_RIME || (k) == PK_BILE || (k) == PK_WATER || (k) == PK_TEAR)   /* those that settle and run */
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
    uint8_t danger[SD_H][SD_W];                                    /* for the dream: what burns in each tile, kept out of */
    uint8_t harm[SD_H][SD_W];                                      /* and anything that harms, not stood in to fight */
    SdCenser c[SD_CENSERS]; int nc;
    float web[SD_HAZ], bolt[SD_HAZ];                               /* a snare torn, seconds till it is whole; the galvanic bolt, still shown */
    int sdir[SD_HAZ];                                              /* a sluice's canal: its way, -1 or 1 */
    float pw[NPW], pw_full[NPW];                                   /* the powerups (26 November): seconds left of each, and from */
} FX;
/* ---- the powerups: quickened hands, might, a ward ---- */
static float sd_fx_rapid(void) { return FX.pw[PW_RAPID] > 0.0f ? 0.5f : 1.0f; }
static float sd_fx_might(void) { return FX.pw[PW_MIGHT] > 0.0f ? 2.0f : 1.0f; }
static int sd_fx_ward(void) { return FX.pw[PW_WARD] > 0.0f; }
static void sd_fx_power(int kind, float secs) {
    static const char *const SAY[NPW] = { "QUICKENED HANDS", "MIGHT: HARM DOUBLED", "A WARD: NOTHING TOUCHES YOU" };
    if (kind < 0 || kind >= NPW) return;
    FX.pw[kind] = FX.pw_full[kind] = secs;
    say("%s FOR %d SECONDS", SAY[kind], (int)(secs + 0.5f));
    snd_sfx_ex(SFX_SHIMMER, 0.9f, 0.5f); snd_tone(1, kind == PW_RAPID ? 76 : kind == PW_MIGHT ? 69 : 81, 0.4f, 0.5f);
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT side_power %d for %.1f t=%.2f\n", kind, secs, SD.t);
#endif
}
static int sd_hz_cell(const SdHaz *z, int c, int *x, int *y);     /* side_works.c */
static void sd_hz_strike(int i, int h, float share);
static float sd_hz_warn(const SdHaz *z);
static int sd_fx_kind(int k) { return k == HZ_VOTIVE || k == HZ_CENSER || k == HZ_BREATH || k == HZ_COLD || k == HZ_RELIQ
                                     || k == HZ_BILE || k == HZ_SLUICE || k == HZ_GALV || k == HZ_SILT || k == HZ_SAINT || k == HZ_SNARE; }

/* ---- each kind's own emitter, where the editor leaves it alone ---- */
static float sdz(float v, float d) { return v >= 0.0f ? v : d; }
static float sd_fx_rate(const SdHaz *z, int h) {
    switch (z->kind) {
    case HZ_VOTIVE: return sdz(z->e_rate, fminf(90.0f, 18.0f * (float)imax(1, FX.vn[h])));   /* more painted, a heavier pour */
    case HZ_BREATH: return sdz(z->e_rate, 26.0f);
    case HZ_COLD: return sdz(z->e_rate, 20.0f);
    case HZ_RELIQ: return sdz(z->e_rate, 34.0f);
    case HZ_BILE: return sdz(z->e_rate, 22.0f);
    case HZ_SLUICE: return sdz(z->e_rate, 60.0f);
    case HZ_SILT: return sdz(z->e_rate, 24.0f);
    case HZ_SAINT: return sdz(z->e_rate, 14.0f);
    default: return sdz(z->e_rate, 0.0f);
    }
}
static float sd_fx_def(int k, int what) {                          /* each kind's own: speed (tiles a second), spread (degrees), life (s) */
    static const struct { int k; float speed, spread, life; } D[] = {
        { HZ_VOTIVE, 1.3f, 14.0f, 7.0f }, { HZ_BREATH, 0.7f, 55.0f, 3.5f }, { HZ_COLD, 0.9f, 35.0f, 4.5f }, { HZ_RELIQ, 1.6f, 180.0f, 2.0f },
        { HZ_BILE, 0.6f, 20.0f, 4.5f }, { HZ_SLUICE, 3.0f, 10.0f, 3.0f }, { HZ_SILT, 0.5f, 20.0f, 4.0f }, { HZ_SAINT, 0.3f, 4.0f, 6.0f } };
    for (int i = 0; i < (int)(sizeof D / sizeof D[0]); i++) if (D[i].k == k) return what == 0 ? D[i].speed : what == 1 ? D[i].spread : D[i].life;
    return what == 0 ? 1.0f : what == 1 ? 30.0f : 3.0f;
}
static float sd_fx_speed(const SdHaz *z) { return sdz(z->e_speed, sd_fx_def(z->kind, 0)) * SD_TILE; }
static float sd_fx_spread(const SdHaz *z) { return sdz(z->e_spread, sd_fx_def(z->kind, 1)); }
static float sd_fx_life(const SdHaz *z) { return sdz(z->e_life, sd_fx_def(z->kind, 2)); }
static float sd_fx_radial(const SdHaz *z) { return sdz(z->radial, 1.0f); }
static float sd_fx_on_for(const SdHaz *z) { return z->kind == HZ_VOTIVE ? fminf(z->hold, 2.2f) : z->kind == HZ_SLUICE ? fminf(z->hold, 2.0f) : z->kind == HZ_GALV ? 0.6f : fminf(z->hold, 4.0f); }

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
        float flow = p->kind == PK_WAX ? p->heat : p->kind == PK_WATER ? 0.95f : p->kind == PK_BILE ? (p->age < p->life * 0.7f ? 0.6f : 0.1f)
                   : p->kind == PK_TEAR ? 0.7f : (p->age < p->life * 0.6f ? 0.45f : 0.0f);
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
    else if (z->kind == HZ_SILT && z->nc) {                         /* out of the silt: a painted cell's floor, any */
        int cx = 0, cy = 0; sd_hz_cell(z, (int)(sd_rndf() * (float)z->nc) % z->nc, &cx, &cy);
        *x = (float)(cx * SD_TILE) + 2.0f + sd_rndf() * 12.0f; *y = (float)((cy + 1) * SD_TILE) - 2.0f; d = 90.0f;
    } else {
        sd_fx_open_dir(t->x, t->y, &d);
        if (sd_solid(t->x, t->y)) { *x += cosf(d * PI_F / 180.0f) * 9.0f; *y -= sinf(d * PI_F / 180.0f) * 9.0f; }
        if (z->kind == HZ_SLUICE) d = FX.sdir[h] > 0 ? -8.0f : 188.0f;   /* the surge: down its canal */
        if (z->kind == HZ_BILE && !sd_solid(t->x, t->y) && d > 45.0f && d < 135.0f) { *y = (float)((t->y + 1) * SD_TILE) - 3.0f; }   /* a grate in the floor: it oozes up */
        if (z->kind == HZ_SAINT) { *y += 1.0f; d = -90.0f; }       /* her tears, from under the face, straight down */
    }
    if (z->e_dir > -900.0f) d = z->e_dir;
    *dir = d;
}
static void sd_fx_emit(const SdHaz *z, int h, float rate, int harm, float dt) {
    float x, y, d;
    sd_fx_mouth(z, h, &x, &y, &d);
    FX.acc[h] += rate * dt;
    int kind = z->kind == HZ_VOTIVE ? PK_WAX : z->kind == HZ_BREATH ? PK_VAPOUR : z->kind == HZ_COLD ? PK_RIME : z->kind == HZ_BILE ? PK_BILE
             : z->kind == HZ_SLUICE ? PK_WATER : z->kind == HZ_SILT ? PK_GAS : z->kind == HZ_SAINT ? PK_TEAR : PK_HAZE;
    float speed = sd_fx_speed(z), spread = sd_fx_spread(z), life = sd_fx_life(z);
    if (kind == PK_HAZE) life = fminf(life, (sd_fx_radial(z) + 0.6f) * SD_TILE / fmaxf(8.0f, speed) * 1.6f);
    if (!harm) { life *= 0.6f; speed *= 0.4f; }
    if (kind == PK_BILE && d > 45.0f && d < 135.0f) speed *= 0.35f;   /* up out of a floor grate: a slow ooze that puddles */
    while (FX.acc[h] >= 1.0f) {
        FX.acc[h] -= 1.0f;
        float jit = kind == PK_WAX || kind == PK_TEAR ? 2.0f : 6.0f, jx = (sd_rndf() - 0.5f) * jit, jy = (sd_rndf() - 0.5f) * jit;
        if (kind == PK_GAS) sd_fx_mouth(z, h, &x, &y, &d);           /* the silt bubbles all over its cells */
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
        case PK_BILE: g = 480.0f; break;
        case PK_WATER: g = 520.0f; break;
        case PK_TEAR: g = 400.0f; break;
        case PK_GAS: g = p->age < 1.2f ? -34.0f : -4.0f; drag = 2.2f;   /* it rises a little, then hangs */ p->vx += (sd_rndf() - 0.5f) * 50.0f * dt; p->vy += (sd_rndf() - 0.5f) * 30.0f * dt; break;
        case PK_FLAME: g = -70.0f; drag = 2.0f; break;
        case PK_SPARK: g = 200.0f; break;
        case PK_STRAND: g = 120.0f; drag = 1.0f; break;
        }
        p->vy += g * dt; p->vx -= p->vx * fminf(1.0f, drag * dt); p->vy -= p->vy * fminf(1.0f, drag * 0.5f * dt);
        float nx = p->x + p->vx * dt, ny = p->y + p->vy * dt;
        int hit = sdf_solid_px((int)nx, (int)ny) || (PK_FLUID(p->kind) && sdf_occ((int)(nx * 0.5f), (int)(ny * 0.5f)));
        if (hit) {
            if (PK_FLUID(p->kind)) { sd_fx_settle(p); continue; }
            if (p->kind == PK_GRIT || p->kind == PK_SPARK || p->kind == PK_STRAND) { p->age = p->life; continue; }
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
        if ((p->kind == PK_VAPOUR || p->kind == PK_RIME || p->kind == PK_BILE || p->kind == PK_WATER || p->kind == PK_GAS) && p->h < L->nhz
            && !sd_fx_in_cells(&L->hz[p->h], (int)(p->x / SD_TILE), (int)(p->y / SD_TILE)))
            p->age += dt * (p->kind == PK_GAS ? 6.0f : 3.0f);          /* outside its painted cells: it thins out fast */
    }
    for (int k = 0; k < FX.n; k++)                                 /* a flame newly lit lights the gas about it */
        if (FX.p[k].kind == PK_FLAME && FX.p[k].age < 0.12f) {
            float fx = FX.p[k].x, fy = FX.p[k].y;
            for (int j = 0; j < FX.n; j++) if (FX.p[j].kind == PK_GAS && fabsf(FX.p[j].x - fx) < 9.0f && fabsf(FX.p[j].y - fy) < 9.0f) {
                FX.p[j].kind = PK_FLAME; FX.p[j].age = 0.0f; FX.p[j].life = 0.5f + 0.3f * sd_rndf(); FX.p[j].harm = 1; FX.p[j].vy = -30.0f; }
        }
    /* what harms: the tiles the dream keeps out of, and the caster in them */
    memset(FX.danger, 0, sizeof FX.danger); memset(FX.harm, 0, sizeof FX.harm);
    int cnt[SD_HAZ] = { 0 }, fire[SD_HAZ] = { 0 }, wdir[SD_HAZ] = { 0 };
    float bx0 = SD.x - SD_HW - 1.0f, bx1 = SD.x + SD_HW + 1.0f, by0 = SD.y - sd_box_h() - 1.0f, by1 = SD.y + 1.0f;
    for (int k = 0; k < FX.n; k++) {
        const SdPart *p = &FX.p[k];
        if (!p->harm || p->h >= L->nhz || (p->kind == PK_WAX && p->heat < 0.4f) || (p->kind == PK_RIME && p->age > p->life * 0.8f)
            || (p->kind == PK_BILE && p->age > p->life * 0.85f) || p->kind == PK_SMOKE || p->kind == PK_GRIT || p->kind == PK_SPARK || p->kind == PK_STRAND) continue;
        int tx = (int)(p->x / SD_TILE), ty = (int)(p->y / SD_TILE);
        int dw = p->kind == PK_WAX || p->kind == PK_FLAME ? 2 : 0;   /* the dream keeps out of what burns; the rest, a few bites, it walks through */
        if (tx >= 0 && ty >= 0 && tx < L->w && ty < L->h) { FX.danger[ty][tx] = (uint8_t)imin(255, FX.danger[ty][tx] + dw); FX.harm[ty][tx] = (uint8_t)imin(255, FX.harm[ty][tx] + 1); }
        if (p->x >= bx0 && p->x <= bx1 && p->y >= by0 && p->y <= by1) { if (p->kind == PK_FLAME) fire[p->h]++; else cnt[p->h]++; wdir[p->h] += p->dir; }
    }
    for (int h = 0; h < L->nhz; h++)                                /* the galvanic pool, warning or loosed: its water the dream keeps out of */
        if (L->hz[h].kind == HZ_GALV && (SD.hz_ph[h] == 1 || SD.hz_ph[h] == 2))
            for (int c = 0; c < L->hz[h].nc; c++) { int x, y; sd_hz_cell(&L->hz[h], c, &x, &y); if (x >= 0 && y >= 0 && x < L->w && y < L->h) FX.danger[y][x] = 9; }
    for (int h = 0; h < L->nhz; h++) {
        FX.bite[h] = fmaxf(0.0f, FX.bite[h] - dt);
        const SdHaz *z = &L->hz[h];
        int need = z->kind == HZ_VOTIVE ? 1 : z->kind == HZ_BREATH || z->kind == HZ_SILT ? 3 : 2;
        if (z->kind == HZ_SLUICE && cnt[h] >= 2 && !SD.dying) {     /* the surge: it carries the caster along its canal */
            int d = FX.sdir[h] ? FX.sdir[h] : (wdir[h] >= 0 ? 1 : -1);
            sd_move_x((float)d * 150.0f * dt); SD.vx = (float)d * fmaxf(fabsf(SD.vx), 60.0f);
        }
        if (fire[h] && FX.bite[h] <= 0.0f && !SD.dying) {         /* the silt's gas alight */
            G.blame = (Blame){ BL_HAZARD, z->kind, 0 };
            sd_hurt((z->harm >= 0 ? (float)z->harm / 100.0f : 0.15f) * (float)G.max_hp, EL_FIRE, SD.x - (float)SD.face);
            FX.bite[h] = SDT[TU_BITE_EVERY];
            continue;
        }
        if (cnt[h] < need || FX.bite[h] > 0.0f || SD.dying) continue;
        float share = z->harm >= 0 ? (float)z->harm / 100.0f : z->kind == HZ_VOTIVE ? 0.12f : z->kind == HZ_RELIQ || z->kind == HZ_SAINT ? 0.05f : 0.06f;
        sd_hz_strike(FX.thing[h], h, share);
        if (z->kind == HZ_COLD || z->kind == HZ_SLUICE) SD.vx *= 0.5f;   /* the rime, the surge's cold: they slow */
        if (z->kind == HZ_BILE) SD.vx *= 0.8f;                     /* the bile: a fifth slower */
        if ((z->kind == HZ_RELIQ || z->kind == HZ_SAINT) && RUN.souls > 0) { RUN.souls = imax(0, RUN.souls - 1); }   /* it drinks a soul */
        FX.bite[h] = SDT[TU_BITE_EVERY];
    }
}

/* ---- the traps of run I, a step each (sd_traps hands them over once it has seen what sets them off) ---- */
static void sd_fx_trap(int i, int h, float dt) {
    const SdHaz *z = &SD.L->hz[h];
    int ph = SD.hz_ph[h];
    if (z->kind == HZ_CENSER || z->kind == HZ_SNARE) return;       /* each on its own, below */
    float w = sd_hz_warn(z), on = sd_fx_on_for(z);
    if (ph) SD.hz_k[h] += dt;
    float k = SD.hz_k[h];
    if (ph == 1 && k >= w) { SD.hz_ph[h] = ph = 2; SD.hz_k[h] = k = 0.0f;
        snd_sfx_ex(z->kind == HZ_VOTIVE ? SFX_IGNITE : z->kind == HZ_RELIQ || z->kind == HZ_SAINT ? SFX_SHIMMER : z->kind == HZ_GALV ? SFX_ZAP
                   : z->kind == HZ_SLUICE ? SFX_SPLASH : z->kind == HZ_BILE ? SFX_DRIP : SFX_HISS, 0.7f, 0.5f);
        if (z->kind == HZ_GALV) {                                   /* the bolt: all in its water are struck, once */
            FX.bolt[h] = 0.3f;
            float bx0 = SD.x - SD_HW, bx1 = SD.x + SD_HW, by0 = SD.y - sd_box_h(), by1 = SD.y + 2.0f;
            for (int c = 0; c < z->nc; c++) {
                int x, y; sd_hz_cell(z, c, &x, &y);
                if (bx1 > (float)(x * SD_TILE) && bx0 < (float)((x + 1) * SD_TILE) && by1 > (float)(y * SD_TILE) + 8.0f && by0 < (float)((y + 1) * SD_TILE)) {
                    sd_hz_strike(i, h, z->harm >= 0 ? (float)z->harm / 100.0f : 0.25f); break; }
            }
        }
    }
    else if (ph == 2 && (k >= on || !SD.st[i])) { SD.hz_ph[h] = ph = 5; SD.hz_k[h] = k = 0.0f; }
    else if (ph == 5 && k >= (z->kind == HZ_VOTIVE ? 1.0f : 0.6f)) { SD.hz_ph[h] = ph = 0; SD.hz_k[h] = k = 0.0f; }
    if (z->kind == HZ_VOTIVE) FX.tip[h] = ph == 1 ? fminf(1.0f, k / w) : ph == 2 ? 1.0f : ph == 5 ? fmaxf(0.0f, 1.0f - k) : 0.0f;
    FX.bolt[h] = fmaxf(0.0f, FX.bolt[h] - dt);
    if (z->kind == HZ_GALV) {                                      /* the water crackles in its warning, sparks in its turn */
        if ((ph == 1 || ph == 2) && z->nc && (FX.acc[h] += (ph == 1 ? 18.0f : 60.0f) * dt) >= 1.0f) {
            FX.acc[h] -= 1.0f;
            int x = 0, y = 0; sd_hz_cell(z, (int)(sd_rndf() * (float)z->nc) % z->nc, &x, &y);
            sd_fx_spawn(PK_SPARK, h, (float)(x * SD_TILE) + sd_rndf() * 16.0f, (float)(y * SD_TILE) + 9.0f, 90.0f, 50.0f, 40.0f, 0.35f, 0);
        }
        return;
    }
    if (ph == 2) sd_fx_emit(z, h, sd_fx_rate(z, h), 1, dt);
    else if (ph == 1 && z->kind != HZ_VOTIVE) sd_fx_emit(z, h, sd_fx_rate(z, h) * 0.2f, 0, dt);   /* the warning: a thin breath first */
    else if (!ph && SD.st[i] && z->kind != HZ_VOTIVE) sd_fx_emit(z, h, 1.5f, 0, dt);   /* idle: a wisp, a drip, a bubble now and then, that it is there */
}
static void sd_fx_snare(int i, int h, float dt) {                  /* the web: walked into, it holds, tears, and calls the dead near */
    const SdHaz *z = &SD.L->hz[h];
    if (FX.web[h] > 0.0f) { FX.web[h] = fmaxf(0.0f, FX.web[h] - dt); return; }   /* torn: it is spun again */
    if (SD.dying) return;
    float bx0 = SD.x - SD_HW, bx1 = SD.x + SD_HW, by0 = SD.y - sd_box_h(), by1 = SD.y;
    for (int c = 0; c < z->nc; c++) {
        int x, y; sd_hz_cell(z, c, &x, &y);
        if (!(bx1 > (float)(x * SD_TILE) && bx0 < (float)((x + 1) * SD_TILE) && by1 > (float)(y * SD_TILE) && by0 < (float)((y + 1) * SD_TILE))) continue;
        if (sd_rndf() * 100.0f < (z->chance >= 0 ? (float)z->chance : 100.0f)) SD.hold_t = 1.5f;
        SD.vx = 0.0f;
        FX.web[h] = sdz(z->hold, 6.0f);                            /* torn for STAYS seconds (6) */
        for (int k = 0; k < 14; k++) sd_fx_spawn(PK_STRAND, h, (float)(x * SD_TILE) + sd_rndf() * 16.0f, (float)(y * SD_TILE) + sd_rndf() * 16.0f, 90.0f, 180.0f, 20.0f, 1.2f, 0);
        for (int m = 0; m < SD_MOBS; m++) {                        /* the dead near hear it */
            SdMob *mb = &SD.mob[m];
            if (mb->alive && fabsf(mb->x - SD.x) < 8.0f * SD_TILE && fabsf(mb->y - SD.y) < 4.0f * SD_TILE) { mb->home_x = SD.x; mb->home_y = SD.y; }
        }
        snd_sfx_ex(SFX_THRUM, 0.7f, 0.5f);
        (void)i;
#ifdef CRAWL_DEBUG
        fprintf(stderr, "EVENT side_trap HZ_SNARE holds t=%.2f\n", SD.t);
#endif
        return;
    }
}
static void sd_fx_ignite(float x, float y) {                       /* a fire spell's way: silt gas it passes through catches */
    if (!SD.on) return;
    for (int k = 0; k < FX.n; k++) {
        SdPart *p = &FX.p[k];
        if (p->kind != PK_GAS || fabsf(p->x - x) > 10.0f || fabsf(p->y - y) > 10.0f) continue;
        p->kind = PK_FLAME; p->age = 0.0f; p->life = 0.5f + 0.3f * sd_rndf(); p->harm = 1; p->vy = -30.0f;
        snd_sfx_ex(SFX_IGNITE, 0.8f, 0.5f);
    }
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
static int sd_fx_harm(int tx, int ty) {                            /* anything harming here, light or not */
    if (tx < 0 || ty < 0 || tx >= SD.L->w || ty >= SD.L->h) return 0;
    return FX.harm[ty][tx] > 0 || FX.danger[ty][tx] >= 2 || sd_fx_censer_danger(tx, ty);
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
    memset(FX.rubble, 0, sizeof FX.rubble); memset(FX.danger, 0, sizeof FX.danger); memset(FX.web, 0, sizeof FX.web); memset(FX.bolt, 0, sizeof FX.bolt); memset(FX.pw, 0, sizeof FX.pw);
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
        if (z->kind == HZ_SLUICE) {                                /* its canal's way: toward the cells painted for it */
            int l = 0, r = 0;
            for (int c = 0; c < z->nc; c++) { int x, y; sd_hz_cell(z, c, &x, &y); if (x < t->x) l++; else if (x > t->x) r++; }
            FX.sdir[h] = r >= l ? 1 : -1;
        }
        if (z->kind == HZ_CENSER && FX.nc < SD_CENSERS) { SdCenser *c = &FX.c[FX.nc++]; memset(c, 0, sizeof *c); c->h = h; c->i = i; sdc_build(c, z, t); }
    }
}
static void sd_fx_step(float dt) {
    sd_fx_censers(dt); sd_fx_particles(dt);
    for (int k = 0; k < NPW; k++) if (FX.pw[k] > 0.0f && (FX.pw[k] -= dt) <= 0.0f) { FX.pw[k] = 0.0f; snd_sfx_ex(SFX_KEY, 0.5f, 0.5f); }
#ifdef CRAWL_DEBUG
    static float tr;                                               /* CRAWL_SIDETRACE: how many, by kind, twice a second */
    if (getenv("CRAWL_SIDETRACE") && (tr += dt) >= 0.5f) {
        int n[NPK] = { 0 }, rest = 0;
        for (int k = 0; k < FX.n; k++) { n[FX.p[k].kind]++; rest += FX.p[k].rest; }
        tr = 0.0f;
        fprintf(stderr, "SIDE fx %d: wax %d vapour %d rime %d haze %d smoke %d grit %d bile %d water %d gas %d flame %d tear %d spark %d strand %d, %d settled t=%.2f\n",
                FX.n, n[0], n[1], n[2], n[3], n[4], n[5], n[6], n[7], n[8], n[9], n[10], n[11], n[12], rest, SD.t);
    }
#endif
}
