/* A part of scene_crawl.c (docs/PLAN-tidy.md): it is #included there in its place and never compiled on its own.
 * The caster's own: the Unholy Confessions, the Alignment (align_*, caster_sync), confessing, the classes' sheets and caps. */

/* ---- the Unholy Confessions (1.0.0b20, docs/PLAN-combat.md section 10): six in place of the lines above, five ranks each
 * bought a point at a time (10, 20, 30, 40, 50: every point moves all of a Confession's numbers its share of the way to the next
 * rank), each number at rank I and at V and evenly between; past V an overlevel adds a quarter of I to V again. A class takes
 * each at a share by its place in the guide's lists (10.6), never named on the screen ---- */
typedef struct { float lo, hi, zero; int from1; } ConfNum;        /* at rank I, at V; what it is with nothing confessed (or before I) */
typedef struct { const char *name; int n; ConfNum v[5]; } Conf;
static const Conf CONF[NCONF] = {
    [CF_RESTORE]  = { "RESTORATION", 3, { { 0.0025f, 0.01f, 0, 1 }, { 0.7f, 0.1f, 0.7f, 1 }, { 10, 50, 0, 0 } } },
    [CF_AGILITY]  = { "AGILITY", 4, { { 0.30f, 0.85f, 0, 0 }, { 2, 10, 0, 0 }, { 3, 15, 0, 0 }, { 5, 25, 0, 0 } } },
    [CF_SPIRIT]   = { "SPIRITUALITY", 5, { { 0.10f, 0.50f, 0, 0 }, { 0.30f, 0.85f, 0, 0 }, { 2, 10, 0, 0 }, { 3, 15, 0, 0 }, { 4, 20, 0, 0 } } },
    [CF_CONST]    = { "CONSTITUTION", 4, { { 0.10f, 0.50f, 0, 0 }, { 0.5f, 2.5f, 0, 0 }, { 15, 100, 0, 0 }, { 0.05f, 0.25f, 0, 1 } } },
    [CF_SURVIVAL] = { "SURVIVAL", 3, { { 0.15f, 0.60f, 0, 0 }, { 3, 15, 0, 0 }, { 0.5f, 2.5f, 0, 0 } } },
    [CF_TOUGH]    = { "TOUGHNESS", 3, { { 2, 14, 0, 0 }, { 0.05f, 0.25f, 0, 0 }, { 4, 8, 2, 1 } } },
};
static const int CONF_CUM[6] = { 0, 10, 30, 60, 100, 150 };      /* the points that make ranks I-V */
#define CONF_TOP 150
static const uint8_t CONF_W[NELEM][NCONF] = {                     /* each class's share, in percent, by its place (10.6) */
    { 90, 80, 100, 70, 50, 60 },                                   /* the Witchfinder */
    { 80, 50, 70, 80, 60, 100 },                                   /* the Anatomist */
    { 60, 100, 80, 60, 90, 50 },                                   /* the Galvanist */
    { 70, 90, 50, 90, 100, 80 },                                   /* the Plague Doctor */
    { 100, 60, 90, 50, 80, 70 },                                   /* the Sin-Eater */
    { 50, 70, 60, 100, 70, 90 },                                   /* the Headsman */
};
static int conf_on(void) {                                         /* the Confessions in place of the old lines (the sheet approved, 8
                                                                      November); CRAWL_LINES=1 in the debug build puts the old back */
#ifdef CRAWL_DEBUG
    static int v = -1;
    if (v < 0) v = getenv("CRAWL_LINES") == 0;
    return v;
#else
    return 1;
#endif
}
static float conf_weight(int cf) { int e = cls_el(); return e >= 0 ? (float)CONF_W[e][cf] / 100.0f : 0.75f; }   /* no past yet: the
                                                                      places' mean */
static int conf_rank(int cf) {
    if (cf < 0 || cf >= NCONF) return 0;
    int p = RUN.conf[cf], r = 0;
    while (r < 5 && p >= CONF_CUM[r + 1]) r++;
    return r == 5 ? 5 + RUN.conf_over[cf] : r;
}
static float conf_raw(int cf, int which, int p, int over) {       /* the number at p points (and overlevels), before the class's share */
    const ConfNum *c = &CONF[cf].v[which];
    float step = (c->hi - c->lo) / 4.0f;
    if (p >= CONF_TOP) return c->hi + step * (float)over;
    if (p < CONF_CUM[1] && c->from1) return 0.0f;                  /* not open till rank I is whole */
    int k = 1;
    while (p > CONF_CUM[k]) k++;                                   /* in the step from rank k-1 to k */
    float a = k == 1 ? c->zero : c->lo + step * (float)(k - 2), b = c->lo + step * (float)(k - 1);
    return a + (b - a) * (float)(p - CONF_CUM[k - 1]) / (float)(CONF_CUM[k] - CONF_CUM[k - 1]);
}
static float conf_at(int cf, int which, int p, int over) {        /* what the caster would have at p points: the class's share in it */
    if (cf < 0 || cf >= NCONF || which < 0 || which >= CONF[cf].n || p <= 0) return 0.0f;
    const ConfNum *c = &CONF[cf].v[which];
    float raw = conf_raw(cf, which, p, over);
    if (c->from1 && p < CONF_CUM[1]) return 0.0f;
    float v = c->zero + (raw - c->zero) * conf_weight(cf);
    if ((cf == CF_AGILITY && which == CV_HARD) || (cf == CF_SPIRIT && which == CV_UNBOUND) || (cf == CF_SURVIVAL && which == CV_SOFT))
        v = fminf(v, 0.9f);                                        /* a status cut never takes it all */
    if (cf == CF_CONST && which == CV_LASTBLOW) v = fminf(v, 0.3f);   /* the owner's cap (CP4) */
    if (cf == CF_RESTORE && which == CV_HEAL_SOULS) v = fmaxf(v, 0.0f);
    return v;
}
static float conf_value(int cf, int which) {
    return cf >= 0 && cf < NCONF ? conf_at(cf, which, RUN.conf[cf], RUN.conf_over[cf]) : 0.0f;
}
static const int8_t CONF_STAT[NCONF][5] = {                       /* the stat each number joins, -1 none (it is felt where it is used) */
    [CF_RESTORE]  = { -1, -1, ST_PHYSIC, -1, -1 },
    [CF_AGILITY]  = { -1, ST_DODGE, ST_MOVE, ST_QUIET, -1 },
    [CF_SPIRIT]   = { -1, -1, ST_HASTE, ST_THRIFT, ST_EFFECT },
    [CF_CONST]    = { -1, ST_SPRINT, ST_HP, -1, -1 },
    [CF_SURVIVAL] = { -1, ST_TREASURE, ST_LUCK, -1, -1 },
    [CF_TOUGH]    = { ST_ARMOR, -1, -1, -1, -1 },
};
static void conf_own(float *own, float *up) {                      /* the Confessions into the caster's own, under the same caps; past V
                                                                      the caps go up with what an overlevel adds */
    if (!conf_on()) return;
    for (int cf = 0; cf < NCONF; cf++) {
        if (!RUN.conf[cf]) continue;
        for (int i = 0; i < CONF[cf].n; i++) {
            int st = CONF_STAT[cf][i];
            if (st < 0) continue;
            float v = conf_value(cf, i);
            own[st] += v;
            if (RUN.conf_over[cf]) up[st] += v - conf_at(cf, i, RUN.conf[cf], 0);
        }
    }
    float worn = conf_value(CF_TOUGH, CV_ARMOUR_WORN);            /* Toughness: a share more of the armour worn */
    if (worn > 0.0f) own[ST_ARMOR] += worn * gear_total_raw(&RUN.gear, ST_ARMOR);
}
static float conf_cut(int k) {                                     /* a share off status k's chance and its length */
    if (k < 0) return 0.0f;
    if (k == AF_DOWN || k == AF_STUN || k == AF_FROZEN)            /* Agility's, and what is worn (D6's gear_cut), under the one cap */
        return fminf((conf_on() ? conf_value(CF_AGILITY, CV_HARD) : 0.0f) + gear_cut(k), 0.9f);
    if (!conf_on()) return 0.0f;
    if (k == AF_UNBOUND) return conf_value(CF_SPIRIT, CV_UNBOUND);
    return conf_value(CF_SURVIVAL, CV_SOFT);                       /* the rest: fire, chill, poison */
}
static float soft_pace(void) { return 1.0f / (1.0f - conf_cut(AF_POISON)); }   /* the old clocks (grabs, webs, bile, the bats' sap):
                                                                      Survival runs them down sooner */
static float soul_share(void) { return 1.0f + (conf_on() ? conf_value(CF_SPIRIT, CV_SOULS) : 0.0f) + class_extra(KX_SOULS); }
#ifdef CRAWL_DEBUG
static void conf_debug(const char *s) {                            /* CRAWL_CONF=dddddd: each Confession's rank, 0-5 (6-9: V and
                                                                      overlevels); or a,b,c,d,e,f: each one's points, to 150 */
    if (strchr(s, ',')) {
        for (int cf = 0; cf < NCONF && *s; cf++) { RUN.conf[cf] = (uint8_t)clampi(atoi(s), 0, CONF_TOP); s = strchr(s, ','); if (!s) break; s++; }
        return;
    }
    for (int cf = 0; cf < NCONF && s[cf] >= '0' && s[cf] <= '9'; cf++) {
        int d = s[cf] - '0';
        RUN.conf[cf] = (uint8_t)CONF_CUM[imin(d, 5)]; RUN.conf_over[cf] = (uint8_t)imax(0, d - 5);
    }
}
#endif
/* ---- the Alignment (docs/PLAN-b11.md, sections 7-10): a line for each element, its spell opened and raised with
 * Alignment Points. A node is bought whole, but Efficiency, Insight and the like a point at a time; every point in
 * a line after its opening is a rank more. Only the class's line comes open (1.0.0b16; Fire did before). ---- */
enum { NK_OPEN, NK_EFF, NK_INS, NK_ABIL, NK_STEP, NK_MASTER, NK_ULT };   /* what a node is */
typedef struct { const char *name; int cost, kind; float val, add; const char *does; } ANode;   /* val: a point's, or the Mastery's Power; add: its Addition */
typedef struct { const char *name; ANode n[NNODE]; } ALine;
static const int LINE_ATK[NELEM] = { AT_FIRE, AT_FROST, AT_CHAIN, AT_FLASK, AT_DRAIN, AT_SPEAR };
static const ALine ALIGN[NELEM] = {
    { "FIRE", { { "FIRE", 3, NK_OPEN, 0, 0, "THE FIREBALL: A BOLT OF FIRE THAT BURSTS WHERE IT STRIKES." },
        { "EFFICIENCY", 2, NK_EFF, 0.05f, 0, "FIRE COSTS 5% LESS SPIRIT A POINT." },
        { "INSIGHT", 2, NK_INS, 0.15f, 0, "FIRE GATHERS 15% QUICKER A POINT." },
        { "FIRESTARTER", 3, NK_ABIL, 0, 0, "THE FIREBALL SETS THE FLOOR ALIGHT ALONG ITS FLIGHT; WHAT WALKS THROUGH IT CATCHES FIRE." },
        { "EFFICIENCY II", 2, NK_EFF, 0.10f, 0, "FIRE COSTS 10% LESS SPIRIT A POINT." },
        { "SLOW EMBERS", 2, NK_STEP, 0.5f, 0, "WHAT BURNS BURNS HALF AS LONG AGAIN, A POINT." },   /* BURNING INTENSITY till 1.0.0b21: the
                                                                      name went to Fire's first augment (PLAN-combat A10) */
        { "FLAMEWALKER", 5, NK_ULT, 0, 0, "WALKING THROUGH YOUR FLAMES GIVES YOU HALF YOUR ARMOUR AGAIN FOR FIVE SECONDS." },
        { "INSIGHT II", 2, NK_INS, 0.15f, 0, "FIRE GATHERS 15% QUICKER A POINT." },
        { "FIRE MASTERY", 5, NK_MASTER, 0.25f, 0, "FIRE HITS A QUARTER HARDER." },
        { "FIREWALL", 5, NK_ULT, 0, 0, "HOLD TO CALL THREE FLAME SPIRITS ROUND YOU; WHAT THEY TOUCH BURNS." } } },
    { "FROST", { { "FROST", 3, NK_OPEN, 0, 0, "FROST SHARD: A SHARD THAT SLOWS WHAT IT HITS." },
        { "EFFICIENCY", 2, NK_EFF, 0.05f, 0, "FROST COSTS 5% LESS SPIRIT A POINT." },
        { "INSIGHT", 2, NK_INS, 0.15f, 0, "FROST GATHERS 15% QUICKER A POINT." },
        { "DEEP CHILL", 3, NK_ABIL, 0, 0, "EVERY SHARD CHILLS; THE THIRD ON ONE CREATURE IN FOUR SECONDS FREEZES IT." },
        { "EFFICIENCY II", 2, NK_EFF, 0.10f, 0, "FROST COSTS 10% LESS SPIRIT A POINT." },
        { "PERMAFROST", 2, NK_STEP, 0.5f, 0, "SLOWING AND FREEZING LAST HALF AS LONG AGAIN, A POINT." },
        { "WINTER'S LANCE", 5, NK_ULT, 0, 0, "HOLD TO CHARGE A LANCE THROUGH EVERYTHING IN ITS LINE; IT FREEZES, AND RIMES THE FLOOR." },
        { "INSIGHT II", 2, NK_INS, 0.20f, 0, "FROST GATHERS 20% QUICKER A POINT." },
        { "FROST MASTERY", 5, NK_MASTER, 0.25f, 0.10f, "FROST HITS A QUARTER HARDER, AND A TENTH OF IT GOES WITH EVERY OTHER SPELL." },
        { "SHATTER", 5, NK_ULT, 0, 0, "A FROZEN CREATURE STRUCK SHATTERS FOR DOUBLE, ITS SHARDS CUTTING ALL ROUND IT." } } },
    { "LIGHTNING", { { "LIGHTNING", 3, NK_OPEN, 0, 0, "A BOLT FROM THE HAND THAT STRIKES WHAT YOU AIM AT, AT ONCE." },
        { "EFFICIENCY", 2, NK_EFF, 0.05f, 0, "LIGHTNING COSTS 5% LESS SPIRIT A POINT." },
        { "INSIGHT", 2, NK_INS, 0.15f, 0, "LIGHTNING GATHERS 15% QUICKER A POINT." },
        { "IONIZATION", 3, NK_ABIL, 0, 0, "LIGHTNING LEAVES A TRAIL THAT HURTS WHAT CROSSES IT, HALF A SECOND." },
        { "EFFICIENCY II", 2, NK_EFF, 0.10f, 0, "LIGHTNING COSTS 10% LESS SPIRIT A POINT." },
        { "LIGHTNING MASTERY", 5, NK_MASTER, 0.30f, 0.20f, "LIGHTNING HITS 30% HARDER, AND A FIFTH OF IT GOES WITH EVERY OTHER SPELL." },
        { "ARC", 5, NK_ULT, 0, 0, "THE BOLT LEAPS ON TO THE NEAREST, UP TO FOUR LEAPS, EACH A QUARTER WEAKER." },
        { "INSIGHT II", 2, NK_INS, 0.20f, 0, "LIGHTNING GATHERS 20% QUICKER A POINT." },
        { "LIGHTNING MASTERY", 5, NK_MASTER, 0.20f, 0.10f, "LIGHTNING HITS A FIFTH HARDER, AND A TENTH OF IT GOES WITH EVERY OTHER SPELL." },
        { "AMPERAGE", 5, NK_ULT, 0, 0, "HOLD TO CHARGE AN ORB THAT GOES THROUGH ALL IT MEETS, ARCING TO ONE MORE FROM EACH." } } },
    { "POISON", { { "POISON", 3, NK_OPEN, 0, 0, "THE BLIGHT FLASK: LOBBED, IT BURSTS AND SICKENS WHAT IT SPLASHES." },
        { "EFFICIENCY", 2, NK_EFF, 0.05f, 0, "POISON COSTS 5% LESS SPIRIT A POINT." },
        { "INSIGHT", 2, NK_INS, 0.15f, 0, "POISON GATHERS 15% QUICKER A POINT." },
        { "SEEP", 3, NK_ABIL, 0, 0, "THE SPLASH LEAVES A POOL OF VENOM TWO SECONDS; WHAT WALKS IN IT IS POISONED AGAIN." },
        { "EFFICIENCY II", 2, NK_EFF, 0.10f, 0, "POISON COSTS 10% LESS SPIRIT A POINT." },
        { "LINGERING", 2, NK_STEP, 0.5f, 0, "THE POOL LASTS HALF AS LONG AGAIN, A POINT." },
        { "PLAGUE PIT", 5, NK_ULT, 0, 0, "HOLD TO CHARGE: THE POOL GROWS WITH IT, LASTS SIX SECONDS AND SLOWS WHAT WADES IN." },
        { "INSIGHT II", 2, NK_INS, 0.20f, 0, "POISON GATHERS 20% QUICKER A POINT." },
        { "POISON MASTERY", 5, NK_MASTER, 0.25f, 0.10f, "POISON HITS A QUARTER HARDER, AND A TENTH OF EVERY OTHER SPELL GOES IN AS POISON." },
        { "CONTAGION", 5, NK_ULT, 0, 0, "WHAT DIES POISONED PASSES IT ON TO ALL WITHIN TWO CELLS." } } },
    { "SPIRIT", { { "SPIRIT", 3, NK_OPEN, 0, 0, "SOUL DRAIN: A BEAM THAT HURTS, AND HEALS YOU FOR SOME OF IT." },
        { "EFFICIENCY", 2, NK_EFF, 0.05f, 0, "SPIRIT COSTS 5% LESS SPIRIT A POINT." },
        { "INSIGHT", 2, NK_INS, 0.15f, 0, "SPIRIT GATHERS 15% QUICKER A POINT." },
        { "HARVEST", 3, NK_ABIL, 0, 0, "WHAT SOUL DRAIN SLAYS LEAVES A SOUL MORE, TWO FROM RANK III." },
        { "EFFICIENCY II", 2, NK_EFF, 0.10f, 0, "SPIRIT COSTS 10% LESS SPIRIT A POINT." },
        { "COMMUNION", 2, NK_STEP, 0.25f, 0, "SOUL DRAIN HEALS A QUARTER MORE, A POINT." },
        { "SOUL TETHER", 5, NK_ULT, 0, 0, "HOLD TO KEEP THE BEAM ON, DRAINING AND SLOWING; IT LEAPS ON WHEN WHAT IT HOLDS DIES." },
        { "INSIGHT II", 2, NK_INS, 0.20f, 0, "SPIRIT GATHERS 20% QUICKER A POINT." },
        { "SPIRIT MASTERY", 5, NK_MASTER, 0.25f, 0.10f, "SPIRIT HITS A QUARTER HARDER, A TENTH OF IT GOES WITH EVERY OTHER SPELL, AND IT HARMS SPECTRES." },
        { "WISP HOST", 5, NK_ULT, 0, 0, "EVERY DRAIN LOOSES THREE WISPS THAT HUNT THE NEAREST ROUND CORNERS." } } },
    { "PHYSICAL", { { "PHYSICAL", 3, NK_OPEN, 0, 0, "THE BONE SPEAR: THROUGH ALL IN A LINE, A BONE A SPEAR, NO SPIRIT." },
        { "EFFICIENCY", 2, NK_EFF, 0.05f, 0, "5% A POINT THAT A SPEAR SPENDS NO BONE; FIVE MORE BONES CARRIED." },
        { "INSIGHT", 2, NK_INS, 0.15f, 0, "THE SPEAR GATHERS 15% QUICKER A POINT." },
        { "IMPALE", 3, NK_ABIL, 0, 0, "THE FIRST A SPEAR STRIKES IS PINNED, AND CAN'T MOVE OR STRIKE, 0.6 S." },
        { "BONE PLATE", 2, NK_STEP, 0.10f, 0, "10% MORE ARMOUR A POINT." },
        { "OSSIFIED", 5, NK_MASTER, 0, 0, "SPEARS ADD HALF YOUR ARMOUR TO THEIR DAMAGE, AND EACH THAT STRIKES GIVES A POINT OF ARMOUR." },
        { "PHALANX", 5, NK_ULT, 0, 0, "HOLD TO CHARGE: A FAN OF SPEARS, THREE AND THEN FIVE, FOR ONE BONE." },
        { "INSIGHT II", 2, NK_INS, 0.20f, 0, "THE SPEAR GATHERS 20% QUICKER A POINT." },
        { "PHYSICAL MASTERY", 5, NK_MASTER, 0.30f, 0, "SPEARS HIT 30% HARDER." },
        { "BALLISTA", 5, NK_ULT, 0, 0, "HOLD TO CHARGE A HEAVY SPEAR THAT KNOCKS BACK AND STUNS ALL IT PASSES." } } },
};
/* The classes (docs/PLAN-b11.md, section 11): what the caster was, by its element. Its line is open free and it
 * brings its gifts; a life keeps one, asked in her room at a fresh start and after a death that costs */
typedef struct { const char *name, *story, *gift; } ClassDef;
static const ClassDef CLASSES[NELEM] = {
    { "THE WITCHFINDER", "THEY LIT THE PYRES IN THE MARKET SQUARES OF THREE COUNTIES AND CALLED THE SMOKE A CLEANSING. THE ONES THEY "
      "BURNED HAVE NOT LEFT THEM; THEY WALK CLOSE, AND THEY ARE STILL BURNING. BELOW, THE FLOOR CATCHES WHERE THE WITCHFINDER'S "
      "FIRE PASSES, AND THE WITCHFINDER WALKS THROUGH IT UNHARMED, AS THEY ALWAYS DID.",
      "FIRE COSTS A FIFTH LESS AND HITS A TENTH HARDER. THREE POINTS FOR FIRE." },
    { "THE ANATOMIST", "THEY KEPT THE COLD ROOM UNDER THE COLLEGE, AND BOUGHT THE NEWLY DEAD BY THE POUND. NOTHING SPOILED IN THEIR "
      "KEEPING AND NOTHING LEFT IT WHOLE. THE COLD SLOWS THE DEAD AS IT SLOWED THE ROT, AND WHAT IS FROZEN COMES APART CLEANLY, "
      "AS IT DID ON THE SLAB.",
      "FROST OPEN, AND IT COSTS A FIFTH LESS. SLOWING AND FREEZING LAST A FIFTH LONGER." },
    { "THE GALVANIST", "THEY MADE HANGED MEN TWITCH FOR PAYING CROWDS, AND SWORE THAT ONE SAT UP AND SPOKE. THE CURRENT STILL RUNS IN "
      "THEIR HANDS. IT LEAPS FROM ONE DEAD THING TO THE NEXT AS IT ONCE LEAPT ALONG THE TABLE, AND LEAVES THE AIR BURNING WHERE "
      "IT WENT.",
      "LIGHTNING OPEN; IT COSTS A FIFTH LESS AND GATHERS A TENTH QUICKER." },
    { "THE PLAGUE DOCTOR", "UNDER THE BEAK AND BEHIND THE GLASS THEY WALKED THE PEST-HOUSES AND CARRIED OUT THE BODIES, AND LEARNT WHAT "
      "A FEVER PASSES ON, AND TO WHOM. THEIR FLASKS STILL HOLD WHAT THE WARDS HELD; WHERE THEY BREAK, THE FLOOR IS NOT SAFE TO "
      "WALK FOR A WHILE.",
      "POISON OPEN, A FIFTH CHEAPER. FLASKS HIT A QUARTER HARDER; POISON BITES HALF AGAIN AS HARD, A FIFTH LONGER." },
    { "THE SIN-EATER", "PAID IN BREAD AND A COIN TO EAT THE SINS OF THE DEAD FROM THEIR CHESTS, THEY ATE FOR A WHOLE PARISH THROUGH A "
      "PLAGUE YEAR. WHAT THEY SWALLOWED HAS NOT SETTLED. THEY KNOW HOW TO DRAW A SOUL OUT, AND HOW TO KEEP IT; THEIRS IS THE "
      "DEEPEST VESSEL.",
      "SPIRIT OPEN, AND IT COSTS A FIFTH LESS. THE VESSEL HOLDS TWENTY MORE." },
    { "THE HEADSMAN", "A HOOD, A BLOCK AND A STEADY ARM, FOR FORTY YEARS. THEY NEVER MISSED AND NEVER SPOKE. THEY CARRY THE BONES OF "
      "THE ONES THEY FINISHED, AND WEAR THEM; BONE PARTS FOR THEM AS IT ALWAYS DID.",
      "PHYSICAL OPEN; A FIFTH MORE SPEARS SPEND NO BONE; TEN MORE BONES; A TENTH MORE ARMOUR." },
};
static const char *const CLASS_SHORT[NELEM] = { "WITCHFINDER", "ANATOMIST", "GALVANIST", "PLAGUE DOCTOR", "SIN-EATER", "HEADSMAN" };
static int cls_el(void) { return RUN.cls >= 1 && RUN.cls <= NELEM ? RUN.cls - 1 : -1; }
/* a line is open by the class, by a lending (the dream on a page with no class yet), or by its opening paid; since
 * 1.0.0b16 no line is open to everyone, not even Fire (docs/PLAN-play.md, 3a) */
static int align_open(int e) { return e == cls_el() || LEDGER.lent == e + 1 || RUN.al[e][ND_OPEN] >= ALIGN[e].n[ND_OPEN].cost; }
/* Physical is shut until the Lampstand's first gift (the owner, 4 November): no Headsman to begin a life with, and no
 * spear bought with points before it; a Headsman already sworn keeps their past */
static int phys_locked(void) { return RUN.gifts < 1 && cls_el() != EL_PHYS && !align_open(EL_PHYS); }
static int class_offered(int k) { return k != EL_PHYS || !phys_locked(); }
static int class_random(void) { int k; do k = rnd(NELEM); while (!class_offered(k)); return k; }   /* 0-based */
/* The Lampstand's gifts (docs/PLAN-play.md, section 3): a line more, free, owed once level 4 is behind you and again once
 * level 12 is, counted by the level about to be entered, so a Portal run or an old page gets what it is owed. Any closed
 * line may be taken, Physical too. With every line open, a gift comes as its opening's points */
static int gifts_owed(int level) { return (level >= 5) + (level >= 13); }
static int gift_due(void) { return G.shop && gifts_owed(G.level) > RUN.gifts; }
static int align_rank(int e) {                                     /* 0: closed; 1 opened; a rank a point after */
    if (!align_open(e)) return 0;
    int r = 1;
    for (int n = ND_EFF; n < NNODE; n++) r += RUN.al[e][n];
    return r;
}
static int align_full(int e, int n) { return RUN.al[e][n] >= ALIGN[e].n[n].cost; }
static int align_last(int e) {                                     /* the last node bought, in the order they must be: -1 none, */
    if (!align_open(e)) return -1;                                 /* or the opening, or an ultimate, which she never takes */
    int last = -1;
    for (int n = ND_EFF; n < NNODE; n++) if (RUN.al[e][n]) last = n;
    return last < 0 || ALIGN[e].n[last].kind == NK_ULT ? -1 : last;
}
static int align_path(int e) { return RUN.al[e][ND_A1] ? 1 : (RUN.al[e][ND_B1] ? 2 : 0); }   /* 1 path A, 2 path B, 0 not yet */
static float align_sum(int e, int kind) {                          /* a line's Efficiency, Insight: its nodes' points */
    float v = 0.0f;
    for (int n = 0; n < NNODE; n++) if (ALIGN[e].n[n].kind == kind) v += ALIGN[e].n[n].val * (float)RUN.al[e][n];
    return v;
}
static float align_power(int e) {                                  /* a Mastery, once whole */
    float v = 0.0f;
    for (int n = 0; n < NNODE; n++) if (ALIGN[e].n[n].kind == NK_MASTER && align_full(e, n)) v += ALIGN[e].n[n].val;
    return v;
}
static float align_add(int e) {                                   /* a Mastery's Addition: its element added to every other spell */
    float v = 0.0f;
    for (int n = 0; n < NNODE; n++) if (ALIGN[e].n[n].kind == NK_MASTER && align_full(e, n)) v += ALIGN[e].n[n].add;
    return align_open(e) ? v : 0.0f;
}
static int spirit_mastery(void) { return align_open(EL_SPIRIT) && align_full(EL_SPIRIT, ND_B2); }   /* Spirit harms spectres */
static int bones_max(void) { return BONES_MAX + (RUN.al[EL_PHYS][ND_EFF] ? 5 : 0) + (cls_el() == EL_PHYS ? 10 : 0); }   /* Physical Efficiency
                                                                                     carries five more, the Headsman ten */
static float bone_spared(void) {                                   /* a spear that spends none: the line's, the Headsman's, what is worn */
    return fminf(0.9f, align_sum(EL_PHYS, NK_EFF) + (cls_el() == EL_PHYS ? 0.2f : 0.0f) + gear_total(&RUN.gear, ST_EFF + EL_PHYS) / 100.0f);
}
static int align_has(int e, int n) { return align_open(e) && align_full(e, n) && ALIGN[e].n[n].cost > 0; }   /* an ability, Mastery or ultimate bought */
static int d6_on(void) {                                           /* the merchant by class (1.0.0b21, D6): the wares by the lines held and the
                                                                      class, the new stats and the classes' abilities felt; CRAWL_D6=0 in the
                                                                      debug build puts back 1.0.0b20's, for the balance runs */
#ifdef CRAWL_DEBUG
    static int on = -1;
    if (on < 0) on = !(getenv("CRAWL_D6") && atoi(getenv("CRAWL_D6")) == 0);
    return on;
#else
    return 1;
#endif
}
static float cls_ab(int e, int which) {                            /* a class's ability worn (G5): felt by that class alone */
    return d6_on() && cls_el() == e ? gear_ability(&RUN.gear, AB_CLASS + e, which) : 0.0f;
}
static float st_odds(int el) {                                     /* G3: an element's status chance worn, as a share more */
    return d6_on() && el >= 0 && el < EL_PHYS ? gear_total(&RUN.gear, ST_CHANCE + el) / 100.0f : 0.0f;
}
static void align_sync(void) {                                     /* each line's spell into the spellbook, at its rank */
    for (int e = 0; e < NELEM; e++) {
        int r = align_rank(e);
        RUN.gear.atk[LINE_ATK[e]] = (int8_t)(r > 0 ? imin(r, 120) : -1);
        int mine = e == cls_el();                                  /* the class's gifts: a fifth off its line's cost, and */
        float eff = e == EL_PHYS ? 0.0f : fminf(0.9f, align_sum(e, NK_EFF) + (mine ? 0.2f : 0.0f));   /* Physical's Efficiency spares bones */
        float ins = align_sum(e, NK_INS) + (mine && e == EL_LIGHT ? 0.1f : 0.0f);   /* the Galvanist's quickness, */
        spell_set_line(e, eff, fminf(0.9f, ins), align_power(e) + (mine && e == EL_FIRE ? 0.1f : 0.0f));   /* the Witchfinder's heat */
        gear_set_lean(e, (float)r);                                /* the wares lean to the lines fed, and the dream weighs them so */
    }
    gear_set_class(cls_el(), d6_on());                             /* and to the class (1.0.0b21, D6) */
    RUN.gear.atk[AT_HELL] = RUN.gear.atk[AT_WISPS] = -1;           /* only the abilities let these loose now */
    spell_set_own(cls_el() >= 0 ? LINE_ATK[cls_el()] : LEDGER.lent ? LINE_ATK[LEDGER.lent - 1] : -1);   /* its first rank free */
    spell_set_atone(RUN.atone);
}

static void caster_sync(void) {                                    /* the caster's own into the totals */
    float own[NSTAT] = { 0 }, up[NSTAT] = { 0 };
    for (int l = 0; l < NLINE; l++) {
        float v = LINES[l].step * (float)RUN.steps[l], o = over_step(l) * (float)RUN.over[l];
        if (l == LN_RESIST) { own[ST_RES_POISON] += v; own[ST_RES_BURN] += v; own[ST_RES_SLOW] += v; own[ST_RES_SPIRIT] += v; own[ST_RES_LIGHT] += v; continue; }
        own[LINES[l].stat] += v + o; up[LINES[l].stat] += o;
    }
    own[ST_LUCK] += LUCK_BASE;
    own[ST_ARMOR] += gear_total_raw(&RUN.gear, ST_ARMOR) * (ALIGN[EL_PHYS].n[ND_A1].val * (float)RUN.al[EL_PHYS][ND_A1]   /* Bone Plate, */
                                                            + (cls_el() == EL_PHYS ? 0.1f : 0.0f));                       /* the Headsman's */
    for (int i = 0; i < 10; i++) own[ST_ARMOR] += G.oss_t[i] > 0.0f;   /* Ossified's points */
    if (G.fwalk_t > 0.0f) own[ST_ARMOR] += 0.5f * (gear_total_raw(&RUN.gear, ST_ARMOR) + own[ST_ARMOR]);   /* Flamewalker */
    conf_own(own, up);                                             /* the Unholy Confessions (C5, section 10) */
    class_own(own);                                                /* the class's own sheet (D5, section 9) */
    for (int st = 0; st < NSTAT; st++) up[st] += class_cap(st);   /* and its caps (D's, docs/BRIEF-caps.md) */
    for (int st = 0; st < NSTAT; st++) gear_set_own(st, own[st], up[st]);
    align_sync();
}
static int line_capped(int l) {                                    /* more would be wasted */
    if (l == LN_HEALTH) return 0;
    if (l == LN_RESIST) return gear_capped(&RUN.gear, ST_RES_POISON) && gear_capped(&RUN.gear, ST_RES_BURN) && gear_capped(&RUN.gear, ST_RES_SLOW)
                               && gear_capped(&RUN.gear, ST_RES_SPIRIT) && gear_capped(&RUN.gear, ST_RES_LIGHT);
    return gear_capped(&RUN.gear, LINES[l].stat);
}
static int ascend(void) {                                          /* every level the experience reaches; how many */
    int to = atone_reach(), n = to - RUN.atone;
    if (n <= 0) return 0;
    for (int l = RUN.atone + 1; l <= to && l <= ATONE_TOP; l++) { RUN.points += 10; RUN.al_points++; }   /* and an Alignment Point (1.0.0b11) */
    RUN.atone = to;
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT ascend to %d (+%d) points %d xp %d by %s t=%.2f\n", to, n, RUN.points, RUN.xp, G.player ? "player" : "autopilot", G.t);
    int cr[NELEM];                                                 /* each line's augment credits so far, for their pacing (C6, A2 b) */
    for (int e = 0; e < NELEM; e++) { cr[e] = RUN.aug_bank[e]; for (int t = 0; t < NAUGT; t++) cr[e] += RUN.aug_cr[e][t]; }
    fprintf(stderr, "AUGCR atone %d level %d credits %d %d %d %d %d %d t=%.2f\n", to, RUN.level, cr[0], cr[1], cr[2], cr[3], cr[4], cr[5], G.t);
#endif
    return n;
}
static void line_buy(int l) {
    RUN.points -= LINES[l].cost; RUN.steps[l]++;
    caster_sync();
    if (l == LN_HEALTH) { G.max_hp = gear_max_hp(&RUN.gear); G.hp = imin(G.hp + (int)LINES[l].step, G.max_hp); }   /* felt at once */
}
/* the cornerstones (docs/PLAN-b9.md, section 6): one chosen, free, at every tenth Level of Atonement */
static int corner_count(void) { int n = 0; for (int c = 0; c < NCS; c++) n += RUN.corner >> c & 1; return n; }
static int corner_due(void) { return imin(RUN.atone / 10, NCS) - corner_count(); }
static int over_due(void) {                                        /* overlevels to choose: one at 55, 60, 65 ... */
    int n = RUN.atone >= ATONE_TOP + 5 ? (RUN.atone - ATONE_TOP) / 5 : 0;
    for (int l = 0; l < NLINE; l++) n -= RUN.over[l];
    for (int cf = 0; cf < NCONF; cf++) n -= RUN.conf_over[cf];
    return n;
}
static int wind_restorable(void) { return (RUN.corner >> CS_WIND & 1) && RUN.wind_spent; }
static int ascend_open(void) { return atone_reach() > RUN.atone || RUN.points > 0 || corner_due() > 0 || over_due() > 0; }

/* The dream's Alignment (docs/PLAN-b11.md, section 14): its own line to the end of a path, Fire's ability, then a
 * second line, then the rest of Fire's; a node at a time in that order, saving for the next. Until it has a class
 * (step 10), its own line and path come from the run's id */
static int align_price(int e, int n);
static void align_buy(int e, int n);
static int align_funds(int e);
static void gift_settle(void);
static int align_ready(int e, int n);
static int align_next(int e, int path, int stop_at_ability) {     /* the next node on the way down a path, -1 at its end */
    if (!align_open(e)) return ND_OPEN;
    for (int n = ND_EFF; n <= ND_ABIL; n++) if (!align_full(e, n)) return n;
    if (stop_at_ability) return -1;
    int p0 = align_path(e) ? (align_path(e) == 1 ? ND_A1 : ND_B1) : (path == 1 ? ND_A1 : ND_B1);
    for (int n = p0; n < p0 + 3; n++) if (!align_full(e, n)) return n;
    return -1;
}
/* A vow taken (section 11): the class's line opens free (a paid opening comes back), the Witchfinder's three points for
 * Fire; a class given up keeps what was bought in its line, no longer free */
static void class_take(int c) {
    int old = cls_el(), e = clampi(c, 1, NELEM) - 1;
    if (old >= 0 && old != e) {
        int kept = 0;
        for (int n = ND_EFF; n < NNODE; n++) kept |= RUN.al[old][n] != 0;
        if (kept && RUN.al[old][ND_OPEN] < ALIGN[old].n[ND_OPEN].cost) { RUN.al[old][ND_OPEN] = (uint8_t)ALIGN[old].n[ND_OPEN].cost; RUN.al_gift |= (uint8_t)(1u << old); }
    }
    if (RUN.gifted >> e & 1) { RUN.gifted &= (uint8_t)~(1u << e); RUN.gifts--; RUN.al[e][ND_OPEN] = 0; }   /* a gift's line sworn to:
                                                                                                        the gift is owed again */
    if (RUN.al[e][ND_OPEN]) {
        if (!(RUN.al_gift >> e & 1)) RUN.al_points += RUN.al[e][ND_OPEN];
        RUN.al[e][ND_OPEN] = 0; RUN.al_gift &= (uint8_t)~(1u << e);
    }
    LEDGER.lent = 0;                                               /* a lending ends with a past of its own */
    int was_open = align_open(e);
    RUN.cls = e + 1;
    RUN.cls_pts = e == EL_FIRE ? 3 : 0;
    if (e == EL_PHYS && !was_open) RUN.gear.bones = (int16_t)imin(bones_max(), RUN.gear.bones + SPEAR_BONES);   /* the spear comes with bones */
    caster_sync();
    if (G.max_hp > 0) {                                            /* the class's health felt at once, the same missing (D5) */
        int was = G.max_hp;
        G.max_hp = gear_max_hp(&RUN.gear);
        G.hp = clampi(G.hp + G.max_hp - was, 1, G.max_hp);
    }
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT class %s from %s by %s t=%.2f\n", CLASSES[e].name, old >= 0 ? CLASSES[old].name : "nothing", G.player ? "player" : "autopilot", G.t);
#endif
}
/* No line open and no past (a page older than the classes, played by the dream, which may not choose for it; or a run
 * met in the crypt before her question): a line is lent at random till a class is taken, so nothing goes down unarmed */
static void line_lend(void) {
    if (RUN.cls) return;
    for (int e = 0; e < NELEM; e++) if (align_open(e)) return;
    LEDGER.lent = 1 + class_random();
    caster_sync();
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT line_lent %s t=%.2f\n", ALIGN[LEDGER.lent - 1].name, G.t);
#endif
}
static void old_spells_said(void) {                               /* to a person: as she greets them, or as they take over there */
    if (!RUN.old_spells || !LEDGER.yours || !G.shop) return;
    RUN.old_spells = 0;
    toast(SP_ITEM_TOME, -1, RK_I, 0, "THE OLD SPELLS ARE FORGOTTEN", "ALIGN AT THE LAMPSTAND");
}
static void auto_align(void) {
    if (LEDGER.yours || LEDGER.kept) return;                       /* a person's points are theirs to spend */
    uint32_t h = RUN.run_id * 2654435761u;
    int own = cls_el() >= 0 ? cls_el() : (int)(h % NELEM), second = (own + 1 + (int)((h >> 8) % (NELEM - 1))) % NELEM;
    if (second == EL_FIRE) second = (second + 1) % NELEM;
    if (second == own) second = (own + 2) % NELEM;
    const int stage[4][3] = { { own, 1 + (int)(h >> 16 & 1), 0 }, { EL_FIRE, 1, 1 }, { second, 1 + (int)(h >> 17 & 1), 0 }, { EL_FIRE, 1 + (int)(h >> 18 & 1), 0 } };
    for (int st = 0, guard = 0; st < 4 && guard < 64; guard++) {
        int e = stage[st][0], n = align_next(e, stage[st][1], stage[st][2]);
        if (n < 0 || (n == ND_OPEN && !align_ready(e, n))) { st++; continue; }   /* done, or shut to it yet */
        if (align_funds(e) < align_price(e, n)) break;             /* saving for it */
        align_buy(e, n);
    }
}

static void auto_confess(void);
static void auto_augment(void);
static void auto_ascend(void) {                                    /* the autopilot's: a little of everything that keeps it alive */
    if (LEDGER.yours || LEDGER.kept) return;                       /* a person's run: what it buys is theirs to choose */
    ascend();
    if (conf_on()) { auto_confess(); goto stones; }
    static const int ORDER[] = { LN_HASTE, LN_DAMAGE, LN_HEALTH, LN_HEALTH, LN_DODGE, LN_RESIST, LN_ATTUNE, LN_HEALTH, LN_LUCK };
    for (int guard = 0; guard < 4000 && RUN.points > 0; guard++) {
        int any = 0;
        for (int i = 0; i < (int)(sizeof ORDER / sizeof ORDER[0]); i++) {
            int l = ORDER[i];
            if (RUN.points < LINES[l].cost || line_capped(l) || (l == LN_RESIST && RUN.steps[l] >= 9)) continue;
            line_buy(l); any = 1;
        }
        if (!any) break;
    }
stones:;
    static const int PICK[NCS] = { CS_WIND, CS_OMEN, CS_LANTERN, CS_SENSE, CS_CARTO };   /* the map it never buys */
    for (int i = 0; i < NCS && corner_due() > 0; i++) {
        int c = PICK[i];
        if (RUN.corner >> c & 1 || (c == CS_CARTO && !RUN.has_map)) continue;
        RUN.corner |= 1 << c;
        if (c == CS_WIND) { RUN.wind_spent = 0; RUN.wind_run = crawl_run(G.level); }
#ifdef CRAWL_DEBUG
        fprintf(stderr, "EVENT cornerstone %d by autopilot t=%.2f\n", c, G.t);
#endif
    }
    static const int OVER_PICK[] = { LN_HASTE, LN_DAMAGE, LN_DODGE, LN_ATTUNE, LN_MOVE, LN_POISON, LN_BURN, LN_SLOW, LN_TREASURE, LN_LUCK };
    while (!conf_on() && over_due() > 0) {                                       /* overlevels: the first line already at its cap */
        int pick = LN_DAMAGE;
        for (int i = 0; i < (int)(sizeof OVER_PICK / sizeof OVER_PICK[0]); i++) if (line_capped(OVER_PICK[i])) { pick = OVER_PICK[i]; break; }
        RUN.over[pick]++; RUN.al_points++; caster_sync();
#ifdef CRAWL_DEBUG
        fprintf(stderr, "EVENT overlevel %d by autopilot t=%.2f\n", pick, G.t);
#endif
    }
}

/* ---- confessing (1.0.0b20, section 10): a point at a time, felt at once; an overlevel past V; the dream's by its class ---- */
static void conf_felt(int was_max) {                               /* more health is had at once, as the old line's was */
    caster_sync();
    G.max_hp = gear_max_hp(&RUN.gear); G.hp = imin(G.hp + imax(0, G.max_hp - was_max), G.max_hp);
}
static void conf_buy(int cf, int by_hand) {
    int was = gear_max_hp(&RUN.gear), r0 = conf_rank(cf);
    RUN.points--; RUN.conf[cf]++;
    conf_felt(was);
#ifdef CRAWL_DEBUG
    if (conf_rank(cf) != r0) fprintf(stderr, "EVENT confess %s %d by %s points %d t=%.2f\n", CONF[cf].name, conf_rank(cf), by_hand ? "player" : "autopilot", RUN.points, G.t);
#else
    (void)r0; (void)by_hand;
#endif
}
static void conf_overlevel(int cf, int by_hand) {                  /* past V, for good; an Alignment Point with it, unless an old line's
                                                                      overlevel paid back already gave it */
    int was = gear_max_hp(&RUN.gear);
    RUN.conf_over[cf]++;
    if (RUN.ap_overs > 0) RUN.ap_overs--; else RUN.al_points++;
    conf_felt(was);
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT confess %s %d overlevel by %s t=%.2f\n", CONF[cf].name, conf_rank(cf), by_hand ? "player" : "autopilot", G.t);
#else
    (void)by_hand;
#endif
}
static void auto_confess(void) {                                   /* section 10.7: the class's two best first, then evened out */
    int best[2] = { CF_CONST, CF_AGILITY }, e = cls_el();
    if (e >= 0) {
        best[0] = best[1] = -1;
        for (int cf = 0; cf < NCONF; cf++) {
            if (best[0] < 0 || CONF_W[e][cf] > CONF_W[e][best[0]]) { best[1] = best[0]; best[0] = cf; }
            else if (best[1] < 0 || CONF_W[e][cf] > CONF_W[e][best[1]]) best[1] = cf;
        }
    }
    for (int guard = 0; guard < 1000 && RUN.points > 0; guard++) {
        int pick = -1;
        for (int i = 0; i < 2; i++)
            if (RUN.conf[best[i]] < CONF_TOP && (pick < 0 || RUN.conf[best[i]] < RUN.conf[pick])) pick = best[i];
        if (pick < 0)
            for (int cf = 0; cf < NCONF; cf++) if (RUN.conf[cf] < CONF_TOP && (pick < 0 || RUN.conf[cf] < RUN.conf[pick])) pick = cf;
        if (pick < 0) break;                                       /* all six whole */
        conf_buy(pick, 0);
    }
    while (over_due() > 0) {                                       /* overlevels: the best of those whole */
        int pick = -1;
        for (int i = 0; i < 2 && pick < 0; i++) if (RUN.conf[best[i]] >= CONF_TOP) pick = best[i];
        for (int cf = 0; cf < NCONF && pick < 0; cf++) if (RUN.conf[cf] >= CONF_TOP) pick = cf;
        if (pick < 0) break;                                       /* none whole: they wait */
        conf_overlevel(pick, 0);
    }
}
/* an old page's lines, paid back (U5): every step as the points it cost, every overlevel as one to choose again */
static void lines_payback(void) {
    if (!conf_on()) return;
    int pay = 0, overs = 0;
    for (int l = 0; l < NLINE; l++) { pay += RUN.steps[l] * LINES[l].cost; overs += RUN.over[l]; RUN.steps[l] = 0; RUN.over[l] = 0; }
    if (!pay && !overs) return;
    RUN.points += pay; RUN.ap_overs += overs; RUN.old_lines = 1;
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT lines_payback points %d overlevels %d t=%.2f\n", pay, overs, G.t);
#endif
}
static void old_lines_said(void) {                                 /* to a person, as she greets them */
    if (!RUN.old_lines || !LEDGER.yours || !G.shop) return;
    RUN.old_lines = 0;
    toast(SP_ITEM_TOME, -1, RK_I, 0, "THE OLD LINES ARE UNDONE", "THEIR POINTS ARE YOURS TO CONFESS");
}

/* ---- the dead's track (D) owns these three bodies (docs/PLAN-combat.md, 17.2 and 17.3 for 1.0.0b20): stubs till D5 ---- */
/* the classes' own (section 9, the draft the owner took for now, K1 b): what each brings at the start, flat (K3), joined to the
 * caster's own under the same caps; its health against the hundred everyone had, its stats in their own units, and what no stat
 * holds. The class's line and its gifts are as they were (K2). Words for the class question and the Codex in CLASS_SHEET */
/* With the owner's caps by class (docs/BRIEF-caps.md, section 6): each class's own element hurts it 15% less and one other 15%
 * more (the Galvanist's Frost 10%, the owner's; the Headsman's strength is its armour, its weakness Lightning), and three caps
 * lifted by three tenths (a resist's and armour's share stopping at 90%, Spirit Cost's at 70%), in each stat's units; armour's
 * share of a blow is capped in armour_keep */
typedef struct { int hp; struct { int8_t st; float v; } s[6]; float af_cut, souls; int8_t strong, weak; struct { int8_t st; float v; } cap[3]; } ClassOwn;
static const ClassOwn CLASS_OWN[NELEM] = {
    [EL_FIRE]   = { 95,  { { ST_HASTE, 6.0f }, { ST_THRIFT, 6.0f }, { ST_EFFECT, 8.0f }, { ST_RES_BURN, 15.0f }, { ST_RES_SLOW, -15.0f }, { -1, 0 } },
                    0.0f, 0.0f, EL_FIRE, EL_FROST, { { ST_RES_BURN, 15.0f }, { ST_HASTE, 15.0f }, { ST_EFFECT, 15.0f } } },
    [EL_FROST]  = { 105, { { ST_ARMOR, 5.0f }, { ST_RES_SLOW, 15.0f }, { ST_PHYSIC, 15.0f }, { ST_RES_BURN, -15.0f }, { -1, 0 }, { -1, 0 } },
                    0.0f, 0.0f, EL_FROST, EL_FIRE, { { ST_RES_SLOW, 15.0f }, { ST_PHYSIC, 120.0f }, { -1, 0 } } },
    [EL_LIGHT]  = { 90,  { { ST_DODGE, 6.0f }, { ST_MOVE, 6.0f }, { ST_QUIET, 15.0f }, { ST_RES_LIGHT, 15.0f }, { ST_RES_SLOW, -10.0f }, { -1, 0 } },
                    0.0f, 0.0f, EL_LIGHT, EL_FROST, { { ST_RES_LIGHT, 15.0f }, { ST_DODGE, 10.5f }, { ST_MOVE, 12.0f } } },
    [EL_POISON] = { 100, { { ST_RES_POISON, 15.0f }, { ST_LUCK, 1.0f }, { ST_TREASURE, 5.0f }, { ST_SPRINT, 0.5f }, { ST_RES_SPIRIT, -15.0f }, { -1, 0 } },
                    0.10f, 0.0f, EL_POISON, EL_SPIRIT, { { ST_RES_POISON, 15.0f }, { ST_LUCK, 7.5f }, { ST_TREASURE, 30.0f } } },
    [EL_SPIRIT] = { 95,  { { ST_REGEN, 0.1f }, { ST_RES_SPIRIT, 15.0f }, { ST_RES_POISON, -15.0f }, { -1, 0 }, { -1, 0 }, { -1, 0 } },
                    0.0f, 0.10f, EL_SPIRIT, EL_POISON, { { ST_RES_SPIRIT, 15.0f }, { ST_VESSEL, 30.0f }, { ST_THRIFT, 10.0f } } },
    [EL_PHYS]   = { 120, { { ST_ARMOR, 4.0f }, { ST_SPRINT, 1.0f }, { ST_RES_LIGHT, -15.0f }, { -1, 0 }, { -1, 0 }, { -1, 0 } },
                    0.0f, 0.0f, EL_PHYS, EL_LIGHT, { { ST_SPRINT, 2.25f }, { ST_DAMAGE, 30.0f }, { -1, 0 } } },
};
#define ARMOUR_CAP 0.75f                                           /* the most of a blow armour keeps off; the Headsman's lifted to 90% */
#define ARMOUR_CAP_UP 0.15f
static int class_sheet_on(void) {                                  /* CRAWL_SHEET=0 (debug) leaves them out, for the balance runs */
#ifdef CRAWL_DEBUG
    static int on = -1;
    if (on < 0) on = !(getenv("CRAWL_SHEET") && atoi(getenv("CRAWL_SHEET")) == 0);
    return on;
#else
    return 1;
#endif
}
static void class_own(float *own) {                                /* the class's own sheet, added to own[ST_*] (section 9) */
    int e = cls_el();
    if (e < 0 || !class_sheet_on()) return;
    const ClassOwn *c = &CLASS_OWN[e];
    own[ST_HP] += (float)(c->hp - 100);
    for (int i = 0; i < 6; i++) if (c->s[i].st >= 0) own[c->s[i].st] += c->s[i].v;
}
static int class_sheet_row(int e, int i, char *name, int nn, char *val, int nv, int *stat) {   /* a row of class e's sheet, in words;
                                                                      0 past them; *stat the stat it is (-1 none) */
    const ClassOwn *c = &CLASS_OWN[e];
    if (stat) *stat = -1;
    if (i == 0) { snprintf(name, (size_t)nn, "HEALTH"); snprintf(val, (size_t)nv, "%d", c->hp); return 1; }
    int k = 0;
    for (int j = 0; j < 6; j++) {
        if (c->s[j].st < 0 || ++k != i) continue;
        int st = c->s[j].st;
        float v = c->s[j].v;
        if (stat) *stat = st;
        snprintf(name, (size_t)nn, "%s", STAT_NAME[st]);
        if (st == ST_ARMOR) snprintf(val, (size_t)nv, "%+.0f", v);
        else if (st == ST_REGEN) snprintf(val, (size_t)nv, "%+.1f/S", v);
        else if (st == ST_THRIFT) snprintf(val, (size_t)nv, "-%.0f%%", v);
        else if (st == ST_SPRINT) snprintf(val, (size_t)nv, "%+.1f S", v);
        else snprintf(val, (size_t)nv, "%+.0f%%", v);
        return 1;
    }
    if (c->af_cut > 0.0f && ++k == i) { snprintf(name, (size_t)nn, "EVERY MARK'S CHANCE"); snprintf(val, (size_t)nv, "-%.0f%%", c->af_cut * 100.0f); return 1; }
    if (c->souls > 0.0f && ++k == i) { snprintf(name, (size_t)nn, "SOULS FROM KILLS"); snprintf(val, (size_t)nv, "+%.0f%%", c->souls * 100.0f); return 1; }
    return 0;
}
static void cap_text(int st, float cap, char *val, int nv) {        /* a cap as her question shows it */
    char num[8];
    snprintf(num, sizeof num, fabsf(cap - roundf(cap)) < 0.01f ? "%.0f" : "%.1f", clampf(cap, 0.0f, 9999.0f));
    if (st == ST_SPRINT) snprintf(val, (size_t)nv, "MAX %s S", num);
    else if (st == ST_VESSEL) snprintf(val, (size_t)nv, "MAX %s", num);
    else snprintf(val, (size_t)nv, "MAX %s%%", num);
}
static int class_cap_row(int e, int i, char *name, int nn, char *val, int nv) {   /* the caps class e lifts, a row each; 0 past them */
    const ClassOwn *c = &CLASS_OWN[e];
    int k = 0;
    for (int j = 0; j < 3; j++) {
        int st = c->cap[j].st;
        if (st < 0 || k++ != i) continue;
        snprintf(name, (size_t)nn, "%s", STAT_NAME[st]);
        cap_text(st, gear_stat_cap(st) - gear_cap_up(st) + c->cap[j].v, val, nv);
        return 1;
    }
    if (e == EL_PHYS && k == i) { snprintf(name, (size_t)nn, "ARMOUR KEEPS OFF"); cap_text(-1, 100.0f * (ARMOUR_CAP + ARMOUR_CAP_UP), val, nv); return 1; }
    return 0;
}
static float class_extra(int what) {                               /* KX_*: what of the sheet no stat holds */
    int e = cls_el();
    if (e < 0 || !class_sheet_on()) return 0.0f;
    return what == KX_AF_CUT ? CLASS_OWN[e].af_cut : what == KX_SOULS ? CLASS_OWN[e].souls : 0.0f;
}
static float gear_cut(int k) {                                     /* D6's (G3, G5): what is worn, off the hard statuses, as Agility's */
    if (!d6_on() || !(k == AF_DOWN || k == AF_STUN || k == AF_FROZEN)) return 0.0f;
    return gear_total(&RUN.gear, ST_STEADY) / 100.0f + (k == AF_DOWN ? cls_ab(EL_PHYS, 0) / 100.0f : 0.0f);   /* Steadiness; the Block */
}
static void abil_short(int ab, float a, char *buf, int n) {        /* a class's ability on Tab's caster page */
    static const char *const WD[NELEM] = { "PYREBRAND", "SLAB", "LIVE WIRE", "FEVER", "SWALLOWED SIN", "BLOCK" };
    int e = clampi(ab - AB_CLASS, 0, NELEM - 1);
    if (cls_el() != e) snprintf(buf, (size_t)n, "%s: NOT YOURS", WD[e]);
    else snprintf(buf, (size_t)n, "%s +%.0f%%", WD[e], a);
}
/* a killing blow lived (10.4): a blow that would end the caster, not a seep (hurt_ex has asked), not fire, bile or a void stood in,
 * nor a burning floor, a pool or poison; Second Wind has gone first. Constitution's chance, at most once a level (a pocket is a
 * place of its own): the blow lands, health stops at 1, and nothing touches the caster for three seconds, as a Second Wind.
 * What is on it stays on it (the owner's words: the blow connects, 1 health, three seconds). */
static int lastblow_can(void) {
    switch (G.blame.what) {
    case BL_POISON: case BL_FIRE: case BL_POOL: return 0;
    case BL_HAZARD: return G.blame.type != HZ_FIRE && G.blame.type != HZ_BILE && G.blame.type != HZ_VOID;   /* as windup_blow has it */
    default: return 1;
    }
}
static void dream_restore(float dt);                               /* Restoration, the dream's (10.3), by the spells' keys below */
static int blow_survived(int kind) {
    (void)kind;
    if (G.lb_spent || G.shop || G.tally || !lastblow_can()) return 0;
    float ch = conf_value(CF_CONST, CV_LASTBLOW);
#ifdef CRAWL_DEBUG
    if (getenv("CRAWL_LASTBLOW")) ch = (float)atof(getenv("CRAWL_LASTBLOW"));   /* testing: the chance as given */
#endif
    if (ch <= 0.0f || rndf() >= ch) return 0;                     /* nothing confessed, nothing rolled */
    G.lb_spent = 1; G.lb_at = G.t;
    G.hp = 1; G.wind_t = 3.0f;
    snd_sfx(SFX_HURT); snd_sfx_ex(SFX_HEART, 1.0f, 0.5f); snd_sfx_ex(SFX_DEEPCHIME, 0.7f, 0.5f);
    rumble(0.9f, 0.6f, 0.5f); pop_readout(); show_bar();
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT lastblow level %d chance %.2f by %d/%d/%d t=%.2f %s\n", G.level, ch, G.blame.what, G.blame.type, G.blame.rank, G.t,
            G.player ? "player" : "autopilot");
#endif
    return 1;
}
static int lastblow_shown(void) { return G.lb_at > 0.0 && G.t - G.lb_at < 3.05 && G.wind_t > 0.0f; }   /* the breath is the blow's */
static float class_cap(int st) {                                   /* the class's lift of stat st's cap (docs/BRIEF-caps.md), in its units */
    int e = cls_el();
    if (e < 0 || !class_sheet_on()) return 0.0f;
    for (int i = 0; i < 3; i++) if (CLASS_OWN[e].cap[i].st == st) return CLASS_OWN[e].cap[i].v;
    return 0.0f;
}
static float armour_cap(void) {                                    /* CRAWL_SHEET=0 leaves armour uncapped, as it was */
    return !class_sheet_on() ? 1.0f : ARMOUR_CAP + (cls_el() == EL_PHYS ? ARMOUR_CAP_UP : 0.0f);
}
/* ---- the dead's track's stubs end ---- */

