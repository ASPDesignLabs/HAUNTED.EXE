/* A part of scene_crawl.c (docs/PLAN-tidy.md): it is #included there in its place and never compiled on its own.
 * The portraits of the slain, drawn: what each shows, their overlays, their cards. */

/* ---- the portraits of the slain, drawn (docs/PLAN-portraits.md): what each shows, from the record, and its overlay kept ---- */

/* portrait i at the clock t: its creature in the rank shown (its unlocked ranks in turn, five seconds each, the next coming
 * through over the last two thirds of a second: v2 and how far, in *mix), on the stone of the run that rank was unlocked in */
static void por_view(int i, float t, PortraitView *v, PortraitView *v2, float *mix) {
    const PorAt *a = &POR.at[i];
    int k = a->kind, rk[NRANK], nr = por_ranks(k, rk), unl = por_unlocked(k), list[NRANK], n = 0;
    memset(v, 0, sizeof *v);
    v->spr[0] = KINDS[k].spr; v->spr[1] = KINDS[k].spr2; v->spr[2] = KINDS[k].side; v->spr[3] = KINDS[k].side2;
    v->family = KINDS[k].family; v->layout = a->layout; v->t = t; v->walk0 = a->ph;
    memcpy(v->ranks, rk, sizeof rk); v->nranks = nr;
    v->unlocked = unl; v->unpaid = por_waiting(k); v->frame = por_high(unl); v->unlit = !por_lit(i);
    v->veil = RUN.por_drawn >> k & 1 ? 1.0f - smoothstepf(POR.veil_t[i], POR.veil_t[i] + POR_DRAW_T, t) : 1.0f;   /* drawn back over a moment */
    for (int j = 0; j < nr; j++) if (unl >> rk[j] & 1) list[n++] = rk[j];
    float u = (t - a->ph) / 5.0f, fr = u - floorf(u);
    *v2 = *v; *mix = 0.0f;
    for (int w = 0; w < 2; w++) {
        PortraitView *p = w ? v2 : v;
        int rank = n ? list[((int)floorf(u) + w) % n] : -1;
        int run = rank >= 0 && RUN.por_run[k][rank] ? RUN.por_run[k][rank] - 1 : por_guess(k, rank >= 0 ? rank : RK_I);
        p->rank = rank; p->wall = portrait_run_wall(run); p->floor = portrait_run_floor(run);
        int ec = rank > RK_I && KINDS[k].family >= 0 && EYE_RANK[rank] >= 0 ? EYE_RANK[rank] : EYE_OWN[k];   /* the spirits' never change */
        p->eye_core = rank >= 0 && crawl_eyes[p->spr[0]].n ? EYE_CORE[ec] : 0; p->eye_glow = EYE_GLOW[ec];
    }
    if (n > 1 && fr > 0.87f) *mix = (fr - 0.87f) / 0.13f;
}
/* a boss's portrait as it stands: its arena, its sprites, its deeds and its gold, its tapestries */
static void por_boss_view(int i, float t, BossPortraitView *v) {
    static const int SPR[NBOSS][5] = {                             /* walking, raising its blow, striking, a pose of its own */
        { SP_BKNIGHT_A, SP_BKNIGHT_B, SP_BKNIGHT_W, SP_BKNIGHT_S, SP_BKNIGHT_P0 },
        { SP_BSPIDER_A, SP_BSPIDER_B, SP_BSPIDER_REAR, SP_BSPIDER_LEAP, SP_BSPIDER_REAR },
        { SP_BDEACON_A, SP_BDEACON_B, SP_BDEACON_CHANT, SP_BDEACON_CHANT, SP_BDEACON_A },
        { SP_WRAITH_A, SP_WRAITH_B, SP_WRAITH_REACH, SP_WRAITH_REACH, SP_WRAITH_A } };
    static const int PROP[NBOSS][3] = {                            /* what stands behind it (and its frames), what lies before it */
        { SP_PILLAR, 1, SP_BROKEN_ARMS }, { SP_PILLAR, 1, SP_PUDDLE_A }, { SP_TIMBER, 1, SP_TOMB_BROKEN }, { SP_BRAZIER_A, 3, SP_OBSIDIAN } };
    int h = por_head(i), b = por_bossof(h), ec = EYE_OWN[BOSSES[b].kind];
    memset(v, 0, sizeof *v);
    memcpy(v->spr, SPR[b], sizeof v->spr);
    v->prop[0] = PROP[b][0]; v->prop[1] = PROP[b][2]; v->prop_n = PROP[b][1];
    v->eye_core = ec >= 0 ? EYE_CORE[ec] : 0; v->eye_glow = ec >= 0 ? EYE_GLOW[ec] : 0;
    v->wall = portrait_run_wall(b); v->floor = portrait_run_floor(b);
    v->unlocked = boss_unlocked(b); v->unpaid = boss_waiting(b);
    v->veil = RUN.por_drawn >> por_bit(h) & 1 ? 1.0f - smoothstepf(POR.veil_t[h], POR.veil_t[h] + POR_DRAW_T, t) : 1.0f;
    v->t = t - POR.at[h].ph; v->scale = 1.25f; v->unlit = !por_lit(h);
}
/* each frame in her rooms: a portrait in view last frame (or never yet drawn) is drawn again when its candles' flames have moved,
 * twenty times a second, its creature's step and turn with them; the rest wait as they are */
static void por_frame(void) {
    if (!G.shop || !POR.n) return;
    static uint8_t a[POR_RES * POR_RES], b[POR_RES * POR_RES], bb[2 * POR_RES * POR_RES];
    float t = (float)G.t;
    for (int i = 0; i < POR.n; i++) {
        int key = (int)floorf(t * 20.0f + 0.37f * (float)i);       /* their turns spread across the frames */
        if (POR.at[i].boss) {                                      /* a boss's, both its faces at once from its left half */
            if (POR.at[i].half || i + 1 >= POR.n) continue;
            if (POR.drawn[i] && (key == POR.key[i] || (POR.seen[i] < POR.frame - 1 && POR.seen[i + 1] < POR.frame - 1))) continue;
            BossPortraitView bv;
            por_boss_view(i, t, &bv);
            memset(bb, C_CLEAR, sizeof bb);
            portrait_boss_face(bb, 2 * POR_RES, 2 * POR_RES, POR_RES, 0, 0, POR_RES / TS, &bv);
            for (int hh = 0; hh < 2; hh++) {
                uint8_t *o = POR_OV[i + hh];
                for (int x = 0; x < POR_RES; x++) for (int y = 0; y < POR_RES; y++) o[(POR_RES - 1 - x) * POR_RES + y] = bb[y * 2 * POR_RES + hh * POR_RES + x];
                POR.key[i + hh] = key; POR.drawn[i + hh] = 1;
            }
            continue;
        }
        if (POR.drawn[i] && (key == POR.key[i] || POR.seen[i] < POR.frame - 1)) continue;
        PortraitView v, v2;
        float mix;
        por_view(i, t, &v, &v2, &mix);
        memset(a, C_CLEAR, sizeof a);
        portrait_face(a, POR_RES, POR_RES, POR_RES, 0, 0, POR_RES / TS, &v);
        if (mix > 0.0f) {                                          /* the next rank coming through, dithered */
            memset(b, C_CLEAR, sizeof b);
            portrait_face(b, POR_RES, POR_RES, POR_RES, 0, 0, POR_RES / TS, &v2);
            int s = POR_RES / TS;
            for (int y = 0; y < POR_RES; y++) for (int x = 0; x < POR_RES; x++)
                if ((float)BAYER[(y / s) & 3][(x / s) & 3] < mix * 16.0f) a[y * POR_RES + x] = b[y * POR_RES + x];
        }
        uint8_t *o = POR_OV[i];                                    /* by columns, as the walls are kept: the first on the right as seen */
        for (int x = 0; x < POR_RES; x++) for (int y = 0; y < POR_RES; y++) o[(POR_RES - 1 - x) * POR_RES + y] = a[y * POR_RES + x];
        POR.key[i] = key; POR.drawn[i] = 1;
    }
    POR.frame++;
}

static void medal_glyph(Frame *f, int x, int y, int on) {          /* a feat's medallion on its ribbon, 8 by 8 */
    static const char *const M[8] = { "#....#..", ".#..#...", "..##....", ".####...", "#oooo#..", "#o##o#..", "#oooo#..", ".####..." };
    for (int r = 0; r < 8; r++) for (int c = 0; c < 8; c++) {
        char q = M[r][c];
        if (q == '.') continue;
        uint8_t col = r < 3 ? (on ? (uint8_t)(R_BLOOD + 9) : (uint8_t)(R_STONE + 6)) : q == 'o' ? (on ? (uint8_t)(R_GOLD + 9) : (uint8_t)(R_STONE + 7))
                    : on ? (uint8_t)(R_GOLD + 13) : (uint8_t)(R_STONE + 10);
        gfx_px(f, x + c + 1, y + r, col);
    }
}
/* a portrait's card (section 2, the mock-up's card.png): the portrait up close in its candles' light, and each rank of its kind,
 * slain against asked, its gold and whether it is taken this life, waits, or is to come */
static void render_bosscard(Frame *f, int p) {                     /* a boss's: its portrait up close, its six deeds, their gold */
    int b = por_bossof(p), drawn = por_is_drawn(p), unl = boss_unlocked(b), w = boss_waiting(b), hi = -1;
    for (int st = 0; st < 6; st++) if (unl >> st & 1) hi = st;
    ui_slab(f, 6, 8, 308, 184, SLAB_TABLET, ORN_CANDLE, G.ui_t);
    gfx_text16(f, 16, 14, drawn || hi >= 0 ? POR_BOSS_NAME[b] : "???", drawn && hi >= 0 ? (uint8_t)RANK_COL[BOSS_GEM[hi]] : (uint8_t)(R_GOLD + 12), 1);
    char right[24];
    if (!drawn) snprintf(right, sizeof right, "VEILED");
    else if (hi <= 0) snprintf(right, sizeof right, hi < 0 ? "NOT SLAIN" : "SLAIN");
    else snprintf(right, sizeof right, "STEP %d", hi);
    gfx_text8(f, 304 - gfx_text_width(right, 1), 18, right, drawn && hi >= 0 ? (uint8_t)RANK_COL[BOSS_GEM[hi]] : (uint8_t)(R_STONE + 11), 1);
    ui_well(f, 12, 34, 296, 106);
    {                                                              /* its portrait, its two faces' stone behind, in its stands' light */
        static uint8_t ov[2 * POR_RES * POR_RES];
        BossPortraitView v;
        float fl[6][3];
        por_boss_view(p, (float)G.t, &v);
        memset(ov, C_CLEAR, sizeof ov);
        portrait_boss_face(ov, 2 * POR_RES, 2 * POR_RES, POR_RES, 0, 0, POR_RES / TS, &v);
        int nf = portrait_boss_flames(fl);
        const uint8_t *base[2];
        for (int h = 0; h < 2; h++) {
            const PorAt *a = &POR.at[p + h];
            const Cell *c = &G.map[a->y][a->x];
            base[h] = crawl_tex[c->face[a->fd] ? c->face[a->fd] : T_BRICK];
        }
        for (int y = 0; y < 104; y++) for (int x = 0; x < 2 * POR_RES; x++) {
            int oy = y + 8;
            uint8_t o = ov[oy * 2 * POR_RES + x], bs = base[x / POR_RES][((x % POR_RES) / 2) * TS + oy / 2];
            float l = 0.30f;
            for (int q = 0; q < nf; q++) { float d = hypotf((float)x / 2.0f - fl[q][0], (float)oy / 2.0f - fl[q][1]) / 64.0f; l += 0.6f * fl[q][2] / (1.0f + 30.0f * d * d); }
            f->pix[(35 + y) * W + 32 + x] = shade(o != C_CLEAR ? o : bs, l, x, y);
        }
    }
    static const char *const DEED[6] = { "SLAIN", "STEP 1", "STEP 2", "STEP 3", "STEP 4", "STEP 5" };
    for (int st = 0; st < 6; st++) {                               /* its deeds in two columns: what each pays, and whether it waits */
        int x = 16 + (st / 3) * 148, y = 145 + (st % 3) * 10, on = unl >> st & 1;
        char g[16];
        if (w >> st & 1) ui_row(f, x - 3, y - 1, 146, 10, 1);
        gfx_text8(f, x, y, DEED[st], on ? (uint8_t)RANK_COL[BOSS_GEM[st]] : (uint8_t)(R_STONE + 9), 1);
        snprintf(g, sizeof g, "%5d", boss_gold(b, st));
        gfx_text8(f, x + 52, y, g, on ? (uint8_t)(R_GOLD + 12) : (uint8_t)(R_STONE + 8), 1);
        const char *ss = !on ? "" : RUN.por_btaken[b] >> st & 1 ? "TAKEN" : w >> st & 1 ? "WAITS" : "LATER";
        if (ss[0]) gfx_text8(f, x + 100, y, ss, w >> st & 1 ? (uint8_t)(R_GOLD + 12) : (uint8_t)(R_STONE + 10), 1);
    }
    char keys[64];
    int gold = boss_wait_gold(b);
    if (!drawn && por_can_draw(p)) snprintf(keys, sizeof keys, "E DRAW BACK  A/D ANOTHER  ESC LEAVE");
    else if (gold) snprintf(keys, sizeof keys, "E TAKE %d  A/D ANOTHER  ESC LEAVE", gold);
    else snprintf(keys, sizeof keys, "A/D ANOTHER  ESC LEAVE");
    hall_keys(f, 179, keys);
}
static void render_portraitcard(Frame *f) {
    dim_view(f);
    if (!POR.n) return;
    if (POR.at[por_head(clampi(G.pt_sel, 0, POR.n - 1))].boss) { render_bosscard(f, por_head(clampi(G.pt_sel, 0, POR.n - 1))); return; }
    int p = clampi(G.pt_sel, 0, POR.n - 1), k = POR.at[p].kind, drawn = RUN.por_drawn >> k & 1, unl = por_unlocked(k), w = por_waiting(k);
    int rk[NRANK], nr = por_ranks(k, rk), hi = por_high(unl), met = kind_met(k);
    ui_slab(f, 6, 8, 308, 184, SLAB_TABLET, ORN_CANDLE, G.ui_t);
    gfx_text16(f, 16, 14, drawn || met ? KINDS[k].plural : "???", drawn && hi >= 0 ? (uint8_t)RANK_COL[hi] : (uint8_t)(R_GOLD + 12), 1);
    char right[24];
    if (!drawn) snprintf(right, sizeof right, "VEILED");
    else if (hi < 0) snprintf(right, sizeof right, "ITS SHADOW");
    else snprintf(right, sizeof right, "RANK %s", hi == RK_GILDED ? "G" : RANK_ROMAN[hi]);
    gfx_text8(f, 304 - gfx_text_width(right, 1), 18, right, hi >= 0 && drawn ? (uint8_t)RANK_COL[hi] : (uint8_t)(R_STONE + 11), 1);
    ui_well(f, 12, 34, 124, 140);
    {                                                              /* the portrait, drawn up close on its own stone, in its candles' light */
        static uint8_t ov[POR_RES * POR_RES];
        PortraitView v, v2;
        float mix, fl[4][3];
        por_view(p, (float)G.t, &v, &v2, &mix);
        memset(ov, C_CLEAR, sizeof ov);
        portrait_face(ov, POR_RES, POR_RES, POR_RES, 0, 0, POR_RES / TS, &v);
        int nf = portrait_flames(&v, fl);
        const Cell *c = &G.map[POR.at[p].y][POR.at[p].x];
        const uint8_t *base = crawl_tex[c->face[POR.at[p].fd] ? c->face[POR.at[p].fd] : T_BRICK];
        for (int y = 0; y < 128; y++) for (int x = 0; x < 118; x++) {
            uint8_t o = ov[y * POR_RES + x + 2], b = base[((x + 2) / 2) * TS + y / 2];
            float l = 0.30f;
            for (int q = 0; q < nf; q++) { float d = hypotf((float)(x + 2) / 2.0f - fl[q][0], (float)y / 2.0f - fl[q][1]) / 64.0f; l += 0.9f * fl[q][2] / (1.0f + 30.0f * d * d); }
            f->pix[(40 + y) * W + 15 + x] = shade(o != C_CLEAR ? o : b, l, x, y);
        }
    }
    ui_well(f, 140, 34, 168, 104);
    int ghost = k == MOB_GHOST, wraith = k == MOB_WRAITH, fi = feat_of(k), rows = imin(nr, 6) + (fi >= 0);
    int sp = rows > 6 ? 12 : 13, y0 = rows > 6 ? 51 : 53;           /* a seventh row, the feat's, closes them up */
    gfx_text8(f, 146, 39, ghost ? "RANK LAID   GOLD" : wraith ? "RANK DRIVEN GOLD" : "RANK SLAIN  GOLD", R_GOLD + 10, 1);
    for (int j = 0; j < rows; j++) {
        int feat = j == rows - 1 && fi >= 0, r = feat ? POR_FEAT : rk[j], y = y0 + j * sp, on = unl >> r & 1, need, have, gem = wraith && r == RK_IV;
        if (feat) need = FEATS[fi].need, have = por_count(k);
        else if (ghost || wraith) need = gem ? 1 : wraith ? WRAITH_STEP[r] : GHOST_STEP[r], have = gem ? on : por_count(k);
        else need = slayer_need(k, r), have = RUN.rec.kills[k][r];
        have = on ? need : imin(have, need);
        char a[16], g[16];
        if (w >> r & 1) ui_row(f, 142, y - 3, 164, sp, 1);
        if (feat) medal_glyph(f, 146, y, on);
        else gfx_text8(f, 146, y, r == RK_GILDED ? "G" : RANK_ROMAN[r], on ? (uint8_t)RANK_COL[r] : (uint8_t)(R_STONE + 9), 1);
        snprintf(a, sizeof a, "%d/%d", have, need);
        gfx_text8(f, 222 - gfx_text_width(a, 1), y, a, on ? (uint8_t)C_WHITE : (uint8_t)(R_STONE + 11), 1);
        if (gem) snprintf(g, sizeof g, "   -"); else snprintf(g, sizeof g, "%4d", por_gold(k, r));
        gfx_text8(f, 226, y, g, on ? (uint8_t)(R_GOLD + 12) : (uint8_t)(R_STONE + 8), 1);
        const char *st = !on ? "" : gem ? "LIT" : RUN.por_taken[k] >> r & 1 ? "TAKEN" : w >> r & 1 ? "WAITS" : "LATER";
        if (st[0]) gfx_text8(f, 266, y, st, w >> r & 1 ? (uint8_t)(R_GOLD + 12) : (uint8_t)(R_STONE + 10), 1);
    }
    const char *words = !drawn ? (por_drawable(k) ? (ghost ? "ONE IS LAID TO REST. ITS TAPESTRY MAY COME DOWN." : wraith ? "IT HAS BEEN DRIVEN BACK. ITS TAPESTRY MAY COME DOWN."
                                                         : "ONE OF ITS KIND IS SLAIN. ITS TAPESTRY MAY COME DOWN.")
                                                : ghost ? "IT WAITS UNDER ITS TAPESTRY TILL ONE IS LAID TO REST, BY SPIRIT ALONE."
                                                : wraith ? "IT WAITS UNDER ITS TAPESTRY TILL THE WRAITH IS DRIVEN BACK."
                                                : "IT WAITS UNDER ITS TAPESTRY TILL ONE OF ITS KIND IS SLAIN.")
                      : !RUN.por_open && unl ? "IT PAYS ONCE A LEVEL IS CLEARED: EACH RANK ONCE A LIFE."
                      : ghost ? "ONLY SPIRIT LAYS ONE TO REST. EACH RANK PAYS ONCE A LIFE."
                      : wraith ? "DRIVEN BACK, IT COUNTS. ITS FOURTH: THE UNBOUND SLAIN."
                      : fi >= 0 ? NULL : "EACH RANK PAYS ONCE A LIFE: AFTER A LEVEL, OR ON WAKING.";
    if (words) text_wrap(f, 144, 144, words, 20, R_STONE + 12, 3);
    else {                                                         /* its feat, under its medallion */
        gfx_text8(f, 144, 144, FEATS[fi].name, unl >> POR_FEAT & 1 ? (uint8_t)(R_GOLD + 12) : (uint8_t)(R_GOLD + 9), 1);
        text_wrap(f, 144, 153, FEATS[fi].rule, 20, R_STONE + 12, 2);
    }
    char keys[64];
    int gold = por_wait_gold(k);
    if (!drawn && por_drawable(k)) snprintf(keys, sizeof keys, "E DRAW IT BACK  A/D ANOTHER  ESC LEAVE");
    else if (gold) snprintf(keys, sizeof keys, "E TAKE %d  A/D ANOTHER  ESC LEAVE", gold);
    else snprintf(keys, sizeof keys, "A/D ANOTHER  ESC LEAVE");
    hall_keys(f, 179, keys);
}

