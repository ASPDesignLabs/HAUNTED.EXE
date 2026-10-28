/* The merchant's wares and what they do. See crawl_gear.h and docs/PLAN-items.md. */
#include "crawl_gear.h"
#include "crawl.h"
#include <stdio.h>
#include <string.h>

const char *const ATK_NAME[NATK] = { "FIRE BOLT", "FROST SHARD", "CHAIN LIGHTNING", "BONE SPEAR", "SOUL DRAIN",
                                     "HELLFIRE", "SEEKER WISPS", "RAPID SPARKS", "BFS9000" };
const char *const DEF_NAME[NDEF] = { "BONE WARD", "BLINK", "MIRROR VEIL", "MEND", "PURIFY", "FROST NOVA",
                                     "DREAD", "TIME SLIP", "SOUL REAPING" };
static const char *const ATK_BLURB[NATK] = { "THE FIREBALL, HOTTER", "SLOWS WHAT IT HITS", "ARCS TO NEARBY FOES", "PIERCES ALL IN A LINE",
                                             "HEALS YOU AS IT HURTS", "HEAVY BLAST, FLOOR BURNS", "HOMING, ROUND CORNERS",
                                             "QUICK AND LIGHT", "SPENDS SOULS. KILLS ALL." };
static const char *const DEF_BLURB[NDEF] = { "ABSORBS THE NEXT BLOWS", "STEP AWAY FROM HARM", "SENDS MISSILES BACK", "HEALS OVER TIME",
                                             "CURES POISON AND SLOW", "FREEZES ALL AROUND", "THE DEAD FLEE",
                                             "SLOWS ALL BUT YOU", "TRIPLE SOULS A WHILE" };

const char *const QUAL_NAME[NQUAL] = { "COMMON", "UNCOMMON", "RARE", "ULTRA-RARE", "EXQUISITE" };
const int QUAL_RAMP[NQUAL] = { R_BONE, R_MOSS, R_GLOW, R_PURPLE, R_GOLD };   /* white, green, blue, purple, gold */
const char *const KIND_NAME[NIK] = { "", "BRACER", "BOOTS", "AMULET", "HOOD", "ROBES", "RING", "BAG" };
const char *const STAT_NAME[NSTAT] = { "ARMOR", "DODGE", "HEALTH", "RESTORES", "RESIST POISON", "RESIST BURN", "RESIST SLOW",
                                       "MOVEMENT", "QUIET", "SPELL HASTE", "SPIRIT COST", "DAMAGE", "FIRE", "FROST", "LIGHTNING",
                                       "SPELL EFFECT", "TREASURE", "LUCK" };
static const char *const STAT_OF[NSTAT] = { "THE BULWARK", "SHADOWS", "VIGOUR", "REJUVENATION", "CLEAN BLOOD", "THE HEARTH",
                                            "THE UNBOUND", "THE LIGHTFOOT", "SILENCE", "QUICKENING", "THE FRUGAL SOUL",
                                            "SPELLTONGUE", "CINDERS", "RIME", "STORMS", "THE ADEPT", "PLENTY", "FORTUNE" };
const char *const ABIL_NAME[NABIL] = { "", "SWIFTNESS", "ARCANE KNOWLEDGE", "TREASURE HUNTER", "KEYMAKER", "COMMUNE", "THORNS", "MIRRORED" };
static const char *const ABIL_WORD[NABIL] = { "", "SWIFT", "ARCANE", "TREASURE HUNTER", "KEYMAKER", "COMMUNING", "THORNED", "MIRRORED" };

static const float QUAL_MULT[NQUAL] = { 1.0f, 1.2f, 1.45f, 1.75f, 2.1f };   /* how strong its stats roll */
static const float QUAL_PRICE[NQUAL] = { 1.0f, 1.5f, 2.2f, 3.2f, 4.5f };
static const float ANOINT[NQUAL] = { 0.10f, 0.13f, 0.16f, 0.20f, 0.25f };
static const int QUAL_STATS[NQUAL] = { 1, 2, 3, 4, 99 };                     /* never more than the slots; Exquisite one more */
static const int ABIL_CHANCE[NQUAL] = { 0, 10, 25, 50, 100 };                /* percent */
static const int KIND_SLOTS[NIK] = { 0, 2, 2, 2, 2, 4, 1, 0 };
static const float KIND_PRICE[NIK] = { 0.0f, 0.45f, 0.6f, 0.7f, 0.7f, 0.9f, 0.8f, 0.5f };
static const int STAT_SCALE[NSTAT] = { 10, 10, 10, 100, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10 };
static const float STAT_CAP[NSTAT] = { 0, 35, 0, 0, 75, 75, 75, 40, 80, 50, 60, 100, 50, 50, 50, 50, 100, 25 };   /* on totals; 0 = none */
static float OWN[NSTAT], CAP_UP[NSTAT];                           /* the caster's own, and the overlevels' raise of each cap */
#define ST_ELEM ST_FIRE                       /* in the tables below: an element, chosen when it is rolled */

/* what each kind can roll; wrists have damage only from Rare up */
static const int KIND_STATS[NIK][10] = {
    { -1 },
    { ST_HASTE, ST_THRIFT, ST_ARMOR, ST_DODGE, ST_DAMAGE, -1 },
    { ST_MOVE, ST_QUIET, ST_DODGE, -1 },
    { ST_HP, ST_REGEN, ST_RES_POISON, ST_RES_BURN, ST_RES_SLOW, -1 },
    { ST_ARMOR, ST_EFFECT, ST_HP, -1 },
    { ST_HP, ST_REGEN, ST_RES_POISON, ST_RES_BURN, ST_RES_SLOW, ST_ARMOR, ST_HASTE, ST_ELEM, ST_DAMAGE, -1 },
    { ST_DAMAGE, ST_ELEM, -1 },
    { -1 },
};

static uint32_t rnext(uint32_t *s) { *s ^= *s << 13; *s ^= *s >> 17; *s ^= *s << 5; return *s; }
static int rpick(uint32_t *s, int n) { return (int)(rnext(s) % (uint32_t)n); }
static float rfrac(uint32_t *s) { return (float)(rnext(s) & 0xffff) / 65536.0f; }
static int is_elem(int st) { return st >= ST_FIRE && st <= ST_SHOCK; }

void gear_reset(Gear *g) {
    memset(g, 0, sizeof *g);
    for (int i = 0; i < NATK; i++) g->atk[i] = -1;
    g->atk[AT_FIRE] = 0;
}

/* about what the caster finds on a level of this depth; prices are set against it */
/* roughly the gold a level gives up (measured from the autopilot's runs); deeper levels hold more */
float gear_haul(int level) { return level < 1 ? 450.0f : 400.0f + 60.0f * level + 45.0f * level * level; }

/* spells and runes, priced against the haul of the level before the one they suit: about two levels' gold */
int gear_price(int kind, int id, int tier, int rare) {
    float p;
    if (kind == GK_ATK && id == AT_BFS) p = gear_haul(tier + 2) * 5.0f;
    else p = gear_haul(tier - 1) * 1.8f;
    if (rare) p *= 1.25f;
    return ((int)(p + 2.5f) / 5) * 5;
}

/* ---- pieces ---- */

/* a stat's value for a piece of this kind and level, before quality and the roll, in its own units */
static float stat_base(int st, int kind, int level) {
    float l = (float)level;
    switch (st) {
    case ST_ARMOR:  return 0.8f * l * (kind == IK_ROBE ? 1.0f : (kind == IK_HOOD ? 0.6f : 0.3f));
    case ST_DODGE:  return (3.0f + 0.7f * l) * (kind == IK_BOOTS ? 1.0f : 0.5f);
    case ST_HP:     return 6.0f * l * (kind == IK_ROBE ? 1.0f : (kind == IK_NECK ? 0.85f : 0.7f));
    case ST_REGEN:  return 0.05f * l;
    case ST_RES_POISON: case ST_RES_BURN: case ST_RES_SLOW: return 4.0f + 1.6f * l;
    case ST_MOVE:   return 4.0f + 0.8f * l;
    case ST_QUIET:  return 12.0f + 2.0f * l;
    case ST_HASTE:  return (3.0f + 0.6f * l) * (kind == IK_ROBE ? 1.2f : 1.0f);
    case ST_THRIFT: return 5.0f + 1.0f * l;
    case ST_DAMAGE: return (5.0f + 2.0f * l) * (kind == IK_RING ? 1.0f : (kind == IK_ROBE ? 0.6f : 0.3f));
    case ST_FIRE: case ST_FROST: case ST_SHOCK: return (4.0f + 1.5f * l) * (kind == IK_RING ? 1.0f : 0.6f);
    default:        return 5.0f + 1.5f * l;     /* ST_EFFECT */
    }
}
static float stat_expect(const Piece *p, int st) {           /* what an average roll of it would be */
    return stat_base(st, p->kind, p->level) * QUAL_MULT[p->quality] * (p->anointed ? 1.0f + ANOINT[p->quality] : 1.0f);
}
static int has_stat(const Piece *p, int st) { for (int i = 0; i < p->nstat; i++) if (p->stat[i] == st) return 1; return 0; }
static void add_stat(Piece *p, int st, uint32_t *rng) {
    if (p->nstat >= PIECE_STATS) return;
    float v = stat_expect(p, st) * (0.85f + 0.3f * rfrac(rng));
    if (STAT_CAP[st] > 0.0f && v > STAT_CAP[st]) v = STAT_CAP[st];   /* no piece promises more than can count */
    int iv = (int)(v * STAT_SCALE[st] + 0.5f);
    p->stat[p->nstat] = (uint8_t)st; p->val[p->nstat] = (int16_t)(iv < 1 ? 1 : (iv > 32000 ? 32000 : iv)); p->nstat++;
}

int bag_capacity(int quality) { static const int C[NQUAL] = { 2, 3, 3, 4, 5 }; return C[quality]; }
const char *bag_name(int size) {
    switch (size) {
    case 2: return "BURLAP SACK";
    case 3: return "GRAVEDIGGER'S SATCHEL";
    case 4: return "PILGRIM'S SCRIP";
    default: return "BONE RELIQUARY";
    }
}

/* the odds of each quality climb set by set, and a step further each time round the sets */
int piece_quality_roll(int level, uint32_t *rng) {
    static const int ODDS[4][NQUAL] = { { 59, 30, 9, 1, 1 }, { 39, 35, 18, 6, 2 }, { 23, 35, 25, 12, 5 }, { 12, 30, 30, 18, 10 } };
    int set = crawl_set(level), loop = crawl_loop(level), share[NQUAL];
    for (int q = 0; q < NQUAL; q++) share[q] = ODDS[set][q];
    for (int l = 0; l < loop && l < 4; l++) { share[4] += share[3]; for (int q = 3; q > 0; q--) share[q] = share[q - 1]; share[0] = 0; }
    int r = rpick(rng, 100), q = 0;
    while (q < NQUAL - 1 && r >= share[q]) r -= share[q++];
    return q;
}

static int pick_ability(uint32_t *rng) {
    static const int W[NABIL] = { 0, 4, 4, 4, 2, 3, 4, 3 };
    int tot = 0;
    for (int i = 0; i < NABIL; i++) tot += W[i];
    int r = rpick(rng, tot), a = 0;
    while (r >= W[a]) r -= W[a++];
    return a;
}

Piece piece_roll(int kind, int level, int quality, uint32_t *rng) {
    Piece p;
    memset(&p, 0, sizeof p);
    p.kind = (uint8_t)kind; p.level = (uint8_t)(level < 1 ? 1 : (level > 250 ? 250 : level));
    p.quality = (uint8_t)(quality >= 0 ? quality : piece_quality_roll(level, rng));
    if (kind == IK_BAG) return p;                                  /* a bag holds; it does nothing else */
    p.anointed = rpick(rng, 10) == 0;
    int pool[10], n = 0;
    for (int i = 0; KIND_STATS[kind][i] >= 0; i++) {
        int st = KIND_STATS[kind][i];
        if (kind == IK_WRIST && st == ST_DAMAGE && p.quality < Q_RARE) continue;
        pool[n++] = st;
    }
    int want = QUAL_STATS[p.quality] < KIND_SLOTS[kind] ? QUAL_STATS[p.quality] : KIND_SLOTS[kind] + (p.quality == Q_EXQUISITE);
    if (want > n) want = n;
    for (int k = 0; k < want; k++) {                               /* different stats, picked at random */
        int j = k + rpick(rng, n - k), t = pool[k];
        pool[k] = pool[j]; pool[j] = t;
        int st = pool[k] == ST_ELEM ? ST_FIRE + rpick(rng, 3) : pool[k];
        add_stat(&p, st, rng);
    }
    if (kind == IK_HOOD || rpick(rng, 100) < ABIL_CHANCE[p.quality]) {   /* hoods always carry one */
        p.ability = (uint8_t)pick_ability(rng);
        if (p.ability == AB_SWIFT && !has_stat(&p, ST_MOVE)) add_stat(&p, ST_MOVE, rng);       /* and bring their stat */
        if (p.ability == AB_ARCANE && !has_stat(&p, ST_THRIFT)) add_stat(&p, ST_THRIFT, rng);
    }
    return p;
}

/* A Black Altar's blessing: the rolled stats rise by the quality's share, no further than their caps */
void piece_anoint(Piece *p) {
    if (p->anointed) return;
    p->anointed = 1;
    for (int i = 0; i < p->nstat; i++) {
        int st = p->stat[i], cap = STAT_CAP[st] > 0.0f ? (int)(STAT_CAP[st] * STAT_SCALE[st] + 0.5f) : 32000;
        int v = (int)(p->val[i] * (1.0f + ANOINT[p->quality]) + 0.5f);
        if (v > cap) v = cap > p->val[i] ? cap : p->val[i];
        p->val[i] = (int16_t)(v < 1 ? 1 : (v > 32000 ? 32000 : v));
    }
}

Piece piece_make(int kind, int level, int stat, uint32_t *rng) {
    Piece p;
    memset(&p, 0, sizeof p);
    p.kind = (uint8_t)kind; p.level = (uint8_t)(level < 1 ? 1 : (level > 250 ? 250 : level)); p.quality = Q_COMMON;
    add_stat(&p, stat, rng);
    if (kind == IK_HOOD) {
        p.ability = (uint8_t)pick_ability(rng);
        if (p.ability == AB_SWIFT && !has_stat(&p, ST_MOVE)) add_stat(&p, ST_MOVE, rng);
        if (p.ability == AB_ARCANE && !has_stat(&p, ST_THRIFT)) add_stat(&p, ST_THRIFT, rng);
    }
    return p;
}

int piece_price(const Piece *p) {
    if (p->kind == IK_NONE) return 0;
    float v = gear_haul(p->level - 1) * KIND_PRICE[p->kind] * QUAL_PRICE[p->quality];
    return ((int)(v + 2.5f) / 5) * 5;
}

/* its best stat: the one rolled furthest above the average */
static int best_stat(const Piece *p) {
    int best = -1; float br = -1.0f;
    for (int i = 0; i < p->nstat; i++) {
        float r = (float)p->val[i] / STAT_SCALE[p->stat[i]] / fmaxf(1e-3f, stat_expect(p, p->stat[i]));
        if (r > br) { br = r; best = p->stat[i]; }
    }
    return best;
}

void piece_name(const Piece *p, char *line1, char *line2, int n) {
    if (p->kind == IK_NONE) { snprintf(line1, (size_t)n, "%s", ""); snprintf(line2, (size_t)n, "NOTHING"); return; }
    if (p->kind == IK_BAG) {
        snprintf(line1, (size_t)n, "%s", QUAL_NAME[p->quality]);
        snprintf(line2, (size_t)n, "%s", bag_name(bag_capacity(p->quality)));
        return;
    }
    snprintf(line1, (size_t)n, "%s%s%s%s", p->anointed ? "ANOINTED " : "", QUAL_NAME[p->quality], p->ability ? " " : "", ABIL_WORD[p->ability]);
    int b = best_stat(p);
    if (b >= 0) snprintf(line2, (size_t)n, "%s OF %s", KIND_NAME[p->kind], STAT_OF[b]);
    else snprintf(line2, (size_t)n, "%s", KIND_NAME[p->kind]);
}
void piece_full_name(const Piece *p, char *buf, int n) {
    char a[48], b[48];
    piece_name(p, a, b, sizeof a);
    snprintf(buf, (size_t)n, "%s%s%s", a, *a ? " " : "", b);
}

void piece_stat_text(int st, int val, char *buf, int n) {
    float v = (float)val / STAT_SCALE[st];
    switch (st) {
    case ST_ARMOR: case ST_HP: snprintf(buf, (size_t)n, "%s +%.0f", STAT_NAME[st], v); break;
    case ST_REGEN: snprintf(buf, (size_t)n, "RESTORES %.2f/S", v); break;
    case ST_THRIFT: snprintf(buf, (size_t)n, "SPIRIT COST -%.0f%%", v); break;
    case ST_FIRE: case ST_FROST: case ST_SHOCK: snprintf(buf, (size_t)n, "ADDS %.0f%% %s", v, STAT_NAME[st]); break;
    default: snprintf(buf, (size_t)n, "%s +%.0f%%", STAT_NAME[st], v); break;
    }
}

void piece_ability_text(const Piece *p, char *buf, int n) {
    int q = p->quality, a = piece_grade(p->ability, q, p->level, 0), b = piece_grade(p->ability, q, p->level, 1);
    switch (p->ability) {
    case AB_SWIFT: snprintf(buf, (size_t)n, "SWIFTNESS: MOVEMENT +%d%% MORE", a); break;
    case AB_ARCANE: snprintf(buf, (size_t)n, "ARCANE KNOWLEDGE: SPIRIT SAVED +%d%% MORE", a); break;
    case AB_TREASURE: snprintf(buf, (size_t)n, "TREASURE HUNTER: TREASURE +%d%%", a); break;
    case AB_KEYMAKER: snprintf(buf, (size_t)n, "KEYMAKER: KEYS SHOWN %d CELLS OFF, L FOR THE WISP", a); break;
    case AB_COMMUNE: snprintf(buf, (size_t)n, "COMMUNE: FOES WITHIN %d CELLS SHOWN FOR %d.%d S, EVERY 15 S", a, b / 10, b % 10); break;
    case AB_THORNS: snprintf(buf, (size_t)n, "THORNS: %d BACK TO WHAT STRIKES YOU", a); break;
    case AB_MIRRORED: snprintf(buf, (size_t)n, "MIRRORED: %d%% OF MISSILES SENT BACK AT %d%%", a, b); break;
    default: snprintf(buf, (size_t)n, "%s", ""); break;
    }
}

/* An ability's numbers by quality (and level, for Thorns): `which` picks the second number where
 * there are two (Commune's time in tenths of a second, Mirrored's share of damage sent back). */
int piece_grade(int ab, int q, int level, int which) {
    static const int PCT_SWIFT[NQUAL] = { 5, 10, 15, 20, 25 }, PCT_HALF[NQUAL] = { 10, 20, 30, 40, 50 };
    static const int KEY_RANGE[NQUAL] = { 6, 8, 10, 12, 16 };
    static const int COMMUNE_R[NQUAL] = { 4, 6, 8, 10, 12 }, COMMUNE_T[NQUAL] = { 10, 15, 20, 25, 30 };
    static const int MIRROR_P[NQUAL] = { 8, 12, 17, 23, 30 }, MIRROR_D[NQUAL] = { 50, 60, 75, 90, 100 };
    switch (ab) {
    case AB_SWIFT: return PCT_SWIFT[q];
    case AB_ARCANE: case AB_TREASURE: return PCT_HALF[q];
    case AB_KEYMAKER: return KEY_RANGE[q];
    case AB_COMMUNE: return which ? COMMUNE_T[q] : COMMUNE_R[q];
    case AB_THORNS: return (int)((2.0f + 0.6f * level) * QUAL_MULT[q] + 0.5f);
    case AB_MIRRORED: return which ? MIRROR_D[q] : MIRROR_P[q];
    default: return 0;
    }
}

/* ---- the caster's numbers ---- */

float gear_total_raw(const Gear *g, int st) {
    float t = 0.0f;
    for (int s = 0; s < NSLOT; s++) {
        const Piece *p = &g->worn[s];
        for (int i = 0; i < p->nstat; i++) if (p->stat[i] == st) t += (float)p->val[i] / STAT_SCALE[st];
    }
    return t;
}

float gear_stat_cap(int st) { return st >= 0 && st < NSTAT && STAT_CAP[st] > 0.0f ? STAT_CAP[st] + CAP_UP[st] : 0.0f; }
void gear_set_own(int st, float add, float cap_up) { if (st >= 0 && st < NSTAT) { OWN[st] = add; CAP_UP[st] = cap_up; } }
float gear_own(int st) { return st >= 0 && st < NSTAT ? OWN[st] : 0.0f; }
float gear_cap_up(int st) { return st >= 0 && st < NSTAT ? CAP_UP[st] : 0.0f; }
int stat_on_kind(int st, int kind) {
    if (kind <= IK_NONE || kind >= NIK) return 0;
    for (const int *k = KIND_STATS[kind]; *k >= 0; k++) if (*k == st || (*k == ST_ELEM && is_elem(st))) return 1;
    return 0;
}

/* abilities add up across what is worn, to a cap; Mirrored's share sent back is the best of them */
static const float CAP0[NABIL] = { 0, 25, 50, 50, 16, 12, 0, 50 }, CAP1[NABIL] = { 0, 0, 0, 0, 0, 30, 0, 0 };
float gear_ability_cap(int ab) { return ab > 0 && ab < NABIL ? CAP0[ab] : 0.0f; }
float gear_ability(const Gear *g, int ab, int which) {
    float t = 0.0f;
    for (int s = 0; s < NSLOT; s++) {
        const Piece *p = &g->worn[s];
        if (p->kind == IK_NONE || p->ability != ab) continue;
        float v = (float)piece_grade(ab, p->quality, p->level, which);
        if (ab == AB_MIRRORED && which) t = fmaxf(t, v); else t += v;
    }
    float cap = which ? CAP1[ab] : CAP0[ab];
    return cap > 0.0f && !(ab == AB_MIRRORED && which) ? fminf(t, cap) : t;
}

/* Abilities raise the summed stat once, rather than each piece's own (so nothing compounds), and
 * then the totals are capped. The elements share one cap between them. */
float gear_total(const Gear *g, int st) {
    float t = gear_total_raw(g, st);
    if (st == ST_MOVE) t *= 1.0f + gear_ability(g, AB_SWIFT, 0) / 100.0f;
    if (st == ST_THRIFT) t *= 1.0f + gear_ability(g, AB_ARCANE, 0) / 100.0f;
    if (st == ST_TREASURE) t += gear_ability(g, AB_TREASURE, 0);  /* Treasure Hunter's share */
    if (is_elem(st)) {
        float all = gear_total_raw(g, ST_FIRE) + gear_total_raw(g, ST_FROST) + gear_total_raw(g, ST_SHOCK);
        if (all > STAT_CAP[st]) t *= STAT_CAP[st] / all;
        return t;
    }
    t += OWN[st];                                                  /* the caster's own, after the abilities, under the one cap */
    return STAT_CAP[st] > 0.0f ? fminf(t, STAT_CAP[st] + CAP_UP[st]) : t;
}

int gear_capped(const Gear *g, int st) {
    if (STAT_CAP[st] <= 0.0f) return 0;
    if (is_elem(st)) return gear_total_raw(g, st) > 0.0f && gear_total_raw(g, ST_FIRE) + gear_total_raw(g, ST_FROST) + gear_total_raw(g, ST_SHOCK) >= STAT_CAP[st] - 0.05f;
    return gear_total(g, st) >= STAT_CAP[st] + CAP_UP[st] - 0.05f;
}

int   gear_armor(const Gear *g) { return (int)gear_total(g, ST_ARMOR); }
float gear_magic_resist(const Gear *g) { return gear_total(g, ST_RES_BURN) / 100.0f; }
float gear_dodge(const Gear *g) { return gear_total(g, ST_DODGE) / 100.0f; }
int   gear_thorns(const Gear *g) { return (int)gear_ability(g, AB_THORNS, 0); }
float gear_regen(const Gear *g) { return gear_total(g, ST_REGEN); }
float gear_poison_resist(const Gear *g) { return gear_total(g, ST_RES_POISON) / 100.0f; }
float gear_slow_resist(const Gear *g) { return gear_total(g, ST_RES_SLOW) / 100.0f; }
int   gear_max_hp(const Gear *g) { return 100 + (int)gear_total(g, ST_HP); }
float gear_haste(const Gear *g) { return 1.0f + gear_total(g, ST_MOVE) / 100.0f; }
float gear_ember(const Gear *g) { return 1.0f + gear_total(g, ST_DAMAGE) / 100.0f; }

/* ---- worth, for the autopilot ---- */

/* each stat against what a plain piece of the level would give, weighed by how much it helps */
float piece_worth(const Piece *p, int level) {
    static const float W[NSTAT] = { 1.0f, 0.8f, 1.0f, 0.6f, 0.35f, 0.35f, 0.25f, 0.35f, 0.15f, 0.8f, 0.3f, 1.0f, 0.6f, 0.6f, 0.6f, 0.3f, 0.0f, 0.0f };
    static const int REF[NSTAT] = { IK_ROBE, IK_BOOTS, IK_ROBE, IK_ROBE, IK_ROBE, IK_ROBE, IK_ROBE, IK_BOOTS, IK_BOOTS,
                                    IK_WRIST, IK_WRIST, IK_RING, IK_RING, IK_RING, IK_RING, IK_HOOD };
    static const float AW[NABIL] = { 0.0f, 0.15f, 0.3f, 0.4f, 0.0f, 0.0f, 0.4f, 0.5f };
    /* Keymaker and Commune are for a person's eyes: the autopilot doesn't count them */
    static const int FELT[NSTAT] = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0 };
    static const int FELT_AB[NABIL] = { 0, 1, 1, 1, 0, 0, 1, 1 };
    if (p->kind == IK_NONE || p->kind == IK_BAG) return 0.0f;
    if (level < 1) level = 1;
    float w = 0.0f;
    for (int i = 0; i < p->nstat; i++) {
        int st = p->stat[i];
        if (FELT[st]) w += W[st] * ((float)p->val[i] / STAT_SCALE[st]) / fmaxf(1e-3f, stat_base(st, REF[st], level));
    }
    return w + (FELT_AB[p->ability] ? AW[p->ability] * (1.0f + 0.25f * p->quality) : 0.0f);
}

int piece_slot(const Gear *g, int kind) {
    switch (kind) {
    case IK_WRIST: {
        if (g->worn[SL_WRIST_L].kind == IK_NONE) return SL_WRIST_L;
        if (g->worn[SL_WRIST_R].kind == IK_NONE) return SL_WRIST_R;
        int lvl = g->worn[SL_WRIST_L].level > g->worn[SL_WRIST_R].level ? g->worn[SL_WRIST_L].level : g->worn[SL_WRIST_R].level;
        return piece_worth(&g->worn[SL_WRIST_L], lvl) <= piece_worth(&g->worn[SL_WRIST_R], lvl) ? SL_WRIST_L : SL_WRIST_R;
    }
    case IK_BOOTS: return SL_BOOTS;
    case IK_NECK: return SL_NECK;
    case IK_HOOD: return SL_HOOD;
    case IK_ROBE: return SL_ROBE;
    case IK_RING: return SL_RING;
    default: return -1;
    }
}

int gear_bag_free(const Gear *g) {
    int n = 0;
    for (int i = 0; i < g->bag_size && i < BAG_MAX; i++) n += g->bag[i].kind == IK_NONE;
    return n;
}
static int bag_put(Gear *g, const Piece *p) {
    for (int i = 0; i < g->bag_size && i < BAG_MAX; i++) if (g->bag[i].kind == IK_NONE) { g->bag[i] = *p; return 1; }
    return 0;
}

static int slot_fits(int slot, int kind) {
    static const int SLOT_KIND[NSLOT] = { IK_WRIST, IK_WRIST, IK_BOOTS, IK_NECK, IK_HOOD, IK_ROBE, IK_RING };
    return SLOT_KIND[slot] == kind;
}
int gear_wear(Gear *g, int slot, int bi) {
    if (slot < 0 || slot >= NSLOT || bi < 0 || bi >= g->bag_size || !slot_fits(slot, g->bag[bi].kind)) return 0;
    Piece t = g->worn[slot];
    g->worn[slot] = g->bag[bi];
    g->bag[bi] = t;
    return 1;
}

static float atk_worth(int id, int tier) {
    static const float B[NATK] = { 1.0f, 0.9f, 1.1f, 1.0f, 1.0f, 1.05f, 0.95f, 0.9f, 3.0f };
    return tier < 0 ? 0.0f : B[id] * (1.0f + 0.3f * tier);
}
static float def_worth(const Gear *g, int id, int q, int level) {
    static const float B[NDEF] = { 1.0f, 0.8f, 0.9f, 1.1f, 0.6f, 0.85f, 0.8f, 0.85f, 0.7f };
    if (q <= 0) return 0.0f;
    int tier = rune_tier(q);
    float b = B[id];
    if (id == DF_PURIFY && level >= 4) b += 0.4f;                  /* spiders */
    if (id == DF_REAP && g->atk[AT_BFS] >= 0) b += 0.6f;
    if (id == DF_MIRROR && level >= 2) b += 0.2f;                  /* wall skulls */
    return b * (1.0f + 0.25f * tier) * (q >= RUNE_EXQ ? 1.3f : 1.0f);   /* and its second effect */
}

float gear_value(const Gear *g, const Offer *o, int level) {
    switch (o->kind) {
    case GK_ATK: {
        float v = atk_worth(o->id, o->tier) - atk_worth(o->id, g->atk[o->id]);
        if (g->atk[o->id] < 0) v += 0.3f;                           /* a new trick for the book */
        if (o->id == AT_SPEAR) v *= fminf(1.0f, fmaxf(0.33f, (float)(g->bones + SPEAR_BONES) / 18.0f));   /* only as good as the bones */
        return v > 0.0f ? v * 1.4f : 0.0f;                          /* spells weigh heavier than armor */
    }
    case GK_DEF: {
        float v = def_worth(g, o->id, o->tier, level) - def_worth(g, o->id, g->def[o->id], level);
        if (g->def[o->id] <= 0) v += 0.3f;
        return v > 0.0f ? v * 1.4f : 0.0f;
    }
    default: {
        const Piece *p = &o->piece;
        if (p->kind == IK_BAG) return 0.0f;                         /* the autopilot never takes anything out of one */
        int s = piece_slot(g, p->kind);
        float v = piece_worth(p, level) - piece_worth(&g->worn[s], level);
        return v > 0.0f ? v * 0.7f : 0.0f;
    }
    }
}

/* Into the book, or onto the caster if it is better than what is worn there (the autopilot's rule;
 * a person chooses at the merchant). What comes off goes into the bag if there is room, or is lost. */
int gear_take(Gear *g, const Offer *o) {
    switch (o->kind) {
    case GK_ATK:
        if (o->tier > g->atk[o->id]) {
            g->atk[o->id] = (int8_t)o->tier;
            if (o->id == AT_SPEAR) g->bones = (int16_t)(g->bones + SPEAR_BONES > BONES_MAX ? BONES_MAX : g->bones + SPEAR_BONES);
        }
        return TAKE_LEARNT;
    case GK_DEF: if (o->tier > g->def[o->id]) g->def[o->id] = (int8_t)o->tier; return TAKE_LEARNT;
    default: break;
    }
    const Piece *p = &o->piece;
    if (p->kind == IK_BAG) {
        int cap = bag_capacity(p->quality);
        if (cap > g->bag_size) {                                    /* the bigger bag takes everything over */
            Piece keep[BAG_MAX];
            int n = 0;
            for (int i = 0; i < g->bag_size && i < BAG_MAX; i++) if (g->bag[i].kind != IK_NONE) keep[n++] = g->bag[i];
            memset(g->bag, 0, sizeof g->bag);
            for (int i = 0; i < n; i++) g->bag[i] = keep[i];
            g->bag_size = (uint8_t)cap; g->bag_quality = p->quality;
        }
        return TAKE_BAG;
    }
    int s = piece_slot(g, p->kind);
    int lvl = p->level;
    if (g->worn[s].kind == IK_NONE) { g->worn[s] = *p; return TAKE_WORN; }
    if (piece_worth(p, lvl) >= piece_worth(&g->worn[s], lvl)) {
        Piece old = g->worn[s];
        g->worn[s] = *p;
        return bag_put(g, &old) ? TAKE_WORN_OLD_BAGGED : TAKE_WORN_OLD_LOST;
    }
    return bag_put(g, p) ? TAKE_BAGGED : TAKE_LOST;
}

/* a person buying: worn at once, whatever it is worth (the merchant has warned what becomes of the old) */
int gear_take_wear(Gear *g, const Offer *o) {
    if (o->kind != GK_PIECE || o->piece.kind == IK_BAG) return gear_take(g, o);
    int s = piece_slot(g, o->piece.kind);
    Piece old = g->worn[s];
    g->worn[s] = o->piece;
    if (old.kind == IK_NONE) return TAKE_WORN;
    return bag_put(g, &old) ? TAKE_WORN_OLD_BAGGED : TAKE_WORN_OLD_LOST;
}

int gear_shop_purchases(int level) { return level <= RUN_LEN ? 1 : (level <= 2 * RUN_LEN ? 2 : 3); }
int gear_shop_pedestals(int level) { return level <= 9 ? 5 : (level <= 14 ? 6 : 7); }

/* ---- offers ---- */

Offer gear_offer(int pool, int level, const Gear *g, uint32_t *rng) {
    Offer o;
    memset(&o, 0, sizeof o);
    if (pool == POOL_GEAR) {                                        /* something to wear, or now and then a bag */
        int w[NIK] = { 0, 3, 2, 2, 2, 3, 2, 0 };
        w[IK_BAG] = g && g->bag_size == 0 ? 3 : (g && g->bag_size >= BAG_MAX ? 0 : 1);
        int tot = 0;
        for (int i = 0; i < NIK; i++) tot += w[i];
        int r = rpick(rng, tot), k = 0;
        while (r >= w[k]) r -= w[k++];
        o.kind = GK_PIECE;
        o.piece = piece_roll(k, level, -1, rng);
        if (k == IK_BAG && g) {                                     /* only ever a bigger bag than yours */
            while (bag_capacity(o.piece.quality) <= g->bag_size && o.piece.quality < Q_EXQUISITE) o.piece.quality++;
            if (bag_capacity(o.piece.quality) <= g->bag_size) { k = IK_WRIST; o.piece = piece_roll(k, level, -1, rng); }
        }
        o.id = k; o.tier = o.piece.level; o.rare = o.piece.quality >= Q_ULTRA;
        o.price = piece_price(&o.piece);
        return o;
    }
    int r = rpick(rng, 12);
    if (r == 0) { o.rare = 1; o.tier = level + 2 + rpick(rng, 2); }    /* a jackpot: well ahead of its time */
    else o.tier = level + (r < 3 ? -1 : (r < 10 ? 0 : 1));
    if (o.tier < 1) o.tier = 1;
    o.kind = GK_ATK;
    if (level >= 3 && rpick(rng, 22) == 0) { o.id = AT_BFS; o.rare = 1; o.tier = 1 + level / 4; }   /* incredibly rare */
    else o.id = rpick(rng, AT_BFS);
    o.price = gear_price(o.kind, o.id, o.tier, o.rare);
    return o;
}

static const char *roman(int t) {
    static const char *const R[] = { "", "I", "II", "III", "IV", "V", "VI", "VII", "VIII", "IX", "X", "XI", "XII", "XIII", "XIV", "XV" };
    return t >= 0 && t < 16 ? R[t] : "XV+";
}

void gear_item_name(int kind, int id, int tier, char *buf, int n) {
    const char *base = kind == GK_DEF ? DEF_NAME[id] : ATK_NAME[id];
    if (kind == GK_DEF && tier > 0) snprintf(buf, (size_t)n, "%s %s", QUAL_NAME[tier > NQUAL ? NQUAL - 1 : tier - 1], base);
    else if (tier <= 0) snprintf(buf, (size_t)n, "%s", base);
    else snprintf(buf, (size_t)n, "%s %s", base, roman(tier));
}

void gear_name(const Offer *o, char *buf, int n) {
    if (o->kind == GK_PIECE) piece_full_name(&o->piece, buf, n);
    else gear_item_name(o->kind, o->id, o->tier, buf, n);
}

const char *gear_blurb(const Offer *o) {
    switch (o->kind) {
    case GK_ATK: return ATK_BLURB[o->id];
    case GK_DEF: return DEF_BLURB[o->id];
    default: return o->piece.kind == IK_BAG ? "KEEPS WHAT YOU DON'T WEAR" : "";
    }
}

int gear_color(int kind, int id) {
    static const int A[NATK] = { R_FLAME, R_GLOW, R_GLOW, R_BONE, R_PURPLE, R_FLAME, R_GHOST, R_GOLD, R_MOSS };
    static const int D[NDEF] = { R_BONE, R_PURPLE, R_GHOST, R_BLOOD, R_MOSS, R_GLOW, R_PURPLE, R_GOLD, R_GHOST };
    return kind == GK_DEF ? D[id] : A[id];
}
int offer_color(const Offer *o) {
    if (o->kind == GK_DEF && o->tier > 0) return QUAL_RAMP[o->tier > NQUAL ? NQUAL - 1 : o->tier - 1];
    return o->kind == GK_PIECE ? QUAL_RAMP[o->piece.quality] : gear_color(o->kind, o->id);
}

/* ---- spells ---- */

float spell_damage_at(const Gear *g, int atk, int t) {
    static const float B[NATK] = { 10.0f, 7.0f, 8.0f, 12.0f, 9.0f, 18.0f, 4.0f, 4.0f, 9999.0f };
    if (t < 0) t = 0;
    return B[atk] * (1.0f + 0.25f * t) * gear_ember(g);
}
float spell_damage(const Gear *g, int atk) { return spell_damage_at(g, atk, g->atk[atk]); }

/* Spirit: free at tier 1 and below (and the fireball you start with); above, a cast costs its damage
 * over SPIRIT_REF (a tier-2 Fire Bolt about 0.1), at most 1 and never under 0.1, less Spirit Cost
 * Reduction. Seeker Wisps cost 1 a wisp. The BFS9000 keeps its own price in souls (bfs_cost). */
#define SPIRIT_REF 150.0f
int wisp_count(int tier) { return 3 + (tier >= 3) + (tier >= 6); }
float spell_cost(const Gear *g, int atk, int tier) {
    if (tier <= 1 || atk == AT_BFS) return 0.0f;
    float keep = 1.0f - gear_total(g, ST_THRIFT) / 100.0f;
    if (atk == AT_WISPS) return (float)wisp_count(tier) * fmaxf(0.1f, keep);
    return fmaxf(0.1f, fminf(1.0f, spell_damage_at(g, atk, tier) / SPIRIT_REF) * keep);
}
/* Soul Drain's healing: half a soul for each point restored at tier 1, down to a quarter at tier 11 */
float heal_cost(const Gear *g, int tier) {
    float f = fmaxf(0.25f, 0.5f - 0.025f * (float)(tier - 1));
    return f * (1.0f - gear_total(g, ST_THRIFT) / 100.0f);
}
float spell_cycle(const Gear *g, int atk) {
    static const float B[NATK] = { 1.95f, 1.8f, 2.2f, 2.0f, 2.0f, 2.9f, 2.2f, 0.7f, 3.0f };
    int t = g->atk[atk] < 0 ? 0 : g->atk[atk];
    return B[atk] / (1.0f + 0.07f * t) / (1.0f + gear_total(g, ST_HASTE) / 100.0f);
}
int bfs_cost(int tier) { int c = 100 - 10 * (tier - 1); return c < 40 ? 40 : c; }

/* Runes by quality (docs/PLAN-b6.md): each works at about the tier it used to be bought at, and the
 * cooldown that went with it; an Exquisite one comes round sooner, and does a second thing. */
int rune_tier(int q) { static const int T[NQUAL + 1] = { 0, 2, 5, 8, 12, 18 }; return T[q < 0 ? 0 : (q > NQUAL ? NQUAL : q)]; }
int rune_from_tier(int t) { return t <= 0 ? 0 : (t <= 3 ? 1 : (t <= 6 ? 2 : (t <= 10 ? 3 : 4))); }
float def_cooldown(int q) {
    if (q >= RUNE_EXQ) return 16.0f;
    return fmaxf(20.0f, 45.0f * powf(0.88f, (float)(rune_tier(q) - 1)));
}
