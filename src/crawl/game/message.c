/* A part of scene_crawl.c (docs/PLAN-tidy.md): it is #included there in its place and never compiled on its own.
 * The hidden message, Tab's documents, the Black Shrine. */

/* ---- the hidden message, Tab's documents and the Black Shrine (1.0.0b13, docs/PLAN-b13.md section 6a) ----
 * On a puzzle level the answer is written down and hidden in one of the level's own things of the run's kind, away
 * from the wing's door, and a few more of the run's own stand about holding nothing. E at one searches it, a second's
 * work. Within two cells the one that holds it glints now and then; Treasure Hunter makes more of that, and whispers
 * near it. A Black Shrine of its own stands off the way: for souls it shows where the message lies, through the walls
 * and on the map, for two and a half minutes. What is found and every clue read go into Tab's DOCUMENTS until the
 * level ends. The dream never searches (it knows the answer), and a person who takes over finds the page empty. */
#define MAXDOC 16
#define BSH_SHOW 150.0f
static struct { int n, sel; struct { char where[64], text[320]; uint8_t msg, run; } d[MAXDOC]; } DOCS;
static const int16_t HIDES[4][4] = { { SP_HIDE0, SP_URN, SP_CANDLES, -1 },          /* a candle-box, a burial urn, a niche's candles */
                                     { SP_HIDE0 + 1, SP_URN, -1, -1 },             /* a drowned satchel, an urn */
                                     { SP_HIDE0 + 2, SP_CART, SP_MINELAMP_A, -1 }, /* a miner's boot, a cart's toolbox, a lamp's base */
                                     { SP_HIDE0 + 3, SP_SKULLPILE, -1, -1 } };     /* a broken geode, the fused dead */
static const char *const DOC_NAME[4] = { "A LEAF FROM THE ORDER'S BOOK OF HOURS", "A LETTER, SEALED IN A STOPPERED BOTTLE",
                                         "A FOREMAN'S NOTE ON A SCRAP OF SACKING", "A STRIP OF GOLD LEAF, SCRATCHED WITH A STYLUS" };
static const char *const DOC_SHORT[4] = { "THE LEAF", "THE LETTER", "THE NOTE", "GOLD LEAF" };
static const char *const DOC_TOAST[4] = { "A LEAF OF THE BOOK OF HOURS", "A LETTER IN A BOTTLE", "A FOREMAN'S NOTE", "A STRIP OF GOLD LEAF" };
static const char *const DOC_OPEN[4] = { "BROTHER, LEST IT BE FORGOT:", "TO WHOEVER THE WATER BRINGS THIS. THE REST OF US ARE GONE. THE WAY IS THIS:",
                                         "FOR THE NEXT SHIFT, AND NOT A WORD TO THE GANGER:", "THE COURT KEEPS THIS, CUT IN GOLD:" };
static const char *const DOC_DUST[4] = { "DUST", "SILT", "GRIT", "ASH" };
static int doc_run(void) { return crawl_set(G.level) & 3; }
static int hide_kind(int spr) {                                    /* a thing of the run's kind that may hold it */
    const int16_t *h = HIDES[doc_run()];
    for (int k = 0; h[k] >= 0; k++) if (h[k] == spr) return 1;
    return 0;
}
static const char *hide_name(int spr) {
    switch (spr) {
    case SP_HIDE0: return "THE CANDLE-BOX";
    case SP_HIDE0 + 1: return "THE DROWNED SATCHEL";
    case SP_HIDE0 + 2: return "THE MINER'S BOOT";
    case SP_HIDE0 + 3: return "THE BROKEN GEODE";
    case SP_URN: return "THE URN";
    case SP_SKULLPILE: return "THE FUSED DEAD";
    case SP_CANDLES: return "THE CANDLES";
    case SP_CART: return "THE CART'S TOOLBOX";
    default: return "THE LAMP'S BASE";
    }
}
static void docs_clear(void) { DOCS.n = DOCS.sel = 0; }
/* into Tab's documents, the newest first: a clue headed by where it was read, or the message by what it is */
static void doc_add(const char *where, const char *text, int msg) {
    if (!G.player || !text || !text[0] || G.shop) return;          /* the dream writes nothing down */
    for (int i = 0; i < DOCS.n; i++) if (!strcmp(DOCS.d[i].text, text)) return;   /* read before */
    if (DOCS.n == MAXDOC) {                                        /* full: the oldest clue goes */
        int o = 0;
        while (o < DOCS.n - 1 && DOCS.d[o].msg) o++;
        memmove(&DOCS.d[o], &DOCS.d[o + 1], sizeof DOCS.d[0] * (size_t)(DOCS.n - 1 - o));
        DOCS.n--;
    }
    int k = DOCS.n++;
    DOCS.d[k].msg = (uint8_t)msg; DOCS.d[k].run = (uint8_t)doc_run();
    if (msg) snprintf(DOCS.d[k].where, sizeof DOCS.d[k].where, "%s", where);
    else {
        const char *room = RTYPES[cell_rtype((int)G.px, (int)G.py)].name;
        snprintf(DOCS.d[k].where, sizeof DOCS.d[k].where, "%s, IN %s%s", where, strncmp(room, "THE ", 4) ? "THE " : "", room);
    }
    snprintf(DOCS.d[k].text, sizeof DOCS.d[k].text, "%s", text);
    DOCS.sel = 0;
}
static int abil_grade(int ab) {                                    /* the best quality worn with an ability: 1 Common to 5 Exquisite, 0 none */
    int q = 0;
    for (int s = 0; s < NSLOT; s++) if (RUN.gear.worn[s].kind != IK_NONE && RUN.gear.worn[s].ability == ab) q = imax(q, RUN.gear.worn[s].quality + 1);
    return q;
}
/* a cell for something laid on the level: open floor off the lanes and the ways, not in the wing, a vault, a secret or
 * the arena, nor within near steps of the wing's door, nor by the exit or a puzzle's thing */
static int hide_cell_ok(int x, int y, int near) {
    if (!hz_floor(x, y) || lane(x, y) || by_way(x, y) || G.bulky[y][x] || item_at(x, y) || in_band(y) || in_wing(x, y) || in_trial(x, y) || vault_of_room(x, y) >= 0) return 0;
    if (G.map[y][x].region == ARENA_REGION) return 0;
    if (G.pz_door >= 0 && abs(x - G.doors[G.pz_door].x) + abs(y - G.doors[G.pz_door].y) < near) return 0;
    if (G.exit_fx >= 0 && abs(x - G.exit_fx) + abs(y - G.exit_fy) < 2) return 0;
    if (G.altar_x >= 0 && abs(x - G.altar_x) + abs(y - G.altar_y) < 3) return 0;
    for (int i = 0; i < G.nthing; i++) if (abs(G.thing[i].cx - x) + abs(G.thing[i].cy - y) < 2) return 0;
    return 1;
}
static int reached(int x, int y) {                                 /* on a lane walked to from the start, or beside one */
    if (bfs_dist[y][x] >= 0) return 1;
    for (int d = 0; d < 4; d++) if (bfs_dist[y + DY[d]][x + DX[d]] >= 0 && G.map[y + DY[d]][x + DX[d]].kind == K_FLOOR) return 1;
    return 0;
}
static void pz_hide(void) {
    G.msg_deco = G.srch_deco = G.bsh_x = G.bsh_y = G.bsh_deco = -1; G.msg_found = G.bsh_found = G.nsearched = G.bsh_arm = 0;
    G.bsh_show = G.bsh_flare = G.srch_t = 0.0f;
    if (G.pz < 0 || G.pz_door < 0 || !G.pz_answer[0]) return;
    int run = doc_run(), want = 3 + rnd(3);
    const Door *dr = &G.doors[G.pz_door];
    for (int tries = 0; tries < 500 && want > 0; tries++) {         /* a few of the run's own, holding nothing (but one may) */
        int x = 1 + rnd(MAPN - 2), y = 1 + rnd(MAPN - 2);
        if (!hide_cell_ok(x, y, 6)) continue;
        add_deco(x + 0.5f + (rndf() - 0.5f) * 0.3f, y + 0.5f + (rndf() - 0.5f) * 0.3f, SP_HIDE0 + run);
        G.bulky[y][x] = 1;
        want--;
    }
    bfs_from(G.cx, G.cy);
    int cand[96], nc = 0;
    for (int i = 0; i < G.ndecos && nc < 96; i++) {
        const Deco *dc = &G.decos[i];
        int x = (int)dc->x, y = (int)dc->y;
        if (dc->spr < 0 || !hide_kind(dc->spr) || x < 1 || y < 1 || x >= MAPN - 1 || y >= MAPN - 1) continue;
        if (in_wing(x, y) || in_trial(x, y) || vault_of_room(x, y) >= 0 || G.map[y][x].secret || in_band(y) || G.map[y][x].region == ARENA_REGION) continue;
        if (abs(x - dr->x) + abs(y - dr->y) < 8 || !reached(x, y)) continue;
        cand[nc++] = i;
    }
    if (!nc) {                                                     /* none of the level's own: one of the run's, laid for it */
        for (int tries = 0; tries < 800 && !nc; tries++) {
            int x = 1 + rnd(MAPN - 2), y = 1 + rnd(MAPN - 2);
            if (!hide_cell_ok(x, y, 8) || !reached(x, y)) continue;
            cand[nc++] = G.ndecos;
            add_deco(x + 0.5f, y + 0.5f, SP_HIDE0 + run);
            G.bulky[y][x] = 1;
        }
        if (!nc || cand[0] >= G.ndecos) return;
    }
    G.msg_deco = cand[rnd(nc)];
    const Deco *md = &G.decos[G.msg_deco];
    for (int tries = 0; tries < 800; tries++) {                    /* the shrine: off the way, away from the door and the message */
        int x = 1 + rnd(MAPN - 2), y = 1 + rnd(MAPN - 2);
        if (!hide_cell_ok(x, y, 8) || !block_corner(x, y) || !reached(x, y)) continue;
        if (hypotf(md->x - x - 0.5f, md->y - y - 0.5f) < 6.0f) continue;
        G.bsh_deco = G.ndecos; G.bsh_x = x; G.bsh_y = y;
        solid_prop(x, y, SP_BSHRINE, 1, K_PROP);
        light_static(x + 0.5f, y + 0.5f, 0.5f, 2.4f);              /* its candle */
        break;
    }
#ifdef CRAWL_DEBUG
    if (getenv("CRAWL_BSHOW")) G.bsh_show = BSH_SHOW;               /* testing: the shrine's showing from the start */
    fprintf(stderr, "MESSAGE level %d: in %s at (%.1f,%.1f) of %d places; shrine at (%d,%d), %d souls\n", G.level, hide_name(md->spr), md->x, md->y, nc,
            G.bsh_x, G.bsh_y, imin(40, 10 + G.level / 2));
#endif
}
static int bsh_price(void) { return imin(40, 10 + G.level / 2); }  /* 15 at level 10, 30 at 40 */
static float msg_shown(void) {                                     /* the shrine's showing: 1, fading in its last ten seconds, 0 done */
    if (G.bsh_show <= 0.0f || G.msg_found || G.msg_deco < 0) return 0.0f;
    return fminf(1.0f, G.bsh_show / 10.0f);
}
/* the glint on the thing that holds it, now and then: within two cells, or with Treasure Hunter three (Common) to six
 * (Exquisite), and through the walls within half that. How bright, 0 none; through: whether the walls hide it */
static float msg_glint(int *through) {
    *through = 0;
    if (G.msg_deco < 0 || G.msg_found || G.shop || PK.depth) return 0.0f;
    const Deco *dc = &G.decos[G.msg_deco];
    int q = abil_grade(AB_TREASURE);
    float d = hypotf(dc->x - G.px, dc->y - G.py), reach = q ? 3.0f + 0.75f * (float)(q - 1) : 2.0f;
    if (d > reach) return 0.0f;
    *through = q && d <= reach * 0.5f;
    float per = q ? 1.7f : 3.4f, u = fmodf((float)G.t + dc->phase * 7.0f, per) / per;
    return u < 0.16f ? sinf(u / 0.16f * PI_F) * (q ? 1.0f : 0.7f) : 0.0f;
}
/* what E would search: one of the run's kind within reach, near the middle of the view, in sight */
static int hide_facing(void) {
    if (G.msg_deco < 0 || G.msg_found || G.shop || PK.depth) return -1;
    int best = -1;
    float bs = 1e9f;
    for (int i = 0; i < G.ndecos; i++) {
        const Deco *dc = &G.decos[i];
        if (dc->spr < 0 || !hide_kind(dc->spr)) continue;
        float d = hypotf(dc->x - G.px, dc->y - G.py);
        if (d > 1.8f) continue;
        float a = fabsf(wrap_pi(atan2f(dc->y - G.py, dc->x - G.px) - G.ang));
        if (a > 0.45f + 0.35f / fmaxf(d, 0.4f)) continue;
        float k = fminf(0.75f / fmaxf(d, 0.1f), 0.9f);              /* a point on its near side, for one that fills its cell */
        if (!line_clear(G.px, G.py, dc->x + (G.px - dc->x) * k, dc->y + (G.py - dc->y) * k)) continue;
        float sc = d + a * 1.5f;
        if (sc < bs) { bs = sc; best = i; }
    }
    return best;
}
static int was_searched(int i) { for (int k = 0; k < G.nsearched; k++) if (G.searched[k] == i) return 1; return 0; }
static void search_begin(int i) {
    const Deco *dc = &G.decos[i];
    if (was_searched(i)) { say("YOU HAVE SEARCHED %s", hide_name(dc->spr)); sfx_at(SFX_KNOCK, dc->x, dc->y, 0.3f); return; }
    G.srch_deco = i; G.srch_t = 1.0f;
    say("SEARCHING %s", hide_name(dc->spr));
    sfx_at(SFX_RUSTLE, dc->x, dc->y, 0.8f);
}
static void msg_take(void) {
    int run = doc_run();
    char text[320];
    snprintf(text, sizeof text, "%s %s", DOC_OPEN[run], G.pz_answer);
    size_t n = strlen(text);
    while (n && (text[n - 1] == ' ' || text[n - 1] == ',' || text[n - 1] == ':')) text[--n] = 0;   /* the list's last, closed */
    if (n && n + 1 < sizeof text && text[n - 1] != '.') { text[n] = '.'; text[n + 1] = 0; }
    G.msg_found = 1; G.bsh_show = 0.0f;
    doc_add(DOC_NAME[run], text, 1);
    toast(SP_HIDE0 + run, -1, RK_I, 0, DOC_TOAST[run], "TAB: DOCUMENTS");
    say("%s", DOC_NAME[run]);
    snd_sfx_ex(SFX_PAGE, 0.9f, 0.5f);
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT msg_found level %d t=%.2f\n", G.level, G.t);
#endif
}
/* a search under way: a second with the hand in it; walking off or turning away stops it */
static void update_search(float dt) {
    if (G.srch_deco < 0) return;
    const Deco *dc = &G.decos[G.srch_deco];
    float d = hypotf(dc->x - G.px, dc->y - G.py), a = fabsf(wrap_pi(atan2f(dc->y - G.py, dc->x - G.px) - G.ang));
    if (!G.player || G.dead || G.ui || d > 2.0f || a > 1.2f) { G.srch_deco = -1; return; }
    G.grab_t = fmaxf(G.grab_t, 0.4f);
    if ((G.srch_t -= dt) > 0.0f) return;
    int i = G.srch_deco;
    G.srch_deco = -1;
    if (G.nsearched < (int)(sizeof G.searched / sizeof G.searched[0])) G.searched[G.nsearched++] = (int16_t)i;
    if (i == G.msg_deco) { msg_take(); return; }
    say("NOTHING IN %s BUT %s", hide_name(dc->spr), DOC_DUST[doc_run()]);
    sfx_at(SFX_KNOCK, dc->x, dc->y, 0.25f);
}
/* the Black Shrine: found once seen within its light or passed within three cells; its showing runs down */
static void update_bshrine_t(float dt) {
    if (G.shop || PK.depth || G.bsh_x < 0) return;
    G.bsh_flare = fmaxf(0.0f, G.bsh_flare - dt);
    if (G.bsh_show > 0.0f && (G.bsh_show -= dt) <= 0.0f) { G.bsh_show = 0.0f; if (G.player && !G.msg_found) say("THE SHRINE'S SHOWING FADES"); }
    if (G.bsh_found || G.dead) return;
    float sx = G.bsh_x + 0.5f, sy = G.bsh_y + 0.5f, d = hypotf(sx - G.px, sy - G.py);
    int seen = d < 7.0f && fabsf(wrap_pi(atan2f(sy - G.py, sx - G.px) - G.ang)) < 0.6f && line_clear(G.px, G.py, sx, sy) && lm_at(sx, sy) > 0.08f;
    if (d >= 3.0f && !seen) return;
    G.bsh_found = 1;
    if (G.player) say("A BLACK SHRINE");
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT bshrine_found t=%.2f\n", G.t);
#endif
}
static int bshrine_facing(void) {
    if (G.shop || PK.depth || G.bsh_x < 0) return 0;
    float sx = G.bsh_x + 0.5f, sy = G.bsh_y + 0.5f;
    return hypotf(sx - G.px, sy - G.py) < 1.6f && fabsf(wrap_pi(atan2f(sy - G.py, sx - G.px) - G.ang)) < 0.7f;
}
/* its one row: show where it is hidden, for souls, asked twice */
static void update_bshrine(void) {
    if (IN.hit[KEY_ESC] || IN.hit[KEY_TAB]) { G.ui = UI_NONE; return; }
    if (!(IN.hit[KEY_ENTER] || IN.hit[KEY_E] || IN.hit[KEY_SPACE] || IN.hit[KEY_MOUSE1])) return;
    if (G.msg_found || G.msg_deco < 0) { snd_sfx_ex(SFX_KNOCK, 0.5f, 0.5f); return; }
    if (spirit() + 1e-4f < (float)bsh_price()) { G.bsh_arm = 0; snd_sfx_ex(SFX_KNOCK, 0.6f, 0.5f); return; }
    if (!G.bsh_arm) { G.bsh_arm = 1; snd_sfx_ex(SFX_KNOCK, 0.6f, 0.5f); return; }
    spend_spirit((float)bsh_price());
    G.bsh_show = BSH_SHOW; G.bsh_flare = 2.0f; G.bsh_arm = 0; G.ui = UI_NONE;
    say("THE SHRINE SHOWS WHERE IT IS HIDDEN");
    snd_sfx_ex(SFX_WHISPER, 1.0f, 0.5f); sfx_at(SFX_SHIMMER, G.bsh_x + 0.5f, G.bsh_y + 0.5f, 0.9f);
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT bshrine_show price=%d t=%.2f\n", bsh_price(), G.t);
#endif
}
static void docs_keys(void) {                                      /* Tab's documents: W/S choose */
    if (DOCS.n < 2) return;
    int ud = (IN.hit[KEY_S] || IN.hit[KEY_DOWN]) - (IN.hit[KEY_W] || IN.hit[KEY_UP]);
    if (ud) { DOCS.sel = (DOCS.sel + DOCS.n + ud) % DOCS.n; snd_sfx_ex(SFX_PAGE, 0.35f, 0.5f); }
}
#ifdef CRAWL_DEBUG
static void tp_near(float tx, float ty) {                          /* testing: a cell or two from a thing, facing it */
    for (int r = 1; r <= 2; r++)
        for (int d = 0; d < 4; d++) {
            int x = (int)tx + DX[d] * r, y = (int)ty + DY[d] * r;
            float k = 0.75f / (float)r;                            /* to its near side: it may fill its cell */
            if (!hz_floor(x, y) || in_wing(x, y) || !line_clear(x + 0.5f, y + 0.5f, tx + (x + 0.5f - tx) * k, ty + (y + 0.5f - ty) * k)) continue;
            G.px = x + 0.5f; G.py = y + 0.5f; G.ang = atan2f(ty - G.py, tx - G.px); human_begin();
            return;
        }
}
#endif

