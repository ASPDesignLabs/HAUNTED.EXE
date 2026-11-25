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
        case SDT_SWITCH: sd_draw_tile(f, side_tile[SD.thrown[i] ? ST_SWITCH1 : ST_SWITCH0], sx, sy, 18, 1.0f, 0); break;
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
    /* the falling blocks, shaking and falling */
    for (int i = 0; i < L->nth; i++) {
        const SdThing *t = &L->th[i];
        if (t->kind != SDT_FALL || SD.fall[i].state == 3) continue;
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
        int bob = (int)floorf(1.5f * sinf(SD.t * 3.0f + (float)i));
        int sx = t->x * SD_TILE - camx, sy = t->y * SD_TILE - camy + bob;
        switch (t->kind) {
        case SDT_KEY: sd_draw_tile(f, side_tile[ST_KEY0 + t->a], sx, sy, 24, 1.0f, 0); break;
        case SDT_LIFE: sd_draw_tile(f, side_tile[ST_LIFE], sx, sy - bob, 24, 1.0f, 0); break;
        case SDT_GOLD: {
            static const int SPR[] = { SP_COINS_A, SP_CHALICE, SP_GEMS, SP_CROWN, SP_CHEST };
            sd_draw_spr(f, SPR[clampi(t->a, 0, 4)], sx + 8, sy + 16 + (TS / 4), 0, 24, 0);
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
        float vx, vy, x = SD.x + (float)SD.face * 6.0f, y = SD.y - 18.0f;
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
