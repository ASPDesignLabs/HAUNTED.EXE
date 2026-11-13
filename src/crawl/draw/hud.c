/* A part of scene_crawl.c (docs/PLAN-tidy.md): it is #included there in its place and never compiled on its own.
 * Her wares' tags, the portrait's face, glyphs, the status bar, text wrapped, her prompts and cards, the wards' rings. */

/* ---- the merchant's wares: prices over the pedestals, a sparkle when one is taken ---- */
static void render_shop_tags(Frame *f, float dx, float dy, float horizon) {
    if (WH.k > 0.0f) return;                                       /* a wheel up in her rooms: the tags wait (1.0.0b18.1) */
    int order[NPED], n = 0, box[NPED][4], nb = 0;                  /* nearest first; a tag that would cover a nearer one waits */
    for (int i = 0; i < G.ped_n; i++) order[n++] = i;
    for (int a = 1; a < n; a++) for (int b = a; b > 0; b--) {
        float da = hypotf(G.ped_x[order[b]] - G.px, G.ped_y[order[b]] - G.py), db = hypotf(G.ped_x[order[b - 1]] - G.px, G.ped_y[order[b - 1]] - G.py);
        int look = order[b] == G.shop_look;                        /* the one looked at comes first of all */
        if (look || (da < db && order[b - 1] != G.shop_look)) { int t = order[b]; order[b] = order[b - 1]; order[b - 1] = t; } else break;
    }
    for (int oi = 0; oi < n; oi++) {
        int i = order[oi];
        float sx, sy, dep;
        if (!project(G.ped_x[i], G.ped_y[i], i == G.shop_pick && G.sparkle > 0.0f ? 0.78f : 1.04f, dx, dy, horizon, &sx, &sy, &dep)) continue;
        int x = (int)sx;
        if (x < 0 || x >= W || dep >= zbuf[x] + 0.3f) continue;
        if (G.offer_gone[i]) {
            if (i != G.shop_pick || G.sparkle <= 0.0f) continue;
            float k = 0.8f - G.sparkle;
            for (int p = 0; p < 18; p++) {                         /* a burst of light where it was */
                float a = p * TAU_F / 18 + k * 2.0f, r = k * (40.0f + 12.0f * (p & 1)) / fmaxf(dep, 0.8f);
                int px = (int)(sx + cosf(a) * r), py = (int)(sy + sinf(a) * r * 0.8f);
                if (px >= 0 && px < W && py >= 0 && py < H) f->pix[py * W + px] = p % 3 == 0 ? (uint8_t)C_WHITE : (uint8_t)(R_GLOW + 12);
            }
            continue;
        }
        const Offer *o = &G.offers[i];
        char tag[16];
        if (o->price == 0) snprintf(tag, sizeof tag, "FREE");
        else snprintf(tag, sizeof tag, "$%d", o->price);
        int w = gfx_text_width(tag, 1), tx = x - w / 2, ty = (int)sy - 10, hit = 0;
        if (ty < 0 || ty > H - 60) continue;
        for (int k = 0; k < nb; k++)
            hit |= tx < box[k][2] + 3 && tx + w + 3 > box[k][0] && ty - 10 < box[k][3] && ty + 9 > box[k][1];
        if (hit) continue;
        box[nb][0] = tx; box[nb][1] = ty - 10; box[nb][2] = tx + w; box[nb][3] = ty + 9; nb++;
        uint8_t col = o->price <= RUN.treasure ? (uint8_t)(R_GOLD + 12) : (uint8_t)(R_BLOOD + 11);
        gfx_text8(f, tx + 1, ty + 1, tag, 0, 1);
        gfx_text8(f, tx, ty, tag, col, 1);
        const char *over = G.shop_held == i || G.offer_back[i] ? "HELD" : (o->rare && ((int)(G.t * 3.0) & 1) ? "RARE" : NULL);
        if (over) { int rw = gfx_text_width(over, 1); gfx_text8(f, x - rw / 2 + 1, ty - 9, over, 0, 1); gfx_text8(f, x - rw / 2, ty - 10, over, over[0] == 'H' ? (uint8_t)(R_GLOW + 12) : (uint8_t)C_WHITE, 1); }
    }
    float sx, sy, dep;                                             /* the map's price, over it */
    if (!RUN.has_map && !g_map_kept && project(MAP_X, MAP_Y, 0.95f, dx, dy, horizon, &sx, &sy, &dep) && sx >= 0 && sx < W && dep < zbuf[(int)sx] + 0.3f && dep < 6.0f) {
        char tag[16];
        snprintf(tag, sizeof tag, "$%d", MAP_PRICE);
        int w = gfx_text_width(tag, 1), tx = (int)sx - w / 2, ty = (int)sy - 10, hit = 0;
        for (int k = 0; k < nb; k++) hit |= tx < box[k][2] + 3 && tx + w + 3 > box[k][0] && ty - 10 < box[k][3] && ty + 9 > box[k][1];
        if (!hit && ty >= 0 && ty <= H - 60) {
            gfx_text8(f, tx + 1, ty + 1, tag, 0, 1);
            gfx_text8(f, tx, ty, tag, MAP_PRICE <= RUN.treasure ? (uint8_t)(R_GOLD + 12) : (uint8_t)(R_BLOOD + 11), 1);
        }
    }
}

/* The caster's face, Wolfenstein style: bloodier as health runs out, glancing about, flinching
 * towards a blow, grinning at loot. The hood is the one it wears. */
static void draw_face(Frame *f, int ox, int oy, int sub) {
    const Gear *g = &RUN.gear;
    float hpf = G.max_hp > 0 ? (float)G.hp / (float)G.max_hp : 0.0f;
    int stage = clampi((int)((1.0f - hpf) * 7.0f), 0, 6);
    int fr = G.dead || G.hp <= 0 ? 29 : (G.grin_t > 0.0f ? 28 : stage * 4 + sub);
    const Piece *hp = &g->worn[SL_HOOD];                           /* the caster's own purple, or the hood's quality */
    int hood = hp->kind ? QUAL_RAMP[hp->quality] : R_PURPLE, shade = hp->kind && hp->quality == Q_COMMON ? -3 : 0;
    int trim = !hp->kind || hp->anointed ? (hood == R_GOLD ? R_BONE : R_GOLD) : R_WOOD;
    const uint8_t *img = crawl_face_img[fr];
    for (int y = 0; y < FACE_H; y++)
        for (int x = 0; x < FACE_W; x++) {
            uint8_t c = img[y * FACE_W + x];
            if (c == C_CLEAR) continue;
            int sx = ox + x, sy = oy + y;
            if (sx < 0 || sx >= W || sy < 0 || sy >= H) continue;
            if (c >= R_PURPLE && c < R_PURPLE + 16) c = ramp_at(hood, c - R_PURPLE + shade);
            else if (c >= R_GOLD && c < R_GOLD + 13 && trim != R_GOLD) c = (uint8_t)(trim + clampi(c - R_GOLD + 3, 0, 15));
            else if (c >= R_SKIN && c < R_SKIN + 16 && ((x + y) & 1)) {
                if (G.poison_t > 0.0f) c = (uint8_t)(R_MOSS + clampi(c - R_SKIN - 1, 0, 15));          /* green about the gills */
                else if (G.slow_t > 0.0f) c = (uint8_t)(R_GHOST + clampi(c - R_SKIN - 2, 0, 15));
            }
            f->pix[sy * W + sx] = c;
        }
}

static int face_sub(void) {
    if (G.face_t > FACE_T - 0.7f) return G.face_side == 0 ? 3 : (G.face_side < 0 ? 1 : 2);   /* the flinch */
    uint32_t h = hash2((int)(G.t / 1.3), 977);                     /* looking about, now and then */
    return h % 10 < 6 ? 0 : (h % 10 < 8 ? 1 : 2);
}

static void bevel(Frame *f, int x, int y, int w, int h, int fill) {   /* a plate sunk in the stone (docs/PLAN-ui.md section 7) */
    (void)fill;
    ui_well(f, x, y, w, h);
}

/* an element's glyph, 7 by 7, in its colour or dimmed; the + and - marks beside it, 3 by 3; struck through */
static void elem_glyph(Frame *f, int x, int y, int e, int dim) {
    for (int r = 0; r < 7; r++) for (int c = 0; c < 7; c++) {
        char ch = ELEM_GLYPH[e][r][c];
        if (ch == '#' || ch == 'o') gfx_px(f, x + c, y + r, dim ? (uint8_t)(R_STONE + (ch == 'o' ? 9 : 7)) : ramp_at(ELEM_RAMP[e], ch == 'o' ? 15 : 11));
    }
}
static void class_mark(Frame *f, int x, int y, int e, int dim) {   /* a class's mark, 11 by 11, in its element's colour */
    for (int r = 0; r < 11; r++) for (int c = 0; c < 11; c++) {
        char ch = CLASS_MARK[e][r][c];
        if (ch == '#' || ch == 'o') gfx_px(f, x + c, y + r, dim ? (uint8_t)(R_STONE + (ch == 'o' ? 9 : 7)) : ramp_at(ELEM_RAMP[e], ch == 'o' ? 15 : 11));
    }
}
static void plus_mark(Frame *f, int x, int y, int minus, uint8_t c) {
    for (int i = 0; i < 3; i++) gfx_px(f, x + i, y + 1, c);
    if (!minus) { gfx_px(f, x + 1, y, c); gfx_px(f, x + 1, y + 2, c); }
}
static void strike_mark(Frame *f, int x, int y) { for (int i = 0; i < 9; i++) gfx_px(f, x - 1 + i, y + 7 - i, R_BLOOD + 11); }
/* What an element does to a kind, as the glyph's marks say it: 2 at 1.5 or more, 1 above plain, -1 below,
 * -2 at half or less, -3 nothing at all (struck through) */
static int weak_grade(float w) { return w >= 1.49f ? 2 : w > 1.01f ? 1 : w <= 0.001f ? -3 : w <= 0.51f ? -2 : w < 0.99f ? -1 : 0; }
static int glyph_width(int g) { return 7 + (g == 0 ? 0 : (g == 2 || g == -2 ? 8 : 4)); }   /* a glyph and its marks */
static void glyph_marked(Frame *f, int x, int y, int e, int g) {
    elem_glyph(f, x, y, e, 0);
    if (g == -3) strike_mark(f, x, y);
    else if (g) { plus_mark(f, x + 8, y + 2, g < 0, g > 0 ? (uint8_t)C_WHITE : (uint8_t)(R_STONE + 11)); if (g == 2 || g == -2) plus_mark(f, x + 12, y + 2, g < 0, g > 0 ? (uint8_t)C_WHITE : (uint8_t)(R_STONE + 11)); }
}
/* the status bar, in the antechamber only (nothing stays put on an OLED for long) */
static void key_icon(Frame *f, int kx, int ky, int color);
static void dead_eye(Frame *f, int x, int y);
static void draw_coin(Frame *f, int x, int y);
static void lantern_mark(Frame *f, int x, int y, int lit) {        /* a little lantern, 5 by 8: a flame in it when lit */
    static const char *const L[8] = { ".###.", "..#..", "#####", "#...#", "#.o.#", "#.o.#", "#...#", "#####" };
    for (int r = 0; r < 8; r++)
        for (int c = 0; c < 5; c++) {
            char ch = L[r][c];
            if (ch == '#') gfx_px(f, x + c, y + r, lit ? (uint8_t)(R_GOLD + 9) : (uint8_t)(R_STONE + 7));
            else if (ch == 'o') gfx_px(f, x + c, y + r, lit ? (uint8_t)(R_FLAME + 13 + (r == 4)) : (uint8_t)(R_STONE + 3));
            else if (r >= 3 && r <= 6 && lit) gfx_px(f, x + c, y + r, R_FLAME + 8);
        }
}
static int small_digits(Frame *f, int x, int y, int n, uint8_t c) {   /* 3 by 5 figures, for the keys' box; returns the width */
    static const char *const D[10] = { "####.##.##.####", ".#.##..#..#.###", "###..#####..###", "###..#.##..####", "#.##.####..#..#",
                                       "####..###..####", "####..####.####", "###..#..#.#..#.", "####.#####.####", "####.####..####" };
    char b[12];
    snprintf(b, sizeof b, "%d", n);
    int w = (int)strlen(b) * 4 - 1;
    for (int i = 0; b[i] && f; i++)
        for (int k = 0; k < 15; k++) if (D[b[i] - '0'][k] == '#') gfx_px(f, x + i * 4 + k % 3, y + k / 3, c);
    return w;
}
static void render_statusbar(Frame *f) {
    int y0 = H - 44 + (int)((1.0f - smoothstepf(0.0f, 1.0f, G.bar)) * 46.0f + 0.5f);   /* it slides down out of sight */
    if (y0 >= H) return;
    ui_slab(f, 0, y0, W, 44 + 4, SLAB_TABLET | SLAB_CALM, ORN_NONE, G.ui_t);   /* a tablet of the run's stone, its foot below the screen */
    ui_box_none();
    {                                                              /* its top edge: the experience towards the next level, gold when one waits */
        int reach = atone_reach(), lo = atone_need(reach), hi = atone_need(reach + 1), waits = reach > RUN.atone;
        int fill = hi > lo ? (int)((float)(RUN.xp - lo) / (float)(hi - lo) * W) : 0;
        for (int x = 0; x < W; x++) {
            int in = x < fill;
            gfx_px(f, x, y0, in ? (uint8_t)(waits ? R_GOLD + 11 : R_PURPLE + 12) : (uint8_t)(R_IRON + 9));
            gfx_px(f, x, y0 + 1, in ? (uint8_t)(waits ? R_GOLD + 6 : R_PURPLE + 7) : (uint8_t)(R_IRON + 6));
        }
    }
    char num[24];
    struct { int x, w; const char *label; } P[5] = { { 4, 40, "LEVEL" }, { 48, 56, "GOLD" }, { 108, 50, "HEALTH" }, { 206, 42, "SOULS" }, { 252, 48, "SPELL" } };
    for (int i = 0; i < 5; i++) {
        bevel(f, P[i].x, y0 + 4, P[i].w, 37, R_IRON + 2);
        gfx_text8(f, P[i].x + (P[i].w - gfx_text_width(P[i].label, 1)) / 2, y0 + 7, P[i].label, R_IRON + 12, 1);
        uint8_t col = C_WHITE;
        switch (i) {
        case 0: snprintf(num, sizeof num, "%d", G.level); break;
        case 1: snprintf(num, sizeof num, "$%d", G.shop ? G.shop_gold : RUN.treasure); col = R_GOLD + 12; break;
        case 2: snprintf(num, sizeof num, "%d%%", G.max_hp ? G.hp * 100 / G.max_hp : 0); col = G.hp * 4 <= G.max_hp ? R_BLOOD + 13 : (G.poison_t > 0.0f ? R_MOSS + 13 : C_WHITE); break;
        case 3: {                                                  /* and how full the vessel is, a line under them, clear of the figures */
            spirit_text(num, sizeof num);
            int cap = vessel(), full = RUN.souls >= cap;
            col = full ? R_GOLD + 12 : R_GLOW + 12;
            int gw = P[i].w - 8, fill = cap > 0 ? clampi((int)((float)gw * spirit() / (float)cap + 0.5f), 0, gw) : 0;
            for (int k = 0; k < gw; k++) {                         /* in a groove cut along the plate's foot */
                gfx_px(f, P[i].x + 4 + k, y0 + 35, UI_M(0));
                gfx_px(f, P[i].x + 4 + k, y0 + 36, k < fill ? (full ? (uint8_t)(R_GOLD + 11) : (uint8_t)(R_GLOW + 10)) : UI_M(1));
                gfx_px(f, P[i].x + 4 + k, y0 + 37, UI_M(9));
            }
            break;
        }
        default: {                                                 /* the spell in hand: its element's glyph and its rank */
            if (G.rst_sel) { restore_hud(f, P[i].x, P[i].w, y0); continue; }   /* or Restoration (D5) */
            int sp = clampi(G.spell, 0, NATK - 1), t = RUN.gear.atk[sp], el = ATK_ELEM[sp];
            uint8_t sc = ramp_at(gear_color(GK_ATK, sp), 13);
            const char *nm = el >= 0 ? (el == EL_LIGHT ? "LIGHTNG" : el == EL_PHYS ? "SPEAR" : ELEM_NAME[el]) : "BFS";
            int nw = gfx_text_width(nm, 1) + (el >= 0 ? 10 : 0), nx = P[i].x + (P[i].w - nw) / 2;
            if (el >= 0) { elem_glyph(f, nx, y0 + 19, el, 0); nx += 10; }
            gfx_text8(f, nx, y0 + 19, nm, sc, 1);
            if (sp == AT_BFS) snprintf(num, sizeof num, "-%d", bfs_cost(t));
            else snprintf(num, sizeof num, "RANK %d", imax(t, 1));
            if (sp == AT_SPEAR) {                                  /* the bones to throw, over the label */
                char bn[8];
                int bare = spear_bare();                           /* no bones: the splinters left, a sliver for its mark */
                snprintf(bn, sizeof bn, "%d", bare ? splint_left() : RUN.gear.bones);
                int bw = gfx_text_width(bn, 1), bx = P[i].x + (P[i].w - bw - 12) / 2, by = y0 + 7;
                gfx_rect(f, P[i].x + 2, by - 1, P[i].w - 4, 9, UI_M(3));
                uint8_t bc = !bare ? (uint8_t)(R_BONE + 14) : splint_left() > 0 ? (uint8_t)(R_BONE + 9) : (uint8_t)(R_BLOOD + 13);
                static const char *BONE[3] = { "##.....##", ".#######.", "##.....##" };   /* a bone */
                static const char *SLIV[3] = { ".......##", "...####..", "##......." };   /* a splinter */
                for (int r = 0; r < 3; r++) for (int k = 0; k < 9; k++) if ((bare ? SLIV : BONE)[r][k] == '#') gfx_px(f, bx + k, by + 2 + r, bc);
                gfx_text8(f, bx + 12, by, bn, bc, 1);
            }
            gfx_text8(f, P[i].x + (P[i].w - gfx_text_width(num, 1)) / 2, y0 + 30, num, R_IRON + 13, 1);
            continue;
        }
        }
        gfx_text16(f, P[i].x + (P[i].w - gfx_text_width(num, 1)) / 2, y0 + 19, num, col, 1);
    }
    bevel(f, 304, y0 + 4, 12, 37, R_IRON + 2);                     /* the keys in hand, the dead's eye, the Cursed Coins */
    for (int c = 0; c < NKEYS; c++) if (G.has_key[c]) key_icon(f, 306, y0 + 6 + c * 6, c);
    if (LEDGER.marked) dead_eye(f, 306, y0 + 24);
    if (RUN.coins > 0) {
        draw_coin(f, 306, y0 + 29);
        int w = small_digits(NULL, 0, 0, RUN.coins, 0);
        small_digits(f, 304 + (12 - w) / 2, y0 + 36, RUN.coins, R_GOLD + 10);
    }
    bevel(f, 162, y0 + 1, 40, 43, 0);
    draw_face(f, 166, y0 + 3, face_sub());
    if (lantern_held()) lantern_mark(f, 195, y0 + 34, lantern_lit());   /* the Lantern of the Dead, lit or shuttered */
}

/* 8x8 type wrapped at spaces to cols a line, lh px apart; returns the lines it took (no frame: only counts) */
static int glued(const char *s, int i) {                          /* a number keeps its unit: "3.0 S" */
    if (i <= 0 || s[i - 1] < '0' || s[i - 1] > '9') return 0;
    int k = i + 1;
    while (s[k] && s[k] != ' ' && s[k] != ',' && s[k] != '.') k++;
    return k - (i + 1) <= 2;
}
static int text_wrap_ex(Frame *f, int x, int y, const char *s, int cols, int col, int maxlines, int lh) {
    int lines = 0;
    while (*s && lines < maxlines) {
        int n = (int)strlen(s), cut = n;
        if (n > cols) { cut = cols; while (cut > 0 && (s[cut] != ' ' || glued(s, cut))) cut--; if (!cut) cut = cols; }
        char line[64];
        snprintf(line, sizeof line, "%.*s", cut, s);
        if (f) gfx_text8(f, x, y + lines * lh, line, (uint8_t)col, 1);
        lines++;
        s += cut;
        while (*s == ' ') s++;
    }
    return lines;
}
static int text_wrap(Frame *f, int x, int y, const char *s, int cols, int col, int maxlines) { return text_wrap_ex(f, x, y, s, cols, col, maxlines, 9); }

/* what the caster is looking at, and what it costs */
static void draw_glyph(Frame *f, int x, int y, int k, int sc, uint8_t c) {
    for (int gy = 0; gy < 7; gy++)
        for (int gx = 0; gx < 5; gx++) if (RUNE_GLYPH[k][gy][gx] == '#') gfx_rect(f, x + gx * sc, y + gy * sc, sc, sc, c);
}
static void draw_coin(Frame *f, int x, int y) {                    /* a Cursed Coin, 7 across */
    static const char *const C[7] = { "..###..", ".#####.", "##.#.##", "#######", "##...##", ".#####.", "..###.." };
    for (int r = 0; r < 7; r++)
        for (int c = 0; c < 7; c++)
            if (C[r][c] == '#') gfx_px(f, x + c, y + r, (uint8_t)(r < 3 ? R_GOLD + 9 : R_GOLD + 5));
            else if (r == 2 && c == 4) gfx_px(f, x + c, y + r, R_BLOOD + 14);   /* its red eye */
            else if (r == 2 || r == 4) gfx_px(f, x + c, y + r, 0);
}
static int shop_carded(void) {                                     /* a person looking at something in her room: its card is up */
    return G.shop && G.player && (G.shop_look >= 0 || G.rep_look || G.comp_look || G.stone_look >= 0 || G.map_look || G.mer_look || G.por_look
                                  || G.tot_look || G.bell_look || G.font_look || G.lamp_look || G.sh_look || G.pt_look);
}
/* In her rooms, for a person (after 1.0.0b21.2, at the owner's word): what is faced on a slim band at the foot of the view, the key
 * that does something with it on a cap before it and a note after it where one matters (a ware's price, gold waiting); the
 * card, the menu or a ware's buying comes up on that key, so that the thing itself stays in sight. */
static void shop_prompt(Frame *f, const char *key, const char *what, int col, const char *note, int ncol) {
    const char *k = key ? pk(key) : NULL;
    int kw = k ? ui_keys_width(k) + 10 : 0, ww = gfx_text_width(what, 1), nw = note && *note ? gfx_text_width(note, 1) + 14 : 0;
    if (kw + ww + nw + 16 > W - 8) nw = 0;                         /* too long: the note gives way */
    int bw = imin(W - 8, kw + ww + nw + 16), bx = (W - bw) / 2, by = H - 44 - 4 - 15;
    ui_band(f, bx, by, bw, 15);
    if (k) ui_keys(f, bx + 10, by + 4, k, UI_M(13));
    gfx_text8(f, bx + 8 + kw, by + 4, what, col, 1);
    if (nw) gfx_text8(f, bx + bw - 8 - gfx_text_width(note, 1), by + 4, note, ncol, 1);
}
static void render_shop_prompt(Frame *f) {
    char what[100] = "", note[48] = "";
    int col = R_STONE + 13, ncol = R_GOLD + 11, i = G.shop_look;
    const char *key = "E";
    if (i >= 0 && !G.offer_gone[i]) {                              /* a ware: its name in its quality's colour, its price */
        const Offer *o = &G.offers[i];
        if (o->kind == GK_PIECE) { char a[48]; piece_name(&o->piece, a, what, sizeof what); col = qual_text(o->piece.quality); }
        else { gear_name(o, what, sizeof what); col = ramp_at(offer_color(o), 13); }
        snprintf(note, sizeof note, o->price ? "$%d" : "FREE", o->price);
        ncol = o->price <= RUN.treasure ? R_GOLD + 12 : R_BLOOD + 12;
        if (G.shop_held == i) snprintf(note, sizeof note, "HELD");
    } else if (G.pt_look) {                                        /* a portrait of the slain */
        int p = por_head(G.pt_look - 1), b = por_bossof(p);
        if (!por_is_drawn(p)) {
            if (por_can_draw(p)) { snprintf(what, sizeof what, b >= 0 ? "DRAW BACK THE TAPESTRIES" : "DRAW BACK THE TAPESTRY"); col = R_BLOOD + 13; }
            else { key = NULL; snprintf(what, sizeof what, b >= 0 ? "A TAPESTRY: WHAT HANGS HERE STILL STANDS" : "A TAPESTRY: NONE OF ITS KIND SLAIN"); col = R_STONE + 11; }
        } else if (b >= 0) {
            int unl = boss_unlocked(b), hi = -1, g = boss_wait_gold(b);
            for (int st = 0; st < 6; st++) if (unl >> st & 1) hi = st;
            snprintf(what, sizeof what, "%s", POR_BOSS_NAME[b]); col = hi >= 0 ? RANK_COL[BOSS_GEM[hi]] : R_STONE + 13;
            if (g) snprintf(note, sizeof note, "$%d WAITS", g);
        } else {
            int k = POR.at[p].kind, hi = por_high(por_unlocked(k)), g = por_wait_gold(k);
            snprintf(what, sizeof what, "%s", KINDS[k].plural); col = hi >= 0 ? RANK_COL[hi] : R_STONE + 13;
            if (g) snprintf(note, sizeof note, "$%d WAITS", g);
        }
    } else if (G.map_look) {
        snprintf(what, sizeof what, "A MAP OF THE CRYPT"); col = R_BONE + 13;
        snprintf(note, sizeof note, "$%d", MAP_PRICE); ncol = MAP_PRICE <= RUN.treasure ? R_GOLD + 12 : R_BLOOD + 12;
    } else if (G.sh_look) {
        snprintf(what, sizeof what, "THE RUNE SHRINE"); col = R_GOLD + 12;
        if (!RUN.shrine) snprintf(note, sizeof note, "SANCTIFIED");
        else if (G.sh_takes > 0) snprintf(note, sizeof note, G.sh_takes == 1 ? "ONE TAKE LEFT" : "%d TAKES LEFT", G.sh_takes);
    } else if (G.lamp_look) {
        snprintf(what, sizeof what, "THE LAMPSTAND"); col = R_GOLD + 12;
        if (gift_due()) snprintf(note, sizeof note, "A GIFT WAITS");
        else if (RUN.al_points > 0) snprintf(note, sizeof note, RUN.al_points == 1 ? "A POINT" : "%d POINTS", RUN.al_points);
    } else if (G.tot_look) { snprintf(what, sizeof what, "THE HEXED TOTEM"); col = R_GOLD + 12; }
    else if (G.comp_look) {
        snprintf(what, sizeof what, "THE CORPUS COMPENDIUM"); col = R_BONE + 13;
        snprintf(note, sizeof note, comp_count() == 1 ? "ONE PIECE" : "%d PIECES", comp_count()); ncol = R_STONE + 11;
    }
    else if (G.rep_look) { snprintf(what, sizeof what, "THE WRETCHED REPOSITORY"); col = R_IRON + 13; }
    else if (G.por_look) { snprintf(what, sizeof what, "THE ACCURSED PORTAL"); col = R_PURPLE + 13; }
    else if (G.bell_look) { snprintf(what, sizeof what, "THE DEAD BELL"); col = R_MOSS + 12; }
    else if (G.font_look) { snprintf(what, sizeof what, "THE WEEPING FONT"); col = R_BLOOD + 12; }
    else if (G.stone_look >= 0) {
        snprintf(what, sizeof what, "A DWELLER'S STONE"); col = R_STONE + 13;
        snprintf(note, sizeof note, "LEVEL %d", RUN.stones[clampi(G.stone_look, 0, STONES - 1)].level); ncol = R_STONE + 11;
    } else if (G.mer_look) { snprintf(what, sizeof what, "THE MERCHANT"); col = R_GOLD + 12; }
    if (what[0]) shop_prompt(f, key, what, col, note, ncol);
}
static void render_shop_card(Frame *f) {
    if (G.player && WH.k > 0.0f) return;                           /* and the cards: a wheel covers them (1.0.0b18.1) */
    if (G.player) { render_shop_prompt(f); return; }               /* a person: the slim prompt; the cards below are the dream's */
    int i = G.shop_step == SH_LOOK || G.shop_step == SH_GO || (G.shop_step == SH_BUY && G.shop_pick >= 0 && !G.offer_gone[G.shop_pick]) ? G.shop_look : -1;
    if (G.shop_step == SH_LOOK && G.shop_t < 0.35) i = -1;
    int y0 = H - 44 - 46;
    if (i < 0 || G.offer_gone[i]) {
        if (!G.player && G.shop_step == SH_THANK && !G.shop_bought) {
            const char *msg = G.shop_saving ? "SAVING UP FOR SOMETHING BETTER" : "NOTHING TAKES ITS FANCY";
            int w = gfx_text_width(msg, 1);
            ui_band(f, (W - w) / 2 - 6, y0 + 22, w + 12, 16);
            gfx_text8(f, (W - w) / 2, y0 + 26, msg, R_GOLD + 10, 1);
        }
        return;
    }
    const Offer *o = &G.offers[i];
    char price[16];
    if (o->price == 0) snprintf(price, sizeof price, "FREE");
    else snprintf(price, sizeof price, "$%d", o->price);
    int bw = 264, bx = (W - bw) / 2, ph = 42, pw = gfx_text_width(price, 1);
    int body = 42;
    char name[100], l1[48], l2[48], stats[160] = "";
    const Piece *p = &o->piece;
    if (o->kind == GK_PIECE) {                                     /* a piece: its stats under its name */
        piece_name(p, l1, l2, sizeof l1);
        for (int k = 0; k < p->nstat; k++) {
            char st[40];
            piece_stat_text(p->stat[k], p->val[k], st, sizeof st);
            snprintf(stats + strlen(stats), sizeof stats - strlen(stats), "%s%s", k ? ", " : "", st);
        }
        if (p->kind == IK_BAG) snprintf(stats, sizeof stats, "HOLDS %d. %s", bag_capacity(p->quality), gear_blurb(o));
        if (p->ability) {
            char ab[64];
            piece_ability_text(p, ab, sizeof ab);
            snprintf(stats + strlen(stats), sizeof stats - strlen(stats), ". %s", ab);
        }
        body = 34 + 9 * text_wrap_ex(NULL, 0, 0, stats, (bw - 16) / 8, 0, 4, 9);
    }
    ph = body;
    y0 = H - 44 - 4 - ph;
    ui_band(f, bx, y0, bw, ph);                 /* a band of the run's stone */
    if (o->kind == GK_PIECE) {
        int col = qual_text(p->quality);
        gfx_text8(f, bx + 8, y0 + 5, KIND_NAME[p->kind], R_STONE + 11, 1);
        gfx_text8(f, bx + 8, y0 + 14, l1, col, 1);
        gfx_text8(f, bx + 8, y0 + 23, l2, col, 1);
        text_wrap_ex(f, bx + 8, y0 + 33, stats, (bw - 16) / 8, R_STONE + 13, 4, 9);
        uint8_t pc = o->price <= RUN.treasure ? R_GOLD + 12 : R_BLOOD + 12;
        if (gfx_text_width(l1, 1) + 12 + pw <= bw - 16) gfx_text16(f, bx + bw - 8 - pw, y0 + 4, price, pc, 1);
        else gfx_text8(f, bx + bw - 8 - pw, y0 + 5, price, pc, 1);   /* a long name: the price keeps to the top line */
        return;
    }
    gear_name(o, name, sizeof name);
    gfx_text8(f, bx + 8, y0 + 5, o->kind == GK_ATK ? "ATTACK SPELL" : "DEFENSIVE RUNE", R_STONE + 11, 1);
    if (o->rare) gfx_text8(f, bx + bw - 8 - gfx_text_width("RARE", 1), y0 + 5, "RARE", ((int)(G.t * 3.0) & 1) ? C_WHITE : R_GOLD + 12, 1);
    gfx_text16(f, bx + 8, y0 + 14, name, ramp_at(offer_color(o), 13), 1);
    gfx_text8(f, bx + 8, y0 + 31, gear_blurb(o), R_STONE + 13, 1);
    gfx_text16(f, bx + bw - 8 - pw, y0 + 14, price, o->price <= RUN.treasure ? R_GOLD + 12 : R_BLOOD + 12, 1);
}

/* in the crypt: the face pops up from the bottom-left corner when hurt or pleased, then goes */
static void render_face_popup(Frame *f) {
    float t = fmaxf(G.face_t, G.grin_t > 0.0f ? fminf(G.grin_t, FACE_T) : 0.0f);
    if (G.dead) t = (float)fminf(G.dead_t, 4.0);
    if (t <= 0.0f) return;
    float k = G.dead ? smoothstepf(0.0f, 0.3f, t) : fminf(smoothstepf(FACE_T, FACE_T - 0.2f, t), smoothstepf(0.0f, 0.35f, t));
    int x = -44 + (int)(k * 50.0f), y = H - 4 - 18 - 4 - 46;
    if (G.readout <= 0.0f) y = H - 4 - 46;
    bevel(f, x, y, 40, 46, 0);
    if (G.ward_hp > 0.0f) for (int i = 1; i < 39; i++) { gfx_px(f, x + i, y + 1, R_BONE + 13); gfx_px(f, x + i, y + 44, R_BONE + 13); }
    draw_face(f, x + 4, y + 3, face_sub());
}

/* what the defensive spells look like while they last */
static void render_wards(Frame *f) {
    if (G.ward_hp > 0.0f) {                                        /* bone shards wheeling round the caster */
        int n = 2 + (int)(G.ward_hp / 12.0f);
        for (int i = 0; i < n && i < 5; i++) {
            float a = (float)G.t * 2.2f + i * TAU_F / n, sa = sinf(a);
            if (sa < -0.2f) continue;                              /* behind */
            int size = (int)(18 + 10 * sa), cx = 160 + (int)(cosf(a) * 150), cy = 128 + (int)(sa * 42);
            const uint8_t *img = crawl_spr[SP_BONEFLY0 + (((int)(G.t * 10.0) + i) & 3)];
            for (int x = 0; x < size; x++)
                for (int y = 0; y < size; y++) {
                    int sx = cx - size / 2 + x, sy = cy - size / 2 + y;
                    if (sx < 0 || sx >= W || sy < 0 || sy >= H) continue;
                    uint8_t c = img[(x * TS / size) * TS + y * TS / size];
                    if (c != C_CLEAR) f->pix[sy * W + sx] = shade(c, 0.9f + 0.3f * sa, sx, sy);
                }
        }
    }
    if (G.mirror_t > 0.0f || G.mirror_flash > 0.0f) {             /* a silvered ripple at the edges of sight */
        float k = fminf(1.0f, fmaxf(G.mirror_t, G.mirror_flash) * 2.0f);
        for (int y = 0; y < H; y++) {
            int w = (int)((5.0f + 3.0f * sinf(y * 0.13f + (float)G.t * 6.0f)) * k);
            for (int x = 0; x < w; x++) {
                if (BAYER[y & 3][x & 3] > (int)(12 * (1.0f - (float)x / (w + 1)))) continue;
                uint8_t c = (uint8_t)(R_GLOW + 8 + ((x + y) & 3));
                f->pix[y * W + x] = c; f->pix[y * W + W - 1 - x] = c;
            }
        }
    }
}

/* The hand as the caster is dressed: the sleeve in the colour of the robe's quality (the caster's own
 * purple without one), a ring on the ring finger with a stone of its quality, and a rune tattooed on the
 * back of the hand for each defensive spell it knows. Rebuilt when any of that changes. */
static uint8_t hand_buf[HAND_H * HAND_W], hand_rune[HAND_H * HAND_W];   /* hand_rune: 1 + spell for tattoo ink */
static Piece hand_worn[NSLOT]; static int8_t hand_def[NDEF]; static int hand_built;
static uint8_t hand_fr[NHF][HAND_H * HAND_W], hand_frl[NHF][HAND_H * HAND_W];   /* its frames, bent from it (crawl_hand_frame) */
static int hand_tip[NHF][2], hand_fr_ok[NHF];                      /* each made when first wanted, after a change */

static const int RING_AT[2] = { 55, 32 };                          /* on the ring finger (back of the hand, fingers up-left) */
static const int RUNE_AT[NDEF][2] = { { 40, 50 }, { 56, 49 }, { 72, 50 }, { 45, 61 }, { 61, 61 }, { 77, 62 }, { 51, 72 }, { 67, 73 }, { 83, 74 } };

