/* A part of scene_crawl.c (docs/PLAN-tidy.md): it is #included there in its place and never compiled on its own.
 * More menus' logic: the Compendium, the Repository, the Codex, the Totem, the Portal; the map's keys; update_ui. */

/* ---- the Corpus Compendium (docs/PLAN-b7.md, section 7): each piece as it was first put on (never
 * anointed), kept once however often it comes again, twelve of a kind at a quality, the weakest going
 * for a thirteenth; and each spell at the highest tier learnt. Kept through death. Sold back at the
 * shop's price x1.2, five hundredths less for each run completed, never under half. ---- */

enum { CP_WRIST, CP_BOOTS, CP_NECK, CP_HOOD, CP_ROBE, CP_RING, CP_BAG, NCP };   /* its spells page went with the spells as wares (1.0.0b11) */
static const int CP_KIND[NCP] = { IK_WRIST, IK_BOOTS, IK_NECK, IK_HOOD, IK_ROBE, IK_RING, IK_BAG };
static const char *const CP_NAME[NCP] = { "BRACERS", "BOOTS", "AMULETS", "HOODS", "ROBES", "RINGS", "BAGS" };
static int piece_alike(const Piece *a, const Piece *b) {
    if (a->kind != b->kind || a->quality != b->quality || a->ability != b->ability || a->nstat != b->nstat) return 0;
    for (int i = 0; i < a->nstat; i++) if (a->stat[i] != b->stat[i] || a->val[i] != b->val[i]) return 0;
    return 1;
}
static void comp_add(const Piece *p) {
    if (p->kind == IK_NONE || p->kind >= NIK || p->quality >= NQUAL || p->anointed) return;
    uint8_t *n = &RUN.ncomp[p->kind][p->quality];
    Piece *list = RUN.comp[p->kind][p->quality];
    for (int i = 0; i < *n; i++) if (piece_alike(&list[i], p)) return;
    if (*n < COMP_MAX) { list[(*n)++] = *p; return; }
    int weak = 0;
    for (int i = 1; i < *n; i++) if (piece_price(&list[i]) < piece_price(&list[weak])) weak = i;
    if (piece_price(p) > piece_price(&list[weak])) list[weak] = *p;
}
static void codex_note(void);
static void comp_note(int level) {                                 /* what is worn and known, into the index */
    const Gear *g = &RUN.gear;
    codex_note();
    for (int s = 0; s < NSLOT; s++) comp_add(&g->worn[s]);
    if (g->bag_size) { Piece b; memset(&b, 0, sizeof b); b.kind = IK_BAG; b.quality = g->bag_quality; b.level = (uint8_t)clampi(level, 1, 255); comp_add(&b); }
}
typedef struct { int q, i, k; } CompRow;
static int comp_rows(int page, CompRow *r) {                       /* a page's entries, Common first */
    int n = 0;
    int kind = CP_KIND[page];
    for (int q = 0; q < NQUAL; q++) for (int i = 0; i < RUN.ncomp[kind][q]; i++) r[n++] = (CompRow){ q, i, -1 };
    return n;
}
static int comp_count(void) { int n = 0; for (int k = 0; k < NIK; k++) for (int q = 0; q < NQUAL; q++) n += RUN.ncomp[k][q]; return n; }
static float comp_factor(void) { return fmaxf(0.5f, 1.2f - 0.05f * (float)RUN.runs_done); }
static int comp_round(float v) { return ((int)(v + 2.5f) / 5) * 5; }
static int comp_price(int page, const CompRow *r) {
    return comp_round((float)piece_price(&RUN.comp[CP_KIND[page]][r->q][r->i]) * comp_factor());
}
static void comp_buy(int page, const CompRow *r) {
    int price = comp_price(page, r);
    if (price > RUN.treasure) { shrine_note("IT WANTS $%d", price); snd_sfx_ex(SFX_KNOCK, 0.5f, 0.5f); return; }
    {
        const Piece *p = &RUN.comp[CP_KIND[page]][r->q][r->i];
        if (p->kind == IK_BAG && bag_capacity(p->quality) <= RUN.gear.bag_size) { shrine_note("YOUR BAG HOLDS AS MUCH"); snd_sfx_ex(SFX_KNOCK, 0.5f, 0.5f); return; }
        int sl = p->kind == IK_BAG ? -1 : piece_slot(&RUN.gear, p->kind), key = page * 64 + r->q * COMP_MAX + r->i;
        if (sl >= 0 && RUN.gear.worn[sl].kind != IK_NONE && !gear_bag_free(&RUN.gear) && G.comp_arm != key) {   /* nowhere for the old one */
            G.comp_arm = key; shrine_note("ENTER AGAIN: THE ONE YOU WEAR IS LOST"); snd_sfx_ex(SFX_KNOCK, 0.7f, 0.5f);
            return;
        }
        Offer o; memset(&o, 0, sizeof o);
        o.kind = GK_PIECE; o.id = p->kind; o.tier = p->level; o.price = price; o.piece = *p;
        take_piece(&o, 1);
        shrine_note(p->kind == IK_BAG ? "THE BAG IS YOURS AGAIN" : "IT IS ON YOU AGAIN");
    }
    G.comp_arm = -1;
    RUN.treasure -= price; G.shop_gold = RUN.treasure;
    snd_sfx_ex(SFX_COIN, 1.0f, 0.5f); snd_sfx_ex(SFX_SHIMMER, 0.7f, 0.5f);
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT comp_buy page %d row q%d i%d k%d for %d (left %d) t=%.2f\n", page, r->q, r->i, r->k, price, RUN.treasure, G.t);
#endif
}
/* the autopilot: one thing from the Compendium a visit, when it beats what it has by more than the pedestals
 * did, with a little of a level's haul kept back; never a bag */
static void comp_auto(void) {
    if (G.player) return;
    int purse = RUN.treasure - (int)(gear_haul(G.level) * 0.15f), bp = -1;
    float best = 0.0f;
    CompRow br = { 0, 0, -1 }, r[NQUAL * COMP_MAX + NATK];
    for (int page = 0; page < NCP; page++) {
        if (page == CP_BAG) continue;
        int n = comp_rows(page, r);
        for (int j = 0; j < n; j++) {
            int price = comp_price(page, &r[j]);
            if (price > purse) continue;
            Offer o; memset(&o, 0, sizeof o);
            o.kind = GK_PIECE; o.piece = RUN.comp[CP_KIND[page]][r[j].q][r[j].i]; o.id = o.piece.kind; o.tier = o.piece.level;
            float v = gear_value(&RUN.gear, &o, G.level) / sqrtf(fmaxf(1.0f, (float)price) / 100.0f + 1.0f);
            if (v > best) { best = v; bp = page; br = r[j]; }
        }
    }
#ifdef CRAWL_DEBUG
    fprintf(stderr, "COMP auto best page %d value %.3f purse %d\n", bp, best, purse);
#endif
    if (bp < 0 || best < 0.1f) return;                           /* worth its gold, roughly: a tier of a spell is */
    int price = comp_price(bp, &br);
    Offer o; memset(&o, 0, sizeof o); o.price = price;
    char nm[64];
    o.kind = GK_PIECE; o.piece = RUN.comp[CP_KIND[bp]][br.q][br.i]; o.id = o.piece.kind; o.tier = o.piece.level; take_piece(&o, 0); piece_full_name(&o.piece, nm, sizeof nm);
    RUN.treasure -= price; G.shop_gold = RUN.treasure;
    say("FROM THE COMPENDIUM: %s", nm);
    snd_sfx_ex(SFX_COIN, 0.9f, 0.5f);
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT comp_auto %s for %d value %.2f (left %d) t=%.2f\n", nm, price, best, RUN.treasure, G.t);
#endif
}

/* A/D the pages, W/S the entries, Enter buys and wears it, Esc closes */
static void update_compmenu(void) {
    if (IN.hit[KEY_ESC] || IN.hit[KEY_TAB]) { G.ui = UI_NONE; return; }
    int lr = (IN.hit[KEY_D] || IN.hit[KEY_RIGHT]) - (IN.hit[KEY_A] || IN.hit[KEY_LEFT]);
    if (lr) { G.comp_page = (G.comp_page + NCP + lr) % NCP; G.ui_sel = 0; G.comp_arm = -1; G.sh_note_t = 0.0f; snd_sfx_ex(SFX_TICK, 0.8f, 0.5f); }
    CompRow r[NQUAL * COMP_MAX + NATK];
    int n = comp_rows(G.comp_page, r);
    if (n <= 0) { G.ui_sel = 0; return; }
    int ud = (IN.hit[KEY_S] || IN.hit[KEY_DOWN]) - (IN.hit[KEY_W] || IN.hit[KEY_UP]);
    if (ud) { G.ui_sel = (G.ui_sel + n + ud) % n; G.comp_arm = -1; snd_sfx_ex(SFX_TICK, 0.6f, 0.5f); }
    G.ui_sel = clampi(G.ui_sel, 0, n - 1);
    if (IN.hit[KEY_ENTER] || IN.hit[KEY_E] || IN.hit[KEY_SPACE] || IN.hit[KEY_MOUSE1]) comp_buy(G.comp_page, &r[G.ui_sel]);
}

/* ---- the Wretched Repository (docs/PLAN-b7.md, section 7): places for pieces, and gold and Cursed Coins
 * without limit, none of it lost to death or her claim. A piece goes in for a tenth of its shop price (the
 * first place takes nothing); gold goes in less a quarter; coins go in free and come out for 2,500 gold
 * each. Two places at first, ten more to buy. People only: the autopilot leaves it be. ---- */

static const int PLACE_PRICE[REPO_MAX - 2] = { 2500, 3750, 5625, 8440, 12655, 18985, 28475, 42715, 64070, 96110 };
#define COIN_OUT 2500
enum { RR_PLACE, RR_BUY, RR_GOLD_IN, RR_GOLD_OUT, RR_COIN_IN, RR_COIN_OUT };
typedef struct { int kind, i; } RepRow;                            /* a place, or one of the money's rows */
static int repo_rows(RepRow *r) {
    int n = 0;
    for (int i = 0; i < RUN.repo_n; i++) r[n++] = (RepRow){ RR_PLACE, i };
    if (RUN.repo_n < REPO_MAX) r[n++] = (RepRow){ RR_BUY, 0 };
    for (int k = RR_GOLD_IN; k <= RR_COIN_OUT; k++) r[n++] = (RepRow){ k, 0 };
    return n;
}
static Piece bag_piece(void) {                                     /* the bag you carry, as a piece */
    Piece b; memset(&b, 0, sizeof b);
    b.kind = IK_BAG; b.quality = RUN.gear.bag_quality; b.level = (uint8_t)clampi(G.level, 1, 255);
    return b;
}
typedef struct { int slot, bag; } RepFrom;                         /* what can go in: a worn piece, a spare, or the empty bag itself */
static int repo_froms(RepFrom *r) {
    const Gear *g = &RUN.gear;
    int n = 0, spares = 0;
    for (int s = 0; s < NSLOT; s++) if (g->worn[s].kind != IK_NONE) r[n++] = (RepFrom){ s, -1 };
    for (int b = 0; b < g->bag_size; b++) if (g->bag[b].kind != IK_NONE) { r[n++] = (RepFrom){ -1, b }; spares++; }
    if (g->bag_size && !spares) r[n++] = (RepFrom){ -1, -1 };
    return n;
}
static Piece repo_from_piece(const RepFrom *f) { return f->slot >= 0 ? RUN.gear.worn[f->slot] : (f->bag >= 0 ? RUN.gear.bag[f->bag] : bag_piece()); }
static int repo_tax(int place, const Piece *p) { return place == 0 ? 0 : (piece_price(p) / 10 + 4) / 5 * 5; }
static int gold_amount(int src) {                                  /* all of it, half, a quarter, a tenth */
    static const int DIV[4] = { 1, 2, 4, 10 };
    return src <= 0 ? 0 : imax(1, src / DIV[clampi(G.rep_amt, 0, 3)]);
}
static void repo_put(int place, const RepFrom *f) {
    Piece p = repo_from_piece(f);
    int tax = repo_tax(place, &p);
    if (tax > RUN.treasure) { shrine_note("IT WANTS $%d TO TAKE IT", tax); snd_sfx_ex(SFX_KNOCK, 0.5f, 0.5f); return; }
    RUN.treasure -= tax;
    if (f->slot >= 0) memset(&RUN.gear.worn[f->slot], 0, sizeof(Piece));
    else if (f->bag >= 0) memset(&RUN.gear.bag[f->bag], 0, sizeof(Piece));
    else { RUN.gear.bag_size = RUN.gear.bag_quality = 0; memset(RUN.gear.bag, 0, sizeof RUN.gear.bag); }
    RUN.repo[place] = p;
    G.max_hp = gear_max_hp(&RUN.gear); if (G.hp > G.max_hp) G.hp = G.max_hp;
    G.rep_mode = 0;
    shrine_note(tax ? "SHUT AWAY, FOR $%d" : "SHUT AWAY, UNTAXED", tax);
    snd_sfx_ex(SFX_UNLOCK, 0.8f, 0.5f);
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT repo_put place %d kind %d tax %d t=%.2f\n", place, p.kind, tax, G.t);
#endif
}
static void repo_take(int place) {
    Piece *p = &RUN.repo[place];
    Gear *g = &RUN.gear;
    if (p->kind == IK_BAG) {
        if (bag_capacity(p->quality) <= g->bag_size) { shrine_note("YOUR BAG HOLDS AS MUCH"); snd_sfx_ex(SFX_KNOCK, 0.5f, 0.5f); return; }
        Offer o; memset(&o, 0, sizeof o); o.kind = GK_PIECE; o.id = IK_BAG; o.piece = *p;
        gear_take(g, &o);
    } else {
        int sl = piece_slot(g, p->kind), b = -1;
        for (int i = 0; i < g->bag_size && b < 0; i++) if (g->bag[i].kind == IK_NONE) b = i;
        if (g->worn[sl].kind == IK_NONE) g->worn[sl] = *p;
        else if (b >= 0) g->bag[b] = *p;
        else { shrine_note("NOWHERE TO PUT IT: WEAR LESS, OR BAG IT"); snd_sfx_ex(SFX_KNOCK, 0.5f, 0.5f); return; }
    }
    memset(p, 0, sizeof *p);
    G.max_hp = gear_max_hp(g);
    shrine_note("TAKEN OUT");
    snd_sfx_ex(SFX_CREAK, 0.7f, 0.5f);
}
static void update_repomenu(void) {
    if (IN.hit[KEY_ESC] || IN.hit[KEY_TAB]) { if (G.rep_mode) { G.rep_mode = 0; snd_sfx_ex(SFX_KEY, 0.6f, 0.5f); } else G.ui = UI_NONE; return; }
    int ud = (IN.hit[KEY_S] || IN.hit[KEY_DOWN]) - (IN.hit[KEY_W] || IN.hit[KEY_UP]);
    int enter = IN.hit[KEY_ENTER] || IN.hit[KEY_E] || IN.hit[KEY_SPACE] || IN.hit[KEY_MOUSE1];
    if (G.rep_mode) {                                              /* which piece goes in */
        RepFrom fr[NSLOT + BAG_MAX + 1];
        int n = repo_froms(fr);
        if (!n) { G.rep_mode = 0; return; }
        if (ud) { G.ui_sel = (G.ui_sel + n + ud) % n; snd_sfx_ex(SFX_TICK, 0.6f, 0.5f); }
        G.ui_sel = clampi(G.ui_sel, 0, n - 1);
        if (enter) { repo_put(G.rep_place, &fr[G.ui_sel]); if (!G.rep_mode) G.ui_sel = G.rep_place; }
        return;
    }
    RepRow r[REPO_MAX + 6];
    int n = repo_rows(r);
    if (ud) { G.ui_sel = (G.ui_sel + n + ud) % n; snd_sfx_ex(SFX_TICK, 0.6f, 0.5f); }
    G.ui_sel = clampi(G.ui_sel, 0, n - 1);
    const RepRow *c = &r[G.ui_sel];
    int lr = (IN.hit[KEY_D] || IN.hit[KEY_RIGHT]) - (IN.hit[KEY_A] || IN.hit[KEY_LEFT]);
    if (lr && (c->kind == RR_GOLD_IN || c->kind == RR_GOLD_OUT)) { G.rep_amt = (G.rep_amt + 4 + lr) % 4; snd_sfx_ex(SFX_TICK, 0.6f, 0.5f); }
    if (!enter) return;
    switch (c->kind) {
    case RR_PLACE:
        if (RUN.repo[c->i].kind != IK_NONE) repo_take(c->i);
        else {
            RepFrom fr[NSLOT + BAG_MAX + 1];
            if (!repo_froms(fr)) { shrine_note("YOU HAVE NOTHING TO SHUT AWAY"); snd_sfx_ex(SFX_KNOCK, 0.5f, 0.5f); }
            else { G.rep_mode = 1; G.rep_place = c->i; G.ui_sel = 0; snd_sfx_ex(SFX_KEY, 0.6f, 0.5f); }
        }
        break;
    case RR_BUY: {
        int price = PLACE_PRICE[clampi(RUN.repo_n - 2, 0, REPO_MAX - 3)];
        if (price > RUN.treasure) { shrine_note("ANOTHER PLACE IS $%d", price); snd_sfx_ex(SFX_KNOCK, 0.5f, 0.5f); break; }
        RUN.treasure -= price; RUN.repo_n++;
        shrine_note("ANOTHER PLACE, FOR $%d", price); snd_sfx_ex(SFX_COIN, 1.0f, 0.5f); snd_sfx_ex(SFX_UNLOCK, 0.6f, 0.5f);
        break;
    }
    case RR_GOLD_IN: {
        int a = gold_amount(RUN.treasure), kept = a - a / 4;
        if (a <= 0) { shrine_note("YOU CARRY NO GOLD"); snd_sfx_ex(SFX_KNOCK, 0.5f, 0.5f); break; }
        RUN.treasure -= a; RUN.repo_gold += kept;
        shrine_note("$%d IN; $%d KEPT", a, kept); snd_sfx_ex(SFX_COIN, 1.0f, 0.5f);
        break;
    }
    case RR_GOLD_OUT: {
        int a = gold_amount(RUN.repo_gold);
        if (a <= 0) { shrine_note("THERE IS NO GOLD IN IT"); snd_sfx_ex(SFX_KNOCK, 0.5f, 0.5f); break; }
        RUN.repo_gold -= a; RUN.treasure += a;
        shrine_note("$%d OUT", a); snd_sfx_ex(SFX_COIN, 1.0f, 0.5f);
        break;
    }
    case RR_COIN_IN:
        if (RUN.coins <= 0) { shrine_note("YOU CARRY NO CURSED COINS"); snd_sfx_ex(SFX_KNOCK, 0.5f, 0.5f); break; }
        RUN.coins--; RUN.repo_coins++;
        shrine_note("A CURSED COIN IN"); snd_sfx_ex(SFX_COIN, 0.8f, 0.5f);
        break;
    case RR_COIN_OUT:
        if (RUN.repo_coins <= 0) { shrine_note("THERE ARE NO COINS IN IT"); snd_sfx_ex(SFX_KNOCK, 0.5f, 0.5f); break; }
        if (RUN.treasure < COIN_OUT) { shrine_note("EACH COMES OUT FOR $%d", COIN_OUT); snd_sfx_ex(SFX_KNOCK, 0.5f, 0.5f); break; }
        RUN.treasure -= COIN_OUT; RUN.repo_coins--; RUN.coins++;
        shrine_note("A CURSED COIN OUT, FOR $%d", COIN_OUT); snd_sfx_ex(SFX_COIN, 0.8f, 0.5f);
        break;
    }
    G.shop_gold = RUN.treasure;
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT repo row %d: carry $%d %d coins, in it $%d %d coins, %d places t=%.2f\n", c->kind, RUN.treasure, RUN.coins, RUN.repo_gold, RUN.repo_coins, RUN.repo_n, G.t);
#endif
}

/* ---- the Codex of Affixations (docs/PLAN-b7.md, section 9): what every ability, rune, boss's gift, stat
 * and hazard does, each written in once you have worn, held, unlocked or met it; and a page on her claim,
 * there from the start. Kept through death, with the page. C opens it; the crypt waits. ---- */

enum { CX_ABIL, CX_RUNE, CX_BOSS, CX_STAT, CX_HAZ, CX_PUZZLE, CX_TRIAL, CX_WAYS, CX_CASTER, CX_ELEM, CX_LINE, CX_CLASS, CX_SOUL, CX_CLAIM,
       /* docs/PLAN-combat.md, two tracks on one branch (its section 17): each adds parts only above its own end mark */
       CX_AUG,                                                    /* an augment chosen (C6, 1.0.0b21): i is its line x NAUGT + its step */
       /* the caster's track (C) ends */
       /* the dead's track (D): */
       /* the dead's track (D) ends */
       NCX };
enum { CXS_VESSEL, CXS_BELL, CXS_FONT, CXS_REPLAY, CXS_SAP, NCXS };   /* the souls' pages */
#define CX_MAX (NABIL + NDEF + NBOSS + NSTAT + NHZ + NPZK + NTRI + NWAY + 3 + NCS + 2 + NELEM + 1 + NCXS \
    /* docs/PLAN-combat.md section 17: each track adds its new pages' count (+ n) only above its own end mark */ \
    + NELEM * NAUGT /* the augments chosen (C6) */ \
    /* the caster's track (C) ends */ \
    /* the dead's track (D): */ \
    + 1 /* the elements' second page, the dead's (D1) */ \
    + 1 /* the class's second page, what they bring (D5) */ \
    /* the dead's track (D) ends */ \
    )
enum { CXC_ATONE, CXC_LUCK, CXC_OVER, CXC_CORNER };                /* the caster's own pages: then a cornerstone each, CXC_CORNER + CS_* */
typedef struct { int sec, i; } CxEntry;
static const char *const CX_PART[NCX] = { "AN ABILITY OF WHAT YOU WEAR", "A RUNE", "A BOSS'S GIFT", "A STAT A PIECE CAN ROLL", "A HAZARD", "A PUZZLE SOLVED",
                                          "A TRIAL WON", "THE WAYS OF THE DEAD", "THE CASTER'S OWN",
                                          "THE ELEMENTS", "A LINE OF THE ALIGNMENT", "WHAT YOU WERE", "SOULS", "THE MERCHANT'S LEDGER",
                                          "AN AUGMENT OF A LINE" };
static void codex_note(void) {                                     /* worn, held or met: written in */
    const Gear *g = &RUN.gear;
    uint32_t ab = RUN.cx_abil; uint64_t st = RUN.cx_stat;
    for (int s = 0; s < NSLOT; s++) {
        const Piece *p = &g->worn[s];
        if (p->kind == IK_NONE) continue;
        if (p->ability) RUN.cx_abil |= 1u << p->ability;
        for (int i = 0; i < p->nstat; i++) RUN.cx_stat |= 1ull << p->stat[i];
    }
    for (int k = 0; k < NDEF; k++) {
        if (g->def[k] > 0) RUN.cx_rune |= 1u << k;
        if (g->def[k] >= RUNE_EXQ) RUN.cx_exq |= 1u << k;
    }
    if (G.player && RUN.cx_abil != ab) for (int a = 1; a < NABIL; a++) if ((RUN.cx_abil & ~ab) >> a & 1) say("WRITTEN IN THE CODEX: %s", ABIL_NAME[a]);
    if (G.player && RUN.cx_stat != st && RUN.cx_abil == ab) say("WRITTEN IN THE CODEX. C TO READ IT");
}
static int codex_entries(CxEntry *e) {
    int n = 0;
    for (int a = 1; a < NABIL; a++) if (RUN.cx_abil >> a & 1) e[n++] = (CxEntry){ CX_ABIL, a };
    for (int k = 0; k < NDEF; k++) if (RUN.cx_rune >> k & 1) e[n++] = (CxEntry){ CX_RUNE, k };
    for (int b = 0; b < NBOSS; b++) if (boss_beaten(b)) e[n++] = (CxEntry){ CX_BOSS, b };
    for (int t = 0; t < NSTAT; t++) if (RUN.cx_stat >> t & 1) e[n++] = (CxEntry){ CX_STAT, t };
    for (int h = 0; h < NHZ; h++) if (RUN.omens >> h & 1) e[n++] = (CxEntry){ CX_HAZ, h };
    for (int k = 0; k < NPZK; k++) if (RUN.pz_kinds >> k & 1) e[n++] = (CxEntry){ CX_PUZZLE, k };   /* 1.0.0b13 */
    for (int k = 0; k < NTRI; k++) if (RUN.tri_kinds >> k & 1) e[n++] = (CxEntry){ CX_TRIAL, k };
    for (int w = 0; w < NWAY; w++) if (RUN.ways >> w & 1) e[n++] = (CxEntry){ CX_WAYS, w };   /* 1.0.0b18 */
    e[n++] = (CxEntry){ CX_CASTER, CXC_ATONE };
    e[n++] = (CxEntry){ CX_CASTER, CXC_LUCK };
    if (RUN.atone >= ATONE_TOP) e[n++] = (CxEntry){ CX_CASTER, CXC_OVER };
    for (int c = 0; c < NCS; c++) if (RUN.corner >> c & 1) e[n++] = (CxEntry){ CX_CASTER, CXC_CORNER + c };
    e[n++] = (CxEntry){ CX_ELEM, 0 };
    if (elem_mode()) e[n++] = (CxEntry){ CX_ELEM, 1 };             /* the dead's elements (1.0.0b19) */
    for (int el = 0; el < NELEM; el++) if (align_open(el)) e[n++] = (CxEntry){ CX_LINE, el };   /* the lines open */
    if (cls_el() >= 0) { e[n++] = (CxEntry){ CX_CLASS, cls_el() }; e[n++] = (CxEntry){ CX_CLASS, NELEM + cls_el() }; }   /* what you were, what they bring */
    e[n++] = (CxEntry){ CX_SOUL, CXS_VESSEL };                     /* the souls, as each is met */
    if (RUN.bell_cursed) e[n++] = (CxEntry){ CX_SOUL, CXS_BELL };
    if (RUN.font_cursed) e[n++] = (CxEntry){ CX_SOUL, CXS_FONT };
    if (RUN.rec.cleared) e[n++] = (CxEntry){ CX_SOUL, CXS_REPLAY };
    if (RUN.cx_more & CXM_SAP) e[n++] = (CxEntry){ CX_SOUL, CXS_SAP };
    e[n++] = (CxEntry){ CX_CLAIM, 0 };
    for (int el = 0; el < NELEM; el++)                             /* the augments chosen (C6), each line's in step order */
        for (int t = 0; t < NAUGT; t++) if (aug_of(el, t)) e[n++] = (CxEntry){ CX_AUG, el * NAUGT + t };
    return n;
}
/* A/D a page, W/S the next part, C or Esc closes */
static void update_codex(void) {
    if (IN.hit[KEY_C] || IN.hit[KEY_ESC] || IN.hit[KEY_TAB]) { G.ui = UI_NONE; snd_sfx_ex(SFX_KEY, 0.6f, 0.5f); return; }
    CxEntry e[CX_MAX];
    int n = codex_entries(e), lr = (IN.hit[KEY_D] || IN.hit[KEY_RIGHT]) - (IN.hit[KEY_A] || IN.hit[KEY_LEFT]);
    int ud = (IN.hit[KEY_S] || IN.hit[KEY_DOWN]) - (IN.hit[KEY_W] || IN.hit[KEY_UP]);
    G.cx_page = clampi(G.cx_page, 0, n - 1);
    if (lr) { G.cx_page = (G.cx_page + n + lr) % n; snd_sfx_ex(SFX_TICK, 0.8f, 0.5f); }
    if (ud) {                                                      /* the first page of the next part, or of this one and the last */
        int sec = e[G.cx_page].sec, p = G.cx_page;
        if (ud > 0) { while (p < n && e[p].sec == sec) p++; if (p >= n) p = 0; }
        else { while (p > 0 && e[p - 1].sec == sec) p--; if (p == G.cx_page) { p = p > 0 ? p - 1 : n - 1; sec = e[p].sec; while (p > 0 && e[p - 1].sec == sec) p--; } }
        G.cx_page = p; snd_sfx_ex(SFX_TICK, 0.6f, 0.5f);
    }
}

static void update_totemmenu(void) {
    int r[NBOSS], n = totem_rows(r);
    int enter = IN.hit[KEY_ENTER] || IN.hit[KEY_E] || IN.hit[KEY_SPACE] || IN.hit[KEY_MOUSE1];
    if (IN.hit[KEY_ESC] || IN.hit[KEY_TAB]) { G.ui = UI_NONE; return; }
    if (!n) { if (enter) G.ui = UI_NONE; return; }
    int up = IN.hit[KEY_W] || IN.hit[KEY_UP], down = IN.hit[KEY_S] || IN.hit[KEY_DOWN];
    int side = (IN.hit[KEY_A] || IN.hit[KEY_LEFT]) || (IN.hit[KEY_D] || IN.hit[KEY_RIGHT]);
    if (up || down) { G.ui_sel = (G.ui_sel + n + down - up) % n; G.tot_arm = -1; snd_sfx_ex(SFX_TICK, 0.6f, 0.5f); }
    G.ui_sel = clampi(G.ui_sel, 0, n - 1);
    int b = r[G.ui_sel];
    if (side && totem_extra(b) > 0) { G.tot_col ^= 1; G.tot_arm = -1; snd_sfx_ex(SFX_TICK, 0.6f, 0.5f); }
    if (totem_extra(b) <= 0) G.tot_col = 0;
    if (!enter) return;
    if (G.tot_col) {
        if (G.tot_arm != b) { G.tot_arm = b; shrine_note("ENTER AGAIN SELLS ONE FOR $%d", SCRIP_PRICE); snd_sfx_ex(SFX_KNOCK, 0.7f, 0.5f); return; }
        totem_sell(b); G.tot_arm = -1;
        return;
    }
    G.tot_arm = -1;
    if (RUN.abil[b] >= ABIL_TOP) { shrine_note("IT CAN GO NO HIGHER"); snd_sfx_ex(SFX_KNOCK, 0.5f, 0.5f); return; }
    int cost = RUN.abil[b] + 1;
    if (RUN.scrip[b] < cost) { shrine_note("IT WANTS %d SCRIP OF %s", cost, BOSSES[b].short_name); snd_sfx_ex(SFX_KNOCK, 0.5f, 0.5f); return; }
    totem_raise(b);
}

/* the Portal: W/S a run or a challenge, Enter goes, Esc closes */
static void update_portalmenu(void) {
    PortRow r[16];
    int n = portal_rows(r);
    if (IN.hit[KEY_ESC] || IN.hit[KEY_TAB]) { G.ui = UI_NONE; return; }
    int up = IN.hit[KEY_W] || IN.hit[KEY_UP], down = IN.hit[KEY_S] || IN.hit[KEY_DOWN];
    if (up || down) { G.ui_sel = (G.ui_sel + n + down - up) % n; snd_sfx_ex(SFX_TICK, 0.6f, 0.5f); }
    G.ui_sel = clampi(G.ui_sel, 0, n - 1);
    int lr = (IN.hit[KEY_D] || IN.hit[KEY_RIGHT]) - (IN.hit[KEY_A] || IN.hit[KEY_LEFT]);
    if (r[G.ui_sel].run < 0 && r[G.ui_sel].boss < 0) {             /* the level to replay */
        if (G.port_lvl < 1 || !(RUN.rec.cleared >> (G.port_lvl - 1) & 1)) G.port_lvl = replay_step(1, -1);
        if (lr) { G.port_lvl = replay_step(G.port_lvl, lr); snd_sfx_ex(SFX_TICK, 0.8f, 0.5f); }
    }
    if (!(IN.hit[KEY_ENTER] || IN.hit[KEY_E] || IN.hit[KEY_SPACE] || IN.hit[KEY_MOUSE1])) return;
    const PortRow *p = &r[G.ui_sel];
    if (p->run < 0 && p->boss < 0) {                               /* the levels cleared: A/D choose, Enter replays */
        if (!(RUN.rec.cleared >> (G.port_lvl - 1) & 1) || G.port_lvl < 1) G.port_lvl = replay_step(1, -1);
        if (spirit() < (float)replay_price(G.port_lvl)) { snd_sfx_ex(SFX_KNOCK, 0.5f, 0.5f); return; }
        replay_begin(G.port_lvl);
        return;
    }
    if (p->run >= 0) {
        if (p->run == crawl_run(G.level) && crawl_step(G.level) == 0) { G.ui = UI_NONE; say("YOU ARE AT ITS START ALREADY"); return; }
        portal_run(p->run);
        return;
    }
    RUN.chal_from = G.level; RUN.hp = G.hp;
    if (RUN.gear.has_held && G.shop_held < 0) RUN.gear.has_held = 0;   /* as on leaving the room by its door */
    snd_sfx_ex(SFX_WHOOSH, 1.0f, 0.5f); snd_sfx_ex(SFX_STINGER, 0.6f, 0.5f);
    challenge_begin(p->boss);
}

/* The map open (docs/PLAN-b9.md, section 1): W/A/S/D or the arrows move it, a cell a tap and smoothly
 * when held; the left mouse drags it and the left stick moves it; Space or Enter brings it back to you;
 * M or Esc rolls it up. It never goes past what the level holds. */
#define MAP_PX 5                            /* pixels a cell on the scroll */
#define MAP_VX0 44
#define MAP_VY0 24
#define MAP_VW 232
#define MAP_VH 132                          /* the part of the scroll the map is drawn in; the level's numbers below */
static void map_clamp(void) {
    int x0 = MAPN, y0 = MAPN, x1 = -1, y1 = -1;
    for (int y = 0; y < MAPN; y++)
        for (int x = 0; x < MAPN; x++)
            if (G.map[y][x].kind != K_WALL) { x0 = imin(x0, x); y0 = imin(y0, y); x1 = imax(x1, x); y1 = imax(y1, y); }
    if (x1 < 0) { G.map_cx = G.map_cy = MAPN * 0.5f; return; }
    float hw = MAP_VW * 0.5f / MAP_PX, hh = MAP_VH * 0.5f / MAP_PX;
    float ax = x0 - 1.0f, bx = x1 + 2.0f, ay = y0 - 1.0f, by = y1 + 2.0f;   /* a cell of stone round it */
    G.map_cx = bx - ax <= 2.0f * hw ? (ax + bx) * 0.5f : clampf(G.map_cx, ax + hw, bx - hw);
    G.map_cy = by - ay <= 2.0f * hh ? (ay + by) * 0.5f : clampf(G.map_cy, ay + hh, by - hh);
}
/* The map up as you walk (docs/PLAN-b11.md, section 1): the arrow keys pan it, a cell a tap and smoothly while
 * held (the pad's D-pad a cell a press); three seconds after the last pan it drifts back to you, or at once on
 * Space; M, Esc or Tab close it (B on the pad). W/A/S/D and the mouse play on. */
static void map_up(void) {
    G.map_on = 1; G.map_cx = G.px; G.map_cy = G.py; G.map_hold = 0.0f; G.map_pan_t = 99.0f;
    map_clamp();
    snd_sfx_ex(SFX_WHOOSH, 0.35f, 0.5f); snd_sfx_ex(SFX_CREAK, 0.2f, 0.5f);
}
static void map_keys(float dt) {
    if (IN.hit[KEY_ESC] || IN.hit[KEY_M] || IN.hit[KEY_TAB]) {
        G.map_on = 0; snd_sfx_ex(SFX_WHOOSH, 0.3f, 0.5f);
        IN.hit[KEY_ESC] = IN.hit[KEY_M] = IN.hit[KEY_TAB] = 0;      /* only the map goes */
        return;
    }
    if (IN.hit[KEY_SPACE]) { G.map_pan_t = 99.0f; G.map_cx = G.px; G.map_cy = G.py; IN.hit[KEY_SPACE] = 0; snd_sfx_ex(SFX_TICK, 0.6f, 0.5f); }
    int hx = IN.hit[KEY_RIGHT] - IN.hit[KEY_LEFT], hy = IN.hit[KEY_DOWN] - IN.hit[KEY_UP];
    float vx = (float)(IN.down[KEY_RIGHT] - IN.down[KEY_LEFT]), vy = (float)(IN.down[KEY_DOWN] - IN.down[KEY_UP]);
    if (hx || hy) { G.map_cx += (float)hx; G.map_cy += (float)hy; G.map_hold = 0.0f; G.map_pan_t = 0.0f; }
    if (vx != 0.0f || vy != 0.0f) { G.map_hold += dt; G.map_pan_t = 0.0f; } else G.map_hold = 0.0f;
    if (G.map_hold >= 0.25f) { G.map_cx += vx * 14.0f * dt; G.map_cy += vy * 14.0f * dt; }
    map_clamp();
}
static void map_follow(float dt) {                                 /* it slides after you, unless panned a moment ago */
    if (!G.map_on) return;
    G.map_pan_t += dt;
    if (G.map_pan_t < 3.0f) return;
    float k = 1.0f - expf(-dt * 5.0f), cx = G.map_cx, cy = G.map_cy;
    G.map_cx = G.px; G.map_cy = G.py;
    map_clamp();                                                   /* where it would rest */
    G.map_cx = cx + (G.map_cx - cx) * k; G.map_cy = cy + (G.map_cy - cy) * k;
}

static void update_ui(void) {
    if (CAST_HIT) G.click_spent = 1;                              /* a click that closes a menu casts nothing */
    if (G.ui == UI_ASCEND) { update_ascend(); return; }
    if (G.ui == UI_ALIGN) { update_align(0); return; }
    if (G.ui == UI_CLASS) { update_class(); return; }
    if (G.ui == UI_HALL2) { update_hall2(); return; }
    if (G.ui == UI_STONES) { update_stones(); return; }
    if (G.ui == UI_WISPS) { update_wispmenu(); return; }
    if (G.ui == UI_SHRINE) { update_shrinemenu(); return; }
    if (G.ui == UI_TOTEM) { update_totemmenu(); return; }
    if (G.ui == UI_PORTAL) { update_portalmenu(); return; }
    if (G.ui == UI_COMP) { update_compmenu(); return; }
    if (G.ui == UI_REPO) { update_repomenu(); return; }
    if (G.ui == UI_CODEX) { update_codex(); return; }
#ifdef CRAWL_DEBUG
    if (G.pilgrim == 2 && G.ui == UI_ALTAR) {                     /* the test player sends the wisp on, after a look */
        static int look;
        if (++look >= 40) { look = 0; G.ui_sel = 0; IN.hit[KEY_ENTER] = 1; }
    }
#endif
    if (G.ui == UI_ALTAR) { update_altarmenu(); return; }
    if (G.ui == UI_BSHRINE) { update_bshrine(); return; }
    if (G.ui == UI_RECKON) { update_reckon(); return; }
    if (G.ui == UI_PAD) { update_padmenu(); return; }
    if (G.ui == UI_PORTRAIT) { update_portrait(); return; }
    if (G.ui == UI_BUY) { update_buy(); return; }
    if (G.ui == UI_TUTOR) { update_tutor(); return; }
    if (G.ui == UI_INVENTORY) update_inventory();
    else if (G.ui == UI_LEDGER) update_ledger();
    else if (G.ui == UI_MERCHANT) update_merchantmenu();
    else if (G.ui == UI_STONE) update_stonemenu();
    else if (G.ui == UI_BESTIARY) update_bestiary();
    else if (G.ui == UI_LETGO) update_letgo();
    else update_quit();
}

