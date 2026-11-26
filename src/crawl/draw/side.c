/* A part of scene_crawl.c (docs/PLAN-tidy.md): it is #included there in its place and never compiled on its own.
 * A side passage drawn (docs/PLAN-side.md): the far wall slower behind, the back wall darker, the tiles, what stands in it, the
 * dead, the spells, the caster; lit by its torches, its doorways and the caster's own glow, through the crypt's colour map. */

#define SDL_LIGHTS 24
static struct { float x, y, r, k; } SDLT[SDL_LIGHTS];
static int sdl_n;
static void sd_light(float x, float y, float r, float k) { if (sdl_n < SDL_LIGHTS) SDLT[sdl_n++] = (typeof(SDLT[0])){ x, y, r, k }; }
static float sd_glow(float wx, float wy) {
    float g = 0.0f;
    for (int i = 0; i < sdl_n; i++) {
        float dx = wx - SDLT[i].x, dy = wy - SDLT[i].y, d2 = dx * dx + dy * dy, r2 = SDLT[i].r * SDLT[i].r;
        if (d2 < r2) g += SDLT[i].k * (1.0f - d2 / r2);
    }
    return g;
}
static void sd_px(Frame *f, int x, int y, uint8_t c, int light) {
    if (c == C_CLEAR || x < 0 || y < 0 || x >= W || y >= SD_VIEW_H) return;
    f->pix[y * W + x] = IS_FULLBRIGHT(c) ? c : crawl_cmap[clampi(light, 0, NLIGHT - 1)][c];
}
static int sd_lit(float wx, float wy, int base, float gk) { return base + (int)(fminf(9.0f, sd_glow(wx, wy) * gk)); }
static void sd_draw_tile(Frame *f, const uint8_t *t, int sx, int sy, int base, float gk, int flip) {
    if (sx <= -STS || sy <= -STS || sx >= W || sy >= SD_VIEW_H) return;
    int lq[2][2];                                                  /* the light at the tile's quarters, for speed */
    for (int qy = 0; qy < 2; qy++) for (int qx = 0; qx < 2; qx++) lq[qy][qx] = sd_lit(SD.camx + (float)(sx + 4 + qx * 8), SD.camy + (float)(sy + 4 + qy * 8), base, gk);
    for (int y = 0; y < STS; y++)
        for (int x = 0; x < STS; x++) {
            uint8_t c = t[y * STS + (flip ? STS - 1 - x : x)];
            if (c != C_CLEAR) sd_px(f, sx + x, sy + y, c, lq[y >> 3][x >> 3]);
        }
}
static void sd_draw_spr(Frame *f, int spr, int sx, int sy, int flip, int light, int flash) {   /* a crawl sprite at half its size, its feet at sy */
    for (int y = 0; y < TS / 2; y++)
        for (int x = 0; x < TS / 2; x++) {
            int n = 0; uint8_t c = C_CLEAR;
            for (int k = 0; k < 4; k++) {
                int px = x * 2 + (k & 1), py = y * 2 + (k >> 1);
                uint8_t v = crawl_spr[spr][(flip ? TS - 1 - px : px) * TS + py];
                if (v != C_CLEAR) { n++; c = v; }
            }
            if (n >= 2) sd_px(f, sx - TS / 4 + x, sy - TS / 2 + y, flash ? (uint8_t)C_WHITE : c, light);
        }
}
static void sd_blob(Frame *f, float wx, float wy, float r, int ramp, int top) {
    int cx = (int)floorf(wx - SD.camx), cy = (int)floorf(wy - SD.camy), ri = (int)ceilf(r);
    for (int y = -ri; y <= ri; y++) for (int x = -ri; x <= ri; x++) {
        float d = sqrtf((float)(x * x + y * y));
        if (d <= r) sd_px(f, cx + x, cy + y, ramp_at(ramp, top - (int)(d / r * 6.0f)), 24);
    }
}
static int sd_back_tile(int tx, int ty) {                          /* what shows behind a tile that isn't stone */
    int l = sd_tile(tx - 1, ty), r = sd_tile(tx + 1, ty), c = sd_tile(tx, ty);
    if (c == '.') return -1;
    if (c == ',') {
        uint32_t h = (uint32_t)tx * 73856093u ^ (uint32_t)ty * 19349663u;
        return h % 23 == 0 ? ST_BACK_NICHE : h % 17 == 0 ? ST_BACK_SKULLS : (h & 1) ? ST_BACK1 : ST_BACK0;
    }
    return l == '.' || r == '.' ? -1 : ST_BACK0;
}

/* ---- the works (side_works.c) ---- */
static int sd_el_ramp(int el) { return el == EL_FIRE ? R_FLAME : el == EL_FROST ? R_GHOST : el == EL_LIGHT ? R_GLOW : el == EL_POISON ? R_MOSS : el == EL_SPIRIT ? R_PURPLE : R_BONE; }
static uint32_t sd_hash(int a, int b, int c) { uint32_t h = (uint32_t)a * 73856093u ^ (uint32_t)b * 19349663u ^ (uint32_t)c * 83492791u; return h ^ (h >> 13); }
static void sd_draw_fx(Frame *f, int camx, int camy);
static void sd_draw_works(Frame *f, int camx, int camy) {
    const SdLevel *L = SD.L;
    for (int i = 0; i < L->nth; i++) {
        const SdThing *t = &L->th[i];
        int x0 = t->x * SD_TILE - camx, y0 = t->y * SD_TILE - camy;
        if (t->kind == SDT_CURRENT && SD.st[i]) {                  /* water running: its body, and streaks going its way */
            int ramp = (t->flags & SDF_POISON) ? R_MOSS : R_GHOST, W2 = t->w * SD_TILE, H2 = t->h * SD_TILE;
            float n = hypotf((float)t->a, (float)t->b), run = SD.t * t->f[0] * SD_TILE;
            for (int y = 0; y < H2; y++) for (int x = 0; x < W2; x++) {
                if (((x + y) & 1) && !(t->flags & SDF_POISON)) continue;   /* clear water: half the pixels, the wall showing through */
                float along = ((float)x * (float)t->a + (float)y * (float)t->b) / n - run, across = ((float)x * -(float)t->b + (float)y * (float)t->a) / n;
                int streak = ((int)floorf(along / 6.0f + (float)(sd_hash((int)floorf(across / 3.0f), i, 0) % 5)) % 5) == 0;
                sd_px(f, x0 + x, y0 + y, (uint8_t)(ramp + (streak ? 13 : 7 + (int)(sd_hash(x, y, i) % 2))), 22);
            }
        } else if (t->kind == SDT_GATE) {                          /* iron bars, risen as far as it has opened */
            int W2 = t->w * SD_TILE, foot = (int)((float)(t->h * SD_TILE) * (1.0f - SD.gk[i]) + 0.5f);
            for (int y = 0; y < foot; y++) for (int x = 1; x < W2; x += 4) {
                sd_px(f, x0 + x, y0 + y, (uint8_t)(R_IRON + 11), sd_lit((float)(t->x * SD_TILE + x), (float)(t->y * SD_TILE + y), 16, 1.0f));
                sd_px(f, x0 + x + 1, y0 + y, (uint8_t)(R_IRON + 6), sd_lit((float)(t->x * SD_TILE + x), (float)(t->y * SD_TILE + y), 16, 1.0f));
            }
            for (int y = 3; y < foot; y += 8) for (int x = 0; x < W2; x++) sd_px(f, x0 + x, y0 + y, (uint8_t)(R_IRON + 8), 18);
            if (foot > 0) for (int x = 1; x < W2; x += 4) sd_px(f, x0 + x, y0 + foot, (uint8_t)(R_IRON + 13), 20);   /* the points */
        } else if (t->kind == SDT_PLATE) {                         /* a flagstone in the floor's top, sunk while pressed */
            int down = sd_pressed(i) ? 2 : 0, top = y0 + SD_TILE - 3 + down;
            for (int x = 2; x < 14; x++) for (int y = 0; y < 3 - down; y++) sd_px(f, x0 + x, top + y, (uint8_t)(R_STONE + (y == 0 ? 12 : 8) - (x == 2 || x == 13)), 18);
            if (down) for (int x = 4; x < 12; x++) sd_px(f, x0 + x, top - 1, (uint8_t)(R_GOLD + 9), 24);
        }
    }
    for (int h = 0; h < L->nhz; h++) {                            /* the traps, each in its own shape */
        int ph = SD.hz_ph[h];
        if (!ph || sd_fx_kind(L->hz[h].kind)) continue;          /* (run I's that pour and drift: sd_draw_fx) */
        const SdHaz *z = &L->hz[h];
        int ramp = z->kind == HZ_VOID ? R_PURPLE : z->kind == HZ_ASH ? R_STONE : sd_el_ramp(z->el);
        float w = sd_hz_warn(z), k = SD.hz_k[h];
        for (int c = 0; c < SD.hz_n[h]; c++) {
            int cx, cy;
            if (!sd_hz_cell(z, SD.hz_at[h] + c, &cx, &cy)) continue;
            int x0 = cx * SD_TILE - camx, y0 = cy * SD_TILE - camy, n = SD.hz_n[h];
            switch (z->fam) {
            case HZF_FALL:
                if (ph == 1) {                                     /* grit sifting down, a shadow gathering */
                    for (int d = 0; d < 6; d++) { int dx = (int)(sd_hash(d, c, h) % 14) + 1, dy = (int)(fmodf(k * 60.0f + (float)(sd_hash(d, h, c) % 40), 40.0f)) - 24; sd_px(f, x0 + dx, y0 + dy, (uint8_t)(R_STONE + 9), 20); }
                    int sw = (int)(k / w * 6.0f);
                    for (int x = 8 - sw; x < 8 + sw; x++) sd_px(f, x0 + x, y0 + SD_TILE - 1, 0, 0);
                } else {                                           /* down: a block falling, then rubble lying */
                    int fy = k < 0.15f ? (int)(-30.0f + k / 0.15f * 30.0f) : 0;
                    for (int y = 6; y < SD_TILE; y++) for (int x = 3; x < 13; x++) {
                        if (fy == 0 && y < 10 + (int)(sd_hash(x, c, h) % 3)) continue;
                        sd_px(f, x0 + x, y0 + y + fy, (uint8_t)(ramp + 6 + (int)(sd_hash(x, y, h) % 5)), sd_lit((float)(cx * SD_TILE + x), (float)(cy * SD_TILE + y), 16, 1.0f));
                    }
                }
                break;
            case HZF_LINE: {
                float lit = ph == 1 ? k / w * (float)n - (float)c : 1.0f;   /* the cracks wake one by one, in their order */
                if (lit <= 0.0f) break;
                for (int x = 0; x < SD_TILE; x++) if ((sd_hash(x, c, h) % 3) == 0 || x % 5 == 2) sd_px(f, x0 + x, y0 + SD_TILE - 1 - (int)(sd_hash(x, h, c) % 2), (uint8_t)(ramp + clampi((int)(6.0f + 8.0f * fminf(1.0f, lit)), 0, 15)), 24);
                if (ph == 2 || ph == 5) {                          /* the burst: a sheet of its element up from the cracks; embers after */
                    int tall = ph == 2 ? 22 : 6;
                    for (int y = 0; y < tall; y++) for (int x = 1; x < 15; x++) {
                        uint32_t hh = sd_hash(x, y + (int)(SD.t * 30.0f), c * 31 + h);
                        if (hh % (ph == 2 ? 3 : 9)) continue;
                        sd_px(f, x0 + x, y0 + SD_TILE - 1 - y, (uint8_t)(ramp + 15 - y / 3), 24);
                    }
                }
                break;
            }
            case HZF_CREEP: {                                      /* a sheen, cell by cell in its order, settled; drained back */
                float share = ph == 4 ? fmaxf(0.0f, 1.0f - k / (0.22f * (float)n)) : fminf(1.0f, k / (0.22f * (float)n));
                if ((float)c >= share * (float)n) break;
                for (int y = SD_TILE - 4; y < SD_TILE; y++) for (int x = 0; x < SD_TILE; x++)
                    if (((x + y + (int)(SD.t * 4.0f)) % 3) || y > SD_TILE - 3) sd_px(f, x0 + x, y0 + y, (uint8_t)(ramp + (y == SD_TILE - 4 ? 13 : 8)), 22);
                if (sd_hash(c, (int)(SD.t * 3.0f), h) % 4 == 0) sd_px(f, x0 + 5 + (int)(sd_hash(c, h, 1) % 6), y0 + SD_TILE - 6, (uint8_t)(ramp + 14), 24);
                break;
            }
            case HZF_CLOUD: {                                      /* gathering, hanging, lifting */
                float d = ph == 1 ? k / w * 0.35f : ph == 2 ? 0.75f : fmaxf(0.0f, 0.75f - k / 0.8f * 0.75f);
                for (int y = 0; y < SD_TILE; y++) for (int x = 0; x < SD_TILE; x++) {
                    uint32_t hh = sd_hash(x + (int)(SD.t * 9.0f), y - (int)(SD.t * 5.0f), c + h * 17);
                    if ((float)(hh % 100) >= d * 100.0f) continue;
                    sd_px(f, x0 + x, y0 + y, (uint8_t)(ramp + 5 + (int)(hh % 6)), 22);
                }
                break;
            }
            }
        }
    }
    sd_draw_fx(f, camx, camy);
}

/* ---- 2e: run I's traps, each with its sign while idle, and what they pour and breathe ---- */
static void sd_fx_line(Frame *f, float x0, float y0, float x1, float y1, uint8_t a, uint8_t b) {   /* a chain: links of two shades */
    int n = (int)fmaxf(fabsf(x1 - x0), fabsf(y1 - y0)) + 1;
    for (int k = 0; k <= n; k++) {
        float u = (float)k / (float)n, wx = x0 + (x1 - x0) * u, wy = y0 + (y1 - y0) * u;
        sd_px(f, (int)floorf(wx - SD.camx), (int)floorf(wy - SD.camy), ((int)(wx + wy) >> 1) & 1 ? a : b, sd_lit(wx, wy, 22, 1.0f));
    }
}
static void sd_draw_fx(Frame *f, int camx, int camy) {
    const SdLevel *L = SD.L;
    for (int i = 0; i < L->nth; i++) {
        const SdThing *t = &L->th[i];
        if (t->kind != SDT_HAZ) continue;
        int h = t->a, ph = SD.hz_ph[h], sx = t->x * SD_TILE - camx, sy = t->y * SD_TILE - camy;
        const SdHaz *z = &L->hz[h];
        float wx = (float)(t->x * SD_TILE) + 8.0f, wy = (float)(t->y * SD_TILE) + 8.0f;
        switch (z->kind) {
        case HZ_ROCK:                                              /* a crack in the roof over each cell, always; rubble where it fell */
            for (int c = 0; c < z->nc; c++) {
                int x, y; sd_hz_cell(z, c, &x, &y);
                int top = y; while (top > 0 && !sd_solid(x, top - 1) && y - top < 10) top--;
                if (top > 0 && sd_solid(x, top - 1)) {
                    int bx = x * SD_TILE - camx, by = top * SD_TILE - camy;
                    for (int k = 0; k < 14; k++) {                 /* a jagged seam along the roof's underside, and two bits hanging */
                        int px = 1 + k, py = -1 - (int)(sd_hash(k, x, top) % 3);
                        if (sd_hash(k, top, x) % 4) { sd_px(f, bx + px, by + py, 0, 0); sd_px(f, bx + px, by + py - 1, (uint8_t)(R_STONE + 13), 22); }
                    }
                    for (int q = 0; q < 3; q++) { sd_px(f, bx + 5, by + q, (uint8_t)(R_STONE + 12 - q * 2), 22); if (q < 2) sd_px(f, bx + 11, by + q, (uint8_t)(R_STONE + 11 - q * 2), 22); }   /* bits hanging loose */
                }
                if (FX.rubble[z->c0 + c]) {                        /* the rubble on the floor, walked through */
                    int bx = x * SD_TILE - camx, by = y * SD_TILE - camy;
                    for (int k = 0; k < 11; k++) {
                        int px = 1 + (int)(sd_hash(k, x, y) % 14), hgt = 1 + (int)(sd_hash(y, k, x) % 3);
                        for (int q = 0; q < hgt; q++) for (int r = 0; r < 2; r++) sd_px(f, bx + px + r, by + SD_TILE - 1 - q, (uint8_t)(R_STONE + 8 + (int)(sd_hash(k, q, r) % 6) - (q == 0 ? 0 : 1)), sd_lit(wx, wy, 22, 1.0f));
                    }
                }
            }
            break;
        case HZ_VOTIVE: {                                          /* an iron sconce on the wall; its candle tips the painted way and pours */
            float a = FX.tip[h] * (0.6f + 0.25f * (float)imin(4, FX.vn[h])) * (float)FX.vdir[h];
            int lt = sd_lit(wx, wy, 24, 1.0f), bx = sx + 8, by = sy + 10;
            for (int y = -2; y <= 4; y++) for (int x = -1; x <= 0; x++) sd_px(f, bx - 4 * FX.vdir[h] + x, by + y, (uint8_t)(R_IRON + 9 + x), lt);   /* the plate on the wall */
            for (int x = -3; x <= 3; x++) sd_px(f, bx + x, by + 1, (uint8_t)(R_IRON + 11), lt);   /* the cup it stands in */
            for (int x = -2; x <= 2; x++) sd_px(f, bx + x, by + 2, (uint8_t)(R_IRON + 8), lt);
            for (int v = 0; v < 9; v++) for (int u = -1; u <= 1; u++) {   /* the candle, turned on its foot */
                float px = (float)u * cosf(a) + (float)v * sinf(a), py = (float)u * sinf(a) - (float)v * cosf(a);
                sd_px(f, bx + (int)lroundf(px), by + (int)lroundf(py), (uint8_t)(R_BONE + (u < 0 ? 13 : u > 0 ? 9 : 11) - (v > 6 && ph == 2 ? 3 : 0)), lt);
            }
            if (FX.tip[h] < 0.6f) {                                /* lit while it stands: the sign it is there */
                float fx = (float)bx + sinf(a) * 10.5f, fy = (float)by - cosf(a) * 10.5f;
                int fl = (int)(SD.t * 10.0f + (float)h) % 3;
                sd_px(f, (int)fx, (int)fy, (uint8_t)(R_FLAME + 15), 31); sd_px(f, (int)fx, (int)fy - 1, (uint8_t)(R_FLAME + 12), 31);
                if (fl) sd_px(f, (int)fx, (int)fy - 2, (uint8_t)(R_FLAME + 8), 31);
            }
            break;
        }
        case HZ_BREATH:                                            /* the crypt's slab, as a tile; its seam lit as it cracks */
            sd_draw_tile(f, side_tile[ph ? ST_HZ_SLAB_OPEN : ST_HZ_SLAB], sx, sy, 21, 1.0f, 0);
            break;
        case HZ_COLD:                                              /* the crypt's cold crack, as a tile; frost glinting on it */
            sd_draw_tile(f, side_tile[ST_HZ_COLD], sx, sy, 21, 1.0f, 0);
            for (int k = 0; k < 3; k++) if (sd_hash(k, (int)(SD.t * 4.0f), h) % 3 == 0) sd_px(f, sx + 3 + (int)(sd_hash(k, h, 7) % 10), sy + 3 + (int)(sd_hash(h, k, 9) % 10), (uint8_t)(R_GHOST + 15), 31);
            break;
        case HZ_RELIQ: {                                           /* the crypt's reliquary on its plinth; its reach marked on the floor */
            int open = ph == 1 || ph == 2;
            sd_draw_spr(f, open ? SP_RELIQ_O : SP_RELIQ, sx + 8, sy + SD_TILE, 0, sd_lit(wx, wy, 23, 1.0f), 0);
            int r = (int)(sd_fx_radial(z) * SD_TILE + 8.0f);
            for (int d = -r; d <= r; d += 3) if (abs(d) > r - 4 || (abs(d) % 6 == 0 && sd_hash(d, h, 3) % 2))
                sd_px(f, sx + 8 + d, sy + SD_TILE - 1, (uint8_t)(R_PURPLE + (open ? 13 : 8)), open ? 31 : 20);
            break;
        }
        case HZ_BILE:                                              /* run II's: the crypt's grate, as a tile */
            sd_draw_tile(f, side_tile[ST_HZ_GRATE], sx, sy, 21, 1.0f, 0); break;
        case HZ_SLUICE:                                            /* the sluice gate, shut; thrown up in its turn */
            sd_draw_tile(f, side_tile[ph == 2 ? ST_HZ_SLUICE_OPEN : ST_HZ_SLUICE], sx, sy, 21, 1.0f, 0); break;
        case HZ_SAINT:                                             /* the saint in her alcove; her eyes alight as she weeps */
            sd_draw_tile(f, side_tile[ph ? ST_HZ_SAINT_LIT : ST_HZ_SAINT], sx, sy, 21, 1.0f, 0); break;
        case HZ_GALV: {                                            /* standing water over its cells, always; aglow in its warning; the bolt */
            for (int c = 0; c < z->nc; c++) {
                int x, y; sd_hz_cell(z, c, &x, &y);
                int bx = x * SD_TILE - camx, by = y * SD_TILE - camy;
                for (int yy = 9; yy < SD_TILE; yy++) for (int xx = 0; xx < SD_TILE; xx++) {
                    if (((xx + yy) & 1) && yy > 9) continue;       /* half the pixels, the wall behind showing through */
                    uint32_t hh = sd_hash(xx + (int)(SD.t * 6.0f), yy, c);
                    int top = yy == 9, glow = ph == 1 ? (hh % 3 == 0) : ph == 2;
                    sd_px(f, bx + xx, by + yy, glow ? (uint8_t)(R_GLOW + 11 + (int)(hh % 5)) : (uint8_t)(R_GHOST + (top ? 12 : 6 + (int)(hh % 3))), glow ? 31 : 22);
                }
                if (!ph && sd_hash(c, (int)(SD.t * 2.0f), h) % 7 == 0) sd_px(f, bx + 3 + (int)(sd_hash(c, h, 1) % 10), by + 9, (uint8_t)(R_GLOW + 14), 31);   /* a glint now and then */
            }
            if (FX.bolt[h] > 0.0f && z->nc) {                      /* a jagged bolt down from the roof into the first cell */
                int x = 0, y = 0; sd_hz_cell(z, 0, &x, &y);
                int top = y; while (top > 0 && !sd_solid(x, top - 1) && y - top < 12) top--;
                int px = x * SD_TILE + 8 - camx;
                for (int yy = top * SD_TILE; yy < y * SD_TILE + 10; yy++) {
                    px += (int)(sd_hash(yy / 3, (int)(SD.t * 30.0f), h) % 3) - 1;
                    sd_px(f, px, yy - camy, (uint8_t)C_WHITE, 31); sd_px(f, px + 1, yy - camy, (uint8_t)(R_GLOW + 13), 31);
                }
            }
            break;
        }
        case HZ_SILT:                                              /* a mud floor over its cells, always; bubbling as it wakes */
            for (int c = 0; c < z->nc; c++) {
                int x, y; sd_hz_cell(z, c, &x, &y);
                int bx = x * SD_TILE - camx, by = y * SD_TILE - camy;
                for (int yy = 12; yy < SD_TILE; yy++) for (int xx = 0; xx < SD_TILE; xx++)
                    sd_px(f, bx + xx, by + yy, (uint8_t)(R_WOOD + 4 + (int)(sd_hash(xx, yy, c) % 3) + (yy == 12 ? 3 : 0)), sd_lit(wx, wy, 20, 1.0f));
                if (sd_hash(c, (int)(SD.t * (ph ? 6.0f : 1.5f)), h) % 3 == 0) {
                    int bxx = bx + 3 + (int)(sd_hash(c, (int)(SD.t * 2.0f), 5) % 10);
                    sd_px(f, bxx, by + 11, (uint8_t)(R_GOLD + 7), 22); sd_px(f, bxx + 1, by + 11, (uint8_t)(R_GOLD + 9), 22);
                }
            }
            break;
        case HZ_SNARE:                                             /* a web across its cells, just to be made out; gone a while once torn */
            if (FX.web[h] > 0.0f) break;
            for (int c = 0; c < z->nc; c++) {
                int x, y; sd_hz_cell(z, c, &x, &y);
                int bx = x * SD_TILE - camx, by = y * SD_TILE - camy;
                for (int k = 0; k < 16; k++) {                     /* strands corner to corner and across, a hub in the middle */
                    sd_px(f, bx + k, by + k, (uint8_t)(R_BONE + 8), 18); sd_px(f, bx + 15 - k, by + k, (uint8_t)(R_BONE + 8), 18);
                    if (k % 2 == 0) { sd_px(f, bx + 8, by + k, (uint8_t)(R_BONE + 7), 16); sd_px(f, bx + k, by + 8, (uint8_t)(R_BONE + 7), 16); }
                }
                for (int a = 0; a < 24; a++) { float r = 5.0f, an = (float)a * 0.2618f; sd_px(f, bx + 8 + (int)lroundf(cosf(an) * r), by + 8 + (int)lroundf(sinf(an) * r), (uint8_t)(R_BONE + 9), 18); }
            }
            break;
        default: break;
        }
    }
    for (int k = 0; k < FX.nc; k++) {                              /* the censers: the hook, the chain as it hangs, the bowl */
        const SdCenser *c = &FX.c[k];
        sd_px(f, (int)c->px - camx - 1, (int)c->py - camy - 2, (uint8_t)(R_IRON + 9), 16); sd_px(f, (int)c->px - camx, (int)c->py - camy - 2, (uint8_t)(R_IRON + 11), 16);
        for (int l = 0; l < SDC_LINKS; l++) sd_fx_line(f, c->lx[l], c->ly[l], c->lx[l + 1], c->ly[l + 1], (uint8_t)(R_IRON + 13), (uint8_t)(R_IRON + 9));
        int bx = (int)floorf(c->bx) - camx, by = (int)floorf(c->by) - camy, lt = sd_lit(c->bx, c->by, 24, 1.0f);
        for (int y = 0; y < 7; y++) for (int x = -6; x <= 6; x++) {   /* the bowl: iron, rounded under, a rim */
            if (x * x * 4 + (y - 1) * (y - 1) * 10 > 170 || (y == 0 && abs(x) > 5)) continue;
            sd_px(f, bx + x, by + y, (uint8_t)(R_IRON + (y == 0 ? 12 : 9 - y + (x < 0))), lt);
        }
        for (int x = -4; x <= 4; x++) if ((x + (int)(SD.t * 8.0f)) % 3) sd_px(f, bx + x, by - 1, (uint8_t)(R_FLAME + 10 + (x & 3)), 31);   /* its coals */
        sd_px(f, bx, by - 2, (uint8_t)(R_IRON + 12), lt);
    }
    for (int k = 0; k < FX.n; k++) {                               /* the particles */
        const SdPart *p = &FX.p[k];
        int px = (int)floorf(p->x) - camx, py = (int)floorf(p->y) - camy;
        float left = p->life - p->age;
        uint32_t hh = sd_hash(k, (int)(p->age * 20.0f), p->kind);
        if (left < 1.0f && (float)(hh % 100) > left * 100.0f) continue;   /* thinning as it goes */
        switch (p->kind) {
        case PK_WAX: {
            uint8_t c = p->heat > 0.6f ? (uint8_t)(R_FLAME + 9 + (int)(p->heat * 6.0f)) : p->heat > 0.3f ? (uint8_t)(R_WARM + 10 + (int)(p->heat * 8.0f)) : (uint8_t)(R_BONE + 9 + (int)(hh % 2));
            int lt = sd_lit(p->x, p->y, 24, 1.0f);
            sd_px(f, px - 1, py - 1, c, lt); sd_px(f, px, py - 1, c, lt); sd_px(f, px - 1, py, c, lt); sd_px(f, px, py, c, lt);
            break;
        }
        case PK_VAPOUR:
            for (int q = 0; q < 5; q++) { static const int O[5][2] = { { 0, 0 }, { 1, 0 }, { -1, 0 }, { 0, 1 }, { 0, -1 } }; if (q && (hh >> q) % 3) continue;
                sd_px(f, px + O[q][0], py + O[q][1], (uint8_t)(p->harm ? R_MOSS + 8 + (int)(hh % 3) : R_STONE + 10), 26); }
            break;
        case PK_RIME: sd_px(f, px, py, (uint8_t)(R_GHOST + 11 + (int)(hh % 5)), 26); if (p->rest && hh % 3 == 0) sd_px(f, px + 1, py, (uint8_t)(R_GHOST + 13), 26); break;
        case PK_HAZE: sd_px(f, px, py, (uint8_t)(R_PURPLE + (p->harm ? 11 : 8) + (int)(hh % 4)), p->harm ? 31 : 24); if (p->harm && hh % 2) sd_px(f, px + 1, py, (uint8_t)(R_PURPLE + 9), 24); break;
        case PK_SMOKE: if (hh % 4) sd_px(f, px, py, (uint8_t)(R_STONE + 10 + (int)(hh % 3)), 24); if (p->age > 0.3f && hh % 3 == 0) sd_px(f, px + 1, py - 1, (uint8_t)(R_STONE + 9), 22); break;
        case PK_GRIT: sd_px(f, px, py, (uint8_t)(R_STONE + 10), 18); break;
        case PK_BILE: { int lt = sd_lit(p->x, p->y, 24, 1.0f); uint8_t c = (uint8_t)(R_MOSS + (p->harm ? 9 : 7) + (int)(hh % 3));
            sd_px(f, px - 1, py - 1, c, lt); sd_px(f, px, py - 1, c, lt); sd_px(f, px - 1, py, c, lt); sd_px(f, px, py, c, lt); break; }
        case PK_WATER: { int lt = sd_lit(p->x, p->y, 24, 1.0f); uint8_t c = p->rest ? (uint8_t)(R_GHOST + 8 + (int)(hh % 3)) : (uint8_t)(R_GHOST + 12 + (int)(hh % 4));
            sd_px(f, px - 1, py - 1, c, lt); sd_px(f, px, py - 1, (uint8_t)(R_GHOST + 14), lt); sd_px(f, px - 1, py, c, lt); sd_px(f, px, py, c, lt); break; }
        case PK_GAS:
            for (int q = 0; q < 5; q++) { static const int O[5][2] = { { 0, 0 }, { 1, 0 }, { -1, 0 }, { 0, 1 }, { 0, -1 } }; if (q && (hh >> q) % 3) continue;
                sd_px(f, px + O[q][0], py + O[q][1], (uint8_t)(p->harm ? R_GOLD + 6 + (int)(hh % 3) : R_WOOD + 8), 24); }
            break;
        case PK_FLAME: sd_px(f, px, py, (uint8_t)(R_FLAME + 9 + (int)(hh % 7)), 31); sd_px(f, px, py - 1, (uint8_t)(R_FLAME + 12), 31); if (hh % 2) sd_px(f, px + 1, py, (uint8_t)(R_FLAME + 10), 31); break;
        case PK_TEAR: { int lt = sd_lit(p->x, p->y, 26, 1.0f);
            if (p->rest) { sd_px(f, px - 1, py, (uint8_t)(R_PURPLE + 9 + (int)(hh % 2)), lt); sd_px(f, px, py, (uint8_t)(R_GHOST + 13), lt); sd_px(f, px, py - 1, (uint8_t)(R_PURPLE + 10), lt); }
            else sd_px(f, px, py, (uint8_t)(R_GHOST + 15), 31);
            break; }
        case PK_SPARK: sd_px(f, px, py, (uint8_t)(R_GLOW + 12 + (int)(hh % 4)), 31); break;
        case PK_STRAND: if (hh % 2) sd_px(f, px, py, (uint8_t)(R_BONE + 9), 20); break;
        }
    }
}

/* ---- the powerups (placeholders till the owner's art): quickened hands a gold double arrow, might a red fist of flame, a ward a pale
 * shield; on the caster while they last; their time left in the corner ---- */
static void sd_draw_power(Frame *f, int kind, int cx, int cy, float k) {
    static const char *const ICON[NPW][9] = {
        { "..#...#..", "...#...#.", "....#...#", ".....#...", "....#...#", "...#...#.", "..#...#..", ".........", "........." },
        { "...###...", "..#####..", ".###.###.", ".#######.", ".#######.", "..#####..", "...###...", "....#....", "........." },
        { ".#######.", ".#.....#.", ".#.###.#.", ".#.###.#.", ".#..#..#.", "..#...#..", "...#.#...", "....#....", "........." } };
    static const int RAMP[NPW] = { R_GOLD, R_FLAME, R_GHOST };
    for (int r = -6; r <= 6; r++) for (int c = -6; c <= 6; c++) {   /* a ring of its colour about it */
        int d2 = r * r + c * c;
        if (d2 >= 30 && d2 <= 38 && ((r + c + (int)(SD.t * 8.0f)) & 1)) sd_px(f, cx + c, cy + r, (uint8_t)(RAMP[kind] + (int)(10.0f * k)), 31);
    }
    for (int r = 0; r < 9; r++) for (int c = 0; c < 9; c++)
        if (ICON[kind][r][c] == '#') sd_px(f, cx - 4 + c, cy - 4 + r, (uint8_t)(RAMP[kind] + 13 + ((r + c) & 1) * 2), 31);
}
static void sd_render(Frame *f) {
    const SdLevel *L = SD.L;
    int camx = (int)floorf(SD.camx), camy = (int)floorf(SD.camy);
    /* the lights: the torches, the doorways, the caster's own, what flies */
    sdl_n = 0;
    float flick = 0.85f + 0.15f * sinf(SD.t * 11.0f) * sinf(SD.t * 7.3f);
    for (int i = 0; i < L->nth; i++) {
        const SdThing *t = &L->th[i];
        float x = (float)(t->x * SD_TILE) + 8.0f, y = (float)(t->y * SD_TILE) + 8.0f;
        if (t->kind == SDT_TORCH) sd_light(x, y - 4.0f, 96.0f, 9.0f * flick);
        else if (t->kind == SDT_DOOR) sd_light(x, y, 54.0f, (t->flags & SDF_GOAL) ? 8.0f : 5.0f);
        else if (t->kind == SDT_LIFE && !SD.taken[i]) sd_light(x, y, 30.0f, 4.0f);
    }
    sd_light(SD.x, SD.y - 14.0f, 64.0f, 4.0f);
    for (int i = 0; i < SD_SHOTS; i++) if (SD.shot[i].alive && !SD.shot[i].foe) sd_light(SD.shot[i].x, SD.shot[i].y, 48.0f, 6.0f);
    if (SD.beam_t > 0.0f) sd_light((SD.bx0 + SD.bx1) * 0.5f, (SD.by0 + SD.by1) * 0.5f, 80.0f, 10.0f);

    /* the far wall, at half the pace, in the dark */
    for (int y = 0; y < SD_VIEW_H; y += 4)
        for (int x = 0; x < W; x += 4) {
            int light = 4 + (int)fminf(4.0f, sd_glow(SD.camx + (float)x, SD.camy + (float)y) * 0.4f);
            for (int yy = y; yy < y + 4 && yy < SD_VIEW_H; yy++)
                for (int xx = x; xx < x + 4; xx++) {
                    int fx = ((xx + camx / 2) % STS + STS) % STS, fy = ((yy + camy / 2) % STS + STS) % STS;
                    f->pix[yy * W + xx] = crawl_cmap[light][side_tile[ST_FAR][fy * STS + fx]];
                }
        }
    int tx0 = camx / SD_TILE, ty0 = camy / SD_TILE, tx1 = (camx + W) / SD_TILE, ty1 = (camy + SD_VIEW_H) / SD_TILE;
    /* the back wall, and behind the doorways */
    for (int ty = ty0; ty <= ty1; ty++)
        for (int tx = tx0; tx <= tx1; tx++) {
            if (sd_solid_ch(sd_tile(tx, ty))) continue;
            int b = sd_back_tile(tx, ty);
            if (b >= 0) sd_draw_tile(f, side_tile[b], tx * SD_TILE - camx, ty * SD_TILE - camy, 8, 0.7f, 0);
        }
    /* what stands on the back wall: doorways, torches, levers */
    for (int i = 0; i < L->nth; i++) {
        const SdThing *t = &L->th[i];
        int sx = t->x * SD_TILE - camx, sy = t->y * SD_TILE - camy;
        switch (t->kind) {
        case SDT_DOOR: {
            int goal = (t->flags & SDF_GOAL) != 0;
            sd_draw_tile(f, side_tile[goal ? ST_GOAL_T : ST_DOOR_T], sx, sy - SD_TILE, 20, 1.0f, 0);
            sd_draw_tile(f, side_tile[goal ? ST_GOAL_B : ST_DOOR_B], sx, sy, 20, 1.0f, 0);
            break;
        }
        case SDT_TORCH: sd_draw_tile(f, side_tile[ST_TORCH0 + (int)(SD.t * 9.0f + (float)i) % 3], sx, sy, 24, 0.0f, 0); break;
        case SDT_SWITCH:
            sd_draw_tile(f, side_tile[SD.thrown[i] ? ST_SWITCH1 : ST_SWITCH0], sx, sy, 18, 1.0f, 0);
            if (t->f[0] > 0.0f) {                                  /* a timed one: its dial, a ring of marks going out as it runs */
                float frac = SWT[i] > 0.0f ? SWT[i] / t->f[0] : 1.0f;
                for (int a = 0; a < 16; a++) {
                    float an = (float)a / 16.0f * TAU_F - PI_F * 0.5f;
                    int lit = (float)a / 16.0f < frac;
                    sd_px(f, sx + 8 + (int)lroundf(cosf(an) * 6.0f), sy - 4 + (int)lroundf(sinf(an) * 3.0f), lit ? (uint8_t)(SWT[i] > 0.0f ? R_FLAME + 12 : R_GOLD + 9) : (uint8_t)(R_STONE + 6), lit ? 31 : 16);
                }
            }
            break;
        case SDT_LEDGE:                                            /* off: its length marked faintly (on, the tiles draw it) */
            if (!SD.st[i]) for (int k = 0; k < t->w * SD_TILE; k += 2) sd_px(f, sx + k, sy, (uint8_t)(R_WOOD + 9), 16);
            break;
        default: break;
        }
    }
    /* the tiles */
    for (int ty = ty0; ty <= ty1; ty++)
        for (int tx = tx0; tx <= tx1; tx++) {
            int ch = sd_tile(tx, ty), t = -1;
            uint32_t h = (uint32_t)(tx * 2654435761u) ^ (uint32_t)(ty * 40503u);
            switch (ch) {
            case '#': t = !sd_solid_ch(sd_tile(tx, ty - 1)) ? ST_STONE_TOP : ST_STONE0 + (int)(h % 3); break;
            case '%': t = !sd_solid_ch(sd_tile(tx, ty - 1)) ? ST_BRICK_TOP : ST_BRICK; break;
            case '=': t = ST_LEDGE; break;
            case '^': t = ST_SPIKES; break;
            case '~': t = sd_tile(tx, ty - 1) == '~' ? ST_WATER_DEEP : ST_WATER0 + ((int)(SD.t * 6.0f) + tx) % 4; break;
            default: break;
            }
            if (t >= 0) sd_draw_tile(f, side_tile[t], tx * SD_TILE - camx, ty * SD_TILE - camy, ch == '~' ? 20 : 17, 1.0f, 0);
        }
    /* the flows, as high as they have risen */
    for (int i = 0; i < L->nth; i++) {
        const SdThing *t = &L->th[i];
        if (t->kind != SDT_FLOW) continue;
        float k = sd_flow_k(t);
        if (k <= 0.02f) continue;
        int top = (int)floorf(sd_flow_top(t)), y1 = (t->y + t->h) * SD_TILE;
        for (int tx = t->x; tx < t->x + t->w; tx++)
            for (int y = top, first = 1; y < y1; y += SD_TILE, first = 0) {
                const uint8_t *tile = side_tile[first ? ST_WATER0 + ((int)(SD.t * 6.0f) + tx) % 4 : ST_WATER_DEEP];
                int sx = tx * SD_TILE - camx, sy = y - camy - (first ? 3 : 0);
                for (int yy = 0; yy < STS && y + yy - (first ? 3 : 0) < y1; yy++)
                    for (int xx = 0; xx < STS; xx++) sd_px(f, sx + xx, sy + yy, tile[yy * STS + xx], 22);
            }
    }
    sd_draw_works(f, camx, camy);                                  /* water running, gates, plates, the traps */
    /* the falling blocks, shaking and falling */
    for (int i = 0; i < L->nth; i++) {
        const SdThing *t = &L->th[i];
        if (t->kind != SDT_FALL || SD.fall[i].state == 3) continue;
        if (!SD.st[i]) {                                           /* off: only its outline, faint, where it will come */
            int bx = t->x * SD_TILE - camx, by = t->y * SD_TILE - camy;
            for (int k = 0; k < SD_TILE; k += 2) { sd_px(f, bx + k, by, (uint8_t)(R_STONE + 8), 16); sd_px(f, bx + k, by + SD_TILE - 1, (uint8_t)(R_STONE + 8), 16);
                sd_px(f, bx, by + k, (uint8_t)(R_STONE + 8), 16); sd_px(f, bx + SD_TILE - 1, by + k, (uint8_t)(R_STONE + 8), 16); }
            continue;
        }
        const SdFall *fl = &SD.fall[i];
        int shake = fl->state == 1 ? ((int)(fl->t * 40.0f) & 1 ? 1 : -1) : 0;
        sd_draw_tile(f, side_tile[fl->state >= 1 ? ST_FALL_CRACK : ST_FALL], t->x * SD_TILE - camx + shake, t->y * SD_TILE - camy + (int)fl->dy, 17, 1.0f, 0);
    }
    /* the platforms, each on its chains up to the stone above */
    for (int i = 0; i < L->nth; i++) {
        const SdThing *t = &L->th[i];
        if (t->kind != SDT_PLAT) continue;
        const SdPlat *p = &SD.plat[i];
        int px = (int)floorf(p->x) - camx, py = (int)floorf(p->y) - camy;
        for (int side = 0; side < 2; side++) {
            float cx = p->x + (side ? (float)(t->w * SD_TILE) - 12.0f : 4.0f);
            int top = sd_floor_px(p->y) - 1;
            while (top > 0 && !sd_solid(sd_floor_px(cx), top)) top--;
            for (int y = (top + 1) * SD_TILE; y < (int)p->y; y += SD_TILE)
                sd_draw_tile(f, side_tile[ST_CHAIN], (int)cx - 4 - camx, y - camy - ((int)p->y - y < SD_TILE ? SD_TILE - ((int)p->y - y) : 0), 14, 1.0f, 0);
        }
        for (int k = 0; k < t->w; k++) sd_draw_tile(f, side_tile[t->w == 1 ? ST_PLAT_M : k == 0 ? ST_PLAT_L : k == t->w - 1 ? ST_PLAT_R : ST_PLAT_M], px + k * SD_TILE, py, 18, 1.0f, 0);
    }
    /* what lies to be found */
    for (int i = 0; i < L->nth; i++) {
        const SdThing *t = &L->th[i];
        if (SD.taken[i]) continue;
        int loose = t->flags & SDF_LOOSE, bob = loose ? 0 : (int)floorf(1.5f * sinf(SD.t * 3.0f + (float)i));
        int sx = (int)floorf(SD.ix[i]) - 8 - camx, sy = (int)floorf(SD.iy[i]) + 5 - SD_TILE - camy + bob;   /* where it lies now: loose things move */
        switch (t->kind) {
        case SDT_KEY: sd_draw_tile(f, side_tile[ST_KEY0 + t->a], sx, sy, 24, 1.0f, 0); break;
        case SDT_LIFE: sd_draw_tile(f, side_tile[ST_LIFE], sx, sy - bob, 24, 1.0f, 0); break;
        case SDT_POWER: sd_draw_power(f, t->a, sx + 8, sy + 8 - bob, 1.0f); break;
        case SDT_GOLD: {
            static const int SPR[] = { SP_COINS_A, SP_CHALICE, SP_GEMS, SP_CROWN, SP_CHEST };
            sd_draw_spr(f, SPR[clampi(t->a, 0, 4)], sx + 8, sy + 16, 0, 24, 0);   /* its foot on the tile's foot (it was drawn a tile low) */
            break;
        }
        default: break;
        }
    }
    /* the dead: their bestiary pictures from the side, until their own are drawn (step 13) */
    for (int i = 0; i < SD_MOBS; i++) {
        const SdMob *m = &SD.mob[i];
        if (!m->alive) continue;
        if (m->state == SDM_DIE && ((int)(m->t * 16.0f) & 1)) continue;
        int spr = ((int)(m->t * 5.0f) & 1) ? KINDS[m->kind].side2 : KINDS[m->kind].side;
        if (spr <= 0) spr = KINDS[m->kind].spr;
        int sink = m->state == SDM_DIE ? (int)(m->t * 12.0f) : 0;
        sd_draw_spr(f, spr, (int)m->x - camx, (int)m->y - camy + sink + (TS / 4 - (int)sd_mob_h(m) / 2 > 0 ? 0 : 0), m->face < 0, sd_lit(m->x, m->y - 10.0f, 18, 1.0f), m->hurt_t > 0.1f);
    }
    /* the shots */
    for (int i = 0; i < SD_SHOTS; i++) {
        const SdShot *s = &SD.shot[i];
        if (!s->alive) continue;
        if (s->foe || s->kind == SS_BONE) { sd_blob(f, s->x, s->y, 1.6f, R_BONE, 14); continue; }
        if (s->kind == SS_FLASK) { sd_blob(f, s->x, s->y, 2.2f, R_MOSS, 13); continue; }
        int ramp = s->el == EL_FIRE ? R_FLAME : s->el == EL_FROST ? R_GLOW : s->el == EL_SPIRIT ? R_GHOST : s->el == EL_PHYS ? R_BONE : R_GLOW;
        for (int k = 3; k >= 0; k--) sd_blob(f, s->x - s->vx * 0.012f * (float)k, s->y, 2.4f - 0.5f * (float)k, ramp, 15 - k * 2);
    }
    if (SD.beam_t > 0.0f) {                                        /* lightning: jagged, a moment */
        float x = SD.bx0, y = SD.by0;
        int n = imax(2, (int)(fabsf(SD.bx1 - SD.bx0) / 6.0f));
        for (int k = 1; k <= n; k++) {
            float nx = SD.bx0 + (SD.bx1 - SD.bx0) * (float)k / (float)n, ny = SD.by0 + (SD.by1 - SD.by0) * (float)k / (float)n + (k < n ? (float)((k * 7 + (int)(SD.t * 60.0f)) % 7 - 3) : 0.0f);
            for (int j = 0; j <= 6; j++) { float a = (float)j / 6.0f; sd_px(f, (int)(x + (nx - x) * a) - camx, (int)(y + (ny - y) * a) - camy, R_GLOW + 15, 24); }
            x = nx; y = ny;
        }
    }
    if (SD.holding) {                                              /* the flask held: its arc dotted, and where it lands */
        float vx, vy, x = SD.x + (float)SD.face * 6.0f, y = SD.y - SD_HAND;
        sd_flask_way(sd_flask_k(), &vx, &vy);
        for (int i = 0; i < 400; i++) {
            float nx = x + vx * 0.01f, ny = y + vy * 0.01f;
            vy += SD_G * 0.01f;
            if (sd_solid(sd_floor_px(nx), sd_floor_px(ny)) || ny > (float)(L->h * SD_TILE)) break;
            x = nx; y = ny;
            if (i % 4 == 0) sd_px(f, (int)x - camx, (int)y - camy, R_MOSS + 13, 24);
        }
        int mx = (int)SD.mark_x - camx, my = (int)SD.mark_y - camy;
        for (int k = -3; k <= 3; k++) { sd_px(f, mx + k, my + k, R_MOSS + 15, 24); sd_px(f, mx + k, my - k, R_MOSS + 15, 24); }
    }
    /* the caster */
    if (!(SD.inv_t > 0.0f && !SD.dying && ((int)(SD.inv_t * 14.0f) & 1))) {
        int sx = (int)floorf(SD.x) - camx - SF_W / 2, sy = (int)floorf(SD.y) - camy - SF_H;
        int light = sd_lit(SD.x, SD.y - 14.0f, 17, 1.0f);
        const uint8_t *fr = side_fig[clampi(SD.frame, 0, NSF - 1)];
        for (int y = 0; y < SF_H; y++)
            for (int x = 0; x < SF_W; x++) {
                uint8_t c = fr[y * SF_W + (SD.face < 0 ? SF_W - 1 - x : x)];
                if (c != C_CLEAR) sd_px(f, sx + x, sy + y, SD.pois_k > 0.0f && c != R_STONE && ((x + y) & 3) == 0 ? (uint8_t)(R_MOSS + 9) : c, light);
            }
        for (int k = 0; k < NPW; k++) {                            /* a powerup on the caster: an edge of its colour, flickering; sparks at
                                                                      the hand for quickened hands */
            if (FX.pw[k] <= 0.0f) continue;
            float left = FX.pw[k];
            if (left < 2.0f && ((int)(SD.t * 10.0f) & 1)) continue; /* (running out: it blinks) */
            static const int RAMP[NPW] = { R_GOLD, R_FLAME, R_GHOST };
            for (int y = -1; y <= SF_H; y++) for (int x = -1; x <= SF_W; x++) {
                int inside = x >= 0 && y >= 0 && x < SF_W && y < SF_H && fr[y * SF_W + (SD.face < 0 ? SF_W - 1 - x : x)] != C_CLEAR;
                if (inside) continue;
                int near = 0;
                for (int d = 0; d < 4 && !near; d++) {
                    int nx = x + (d == 0) - (d == 1), ny = y + (d == 2) - (d == 3);
                    near = nx >= 0 && ny >= 0 && nx < SF_W && ny < SF_H && fr[ny * SF_W + (SD.face < 0 ? SF_W - 1 - nx : nx)] != C_CLEAR;
                }
                if (!near || (k == PW_RAPID) || ((x * 3 + y + (int)(SD.t * 12.0f)) % (k == PW_WARD ? 2 : 3)) == 0) continue;
                sd_px(f, sx + x, sy + y, (uint8_t)(RAMP[k] + 10 + (int)(sd_hash(x, y, (int)(SD.t * 10.0f)) % 5)), 31);
            }
            if (k == PW_RAPID) for (int s = 0; s < 3; s++) {
                float a = SD.t * 9.0f + (float)s * 2.1f;
                sd_px(f, (int)floorf(SD.x) - camx + SD.face * 6 + (int)lroundf(cosf(a) * 3.0f), (int)floorf(SD.y - SD_HAND) - camy + (int)lroundf(sinf(a) * 3.0f), (uint8_t)(R_GOLD + 14), 31);
            }
        }
    }
    for (int k = 0, n = 0; k < NPW; k++) {                         /* their time left, top right: the icon and a bar running down */
        if (FX.pw[k] <= 0.0f) continue;
        int cx = W - 14 - n * 26, cy = 10;
        sd_draw_power(f, k, cx, cy, 1.0f);
        int bw = (int)(18.0f * FX.pw[k] / fmaxf(FX.pw_full[k], 0.1f));
        for (int x = 0; x < 18; x++) f->pix[(cy + 9) * W + cx - 9 + x] = x < bw ? (uint8_t)(k == PW_RAPID ? R_GOLD + 13 : k == PW_MIGHT ? R_FLAME + 12 : R_GHOST + 13) : (uint8_t)(R_STONE + 4);
        n++;
    }
    /* the lives, top left: a lamp each, the lost ones cold */
    for (int k = 0; k < SD_LIVES; k++) {
        const uint8_t *t = side_tile[ST_LIFE];
        for (int y = 2; y < STS; y++) for (int x = 2; x < 14; x++) {
            uint8_t c = t[y * STS + x];
            if (c == C_CLEAR) continue;
            if (k >= sd_lives()) c = IS_FULLBRIGHT(c) ? C_CLEAR : (uint8_t)(R_STONE + 5);
            if (c != C_CLEAR) f->pix[(y + 1) * W + 4 + k * 13 + x - 2] = c;
        }
    }
    if (SD.in_t < 3.0f) {                                          /* its name, as you come in */
        int w = gfx_text_width(L->name, 1);
        gfx_text8(f, (W - w) / 2 + 1, 13, L->name, 0, 1);
        gfx_text8(f, (W - w) / 2, 12, L->name, R_GOLD + 11, 1);
    }
    for (int y = SD_VIEW_H; y < H; y++) memset(f->pix + y * W, 0, W);   /* under the bar */
}
