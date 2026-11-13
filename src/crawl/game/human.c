/* A part of scene_crawl.c (docs/PLAN-tidy.md): it is #included there in its place and never compiled on its own.
 * A person at the controls: captions, doors, the exit and the reckoning, E (human_use), the body against walls, the target looked at; Restoration; the wheels; the sprint, human_walk, open_ui, human_cast. */

/* ================================================================== */
/* A person at the controls (hold H for three seconds)                 */
/* ================================================================== */

#define MOUSE_TURN 0.0015f                  /* radians per mouse count */
#define KEY_TURN 2.6f                       /* radians a second with the arrow keys */
static const char *const KEY_COLOR[NKEYS] = { "GOLD", "SILVER", "CRIMSON" };

static void say(const char *fmt, ...) {
    char b[64];
    va_list ap;
    va_start(ap, fmt); vsnprintf(b, sizeof b, fmt, ap); va_end(ap);
    if (strcmp(b, G.msg) || G.msg_t < 1.2f) { snprintf(G.msg, sizeof G.msg, "%s", b); G.msg_t = 2.2f; }
}

/* The first time each kind of creature, in each rank, comes into view on a level, a caption names
 * it: A BLOODY SKELETON. One at a time; the rest wait their turn. */
static void name_newcomers(void) {
    if (G.msg_t > 0.4f || G.dead || G.shop || G.tally) return;
    float fx = cosf(G.ang), fy = sinf(G.ang);
    for (int i = 0; i < MAXMOBS; i++) {
        const Mob *m = &G.mobs[i];
        if (!m->alive || m->state != MS_WALK || (G.named[m->type] >> m->rank & 1)) continue;
        if ((m->type == MOB_SPIDER || m->type == MOB_GARGOYLE) && m->sub == 0) continue;   /* still hidden in its web, or still a statue */
        if (m->type == MOB_SAC || shade_hidden(m) || m->boss || m->ally) continue;   /* a shade shows only in light; a boss has its card */
        float dx = m->x - G.px, dy = m->y - G.py, d = sqrtf(dx * dx + dy * dy) + 1e-4f;
        if (d > 9.0f || (dx * fx + dy * fy) / d < 0.5f || !line_clear(G.px, G.py, m->x, m->y)) continue;
        G.named[m->type] |= (uint8_t)(1 << m->rank);
        rec_seen(m);
        const char *word = KINDS[m->type].family == PF_CHITIN && m->rank == RK_V ? "PALE " : RANK_WORD[m->rank];
        char first = *word ? *word : KINDS[m->type].name[0];
        say("%s %s%s", KIND_IS(m, KF_UNIQUE) ? "THE" : (strchr("AEIOU", first) ? "AN" : "A"), word, KINDS[m->type].name);
        return;
    }
}

static void fizzle(int sp) {                                      /* chain or drain with nothing to bite on */
    float ex = G.px, ey = G.py;
    for (float d = 0.3f; d < 4.0f; d += 0.1f) {
        float x = G.px + cosf(G.ang) * d, y = G.py + sinf(G.ang) * d;
        if (blocks_light((int)x, (int)y)) break;
        ex = x; ey = y;
    }
    add_beam(sp, G.px + cosf(G.ang) * 0.35f, G.py + sinf(G.ang) * 0.35f, 0.42f, ex, ey, 0.45f);
    snd_sfx_ex(SFX_ZAP, 0.5f, 0.5f);
}

static int open_door(Door *d) {
    if (d->locked == SEALED) { say("IT IS SEALED UNTIL THE BOSS FALLS"); snd_sfx_ex(SFX_KNOCK, 0.6f, 0.5f); return 0; }
    if (d->locked == TRIALED) { if (G.msg_t <= 0.0f) { say("THE TRIAL HOLDS IT SHUT"); sfx_at(SFX_KNOCK, d->x + 0.5f, d->y + 0.5f, 0.7f); } return 0; }
    if (d->locked == GATED) {                                      /* a winch's or a hearth's gate */
        if (G.msg_t <= 0.0f) {
            for (int i = 0; i < G.nmk; i++)
                if (G.mk[i].door == d - G.doors) {
                    if (G.mk[i].kind == MKK_GSEAL) say("%s", G.py < d->y + 0.5f ? "IT OPENS FROM THE OTHER SIDE" : "STRIKE ITS SEAL WITH A SPELL");   /* the tutorial's */
                    else say(G.mk[i].kind == MKK_HEARTH ? "IT RISES WHILE THE HEARTH BURNS" : "%s IS NEAR: IT WANTS LIGHTNING", mk_name(&G.mk[i]));
                    break;
                }
            sfx_at(SFX_KNOCK, d->x + 0.5f, d->y + 0.5f, 0.7f);
        }
        return 0;
    }
    if (d->locked == WINGED) {                                     /* the wing's: only its puzzle opens it */
        if (G.msg_t <= 0.0f) { say("%s", G.pz_exit ? "THE WAY ON LIES BEYOND" : "SEALED, AND NO KEY FITS IT"); sfx_at(SFX_KNOCK, d->x + 0.5f, d->y + 0.5f, 0.7f); }
        return 0;
    }
    if (d->locked == 1) {
        if (!G.has_key[d->color]) { say("THE %s DOOR IS LOCKED", KEY_COLOR[d->color]); return 0; }
        d->locked = 2; G.has_key[d->color] = 0; G.unlock_t = G.level_t; snd_sfx(SFX_UNLOCK); pop_readout();
        if (g_drawn) for (int i = 0; i < G.ndoors; i++) if (G.doors[i].locked == 1 && G.doors[i].color == d->color) G.doors[i].locked = 2;   /* a drawn
                                                                      vault's doors are one lock (docs/PLAN-tutorial.md, T19 a) */
    }
    if (!d->opening && d->open < 0.95f) d->opening = 1;
    return 1;
}

static int pocket_door(int x, int y);
static void human_exit(void) {                                    /* into the runes */
    G.exiting = 1; G.leaving = 1; G.exit_t = 0.0f; G.u = 1.0f; G.pc_t = -1.0f; G.hand = 0.0f;
    G.cx = G.tx = G.exit_fx; G.cy = G.ty = G.exit_fy;
    begin_turn(heading_angle(G.exit_dir)); G.adur = 0.4f;
    G.state = PS_EXIT; G.spd = 0.0f;
}

static int map_gold(void);
static void dim_view(Frame *f);
/* ---- the reckoning before you leave (1.0.0b13, docs/PLAN-b13.md section 10a) ----
 * A person going out by the exit with anything still left on the level is shown what, on a slip pinned to the
 * exit's door in the run's ink, and must choose DESCEND BELOW to go; NOT YET (where it starts) steps back a pace.
 * It counts what the game knows, map or no map; on a boss's level only what lies outside the arena. Not for the
 * dream, nor a challenge's or a replay's way out. */
enum { RK_MOBS, RK_SECRETS, RK_WING, RK_GOLD, RK_TRIAL, NRK };
static int reckon_lines(char out[NRK][48], int glyph[NRK]) {
    int n = 0;
    int mobs = 0;
    for (int i = 0; i < MAXMOBS; i++) {
        const Mob *m = &G.mobs[i];
        if (m->alive && m->state != MS_DIE && !KIND_IS(m, KF_UNCOUNTED) && !m->boss && !m->add && !m->ally && !(G.boss >= 0 && in_arena(m->x, m->y))) mobs++;
    }
    if (mobs) { snprintf(out[n], 48, "%d CREATURE%s STILL WALK%s", mobs, mobs == 1 ? "" : "S", mobs == 1 ? "S" : ""); glyph[n++] = RK_MOBS; }
    if (G.nlinks > G.links_found) { snprintf(out[n], 48, "%d OF %d SECRETS STILL HIDDEN", G.nlinks - G.links_found, G.nlinks); glyph[n++] = RK_SECRETS; }
    if (G.pz >= 0 && !G.pz_solved) { snprintf(out[n], 48, "THE SEALED WING IS STILL SHUT"); glyph[n++] = RK_WING; }
    int gold = map_gold();
    if (gold > 0) {
        char num[16];
        if (gold >= 1000) snprintf(num, sizeof num, "%d,%03d", gold / 1000, gold % 1000); else snprintf(num, sizeof num, "%d", gold);
        snprintf(out[n], 48, "ABOUT %s GOLD STILL LIES HERE", num); glyph[n++] = RK_GOLD;
    }
    if (G.tri >= 0 && G.tri_state == TRS_SHUT) { snprintf(out[n], 48, "THE TRIAL STILL WAITS"); glyph[n++] = RK_TRIAL; }
    else if (G.tri >= 0 && G.tri_state == TRS_WON && tri_reward_at(IT_CHEST) >= 0) { snprintf(out[n], 48, "THE TRIAL'S HOARD STILL LIES IN ITS ROOM"); glyph[n++] = RK_TRIAL; }
    return n;
}
/* E at the exit, or walking into it: the reckoning first, if anything is left */
static void human_leave(void) {
    char l[NRK][48]; int g[NRK];
    if (tu_exit_barred()) return;                                  /* not without the map, while level 2's lessons run */
    if (G.bchallenge || G.replay || PK.depth || !reckon_lines(l, g)) { human_exit(); return; }
    open_ui(UI_RECKON);
    G.ui_sel = 1; G.rk_fresh = 1;                                  /* on NOT YET; the press that opened it is spent */
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT reckon %d lines t=%.2f\n", reckon_lines(l, g), G.t);
#endif
}
static void update_reckon(void) {
    if (G.rk_fresh) { G.rk_fresh = 0; return; }
    if (IN.hit[KEY_W] || IN.hit[KEY_UP] || IN.hit[KEY_S] || IN.hit[KEY_DOWN]) { G.ui_sel ^= 1; snd_sfx_ex(SFX_TICK, 0.6f, 0.5f); }
    int take = IN.hit[KEY_ENTER] || IN.hit[KEY_E] || IN.hit[KEY_SPACE] || IN.hit[KEY_MOUSE1];
    if (IN.hit[KEY_ESC] || (take && G.ui_sel == 1)) {               /* not yet: a pace back from the doorway */
        G.ui = UI_NONE;
        float bx = G.px - DX[G.exit_dir] * 0.6f, by = G.py - DY[G.exit_dir] * 0.6f;
        if (passable((int)bx, (int)by)) { G.px = bx; G.py = by; }
        snd_sfx_ex(SFX_STEP, 0.8f, 0.5f);
#ifdef CRAWL_DEBUG
        fprintf(stderr, "EVENT reckon_stay t=%.2f\n", G.t);
#endif
        return;
    }
    if (!take) return;
    G.ui = UI_NONE;
    snd_sfx_ex(SFX_PAGE, 0.7f, 0.5f);
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT reckon_descend t=%.2f\n", G.t);
#endif
    human_exit();
}
static void reckon_glyph(Frame *f, int x, int y, int k, uint8_t c) {   /* the dead, a crack, a seal, coins, an hourglass */
    static const char *const GL[NRK][7] = {
        { ".###.", "#####", "#.#.#", "#####", ".#.#.", ".###.", "....." },
        { "..#..", "..#..", ".#...", ".##..", "...#.", "..#..", "..#.." },
        { ".###.", "#...#", "#.#.#", "#.#.#", "#...#", ".###.", "..#.." },
        { ".###.", "#...#", ".###.", "#...#", ".###.", "#...#", ".###." },
        { "#####", ".#.#.", "..#..", "..#..", ".#.#.", "#.#.#", "#####" } };
    for (int r = 0; r < 7; r++) for (int q = 0; q < 5; q++) if (GL[k][r][q] == '#') gfx_px(f, x + q, y + r, c);
}
static void render_reckon_in(Frame *f);
static void render_reckon(Frame *f) { UI_PLAIN_BEGIN; ui_box_none(); render_reckon_in(f); UI_PLAIN_END; }   /* a slip of paper, in its own ink */
static void render_reckon_in(Frame *f) {
    char l[NRK][48]; int g[NRK];
    int n = reckon_lines(l, g), run = crawl_set(G.level) & 3, pw = 296, ph = 52 + n * 11 + 26, px = (W - pw) / 2, py = imax(4, (H - 44 - ph) / 2 - 6);
    dim_view(f);
    uint8_t ink = crawl_doc_page(f->pix, W, px, py, pw, ph, run + 4, (unsigned)(G.level * 131 + 9));
    gfx_px(f, px + pw / 2, py + 3, R_IRON + 2); gfx_px(f, px + pw / 2 + 1, py + 3, R_IRON + 10); gfx_px(f, px + pw / 2, py + 4, R_IRON + 6);   /* the nail */
    const char *head = "THE DEAD KEEP COUNT";
    gfx_text8(f, px + (pw - gfx_text_width(head, 1)) / 2, py + 12, head, run == 0 ? (uint8_t)(R_BLOOD + 9) : ink, 1);
    for (int i = 0; i < n; i++) {
        int y = py + 30 + i * 11;
        reckon_glyph(f, px + 14, y, g[i], ink);
        gfx_text8(f, px + 26, y, l[i], ink, 1);
    }
    static const char *const OPT[2] = { "DESCEND BELOW", "NOT YET" };
    for (int k = 0; k < 2; k++) {
        int y = py + 36 + n * 11 + k * 11, w = gfx_text_width(OPT[k], 1), x = px + (pw - w) / 2;
        if (G.ui_sel == k) { gfx_text8(f, x - 12, y, ">", R_BLOOD + 10, 1); gfx_text8(f, x + w + 4, y, "<", R_BLOOD + 10, 1); }
        gfx_text8(f, x, y, OPT[k], G.ui_sel == k ? (uint8_t)(R_BLOOD + 9) : ink, 1);
    }
    crawl_doc_ink(f->pix, W, px, py, pw, ph, run, ink, (unsigned)(G.level * 131 + 9));
    const char *hint = "W/S CHOOSE  ENTER TAKES  ESC NOT YET";
    int hw = hint_width(hint);
    gfx_rect(f, (W - hw) / 2 - 4, py + ph + 2, hw + 8, 11, 0);
    hint8(f, (W - hw) / 2, py + ph + 4, hint, R_STONE + 11);
}

/* walking into something: a door opens (if it can), the exit takes you */
static void human_bump(int cx, int cy) {
    Cell *c = cell(cx, cy);
    if (G.shop) { if (c->kind == K_DOOR) open_door(&G.doors[c->door]); return; }
    if (c->kind == K_DOOR) open_door(&G.doors[c->door]);
    else if (c->kind == K_WALL && c->secret == 3) pocket_door(cx, cy);   /* a secret's doorway: walked through */
    else if (G.exit_fx >= 0 && cx == G.exit_wx && cy == G.exit_wy && (int)G.px == G.exit_fx && (int)G.py == G.exit_fy) {
        if (boss_sealed()) { if (G.msg_t <= 0.0f) { say("THE WAY IS SHUT WHILE IT LIVES"); snd_sfx_ex(SFX_KNOCK, 0.6f, 0.5f); } }
        else human_leave();
    }
}

/* E or Space: the door or exit just ahead */
/* The Backrooms' end: E at the visportal, once a chain, gives your health back in full, or fifty souls
 * if you are whole. It goes on drifting until it has given; then it stands still. */
static int visportal_use(void) {
    int sev = G.ptheme == PT_SEVERANCE;                            /* Severance's water cooler gives the same, as one drink */
    if (PK.vis_used[PK.chain]) { say(sev ? "THE COOLER HAS GIVEN ITS ONE DRINK" : "THE DOORWAYS GO ON, AND GIVE NO MORE"); snd_sfx_ex(SFX_KNOCK, 0.5f, 0.5f); return 1; }
    PK.vis_used[PK.chain] = 1;
    int heal = G.hp < G.max_hp;
    if (heal) { G.hp = G.max_hp; G.poison_t = G.slow_t = 0.0f; say(sev ? "A CUP OF WATER. YOU ARE WHOLE AGAIN" : "THE DOORWAYS MAKE YOU WHOLE AGAIN"); }
    else { soul_add(50); say(sev ? "A CUP OF WATER. FIFTY SOULS" : "THE DOORWAYS GIVE YOU FIFTY SOULS"); }
    if (sev && G.vis_deco >= 0) G.decos[G.vis_deco].spr = SP_COOLER_LOW;
    gain_coin(PK.chain, "AT THE END OF A CHAIN");
    xp_gain(60.0f * xp_level() * (float)imax(1, G_LEVEL.secrets[clampi(PK.chain, 0, MAXSECRETS - 1)].links), "THE TRIAL'S END");
    snd_sfx_ex(sev ? SFX_POTION : SFX_SHIMMER, 1.0f, 0.5f); snd_sfx_ex(sev ? SFX_DRIP : SFX_POTION, 0.7f, 0.5f);
    G.grab_t = 1.0f; show_bar();
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT visportal chain %d %s t=%.2f\n", PK.chain, heal ? "heals" : "souls", G.t);
#endif
    return 1;
}

static int human_use(void) {
    int cx = (int)floorf(G.px + cosf(G.ang) * 0.8f), cy = (int)floorf(G.py + sinf(G.ang) * 0.8f);
    Cell *c = cell(cx, cy);
    if (!G.shop && !PK.depth) {                                    /* a puzzle's things (1.0.0b13) */
        int ti = thing_facing();
        if (ti >= 0) { thing_use(ti); G.grab_t = 0.6f; return 1; }
    }
    if (c->kind == K_DOOR) { open_door(&G.doors[c->door]); return 1; }
    if (c->kind == K_WALL && c->secret == 3 && pocket_door(cx, cy)) return 1;   /* a secret's doorway */
    if (PK.depth && cx == G.vis_x && cy == G.vis_y) return visportal_use();
    if (!PK.depth && altar_callable()) {                           /* the altar's wisp, waiting at a secret you have been into */
        const Wisp *w = &G.wisps[1];
        float d = hypotf(w->x - G.px, w->y - G.py);
        if (w->on && d < 2.2f && fabsf(wrap_pi(atan2f(w->y - G.py, w->x - G.px) - G.ang)) < 0.35f + 0.4f / fmaxf(d, 0.5f)) { altar_call_home(); return 1; }
    }
    if (c->kind == K_WALL && c->secret == 2)                       /* a cracked wall: it gives */
        for (int i = 0; i < G.nsecrets; i++) if (G.secrets[i].sx == cx && G.secrets[i].sy == cy && !G.secrets[i].state) { open_secret(i); G.grab_t = 1.0f; return 1; }
    if (!G.shop && font_facing() >= 0) return font_drink(font_facing());
    if (!G.shop && !PK.depth && rest_facing(1.3f) >= 0) {         /* a place to mend: to kneel there, or to get up */
        if (G.rest_on) { rest_end("let go"); return 1; }
        return rest_begin(rest_facing(1.3f));
    }
    if (!G.shop && G.boss == BOSS_WRAITH && brazier_facing() >= 0) {   /* a second's work to kindle it again */
        G.bbz_at = brazier_facing(); G.bbz_t = 1.0f;
        say("KINDLING THE BRAZIER"); snd_sfx_ex(SFX_IGNITE, 0.5f, 0.5f);
        return 1;
    }
    if (!G.shop && G.exit_fx >= 0 && cx == G.exit_wx && cy == G.exit_wy && hypotf(G.exit_fx + 0.5f - G.px, G.exit_fy + 0.5f - G.py) < 1.0f) {
        if (boss_sealed()) { say("THE WAY IS SHUT WHILE IT LIVES"); snd_sfx_ex(SFX_KNOCK, 0.6f, 0.5f); return 1; }
        human_leave(); return 1;
    }
    if (!G.shop && G.altar_x >= 0 && hypotf(G.altar_x + 0.5f - G.px, G.altar_y + 0.5f - G.py) < 1.6f
        && fabsf(wrap_pi(atan2f(G.altar_y + 0.5f - G.py, G.altar_x + 0.5f - G.px) - G.ang)) < 0.7f) { open_ui(UI_ALTAR); G.grab_t = 1.0f; return 1; }
    if (bshrine_facing()) { open_ui(UI_BSHRINE); G.grab_t = 1.0f; return 1; }   /* the Black Shrine (1.0.0b13) */
    if (G.srch_deco < 0) { int k = hide_facing(); if (k >= 0) { search_begin(k); return 1; } }   /* a thing that may hold the message */
    if (!G.shop && !PK.depth && G.carry && c->kind != K_DOOR) return thing_set_down();   /* nothing to put it in: on the floor */
    return 0;
}

/* what E would do now, in a word, for the hollow cut in touch's left stone (docs/PLAN-mobile.md 6.5): human_use's order,
 * and her rooms' looks; 0 when it would do nothing */
static const char *use_word(void) {
    if (!G.player || G.dead || G.tally || G.exiting) return 0;
    if (G.shop) {
        if ((G.shop_look >= 0 && !G.offer_gone[G.shop_look]) || G.map_look) return "BUY";
        if (G.pt_look) { int p = por_head(G.pt_look - 1); return por_is_drawn(p) ? "LOOK" : por_can_draw(p) ? "DRAW" : 0; }
        if (G.mer_look) return "SPEAK";
        if (G.stone_look >= 0) return "READ";
        if (G.sh_look || G.lamp_look || G.tot_look || G.comp_look || G.rep_look || G.por_look || G.bell_look || G.font_look) return "OPEN";
        return 0;
    }
    int cx = (int)floorf(G.px + cosf(G.ang) * 0.8f), cy = (int)floorf(G.py + sinf(G.ang) * 0.8f);
    const Cell *c = cell(cx, cy);
    if (!PK.depth) {
        int ti = thing_facing();
        if (ti >= 0) {
            static const char *const VERB[] = { "TAKE", "PLACE", "TURN", "PULL", "LIGHT", "STRIKE", "STAND", "READ" };
            int r = G.thing[ti].role;
            return G.carry && r == TR_PLACE ? "PLACE" : r >= 0 && r < (int)(sizeof VERB / sizeof VERB[0]) ? VERB[r] : "TOUCH";
        }
    }
    if (c->kind == K_DOOR) return "OPEN";
    if (c->kind == K_WALL && c->secret == 3) return "ENTER";
    if (PK.depth && cx == G.vis_x && cy == G.vis_y) return G.ptheme == PT_SEVERANCE ? "DRINK" : "TOUCH";
    if (!PK.depth && altar_callable()) {
        const Wisp *w = &G.wisps[1];
        float d = hypotf(w->x - G.px, w->y - G.py);
        if (w->on && d < 2.2f && fabsf(wrap_pi(atan2f(w->y - G.py, w->x - G.px) - G.ang)) < 0.35f + 0.4f / fmaxf(d, 0.5f)) return "CALL";
    }
    if (c->kind == K_WALL && c->secret == 2)
        for (int i = 0; i < G.nsecrets; i++) if (G.secrets[i].sx == cx && G.secrets[i].sy == cy && !G.secrets[i].state) return "PUSH";
    if (font_facing() >= 0) return "DRINK";
    if (!PK.depth && rest_facing(1.3f) >= 0) return G.rest_on ? "RISE" : "KNEEL";
    if (G.boss == BOSS_WRAITH && brazier_facing() >= 0) return "KINDLE";
    if (G.exit_fx >= 0 && cx == G.exit_wx && cy == G.exit_wy && hypotf(G.exit_fx + 0.5f - G.px, G.exit_fy + 0.5f - G.py) < 1.0f) return boss_sealed() ? 0 : "LEAVE";
    if (G.altar_x >= 0 && hypotf(G.altar_x + 0.5f - G.px, G.altar_y + 0.5f - G.py) < 1.6f
        && fabsf(wrap_pi(atan2f(G.altar_y + 0.5f - G.py, G.altar_x + 0.5f - G.px) - G.ang)) < 0.7f) return "PRAY";
    if (bshrine_facing()) return "PRAY";
    if (G.srch_deco < 0 && hide_facing() >= 0) return "SEARCH";
    if (!PK.depth && ward_facing() >= 0) return "LIGHT";
    if (!PK.depth && G.carry && c->kind != K_DOOR) return "DROP";
    return 0;
}

static int body_blocked(float x, float y, int *bx, int *by) {
    float xs[2] = { x - BODY, x + BODY }, ys[2] = { y - BODY, y + BODY };
    for (int i = 0; i < 2; i++)
        for (int j = 0; j < 2; j++) {
            int cx = (int)floorf(xs[i]), cy = (int)floorf(ys[j]);
            if (foot_of(cx, cy) ? foot_hit(xs[i], ys[j]) : blocks_move(cx, cy)) { *bx = cx; *by = cy; return 1; }   /* a prop's part,
                                                                      as against walls: the box is no wider than a quarter (1.0.0b18.1) */
        }
    return 0;
}

static void push_from(float ox, float oy, float r) {              /* out of something round in the way */
    float vx = G.px - ox, vy = G.py - oy, d = hypotf(vx, vy);
    int bx, by;
    if (d >= r || d < 1e-4f) return;
    float nx = G.px + vx / d * (r - d), ny = G.py + vy / d * (r - d);
    if (!body_blocked(nx, ny, &bx, &by)) { G.px = nx; G.py = ny; }
}

/* 1.0.0b18.1: the body caught in something solid (a door that shut on it, whatever else might close on it): every step
 * would be refused, so it is set where it fits, the nearest place within a cell. Walls are a cell thick, so it can't come
 * out on their far side. */
static void unstick(void) {
    int bx, by;
    if (!body_blocked(G.px, G.py, &bx, &by)) return;
    for (int r = 1; r <= 20; r++) {                                /* rings a twentieth of a cell apart */
        float rr = 0.05f * (float)r;
        for (int k = 0; k < 16; k++) {
            float a = (float)k * (TAU_F / 16.0f), x = G.px + cosf(a) * rr, y = G.py + sinf(a) * rr;
            int ox, oy;
            if (body_blocked(x, y, &ox, &oy)) continue;
#ifdef CRAWL_DEBUG
            fprintf(stderr, "EVENT unstuck from (%.2f,%.2f) in (%d,%d) kind %d to (%.2f,%.2f) t=%.2f\n", G.px, G.py, bx, by, cell(bx, by)->kind, x, y, G.t);
#endif
            G.px = x; G.py = y;
            return;
        }
    }
}

/* one step of walking, sliding along walls; returns how far it got */
static float human_move(float dx, float dy) {
    unstick();
    float ox = G.px, oy = G.py;
    int bx, by;
    if (dx != 0.0f) { if (!body_blocked(G.px + dx, G.py, &bx, &by)) G.px += dx; else human_bump(bx, by); }
    if (dy != 0.0f) { if (!body_blocked(G.px, G.py + dy, &bx, &by)) G.py += dy; else human_bump(bx, by); }
    for (int i = 0; i < MAXMOBS; i++) {
        const Mob *m = &G.mobs[i];
        if (solid_mob(m) && m->state == MS_WALK && m->type != MOB_BAT && !m->ally && G.wwalk_t <= 0.0f) push_from(m->x, m->y, 0.5f);   /* yours, and
                                                                                       wraithwalking all of them, you go through */
    }
    if (G.shop) {                                                  /* the pedestals and the merchant's chest */
        for (int i = 0; i < G.ped_n; i++) push_from(G.ped_x[i], G.ped_y[i], 0.5f);
        push_from(CAT_X, CAT_Y, 0.75f);
    }
    return hypotf(G.px - ox, G.py - oy);
}

/* what a cast would bite on: the nearest creature close to the middle of the view */
/* The target's health (docs/PLAN-b9.md, section 2): the creature nearest the middle of the view, in sight
 * and within twelve cells, stays the target a second and a half after you look away. Spirits count (their
 * readout says nothing touches them); a boss has its own bar. */
static int look_at(void) {
    float dx = cosf(G.ang), dy = sinf(G.ang);
    int best = -1; float bs = 1e9f;
    for (int i = 0; i < MAXMOBS; i++) {
        const Mob *m = &G.mobs[i];
        int spirit = m->alive && m->state == MS_WALK && KIND_IS(m, KF_SPIRIT) && !m->boss && !m->ally;
        if (!spirit && (!targetable(m) || m->boss)) continue;
        if (m->type == MOB_SAC || m->small) continue;
        float vx = m->x - G.px, vy = m->y - G.py, depth = vx * dx + vy * dy, lat = fabsf(-vx * dy + vy * dx);
        if (depth < 0.3f || depth > 12.0f || lat > depth * 0.07f + 0.35f || !line_clear(G.px, G.py, m->x, m->y)) continue;
        float sc = depth + lat * 6.0f;
        if (sc < bs) { bs = sc; best = i; }
    }
    return best;
}
static void look_update(float dt) {
    int i = look_at();
    if (i >= 0) { G.look_mob = i; G.look_t = fmaxf(G.look_t, 1.5f); return; }
    G.look_t = fmaxf(0.0f, G.look_t - dt);
    const Mob *m = &G.mobs[clampi(G.look_mob, 0, MAXMOBS - 1)];
    if (!m->alive || m->ally) G.look_t = 0.0f;
    else if (m->state != MS_WALK) G.look_t = fminf(G.look_t, 0.6f);   /* fallen: the bar empties, and goes */
}

static int aim_target(void) {
    float dx = cosf(G.ang), dy = sinf(G.ang);
    int best = -1; float bs = 1e9f;
    for (int i = 0; i < MAXMOBS; i++) {
        const Mob *m = &G.mobs[i];
        if (!targetable(m)) continue;
        float vx = m->x - G.px, vy = m->y - G.py, depth = vx * dx + vy * dy, lat = fabsf(-vx * dy + vy * dx);
        if (depth < 0.3f || depth > 10.0f || lat > depth * 0.12f + 0.3f || !line_clear(G.px, G.py, m->x, m->y)) continue;
        float sc = depth + lat * 4.0f;
        if (sc < bs) { bs = sc; best = i; }
    }
    return best;
}

/* ---- Restoration (1.0.0b20, PLAN-combat 10.3; the dead's track's, D5) ----
 * Once Restoration's rank I is confessed it is a spell among the others: 8, the wheel and the spells' wheel put it in hand
 * (G.rst_sel; G.spell stays what it was, for when it is put down). Held, the cast channels it after a breath (RS_WIND): souls
 * by the second, health by the second (a share of it, Restoration's numbers with the class's weight), walking at half pace
 * (slow_factor). A blow doesn't end it; a stun, a knock-down, Unbound or the void do, and so do letting go, a menu and the
 * souls running out. Drawn as a pale glow down the arm (render_restore). The dream channels it out of a fight, hurt, with
 * souls to spare (dream_restore). It casts nothing, so nothing the dead do hears it as a spell */
#define RS_ID 100                                                  /* its place among the spells' ids, past every AT_ and SHOT_ */
#define RS_WIND 0.4f
static const char *const RESTORE_MARK[11] = { "....###....", "...#...#...", "...#...#...", "....#.#....", ".####o####.", ".....#.....",
                                              ".....#.....", ".....o.....", ".....#.....", ".....#.....", "..........." };   /* an ankh */
static int restore_open(void) { return conf_value(CF_RESTORE, CV_HEAL) > 0.0f; }
static int restore_breaks(void) { return G.dead || no_cast() || af_on(AF_STUN) || af_on(AF_DOWN) || af_on(AF_UNBOUND); }
static void restore_stop(const char *why) {
    if (G.rst_on) {
#ifdef CRAWL_DEBUG
        fprintf(stderr, "EVENT restore_end %s healed %d souls %.1f t=%.2f %s\n", why, G.rst_healed, G.rst_spent, G.t, G.player ? "player" : "autopilot");
#else
        (void)why;
#endif
    }
    G.rst_on = 0; G.rst_t = 0.0f; G.rst_acc = 0.0f; G.rst_healed = 0; G.rst_spent = 0.0f;
}
static void restore_begin(void) {
    G.rst_on = 1; G.rst_healed = 0; G.rst_spent = 0.0f; G.rst_snd = 0.0f;
    snd_sfx_ex(SFX_SHIMMER, 0.55f, 0.5f);
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT restore_begin hp %d/%d souls %.1f t=%.2f %s\n", G.hp, G.max_hp, spirit(), G.t, G.player ? "player" : "autopilot");
#endif
}
static void restore_tick(float dt) {                               /* a channel's second: souls in, health out */
    if (G.hp >= G.max_hp) { G.rst_acc = 0.0f; return; }             /* whole: it costs nothing to hold */
    float cost = conf_value(CF_RESTORE, CV_HEAL_SOULS) * dt;
    if (!spend_spirit(cost)) {
        if (G.player) say("NO SOULS LEFT TO SPEND");
        restore_stop("souls");
        return;
    }
    G.rst_spent += cost;
    G.rst_acc += conf_value(CF_RESTORE, CV_HEAL) * (float)G.max_hp * dt;
    while (G.rst_acc >= 1.0f && G.hp < G.max_hp) { G.rst_acc -= 1.0f; G.hp++; G.rst_healed++; }
    if ((G.rst_snd -= dt) <= 0.0f) { G.rst_snd = 1.6f; snd_sfx_ex(SFX_SHIMMER, 0.3f, 0.5f); }   /* a breath of it now and then */
}
static void restore_glow_step(float dt) { G.rst_k += ((G.rst_on ? 1.0f : 0.0f) - G.rst_k) * fminf(1.0f, dt * 6.0f); }
static int restore_cast(float dt, int trig, int may) {              /* a person's, from human_cast: 1 while it has the cast button */
    if (G.rst_sel && !restore_open()) G.rst_sel = 0;                 /* a death took the Confessions */
    if (!G.rst_sel) { if (G.rst_on) restore_stop("put down"); restore_glow_step(dt); return 0; }
    if (!may || !trig || restore_breaks()) restore_stop(!may ? "menu" : !trig ? "let go" : "broken");
    else if ((G.rst_t += dt) >= RS_WIND) {                           /* a breath, and it flows */
        if (!G.rst_on) restore_begin();
        restore_tick(dt);
    }
    restore_glow_step(dt);
    return 1;
}
static void dream_restore(float dt) {                              /* the dream's, from update_player: out of a fight, hurt, souls to spare */
    if (G.player) return;
    int may = restore_open() && !restore_breaks() && !G.shop && !G.tally && !in_combat() && G.state != PS_CAST && G.state != PS_AIM;
    if (G.rst_on && (!may || G.hp >= (int)(0.9f * (float)G.max_hp) || spirit() < 4.0f)) restore_stop(!may ? "fight" : "enough");
    else if (!G.rst_on && may && G.hp < (int)(0.6f * (float)G.max_hp)) {   /* souls enough to mend a fifth of its health, and six over */
        float heal = conf_value(CF_RESTORE, CV_HEAL), need = heal > 0.0f ? 0.2f / heal * conf_value(CF_RESTORE, CV_HEAL_SOULS) : 1e9f;
        if (spirit() >= 6.0f + need) { G.rst_t = RS_WIND; restore_begin(); }
    }
    if (G.rst_on) restore_tick(dt);
    restore_glow_step(dt);
}
static void pick_restore(void) {
    if (!restore_open()) { say("RESTORATION IS NOT YET CONFESSED"); return; }
    if (!G.rst_sel) snd_sfx_ex(SFX_TICK, 0.8f, 0.5f);
    G.rst_sel = 1;
    show_bar();
    say("RESTORATION  RANK %d", imax(1, conf_rank(CF_RESTORE)));
}
static void restore_hud(Frame *f, int x, int w, int y0) {          /* the status bar's spell, while it is in hand */
    static const char *const ANKH[7] = { ".###.", "#...#", ".#.#.", "#####", "..#..", "..#..", "..#.." };
    const char *nm = "RESTORE";                                    /* as wide as the lightning's name and glyph, which the box allows */
    if (gfx_text_width(nm, 1) + 7 > w + 18) nm = "HEAL";
    int nw = gfx_text_width(nm, 1) + 7, nx = x + (w - nw) / 2;
    for (int r = 0; r < 7; r++) for (int c = 0; c < 5; c++) if (ANKH[r][c] == '#') gfx_px(f, nx + c, y0 + 19 + r, ramp_at(R_GHOST, 13));
    gfx_text8(f, nx + 7, y0 + 19, nm, ramp_at(R_GHOST, G.rst_on ? 15 : 12), 1);
    char num[16];
    snprintf(num, sizeof num, "RANK %d", imax(1, conf_rank(CF_RESTORE)));
    gfx_text8(f, x + (w - gfx_text_width(num, 1)) / 2, y0 + 30, num, R_IRON + 13, 1);
}

static const int KEY_SPELL[7] = { AT_FIRE, AT_FROST, AT_CHAIN, AT_FLASK, AT_DRAIN, AT_SPEAR, AT_BFS };   /* 1-7 */
static void pick_spell(int sp) {
    if (sp == RS_ID) { pick_restore(); return; }                   /* Restoration (D5) */
    if (sp < 0 || sp >= NATK) return;
    if (RUN.gear.atk[sp] < 0) {
        if (sp == AT_BFS) say("NOT IN YOUR SPELLBOOK");
        else say("%s IS NOT YET OPEN: THE LAMPSTAND", ELEM_NAME[ATK_ELEM[sp]]);
        return;
    }
    if (sp != G.spell || G.rst_sel) snd_sfx_ex(SFX_TICK, 0.8f, 0.5f);
    G.spell = sp; G.rst_sel = 0;                                    /* Restoration put down, if it was in hand */
    show_bar();
    if (sp == AT_BFS) say("THE BFS9000");
    else say("%s  RANK %d", ATK_NAME[sp], RUN.gear.atk[sp]);
}

/* ---- the wheels (1.0.0b16) ---- */
static void wheel_fill(int kind) {
    WH.n = 0; WH.hi = -1;
    if (kind == WH_SPELLS) {                                       /* the six lines in their places round it, top first, the BFS9000 a seventh */
        int nb = RUN.gear.atk[AT_BFS] >= 0 ? 7 : 6, n = nb + restore_open();   /* and Restoration last, once confessed (D5) */
        for (int i = 0; i < n; i++) {
            int sp = i < nb ? KEY_SPELL[i] : RS_ID;
            WH.s[WH.n] = (WSlot){ WS_SPELL, sp, sp == RS_ID || RUN.gear.atk[sp] >= 0, -PI_F * 0.5f + (float)i * 2.0f * PI_F / (float)n };
            if (sp == RS_ID ? G.rst_sel : sp == G.spell && !G.rst_sel) WH.hi = WH.n;
            WH.n++;
        }
        return;
    }
    int nr = 0, na = 0, rk[NDEF], ab[NBOSS];                       /* the runes held down the right half, the gifts down the left */
    for (int k = 0; k < NDEF; k++) if (RUN.gear.def[k] > 0) rk[nr++] = k;
    for (int b = BOSS_SPIDER; b < NBOSS; b++) if (ab_on_mouse(b)) ab[na++] = b;
    int rr = rune_readied(), ar = ab_readied();
    for (int i = 0; i < nr && WH.n < WHEEL_MAX; i++) {
        WH.s[WH.n] = (WSlot){ WS_RUNE, rk[i], rune_by_hand(rk[i]), -PI_F * 0.5f + PI_F * (float)(i + 1) / (float)(nr + 1) };
        if (rk[i] == rr && (RUN.mouse_rune || ar < 0)) WH.hi = WH.n;
        WH.n++;
    }
    for (int i = 0; i < na && WH.n < WHEEL_MAX; i++) {
        WH.s[WH.n] = (WSlot){ WS_GIFT, ab[i], 1, -PI_F * 0.5f - PI_F * (float)(i + 1) / (float)(na + 1) };
        if (ab[i] == ar && WH.hi < 0) WH.hi = WH.n;
        WH.n++;
    }
}
static void wheel_open(int kind, int src) {
    if (G.silenced && kind == WH_GIFTS) { say("THE VOID TAKES THE WORDS FROM YOU"); return; }
    wheel_fill(kind);
    WH.open = kind; WH.src = src; WH.t = 0.0f; WH.mx = WH.my = 0.0f;
    G.map_on = 0;
    snd_sfx_ex(SFX_WHOOSH, 0.35f, 0.5f); snd_sfx_ex(SFX_TICK, 0.5f, 0.5f);
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT wheel_open %s slots %d by %s t=%.2f\n", kind == WH_SPELLS ? "spells" : "gifts", WH.n, src ? "keys" : "pad", G.t);
#endif
}
static void wheel_close(int take) {
    if (!WH.open) return;
    if (take && WH.hi >= 0 && WH.hi < WH.n && WH.s[WH.hi].ok) {
        const WSlot *w = &WH.s[WH.hi];
        if (w->kind == WS_SPELL) pick_spell(w->id);
        else if (w->kind == WS_RUNE) { RUN.rune_sel = w->id; RUN.mouse_rune = 1; say("READIED: %s", DEF_NAME[w->id]); snd_sfx_ex(SFX_TICK, 0.7f, 0.5f); }
        else { RUN.abil_sel = w->id; RUN.mouse_rune = 0; say("READIED: %s", BA_NAME[w->id]); snd_sfx_ex(SFX_TICK, 0.7f, 0.5f); }
        show_bar();
#ifdef CRAWL_DEBUG
        fprintf(stderr, "EVENT wheel_take %d %d t=%.2f\n", w->kind, w->id, G.t);
#endif
    }
    WH.open = 0;
}
/* a ware held for the next visit, for a quarter of its price: Q tapped while looking at it (on letting go since 1.0.0b18.1,
 * so that Q held may open the gifts' wheel in her rooms as anywhere) */
static void shop_say(const char *fmt, ...);
static void shop_hold(int i) {
    const Offer *o = &G.offers[i];
    int cost = (o->price + 3) / 4;
    if (o->price <= 0) shop_say("IT IS FREE: NOTHING TO HOLD");
    else if (G.shop_held >= 0) shop_say(G.shop_held == i ? "IT IS HELD FOR YOU" : "ONE WARE HELD AT A TIME");
    else if (cost > RUN.treasure) shop_say("HOLDING IT COSTS $%d", cost);
    else {
        RUN.treasure -= cost;
        RUN.gear.held = *o; RUN.gear.has_held = 1; G.shop_held = i;
        shop_say("HELD FOR NEXT TIME: $%d", cost);
        snd_sfx_ex(SFX_COIN, 0.8f, 0.5f); sfx_at(SFX_TRILL, CAT_X, CAT_Y, 0.9f); G.cat_slow = 1.2f;
    }
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT shop_hold %d price %d held %d left %d t=%.2f\n", i, o->price, G.shop_held, RUN.treasure, G.t);
#endif
}
static void wheel_input(float dt) {
    if (IN.down[KEY_Q]) {                                          /* Q: tapped, the next readied; held, the gifts' wheel */
        if ((WH.q_t += dt) >= WHEEL_HOLD && !WH.q_w) { WH.q_w = 1; if (!WH.open) wheel_open(WH_GIFTS, 1); }
    } else if (WH.q_t > 0.0f) {
        if (WH.q_w) { if (WH.open && WH.src) wheel_close(1); }
        else if (!WH.open) {
            if (G.shop && G.shop_look >= 0 && !G.offer_gone[G.shop_look]) shop_hold(G.shop_look);   /* at her pedestals: held */
            else ready_next();
        }
        WH.q_t = 0.0f; WH.q_w = 0;
    }
    if (PADX.wheel_req && !WH.open) wheel_open(PADX.wheel_req, TCH.wheel ? 2 : 0);   /* by touch it stays till a tap (src 2) */
    TCH.wheel = 0;
    if (!WH.open) return;
    if (!WH.src && PADX.wheel_rel == WH.open) { wheel_close(1); return; }
    if (PADX.wheel_cancel || IN.hit[KEY_ESC]) { IN.hit[KEY_ESC] = 0; wheel_close(0); snd_sfx_ex(SFX_KEY, 0.5f, 0.5f); return; }
    WH.t += dt;
    float vx = IN.lx, vy = -IN.ly;                                 /* the stick, y down as the screen is: the free hand's (1.0.0b18.1): */
    if (IN.pad) {                                                  /* a D-pad held, the right; X held, the left; a shoulder, either */
        int hand = WH.src ? PADX.q_hand : PADX.w_hand;
        int right = hand == 1 || (hand == 0 && PADX.prx * PADX.prx + PADX.pry * PADX.pry > PADX.plx * PADX.plx + PADX.ply * PADX.ply);
        vx = right ? PADX.prx : PADX.plx; vy = -(right ? PADX.pry : PADX.ply);
    }
    if (WH.src || IN.mdx != 0.0f || IN.mdy != 0.0f) {              /* or the mouse, gathered into a point */
        WH.mx += IN.mdx; WH.my += IN.mdy;
        float r = hypotf(WH.mx, WH.my);
        if (r > 60.0f) { WH.mx *= 60.0f / r; WH.my *= 60.0f / r; }
        if (hypotf(WH.mx, WH.my) > hypotf(vx, vy) * 60.0f) { vx = WH.mx / 60.0f; vy = WH.my / 60.0f; }
    }
    if (hypotf(vx, vy) < 0.5f || WH.n <= 0) return;
    float a = atan2f(vy, vx), bd = 1e9f;
    int best = -1;
    for (int i = 0; i < WH.n; i++) { float d = fabsf(wrap_pi(a - WH.s[i].ang)); if (d < bd) { bd = d; best = i; } }
    if (best != WH.hi) { WH.hi = best; snd_sfx_ex(SFX_TICK, WH.s[best].ok ? 0.6f : 0.3f, 0.5f); }
}

static void spell_keys(void) {
    for (int k = 0; k < 7; k++) if (IN.hit[KEY_1 + k]) pick_spell(KEY_SPELL[k]);
    if (IN.hit[KEY_8]) pick_restore();                             /* 8: Restoration (D5) */
    if (IN.wheel) {                                                /* the wheel turns the pages, past what isn't open; Restoration an eighth */
        int dir = IN.wheel > 0 ? -1 : 1, at = 0, m = 7 + restore_open();
        for (int k = 0; k < 7; k++) if (KEY_SPELL[k] == G.spell) at = k;
        if (G.rst_sel) at = 7;
        for (int n = 0; n < m; n++) { at = (at + dir + m) % m; if (at == 7 || RUN.gear.atk[KEY_SPELL[at]] >= 0) break; }
        pick_spell(at == 7 ? RS_ID : KEY_SPELL[at]);
    }
}

/* the sprint (docs/PLAN-ai.md, 24): two and a half seconds and what SPRINT adds; once it stops, a wait and then the
 * whole meter fills, both shorter by a quarter for each whole hundred of health past 200, to a quarter second and
 * three quarters (the owner's SP2 b) */
static float sprint_max(void) { return SPRINT_BASE + gear_total(&RUN.gear, ST_SPRINT); }
static void sprint_recovery(float *wait, float *fill) {
    int n = G.max_hp >= 300 ? imin((G.max_hp - 200) / 100, 40) : 0;
    float k = powf(0.75f, (float)n);
    *wait = fmaxf(0.25f, 1.5f * k); *fill = fmaxf(0.75f, 3.5f * k);
    float q = 1.0f + (conf_on() ? conf_value(CF_CONST, CV_RECOVER) : 0.0f);   /* Constitution: quicker */
    *wait /= q; *fill /= q;
    if (G.af_n > 0) { *wait /= 0.4f; *fill /= 0.4f; }              /* Chilled: it comes back at two fifths the pace (S4) */
}
static void sprint_tick(float dt, int on, int moved) {
    float mx = sprint_max(), wait, fill;
    sprint_recovery(&wait, &fill);
    if (!G.spr_set) { G.spr = mx; G.spr_set = 1; }
    if (on && moved) {
        if ((G.spr -= dt) <= 0.0f) {                                /* run dry: a walk until it is asked for again */
            G.spr = 0.0f; G.spr_dry = 1; PADX.run = 0; on = 0;
            snd_sfx_ex(SFX_BREATH, 0.5f, 0.5f);
#ifdef CRAWL_DEBUG
            fprintf(stderr, "EVENT sprint_dry max %.2f wait %.2f fill %.2f hp %d t=%.2f\n", mx, wait, fill, G.max_hp, G.t);
#endif
        }
    }
    if (G.spr_on && !on) G.spr_wait = wait;                        /* it stopped: the wait begins */
    if (!on) {                                                     /* held against a wall it neither drains nor fills */
        if (G.spr_wait > 0.0f) G.spr_wait -= dt;
        else if (G.spr < mx) {
            G.spr = fminf(mx, G.spr + mx / fill * dt);
#ifdef CRAWL_DEBUG
            if (G.spr >= mx) fprintf(stderr, "EVENT sprint_full t=%.2f\n", G.t);
#endif
        }
    }
#ifdef CRAWL_DEBUG
    if (on && !G.spr_on) fprintf(stderr, "EVENT sprint_on left %.2f t=%.2f\n", G.spr, G.t);
#endif
    G.spr_on = on;                                                 /* asked for and able, moving or not */
    G.spr_show = G.spr < mx - 1e-3f ? 0.6f : fmaxf(0.0f, G.spr_show - dt);   /* the meter lingers a moment once full */
}

/* turning and walking, shared by the crypt and the merchant's room */
static float pad_turn(float dt) { return IN.rx * (0.4f + 0.6f * IN.rx * IN.rx) * PAD_TURN * (float)PL.turn * 0.1f * dt; }   /* gentle near the middle; a person's turning */
static void human_walk(float dt) {
    int want = IN.down[KEY_SHIFT] || PADX.run;
    if (!IN.down[KEY_SHIFT]) G.spr_dry = 0;                        /* Shift let go: it may be asked for again */
    float turn = want ? 1.5f : 1.0f;                               /* the arrows turn quicker with it, for nothing */
    int ar = !G.map_on;                                            /* the arrows pan the map while it is up */
    G.ang = wrap_pi(G.ang + IN.mdx * MOUSE_TURN + ((float)(ar && IN.down[KEY_RIGHT]) - (float)(ar && IN.down[KEY_LEFT])) * KEY_TURN * turn * dt + pad_turn(dt));
    float fwd = (float)(IN.down[KEY_W] || (ar && IN.down[KEY_UP])) - (float)(IN.down[KEY_S] || (ar && IN.down[KEY_DOWN])) + IN.ly;
    float side = (float)IN.down[KEY_D] - (float)IN.down[KEY_A] + IN.lx;
    int on = want && !G.spr_dry && G.spr > 0.0f && fwd > 0.33f && fwd >= fabsf(side) && !af_on(AF_STUN);   /* a sprint goes forward; stunned, a walk */
    float run = on ? 1.5f : 1.0f;
    float len = sqrtf(fwd * fwd + side * side), moved = 0.0f;
    if (len > 0.0f) {                                              /* the stick part way over: slower */
        float sp = WALK * run * gear_haste(&RUN.gear) * slow_factor() * dt / fmaxf(len, 1.0f);
        float c = cosf(G.ang), s = sinf(G.ang);
        moved = human_move((c * fwd - s * side) * sp, (s * fwd + c * side) * sp);
    }
    sprint_tick(dt, on, moved > 1e-4f);
    G.spd = dt > 0.0f ? moved / dt : 0.0f;
    float ph0 = G.bobph;
    G.bobph += moved * TAU_F;
    if (floorf(ph0 / PI_F) != floorf(G.bobph / PI_F)) footstep();
    G.camz = 0.5f + 0.014f * clampf(G.spd / CRUISE, 0.0f, 1.0f) * sinf(G.bobph);
    if (G.rest_on) G.camz -= 0.1f;                                 /* kneeling at a place to mend */
    int ncx = (int)G.px, ncy = (int)G.py;
    if (ncx != G.cx || ncy != G.cy) {
        G.cx = G.tx = ncx; G.cy = G.ty = ncy; G.u = 1.0f;
        if (!G.shop && G.map[ncy][ncx].visits < 250) { if (G.map[ncy][ncx].visits == 0) G.explored++; G.map[ncy][ncx].visits++; }
    }
}

/* a person's dart: the way set when it began, the mouse still turning */
static void dash_step(float dt) {
    float k = fminf(dt, G.dash_t);
    G.ang = wrap_pi(G.ang + IN.mdx * MOUSE_TURN + pad_turn(dt));
    G.px += G.dash_vx * k; G.py += G.dash_vy * k;
    if ((G.dash_t -= dt) < 0.0f) G.dash_t = 0.0f;
    G.spd = 0.0f;
    int ncx = (int)G.px, ncy = (int)G.py;
    if (ncx != G.cx || ncy != G.cy) {
        G.cx = G.tx = ncx; G.cy = G.ty = ncy; G.u = 1.0f;
        if (G.map[ncy][ncx].visits < 250) { if (G.map[ncy][ncx].visits == 0) G.explored++; G.map[ncy][ncx].visits++; }
    }
}

static void best_default_rank(void);
static void update_bestiary(void);
static int toast_showing(void);
static void open_ui(int which) {
    G.ui = which; G.map_on = 0;                                    /* another screen takes the map's place */
    G.al_aug = 0;                                                  /* the Lampstand opens on its tree, not a pair (1.0.0b21) */
    show_bar();                                                    /* and it is there when the menu closes */
    if (which == UI_INVENTORY) { G.ui_sel = 0; G.inv_page = 0; G.inv0 = RUN.gear; snd_sfx_ex(SFX_KEY, 0.8f, 0.5f); }
    else if (which == UI_BESTIARY) { G.best_feat = 0; best_default_rank(); snd_sfx_ex(SFX_KEY, 0.8f, 0.5f); }
    else if (which == UI_ALTAR) {                                  /* on the first gift it can give you now */
        AltarRow r[3];
        altar_rows(r);
        G.ui_sel = 0;
        for (int i = 2; i >= 0; i--) if (r[i].ok && spirit() + 1e-4f >= (float)r[i].price) G.ui_sel = i;
        snd_sfx_ex(SFX_WHISPER, 0.5f, 0.5f);
    }
    else if (which == UI_BSHRINE) { G.bsh_arm = 0; snd_sfx_ex(SFX_WHISPER, 0.5f, 0.5f); }
    else if (which == UI_RECKON) { G.ui_sel = 1; snd_sfx_ex(SFX_PAGE, 0.6f, 0.5f); }   /* on NOT YET */
    else if (which == UI_SHRINE) {                                 /* on a pull, with what coins you have */
        G.sh_mode = SM_MENU; G.ui_sel = 0; G.sh_arm = -1;
        G.sh_coins = clampi(RUN.coins, 1, 3);
        if (G.sh_fav && !(RUN.attuned >> fav_kind(G.sh_fav) & 1)) G.sh_fav = 0;
        if (RUN.shrine) for (int k = 0; k < NDEF; k++) shrine_attune(k, RUN.gear.def[k]);   /* what you hold at Rare, it knows */
        snd_sfx_ex(SFX_WHISPER, 0.6f, 0.5f);
    }
    else if (which == UI_TOTEM) {                                  /* on the first rank it can raise now */
        int r[NBOSS], n = totem_rows(r);
        G.ui_sel = 0; G.tot_arm = -1; G.tot_col = 0; G.sh_note_t = 0.0f;
        for (int i = n - 1; i >= 0; i--) if (RUN.abil[r[i]] < ABIL_TOP && RUN.scrip[r[i]] > RUN.abil[r[i]]) G.ui_sel = i;
        snd_sfx_ex(SFX_WHISPER, 0.6f, 0.5f);
    }
    else if (which == UI_PORTAL) {                                 /* on the run you are in */
        PortRow r[16];
        int n = portal_rows(r);
        G.ui_sel = 0;
        for (int i = 0; i < n; i++) if (r[i].run == crawl_run(G.level)) G.ui_sel = i;
        snd_sfx_ex(SFX_WHISPER, 0.7f, 0.5f); snd_sfx_ex(SFX_SHIMMER, 0.4f, 0.5f);
    }
    else if (which == UI_CODEX) { snd_sfx_ex(SFX_KEY, 0.8f, 0.5f); snd_sfx_ex(SFX_WHISPER, 0.3f, 0.5f); }
    else if (which == UI_STONES) { G.cs_arm = -1; snd_sfx_ex(SFX_WHISPER, 0.4f, 0.5f); }
    else if (which == UI_ALIGN) {
        G.al_arm = -1; G.al_node = ND_OPEN; snd_sfx_ex(SFX_WHISPER, 0.5f, 0.5f); snd_sfx_ex(SFX_SHIMMER, 0.35f, 0.5f);
        if (gift_due()) for (int e = 0; e < NELEM; e++) if (!align_open(e)) { G.al_el = e; break; }   /* a gift: the first line it may be */
    }
    else if (which == UI_HALL2) { G.ui_sel = 0; G.h2_arm = -1; snd_sfx_ex(SFX_KEY, 0.7f, 0.5f); }
    else if (which == UI_TUTOR) { }                                /* a lesson's page: its own sound, in tu_show */
    else if (which == UI_CLASS) { G.vow_sel = cls_el() >= 0 ? (cls_el() + 1) % NELEM : 0; G.vow_arm = -1; snd_sfx_ex(SFX_WHISPER, 0.6f, 0.5f); }
    else if (which == UI_ASCEND) {
        G.ui_sel = 0; G.asc_arm = -1; memset(G.asc_buy, 0, sizeof G.asc_buy); snd_sfx_ex(SFX_WHISPER, 0.5f, 0.5f);
        G.cf_arm = -1; memset(G.cf_buy, 0, sizeof G.cf_buy); G.cf_rep0 = -1.0f; G.cf_repn = 0;
        int murmur = 0;
        for (int l = 0; l < NLINE; l++) murmur |= RUN.over[l] != 0;
        for (int cf = 0; cf < NCONF; cf++) murmur |= RUN.conf_over[cf] != 0;
        if (murmur) snd_sfx_ex(SFX_WHISPER, 0.3f, 0.2f);           /* the overlevelled: the dead murmur */
    }
    else if (which == UI_REPO) { G.ui_sel = 0; G.rep_mode = 0; G.sh_note_t = 0.0f; snd_sfx_ex(SFX_UNLOCK, 0.6f, 0.5f); snd_sfx_ex(SFX_CREAK, 0.5f, 0.5f); }
    else if (which == UI_COMP) { G.ui_sel = 0; G.comp_arm = -1; G.sh_note_t = 0.0f; snd_sfx_ex(SFX_KEY, 0.7f, 0.5f); snd_sfx_ex(SFX_WHISPER, 0.4f, 0.5f); }
    else if (which == UI_STONE) { G.ui_sel = 0; snd_sfx_ex(SFX_WHISPER, 0.5f, 0.5f); snd_sfx_ex(SFX_WIND, 0.4f, 0.5f); }
    else if (which == UI_MERCHANT) {                               /* on the ledger */
        G.ui_sel = MM_SIGN; sfx_at(SFX_TRILL, CAT_X, CAT_Y, 0.8f); G.cat_trill = 0.4f; G.cat_slow = 1.0f;
    }
    else if (which == UI_WISPS) {                                  /* on the wisp L last cast */
        WispChoice c[WISP_CHOICES];
        int n = wisp_choices(c), i = wisp_choice_at(c, n, RUN.wisp_kind, RUN.wisp_color);
        G.ui_sel = i < 0 ? 0 : i; snd_sfx_ex(SFX_SHIMMER, 0.5f, 0.5f);
    }
    else { G.ui_sel = 1; G.quit_arm = 0; G.quit_line++; snd_sfx_ex(SFX_KNOCK, 0.6f, 0.5f); }
}

/* the hand and the spell in it, the way a person casts: click (or Ctrl) and it goes where you look */
static int spell_holds(int sp);
static int spell_charges(int sp);
static int spell_tethers(int sp);
static void tether_hold(float dt);
static void tether_end(void);
static void cast_charged(int sp, int t, float k);
static void human_cast(float dt, int may) {
    G.cooldown -= dt;
    if (G.spell < 0 || G.spell >= NATK || RUN.gear.atk[G.spell] < 0) G.spell = own_spell();   /* a line gone (a new past after a death) */
    int trig = may && (IN.down[KEY_MOUSE1] || (IN.down[KEY_CAST] && !(WEB && G.map_on)));   /* the map keeps the browser's Space */
    if (trig && no_cast()) { if (IN.hit[KEY_MOUSE1] || IN.hit[KEY_CAST]) hush_say(); trig = 0; }
    if (!may && G.chg_on) { G.chg_on = 0; tether_end(); }          /* a screen opened over it: the charge is let fall */
    if (restore_cast(dt, trig, may)) trig = 0;                     /* Restoration in hand: held, it channels (D5, 10.3) */
    if (spell_holds(G.spell) || G.chg_on) {                        /* a charging ultimate's spell, or Soul Tether's: held, not repeated */
        if (!G.chg_on && trig && G.pc_t < 0.0f && G.cooldown <= 0.0f && !G.dead && (IN.hit[KEY_MOUSE1] || IN.hit[KEY_CAST])) {
            G.chg_on = 1; G.chg_sp = G.spell; G.chg_t = 0.0f;
        }
        if (G.chg_on) {
            if (trig && !G.dead) {
                G.chg_t += dt;
                if (spell_tethers(G.chg_sp) && G.chg_t >= 0.25f) tether_hold(dt);   /* Soul Tether: the beam kept on */
            } else if (G.dead) { G.chg_on = 0; tether_end(); }
            else {
                G.chg_on = 0;
                if (G.tether) tether_end();
                else if (G.chg_t < 0.25f || (!spell_charges(G.chg_sp) && !fire_grows(G.chg_sp))) {   /* a tap: the ordinary spell, wind-up and all */
                    float cyc = cast_cycle(G.chg_sp);
                    G.pc_wind = spell_windup(&RUN.gear, G.chg_sp, spell_tier(G.chg_sp)) * af_slow_cast();
                    G.pc_t = 0.0f; G.pc_fired = 0; G.pc_len = G.pc_wind + 0.25f; G.cooldown = fmaxf(cyc, G.pc_len);
                } else if (fire_grows(G.chg_sp) && !(spell_charges(G.chg_sp) && G.chg_t >= FIRE_GROW_T)) {   /* Burning Intensity: the bolt grown;
                                                                      past three seconds with the Firewall, the Firewall (A9) */
                    cast_held(G.chg_sp, aim_target(), G.chg_t);
                    G.pc_t = 0.3f; G.pc_fired = 1; G.pc_wind = 0.3f; G.pc_len = 0.55f; G.calm = 0.0f;
                    G.cooldown = cast_cycle(G.chg_sp);
                } else {                                           /* the charge goes: it took the wind-up's place */
                    cast_charged(G.chg_sp, aim_target(), clampf(G.chg_t / hold_span(G.chg_sp), 0.0f, 1.0f));
                    G.pc_t = 0.3f; G.pc_fired = 1; G.pc_wind = 0.3f; G.pc_len = 0.55f; G.calm = 0.0f;
                    G.cooldown = cast_cycle(G.chg_sp) * 1.5f;
                }
            }
        }
        trig = 0;                                                  /* holding never repeats it */
    }
    if (trig && G.pc_t < 0.0f && G.cooldown <= 0.0f && !G.dead) {
        if (G.spell == AT_BFS && RUN.souls < bfs_cost(RUN.gear.atk[AT_BFS])) {
            if (IN.hit[KEY_MOUSE1] || IN.hit[KEY_CAST]) { say("THE BFS9000 NEEDS %d SOULS", bfs_cost(RUN.gear.atk[AT_BFS])); snd_sfx_ex(SFX_ZAP, 0.4f, 0.5f); }
        } else {
            float cyc = cast_cycle(G.spell);
            G.pc_wind = spell_windup(&RUN.gear, G.spell, spell_tier(G.spell)) * af_slow_cast();
            G.pc_t = 0.0f; G.pc_fired = 0; G.pc_len = G.pc_wind + 0.25f; G.cooldown = fmaxf(cyc, G.pc_len);
        }
    }
    /* the hand is out of sight but for a cast or a ware reached for; after one it stays up a moment, ready
     * for the next, then sinks away */
    float rest = G.shop ? 0.45f : 0.62f;                           /* lower in the merchant's room, to see the wares */
    G.ready_t = G.pc_t >= 0.0f || G.grab_t > 0.0f || G.chg_on ? 0.9f : fmaxf(0.0f, G.ready_t - dt);
    G.hand_base = G.ready_t > 0.0f ? fminf(rest, G.hand_base + dt / 0.08f) : fmaxf(0.0f, G.hand_base - dt / 0.3f);
    float base = G.hand_base, h = base;
    if (G.pc_t >= 0.0f) {
        G.pc_t += dt;
        float L = G.pc_len, top = lerpf(rest, 1.0f, G.bar), w = G.pc_wind;   /* with the bar away the sleeve shows at rest: no second jump */
        if (!G.pc_fired && G.pc_t >= w) { G.pc_fired = 1; cast_spell(G.spell, aim_target()); G.calm = 0.0f; }
        float up = fminf(0.18f, w);                                /* up, it gathers, it goes, and down */
        h = G.pc_t < up ? lerpf(base, top, G.pc_t / up) : (G.pc_t < w + 0.08f ? top : lerpf(top, base, (G.pc_t - w - 0.08f) / fmaxf(0.05f, L - w - 0.08f)));
        if (G.pc_t >= L) G.pc_t = -1.0f;
    }
    if (G.grab_t > 0.0f) { G.grab_t -= dt; h = fmaxf(h, base + (1.0f - base) * sinf(PI_F * clampf(1.0f - G.grab_t, 0.0f, 1.0f))); }
    if (G.chg_on) h = fmaxf(h, lerpf(base, lerpf(rest, 1.0f, G.bar), clampf(G.chg_t / 0.2f, 0.0f, 1.0f)));   /* up, gathering it */
    G.hand = h;
}

/* The feet go the way the wisp went, to its end, pressing E at doors, the exit and the altar's secrets:
 * by the console until a key is touched, or the test player (CRAWL_FOLLOWWISP), who can't be hurt */
static void pilgrim_step(float dt) {
    const Wisp *fw = G.wisps[1].on && G.wisps[1].state != WS_GONE ? &G.wisps[1] : &G.wisps[0];
    float lean;
    if (!fw->on) {
        if (G.pilgrim == 1) { G.pilgrim = 0; say("THE WISP IS GONE, AND THE PULL WITH IT"); }
        return;
    }
#ifdef CRAWL_DEBUG
    if (G.pilgrim == 2) G.hp = G.max_hp;                           /* the test player can't be hurt */
    if (G.pilgrim == 2 && G.altar_wisp == AW_SEEK && G.altar_seek >= 0 && G.secrets[G.altar_seek].state == 2) {   /* testing the altar's */
        const Secret *sc = &G.secrets[G.altar_seek];                /* wisp: in, out, and E at it */
        if (G.altar_inside && !in_secret(G.altar_seek)) { G.ang = atan2f(G.wisps[1].y - G.py, G.wisps[1].x - G.px); IN.hit[KEY_E] = 1; return; }
        float lx = sc->sx + 0.5f, ly = sc->sy + 0.5f, along = (G.px - lx) * DX[sc->d] + (G.py - ly) * DY[sc->d];
        float off = (G.px - lx) * DY[sc->d] + (G.py - ly) * DX[sc->d];   /* across the stone's path, which is the way in */
        float want = G.altar_inside ? -1.0f : 2.0f;                    /* in two cells, or back out to the hall */
        float tx = lx + DX[sc->d] * want, ty = ly + DY[sc->d] * want;
        if (fabsf(off) > 0.08f) { tx = lx + DX[sc->d] * along; ty = ly + DY[sc->d] * along; }   /* in line with it first */
        G.ang = atan2f(ty - G.py, tx - G.px); IN.down[KEY_W] = 1;
        return;
    }
    if (G.pilgrim == 2 && G.altar_wisp == AW_REST && hypotf(G.altar_x + 0.5f - G.px, G.altar_y + 0.5f - G.py) < 1.5f
        && line_clear(G.px, G.py, G.altar_x + 0.5f, G.altar_y + 0.5f)) {
        static float wait;                                          /* at the altar: a look at it resting, then its menu */
        G.ang = atan2f(G.altar_y + 0.5f - G.py, G.altar_x + 0.5f - G.px);
        if ((wait += dt) < 2.0f) return;
        wait = 0.0f; IN.hit[KEY_E] = 1;                             /* and Enter there (update_ui) sends it on */
        return;
    }
#endif
    if (!wisp_lean(fw, &lean)) return;                             /* only just kindled: it hasn't found its way yet */
    G.ang = wrap_pi(G.ang + wrap_pi(lean - G.ang) * fminf(1.0f, dt * 8.0f));
    IN.down[KEY_W] = 1;
    IN.hit[KEY_E] = fmodf((float)G.t, 0.5f) < dt;                  /* doors in the way */
    if (G.pilgrim == 2 && G.altar_x >= 0 && hypotf(G.altar_x + 0.5f - G.px, G.altar_y + 0.5f - G.py) < 2.5f) IN.hit[KEY_E] = 0;   /* not the altar's menu */
    if ((int)G.px == G.exit_fx && (int)G.py == G.exit_fy) { G.ang = heading_angle(G.exit_dir); IN.hit[KEY_E] = 1; }
    for (int i = 0; i < G.nsecrets; i++)                            /* the altar's wisp's secrets */
        if (!G.secrets[i].state && (int)G.px == G.secrets[i].fx && (int)G.py == G.secrets[i].fy) { G.ang = heading_angle(G.secrets[i].d); IN.hit[KEY_E] = 1; }
#ifdef CRAWL_DEBUG
    if (G.pilgrim == 2) {
        if (fmodf((float)G.t, 2.0f) < dt)
            fprintf(stderr, "FOLLOW t=%.1f at (%.2f,%.2f) route=%d goal=(%d,%d) wisp=(%.1f,%.1f) state=%d lean=%.2f\n", G.t, G.px, G.py,
                    fw->field[(int)G.py][(int)G.px], fw->gx, fw->gy, fw->x, fw->y, fw->state, lean);
        G.hp = G.max_hp;
    }
#endif
}

