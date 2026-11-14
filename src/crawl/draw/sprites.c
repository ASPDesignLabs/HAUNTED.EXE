/* A part of scene_crawl.c (docs/PLAN-tidy.md): it is #included there in its place and never compiled on its own.
 * Sprites: eyes and vapour, beams, the cat, which sprite a creature shows (mob_sprite, mob_side), render_sprites. */

static void eye_px(Frame *f, int x, int y, uint8_t c, float depth) {
    if (x >= 0 && x < W && y >= 0 && y < H && depth < zbuf[x]) f->pix[y * W + x] = c;
}
/* the mark over a mouse carrying word of you (docs/PLAN-mice.md, M9): a point with sound going out either side, in their green, the
 * rings pulsing; bigger near. Hidden only by walls, as the eyes are. */
static void render_mflag(Frame *f, const Draw *d, float left, float top, float size) {
    static const char *const MARK[5] = { ".#.......#.", "#...###...#", "#..#####..#", "#...###...#", ".#.......#." };
    int k = size > 90.0f ? 2 : 1, cx = (int)(left + size * 0.5f), cy = (int)(top + size * 0.52f) - 5 * k - 6;
    int ring = ((int)((float)G.t * 5.0f)) & 1;
    for (int y = 0; y < 5; y++)
        for (int x = 0; x < 11; x++) {
            if (MARK[y][x] != '#') continue;
            int inner = x > 2 && x < 8;
            if (!inner && ring) continue;                          /* the sound going out, now and again */
            uint8_t c = inner ? (uint8_t)C_MOUSE_EYE : (uint8_t)(R_MOSS + 12);
            for (int yy = 0; yy < k; yy++) for (int xx = 0; xx < k; xx++) eye_px(f, cx + (x - 5) * k + xx, cy + (y - 2) * k + yy, c, d->depth);
        }
}
static void render_eyes(Frame *f, const Draw *d, float left, float top, float size) {
    const EyeAt *e = &crawl_eyes[d->spr];
    if (!e->n || d->alpha < 0.3f) return;
    int st = d->estate, rank = d->erank, ec = EYE_OWN[d->etype];
    if (st == 3) ec = EC_ASH;                                      /* asleep: an ember */
    else if (d->ecol) ec = d->ecol - 1;                            /* its own, whatever its rank (the Wandering Spider's fire) */
    else if (rank > RK_I && EYE_RANK[rank] >= 0) ec = EYE_RANK[rank];
    else if ((d->etype == MOB_SKEL || d->etype == MOB_SHIELD || d->etype == MOB_HOUND)
             && fmaxf(crawl_decay(G.level), crawl_depth(G.level)) > 0.55f) ec = EC_COLD;   /* the old dead's eyes go cold */
    float ph = d->x * 3.1f + d->y * 1.7f;
    if (st == 0 && fmodf((float)G.t * 0.37f + ph, 5.0f) < 0.08f) return;   /* a blink, now and then, while it is calm */
    uint8_t core = EYE_CORE[ec], glow = EYE_GLOW[ec];
    int born = ec >= EC_EL0 && ec <= EC_EL_LAST;                   /* born to an element (1.0.0b19) */
    if (st == 2) core = C_WHITE;                                   /* striking: they flare */
    else if (born && st != 4 && st != 5 && fmodf((float)G.t * (ec == EC_EL0 + EL_LIGHT ? 11.0f : 6.0f) + ph, 1.0f) < 0.3f) core = glow;   /* its
                                                                      element stirring in them: a flicker, quicker for lightning */
    if (st == 4) st = 2;                                           /* striking with its element (Mob.elem_on): they flare in its colour */
    if (st == 5) { core = R_STONE + 11; glow = R_STONE + 6; born = 0; st = 1; }   /* Unbound (1.0.0b19): the fire gone out of them, grey,
                                                                      and its element with it */
    float es = size / TS;
    int sx[2], sy[2], n = e->n;
    for (int i = 0; i < n; i++) {
        int ty = (d->flags & DR_FLIPV) ? TS - 1 - e->y[i] : e->y[i], tx = (d->flags & DR_FLIPH) ? TS - 1 - e->x[i] : e->x[i];
        sx[i] = (int)floorf(left + (tx + 0.5f) * es); sy[i] = (int)floorf(top + (ty + 0.5f) * es);
    }
    if (n == 2 && abs(sx[1] - sx[0]) < 2) {                        /* far off: two points with a pixel between */
        int c = (sx[0] + sx[1]) / 2, l = sx[0] <= sx[1] ? 0 : 1;
        sx[l] = c - 1; sx[1 - l] = c + 1; sy[1] = sy[0] = (sy[0] + sy[1]) / 2;
    }
    int near = es >= 0.85f, cs = near ? imin(4, imax(2, (int)(es * 0.8f + 0.6f))) : 1, arm = cs + (st == 2) + (int)(es > 3.0f);
    for (int i = 0; i < n; i++) {
        int x0 = sx[i] - cs / 2, y0 = sy[i] - cs / 2, out = n == 2 ? (sx[i] < sx[1 - i] ? -1 : 1) : 1;
        if (near) {
            int flare = d->estate == 4;                            /* its element's flare: a full halo, a ring wider */
            if (st >= 1)                                           /* alerted: a halo, dithered */
                for (int y = y0 - 1 - flare; y <= y0 + cs + flare; y++)
                    for (int x = x0 - 1 - flare; x <= x0 + cs + flare; x++)
                        if (flare || ((x + y) & 1)) eye_px(f, x, y, glow, d->depth);
            if (born && !flare) {                                  /* a mote of it now and then, each its own way, so they read
                                                                      apart from the ranks' colours: fire rises, frost falls white,
                                                                      lightning sparks at the side, poison drips, spirit sways up */
                int el = ec - EC_EL0, mx = x0 + cs / 2, my = y0, side = i ? 1 : -1;
                float mt = fmodf((float)G.t * 1.3f + ph * 0.7f + (float)i * 0.45f, 1.0f);
                uint8_t mc = glow;
                switch (el) {
                case EL_FIRE: mx += side * (int)(mt * 2.0f); my = y0 - 1 - (int)(mt * (float)(cs + 4)); break;
                case EL_FROST: mx += side * 2; my = y0 + cs + (int)(mt * (float)(cs + 3)); mc = C_WHITE; break;
                case EL_LIGHT: mt *= 2.0f; mx = side > 0 ? x0 + cs + 1 + (int)(mt * 2.0f) : x0 - 2 - (int)(mt * 2.0f); my = y0 + (int)(mt * 3.0f) % imax(1, cs); break;
                case EL_POISON: mx += side; my = y0 + cs + (int)(mt * mt * (float)(cs + 5)); break;
                default: mx += (int)(2.0f * sinf(mt * TAU_F)); my = y0 - 1 - (int)(mt * (float)(cs + 5)); break;
                }
                if (mt < 0.7f) eye_px(f, mx, my, mc, d->depth);
            }
            switch (rank) {
            case RK_II: for (int k = 0; k < cs; k++) eye_px(f, out > 0 ? x0 + cs : x0 - 1, y0 + k, glow, d->depth); break;   /* a glow beside */
            case RK_III: for (int k = 1; k <= cs; k++) { eye_px(f, x0 + cs / 2, y0 - k, glow, d->depth); eye_px(f, x0 + cs / 2, y0 + cs - 1 + k, glow, d->depth); } break;   /* a slit */
            case RK_IV: for (int k = 1; k <= arm + 1; k++) eye_px(f, (out > 0 ? x0 + cs - 1 : x0) + out * k, y0 - (k + 1) / 2, k > arm ? (uint8_t)(R_BLOOD + 9) : glow, d->depth); break;   /* to the temple */
            case RK_V: {                                           /* a white flare, slowly breathing */
                int a = arm + (int)(1.5f + 1.5f * sinf((float)G.t * 1.7f + ph));
                for (int k = 1; k <= a; k++) {
                    uint8_t c = k == a ? (born ? (uint8_t)(glow - 4) : ec == EC_FIRE ? (uint8_t)(R_FLAME + 3) : (uint8_t)(R_PURPLE + 11)) : glow;
                    eye_px(f, x0 + cs / 2, y0 - k, c, d->depth); eye_px(f, x0 + cs / 2, y0 + cs - 1 + k, c, d->depth);
                    eye_px(f, x0 - k, y0 + cs / 2, c, d->depth); eye_px(f, x0 + cs - 1 + k, y0 + cs / 2, c, d->depth);
                }
                break;
            }
            case RK_GILDED:
                eye_px(f, x0 + cs / 2, y0 - 1, glow, d->depth); eye_px(f, x0 + cs / 2, y0 + cs, glow, d->depth);
                eye_px(f, x0 - 1, y0 + cs / 2, glow, d->depth); eye_px(f, x0 + cs, y0 + cs / 2, glow, d->depth);
                break;
            default: break;
            }
        }
        for (int y = 0; y < cs; y++) for (int x = 0; x < cs; x++) eye_px(f, x0 + x, y0 + y, core, d->depth);
    }
}

/* Spirit vapour (docs/InitialArtPass.md section 7): the soul showing, more of it the higher the rank. III a thread from the
 * eyes, cold; IV a plume from the eyes and the chest, red, embers falling; V a cloak of dark smoke about it, a violet rim and
 * a white spark now and then; gilded, gold motes going round its head. Dithered, slow, fogged with the body, hidden by
 * walls, drawn before the eyes so it never covers them; its randomness the renderer's own (hash2), never the game's.
 * g_fx: 0 full, 1 calm (half of it, slower), 2 none (the CREATURE EFFECTS setting). */
static int g_fx;
static void vap_px(Frame *f, int x, int y, int ps, uint8_t c, float depth, float fog, int dark) {
    for (int j = 0; j < ps; j++)
        for (int i = 0; i < ps; i++) {
            int px = x + i, py = y + j;
            if (px < 0 || px >= W || py < 0 || py >= H || depth >= zbuf[px] || fog * 16.0f > BAYER[py & 3][px & 3] + 0.5f) continue;
            uint8_t *p = &f->pix[py * W + px];
            if (dark) { if (!IS_FULLBRIGHT(*p)) *p = crawl_cmap[6][*p]; }   /* smoke that eats the light behind it */
            else *p = c;
        }
}
static int is_back(int s);
static void render_vapour(Frame *f, const Draw *d, float left, float top, float size, float fog) {
    int rank = d->erank;
    if (d->vap <= 0.0f || g_fx >= 2 || rank < RK_III) return;
    float es = size / TS, k = d->vap * (g_fx ? 0.5f : 1.0f), slow = g_fx ? 0.6f : 1.0f;
    int ps = es >= 1.6f ? 2 : 1;
    uint32_t seed = (uint32_t)(int)(d->x * 13.0f) * 2654435761u ^ (uint32_t)(int)(d->y * 7.0f) * 40503u;
    const EyeAt *e = &crawl_eyes[d->spr];
    float ex[3], ey[3]; int ne = 0;
    for (int i = 0; i < e->n; i++) { ex[ne] = (d->flags & DR_FLIPH) ? TS - 1 - e->x[i] : e->x[i]; ey[ne] = (d->flags & DR_FLIPV) ? TS - 1 - e->y[i] : e->y[i]; ne++; }
    if (!ne) { ex[ne] = 32.0f; ey[ne] = is_back(d->spr) ? 9.0f : 16.0f; ne++; }   /* a back: from the skull */
    float t = (float)G.t * slow;
    #define VX(fx) ((int)floorf(left + (fx) * es))
    #define VY(fy) ((int)floorf(top + (fy) * es))
    if (rank == RK_GILDED) {                                       /* gold motes going round its head */
        float cx = 0.0f, cy = 0.0f;
        for (int i = 0; i < ne; i++) { cx += ex[i]; cy += ey[i]; }
        cx /= (float)ne; cy /= (float)ne;
        int nm = k >= 1.0f ? 4 : 2;
        for (int m = 0; m < nm; m++) {
            float a = t * 1.1f + (float)m * TAU_F / (float)nm, x = cx + cosf(a) * 11.0f, y = cy - 3.0f + sinf(a) * 3.5f + sinf(t * 2.0f + (float)m) * 1.0f;
            int glint = fmodf(t * 0.7f + (float)m * 0.37f, 1.0f) < 0.06f;
            vap_px(f, VX(x), VY(y), ps, glint ? (uint8_t)C_WHITE : (uint8_t)(R_GOLD + 11 - (m & 1) * 3), d->depth, fog, 0);
        }
        return;
    }
    if (rank == RK_V) {                                            /* a cloak of dark smoke about it */
        int np = (int)(30.0f * k);
        for (int i = 0; i < np; i++) {
            uint32_t h = hash2((int)(seed % 9973u) + i * 7, 11);
            float p = fmodf(t * 0.22f + (float)(h % 1000) / 1000.0f, 1.0f);
            float bx = 14.0f + (float)((h >> 10) % 36), by = 14.0f + (float)((h >> 16) % 44);
            const uint8_t *col = &crawl_spr[d->spr][clampi((d->flags & DR_FLIPH) ? TS - 1 - (int)bx : (int)bx, 0, TS - 1) * TS];
            int sy = (d->flags & DR_FLIPV) ? TS - 1 - (int)by : (int)by;
            if (col[clampi(sy, 0, TS - 1)] == C_CLEAR) continue;   /* it rises off the body */
            float x = bx + sinf(p * 6.0f + (float)i) * 2.0f, y = by - p * 22.0f;
            if (BAYER[i & 3][(i >> 2) & 3] < (int)(p * 16.0f)) continue;   /* thinning as it rises */
            int rim = (h >> 24) % 5 == 0, spark = (h >> 24) % 61 == 0 && p < 0.4f;
            vap_px(f, VX(x), VY(y), ps, spark ? (uint8_t)C_WHITE : (uint8_t)(R_PURPLE + 12), d->depth, fog, !rim && !spark);
        }
    }
    for (int s = 0; s < ne + (rank >= RK_IV); s++) {                /* threads or a plume from the eyes, and the chest */
        float sx = s < ne ? ex[s] : 32.0f, sy = s < ne ? ey[s] : (ey[0] + 16.0f);
        int np = (int)((rank == RK_III ? 5.0f : 8.0f) * k + 0.5f);
        float rise = rank == RK_III ? 14.0f : 20.0f, amp = rank == RK_III ? 1.2f : 2.6f;
        for (int i = 0; i < np; i++) {
            float p = fmodf(t * (rank == RK_III ? 0.32f : 0.42f) + (float)i / (float)np + (float)((seed >> (s * 3)) & 7) * 0.13f, 1.0f);
            float x = sx + sinf(p * 5.0f + (float)i * 1.3f + (float)s) * amp * p, y = sy - p * rise;
            if (p > 0.55f && ((i + (int)(p * 8.0f)) & 1)) continue;   /* fraying at the top */
            uint8_t c = rank == RK_III ? (uint8_t)(p < 0.5f ? R_GLOW + 13 : R_GHOST + 12) : (uint8_t)(p < 0.3f ? R_FLAME + 9 : p < 0.65f ? R_BLOOD + 13 : R_BLOOD + 9);
            if (rank == RK_V) c = p < 0.3f ? (uint8_t)C_WHITE : (uint8_t)(R_PURPLE + 13);
            vap_px(f, VX(x), VY(y), ps, c, d->depth, fog, 0);
        }
        if (rank == RK_IV && s == ne)                              /* embers falling off it */
            for (int i = 0; i < 3; i++) {
                float p = fmodf(t * 0.5f + (float)i * 0.33f, 1.0f);
                vap_px(f, VX(sx + (float)(i - 1) * 4.0f + sinf(p * 9.0f) * 1.5f), VY(sy + p * 18.0f), 1, (uint8_t)(R_FLAME + 6 + (i & 1) * 3), d->depth, fog, 0);
            }
    }
    #undef VX
    #undef VY
}

static int cmp_draw(const void *a, const void *b) {
    float da = ((const Draw *)a)->depth, db = ((const Draw *)b)->depth;
    return da < db ? 1 : (da > db ? -1 : 0);
}

/* world to screen; 0 if behind the eye */
static int project(float x, float y, float z, float dx, float dy, float horizon, float *sx, float *sy, float *depth) {
    float vx = x - G.px, vy = y - G.py, dp = vx * dx + vy * dy;
    if (dp < 0.15f) return 0;
    *sx = W / 2 + FOCAL * (vx * -dy + vy * dx) / dp;
    *sy = horizon - (z - G.camz) * FOCAL / dp;
    *depth = dp;
    return 1;
}

/* a line through the crypt, hidden by walls; colour from the ramp, lit by nothing (it glows) */
static void line3d(Frame *f, float dx, float dy, float horizon, float x0, float y0, float z0, float x1, float y1, float z1,
                   uint8_t core, uint8_t halo, int dither) {
    float ax, ay, ad, bx, by, bd;
    int a_ok = project(x0, y0, z0, dx, dy, horizon, &ax, &ay, &ad), b_ok = project(x1, y1, z1, dx, dy, horizon, &bx, &by, &bd);
    float len = a_ok && b_ok ? hypotf(bx - ax, by - ay) : 60.0f;
    int n = clampi((int)len + 2, 2, 400);
    for (int i = 0; i <= n; i++) {
        float t = (float)i / n, sx, sy, dep;
        if (!project(lerpf(x0, x1, t), lerpf(y0, y1, t), lerpf(z0, z1, t), dx, dy, horizon, &sx, &sy, &dep)) continue;
        int x = (int)sx, y = (int)sy;
        if (x < 0 || x >= W || y < 0 || y >= H || dep >= zbuf[x]) continue;
        if (dither && BAYER[y & 3][x & 3] < 8) continue;
        f->pix[y * W + x] = core;
        if (halo) {
            if (y > 0 && f->pix[(y - 1) * W + x] != core) f->pix[(y - 1) * W + x] = halo;
            if (y < H - 1 && f->pix[(y + 1) * W + x] != core) f->pix[(y + 1) * W + x] = halo;
        }
    }
}

/* lightning and the soul drain, a moment each; Ionization's trails, fainter, crackling */
static void render_beams(Frame *f, float dx, float dy, float horizon) {
    for (int i = 0; i < MAXTRAILS; i++) {
        const Trail *tr = &G.trails[i];
        if (!tr->alive) continue;
        uint32_t r = (uint32_t)(G.t * 20.0) * 2654435761u + (uint32_t)i * 977u;
        float px = tr->x0, py = tr->y0, pz = 0.42f, ox = -(tr->y1 - tr->y0), oy = tr->x1 - tr->x0, ol = hypotf(ox, oy) + 1e-4f;
        ox /= ol; oy /= ol;
        for (int s = 1; s <= 9; s++) {
            float t = (float)s / 9, j = s == 9 ? 0.0f : (rng_f(&r) - 0.5f) * 0.25f;
            float nx = lerpf(tr->x0, tr->x1, t) + ox * j, ny = lerpf(tr->y0, tr->y1, t) + oy * j, nz = lerpf(0.42f, 0.5f, t) + j * 0.3f;
            if ((rng_next(&r) & 3) != 0) line3d(f, dx, dy, horizon, px, py, pz, nx, ny, nz, (uint8_t)(R_GOLD + 9), (uint8_t)(R_GOLD + 5), 1);
            px = nx; py = ny; pz = nz;
        }
    }
    if (G.pz == PZ_PRISMS && !PK.depth && !G.shop && G.pz_wx >= 0) {   /* the prisms' beam, as far as it goes (1.0.0b13) */
        float bx[32], by[32]; int nb;
        int lit = prism_trace(bx, by, &nb);
        for (int s = 0; s + 1 < nb; s++)
            line3d(f, dx, dy, horizon, bx[s], by[s], 0.42f, bx[s + 1], by[s + 1], 0.42f, lit ? (uint8_t)C_WHITE : (uint8_t)(R_FLAME + 12), (uint8_t)(R_FLAME + 7), 0);
    }
    for (int i = 0; i < MAXBEAMS; i++) {
        const Beam *b = &G.beams[i];
        if (!b->alive) continue;
        float k = 1.0f - b->t / 0.3f;
        if (b->kind == AT_CHAIN) {                                 /* a jagged bolt, re-forked every frame */
            uint32_t r = (uint32_t)(G.t * 30.0) * 2654435761u + (uint32_t)i * 40503u;
            float px = b->x0, py = b->y0, pz = b->z0, ox = -(b->y1 - b->y0), oy = b->x1 - b->x0, ol = hypotf(ox, oy) + 1e-4f;
            ox /= ol; oy /= ol;
            int segs = 7;
            for (int s = 1; s <= segs; s++) {
                float t = (float)s / segs, j = s == segs ? 0.0f : (rng_f(&r) - 0.5f) * 0.45f, jz = s == segs ? 0.0f : (rng_f(&r) - 0.5f) * 0.3f;
                float nx = lerpf(b->x0, b->x1, t) + ox * j, ny = lerpf(b->y0, b->y1, t) + oy * j, nz = lerpf(b->z0, b->z1, t) + jz;
                line3d(f, dx, dy, horizon, px, py, pz, nx, ny, nz, k > 0.4f ? C_WHITE : (uint8_t)(R_GOLD + 12), (uint8_t)(R_GOLD + (k > 0.5f ? 10 : 6)), k < 0.25f);
                px = nx; py = ny; pz = nz;
            }
        } else {                                                   /* the drain: a violet thread, wavering, flowing home */
            float ox = -(b->y1 - b->y0), oy = b->x1 - b->x0, ol = hypotf(ox, oy) + 1e-4f;
            ox /= ol; oy /= ol;
            float px = b->x1, py = b->y1, pz = b->z1;
            for (int s = 1; s <= 12; s++) {
                float t = (float)s / 12, w = sinf(t * 9.0f + b->t * 30.0f) * 0.12f * sinf(t * PI_F);
                float nx = lerpf(b->x1, b->x0, t) + ox * w, ny = lerpf(b->y1, b->y0, t) + oy * w, nz = lerpf(b->z1, b->z0, t) + w * 0.6f;
                line3d(f, dx, dy, horizon, px, py, pz, nx, ny, nz, (uint8_t)(R_PURPLE + 14), (uint8_t)(R_PURPLE + 8), k < 0.3f);
                px = nx; py = ny; pz = nz;
            }
        }
    }
}

/* The merchant, put together each frame: blinking, ears twitching, the tail swishing over the
 * side of the chest, and the eyes following the caster. */
static uint8_t cat_buf[CAT_H * CAT_W];
static void cat_disc(float cx, float cy, float r, int fill, float t) {
    for (int y = (int)(cy - r - 1); y <= (int)(cy + r + 1); y++)
        for (int x = (int)(cx - r - 1); x <= (int)(cx + r + 1); x++) {
            if (x < 0 || y < 0 || x >= CAT_W || y >= CAT_H) continue;
            float dx = x + 0.5f - cx, dy = y + 0.5f - cy;
            if (dx * dx + dy * dy > r * r) continue;
            uint8_t c;
            if (!fill) c = (uint8_t)(R_WARM + 1);
            else {
                int stripe = ((int)(t * 11.0f)) & 1, tip = t > 0.86f;
                float lit = 0.5f - dx / r * 0.35f;
                c = (uint8_t)(R_WARM + clampi((int)((tip ? 3 : (stripe ? 5 : 9)) + lit * 4.0f - 2.0f), 2, 13));
            }
            cat_buf[y * CAT_W + x] = c;
        }
}
static void build_cat(void) {
    memcpy(cat_buf, crawl_cat_img, sizeof cat_buf);
    /* the tail: out of the rump and down over the end of the chest, swishing (more after a sale) */
    float sw = (float)G.t * (1.3f + 2.2f * G.cat_tail_k), amp = 4.0f + 5.0f * G.cat_tail_k;
    float p0x = crawl_cat_tail[0], p0y = crawl_cat_tail[1], p1x = p0x + 12.0f, p1y = p0y + 1.0f;
    float p2x = p0x + 10.0f + amp * sinf(sw), p2y = p0y + 31.0f - fabsf(amp * sinf(sw)) * 0.4f;
    for (int pass = 0; pass < 2; pass++)
        for (int i = 0; i <= 48; i++) {
            float t = i / 48.0f, u = 1.0f - t;
            float bx = u * u * p0x + 2 * u * t * p1x + t * t * p2x, by = u * u * p0y + 2 * u * t * p1y + t * t * p2y;
            float r = 3.4f - 1.5f * t;
            cat_disc(bx, by, pass ? r : r + 1.0f, pass, t);
        }
    /* an ear flicks */
    if (G.cat_ear > 0.0f) {
        const uint8_t *e = crawl_cat_ear[G.cat_ear_side];
        float k = sinf(PI_F * (0.32f - G.cat_ear) / 0.32f);
        int dir = G.cat_ear_side ? 1 : -1;
        for (int y = e[1]; y <= e[3]; y++) {
            int sh = (int)lroundf(k * 2.4f * (float)(e[3] - y) / (float)(e[3] - e[1])) * dir;
            for (int x = e[0]; x <= e[2]; x++) {
                int sx = x - sh;
                cat_buf[y * CAT_W + x] = sx >= e[0] && sx <= e[2] ? crawl_cat_img[y * CAT_W + sx] : C_CLEAR;
            }
        }
    }
    /* the eyes: green, slit pupils on the caster, lids for a blink or a slow, fond one */
    float lid = 0.0f;
    if (G.cat_blink > 0.0f) lid = sinf(PI_F * (0.16f - G.cat_blink) / 0.16f);
    if (G.cat_slow > 0.0f && G.cat_slow <= 1.4f) { float u = 1.4f - G.cat_slow; lid = fmaxf(lid, u < 0.45f ? u / 0.45f : (u < 0.85f ? 1.0f : 1.0f - (u - 0.85f) / 0.55f)); }
    int look = (int)lroundf(G.cat_look);
    for (int e = 0; e < 2; e++) {
        int cx = crawl_cat_eye[e][0], cy = crawl_cat_eye[e][1], rx = crawl_cat_eye[e][2], ry = crawl_cat_eye[e][3];
        int shut = (int)(lid * (2 * ry + 1) + 0.5f);
        for (int y = cy - ry; y <= cy + ry; y++)
            for (int x = cx - rx; x <= cx + rx; x++) {
                float ux = (x - cx) / (rx + 0.5f), uy = (y - cy) / (ry + 0.5f);
                if (ux * ux + uy * uy > 1.0f) continue;
                int row = y - (cy - ry);
                uint8_t c;
                if (row < shut) c = row == shut - 1 ? crawl_cat_lid[1] : crawl_cat_lid[0];
                else {
                    c = row == 0 ? crawl_cat_eyecol[0] : (row <= ry ? crawl_cat_eyecol[1] : crawl_cat_eyecol[2]);
                    if (x == cx + look) c = crawl_cat_eyecol[3];
                    if (x == cx - 1 + look && y == cy - 1) c = crawl_cat_eyecol[4];
                }
                cat_buf[y * CAT_W + x] = c;
            }
    }
}

static int spr_bottom(int spr) {                                    /* lowest drawn row of a sprite */
    static int8_t cache[NSPR];
    static int init;
    if (!init) { memset(cache, -1, sizeof cache); init = 1; }
    if (cache[spr] < 0) {
        int b = 0;
        for (int x = 0; x < TS; x++) for (int y = 0; y < TS; y++) if (crawl_spr[spr][x * TS + y] != C_CLEAR && y > b) b = y;
        cache[spr] = (int8_t)b;
    }
    return cache[spr];
}

static int key_sprite(int color, float ph) {
    static const int BASE[NKEYS] = { SP_KEY0, SP_KEYS0, SP_KEYC0 };
    return BASE[color] + (((int)(ph * 5.0f)) & 3);
}

static int mob_sprite(const Mob *m, float *scale) {
    if ((m->webbed > 0.0f || (m->ally && m->claw > 0.0f)) && m->state == MS_WALK) {   /* held still, clawing now and then; or one of
                                                                                        yours, striking */
        Mob c = *m;
        c.webbed = 0.0f; c.ally = 0;
        if (m->webbed > 0.0f) c.anim = 0.0f;
        c.swing = m->claw > 0.0f ? 0.2f : 0.0f; c.throw_t = m->claw > 0.0f && m->type == MOB_SKEL ? 0.2f : 0.0f;
        return mob_sprite(&c, scale);
    }
    int fr = ((int)m->anim) & 1;
    *scale = KINDS[m->type].scale;
    switch (m->type) {
    case MOB_SHAMBLER:
        if (m->state == MS_DIE) return m->t < 0.35f ? SP_SHAMB_D1 : SP_SHAMB_D2;
        if (m->sub == 2) return m->sub_t < 0.8f ? SP_SHAMB_RISE1 : SP_SHAMB_RISE2;
        return m->swing > 0.0f ? SP_SHAMB_GRAB : (fr ? SP_SHAMB_B : SP_SHAMB_A);
    case MOB_DROWNED:
        if (m->state == MS_DIE) return SP_DROWN_BURST;
        if (m->sub == 2) return SP_DROWN_RISE;
        if (m->c4 == C4_SWELL) return SP_DROWN_SWELL;               /* C4: the spew */
        if (m->c4 == C4_SPEW) return SP_DROWN_SPEW;
        return m->swing > 0.0f ? SP_DROWN_GRAB : (fr ? SP_DROWN_B : SP_DROWN_A);
    case MOB_HOUND:
        if (m->state == MS_DIE) return SP_HOUND_D;
        return m->swing > 0.0f ? SP_HOUND_BITE : (fr ? SP_HOUND_B : SP_HOUND_A);
    case MOB_HUNTER:
        if (m->state == MS_DIE) return m->boss ? SP_BSPIDER_D : SP_HUNTER_D;   /* the Wandering Spider has its own */
        if (m->boss) return m->sub == 3 ? SP_BSPIDER_LEAP : (G.web_rear > 0.0f ? SP_BSPIDER_REAR : (((int)(m->t * 10.0f)) & 1 ? SP_BSPIDER_B : SP_BSPIDER_A));
        return m->sub == 3 ? SP_HUNTER_LEAP : (((int)(m->t * 10.0f)) & 1 ? SP_HUNTER_B : SP_HUNTER_A);
    case MOB_GHOUL:
        if (m->state == MS_DIE) return SP_GHOUL_D;
        return m->sub == 3 ? SP_GHOUL_LEAP : (m->sub == 4 ? SP_GHOUL_FEED : (fr ? SP_GHOUL_B : SP_GHOUL_A));
    case MOB_KNIGHT: {
        if (m->state == MS_DIE) return SP_KNIGHT_D;
        int base = (m->boss ? SP_BKNIGHT_A : SP_KNIGHT_A) + knight_stage(m) * 4;   /* the Ill-Fated Knight has its own */
        if (m->boss && G.bpose_t > 0.0f && G.bpose != BP_NONE && m->state == MS_WALK)   /* the Ill-Fated Knight casting */
            return G.bpose == BP_PLANT ? SP_BKNIGHT_P0 + knight_stage(m) : base + (G.bpose == BP_RAISE ? 2 : 3);
        return base + (m->swing > 0.0f ? 2 : (m->throw_t > 0.0f ? 3 : fr));
    }
    case MOB_BROOD:
        if (m->state == MS_DIE) return SP_BROOD_D;
        return m->swing > 0.0f ? SP_BROOD_LAY : (fr ? SP_BROOD_B : SP_BROOD_A);
    case MOB_LING: return ((int)(m->t * 12.0f)) & 1 ? SP_LING_B : SP_LING_A;
    case MOB_SHADE:
        if (m->state == MS_DIE) return SP_SHADE_D;
        return m->swing > 0.0f ? SP_SHADE_TOUCH : (fr ? SP_SHADE_B : SP_SHADE_A);
    case MOB_IMP:
        if (m->state == MS_DIE) return SP_IMP_D;
        return m->throw_t > 0.0f ? SP_IMP_THROW : (((int)(m->t * 8.0f)) & 1 ? SP_IMP_B : SP_IMP_A);
    case MOB_DEACON:
        if (m->state == MS_DIE) return m->boss ? SP_BDEACON_D : SP_DEACON_D;   /* the Black Deacon has his own */
        if (m->boss) return (G.bpose == BP_LITANY && G.bpose_t > 0.0f) || m->swing > 0.0f ? SP_BDEACON_CHANT : (fr ? SP_BDEACON_B : SP_BDEACON_A);
        return m->swing > 0.0f ? SP_DEACON_CHANT : (fr ? SP_DEACON_B : SP_DEACON_A);
    case MOB_GARGOYLE:
        if (m->state == MS_DIE) return SP_GARG_D;
        if (m->sub == 0 || (m->sub == 1 && m->sub_t < 0.4f && ((int)(m->sub_t * 20.0f) & 1))) return SP_GARG_PERCH;
        return m->sub == 3 || m->swing > 0.0f ? SP_GARG_SWOOP : (((int)(m->t * 7.0f)) & 1 ? SP_GARG_B : SP_GARG_A);
    case MOB_WHIPPER:
        if (m->state == MS_DIE) return SP_WHIP_D;
        if (m->call_t > 0.0f) return SP_WHIP_HORN;                 /* the horn to his jaw */
        return m->swing > 0.0f || m->throw_t > 0.0f ? SP_WHIP_CRACK : (fr ? SP_WHIP_B : SP_WHIP_A);
    case MOB_SEXTON:
        if (m->state == MS_DIE) return SP_SEXTON_D;
        if (m->ice_t > 0.0f) return SP_SEXTON_ICE;                 /* the bell frozen */
        if (m->ring_t > 0.0f) return m->rings ? (((int)(m->ring_t * 9.0f)) & 1 ? SP_SEXTON_RING_B : SP_SEXTON_RING_A) : SP_SEXTON_RING_B;
        return fr ? SP_SEXTON_B : SP_SEXTON_A;
    case MOB_LIGHTLESS:
        if (m->state == MS_DIE) return SP_LIGHT_D;
        if (m->swing > 0.0f) return SP_LIGHT_STRIKE;
        if (m->wait > 0.0f && m->alert <= 0.0f && m->aware != MA_CALM) return SP_LIGHT_LISTEN;   /* lost in the dark, listening */
        return fr ? SP_LIGHT_B : SP_LIGHT_A;
    case MOB_RAT:                                                  /* the charnel rat (docs/PLAN-mice.md) */
        if (m->state == MS_DIE) return SP_RAT_D;
        if (m->swing > 0.0f) return SP_RAT_BITE;
        if (m->told_t > 0.0f) return SP_RAT_REAR;                   /* told: up on its haunches, squealing */
        return ((int)(m->t * 9.0f)) & 1 ? SP_RAT_B : SP_RAT_A;
    case MOB_SAC: return m->state == MS_DIE ? SP_SAC_OPEN : (m->sub_t > 5.0f && ((int)(m->t * 6.0f) & 1) ? SP_SAC_B : SP_SAC_A);
    case MOB_WRAITH:
        if (m->sub == 1) return SP_WRAITH_BACK;
        return hypotf(m->x - G.px, m->y - G.py) < 1.8f ? SP_WRAITH_REACH : (fr ? SP_WRAITH_B : SP_WRAITH_A);
    case MOB_SKEL:
        if (m->state == MS_WALK && m->sub == 5) return m->sub_t < 0.35f ? SP_SKEL_D2 : SP_SKEL_D1;   /* getting up */
        return m->state == MS_DIE ? (m->t < 0.25f ? SP_SKEL_D1 : (m->t < 0.5f ? SP_SKEL_D2 : SP_SKEL_D3))
                                  : (m->throw_t > 0.0f ? SP_SKEL_T : (fr ? SP_SKEL_B : SP_SKEL_A));
    case MOB_SHIELD: {
        if (m->state == MS_DIE) return m->t < 0.25f ? SP_SKEL_D1 : (m->t < 0.5f ? SP_SKEL_D2 : SP_SKEL_D3);
        int o = m->shield > 0.0f ? 0 : SP_SHIELD_XA - SP_SHIELD_A;
        if (m->shield > 0.0f && m->state == MS_WALK && m->c4 == C4_STAMP) return SP_SHIELD_RAISE;   /* C4: the charge */
        if (m->shield > 0.0f && m->state == MS_WALK && m->c4 == C4_RUSH) return SP_SHIELD_RUSH;
        return o + (m->swing > 0.0f ? SP_SHIELD_S : (fr ? SP_SHIELD_B : SP_SHIELD_A));
    }
    case MOB_GHOST:
        *scale = 1.35f;                                            /* taller than a man */
        return m->state == MS_WALK ? (fr ? SP_GHOST_B : SP_GHOST_A) : (((int)(m->t * 14.0f)) & 1 ? SP_GHOST_D1 : SP_GHOST_D2);
    case MOB_WSKULL:
        *scale = 0.62f;
        if (m->state == MS_DIE) return SP_WSKULL_D;
        if (m->sub == 1) return m->sub_t > 2.2f && ((int)(m->sub_t * 16.0f) & 1) ? SP_WSKULL2 : (m->sub_t > 0.5f ? SP_WSKULL1 : SP_WSKULL0);
        return SP_WSKULL0;
    case MOB_SPIDER:
        *scale = 0.42f;
        if (m->state == MS_DIE) return SP_SPIDER_D;
        if (m->sub == 2) return SP_SPIDER_BITE;
        return ((int)(m->t * 10.0f)) & 1 ? SP_SPIDER_B : SP_SPIDER_A;
    default:
        if (m->state == MS_WALK && m->sleep == 1) {                /* a roost (1.0.0b18): asleep in its wings; stirring when you are near or
                                                                      it is called */
            const Pack *p = m->pack ? &G.packs[m->pack - 1] : NULL;
            int stir = (p && p->state == PAS_CALL) || hypotf(G.px - m->x, G.py - m->y) < 5.0f || ((int)(G.t * 0.7 + m->x * 3.0f) % 9) == 0;
            return stir ? SP_BAT_ROOST2 : SP_BAT_ROOST;
        }
        return m->state == MS_DIE ? SP_BAT_D : ((int)(m->t * 9.0f) & 1 ? SP_BAT_B : SP_BAT_A);
    }
}

/* S1: a creature looking across your line of sight shows its side, turned to the way it faces; one looking away shows
 * its side too, until there are backs to show (docs/PLAN-ai.md, section 1). Only its walk: a blow or a cast faces you. */
static int is_back(int s) {                                        /* 1.0.0b18: a back view: no eyes, the vapour from the head */
    return (s >= SP_SKEL_BKA && s <= SP_SHIELD_BKB) || (s >= SP_KNIGHT_BKA && s <= SP_GARG_BKB) || (s >= SP_HUNTER_BKA && s <= SP_BROOD_BKB)
        || s == SP_RAT_BKA || s == SP_RAT_BKB || s == SP_MOUSE_BK;
}
static int mob_side(const Mob *m, int spr, int *flip) {
    *flip = 0;
    if (!minded(m) || !has_facing(m)) return spr;
    const Kind *k = &KINDS[m->type];
    if (m->type == MOB_KNIGHT && spr >= SP_KNIGHT_A && spr <= SP_KNIGHT3_S && (spr - SP_KNIGHT_A) % 4 < 2   /* any plate, walking */
        && fabsf(wrap_pi(m->face - atan2f(G.py - m->y, G.px - m->x))) > 2.36f)
        return SP_KNIGHT_BKA + (spr - SP_KNIGHT_A) / 4 * 2 + (spr - SP_KNIGHT_A) % 4;
    if (k->side < 0 || (spr != k->spr && spr != k->spr2) || (m->type == MOB_HUNTER && m->sub == 0)) return spr;
    float off = fabsf(wrap_pi(m->face - atan2f(G.py - m->y, G.px - m->x)));
    if (off < 0.79f) return spr;                                   /* facing you, near enough */
    if (off > 2.36f) {                                             /* its back to you (1.0.0b18): where a back is drawn */
        if (m->type == MOB_SKEL) return spr == k->spr2 ? SP_SKEL_BKB : SP_SKEL_BKA;
        if (m->type == MOB_SHIELD) return spr == k->spr2 ? SP_SHIELD_BKB : SP_SHIELD_BKA;
        if (m->type == MOB_WHIPPER) return spr == k->spr2 ? SP_WHIP_BKB : SP_WHIP_BKA;
        if (m->type == MOB_DEACON) return spr == k->spr2 ? SP_DEACON_BKB : SP_DEACON_BKA;
        if (m->type == MOB_SEXTON) return spr == k->spr2 ? SP_SEXTON_BKB : SP_SEXTON_BKA;
        if (m->type == MOB_SHAMBLER) return spr == k->spr2 ? SP_SHAMB_BKB : SP_SHAMB_BKA;
        if (m->type == MOB_DROWNED) return spr == k->spr2 ? SP_DROWN_BKB : SP_DROWN_BKA;
        if (m->type == MOB_GHOUL) return spr == k->spr2 ? SP_GHOUL_BKB : SP_GHOUL_BKA;
        if (m->type == MOB_HOUND) return spr == k->spr2 ? SP_HOUND_BKB : SP_HOUND_BKA;
        if (m->type == MOB_IMP) return spr == k->spr2 ? SP_IMP_BKB : SP_IMP_BKA;
        if (m->type == MOB_GARGOYLE) return spr == k->spr2 ? SP_GARG_BKB : SP_GARG_BKA;
        if (m->type == MOB_HUNTER) return spr == k->spr2 ? SP_HUNTER_BKB : SP_HUNTER_BKA;
        if (m->type == MOB_BROOD) return spr == k->spr2 ? SP_BROOD_BKB : SP_BROOD_BKA;
        if (m->type == MOB_RAT) return spr == k->spr2 ? SP_RAT_BKB : SP_RAT_BKA;
    }
    *flip = sinf(m->face - G.ang) < 0.0f;
    return spr == k->spr2 ? k->side2 : k->side;
}

static int shot_sprite(const Shot *s, float *scale) {
    int fl = ((int)(G.t * 14.0)) & 1;
    *scale = 1.0f;
    switch (s->kind) {
    case AT_FROST: *scale = 0.7f; return SP_FROST;
    case AT_SPEAR: *scale = 0.8f; return SP_SPEAR;
    case AT_HELL: *scale = 1.3f; return fl ? SP_HELL_B : SP_HELL_A;
    case AT_WISPS: *scale = 0.6f; return SP_WISP;
    case AT_FLASK: *scale = 0.4f; return SP_FLASK;
    case SHOT_REFLECT: return s->z > 0.5f ? (fl ? SP_SKULLFIRE_B : SP_SKULLFIRE_A) : SP_BONEFLY0 + (((int)(G.t * 14.0)) & 3);
    case SHOT_WEB: *scale = 0.3f; return SP_SAC_A;                 /* a glob of web */
    case SHOT_ORB: *scale = 0.8f; return fl ? SP_ORB_B : SP_ORB_A;
    case SHOT_LANCE: *scale = 0.9f; return SP_LANCE;
    case SHOT_BALLISTA: *scale = 0.9f; return SP_BALLISTA;
    case SHOT_SPLINTER: *scale = 0.45f; return SP_SPEAR;
    default: return fl ? SP_FIRE_B : SP_FIRE_A;
    }
}

/* the Backrooms' glitch: in bursts, bands of rows torn sideways, and some of them slipping colour,
 * a red copy one way and a blue the other; 0 when it is still. Creatures caught in a loop glitch most. */
static int glitch_rows(const Draw *d, int8_t *shift, uint8_t *slip) {
    uint32_t seed = (uint32_t)(int)(d->x * 7.0f) * 73856093u ^ (uint32_t)(int)(d->y * 7.0f) * 19349663u;
    int slot = (int)(G.t * 12.0);
    if (hash2(slot / 6, (int)(seed & 0xFFFF)) % 100 >= ((d->flags & DR_LOOPED) ? 70u : 35u)) return 0;
    for (int y = 0; y < TS;) {
        uint32_t h = hash2(slot, (int)(seed % 9973u) + y * 31);
        int band = 2 + (int)(h % 6), dx = (h >> 8) % 3 == 0 ? (int)((h >> 12) % 9) - 4 : 0, sl = (h >> 20) % 4 == 0;
        for (int k = 0; k < band && y < TS; k++, y++) { shift[y] = (int8_t)dx; slip[y] = (uint8_t)sl; }
    }
    return 1;
}

/* the look a decoration is drawn in: a prop with other looks takes one by where it stands (crawl_alt) */
static int deco_look(const Deco *dc) {
    int look = dc->spr;
    if (crawl_alt[look][1] >= 0) {
        int na = 1;
        while (na < NALT && crawl_alt[look][na] >= 0) na++;
        float hx = dc->x, hy = dc->y;
        if (foot_of((int)hx, (int)hy)) { hx = floorf(hx) + 0.5f; hy = floorf(hy) + 0.5f; }   /* a prop in its part: the look its cell gave it */
        look = crawl_alt[look][hash2((int)(hx * 8.0f), (int)(hy * 8.0f)) % (uint32_t)na];
    }
    return look;
}
static void render_sprites(Frame *f, float dx, float dy, float horizon) {
    static Draw list[MAXDECO + MAXMOBS + MAXSHOTS + MAXBOOMS + MAXBONES + MAXITEMS + MAXBURNS + MAXPOOLS + MAXHAZ * 4 + MAXSITE * 4 + 16 + DFIRE_CELLS * 2
                     + MAXMICE];   /* and wisps, a double, after-images; the mice */
    int n = 0;
    float rx = -dy, ry = dx;
    #define ADD(X, Y, Z, S, GH) do { float vx_ = (X) - G.px, vy_ = (Y) - G.py, dp_ = vx_ * dx + vy_ * dy; \
        if (dp_ > 0.12f && dp_ < 24.0f && n < (int)(sizeof list / sizeof list[0])) list[n++] = (Draw){ X, Y, Z, dp_, S, (GH) > 0.0f ? (GH) : 1.0f, 0.0f, 1.0f, 0, 0, 0.0f, NULL, 0, 0.0f, 0.0f, 0, 0, 0, 0, 0, NULL, 0.0f, 0, 0, 0 }; } while (0)
    for (int i = 0; i < G.ndecos; i++) {
        const Deco *dc = &G.decos[i];
        static const float FPS[4] = { 0.0f, 0.0f, 3.0f, 9.0f };    /* by frame count: candles flicker, fire roars */
        if (dc->spr < 0) continue;                                 /* gone: risen, or never a statue */
        int look = deco_look(dc);
        int spr = look + (dc->nfr > 1 ? (int)((float)G.t * FPS[dc->nfr & 3] + dc->phase) % dc->nfr : 0), n0 = n;
        float zx = 0.0f, ox = 0.0f, oy = 0.0f;
        if (dc->flags & (DF_FLIP | DF_FLOAT | DF_SINK | DF_LOOP)) {   /* out of place */
            if (dc->flags & DF_FLOAT) zx = 0.40f + 0.05f * sinf((float)G.t * 1.3f + dc->phase);
            if (dc->flags & DF_SINK) zx = (dc->flags & DF_FLIP) ? 0.25f : -0.22f;   /* most things fill the lower half of their picture */
            else if (dc->flags & DF_FLIP) zx = 0.0f;
            if (dc->flags & DF_LOOP) {                             /* a few steps forward, and back to where it began */
                float u = (float)G.t * 0.7f + dc->phase, k = 0.5f * (u - floorf(u)) - 0.25f;
                if ((int)dc->phase & 1) ox = k; else oy = k;
            }
        }
        ADD(dc->x + ox, dc->y + oy, zx, spr, 0);
        if (n > n0 && (dc->flags & (DF_FLIP | DF_FLOAT | DF_SINK | DF_LOOP))) {
            list[n - 1].flags = ((dc->flags & DF_FLIP) ? DR_FLIPV : 0) | ((dc->flags & DF_SINK) ? DR_CLIP : 0) | ((dc->flags & DF_LOOP) ? DR_GLITCH | DR_LOOPED : 0);
        }
        if (n > n0 && (dc->flags & DF_SWAY)) {                     /* a slow swing, and a shiver on top of it */
            list[n - 1].flags = DR_SWAY;
            list[n - 1].sway = 2.6f * sinf((float)G.t * 0.9f + dc->phase) + 0.6f * sinf((float)G.t * 2.3f + dc->phase * 1.7f);
        }
        if (n > n0 && i == G.msg_deco) {                           /* what holds the hidden message (1.0.0b13): its glint; the shrine's showing */
            list[n - 1].mg = msg_glint(&list[n - 1].mgw);
            float sh = msg_shown();
            if (sh > 0.0f && (sh >= 1.0f || fmodf((float)G.t * 3.0f, 1.0f) < sh)) { list[n - 1].flags |= DR_GLINT; list[n - 1].glint = R_PURPLE; }
        }
    }
    if (!G.shop && !PK.depth && G.bsh_x >= 0) {                    /* the Black Shrine's eyes: faint, or flaring while it shows */
        int n0 = n;
        ADD(G.bsh_x + 0.5f, G.bsh_y + 0.5f, 0.0f, G.bsh_flare > 0.0f || msg_shown() > 0.0f ? SP_BSHRINE_E1 : SP_BSHRINE_E0, 0);
        if (n > n0) { list[n - 1].flags = DR_BRIGHT; list[n - 1].depth -= 0.02f; }
    }
    if (!G.shop && G.altar_x >= 0 && (G.altar_state == ALTAR_AWAKE || G.altar_state == ALTAR_READY || G.blessed)) {   /* the altar's fire, at full brightness */
        int n0 = n, fr = ((int)(G.t * 8.0)) & 1;
        ADD(G.altar_x + 0.5f, G.altar_y + 0.5f, 0.0f, (G.altar_state == ALTAR_READY ? SP_BFLAME_G0 : SP_BFLAME_V0) + fr, 0);
        if (n > n0) { list[n - 1].flags = DR_BRIGHT | (G.blessed ? DR_EDGE : 0); list[n - 1].depth -= 0.02f; }
    }
    if (G.shop && RUN.shrine > 0) {                                /* the cursed shrine: its glow in its rank's colour, its runes going round */
        int remap = QUAL_RAMP[RUN.shrine - 1], n0 = n, kinds[NDEF], nk = 0;
        ADD(SHRINE_X, SHRINE_Y, 0.0f, ((int)(G.t * 3.0)) & 1 ? SP_SHRINE_G1 : SP_SHRINE_G0, 0);
        if (n > n0) { list[n - 1].flags = DR_BRIGHT; list[n - 1].remap = remap; list[n - 1].depth -= 0.02f; }
        for (int k = 0; k < NDEF; k++) if (RUN.attuned >> k & 1) kinds[nk++] = k;
        for (int j = 0; j < (nk ? nk : 1); j++) {
            float a = (float)G.t * 0.7f + (float)j * TAU_F / (float)(nk ? nk : 1);
            int n1 = n;
            ADD(SHRINE_X + cosf(a) * 0.62f, SHRINE_Y - 0.14f + sinf(a) * 0.28f, 0.46f + 0.05f * sinf((float)G.t * 2.0f + (float)j),
                nk ? SP_SRUNE0 + kinds[j] : SP_SRUNE_X, 0);
            if (n > n1) { list[n - 1].flags = DR_BRIGHT; list[n - 1].remap = remap; list[n - 1].scale = 0.2f; }
        }
    }
    if (G.decoy_t > 0.0f && !G.shop) {                             /* an Exquisite Blink's double, fading where you stood */
        int n0 = n;
        ADD(G.decoy_x, G.decoy_y, 0.0f, SP_DOUBLE, fminf(0.8f, 0.15f + G.decoy_t / DECOY_T));
        if (n > n0) list[n - 1].flags = DR_BRIGHT;
    }
    for (int i = 0; i < MAXMOBS; i++) {
        Mob *m = &G.mobs[i];
        if (!m->alive || buried(m)) continue;
        float sc;
        int s = mob_sprite(m, &sc), n0 = n, statue = m->type == MOB_GARGOYLE && m->sub == 0, flipH = 0;
        s = mob_side(m, s, &flipH);
        float gh = m->type == MOB_GHOST ? (m->state == MS_RECOIL ? 0.9f : 0.72f * (1.0f - m->fade) + 0.01f) : (m->type == MOB_SHADE ? shade_alpha(m) : 0.0f);
        int commune = G.commune_show > 0.0f && m->state == MS_WALK && hypotf(m->x - G.px, m->y - G.py) <= G.commune_r;
        if (m->type == MOB_SHADE && gh < 0.04f && !commune) continue;
        if (m->boss == 1 + BOSS_WRAITH && m->state == MS_WALK) {   /* into the stone and out; faint in the dark */
            float a = G.bww == 1 ? 1.0f - G.bww_t / 0.6f : (G.bww == 2 ? 0.0f : (G.bww == 3 ? G.bww_t / 0.5f : 1.0f));
            if (!wraith_lit(m->x, m->y)) a *= 0.35f;
            if (a < 0.03f) continue;
            gh = a;
        }
        ADD(m->x, m->y, m->z, s, commune && gh > 0.0f && gh < 0.04f ? 0.04f : gh);
        if (n == n0) continue;
        Draw *d = &list[n - 1];
        d->scale = statue ? 1.0f : sc * RANK_SIZE[m->rank];       /* a statue like the others: its rank doesn't show yet */
        int waits = m->boss && G.bstate != BS_FIGHT && m->state == MS_WALK;   /* untouchable: a pale shell */
#ifdef CRAWL_DEBUG
        if (g_stage >= 0) waits = 0;                                /* a boss stood for its frames shows as in the fight */
#endif
        if (m->boss) { d->scale *= BOSSES[m->boss - 1].scale; if (waits) d->flags |= DR_EDGE; }
        if (m->small) d->scale *= 0.62f;
        if (m->rank != RK_I && KINDS[m->type].family >= 0 && !statue) d->lut = crawl_rank_lut[KINDS[m->type].family][m->rank];
        if (m->type == MOB_SPIDER && m->state == MS_WALK && m->sub == 0) {
            d->flags = DR_EYES;
            if (m->elem > 0 && m->elem <= EL_PHYS) d->ecol = 1 + EC_EL0 + m->elem - 1;   /* born to an element: its points in its colour */
        }
        if (m->type == MOB_HUNTER && m->state == MS_WALK && m->sub == 0) d->flags = DR_FLIPV;
        if (m->type == MOB_BAT && m->state == MS_WALK && m->sleep == 1) d->z -= 0.33f * d->scale;   /* a roost, asleep: drawn as it hangs,
                                                                      its feet at the ceiling */
        if (m->frozen > 0.0f && m->state == MS_WALK) d->flags |= DR_ICE;   /* only its eyes, in the dark of the web */
        else if (m->chl_n > 0 && m->state == MS_WALK) d->flags |= (m->chl_n >= 2 ? DR_CHILLA : 0) | (m->chl_n != 2 ? DR_CHILLB : 0);   /* Chilled,
                                                                      bluer by its stacks */
        if (G.ptheme == PT_BACKROOMS) d->flags |= DR_GLITCH;           /* nothing here is quite right */
        if (G.ptheme == PT_SEVERANCE) d->flags |= DR_LIT;              /* nor here: everything is too clean */
        if (flipH) d->flags |= DR_FLIPH;
        if (waits) d->flags |= DR_EDGE;
        if (m->state == MS_WALK) d->flags |= m->c_flags;           /* the contract: what track C's attacks ask (C4) */
        if (m->boss == 1 + BOSS_KNIGHT && m->state == MS_WALK) {
            d->flags |= DR_SHINE;
            if (G.bdash_age < 0.8f) {                              /* Swift Dodge: where it was a moment ago, fading */
                Draw keep = *d;
                for (int j = 0; j < 3; j++) {
                    float f = (float)j / 3.0f, a = (0.5f - 0.12f * (float)j) * (1.0f - G.bdash_age / 0.8f);
                    int n1 = n;
                    if (a > 0.04f) ADD(G.bdash_x0 + (G.bdash_x1 - G.bdash_x0) * f, G.bdash_y0 + (G.bdash_y1 - G.bdash_y0) * f, m->z, s, a);
                    if (n > n1) { list[n - 1].scale = keep.scale; list[n - 1].lut = keep.lut; }
                }
                d = &list[n0];                                     /* the list may have moved on: the knight is still at n0 */
            }
        }
        if (m->boss == 1 + BOSS_SPIDER && G.bmend_t > 0.0f && m->state == MS_WALK) d->flags |= DR_MEND;   /* its Mend Rune */
        if (m->pois_t > 0.0f && m->state == MS_WALK) d->flags |= DR_POISONED;
        if (m->state == MS_WALK) {                                 /* its strengths shown: a hard rim of their colour */
            int a = strength_shown(m, 0) ? m->ward[0] : 0, b = strength_shown(m, 1) ? m->ward[1] : 0;
            if (!a) { a = b; b = 0; }
            if (a) d->ward = a + (m->wring > 0.0f ? 16 : 0) + 32 * b;
        }
        if (m->pin > 0.0f && m->state == MS_WALK) d->flags |= DR_PINNED;
        if (m->burn_t > 0.0f && m->state == MS_WALK) d->flags |= DR_BURNING;
        if ((m->bld_t > 0.0f || aug_show(1)) && m->state == MS_WALK) d->flags |= DR_BLEED;   /* Barbed's bleed (D7) */
        if (m->boss == 1 + BOSS_WRAITH && G.bslip_t > 0.9f) d->flags |= DR_RIPPLE;
        if (m->boss == 1 + BOSS_DEACON) {
            if (G.bpose == BP_LITANY && G.bpose_t > 0.0f && m->state == MS_WALK) d->flags |= DR_MEND;   /* the Litany's green light */
            if (G.bward > 0.0f && G.bstate == BS_FIGHT && m->state == MS_WALK) {   /* his Bone Ward, wheeling about him */
                for (int k = 0; k < 6; k++) {
                    float a = (float)G.t * 2.2f + (float)k * (TAU_F / 6.0f);
                    int n1 = n;
                    ADD(m->x + cosf(a) * 0.9f, m->y + sinf(a) * 0.9f, 0.25f + 0.15f * sinf(a * 2.0f), SP_BONEFLY0 + (k & 3), 0);
                    if (n > n1) list[n - 1].scale = 0.55f;
                }
                d = &list[n0];
            }
        }
        if (m->ally && m->state == MS_WALK) {                      /* yours: a violet edge, crumbling as its time runs out */
            d->flags |= DR_ALLY | (m->z < -0.01f ? DR_CLIP : 0);
            if (m->ally_t < 2.0f) d->alpha = fmaxf(0.2f, m->ally_t / 2.0f);
        }
        if (m->webbed > 0.0f && m->state == MS_WALK) { d->flags |= DR_WEBBED; d->web = m->webbed / fmaxf(1.0f, m->webmax); }
        if (commune) d->flags = (d->flags & ~DR_EYES) | DR_COMMUNE;   /* Commune: its shape, wherever it is */
        int backed = is_back(s);                                   /* a back: no eyes, but its dressing and vapour */
        if (m->state == MS_WALK && !statue && m->type != MOB_SHADE && (crawl_eyes[s].n || backed) && !(d->flags & (DR_EYES | DR_IMPLODE))) {
            d->eye = !backed; d->etype = m->type; d->erank = m->boss ? RK_V : m->rank;   /* its eyes, after it (render_eyes) */
            if (m->boss == 1 + BOSS_SPIDER) d->ecol = 1 + EC_FIRE;     /* her two great eyes keep their fire (the owner's word) */
            else if (m->elem > 0 && m->elem <= EL_PHYS) d->ecol = 1 + EC_EL0 + m->elem - 1;   /* born to an element: its colour */
            int dx = crawl_dress_ix[s][m->rank];
            if (m->rank != RK_I && dx >= 0 && !m->boss) d->ov = crawl_dress[dx];   /* its rank's dressing */
            if (!m->boss && m->rank >= RK_III && !KIND_IS(m, KF_SPIRIT)) d->vap = 1.0f;   /* and its spirit, showing */
            d->estate = m->type == MOB_WSKULL ? (m->sub == 0 ? 3 : 2) : (m->swing > 0.0f || m->throw_t > 0.0f ? 2 : (m->alert ? 1 : 0));
            if (m->elem && m->elem_on && d->estate != 3) d->estate = 4;   /* its attack carries its element: they flare in it */
#ifdef CRAWL_DEBUG
            if (m->elem && getenv("CRAWL_EYEFLARE")) d->estate = 4;   /* for the sheet */
#endif
            if (m->unb_t > 0.0f && d->estate != 3) d->estate = 5;   /* Unbound: its eyes gone grey, its element out of them */
        } else if (m->state == MS_WALK && m->type == MOB_LIGHTLESS && !(d->flags & (DR_EYES | DR_IMPLODE))) {   /* no eyes, but its rank's marks */
            int dx = crawl_dress_ix[s][m->rank];
            if (m->rank != RK_I && dx >= 0) d->ov = crawl_dress[dx];
        }
        if (m->state == MS_DIE && m->sub == 77) {                  /* drawn in to a point */
            float k = smoothstepf(0.1f, 0.75f, m->t);
            d->z += sc * 0.5f * k; d->scale = sc * (1.0f - k) + 0.02f; d->flags = DR_IMPLODE;
        }
    }
    for (int i = 0; i < G.nmice && !PK.depth && !G.shop; i++) {    /* the mice (docs/PLAN-mice.md): by what they do and which way */
        const Mouse *o = &G.mice[i];                               /* they face you; their eyes as every creature's */
        if (!o->alive || o->st == MO_WALL) continue;
        float sc = crawl_mouse_box(G.mstage);
        if (o->st == MO_PEEK) {                                    /* only its eyes, in the hole's mouth */
            const MHole *mh = &G.mholes[o->out];
            int w, hg, n0 = n;
            crawl_mhole_size(G.level >= 13 && G.mstage == crawl_mouse_stage(G.level) ? crawl_mhole_stage(G.level) : G.mstage, &w, &hg);
            float es = 0.0583f * (float)w, ez = (float)hg * 0.45f / TS - 0.5234f * es;
            ADD(mh->mx + DX[mh->d] * 0.03f, mh->my + DY[mh->d] * 0.03f, ez, SP_MOUSE_F, 0);
            if (n > n0) { list[n - 1].scale = es; list[n - 1].flags = DR_EYES; list[n - 1].ecol = 1 + EC_GREEN; }
            continue;
        }
        float off = fabsf(wrap_pi(o->face - atan2f(G.py - o->y, G.px - o->x)));
        int moving = o->st == MO_WALK || o->st == MO_FLEE || o->st == MO_RUN, s, side = 0, flags = 0;
        float z = 0.0f, a = 0.0f;
        if (o->st == MO_DEAD) { s = SP_MOUSE_F; flags = DR_FLIPV; z = -0.62f * sc; a = fmaxf(0.05f, 1.0f - o->t / 1.5f); }   /* on its back */
        else if (o->st == MO_TELL) { s = SP_MOUSE_TELL; side = 1; }
        else if (o->st == MO_PREEN && ((int)(o->t * 0.4f + (float)i) % 3) != 2) { s = SP_MOUSE_PREEN; side = 1; }
        else if (off < 0.79f) s = SP_MOUSE_F;
        else if (off > 2.36f) s = SP_MOUSE_BK;
        else { s = moving && (((int)((float)G.t * 14.0f) + i) & 1) ? SP_MOUSE_SB : SP_MOUSE_SA; side = 1; }
        int n0 = n;
        ADD(o->x, o->y, z, s, a);
        if (n == n0) continue;
        Draw *d = &list[n - 1];
        d->scale = sc; d->flags |= flags;
        if (side && sinf(o->face - G.ang) < 0.0f) d->flags |= DR_FLIPH;
        if (o->st != MO_DEAD && crawl_eyes[s].n && !is_back(s)) {
            d->eye = 1; d->etype = MOB_RAT; d->erank = RK_I; d->estate = o->st == MO_FLEE || o->msg ? 1 : 0;
        }
        d->mflag = o->msg && (o->st == MO_RUN || o->st == MO_TELL);
    }
    for (int i = 0; i < MAXHAZ && !PK.depth && !G.shop; i++) {      /* the hazards standing up off the floor */
        const Hazard *h = &G.haz[i];
        if (!h->alive) continue;
        int ph = hz_phase(h), n0 = n;
        float kf = h->off > 0.0f ? 1.0f - clampf((h->t - h->warn - h->on) / h->off, 0.0f, 1.0f) : 0.0f;
        switch (h->kind) {
        case HZ_ROCK:
            if (ph == 0) {
                ADD(h->x, h->y, 0.0f, SP_GRIT0 + (((int)(G.t * 8.0)) & 1), 0);
                if (h->t > h->warn - 0.28f) { int n1 = n; ADD(h->x, h->y, (h->warn - h->t) / 0.28f * 0.8f, SP_ROCK, 0); if (n > n1) list[n - 1].scale = 0.8f; }
            } else ADD(h->x, h->y, -0.15f * (1.0f - kf), SP_RUBBLE, fmaxf(0.05f, kf));
            break;
        case HZ_FIRE: case HZ_FISSURE:
            if (ph == 1)
                for (int c = 0; c < h->n; c++) {
                    int n1 = n;
                    ADD(h->cx[c] + 0.5f, h->cy[c] + 0.5f, 0.0f, SP_ERUPT0 + ((((int)(G.t * 10.0)) + c) & 1), 0);
                    if (n > n1) { list[n - 1].flags = DR_BRIGHT; list[n - 1].scale = 0.85f; }
                }
            break;
        case HZ_VOTIVE:                                            /* the rack, and its wax burning */
            ADD(h->x, h->y, 0.0f, SP_VOTIVE, ph == 2 ? fmaxf(0.1f, kf) : 0.0f);
            if (ph == 1)
                for (int c = 1; c < h->n; c++) {
                    int n1 = n;
                    ADD(h->cx[c] + 0.5f, h->cy[c] + 0.5f, 0.0f, SP_BURN_A + ((((int)(G.t * 8.0)) + c) & 1), 0);
                    if (n > n1) { list[n - 1].flags = DR_BRIGHT; list[n - 1].scale = 0.55f; }
                }
            break;
        case HZ_BREATH: case HZ_SILT: case HZ_DAMP: {              /* the breath and the gas, hanging over their cells */
            float a = ph == 1 ? (h->kind == HZ_DAMP ? 0.32f : 0.42f) : (ph == 2 ? 0.4f * kf : (h->kind == HZ_DAMP ? 0.32f * clampf(h->t / h->warn, 0.0f, 1.0f) : 0.0f));
            if (a < 0.03f) break;
            for (int c = 0; c < h->n; c += 1 + (h->n > 10)) {
                int n1 = n;
                float bob = 0.04f * sinf((float)G.t * 1.1f + c * 1.7f);
                ADD(h->cx[c] + 0.5f, h->cy[c] + 0.5f, -0.08f + bob, h->kind == HZ_BREATH ? SP_VAPOUR : SP_GAS, a);
                if (n > n1) list[n - 1].scale = 0.95f;
            }
            break;
        }
        case HZ_TIMBER:                                            /* dust sifting; the roof coming down; rubble lying a while */
            for (int c = 0; c < h->n; c++) {
                float x = h->cx[c] + 0.5f, y = h->cy[c] + 0.5f;
                if (ph == 0) {
                    ADD(x, y, 0.0f, SP_GRIT0 + ((((int)(G.t * 8.0)) + c) & 1), 0);
                    if (h->t > h->warn - 0.28f) { int n1 = n; ADD(x, y, (h->warn - h->t) / 0.28f * 0.8f, SP_ROCK, 0); if (n > n1) list[n - 1].scale = 0.8f; }
                } else ADD(x, y, -0.05f - 0.15f * clampf(1.0f - (h->warn + h->on + h->off - h->t), 0.0f, 1.0f), SP_RUBBLE, 0);
            }
            break;
        case HZ_CART: {                                            /* shaking at its end, then rolling, sparks off its wheels */
            float u = ph == 0 ? 0.0f : cart_at(h);
            int k = imin(h->n - 1, (int)u), k2 = imin(h->n - 1, k + 1);
            float f = u - (float)k, x = lerpf(h->cx[k], h->cx[k2], f) + 0.5f, y = lerpf(h->cy[k], h->cy[k2], f) + 0.5f;
            if (ph == 0) { x += 0.03f * sinf((float)G.t * 40.0f); }
            ADD(x, y, 0.0f, SP_CART, 0);
            if (ph == 1) { int n1 = n; ADD(x, y, -0.4f, SP_SPARK, 0); if (n > n1) { list[n - 1].flags = DR_BRIGHT; list[n - 1].scale = 0.35f; } }
            break;
        }
        case HZ_GLASS:                                             /* the spikes stand up, then sink */
            if (ph >= 1) { float k = ph == 1 ? clampf((h->t - h->warn) / 0.12f, 0.0f, 1.0f) : kf; ADD(h->x, h->y, -0.5f * (1.0f - k), SP_SPIKES, 0); }
            break;
        case HZ_ASH: {                                             /* ash coming down, thick where it chokes */
            float a = ph == 0 ? 0.3f * clampf(h->t / h->warn, 0.0f, 1.0f) : (ph == 1 ? 0.65f : 0.65f * kf);
            for (int j = 0; j < 5 && a > 0.03f; j++) {
                float ang = j * 1.2566f + 0.3f, rr = j ? 1.1f : 0.0f, fall = fmodf((float)G.t * 0.35f + j * 0.37f, 1.0f);
                int n1 = n;
                ADD(h->x + cosf(ang) * rr, h->y + sinf(ang) * rr, 0.25f - 0.5f * fall, SP_ASHFALL, a);
                if (n > n1) list[n - 1].scale = 1.0f;
            }
            break;
        }
        case HZ_VOID: {
            float a = ph == 0 ? 0.12f + 0.15f * clampf(h->t / h->warn, 0.0f, 1.0f) : (ph == 1 ? 0.55f : 0.55f * kf);
            ADD(h->x, h->y, 0.0f, SP_VOIDCOL, fmaxf(0.03f, a));
            if (n > n0) list[n - 1].scale = 0.8f + 0.4f * h->r;
            break;
        }
        case HZ_GRASP:
            if (ph > 0) { float k = clampf((h->t - h->warn) / 0.25f, 0.0f, 1.0f); ADD(h->x, h->y, -0.45f * (1.0f - k), SP_HANDS, fmaxf(0.05f, kf)); }
            break;
        default: break;
        }
    }
    for (int i = 0; i < G.nthing && !PK.depth && !G.shop; i++) {  /* a puzzle's things (1.0.0b13) */
        const Thing *t = &G.thing[i];
        if (t->spr < 0 || G.carry == i + 1) continue;
        int n0 = n, spr = t->spr + (t->frames > 1 ? t->state % t->frames : 0), back = 0;
        if (t->facing) spr = t->spr + ((t->state - dir_to(G.px, G.py, t->x, t->y) + 4) & 3);   /* away, to your right, to you, to your left */
        if (t->spr >= SP_PBELL0 && t->spr <= SP_PBELL3 && t->t < 0.8f) { spr = SP_PBELL0R + t->spr - SP_PBELL0; back = t->t >= 0.42f; }   /* rung: it leans, and swings back */
        ADD(t->x, t->y, t->z, spr, 0);
        if (n > n0) {
            list[n - 1].scale = t->scale;
            if (t->t < 0.35f || (t->role == TR_STRIKE && t->t < 1.2f)) list[n - 1].flags = DR_EDGE;   /* just touched */
            if (back) list[n - 1].flags |= DR_FLIPH;
            if (t->ramp >= 0) list[n - 1].remap = t->ramp;
        }
        if (t->mark && n > n0) {                                   /* its mark, carved on it */
            int n1 = n;
            ADD(t->x, t->y, t->z + t->mz, t->mark >= 100 ? SP_PZ_LETTER0 + t->mark - 100 : t->mark > 16 ? SP_PIP1 + t->mark - 17 : SP_MARK0 + t->mark - 1, 0);
            if (n > n1) { list[n - 1].depth -= 0.01f; list[n - 1].scale = t->scale; }
        }
    }
    for (int i = 0; i < G.nsite && !PK.depth && !G.shop; i++) {   /* the fixed hazards' own things (1.0.0b13) */
        const HzSite *st = &G.site[i];
        const Hazard *h = st->haz >= 0 && G.haz[st->haz].alive ? &G.haz[st->haz] : NULL;
        float cx = st->x + 0.5f, cy = st->y + 0.5f;
        int n0 = n;
        switch (st->kind) {
        case HZ_CENSER: {
            float bx, by;
            censer_bob(st, st->t, &bx, &by, NULL);
            ADD(bx, by, 0.0f, SP_CENSER, 0);
            for (int j = 1; j <= 3; j++) {                         /* its smoke, trailing */
                float ox, oy;
                censer_bob(st, st->t - 0.13f * (float)j, &ox, &oy, NULL);
                int n1 = n;
                ADD(ox, oy, 0.12f + 0.06f * (float)j, SP_SMOKE, 0.4f / (float)j);
                if (n > n1) list[n - 1].scale = 0.3f + 0.05f * (float)j;
            }
            break;
        }
        case HZ_RELIQ: ADD(cx, cy, 0.0f, st->state ? SP_RELIQ_O : SP_RELIQ, 0); break;
        case HZ_CRYSTAL:
            ADD(cx, cy, 0.0f, SP_CRYSTAL_A + (((int)(G.t * 2.0)) & 1), 0);
            if (n > n0 && h && hz_phase(h) == 0 && fmodf((float)G.t * (3.0f + 9.0f * h->t / h->warn), 1.0f) < 0.5f) list[n - 1].flags = DR_BRIGHT;   /* singing higher */
            break;
        case HZ_SNARE: if (!st->state) ADD(cx, cy, 0.0f, SP_SNARE, 0.3f); break;
        case HZ_CART: if (!h) { int e = st->state ? st->n - 1 : 0; ADD(st->cx[e] + 0.5f, st->cy[e] + 0.5f, 0.0f, SP_CART, 0); } break;
        default: break;
        }
    }
    for (int i = 0; i < MAXSHOTS; i++) {
        const Shot *s = &G.shots[i];
        if (!s->alive || (s->x - G.px) * dx + (s->y - G.py) * dy < (s->kind == SHOT_ORB ? 1.1f : 0.6f)) continue;   /* the hand covers it leaving */
        float sc;
        int sp = shot_sprite(s, &sc), n0 = n;
        if (s->scale > 0.0f) sc *= s->scale;                       /* Burning Intensity's grown bolt, Firecracker's spark (1.0.0b21) */
        ADD(s->x, s->y, -0.02f + (1.0f - sc) * 0.5f + (s->kind == AT_FLASK ? s->z : 0.0f), sp, 0);
        if (n > n0) list[n - 1].scale = sc;
    }
    for (int i = 0; i < MAXBOOMS; i++) {
        const Boom *b = &G.booms[i];
        if (!b->alive) continue;
        float sc = b->big ? b->r * 0.8f : 1.0f;
        int n0 = n;
        ADD(b->x, b->y, b->big >= 2 ? -0.12f : (b->big ? -0.1f : 0.0f), b->big == 3 ? SP_FROST : b->big == 4 ? SP_ORB_A + (((int)(G.t * 20.0)) & 1)
                                                                       : SP_BOOM1 + clampi((int)(b->t / 0.2f), 0, 2), b->big == 4 ? 1.0f - b->t / 0.6f : 0);
        if (n > n0) { list[n - 1].scale = b->big == 4 ? 0.8f + b->t * 1.5f : sc; if (b->big == 2) list[n - 1].flags |= DR_VENOM | DR_BRIGHT;
                      if (b->big >= 3) list[n - 1].flags |= DR_BRIGHT | (b->big == 3 ? DR_ICE : 0); }
    }
    for (int i = 0; i < MAXPOOLS; i++) {                           /* a drowned one's foul pool */
        const Pool *p = &G.pools[i];
        if (p->alive) ADD(p->x, p->y, 0.0f, SP_DROWN_POOL, 1.0f - smoothstepf(4.0f, 5.0f, p->t));
    }
    for (int i = 0; i < MAXBURNS; i++) {
        const Burn *b = &G.burns[i];
        if (!b->alive) continue;
        int n0 = n;
        ADD(b->x, b->y, 0.0f, ((int)(G.t * 9.0) + i) & 1 ? SP_BURN_B : SP_BURN_A, 1.0f - smoothstepf(b->life - 0.9f, b->life, b->t));
        if (n > n0) list[n - 1].scale = b->foe == 2 ? 0.55f : 0.9f;   /* Firestarter's: lower flames, in a line */
    }
    if (g_drawn && DF.n && DF.fade > 0.02f)                        /* a drawn level's burning floor: two flames a cell, low as it goes */
        for (int i = 0; i < DF.n; i++)
            for (int k = 0; k < 2; k++) {
                int n0 = n;
                float fx = (float)DF.cx[i] + (k ? 0.72f : 0.28f), fy = (float)DF.cy[i] + (((i + k) & 1) ? 0.7f : 0.3f);
                ADD(fx, fy, 0.0f, (((int)(G.t * 9.0)) + i + k) & 1 ? SP_BURN_B : SP_BURN_A, fminf(1.0f, DF.fade * 1.5f));
                if (n > n0) list[n - 1].scale = (0.85f + 0.15f * sinf((float)G.t * 7.0f + (float)(i * 2 + k))) * (0.35f + 0.65f * DF.fade);
            }
    for (int i = 0; i < MAXVENOM; i++) {                           /* Seep's pools, green; the Lance's rime */
        const Venom *v = &G.venoms[i];
        if (!v->alive) continue;
        int n0 = n;
        if (v->rime == 2) {                                        /* Miasma's cloud (D7): a green vapour standing, gathering and thinning */
            float a = 0.6f * smoothstepf(0.0f, 0.25f, v->t) * (1.0f - smoothstepf(v->life - 0.5f, v->life, v->t));
            for (int c = 0; c < 2 && a > 0.03f; c++) {
                int n1 = n;
                float ox = c ? 0.25f : -0.2f, oy = c ? -0.15f : 0.2f, bob = 0.04f * sinf((float)G.t * 1.3f + (float)c * 2.1f);
                ADD(v->x + ox, v->y + oy, -0.02f + bob, SP_VAPOUR, a);
                if (n > n1) { list[n - 1].scale = 0.8f + 0.3f * smoothstepf(0.0f, 1.0f, v->t); list[n - 1].flags |= DR_BRIGHT; }
            }
            continue;
        }
        ADD(v->x, v->y, 0.0f, v->rime ? SP_RIME : SP_DROWN_POOL, 1.0f - smoothstepf(v->life - 0.6f, v->life, v->t));
        if (n > n0) { list[n - 1].scale = (v->rime ? 1.0f : 0.8f) * v->r / (v->rime ? 0.45f : 0.55f); if (!v->rime) list[n - 1].flags |= DR_VENOM; }
    }
    for (int i = 0; i < MAXMOTES; i++) {                           /* souls where the dead fell, fading in their last two seconds */
        const SoulMote *m = &G.motes[i];
        if (!m->alive) continue;
        int n0 = n;
        ADD(m->x, m->y, 0.18f + 0.04f * sinf((float)G.t * 3.0f + (float)i), SP_MOTE, 1.0f - smoothstepf(MOTE_LIFE - 2.0f, MOTE_LIFE, m->t));
        if (n > n0) { list[n - 1].scale = 0.45f; list[n - 1].flags |= DR_BRIGHT; }
    }
    for (int k = 0; k < 3 && G.fwall_t > 0.0f; k++) {              /* Firewall's flame spirits, circling */
        float an = (float)G.t * 2.2f + (float)k * TAU_F / 3.0f;
        int n0 = n;
        ADD(G.px + cosf(an) * 1.2f, G.py + sinf(an) * 1.2f, 0.12f + 0.04f * sinf((float)G.t * 5.0f + (float)k), ((int)(G.t * 10.0) + k) & 1 ? SP_FSPIRIT_B : SP_FSPIRIT_A,
            1.0f - smoothstepf(0.0f, 0.6f, 0.6f - G.fwall_t));
        if (n > n0) { list[n - 1].scale = 0.45f; list[n - 1].flags |= DR_BRIGHT; }
    }
    for (int i = 0; i < MAXMOBS; i++) {                            /* C4's: a stamp's dust, a charge's after-images, a spew's gouts */
        const Mob *m = &G.mobs[i];
        if (!m->alive || m->state != MS_WALK || !m->c4) continue;
        float sc = KINDS[m->type].scale * RANK_SIZE[m->rank];
        const uint8_t *lut = m->rank != RK_I && KINDS[m->type].family >= 0 ? crawl_rank_lut[KINDS[m->type].family][m->rank] : NULL;
        if (m->c4 == C4_STAMP || (m->c4 == C4_RUSH && m->c4_t < 0.5f)) {
            int n0 = n, rush = m->c4 == C4_RUSH;
            ADD(rush ? m->c4x : m->x, rush ? m->c4y : m->y, 0.0f, rush ? SP_DUST_B : SP_DUST_A, rush ? 0.3f + m->c4_t : 0.0f);
            if (n > n0) { list[n - 1].scale = 0.8f * sc; list[n - 1].flags |= DR_LIT; }
        }
        if (m->c4 == C4_RUSH) {
            for (int k = 1; k <= 2; k++) {                         /* dust kicked up along its way, seen past it head on */
                int n0 = n;
                ADD(m->x - cosf(m->c4a) * 0.5f * (float)k, m->y - sinf(m->c4a) * 0.5f * (float)k, 0.0f, SP_DUST_B, 0.25f * (float)k);
                if (n > n0) { list[n - 1].scale = (0.75f + 0.2f * (float)k) * sc; list[n - 1].flags |= DR_LIT; }
            }
            for (int k = 1; k <= 2; k++) {
                float back = 0.45f * (float)k;
                int n0 = n;
                ADD(m->x - cosf(m->c4a) * back, m->y - sinf(m->c4a) * back, m->z, SP_SHIELD_RUSH, k == 1 ? 0.5f : 0.72f);
                if (n > n0) { list[n - 1].scale = sc; list[n - 1].lut = lut; }
            }
        }
        if (m->c4 == C4_SPEW) {
            for (int k = 0; k < 6; k++) {
                float u = fmodf(m->c4_t * 2.6f + (float)k / 6.0f, 1.0f), off = ((float)((k * 37) % 7) / 6.0f - 0.5f) * 0.5f, r = 0.35f + 1.9f * u;
                if (m->c4_t > 0.3f && u < 0.3f) continue;          /* the last of it, not more */
                int n0 = n;
                ADD(m->x + cosf(m->c4a + off) * r, m->y + sinf(m->c4a + off) * r, 0.42f - 0.34f * u, (k & 1) ? SP_SPEW_B : SP_SPEW_A, 0.0f);
                if (n > n0) list[n - 1].scale = 0.3f + 0.3f * u;
            }
        }
    }
    for (int i = 0; i < MAXBONES; i++) {
        const Bone *b = &G.bones[i];
        if (!b->alive) continue;
        if (b->kind == MIS_FIRE) {
            int n0 = n;
            ADD(b->x, b->y, 0.12f, ((int)(G.t * 14.0)) & 1 ? SP_SKULLFIRE_B : SP_SKULLFIRE_A, 0);
            if (n > n0) list[n - 1].scale = 0.8f;
        } else if (b->kind == MIS_EMBER) {                         /* lobbed: up, and down where you were going */
            float k = b->life0 > 0.0f ? 1.0f - b->life / b->life0 : 0.5f;
            int n0 = n;
            ADD(b->x, b->y, 0.05f + 0.4f * sinf(PI_F * k), ((int)(G.t * 12.0)) & 1 ? SP_EMBER_B : SP_EMBER_A, 0);
            if (n > n0) list[n - 1].scale = 0.45f;
        } else if (b->kind == MIS_WEB) {                           /* a glob of web */
            int n0 = n;
            ADD(b->x, b->y, 0.15f, SP_SAC_A, 0);
            if (n > n0) list[n - 1].scale = 0.35f;
        } else if (b->kind == MIS_TURNED) {                        /* your spell, coming back */
            Shot fake = { 0 };
            float sc;
            fake.kind = b->atk;
            int n0 = n;
            ADD(b->x, b->y, 0.1f, shot_sprite(&fake, &sc), 0);
            if (n > n0) list[n - 1].scale = sc;
        } else {
            float z = b->life0 > 0.0f && b->kind == MIS_BONE ? 0.04f + 0.35f * sinf(PI_F * (1.0f - b->life / b->life0)) : 0.04f;   /* a lobbed one arcs */
            int n0 = n, e = bone_el(b);
            ADD(b->x, b->y, z, SP_BONEFLY0 + (((int)b->spin) & 3), 0);
            if (e >= 0 && e != EL_PHYS && b->kind == MIS_BONE && n > n0) {   /* its element (C3): shimmering, motes trailing as a wisp's */
                list[n - 1].flags |= DR_ELEMX | DR_BRIGHT; list[n - 1].remap = ELEM_RAMP[e];
                float sp = hypotf(b->vx, b->vy) + 1e-4f;
                for (int k = 1; k <= 3; k++) {
                    float back = 0.18f * (float)k, wob = 0.06f * sinf((float)G.t * 17.0f + (float)(k * 2 + i)), sc = 0.34f - 0.05f * (float)k;
                    int m0 = n;
                    ADD(b->x - b->vx / sp * back - b->vy / sp * wob, b->y - b->vy / sp * back + b->vx / sp * wob,
                        z + 0.5f * (1.0f - sc) + 0.04f * sinf((float)G.t * 11.0f + (float)k), SP_BONEFLY0 + ((k + i) & 3), 0);   /* at the bone's height */
                    if (n > m0) { list[n - 1].scale = sc; list[n - 1].flags |= DR_ELEMX | DR_BRIGHT; list[n - 1].remap = ELEM_RAMP[e]; }
                }
            }
        }
    }
    for (int i = 0; i < G.nitems; i++) {
        const Item *it = &G.items[i];
        if (!it->alive) continue;
        static const int SPR[] = { SP_COINS_A, SP_CHALICE, SP_GEMS, SP_CROWN, SP_CHEST, SP_KEY0, SP_POTION, SP_SOULURN, SP_RINGITEM, SP_ITEM_BONE, SP_SCRIP, SP_MAPFLOOR };
        float ph = (float)G.t + it->phase, z = 0.0f;
        int sp = SPR[it->type];
        if (it->type == IT_COINS && ((int)(ph * 2.5f) & 1)) sp = SP_COINS_B;
        if (it->type == IT_KEY) { sp = key_sprite(it->color, ph); z = 0.14f + 0.04f * sinf(ph * 2.5f); }
        int n0 = n;
        ADD(it->x, it->y, z, sp, 0);
        if (n > n0 && it->type == IT_KEY && hypotf(it->x - G.px, it->y - G.py) <= gear_ability(&RUN.gear, AB_KEYMAKER, 0)) {
            static const int GLINT[NKEYS] = { R_GOLD, R_BONE, R_BLOOD };
            list[n - 1].flags |= DR_GLINT; list[n - 1].glint = GLINT[it->color];
        }
        if (n > n0 && it->type != IT_KEY) { float u = ph * 0.6f - floorf(ph * 0.6f); list[n - 1].twinkle = u < 0.1f ? sinf(u / 0.1f * PI_F) : 0.0f; }
        if (n > n0 && it->color == TRI_REWARD) {                   /* a won trial's: full bright; the hoard glints often, edged in gold */
            list[n - 1].flags |= DR_BRIGHT;
            if (it->type == IT_CHEST) {
                float u = ph * 1.4f - floorf(ph * 1.4f);
                list[n - 1].twinkle = u < 0.25f ? sinf(u / 0.25f * PI_F) : 0.0f;
                list[n - 1].flags |= DR_EDGE; list[n - 1].glint = R_GOLD;   /* a gold edge, shimmering (and its own light: build_lightmap) */
            }
        }
    }
    if (G.shop)                                                    /* the bosses' scrip on the Totem's hooks, one for each ability known */
        for (int b = 0; b < NBOSS; b++) {
            if (!RUN.abil[b]) continue;
            static const float HOOK[NBOSS] = { -0.2f, -0.068f, 0.068f, 0.2f };
            int n0 = n;
            ADD(TOTEM_X + rx * HOOK[b], TOTEM_Y + ry * HOOK[b], 0.54f + 0.01f * sinf((float)G.t * 1.3f + (float)b), SP_SCRIP, 0);
            if (n > n0) { list[n - 1].scale = 0.34f; list[n - 1].depth -= 0.05f; }
        }
    if (G.shop) {
        build_cat();
        int n0 = n;
        ADD(CAT_X, CAT_Y, 0.0f, SP_PEDESTAL, 0);
        if (n > n0) { list[n - 1].scale = CAT_S; list[n - 1].flags = DR_CAT; }
        if (!RUN.has_map && !g_map_kept) {                         /* the map, floating at her left until it is yours */
            n0 = n;
            ADD(MAP_X, MAP_Y, 0.42f + 0.04f * sinf((float)G.t * 1.6f), SP_MAPWARE, 0);
            if (n > n0) { float u = (float)G.t * 0.4f; u -= floorf(u); list[n - 1].scale = 0.5f; list[n - 1].twinkle = u < 0.1f ? sinf(u / 0.1f * PI_F) : 0.0f; }
        }
        for (int i = 0; i < G.ped_n; i++) {
            if (G.offer_gone[i]) continue;
            const Offer *o = &G.offers[i];
            int sp = offer_sprite(o);
            float sc = 0.42f, z = 0.55f + 0.03f * sinf((float)G.t * 2.0f + i * 2.1f) - (TS - 1 - spr_bottom(sp)) / (float)TS * sc;
            n0 = n;
            ADD(G.ped_x[i], G.ped_y[i], z, sp, 0);
            if (n > n0) {
                Draw *d = &list[n - 1];
                d->scale = sc; d->remap = offer_color(o);
                float u = (float)G.t * 0.5f + i * 0.33f; u -= floorf(u);
                d->twinkle = u < 0.12f ? sinf(u / 0.12f * PI_F) : 0.0f;
            }
        }
    }
    #undef ADD
    qsort(list, (size_t)n, sizeof list[0], cmp_draw);
    for (int i = n - 1, nv = 0; i >= 0; i--)                       /* the nearest four carry their full vapour, the rest a thread */
        if (list[i].vap > 0.0f && (nv++ >= 4 || list[i].depth > 12.0f)) list[i].vap = 0.5f;
    float bfs_dark = G.bfs_t > 0.0f ? 1.0f - smoothstepf(0.3f, 1.4f, G.bfs_t) : 0.0f;
    for (int i = 0; i < n; i++) {
        Draw *d = &list[i];
        float vx = d->x - G.px, vy = d->y - G.py;
        float lat = vx * rx + vy * ry;
        float size = FOCAL / d->depth * d->scale;
        float scx = W / 2 + FOCAL * lat / d->depth;
        float top = horizon - (d->scale - G.camz + d->z) * FOCAL / d->depth;
        float left = scx - size / 2;
        if (d->flags & DR_EYES) {                                  /* two red points, blinking now and then */
            if (fmodf((float)G.t * 0.7f + d->x * 3.1f, 4.0f) < 0.12f) continue;
            int big = size > 40.0f, gap = (int)fmaxf(1.0f, 3.0f * size / TS);
            for (int e = 0; e < 2; e++) {
                int sx = (int)(left + 32.0f * size / TS) + (e ? gap : -gap - big), sy = (int)(top + 30.5f * size / TS);
                for (int yy = 0; yy <= big; yy++)
                    for (int xx = 0; xx <= big; xx++) {
                        int px = sx + xx, py = sy + yy;
                        if (px >= 0 && px < W && py >= 0 && py < H && d->depth < zbuf[px])
                            f->pix[py * W + px] = d->ecol ? (sinf((float)G.t * 2.0f + d->x) > 0.0f ? EYE_CORE[d->ecol - 1] : EYE_GLOW[d->ecol - 1])
                                                          : (uint8_t)(R_BLOOD + 13 + (int)(2.5f * (0.5f + 0.5f * sinf((float)G.t * 2.0f + d->x))));
                    }
            }
            continue;
        }
        if (d->flags & DR_IMPLODE && d->scale < 0.06f) {           /* the last point of light */
            int sx = (int)scx, sy = (int)(top + size * 0.5f), arm = (int)clampf(6.0f / d->depth, 1.0f, 6.0f);
            for (int k = -arm; k <= arm; k++) {
                int xs[2] = { sx + k, sx }, ys[2] = { sy, sy + k };
                for (int j = 0; j < 2; j++)
                    if (xs[j] >= 0 && xs[j] < W && ys[j] >= 0 && ys[j] < H && d->depth < zbuf[xs[j]]) f->pix[ys[j] * W + xs[j]] = abs(k) > arm / 2 ? (uint8_t)(R_GLOW + 10) : (uint8_t)C_WHITE;
            }
            continue;
        }
        int x0 = clampi((int)ceilf(left - 0.5f), 0, W), x1 = clampi((int)ceilf(left + size - 0.5f), 0, W);
        int y0 = clampi((int)ceilf(top - 0.5f), 0, H), y1 = clampi((int)ceilf(top + size - 0.5f), 0, H);
        if (x0 >= x1 || y0 >= y1) continue;
        if (d->flags & DR_CAT) {                                   /* the merchant: a bigger, row-major picture */
            float light = fminf(1.3f, lm_at(d->x + 0.4f, d->y) * atten(d->depth) + 0.25f), cstep = CAT_W / size;
            for (int x = x0; x < x1; x++) {
                if (d->depth >= zbuf[x]) continue;
                int tx = clampi((int)((x + 0.5f - left) * cstep), 0, CAT_W - 1);
                float ty = (y0 + 0.5f - top) * cstep;
                for (int y = y0; y < y1; y++, ty += cstep) {
                    uint8_t c = cat_buf[clampi((int)ty, 0, CAT_H - 1) * CAT_W + tx];
                    if (c != C_CLEAR) f->pix[y * W + x] = shade(c, light, x, y);
                }
            }
            continue;
        }
        float light = lm_at(d->x, d->y) * atten(d->depth) * (1.0f - 0.85f * bfs_dark);
        if (d->flags & DR_LIT) light = fminf(1.3f, light * 1.3f + 0.15f);
        float fog = fog_amount(d->x, d->y, 1.0f - expf(-d->depth * 0.28f));
        const uint8_t *img = crawl_spr[d->spr];
        float step = TS / size;
        if (d->flags & DR_SWAY) {                                  /* hung from the top: the lower it is, the further it swings */
            for (int x = x0; x < x1; x++) {
                if (d->depth >= zbuf[x]) continue;
                float tx0 = (x + 0.5f - left) * step, ty = (y0 + 0.5f - top) * step;
                for (int y = y0; y < y1; y++, ty += step) {
                    int tyi = clampi((int)ty, 0, TS - 1), tx = (int)(tx0 - d->sway * tyi / (float)TS);
                    if (tx < 0 || tx >= TS) continue;
                    uint8_t c = img[tx * TS + tyi];
                    if (c != C_CLEAR) f->pix[y * W + x] = fogged(shade(c, light, x, y), fog, light, x, y);
                }
            }
            continue;
        }
        int com = d->flags & (DR_COMMUNE | DR_GLINT);              /* seen where it can't be: Commune's red, a key's glint */
        float ck = (d->flags & DR_COMMUNE) ? fminf(1.0f, fminf(G.commune_show, G.commune_len - G.commune_show) * 5.0f + 0.2f) : (com ? 0.6f : 0.0f);
        if (d->flags & DR_CLIP) {                                  /* half sunk: cut where the floor and ceiling are */
            y0 = clampi(y0, (int)ceilf(horizon - (1.0f - G.camz) * FOCAL / d->depth), H);
            y1 = clampi(y1, 0, (int)ceilf(horizon + G.camz * FOCAL / d->depth));
            if (y0 >= y1) continue;
        }
        static int8_t gshift[TS];
        static uint8_t gslip[TS];
        int glitch = (d->flags & DR_GLITCH) && glitch_rows(d, gshift, gslip);
        for (int x = x0; x < x1; x++) {
            int behind = d->depth >= zbuf[x];
            if (behind && !com) continue;
            int tx = clampi((int)((x + 0.5f - left) * step), 0, TS - 1);
            if (d->flags & DR_FLIPH) tx = TS - 1 - tx;
            const uint8_t *col = &img[tx * TS];
            float ty = (y0 + 0.5f - top) * step;
            for (int y = y0; y < y1; y++, ty += step) {
                int tyi = clampi((int)ty, 0, TS - 1), sy = (d->flags & DR_FLIPV) ? TS - 1 - tyi : tyi;
                uint8_t c = col[sy];
                if (glitch) {
                    int gx = tx - gshift[tyi];
                    c = gx >= 0 && gx < TS ? img[gx * TS + sy] : C_CLEAR;
                    if (c == C_CLEAR && gslip[tyi] && !behind) {   /* the colours slipping off it */
                        if (gx - 2 >= 0 && gx - 2 < TS && img[(gx - 2) * TS + sy] != C_CLEAR) f->pix[y * W + x] = (uint8_t)(R_BLOOD + 11);
                        else if (gx + 2 >= 0 && gx + 2 < TS && img[(gx + 2) * TS + sy] != C_CLEAR) f->pix[y * W + x] = (uint8_t)(R_GLOW + 9);
                        continue;
                    }
                }
                int ovr = 0;                                       /* its rank's dressing, in its own colours */
                if (d->ov) {
                    int ox = glitch ? tx - gshift[tyi] : tx;
                    if (ox >= 0 && ox < TS && d->ov[ox * TS + sy] != C_CLEAR) { c = d->ov[ox * TS + sy]; ovr = 1; }
                }
                if (c == C_CLEAR) {
                    if (d->ward && !behind && ((tyi < TS - 1 && col[tyi + 1] != C_CLEAR) || (tyi > 0 && col[tyi - 1] != C_CLEAR)
                        || (tx > 0 && img[(tx - 1) * TS + tyi] != C_CLEAR) || (tx < TS - 1 && img[(tx + 1) * TS + tyi] != C_CLEAR))) {
                        int we = (d->ward & 15) - 1, w2 = d->ward >> 5;     /* warded: a hard rim; struck, it rings white; two, they alternate */
                        if (w2 && ((x + y) >> 1 & 1)) we = w2 - 1;
                        f->pix[y * W + x] = (d->ward & 16) ? (uint8_t)C_WHITE : ramp_at(ELEM_RAMP[we], 13);
                        continue;
                    }
                    if ((d->flags & (DR_EDGE | DR_ALLY)) && !behind && ((x + y + (int)(G.t * 10.0)) & 1)
                        && ((tyi < TS - 1 && col[tyi + 1] != C_CLEAR) || (tx > 0 && img[(tx - 1) * TS + tyi] != C_CLEAR)
                            || (tx < TS - 1 && img[(tx + 1) * TS + tyi] != C_CLEAR)))
                        f->pix[y * W + x] = (d->flags & DR_ALLY) ? (uint8_t)(R_PURPLE + 13) : d->glint && !(d->flags & DR_GLINT) ? ramp_at(d->glint, 12 + ((x ^ y) & 3)) : (uint8_t)C_WHITE;   /* (a ramp in glint: a won trial's hoard, gold) the blessing: a white edge
                                                                                                         about its fire; yours, a violet one */
                    continue;
                }
                int faded = d->alpha < 1.0f && d->alpha * 16.0f <= BAYER[(y + (int)(G.t * 20.0)) & 3][x & 3] + 0.5f;
                if (behind || (com && faded)) {
                    if (d->flags & DR_GLINT) { if (((x + y + (int)(G.t * 6.0)) & 3) < 2) f->pix[y * W + x] = c; }   /* its own colours, shimmering */
                    else if (ck * 16.0f > BAYER[y & 3][x & 3]) f->pix[y * W + x] = (uint8_t)(R_BLOOD + 9 + ((x + y) & 1) * 3);
                    continue;
                }
                if (faded) continue;
                if (d->lut && !ovr) c = d->lut[c];
                if ((d->flags & DR_SHINE) && c >= R_IRON && c < R_IRON + 16) {   /* polished: brighter, and the glint going by */
                    int v = c - R_IRON;
                    if (G.bmirror_t > 0.0f) { f->pix[y * W + x] = ((x + y) & 1) ? (uint8_t)C_WHITE : (uint8_t)(R_GHOST + 14); continue; }   /* the Mirror Rune */
                    float u = ((float)tx + (float)(TS - tyi)) / (2.0f * TS) - (fmodf((float)G.t * 0.3f, 1.8f) - 0.4f);
                    if (fabsf(u) < 0.035f && v > 3) { f->pix[y * W + x] = fabsf(u) < 0.012f ? (uint8_t)C_WHITE : (uint8_t)(R_IRON + 15); continue; }
                    c = (uint8_t)(R_IRON + imin(15, v + 2 + v / 4));
                }
                if ((d->flags & DR_MEND) && ((x + y + (int)(G.t * 8.0)) & 3) == 0) { f->pix[y * W + x] = (uint8_t)(R_MOSS + 12 + ((x >> 1) & 3)); continue; }   /* mending: green */
                if ((d->flags & DR_POISONED) && hash2(tx >> 1, (tyi + (int)(G.t * 9.0)) >> 1) % 23u == 0) { f->pix[y * W + x] = (uint8_t)(R_MOSS + 11 + ((tyi >> 2) & 3)); continue; }   /* bubbles rising */
                if ((d->flags & DR_BURNING) && tyi > TS / 3
                    && (hash2(tx >> 1, (tyi + (int)(G.t * 24.0)) >> 1) & 7) == 0) { f->pix[y * W + x] = (uint8_t)(R_FLAME + 9 + (int)(hash2(tyi, tx) & 6)); continue; }   /* flames licking up it */
                if ((d->flags & DR_BLEED) && tyi > TS / 5 && hash2(tx, 91) % 5u == 0) {   /* bleeding (D7): thin runs of blood going down it,
                                                                      a bright drop at each foot, a darkening trail above */
                    int ph = (tyi - (int)(G.t * 9.0) + (int)(hash2(tx, 7) & 31)) & 31;
                    if (ph < 6) { f->pix[y * W + x] = (uint8_t)(R_BLOOD + (ph == 5 ? 12 : ph >= 3 ? 9 : 6)); continue; }
                }
                if ((d->flags & DR_VENOM) && ((c >= R_FLAME && c < R_FLAME + 16) || (c >= R_GLOW && c < R_GLOW + 16) || c == C_WHITE)) c = (uint8_t)(R_MOSS + (c == C_WHITE ? 15 : (c & 15)));
                if ((d->flags & DR_ELEMX) && (d->scale < 0.4f || ((x + y + (int)(G.t * 15.0)) & 1))) {   /* an element carried (C3) */
                    int lo = d->remap == R_FLAME ? 4 : 7;                  /* fire kept orange, apart from lightning's gold */
                    f->pix[y * W + x] = ramp_at(d->remap, clampi(((c - 1) & 15) / 3 + lo + (int)(hash2(x >> 1, y + (int)(G.t * 10.0)) & 3), lo, lo + 8));
                    continue;
                }
                if (d->remap && c >= R_PURPLE && c < R_PURPLE + 16) c = ramp_at(d->remap, c - R_PURPLE);
                if (d->flags & DR_IMPLODE) { f->pix[y * W + x] = c == 0 ? 0 : (uint8_t)(R_GLOW + clampi(6 + (int)((c & 15) * 0.6f), 6, 15)); continue; }
                if ((d->flags & DR_ICE) && c != 0 && ((x + y) & 1)) {    /* rimed with frost */
                    f->pix[y * W + x] = fogged((uint8_t)(R_GLOW + clampi(2 + (int)(((c - 1) & 15) * 0.5f + light * 4.0f), 2, 12)), fog, light, x, y);
                    continue;
                }
                if ((d->flags & (DR_CHILLA | DR_CHILLB)) && c != 0     /* Chilled: the frost creeping over it, a little more a stack */
                    && BAYER[y & 3][x & 3] < 1 + ((d->flags & DR_CHILLA ? 2 : 0) + (d->flags & DR_CHILLB ? 1 : 0)) * 4 / 3) {   /* 2, 3, 5 of 16 */
                    f->pix[y * W + x] = fogged((uint8_t)(R_GLOW + clampi((int)(((c - 1) & 15) * 0.4f + light * 3.0f), 1, 9)), fog, light, x, y);
                    continue;
                }
                f->pix[y * W + x] = (d->flags & DR_BRIGHT) ? c : fogged(shade(c, light, x, y), fog, light, x, y);
            }
        }
        if (((d->flags & DR_SHINE) && G.bmirror_t > 0.0f) || (d->flags & DR_RIPPLE)) {   /* the Mirror Rune's silver ring; Time Slip's grey */
            int grey = (d->flags & DR_RIPPLE) != 0;
            float k = grey ? (1.5f - G.bslip_t) / 0.6f : 1.0f - G.bmirror_t / 0.35f, rx = size * (0.22f + (grey ? 0.9f : 0.4f) * k), ry = rx * 0.85f;
            float cx = left + size * 0.5f, cy = top + size * 0.55f;
            for (int a = 0; a < 96; a++) {
                if (k > 0.5f && ((a + (int)(G.t * 30.0)) & 1)) continue;   /* thinning as it goes */
                int px = (int)(cx + cosf(a * (TAU_F / 96.0f)) * rx), py = (int)(cy + sinf(a * (TAU_F / 96.0f)) * ry);
                if (px >= 0 && px < W && py >= 0 && py < H && d->depth < zbuf[px] + 0.6f)
                    f->pix[py * W + px] = grey ? (uint8_t)(R_STONE + 9 + (a & 3)) : (a & 3) ? (uint8_t)C_WHITE : (uint8_t)(R_GHOST + 12);
            }
        }
        if (d->flags & DR_PINNED)                                  /* Impale: a bone shaft through it */
            for (int i = 0; i <= (int)(size * 0.8f); i++) {
                int px = (int)(left + size * 0.1f) + i, py = (int)(top + size * (0.42f + 0.12f * (float)i / (size * 0.8f + 1.0f)));
                if (px >= 0 && px < W && py >= 0 && py + 1 < H && d->depth < zbuf[px]) { f->pix[py * W + px] = R_BONE + 13; f->pix[(py + 1) * W + px] = R_BONE + 8; }
            }
        if (d->spr == SP_LAMPSTAND && G.shop)                      /* each line open burning in its bowl, in its colour, */
            for (int b = 0; b < NELEM; b++) {                      /* taller as it ranks */
                int r = align_rank(b);
                if (!r) continue;
                for (int dy = -16; dy <= 0; dy++)
                    for (int dx = -5; dx <= 5; dx++) {
                        int v = crawl_lamp_flame(dx, dy, r, (float)G.t, b);
                        if (!v) continue;
                        int x0 = (int)(left + (float)(LAMP_BOWL[b][0] + dx) * size / TS), x1 = (int)(left + (float)(LAMP_BOWL[b][0] + dx + 1) * size / TS);
                        int y0 = (int)(top + (float)(LAMP_BOWL[b][1] + dy) * size / TS), y1 = (int)(top + (float)(LAMP_BOWL[b][1] + dy + 1) * size / TS);
                        for (int py = imax(0, y0); py < imin(H, imax(y1, y0 + 1)); py++)
                            for (int px = imax(0, x0); px < imin(W, imax(x1, x0 + 1)); px++)
                                if (d->depth < zbuf[px]) f->pix[py * W + px] = ramp_at(ELEM_RAMP[b], v);
                    }
            }
        if (d->flags & DR_WEBBED) {                                /* your web over it: strands, fewer as it is torn */
            uint32_t hs = (uint32_t)(int)(d->x * 13.0f) * 2654435761u ^ (uint32_t)(int)(d->y * 7.0f) * 40503u;
            for (int k = 0, ns = 3 + (int)(6.0f * d->web); k < ns; k++) {
                uint32_t h1 = hs + (uint32_t)k * 2246822519u;
                h1 ^= h1 >> 13; h1 *= 0x5bd1e995u; h1 ^= h1 >> 15;
                float ax = left + size * (0.2f + 0.6f * (float)(h1 & 255) / 255.0f), ay = top + size * (0.3f + 0.65f * (float)((h1 >> 8) & 255) / 255.0f);
                float bx = left + size * (0.2f + 0.6f * (float)((h1 >> 16) & 255) / 255.0f), by = top + size * (0.3f + 0.65f * (float)((h1 >> 24) & 255) / 255.0f);
                int np = (int)fmaxf(fabsf(bx - ax), fabsf(by - ay)) + 1;
                for (int i = 0; i <= np; i++) {
                    int px = (int)(ax + (bx - ax) * (float)i / (float)np), py = (int)(ay + (by - ay) * (float)i / (float)np);
                    if (px < 0 || px >= W || py < 0 || py >= H || d->depth >= zbuf[px] || ((px + py) & 1)) continue;
                    f->pix[py * W + px] = (uint8_t)(R_GHOST + 11 + (k & 3));
                }
            }
        }
        if (d->flags & DR_GLINT) {                                 /* Keymaker: a sparkle where the key lies, through the stone */
            float pulse = 0.5f + 0.5f * sinf((float)G.t * 5.0f + d->x);
            int cx = (int)(left + size * 0.5f), cy = (int)(top + size * 0.62f), arm = 3 + (int)(pulse * 4.0f + fminf(size / 12.0f, 4.0f));
            for (int k = -arm; k <= arm; k++) {
                int a = abs(k);
                uint8_t c = a <= 1 ? (uint8_t)C_WHITE : ramp_at(d->glint, 14 - a);
                int px[4] = { cx + k, cx, cx + k / 2, cx + k / 2 }, py[4] = { cy, cy + k, cy + k / 2, cy - k / 2 };
                for (int j = 0; j < (a <= arm / 2 ? 4 : 2); j++)
                    if (px[j] >= 0 && px[j] < W && py[j] >= 0 && py[j] < H) f->pix[py[j] * W + px[j]] = c;
            }
        }
        if (d->mg > 0.0f) {                                        /* the hidden message: a pale point catching on its edge */
            int gx = (int)(left + size * 0.6f), gy = (int)(top + size * 0.73f), arm = (int)(d->mg * fmaxf(1.0f, size / 20.0f) + 0.5f);
            for (int k = -arm; k <= arm; k++) {
                int xs[2] = { gx + k, gx }, ys[2] = { gy, gy + k };
                for (int j = 0; j < 2; j++)
                    if (xs[j] >= 0 && xs[j] < W && ys[j] >= 0 && ys[j] < H && (d->mgw || d->depth < zbuf[xs[j]]))
                        f->pix[ys[j] * W + xs[j]] = abs(k) <= arm / 3 ? (uint8_t)C_WHITE : (uint8_t)(R_GHOST + 13);
            }
        }
        if (d->twinkle > 0.0f && crawl_glint[d->spr][0] >= 0) {     /* a glint off the gold */
            int gx = (int)(left + (crawl_glint[d->spr][0] + 0.5f) * size / TS), gy = (int)(top + (crawl_glint[d->spr][1] + 0.5f) * size / TS);
            int arm = (int)(d->twinkle * fmaxf(1.0f, size / 28.0f));
            for (int k = -arm; k <= arm; k++) {
                int xs[2] = { gx + k, gx }, ys[2] = { gy, gy + k };
                for (int j = 0; j < 2; j++)
                    if (xs[j] >= 0 && xs[j] < W && ys[j] >= 0 && ys[j] < H && d->depth < zbuf[xs[j]]) f->pix[ys[j] * W + xs[j]] = abs(k) == arm && arm > 1 ? (uint8_t)(R_GOLD + 11) : (uint8_t)C_WHITE;
            }
        }
        if (d->vap > 0.0f) render_vapour(f, d, left, top, size, fog_amount(d->x, d->y, 1.0f - expf(-d->depth * 0.28f)));
        if (d->eye) render_eyes(f, d, left, top, size);           /* its eyes, whatever the light (docs/InitialArtPass.md) */
        if (d->mflag) render_mflag(f, d, left, top, size);         /* a mouse carrying word: its mark (docs/PLAN-mice.md, M9) */
    }
    for (int i = 0; i < MAXMOBS; i++) {                            /* spiders hang by a thread */
        const Mob *m = &G.mobs[i];
        if (!m->alive || m->type != MOB_SPIDER || (m->state == MS_WALK && m->sub == 0) || (m->state == MS_DIE && m->sub != 9)) continue;
        if (m->state == MS_DIE && m->t > 0.25f) continue;          /* snapped */
        float L = lm_at(m->x, m->y);
        uint8_t c = (uint8_t)(R_BONE + clampi((int)(4.0f + L * 6.0f), 3, 11));
        line3d(f, dx, dy, horizon, m->ax, m->ay, 0.99f, m->x, m->y, m->z + 0.42f * 0.7f, c, 0, 1);
    }
}

