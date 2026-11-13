/* A part of scene_crawl.c (docs/PLAN-tidy.md): it is #included there in its place and never compiled on its own.
 * Her hall's things: the Rune Shrine, the Hexed Totem, the Accursed Portal (runs, replays, challenges), the bell's and font's errands, update_shop. */

/* ---- the Rune Shrine (docs/PLAN-b6.md): cursed awake with a coin, raised with more, and pulled for
 * runes, which come into its nine slots; three alike become one better. Each pull earns one take, a
 * rune carried off before you leave the room. ---- */

static const int SHRINE_COST[NQUAL] = { 1, 6, 12, 24, 32 };       /* to curse it, then to raise it to each rank after */
static const char *const DEF_SHORT[NDEF] = { "WARD", "BLINK", "MIRROR", "MEND", "PURIFY", "NOVA", "DREAD", "SLIP", "REAP" };
static const char *const Q_TAG[NQUAL] = { "C", "U", "R", "UR", "EX" };   /* a quality in a letter or two, as well as its colour */
enum { SM_MENU, SM_TAKE, SM_LETGO, SM_FULL, SM_LEAVE, SM_PULL };   /* the screen: its actions, choosing a slot, two questions, a pull */
enum { SR_PULL, SR_FAVOUR, SR_RAISE, SR_TAKE, SR_LETGO, SR_LEAVE, NSR };

static int pull_most(int coins) { static const int M[4] = { 0, 2, 3, 5 }; return M[clampi(coins, 0, 3)]; }
static int fav_kind(int f) { return (f - 1) / 3; }
static int fav_coins(int f) { return f ? (f - 1) % 3 + 1 : 0; }
static int fav_step(int f, int dir) {                              /* the next favour: none, or 1-3 coins on an attuned kind */
    for (int i = 0; i < 1 + 3 * NDEF; i++) {
        f = (f + dir + 1 + 3 * NDEF) % (1 + 3 * NDEF);
        if (!f || (RUN.attuned >> fav_kind(f) & 1)) return f;
    }
    return 0;
}
static int shrine_count(int filled) { int n = 0; for (int i = 0; i < SHRINE_SLOTS; i++) n += (RUN.slot_def[i] != 0) == filled; return n; }
static int runes_held(void) { int n = 0; for (int k = 0; k < NDEF; k++) n += RUN.gear.def[k] > 0; return n; }

static void shrine_note(const char *fmt, ...) {                    /* a line on the shrine's screen, for a few seconds */
    char b[sizeof G.sh_note];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(b, sizeof b, fmt, ap);
    va_end(ap);
    snprintf(G.sh_note, sizeof G.sh_note, "%s", pkp(b));            /* ENTER AGAIN, or A on a controller (never printed into itself:
                                                                      the C library may clear the line before it reads it) */
    G.sh_note_t = 4.0f;
    if (!G.player) say("%s", G.sh_note);                           /* the autopilot's doings, for whoever watches */
}
static void shop_say(const char *fmt, ...) {                      /* her wares' words: on the buying screen while it is up, else said */
    char b[80];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(b, sizeof b, fmt, ap);
    va_end(ap);
    if (G.ui == UI_BUY) shrine_note("%s", b); else say("%s", b);
}
static void shrine_attune(int k, int q) {                          /* a rune at Rare or better: the shrine knows it */
    if (q < Q_RARE + 1 || (RUN.attuned >> k & 1)) return;
    RUN.attuned |= 1 << k;
    shrine_note("THE SHRINE IS ATTUNED TO %s", DEF_NAME[k]);
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT shrine_attune %s t=%.2f\n", DEF_NAME[k], G.t);
#endif
}

static void shrine_curse(void) {
    RUN.coins -= SHRINE_COST[0]; RUN.shrine = 1;
    if (G.sh_deco >= 0 && G.sh_deco < G.ndecos) G.decos[G.sh_deco].spr = SP_SHRINE_C;
    G.sh_glow = 2.0f;
    snd_sfx_ex(SFX_STINGER, 0.6f, 0.5f); snd_sfx_ex(SFX_IGNITE, 0.9f, 0.5f);
    shrine_note("THE SHRINE IS CURSED. IT GIVES COMMON RUNES");
    if (RUN.gear.def[DF_MIRROR] <= 0) {                            /* and a Mirror with it, to raise by hand (1.0.0b16) */
        RUN.gear.def[DF_MIRROR] = 1;
        toast(SP_SRUNE0 + DF_MIRROR, -1, RK_I, 0, "A MIRROR VEIL, FROM THE SHRINE", pk("RIGHT-CLICK RAISES IT: WHAT IT MEETS AT ONCE GOES HOME"));
#ifdef CRAWL_DEBUG
        fprintf(stderr, "EVENT shrine_mirror t=%.2f\n", G.t);
#endif
    }
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT shrine_curse coins=%d t=%.2f\n", RUN.coins, G.t);
#endif
}
static void shrine_raise(void) {
    RUN.coins -= SHRINE_COST[RUN.shrine]; RUN.shrine++;
    G.sh_glow = 2.0f;
    snd_sfx_ex(SFX_STINGER, 0.7f, 0.5f); snd_sfx_ex(SFX_SHIMMER, 0.9f, 0.5f);
    shrine_note("RAISED: IT GIVES RUNES UP TO %s", QUAL_NAME[RUN.shrine - 1]);
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT shrine_raise %d coins=%d t=%.2f\n", RUN.shrine, RUN.coins, G.t);
#endif
}

/* A pull: one coin shows one or two runes, two coins two or three, three coins three to five. Each is
 * of a quality up to the shrine's rank, each two thirds as likely as the one below; a favoured kind is a
 * third likelier for each coin offered on it. They wait in a queue and come into the slots one by one. */
static void shrine_pull(int coins, int fav) {
    RUN.coins -= coins + fav_coins(fav);
    G.sh_takes++; G.sh_pulls++;
    int n = coins <= 1 ? 1 + rnd(2) : (coins == 2 ? 2 + rnd(2) : 3 + rnd(3));
    float wq[NQUAL], tq = 0.0f, wk[NDEF], tk = 0.0f;
    for (int q = 0; q < NQUAL; q++) { wq[q] = q < RUN.shrine ? powf(2.0f / 3.0f, (float)q) : 0.0f; tq += wq[q]; }
    for (int k = 0; k < NDEF; k++) { wk[k] = fav && fav_kind(fav) == k ? powf(4.0f / 3.0f, (float)fav_coins(fav)) : 1.0f; tk += wk[k]; }
    G.sh_nq = 0;
    for (int i = 0; i < n; i++) {
        float r = rndf() * tq; int q = 0;
        while (q < NQUAL - 1 && r >= wq[q]) r -= wq[q++];
        r = rndf() * tk; int k = 0;
        while (k < NDEF - 1 && r >= wk[k]) r -= wk[k++];
        G.sh_qk[G.sh_nq] = k; G.sh_qq[G.sh_nq] = q + 1; G.sh_nq++;
    }
    G.sh_qt = 0.5f; G.sh_glow = 1.2f; G.sh_busy = 1;
    snd_sfx_ex(SFX_COIN, 0.8f, 0.5f); snd_sfx_ex(SFX_WHISPER, 0.8f, 0.5f);
    shrine_note("%d RUNES COME OUT OF THE STONE", n);
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT shrine_pull coins=%d favour=%d n=%d left=%d t=%.2f\n", coins, fav, n, RUN.coins, G.t);
#endif
}
/* any three of a kind and quality in the slots become one of the next quality, up to Exquisite */
static int shrine_combine(void) {
    for (int k = 0; k < NDEF; k++)
        for (int q = 1; q < RUNE_EXQ; q++) {
            int at[SHRINE_SLOTS], n = 0;
            for (int i = 0; i < SHRINE_SLOTS; i++) if (RUN.slot_def[i] == k + 1 && RUN.slot_q[i] == q) at[n++] = i;
            if (n < 3) continue;
            RUN.slot_q[at[0]] = (uint8_t)(q + 1);
            for (int j = 1; j < 3; j++) RUN.slot_def[at[j]] = RUN.slot_q[at[j]] = 0;
            for (int j = 0; j < 3; j++) G.sh_flash[at[j]] = 0.7f;
            snd_sfx_ex(SFX_SHIMMER, 1.0f, 0.5f); snd_sfx_ex(SFX_RUNE, 1.0f, 0.5f);
            shrine_note("THREE %s %s BECOME ONE %s", QUAL_NAME[q - 1], DEF_SHORT[k], QUAL_NAME[q]);
#ifdef CRAWL_DEBUG
            fprintf(stderr, "EVENT shrine_combine %s q=%d t=%.2f\n", DEF_NAME[k], q + 1, G.t);
#endif
            shrine_attune(k, q + 1);
            return 1;
        }
    return 0;
}
/* the runes of a pull arriving, and any three alike becoming one; 1 while that goes on */
static int shrine_step(float dt) {
    if ((G.sh_qt -= dt) > 0.0f) return 1;
    if (shrine_combine()) { G.sh_qt = 0.55f; return 1; }
    if (G.sh_nq <= 0) return 0;
    int k = G.sh_qk[0], q = G.sh_qq[0], at = -1;
    G.sh_nq--;
    for (int i = 0; i < G.sh_nq; i++) { G.sh_qk[i] = G.sh_qk[i + 1]; G.sh_qq[i] = G.sh_qq[i + 1]; }
    for (int i = 0; i < SHRINE_SLOTS && at < 0; i++) if (!RUN.slot_def[i]) at = i;
    if (at < 0) { shrine_note("NO ROOM: THE %s %s FADES", QUAL_NAME[q - 1], DEF_SHORT[k]); snd_sfx_ex(SFX_ZAP, 0.4f, 0.5f); }
    else {
        RUN.slot_def[at] = (uint8_t)(k + 1); RUN.slot_q[at] = (uint8_t)q; G.sh_flash[at] = 0.5f;
        snd_sfx_ex(SFX_RUNE, 0.7f, 0.5f);
        shrine_attune(k, q);
    }
    G.sh_qt = 0.4f;
    return 1;
}
static void shrine_tick(float dt) {                                /* every frame in the merchant's room */
    for (int i = 0; i < SHRINE_SLOTS; i++) G.sh_flash[i] = fmaxf(0.0f, G.sh_flash[i] - dt);
    G.sh_glow = fmaxf(0.0f, G.sh_glow - dt);
    G.sh_note_t = fmaxf(0.0f, G.sh_note_t - dt);
    if (G.sh_busy) G.sh_busy = shrine_step(dt);                    /* whoever is at the controls */
    if (!G.sh_busy && G.sh_mode == SM_PULL) G.sh_mode = SM_MENU;
}
static void shrine_take(int i) {                                   /* a take spent on slot i; the better of two is kept */
    int k = RUN.slot_def[i] - 1, q = RUN.slot_q[i];
    if (k < 0) return;
    int was = RUN.gear.def[k];
    if (q > was) RUN.gear.def[k] = (int8_t)q;
    RUN.slot_def[i] = RUN.slot_q[i] = 0;
    G.sh_takes--; G.sh_flash[i] = 0.5f; G.rune_glow[k] = 1.4f;
    snd_sfx_ex(SFX_RUNE, 1.0f, 0.5f); snd_sfx_ex(SFX_SHIMMER, 0.6f, 0.5f);
    shrine_note(q > was ? "TAKEN: %s %s" : "TAKEN, BUT YOURS WAS AS GOOD: %s %s", QUAL_NAME[q - 1], DEF_SHORT[k]);
    shrine_attune(k, q);
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT shrine_take %s q=%d had=%d t=%.2f\n", DEF_NAME[k], q, was, G.t);
#endif
}
static void shrine_letgo(int i) {
    if (!RUN.slot_def[i]) return;
    shrine_note("LET GO: %s %s", QUAL_NAME[RUN.slot_q[i] - 1], DEF_NAME[RUN.slot_def[i] - 1]);
    RUN.slot_def[i] = RUN.slot_q[i] = 0; G.sh_flash[i] = 0.4f;
    snd_sfx_ex(SFX_WHOOSH, 0.5f, 0.5f);
}

/* the autopilot at the shrine: what a rune would do for it; room made before a pull by letting go of
 * what it has no use for; the best rune taken after */
static float rune_gain(int k, int q) {
    Offer o;
    memset(&o, 0, sizeof o);
    o.kind = GK_DEF; o.id = k; o.tier = q;
    return gear_value(&RUN.gear, &o, G.level);
}
static void auto_make_room(int need) {
    while (shrine_count(0) < need) {
        int worst = -1; float ws = 1e9f;
        for (int i = 0; i < SHRINE_SLOTS; i++) {
            if (!RUN.slot_def[i]) continue;
            int k = RUN.slot_def[i] - 1, q = RUN.slot_q[i], same = 0;
            for (int j = 0; j < SHRINE_SLOTS; j++) same += RUN.slot_def[j] == k + 1 && RUN.slot_q[j] == q;
            float s = (float)q + (same >= 2 ? 3.0f : 0.0f) + (q > RUN.gear.def[k] ? 2.0f : 0.0f);   /* a pair waits for its third */
            if (s < ws) { ws = s; worst = i; }
        }
        if (worst < 0) break;
        shrine_letgo(worst);
    }
}
static void auto_take(void) {
    int best = -1; float bs = 0.0f;
    for (int i = 0; i < SHRINE_SLOTS; i++) {
        if (!RUN.slot_def[i]) continue;
        float v = rune_gain(RUN.slot_def[i] - 1, RUN.slot_q[i]);
        if (v > bs) { bs = v; best = i; }
    }
    if (best >= 0) shrine_take(best);
    else { G.sh_takes = 0; shrine_note("NOTHING IN THE SLOTS IT WANTS"); }
}
/* whether the autopilot has business at the shrine: cursing it with its first coin, raising it when it
 * can, otherwise pulling with three coins while it holds few runes, at the top rank, or once it has
 * half the next rank's price put by, and three more (so that it still saves up); two pulls a visit */
static int shrine_errand(void) {
    if (!RUN.shrine) return RUN.coins >= SHRINE_COST[0];
    if (RUN.shrine < NQUAL && RUN.coins >= SHRINE_COST[RUN.shrine]) return 1;
    if (RUN.coins < 3 || G.sh_pulls >= 2) return 0;
    return runes_held() < 3 || RUN.shrine >= NQUAL || RUN.coins >= SHRINE_COST[RUN.shrine] / 2 + 3;
}
static int auto_shrine_act(void) {                                 /* one deed; 1 if it began a pull */
    if (!RUN.shrine) { shrine_curse(); return 0; }
    if (RUN.shrine < NQUAL && RUN.coins >= SHRINE_COST[RUN.shrine]) { shrine_raise(); return 0; }
    auto_make_room(pull_most(3));
    shrine_pull(3, 0);
    return 1;
}

/* ---- the Hexed Totem (docs/PLAN-b7.md, section 7): each beaten boss's ability, unlocked with a scrip of
 * that boss and raised with more, one for rank I, two for II, three for III; scrip beyond what its ability
 * still needs sells there for 20,000 gold, asked twice. Bosses not yet beaten don't show. ---- */
#define SCRIP_PRICE 20000
static int boss_beaten(int b) { return RUN.rec.boss[b].slain > 0 || RUN.abil[b] > 0; }
static int totem_need(int b) { int n = 0; for (int r = RUN.abil[b] + 1; r <= ABIL_TOP; r++) n += r; return n; }
static int totem_extra(int b) { return imax(0, RUN.scrip[b] - totem_need(b)); }
static int totem_rows(int *t) { int n = 0; for (int b = 0; b < NBOSS; b++) if (boss_beaten(b)) t[n++] = b; return n; }
static void totem_raise(int b) {
    RUN.scrip[b] -= RUN.abil[b] + 1; RUN.abil[b]++;
    if (ab_on_mouse(b) && !ab_on_mouse(RUN.abil_sel)) RUN.abil_sel = b;   /* the first on the right mouse is readied */
    G.ab_cd[b] = 0.0f;
    static const char *const RANK[ABIL_TOP + 1] = { "", "I", "II", "III" };
    shrine_note(RUN.abil[b] == 1 ? "THE TOTEM TEACHES YOU %s" : "%s, RANK %s", BA_NAME[b], RANK[RUN.abil[b]]);
    snd_sfx_ex(SFX_RUNE, 1.0f, 0.5f); snd_sfx_ex(SFX_STINGER, 0.5f, 0.5f); snd_sfx_ex(SFX_WHISPER, 0.7f, 0.5f);
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT totem_raise %s to %d scrip left %d t=%.2f\n", BA_NAME[b], RUN.abil[b], RUN.scrip[b], G.t);
#endif
}
static void totem_sell(int b) {
    RUN.scrip[b]--; RUN.treasure += SCRIP_PRICE;
    shrine_note("A SCRIP SOLD: $%d", SCRIP_PRICE);
    snd_sfx_ex(SFX_COIN, 1.1f, 0.5f); snd_sfx_ex(SFX_WHISPER, 0.5f, 0.5f);
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT totem_sell %d left %d t=%.2f\n", b, RUN.scrip[b], G.t);
#endif
}
/* what an ability does at a rank, in a few lines */
static void ab_text(int b, int r, char *buf, int n) {
    static const char *const HOLD[ABIL_TOP] = { "A THIRD OF", "TWO THIRDS OF", "ALL OF" };
    r = clampi(r, 1, ABIL_TOP);
    float wait = BA_WAIT[b][r - 1];
    switch (b) {
    case BOSS_KNIGHT: snprintf(buf, (size_t)n, PADX.used ? "B, AND THE WAY THE LEFT STICK LEANS: DART UP TO FIVE CELLS, UNTOUCHABLE. AGAIN IN %.0f S."
                                                       : "TAP W, A, S OR D TWICE: DART UP TO FIVE CELLS THAT WAY, UNTOUCHABLE. AGAIN IN %.0f S.", wait); break;
    case BOSS_SPIDER: snprintf(buf, (size_t)n, PADX.used ? "LT: A WEB HOLDS WHAT YOU AIM AT, AND IT CLAWS AT THAT, NOT YOU. IT HOLDS %s YOUR HEALTH, %d AT ONCE. AGAIN IN %.0f S." : "RIGHT CLICK: A WEB HOLDS WHAT YOU AIM AT, AND IT CLAWS AT THAT, NOT YOU. IT HOLDS %s YOUR HEALTH, %d AT ONCE. AGAIN IN %.0f S.",
                               HOLD[r - 1], r, wait); break;
    case BOSS_DEACON: snprintf(buf, (size_t)n, PADX.used ? "LT AT THE REMAINS OF WHAT YOU SLEW: IT RISES AND FIGHTS FOR YOU %d S, %d AT ONCE. AGAIN IN %.0f S." : "RIGHT CLICK ON THE REMAINS OF WHAT YOU SLEW: IT RISES AND FIGHTS FOR YOU %d S, %d AT ONCE. AGAIN IN %.0f S.",
                               8 + 4 * r, r, wait); break;
    default: snprintf(buf, (size_t)n, PADX.used ? "LT: %.1f S THROUGH CREATURES AND THEIR MISSILES, AND THEY LOSE YOU. AGAIN IN %.0f S." : "RIGHT CLICK: %.1f S THROUGH CREATURES AND THEIR MISSILES, AND THEY LOSE YOU. AGAIN IN %.0f S.", 1.0f + 0.5f * (float)r, wait); break;
    }
}

/* ---- the Accursed Portal (docs/PLAN-b7.md, section 7): the start of any run opened (run II once run I's
 * boss has fallen on the way down, and so on), forward or back, as you are, into the merchant's room before
 * it; and the challenges: each boss beaten, fought again from its antechamber at its step, for its scrip, and
 * back to the merchant's room you left. A challenge won is not a run completed. ---- */
typedef struct { int run, boss; } PortRow;                         /* a run's start (boss -1), a challenge (run -1), or the levels
                                                                      cleared to replay (both -1) */
static const char *const SET_NAME[4] = { "THE CRYPTS", "THE CATACOMBS", "THE HOLLOWS", "THE DEEP" };
static const char *const RUN_ROMAN[8] = { "I", "II", "III", "IV", "V", "VI", "VII", "VIII" };
static int portal_rows(PortRow *r) {
    int n = 0;
    for (int k = imax(0, RUN.run_top - 5); k <= RUN.run_top; k++) r[n++] = (PortRow){ k, -1 };
    for (int b = 0; b < NBOSS; b++) if (RUN.rec.boss[b].slain > 0) r[n++] = (PortRow){ -1, b };
    if (RUN.rec.cleared) r[n++] = (PortRow){ -1, -1 };             /* the levels cleared, to replay (1.0.0b11) */
    return n;
}
/* replaying a level the record shows cleared (section 12): its number this time down, its price in souls, the next one cleared
 * from k in a direction */
static int replay_level(int l) { return l + DESCENT_LEN * crawl_loop(G.level); }
static int replay_price(int l) { return 5 + l; }
static int replay_step(int l, int d) {
    for (int k = 1; k <= DESCENT_LEN; k++) {
        int c = ((l - 1 + d * k) % DESCENT_LEN + DESCENT_LEN) % DESCENT_LEN + 1;
        if (RUN.rec.cleared >> (c - 1) & 1) return c;
    }
    return l;
}
static void replay_begin(int l) {                                  /* paid: her room before it, marked a replay */
    int from = G.level;
    spend_spirit((float)replay_price(l));
    G.ui = UI_NONE;
    if (RUN.gear.has_held && G.shop_held < 0) RUN.gear.has_held = 0;   /* as on leaving the room by its door */
    G.level = RUN.level = replay_level(l); RUN.hp = G.hp; RUN.shop_next = 1;
    begin_shop();
    RUN.replay_from = from;
    G.sdur = 0.0f; G.shop_look = -1; G.pc_t = -1.0f;
    snd_sfx_ex(SFX_WHOOSH, 1.0f, 0.5f); snd_sfx_ex(SFX_STINGER, 0.5f, 0.5f);
    say("A REPLAY: THE MERCHANT, THEN LEVEL %d", G.level);
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT replay level %d from %d souls %d t=%.2f\n", G.level, from, RUN.souls, G.t);
#endif
}
static int boss_level_of(int b) { return RUN_LEN * (b + 1) + DESCENT_LEN * crawl_loop(G.level); }   /* its level, this time down */
static void step_text(int st, char *buf, int n) {                  /* what a challenge's step brings */
    if (!st) { snprintf(buf, (size_t)n, "AS YOU FIRST MET IT, FROM ITS ANTECHAMBER. A WIN GIVES ONE OF ITS SCRIP."); return; }
    snprintf(buf, (size_t)n, "STEP %d: HEALTH +%d%%, BLOWS +%d%%, ITS CASTS %d%% MORE OFTEN AND MORE OF THEM%s. A WIN GIVES %s SCRIP.",
             st, 25 * st, 10 * st, 20 * st, st >= 4 ? ", ITS ADDS TWO RANKS UP" : (st >= 2 ? ", ITS ADDS A RANK UP" : ""), st >= 3 ? "TWO" : "ONE");
}
static void portal_run(int k) {                                    /* to the merchant's room before run k's first level */
    G.ui = UI_NONE; RUN.replay_from = 0;
    G.level = RUN.level = 1 + RUN_LEN * k; RUN.hp = G.hp; RUN.shop_next = 1;
    begin_shop();
    G.sdur = 0.0f; G.shop_look = -1; G.pc_t = -1.0f;
    snd_sfx_ex(SFX_WHOOSH, 1.0f, 0.5f); snd_sfx_ex(SFX_STINGER, 0.5f, 0.5f);
    say("RUN %s, %s: THE MERCHANT, THEN LEVEL %d", RUN_ROMAN[k % 8], SET_NAME[k % 4], G.level);
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT portal_run %d level %d t=%.2f\n", k, G.level, G.t);
#endif
}
/* a challenge (again, after a death in it): the boss's level, the caster in its antechamber, the way back
 * up into the level sealed, so that the fight is all there is */
static void challenge_begin(int b) {
    G.ui = UI_NONE;
    G.bchallenge = 1; G.level = boss_level_of(b);
    start_level();
    for (int x = 1; x < MAPN - 1; x++)                             /* the way down into the antechamber: fallen in */
        if (G.map[BAND_Y0 - 1][x].kind != K_WALL && G.map[BAND_Y0][x].kind != K_WALL) {
            Cell *c = &G.map[BAND_Y0 - 1][x];
            c->kind = K_WALL; c->tex = (uint8_t)(G.faced ? T_MATV(M1_RUBBLE, x & 1) : T_RUBBLE); c->door = -1;
        }
    if (G.bstart_x >= 0) {
        G.cx = G.tx = G.bstart_x; G.cy = G.ty = G.bstart_y; G.px = G.cx + 0.5f; G.py = G.cy + 0.5f;
        G.h = G.next_h = 0; G.ang = 0.0f; G.u = 1.0f;
    }
    G.map[G.cy][G.cx].visits = 1;
    if (G.player) human_begin(); else { G.leaving = 1; start_move(); }   /* the autopilot goes in at once */
    say("A CHALLENGE, AT STEP %d", G.bstep);
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT challenge %s step %d level %d from %d t=%.2f\n", BOSSES[b].name, G.bstep, G.level, RUN.chal_from, G.t);
#endif
}

static void shop_human(float dt);
static void update_ui(void);

static void comp_auto(void);
/* The autopilot at the Dead Bell and the Weeping Font (section 12): a coin curses each, once for good, before the shrine
 * has them all; it tolls when the price leaves it a fifth of its vessel, never for a boss's level nor for the secrets; it
 * drinks when it has the souls to spare after the bell. Never on a person's run. Through the arch and back, the bell first,
 * then the shrine */
static int bell_price(void);
static int font_price(void);
static int bell_tolled(void);
static void bell_toll(int shown);
static void wfont_drink(void);
static int bell_wanted(void) {
    if (LEDGER.yours || LEDGER.kept || boss_of_level(G.level) >= 0) return 0;
    if (!RUN.bell_cursed) return RUN.coins >= 1;
    return !bell_tolled() && spirit() - (float)bell_price() >= (float)vessel() / 5.0f;
}
static int font_wanted(void) {
    if (LEDGER.yours || LEDGER.kept) return 0;
    if (!RUN.font_cursed) return RUN.coins >= 1 + (!RUN.bell_cursed && boss_of_level(G.level) < 0);
    float after = spirit() - (bell_wanted() && RUN.bell_cursed ? (float)bell_price() : 0.0f);
    return !G.font_drunk && RUN.font_next != G.level && after - (float)font_price() >= (float)vessel() / 5.0f;
}
static int hall_errand(void) { return bell_wanted() || font_wanted(); }
static void hall_act(int bell) {                                   /* at the one it went to: a coin, or the souls */
    if (bell ? !RUN.bell_cursed : !RUN.font_cursed) {
        RUN.coins--;
        if (bell) RUN.bell_cursed = 1; else RUN.font_cursed = 1;
        int dc = bell ? G.bell_deco : G.font_deco;
        if (dc >= 0 && dc < G.ndecos) G.decos[dc].spr = bell ? SP_DBELL_C : SP_FONT_C;
        snd_sfx_ex(SFX_COIN, 0.8f, 0.5f); snd_sfx_ex(SFX_WAIL, 0.5f, 0.5f);
#ifdef CRAWL_DEBUG
        fprintf(stderr, "EVENT %s by autopilot t=%.2f\n", bell ? "bell_corrupted" : "font_cursed", G.t);
#endif
    }
    if (bell && bell_wanted()) bell_toll(0);
    if (!bell && font_wanted()) wfont_drink();
}
static void shop_after(void) {                                     /* the wares done with: the shrine, if it has business there */
    G.shop_t = 0.0;
    if (!G.comp_tried) { G.comp_tried = 1; comp_auto(); }          /* and the Compendium, once */
    if (hall_errand()) { G.shop_step = SH_HALL; G.hall_step = 1; shop_walk(20.5f, 12.4f, 1.8f); }   /* to the arch, the bell, the font */
    else if (shrine_errand()) { G.shop_step = SH_SHRINE_GO; G.sh_auto = 0; shop_walk(SHRINE_X, SHRINE_Y - 1.7f, 1.6f); }
    else { G.shop_step = SH_THANK; shop_turn(face_of(CAT_X, CAT_Y), 0.7f); }
}

static void update_shop(double dt) {
    float fdt = (float)dt;
    G.shop_t += dt;
    float st = (float)G.shop_t;
    shop_cat(fdt);
    shrine_tick(fdt);
    if (G.player) {
        if (G.ui) {                                                /* a menu up: the room still comes in out of the dark behind it */
            update_ui();
            if (!SHOP_BEYOND()) G.fade = smoothstepf(0.0f, 0.7f, (float)G.level_t);
            build_lightmap();
            return;
        }
        if (!WH.open && WH.k <= 0.0f) shop_human(fdt);            /* a wheel up, or going: she and her wares wait, so the press
                                                                      that closed it buys nothing (1.0.0b18.1) */
        G.sdur = 0.0f;
    }
    if (G.sdur > 0.0f) {                                           /* the current turn or walk */
        G.smove += 0.0f;
        float k = smoothstepf(0.0f, 1.0f, fminf(1.0f, st / G.sdur));
        G.ang = lerpf(G.sa0, G.sa1, k);
        float w = fminf(1.0f, st / G.sdur);
        if (G.sx0 != G.sx1 || G.sy0 != G.sy1) {
            G.px = lerpf(G.sx0, G.sx1, w); G.py = lerpf(G.sy0, G.sy1, w);
            G.bobph += fdt * 9.0f; G.bob = sinf(G.bobph) * 0.012f;
            if (G.sx1 != G.sx0) { G.ang = G.sa0 + wrap_pi(atan2f(G.sy1 - G.sy0, G.sx1 - G.sx0) - G.sa0) * fminf(1.0f, st / 0.3f); }
        }
        G.cx = (int)G.px; G.cy = (int)G.py;
    }
    G.camz = 0.5f + G.bob;
    if (G.shop_gold != RUN.treasure) {                             /* the purse counting down */
        int d = RUN.treasure - G.shop_gold, step = abs(d) / 12 + 1;
        G.shop_gold += d > 0 ? (step < d ? step : d) : -(step < -d ? step : -d);
    }
    G.sparkle = fmaxf(0.0f, G.sparkle - fdt);
    G.grin_t = fmaxf(0.0f, G.grin_t - fdt);
    if (G.hp < G.shop_heal && st > 0.5f && (G.regen_acc += fdt * 12.0f) >= 1.0f) { int h = (int)G.regen_acc; G.regen_acc -= h; G.hp = clampi(G.hp + h, 0, G.shop_heal); }
    switch (G.player ? -1 : G.shop_step) {
    case -1: break;                                                /* a person is choosing */
    case SH_GREET:                                                 /* the merchant first: a slow blink */
        if (st > 0.8f && st - fdt <= 0.8f) G.cat_slow = 1.4f;
        if (st >= 2.4f) {
            if (RUN.peds < NPED && RUN.treasure >= ped_price() + (int)gear_haul(G.level)) ped_buy();   /* another, if a level's haul is left after */
            auto_ascend(); auto_align(); auto_augment();   /* its augments as their steps open (A15) */
            G.shop_step = SH_LOOK; G.shop_look = 0; G.shop_t = 0.0; shop_turn(face_of(G.ped_x[0], G.ped_y[0]), 0.7f);
        }
        break;
    case SH_LOOK: {                                                /* each ware in turn */
        float hold = 0.7f + 1.0f + 0.5f * (float)(hash2(G.level, G.shop_look) % 100) / 100.0f;
        if (st >= hold) {
            if (++G.shop_look >= G.ped_n) { G.shop_look = -1; G.shop_step = SH_THINK; G.shop_t = 0.0; shop_turn(face_of(SHOP_MID, 13.0f), 0.5f); }
            else { G.shop_t = 0.0; shop_turn(face_of(G.ped_x[G.shop_look], G.ped_y[G.shop_look]), 0.45f); }
        }
        break;
    }
    case SH_THINK:
        if (st >= 1.1f) {
            G.shop_pick = shop_choose();
            G.shop_t = 0.0;
            if (G.shop_pick >= 0) {
                int i = G.shop_pick;
                G.shop_look = i;
                shop_walk(G.ped_x[i] + (SHOP_MID - G.ped_x[i]) * 0.25f, G.ped_y[i] + 1.0f, 1.6f);
                G.shop_step = SH_GO;
            } else shop_after();
#ifdef CRAWL_DEBUG
            fprintf(stderr, "EVENT shop_pick %d saving=%d goal=%d t=%.2f\n", G.shop_pick, G.shop_saving, RUN.goal, G.t);
#endif
        }
        break;
    case SH_GO:
        if (st >= G.sdur + 0.15f) { G.shop_step = SH_BUY; G.shop_t = 0.0; shop_turn(face_of(G.ped_x[G.shop_pick], G.ped_y[G.shop_pick]), 0.3f); }
        break;
    case SH_BUY: {                                                 /* reach out and take it */
        G.hand = st < 0.35f ? st / 0.35f : (st > 1.05f ? fmaxf(0.0f, 1.0f - (st - 1.05f) / 0.3f) : 1.0f);
        if (G.shop_pick >= 0 && !G.offer_gone[G.shop_pick] && st >= 0.5f) shop_purchase(G.shop_pick);
        if (st >= 1.5f) {
            G.hand = 0.0f; G.shop_t = 0.0;
            if (G.shop_left > 0) { G.shop_step = SH_THINK; shop_turn(face_of(SHOP_MID, 13.0f), 0.5f); }   /* another, deeper down */
            else shop_after();
        }
        break;
    }
    case SH_SHRINE_GO:
        if (st >= G.sdur + 0.1f) { G.shop_step = SH_SHRINE; G.shop_t = 0.0; shop_turn(face_of(SHRINE_X, SHRINE_Y), 0.35f); }
        break;
    case SH_SHRINE:                                                /* a hand on the stone while its business is done */
        G.hand = 0.55f * fminf(1.0f, st / 0.35f);
        if (G.sh_auto) {                                           /* a pull coming in, then the best of it taken */
            if (G.sh_busy) break;
            auto_take(); G.sh_auto = 0; G.shop_t = 0.0;
            break;
        }
        if (st < 1.0f) break;
        if (shrine_errand()) { G.sh_auto = auto_shrine_act(); G.shop_t = 0.0; break; }
        G.hand = 0.0f; G.shop_t = 0.0;
        G.shop_step = SH_THANK; shop_turn(face_of(CAT_X, CAT_Y), 0.7f);
        break;
    case SH_HALL:                                                  /* through the arch, to the bell and the font, and back */
        if (st < G.sdur + 0.15f) break;
        G.shop_t = 0.0;
        switch (G.hall_step) {
        case 1: G.hall_step = 2; shop_walk(20.5f, 8.6f, 1.6f); break;   /* in under the arch */
        case 2:
            if (bell_wanted()) { G.hall_step = 3; shop_walk(BELL_X + 0.5f, BELL_Y + 1.9f, 1.4f); }
            else if (font_wanted()) { G.hall_step = 5; shop_walk(FONT_X + 0.5f, FONT_Y + 1.9f, 1.4f); }
            else { G.hall_step = 7; shop_walk(20.5f, 8.6f, 1.6f); }
            break;
        case 3: G.hall_step = 4; shop_turn(face_of(BELL_X + 0.5f, BELL_Y + 0.5f), 0.4f); break;
        case 4: G.hand = 0.5f; hall_act(1); G.hand = 0.0f;
                if (font_wanted()) { G.hall_step = 5; shop_walk(FONT_X + 0.5f, FONT_Y + 1.9f, 1.4f); } else { G.hall_step = 7; shop_walk(20.5f, 8.6f, 1.6f); }
                break;
        case 5: G.hall_step = 6; shop_turn(face_of(FONT_X + 0.5f, FONT_Y + 0.5f), 0.4f); break;
        case 6: hall_act(0); G.hall_step = 7; shop_walk(20.5f, 8.6f, 1.6f); break;
        case 7: G.hall_step = 8; shop_walk(20.5f, 12.4f, 1.6f); break;   /* and back out under the arch */
        default:                                                   /* then the shrine, if it has business there */
            G.hall_step = 0;
            if (shrine_errand()) { G.shop_step = SH_SHRINE_GO; G.sh_auto = 0; shop_walk(SHRINE_X, SHRINE_Y - 1.7f, 1.6f); }
            else { G.shop_step = SH_THANK; shop_turn(face_of(CAT_X, CAT_Y), 0.7f); }
            break;
        }
        break;
    case SH_THANK:                                                 /* a look back at the merchant */
        if (!G.shop_bought && st > 0.9f && st - fdt <= 0.9f) G.cat_slow = 1.4f;
        if (st >= 2.1f) { G.shop_step = SH_LEAVE; G.shop_t = 0.0; shop_walk(SHOP_DOOR - 0.8f, 15.5f, 1.8f); }
        break;
    default: {                                                     /* out through the door */
        Door *d = &G.doors[0];
        if (hypotf(d->x + 0.5f - G.px, d->y + 0.5f - G.py) < 2.2f && d->open < 1.0f) d->opening = 1;
        if (st >= G.sdur && G.sx1 < SHOP_DOOR) { G.shop_t = 0.0; shop_walk(SHOP_DOOR + 3.5f, 15.5f, 1.8f); }
        break;
    }
    }
    if (SHOP_BEYOND()) {                                           /* through the door and into the level */
        G.fade = 1.0f - smoothstepf(SHOP_DOOR + 1.4f, SHOP_DOOR + 2.6f, G.px);
        if (G.px >= SHOP_DOOR + 2.6f) {
            if (G.shop_held < 0) RUN.gear.has_held = 0;            /* last visit's hold, bought or passed over, is gone */
            start_level(); return;
        }
    }
    update_doors(fdt);
    if (!SHOP_BEYOND()) G.fade = smoothstepf(0.0f, 0.7f, (float)G.level_t);
    G.mood_t -= fdt;
    if (G.mood_t <= 0.0f) { snd_set_mood(0.6f); G.mood_t = 0.5f; }
    G.gift_glow = fmaxf(0.0f, G.gift_glow - fdt);
    build_lightmap();
}

