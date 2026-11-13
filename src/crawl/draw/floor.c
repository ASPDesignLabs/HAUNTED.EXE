/* A part of scene_crawl.c (docs/PLAN-tidy.md): it is #included there in its place and never compiled on its own.
 * Rendering begins: shading and fog, the floor and ceiling (render_floor), secrets' cracks, the floor's marks (hazards, omens, wards, ice: fm_pixel). */

/* ================================================================== */
/* ================================================================== */
/* Rendering                                                           */
/* ================================================================== */

static inline uint8_t shade(uint8_t c, float light, int x, int y) {
    if (IS_FULLBRIGHT(c)) return c;
    int lv = (int)(light * LIGHT_UNIT + BAYER[y & 3][x & 3] * (1.0f / 16.0f));
    return crawl_cmap[clampi(lv, 0, NLIGHT - 1)][c];
}

/* Fog, done the VGA way: dithered pixels of grey haze, mixed from the bank at the spot being drawn
 * and the one the caster stands in, growing with distance, lit by whatever lights the spot. */
static float g_camfog;
static inline float fog_at(float x, float y) {
    float fx = x - 0.5f, fy = y - 0.5f;
    int ix = (int)floorf(fx), iy = (int)floorf(fy);
    if (ix < 0 || iy < 0 || ix >= MAPN - 1 || iy >= MAPN - 1) return 0.0f;
    float ax = fx - ix, ay = fy - iy;
    return lerpf(lerpf(G.fog[iy][ix], G.fog[iy][ix + 1], ax), lerpf(G.fog[iy + 1][ix], G.fog[iy + 1][ix + 1], ax), ay);
}
static inline float fog_amount(float x, float y, float depth_k) {   /* depth_k: 1 - exp(-distance * 0.28) */
#ifdef CRAWL_DEBUG
    if (g_stage_fog) return fminf(depth_k * 1.4f, 0.7f);
#endif
    return fminf((fog_at(x, y) * 0.6f + g_camfog * 0.4f) * depth_k, 0.7f);
}
static int fog_tint;                                               /* a trial's room, turned: its fog's colour (0 none), set each frame */
static inline uint8_t fogged(uint8_t c, float fog, float light, int x, int y) {
    if (fog * 16.0f > BAYER[y & 3][x & 3] + 0.5f)                  /* a corrupted level's fog has a sickly cast; a trial's, its own */
        return (uint8_t)((fog_tint && ((x ^ y) & 1) ? fog_tint : G.corrupt && ((x ^ y) & 1) ? R_MOSS : R_CEIL) + clampi((int)(3.0f + light * 8.0f), 3, 12));
    return c;
}

static int fm_any;                                                 /* any hazard marks on the floor this frame */
static int fm_at(int cx, int cy, float wx, float wy, float fx, float fy, int x, int y, uint8_t *base, float *lmul);
_Static_assert(TS == 64, "render_floor finds a texel's cell by shifting six bits");
static void render_floor(Frame *f, float dx, float dy, float plx, float ply, float horizon) {
    float lx = dx - plx, ly = dy - ply, rx = dx + plx, ry = dy + ply;
    int themed = G.ptheme != PT_NONE, sev = G.ptheme == PT_SEVERANCE;   /* carpet underfoot, tiles or panels above */
    static uint8_t lamp_lit[MAXLIGHTS + 1];
    if (themed) for (int l = 0; l < G.nlights; l++) lamp_lit[l + 1] = torch_flicker(&G.lights[l]) > 0.5f;
    for (int y = 0; y < H; y++) {
        uint8_t *row = &f->pix[y * W];
        float p = y - horizon + 0.5f;
        int floor_row = p > 0.0f;
        float dist = floor_row ? G.camz * FOCAL / p : (1.0f - G.camz) * FOCAL / -p;
        if (dist > 30.0f) { memset(row, 0, W); continue; }
        float sx = dist * (rx - lx) / W, sy = dist * (ry - ly) / W;
        float wx = G.px + dist * lx, wy = G.py + dist * ly;
        /* the same in texels, 16.16 fixed point, so a pixel's cell and texel come by shifts (TS is 64) */
        int32_t U = (int32_t)floor((double)wx * (TS << 16)), V = (int32_t)floor((double)wy * (TS << 16));
        int32_t dU = (int32_t)lrint((double)sx * (TS << 16)), dV = (int32_t)lrint((double)sy * (TS << 16));
        float a = atten(dist), dk = 1.0f - expf(-dist * 0.28f);
        int grout = floor_row && dist < 6.0f, lcx = -9999, lcy = -9999;
        const Cell *lc = cell(-1, -1);                              /* the last cell looked up along the row */
        for (int x = 0; x < W; x++, wx += sx, wy += sy, U += dU, V += dV) {
            uint8_t base;
            float lmul = 1.0f;
            int cx = U >> 22, cy = V >> 22, tx = (U >> 16) & (TS - 1), ty = (V >> 16) & (TS - 1);
            if (floor_row) {
                float fx = wx - cx, fy = wy - cy;
                if (themed) {                                      /* each cell's carpet turned about, so it doesn't repeat */
                    uint32_t h = hash2(cx, cy);
                    if (h & 1) tx = TS - 1 - tx;
                    if (h & 2) ty = TS - 1 - ty;
                    if (h & 4) { int q = tx; tx = ty; ty = q; }
                    base = crawl_tex[sev ? T_SCARPET : T_CARPET][tx * TS + ty];
                } else if (G.faced && (cx != lcx || cy != lcy ? (lcx = cx, lcy = cy, lc = cell(cx, cy))->flo : lc->flo)) {   /* a drawn run's floor, each cell turned about */
                    int h = lc->fflip >> 5;
                    if (h & 1) tx = TS - 1 - tx;
                    if (h & 2) ty = TS - 1 - ty;
                    if (h & 4) { int q = tx; tx = ty; ty = q; }
                    base = crawl_tex[lc->flo][tx * TS + ty];
                } else {
                    base = (uint8_t)(R_FLOOR + 8 + ((cx * 7 + cy * 13) & 1));
                    if (grout && (fx < 0.035f || fy < 0.035f)) base = R_FLOOR + 5;
                }
                if (fm_any) {                                      /* a hazard's mark */
                    int o = fm_at(cx, cy, wx, wy, fx, fy, x, y, &base, &lmul);
                    if (o >= 0) { row[x] = (uint8_t)o; continue; }
                }
                if (G.nova_t > 0.0f) {                             /* the frost nova racing out over the floor */
                    float r = 2.2f * sqrtf(G.nova_t / 0.6f), d = hypotf(wx - G.px, wy - G.py);
                    if (d < r && (fabsf(d - r) < 0.22f || BAYER[y & 3][x & 3] < (int)(10.0f * (1.0f - G.nova_t / 0.6f)))) {
                        row[x] = (uint8_t)(R_GLOW + clampi((int)(15.0f - fabsf(d - r) * 30.0f), 5, 15));
                        continue;
                    }
                }
            } else if (themed) {
                const Cell *c = cell(cx, cy);
                int t = sev ? (c->lamp && c->lamp != 255 && lamp_lit[c->lamp] ? T_SCEILLAMP : T_SCEIL)
                      : c->lamp == 255 ? T_CEILDEAD : c->lamp ? (lamp_lit[c->lamp] ? T_CEILLAMP : T_CEILDEAD)
                      : hash2(cx, cy) % 14 == 0 ? T_CEILSTAIN : T_CEILTILE;
                base = crawl_tex[t][tx * TS + ty];
            } else if (G.faced) {
                if (cx != lcx || cy != lcy) { lcx = cx; lcy = cy; lc = cell(cx, cy); }
                if (lc->cei) {
                    int h = lc->fflip >> 5;
                    if (h & 1) tx = TS - 1 - tx;
                    if (h & 2) ty = TS - 1 - ty;
                    base = crawl_tex[lc->cei][tx * TS + ty];
                } else base = R_CEIL + 8;
            } else base = R_CEIL + 8;
            float L = lm_at(wx, wy) * a * lmul;
            row[x] = fogged(shade(base, L, x, y), fog_amount(wx, wy, dk * (floor_row ? 1.15f : 0.8f)), L, x, y);
        }
    }
}

/* cracks for a secret wall: dark lines from a point near the middle, a lit edge along each */
static uint8_t crack_mask[TS * TS];                                /* column-major, like the textures: 1 crack, 2 its lit edge */
static void build_cracks(void) {
    uint32_t r = 0x5ec2e7u;
    for (int k = 0; k < 6; k++) {
        float x = 30 + rng_f(&r) * 6, y = 26 + rng_f(&r) * 8, a = TAU_F * k / 6.0f + rng_f(&r) * 0.6f;
        int len = 12 + (int)(rng_f(&r) * 16), branch = 5 + (int)(rng_f(&r) * 8);
        for (int i = 0; i < len; i++) {
            a += (rng_f(&r) - 0.5f) * 0.7f;
            x += cosf(a); y += sinf(a);
            int ix = (int)x, iy = (int)y;
            if (ix < 1 || iy < 1 || ix >= TS - 1 || iy >= TS - 1) break;
            crack_mask[ix * TS + iy] = 1;
            if (!crack_mask[(ix + 1) * TS + iy + 1]) crack_mask[(ix + 1) * TS + iy + 1] = 2;
            if (i == branch) {                                     /* a fork */
                float bx = x, by = y, ba = a + (rng_f(&r) < 0.5f ? 0.9f : -0.9f);
                for (int j = 0; j < 7; j++) {
                    bx += cosf(ba); by += sinf(ba); ba += (rng_f(&r) - 0.5f) * 0.6f;
                    int jx = (int)bx, jy = (int)by;
                    if (jx >= 1 && jy >= 1 && jx < TS - 1 && jy < TS - 1) crack_mask[jx * TS + jy] = 1;
                }
            }
        }
    }
}

/* where a ray meets the sliding block: distance, across its face, and which way the face looks */
/* ---- the hazards, drawn: their marks on the floor, made each frame from G.haz ---- */
enum { FM_NONE, FM_SHADOW, FM_CRACK, FM_FIRE, FM_EMBER, FM_SHEEN, FM_BILE, FM_VRING, FM_VOID, FM_ARC, FM_GRASP,
       FM_OMEN, FM_OMEN_CELL, FM_OMEN_ARC,                      /* Premonition's zones: round, a cell's worth, a blade's fan */
       FM_VAPOUR, FM_WAX, FM_RIME, FM_SURGE, FM_SPARK, FM_BUBBLE, FM_POOL, FM_DUST, FM_SHIMMER, FM_SRING, FM_ASH, FM_STAR,
       FM_CHALK, FM_HAZE,                                       /* the runs' own (1.0.0b13) */
       FM_PLATE, FM_PIPE,                                       /* a puzzle's flagstone, stone or glass, and its channels (1.0.0b13) */
       FM_ICE,                                                  /* the tutorial's ice field (docs/PLAN-tutorial.md 7.1) */
       FM_WARD };                                               /* a soul-lamp's ward, at its edge (1.0.0b18.1) */
typedef struct { uint8_t kind, prio; float k, ax, ay, r, ang; } FMark;
static FMark fmark[MAPN][MAPN];
static void fm_set(int x, int y, int kind, int prio, float k, float ax, float ay, float r, float ang) {
    if (x < 0 || y < 0 || x >= MAPN || y >= MAPN || fmark[y][x].prio > prio) return;
    FMark *m = &fmark[y][x];
    m->kind = (uint8_t)kind; m->prio = (uint8_t)prio; m->k = k; m->ax = ax; m->ay = ay; m->r = r; m->ang = ang;
}
static void fm_put(int x, int y, int kind, int prio, float k, const Hazard *h) { fm_set(x, y, kind, prio, k, h->x, h->y, h->r, h->ang); }
static void fm_disc(float ax, float ay, float r, int kind, int prio, float k) {
    for (int y = (int)floorf(ay - r - 1.0f); y <= (int)floorf(ay + r + 1.0f); y++)
        for (int x = (int)floorf(ax - r - 1.0f); x <= (int)floorf(ax + r + 1.0f); x++) fm_set(x, y, kind, prio, k, ax, ay, r, 0.0f);
}
/* the runs' own hazards on the floor (1.0.0b13): warning, harming, fading */
static void hz_marks_run(const Hazard *h, int ph, float kw, float kf) {
    switch (h->kind) {
    case HZ_BREATH: for (int c = 0; c < h->n; c++) fm_put(h->cx[c], h->cy[c], FM_VAPOUR, 2, ph == 0 ? 0.3f * kw : (ph == 1 ? 1.0f : kf), h); break;
    case HZ_DAMP: for (int c = 0; c < h->n; c++) fm_put(h->cx[c], h->cy[c], FM_SHIMMER, 2, ph == 0 ? kw : (ph == 1 ? 1.0f : kf), h); break;
    case HZ_SILT: for (int c = 0; c < h->n; c++) fm_put(h->cx[c], h->cy[c], ph == 0 ? FM_BUBBLE : FM_SHIMMER, 2, ph == 0 ? 1.0f : (ph == 1 ? 0.8f : 0.8f * kf), h); break;
    case HZ_VOTIVE:
        for (int c = 0; c < h->n; c++) {
            if (ph == 1) fm_put(h->cx[c], h->cy[c], FM_FIRE, 3, 1.0f, h);
            else if (ph == 2) fm_put(h->cx[c], h->cy[c], FM_EMBER, 1, kf, h);
            else if (kw * (float)(h->n + 1) >= (float)h->cd[c]) fm_put(h->cx[c], h->cy[c], FM_WAX, 2, clampf(kw * (float)(h->n + 1) - (float)h->cd[c], 0.0f, 1.0f), h);
        }
        break;
    case HZ_COLD:
        for (int c = 0; c < h->n; c++) {
            float g = ph == 0 ? clampf(kw * (h->r + 1.0f) - (float)h->cd[c], 0.0f, 1.0f) : (ph == 1 ? 1.0f : kf);
            if (g > 0.0f) fm_put(h->cx[c], h->cy[c], FM_RIME, 2, g, h);
        }
        break;
    case HZ_SLUICE: {
        float front = ph == 1 ? (h->t - h->warn) * 6.0f : -1.0f;
        for (int c = 0; c < h->n; c++) {
            if (ph == 0) { if (h->cd[c] <= 1) fm_put(h->cx[c], h->cy[c], FM_SHEEN, 2, kw, h); }
            else if (ph == 1) { if ((float)h->cd[c] <= front) fm_put(h->cx[c], h->cy[c], FM_SURGE, 3, (float)h->cd[c] > front - 1.5f ? 1.0f : 0.25f, h); }
            else fm_put(h->cx[c], h->cy[c], FM_SHEEN, 2, kf, h);
        }
        break;
    }
    case HZ_GALV: for (int c = 0; c < h->n; c++) fm_put(h->cx[c], h->cy[c], FM_SPARK, 3, ph == 0 ? 0.35f * kw : (ph == 1 ? 1.0f : 0.4f * kf), h); break;
    case HZ_SAINT: for (int c = 0; c < h->n; c++) { float w = hz_bile_wet(h, c); if (w > 0.0f) fm_put(h->cx[c], h->cy[c], FM_POOL, 2, w, h); } break;
    case HZ_TIMBER: if (ph == 0) for (int c = 0; c < h->n; c++) fm_put(h->cx[c], h->cy[c], FM_DUST, 2, kw, h); break;
    case HZ_LODE: if (ph == 0) fm_disc(h->x, h->y, h->r, FM_SRING, 2, kw); else if (ph == 1) fm_disc(h->x, h->y, h->r, FM_SPARK, 3, 1.0f); break;
    case HZ_CRYSTAL: if (ph == 0) fm_disc(h->x, h->y, h->r, FM_SRING, 2, kw); break;
    case HZ_ASH: fm_disc(h->x, h->y, h->r, FM_ASH, 2, ph == 0 ? 0.4f * kw : (ph == 1 ? 1.0f : kf)); break;
    case HZ_GLASS: fm_disc(h->x, h->y, h->r + 0.6f, FM_STAR, 2, ph == 0 ? kw : (ph == 1 ? 1.0f : kf)); break;
    default: break;
    }
}
static void censer_bob(const HzSite *s, float t, float *bx, float *by, float *v);
static void site_marks(void) {                                     /* what the fixed ones keep on the floor */
    for (int i = 0; i < G.nsite; i++) {
        const HzSite *s = &G.site[i];
        switch (s->kind) {
        case HZ_DCOLD: for (int c = 0; c < s->n; c++) fm_set(s->cx[c], s->cy[c], FM_RIME, 2, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f); break;
        case HZ_TITHE: fm_disc(s->x + 0.5f, s->y + 0.5f, 1.3f, FM_CHALK, 2, s->drink ? 1.0f : 0.0f); break;
        case HZ_RELIQ: if (s->state) fm_disc(s->x + 0.5f, s->y + 0.5f, 2.0f, FM_HAZE, 1, s->state == 1 ? clampf(s->k / 0.9f, 0.0f, 1.0f) * 0.6f : 0.6f); break;
        case HZ_CENSER: { float bx, by; censer_bob(s, s->t, &bx, &by, NULL); fm_disc(bx, by, 0.35f, FM_SHADOW, 2, 0.8f); break; }
        case HZ_SNARE: if (!s->state && (RUN.corner >> CS_OMEN & 1)) fm_set(s->x, s->y, FM_OMEN_CELL, 1, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f); break;   /* Premonition sees it */
        default: break;
        }
    }
}
static void fm_omen(int x, int y, int kind, float k, float ax, float ay, float r, float ang) {
    if (x < 0 || y < 0 || x >= MAPN || y >= MAPN || fmark[y][x].prio > 1) return;
    FMark *m = &fmark[y][x];
    m->kind = (uint8_t)kind; m->prio = 1; m->k = k; m->ax = ax; m->ay = ay; m->r = r; m->ang = ang;
}
static void fm_omen_disc(float ax, float ay, float r) {
    for (int y = (int)floorf(ay - r - 1.0f); y <= (int)floorf(ay + r + 1.0f); y++)
        for (int x = (int)floorf(ax - r - 1.0f); x <= (int)floorf(ax + r + 1.0f); x++) fm_omen(x, y, FM_OMEN, 1.0f, ax, ay, r, 0.0f);
}
static int omen_marks(void) {                                      /* Premonition, besides the hazards: embers, bursts, the Litany */
    if (!(RUN.corner >> CS_OMEN & 1) || G.shop) return 0;
    int any = 0;
    for (int i = 0; i < MAXBONES; i++) {                           /* where an ember will come down */
        const Bone *b = &G.bones[i];
        if (!b->alive || b->kind != MIS_EMBER) continue;
        fm_omen_disc(b->x + b->vx * fmaxf(0.0f, b->life), b->y + b->vy * fmaxf(0.0f, b->life), 0.7f); any = 1;
    }
    for (int i = 0; i < MAXMOBS; i++) {                            /* a drowned one near its end: it bursts */
        const Mob *m = &G.mobs[i];
        if (!m->alive || m->state != MS_WALK || m->type != MOB_DROWNED || buried(m) || m->hp > 0.25f * m->maxhp) continue;
        fm_omen_disc(m->x, m->y, 0.95f); any = 1;
    }
    if (G.boss == BOSS_DEACON && G.bpose == BP_LITANY && !PK.depth) {   /* the Litany reaches all he can see */
        const Mob *k = boss_mob();
        if (k && k->alive)
            for (int y = imax(1, (int)k->y - 11); y <= imin(MAPN - 2, (int)k->y + 11); y++)
                for (int x = imax(1, (int)k->x - 11); x <= imin(MAPN - 2, (int)k->x + 11); x++)
                    if (G.map[y][x].kind == K_FLOOR && line_clear(k->x, k->y, x + 0.5f, y + 0.5f)) { fm_omen(x, y, FM_OMEN_CELL, 0.5f, 0.0f, 0.0f, 0.0f, 0.0f); any = 1; }
    }
    return any;
}
static int ward_marks(void) {                                      /* the soul-lamps' ward, where it meets the open floor */
    int any = 0;
    for (int y = 1; y < MAPN - 1; y++)
        for (int x = 1; x < MAPN - 1; x++) {
            if (!wraith_barred(x, y) || fmark[y][x].kind != FM_NONE || G.map[y][x].kind != K_FLOOR) continue;
            int edge = 0;
            for (int k = 0; k < 4; k++) edge |= !wraith_barred(x + DX[k], y + DY[k]) && is_open(x + DX[k], y + DY[k]);
            if (edge) { fm_set(x, y, FM_WARD, 0, 1.0f, WD.flare_x, WD.flare_y, WD.flare_t, 0.0f); any = 1; }
        }
    return any;
}
static int ice_marks(void) {                                       /* level 2's ice over its floor, as far as each cell is laid */
    int any = 0;
    for (int i = 0; i < IC.n && g_drawn == 2; i++) {
        float k = ice_k(i);
        if (k > 0.0f) { fm_set(IC.cx[i], IC.cy[i], FM_ICE, 1, k, 0.0f, 0.0f, 0.0f, 0.0f); any = 1; }
    }
    return any;
}
static int hz_marks(void) {
    memset(fmark, 0, sizeof fmark);
    if (G.shop) return 0;
    int any = omen_marks();
    any |= ice_marks();
    for (int i = 0; i < MAXHAZ && !PK.depth; i++) {
        const Hazard *h = &G.haz[i];
        if (!h->alive) continue;
        if (h->pre > 0.0f) {                                       /* Premonition's zone, before the hazard shows itself */
            any = 1;
            if (hz_cellkind(h->kind)) { for (int c = 0; c < h->n; c++) fm_omen(h->cx[c], h->cy[c], FM_OMEN_CELL, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f); }
            else for (int y = (int)floorf(h->y - h->r - 1.0f); y <= (int)floorf(h->y + h->r + 1.0f); y++)
                for (int x = (int)floorf(h->x - h->r - 1.0f); x <= (int)floorf(h->x + h->r + 1.0f); x++)
                    fm_omen(x, y, h->kind == HZ_ARC ? FM_OMEN_ARC : FM_OMEN, 1.0f, h->x, h->y, h->r, h->ang);
            continue;
        }
        int ph = hz_phase(h);
        float kw = clampf(h->t / h->warn, 0.0f, 1.0f), kf = h->off > 0.0f ? 1.0f - clampf((h->t - h->warn - h->on) / h->off, 0.0f, 1.0f) : 0.0f;
        int x0 = (int)floorf(h->x - h->r - 1.0f), x1 = (int)floorf(h->x + h->r + 1.0f), y0 = (int)floorf(h->y - h->r - 1.0f), y1 = (int)floorf(h->y + h->r + 1.0f);
        any = 1;
        if (h->kind >= HZ_CENSER && h->kind != HZ_FISSURE) { hz_marks_run(h, ph, kw, kf); continue; }
        switch (h->kind) {
        case HZ_ROCK: if (ph == 0) for (int y = y0; y <= y1; y++) for (int x = x0; x <= x1; x++) fm_put(x, y, FM_SHADOW, 2, kw, h); break;
        case HZ_FIRE: case HZ_FISSURE:
            for (int c = 0; c < h->n; c++) {
                if (ph == 1) { fm_put(h->cx[c], h->cy[c], FM_FIRE, 3, 1.0f, h); continue; }
                if (ph == 2) { fm_put(h->cx[c], h->cy[c], FM_EMBER, 1, kf, h); continue; }
                float reach = h->warn * 0.6f * (float)h->cd[c] / fmaxf(1.0f, h->r);   /* the crack gets there along the line */
                if (h->t >= reach) fm_put(h->cx[c], h->cy[c], FM_CRACK, 2, clampf((h->t - reach) / fmaxf(0.1f, h->warn - reach), 0.0f, 1.0f), h);
            }
            break;
        case HZ_BILE:
            for (int c = 0; c < h->n; c++) {
                float w = hz_bile_wet(h, c);
                if (w <= 0.0f) continue;
                if (w >= 1.0f && ph < 2) fm_put(h->cx[c], h->cy[c], FM_BILE, 3, 1.0f, h);
                else fm_put(h->cx[c], h->cy[c], FM_SHEEN, 2, w, h);
            }
            break;
        case HZ_VOID:
            for (int y = y0; y <= y1; y++) for (int x = x0; x <= x1; x++)
                fm_put(x, y, ph == 0 ? FM_VRING : FM_VOID, ph == 1 ? 3 : 2, ph == 0 ? kw : (ph == 1 ? 1.0f : kf), h);
            break;
        case HZ_ARC:
            if (ph < 2 || h->t < h->warn + h->on + 0.15f)
                for (int y = y0; y <= y1; y++) for (int x = x0; x <= x1; x++) fm_put(x, y, FM_ARC, 2, ph == 0 ? kw : 2.0f, h);
            break;
        default:
            if (ph == 0) for (int y = y0; y <= y1; y++) for (int x = x0; x <= x1; x++) fm_put(x, y, FM_GRASP, 2, kw, h);
            break;
        }
    }
    if (G.nsite && !PK.depth) { site_marks(); any = 1; }
    for (int i = 0; i < G.nthing && !PK.depth; i++) {             /* a puzzle's flagstones, stepping stones, glass and channels */
        const Thing *t = &G.thing[i];
        if (t->role == TR_PLATE) { fm_set(t->cx, t->cy, FM_PLATE, 1, t->on ? 1.0f : 0.0f, (float)t->look, (float)t->mark, (float)t->state, 0.0f); any = 1; }
        else if (t->role == TR_TURN && t->spr < 0) { fm_set(t->cx, t->cy, FM_PIPE, 1, (float)t->look, (float)pipe_mask(t->val, t->state), (float)i, 0.0f, 0.0f); any = 1; }
    }
    if (WD.mask && !PK.depth) any |= ward_marks();                 /* the soul-lamps' ward, under all the rest (1.0.0b18.1) */
    return any;
}
/* one floor pixel under a mark: returns a colour drawn as it is (bright, or the void's dark), or -1 to
 * be lit as usual, with the floor's colour and a dimming for its light set */
static int fm_pixel(const FMark *m, float wx, float wy, float fx, float fy, int x, int y, uint8_t *base, float *lmul) {
    float d = hypotf(wx - m->ax, wy - m->ay), k = m->k;
    int b = BAYER[y & 3][x & 3];
    switch (m->kind) {
    case FM_SHADOW: {                                              /* a blur that hardens into a dark disc */
        float rr = m->r * (0.45f + 0.55f * k), edge = 0.05f + 0.3f * (1.0f - k);
        if (d < rr) *lmul *= 1.0f - 0.85f * k * (1.0f - smoothstepf(rr - edge, rr, d));
        if (k > 0.6f && fabsf(d - rr) < 0.035f) return 0;
        return -1;
    }
    case FM_CRACK: case FM_EMBER: {
        int tx = (int)(fx * TS) & (TS - 1), ty = (int)(fy * TS) & (TS - 1);
        uint32_t hh = hash2((int)floorf(wx), (int)floorf(wy));
        if (hh & 1) tx = TS - 1 - tx;
        if (hh & 2) { int q = tx; tx = ty; ty = q; }
        if (crack_mask[tx * TS + ty]) {
            if (m->kind == FM_EMBER) return R_BLOOD + clampi((int)(3.0f + 9.0f * k), 0, 15);
            return k < 0.5f ? R_BLOOD + clampi((int)(5.0f + k * 18.0f), 0, 15) : R_FLAME + clampi((int)(3.0f + (k - 0.5f) * 24.0f), 0, 15);
        }
        if (m->kind == FM_EMBER) *lmul *= 0.75f;
        return -1;
    }
    case FM_FIRE: {
        float v = 0.5f + 0.5f * sinf(wx * 9.0f + (float)G.t * 7.0f) * sinf(wy * 8.0f - (float)G.t * 5.0f);
        return R_FLAME + clampi((int)(5.0f + 10.0f * v), 0, 15);
    }
    case FM_SHEEN:                                                 /* wet and dark, a glint or two */
        *lmul *= 1.0f - 0.4f * k;
        if (k > 0.3f && (hash2((int)(wx * 9.0f), (int)(wy * 9.0f)) & 63) == 0) *base = (uint8_t)(R_GHOST + 12);
        return -1;
    case FM_BILE: {                                                /* foul and green, bubbling: it shows in the dark */
        int cx = (int)floorf(wx), cy = (int)floorf(wy);
        for (int i = 0; i < 3; i++) {
            uint32_t hh = hash2(cx * 3 + i, cy * 5 - i);
            float bx = 0.2f + 0.6f * (float)(hh & 255) / 255.0f, by = 0.2f + 0.6f * (float)((hh >> 8) & 255) / 255.0f;
            float ph = (float)G.t * 0.8f + (float)((hh >> 16) & 255) / 255.0f;
            float rr = 0.14f * (ph - floorf(ph)), dd = hypotf(fx - bx, fy - by);
            if (fabsf(dd - rr) < 0.025f) return R_MOSS + 15;     /* a bubble's skin */
        }
        float v = 0.5f + 0.25f * sinf((wx + wy) * 5.0f + (float)G.t * 1.7f);
        return R_MOSS + clampi((int)(3.0f + 6.0f * v + (b > 12 ? 1 : 0)), 0, 15);
    }
    case FM_VRING:                                                 /* a ring where it will open, and a haze in it */
        if (fabsf(d - m->r) < 0.05f + 0.05f * k) return R_PURPLE + clampi((int)(9.0f + 6.0f * k), 0, 15);
        if (d < m->r && b < (int)(k * 4.0f)) return R_PURPLE + 4;
        return -1;
    case FM_VOID:
        if (fabsf(d - m->r) < 0.07f) return R_PURPLE + clampi((int)(8.0f + 7.0f * k), 0, 15);
        if (d < m->r && b < (int)(k * 11.0f)) return R_PURPLE + 2 + (b & 3);
        return -1;
    case FM_ARC: {                                                 /* a pale fan before the blade */
        float a = fabsf(wrap_pi(atan2f(wy - m->ay, wx - m->ax) - m->ang));
        if (d > m->r || a > 0.8f || d < 0.2f) return -1;
        if (k >= 2.0f || fabsf(d - m->r) < 0.06f || fabsf(a - 0.8f) * d < 0.05f) return R_BONE + 14;
        return b < (int)(k * 7.0f) ? R_BONE + 12 : -1;
    }
    case FM_GRASP:
        if (fabsf(d - m->r) < 0.07f) return 0;
        if (d < m->r && b < (int)(k * 8.0f)) return 0;
        return -1;
    case FM_VAPOUR: case FM_SHIMMER: case FM_HAZE: {              /* a haze dithered over the floor: grey-green breath, sallow gas, violet hunger */
        float v = 0.5f + 0.5f * sinf(wx * 4.0f + (float)G.t * 1.3f) * sinf(wy * 3.5f - (float)G.t * 0.9f);
        if (b >= (int)(k * (m->kind == FM_VAPOUR ? 9.0f : 6.0f) * (0.6f + 0.4f * v))) return -1;
        *base = (uint8_t)((m->kind == FM_VAPOUR ? R_MOSS + 5 : m->kind == FM_SHIMMER ? R_WARM + 9 : R_PURPLE + 7) + (b & 3));
        return m->kind == FM_HAZE ? *base : -1;
    }
    case FM_WAX: case FM_POOL: {                                   /* pale and wet: wax running, the saint's tears pooling */
        *lmul *= 1.0f - 0.25f * k;
        if (b < (int)(k * 10.0f)) *base = (uint8_t)(m->kind == FM_WAX ? R_BONE + 11 + (b & 3) : R_GHOST + 5 + (b & 3));
        if (m->kind == FM_POOL && k > 0.5f && (hash2((int)(wx * 9.0f), (int)(wy * 9.0f)) & 63) == 0) return R_PURPLE + 12;   /* a violet glint */
        return -1;
    }
    case FM_ICE:                                                   /* the tutorial's ice: laid texel by texel, catching the light */
        *base = crawl_ice_at(wx, wy, k, *base);
        *lmul *= 1.0f + 0.15f * k;
        return -1;
    case FM_RIME: {                                                /* frost feathering over the floor */
        uint32_t hh = hash2((int)(wx * 16.0f), (int)(wy * 16.0f));
        if ((float)(hh & 255) / 255.0f < k * 0.75f) *base = (uint8_t)(R_GHOST + 10 + (hh >> 10 & 3) + ((hh >> 12 & 7) == 0 ? 2 : 0));
        return -1;
    }
    case FM_SURGE: {                                               /* the surge: foam at its front, wet behind */
        uint32_t hh = hash2((int)(wx * 10.0f + (float)G.t * 8.0f), (int)(wy * 10.0f));
        *lmul *= 0.75f;
        if ((float)(hh & 255) / 255.0f < 0.25f + 0.5f * k) *base = (uint8_t)(R_GHOST + 11 + (hh >> 9 & 3));
        return -1;
    }
    case FM_SPARK: {                                               /* lightning crawling over the water, at full brightness */
        uint32_t hh = hash2((int)(wx * 12.0f) + (int)(G.t * 20.0) * 131, (int)(wy * 12.0f));
        if ((int)(hh & 31) < (int)(k * 7.0f)) return R_GLOW + 11 + (int)(hh >> 8 & 3);
        if (k > 0.8f && (hh >> 12 & 7) == 0) return C_WHITE;
        return -1;
    }
    case FM_BUBBLE: {                                              /* silt giving up its gas */
        int cx = (int)floorf(wx), cy = (int)floorf(wy);
        for (int i = 0; i < 4; i++) {
            uint32_t hh = hash2(cx * 7 + i, cy * 3 - i);
            float bx = 0.15f + 0.7f * (float)(hh & 255) / 255.0f, by = 0.15f + 0.7f * (float)((hh >> 8) & 255) / 255.0f;
            float ph = (float)G.t * 1.4f + (float)((hh >> 16) & 255) / 255.0f, rr = 0.1f * (ph - floorf(ph)), dd = hypotf(fx - bx, fy - by);
            if (fabsf(dd - rr) < 0.022f) { *base = R_WARM + 13; return -1; }
        }
        return -1;
    }
    case FM_DUST:                                                  /* dust sifting down: the floor darkening, grit on it */
        *lmul *= 1.0f - 0.5f * k;
        if (b < (int)(k * 3.0f)) *base = R_STONE + 9;
        return -1;
    case FM_SRING:                                                 /* a ring of sparks where it will reach */
        if (fabsf(d - m->r) < 0.04f + 0.04f * k && (((int)(atan2f(wy - m->ay, wx - m->ax) * 9.0f + (float)G.t * 6.0f)) & 1)) return R_GLOW + clampi((int)(8.0f + 7.0f * k), 0, 15);
        return -1;
    case FM_ASH:                                                   /* ash settling */
        *lmul *= 1.0f - 0.35f * k;
        if (d < m->r && b < (int)(k * 7.0f)) *base = (uint8_t)(R_STONE + 3 + (b & 3));
        return -1;
    case FM_STAR: {                                                /* the glass cracking in a star round where the spikes will stand */
        float a = atan2f(wy - m->ay, wx - m->ax), u = a * 7.0f / TAU_F + (float)(hash2((int)m->ax, (int)m->ay) & 255) / 255.0f;
        float off = fabsf(u - floorf(u + 0.5f));
        if (d < (m->r + 0.2f) * k && off * d < 0.035f + 0.02f * (1.0f - d / (m->r + 0.2f))) { *base = R_GHOST + 13; *lmul = fmaxf(*lmul, 0.9f); }
        return -1;
    }
    case FM_CHALK: {                                               /* a ring of the cult's marks, lit while it takes its tithe */
        float a = atan2f(wy - m->ay, wx - m->ax);
        int mark = ((int)floorf((a + PI_F) * 12.0f / TAU_F * 2.0f)) & 1;
        if (fabsf(d - m->r) < 0.05f || (mark && fabsf(d - m->r + 0.18f) < 0.07f && fabsf(fmodf((a + PI_F) * 12.0f / TAU_F, 1.0f) - 0.5f) < 0.18f)) {
            if (k > 0.5f) return R_PURPLE + clampi((int)(11.0f + 4.0f * sinf((float)G.t * 6.0f)), 0, 15);
            *base = R_BONE + 13;
        }
        return -1;
    }
    case FM_PIPE: {                                                /* a channel cut in the floor (or gold in the stone), wet where the water has come */
        int mask = (int)m->ax, wet = 0, look = (int)m->k;
        if (G.pz == PZ_CULVERTS || G.pz == PZ_VEINS) { uint32_t wm; pipe_flow(&wm); for (int k = 0; k < 16; k++) if ((wm >> k & 1) && G.pz_a[4 + k] == (int)m->ay) wet = 1; }
        float cx = fx - 0.5f, cy = fy - 0.5f, wdt = 0.13f;
        int in = 0;
        if (fabsf(cx) < wdt && fabsf(cy) < wdt) in = 1;
        if ((mask & 1) && cx > 0.0f && fabsf(cy) < wdt) in = 1;
        if ((mask & 4) && cx < 0.0f && fabsf(cy) < wdt) in = 1;
        if ((mask & 2) && cy > 0.0f && fabsf(cx) < wdt) in = 1;
        if ((mask & 8) && cy < 0.0f && fabsf(cx) < wdt) in = 1;
        float ex = fminf(fx, 1.0f - fx), ey = fminf(fy, 1.0f - fy);
        if (fminf(ex, ey) < 0.04f) { *base = R_STONE + 2; return -1; }   /* the tile's edge */
        if (!in) return -1;
        if (look) return wet ? R_GOLD + 9 + (b & 3) : (int)(R_GOLD + 3 + (b & 1));   /* the veins: dull, then bright */
        if (wet) return R_GLOW + 6 + ((b + (int)(G.t * 8.0)) & 3);               /* the water, moving */
        *base = R_STONE + 3;
        return -1;
    }
    case FM_PLATE: {                                               /* a flagstone set in the floor, its edge cut deep; sunk, darker */
        float ex = fminf(fx, 1.0f - fx), ey = fminf(fy, 1.0f - fy), e = fminf(ex, ey);
        if ((int)m->ax == 1) {                                     /* a stepping stone in dark water */
            float r = hypotf(fx - 0.5f, fy - 0.5f);
            if (r > 0.36f) { *base = (uint8_t)(R_GHOST + 2 + ((b + (int)(G.t * 3.0)) & 1)); *lmul *= 0.7f; return -1; }
            if (r > 0.32f) { *base = R_STONE + 3; return -1; }
            int mk = (int)m->ay;
            if (mk > 0 && mk <= 16) {
                int gx = (int)((fx - 0.25f) * 14.0f), gy = (int)((fy - 0.25f) * 14.0f);
                if (gx >= 0 && gy >= 0 && gx < 7 && gy < 7 && PZ_MARKS[mk - 1][gy][gx] == '#') { *base = R_STONE + 13; return -1; }
            }
            *base = (uint8_t)(R_STONE + 7 + (k > 0.5f ? -3 : 0));
            return -1;
        }
        if ((int)m->ax == 2) {                                     /* a tile of the glass floor: dark, or lit from below */
            if (e < 0.05f) { *base = R_IRON + 1; return -1; }
            if (m->r > 0.5f) return R_GLOW + 5 + (b & 3);
            *base = (uint8_t)(R_IRON + 3 + ((int)((fx + fy) * 6.0f) & 1));
            if (fabsf(fx - fy) < 0.02f) *base = R_GHOST + 9;      /* a glint across it */
            return -1;
        }
        if (e < 0.06f) { *base = R_STONE + 1; return -1; }
        if (e < 0.1f) { *base = (uint8_t)(R_STONE + (k > 0.5f ? 4 : 10)); return -1; }
        *lmul *= k > 0.5f ? 0.65f : 1.0f;
        int mk = (int)m->ay;                                       /* a mark cut in it */
        if (mk > 0 && mk <= 16) {
            int gx = (int)((fx - 0.25f) * 14.0f), gy = (int)((fy - 0.25f) * 14.0f);
            if (gx >= 0 && gy >= 0 && gx < 7 && gy < 7 && PZ_MARKS[mk - 1][gy][gx] == '#') *base = R_STONE + 2;
        }
        if (m->r > 0.5f) *base = (uint8_t)(R_GLOW + 6 + (b & 3));  /* lit (the glass floor) */
        return -1;
    }
    case FM_OMEN: case FM_OMEN_CELL: case FM_OMEN_ARC: {          /* Premonition: a dashed purple edge, pulsing, and a faint haze */
        uint8_t edge = (uint8_t)(R_PURPLE + clampi((int)(11.0f + 4.0f * sinf((float)G.t * 7.0f)), 0, 15));
        int inside, on_edge;
        if (m->kind == FM_OMEN_CELL) {                             /* only the edges with no marked cell beyond */
            int cx = (int)floorf(wx), cy = (int)floorf(wy);
            #define OM(xx, yy) ((xx) >= 0 && (yy) >= 0 && (xx) < MAPN && (yy) < MAPN && fmark[yy][xx].kind == FM_OMEN_CELL)
            int dash = ((int)((fx + fy) * 8.0f + (float)G.t * 3.0f) & 1);
            on_edge = dash && ((fx < 0.07f && !OM(cx - 1, cy)) || (fx > 0.93f && !OM(cx + 1, cy)) || (fy < 0.07f && !OM(cx, cy - 1)) || (fy > 0.93f && !OM(cx, cy + 1)));
            #undef OM
            inside = 1;
        } else if (m->kind == FM_OMEN_ARC) {
            float a = fabsf(wrap_pi(atan2f(wy - m->ay, wx - m->ax) - m->ang));
            inside = d <= m->r && a <= 0.8f && d > 0.2f;
            on_edge = inside && (fabsf(d - m->r) < 0.06f || fabsf(a - 0.8f) * d < 0.05f) && ((int)(d * 10.0f) & 1);
        } else {
            float a = atan2f(wy - m->ay, wx - m->ax);
            inside = d < m->r;
            on_edge = fabsf(d - m->r) < 0.05f && ((int)(a * 7.0f + (float)G.t * 2.0f) & 1);   /* dashed, turning slowly */
        }
        if (on_edge) return edge;
        if (inside && b < (k < 0.75f ? 1 : 2)) return R_PURPLE + 7;
        return -1;
    }
    case FM_WARD: {                                                /* a soul-lamp's ward: a faint violet line where it meets the open */
        int cx = (int)floorf(wx), cy = (int)floorf(wy);
        #define WO(xx, yy) (!wraith_barred(xx, yy) && is_open(xx, yy))
        int on = (fx < 0.06f && WO(cx - 1, cy)) || (fx > 0.94f && WO(cx + 1, cy)) || (fy < 0.06f && WO(cx, cy - 1)) || (fy > 0.94f && WO(cx, cy + 1));
        #undef WO
        if (!on) return -1;
        float fl = m->r > 0.0f ? m->r * (1.0f - smoothstepf(0.0f, 1.8f, hypotf(wx - m->ax, wy - m->ay))) : 0.0f;   /* flaring where the Wraith is held */
        if (!((int)((fx + fy) * 6.0f + (float)G.t * 0.8f) & 1) && fl < 0.3f) return -1;   /* dashed, drifting slowly */
        return R_PURPLE + clampi((int)(6.0f + 1.5f * sinf((float)G.t * 1.7f + wx + wy) + 9.0f * fl), 0, 15);
    }
    }
    return -1;
}
static int fm_at(int cx, int cy, float wx, float wy, float fx, float fy, int x, int y, uint8_t *base, float *lmul) {
    if (cx < 0 || cy < 0 || cx >= MAPN || cy >= MAPN || !fmark[cy][cx].kind) return -1;
    return fm_pixel(&fmark[cy][cx], wx, wy, fx, fy, x, y, base, lmul);
}

