/* The merchant's wares and what they do. See crawl_gear.h. */
#include "crawl_gear.h"
#include "crawl.h"
#include <stdio.h>
#include <string.h>

const char *const ATK_NAME[NATK] = { "FIRE BOLT", "FROST SHARD", "CHAIN LIGHTNING", "BONE SPEAR", "SOUL DRAIN",
                                     "HELLFIRE", "SEEKER WISPS", "RAPID SPARKS", "BFS9000" };
const char *const DEF_NAME[NDEF] = { "BONE WARD", "BLINK", "MIRROR VEIL", "MEND", "PURIFY", "FROST NOVA",
                                     "DREAD", "TIME SLIP", "SOUL REAPING" };
static const char *const ROBE_NAME[NROBE] = { "VIOLET CLOAK", "ROBE", "SPIDER-SILK ROBE", "SHADOW ROBE", "THORNED ROBE", "BLESSED VESTMENTS" };
static const char *const HOOD_NAME[NHOOD] = { "VIOLET HOOD", "HOOD", "WARDED HOOD", "HUNTER'S HOOD", "DEATH'S COWL" };
static const char *const RING_NAME[NRING] = { "BONE CHARM", "SOUL JAR", "RING OF HASTE", "EMBER RING", "SEEKER'S RING" };

static const char *const ATK_BLURB[NATK] = { "THE FIREBALL, HOTTER", "SLOWS WHAT IT HITS", "ARCS TO NEARBY FOES", "PIERCES ALL IN A LINE",
                                             "HEALS YOU AS IT HURTS", "HEAVY BLAST, FLOOR BURNS", "HOMING, ROUND CORNERS",
                                             "QUICK AND LIGHT", "SPENDS SOULS. KILLS ALL." };
static const char *const DEF_BLURB[NDEF] = { "ABSORBS THE NEXT BLOWS", "STEP AWAY FROM HARM", "SENDS MISSILES BACK", "HEALS OVER TIME",
                                             "CURES POISON AND SLOW", "FREEZES ALL AROUND", "THE DEAD FLEE",
                                             "SLOWS ALL BUT YOU", "TRIPLE SOULS A WHILE" };
static const char *const ROBE_BLURB[NROBE] = { "", "SOFTENS EVERY BLOW", "RESISTS POISON, SLOW", "NOW AND THEN, NOT THERE",
                                               "HURTS WHAT HURTS YOU", "SLOWLY HEALS" };
static const char *const HOOD_BLURB[NHOOD] = { "", "A LITTLE MORE ARMOR", "TURNS FIRE AND MAGIC", "SPOTS LURKERS SOONER", "BONUS SOULS" };
static const char *const RING_BLURB[NRING] = { "MORE HEALTH", "MORE SOULS PER KILL", "QUICKER FEET", "STRONGER SPELLS", "SENSES HIDDEN GOLD" };

static uint32_t rnext(uint32_t *s) { *s ^= *s << 13; *s ^= *s >> 17; *s ^= *s << 5; return *s; }
static int rpick(uint32_t *s, int n) { return (int)(rnext(s) % (uint32_t)n); }

void gear_reset(Gear *g) {
    memset(g, 0, sizeof *g);
    for (int i = 0; i < NATK; i++) g->atk[i] = -1;
    g->atk[AT_FIRE] = 0;
    g->robe = RB_CLOAK; g->hood = HD_CLOAK;
    for (int i = 0; i < NFINGERS; i++) g->ring[i] = -1;
}

/* about what the caster finds on a level of this depth; prices are set against it */
/* roughly the gold a level gives up (measured from the autopilot's runs); deeper levels hold more */
float gear_haul(int level) { return level < 1 ? 450.0f : 400.0f + 60.0f * level + 45.0f * level * level; }

/* priced against the haul of the level before the one it suits: armor about one level's gold, spells two */
int gear_price(int kind, int id, int tier, int rare) {
    float p;
    if (kind == GK_ATK && id == AT_BFS) p = gear_haul(tier + 2) * 5.0f;
    else if (kind == GK_ATK || kind == GK_DEF) p = gear_haul(tier - 1) * 1.8f;
    else if (kind == GK_ROBE) p = gear_haul(tier - 1) * 0.9f;
    else p = gear_haul(tier - 1) * 0.75f;                         /* hoods and rings */
    if (rare) p *= 1.25f;
    return ((int)(p + 2.5f) / 5) * 5;
}

Offer gear_offer(int pool, int level, uint32_t *rng) {
    Offer o = { 0 };
    int r = rpick(rng, 12);
    if (r == 0) { o.rare = 1; o.tier = level + 2 + rpick(rng, 2); }    /* a jackpot: well ahead of its time */
    else o.tier = level + (r < 3 ? -1 : (r < 10 ? 0 : 1));
    if (o.tier < 1) o.tier = 1;
    if (pool == POOL_ATTACK) {
        o.kind = GK_ATK;
        if (level >= 3 && rpick(rng, 22) == 0) { o.id = AT_BFS; o.rare = 1; o.tier = 1 + level / 4; }   /* incredibly rare */
        else o.id = rpick(rng, AT_BFS);
    } else if (pool == POOL_DEFENSE) {
        o.kind = GK_DEF; o.id = rpick(rng, NDEF);
    } else {
        int k = rpick(rng, 10);
        if (k < 4) { o.kind = GK_ROBE; o.id = 1 + rpick(rng, NROBE - 1); }
        else if (k < 7) { o.kind = GK_HOOD; o.id = 1 + rpick(rng, NHOOD - 1); }
        else { o.kind = GK_RING; o.id = rpick(rng, NRING); }
    }
    o.price = gear_price(o.kind, o.id, o.tier, o.rare);
    return o;
}

/* ---- worth, for the autopilot ---- */

static float atk_worth(int id, int tier) {
    static const float B[NATK] = { 1.0f, 0.9f, 1.1f, 1.0f, 1.0f, 1.05f, 0.95f, 0.9f, 3.0f };
    return tier < 0 ? 0.0f : B[id] * (1.0f + 0.3f * tier);
}
static float def_worth(const Gear *g, int id, int tier, int level) {
    static const float B[NDEF] = { 1.0f, 0.8f, 0.9f, 1.1f, 0.6f, 0.85f, 0.8f, 0.85f, 0.7f };
    if (tier <= 0) return 0.0f;
    float b = B[id];
    if (id == DF_PURIFY && level >= 4) b += 0.4f;                  /* spiders */
    if (id == DF_REAP && g->atk[AT_BFS] >= 0) b += 0.6f;
    if (id == DF_MIRROR && level >= 2) b += 0.2f;                  /* wall skulls */
    return b * (1.0f + 0.25f * tier);
}
static float robe_worth(int id, int tier, int level) {
    static const float B[NROBE] = { 0.0f, 1.0f, 0.6f, 0.9f, 0.8f, 0.9f };
    float b = B[id] + (id == RB_SILK && level >= 4 ? 0.4f : 0.0f);
    return b * tier;
}
static float hood_worth(const Gear *g, int id, int tier, int level) {
    static const float B[NHOOD] = { 0.0f, 0.5f, 0.5f, 0.4f, 0.3f };
    float b = B[id];
    if (id == HD_WARDED && level >= 2) b += 0.3f;
    if (id == HD_HUNTER && level >= 4) b += 0.3f;
    if (id == HD_COWL && g->atk[AT_BFS] >= 0) b += 0.4f;
    return b * tier;
}
static float ring_worth(const Gear *g, int id, int tier) {
    static const float B[NRING] = { 0.7f, 0.3f, 0.6f, 0.9f, 0.5f };
    float b = B[id] + (id == RG_SOUL && g->atk[AT_BFS] >= 0 ? 0.5f : 0.0f);
    return b * tier;
}

float gear_value(const Gear *g, const Offer *o, int level) {
    switch (o->kind) {
    case GK_ATK: {
        float v = atk_worth(o->id, o->tier) - atk_worth(o->id, g->atk[o->id]);
        if (g->atk[o->id] < 0) v += 0.3f;                           /* a new trick for the book */
        return v > 0.0f ? v * 1.4f : 0.0f;                          /* spells weigh heavier than armor */
    }
    case GK_DEF: {
        float v = def_worth(g, o->id, o->tier, level) - def_worth(g, o->id, g->def[o->id], level);
        if (g->def[o->id] <= 0) v += 0.3f;
        return v > 0.0f ? v * 1.4f : 0.0f;
    }
    case GK_ROBE: {                                                 /* only worth it if it beats what is worn */
        if (g->robe_own[o->id] >= o->tier) return 0.0f;
        float v = robe_worth(o->id, o->tier, level) - robe_worth(g->robe, g->robe_tier, level);
        return v > 0.0f ? v : 0.0f;
    }
    case GK_HOOD: {
        if (g->hood_own[o->id] >= o->tier) return 0.0f;
        float v = hood_worth(g, o->id, o->tier, level) - hood_worth(g, g->hood, g->hood_tier, level);
        return v > 0.0f ? v : 0.0f;
    }
    default: {
        int same = -1, free_f = -1; float wv = 1e9f;
        for (int f = 0; f < NFINGERS; f++) {
            if (g->ring[f] == o->id) same = f;
            if (g->ring[f] < 0) { if (free_f < 0) free_f = f; continue; }
            float v = ring_worth(g, g->ring[f], g->ring_tier[f]);
            if (v < wv) wv = v;
        }
        int own = g->ring_own[o->id];
        if (own > 0) {                                             /* one of these already: it grows stronger */
            int nt = own + 1 > o->tier ? own + 1 : o->tier;
            float gain = ring_worth(g, o->id, nt) - ring_worth(g, o->id, own);
            if (same >= 0) return gain;
            float v = ring_worth(g, o->id, nt) - (free_f >= 0 ? 0.0f : wv);   /* and it would go on a finger */
            return v > 0.0f ? v : 0.0f;
        }
        float v = ring_worth(g, o->id, o->tier) - (free_f >= 0 ? 0.0f : wv);
        return v > 0.0f ? v : 0.0f;
    }
    }
}

void gear_sync(Gear *g) {
    g->robe_tier = g->robe == RB_CLOAK ? 0 : g->robe_own[g->robe];
    g->hood_tier = g->hood == HD_CLOAK ? 0 : g->hood_own[g->hood];
    for (int f = 0; f < NFINGERS; f++) g->ring_tier[f] = g->ring[f] < 0 ? 0 : g->ring_own[g->ring[f]];
}

void gear_wear_robe(Gear *g, int id) { if (id == RB_CLOAK || g->robe_own[id] > 0) g->robe = (int8_t)id; gear_sync(g); }
void gear_wear_hood(Gear *g, int id) { if (id == HD_CLOAK || g->hood_own[id] > 0) g->hood = (int8_t)id; gear_sync(g); }
void gear_wear_ring(Gear *g, int finger, int id) {
    if (finger < 0 || finger >= NFINGERS || (id >= 0 && g->ring_own[id] <= 0)) return;
    if (id >= 0)
        for (int f = 0; f < NFINGERS; f++) if (g->ring[f] == id) g->ring[f] = g->ring[finger];   /* swap fingers */
    g->ring[finger] = (int8_t)id;
    gear_sync(g);
}

/* into the wardrobe; worn at once if it is better than what is on (the autopilot's rule) */
void gear_take(Gear *g, const Offer *o) {
    switch (o->kind) {
    case GK_ATK: if (o->tier > g->atk[o->id]) g->atk[o->id] = (int8_t)o->tier; break;
    case GK_DEF: if (o->tier > g->def[o->id]) g->def[o->id] = (int8_t)o->tier; break;
    case GK_ROBE:
        if (o->tier > g->robe_own[o->id]) g->robe_own[o->id] = (int8_t)o->tier;
        if (robe_worth(o->id, g->robe_own[o->id], o->tier) >= robe_worth(g->robe, g->robe_tier, o->tier)) g->robe = (int8_t)o->id;
        break;
    case GK_HOOD:
        if (o->tier > g->hood_own[o->id]) g->hood_own[o->id] = (int8_t)o->tier;
        if (hood_worth(g, o->id, g->hood_own[o->id], o->tier) >= hood_worth(g, g->hood, g->hood_tier, o->tier)) g->hood = (int8_t)o->id;
        break;
    default: {
        int own = g->ring_own[o->id];
        if (own > 0) {                                              /* the same ring again: it grows stronger */
            int nt = own + 1 > o->tier ? own + 1 : o->tier;
            g->ring_own[o->id] = (int8_t)(nt > 12 ? 12 : nt);
        } else g->ring_own[o->id] = (int8_t)o->tier;
        gear_sync(g);
        for (int f = 0; f < NFINGERS; f++) if (g->ring[f] == o->id) { gear_sync(g); return; }   /* already on */
        int f = -1; float wv = 1e9f;
        for (int k = 0; k < NFINGERS; k++) {
            if (g->ring[k] < 0) { f = k; break; }
            float v = ring_worth(g, g->ring[k], g->ring_tier[k]);
            if (v < wv) { wv = v; f = k; }                           /* all four taken: off comes the weakest, into the bag */
        }
        if (ring_worth(g, o->id, g->ring_own[o->id]) >= wv || g->ring[f] < 0) g->ring[f] = (int8_t)o->id;
    }
    }
    gear_sync(g);
}

static const char *roman(int t) {
    static const char *const R[] = { "", "I", "II", "III", "IV", "V", "VI", "VII", "VIII", "IX", "X", "XI", "XII", "XIII", "XIV", "XV" };
    return t >= 0 && t < 16 ? R[t] : "XV+";
}

void gear_item_name(int kind, int id, int tier, char *buf, int n) {
    const char *base = "";
    static const char *const PLAIN_ROBE[4] = { "PADDED ROBE", "QUILTED ROBE", "MAIL-LINED ROBE", "ARCHMAGE'S ROBE" };
    static const char *const PLAIN_HOOD[4] = { "CLOTH HOOD", "LEATHER HOOD", "MAIL COWL", "ARCHMAGE'S HOOD" };
    int pt = tier < 1 ? 1 : tier;
    switch (kind) {
    case GK_ATK: base = ATK_NAME[id]; break;
    case GK_DEF: base = DEF_NAME[id]; break;
    case GK_ROBE: base = id == RB_PLAIN ? PLAIN_ROBE[pt > 6 ? 3 : (pt - 1) / 2] : ROBE_NAME[id]; break;
    case GK_HOOD: base = id == HD_PLAIN ? PLAIN_HOOD[pt > 6 ? 3 : (pt - 1) / 2] : HOOD_NAME[id]; break;
    default: base = RING_NAME[id]; break;
    }
    if (tier <= 0) snprintf(buf, (size_t)n, "%s", base);
    else snprintf(buf, (size_t)n, "%s %s", base, roman(tier));
}

void gear_name(const Offer *o, char *buf, int n) { gear_item_name(o->kind, o->id, o->tier, buf, n); }

const char *gear_blurb(const Offer *o) {
    switch (o->kind) {
    case GK_ATK: return ATK_BLURB[o->id];
    case GK_DEF: return DEF_BLURB[o->id];
    case GK_ROBE: return ROBE_BLURB[o->id];
    case GK_HOOD: return HOOD_BLURB[o->id];
    default: return RING_BLURB[o->id];
    }
}

int gear_color(int kind, int id) {
    static const int A[NATK] = { R_FLAME, R_GLOW, R_GLOW, R_BONE, R_PURPLE, R_FLAME, R_GHOST, R_GOLD, R_MOSS };
    static const int D[NDEF] = { R_BONE, R_PURPLE, R_GHOST, R_BLOOD, R_MOSS, R_GLOW, R_PURPLE, R_GOLD, R_GHOST };
    static const int RG[NRING] = { R_BONE, R_GHOST, R_GLOW, R_FLAME, R_MOSS };
    switch (kind) {
    case GK_ATK: return A[id];
    case GK_DEF: return D[id];
    case GK_ROBE: return gear_robe_ramp(id, 1);
    case GK_HOOD: return gear_hood_ramp(id, 1);
    default: return RG[id];
    }
}

int gear_robe_ramp(int robe, int tier) {
    switch (robe) {
    case RB_PLAIN: return tier <= 2 ? R_WARM : (tier <= 4 ? R_BRICK : (tier <= 6 ? R_MOSS : R_PURPLE));
    case RB_SILK: return R_GHOST;
    case RB_SHADOW: return R_IRON;
    case RB_THORN: return R_WOOD;
    case RB_BLESSED: return R_BONE;
    default: return R_PURPLE;
    }
}
int gear_hood_ramp(int hood, int tier) {
    switch (hood) {
    case HD_PLAIN: return tier <= 2 ? R_WARM : (tier <= 4 ? R_WOOD : (tier <= 6 ? R_IRON : R_PURPLE));
    case HD_WARDED: return R_IRON;
    case HD_HUNTER: return R_MOSS;
    case HD_COWL: return R_IRON;
    default: return R_PURPLE;
    }
}

/* ---- the caster's numbers ---- */

static int ring_tier(const Gear *g, int id) {
    for (int f = 0; f < NFINGERS; f++) if (g->ring[f] == id) return g->ring_tier[f];
    return 0;
}

int gear_armor(const Gear *g) {
    static const float RB[NROBE] = { 0.0f, 1.0f, 0.6f, 0.4f, 0.5f, 0.4f };
    int a = (int)(RB[g->robe] * g->robe_tier);
    if (g->hood == HD_PLAIN) a += g->hood_tier / 2;
    return a;
}
float gear_magic_resist(const Gear *g) { return g->hood == HD_WARDED ? fminf(0.7f, 0.1f * g->hood_tier) : 0.0f; }
float gear_dodge(const Gear *g) { return g->robe == RB_SHADOW ? fminf(0.35f, 0.04f + 0.03f * g->robe_tier) : 0.0f; }
int   gear_thorns(const Gear *g) { return g->robe == RB_THORN ? 3 + 2 * g->robe_tier : 0; }
float gear_regen(const Gear *g) { return g->robe == RB_BLESSED ? 1.0f / fmaxf(2.0f, 6.0f - 0.4f * g->robe_tier) : 0.0f; }
float gear_poison_resist(const Gear *g) { return g->robe == RB_SILK ? fminf(0.8f, 0.12f * g->robe_tier) : 0.0f; }
int   gear_max_hp(const Gear *g) { return 100 + 10 * ring_tier(g, RG_BONE); }
float gear_soul_mult(const Gear *g) { return 1.0f + fminf(1.5f, 0.25f * ring_tier(g, RG_SOUL)); }
float gear_cowl(const Gear *g) { return g->hood == HD_COWL ? fminf(0.6f, 0.08f * g->hood_tier) : 0.0f; }
float gear_haste(const Gear *g) { return 1.0f + fminf(0.3f, 0.05f * ring_tier(g, RG_HASTE)); }
float gear_ember(const Gear *g) { return 1.0f + 0.06f * ring_tier(g, RG_EMBER); }
int   gear_seek(const Gear *g) { int t = ring_tier(g, RG_SEEKER); return t ? 3 + t : 0; }
float gear_hunter(const Gear *g) { return g->hood == HD_HUNTER ? 1.0f + 0.15f * g->hood_tier : 1.0f; }
int   gear_rings_worn(const Gear *g) { int n = 0; for (int f = 0; f < NFINGERS; f++) n += g->ring[f] >= 0; return n; }

float spell_damage(const Gear *g, int atk) {
    static const float B[NATK] = { 10.0f, 7.0f, 8.0f, 12.0f, 9.0f, 18.0f, 4.0f, 4.0f, 9999.0f };
    int t = g->atk[atk] < 0 ? 0 : g->atk[atk];
    return B[atk] * (1.0f + 0.25f * t) * gear_ember(g);
}
float spell_cycle(const Gear *g, int atk) {
    static const float B[NATK] = { 1.95f, 1.8f, 2.2f, 2.0f, 2.0f, 2.9f, 2.2f, 0.7f, 3.0f };
    int t = g->atk[atk] < 0 ? 0 : g->atk[atk];
    return B[atk] / (1.0f + 0.07f * t) / (1.0f + 0.03f * ring_tier(g, RG_EMBER));
}
int bfs_cost(int tier) { int c = 100 - 10 * (tier - 1); return c < 40 ? 40 : c; }
float def_cooldown(int tier) { return fmaxf(20.0f, 45.0f * powf(0.88f, (float)(tier - 1))); }
