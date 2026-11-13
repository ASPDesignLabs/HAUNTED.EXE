/* A part of scene_crawl.c (docs/PLAN-tidy.md): it is #included there in its place and never compiled on its own.
 * The record and its achievements, the portraits' part of it, experience, Levels of Atonement. */

/* ---- the record and its achievements (PLAN-bestiary, section 11): the autopilot earns them too ---- */

enum { ACH_SECRET0 = REC_KINDS * NRANK, ACH_LEVEL0 = ACH_SECRET0 + 16, ACH_DESCENT0 = ACH_LEVEL0 + 20,   /* bits: slayers by kind and rank, then the rest; */
       ACH_LEVEL21 = 256, ACH_BOSS0 = ACH_LEVEL21 + 20,          /* from version 6: levels 21 to 40, then the bosses */
       ACH_DEED0 = ACH_BOSS0 + 6 * 8,                            /* 1.0.0b13: puzzles at 1, 5, 20; trials at 1, 10, 30; each run's trials all won */
       ACH_WAY0 = ACH_DEED0 + 10,                                /* 1.0.0b18: ambushes, hunts shaken, packs beheaded, callers hushed, a file untouched */
       ACH_POR0 = ACH_WAY0 + 11,                                 /* the portraits (docs/PLAN-portraits.md, step 5): the Spectre's three, the Wraith's */
       ACH_FEAT0 = ACH_POR0 + 6 };                               /* three, then a feat for each kind that can be slain (FEATS) */
static const int PZ_STEP[3] = { 1, 5, 20 }, TRI_STEP[3] = { 1, 10, 30 };
static const char *const PZ_ACH[3] = { "A KEY OF THOUGHT", "THE PATIENT MIND", "NOTHING SEALED FOR LONG" };
static const char *const TRI_ACH[3] = { "AGAINST THE SAND", "THE HOURGLASS, BROKEN", "TIME'S OWN ENEMY" };
static const char *const TRI_RUN_ACH[4] = { "THE CRYPTS' TRIALS", "THE CATACOMBS' TRIALS", "THE HOLLOWS' TRIALS", "THE DEEP'S TRIALS" };
#define NSECRET_STEPS 7
static const int SECRET_STEP[NSECRET_STEPS] = { 1, 5, 10, 20, 30, 40, 50 };
static const char *const SECRET_NAME[NSECRET_STEPS] = { "A HOLLOW KNOCK", "LISTENING WALLS", "THE STONE GIVES WAY", "NOTHING STAYS HIDDEN",
                                                        "THE BUILDERS' TRICKS", "EVERY LAST CRACK", "KEEPER OF SECRETS" };
#define MAX_DESCENT 20                                              /* level 800: as far as anyone will go */
static const char *const ORDINAL[MAX_DESCENT + 1] = { "", "FIRST", "SECOND", "THIRD", "FOURTH", "FIFTH", "SIXTH", "SEVENTH", "EIGHTH", "NINTH",
    "TENTH", "ELEVENTH", "TWELFTH", "THIRTEENTH", "FOURTEENTH", "FIFTEENTH", "SIXTEENTH", "SEVENTEENTH", "EIGHTEENTH", "NINETEENTH", "TWENTIETH" };

/* how many of a kind in a rank make an achievement: 20 of the lowest rank it comes in, then 25, 20, 15 and 10, 5 gilded (the
 * owner's, for the portraits: docs/PLAN-portraits.md, Q15; 60, 40, 25, 15, 8 and 3 before); 0 if it never comes in that rank
 * (or can't be killed) */
static int slayer_need(int kind, int rank) {
    const Kind *k = &KINDS[kind];
    if (k->flags & (KF_SPIRIT | KF_UNCOUNTED)) return 0;
    static const int NEED[5] = { 20, 25, 20, 15, 10 };
    if (k->family < 0) return rank == RK_I ? NEED[0] : 0;          /* off the ladder: one rank, the first's count */
    if (rank == RK_GILDED) return k->gild ? 5 : 0;
    int low = crawl_set(k->debut) == 3 ? RK_II : RK_I;             /* the Deep has no rank I */
    return rank < low || rank > k->top ? 0 : NEED[rank - low];
}
static const char *rank_word(int kind, int rank) { return KINDS[kind].family == PF_CHITIN && rank == RK_V ? "PALE " : RANK_WORD[rank]; }

/* The tracker: pop-ups at the upper right, one at a time, in the manner of Steam's. An unlocked
 * achievement stays a little over four seconds; a note of progress, halfway there, less. */
typedef struct { int spr, family, rank, progress; char title[32], desc[32]; } Toast;
#define MAXTOASTS 12
static struct { Toast q[MAXTOASTS]; int n; float t; } TOASTS;
static float toast_len(const Toast *t) { return t->progress ? 2.8f : 4.4f; }
static void toast(int spr, int family, int rank, int progress, const char *title, const char *desc) {
    if (TOASTS.n >= MAXTOASTS || CON.used) return;
    Toast *t = &TOASTS.q[TOASTS.n++];
    t->spr = spr; t->family = family; t->rank = rank; t->progress = progress;
    snprintf(t->title, sizeof t->title, "%s", title); snprintf(t->desc, sizeof t->desc, "%s", desc);
    if (TOASTS.n == 1) TOASTS.t = -1.0f;                           /* its chime as it comes in */
}
static int boss_fighting(void);
static void update_toasts(float dt) {
    if (!TOASTS.n || boss_fighting()) return;                      /* they wait out a boss's fight, */
    if (G.hint_t > 0.0f && G.player && !G.ui && !G.dead) return;   /* and the controls card, which they would cover, */
    if ((G.player && G.ui) || G.tally) return;                     /* and a menu or the tally, over whose corner they had come */
    if (TOASTS.t < 0.0f) { TOASTS.t = 0.0f; snd_sfx_ex(TOASTS.q[0].progress ? SFX_TICK : SFX_KEYGET, TOASTS.q[0].progress ? 0.6f : 0.9f, 0.5f); }
    TOASTS.t += dt;
    if (TOASTS.t >= toast_len(&TOASTS.q[0])) {
        memmove(TOASTS.q, TOASTS.q + 1, sizeof TOASTS.q[0] * (size_t)(TOASTS.n - 1));
        if (--TOASTS.n) TOASTS.t = -1.0f;
    }
}

/* A Cursed Coin: one at the end of each chain, one more for every secret on the level found. Each is
 * taken once a level, dying or not (RUN.coin_bits, kept in saves), and pops up as an achievement does. */
#define COIN_ALL 7
static void gain_coin(int bit, const char *why) {
    if (G.replay) return;                                          /* a replay pays no Cursed Coins (docs/PLAN-b11.md, section 12) */
    if (RUN.coin_level != G.level) { RUN.coin_level = G.level; RUN.coin_bits = 0; }
    if (RUN.coin_bits >> bit & 1) return;
    RUN.coin_bits |= 1 << bit;
    RUN.coins++;
    toast(SP_CCOIN, -1, RK_I, 0, "A CURSED COIN", why);
    snd_sfx_ex(SFX_COIN, 0.7f, 0.5f);
    if (G.player) pop_readout();
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT coin %d coins=%d t=%.2f by %s\n", bit, RUN.coins, G.t, G.player ? "player" : "autopilot");
#endif
}

static int ach_has(int bit) { return RUN.rec.got[bit >> 3] >> (bit & 7) & 1; }
static void ach_earn(int bit, int spr, int family, int rank, const char *title, const char *desc) {
    if (ach_has(bit) || CON.used) return;                          /* the dead took note of the codes */
    RUN.rec.got[bit >> 3] |= (uint8_t)(1 << (bit & 7));
    toast(spr, family, rank, 0, title, desc);
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT achievement %d %s: %s t=%.1f\n", bit, title, desc, G.t);
#endif
}

static void rec_kill(const Mob *m) {
    Record *r = &RUN.rec;
    if (m->type >= REC_KINDS) return;
    if (r->kills[m->type][m->rank] < 65535) r->kills[m->type][m->rank]++;
    int el = m->last_atk >= 0 ? ATK_ELEM[m->last_atk] : -1;
    if (el >= 0 && r->el_kills[m->type][el] < 255) r->el_kills[m->type][el]++;
    int need = slayer_need(m->type, m->rank), have = r->kills[m->type][m->rank], bit = m->type * NRANK + m->rank;
    if (!need || ach_has(bit)) return;
    char title[32], desc[32];
    snprintf(title, sizeof title, "%s%s", rank_word(m->type, m->rank), KINDS[m->type].plural);
    if (have >= need) {
        snprintf(desc, sizeof desc, "%d SLAIN", need); ach_earn(bit, KINDS[m->type].spr, KINDS[m->type].family, m->rank, title, desc);
        if (ach_has(bit)) RUN.por_run[m->type][m->rank] = (uint8_t)(1 + crawl_set(G.level));   /* its portrait shows this run's stone */
    }
    else if (need >= 8 && have == need / 2) { snprintf(desc, sizeof desc, "%d OF %d SLAIN", have, need); toast(KINDS[m->type].spr, KINDS[m->type].family, m->rank, 1, title, desc); }
}
static void rec_seen(const Mob *m) {
    if (m->type >= REC_KINDS || m->boss) return;
    RUN.rec.seen[m->type] |= (uint8_t)(1 << m->rank);
    if (m->rank < RK_GILDED && RUN.top_rank[m->type] < m->rank + 1) RUN.top_rank[m->type] = (uint8_t)(m->rank + 1);   /* for a boss's adds */
}
/* what is learnt of a kind (docs/PLAN-b11.md, section 2): what an element does to it, from five slain by it
 * or twelve of its hits landed; and each harm it does, from three times it has hurt you with it */
static int el_learnt(int k, int el) {
    const Record *r = &RUN.rec;
    return k < REC_KINDS && el >= 0 && (r->el_kills[k][el] >= 5 || r->el_hits[k][el] >= 12);
}
static int harm_learnt(int k, int el) { return k < REC_KINDS && el >= 0 && RUN.rec.harms[k][el] >= 3; }
static void rec_hit(const Mob *m, int atk) {                       /* a spell's hit landed */
    int el = atk >= 0 && atk < NATK ? ATK_ELEM[atk] : -1;
    if (m->type >= REC_KINDS || m->boss || el < 0 || RUN.rec.el_hits[m->type][el] >= 255) return;
    RUN.rec.el_hits[m->type][el]++;
}
static int kind_harms(int k, int rank) {                           /* what a kind does to you, at a rank */
    int h = KINDS[k].harm;
    if (k == MOB_BAT && rank >= RK_II && rank != RK_GILDED) h |= H_SPIRIT;   /* Soul Sap, from rank II */
    return h;
}
static void rec_secret(void) {
    Record *r = &RUN.rec;
    if (r->secrets < 65535) r->secrets++;
    for (int i = 0; i < NSECRET_STEPS; i++)
        if (r->secrets >= SECRET_STEP[i] && !ach_has(ACH_SECRET0 + i)) {
            char d[32];
            if (i) snprintf(d, sizeof d, "%d SECRETS FOUND", SECRET_STEP[i]); else snprintf(d, sizeof d, "A SECRET FOUND");
            ach_earn(ACH_SECRET0 + i, SP_CHEST, -1, 0, SECRET_NAME[i], d);
        }
}
static int ach_level(int lv) { return lv <= 20 ? ACH_LEVEL0 + lv - 1 : ACH_LEVEL21 + lv - 21; }   /* level lv cleared, 1 to 40 */
static int level_perfect(void) {                                   /* the tally's PERFECT: every treasure and every creature */
    return G.items_total > 0 && G.items_got == G.items_total && G.kills_total > 0 && G.kills == G.kills_total;
}
static void rec_cleared(void) {
    Record *r = &RUN.rec;
    RUN.perfect = level_perfect();
#ifdef CRAWL_DEBUG
    if (getenv("CRAWL_PERFECT")) RUN.perfect = 1;                  /* every level counted perfect, for her hall's music */
#endif
    char t[48], d[32];
    if (G.level <= DESCENT_LEN) {
        r->cleared |= 1ull << (G.level - 1);
        level_name(G.level, t, sizeof t);
        snprintf(d, sizeof d, "LEVEL %d CLEARED", G.level);
        ach_earn(ach_level(G.level), SP_CHALICE, -1, 0, t, d);
    }
    if (G.level % DESCENT_LEN == 0) {
        int k = G.level / DESCENT_LEN;
        if (k > r->descents) r->descents = (uint16_t)k;
        if (k >= 2 && k <= MAX_DESCENT) {
            snprintf(t, sizeof t, "THE %s DESCENT", ORDINAL[k]); snprintf(d, sizeof d, "LEVEL %d CLEARED", G.level);
            ach_earn(ACH_DESCENT0 + k - 2, SP_CROWN, -1, 0, t, d);
        }
    }
    RUN.por_open |= POR_CLEARED;                                   /* her room pays the portraits from now on, this life */
}

/* The portraits' own achievements (docs/PLAN-portraits.md, 5 A and B, the owner's): the Spectre laid to rest, the Wraith driven back
 * (its fourth gem is the Wraith Unbound's fall, the boss's own), and a feat for each kind that can be slain, a medallion under its frame */
typedef struct { int kind, need; const char *name, *did, *rule; } PorFeat;
#define NPFEAT 20
static const PorFeat FEATS[NPFEAT] = {
    { MOB_SKEL, 20, "IT NEVER THREW", "%d BEFORE THEY THREW", "SLAIN BEFORE ITS FIRST THROW." },
    { MOB_BAT, 20, "SHAKEN OFF", "%d BATS SHAKEN OFF", "LATCHED ON YOU, AND SHAKEN OFF." },
    { MOB_WSKULL, 20, "BEFORE THE SCREAM", "%d BEFORE THE SCREAM", "SILENCED BEFORE ITS SCREAM GOES OUT." },
    { MOB_SHIELD, 20, "THROUGH THE SHIELD", "%d THROUGH THE SHIELD", "SLAIN WITH ITS SHIELD BROKEN FIRST." },
    { MOB_SPIDER, 20, "STILL IN ITS WEB", "%d STILL IN THEIR WEBS", "SLAIN IN ITS WEB, BEFORE IT DROPS." },
    { MOB_SHAMBLER, 20, "BACK IN THE GROUND", "%d BEFORE THEY STOOD", "SLAIN AS IT RISES, BEFORE IT STANDS." },
    { MOB_DROWNED, 10, "NOT A DROP", "%d BURST WIDE OF YOU", "SLAIN, AND ITS BURST NEVER REACHES YOU." },
    { MOB_HOUND, 10, "BOTH OF THEM", "%d PAIRS IN FIVE SECONDS", "TWO SLAIN WITHIN FIVE SECONDS." },
    { MOB_HUNTER, 20, "OFF THE CEILING", "%d SLAIN OVERHEAD", "SLAIN ON THE CEILING, OR AS IT DROPS." },
    { MOB_GHOUL, 20, "AT ITS MEAL", "%d SLAIN FEEDING", "SLAIN AT ITS MEAL." },
    { MOB_KNIGHT, 10, "IN FULL PLATE", "%d IN FULL PLATE", "SLAIN BEFORE IT SHEDS ANY PLATE." },
    { MOB_BROOD, 5, "NONE HATCHED", "%d BEFORE A SAC HATCHED", "SLAIN BEFORE ANY SAC OF HERS HATCHES." },
    { MOB_LING, 200, "THE WHOLE BROOD", "%d SLAIN", "TWO HUNDRED OF THEM SLAIN." },
    { MOB_SHADE, 10, "FACED DOWN", "%d FADING FROM THE LIGHT", "SLAIN AS IT FADES BACK FROM YOUR LIGHT." },
    { MOB_IMP, 25, "QUENCHED", "%d SLAIN BY FROST", "SLAIN BY FROST." },
    { MOB_DEACON, 10, "NONE RISE", "%d BEFORE ANY ROSE", "SLAIN BEFORE ANY RISES FOR HIM." },
    { MOB_GARGOYLE, 10, "STILL STONE", "%d AS THEY WOKE", "SLAIN WITHIN TWO SECONDS OF WAKING." },
    { MOB_WHIPPER, 5, "NO HORN", "%d BEFORE THE HORN", "SLAIN BEFORE HIS HORN SOUNDS." },
    { MOB_SEXTON, 10, "UNRUNG", "%d, THE BELL UNRUNG", "SLAIN, HIS BELL NEVER RUNG TO CALL THEM." },
    { MOB_LIGHTLESS, 10, "IN THE DARK", "%d, THE LANTERN SHUT", "SLAIN WITH YOUR LANTERN SHUTTERED." } };
static const int GHOST_STEP[3] = { 5, 15, 40 }, WRAITH_STEP[3] = { 10, 40, 100 };
static const char *const GHOST_ACH[3] = { "LAID TO REST", "A CHILL LIFTED", "THE HALLS GONE QUIET" };
static const char *const WRAITH_ACH[3] = { "DRIVEN BACK", "KEPT AT BAY", "IT LEARNS FEAR" };
#define NPORACH (6 + NPFEAT)
typedef struct { int bit, kind, gem, need; const char *title; char desc[32]; } PorAch;
static void por_ach(int i, PorAch *a) {                            /* the i-th of them, 0 to NPORACH - 1 */
    if (i < 6) {
        int w = i >= 3, j = i % 3;
        a->bit = ACH_POR0 + i; a->kind = w ? MOB_WRAITH : MOB_GHOST; a->gem = j; a->need = w ? WRAITH_STEP[j] : GHOST_STEP[j];
        a->title = w ? WRAITH_ACH[j] : GHOST_ACH[j];
        snprintf(a->desc, sizeof a->desc, w ? "%d TIMES DRIVEN BACK" : "%d LAID TO REST", a->need);
        return;
    }
    const PorFeat *f = &FEATS[i - 6];
    a->bit = ACH_FEAT0 + i - 6; a->kind = f->kind; a->gem = POR_FEAT; a->need = f->need; a->title = f->name;
    snprintf(a->desc, sizeof a->desc, f->did, f->need);
}
static int feat_of(int k) { for (int i = 0; i < NPFEAT; i++) if (FEATS[i].kind == k) return i; return -1; }
static int por_count(int k) {                                      /* where a kind's own count stands */
    if (k < 0 || k >= REC_KINDS) return 0;
    if (k == MOB_LING) { int n = 0; for (int r = 0; r < NRANK; r++) n += RUN.rec.kills[k][r]; return n; }   /* the record's */
    if (k == MOB_IMP) return RUN.rec.el_kills[k][EL_FROST];
    return RUN.por_feat[k];
}
/* what kind k's own count reaches now, earned: with a pop-up and the run it was earned in, or quietly (the sweep, under its one
 * pop-up); halfway, a note of progress; how many earned */
static int por_reckon(int k, int quiet, PorAch *first) {
    int have = por_count(k), n = 0;
    for (int i = 0; i < NPORACH; i++) {
        PorAch a;
        por_ach(i, &a);
        if (a.kind != k || ach_has(a.bit) || CON.used) continue;
        if (have >= a.need) {
            if (quiet) { RUN.rec.got[a.bit >> 3] |= (uint8_t)(1 << (a.bit & 7)); if (!n && first) *first = a; }
            else {
                ach_earn(a.bit, KINDS[k].spr, KINDS[k].family, RK_I, a.title, a.desc);
                if (a.gem < NRANK) RUN.por_run[k][a.gem] = (uint8_t)(1 + crawl_set(G.level));   /* its portrait shows this run's stone */
            }
            n++;
#ifdef CRAWL_DEBUG
            if (quiet) fprintf(stderr, "EVENT achievement %d %s: swept, %d of %d\n", a.bit, a.title, have, a.need);
#endif
        } else if (!quiet && a.need >= 8 && have == a.need / 2) {
            char d[32];
            snprintf(d, sizeof d, "%d OF %d", have, a.need);
            toast(KINDS[k].spr, KINDS[k].family, RK_I, 1, a.title, d);
        }
    }
    return n;
}
static void por_bump(int k) {                                      /* one more of kind k's own deeds */
    if (k < 0 || k >= REC_KINDS) return;
    if (RUN.por_feat[k] < 65535) RUN.por_feat[k]++;
    por_reckon(k, 0, NULL);
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT feat %s %d t=%.2f by %s\n", KINDS[k].plural, por_count(k), G.t, G.player ? "player" : "autopilot");
#endif
}
static void knight_shed(Mob *m);
/* a kill: whether it was its kind's feat (the record has counted it already) */
static void por_feat_kill(Mob *m) {
    int f = 0;
    switch (m->type) {
    case MOB_SKEL: f = !(m->feat & FT_THREW); break;
    case MOB_WSKULL: f = !m->called; break;                       /* its scream not yet down the halls */
    case MOB_SHIELD: f = (m->feat & FT_BROKE) != 0; break;
    case MOB_SPIDER: f = !(m->feat & FT_DROPPED); break;
    case MOB_SHAMBLER: f = m->sub == 2; break;                     /* out of the earth, not yet on its feet */
    case MOB_HOUND: {                                              /* two within five seconds, a pair at a time */
        static double last = -1e9;
        if (G.t - last >= 0.0 && G.t - last <= 5.0) { f = 1; last = -1e9; } else last = G.t;
        break;
    }
    case MOB_HUNTER: case MOB_GHOUL: f = (m->feat & FT_NOW) != 0; break;   /* overhead, or at its meal, as the blow fell */
    case MOB_KNIGHT: knight_shed(m); f = !(m->feat & FT_SHED); break;
    case MOB_BROOD: f = !(m->feat & FT_HATCHED); break;
    case MOB_SHADE: f = m->shy > 0.0f; break;
    case MOB_DEACON: f = !m->raised; break;
    case MOB_GARGOYLE: f = m->feat_t > 0.0f && G.level_t - (double)m->feat_t <= 2.0; break;
    case MOB_WHIPPER: f = !(m->feat & FT_HORN); break;
    case MOB_SEXTON: f = !(m->feat & FT_RUNG); break;
    case MOB_LIGHTLESS: f = lantern_held() && RUN.lantern_off; break;
    case MOB_LING: case MOB_IMP: por_reckon(m->type, 0, NULL); return;   /* counted by the record */
    default: return;                                               /* the Drowned's when its burst is done (update_pools) */
    }
    if (f) por_bump(m->type);
}

/* ---- the portraits of the slain (docs/PLAN-portraits.md): what the record keeps for them ---- */

/* the run an unlocked rank is shown in when nobody saw where (an older page, or a count lowered): the first run that rank is
 * common in, rank I the kind's own, II the Catacombs, III the Hollows, IV and V the Deep, gilded the kind's own */
static int por_guess(int k, int gem) {
    static const int RUN_OF[POR_GEMS] = { 0, 1, 2, 3, 3, 0, 0, 0 };
    return imax(crawl_set(KINDS[k].debut), RUN_OF[gem]);
}
static int por_unlocked(int k);
static void por_fill(void) {                                       /* every rank unlocked has a run to be shown in */
    for (int k = 0; k < NKINDS; k++) for (int r = 0; r < NRANK; r++)
        if (por_unlocked(k) >> r & 1 && !RUN.por_run[k][r]) RUN.por_run[k][r] = (uint8_t)(1 + por_guess(k, r));
}
static const int RANK_COL[NRANK] = { R_BONE + 13, R_MOSS + 13, R_GHOST + 13, R_BLOOD + 13, R_IRON + 11, R_GOLD + 11 };   /* a rank's in words */
/* the frame's colour is the highest rank unlocked, black over gilded over the rest */
static int por_high(int unl) {
    static const int ORD[NRANK] = { 0, 1, 2, 3, 5, 4 };
    int hi = -1;
    for (int r = 0; r < NRANK; r++) if (unl >> r & 1 && (hi < 0 || ORD[r] > ORD[hi])) hi = r;
    return hi;
}
static int kind_ranks(int k, int *out);
static int por_ranks(int k, int *out) {                            /* a portrait's ranks: those of its kind with an achievement */
    if (k == MOB_GHOST || k == MOB_WRAITH) {                       /* the Spectre's three laid to rest, the Wraith's three and its fall */
        int n = k == MOB_WRAITH ? 4 : 3;
        for (int r = 0; r < n; r++) out[r] = r;
        return n;
    }
    int rk[NRANK], nr = kind_ranks(k, rk), n = 0;
    for (int j = 0; j < nr; j++) if (slayer_need(k, rk[j])) out[n++] = rk[j];
    return n;
}
static int por_unlocked(int k) {                                   /* a portrait's gems unlocked, a bit each (RK_*, and POR_FEAT) */
    int b = 0;
    if (k == MOB_GHOST || k == MOB_WRAITH) {
        for (int r = 0; r < 3; r++) if (ach_has(ACH_POR0 + (k == MOB_WRAITH ? 3 : 0) + r)) b |= 1 << r;
        if (k == MOB_WRAITH && ach_has(ACH_BOSS0 + 6 * BOSS_WRAITH)) b |= 1 << RK_IV;   /* the Wraith Unbound's fall */
    } else for (int r = 0; r < NRANK; r++) if (slayer_need(k, r) && ach_has(k * NRANK + r)) b |= 1 << r;
    int f = feat_of(k);
    if (f >= 0 && ach_has(ACH_FEAT0 + f)) b |= 1 << POR_FEAT;
    return b;
}
/* of those, the ones whose gold waits at her room (the card's, step 4); the Wraith's fourth lights but pays nothing (Q14) */
static int por_waiting(int k) {
    return RUN.por_open ? por_unlocked(k) & ~RUN.por_taken[k] & ~(k == MOB_WRAITH ? 1 << RK_IV : 0) : 0;
}
/* a boss's portrait: slain, then each challenge step, a bit each */
static int boss_unlocked(int b) {
    int w = 0;
    for (int s = 0; s < 6; s++) if (ach_has(ACH_BOSS0 + 6 * b + s)) w |= 1 << s;
    return w;
}
static int boss_waiting(int b) { return RUN.por_open ? boss_unlocked(b) & ~RUN.por_btaken[b] : 0; }
/* what a boss's deed pays, once a life (section 3): its fall on the way down, then a fifth more for each challenge step */
static int boss_gold(int b, int s) { static const int BASE[NBOSS] = { 1500, 2500, 3500, 4500 }; return b < 0 || b >= NBOSS ? 0 : BASE[b] * (5 + s) / 5; }
static int boss_wait_gold(int b) { int w = boss_waiting(b), g = 0; for (int s = 0; s < 6; s++) if (w >> s & 1) g += boss_gold(b, s); return g; }
#define POR_BOSSBIT 24                                             /* a boss's tapestry's bit in RUN.por_drawn, after the kinds' */
static const int BOSS_GEM[6] = { RK_I, RK_II, RK_III, RK_IV, RK_GILDED, RK_V };   /* a deed's colour: slain bone, the steps green to black */
static const char *const POR_BOSS_NAME[NBOSS] = { "THE ILL-FATED KNIGHT", "THE WANDERING SPIDER", "THE BLACK DEACON", "THE WRAITH UNBOUND" };   /* as its
                                                                      achievements and the settled sheet name them */
/* a count lowered (Q15) leaves a record already past it unearned till its next kill: as a run or page is read, whatever its
 * counts reach now is earned at once, under one pop-up */
static void slayer_sweep(void) {
    if (CON.used) return;
    int n = 0, k0 = 0, r0 = 0;
    char title[32], desc[32];
    for (int k = 0; k < NKINDS; k++) for (int r = 0; r < NRANK; r++) {
        int need = slayer_need(k, r), bit = k * NRANK + r;
        if (!need || ach_has(bit) || RUN.rec.kills[k][r] < need) continue;
        RUN.rec.got[bit >> 3] |= (uint8_t)(1 << (bit & 7));
        if (!n++) { k0 = k; r0 = r; snprintf(title, sizeof title, "%s%s", rank_word(k, r), KINDS[k].plural); snprintf(desc, sizeof desc, "%d SLAIN", need); }
#ifdef CRAWL_DEBUG
        fprintf(stderr, "EVENT achievement %d %s%s: swept, %d slain of %d\n", bit, rank_word(k, r), KINDS[k].plural, RUN.rec.kills[k][r], need);
#endif
    }
    for (int k = 0; k < NKINDS; k++) {                             /* and the portraits' own (step 5): the spiderlings and imps the record
                                                                      counted before them */
        PorAch a;
        int m = por_reckon(k, 1, &a);
        if (m && !n) { k0 = k; r0 = RK_I; snprintf(title, sizeof title, "%s", a.title); snprintf(desc, sizeof desc, "%s", a.desc); }
        n += m;
    }
    if (!n) return;
    por_fill();
    if (n > 1) { snprintf(title, sizeof title, "THE DEAD RECKONED AGAIN"); snprintf(desc, sizeof desc, "%d ACHIEVEMENTS", n); }
    toast(KINDS[k0].spr, KINDS[k0].family, r0, 0, title, desc);
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT reckoned %d\n", n);
#endif
}
/* a death that costs begins a life: every portrait's gold is owed again, and her room pays it as you wake */
static void por_life(void) {
    memset(RUN.por_taken, 0, sizeof RUN.por_taken); memset(RUN.por_btaken, 0, sizeof RUN.por_btaken);
    RUN.por_open = POR_WOKEN;
}
/* the portraits as they hang (gen_shop, por_hang): where, which kind, its candles' layout, its walk's phase; each drawn into an
 * overlay of POR_RES texels a side, column by column as the walls are, again only while it was in view and its candles or its
 * creature have moved (por_frame), and render_walls laying it over its face's stone */
#define POR_RES 128
#define POR_DRAW_T 1.3f                                            /* a tapestry takes so long to draw back */
#define NPOR 30                                                    /* the twenty-two kinds', and the bosses' four, two faces each */
typedef struct { int x, y, fd, kind, layout; float ph; int boss, half; } PorAt;   /* boss: 1 + which (its kind -1); half: 0 its left as seen */
/* a portrait's candle's flame on the wall: its face (fd), the face's line (x for faces looking east or west, else y), where along it,
 * how far down (0 the top, 1 the foot), its glow's strength, and its light (for the flicker) */
typedef struct { int fd, light; float plane, along, z, pw; } PorFlame;
static struct { int n, frame, nwf; PorAt at[NPOR]; int key[NPOR], drawn[NPOR], seen[NPOR]; PorFlame wf[NPOR * 4];
                float veil_t[NPOR]; } POR;                         /* veil_t: when its tapestry began to be drawn back (step 4) */
static uint8_t POR_OV[NPOR][POR_RES * POR_RES];
static int por_bossof(int i) { return POR.at[i].boss - 1; }       /* the boss a portrait's face is half of, or -1 */
static int por_head(int i) { return POR.at[i].boss && POR.at[i].half ? i - 1 : i; }   /* a boss's left half stands for it */
static int por_bit(int i) { return POR.at[i].boss ? POR_BOSSBIT + POR.at[i].boss - 1 : POR.at[i].kind; }   /* its tapestry, in por_drawn */
static int best_kinds(int *out);
static int kind_ranks(int k, int *out);
#ifdef CRAWL_DEBUG
static void por_trace(const char *why) {                           /* PORTRAITS: the state, as it stands */
    int drawn = 0, unl = 0, wait = 0, bw = 0, taken = 0;
    for (int k = 0; k < NKINDS; k++) {
        drawn += RUN.por_drawn >> k & 1;
        for (int r = 0; r < POR_GEMS; r++) taken += RUN.por_taken[k] >> r & 1;
        for (int r = 0; r < POR_GEMS; r++) { unl += por_unlocked(k) >> r & 1; wait += por_waiting(k) >> r & 1; }
    }
    for (int b = 0; b < NBOSS; b++) for (int s = 0; s < 6; s++) bw += boss_waiting(b) >> s & 1;
    fprintf(stderr, "PORTRAITS %s: open %d drawn %d unlocked %d waiting %d taken %d bosses' waiting %d runs", why, RUN.por_open, drawn, unl, wait, taken, bw);
    for (int k = 0; k < NKINDS; k++) if (por_unlocked(k)) {
        fprintf(stderr, " %d:", k);
        for (int r = 0; r < NRANK; r++) if (por_unlocked(k) >> r & 1) fprintf(stderr, "%c%d", "12345G"[r], RUN.por_run[k][r] - 1);
    }
    fprintf(stderr, " counts");                                    /* the portraits' own (step 5) */
    for (int k = 0; k < NKINDS; k++) if (RUN.por_feat[k]) fprintf(stderr, " %d:%d", k, RUN.por_feat[k]);
    fprintf(stderr, "\n");
}
#endif

static Item *add_item(int x, int y, int type);
static void add_pool(float x, float y);
static void wraith_back(Mob *m);
static void wraith_back_for(Mob *m, float secs);
static int mk_shot(Shot *s);
static int mk_aimed(void);
static void mk_strike(int k, int el);
static void mk_burst(float x, float y, int el, unsigned skip);
/* ---- experience (docs/PLAN-b9.md, section 3): banked as it comes, claimed as Levels of Atonement with
 * the merchant. A kill pays its kind's worth by the level and its rank; in a pocket, the rank it would have
 * had on the level, and a quarter more for the first link and a tenth for each after, 1.6 at most. Secrets,
 * a chain's end, a boss, a level cleared and each room first set foot in pay too. Every gain is shown with
 * its value, and the moment enough is banked for another level the choir sings. ---- */
static const int KIND_XP[] = {
    [MOB_SKEL] = 8, [MOB_GHOST] = 0, [MOB_BAT] = 5, [MOB_WSKULL] = 6, [MOB_SHIELD] = 12, [MOB_SPIDER] = 7,
    [MOB_SHAMBLER] = 12, [MOB_DROWNED] = 16, [MOB_HOUND] = 8, [MOB_WRAITH] = 0, [MOB_HUNTER] = 12, [MOB_GHOUL] = 14,
    [MOB_KNIGHT] = 24, [MOB_BROOD] = 26, [MOB_LING] = 1, [MOB_SAC] = 0, [MOB_SHADE] = 16, [MOB_IMP] = 12,
    [MOB_DEACON] = 20, [MOB_GARGOYLE] = 18, [MOB_WHIPPER] = 22, [MOB_SEXTON] = 16, [MOB_LIGHTLESS] = 26 };
static const float RANK_XP[NRANK] = { 1.0f, 1.4f, 1.9f, 2.5f, 3.2f, 4.0f };
static const int BOSS_XP[NBOSS] = { 24, 22, 22, 26 };              /* forty of these, at the boss's level */
#define ATONE_TOP 50                        /* past it, overlevels */
static float xp_level(void) { return 1.0f + 0.12f * (float)(imax(1, G.level) - 1); }
#define ATONE_K 42                          /* 42 x (n-1)^2 in all for level n: the autopilot at 50 about level 35 */
static int atone_need(int n) {                                     /* experience in all to reach Level of Atonement n */
    if (n <= 1) return 0;
    if (n <= ATONE_TOP) return ATONE_K * (n - 1) * (n - 1);
    return ATONE_K * (ATONE_TOP - 1) * (ATONE_TOP - 1) + (n - ATONE_TOP) * ATONE_K * (2 * ATONE_TOP - 3);   /* each past it as dear as the last */
}
static int atone_reach(void) {                                     /* the level the experience banked would buy */
    int n = imax(1, RUN.atone);
    while (n < 9999 && atone_need(n + 1) <= RUN.xp) n++;
    return n;
}
#define XPF_N 5
static struct { char what[XPF_N][36]; int v[XPF_N]; float t[XPF_N]; int n; float banner; int banner_lvl; } XPF;   /* the gains shown, newest first */
static void xp_gain(float amount, const char *what) {
    amount *= (G.corrupt ? 0.5f : 1.0f) * (G.fonted ? 1.1f : 1.0f);   /* the bell's half; the font's tenth more */
    char cursed[36];
    if (G.tri_boon < 0 && !G.shop && !PK.depth) { amount *= 0.8f; snprintf(cursed, sizeof cursed, "%.27s, CURSED", what); what = cursed; }   /* the trial's curse */
    int v = (int)(amount + 0.5f);
    if (v <= 0) return;
    int was = atone_reach();
    RUN.xp = RUN.xp > 2000000000 - v ? 2000000000 : RUN.xp + v;
    G.level_xp += v;
    memmove(&XPF.what[1], &XPF.what[0], sizeof XPF.what[0] * (XPF_N - 1));
    memmove(&XPF.v[1], &XPF.v[0], sizeof XPF.v[0] * (XPF_N - 1));
    memmove(&XPF.t[1], &XPF.t[0], sizeof XPF.t[0] * (XPF_N - 1));
    snprintf(XPF.what[0], sizeof XPF.what[0], "%s", what);
    XPF.v[0] = v; XPF.t[0] = 0.0f; XPF.n = imin(XPF.n + 1, XPF_N);
    int now = atone_reach();
    if (now > was) {                                               /* enough for another: she will hear it */
        XPF.banner = 3.2f; XPF.banner_lvl = now;
        snd_sfx_ex(SFX_ATONE, 1.0f, 0.5f);
        if (G.player) rumble(0.3f, 0.2f, 0.6f);
    }
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT xp +%d %s total %d reach %d level %d t=%.2f\n", v, what, RUN.xp, now, G.level, G.t);
#endif
}
static void xp_kill(const Mob *m) {
    if (m->boss || m->ally || m->type < 0 || m->type >= (int)(sizeof KIND_XP / sizeof KIND_XP[0]) || !KIND_XP[m->type]) return;
    int rank = m->rank0 ? m->rank0 - 1 : m->rank;
    float v = (float)KIND_XP[m->type] * xp_level() * RANK_XP[clampi(rank, 0, NRANK - 1)];
    if (m->rank0 && PK.depth >= 2) v *= fminf(1.6f, 1.25f + 0.1f * (float)(PK.depth - 2));   /* a chain's: more, but not a black rank's worth */
    if (m->add) v *= 0.5f;                                         /* a boss's adds */
    char what[36];
    snprintf(what, sizeof what, "%s%s", m->rank == RK_I ? "" : rank_word(m->type, m->rank), KINDS[m->type].name);
    if (G.tri_boon > 0 && !PK.depth) { v *= 1.2f; size_t n = strlen(what); snprintf(what + n, sizeof what - n, ", BOON"); }   /* the trial's boon */
    xp_gain(v, what);
}
/* ---- Levels of Atonement (docs/PLAN-b9.md, sections 4 and 5): ascending with the merchant takes every
 * level the banked experience reaches and gives ten points for each up to the fiftieth; the points buy steps
 * in these lines, which join what is worn under its cap ---- */
typedef struct { const char *name, *does; int cost, stat; float step; } Line;
static const Line LINES[NLINE] = {
    [LN_HEALTH]   = { "HEALTH", "+5 HEALTH", 1, ST_HP, 5.0f },
    [LN_DAMAGE]   = { "DAMAGE", "+1% TO EVERY SPELL'S DAMAGE", 3, ST_DAMAGE, 1.0f },
    [LN_RESIST]   = { "RESIST ALL", "+5% TO EACH RESIST: POISON, BURN, SLOW, SPIRIT, LIGHTNING", 3, -1, 5.0f },
    [LN_POISON]   = { "RESIST POISON", "+15%: POISON STINGS LESS, AND NOT AS LONG", 3, ST_RES_POISON, 15.0f },
    [LN_BURN]     = { "RESIST BURN", "+15%: FIRE AND BURNING HURT LESS", 3, ST_RES_BURN, 15.0f },
    [LN_SLOW]     = { "RESIST SLOW", "+15%: GRABS AND FROST SLOW YOU LESS", 3, ST_RES_SLOW, 15.0f },
    [LN_HASTE]    = { "SPELL HASTE", "+1%: EVERY SPELL COMES ROUND SOONER", 3, ST_HASTE, 1.0f },
    [LN_DODGE]    = { "DODGE", "+1%: A BLOW MAY PASS THROUGH YOU", 3, ST_DODGE, 1.0f },
    [LN_TREASURE] = { "TREASURE", "+1% ON ALL GOLD PICKED UP", 2, ST_TREASURE, 1.0f },
    [LN_MOVE]     = { "MOVEMENT", "+1% TO YOUR PACE", 2, ST_MOVE, 1.0f },
    [LN_LUCK]     = { "LUCK", "+0.1%: THE DEAD DROP GOLD MORE OFTEN", 1, ST_LUCK, 0.1f },
    [LN_ATTUNE]   = { "ATTUNEMENT", "+1%: SPELLS COST LESS SPIRIT", 1, ST_THRIFT, 1.0f },
    [LN_SPIRIT]   = { "RESIST SPIRIT", "+15%: SPIRIT HURTS LESS, AND WHAT DRAINS SOULS TAKES FEWER", 3, ST_RES_SPIRIT, 15.0f },
    [LN_LIGHT]    = { "RESIST LIGHTNING", "+15%: LIGHTNING HURTS LESS, AND BREAKS A SPELL LESS OFTEN", 3, ST_RES_LIGHT, 15.0f },
    [LN_SPRINT]   = { "SPRINT", "+0.5 S: YOU SPRINT LONGER, TO TEN SECONDS", 1, ST_SPRINT, 0.5f },   /* docs/PLAN-ai.md, 24 */
    [LN_PHYSIC]   = { "PHYSIC", "+5%: A POTION RESTORES MORE, TO FIVE TIMES", 1, ST_PHYSIC, 5.0f },
};
static int line_overs(int l) { return l != LN_HEALTH && l != LN_RESIST && l != LN_SPRINT && l != LN_PHYSIC; }   /* past its cap? */
#define LUCK_BASE 2.0f                      /* everyone's, at the start */
static float over_step(int l) { return l == LN_LUCK ? 1.0f : 5.0f; }   /* an overlevel's: Luck 1, the rest 5 */
static float line_own(int l) { return LINES[l].step * (float)RUN.steps[l] + over_step(l) * (float)RUN.over[l]; }
