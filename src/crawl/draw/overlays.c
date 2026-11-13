/* A part of scene_crawl.c (docs/PLAN-tidy.md): it is #included there in its place and never compiled on its own.
 * render_ui (which screen is drawn); the wheels; Toughness's cracks; the warded; the things that want a spell. */

static void render_ui(Frame *f) {
    static const char *const NAME[] = { "none", "wardrobe", "quit", "ledger", "bestiary", "letgo", "wisps", "altar", "shrine", "totem",
                                        "portal", "merchant", "stone", "compendium", "repository", "codex", "ascend", "cornerstones",
                                        "alignment", "class", "hall2", "bshrine", "reckon", "controller", "portrait", "buy", "tutor" };
    ui_on(f, 1, G.ui >= 0 && G.ui < (int)(sizeof NAME / sizeof NAME[0]) ? NAME[G.ui] : "?");
    render_ui_in(f);
    ui_off();
}
static void render_ui_in(Frame *f) {
    if (G.ui == UI_ASCEND) { render_ascend(f); return; }
    if (G.ui == UI_ALIGN) { dim_view(f); render_align(f, 0); return; }
    if (G.ui == UI_CLASS) { render_class(f); return; }
    if (G.ui == UI_HALL2) { render_hall2(f); return; }
    if (G.ui == UI_STONES) { dim_view(f); render_stones(f, 0); return; }
    if (G.ui == UI_PORTAL) { render_portalmenu(f); return; }
    if (G.ui == UI_COMP) { render_compmenu(f); return; }
    if (G.ui == UI_REPO) { render_repomenu(f); return; }
    if (G.ui == UI_CODEX) { render_codex(f); return; }
    if (G.ui == UI_SHRINE) { render_shrinemenu(f); return; }
    if (G.ui == UI_TOTEM) { render_totemmenu(f); return; }
    if (G.ui == UI_WISPS) { render_wispmenu(f); return; }
    if (G.ui == UI_ALTAR) { render_altarmenu(f); return; }
    if (G.ui == UI_BSHRINE) { render_bshrine(f); return; }
    if (G.ui == UI_RECKON) { render_reckon(f); return; }
    if (G.ui == UI_PAD) { render_padmenu(f); return; }
    if (G.ui == UI_PORTRAIT) { render_portraitcard(f); return; }
    if (G.ui == UI_BUY) { render_buy(f); return; }
    if (G.ui == UI_TUTOR) { render_tutor(f); return; }
    if (G.ui == UI_INVENTORY) render_inventory(f);
    else if (G.ui == UI_QUIT) render_quit(f);
    else if (G.ui == UI_LEDGER) render_ledger(f);
    else if (G.ui == UI_MERCHANT) render_merchantmenu(f);
    else if (G.ui == UI_STONE) render_stonemenu(f);
    else if (G.ui == UI_BESTIARY) render_bestiary(f);
    else if (G.ui == UI_LETGO) render_letgo(f);
}

/* The world grey under a wheel (1.0.0b16): each pixel drawn so far to the iron ramp by its brightness, dithered in as the
 * wheel comes up; what is drawn after it (the bar, the wheel) keeps its colours */
static void wheel_grey(Frame *f) {
    if (WH.k <= 0.0f) return;
    static const uint8_t BAYER[16] = { 0, 8, 2, 10, 12, 4, 14, 6, 3, 11, 1, 9, 15, 7, 13, 5 };
    uint8_t lut[256];
    for (int c = 0; c < 256; c++) {
        float lum = 0.3f * f->pal[c][0] + 0.59f * f->pal[c][1] + 0.11f * f->pal[c][2];
        lut[c] = c ? (uint8_t)(R_IRON + clampi((int)(lum * 13.0f / 52.0f + 0.5f), 0, 14)) : 0;
    }
    int lim = (int)(WH.k * 16.0f + 0.5f);
    for (int y = 0; y < H; y++) for (int x = 0; x < W; x++)
        if (lim >= 16 || BAYER[(y & 3) * 4 + (x & 3)] < lim) f->pix[y * W + x] = lut[f->pix[y * W + x]];
}
static const char *const BFS_MARK[11] = { "...........", ".#########.", ".#.......#.", ".#.#####.#.", ".#.#...#.#.", ".#.#.o.#.#.",
                                          ".#.#...#.#.", ".#.#####.#.", ".#.......#.", ".#########.", "..........." };
static void wheel_render(Frame *f) {
    if (WH.k <= 0.0f || !WH.n) return;
    float e = smoothstepf(0.0f, 1.0f, WH.k);
    int gifts = WH.open == WH_GIFTS;
    int nr = 0, na = 0;
    for (int i = 0; i < WH.n; i++) { nr += WH.s[i].kind == WS_RUNE; na += WH.s[i].kind == WS_GIFT; }
    int full = gifts ? 60 : 54, small = gifts && imax(nr, na) > 5 ? 9 : 11;   /* a crowded half: smaller sockets */
    int cx = W / 2, cy = 88, R = (int)((float)full * (0.5f + 0.5f * e));
    for (int y = cy - R - 22; y <= cy + R + 22; y++) for (int x = cx - R - 22; x <= cx + R + 22; x++) {   /* a dark ring for it to sit in */
        if (x < 0 || x >= W || y < 0 || y >= H) continue;
        float d = hypotf((float)(x - cx), (float)(y - cy));
        if (d > R + 20) continue;
        uint8_t *px = &f->pix[y * W + x];
        if (*px >= R_IRON && *px < R_IRON + 16) *px = (uint8_t)(R_IRON + (*px - R_IRON) * (d < R - 18 ? 3 : 2) / 5);
        if (fabsf(d - (float)R) < 0.6f && ((x + y) & 1)) *px = UI_M(6);
    }
    for (int i = 0; i < WH.n; i++) {
        const WSlot *w = &WH.s[i];
        int x = cx + (int)lroundf(cosf(w->ang) * (float)R), y = cy + (int)lroundf(sinf(w->ang) * (float)R), sel = i == WH.hi;
        int cur = (w->kind == WS_SPELL && (w->id == RS_ID ? G.rst_sel : w->id == G.spell && !G.rst_sel)) || (w->kind == WS_RUNE && w->id == rune_readied()) || (w->kind == WS_GIFT && w->id == ab_readied());
        int st = !w->ok ? SK_CLOSED : cur ? SK_HELD : SK_READY, r = sel ? small + 2 : small;
        float fill = 1.0f;
        if (w->kind == WS_SPELL && w->id == RS_ID) ui_socket_mark(f, x, y, r, RESTORE_MARK, R_GHOST, st, sel, 0, G.ui_t);   /* Restoration (D5) */
        else if (w->kind == WS_SPELL) {
            int el = ATK_ELEM[w->id];
            if (el >= 0) ui_socket(f, x, y, r, UI_NODE_ICON[el][ND_OPEN], ELEM_RAMP[el], st, sel, 0, G.ui_t);
            else ui_socket_mark(f, x, y, r, BFS_MARK, R_PURPLE, st, sel, 0, G.ui_t);
        } else if (w->kind == WS_RUNE) {
            int q = clampi(RUN.gear.def[w->id], 1, RUNE_EXQ);
            ui_socket_mark(f, x, y, r, RUNE_MARK[w->id], gear_color(GK_DEF, w->id), st, sel, 0, G.ui_t);
            float wait = def_cooldown(q);
            fill = wait > 0.0f ? 1.0f - clampf(G.def_cd[w->id] / wait, 0.0f, 1.0f) : 1.0f;
        } else {
            ui_socket_mark(f, x, y, r, BA_MARK[w->id], BA_RAMP[w->id], st, sel, 0, G.ui_t);
            float wait = ab_wait(w->id);
            fill = wait > 0.0f ? 1.0f - clampf(G.ab_cd[w->id] / wait, 0.0f, 1.0f) : 1.0f;
        }
        if (w->kind != WS_SPELL && w->ok && fill < 1.0f)           /* coming round */
            for (int k = 0; k < 15; k++) gfx_px(f, x - 7 + k, y + r + 2, k < (int)(fill * 15.0f + 0.5f) ? (uint8_t)(R_STONE + 10) : (uint8_t)(R_STONE + 3));
    }
    char a[40] = "", b[40] = "", c[40] = "";                       /* in the middle: what is pointed at */
    if (WH.hi >= 0 && WH.hi < WH.n) {
        const WSlot *w = &WH.s[WH.hi];
        if (w->kind == WS_SPELL && w->id == RS_ID) {               /* Restoration (D5) */
            snprintf(a, sizeof a, "RESTORATION");
            snprintf(b, sizeof b, "RANK %d", imax(1, conf_rank(CF_RESTORE)));
            snprintf(c, sizeof c, "HELD, IT HEALS");
        } else if (w->kind == WS_SPELL) {
            snprintf(a, sizeof a, "%s", w->id == AT_BFS ? "BFS9000" : ELEM_NAME[ATK_ELEM[w->id]]);
            if (!w->ok) snprintf(b, sizeof b, "NOT OPEN");
            else if (w->id == AT_SPEAR) snprintf(b, sizeof b, "RANK %d  %s %d", RUN.gear.atk[w->id], spear_bare() ? "SPLINTERS" : "BONES", spear_bare() ? splint_left() : RUN.gear.bones);
            else snprintf(b, sizeof b, "RANK %d", imax(1, RUN.gear.atk[w->id]));
        } else if (w->kind == WS_RUNE) {
            snprintf(a, sizeof a, "%s", DEF_NAME[w->id]);
            snprintf(b, sizeof b, "%s", QUAL_NAME[clampi(RUN.gear.def[w->id], 1, RUNE_EXQ) - 1]);
            if (!w->ok) snprintf(c, sizeof c, "IT RISES BY ITSELF");
            else if (G.def_cd[w->id] > 0.0f) snprintf(c, sizeof c, "IN %.0f S", ceilf(G.def_cd[w->id]));
            else snprintf(c, sizeof c, "%s", PADX.used ? "READY: RB" : "READY");
        } else {
            static const char *const RK[4] = { "", "RANK I", "RANK II", "RANK III" };
            snprintf(a, sizeof a, "%s", BA_NAME[w->id]);
            snprintf(b, sizeof b, "%s", RK[clampi(RUN.abil[w->id], 0, 3)]);
            if (G.ab_cd[w->id] > 0.0f) snprintf(c, sizeof c, "IN %.0f S", ceilf(G.ab_cd[w->id]));
            else snprintf(c, sizeof c, "%s", PADX.used ? "READY: LB" : "READY");
        }
    }
    int col = UI_M(14);
    if (*a) ui_text8(f, cx - 4 * (int)strlen(a), cy - 12, a, FACE_INSC, R_GOLD + 11, TX_INLAID);
    if (*b) ui_text8(f, cx - 4 * (int)strlen(b), cy - 2, b, FACE_BODY, col, TX_INLAID);
    if (*c) ui_text8(f, cx - 4 * (int)strlen(c), cy + 8, c, FACE_BODY, col, TX_INLAID);
    if (e < 0.6f) return;
    const char *title = WH.open == WH_SPELLS ? "SPELLS" : "RUNES AND GIFTS";
    ui_band(f, cx - 4 * (int)strlen(title) - 6, 2, 8 * (int)strlen(title) + 12, 12);
    ui_text8(f, cx - 4 * (int)strlen(title), 4, title, FACE_INSC, R_GOLD + 12, TX_INLAID);
    if (gifts) {                                                   /* the halves: the gifts left, the runes right */
        const char *l = PADX.used ? "GIFTS: LB" : "GIFTS", *rr = PADX.used ? "RUNES: RB" : "RUNES";
        ui_text8(f, 6, cy - 4, l, FACE_BODY, UI_M(12), TX_INLAID);
        ui_text8(f, W - 6 - 8 * (int)strlen(rr), cy - 4, rr, FACE_BODY, UI_M(12), TX_INLAID);
        if (!nr) ui_text8(f, cx + R - 24, cy - 4, "NONE", FACE_BODY, UI_M(7), TX_INLAID);
        if (!na) ui_text8(f, cx - R - 8, cy - 4, "NONE", FACE_BODY, UI_M(7), TX_INLAID);
    }
    static const char *const KQ[3] = { "MOUSE POINTS", "RELEASE TAKES", "ESC LEAVES" }, *const KM[3] = { "MOUSE POINTS", "RELEASE TAKES", "ESC LEAVES" };
    int hand = WH.src ? PADX.q_hand : PADX.w_hand;                 /* the stick that points: the free hand's (1.0.0b18.1) */
    const char *KPAD[3] = { hand == 1 ? "R-STICK AIMS" : hand == 2 ? "L-STICK AIMS" : "A STICK AIMS", "RELEASE TAKES", "\1B LEAVES" };   /* as long as it was */
    const char *const *K = PADX.used ? KPAD : WH.src ? KQ : KM;
    for (int i = 0; i < 3; i++) ui_keys(f, 6, cy + 20 + i * 12, K[i], UI_M(13));   /* clear of the words in its middle */
}
/* Toughness's cracks (1.0.0b20, PLAN-combat 10.5; D5): motes rising off a shut secret's crack, in the run's colour (the
 * Order's candle gold, the drowned's ghost-light, the mine's amber, the black glass's violet), seen as far as Toughness's
 * reach (conf_value: today's two cells and the weighted gain past them, 4 to 8 at full weight), more of them by its rank,
 * at full brightness through dark and fog, as the eyes are, hidden by the walls. Within two cells the cracks show anyway */
static int cracks_on(void) {                                       /* in play (the owner approved the sheet); CRAWL_CRACKS=0 (debug) leaves them out */
#ifdef CRAWL_DEBUG
    static int on = -1;
    if (on < 0) on = !(getenv("CRAWL_CRACKS") && atoi(getenv("CRAWL_CRACKS")) == 0);
    return on;
#else
    return 1;
#endif
}
static void render_cracks(Frame *f, float dx, float dy, float horizon) {
    static const uint8_t RAMP[4] = { R_GOLD, R_GHOST, R_FLAME, R_PURPLE };
    static const int TOP[4] = { 12, 15, 15, 15 };                  /* the gold ramp is thirteen */
    float reach = conf_value(CF_TOUGH, CV_CRACKS);
    if (reach <= 0.0f || !cracks_on() || PK.depth || G.shop || G.dead) return;
    int run = crawl_set(G.level), n = 3 + 3 * imin(imax(1, conf_rank(CF_TOUGH)), 5);
    for (int i = 0; i < G.nsecrets; i++) {
        const Secret *sc = &G.secrets[i];
        if (sc->state) continue;
        float nx = (float)(sc->sx - sc->fx), ny = (float)(sc->sy - sc->fy);   /* into the wall */
        float cx = (float)sc->fx + 0.5f + 0.5f * nx, cy = (float)sc->fy + 0.5f + 0.5f * ny;   /* the crack's face */
        float d = hypotf(cx - G.px, cy - G.py);
        if (d > reach || (G.px - cx) * nx + (G.py - cy) * ny > 0.0f) continue;   /* too far, or behind it */
#ifdef CRAWL_DEBUG
        { static double said; if (getenv("CRAWL_CRACKS") && atoi(getenv("CRAWL_CRACKS")) >= 2 && G.t - said > 1.0) { said = G.t;   /* CRAWL_CRACKS=2: where */
            fprintf(stderr, "CRACKS secret %d face (%.1f,%.1f) at %.1f cells, reach %.1f, rank %d t=%.2f\n", i, cx, cy, d, reach, conf_rank(CF_TOUGH), G.t); } }
#endif
        float edge = 1.0f - smoothstepf(reach - 1.0f, reach, d);   /* thinning at the edge of its reach */
        for (int k = 0; k < n; k++) {
            uint32_t h = hash2(sc->sx * 31 + k, sc->sy * 17 + i);
            float life = 1.8f + 1.2f * (float)(h & 255) / 255.0f, ph = fmodf((float)G.t / life + (float)(h >> 8 & 255) / 255.0f, 1.0f);
            float along = ((float)(h >> 16 & 255) / 255.0f - 0.5f) * 0.8f + 0.05f * sinf((float)G.t * 1.3f + (float)k * 2.1f);
            float out = 0.04f + 0.14f * ph, z = 0.06f + 0.8f * ph;   /* off the stone and up, slowly */
            float x = cx - nx * out - ny * along, y = cy - ny * out + nx * along, sx, sy, dep;
            if (!project(x, y, z, dx, dy, horizon, &sx, &sy, &dep)) continue;
            float a = smoothstepf(0.0f, 0.15f, ph) * (1.0f - smoothstepf(0.55f, 1.0f, ph)) * edge;
            int lv = (int)(a * 16.0f);
            if (lv < 3) continue;
            uint8_t c = (uint8_t)(RAMP[run] + imin(TOP[run], 6 + lv * (TOP[run] - 5) / 16)), halo = (uint8_t)(RAMP[run] + 3 + lv / 4);
            int px = (int)floorf(sx), py = (int)floorf(sy), sz = FOCAL / dep / 64.0f > 1.6f ? 2 : 1;
            for (int yy = -1; yy <= sz; yy++)                      /* a bright core, a dim cross about it */
                for (int xx = -1; xx <= sz; xx++) {
                    int core = xx >= 0 && xx < sz && yy >= 0 && yy < sz, edge = (xx < 0 || xx >= sz) != (yy < 0 || yy >= sz);
                    int qx = px + xx, qy = py + yy;
                    if ((core || (edge && lv >= 6)) && qx >= 0 && qx < W && qy >= 0 && qy < H && dep < zbuf[qx] + 0.02f)
                        f->pix[qy * W + qx] = core ? c : halo;
                }
        }
    }
}
/* a creature near you strong against a line you hold (its rank's, once seen struck; a ward, 1.0.0b16): its element's glyph
 * over its head, barred (docs/PLAN-play.md section 6, PLAN-combat.md 7.3) */
static void render_warded(Frame *f, float dx, float dy, float horizon) {
    for (int i = 0; i < MAXMOBS; i++) {                            /* Static Build (D7): a small bolt over its head for each strike that
                                                                      raises its stun, three at most */
        const Mob *m = &G.mobs[i];
        int np = aug_show(2) ? 3 : imin(3, m->stb_n - 1);
        if (np <= 0 || !m->alive || m->state != MS_WALK || buried(m) || hypotf(m->x - G.px, m->y - G.py) > 12.0f
            || !line_clear(G.px, G.py, m->x, m->y)) continue;
        float sx, sy, dep, z = fminf(1.1f, KINDS[m->type].scale * RANK_SIZE[m->rank] + 0.02f + fmaxf(0.0f, m->z));
        if (!project(m->x, m->y, z, dx, dy, horizon, &sx, &sy, &dep)) continue;
        static const char BOLT[6][4] = { ".##", ".#.", "##.", ".##", ".#.", "#.." };
        int ps = clampi((int)(FOCAL / dep / 64.0f * 2.0f + 0.5f), 1, 3), gap = 5 * ps;
        for (int p = 0; p < np; p++) {
            int gx = (int)sx - (np * gap) / 2 + p * gap, gy = (int)sy - 7 * ps;
            int lit = ((int)(G.t * 9.0) + p * 3 + i) % 7 != 0;   /* now and then one flickers out */
            for (int r = 0; r < 6; r++)
                for (int c = 0; c < 3; c++) {
                    if (BOLT[r][c] != '#') continue;
                    uint8_t col = lit ? (((int)(G.t * 14.0) + r + p) % 9 == 0 ? (uint8_t)C_WHITE : (uint8_t)(R_GOLD + 12 - (r == 0 || r == 5))) : (uint8_t)(R_GOLD + 6);
                    for (int yy = 0; yy < ps; yy++)
                        for (int xx = 0; xx < ps; xx++) {
                            int qx = gx + c * ps + xx, qy = gy + r * ps + yy;
                            if (qx >= 0 && qx < W && qy >= 0 && qy < H && dep < zbuf[qx] + 0.05f) f->pix[qy * W + qx] = col;
                        }
                }
        }
    }
    for (int i = 0; i < MAXMOBS; i++) {                            /* stunned (1.0.0b19): a ring of sparks going round over its head */
        const Mob *m = &G.mobs[i];
        if (!m->alive || m->state != MS_WALK || m->stun_t <= 0.0f || buried(m) || hypotf(m->x - G.px, m->y - G.py) > 12.0f
            || !line_clear(G.px, G.py, m->x, m->y)) continue;
        float sx, sy, dep, z = fminf(1.1f, KINDS[m->type].scale * RANK_SIZE[m->rank] + 0.06f + fmaxf(0.0f, m->z));
        if (!project(m->x, m->y, z, dx, dy, horizon, &sx, &sy, &dep)) continue;
        float r = clampf(FOCAL / dep * 0.16f, 3.0f, 16.0f);
        int ps = dep < 4.0f ? 2 : 1;
        for (int k = 0; k < 5; k++) {
            float a = (float)G.t * 6.0f + (float)k * TAU_F / 5.0f;
            int px = (int)(sx + cosf(a) * r), py = (int)(sy + sinf(a) * r * 0.35f);
            uint8_t c = k & 1 ? (uint8_t)C_WHITE : (uint8_t)(R_GOLD + 12);
            for (int yy = 0; yy < ps; yy++)
                for (int xx = 0; xx < ps; xx++) {
                    int qx = px + xx, qy = py + yy;
                    if (qx >= 0 && qx < W && qy >= 0 && qy < H && dep < zbuf[qx] + 0.05f) f->pix[qy * W + qx] = c;
                }
        }
    }
    for (int i = 0; i < MAXMOBS; i++) {
        const Mob *m = &G.mobs[i];
        int sh[2], nw = 0;
        for (int w = 0; w < 2; w++) if (strength_shown(m, w)) sh[nw++] = m->ward[w];
        if (!m->alive || m->state != MS_WALK || !nw || buried(m) || m->sleep == 1) continue;
        float d = hypotf(m->x - G.px, m->y - G.py);
        if (d > 7.0f || (m->type == MOB_SHADE && shade_alpha(m) < 0.2f) || !line_clear(G.px, G.py, m->x, m->y)) continue;
        float sx, sy, dep, z = fminf(1.05f, 0.95f * RANK_SIZE[m->rank] + 0.08f);
        if (!project(m->x, m->y, z, dx, dy, horizon, &sx, &sy, &dep)) continue;
        int k = clampi((int)(FOCAL / dep / 64.0f * 1.6f + 0.5f), 1, 3);
        for (int w = 0; w < nw; w++) {
            int e = sh[w] - 1, gx = (int)sx - (nw * 9 * k) / 2 + w * 9 * k, gy = (int)sy - 8 * k;
            for (int r = -1; r < 8; r++)
                for (int c = -1; c < 8; c++) {
                    char ch = r >= 0 && r < 7 && c >= 0 && c < 7 ? ELEM_GLYPH[e][r][c] : '.';
                    int bar = r == 3 && c >= -1 && c <= 7;          /* barred */
                    uint8_t col = bar ? (uint8_t)(m->wring > 0.0f ? C_WHITE : R_STONE + 13) : (ch == '#' || ch == 'o') ? ramp_at(ELEM_RAMP[e], ch == 'o' ? 15 : 12) : 0;
                    if (!col) continue;
                    for (int yy = 0; yy < k; yy++)
                        for (int xx = 0; xx < k; xx++) {
                            int px = gx + c * k + xx, py = gy + r * k + yy;
                            if (px >= 0 && px < W && py >= 0 && py < H && dep < zbuf[px] + 0.05f) f->pix[py * W + px] = col;
                        }
                }
        }
    }
    render_cracks(f, dx, dy, horizon);                             /* and Toughness's motes at a shut secret's crack (D5) */
}
/* the seals on things that want a spell, at full brightness through dark and fog, hidden by the walls; never smaller than
 * about half a texel a pixel, so one down a long hall can still be read */
/* a lit soul-lamp's pips, five on its sprite: those not banked put out, the one burning flickering in its last seconds */
static void ward_pips(Frame *f, float dx, float dy, float horizon) {
    static const int PIPY[4] = { 51, 40, 46, 47 };                 /* the pips' row on each run's lamp (crawl_art.c, spr_wlamp) */
    static const uint8_t DARK[4] = { R_IRON + 1, R_WOOD + 1, R_IRON + 1, R_STONE + 1 };
    if (PK.depth || G.shop || !WD.mask) return;
    int run = crawl_set(G.level) & 3;
    for (int i = 0; i < WD.n; i++) {
        const WLamp *L = &WD.l[i];
        const Deco *dc = &G.decos[L->deco];
        float sx, sy, dep;
        if (L->banked <= 0.0f || hypotf(dc->x - G.px, dc->y - G.py) > 12.0f) continue;
        if (!project(dc->x, dc->y, 1.0f - (float)PIPY[run] / 64.0f, dx, dy, horizon, &sx, &sy, &dep)) continue;
        float tex = FOCAL / dep / 64.0f, part = fmodf(L->banked, WARD_SOUL_T);
        int lit = (int)ceilf(L->banked / WARD_SOUL_T - 1e-3f);
        if (part < 1e-3f) part = WARD_SOUL_T;
        for (int p = 0; p < 5; p++) {
            int dark = p >= lit || (p == lit - 1 && part < 15.0f && fmodf((float)G.t * 3.0f, 1.0f) < 0.45f);
            if (!dark) continue;
            int x0 = (int)floorf(sx + (float)(24 + 4 * p - 32) * tex), x1 = (int)floorf(sx + (float)(26 + 4 * p - 32) * tex);
            int y0 = (int)floorf(sy), y1 = (int)floorf(sy + 2.0f * tex);
            if (x1 <= x0) x1 = x0 + 1;
            if (y1 <= y0) y1 = y0 + 1;
            for (int x = x0; x < x1; x++) {
                if (x < 0 || x >= W || dep >= zbuf[x] + 0.05f) continue;
                for (int y = y0; y < y1; y++) if (y >= 0 && y < H) f->pix[y * W + x] = DARK[run];
            }
        }
    }
}
static void render_marks(Frame *f, float dx, float dy, float horizon) {
    for (int i = 0; i < G.nmk; i++) {
        const Marked *m = &G.mk[i];
        if ((m->done && m->done_t > 3.0f) || hypotf(m->x - G.px, m->y - G.py) > 15.0f) continue;
        const MkKind *K = &MKK[m->kind];
        float sx, sy, dep;
        if (!project(m->x, m->y, mk_z(m), dx, dy, horizon, &sx, &sy, &dep)) continue;
        float tex = FOCAL / dep / 64.0f, k = clampf(tex, 0.55f, 2.4f), sx0 = sx;
        if (m->kind == MKK_RELIQ && m->st == 1 && !m->done) continue;   /* turning: no face to strike */
        int nseal = m->kind == MKK_TABLET ? 3 : 1;                 /* the tablet shows its order, a glyph a socket */
        for (int q = 0; q < nseal; q++) {
        sx = sx0 + (float)((m->done ? K->dx : K->sx) - 32 + (nseal > 1 ? (q - 1) * 12 : 0)) * tex;
        int state = m->done ? MK_DONE : m->hit_t > 0.0f ? MK_HIT : m->wrong_t > 0.0f ? MK_WRONG : MK_WAIT;
        if (nseal > 1) state = m->done || q < m->st ? MK_DONE : MK_WAIT;
        static uint8_t buf[48 * 48];
        memset(buf, C_CLEAR, sizeof buf);
        crawl_mark(buf, 48, 48, 24, 24, nseal > 1 ? m->els[q] : m->el, state, nseal > 1 ? 1 : m->pips, nseal > 1 ? 0 : m->got, (float)G.t + (float)i * 0.37f + q * 0.5f);
        for (int by = 0; by < 48; by++)
            for (int bx = 0; bx < 48; bx++) {
                uint8_t c = buf[by * 48 + bx];
                if (c == C_CLEAR) continue;
                int x0 = (int)floorf(sx + ((float)bx - 24.0f) * k), y0 = (int)floorf(sy + ((float)by - 24.0f) * k);
                int x1 = (int)floorf(sx + ((float)bx - 23.0f) * k), y1 = (int)floorf(sy + ((float)by - 23.0f) * k);
                if (x1 <= x0) x1 = x0 + 1;
                if (y1 <= y0) y1 = y0 + 1;
                for (int x = x0; x < x1; x++) {
                    if (x < 0 || x >= W || dep >= zbuf[x] + 0.05f) continue;
                    for (int y = y0; y < y1; y++) if (y >= 0 && y < H) f->pix[y * W + x] = c;
                }
            }
        }
    }
    ward_pips(f, dx, dy, horizon);                                 /* and the soul-lamps' pips (1.0.0b18.1) */
}
