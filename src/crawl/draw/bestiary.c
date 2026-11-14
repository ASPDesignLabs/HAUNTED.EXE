/* A part of scene_crawl.c (docs/PLAN-tidy.md): it is #included there in its place and never compiled on its own.
 * Pop-ups (toasts); the bestiary: its logic and its pages. */

/* ---- the tracker's pop-up, and the bestiary (B) ---- */

static const char *const RANK_ROMAN[NRANK] = { "I", "II", "III", "IV", "V", "GILDED" };

/* a sprite drawn flat into a box, scaled to size, in a rank's colours, or as a dark shape; flip: facing left */
static void ui_sprite_ex(Frame *f, int spr, int x0, int y0, int size, const uint8_t *lut, int shadow, int flip) {
    for (int y = 0; y < size; y++)
        for (int x = 0; x < size; x++) {
            int sx = flip ? size - 1 - x : x;
            uint8_t c = crawl_spr[spr][(sx * TS / size) * TS + y * TS / size];
            if (c == C_CLEAR) continue;
            gfx_px(f, x0 + x, y0 + y, shadow ? (uint8_t)(R_IRON + 2) : (lut ? lut[c] : c));
        }
}
static void ui_sprite(Frame *f, int spr, int x0, int y0, int size, const uint8_t *lut, int shadow) { ui_sprite_ex(f, spr, x0, y0, size, lut, shadow, 0); }
/* the charnel rat's page (docs/PLAN-mice.md, M5): what it grew from, the seven sizes of the mice running in a row over it, each at
 * its size against the others, the house mouse on the left to the spurt on the right, their feet on one line */
static void best_growth(Frame *f, int x0, int base) {
    static uint8_t spr[MOUSE_STAGES][TS * TS];
    static int made;
    if (!made) { for (int s = 0; s < MOUSE_STAGES; s++) crawl_mouse_sprite(s, MP_RUNA, spr[s], NULL); made = 1; }
    int x = x0;
    for (int s = 0; s < MOUSE_STAGES; s++) {
        int size = (int)(crawl_mouse_box(s) * 75.0f + 0.5f);
        for (int y = 0; y < size; y++)
            for (int xx = 0; xx < size; xx++) {
                uint8_t c = spr[s][(y * TS / size) * TS + xx * TS / size];
                if (c != C_CLEAR) gfx_px(f, x + xx, base - size + y, c);
            }
        x += size + 2;
    }
}

static int toast_showing(void) { return TOASTS.n > 0 && TOASTS.t >= 0.0f && !boss_fighting(); }

static void render_toasts(Frame *f) {
    if (!toast_showing()) return;
    const Toast *t = &TOASTS.q[0];
    float len = toast_len(t), k = smoothstepf(0.0f, 0.25f, TOASTS.t) * (1.0f - smoothstepf(len - 0.3f, len, TOASTS.t));
    char a[32], b[32];                                             /* the title on a line, or two */
    snprintf(a, sizeof a, "%s", t->title); b[0] = 0;
    if (strlen(a) > 20) {
        int cut = 20;
        while (cut > 0 && a[cut] != ' ') cut--;
        if (cut > 0) { snprintf(b, sizeof b, "%s", a + cut + 1); a[cut] = 0; }
    }
    const char *head = t->spr == SP_CCOIN ? "FOR THE SHRINE" : t->spr == SP_ITEM_TOME ? "THE ALIGNMENT" : t->spr >= SP_HIDE0 && t->spr < SP_HIDE0 + 4 ? "A DOCUMENT"
                     : t->progress ? "PROGRESS" : "ACHIEVEMENT";
    int tw = (int)strlen(head);
    if ((int)strlen(a) > tw) tw = (int)strlen(a);
    if ((int)strlen(b) > tw) tw = (int)strlen(b);
    if ((int)strlen(t->desc) > tw) tw = (int)strlen(t->desc);
    int lines = 3 + (b[0] != 0), bw = 38 + tw * 8 + 6, bh = lines * 9 + 8 > 36 ? lines * 9 + 8 : 36;
    int x = W - (int)((float)(bw + 3) * k), y = 3;
    int edge = t->progress ? R_STONE + 9 : R_GOLD + 9;
    ui_band(f, x, y, bw, bh);                                      /* a small tablet of the run's stone, edged in gold for an achievement */
    for (int i = 1; i < bw - 1; i++) { gfx_px(f, x + i, y + 1, (uint8_t)edge); gfx_px(f, x + i, y + bh - 2, (uint8_t)(edge - 4)); }
    for (int i = 1; i < bh - 1; i++) { gfx_px(f, x + 1, y + i, (uint8_t)edge); gfx_px(f, x + bw - 2, y + i, (uint8_t)(edge - 4)); }
    bevel(f, x + 4, y + 4, 30, 28, R_IRON + 2);
    const uint8_t *lut = t->family >= 0 && t->rank != RK_I ? crawl_rank_lut[t->family][t->rank] : NULL;
    ui_sprite(f, t->spr, x + 6, y + 5, 26, lut, 0);
    int tx = x + 38, ty = y + 5;
    gfx_text8(f, tx, ty, head, t->progress ? (uint8_t)(R_STONE + 12) : (uint8_t)(R_GOLD + 12), 1); ty += 9;
    gfx_text8(f, tx, ty, a, C_WHITE, 1); ty += 9;
    if (b[0]) { gfx_text8(f, tx, ty, b, C_WHITE, 1); ty += 9; }
    gfx_text8(f, tx, ty, t->desc, R_STONE + 12, 1);
}

/* the bestiary's pages: the kinds in the order you meet them, then the achievements */
static int best_kinds(int *out) {
    int n = 0;
    for (int lv = 1; lv <= DESCENT_LEN; lv++) for (int k = 0; k < NKINDS; k++) if (KINDS[k].debut == lv && (KINDS[k].flags & KF_PAGE)) {   /* by first */
        if (k == MOB_RAT) {                                        /* level, to the Deep's; the rat a secret till it is met (docs/PLAN-mice.md, M5) */
            int met = RUN.rec.seen[k] != 0;
            for (int r = 0; r < NRANK; r++) met |= RUN.rec.kills[k][r] != 0;
            if (!met) continue;
        }
        out[n++] = k;
    }
    return n;
}
/* the portraits' kinds (docs/PLAN-portraits.md): the bestiary's but the charnel rat, which has no place in her hall yet, so that
 * every other kind's portrait, third and gold stay as they were (docs/PLAN-mice.md, section 5) */
static int por_kinds(int *out) {
    int n = 0;
    for (int lv = 1; lv <= DESCENT_LEN; lv++) for (int k = 0; k < NKINDS; k++) if (KINDS[k].debut == lv && (KINDS[k].flags & KF_PAGE) && k != MOB_RAT) out[n++] = k;
    return n;
}
static int kind_ranks(int k, int *out) {                           /* the ranks a kind comes in */
    int n = 0;
    if (KINDS[k].family < 0) { out[n++] = RK_I; return n; }
    for (int r = 0; r < NRANK; r++) if (slayer_need(k, r)) out[n++] = r;
    return n;
}
static int kind_met(int k) {
    if (RUN.rec.seen[k]) return 1;
    for (int r = 0; r < NRANK; r++) if (RUN.rec.kills[k][r]) return 1;
    return 0;
}

typedef struct { int done, known; char title[40], prog[16]; } AchLine;
static int ach_lines(AchLine *L, int max) {
    const Record *r = &RUN.rec;
    int order[NKINDS], nk = best_kinds(order), n = 0;
    for (int i = 0; i < nk && n < max; i++) {
        int k = order[i], rk[NRANK], nr = kind_ranks(k, rk);
        for (int j = 0; j < nr && n < max; j++) {
            int need = slayer_need(k, rk[j]);
            if (!need) continue;
            AchLine *l = &L[n++];
            l->done = ach_has(k * NRANK + rk[j]); l->known = kind_met(k);
            if (l->known) snprintf(l->title, sizeof l->title, "%s%s", rank_word(k, rk[j]), KINDS[k].plural);
            else snprintf(l->title, sizeof l->title, "%s???", rank_word(k, rk[j]));
            snprintf(l->prog, sizeof l->prog, "%d/%d", r->kills[k][rk[j]] < need ? r->kills[k][rk[j]] : need, need);
        }
        for (int q = 0; q < NPORACH && n < max; q++) {             /* its portrait's own: laid to rest, driven back, its feat */
            PorAch a;
            por_ach(q, &a);
            if (a.kind != k) continue;
            AchLine *l = &L[n++];
            l->done = ach_has(a.bit); l->known = kind_met(k) || por_count(k) > 0;
            snprintf(l->title, sizeof l->title, "%s", l->known || l->done ? a.title : "A FEAT: ???");
            snprintf(l->prog, sizeof l->prog, "%d/%d", imin(por_count(k), a.need), a.need);
        }
    }
    for (int i = 0; i < NSECRET_STEPS && n < max; i++) {
        AchLine *l = &L[n++];
        l->done = ach_has(ACH_SECRET0 + i); l->known = 1;
        snprintf(l->title, sizeof l->title, "%s", SECRET_NAME[i]);
        snprintf(l->prog, sizeof l->prog, "%d/%d", r->secrets < SECRET_STEP[i] ? r->secrets : SECRET_STEP[i], SECRET_STEP[i]);
    }
    for (int lv = 1; lv <= DESCENT_LEN && n < max; lv++) {
        AchLine *l = &L[n++];
        l->done = ach_has(ach_level(lv)); l->known = 1;
        char nm[40];
        level_name(lv, nm, sizeof nm);
        if (l->done || lv <= RUN.level) snprintf(l->title, sizeof l->title, "%s", nm); else snprintf(l->title, sizeof l->title, "LEVEL %d", lv);
        snprintf(l->prog, sizeof l->prog, "L%d", lv);
    }
    int top = r->descents + 1 < 2 ? 2 : r->descents + 1;           /* the descents so far, and the next */
    for (int k = 2; k <= top && k <= MAX_DESCENT && n < max; k++) {
        AchLine *l = &L[n++];
        l->done = ach_has(ACH_DESCENT0 + k - 2); l->known = 1;
        snprintf(l->title, sizeof l->title, "THE %s DESCENT", NTH[k]);
        snprintf(l->prog, sizeof l->prog, "L%d", k * DESCENT_LEN);
    }
    for (int k = 0; k < 6 && n < max; k++) {                       /* 1.0.0b13: puzzles solved, trials won */
        AchLine *l = &L[n++];
        int have = k < 3 ? RUN.deed_pz : RUN.deed_tri, need = k < 3 ? PZ_STEP[k] : TRI_STEP[k - 3];
        l->done = ach_has(ACH_DEED0 + k); l->known = 1;
        snprintf(l->title, sizeof l->title, "%s", k < 3 ? PZ_ACH[k] : TRI_ACH[k - 3]);
        snprintf(l->prog, sizeof l->prog, "%d/%d", have < need ? have : need, need);
    }
    for (int r = 0; r < 4 && n < max; r++) {                       /* and each run's trials, every one won */
        AchLine *l = &L[n++];
        int got = 0, all = 0;
        for (int k = 0; k < 5 && TRI_RUN[r][k] >= 0; k++) { all++; got += (int)(RUN.tri_kinds >> TRI_RUN[r][k] & 1); }
        l->done = ach_has(ACH_DEED0 + 6 + r); l->known = 1;
        snprintf(l->title, sizeof l->title, "%s", TRI_RUN_ACH[r]);
        snprintf(l->prog, sizeof l->prog, "%d/%d", got, all);
    }
    if (mind_on() || RUN.ways) for (int k = 0; k < 11 && n < max; k++) {   /* 1.0.0b18: the dead's ways answered */
        AchLine *l = &L[n++];
        int have, need; const char *t;
        if (k < 3) { have = RUN.deed_amb; need = AMB_STEP[k]; t = AMB_ACH[k]; }
        else if (k < 5) { have = RUN.deed_shaken; need = SHAKE_STEP[k - 3]; t = SHAKE_ACH[k - 3]; }
        else if (k < 8) { have = RUN.deed_heads; need = HEAD_STEP[k - 5]; t = HEAD_ACH[k - 5]; }
        else if (k < 10) { have = RUN.deed_hush; need = HUSH_STEP[k - 8]; t = HUSH_ACH[k - 8]; }
        else { have = RUN.deed_file; need = 1; t = FILE_ACH; }
        l->done = ach_has(ACH_WAY0 + k); l->known = 1;
        snprintf(l->title, sizeof l->title, "%s", t);
        snprintf(l->prog, sizeof l->prog, "%d/%d", have < need ? have : need, need);
    }
    return n;
}
#define ACH_ROWS 15

static void best_default_rank(void) {
    int order[NKINDS], nk = best_kinds(order);
    G.best_rank = RK_I;
    if (G.best_page >= nk) return;
    int k = order[G.best_page], rk[NRANK], nr = kind_ranks(k, rk);
    G.best_rank = rk[0];
    for (int j = 0; j < nr; j++) if (RUN.rec.seen[k] >> rk[j] & 1) { G.best_rank = rk[j]; break; }
}

static void update_bestiary(void) {
    if (IN.hit[KEY_B] || IN.hit[KEY_ESC] || IN.hit[KEY_TAB]) { G.ui = UI_NONE; snd_sfx_ex(SFX_KEY, 0.6f, 0.5f); return; }
    int order[NKINDS], nk = best_kinds(order), pages = nk + 1;
    int dx = (IN.hit[KEY_D] || IN.hit[KEY_RIGHT]) - (IN.hit[KEY_A] || IN.hit[KEY_LEFT]);
    int dy = (IN.hit[KEY_S] || IN.hit[KEY_DOWN]) - (IN.hit[KEY_W] || IN.hit[KEY_UP]);
    if (dx) { G.best_page = (G.best_page + dx + pages) % pages; G.best_scroll = 0; G.best_feat = 0; best_default_rank(); snd_sfx_ex(SFX_TICK, 0.6f, 0.5f); }
    if (IN.hit[KEY_SPACE] && G.best_page < nk && mind_on()) { G.best_ways ^= 1; snd_sfx_ex(SFX_PAGE, 0.5f, 0.5f); }   /* its lore, or its ways */
    if (!dy) return;
    if (G.best_page < nk) {                                        /* through the ranks it comes in, then its feat */
        int k = order[G.best_page], rk[NRANK], nr = kind_ranks(k, rk), at = 0, ft = feat_of(k) >= 0, n = nr + ft;
        for (int j = 0; j < nr; j++) if (rk[j] == G.best_rank) at = j;
        if (G.best_feat && ft) at = nr;
        at = (at + dy + n) % n;
        G.best_feat = at == nr;
        if (!G.best_feat) G.best_rank = rk[at];
    } else {
        static AchLine L[256];
        int n = ach_lines(L, 256);
        G.best_scroll = clampi(G.best_scroll + dy * 3, 0, n > ACH_ROWS ? n - ACH_ROWS : 0);
    }
    snd_sfx_ex(SFX_TICK, 0.6f, 0.5f);
}

static void medal_glyph(Frame *f, int x, int y, int on);
static void render_bestiary(Frame *f) {
    dim_view(f);
    int order[NKINDS], nk = best_kinds(order), pages = nk + 1;
    static char line[64];
    const Record *r = &RUN.rec;
    if (G.best_page >= nk) {                                       /* the achievements, earned and not */
        static AchLine L[256];
        int n = ach_lines(L, 256), got = 0;
        for (int i = 0; i < n; i++) got += L[i].done;
        snprintf(line, sizeof line, "%d OF %d EARNED", got, n);    /* in the page count's place: it is the last page */
        tab_frame(f, "ACHIEVEMENTS", R_GOLD + 12, line, R_GOLD + 10, "A/D PAGE  W/S SCROLL  B CLOSE");
        ui_well(f, 9, 37, 302, 154);
        for (int i = 0; i < ACH_ROWS && G.best_scroll + i < n; i++) {
            const AchLine *l = &L[G.best_scroll + i];
            int y = 40 + i * 10;
            gfx_text8(f, 14, y, l->done ? "*" : "-", l->done ? (uint8_t)(R_GOLD + 12) : (uint8_t)(R_STONE + 7), 1);
            gfx_text8(f, 26, y, l->title, l->done ? (uint8_t)C_WHITE : (uint8_t)(R_STONE + (l->known ? 11 : 8)), 1);
            gfx_text8(f, W - 14 - gfx_text_width(l->prog, 1), y, l->prog, l->done ? (uint8_t)(R_GOLD + 11) : (uint8_t)(R_STONE + 10), 1);
        }
        if (n > ACH_ROWS) {                                        /* where in the list: a groove and its thumb */
            int bar = 148 * ACH_ROWS / n, at = 148 * G.best_scroll / n;
            for (int y = 0; y < 148; y++) { gfx_px(f, 307, 40 + y, UI_W(0)); gfx_px(f, 308, 40 + y, UI_M(10)); }
            for (int y = 0; y < bar; y++) { gfx_px(f, 307, 40 + at + y, UI_M(12)); gfx_px(f, 308, 40 + at + y, UI_M(6)); }
        }
        return;
    }
    snprintf(line, sizeof line, "%d / %d", G.best_page + 1, pages);
    tab_frame(f, "BESTIARY", R_GOLD + 12, line, UI_M(11), "A/D CREATURE  W/S RANK  B CLOSE");
    int k = order[G.best_page], met = kind_met(k), seen = r->seen[k] >> G.best_rank & 1 || r->kills[k][G.best_rank] > 0;
    int spirit = (KINDS[k].flags & KF_SPIRIT) != 0;                /* the spectre and the Wraith: no rank, and nothing kills them */
    ui_niche(f, 12, 38, 104, 104);                                /* it stands in a niche, lit from above */
    ui_well(f, 120, 37, 191, 106);
    ui_well(f, 9, 146, 302, 45);
    const uint8_t *lut = KINDS[k].family >= 0 && G.best_rank != RK_I ? crawl_rank_lut[KINDS[k].family][G.best_rank] : NULL;
    /* it walks on the spot and turns about: towards you, its right side, towards you, its left */
    float ph = fmodf(G.ui_t, 6.4f);
    int step = (int)(G.ui_t * 4.0f) & 1, facing = (int)(ph / 1.6f);   /* 0 front, 1 right, 2 front, 3 left */
    int spr = facing & 1 ? (step ? KINDS[k].side2 : KINDS[k].side) : (step ? KINDS[k].spr2 : KINDS[k].spr);
    ui_sprite_ex(f, spr, 16, 42, 96, lut, !met || !seen, facing == 3);
    if (k == MOB_RAT && met) best_growth(f, 17, 72);               /* what it grew from (docs/PLAN-mice.md) */
    int x0 = 124;
    gfx_text16(f, x0, 38, met ? KINDS[k].name : "???", C_WHITE, 1);
    int fi = feat_of(k), feat = G.best_feat && fi >= 0, fdone = fi >= 0 && ach_has(ACH_FEAT0 + fi);   /* its feat (the portraits') */
    if (feat) snprintf(line, sizeof line, "FEAT: %s", FEATS[fi].name);
    else if (spirit) snprintf(line, sizeof line, "A SPIRIT: NO RANK");
    else if (seen) snprintf(line, sizeof line, "RANK %s  %s", RANK_ROMAN[G.best_rank], G.best_rank == RK_I ? "" : rank_word(k, G.best_rank));
    else snprintf(line, sizeof line, "RANK %s  NOT MET YET", RANK_ROMAN[G.best_rank]);
    gfx_text8(f, x0, 57, line, feat ? (uint8_t)(fdone ? R_GOLD + 13 : R_GOLD + 9) : seen || spirit ? (uint8_t)RANK_COL[G.best_rank] : (uint8_t)(R_STONE + 8), 1);
    /* its ways (1.0.0b18): what it does among the others, written in its voice, on Space in place of its lore */
    static const char *const WAYS[NKINDS] = {
        [MOB_SKEL] = "IT KEEPS A POST OR WALKS A ROUND, MARCHES IN THE FILE, AND THROWS ROUND CORNERS.",
        [MOB_GHOST] = "IT DRIFTS TO WHERE THE DEAD LIE IN WAIT, AND HANGS THERE.",
        [MOB_BAT] = "THEY SLEEP IN A ROOST ON THE DIMMEST CEILING. WAKE ONE AND THE REST COME DOWN.",
        [MOB_WSKULL] = "IT SCREAMS WHEN IT WAKES, AND THE DEAD NEARBY COME. SILENCE IT FIRST.",
        [MOB_SHIELD] = "IT LEADS THE FILE OF THE ORDER, AND HOLDS THE ROOM IT IS SET TO HOLD.",
        [MOB_SPIDER] = "ITS WEB IS A TRIPWIRE: WHAT HANGS ABOVE IT DROPS WHEN IT DOES.",
        [MOB_SHAMBLER] = "THE BURIED LIE IN A WAKE. WHEN ONE RISES, THE REST STIR AFTER IT.",
        [MOB_DROWNED] = "IT WAITS UNDER STILL WATER BY A DOOR, AND RISES WITH OTHERS.",
        [MOB_HOUND] = "THEY TAKE TURNS, ONE AT YOUR BACK. BROKEN, ONE RUNS TO FETCH THE OTHERS.",
        [MOB_WRAITH] = "AT ITS WAIL THE DEAD NEARBY HOLD STILL A WHILE, AND LISTEN.",
        [MOB_HUNTER] = "SET LOOSE, IT FOLLOWS YOUR TRAIL ALONG THE CEILING. WATER BREAKS THE SCENT.",
        [MOB_GHOUL] = "IT HEARS A DEATH FAR OFF AND COMES TO FEED, WITH ONE OR TWO MORE.",
        [MOB_KNIGHT] = "HIS HOUNDS CIRCLE UNTIL HE IS CLOSE. FELL HIM AND THEY RUN.",
        [MOB_BROOD] = "TWO HUNTERS GUARD HER, AND GO BACK TO HER WHEN SHE IS HURT.",
        [MOB_LING] = "THEY COME OUT OF HER SACS IN FOURS, AND THEY DO NOT THINK.",
        [MOB_SHADE] = "IT STALKS BEHIND YOU AND STRIKES WHEN YOU STAND STILL. LIGHT FACES IT DOWN.",
        [MOB_IMP] = "A BROOD SPLITS ROUND ITS VENT TO LOB EMBERS, AND FALLS BACK INTO THE HEAT.",
        [MOB_DEACON] = "HE CALLS WHEN HE SEES YOU, AND HIS CONGREGATION COMES, A SHIELD IN FRONT.",
        [MOB_GARGOYLE] = "SENTINELS: WAKE ONE AND EVERY STATUE NEAR IT WAKES.",
        [MOB_WHIPPER] = "HE KEEPS BACK AND SENDS HIS HOUNDS IN ONE BY ONE TO THE CRACK OF THE WHIP.",
        [MOB_SEXTON] = "HE NEVER HUNTS. HIS BELL BRINGS EVERYTHING THAT HEARS IT TO HIS PLACE.",
        [MOB_LIGHTLESS] = "IT ROAMS, QUICK WHEN IT KNOWS, AND PUTS OUT THE WISPS IT REACHES.",
        [MOB_RAT] = "IT ROAMS ALONE. A MOUSE RUNS THROUGH THE WALLS TO TELL IT OF YOU; KILL THE ONE THAT RUNS." };
    const char *ways = met && mind_on() && WAYS[k] ? WAYS[k] : NULL;
    if (!ways) G.best_ways = 0;
    if (feat) {                                                    /* what it asks, and how far along */
        int have = imin(por_count(k), FEATS[fi].need);
        text_wrap(f, x0, 70, FEATS[fi].rule, 23, R_STONE + 12, 2);
        if (fdone) snprintf(line, sizeof line, "EARNED: ITS MEDALLION");
        else snprintf(line, sizeof line, "%d OF %d", have, FEATS[fi].need);
        gfx_text8(f, x0, 92, line, fdone ? (uint8_t)(R_GOLD + 12) : (uint8_t)(R_STONE + 11), 1);
    }
    else if (met && G.best_ways) text_wrap(f, x0, 70, ways, 23, R_BONE + 12, 4);
    else if (met) text_wrap(f, x0, 70, KINDS[k].lore, 23, R_STONE + 12, 4);
    else text_wrap(f, x0, 70, "NOT YET MET. IT IS DOWN THERE SOMEWHERE.", 23, R_STONE + 8, 4);
    if (ways) {                                                    /* which of the two: a pip each at the well's corner */
        for (int j = 0; j < 2; j++) for (int q = 0; q < 4; q++) gfx_px(f, 300 + j * 5 + (q & 1), 39 + (q >> 1), j == G.best_ways ? (uint8_t)(R_GOLD + 12) : UI_M(9));
        const char *h = pk(G.best_ways ? "SPACE: ITS LORE" : "SPACE: ITS WAYS");
        hint8(f, 308 - hint_width(h), 108, h, R_STONE + 9);
    }
    /* slain, rank by rank, against what each achievement asks; its ranks as lozenges at the niche's foot, the one shown lit */
    int rk[NRANK], nr = kind_ranks(k, rk);
    int nl = nr + (fi >= 0);                                       /* and its feat's medallion after them */
    if (!spirit && fi >= 0) medal_glyph(f, 64 - nl * 6 + nr * 12 - 5, 131, feat);
    if (!spirit) for (int j = 0; j < nr; j++) {
        int lx = 64 - nl * 6 + j * 12, on = rk[j] == G.best_rank && !feat, ly = 135;
        for (int d = 0; d < 4; d++) for (int q = -d; q <= d; q++) {
            uint8_t c = on ? (uint8_t)RANK_COL[rk[j]] : UI_W(d == 3 || abs(q) == d ? 0 : 3);
            gfx_px(f, lx + q, ly - 3 + d, c); gfx_px(f, lx + q, ly + 3 - d, c);
        }
    }
    gfx_text8(f, x0, 108, k == MOB_GHOST ? "LAID" : k == MOB_WRAITH ? "DRIVEN" : "SLAIN", R_GOLD + 10, 1);
    if (k == MOB_GHOST || k == MOB_WRAITH) {                       /* the portraits' own: laid to rest, or driven back */
        int w = k == MOB_WRAITH, have = por_count(k);
        for (int j = 0; j < 3 + w; j++) {
            int need = j == 3 ? 1 : w ? WRAITH_STEP[j] : GHOST_STEP[j], got = j == 3 ? ach_has(ACH_BOSS0 + 6 * BOSS_WRAITH) : have;
            int done = j == 3 ? got : ach_has(ACH_POR0 + 3 * w + j);
            snprintf(line, sizeof line, "%-3s%3d/%d", RANK_ROMAN[j], imin(got, 999), need);
            gfx_text8(f, x0 + (j & 1) * 92, 118 + (j / 2) * 9, line, done ? (uint8_t)(R_GOLD + 12) : (uint8_t)(R_STONE + 11), 1);
        }
    }
    else if (spirit) gfx_text8(f, x0 + 48, 108, "IT CANNOT BE", R_STONE + 9, 1);
    else for (int j = 0; j < nr; j++) {
        int need = slayer_need(k, rk[j]), have = r->kills[k][rk[j]], done = ach_has(k * NRANK + rk[j]);
        snprintf(line, sizeof line, "%-3s%3d/%d", rk[j] == RK_GILDED ? "G" : RANK_ROMAN[rk[j]], have < 999 ? have : 999, need);
        int cx = x0 + (j & 1) * 92, cy = 118 + (j / 2) * 9;
        gfx_text8(f, cx, cy, line, done ? (uint8_t)(R_GOLD + 12) : (rk[j] == G.best_rank && !feat ? (uint8_t)C_WHITE : (uint8_t)(R_STONE + 11)), 1);
    }
    /* what hurts it and what it does, in glyphs (docs/PLAN-b11.md, section 2): each element learnt from five
     * kills or twelve hits, each harm from three times it has hurt you */
    gfx_text8(f, 14, 151, "WHAT HURTS IT", R_GOLD + 10, 1);
    if (spirit) { gfx_text8(f, 14, 161, "NOTHING YOU CARRY TOUCHES IT", R_STONE + 11, 1); return; }
    if (elem_mode()) {                                             /* 1.0.0b19: what the rank shown adds, from the lines held */
        int ns = k == MOB_WSKULL || k == MOB_SAC || k == MOB_LING ? 0 : rank_strengths(G.best_rank);
        const char *t = ns == 2 ? "SHRUGS OFF TWO MORE" : ns ? "SHRUGS OFF ONE MORE" : NULL;
        if (t) gfx_text8(f, W - 14 - gfx_text_width(t, 1), 151, t, R_STONE + 11, 1);
    } else if (k != MOB_WSKULL && k != MOB_SAC && k != MOB_LING && (RUN.level >= 5 || crawl_loop(RUN.level) > 0))   /* 1.0.0b16 */
        gfx_text8(f, W - 14 - gfx_text_width("SOME COME WARDED", 1), 151, "SOME COME WARDED", R_STONE + 11, 1);
    static const char *const ROW[4] = { "MORE", "LESS", "AS USUAL", "NOT LEARNT" };
    static const uint8_t ROWC[4] = { R_MOSS + 13, R_BLOOD + 13, R_STONE + 12, R_STONE + 8 };
    int at[4] = { 0 };
    for (int row = 0; row < 4; row++) gfx_text8(f, 14 + (row & 1) * 150, 161 + (row >> 1) * 10, ROW[row], ROWC[row], 1);
    for (int e = 0; e < NELEM; e++) {
        int g = weak_grade(kind_weak(k, e)), row = !el_learnt(k, e) ? 3 : g > 0 ? 0 : g < 0 ? 1 : 2;
        int x = 14 + (row & 1) * 150 + gfx_text_width(ROW[row], 1) + 6 + at[row], y = 161 + (row >> 1) * 10;
        if (row == 3) elem_glyph(f, x, y, e, 1);
        else glyph_marked(f, x, y, e, g);
        at[row] += (row == 3 ? 7 : glyph_width(g)) + 4;
    }
    int harms = kind_harms(k, G.best_rank), x = 14 + gfx_text_width("WHAT IT DOES", 1) + 6;
    gfx_text8(f, 14, 181, "WHAT IT DOES", R_GOLD + 10, 1);
    if (!harms) gfx_text8(f, x, 181, "NOTHING OF ITS OWN", R_STONE + 11, 1);
    for (int e = 0; e < NELEM; e++) {
        if (!(harms >> e & 1)) continue;
        if (harm_learnt(k, e)) { elem_glyph(f, x, 181, e, 0); gfx_text8(f, x + 10, 181, ELEM_NAME[e], ramp_at(ELEM_RAMP[e], 13), 1); x += 14 + gfx_text_width(ELEM_NAME[e], 1); }
        else { gfx_text8(f, x, 181, "?", R_STONE + 8, 1); x += 14; }
    }
}

