/* A part of scene_crawl.c (docs/PLAN-tidy.md): it is #included there in its place and never compiled on its own.
 * The map on its scroll. */

/* ---- the map on its scroll (docs/PLAN-b9.md, section 1) ----
 * Parchment between two rollers across the middle eight-tenths of the view, torn top and bottom, a
 * burn or two, stained; made once a level, so it holds still. On it, in brown ink and seen from above,
 * north up: every wall face between floor and stone that has come within three cells of you, the doors
 * (a vault's in its key's colour once you have been near it), the exit, keys seen, the Black Altar, a
 * secret's doorway once opened, and you, in red. A secret still shut is drawn as the stone it looks like. */
static uint8_t g_scroll[H][W];
static int g_scroll_for = -1;
static float map_noise(float x, int s) {
    int i = (int)floorf(x);
    float t = x - (float)i;
    t = t * t * (3.0f - 2.0f * t);
    float a = (float)(hash2(i, s) & 0xffff) / 65535.0f, b = (float)(hash2(i + 1, s) & 0xffff) / 65535.0f;
    return a + (b - a) * t;
}
static float map_noise2(float x, float y, int s) {
    int j = (int)floorf(y);
    float t = y - (float)j;
    t = t * t * (3.0f - 2.0f * t);
    float a = map_noise(x, s + j * 7919), b = map_noise(x, s + (j + 1) * 7919);
    return a + (b - a) * t;
}
#define SCROLL_X0 40
#define SCROLL_X1 279
/* The parchment, made once a level from its seed (docs/PLAN-b11.md, section 1): solid, but torn in from its
 * edges and burnt through near its middle, about a quarter of it in all; the crypt shows through the holes */
static void map_scroll_make(int seed) {
    memset(g_scroll, 0, sizeof g_scroll);
    int top[W], bot[W], tx[6], tw[6], td[6], hx[2], hy[2], hr[2], nb = 1 + (int)(hash2(3, seed + 9) & 1u);
    for (int k = 0; k < 6; k++) {                                  /* three tears from the top, three from the bottom */
        tx[k] = SCROLL_X0 + 10 + (int)(hash2(k, seed + 11) % 220u);
        if (k < 3 && abs(tx[k] - W / 2) < 66) tx[k] += tx[k] < W / 2 ? -66 : 66;   /* the name stays whole */
        tw[k] = 9 + (int)(hash2(k, seed + 12) % 14u); td[k] = 18 + (int)(hash2(k, seed + 13) % 26u);
    }
    for (int k = 0; k < 2; k++) {                                  /* and a burn or two near the middle */
        hx[k] = W / 2 + (k ? 1 : -1) * (24 + (int)(hash2(k, seed + 14) % 44u));
        hy[k] = 70 + (int)(hash2(k, seed + 15) % 50u);
        hr[k] = 13 + (int)(hash2(k, seed + 16) % 9u);
    }
    for (int x = SCROLL_X0; x <= SCROLL_X1; x++) {
        float t = 8.0f + 3.0f * map_noise((float)x / 7.0f, seed) + 1.2f * map_noise((float)x / 2.0f, seed + 1);
        float b = 180.0f - 3.0f * map_noise((float)x / 7.0f, seed + 2) - 1.2f * map_noise((float)x / 2.0f, seed + 3);
        for (int k = 0; k < 6; k++) {
            int d = abs(x - tx[k]);
            if (d >= tw[k]) continue;
            float v = (float)td[k] * powf(1.0f - (float)d / (float)tw[k], 0.8f) + 3.0f * map_noise((float)x / 1.7f, seed + 20 + k);   /* jagged */
            if (k < 3) t = fmaxf(t, 8.0f + v); else b = fminf(b, 180.0f - v);
        }
        top[x] = (int)t; bot[x] = (int)b;
    }
    for (int y = 0; y < H; y++)
        for (int x = SCROLL_X0; x <= SCROLL_X1; x++) {
            if (y < top[x] || y > bot[x]) continue;
            float hole = 99.0f;
            for (int k = 0; k < nb; k++) {                         /* a ragged ring of char round each burn */
                float a = atan2f((float)(y - hy[k]), (float)(x - hx[k]));
                float r = (float)hr[k] * (1.0f + 0.28f * (map_noise(a * 2.2f + 9.0f, seed + 30 + k) - 0.5f) + 0.12f * (map_noise(a * 6.0f + 3.0f, seed + 40 + k) - 0.5f));
                hole = fminf(hole, hypotf((float)(x - hx[k]), (float)(y - hy[k]) * 1.15f) - r);
            }
            if (hole < 0.0f) continue;
            int edge = imin(y - top[x], bot[x] - y);
            float v = 0.68f + 0.16f * (map_noise2((float)x / 23.0f, (float)y / 17.0f, seed + 5) - 0.5f) + 0.05f * ((float)(hash2(x, y + seed) & 255u) / 255.0f - 0.5f);
            float st = 0.65f * map_noise2((float)x / 11.0f, (float)y / 9.0f, seed + 7) + 0.35f * map_noise2((float)x / 4.0f, (float)y / 5.0f, seed + 8);
            if (st > 0.68f) v -= (st - 0.68f) * 1.0f;              /* old stains */
            int side = imin(x - SCROLL_X0, SCROLL_X1 - x);
            if (side < 6) v -= 0.03f * (float)(6 - side);          /* curling into the rollers */
            uint8_t c = ramp_at(R_BONE, (int)(v * 15.0f + 0.5f));
            if (edge < 1 || hole < 1.0f) c = R_WOOD + 2;           /* scorched and frayed at the edges */
            else if (edge < 3 || hole < 2.2f) c = R_WOOD + 7;
            else if (hole < 3.5f) c = ramp_at(R_BONE, (int)(v * 15.0f + 0.5f) - 4);   /* browned near the burn */
            g_scroll[y][x] = c;
        }
    g_scroll_for = seed;
}
static void map_roller(Frame *f, int cx) {
    for (int y = 2; y <= 186; y++) {
        int paper = y >= 9 && y <= 179;
        float r = paper ? 4.6f : 2.0f;
        for (int x = cx - 5; x <= cx + 5; x++) {
            float d = ((float)x + 0.5f - (float)cx) / r;
            if (fabsf(d) > 1.0f) continue;
            float lit = sqrtf(1.0f - d * d) * (0.8f - 0.3f * d);   /* lit from the left */
            gfx_px(f, x, y, paper ? ramp_at(R_BONE, 3 + (int)(lit * 7.0f) - (y % 9 == 0)) : ramp_at(R_WOOD, 1 + (int)(lit * 7.0f)));
        }
    }
    for (int k = 0; k < 2; k++) {                                  /* the knobs, a gold stud on each */
        int ky = k ? 185 : 3;
        for (int y = ky - 3; y <= ky + 3; y++)
            for (int x = cx - 4; x <= cx + 4; x++) {
                float nx = ((float)x + 0.5f - (float)cx) / 3.6f, ny = ((float)y + 0.5f - (float)ky) / 2.8f, d = nx * nx + ny * ny;
                if (d <= 1.0f) gfx_px(f, x, y, ramp_at(R_WOOD, 2 + (int)(5.0f * sqrtf(1.0f - d) * (0.8f - 0.3f * nx - 0.2f * ny))));
            }
        gfx_px(f, cx, ky, R_GOLD + 9); gfx_px(f, cx - 1, ky - 1, R_GOLD + 12);
    }
}
static int map_hidden(int x, int y) {                              /* a secret still shut looks like stone */
    for (int i = 0; i < G.nsecrets; i++) {
        const Secret *sc = &G.secrets[i];
        if (sc->state) continue;
        if ((x == sc->sx && y == sc->sy) || (x == sc->door_x && y == sc->door_y) || (x >= sc->x0 && x <= sc->x1 && y >= sc->y0 && y <= sc->y1)) return 1;
    }
    return 0;
}
static int map_open(int x, int y) {
    if (x < 0 || y < 0 || x >= MAPN || y >= MAPN) return 0;
    int k = G.map[y][x].kind;
    return (k == K_FLOOR || k == K_DOOR || k == K_PROP) && !map_hidden(x, y);
}
static int map_bx, map_by;                                          /* where cell (0, 0) falls on the screen */
static uint8_t map_ghost[H][W];                                     /* ink over a hole in the parchment, drawn last, eerie */
static void ink_px(Frame *f, int x, int y, uint8_t c, int x0, int y0, int x1, int y1) {   /* ink on the paper, ghost-light over a hole */
    if (x < x0 || x >= x1 || y < y0 || y >= y1) return;
    if (g_scroll[y][x]) f->pix[y * W + x] = c; else map_ghost[y][x] = 1;
}
static void map_px(Frame *f, int x, int y, uint8_t c) { ink_px(f, x, y, c, MAP_VX0, MAP_VY0, MAP_VX0 + MAP_VW, MAP_VY0 + MAP_VH); }   /* the map's part */
static int map_text(Frame *f, int x, int y, const char *t, uint8_t c) {   /* the font, inked; returns the width */
    int x0 = x;
    for (; *t; t++, x += 8)
        for (int r = 0; r < 8; r++)
            for (int k = 0; k < 8; k++)
                if (font_bios8[(uint8_t)*t][r] >> (7 - k) & 1) ink_px(f, x + k, y + r, c, SCROLL_X0 + 4, 0, SCROLL_X1 - 3, H);
    return x - x0;
}
static void map_ghostly(Frame *f) {                                /* the ink crossing a hole: pale ghost-light, a faint violet edge, adrift */
    float t = G.ui_t;
    int pulse = (int)(1.5f + 1.5f * sinf(t * 1.7f));
    for (int pass = 0; pass < 2; pass++)                           /* the edge first, then the light over it */
        for (int y = 1; y < H - 1; y++) {
            int slip = fmodf(t + (float)(y >> 3) * 0.37f, 2.6f) < 0.1f ? 1 : 0;   /* now and then a band of it drifts a pixel */
            for (int x = SCROLL_X0; x <= SCROLL_X1; x++) {
                if (!map_ghost[y][x]) continue;
                if (pass == 1) { if (!g_scroll[y][x + slip]) f->pix[y * W + x + slip] = (uint8_t)(R_GHOST + 12 + pulse); continue; }
                for (int d = 0; d < 4; d++) {
                    int nx = x + DX[d], ny = y + DY[d];
                    if (!map_ghost[ny][nx] && !g_scroll[ny][nx + slip]) f->pix[ny * W + nx + slip] = (uint8_t)(R_PURPLE + 5);
                }
            }
        }
}
static void map_face(Frame *f, int wx, int wy, uint8_t c) {        /* a mark on a wall's face, two thick, where it meets the floor */
    for (int d = 0; d < 4; d++) {
        int nx = wx + DX[d], ny = wy + DY[d];
        if (!map_open(nx, ny) || !(G.mapped[ny][nx] & 1)) continue;
        int px = map_bx + nx * MAP_PX, py = map_by + ny * MAP_PX;
        for (int i = 1; i < MAP_PX; i++)
            for (int t = 0; t < 2; t++) {
                if (d == 0) map_px(f, px + t, py + i, c);                      /* the wall is west of the floor */
                else if (d == 2) map_px(f, px + MAP_PX - t, py + i, c);        /* east */
                else if (d == 1) map_px(f, px + i, py + t, c);                 /* north */
                else map_px(f, px + i, py + MAP_PX - t, c);                    /* south */
            }
    }
}
/* the level's numbers (section 1), across the scroll's foot: the creatures the tally counts still about, the
 * treasure still lying (rounded, the Treasure bonus in it), the secrets found of all, and the keys */
static int map_creatures(void) {
    int n = 0;
    for (int i = 0; i < MAXMOBS; i++) {
        const Mob *m = &G.mobs[i];
        n += m->alive && m->state != MS_DIE && !KIND_IS(m, KF_UNCOUNTED) && !m->boss && !m->add && !m->ally;
    }
    return n;
}
static int map_gold(void) {
    int v = 0;
    if (PK.depth) { for (int i = 0; i < G.nitems; i++) if (G.items[i].alive && G.items[i].type < IT_KEY) v += G.items[i].value ? G.items[i].value : treasure_value(G.items[i].type); }
    else v = imax(0, G.value_total - G.value_got);
    float g = (float)v * (1.0f + gear_total(&RUN.gear, ST_TREASURE) / 100.0f);
    if (g <= 0.0f) return 0;
    return g < 1000.0f ? imax(50, (int)(g / 50.0f + 0.5f) * 50) : (int)(g / 100.0f + 0.5f) * 100;
}
static void map_key_mark(Frame *f, int x, int y, int color, int held) {   /* solid if held, an outline if still lying */
    static const char *const KEYM[5] = { ".##......", "####.....", "#########", "####...#.", ".##....#." };
    static const uint8_t KC[NKEYS] = { R_GOLD + 6, R_IRON + 10, R_BLOOD + 10 };
    for (int r = 0; r < 5; r++)
        for (int k = 0; k < 9; k++) {
            if (KEYM[r][k] != '#') continue;
            int edge = held;
            for (int d = 0; d < 4 && !edge; d++) {
                int nr = r + DY[d], nk = k + DX[d];
                edge = nr < 0 || nr > 4 || nk < 0 || nk > 8 || KEYM[nr][nk] != '#';
            }
            if (edge) ink_px(f, x + k, y + r, KC[color], SCROLL_X0 + 4, 0, SCROLL_X1 - 3, H);
        }
}
static void map_numbers(Frame *f) {
    const uint8_t ink = R_WOOD + 2;
    char t[48], v[16];
    int y = MAP_VY0 + MAP_VH + 4, xl = SCROLL_X0 + 10, xr = SCROLL_X1 - 10;
    snprintf(t, sizeof t, "CREATURES %d", map_creatures());
    map_text(f, xl, y, t, ink);
    int found = G.links_found, all = G.nlinks;
    if (PK.depth) { found = PK.reached[PK.chain]; all = G_LEVEL.secrets[clampi(PK.chain, 0, MAXSECRETS - 1)].links; }
    if (all > 0) snprintf(t, sizeof t, "SECRETS %d OF %d", imin(found, all), all); else snprintf(t, sizeof t, "NO SECRETS");
    map_text(f, xr - gfx_text_width(t, 1), y, t, ink);
    y += 10;
    int g = map_gold();
    if (g > 0) { commas(g, v, sizeof v); snprintf(t, sizeof t, "GOLD ABOUT %s", v); } else snprintf(t, sizeof t, "NO GOLD LEFT");
    map_text(f, xl, y, t, ink);
    int has[NKEYS] = { 0 }, nk = 0;                                /* the colours this level has: a vault door of each */
    for (int k = 0; k < G.nvaults && !PK.depth; k++) if (G.vault_door[k] >= 0) { int c = clampi(G.doors[G.vault_door[k]].color, 0, NKEYS - 1); if (!has[c]) nk++; has[c] = 1; }
    if (nk) {
        int x = xr - nk * 12 + 3;
        map_text(f, x - 40, y, "KEYS", ink);
        for (int c = 0; c < NKEYS; c++) if (has[c]) { map_key_mark(f, x, y + 1, c, G.has_key[c] > 0); x += 12; }
    }
}
static int map_shown(void) { return G.map_on && G.player && !G.shop && !G.dead && !G.tally && !G.ui && RUN.has_map; }
/* the map over the crypt as it goes on: the scroll across the middle, holes and all */
static void render_map(Frame *f) {
    static const uint8_t KEY_INK[NKEYS] = { R_GOLD + 5, R_IRON + 9, R_BLOOD + 9 };
    const uint8_t ink = R_WOOD + 2;
    int seed = G.level * 131 + PK.depth * 17 + 7;
    if (g_scroll_for != seed) map_scroll_make(seed);
    memset(map_ghost, 0, sizeof map_ghost);
    for (int y = 0; y < H; y++) for (int x = SCROLL_X0; x <= SCROLL_X1; x++) if (g_scroll[y][x]) f->pix[y * W + x] = g_scroll[y][x];
    map_roller(f, 37); map_roller(f, 282);
    map_bx = (int)floorf(MAP_VX0 + MAP_VW * 0.5f - G.map_cx * MAP_PX);
    map_by = (int)floorf(MAP_VY0 + MAP_VH * 0.5f - G.map_cy * MAP_PX);
    for (int y = 0; y < MAPN; y++)                                 /* where you have walked: washed a shade darker, faintly hatched */
        for (int x = 0; x < MAPN; x++) {
            if (!(G.mapped[y][x] & 8) || !map_open(x, y)) continue;
            int px = map_bx + x * MAP_PX, py = map_by + y * MAP_PX;
            for (int j = 1; j < MAP_PX; j++)
                for (int i = 1; i < MAP_PX; i++) {
                    int sx = px + i, sy = py + j;
                    if (sx < MAP_VX0 || sx >= MAP_VX0 + MAP_VW || sy < MAP_VY0 || sy >= MAP_VY0 + MAP_VH) continue;
                    uint8_t c = g_scroll[sy][sx];
                    if (c < R_BONE || c >= R_BONE + 16) continue;  /* paper only: a scorch or a hole stays as it is */
                    f->pix[sy * W + sx] = (uint8_t)imax(R_BONE, c - (((sx + sy) % 3) == 0 ? 4 : 2));
                }
        }
    for (int y = 0; y < MAPN && WD.mask && !PK.depth; y++)        /* a lit soul-lamp's ward: washed violet (1.0.0b18.1) */
        for (int x = 0; x < MAPN; x++) {
            if (!(G.mapped[y][x] & 1) || !wraith_barred(x, y) || !map_open(x, y)) continue;
            int px = map_bx + x * MAP_PX, py = map_by + y * MAP_PX;
            for (int j = 1; j < MAP_PX; j++)
                for (int i = 1; i < MAP_PX; i++) {
                    int sx = px + i, sy = py + j;
                    if (sx < MAP_VX0 || sx >= MAP_VX0 + MAP_VW || sy < MAP_VY0 || sy >= MAP_VY0 + MAP_VH || ((sx + 2 * sy) & 3)) continue;
                    uint8_t c = g_scroll[sy][sx];
                    if (c >= R_BONE && c < R_BONE + 16) f->pix[sy * W + sx] = (uint8_t)(R_PURPLE + 6);
                }
        }
    for (int y = 0; y < MAPN; y++)                                 /* the walls: a line wherever floor meets stone */
        for (int x = 0; x < MAPN; x++) {
            if (!(G.mapped[y][x] & 1) || !map_open(x, y)) continue;
            int px = map_bx + x * MAP_PX, py = map_by + y * MAP_PX;
            if (px > MAP_VX0 + MAP_VW || py > MAP_VY0 + MAP_VH || px + MAP_PX < MAP_VX0 || py + MAP_PX < MAP_VY0) continue;
            for (int i = 0; i <= MAP_PX; i++) {
                if (!map_open(x + 1, y)) map_px(f, px + MAP_PX, py + i, ink);
                if (!map_open(x - 1, y)) map_px(f, px, py + i, ink);
                if (!map_open(x, y + 1)) map_px(f, px + i, py + MAP_PX, ink);
                if (!map_open(x, y - 1)) map_px(f, px + i, py, ink);
            }
            if (G.map[y][x].kind == K_DOOR && G.map[y][x].door >= 0) {   /* a door: a bar across the way */
                const Door *dr = &G.doors[G.map[y][x].door];
                uint8_t c = dr->locked == SEALED ? (uint8_t)(R_BLOOD + 8)
                          : dr->locked == WINGED && (G.mapped[y][x] & 4) ? (uint8_t)(R_PURPLE + 9)   /* the sealed wing's, once seen */
                          : G.tri >= 0 && G.map[y][x].door == G.tri_door && (G.mapped[y][x] & 4) ? (uint8_t)(R_BLOOD + 11)   /* a trial's */
                          : dr->locked == 1 && (G.mapped[y][x] & 2) && dr->color >= 0 && dr->color < NKEYS ? KEY_INK[dr->color] : ink;
                int across = !map_open(x - 1, y) && !map_open(x + 1, y);   /* stone east and west: the way runs north and south */
                for (int i = 1; i < MAP_PX; i++)
                    for (int t = 2; t <= 3; t++) { if (across) map_px(f, px + i, py + t, c); else map_px(f, px + t, py + i, c); }
            }
            if (G.map[y][x].kind == K_PROP) {                       /* something low: a grave, a fire pit; in its part of the cell */
                int fm = foot_of(x, y), ox = 2, oy = 2;
                if (fm) { ox = (fm & 10) && !(fm & 5) ? 3 : (fm & 5) && !(fm & 10) ? 1 : 2; oy = (fm & 12) && !(fm & 3) ? 3 : (fm & 3) && !(fm & 12) ? 1 : 2; }
                map_px(f, px + ox, py + oy, ramp_at(R_WOOD, 6));
            }
        }
    if (G.exit_wx >= 0) map_face(f, G.exit_wx, G.exit_wy, R_MOSS + 5);   /* the way out, in green */
    for (int i = 0; i < G.nsecrets; i++)                           /* a chained room's doorway, opened */
        if (G.secrets[i].state && G.secrets[i].door_x >= 0) map_face(f, G.secrets[i].door_x, G.secrets[i].door_y, G.secrets[i].sealed ? (uint8_t)(R_BLOOD + 8) : (uint8_t)(R_PURPLE + 7));
    if (PK.depth) {                                                /* a pocket's doorways, back and on */
        if (PK.back_x >= 0) map_face(f, PK.back_x, PK.back_y, R_PURPLE + 7);
        if (PK.on_x >= 0) map_face(f, PK.on_x, PK.on_y, R_PURPLE + 7);
    }
    if (G.vis_x >= 0 && G.vis_y >= 0 && G.vis_x < MAPN && G.vis_y < MAPN) {   /* the visportal, or the cooler */
        if (G.map[G.vis_y][G.vis_x].kind == K_WALL) map_face(f, G.vis_x, G.vis_y, R_GOLD + 6);
        else if (G.mapped[G.vis_y][G.vis_x] & 1)
            for (int k = 1; k < 4; k++) for (int j = 1; j < 4; j++) map_px(f, map_bx + G.vis_x * MAP_PX + k, map_by + G.vis_y * MAP_PX + j, R_GOLD + 6);
    }
    for (int i = 0; i < WD.n && !PK.depth && !G.shop; i++)        /* the soul-lamps: a mark on their wall, blue while lit */
        if (G.mapped[WD.l[i].cy][WD.l[i].cx] & 1) {
            int px = map_bx + WD.l[i].cx * MAP_PX + 2 + DX[WD.l[i].d] * 2, py = map_by + WD.l[i].cy * MAP_PX + 2 + DY[WD.l[i].d] * 2;
            uint8_t c = WD.l[i].banked > 0.0f ? (uint8_t)(R_GLOW + 12) : (uint8_t)(R_IRON + 3);
            map_px(f, px, py, c); map_px(f, px + 1, py, c); map_px(f, px, py + 1, c); map_px(f, px + 1, py + 1, c);
        }
    if (G.altar_x >= 0 && (G.mapped[G.altar_y][G.altar_x] & 1)) {   /* the Black Altar */
        int px = map_bx + G.altar_x * MAP_PX, py = map_by + G.altar_y * MAP_PX;
        for (int k = 1; k < 4; k++) for (int j = 1; j < 4; j++) map_px(f, px + k, py + j, k == 2 && j == 2 ? (uint8_t)(R_PURPLE + 10) : (uint8_t)(R_IRON + 1));
    }
    if (!PK.depth && !G.shop && G.bsh_x >= 0 && G.bsh_found) {    /* the Black Shrine, once found: black, two violet eyes */
        int px = map_bx + G.bsh_x * MAP_PX, py = map_by + G.bsh_y * MAP_PX;
        for (int k = 1; k < 4; k++) for (int j = 1; j < 5; j++) map_px(f, px + k, py + j, R_IRON + 1);
        map_px(f, px + 1, py + 2, R_PURPLE + 13); map_px(f, px + 3, py + 2, R_PURPLE + 13);
    }
    if (!PK.depth && msg_shown() > 0.0f && (msg_shown() >= 1.0f || fmodf(G.ui_t * 3.0f, 1.0f) < msg_shown())) {   /* where the message lies */
        int px = map_bx + (int)(G.decos[G.msg_deco].x * MAP_PX), py = map_by + (int)(G.decos[G.msg_deco].y * MAP_PX);
        for (int k = -2; k <= 2; k++) { map_px(f, px + k, py + k, R_PURPLE + 12); map_px(f, px + k, py - k, R_PURPLE + 12); }
    }
    for (int i = 0; i < G.nitems; i++) {                           /* the keys you have seen, until taken */
        const Item *it = &G.items[i];
        if (!it->alive || it->type != IT_KEY || !it->seen) continue;
        int px = map_bx + (int)(it->x * MAP_PX), py = map_by + (int)(it->y * MAP_PX);
        static const char *const KEYG[3] = { "##...", "#####", "##.#." };
        for (int r = 0; r < 3; r++) for (int k = 0; k < 5; k++) if (KEYG[r][k] == '#') map_px(f, px - 2 + k, py - 1 + r, KEY_INK[clampi(it->color, 0, NKEYS - 1)]);
    }
    if (RUN.corner >> CS_CARTO & 1)                                /* the Cartographer: a secret's wall, once passed */
        for (int i = 0; i < G.nsecrets; i++) {
            const Secret *sc = &G.secrets[i];
            if (sc->state || !(G.mapped[sc->sy][sc->sx] & 2)) continue;
            for (int d = 0; d < 4; d++) {
                int nx = sc->sx + DX[d], ny = sc->sy + DY[d];
                if (!map_open(nx, ny)) continue;
                int px = map_bx + nx * MAP_PX, py = map_by + ny * MAP_PX;
                for (int k = 0; k <= MAP_PX; k += 2) {             /* a dotted crack along its face */
                    if (d == 0) map_px(f, px + 1, py + k, R_PURPLE + 9); else if (d == 2) map_px(f, px + MAP_PX - 1, py + k, R_PURPLE + 9);
                    else if (d == 1) map_px(f, px + k, py + 1, R_PURPLE + 9); else map_px(f, px + k, py + MAP_PX - 1, R_PURPLE + 9);
                }
            }
        }
    if (G.cshown && !PK.depth)                                     /* a corrupted level's secrets, shown: a violet mark at each shut */
        for (int i = 0; i < G.nsecrets; i++) {
            const Secret *sc = &G.secrets[i];
            if (sc->state) continue;
            int px = map_bx + sc->fx * MAP_PX + MAP_PX / 2, py = map_by + sc->fy * MAP_PX + MAP_PX / 2;
            for (int k = -2; k <= 2; k++) { map_px(f, px + k, py, R_PURPLE + 13); map_px(f, px, py + k, R_PURPLE + 13); }
            map_px(f, px, py, R_PURPLE + 15);
        }
    if (RUN.corner >> CS_SENSE & 1)                                /* Grave Sense: the dead within ten cells */
        for (int i = 0; i < MAXMOBS; i++) {
            const Mob *m = &G.mobs[i];
            if (!m->alive || m->state != MS_WALK || m->ally || hypotf(m->x - G.px, m->y - G.py) > 10.0f) continue;
            int px = map_bx + (int)(m->x * MAP_PX), py = map_by + (int)(m->y * MAP_PX), on = ((int)(G.ui_t * 3.0f + i) & 3) != 0;
            for (int k = 0; k < 4; k++) map_px(f, px - 1 + (k & 1), py - 1 + (k >> 1), on ? (uint8_t)(R_BLOOD + 12) : (uint8_t)(R_BLOOD + 8));
        }
    {                                                              /* and you, an arrow the way you face */
        float cx = map_bx + G.px * MAP_PX, cy = map_by + G.py * MAP_PX, dx = cosf(G.ang), dy = sinf(G.ang);
        float ax = cx + 4.5f * dx, ay = cy + 4.5f * dy, bx = cx - 2.5f * dx - 2.6f * dy, by = cy - 2.5f * dy + 2.6f * dx, qx = cx - 2.5f * dx + 2.6f * dy, qy = cy - 2.5f * dy - 2.6f * dx;
        for (int y = (int)cy - 6; y <= (int)cy + 6; y++)
            for (int x = (int)cx - 6; x <= (int)cx + 6; x++) {
                float px = x + 0.5f, py = y + 0.5f;
                float e0 = (bx - ax) * (py - ay) - (by - ay) * (px - ax), e1 = (qx - bx) * (py - by) - (qy - by) * (px - bx), e2 = (ax - qx) * (py - qy) - (ay - qy) * (px - qx);
                if ((e0 >= 0 && e1 >= 0 && e2 >= 0) || (e0 <= 0 && e1 <= 0 && e2 <= 0)) map_px(f, x, y, R_BLOOD + 11);
            }
    }
    char name[48];                                                 /* where it is, across the top */
    if (PK.depth) snprintf(name, sizeof name, "%s", G.ptheme == PT_SEVERANCE ? "THE SEVERED FLOOR" : "THE BACKROOMS");
    else level_name(G.level, name, sizeof name);
    map_text(f, (W - gfx_text_width(name, 1)) / 2, 13, name, R_WOOD + 3);
    for (int y = 12; y <= 20; y++) ink_px(f, 266, y, R_WOOD + 3, 0, 0, W, H);   /* north */
    ink_px(f, 265, 13, R_WOOD + 3, 0, 0, W, H); ink_px(f, 267, 13, R_WOOD + 3, 0, 0, W, H); ink_px(f, 264, 14, R_WOOD + 3, 0, 0, W, H); ink_px(f, 268, 14, R_WOOD + 3, 0, 0, W, H);
    map_text(f, 255, 13, "N", R_WOOD + 3);
    map_numbers(f);
    map_ghostly(f);
    const char *hint = PADX.used ? "D-PAD MOVES IT  \1B CLOSES" : "ARROWS MOVE IT  SPACE RETURNS  M CLOSES";
    hint8(f, (W - hint_width(hint)) / 2, 190, hint, R_STONE + 11);
}

static void render_ui_in(Frame *f);
static void render_portraitcard(Frame *f);
