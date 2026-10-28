/* What the CATACOMB caster can buy from the merchant, keep and wear: the spellbook (attack spells by
 * tier, defensive runes by quality) and pieces to wear (docs/PLAN-items.md): bracers, boots, an amulet, a hood,
 * robes and a ring, each of a quality, rolled with a few stats and now and then an ability, and a bag
 * to keep spares in. Pure data and arithmetic; the game decides when things happen. */
#ifndef CRAWL_GEAR_H
#define CRAWL_GEAR_H

#include "core.h"

enum { AT_FIRE, AT_FROST, AT_CHAIN, AT_SPEAR, AT_DRAIN, AT_HELL, AT_WISPS, AT_SPARKS, AT_BFS, NATK };
enum { DF_WARD, DF_BLINK, DF_MIRROR, DF_MEND, DF_PURIFY, DF_NOVA, DF_DREAD, DF_SLIP, DF_REAP, NDEF };
enum { GK_ATK, GK_DEF, GK_PIECE };                     /* what kind of thing an offer is */
enum { POOL_ATTACK, POOL_GEAR };                         /* the merchant's; runes come from the Rune Shrine */

enum { Q_COMMON, Q_UNCOMMON, Q_RARE, Q_ULTRA, Q_EXQUISITE, NQUAL };
enum { IK_NONE, IK_WRIST, IK_BOOTS, IK_NECK, IK_HOOD, IK_ROBE, IK_RING, IK_BAG, NIK };   /* what a piece is */
enum { SL_WRIST_L, SL_WRIST_R, SL_BOOTS, SL_NECK, SL_HOOD, SL_ROBE, SL_RING, NSLOT };   /* where it is worn */
enum { ST_ARMOR, ST_DODGE, ST_HP, ST_REGEN, ST_RES_POISON, ST_RES_BURN, ST_RES_SLOW, ST_MOVE, ST_QUIET,
       ST_HASTE, ST_THRIFT, ST_DAMAGE, ST_FIRE, ST_FROST, ST_SHOCK, ST_EFFECT,
       ST_TREASURE, ST_LUCK, NSTAT };                   /* the last two never roll on a piece: only the caster's own (1.0.0b9) */
enum { AB_NONE, AB_SWIFT, AB_ARCANE, AB_TREASURE, AB_KEYMAKER, AB_COMMUNE, AB_THORNS, AB_MIRRORED, NABIL };
#define PIECE_STATS 6                                   /* five at most, and one an ability may add */
#define BAG_MAX 5

typedef struct {
    uint8_t kind, quality, level, anointed, ability, nstat;   /* kind IK_NONE: nothing there */
    uint8_t stat[PIECE_STATS];
    int16_t val[PIECE_STATS];                           /* in tenths (hundredths for health restored) */
} Piece;

typedef struct { int kind, id, tier, price, rare; Piece piece; } Offer;   /* a piece: id its kind, tier its level; a rune: tier its quality, 1-5 */

typedef struct {
    int8_t atk[NATK];                   /* tier known, -1 if not; Fire Bolt starts at tier 0 (the fireball) */
    int8_t def[NDEF];                   /* quality held: 0 none, 1 Common to 5 Exquisite (RUNE_EXQ) */
    Piece worn[NSLOT];
    Piece bag[BAG_MAX];
    uint8_t bag_size, bag_quality;      /* 0: no bag */
    int16_t bones;                      /* for the spear */
    uint8_t has_held, spare;            /* a ware held for the next visit */
    Offer held;
} Gear;

void gear_reset(Gear *g);
Offer gear_offer(int pool, int level, const Gear *g, uint32_t *rng);   /* what the merchant puts out on one pedestal */
int  gear_price(int kind, int id, int tier, int rare);   /* spells and runes */
float gear_haul(int level);                              /* about what a level's treasure comes to */
enum { TAKE_LEARNT, TAKE_WORN, TAKE_WORN_OLD_BAGGED, TAKE_WORN_OLD_LOST, TAKE_BAGGED, TAKE_LOST, TAKE_BAG };
int  gear_take(Gear *g, const Offer *o);                 /* into the book, or worn if it is better (the autopilot's rule) */
int  gear_take_wear(Gear *g, const Offer *o);            /* a person buying: worn at once, the old piece bagged or lost */
int  gear_shop_purchases(int level);                     /* wares a visit: one in run I, two in run II, three after */
int  gear_shop_pedestals(int level);                     /* five to level 9, six to 14, seven after */
float gear_value(const Gear *g, const Offer *o, int level);   /* how much better off the caster would be (0 = useless) */
void gear_name(const Offer *o, char *buf, int n);        /* "FROST SHARD III", or a piece's whole name */
void gear_item_name(int kind, int id, int tier, char *buf, int n);   /* a spell by its tier, a rune by its quality */
const char *gear_blurb(const Offer *o);                  /* one line on what it does */
int  gear_color(int kind, int id);                       /* palette ramp a spell or rune glows in */
int  offer_color(const Offer *o);                        /* and any ware, a piece by its quality */

/* pieces */
Piece piece_roll(int kind, int level, int quality, uint32_t *rng);   /* quality -1: by the depth's odds */
int  piece_quality_roll(int level, uint32_t *rng);
void piece_anoint(Piece *p);                                         /* a Black Altar's blessing */
Piece piece_make(int kind, int level, int stat, uint32_t *rng);    /* a Common piece of one stat (old saves' robes and rings) */
int  piece_price(const Piece *p);
void piece_name(const Piece *p, char *line1, char *line2, int n);  /* "ANOINTED RARE SWIFT", "BOOTS OF THE LIGHTFOOT" */
void piece_full_name(const Piece *p, char *buf, int n);
void piece_stat_text(int stat, int val, char *buf, int n);          /* "ARMOR +3.5" */
void piece_ability_text(const Piece *p, char *buf, int n);          /* "THORNS: 7 BACK TO WHAT STRIKES" */
float piece_worth(const Piece *p, int level);            /* what it does for the autopilot, roughly */
int  piece_slot(const Gear *g, int kind);                /* where a new piece of this kind would go (the weaker bracer) */
int  piece_grade(int ability, int quality, int level, int which);   /* an ability's numbers, see crawl_gear.c */
int  bag_capacity(int quality);
const char *bag_name(int size);
int  gear_bag_free(const Gear *g);                       /* empty places in the bag */
int  gear_wear(Gear *g, int slot, int bag_index);        /* swap a bag item into a slot; 0 if it can't go there */
extern const char *const QUAL_NAME[NQUAL], *const KIND_NAME[NIK], *const STAT_NAME[NSTAT], *const ABIL_NAME[NABIL];
extern const int QUAL_RAMP[NQUAL];

/* the caster's numbers, from what it wears: totals after abilities and caps */
float gear_total(const Gear *g, int stat);               /* in the stat's own units (points, % or HP/s) */
float gear_total_raw(const Gear *g, int stat);           /* before abilities and caps */
int   gear_capped(const Gear *g, int stat);              /* worn past its cap: any more is wasted */
float gear_ability(const Gear *g, int ability, int which);   /* abilities summed, capped */
int   gear_armor(const Gear *g);                         /* points taken off every blow */
float gear_magic_resist(const Gear *g);                  /* fraction of fire and burning shrugged off */
float gear_dodge(const Gear *g);
int   gear_thorns(const Gear *g);                        /* damage back to whatever bites or strikes */
float gear_regen(const Gear *g);                         /* HP per second */
float gear_poison_resist(const Gear *g);                 /* poison lasts this much less */
float gear_slow_resist(const Gear *g);                   /* and slowing */
int   gear_max_hp(const Gear *g);
float gear_haste(const Gear *g);                         /* walking speed factor */
float gear_ember(const Gear *g);                         /* spell damage factor */
float gear_stat_cap(int stat);                           /* the most a total of it counts for; 0: no cap */
int   stat_on_kind(int stat, int kind);                  /* whether a kind of piece can roll it */
float gear_ability_cap(int ability);                     /* the most an ability adds up to; 0: no cap */
/* the caster's own (docs/PLAN-b9.md): what Levels of Atonement bought, added to what is worn under one cap,
 * and how far overlevels have raised that cap. Treasure is Treasure Hunter's and the caster's together */
void  gear_set_own(int stat, float add, float cap_up);
float gear_own(int stat);
float gear_cap_up(int stat);

/* spells, and the spirit (souls) and bones they cost */
#define BONES_MAX 30                                     /* bones the caster can carry */
#define SPEAR_BONES 6                                    /* what buying or improving the spear brings */
float spell_damage(const Gear *g, int atk);              /* per hit, with the damage worn */
float spell_damage_at(const Gear *g, int atk, int tier); /* the same at another tier (a weak cast) */
float spell_cost(const Gear *g, int atk, int tier);      /* spirit a cast takes at this tier: 0 at tier 1 and below */
float heal_cost(const Gear *g, int tier);                /* spirit for each point Soul Drain restores */
int   wisp_count(int tier);                              /* Seeker Wisps a cast sends */
float spell_cycle(const Gear *g, int atk);               /* seconds between casts */
int   bfs_cost(int tier);                                /* souls a BFS9000 shot burns */

/* runes, by quality: one of each held at most, the best, and every one held is worn */
#define RUNE_EXQ (Q_EXQUISITE + 1)                       /* an Exquisite rune, which does a second thing */
int   rune_tier(int q);                                  /* the strength a quality works at: 2, 5, 8, 12, 18 */
int   rune_from_tier(int t);                             /* old saves: 1-3 Common, 4-6 Uncommon, 7-10 Rare, 11 up Ultra-Rare */
float def_cooldown(int q);                               /* seconds between casts of a rune of this quality */
extern const char *const ATK_NAME[NATK], *const DEF_NAME[NDEF];

#endif
