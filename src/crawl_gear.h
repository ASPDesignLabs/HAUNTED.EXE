/* What the CATACOMB caster can buy from the merchant, keep and wear: the spellbook (attack and
 * defensive spells), a robe, a hood and up to four rings, with tiers that scale strength and
 * price.  Pure data and arithmetic; the game decides when things happen. */
#ifndef CRAWL_GEAR_H
#define CRAWL_GEAR_H

#include "core.h"

enum { AT_FIRE, AT_FROST, AT_CHAIN, AT_SPEAR, AT_DRAIN, AT_HELL, AT_WISPS, AT_SPARKS, AT_BFS, NATK };
enum { DF_WARD, DF_BLINK, DF_MIRROR, DF_MEND, DF_PURIFY, DF_NOVA, DF_DREAD, DF_SLIP, DF_REAP, NDEF };
enum { RB_CLOAK, RB_PLAIN, RB_SILK, RB_SHADOW, RB_THORN, RB_BLESSED, NROBE };   /* RB_CLOAK: what it starts in */
enum { HD_CLOAK, HD_PLAIN, HD_WARDED, HD_HUNTER, HD_COWL, NHOOD };
enum { RG_BONE, RG_SOUL, RG_HASTE, RG_EMBER, RG_SEEKER, NRING };
enum { GK_ATK, GK_DEF, GK_ROBE, GK_HOOD, GK_RING };     /* what kind of thing an offer is */
enum { POOL_ATTACK, POOL_DEFENSE, POOL_GEAR, NPOOL };
#define NFINGERS 4

typedef struct { int kind, id, tier, price, rare; } Offer;

typedef struct {
    int8_t atk[NATK];                   /* tier known, -1 if not; Fire Bolt starts at tier 0 (the fireball) */
    int8_t def[NDEF];                   /* tier known, 0 if not */
    int8_t robe, robe_tier, hood, hood_tier;      /* worn, and its tier (kept in step by gear_sync) */
    int8_t ring[NFINGERS], ring_tier[NFINGERS];   /* ring type on each finger, -1 if bare */
    int8_t robe_own[NROBE], hood_own[NHOOD], ring_own[NRING];   /* the wardrobe: tier owned of each, 0 = none */
} Gear;

void gear_reset(Gear *g);
Offer gear_offer(int pool, int level, uint32_t *rng);     /* what the merchant puts out on one pedestal */
int  gear_price(int kind, int id, int tier, int rare);
float gear_haul(int level);                              /* about what a level's treasure comes to */
void gear_take(Gear *g, const Offer *o);                 /* into the wardrobe, and worn if it is better */
void gear_sync(Gear *g);                                 /* worn tiers from the wardrobe */
void gear_wear_robe(Gear *g, int id);                    /* id must be owned, or the cloak */
void gear_wear_hood(Gear *g, int id);
void gear_wear_ring(Gear *g, int finger, int id);        /* -1 bares the finger; a ring worn elsewhere moves */
void gear_item_name(int kind, int id, int tier, char *buf, int n);
float gear_value(const Gear *g, const Offer *o, int level);   /* how much better off the caster would be (0 = useless) */
void gear_name(const Offer *o, char *buf, int n);        /* "FROST SHARD III" */
const char *gear_blurb(const Offer *o);                  /* one line on what it does */
int  gear_color(int kind, int id);                       /* palette ramp the item glows in */
int  gear_robe_ramp(int robe, int tier);                 /* the cloth's colour, on the sleeve */
int  gear_hood_ramp(int hood, int tier);

/* the caster's numbers, from what it wears */
int   gear_armor(const Gear *g);                         /* points taken off every blow */
float gear_magic_resist(const Gear *g);                  /* fraction of fire and magic shrugged off */
float gear_dodge(const Gear *g);
int   gear_thorns(const Gear *g);                        /* damage back to whatever bites or strikes */
float gear_regen(const Gear *g);                         /* HP per second */
float gear_poison_resist(const Gear *g);                 /* poison and slow last this much less */
int   gear_max_hp(const Gear *g);
float gear_soul_mult(const Gear *g);
float gear_cowl(const Gear *g);                          /* chance of a bonus soul */
float gear_haste(const Gear *g);                         /* walking speed factor */
float gear_ember(const Gear *g);                         /* spell damage factor */
int   gear_seek(const Gear *g);                          /* senses treasure this many cells through walls; 0 = no */
float gear_hunter(const Gear *g);                        /* how much further it spots lurkers */
int   gear_rings_worn(const Gear *g);

/* spells */
float spell_damage(const Gear *g, int atk);              /* per hit, with the Ember ring */
float spell_cycle(const Gear *g, int atk);               /* seconds between casts */
int   bfs_cost(int tier);                                /* souls a BFS9000 shot burns */
float def_cooldown(int tier);
extern const char *const ATK_NAME[NATK], *const DEF_NAME[NDEF];

#endif
