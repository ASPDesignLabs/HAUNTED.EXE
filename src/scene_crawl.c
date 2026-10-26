/* CATACOMB - a Catacomb 3-D / Wolfenstein 3-D style crawl through generated
 * crypts, in VGA mode 13h (320x200, 256 colours).
 *
 * A grid raycaster with sliding doors, torch-lit walls (light spreads through
 * open space, not through walls), billboard sprites, and an autopilot that
 * explores with hard 90-degree turns.  Each level opens in the merchant's
 * antechamber (the owner's cat on a chest), where the caster buys one spell,
 * rune or piece of gear with its gold; crawl_gear.c holds the wares.  In the
 * crypt it detours for treasure it has seen or a key it can hear, opens the
 * gold, silver and crimson vaults, and leaves through the exit for a tally.
 * Skeletons throw bones, bats bite, wall skulls gather fire, shield-bearers
 * swing swords and spiders drop from their webs; wraiths only loom.  The hand
 * casts whichever spell suits the target, and its tattooed runes defend it on
 * their own.  HP, gold, souls and the face show only in pop-ups that slide in
 * when something changes, so nothing sits still on an OLED. */
#include "crawl.h"
#include "crawl_gear.h"
#include "gfx.h"
#include "snd.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>
#include "../build/version.h"

#define W 320
#define H 200
#define MAPN 45                             /* 11 blocks of 3x3 cells and the walls between: 4 * 11 + 1 */
#define LMR 4                               /* light-map texels per cell */
#define LMN (MAPN * LMR)
#define PLANE_LEN 0.75f
#define FOCAL ((W / 2) / PLANE_LEN)
#define CRUISE 2.1f                         /* cells per second */
#define WALK 3.0f                           /* cells a second, for a person at the keys */
#define BODY 0.24f                          /* the caster's girth, against walls */

enum { K_FLOOR, K_WALL, K_DOOR, K_PILLAR, K_PROP };   /* K_PROP: low and solid (a grave, a fire pit): walked round, seen and cast over */
enum { TH_CORRIDOR, TH_OSSUARY, TH_CRYPT, TH_BRICK, TH_MOSS, TH_COUNT };
enum { PS_MOVE, PS_TURN, PS_DOOR, PS_AIM, PS_CAST, PS_UNAIM, PS_HOLD, PS_EXIT, PS_SHOP, PS_PLAYER, PS_SETTLE, PS_PUSH, PS_ALTAR, PS_USE };
enum { MOB_SKEL, MOB_GHOST, MOB_BAT, MOB_WSKULL, MOB_SHIELD, MOB_SPIDER,
       MOB_SHAMBLER, MOB_DROWNED, MOB_HOUND, MOB_WRAITH, MOB_HUNTER, MOB_GHOUL, MOB_KNIGHT, MOB_BROOD, MOB_LING, MOB_SAC,
       MOB_SHADE, MOB_IMP, MOB_DEACON, MOB_GARGOYLE };   /* kept in saves (the record): new kinds go at the end */
enum { MS_WALK, MS_DIE, MS_RECOIL, MS_FLEE };   /* the last two: wraiths only */
enum { IT_COINS, IT_CHALICE, IT_GEMS, IT_CROWN, IT_CHEST, IT_KEY, IT_POTION, IT_URN, IT_RING, IT_BONE, IT_SCRIP };   /* IT_SCRIP: color is its boss */
static const int ITEM_VALUE[] = { 10, 25, 50, 100, 250, 0, 0, 0, 0, 0, 0 };
enum { MIS_BONE, MIS_FIRE, MIS_EMBER, MIS_TURNED, MIS_WEB };   /* what the dead throw: bones, a wall skull's fireball, an imp's
                                                                    ember; a spell of yours the Ill-Fated Knight's Mirror Rune
                                                                    turned; the Wandering Spider's glob of web */
enum { DM_PIERCE = 1, DM_ARC = 2, DM_SLOW = 4 };   /* how a hit lands: through shields, round them, chilling */
#define DM_ATK(a) (((a) + 1) << 8)          /* and which spell it is, for the creature's weakness to it */
enum { HURT_BLOW, HURT_MAGIC, HURT_POISON, HURT_STONE, HURT_VOID };   /* STONE: no dodging it; VOID: nothing resists it */
#define NKEYS 3                             /* gold, silver, crimson */
#define SHOT_REFLECT NATK                   /* a shot kind: something the Mirror Veil sent back */
#define NPED 7                              /* the merchant's pedestals, at most */
enum { ALTAR_DORMANT, ALTAR_AWAKE, ALTAR_READY, ALTAR_SPENT };   /* a Black Altar: asleep, seeking, gold, given */
enum { AW_NONE, AW_SEEK, AW_HOME, AW_REST };   /* its wisp: not woken, at a secret's door, on its way home, resting over it */
#define ALTAR_SOULS 10                      /* what waking one costs */
#define ALTAR_BLESS 25                      /* its blessing on your exit wisps, once a level */
#define BLESS_REACH 4.0f                    /* how near a blessed wisp the undead turn */

typedef struct { uint8_t kind, tex, theme, visits; int16_t region, door; uint8_t secret, lamp; } Cell;   /* secret: 1 a hidden closet, 2 its cracked wall; lamp: a pocket's ceiling panel, 1 + its light, 255 dead */
typedef struct { int x, y, along_x; float open, idle; int opening, locked, color; } Door;   /* locked: 1 locked, 2 unlocked */
typedef struct { float x, y, power, radius, phase; int cx, cy, first, count; } Light;
typedef struct { float x, y, phase; int spr, nfr, flags; } Deco;   /* nfr: frames to cycle through */
enum { DF_SWAY = 1,                                                  /* hanging: swings a little, from the top */
       DF_FLIP = 2, DF_FLOAT = 4, DF_SINK = 8,                       /* the Backrooms' things out of place: upside down on the */
       DF_LOOP = 16 };                                               /* ceiling, floating, half sunk; a creature caught in a loop */
typedef struct {
    int type, state, alive; float x, y, z, t, anim, speed; int cx, cy, tx, ty;
    int seen, attacking, hits, misses; float cool, throw_t, contact, orbit, stagger, ignore;   /* skeleton throws, bat bites */
    float fade, st;                                                /* wraiths: how far gone, time in state */
    float hp, maxhp, shield, face;                                 /* face: where a shield-bearer is looking */
    float frozen, feared, slowed, swing;                           /* frost nova, dread, frost shard; a sword raised */
    int wfeared, dreaded; float wfx, wfy;                          /* turned by a blessed wisp: it flees from where that was;
                                                                      dreaded: Exquisite Dread has it, living or dead */
    int sub, guard; float ax, ay, bx, by, sub_t;                   /* wall skulls and spiders: a phase, an anchor, a way back */
    int rank, home, hunts, last_atk, deco, raised;                 /* its rank (RK_*); the room it keeps to (-1: none); whether it may
                                                                      hunt now; the spell that last hurt it; the grave or puddle it
                                                                      rises from, or the bones a deacon calls; how many it has raised */
    float alert, dmgk, lost;                                       /* how long it will go on hunting; its rank's weight of blow; how
                                                                      long since it saw the caster */
    int gen;                                                       /* in a pocket: the order it was made in, 1 on, so the slain stay so */
    int boss, add, small;                                          /* 1 + BOSS_* for a boss; one of its wave's adds; made small */
} Mob;
/* What each kind of creature is (docs/PLAN-bestiary.md): its names, a line for the bestiary, its
 * pictures there (walking towards you, and from the side), the ramp its rank recolours (-1: off the
 * ladder), the first level it is met on, the highest rank it comes in, whether one may be gilded,
 * what it is (KF_*), how tough and quick it is, how big it is drawn and how wide a target, the
 * sound it dies with and what it leaves, and what each spell does to it (x1 plain, x0 nothing). The
 * autopilot plays to these. A kind's number is kept in saves: add new ones at the end. */
enum {
    KF_UNDEAD = 1,                          /* Dread drives it off; it falls back a step when hit */
    KF_SOLID = 2,                           /* it stands in a cell and is in the way */
    KF_SPIRIT = 4,                          /* nothing you carry hurts it */
    KF_UNIQUE = 8,                          /* one of it: THE WRAITH */
    KF_PAGE = 16,                           /* it has a page in the bestiary */
    KF_DOORS = 32,                          /* it pulls doors open */
    KF_MELEE = 64,                          /* it comes right up to you */
    KF_UNCOUNTED = 128,                     /* not a kill: not in the tally, nor slain for achievements */
};
typedef struct {
    const char *name, *plural, *lore;
    int spr, spr2, side, side2, family, debut, top, gild, flags, sfx_die, remains;
    float hp, speed, scale, rad, weak[NATK];
} Kind;
#define W1 { 1, 1, 1, 1, 1, 1, 1, 1, 1 }
static const Kind KINDS[] = {
    /*                                                   fire   frost  chain  spear  drain  hell   wisps  sparks bfs */
    [MOB_SKEL] = { "SKELETON", "SKELETONS", "THE ORDER'S DEAD, STILL AT THEIR POSTS. THEY THROW WHAT THEY ARE MADE OF.",
        SP_SKEL_A, SP_SKEL_B, SP_SKEL_SA, SP_SKEL_SB, PF_BONE, 1, RK_V, 1, KF_UNDEAD | KF_SOLID | KF_PAGE | KF_DOORS, SFX_BONES, SP_SKEL_D3,
        20, 0.9f, 1.0f, 0.42f,                          { 1.0f, 0.75f, 1.25f, 1.25f, 0.75f, 1.25f, 1.0f, 1.0f, 1.0f } },
    [MOB_GHOST] = { "SPECTRE", "SPECTRES", "A CHILL WITH A FACE. IT ONLY WANTS YOU TO KNOW IT IS THERE.",
        SP_GHOST_A, SP_GHOST_B, SP_GHOST_SA, SP_GHOST_SB, -1, 1, RK_I, 0, KF_SPIRIT | KF_PAGE | KF_UNCOUNTED, SFX_GHOST_DIE, -1,
        1e9f, 0.7f, 1.35f, 0.42f,                       { 0 } },
    [MOB_BAT] = { "BAT", "BATS", "THEY COME FOR THE FACE, AND BITE WHEN THEY HAVE IT. DEEP DOWN THEY ARE THE COLOUR OF EMBERS.",
        SP_BAT_A, SP_BAT_B, SP_BAT_SA, SP_BAT_SB, PF_WING, 1, RK_IV, 0, KF_SOLID | KF_PAGE, SFX_BAT, -1,
        10, 1.7f, 1.0f, 0.30f,                          { 1.0f, 1.5f, 1.25f, 1.0f, 1.0f, 1.0f, 1.25f, 1.25f, 1.0f } },
    [MOB_WSKULL] = { "WALL SKULL", "WALL SKULLS", "SET IN THE STONE TO WATCH THE HALLS. WALK UP TO ONE AND MEET ITS EYE, AND IT WAKES.",
        SP_WSKULL1, SP_WSKULL1, SP_WSKULL_S, SP_WSKULL_S, PF_BONE, 3, RK_V, 0, KF_PAGE, SFX_SHATTER, -1,
        1, 0.0f, 0.62f, 0.38f,                          W1 },
    [MOB_SHIELD] = { "SHIELD-BEARER", "SHIELD-BEARERS", "A SKELETON THAT REMEMBERS ITS DRILL. GO ROUND THE SHIELD, OR THROUGH IT.",
        SP_SHIELD_A, SP_SHIELD_B, SP_SHIELD_SA, SP_SHIELD_SB, PF_BONE, 4, RK_V, 1, KF_UNDEAD | KF_SOLID | KF_PAGE | KF_DOORS | KF_MELEE, SFX_BONES, SP_SKEL_D3,
        30, 1.0f, 1.0f, 0.42f,                          { 0.75f, 1.0f, 1.5f, 1.5f, 1.0f, 1.0f, 1.0f, 0.75f, 1.0f } },
    [MOB_SPIDER] = { "SPIDER", "SPIDERS", "IT WAITS IN A CORNER WEB, AND ALL YOU SEE ARE ITS EYES. THE DEEP ONES ARE PALE.",
        SP_SPIDER_A, SP_SPIDER_B, SP_SPIDER_SA, SP_SPIDER_SB, PF_CHITIN, 6, RK_V, 0, KF_PAGE, SFX_SKITTER, -1,
        15, 0.0f, 0.42f, 0.38f,                         { 1.25f, 1.5f, 1.0f, 1.0f, 1.0f, 1.25f, 1.25f, 1.0f, 1.0f } },
    [MOB_SHAMBLER] = { "SHAMBLER", "SHAMBLERS", "THE BURIED, UP AGAIN: SLOW, AND HUNGRY, AND IT WILL HOLD ON TO YOU. FIRE TAKES IT WELL.",
        SP_SHAMB_A, SP_SHAMB_B, SP_SHAMB_SA, SP_SHAMB_SB, PF_FLESH, 2, RK_V, 1, KF_UNDEAD | KF_SOLID | KF_PAGE | KF_DOORS | KF_MELEE, SFX_GROAN, SP_SHAMB_D2,
        32, 0.6f, 1.0f, 0.42f,                          { 1.5f, 0.75f, 1.0f, 1.0f, 1.25f, 1.5f, 1.0f, 1.0f, 1.0f } },
    [MOB_DROWNED] = { "DROWNED", "DROWNED", "SWOLLEN WITH THE WATER IT DIED IN. STAND BACK WHEN IT GOES: IT BURSTS.",
        SP_DROWN_A, SP_DROWN_B, SP_DROWN_SA, SP_DROWN_SB, PF_STONE, 7, RK_V, 1, KF_UNDEAD | KF_SOLID | KF_PAGE | KF_MELEE, SFX_SHATTER, -1,
        46, 0.55f, 1.1f, 0.46f,                         { 1.5f, 0.5f, 1.25f, 1.0f, 1.0f, 1.5f, 1.0f, 1.0f, 1.0f } },
    [MOB_HOUND] = { "BONE HOUND", "BONE HOUNDS", "A DOG'S BONES THAT STILL KNOW THEIR WORK. THEY HUNT IN PAIRS, BITE, AND RUN.",
        SP_HOUND_A, SP_HOUND_B, SP_HOUND_SA, SP_HOUND_SB, PF_BONE, 8, RK_V, 0, KF_UNDEAD | KF_SOLID | KF_PAGE | KF_MELEE, SFX_BONES, SP_HOUND_D,
        14, 2.0f, 0.75f, 0.34f,                         { 1.0f, 1.5f, 1.25f, 1.0f, 0.75f, 1.0f, 1.25f, 1.25f, 1.0f } },
    [MOB_WRAITH] = { "WRAITH", "WRAITHS", "IT WALKS THE HALLS AS YOU DO, AND IT DOES NOT TIRE. FIRE AND LIGHT DRIVE IT BACK, A WHILE.",
        SP_WRAITH_A, SP_WRAITH_B, SP_WRAITH_SA, SP_WRAITH_SB, -1, 9, RK_I, 0, KF_SPIRIT | KF_SOLID | KF_UNIQUE | KF_PAGE | KF_DOORS | KF_MELEE | KF_UNCOUNTED, SFX_WAIL, -1,
        1e9f, 0.75f, 1.6f, 0.5f,                        { 0 } },
    [MOB_HUNTER] = { "HUNTING SPIDER", "HUNTING SPIDERS", "IT DOES NOT WAIT IN A WEB. IT WALKS, ON THE FLOOR OR OVERHEAD, AND IT COMES FOR YOU.",
        SP_HUNTER_A, SP_HUNTER_B, SP_HUNTER_SA, SP_HUNTER_SB, PF_HIDE, 11, RK_V, 0, KF_PAGE | KF_MELEE, SFX_SKITTER, SP_HUNTER_D,
        22, 1.6f, 0.6f, 0.34f,                          { 1.25f, 1.5f, 1.0f, 1.0f, 1.0f, 1.25f, 1.25f, 1.0f, 1.0f } },
    [MOB_GHOUL] = { "GHOUL", "GHOULS", "IT EATS WHAT THE OTHERS LEAVE. CATCH IT FEEDING AND IT IS OPEN.",
        SP_GHOUL_A, SP_GHOUL_B, SP_GHOUL_SA, SP_GHOUL_SB, PF_FLESH, 12, RK_V, 1, KF_UNDEAD | KF_SOLID | KF_PAGE | KF_MELEE, SFX_GROAN, SP_GHOUL_D,
        30, 1.5f, 0.85f, 0.40f,                         { 1.0f, 1.5f, 1.0f, 1.0f, 1.25f, 1.0f, 1.0f, 1.0f, 1.0f } },
    [MOB_KNIGHT] = { "SKELETON KNIGHT", "SKELETON KNIGHTS", "PLATE OVER BONE. HIT IT TILL THE ARMOUR FALLS AWAY; IT GETS QUICKER AS IT DOES.",
        SP_KNIGHT_A, SP_KNIGHT_B, SP_KNIGHT_SA, SP_KNIGHT_SB, PF_ARMOUR, 13, RK_V, 1, KF_UNDEAD | KF_SOLID | KF_PAGE | KF_DOORS | KF_MELEE, SFX_SHATTER, SP_KNIGHT_D,
        60, 0.8f, 1.2f, 0.46f,                          { 0.75f, 1.0f, 1.5f, 1.5f, 1.0f, 1.0f, 0.75f, 0.75f, 1.0f } },
    [MOB_BROOD] = { "BROOD MOTHER", "BROOD MOTHERS", "SHE LAYS AS SHE GOES. KILL HER AND THE SACS DIE WITH HER.",
        SP_BROOD_A, SP_BROOD_B, SP_BROOD_SA, SP_BROOD_SB, PF_CHITIN, 14, RK_V, 0, KF_SOLID | KF_PAGE, SFX_SKITTER, SP_BROOD_D,
        70, 0.6f, 1.0f, 0.5f,                           { 1.5f, 1.0f, 1.25f, 1.0f, 1.0f, 1.5f, 1.0f, 1.0f, 1.0f } },
    [MOB_LING] = { "SPIDERLING", "SPIDERLINGS", "HER CHILDREN. ONE BITE EACH, AND THERE ARE ALWAYS MORE.",
        SP_LING_A, SP_LING_B, SP_LING_A, SP_LING_B, PF_CHITIN, 14, RK_V, 0, KF_PAGE | KF_MELEE, SFX_SKITTER, -1,
        4, 2.2f, 0.22f, 0.24f,                          { 1.5f, 1.5f, 1.5f, 1.0f, 1.0f, 1.5f, 1.0f, 1.25f, 1.0f } },
    [MOB_SAC] = { "EGG SAC", "EGG SACS", "",
        SP_SAC_A, SP_SAC_B, SP_SAC_A, SP_SAC_B, -1, 14, RK_I, 0, KF_UNCOUNTED, SFX_SHATTER, SP_SAC_OPEN,
        6, 0.0f, 0.5f, 0.30f,                           { 1.5f, 1.0f, 1.0f, 1.0f, 0.0f, 1.5f, 1.0f, 1.0f, 1.0f } },
    [MOB_SHADE] = { "SHADE", "SHADES", "A SHAPE OF DARK THAT SHOWS ONLY IN LIGHT. ITS TOUCH TAKES SOULS.",
        SP_SHADE_A, SP_SHADE_B, SP_SHADE_SA, SP_SHADE_SB, -1, 16, RK_I, 0, KF_PAGE | KF_MELEE, SFX_GHOST_DIE, -1,
        30, 1.1f, 1.1f, 0.40f,                          { 1.25f, 0.5f, 1.0f, 0.75f, 1.5f, 1.25f, 1.0f, 1.0f, 1.0f } },
    [MOB_IMP] = { "EMBER IMP", "EMBER IMPS", "BORN OF THE MAGMA, AND IT THROWS IT. COLD IS WHAT IT FEARS.",
        SP_IMP_A, SP_IMP_B, SP_IMP_SA, SP_IMP_SB, PF_CLAY, 17, RK_V, 0, KF_SOLID | KF_PAGE, SFX_SHATTER, SP_IMP_D,
        18, 1.4f, 0.55f, 0.30f,                         { 0.5f, 1.5f, 1.0f, 1.0f, 1.0f, 0.5f, 1.0f, 1.0f, 1.0f } },
    [MOB_DEACON] = { "DEACON", "DEACONS", "A PRIEST OF THE DEAD, AND THEY GET UP WHEN HE ASKS. KILL HIM FIRST.",
        SP_DEACON_A, SP_DEACON_B, SP_DEACON_SA, SP_DEACON_SB, PF_ROBE, 18, RK_V, 1, KF_UNDEAD | KF_SOLID | KF_PAGE | KF_DOORS, SFX_BONES, SP_DEACON_D,
        40, 0.9f, 1.05f, 0.42f,                         { 1.25f, 1.0f, 1.0f, 1.25f, 0.75f, 1.25f, 1.25f, 1.0f, 1.0f } },
    [MOB_GARGOYLE] = { "GARGOYLE", "GARGOYLES", "IT IS ONLY A STATUE UNTIL YOU PASS. STONE SHRUGS OFF FIRE AND FROST.",
        SP_GARG_A, SP_GARG_B, SP_GARG_SA, SP_GARG_SB, PF_STONE, 19, RK_V, 0, KF_PAGE | KF_MELEE, SFX_SHATTER, SP_GARG_D,
        45, 2.2f, 0.9f, 0.42f,                          { 0.5f, 0.5f, 1.5f, 1.5f, 0.0f, 1.0f, 1.0f, 0.75f, 1.0f } },
};
#undef W1
#define NKINDS ((int)(sizeof KINDS / sizeof KINDS[0]))
#define KIND_IS(m, f) (KINDS[(m)->type].flags & (f))
/* the ladder: a rank means about the same strength at any depth; depth shows in which ranks come */
static const float RANK_HP[NRANK] = { 1.0f, 1.6f, 2.4f, 3.5f, 5.0f, 3.0f };
static const float RANK_DMG[NRANK] = { 1.0f, 1.15f, 1.3f, 1.5f, 1.7f, 1.2f };
static const float RANK_SPEED[NRANK] = { 1.0f, 1.05f, 1.1f, 1.15f, 1.15f, 1.0f };
static const int RANK_SOULS[NRANK] = { 1, 2, 3, 4, 6, 5 };
static const float RANK_SIZE[NRANK] = { 1.0f, 1.04f, 1.08f, 1.12f, 1.16f, 1.08f };   /* a second cue, for eyes that mix up the colours */
static const char *const RANK_WORD[NRANK] = { "", "ROTTEN ", "COLD ", "BLOODY ", "BLACK ", "GILDED " };

/* ================================================================== */
/* Bosses (docs/PLAN-b7.md): the last level of each run holds one in a planned arena across the map's
 * south end (rows BAND_Y0 on), behind an antechamber; the rest of the level is made as any level is.
 * A boss is one of the run's creatures, much stronger (Mob.boss), and every quarter of its health it
 * turns untouchable and calls a wave of adds, and again at a tenth; its health stops at each mark.
 * The arenas' maps: # wall, . floor, @ the arch (shut behind you), a where a challenge starts, E the
 * exit, O a pillar, = something low (seen and cast over), G a gate and h a burrow (the adds come out
 * of them), % bone heaps (and out of these), F a font, B a brazier, v an outlet, : a low well, w a
 * sheet of web, and K S U W where each boss stands at first. */
/* ================================================================== */
#define BAND_Y0 29
#define BAND_H 16
#define ARENA_X0 13                         /* the arena's floor, east of the arch */
#define ARENA_REGION 900                    /* its cells' region, and the antechamber's */
#define SEALED 3                            /* a Door's lock: shut behind you until the boss falls */
#define NWAVES 4
#define MAXFONTS 3
#define MAXSPAWN 12
enum { BOSS_KNIGHT, BOSS_SPIDER, BOSS_DEACON, BOSS_WRAITH, NBOSS };
#define BOSS_REC 8                          /* bosses the record keeps room for */
enum { BS_NONE, BS_WAIT, BS_FIGHT, BS_WAVE, BS_RISE, BS_DEAD };
static const float WAVE_MARK[NWAVES] = { 0.75f, 0.50f, 0.25f, 0.10f };
static const char *const MAP_LISTS[BAND_H] = {
    "######.###########G###############G##########",
    "######.#####F...........................#####",
    "###.......###B.OO..................OO..B#####",
    "###.......###..OO......==..==......OO...#####",
    "###.......###...........................#####",
    "###...a.....@.................K.........E####",
    "###.......###...........................#####",
    "###.......###......==..........==.......#####",
    "###.......###...........................#####",
    "#############...........................#####",
    "#############..OO..................OO...#####",
    "#############..OO......==..==......OO...#####",
    "#############B.........................B#####",
    "#############...........................F####",
    "##################G#######F#######G##########",
    "#############################################",
};
static const char *const MAP_WEB[BAND_H] = {
    "######.###############h###F###h##############",
    "######.#############.............############",
    "###.......####v##......w............##v######",
    "###.......#####....OO...........OO....#######",
    "###.......####.....OO...........OO.....######",
    "###...a.....@........w....S.............E####",
    "###.......###............:::............#####",
    "###.......###............:::............#####",
    "###.......###............:::............#####",
    "#############..................w........#####",
    "##############.....OO...........OO.....######",
    "###############....OO...........OO....#######",
    "##############v##............w......##v######",
    "####################.............############",
    "####################F#h#######h#F############",
    "#############################################",
};
static const char *const MAP_PULPIT[BAND_H] = {
    "######.######################################",
    "######.#####F...........................#####",
    "###.......###%...O...%...O...%...O...%..#####",
    "###.......###..==..==..==..==..==.......#####",
    "###.......###...........................#####",
    "###...a.....@.......................U...E####",
    "###.......###...........................#####",
    "###.......###..==..............==.......#####",
    "###.......###...........................#####",
    "############F...........................#####",
    "#############...........................#####",
    "#############..==..==..==..==..==.......#####",
    "#############%...O...%...O...%...O...%..#####",
    "#############...........................#####",
    "##########################F##################",
    "#############################################",
};
static const char *const MAP_BOTTOM[BAND_H] = {
    "######.###################F##################",
    "######.#############.............############",
    "###.......#######....O....B....O....#########",
    "###.......#####B.....................B#######",
    "###.......####.........................######",
    "###...a.....@...........................E####",
    "###.......###....O......................#####",
    "###.......###.................W.........#####",
    "###.......###......................O....#####",
    "#############...........................#####",
    "##############.........................######",
    "###############B.....................B#######",
    "#################....O....B....O....#########",
    "####################.............############",
    "####################F###########F############",
    "#############################################",
};

typedef struct { int type, n, small; } AddGroup;   /* small: drawn and made small (the spider's brood) */
#define WAVE_GROUPS 4
typedef struct {
    const char *name, *lore, *scrip, *short_name;
    int kind;                               /* the creature it is a stronger one of */
    float hp, scale, speed, blow;           /* health in rank-I skeleton knights of the level; size, pace and blow against its kind's */
    const char *const *map;
    int pillar, low;                        /* how its pillars and low things are drawn */
    AddGroup waves[NWAVES][WAVE_GROUPS];
} BossDef;
static const BossDef BOSSES[NBOSS] = {
    [BOSS_KNIGHT] = { "THE ILL-FATED SKELETON KNIGHT", "HE RODE DOWN TO FETCH THE ORDER'S DEAD, AND NEVER RODE BACK UP.",
        "THE ILL-FATED KNIGHT'S ODIOUS SCRIP", "THE ILL-FATED KNIGHT", MOB_KNIGHT, 20.0f, 1.5f, 1.3f, 1.4f, MAP_LISTS, SP_PILLAR, SP_BROKEN_ARMS,
        { { { MOB_SKEL, 1, 0 }, { MOB_SHIELD, 2, 0 } },
          { { MOB_SKEL, 1, 0 }, { MOB_SHIELD, 2, 0 }, { MOB_HOUND, 1, 0 } },
          { { MOB_SKEL, 1, 0 }, { MOB_SHIELD, 2, 0 }, { MOB_HOUND, 2, 0 } },
          { { MOB_SKEL, 2, 0 }, { MOB_SHIELD, 2, 0 }, { MOB_HOUND, 1, 0 }, { MOB_KNIGHT, 1, 0 } } } },
    [BOSS_SPIDER] = { "THE WANDERING HUNTING SPIDER", "IT HAS WALKED THESE CISTERNS SINCE BEFORE THE ORDER CAME. ALL OF THEM ARE ITS.",
        "THE WANDERING SPIDER'S ODIOUS SCRIP", "THE WANDERING SPIDER", MOB_HUNTER, 22.0f, 1.0f, 1.2f, 1.3f, MAP_WEB, SP_PILLAR, SP_PUDDLE_A,
        { { { MOB_HUNTER, 3, 1 } }, { { MOB_HUNTER, 4, 1 } }, { { MOB_HUNTER, 5, 1 } }, { { MOB_HUNTER, 6, 1 } } } },
    [BOSS_DEACON] = { "THE BLACK DEACON", "HE KEPT THE ORDER'S RITES LONG AFTER THE ORDER WAS DUST.",
        "THE BLACK DEACON'S ODIOUS SCRIP", "THE BLACK DEACON", MOB_DEACON, 15.0f, 2.0f, 1.0f, 1.3f, MAP_PULPIT, SP_TIMBER, SP_TOMB_BROKEN,
        { { { MOB_DEACON, 1, 0 }, { MOB_SHAMBLER, 3, 0 } },
          { { MOB_DEACON, 1, 0 }, { MOB_SHAMBLER, 2, 0 }, { MOB_GHOUL, 1, 0 } },
          { { MOB_DEACON, 1, 0 }, { MOB_SHIELD, 2, 0 }, { MOB_GHOUL, 2, 0 } },
          { { MOB_DEACON, 2, 0 }, { MOB_SHIELD, 2, 0 }, { MOB_GHOUL, 1, 0 }, { MOB_KNIGHT, 1, 0 } } } },
    [BOSS_WRAITH] = { "THE WRAITH UNBOUND", "IT HAS WALKED BEHIND YOU SINCE THE NINTH LEVEL. HERE, AT THE BOTTOM, IT STOPS.",
        "THE WRAITH'S ODIOUS SCRIP", "THE WRAITH", MOB_WRAITH, 9.0f, 1.0f, 1.0f, 1.3f, MAP_BOTTOM, SP_OBSIDIAN, SP_OBSIDIAN,
        { { { MOB_SHADE, 2, 0 }, { MOB_DROWNED, 1, 0 } },
          { { MOB_SHADE, 2, 0 }, { MOB_DROWNED, 2, 0 } },
          { { MOB_SHADE, 3, 0 }, { MOB_DROWNED, 2, 0 } },
          { { MOB_SHADE, 4, 0 }, { MOB_DROWNED, 2, 0 } } } },
};
static int boss_of_level(int level) { return crawl_boss(level) ? crawl_set(level) : -1; }   /* which boss its last level holds */

/* Hazards (docs/PLAN-b7.md, section 6): what a boss works on its arena, and the omens of them in the
 * three levels before it. Each warns (warn), harms (on), then fades (off); harm is a share of your health,
 * a hit or a second, and falls only on the dweller. Fire and bile cover cells, in the order they spread. */
enum { HZ_ROCK, HZ_FIRE, HZ_BILE, HZ_VOID, HZ_ARC, HZ_GRASP, NHZ };
#define MAXHAZ 48
#define HZ_CELLS 24
#define MAXOUT 6                            /* a boss's outlets for its bile */
typedef struct {
    int alive, kind, omen, hit, n, src, wx, wy, wtex;   /* src: the outlet it pours from; wx, wy, wtex: an omen's grate and the wall it was */
    float x, y, r, ang, t, warn, on, off, share;
    int8_t cx[HZ_CELLS], cy[HZ_CELLS]; uint8_t cd[HZ_CELLS];
} Hazard;
static int boss_sealed(void);
static int boss_fighting(void);
static int in_arena(float x, float y);
static void boss_spawn(void);
static void update_boss(float dt);
static int font_drink(int i);
static Mob *boss_mob(void);
static void hz_clear(void);

typedef struct { int alive, kind, target; float x, y, z, vx, vy, life, dmg; uint32_t pierced; int tier; } Shot;   /* tier: as cast */
typedef struct { int alive, kind; float x, y, vx, vy, life, spin, dmg, life0; int from, atk; } Bone;   /* life0: an ember's whole flight; from: its thrower;
                                                                                                    atk: a turned spell's kind */
typedef struct { int alive, type, cx, cy, seen, color, lx, ly, secret, gen; float x, y, phase; } Item;   /* lx, ly: the lane cell it is taken from;
                                                                                                   gen: 1 + its place among a pocket's things (0: not a pocket's) */
typedef struct { int alive; float x, y, t, r; int big; } Boom;
typedef struct { int alive, kind; float x0, y0, z0, x1, y1, z1, t; } Beam;   /* lightning and soul drain, for a moment */
typedef struct { int alive; float x, y, t, dps; int foe; } Burn;           /* hellfire on the floor; foe: an ember's, which burns the caster */
typedef struct { int alive; float x, y, t; } Pool;                         /* what a drowned one leaves: foul, a few seconds */
#define MAXPOOLS 4
typedef struct { int x, y, w, h, region, theme; } Room;
/* stand at f, press d: the wall at s slides len cells back into its room (x0..x1, y0..y1). links: the chain
 * it begins (1, a room alone); door: the doorway at the back of its room into the pockets (x -1: none) */
typedef struct { int fx, fy, d, sx, sy, tex, state, rolled, len, links, door_x, door_y, x0, y0, x1, y1; uint32_t seed; } Secret;   /* seed: its pockets' */
#define MAXSECRETS 6
#define SLIDE_TIME 2.3f                     /* how long a secret wall takes to slide right through its room */
#define MAXWISPS 2                          /* the one you summon, and the Black Altar's */
#define WISP_MOTES 192                      /* the trail a wisp leaves hanging in the air */
#define WISP_MOTE_T 12.0f                   /* how long a mote of it hangs there */
#define WISP_PACE (1.5f * WALK)             /* how fast it goes on ahead */
#define WISP_COST 1.0f                      /* a soul a wisp */
enum { WK_EXIT, WK_KEY, WK_ALTAR };         /* where a wisp leads: the way out, a key, the altar's secrets */
enum { WS_KINDLE, WS_FLY, WS_HOVER, WS_GONE };
typedef struct { float x, y, z, t; } Mote;
typedef struct {
    int on, kind, color, state;
    float x, y, z, a, t, flare;             /* where it is, how bright, its age, its going out */
    float hx, hy, vx, vy, retarget, mote_d; /* where it hovers at the end; its way of going */
    int gx, gy, final;                      /* the cell it seeks, and whether that is the end of it */
    int16_t field[MAPN][MAPN];              /* steps to that cell, every way round; -1 where none */
    Mote motes[WISP_MOTES]; int mote_head;
} Wisp;

#define MAXDOORS 64
#define MAXLIGHTS 160
#define MAXLC 420000
#define MAXDECO 900
#define MAXMOBS 32
#define MAXSHOTS 16
#define MAXBOOMS 8
#define MAXBONES 10
#define MAXITEMS 96
#define MAXBEAMS 8
#define MAXBURNS 8
#define BONE_SPEED 5.5f
#define FIRE_SPEED 4.5f
#define EMBER_SPEED 4.0f
#define GARG_Z 0.1f                         /* how high a gargoyle flies: its wings reach the ceiling */
#define DEACON_CHANT 1.6f                   /* how long a deacon chants over the bones before they rise */
#define READOUT_T 3.6f
#define BAR_T 5.0f                          /* the status bar stays this long after anything it should show */
#define BAR_SLIDE 0.3f                      /* and takes this long to slide away or back */
#define FACE_T 2.6f

static const int DX[4] = { 1, 0, -1, 0 }, DY[4] = { 0, 1, 0, -1 };
static const uint8_t BAYER[4][4] = { {0, 8, 2, 10}, {12, 4, 14, 6}, {3, 11, 1, 9}, {15, 7, 13, 5} };

static const char *const LEVEL_NAMES[DESCENT_LEN] = {              /* ten to a run; the last of each is its boss's */
    "THE OSSUARY", "HALL OF TORCHES", "CRYPT OF THE NAMELESS", "THE BONE GALLERY", "THE WARDEN'S HALL",
    "THE CHAPTER HOUSE", "THE PILGRIMS' STAIR", "THE RELIQUARY", "THE KNIGHTS' VAULT", "THE ILL-FATED LISTS",
    "THE DROWNED VAULT", "CATACOMB OF SAINTS", "THE WEEPING STAIR", "CISTERN OF BONES", "THE ABBESS BELOW",
    "THE SILTED NAVE", "THE BLACK CANAL", "THE SPINNERS' GALLERY", "THE HANGING DARK", "THE WANDERER'S WEB",
    "THE CHARNEL MINE", "WARRENS OF THE DEAD", "THE FOSSIL ROAD", "THE BROKEN SEAM", "THE COLOSSUS DEEP",
    "THE TIMBERED DARK", "THE GNAWING CAVES", "THE LAMPLESS SHAFT", "THE FLOODED CHAPEL", "THE DEACON'S PULPIT",
    "THE GLITTERING DARK", "VEINS OF THE EARTH", "THE EMBER ROOT", "THE BLACK GLASS", "THRONE OF THE HOLLOW KING",
    "THE SINGING GEODE", "THE CINDER FIELDS", "THE MOLTEN STAIR", "THE LAST LIGHT", "THE BOTTOM OF EVERYTHING",
};
/* the level's name; past the fortieth they come round again with a numeral: THE OSSUARY II */
static void level_name(int level, char *buf, int n) {
    static const char *const NUM[] = { "", " II", " III", " IV", " V", " VI", " VII", " VIII", " IX", " X" };
    int loop = crawl_loop(level);
    snprintf(buf, (size_t)n, "%s%s", LEVEL_NAMES[(level - 1) % DESCENT_LEN], loop < 10 ? NUM[loop] : " X");
}

/* A run's record: what it has slain, met, found and cleared, and the achievements earned. It is kept
 * in the save with the run (version 2), so each save has its own. Its sizes are fixed so saves keep
 * their shape as the bestiary grows. */
#define REC_KINDS 24
#define ACH_BITS 512                        /* 256 until version 6 */
typedef struct {
    uint16_t kills[REC_KINDS][NRANK];
    uint8_t spell_kills[REC_KINDS][NATK];   /* by which spell, to 255: three, and its effect is known */
    uint8_t seen[REC_KINDS];                /* the ranks of each kind met */
    uint16_t secrets, descents;             /* secrets found; times all the way down (40, 80, 120 ...) */
    uint64_t cleared;                       /* levels 1 to 40 cleared, a bit each */
    uint8_t got[ACH_BITS / 8];              /* achievements earned, a bit each (see ach_bit) */
    struct { uint8_t met; uint16_t slain, tries, best, scrip; } boss[BOSS_REC];   /* version 6: each boss met, slain, tried, its
                                                                                 best time in seconds (0: none yet), scrip won */
} Record;
#define REC_BYTES (REC_KINDS * NRANK * 2 + REC_KINDS * NATK + REC_KINDS + 2 + 2 + 4 + 256 / 8)   /* as packed since version 2 */
#define REC_EXTRA (4 + (ACH_BITS - 256) / 8 + BOSS_REC * 9)   /* version 6: levels 33 to 40 cleared, the achievements past the 256th, the bosses */

/* The run: what carries from level to level, through death and across scene switches */
#define SHRINE_SLOTS 9
#define SHRINE_X 15.5f
#define SHRINE_Y 17.5f                      /* the Rune Shrine, in the merchant's room against the south wall */
static struct {
    int active, level, hp, treasure, souls;
    float soul_frac;
    Gear gear;
    int goal;                               /* gold it is saving towards (0: not saving) */
    int shop_next;                          /* the merchant comes first when the level is (re)entered */
    int loaded;                             /* just read from a save: nothing in G to carry over */
    Record rec;
    int wisp_kind, wisp_color, wisp_auto;   /* the wisp L casts at a tap, once the menu's toggle is on */
    int coins, shrine, attuned;             /* Cursed Coins; the Rune Shrine's rank (0 sanctified, 1-5 the best quality
                                               it gives); the rune kinds it is attuned to, a bit each */
    uint8_t slot_def[SHRINE_SLOTS], slot_q[SHRINE_SLOTS];   /* the shrine's slots: rune kind + 1 (0 empty), quality 1-5 */
    int coin_level, coin_bits;              /* the coins already taken on that level: a bit a chain's end, bit 7 every secret */
    int runs_done;                          /* runs completed: their bosses beaten on the way down */
    int scrip[BOSS_REC], bstep[BOSS_REC];   /* each boss's Named Odious Scrip, held at the Hexed Totem; its challenge's step, 0-5 */
    uint8_t top_rank[REC_KINDS];            /* 1 + the highest rank of each kind met since the last death (0: none yet) */
    int omens;                              /* the hazards met, a bit each (HZ_*): the first of each is named */
} RUN;

/* The console (~): its lines and what is typed, and what a code has done to this session, which lasts
 * until the program is closed: no honours, no saved progress and no graves once one has worked */
#define CON_LINES 48
#define CON_ROWS 5
static struct {
    int open, opens; float slide, t;
    char line[CON_LINES][40]; uint8_t ink[CON_LINES]; int n, scroll;
    char typed[25], last[25]; int ntyped;
    int used, rank;                         /* a code has worked; the rank every creature comes at (-1: as it falls) */
} CON = { .rank = -1 };

static struct {
    uint32_t rs;
    double t, level_t;
    int level, ptheme, vis_x, vis_y, vis_deco;                     /* a pocket's look (PT_*), PT_NONE on the level; its visportal or water cooler */
    Cell map[MAPN][MAPN];
    Room rooms[12]; int nrooms;
    Door doors[MAXDOORS]; int ndoors;
    Light lights[MAXLIGHTS]; int nlights;
    uint16_t lc_idx[MAXLC]; float lc_w[MAXLC]; int nlc;
    float lm[LMN * LMN];
    Deco decos[MAXDECO]; int ndecos;
    uint8_t bulky[MAPN][MAPN];          /* cells holding something big to walk around */
    float fog[MAPN][MAPN];                  /* fog banks, 0..1, thicker the deeper the level */
    Mob mobs[MAXMOBS];
    Shot shots[MAXSHOTS];
    Boom booms[MAXBOOMS];
    Bone bones[MAXBONES];
    Beam beams[MAXBEAMS];
    Burn burns[MAXBURNS];
    Pool pools[MAXPOOLS];
    int wraith_done, wraith_now; float wraith_try;   /* the Wraith has come this level (or is called now); when to look for a place for it */
    int pilgrim;                            /* the feet follow the wisp: 1 by a code, till a key is touched; 2 the test player */
    Item items[MAXITEMS]; int nitems, items_total, items_got, value_total, value_got;
    int nvaults, vault_room[NKEYS], vault_door[NKEYS];   /* one locked room per key colour, in order */
    int ncells, explored;

    /* player */
    float px, py, ang, spd, bob, bobph, camz;
    int cx, cy, h, tx, ty, next_h, state, fired, target, spell;
    float u, st, ang0, ang1, adur, hand, cooldown, calm, cast_len;
    int resume_state;
    float blocked;                      /* seconds spent waiting on a creature */

    float pvx, pvy;                     /* the caster's velocity, for skeletons leading their throws */
    int hp, max_hp, level_treasure, level_souls, has_key[NKEYS];
    double unlock_t;

    /* what is on the caster: poison, slow, and the defensive spells running */
    float poison_t, poison_dps, poison_acc, slow_t, purify_t, regen_acc;
    float ward_hp, ward_t, mirror_t, slip_t, reap_t, mend_t, mend_rate, mirror_flash;   /* mirror_flash: Mirrored just sent one back */
    float ward_shards, decoy_x, decoy_y, decoy_t;   /* Exquisite runes: the Bone Ward's burst when it breaks; Blink's double */
    float whisper_t, commune_t, commune_show, commune_len, commune_r;   /* Treasure Hunter's whisper; Commune's clock and showing */
    int altar_x, altar_y, altar_state, altar_seek;   /* the Black Altar (x -1: none); the secret its wisp seeks */
    int altar_wisp, altar_inside;                    /* where its wisp is (AW_*); whether you have been into the secret it shows */
    int blessed;                                     /* the altar's blessing: exit wisps turn the undead, this level */
    float def_cd[NDEF], def_wait[NDEF], def_hush[NDEF], rune_glow[NDEF];   /* cooldown; a cast on its way; a missed cue; glow on the hand */
    float bfs_t;                        /* the BFS9000's implosion, playing */
    float nova_t, dread_t, rune_lift;   /* rune_lift: the hand comes up to show a rune burning */              /* the frost nova bursting; Dread's shadow going out */
    float face_t, grin_t; int face_side;    /* the portrait popping in, the grin, which way it flinched */

    /* the way out: a door set in a wall, opened from the floor cell in front of it */
    int exit_wx, exit_wy, exit_fx, exit_fy, exit_dir, exit_in_vault, exit_seen;
    int leaving, exiting;                   /* heading for the exit; stepping through it */
    float exit_open, exit_t;
    int kills, kills_total;
    int spawned[NKINDS];                    /* how many of each kind have come on this level: a newcomer comes alone */
    uint8_t named[NKINDS];                  /* the ranks of each kind the caption has named on this level */
    Secret secrets[MAXSECRETS]; int nsecrets, secrets_found, nlinks, links_found;   /* rooms behind cracked walls; every link of every chain */
    int slide, seek_secret; float slide_t;  /* the wall sliding back, and how far; a secret the autopilot means to press */
    int use_x, use_y, use_kind;             /* the autopilot in a chain: a doorway or visportal it means to use (USE_*) */
    Wisp wisps[MAXWISPS];                   /* will-o'-wisps in the crypt: [0] summoned, [1] the altar's */
    int tally;                              /* the level-complete screen */
    double tally_t, clear_time;
    int tally_shown[6], tally_done;           /* one a row: time, kills, treasure, souls, secrets, and a boss's fight */
    float hurt, gold, readout, kick, heartbeat, chill;              /* chill: the cold flash when a wraith shrieks */
    int dead;
    int retries;                        /* deaths on this level so far: the crypt goes a little easier each time */
    double dead_t;

    /* the merchant's antechamber, before each level */
    int shop, shop_step, shop_pick, shop_bought, shop_look, shop_gold, shop_saving, shop_heal;
    Offer offers[NPED];
    int offer_gone[NPED], offer_back[NPED]; /* offer_back: held last visit, back as it was */
    float ped_x[NPED], ped_y[NPED];         /* the pedestals, in an arc round the back wall */
    int ped_n, shop_left, shop_confirm, shop_letgo, shop_held;   /* pedestals; wares still to be had; a ware awaiting a
                                               second E; one awaiting room in the bag; the ware held this visit */
    double shop_t;
    float sa0, sa1, sdur, sx0, sy0, sx1, sy1, smove, sparkle;    /* turning and walking: from, to, how long */
    float cat_blink, cat_slow, cat_ear, cat_trill, cat_look, purr_t, cat_next_blink, cat_next_ear, cat_tail_k;
    int cat_ear_side;
    /* the Rune Shrine, this visit: its decoration; looked at; takes earned and not yet used; pulls made; the
       screen's mode, slot, coins for a pull and favour (0 none, else 1 + 3 x kind + coins - 1); a choice
       awaiting a second Enter; runes on their way into the slots; the autopilot's errand there */
    int sh_deco, sh_look, sh_takes, sh_pulls, sh_mode, sh_slot, sh_coins, sh_fav, sh_arm;
    int sh_qk[6], sh_qq[6], sh_nq, sh_busy, sh_auto;   /* sh_busy: a pull still coming in, or three becoming one */
    float sh_qt, sh_glow, sh_flash[SHRINE_SLOTS], sh_note_t;
    char sh_note[80];

    /* a person at the controls */
    int player, ui, ui_sel, quit_line, status_ret;
    int graves_mode, graves_arm, graves_msg;    /* the save slots: carving (0) or rising (1); a second Enter's slot; why */
    int best_page, best_rank, best_scroll;      /* the bestiary: which creature (or the achievements), which rank, how far down */
    float ui_t;                                 /* a clock that runs while a menu holds the crypt still */
    float pc_t, pc_len, grab_t, hint_t, msg_t;     /* their cast; reaching for a ware; the hints; a message */
    float bar, bar_t, peek, lift;           /* the status bar shown (0..1), time before it goes; H held; the readout raised */
    float hand_base, ready_t;               /* a person's hand: how far up between casts, and how long it stays */
    float l_t; int l_down;                  /* L held this long (a tap casts, a hold opens the menu) */
    int pc_fired, spirit_said;              /* spirit_said: the caption for a weak cast, once a level */
    int click_spent;                        /* the click went on a ware or the cat: no cast until it is let go */
    char msg[64];
    Gear inv0;                              /* the gear as it was when the inventory opened */
    int inv_page;                           /* the wardrobe (0) or the caster's numbers and spellbook (1) */

    float fade, card;
    int ending;
    double end_t;
    float mood_t;

    /* the boss of this level (-1: none) and its fight (docs/PLAN-b7.md): its state (BS_*), its creature,
       the waves called, the adds still standing, the challenge's step (0 on the way down), whether this
       is a challenge from the Portal; the arch's door; the fonts (wall cell, the floor before it, full);
       where the adds come out; where it stood; clocks for the fight, the state, the card and the bar */
    int boss, bstate, bmob, bwave, badds, bstep, bchallenge, barch, bnfont, bnspawn, bhome_x, bhome_y, bstart_x, bstart_y;
    int bfont_x[MAXFONTS], bfont_y[MAXFONTS], bfont_fx[MAXFONTS], bfont_fy[MAXFONTS], bfont_full[MAXFONTS];
    int bspawn_x[MAXSPAWN], bspawn_y[MAXSPAWN];
    float bfight_t, bstate_t, bcard, bbar, bring;
    /* the hazards: those warning or harming now; what they add up to this step; the next omen; the boss's
       next hazard and next second cast, and its pose casting one; whether the void has you; the outlets */
    Hazard haz[MAXHAZ];
    float hz_acc, hz_slow, omen_t, bz_t, bz2_t, bcast_t, sil_k;
    int silenced, bnout, bout_haz[MAXOUT];
    int8_t bout_cx[MAXOUT][HZ_CELLS], bout_cy[MAXOUT][HZ_CELLS]; uint8_t bout_cd[MAXOUT][HZ_CELLS], bout_n[MAXOUT];
    /* the bosses' own ways: a pose (BP_*) and how long it lasts; the knight's Swift Dodge (its clock, where it
       set out and where it is, its pace, the three cooldowns, the time since it set out) and Mirror Rune (its
       flash, and the wait before it turns another); the harm it has taken lately, fading over three seconds */
    int bpose; float bpose_t;
    float bdash_t, bdash_x0, bdash_y0, bdash_x1, bdash_y1, bdash_vx, bdash_vy, bdash_cd[3], bdash_age;
    float bmirror_t, bmirror_cd, bhurt;
    /* the spider's: when it next goes up; its Mend Rune (health when it last mended, mending left, the wait);
       its web on you (what is left of it, all of it, the wait for the next, the rearing before a spit, how
       long since you struck at it); the fifteen-second spiders' clock */
    float bsp_t, bmend_base, bmend_t, bmend_cd, web_hp, web_max, web_cd, web_rear, web_idle, btrickle_t;
    /* the Deacon's: his Bone Ward (what is left of it, the wait for it to grow back), harm taken mid-chant */
    float bward, bward_cd, bchant_dmg;
    /* the Wraith's: its arena's braziers (cell, lit, decoration), the one being kindled and how long it has
       left; its Time Slip (how long it holds, where, the wait); Wraithwalk (the phase: 1 going, 2 gone, 3
       coming out of a wall; its clock; the wait) */
    int bbz_n, bbz_x[8], bbz_y[8], bbz_lit[8], bbz_deco[8], bbz_at;
    float bbz_t, bslip_t, bslip_x, bslip_y, bslip_cd, bgutter_cd;
    int bww; float bww_t, bww_cd;
} G;
enum { BP_NONE, BP_PLANT, BP_RAISE, BP_STRIKE,      /* the knight's: sword planted with a shout, raised, swept; */
       BP_LITANY };                                 /* the Deacon's censer raised, the green light growing */
static float zbuf[W];
enum { UI_NONE, UI_INVENTORY, UI_QUIT, UI_GRAVES, UI_BESTIARY, UI_LETGO, UI_WISPS, UI_ALTAR, UI_SHRINE };   /* the menus a person can open */
static Input IN;                            /* their keys and mouse, this step */

/* pockets (see the pockets' code below): the level as it waits, and where in a chain you are */
#define POCKET_LINKS 8
typedef __typeof__(G) GState;
static GState G_LEVEL, G_KEEP;                                     /* the level as it waits; the caster on the way between */
static struct {
    int level, depth, chain;                                       /* depth: 0 in the level, else the link you are in (pockets are 2 on) */
    int back_x, back_y, on_x, on_y;                                /* this pocket's doorways: the way back, and on (x -1: none) */
    float since;                                                   /* since the last doorway, for the fade */
    uint8_t reached[MAXSECRETS];                                   /* the deepest link found in each chain */
    uint32_t taken[MAXSECRETS][POCKET_LINKS];                      /* the things picked up in each pocket, a bit each */
    uint8_t vis_used[MAXSECRETS];                                  /* each chain's visportal, once it has given */
    uint32_t slain[MAXSECRETS][POCKET_LINKS];                      /* the creatures killed in each pocket, a bit each */
    uint8_t auto_done[MAXSECRETS];                                 /* the autopilot: the chains it has been into, */
    int auto_back; float auto_t;                                   /* whether it is on its way out, and its time in them */
} PK;
/* the pockets' creatures: a quarter stronger in the Backrooms, 30% in Severance (whose also strike sooner) */
static float theme_boost(void) { return G.ptheme == PT_BACKROOMS ? 1.25f : G.ptheme == PT_SEVERANCE ? 1.3f : 1.0f; }

static uint8_t ramp_at(int ramp, int k) { return (uint8_t)(ramp + clampi(k, 0, ramp == R_GOLD ? 12 : 15)); }   /* the gold ramp is short */
static uint8_t qual_text(int q) { return q == Q_COMMON ? (uint8_t)C_WHITE : ramp_at(QUAL_RAMP[q], 13); }   /* white, then green, blue, purple, gold */
static int rnd(int n) { return (int)(rng_next(&G.rs) % (uint32_t)n); }
static float decay(void) { return crawl_decay(G.level); }        /* rot and ruin: 0 as a set begins, 1 on its last level */
static float depth(void) { return crawl_depth(G.level); }         /* how far down: 0 in the Crypts, 1 from the Deep on */
/* a creature's blow: a little more each level, more by its rank, less after each death on the level */
static float theme_boost(void);
static float mob_damage(const Mob *m) {
    float d = (4.0f + 0.8f * (float)G.level) * m->dmgk * theme_boost();
    int r = G.retries < 3 ? G.retries : 3;
    return d * (1.0f - 0.15f * (float)r);
}
static float throw_interval(void) { return 1.5f / powf(1.05f, (float)(G.level - 1)); }   /* 5% quicker each level */
static float rndf(void) { return rng_f(&G.rs); }
static uint32_t hash2(int x, int y) {
    uint32_t h = (uint32_t)x * 73856093u ^ (uint32_t)y * 19349663u ^ (uint32_t)G.level * 83492791u;
    h ^= h >> 13; h *= 0x5bd1e995u; h ^= h >> 15;
    return h;
}

static Cell *cell(int x, int y) {
    static Cell solid = { K_WALL, T_STONE, 0, 0, -1, -1, 0, 0 };
    if (x < 0 || y < 0 || x >= MAPN || y >= MAPN) return &solid;
    return &G.map[y][x];
}
static int is_open(int x, int y) { int k = cell(x, y)->kind; return k == K_FLOOR || k == K_DOOR; }
static int blocks_light(int x, int y) {
    Cell *c = cell(x, y);
    return c->kind == K_WALL || (c->kind == K_DOOR && G.doors[c->door].open < 0.5f);
}
static int slide_covers(int x, int y);
static int blocks_move(int x, int y) {
    Cell *c = cell(x, y);
    if (G.slide >= 0 && slide_covers(x, y)) return 1;
    return c->kind == K_WALL || c->kind == K_PILLAR || c->kind == K_PROP || (c->kind == K_DOOR && G.doors[c->door].open < 0.9f);
}
/* where the caster may walk: a vault door only once its key is in hand */
static int passable(int x, int y) {
    Cell *c = cell(x, y);
    if (c->kind == K_FLOOR) return 1;
    if (c->kind != K_DOOR) return 0;
    const Door *d = &G.doors[c->door];
    return d->locked == SEALED ? 0 : d->locked != 1 || G.has_key[d->color];
}
static int vault_of_room(int x, int y);
static Mob *spawn_mob(int x, int y, int type);
static int may_spawn(int type);
static int occupied(int x, int y, const Mob *self);
static void name_newcomers(void);
static void garg_wake(Mob *m);
static float heading_angle_raw(int h) { return h * (PI_F * 0.5f) + PI_F; }   /* facing back the other way */
static float wrap_pi(float a) { while (a > PI_F) a -= TAU_F; while (a < -PI_F) a += TAU_F; return a; }

/* ================================================================== */
/* Level generation: rooms and mazes                                   */
/* ================================================================== */

static void carve(int x, int y, int region, int theme) {
    Cell *c = &G.map[y][x];
    c->kind = K_FLOOR; c->region = (int16_t)region; c->theme = (uint8_t)theme;
}

static int theme_tex(int theme, uint32_t h) {
    int k = (int)(h % 10);
    switch (theme) {
    case TH_OSSUARY: return k < 6 ? T_SKULLS : (k < 9 ? T_NICHE : T_STONE);
    case TH_CRYPT:   return k < 5 ? T_STONE : (k < 8 ? T_NICHE : T_BANNER);
    case TH_BRICK:   return k < 8 ? T_BRICK : T_BANNER;
    case TH_MOSS:    return k < 6 ? T_MOSS : T_RUBBLE;
    default:         return k < 6 ? T_STONE : (k < 9 ? T_RUBBLE : T_SKULLS);
    }
}

static void add_deco_ex(float x, float y, int spr, int nfr, int flags) {
    if (G.ndecos < MAXDECO) G.decos[G.ndecos++] = (Deco){ x, y, rndf() * 10.0f, spr, nfr, flags };
}
static void add_deco(float x, float y, int spr) {
    add_deco_ex(x, y, spr, 1, spr == SP_CHAINS ? DF_SWAY : 0);
    if (spr == SP_SKULLPILE || spr == SP_BONES2 || spr == SP_URN || spr == SP_CANDLES || spr == SP_CHAINS || spr == SP_BROKEN_ARMS
        || spr == SP_COLLAR || spr == SP_CARCASS || spr == SP_HELM || spr == SP_FUNGUS || spr == SP_SAC_A)
        G.bulky[clampi((int)y, 0, MAPN - 1)][clampi((int)x, 0, MAPN - 1)] = 1;
}

/* static light: record its contribution to every light-map texel it reaches
 * through open space (and the faces of the walls around that space) */
static void light_static(float lx, float ly, float power, float radius) {
    if (G.nlights >= MAXLIGHTS) return;
    Light *L = &G.lights[G.nlights++];
    L->x = lx; L->y = ly; L->power = power; L->radius = radius; L->phase = rndf() * 10.0f;
    L->cx = (int)lx; L->cy = (int)ly; L->first = G.nlc; L->count = 0;
    static int stamp[MAPN][MAPN], qx[MAPN * MAPN], qy[MAPN * MAPN], dist[MAPN][MAPN];
    static int gen;
    gen++;
    int qh = 0, qt = 0, maxd = (int)ceilf(radius) + 1;
    qx[qt] = L->cx; qy[qt] = L->cy; qt++; stamp[L->cy][L->cx] = gen; dist[L->cy][L->cx] = 0;
    while (qh < qt) {
        int x = qx[qh], y = qy[qh]; qh++;
        if (dist[y][x] >= maxd) continue;
        for (int d = 0; d < 4; d++) {
            int nx = x + DX[d], ny = y + DY[d];
            if (nx < 0 || ny < 0 || nx >= MAPN || ny >= MAPN || stamp[ny][nx] == gen) continue;
            stamp[ny][nx] = gen; dist[ny][nx] = dist[y][x] + 1;
            if (G.map[ny][nx].kind != K_WALL) { qx[qt] = nx; qy[qt] = ny; qt++; }
            else { qx[qt] = nx; qy[qt] = ny; qt++; dist[ny][nx] = maxd; }   /* lit face, no further */
        }
    }
    for (int i = 0; i < qt; i++) {
        int x = qx[i], y = qy[i];
        for (int ty = 0; ty < LMR; ty++)
            for (int tx = 0; tx < LMR; tx++) {
                float wx = x + (tx + 0.5f) / LMR, wy = y + (ty + 0.5f) / LMR;
                float dd = sqrtf((wx - lx) * (wx - lx) + (wy - ly) * (wy - ly));
                if (dd >= radius || G.nlc >= MAXLC) continue;
                float w = power / (1.0f + 0.9f * dd * dd) * (1.0f - smoothstepf(radius * 0.55f, radius, dd));
                G.lc_idx[G.nlc] = (uint16_t)((y * LMR + ty) * LMN + x * LMR + tx);
                G.lc_w[G.nlc] = w;
                G.nlc++; L->count++;
            }
    }
}

static int in_room(const Room *R, int x, int y) { return x >= R->x && y >= R->y && x < R->x + R->w && y < R->y + R->h; }

static void dress_level(void);
/* ---- the plan ----
 * A level is laid out first as NB x NB blocks of 3x3 cells with a one-cell wall between
 * neighbouring blocks: on the plan (MG x MG) odd rows and columns are blocks, even ones the walls
 * between them. Rooms span several blocks; the halls between them are a maze of blocks. The plan is
 * then drawn into the cell map, where a stretch of hall is 1, 2 or 3 cells wide but always keeps
 * its middle lane, and the autopilot walks those lanes: every cell with x or y = 2 (mod 4). */
#define NB 11
#define MG (2 * NB + 1)
typedef struct { uint8_t kind, theme, w, lo, trunk; int8_t room; int16_t region; uint8_t keep; } MCell;   /* keep: not filled as a dead end */
static MCell MM[MG][MG];
static int mroom[12][4];                                         /* the rooms on the plan: x, y, w, h */
static int mstart(int m) { return (m & 1) ? 2 * m - 1 : 2 * m; }  /* the first cell of plan row/column m */
static MCell *mc(int x, int y) {
    static MCell solid = { K_WALL, 0, 0, 0, 0, -1, -1, 0 };
    return (x < 0 || y < 0 || x >= MG || y >= MG) ? &solid : &MM[y][x];
}
static int mopen(int x, int y) { int k = mc(x, y)->kind; return k == K_FLOOR || k == K_DOOR; }
static int mdeg(int x, int y) { int n = 0; for (int d = 0; d < 4; d++) n += mopen(x + DX[d], y + DY[d]); return n; }
static void mcarve(int x, int y, int region, int theme, int room) {
    MCell *c = &MM[y][x];
    c->kind = K_FLOOR; c->region = (int16_t)region; c->theme = (uint8_t)theme; c->room = (int8_t)room;
}
static int in_mroom(int r, int x, int y) { return x >= mroom[r][0] && y >= mroom[r][1] && x < mroom[r][0] + mroom[r][2] && y < mroom[r][1] + mroom[r][3]; }
static int lane(int x, int y) { return (x & 3) == 2 || (y & 3) == 2; }
static int lane_dist(int x, int y) { int a = abs((x & 3) - 2), b = abs((y & 3) - 2); return a < b ? a : b; }
/* where the autopilot may walk: the middle lanes */
static int apass(int x, int y) { return lane(x, y) && passable(x, y); }

static int uf[1024];
static int uf_find(int a) { while (uf[a] != a) a = uf[a] = uf[uf[a]]; return a; }

static void mz_grow(int sx, int sy, int region) {
    static int st[MG * MG][2];
    int sp = 0, last = -1;
    mcarve(sx, sy, region, TH_CORRIDOR, -1);
    st[sp][0] = sx; st[sp][1] = sy; sp++;
    while (sp) {
        int x = st[sp - 1][0], y = st[sp - 1][1], avail[4], na = 0;
        for (int d = 0; d < 4; d++) {
            int nx = x + DX[d] * 2, ny = y + DY[d] * 2;
            if (nx >= 1 && ny >= 1 && nx < MG - 1 && ny < MG - 1 && MM[ny][nx].kind == K_WALL) avail[na++] = d;
        }
        if (!na) { sp--; last = -1; continue; }
        int d = avail[rnd(na)];
        for (int i = 0; i < na; i++) if (avail[i] == last && rndf() < 0.5f) d = last;   /* long straight runs */
        mcarve(x + DX[d], y + DY[d], region, TH_CORRIDOR, -1);
        mcarve(x + DX[d] * 2, y + DY[d] * 2, region, TH_CORRIDOR, -1);
        st[sp][0] = x + DX[d] * 2; st[sp][1] = y + DY[d] * 2; sp++;
        last = d;
    }
}

static void mz_fill_dead_ends(void) {                            /* so the walk rarely has to turn back */
    for (int changed = 1; changed; ) {
        changed = 0;
        for (int y = 1; y < MG - 1; y++)
            for (int x = 1; x < MG - 1; x++) {
                if (!mopen(x, y) || MM[y][x].room >= 0 || MM[y][x].keep || mdeg(x, y) > 1) continue;
                MM[y][x].kind = K_WALL; MM[y][x].region = -1; changed = 1;
            }
    }
}

/* Make one room a vault with a single way in. Its other entrances are walled up, but only if the
 * rest of the level stays connected without the room. */
static int mz_vault(int *kx, int *ky, int *room) {
    int order[12], no = 0;
    for (int r = 1; r < G.nrooms; r++) {
        int taken = 0;
        for (int k = 0; k < G.nvaults; k++) taken |= G.vault_room[k] == r;
        if (!taken) order[no++] = r;
    }
    for (int i = no - 1; i > 0; i--) { int j = rnd(i + 1), t = order[i]; order[i] = order[j]; order[j] = t; }
    int sx = mroom[0][0], sy = mroom[0][1];
    for (int oi = 0; oi < no; oi++) {
        int r = order[oi], *R = mroom[r];
        if (R[2] * R[3] > 15) continue;                            /* the big halls stay open */
        int ex[24], ey[24], ed[24], ne = 0;
        for (int y = R[1]; y < R[1] + R[3]; y++)
            for (int x = R[0]; x < R[0] + R[2]; x++)
                for (int d = 0; d < 4; d++) {
                    int nx = x + DX[d], ny = y + DY[d];
                    if (in_mroom(r, nx, ny) || !mopen(nx, ny) || ne >= 24) continue;
                    ex[ne] = nx; ey[ne] = ny; ed[ne] = d; ne++;
                }
        if (!ne || ne > 8) continue;
        int keep = -1;
        for (int i = 0; i < ne; i++) {
            if (!mopen(ex[i] + DX[ed[i]], ey[i] + DY[ed[i]])) continue;
            if (keep < 0 || MM[ey[i]][ex[i]].kind == K_DOOR) keep = i;
        }
        if (keep < 0) continue;
        uint8_t saved[24];
        for (int i = 0; i < ne; i++) { saved[i] = MM[ey[i]][ex[i]].kind; if (i != keep) MM[ey[i]][ex[i]].kind = K_WALL; }
        static uint8_t seen[MG][MG]; static int qx[MG * MG], qy[MG * MG];   /* the rest must still be reachable */
        memset(seen, 0, sizeof seen);
        int qh = 0, qt = 0, total = 0;
        for (int y = 0; y < MG; y++)
            for (int x = 0; x < MG; x++)
                if (mopen(x, y) && !in_mroom(r, x, y) && !(x == ex[keep] && y == ey[keep])) total++;
        seen[sy][sx] = 1; qx[qt] = sx; qy[qt] = sy; qt++;
        while (qh < qt) {
            int x = qx[qh], y = qy[qh]; qh++;
            for (int d = 0; d < 4; d++) {
                int nx = x + DX[d], ny = y + DY[d];
                if (seen[ny][nx] || !mopen(nx, ny) || in_mroom(r, nx, ny) || (nx == ex[keep] && ny == ey[keep])) continue;
                seen[ny][nx] = 1; qx[qt] = nx; qy[qt] = ny; qt++;
            }
        }
        if (qt == total) {
            for (int i = 0; i < ne; i++) if (i != keep) MM[ey[i]][ex[i]].region = -1;
            MM[ey[keep]][ex[keep]].kind = K_DOOR;
            *room = r; *kx = ex[keep]; *ky = ey[keep];
            return 1;
        }
        for (int i = 0; i < ne; i++) MM[ey[i]][ex[i]].kind = saved[i];
    }
    return 0;
}

/* how wide each stretch of hall is: the trunk (from the first room to the farthest) three cells,
 * the rest one, two or three, a whole stretch between junctions alike; junctions are open blocks */
static void mz_widths(void) {
    static int dist[MG][MG], from[MG][MG], qx[MG * MG], qy[MG * MG];
    for (int y = 0; y < MG; y++) for (int x = 0; x < MG; x++) dist[y][x] = -1;
    int sx = mroom[0][0], sy = mroom[0][1], qh = 0, qt = 0, far = -1, fd = -1;
    dist[sy][sx] = 0; qx[qt] = sx; qy[qt] = sy; qt++;
    while (qh < qt) {
        int x = qx[qh], y = qy[qh]; qh++;
        if (MM[y][x].room > 0 && dist[y][x] > fd) { fd = dist[y][x]; far = y * MG + x; }
        for (int d = 0; d < 4; d++) {
            int nx = x + DX[d], ny = y + DY[d];
            if (!mopen(nx, ny) || dist[ny][nx] >= 0) continue;
            dist[ny][nx] = dist[y][x] + 1; from[ny][nx] = d; qx[qt] = nx; qy[qt] = ny; qt++;
        }
    }
    for (int p = far; p >= 0 && dist[p / MG][p % MG] > 0; ) {
        int x = p % MG, y = p / MG, d = from[y][x];
        MM[y][x].trunk = 1;
        p = (y - DY[d]) * MG + (x - DX[d]);
    }
    #define BI(x, y) (((y) / 2) * NB + (x) / 2)
    #define HALL(x, y) (mopen(x, y) && mc(x, y)->room < 0)
    for (int i = 0; i < NB * NB; i++) uf[i] = i;
    for (int y = 1; y < MG; y += 2)
        for (int x = 1; x < MG; x += 2) {
            if (!HALL(x, y) || mdeg(x, y) != 2) continue;
            for (int d = 0; d < 2; d++) {                          /* east and south: each pair once */
                int nx = x + 2 * DX[d], ny = y + 2 * DY[d];
                if (mopen(x + DX[d], y + DY[d]) && mc(x + DX[d], y + DY[d])->kind == K_FLOOR && HALL(nx, ny) && mdeg(nx, ny) == 2)
                    uf[uf_find(BI(x, y))] = uf_find(BI(nx, ny));
            }
        }
    static int rw[NB * NB], rlo[NB * NB], rtrunk[NB * NB];
    for (int i = 0; i < NB * NB; i++) { rw[i] = -1; rtrunk[i] = 0; }
    for (int y = 1; y < MG; y += 2)
        for (int x = 1; x < MG; x += 2)
            if (HALL(x, y) && (MM[y][x].trunk || MM[y][x + 1].trunk || MM[y + 1][x].trunk || MM[y][x - 1].trunk || MM[y - 1][x].trunk)) rtrunk[uf_find(BI(x, y))] = 1;
    for (int y = 1; y < MG; y += 2)
        for (int x = 1; x < MG; x += 2) {
            if (!HALL(x, y)) continue;
            MCell *m = &MM[y][x];
            if (mdeg(x, y) != 2) { m->w = 3; m->lo = 0; continue; }  /* a junction: open */
            int r = uf_find(BI(x, y));
            if (rw[r] < 0) {
                int k = rnd(20);
                rw[r] = rtrunk[r] ? 3 : (k < 4 ? 1 : (k < 12 ? 2 : 3));
                rlo[r] = rw[r] == 2 ? rnd(2) : (rw[r] == 1 ? 1 : 0);
            }
            m->w = (uint8_t)rw[r]; m->lo = (uint8_t)rlo[r];
        }
    #undef BI
    #undef HALL
}

/* the lanes of hall block (mx,my) on its way out in direction d: lo, and how many */
static void mz_arm(int mx, int my, int d, int *lo, int *w) {
    const MCell *conn = mc(mx + DX[d], my + DY[d]), *nb = mc(mx + 2 * DX[d], my + 2 * DY[d]), *self = mc(mx, my), *src = self;
    if (conn->kind == K_DOOR) { *lo = 1; *w = 1; return; }
    if (self->room >= 0) src = nb;                                 /* a room's archway takes the hall's width */
    else if (nb->room < 0 && mdeg(mx, my) != 2) src = nb;          /* a junction meets each hall at its own width */
    if (src->room >= 0) { *lo = 0; *w = 3; return; }
    *lo = src->lo; *w = src->w;
}

/* the plan, drawn into the cell map */
static void mz_draw(void) {
    for (int my = 0; my < MG; my++)
        for (int mx = 0; mx < MG; mx++) {
            const MCell *m = &MM[my][mx];
            if (!mopen(mx, my)) continue;
            int fx = mstart(mx), fy = mstart(my), bw = (mx & 1) ? 3 : 1, bh = (my & 1) ? 3 : 1;
            if (m->room >= 0) {                                    /* a room: all of it, walls between its blocks too */
                for (int y = fy; y < fy + bh; y++) for (int x = fx; x < fx + bw; x++) carve(x, y, m->region, m->theme);
                continue;
            }
            if ((mx & 1) && (my & 1)) {                            /* a hall block: its middle, and an arm each way out */
                carve(fx + 1, fy + 1, m->region, TH_CORRIDOR);
                for (int d = 0; d < 4; d++) {
                    if (!mopen(mx + DX[d], my + DY[d])) continue;
                    int lo, w;
                    mz_arm(mx, my, d, &lo, &w);
                    for (int l = lo; l < lo + w; l++)
                        for (int k = 0; k < 2; k++) {
                            int lx = DX[d] ? 1 + DX[d] * k : l, ly = DX[d] ? l : 1 + DY[d] * k;
                            carve(fx + lx, fy + ly, m->region, TH_CORRIDOR);
                        }
                }
            } else if ((mx & 1) != (my & 1)) {                     /* the wall between two blocks, opened */
                int horiz = !(mx & 1), d = horiz ? 0 : 1, ax = horiz ? mx - 1 : mx, ay = horiz ? my : my - 1;
                if (m->kind == K_DOOR) {
                    int x = horiz ? fx : fx + 1, y = horiz ? fy + 1 : fy;
                    carve(x, y, m->region, TH_CORRIDOR); G.map[y][x].kind = K_DOOR;
                    continue;
                }
                const MCell *A = mc(ax, ay), *B = mc(ax + 2 * DX[d], ay + 2 * DY[d]);
                int lo = 0, w = 3;
                if (A->room < 0) mz_arm(ax, ay, d, &lo, &w);
                else if (B->room < 0) mz_arm(ax + 2 * DX[d], ay + 2 * DY[d], d + 2, &lo, &w);
                for (int l = lo; l < lo + w; l++) carve(horiz ? fx : fx + l, horiz ? fy + l : fy, m->region, TH_CORRIDOR);
            }
        }
}

/* Secrets (docs/PLAN-b6.md): a set's levels hold 3, 4, 4, 5 and 6 of them, every link of a chain
 * counted, split into chains that grow longer deeper into the set (the first always two or more, so
 * every level has a way into a pocket). Each chain begins in a room behind a cracked wall beside a hall,
 * made of blocks the plan left solid: one, two side by side, or two by two where there is the room;
 * the wall slides right through it and comes to rest at the back. A chain of two or more has a doorway
 * in the back wall, beside where the stone stops. */
static int imin(int a, int b) { return a < b ? a : b; }
static int imax(int a, int b) { return a > b ? a : b; }
static int secret_block_free(int mx, int my) { return mx >= 1 && my >= 1 && mx < MG - 1 && my < MG - 1 && MM[my][mx].kind == K_WALL && !MM[my][mx].trunk; }
/* a doorway into a room's pockets: in its back wall if it can, else a side wall, beside where the stone
 * stops and never behind it, with solid rock beyond (kept so) for the doorway not to show elsewhere */
static int secret_door(Secret *sc) {
    int rx = sc->sx + DX[sc->d] * sc->len, ry = sc->sy + DY[sc->d] * sc->len, best = -1; float bs = -1e9f;
    for (int y = sc->y0 - 1; y <= sc->y1 + 1; y++)
        for (int x = sc->x0 - 1; x <= sc->x1 + 1; x++) {
            int inside = x >= sc->x0 && x <= sc->x1 && y >= sc->y0 && y <= sc->y1;
            if (inside || x < 1 || y < 1 || x >= MAPN - 1 || y >= MAPN - 1) continue;
            for (int n = 0; n < 4; n++) {                          /* n: outwards from the room */
                int ix = x - DX[n], iy = y - DY[n], bx = x + DX[n], by = y + DY[n];
                if (ix < sc->x0 || ix > sc->x1 || iy < sc->y0 || iy > sc->y1 || (ix == rx && iy == ry)) continue;
                if (n == ((sc->d + 2) & 3) || G.map[y][x].kind != K_WALL || G.map[y][x].secret) continue;   /* not the front */
                if (bx < 1 || by < 1 || bx >= MAPN - 1 || by >= MAPN - 1 || G.map[by][bx].kind != K_WALL || G.map[by][bx].secret) continue;
                int along = (ix - sc->sx) * DX[sc->d] + (iy - sc->sy) * DY[sc->d];
                float sc0 = (n == sc->d ? 20.0f : 0.0f) + (float)along + rndf() * 2.0f;
                if (sc0 > bs) { bs = sc0; best = (y * MAPN + x) * 4 + n; }
            }
        }
    if (best < 0) return 0;
    int n = best % 4, x = best / 4 % MAPN, y = best / 4 / MAPN, bx = x + DX[n], by = y + DY[n];
    if ((bx & 3) && (by & 3)) MM[(by - 1) / 4 * 2 + 1][(bx - 1) / 4 * 2 + 1].trunk = 1;
    sc->door_x = x; sc->door_y = y;
    G.map[y][x].secret = 3;
    return 1;
}
static void place_secrets(void) {
    static const int LINKS[RUN_LEN] = { 3, 3, 4, 4, 4, 5, 5, 5, 6, 6 };
    G.nsecrets = G.secrets_found = G.nlinks = G.links_found = 0; G.slide = -1;
    int step = crawl_step(G.level), left = LINKS[step], chain[MAXSECRETS], nch = 0;
    float sk = (float)step / (float)(RUN_LEN - 1), longer = 0.3f + 0.4f * sk;   /* longer chains deeper into the run */
    while (left > 0 && nch < MAXSECRETS) {
        int len = 1;
        while (len < left && rndf() < longer) len++;
        if (!nch && len < 2 && left >= 2) len = 2;
        chain[nch++] = len; left -= len;
    }
    left = 0;                                                      /* now: links that found no room, to go to a chain that has one */
    for (int i = 1; i < nch; i++) { int v = chain[i], j = i - 1; while (j >= 0 && chain[j] < v) { chain[j + 1] = chain[j]; j--; } chain[j + 1] = v; }   /* the longest first */
    static int cand[MG * MG * 4];
    int nc = 0;
    for (int my = 1; my < MG; my += 2)
        for (int mx = 1; mx < MG; mx += 2) {
            if (mopen(mx, my) || MM[my][mx].kind != K_WALL) continue;   /* (K_PILLAR on the plan: the arena's) */
            for (int d = 0; d < 4; d++) {                          /* d: from the room towards where it is pressed */
                int cx = mx + DX[d], cy = my + DY[d], nx = mx + 2 * DX[d], ny = my + 2 * DY[d];
                if (nx < 1 || ny < 1 || nx >= MG - 1 || ny >= MG - 1 || mopen(cx, cy) || !mopen(nx, ny)) continue;
                int sx = mstart(cx) + (cx & 1), sy = mstart(cy) + (cy & 1), fx = sx + DX[d], fy = sy + DY[d];
                if (G.map[fy][fx].kind != K_FLOOR || !lane(fx, fy)) continue;
                int doorish = 0;
                for (int e = 0; e < 4; e++) doorish |= cell(fx + DX[e], fy + DY[e])->kind == K_DOOR;
                if (doorish) continue;
                cand[nc++] = (my * MG + mx) * 4 + d;
            }
        }
    for (int i = nc - 1; i > 0; i--) { int j = rnd(i + 1), t = cand[i]; cand[i] = cand[j]; cand[j] = t; }
    int placed = 0;
    for (int i = 0; i < nc && placed < nch; i++) {
        int b = cand[i] / 4, d = cand[i] % 4, mx = b % MG, my = b / MG, in = (d + 2) & 3;
        int cx = mx + DX[d], cy = my + DY[d], sx = mstart(cx) + (cx & 1), sy = mstart(cy) + (cy & 1);
        int clash = MM[my][mx].trunk;                              /* trunk marks a room already taken */
        for (int k = 0; k < G.nsecrets; k++) clash |= abs(G.secrets[k].sx - sx) + abs(G.secrets[k].sy - sy) < 8;
        if (clash) continue;
        /* how big: two by two, two side by side, or the one block, as the plan allows */
        int px = DY[in], py = DX[in], side = rnd(2) ? 1 : -1, bx[4] = { mx }, by[4] = { my }, nb = 1;
        float r = rndf();
        if (r < 0.15f + 0.4f * sk) {
            for (int sd = 0; sd < 2 && nb == 1; sd++, side = -side) {
                int ax = mx + 2 * px * side, ay = my + 2 * py * side, dx2 = mx + 2 * DX[in], dy2 = my + 2 * DY[in];
                if (secret_block_free(ax, ay) && secret_block_free(dx2, dy2) && secret_block_free(ax + 2 * DX[in], ay + 2 * DY[in])) {
                    bx[1] = ax; by[1] = ay; bx[2] = dx2; by[2] = dy2; bx[3] = ax + 2 * DX[in]; by[3] = ay + 2 * DY[in]; nb = 4;
                }
            }
        }
        if (nb == 1 && r < 0.65f)
            for (int sd = 0; sd < 2 && nb == 1; sd++, side = -side)
                if (secret_block_free(mx + 2 * px * side, my + 2 * py * side)) { bx[1] = mx + 2 * px * side; by[1] = my + 2 * py * side; nb = 2; }
        int x0 = MAPN, y0 = MAPN, x1 = -1, y1 = -1;
        for (int k = 0; k < nb; k++) {
            MM[by[k]][bx[k]].trunk = 1;
            x0 = imin(x0, mstart(bx[k])); y0 = imin(y0, mstart(by[k])); x1 = imax(x1, mstart(bx[k]) + 2); y1 = imax(y1, mstart(by[k]) + 2);
        }
        for (int y = y0; y <= y1; y++)
            for (int x = x0; x <= x1; x++) { carve(x, y, -2, TH_CORRIDOR); G.map[y][x].secret = 1; }
        G.map[sy][sx].secret = 2;
        Secret *sc = &G.secrets[G.nsecrets++];
        *sc = (Secret){ sx + DX[d], sy + DY[d], in, sx, sy, 0, 0, 0, nb == 4 ? 7 : 3, chain[placed++], -1, -1, x0, y0, x1, y1, rng_next(&G.rs) | 1u };
        if (sc->links >= 2 && !secret_door(sc)) { left += sc->links - 1; sc->links = 1; }   /* nowhere for a doorway: alone after all */
    }
    for (int k = placed; k < nch; k++) left += chain[k];
    for (int k = 0; k < G.nsecrets && left > 0; k++)               /* what found no room goes deeper into a chain */
        if (G.secrets[k].door_x >= 0) { G.secrets[k].links += left; left = 0; }
    for (int k = 0; k < G.nsecrets && left > 0; k++)               /* or makes one of a room alone */
        if (secret_door(&G.secrets[k])) { G.secrets[k].links += left; left = 0; }
    for (int k = 0; k < G.nsecrets; k++) G.nlinks += G.secrets[k].links;
#ifdef CRAWL_DEBUG
    fprintf(stderr, "SECRETS level %d: %d links in %d rooms:", G.level, G.nlinks, G.nsecrets);
    for (int k = 0; k < G.nsecrets; k++)
        fprintf(stderr, " [%dx%d chain %d door (%d,%d)]", G.secrets[k].x1 - G.secrets[k].x0 + 1, G.secrets[k].y1 - G.secrets[k].y0 + 1, G.secrets[k].links, G.secrets[k].door_x, G.secrets[k].door_y);
    fprintf(stderr, "\n");
#endif
}

/* the sliding wall: a block of stone at its cell plus how far it has come */
static void slide_box(float *bx, float *by) {
    const Secret *sc = &G.secrets[G.slide];
    *bx = sc->sx + DX[sc->d] * G.slide_t; *by = sc->sy + DY[sc->d] * G.slide_t;
}
static int slide_covers(int x, int y) {
    float bx, by;
    slide_box(&bx, &by);
    return bx < x + 1 - 0.02f && bx + 1 > x + 0.02f && by < y + 1 - 0.02f && by + 1 > y + 0.02f;
}

#define ARENA_PLAN 15                       /* plan rows from here on are a boss's (cells from BAND_Y0) */
#define ARENA_DOOR_MX 3                     /* and the way down to it: plan column 3, cell x 6 */
static void stamp_arena(void);
static int hz_patch(int sx, int sy, int reach, int8_t *cx, int8_t *cy, uint8_t *cd, int max);
static void swap_rooms(int a, int b) {                             /* rooms a and b change places in the lists */
    for (int y = 0; y < MG; y++)
        for (int x = 0; x < MG; x++) {
            if (MM[y][x].room == a) MM[y][x].room = (int8_t)b;
            else if (MM[y][x].room == b) MM[y][x].room = (int8_t)a;
        }
    for (int k = 0; k < 4; k++) { int t = mroom[a][k]; mroom[a][k] = mroom[b][k]; mroom[b][k] = t; }
    Room t = G.rooms[a]; G.rooms[a] = G.rooms[b]; G.rooms[b] = t;
}

static void gen_level(void) {
    memset(G.map, 0, sizeof G.map);
    for (int y = 0; y < MAPN; y++)
        for (int x = 0; x < MAPN; x++) { G.map[y][x].kind = K_WALL; G.map[y][x].region = -1; G.map[y][x].door = -1; }
    memset(MM, 0, sizeof MM);
    for (int y = 0; y < MG; y++) for (int x = 0; x < MG; x++) { MM[y][x].kind = K_WALL; MM[y][x].region = -1; MM[y][x].room = -1; }
    if (G.boss >= 0)                                               /* the boss's strip of the plan: kept out of the rest */
        for (int y = ARENA_PLAN; y < MG; y++) for (int x = 0; x < MG; x++) MM[y][x].kind = K_PILLAR;
    G.nrooms = 0; G.ndoors = 0; G.nlights = 0; G.nlc = 0; G.ndecos = 0;
    memset(G.bulky, 0, sizeof G.bulky);
    int nreg = 0;

    /* rooms: chambers of one to four blocks a side, a few of them big halls */
    static const int SPAN[10] = { 1, 1, 3, 3, 3, 3, 5, 5, 5, 7 };
    for (int tries = 0; tries < 400 && G.nrooms < 9; tries++) {
        int w = SPAN[rnd(10)], h = SPAN[rnd(10)];
        if (w * h > 35 || (w == 1 && h == 1 && rnd(2))) continue;
        int x = 1 + 2 * rnd((MG - w - 1) / 2 + 1), y = 1 + 2 * rnd((MG - h - 1) / 2 + 1);
        if (x + w >= MG - 1 || y + h >= MG - 1) continue;
        int ok = 1;
        for (int yy = y - 1; yy <= y + h && ok; yy++)
            for (int xx = x - 1; xx <= x + w; xx++) if (MM[yy][xx].kind != K_WALL) { ok = 0; break; }
        if (!ok) continue;
        int theme = 1 + rnd(TH_COUNT - 1);
        for (int yy = y; yy < y + h; yy++) for (int xx = x; xx < x + w; xx++) mcarve(xx, yy, nreg, theme, G.nrooms);
        mroom[G.nrooms][0] = x; mroom[G.nrooms][1] = y; mroom[G.nrooms][2] = w; mroom[G.nrooms][3] = h;
        G.rooms[G.nrooms++] = (Room){ 0, 0, 0, 0, nreg, theme };
        nreg++;
    }
    if (G.boss >= 0) {                                             /* the start as far from the arena as the rooms allow */
        int far = 0, fd = -1;
        for (int r = 0; r < G.nrooms; r++) {
            int d = abs(mroom[r][0] + mroom[r][2] / 2 - ARENA_DOOR_MX) + abs(mroom[r][1] + mroom[r][3] / 2 - (ARENA_PLAN - 2));
            if (d > fd) { fd = d; far = r; }
        }
        if (far) swap_rooms(0, far);
    }
    for (int y = 1; y < MG - 1; y += 2)
        for (int x = 1; x < MG - 1; x += 2)
            if (MM[y][x].kind == K_WALL && nreg < 1000) mz_grow(x, y, nreg++);
    if (G.boss >= 0) {                                             /* and a way down into its antechamber, kept open */
        MCell *a = &MM[ARENA_PLAN - 2][ARENA_DOOR_MX], *d = &MM[ARENA_PLAN - 1][ARENA_DOOR_MX];
        d->kind = K_DOOR; d->region = a->region; d->theme = TH_CORRIDOR; d->room = -1; d->keep = a->keep = 1;
    }

    /* connect every region, mostly as a tree, with a few extra loops; doors into most rooms */
    static int con[MG * MG][4];
    int nc = 0;
    for (int y = 1; y < MG - 1; y++)
        for (int x = 1; x < MG - 1; x++) {
            if (MM[y][x].kind != K_WALL || !((x ^ y) & 1)) continue;   /* only a wall between two blocks */
            int a = -1, b = -1;
            if (mopen(x - 1, y) && mopen(x + 1, y)) { a = MM[y][x - 1].region; b = MM[y][x + 1].region; }
            else if (mopen(x, y - 1) && mopen(x, y + 1)) { a = MM[y - 1][x].region; b = MM[y + 1][x].region; }
            if (a >= 0 && b >= 0 && a != b) { con[nc][0] = x; con[nc][1] = y; con[nc][2] = a; con[nc][3] = b; nc++; }
        }
    for (int i = nc - 1; i > 0; i--) {
        int j = rnd(i + 1);
        for (int k = 0; k < 4; k++) { int t = con[i][k]; con[i][k] = con[j][k]; con[j][k] = t; }
    }
    for (int i = 0; i < nreg; i++) uf[i] = i;
    for (int pass = 0; pass < 2; pass++)
        for (int i = 0; i < nc; i++) {
            int x = con[i][0], y = con[i][1], a = con[i][2], b = con[i][3];
            if (MM[y][x].kind != K_WALL) continue;
            int ra = uf_find(a), rb = uf_find(b);
            int extra = pass == 1 && rnd(100) < 8;
            if (ra == rb && !extra) continue;
            if (extra) {
                int near = 0;
                for (int d = 0; d < 4; d++) if (mc(x + DX[d] * 2, y + DY[d] * 2)->kind == K_DOOR) near = 1;
                if (near) continue;
            }
            uf[ra] = rb;
            int room_side = mc(x - 1, y)->room >= 0 || mc(x + 1, y)->room >= 0 || mc(x, y - 1)->room >= 0 || mc(x, y + 1)->room >= 0;
            int hall_side = (mopen(x - 1, y) && mc(x - 1, y)->room < 0) || (mopen(x + 1, y) && mc(x + 1, y)->room < 0)
                         || (mopen(x, y - 1) && mc(x, y - 1)->room < 0) || (mopen(x, y + 1) && mc(x, y + 1)->room < 0);
            mcarve(x, y, room_side && !hall_side ? a : (mc(x - 1, y)->room < 0 && mopen(x - 1, y) ? MM[y][x - 1].region
                        : (mc(x, y - 1)->room < 0 && mopen(x, y - 1) ? MM[y - 1][x].region : b)), TH_CORRIDOR, -1);
            if (room_side && rnd(10) < 7) MM[y][x].kind = K_DOOR;
        }
    mz_fill_dead_ends();

    /* the locked rooms: gold always, silver from level 3, crimson from level 5 */
    int want = G.level >= 5 ? 3 : (G.level >= 3 ? 2 : 1), vkx[NKEYS], vky[NKEYS];
    G.nvaults = 0;
    for (int k = 0; k < NKEYS; k++) G.vault_room[k] = G.vault_door[k] = -1;
    for (int k = 0; k < want; k++) {
        int room, px, py;
        if (!mz_vault(&px, &py, &room)) break;
        G.vault_room[k] = room; G.nvaults++;
        vkx[k] = mstart(px) + (px & 1); vky[k] = mstart(py) + (py & 1);
        mz_fill_dead_ends();
    }
    mz_widths();
    mz_draw();
    for (int r = 0; r < G.nrooms; r++) {                          /* the rooms, in cells */
        Room *R = &G.rooms[r];
        R->x = mstart(mroom[r][0]); R->y = mstart(mroom[r][1]);
        R->w = mstart(mroom[r][0] + mroom[r][2] - 1) + 3 - R->x; R->h = mstart(mroom[r][1] + mroom[r][3] - 1) + 3 - R->y;
    }

    /* doors: only where they sit between two walls, never next to each other */
    for (int y = 1; y < MAPN - 1; y++)
        for (int x = 1; x < MAPN - 1; x++) {
            Cell *c = &G.map[y][x];
            if (c->kind != K_DOOR) continue;
            int ax = is_open(x - 1, y) && is_open(x + 1, y) && !is_open(x, y - 1) && !is_open(x, y + 1);
            int ay = is_open(x, y - 1) && is_open(x, y + 1) && !is_open(x - 1, y) && !is_open(x + 1, y);
            int adj = 0, vault = -1;
            for (int k = 0; k < G.nvaults; k++) if (x == vkx[k] && y == vky[k]) vault = k;
            for (int d = 0; d < 4; d++) if (cell(x + DX[d], y + DY[d])->kind == K_DOOR) adj = 1;
            if (vault < 0 && ((!ax && !ay) || adj || G.ndoors >= MAXDOORS - 1)) { c->kind = K_FLOOR; continue; }
            c->door = (int16_t)G.ndoors;
            G.doors[G.ndoors++] = (Door){ x, y, ax, 0.0f, 0.0f, 0, vault >= 0, vault >= 0 ? vault : 0 };
            if (vault >= 0) G.vault_door[vault] = G.ndoors - 1;
        }

    place_secrets();

    /* wall textures from the neighbouring room's theme, or the corridor zone */
    for (int y = 0; y < MAPN; y++)
        for (int x = 0; x < MAPN; x++) {
            Cell *c = &G.map[y][x];
            if (c->kind != K_WALL) continue;
            int theme = -1;
            for (int d = 0; d < 4; d++) {
                Cell *n = cell(x + DX[d], y + DY[d]);
                if (n->kind == K_FLOOR && n->theme != TH_CORRIDOR) theme = n->theme;
            }
            if (theme < 0) {
                uint32_t z = hash2(x / 9 + 101, y / 9 + 57) % 10;
                theme = z < 6 ? TH_CORRIDOR : (z < 8 ? TH_MOSS : TH_OSSUARY);
            }
            c->tex = (uint8_t)theme_tex(theme, hash2(x, y));
        }
    for (int i = 0; i < G.nsecrets; i++) {                       /* a plain stone for the cracks to show in */
        Secret *sc = &G.secrets[i];
        Cell *w = &G.map[sc->sy][sc->sx];
        if (w->tex == T_NICHE || w->tex == T_BANNER || w->tex == T_SKULLS) w->tex = T_STONE;
        sc->tex = w->tex;
        if (sc->door_x >= 0) G.map[sc->door_y][sc->door_x].tex = T_PASSAGE;
    }

    /* colonnades down some of the big rooms: a pillar where four of their blocks meet */
    for (int r = 0; r < G.nrooms; r++) {
        if (mroom[r][2] < 3 || mroom[r][3] < 3 || rnd(10) >= 4) continue;
        for (int my = mroom[r][1] + 1; my < mroom[r][1] + mroom[r][3] - 1; my += 2)
            for (int mx = mroom[r][0] + 1; mx < mroom[r][0] + mroom[r][2] - 1; mx += 2) {
                int x = mstart(mx), y = mstart(my);
                G.map[y][x].kind = K_PILLAR;
                add_deco(x + 0.5f, y + 0.5f, SP_PILLAR);
            }
    }

    /* torches along the halls */
    static int order[MAPN * MAPN];
    int no = 0;
    for (int i = 0; i < MAPN * MAPN; i++) order[no++] = i;
    for (int i = no - 1; i > 0; i--) { int j = rnd(i + 1), t = order[i]; order[i] = order[j]; order[j] = t; }
    for (int i = 0; i < no && G.nlights < MAXLIGHTS - 40; i++) {
        int x = order[i] % MAPN, y = order[i] / MAPN;
        if (G.map[y][x].kind != K_FLOOR || G.map[y][x].secret) continue;
        int near = 0;
        for (int l = 0; l < G.nlights; l++) if (abs(G.lights[l].cx - x) + abs(G.lights[l].cy - y) < 6) near = 1;
        if (near || rndf() > 0.68f - 0.22f * depth()) continue;     /* fewer torches the deeper you go */
        int d = rnd(4);
        for (int k = 0; k < 4; k++, d = (d + 1) & 3) {
            int wx = x + DX[d], wy = y + DY[d];
            Cell *w = cell(wx, wy);
            if (w->kind != K_WALL || w->secret || wx <= 0 || wy <= 0 || wx >= MAPN - 1 || wy >= MAPN - 1) continue;
            int bydoor = 0;
            for (int e = 0; e < 4; e++) if (cell(wx + DX[e], wy + DY[e])->kind == K_DOOR) bydoor = 1;
            if (bydoor) continue;
            w->tex = T_TORCH0;
            light_static(x + 0.5f + DX[d] * 0.42f, y + 0.5f + DY[d] * 0.42f, 1.55f, 4.6f);
            break;
        }
    }

    dress_level();

    G.ncells = 0;                                                  /* what the autopilot can walk */
    for (int y = 0; y < MAPN; y++) for (int x = 0; x < MAPN; x++) if (G.map[y][x].kind == K_FLOOR && lane(x, y) && !G.map[y][x].secret) G.ncells++;

    /* fog banks: smooth blobs from a coarse random lattice; a few wisps at first, thick in the Deep */
    #define FOGL (MAPN * 10 / 48 + 3)
    float lat[FOGL][FOGL];
    for (int j = 0; j < FOGL; j++) for (int i = 0; i < FOGL; i++) lat[j][i] = rndf();
    float dk = clampf(0.45f * decay() + 0.55f * depth(), 0.0f, 1.0f), thr = 0.72f - 0.32f * dk, amt = 0.4f + 0.45f * dk;
    for (int y = 0; y < MAPN; y++)
        for (int x = 0; x < MAPN; x++) {
            float fx = x / 4.8f, fy = y / 4.8f;
            int ix = (int)fx, iy = (int)fy;
            float ax = fx - ix, ay = fy - iy;
            ax = ax * ax * (3 - 2 * ax); ay = ay * ay * (3 - 2 * ay);
            float n = lerpf(lerpf(lat[iy][ix], lat[iy][ix + 1], ax), lerpf(lat[iy + 1][ix], lat[iy + 1][ix + 1], ax), ay);
            G.fog[y][x] = clampf((n - thr) * 3.2f, 0.0f, 1.0f) * amt;
        }
    #undef FOGL
    if (G.boss >= 0) stamp_arena();
}

/* set dressing: bones, webs, chains and roots in the halls, a totem at some crossings; each room a
 * graveyard, a fire hall, a gibbet room or a plain crypt, the big ones under a chandelier */
static int wall_toward(int x, int y) {                             /* a wall beside (x,y): which way, or -1 */
    for (int d = 0, k = rnd(4); d < 4; d++, k = (k + 1) & 3) if (cell(x + DX[k], y + DY[k])->kind == K_WALL) return k;
    return -1;
}
static int by_way(int x, int y) {                                  /* next to a door, or to where a room opens */
    for (int d = 0; d < 4; d++) {
        const Cell *n = cell(x + DX[d], y + DY[d]);
        if (n->kind == K_DOOR || (n->kind == K_FLOOR && n->theme != G.map[y][x].theme)) return 1;
    }
    return 0;
}
static int block_corner(int x, int y) { return ((x & 3) == 1 || (x & 3) == 3) && ((y & 3) == 1 || (y & 3) == 3); }
static void solid_prop(int x, int y, int spr, int nfr, int kind) {
    G.map[y][x].kind = (uint8_t)kind;
    add_deco_ex(x + 0.5f, y + 0.5f, spr, nfr, 0);
}
static void fire_pit(int x, int y) { solid_prop(x, y, SP_FIREPIT_A, 3, K_PROP); light_static(x + 0.5f, y + 0.5f, 1.7f, 5.6f); }
static void totem(int x, int y) { solid_prop(x, y, SP_TOTEM_A, 2, K_PILLAR); light_static(x + 0.5f, y + 0.5f, 1.0f, 3.8f); }
static void chandelier(int x, int y) { add_deco_ex(x + 0.5f, y + 0.5f, SP_CHANDELIER_A, 2, DF_SWAY); light_static(x + 0.5f, y + 0.5f, 1.35f, 6.0f); }
static void candlestand(int x, int y) { solid_prop(x, y, SP_CANDLESTAND_A, 2, K_PROP); light_static(x + 0.5f, y + 0.5f, 0.95f, 3.6f); }
static void lantern(int x, int y) { add_deco_ex(x + 0.5f, y + 0.5f, SP_LANTERN_A, 2, DF_SWAY); light_static(x + 0.5f, y + 0.5f, 1.1f, 5.0f); }
static void brazier(int x, int y) { solid_prop(x, y, SP_BRAZIER_A, 3, K_PROP); light_static(x + 0.5f, y + 0.5f, 1.5f, 5.2f); }
static void minelamp(int x, int y) { solid_prop(x, y, SP_MINELAMP_A, 2, K_PILLAR); light_static(x + 0.5f, y + 0.5f, 0.95f, 4.2f); }
static void crystals(int x, int y) {                               /* a steady glow, not a flame */
    int n = G.nlights;
    solid_prop(x, y, SP_CRYSTAL_A, 2, K_PROP); light_static(x + 0.5f, y + 0.5f, 1.15f, 4.6f);
    if (G.nlights > n) G.lights[n].phase = -1000.0f;
}
static void magma(int x, int y) { solid_prop(x, y, SP_MAGMA_A, 3, K_PROP); light_static(x + 0.5f, y + 0.5f, 1.6f, 5.4f); }

/* The dressing comes in as you go down (PLAN-bestiary, section 8): each set lights and furnishes
 * itself in its own way, and some things show a level or two before what they warn of. In the second
 * time down and after, everything a set has is there from its first level. */
static int since(int level) { return G.level >= level || crawl_loop(G.level) > 0; }

/* a light for a hall crossing, by the set */
static void crossing_light(int x, int y) {
    switch (crawl_set(G.level)) {
    case 0: if (since(3)) totem(x, y); else candlestand(x, y); break;
    case 1: if (rnd(2)) totem(x, y); else lantern(x, y); break;
    case 2: if (rnd(2)) minelamp(x, y); else brazier(x, y); break;
    default: if (rnd(3)) crystals(x, y); else magma(x, y); break;
    }
}
/* a room's own light, in a corner of one of its blocks */
static void room_light(int x, int y, int vault) {
    switch (crawl_set(G.level)) {
    case 0: if (vault || rnd(3) || !since(3)) chandelier(x, y); else totem(x, y); break;
    case 1: if (vault || rnd(3)) chandelier(x, y); else lantern(x, y); break;
    case 2: if (rnd(2)) brazier(x, y); else minelamp(x, y); break;
    default: if (vault || rnd(3)) crystals(x, y); else magma(x, y); break;
    }
}
/* a fire hall's fire */
static void hall_fire(int x, int y) {
    int set = crawl_set(G.level);
    if (set <= 1) fire_pit(x, y); else if (set == 2) brazier(x, y); else magma(x, y);
}

/* something on the floor of a hall, by a wall: mostly bones, now and then a sign of what is coming */
static int hall_litter(void) {
    int set = crawl_set(G.level), k = rnd(100);
    if (set <= 1 && since(3) && k < 6) return SP_BROKEN_ARMS;      /* shield-bearers from 4 */
    if (set >= 1 && set <= 2 && since(7) && k < 12) return SP_COLLAR;   /* hounds from 8 */
    if (set >= 2 && since(11) && k < 18) return SP_CARCASS;         /* ghouls from 12 */
    if (set == 2 && since(12) && k < 23) return SP_HELM;            /* knights from 13 */
    return rnd(4) ? SP_BONES1 : SP_BONES2;
}

static void dress_level(void) {
    float dk = decay();
    int set = crawl_set(G.level);
    /* the halls */
    for (int y = 1; y < MAPN - 1; y++)
        for (int x = 1; x < MAPN - 1; x++) {
            Cell *c = &G.map[y][x];
            if (c->kind != K_FLOOR || c->theme != TH_CORRIDOR || c->secret) continue;
            int o[4];
            for (int d = 0; d < 4; d++) o[d] = is_open(x + DX[d], y + DY[d]);
            int n = o[0] + o[1] + o[2] + o[3];
            float cx = x + 0.5f, cy = y + 0.5f;
            if (n == 2 && !(o[0] && o[2]) && !(o[1] && o[3])) {      /* a corner: cobweb in the dead corner */
                if (rndf() < 0.45f + 0.35f * dk) add_deco(cx + (o[0] ? -0.34f : 0.34f), cy + (o[1] ? -0.34f : 0.34f), SP_COBWEB);
                continue;
            }
            int wd = wall_toward(x, y);
            if (lane(x, y)) {                                      /* on a lane: only things kept to the side */
                if (wd < 0 || n > 2) continue;
                if (rndf() < 0.09f + 0.08f * dk) add_deco(cx + DX[wd] * 0.31f + DY[wd] * (rndf() - 0.5f) * 0.3f, cy + DY[wd] * 0.31f + DX[wd] * (rndf() - 0.5f) * 0.3f, hall_litter());
                else if (rndf() < 0.05f) add_deco(cx + DX[wd] * 0.34f, cy + DY[wd] * 0.34f, SP_CHAINS);
                else if (since(8) && rndf() < 0.025f) add_deco(cx + DX[wd] * 0.3f, cy + DY[wd] * 0.3f, SP_ICICLES);   /* the Wraith has been by */
                continue;
            }
            float k = rndf();                                      /* beside the lane, in the wide halls */
            if (k < 0.08f) add_deco_ex(cx, cy, set <= 1 && since(3) && rnd(3) ? SP_HOOKS : SP_CHAINS, 1, DF_SWAY);
            else if (k < 0.13f + 0.06f * dk) { if (since(6) && set <= 2) add_deco_ex(cx, cy, SP_ROOTS, 1, DF_SWAY); }
            else if (k < 0.17f + 0.06f * dk) { if (since(4)) add_deco_ex(cx, cy, SP_WEBSHEET, 1, DF_SWAY); }
            else if (set == 1 && k < 0.24f + 0.06f * dk) add_deco(cx, cy, rnd(4) ? SP_PUDDLE_A : SP_PUDDLE_B);   /* water underfoot */
            else if (set >= 1 && set <= 2 && wd >= 0 && k < 0.27f + 0.04f * dk) solid_prop(x, y, SP_STALAG, 1, K_PILLAR);   /* flowstone */
            else if (set == 2 && k < 0.31f) add_deco(cx, cy, SP_FUNGUS);
            else if (set == 3 && wd >= 0 && k < 0.31f) solid_prop(x, y, SP_OBSIDIAN, 1, K_PROP);
            else if (wd >= 0 && k < 0.40f + 0.1f * dk) add_deco(cx + DX[wd] * 0.3f, cy + DY[wd] * 0.3f, hall_litter());
        }
    /* a light at some crossings of the wide halls */
    for (int my = 1; my < MG; my += 2)
        for (int mx = 1; mx < MG; mx += 2) {
            if (!mopen(mx, my) || MM[my][mx].room >= 0 || mdeg(mx, my) < 3 || rnd(10) >= 4) continue;
            int x0 = mstart(mx), y0 = mstart(my), k = rnd(4);
            for (int t = 0; t < 4; t++, k = (k + 1) & 3) {
                int x = x0 + (k & 1) * 2, y = y0 + (k >> 1) * 2;
                if (G.map[y][x].kind != K_FLOOR || by_way(x, y)) continue;
                crossing_light(x, y);
                break;
            }
        }
    /* the rooms: a plain crypt, a graveyard, a fire hall or a gibbet room, or what the deeper sets
     * make of them (a cave, a mine, a web room; a crystal cave, an altar room, a field of glass) */
    for (int r = 0; r < G.nrooms; r++) {
        Room *R = &G.rooms[r];
        int vault = 0;
        for (int k = 0; k < G.nvaults; k++) vault |= G.vault_room[k] == r;
        int big = mroom[r][2] >= 3 && mroom[r][3] >= 3, style = vault ? 0 : rnd(10);
        style = style < 3 ? 0 : (style < 6 ? 1 : (style < 8 ? 2 : 3));   /* crypt, graveyard, fire hall, gibbets */
        if (style == 3 && set <= 1 && !since(3)) style = 0;        /* no gibbets on the first two levels */
        if (r == 0 && style == 1) style = 2;                       /* the first room has a fire to start by */
        int lit = 0;
        if (big && !vault && set <= 2) {                           /* lights where four blocks meet, hung */
            for (int my = mroom[r][1] + 1; my < mroom[r][1] + mroom[r][3] - 1; my += 2)
                for (int mx = mroom[r][0] + 1; mx < mroom[r][0] + mroom[r][2] - 1; mx += 2) {
                    int x = mstart(mx), y = mstart(my);
                    if (G.map[y][x].kind != K_FLOOR || rnd(10) >= (style == 2 ? 4 : 7)) continue;
                    if (style == 2 && !lit && set <= 1) fire_pit(x, y);
                    else if (set <= 1) chandelier(x, y);
                    else lantern(x, y);
                    lit++;
                }
        }
        if (style == 2 && !lit) {                                  /* a fire hall's fire, in a corner of its middle block */
            for (int tries = 0; tries < 20; tries++) {
                int x = R->x + rnd(R->w), y = R->y + rnd(R->h);
                if (G.map[y][x].kind != K_FLOOR || !block_corner(x, y) || by_way(x, y)) continue;
                hall_fire(x, y); lit = 1;
                break;
            }
        }
        for (int tries = 0; tries < 40 && lit < (big ? 2 : 1); tries++) {   /* every room some light of its own; a big one two */
            int x = R->x + rnd(R->w), y = R->y + rnd(R->h);
            if (G.map[y][x].kind != K_FLOOR || !block_corner(x, y) || by_way(x, y)) continue;
            room_light(x, y, vault);
            lit++;
        }
        for (int y = R->y; y < R->y + R->h; y++)
            for (int x = R->x; x < R->x + R->w; x++) {
                if (G.map[y][x].kind != K_FLOOR || lane(x, y)) continue;
                int corner = block_corner(x, y) && !by_way(x, y);
                if (style == 1 && corner && rndf() < 0.6f) {       /* rows of graves; a mine's carts and props; an altar room */
                    if (set <= 1) solid_prop(x, y, SP_TOMB_ROUND + rnd(4), 1, K_PROP);
                    else if (set == 2) { if (rnd(3)) solid_prop(x, y, SP_CART, 1, K_PROP); else minelamp(x, y); }
                    else if (since(17) && rnd(2)) { solid_prop(x, y, SP_ALTAR_A, 2, K_PROP); light_static(x + 0.5f, y + 0.5f, 0.8f, 3.2f); }
                    else if (since(18)) solid_prop(x, y, SP_GARG_PERCH, 1, K_PROP);   /* a statue: gargoyles from 19 */
                    else solid_prop(x, y, SP_OBSIDIAN, 1, K_PROP);
                    continue;
                }
                if (style == 3 && corner && rndf() < 0.3f) {       /* cages over the floor; deeper, webs and their sacs, or glass */
                    if (set <= 1) add_deco_ex(x + 0.5f, y + 0.5f, since(4) && rnd(3) ? SP_GIBBET_SKEL : SP_GIBBET, 1, DF_SWAY);
                    else if (set == 2) add_deco_ex(x + 0.5f, y + 0.5f, since(13) && rnd(2) ? SP_SAC_A : SP_WEBSHEET, 1, DF_SWAY);
                    else solid_prop(x, y, SP_OBSIDIAN, 1, K_PROP);
                    continue;
                }
                if (set == 1 && style == 1 && rndf() < 0.1f) { add_deco(x + 0.5f, y + 0.5f, rnd(3) ? SP_PUDDLE_A : SP_PUDDLE_B); continue; }   /* the graves have flooded */
                if (set == 1 && since(9) && style == 0 && corner && rnd(30) == 0) { add_deco_ex(x + 0.5f, y + 0.5f, SP_BELL, 1, DF_SWAY); continue; }
                if (set == 2 && style == 0 && corner && since(12) && rnd(12) == 0) { solid_prop(x, y, SP_ARMOURSTAND, 1, K_PROP); continue; }
                if (set == 3 && style == 0 && corner && rnd(4) == 0) { crystals(x, y); continue; }
                if ((style == 1 || style == 3) && since(4) && rndf() < 0.04f + 0.05f * dk) { add_deco_ex(x + 0.5f, y + 0.5f, since(6) && set <= 2 && rnd(2) ? SP_ROOTS : SP_WEBSHEET, 1, DF_SWAY); continue; }
                int d = wall_toward(x, y);
                if (d < 0 || by_way(x, y) || rndf() > (style == 0 ? 0.3f : 0.12f) + 0.1f * dk) continue;
                int spr, k = rnd(10);
                if (k == 0 && G.nlights < MAXLIGHTS - 4) {
                    spr = SP_CANDLES;
                    light_static(x + 0.5f + DX[d] * 0.3f, y + 0.5f + DY[d] * 0.3f, 0.95f, 3.0f);
                } else if (R->theme == TH_OSSUARY) spr = k < 5 ? SP_SKULLPILE : SP_BONES2;
                else if (set == 2 && k < 3) spr = since(13) && k == 1 ? SP_SAC_A : (since(11) && k == 2 ? SP_CARCASS : SP_FUNGUS);
                else if (set == 3 && k < 3) spr = k == 1 ? SP_SKULLPILE : SP_BONES2;
                else spr = k < 3 ? SP_URN : (k < 6 ? SP_BONES2 : (k < 8 ? SP_SKULLPILE : hall_litter()));
                add_deco(x + 0.5f + DX[d] * 0.3f, y + 0.5f + DY[d] * 0.3f, spr);
            }
    }
}

/* ================================================================== */
/* Treasure, the key, a potion                                         */
/* ================================================================== */

/* the boss's strip of the map: nothing of the level's own is put there */
static int in_band(int y) { return G.boss >= 0 && !PK.depth && y >= BAND_Y0; }

/* the boss's antechamber and arena, from its map, into the strip of the map the plan kept for them; it
 * is stamped last, while the strip is still solid, so nothing of the level spills into it */
static void stamp_arena(void) {
    const BossDef *B = &BOSSES[G.boss];
    static const int THEME[NBOSS] = { TH_CRYPT, TH_MOSS, TH_OSSUARY, TH_CORRIDOR };
    G.bnfont = G.bnspawn = 0; G.barch = -1; G.bhome_x = G.bhome_y = G.bstart_x = G.bstart_y = -1;
    G.exit_fx = G.exit_fy = -1; G.exit_in_vault = -1;
    for (int r = 0; r < BAND_H; r++)
        for (int x = 0; x < MAPN; x++) {
            int y = BAND_Y0 + r;
            char ch = B->map[r][x];
            Cell *c = &G.map[y][x];
            memset(c, 0, sizeof *c);
            c->region = -1; c->door = -1; c->visits = 1;           /* nothing here for the autopilot to explore */
            G.fog[y][x] = 0.0f;
            if (ch == '#' || ch == 'E' || ch == 'G' || ch == 'h' || ch == 'F' || ch == 'v') {
                c->kind = K_WALL;
                c->tex = (uint8_t)(ch == 'E' ? T_EXIT0 : ch == 'G' ? T_GATE : ch == 'h' ? T_NICHE : ch == 'F' ? T_FONT_DRY : ch == 'v' ? T_GRATE
                                   : theme_tex(THEME[G.boss], hash2(x, y)));
                if (ch == 'E') { G.exit_wx = x; G.exit_wy = y; }
                if (ch == 'F' && G.bnfont < MAXFONTS) { G.bfont_x[G.bnfont] = x; G.bfont_y[G.bnfont] = y; G.bfont_full[G.bnfont++] = 0; }
                if ((ch == 'G' || ch == 'h') && G.bnspawn < MAXSPAWN) { G.bspawn_x[G.bnspawn] = x; G.bspawn_y[G.bnspawn++] = y; }
                continue;
            }
            c->kind = K_FLOOR; c->region = ARENA_REGION; c->theme = (uint8_t)THEME[G.boss];
            switch (ch) {
            case '@': c->kind = K_DOOR; break;
            case 'a': G.bstart_x = x; G.bstart_y = y; break;
            case 'O': solid_prop(x, y, B->pillar, 1, K_PILLAR); break;
            case '=': case ':': solid_prop(x, y, B->low, 1, K_PROP); break;
            case '%': add_deco(x + 0.5f, y + 0.5f, G.boss == BOSS_DEACON ? SP_BONES2 : SP_SKULLPILE); if (G.bnspawn < MAXSPAWN) { G.bspawn_x[G.bnspawn] = x; G.bspawn_y[G.bnspawn++] = y; } break;
            case 'w': add_deco_ex(x + 0.5f, y + 0.5f, SP_WEBSHEET, 1, DF_SWAY); break;
            case 'K': case 'S': case 'U': case 'W': G.bhome_x = x; G.bhome_y = y; break;
            default: break;
            }
        }
    /* what stands in the walls is used from the floor before it: the exit, the fonts, the gates and burrows */
    #define FRONT(wx, wy, fx, fy, dir) do { for (int d_ = 0; d_ < 4; d_++) { int nx_ = (wx) + DX[d_], ny_ = (wy) + DY[d_]; \
        if (ny_ >= BAND_Y0 && nx_ >= ARENA_X0 && G.map[ny_][nx_].kind == K_FLOOR) { fx = nx_; fy = ny_; dir = (d_ + 2) & 3; break; } } } while (0)
    { int fx = -1, fy = -1, dir = 0; FRONT(G.exit_wx, G.exit_wy, fx, fy, dir); G.exit_fx = fx; G.exit_fy = fy; G.exit_dir = dir; }
    for (int i = 0; i < G.bnfont; i++) { int dir = 0; G.bfont_fx[i] = G.bfont_fy[i] = -1; FRONT(G.bfont_x[i], G.bfont_y[i], G.bfont_fx[i], G.bfont_fy[i], dir); (void)dir; }
    for (int i = 0; i < G.bnspawn; i++)
        if (G.map[G.bspawn_y[i]][G.bspawn_x[i]].kind == K_WALL) { int fx = G.bspawn_x[i], fy = G.bspawn_y[i], dir = 0; FRONT(G.bspawn_x[i], G.bspawn_y[i], fx, fy, dir); (void)dir; G.bspawn_x[i] = fx; G.bspawn_y[i] = fy; }
    #undef FRONT
    for (int y = BAND_Y0; y < MAPN; y++)                            /* the arch: a door that seals behind you */
        for (int x = 0; x < MAPN; x++)
            if (G.map[y][x].kind == K_DOOR && G.ndoors < MAXDOORS) {
                G.map[y][x].door = (int16_t)G.ndoors; G.barch = G.ndoors;
                G.doors[G.ndoors++] = (Door){ x, y, 1, 0.0f, 0.0f, 0, 0, 2 };
            }
    /* light: a candle in the antechamber; braziers; torches along the arena's long walls, but for the
     * Wraith's, which has only its braziers */
    add_deco(6.5f, 31.35f, SP_CANDLES); light_static(6.5f, 31.6f, 1.0f, 4.2f);
    for (int r = 0; r < BAND_H; r++)
        for (int x = 0; x < MAPN; x++) {
            if (B->map[r][x] != 'B') continue;
            if (G.boss != BOSS_WRAITH || G.bbz_n >= 8) { brazier(x, BAND_Y0 + r); continue; }
            solid_prop(x, BAND_Y0 + r, SP_BRAZIER_A, 3, K_PROP);   /* the Wraith's: lit each frame, so they can go out */
            G.bbz_x[G.bbz_n] = x; G.bbz_y[G.bbz_n] = BAND_Y0 + r; G.bbz_lit[G.bbz_n] = 1; G.bbz_deco[G.bbz_n++] = G.ndecos - 1;
        }
    if (G.boss != BOSS_WRAITH)
        for (int x = ARENA_X0 + 5; x < MAPN - 6; x += 8)
            for (int k = 0; k < 2; k++) {
                int wy = k ? MAPN - 2 : BAND_Y0, fy = k ? wy - 1 : wy + 1;
                Cell *w = &G.map[wy][x];
                if (w->kind != K_WALL || w->tex == T_GATE || w->tex == T_NICHE || w->tex == T_FONT_DRY || G.map[fy][x].kind != K_FLOOR) continue;
                w->tex = T_TORCH0;
                light_static(x + 0.5f, wy + 0.5f + (k ? -0.42f : 0.42f), G.boss == BOSS_SPIDER ? 1.1f : 1.55f, 4.6f);
            }
    light_static(ARENA_X0 + 0.6f, 34.5f, 0.9f, 3.6f);               /* the arch, seen from inside */
    G.bnout = 0;                                                   /* the outlets the bile comes from: drains in the walls, and the well */
    for (int r = 0; r < BAND_H && G.bnout < MAXOUT; r++)
        for (int x = 0; x < MAPN && G.bnout < MAXOUT; x++) {
            char ch = B->map[r][x];
            int y = BAND_Y0 + r, sx = -1, sy = -1;
            if (ch == 'v') { for (int d = 0; d < 4; d++) if (in_arena((float)(x + DX[d]), (float)(y + DY[d])) && G.map[y + DY[d]][x + DX[d]].kind == K_FLOOR) { sx = x + DX[d]; sy = y + DY[d]; } }
            else if (ch == ':' && B->map[r - 1][x] != ':' && B->map[r][x - 1] != ':') { sx = x - 1; sy = y + 1; }   /* the well: from its west side */
            if (sx < 0) continue;
            G.bout_n[G.bnout] = (uint8_t)hz_patch(sx, sy, ch == ':' ? 2 : 3, G.bout_cx[G.bnout], G.bout_cy[G.bnout], G.bout_cd[G.bnout], ch == ':' ? 14 : 18);
            G.bout_haz[G.bnout++] = -1;
        }
    if (G.boss == BOSS_KNIGHT) { chandelier(21, 36); chandelier(31, 36); }   /* the lists under their candles, so it can be seen */
    else if (G.boss == BOSS_SPIDER) { lantern(19, 36); lantern(33, 36); }    /* the cistern, dimly */
    else if (G.boss == BOSS_DEACON) {                              /* the chapel, lit for the rite */
        static const int CH[5][2] = { { 17, 33 }, { 35, 33 }, { 26, 36 }, { 20, 39 }, { 32, 39 } };
        for (int i = 0; i < 5; i++) { chandelier(CH[i][0], CH[i][1]); light_static(CH[i][0] + 0.5f, CH[i][1] + 0.5f, 0.9f, 7.0f); }
    }
}

static int item_at(int x, int y) {
    for (int i = 0; i < G.nitems; i++) if (G.items[i].alive && G.items[i].cx == x && G.items[i].cy == y) return 1;
    return 0;
}

static int free_floor(int x, int y) {
    if (cell(x, y)->kind != K_FLOOR || cell(x, y)->secret || G.bulky[y][x] || item_at(x, y) || lane_dist(x, y) > 1 || in_band(y)) return 0;   /* within reach of a lane */
    for (int d = 0; d < 4; d++) if (cell(x + DX[d], y + DY[d])->kind == K_DOOR) return 0;
    return 1;
}

/* treasure is worth more the deeper it lies */
static int treasure_value(int type) { return (int)(ITEM_VALUE[type] * (1.0f + 0.2f * (float)(G.level - 1)) + 0.5f); }

static int vault_of_room(int x, int y) {
    for (int k = 0; k < G.nvaults; k++) if (G.vault_room[k] >= 0 && in_room(&G.rooms[G.vault_room[k]], x, y)) return k;
    return -1;
}

static Item *add_item(int x, int y, int type) {
    if (G.nitems >= MAXITEMS) return NULL;
    Item *it = &G.items[G.nitems++];
    memset(it, 0, sizeof *it);
    it->alive = 1; it->type = type; it->cx = x; it->cy = y;
    it->x = x + 0.5f + (rndf() - 0.5f) * 0.16f; it->y = y + 0.5f + (rndf() - 0.5f) * 0.16f;
    it->phase = rndf() * 10.0f;
    it->lx = x; it->ly = y;                                        /* the lane it is taken from: one running past it */
    int best = -1;
    for (int yy = y - 1; yy <= y + 1; yy++)
        for (int xx = x - 1; xx <= x + 1; xx++) {
            if (!lane(xx, yy) || !is_open(xx, yy)) continue;
            int through = 0;
            for (int d = 0; d < 4; d++) through += lane(xx + DX[d], yy + DY[d]) && is_open(xx + DX[d], yy + DY[d]);
            int sc = through * 4 - abs(xx - x) - abs(yy - y);
            if (sc > best) { best = sc; it->lx = xx; it->ly = yy; }
        }
    if (type < IT_KEY) { G.items_total++; G.value_total += treasure_value(type); }
    return it;
}

static int random_treasure(void) { int k = rnd(20); return k < 9 ? IT_COINS : (k < 14 ? IT_CHALICE : (k < 18 ? IT_GEMS : IT_CROWN)); }

/* The final door: set in a wall at the far end of the level, or now and then inside the vault. */
static void place_exit(int sx, int sy) {
    static int dist[MAPN][MAPN], qx[MAPN * MAPN], qy[MAPN * MAPN];
    for (int y = 0; y < MAPN; y++) for (int x = 0; x < MAPN; x++) dist[y][x] = -1;
    int qh = 0, qt = 0, maxd = 0;
    dist[sy][sx] = 0; qx[qt] = sx; qy[qt] = sy; qt++;
    while (qh < qt) {
        int x = qx[qh], y = qy[qh]; qh++;
        if (dist[y][x] > maxd) maxd = dist[y][x];
        for (int d = 0; d < 4; d++) {
            int nx = x + DX[d], ny = y + DY[d];
            if (!is_open(nx, ny) || dist[ny][nx] >= 0) continue;
            dist[ny][nx] = dist[y][x] + 1; qx[qt] = nx; qy[qt] = ny; qt++;
        }
    }
    int want_vault = G.nvaults > 0 && rnd(100) < 45, xv = G.nvaults > 0 ? rnd(G.nvaults) : -1;
    for (int pass = 0; pass < 2; pass++) {
        int best = -1, bd = 0; float bs = -1e9f;
        for (int y = 1; y < MAPN - 1; y++)
            for (int x = 1; x < MAPN - 1; x++) {
                if (G.map[y][x].kind != K_FLOOR || dist[y][x] < 0 || G.bulky[y][x] || (x == sx && y == sy) || !lane(x, y)) continue;
                int inv = vault_of_room(x, y);
                if (want_vault ? inv != xv : inv >= 0) continue;
                for (int d = 0; d < 4; d++) {
                    int wx = x + DX[d], wy = y + DY[d];
                    Cell *w = cell(wx, wy);
                    if (w->kind != K_WALL || (w->tex >= T_TORCH0 && w->tex <= T_TORCH2) || wx < 0 || wy < 0 || wx >= MAPN || wy >= MAPN) continue;
                    if (DX[d] ? (y & 3) != 2 : (x & 3) != 2) continue;     /* the lane runs into it */
                    int open = 0;
                    for (int e = 0; e < 4; e++) open += is_open(wx + DX[e], wy + DY[e]) || cell(wx + DX[e], wy + DY[e])->kind >= K_PILLAR;
                    if (open != 1) continue;                           /* only one face, so it can't be seen from behind */
                    float sc = want_vault ? rndf() : (float)dist[y][x] + rndf() * 5.0f;
                    if (sc > bs) { bs = sc; best = (y * MAPN + x) * 4 + d; bd = d; }
                }
            }
        if (best < 0) { want_vault = 0; continue; }
        int x = (best / 4) % MAPN, y = (best / 4) / MAPN;
        G.exit_fx = x; G.exit_fy = y; G.exit_dir = bd;
        G.exit_wx = x + DX[bd]; G.exit_wy = y + DY[bd];
        G.exit_in_vault = want_vault ? xv : -1;
        G.map[G.exit_wy][G.exit_wx].tex = T_EXIT0;
        light_static(x + 0.5f + DX[bd] * 0.35f, y + 0.5f + DY[bd] * 0.35f, 0.9f, 3.2f);   /* its runes light the stone */
        return;
    }
    G.exit_fx = G.exit_fy = -1;
}

static int room_cell(int room, int *ox, int *oy) {                 /* a free floor cell in a room */
    const Room *R = &G.rooms[room];
    for (int tries = 0; tries < 120; tries++) {
        int x = R->x + rnd(R->w), y = R->y + rnd(R->h);
        if (G.map[y][x].kind != K_FLOOR || G.map[y][x].secret || G.bulky[y][x] || item_at(x, y) || lane_dist(x, y) > 1) continue;
        *ox = x; *oy = y;
        return 1;
    }
    return 0;
}

static void place_items(int sx, int sy) {
    G.nitems = 0; G.items_total = 0; G.items_got = 0; G.value_total = G.value_got = 0;
    static int dist[MAPN][MAPN], qx[MAPN * MAPN], qy[MAPN * MAPN];
    for (int y = 0; y < MAPN; y++) for (int x = 0; x < MAPN; x++) dist[y][x] = -1;
    int qh = 0, qt = 0, maxd = 0;
    dist[sy][sx] = 0; qx[qt] = sx; qy[qt] = sy; qt++;
    while (qh < qt) {                                              /* reachable without any key */
        int x = qx[qh], y = qy[qh]; qh++;
        if (dist[y][x] > maxd) maxd = dist[y][x];
        for (int d = 0; d < 4; d++) {
            int nx = x + DX[d], ny = y + DY[d];
            if (!passable(nx, ny) || dist[ny][nx] >= 0) continue;
            dist[ny][nx] = dist[y][x] + 1; qx[qt] = nx; qy[qt] = ny; qt++;
        }
    }
    for (int r = 1; r < G.nrooms; r++) {                           /* a little in most rooms, more deeper down */
        if (vault_of_room(G.rooms[r].x, G.rooms[r].y) >= 0) continue;
        Room *R = &G.rooms[r];
        int n = rnd(3) + (G.level >= 4);
        for (int k = 0, tries = 0; k < n && tries < 30; tries++) {
            int x = R->x + rnd(R->w), y = R->y + rnd(R->h);
            if (dist[y][x] < 0 || !free_floor(x, y)) continue;
            add_item(x, y, random_treasure()); k++;
        }
    }
    for (int i = 0, placed = 0; i < 400 && placed < 7 + G.level / 2; i++) {   /* and along the halls */
        int x = 1 + rnd(MAPN - 2), y = 1 + rnd(MAPN - 2);
        if (G.map[y][x].theme != TH_CORRIDOR || dist[y][x] < 4 || !free_floor(x, y)) continue;
        add_item(x, y, rnd(3) ? IT_COINS : IT_CHALICE); placed++;
    }
    static const int HOARD[10] = { IT_CHEST, IT_CROWN, IT_GEMS, IT_COINS, IT_COINS, IT_CHALICE, IT_GEMS, IT_CHALICE, IT_CROWN, IT_GEMS };
    for (int k = 0; k < G.nvaults; k++) {                          /* the hoards: richer behind silver, richer still behind crimson */
        int n = k == 0 ? 6 : (k == 1 ? 8 : 10), x, y;
        for (int i = 0; i < n; i++) if (room_cell(G.vault_room[k], &x, &y)) add_item(x, y, HOARD[i]);
        for (int i = 0; i < k + 1 && k >= 1; i++) if (room_cell(G.vault_room[k], &x, &y)) add_item(x, y, IT_URN);
        if (k == 2 && rnd(4) == 0 && room_cell(G.vault_room[k], &x, &y)) add_item(x, y, IT_RING);
    }
    /* the keys: usually out in the halls, a short walk from their door; now and then locked in the vault before */
    static int ddist[MAPN][MAPN];
    for (int k = 0; k < G.nvaults; k++) {
        int x, y;
        if (k > 0 && rnd(100) < 35 && room_cell(G.vault_room[k - 1], &x, &y)) { Item *it = add_item(x, y, IT_KEY); if (it) it->color = k; continue; }
        const Door *vd = &G.doors[G.vault_door[k]];
        for (int yy = 0; yy < MAPN; yy++) for (int xx = 0; xx < MAPN; xx++) ddist[yy][xx] = -1;
        qh = qt = 0;
        for (int d = 0; d < 4; d++) {
            int nx = vd->x + DX[d], ny = vd->y + DY[d];
            if (passable(nx, ny) && dist[ny][nx] >= 0) { ddist[ny][nx] = 1; qx[qt] = nx; qy[qt] = ny; qt++; }
        }
        while (qh < qt) {
            int xx = qx[qh], yy = qy[qh]; qh++;
            for (int d = 0; d < 4; d++) {
                int nx = xx + DX[d], ny = yy + DY[d];
                if (!passable(nx, ny) || ddist[ny][nx] >= 0) continue;
                ddist[ny][nx] = ddist[yy][xx] + 1; qx[qt] = nx; qy[qt] = ny; qt++;
            }
        }
        int best = -1; float bs = -1e9f;
        for (int yy = 1; yy < MAPN - 1; yy++)
            for (int xx = 1; xx < MAPN - 1; xx++) {
                if (dist[yy][xx] < maxd / 5 || ddist[yy][xx] < 0 || !free_floor(xx, yy)) continue;
                float sc = -fabsf(ddist[yy][xx] - 20.0f) + (G.map[yy][xx].theme != TH_CORRIDOR ? 4.0f : 0.0f) + rndf() * 6.0f
                        - (dist[yy][xx] > maxd * 3 / 5 ? 10.0f : 0.0f);
                if (sc > bs) { bs = sc; best = yy * MAPN + xx; }
            }
        if (best >= 0) { Item *it = add_item(best % MAPN, best / MAPN, IT_KEY); if (it) it->color = k; }
    }
    for (int i = 0; i < 300; i++) {                                /* one healing potion */
        int x = 1 + rnd(MAPN - 2), y = 1 + rnd(MAPN - 2);
        if (dist[y][x] < 6 || !free_floor(x, y)) continue;
        add_item(x, y, IT_POTION);
        break;
    }
    /* and in each secret room, off the stone's path and clear of its doorway, within reach of a lane:
     * treasure, more for a bigger room, and often something to heal with. None of it counts towards the
     * level's treasure, so a secret is pure gain */
    static const int LOOT[6] = { IT_CHEST, IT_CROWN, IT_GEMS, IT_GEMS, IT_CROWN, IT_CHALICE };
    for (int k = 0; k < G.nsecrets; k++) {
        const Secret *sc = &G.secrets[k];
        static int spots[64][2];
        int ns = 0;
        for (int y = sc->y0; y <= sc->y1; y++)
            for (int x = sc->x0; x <= sc->x1; x++) {
                int along = (x - sc->sx) * DX[sc->d] + (y - sc->sy) * DY[sc->d], across = (x - sc->sx) * DY[sc->d] + (y - sc->sy) * DX[sc->d];
                if (across == 0 || lane_dist(x, y) > 1 || ns >= 64) continue;   /* the stone's way, and out of reach */
                if (sc->door_x >= 0 && abs(x - sc->door_x) + abs(y - sc->door_y) <= 1) continue;
                if (along < 2 && sc->len > 3) continue;            /* a big room keeps its front clear */
                spots[ns][0] = x; spots[ns][1] = y; ns++;
            }
        for (int i = ns - 1; i > 0; i--) { int j = rnd(i + 1); int t0 = spots[i][0], t1 = spots[i][1]; spots[i][0] = spots[j][0]; spots[i][1] = spots[j][1]; spots[j][0] = t0; spots[j][1] = t1; }
        int big = (sc->x1 - sc->x0 + 1) * (sc->y1 - sc->y0 + 1), n = 2 + (G.level >= 4) + (rnd(3) == 0) + (big > 9) + (big > 21);
        for (int i = 0; i < n && i < ns; i++) {
            int type = (i == n - 1 && rnd(10) < 6) ? IT_POTION : LOOT[rnd(6)];
            Item *it = add_item(spots[i][0], spots[i][1], type);
            if (!it) break;
            it->secret = 1;
            if (type < IT_KEY) { G.items_total--; G.value_total -= treasure_value(type); }
        }
    }
}

/* wall skulls, from level 3: on a wall the caster will walk straight at, at the end of a run of hall */
static void place_wall_skulls(int sx, int sy) {
    if (G.level < KINDS[MOB_WSKULL].debut) return;
    int want = G.level == KINDS[MOB_WSKULL].debut ? 1 : 2 + (G.level >= 5) + rnd(2), placed = 0, px[6], py[6];
    for (int tries = 0; tries < 3000 && placed < want; tries++) {
        int x = 1 + rnd(MAPN - 2), y = 1 + rnd(MAPN - 2), d = rnd(4);
        if (G.map[y][x].kind != K_FLOOR || G.map[y][x].secret || G.bulky[y][x] || vault_of_room(x, y) >= 0 || abs(x - sx) + abs(y - sy) < 6 || in_band(y)) continue;
        Cell *w = cell(x + DX[d], y + DY[d]);
        if (w->kind != K_WALL || w->tex >= T_TORCH0 || w->secret) continue;
        int run = 1;
        for (int k = 1; k <= 3; k++) run &= cell(x - DX[d] * k, y - DY[d] * k)->kind == K_FLOOR;
        if (!run) continue;
        int far = 1;
        for (int i = 0; i < placed; i++) if (abs(px[i] - x) + abs(py[i] - y) < 6) far = 0;
        if (!far) continue;
        Mob *m = spawn_mob(x, y, MOB_WSKULL);
        if (!m) break;
        m->x = x + 0.5f + DX[d] * 0.42f; m->y = y + 0.5f + DY[d] * 0.42f; m->z = 0.3f;
        m->face = heading_angle_raw(d);
        px[placed] = x; py[placed] = y; placed++;
    }
}

/* spiders, from level 6: in some of the corner webs */
static void place_spiders(int sx, int sy) {
    if (G.level < KINDS[MOB_SPIDER].debut) return;
    int want = G.level == KINDS[MOB_SPIDER].debut ? 1 : 2 + (G.level >= 8) + rnd(2), placed = 0;
    for (int tries = 0; tries < 60 && placed < want && G.ndecos; tries++) {
        const Deco *dc = &G.decos[rnd(G.ndecos)];
        if (dc->spr != SP_COBWEB || hypotf(dc->x - sx, dc->y - sy) < 5.0f) continue;
        int clash = 0;
        for (int i = 0; i < MAXMOBS; i++) if (G.mobs[i].alive && G.mobs[i].type == MOB_SPIDER && hypotf(G.mobs[i].ax - dc->x, G.mobs[i].ay - dc->y) < 4.0f) clash = 1;
        if (clash) continue;
        Mob *m = spawn_mob((int)dc->x, (int)dc->y, MOB_SPIDER);
        if (!m) break;
        m->ax = m->x = dc->x; m->ay = m->y = dc->y; m->z = 0.62f;
        placed++;
    }
}

/* The buried, from level 2: a grave whose earth has been turned over from below, with something under
 * it that climbs out when you come near. The Drowned, from 7: a puddle that stirs. */
static void place_risers(void) {
    int set = crawl_set(G.level);
    for (int pass = 0; pass < 2; pass++) {
        int type = pass ? MOB_DROWNED : MOB_SHAMBLER;
        if ((pass ? set != 1 : set > 1) || !may_spawn(type)) continue;
        int want = G.level == KINDS[type].debut ? 1 : 2 + rnd(2), placed = 0;
        for (int tries = 0; tries < 300 && placed < want && G.ndecos; tries++) {
            int di = rnd(G.ndecos), sp = G.decos[di].spr;
            int x = (int)G.decos[di].x, y = (int)G.decos[di].y, at = -1;
            if (pass) {                                            /* in the puddle itself */
                if ((sp != SP_PUDDLE_A && sp != SP_PUDDLE_B) || G.map[y][x].kind != K_FLOOR) continue;
                at = di;
            } else {                                               /* on the floor before the stone */
                if (sp < SP_TOMB_ROUND || sp > SP_TOMB_BROKEN) continue;
                int d = rnd(4), k = 0;
                for (; k < 4; k++, d = (d + 1) & 3) if (G.map[y + DY[d]][x + DX[d]].kind == K_FLOOR && !G.map[y + DY[d]][x + DX[d]].secret) break;
                if (k == 4) continue;
                x += DX[d]; y += DY[d];
            }
            if (occupied(x, y, NULL) || abs(x - G.cx) + abs(y - G.cy) < 8) continue;
            Mob *m = spawn_mob(x, y, type);
            if (!m) return;
            m->sub = 1; m->home = G.map[y][x].region;
            if (pass) { m->deco = at; G.decos[at].spr = SP_PUDDLE_B; }
            else { m->deco = G.ndecos; add_deco(x + 0.5f, y + 0.5f, SP_MOUND); }
            placed++;
        }
    }
}

/* Ember imps, from 17: born of the magma, they keep near it till they know you are there */
static void place_imps(void) {
    if (!may_spawn(MOB_IMP)) return;
    int want = G.level == KINDS[MOB_IMP].debut ? 1 : 2 + rnd(2), placed = 0;
    for (int tries = 0; tries < 200 && placed < want && G.ndecos; tries++) {
        const Deco *dc = &G.decos[rnd(G.ndecos)];
        int d = rnd(4), x = (int)dc->x + DX[d], y = (int)dc->y + DY[d];
        if (dc->spr != SP_MAGMA_A || G.map[y][x].kind != K_FLOOR || G.map[y][x].secret || occupied(x, y, NULL) || abs(x - G.cx) + abs(y - G.cy) < 8) continue;
        Mob *m = spawn_mob(x, y, MOB_IMP);
        if (!m) return;
        m->home = G.map[y][x].region;
        placed++;
    }
}

/* Gargoyles, from 19: some of the statues are not statues. If the level has too few, one stands in a
 * room's corner, on a plinth like theirs. */
static void place_gargoyles(void) {
    if (!may_spawn(MOB_GARGOYLE)) return;
    int want = G.level == KINDS[MOB_GARGOYLE].debut ? 1 : 2 + rnd(2), placed = 0;
    for (int tries = 0; tries < 400 && placed < want; tries++) {
        int x, y, di = -1;
        if (tries < 200) {                                         /* a statue already standing */
            if (!G.ndecos) continue;
            di = rnd(G.ndecos);
            if (G.decos[di].spr != SP_GARG_PERCH) continue;
            x = (int)G.decos[di].x; y = (int)G.decos[di].y;
        } else {                                                   /* or a corner of a room */
            const Room *R = &G.rooms[rnd(G.nrooms)];
            x = R->x + rnd(R->w); y = R->y + rnd(R->h);
            if (G.map[y][x].kind != K_FLOOR || G.map[y][x].secret || !block_corner(x, y) || lane(x, y) || by_way(x, y) || occupied(x, y, NULL) || in_band(y)) continue;
        }
        if (abs(x - G.cx) + abs(y - G.cy) < 10) continue;
        int taken = 0;
        for (int i = 0; i < MAXMOBS; i++) taken |= G.mobs[i].alive && G.mobs[i].cx == x && G.mobs[i].cy == y;
        if (taken) continue;
        Mob *m = spawn_mob(x, y, MOB_GARGOYLE);
        if (!m) return;
        if (di >= 0) G.decos[di].spr = -1;                         /* it is drawn as itself now */
        G.map[y][x].kind = K_PROP;
        m->home = G.map[y][x].region;
        placed++;
    }
}

/* the silver and crimson rooms are guarded */
static void place_guardians(void) {
    for (int k = 1; k < G.nvaults; k++) {
        int x, y;
        if (!room_cell(G.vault_room[k], &x, &y)) continue;
        if (may_spawn(MOB_SPIDER) && G.level > KINDS[MOB_SPIDER].debut && rnd(5) < 2) {   /* a spider in a corner of the room */
            const Room *R = &G.rooms[G.vault_room[k]];
            int cx = rnd(2) ? R->x : R->x + R->w - 1, cy = rnd(2) ? R->y : R->y + R->h - 1;
            if (G.map[cy][cx].kind != K_FLOOR) continue;
            float wx = cx + (cx == R->x ? 0.16f : 0.84f), wy = cy + (cy == R->y ? 0.16f : 0.84f);
            add_deco(wx, wy, SP_COBWEB);
            Mob *m = spawn_mob(cx, cy, MOB_SPIDER);
            if (m) { m->ax = m->x = wx; m->ay = m->y = wy; m->z = 0.62f; m->guard = 1; }
        } else {
            Mob *m = spawn_mob(x, y, may_spawn(MOB_SHIELD) ? MOB_SHIELD : MOB_SKEL);
            if (m) m->guard = 1;
        }
    }
}

/* ================================================================== */
/* Light map                                                           */
/* ================================================================== */

/* flat: an even pool of light that fades only towards its edge, rather than a flame's bright heart */
static void light_dynamic_ex(float lx, float ly, float power, float radius, int flat) {
    int x0 = clampi((int)((lx - radius) * LMR), 0, LMN - 1), x1 = clampi((int)((lx + radius) * LMR), 0, LMN - 1);
    int y0 = clampi((int)((ly - radius) * LMR), 0, LMN - 1), y1 = clampi((int)((ly + radius) * LMR), 0, LMN - 1);
    int lcx = (int)lx, lcy = (int)ly;
    for (int ty = y0; ty <= y1; ty++)
        for (int tx = x0; tx <= x1; tx++) {
            float wx = (tx + 0.5f) / LMR, wy = (ty + 0.5f) / LMR;
            float d = sqrtf((wx - lx) * (wx - lx) + (wy - ly) * (wy - ly));
            if (d >= radius) continue;
            int cx = tx / LMR, cy = ty / LMR;
            if (abs(cx - lcx) + abs(cy - lcy) > 1) {               /* cheap occlusion: step towards the light */
                int blocked = 0;
                float sx = wx, sy = wy, vx = (lx - wx) / d * 0.35f, vy = (ly - wy) / d * 0.35f;
                for (float s = 0.35f; s < d - 0.35f; s += 0.35f) {
                    sx += vx; sy += vy;
                    if (blocks_light((int)sx, (int)sy)) { blocked = 1; break; }
                }
                if (blocked) continue;
            }
            float q = 1.0f - d * d / (radius * radius);
            G.lm[ty * LMN + tx] += flat ? power * q : power / (1.0f + 0.9f * d * d) * (1.0f - smoothstepf(radius * 0.55f, radius, d));
        }
}
static void light_dynamic(float lx, float ly, float power, float radius) { light_dynamic_ex(lx, ly, power, radius, 0); }

static float torch_flicker(const Light *L) {
    if (L->phase < -100.0f) return 1.0f;                          /* a steady glow, not a flame */
    if (L->phase < 0.0f) {                                         /* a failing tube: steady, then a burst of blinking */
        float t = (float)G.t - L->phase * 13.0f;
        if (fmodf(t, 5.3f) > 0.9f) return 1.0f;
        uint32_t h = (uint32_t)(int)(t * 14.0f) * 2654435761u;
        return (h >> 13 & 3) ? 0.12f : 1.0f;
    }
    float t = (float)G.t + L->phase;
    return 0.80f + 0.12f * sinf(t * 9.1f) * sinf(t * 5.3f + 1.0f) + 0.08f * sinf(t * 23.0f + L->phase * 3.0f);
}

static void build_lightmap(void) {
    float ambient = G.shop ? 0.06f : (PK.depth ? (G.ptheme == PT_SEVERANCE ? 0.16f : 0.08f) : 0.06f - 0.012f * depth());   /* the dark itself deepens, set by set; not in a pocket */
    for (int i = 0; i < LMN * LMN; i++) G.lm[i] = ambient;
    const Mob *wr = NULL;                                          /* torches gutter where the Wraith goes */
    for (int i = 0; i < MAXMOBS; i++) if (G.mobs[i].alive && G.mobs[i].type == MOB_WRAITH) wr = &G.mobs[i];
    for (int l = 0; l < G.nlights; l++) {
        const Light *L = &G.lights[l];
        float f = torch_flicker(L);
        if (wr && fabsf(L->x - wr->x) + fabsf(L->y - wr->y) < 5.0f) f *= 0.35f + 0.15f * sinf((float)G.t * 13.0f + l);
        for (int k = L->first; k < L->first + L->count; k++) G.lm[G.lc_idx[k]] += G.lc_w[k] * f;
    }
    light_dynamic(G.px, G.py, 0.62f, 2.8f);                        /* the caster's own faint glow */
    for (int b = 0; b < G.bbz_n && G.boss == BOSS_WRAITH && !PK.depth && !G.shop; b++)   /* the Wraith's braziers, those still lit */
        if (G.bbz_lit[b]) light_dynamic(G.bbz_x[b] + 0.5f, G.bbz_y[b] + 0.5f, 1.5f * (0.92f + 0.08f * sinf((float)G.t * 9.0f + b)), 5.2f);
    if (G.shop && RUN.shrine > 0)                                  /* the cursed shrine, brighter as it works */
        light_dynamic(SHRINE_X, SHRINE_Y - 0.4f, 0.7f + 0.8f * fminf(1.0f, G.sh_glow) + 0.08f * sinf((float)G.t * 5.0f), 3.6f);
    for (int i = 0; i < MAXWISPS && !G.shop; i++) {                /* and the wisps', four cells round them */
        const Wisp *w = &G.wisps[i];
        if (w->on && w->a > 0.0f) light_dynamic_ex(w->x, w->y, 0.6f * w->a * (0.93f + 0.07f * sinf((float)G.t * 7.0f + i)), 4.0f, 1);
    }
    for (int i = 0; i < MAXSHOTS; i++)
        if (G.shots[i].alive) {
            static const float POW[NATK + 1] = { 1.7f, 1.1f, 1.2f, 0.4f, 1.0f, 2.3f, 0.8f, 0.7f, 0.0f, 1.4f };
            light_dynamic(G.shots[i].x, G.shots[i].y, POW[G.shots[i].kind], G.shots[i].kind == AT_HELL ? 4.4f : 3.4f);
        }
    for (int i = 0; i < MAXBONES; i++)
        if (G.bones[i].alive && G.bones[i].kind != MIS_BONE) light_dynamic(G.bones[i].x, G.bones[i].y, G.bones[i].kind == MIS_FIRE ? 1.6f : 0.9f, G.bones[i].kind == MIS_FIRE ? 3.6f : 2.6f);
    for (int i = 0; i < MAXBOOMS; i++)
        if (G.booms[i].alive) light_dynamic(G.booms[i].x, G.booms[i].y, (G.booms[i].big ? 3.6f : 2.8f) * (1.0f - G.booms[i].t / 0.6f), G.booms[i].big ? 5.5f : 4.2f);
    for (int i = 0; i < MAXBURNS; i++)
        if (G.burns[i].alive) light_dynamic(G.burns[i].x, G.burns[i].y, 1.5f * (1.0f - smoothstepf(1.8f, 2.5f, G.burns[i].t)) * (0.85f + 0.15f * sinf((float)G.t * 17.0f + i)), 3.2f);
    for (int i = 0; i < MAXBEAMS; i++)
        if (G.beams[i].alive) light_dynamic((G.beams[i].x0 + G.beams[i].x1) * 0.5f, (G.beams[i].y0 + G.beams[i].y1) * 0.5f, 1.6f * (1.0f - G.beams[i].t / 0.3f), 3.5f);
    for (int i = 0; i < MAXMOBS; i++) {                            /* a wall skull gathering its fire; the bones a deacon calls */
        const Mob *m = &G.mobs[i];
        if (m->alive && m->type == MOB_WSKULL && m->state == MS_WALK && m->sub == 1)
            light_dynamic(m->x, m->y, 0.3f + 1.2f * fminf(1.0f, m->sub_t / 3.0f), 3.0f);
        if (m->alive && m->type == MOB_DEACON && m->state == MS_WALK && m->swing > 0.0f)
            light_dynamic(m->ax, m->ay, 0.4f + 0.8f * (1.0f - m->swing / DEACON_CHANT), 2.6f);
    }
    for (int i = 0; i < G.nitems; i++)                             /* keys glow, the urns faintly, a closet's loot a little */
        if (G.items[i].alive && G.items[i].secret && !G.map[G.items[i].cy][G.items[i].cx].secret)
            light_dynamic(G.items[i].x, G.items[i].y, 0.35f + 0.1f * sinf((float)G.t * 2.0f + i), 1.8f);
    for (int i = 0; i < G.nitems; i++)
        if (G.items[i].alive && (G.items[i].type == IT_KEY || G.items[i].type == IT_URN))
            light_dynamic(G.items[i].x, G.items[i].y, (G.items[i].type == IT_KEY ? 0.75f : 0.35f) + 0.2f * sinf((float)G.t * 3.0f + i), 2.2f);
    if (G.state == PS_CAST && G.hand > 0.3f)
        light_dynamic(G.px + cosf(G.ang) * 0.3f, G.py + sinf(G.ang) * 0.3f, 0.9f * G.hand, 2.4f);
    if (!G.shop && G.altar_x >= 0 && (G.altar_state == ALTAR_AWAKE || G.altar_state == ALTAR_READY || G.blessed))   /* the altar's fire */
        light_dynamic(G.altar_x + 0.5f, G.altar_y + 0.5f, 1.1f + 0.15f * sinf((float)G.t * 9.0f), 3.6f);
}

static float lm_at(float x, float y) {
    float fx = x * LMR - 0.5f, fy = y * LMR - 0.5f;
    int ix = (int)floorf(fx), iy = (int)floorf(fy);
    float ax = fx - ix, ay = fy - iy;
    if (ix < 0 || iy < 0 || ix >= LMN - 1 || iy >= LMN - 1) return 0.0f;
    const float *p = &G.lm[iy * LMN + ix];
    return (p[0] * (1 - ax) + p[1] * ax) * (1 - ay) + (p[LMN] * (1 - ax) + p[LMN + 1] * ax) * ay;
}

static inline float atten(float d) { return 1.0f / (1.0f + 0.010f * d * d); }

/* ================================================================== */
/* Mobs, shots, explosions                                             */
/* ================================================================== */

static int line_clear(float x0, float y0, float x1, float y1) {
    float dx = x1 - x0, dy = y1 - y0, d = sqrtf(dx * dx + dy * dy);
    int n = (int)(d / 0.1f);
    for (int i = 1; i < n; i++) {
        float t = (float)i / n;
        int cx = (int)(x0 + dx * t), cy = (int)(y0 + dy * t);
        Cell *c = cell(cx, cy);
        if (c->kind == K_WALL || c->kind == K_PILLAR || (c->kind == K_DOOR && G.doors[c->door].open < 0.7f)) return 0;
    }
    return 1;
}

/* would a fireball thrown from the caster get there? (finer than line_clear, and from where it leaves the hand) */
static int shot_clear(float x1, float y1) {
    float a = atan2f(y1 - G.py, x1 - G.px), x0 = G.px + cosf(a) * 0.35f, y0 = G.py + sinf(a) * 0.35f;
    float dx = x1 - x0, dy = y1 - y0, d = sqrtf(dx * dx + dy * dy);
    int n = (int)(d / 0.04f) + 1;
    for (int i = 0; i <= n; i++) {
        float t = (float)i / n;
        int cx = (int)(x0 + dx * t), cy = (int)(y0 + dy * t);
        if (blocks_light(cx, cy) || cell(cx, cy)->kind == K_PILLAR) return 0;
    }
    return 1;
}

static void sfx_at(int id, float x, float y, float gain) {
    float dx = x - G.px, dy = y - G.py, d = sqrtf(dx * dx + dy * dy);
    float rx = -sinf(G.ang), ry = cosf(G.ang);
    float lat = dx * rx + dy * ry;
    float g = gain / (1.0f + 0.05f * d * d);
    if (g > 0.03f) snd_sfx_ex(id, g, 0.5f + 0.4f * clampf(lat / (d + 0.5f), -1.0f, 1.0f));
}

/* creatures that stand in a cell and get in the way (not wraiths, wall skulls or spiders in their webs) */
static int buried(const Mob *m);
static int solid_mob(const Mob *m) { return m->alive && KIND_IS(m, KF_SOLID) && !buried(m); }

static int occupied(int x, int y, const Mob *self) {
    if ((x == G.cx && y == G.cy) || (x == G.tx && y == G.ty)) return 1;
    for (int i = 0; i < MAXMOBS; i++) {
        const Mob *m = &G.mobs[i];
        if (m != self && solid_mob(m) && ((m->tx == x && m->ty == y) || (m->cx == x && m->cy == y))) return 1;
    }
    return 0;
}

static float mob_base_hp(int type) { return KINDS[type].hp; }
static float shield_hp(void) { return 10.0f + 6.0f * (float)(G.level - 3); }   /* grows tougher each level */

/* a creature takes a rank: its colour, and its toughness, blow, pace and worth with it */
static void set_rank(Mob *m, int rank) {
    m->rank = rank;
    m->speed = KINDS[m->type].speed * RANK_SPEED[rank];
    m->hp = m->maxhp = m->type == MOB_WSKULL ? 1.0f : mob_base_hp(m->type) * powf(1.04f, (float)(G.level - 1)) * RANK_HP[rank];
    if (m->type == MOB_SHIELD) m->shield = shield_hp() * sqrtf(RANK_HP[rank]);
    m->dmgk = RANK_DMG[rank];
}

/* which rank, by the set's mix (PLAN-bestiary, section 4): the set's newest rung comes in on its run's
 * fifth level; each time round again the mix climbs a rung, up to black */
static int pick_rank(int type) {
    if (KINDS[type].family < 0) return RK_I;
    if (CON.rank >= 0) return CON.rank;
#ifdef CRAWL_DEBUG
    if (getenv("CRAWL_RANK")) return clampi(atoi(getenv("CRAWL_RANK")), 0, NRANK - 1);
#endif
    static const int MIX[4][5] = { { 85, 15, 0, 0, 0 }, { 55, 35, 10, 0, 0 }, { 20, 40, 30, 10, 0 }, { 0, 25, 35, 30, 10 } };
    int set = crawl_set(G.level), step = crawl_step(G.level), loop = crawl_loop(G.level), share[5];
    if (type == MOB_BAT && set == 3) return RK_IV;                 /* the Deep's bats: embers, all of them */
    for (int i = 0; i < 5; i++) share[i] = MIX[set][i];
    if (step < 4 && loop == 0) { share[set] += share[set + 1]; share[set + 1] = 0; }
    for (int l = 0; l < loop; l++) { share[4] += share[3]; for (int i = 3; i > 0; i--) share[i] = share[i - 1]; share[0] = 0; }
    int r = rnd(100), k = 0;
    while (k < 4 && r >= share[k]) r -= share[k++];
    return k < KINDS[type].top ? k : KINDS[type].top;              /* a black bat is lost in the dark: they stop at red */
}

/* may one of these come now? not before its first level; and on that level, only the one */
static int may_spawn(int type) {
    if (G.level < KINDS[type].debut) return 0;
    return G.level > KINDS[type].debut || G.spawned[type] == 0;
}

static Mob *spawn_mob(int x, int y, int type) {
    for (int i = 0; i < MAXMOBS; i++) {
        Mob *m = &G.mobs[i];
        if (m->alive) continue;
        memset(m, 0, sizeof *m);
        m->alive = 1; m->type = type; m->state = MS_WALK;
        m->x = x + 0.5f; m->y = y + 0.5f; m->cx = m->tx = x; m->cy = m->ty = y;
        m->home = -1; m->last_atk = -1; m->deco = -1;
        set_rank(m, pick_rank(type));
        G.spawned[type]++;
        m->face = rndf() * TAU_F;
        if (!(KINDS[type].flags & KF_UNCOUNTED)) G.kills_total++;
        m->t = rndf() * 10.0f; m->orbit = rndf() * TAU_F;
        return m;
    }
    return NULL;
}

/* what wanders the halls: skeletons, spectres and bats, shield-bearers from level 4; the spectres
 * give way to the Wraith after the first run, and bats grow rare in the Deep */
static int random_type(void) {
    int w[NKINDS] = { 0 }, total = 0;
    w[MOB_SKEL] = 10;
    w[MOB_GHOST] = G.level <= 10 ? 5 : 0;
    w[MOB_BAT] = crawl_set(G.level) == 3 ? 2 : 5;
    w[MOB_SHIELD] = may_spawn(MOB_SHIELD) ? 4 : 0;
    w[MOB_SHAMBLER] = may_spawn(MOB_SHAMBLER) ? (crawl_set(G.level) <= 1 ? 5 : 3) : 0;
    w[MOB_HOUND] = may_spawn(MOB_HOUND) ? 4 : 0;
    w[MOB_HUNTER] = may_spawn(MOB_HUNTER) ? 3 : 0;
    w[MOB_GHOUL] = may_spawn(MOB_GHOUL) ? 4 : 0;
    w[MOB_KNIGHT] = may_spawn(MOB_KNIGHT) ? 2 : 0;
    w[MOB_SHADE] = may_spawn(MOB_SHADE) ? 3 : 0;
    w[MOB_IMP] = may_spawn(MOB_IMP) ? 1 : 0;
    w[MOB_DEACON] = may_spawn(MOB_DEACON) ? 2 : 0;
    for (int i = 0; i < NKINDS; i++) total += w[i];
    int r = rnd(total), k = 0;
    while (r >= w[k]) r -= w[k++];
    return k;
}

/* a wanderer, or a few: hounds hunt in pairs (except the first, which comes alone), a knight keeps two,
 * and a deacon walks with its dead */
static void spawn_wanderer_type(int x, int y, int type) {
    Mob *m = spawn_mob(x, y, type);
    int escort = type == MOB_HOUND && G.level > KINDS[MOB_HOUND].debut ? 1
               : type == MOB_KNIGHT && G.level > KINDS[MOB_KNIGHT].debut ? 2
               : type == MOB_DEACON ? (G.level > KINDS[MOB_DEACON].debut ? 3 : 2) : 0;
    for (int d = 0; m && d < 4 && escort; d++)
        if (G.map[y + DY[d]][x + DX[d]].kind == K_FLOOR && !occupied(x + DX[d], y + DY[d], NULL)) {
            spawn_mob(x + DX[d], y + DY[d], type == MOB_DEACON ? MOB_SKEL : MOB_HOUND);
            escort--;
        }
}
static void spawn_wanderer(int x, int y) { spawn_wanderer_type(x, y, random_type()); }

/* cells reachable from (sx,sy) at a BFS depth in [dmin, dmax] */
static int pick_cell_at_depth(int sx, int sy, int dmin, int dmax, int need_hidden, int *ox, int *oy) {
    static int dist[MAPN][MAPN], qx[MAPN * MAPN], qy[MAPN * MAPN], cand[MAPN * MAPN];
    for (int y = 0; y < MAPN; y++) for (int x = 0; x < MAPN; x++) dist[y][x] = -1;
    int qh = 0, qt = 0, nc = 0;
    dist[sy][sx] = 0; qx[qt] = sx; qy[qt] = sy; qt++;
    while (qh < qt) {
        int x = qx[qh], y = qy[qh]; qh++;
        int d = dist[y][x];
        if (d >= dmin && d <= dmax && G.map[y][x].kind == K_FLOOR && !occupied(x, y, NULL) && !in_band(y)
            && (!need_hidden || !line_clear(G.px, G.py, x + 0.5f, y + 0.5f))) cand[nc++] = y * MAPN + x;
        if (d >= dmax) continue;
        for (int k = 0; k < 4; k++) {
            int nx = x + DX[k], ny = y + DY[k];
            if (!passable(nx, ny) || dist[ny][nx] >= 0) continue;
            dist[ny][nx] = d + 1; qx[qt] = nx; qy[qt] = ny; qt++;
        }
    }
    if (!nc) return 0;
    int c = cand[rnd(nc)];
    *ox = c % MAPN; *oy = c / MAPN;
    return 1;
}

static void pop_readout(void) {                                    /* in from the bottom; if it is up already, it stays */
    if (G.readout <= 0.0f || G.readout > READOUT_T) G.readout = READOUT_T;
    else if (G.readout < READOUT_T - 0.25f) G.readout = READOUT_T - 0.25f;
}
static void show_bar(void) { G.bar_t = BAR_T; }

#ifdef CRAWL_DEBUG
static int hits_bone, hits_bat, throws, shrieks, hits_fire, hits_sword, bites, casts[NATK], defs[NDEF], bfs_kills, dodges, wskull_wakes, spider_drops,
           grabs, hound_bites, wraith_touches, wraith_drives, leaps, knight_blows, brood_bites, ling_bites, sacs_laid,
           shade_touches, embers, ember_hits, ember_burns, ember_scorch, raised, garg_wakes, mirrored, turned,
           exq_shatter, exq_bruise, exq_reaped, exq_mendward, exq_dread, exq_mirror,   /* Exquisite runes' second effects */
           knight_turned, turned_hits,                                                /* the Ill-Fated Knight's Mirror Rune */
           spider_mends, spider_ups, webbed, web_torn, egg_shots,                     /* the Wandering Spider */
           ward_broken, ward_grown, chant_broken, litany_hits, litany_missed,         /* the Black Deacon */
           brazier_out, brazier_lit, wraith_slips, wraithwalks;                       /* the Wraith Unbound */
#define DBG_COUNT(v) ((v)++, fprintf(stderr, "EVENT %s t=%.2f\n", #v, G.t))
#else
#define DBG_COUNT(v) ((void)0)
#endif

static int in_combat(void);

/* The caster takes a blow. Shadow robes may make it miss, a ward soaks it up, armor blunts it.
 * Returns what got through. (sx, sy): where it came from, for the portrait's flinch. */
static void ward_burst(void);
static int hurt_ex(float dmg, int kind, float sx, float sy) {
    if (G.dead || G.ending || G.exiting || G.tally || G.shop || G.level_t < 1.5) return 0;
    const Gear *g = &RUN.gear;
    int seeping = kind == HURT_POISON || kind == HURT_VOID;        /* nothing turns these: no dodge, armour or ward */
    if (!seeping) {
        if (kind != HURT_STONE && rndf() < gear_dodge(g)) { snd_sfx_ex(SFX_WHOOSH, 0.5f, 0.5f); DBG_COUNT(dodges); return 0; }   /* it passed through a shadow */
        if (kind == HURT_MAGIC) dmg *= 1.0f - gear_magic_resist(g);
        dmg -= kind == HURT_MAGIC ? gear_armor(g) * 0.5f : (float)gear_armor(g);
        if (dmg < 1.0f) dmg = 1.0f;
        if (G.ward_hp > 0.0f) {                                    /* the bone ward takes it */
            float ab = fminf(G.ward_hp, dmg);
            G.ward_hp -= ab; dmg -= ab;
            sfx_at(SFX_BONES, sx, sy, 0.5f);
            if (G.ward_hp <= 0.0f) { G.ward_t = 0.0f; snd_sfx_ex(SFX_SHATTER, 0.8f, 0.5f); if (G.ward_shards > 0.0f) ward_burst(); }
            if (dmg < 0.5f) return 0;
        }
    }
    int d = (int)(dmg + 0.5f);
    if (d < 1) d = 1;
    G.hp -= d;
    if (!seeping) {
        G.hurt = 1.0f;
        G.kick = (rnd(2) ? 1.0f : -1.0f) * 0.07f;
        float lat = (sx - G.px) * -sinf(G.ang) + (sy - G.py) * cosf(G.ang);
        G.face_side = fabsf(lat) < 0.3f ? 0 : (lat < 0.0f ? -1 : 1);
        G.face_t = FACE_T;
    } else G.hurt = fmaxf(G.hurt, 0.25f);
    pop_readout(); show_bar();
    if (G.hp > 0) { if (!seeping) snd_sfx(SFX_HURT); return d; }
    G.hp = 0; G.dead = 1; G.dead_t = 0.0; G.retries++;
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT died level %d t=%.1f\n", G.level, G.level_t);
#endif
    G.hand = 0.0f; G.spd = 0.0f; G.face_t = 1e9f;
    snd_sfx(SFX_DEATH);
    snd_set_boss(0, 0);
    return d;
}

static void throw_missile(const Mob *m, int kind) {
    for (int i = 0; i < MAXBONES; i++) {
        Bone *b = &G.bones[i];
        if (b->alive) continue;
        float sp = kind == MIS_FIRE ? FIRE_SPEED : (kind == MIS_EMBER ? EMBER_SPEED : BONE_SPEED);
        float tx = G.px, ty = G.py, tt = hypotf(tx - m->x, ty - m->y) / sp;
        if (kind != MIS_FIRE) { tx += G.pvx * tt * 0.6f; ty += G.pvy * tt * 0.6f; }   /* leads the caster a little */
        float far = fmaxf(0.0f, hypotf(G.px - m->x, G.py - m->y) - 4.0f);   /* across a great hall a bone goes wide */
        float a = atan2f(ty - m->y, tx - m->x) + (rndf() - 0.5f) * (kind == MIS_FIRE ? 0.05f : 0.14f + 0.05f * far);
        *b = (Bone){ 1, kind, m->x + cosf(a) * 0.3f, m->y + sinf(a) * 0.3f, cosf(a) * sp, sinf(a) * sp, 4.0f, rndf() * 4.0f,
                     kind == MIS_FIRE ? 2.0f * mob_damage(m) : (kind == MIS_EMBER ? 0.8f * mob_damage(m) : mob_damage(m)), 0.0f, (int)(m - G.mobs), 0 };
        if (kind == MIS_EMBER) b->life = b->life0 = fminf(4.0f, hypotf(tx - m->x, ty - m->y) / sp);   /* lobbed to where the caster will be */
        sfx_at(kind == MIS_BONE ? SFX_THROW : SFX_CAST, m->x, m->y, kind == MIS_EMBER ? 0.7f : 1.0f);
        return;
    }
}
static void throw_bone(const Mob *m) { throw_missile(m, MIS_BONE); }

static void explode(float x, float y);
static Shot *new_shot(int kind, float x, float y, float a, float speed, float dmg);
static void say(const char *fmt, ...);

static void ember_land(const Bone *b) {
    for (int i = 0; i < MAXBURNS; i++)
        if (!G.burns[i].alive) { G.burns[i] = (Burn){ 1, b->x, b->y, 0.0f, b->dmg * 0.8f, 1 }; break; }
    sfx_at(SFX_EXPLODE, b->x, b->y, 0.4f);
    DBG_COUNT(ember_burns);
}

static void update_bones(float dt) {
    for (int i = 0; i < MAXBONES; i++) {
        Bone *b = &G.bones[i];
        if (!b->alive) continue;
        float k = G.slip_t > 0.0f && hypotf(b->x - G.px, b->y - G.py) < 7.0f ? 0.3f : 1.0f;   /* Time Slip */
        b->life -= dt; b->spin += dt * 14.0f * k;
        for (int s = 0; s < 4 && b->alive; s++) {
            float nx = b->x + b->vx * dt * k / 4, ny = b->y + b->vy * dt * k / 4;
            if (blocks_light((int)nx, (int)ny) || cell((int)nx, (int)ny)->kind == K_PILLAR) {
                b->alive = 0;
                if (b->kind == MIS_FIRE || (b->kind == MIS_TURNED && (b->atk == AT_FIRE || b->atk == AT_HELL))) explode(b->x, b->y);
                else if (b->kind == MIS_EMBER) ember_land(b);
                else if (b->kind == MIS_TURNED) sfx_at(b->atk == AT_FROST ? SFX_FROST : SFX_ZAP, b->x, b->y, 0.5f);
                else if (b->kind == MIS_WEB) sfx_at(SFX_DIRT, b->x, b->y, 0.4f);
                else sfx_at(SFX_BONES, b->x, b->y, 0.4f);
                break;
            }
            b->x = nx; b->y = ny;
            float d = hypotf(b->x - G.px, b->y - G.py);
            if (G.mirror_t > 0.0f && d < 0.7f && !G.dead) {           /* the Mirror Veil sends it back */
                b->alive = 0;
                int exq = RUN.gear.def[DF_MIRROR] >= RUNE_EXQ;      /* Exquisite: home to its thrower, twice as hard */
                Shot *sh = new_shot(SHOT_REFLECT, b->x, b->y, atan2f(-b->vy, -b->vx), b->kind == MIS_FIRE ? 6.0f : 7.0f, b->dmg * (exq ? 3.0f : 1.5f));
                if (sh) { sh->z = (float)b->kind; if (exq) { sh->target = b->from; sh->life = 3.0f; DBG_COUNT(exq_mirror); } }
                snd_sfx_ex(SFX_SHIMMER, 0.35f, 0.5f);
                break;
            }
            if (!G.dead && d < 0.36f && rndf() * 100.0f < gear_ability(&RUN.gear, AB_MIRRORED, 0)) {   /* Mirrored: back to its thrower */
                b->alive = 0;
                Shot *sh = new_shot(SHOT_REFLECT, b->x, b->y, atan2f(-b->vy, -b->vx), b->kind == MIS_FIRE ? 6.0f : 7.0f,
                                    b->dmg * gear_ability(&RUN.gear, AB_MIRRORED, 1) / 100.0f);
                if (sh) { sh->z = (float)b->kind; sh->target = b->from; sh->life = 3.0f; }
                snd_sfx_ex(SFX_SHIMMER, 0.5f, 0.5f); G.mirror_flash = 0.35f;
                DBG_COUNT(mirrored);
                break;
            }
            if (!G.dead && d < 0.36f) {
                b->alive = 0;
                if (b->kind == MIS_FIRE) { hurt_ex(b->dmg, HURT_MAGIC, b->x - b->vx, b->y - b->vy); explode(b->x, b->y); DBG_COUNT(hits_fire); }
                else if (b->kind == MIS_EMBER) { hurt_ex(b->dmg, HURT_MAGIC, b->x - b->vx, b->y - b->vy); ember_land(b); DBG_COUNT(ember_hits); }
                else if (b->kind == MIS_WEB) {                     /* held fast */
                    G.web_max = G.web_hp = (float)G.max_hp / 3.0f; G.web_idle = 0.0f;
                    snd_sfx_ex(SFX_DIRT, 0.9f, 0.5f); say("CAUGHT IN THE WEB: CAST TO TEAR IT");
                    DBG_COUNT(webbed);
                }
                else if (b->kind == MIS_TURNED) {                  /* your own spell, at half strength */
                    hurt_ex(b->dmg, b->atk == AT_SPEAR ? HURT_BLOW : HURT_MAGIC, b->x - b->vx, b->y - b->vy);
                    if (b->atk == AT_FIRE || b->atk == AT_HELL) explode(b->x, b->y); else sfx_at(b->atk == AT_FROST ? SFX_FROST : SFX_ZAP, b->x, b->y, 0.6f);
                    DBG_COUNT(turned_hits);
                }
                else { hurt_ex(b->dmg, HURT_BLOW, b->x - b->vx, b->y - b->vy); DBG_COUNT(hits_bone); }
            }
        }
        if (b->life <= 0.0f && b->alive) { if (b->kind == MIS_EMBER) ember_land(b); b->alive = 0; }
    }
}

/* souls: one or more a kill, by its rank, three times over while Soul Reaping runs */
static void gain_souls(float n) {
    float mult = G.reap_t > 0.0f ? 3.0f : 1.0f;
    RUN.soul_frac += n * mult;
    int whole = (int)RUN.soul_frac;
    if (whole > 0) { RUN.souls += whole; RUN.soul_frac -= (float)whole; G.level_souls += whole; snd_sfx_ex(SFX_GHOST_TICK, 0.5f, 0.5f); if (G.player) pop_readout(); }
}

/* Spirit is the souls, spent on spells to the tenth (PLAN-items: spirit and bones). Spending never
 * touches what the level has given (G.level_souls), so the wisp still comes after ten gained. */
static float spirit(void) { return (float)RUN.souls + RUN.soul_frac; }
static void spirit_text(char *buf, int n) {                         /* to the tenth, while it fits */
    if (RUN.souls < 1000) snprintf(buf, (size_t)n, "%d.%d", RUN.souls, clampi((int)(RUN.soul_frac * 10.0f), 0, 9));
    else snprintf(buf, (size_t)n, "%d", RUN.souls);
}
static int spend_spirit(float c) {
    if (c <= 0.0f) return 1;
    if (spirit() + 1e-4f < c) return 0;
    RUN.soul_frac -= c;
    while (RUN.soul_frac < -1e-5f) { RUN.soul_frac += 1.0f; RUN.souls--; }
    if (RUN.soul_frac < 0.0f) RUN.soul_frac = 0.0f;
    if (G.player) pop_readout();                                    /* a person sees it go */
    return 1;
}

/* ---- the record and its achievements (PLAN-bestiary, section 11): the autopilot earns them too ---- */

enum { ACH_SECRET0 = REC_KINDS * NRANK, ACH_LEVEL0 = ACH_SECRET0 + 16, ACH_DESCENT0 = ACH_LEVEL0 + 20,   /* bits: slayers by kind and rank, then the rest; */
       ACH_LEVEL21 = 256, ACH_BOSS0 = ACH_LEVEL21 + 20 };       /* from version 6: levels 21 to 40, then the bosses */
#define NSECRET_STEPS 7
static const int SECRET_STEP[NSECRET_STEPS] = { 1, 5, 10, 20, 30, 40, 50 };
static const char *const SECRET_NAME[NSECRET_STEPS] = { "A HOLLOW KNOCK", "LISTENING WALLS", "THE STONE GIVES WAY", "NOTHING STAYS HIDDEN",
                                                        "THE BUILDERS' TRICKS", "EVERY LAST CRACK", "KEEPER OF SECRETS" };
#define MAX_DESCENT 20                                              /* level 800: as far as anyone will go */
static const char *const ORDINAL[MAX_DESCENT + 1] = { "", "FIRST", "SECOND", "THIRD", "FOURTH", "FIFTH", "SIXTH", "SEVENTH", "EIGHTH", "NINTH",
    "TENTH", "ELEVENTH", "TWELFTH", "THIRTEENTH", "FOURTEENTH", "FIFTEENTH", "SIXTEENTH", "SEVENTEENTH", "EIGHTEENTH", "NINETEENTH", "TWENTIETH" };

/* how many of a kind in a rank make an achievement: 60 of the lowest rank it comes in, fewer up the
 * ladder, 3 gilded; 0 if it never comes in that rank (or can't be killed) */
static int slayer_need(int kind, int rank) {
    const Kind *k = &KINDS[kind];
    if (k->flags & (KF_SPIRIT | KF_UNCOUNTED)) return 0;
    if (k->family < 0) return rank == RK_I ? 60 : 0;               /* off the ladder: one rank, sixty */
    if (rank == RK_GILDED) return k->gild ? 3 : 0;
    int low = crawl_set(k->debut) == 3 ? RK_II : RK_I;             /* the Deep has no rank I */
    static const int NEED[5] = { 60, 40, 25, 15, 8 };
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
    if (!TOASTS.n || boss_fighting()) return;                      /* they wait out a boss's fight */
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
    if (m->last_atk >= 0 && r->spell_kills[m->type][m->last_atk] < 255) r->spell_kills[m->type][m->last_atk]++;
    int need = slayer_need(m->type, m->rank), have = r->kills[m->type][m->rank], bit = m->type * NRANK + m->rank;
    if (!need || ach_has(bit)) return;
    char title[32], desc[32];
    snprintf(title, sizeof title, "%s%s", rank_word(m->type, m->rank), KINDS[m->type].plural);
    if (have >= need) { snprintf(desc, sizeof desc, "%d SLAIN", need); ach_earn(bit, KINDS[m->type].spr, KINDS[m->type].family, m->rank, title, desc); }
    else if (need >= 8 && have == need / 2) { snprintf(desc, sizeof desc, "%d OF %d SLAIN", have, need); toast(KINDS[m->type].spr, KINDS[m->type].family, m->rank, 1, title, desc); }
}
static void rec_seen(const Mob *m) {
    if (m->type >= REC_KINDS || m->boss) return;
    RUN.rec.seen[m->type] |= (uint8_t)(1 << m->rank);
    if (m->rank < RK_GILDED && RUN.top_rank[m->type] < m->rank + 1) RUN.top_rank[m->type] = (uint8_t)(m->rank + 1);   /* for a boss's adds */
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
static void rec_cleared(void) {
    Record *r = &RUN.rec;
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
}

static Item *add_item(int x, int y, int type);
static void add_pool(float x, float y);
static void wraith_back(Mob *m);
static void kill_mob(Mob *m) {
    m->state = MS_DIE; m->t = 0.0f;
    if (!KIND_IS(m, KF_UNCOUNTED)) { if (!PK.depth) G.kills++; if (!m->boss) rec_kill(m); }   /* a pocket's are in the record, not the level's tally */
    if (PK.depth && m->gen > 0 && m->gen <= 32) PK.slain[PK.chain][clampi(PK.depth, 0, POCKET_LINKS - 1)] |= 1u << (m->gen - 1);
    sfx_at(KINDS[m->type].sfx_die, m->x, m->y, 1.0f);
    if (m->type == MOB_SPIDER && m->z > 0.3f) m->sub = 9;           /* it falls from the web */
    if (m->type == MOB_DROWNED) { add_pool(m->x, m->y); sfx_at(SFX_EXPLODE, m->x, m->y, 0.6f); }   /* it bursts */
    if (m->type == MOB_BROOD) {                                    /* her sacs die with the last of the mothers */
        int others = 0;
        for (int i = 0; i < MAXMOBS; i++) others += G.mobs[i].alive && G.mobs[i].state == MS_WALK && G.mobs[i].type == MOB_BROOD && &G.mobs[i] != m;
        if (!others) for (int i = 0; i < MAXMOBS; i++) if (G.mobs[i].alive && G.mobs[i].type == MOB_SAC && G.mobs[i].state == MS_WALK) { G.mobs[i].state = MS_DIE; G.mobs[i].t = 0.0f; }
    }
    if (m->boss) return;                                           /* a boss leaves only its scrip */
    gain_souls((float)RANK_SOULS[m->rank]);
    if (G.reap_t > 0.0f && RUN.gear.def[DF_REAP] >= RUNE_EXQ && !G.dead && !KIND_IS(m, KF_UNCOUNTED)) {   /* Exquisite: a little life back */
        G.hp = clampi(G.hp + (G.max_hp + 19) / 20, 0, G.max_hp);
        DBG_COUNT(exq_reaped);
    }
    /* what it leaves: the gilded always something good, the higher ranks now and then coin or a potion.
     * Like a closet's loot it isn't counted in the level's treasure, and it glints a little */
    int drop = -1;
    if (m->add) ;                                                  /* a boss's adds leave no treasure, nor potions */
    else if (m->rank == RK_GILDED) drop = rnd(3) ? IT_CROWN : IT_CHEST;
    else if (m->rank >= RK_IV && rnd(10) == 0) drop = IT_POTION;
    else if (m->rank >= RK_II && rndf() < 0.08f * (float)m->rank) drop = m->rank >= RK_IV ? IT_GEMS : IT_COINS;
    if (drop >= 0 && G.map[m->cy][m->cx].kind == K_FLOOR) {
        Item *it = add_item(m->cx, m->cy, drop);
        if (it) {
            it->secret = 1; it->seen = 1;
            if (drop < IT_KEY) { G.items_total--; G.value_total -= treasure_value(drop); }
        }
    }
    /* the bony ones leave a bone for the spear now and then: about one in eight, more at higher ranks */
    int bony = m->type == MOB_SKEL || m->type == MOB_SHIELD || m->type == MOB_KNIGHT || m->type == MOB_HOUND;
    if (bony && G.map[m->cy][m->cx].kind == K_FLOOR && rndf() < 0.125f + 0.03f * (float)m->rank) {
        Item *it = add_item(m->cx, m->cy, IT_BONE);
        if (it) { it->secret = 1; it->seen = 1; }
    }
}

/* the knight's plate comes off as it is hurt: 0 whole, 1 half, 2 down to the breastplate */
static int knight_stage(const Mob *m) {
    if (m->boss) return G.bwave <= 0 ? 0 : (G.bwave <= 2 ? 1 : 2);   /* the Ill-Fated Knight sheds plate as its waves come */
    return m->hp > m->maxhp * 0.66f ? 0 : (m->hp > m->maxhp * 0.33f ? 1 : 2);
}
static float knight_pace(const Mob *m, int st) {                   /* quicker as the plate comes off */
    if (m->boss) return KINDS[MOB_KNIGHT].speed * RANK_SPEED[m->rank] * BOSSES[m->boss - 1].speed * (1.0f + 0.15f * st);
    return KINDS[MOB_KNIGHT].speed * RANK_SPEED[m->rank] * (1.0f + 0.25f * st);
}

/* How much a spell bites a creature: its weakness to the spell, and each element worn added in by how
 * much that element hurts it (fire as Fire Bolt does, frost as Frost Shard, lightning as Chain
 * Lightning). A knight down to its breastplate has nothing left to keep fire off. */
static float weak_to(const Mob *m, int atk) {
    if (m->type == MOB_KNIGHT && atk == AT_FIRE && knight_stage(m) == 2) return 1.0f;
    if (m->type == MOB_WRAITH && m->boss) return atk == AT_FIRE || atk == AT_HELL ? 1.2f : 1.0f;   /* the Wraith Unbound, in light */
    return KINDS[m->type].weak[atk];
}
static float hit_mult(const Mob *m, int atk) {
    const Gear *g = &RUN.gear;
    float k = weak_to(m, atk);
    if (atk == AT_BFS) return k;
    return k + gear_total(g, ST_FIRE) / 100.0f * weak_to(m, AT_FIRE) + gear_total(g, ST_FROST) / 100.0f * weak_to(m, AT_FROST)
             + gear_total(g, ST_SHOCK) / 100.0f * weak_to(m, AT_CHAIN);
}
static float slow_factor(void) {                                   /* slowed, the caster walks at 60%; resisted, less slowly; */
    if (G.web_hp > 0.0f) return 0.0f;                              /* held in the Wandering Spider's web: not at all */
    return (G.slow_t > 0.0f ? 1.0f - 0.4f * (1.0f - 0.5f * gear_slow_resist(&RUN.gear)) : 1.0f) * (G.hz_slow > 0.0f ? G.hz_slow : 1.0f);   /* bile and void besides */
}
static float spell_effect(void) { return 1.0f + gear_total(&RUN.gear, ST_EFFECT) / 100.0f; }   /* all a spell does besides damage */

/* A hit on a creature, from (fx, fy) (shields only stop what comes from in front). Returns 1 if it died. */
static int boss_hurt(Mob *m, float dmg);
static int wraith_open(const Mob *m);
static int damage_mob(Mob *m, float dmg, int flags, float fx, float fy, float slow) {
    if (m->boss && m->alive && m->state == MS_WALK && dmg > 0.0f) {   /* a boss: its weaknesses, then its marks */
        if (G.bdash_t > 0.0f) return 0;                            /* darting: nothing touches it */
        if (m->type == MOB_HUNTER && (m->sub == 0 || m->sub == 5)) return 0;   /* overhead, out of reach */
        if (!wraith_open(m)) return 0;                             /* the Wraith: in the dark, through it */
        int a = (flags >> 8) - 1;
        if (a >= 0 && a < NATK) { dmg *= hit_mult(m, a); m->last_atk = a; }
        if ((flags & DM_SLOW) && slow > 0.0f) m->slowed = fmaxf(m->slowed, slow * 0.5f);
        m->misses = 0;
        if (m->type == MOB_DEACON && G.bstate == BS_FIGHT && G.bward > 0.0f) {   /* the Bone Ward takes the first blows */
            float ab = fminf(G.bward, dmg);
            G.bward -= ab; dmg -= ab;
            if (G.bward <= 0.0f) { G.bward = 0.0f; G.bward_cd = def_cooldown(Q_RARE + 1); sfx_at(SFX_SHATTER, m->x, m->y, 1.3f); DBG_COUNT(ward_broken); }
            if (dmg <= 0.0f) { sfx_at(SFX_BONES, m->x, m->y, 0.5f); return 0; }
        }
        if (m->type == MOB_DEACON && G.bstate == BS_FIGHT && m->swing > 0.0f && (G.bchant_dmg += dmg) >= 0.1f * m->maxhp) {
            m->swing = 0.0f; m->deco = -1; m->contact = 3.0f;         /* a tenth of his health mid-chant breaks it off */
            sfx_at(SFX_GROAN, m->x, m->y, 1.0f);
            DBG_COUNT(chant_broken);
        }
        if (G.bstate == BS_FIGHT) G.bhurt += dmg;
        return boss_hurt(m, dmg);
    }
    if (m->type == MOB_WRAITH && m->alive && m->state == MS_WALK) {   /* fire won't kill it, but it drives it back */
        int a = (flags >> 8) - 1;
        if (a == AT_FIRE || a == AT_HELL) wraith_back(m);
        m->misses = 0;
        return 0;
    }
    if (!m->alive || m->state != MS_WALK || KIND_IS(m, KF_SPIRIT) || dmg <= 0.0f || buried(m)) return 0;
    if (m->type == MOB_SHIELD && m->shield > 0.0f && !(flags & (DM_PIERCE | DM_ARC))) {
        float a = atan2f(fy - m->y, fx - m->x);
        if (fabsf(wrap_pi(a - m->face)) < 1.25f) {
            float ab = fminf(m->shield, dmg);
            m->shield -= ab; dmg -= ab;
            if (m->shield <= 0.0f) sfx_at(SFX_SHATTER, m->x, m->y, 1.0f);   /* splinters */
            else sfx_at(SFX_STEP, m->x, m->y, 1.4f);                       /* a thud on the wood */
            m->misses = 0;                                         /* a blow on the shield still counts */
            if (dmg <= 0.0f) { m->stagger = fmaxf(m->stagger, 0.2f); return 0; }
        }
    }
    if ((flags & DM_SLOW) && slow > 0.0f) m->slowed = fmaxf(m->slowed, slow);
    int atk = (flags >> 8) - 1;
    if (atk >= 0 && atk < NATK) { dmg *= hit_mult(m, atk); m->last_atk = atk; }   /* what it is weak to, or shrugs off */
    if (m->frozen > 0.0f && RUN.gear.def[DF_NOVA] >= RUNE_EXQ) { dmg *= 2.0f; DBG_COUNT(exq_shatter); }   /* Exquisite runes: the frozen */
    if (G.slip_t > 0.0f && RUN.gear.def[DF_SLIP] >= RUNE_EXQ && hypotf(m->x - G.px, m->y - G.py) < 7.0f) { dmg *= 1.25f; DBG_COUNT(exq_bruise); }   /* shatter, the slowed bruise */
    if (m->type == MOB_GHOUL && m->sub == 4) { dmg *= 2.0f; m->sub = 0; }   /* caught feeding */
    if (m->type == MOB_GARGOYLE) garg_wake(m);                     /* it was never a statue */
    if (m->type == MOB_HUNTER && m->sub == 0) { m->sub = 1; m->sub_t = 0.0f; }   /* hit, it drops from the ceiling */
    m->misses = 0;
    m->alert = fmaxf(m->alert, 12.0f);                             /* hurt, it knows where you are */
    if (m->rank == RK_GILDED && m->hp > m->maxhp * 0.5f && m->hp - dmg <= m->maxhp * 0.5f) { m->feared = 6.0f; m->wfeared = 0; }   /* the gilded run, with their gold */
    m->hp -= dmg;
    if (m->hp <= 0.0f) { kill_mob(m); return 1; }
    if (m->type == MOB_SKEL || m->type == MOB_SHIELD) {            /* knocked back a step */
        m->stagger = 0.6f; m->throw_t = 0.0f; m->swing = 0.0f; m->cool = fmaxf(m->cool, 0.9f);
        sfx_at(SFX_BONES, m->x, m->y, 0.45f);
    }
    if (m->type == MOB_SPIDER && m->sub == 0) { m->sub = 1; m->sub_t = 0.0f; }   /* disturbed: it comes down */
    return 0;
}

static void thorns(Mob *m) {
    int t = gear_thorns(&RUN.gear);
    if (t > 0 && m->alive) damage_mob(m, (float)t, DM_ARC, G.px, G.py, 0.0f);
}

/* an Exquisite Bone Ward, broken: its shards fly out and cut everything within two cells */
static void ward_burst(void) {
    float dmg = G.ward_shards;
    G.ward_shards = 0.0f;
    for (int i = 0; i < MAXMOBS; i++) {
        Mob *m = &G.mobs[i];
        if (!m->alive || m->state != MS_WALK || KIND_IS(m, KF_SPIRIT) || buried(m)) continue;
        if (hypotf(m->x - G.px, m->y - G.py) < 2.0f && line_clear(G.px, G.py, m->x, m->y)) damage_mob(m, dmg, DM_ARC, G.px, G.py, 0.0f);
    }
    for (int k = 0; k < 6; k++) {                                   /* to be seen going; they touch nothing */
        Shot *sh = new_shot(SHOT_REFLECT, G.px, G.py, G.ang + 0.3f + (float)k * (2.0f * PI_F / 6.0f), 7.0f, 0.0f);
        if (sh) { sh->tier = -1; sh->life = 0.3f; }
    }
    sfx_at(SFX_BONES, G.px, G.py, 1.4f);
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT ward_burst %.0f t=%.2f\n", dmg, G.t);
#endif
}

static void explode(float x, float y) {
    for (int i = 0; i < MAXBOOMS; i++)
        if (!G.booms[i].alive) { G.booms[i] = (Boom){ 1, x, y, 0.0f, 1.0f, 0 }; break; }
    sfx_at(SFX_EXPLODE, x, y, 1.0f);
}
static void explode_big(float x, float y, float r) {
    for (int i = 0; i < MAXBOOMS; i++)
        if (!G.booms[i].alive) { G.booms[i] = (Boom){ 1, x, y, 0.0f, r, 1 }; break; }
    sfx_at(SFX_EXPLODE, x, y, 1.4f); sfx_at(SFX_BOOM, x, y, 0.6f);
}

/* everything within r of (x, y) takes dmg, but not `skip` (it took the direct hit) */
static void splash(float x, float y, float r, float dmg, const Mob *skip, int atk) {
    for (int i = 0; i < MAXMOBS; i++) {
        Mob *m = &G.mobs[i];
        if (m == skip || !m->alive || m->state != MS_WALK || KIND_IS(m, KF_SPIRIT)) continue;
        if (hypotf(m->x - x, m->y - y) < r && line_clear(x, y, m->x, m->y)) damage_mob(m, dmg, DM_ARC | DM_ATK(atk), x, y, 0.0f);
    }
}

/* Wraiths drift through stone towards the caster, looming, until it comes close; then they recoil
 * with a shriek and flee back through the walls, fading as they go. Nothing can touch them, and
 * they touch nothing. */
static void update_ghost(Mob *m, float dt) {
    m->t += dt; m->st += dt; m->anim += dt * 1.6f;
    float dx = G.px - m->x, dy = G.py - m->y, d = sqrtf(dx * dx + dy * dy) + 1e-4f;
    m->z = 0.10f + 0.06f * sinf(m->t * 1.7f);
    if (m->state == MS_WALK) {
        float sp = (d > 6.0f ? 0.75f : 0.5f) * (G.dead || G.exiting ? 0.0f : 1.0f);
        float weave = sinf(m->t * 0.6f + m->orbit) * 0.4f;
        m->x += (dx / d * sp - dy / d * weave) * dt;
        m->y += (dy / d * sp + dx / d * weave) * dt;
        if ((d < 2.1f && line_clear(m->x, m->y, G.px, G.py)) || d < 1.2f) {
            m->state = MS_RECOIL; m->st = 0.0f;
            sfx_at(SFX_SHRIEK, m->x, m->y, 1.4f);
            G.chill = 1.0f;
            DBG_COUNT(shrieks);
        } else if (m->st > 30.0f) { m->state = MS_FLEE; m->st = 0.0f; }   /* tired of waiting: melts away */
        if (d < 6.0f && rnd(500) == 0) sfx_at(SFX_MOAN, m->x, m->y, 0.9f);
    } else if (m->state == MS_RECOIL) {
        m->x -= dx / d * 0.6f * dt; m->y -= dy / d * 0.6f * dt;     /* reeling back */
        if (m->st > 0.55f) { m->state = MS_FLEE; m->st = 0.0f; }
    } else {                                                       /* fleeing, through whatever is in the way */
        m->x -= dx / d * 4.0f * dt; m->y -= dy / d * 4.0f * dt;
        m->fade += dt / 0.9f;
        if (m->fade >= 1.0f) m->alive = 0;
    }
    m->x = clampf(m->x, 0.5f, MAPN - 0.5f); m->y = clampf(m->y, 0.5f, MAPN - 0.5f);
    m->cx = m->tx = (int)m->x; m->cy = m->ty = (int)m->y;
}

/* The hunt: steps from every cell to the caster, rebuilt as it moves, which creatures that know where
 * it is follow round corners and through doors (a closed door counts: they open it; a locked one
 * doesn't). -1 where it is too far, or can't be walked. */
#define HUNT_FAR 64
static int16_t hunt[MAPN][MAPN];
static int hunt_cx = -1, hunt_cy = -1;
static float hunt_t;
static int hunt_pass(int x, int y) {
    const Cell *c = cell(x, y);
    if (c->kind == K_FLOOR) return !(G.slide >= 0 && slide_covers(x, y));
    return c->kind == K_DOOR && G.doors[c->door].locked != 1 && G.doors[c->door].locked != SEALED;
}
static void build_hunt(float dt) {
    int cx = (int)G.px, cy = (int)G.py;
    hunt_t -= dt;
    if (cx == hunt_cx && cy == hunt_cy && hunt_t > 0.0f) return;   /* doors and walls move too: now and then, anyway */
    hunt_cx = cx; hunt_cy = cy; hunt_t = 0.5f;
    static int q[MAPN * MAPN];
    memset(hunt, 0xff, sizeof hunt);
    int qh = 0, qt = 0;
    hunt[cy][cx] = 0; q[qt++] = cy * MAPN + cx;
    while (qh < qt) {
        int x = q[qh] % MAPN, y = q[qh] / MAPN; qh++;
        if (hunt[y][x] >= HUNT_FAR) continue;
        for (int d = 0; d < 4; d++) {
            int nx = x + DX[d], ny = y + DY[d];
            if (nx < 1 || ny < 1 || nx >= MAPN - 1 || ny >= MAPN - 1 || hunt[ny][nx] >= 0 || !hunt_pass(nx, ny)) continue;
            hunt[ny][nx] = (int16_t)(hunt[y][x] + 1); q[qt++] = ny * MAPN + nx;
        }
    }
}
/* only the nearest few hunt at once, so the whole level doesn't arrive together */
#define MAX_HUNTERS 3
static void pick_hunters(void) {
    int best[MAX_HUNTERS], bd[MAX_HUNTERS], n = 0;
    for (int i = 0; i < MAXMOBS; i++) {
        Mob *m = &G.mobs[i];
        m->hunts = m->type == MOB_LING;                            /* a swarm comes all at once */
        if (!m->alive || m->state != MS_WALK || m->alert <= 0.0f || m->type == MOB_LING) continue;
        int h = hunt[m->cy][m->cx];
        if (h < 0) continue;
        int k = n < MAX_HUNTERS ? n++ : MAX_HUNTERS;
        if (k == MAX_HUNTERS) { if (h >= bd[MAX_HUNTERS - 1]) continue; k = MAX_HUNTERS - 1; }
        while (k > 0 && bd[k - 1] > h) { best[k] = best[k - 1]; bd[k] = bd[k - 1]; k--; }
        best[k] = i; bd[k] = h;
    }
    for (int i = 0; i < n; i++) G.mobs[best[i]].hunts = 1;
    for (int i = 0; i < MAXMOBS; i++) if (G.mobs[i].alive && (G.mobs[i].boss || G.mobs[i].add)) G.mobs[i].hunts = 1;   /* a boss's fight is all at once */
}

/* a noise: whatever is within so many steps knows where the caster is now */
static void alert_near(int steps, float secs) {
    for (int i = 0; i < MAXMOBS; i++) {
        Mob *m = &G.mobs[i];
        if (!m->alive || m->state != MS_WALK) continue;
        int h = hunt[m->cy][m->cx];
        if (h >= 0 && h <= steps) m->alert = fmaxf(m->alert, secs);
    }
}

/* A footstep: heard three steps off down the halls, by anything there that isn't already after you.
 * Quiet boots soften it (80% Quiet: only the next cell hears). */
static void footstep(void) {
    snd_sfx_ex(SFX_STEP, 1.0f, ((int)(G.bobph / PI_F) & 1) ? 0.44f : 0.56f);
    alert_near((int)(3.0f * (1.0f - gear_total(&RUN.gear, ST_QUIET) / 100.0f) + 0.5f), 4.0f);
}

/* how fast a creature's clock runs: frozen stops it, frost slows it, Time Slip slows everything near */
static float mob_time(Mob *m, float dt) {
    if (m->frozen > 0.0f) { m->frozen -= dt; return 0.0f; }
    float k = 1.0f;
    if (m->slowed > 0.0f) { m->slowed -= dt; k *= 0.5f; }
    if (G.slip_t > 0.0f && hypotf(m->x - G.px, m->y - G.py) < 7.0f) k *= 0.3f;
    return dt * k;
}

/* The Wraith: from level 9, once a level, after about four minutes there, from the far end. It walks
 * the halls as you do, solid, down the hunt field, and pulls doors open. It can't be killed: fire,
 * the wisp's light close by, and Dread drive it back for a while; then it comes on again. Its touch
 * drains and chills. Torches gutter as it passes (build_lightmap). */
static void wraith_back(Mob *m) {
    if (m->sub == 1) { m->sub_t = 0.0f; return; }
    m->sub = 1; m->sub_t = 0.0f;
    sfx_at(SFX_SHRIEK, m->x, m->y, 1.3f);
    G.chill = fmaxf(G.chill, 0.4f);
    DBG_COUNT(wraith_drives);
}
static void update_wraith(Mob *m, float dt) {
    m->t += dt;
    dt = mob_time(m, dt);
    if (dt <= 0.0f) return;
    m->anim += dt * 2.0f; m->cool -= dt; m->sub_t += dt;
    float pd = hypotf(G.px - m->x, G.py - m->y);
    if (m->feared > 0.0f) { m->feared = 0.0f; wraith_back(m); }    /* Dread */
    for (int i = 0; i < MAXWISPS && m->sub != 1; i++)              /* a wisp's light, close by */
        if (G.wisps[i].on && G.wisps[i].a > 0.5f && hypotf(G.wisps[i].x - m->x, G.wisps[i].y - m->y) < 3.0f) wraith_back(m);
    if (m->sub == 1 && m->sub_t > 5.0f) m->sub = 0;
    if (m->sub == 0 && !G.dead && pd < 1.1f && m->cool <= 0.0f) {  /* its touch */
        m->cool = 2.0f;
        sfx_at(SFX_SHRIEK, m->x, m->y, 1.0f);
        if (hurt_ex(mob_damage(m) * 1.5f, HURT_MAGIC, m->x, m->y) > 0) {
            G.chill = 1.0f; G.slow_t = fmaxf(G.slow_t, 2.0f * (1.0f - gear_slow_resist(&RUN.gear)));
            DBG_COUNT(wraith_touches);
        }
    }
    if (rnd(900) == 0) sfx_at(SFX_WAIL, m->x, m->y, 0.9f);
    float gx = m->tx + 0.5f, gy = m->ty + 0.5f, dx = gx - m->x, dy = gy - m->y, d = sqrtf(dx * dx + dy * dy);
    float step = m->speed * dt * (m->sub == 1 ? 1.6f : 1.0f);
    if (d > step) { m->x += dx / d * step; m->y += dy / d * step; return; }
    m->x = gx; m->y = gy; m->cx = m->tx; m->cy = m->ty;
    if (G.dead || G.exiting) return;
    int here = hunt[m->cy][m->cx], best = -1; float bs = -1e9f;
    for (int k = 0; k < 4; k++) {
        int nx = m->cx + DX[k], ny = m->cy + DY[k], h = hunt[ny][nx];
        if (h < 0 || occupied(nx, ny, m)) continue;
        if (here >= 0 && (m->sub == 1 ? h <= here : h >= here)) continue;   /* down the field coming, up it going */
        float s = rndf() * 0.3f + (here < 0 ? -hypotf(nx + 0.5f - G.px, ny + 0.5f - G.py) : 0.0f);
        if (s > bs) { bs = s; best = k; }
    }
    if (best < 0) return;
    int nx = m->cx + DX[best], ny = m->cy + DY[best];
    const Cell *c = cell(nx, ny);
    if (c->kind == K_DOOR && G.doors[c->door].open < 0.9f) {        /* a door opening by itself: it is coming */
        Door *dr = &G.doors[c->door];
        if (!dr->opening) { dr->opening = 1; sfx_at(SFX_DOOR, nx + 0.5f, ny + 0.5f, 1.0f); }
        return;
    }
    m->tx = nx; m->ty = ny;
}

/* the buried and the drowned: under the earth of a grave or in a puddle, until you come near */
static int buried(const Mob *m) { return (m->type == MOB_SHAMBLER || m->type == MOB_DROWNED) && m->sub == 1; }
static int update_riser(Mob *m, float dt) {                       /* 1 while it is not yet up and about */
    if (m->state != MS_WALK || !m->sub || (m->type != MOB_SHAMBLER && m->type != MOB_DROWNED)) return 0;
    m->t += dt;
    if (m->sub == 1) {
        float d = hypotf(G.px - m->x, G.py - m->y);
        if (!G.dead && G.level_t > 1.5 && d < (m->type == MOB_DROWNED ? 3.5f : 4.5f) && line_clear(m->x, m->y, G.px, G.py)) {
            m->sub = 2; m->sub_t = 0.0f;
            sfx_at(m->type == MOB_DROWNED ? SFX_DRIP : SFX_DIRT, m->x, m->y, 1.4f); sfx_at(SFX_GROAN, m->x, m->y, 1.0f);
            if (m->deco >= 0) G.decos[m->deco].spr = m->type == MOB_DROWNED ? SP_PUDDLE_A : SP_PIT;   /* the earth opened, the water stilled */
        }
        return 1;
    }
    if ((m->sub_t += dt) >= 1.6f) { m->sub = 0; m->alert = 12.0f; }
    return 1;
}

/* where a drowned one burst: a foul pool that poisons for a few seconds */
static void add_pool(float x, float y) {
    for (int i = 0; i < MAXPOOLS; i++) if (!G.pools[i].alive) { G.pools[i] = (Pool){ 1, x, y, 0.0f }; return; }
}
static void update_pools(float dt) {
    for (int i = 0; i < MAXPOOLS; i++) {
        Pool *p = &G.pools[i];
        if (!p->alive) continue;
        if ((p->t += dt) > 5.0f) { p->alive = 0; continue; }
        if (!G.dead && G.purify_t <= 0.0f && hypotf(G.px - p->x, G.py - p->y) < 0.9f) {
            float k = 1.0f - gear_poison_resist(&RUN.gear);
            G.poison_t = fmaxf(G.poison_t, 2.5f * k);
            G.poison_dps = fmaxf(G.poison_dps, (1.0f + (float)(G.level > 4 ? (G.level - 4) / 2 : 0)) * (0.5f + 0.5f * k));
        }
    }
}

/* The wall skull: asleep until the caster comes close, then a scream and three seconds of fire
 * gathering, then a fireball; again and again while the caster stays near. One hit ends it. */
static void update_wskull(Mob *m, float dt) {
    m->t += dt;
    if (m->state == MS_DIE) {                                      /* falls from the wall and breaks */
        m->z = fmaxf(0.0f, m->z - dt * 1.6f);
        if (m->t >= 0.55f) { add_deco(m->x, m->y, SP_BONES1); m->alive = 0; }
        return;
    }
    dt = mob_time(m, dt);
    m->sub_t += dt;
    if (m->feared > 0.0f) { m->feared -= dt; m->sub = 0; m->sub_t = 0.0f; return; }   /* Dread: it cowers silent */
    float d = hypotf(G.px - m->x, G.py - m->y);
    int seen = !G.dead && !G.exiting && line_clear(m->x, m->y, G.px, G.py);
    switch (m->sub) {
    case 0: {                                                      /* asleep, until the caster walks up to it, eyes on it */
        float vx = (G.px - m->x) / (d + 1e-4f), vy = (G.py - m->y) / (d + 1e-4f);
        int facing = vx * cosf(m->face) + vy * sinf(m->face) > 0.6f && -(vx * cosf(G.ang) + vy * sinf(G.ang)) > 0.8f;
        if (seen && facing && d < 3.0f) {
            m->sub = 1; m->sub_t = 0.0f; sfx_at(SFX_SCREAM, m->x, m->y, 1.5f); DBG_COUNT(wskull_wakes);
#ifdef CRAWL_DEBUG
            fprintf(stderr, "   skull at (%.2f,%.2f) caster (%.2f,%.2f) ang %.2f\n", m->x, m->y, G.px, G.py, G.ang);
#endif
            m->ignore = (float)G.t + 0.4f + 1.2f * rndf() + (rndf() < 0.2f ? 1.6f : 0.0f);   /* startled */
            G.kick = 0.05f; G.chill = fmaxf(G.chill, 0.35f);
        }
        break;
    }
    case 1:                                                        /* the fire gathers: three seconds */
        if (m->sub_t >= 3.0f) {
            if (seen && d < 9.0f) throw_missile(m, MIS_FIRE);
            m->sub = 2; m->sub_t = 0.0f;
        }
        break;
    default:                                                       /* spent; it gathers again if you linger */
        if (m->sub_t >= 3.0f) {
            m->sub = seen && d < 4.5f ? 1 : 0; m->sub_t = 0.0f;   /* awake now: it needs no second look */
            if (m->sub == 1) sfx_at(SFX_SCREAM, m->x, m->y, 0.9f);
        }
        break;
    }
}

/* The spider: hidden in its corner web with only its eyes showing; it drops on its thread when the
 * caster comes within two steps, bites (poison and a slow), and climbs back to wait. */
static void bite(Mob *m) {
    const Gear *g = &RUN.gear;
    sfx_at(SFX_SKITTER, m->x, m->y, 1.4f); snd_sfx_ex(SFX_HURT, 0.7f, 0.5f);
    G.face_t = FACE_T; G.face_side = 0; G.hurt = fmaxf(G.hurt, 0.6f);
    thorns(m);
    DBG_COUNT(bites);
    if (G.purify_t > 0.0f) return;                                  /* Purify's protection */
    float k = 1.0f - gear_poison_resist(g);                        /* resisted, it is shorter and weaker */
    G.poison_t = fmaxf(G.poison_t, 6.0f * k);
    G.poison_dps = (1.0f + (float)((G.level - 4) / 2 > 0 ? (G.level - 4) / 2 : 0)) * m->dmgk * (0.5f + 0.5f * k);
    G.slow_t = fmaxf(G.slow_t, 4.0f * (1.0f - gear_slow_resist(g)));
}

static void update_spider(Mob *m, float dt) {
    m->t += dt;
    if (m->state == MS_DIE) {
        m->z = fmaxf(0.0f, m->z - dt * 1.4f);
        if (m->t >= 0.9f) { add_deco(m->x, m->y, SP_SPIDER_D); m->alive = 0; }
        return;
    }
    dt = mob_time(m, dt);
    m->sub_t += dt; m->cool -= dt;
    if (m->feared > 0.0f) m->feared -= dt;                         /* Exquisite Dread: it keeps to its web */
    float dax = G.px - m->ax, day = G.py - m->ay, da = sqrtf(dax * dax + day * day) + 1e-4f;
    float reach = fminf(da - 0.45f, 0.75f);                         /* how far out from the corner it drops */
    switch (m->sub) {
    case 0:
        m->x = m->ax; m->y = m->ay; m->z = 0.62f;
        if (!G.dead && m->cool <= 0.0f && m->feared <= 0.0f && da < 2.0f && line_clear(m->ax, m->ay, G.px, G.py)) { m->sub = 1; m->sub_t = 0.0f; sfx_at(SFX_SKITTER, m->x, m->y, 1.0f); DBG_COUNT(spider_drops); }
        break;
    case 1: {                                                      /* down the thread */
        float k = fminf(1.0f, m->sub_t / 0.35f);
        m->z = lerpf(0.62f, 0.12f, k);
        m->x = m->ax + dax / da * reach * k; m->y = m->ay + day / da * reach * k;
        if (k >= 1.0f) { m->sub = 2; m->sub_t = 0.0f; }
        break;
    }
    case 2:                                                        /* the bite */
        if (m->sub_t >= 0.25f) {
            if (!G.dead && hypotf(G.px - m->x, G.py - m->y) < 1.05f) bite(m);
            m->sub = 3; m->sub_t = 0.0f;
            m->bx = m->x; m->by = m->y;                             /* where it climbs back from */
        }
        break;
    default: {                                                     /* back up to the web */
        float k = fminf(1.0f, m->sub_t / 1.2f);
        m->z = lerpf(0.12f, 0.62f, k);
        m->x = lerpf(m->bx, m->ax, k); m->y = lerpf(m->by, m->ay, k);
        if (k >= 1.0f) { m->sub = 0; m->cool = 3.0f; }
        break;
    }
    }
    m->cx = m->tx = (int)m->x; m->cy = m->ty = (int)m->y;
}

/* A leap at the caster, for hunting spiders and ghouls: from where it is to just short of where the
 * caster stands, in an arc; a blow, or a bite, if it lands close. */
static void start_leap(Mob *m) {
    float a = atan2f(G.py - m->y, G.px - m->x), d = fmaxf(0.0f, hypotf(G.px - m->x, G.py - m->y) - 0.7f);
    m->sub = 3; m->sub_t = 0.0f;
    m->bx = m->x; m->by = m->y; m->ax = m->x + cosf(a) * d; m->ay = m->y + sinf(a) * d;
    sfx_at(m->type == MOB_GHOUL ? SFX_GROAN : (m->type == MOB_GARGOYLE ? SFX_WHOOSH : SFX_SKITTER), m->x, m->y, 1.2f);
}
static int update_leap(Mob *m, float dt) {
    if (m->sub != 3) return 0;
    m->sub_t += dt;
    int garg = m->type == MOB_GARGOYLE;
    float k = fminf(1.0f, m->sub_t / 0.45f);
    m->x = lerpf(m->bx, m->ax, k); m->y = lerpf(m->by, m->ay, k); m->z = garg ? GARG_Z - 0.1f * sinf(PI_F * k) : 0.35f * sinf(PI_F * k);
    if (k < 1.0f) return 1;
    m->sub = m->type == MOB_HUNTER || garg ? 2 : 0; m->z = garg ? GARG_Z : 0.0f; m->cool = m->type == MOB_HUNTER ? 3.0f : 2.5f;
    m->cx = m->tx = (int)m->x; m->cy = m->ty = (int)m->y;
    if (!G.dead && hypotf(G.px - m->x, G.py - m->y) < 1.2f) {
        if (hurt_ex(mob_damage(m) * (m->type == MOB_HUNTER ? 0.6f : 0.85f), HURT_BLOW, m->x, m->y) > 0) thorns(m);
        if (m->type == MOB_HUNTER && rnd(3) == 0) bite(m);         /* now and then the fangs go in */
        DBG_COUNT(leaps);
    }
    return 1;
}

/* an egg sac: it waits while the caster is far, then stirs, and splits into spiderlings */
static void update_sac(Mob *m, float dt) {
    m->t += dt;
    if (m->state == MS_DIE) { if (m->t >= 0.4f) { add_deco(m->x, m->y, SP_SAC_OPEN); m->alive = 0; } return; }
    if (hypotf(G.px - m->x, G.py - m->y) < 10.0f) m->sub_t += dt;
    if (m->sub_t < 8.0f) return;
    for (int k = 0; k < (m->small ? 4 : 3); k++) {                  /* it hatches; the Wandering Spider's, small hunters */
        Mob *l = spawn_mob(m->cx, m->cy, m->small ? MOB_HUNTER : MOB_LING);
        if (!l) break;
        l->x = m->x + (rndf() - 0.5f) * 0.6f; l->y = m->y + (rndf() - 0.5f) * 0.6f; l->alert = 30.0f;
        set_rank(l, m->rank < RK_V ? m->rank : RK_V);              /* their mother's colour */
        if (m->small) { l->small = 1; l->hp = l->maxhp = l->maxhp * 0.5f; l->sub = 2; l->home = -1; }
    }
    sfx_at(SFX_SKITTER, m->x, m->y, 1.4f);
    add_deco(m->x, m->y, SP_SAC_OPEN);
    m->alive = 0;
}

/* A shade shows only in light: torches, fires, the wisp's pool, a spell going by. For a moment after
 * its touch the cold of it shows too. */
static float shade_alpha(const Mob *m) {
    if (m->state != MS_WALK) return 0.8f;
    float a = smoothstepf(0.3f, 0.6f, lm_at(m->x, m->y));
    return fmaxf(a, m->swing > 0.0f || m->sub_t > 0.0f ? 0.75f : 0.0f);
}
static int shade_hidden(const Mob *m) { return m->type == MOB_SHADE && shade_alpha(m) < 0.5f; }

/* a gargoyle wakes: the statue moves, and its plinth is floor again */
static void garg_wake(Mob *m) {
    if (m->sub != 0) return;
    m->sub = 1; m->sub_t = 0.0f;
    sfx_at(SFX_DIRT, m->x, m->y, 1.2f);
    if (G.map[m->cy][m->cx].kind == K_PROP) G.map[m->cy][m->cx].kind = K_FLOOR;
    DBG_COUNT(garg_wakes);
}
/* what stops a flier: walls and shut doors, not the things on the floor */
static int blocks_flight(int x, int y) { return cell(x, y)->kind != K_PROP && blocks_move(x, y); }

/* The Ill-Fated Knight's Swift Dodge (docs/PLAN-b7.md): half a second untouchable, darting two to five
 * cells along clear floor, with after-images, a whoosh and a clank of plate. Three reasons, three waits:
 * to close in when you are four cells off and in its sight (5 s), to get away when it has lost a tenth of
 * its health lately or stands in fire (9 s), to step aside from a shot about to strike it (16 s). Never
 * while a wave holds it: the hold comes first. */
static int knight_blocked(float x, float y) {
    for (int k = 0; k < 4; k++)
        if (blocks_move((int)(x + ((k & 1) ? 0.28f : -0.28f)), (int)(y + ((k & 2) ? 0.28f : -0.28f)))) return 1;
    return 0;
}
static float dash_room(const Mob *m, float a, float most) {
    float d = 0.0f, ca = cosf(a), sa = sinf(a);
    while (d + 0.25f <= most && in_arena(m->x + ca * (d + 0.25f), m->y + sa * (d + 0.25f)) && !knight_blocked(m->x + ca * (d + 0.25f), m->y + sa * (d + 0.25f)))
        d += 0.25f;
    return d;
}
static void knight_dash(Mob *m, float a, float dist, const char *why) {
    G.bdash_t = 0.5f; G.bdash_age = 0.0f;
    G.bdash_x0 = G.bdash_x1 = m->x; G.bdash_y0 = G.bdash_y1 = m->y;
    G.bdash_vx = cosf(a) * dist / 0.5f; G.bdash_vy = sinf(a) * dist / 0.5f;
    m->swing = 0.0f;
    sfx_at(SFX_WHOOSH, m->x, m->y, 0.9f); sfx_at(SFX_SHATTER, m->x, m->y, 0.3f);
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT knight_dash %s %.1f cells t=%.2f\n", why, dist, G.t);
#else
    (void)why;
#endif
}
static int knight_boss(Mob *m, float dt, float pd0, int sees) {
    for (int k = 0; k < 3; k++) if (G.bdash_cd[k] > 0.0f) G.bdash_cd[k] -= dt;
    G.bhurt *= expf(-dt / 3.0f);
    if (G.bdash_t > 0.0f) {                                        /* on its way */
        float nx = m->x + G.bdash_vx * dt, ny = m->y + G.bdash_vy * dt;
        G.bdash_t -= dt;
        if (!in_arena(nx, ny) || knight_blocked(nx, ny)) G.bdash_t = 0.0f;
        else { m->x = nx; m->y = ny; }
        m->cx = m->tx = (int)m->x; m->cy = m->ty = (int)m->y;
        G.bdash_x1 = m->x; G.bdash_y1 = m->y;
        m->face = atan2f(G.py - m->y, G.px - m->x);
        return 1;
    }
    if (m->swing > 0.0f || G.dead) return 0;
    int burning = 0;
    for (int i = 0; i < MAXBURNS; i++) burning |= G.burns[i].alive && !G.burns[i].foe && hypotf(G.burns[i].x - m->x, G.burns[i].y - m->y) < 0.7f;
    if (G.bdash_cd[1] <= 0.0f && (burning || G.bhurt >= m->maxhp * 0.1f)) {   /* away, as far as it can */
        static const float TURN[5] = { 0.0f, 0.6f, -0.6f, 1.2f, -1.2f };
        float away = atan2f(m->y - G.py, m->x - G.px), best = 0.0f, ba = away;
        for (int k = 0; k < 5; k++) { float r = dash_room(m, away + TURN[k], 5.0f); if (r > best + 0.5f) { best = r; ba = away + TURN[k]; } }
        if (best >= 2.0f) { G.bdash_cd[1] = 9.0f; G.bhurt = 0.0f; knight_dash(m, ba, best, "flee"); return 1; }
    }
    if (G.bdash_cd[2] <= 0.0f)                                     /* a shot about to strike: aside */
        for (int i = 0; i < MAXSHOTS; i++) {
            const Shot *sh = &G.shots[i];
            if (!sh->alive || sh->tier < 0) continue;
            float sp = hypotf(sh->vx, sh->vy) + 1e-3f, dx = m->x - sh->x, dy = m->y - sh->y;
            float along = (dx * sh->vx + dy * sh->vy) / sp, off = fabsf(dx * sh->vy - dy * sh->vx) / sp;
            if (along <= 0.0f || along > 3.5f || off > 0.7f) continue;
            float a = atan2f(sh->vy, sh->vx), l = dash_room(m, a + PI_F * 0.5f, 3.0f), r = dash_room(m, a - PI_F * 0.5f, 3.0f);
            if (fmaxf(l, r) < 2.0f) break;
            G.bdash_cd[2] = 16.0f;
            knight_dash(m, l >= r ? a + PI_F * 0.5f : a - PI_F * 0.5f, fmaxf(l, r), "dodge");
            return 1;
        }
    if (G.bdash_cd[0] <= 0.0f && sees && pd0 >= 4.0f) {             /* to a cell short of you, ready to strike */
        float a = atan2f(G.py - m->y, G.px - m->x), r = dash_room(m, a, fminf(5.0f, pd0 - 1.2f));
        if (r >= 2.0f) { G.bdash_cd[0] = 5.0f; knight_dash(m, a, r, "close"); m->cool = 0.0f; return 1; }
    }
    return 0;
}

/* Its Mirror Rune: one missile in seven that strikes it goes back at you, never two within two seconds,
 * at half the spell's damage and slower, so it can be dodged; its plate flashes silver in a ring and a
 * struck bell rings. Chain Lightning, Soul Drain and the BFS9000 aren't missiles. */
static int knight_turns(Mob *mb, Shot *s) {
    if (mb->boss != 1 + BOSS_KNIGHT || G.bstate != BS_FIGHT || G.bmirror_cd > 0.0f || s->tier < 0) return 0;
    if (s->kind != AT_FIRE && s->kind != AT_FROST && s->kind != AT_SPEAR && s->kind != AT_HELL && s->kind != AT_WISPS && s->kind != AT_SPARKS) return 0;
    if (rnd(7)) return 0;
    G.bmirror_cd = 2.0f; G.bmirror_t = 0.35f;
    s->alive = 0;
    for (int i = 0; i < MAXBONES; i++) {
        Bone *b = &G.bones[i];
        if (b->alive) continue;
        float a = atan2f(G.py - mb->y, G.px - mb->x), sp = fminf(hypotf(s->vx, s->vy) * 0.8f, 6.0f);
        *b = (Bone){ 1, MIS_TURNED, mb->x + cosf(a) * 0.5f, mb->y + sinf(a) * 0.5f, cosf(a) * sp, sinf(a) * sp, 4.0f, 0.0f,
                     fminf(s->dmg * 0.5f, (float)G.max_hp * 0.2f), 0.0f, (int)(mb - G.mobs), s->kind };
        break;
    }
    sfx_at(SFX_MIRRORBELL, mb->x, mb->y, 1.0f);
    DBG_COUNT(knight_turned);
    return 1;
}

/* The Wandering Spider (docs/PLAN-b7.md): up to the ceiling every four to seven seconds and across it,
 * out of reach, to a spot two cells short of you, then down and a leap. Its Mend Rune mends an eighth of
 * its health over four seconds once it has lost a fifth since it last mended, at most every twenty and
 * never past the last wave mark, glowing green. Its Tangled Web: it rears with a hiss and spits a glob
 * that can be sidestepped; if it lands you can't walk until your spells have torn a third of your health
 * out of it (fire twice over); the next comes 12 s after you are free. */
static int web_flying(void) {
    for (int i = 0; i < MAXBONES; i++) if (G.bones[i].alive && G.bones[i].kind == MIS_WEB) return 1;
    return 0;
}
static int spider_boss(Mob *m, float dt, float pd0, int sees) {
    float top = 1.0f - KINDS[MOB_HUNTER].scale * BOSSES[BOSS_SPIDER].scale;
    if (G.bmend_base > m->maxhp) G.bmend_base = m->maxhp;
    if (G.bmend_cd > 0.0f) G.bmend_cd -= dt;
    if (G.bmend_t > 0.0f) {
        float cap = G.bwave > 0 ? m->maxhp * WAVE_MARK[G.bwave - 1] : m->maxhp;
        m->hp = fminf(fmaxf(cap, m->hp), m->hp + m->maxhp * 0.125f / 4.0f * dt);
        if ((G.bmend_t -= dt) <= 0.0f) G.bmend_base = m->hp;
    } else if (G.bmend_cd <= 0.0f && m->hp <= G.bmend_base - 0.2f * m->maxhp) {
        G.bmend_t = 4.0f; G.bmend_cd = 20.0f;
        sfx_at(SFX_RUNE, m->x, m->y, 1.2f); sfx_at(SFX_POTION, m->x, m->y, 0.8f);
        DBG_COUNT(spider_mends);
    }
    if (m->sub == 3 || m->sub == 1) return 0;                      /* a leap, or the drop: the hunter's own */
    if (m->sub == 5) {                                             /* up to the ceiling */
        m->sub_t += dt;
        m->z = top * fminf(1.0f, m->sub_t / 0.4f);
        if (m->sub_t >= 0.4f) { m->sub = 0; m->sub_t = 0.0f; }
        return 1;
    }
    if (m->sub == 0) {                                             /* across it, to two cells short of you, and down */
        m->z = top; m->sub_t += dt; m->t += dt;
        float d = fmaxf(pd0, 0.01f), gx = G.px + (m->x - G.px) / d * 2.0f, gy = G.py + (m->y - G.py) / d * 2.0f;
        float vx = gx - m->x, vy = gy - m->y, vd = hypotf(vx, vy), step = m->speed * 1.4f * dt;
        if (vd > 0.3f) {
            float nx = m->x + vx / vd * fminf(step, vd), ny = m->y + vy / vd * fminf(step, vd);
            if (in_arena(nx, ny) && !knight_blocked(nx, ny)) { m->x = nx; m->y = ny; m->cx = m->tx = (int)nx; m->cy = m->ty = (int)ny; }
            else vd = 0.0f;
        }
        if ((vd <= 0.3f || m->sub_t > 3.0f) && G.bstate == BS_FIGHT) { m->sub = 1; m->sub_t = 0.0f; m->cool = 0.0f; sfx_at(SFX_SKITTER, m->x, m->y, 1.3f); }
        return 1;
    }
    if (G.web_rear > 0.0f) {                                       /* rearing: then the glob */
        m->face = atan2f(G.py - m->y, G.px - m->x);
        if ((G.web_rear -= dt) <= 0.0f)
            for (int i = 0; i < MAXBONES; i++) {
                Bone *b = &G.bones[i];
                if (b->alive) continue;
                float a = atan2f(G.py - m->y, G.px - m->x);
                *b = (Bone){ 1, MIS_WEB, m->x + cosf(a) * 0.5f, m->y + sinf(a) * 0.5f, cosf(a) * 3.5f, sinf(a) * 3.5f, 4.0f, 0.0f, 0.0f, 0.0f, (int)(m - G.mobs), 0 };
                sfx_at(SFX_THROW, m->x, m->y, 1.0f);
                break;
            }
        return 1;
    }
    if (m->swing > 0.0f || G.dead) return 0;
    if (G.web_hp <= 0.0f && !web_flying() && (G.web_cd -= dt) <= 0.0f && sees && pd0 > 2.5f && pd0 < 9.0f) {
        G.web_rear = 0.6f; G.web_cd = 12.0f;
        sfx_at(SFX_SKITTER, m->x, m->y, 1.5f); sfx_at(SFX_WHISPER, m->x, m->y, 0.6f);   /* a hiss */
        return 1;
    }
    if ((G.bsp_t -= dt) <= 0.0f) {                                 /* up it goes */
        G.bsp_t = 4.0f + 3.0f * rndf();
        m->sub = 5; m->sub_t = 0.0f;
        sfx_at(SFX_SKITTER, m->x, m->y, 1.1f);
        DBG_COUNT(spider_ups);
        return 1;
    }
    return 0;
}
/* a spell cast while held strikes the web instead; fire tears it twice as fast */
static int web_strike(int sp, float dmg) {
    if (G.web_hp <= 0.0f) return 0;
    G.web_hp -= dmg * (sp == AT_FIRE || sp == AT_HELL ? 2.0f : 1.0f);
    G.web_idle = 0.0f;
    sfx_at(sp == AT_FIRE || sp == AT_HELL ? SFX_IGNITE : SFX_SKITTER, G.px, G.py, 0.8f);
    if (G.web_hp <= 0.0f) { G.web_hp = 0.0f; G.web_cd = 12.0f; snd_sfx_ex(SFX_SHATTER, 0.6f, 0.5f); DBG_COUNT(web_torn); }
    return 1;
}
/* Egg Shot: a sac stuck to the stone in sight of you, which hatches four small spiders in five seconds
 * unless it is burned or broken first */
static void egg_shot(const Mob *m) {
    for (int tries = 0; tries < 60; tries++) {
        float a = rndf() * TAU_F, d = 2.0f + 4.0f * rndf();
        int x = (int)floorf(G.px + cosf(a) * d), y = (int)floorf(G.py + sinf(a) * d), wall = 0;
        if (!in_arena(x + 0.5f, y + 0.5f) || G.map[y][x].kind != K_FLOOR || occupied(x, y, NULL)) continue;
        for (int k = 0; k < 4; k++) wall |= G.map[y + DY[k]][x + DX[k]].kind == K_WALL;
        if (!wall || !line_clear(G.px, G.py, x + 0.5f, y + 0.5f)) continue;
        Mob *e = spawn_mob(x, y, MOB_SAC);
        if (!e) return;
        e->small = 1; e->sub_t = 3.0f; e->rank = m->rank; e->alert = 30.0f;
        sfx_at(SFX_THROW, m->x, m->y, 1.0f); sfx_at(SFX_SKITTER, e->x, e->y, 1.0f);
        DBG_COUNT(egg_shots);
        return;
    }
}

/* The Wraith Unbound (docs/PLAN-b7.md): here it can be hurt, but only in light, from its braziers or your
 * wisp; it puts the braziers out as it passes, and a void over one does too; E kindles one again in a
 * second, and a wisp close by does it at once. Its Time Slip Rune slows your missiles near it with a grey
 * ripple and a deep chime while it steps aside; Wraithwalk takes it into the stone and out of a wall
 * near you. */
static int wraith_lit(float x, float y) {
    for (int b = 0; b < G.bbz_n; b++) if (G.bbz_lit[b] && hypotf(G.bbz_x[b] + 0.5f - x, G.bbz_y[b] + 0.5f - y) < 4.0f) return 1;
    for (int i = 0; i < MAXWISPS; i++) if (G.wisps[i].on && G.wisps[i].a > 0.5f && hypotf(G.wisps[i].x - x, G.wisps[i].y - y) < 3.0f) return 1;
    return 0;
}
static int wraith_open(const Mob *m) { return m->type != MOB_WRAITH || !m->boss || (!G.bww && wraith_lit(m->x, m->y)); }
static void brazier_set(int b, int lit) {
    if (G.bbz_lit[b] == lit) return;
    G.bbz_lit[b] = lit;
    Deco *dc = &G.decos[G.bbz_deco[b]];
    dc->spr = lit ? SP_BRAZIER_A : SP_BRAZIER_OUT; dc->nfr = lit ? 3 : 1;
    sfx_at(lit ? SFX_IGNITE : SFX_WHOOSH, G.bbz_x[b] + 0.5f, G.bbz_y[b] + 0.5f, lit ? 1.0f : 0.6f);
#ifdef CRAWL_DEBUG
    if (lit) DBG_COUNT(brazier_lit); else DBG_COUNT(brazier_out);
#endif
}
static int brazier_facing(void) {                                   /* an unlit brazier before the dweller */
    for (int b = 0; b < G.bbz_n; b++) {
        float d = hypotf(G.bbz_x[b] + 0.5f - G.px, G.bbz_y[b] + 0.5f - G.py);
        if (!G.bbz_lit[b] && d < 1.8f && fabsf(wrap_pi(atan2f(G.bbz_y[b] + 0.5f - G.py, G.bbz_x[b] + 0.5f - G.px) - G.ang)) < 0.8f) return b;
    }
    return -1;
}
static void wraith_boss(Mob *m, float dt) {
    m->t += dt;
    dt = mob_time(m, dt);
    if (dt <= 0.0f) return;
    m->anim += dt * 2.0f; m->cool -= dt; m->sub = 0;               /* nothing drives it back here */
    if (G.bgutter_cd > 0.0f) G.bgutter_cd -= dt;
    for (int b = 0; b < G.bbz_n && G.bgutter_cd <= 0.0f; b++)      /* it gutters a brazier it passes close by, one in eight seconds */
        if (G.bbz_lit[b] && !G.bww && hypotf(G.bbz_x[b] + 0.5f - m->x, G.bbz_y[b] + 0.5f - m->y) < 1.05f) { brazier_set(b, 0); G.bgutter_cd = 8.0f; }
    float pd = hypotf(G.px - m->x, G.py - m->y);
    if (G.bww) {                                                   /* Wraithwalk */
        G.bww_t += dt;
        if (G.bww == 1 && G.bww_t >= 0.6f) { G.bww = 2; G.bww_t = 0.0f; }
        else if (G.bww == 2 && G.bww_t >= 1.5f) {                  /* out of a wall near you */
            for (int tries = 0; tries < 60; tries++) {
                float a = rndf() * TAU_F, d = 2.0f + 1.5f * rndf();
                int x = (int)floorf(G.px + cosf(a) * d), y = (int)floorf(G.py + sinf(a) * d), wall = 0;
                if (!in_arena(x + 0.5f, y + 0.5f) || G.map[y][x].kind != K_FLOOR || occupied(x, y, m)) continue;
                for (int k = 0; k < 4; k++) wall |= G.map[y + DY[k]][x + DX[k]].kind == K_WALL;
                if (!wall) continue;
                m->x = x + 0.5f; m->y = y + 0.5f; m->cx = m->tx = x; m->cy = m->ty = y;
                break;
            }
            G.bww = 3; G.bww_t = 0.0f;
            sfx_at(SFX_SHRIEK, m->x, m->y, 0.9f);
        } else if (G.bww == 3 && G.bww_t >= 0.5f) { G.bww = 0; G.bww_cd = 16.0f + 4.0f * rndf(); m->cool = fminf(m->cool, 0.3f); }
        return;
    }
    if ((G.bww_cd -= dt) <= 0.0f && pd > 2.0f && !G.dead) {
        G.bww = 1; G.bww_t = 0.0f;
        sfx_at(SFX_WAIL, m->x, m->y, 0.9f);
        DBG_COUNT(wraithwalks);
        return;
    }
    if (G.bslip_cd > 0.0f) G.bslip_cd -= dt;
    if (G.bslip_cd <= 0.0f && wraith_lit(m->x, m->y))              /* its Time Slip Rune: a missile about to strike */
        for (int i = 0; i < MAXSHOTS; i++) {
            const Shot *sh = &G.shots[i];
            if (!sh->alive || sh->tier < 0) continue;
            float sp = hypotf(sh->vx, sh->vy) + 1e-3f, dx = m->x - sh->x, dy = m->y - sh->y;
            float along = (dx * sh->vx + dy * sh->vy) / sp, off = fabsf(dx * sh->vy - dy * sh->vx) / sp;
            if (along <= 0.0f || along > 2.5f || off > 0.7f) continue;
            G.bslip_t = 1.5f; G.bslip_cd = def_cooldown(Q_RARE + 1); G.bslip_x = m->x; G.bslip_y = m->y;
            float a = atan2f(sh->vy, sh->vx) + (rnd(2) ? 0.5f : -0.5f) * PI_F;   /* and it steps out of the way */
            int nx = (int)(m->x + cosf(a) * 1.2f), ny = (int)(m->y + sinf(a) * 1.2f);
            if (in_arena(nx + 0.5f, ny + 0.5f) && G.map[ny][nx].kind == K_FLOOR) { m->tx = nx; m->ty = ny; }
            sfx_at(SFX_DEEPCHIME, m->x, m->y, 1.0f);
            DBG_COUNT(wraith_slips);
            break;
        }
    if (!G.dead && pd < 1.1f && m->cool <= 0.0f) {                 /* its touch */
        m->cool = 2.5f;
        sfx_at(SFX_SHRIEK, m->x, m->y, 1.0f);
        if (hurt_ex(mob_damage(m) * 1.2f, HURT_MAGIC, m->x, m->y) > 0) {
            G.chill = 1.0f; G.slow_t = fmaxf(G.slow_t, 2.0f * (1.0f - gear_slow_resist(&RUN.gear)));
            DBG_COUNT(wraith_touches);
        }
    }
    float gx = m->tx + 0.5f, gy = m->ty + 0.5f, dx = gx - m->x, dy = gy - m->y, d = sqrtf(dx * dx + dy * dy), step = m->speed * dt;
    if (d > step) { m->x += dx / d * step; m->y += dy / d * step; return; }
    m->x = gx; m->y = gy; m->cx = m->tx; m->cy = m->ty;
    if (G.dead) return;
    int best = -1; float bs = 1e9f;
    for (int k = 0; k < 4; k++) {                                  /* straight for you, round what stands in the way */
        int nx = m->cx + DX[k], ny = m->cy + DY[k];
        if (!in_arena(nx + 0.5f, ny + 0.5f) || blocks_move(nx, ny) || occupied(nx, ny, m)) continue;
        float sc = hypotf(nx + 0.5f - G.px, ny + 0.5f - G.py) + (hunt[ny][nx] >= 0 ? 0.0f : 2.0f) + rndf() * 0.3f;
        if (sc < bs) { bs = sc; best = k; }
    }
    if (best >= 0) { m->tx = m->cx + DX[best]; m->ty = m->cy + DY[best]; }
}

static void update_mob(Mob *m, float dt) {
    if (m->state == MS_DIE && m->sub == 77) { if ((m->t += dt) >= 0.9f) m->alive = 0; return; }   /* gone into the BFS9000 */
    if (m->boss && m->state == MS_WALK && (G.bstate != BS_FIGHT || G.bpose_t > 0.0f)) {   /* waiting, holding while its wave
                                                                                            fights, or casting */
        m->t += dt;
        if (G.bstate != BS_WAIT) m->face += wrap_pi(atan2f(G.py - m->y, G.px - m->x) - m->face) * fminf(1.0f, dt * 2.0f);
        return;
    }
    if (G.ptheme == PT_SEVERANCE && m->cool > 0.0f) m->cool -= dt * 0.25f;   /* Severance's strike and throw a fifth sooner */
    if (m->type == MOB_GHOST) { update_ghost(m, dt); return; }
    if (m->type == MOB_WSKULL) { update_wskull(m, dt); return; }
    if (m->type == MOB_SPIDER) { update_spider(m, dt); return; }
    if (m->type == MOB_WRAITH) { if (m->boss) wraith_boss(m, dt); else update_wraith(m, dt); return; }
    if (m->type == MOB_SAC) { update_sac(m, dt); return; }
    if (update_riser(m, dt)) return;
    m->t += dt;
    if (m->state == MS_DIE) {
        float dur = m->type == MOB_BAT ? 0.9f : 0.75f;
        if (m->type == MOB_BAT || m->type == MOB_HUNTER || m->type == MOB_GARGOYLE) m->z = fmaxf(0.0f, m->z - dt * (m->z > 0.3f ? 1.6f : 0.6f));   /* down it falls */
        if (m->t >= dur) {
            if (KINDS[m->type].remains >= 0) add_deco(m->x, m->y, KINDS[m->type].remains);
            m->alive = 0;
        }
        return;
    }
    if (m->type == MOB_SKEL && m->sub == 5) { if ((m->sub_t += dt) >= 0.7f) m->sub = 0; return; }   /* called up, it gets to its feet */
    dt = mob_time(m, dt);
    if (dt <= 0.0f) return;
    m->anim += dt * m->speed * 3.0f;
    m->z = m->type == MOB_BAT ? 0.20f + 0.07f * sinf(m->t * 7.0f) : 0.0f;
    float pdx0 = G.px - m->x, pdy0 = G.py - m->y, pd0 = sqrtf(pdx0 * pdx0 + pdy0 * pdy0);
    int undead = KIND_IS(m, KF_UNDEAD) != 0;
    int sees = !G.dead && pd0 < 9.0f && line_clear(m->x, m->y, G.px, G.py);
    if (sees && G.level_t > 1.5) {                                 /* it has you: it will come after you a while */
        m->alert = m->type == MOB_SKEL ? 7.0f : 12.0f;            /* a thrower gives up sooner */
        if (m->lost > 1.0f && m->type == MOB_SKEL) m->cool = fmaxf(m->cool, 0.6f * throw_interval());   /* finding its range again */
        m->lost = 0.0f;
    } else {
        if (m->alert > 0.0f) m->alert -= dt;
        m->lost += dt;
    }
    if (m->feared > 0.0f) m->feared -= dt; else m->dreaded = 0;
    if (m->stagger > 0.0f) { m->stagger -= dt; return; }
    int keeps_off = (m->type == MOB_IMP || m->type == MOB_DEACON) && !m->boss && !m->add;   /* they fight from further off, and back
                                                                                    away (not in a boss's fight: they hold the floor) */
    int cowed = m->feared > 0.0f && (undead || m->dreaded);       /* Dread */
    int lured = !cowed && G.decoy_t > 0.0f && hypotf(m->x - G.decoy_x, m->y - G.decoy_y) < 6.0f;   /* Blink's double draws it */
    int fights = !G.dead && !cowed && !lured;                     /* it may go for the caster now */
    int afraid = cowed || (keeps_off && sees && pd0 < 2.8f);
    if (m->type == MOB_SKEL && fights && !afraid) {               /* once it has seen you, it keeps throwing */
        int los = sees && G.level_t > 1.5;
        if (los && !m->seen) { m->seen = 1; m->cool = (0.45f + 0.35f * rndf()) * throw_interval() / 1.5f; }
        if (m->throw_t > 0.0f) {
            m->throw_t -= dt;
            if (m->throw_t <= 0.0f) { throw_bone(m); m->cool = throw_interval() * (1.0f + (rndf() - 0.5f) * 0.27f); DBG_COUNT(throws); }
            return;                                                /* stands still to throw */
        }
        if (m->seen && los && (m->cool -= dt) <= 0.0f) { m->throw_t = 0.35f; return; }
    }
    if (m->type == MOB_SHIELD && fights && !afraid) {             /* closes in behind its shield and strikes */
        int los = sees && G.level_t > 1.5;
        if (los) { m->seen = 1; m->face += wrap_pi(atan2f(pdy0, pdx0) - m->face) * fminf(1.0f, dt * 6.0f); }
        m->cool -= dt;
        if (m->swing > 0.0f) {
            m->swing -= dt;
            if (m->swing <= 0.0f) {
                m->swing = 0.0f; m->cool = 1.4f;
                sfx_at(SFX_SWORD, m->x, m->y, 1.2f);
                if (pd0 < 1.6f && hurt_ex(mob_damage(m) + 1.0f, HURT_BLOW, m->x, m->y) > 0) { thorns(m); DBG_COUNT(hits_sword); }
            }
            return;
        }
        if (los && pd0 < 1.35f && m->cool <= 0.0f) { m->swing = 0.45f; return; }
    }
    if ((m->type == MOB_SHAMBLER || m->type == MOB_DROWNED) && fights && !afraid) {   /* it lurches in and takes hold */
        m->cool -= dt;
        if (m->swing > 0.0f) {
            m->swing -= dt;
            if (m->swing <= 0.0f) {
                m->swing = 0.0f; m->cool = 1.8f;
                if (pd0 < 1.45f && hurt_ex(mob_damage(m), HURT_BLOW, m->x, m->y) > 0) {
                    G.slow_t = fmaxf(G.slow_t, (m->type == MOB_DROWNED ? 2.5f : 1.2f) * (1.0f - gear_slow_resist(&RUN.gear)));   /* it holds on */
                    thorns(m); DBG_COUNT(grabs);
                }
            }
            return;
        }
        if (sees && pd0 < 1.25f && m->cool <= 0.0f) { m->swing = 0.6f; sfx_at(SFX_GROAN, m->x, m->y, 1.0f); return; }
        if (pd0 < 7.0f && rnd(500) == 0) sfx_at(SFX_GROAN, m->x, m->y, 0.7f);
    }
    if (m->type == MOB_HOUND && fights && !afraid) {               /* in, a bite, and away */
        m->cool -= dt;
        if (m->swing > 0.0f) {
            m->swing -= dt;
            if (m->swing <= 0.0f) {
                m->swing = 0.0f; m->cool = 1.3f; m->feared = 0.8f; m->wfeared = 0;
                sfx_at(SFX_BONES, m->x, m->y, 0.7f);
                if (pd0 < 1.3f && hurt_ex(mob_damage(m) * 0.8f, HURT_BLOW, m->x, m->y) > 0) { thorns(m); DBG_COUNT(hound_bites); }
            }
            return;
        }
        if (sees && pd0 < 1.15f && m->cool <= 0.0f) { m->swing = 0.25f; return; }
    }
    if (m->boss == 1 + BOSS_SPIDER && spider_boss(m, dt, pd0, sees)) return;
    if (m->type == MOB_HUNTER && !G.dead) {                        /* along the ceiling, down a thread, and a leap */
        if (update_leap(m, dt)) return;
        m->cool -= dt;
        if (m->sub == 0) {                                         /* overhead, upside down */
            m->z = 1.0f - KINDS[MOB_HUNTER].scale * RANK_SIZE[m->rank];
            if (fights && sees && pd0 < 2.6f && G.level_t > 1.5) { m->sub = 1; m->sub_t = 0.0f; sfx_at(SFX_SKITTER, m->x, m->y, 1.3f); }
        } else if (m->sub == 1) {                                  /* it drops */
            m->sub_t += dt;
            m->z = lerpf(1.0f - KINDS[MOB_HUNTER].scale, 0.0f, fminf(1.0f, m->sub_t / 0.4f));
            if (m->sub_t >= 0.4f) { m->sub = 2; m->z = 0.0f; m->alert = 12.0f; }
            return;
        } else if (fights && sees && pd0 < 2.4f && pd0 > 0.9f && m->cool <= 0.0f) { start_leap(m); return; }
    }
    if (m->type == MOB_GHOUL && !G.dead) {                          /* it leaps the last of the way; it feeds on the dead */
        if (update_leap(m, dt)) return;
        m->cool -= dt;
        if (m->sub == 4) {                                         /* feeding: it mends, and is open */
            m->hp = fminf(m->maxhp, m->hp + m->maxhp * 0.06f * dt);
            if ((m->sub_t -= dt) <= 0.0f || (sees && pd0 < 6.0f)) m->sub = 0;
            if (rnd(90) == 0) sfx_at(SFX_BONES, m->x, m->y, 0.5f);
            return;
        }
        if (fights && !afraid && sees && pd0 < 2.6f && pd0 > 0.9f && m->cool <= 0.0f) { start_leap(m); return; }
        if (m->alert <= 0.0f && m->hp < m->maxhp && (m->contact -= dt) <= 0.0f) {   /* bones near? it stops to eat */
            m->contact = 2.0f;
            for (int i = 0; i < G.ndecos; i++) {
                int sp = G.decos[i].spr;
                if ((sp == SP_BONES1 || sp == SP_BONES2 || sp == SP_SKEL_D3 || sp == SP_SKULLPILE || sp == SP_CARCASS)
                    && fabsf(G.decos[i].x - m->x) < 1.2f && fabsf(G.decos[i].y - m->y) < 1.2f) { m->sub = 4; m->sub_t = 4.0f; break; }
            }
        }
    }
    if (m->boss == 1 + BOSS_KNIGHT && knight_boss(m, dt, pd0, sees)) return;
    if (m->type == MOB_KNIGHT && fights && !afraid) {              /* a long wind-up, and a blow that tells */
        int st = knight_stage(m);
        if (st != m->sub) { m->sub = st; sfx_at(SFX_SHATTER, m->x, m->y, 1.1f); m->speed = knight_pace(m, st); }
        m->cool -= dt;
        if (m->throw_t > 0.0f) m->throw_t -= dt;                   /* the blow, still showing */
        if (m->swing > 0.0f) {
            m->swing -= dt;
            if (m->swing <= 0.0f) {
                m->swing = 0.0f; m->cool = 2.0f; m->throw_t = 0.3f;
                sfx_at(SFX_SWORD, m->x, m->y, 1.4f);
                if (pd0 < 1.7f && hurt_ex(mob_damage(m) * 1.5f, HURT_BLOW, m->x, m->y) > 0) { thorns(m); DBG_COUNT(knight_blows); }
            }
            return;
        }
        if (sees && pd0 < 1.5f && m->cool <= 0.0f) { m->swing = 0.9f; sfx_at(SFX_SWORD, m->x, m->y, 0.4f); return; }
    }
    if (m->type == MOB_BROOD && !G.dead) {                          /* she lays as she goes, and bites what comes close */
        m->cool -= dt;
        if (m->swing > 0.0f) { if ((m->swing -= dt) <= 0.0f) m->swing = 0.0f; return; }   /* laying */
        if (fights && sees && pd0 < 1.4f && m->cool <= 0.0f) {
            m->cool = 1.6f;
            if (hurt_ex(mob_damage(m) * 1.2f, HURT_BLOW, m->x, m->y) > 0) { thorns(m); DBG_COUNT(brood_bites); }
            sfx_at(SFX_SKITTER, m->x, m->y, 1.2f);
            return;
        }
        int sacs = 0;
        for (int i = 0; i < MAXMOBS; i++) sacs += G.mobs[i].alive && G.mobs[i].type == MOB_SAC && G.mobs[i].state == MS_WALK;
        if (m->alert > 0.0f && sacs < 4 && (m->contact -= dt) <= 0.0f) {
            m->contact = 7.0f; m->swing = 1.0f;
            Mob *e = spawn_mob(m->cx, m->cy, MOB_SAC);
            if (e) { e->rank = m->rank; DBG_COUNT(sacs_laid); }
            return;
        }
    }
    if (m->type == MOB_LING && !G.dead) {                           /* a swarm of small bites */
        m->cool -= dt; m->alert = fmaxf(m->alert, 5.0f);
        if (fights && pd0 < 0.8f && m->cool <= 0.0f) {
            m->cool = 1.0f;
            if (hurt_ex(mob_damage(m) * 0.4f, HURT_BLOW, m->x, m->y) > 0) { thorns(m); DBG_COUNT(ling_bites); }
        }
    }
    if (m->type == MOB_SHADE && !G.dead) {                         /* unseen in the dark; its touch takes souls */
        m->cool -= dt;
        if (m->sub_t > 0.0f) m->sub_t -= dt;
        if (m->swing > 0.0f) {
            m->swing -= dt;
            if (m->swing <= 0.0f) {
                m->swing = 0.0f; m->cool = 2.2f; m->sub_t = 1.5f;
                if (pd0 < 1.1f) {
                    int take = RUN.souls < 2 + m->rank ? RUN.souls : 2 + m->rank;
                    RUN.souls -= take;
                    if (hurt_ex(mob_damage(m) * (take ? 0.5f : 1.0f), HURT_MAGIC, m->x, m->y) > 0) thorns(m);   /* nothing to take: it takes more of you */
                    snd_sfx_ex(SFX_WHISPER, 0.9f, 0.5f); pop_readout();
                    DBG_COUNT(shade_touches);
                }
            }
            return;
        }
        if (fights && pd0 < 0.95f && m->cool <= 0.0f) { m->swing = 0.5f; sfx_at(SFX_GHOST_TICK, m->x, m->y, 0.8f); return; }
        if (pd0 < 6.0f && shade_hidden(m) && rnd(100) == 0) sfx_at(SFX_STEP, m->x, m->y, 0.45f);   /* footsteps, and no one there */
    }
    if (m->type == MOB_IMP && !G.dead) {                           /* it keeps off, and lobs embers */
        if (m->throw_t > 0.0f) {
            if ((m->throw_t -= dt) <= 0.0f) { throw_missile(m, MIS_EMBER); m->cool = 2.4f + rndf(); DBG_COUNT(embers); }
            return;
        }
        m->cool -= dt;
        if (fights && sees && pd0 > 1.5f && pd0 < 8.0f && m->cool <= 0.0f && G.level_t > 1.5) { m->throw_t = 0.4f; return; }
    }
    if (m->type == MOB_DEACON && !G.dead) {                        /* it keeps back, and calls the fallen up again */
        if (m->swing > 0.0f) {                                     /* chanting over the bones */
            if ((m->swing -= dt) > 0.0f) return;
            m->swing = 0.0f; m->contact = m->boss ? 12.0f : 6.0f;      /* the Black Deacon calls one at a time, with a pause */
            int di = m->deco;
            m->deco = -1;
            if (di >= 0 && (G.decos[di].spr == SP_SKEL_D3 || G.decos[di].spr == SP_BONES2)) {
                int x = (int)G.decos[di].x, y = (int)G.decos[di].y;
                Mob *r = occupied(x, y, NULL) ? NULL : spawn_mob(x, y, MOB_SKEL);
                if (r) {
                    G.decos[di].spr = -1;                          /* the heap gets up */
                    set_rank(r, m->rank > RK_I && m->rank < RK_GILDED ? m->rank - 1 : RK_I);
                    r->sub = 5; r->sub_t = 0.0f; r->alert = 12.0f; r->seen = 1; r->cool = throw_interval();
                    m->raised++;
                    if (m->boss) { r->add = 3; r->home = -1; r->alert = 30.0f; }   /* the Black Deacon's: they come for you */
                    sfx_at(SFX_BONES, r->x, r->y, 1.1f);
                    DBG_COUNT(raised);
                }
            }
            return;
        }
        int called = 0;
        for (int i = 0; i < MAXMOBS && m->boss; i++) called += G.mobs[i].alive && G.mobs[i].add == 3 && G.mobs[i].state != MS_DIE;
        if (fights && m->alert > 0.0f && (m->boss ? called < 3 : m->raised < 4) && (m->contact -= dt) <= 0.0f) {
            m->contact = 1.0f;                                     /* it looks again in a moment */
            for (int i = 0; i < G.ndecos; i++) {
                const Deco *dc = &G.decos[i];
                if (dc->spr != SP_SKEL_D3 && dc->spr != SP_BONES2) continue;
                if (hypotf(dc->x - m->x, dc->y - m->y) > 6.0f || !line_clear(m->x, m->y, dc->x, dc->y) || occupied((int)dc->x, (int)dc->y, NULL)) continue;
                m->swing = DEACON_CHANT; m->deco = i; m->ax = dc->x; m->ay = dc->y;
                G.bchant_dmg = 0.0f;
                sfx_at(SFX_WHISPER, m->x, m->y, 1.0f);
                return;
            }
        }
    }
    if (m->type == MOB_GARGOYLE) {                                 /* stone till you pass; then wings */
        if (m->sub == 0) {
            if (!G.dead && sees && pd0 < 3.0f && G.level_t > 1.5) garg_wake(m);
            return;
        }
        if (m->sub == 1) {                                         /* it shakes itself loose */
            m->sub_t += dt; m->z = GARG_Z * smoothstepf(0.2f, 0.8f, m->sub_t);
            if (m->sub_t >= 0.8f) { m->sub = 2; m->alert = 12.0f; }
            return;
        }
        if (update_leap(m, dt)) return;
        m->z = GARG_Z + 0.03f * sinf(m->t * 5.0f);
        m->cool -= dt;
        if (m->swing > 0.0f) {                                     /* its claws */
            if ((m->swing -= dt) <= 0.0f) {
                m->swing = 0.0f; m->cool = 1.6f;
                if (pd0 < 1.3f && hurt_ex(mob_damage(m) * 1.1f, HURT_BLOW, m->x, m->y) > 0) thorns(m);
            }
            return;
        }
        if (fights && sees && pd0 < 1.2f && m->cool <= 0.0f) { m->swing = 0.3f; return; }
        if (fights && sees && pd0 > 1.4f && pd0 < 3.2f && m->cool <= 0.0f) { start_leap(m); return; }
    }
    if (m->type == MOB_BAT && !G.dead) {                           /* bats come for your face */
        int los = pd0 < 5.0f && sees;
        if (los && fights) m->attacking = 1;
        if (m->attacking && (pd0 > 7.0f || !fights)) { m->attacking = 0; m->tx = m->cx; m->ty = m->cy; }
        if (m->attacking) {
            m->orbit += dt * (1.3f + 0.4f * sinf(m->t * 0.7f));
            float a = G.ang + sinf(m->orbit) * 1.5f, r = 0.95f + 0.18f * sinf(m->orbit * 2.3f);
            float gx = G.px + cosf(a) * r, gy = G.py + sinf(a) * r;
            float vx = gx - m->x, vy = gy - m->y, vd = sqrtf(vx * vx + vy * vy) + 1e-4f, step = fminf(vd, 2.6f * dt);
            float nx = m->x + vx / vd * step, ny = m->y + vy / vd * step;
            if (!blocks_move((int)nx, (int)m->y)) m->x = nx;
            if (!blocks_move((int)m->x, (int)ny)) m->y = ny;
            m->cx = m->tx = (int)m->x; m->cy = m->ty = (int)m->y;
            if (hypotf(G.px - m->x, G.py - m->y) < 1.25f) {           /* touching: after 3 s it bites */
                m->contact += dt;
                if (m->contact >= 3.0f) {
                    m->contact = 0.0f;
                    if (hurt_ex(mob_damage(m), HURT_BLOW, m->x, m->y) > 0) thorns(m);
                    sfx_at(SFX_BAT, m->x, m->y, 1.0f); DBG_COUNT(hits_bat);
                }
            } else m->contact = 0.0f;
            if (rnd(60) == 0) sfx_at(SFX_BAT, m->x, m->y, 0.6f);
            return;
        }
    }
    float gx = m->tx + 0.5f, gy = m->ty + 0.5f, dx = gx - m->x, dy = gy - m->y, d = sqrtf(dx * dx + dy * dy);
    float step = m->speed * dt * (m->type == MOB_BAT ? 0.8f + 0.4f * sinf(m->t * 3.0f) : 1.0f) * (afraid ? 1.4f : 1.0f);
    if (d > step) { m->x += dx / d * step; m->y += dy / d * step; if (m->type == MOB_SHIELD && !m->seen) m->face = atan2f(dy, dx); return; }
    m->x = gx; m->y = gy; m->cx = m->tx; m->cy = m->ty;
    /* next cell: close in on the caster when it is near and visible, otherwise wander; flee if afraid */
    float pdx = G.px - m->x, pdy = G.py - m->y, pd = sqrtf(pdx * pdx + pdy * pdy);
    float keep = KIND_IS(m, KF_MELEE) ? 1.05f : (keeps_off ? 4.0f : 2.2f);   /* those that fight hand to hand come right up close */
    int chase = !lured && pd < (KIND_IS(m, KF_MELEE) ? 9.0f : 7.0f) && pd > keep && line_clear(m->x, m->y, G.px, G.py);
    int here = hunt[m->cy][m->cx], opens = KIND_IS(m, KF_DOORS) != 0;   /* the dead open doors; bats and hounds wait behind them */
    int hunting = !afraid && !lured && !chase && !G.dead && m->alert > 0.0f && m->hunts && here > 0 && pd > keep;
    int best = -1; float bs = -1e9f;
    for (int k = 0; k < 4; k++) {
        int nx = m->cx + DX[k], ny = m->cy + DY[k];
        const Cell *c = cell(nx, ny);
        if (hunting) {                                             /* down the field, towards the caster */
            int h = hunt[ny][nx];
            if (h < 0 || h >= here || occupied(nx, ny, m)) continue;
            if (c->kind == K_DOOR && !opens && G.doors[c->door].open < 0.9f) continue;
            float s = rndf() * 0.5f - (float)h;
            if (s > bs) { bs = s; best = k; }
            continue;
        }
        if ((m->type == MOB_GARGOYLE ? blocks_flight(nx, ny) : blocks_move(nx, ny)) || occupied(nx, ny, m)) continue;
        if (!chase && !afraid && !lured && m->home >= 0 && c->region != m->home) continue;   /* keeps to its own room till it knows */
        float s = rndf();
        if (afraid) s += hypotf(nx + 0.5f - (m->wfeared ? m->wfx : G.px), ny + 0.5f - (m->wfeared ? m->wfy : G.py)) * 2.0f;
        else if (lured) s -= hypotf(nx + 0.5f - G.decoy_x, ny + 0.5f - G.decoy_y);
        else if (chase) s -= hypotf(nx + 0.5f - G.px, ny + 0.5f - G.py);
        if (s > bs) { bs = s; best = k; }
    }
    if (best >= 0 && hunting) {
        int nx = m->cx + DX[best], ny = m->cy + DY[best];
        const Cell *c = cell(nx, ny);
        if (c->kind == K_DOOR && G.doors[c->door].open < 0.9f) {    /* a door in the way: it pulls it open, and waits */
            Door *dr = &G.doors[c->door];
            if (!dr->opening) { dr->opening = 1; sfx_at(SFX_DOOR, nx + 0.5f, ny + 0.5f, 0.8f); }
            return;
        }
        m->tx = nx; m->ty = ny;
    } else if (best >= 0 && (afraid || lured || !(pd <= keep))) { m->tx = m->cx + DX[best]; m->ty = m->cy + DY[best]; }
    if (m->type == MOB_BAT && pd < 6.0f && rnd(40) == 0) sfx_at(SFX_BAT, m->x, m->y, 0.6f);
}

/* ---- the caster's spells ---- */

static int wisp_field[MAPN][MAPN];                                  /* steps to the wisps' target, for going round corners */
static int reflect_field[MAPN][MAPN];                               /* the same, to a reflected missile's thrower */
static void field_bfs(int f[MAPN][MAPN], int tx, int ty) {
    static int qx[MAPN * MAPN], qy[MAPN * MAPN];
    for (int y = 0; y < MAPN; y++) for (int x = 0; x < MAPN; x++) f[y][x] = -1;
    int qh = 0, qt = 0;
    f[ty][tx] = 0; qx[qt] = tx; qy[qt] = ty; qt++;
    while (qh < qt) {
        int x = qx[qh], y = qy[qh]; qh++;
        if (f[y][x] > 14) continue;
        for (int d = 0; d < 4; d++) {
            int nx = x + DX[d], ny = y + DY[d];
            if (blocks_light(nx, ny) || cell(nx, ny)->kind == K_PILLAR || f[ny][nx] >= 0) continue;
            f[ny][nx] = f[y][x] + 1; qx[qt] = nx; qy[qt] = ny; qt++;
        }
    }
}
static void wisp_bfs(int tx, int ty) { field_bfs(wisp_field, tx, ty); }

static Shot *new_shot(int kind, float x, float y, float a, float speed, float dmg) {
    for (int i = 0; i < MAXSHOTS; i++) {
        Shot *s = &G.shots[i];
        if (s->alive) continue;
        *s = (Shot){ 1, kind, -1, x, y, 0.0f, cosf(a) * speed, sinf(a) * speed, kind == AT_SPEAR ? 1.0f : 1.6f, dmg, 0, 1 };
        return s;
    }
    return NULL;
}

static void add_beam(int kind, float x0, float y0, float z0, float x1, float y1, float z1) {
    for (int i = 0; i < MAXBEAMS; i++)
        if (!G.beams[i].alive) { G.beams[i] = (Beam){ 1, kind, x0, y0, z0, x1, y1, z1, 0.0f }; return; }
}

static int spell_tier(int sp) { return RUN.gear.atk[sp] < 0 ? 0 : RUN.gear.atk[sp]; }

/* lightning: to the target, then hopping on to others nearby */
static void chain_lightning(int t, float dmg, int tier) {
    uint32_t hit = 0;
    Mob *m = &G.mobs[t];
    float hx = G.px + cosf(G.ang) * 0.35f, hy = G.py + sinf(G.ang) * 0.35f;
    add_beam(AT_CHAIN, hx, hy, 0.42f, m->x, m->y, m->z + 0.5f);
    hit |= 1u << t;
    damage_mob(m, dmg, DM_ARC | DM_ATK(AT_CHAIN), G.px, G.py, 0.0f);
    int hops = 2 + (tier >= 4) + (tier >= 7);
    float cx = m->x, cy = m->y, cz = m->z;
    for (int h = 0; h < hops; h++) {
        int best = -1; float bd = 3.5f;
        for (int i = 0; i < MAXMOBS; i++) {
            Mob *o = &G.mobs[i];
            if ((hit >> i & 1) || !o->alive || o->state != MS_WALK || KIND_IS(o, KF_SPIRIT)) continue;
            float d = hypotf(o->x - cx, o->y - cy);
            if (d < bd && line_clear(cx, cy, o->x, o->y)) { bd = d; best = i; }
        }
        if (best < 0) break;
        Mob *o = &G.mobs[best];
        add_beam(AT_CHAIN, cx, cy, cz + 0.5f, o->x, o->y, o->z + 0.5f);
        hit |= 1u << best;
        dmg *= 0.75f;
        damage_mob(o, dmg, DM_ARC | DM_ATK(AT_CHAIN), cx, cy, 0.0f);
        cx = o->x; cy = o->y; cz = o->z;
    }
    snd_sfx_ex(SFX_ZAP, 1.2f, 0.5f);
}

/* the BFS9000: everything the caster can see collapses into a point of light */
static void bfs9000(void) {
    int tier = RUN.gear.atk[AT_BFS];
    RUN.souls -= bfs_cost(tier);
    G.bfs_t = 0.001f;
    snd_sfx_ex(SFX_IMPLODE, 1.3f, 0.5f);
    for (int i = 0; i < MAXMOBS; i++) {
        Mob *m = &G.mobs[i];
        if (!m->alive || m->state != MS_WALK || (KIND_IS(m, KF_SPIRIT) && !m->boss)) continue;
        if (hypotf(m->x - G.px, m->y - G.py) > 14.0f || !line_clear(G.px, G.py, m->x, m->y)) continue;
        if (m->boss) { damage_mob(m, m->maxhp * 0.08f, 0, G.px, G.py, 0.0f); continue; }   /* a boss takes a twelfth or so */
        m->hp = 0.0f;
        kill_mob(m);
        m->sub = 77;                                               /* drawn imploding */
#ifdef CRAWL_DEBUG
        bfs_kills++;
#endif
    }
}

/* let the chosen spell go at target t (or straight ahead if it is gone) */
static void fizzle(int sp);
static void say(const char *fmt, ...);
static void cast_spell(int sp, int t) {
    const Gear *g = &RUN.gear;
    Mob *m = t >= 0 ? &G.mobs[t] : &G.mobs[0];
    int have = t >= 0 && m->alive && m->state == MS_WALK;
    float tx = have ? m->x : G.px + cosf(G.ang) * 4, ty = have ? m->y : G.py + sinf(G.ang) * 4;
    float a = atan2f(ty - G.py, tx - G.px);
    if (G.player && sp != AT_CHAIN && sp != AT_DRAIN) a = G.ang;   /* a person's shots go where they look */
    /* what it costs: a bone for every spear; spirit above tier 1, and with too little, it goes at tier-1
     * strength (and heals nothing) */
    if (sp == AT_SPEAR && RUN.gear.bones <= 0) { say("NO BONES FOR THE SPEAR"); fizzle(sp); return; }
    int tier = spell_tier(sp), weak = 0;
    if (sp != AT_BFS && !((sp == AT_CHAIN || sp == AT_DRAIN) && !have) && !spend_spirit(spell_cost(g, sp, tier))) {
        tier = 1; weak = 1;
        if (!G.spirit_said) { G.spirit_said = 1; say("LOW ON SPIRIT: SPELLS CAST WEAK"); }
#ifdef CRAWL_DEBUG
        fprintf(stderr, "EVENT weak_%s t=%.2f\n", ATK_NAME[sp], G.t);
#endif
    }
    if (sp == AT_SPEAR) RUN.gear.bones--;
    float dmg = spell_damage_at(g, sp, tier);
    if (web_strike(sp, dmg * (sp == AT_WISPS ? (float)wisp_count(tier) : 1.0f))) return;   /* held: it strikes the web */
    float hx = G.px + cosf(a) * 0.35f, hy = G.py + sinf(a) * 0.35f;
    int bat = have && m->type == MOB_BAT && !G.player;
    alert_near(8, 8.0f);                                           /* a spell is heard down the halls */
#ifdef CRAWL_DEBUG
    casts[sp]++;
    fprintf(stderr, "EVENT cast_%s t=%.2f\n", ATK_NAME[sp], G.t);
#endif
    switch (sp) {
    case AT_CHAIN: if (have) chain_lightning(t, dmg, tier); else fizzle(sp); return;
    case AT_DRAIN:
        if (have) {
            add_beam(AT_DRAIN, hx, hy, 0.42f, m->x, m->y, m->z + 0.5f);
            float before = m->hp;
            damage_mob(m, dmg, DM_ARC | DM_ATK(AT_DRAIN), G.px, G.py, 0.0f);
            float dealt = before - fmaxf(m->hp, 0.0f);
            float heal = fminf(dealt * fminf(0.8f, 0.5f + 0.05f * tier) * spell_effect(), (float)(G.max_hp - G.hp));
            if (heal > 0.0f && (weak || !spend_spirit(heal * heal_cost(g, tier)))) {   /* healing costs spirit too */
                heal = 0.0f;
                if (!G.spirit_said) { G.spirit_said = 1; say("LOW ON SPIRIT: SPELLS CAST WEAK"); }
            }
            G.hp = clampi(G.hp + (int)(heal + 0.5f), 0, G.max_hp);
            pop_readout();
            snd_sfx_ex(SFX_WHISPER, 0.8f, 0.5f);
        } else fizzle(sp);
        return;
    case AT_BFS: bfs9000(); return;
    case AT_WISPS: {
        int n = wisp_count(tier);
        if (have) wisp_bfs(m->cx, m->cy);
        for (int i = 0; i < n; i++) {
            Shot *s = new_shot(AT_WISPS, hx, hy, a + (i - (n - 1) * 0.5f) * 0.45f, 4.5f, dmg);
            if (s) { s->target = have ? t : -1; s->life = 3.0f; s->tier = tier; }
        }
        snd_sfx_ex(SFX_SHIMMER, 0.6f, 0.5f);
        return;
    }
    default: {
        float speed = sp == AT_SPEAR ? 12.0f : (sp == AT_SPARKS ? 11.0f : (sp == AT_HELL ? 6.0f : (sp == AT_FROST ? 9.0f : 8.0f)));
        if (bat) a += (rndf() - 0.5f) * (sp == AT_SPARKS || sp == AT_FROST ? 0.3f : 0.6f);   /* bats are hard to hit */
        Shot *s = new_shot(sp, hx, hy, a, speed, dmg);
        if (s) s->tier = tier;
        snd_sfx_ex(sp == AT_FROST ? SFX_FROST : SFX_CAST, sp == AT_SPARKS ? 0.6f : (sp == AT_HELL ? 1.3f : 1.0f), 0.5f);
        return;
    }
    }
}

static void update_shots(float dt) {
    const Gear *g = &RUN.gear;
    for (int i = 0; i < MAXSHOTS; i++) {
        Shot *s = &G.shots[i];
        if (!s->alive) continue;
        s->life -= dt;
        if (s->kind == AT_WISPS) {                                 /* homing, round corners if need be */
            Mob *m = s->target >= 0 ? &G.mobs[s->target] : NULL;
            if (m && (!m->alive || m->state != MS_WALK)) { m = NULL; s->target = -1; }
            if (!m) {                                              /* find another */
                float bd = 6.0f;
                for (int k = 0; k < MAXMOBS; k++) {
                    Mob *o = &G.mobs[k];
                    if (!o->alive || o->state != MS_WALK || o->type == MOB_GHOST) continue;
                    float d = hypotf(o->x - s->x, o->y - s->y);
                    if (d < bd && line_clear(s->x, s->y, o->x, o->y)) { bd = d; m = o; s->target = k; }
                }
            }
            if (m) {
                float gx = m->x, gy = m->y;
                int cx = (int)s->x, cy = (int)s->y;
                if (!line_clear(s->x, s->y, m->x, m->y) && wisp_field[cy][cx] > 0) {
                    for (int d = 0; d < 4; d++) {
                        int nx = cx + DX[d], ny = cy + DY[d];
                        if (wisp_field[ny][nx] >= 0 && wisp_field[ny][nx] < wisp_field[cy][cx]) { gx = nx + 0.5f; gy = ny + 0.5f; break; }
                    }
                }
                float want = atan2f(gy - s->y, gx - s->x), have = atan2f(s->vy, s->vx);
                have += clampf(wrap_pi(want - have), -6.0f * dt, 6.0f * dt);
                s->vx = cosf(have) * 4.5f; s->vy = sinf(have) * 4.5f;
            }
        }
        if (s->kind == SHOT_REFLECT && s->target >= 0) {          /* Mirrored: home on the thrower, round corners */
            Mob *m = &G.mobs[s->target];
            if (!m->alive || m->state != MS_WALK) s->target = -1;
            else {
                float gx = m->x, gy = m->y, sp = hypotf(s->vx, s->vy);
                int cx = (int)s->x, cy = (int)s->y;
                if (!line_clear(s->x, s->y, m->x, m->y)) {
                    field_bfs(reflect_field, m->cx, m->cy);
                    for (int d = 0; d < 4 && reflect_field[cy][cx] > 0; d++) {
                        int nx = cx + DX[d], ny = cy + DY[d];
                        if (reflect_field[ny][nx] >= 0 && reflect_field[ny][nx] < reflect_field[cy][cx]) { gx = nx + 0.5f; gy = ny + 0.5f; break; }
                    }
                }
                float want = atan2f(gy - s->y, gx - s->x), have = atan2f(s->vy, s->vx);
                have += clampf(wrap_pi(want - have), -7.0f * dt, 7.0f * dt);
                s->vx = cosf(have) * sp; s->vy = sinf(have) * sp;
            }
        }
        float sdt = G.bslip_t > 0.0f && hypotf(s->x - G.bslip_x, s->y - G.bslip_y) < 3.0f ? dt * 0.2f : dt;   /* the Wraith's Time Slip */
        int n = 1 + (int)(sqrtf(s->vx * s->vx + s->vy * s->vy) * sdt / 0.08f);
        for (int k = 0; k < n && s->alive; k++) {
            float nx = s->x + s->vx * sdt / n, ny = s->y + s->vy * sdt / n;
            if (blocks_light((int)nx, (int)ny) || cell((int)nx, (int)ny)->kind == K_PILLAR) {
                if (s->kind == AT_WISPS) {                         /* wisps slide along the stone */
                    int bx = blocks_light((int)nx, (int)s->y) || cell((int)nx, (int)s->y)->kind == K_PILLAR;
                    int by = blocks_light((int)s->x, (int)ny) || cell((int)s->x, (int)ny)->kind == K_PILLAR;
                    if (bx) s->vx = -s->vx * 0.5f;
                    if (by || !bx) s->vy = -s->vy * 0.5f;
                    break;
                }
                s->alive = 0;
                if (s->kind == AT_HELL) { explode_big(s->x, s->y, 1.6f); splash(s->x, s->y, 1.5f, s->dmg, NULL, AT_HELL); }
                else if (s->kind == AT_FROST) sfx_at(SFX_FROST, s->x, s->y, 0.6f);
                else if (s->kind == SHOT_REFLECT && s->z < 0.5f) sfx_at(SFX_BONES, s->x, s->y, 0.4f);
                else if (s->kind != AT_SPEAR && s->kind != AT_SPARKS) explode(s->x, s->y);
                break;
            }
            s->x = nx; s->y = ny;
            for (int mi = 0; mi < MAXMOBS && s->tier >= 0; mi++) {    /* tier -1: a broken ward's shards, only for show */
                Mob *mb = &G.mobs[mi];
                if (!mb->alive || mb->state != MS_WALK || mb->type == MOB_GHOST) continue;   /* fire passes through wraiths */
                if (s->kind == AT_SPEAR && (s->pierced >> mi & 1)) continue;
                float hr = KINDS[mb->type].rad * (mb->boss ? BOSSES[mb->boss - 1].scale : 1.0f);
                if (hypotf(mb->x - s->x, mb->y - s->y) >= hr) continue;
                if (mb->boss && (G.bdash_t > 0.0f || (mb->type == MOB_HUNTER && (mb->sub == 0 || mb->sub == 5)) || !wraith_open(mb))) continue;   /* darting, or
                                                                                                                         overhead: under it */
                if (knight_turns(mb, s)) break;
                float fx = s->x - s->vx * 0.1f, fy = s->y - s->vy * 0.1f;
                switch (s->kind) {
                case AT_SPEAR:                                     /* through everything in the line, shields too */
                    s->pierced |= 1u << mi;
                    damage_mob(mb, s->dmg, DM_PIERCE | DM_ATK(AT_SPEAR), fx, fy, 0.0f);
                    sfx_at(SFX_BONES, mb->x, mb->y, 0.5f);
                    continue;
                case AT_HELL:
                    s->alive = 0; explode_big(s->x, s->y, 1.6f);
                    damage_mob(mb, s->dmg, DM_ATK(AT_HELL), fx, fy, 0.0f); splash(s->x, s->y, 1.5f, s->dmg, mb, AT_HELL);
                    for (int b = 0; b < MAXBURNS; b++)
                        if (!G.burns[b].alive) { G.burns[b] = (Burn){ 1, s->x, s->y, 0.0f, (4.0f + s->tier) * spell_effect(), 0 }; break; }
                    break;
                case AT_FIRE:
                    s->alive = 0; explode(s->x, s->y);
                    damage_mob(mb, s->dmg, DM_ATK(AT_FIRE), fx, fy, 0.0f);
                    if (s->tier >= 2) splash(s->x, s->y, 0.75f, s->dmg * 0.5f, mb, AT_FIRE);
                    break;
                case AT_FROST:
                    s->alive = 0; sfx_at(SFX_FROST, s->x, s->y, 0.8f);
                    damage_mob(mb, s->dmg, DM_SLOW | DM_ATK(AT_FROST), fx, fy, (2.5f + 0.25f * s->tier) * spell_effect());
                    break;
                default:                                           /* sparks, wisps, reflected missiles */
                    s->alive = 0;
                    if (s->kind == SHOT_REFLECT) explode(s->x, s->y); else sfx_at(SFX_ZAP, s->x, s->y, 0.35f);
                    damage_mob(mb, s->dmg, s->kind == SHOT_REFLECT ? 0 : DM_ATK(s->kind), fx, fy, 0.0f);
                    break;
                }
                break;
            }
        }
        if (s->alive && s->life <= 0) { s->alive = 0; if (s->kind == AT_FIRE || s->kind == AT_HELL) explode(s->x, s->y); }
    }
    for (int i = 0; i < MAXBOOMS; i++) if (G.booms[i].alive && (G.booms[i].t += dt) >= 0.6f) G.booms[i].alive = 0;
    for (int i = 0; i < MAXBEAMS; i++) if (G.beams[i].alive && (G.beams[i].t += dt) >= 0.3f) G.beams[i].alive = 0;
    for (int i = 0; i < MAXBURNS; i++) {                           /* the floor burns a while after hellfire */
        Burn *b = &G.burns[i];
        if (!b->alive) continue;
        float pt = b->t; b->t += dt;
        if (b->t >= 2.5f) { b->alive = 0; continue; }
        if (b->foe) {                                              /* an ember's: it burns whoever stands in it */
            if ((int)(pt * 2.0f) != (int)(b->t * 2.0f) && hypotf(G.px - b->x, G.py - b->y) < 0.6f && hurt_ex(b->dps * 0.5f, HURT_MAGIC, b->x, b->y) > 0) DBG_COUNT(ember_scorch);
            continue;
        }
        if ((int)(pt * 4.0f) != (int)(b->t * 4.0f))
            for (int k = 0; k < MAXMOBS; k++) {
                Mob *m = &G.mobs[k];
                if (m->alive && m->state == MS_WALK && !KIND_IS(m, KF_SPIRIT) && m->type != MOB_WSKULL && hypotf(m->x - b->x, m->y - b->y) < 0.7f)
                    damage_mob(m, b->dps * 0.25f, DM_ARC | DM_ATK(AT_HELL), b->x, b->y, 0.0f);
            }
    }
    (void)g;
}

/* ---- the caster's defences: tattooed runes, each waiting for its moment ---- */

static int incoming(float within) {                                 /* missiles that will reach the caster soon */
    int n = 0;
    for (int i = 0; i < MAXBONES; i++) {
        const Bone *b = &G.bones[i];
        if (!b->alive) continue;
        float dx = G.px - b->x, dy = G.py - b->y, sp = hypotf(b->vx, b->vy) + 1e-3f;
        float along = (dx * b->vx + dy * b->vy) / sp, off = fabsf(dx * b->vy - dy * b->vx) / sp;
        if (along > 0.0f && off < 0.6f && along / sp < within) n++;
    }
    return n;
}
static int enemies_within(float r, int undead_only) {
    int n = 0;
    for (int i = 0; i < MAXMOBS; i++) {
        const Mob *m = &G.mobs[i];
        if (!m->alive || m->state != MS_WALK || m->type == MOB_GHOST || m->type == MOB_WSKULL || m->type == MOB_SAC) continue;
        if ((m->type == MOB_SPIDER || m->type == MOB_GARGOYLE) && m->sub == 0) continue;
        if (undead_only && !KIND_IS(m, KF_UNDEAD)) continue;
        if (hypotf(m->x - G.px, m->y - G.py) < r && line_clear(m->x, m->y, G.px, G.py)) n++;
    }
    return n;
}
static int skull_charging(float r, float after) {
    for (int i = 0; i < MAXMOBS; i++) {
        const Mob *m = &G.mobs[i];
        if (m->alive && m->type == MOB_WSKULL && m->state == MS_WALK && m->sub == 1 && m->sub_t >= after
            && hypotf(m->x - G.px, m->y - G.py) < r && line_clear(m->x, m->y, G.px, G.py)) return 1;
    }
    return 0;
}
static int melee_close(void) {                                      /* a sword raised, or a spider coming down */
    for (int i = 0; i < MAXMOBS; i++) {
        const Mob *m = &G.mobs[i];
        if (!m->alive || m->state != MS_WALK) continue;
        float d = hypotf(m->x - G.px, m->y - G.py);
        if ((m->type == MOB_SHIELD && d < 1.5f) || (m->type == MOB_SPIDER && m->sub >= 1 && m->sub <= 2 && d < 1.4f)) return 1;
    }
    return 0;
}
static int in_combat(void) { return enemies_within(7.0f, 0) > 0 || incoming(2.0f) > 0 || skull_charging(9.0f, 0.0f); }

static int def_trigger(int k) {
    float hpf = (float)G.hp / (float)G.max_hp;
    switch (k) {
    case DF_WARD: return G.ward_hp <= 0.0f && (incoming(0.7f) || skull_charging(7.0f, 1.5f) || melee_close());
    case DF_BLINK: return (melee_close() && hpf < 0.6f) || (incoming(0.4f) && G.ward_hp <= 0.0f && G.mirror_t <= 0.0f && hpf < 0.5f);
    case DF_MIRROR: return G.mirror_t <= 0.0f && (incoming(1.5f) >= 2 || skull_charging(9.0f, 2.0f));
    case DF_MEND: return G.mend_t <= 0.0f && hpf < 0.45f;
    case DF_PURIFY: return G.poison_t > 2.0f || G.slow_t > 1.5f;
    case DF_NOVA: return enemies_within(1.8f, 0) >= 2 || melee_close();
    case DF_DREAD: return enemies_within(4.0f, 1) >= 2;
    case DF_SLIP: return G.slip_t <= 0.0f && (enemies_within(5.0f, 0) + incoming(2.0f) >= 3 || (hpf < 0.35f && in_combat()));
    default: return G.reap_t <= 0.0f && enemies_within(7.0f, 0) >= 2;
    }
}

static void begin_turn(float target);
static float heading_angle(int h);
static int decide(int x, int y, int h);

/* Blink: two to six steps to wherever is furthest from trouble, out of sight if it can */
#define DECOY_T 3.0f                        /* how long an Exquisite Blink's double holds them */
static void blink(void) {
    static int dist[MAPN][MAPN], qx[MAPN * MAPN], qy[MAPN * MAPN];
    int range = clampi(2 + rune_tier(RUN.gear.def[DF_BLINK]) / 3, 2, 6);
    for (int y = 0; y < MAPN; y++) for (int x = 0; x < MAPN; x++) dist[y][x] = -1;
    int qh = 0, qt = 0, bx = -1, by = -1; float bs = -1e9f;
    dist[G.cy][G.cx] = 0; qx[qt] = G.cx; qy[qt] = G.cy; qt++;
    while (qh < qt) {
        int x = qx[qh], y = qy[qh]; qh++;
        if (dist[y][x] >= 2 && G.map[y][x].kind == K_FLOOR && !occupied(x, y, NULL)) {
            float s = dist[y][x] * 0.3f;
            for (int i = 0; i < MAXMOBS; i++) {
                const Mob *m = &G.mobs[i];
                if (!m->alive || m->state != MS_WALK || m->type == MOB_GHOST) continue;
                float d = hypotf(m->x - x - 0.5f, m->y - y - 0.5f);
                s += fminf(d, 6.0f) * 0.5f - (line_clear(m->x, m->y, x + 0.5f, y + 0.5f) ? 1.5f : 0.0f);
            }
            if (s > bs) { bs = s; bx = x; by = y; }
        }
        if (dist[y][x] >= range) continue;
        for (int d = 0; d < 4; d++) {
            int nx = x + DX[d], ny = y + DY[d];
            if (!passable(nx, ny) || dist[ny][nx] >= 0 || cell(nx, ny)->kind == K_DOOR) continue;
            dist[ny][nx] = dist[y][x] + 1; qx[qt] = nx; qy[qt] = ny; qt++;
        }
    }
    if (bx < 0) return;
    if (RUN.gear.def[DF_BLINK] >= RUNE_EXQ) {                      /* Exquisite: a double stays */
        G.decoy_x = G.px; G.decoy_y = G.py; G.decoy_t = DECOY_T;
#ifdef CRAWL_DEBUG
        int nl = 0;
        for (int i = 0; i < MAXMOBS; i++) nl += G.mobs[i].alive && G.mobs[i].state == MS_WALK && hypotf(G.mobs[i].x - G.px, G.mobs[i].y - G.py) < 6.0f;
        fprintf(stderr, "EVENT decoy near=%d t=%.2f\n", nl, G.t);
#endif
    }
    G.cx = G.tx = bx; G.cy = G.ty = by; G.px = bx + 0.5f; G.py = by + 0.5f; G.u = 1.0f;
    G.spd = 0.0f;
    if (!G.player) {
        G.hand = 0.0f;
        G.next_h = decide(G.cx, G.cy, G.h);
        begin_turn(heading_angle(G.next_h)); G.state = PS_TURN;
    }
    G.chill = fmaxf(G.chill, 0.5f);
    snd_sfx_ex(SFX_WHOOSH, 1.0f, 0.5f);
}

static void cast_defense(int k) {
    int q = RUN.gear.def[k], t = rune_tier(q);                     /* by its quality, at about the tier it once was */
    if (q <= 0) return;
    G.def_cd[k] = def_cooldown(q);
    G.rune_glow[k] = 1.4f; G.rune_lift = 1.4f;
    snd_sfx_ex(SFX_RUNE, 0.9f, 0.5f);
#ifdef CRAWL_DEBUG
    defs[k]++;
    fprintf(stderr, "EVENT defense %s q=%d t=%.2f hp=%d\n", DEF_NAME[k], q, G.t, G.hp);
#endif
    float eff = spell_effect();                                    /* the runes' strength */
    switch (k) {
    case DF_WARD: G.ward_hp = (10.0f + 5.0f * t) * eff; G.ward_t = 12.0f; G.ward_shards = q >= RUNE_EXQ ? G.ward_hp * 0.5f : 0.0f; break;
    case DF_BLINK: blink(); break;
    case DF_MIRROR: G.mirror_t = (3.0f + 0.5f * t) * eff; break;
    case DF_MEND: G.mend_t = 4.0f; G.mend_rate = (15.0f + 5.0f * t) / 4.0f * eff; break;
    case DF_PURIFY: G.poison_t = G.slow_t = 0.0f; G.purify_t = (4.0f + t) * eff; break;
    case DF_NOVA:
        for (int i = 0; i < MAXMOBS; i++) {
            Mob *m = &G.mobs[i];
            if (m->alive && m->state == MS_WALK && !KIND_IS(m, KF_SPIRIT) && hypotf(m->x - G.px, m->y - G.py) < 1.8f + 0.1f * t)
                { m->frozen = (2.0f + 0.3f * t) * eff; m->swing = 0.0f; m->throw_t = 0.0f; }
        }
        snd_sfx_ex(SFX_FROST, 1.3f, 0.5f);
        G.nova_t = 0.001f; G.chill = fmaxf(G.chill, 0.55f);
        break;
    case DF_DREAD:
        for (int i = 0; i < MAXMOBS; i++) {
            Mob *m = &G.mobs[i];
            int dead = KIND_IS(m, KF_UNDEAD) || m->type == MOB_WSKULL || m->type == MOB_WRAITH;
            int any = q >= RUNE_EXQ && !KIND_IS(m, KF_SPIRIT) && !buried(m);   /* Exquisite: the living too */
            if (m->alive && m->state == MS_WALK && (dead || any) && hypotf(m->x - G.px, m->y - G.py) < 5.0f)
                { m->feared = (3.0f + 0.5f * t) * eff; m->wfeared = 0; m->dreaded = !dead; if (!dead) DBG_COUNT(exq_dread); }
        }
        G.dread_t = 0.001f;
        snd_sfx_ex(SFX_MOAN, 1.2f, 0.5f);
        break;
    case DF_SLIP: G.slip_t = (3.0f + 0.4f * t) * eff; break;
    default: G.reap_t = (15.0f + t) * eff; break;
    }
}

/* each rune watches for its moment; the caster is only human: sometimes slow, sometimes not at all */
static void update_defense(float dt) {
    const Gear *g = &RUN.gear;
    int fighting = in_combat();
    for (int k = 0; k < NDEF; k++) {
        if (g->def[k] <= 0) continue;
        G.def_cd[k] -= dt; G.def_hush[k] -= dt;
        if (G.def_wait[k] > 0.0f) {
            G.def_wait[k] -= dt;
            if (G.def_wait[k] <= 0.0f && G.def_cd[k] <= 0.0f && !G.dead && !G.silenced) cast_defense(k);
            continue;
        }
        if (G.def_cd[k] > 0.0f || G.def_hush[k] > 0.0f || G.dead || G.silenced) continue;   /* the void: no rune fires */
        if (def_trigger(k)) {
            if (rndf() < 0.12f) { G.def_hush[k] = 1.5f; continue; }   /* missed the moment */
            G.def_wait[k] = 0.15f + 0.55f * rndf() + (rndf() < 0.1f ? 0.8f : 0.0f);
        } else if (fighting && rndf() < dt * 0.004f) cast_defense(k);   /* a nervous, wasted cast */
    }
}

/* poison, slow and the timed spells wearing off; mending and blessed regeneration */
static void update_status(float dt) {
    if (G.poison_t > 0.0f) {
        G.poison_t -= dt;
        G.poison_acc += G.poison_dps * dt;
        while (G.poison_acc >= 1.0f && !G.dead) { G.poison_acc -= 1.0f; hurt_ex(1.0f, HURT_POISON, G.px, G.py); }
    } else G.poison_acc = 0.0f;
    G.slow_t = fmaxf(0.0f, G.slow_t - dt);
    G.purify_t = fmaxf(0.0f, G.purify_t - dt);
    if (G.ward_t > 0.0f && (G.ward_t -= dt) <= 0.0f) G.ward_hp = G.ward_shards = 0.0f;   /* it fades unbroken */
    G.decoy_t = fmaxf(0.0f, G.decoy_t - dt);
    G.mirror_t = fmaxf(0.0f, G.mirror_t - dt); G.mirror_flash = fmaxf(0.0f, G.mirror_flash - dt);
    G.slip_t = fmaxf(0.0f, G.slip_t - dt);
    G.reap_t = fmaxf(0.0f, G.reap_t - dt);
    for (int k = 0; k < NDEF; k++) G.rune_glow[k] = fmaxf(0.0f, G.rune_glow[k] - dt);
    G.rune_lift = fmaxf(0.0f, G.rune_lift - dt);
    if (G.nova_t > 0.0f && (G.nova_t += dt) > 0.6f) G.nova_t = 0.0f;
    if (G.dread_t > 0.0f && (G.dread_t += dt) > 0.8f) G.dread_t = 0.0f;
    float heal = 0.0f, mend = 0.0f;
    if (G.mend_t > 0.0f) { G.mend_t -= dt; mend = G.mend_rate * dt; heal += mend; }
    if (G.purify_t > 0.0f && RUN.gear.def[DF_PURIFY] >= RUNE_EXQ) heal += (float)G.max_hp * 0.02f * dt;   /* Exquisite Purify mends too */
    heal += gear_regen(&RUN.gear) * dt;
    if (mend > 0.0f && G.hp >= G.max_hp && !G.dead && RUN.gear.def[DF_MEND] >= RUNE_EXQ) {   /* Exquisite Mend: past full, a small ward */
        float cap = (float)G.max_hp * 0.25f;
        if (G.ward_hp < cap) {
            if (G.ward_hp <= 0.0f) DBG_COUNT(exq_mendward);
            G.ward_hp = fminf(cap, G.ward_hp + mend); G.ward_t = fmaxf(G.ward_t, 8.0f);
        }
    }
    if (heal > 0.0f && G.hp < G.max_hp && !G.dead) {
        G.regen_acc += heal;
        if (G.regen_acc >= 1.0f) { int h = (int)G.regen_acc; G.regen_acc -= h; G.hp = clampi(G.hp + h, 0, G.max_hp); if (G.mend_t > 0.0f) pop_readout(); }
    }
}

static void update_doors(float dt) {
    for (int i = 0; i < G.ndoors; i++) {
        Door *d = &G.doors[i];
        float pdist = hypotf(d->x + 0.5f - G.px, d->y + 0.5f - G.py);
        if (d->opening) {
            d->open = fminf(1.0f, d->open + dt / 0.85f);
            if (d->open >= 1.0f) { d->opening = 0; d->idle = 0.0f; }
        } else if (d->open > 0.0f) {
            int busy = pdist < 1.6f || (d->locked == 2 && vault_of_room((int)G.px, (int)G.py) == d->color);
            if (d->locked == SEALED) busy = (int)G.px == d->x && (int)G.py == d->y;   /* it shuts behind you */
            for (int m = 0; m < MAXMOBS; m++)
                if (G.mobs[m].alive && ((G.mobs[m].cx == d->x && G.mobs[m].cy == d->y) || (G.mobs[m].tx == d->x && G.mobs[m].ty == d->y))) busy = 1;
            d->idle = busy ? 0.0f : d->idle + dt;
            if (d->idle > 2.5f) {
                if (d->open >= 1.0f) sfx_at(SFX_DOOR, d->x + 0.5f, d->y + 0.5f, 0.6f);
                d->open = fmaxf(0.0f, d->open - dt / 0.85f);
            }
        }
    }
}

/* ================================================================== */
/* The caster's walk                                                   */
/* ================================================================== */

static float heading_angle(int h) { return h * (PI_F * 0.5f); }
static void begin_turn(float target);

static void say(const char *fmt, ...);
static void open_secret(int i) {
    Secret *sc = &G.secrets[i];
    if (sc->state || G.slide >= 0) return;
    sc->state = 1; G.slide = i; G.slide_t = 0.0f;
    G.map[sc->sy][sc->sx].kind = K_FLOOR;                          /* the block is drawn by itself while it moves */
    int nl = G.nlights;
    light_static(sc->sx + 0.5f + DX[sc->d] * 1.5f, sc->sy + 0.5f + DY[sc->d] * 1.5f, 0.75f, 3.4f);   /* the hoard's own glow */
    if (sc->len > 3 || sc->x1 - sc->x0 + sc->y1 - sc->y0 > 4)     /* a bigger room: the far end too */
        light_static((sc->x0 + sc->x1 + 1) * 0.5f + DX[sc->d] * 1.0f, (sc->y0 + sc->y1 + 1) * 0.5f + DY[sc->d] * 1.0f, 0.7f, 4.0f);
    if (sc->door_x >= 0)                                           /* what comes through the doorway */
        light_static(sc->door_x + 0.5f - (sc->door_x - imin(imax(sc->door_x, sc->x0), sc->x1)) * 0.6f,
                     sc->door_y + 0.5f - (sc->door_y - imin(imax(sc->door_y, sc->y0), sc->y1)) * 0.6f, 0.55f, 2.6f);
    for (int l = nl; l < G.nlights; l++) G.lights[l].phase = -1000.0f;   /* steady, not flames */
    G.secrets_found++; G.links_found++;
    rec_secret();
    say("A SECRET!   %d OF %d", G.links_found, G.nlinks);
    if (G.links_found >= G.nlinks) gain_coin(COIN_ALL, "EVERY SECRET HERE FOUND");
    sfx_at(SFX_PUSHWALL, sc->sx + 0.5f, sc->sy + 0.5f, 1.0f);
    snd_sfx_ex(SFX_SECRET, 1.0f, 0.5f);
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT secret %d of %d (room %d, chain of %d) t=%.2f by %s\n", G.links_found, G.nlinks, i, sc->links, G.t, G.player ? "player" : "autopilot");
#endif
}
static void update_slide(float dt) {
    if (G.slide < 0) return;
    Secret *sc = &G.secrets[G.slide];
    G.slide_t += dt * (float)sc->len / SLIDE_TIME;
    if (G.slide_t < (float)sc->len) return;
    int ex = sc->sx + DX[sc->d] * sc->len, ey = sc->sy + DY[sc->d] * sc->len;   /* it comes to rest at the back */
    G.map[ey][ex].kind = K_WALL; G.map[ey][ex].tex = (uint8_t)sc->tex;
    G.map[sc->sy][sc->sx].secret = 0;
    for (int y = sc->y0; y <= sc->y1; y++) for (int x = sc->x0; x <= sc->x1; x++) G.map[y][x].secret = 0;
    sc->state = 2; G.slide = -1;
    for (int i = 0; i < MAXWISPS; i++) G.wisps[i].gx = -1;        /* the closet is part of the crypt now: map the ways again */
}

/* steps from (sx,sy) to the nearest unexplored cell, not passing back through (bx,by) */
static int dist_to_unexplored(int sx, int sy, int bx, int by) {
    static int stamp[MAPN][MAPN], dist[MAPN][MAPN], qx[MAPN * MAPN], qy[MAPN * MAPN], gen;
    gen++;
    int qh = 0, qt = 0;
    stamp[sy][sx] = gen; dist[sy][sx] = 0; qx[qt] = sx; qy[qt] = sy; qt++;
    stamp[by][bx] = gen;
    while (qh < qt) {
        int x = qx[qh], y = qy[qh]; qh++;
        if (G.map[y][x].visits == 0) return dist[y][x];
        for (int d = 0; d < 4; d++) {
            int nx = x + DX[d], ny = y + DY[d];
            if (!apass(nx, ny) || stamp[ny][nx] == gen) continue;
            stamp[ny][nx] = gen; dist[ny][nx] = dist[y][x] + 1; qx[qt] = nx; qy[qt] = ny; qt++;
        }
    }
    return 999;
}

static int bfs_dist[MAPN][MAPN];
static void bfs_from(int sx, int sy) {                             /* along the lanes */
    static int qx[MAPN * MAPN], qy[MAPN * MAPN];
    for (int y = 0; y < MAPN; y++) for (int x = 0; x < MAPN; x++) bfs_dist[y][x] = -1;
    int qh = 0, qt = 0;
    bfs_dist[sy][sx] = 0; qx[qt] = sx; qy[qt] = sy; qt++;
    while (qh < qt) {
        int x = qx[qh], y = qy[qh]; qh++;
        for (int d = 0; d < 4; d++) {
            int nx = x + DX[d], ny = y + DY[d];
            if (!apass(nx, ny) || bfs_dist[ny][nx] >= 0) continue;
            bfs_dist[ny][nx] = bfs_dist[y][x] + 1; qx[qt] = nx; qy[qt] = ny; qt++;
        }
    }
}

/* the way to open vault k: its key (which may be locked up in another vault), then its door */
static int key_goal(int k, int depth, int *gx, int *gy) {
    if (depth > 3 || k < 0 || G.vault_door[k] < 0) return 0;
    const Door *d = &G.doors[G.vault_door[k]];
    if (d->locked != 1) return 0;
    if (G.has_key[k]) { *gx = d->x; *gy = d->y; return 1; }
    for (int i = 0; i < G.nitems; i++) {
        const Item *it = &G.items[i];
        if (!it->alive || it->type != IT_KEY || it->color != k) continue;
        int v = vault_of_room(it->cx, it->cy);
        if (v >= 0 && v != k && G.doors[G.vault_door[v]].locked == 1) return key_goal(v, depth + 1, gx, gy);
        *gx = it->lx; *gy = it->ly;
        return 1;
    }
    return 0;
}

/* a key whose vault is still shut: the caster can hear it chime from a little way off */
static int key_calls(const Item *it) {
    if (it->type != IT_KEY || it->color < 0 || it->color >= G.nvaults || G.vault_door[it->color] < 0) return 0;
    return G.doors[G.vault_door[it->color]].locked == 1;
}

static int worth_taking(const Item *it) {
    if (it->type == IT_SCRIP) return 1;
    if (it->type == IT_POTION) return G.hp < G.max_hp * 4 / 5;      /* saved for when it is needed */
    if (it->type == IT_BONE) return RUN.gear.atk[AT_SPEAR] >= 0 && RUN.gear.bones < BONES_MAX;
    return 1;
}

/* The autopilot and the Black Altar: it wakes one with 25 spirit or more (keeping 15 for its spells)
 * while there is time to follow its wisp, blesses its exit wisps with 40 or more when the level has
 * undead, and anoints when it burns gold. It asks from the lane beside it. */
#define ALTAR_TIME 240.0                    /* past this, it leaves the altar and its secrets be */
#define WAKE_TIME 180.0                     /* and past this, it is too late to wake one */
static int altar_wake(void), altar_bless(void), altar_anoint(void), altar_send(void), nearest_shut(float x, float y), altar_price(void);
static int altar_spot(int *sx, int *sy) {
    if (G.shop || G.altar_x < 0) return 0;
    for (int d = 0; d < 4; d++) {
        int x = G.altar_x + DX[d], y = G.altar_y + DY[d];
        if (apass(x, y)) { *sx = x; *sy = y; return 1; }
    }
    return 0;
}
static int undead_about(void) {
    for (int i = 0; i < MAXMOBS; i++) if (G.mobs[i].alive && KIND_IS(&G.mobs[i], KF_UNDEAD)) return 1;
    return 0;
}
static int wants_wake(void) { return G.altar_state == ALTAR_DORMANT && G.level_t <= WAKE_TIME && spirit() >= 25.0f && G.secrets_found < G.nsecrets; }
static int wants_bless(void) { return !G.blessed && G.level_t <= ALTAR_TIME && spirit() >= 40.0f && undead_about(); }
static int wants_send(void) {                                      /* its wisp home and resting: on, if 15 spirit is left after */
    return G.altar_state == ALTAR_AWAKE && G.altar_wisp == AW_REST && G.level_t <= ALTAR_TIME
        && nearest_shut(G.altar_x + 0.5f, G.altar_y + 0.5f) >= 0 && spirit() - (float)altar_price() >= 15.0f;
}
static int wants_anoint(void) {
    if (G.altar_state == ALTAR_READY)
        for (int s = 0; s < NSLOT; s++) if (RUN.gear.worn[s].kind && !RUN.gear.worn[s].anointed) return 1;
    return 0;
}
static int altar_wanted(void) {
    if (G.player || G.leaving || G.shop || G.altar_x < 0) return 0;
    return wants_anoint() || wants_wake() || wants_send() || wants_bless();
}

/* The autopilot in a chain (docs/PLAN-b6.md): it goes through an open chained room's doorway while it
 * has two thirds of its health and before 180 s, and in the pockets takes the treasure it sees near,
 * then the visportal's gift (or the cooler's drink) at the end, or the way on; it turns back when its
 * health falls under two thirds, when it has the gift, or after CHAIN_TIME in a level's chains. It
 * stands on the lane beside the cell before a doorway or the visportal and turns to use it (PS_USE). */
enum { USE_NONE, USE_DOOR, USE_VIS };
#define CHAIN_TIME 150.0f
static int pocket_go(int x, int y);
static int visportal_use(void);
static int use_spot(int tx, int ty, int *sx, int *sy) {
    for (int d = 0; d < 4; d++) {
        int fx = tx + DX[d], fy = ty + DY[d];
        if (fx < 1 || fy < 1 || fx >= MAPN - 1 || fy >= MAPN - 1 || !passable(fx, fy)) continue;
        if (apass(fx, fy)) { *sx = fx; *sy = fy; return 1; }
        for (int e = 0; e < 4; e++) if (apass(fx + DX[e], fy + DY[e])) { *sx = fx + DX[e]; *sy = fy + DY[e]; return 1; }
    }
    return 0;
}
static int chain_goal(int x, int y, int *gx, int *gy) {
    if (G.player || G.shop) return 0;
    int sx, sy;
    G.use_kind = USE_NONE;
    bfs_from(x, y);
    if (!PK.depth) {
        if (G.leaving || G.level_t > 180.0 || G.hp * 3 < G.max_hp * 2 || PK.auto_t >= CHAIN_TIME) return 0;
        int best = -1, bd = 41;
        for (int i = 0; i < G.nsecrets; i++) {
            const Secret *sc = &G.secrets[i];
            if (sc->state != 2 || sc->door_x < 0 || PK.auto_done[i] || !use_spot(sc->door_x, sc->door_y, &sx, &sy)) continue;
            int d = bfs_dist[sy][sx];
            if (d >= 0 && d < bd) { bd = d; best = i; *gx = sx; *gy = sy; }
        }
        if (best < 0) return 0;
        G.use_x = G.secrets[best].door_x; G.use_y = G.secrets[best].door_y; G.use_kind = USE_DOOR;
        return 1;
    }
    if (!PK.auto_back && (G.hp * 3 < G.max_hp * 2 || PK.auto_t >= CHAIN_TIME || (G.vis_x >= 0 && PK.vis_used[PK.chain]))) {
        PK.auto_back = 1;
#ifdef CRAWL_DEBUG
        fprintf(stderr, "EVENT chain_back %s link %d hp=%d/%d chain_t=%.1f t=%.2f\n", G.hp * 3 < G.max_hp * 2 ? "hurt" : PK.auto_t >= CHAIN_TIME ? "time" : "done",
                PK.depth, G.hp, G.max_hp, PK.auto_t, G.t);
#endif
    }
    int tx = PK.back_x, ty = PK.back_y, kind = USE_DOOR;
    if (!PK.auto_back) {
        int bi = -1, bd = 17;                                      /* treasure it has seen, close by */
        for (int i = 0; i < G.nitems; i++) {
            const Item *it = &G.items[i];
            if (!it->alive || !it->seen || !worth_taking(it) || (it->lx == x && it->ly == y)) continue;
            int d = bfs_dist[it->ly][it->lx];
            if (d >= 0 && d < bd) { bd = d; bi = i; }
        }
        if (bi >= 0) { *gx = G.items[bi].lx; *gy = G.items[bi].ly; return 1; }
        if (G.vis_x >= 0) { tx = G.vis_x; ty = G.vis_y; kind = USE_VIS; }
        else if (PK.on_x >= 0) { tx = PK.on_x; ty = PK.on_y; }
    }
#ifdef CRAWL_DEBUG
    { static double next; if (G.t >= next) { next = G.t + 3.0; int ok = use_spot(tx, ty, &sx, &sy);
      fprintf(stderr, "CHAIN at (%d,%d) to (%d,%d) kind %d spot %d (%d,%d) dist %d back %d t=%.1f\n", x, y, tx, ty, kind, ok, sx, sy, ok ? bfs_dist[sy][sx] : -9, PK.auto_back, G.t); } }
#endif
    if (!use_spot(tx, ty, &sx, &sy) || bfs_dist[sy][sx] < 0) return 0;
    G.use_x = tx; G.use_y = ty; G.use_kind = kind;
    *gx = sx; *gy = sy;
    return 1;
}

/* something worth walking to from (x,y): a vault door once its key is held, else treasure it has seen */
/* the autopilot in a boss's arena: its scrip once it falls; a full font when badly hurt; otherwise a
 * crossing of lanes three to six cells from the boss and away from its adds, changed every few seconds
 * so that what is thrown or swung finds nothing standing */
static int boss_goal(int x, int y, int *gx, int *gy) {
    if (G.boss < 0 || PK.depth || !in_arena(x + 0.5f, y + 0.5f)) return 0;
    if (G.bstate == BS_DEAD) {
        for (int i = 0; i < G.nitems; i++)
            if (G.items[i].alive && G.items[i].type == IT_SCRIP) { *gx = G.items[i].lx; *gy = G.items[i].ly; return 1; }
        return 0;
    }
    if (!boss_fighting()) return 0;
    if (G.hp < G.max_hp * 0.4f)
        for (int i = 0; i < G.bnfont; i++)
            if (G.bfont_full[i] && G.bfont_fx[i] >= 0 && apass(G.bfont_fx[i], G.bfont_fy[i])) { *gx = G.bfont_fx[i]; *gy = G.bfont_fy[i]; return 1; }
    const Mob *b = boss_mob();
    float bx = b ? b->x : (float)G.exit_fx, by = b ? b->y : (float)G.exit_fy;
    if (G.bstate == BS_WAVE && G.badds > 0) {                      /* adds out and none near: go and find the nearest */
        float nd = 1e9f; const Mob *na = NULL;
        for (int i = 0; i < MAXMOBS; i++) {
            const Mob *o = &G.mobs[i];
            float d = hypotf(o->x - G.px, o->y - G.py);
            if (o->alive && o->add == 1 && o->state == MS_WALK && d < nd) { nd = d; na = o; }
        }
        if (na && nd > 5.0f) { bx = na->x; by = na->y; }
    }
    if (G.boss == BOSS_WRAITH && G.bstate == BS_FIGHT) {           /* the Wraith's: under four lit, kindle the nearest out */
        int lit = 0, nb = -1; float nd = 1e9f;
        for (int k = 0; k < G.bbz_n; k++) {
            float d = hypotf(G.bbz_x[k] + 0.5f - G.px, G.bbz_y[k] + 0.5f - G.py);
            lit += G.bbz_lit[k];
            if (!G.bbz_lit[k] && d < nd) { nd = d; nb = k; }
        }
        if (lit < 4 && nb >= 0) {
            int bc = -1; float bdd = 1e9f;
            for (int cy = G.bbz_y[nb] - 2; cy <= G.bbz_y[nb] + 2; cy++)
                for (int cx = G.bbz_x[nb] - 2; cx <= G.bbz_x[nb] + 2; cx++) {
                    float dd = hypotf(cx - G.bbz_x[nb], cy - G.bbz_y[nb]);
                    if (cx < 0 || cy < 0 || cx >= MAPN || cy >= MAPN || dd > 1.9f || !apass(cx, cy) || !in_arena(cx + 0.5f, cy + 0.5f)) continue;
                    if (dd < bdd) { bdd = dd; bc = cy * MAPN + cx; }
                }
            if (bc >= 0 && bc != y * MAPN + x) { *gx = bc % MAPN; *gy = bc / MAPN; return 1; }
            if (bc >= 0) return 0;                                 /* there: stand and kindle it */
        }
    }
    static int keep_x = -1, keep_y = -1; static double keep_t = -1.0;
    if (keep_x >= 0 && G.t < keep_t && apass(keep_x, keep_y) && in_arena(keep_x + 0.5f, keep_y + 0.5f)
        && fabsf(hypotf(keep_x + 0.5f - bx, keep_y + 0.5f - by) - 4.5f) < 2.0f && (keep_x != x || keep_y != y)) { *gx = keep_x; *gy = keep_y; return 1; }
    int best = -1; float bs = 1e9f;
    for (int cy = BAND_Y0; cy < MAPN; cy++)
        for (int cx = ARENA_X0; cx < MAPN; cx++) {
            if ((cx & 3) != 2 || (cy & 3) != 2 || !apass(cx, cy) || (cx == x && cy == y)) continue;
            float d = hypotf(cx + 0.5f - bx, cy + 0.5f - by), sc = fabsf(d - 4.5f) * 2.0f + 0.25f * (float)(abs(cx - x) + abs(cy - y)) + rndf();
            for (int i = 0; i < MAXMOBS; i++)
                if (G.mobs[i].alive && G.mobs[i].add && G.mobs[i].state == MS_WALK && hypotf(G.mobs[i].x - cx - 0.5f, G.mobs[i].y - cy - 0.5f) < 2.5f) sc += 4.0f;
            if (G.boss == BOSS_WRAITH && !wraith_lit(cx + 0.5f, cy + 0.5f)) sc += 3.0f;   /* in the light, where it can be hurt */
            if (sc < bs) { bs = sc; best = cy * MAPN + cx; }
        }
    if (best < 0) return 0;
    keep_x = best % MAPN; keep_y = best / MAPN; keep_t = G.t + 3.0 + rndf() * 2.0;
    *gx = keep_x; *gy = keep_y;
    return 1;
}

static int pick_goal(int x, int y, int *gx, int *gy) {
    if (boss_goal(x, y, gx, gy)) return 1;
    if (chain_goal(x, y, gx, gy)) return 1;
    int saving = RUN.goal > RUN.treasure;
    if (G.leaving && G.exit_fx >= 0) {                              /* time to go: the exit, by way of key and vault if need be */
        if (G.exit_in_vault >= 0 && key_goal(G.exit_in_vault, 0, gx, gy)) return 1;
        int hurt = G.hp < G.max_hp * 0.35f;                         /* badly hurt, it stops only for a potion */
        if (G.level_t < (G.hp >= G.max_hp * 0.6f ? 350.0 : 320.0)) {   /* but first, treasure it remembers seeing */
            for (int k = 0; k < G.nvaults && !hurt; k++)            /* a key in hand opens a hoard on the way out */
                if (G.has_key[k] && G.vault_door[k] >= 0 && G.doors[G.vault_door[k]].locked == 1) {
                    *gx = G.doors[G.vault_door[k]].x; *gy = G.doors[G.vault_door[k]].y;
                    return 1;
                }
            bfs_from(x, y);
            int best = -1, bd = 31;
            for (int i = 0; i < G.nitems; i++) {
                const Item *it = &G.items[i];
                if (!it->alive || !worth_taking(it) || (it->lx == x && it->ly == y) || (hurt && it->type != IT_POTION)) continue;
                int d = bfs_dist[it->ly][it->lx];
                if (it->type == IT_KEY) { if (d < 0 || d > 22 || !key_calls(it)) continue; d = imax(0, d - 8); }   /* a vault's worth: worth the walk */
                else if (!it->seen) continue;
                if (d >= 0 && d < bd) { bd = d; best = i; }
            }
            if (best >= 0) { *gx = G.items[best].lx; *gy = G.items[best].ly; return 1; }
        }
        *gx = G.exit_fx; *gy = G.exit_fy;
        return 1;
    }
    for (int k = 0; k < G.nvaults; k++)                             /* a key in hand: straight to its door */
        if (G.has_key[k] && G.vault_door[k] >= 0 && G.doors[G.vault_door[k]].locked == 1) {
            *gx = G.doors[G.vault_door[k]].x; *gy = G.doors[G.vault_door[k]].y;
            return 1;
        }
    bfs_from(x, y);
    int ax, ay;
    if (altar_wanted() && altar_spot(&ax, &ay) && bfs_dist[ay][ax] > 0
        && bfs_dist[ay][ax] <= (G.altar_state == ALTAR_READY ? 30 : 24)) { *gx = ax; *gy = ay; return 1; }   /* the altar, when it passes */
    if (!G.player && G.altar_state == ALTAR_AWAKE && G.altar_wisp == AW_SEEK && G.altar_seek >= 0 && !G.secrets[G.altar_seek].state
        && G.level_t < ALTAR_TIME) {                               /* where the violet wisp leads */
        const Secret *sc = &G.secrets[G.altar_seek];
        if (bfs_dist[sc->fy][sc->fx] > 0) { G.seek_secret = G.altar_seek; *gx = sc->fx; *gy = sc->fy; return 1; }
    }
    int best = -1; float bs = 1e9f;
    int reach = saving ? 30 : 24;                                   /* gold buys things now: it goes out of its way */
    for (int i = 0; i < G.nitems; i++) {
        const Item *it = &G.items[i];
        if (!it->alive || (it->lx == x && it->ly == y) || !worth_taking(it)) continue;
        int d = bfs_dist[it->ly][it->lx];
        if (!it->seen && !(it->type == IT_KEY && d >= 0 && d <= 14 && key_calls(it))) continue;   /* a key's jingle carries */
        if (d < 0 || d > reach) continue;
        float sc = (float)d - (it->type == IT_KEY ? 8.0f : 0.0f) - (it->type == IT_POTION ? 6.0f : 0.0f)
                 - (it->type == IT_CROWN || it->type == IT_CHEST || it->type == IT_RING ? 4.0f : 0.0f);
        if (sc < bs) { bs = sc; best = i; }
    }
    if (best < 0) return 0;
    *gx = G.items[best].lx; *gy = G.items[best].ly;
    return 1;
}

/* which way to leave (x,y): towards a goal if there is one, else new ground first,
 * otherwise towards the nearest unexplored cell */
static int hz_danger(int x, int y);
static int decide(int x, int y, int h) {
    int gx, gy;
#ifdef CRAWL_DEBUG
    if (getenv("CRAWL_GOALS")) { int ok = pick_goal(x, y, &gx, &gy); fprintf(stderr, "GOAL at (%d,%d) -> %d (%d,%d) t=%.2f\n", x, y, ok, ok ? gx : -1, ok ? gy : -1, G.t); }
#endif
    if (pick_goal(x, y, &gx, &gy)) {
        bfs_from(gx, gy);
        int gbest = -1, gs = 1 << 30;
        for (int d = 0; d < 4; d++) {
            int nx = x + DX[d], ny = y + DY[d];
            if (!apass(nx, ny) || bfs_dist[ny][nx] < 0) continue;
            int sc = bfs_dist[ny][nx] * 4 + (d == h ? 0 : 1) + (d == ((h + 2) & 3) ? 2 : 0) + (G.bulky[ny][nx] ? 5 : 0) + 400 * hz_danger(nx, ny);   /* not into a warning */
            if (sc < gs) { gs = sc; gbest = d; }
        }
        if (gbest >= 0 && bfs_dist[y][x] > 0) return gbest;
    }
    int best = -1; float bs = -1e9f;
    for (int d = 0; d < 4; d++) {
        if (d == ((h + 2) & 3)) continue;
        int nx = x + DX[d], ny = y + DY[d];
        if (!apass(nx, ny)) continue;
        float s;
        if (G.map[ny][nx].visits == 0) s = 100.0f + (d == h ? 1.3f : 0.0f) + rndf() * 1.6f;
        else s = -(float)dist_to_unexplored(nx, ny, x, y) + (d == h ? 0.4f : 0.0f) + rndf() * 0.8f;
        if (G.bulky[ny][nx]) s -= 2.5f;                           /* walk past piles, not through them */
        s -= 400.0f * (float)hz_danger(nx, ny);
        if (s > bs) { bs = s; best = d; }
    }
    return best < 0 ? ((h + 2) & 3) : best;
}

static void start_move(void) {
    G.tx = G.cx + DX[G.h]; G.ty = G.cy + DY[G.h];
    if (!apass(G.tx, G.ty)) {                                   /* should not happen: turn instead */
        G.next_h = decide(G.cx, G.cy, G.h);
        G.tx = G.cx; G.ty = G.cy; G.u = 1.0f;
        G.state = PS_MOVE;
        return;
    }
    G.next_h = decide(G.tx, G.ty, G.h);
    G.u = 0.0f;
    Cell *c = &G.map[G.ty][G.tx];
    if (c->kind == K_DOOR && G.doors[c->door].open < 0.95f) {
        Door *d = &G.doors[c->door];
        if (d->locked == 1) { d->locked = 2; G.has_key[d->color] = 0; G.unlock_t = G.level_t; snd_sfx(SFX_UNLOCK); pop_readout(); }
        if (!d->opening) { d->opening = 1; sfx_at(SFX_DOOR, d->x + 0.5f, d->y + 0.5f, 1.0f); }
        G.state = PS_DOOR;
        G.spd = 0.0f;
        return;
    }
    G.state = PS_MOVE;
}

/* returns 1 when this step brought the caster to the exit, which takes over from walking */
static int arrive(void) {
    G.cx = G.tx; G.cy = G.ty;
    G.px = G.cx + 0.5f; G.py = G.cy + 0.5f;
    if (G.leaving && !G.exiting && G.cx == G.exit_fx && G.cy == G.exit_fy && !boss_sealed()) {   /* the way out: face it, open it, go */
        G.exiting = 1; G.exit_t = 0.0f; G.u = 1.0f;
        begin_turn(heading_angle(G.exit_dir)); G.adur = 0.4f;
        G.state = PS_EXIT; G.spd = 0.0f; G.hand = 0.0f;
        return 1;
    }
    for (int i = 0; i < G.bnfont && boss_fighting(); i++)          /* before a full font, badly hurt: a draught */
        if (G.cx == G.bfont_fx[i] && G.cy == G.bfont_fy[i] && G.bfont_full[i] && G.hp < G.max_hp * 0.4f) font_drink(i);
    if (G.map[G.cy][G.cx].visits < 250) {
        if (G.map[G.cy][G.cx].visits == 0) G.explored++;
        G.map[G.cy][G.cx].visits++;
    }
    for (int y = G.cy - 1; y <= G.cy + 1; y++)                     /* the lanes beside it are seen in passing */
        for (int x = G.cx - 1; x <= G.cx + 1; x++)
            if (apass(x, y) && G.map[y][x].visits == 0) { G.map[y][x].visits = 1; G.explored++; }
    for (int i = 0; i < G.nsecrets && G.seek_secret < 0 && !G.leaving; i++) {   /* the cracks catch its eye: every one while it */
        Secret *sc = &G.secrets[i];                                /* has time, a third of them after */
        if (sc->state || sc->rolled || abs(G.cx - sc->fx) + abs(G.cy - sc->fy) > 1) continue;
        sc->rolled = 1;
        if (G.level_t < 180.0 || rndf() < 0.35f) G.seek_secret = i;
    }
    if (G.seek_secret >= 0) {                                      /* one step across to stand before it */
        const Secret *sc = &G.secrets[G.seek_secret];
        for (int d = 0; d < 4; d++) if (G.cx + DX[d] == sc->fx && G.cy + DY[d] == sc->fy) G.next_h = d;
    }
    return 0;
}

/* at the lane beside a doorway or the visportal it means to use: face it, and go through or ask */
static int at_use(void) {
    int sx, sy;
    if (G.use_kind == USE_NONE || G.player || !use_spot(G.use_x, G.use_y, &sx, &sy) || G.cx != sx || G.cy != sy) return 0;
    begin_turn(atan2f(G.use_y + 0.5f - G.py, G.use_x + 0.5f - G.px));
    G.state = PS_USE; G.spd = 0.0f; G.fired = 0; G.u = 1.0f;
    return 1;
}

/* beside the altar with something to ask of it: face it, and stop a moment */
static int at_altar(void) {
    int ax, ay;
    if (!altar_wanted() || !altar_spot(&ax, &ay) || G.cx != ax || G.cy != ay) return 0;
    begin_turn(atan2f(G.altar_y + 0.5f - G.py, G.altar_x + 0.5f - G.px));
    G.state = PS_ALTAR; G.spd = 0.0f; G.fired = 0; G.u = 1.0f;
    return 1;
}

/* standing before the secret it means to press: face it, press, wait for the stone to move */
static int at_secret(void) {
    if (G.seek_secret < 0) return 0;
    const Secret *sc = &G.secrets[G.seek_secret];
    if (G.cx != sc->fx || G.cy != sc->fy) return 0;
    begin_turn(heading_angle(sc->d));
    G.state = PS_PUSH; G.spd = 0.0f; G.fired = 0; G.u = 1.0f;
    return 1;
}

static void advance(float du) {                     /* move along the current step, with head bob and footsteps */
    G.u += du;
    float k = fminf(G.u, 1.0f);
    G.px = lerpf(G.cx + 0.5f, G.tx + 0.5f, k);
    G.py = lerpf(G.cy + 0.5f, G.ty + 0.5f, k);
    float ph0 = G.bobph;
    G.bobph += du * TAU_F;
    if (floorf(ph0 / PI_F) != floorf(G.bobph / PI_F)) footstep();
}

static void begin_turn(float target) {
    G.ang0 = G.ang;
    G.ang1 = G.ang + wrap_pi(target - G.ang);
    float delta = fabsf(G.ang1 - G.ang0);
    G.adur = delta > 2.0f ? 0.55f : (delta > 0.2f ? 0.30f : 0.05f);
    G.st = 0.0f;
}

/* can the caster pick this one out? a spider in its web shows only its eyes, and a wall skull
 * asleep is only a carving: neither, until it stirs */
static int targetable(const Mob *m) {
    if (!m->alive || m->state != MS_WALK || m->type == MOB_GHOST || buried(m) || shade_hidden(m)) return 0;
    if (m->type == MOB_GARGOYLE && m->sub == 0) return 0;          /* only a statue, as far as anyone can tell */
    if ((m->type == MOB_SPIDER || m->type == MOB_WSKULL) && m->sub == 0) return 0;
    if (m->boss && G.bstate != BS_FIGHT) return 0;                 /* untouchable: its adds first */
    if (m->boss && m->type == MOB_HUNTER && (m->sub == 0 || m->sub == 5)) return 0;   /* overhead */
    if (m->boss && !wraith_open(m)) return 0;                      /* the Wraith in the dark, or in the stone */
    return 1;
}

/* what to throw at: anything ahead, plus whatever is attacking from the side or behind
 * (a skeleton that has spotted the caster, a bat at its face, a sword, a spider, a skull's fire) */
static int find_target(void) {
    float dx = cosf(G.ang), dy = sinf(G.ang), rx = -dy, ry = dx;
    int best = -1; float bd = 1e9f;
    for (int i = 0; i < MAXMOBS; i++) {
        Mob *m = &G.mobs[i];
        if (!targetable(m)) continue;
        float vx = m->x - G.px, vy = m->y - G.py, dist = sqrtf(vx * vx + vy * vy);
        float depth = vx * dx + vy * dy, lat = vx * rx + vy * ry, score;
        if (depth >= 0.35f && depth <= 7.5f && fabsf(lat) <= fmaxf(depth * 0.42f, 0.3f)) score = depth;
        else if ((m->type == MOB_SKEL && m->seen && dist < 8.0f) || (m->type == MOB_BAT && dist < 1.4f) || (m->type == MOB_SHIELD && m->seen && dist < 3.0f)
                 || (m->type == MOB_SPIDER && m->sub >= 1 && dist < 2.0f) || (m->type == MOB_WSKULL && m->sub >= 1 && dist < 9.0f)
                 || (m->type == MOB_WRAITH && m->sub == 0 && dist < 7.0f) || (KIND_IS(m, KF_MELEE) && m->alert > 0.0f && dist < 4.0f)
                 || ((m->boss || m->add) && dist < 9.0f)) score = dist + 1.5f;   /* in a boss's fight, whatever is in it */
        else continue;
        if (m->type == MOB_WSKULL) score -= m->sub == 1 ? 4.0f : 1.5f;   /* one hit ends it: first, before it fires */
        if (m->type == MOB_DEACON) score -= 2.0f;                  /* the priest first, before it raises more */
        if (m->ignore > (float)G.t || !shot_clear(m->x, m->y)) continue;
        if (score < bd) { bd = score; best = i; }
    }
    return best;
}

/* which spell for this target: a crowd, a shield, a bat, a skull, or low on health */
static int choose_spell(int t) {
    const Gear *g = &RUN.gear;
    const Mob *m = &G.mobs[t];
    if (m->type == MOB_WRAITH && !m->boss) return g->atk[AT_HELL] >= 0 ? AT_HELL : AT_FIRE;   /* only fire turns it */
    float hpf = (float)G.hp / (float)G.max_hp;
    int vis = 0, close = 0, near = 0, packed = 0;
    for (int i = 0; i < MAXMOBS; i++) {
        const Mob *o = &G.mobs[i];
        if (!targetable(o)) continue;
        float d = hypotf(o->x - G.px, o->y - G.py);
        if (d < 10.0f && line_clear(G.px, G.py, o->x, o->y)) { vis++; if (d < 4.0f) close++; }
        if (o != m && hypotf(o->x - m->x, o->y - m->y) < 3.5f) { near++; if (hypotf(o->x - m->x, o->y - m->y) < 1.5f) packed++; }
    }
    if (g->atk[AT_BFS] >= 0 && RUN.souls >= bfs_cost(g->atk[AT_BFS]) && (vis >= 4 || (hpf < 0.3f && close >= 2))) return AT_BFS;
#ifdef CRAWL_DEBUG
    if (getenv("CRAWL_BFSANY") && g->atk[AT_BFS] >= 0 && RUN.souls >= bfs_cost(g->atk[AT_BFS]) && vis >= 2) return AT_BFS;
#endif
    int owned[NATK], no = 0, pick = AT_FIRE;
    float best = -1.0f;
    int shielded = m->type == MOB_SHIELD && m->shield > 0.0f;
    float have = spirit();
    for (int sp = 0; sp < AT_BFS; sp++) {
        if (g->atk[sp] < 0 || (sp == AT_SPEAR && g->bones <= 0)) continue;   /* no bones, no spears */
        owned[no++] = sp;
        int tier = g->atk[sp];
        float cost = spell_cost(g, sp, tier);
        if (cost > have) { tier = 1; cost = 0.0f; }                 /* what it would really get */
        float v = spell_damage_at(g, sp, tier) / spell_cycle(g, sp);
        if (sp == AT_WISPS) v *= (float)wisp_count(tier);
        if (m->type == MOB_WSKULL) v = 1.0f / spell_cycle(g, sp);   /* any hit will do: the quickest */
        switch (sp) {
        case AT_SPEAR: if (shielded) v *= 2.2f; if (near) v *= 1.3f; break;
        case AT_CHAIN: v *= 1.0f + 0.6f * (float)(near < 3 ? near : 3); if (shielded) v *= 1.8f; break;
        case AT_HELL: v *= 1.0f + 0.8f * packed; break;
        case AT_DRAIN: if (hpf < 0.45f && have > 5.0f) v *= 2.2f; break;   /* the healing costs spirit too */
        case AT_FROST: if (m->type == MOB_BAT || m->type == MOB_SPIDER || m->type == MOB_SHIELD) v *= 1.2f; break;   /* slowing what closes in */
        default: break;
        }
        if (m->type != MOB_WSKULL) v *= hit_mult(m, sp);           /* what it is weak to, the elements worn too */
        if (m->boss == 1 + BOSS_KNIGHT && sp != AT_CHAIN && sp != AT_DRAIN) v *= 0.8f;   /* its Mirror Rune turns missiles */
        if (sp == AT_WISPS) v *= 0.75f;                            /* not every wisp gets there */
        v /= 1.0f + cost * (have < 15.0f ? 1.0f : 0.25f);          /* thriftier as spirit runs low */
        if (sp == G.spell) v *= 0.8f;                              /* and a little variety */
        v *= 0.85f + 0.3f * rndf();
        if (v > best) { best = v; pick = sp; }
    }
    if (no > 1 && rnd(100) < 12) pick = owned[rnd(no)];             /* a whim */
    return pick;
}

static void aim_at(int t) {
    G.target = t; G.resume_state = PS_MOVE;
    G.spell = choose_spell(t);
    float cyc = spell_cycle(&RUN.gear, G.spell);
    G.cast_len = fminf(0.75f, fmaxf(0.3f, cyc * 0.42f));
    begin_turn(atan2f(G.mobs[t].y - G.py, G.mobs[t].x - G.px));
    G.adur = 0.18f + 0.13f * fabsf(G.ang1 - G.ang0);
    G.state = PS_AIM; G.spd = 0.0f;
}

static void pocket_return(int died);
static void update_player(float dt) {
    if (G.web_hp > 0.0f && !G.dead && (G.web_idle += dt) > 1.2f) {   /* held, and nothing to aim at: it tears at the web */
        G.web_idle = 0.0f;
        cast_spell(AT_FIRE, -1);
    }
    G.cooldown -= dt;
    G.calm += dt;
    if ((G.state == PS_AIM || G.state == PS_HOLD || G.state == PS_UNAIM || (G.state == PS_CAST && G.fired) || (G.state == PS_CAST && G.silenced))
        && hz_danger((int)G.px, (int)G.py)) {                      /* the floor warns: it stops aiming and goes on */
        G.hand = 0.0f; G.state = PS_MOVE; G.ang = wrap_pi(G.ang);
        begin_turn(heading_angle(G.h)); G.state = PS_TURN; G.st = 0.0f; G.adur = 0.15f;
    }
    switch (G.state) {
    case PS_MOVE: {
        int keyed = G.has_key[0] || G.has_key[1] || G.has_key[2];
        float target = (keyed ? CRUISE * 1.25f : CRUISE) * gear_haste(&RUN.gear) * slow_factor();   /* a key in hand: hurry */
        if (G.next_h != G.h && G.u > 0.62f) target = CRUISE * 0.45f;
        int blocker = -1;                                          /* something in the way: stop and deal with it */
        for (int i = 0; i < MAXMOBS && G.blocked < 3.0f; i++) {
            Mob *m = &G.mobs[i];
            if (!solid_mob(m) || m->state != MS_WALK) continue;
            float vx = m->x - G.px, vy = m->y - G.py;
            float ahead = vx * cosf(G.ang) + vy * sinf(G.ang), side = -vx * sinf(G.ang) + vy * cosf(G.ang);
            if (ahead > 0.0f && ahead < 1.3f && fabsf(side) < 0.55f && line_clear(G.px, G.py, m->x, m->y)) blocker = i;
        }
        if (blocker >= 0) {
            target = 0.0f; G.spd = fminf(G.spd, 0.3f); G.blocked += dt;
            if (G.cooldown <= 0.0f && !G.silenced) { aim_at(blocker); break; }
        } else G.blocked = 0.0f;
        G.spd += clampf(target - G.spd, -6.0f * dt, 6.0f * dt);
        advance(G.spd * dt);
        if (G.next_h != G.h && G.u >= 0.78f) {
            /* corner ahead: start the quick turn while finishing the step */
            begin_turn(heading_angle(G.next_h));
            G.state = PS_TURN;
            break;
        }
        if (G.u >= 1.0f) {
            if (arrive() || at_secret() || at_altar() || at_use()) break;
            if (G.next_h != G.h) {
                begin_turn(heading_angle(G.next_h));
                G.state = PS_TURN;
            } else start_move();
        }
        if (G.cooldown <= 0.0f && G.state == PS_MOVE && !G.silenced) {
            int t = find_target();
            if (t >= 0) aim_at(t);
        }
        break;
    }
    case PS_TURN: {
        G.st += dt;
        if (G.u < 1.0f) {                                         /* still finishing the step into the corner */
            advance(fmaxf(G.spd, CRUISE * 0.4f) * dt);
            if (G.u >= 1.0f && (arrive() || at_secret() || at_altar() || at_use())) break;
        } else G.spd *= 0.85f;
        float k = smoothstepf(0.0f, 1.0f, G.st / G.adur);
        G.ang = lerpf(G.ang0, G.ang1, k);
        if (G.st >= G.adur) {
            if (G.u < 1.0f) { advance(1.0f - G.u); if (arrive() || at_secret() || at_altar() || at_use()) break; }
            G.ang = wrap_pi(G.ang1);
            G.h = G.next_h;
            G.spd = CRUISE * 0.6f;
            start_move();
        }
        break;
    }
    case PS_DOOR: {
        Cell *c = cell(G.tx, G.ty);
        if (c->kind != K_DOOR || G.doors[c->door].open >= 0.95f) G.state = PS_MOVE;
        break;
    }
    case PS_AIM: {
        G.st += dt;
        Mob *m = &G.mobs[G.target];
        if (m->alive) G.ang1 = G.ang0 + wrap_pi(atan2f(m->y - G.py, m->x - G.px) - G.ang0);
        G.ang = lerpf(G.ang0, G.ang1, smoothstepf(0.0f, 1.0f, G.st / G.adur));
        if (G.st >= G.adur) { G.state = PS_CAST; G.st = 0.0f; G.fired = 0; }
        break;
    }
    case PS_CAST: {
        G.st += dt;
        float L = G.cast_len, up = 0.24f * L, fire = 0.27f * L, down = 0.73f * L;
        G.hand = G.st < up ? G.st / up : (G.st > down ? fmaxf(0.0f, 1.0f - (G.st - down) / (0.24f * L)) : 1.0f);
        if (!G.fired && G.st >= fire) {
            G.fired = 1;
            if (G.spell == AT_BFS && RUN.souls < bfs_cost(RUN.gear.atk[AT_BFS])) G.spell = AT_FIRE;
            cast_spell(G.spell, G.target);
            G.calm = 0.0f;
        }
        if (G.st >= L) {
            Mob *m = &G.mobs[G.target];
            G.hand = 0.0f; G.cooldown = fmaxf(0.15f, spell_cycle(&RUN.gear, G.spell) - L);
            if (m->alive && ++m->misses >= 3) m->ignore = (float)G.t + 5.0f;   /* three throws and no hit: leave it */
            int stand = m->type == MOB_SKEL || m->type == MOB_SHIELD || m->type == MOB_WSKULL || (m->type == MOB_SPIDER && m->sub >= 1)
                        || m->type == MOB_SHAMBLER || m->type == MOB_DROWNED;   /* the slow ones: stand and finish them */
            if (targetable(m) && stand && hypotf(m->x - G.px, m->y - G.py) < 8.0f
                && m->ignore < (float)G.t && shot_clear(m->x, m->y)) { G.state = PS_HOLD; G.st = 0.0f; break; }   /* stand and finish it */
            begin_turn(heading_angle(G.h)); G.adur = 0.18f + 0.13f * fabsf(G.ang1 - G.ang0); G.state = PS_UNAIM;
        }
        break;
    }
    case PS_HOLD: {
        Mob *m = &G.mobs[G.target];
        G.st += dt;
        int ok = targetable(m) && shot_clear(m->x, m->y) && G.st < 6.0f;
        if (ok) G.ang += wrap_pi(atan2f(m->y - G.py, m->x - G.px) - G.ang) * fminf(1.0f, dt * 10.0f);
        if (!ok) { begin_turn(heading_angle(G.h)); G.adur = 0.18f + 0.13f * fabsf(G.ang1 - G.ang0); G.state = PS_UNAIM; }
        else if (G.cooldown <= 0.0f) {
            G.spell = choose_spell(G.target);
            G.cast_len = fminf(0.75f, fmaxf(0.3f, spell_cycle(&RUN.gear, G.spell) * 0.42f));
            G.state = PS_CAST; G.st = 0.0f; G.fired = 0;
        }
        break;
    }
    case PS_EXIT: {
        G.st += dt;
        float pt = G.exit_t;
        G.exit_t += dt;
        G.ang = G.st < G.adur ? lerpf(G.ang0, G.ang1, smoothstepf(0.0f, 1.0f, G.st / G.adur)) : G.ang1;
        if (pt < 0.4f && G.exit_t >= 0.4f) { snd_sfx(SFX_DOOR); snd_sfx_ex(SFX_SHIMMER, 0.8f, 0.5f); }
        G.exit_open = clampf((G.exit_t - 0.4f) / 0.7f, 0.0f, 1.0f);
        if (G.exit_t > 1.2f && G.exit_t < 2.1f) { G.px += cosf(G.ang) * dt * 0.5f; G.py += sinf(G.ang) * dt * 0.5f; G.bobph += dt * 5.0f; }
        if (G.exit_t >= 2.4f && !G.tally) {
            G.tally = 1; G.tally_t = 0.0; G.clear_time = G.level_t; G.retries = 0;
            snd_sfx_ex(SFX_FANFARE, 1.0f, 0.5f);
            rec_cleared();
#ifdef CRAWL_DEBUG
            fprintf(stderr, "EVENT cleared level %d t=%.1f by %s\n", G.level, G.level_t, G.player ? "player" : "autopilot");
#endif
        }
        break;
    }
    case PS_PUSH: {
        G.st += dt;
        G.ang = lerpf(G.ang0, G.ang1, smoothstepf(0.0f, 1.0f, fminf(1.0f, G.st / G.adur)));
        float k = G.st - G.adur;
        G.hand = k < 0.0f ? 0.0f : (k < 0.25f ? k / 0.25f * 0.7f : (k < 0.8f ? 0.7f : fmaxf(0.0f, 0.7f - (k - 0.8f) * 2.5f)));
        if (k >= 0.3f && !G.fired) { G.fired = 1; open_secret(G.seek_secret); }
        if (k >= 1.2f && G.slide < 0) {                             /* the way is open: in it goes */
            G.ang = wrap_pi(G.ang1); G.hand = 0.0f;
            G.h = G.next_h = G.secrets[G.seek_secret].d; G.seek_secret = -1;
            G.state = PS_MOVE; start_move();
        }
        break;
    }
    case PS_USE: {                                                 /* a doorway in a chain, or the visportal's gift */
        G.st += dt;
        G.ang = lerpf(G.ang0, G.ang1, smoothstepf(0.0f, 1.0f, fminf(1.0f, G.st / G.adur)));
        float k = G.st - G.adur;
        G.hand = G.use_kind == USE_VIS ? (k < 0.0f ? 0.0f : (k < 0.25f ? k / 0.25f * 0.7f : (k < 0.9f ? 0.7f : fmaxf(0.0f, 0.7f - (k - 0.9f) * 2.5f)))) : 0.0f;
        if (k >= 0.3f && !G.fired) {
            G.fired = 1;
#ifdef CRAWL_DEBUG
            fprintf(stderr, "EVENT chain_use %s (%d,%d) link %d t=%.2f\n", G.use_kind == USE_DOOR ? "door" : "vis", G.use_x, G.use_y, PK.depth, G.t);
#endif
            if (G.use_kind == USE_DOOR) {                          /* through it: a new place, and G with it */
                if (!PK.depth) for (int i = 0; i < G.nsecrets; i++) if (G.secrets[i].door_x == G.use_x && G.secrets[i].door_y == G.use_y) PK.auto_done[i] = 1;
                if (pocket_go(G.use_x, G.use_y)) break;
            } else { visportal_use(); PK.auto_back = 1; }
        }
        if (k >= (G.use_kind == USE_VIS ? 1.2f : 0.5f)) {
            G.ang = wrap_pi(G.ang1); G.hand = 0.0f; G.use_kind = USE_NONE;
            G.next_h = decide(G.cx, G.cy, G.h);
            begin_turn(heading_angle(G.next_h));
            G.state = PS_TURN;
        }
        break;
    }
    case PS_ALTAR: {                                               /* a hand to the altar: anointed, woken, sent on, blessed */
        G.st += dt;
        G.ang = lerpf(G.ang0, G.ang1, smoothstepf(0.0f, 1.0f, fminf(1.0f, G.st / G.adur)));
        float k = G.st - G.adur;
        G.hand = k < 0.0f ? 0.0f : (k < 0.25f ? k / 0.25f * 0.7f : (k < 0.9f ? 0.7f : fmaxf(0.0f, 0.7f - (k - 0.9f) * 2.5f)));
        if (k >= 0.4f && !G.fired) {
            G.fired = 1;
            if (wants_anoint()) altar_anoint();
            if (wants_wake()) altar_wake();
            if (wants_send()) altar_send();
            if (wants_bless()) altar_bless();
        }
        if (k >= 1.2f) {
            G.ang = wrap_pi(G.ang1); G.hand = 0.0f;
            G.next_h = decide(G.cx, G.cy, G.h);
            begin_turn(heading_angle(G.next_h));
            G.state = PS_TURN;
        }
        break;
    }
    case PS_UNAIM:
        G.st += dt;
        G.ang = lerpf(G.ang0, G.ang1, smoothstepf(0.0f, 1.0f, G.st / G.adur));
        if (G.st >= G.adur) { G.ang = wrap_pi(G.ang1); G.state = G.resume_state; }
        break;
    case PS_SETTLE: {                                              /* handed back: to the middle of the cell, square to the walls */
        G.st += dt;
        float k = smoothstepf(0.0f, 1.0f, fminf(1.0f, G.st / 0.45f));
        G.px = lerpf(G.sx0, G.cx + 0.5f, k); G.py = lerpf(G.sy0, G.cy + 0.5f, k);
        G.ang = lerpf(G.ang0, G.ang1, k);
        G.hand = fmaxf(0.0f, G.hand - dt * 2.0f);
        if (G.st >= 0.45f) { G.ang = wrap_pi(G.ang1); G.u = 1.0f; G.tx = G.cx; G.ty = G.cy; start_move(); }
        break;
    }
    }
    float amp = 0.014f * clampf(G.spd / CRUISE, 0.0f, 1.0f);
    G.camz = 0.5f + amp * sinf(G.bobph);
}

/* ================================================================== */
/* Level flow                                                          */
/* ================================================================== */

static void begin_shop(void);
static void human_begin(void);

static void reset_caster_state(void) {
    G.poison_t = G.poison_dps = G.poison_acc = G.slow_t = G.purify_t = G.regen_acc = 0.0f;
    G.ward_hp = G.ward_t = G.mirror_t = G.slip_t = G.reap_t = G.mend_t = G.mend_rate = G.mirror_flash = 0.0f;
    G.ward_shards = G.decoy_t = 0.0f;
    memset(G.def_cd, 0, sizeof G.def_cd); memset(G.def_wait, 0, sizeof G.def_wait);
    memset(G.def_hush, 0, sizeof G.def_hush); memset(G.rune_glow, 0, sizeof G.rune_glow);
    G.bfs_t = 0.0f; G.face_t = G.grin_t = 0.0f;
    G.l_down = 0; G.l_t = 0.0f; G.pilgrim = 0;
    G.max_hp = gear_max_hp(&RUN.gear);
    if (G.hp > G.max_hp) G.hp = G.max_hp;
}

#ifdef CRAWL_DEBUG
/* the level from above, for checking the generator; seen: the lanes the autopilot has counted as seen */
static void debug_map(int seen) {
    {
        static char rows[MAPN][MAPN + 1];
        for (int y = 0; y < MAPN; y++) {
            for (int x = 0; x < MAPN; x++) {
                const Cell *c = &G.map[y][x];
                char ch = c->kind == K_WALL ? '#' : c->kind == K_PILLAR ? 'O' : c->kind == K_PROP ? 'p' : c->kind == K_DOOR ? (G.doors[c->door].locked ? 'L' : 'D') : (lane(x, y) ? (seen && c->visits ? 'o' : '.') : ' ');
                if (c->kind == K_WALL && c->tex == T_TORCH0) ch = 't';
                if (c->secret) ch = c->secret == 2 ? '?' : (c->secret == 3 ? '&' : ':');
                rows[y][x] = ch;
            }
            rows[y][MAPN] = 0;
        }
        for (int i = 0; i < G.nitems; i++) rows[G.items[i].cy][G.items[i].cx] = G.items[i].type == IT_KEY ? 'k' : G.items[i].type == IT_POTION ? '+' : '$';
        for (int i = 0; i < MAXMOBS; i++) if (G.mobs[i].alive) rows[(int)G.mobs[i].y][(int)G.mobs[i].x] = 'm';
        if (G.exit_fx >= 0) { rows[G.exit_fy][G.exit_fx] = 'x'; rows[G.exit_wy][G.exit_wx] = 'X'; }
        rows[G.cy][G.cx] = seen ? '@' : 'S';
        fprintf(stderr, "MAP level %d rooms %d vaults %d doors %d lights %d decos %d items %d lanes %d\n", G.level, G.nrooms, G.nvaults, G.ndoors, G.nlights, G.ndecos, G.nitems, G.ncells);
        for (int y = 0; y < MAPN; y++) fprintf(stderr, "MAP %s\n", rows[y]);
        if (!seen) {
            for (int y = 0; y < MG; y++) {                          /* and the plan it was drawn from */
                char pr[MG + 1];
                for (int x = 0; x < MG; x++) pr[x] = MM[y][x].kind == K_WALL ? '#' : MM[y][x].kind == K_DOOR ? 'D' : MM[y][x].room >= 0 ? (char)('a' + MM[y][x].room) : (char)('0' + MM[y][x].w);
                pr[MG] = 0;
                fprintf(stderr, "PLAN %s\n", pr);
            }
            static int st[MAPN][MAPN], qx[MAPN * MAPN], qy[MAPN * MAPN];   /* can every lane be reached, keys aside? */
            memset(st, 0, sizeof st);
            int qh = 0, qt = 0, total = 0;
            st[G.cy][G.cx] = 1; qx[qt] = G.cx; qy[qt] = G.cy; qt++;
            while (qh < qt) {
                int x = qx[qh], y = qy[qh]; qh++;
                for (int d = 0; d < 4; d++) {
                    int nx = x + DX[d], ny = y + DY[d];
                    if (st[ny][nx] || !is_open(nx, ny) || !lane(nx, ny)) continue;
                    st[ny][nx] = 1; qx[qt] = nx; qy[qt] = ny; qt++;
                }
            }
            for (int y = 0; y < MAPN; y++) for (int x = 0; x < MAPN; x++) total += is_open(x, y) && lane(x, y);
            fprintf(stderr, "MAP lanes reachable %d of %d\n", qt, total);
        }
    }
}
#endif

/* A Black Altar, from level 6, on a level with secrets to find: in the exit room, clear of the way,
 * else in the room farthest from the start (PLAN-items: Black Altars). */
static void place_altar(void) {
    G.altar_x = -1; G.altar_state = ALTAR_DORMANT; G.altar_seek = -1; G.blessed = 0; G.altar_wisp = AW_NONE; G.altar_inside = 0;
    if (G.level < 6 || G.nsecrets == 0) return;
    int room = -1;
    for (int r = 0; r < G.nrooms && G.exit_fx >= 0 && room < 0; r++) if (in_room(&G.rooms[r], G.exit_fx, G.exit_fy)) room = r;
    for (int pass = 0; pass < 2; pass++) {
        if (pass == 1 || room <= 0) {                               /* the room farthest from the start */
            bfs_from(G.cx, G.cy);
            int bd = -1;
            room = -1;
            for (int r = 1; r < G.nrooms; r++) {
                const Room *Rm = &G.rooms[r];
                int d = bfs_dist[Rm->y + Rm->h / 2][Rm->x + Rm->w / 2];
                if (vault_of_room(Rm->x, Rm->y) < 0 && d > bd) { bd = d; room = r; }
            }
            if (room < 0) return;
        }
        const Room *Rm = &G.rooms[room];
        for (int tries = 0; tries < 300; tries++) {
            int x = Rm->x + rnd(Rm->w), y = Rm->y + rnd(Rm->h);
            if (G.map[y][x].kind != K_FLOOR || G.map[y][x].secret || !block_corner(x, y) || lane(x, y) || by_way(x, y) || G.bulky[y][x] || item_at(x, y) || in_band(y)) continue;
            if (G.exit_fx >= 0 && abs(x - G.exit_fx) + abs(y - G.exit_fy) < 2) continue;   /* not in the way out */
            solid_prop(x, y, SP_BALTAR, 1, K_PROP);
            G.altar_x = x; G.altar_y = y;
#ifdef CRAWL_DEBUG
            fprintf(stderr, "EVENT altar at (%d,%d) in %s room\n", x, y, pass == 0 && in_room(Rm, G.exit_fx, G.exit_fy) ? "the exit" : "the farthest");
#endif
            return;
        }
    }
}

static void pocket_reset_level(void);
static void start_level(void) {
    pocket_reset_level();
    memset(G.has_key, 0, sizeof G.has_key);
    G.shop = 0;
    hz_clear(); snd_set_boss(0, 0);
    G.boss = boss_of_level(G.level); G.bstate = G.boss >= 0 ? BS_WAIT : BS_NONE; G.bwave = G.badds = 0; G.bmob = -1;
    G.bz_t = 6.0f; G.bz2_t = 9.0f; G.bcast_t = 0.0f; G.omen_t = 30.0f + 20.0f * rndf();
    G.bfight_t = G.bstate_t = G.bcard = G.bring = 0.0f; G.bbar = 1.0f;
    G.bpose = BP_NONE; G.bpose_t = G.bdash_t = G.bmirror_t = G.bmirror_cd = G.bhurt = 0.0f; G.bdash_age = 99.0f;
    memset(G.bdash_cd, 0, sizeof G.bdash_cd);
    G.bward = G.bward_cd = G.bchant_dmg = 0.0f;
    G.bbz_n = 0; G.bbz_at = -1; G.bbz_t = G.bslip_t = G.bslip_cd = G.bgutter_cd = 0.0f; G.bww = 0; G.bww_t = 0.0f; G.bww_cd = 12.0f;
    G.bsp_t = 5.0f; G.bmend_t = G.bmend_cd = G.web_hp = G.web_rear = G.web_idle = G.btrickle_t = 0.0f; G.web_cd = 8.0f; G.bmend_base = 1e9f;
    if (G.boss < 0) G.bchallenge = 0;
    G.bstep = G.bchallenge && G.boss >= 0 ? RUN.bstep[G.boss] : 0;
    crawl_art_level(G.level);
    gen_level();
    memset(G.mobs, 0, sizeof G.mobs);
    memset(G.shots, 0, sizeof G.shots);
    memset(G.booms, 0, sizeof G.booms);
    memset(G.bones, 0, sizeof G.bones);
    memset(G.beams, 0, sizeof G.beams);
    memset(G.burns, 0, sizeof G.burns);
    reset_caster_state();
    const Room *R = &G.rooms[0];
    G.cx = R->x + R->w / 2; G.cy = R->y + R->h / 2;
    G.cx -= ((G.cx - 2) % 4 + 4) % 4; G.cy -= ((G.cy - 2) % 4 + 4) % 4;   /* where two lanes cross */
    if (G.map[G.cy][G.cx].kind != K_FLOOR) { G.cx = R->x + 1; G.cy = R->y + 1; }
    G.exit_seen = G.leaving = G.exiting = 0; G.exit_open = G.exit_t = 0.0f; G.seek_secret = -1;
    G.kills = G.kills_total = 0; G.tally = 0; G.tally_t = 0.0; G.tally_done = 0;
    memset(G.tally_shown, 0, sizeof G.tally_shown);
    place_items(G.cx, G.cy);
    if (G.boss < 0) place_exit(G.cx, G.cy);                        /* a boss level's way out is beyond its boss */
    place_altar();
    G.level_treasure = 0; G.level_souls = 0; G.dead = 0; G.dead_t = 0.0; G.spirit_said = 0;
    G.wraith_done = G.wraith_now = 0; G.wraith_try = 0.0f;          /* the Wraith comes once a level, and again after a death */
    memset(G.wisps, 0, sizeof G.wisps);                            /* wisps don't go down the stairs with you */
    G.hurt = G.gold = G.kick = G.chill = 0.0f; G.pvx = G.pvy = 0.0f;
    G.px = G.cx + 0.5f; G.py = G.cy + 0.5f;
    G.h = rnd(4);
    for (int k = 0; k < 4 && !apass(G.cx + DX[G.h], G.cy + DY[G.h]); k++) G.h = (G.h + 1) & 3;
    G.ang = heading_angle(G.h);
    G.map[G.cy][G.cx].visits = 1; G.explored = 1;
    G.spd = 0.0f; G.bobph = 0.0f; G.camz = 0.5f; G.hand = 0.0f; G.cooldown = 2.0f; G.calm = 0.0f;
    start_move();
    memset(G.spawned, 0, sizeof G.spawned); memset(G.named, 0, sizeof G.named);
    place_wall_skulls(G.cx, G.cy);
    place_spiders(G.cx, G.cy);
    place_guardians();
    int x, y;
    place_risers();
    place_imps();
    place_gargoyles();
    for (int i = 0; i < 3 + (G.level >= 4); i++) if (pick_cell_at_depth(G.cx, G.cy, 7, 40, 1, &x, &y)) spawn_wanderer(x, y);
    static const int WANDERS[] = { MOB_SHIELD, MOB_SHAMBLER, MOB_HOUND, MOB_HUNTER, MOB_GHOUL, MOB_KNIGHT, MOB_SHADE, MOB_IMP, MOB_DEACON };
    for (int i = 0; i < (int)(sizeof WANDERS / sizeof WANDERS[0]); i++)   /* a newcomer's first level always brings one, alone */
        if (G.level == KINDS[WANDERS[i]].debut && !G.spawned[WANDERS[i]] && pick_cell_at_depth(G.cx, G.cy, 8, 30, 1, &x, &y)) spawn_wanderer_type(x, y, WANDERS[i]);
    for (int r = 1; r < G.nrooms; r++) {                           /* residents: the dead keep to their rooms till they know you are there */
        const Room *R = &G.rooms[r];
        if (vault_of_room(R->x, R->y) >= 0 || (R->theme != TH_OSSUARY && R->theme != TH_CRYPT) || rnd(100) >= 45) continue;
        if (!room_cell(r, &x, &y) || abs(x - G.cx) + abs(y - G.cy) < 8) continue;
        int type = R->theme == TH_CRYPT && rnd(3) == 0 ? (may_spawn(MOB_KNIGHT) && rnd(2) ? MOB_KNIGHT : (may_spawn(MOB_SHIELD) ? MOB_SHIELD : MOB_SKEL)) : MOB_SKEL;
        Mob *m = spawn_mob(x, y, type);
        if (m) m->home = G.map[y][x].region;
    }
    if (may_spawn(MOB_BROOD) && (G.level == KINDS[MOB_BROOD].debut || rnd(10) < 6))   /* a brood mother in a room of her own, a sac or two about her */
        for (int tries = 0; tries < 20; tries++) {
            int r = 1 + rnd(G.nrooms > 1 ? G.nrooms - 1 : 1);
            if (r >= G.nrooms || vault_of_room(G.rooms[r].x, G.rooms[r].y) >= 0 || !room_cell(r, &x, &y) || abs(x - G.cx) + abs(y - G.cy) < 10) continue;
            Mob *m = spawn_mob(x, y, MOB_BROOD);
            if (!m) break;
            m->home = G.map[y][x].region; m->contact = 3.0f;
            int sx, sy;
            for (int k = 0; k < 1 + rnd(2); k++) if (room_cell(r, &sx, &sy)) { Mob *e = spawn_mob(sx, sy, MOB_SAC); if (e) e->rank = m->rank; }
            break;
        }
    if (G.boss >= 0) boss_spawn();
    hunt_cx = -1;
    if (G.level >= 7 && rnd(3) == 0) {                             /* now and then one of them is gilded, and carries treasure */
        int pick[MAXMOBS], np = 0;
        for (int i = 0; i < MAXMOBS; i++) {
            const Mob *m = &G.mobs[i];
            if (m->alive && !m->guard && !m->boss && KINDS[m->type].gild) pick[np++] = i;
        }
        if (np) set_rank(&G.mobs[pick[rnd(np)]], RK_GILDED);
    }
#ifdef CRAWL_DEBUG
    { int nt[NKINDS] = { 0 }, nr[NRANK] = { 0 }; for (int i = 0; i < MAXMOBS; i++) if (G.mobs[i].alive) { nt[G.mobs[i].type]++; nr[G.mobs[i].rank]++; }
      fprintf(stderr, "LEVEL %d mobs:", G.level);
      for (int k = 0; k < NKINDS; k++) if (nt[k]) fprintf(stderr, " %s %d", KINDS[k].name, nt[k]);
      fprintf(stderr, "; ranks %d %d %d %d %d gilded %d\n", nr[0], nr[1], nr[2], nr[3], nr[4], nr[5]); }
#endif
#ifdef CRAWL_DEBUG
    if (getenv("CRAWL_TPBOSS") && atoi(getenv("CRAWL_TPBOSS")) == 2 && G.boss >= 0 && G.bstart_x >= 0) {   /* testing: the autopilot */
        G.cx = G.bstart_x; G.cy = G.bstart_y; G.px = G.cx + 0.5f; G.py = G.cy + 0.5f; G.h = 0; G.ang = 0.0f;   /* in the antechamber, going */
        G.leaving = 1; start_move();
    }
#endif
    G.level_t = 0.0; G.card = 0.0f; G.ending = 0;
    G.readout = READOUT_T + 1.5f;                                  /* show where things stand as the level opens */
    G.bar = 1.0f; show_bar();
    if (G.player) human_begin();
#ifdef CRAWL_DEBUG
    if (getenv("CRAWL_MAP")) debug_map(0);
#endif
}

/* every spell at tier t, every rune at the quality tier t comes to in an old save, and an Uncommon
 * piece of level 2t in every slot (about what tier t used to wear); for the console and the test
 * switches alike */
static void give_gear(int t) {
    uint32_t rs = 0x9e3779b9u ^ (uint32_t)t;
    static const int KIND_OF[NSLOT] = { IK_WRIST, IK_WRIST, IK_BOOTS, IK_NECK, IK_HOOD, IK_ROBE, IK_RING };
    for (int i = 0; i < NATK; i++) RUN.gear.atk[i] = (int8_t)t;
    for (int i = 0; i < NDEF; i++) RUN.gear.def[i] = (int8_t)rune_from_tier(t);
    RUN.gear.bones = (int16_t)clampi(SPEAR_BONES * t, 0, BONES_MAX);   /* as if the spear were bought t times */
    for (int s = 0; s < NSLOT && t > 0; s++) RUN.gear.worn[s] = piece_roll(KIND_OF[s], 2 * t, Q_UNCOMMON, &rs);
}
static int give_ability(int a) {                                   /* every worn piece with that ability */
    int n = 0;
    for (int sl = 0; sl < NSLOT; sl++) if (RUN.gear.worn[sl].kind) { RUN.gear.worn[sl].ability = (uint8_t)clampi(a, 0, NABIL - 1); n++; }
    return n;
}
static void give_bag(int q) {                                      /* a bag of that quality, all but one place filled */
    uint32_t rs = 0x85ebca6bu ^ (uint32_t)q;
    q = clampi(q, 0, NQUAL - 1);
    RUN.gear.bag_size = (uint8_t)bag_capacity(q); RUN.gear.bag_quality = (uint8_t)q;
    for (int i = 0; i + 1 < RUN.gear.bag_size; i++) RUN.gear.bag[i] = piece_roll(IK_WRIST + (int)(rs % 6), 3 + i * 2, i % NQUAL, &rs);
}

#ifdef CRAWL_DEBUG
/* for testing: CRAWL_GEAR=n (give_gear), CRAWL_RUNES=q (every rune at quality q, 1 Common to 5
 * Exquisite), CRAWL_ABIL, CRAWL_BAG, CRAWL_GOLD, CRAWL_SOULS, CRAWL_COINS (Cursed Coins), CRAWL_SHRINE=r
 * (the Rune Shrine's rank, 0 sanctified to 5) */
static void debug_gear(void) {
    if (getenv("CRAWL_GEAR")) give_gear(atoi(getenv("CRAWL_GEAR")));
    if (getenv("CRAWL_RUNES")) for (int i = 0; i < NDEF; i++) RUN.gear.def[i] = (int8_t)clampi(atoi(getenv("CRAWL_RUNES")), 0, RUNE_EXQ);
    if (getenv("CRAWL_COINS")) RUN.coins = atoi(getenv("CRAWL_COINS"));
    if (getenv("CRAWL_SHRINE")) RUN.shrine = clampi(atoi(getenv("CRAWL_SHRINE")), 0, NQUAL);
    if (getenv("CRAWL_ABIL")) give_ability(atoi(getenv("CRAWL_ABIL")));
    if (getenv("CRAWL_BAG")) give_bag(atoi(getenv("CRAWL_BAG")));
    if (getenv("CRAWL_GOLD")) RUN.treasure = atoi(getenv("CRAWL_GOLD"));
    if (getenv("CRAWL_SOULS")) RUN.souls = atoi(getenv("CRAWL_SOULS"));
}
#endif

/* A piece comes to the caster, bought or found. Found, or bought by the autopilot: worn if it beats
 * what is on, otherwise into the bag. Bought by a person: worn at once (wear). What has nowhere to go
 * is lost. A caption says which. */
static void take_piece(const Offer *o, int wear) {
    const Piece *p = &o->piece;
    char a[48], b[48];
    piece_name(p, a, b, sizeof a);
    int slot = piece_slot(&RUN.gear, p->kind);
    const char *was = slot >= 0 ? KIND_NAME[RUN.gear.worn[slot].kind] : "";
    int old = G.max_hp;
    switch (wear ? gear_take_wear(&RUN.gear, o) : gear_take(&RUN.gear, o)) {
    case TAKE_WORN: say("NOW WEARING %s", b); break;
    case TAKE_WORN_OLD_BAGGED: say("%s ON; THE OLD %s IN THE BAG", KIND_NAME[p->kind], was); break;
    case TAKE_WORN_OLD_LOST: say("%s ON; THE OLD %s LOST", KIND_NAME[p->kind], was); break;
    case TAKE_BAGGED: say("INTO THE BAG: %s", b); break;
    case TAKE_LOST: say("NO ROOM FOR IT: %s LEFT", KIND_NAME[p->kind]); break;
    case TAKE_BAG: say("A %s: ROOM FOR %d", bag_name(RUN.gear.bag_size), RUN.gear.bag_size); break;
    default: break;
    }
    if (p->ability == AB_KEYMAKER && G.player) say("KEYMAKER: KEY WISPS IN THE L MENU");   /* on every purchase */
    G.max_hp = gear_max_hp(&RUN.gear);
    if (G.max_hp > old) G.hp += G.max_hp - old;                    /* new strength comes with it */
    if (G.hp > G.max_hp) G.hp = G.max_hp;
}

/* The run carries on across scene switches: the level, HP, gold, souls and gear are kept, and the
 * current level is laid out afresh (without a second visit to the merchant). */

/* ---- pockets (docs/PLAN-b6.md): the deeper links of a secret's chain, each a place of its own beyond
 * a doorway. Walking into the doorway at the back of a chained room takes you into the chain's first
 * pocket; its doorway back returns you there, and a doorway on goes deeper. The level waits, as it is,
 * while you are away (G_LEVEL keeps it), and a pocket is built afresh each time from its chain's seed,
 * less what has been taken from it. Dying in one throws you back to the chain's room, whole. ---- */


/* what goes with the caster through a doorway; the rest of G is the place. Ranges of G, in its order */
#define G_SPAN(a, b) (offsetof(GState, b) + sizeof(((GState *)0)->b) - offsetof(GState, a))
#define G_CARRY(from, a, b) memcpy((char *)&G + offsetof(GState, a), (const char *)(from) + offsetof(GState, a), G_SPAN(a, b))
static void carry_caster(const GState *from) {
    G.t = from->t; G.rs = from->rs; G.kills = from->kills; G.card = from->card; G.mood_t = from->mood_t;
    G_CARRY(from, px, blocked);                                    /* where it stands and how it moves, the hand, the spell */
    G_CARRY(from, pvx, unlock_t);                                  /* health, this level's gains, keys */
    G_CARRY(from, poison_t, commune_r);                            /* what is on it, the runes running, the abilities' clocks */
    G_CARRY(from, def_cd, face_side);                              /* the runes' cooldowns, the portrait */
    G_CARRY(from, hurt, chill);
    G_CARRY(from, player, inv_page);                               /* a person's controls and menus */
}

static void pocket_say_found(int link) {                           /* the first time into a link: a secret, on the level's count */
    if (link <= PK.reached[PK.chain]) return;
    PK.reached[PK.chain] = (uint8_t)link;
    G_LEVEL.links_found++;
    rec_secret();
    say("A SECRET!   %d OF %d", G_LEVEL.links_found, G_LEVEL.nlinks);
    if (G_LEVEL.links_found >= G_LEVEL.nlinks) gain_coin(COIN_ALL, "EVERY SECRET HERE FOUND");
    snd_sfx_ex(SFX_SECRET, 0.9f, 0.5f);
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT secret %d of %d (pocket %d of chain %d) t=%.2f\n", G_LEVEL.links_found, G_LEVEL.nlinks, link, PK.chain, G.t);
#endif
}

/* A pocket, built into G: blocks of three by three in a square, joined as a maze with loops, some
 * walls opened right across into halls; lit here and there, dark between. It grows with the level and
 * the link, from 13 cells across early in the Crypts to the whole 45 at the end of a long chain in the
 * Deep. The way back is a doorway in its west wall; the way on, if the chain goes on, a doorway in an
 * outer wall as far as can be from it; else the chain's reward (for now a hoard, until the themes bring
 * theirs). Its things come in the same order each time, so what was taken can stay taken. */
static void gen_pocket(const Secret *sc, int link) {
    memset(G.haz, 0, sizeof G.haz); G.hz_acc = 0.0f;               /* the level's hazards wait with it */
    if (G.silenced) { G.silenced = 0; snd_set_muffle(0); }
    int lv = (G.level - 1) % DESCENT_LEN;
    float t = clampf(0.6f * (float)lv / (float)(DESCENT_LEN - 1) + 0.4f * (float)(link - 2) / 4.0f, 0.0f, 1.0f);
    int n = clampi(3 + (int)(t * 8.0f + 0.5f), 3, 11), S = 4 * n + 1, o = ((MAPN - S) / 2) & ~3;
    uint32_t keep_rs = G.rs;
    G.rs = sc->seed ^ ((uint32_t)link * 2654435761u);
    G.ptheme = crawl_set(G.level) < 2 ? PT_BACKROOMS : PT_SEVERANCE;   /* the Backrooms for the first two runs, Severance for the last */
    G.vis_x = G.vis_y = G.vis_deco = -1;
    int back = G.ptheme == PT_BACKROOMS, sev = G.ptheme == PT_SEVERANCE;
    for (int y = 0; y < MAPN; y++)
        for (int x = 0; x < MAPN; x++) { Cell *c = &G.map[y][x]; memset(c, 0, sizeof *c); c->kind = K_WALL; c->tex = back ? T_PAPER : sev ? T_WHITE : T_STONE; c->region = -1; c->door = -1; }
    #define BX(i) (o + 4 * (i) + 1)
    for (int j = 0; j < n && !sev; j++)                            /* Severance carves its halls once it knows its far end */
        for (int i = 0; i < n; i++)
            for (int y = BX(j); y < BX(j) + 3; y++) for (int x = BX(i); x < BX(i) + 3; x++) carve(x, y, 0, TH_CORRIDOR);
    /* the maze: a spanning tree over the blocks, then loops; a wall opened right across, or by a door's width */
    static uint8_t seen[11][11], open_e[11][11], open_s[11][11];
    static int st[121];
    memset(seen, 0, sizeof seen); memset(open_e, 0, sizeof open_e); memset(open_s, 0, sizeof open_s);
    int sp = 0, j0 = rnd(n);
    st[sp++] = j0 * 11; seen[j0][0] = 1;
    while (sp > 0) {
        int b = st[sp - 1], bi = b % 11, bj = b / 11, opts[4], no = 0;
        for (int d = 0; d < 4; d++) { int ni = bi + DX[d], nj = bj + DY[d]; if (ni >= 0 && nj >= 0 && ni < n && nj < n && !seen[nj][ni]) opts[no++] = d; }
        if (!no) { sp--; continue; }
        int d = opts[rnd(no)], ni = bi + DX[d], nj = bj + DY[d];
        if (d == 0) open_e[bj][bi] = 1 + rnd(2); else if (d == 2) open_e[nj][ni] = 1 + rnd(2);
        else if (d == 1) open_s[bj][bi] = 1 + rnd(2); else open_s[nj][ni] = 1 + rnd(2);
        seen[nj][ni] = 1; st[sp++] = nj * 11 + ni;
    }
    for (int j = 0; j < n; j++)
        for (int i = 0; i < n; i++) {
            if (i + 1 < n && !open_e[j][i] && rnd(100) < 35) open_e[j][i] = 1 + rnd(2);
            if (j + 1 < n && !open_s[j][i] && rnd(100) < 35) open_s[j][i] = 1 + rnd(2);
        }
    for (int j = 0; j < n && !sev; j++)
        for (int i = 0; i < n; i++) {
            if (open_e[j][i]) for (int k = open_e[j][i] == 2 ? 0 : 1; k < (open_e[j][i] == 2 ? 3 : 2); k++) carve(BX(i) + 3, BX(j) + k, 0, TH_CORRIDOR);
            if (open_s[j][i]) for (int k = open_s[j][i] == 2 ? 0 : 1; k < (open_s[j][i] == 2 ? 3 : 2); k++) carve(BX(i) + k, BX(j) + 3, 0, TH_CORRIDOR);
            if (i + 1 < n && j + 1 < n && open_e[j][i] == 2 && open_s[j][i] == 2 && open_e[j + 1][i] == 2 && open_s[j][i + 1] == 2)
                carve(BX(i) + 3, BX(j) + 3, 0, TH_CORRIDOR);       /* four walls gone: the corner too, a hall */
        }
    if (back)                                                      /* here and there a leak, now and then a socket */
        for (int y = o; y < o + S; y++)
            for (int x = o; x < o + S; x++) {
                uint32_t h = hash2(x, y) ^ sc->seed;
                if (G.map[y][x].kind == K_WALL) G.map[y][x].tex = h % 100 < 6 ? T_PAPER_DAMP : h % 100 < 9 ? T_PAPER_OUTLET : T_PAPER;
            }
    /* the way back, in the west wall; the farthest block, by the maze */
    PK.back_x = o; PK.back_y = BX(j0) + 1;
    int door_tex = back ? T_BDOOR : sev ? T_SDOOR : T_PASSAGE;
    G.map[PK.back_y][PK.back_x].secret = 3; G.map[PK.back_y][PK.back_x].tex = (uint8_t)door_tex;
    static int dist[11][11], q[121];
    for (int j = 0; j < n; j++) for (int i = 0; i < n; i++) dist[j][i] = -1;
    int qh = 0, qt = 0, far = j0 * 11;
    dist[j0][0] = 0; q[qt++] = j0 * 11;
    while (qh < qt) {
        int b = q[qh++], bi = b % 11, bj = b / 11;
        if (dist[bj][bi] > dist[far / 11][far % 11]) far = b;
        for (int d = 0; d < 4; d++) {
            int ni = bi + DX[d], nj = bj + DY[d];
            if (ni < 0 || nj < 0 || ni >= n || nj >= n || dist[nj][ni] >= 0) continue;
            int op = d == 0 ? open_e[bj][bi] : d == 2 ? open_e[nj][ni] : d == 1 ? open_s[bj][bi] : open_s[nj][ni];
            if (!op) continue;
            dist[nj][ni] = dist[bj][bi] + 1; q[qt++] = nj * 11 + ni;
        }
    }
    static uint8_t office[11][11];                                 /* Severance: which blocks are rooms, the rest narrow halls */
    if (sev) {                                                     /* white halls from block to block, turning corners; a few offices */
        for (int j = 0; j < n; j++)
            for (int i = 0; i < n; i++) office[j][i] = (i == 0 && j == j0) || j * 11 + i == far || (hash2(i, j) ^ sc->seed) % 100 < 22;
        for (int j = 0; j < n; j++)
            for (int i = 0; i < n; i++) {
                if (office[j][i]) { for (int y = BX(j); y < BX(j) + 3; y++) for (int x = BX(i); x < BX(i) + 3; x++) carve(x, y, 0, TH_CORRIDOR); }
                else carve(BX(i) + 1, BX(j) + 1, 0, TH_CORRIDOR);
                if (open_e[j][i]) {
                    for (int x = BX(i) + 1; x <= BX(i + 1) + 1; x++) carve(x, BX(j) + 1, 0, TH_CORRIDOR);
                    if (open_e[j][i] == 2 && office[j][i] && office[j][i + 1]) for (int k = 0; k < 3; k++) carve(BX(i) + 3, BX(j) + k, 0, TH_CORRIDOR);
                }
                if (open_s[j][i]) {
                    for (int y = BX(j) + 1; y <= BX(j + 1) + 1; y++) carve(BX(i) + 1, y, 0, TH_CORRIDOR);
                    if (open_s[j][i] == 2 && office[j][i] && office[j + 1][i]) for (int k = 0; k < 3; k++) carve(BX(i) + k, BX(j) + 3, 0, TH_CORRIDOR);
                }
            }
        for (int y = o; y < o + S; y++)                            /* a placard here and there, now and then a picture */
            for (int x = o; x < o + S; x++) {
                uint32_t h = hash2(x, y) ^ sc->seed;
                if (G.map[y][x].kind == K_WALL && !G.map[y][x].secret) G.map[y][x].tex = h % 100 < 5 ? T_WHITE_SIGN : h % 100 < 8 ? T_WHITE_ART : T_WHITE;
            }
    }
    PK.on_x = PK.on_y = -1;
    int gen = 0;
    uint32_t taken = PK.taken[PK.chain][clampi(link, 0, POCKET_LINKS - 1)];
    #define POCKET_ITEM(x, y, type) do { int g_ = gen++; Item *it_ = add_item(x, y, type);   /* made either way, so the dice fall alike */ \
        if (it_) { it_->secret = 1; it_->gen = g_ + 1; if (taken >> g_ & 1) it_->alive = 0; } } while (0)
    if (link < sc->links) {                                        /* the way on: in an outer wall, the farthest such block */
        int best = -1, bd = -1;
        for (int j = 0; j < n; j++)
            for (int i = 0; i < n; i++) {
                if (!(i == n - 1 || j == 0 || j == n - 1) || dist[j][i] < 0 || (i == 0 && j == j0)) continue;
                if (dist[j][i] > bd) { bd = dist[j][i]; best = j * 11 + i; }
            }
        int bi = best % 11, bj = best / 11;
        if (bi == n - 1) { PK.on_x = o + S - 1; PK.on_y = BX(bj) + 1; }
        else if (bj == 0) { PK.on_x = BX(bi) + 1; PK.on_y = o; }
        else { PK.on_x = BX(bi) + 1; PK.on_y = o + S - 1; }
        G.map[PK.on_y][PK.on_x].secret = 3; G.map[PK.on_y][PK.on_x].tex = (uint8_t)door_tex;
        if (sev) {                                                 /* a hall from the block's middle out to the door */
            int mx = BX(bi) + 1, my = BX(bj) + 1, sx = (mx > PK.on_x) - (mx < PK.on_x), sy = (my > PK.on_y) - (my < PK.on_y);
            for (int x = PK.on_x + sx, y = PK.on_y + sy; x != mx || y != my; x += sx, y += sy) carve(x, y, 0, TH_CORRIDOR);
        }
    } else {                                                       /* the chain's end: its reward, for now a hoard */
        int cx = BX(far % 11) + 1, cy = BX(far / 11) + 1, vd = -1;
        for (int d = 0; d < 4 && (back || sev) && vd < 0; d++) {   /* the Backrooms' end: a doorway onto doorways; Severance's, a water cooler */
            int wx = cx + DX[d] * 2, wy = cy + DY[d] * 2;
            if (G.map[wy][wx].kind != K_WALL || G.map[wy][wx].secret) continue;
            vd = d;
            if (back) { G.map[wy][wx].tex = T_VISPORT0; G.vis_x = wx; G.vis_y = wy; }
            else { G.vis_x = cx + DX[d]; G.vis_y = cy + DY[d]; solid_prop(G.vis_x, G.vis_y, PK.vis_used[PK.chain] ? SP_COOLER_LOW : SP_COOLER, 1, K_PROP); G.vis_deco = G.ndecos - 1; }
        }
        if (vd >= 0) {                                             /* the hoard to either side, out of its way */
            int qx = DY[vd] != 0, qy = DX[vd] != 0;
            POCKET_ITEM(cx + DX[vd] + qx, cy + DY[vd] + qy, IT_CHEST);
            POCKET_ITEM(cx + qx, cy + qy, IT_CROWN);
            POCKET_ITEM(cx - qx, cy - qy, IT_GEMS);
        } else {
            POCKET_ITEM(cx, cy, IT_CHEST);
            POCKET_ITEM(cx - 1, cy, IT_CROWN);
            POCKET_ITEM(cx + 1, cy, IT_GEMS);
            light_static(cx + 0.5f, cy + 0.5f, 1.4f, 5.0f);
        }
    }
    /* light here and there, and a few things lying about; in the Backrooms a tube panel over every block,
     * most lit, some failing, some dead */
    for (int j = 0; j < n; j++)
        for (int i = 0; i < n; i++) {
            int r = rnd(100), first = (i == 0 && j == j0) || (back && G.vis_x >= 0 && j * 11 + i == far);   /* the way in and the end, lit */
            if (sev) {                                             /* Severance: every panel lit, and steady */
                G.map[BX(j) + 1][BX(i) + 1].lamp = (uint8_t)(1 + G.nlights);
                light_static(BX(i) + 1.5f, BX(j) + 1.5f, 1.45f + 0.1f * rndf(), 6.0f);
                G.lights[G.nlights - 1].phase = -1000.0f;
            } else if (back) {
                Cell *lc = &G.map[BX(j) + 1][BX(i) + 1];
                if (!first && r >= 75) { lc->lamp = 255; continue; }
                lc->lamp = (uint8_t)(1 + G.nlights);
                light_static(BX(i) + 1.5f, BX(j) + 1.5f, 1.3f + 0.25f * rndf(), 5.0f);
                G.lights[G.nlights - 1].phase = (!first && r >= 60) ? -10.0f - 80.0f * rndf() : -1000.0f;
            } else if (first || r < 55) {
                light_static(BX(i) + 1.5f, BX(j) + 1.5f, 1.25f + 0.35f * rndf(), 5.0f);
                G.lights[G.nlights - 1].phase = -1000.0f;              /* steady: no flames here */
            }
        }
    static const int LOOT[5] = { IT_COINS, IT_COINS, IT_GEMS, IT_CHALICE, IT_CROWN };
    for (int k = 0; k < 2 + link + rnd(2); k++) {
        int i = rnd(n), j = rnd(n);
        if (i == 0 && j == j0) continue;
        POCKET_ITEM(BX(i) + 1, BX(j) + 1, LOOT[rnd(5)]);
    }
    for (int j = 0; j < n && sev; j++)                             /* in the offices, four desks in a corner (off the lanes), where */
        for (int i = 0; i < n; i++) {                              /* nothing lies already */
            uint32_t h = hash2(i, j) ^ sc->seed;
            int x = BX(i) + (int)(h & 1) * 2, y = BX(j) + (int)(h >> 1 & 1) * 2, clear = office[j][i] && !(i == 0 && j == j0) && j * 11 + i != far;
            for (int k = 0; k < G.nitems && clear; k++) clear = !(G.items[k].lx == x && G.items[k].ly == y);
            if (clear) solid_prop(x, y, SP_DESKS, 1, K_PROP);
        }
    if (back) {                                                    /* things from the crypt out of place, and some caught in a loop */
        static const int OUT[8] = { SP_URN, SP_SKULLPILE, SP_BONES1, SP_BONES2, SP_TOMB_ROUND, SP_TOMB_CROSS, SP_BROKEN_ARMS, SP_CANDLESTAND_A };
        static const int HOW[4] = { DF_FLIP, DF_FLOAT, DF_SINK, DF_FLIP | DF_SINK };   /* on the ceiling, floating, half sunk below or above */
        for (int k = 0; k < 1 + n * n / 6; k++) {
            int i = rnd(n), j = rnd(n), cx = BX(i) + rnd(3), cy = BX(j) + rnd(3), sp = OUT[rnd(8)], how = HOW[rnd(4)];
            if (i == 0 && j == j0) continue;
            add_deco_ex(cx + 0.5f, cy + 0.5f, sp, sp == SP_CANDLESTAND_A ? 2 : 1, how);
        }
        static const int LOOPER[5][3] = { { SP_SKEL_A, 1, 0 }, { SP_BAT_A, 1, DF_FLOAT }, { SP_SHAMB_A, 2, 0 }, { SP_SPIDER_A, 6, 0 }, { SP_GHOST_A, 1, 0 } };
        for (int k = 0; k < 1 + link / 2; k++) {                   /* the level's own kinds, harmless and untouchable */
            int i = rnd(n), j = rnd(n), w = rnd(5);
            if ((i == 0 && j == j0) || G.level < LOOPER[w][1]) continue;
            add_deco_ex(BX(i) + 1.5f, BX(j) + 1.5f, LOOPER[w][0], 2, DF_LOOP | LOOPER[w][2]);
        }
    }
    /* its creatures: the level's own kinds, away from the way in, a rank higher for each link past the
     * first (up to black), a few more in the Backrooms; the slain stay slain until you die in the chain */
    uint32_t slain = PK.slain[PK.chain][clampi(link, 0, POCKET_LINKS - 1)];
    int want = clampi((1 + n * n / 4) * (back ? 5 : 4) / 4, 2, 16), mg = 0;
    for (int k = 0, tries = 0; k < want && tries < 200; tries++) {
        int i = rnd(n), j = rnd(n), x = BX(i) + rnd(3), y = BX(j) + rnd(3), type = random_type();
        if (i + abs(j - j0) < 2 || G.map[y][x].kind != K_FLOOR || occupied(x, y, NULL)) continue;
        spawn_wanderer_type(x, y, type);
        k++;
        for (int q = 0; q < MAXMOBS; q++) {
            Mob *m = &G.mobs[q];
            if (!m->alive || m->gen) continue;
            m->gen = ++mg;
            if (KINDS[m->type].family >= 0) set_rank(m, imin(m->rank + link - 1, imin(RK_V, KINDS[m->type].top)));
            m->hp = m->maxhp = m->maxhp * theme_boost();
            if (mg <= 32 && (slain >> (mg - 1) & 1)) { m->alive = 0; if (!KIND_IS(m, KF_UNCOUNTED)) G.kills_total--; }
        }
    }
    #undef POCKET_ITEM
    #undef BX
    G.rooms[0] = (Room){ o + 1, o + 4 * j0 + 1, 3, 3, 0, TH_CORRIDOR }; G.nrooms = 1;
    G.ncells = 0;
    for (int y = 0; y < MAPN; y++) for (int x = 0; x < MAPN; x++) if (G.map[y][x].kind == K_FLOOR && lane(x, y)) G.ncells++;
    G.rs = keep_rs ^ (sc->seed + (uint32_t)link);
#ifdef CRAWL_DEBUG
    int alive = 0;
    for (int i = 0; i < G.nitems; i++) alive += G.items[i].alive;
    fprintf(stderr, "POCKET chain %d link %d of %d: %dx%d at %d, back (%d,%d) on (%d,%d), %d things, %d lights, %d of %d light texels\n", PK.chain, link, sc->links, S, S, o, PK.back_x, PK.back_y, PK.on_x, PK.on_y, alive, G.nlights, G.nlc, MAXLC);
#endif
}

/* stand in the cell before a doorway, facing away from it into the place */
static void stand_at_door(int dx, int dy) {
    for (int d = 0; d < 4; d++) {
        int fx = dx + DX[d], fy = dy + DY[d];
        if (fx < 0 || fy < 0 || fx >= MAPN || fy >= MAPN || G.map[fy][fx].kind != K_FLOOR) continue;
        G.cx = G.tx = fx; G.cy = G.ty = fy; G.px = fx + 0.5f; G.py = fy + 0.5f;
        G.h = G.next_h = d; G.ang = heading_angle(d);
        return;
    }
}

/* into a pocket of the chain: the place is built, the caster brought along */
static void collect(Item *it);
static void pocket_build(int link, int from_deeper) {
    G_KEEP = G;
    memset(&G, 0, sizeof G);
    carry_caster(&G_KEEP);
    G.level = G_KEEP.level; G.retries = G_KEEP.retries; G.blessed = 0;
    G.exit_fx = G.exit_fy = G.exit_wx = G.exit_wy = -1; G.exit_in_vault = -1;
    G.altar_x = G.altar_y = -1; G.altar_seek = -1; G.slide = G.seek_secret = -1;
    for (int k = 0; k < NKEYS; k++) G.vault_room[k] = G.vault_door[k] = -1;
    gen_pocket(&G_LEVEL.secrets[PK.chain], link);
    crawl_art_theme(G.ptheme, G.level);
    snd_set_room(G.ptheme == PT_BACKROOMS ? 1 : G.ptheme == PT_SEVERANCE ? 2 : 0);   /* no music, a hum, an echo; Severance's muzak */
    PK.depth = link; PK.since = 0.0f; G.use_kind = 0;
    if (from_deeper) stand_at_door(PK.on_x, PK.on_y); else stand_at_door(PK.back_x, PK.back_y);
    G.map[G.cy][G.cx].visits = 1; G.explored = 1;
    G.level_t = 1.0;                                               /* the pocket's own clock; the hand is free at once */
    G.dead = 0; G.ending = 0; G.leaving = G.exiting = 0;
    hunt_cx = -1;
    if (G.player) human_begin(); else { G.state = PS_MOVE; G.u = 1.0f; }
    snd_sfx_ex(SFX_WHOOSH, 0.9f, 0.5f); snd_sfx_ex(SFX_SHIMMER, 0.5f, 0.5f);
    pocket_say_found(link);
#ifdef CRAWL_DEBUG
    if (getenv("CRAWL_TAKEALL")) for (int i = 0; i < G.nitems; i++) if (G.items[i].alive) collect(&G.items[i]);   /* testing: what was taken stays taken */
#endif
}

/* back out to the level, before the chain's doorway; a death there comes back whole */
static void pocket_return(int died) {
    const Secret *sc = &G_LEVEL.secrets[PK.chain];
    G_KEEP = G;
    G = G_LEVEL;
    carry_caster(&G_KEEP);
    crawl_art_theme(PT_NONE, G.level);
    snd_set_room(0);
    PK.depth = 0; PK.since = 0.0f; PK.auto_back = 0; G.use_kind = 0;
    stand_at_door(sc->door_x, sc->door_y);
    G.camz = 0.5f; G.spd = 0.0f;
    if (died) {                                                    /* thrown out, whole; the pocket behind resets */
        G.hp = G.max_hp = gear_max_hp(&RUN.gear);
        G.poison_t = G.slow_t = 0.0f; G.hurt = 0.0f; G.face_t = 0.0f;
        memset(PK.slain[PK.chain], 0, sizeof PK.slain[PK.chain]);   /* and the chain behind you is as it was */
        say("THROWN BACK OUT");
#ifdef CRAWL_DEBUG
        fprintf(stderr, "EVENT pocket_thrown chain %d t=%.2f\n", PK.chain, G.t);
#endif
    }
    hunt_cx = -1;
    if (G.player) human_begin(); else { G.state = PS_MOVE; G.u = 1.0f; }
    snd_sfx_ex(SFX_WHOOSH, 0.8f, 0.5f);
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT pocket_out chain %d level_t=%.2f hp=%d t=%.2f player=%d died=%d\n", PK.chain, G.level_t, G.hp, G.t, G.player, died);
#endif
}

/* walked into a doorway (secret 3) at (x, y): which way it goes */
static int pocket_door(int x, int y) {
    if (abs(x - G.cx) + abs(y - G.cy) != 1) return 0;
    return pocket_go(x, y);
}
static int pocket_go(int x, int y) {
    if (!PK.depth) {
        for (int i = 0; i < G.nsecrets; i++)
            if (G.secrets[i].door_x == x && G.secrets[i].door_y == y && G.secrets[i].state == 2) {
#ifdef CRAWL_DEBUG
                fprintf(stderr, "EVENT pocket_in chain %d level_t=%.2f t=%.2f\n", i, G.level_t, G.t);
#endif
                G_LEVEL = G; PK.chain = i;
                pocket_build(2, 0);
                return 1;
            }
        return 0;
    }
    if (x == PK.back_x && y == PK.back_y) {
        if (PK.depth <= 2) pocket_return(0); else pocket_build(PK.depth - 1, 1);
        return 1;
    }
    if (x == PK.on_x && y == PK.on_y) { pocket_build(PK.depth + 1, 0); return 1; }
    return 0;
}
static void pocket_reset_level(void) { memset(&PK, 0, sizeof PK); PK.level = G.level; snd_set_room(0); }

static void crawl_enter(uint32_t seed) {
    crawl_art_init();
    CON.open = 0; CON.slide = 0.0f; snd_hold(0);                   /* the console goes up; what a code did stays */
    int resume = RUN.active;
    if (resume && !RUN.loaded) {                                   /* back from the other scene: where was it? */
        int won = G.tally || G.exiting;
        RUN.level = G.level + won; RUN.hp = G.hp > 0 ? G.hp : gear_max_hp(&RUN.gear);
        RUN.shop_next = won || (G.shop && !G.shop_bought);
    }
    RUN.loaded = 0;
    memset(&G, 0, sizeof G);
    G.rs = seed | 1u;
    if (!resume) {
        memset(&RUN, 0, sizeof RUN);
        RUN.active = 1; RUN.level = 1; RUN.hp = 100; RUN.shop_next = 1;
        gear_reset(&RUN.gear);
#ifdef CRAWL_DEBUG
        if (getenv("CRAWL_LEVEL")) RUN.level = atoi(getenv("CRAWL_LEVEL"));
        debug_gear();
#endif
    }
#ifdef CRAWL_DEBUG
    if (getenv("CRAWL_SEED")) G.rs = (uint32_t)atoi(getenv("CRAWL_SEED")) | 1u;
#endif
    G.level = RUN.level; G.hp = RUN.hp;
    if (RUN.shop_next) begin_shop(); else start_level();
}

static void collect(Item *it) {
    it->alive = 0;
    if (it->gen && PK.depth) PK.taken[PK.chain][clampi(PK.depth, 0, POCKET_LINKS - 1)] |= 1u << (it->gen - 1);   /* gone from the pocket for good */
    switch (it->type) {
    case IT_KEY:
        G.has_key[it->color] = 1; snd_sfx(SFX_KEYGET); G.gold = 1.0f; G.grin_t = 1.6f;
#ifdef CRAWL_DEBUG
        bfs_from(G.doors[G.vault_door[it->color]].x, G.doors[G.vault_door[it->color]].y);
        fprintf(stderr, "EVENT key%d t=%.2f path=%d\n", it->color, G.t, bfs_dist[G.cy][G.cx]);
#endif
        break;
    case IT_POTION: G.hp = G.hp + 25 > G.max_hp ? G.max_hp : G.hp + 25; snd_sfx(SFX_POTION); break;
    case IT_SCRIP: {                                               /* a boss's: to the Hexed Totem's account at once */
        int b = clampi(it->color, 0, BOSS_REC - 1), n = G.bstep >= 3 ? 2 : 1;
        RUN.scrip[b] += n; RUN.rec.boss[b].scrip = (uint16_t)imin(65535, RUN.rec.boss[b].scrip + n);
        toast(SP_SCRIP, -1, RK_I, 0, b < NBOSS ? BOSSES[b].short_name : "A BOSS", n > 1 ? "TWO OF ITS ODIOUS SCRIP" : "ITS ODIOUS SCRIP");
        snd_sfx(SFX_KEYGET); G.gold = 1.0f; G.grin_t = 1.6f;
#ifdef CRAWL_DEBUG
        fprintf(stderr, "EVENT scrip %d +%d = %d t=%.2f\n", b, n, RUN.scrip[b], G.t);
#endif
        break;
    }
    case IT_BONE:                                                  /* for the spear */
        if (RUN.gear.bones < BONES_MAX) RUN.gear.bones++;
        snd_sfx_ex(SFX_BONES, 0.5f, 0.5f);
        break;
    case IT_URN: {                                                 /* it shatters and the souls inside are yours */
        int n = 5 + rnd(6);
        RUN.souls += n; G.level_souls += n;
        snd_sfx_ex(SFX_SHATTER, 0.8f, 0.5f); snd_sfx_ex(SFX_SHIMMER, 0.6f, 0.5f); G.chill = 0.5f;
        break;
    }
    case IT_RING: {                                                /* a ring, for nothing */
        Offer o;
        memset(&o, 0, sizeof o);
        o.kind = GK_PIECE; o.id = IK_RING;
        o.piece = piece_roll(o.id, G.level, -1, &G.rs); o.tier = G.level;
        take_piece(&o, 0);
        snd_sfx(SFX_KEYGET); G.gold = 1.0f; G.grin_t = 1.6f;
        break;
    }
    default: {
        int v = treasure_value(it->type);
        int got = (int)(v * (1.0f + gear_ability(&RUN.gear, AB_TREASURE, 0) / 100.0f) + 0.5f);   /* Treasure Hunter */
        RUN.treasure += got; G.level_treasure += got;
        if (!it->secret) { G.value_got += v; G.items_got++; }
        snd_sfx_ex(SFX_COIN, it->type >= IT_CROWN ? 1.2f : 0.9f, 0.5f);
        G.gold = it->type >= IT_CROWN ? 1.0f : 0.6f;
        if (it->type >= IT_CROWN) G.grin_t = 1.6f;
        break;
    }
    }
    pop_readout();
}

/* ---- hazards (docs/PLAN-b7.md, section 6) ----
 * Rockfall: grit and a growing shadow, then a rock that strikes once and lies as rubble. Hellrent:
 * cracks running along a line, glowing up to the burst, then fire for a moment and embers. Bile: a wet
 * sheen creeping from an outlet a cell at a time, foul (poison, and slow) once it has settled, draining
 * back when the outlet shuts. The void: a ring and a thin column, then a purple volume that saps health,
 * slows and silences. The Sundering Arc: a pale arc on the floor before the knight, then its sweep.
 * Grasping Dark: a black ring under you, then hands. Omens are the same at half the harm. */
static uint8_t hz_dmap[MAPN][MAPN];                                 /* warned or harmful cells, for the autopilot */
static int hz_step(void) { return G.boss >= 0 ? G.bstep : 0; }
static float hz_warn(float base) { return fmaxf(0.9f, base - 0.06f * (float)hz_step()); }   /* never under 0.9 s */
static Hazard *hz_new(int kind, int omen) {
    for (int i = 0; i < MAXHAZ; i++)
        if (!G.haz[i].alive) {
            Hazard *h = &G.haz[i];
            memset(h, 0, sizeof *h);
            h->alive = 1; h->kind = kind; h->omen = omen; h->src = -1; h->wx = -1;
#ifdef CRAWL_DEBUG
            fprintf(stderr, "EVENT hazard %d%s t=%.2f\n", kind, omen ? " omen" : "", G.t);
#endif
            if (!(RUN.omens >> kind & 1)) {                        /* the first of each kind is named */
                static const char *const NAME[NHZ] = { "THE CEILING GROANS ABOVE YOU", "THE FLOOR SPLITS AND BURNS", "BILE RISES FROM THE DRAINS",
                                                       "THE VOID OPENS", "MIND THE BLADE'S ARC", "THE DARK REACHES UP" };
                RUN.omens |= 1 << kind;
                say("%s", NAME[kind]);
            }
            return h;
        }
    return NULL;
}
static int hz_phase(const Hazard *h) { return h->t < h->warn ? 0 : (h->t < h->warn + h->on ? 1 : 2); }   /* warning, harm, fading */
static int hz_floor(int x, int y) { return x > 0 && y > 0 && x < MAPN - 1 && y < MAPN - 1 && G.map[y][x].kind == K_FLOOR && !G.map[y][x].secret; }

static void hz_rock(float x, float y, int omen) {
    Hazard *h = hz_new(HZ_ROCK, omen);
    if (!h) return;
    h->x = x; h->y = y; h->r = 0.55f; h->warn = hz_warn(1.2f); h->on = 0.05f; h->off = 3.0f; h->share = omen ? 0.06f : 0.12f;
    sfx_at(SFX_CREAK, x, y, 0.7f);
}
/* cells for hellrent: a line from (x,y) towards (tx,ty), so many long, in order */
static int hz_line(Hazard *h, float x, float y, float tx, float ty, int len, int from) {
    float a = atan2f(ty - y, tx - x), cx = cosf(a), cy = sinf(a);
    int n = h->n;
    for (int i = 0; i < len * 3 && n < HZ_CELLS; i++) {
        int px = (int)floorf(x + cx * i / 3.0f), py = (int)floorf(y + cy * i / 3.0f), dup = 0;
        if (!hz_floor(px, py)) { if (G.map[clampi(py, 0, MAPN - 1)][clampi(px, 0, MAPN - 1)].kind == K_WALL) break; continue; }
        for (int k = 0; k < n; k++) dup |= h->cx[k] == px && h->cy[k] == py;
        if (dup) continue;
        h->cx[n] = (int8_t)px; h->cy[n] = (int8_t)py; h->cd[n] = (uint8_t)(from + i / 3); n++;
        if (i / 3 >= len) break;
    }
    return h->n = n;
}
static Hazard *hz_fire(int omen) {
    Hazard *h = hz_new(HZ_FIRE, omen);
    if (!h) return NULL;
    h->warn = hz_warn(1.5f); h->on = 1.2f; h->off = 2.0f; h->share = omen ? 0.04f : 0.08f;
    return h;
}
static void hz_fire_done(Hazard *h, float sx, float sy) {
    if (!h) return;
    if (!h->n) { h->alive = 0; return; }
    int maxd = 1;
    for (int i = 0; i < h->n; i++) if (h->cd[i] > maxd) maxd = h->cd[i];
    h->r = (float)maxd;
    h->x = sx; h->y = sy;
    sfx_at(SFX_PUSHWALL, sx, sy, 0.6f);
}
static Hazard *hz_bile(const int8_t *cx, const int8_t *cy, const uint8_t *cd, int n, int omen) {
    Hazard *h = hz_new(HZ_BILE, omen);
    if (!h) return NULL;
    h->n = imin(n, HZ_CELLS);
    int maxd = 1;
    for (int i = 0; i < h->n; i++) { h->cx[i] = cx[i]; h->cy[i] = cy[i]; h->cd[i] = cd[i]; if (cd[i] > maxd) maxd = cd[i]; }
    h->r = (float)maxd; h->warn = 2.0f; h->on = omen ? 8.0f : 1e6f; h->off = 3.0f; h->share = omen ? 0.5f : 1.0f;
    if (h->n) { h->x = h->cx[0] + 0.5f; h->y = h->cy[0] + 0.5f; }
    sfx_at(SFX_DRIP, h->x, h->y, 1.0f); sfx_at(SFX_GROAN, h->x, h->y, 0.3f);
    return h;
}
static void hz_void(float x, float y, float r, int omen) {
    Hazard *h = hz_new(HZ_VOID, omen);
    if (!h) return;
    h->x = x; h->y = y; h->r = r; h->warn = hz_warn(1.5f); h->on = omen ? 5.0f : 6.0f + 2.0f * rndf(); h->off = 1.0f; h->share = omen ? 0.02f : 0.04f;
    sfx_at(SFX_SHIMMER, x, y, 0.5f);
}
static void hz_arc(float x, float y, float ang, float share) {
    Hazard *h = hz_new(HZ_ARC, 0);
    if (!h) return;
    h->x = x; h->y = y; h->ang = ang; h->r = 3.2f; h->warn = hz_warn(0.8f); h->on = 0.05f; h->off = 0.4f; h->share = share;
}
static void hz_grasp(float x, float y) {
    Hazard *h = hz_new(HZ_GRASP, 0);
    if (!h) return;
    h->x = x; h->y = y; h->r = 0.8f; h->warn = hz_warn(1.0f); h->on = 0.05f; h->off = 0.7f; h->share = 0.10f;
    sfx_at(SFX_WHISPER, x, y, 0.8f);
}

/* how far a bile cell has come: 0 dry, up to 1 settled and foul (the sheen creeps out, then drains back) */
static float hz_bile_wet(const Hazard *h, int i) {
    float creep = h->t / h->warn * (h->r + 1.0f) - (float)h->cd[i];
    if (h->t >= h->warn + h->on) creep = fminf(creep, (1.0f - (h->t - h->warn - h->on) / h->off) * (h->r + 1.0f) - (float)h->cd[i]);
    return clampf(creep, 0.0f, 1.0f);
}
static int hz_in_arc(const Hazard *h, float x, float y) {
    float d = hypotf(x - h->x, y - h->y);
    return d <= h->r && d > 0.2f && fabsf(wrap_pi(atan2f(y - h->y, x - h->x) - h->ang)) <= 0.8f;
}
/* how much a hazard has a cell: 2 harming, 1 warning (or fading fire), 0 not at all */
static int hz_covers(const Hazard *h, int x, int y) {
    int ph = hz_phase(h);
    float cx = x + 0.5f, cy = y + 0.5f;
    switch (h->kind) {
    case HZ_ROCK: return ph == 0 && hypotf(cx - h->x, cy - h->y) < h->r + 0.45f ? 1 : 0;
    case HZ_FIRE: for (int i = 0; i < h->n; i++) if (h->cx[i] == x && h->cy[i] == y) return ph == 1 ? 2 : (ph == 0 ? 1 : 0); return 0;
    case HZ_BILE: for (int i = 0; i < h->n; i++) if (h->cx[i] == x && h->cy[i] == y) { float w = hz_bile_wet(h, i); return w >= 1.0f && ph < 2 ? 2 : (w > 0.0f ? 1 : 0); } return 0;
    case HZ_VOID: return ph < 2 && hypotf(cx - h->x, cy - h->y) < h->r + 0.35f ? (ph == 1 ? 2 : 1) : 0;
    case HZ_ARC: return ph == 0 && hz_in_arc(h, cx, cy) ? 1 : 0;
    default: return ph == 0 && hypotf(cx - h->x, cy - h->y) < h->r + 0.35f ? 1 : 0;   /* HZ_GRASP */
    }
}

static void update_hazards(float dt) {
    int was = G.silenced;
    G.silenced = 0; G.hz_slow = 1.0f;
    memset(hz_dmap, 0, sizeof hz_dmap);
    if (PK.depth || G.shop || G.tally) { if (was) snd_set_muffle(0); return; }
    int px = (int)G.px, py = (int)G.py, mhp = G.max_hp > 0 ? G.max_hp : 100;
    for (int i = 0; i < MAXHAZ; i++) {
        Hazard *h = &G.haz[i];
        if (!h->alive) continue;
        float pt = h->t;
        h->t += dt;
        if (h->t >= h->warn + h->on + h->off) {                    /* done: an omen's grate is a wall again */
            if (h->wx >= 0) G.map[h->wy][h->wx].tex = (uint8_t)h->wtex;
            h->alive = 0; continue;
        }
        int ph = hz_phase(h), struck = pt < h->warn && h->t >= h->warn;
        switch (h->kind) {
        case HZ_ROCK:
            if (struck) {
                sfx_at(SFX_BOOM, h->x, h->y, 0.9f); sfx_at(SFX_SHATTER, h->x, h->y, 0.5f);
                for (int j = 0; j < MAXBOOMS; j++) if (!G.booms[j].alive) { memset(&G.booms[j], 0, sizeof G.booms[j]); G.booms[j].alive = 1; G.booms[j].x = h->x; G.booms[j].y = h->y; break; }
                if (!G.dead && hypotf(G.px - h->x, G.py - h->y) < h->r + BODY) { int d = hurt_ex(h->share * (float)mhp, HURT_STONE, h->x, h->y);
#ifdef CRAWL_DEBUG
                    fprintf(stderr, "EVENT hz_hit rock %d t=%.2f\n", d, G.t);
#endif
                    (void)d; }
            }
            break;
        case HZ_FIRE:
            if (struck) { sfx_at(SFX_IGNITE, h->x, h->y, 1.0f); sfx_at(SFX_EXPLODE, h->x, h->y, 0.5f); }
            if (ph == 1) for (int k = 0; k < h->n; k++) if (h->cx[k] == px && h->cy[k] == py) { G.hz_acc += h->share * (float)mhp * dt; break; }
            break;
        case HZ_BILE:
            if (ph < 2)
                for (int k = 0; k < h->n; k++)
                    if (h->cx[k] == px && h->cy[k] == py && hz_bile_wet(h, k) >= 1.0f && G.purify_t <= 0.0f) {   /* Purify keeps it off */
                        float r = 1.0f - gear_poison_resist(&RUN.gear);
                        G.poison_t = fmaxf(G.poison_t, 2.5f * r);
                        G.poison_dps = fmaxf(G.poison_dps, (1.0f + (float)imax(0, (G.level - 4) / 2)) * h->share * (0.5f + 0.5f * r));
                        G.hz_slow = fminf(G.hz_slow, 0.8f);
                        break;
                    }
            break;
        case HZ_VOID:
            if (struck) sfx_at(SFX_WAIL, h->x, h->y, 0.5f);
            if (ph == 1 && hypotf(G.px - h->x, G.py - h->y) < h->r) { G.hz_acc += h->share * (float)mhp * dt; G.hz_slow = fminf(G.hz_slow, 0.6f); G.silenced = 1; }
            break;
        case HZ_ARC:
            if (struck) { snd_sfx_ex(SFX_SWORD, 1.2f, 0.5f); if (!G.dead && hz_in_arc(h, G.px, G.py)) hurt_ex(h->share, HURT_BLOW, h->x, h->y); }
            break;
        case HZ_GRASP:
            if (struck) {
                sfx_at(SFX_DIRT, h->x, h->y, 0.8f);
                if (!G.dead && hypotf(G.px - h->x, G.py - h->y) < h->r + BODY) { hurt_ex(h->share * (float)mhp, HURT_BLOW, h->x, h->y); G.slow_t = fmaxf(G.slow_t, 1.0f); }
            }
            break;
        }
        int x0 = imax(1, (int)(h->x - h->r - 2.0f)), x1 = imin(MAPN - 2, (int)(h->x + h->r + 2.0f));
        int y0 = imax(1, (int)(h->y - h->r - 2.0f)), y1 = imin(MAPN - 2, (int)(h->y + h->r + 2.0f));
        if (h->kind == HZ_FIRE || h->kind == HZ_BILE)
            for (int k = 0; k < h->n; k++) { int c = hz_covers(h, h->cx[k], h->cy[k]); if (c > hz_dmap[h->cy[k]][h->cx[k]]) hz_dmap[h->cy[k]][h->cx[k]] = (uint8_t)c; }
        else
            for (int y = y0; y <= y1; y++) for (int x = x0; x <= x1; x++) { int c = hz_covers(h, x, y); if (c > hz_dmap[y][x]) hz_dmap[y][x] = (uint8_t)c; }
    }
    while (G.hz_acc >= 1.0f && !G.dead) {                          /* fire and void, a point at a time */
        G.hz_acc -= 1.0f;
#ifdef CRAWL_DEBUG
        static int burns; if ((++burns & 15) == 1) fprintf(stderr, "EVENT hz_hit %s x16 t=%.2f\n", G.silenced ? "void" : "fire", G.t);
#endif
        hurt_ex(1.0f, G.silenced ? HURT_VOID : HURT_MAGIC, G.px, G.py);
    }
    if (G.dead) G.hz_acc = 0.0f;
    if (G.silenced != was) snd_set_muffle(G.silenced);
    G.sil_k += ((float)G.silenced - G.sil_k) * fminf(1.0f, dt * 8.0f);
}
static int hz_danger(int x, int y) { return x >= 0 && y >= 0 && x < MAPN && y < MAPN ? hz_dmap[y][x] : 0; }
static void hz_clear(void) {                                       /* a new place: none of them come along */
    for (int i = 0; i < MAXHAZ; i++) if (G.haz[i].alive && G.haz[i].wx >= 0) G.map[G.haz[i].wy][G.haz[i].wx].tex = (uint8_t)G.haz[i].wtex;
    memset(G.haz, 0, sizeof G.haz); memset(hz_dmap, 0, sizeof hz_dmap);
    G.hz_acc = 0.0f; G.hz_slow = 1.0f;
    if (G.silenced) { G.silenced = 0; snd_set_muffle(0); }
}

/* a floor cell near the dweller, in sight, for an omen: from d0 to d1 off, ahead of you if it can be,
 * so that it is seen */
static int hz_near(float d0, float d1, int *ox, int *oy) {
    for (int tries = 0; tries < 60; tries++) {
        float a = tries < 40 ? G.ang + (rndf() - 0.5f) * 2.0f : rndf() * TAU_F, d = d0 + (d1 - d0) * rndf();
        int x = (int)floorf(G.px + cosf(a) * d), y = (int)floorf(G.py + sinf(a) * d);
        if (!hz_floor(x, y) || in_band(y) || vault_of_room(x, y) >= 0 || !line_clear(G.px, G.py, x + 0.5f, y + 0.5f)) continue;
        *ox = x; *oy = y;
        return 1;
    }
    return 0;
}
/* bile's patch: the floor within reach of a cell, nearest first */
static int hz_patch(int sx, int sy, int reach, int8_t *cx, int8_t *cy, uint8_t *cd, int max) {
    static int qx[MAPN * MAPN], qy[MAPN * MAPN], qd[MAPN * MAPN];
    static uint8_t seen[MAPN][MAPN];
    memset(seen, 0, sizeof seen);
    int qh = 0, qt = 0, n = 0;
    if (!hz_floor(sx, sy)) return 0;
    qx[qt] = sx; qy[qt] = sy; qd[qt++] = 0; seen[sy][sx] = 1;
    while (qh < qt && n < max) {
        int x = qx[qh], y = qy[qh], d = qd[qh++];
        cx[n] = (int8_t)x; cy[n] = (int8_t)y; cd[n] = (uint8_t)d; n++;
        if (d >= reach) continue;
        for (int k = 0; k < 4; k++) {
            int nx = x + DX[k], ny = y + DY[k];
            if (seen[ny][nx] || !hz_floor(nx, ny)) continue;
            seen[ny][nx] = 1; qx[qt] = nx; qy[qt] = ny; qd[qt++] = d + 1;
        }
    }
    return n;
}

/* the omens: in the three levels before a boss, and in its own level's halls, its hazard comes by
 * itself near the dweller, at half the harm: a stone from the ceiling in the Crypts, bile from a drain in
 * the Catacombs, the floor splitting in the Hollows, the void in the Deep */
static float omen_every(void) { static const float E[4] = { 90.0f, 60.0f, 40.0f, 30.0f }; int s = crawl_step(G.level) - (RUN_LEN - 4); return s >= 0 ? E[s] : 0.0f; }
static void update_omens(float dt) {
    float every = omen_every();
#ifdef CRAWL_DEBUG
    if (getenv("CRAWL_OMEN")) { every = (float)atof(getenv("CRAWL_OMEN")); G.omen_t = fminf(G.omen_t, every); }   /* testing: omens on any level, this often */
#endif
    if (PK.depth || G.shop || G.tally || G.dead || G.exiting || G.level_t < 8.0 || every <= 0.0f || in_arena(G.px, G.py)) return;
    if ((G.omen_t -= dt) > 0.0f) return;
    G.omen_t = every * (0.75f + 0.5f * rndf());
    int x, y;
    switch (crawl_set(G.level)) {
    case 0: if (hz_near(1.0f, 5.0f, &x, &y)) hz_rock(x + 0.5f, y + 0.5f, 1); break;
    case 1:                                                        /* a drain in a wall near you, onto the lane */
        for (int tries = 0; tries < 40; tries++) {
            if (!hz_near(2.0f, 6.0f, &x, &y)) break;
            int d = rnd(4), wx = x + DX[d], wy = y + DY[d];
            Cell *w = &G.map[wy][wx];
            if (w->kind != K_WALL || w->secret || (w->tex >= T_TORCH0 && w->tex <= T_TORCH2) || w->tex == T_EXIT0 || wx <= 0 || wy <= 0 || wx >= MAPN - 1 || wy >= MAPN - 1) continue;
            int8_t cx[HZ_CELLS], cy[HZ_CELLS]; uint8_t cd[HZ_CELLS];
            int n = hz_patch(x, y, 2, cx, cy, cd, 12);
            Hazard *h = n ? hz_bile(cx, cy, cd, n, 1) : NULL;
            if (h) { h->wx = wx; h->wy = wy; h->wtex = w->tex; w->tex = T_GRATE; }
            break;
        }
        break;
    case 2:
        if (hz_near(1.0f, 4.0f, &x, &y)) { Hazard *h = hz_fire(1); float a = rndf() * TAU_F; if (h) { hz_line(h, x + 0.5f, y + 0.5f, x + 0.5f + cosf(a), y + 0.5f + sinf(a), 4 + rnd(3), 0); hz_fire_done(h, x + 0.5f, y + 0.5f); } }
        break;
    default: if (hz_near(1.5f, 5.0f, &x, &y)) hz_void(x + 0.5f, y + 0.5f, 1.2f, 1); break;
    }
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT omen set %d t=%.2f\n", crawl_set(G.level), G.t);
#endif
}

/* a boss's own hazard, over its fight: how often and how much grow with the challenge's steps */
static void boss_hazards(float dt) {
    const Mob *m = boss_mob();
    int step = G.bstep, wave = G.bstate == BS_WAVE || G.bstate == BS_RISE;
    float pace = 1.0f + 0.2f * (float)step;
    if (G.bcast_t > 0.0f) G.bcast_t -= dt;
    switch (G.boss) {
    case BOSS_KNIGHT:                                              /* Rockfall: three to five about you, one on your cell */
        if (G.bdash_t > 0.0f || G.bpose_t > 0.0f) break;            /* one thing at a time */
        if (m && !wave && (G.bz2_t -= dt * pace) <= 0.0f && hypotf(G.px - m->x, G.py - m->y) < 3.0f) {   /* the Sundering Arc, at you */
            G.bz2_t = 7.0f + 3.0f * rndf();
            hz_arc(m->x, m->y, atan2f(G.py - m->y, G.px - m->x), mob_damage(m) * 1.3f);
            G.bpose = BP_RAISE; G.bpose_t = hz_warn(0.8f);         /* the sword up while the arc shows, then the sweep */
        }
        if ((G.bz_t -= dt * pace * (wave ? 0.5f : 1.0f)) > 0.0f) break;
        G.bz_t = 10.0f + 4.0f * rndf();
        G.bcast_t = 0.5f;
        if (!wave) { G.bpose = BP_PLANT; G.bpose_t = 0.5f; }        /* it plants its sword with a shout */
        if (m) sfx_at(SFX_GROAN, m->x, m->y, 1.0f);
        hz_rock(G.px, G.py, 0);
        for (int i = 0, n = 2 + rnd(3) + step / 2; i < n; i++) {
            float a = rndf() * TAU_F, d = 1.5f + 2.5f * rndf();
            int x = (int)floorf(G.px + cosf(a) * d), y = (int)floorf(G.py + sinf(a) * d);
            if (in_arena((float)x, (float)y) && hz_floor(x, y)) hz_rock(x + 0.5f, y + 0.5f, 0);
        }
        break;
    case BOSS_SPIDER: {                                            /* the outlets: so many open at once, one changing every 20 s */
        if (m && !wave && (G.bz2_t -= dt * pace) <= 0.0f) { G.bz2_t = 14.0f + 4.0f * rndf(); egg_shot(m); }   /* Egg Shot */
        int want = imin(G.bnout, 2 + (step >= 2) + (step >= 4) + wave), open = 0;
        for (int o = 0; o < G.bnout; o++) {
            int hi = G.bout_haz[o];
            if (hi >= 0 && (!G.haz[hi].alive || G.haz[hi].src != o)) G.bout_haz[o] = hi = -1;
            open += hi >= 0 && hz_phase(&G.haz[hi]) < 2;
        }
        int change = (G.bz_t -= dt * pace) <= 0.0f;
        if (change) G.bz_t = 20.0f;
        if (change || open > want) {                               /* shut one: it drains back */
            int c[MAXOUT], nc = 0;
            for (int o = 0; o < G.bnout; o++) if (G.bout_haz[o] >= 0 && hz_phase(&G.haz[G.bout_haz[o]]) < 2) c[nc++] = o;
            if (nc) { Hazard *h = &G.haz[G.bout_haz[c[rnd(nc)]]]; h->on = fmaxf(0.0f, h->t - h->warn); if (h->t < h->warn) h->warn = h->t; open--; }
        }
        for (int tries = 0; open < want && tries < 8; tries++) {   /* and another opens */
            int o = rnd(G.bnout);
            if (G.bout_haz[o] >= 0) continue;
            Hazard *h = hz_bile(G.bout_cx[o], G.bout_cy[o], G.bout_cd[o], G.bout_n[o], 0);
            if (h) { h->src = o; G.bout_haz[o] = (int)(h - G.haz); open++; }
        }
        break;
    }
    case BOSS_DEACON:                                              /* Hellrent: a line at you, a ring round him, a cross */
        if (!wave && m && m->swing <= 0.0f && G.bpose_t <= 0.0f && (G.bz2_t -= dt * pace) <= 0.0f) {   /* Litany */
            G.bz2_t = 18.0f + 6.0f * rndf();
            G.bpose = BP_LITANY; G.bpose_t = 2.0f;
            sfx_at(SFX_WAIL, m->x, m->y, 0.9f);
        }
        if (wave || !m || (G.bz_t -= dt * pace) > 0.0f) break;
        G.bz_t = 9.0f + 3.0f * rndf();
        G.bcast_t = 0.6f;
        for (int k = 0; k < 1 + (step >= 2); k++) {
            Hazard *h = hz_fire(0);
            if (!h) break;
            int shape = rnd(3);
            if (shape == 0) hz_line(h, m->x, m->y, G.px, G.py, 10, 0);
            else if (shape == 1) for (int a = 0; a < 8; a++) hz_line(h, m->x + cosf(a * TAU_F / 8.0f) * 2.0f, m->y + sinf(a * TAU_F / 8.0f) * 2.0f, m->x + cosf(a * TAU_F / 8.0f) * 3.0f, m->y + sinf(a * TAU_F / 8.0f) * 3.0f, 0, a & 1);
            else { hz_line(h, G.px - 4.0f, G.py, G.px + 4.0f, G.py, 8, 0); hz_line(h, G.px, G.py - 4.0f, G.px, G.py + 4.0f, 8, 0); }
            hz_fire_done(h, m->x, m->y);
        }
        break;
    case BOSS_WRAITH:                                              /* Void Energies: one on you, the rest across your ways out */
        if ((G.bz2_t -= dt * pace) <= 0.0f) {                      /* Grasping Dark, wherever no fire is near */
            G.bz2_t = 6.0f + 3.0f * rndf();
            int lit = 0;
            for (int l = 0; l < G.nlights; l++) lit |= hypotf(G.lights[l].x - G.px, G.lights[l].y - G.py) < 3.5f;
            if (!lit) hz_grasp(G.px, G.py);
        }
        if ((G.bz_t -= dt * pace) > 0.0f) break;
        G.bz_t = 8.0f + 4.0f * rndf();
        G.bcast_t = 0.6f;
        hz_void(G.px, G.py, 1.4f, 0);
        for (int i = 0, n = rnd(3) + (step >= 3) + (step >= 5); i < n; i++) {
            float a = rndf() * TAU_F, d = 3.0f + 2.0f * rndf();
            int x = (int)floorf(G.px + cosf(a) * d), y = (int)floorf(G.py + sinf(a) * d);
            if (in_arena((float)x, (float)y) && hz_floor(x, y)) hz_void(x + 0.5f, y + 0.5f, 1.2f + 0.5f * rndf(), 0);
        }
        break;
    }
}

/* ---- a boss's fight (docs/PLAN-b7.md) ----
 * It waits, untouchable, until you are a step into its arena; then the arch seals behind you, its name
 * comes up and its bar along the top of the view. At 75%, 50%, 25% and 10% of its health it stops (the
 * blow that would take it past the mark is lost), turns untouchable and calls a wave; two seconds later
 * the adds come in where its map says, and once the last of them is down it rises and fights on. When it
 * falls, its scrip drops, the way out opens, and on the way down the run is done. */
static Mob *boss_mob(void) {
    if (G.boss < 0 || G.bmob < 0 || PK.depth) return NULL;
    Mob *m = &G.mobs[G.bmob];
    return m->alive && m->boss ? m : NULL;
}
static int boss_sealed(void) { return G.boss >= 0 && !PK.depth && G.bstate != BS_DEAD; }   /* its way out shut till it falls */
static int boss_fighting(void) { return G.boss >= 0 && !PK.depth && !G.shop && G.bstate >= BS_FIGHT && G.bstate <= BS_RISE; }
static int in_arena(float x, float y) { return G.boss >= 0 && !PK.depth && y >= BAND_Y0 && x >= ARENA_X0 && x < MAPN - 1 && y < MAPN - 1; }   /* the map's rim is stone */
static void font_fill(int i) {
    if (i < 0 || i >= G.bnfont || G.bfont_full[i]) return;
    G.bfont_full[i] = 1; G.map[G.bfont_y[i]][G.bfont_x[i]].tex = T_FONT;
    sfx_at(SFX_DRIP, G.bfont_x[i] + 0.5f, G.bfont_y[i] + 0.5f, 1.0f);
}
static int font_drink(int i) {                                     /* E at a font: a third of your health, once */
    if (!G.bfont_full[i]) { say("THE FONT IS DRY"); snd_sfx_ex(SFX_KNOCK, 0.5f, 0.5f); return 1; }
    G.bfont_full[i] = 0; G.map[G.bfont_y[i]][G.bfont_x[i]].tex = T_FONT_DRY;
    G.hp = imin(G.max_hp, G.hp + (G.max_hp + 2) / 3);
    snd_sfx_ex(SFX_POTION, 1.0f, 0.5f); G.grab_t = 1.0f; show_bar(); pop_readout();
    say("THE FONT GIVES WHAT IT HOLDS");
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT font %d hp=%d t=%.2f\n", i, G.hp, G.t);
#endif
    return 1;
}
static int font_facing(void) {                                     /* the font the dweller faces, near enough to drink */
    int cx = (int)floorf(G.px + cosf(G.ang) * 0.8f), cy = (int)floorf(G.py + sinf(G.ang) * 0.8f);
    for (int i = 0; i < G.bnfont && !PK.depth; i++)
        if (cx == G.bfont_x[i] && cy == G.bfont_y[i] && hypotf(G.bfont_fx[i] + 0.5f - G.px, G.bfont_fy[i] + 0.5f - G.py) < 1.2f) return i;
    return -1;
}

static void boss_spawn(void) {
    const BossDef *B = &BOSSES[G.boss];
    G.bmob = -1;
    if (G.bhome_x < 0) return;
    Mob *m = spawn_mob(G.bhome_x, G.bhome_y, B->kind);
    if (!m) return;
    m->boss = 1 + G.boss;
    set_rank(m, RK_I);
    float hp = B->hp * KINDS[MOB_KNIGHT].hp * powf(1.04f, (float)(G.level - 1)) * (1.0f + 0.25f * (float)G.bstep);
#ifdef CRAWL_DEBUG
    if (getenv("CRAWL_BOSSHP")) hp *= (float)atof(getenv("CRAWL_BOSSHP"));
#endif
    m->hp = m->maxhp = hp;
    m->speed *= B->speed;
    m->dmgk *= B->blow * (1.0f + 0.1f * (float)G.bstep);
    m->home = -1; m->face = PI_F;                                  /* facing the arch */
    if (m->type == MOB_HUNTER) m->sub = 0;                         /* overhead, waiting */
    G.bmob = (int)(m - G.mobs);
}

static void boss_begin(void) {
    G.bstate = BS_FIGHT; G.bstate_t = 0.0f; G.bfight_t = 0.0f; G.bcard = 0.001f;
    if (G.barch >= 0) { Door *d = &G.doors[G.barch]; d->locked = SEALED; d->opening = 0; d->idle = 99.0f; }
    Mob *m = boss_mob();
    if (m) { m->alert = 99.0f; m->seen = 1; rec_seen(m); }
    if (m && G.boss == BOSS_DEACON) G.bward = 0.04f * m->maxhp;   /* his Bone Ward, whole */
    RUN.rec.boss[G.boss].met = 1;
    if (RUN.rec.boss[G.boss].tries < 65535) RUN.rec.boss[G.boss].tries++;
    font_fill(0);
    snd_sfx_ex(SFX_DOOR, 1.2f, 0.5f); snd_sfx_ex(SFX_STINGER, 1.0f, 0.5f);
    snd_set_boss(G.boss + 1, 0);                                   /* its waltz */
    show_bar();
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT boss_begin %s step %d hp=%.0f t=%.2f by %s\n", BOSSES[G.boss].name, G.bstep, m ? m->maxhp : 0.0f, G.t, G.player ? "player" : "autopilot");
#endif
}

static void boss_wave(Mob *m) {
    G.bstate = BS_WAVE; G.bstate_t = 0.0f; G.badds = -1; G.bwave++;
    G.bdash_t = 0.0f; G.bpose = BP_NONE; G.bpose_t = 0.0f;
    if (m->type == MOB_KNIGHT && knight_stage(m) != m->sub) {      /* plate falls away */
        m->sub = knight_stage(m); m->speed = knight_pace(m, m->sub);
        sfx_at(SFX_SHATTER, m->x, m->y, 1.3f);
    }
    m->swing = 0.0f; m->throw_t = 0.0f; m->frozen = m->slowed = m->feared = 0.0f; m->stagger = 0.0f;
    snd_sfx_ex(SFX_SHRIEK, 0.8f, 0.5f); snd_sfx_ex(SFX_STINGER, 0.7f, 0.5f);
    snd_set_boss(G.boss + 1, 1);                                   /* turned up while the adds are out */
    G.kick = 0.05f;
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT boss_wave %d hp=%.0f/%.0f t=%.2f\n", G.bwave, m->hp, m->maxhp, G.t);
#endif
}

/* the rank its adds come at: the highest of each kind met since the last death, a rank more at the
 * challenge's second step and another at its fourth; never under what the level would bring */
static int add_rank(int type) {
    int r = pick_rank(type), top = type < REC_KINDS ? RUN.top_rank[type] - 1 : -1;
    if (top > r && top < RK_GILDED) r = top;
    r += (G.bstep >= 2) + (G.bstep >= 4);
    return clampi(r, 0, KINDS[type].top < RK_GILDED ? KINDS[type].top : RK_V);
}

/* a wave's adds, out of the gates, burrows or heaps in turn, or, in the Wraith's arena, out of the dark
 * well away from you; each comes with a puff of dust */
static void boss_adds(void) {
    const BossDef *B = &BOSSES[G.boss];
    const AddGroup *wave = B->waves[clampi(G.bwave - 1, 0, NWAVES - 1)];
    int n = 0, k = rnd(G.bnspawn > 0 ? G.bnspawn : 1);
    for (int g = 0; g < WAVE_GROUPS; g++)
        for (int i = 0; i < wave[g].n; i++) {
            int x = -1, y = -1;
            for (int tries = 0; tries < 40 && x < 0; tries++) {
                int sx, sy;
                if (G.bnspawn > 0) { sx = G.bspawn_x[k % G.bnspawn]; sy = G.bspawn_y[k % G.bnspawn]; k++; }
                else { sx = ARENA_X0 + rnd(MAPN - ARENA_X0 - 1); sy = BAND_Y0 + 1 + rnd(BAND_H - 3); if (hypotf(sx + 0.5f - G.px, sy + 0.5f - G.py) < 7.0f) continue; }
                for (int d = -1; d < 4 && x < 0; d++) {
                    int cx = sx + (d < 0 ? 0 : DX[d]), cy = sy + (d < 0 ? 0 : DY[d]);
                    if (in_arena((float)cx, (float)cy) && G.map[cy][cx].kind == K_FLOOR && !occupied(cx, cy, NULL)) { x = cx; y = cy; }
                }
            }
            if (x < 0) continue;
            Mob *m = spawn_mob(x, y, wave[g].type);
            if (!m) continue;
            set_rank(m, add_rank(wave[g].type));
            m->add = 1; m->alert = 30.0f; m->seen = 1; m->home = -1;
            if (wave[g].small) { m->small = 1; m->hp = m->maxhp = m->maxhp * 0.5f; }
            if (m->type == MOB_HUNTER) m->sub = 1;                 /* down and coming */
            if (m->type == MOB_SHAMBLER || m->type == MOB_DROWNED) { m->sub = 2; m->sub_t = 0.0f; m->deco = -1; }   /* up at once */
            for (int j = 0; j < MAXBOOMS; j++)
                if (!G.booms[j].alive) { memset(&G.booms[j], 0, sizeof G.booms[j]); G.booms[j].alive = 1; G.booms[j].x = m->x; G.booms[j].y = m->y; break; }
            n++;
        }
    G.badds = n;
    snd_sfx_ex(SFX_DOOR, 0.9f, 0.5f);
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT boss_adds %d wave %d t=%.2f\n", n, G.bwave, G.t);
#endif
}

static void boss_fall(Mob *m) {
    const BossDef *B = &BOSSES[G.boss];
    G.bstate = BS_DEAD; G.bstate_t = 0.0f;
    G.web_hp = 0.0f;                                               /* its web goes slack */
    Item *it = add_item(m->cx, m->cy, IT_SCRIP);
    if (it) { it->secret = 1; it->seen = 1; it->color = G.boss; }
    if (G.barch >= 0) G.doors[G.barch].locked = 0;
    for (int i = 0; i < MAXMOBS; i++) {                            /* what it called crumbles with it */
        Mob *o = &G.mobs[i];
        if (o != m && o->alive && o->state != MS_DIE && in_arena(o->x, o->y)) { o->state = MS_DIE; o->t = 0.0f; }
    }
    Record *r = &RUN.rec;
    int secs = clampi((int)(G.bfight_t + 0.5f), 1, 65535);
    if (r->boss[G.boss].slain < 65535) r->boss[G.boss].slain++;
    if (!r->boss[G.boss].best || secs < r->boss[G.boss].best) r->boss[G.boss].best = (uint16_t)secs;
    char d[32];
    if (!G.bchallenge) { snprintf(d, sizeof d, "SLAIN"); ach_earn(ACH_BOSS0 + G.boss * 6, SP_SCRIP, -1, 0, B->short_name, d); }
    else { snprintf(d, sizeof d, "SLAIN AT STEP %d", G.bstep); ach_earn(ACH_BOSS0 + G.boss * 6 + clampi(G.bstep, 1, 5), SP_SCRIP, -1, 0, B->short_name, d); }
    if (!G.bchallenge) RUN.runs_done++;                            /* the run is done */
    else if (RUN.bstep[G.boss] < 5) RUN.bstep[G.boss]++;           /* the next challenge a step harder */
    snd_sfx_ex(SFX_FANFARE, 0.9f, 0.5f); snd_sfx_ex(SFX_SHATTER, 1.0f, 0.5f);
    snd_boss_fall();
    say("THE WAY OUT IS OPEN");
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT boss_fall %s in %d s t=%.2f by %s\n", B->name, secs, G.t, G.player ? "player" : "autopilot");
#endif
}

/* a blow on the boss: nothing while it is untouchable; a mark stops it and calls the next wave */
static int boss_hurt(Mob *m, float dmg) {
    if (G.bstate != BS_FIGHT) { G.bring = 0.35f; sfx_at(SFX_KNOCK, m->x, m->y, 0.7f); return 0; }   /* a dull ring */
    float hp = m->hp - dmg;
    if (G.bwave < NWAVES && hp <= m->maxhp * WAVE_MARK[G.bwave]) { m->hp = m->maxhp * WAVE_MARK[G.bwave]; boss_wave(m); return 0; }
    m->hp = hp;
    if (m->hp > 0.0f) return 0;
    m->hp = 0.0f;
    kill_mob(m);
    boss_fall(m);
    return 1;
}

static void update_boss(float dt) {
    if (G.boss < 0 || PK.depth || G.shop || G.tally) return;
    if (G.bring > 0.0f) G.bring -= dt;
    if (G.bmirror_t > 0.0f) G.bmirror_t -= dt;
    if (G.bmirror_cd > 0.0f) G.bmirror_cd -= dt;
    G.bdash_age += dt;
    if (G.bpose_t > 0.0f && (G.bpose_t -= dt) <= 0.0f) {            /* a raised sword comes down */
        if (G.bpose == BP_RAISE) { G.bpose = BP_STRIKE; G.bpose_t = 0.35f; Mob *k = boss_mob(); if (k) sfx_at(SFX_SWORD, k->x, k->y, 1.5f); }
        else if (G.bpose == BP_LITANY) {                           /* the Litany ends: if he sees you, it strikes */
            Mob *k = boss_mob();
            G.bpose = BP_NONE;
            if (k && G.bstate == BS_FIGHT) {
                int seen = !G.dead;
                float lx = G.px - k->x, ly = G.py - k->y, ld = hypotf(lx, ly);
                for (float u = 0.0f; u < ld && seen; u += 0.1f) {  /* the timber props hide you; the pews don't */
                    int cx = (int)(k->x + lx / ld * u), cy = (int)(k->y + ly / ld * u);
                    if (blocks_light(cx, cy) || G.map[cy][cx].kind == K_PILLAR) seen = 0;
                }
                if (seen) { hurt_ex((float)G.max_hp * 0.2f, HURT_MAGIC, k->x, k->y); sfx_at(SFX_ZAP, G.px, G.py, 1.2f); DBG_COUNT(litany_hits); }
                else { sfx_at(SFX_WHISPER, k->x, k->y, 0.5f); DBG_COUNT(litany_missed); }
            }
        }
        else G.bpose = BP_NONE;
    }
    if (G.bcard > 0.0f) G.bcard += dt;
    G.bstate_t += dt;
    Mob *m = boss_mob();
    if (m && m->maxhp > 0.0f) G.bbar += (m->hp / m->maxhp - G.bbar) * fminf(1.0f, dt * 6.0f);
    if (G.bstate == BS_WAIT) {
        if (!G.dead && in_arena(G.px, G.py) && G.px >= ARENA_X0 + 0.6f) boss_begin();
        return;
    }
    if (G.bstate == BS_DEAD) return;
    G.bfight_t += dt;
#ifdef CRAWL_DEBUG
    if (m && G.bstate == BS_FIGHT && (int)((G.bfight_t - dt) / 10.0f) != (int)(G.bfight_t / 10.0f))   /* how the fight goes */
        fprintf(stderr, "BOSS fight t=%.1f hp=%.0f/%.0f ward=%.0f d=%.1f sight=%d target=%d aim=%d state=%d pose=%d swing=%.1f\n", G.t, m->hp, m->maxhp, G.bward,
                hypotf(m->x - G.px, m->y - G.py), line_clear(m->x, m->y, G.px, G.py), targetable(m), G.target == G.bmob, G.state, G.bpose, m->swing);
#endif
    boss_hazards(dt);
    if (G.boss == BOSS_WRAITH) {
        if (G.bslip_t > 0.0f) G.bslip_t -= dt;
        if (G.bbz_at < 0 && !G.player && !G.dead)                 /* the autopilot kindles what it stands by */
            for (int b = 0; b < G.bbz_n; b++)
                if (!G.bbz_lit[b] && hypotf(G.bbz_x[b] + 0.5f - G.px, G.bbz_y[b] + 0.5f - G.py) < 2.0f) { G.bbz_at = b; G.bbz_t = 1.0f; break; }
        if (G.bbz_at >= 0) {
            int b = G.bbz_at;
            if (G.dead || G.bbz_lit[b] || hypotf(G.bbz_x[b] + 0.5f - G.px, G.bbz_y[b] + 0.5f - G.py) > 2.3f) G.bbz_at = -1;   /* walked off */
            else if ((G.bbz_t -= dt) <= 0.0f) { brazier_set(b, 1); G.bbz_at = -1; }
        }
        for (int b = 0; b < G.bbz_n; b++) {
            float bx = G.bbz_x[b] + 0.5f, by = G.bbz_y[b] + 0.5f;
            if (!G.bbz_lit[b])                                     /* a wisp close by kindles it at once */
                for (int i = 0; i < MAXWISPS; i++)
                    if (G.wisps[i].on && G.wisps[i].a > 0.5f && hypotf(G.wisps[i].x - bx, G.wisps[i].y - by) < 1.5f) brazier_set(b, 1);
            if (G.bbz_lit[b])                                      /* the void puts it out */
                for (int i = 0; i < MAXHAZ; i++)
                    if (G.haz[i].alive && G.haz[i].kind == HZ_VOID && hz_covers(&G.haz[i], G.bbz_x[b], G.bbz_y[b]) == 2) { brazier_set(b, 0); break; }
        }
    }
    if (G.boss == BOSS_DEACON && m && G.bward <= 0.0f && (G.bward_cd -= dt) <= 0.0f) {   /* his Bone Ward grows back */
        G.bward = 0.04f * m->maxhp; G.bward_cd = 0.0f;
        sfx_at(SFX_RUNE, m->x, m->y, 1.0f);
        DBG_COUNT(ward_grown);
    }
    if (G.boss == BOSS_SPIDER && G.bnspawn > 0 && (G.btrickle_t += dt) >= 15.0f) {   /* every fifteen seconds, one more out of a burrow */
        int out = 0;
        G.btrickle_t = 0.0f;
        for (int i = 0; i < MAXMOBS; i++) out += G.mobs[i].alive && G.mobs[i].add == 2 && G.mobs[i].state != MS_DIE;
        int k = rnd(G.bnspawn), x = G.bspawn_x[k], y = G.bspawn_y[k];
        for (int d = 0; d < 4 && out < 4; d++) {
            int cx = x + DX[d], cy = y + DY[d];
            if (!in_arena((float)cx, (float)cy) || G.map[cy][cx].kind != K_FLOOR || occupied(cx, cy, NULL)) continue;
            Mob *a = spawn_mob(cx, cy, MOB_HUNTER);
            if (a) { set_rank(a, add_rank(MOB_HUNTER)); a->add = 2; a->small = 1; a->hp = a->maxhp = a->maxhp * 0.5f; a->sub = 2; a->alert = 30.0f; a->seen = 1; a->home = -1; }
            break;
        }
    }
    for (int i = 0; i < MAXMOBS; i++) if (G.mobs[i].alive && (G.mobs[i].boss || G.mobs[i].add)) G.mobs[i].alert = fmaxf(G.mobs[i].alert, 20.0f);
    if (G.bstate == BS_WAVE) {
        if (G.badds < 0) { if (G.bstate_t >= 2.0f) boss_adds(); return; }
        int left = 0;
        for (int i = 0; i < MAXMOBS; i++) left += G.mobs[i].alive && G.mobs[i].add == 1 && G.mobs[i].state != MS_DIE;
        G.badds = left;
#ifdef CRAWL_DEBUG
        if (left && G.bstate_t > 20.0f && (int)((G.bstate_t - dt) / 10.0f) != (int)(G.bstate_t / 10.0f)) {   /* adds that linger */
            fprintf(stderr, "BOSS adds left %d t=%.1f:", left, G.t);
            for (int i = 0; i < MAXMOBS; i++) {
                const Mob *o = &G.mobs[i];
                if (o->alive && o->add == 1 && o->state != MS_DIE)
                    fprintf(stderr, " %s@%.1f,%.1f sub%d alert%.0f hunts%d seen%d", KINDS[o->type].name, o->x, o->y, o->sub, o->alert, o->hunts, o->seen);
            }
            fprintf(stderr, " caster@%.1f,%.1f\n", G.px, G.py);
        }
#endif
        if (left) return;
        G.bstate = BS_RISE; G.bstate_t = 0.0f;
        snd_set_boss(G.boss + 1, 0);                               /* and settling back */
        if (G.bwave == 1) font_fill(1);                            /* a font fills after the first wave, and the third */
        if (G.bwave == 3) font_fill(2);
        if (m) sfx_at(SFX_GROAN, m->x, m->y, 1.0f);
#ifdef CRAWL_DEBUG
        fprintf(stderr, "EVENT boss_rise wave %d t=%.2f\n", G.bwave, G.t);
#endif
    } else if (G.bstate == BS_RISE && G.bstate_t >= 1.2f) { G.bstate = BS_FIGHT; G.bstate_t = 0.0f; }
}

/* Abilities that run by themselves. Treasure Hunter: an eerie whisper from an unopened secret wall
 * within three cells, louder as you near it. Commune: every 15 s, the creatures within its reach show
 * as red shapes through the walls for a moment (render_commune). */
static int summon_wisp(int kind, int color, float cost);
static void wisp_out(Wisp *w);
/* The Black Altar (docs/PLAN-b6.md): gold once every secret is found, every link of every chain. Its
 * wisp goes to one secret at a time and waits at its door, the cell before the cracked wall, while you
 * are inside; once you have been in and come out again, E at the wisp sends it home, and at the altar
 * it can be sent on to the next for ALTAR_SOULS and five more for every secret found on the level. */
static int nearest_shut(float x, float y) {
    int best = -1; float bd = 1e9f;
    for (int i = 0; i < G.nsecrets; i++) {
        if (G.secrets[i].state) continue;
        float d = hypotf(G.secrets[i].fx + 0.5f - x, G.secrets[i].fy + 0.5f - y);
        if (d < bd) { bd = d; best = i; }
    }
    return best;
}
static int in_secret(int i) {                                      /* standing in that secret's room */
    const Secret *sc = &G.secrets[i];
    return G.cx >= sc->x0 && G.cx <= sc->x1 && G.cy >= sc->y0 && G.cy <= sc->y1;
}
static int altar_price(void) { return ALTAR_SOULS + 5 * G.links_found; }
static int altar_callable(void) {                                  /* been in, and out again: it may be sent home */
    return G.altar_wisp == AW_SEEK && G.altar_seek >= 0 && G.secrets[G.altar_seek].state && G.altar_inside && !in_secret(G.altar_seek);
}
static void altar_call_home(void) {
    G.altar_wisp = AW_HOME; G.wisps[1].retarget = 0.0f;
    sfx_at(SFX_SHIMMER, G.wisps[1].x, G.wisps[1].y, 0.8f);
    if (G.player) say("THE WISP GOES HOME TO ITS ALTAR");
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT altar wisp home t=%.2f\n", G.t);
#endif
}
static void update_altar(void) {
    if (G.altar_x < 0 || G.altar_state >= ALTAR_READY) return;
    if (G.links_found >= G.nlinks) {
        G.altar_state = ALTAR_READY; G.altar_seek = -1;
        if (G.wisps[1].on && G.wisps[1].state != WS_GONE) wisp_out(&G.wisps[1]);   /* its work done */
        G.altar_wisp = AW_NONE;
        sfx_at(SFX_SHIMMER, G.altar_x + 0.5f, G.altar_y + 0.5f, 1.2f);
        if (G.player) say("THE BLACK ALTAR BURNS GOLD");
        return;
    }
    if (G.altar_wisp == AW_SEEK && G.altar_seek >= 0) {
        if (in_secret(G.altar_seek)) G.altar_inside = 1;
        if (!G.player && G.secrets[G.altar_seek].state && !in_secret(G.altar_seek)   /* the autopilot calls it home as it goes */
            && hypotf(G.wisps[1].x - G.px, G.wisps[1].y - G.py) < 4.0f) altar_call_home();
    }
    if (G.altar_wisp == AW_HOME && G.wisps[1].on && G.wisps[1].state == WS_HOVER) G.altar_wisp = AW_REST;
    if ((G.altar_wisp == AW_HOME || G.altar_wisp == AW_REST) && !G.wisps[1].on) summon_wisp(WK_ALTAR, 0, 0.0f);   /* lost: it comes again */
}

/* The altar's three gifts, chosen from its menu (E) or by the autopilot: its wisp for ten souls, its
 * blessing on your exit wisps for twenty-five, and when it burns gold, an anointing */
static int altar_wake(void) {
    if (G.altar_state != ALTAR_DORMANT) return 0;
    if (!spend_spirit((float)ALTAR_SOULS)) { if (G.player) say("THE ALTAR WANTS %d SOULS", ALTAR_SOULS); snd_sfx_ex(SFX_KNOCK, 0.6f, 0.5f); return 0; }
    G.altar_state = ALTAR_AWAKE; G.altar_seek = nearest_shut(G.px, G.py); G.altar_wisp = AW_SEEK; G.altar_inside = 0;
    if (G.player) say("THE ALTAR'S WISP SEEKS WHAT IS HIDDEN");
    snd_sfx_ex(SFX_WHISPER, 0.9f, 0.5f); sfx_at(SFX_SHIMMER, G.altar_x + 0.5f, G.altar_y + 0.5f, 1.0f);
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT altar wakes t=%.2f\n", G.t);
#endif
    summon_wisp(WK_ALTAR, 0, 0.0f);                                /* the altar's own, free */
    return 1;
}
static int altar_send(void) {                                      /* its wisp, resting, sent on to the next secret */
    if (G.altar_state != ALTAR_AWAKE || G.altar_wisp != AW_REST || nearest_shut(G.px, G.py) < 0) return 0;
    if (!spend_spirit((float)altar_price())) { if (G.player) say("SENDING IT ON WANTS %d SOULS", altar_price()); snd_sfx_ex(SFX_KNOCK, 0.6f, 0.5f); return 0; }
    G.altar_seek = nearest_shut(G.px, G.py); G.altar_wisp = AW_SEEK; G.altar_inside = 0; G.wisps[1].retarget = 0.0f;
    if (G.player) say("THE WISP GOES TO THE NEXT SECRET");
    snd_sfx_ex(SFX_WHISPER, 0.9f, 0.5f); sfx_at(SFX_SHIMMER, G.altar_x + 0.5f, G.altar_y + 0.5f, 1.0f);
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT altar sends price=%d t=%.2f\n", altar_price(), G.t);
#endif
    return 1;
}
static int altar_bless(void) {
    if (G.blessed) return 0;
    if (!spend_spirit((float)ALTAR_BLESS)) { if (G.player) say("THE BLESSING WANTS %d SOULS", ALTAR_BLESS); snd_sfx_ex(SFX_KNOCK, 0.6f, 0.5f); return 0; }
    G.blessed = 1;
    if (G.player) say("YOUR EXIT WISPS WILL TURN THE DEAD");
    snd_sfx_ex(SFX_RUNE, 0.8f, 0.5f); snd_sfx_ex(SFX_MOAN, 0.7f, 0.5f); sfx_at(SFX_SHIMMER, G.altar_x + 0.5f, G.altar_y + 0.5f, 1.2f);
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT altar blesses t=%.2f\n", G.t);
#endif
    return 1;
}
static int altar_anoint(void) {
    if (G.altar_state != ALTAR_READY) return 0;
    int can[NSLOT], n = 0, worn = 0;                               /* one worn piece, at random, not anointed yet */
    for (int s = 0; s < NSLOT; s++) if (RUN.gear.worn[s].kind) { worn++; if (!RUN.gear.worn[s].anointed) can[n++] = s; }
    if (!n) { if (G.player) say(worn ? "ALL YOU WEAR IS ANOINTED ALREADY" : "YOU WEAR NOTHING FOR IT TO BLESS"); return 0; }
    Piece *p = &RUN.gear.worn[can[rnd(n)]];
    piece_anoint(p);
    G.altar_state = ALTAR_SPENT;
    G.max_hp = gear_max_hp(&RUN.gear);
    char a[48], b[48];
    piece_name(p, a, b, sizeof a);
    say("ANOINTED: %s", b);
    snd_sfx_ex(SFX_RUNE, 1.0f, 0.5f); snd_sfx_ex(SFX_SHIMMER, 0.9f, 0.5f); G.grin_t = 2.0f;
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT altar anoints %s %s t=%.2f\n", a, b, G.t);
#endif
    return 1;
}

static void update_abilities(float dt) {
    const Gear *g = &RUN.gear;
    if (gear_ability(g, AB_TREASURE, 0) > 0.0f && (G.whisper_t -= dt) <= 0.0f) {
        G.whisper_t = 2.4f;
        float best = 3.5f; int bi = -1;
        for (int i = 0; i < G.nsecrets; i++) {
            if (G.secrets[i].state) continue;
            float d = hypotf(G.secrets[i].sx + 0.5f - G.px, G.secrets[i].sy + 0.5f - G.py);
            if (d < best) { best = d; bi = i; }
        }
        if (bi >= 0) {
            sfx_at(SFX_WHISPER, G.secrets[bi].sx + 0.5f, G.secrets[bi].sy + 0.5f, 0.35f + 0.9f * (1.0f - best / 3.5f));
            G.whisper_t = 1.6f + best * 0.5f;                       /* quicker, the nearer */
        }
    }
    float reach = gear_ability(g, AB_COMMUNE, 0);
    G.commune_show = fmaxf(0.0f, G.commune_show - dt);
    if (reach > 0.0f && (G.commune_t += dt) >= 15.0f) {
        G.commune_t = 0.0f;
        G.commune_show = G.commune_len = gear_ability(g, AB_COMMUNE, 1) / 10.0f;
        G.commune_r = reach;
        snd_sfx_ex(SFX_HEART, 0.7f, 0.5f);
#ifdef CRAWL_DEBUG
        fprintf(stderr, "EVENT commune t=%.2f r=%.0f for %.1f\n", G.t, reach, G.commune_len);
#endif
    }
}

static void update_items(void) {
    for (int i = 0; i < G.nitems; i++) {
        Item *it = &G.items[i];
        if (!it->alive) continue;
        float d = hypotf(it->x - G.px, it->y - G.py);
        if (!it->seen && d < (it->type == IT_KEY ? 13.0f : 8.5f) && line_clear(G.px, G.py, it->x, it->y)) it->seen = 1;
        if (it->type == IT_BONE && RUN.gear.bones >= BONES_MAX) continue;   /* no room for it */
        if (d < (G.player ? 0.5f : 1.5f) && G.level_t > 1.0 && !G.ending) collect(it);   /* the autopilot reaches out */
    }
}

/* the tally counts up row by row: time, kills, treasure, souls, secrets */
#define TALLY_ROWS 6
static const float TALLY_ROW0 = 1.6f, TALLY_ROW_GAP = 1.0f, TALLY_COUNT = 0.75f, TALLY_END = 9.0f;
static int tally_rows(void) { return G.boss >= 0 && G.bstate == BS_DEAD ? 6 : 5; }   /* a boss's level: how long its fight took */
static int tally_value(int row, float t, int final) {
    float k = smoothstepf(0.0f, 1.0f, (t - TALLY_ROW0 - row * TALLY_ROW_GAP) / TALLY_COUNT);
    return (int)floorf(final * k);
}
static void update_tally(double dt) {
    float pt = (float)G.tally_t;
    G.tally_t += dt;
    float t = (float)G.tally_t;
    int finals[TALLY_ROWS] = { (int)G.clear_time, G.kills, G.items_got, G.level_souls, G.links_found, (int)G.bfight_t };
    for (int r = 0; r < tally_rows(); r++) {
        int v = tally_value(r, t, finals[r]);
        if (v != G.tally_shown[r]) { G.tally_shown[r] = v; snd_sfx_ex(SFX_TICK, 0.7f, 0.5f); }
        float done = TALLY_ROW0 + r * TALLY_ROW_GAP + TALLY_COUNT;
        if (pt < done && t >= done) snd_sfx_ex(r == 2 ? SFX_COIN : (r == 4 && G.nlinks && G.links_found == G.nlinks ? SFX_SECRET : SFX_ENTER), 1.0f, 0.5f);
    }
    float perfect = TALLY_ROW0 + tally_rows() * TALLY_ROW_GAP + 0.2f;
    if (pt < perfect && t >= perfect && G.items_total > 0 && G.items_got == G.items_total) snd_sfx_ex(SFX_KEYGET, 1.0f, 0.5f);
    if (t >= TALLY_END) { G.level++; RUN.level = G.level; RUN.hp = G.hp; begin_shop(); }
}

static void update_shop(double dt);

static void update_ui(void);
static void open_ui(int which);
static void update_human(float dt);
/* how far up the hand is: a cast, or a rune shown off */
static float hand_raise(void) {
    float lift = G.rune_lift > 0.0f ? 0.8f * smoothstepf(1.4f, 1.25f, G.rune_lift) * smoothstepf(0.0f, 0.3f, G.rune_lift) : 0.0f;
    return fmaxf(G.hand, lift);
}
/* the hand rises from the bottom right, reaching up and over, Catacomb 3-D / Doom style */
static void hand_origin(float hand, float *ox, float *oy) {
    float walk = G.player ? clampf(G.spd / WALK, 0.0f, 1.0f) : 0.0f;
    *ox = 170.0f + (int)(2.0f * sinf((float)G.t * 3.0f) + 5.0f * walk * sinf(G.bobph * 0.5f));
    *oy = H - (int)(hand * 130.0f) + (int)(2.0f * sinf((float)G.t * 6.0f) + 5.0f * walk * fabsf(sinf(G.bobph * 0.5f))) - (G.shop || G.player ? 40 : 0);
}

/* a key still lying in the level, or -1 */
static int key_lying(int color) {
    for (int i = 0; i < G.nitems; i++) if (G.items[i].alive && G.items[i].type == IT_KEY && G.items[i].color == color) return i;
    return -1;
}

/* a goal in a locked vault: first the key to it (and to the vault that key lies in) */
static void past_vaults(int *gx, int *gy, int k) {
    for (int depth = 0; depth < 4 && k >= 0 && G.vault_door[k] >= 0 && G.doors[G.vault_door[k]].locked == 1 && !G.has_key[k]; depth++) {
        int found = -1;
        for (int i = 0; i < G.nitems && found < 0; i++) if (G.items[i].alive && G.items[i].type == IT_KEY && G.items[i].color == k) found = i;
        if (found < 0) return;
        *gx = G.items[found].cx; *gy = G.items[found].cy;
        int v = vault_of_room(*gx, *gy);
        k = v != k ? v : -1;
    }
}

/* room for a body to walk straight there: past no wall, door, pillar or stone, nor a corner of one */
static int walk_clear(float x0, float y0, float x1, float y1) {
    float dx = x1 - x0, dy = y1 - y0, d = sqrtf(dx * dx + dy * dy);
    if (d < 0.01f) return 1;
    float nx = -dy / d * 0.3f, ny = dx / d * 0.3f;
    int n = (int)(d / 0.1f);
    for (int side = -1; side <= 1; side++)
        for (int i = 1; i <= n; i++) {
            float t = (float)i / n;
            Cell *c = cell((int)(x0 + dx * t + nx * side), (int)(y0 + dy * t + ny * side));
            if (c->kind == K_WALL || c->kind == K_PILLAR || c->kind == K_PROP || (c->kind == K_DOOR && G.doors[c->door].open < 0.7f)) return 0;
        }
    return 1;
}

static const char *const KEY_COLOR[NKEYS];
/* Will-o'-wisps (PLAN-b5). Summoned for a soul, or woken from a Black Altar, a wisp goes on ahead along
 * the way to what it seeks, in that thing's colour, and hovers there till you come. It leaves a trail of
 * motes hanging in the air, to follow round a corner. */
static const int KEY_RAMP[NKEYS] = { R_GOLD, R_BONE, R_BLOOD };
static int wisp_ramp_of(const Wisp *w) { return w->kind == WK_ALTAR ? R_PURPLE : (w->kind == WK_KEY ? KEY_RAMP[w->color] : R_GLOW); }

/* what a wisp of this kind seeks now: the cell, where it hovers there, and whether that is the end of
 * it (or a key it must fetch first); 0 if there is nothing left to seek */
static int wisp_target(int kind, int color, int *gx, int *gy, float *hx, float *hy, int *final) {
    int x, y;
    float px, py;
    if (kind == WK_EXIT && PK.depth) {                             /* in a pocket, the way out is the way back */
        int d = 0;
        for (; d < 4; d++) if (cell(PK.back_x + DX[d], PK.back_y + DY[d])->kind == K_FLOOR) break;
        if (d == 4) return 0;
        x = PK.back_x + DX[d]; y = PK.back_y + DY[d];
        px = x + 0.5f - DX[d] * 0.3f; py = y + 0.5f - DY[d] * 0.3f;   /* before the doorway */
        *gx = x; *gy = y;
    } else if (kind == WK_EXIT) {
        if (G.exit_fx < 0) return 0;
        x = G.exit_fx; y = G.exit_fy;
        px = x + 0.5f + DX[G.exit_dir] * 0.3f; py = y + 0.5f + DY[G.exit_dir] * 0.3f;   /* before the door */
        *gx = x; *gy = y; past_vaults(gx, gy, G.exit_in_vault);
    } else if (kind == WK_KEY) {
        int i = key_lying(color);
        if (i < 0) return 0;
        x = G.items[i].cx; y = G.items[i].cy; px = G.items[i].x; py = G.items[i].y;   /* over the key */
        *gx = x; *gy = y; past_vaults(gx, gy, vault_of_room(x, y));
    } else if (G.altar_wisp == AW_HOME || G.altar_wisp == AW_REST) {   /* home: over the altar */
        if (G.altar_x < 0) return 0;
        int d = 0;
        for (; d < 4; d++) if (is_open(G.altar_x + DX[d], G.altar_y + DY[d])) break;
        if (d == 4) return 0;
        x = G.altar_x + DX[d]; y = G.altar_y + DY[d];
        px = x + 0.5f - DX[d] * 0.45f; py = y + 0.5f - DY[d] * 0.45f;   /* at the altar's edge, over its fire */
        *gx = x; *gy = y; past_vaults(gx, gy, vault_of_room(x, y));
    } else {
        if (G.altar_state != ALTAR_AWAKE || G.altar_seek < 0) return 0;
        const Secret *sc = &G.secrets[G.altar_seek];
        x = sc->fx; y = sc->fy;
        px = x + 0.5f + DX[sc->d] * 0.3f; py = y + 0.5f + DY[sc->d] * 0.3f;   /* at its door, till you have been in and out */
        *gx = x; *gy = y; past_vaults(gx, gy, vault_of_room(x, y));
    }
    *final = *gx == x && *gy == y;
    *hx = *final ? px : *gx + 0.5f; *hy = *final ? py : *gy + 0.5f;
    return 1;
}

/* the way to it: open floor and doors, but not through a vault you have no key for */
static void wisp_route(Wisp *w) {
    static int q[MAPN * MAPN];
    memset(w->field, 0xff, sizeof w->field);
    if (w->gx < 0) return;
    int qh = 0, qt = 0;
    w->field[w->gy][w->gx] = 0; q[qt++] = w->gy * MAPN + w->gx;
    while (qh < qt) {
        int x = q[qh] % MAPN, y = q[qh] / MAPN; qh++;
        for (int d = 0; d < 4; d++) {
            int nx = x + DX[d], ny = y + DY[d];
            if (nx < 0 || ny < 0 || nx >= MAPN || ny >= MAPN || w->field[ny][nx] >= 0 || !is_open(nx, ny)) continue;
            const Cell *c = &G.map[ny][nx];
            if (c->kind == K_DOOR && ((G.doors[c->door].locked == 1 && !G.has_key[G.doors[c->door].color]) || G.doors[c->door].locked == SEALED)) continue;
            w->field[ny][nx] = (int16_t)(w->field[y][x] + 1); q[qt++] = ny * MAPN + nx;
        }
    }
}

/* a wisp's line of flight: no wall or pillar in the way, nor a corner of one (doors it goes through) */
static int wisp_line(float x0, float y0, float x1, float y1) {
    float dx = x1 - x0, dy = y1 - y0, d = sqrtf(dx * dx + dy * dy);
    if (d < 0.01f) return 1;
    float nx = -dy / d * 0.15f, ny = dx / d * 0.15f;
    int n = (int)(d / 0.1f) + 1;
    for (int side = -1; side <= 1; side++)
        for (int i = 1; i <= n; i++) {
            float t = (float)i / n;
            int k = cell((int)(x0 + dx * t + nx * side), (int)(y0 + dy * t + ny * side))->kind;
            if (k == K_WALL || k == K_PILLAR) return 0;
        }
    return 1;
}

/* a wisp of this kind sets out: from the hand, or from the altar; a soul (cost) unless it is free */
static int summon_wisp(int kind, int color, float cost) {
    int gx, gy, fin;
    float hx, hy;
    if (!wisp_target(kind, color, &gx, &gy, &hx, &hy, &fin)) {
        say(kind == WK_KEY ? "THAT KEY LIES NOWHERE NOW" : "THERE IS NOTHING LEFT FOR IT TO FIND");
        return 0;
    }
    if (cost > 0.0f && !spend_spirit(cost)) { say("A WISP COSTS A SOUL"); snd_sfx_ex(SFX_KNOCK, 0.6f, 0.5f); return 0; }
    Wisp *w = &G.wisps[kind == WK_ALTAR ? 1 : 0];                  /* casting another sends the first out */
    memset(w, 0, sizeof *w);
    w->on = 1; w->kind = kind; w->color = color; w->state = WS_KINDLE; w->gx = -1;
    if (kind == WK_ALTAR) { w->x = G.altar_x + 0.5f; w->y = G.altar_y + 0.5f; w->z = 0.7f; }
    else { w->x = G.px + cosf(G.ang) * 0.3f; w->y = G.py + sinf(G.ang) * 0.3f; w->z = 0.4f; }
    if (!is_open((int)w->x, (int)w->y) && kind != WK_ALTAR) { w->x = G.px; w->y = G.py; }
    snd_sfx_ex(SFX_SHIMMER, 0.7f, 0.5f);
    if (kind == WK_EXIT) say("A WISP KINDLES. IT KNOWS THE WAY OUT");
    else if (kind == WK_KEY) say("A WISP GOES FOR THE %s KEY", KEY_COLOR[color]);
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT wisp %d %d t=%.2f\n", kind, color, G.t);
#endif
    return 1;
}

/* the L menu: what can be cast now, and what each costs */
typedef struct { int kind, color; float cost; } WispChoice;
#define WISP_CHOICES (2 + NKEYS)
static int wisp_choices(WispChoice *c) {
    int n = 0;
    c[n++] = (WispChoice){ WK_EXIT, 0, WISP_COST };
    if (gear_ability(&RUN.gear, AB_KEYMAKER, 0) > 0.0f)            /* Keymaker: a wisp to each key still lying */
        for (int k = 0; k < NKEYS; k++) if (key_lying(k) >= 0) c[n++] = (WispChoice){ WK_KEY, k, WISP_COST };
    if (G.altar_state == ALTAR_AWAKE && G.altar_wisp == AW_SEEK && G.altar_seek >= 0) c[n++] = (WispChoice){ WK_ALTAR, 0, 0.0f };   /* the altar's, back */
    return n;
}
static int wisp_choice_at(const WispChoice *c, int n, int kind, int color) {
    for (int i = 0; i < n; i++) if (c[i].kind == kind && (kind != WK_KEY || c[i].color == color)) return i;
    return -1;
}
static void cast_choice(const WispChoice *c) {
    if (summon_wisp(c->kind, c->color, c->cost)) { RUN.wisp_kind = c->kind; RUN.wisp_color = c->color; }
}

/* the altar's menu: its three gifts, what each costs, and why one can't be had */
typedef struct { const char *what, *cost, *note; int ok, price; } AltarRow;
static void altar_rows(AltarRow *r) {
    static char found[40];
    static char price[24];
    if (G.altar_state == ALTAR_DORMANT) r[0] = (AltarRow){ "WAKE ITS WISP", "10 SOULS", "IT LEADS YOU TO A SECRET", 1, ALTAR_SOULS };
    else if (G.altar_state == ALTAR_AWAKE && G.altar_wisp == AW_REST && nearest_shut(G.px, G.py) >= 0) {
        snprintf(price, sizeof price, "%d SOULS", altar_price());
        r[0] = (AltarRow){ "SEND ITS WISP ON", price, "TO THE NEAREST SECRET STILL SHUT", 1, altar_price() };
    }
    else if (G.altar_state == ALTAR_AWAKE && G.altar_wisp == AW_REST) r[0] = (AltarRow){ "ITS WISP RESTS", "", "NO SECRET WALL IS LEFT SHUT", 0, 0 };
    else if (G.altar_state == ALTAR_AWAKE) r[0] = (AltarRow){ "ITS WISP IS OUT", "", "IN, OUT, THEN E SENDS IT HOME", 0, 0 };
    else r[0] = (AltarRow){ "ITS WISP HAS DONE ITS WORK", "", "EVERY SECRET HERE IS FOUND", 0, 0 };
    r[1] = G.blessed ? (AltarRow){ "YOUR EXIT WISPS ARE BLESSED", "", "UNTIL YOU LEAVE THIS LEVEL", 0, 0 }
                     : (AltarRow){ "BLESS YOUR EXIT WISPS", "25 SOULS", "THEY TURN THE UNDEAD, THIS LEVEL", 1, ALTAR_BLESS };
    if (G.altar_state == ALTAR_READY) r[2] = (AltarRow){ "ANOINT WHAT YOU WEAR", "FREE", "ONE WORN PIECE GROWS STRONGER", 1, 0 };
    else if (G.altar_state == ALTAR_SPENT) r[2] = (AltarRow){ "IT HAS ANOINTED ALREADY", "", "IT HAS GIVEN WHAT IT HAD", 0, 0 };
    else {
        snprintf(found, sizeof found, "FIND EVERY SECRET: %d OF %d SO FAR", G.links_found, G.nlinks);
        r[2] = (AltarRow){ "ANOINT WHAT YOU WEAR", "", found, 0, 0 };
    }
}

static void wisp_out(Wisp *w) { w->state = WS_GONE; w->flare = 0.0f; sfx_at(SFX_SHIMMER, w->x, w->y, 0.6f); }

/* a blessed exit wisp: the undead about it flee from it, and go on fleeing for three seconds after it
 * has gone by; the Wraith is driven back */
static void turn_undead(const Wisp *w) {
    for (int i = 0; i < MAXMOBS; i++) {
        Mob *m = &G.mobs[i];
        if (!m->alive || m->state != MS_WALK || !(KIND_IS(m, KF_UNDEAD) || m->type == MOB_WSKULL || m->type == MOB_WRAITH)) continue;
        if (hypotf(m->x - w->x, m->y - w->y) > BLESS_REACH || !line_clear(w->x, w->y, m->x, m->y)) continue;
        if (m->type == MOB_WRAITH) { if (m->sub != 1) wraith_back(m); continue; }
        if (m->feared <= 0.0f || !m->wfeared) { sfx_at(SFX_MOAN, m->x, m->y, 0.6f); DBG_COUNT(turned); }
        m->feared = fmaxf(m->feared, 3.0f); m->wfeared = 1; m->wfx = w->x; m->wfy = w->y;
    }
}

static void update_wisps(float dt) {
    for (int i = 0; i < MAXWISPS; i++) {
        Wisp *w = &G.wisps[i];
        if (!w->on || dt <= 0.0f) continue;
        w->t += dt;
        if (w->state == WS_GONE) {                                 /* it flares and goes out; its trail lingers a while */
            w->flare += dt;
            w->a = w->flare < 0.15f ? 1.0f + w->flare * 4.0f : fmaxf(0.0f, 1.6f * (1.0f - (w->flare - 0.15f) / 0.6f));
            if (w->flare > 0.75f + WISP_MOTE_T) w->on = 0;
            continue;
        }
        if ((w->retarget -= dt) <= 0.0f) {                         /* a key taken, a secret opened: where now */
            int gx, gy, fin;
            float hx, hy;
            w->retarget = 0.35f;
            if (!wisp_target(w->kind, w->color, &gx, &gy, &hx, &hy, &fin)) { wisp_out(w); continue; }
            if (gx != w->gx || gy != w->gy) { w->gx = gx; w->gy = gy; wisp_route(w); if (w->state == WS_HOVER) w->state = WS_FLY; }
            w->hx = hx; w->hy = hy; w->final = fin;
        }
        float ox = w->x, oy = w->y;
        if (w->state == WS_KINDLE) {                               /* a spark that grows, then goes */
            w->a = fminf(1.0f, w->a + dt / 0.45f);
            w->z += dt * 0.2f;
            if (w->a >= 1.0f) w->state = WS_FLY;
        } else if (w->state == WS_FLY) {
            int cx = (int)w->x, cy = (int)w->y;
            int f = cx >= 0 && cy >= 0 && cx < MAPN && cy < MAPN ? w->field[cy][cx] : -1;
            float tx = w->hx, ty = w->hy, pace = WISP_PACE;
            if (f > 0) {                                           /* down the way, as far along it as it can see */
                int x = cx, y = cy;
                for (int k = 0; k < 3 && w->field[y][x] > 0; k++) {
                    int bx = -1, by = -1;
                    float best = 1e9f;
                    for (int d = 0; d < 4; d++) {
                        int nx = x + DX[d], ny = y + DY[d];
                        if (nx < 0 || ny < 0 || nx >= MAPN || ny >= MAPN || w->field[ny][nx] != w->field[y][x] - 1) continue;
                        float e = hypotf((float)(nx - w->gx), (float)(ny - w->gy));   /* of equal ways, the straightest */
                        if (e < best) { best = e; bx = nx; by = ny; }
                    }
                    if (bx < 0 || (k > 0 && !wisp_line(w->x, w->y, bx + 0.5f, by + 0.5f))) break;
                    x = bx; y = by;
                }
                if (w->field[y][x] > 0) { tx = x + 0.5f; ty = y + 0.5f; }
            } else if (f < 0 && cx >= 0 && cy >= 0 && cx < MAPN && cy < MAPN) {   /* strayed off its way (over a prop): back onto it */
                int best = -1;
                for (int d = 0; d < 4; d++) {
                    int nx = cx + DX[d], ny = cy + DY[d];
                    if (nx < 0 || ny < 0 || nx >= MAPN || ny >= MAPN || w->field[ny][nx] < 0) continue;
                    if (best < 0 || w->field[ny][nx] < w->field[cy + DY[best]][cx + DX[best]]) best = d;
                }
                if (best >= 0) { tx = cx + DX[best] + 0.5f; ty = cy + DY[best] + 0.5f; }
            }
            if (f >= 0 && f < 2) pace *= 0.45f + 0.55f * fminf(1.0f, hypotf(w->hx - w->x, w->hy - w->y));   /* slowing as it comes there */
            float dx = tx - w->x, dy = ty - w->y, d = hypotf(dx, dy);
            if (f == 0 && d < 0.05f) w->state = WS_HOVER;
            else if (d > 1e-4f) { float st = fminf(d, pace * dt); w->x += dx / d * st; w->y += dy / d * st; }
            w->z += (0.5f + 0.05f * sinf(w->t * 2.1f) - w->z) * fminf(1.0f, dt * 3.0f);
        } else {                                                   /* there: it hovers, bobbing, till you come */
            w->x += (w->hx - w->x) * fminf(1.0f, dt * 4.0f); w->y += (w->hy - w->y) * fminf(1.0f, dt * 4.0f);
            w->z += (0.55f + 0.08f * sinf(w->t * 2.6f) - w->z) * fminf(1.0f, dt * 3.0f);
            if (w->final && w->kind != WK_ALTAR && hypotf(G.px - w->hx, G.py - w->hy) < 1.5f) { wisp_out(w); continue; }
        }
        w->vx = (w->x - ox) / dt; w->vy = (w->y - oy) / dt;
        if (w->kind == WK_EXIT && G.blessed && w->a > 0.5f) turn_undead(w);
        if ((w->mote_d += hypotf(w->x - ox, w->y - oy)) >= 0.22f) {   /* a mote of it left hanging */
            Mote *m = &w->motes[w->mote_head];
            w->mote_head = (w->mote_head + 1) % WISP_MOTES;
            m->x = w->x; m->y = w->y; m->z = w->z; m->t = (float)G.t;
            w->mote_d = 0.0f;
        }
    }
}

/* the way a wisp went, from where the caster stands, as far down it as there is a clear walk */
static int wisp_lean(const Wisp *w, float *out) {
    int x = (int)G.px, y = (int)G.py;
    if (!w->on || w->gx < 0 || x < 0 || y < 0 || x >= MAPN || y >= MAPN) return 0;
    float tx = w->hx, ty = w->hy;
    if (w->field[y][x] < 0) { *out = atan2f(ty - G.py, tx - G.px); return 1; }
    for (int k = 0; k < 7 && w->field[y][x] > 0; k++) {
        int bx = -1, by = -1;
        float best = 1e9f;
        for (int d = 0; d < 4; d++) {
            int nx = x + DX[d], ny = y + DY[d];
            if (nx < 0 || ny < 0 || nx >= MAPN || ny >= MAPN || w->field[ny][nx] != w->field[y][x] - 1) continue;
            float e = hypotf((float)(nx - w->gx), (float)(ny - w->gy));
            if (e < best) { best = e; bx = nx; by = ny; }
        }
        if (bx < 0 || (k > 0 && !walk_clear(G.px, G.py, bx + 0.5f, by + 0.5f))) break;
        x = bx; y = by; tx = x + 0.5f; ty = y + 0.5f;
    }
    *out = atan2f(ty - G.py, tx - G.px);
    return 1;
}

/* one of a kind a few cells ahead, in view (the console, CRAWL_MEET) */
static int meet_creature(int k) {
    int far = 0;
    for (int d = 1; d <= 7; d++) {                                 /* the farthest open cell in view, up to seven */
        int mx = (int)(G.px + cosf(G.ang) * d), my = (int)(G.py + sinf(G.ang) * d);
        if (G.map[my][mx].kind != K_FLOOR || !line_clear(G.px, G.py, mx + 0.5f, my + 0.5f)) break;
        if (d >= 3 && !occupied(mx, my, NULL)) far = d;
    }
    if (!far) return 0;
    Mob *m = spawn_mob((int)(G.px + cosf(G.ang) * far), (int)(G.py + sinf(G.ang) * far), k);
    if (m && k == MOB_HUNTER) m->sub = 0;
    return m != NULL;
}

static int con_update(float dt);
static void crawl_update(double dt) {
    float fdt = (float)dt;
    if (con_update(fdt)) return;                                   /* the console is down: the crypt waits, the music plays */
    G.msg_t = fmaxf(0.0f, G.msg_t - fdt); G.hint_t = fmaxf(0.0f, G.hint_t - fdt);
    G.ui_t += fdt;
    update_toasts(fdt);
    if (G.player && G.ui && !G.shop) { update_ui(); return; }     /* a menu is up: the crypt holds its breath */
    if (G.player && (G.tally || G.dead) && IN.hit[KEY_ESC]) { open_ui(UI_QUIT); return; }
    if (G.player && (G.tally || G.dead) && IN.hit[KEY_B]) { open_ui(UI_BESTIARY); return; }
    G.t += dt; G.level_t += dt; G.card += fdt;
    if (G.shop || G.dead || G.tally) G.bar_t = BAR_T;               /* always up at the merchant's, and at the end */
    else G.bar_t = fmaxf(0.0f, G.bar_t - fdt);
    G.bar = G.bar_t > 0.0f ? fminf(1.0f, G.bar + fdt / BAR_SLIDE) : fmaxf(0.0f, G.bar - fdt / BAR_SLIDE);
    G.peek = fmaxf(0.0f, G.peek - fdt);                            /* the readout rises over the HOLD H plate, then settles */
    G.lift += ((G.peek > 0.0f ? 1.0f : 0.0f) - G.lift) * fminf(1.0f, fdt * 8.0f);
    if (G.shop) { update_shop(dt); return; }
    if (G.tally) {                                                 /* the level-complete screen; the crypt waits */
        update_tally(dt);
        if (G.tally) G.fade = smoothstepf(0.0f, 0.4f, (float)G.tally_t) * (1.0f - smoothstepf(TALLY_END - 0.7f, TALLY_END, (float)G.tally_t));
        return;
    }
    float ox = G.px, oy = G.py;
    if (G.level_t > 1.0 && !G.dead) { if (G.player && !G.exiting) update_human(fdt); else update_player(fdt); }
    if (fdt > 0.0f) { G.pvx = (G.px - ox) / fdt; G.pvy = (G.py - oy) / fdt; }
    build_hunt(fdt);
    pick_hunters();
#ifdef CRAWL_DEBUG
    if (getenv("CRAWL_KILLAT") && !G.dead && G.t >= atof(getenv("CRAWL_KILLAT")) && G.t - dt < atof(getenv("CRAWL_KILLAT"))) {   /* testing: struck down */
        G.level_t = fmax(G.level_t, 2.0); hurt_ex(99999.0f, HURT_MAGIC, G.px, G.py);
    }
    static int meet_level = -1;                                    /* one of a kind, ahead, once the level's card has gone (and in each pocket) */
    if (getenv("CRAWL_MEET") && meet_level != G.level * 16 + PK.depth && G.level_t > 5.0 && !G.dead
        && meet_creature(clampi(atoi(getenv("CRAWL_MEET")), 0, NKINDS - 1))) meet_level = G.level * 16 + PK.depth;
#endif
    for (int i = 0; i < MAXMOBS; i++) if (G.mobs[i].alive) update_mob(&G.mobs[i], fdt);
    update_pools(fdt);
    /* the Wraith comes, once, after about four minutes on a level, from the far end of it */
    float wraith_at = 240.0f;
#ifdef CRAWL_DEBUG
    if (getenv("CRAWL_WRAITH")) wraith_at = (float)atof(getenv("CRAWL_WRAITH"));
#endif
    if (!G.wraith_done && !PK.depth && (G.wraith_now || ((G.level >= KINDS[MOB_WRAITH].debut || crawl_loop(G.level) > 0) && G.level_t > wraith_at && !crawl_boss(G.level)))
        && !G.tally && !G.exiting && (G.wraith_try -= fdt) <= 0.0f) {
        int x, y;
        G.wraith_try = 5.0f;
        if (pick_cell_at_depth(G.cx, G.cy, 20, 400, 1, &x, &y)) {
            Mob *m = spawn_mob(x, y, MOB_WRAITH);
            if (m) { m->alert = 1e9f; G.wraith_done = 1; G.wraith_now = 0; sfx_at(SFX_WAIL, x + 0.5f, y + 0.5f, 3.0f); }
#ifdef CRAWL_DEBUG
            fprintf(stderr, "EVENT wraith comes t=%.1f\n", G.level_t);
#endif
        }
    }
    update_shots(fdt);
    update_bones(fdt);
    update_doors(fdt);
    update_slide(fdt);
    update_wisps(fdt);
#ifdef CRAWL_DEBUG
    static int wisp_level = -1;                                    /* testing: an exit wisp, free, as the level opens */
    if (getenv("CRAWL_BLESS") && !G.shop) G.blessed = 1;          /* testing: the altar's blessing on every level */
    if (getenv("CRAWL_WISP") && wisp_level != G.level && G.level_t > 1.5 && !G.dead) {
        int c = atoi(getenv("CRAWL_WISP"));
        summon_wisp(c >= 2 ? WK_KEY : WK_EXIT, c >= 2 ? (c - 2) % NKEYS : 0, 0.0f);
        wisp_level = G.level;
    }
#endif
    if (!G.dead) { update_items(); update_abilities(fdt); update_altar(); update_status(fdt); if (G.level_t > 1.0) update_defense(fdt); }
    update_boss(fdt);
    update_hazards(fdt);
    update_omens(fdt);
    if (G.bfs_t > 0.0f && (G.bfs_t += fdt) > 1.6f) G.bfs_t = 0.0f;
    G.hurt = fmaxf(0.0f, G.hurt - fdt / 0.35f);
    G.gold = fmaxf(0.0f, G.gold - fdt / 0.45f);
    G.chill = fmaxf(0.0f, G.chill - fdt / 0.6f);
    G.face_t = fmaxf(0.0f, G.face_t - fdt);
    G.grin_t = fmaxf(0.0f, G.grin_t - fdt);
    G.kick *= expf(-fdt * 9.0f);
    if (G.readout > 0.0f) G.readout -= fdt;
    if (G.hp <= G.max_hp / 4 && !G.dead && (G.heartbeat -= fdt) <= 0.0f) { snd_sfx_ex(SFX_HEART, 0.9f, 0.5f); G.heartbeat = 1.1f; }

    name_newcomers();

    /* keep the action coming: if nothing has happened for a while, wake something up ahead */
    if (G.calm > (G.player ? 35.0f : 11.0f) && !PK.depth) {        /* a person clears a level for good, mostly */
        int x, y, alive = 0;
        for (int i = 0; i < MAXMOBS; i++) { const Mob *m = &G.mobs[i]; alive += m->alive && !m->guard && !KIND_IS(m, KF_UNCOUNTED) && m->type != MOB_WSKULL && m->type != MOB_SPIDER && m->type != MOB_GARGOYLE && m->type != MOB_LING; }
        if (alive < 8 && pick_cell_at_depth(G.cx, G.cy, 4, 8, 1, &x, &y)) spawn_wanderer(x, y);
        G.calm = 4.0f;
    }

    G.mood_t -= fdt;
    if (G.mood_t <= 0.0f) {                                        /* torch crackle follows nearby torches */
        float m = 0.0f;
        for (int l = 0; l < G.nlights; l++) {
            float d = hypotf(G.lights[l].x - G.px, G.lights[l].y - G.py);
            if (d < 5.0f) m += 1.2f / (1.0f + d * d);
        }
        snd_set_mood(clampf(m, 0.0f, 1.0f));
        G.mood_t = 0.15f;
    }

#ifdef CRAWL_DEBUG
    if (getenv("CRAWL_MAPAT") && G.level_t >= atof(getenv("CRAWL_MAPAT")) && G.level_t - dt < atof(getenv("CRAWL_MAPAT"))) debug_map(1);
    {
        static double next;
        if (G.t >= next) {
            next = G.t + 1.0;
            int alive = 0;
            for (int i = 0; i < MAXMOBS; i++) alive += G.mobs[i].alive;
            fprintf(stderr, "lvl=%d t=%5.1f state=%d cell=(%d,%d)->(%d,%d) h=%d next=%d spd=%.2f mobs=%d explored=%d/%d hp=%d gold=%d items=%d/%d key=%d vault=%d dead=%d souls=%d val=%d/%d\n",
                    G.level, G.t, G.state, G.cx, G.cy, G.tx, G.ty, G.h, G.next_h, G.spd, alive, G.explored, G.ncells,
                    G.hp, RUN.treasure, G.items_got, G.items_total, G.has_key[0] | G.has_key[1] << 1 | G.has_key[2] << 2,
                    G.nvaults ? G.doors[G.vault_door[0]].locked : -1, G.dead, RUN.souls, G.value_got, G.value_total);
            fprintf(stderr, "   throws=%d bonehits=%d bathits=%d firehits=%d swordhits=%d bites=%d dodges=%d exit=(%d,%d) seen=%d leaving=%d kills=%d/%d\n",
                    throws, hits_bone, hits_bat, hits_fire, hits_sword, bites, dodges, G.exit_fx, G.exit_fy, G.exit_seen, G.leaving, G.kills, G.kills_total);
        }
    }
#endif
    /* Explore first and gather treasure: head for the exit (via its key and vault if need be) once
     * most of the treasure is in hand and most of the level seen, or after four and a half minutes,
     * or at five and a half whatever happens. Not while a key is in hand for a vault that doesn't
     * hold the exit, or while a freshly opened vault is still full. */
    if (!G.exit_seen && G.exit_fx >= 0 && hypotf(G.exit_fx + 0.5f - G.px, G.exit_fy + 0.5f - G.py) < 7.0f
        && line_clear(G.px, G.py, G.exit_fx + 0.5f, G.exit_fy + 0.5f)) {
        G.exit_seen = 1;
        snd_sfx_ex(SFX_SHIMMER, 0.35f, 0.5f);                       /* a faint call from the runes */
#ifdef CRAWL_DEBUG
        fprintf(stderr, "EVENT exit_seen t=%.2f\n", G.t);
#endif
    }
    int pending = 0;
    for (int k = 0; k < G.nvaults; k++) {
        const Door *d = &G.doors[G.vault_door[k]];
        if (k == G.exit_in_vault) continue;
        if (G.has_key[k] && d->locked == 1) pending = 1;
        if (d->locked == 2 && G.level_t - G.unlock_t < 30.0) {
            for (int i = 0; i < G.nitems; i++)
                if (G.items[i].alive && G.items[i].type != IT_KEY && vault_of_room(G.items[i].cx, G.items[i].cy) == k) pending = 1;
        }
    }
    float got = G.value_total ? (float)G.value_got / (float)G.value_total : 1.0f;
    float want = RUN.goal > RUN.treasure ? 0.9f : 0.75f;            /* saving up: it wants nearly all of it */
    double stay = G.hp >= G.max_hp * 0.6f ? 30.0 : 0.0;            /* in good health it lingers for the treasure */
    if (!G.leaving && !G.dead && G.exit_fx >= 0 && !G.player
        && ((G.level_t > 270.0 + stay && !pending) || G.level_t > 330.0 + stay
            || (got >= want && G.explored > G.ncells * 0.7f && G.level_t > 150.0 && !pending))) {
        G.leaving = 1;
        if (spirit() >= 2.0f && !(G.wisps[0].on && G.wisps[0].kind == WK_EXIT && G.wisps[0].state != WS_GONE))
            summon_wisp(WK_EXIT, 0, WISP_COST);                    /* a soul to spare: a wisp to light the way out */
#ifdef CRAWL_DEBUG
        fprintf(stderr, "EVENT leaving t=%.2f in_vault=%d got=%.2f\n", G.t, G.exit_in_vault, got);
#endif
    }
    if (!G.tally && !G.dead && !G.player && !PK.depth && (G.exit_fx < 0 || G.level_t > (boss_sealed() ? 1500.0 : 600.0))) {   /* no exit, or hopelessly lost: call it done (a boss gets longer) */
        G.tally = 1; G.tally_t = 0.0; G.clear_time = G.level_t; G.retries = 0; snd_sfx_ex(SFX_FANFARE, 1.0f, 0.5f);
        rec_cleared();
    }
    float fout = 1.0f;
    if (G.dead) {                                                  /* down to the floor, a red haze, then the level again */
        G.dead_t += dt;
        G.camz = lerpf(0.5f, 0.1f, smoothstepf(0.0f, 0.9f, (float)G.dead_t));
        fout = 1.0f - smoothstepf(3.8f, 4.8f, (float)G.dead_t);
        if (G.dead_t >= 5.0) {
            if (PK.depth) pocket_return(1);                        /* in a pocket: thrown back out to its chain's room */
            else { RUN.treasure -= G.level_treasure; G.hp = gear_max_hp(&RUN.gear); start_level(); }
        }
    }
    PK.since += fdt;
    if (PK.depth && !G.player) PK.auto_t += fdt;                    /* the autopilot's time in the level's chains */
    float fin = fminf(smoothstepf(0.0f, 1.2f, (float)G.level_t), smoothstepf(0.0f, 0.6f, PK.since));   /* and through a doorway */
    if (G.exiting) fout = 1.0f - smoothstepf(1.6f, 2.4f, G.exit_t);  /* into the light */
    G.fade = fin * fout;
    build_lightmap();
}

/* ================================================================== */
/* The merchant's antechamber, before each level: the owner's cat on a chest, and five to seven wares
 * on pedestals in an arc round the back wall (an attack spell, a defensive rune, the rest things to
 * wear), one to three to be had for the gold in hand, deeper down more. The first is free. A person
 * can hold one for the next visit. Then out through the east door and into the level. */
/* ================================================================== */

#define CAT_X 10.62f
#define CAT_Y 15.45f
#define CAT_S 1.05f
#define SHOP_MID 15.5f                      /* the middle of the room, east to west */
#define SHOP_DOOR 21                        /* the east door, and the way on */
enum { SH_GREET, SH_LOOK, SH_THINK, SH_GO, SH_BUY, SH_THANK, SH_SHRINE_GO, SH_SHRINE, SH_LEAVE };
static int offer_sprite(const Offer *o) {
    static const int PIECE_SPR[NIK] = { SP_ITEM_RING, SP_ITEM_BRACER, SP_ITEM_BOOTS, SP_ITEM_AMULET, SP_ITEM_HOOD, SP_ITEM_ROBE, SP_ITEM_RING, SP_ITEM_BAG };
    return o->kind == GK_ATK ? SP_ITEM_TOME : (o->kind == GK_DEF ? SP_ITEM_RUNE : PIECE_SPR[o->piece.kind]);
}

static void gen_shop(void) {
    memset(G.map, 0, sizeof G.map);
    for (int y = 0; y < MAPN; y++)
        for (int x = 0; x < MAPN; x++) {
            Cell *c = &G.map[y][x];
            c->kind = K_WALL; c->region = -1; c->door = -1;
            c->tex = (uint8_t)(hash2(x, y) % 10 < 8 ? T_BRICK : T_BANNER);
        }
    G.nrooms = 0; G.ndoors = 0; G.nlights = 0; G.nlc = 0; G.ndecos = 0; G.nitems = 0;
    G.items_total = G.items_got = G.value_total = G.value_got = 0; G.nvaults = 0;
    G.nsecrets = G.secrets_found = G.nlinks = G.links_found = 0; G.slide = G.seek_secret = -1;
    for (int k = 0; k < NKEYS; k++) G.vault_room[k] = G.vault_door[k] = -1;
    memset(G.bulky, 0, sizeof G.bulky); memset(G.fog, 0, sizeof G.fog);
    for (int y = 12; y <= 17; y++) for (int x = 10; x <= 20; x++) carve(x, y, 0, TH_BRICK);
    G.rooms[G.nrooms++] = (Room){ 10, 12, 11, 6, 0, TH_BRICK };
    for (int x = SHOP_DOOR; x <= SHOP_DOOR + 5; x++) carve(x, 15, 1, TH_CORRIDOR);
    G.map[15][SHOP_DOOR].kind = K_DOOR; G.map[15][SHOP_DOOR].door = 0;
    G.doors[0] = (Door){ SHOP_DOOR, 15, 1, 0.0f, 0.0f, 0, 0, 0 }; G.ndoors = 1;
    G.map[11][15].tex = T_SIGN;                                    /* the merchant's sign behind the wares */
    G.map[11][12].tex = G.map[11][18].tex = T_TORCH0;
    light_static(12.5f, 11.92f, 1.35f, 4.6f); light_static(18.5f, 11.92f, 1.35f, 4.6f);
    G.map[17][9].tex = T_TORCH0; light_static(10.08f, 17.5f, 1.2f, 4.2f);   /* by the cat */
    G.map[13][SHOP_DOOR].tex = T_TORCH0; light_static(SHOP_DOOR - 0.08f, 13.5f, 1.2f, 4.2f);   /* the east end */
    G.map[11][14].tex = G.map[11][16].tex = T_BRICK;
    add_deco(10.4f, 12.4f, SP_CANDLES); light_static(10.45f, 12.45f, 0.9f, 3.0f);
    add_deco(15.5f, 12.3f, SP_CANDLES); light_static(15.5f, 12.35f, 0.8f, 3.2f);   /* under the sign */
    add_deco(20.55f, 12.45f, SP_URN); add_deco(20.55f, 17.55f, SP_URN);
    G.sh_deco = G.ndecos;                                          /* the Rune Shrine, across from the wares */
    add_deco(SHRINE_X, SHRINE_Y, RUN.shrine ? SP_SHRINE_C : SP_SHRINE_S);
    G.map[(int)SHRINE_Y][(int)SHRINE_X].kind = K_PROP;
    add_deco(SHRINE_X - 1.15f, 17.6f, SP_CANDLES); light_static(SHRINE_X - 1.1f, 17.55f, 0.95f, 3.4f);
    add_deco(SHRINE_X + 1.15f, 17.6f, SP_CANDLES); light_static(SHRINE_X + 1.1f, 17.55f, 0.95f, 3.4f);
    G.ped_n = gear_shop_pedestals(G.level);                        /* an arc, its ends come forward */
    float half = 3.0f + 0.4f * (float)(G.ped_n - 5);
    for (int i = 0; i < G.ped_n; i++) {
        float t = -1.0f + 2.0f * (float)i / (float)(G.ped_n - 1);
        G.ped_x[i] = SHOP_MID + t * half; G.ped_y[i] = 12.9f + 0.75f * t * t;
        add_deco(G.ped_x[i], G.ped_y[i], SP_PEDESTAL);
    }
    G.ncells = 0;
    for (int y = 0; y < MAPN; y++) for (int x = 0; x < MAPN; x++) if (G.map[y][x].kind == K_FLOOR) G.ncells++;
}

static void shop_turn(float to, float dur) {
    G.sa0 = G.ang; G.sa1 = G.ang + wrap_pi(to - G.ang); G.sdur = dur; G.smove = 0.0f;
    G.sx0 = G.sx1 = G.px; G.sy0 = G.sy1 = G.py;
}
static void shop_walk(float x, float y, float speed) {
    G.sx0 = G.px; G.sy0 = G.py; G.sx1 = x; G.sy1 = y;
    G.sa0 = G.sa1 = G.ang; G.sdur = fmaxf(0.2f, hypotf(x - G.px, y - G.py) / speed); G.smove = 1.0f;
}
static float face_of(float x, float y) { return atan2f(y - G.py, x - G.px); }

/* which ware, if any: best value for the gold, a whim now and then, sometimes saving up for a
 * spell it cannot yet afford, sometimes splashing out on the dearest thing it can */
static int shop_choose(void) {
    const Gear *g = &RUN.gear;
    float v[NPED];
    int best = -1, dear = -1, purse = RUN.treasure;
    float bs = 0.0f;
    if (G.shop_bought > 0) purse -= (int)(gear_haul(G.level) * 0.15f);   /* after the first, a little gold kept back */
    for (int i = 0; i < G.ped_n; i++) {
        v[i] = G.offer_gone[i] ? 0.0f : gear_value(g, &G.offers[i], G.level);
        if (v[i] <= 0.0f || G.offers[i].price > purse) continue;
        float s = v[i] / sqrtf(fmaxf(1.0f, (float)G.offers[i].price) / 100.0f + 1.0f) * (0.8f + 0.4f * rndf());
        if (s > bs) { bs = s; best = i; }
        if (dear < 0 || G.offers[i].price > G.offers[dear].price) dear = i;
    }
    G.shop_saving = 0;
    int natk = 0, fresh = -1;                                      /* three attack spells before anything else: the */
    for (int k = 0; k < NATK; k++) natk += g->atk[k] >= 0;         /* runs that skipped them were the ones that died */
    for (int i = 0; i < G.ped_n && natk < 3; i++) {
        const Offer *o = &G.offers[i];
        if (!G.offer_gone[i] && o->kind == GK_ATK && o->id != AT_SPEAR && g->atk[o->id] < 0 && o->price <= purse && (fresh < 0 || v[i] > v[fresh])) fresh = i;
    }
    if (fresh >= 0) return fresh;
    if (G.level == 1 || G.shop_bought > 0) return best;            /* the first is free; after one, the best for the gold */
    #define SPELL(o) ((o).kind == GK_ATK || (o).kind == GK_DEF)
    float next = gear_haul(G.level) * 0.6f;                        /* what the coming level should bring, cautiously */
    if (RUN.goal > 0) {                                            /* saving: a spell worth the wait now? */
        int sp = -1;
        for (int i = 0; i < G.ped_n; i++)
            if (SPELL(G.offers[i]) && G.offers[i].price <= RUN.treasure && v[i] > 0.5f && (sp < 0 || v[i] > v[sp])) sp = i;
        if (sp >= 0) { RUN.goal = 0; return sp; }
        if (RUN.treasure < RUN.goal && RUN.goal <= RUN.treasure + 2.0f * next && rndf() < 0.7f) { G.shop_saving = 1; return -1; }
        RUN.goal = 0;                                              /* bought elsewhere, or out of reach */
    }
    for (int i = 0; i < G.ped_n; i++) {                            /* a spell it can't quite afford: put the gold by, */
        if (!SPELL(G.offers[i]) || G.offers[i].price <= RUN.treasure || v[i] <= (best >= 0 ? v[best] : 0.3f)) continue;
        if (G.offers[i].price > RUN.treasure + 2.0f * next) continue;   /* if two levels might bring it */
        float p = G.offers[i].price <= RUN.treasure + next ? 0.4f : 0.15f;
        if (best >= 0 && SPELL(G.offers[best])) p *= 0.3f;         /* a spell in hand already */
        if (rndf() < p) { RUN.goal = G.offers[i].price; G.shop_saving = 1; return -1; }
    }
    #undef SPELL
    if (dear >= 0 && rndf() < 0.12f) return dear;                  /* splurge */
    return best;
}

static void begin_shop(void) {
    hz_clear(); snd_set_boss(0, 0);
    G.shop = 1; G.state = PS_SHOP;
    memset(G.wisps, 0, sizeof G.wisps);                            /* nor into the merchant's room */
    crawl_art_level(1);                                            /* the antechamber looks the same every time */
    gen_shop();
    memset(G.mobs, 0, sizeof G.mobs); memset(G.shots, 0, sizeof G.shots); memset(G.booms, 0, sizeof G.booms);
    memset(G.bones, 0, sizeof G.bones); memset(G.beams, 0, sizeof G.beams); memset(G.burns, 0, sizeof G.burns);
    reset_caster_state();
    if (G.hp <= 0) G.hp = G.max_hp;
    G.tally = 0; G.dead = 0; G.exiting = G.leaving = 0; G.readout = 0.0f; G.card = 99.0f;
    G.hurt = G.gold = G.kick = G.chill = 0.0f;
    G.px = 13.6f; G.py = 16.2f; G.cx = 13; G.cy = 16;              /* close by the merchant, the wares before you */
    G.ang = face_of(CAT_X, CAT_Y);
    G.camz = 0.5f; G.hand = 0.0f; G.spd = 0.0f; G.bob = 0.0f; G.bar = 1.0f;
    int bag_up = 0;
    for (int p = 0; p < G.ped_n; p++) {                            /* a spell, the rest to wear; one bag at most */
        int pool = p == 0 ? POOL_ATTACK : POOL_GEAR;               /* runes are the Rune Shrine's now */
        Offer o = gear_offer(pool, G.level, &RUN.gear, &G.rs);
        for (int k = 0; k < 8 && bag_up && o.kind == GK_PIECE && o.piece.kind == IK_BAG; k++) o = gear_offer(pool, G.level, &RUN.gear, &G.rs);
        bag_up |= o.kind == GK_PIECE && o.piece.kind == IK_BAG;
        if (G.level == 1) o.price = 0;
        G.offers[p] = o; G.offer_gone[p] = G.offer_back[p] = 0;
    }
    if (RUN.gear.has_held) {                                       /* last visit's hold: back as it was, at the price it showed */
        const Offer *h = &RUN.gear.held;
        int p = h->kind == GK_ATK ? 0 : 2;
        G.offers[p] = *h; G.offer_back[p] = 1;
    }
    G.shop_left = gear_shop_purchases(G.level); G.shop_confirm = G.shop_letgo = G.shop_held = -1;
    G.sh_look = G.sh_takes = G.sh_pulls = G.sh_nq = G.sh_busy = G.sh_auto = G.sh_fav = 0; G.sh_mode = 0; G.sh_arm = -1;
    G.sh_qt = G.sh_glow = G.sh_note_t = 0.0f; memset(G.sh_flash, 0, sizeof G.sh_flash);
    G.shop_step = SH_GREET; G.shop_t = 0.0; G.shop_pick = -1; G.shop_bought = 0; G.shop_look = -1;
    G.shop_gold = RUN.treasure; G.shop_saving = 0; G.sparkle = 0.0f;
    G.shop_heal = G.hp + (G.max_hp - G.hp) * 4 / 10;               /* a rest by the purring cat */
    G.cat_blink = G.cat_slow = G.cat_ear = G.cat_trill = 0.0f; G.purr_t = 0.4f;
    G.cat_next_blink = 2.5f + 2.0f * rndf(); G.cat_next_ear = 3.0f + 3.0f * rndf(); G.cat_tail_k = 0.0f;
    G.sdur = 0.0f; G.smove = 0.0f;
    G.level_t = 0.0;
#ifdef CRAWL_DEBUG
    fprintf(stderr, "SHOP level %d gold %d souls %d:", G.level, RUN.treasure, RUN.souls);
    for (int p = 0; p < G.ped_n; p++) { char nm[80]; gear_name(&G.offers[p], nm, sizeof nm); fprintf(stderr, " [%s $%d%s%s]", nm, G.offers[p].price, G.offers[p].rare ? " RARE" : "", G.offer_back[p] ? " HELD" : ""); }
    fprintf(stderr, "\n");
#endif
}

static void shop_cat(float dt) {
    G.cat_blink = fmaxf(0.0f, G.cat_blink - dt);
    G.cat_slow = fmaxf(0.0f, G.cat_slow - dt);
    G.cat_ear = fmaxf(0.0f, G.cat_ear - dt);
    G.cat_trill = fmaxf(0.0f, G.cat_trill - dt);
    G.cat_tail_k = fmaxf(0.0f, G.cat_tail_k - dt * 0.25f);
    if ((G.cat_next_blink -= dt) <= 0.0f) { if (G.cat_slow <= 0.0f) G.cat_blink = 0.16f; G.cat_next_blink = 2.5f + 4.0f * rndf(); }
    if ((G.cat_next_ear -= dt) <= 0.0f) { G.cat_ear = 0.32f; G.cat_ear_side = rnd(2); G.cat_next_ear = 3.5f + 5.0f * rndf(); }
    float want = clampf(wrap_pi(atan2f(G.py - CAT_Y, G.px - CAT_X)) * 2.2f, -1.0f, 1.0f);   /* eyes on the caster */
    G.cat_look += (want - G.cat_look) * fminf(1.0f, dt * 4.0f);
    if ((G.purr_t -= dt) <= 0.0f) { sfx_at(SFX_PURR, CAT_X, CAT_Y, 0.55f); G.purr_t = 1.95f; }   /* quietly, on and on */
}

/* the ware on pedestal i changes hands */
static void shop_purchase(int i) {
    const Offer *o = &G.offers[i];
    G.shop_pick = i; G.shop_bought++; G.shop_left--; G.offer_gone[i] = 1; G.sparkle = 0.8f; G.shop_confirm = -1;
    if (i == G.shop_held) { RUN.gear.has_held = 0; G.shop_held = -1; }   /* bought after all: the hold is spent */
    RUN.treasure -= o->price;
    if (o->kind == GK_PIECE) take_piece(o, G.player);
    else gear_take(&RUN.gear, o);
    if (o->price > 0) snd_sfx_ex(SFX_COIN, 1.1f, 0.5f);
    snd_sfx_ex(SFX_SHIMMER, 0.8f, 0.5f);
    G.grin_t = 2.5f;
    sfx_at(SFX_TRILL, CAT_X, CAT_Y, 1.3f); G.cat_trill = 0.5f; G.cat_slow = 1.6f; G.cat_tail_k = 1.0f;
#ifdef CRAWL_DEBUG
    char nm[80]; gear_name(o, nm, sizeof nm);
    fprintf(stderr, "EVENT bought %s for %d (left %d) t=%.2f\n", nm, o->price, RUN.treasure, G.t);
#endif
}


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
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(G.sh_note, sizeof G.sh_note, fmt, ap);
    va_end(ap);
    G.sh_note_t = 4.0f;
    if (!G.player) say("%s", G.sh_note);                           /* the autopilot's doings, for whoever watches */
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

static void shop_human(float dt);
static void update_ui(void);

static void shop_after(void) {                                     /* the wares done with: the shrine, if it has business there */
    G.shop_t = 0.0;
    if (shrine_errand()) { G.shop_step = SH_SHRINE_GO; G.sh_auto = 0; shop_walk(SHRINE_X, SHRINE_Y - 1.7f, 1.6f); }
    else { G.shop_step = SH_THANK; shop_turn(face_of(CAT_X, CAT_Y), 0.7f); }
}

static void update_shop(double dt) {
    float fdt = (float)dt;
    G.shop_t += dt;
    float st = (float)G.shop_t;
    shop_cat(fdt);
    shrine_tick(fdt);
    if (G.player) {
        if (G.ui) { update_ui(); return; }
        shop_human(fdt);
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
        if (st >= 2.4f) { G.shop_step = SH_LOOK; G.shop_look = 0; G.shop_t = 0.0; shop_turn(face_of(G.ped_x[0], G.ped_y[0]), 0.7f); }
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
        G.hand = 0.0f; G.shop_t = 0.0; G.shop_step = SH_THANK; shop_turn(face_of(CAT_X, CAT_Y), 0.7f);
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
    if (G.px > SHOP_DOOR + 1.4f) {                                 /* through the door and into the level */
        G.fade = 1.0f - smoothstepf(SHOP_DOOR + 1.4f, SHOP_DOOR + 2.6f, G.px);
        if (G.px >= SHOP_DOOR + 2.6f) {
            if (G.shop_held < 0) RUN.gear.has_held = 0;            /* last visit's hold, bought or passed over, is gone */
            start_level(); return;
        }
    }
    update_doors(fdt);
    if (G.px <= SHOP_DOOR + 1.4f) G.fade = smoothstepf(0.0f, 0.7f, (float)G.level_t);
    G.mood_t -= fdt;
    if (G.mood_t <= 0.0f) { snd_set_mood(0.6f); G.mood_t = 0.5f; }
    build_lightmap();
}

/* ================================================================== */
/* A person at the controls (hold H for three seconds)                 */
/* ================================================================== */

#define MOUSE_TURN 0.0015f                  /* radians per mouse count */
#define KEY_TURN 2.6f                       /* radians a second with the arrow keys */
static const char *const ATK_SHORT[NATK] = { "FIRE", "FROST", "CHAIN", "SPEAR", "DRAIN", "HELL", "WISPS", "SPARKS", "BFS" };
static const char *const KEY_COLOR[NKEYS] = { "GOLD", "SILVER", "CRIMSON" };

static void say(const char *fmt, ...) {
    char b[64];
    va_list ap;
    va_start(ap, fmt); vsnprintf(b, sizeof b, fmt, ap); va_end(ap);
    if (strcmp(b, G.msg) || G.msg_t < 1.2f) { snprintf(G.msg, sizeof G.msg, "%s", b); G.msg_t = 2.2f; }
}

/* The first time each kind of creature, in each rank, comes into view on a level, a caption names
 * it: A BLOODY SKELETON. One at a time; the rest wait their turn. */
static void name_newcomers(void) {
    if (G.msg_t > 0.4f || G.dead || G.shop || G.tally) return;
    float fx = cosf(G.ang), fy = sinf(G.ang);
    for (int i = 0; i < MAXMOBS; i++) {
        const Mob *m = &G.mobs[i];
        if (!m->alive || m->state != MS_WALK || (G.named[m->type] >> m->rank & 1)) continue;
        if ((m->type == MOB_SPIDER || m->type == MOB_GARGOYLE) && m->sub == 0) continue;   /* still hidden in its web, or still a statue */
        if (m->type == MOB_SAC || shade_hidden(m) || m->boss) continue;   /* a shade shows only in light; a boss has its card */
        float dx = m->x - G.px, dy = m->y - G.py, d = sqrtf(dx * dx + dy * dy) + 1e-4f;
        if (d > 9.0f || (dx * fx + dy * fy) / d < 0.5f || !line_clear(G.px, G.py, m->x, m->y)) continue;
        G.named[m->type] |= (uint8_t)(1 << m->rank);
        rec_seen(m);
        const char *word = KINDS[m->type].family == PF_CHITIN && m->rank == RK_V ? "PALE " : RANK_WORD[m->rank];
        char first = *word ? *word : KINDS[m->type].name[0];
        say("%s %s%s", KIND_IS(m, KF_UNIQUE) ? "THE" : (strchr("AEIOU", first) ? "AN" : "A"), word, KINDS[m->type].name);
        return;
    }
}

static void fizzle(int sp) {                                      /* chain or drain with nothing to bite on */
    float ex = G.px, ey = G.py;
    for (float d = 0.3f; d < 4.0f; d += 0.1f) {
        float x = G.px + cosf(G.ang) * d, y = G.py + sinf(G.ang) * d;
        if (blocks_light((int)x, (int)y)) break;
        ex = x; ey = y;
    }
    add_beam(sp, G.px + cosf(G.ang) * 0.35f, G.py + sinf(G.ang) * 0.35f, 0.42f, ex, ey, 0.45f);
    snd_sfx_ex(SFX_ZAP, 0.5f, 0.5f);
}

static int open_door(Door *d) {
    if (d->locked == SEALED) { say("IT IS SEALED UNTIL THE BOSS FALLS"); snd_sfx_ex(SFX_KNOCK, 0.6f, 0.5f); return 0; }
    if (d->locked == 1) {
        if (!G.has_key[d->color]) { say("THE %s DOOR IS LOCKED", KEY_COLOR[d->color]); return 0; }
        d->locked = 2; G.has_key[d->color] = 0; G.unlock_t = G.level_t; snd_sfx(SFX_UNLOCK); pop_readout();
    }
    if (!d->opening && d->open < 0.95f) { d->opening = 1; sfx_at(SFX_DOOR, d->x + 0.5f, d->y + 0.5f, 1.0f); }
    return 1;
}

static int pocket_door(int x, int y);
static void human_exit(void) {                                    /* into the runes */
    G.exiting = 1; G.leaving = 1; G.exit_t = 0.0f; G.u = 1.0f; G.pc_t = -1.0f; G.hand = 0.0f;
    G.cx = G.tx = G.exit_fx; G.cy = G.ty = G.exit_fy;
    begin_turn(heading_angle(G.exit_dir)); G.adur = 0.4f;
    G.state = PS_EXIT; G.spd = 0.0f;
}

/* walking into something: a door opens (if it can), the exit takes you */
static void human_bump(int cx, int cy) {
    Cell *c = cell(cx, cy);
    if (G.shop) { if (c->kind == K_DOOR) open_door(&G.doors[c->door]); return; }
    if (c->kind == K_DOOR) open_door(&G.doors[c->door]);
    else if (c->kind == K_WALL && c->secret == 3) pocket_door(cx, cy);   /* a secret's doorway: walked through */
    else if (G.exit_fx >= 0 && cx == G.exit_wx && cy == G.exit_wy && (int)G.px == G.exit_fx && (int)G.py == G.exit_fy) {
        if (boss_sealed()) { if (G.msg_t <= 0.0f) { say("THE WAY IS SHUT WHILE IT LIVES"); snd_sfx_ex(SFX_KNOCK, 0.6f, 0.5f); } }
        else human_exit();
    }
}

/* E or Space: the door or exit just ahead */
/* The Backrooms' end: E at the visportal, once a chain, gives your health back in full, or fifty souls
 * if you are whole. It goes on drifting until it has given; then it stands still. */
static int visportal_use(void) {
    int sev = G.ptheme == PT_SEVERANCE;                            /* Severance's water cooler gives the same, as one drink */
    if (PK.vis_used[PK.chain]) { say(sev ? "THE COOLER HAS GIVEN ITS ONE DRINK" : "THE DOORWAYS GO ON, AND GIVE NO MORE"); snd_sfx_ex(SFX_KNOCK, 0.5f, 0.5f); return 1; }
    PK.vis_used[PK.chain] = 1;
    int heal = G.hp < G.max_hp;
    if (heal) { G.hp = G.max_hp; G.poison_t = G.slow_t = 0.0f; say(sev ? "A CUP OF WATER. YOU ARE WHOLE AGAIN" : "THE DOORWAYS MAKE YOU WHOLE AGAIN"); }
    else { RUN.souls += 50; G.level_souls += 50; pop_readout(); say(sev ? "A CUP OF WATER. FIFTY SOULS" : "THE DOORWAYS GIVE YOU FIFTY SOULS"); }
    if (sev && G.vis_deco >= 0) G.decos[G.vis_deco].spr = SP_COOLER_LOW;
    gain_coin(PK.chain, "AT THE END OF A CHAIN");
    snd_sfx_ex(sev ? SFX_POTION : SFX_SHIMMER, 1.0f, 0.5f); snd_sfx_ex(sev ? SFX_DRIP : SFX_POTION, 0.7f, 0.5f);
    G.grab_t = 1.0f; show_bar();
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT visportal chain %d %s t=%.2f\n", PK.chain, heal ? "heals" : "souls", G.t);
#endif
    return 1;
}

static int human_use(void) {
    int cx = (int)floorf(G.px + cosf(G.ang) * 0.8f), cy = (int)floorf(G.py + sinf(G.ang) * 0.8f);
    Cell *c = cell(cx, cy);
    if (c->kind == K_DOOR) { open_door(&G.doors[c->door]); return 1; }
    if (c->kind == K_WALL && c->secret == 3 && pocket_door(cx, cy)) return 1;   /* a secret's doorway */
    if (PK.depth && cx == G.vis_x && cy == G.vis_y) return visportal_use();
    if (!PK.depth && altar_callable()) {                           /* the altar's wisp, waiting at a secret you have been into */
        const Wisp *w = &G.wisps[1];
        float d = hypotf(w->x - G.px, w->y - G.py);
        if (w->on && d < 2.2f && fabsf(wrap_pi(atan2f(w->y - G.py, w->x - G.px) - G.ang)) < 0.35f + 0.4f / fmaxf(d, 0.5f)) { altar_call_home(); return 1; }
    }
    if (c->kind == K_WALL && c->secret == 2)                       /* a cracked wall: it gives */
        for (int i = 0; i < G.nsecrets; i++) if (G.secrets[i].sx == cx && G.secrets[i].sy == cy && !G.secrets[i].state) { open_secret(i); G.grab_t = 1.0f; return 1; }
    if (!G.shop && font_facing() >= 0) return font_drink(font_facing());
    if (!G.shop && G.boss == BOSS_WRAITH && brazier_facing() >= 0) {   /* a second's work to kindle it again */
        G.bbz_at = brazier_facing(); G.bbz_t = 1.0f;
        say("KINDLING THE BRAZIER"); snd_sfx_ex(SFX_IGNITE, 0.5f, 0.5f);
        return 1;
    }
    if (!G.shop && G.exit_fx >= 0 && cx == G.exit_wx && cy == G.exit_wy && hypotf(G.exit_fx + 0.5f - G.px, G.exit_fy + 0.5f - G.py) < 1.0f) {
        if (boss_sealed()) { say("THE WAY IS SHUT WHILE IT LIVES"); snd_sfx_ex(SFX_KNOCK, 0.6f, 0.5f); return 1; }
        human_exit(); return 1;
    }
    if (!G.shop && G.altar_x >= 0 && hypotf(G.altar_x + 0.5f - G.px, G.altar_y + 0.5f - G.py) < 1.6f
        && fabsf(wrap_pi(atan2f(G.altar_y + 0.5f - G.py, G.altar_x + 0.5f - G.px) - G.ang)) < 0.7f) { open_ui(UI_ALTAR); G.grab_t = 1.0f; return 1; }
    return 0;
}

static int body_blocked(float x, float y, int *bx, int *by) {
    float xs[2] = { x - BODY, x + BODY }, ys[2] = { y - BODY, y + BODY };
    for (int i = 0; i < 2; i++)
        for (int j = 0; j < 2; j++) {
            int cx = (int)floorf(xs[i]), cy = (int)floorf(ys[j]);
            if (blocks_move(cx, cy)) { *bx = cx; *by = cy; return 1; }
        }
    return 0;
}

static void push_from(float ox, float oy, float r) {              /* out of something round in the way */
    float vx = G.px - ox, vy = G.py - oy, d = hypotf(vx, vy);
    int bx, by;
    if (d >= r || d < 1e-4f) return;
    float nx = G.px + vx / d * (r - d), ny = G.py + vy / d * (r - d);
    if (!body_blocked(nx, ny, &bx, &by)) { G.px = nx; G.py = ny; }
}

/* one step of walking, sliding along walls; returns how far it got */
static float human_move(float dx, float dy) {
    float ox = G.px, oy = G.py;
    int bx, by;
    if (dx != 0.0f) { if (!body_blocked(G.px + dx, G.py, &bx, &by)) G.px += dx; else human_bump(bx, by); }
    if (dy != 0.0f) { if (!body_blocked(G.px, G.py + dy, &bx, &by)) G.py += dy; else human_bump(bx, by); }
    for (int i = 0; i < MAXMOBS; i++) {
        const Mob *m = &G.mobs[i];
        if (solid_mob(m) && m->state == MS_WALK && m->type != MOB_BAT) push_from(m->x, m->y, 0.5f);
    }
    if (G.shop) {                                                  /* the pedestals and the merchant's chest */
        for (int i = 0; i < G.ped_n; i++) push_from(G.ped_x[i], G.ped_y[i], 0.5f);
        push_from(CAT_X, CAT_Y, 0.75f);
    }
    return hypotf(G.px - ox, G.py - oy);
}

/* what a cast would bite on: the nearest creature close to the middle of the view */
static int aim_target(void) {
    float dx = cosf(G.ang), dy = sinf(G.ang);
    int best = -1; float bs = 1e9f;
    for (int i = 0; i < MAXMOBS; i++) {
        const Mob *m = &G.mobs[i];
        if (!targetable(m)) continue;
        float vx = m->x - G.px, vy = m->y - G.py, depth = vx * dx + vy * dy, lat = fabsf(-vx * dy + vy * dx);
        if (depth < 0.3f || depth > 10.0f || lat > depth * 0.12f + 0.3f || !line_clear(G.px, G.py, m->x, m->y)) continue;
        float sc = depth + lat * 4.0f;
        if (sc < bs) { bs = sc; best = i; }
    }
    return best;
}

static void pick_spell(int sp) {
    if (sp < 0 || sp >= NATK) return;
    if (RUN.gear.atk[sp] < 0) { say("NOT IN YOUR SPELLBOOK"); return; }
    if (sp != G.spell) snd_sfx_ex(SFX_TICK, 0.8f, 0.5f);
    G.spell = sp;
    show_bar();
    char nm[40]; gear_item_name(GK_ATK, sp, RUN.gear.atk[sp], nm, sizeof nm);
    say("%s", nm);
}

static void spell_keys(void) {
    for (int k = 0; k < 9; k++) if (IN.hit[KEY_1 + k]) pick_spell(k);
    if (IN.wheel) {                                                /* the wheel turns the pages */
        int dir = IN.wheel > 0 ? -1 : 1, sp = G.spell;
        for (int n = 0; n < NATK; n++) { sp = (sp + dir + NATK) % NATK; if (RUN.gear.atk[sp] >= 0) break; }
        pick_spell(sp);
    }
}

/* turning and walking, shared by the crypt and the merchant's room */
static void human_walk(float dt) {
    float run = IN.down[KEY_SHIFT] ? 1.5f : 1.0f;
    G.ang = wrap_pi(G.ang + IN.mdx * MOUSE_TURN + ((float)IN.down[KEY_RIGHT] - (float)IN.down[KEY_LEFT]) * KEY_TURN * run * dt);
    float fwd = (float)(IN.down[KEY_W] || IN.down[KEY_UP]) - (float)(IN.down[KEY_S] || IN.down[KEY_DOWN]);
    float side = (float)IN.down[KEY_D] - (float)IN.down[KEY_A];
    float len = sqrtf(fwd * fwd + side * side), moved = 0.0f;
    if (len > 0.0f) {
        float sp = WALK * run * gear_haste(&RUN.gear) * slow_factor() * dt / len;
        float c = cosf(G.ang), s = sinf(G.ang);
        moved = human_move((c * fwd - s * side) * sp, (s * fwd + c * side) * sp);
    }
    G.spd = dt > 0.0f ? moved / dt : 0.0f;
    float ph0 = G.bobph;
    G.bobph += moved * TAU_F;
    if (floorf(ph0 / PI_F) != floorf(G.bobph / PI_F)) footstep();
    G.camz = 0.5f + 0.014f * clampf(G.spd / CRUISE, 0.0f, 1.0f) * sinf(G.bobph);
    int ncx = (int)G.px, ncy = (int)G.py;
    if (ncx != G.cx || ncy != G.cy) {
        G.cx = G.tx = ncx; G.cy = G.ty = ncy; G.u = 1.0f;
        if (!G.shop && G.map[ncy][ncx].visits < 250) { if (G.map[ncy][ncx].visits == 0) G.explored++; G.map[ncy][ncx].visits++; }
    }
}

static void best_default_rank(void);
static void update_bestiary(void);
static int toast_showing(void);
static void open_ui(int which) {
    G.ui = which;
    show_bar();                                                    /* and it is there when the menu closes */
    if (which == UI_INVENTORY) { G.ui_sel = 0; G.inv_page = 0; G.inv0 = RUN.gear; snd_sfx_ex(SFX_KEY, 0.8f, 0.5f); }
    else if (which == UI_BESTIARY) { best_default_rank(); snd_sfx_ex(SFX_KEY, 0.8f, 0.5f); }
    else if (which == UI_ALTAR) {                                  /* on the first gift it can give you now */
        AltarRow r[3];
        altar_rows(r);
        G.ui_sel = 0;
        for (int i = 2; i >= 0; i--) if (r[i].ok && spirit() + 1e-4f >= (float)r[i].price) G.ui_sel = i;
        snd_sfx_ex(SFX_WHISPER, 0.5f, 0.5f);
    }
    else if (which == UI_SHRINE) {                                 /* on a pull, with what coins you have */
        G.sh_mode = SM_MENU; G.ui_sel = 0; G.sh_arm = -1;
        G.sh_coins = clampi(RUN.coins, 1, 3);
        if (G.sh_fav && !(RUN.attuned >> fav_kind(G.sh_fav) & 1)) G.sh_fav = 0;
        if (RUN.shrine) for (int k = 0; k < NDEF; k++) shrine_attune(k, RUN.gear.def[k]);   /* what you hold at Rare, it knows */
        snd_sfx_ex(SFX_WHISPER, 0.6f, 0.5f);
    }
    else if (which == UI_WISPS) {                                  /* on the wisp L last cast */
        WispChoice c[WISP_CHOICES];
        int n = wisp_choices(c), i = wisp_choice_at(c, n, RUN.wisp_kind, RUN.wisp_color);
        G.ui_sel = i < 0 ? 0 : i; snd_sfx_ex(SFX_SHIMMER, 0.5f, 0.5f);
    }
    else { G.ui_sel = 1; G.quit_line++; snd_sfx_ex(SFX_KNOCK, 0.6f, 0.5f); }
}

/* the hand and the spell in it, the way a person casts: click (or Ctrl) and it goes where you look */
static void human_cast(float dt, int may) {
    G.cooldown -= dt;
    int trig = may && (IN.down[KEY_MOUSE1] || IN.down[KEY_CTRL]);
    if (trig && G.silenced) { if (IN.hit[KEY_MOUSE1] || IN.hit[KEY_CTRL]) { say("THE VOID TAKES THE WORDS FROM YOU"); snd_sfx_ex(SFX_KNOCK, 0.4f, 0.5f); } trig = 0; }
    if (trig && G.pc_t < 0.0f && G.cooldown <= 0.0f && !G.dead) {
        if (G.spell == AT_BFS && RUN.souls < bfs_cost(RUN.gear.atk[AT_BFS])) {
            if (IN.hit[KEY_MOUSE1] || IN.hit[KEY_CTRL]) { say("THE BFS9000 NEEDS %d SOULS", bfs_cost(RUN.gear.atk[AT_BFS])); snd_sfx_ex(SFX_ZAP, 0.4f, 0.5f); }
        } else {
            float cyc = spell_cycle(&RUN.gear, G.spell);
            G.pc_t = 0.0f; G.pc_fired = 0; G.pc_len = clampf(cyc * 0.35f, 0.22f, 0.5f); G.cooldown = cyc;
        }
    }
    /* the hand is out of sight but for a cast or a ware reached for; after one it stays up a moment, ready
     * for the next, then sinks away */
    float rest = G.shop ? 0.45f : 0.62f;                           /* lower in the merchant's room, to see the wares */
    G.ready_t = G.pc_t >= 0.0f || G.grab_t > 0.0f ? 0.9f : fmaxf(0.0f, G.ready_t - dt);
    G.hand_base = G.ready_t > 0.0f ? fminf(rest, G.hand_base + dt / 0.08f) : fmaxf(0.0f, G.hand_base - dt / 0.3f);
    float base = G.hand_base, h = base;
    if (G.pc_t >= 0.0f) {
        G.pc_t += dt;
        float L = G.pc_len, top = lerpf(rest, 1.0f, G.bar);       /* with the bar away the sleeve shows at rest: no second jump */
        if (!G.pc_fired && G.pc_t >= 0.35f * L) { G.pc_fired = 1; cast_spell(G.spell, aim_target()); G.calm = 0.0f; }
        h = G.pc_t < 0.3f * L ? lerpf(base, top, G.pc_t / (0.3f * L)) : (G.pc_t < 0.7f * L ? top : lerpf(top, base, (G.pc_t - 0.7f * L) / (0.3f * L)));
        if (G.pc_t >= L) G.pc_t = -1.0f;
    }
    if (G.grab_t > 0.0f) { G.grab_t -= dt; h = fmaxf(h, base + (1.0f - base) * sinf(PI_F * clampf(1.0f - G.grab_t, 0.0f, 1.0f))); }
    G.hand = h;
}

/* The feet go the way the wisp went, to its end, pressing E at doors, the exit and the altar's secrets:
 * by the console until a key is touched, or the test player (CRAWL_FOLLOWWISP), who can't be hurt */
static void pilgrim_step(float dt) {
    const Wisp *fw = G.wisps[1].on && G.wisps[1].state != WS_GONE ? &G.wisps[1] : &G.wisps[0];
    float lean;
    if (!fw->on) {
        if (G.pilgrim == 1) { G.pilgrim = 0; say("THE WISP IS GONE, AND THE PULL WITH IT"); }
        return;
    }
#ifdef CRAWL_DEBUG
    if (G.pilgrim == 2) G.hp = G.max_hp;                           /* the test player can't be hurt */
    if (G.pilgrim == 2 && G.altar_wisp == AW_SEEK && G.altar_seek >= 0 && G.secrets[G.altar_seek].state == 2) {   /* testing the altar's */
        const Secret *sc = &G.secrets[G.altar_seek];                /* wisp: in, out, and E at it */
        if (G.altar_inside && !in_secret(G.altar_seek)) { G.ang = atan2f(G.wisps[1].y - G.py, G.wisps[1].x - G.px); IN.hit[KEY_E] = 1; return; }
        float lx = sc->sx + 0.5f, ly = sc->sy + 0.5f, along = (G.px - lx) * DX[sc->d] + (G.py - ly) * DY[sc->d];
        float off = (G.px - lx) * DY[sc->d] + (G.py - ly) * DX[sc->d];   /* across the stone's path, which is the way in */
        float want = G.altar_inside ? -1.0f : 2.0f;                    /* in two cells, or back out to the hall */
        float tx = lx + DX[sc->d] * want, ty = ly + DY[sc->d] * want;
        if (fabsf(off) > 0.08f) { tx = lx + DX[sc->d] * along; ty = ly + DY[sc->d] * along; }   /* in line with it first */
        G.ang = atan2f(ty - G.py, tx - G.px); IN.down[KEY_W] = 1;
        return;
    }
    if (G.pilgrim == 2 && G.altar_wisp == AW_REST && hypotf(G.altar_x + 0.5f - G.px, G.altar_y + 0.5f - G.py) < 1.5f
        && line_clear(G.px, G.py, G.altar_x + 0.5f, G.altar_y + 0.5f)) {
        static float wait;                                          /* at the altar: a look at it resting, then its menu */
        G.ang = atan2f(G.altar_y + 0.5f - G.py, G.altar_x + 0.5f - G.px);
        if ((wait += dt) < 2.0f) return;
        wait = 0.0f; IN.hit[KEY_E] = 1;                             /* and Enter there (update_ui) sends it on */
        return;
    }
#endif
    if (!wisp_lean(fw, &lean)) return;                             /* only just kindled: it hasn't found its way yet */
    G.ang = wrap_pi(G.ang + wrap_pi(lean - G.ang) * fminf(1.0f, dt * 8.0f));
    IN.down[KEY_W] = 1;
    IN.hit[KEY_E] = fmodf((float)G.t, 0.5f) < dt;                  /* doors in the way */
    if (G.pilgrim == 2 && G.altar_x >= 0 && hypotf(G.altar_x + 0.5f - G.px, G.altar_y + 0.5f - G.py) < 2.5f) IN.hit[KEY_E] = 0;   /* not the altar's menu */
    if ((int)G.px == G.exit_fx && (int)G.py == G.exit_fy) { G.ang = heading_angle(G.exit_dir); IN.hit[KEY_E] = 1; }
    for (int i = 0; i < G.nsecrets; i++)                            /* the altar's wisp's secrets */
        if (!G.secrets[i].state && (int)G.px == G.secrets[i].fx && (int)G.py == G.secrets[i].fy) { G.ang = heading_angle(G.secrets[i].d); IN.hit[KEY_E] = 1; }
#ifdef CRAWL_DEBUG
    if (G.pilgrim == 2) {
        if (fmodf((float)G.t, 2.0f) < dt)
            fprintf(stderr, "FOLLOW t=%.1f at (%.2f,%.2f) route=%d goal=(%d,%d) wisp=(%.1f,%.1f) state=%d lean=%.2f\n", G.t, G.px, G.py,
                    fw->field[(int)G.py][(int)G.px], fw->gx, fw->gy, fw->x, fw->y, fw->state, lean);
        G.hp = G.max_hp;
    }
#endif
}

static void update_human(float dt) {
    G.calm += dt;
    if (G.pilgrim == 1) {                                          /* a key touched: the feet are the caster's again */
        int touched = IN.ntext > 0;
        for (int k = 0; k < KEY_COUNT && !touched; k++) touched = IN.hit[k];
        if (touched) { G.pilgrim = 0; say("YOUR FEET ARE YOUR OWN AGAIN"); }
    }
#ifdef CRAWL_DEBUG
    if (getenv("CRAWL_FOLLOWWISP")) G.pilgrim = 2;                 /* testing: go the way the wisp went, to its end */
#endif
    if (G.pilgrim) pilgrim_step(dt);
    spell_keys();
    if (IN.hit[KEY_TAB]) { open_ui(UI_INVENTORY); return; }
    if (IN.hit[KEY_B]) { open_ui(UI_BESTIARY); return; }
    if (IN.hit[KEY_ESC]) { open_ui(UI_QUIT); return; }
    if (IN.hit[KEY_L]) {
        if (RUN.wisp_auto) { G.l_down = 1; G.l_t = 0.0f; }
        else { open_ui(UI_WISPS); return; }
    }
    if (G.l_down) {                                                /* the toggle is on: a tap casts, a hold opens the menu */
        WispChoice c[WISP_CHOICES];
        int n = wisp_choices(c), i = wisp_choice_at(c, n, RUN.wisp_kind, RUN.wisp_color);
        if (IN.down[KEY_L] && (G.l_t += dt) < 0.6f) { }
        else if (IN.down[KEY_L] || i < 0) { G.l_down = 0; open_ui(UI_WISPS); return; }
        else { G.l_down = 0; cast_choice(&c[i]); }
    }
    if (IN.hit[KEY_H]) show_bar();                                 /* a look at where things stand */
    human_walk(dt);
    if (IN.hit[KEY_E] || IN.hit[KEY_SPACE]) human_use();
    if (G.exiting) return;
    if (!IN.down[KEY_MOUSE1]) G.click_spent = 0;
    human_cast(dt, !G.click_spent);
}

/* in the merchant's room: walk up to a ware to see it, E, Space or a click to buy it */
static void shop_human(float dt) {
    spell_keys();
    if (IN.hit[KEY_TAB]) { open_ui(UI_INVENTORY); return; }
    if (IN.hit[KEY_B]) { open_ui(UI_BESTIARY); return; }
    if (IN.hit[KEY_ESC]) { open_ui(UI_QUIT); return; }
    human_walk(dt);
    int was = G.shop_look;
    G.shop_look = -1;
    float bd = 3.2f;
    for (int i = 0; i < G.ped_n; i++) {
        if (G.offer_gone[i]) continue;
        float d = hypotf(G.ped_x[i] - G.px, G.ped_y[i] - G.py);
        if (d < bd && fabsf(wrap_pi(atan2f(G.ped_y[i] - G.py, G.ped_x[i] - G.px) - G.ang)) < 0.3f + 0.25f / d) { bd = d; G.shop_look = i; }
    }
    if (G.shop_look != was) G.shop_confirm = -1;                   /* a warning is for the ware it was given over */
    float sd = hypotf(SHRINE_X - G.px, SHRINE_Y - G.py);
    G.sh_look = G.shop_look < 0 && sd < 3.0f && fabsf(wrap_pi(atan2f(SHRINE_Y - G.py, SHRINE_X - G.px) - G.ang)) < 0.3f + 0.3f / sd;
#ifdef CRAWL_DEBUG
    if (G.shop_look != was) fprintf(stderr, "EVENT shop_look %d t=%.2f at (%.2f,%.2f) ang %.3f\n", G.shop_look, G.t, G.px, G.py, G.ang);
#endif
    int i = G.shop_look;
    if (!IN.down[KEY_MOUSE1]) G.click_spent = 0;
    if (IN.hit[KEY_E] || IN.hit[KEY_SPACE] || IN.hit[KEY_MOUSE1]) {
        if (i >= 0) {
            G.click_spent = IN.hit[KEY_MOUSE1];
            const Offer *o = &G.offers[i];
            int worn = -1;                                         /* buying for a slot already worn */
            if (o->kind == GK_PIECE && o->piece.kind != IK_BAG) {
                int sl = piece_slot(&RUN.gear, o->piece.kind);
                if (RUN.gear.worn[sl].kind != IK_NONE) worn = sl;
            }
            if (G.shop_left <= 0) say("NO MORE WARES THIS VISIT");
            else if (o->price > RUN.treasure) { say("YOU CAN'T AFFORD THAT"); G.cat_ear = 0.32f; G.cat_ear_side = rnd(2); }
            else if (worn >= 0 && !RUN.gear.bag_size && G.shop_confirm != i) { G.shop_confirm = i; snd_sfx_ex(SFX_KNOCK, 0.7f, 0.5f); }   /* the card warns; a second E buys */
            else if (worn >= 0 && RUN.gear.bag_size && !gear_bag_free(&RUN.gear)) { G.shop_letgo = i; G.ui = UI_LETGO; G.ui_sel = 0; snd_sfx_ex(SFX_KEY, 0.7f, 0.5f); }
            else { shop_purchase(i); G.grab_t = 1.0f; }
        } else if (G.sh_look) { open_ui(UI_SHRINE); G.click_spent = IN.hit[KEY_MOUSE1]; G.grab_t = 1.0f; }
        else if (!human_use() && hypotf(G.px - CAT_X, G.py - CAT_Y) < 1.8f) {   /* a pat */
            G.cat_slow = 1.4f; sfx_at(SFX_TRILL, CAT_X, CAT_Y, 0.9f); G.click_spent = IN.hit[KEY_MOUSE1];
        }
    }
    if (IN.hit[KEY_Q] && i >= 0) {                                 /* hold it for the next visit, for a quarter of its price */
        const Offer *o = &G.offers[i];
        int cost = (o->price + 3) / 4;
        if (o->price <= 0) say("IT IS FREE: NOTHING TO HOLD");
        else if (G.shop_held >= 0) say(G.shop_held == i ? "IT IS HELD FOR YOU" : "ONE WARE HELD AT A TIME");
        else if (cost > RUN.treasure) say("HOLDING IT COSTS $%d", cost);
        else {
            RUN.treasure -= cost;
            RUN.gear.held = *o; RUN.gear.has_held = 1; G.shop_held = i;
            say("HELD FOR NEXT TIME: $%d", cost);
            snd_sfx_ex(SFX_COIN, 0.8f, 0.5f); sfx_at(SFX_TRILL, CAT_X, CAT_Y, 0.9f); G.cat_slow = 1.2f;
        }
    }
    human_cast(dt, !G.click_spent);
}

/* the bag is full and a ware is bought for a slot already worn: let something go to make room, or not */
static void update_letgo(void) {
    Gear *g = &RUN.gear;
    int n = g->bag_size;
    if (IN.hit[KEY_ESC] || IN.hit[KEY_TAB]) { G.ui = UI_NONE; G.shop_letgo = -1; say("THOUGHT BETTER OF IT"); snd_sfx_ex(SFX_KEY, 0.6f, 0.5f); return; }
    if (IN.hit[KEY_W] || IN.hit[KEY_UP]) { G.ui_sel = (G.ui_sel + n - 1) % n; snd_sfx_ex(SFX_TICK, 0.6f, 0.5f); }
    if (IN.hit[KEY_S] || IN.hit[KEY_DOWN]) { G.ui_sel = (G.ui_sel + 1) % n; snd_sfx_ex(SFX_TICK, 0.6f, 0.5f); }
    if (IN.hit[KEY_ENTER] || IN.hit[KEY_E] || IN.hit[KEY_SPACE]) {
        int i = G.shop_letgo;
        memset(&g->bag[G.ui_sel], 0, sizeof g->bag[G.ui_sel]);    /* gone, and nothing for it */
        G.ui = UI_NONE; G.shop_letgo = -1;
        if (i >= 0 && !G.offer_gone[i] && G.offers[i].price <= RUN.treasure && G.shop_left > 0) { shop_purchase(i); G.grab_t = 1.0f; }
    }
}

/* the person takes over, from wherever the autopilot was */
static void human_begin(void) {
    G.state = PS_PLAYER; G.pc_t = -1.0f; G.u = 1.0f; G.spd = 0.0f; G.seek_secret = -1;
    G.cx = G.tx = (int)G.px; G.cy = G.ty = (int)G.py;
    if (RUN.gear.atk[G.spell] < 0) G.spell = AT_FIRE;
}

/* back to the autopilot: the merchant's routine picks up where it fits; in the crypt, settle into the grid */
static void human_end(void) {
    G.player = 0; G.ui = UI_NONE; G.status_ret = PLAY_AUTO; G.pc_t = -1.0f;
    if (G.shop) {
        if (G.shop_left <= 0) { G.shop_step = SH_LEAVE; G.shop_t = 0.0; shop_walk(SHOP_DOOR - 0.8f, 15.5f, 1.8f); }
        else { G.shop_step = SH_THINK; G.shop_t = 0.0; shop_turn(face_of(SHOP_MID, 13.0f), 0.5f); }
        return;
    }
    if (G.exiting || G.tally || G.dead) return;
    if (PK.depth) { pocket_return(0); return; }                    /* sleepwalking in a pocket: the feet find the way back out */
    G.cx = G.tx = (int)G.px; G.cy = G.ty = (int)G.py;
    if (!apass(G.cx, G.cy)) {                                      /* onto the nearest lane */
        int bd = 99, bx = G.cx, by = G.cy;
        for (int y = G.cy - 3; y <= G.cy + 3; y++)
            for (int x = G.cx - 3; x <= G.cx + 3; x++) {
                int dd = abs(x - G.cx) + abs(y - G.cy);
                if (apass(x, y) && dd < bd && line_clear(G.px, G.py, x + 0.5f, y + 0.5f)) { bd = dd; bx = x; by = y; }
            }
        G.cx = G.tx = bx; G.cy = G.ty = by;
    }
    G.sx0 = G.px; G.sy0 = G.py;
    int h = ((int)lroundf(wrap_pi(G.ang) / (PI_F * 0.5f)) + 4) & 3;
    G.h = G.next_h = h;
    G.ang0 = G.ang; G.ang1 = G.ang + wrap_pi(heading_angle(h) - G.ang);
    G.state = PS_SETTLE; G.st = 0.0f;
}

/* ---- the inventory (Tab): what is worn, the bag, and the spellbook ---- */

static const char *const SLOT_NAME[NSLOT] = { "LEFT WRIST", "RIGHT WRIST", "BOOTS", "NECK", "HOOD", "ROBES", "RING" };

static int inv_rows(void) { return NSLOT + RUN.gear.bag_size; }
static const Piece *inv_piece(int r) { return r < NSLOT ? &RUN.gear.worn[r] : &RUN.gear.bag[r - NSLOT]; }

/* what Enter would do with the row: 1 into the bag, 2 worn; or why not (as a negative) */
enum { INV_NOTHING = -1, INV_NO_BAG = -2, INV_BAG_FULL = -3, INV_TO_BAG = 1, INV_TO_WEAR = 2 };
static int inv_action(int r) {
    const Piece *p = inv_piece(r);
    if (p->kind == IK_NONE) return INV_NOTHING;
    if (r >= NSLOT) return INV_TO_WEAR;
    if (!RUN.gear.bag_size) return INV_NO_BAG;
    return gear_bag_free(&RUN.gear) ? INV_TO_BAG : INV_BAG_FULL;
}

static void update_inventory(void) {
    spell_keys();
    if (IN.hit[KEY_TAB] || IN.hit[KEY_ESC]) { G.ui = UI_NONE; snd_sfx_ex(SFX_KEY, 0.6f, 0.5f); return; }
    if (IN.hit[KEY_A] || IN.hit[KEY_LEFT] || IN.hit[KEY_D] || IN.hit[KEY_RIGHT]) { G.inv_page ^= 1; snd_sfx_ex(SFX_TICK, 0.8f, 0.5f); }
    if (G.inv_page) return;
    int n = inv_rows();
    if (G.ui_sel >= n) G.ui_sel = n - 1;
    if (IN.hit[KEY_W] || IN.hit[KEY_UP]) { G.ui_sel = (G.ui_sel + n - 1) % n; snd_sfx_ex(SFX_TICK, 0.6f, 0.5f); }
    if (IN.hit[KEY_S] || IN.hit[KEY_DOWN]) { G.ui_sel = (G.ui_sel + 1) % n; snd_sfx_ex(SFX_TICK, 0.6f, 0.5f); }
    if (!(IN.hit[KEY_ENTER] || IN.hit[KEY_E] || IN.hit[KEY_SPACE])) return;
    Gear *g = &RUN.gear;
    int r = G.ui_sel, act = inv_action(r);
    if (act == INV_TO_BAG) {
        for (int i = 0; i < g->bag_size; i++)
            if (g->bag[i].kind == IK_NONE) { g->bag[i] = g->worn[r]; memset(&g->worn[r], 0, sizeof g->worn[r]); break; }
    } else if (act == INV_TO_WEAR) {
        int bi = r - NSLOT, slot = piece_slot(g, g->bag[bi].kind);
        gear_wear(g, slot, bi);
        if (g->worn[slot].ability == AB_KEYMAKER) say("KEYMAKER: KEY WISPS IN THE L MENU");   /* and every time it is put on */
    } else { snd_sfx_ex(SFX_KNOCK, 0.5f, 0.5f); return; }
    G.max_hp = gear_max_hp(g);
    if (G.hp > G.max_hp) G.hp = G.max_hp;
    snd_sfx_ex(SFX_STEP, 1.2f, 0.5f);
}

/* ---- Esc: leave the crypt? ---- */

enum { Q_FLEE, Q_STAY, Q_SLEEP, Q_CARVE, Q_RISE, Q_OPTIONS };
static void open_graves(int mode);
static void quit_choose(int c) {
    if (c == Q_CARVE || c == Q_RISE) { open_graves(c == Q_RISE); return; }
    if (c == Q_FLEE) { G.status_ret = PLAY_QUIT; snd_sfx_ex(SFX_GHOST_DIE, 0.8f, 0.5f); }
    else if (c == Q_STAY) { G.ui = UI_NONE; snd_sfx_ex(SFX_KEY, 0.6f, 0.5f); }
    else { G.ui = UI_NONE; human_end(); snd_sfx_ex(SFX_WHISPER, 0.7f, 0.5f); }
}
static void update_quit(void) {
    if (G.status_ret == PLAY_QUIT) return;
    if (IN.hit[KEY_W] || IN.hit[KEY_UP]) { G.ui_sel = (G.ui_sel + Q_OPTIONS - 1) % Q_OPTIONS; snd_sfx_ex(SFX_TICK, 0.6f, 0.5f); }
    if (IN.hit[KEY_S] || IN.hit[KEY_DOWN]) { G.ui_sel = (G.ui_sel + 1) % Q_OPTIONS; snd_sfx_ex(SFX_TICK, 0.6f, 0.5f); }
    if (IN.hit[KEY_ENTER] || IN.hit[KEY_SPACE] || IN.hit[KEY_E] || IN.hit[KEY_MOUSE1]) { quit_choose(G.ui_sel); return; }
    if (IN.hit[KEY_Y]) { quit_choose(Q_FLEE); return; }
    if (IN.hit[KEY_N] || IN.hit[KEY_ESC]) { quit_choose(Q_STAY); return; }
}

/* where the console's codes and the test switches put the caster */
static void tp_exit(void) { G.px = G.exit_fx + 0.5f; G.py = G.exit_fy + 0.5f; G.ang = heading_angle(G.exit_dir); human_begin(); }
static void tp_secret(int i) {                                     /* three steps back from it, facing it */
    const Secret *sc = &G.secrets[i];
    G.px = sc->fx + 0.5f - DX[sc->d] * 2.5f; G.py = sc->fy + 0.5f - DY[sc->d] * 2.5f; G.ang = heading_angle(sc->d); human_begin();
}
#ifdef CRAWL_DEBUG
static void tp_door(void) {                                        /* a chained room opened, and you before its doorway */
    for (int i = 0; i < G.nsecrets; i++) {
        Secret *sc = &G.secrets[i];
        if (sc->door_x < 0) continue;
        if (!sc->state) { open_secret(i); while (G.slide >= 0) update_slide(0.5f); }
        for (int n = 0; n < 4; n++) {
            int fx = sc->door_x - DX[n], fy = sc->door_y - DY[n];
            if (fx < sc->x0 || fx > sc->x1 || fy < sc->y0 || fy > sc->y1 || G.map[fy][fx].kind != K_FLOOR) continue;
            int bx = fx - DX[n], by = fy - DY[n];
            float back = (bx >= sc->x0 && bx <= sc->x1 && by >= sc->y0 && by <= sc->y1 && G.map[by][bx].kind == K_FLOOR) ? 1.0f : 0.0f;
            G.px = fx + 0.5f - DX[n] * back; G.py = fy + 0.5f - DY[n] * back; G.ang = heading_angle(n); human_begin();
            int k = clampi(atoi(getenv("CRAWL_TPDOOR")), 1, sc->links);   /* 2 on: straight into that link of the chain */
            if (k < 2) return;
            G.cx = fx; G.cy = fy;
            pocket_door(sc->door_x, sc->door_y);
            while (PK.depth < k) pocket_build(PK.depth + 1, 0);
            if (PK.on_x >= 0) { stand_at_door(PK.on_x, PK.on_y); G.ang = wrap_pi(G.ang + PI_F); }   /* facing the way on */
            if (getenv("CRAWL_TPTHING")) {                          /* or a few steps from the k-th thing out of place, facing it */
                int want = atoi(getenv("CRAWL_TPTHING")), seen = 0;
                for (int i = 0; i < G.ndecos; i++) {
                    const Deco *dc = &G.decos[i];
                    if (!(dc->flags & (DF_FLIP | DF_FLOAT | DF_SINK | DF_LOOP)) && dc->spr != SP_DESKS) continue;
                    if (seen++ != want) continue;
                    int tx = (int)dc->x, ty = (int)dc->y;
                    for (int d = 0, done = 0; d < 4 && !done; d++)
                        for (int r = 3; r >= 2 && !done; r--) {
                            int ok = 1;
                            for (int k = 1; k <= r; k++) ok &= G.map[ty + DY[d] * k][tx + DX[d] * k].kind == K_FLOOR;
                            if (!ok) continue;
                            G.px = tx + DX[d] * r + 0.5f; G.py = ty + DY[d] * r + 0.5f; G.ang = heading_angle((d + 2) & 3); done = 1;
                        }
                    fprintf(stderr, "THING %d: sprite %d flags %d at (%.1f,%.1f)\n", want, dc->spr, dc->flags, dc->x, dc->y);
                }
            }
            if (getenv("CRAWL_TPVIS") && G.vis_x >= 0) {            /* or a few steps from the visportal (or cooler), facing it */
                int bd = -1, bfar = 0;
                for (int d = 0; d < 4; d++) {                       /* from the side with the longest clear run */
                    int far = 0;
                    while (far < 3 && G.map[G.vis_y + DY[d] * (far + 1)][G.vis_x + DX[d] * (far + 1)].kind == K_FLOOR) far++;
                    if (far > bfar) { bfar = far; bd = d; }
                }
                if (bd >= 0) {
                    int k = bfar > 1 ? bfar : 1;
                    G.px = G.vis_x + DX[bd] * k + 0.5f; G.py = G.vis_y + DY[bd] * k + 0.5f; G.ang = heading_angle((bd + 2) & 3);
                }
            }
            return;
        }
    }
}
#endif
static int tp_altar(void) {                                        /* before the Black Altar, facing it */
    for (int d = 0; d < 4 && G.altar_x >= 0; d++) {
        int x = G.altar_x + DX[d], y = G.altar_y + DY[d];
        if (!is_open(x, y)) continue;
        int k = vault_of_room(x, y);                               /* in a vault: its door unlocked, as if by its key */
        if (k >= 0 && G.vault_door[k] >= 0 && G.doors[G.vault_door[k]].locked == 1) G.doors[G.vault_door[k]].locked = 2;
        G.px = x + 0.5f; G.py = y + 0.5f; G.ang = heading_angle((d + 2) & 3); human_begin();
        return 1;
    }
    return 0;
}

/* ---- the console (~) ----
 * A plate that drops from the top while you play, as in Quake: a greeting, the date and the hour, and a
 * line to type in. The crypt waits while it is down and its sounds with it; the music plays on. The codes
 * all begin CATA. They are kept here only as hashes (FNV-1a over the word, in capitals), and the list the
 * last of them prints is kept scrambled, so the words can't be read off the source. */
enum { ARG_NONE, ARG_NUM, ARG_OPTNUM, ARG_NAME };
enum { CH_LEVEL, CH_SEED, CH_GEAR, CH_BAG, CH_ABIL, CH_GOLD, CH_SOULS, CH_TPEXIT, CH_TPSECRET, CH_TPALTAR,
       CH_WISP, CH_FOLLOW, CH_RANK, CH_WRAITH, CH_MEET, CH_HELP };
typedef struct { uint32_t hash; uint8_t len, arg, what; } ConCode;
static const ConCode CON_CODES[] = {
    { 0x22375510u,  8, ARG_NUM, CH_LEVEL },
    { 0xFA1F68B3u,  8, ARG_NUM, CH_SEED },
    { 0xA949BC21u,  8, ARG_NUM, CH_GEAR },
    { 0x12387D8Cu,  8, ARG_NUM, CH_BAG },
    { 0x477E6818u,  8, ARG_NUM, CH_ABIL },
    { 0x195BAFB8u,  9, ARG_NONE, CH_GOLD },
    { 0x1653EAEBu, 11, ARG_NONE, CH_SOULS },
    { 0x52F797C1u, 10, ARG_NONE, CH_TPEXIT },
    { 0x914EE69Cu,  9, ARG_OPTNUM, CH_TPSECRET },
    { 0xDB447926u,  9, ARG_NONE, CH_TPALTAR },
    { 0x4A355609u,  8, ARG_NONE, CH_WISP },
    { 0xB6CAFEECu, 11, ARG_NONE, CH_FOLLOW },
    { 0x73316BC4u,  8, ARG_NUM, CH_RANK },
    { 0xB8075035u, 10, ARG_NONE, CH_WRAITH },
    { 0x45627696u,  8, ARG_NAME, CH_MEET },
    { 0xE53746F5u,  8, ARG_NONE, CH_HELP },
};
static const uint8_t CON_HELP[490] = {
    0x19, 0x4C, 0xB8, 0x66, 0xEA, 0x74, 0x65, 0xFB, 0xE2, 0x5B, 0x34, 0xCF, 0xB6, 0xFD, 0x87, 0xA4, 0x64, 0x3D, 0x68, 0xF8,
    0x5E, 0x0D, 0x35, 0x6D, 0xD7, 0x69, 0x44, 0x31, 0x6C, 0x8A, 0x59, 0xD7, 0xBB, 0x7E, 0xC9, 0x02, 0x0A, 0x71, 0x8E, 0xEB,
    0x42, 0x15, 0xE0, 0x47, 0x7F, 0x8A, 0x48, 0x5F, 0x8F, 0x6B, 0x99, 0x9B, 0x3E, 0x27, 0x42, 0x14, 0x7F, 0x65, 0x57, 0xDA,
    0x43, 0xAD, 0x98, 0xED, 0x90, 0x0E, 0x6D, 0x33, 0xAF, 0x25, 0x2F, 0xA6, 0x47, 0x75, 0x1A, 0x0F, 0xF6, 0xD9, 0x4D, 0x65,
    0x2F, 0x0F, 0x25, 0xD7, 0xED, 0xD1, 0xF5, 0x37, 0x9E, 0x45, 0xE5, 0xEB, 0x86, 0x5D, 0x11, 0x86, 0x68, 0x4D, 0x82, 0x8D,
    0xCD, 0xD0, 0x54, 0x4A, 0xF1, 0x34, 0xB7, 0x04, 0x56, 0x57, 0x88, 0x73, 0x2A, 0x3C, 0x3C, 0x55, 0x1F, 0xE6, 0x7C, 0xBB,
    0x23, 0xD0, 0x05, 0x93, 0x0F, 0x7D, 0xA1, 0xC3, 0xEB, 0xAD, 0x38, 0xE8, 0x0E, 0x84, 0xAA, 0x68, 0x03, 0xC1, 0xD5, 0x28,
    0x5F, 0x7F, 0x1C, 0x53, 0xE4, 0xBD, 0x9C, 0x17, 0xBF, 0x83, 0xB9, 0xF7, 0x5B, 0xF1, 0xBD, 0xDF, 0xD7, 0x64, 0xAF, 0x23,
    0x35, 0xE3, 0x2C, 0x86, 0x82, 0x9D, 0x60, 0x1C, 0xAD, 0xE7, 0x7A, 0x85, 0xF5, 0x18, 0xBC, 0xD2, 0x02, 0xF2, 0x1D, 0x05,
    0xDA, 0xC1, 0xB0, 0xFB, 0x92, 0xF4, 0xB4, 0x33, 0xB6, 0x59, 0x08, 0x03, 0x5D, 0x82, 0xE0, 0xA3, 0x64, 0xB2, 0xA1, 0x3F,
    0xC3, 0x9D, 0x95, 0xFD, 0x00, 0x3C, 0xDB, 0xE7, 0xCA, 0xFD, 0xCD, 0x47, 0x0E, 0x21, 0x63, 0xB4, 0x07, 0xA9, 0x77, 0x35,
    0x65, 0xC8, 0x8C, 0x02, 0xFF, 0xAA, 0x1E, 0x42, 0x5D, 0x42, 0xA0, 0xAB, 0x02, 0xA1, 0x3B, 0xEF, 0xA2, 0xD1, 0x6D, 0xF3,
    0xCF, 0xA5, 0xD5, 0xC3, 0xD4, 0x62, 0x91, 0x4F, 0xB3, 0x46, 0x96, 0x1E, 0x85, 0xE2, 0x58, 0x2D, 0x7A, 0x2D, 0xB8, 0x68,
    0x8E, 0x70, 0x00, 0xF8, 0x87, 0x56, 0x46, 0xAA, 0xC2, 0x99, 0xE0, 0xBD, 0x0A, 0x2C, 0x11, 0x84, 0x57, 0x4B, 0x33, 0x7A,
    0xC6, 0x64, 0x25, 0x33, 0x32, 0x88, 0x4A, 0xA3, 0xDA, 0x0D, 0xAC, 0x13, 0x01, 0x71, 0x94, 0x83, 0x27, 0x15, 0xF6, 0x43,
    0x77, 0x9A, 0x23, 0x33, 0x8B, 0x71, 0x88, 0x96, 0x4C, 0x6B, 0x53, 0x1A, 0x66, 0x04, 0x53, 0xD6, 0x55, 0xB9, 0x98, 0x83,
    0xBA, 0x6D, 0x0C, 0x26, 0xA0, 0x51, 0x25, 0xB3, 0x4B, 0x01, 0x74, 0x78, 0x9F, 0xAA, 0x58, 0x1F, 0x4A, 0x1B, 0x2E, 0xB2,
    0xFB, 0x8B, 0xF3, 0x3A, 0x86, 0x24, 0xF4, 0xF6, 0xEA, 0x4E, 0x0A, 0x8A, 0x77, 0x4D, 0xEC, 0xC1, 0xC1, 0xDD, 0x4C, 0x44,
    0xF5, 0x55, 0xA0, 0x07, 0x33, 0x39, 0xFF, 0x1A, 0x59, 0x2D, 0x16, 0x54, 0x1F, 0xF5, 0x11, 0xC9, 0x33, 0xCB, 0x0F, 0xFF,
    0x08, 0x09, 0xD8, 0xC3, 0x88, 0xCC, 0x22, 0xEC, 0x0E, 0x80, 0x8D, 0x1E, 0x6E, 0xB5, 0xA2, 0x4F, 0x51, 0x70, 0x04, 0x37,
    0xEF, 0xD9, 0xB6, 0x74, 0xBF, 0x95, 0xB1, 0xEC, 0x40, 0xE4, 0xAD, 0xAB, 0xAE, 0x69, 0xB8, 0x23, 0x2E, 0xE5, 0x49, 0xE7,
    0x99, 0x83, 0x01, 0x02, 0xB6, 0xFD, 0x18, 0xAF, 0xF8, 0x16, 0xBF, 0x99, 0x09, 0xFC, 0x08, 0x16, 0xDD, 0xA0, 0xDC, 0x97,
    0x92, 0x9D, 0xA4, 0x3F, 0xA6, 0x28, 0x18, 0x60, 0x48, 0x88, 0xED, 0xB3, 0x3B, 0xA3, 0xA5, 0x4B, 0xC0, 0x8C, 0xF4, 0xE6,
    0x02, 0x2A, 0xA8, 0xFD, 0xAB, 0x90, 0xB9, 0x7D, 0x7D, 0x40, 0x64, 0xBA, 0x1A, 0xA0, 0x68, 0x6F, 0x06, 0xA9, 0xF8, 0x63,
    0x9A, 0xB9, 0x04, 0x4E, 0x5D, 0x31, 0xCC, 0xC2, 0x71, 0xA1,
};

static uint32_t fnv1a(const char *s, int n) {
    uint32_t h = 0x811C9DC5u;
    for (int i = 0; i < n; i++) { h ^= (uint8_t)s[i]; h *= 0x01000193u; }
    return h;
}

/* a line into the console, wrapped at word ends to fit */
static void con_print(const char *msg, int ink) {
    for (const char *p = msg; *p; ) {
        int n = (int)strlen(p);
        if (n > 37) { n = 37; while (n > 0 && p[n] != ' ') n--; if (n == 0) n = 37; }
        if (CON.n == CON_LINES) { memmove(CON.line, CON.line + 1, sizeof CON.line[0] * (CON_LINES - 1)); memmove(CON.ink, CON.ink + 1, CON_LINES - 1); CON.n--; }
        memcpy(CON.line[CON.n], p, (size_t)n); CON.line[CON.n][n] = 0; CON.ink[CON.n++] = (uint8_t)ink;
#ifdef CRAWL_DEBUG
        fprintf(stderr, "CONSOLE %s\n", CON.line[CON.n - 1]);
#endif
        p += n; while (*p == ' ') p++;
    }
}
static void con_printf(int ink, const char *fmt, ...) {
    char buf[96];
    va_list ap; va_start(ap, fmt); vsnprintf(buf, sizeof buf, fmt, ap); va_end(ap);
    con_print(buf, ink);
}

/* the day and the hour as the crypt would say them: FRIDAY 23 OCTOBER 2026, A QUARTER PAST TEN AT NIGHT */
static void con_when(char *out, int cap) {
    static const char *const DAY[7] = { "SUNDAY", "MONDAY", "TUESDAY", "WEDNESDAY", "THURSDAY", "FRIDAY", "SATURDAY" };
    static const char *const MONTH[12] = { "JANUARY", "FEBRUARY", "MARCH", "APRIL", "MAY", "JUNE", "JULY", "AUGUST",
                                           "SEPTEMBER", "OCTOBER", "NOVEMBER", "DECEMBER" };
    static const char *const HOUR[12] = { "TWELVE", "ONE", "TWO", "THREE", "FOUR", "FIVE", "SIX", "SEVEN", "EIGHT", "NINE", "TEN", "ELEVEN" };
    static const char *const MIN[12] = { "", "FIVE PAST ", "TEN PAST ", "A QUARTER PAST ", "TWENTY PAST ", "TWENTY-FIVE PAST ", "HALF PAST ",
                                         "TWENTY-FIVE TO ", "TWENTY TO ", "A QUARTER TO ", "TEN TO ", "FIVE TO " };
    time_t now = time(NULL);
    struct tm *tm = localtime(&now);
    if (!tm) { snprintf(out, (size_t)cap, "THE HOUR IS LOST DOWN HERE"); return; }
    int m5 = (tm->tm_min + 2) / 5, h = tm->tm_hour;
    if (m5 == 12) { m5 = 0; h = (h + 1) % 24; }
    int named = (h + (m5 > 6)) % 24;                               /* "to" names the next hour */
    const char *part = named >= 5 && named < 12 ? " IN THE MORNING" : (named >= 12 && named < 17 ? " IN THE AFTERNOON"
                     : (named >= 17 && named < 21 ? " IN THE EVENING" : " AT NIGHT"));
    char hour[48];
    if (m5 == 0 && (named == 0 || named == 12)) snprintf(hour, sizeof hour, "%s", named ? "NOON" : "MIDNIGHT");
    else if (m5 == 0) snprintf(hour, sizeof hour, "%s O'CLOCK%s", HOUR[named % 12], part);
    else snprintf(hour, sizeof hour, "%s%s%s", MIN[m5], HOUR[named % 12], part);
    snprintf(out, (size_t)cap, "%s %d %s %d, %s", DAY[tm->tm_wday % 7], tm->tm_mday, MONTH[tm->tm_mon % 12], tm->tm_year + 1900, hour);
}

static void con_open(void) {
    static const char *const HELLO[] = { "THE CRYPT HEARS YOU TYPING.", "SPEAK, CASTER. THE DEAD ARE LISTENING.",
        "A DRAUGHT FROM BELOW. SOMEONE LEFT THIS OPEN.", "THE BONES WRITE DOWN EVERY WORD.",
        "WELCOME BACK. THE TORCHES HAD BEGUN TO WORRY.", "MIND THE DUST. NOBODY HAS TYPED HERE IN YEARS." };
    char when[96], ver[48];
    CON.open = 1; CON.scroll = 0;
    snd_hold(1);                                                   /* the crypt's sounds wait with it */
    if (CON.n) con_print("", R_STONE + 6);
    con_print(HELLO[CON.opens++ % (int)(sizeof HELLO / sizeof HELLO[0])], R_BONE + 13);
    con_when(when, sizeof when);
    con_print(when, R_STONE + 12);
    snprintf(ver, sizeof ver, "HAUNTED.EXE %s, CATACOMB", HVS_VERSION);
    for (char *c = ver; *c; c++) if (*c >= 'a' && *c <= 'z') *c = (char)(*c - 32);
    con_print(ver, R_GOLD + 11);
}
static void con_close(void) { CON.open = 0; snd_hold(0); }

/* the list the help code prints: unscrambled into lines as it goes */
static void con_help(void) {
    char line[48];
    int n = 0, k = 0x5A, lines = 1;
    con_print("THE CODES. PGUP, PGDN OR THE WHEEL.", R_GOLD + 12);
    for (int i = 0; i <= (int)sizeof CON_HELP; i++) {
        char c = i < (int)sizeof CON_HELP ? (char)(CON_HELP[i] ^ k) : '\n';
        k = (k * 37 + 11) & 0xFF;
        if (c == '\n' || n == 46) { line[n] = 0; con_print(line, R_BONE + 12); lines++; n = 0; }
        else line[n++] = c;
    }
    CON.scroll = lines > CON_ROWS ? lines - CON_ROWS : 0;           /* the list from its top */
}

/* a creature by its name in the bestiary, letters only: CALL then GHOUL, SKELETONKNIGHT ... */
static int con_kind(const char *name) {
    for (int k = 0; k < NKINDS; k++)
        for (int w = 0; w < 2; w++) {
            const char *a = w ? KINDS[k].plural : KINDS[k].name, *b = name;
            if (!a || !*a) continue;
            for (; *a && *b; a++) { if ((*a >= 'A' && *a <= 'Z') || (*a >= '0' && *a <= '9')) { if (*a != *b) break; b++; } }
            while (*a && !((*a >= 'A' && *a <= 'Z') || (*a >= '0' && *a <= '9'))) a++;
            if (!*a && !*b) return k;
        }
    return -1;
}

/* what a code does; 1 if it did it */
static int con_do(int what, int num, const char *rest) {
    static const char *const RANK_SAY[NRANK] = { "AT THEIR OWN RANK", "ROTTEN", "COLD", "BLOODY", "BLACK", "GILDED" };
    int in_crypt = !G.shop && !G.dead && !G.tally && !G.exiting;
    int ok = R_GOLD + 12, no = R_BLOOD + 13;
    switch (what) {
    case CH_LEVEL:
        if (num < 1 || num > 99) { con_print("LEVELS GO FROM 1 TO 99.", no); return 0; }
        G.level = RUN.level = num; RUN.hp = G.hp > 0 ? G.hp : gear_max_hp(&RUN.gear); G.retries = 0; G.ui = UI_NONE;
        begin_shop();
        G.sdur = 0.0f; G.shop_look = -1; G.pc_t = -1.0f;
        con_printf(ok, "DOWN YOU GO: THE MERCHANT, THEN LEVEL %d.", num);
        return 1;
    case CH_SEED:
        G.rs = (uint32_t)num | 1u;
        if (!G.shop) { start_level(); con_printf(ok, "THE STONES SHIFT. LEVEL %d AGAIN, FROM SEED %d.", G.level, num); }
        else con_printf(ok, "THE NEXT LEVEL WILL GROW FROM SEED %d.", num);
        return 1;
    case CH_GEAR:
        if (num < 1 || num > 15) { con_print("TIERS GO FROM 1 TO 15.", no); return 0; }
        give_gear(num);
        G.max_hp = gear_max_hp(&RUN.gear); if (G.hp > G.max_hp) G.hp = G.max_hp;
        con_printf(ok, "EVERY SPELL AT TIER %d, EVERY RUNE %s, AND NEW CLOTHES.", num, QUAL_NAME[rune_from_tier(num) - 1]);
        return 1;
    case CH_BAG:
        if (num < 1 || num > NQUAL) { con_print("QUALITIES GO FROM 1, COMMON, TO 5, EXQUISITE.", no); return 0; }
        give_bag(num - 1);
        con_printf(ok, "A BAG, %s, ALL BUT ONE PLACE FILLED.", QUAL_NAME[num - 1]);
        return 1;
    case CH_ABIL:
        if (num < 1 || num >= NABIL) { con_print("ABILITIES GO FROM 1 TO 7.", no); return 0; }
        if (!give_ability(num)) { con_print("YOU WEAR NOTHING FOR IT TO TEACH.", no); return 0; }
        G.max_hp = gear_max_hp(&RUN.gear); if (G.hp > G.max_hp) G.hp = G.max_hp;
        con_printf(ok, "ALL YOU WEAR KNOWS %s NOW.", ABIL_NAME[num]);
        return 1;
    case CH_GOLD:
        RUN.treasure += 10000; pop_readout();
        con_print("10,000 GOLD. THE MERCHANT WILL BE PLEASED.", ok);
        return 1;
    case CH_SOULS:
        RUN.souls += 100; pop_readout();
        con_print("A HUNDRED SOULS, FROM SOMEWHERE.", ok);
        return 1;
    case CH_RANK:
        if (num < 1 || num > NRANK) { con_print("RANKS GO FROM 1 TO 5, AND 6 FOR GILDED.", no); return 0; }
        CON.rank = num - 1;
        con_printf(ok, "FROM NOW ON THEY COME %s.", RANK_SAY[num - 1]);
        return 1;
    case CH_HELP:
        con_help();
        return 1;
    }
    if (!in_crypt) { con_print("NOT HERE. IN THE CRYPT ITSELF.", no); return 0; }
    switch (what) {
    case CH_TPEXIT:
        if (G.exit_fx < 0) { con_print("THIS LEVEL HAS NO WAY OUT.", no); return 0; }
        tp_exit();
        con_print("YOU STAND BEFORE THE WAY OUT.", ok);
        return 1;
    case CH_TPSECRET: {
        int i = -1;
        if (!G.nsecrets) { con_print("THIS LEVEL KEEPS NO SECRETS.", no); return 0; }
        if (num > 0) {
            if (num > G.nsecrets) { con_printf(no, "THIS LEVEL HAS ONLY %d.", G.nsecrets); return 0; }
            if (G.secrets[num - 1].state) { con_print("THAT ONE IS OPEN ALREADY.", no); return 0; }
            i = num - 1;
        } else for (int k = 0; k < G.nsecrets && i < 0; k++) if (!G.secrets[k].state) i = k;
        if (i < 0) { con_print("EVERY SECRET HERE IS FOUND.", no); return 0; }
        tp_secret(i);
        con_print("A CRACKED WALL, BEFORE YOU.", ok);
        return 1;
    }
    case CH_TPALTAR:
        if (!tp_altar()) { con_print("THERE IS NO ALTAR ON THIS LEVEL.", no); return 0; }
        con_print("THE BLACK ALTAR, BEFORE YOU.", ok);
        return 1;
    case CH_WISP:
        if (!summon_wisp(WK_EXIT, 0, 0.0f)) { con_print("NO WISP WOULD COME.", no); return 0; }
        con_print("A WISP, FOR NOTHING.", ok);
        return 1;
    case CH_FOLLOW:
        if (!G.wisps[0].on && !G.wisps[1].on) { con_print("NO WISP TO FOLLOW. L CASTS ONE.", no); return 0; }
        G.pilgrim = 1;
        con_print("YOUR FEET KNOW THE WAY NOW. ANY KEY TAKES THEM BACK.", ok);
        return 1;
    case CH_WRAITH:
        for (int i = 0; i < MAXMOBS; i++) if (G.mobs[i].alive && G.mobs[i].type == MOB_WRAITH) { con_print("IT IS ALREADY HERE.", no); return 0; }
        G.wraith_now = 1; G.wraith_done = 0; G.wraith_try = 0.0f;
        con_print("SOMETHING COLD IS ON ITS WAY.", ok);
        return 1;
    case CH_MEET: {
        int k = con_kind(rest);
        if (!*rest) { con_print("CALL WHAT? A NAME FROM THE BESTIARY.", no); return 0; }
        if (k < 0) { con_print("THE BESTIARY HAS NO SUCH NAME.", no); return 0; }
        if (!meet_creature(k)) { con_print("NO ROOM AHEAD FOR IT. FACE A LONGER WAY.", no); return 0; }
        con_printf(ok, "A %s, AHEAD.", KINDS[k].name);
        return 1;
    }
    }
    return 0;
}

/* a word typed and Enter: which code, if any, and what follows it */
static void con_run(const char *w) {
    int n = (int)strlen(w);
    for (int i = 0; i < (int)(sizeof CON_CODES / sizeof CON_CODES[0]); i++) {
        const ConCode *c = &CON_CODES[i];
        if (n < c->len || fnv1a(w, c->len) != c->hash) continue;
        const char *rest = w + c->len;
        int num = 0;
        if (c->arg == ARG_NONE && *rest) continue;
        if (c->arg == ARG_NUM || c->arg == ARG_OPTNUM) {
            int digits = 1;
            for (const char *d = rest; *d; d++) digits &= *d >= '0' && *d <= '9';
            if (!digits || strlen(rest) > 9) { con_print("THE CRYPT KNOWS THE WORD, NOT WHAT FOLLOWS IT.", R_BLOOD + 13); return; }
            if (!*rest && c->arg == ARG_NUM) { con_print("THAT WORD WANTS A NUMBER AFTER IT.", R_BLOOD + 13); return; }
            num = atoi(rest);
        }
        if (con_do(c->what, num, rest) && c->what != CH_HELP && !CON.used) {
            CON.used = 1;                                          /* the price, until the program is closed */
            con_print("THE DEAD TAKE NOTE. NO HONOURS AND NO GRAVES WHILE THIS LASTS.", R_BLOOD + 12);
        }
#ifdef CRAWL_DEBUG
        fprintf(stderr, "EVENT console code %d %d t=%.2f\n", c->what, num, G.t);
#endif
        return;
    }
    con_print("THE CRYPT DOES NOT KNOW THAT WORD.", R_BLOOD + 13);
}

/* 1 while the console is down and the crypt should wait */
static int con_update(float dt) {
    CON.t += dt;
    CON.slide = CON.open ? fminf(1.0f, CON.slide + dt / 0.25f) : fmaxf(0.0f, CON.slide - dt / 0.25f);
    if (CON.open && !G.player) con_close();
    if (!G.player) return 0;
    if (!CON.open) {
        if (IN.hit[KEY_CONSOLE] && !G.ui) { con_open(); return 1; }
        return 0;
    }
    if (IN.hit[KEY_CONSOLE] || IN.hit[KEY_ESC]) { con_close(); return 1; }
    for (int i = 0; i < IN.ntext && CON.ntyped < 24; i++) CON.typed[CON.ntyped++] = IN.text[i];
    CON.typed[CON.ntyped] = 0;
    if (IN.hit[KEY_BACK] && CON.ntyped) CON.typed[--CON.ntyped] = 0;
    if (IN.hit[KEY_UP] && CON.last[0]) { memcpy(CON.typed, CON.last, sizeof CON.typed); CON.ntyped = (int)strlen(CON.typed); }
    int most = CON.n > CON_ROWS ? CON.n - CON_ROWS : 0;
    if (IN.hit[KEY_PGUP] || IN.wheel > 0) CON.scroll = clampi(CON.scroll + (IN.wheel > 0 ? IN.wheel : CON_ROWS - 1), 0, most);
    if (IN.hit[KEY_PGDN] || IN.wheel < 0) CON.scroll = clampi(CON.scroll - (IN.wheel < 0 ? -IN.wheel : CON_ROWS - 1), 0, most);
    if (IN.hit[KEY_ENTER] && CON.ntyped) {
        char w[25];
        memcpy(w, CON.typed, sizeof w);
        memcpy(CON.last, w, sizeof w);
        CON.ntyped = 0; CON.typed[0] = 0; CON.scroll = 0;
        con_printf(R_STONE + 11, "> %s", w);
        con_run(w);
    }
    return 1;
}

/* the plate: black, an iron rim below, five rows and the one you type in */
static void con_render(Frame *f) {
    if (CON.slide <= 0.0f) return;
    int ph = CON_ROWS * 10 + 22, top = (int)(-ph * (1.0f - smoothstepf(0.0f, 1.0f, CON.slide)));
    for (int y = 0; y < ph; y++) {
        int yy = top + y;
        if (yy < 0 || yy >= H) continue;
        uint8_t c = y == ph - 3 ? (uint8_t)(R_STONE + 11) : (y == ph - 2 ? (uint8_t)(R_IRON + 6) : (y == ph - 1 ? (uint8_t)(R_IRON + 2) : 0));
        memset(&f->pix[yy * W], c, W);
    }
    int first = CON.n - CON_ROWS - CON.scroll;
    for (int r = 0; r < CON_ROWS; r++) {
        int i = first + r;
        if (i >= 0 && i < CON.n) gfx_text8(f, 6, top + 4 + r * 10, CON.line[i], CON.ink[i], 1);
    }
    if (first > 0) gfx_text8(f, W - 10, top + 4, "\x18", R_GOLD + 10, 1);                     /* more above */
    if (CON.scroll > 0) gfx_text8(f, W - 10, top + 4 + (CON_ROWS - 1) * 10, "\x19", R_GOLD + 10, 1);   /* and below */
    int iy = top + 4 + CON_ROWS * 10 + 3;
    for (int x = 6; x < W - 6; x++) gfx_px(f, x, iy - 3, R_STONE + 4);
    gfx_text8(f, 6, iy, ">", R_GOLD + 12, 1);
    gfx_text8(f, 18, iy, CON.typed, R_BONE + 14, 1);
    if (fmodf(CON.t, 0.8f) < 0.45f) gfx_rect(f, 18 + CON.ntyped * 8, iy + 6, 7, 2, R_BONE + 14);   /* the cursor */
}

/* ---- the scene's side of the bargain with the app ---- */

static void crawl_input(const Input *in, double dt) { (void)dt; IN = *in; }

static void crawl_control(int what) {
    if (what == CTRL_PAUSE) {
        if (CON.open) { CON.open = 0; snd_hold(0); }
        if (G.player && !G.ui && G.status_ret != PLAY_QUIT) open_ui(UI_QUIT);
        return;
    }
    if (what == CTRL_PEEK) { if (!G.player) { pop_readout(); G.peek = 0.15f; } return; }
    show_bar();
    G.player = 1; G.status_ret = PLAY_PLAYER; G.ui = UI_NONE;
    if (!G.quit_line) G.quit_line = 1 + rnd(97);                    /* the crypt's arguments, from somewhere new each time */
    if (G.shop) { G.sdur = 0.0f; G.shop_look = -1; G.pc_t = -1.0f; }   /* no cast left half-made */
    else if (!G.exiting && !G.tally && !G.dead) human_begin();
    G.hint_t = 6.0f;
    snd_sfx_ex(SFX_RUNE, 1.0f, 0.5f); snd_sfx_ex(SFX_WHOOSH, 0.6f, 0.5f);
    say("THE CONTROLS ARE YOURS");
#ifdef CRAWL_DEBUG
    if (getenv("CRAWL_TPEXIT") && !G.shop && G.exit_fx >= 0) tp_exit();    /* testing: straight to the exit, facing it */
    if (getenv("CRAWL_TPSECRET") && !G.shop && G.nsecrets) tp_secret(atoi(getenv("CRAWL_TPSECRET")) % G.nsecrets);
    if (getenv("CRAWL_TPALTAR") && !G.shop) tp_altar();
    if (getenv("CRAWL_TPDOOR") && !G.shop) tp_door();              /* before a secret's doorway, its room open */
    if (getenv("CRAWL_TPBOSS") && !G.shop && G.boss >= 0 && G.bstart_x >= 0) {   /* in the boss's antechamber, facing the arch */
        G.px = G.bstart_x + 0.5f; G.py = G.bstart_y + 0.5f; G.ang = 0.0f; human_begin();
    }
    fprintf(stderr, "EVENT control t=%.2f at (%.2f,%.2f) ang %.3f shop %d pocket %d\n", G.t, G.px, G.py, G.ang, G.shop, PK.depth);
#endif
}

static int crawl_status(void) { return G.player ? G.status_ret : PLAY_AUTO; }

/* What a save keeps: the level to start from, health, gold, souls and the gear, and since version 2
 * the run's record (kills, secrets, levels cleared, achievements). Not where you stood: you come back
 * to the start of the level, so its gold and souls are taken back out. Version 3 packs the gear field
 * by field (what is worn, the bag, the bones, a ware held at the merchant's) and keeps souls in
 * tenths. Version 4 adds four bytes after the record: the wisp L casts and the menu's toggle. Version
 * 5 keeps runes by quality, not tier, and adds 32 bytes for the Rune Shrine: the coins, its rank, its
 * attunements and its nine slots. Older saves' runes come by their tier (rune_from_tier), and a rune
 * held at the merchant's gives back what holding it cost. Version 6 (runs of ten) adds the rest of the
 * record: levels cleared to 40, and room for more achievements; older records' descents of twenty
 * become descents of forty (rec_from_old). Versions 1 and 2 still load: their spells and runes stay,
 * their robe, hood and best ring become Common pieces of their tier, and version 1 has an empty record. */
#define SAVE_MAGIC 0x31535648u              /* "HVS1" */
#define OLD_GEAR 46                         /* versions 1 and 2: the gear as it was, copied raw */
#define SAVE_V1 (28 + OLD_GEAR)
#define PIECE_BYTES (6 + 3 * PIECE_STATS)
#define GEAR_BYTES (NATK + NDEF + NSLOT * PIECE_BYTES + 2 + BAG_MAX * PIECE_BYTES + 2 + 1 + 8 + PIECE_BYTES)
#define SAVE_V3 (28 + GEAR_BYTES)
static void put32(uint8_t *p, int32_t v) { for (int i = 0; i < 4; i++) p[i] = (uint8_t)((uint32_t)v >> (8 * i)); }
static int32_t get32(const uint8_t *p) { uint32_t v = 0; for (int i = 0; i < 4; i++) v |= (uint32_t)p[i] << (8 * i); return (int32_t)v; }
static void put16(uint8_t *p, int v) { p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8); }
static int get16(const uint8_t *p) { return p[0] | p[1] << 8; }

static void piece_pack(uint8_t *p, const Piece *pc) {
    p[0] = pc->kind; p[1] = pc->quality; p[2] = pc->level; p[3] = pc->anointed; p[4] = pc->ability; p[5] = pc->nstat;
    for (int i = 0; i < PIECE_STATS; i++) { p[6 + i] = pc->stat[i]; put16(p + 6 + PIECE_STATS + 2 * i, pc->val[i]); }
}
static Piece piece_unpack(const uint8_t *p) {                       /* anything out of range comes back as nothing */
    Piece pc;
    memset(&pc, 0, sizeof pc);
    if (p[0] >= NIK || p[1] >= NQUAL || p[4] >= NABIL || p[5] > PIECE_STATS) return pc;
    for (int i = 0; i < p[5]; i++) if (p[6 + i] >= NSTAT) return pc;
    pc.kind = p[0]; pc.quality = p[1]; pc.level = p[2]; pc.anointed = p[3] != 0; pc.ability = p[4]; pc.nstat = p[5];
    for (int i = 0; i < PIECE_STATS; i++) { pc.stat[i] = p[6 + i]; pc.val[i] = (int16_t)get16(p + 6 + PIECE_STATS + 2 * i); }
    return pc;
}
static void gear_pack(uint8_t *p, const Gear *g) {
    for (int i = 0; i < NATK; i++) *p++ = (uint8_t)g->atk[i];
    for (int i = 0; i < NDEF; i++) *p++ = (uint8_t)g->def[i];
    for (int s = 0; s < NSLOT; s++, p += PIECE_BYTES) piece_pack(p, &g->worn[s]);
    *p++ = g->bag_size; *p++ = g->bag_quality;
    for (int i = 0; i < BAG_MAX; i++, p += PIECE_BYTES) piece_pack(p, &g->bag[i]);
    put16(p, g->bones); p += 2;
    *p++ = g->has_held;
    p[0] = (uint8_t)g->held.kind; p[1] = (uint8_t)g->held.id; p[2] = (uint8_t)g->held.tier; p[3] = (uint8_t)g->held.rare;
    put32(p + 4, g->held.price); p += 8;
    piece_pack(p, &g->held.piece);
}
static void gear_unpack(const uint8_t *p, Gear *g, int by_quality) {   /* runes by quality from version 5, by tier before */
    static const int SLOT_IK[NSLOT] = { IK_WRIST, IK_WRIST, IK_BOOTS, IK_NECK, IK_HOOD, IK_ROBE, IK_RING };
    gear_reset(g);
    for (int i = 0; i < NATK; i++) g->atk[i] = (int8_t)clampi((int8_t)*p++, -1, 15);
    if (g->atk[AT_FIRE] < 0) g->atk[AT_FIRE] = 0;
    for (int i = 0; i < NDEF; i++, p++) g->def[i] = (int8_t)(by_quality ? clampi((int8_t)*p, 0, RUNE_EXQ) : rune_from_tier(clampi((int8_t)*p, 0, 15)));
    for (int s = 0; s < NSLOT; s++, p += PIECE_BYTES) {
        g->worn[s] = piece_unpack(p);
        if (g->worn[s].kind != SLOT_IK[s]) memset(&g->worn[s], 0, sizeof g->worn[s]);   /* only what goes there */
    }
    g->bag_size = (uint8_t)clampi(p[0], 0, BAG_MAX); g->bag_quality = (uint8_t)clampi(p[1], 0, NQUAL - 1); p += 2;
    for (int i = 0; i < BAG_MAX; i++, p += PIECE_BYTES) if (i < g->bag_size) g->bag[i] = piece_unpack(p);
    g->bones = (int16_t)clampi(get16(p), 0, BONES_MAX); p += 2;
    g->has_held = *p++ != 0;
    g->held.kind = p[0]; g->held.id = p[1]; g->held.tier = p[2]; g->held.rare = p[3] != 0; g->held.price = get32(p + 4); p += 8;
    g->held.piece = piece_unpack(p);
    if (g->held.kind > GK_PIECE || (g->held.kind == GK_ATK && g->held.id >= NATK) || (g->held.kind == GK_DEF && g->held.id >= NDEF) || g->held.price < 0)
        g->has_held = 0;
    if (!g->has_held) memset(&g->held, 0, sizeof g->held);
}
/* versions 1 and 2: the spellbook as it was; the robe, hood and best ring as Common pieces of their tier */
static void gear_from_old(const uint8_t *p, Gear *g, uint32_t seed) {
    static const int ROBE_STAT[6] = { -1, ST_ARMOR, ST_RES_POISON, ST_ARMOR, ST_ARMOR, ST_REGEN };   /* plain, silk, shadow, thorned, blessed */
    static const int HOOD_STAT[5] = { -1, ST_ARMOR, ST_ARMOR, ST_EFFECT, ST_HP };                   /* plain, warded, hunter's, cowl */
    uint32_t rs = seed | 1u;
    gear_reset(g);
    for (int i = 0; i < NATK; i++) g->atk[i] = (int8_t)clampi((int8_t)p[i], -1, 15);
    if (g->atk[AT_FIRE] < 0) g->atk[AT_FIRE] = 0;
    for (int i = 0; i < NDEF; i++) g->def[i] = (int8_t)rune_from_tier(clampi((int8_t)p[NATK + i], 0, 15));
    int robe = (int8_t)p[18], robe_t = (int8_t)p[19], hood = (int8_t)p[20], hood_t = (int8_t)p[21];
    if (robe > 0 && robe < 6 && robe_t > 0) g->worn[SL_ROBE] = piece_make(IK_ROBE, robe_t, ROBE_STAT[robe], &rs);
    if (hood > 0 && hood < 5 && hood_t > 0) g->worn[SL_HOOD] = piece_make(IK_HOOD, hood_t, HOOD_STAT[hood], &rs);
    int best = 0;
    for (int f = 0; f < 4; f++) if ((int8_t)p[22 + f] >= 0 && (int8_t)p[26 + f] > best) best = (int8_t)p[26 + f];
    if (best > 0) g->worn[SL_RING] = piece_make(IK_RING, best, ST_DAMAGE, &rs);
}
/* the record, little-endian, field by field: the shape it has had since version 2, then (version 6)
 * the rest of the levels cleared and of the achievements */
static void rec_pack(uint8_t *p, const Record *r) {
    for (int k = 0; k < REC_KINDS; k++) for (int i = 0; i < NRANK; i++, p += 2) put16(p, r->kills[k][i]);
    memcpy(p, r->spell_kills, sizeof r->spell_kills); p += sizeof r->spell_kills;
    memcpy(p, r->seen, sizeof r->seen); p += sizeof r->seen;
    put16(p, r->secrets); put16(p + 2, r->descents); put32(p + 4, (int32_t)(uint32_t)r->cleared); p += 8;
    memcpy(p, r->got, 256 / 8);
}
static void rec_unpack(const uint8_t *p, Record *r) {
    for (int k = 0; k < REC_KINDS; k++) for (int i = 0; i < NRANK; i++, p += 2) r->kills[k][i] = (uint16_t)get16(p);
    memcpy(r->spell_kills, p, sizeof r->spell_kills); p += sizeof r->spell_kills;
    memcpy(r->seen, p, sizeof r->seen); p += sizeof r->seen;
    r->secrets = (uint16_t)get16(p); r->descents = (uint16_t)get16(p + 2); r->cleared = (uint32_t)get32(p + 4); p += 8;
    memcpy(r->got, p, 256 / 8);
}
static void rec_pack_extra(uint8_t *p, const Record *r) {
    put32(p, (int32_t)(uint32_t)(r->cleared >> 32)); p += 4;
    memcpy(p, r->got + 256 / 8, (ACH_BITS - 256) / 8); p += (ACH_BITS - 256) / 8;
    for (int b = 0; b < BOSS_REC; b++, p += 9) { p[0] = r->boss[b].met; put16(p + 1, r->boss[b].slain); put16(p + 3, r->boss[b].tries); put16(p + 5, r->boss[b].best); put16(p + 7, r->boss[b].scrip); }
}
static void rec_unpack_extra(const uint8_t *p, Record *r) {
    r->cleared |= (uint64_t)(uint32_t)get32(p) << 32; p += 4;
    memcpy(r->got + 256 / 8, p, (ACH_BITS - 256) / 8); p += (ACH_BITS - 256) / 8;
    for (int b = 0; b < BOSS_REC; b++, p += 9) { r->boss[b].met = p[0] != 0; r->boss[b].slain = (uint16_t)get16(p + 1); r->boss[b].tries = (uint16_t)get16(p + 3); r->boss[b].best = (uint16_t)get16(p + 5); r->boss[b].scrip = (uint16_t)get16(p + 7); }
}
/* version 6's hall, as far as it is built: the runs completed, each boss's scrip and challenge step, the
 * ranks met since the last death */
#define SAVE_V6_HALL (2 + 2 * BOSS_REC + BOSS_REC + REC_KINDS + 1)
static void hall_pack(uint8_t *p) {
    put16(p, clampi(RUN.runs_done, 0, 65535)); p += 2;
    for (int b = 0; b < BOSS_REC; b++, p += 2) put16(p, clampi(RUN.scrip[b], 0, 65535));
    for (int b = 0; b < BOSS_REC; b++) *p++ = (uint8_t)clampi(RUN.bstep[b], 0, 5);
    memcpy(p, RUN.top_rank, REC_KINDS); p += REC_KINDS;
    *p = (uint8_t)RUN.omens;
}
static void hall_unpack(const uint8_t *p) {
    RUN.runs_done = get16(p); p += 2;
    for (int b = 0; b < BOSS_REC; b++, p += 2) RUN.scrip[b] = get16(p);
    for (int b = 0; b < BOSS_REC; b++) RUN.bstep[b] = clampi(*p++, 0, 5);
    for (int k = 0; k < REC_KINDS; k++) RUN.top_rank[k] = (uint8_t)clampi(p[k], 0, NRANK);
    RUN.omens = p[REC_KINDS] & ((1 << NHZ) - 1);
}
/* before version 6 a descent was twenty levels: the 40th, 80th ... stay as the descents they are now
 * (the 40th as level 40 cleared), and the rest are let go */
static void rec_from_old(Record *r) {
    uint8_t old[19];
    for (int k = 2; k <= MAX_DESCENT; k++) { int b = ACH_DESCENT0 + k - 2; old[k - 2] = r->got[b >> 3] >> (b & 7) & 1; r->got[b >> 3] &= (uint8_t)~(1 << (b & 7)); }
    for (int k = 2; k <= MAX_DESCENT; k++) {
        if (!old[k - 2] || (k * 20) % DESCENT_LEN) continue;
        int nk = k * 20 / DESCENT_LEN, b = nk == 1 ? ach_level(DESCENT_LEN) : ACH_DESCENT0 + nk - 2;
        r->got[b >> 3] |= (uint8_t)(1 << (b & 7));
    }
    r->descents /= 2;
}
#define SAVE_V4_EXTRA 4                     /* version 4: the wisp L casts, and the toggle */
#define SAVE_V5_SHRINE 32                   /* version 5: Cursed Coins and the Rune Shrine */
#define SAVE_V6_EXT (REC_EXTRA + SAVE_V6_HALL)   /* version 6: runs of ten, the bosses (docs/PLAN-b7.md) */
static int save_version(const uint8_t *buf, int len) {             /* 1 to 6 if it is a save we can read, else 0 */
    if (len < SAVE_V1 || (uint32_t)get32(buf) != SAVE_MAGIC) return 0;
    int v = get32(buf + 4), v5 = SAVE_V3 + REC_BYTES + SAVE_V4_EXTRA + SAVE_V5_SHRINE;
    if (v == 1 || (v == 2 && len >= SAVE_V1 + REC_BYTES) || (v == 3 && len >= SAVE_V3 + REC_BYTES)
        || (v == 4 && len >= SAVE_V3 + REC_BYTES + SAVE_V4_EXTRA) || (v == 5 && len >= v5) || (v == 6 && len >= v5 + SAVE_V6_EXT)) return v;
    return 0;
}
static int save_end(int v) {                                       /* a grave's time follows */
    return (v >= 3 ? SAVE_V3 : SAVE_V1) + (v >= 2 ? REC_BYTES : 0) + (v >= 4 ? SAVE_V4_EXTRA : 0) + (v >= 5 ? SAVE_V5_SHRINE : 0) + (v >= 6 ? SAVE_V6_EXT : 0);
}
/* the shrine's 32 bytes: coins (u16), rank, attunements (u16, a bit a rune), then the slots as rune
 * kind + 1 and quality, then the level whose coins were last taken (u16) and which; the rest spare */
static void shrine_pack(uint8_t *p) {
    memset(p, 0, SAVE_V5_SHRINE);
    put16(p, clampi(RUN.coins, 0, 65535)); p[2] = (uint8_t)RUN.shrine; put16(p + 3, RUN.attuned);
    for (int i = 0; i < SHRINE_SLOTS; i++) { p[5 + 2 * i] = RUN.slot_def[i]; p[6 + 2 * i] = RUN.slot_q[i]; }
    put16(p + 23, clampi(RUN.coin_level, 0, 65535)); p[25] = (uint8_t)RUN.coin_bits;   /* the coins taken on that level */
}
static void shrine_unpack(const uint8_t *p) {                      /* anything out of range comes back empty */
    RUN.coins = get16(p); RUN.shrine = clampi(p[2], 0, NQUAL); RUN.attuned = get16(p + 3) & ((1 << NDEF) - 1);
    for (int i = 0; i < SHRINE_SLOTS; i++) {
        int d = p[5 + 2 * i], q = p[6 + 2 * i];
        if (d < 1 || d > NDEF || q < 1 || q > RUNE_EXQ) d = q = 0;
        RUN.slot_def[i] = (uint8_t)d; RUN.slot_q[i] = (uint8_t)q;
    }
    RUN.coin_level = get16(p + 23); RUN.coin_bits = p[25];
}

static int crawl_save(uint8_t *buf, int cap) {
    int n = save_end(6);
    if (cap < n || CON.used) return 0;                             /* the dead took note: nothing is kept */
    int level = G.level, hp = G.dead || G.hp <= 0 ? gear_max_hp(&RUN.gear) : G.hp, gold = RUN.treasure, souls = RUN.souls, shop = 0;
    if (G.tally || G.exiting) { level = G.level + 1; shop = 1; }   /* the level is won */
    else if (G.shop) shop = !G.shop_bought;
    else { gold -= G.level_treasure; souls -= G.level_souls; }
    put32(buf, (int32_t)SAVE_MAGIC);
    put32(buf + 4, 6);                                             /* version */
    put32(buf + 8, level); put32(buf + 12, hp); put32(buf + 16, gold); put32(buf + 24, shop);
    put32(buf + 20, souls * 10 + clampi((int)(RUN.soul_frac * 10.0f), 0, 9));   /* spirit, in tenths */
    gear_pack(buf + 28, &RUN.gear);
    rec_pack(buf + SAVE_V3, &RUN.rec);
    uint8_t *x = buf + SAVE_V3 + REC_BYTES;
    x[0] = (uint8_t)RUN.wisp_kind; x[1] = (uint8_t)RUN.wisp_color; x[2] = (uint8_t)(RUN.wisp_auto != 0); x[3] = 0;
    shrine_pack(x + SAVE_V4_EXTRA);
    rec_pack_extra(x + SAVE_V4_EXTRA + SAVE_V5_SHRINE, &RUN.rec);
    hall_pack(x + SAVE_V4_EXTRA + SAVE_V5_SHRINE + REC_EXTRA);
    return n;
}

static void crawl_load(const uint8_t *buf, int len) {
    int v = save_version(buf, len);
    if (!v) return;
    memset(&RUN, 0, sizeof RUN);
    RUN.active = 1; RUN.loaded = 1;
    if (v >= 2) rec_unpack(buf + (v >= 3 ? SAVE_V3 : SAVE_V1), &RUN.rec);
    TOASTS.n = 0;                                                  /* another run's news is no news */
    RUN.level = clampi(get32(buf + 8), 1, 999); RUN.treasure = get32(buf + 16); RUN.souls = get32(buf + 20);
    RUN.shop_next = get32(buf + 24) != 0;
    if (v >= 3) { gear_unpack(buf + 28, &RUN.gear, v >= 5); RUN.soul_frac = (float)(RUN.souls % 10) / 10.0f; RUN.souls /= 10; }
    else gear_from_old(buf + 28, &RUN.gear, (uint32_t)RUN.level * 2654435761u);
    RUN.hp = clampi(get32(buf + 12), 1, gear_max_hp(&RUN.gear));
    if (v >= 4) {
        const uint8_t *x = buf + SAVE_V3 + REC_BYTES;
        RUN.wisp_kind = clampi(x[0], WK_EXIT, WK_ALTAR); RUN.wisp_color = clampi(x[1], 0, NKEYS - 1); RUN.wisp_auto = x[2] != 0;
        if (v >= 5) shrine_unpack(x + SAVE_V4_EXTRA);
        if (v >= 6) { rec_unpack_extra(x + SAVE_V4_EXTRA + SAVE_V5_SHRINE, &RUN.rec); hall_unpack(x + SAVE_V4_EXTRA + SAVE_V5_SHRINE + REC_EXTRA); }
    }
    if (v < 6) rec_from_old(&RUN.rec);
    if (RUN.gear.has_held && RUN.gear.held.kind == GK_DEF) {        /* the merchant sells no runes now: the hold is paid back */
        RUN.treasure += (RUN.gear.held.price + 3) / 4;
        RUN.gear.has_held = 0; memset(&RUN.gear.held, 0, sizeof RUN.gear.held);
    }
    if (RUN.treasure < 0) RUN.treasure = 0;
    if (RUN.souls < 0 || RUN.soul_frac < 0.0f) { RUN.souls = RUN.souls < 0 ? 0 : RUN.souls; RUN.soul_frac = 0.0f; }
}

/* ---- the graves: three save slots, carved and risen from the Esc menu ---- */

/* A grave holds what fleeing to the surface keeps (the level to start from, health, gold, souls and
 * gear), then when it was carved. */
typedef struct { int used, level, hp, gold, souls; int64_t when; } Grave;
static const char *const ROMAN[SLOT_COUNT] = { "I", "II", "III" };

static Grave grave_info(int i) {
    Grave g;
    memset(&g, 0, sizeof g);
    uint8_t buf[SLOT_MAX];
    int n = slot_read(i, buf, sizeof buf), v = save_version(buf, n), base = save_end(v);
    if (!v) return g;
    g.used = 1; g.level = get32(buf + 8); g.hp = get32(buf + 12); g.gold = get32(buf + 16); g.souls = get32(buf + 20) / (v >= 3 ? 10 : 1);
    if (n >= base + 8) g.when = (int64_t)((uint64_t)(uint32_t)get32(buf + base) | (uint64_t)(uint32_t)get32(buf + base + 4) << 32);
    return g;
}

static void open_graves(int mode) {
    G.ui = UI_GRAVES; G.graves_mode = mode; G.graves_arm = -1; G.graves_msg = 0; G.ui_sel = 0;
    int64_t newest = -1;
    for (int i = SLOT_COUNT - 1; i >= 0; i--) {
        Grave g = grave_info(i);
        if (!mode && !g.used) G.ui_sel = i;                        /* carving: the first empty grave */
        if (mode && g.used && g.when >= newest) { newest = g.when; G.ui_sel = i; }   /* rising: the newest */
    }
    snd_sfx_ex(SFX_KEY, 0.7f, 0.5f);
}

static void carve_grave(int i) {
    uint8_t buf[SLOT_MAX];
    if (CON.used) { G.ui = UI_NONE; say("NO GRAVES WHILE THE DEAD TAKE NOTE"); snd_sfx_ex(SFX_KNOCK, 0.6f, 0.5f); return; }
    int n = crawl_save(buf, SLOT_MAX - 8);
    if (n <= 0) return;
    uint64_t now = (uint64_t)(int64_t)time(NULL);
    put32(buf + n, (int32_t)(uint32_t)(now & 0xffffffffu)); put32(buf + n + 4, (int32_t)(uint32_t)(now >> 32));
    slot_write(i, buf, n + 8);
    G.ui = UI_NONE;
    say("CARVED INTO GRAVE %s", ROMAN[i]);
    snd_sfx_ex(SFX_KNOCK, 1.0f, 0.5f); snd_sfx_ex(SFX_RUNE, 0.6f, 0.5f);
}

/* the grave's run comes back at the start of its level, still in your hands */
static void rise_from_grave(int i) {
    uint8_t buf[SLOT_MAX];
    int n = slot_read(i, buf, sizeof buf), ql = G.quit_line;
    uint32_t seed = G.rs * 1664525u + 1013904223u;
    crawl_load(buf, n);
    crawl_enter(seed);
    G.player = 1; G.status_ret = PLAY_PLAYER; G.ui = UI_NONE; G.quit_line = ql;
    if (G.shop) { G.sdur = 0.0f; G.shop_look = -1; G.pc_t = -1.0f; }   /* no cast left half-made */
    else human_begin();
    say("RISEN FROM GRAVE %s: LEVEL %d", ROMAN[i], G.level);
    snd_sfx_ex(SFX_WHOOSH, 0.8f, 0.5f); snd_sfx_ex(SFX_WHISPER, 0.8f, 0.5f);
}

enum { GM_NONE, GM_ARMED, GM_EMPTY };
static void update_graves(void) {
    if (IN.hit[KEY_ESC] || IN.hit[KEY_TAB] || IN.hit[KEY_N]) {        /* back to the question */
        G.ui = UI_QUIT; G.ui_sel = G.graves_mode ? Q_RISE : Q_CARVE; snd_sfx_ex(SFX_KEY, 0.6f, 0.5f);
        return;
    }
    int mv = (IN.hit[KEY_S] || IN.hit[KEY_DOWN]) - (IN.hit[KEY_W] || IN.hit[KEY_UP]), pick = -1;
    for (int k = 0; k < SLOT_COUNT; k++) if (IN.hit[KEY_1 + k]) pick = k;
    if (mv || pick >= 0) {
        G.ui_sel = pick >= 0 ? pick : (G.ui_sel + mv + SLOT_COUNT) % SLOT_COUNT;
        G.graves_arm = -1; G.graves_msg = GM_NONE;
        snd_sfx_ex(SFX_TICK, 0.6f, 0.5f);
    }
    if (!(IN.hit[KEY_ENTER] || IN.hit[KEY_SPACE] || IN.hit[KEY_E] || IN.hit[KEY_MOUSE1])) return;
    int i = G.ui_sel, used = grave_info(i).used;
    if (G.graves_mode && !used) { G.graves_msg = GM_EMPTY; snd_sfx_ex(SFX_ZAP, 0.4f, 0.5f); return; }
    if ((used || G.graves_mode) && G.graves_arm != i) {               /* over a carved name, or losing this run: ask twice */
        G.graves_arm = i; G.graves_msg = GM_ARMED; snd_sfx_ex(SFX_KNOCK, 0.5f, 0.5f);
        return;
    }
    if (G.graves_mode) rise_from_grave(i); else carve_grave(i);
}

/* the L menu: W/S choose, Enter casts, Space the toggle, Esc or L closes */
static void update_wispmenu(void) {
    WispChoice c[WISP_CHOICES];
    int n = wisp_choices(c);
    if (IN.hit[KEY_ESC] || IN.hit[KEY_L]) { G.ui = UI_NONE; return; }
    if (IN.hit[KEY_W] || IN.hit[KEY_UP]) { G.ui_sel = (G.ui_sel + n - 1) % n; snd_sfx_ex(SFX_TICK, 0.6f, 0.5f); }
    if (IN.hit[KEY_S] || IN.hit[KEY_DOWN]) { G.ui_sel = (G.ui_sel + 1) % n; snd_sfx_ex(SFX_TICK, 0.6f, 0.5f); }
    G.ui_sel = clampi(G.ui_sel, 0, n - 1);
    if (IN.hit[KEY_SPACE]) {
        RUN.wisp_auto ^= 1;
        if (RUN.wisp_auto) { RUN.wisp_kind = c[G.ui_sel].kind; RUN.wisp_color = c[G.ui_sel].color; }
        snd_sfx_ex(SFX_KEY, 0.7f, 0.5f);
    }
    if (IN.hit[KEY_ENTER] || IN.hit[KEY_E] || IN.hit[KEY_MOUSE1]) { G.ui = UI_NONE; cast_choice(&c[G.ui_sel]); }
}

/* the altar's menu: W/S choose, Enter takes the gift, Esc closes */
static void update_altarmenu(void) {
    AltarRow r[3];
    altar_rows(r);
    if (IN.hit[KEY_ESC]) { G.ui = UI_NONE; return; }
    if (IN.hit[KEY_W] || IN.hit[KEY_UP]) { G.ui_sel = (G.ui_sel + 2) % 3; snd_sfx_ex(SFX_TICK, 0.6f, 0.5f); }
    if (IN.hit[KEY_S] || IN.hit[KEY_DOWN]) { G.ui_sel = (G.ui_sel + 1) % 3; snd_sfx_ex(SFX_TICK, 0.6f, 0.5f); }
    G.ui_sel = clampi(G.ui_sel, 0, 2);
    if (!(IN.hit[KEY_ENTER] || IN.hit[KEY_E] || IN.hit[KEY_SPACE] || IN.hit[KEY_MOUSE1])) return;
    if (!r[G.ui_sel].ok) { snd_sfx_ex(SFX_KNOCK, 0.5f, 0.5f); return; }   /* its note says why */
    G.ui = UI_NONE;
    if (G.ui_sel == 0) { if (G.altar_state == ALTAR_DORMANT) altar_wake(); else altar_send(); }
    else if (G.ui_sel == 1) altar_bless(); else altar_anoint();
}

/* the shrine's screen: W/S the actions, A/D their values, Enter; among the slots W/A/S/D and Enter;
 * Esc goes back, and from the actions leaves (asking first if a take would be lost) */
static int shrine_rows(void) { return RUN.shrine ? NSR : 2; }
static void shrine_leave(void) {
    if (G.sh_takes > 0 && shrine_count(1) > 0) { G.sh_mode = SM_LEAVE; G.ui_sel = 0; snd_sfx_ex(SFX_KNOCK, 0.6f, 0.5f); return; }
    G.ui = UI_NONE;
}
static void shrine_slots(int mode, int row) {                      /* into the slots, on one that holds a rune */
    G.sh_mode = mode; G.sh_arm = -1; G.ui_sel = row;
    if (!RUN.slot_def[G.sh_slot]) for (int i = 0; i < SHRINE_SLOTS; i++) if (RUN.slot_def[i]) { G.sh_slot = i; break; }
    snd_sfx_ex(SFX_TICK, 0.6f, 0.5f);
}
static void update_shrinemenu(void) {
    int enter = IN.hit[KEY_ENTER] || IN.hit[KEY_E] || IN.hit[KEY_SPACE] || IN.hit[KEY_MOUSE1];
    int up = IN.hit[KEY_W] || IN.hit[KEY_UP], down = IN.hit[KEY_S] || IN.hit[KEY_DOWN];
    int left = IN.hit[KEY_A] || IN.hit[KEY_LEFT], right = IN.hit[KEY_D] || IN.hit[KEY_RIGHT];
    if (G.sh_mode == SM_PULL) return;                              /* the runes are coming */
    if (G.sh_mode == SM_TAKE || G.sh_mode == SM_LETGO) {
        if (IN.hit[KEY_ESC]) { G.sh_mode = SM_MENU; G.sh_arm = -1; return; }
        int mx = right - left, my = down - up;
        if (mx || my) { G.sh_slot = (G.sh_slot % 3 + mx + 3) % 3 + (G.sh_slot / 3 + my + 3) % 3 * 3; G.sh_arm = -1; snd_sfx_ex(SFX_TICK, 0.6f, 0.5f); }
        if (!enter) return;
        int i = G.sh_slot, k = RUN.slot_def[i] - 1, q = RUN.slot_q[i];
        if (k < 0) { snd_sfx_ex(SFX_KNOCK, 0.5f, 0.5f); return; }
        if (G.sh_mode == SM_TAKE) {
            if (RUN.gear.def[k] >= q && G.sh_arm != i) { G.sh_arm = i; shrine_note("YOURS IS AS GOOD. ENTER AGAIN TAKES IT ANYWAY"); snd_sfx_ex(SFX_KNOCK, 0.7f, 0.5f); return; }
            shrine_take(i);
            if (G.sh_takes <= 0 || !shrine_count(1)) G.sh_mode = SM_MENU;
        } else {
            if (G.sh_arm != i) { G.sh_arm = i; shrine_note("ENTER AGAIN LETS IT GO FOR GOOD"); snd_sfx_ex(SFX_KNOCK, 0.7f, 0.5f); return; }
            shrine_letgo(i);
            if (!shrine_count(1)) G.sh_mode = SM_MENU;
        }
        G.sh_arm = -1;
        return;
    }
    if (G.sh_mode == SM_FULL || G.sh_mode == SM_LEAVE) {             /* a question */
        int full = G.sh_mode == SM_FULL, n = full ? 3 : 2;
        if (IN.hit[KEY_ESC]) { G.sh_mode = SM_MENU; G.ui_sel = full ? SR_PULL : SR_LEAVE; return; }
        if (up || down) { G.ui_sel = (G.ui_sel + n + down - up) % n; snd_sfx_ex(SFX_TICK, 0.6f, 0.5f); }
        G.ui_sel = clampi(G.ui_sel, 0, n - 1);
        if (!enter) return;
        if (full) {
            if (G.ui_sel == 0) shrine_slots(SM_LETGO, SR_PULL);
            else if (G.ui_sel == 1) { shrine_pull(G.sh_coins, G.sh_fav); G.sh_mode = SM_PULL; G.ui_sel = SR_PULL; }
            else { G.sh_mode = SM_MENU; G.ui_sel = SR_PULL; }
        } else if (G.ui_sel == 0) shrine_slots(SM_TAKE, SR_TAKE);
        else G.ui = UI_NONE;
        return;
    }
    int n = shrine_rows();
    if (IN.hit[KEY_ESC]) { shrine_leave(); return; }
    if (up || down) { G.ui_sel = (G.ui_sel + n + down - up) % n; G.sh_arm = -1; snd_sfx_ex(SFX_TICK, 0.6f, 0.5f); }
    G.ui_sel = clampi(G.ui_sel, 0, n - 1);
    if (!RUN.shrine) {                                             /* sanctified: it can only be cursed */
        if (!enter) return;
        if (G.ui_sel == 1) { G.ui = UI_NONE; return; }
        if (RUN.coins < SHRINE_COST[0]) { shrine_note("YOU HAVE NO CURSED COINS"); snd_sfx_ex(SFX_KNOCK, 0.5f, 0.5f); return; }
        shrine_curse(); G.ui_sel = SR_PULL; G.sh_coins = clampi(RUN.coins, 1, 3);
        return;
    }
    if (left || right) {
        if (G.ui_sel == SR_PULL) G.sh_coins = clampi(G.sh_coins + right - left, 1, 3);
        else if (G.ui_sel == SR_FAVOUR) G.sh_fav = fav_step(G.sh_fav, right - left);
        snd_sfx_ex(SFX_TICK, 0.6f, 0.5f);
    }
    if (!enter) return;
    switch (G.ui_sel) {
    case SR_PULL: {
        int cost = G.sh_coins + fav_coins(G.sh_fav);
        if (cost > RUN.coins) { shrine_note("THAT PULL TAKES %d COIN%s; YOU HAVE %d", cost, cost == 1 ? "" : "S", RUN.coins); snd_sfx_ex(SFX_KNOCK, 0.5f, 0.5f); return; }
        if (pull_most(G.sh_coins) > shrine_count(0)) { G.sh_mode = SM_FULL; G.ui_sel = 0; snd_sfx_ex(SFX_KNOCK, 0.6f, 0.5f); return; }
        shrine_pull(G.sh_coins, G.sh_fav); G.sh_mode = SM_PULL;
        return;
    }
    case SR_FAVOUR: G.sh_fav = fav_step(G.sh_fav, 1); snd_sfx_ex(SFX_TICK, 0.6f, 0.5f); return;
    case SR_RAISE:
        if (RUN.shrine >= NQUAL) { shrine_note("IT CAN RISE NO HIGHER"); return; }
        if (RUN.coins < SHRINE_COST[RUN.shrine]) { shrine_note("RAISING IT TAKES %d COINS", SHRINE_COST[RUN.shrine]); snd_sfx_ex(SFX_KNOCK, 0.5f, 0.5f); return; }
        if (G.sh_arm != SR_RAISE) { G.sh_arm = SR_RAISE; shrine_note("ENTER AGAIN SPENDS %d COINS", SHRINE_COST[RUN.shrine]); snd_sfx_ex(SFX_KNOCK, 0.7f, 0.5f); return; }
        shrine_raise(); G.sh_arm = -1;
        return;
    case SR_TAKE:
        if (G.sh_takes <= 0) { shrine_note("PULL FIRST: EACH PULL EARNS ONE TAKE"); snd_sfx_ex(SFX_KNOCK, 0.5f, 0.5f); return; }
        if (!shrine_count(1)) { shrine_note("THE SLOTS ARE EMPTY"); return; }
        shrine_slots(SM_TAKE, SR_TAKE);
        return;
    case SR_LETGO:
        if (!shrine_count(1)) { shrine_note("THE SLOTS ARE EMPTY"); return; }
        shrine_slots(SM_LETGO, SR_LETGO);
        return;
    default: shrine_leave(); return;
    }
}

static void update_ui(void) {
    if (IN.hit[KEY_MOUSE1]) G.click_spent = 1;                    /* a click that closes a menu casts nothing */
    if (G.ui == UI_WISPS) { update_wispmenu(); return; }
    if (G.ui == UI_SHRINE) { update_shrinemenu(); return; }
#ifdef CRAWL_DEBUG
    if (G.pilgrim == 2 && G.ui == UI_ALTAR) {                     /* the test player sends the wisp on, after a look */
        static int look;
        if (++look >= 40) { look = 0; G.ui_sel = 0; IN.hit[KEY_ENTER] = 1; }
    }
#endif
    if (G.ui == UI_ALTAR) { update_altarmenu(); return; }
    if (G.ui == UI_INVENTORY) update_inventory();
    else if (G.ui == UI_GRAVES) update_graves();
    else if (G.ui == UI_BESTIARY) update_bestiary();
    else if (G.ui == UI_LETGO) update_letgo();
    else update_quit();
}

/* ================================================================== */
/* ================================================================== */
/* Rendering                                                           */
/* ================================================================== */

static inline uint8_t shade(uint8_t c, float light, int x, int y) {
    if (IS_FULLBRIGHT(c)) return c;
    int lv = (int)(light * LIGHT_UNIT + BAYER[y & 3][x & 3] * (1.0f / 16.0f));
    return crawl_cmap[clampi(lv, 0, NLIGHT - 1)][c];
}

/* Fog, done the VGA way: dithered pixels of grey haze, mixed from the bank at the spot being drawn
 * and the one the caster stands in, growing with distance, lit by whatever lights the spot. */
static float g_camfog;
static inline float fog_at(float x, float y) {
    float fx = x - 0.5f, fy = y - 0.5f;
    int ix = (int)floorf(fx), iy = (int)floorf(fy);
    if (ix < 0 || iy < 0 || ix >= MAPN - 1 || iy >= MAPN - 1) return 0.0f;
    float ax = fx - ix, ay = fy - iy;
    return lerpf(lerpf(G.fog[iy][ix], G.fog[iy][ix + 1], ax), lerpf(G.fog[iy + 1][ix], G.fog[iy + 1][ix + 1], ax), ay);
}
static inline float fog_amount(float x, float y, float depth_k) {   /* depth_k: 1 - exp(-distance * 0.28) */
    return fminf((fog_at(x, y) * 0.6f + g_camfog * 0.4f) * depth_k, 0.7f);
}
static inline uint8_t fogged(uint8_t c, float fog, float light, int x, int y) {
    if (fog * 16.0f > BAYER[y & 3][x & 3] + 0.5f) return (uint8_t)(R_CEIL + clampi((int)(3.0f + light * 8.0f), 3, 12));
    return c;
}

static int fm_any;                                                 /* any hazard marks on the floor this frame */
static int fm_at(int cx, int cy, float wx, float wy, float fx, float fy, int x, int y, uint8_t *base, float *lmul);
static void render_floor(Frame *f, float dx, float dy, float plx, float ply, float horizon) {
    float lx = dx - plx, ly = dy - ply, rx = dx + plx, ry = dy + ply;
    int themed = G.ptheme != PT_NONE, sev = G.ptheme == PT_SEVERANCE;   /* carpet underfoot, tiles or panels above */
    static uint8_t lamp_lit[MAXLIGHTS + 1];
    if (themed) for (int l = 0; l < G.nlights; l++) lamp_lit[l + 1] = torch_flicker(&G.lights[l]) > 0.5f;
    for (int y = 0; y < H; y++) {
        uint8_t *row = &f->pix[y * W];
        float p = y - horizon + 0.5f;
        int floor_row = p > 0.0f;
        float dist = floor_row ? G.camz * FOCAL / p : (1.0f - G.camz) * FOCAL / -p;
        if (dist > 30.0f) { memset(row, 0, W); continue; }
        float sx = dist * (rx - lx) / W, sy = dist * (ry - ly) / W;
        float wx = G.px + dist * lx, wy = G.py + dist * ly;
        float a = atten(dist), dk = 1.0f - expf(-dist * 0.28f);
        int grout = floor_row && dist < 6.0f;
        for (int x = 0; x < W; x++, wx += sx, wy += sy) {
            uint8_t base;
            float lmul = 1.0f;
            if (floor_row) {
                int cx = (int)floorf(wx), cy = (int)floorf(wy);
                float fx = wx - cx, fy = wy - cy;
                if (themed) {                                      /* each cell's carpet turned about, so it doesn't repeat */
                    uint32_t h = hash2(cx, cy);
                    int tx = (int)(fx * TS) & (TS - 1), ty = (int)(fy * TS) & (TS - 1);
                    if (h & 1) tx = TS - 1 - tx;
                    if (h & 2) ty = TS - 1 - ty;
                    if (h & 4) { int q = tx; tx = ty; ty = q; }
                    base = crawl_tex[sev ? T_SCARPET : T_CARPET][tx * TS + ty];
                } else {
                    base = (uint8_t)(R_FLOOR + 8 + ((cx * 7 + cy * 13) & 1));
                    if (grout && (fx < 0.035f || fy < 0.035f)) base = R_FLOOR + 5;
                }
                if (fm_any) {                                      /* a hazard's mark */
                    int o = fm_at(cx, cy, wx, wy, fx, fy, x, y, &base, &lmul);
                    if (o >= 0) { row[x] = (uint8_t)o; continue; }
                }
                if (G.nova_t > 0.0f) {                             /* the frost nova racing out over the floor */
                    float r = 2.2f * sqrtf(G.nova_t / 0.6f), d = hypotf(wx - G.px, wy - G.py);
                    if (d < r && (fabsf(d - r) < 0.22f || BAYER[y & 3][x & 3] < (int)(10.0f * (1.0f - G.nova_t / 0.6f)))) {
                        row[x] = (uint8_t)(R_GLOW + clampi((int)(15.0f - fabsf(d - r) * 30.0f), 5, 15));
                        continue;
                    }
                }
            } else if (themed) {
                int cx = (int)floorf(wx), cy = (int)floorf(wy);
                int tx = (int)((wx - cx) * TS) & (TS - 1), ty = (int)((wy - cy) * TS) & (TS - 1);
                const Cell *c = cell(cx, cy);
                int t = sev ? (c->lamp && c->lamp != 255 && lamp_lit[c->lamp] ? T_SCEILLAMP : T_SCEIL)
                      : c->lamp == 255 ? T_CEILDEAD : c->lamp ? (lamp_lit[c->lamp] ? T_CEILLAMP : T_CEILDEAD)
                      : hash2(cx, cy) % 14 == 0 ? T_CEILSTAIN : T_CEILTILE;
                base = crawl_tex[t][tx * TS + ty];
            } else base = R_CEIL + 8;
            float L = lm_at(wx, wy) * a * lmul;
            row[x] = fogged(shade(base, L, x, y), fog_amount(wx, wy, dk * (floor_row ? 1.15f : 0.8f)), L, x, y);
        }
    }
}

/* cracks for a secret wall: dark lines from a point near the middle, a lit edge along each */
static uint8_t crack_mask[TS * TS];                                /* column-major, like the textures: 1 crack, 2 its lit edge */
static void build_cracks(void) {
    uint32_t r = 0x5ec2e7u;
    for (int k = 0; k < 6; k++) {
        float x = 30 + rng_f(&r) * 6, y = 26 + rng_f(&r) * 8, a = TAU_F * k / 6.0f + rng_f(&r) * 0.6f;
        int len = 12 + (int)(rng_f(&r) * 16), branch = 5 + (int)(rng_f(&r) * 8);
        for (int i = 0; i < len; i++) {
            a += (rng_f(&r) - 0.5f) * 0.7f;
            x += cosf(a); y += sinf(a);
            int ix = (int)x, iy = (int)y;
            if (ix < 1 || iy < 1 || ix >= TS - 1 || iy >= TS - 1) break;
            crack_mask[ix * TS + iy] = 1;
            if (!crack_mask[(ix + 1) * TS + iy + 1]) crack_mask[(ix + 1) * TS + iy + 1] = 2;
            if (i == branch) {                                     /* a fork */
                float bx = x, by = y, ba = a + (rng_f(&r) < 0.5f ? 0.9f : -0.9f);
                for (int j = 0; j < 7; j++) {
                    bx += cosf(ba); by += sinf(ba); ba += (rng_f(&r) - 0.5f) * 0.6f;
                    int jx = (int)bx, jy = (int)by;
                    if (jx >= 1 && jy >= 1 && jx < TS - 1 && jy < TS - 1) crack_mask[jx * TS + jy] = 1;
                }
            }
        }
    }
}

/* where a ray meets the sliding block: distance, across its face, and which way the face looks */
/* ---- the hazards, drawn: their marks on the floor, made each frame from G.haz ---- */
enum { FM_NONE, FM_SHADOW, FM_CRACK, FM_FIRE, FM_EMBER, FM_SHEEN, FM_BILE, FM_VRING, FM_VOID, FM_ARC, FM_GRASP };
typedef struct { uint8_t kind, prio; float k, ax, ay, r, ang; } FMark;
static FMark fmark[MAPN][MAPN];
static void fm_put(int x, int y, int kind, int prio, float k, const Hazard *h) {
    if (x < 0 || y < 0 || x >= MAPN || y >= MAPN || fmark[y][x].prio > prio) return;
    FMark *m = &fmark[y][x];
    m->kind = (uint8_t)kind; m->prio = (uint8_t)prio; m->k = k; m->ax = h->x; m->ay = h->y; m->r = h->r; m->ang = h->ang;
}
static int hz_marks(void) {
    memset(fmark, 0, sizeof fmark);
    if (PK.depth || G.shop) return 0;
    int any = 0;
    for (int i = 0; i < MAXHAZ; i++) {
        const Hazard *h = &G.haz[i];
        if (!h->alive) continue;
        int ph = hz_phase(h);
        float kw = clampf(h->t / h->warn, 0.0f, 1.0f), kf = h->off > 0.0f ? 1.0f - clampf((h->t - h->warn - h->on) / h->off, 0.0f, 1.0f) : 0.0f;
        int x0 = (int)floorf(h->x - h->r - 1.0f), x1 = (int)floorf(h->x + h->r + 1.0f), y0 = (int)floorf(h->y - h->r - 1.0f), y1 = (int)floorf(h->y + h->r + 1.0f);
        any = 1;
        switch (h->kind) {
        case HZ_ROCK: if (ph == 0) for (int y = y0; y <= y1; y++) for (int x = x0; x <= x1; x++) fm_put(x, y, FM_SHADOW, 2, kw, h); break;
        case HZ_FIRE:
            for (int c = 0; c < h->n; c++) {
                if (ph == 1) { fm_put(h->cx[c], h->cy[c], FM_FIRE, 3, 1.0f, h); continue; }
                if (ph == 2) { fm_put(h->cx[c], h->cy[c], FM_EMBER, 1, kf, h); continue; }
                float reach = h->warn * 0.6f * (float)h->cd[c] / fmaxf(1.0f, h->r);   /* the crack gets there along the line */
                if (h->t >= reach) fm_put(h->cx[c], h->cy[c], FM_CRACK, 2, clampf((h->t - reach) / fmaxf(0.1f, h->warn - reach), 0.0f, 1.0f), h);
            }
            break;
        case HZ_BILE:
            for (int c = 0; c < h->n; c++) {
                float w = hz_bile_wet(h, c);
                if (w <= 0.0f) continue;
                if (w >= 1.0f && ph < 2) fm_put(h->cx[c], h->cy[c], FM_BILE, 3, 1.0f, h);
                else fm_put(h->cx[c], h->cy[c], FM_SHEEN, 2, w, h);
            }
            break;
        case HZ_VOID:
            for (int y = y0; y <= y1; y++) for (int x = x0; x <= x1; x++)
                fm_put(x, y, ph == 0 ? FM_VRING : FM_VOID, ph == 1 ? 3 : 2, ph == 0 ? kw : (ph == 1 ? 1.0f : kf), h);
            break;
        case HZ_ARC:
            if (ph < 2 || h->t < h->warn + h->on + 0.15f)
                for (int y = y0; y <= y1; y++) for (int x = x0; x <= x1; x++) fm_put(x, y, FM_ARC, 2, ph == 0 ? kw : 2.0f, h);
            break;
        default:
            if (ph == 0) for (int y = y0; y <= y1; y++) for (int x = x0; x <= x1; x++) fm_put(x, y, FM_GRASP, 2, kw, h);
            break;
        }
    }
    return any;
}
/* one floor pixel under a mark: returns a colour drawn as it is (bright, or the void's dark), or -1 to
 * be lit as usual, with the floor's colour and a dimming for its light set */
static int fm_pixel(const FMark *m, float wx, float wy, float fx, float fy, int x, int y, uint8_t *base, float *lmul) {
    float d = hypotf(wx - m->ax, wy - m->ay), k = m->k;
    int b = BAYER[y & 3][x & 3];
    switch (m->kind) {
    case FM_SHADOW: {                                              /* a blur that hardens into a dark disc */
        float rr = m->r * (0.45f + 0.55f * k), edge = 0.05f + 0.3f * (1.0f - k);
        if (d < rr) *lmul *= 1.0f - 0.85f * k * (1.0f - smoothstepf(rr - edge, rr, d));
        if (k > 0.6f && fabsf(d - rr) < 0.035f) return 0;
        return -1;
    }
    case FM_CRACK: case FM_EMBER: {
        int tx = (int)(fx * TS) & (TS - 1), ty = (int)(fy * TS) & (TS - 1);
        uint32_t hh = hash2((int)floorf(wx), (int)floorf(wy));
        if (hh & 1) tx = TS - 1 - tx;
        if (hh & 2) { int q = tx; tx = ty; ty = q; }
        if (crack_mask[tx * TS + ty]) {
            if (m->kind == FM_EMBER) return R_BLOOD + clampi((int)(3.0f + 9.0f * k), 0, 15);
            return k < 0.5f ? R_BLOOD + clampi((int)(5.0f + k * 18.0f), 0, 15) : R_FLAME + clampi((int)(3.0f + (k - 0.5f) * 24.0f), 0, 15);
        }
        if (m->kind == FM_EMBER) *lmul *= 0.75f;
        return -1;
    }
    case FM_FIRE: {
        float v = 0.5f + 0.5f * sinf(wx * 9.0f + (float)G.t * 7.0f) * sinf(wy * 8.0f - (float)G.t * 5.0f);
        return R_FLAME + clampi((int)(5.0f + 10.0f * v), 0, 15);
    }
    case FM_SHEEN:                                                 /* wet and dark, a glint or two */
        *lmul *= 1.0f - 0.4f * k;
        if (k > 0.3f && (hash2((int)(wx * 9.0f), (int)(wy * 9.0f)) & 63) == 0) *base = (uint8_t)(R_GHOST + 12);
        return -1;
    case FM_BILE: {                                                /* foul and green, bubbling: it shows in the dark */
        int cx = (int)floorf(wx), cy = (int)floorf(wy);
        for (int i = 0; i < 3; i++) {
            uint32_t hh = hash2(cx * 3 + i, cy * 5 - i);
            float bx = 0.2f + 0.6f * (float)(hh & 255) / 255.0f, by = 0.2f + 0.6f * (float)((hh >> 8) & 255) / 255.0f;
            float ph = (float)G.t * 0.8f + (float)((hh >> 16) & 255) / 255.0f;
            float rr = 0.14f * (ph - floorf(ph)), dd = hypotf(fx - bx, fy - by);
            if (fabsf(dd - rr) < 0.025f) return R_MOSS + 15;     /* a bubble's skin */
        }
        float v = 0.5f + 0.25f * sinf((wx + wy) * 5.0f + (float)G.t * 1.7f);
        return R_MOSS + clampi((int)(3.0f + 6.0f * v + (b > 12 ? 1 : 0)), 0, 15);
    }
    case FM_VRING:                                                 /* a ring where it will open, and a haze in it */
        if (fabsf(d - m->r) < 0.05f + 0.05f * k) return R_PURPLE + clampi((int)(9.0f + 6.0f * k), 0, 15);
        if (d < m->r && b < (int)(k * 4.0f)) return R_PURPLE + 4;
        return -1;
    case FM_VOID:
        if (fabsf(d - m->r) < 0.07f) return R_PURPLE + clampi((int)(8.0f + 7.0f * k), 0, 15);
        if (d < m->r && b < (int)(k * 11.0f)) return R_PURPLE + 2 + (b & 3);
        return -1;
    case FM_ARC: {                                                 /* a pale fan before the blade */
        float a = fabsf(wrap_pi(atan2f(wy - m->ay, wx - m->ax) - m->ang));
        if (d > m->r || a > 0.8f || d < 0.2f) return -1;
        if (k >= 2.0f || fabsf(d - m->r) < 0.06f || fabsf(a - 0.8f) * d < 0.05f) return R_BONE + 14;
        return b < (int)(k * 7.0f) ? R_BONE + 12 : -1;
    }
    case FM_GRASP:
        if (fabsf(d - m->r) < 0.07f) return 0;
        if (d < m->r && b < (int)(k * 8.0f)) return 0;
        return -1;
    }
    return -1;
}
static int fm_at(int cx, int cy, float wx, float wy, float fx, float fy, int x, int y, uint8_t *base, float *lmul) {
    if (cx < 0 || cy < 0 || cx >= MAPN || cy >= MAPN || !fmark[cy][cx].kind) return -1;
    return fm_pixel(&fmark[cy][cx], wx, wy, fx, fy, x, y, base, lmul);
}

static int slide_ray(float rdx, float rdy, float *t, float *u, int *side) {
    float bx, by, tx0, tx1, ty0, ty1;
    slide_box(&bx, &by);
    if (fabsf(rdx) < 1e-6f) { if (G.px < bx || G.px > bx + 1) return 0; tx0 = -1e30f; tx1 = 1e30f; }
    else { tx0 = (bx - G.px) / rdx; tx1 = (bx + 1 - G.px) / rdx; if (tx0 > tx1) { float q = tx0; tx0 = tx1; tx1 = q; } }
    if (fabsf(rdy) < 1e-6f) { if (G.py < by || G.py > by + 1) return 0; ty0 = -1e30f; ty1 = 1e30f; }
    else { ty0 = (by - G.py) / rdy; ty1 = (by + 1 - G.py) / rdy; if (ty0 > ty1) { float q = ty0; ty0 = ty1; ty1 = q; } }
    float tn = fmaxf(tx0, ty0), tf = fminf(tx1, ty1);
    if (tn > tf || tn <= 0.0f) return 0;
    *side = tx0 > ty0 ? 0 : 1;
    *u = *side == 0 ? G.py + rdy * tn - by : G.px + rdx * tn - bx;
    *t = tn;
    return 1;
}

static void render_walls(Frame *f, float dx, float dy, float plx, float ply, float horizon) {
    int tframe = (int)(G.t * 8.0);
    static int cracks_built;
    if (!cracks_built) { build_cracks(); cracks_built = 1; }
    for (int x = 0; x < W; x++) {
        float camx = 2.0f * (x + 0.5f) / W - 1.0f;
        float rdx = dx + plx * camx, rdy = dy + ply * camx;
        int mx = (int)G.px, my = (int)G.py;
        float ddx = rdx == 0.0f ? 1e30f : fabsf(1.0f / rdx), ddy = rdy == 0.0f ? 1e30f : fabsf(1.0f / rdy);
        int sx = rdx < 0 ? -1 : 1, sy = rdy < 0 ? -1 : 1;
        float sdx = rdx < 0 ? (G.px - mx) * ddx : (mx + 1.0f - G.px) * ddx;
        float sdy = rdy < 0 ? (G.py - my) * ddy : (my + 1.0f - G.py) * ddy;
        int side = 0, tex = T_STONE, texx = 0, hit = 0;
        float perp = 30.0f, cracks = 0.0f;
        for (int i = 0; i < 100 && !hit; i++) {
            if (sdx < sdy) { sdx += ddx; mx += sx; side = 0; } else { sdy += ddy; my += sy; side = 1; }
            Cell *c = cell(mx, my);
            if (G.slide >= 0 && slide_covers(mx, my)) {           /* the secret wall, on its way back */
                float t, u; int sd;
                if (slide_ray(rdx, rdy, &t, &u, &sd) && (int)floorf(G.px + rdx * t + rdx * 1e-3f) == mx && (int)floorf(G.py + rdy * t + rdy * 1e-3f) == my) {
                    perp = t; tex = G.secrets[G.slide].tex; side = sd;
                    texx = clampi((int)(u * TS), 0, TS - 1);
                    if ((side == 0 && rdx > 0) || (side == 1 && rdy < 0)) texx = TS - 1 - texx;
                    cracks = 1.0f;
                    hit = 1;
                    continue;
                }
            }
            if (c->kind == K_WALL) {
                perp = side == 0 ? sdx - ddx : sdy - ddy;
                tex = c->tex;
                int pxc = side == 0 ? mx - sx : mx, pyc = side == 1 ? my - sy : my;
                if (cell(pxc, pyc)->kind == K_DOOR) tex = T_JAMB;
                if (tex == T_TORCH0) tex = T_TORCH0 + (int)((tframe + hash2(mx, my)) % 3);
                if (tex == T_EXIT0) tex = boss_sealed() ? T_GATE : G.exit_open > 0.3f ? T_EXIT_OPEN : T_EXIT0 + ((int)(G.t * 2.5) & 1);   /* barred while a boss lives */
                if (tex == T_VISPORT0) tex = T_VISPORT0 + (PK.vis_used[PK.chain] ? 0 : ((int)(G.t * 5.0) & 3));
                float wallx = side == 0 ? G.py + perp * rdy : G.px + perp * rdx;
                wallx -= floorf(wallx);
                texx = (int)(wallx * TS);
                if ((side == 0 && rdx > 0) || (side == 1 && rdy < 0)) texx = TS - 1 - texx;
                if (c->secret == 2) cracks = 1.0f - smoothstepf(1.3f, 2.8f, perp);   /* only close up do the cracks show */
                hit = 1;
            } else if (c->kind == K_DOOR) {
                Door *d = &G.doors[c->door];
                float t, u;
                if (d->along_x) {
                    if (rdx == 0.0f) continue;
                    t = (mx + 0.5f - G.px) / rdx;
                    u = G.py + t * rdy - my;
                } else {
                    if (rdy == 0.0f) continue;
                    t = (my + 0.5f - G.py) / rdy;
                    u = G.px + t * rdx - mx;
                }
                if (t <= 0.0f || u < 0.0f || u >= 1.0f || u < d->open) continue;
                static const int LOCKTEX[NKEYS] = { T_LOCKED, T_LOCKED_S, T_LOCKED_C };
                perp = t; tex = d->locked ? LOCKTEX[d->color] : T_DOOR; side = d->along_x ? 0 : 1;
                texx = clampi((int)((u - d->open) * TS), 0, TS - 1);
                hit = 1;
            }
        }
        if (perp < 0.05f) perp = 0.05f;
        zbuf[x] = perp;
        if (!hit) continue;
        float lineh = FOCAL / perp;
        float top = horizon - (1.0f - G.camz) * lineh, bot = horizon + G.camz * lineh;
        float hx = G.px + perp * rdx - (side == 0 ? sx * 0.13f : 0.0f);
        float hy = G.py + perp * rdy - (side == 1 ? sy * 0.13f : 0.0f);
        float light = lm_at(hx, hy) * atten(perp) * (side ? 0.82f : 1.0f);
        float fog = fog_amount(hx, hy, 1.0f - expf(-perp * 0.28f));
        const uint8_t *col = &crawl_tex[tex][texx * TS];
        int y0 = clampi((int)ceilf(top - 0.5f), 0, H), y1 = clampi((int)ceilf(bot - 0.5f), 0, H);
        float tstep = TS / lineh, ty = (y0 + 0.5f - top) * tstep;
        if (cracks > 0.0f) {
            const uint8_t *cm = &crack_mask[texx * TS];
            int thr = (int)(cracks * 16.0f);
            for (int y = y0; y < y1; y++, ty += tstep) {
                int tyi = clampi((int)ty, 0, TS - 1), m = cm[tyi];
                float L = m && BAYER[y & 3][x & 3] < thr ? (m == 1 ? light * 0.2f : light * 1.3f) : light;
                f->pix[y * W + x] = fogged(shade(col[tyi], L, x, y), fog, light, x, y);
            }
            continue;
        }
        for (int y = y0; y < y1; y++, ty += tstep)
            f->pix[y * W + x] = fogged(shade(col[clampi((int)ty, 0, TS - 1)], light, x, y), fog, light, x, y);
    }
}

/* alpha < 1: dithered see-through. remap: the ramp R_PURPLE turns into. flags: how it is drawn.
 * lut: a creature's rank, recolouring its body (crawl_rank_lut) */
typedef struct { float x, y, z, depth; int spr; float alpha, twinkle, scale; int remap, flags; float sway; const uint8_t *lut; int glint; } Draw;   /* glint: a key's ramp, for Keymaker's sparkle */
enum { DR_EYES = 1, DR_IMPLODE = 2, DR_BRIGHT = 4, DR_ICE = 8, DR_CAT = 16, DR_SWAY = 32, DR_FLIPV = 64, DR_COMMUNE = 128, DR_GLINT = 256, DR_EDGE = 512,
       DR_GLITCH = 1024, DR_CLIP = 2048, DR_LOOPED = 4096, DR_LIT = 8192, DR_SHINE = 16384, DR_MEND = 32768,
       DR_RIPPLE = 65536 };                                         /* RIPPLE: the Wraith's Time Slip, grey, going out */   /* SHINE: polished plate, a glint
                                                                                                 sliding over it */
/* GLITCH: rows torn sideways and colours slipping, now and then (the Backrooms), more often when LOOPED; CLIP: cut at
 * the floor and ceiling; LIT: brighter than the light it stands in (Severance's creatures, under the panels) */
/* FLIPV: upside down, on the ceiling; COMMUNE: red through walls; GLINT: a key glinting through walls (Keymaker) */

static int cmp_draw(const void *a, const void *b) {
    float da = ((const Draw *)a)->depth, db = ((const Draw *)b)->depth;
    return da < db ? 1 : (da > db ? -1 : 0);
}

/* world to screen; 0 if behind the eye */
static int project(float x, float y, float z, float dx, float dy, float horizon, float *sx, float *sy, float *depth) {
    float vx = x - G.px, vy = y - G.py, dp = vx * dx + vy * dy;
    if (dp < 0.15f) return 0;
    *sx = W / 2 + FOCAL * (vx * -dy + vy * dx) / dp;
    *sy = horizon - (z - G.camz) * FOCAL / dp;
    *depth = dp;
    return 1;
}

/* a line through the crypt, hidden by walls; colour from the ramp, lit by nothing (it glows) */
static void line3d(Frame *f, float dx, float dy, float horizon, float x0, float y0, float z0, float x1, float y1, float z1,
                   uint8_t core, uint8_t halo, int dither) {
    float ax, ay, ad, bx, by, bd;
    int a_ok = project(x0, y0, z0, dx, dy, horizon, &ax, &ay, &ad), b_ok = project(x1, y1, z1, dx, dy, horizon, &bx, &by, &bd);
    float len = a_ok && b_ok ? hypotf(bx - ax, by - ay) : 60.0f;
    int n = clampi((int)len + 2, 2, 400);
    for (int i = 0; i <= n; i++) {
        float t = (float)i / n, sx, sy, dep;
        if (!project(lerpf(x0, x1, t), lerpf(y0, y1, t), lerpf(z0, z1, t), dx, dy, horizon, &sx, &sy, &dep)) continue;
        int x = (int)sx, y = (int)sy;
        if (x < 0 || x >= W || y < 0 || y >= H || dep >= zbuf[x]) continue;
        if (dither && BAYER[y & 3][x & 3] < 8) continue;
        f->pix[y * W + x] = core;
        if (halo) {
            if (y > 0 && f->pix[(y - 1) * W + x] != core) f->pix[(y - 1) * W + x] = halo;
            if (y < H - 1 && f->pix[(y + 1) * W + x] != core) f->pix[(y + 1) * W + x] = halo;
        }
    }
}

/* lightning and the soul drain, a moment each */
static void render_beams(Frame *f, float dx, float dy, float horizon) {
    for (int i = 0; i < MAXBEAMS; i++) {
        const Beam *b = &G.beams[i];
        if (!b->alive) continue;
        float k = 1.0f - b->t / 0.3f;
        if (b->kind == AT_CHAIN) {                                 /* a jagged bolt, re-forked every frame */
            uint32_t r = (uint32_t)(G.t * 30.0) * 2654435761u + (uint32_t)i * 40503u;
            float px = b->x0, py = b->y0, pz = b->z0, ox = -(b->y1 - b->y0), oy = b->x1 - b->x0, ol = hypotf(ox, oy) + 1e-4f;
            ox /= ol; oy /= ol;
            int segs = 7;
            for (int s = 1; s <= segs; s++) {
                float t = (float)s / segs, j = s == segs ? 0.0f : (rng_f(&r) - 0.5f) * 0.45f, jz = s == segs ? 0.0f : (rng_f(&r) - 0.5f) * 0.3f;
                float nx = lerpf(b->x0, b->x1, t) + ox * j, ny = lerpf(b->y0, b->y1, t) + oy * j, nz = lerpf(b->z0, b->z1, t) + jz;
                line3d(f, dx, dy, horizon, px, py, pz, nx, ny, nz, k > 0.4f ? C_WHITE : (uint8_t)(R_GLOW + 12), (uint8_t)(R_GLOW + (k > 0.5f ? 11 : 7)), k < 0.25f);
                px = nx; py = ny; pz = nz;
            }
        } else {                                                   /* the drain: a red thread, wavering, flowing home */
            float ox = -(b->y1 - b->y0), oy = b->x1 - b->x0, ol = hypotf(ox, oy) + 1e-4f;
            ox /= ol; oy /= ol;
            float px = b->x1, py = b->y1, pz = b->z1;
            for (int s = 1; s <= 12; s++) {
                float t = (float)s / 12, w = sinf(t * 9.0f + b->t * 30.0f) * 0.12f * sinf(t * PI_F);
                float nx = lerpf(b->x1, b->x0, t) + ox * w, ny = lerpf(b->y1, b->y0, t) + oy * w, nz = lerpf(b->z1, b->z0, t) + w * 0.6f;
                line3d(f, dx, dy, horizon, px, py, pz, nx, ny, nz, (uint8_t)(R_BLOOD + 14), (uint8_t)(R_BLOOD + 8), k < 0.3f);
                px = nx; py = ny; pz = nz;
            }
        }
    }
}

/* The merchant, put together each frame: blinking, ears twitching, the tail swishing over the
 * side of the chest, and the eyes following the caster. */
static uint8_t cat_buf[CAT_H * CAT_W];
static void cat_disc(float cx, float cy, float r, int fill, float t) {
    for (int y = (int)(cy - r - 1); y <= (int)(cy + r + 1); y++)
        for (int x = (int)(cx - r - 1); x <= (int)(cx + r + 1); x++) {
            if (x < 0 || y < 0 || x >= CAT_W || y >= CAT_H) continue;
            float dx = x + 0.5f - cx, dy = y + 0.5f - cy;
            if (dx * dx + dy * dy > r * r) continue;
            uint8_t c;
            if (!fill) c = (uint8_t)(R_WARM + 1);
            else {
                int stripe = ((int)(t * 11.0f)) & 1, tip = t > 0.86f;
                float lit = 0.5f - dx / r * 0.35f;
                c = (uint8_t)(R_WARM + clampi((int)((tip ? 3 : (stripe ? 5 : 9)) + lit * 4.0f - 2.0f), 2, 13));
            }
            cat_buf[y * CAT_W + x] = c;
        }
}
static void build_cat(void) {
    memcpy(cat_buf, crawl_cat_img, sizeof cat_buf);
    /* the tail: out of the rump and down over the end of the chest, swishing (more after a sale) */
    float sw = (float)G.t * (1.3f + 2.2f * G.cat_tail_k), amp = 4.0f + 5.0f * G.cat_tail_k;
    float p0x = crawl_cat_tail[0], p0y = crawl_cat_tail[1], p1x = p0x + 12.0f, p1y = p0y + 1.0f;
    float p2x = p0x + 10.0f + amp * sinf(sw), p2y = p0y + 31.0f - fabsf(amp * sinf(sw)) * 0.4f;
    for (int pass = 0; pass < 2; pass++)
        for (int i = 0; i <= 48; i++) {
            float t = i / 48.0f, u = 1.0f - t;
            float bx = u * u * p0x + 2 * u * t * p1x + t * t * p2x, by = u * u * p0y + 2 * u * t * p1y + t * t * p2y;
            float r = 3.4f - 1.5f * t;
            cat_disc(bx, by, pass ? r : r + 1.0f, pass, t);
        }
    /* an ear flicks */
    if (G.cat_ear > 0.0f) {
        const uint8_t *e = crawl_cat_ear[G.cat_ear_side];
        float k = sinf(PI_F * (0.32f - G.cat_ear) / 0.32f);
        int dir = G.cat_ear_side ? 1 : -1;
        for (int y = e[1]; y <= e[3]; y++) {
            int sh = (int)lroundf(k * 2.4f * (float)(e[3] - y) / (float)(e[3] - e[1])) * dir;
            for (int x = e[0]; x <= e[2]; x++) {
                int sx = x - sh;
                cat_buf[y * CAT_W + x] = sx >= e[0] && sx <= e[2] ? crawl_cat_img[y * CAT_W + sx] : C_CLEAR;
            }
        }
    }
    /* the eyes: green, slit pupils on the caster, lids for a blink or a slow, fond one */
    float lid = 0.0f;
    if (G.cat_blink > 0.0f) lid = sinf(PI_F * (0.16f - G.cat_blink) / 0.16f);
    if (G.cat_slow > 0.0f && G.cat_slow <= 1.4f) { float u = 1.4f - G.cat_slow; lid = fmaxf(lid, u < 0.45f ? u / 0.45f : (u < 0.85f ? 1.0f : 1.0f - (u - 0.85f) / 0.55f)); }
    int look = (int)lroundf(G.cat_look);
    for (int e = 0; e < 2; e++) {
        int cx = crawl_cat_eye[e][0], cy = crawl_cat_eye[e][1], rx = crawl_cat_eye[e][2], ry = crawl_cat_eye[e][3];
        int shut = (int)(lid * (2 * ry + 1) + 0.5f);
        for (int y = cy - ry; y <= cy + ry; y++)
            for (int x = cx - rx; x <= cx + rx; x++) {
                float ux = (x - cx) / (rx + 0.5f), uy = (y - cy) / (ry + 0.5f);
                if (ux * ux + uy * uy > 1.0f) continue;
                int row = y - (cy - ry);
                uint8_t c;
                if (row < shut) c = row == shut - 1 ? crawl_cat_lid[1] : crawl_cat_lid[0];
                else {
                    c = row == 0 ? crawl_cat_eyecol[0] : (row <= ry ? crawl_cat_eyecol[1] : crawl_cat_eyecol[2]);
                    if (x == cx + look) c = crawl_cat_eyecol[3];
                    if (x == cx - 1 + look && y == cy - 1) c = crawl_cat_eyecol[4];
                }
                cat_buf[y * CAT_W + x] = c;
            }
    }
}

static int spr_bottom(int spr) {                                    /* lowest drawn row of a sprite */
    static int8_t cache[NSPR];
    static int init;
    if (!init) { memset(cache, -1, sizeof cache); init = 1; }
    if (cache[spr] < 0) {
        int b = 0;
        for (int x = 0; x < TS; x++) for (int y = 0; y < TS; y++) if (crawl_spr[spr][x * TS + y] != C_CLEAR && y > b) b = y;
        cache[spr] = (int8_t)b;
    }
    return cache[spr];
}

static int key_sprite(int color, float ph) {
    static const int BASE[NKEYS] = { SP_KEY0, SP_KEYS0, SP_KEYC0 };
    return BASE[color] + (((int)(ph * 5.0f)) & 3);
}

static int mob_sprite(const Mob *m, float *scale) {
    int fr = ((int)m->anim) & 1;
    *scale = KINDS[m->type].scale;
    switch (m->type) {
    case MOB_SHAMBLER:
        if (m->state == MS_DIE) return m->t < 0.35f ? SP_SHAMB_D1 : SP_SHAMB_D2;
        if (m->sub == 2) return m->sub_t < 0.8f ? SP_SHAMB_RISE1 : SP_SHAMB_RISE2;
        return m->swing > 0.0f ? SP_SHAMB_GRAB : (fr ? SP_SHAMB_B : SP_SHAMB_A);
    case MOB_DROWNED:
        if (m->state == MS_DIE) return SP_DROWN_BURST;
        if (m->sub == 2) return SP_DROWN_RISE;
        return m->swing > 0.0f ? SP_DROWN_GRAB : (fr ? SP_DROWN_B : SP_DROWN_A);
    case MOB_HOUND:
        if (m->state == MS_DIE) return SP_HOUND_D;
        return m->swing > 0.0f ? SP_HOUND_BITE : (fr ? SP_HOUND_B : SP_HOUND_A);
    case MOB_HUNTER:
        if (m->state == MS_DIE) return SP_HUNTER_D;
        if (m->boss) return m->sub == 3 || G.web_rear > 0.0f ? SP_BSPIDER_LEAP : (((int)(m->t * 10.0f)) & 1 ? SP_BSPIDER_B : SP_BSPIDER_A);
        return m->sub == 3 ? SP_HUNTER_LEAP : (((int)(m->t * 10.0f)) & 1 ? SP_HUNTER_B : SP_HUNTER_A);
    case MOB_GHOUL:
        if (m->state == MS_DIE) return SP_GHOUL_D;
        return m->sub == 3 ? SP_GHOUL_LEAP : (m->sub == 4 ? SP_GHOUL_FEED : (fr ? SP_GHOUL_B : SP_GHOUL_A));
    case MOB_KNIGHT: {
        if (m->state == MS_DIE) return SP_KNIGHT_D;
        int base = SP_KNIGHT_A + knight_stage(m) * 4;
        if (m->boss && G.bpose_t > 0.0f && G.bpose != BP_NONE && m->state == MS_WALK)   /* the Ill-Fated Knight casting */
            return G.bpose == BP_PLANT ? SP_KNIGHT_P0 + knight_stage(m) : base + (G.bpose == BP_RAISE ? 2 : 3);
        return base + (m->swing > 0.0f ? 2 : (m->throw_t > 0.0f ? 3 : fr));
    }
    case MOB_BROOD:
        if (m->state == MS_DIE) return SP_BROOD_D;
        return m->swing > 0.0f ? SP_BROOD_LAY : (fr ? SP_BROOD_B : SP_BROOD_A);
    case MOB_LING: return ((int)(m->t * 12.0f)) & 1 ? SP_LING_B : SP_LING_A;
    case MOB_SHADE:
        if (m->state == MS_DIE) return SP_SHADE_D;
        return m->swing > 0.0f ? SP_SHADE_TOUCH : (fr ? SP_SHADE_B : SP_SHADE_A);
    case MOB_IMP:
        if (m->state == MS_DIE) return SP_IMP_D;
        return m->throw_t > 0.0f ? SP_IMP_THROW : (((int)(m->t * 8.0f)) & 1 ? SP_IMP_B : SP_IMP_A);
    case MOB_DEACON:
        if (m->state == MS_DIE) return SP_DEACON_D;
        if (m->boss && G.bpose == BP_LITANY && G.bpose_t > 0.0f) return SP_DEACON_CHANT;
        return m->swing > 0.0f ? SP_DEACON_CHANT : (fr ? SP_DEACON_B : SP_DEACON_A);
    case MOB_GARGOYLE:
        if (m->state == MS_DIE) return SP_GARG_D;
        if (m->sub == 0 || (m->sub == 1 && m->sub_t < 0.4f && ((int)(m->sub_t * 20.0f) & 1))) return SP_GARG_PERCH;
        return m->sub == 3 || m->swing > 0.0f ? SP_GARG_SWOOP : (((int)(m->t * 7.0f)) & 1 ? SP_GARG_B : SP_GARG_A);
    case MOB_SAC: return m->state == MS_DIE ? SP_SAC_OPEN : (m->sub_t > 5.0f && ((int)(m->t * 6.0f) & 1) ? SP_SAC_B : SP_SAC_A);
    case MOB_WRAITH:
        if (m->sub == 1) return SP_WRAITH_BACK;
        return hypotf(m->x - G.px, m->y - G.py) < 1.8f ? SP_WRAITH_REACH : (fr ? SP_WRAITH_B : SP_WRAITH_A);
    case MOB_SKEL:
        if (m->state == MS_WALK && m->sub == 5) return m->sub_t < 0.35f ? SP_SKEL_D2 : SP_SKEL_D1;   /* getting up */
        return m->state == MS_DIE ? (m->t < 0.25f ? SP_SKEL_D1 : (m->t < 0.5f ? SP_SKEL_D2 : SP_SKEL_D3))
                                  : (m->throw_t > 0.0f ? SP_SKEL_T : (fr ? SP_SKEL_B : SP_SKEL_A));
    case MOB_SHIELD: {
        if (m->state == MS_DIE) return m->t < 0.25f ? SP_SKEL_D1 : (m->t < 0.5f ? SP_SKEL_D2 : SP_SKEL_D3);
        int o = m->shield > 0.0f ? 0 : SP_SHIELD_XA - SP_SHIELD_A;
        return o + (m->swing > 0.0f ? SP_SHIELD_S : (fr ? SP_SHIELD_B : SP_SHIELD_A));
    }
    case MOB_GHOST:
        *scale = 1.35f;                                            /* taller than a man */
        return m->state == MS_WALK ? (fr ? SP_GHOST_B : SP_GHOST_A) : (((int)(m->t * 14.0f)) & 1 ? SP_GHOST_D1 : SP_GHOST_D2);
    case MOB_WSKULL:
        *scale = 0.62f;
        if (m->state == MS_DIE) return SP_WSKULL_D;
        if (m->sub == 1) return m->sub_t > 2.2f && ((int)(m->sub_t * 16.0f) & 1) ? SP_WSKULL2 : (m->sub_t > 0.5f ? SP_WSKULL1 : SP_WSKULL0);
        return SP_WSKULL0;
    case MOB_SPIDER:
        *scale = 0.42f;
        if (m->state == MS_DIE) return SP_SPIDER_D;
        if (m->sub == 2) return SP_SPIDER_BITE;
        return ((int)(m->t * 10.0f)) & 1 ? SP_SPIDER_B : SP_SPIDER_A;
    default:
        return m->state == MS_DIE ? SP_BAT_D : ((int)(m->t * 9.0f) & 1 ? SP_BAT_B : SP_BAT_A);
    }
}

static int shot_sprite(const Shot *s, float *scale) {
    int fl = ((int)(G.t * 14.0)) & 1;
    *scale = 1.0f;
    switch (s->kind) {
    case AT_FROST: *scale = 0.7f; return SP_FROST;
    case AT_SPEAR: *scale = 0.8f; return SP_SPEAR;
    case AT_HELL: *scale = 1.3f; return fl ? SP_HELL_B : SP_HELL_A;
    case AT_WISPS: *scale = 0.6f; return SP_WISP;
    case AT_SPARKS: *scale = 0.6f; return SP_SPARK;
    case SHOT_REFLECT: return s->z > 0.5f ? (fl ? SP_SKULLFIRE_B : SP_SKULLFIRE_A) : SP_BONEFLY0 + (((int)(G.t * 14.0)) & 3);
    default: return fl ? SP_FIRE_B : SP_FIRE_A;
    }
}

/* the Backrooms' glitch: in bursts, bands of rows torn sideways, and some of them slipping colour,
 * a red copy one way and a blue the other; 0 when it is still. Creatures caught in a loop glitch most. */
static int glitch_rows(const Draw *d, int8_t *shift, uint8_t *slip) {
    uint32_t seed = (uint32_t)(int)(d->x * 7.0f) * 73856093u ^ (uint32_t)(int)(d->y * 7.0f) * 19349663u;
    int slot = (int)(G.t * 12.0);
    if (hash2(slot / 6, (int)(seed & 0xFFFF)) % 100 >= ((d->flags & DR_LOOPED) ? 70u : 35u)) return 0;
    for (int y = 0; y < TS;) {
        uint32_t h = hash2(slot, (int)(seed % 9973u) + y * 31);
        int band = 2 + (int)(h % 6), dx = (h >> 8) % 3 == 0 ? (int)((h >> 12) % 9) - 4 : 0, sl = (h >> 20) % 4 == 0;
        for (int k = 0; k < band && y < TS; k++, y++) { shift[y] = (int8_t)dx; slip[y] = (uint8_t)sl; }
    }
    return 1;
}

static void render_sprites(Frame *f, float dx, float dy, float horizon) {
    static Draw list[MAXDECO + MAXMOBS + MAXSHOTS + MAXBOOMS + MAXBONES + MAXITEMS + MAXBURNS + MAXPOOLS + MAXHAZ + 16];   /* and wisps, a double, after-images */
    int n = 0;
    float rx = -dy, ry = dx;
    #define ADD(X, Y, Z, S, GH) do { float vx_ = (X) - G.px, vy_ = (Y) - G.py, dp_ = vx_ * dx + vy_ * dy; \
        if (dp_ > 0.12f && dp_ < 24.0f && n < (int)(sizeof list / sizeof list[0])) list[n++] = (Draw){ X, Y, Z, dp_, S, (GH) > 0.0f ? (GH) : 1.0f, 0.0f, 1.0f, 0, 0, 0.0f, NULL, 0 }; } while (0)
    for (int i = 0; i < G.ndecos; i++) {
        const Deco *dc = &G.decos[i];
        static const float FPS[4] = { 0.0f, 0.0f, 3.0f, 9.0f };    /* by frame count: candles flicker, fire roars */
        if (dc->spr < 0) continue;                                 /* gone: risen, or never a statue */
        int spr = dc->spr + (dc->nfr > 1 ? (int)((float)G.t * FPS[dc->nfr & 3] + dc->phase) % dc->nfr : 0), n0 = n;
        float zx = 0.0f, ox = 0.0f, oy = 0.0f;
        if (dc->flags & (DF_FLIP | DF_FLOAT | DF_SINK | DF_LOOP)) {   /* out of place */
            if (dc->flags & DF_FLOAT) zx = 0.40f + 0.05f * sinf((float)G.t * 1.3f + dc->phase);
            if (dc->flags & DF_SINK) zx = (dc->flags & DF_FLIP) ? 0.25f : -0.22f;   /* most things fill the lower half of their picture */
            else if (dc->flags & DF_FLIP) zx = 0.0f;
            if (dc->flags & DF_LOOP) {                             /* a few steps forward, and back to where it began */
                float u = (float)G.t * 0.7f + dc->phase, k = 0.5f * (u - floorf(u)) - 0.25f;
                if ((int)dc->phase & 1) ox = k; else oy = k;
            }
        }
        ADD(dc->x + ox, dc->y + oy, zx, spr, 0);
        if (n > n0 && (dc->flags & (DF_FLIP | DF_FLOAT | DF_SINK | DF_LOOP))) {
            list[n - 1].flags = ((dc->flags & DF_FLIP) ? DR_FLIPV : 0) | ((dc->flags & DF_SINK) ? DR_CLIP : 0) | ((dc->flags & DF_LOOP) ? DR_GLITCH | DR_LOOPED : 0);
        }
        if (n > n0 && (dc->flags & DF_SWAY)) {                     /* a slow swing, and a shiver on top of it */
            list[n - 1].flags = DR_SWAY;
            list[n - 1].sway = 2.6f * sinf((float)G.t * 0.9f + dc->phase) + 0.6f * sinf((float)G.t * 2.3f + dc->phase * 1.7f);
        }
    }
    if (!G.shop && G.altar_x >= 0 && (G.altar_state == ALTAR_AWAKE || G.altar_state == ALTAR_READY || G.blessed)) {   /* the altar's fire, at full brightness */
        int n0 = n, fr = ((int)(G.t * 8.0)) & 1;
        ADD(G.altar_x + 0.5f, G.altar_y + 0.5f, 0.0f, (G.altar_state == ALTAR_READY ? SP_BFLAME_G0 : SP_BFLAME_V0) + fr, 0);
        if (n > n0) { list[n - 1].flags = DR_BRIGHT | (G.blessed ? DR_EDGE : 0); list[n - 1].depth -= 0.02f; }
    }
    if (G.shop && RUN.shrine > 0) {                                /* the cursed shrine: its glow in its rank's colour, its runes going round */
        int remap = QUAL_RAMP[RUN.shrine - 1], n0 = n, kinds[NDEF], nk = 0;
        ADD(SHRINE_X, SHRINE_Y, 0.0f, ((int)(G.t * 3.0)) & 1 ? SP_SHRINE_G1 : SP_SHRINE_G0, 0);
        if (n > n0) { list[n - 1].flags = DR_BRIGHT; list[n - 1].remap = remap; list[n - 1].depth -= 0.02f; }
        for (int k = 0; k < NDEF; k++) if (RUN.attuned >> k & 1) kinds[nk++] = k;
        for (int j = 0; j < (nk ? nk : 1); j++) {
            float a = (float)G.t * 0.7f + (float)j * TAU_F / (float)(nk ? nk : 1);
            int n1 = n;
            ADD(SHRINE_X + cosf(a) * 0.62f, SHRINE_Y - 0.14f + sinf(a) * 0.28f, 0.46f + 0.05f * sinf((float)G.t * 2.0f + (float)j),
                nk ? SP_SRUNE0 + kinds[j] : SP_SRUNE_X, 0);
            if (n > n1) { list[n - 1].flags = DR_BRIGHT; list[n - 1].remap = remap; list[n - 1].scale = 0.2f; }
        }
    }
    if (G.decoy_t > 0.0f && !G.shop) {                             /* an Exquisite Blink's double, fading where you stood */
        int n0 = n;
        ADD(G.decoy_x, G.decoy_y, 0.0f, SP_DOUBLE, fminf(0.8f, 0.15f + G.decoy_t / DECOY_T));
        if (n > n0) list[n - 1].flags = DR_BRIGHT;
    }
    for (int i = 0; i < MAXMOBS; i++) {
        Mob *m = &G.mobs[i];
        if (!m->alive || buried(m)) continue;
        float sc;
        int s = mob_sprite(m, &sc), n0 = n, statue = m->type == MOB_GARGOYLE && m->sub == 0;
        float gh = m->type == MOB_GHOST ? (m->state == MS_RECOIL ? 0.9f : 0.72f * (1.0f - m->fade) + 0.01f) : (m->type == MOB_SHADE ? shade_alpha(m) : 0.0f);
        int commune = G.commune_show > 0.0f && m->state == MS_WALK && hypotf(m->x - G.px, m->y - G.py) <= G.commune_r;
        if (m->type == MOB_SHADE && gh < 0.04f && !commune) continue;
        if (m->boss == 1 + BOSS_WRAITH && m->state == MS_WALK) {   /* into the stone and out; faint in the dark */
            float a = G.bww == 1 ? 1.0f - G.bww_t / 0.6f : (G.bww == 2 ? 0.0f : (G.bww == 3 ? G.bww_t / 0.5f : 1.0f));
            if (!wraith_lit(m->x, m->y)) a *= 0.35f;
            if (a < 0.03f) continue;
            gh = a;
        }
        ADD(m->x, m->y, m->z, s, commune && gh > 0.0f && gh < 0.04f ? 0.04f : gh);
        if (n == n0) continue;
        Draw *d = &list[n - 1];
        d->scale = statue ? 1.0f : sc * RANK_SIZE[m->rank];       /* a statue like the others: its rank doesn't show yet */
        if (m->boss) { d->scale *= BOSSES[m->boss - 1].scale; if (G.bstate != BS_FIGHT && m->state == MS_WALK) d->flags |= DR_EDGE; }   /* untouchable: a pale shell */
        if (m->small) d->scale *= 0.62f;
        if (m->rank != RK_I && KINDS[m->type].family >= 0 && !statue) d->lut = crawl_rank_lut[KINDS[m->type].family][m->rank];
        if (m->type == MOB_SPIDER && m->state == MS_WALK && m->sub == 0) d->flags = DR_EYES;
        if (m->type == MOB_HUNTER && m->state == MS_WALK && m->sub == 0) d->flags = DR_FLIPV;
        if (m->frozen > 0.0f && m->state == MS_WALK) d->flags |= DR_ICE;   /* only its eyes, in the dark of the web */
        if (G.ptheme == PT_BACKROOMS) d->flags |= DR_GLITCH;           /* nothing here is quite right */
        if (G.ptheme == PT_SEVERANCE) d->flags |= DR_LIT;              /* nor here: everything is too clean */
        if (m->boss && G.bstate != BS_FIGHT && m->state == MS_WALK) d->flags |= DR_EDGE;
        if (m->boss == 1 + BOSS_KNIGHT && m->state == MS_WALK) {
            d->flags |= DR_SHINE;
            if (G.bdash_age < 0.8f) {                              /* Swift Dodge: where it was a moment ago, fading */
                Draw keep = *d;
                for (int j = 0; j < 3; j++) {
                    float f = (float)j / 3.0f, a = (0.5f - 0.12f * (float)j) * (1.0f - G.bdash_age / 0.8f);
                    int n1 = n;
                    if (a > 0.04f) ADD(G.bdash_x0 + (G.bdash_x1 - G.bdash_x0) * f, G.bdash_y0 + (G.bdash_y1 - G.bdash_y0) * f, m->z, s, a);
                    if (n > n1) { list[n - 1].scale = keep.scale; list[n - 1].lut = keep.lut; }
                }
                d = &list[n0];                                     /* the list may have moved on: the knight is still at n0 */
            }
        }
        if (m->boss == 1 + BOSS_SPIDER && G.bmend_t > 0.0f && m->state == MS_WALK) d->flags |= DR_MEND;   /* its Mend Rune */
        if (m->boss == 1 + BOSS_WRAITH && G.bslip_t > 0.9f) d->flags |= DR_RIPPLE;
        if (m->boss == 1 + BOSS_DEACON) {
            d->lut = crawl_rank_lut[KINDS[MOB_DEACON].family][RK_V];   /* black robes */
            if (G.bpose == BP_LITANY && G.bpose_t > 0.0f && m->state == MS_WALK) d->flags |= DR_MEND;   /* the Litany's green light */
            if (G.bward > 0.0f && G.bstate == BS_FIGHT && m->state == MS_WALK) {   /* his Bone Ward, wheeling about him */
                for (int k = 0; k < 6; k++) {
                    float a = (float)G.t * 2.2f + (float)k * (TAU_F / 6.0f);
                    int n1 = n;
                    ADD(m->x + cosf(a) * 0.9f, m->y + sinf(a) * 0.9f, 0.25f + 0.15f * sinf(a * 2.0f), SP_BONEFLY0 + (k & 3), 0);
                    if (n > n1) list[n - 1].scale = 0.55f;
                }
                d = &list[n0];
            }
        }
        if (commune) d->flags = (d->flags & ~DR_EYES) | DR_COMMUNE;   /* Commune: its shape, wherever it is */
        if (m->state == MS_DIE && m->sub == 77) {                  /* drawn in to a point */
            float k = smoothstepf(0.1f, 0.75f, m->t);
            d->z += sc * 0.5f * k; d->scale = sc * (1.0f - k) + 0.02f; d->flags = DR_IMPLODE;
        }
    }
    for (int i = 0; i < MAXHAZ && !PK.depth && !G.shop; i++) {      /* the hazards standing up off the floor */
        const Hazard *h = &G.haz[i];
        if (!h->alive) continue;
        int ph = hz_phase(h), n0 = n;
        float kf = h->off > 0.0f ? 1.0f - clampf((h->t - h->warn - h->on) / h->off, 0.0f, 1.0f) : 0.0f;
        switch (h->kind) {
        case HZ_ROCK:
            if (ph == 0) {
                ADD(h->x, h->y, 0.0f, SP_GRIT0 + (((int)(G.t * 8.0)) & 1), 0);
                if (h->t > h->warn - 0.28f) { int n1 = n; ADD(h->x, h->y, (h->warn - h->t) / 0.28f * 0.8f, SP_ROCK, 0); if (n > n1) list[n - 1].scale = 0.8f; }
            } else ADD(h->x, h->y, -0.15f * (1.0f - kf), SP_RUBBLE, fmaxf(0.05f, kf));
            break;
        case HZ_FIRE:
            if (ph == 1)
                for (int c = 0; c < h->n; c++) {
                    int n1 = n;
                    ADD(h->cx[c] + 0.5f, h->cy[c] + 0.5f, 0.0f, SP_ERUPT0 + ((((int)(G.t * 10.0)) + c) & 1), 0);
                    if (n > n1) { list[n - 1].flags = DR_BRIGHT; list[n - 1].scale = 0.85f; }
                }
            break;
        case HZ_VOID: {
            float a = ph == 0 ? 0.12f + 0.15f * clampf(h->t / h->warn, 0.0f, 1.0f) : (ph == 1 ? 0.55f : 0.55f * kf);
            ADD(h->x, h->y, 0.0f, SP_VOIDCOL, fmaxf(0.03f, a));
            if (n > n0) list[n - 1].scale = 0.8f + 0.4f * h->r;
            break;
        }
        case HZ_GRASP:
            if (ph > 0) { float k = clampf((h->t - h->warn) / 0.25f, 0.0f, 1.0f); ADD(h->x, h->y, -0.45f * (1.0f - k), SP_HANDS, fmaxf(0.05f, kf)); }
            break;
        default: break;
        }
    }
    for (int i = 0; i < MAXSHOTS; i++) {
        const Shot *s = &G.shots[i];
        if (!s->alive || (s->x - G.px) * dx + (s->y - G.py) * dy < 0.6f) continue;   /* the hand covers it leaving */
        float sc;
        int sp = shot_sprite(s, &sc), n0 = n;
        ADD(s->x, s->y, -0.02f + (1.0f - sc) * 0.5f, sp, 0);
        if (n > n0) list[n - 1].scale = sc;
    }
    for (int i = 0; i < MAXBOOMS; i++) {
        const Boom *b = &G.booms[i];
        if (!b->alive) continue;
        float sc = b->big ? b->r * 0.8f : 1.0f;
        int n0 = n;
        ADD(b->x, b->y, b->big ? -0.1f : 0.0f, SP_BOOM1 + clampi((int)(b->t / 0.2f), 0, 2), 0);
        if (n > n0) list[n - 1].scale = sc;
    }
    for (int i = 0; i < MAXPOOLS; i++) {                           /* a drowned one's foul pool */
        const Pool *p = &G.pools[i];
        if (p->alive) ADD(p->x, p->y, 0.0f, SP_DROWN_POOL, 1.0f - smoothstepf(4.0f, 5.0f, p->t));
    }
    for (int i = 0; i < MAXBURNS; i++) {
        const Burn *b = &G.burns[i];
        if (!b->alive) continue;
        int n0 = n;
        ADD(b->x, b->y, 0.0f, ((int)(G.t * 9.0) + i) & 1 ? SP_BURN_B : SP_BURN_A, 1.0f - smoothstepf(1.6f, 2.5f, b->t));
        if (n > n0) list[n - 1].scale = 0.9f;
    }
    for (int i = 0; i < MAXBONES; i++) {
        const Bone *b = &G.bones[i];
        if (!b->alive) continue;
        if (b->kind == MIS_FIRE) {
            int n0 = n;
            ADD(b->x, b->y, 0.12f, ((int)(G.t * 14.0)) & 1 ? SP_SKULLFIRE_B : SP_SKULLFIRE_A, 0);
            if (n > n0) list[n - 1].scale = 0.8f;
        } else if (b->kind == MIS_EMBER) {                         /* lobbed: up, and down where you were going */
            float k = b->life0 > 0.0f ? 1.0f - b->life / b->life0 : 0.5f;
            int n0 = n;
            ADD(b->x, b->y, 0.05f + 0.4f * sinf(PI_F * k), ((int)(G.t * 12.0)) & 1 ? SP_EMBER_B : SP_EMBER_A, 0);
            if (n > n0) list[n - 1].scale = 0.45f;
        } else if (b->kind == MIS_WEB) {                           /* a glob of web */
            int n0 = n;
            ADD(b->x, b->y, 0.15f, SP_SAC_A, 0);
            if (n > n0) list[n - 1].scale = 0.35f;
        } else if (b->kind == MIS_TURNED) {                        /* your spell, coming back */
            Shot fake = { 0 };
            float sc;
            fake.kind = b->atk;
            int n0 = n;
            ADD(b->x, b->y, 0.1f, shot_sprite(&fake, &sc), 0);
            if (n > n0) list[n - 1].scale = sc;
        } else ADD(b->x, b->y, 0.04f, SP_BONEFLY0 + (((int)b->spin) & 3), 0);
    }
    for (int i = 0; i < G.nitems; i++) {
        const Item *it = &G.items[i];
        if (!it->alive) continue;
        static const int SPR[] = { SP_COINS_A, SP_CHALICE, SP_GEMS, SP_CROWN, SP_CHEST, SP_KEY0, SP_POTION, SP_SOULURN, SP_RINGITEM, SP_ITEM_BONE, SP_SCRIP };
        float ph = (float)G.t + it->phase, z = 0.0f;
        int sp = SPR[it->type];
        if (it->type == IT_COINS && ((int)(ph * 2.5f) & 1)) sp = SP_COINS_B;
        if (it->type == IT_KEY) { sp = key_sprite(it->color, ph); z = 0.14f + 0.04f * sinf(ph * 2.5f); }
        int n0 = n;
        ADD(it->x, it->y, z, sp, 0);
        if (n > n0 && it->type == IT_KEY && hypotf(it->x - G.px, it->y - G.py) <= gear_ability(&RUN.gear, AB_KEYMAKER, 0)) {
            static const int GLINT[NKEYS] = { R_GOLD, R_BONE, R_BLOOD };
            list[n - 1].flags |= DR_GLINT; list[n - 1].glint = GLINT[it->color];
        }
        if (n > n0 && it->type != IT_KEY) { float u = ph * 0.6f - floorf(ph * 0.6f); list[n - 1].twinkle = u < 0.1f ? sinf(u / 0.1f * PI_F) : 0.0f; }
    }
    if (G.shop) {
        build_cat();
        int n0 = n;
        ADD(CAT_X, CAT_Y, 0.0f, SP_PEDESTAL, 0);
        if (n > n0) { list[n - 1].scale = CAT_S; list[n - 1].flags = DR_CAT; }
        for (int i = 0; i < G.ped_n; i++) {
            if (G.offer_gone[i]) continue;
            const Offer *o = &G.offers[i];
            int sp = offer_sprite(o);
            float sc = 0.42f, z = 0.55f + 0.03f * sinf((float)G.t * 2.0f + i * 2.1f) - (TS - 1 - spr_bottom(sp)) / (float)TS * sc;
            n0 = n;
            ADD(G.ped_x[i], G.ped_y[i], z, sp, 0);
            if (n > n0) {
                Draw *d = &list[n - 1];
                d->scale = sc; d->remap = offer_color(o);
                float u = (float)G.t * 0.5f + i * 0.33f; u -= floorf(u);
                d->twinkle = u < 0.12f ? sinf(u / 0.12f * PI_F) : 0.0f;
            }
        }
    }
    #undef ADD
    qsort(list, (size_t)n, sizeof list[0], cmp_draw);
    float bfs_dark = G.bfs_t > 0.0f ? 1.0f - smoothstepf(0.3f, 1.4f, G.bfs_t) : 0.0f;
    for (int i = 0; i < n; i++) {
        Draw *d = &list[i];
        float vx = d->x - G.px, vy = d->y - G.py;
        float lat = vx * rx + vy * ry;
        float size = FOCAL / d->depth * d->scale;
        float scx = W / 2 + FOCAL * lat / d->depth;
        float top = horizon - (d->scale - G.camz + d->z) * FOCAL / d->depth;
        float left = scx - size / 2;
        if (d->flags & DR_EYES) {                                  /* two red points, blinking now and then */
            if (fmodf((float)G.t * 0.7f + d->x * 3.1f, 4.0f) < 0.12f) continue;
            int big = size > 40.0f, gap = (int)fmaxf(1.0f, 3.0f * size / TS);
            for (int e = 0; e < 2; e++) {
                int sx = (int)(left + 32.0f * size / TS) + (e ? gap : -gap - big), sy = (int)(top + 30.5f * size / TS);
                for (int yy = 0; yy <= big; yy++)
                    for (int xx = 0; xx <= big; xx++) {
                        int px = sx + xx, py = sy + yy;
                        if (px >= 0 && px < W && py >= 0 && py < H && d->depth < zbuf[px]) f->pix[py * W + px] = (uint8_t)(R_BLOOD + 13 + (int)(2.5f * (0.5f + 0.5f * sinf((float)G.t * 2.0f + d->x))));
                    }
            }
            continue;
        }
        if (d->flags & DR_IMPLODE && d->scale < 0.06f) {           /* the last point of light */
            int sx = (int)scx, sy = (int)(top + size * 0.5f), arm = (int)clampf(6.0f / d->depth, 1.0f, 6.0f);
            for (int k = -arm; k <= arm; k++) {
                int xs[2] = { sx + k, sx }, ys[2] = { sy, sy + k };
                for (int j = 0; j < 2; j++)
                    if (xs[j] >= 0 && xs[j] < W && ys[j] >= 0 && ys[j] < H && d->depth < zbuf[xs[j]]) f->pix[ys[j] * W + xs[j]] = abs(k) > arm / 2 ? (uint8_t)(R_GLOW + 10) : (uint8_t)C_WHITE;
            }
            continue;
        }
        int x0 = clampi((int)ceilf(left - 0.5f), 0, W), x1 = clampi((int)ceilf(left + size - 0.5f), 0, W);
        int y0 = clampi((int)ceilf(top - 0.5f), 0, H), y1 = clampi((int)ceilf(top + size - 0.5f), 0, H);
        if (x0 >= x1 || y0 >= y1) continue;
        if (d->flags & DR_CAT) {                                   /* the merchant: a bigger, row-major picture */
            float light = fminf(1.3f, lm_at(d->x + 0.4f, d->y) * atten(d->depth) + 0.25f), cstep = CAT_W / size;
            for (int x = x0; x < x1; x++) {
                if (d->depth >= zbuf[x]) continue;
                int tx = clampi((int)((x + 0.5f - left) * cstep), 0, CAT_W - 1);
                float ty = (y0 + 0.5f - top) * cstep;
                for (int y = y0; y < y1; y++, ty += cstep) {
                    uint8_t c = cat_buf[clampi((int)ty, 0, CAT_H - 1) * CAT_W + tx];
                    if (c != C_CLEAR) f->pix[y * W + x] = shade(c, light, x, y);
                }
            }
            continue;
        }
        float light = lm_at(d->x, d->y) * atten(d->depth) * (1.0f - 0.85f * bfs_dark);
        if (d->flags & DR_LIT) light = fminf(1.3f, light * 1.3f + 0.15f);
        float fog = fog_amount(d->x, d->y, 1.0f - expf(-d->depth * 0.28f));
        const uint8_t *img = crawl_spr[d->spr];
        float step = TS / size;
        if (d->flags & DR_SWAY) {                                  /* hung from the top: the lower it is, the further it swings */
            for (int x = x0; x < x1; x++) {
                if (d->depth >= zbuf[x]) continue;
                float tx0 = (x + 0.5f - left) * step, ty = (y0 + 0.5f - top) * step;
                for (int y = y0; y < y1; y++, ty += step) {
                    int tyi = clampi((int)ty, 0, TS - 1), tx = (int)(tx0 - d->sway * tyi / (float)TS);
                    if (tx < 0 || tx >= TS) continue;
                    uint8_t c = img[tx * TS + tyi];
                    if (c != C_CLEAR) f->pix[y * W + x] = fogged(shade(c, light, x, y), fog, light, x, y);
                }
            }
            continue;
        }
        int com = d->flags & (DR_COMMUNE | DR_GLINT);              /* seen where it can't be: Commune's red, a key's glint */
        float ck = (d->flags & DR_COMMUNE) ? fminf(1.0f, fminf(G.commune_show, G.commune_len - G.commune_show) * 5.0f + 0.2f) : (com ? 0.6f : 0.0f);
        if (d->flags & DR_CLIP) {                                  /* half sunk: cut where the floor and ceiling are */
            y0 = clampi(y0, (int)ceilf(horizon - (1.0f - G.camz) * FOCAL / d->depth), H);
            y1 = clampi(y1, 0, (int)ceilf(horizon + G.camz * FOCAL / d->depth));
            if (y0 >= y1) continue;
        }
        static int8_t gshift[TS];
        static uint8_t gslip[TS];
        int glitch = (d->flags & DR_GLITCH) && glitch_rows(d, gshift, gslip);
        for (int x = x0; x < x1; x++) {
            int behind = d->depth >= zbuf[x];
            if (behind && !com) continue;
            int tx = clampi((int)((x + 0.5f - left) * step), 0, TS - 1);
            const uint8_t *col = &img[tx * TS];
            float ty = (y0 + 0.5f - top) * step;
            for (int y = y0; y < y1; y++, ty += step) {
                int tyi = clampi((int)ty, 0, TS - 1), sy = (d->flags & DR_FLIPV) ? TS - 1 - tyi : tyi;
                uint8_t c = col[sy];
                if (glitch) {
                    int gx = tx - gshift[tyi];
                    c = gx >= 0 && gx < TS ? img[gx * TS + sy] : C_CLEAR;
                    if (c == C_CLEAR && gslip[tyi] && !behind) {   /* the colours slipping off it */
                        if (gx - 2 >= 0 && gx - 2 < TS && img[(gx - 2) * TS + sy] != C_CLEAR) f->pix[y * W + x] = (uint8_t)(R_BLOOD + 11);
                        else if (gx + 2 >= 0 && gx + 2 < TS && img[(gx + 2) * TS + sy] != C_CLEAR) f->pix[y * W + x] = (uint8_t)(R_GLOW + 9);
                        continue;
                    }
                }
                if (c == C_CLEAR) {
                    if ((d->flags & DR_EDGE) && !behind && ((x + y + (int)(G.t * 10.0)) & 1)
                        && ((tyi < TS - 1 && col[tyi + 1] != C_CLEAR) || (tx > 0 && img[(tx - 1) * TS + tyi] != C_CLEAR)
                            || (tx < TS - 1 && img[(tx + 1) * TS + tyi] != C_CLEAR)))
                        f->pix[y * W + x] = C_WHITE;               /* the blessing: a white edge about its fire */
                    continue;
                }
                int faded = d->alpha < 1.0f && d->alpha * 16.0f <= BAYER[(y + (int)(G.t * 20.0)) & 3][x & 3] + 0.5f;
                if (behind || (com && faded)) {
                    if (d->flags & DR_GLINT) { if (((x + y + (int)(G.t * 6.0)) & 3) < 2) f->pix[y * W + x] = c; }   /* its own colours, shimmering */
                    else if (ck * 16.0f > BAYER[y & 3][x & 3]) f->pix[y * W + x] = (uint8_t)(R_BLOOD + 9 + ((x + y) & 1) * 3);
                    continue;
                }
                if (faded) continue;
                if (d->lut) c = d->lut[c];
                if ((d->flags & DR_SHINE) && c >= R_IRON && c < R_IRON + 16) {   /* polished: brighter, and the glint going by */
                    int v = c - R_IRON;
                    if (G.bmirror_t > 0.0f) { f->pix[y * W + x] = ((x + y) & 1) ? (uint8_t)C_WHITE : (uint8_t)(R_GHOST + 14); continue; }   /* the Mirror Rune */
                    float u = ((float)tx + (float)(TS - tyi)) / (2.0f * TS) - (fmodf((float)G.t * 0.3f, 1.8f) - 0.4f);
                    if (fabsf(u) < 0.035f && v > 3) { f->pix[y * W + x] = fabsf(u) < 0.012f ? (uint8_t)C_WHITE : (uint8_t)(R_IRON + 15); continue; }
                    c = (uint8_t)(R_IRON + imin(15, v + 2 + v / 4));
                }
                if ((d->flags & DR_MEND) && ((x + y + (int)(G.t * 8.0)) & 3) == 0) { f->pix[y * W + x] = (uint8_t)(R_MOSS + 12 + ((x >> 1) & 3)); continue; }   /* mending: green */
                if (d->remap && c >= R_PURPLE && c < R_PURPLE + 16) c = ramp_at(d->remap, c - R_PURPLE);
                if (d->flags & DR_IMPLODE) { f->pix[y * W + x] = c == 0 ? 0 : (uint8_t)(R_GLOW + clampi(6 + (int)((c & 15) * 0.6f), 6, 15)); continue; }
                if ((d->flags & DR_ICE) && c != 0 && ((x + y) & 1)) {    /* rimed with frost */
                    f->pix[y * W + x] = fogged((uint8_t)(R_GLOW + clampi(2 + (int)(((c - 1) & 15) * 0.5f + light * 4.0f), 2, 12)), fog, light, x, y);
                    continue;
                }
                f->pix[y * W + x] = (d->flags & DR_BRIGHT) ? c : fogged(shade(c, light, x, y), fog, light, x, y);
            }
        }
        if (((d->flags & DR_SHINE) && G.bmirror_t > 0.0f) || (d->flags & DR_RIPPLE)) {   /* the Mirror Rune's silver ring; Time Slip's grey */
            int grey = (d->flags & DR_RIPPLE) != 0;
            float k = grey ? (1.5f - G.bslip_t) / 0.6f : 1.0f - G.bmirror_t / 0.35f, rx = size * (0.22f + (grey ? 0.9f : 0.4f) * k), ry = rx * 0.85f;
            float cx = left + size * 0.5f, cy = top + size * 0.55f;
            for (int a = 0; a < 96; a++) {
                if (k > 0.5f && ((a + (int)(G.t * 30.0)) & 1)) continue;   /* thinning as it goes */
                int px = (int)(cx + cosf(a * (TAU_F / 96.0f)) * rx), py = (int)(cy + sinf(a * (TAU_F / 96.0f)) * ry);
                if (px >= 0 && px < W && py >= 0 && py < H && d->depth < zbuf[px] + 0.6f)
                    f->pix[py * W + px] = grey ? (uint8_t)(R_STONE + 9 + (a & 3)) : (a & 3) ? (uint8_t)C_WHITE : (uint8_t)(R_GHOST + 12);
            }
        }
        if (d->flags & DR_GLINT) {                                 /* Keymaker: a sparkle where the key lies, through the stone */
            float pulse = 0.5f + 0.5f * sinf((float)G.t * 5.0f + d->x);
            int cx = (int)(left + size * 0.5f), cy = (int)(top + size * 0.62f), arm = 3 + (int)(pulse * 4.0f + fminf(size / 12.0f, 4.0f));
            for (int k = -arm; k <= arm; k++) {
                int a = abs(k);
                uint8_t c = a <= 1 ? (uint8_t)C_WHITE : ramp_at(d->glint, 14 - a);
                int px[4] = { cx + k, cx, cx + k / 2, cx + k / 2 }, py[4] = { cy, cy + k, cy + k / 2, cy - k / 2 };
                for (int j = 0; j < (a <= arm / 2 ? 4 : 2); j++)
                    if (px[j] >= 0 && px[j] < W && py[j] >= 0 && py[j] < H) f->pix[py[j] * W + px[j]] = c;
            }
        }
        if (d->twinkle > 0.0f && crawl_glint[d->spr][0] >= 0) {     /* a glint off the gold */
            int gx = (int)(left + (crawl_glint[d->spr][0] + 0.5f) * size / TS), gy = (int)(top + (crawl_glint[d->spr][1] + 0.5f) * size / TS);
            int arm = (int)(d->twinkle * fmaxf(1.0f, size / 28.0f));
            for (int k = -arm; k <= arm; k++) {
                int xs[2] = { gx + k, gx }, ys[2] = { gy, gy + k };
                for (int j = 0; j < 2; j++)
                    if (xs[j] >= 0 && xs[j] < W && ys[j] >= 0 && ys[j] < H && d->depth < zbuf[xs[j]]) f->pix[ys[j] * W + xs[j]] = abs(k) == arm && arm > 1 ? (uint8_t)(R_GOLD + 11) : (uint8_t)C_WHITE;
            }
        }
    }
    for (int i = 0; i < MAXMOBS; i++) {                            /* spiders hang by a thread */
        const Mob *m = &G.mobs[i];
        if (!m->alive || m->type != MOB_SPIDER || (m->state == MS_WALK && m->sub == 0) || (m->state == MS_DIE && m->sub != 9)) continue;
        if (m->state == MS_DIE && m->t > 0.25f) continue;          /* snapped */
        float L = lm_at(m->x, m->y);
        uint8_t c = (uint8_t)(R_BONE + clampi((int)(4.0f + L * 6.0f), 3, 11));
        line3d(f, dx, dy, horizon, m->ax, m->ay, 0.99f, m->x, m->y, m->z + 0.42f * 0.7f, c, 0, 1);
    }
}

/* ---- the merchant's wares: prices over the pedestals, a sparkle when one is taken ---- */
static void render_shop_tags(Frame *f, float dx, float dy, float horizon) {
    int order[NPED], n = 0, box[NPED][4], nb = 0;                  /* nearest first; a tag that would cover a nearer one waits */
    for (int i = 0; i < G.ped_n; i++) order[n++] = i;
    for (int a = 1; a < n; a++) for (int b = a; b > 0; b--) {
        float da = hypotf(G.ped_x[order[b]] - G.px, G.ped_y[order[b]] - G.py), db = hypotf(G.ped_x[order[b - 1]] - G.px, G.ped_y[order[b - 1]] - G.py);
        int look = order[b] == G.shop_look;                        /* the one looked at comes first of all */
        if (look || (da < db && order[b - 1] != G.shop_look)) { int t = order[b]; order[b] = order[b - 1]; order[b - 1] = t; } else break;
    }
    for (int oi = 0; oi < n; oi++) {
        int i = order[oi];
        float sx, sy, dep;
        if (!project(G.ped_x[i], G.ped_y[i], i == G.shop_pick && G.sparkle > 0.0f ? 0.78f : 1.04f, dx, dy, horizon, &sx, &sy, &dep)) continue;
        int x = (int)sx;
        if (x < 0 || x >= W || dep >= zbuf[x] + 0.3f) continue;
        if (G.offer_gone[i]) {
            if (i != G.shop_pick || G.sparkle <= 0.0f) continue;
            float k = 0.8f - G.sparkle;
            for (int p = 0; p < 18; p++) {                         /* a burst of light where it was */
                float a = p * TAU_F / 18 + k * 2.0f, r = k * (40.0f + 12.0f * (p & 1)) / fmaxf(dep, 0.8f);
                int px = (int)(sx + cosf(a) * r), py = (int)(sy + sinf(a) * r * 0.8f);
                if (px >= 0 && px < W && py >= 0 && py < H) f->pix[py * W + px] = p % 3 == 0 ? (uint8_t)C_WHITE : (uint8_t)(R_GLOW + 12);
            }
            continue;
        }
        const Offer *o = &G.offers[i];
        char tag[16];
        if (o->price == 0) snprintf(tag, sizeof tag, "FREE");
        else snprintf(tag, sizeof tag, "$%d", o->price);
        int w = gfx_text_width(tag, 1), tx = x - w / 2, ty = (int)sy - 10, hit = 0;
        if (ty < 0 || ty > H - 60) continue;
        for (int k = 0; k < nb; k++)
            hit |= tx < box[k][2] + 3 && tx + w + 3 > box[k][0] && ty - 10 < box[k][3] && ty + 9 > box[k][1];
        if (hit) continue;
        box[nb][0] = tx; box[nb][1] = ty - 10; box[nb][2] = tx + w; box[nb][3] = ty + 9; nb++;
        uint8_t col = o->price <= RUN.treasure ? (uint8_t)(R_GOLD + 12) : (uint8_t)(R_BLOOD + 11);
        gfx_text8(f, tx + 1, ty + 1, tag, 0, 1);
        gfx_text8(f, tx, ty, tag, col, 1);
        const char *over = G.shop_held == i || G.offer_back[i] ? "HELD" : (o->rare && ((int)(G.t * 3.0) & 1) ? "RARE" : NULL);
        if (over) { int rw = gfx_text_width(over, 1); gfx_text8(f, x - rw / 2 + 1, ty - 9, over, 0, 1); gfx_text8(f, x - rw / 2, ty - 10, over, over[0] == 'H' ? (uint8_t)(R_GLOW + 12) : (uint8_t)C_WHITE, 1); }
    }
}

/* The caster's face, Wolfenstein style: bloodier as health runs out, glancing about, flinching
 * towards a blow, grinning at loot. The hood is the one it wears. */
static void draw_face(Frame *f, int ox, int oy, int sub) {
    const Gear *g = &RUN.gear;
    float hpf = G.max_hp > 0 ? (float)G.hp / (float)G.max_hp : 0.0f;
    int stage = clampi((int)((1.0f - hpf) * 7.0f), 0, 6);
    int fr = G.dead || G.hp <= 0 ? 29 : (G.grin_t > 0.0f ? 28 : stage * 4 + sub);
    const Piece *hp = &g->worn[SL_HOOD];                           /* the caster's own purple, or the hood's quality */
    int hood = hp->kind ? QUAL_RAMP[hp->quality] : R_PURPLE, shade = hp->kind && hp->quality == Q_COMMON ? -3 : 0;
    int trim = !hp->kind || hp->anointed ? (hood == R_GOLD ? R_BONE : R_GOLD) : R_WOOD;
    const uint8_t *img = crawl_face_img[fr];
    for (int y = 0; y < FACE_H; y++)
        for (int x = 0; x < FACE_W; x++) {
            uint8_t c = img[y * FACE_W + x];
            if (c == C_CLEAR) continue;
            int sx = ox + x, sy = oy + y;
            if (sx < 0 || sx >= W || sy < 0 || sy >= H) continue;
            if (c >= R_PURPLE && c < R_PURPLE + 16) c = ramp_at(hood, c - R_PURPLE + shade);
            else if (c >= R_GOLD && c < R_GOLD + 13 && trim != R_GOLD) c = (uint8_t)(trim + clampi(c - R_GOLD + 3, 0, 15));
            else if (c >= R_SKIN && c < R_SKIN + 16 && ((x + y) & 1)) {
                if (G.poison_t > 0.0f) c = (uint8_t)(R_MOSS + clampi(c - R_SKIN - 1, 0, 15));          /* green about the gills */
                else if (G.slow_t > 0.0f) c = (uint8_t)(R_GHOST + clampi(c - R_SKIN - 2, 0, 15));
            }
            f->pix[sy * W + sx] = c;
        }
}

static int face_sub(void) {
    if (G.face_t > FACE_T - 0.7f) return G.face_side == 0 ? 3 : (G.face_side < 0 ? 1 : 2);   /* the flinch */
    uint32_t h = hash2((int)(G.t / 1.3), 977);                     /* looking about, now and then */
    return h % 10 < 6 ? 0 : (h % 10 < 8 ? 1 : 2);
}

static void bevel(Frame *f, int x, int y, int w, int h, int fill) {
    gfx_rect(f, x, y, w, h, fill);
    for (int i = 0; i < w; i++) { gfx_px(f, x + i, y, R_IRON + 1); gfx_px(f, x + i, y + h - 1, R_IRON + 9); }
    for (int i = 0; i < h; i++) { gfx_px(f, x, y + i, R_IRON + 1); gfx_px(f, x + w - 1, y + i, R_IRON + 9); }
}

/* the status bar, in the antechamber only (nothing stays put on an OLED for long) */
static void key_icon(Frame *f, int kx, int ky, int color);
static void draw_coin(Frame *f, int x, int y);
static int small_digits(Frame *f, int x, int y, int n, uint8_t c) {   /* 3 by 5 figures, for the keys' box; returns the width */
    static const char *const D[10] = { "####.##.##.####", ".#.##..#..#.###", "###..#####..###", "###..#.##..####", "#.##.####..#..#",
                                       "####..###..####", "####..####.####", "###..#..#.#..#.", "####.#####.####", "####.####..####" };
    char b[12];
    snprintf(b, sizeof b, "%d", n);
    int w = (int)strlen(b) * 4 - 1;
    for (int i = 0; b[i] && f; i++)
        for (int k = 0; k < 15; k++) if (D[b[i] - '0'][k] == '#') gfx_px(f, x + i * 4 + k % 3, y + k / 3, c);
    return w;
}
static void render_statusbar(Frame *f) {
    int y0 = H - 44 + (int)((1.0f - smoothstepf(0.0f, 1.0f, G.bar)) * 46.0f + 0.5f);   /* it slides down out of sight */
    if (y0 >= H) return;
    gfx_rect(f, 0, y0, W, 44, R_IRON + 4);
    for (int x = 0; x < W; x++) { gfx_px(f, x, y0, R_IRON + 9); gfx_px(f, x, y0 + 1, R_IRON + 6); }
    char num[24];
    struct { int x, w; const char *label; } P[5] = { { 4, 40, "LEVEL" }, { 48, 56, "GOLD" }, { 108, 50, "HEALTH" }, { 206, 42, "SOULS" }, { 252, 48, "SPELL" } };
    for (int i = 0; i < 5; i++) {
        bevel(f, P[i].x, y0 + 4, P[i].w, 37, R_IRON + 2);
        gfx_text8(f, P[i].x + (P[i].w - gfx_text_width(P[i].label, 1)) / 2, y0 + 7, P[i].label, R_IRON + 12, 1);
        uint8_t col = C_WHITE;
        switch (i) {
        case 0: snprintf(num, sizeof num, "%d", G.level); break;
        case 1: snprintf(num, sizeof num, "$%d", G.shop ? G.shop_gold : RUN.treasure); col = R_GOLD + 12; break;
        case 2: snprintf(num, sizeof num, "%d%%", G.max_hp ? G.hp * 100 / G.max_hp : 0); col = G.hp * 4 <= G.max_hp ? R_BLOOD + 13 : (G.poison_t > 0.0f ? R_MOSS + 13 : C_WHITE); break;
        case 3: spirit_text(num, sizeof num); col = R_GLOW + 12; break;
        default: {                                                 /* the spell in hand, by name and tier */
            int sp = clampi(G.spell, 0, NATK - 1), t = RUN.gear.atk[sp];
            uint8_t sc = ramp_at(gear_color(GK_ATK, sp), 13);
            gfx_text8(f, P[i].x + (P[i].w - gfx_text_width(ATK_SHORT[sp], 1)) / 2, y0 + 19, ATK_SHORT[sp], sc, 1);
            if (sp == AT_BFS) snprintf(num, sizeof num, "-%d", bfs_cost(t));
            else { char nm[40]; gear_item_name(GK_ATK, sp, t, nm, sizeof nm); const char *r = strrchr(nm, ' '); snprintf(num, sizeof num, "%s", t > 0 && r ? r + 1 : ""); }
            if (sp == AT_SPEAR) {                                  /* the bones to throw, over the label */
                char bn[8];
                snprintf(bn, sizeof bn, "%d", RUN.gear.bones);
                int bw = gfx_text_width(bn, 1), bx = P[i].x + (P[i].w - bw - 12) / 2, by = y0 + 7;
                gfx_rect(f, P[i].x + 2, by - 1, P[i].w - 4, 9, R_IRON + 2);
                uint8_t bc = RUN.gear.bones > 0 ? (uint8_t)(R_BONE + 14) : (uint8_t)(R_BLOOD + 13);
                static const char *BONE[3] = { "##.....##", ".#######.", "##.....##" };   /* a bone */
                for (int r = 0; r < 3; r++) for (int k = 0; k < 9; k++) if (BONE[r][k] == '#') gfx_px(f, bx + k, by + 2 + r, bc);
                gfx_text8(f, bx + 12, by, bn, bc, 1);
            }
            gfx_text8(f, P[i].x + (P[i].w - gfx_text_width(num, 1)) / 2, y0 + 30, num, R_IRON + 13, 1);
            continue;
        }
        }
        gfx_text16(f, P[i].x + (P[i].w - gfx_text_width(num, 1)) / 2, y0 + 19, num, col, 1);
    }
    bevel(f, 304, y0 + 4, 12, 37, R_IRON + 2);                     /* the keys in hand, and under them the Cursed Coins */
    for (int c = 0; c < NKEYS; c++) if (G.has_key[c]) key_icon(f, 306, y0 + 7 + c * 7, c);
    if (RUN.coins > 0) {
        draw_coin(f, 306, y0 + 27);
        int w = small_digits(NULL, 0, 0, RUN.coins, 0);
        small_digits(f, 304 + (12 - w) / 2, y0 + 35, RUN.coins, R_GOLD + 10);
    }
    bevel(f, 162, y0 + 1, 40, 43, 0);
    draw_face(f, 166, y0 + 3, face_sub());
}

/* 8x8 type wrapped at spaces to cols a line, lh px apart; returns the lines it took (no frame: only counts) */
static int glued(const char *s, int i) {                          /* a number keeps its unit: "3.0 S" */
    if (i <= 0 || s[i - 1] < '0' || s[i - 1] > '9') return 0;
    int k = i + 1;
    while (s[k] && s[k] != ' ' && s[k] != ',' && s[k] != '.') k++;
    return k - (i + 1) <= 2;
}
static int text_wrap_ex(Frame *f, int x, int y, const char *s, int cols, int col, int maxlines, int lh) {
    int lines = 0;
    while (*s && lines < maxlines) {
        int n = (int)strlen(s), cut = n;
        if (n > cols) { cut = cols; while (cut > 0 && (s[cut] != ' ' || glued(s, cut))) cut--; if (!cut) cut = cols; }
        char line[64];
        snprintf(line, sizeof line, "%.*s", cut, s);
        if (f) gfx_text8(f, x, y + lines * lh, line, (uint8_t)col, 1);
        lines++;
        s += cut;
        while (*s == ' ') s++;
    }
    return lines;
}
static int text_wrap(Frame *f, int x, int y, const char *s, int cols, int col, int maxlines) { return text_wrap_ex(f, x, y, s, cols, col, maxlines, 9); }

/* what the caster is looking at, and what it costs */
static void draw_glyph(Frame *f, int x, int y, int k, int sc, uint8_t c) {
    for (int gy = 0; gy < 7; gy++)
        for (int gx = 0; gx < 5; gx++) if (RUNE_GLYPH[k][gy][gx] == '#') gfx_rect(f, x + gx * sc, y + gy * sc, sc, sc, c);
}
static void draw_coin(Frame *f, int x, int y) {                    /* a Cursed Coin, 7 across */
    static const char *const C[7] = { "..###..", ".#####.", "##.#.##", "#######", "##...##", ".#####.", "..###.." };
    for (int r = 0; r < 7; r++)
        for (int c = 0; c < 7; c++)
            if (C[r][c] == '#') gfx_px(f, x + c, y + r, (uint8_t)(r < 3 ? R_GOLD + 9 : R_GOLD + 5));
            else if (r == 2 && c == 4) gfx_px(f, x + c, y + r, R_BLOOD + 14);   /* its red eye */
            else if (r == 2 || r == 4) gfx_px(f, x + c, y + r, 0);
}
static void render_shop_card(Frame *f) {
    int i = G.shop_step == SH_LOOK || G.shop_step == SH_GO || (G.shop_step == SH_BUY && G.shop_pick >= 0 && !G.offer_gone[G.shop_pick]) ? G.shop_look : -1;
    if (G.shop_step == SH_LOOK && G.shop_t < 0.35) i = -1;
    if (G.player) i = G.shop_look;
    int y0 = H - 44 - 46;
    if (G.player && G.sh_look && i < 0) {                          /* the shrine, and what E does there */
        int bw = 264, bx = (W - bw) / 2, ph = 42;
        y0 = H - 44 - 4 - ph;
        gfx_rect(f, bx, y0, bw, ph, 0);
        for (int x = 0; x < bw; x++) { gfx_px(f, bx + x, y0, R_STONE + 10); gfx_px(f, bx + x, y0 + ph - 1, R_STONE + 6); }
        for (int y = 0; y < ph; y++) { gfx_px(f, bx, y0 + y, R_STONE + 10); gfx_px(f, bx + bw - 1, y0 + y, R_STONE + 6); }
        char l1[48], l2[48];
        if (!RUN.shrine) { snprintf(l1, sizeof l1, "SANCTIFIED. IT GIVES NOTHING."); snprintf(l2, sizeof l2, RUN.coins ? "E: CURSE IT WITH A CURSED COIN" : "IT WANTS A CURSED COIN"); }
        else {
            snprintf(l1, sizeof l1, "CURSED: RUNES UP TO %s", QUAL_NAME[RUN.shrine - 1]);
            if (G.sh_takes > 0) snprintf(l2, sizeof l2, "E OPENS IT. %d TAKE%s LEFT", G.sh_takes, G.sh_takes == 1 ? "" : "S");
            else snprintf(l2, sizeof l2, "E OPENS IT");
        }
        gfx_text16(f, bx + 8, y0 + 4, "THE RUNE SHRINE", R_GOLD + 12, 1);
        char c[16]; snprintf(c, sizeof c, "%d", RUN.coins);
        gfx_text8(f, bx + bw - 8 - gfx_text_width(c, 1), y0 + 8, c, R_GOLD + 11, 1);
        draw_coin(f, bx + bw - 19 - gfx_text_width(c, 1), y0 + 8);
        gfx_text8(f, bx + 8, y0 + 21, l1, RUN.shrine ? ramp_at(QUAL_RAMP[RUN.shrine - 1], 12) : (uint8_t)(R_STONE + 12), 1);
        gfx_text8(f, bx + 8, y0 + 31, l2, R_STONE + 10, 1);
        return;
    }
    if (i < 0 || G.offer_gone[i]) {
        if (!G.player && G.shop_step == SH_THANK && !G.shop_bought) {
            const char *msg = G.shop_saving ? "SAVING UP FOR SOMETHING BETTER" : "NOTHING TAKES ITS FANCY";
            int w = gfx_text_width(msg, 1);
            gfx_rect(f, (W - w) / 2 - 6, y0 + 22, w + 12, 16, 0);
            gfx_text8(f, (W - w) / 2, y0 + 26, msg, R_GOLD + 10, 1);
        }
        return;
    }
    const Offer *o = &G.offers[i];
    char price[16];
    if (o->price == 0) snprintf(price, sizeof price, "FREE");
    else snprintf(price, sizeof price, "$%d", o->price);
    int bw = 264, bx = (W - bw) / 2, ph = 42, pw = gfx_text_width(price, 1);
    char foot[2][48] = { "", "" };                                 /* for a person: what E and Q do here, or a warning */
    int warn = 0;
    if (G.player) {
        if (G.shop_confirm == i) {
            int sl = piece_slot(&RUN.gear, o->piece.kind);
            snprintf(foot[0], sizeof foot[0], "YOUR OLD %s WILL BE LOST.", KIND_NAME[RUN.gear.worn[sl].kind]);
            snprintf(foot[1], sizeof foot[1], "NO GOLD COMES BACK. E AGAIN BUYS.");
            warn = 1;
        } else {
            if (G.shop_left <= 0) snprintf(foot[0], sizeof foot[0], "NO MORE WARES THIS VISIT");
            else if (o->price > 0 && G.shop_held < 0) snprintf(foot[0], sizeof foot[0], "E BUYS  Q HOLDS FOR $%d", (o->price + 3) / 4);
            else snprintf(foot[0], sizeof foot[0], "E BUYS");
            if (G.shop_left > 0) snprintf(foot[1], sizeof foot[1], G.shop_left == 1 ? "ONE MORE WARE THIS VISIT" : "%d MORE WARES THIS VISIT", G.shop_left);
            if (G.offer_back[i]) snprintf(foot[1], sizeof foot[1], "HELD FOR YOU LAST TIME");
            if (G.shop_held == i) snprintf(foot[1], sizeof foot[1], "HELD FOR YOUR NEXT VISIT");
        }
    }
    int fl = (foot[0][0] != 0) + (foot[1][0] != 0), body = 42;     /* the footer goes under the body */
    char name[100], l1[48], l2[48], stats[160] = "";
    const Piece *p = &o->piece;
    if (o->kind == GK_PIECE) {                                     /* a piece: its stats under its name */
        piece_name(p, l1, l2, sizeof l1);
        for (int k = 0; k < p->nstat; k++) {
            char st[40];
            piece_stat_text(p->stat[k], p->val[k], st, sizeof st);
            snprintf(stats + strlen(stats), sizeof stats - strlen(stats), "%s%s", k ? ", " : "", st);
        }
        if (p->kind == IK_BAG) snprintf(stats, sizeof stats, "HOLDS %d. %s", bag_capacity(p->quality), gear_blurb(o));
        if (p->ability) {
            char ab[64];
            piece_ability_text(p, ab, sizeof ab);
            snprintf(stats + strlen(stats), sizeof stats - strlen(stats), ". %s", ab);
        }
        body = 34 + 9 * text_wrap_ex(NULL, 0, 0, stats, (bw - 16) / 8, 0, 4, 9);
    }
    ph = body + (fl ? 9 * fl + 2 : 0);
    y0 = H - 44 - 4 - ph;
    gfx_rect(f, bx, y0, bw, ph, 0);
    for (int x = 0; x < bw; x++) { gfx_px(f, bx + x, y0, R_STONE + 10); gfx_px(f, bx + x, y0 + ph - 1, R_STONE + 6); }
    for (int y = 0; y < ph; y++) { gfx_px(f, bx, y0 + y, R_STONE + 10); gfx_px(f, bx + bw - 1, y0 + y, R_STONE + 6); }
    for (int k = 0, fy = y0 + body; k < 2; k++)                    /* the footer, at the bottom of the card */
        if (foot[k][0]) { gfx_text8(f, bx + 8, fy, foot[k], warn ? (uint8_t)(R_BLOOD + 13) : (k ? (uint8_t)(R_GOLD + 10) : (uint8_t)(R_STONE + 10)), 1); fy += 9; }
    if (o->kind == GK_PIECE) {
        int col = qual_text(p->quality);
        gfx_text8(f, bx + 8, y0 + 5, KIND_NAME[p->kind], R_STONE + 11, 1);
        gfx_text8(f, bx + 8, y0 + 14, l1, col, 1);
        gfx_text8(f, bx + 8, y0 + 23, l2, col, 1);
        text_wrap_ex(f, bx + 8, y0 + 33, stats, (bw - 16) / 8, R_STONE + 13, 4, 9);
        uint8_t pc = o->price <= RUN.treasure ? R_GOLD + 12 : R_BLOOD + 12;
        if (gfx_text_width(l1, 1) + 12 + pw <= bw - 16) gfx_text16(f, bx + bw - 8 - pw, y0 + 4, price, pc, 1);
        else gfx_text8(f, bx + bw - 8 - pw, y0 + 5, price, pc, 1);   /* a long name: the price keeps to the top line */
        return;
    }
    gear_name(o, name, sizeof name);
    gfx_text8(f, bx + 8, y0 + 5, o->kind == GK_ATK ? "ATTACK SPELL" : "DEFENSIVE RUNE", R_STONE + 11, 1);
    if (o->rare) gfx_text8(f, bx + bw - 8 - gfx_text_width("RARE", 1), y0 + 5, "RARE", ((int)(G.t * 3.0) & 1) ? C_WHITE : R_GOLD + 12, 1);
    gfx_text16(f, bx + 8, y0 + 14, name, ramp_at(offer_color(o), 13), 1);
    gfx_text8(f, bx + 8, y0 + 31, gear_blurb(o), R_STONE + 13, 1);
    gfx_text16(f, bx + bw - 8 - pw, y0 + 14, price, o->price <= RUN.treasure ? R_GOLD + 12 : R_BLOOD + 12, 1);
}

/* in the crypt: the face pops up from the bottom-left corner when hurt or pleased, then goes */
static void render_face_popup(Frame *f) {
    float t = fmaxf(G.face_t, G.grin_t > 0.0f ? fminf(G.grin_t, FACE_T) : 0.0f);
    if (G.dead) t = (float)fminf(G.dead_t, 4.0);
    if (t <= 0.0f) return;
    float k = G.dead ? smoothstepf(0.0f, 0.3f, t) : fminf(smoothstepf(FACE_T, FACE_T - 0.2f, t), smoothstepf(0.0f, 0.35f, t));
    int x = -44 + (int)(k * 50.0f), y = H - 4 - 18 - 4 - 46;
    if (G.readout <= 0.0f) y = H - 4 - 46;
    bevel(f, x, y, 40, 46, 0);
    if (G.ward_hp > 0.0f) for (int i = 1; i < 39; i++) { gfx_px(f, x + i, y + 1, R_BONE + 13); gfx_px(f, x + i, y + 44, R_BONE + 13); }
    draw_face(f, x + 4, y + 3, face_sub());
}

/* what the defensive spells look like while they last */
static void render_wards(Frame *f) {
    if (G.ward_hp > 0.0f) {                                        /* bone shards wheeling round the caster */
        int n = 2 + (int)(G.ward_hp / 12.0f);
        for (int i = 0; i < n && i < 5; i++) {
            float a = (float)G.t * 2.2f + i * TAU_F / n, sa = sinf(a);
            if (sa < -0.2f) continue;                              /* behind */
            int size = (int)(18 + 10 * sa), cx = 160 + (int)(cosf(a) * 150), cy = 128 + (int)(sa * 42);
            const uint8_t *img = crawl_spr[SP_BONEFLY0 + (((int)(G.t * 10.0) + i) & 3)];
            for (int x = 0; x < size; x++)
                for (int y = 0; y < size; y++) {
                    int sx = cx - size / 2 + x, sy = cy - size / 2 + y;
                    if (sx < 0 || sx >= W || sy < 0 || sy >= H) continue;
                    uint8_t c = img[(x * TS / size) * TS + y * TS / size];
                    if (c != C_CLEAR) f->pix[sy * W + sx] = shade(c, 0.9f + 0.3f * sa, sx, sy);
                }
        }
    }
    if (G.mirror_t > 0.0f || G.mirror_flash > 0.0f) {             /* a silvered ripple at the edges of sight */
        float k = fminf(1.0f, fmaxf(G.mirror_t, G.mirror_flash) * 2.0f);
        for (int y = 0; y < H; y++) {
            int w = (int)((5.0f + 3.0f * sinf(y * 0.13f + (float)G.t * 6.0f)) * k);
            for (int x = 0; x < w; x++) {
                if (BAYER[y & 3][x & 3] > (int)(12 * (1.0f - (float)x / (w + 1)))) continue;
                uint8_t c = (uint8_t)(R_GLOW + 8 + ((x + y) & 3));
                f->pix[y * W + x] = c; f->pix[y * W + W - 1 - x] = c;
            }
        }
    }
}

/* The hand as the caster is dressed: the sleeve in the colour of the robe's quality (the caster's own
 * purple without one), a ring on the ring finger with a stone of its quality, and a rune tattooed on the
 * back of the hand for each defensive spell it knows. Rebuilt when any of that changes. */
static uint8_t hand_buf[HAND_H * HAND_W], hand_rune[HAND_H * HAND_W];   /* hand_rune: 1 + spell for tattoo ink */
static Piece hand_worn[NSLOT]; static int8_t hand_def[NDEF]; static int hand_built;

static const int RING_AT[2] = { 55, 32 };                          /* on the ring finger (back of the hand, fingers up-left) */
static const int RUNE_AT[NDEF][2] = { { 40, 50 }, { 56, 49 }, { 72, 50 }, { 45, 61 }, { 61, 61 }, { 77, 62 }, { 51, 72 }, { 67, 73 }, { 83, 74 } };

static int is_skin(int x, int y) {
    if (x < 0 || y < 0 || x >= HAND_W || y >= HAND_H) return 0;
    uint8_t c = crawl_hand_img[y * HAND_W + x];
    return c >= R_SKIN && c < R_SKIN + 16;
}

static void build_hand(void) {
    const Gear *g = &RUN.gear;
    const Piece *robe = &g->worn[SL_ROBE], *ring = &g->worn[SL_RING];
    memcpy(hand_buf, crawl_hand_img, sizeof hand_buf);
    memset(hand_rune, 0, sizeof hand_rune);
    int ramp = robe->kind ? QUAL_RAMP[robe->quality] : R_PURPLE, top = ramp == R_GOLD ? 12 : 15;
    int shift = robe->kind && robe->quality == Q_COMMON ? -3 : 0;  /* plain cloth, not bone-white */
    for (int y = 0; y < HAND_H; y++)                               /* the sleeve */
        for (int x = 0; x < HAND_W; x++) {
            uint8_t *p = &hand_buf[y * HAND_W + x];
            if (*p < R_PURPLE || *p >= R_PURPLE + 16) continue;
            *p = (uint8_t)(ramp + clampi(*p - R_PURPLE + shift, 0, top));
        }
    if (robe->kind && robe->anointed)                              /* anointed: a second band of trim */
        for (int y = 1; y < HAND_H; y++)
            for (int x = 0; x < HAND_W - 6; x++)
                if (crawl_hand_img[y * HAND_W + x] >= R_GOLD && crawl_hand_img[y * HAND_W + x] < R_GOLD + 13) {
                    int tx = x + 4, ty = y + 5;
                    if (tx < HAND_W && ty < HAND_H && crawl_hand_img[ty * HAND_W + tx] >= R_PURPLE && crawl_hand_img[ty * HAND_W + tx] < R_PURPLE + 16)
                        hand_buf[ty * HAND_W + tx] = (uint8_t)(R_GOLD + 6 + (x & 1) * 3);
                }
    /* the right bracer: an iron cuff on the wrist just above the sleeve's trim, inlaid in its quality's colour */
    const Piece *br = &g->worn[SL_WRIST_R];
    if (br->kind) {
        int cr = QUAL_RAMP[br->quality];
        for (int x = 0; x < HAND_W; x++) {
            int top = -1;
            for (int y = 0; y < HAND_H && top < 0; y++)
                if (crawl_hand_img[y * HAND_W + x] >= R_GOLD && crawl_hand_img[y * HAND_W + x] < R_GOLD + 13) top = y;
            if (top < 0) continue;
            for (int dy = 1; dy <= 7; dy++) {
                int y = top - dy;
                if (!is_skin(x, y)) continue;
                static const int IRON[8] = { 0, 4, 7, -1, -1, -1, 10, 13 };   /* shadow by the trim, the inlay, a lit edge */
                uint8_t c = IRON[dy] >= 0 ? (uint8_t)(R_IRON + IRON[dy]) : ramp_at(cr, dy == 3 ? 9 : (dy == 4 ? 13 : 11));
                if (dy >= 6 && x % 10 == 5) c = (uint8_t)(R_GOLD + (dy == 7 ? 12 : 9));   /* rivets */
                hand_buf[y * HAND_W + x] = c;
            }
        }
    }
    /* the ring: a band across the finger, a stone set in it */
    if (ring->kind) {
        static const int METAL[NQUAL] = { R_IRON, R_BONE, R_BONE, R_GOLD, R_GOLD };
        int q = ring->quality, wide = q >= Q_ULTRA ? 2 : 1, metal = METAL[q];
        float ux = 0.29f, uy = 0.96f, vx = 0.96f, vy = -0.29f;      /* along the finger, and across it */
        float cx = (float)RING_AT[0], cy = (float)RING_AT[1];
        for (int a = -wide; a <= 1; a++)
            for (int side = -1; side <= 1; side += 2)
                for (int sd = side < 0 ? 0 : 1; sd <= 7; sd++) {
                    float s = (float)(side * sd);
                    int x = (int)lroundf(cx + vx * s + ux * a), y = (int)lroundf(cy + vy * s + uy * a);
                    if (!is_skin(x, y) || crawl_hand_img[y * HAND_W + x] <= R_SKIN + 1) break;   /* the edge of the finger */
                    float lit = 0.5f + 0.5f * cosf(s * 0.25f + 0.6f) - (a == 1 ? 0.25f : 0.0f) - (a == -wide ? 0.1f : 0.0f);
                    hand_buf[y * HAND_W + x] = ramp_at(metal, clampi((int)(4 + lit * 10), 2, 14));
                }
        if (q > Q_COMMON) {
            static const int GEM[NQUAL] = { 0, R_MOSS, R_GLOW, R_PURPLE, R_BLOOD };
            int gx = (int)lroundf(cx - ux * 0.5f), gy = (int)lroundf(cy - uy * 0.5f), r = q >= Q_ULTRA ? 2 : 1;
            for (int y = -r; y <= r; y++)
                for (int x = -r; x <= r; x++)
                    if (abs(x) + abs(y) <= r + (r > 1)) hand_buf[(gy + y) * HAND_W + gx + x] = (uint8_t)(GEM[q] + 9 + (x < 0) * 3 + (y < 0) * 2);
            hand_buf[(gy - r) * HAND_W + gx - (r > 1)] = C_WHITE;
        }
    }
    /* rune tattoos */
    for (int k = 0; k < NDEF; k++) {
        if (g->def[k] <= 0) continue;
        for (int y = 0; y < 7; y++)
            for (int x = 0; x < 5; x++) {
                if (RUNE_GLYPH[k][y][x] != '#') continue;
                int hx = RUNE_AT[k][0] - 2 + x + (6 - y) / 3, hy = RUNE_AT[k][1] - 3 + y;   /* slanted with the hand */
                if (!is_skin(hx, hy)) continue;
                hand_buf[hy * HAND_W + hx] = (uint8_t)(R_IRON + 4);
                hand_rune[hy * HAND_W + hx] = (uint8_t)(1 + k);
            }
    }
    memcpy(hand_worn, g->worn, sizeof hand_worn); memcpy(hand_def, g->def, sizeof hand_def); hand_built = 1;
}

static void render_hand(Frame *f) {
    float hand = hand_raise();
#ifdef CRAWL_DEBUG
    if (getenv("CRAWL_SHOWHAND")) { hand = 1.0f; G.rune_glow[atoi(getenv("CRAWL_SHOWHAND")) % NDEF] = 1.0f; }
#endif
    if (hand <= 0.0f) return;
    if (!hand_built || memcmp(hand_worn, RUN.gear.worn, sizeof hand_worn) || memcmp(hand_def, RUN.gear.def, sizeof hand_def)) build_hand();
    float fox, foy;
    hand_origin(hand, &fox, &foy);
    int ox = (int)fox, oy = (int)foy;
    float light = fminf(lm_at(G.px, G.py) + 0.6f * hand, 1.25f);
    static uint8_t grey[256]; static int grey_built;
    if (!grey_built) {                                             /* each colour as a grey of its brightness */
        for (int c = 0; c < 256; c++) grey[c] = (uint8_t)(R_STONE + clampi((crawl_pal[c][0] * 3 + crawl_pal[c][1] * 6 + crawl_pal[c][2]) * 15 / 630, 0, 15));
        grey_built = 1;
    }
    int greyed = G.silenced;
    oy += (int)(G.sil_k * 18.0f);                                  /* the void: the hand drops */
    for (int y = 0; y < HAND_H; y++) {
        int sy = oy + y;
        if (sy < 0 || sy >= H) continue;
        for (int x = 0; x < HAND_W; x++) {
            uint8_t c = hand_buf[y * HAND_W + x];
            if (c == C_CLEAR) continue;
            int sx = ox + x;
            if (sx < 0 || sx >= W) continue;
            int rk = hand_rune[y * HAND_W + x];
            if (rk && G.rune_glow[rk - 1] > 0.0f) {                    /* the rune burning as its spell goes off */
                static const int GLOW[NDEF] = { R_GLOW, R_GLOW, R_GLOW, R_GOLD, R_MOSS, R_GLOW, R_FLAME, R_GLOW, R_GLOW };
                float k = fminf(1.0f, G.rune_glow[rk - 1] / 0.8f);
                f->pix[sy * W + sx] = ramp_at(GLOW[rk - 1], (int)(6 + 9 * k));
                continue;
            }
            float lit = light;
            f->pix[sy * W + sx] = shade(greyed ? grey[c] : c, lit, sx, sy);
        }
    }
    float ct = -1.0f, fire = 0.0f;                                 /* how far into a cast, and when it lets go */
    if (G.state == PS_CAST) { ct = G.st; fire = 0.27f * G.cast_len; }
    else if (G.player && G.pc_t >= 0.0f) { ct = G.pc_t; fire = 0.35f * G.pc_len; }
    if (ct >= 0.0f && ct < fire + 0.03f) {                         /* the spell forming over the fingertips */
        static const int RAMP[NATK] = { R_FLAME, R_GLOW, R_GLOW, R_BONE, R_BLOOD, R_FLAME, R_GLOW, R_FLAME, R_PURPLE };
        static const float SIZE[NATK] = { 1.0f, 0.8f, 0.8f, 0.7f, 0.9f, 1.35f, 0.75f, 0.55f, 1.2f };
        int sp = clampi(G.spell, 0, NATK - 1), ramp = RAMP[sp];
        float r = (3.0f + 7.0f * clampf(ct / (fire * 0.8f), 0.0f, 1.0f)) * SIZE[sp];
        int cx = ox + 32, cy = oy + 1;                             /* between the first two fingertips */
        uint32_t jr = (uint32_t)(G.t * 40.0) * 2246822519u;
        for (int y = (int)(cy - r - 2); y <= (int)(cy + r + 2); y++)
            for (int x = (int)(cx - r - 2); x <= (int)(cx + r + 2); x++) {
                if (x < 0 || x >= W || y < 0 || y >= H) continue;
                float d = hypotf(x + 0.5f - cx, y + 0.5f - cy) / r;
                if (sp == AT_BFS) {                                /* a hole in the air with a violet rim */
                    if (d < 0.8f) f->pix[y * W + x] = 0;
                    else if (d < 1.15f) f->pix[y * W + x] = (uint8_t)(R_PURPLE + 15 - (int)((d - 0.8f) * 20));
                    continue;
                }
                if (sp == AT_CHAIN && d > 0.6f && d < 1.3f && (rng_next(&jr) & 7) == 0) { f->pix[y * W + x] = C_WHITE; continue; }   /* crackle */
                if (d > 1.0f) continue;
                f->pix[y * W + x] = (uint8_t)(ramp + clampi((int)((1.05f - d * 0.8f) * 15), 2, 15));
            }
    }
}

/* a soft ball of wisp-light: pass 0 lays down its dithered halo, pass 1 the ball, white at the heart */
static int wisp_ramp = R_GLOW;                                     /* the colour of the wisp being drawn */
/* a glowing ball on the screen: pass 0 its dithered halo, pass 1 its body; depth hides it behind the walls */
static void wisp_ball(Frame *f, float cx, float cy, float r, float a, int pass, float depth) {
    float reach = pass ? 1.0f : 2.0f;
    int x0 = (int)floorf(cx - r * reach), x1 = (int)ceilf(cx + r * reach), y0 = (int)floorf(cy - r * reach), y1 = (int)ceilf(cy + r * reach);
    for (int x = x0 < 0 ? 0 : x0; x <= x1 && x < W; x++) {
        if (depth > zbuf[x]) continue;
        for (int y = y0 < 0 ? 0 : y0; y <= y1 && y < H; y++) {
            float d = hypotf(x + 0.5f - cx, y + 0.5f - cy) / r;
            if (pass) {
                if (d >= 1.0f) continue;
                f->pix[y * W + x] = d < 0.42f && r > 1.6f ? C_WHITE : (uint8_t)(wisp_ramp + 15 - (int)(d * 6.0f));
            } else if (d >= 1.0f && d < 2.0f) {
                float k = (2.0f - d) * a;
                if (k * 16.0f > BAYER[y & 3][x & 3] + 0.5f) f->pix[y * W + x] = (uint8_t)(wisp_ramp + 6 + (int)(k * 4.0f));
            }
        }
    }
}

/* the wisps in the crypt: the motes they left hanging, then each body, its flame licking back the way
 * it came (or up, while it hovers) */
static void render_wisps(Frame *f, float dx, float dy, float horizon) {
    float t = (float)G.t;
    for (int i = 0; i < MAXWISPS; i++) {
        const Wisp *w = &G.wisps[i];
        if (!w->on) continue;
        wisp_ramp = wisp_ramp_of(w);
        for (int k = 0; k < WISP_MOTES; k++) {
            const Mote *m = &w->motes[k];
            float age = t - m->t, sx, sy, dp;
            if (m->t <= 0.0f || age < 0.0f || age > WISP_MOTE_T) continue;
            if (!project(m->x, m->y, m->z + 0.03f * sinf(t * 1.3f + k), dx, dy, horizon, &sx, &sy, &dp) || dp < 0.6f) continue;   /* not those overhead */
            int x = (int)sx, y = (int)sy;
            if (x < 0 || x >= W || y < 0 || y >= H || dp > zbuf[x]) continue;
            float k01 = 1.0f - age / WISP_MOTE_T, tw = 0.5f + 0.5f * sinf(t * 5.0f + k * 1.7f);
            if (k01 < 0.4f && BAYER[y & 3][x & 3] >= (int)(k01 / 0.4f * 16.0f)) continue;   /* thinning out as it goes */
            float r = clampf(0.03f * FOCAL / dp, 0.6f, 3.5f) * (0.55f + 0.45f * k01) * (0.85f + 0.3f * tw);
            if (r < 1.1f) { f->pix[y * W + x] = (uint8_t)(wisp_ramp + 8 + (int)(k01 * 5.0f + tw * 2.0f)); continue; }
            if (k01 > 0.75f) wisp_ball(f, sx, sy, r, 0.5f * k01, 0, dp);   /* fresh ones with a glow about them */
            wisp_ball(f, sx, sy, r, 1.0f, 1, dp);
        }
        float sx, sy, dp, e = clampf(w->a, 0.0f, 1.0f);
        if (w->a <= 0.0f || !project(w->x, w->y, w->z, dx, dy, horizon, &sx, &sy, &dp) || dp < 0.35f) continue;
        float flick = 1.0f + 0.09f * sinf(t * 17.0f + i) + 0.06f * sinf(t * 29.0f + 1.3f);
        float s = clampf(FOCAL / dp * 0.011f, 0.22f, 2.6f) * (0.35f + 0.65f * e) * (w->state == WS_GONE ? fmaxf(w->a, 0.05f) : 1.0f);
        float tdx = 0.0f, tdy = -1.0f, bx, by, bd;                 /* which way its flame streams */
        if (hypotf(w->vx, w->vy) > 0.3f && project(w->x - w->vx * 0.2f, w->y - w->vy * 0.2f, w->z, dx, dy, horizon, &bx, &by, &bd)) {
            float l = hypotf(bx - sx, by - sy);
            if (l > 0.5f) { tdx = (bx - sx) / l; tdy = (by - sy) / l; }
        }
        if (w->kind == WK_EXIT && G.blessed) {                     /* blessed: a pale gold halo about it */
            wisp_ramp = R_GOLD + 4;                                /* the halo's colours, from the pale end of the gold */
            wisp_ball(f, sx, sy, 7.5f * s * flick, 0.5f * e, 0, dp);
            wisp_ramp = wisp_ramp_of(w);
        }
        for (int pass = 0; pass < 2; pass++) {
            for (int k = 4; k >= 1; k--) {                         /* tongues of it, licking back */
                float wv = sinf(t * 11.0f + k * 2.1f + i) * 0.9f * k;
                wisp_ball(f, sx + (tdx * 3.4f * k - tdy * wv) * s, sy + (tdy * 3.4f * k + tdx * wv) * s, (5.6f - 1.0f * k) * s * flick, 0.75f * e, pass, dp);
            }
            for (int k = 1; k <= 2; k++) {                         /* and flickering up off its crown */
                float ph = t * (2.3f + 0.7f * k) + k * 1.7f + i, u = ph - floorf(ph);
                wisp_ball(f, sx + sinf(ph * 5.0f) * 1.5f * s, sy - (4.0f + 7.0f * u) * s, (2.4f - 1.6f * u) * s, 0.6f * e * (1.0f - u), pass, dp);
            }
            wisp_ball(f, sx, sy, 6.2f * s * flick, e, pass, dp);
        }
    }
}

static void card_box(Frame *f, int bw, int bh, int by) {
    int bx = (W - bw) / 2;
    gfx_rect(f, bx, by, bw, bh, 0);
    for (int i = 0; i < bw; i++) { gfx_px(f, bx + i, by, R_STONE + 11); gfx_px(f, bx + i, by + bh - 1, R_STONE + 11);
                                   gfx_px(f, bx + i, by + 2, R_STONE + 6); gfx_px(f, bx + i, by + bh - 3, R_STONE + 6); }
    for (int i = 0; i < bh; i++) { gfx_px(f, bx, by + i, R_STONE + 11); gfx_px(f, bx + bw - 1, by + i, R_STONE + 11);
                                   if (i >= 2 && i < bh - 2) { gfx_px(f, bx + 2, by + i, R_STONE + 6); gfx_px(f, bx + bw - 3, by + i, R_STONE + 6); } }
}

static void card_text(Frame *f, int y, const char *s, int c) { gfx_text16(f, (W - gfx_text_width(s, 1)) / 2, y, s, c, 1); }

static void render_card(Frame *f) {
    char line[48];
    if (G.dead && G.dead_t > 1.1 && G.dead_t < 4.6) {
        card_box(f, 232, 64, 68);
        card_text(f, 80, "YOU HAVE PERISHED", R_BLOOD + 13);
        snprintf(line, sizeof line, "LEVEL %d BEGINS AGAIN", G.level);
        card_text(f, 104, line, C_WHITE);
        return;
    }
    if (G.boss >= 0 && !PK.depth && G.bcard > 0.3f && G.bcard < 4.6f) {   /* a boss's name, and a line about it */
        const BossDef *B = &BOSSES[G.boss];
        int bw = gfx_text_width(B->name, 1) + 32;
        card_box(f, bw > 250 ? bw : 250, 74, 62);
        card_text(f, 72, B->name, R_BLOOD + 13);
        text_wrap(f, (W - 236) / 2, 98, B->lore, 29, R_STONE + 13, 3);
        return;
    }
    if (G.card < 0.3f || G.card > 3.6f) return;
    char name[48];
    snprintf(line, sizeof line, "LEVEL %d", G.level);
    level_name(G.level, name, sizeof name);
    int bw = gfx_text_width(name, 1) + 32;
    card_box(f, bw > 210 ? bw : 210, 64, 68);
    card_text(f, 80, line, C_WHITE);
    card_text(f, 104, name, R_FLAME + 11);
}

/* a boss's bar along the top of the view: its name, its health with a notch at each wave's mark, grey
 * while it is untouchable, and how many of its adds are still standing */
/* held in the web: strands in from the edges, fewer as it tears */
static void render_web(Frame *f) {
    float k = G.web_max > 0.0f ? G.web_hp / G.web_max : 0.0f;
    for (int i = 0; i < 28; i++) {
        if (hash2(i, 77) % 100 >= (uint32_t)(25 + 75 * k)) continue;   /* torn away */
        float t = (float)(hash2(i, 3) % 1000) / 1000.0f;
        int side = i & 3, ex, ey;
        if (side == 0) { ex = (int)(t * W); ey = 0; } else if (side == 1) { ex = (int)(t * W); ey = H - 1; }
        else if (side == 2) { ex = 0; ey = (int)(t * H); } else { ex = W - 1; ey = (int)(t * H); }
        float reach = (0.18f + 0.22f * (float)(hash2(i, 9) % 100) / 100.0f) * (0.5f + 0.5f * k);
        int ix = ex + (int)((W / 2 - ex) * reach), iy = ey + (int)((H / 2 - ey) * reach);
        gfx_line(f, ex, ey, ix, iy, (uint8_t)(R_GHOST + 10 + (i % 4)));
        if (i > 0 && (hash2(i, 5) & 1)) {                          /* a cross thread to the last strand's middle */
            int j = i - 1, s2 = j & 3; float t2 = (float)(hash2(j, 3) % 1000) / 1000.0f;
            int ex2 = s2 == 2 ? 0 : (s2 == 3 ? W - 1 : (int)(t2 * W)), ey2 = s2 == 0 ? 0 : (s2 == 1 ? H - 1 : (int)(t2 * H));
            if (s2 == side) gfx_line(f, (ex + ix) / 2, (ey + iy) / 2, (ex2 + W / 2) / 2 + (ex2 - W / 2) / 3, (ey2 + H / 2) / 2 + (ey2 - H / 2) / 3, (uint8_t)(R_GHOST + 9));
        }
    }
}

static void render_bossbar(Frame *f) {
    if (G.boss < 0 || PK.depth || G.shop || G.bstate < BS_FIGHT || (G.bstate == BS_DEAD && G.bstate_t > 3.0f)) return;
    const BossDef *B = &BOSSES[G.boss];
    int off = G.bstate != BS_FIGHT, bw = 180, bx = (W - bw) / 2, by = 13;
    int nw = gfx_text_width(B->name, 1);
    gfx_rect(f, (W - nw) / 2 - 3, 2, nw + 6, 10, 0);
    gfx_text8(f, (W - nw) / 2, 3, B->name, off ? (uint8_t)(R_STONE + 12) : (uint8_t)(R_BLOOD + 13), 1);
    gfx_rect(f, bx - 2, by - 2, bw + 4, 9, 0);
    for (int i = -1; i <= bw; i++) { gfx_px(f, bx + i, by - 1, R_STONE + 9); gfx_px(f, bx + i, by + 5, R_STONE + 5); }
    for (int j = -1; j <= 5; j++) { gfx_px(f, bx - 1, by + j, R_STONE + 9); gfx_px(f, bx + bw, by + j, R_STONE + 5); }
    int fill = (int)(clampf(G.bbar, 0.0f, 1.0f) * bw + 0.5f), ring = G.bring > 0.0f && ((int)(G.t * 20.0) & 1);
    for (int y = 0; y < 5; y++)
        for (int x = 0; x < bw; x++) {
            uint8_t c = x < fill ? (uint8_t)((off ? R_STONE + 7 : R_BLOOD + 10) + (y == 0 ? 4 : 0)) : (uint8_t)(R_BLOOD + 2);
            if (ring && x < fill) c = (uint8_t)(R_STONE + 13);
            gfx_px(f, bx + x, by + y, c);
        }
    for (int w = 0; w < NWAVES; w++) {                             /* the marks: bright until passed */
        int x = bx + (int)(WAVE_MARK[w] * bw + 0.5f);
        for (int y = -1; y <= 5; y++) gfx_px(f, x, by + y, w < G.bwave ? (uint8_t)(R_STONE + 6) : C_WHITE);
    }
    char line[32];
    if (G.bstate == BS_WAVE) {
        if (G.badds < 0) snprintf(line, sizeof line, "IT CALLS"); else snprintf(line, sizeof line, "%d REMAIN", G.badds);
        int lw = gfx_text_width(line, 1);
        gfx_rect(f, (W - lw) / 2 - 3, by + 7, lw + 6, 10, 0);
        gfx_text8(f, (W - lw) / 2, by + 8, line, R_GOLD + 11, 1);
    }
}

/* Wolfenstein-style intermission: the numbers run up one row at a time */
static void render_tally(Frame *f) {
    memset(f->pix, 0, (size_t)W * H);
    float t = (float)G.tally_t;
    card_box(f, 304, 164, 18);
    char line[48];
    snprintf(line, sizeof line, "LEVEL %d COMPLETE!", G.level);
    int flash = t < 0.6f && ((int)(t * 12.0f) & 1);
    gfx_text16(f, (W - gfx_text_width(line, 2)) / 2 + 1, 29, line, R_GOLD + 2, 2);          /* drop shadow */
    gfx_text16(f, (W - gfx_text_width(line, 2)) / 2, 28, line, flash ? C_WHITE : R_GOLD + 12, 2);
    char name[48];
    level_name(G.level, name, sizeof name);
    card_text(f, 64, name, R_FLAME + 9);
    static const char *LABEL[TALLY_ROWS] = { "TIME", "KILLS", "TREASURE", "SOULS", "SECRETS", "BOSS" };
    int totals[TALLY_ROWS] = { 0, G.kills_total, G.items_total, 0, G.nlinks, 0 }, nr = tally_rows();
    for (int r = 0; r < nr; r++) {
        if (t < TALLY_ROW0 + r * TALLY_ROW_GAP - 0.3f) continue;
        int y = nr > 5 ? 80 + r * 14 : 82 + r * 16, v = G.tally_shown[r];
        gfx_text16(f, 52, y, LABEL[r], C_WHITE, 1);
        if (r == 0 || r == 5) snprintf(line, sizeof line, "%d:%02d", v / 60, v % 60);
        else if (r == 3) snprintf(line, sizeof line, "+%d", v);
        else snprintf(line, sizeof line, "%d / %d", v, totals[r]);
        int done = t >= TALLY_ROW0 + r * TALLY_ROW_GAP + TALLY_COUNT;
        gfx_text16(f, 268 - gfx_text_width(line, 1), y, line, done ? R_GOLD + 12 : R_GOLD + 8, 1);
        for (int x = 52 + gfx_text_width(LABEL[r], 1) + 6; x < 262 - gfx_text_width(line, 1); x += 4) gfx_px(f, x, y + 12, R_STONE + 7);   /* leader dots */
    }
    if (t > TALLY_ROW0 + nr * TALLY_ROW_GAP + 0.2f) {
        int perfect = G.items_total > 0 && G.items_got == G.items_total;
        int all_dead = G.kills_total > 0 && G.kills == G.kills_total;
        const char *msg = perfect && all_dead ? "PERFECT!" : (perfect ? "ALL TREASURE!" : (all_dead ? "NONE LEFT STANDING" : ""));
        if (*msg && ((int)(t * 3.0f) & 1)) card_text(f, 168, msg, perfect ? R_GOLD + 12 : R_BLOOD + 13);
    }
}

/* HP, gold, souls and keys: slides up from the bottom edge when something changes, then away */
static const char *const KEY_ART[5] = { ".##.....", "#..#####", "#..#..#.", ".##...#.", "........" };
static void key_icon(Frame *f, int kx, int ky, int color) {
    static const int RAMP[NKEYS] = { R_GOLD, R_IRON, R_BLOOD };
    for (int y = 0; y < 5; y++)
        for (int x = 0; x < 8; x++)
            if (KEY_ART[y][x] == '#') gfx_px(f, kx + x, ky + y, (uint8_t)(RAMP[color] + (color == 0 ? (y < 2 ? 12 : 9) : (y < 2 ? 14 : 11))));
}
static void render_readout(Frame *f) {
    if (G.readout <= 0.0f || G.dead) return;
    float shown = READOUT_T - G.readout, k = fminf(smoothstepf(0.0f, 0.25f, shown), 1.0f - smoothstepf(0.0f, 0.35f, 0.35f - G.readout));
    if (G.readout > READOUT_T) k = smoothstepf(READOUT_T + 1.5f, READOUT_T + 1.25f, G.readout);
    if (k <= 0.0f) return;
    char num[16], gold[16], souls[16];
    snprintf(gold, sizeof gold, "%d", RUN.treasure);
    spirit_text(souls, sizeof souls);
    int nk = 0;
    for (int c = 0; c < NKEYS; c++) nk += G.has_key[c] > 0;
    int show_souls = RUN.souls > 0 || RUN.gear.atk[AT_BFS] >= 0;
    int gx = 84, sx = gx + 10 + gfx_text_width(gold, 1) + 8;
    int kx = show_souls ? sx + 12 + gfx_text_width(souls, 1) + 8 : sx;
    char coins[12];
    snprintf(coins, sizeof coins, "%d", RUN.coins);
    int cx = kx + nk * 11 + (nk ? 2 : 0);
    int pw = cx + (RUN.coins > 0 ? 10 + gfx_text_width(coins, 1) + 6 : 0) + 4, ph = 18, px = 6, py = H - 4 - (int)(k * (ph + 4)) + 4 - (int)(G.lift * 40.0f + 0.5f);   /* raised over HOLD H */
    gfx_rect(f, px, py, pw, ph, 0);
    for (int i = 0; i < pw; i++) { gfx_px(f, px + i, py, R_STONE + 9); gfx_px(f, px + i, py + ph - 1, R_STONE + 5); }
    for (int i = 0; i < ph; i++) { gfx_px(f, px, py + i, R_STONE + 9); gfx_px(f, px + pw - 1, py + i, R_STONE + 5); }
    int low = G.hp <= G.max_hp / 4 && ((int)(G.t * 4.0) & 1);
    uint8_t hcol = G.poison_t > 0.0f ? (uint8_t)(R_MOSS + 13) : (low ? (uint8_t)(R_BLOOD + 15) : (uint8_t)(R_BLOOD + 12));
    gfx_glyph8(f, px + 5, py + 5, 3, hcol, -1, 1);
    snprintf(num, sizeof num, "%3d", G.hp);
    gfx_text8(f, px + 14, py + 5, num, C_WHITE, 1);
    int bx = px + 42, bw = 34, fill = (G.hp * bw + G.max_hp / 2) / G.max_hp;
    for (int y = 0; y < 4; y++)
        for (int x = 0; x < bw; x++) {
            uint8_t c = x < fill ? (uint8_t)((G.poison_t > 0.0f ? R_MOSS : R_BLOOD) + (y == 0 ? 14 : 11)) : (uint8_t)(R_BLOOD + 3);
            if (G.ward_hp > 0.0f && y == 0 && x < (int)(G.ward_hp * bw / 40.0f)) c = (uint8_t)(R_BONE + 14);   /* the ward, a pale line over it */
            gfx_px(f, bx + x, py + 7 + y, c);
        }
    gfx_glyph8(f, px + gx, py + 5, '$', R_GOLD + 12, -1, 1);
    gfx_text8(f, px + gx + 10, py + 5, gold, R_GOLD + 10, 1);
    if (show_souls) {                                              /* a soul: a little wisp */
        int wx = px + sx, wy = py + 4;
        static const char *SOUL[8] = { "..#..", ".###.", "#####", "#####", ".###.", "..##.", "...#.", "....#" };
        for (int y = 0; y < 8; y++) for (int x = 0; x < 5; x++) if (SOUL[y][x] == '#') gfx_px(f, wx + x, wy + y, (uint8_t)(R_GLOW + (y < 4 ? 13 : 9)));
        int ready = RUN.gear.atk[AT_BFS] >= 0 && RUN.souls >= bfs_cost(RUN.gear.atk[AT_BFS]);
        gfx_text8(f, wx + 8, py + 5, souls, ready && ((int)(G.t * 3.0) & 1) ? C_WHITE : (uint8_t)(R_GLOW + 11), 1);
    }
    for (int c = 0, i = 0; c < NKEYS; c++) if (G.has_key[c] > 0) key_icon(f, px + kx + (i++) * 11, py + 6, c);
    if (RUN.coins > 0) { draw_coin(f, px + cx, py + 5); gfx_text8(f, px + cx + 10, py + 5, coins, R_GOLD + 10, 1); }
}

/* screen tints: red when hurt or dying, gold for treasure; index 0 stays black for the OLED */
static void tint(Frame *f) {
    float red = 0.30f * G.hurt, gold = 0.22f * G.gold, dark = 0.0f, chill = 0.3f * G.chill;
    float green = G.poison_t > 0.0f ? 0.20f + 0.07f * sinf((float)G.t * 4.0f) : 0.0f;          /* poisoned */
    float blue = G.slow_t > 0.0f && G.poison_t <= 0.0f ? 0.10f * fminf(1.0f, G.slow_t) : 0.0f;  /* webbed and slowed */
    float violet = G.slip_t > 0.0f ? 0.10f * fminf(1.0f, G.slip_t) : 0.0f;                      /* Time Slip */
    if (G.dead) { float k = smoothstepf(0.0f, 1.4f, (float)G.dead_t); red = fmaxf(red, 0.45f * k); dark = 0.35f * k; }
    else if (G.hp <= G.max_hp / 4) red = fmaxf(red, 0.08f * (0.5f + 0.5f * sinf((float)G.t * 5.7f)));
    if (G.dread_t > 0.0f) { float k = sinf(PI_F * G.dread_t / 0.8f); dark = fmaxf(dark, 0.45f * k); violet = fmaxf(violet, 0.18f * k); }   /* Dread: a shadow falls */
    if (G.bfs_t > 0.0f) dark = fmaxf(dark, G.bfs_t < 0.12f ? 0.97f : 0.9f * (1.0f - smoothstepf(0.12f, 1.3f, G.bfs_t)));   /* the light goes out of the room */
    if (red <= 0.0f && gold <= 0.0f && chill <= 0.0f && green <= 0.0f && blue <= 0.0f && violet <= 0.0f && dark <= 0.0f) return;
    for (int i = 1; i < 256; i++) {
        int glow = G.bfs_t > 0.0f && i >= R_GLOW && i < R_GLOW + 16;   /* the imploding keep their light */
        for (int c = 0; c < 3; c++) {
            float v = f->pal[i][c] * (glow || i == C_WHITE ? 1.0f : 1.0f - dark);
            v += (50.0f * (c == 0) - v) * red * (c == 0 ? 0.6f : 1.0f);
            if (c < 2) v += ((c ? 50.0f : 63.0f) - v) * gold; else v -= v * gold;
            v += ((c == 0 ? 30.0f : 52.0f) - v) * chill;                 /* a wraith's cold breath */
            v += ((c == 1 ? 40.0f : 8.0f) - v) * green;
            v += ((c == 2 ? 50.0f : 20.0f) - v) * blue;
            v += ((c == 1 ? 10.0f : 44.0f) - v) * violet;
            f->pal[i][c] = (uint8_t)clampi((int)(v + 0.5f), 0, 63);
        }
    }
}

/* a line of news at the top of the view, and the first-time hints */
static void render_messages(Frame *f) {
    if (G.msg_t > 0.0f && G.msg[0] && !(G.player && G.ui)) {
        if (G.msg_t > 0.3f || ((int)(G.msg_t * 20.0f) & 1)) {
            int w = gfx_text_width(G.msg, 1), x = (W - w) / 2, y = toast_showing() ? 50 : (boss_fighting() ? 34 : 14);   /* below a pop-up, or a boss's bar */
            gfx_rect(f, x - 5, y, w + 10, 14, 0);
            gfx_text8(f, x, y + 3, G.msg, R_GOLD + 11, 1);
        }
    }
    if (G.hint_t > 0.0f && G.player && !G.ui) {
        static const char *const HINT[4] = { "WASD WALK  MOUSE TURN  CLICK CASTS", "1-9 SPELLS  E OPENS  TAB INVENTORY", "B BESTIARY  ESC LEAVE OR SLEEPWALK",
                                             "L WISPS  H THE STATUS BAR" };
        for (int i = 0; i < 4; i++) {
            int w = gfx_text_width(HINT[i], 1), x = (W - w) / 2, y = 34 + i * 11;
            gfx_rect(f, x - 4, y - 2, w + 8, 11, 0);
            gfx_text8(f, x, y, HINT[i], R_STONE + 13, 1);
        }
    }
}

static void dim_view(Frame *f) {                                   /* the crypt goes dim behind a menu */
    for (int y = 0; y < H; y++)
        for (int x = (y & 1); x < W; x += 2) f->pix[y * W + x] = 0;
}

static void panel(Frame *f, int x, int y, int w, int h) {
    gfx_rect(f, x, y, w, h, 0);
    for (int i = 0; i < w; i++) { gfx_px(f, x + i, y, R_STONE + 11); gfx_px(f, x + i, y + h - 1, R_STONE + 6); gfx_px(f, x + i, y + 2, R_STONE + 6); gfx_px(f, x + i, y + h - 3, R_STONE + 4); }
    for (int i = 0; i < h; i++) { gfx_px(f, x, y + i, R_STONE + 11); gfx_px(f, x + w - 1, y + i, R_STONE + 6); if (i >= 2 && i < h - 2) { gfx_px(f, x + 2, y + i, R_STONE + 6); gfx_px(f, x + w - 3, y + i, R_STONE + 4); } }
}

/* the caster's total of a stat, in its units: health as the whole of it */
static void total_text(const Gear *g, int st, char *buf, int n) {
    float t = gear_total(g, st);
    if (st == ST_HP) snprintf(buf, (size_t)n, "%d", gear_max_hp(g));
    else if (st == ST_ARMOR) snprintf(buf, (size_t)n, "%.0f", t);
    else if (st == ST_REGEN) snprintf(buf, (size_t)n, "%.2f/S", t);
    else if (st == ST_THRIFT && t >= 0.5f) snprintf(buf, (size_t)n, "-%.0f%%", t);
    else snprintf(buf, (size_t)n, "%.0f%%", t);
}

/* a piece's card: its name in its quality's colour, its stats (each beside the caster's total, green
 * or red against when the wardrobe opened), its ability; the bottom it reached */
static int piece_card(Frame *f, int x, int y, int w, const Piece *p, const Gear *g, const Gear *g0) {
    if (p->kind == IK_NONE) { gfx_text8(f, x, y, "NOTHING", R_STONE + 8, 1); return y + 8; }
    char l1[48], l2[48], line[100];
    piece_name(p, l1, l2, sizeof l1);
    int col = qual_text(p->quality);
    snprintf(line, sizeof line, "%s%s%s", l1, *l1 ? " " : "", l2);
    y += text_wrap_ex(f, x, y, line, w / 8, col, 4, 8) * 8 + 2;
    if (p->kind == IK_BAG) {
        snprintf(line, sizeof line, "HOLDS %d", bag_capacity(p->quality));
        gfx_text8(f, x, y, line, R_STONE + 12, 1);
        return y + 8;
    }
    for (int i = 0; i < p->nstat; i++) {
        piece_stat_text(p->stat[i], p->val[i], line, sizeof line);
        gfx_text8(f, x, y, line, R_STONE + 12, 1);
        if (g) {
            float v = gear_total(g, p->stat[i]), v0 = gear_total(g0, p->stat[i]);
            int c = v > v0 + 1e-3f ? R_MOSS + 13 : (v < v0 - 1e-3f ? R_BLOOD + 13 : R_STONE + 9);
            total_text(g, p->stat[i], line, sizeof line);
            gfx_text8(f, x + w - gfx_text_width(line, 1), y, line, c, 1);
        }
        y += 8;
    }
    if (p->ability) {
        piece_ability_text(p, line, sizeof line);
        y += 2 + text_wrap_ex(f, x, y + 2, line, w / 8, R_GOLD + 11, 3, 8) * 8;
    }
    return y;
}

/* the bag is full and a ware is bought for a slot already worn: which spare to let go */
static void render_letgo(Frame *f) {
    dim_view(f);
    panel(f, 4, 4, W - 8, H - 8);
    const Gear *g = &RUN.gear;
    gfx_text16(f, 14, 9, "THE BAG IS FULL", R_GOLD + 12, 1);
    gfx_text8(f, 14, 26, "W/S PICK  ENTER LET GO  ESC NOT NOW", R_STONE + 9, 1);
    int i = G.shop_letgo;
    if (i >= 0) {
        const Piece *np = &G.offers[i].piece;
        int sl = piece_slot(g, np->kind);
        char line[48];
        snprintf(line, sizeof line, "TO MAKE ROOM FOR YOUR OLD %s", KIND_NAME[g->worn[sl].kind]);
        gfx_text8(f, 14, 38, line, R_STONE + 12, 1);
    }
    int n = g->bag_size, sel = clampi(G.ui_sel, 0, n - 1);
    for (int r = 0; r < n; r++) {
        int y = 54 + r * 10;
        const Piece *p = &g->bag[r];
        if (r == sel) gfx_rect(f, 10, y - 1, 102, 10, R_IRON + 3);
        gfx_text8(f, 14, y, p->kind ? KIND_NAME[p->kind] : "-", p->kind ? qual_text(p->quality) : R_STONE + 7, 1);
        if (p->anointed) gfx_glyph8(f, 14 + gfx_text_width(KIND_NAME[p->kind], 1), y, '*', R_GOLD + 12, -1, 1);
    }
    piece_card(f, 120, 54, W - 10 - 120, &g->bag[sel], NULL, NULL);
}

/* Tab, the second page: the caster's numbers (what abilities add in gold beside them) and the spellbook */
static void render_caster(Frame *f) {
    static const char *const SHORT[NSTAT] = { "ARMOR", "DODGE", "HEALTH", "RESTORES", "POISON WARD", "BURN WARD", "SLOW WARD",
                                              "MOVEMENT", "QUIET", "SPELL HASTE", "SPIRIT COST", "DAMAGE", "FIRE", "FROST",
                                              "LIGHTNING", "EFFECT" };
    const Gear *g = &RUN.gear, *g0 = &G.inv0;
    gfx_text16(f, 14, 9, "THE CASTER", R_GOLD + 12, 1);
    gfx_text8(f, 14, 26, "A/D PAGE  TAB CLOSE", R_STONE + 9, 1);
    {                                                              /* the Cursed Coins, for the shrine */
        char cc[24];
        snprintf(cc, sizeof cc, "%d CURSED COIN%s", RUN.coins, RUN.coins == 1 ? "" : "S");
        int cw = gfx_text_width(cc, 1);
        draw_coin(f, W - 10 - cw - 10, 12);
        gfx_text8(f, W - 10 - cw, 12, cc, RUN.coins ? (uint8_t)(R_GOLD + 10) : (uint8_t)(R_STONE + 8), 1);
    }
    char v[24], extra[16];
    int capped = 0;
    for (int st = 0; st < NSTAT; st++) {
        int x = st < 8 ? 14 : 162, y = 40 + (st % 8) * 9, w = st < 8 ? 140 : 144;
        float t = gear_total(g, st), t0 = gear_total(g0, st), add = 0.0f;
        if (st == ST_MOVE || st == ST_THRIFT) add = t - fminf(gear_total_raw(g, st), t);   /* Swiftness, Arcane Knowledge */
        total_text(g, st, v, sizeof v);
        int c = t > t0 + 1e-3f ? R_MOSS + 13 : (t < t0 - 1e-3f ? R_BLOOD + 13 : (t > 0.0f ? R_STONE + 13 : R_STONE + 7));
        if (gear_capped(g, st)) { c = t < t0 - 1e-3f ? c : R_GLOW + 12; capped = 1; }   /* at its cap */
        gfx_text8(f, x, y, SHORT[st], t > 0.0f ? (uint8_t)(R_STONE + 11) : (uint8_t)(R_STONE + 7), 1);
        int vw = gfx_text_width(v, 1);
        gfx_text8(f, x + w - vw, y, v, c, 1);
        if (add >= 0.5f) { snprintf(extra, sizeof extra, "+%.0f", add); gfx_text8(f, x + w - vw - 4 - gfx_text_width(extra, 1), y, extra, R_GOLD + 11, 1); }
    }
    if (capped) gfx_text8(f, W - 10 - gfx_text_width("BLUE: AT ITS CAP", 1), 26, "BLUE: AT ITS CAP", R_GLOW + 12, 1);
    /* the abilities, summed and capped */
    int n = 0, y = 117;
    for (int ab = 1; ab < NABIL; ab++) {
        float a = gear_ability(g, ab, 0), b = gear_ability(g, ab, 1);
        if (a <= 0.0f) continue;
        switch (ab) {
        case AB_SWIFT: snprintf(v, sizeof v, "SWIFT +%.0f%%", a); break;
        case AB_ARCANE: snprintf(v, sizeof v, "ARCANE +%.0f%%", a); break;
        case AB_TREASURE: snprintf(v, sizeof v, "TREASURE +%.0f%%", a); break;
        case AB_KEYMAKER: snprintf(v, sizeof v, "KEYMAKER %.0f CELLS", a); break;
        case AB_COMMUNE: snprintf(v, sizeof v, "COMMUNE %.0f, %.1f S", a, b / 10.0f); break;
        case AB_THORNS: snprintf(v, sizeof v, "THORNS %.0f", a); break;
        default: snprintf(v, sizeof v, "MIRROR %.0f%% AT %.0f%%", a, b); break;
        }
        gfx_text8(f, n % 2 ? 162 : 14, y + (n / 2) * 9, v, R_GOLD + 11, 1);
        n++;
    }
    if (!n) gfx_text8(f, 14, y, "NO ABILITIES WORN", R_STONE + 8, 1);
    /* the spellbook: number keys for the attack spells, the runes as they stand */
    y += ((n + 1) / 2 > 1 ? (n + 1) / 2 : 1) * 9 + 4;
    for (int i = 10; i < W - 10; i++) gfx_px(f, i, y, R_STONE + 5);
    y += 5;
    int x = 14;
    gfx_text8(f, x, y, "SPELLS", R_GOLD + 10, 1);
    x = 66;
    for (int sp = 0; sp < NATK; sp++) {
        if (g->atk[sp] < 0) continue;
        char e[16]; snprintf(e, sizeof e, "%d %s", sp + 1, ATK_SHORT[sp]);
        int w = gfx_text_width(e, 1);
        if (x + w > W - 10) { x = 66; y += 9; }
        gfx_text8(f, x, y, e, sp == G.spell ? (uint8_t)(R_GOLD + 12) : (uint8_t)(R_STONE + 11), 1);
        x += w + 8;
    }
    y += 10; x = 14;
    gfx_text8(f, x, y, "RUNES", R_GOLD + 10, 1);
    x = 66;
    int any = 0;
    for (int k = 0; k < NDEF; k++) {
        int q = g->def[k];
        if (q <= 0) continue;
        any = 1;
        int w = gfx_text_width(DEF_SHORT[k], 1), wt = gfx_text_width(Q_TAG[q - 1], 1);
        if (x + w + 5 + wt > W - 10) { x = 66; y += 9; }
        if (y > H - 14) break;
        int ramp = QUAL_RAMP[q - 1], cool = G.def_cd[k] > 0.0f;   /* dimmer while it comes round again */
        gfx_text8(f, x, y, DEF_SHORT[k], (uint8_t)(ramp + (cool ? 7 : 12)), 1);
        gfx_text8(f, x + w + 5, y, Q_TAG[q - 1], (uint8_t)(ramp + (cool ? 5 : 9)), 1);
        x += w + 5 + wt + 9;
    }
    if (!any) gfx_text8(f, x, y, "NONE YET", R_STONE + 8, 1);
}

/* Tab: what you wear and the bag (Enter moves between them) and the selected piece; A/D turns the page */
static void render_inventory(Frame *f) {
    dim_view(f);
    panel(f, 4, 4, W - 8, H - 8);
    if (G.inv_page) { render_caster(f); return; }
    gfx_text16(f, 14, 9, "WARDROBE", R_GOLD + 12, 1);
    gfx_text8(f, 14, 26, "W/S PICK  ENTER MOVE  A/D PAGE", R_STONE + 9, 1);
    bevel(f, 274, 8, 38, 46, 0);
    draw_face(f, 277, 11, 0);
    const Gear *g = &RUN.gear;
    int n = inv_rows(), sel = G.ui_sel < n ? G.ui_sel : n - 1;
    for (int r = 0; r < n; r++) {
        int y = r < NSLOT ? 40 + r * 10 : 124 + (r - NSLOT) * 10;
        const Piece *p = inv_piece(r);
        char nm[24];
        if (r < NSLOT) snprintf(nm, sizeof nm, "%s", SLOT_NAME[r]);
        else snprintf(nm, sizeof nm, "%s", p->kind ? KIND_NAME[p->kind] : "-");
        if (r == sel) gfx_rect(f, 10, y - 1, 102, 10, R_IRON + 3);
        int col = p->kind ? qual_text(p->quality) : R_STONE + 7;
        gfx_text8(f, 14, y, nm, col, 1);
        if (p->anointed) gfx_glyph8(f, 14 + gfx_text_width(nm, 1), y, '*', R_GOLD + 12, -1, 1);
    }
    char line[48];
    if (g->bag_size) snprintf(line, sizeof line, "BAG %d/%d", g->bag_size - gear_bag_free(g), g->bag_size);
    else snprintf(line, sizeof line, "NO BAG");
    gfx_text8(f, 14, 113, line, R_GOLD + 10, 1);
    /* the selected piece */
    snprintf(line, sizeof line, "%s", sel < NSLOT ? SLOT_NAME[sel] : "IN THE BAG");
    gfx_text8(f, 120, 40, line, R_STONE + 10, 1);
    static const char *const WHY[] = { "", "NOTHING THERE", "NO BAG TO HOLD IT", "THE BAG IS FULL" };
    int act = inv_action(sel);
    const char *what = act == INV_TO_BAG ? "ENTER: INTO THE BAG" : (act == INV_TO_WEAR ? "ENTER: WEAR IT" : WHY[-act]);
    gfx_text8(f, 120, 49, what, act > 0 ? (uint8_t)(R_GOLD + 11) : (uint8_t)(R_STONE + 8), 1);
    piece_card(f, 120, 62, W - 10 - 120, inv_piece(sel), g, &G.inv0);
}

/* Esc: the crypt makes its case for staying */
static void render_quit(Frame *f) {
    static const char *const LINE[] = {
        "THE MERCHANT WILL MISS YOU. THE MERCHANT WILL NEVER SAY SO.",
        "LEAVING? THE DEAD HAVE BEEN SO PATIENT WITH YOU.",
        "YOU STILL HAVE %d GOLD. IT SPENDS NOTHING UP THERE.",
        "OUT THERE IT IS JUST EMAIL. DOWN HERE YOU ARE A WIZARD.",
        "THE SKELETONS WERE JUST STARTING TO LIKE YOU.",
        "THE TORCHES WILL GO OUT WITHOUT YOU. ALL OF THEM.",
        "THE CAT IS WATCHING. THE CAT IS ALWAYS WATCHING.",
        "ONE MORE LEVEL. EVERYONE SAYS ONE MORE LEVEL.",
        "THE WRAITHS WILL TELL EVERYONE YOU RAN.",
        "YOUR DESK IS THE REAL TOMB, YOU KNOW.",
        "SOMEWHERE ON LEVEL %d A KEY IS STILL WAITING FOR YOU.",
        "WHO WILL PET THE MERCHANT WHILE YOU ARE GONE?",
        "THE SPIDERS SAY STAY AS LONG AS YOU LIKE. THE SPIDERS INSIST.",
        "GO, THEN. THE CRYPT WILL KEEP YOUR PLACE. AND YOUR NAME.",
        "REAL WIZARDS DO NOT HAVE MEETINGS.",
        "%d SOULS. SO CLOSE TO SOMETHING WONDERFUL.",
    };
    enum { NLINE = (int)(sizeof LINE / sizeof LINE[0]) };
    dim_view(f);
    int bw = 280, bh = 176, bx = (W - bw) / 2, by = (H - bh) / 2 - 6;
    panel(f, bx, by, bw, bh);
    gfx_text16(f, (W - gfx_text_width("LEAVE THE CRYPT?", 1)) / 2, by + 8, "LEAVE THE CRYPT?", R_BLOOD + 13, 1);
    char msg[96];
    int li = G.quit_line % NLINE, arg = li == 2 ? RUN.treasure : (li == 10 ? G.level : RUN.souls);
    snprintf(msg, sizeof msg, LINE[li], arg);
    int y = by + 30;                                               /* wrapped at word ends, 32 to a line */
    for (const char *p = msg; *p && y < by + 60; y += 10) {
        int n = (int)strlen(p);
        if (n > 32) { n = 32; while (n > 0 && p[n] != ' ') n--; if (n == 0) n = 32; }
        char ln[40]; memcpy(ln, p, (size_t)n); ln[n] = 0;
        gfx_text8(f, (W - gfx_text_width(ln, 1)) / 2, y, ln, R_STONE + 13, 1);
        p += n; while (*p == ' ') p++;
    }
    static const char *const OPT[Q_OPTIONS] = { "FLEE TO THE SURFACE", "STAY IN THE DARK", "SLEEPWALK", "CARVE A GRAVESTONE", "RISE FROM A GRAVE" };
    static const char *const NOTE[Q_OPTIONS] = { "YOUR LEVEL, GOLD AND GEAR ARE KEPT", "BACK TO THE CRYPT", "THE AUTOPILOT WALKS ON WITHOUT YOU",
                                                 "SAVE THIS RUN IN ONE OF 3 SLOTS", "LOAD A RUN FROM ONE OF 3 SLOTS" };
    for (int i = 10 + bx; i < bx + bw - 10; i += 2) gfx_px(f, i, by + 120, R_STONE + 5);   /* leaving, then the graves */
    for (int i = 0; i < Q_OPTIONS; i++) {
        int oy = by + 66 + i * 18 + (i >= Q_CARVE ? 4 : 0), sel = i == G.ui_sel;
        int w = gfx_text_width(OPT[i], 1), ox = (W - w) / 2;
        gfx_text16(f, ox, oy, OPT[i], sel ? (uint8_t)C_WHITE : (uint8_t)(R_STONE + 9), 1);
        if (sel) {                                                 /* a candle flame for a cursor */
            int fl = (int)(G.t * 10.0) & 1;
            for (int k = 0; k < 2; k++) {
                int cx = k ? ox + w + 8 : ox - 12;
                for (int yy = 0; yy < 9; yy++)
                    for (int xx = -2; xx <= 2; xx++)
                        if (abs(xx) * 3 <= yy + fl) gfx_px(f, cx + xx, oy + 3 + yy, (uint8_t)(R_FLAME + 15 - yy));
            }
        }
    }
    const char *note = NOTE[clampi(G.ui_sel, 0, Q_OPTIONS - 1)];
    if (CON.used && G.ui_sel == Q_FLEE) note = "THE DEAD TOOK NOTE: NOTHING KEPT";
    if (CON.used && G.ui_sel == Q_CARVE) note = "THE DEAD TOOK NOTE: NO GRAVES";
    gfx_text8(f, (W - gfx_text_width(note, 1)) / 2, by + bh - 14, note, R_GOLD + 9, 1);
}

/* a headstone, 22 wide and 26 tall, its numeral cut into it */
static void headstone(Frame *f, int x, int y, int i, int used) {
    int w = 22, h = 26, r = w / 2;
    for (int yy = 0; yy < h; yy++)
        for (int xx = 0; xx < w; xx++) {
            float dx = xx + 0.5f - r, dy = yy + 0.5f - r;
            if (yy < r && dx * dx + dy * dy > r * r) continue;       /* the rounded top */
            int edge = xx == 0 || xx == w - 1 || yy == h - 1 || (yy < r && dx * dx + dy * dy > (r - 1.2f) * (r - 1.2f));
            int k = used ? (edge ? 6 : 10 - (xx + yy) % 3 / 2) : (edge ? 4 : 6);
            gfx_px(f, x + xx, y + yy, (uint8_t)(R_STONE + k));
        }
    for (int xx = -3; xx < w + 3; xx++) gfx_px(f, x + xx, y + h, R_STONE + 3);   /* the ground it stands in */
    int tw = gfx_text_width(ROMAN[i], 1);
    gfx_text8(f, x + (w - tw) / 2, y + 9, ROMAN[i], used ? R_STONE + 3 : R_STONE + 2, 1);
}

/* CARVE A GRAVESTONE / RISE FROM A GRAVE: the three save slots */
static void render_graves(Frame *f) {
    static const char *const MONTH[12] = { "JAN", "FEB", "MAR", "APR", "MAY", "JUN", "JUL", "AUG", "SEP", "OCT", "NOV", "DEC" };
    dim_view(f);
    int bw = 288, bh = 150, bx = (W - bw) / 2, by = (H - bh) / 2 - 6;
    panel(f, bx, by, bw, bh);
    const char *title = G.graves_mode ? "RISE FROM A GRAVE" : "CARVE A GRAVESTONE";
    gfx_text16(f, (W - gfx_text_width(title, 1)) / 2, by + 8, title, G.graves_mode ? R_BLOOD + 13 : R_GOLD + 12, 1);
    for (int i = 0; i < SLOT_COUNT; i++) {
        int y = by + 30 + i * 34, sel = i == G.ui_sel;
        Grave g = grave_info(i);
        if (sel) gfx_rect(f, bx + 6, y - 3, bw - 12, 33, R_IRON + 3);
        headstone(f, bx + 14, y, i, g.used);
        char l1[48], l2[48];
        if (g.used) {
            snprintf(l1, sizeof l1, "LEVEL %d  $%d  %d SOULS", g.level, g.gold, g.souls);
            time_t when = (time_t)g.when;
            struct tm *tm = g.when > 0 ? localtime(&when) : NULL;
            if (tm) snprintf(l2, sizeof l2, "%d %s %d  %02d:%02d  HP %d", tm->tm_mday, MONTH[tm->tm_mon % 12], tm->tm_year + 1900, tm->tm_hour, tm->tm_min, g.hp);
            else snprintf(l2, sizeof l2, "HP %d", g.hp);
        } else {
            snprintf(l1, sizeof l1, "UNMARKED");
            snprintf(l2, sizeof l2, "AN EMPTY GRAVE");
        }
        gfx_text8(f, bx + 46, y + 5, l1, sel ? (uint8_t)C_WHITE : (uint8_t)(g.used ? R_STONE + 12 : R_STONE + 9), 1);
        gfx_text8(f, bx + 46, y + 16, l2, R_STONE + 9, 1);
    }
    const char *note = G.graves_mode ? "ENTER RISES   ESC GOES BACK" : "ENTER CARVES   ESC GOES BACK";
    int col = R_GOLD + 9;
    if (G.graves_msg == GM_EMPTY) { note = "NOTHING LIES HERE"; col = R_STONE + 10; }
    else if (G.graves_msg == GM_ARMED && G.graves_arm == G.ui_sel) {
        note = G.graves_mode ? "ENTER AGAIN. THIS RUN WILL BE LOST" : "ENTER AGAIN TO CARVE OVER IT";
        col = R_BLOOD + 13;
    }
    gfx_text8(f, (W - gfx_text_width(note, 1)) / 2, by + bh - 14, note, col, 1);
}


/* ---- the tracker's pop-up, and the bestiary (B) ---- */

static const char *const RANK_ROMAN[NRANK] = { "I", "II", "III", "IV", "V", "GILDED" };
static const int RANK_COL[NRANK] = { R_BONE + 13, R_MOSS + 13, R_GHOST + 13, R_BLOOD + 13, R_IRON + 11, R_GOLD + 11 };

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
    const char *head = t->spr == SP_CCOIN ? "FOR THE SHRINE" : t->progress ? "PROGRESS" : "ACHIEVEMENT";
    int tw = (int)strlen(head);
    if ((int)strlen(a) > tw) tw = (int)strlen(a);
    if ((int)strlen(b) > tw) tw = (int)strlen(b);
    if ((int)strlen(t->desc) > tw) tw = (int)strlen(t->desc);
    int lines = 3 + (b[0] != 0), bw = 38 + tw * 8 + 6, bh = lines * 9 + 8 > 36 ? lines * 9 + 8 : 36;
    int x = W - (int)((float)(bw + 3) * k), y = 3;
    int edge = t->progress ? R_STONE + 9 : R_GOLD + 9;
    gfx_rect(f, x, y, bw, bh, 0);
    for (int i = 0; i < bw; i++) { gfx_px(f, x + i, y, (uint8_t)edge); gfx_px(f, x + i, y + bh - 1, (uint8_t)(edge - 4)); }
    for (int i = 0; i < bh; i++) { gfx_px(f, x, y + i, (uint8_t)edge); gfx_px(f, x + bw - 1, y + i, (uint8_t)(edge - 4)); }
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
    for (int lv = 1; lv <= 20; lv++) for (int k = 0; k < NKINDS; k++) if (KINDS[k].debut == lv && (KINDS[k].flags & KF_PAGE)) out[n++] = k;
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
        snprintf(l->title, sizeof l->title, "THE %s DESCENT", ORDINAL[k]);
        snprintf(l->prog, sizeof l->prog, "L%d", k * DESCENT_LEN);
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
    if (dx) { G.best_page = (G.best_page + dx + pages) % pages; G.best_scroll = 0; best_default_rank(); snd_sfx_ex(SFX_TICK, 0.6f, 0.5f); }
    if (!dy) return;
    if (G.best_page < nk) {                                        /* through the ranks it comes in */
        int rk[NRANK], nr = kind_ranks(order[G.best_page], rk), at = 0;
        for (int j = 0; j < nr; j++) if (rk[j] == G.best_rank) at = j;
        G.best_rank = rk[(at + dy + nr) % nr];
    } else {
        static AchLine L[256];
        int n = ach_lines(L, 256);
        G.best_scroll = clampi(G.best_scroll + dy * 3, 0, n > ACH_ROWS ? n - ACH_ROWS : 0);
    }
    snd_sfx_ex(SFX_TICK, 0.6f, 0.5f);
}

static void render_bestiary(Frame *f) {
    dim_view(f);
    panel(f, 4, 4, W - 8, H - 8);
    int order[NKINDS], nk = best_kinds(order), pages = nk + 1;
    char line[64];
    gfx_text16(f, 14, 9, G.best_page < nk ? "BESTIARY" : "ACHIEVEMENTS", R_GOLD + 12, 1);
    gfx_text8(f, 14, 26, G.best_page < nk ? "A/D CREATURE  W/S RANK  B CLOSE" : "A/D PAGE  W/S SCROLL  B CLOSE", R_STONE + 9, 1);
    snprintf(line, sizeof line, "%d / %d", G.best_page + 1, pages);
    if (G.best_page < nk) gfx_text8(f, W - 14 - gfx_text_width(line, 1), 13, line, R_STONE + 11, 1);
    const Record *r = &RUN.rec;
    if (G.best_page >= nk) {                                       /* the achievements, earned and not */
        static AchLine L[256];
        int n = ach_lines(L, 256), got = 0;
        for (int i = 0; i < n; i++) got += L[i].done;
        snprintf(line, sizeof line, "%d OF %d EARNED", got, n);    /* in the page count's place: it is the last page */
        gfx_text8(f, W - 14 - gfx_text_width(line, 1), 13, line, R_GOLD + 10, 1);
        for (int i = 0; i < ACH_ROWS && G.best_scroll + i < n; i++) {
            const AchLine *l = &L[G.best_scroll + i];
            int y = 40 + i * 10;
            gfx_text8(f, 14, y, l->done ? "*" : "-", l->done ? (uint8_t)(R_GOLD + 12) : (uint8_t)(R_STONE + 7), 1);
            gfx_text8(f, 26, y, l->title, l->done ? (uint8_t)C_WHITE : (uint8_t)(R_STONE + (l->known ? 11 : 8)), 1);
            gfx_text8(f, W - 14 - gfx_text_width(l->prog, 1), y, l->prog, l->done ? (uint8_t)(R_GOLD + 11) : (uint8_t)(R_STONE + 10), 1);
        }
        if (n > ACH_ROWS) {                                        /* where in the list */
            int bar = 150 * ACH_ROWS / n, at = 150 * G.best_scroll / n;
            for (int y = 0; y < bar; y++) gfx_px(f, W - 8, 40 + at + y, R_STONE + 9);
        }
        return;
    }
    int k = order[G.best_page], met = kind_met(k), seen = r->seen[k] >> G.best_rank & 1 || r->kills[k][G.best_rank] > 0;
    int spirit = (KINDS[k].flags & KF_SPIRIT) != 0;                /* the spectre and the Wraith: no rank, and nothing kills them */
    bevel(f, 12, 38, 104, 104, 0);
    const uint8_t *lut = KINDS[k].family >= 0 && G.best_rank != RK_I ? crawl_rank_lut[KINDS[k].family][G.best_rank] : NULL;
    /* it walks on the spot and turns about: towards you, its right side, towards you, its left */
    float ph = fmodf(G.ui_t, 6.4f);
    int step = (int)(G.ui_t * 4.0f) & 1, facing = (int)(ph / 1.6f);   /* 0 front, 1 right, 2 front, 3 left */
    int spr = facing & 1 ? (step ? KINDS[k].side2 : KINDS[k].side) : (step ? KINDS[k].spr2 : KINDS[k].spr);
    ui_sprite_ex(f, spr, 16, 42, 96, lut, !met || !seen, facing == 3);
    int x0 = 124;
    gfx_text16(f, x0, 38, met ? KINDS[k].name : "???", C_WHITE, 1);
    if (spirit) snprintf(line, sizeof line, "A SPIRIT: NO RANK");
    else if (seen) snprintf(line, sizeof line, "RANK %s  %s", RANK_ROMAN[G.best_rank], G.best_rank == RK_I ? "" : rank_word(k, G.best_rank));
    else snprintf(line, sizeof line, "RANK %s  NOT MET YET", RANK_ROMAN[G.best_rank]);
    gfx_text8(f, x0, 57, line, seen || spirit ? (uint8_t)RANK_COL[G.best_rank] : (uint8_t)(R_STONE + 8), 1);
    if (met) text_wrap(f, x0, 70, KINDS[k].lore, 23, R_STONE + 12, 4);
    else text_wrap(f, x0, 70, "NOT YET MET. IT IS DOWN THERE SOMEWHERE.", 23, R_STONE + 8, 4);
    /* slain, rank by rank, against what each achievement asks */
    int rk[NRANK], nr = kind_ranks(k, rk);
    gfx_text8(f, x0, 108, "SLAIN", R_GOLD + 10, 1);
    if (spirit) gfx_text8(f, x0 + 48, 108, "IT CANNOT BE", R_STONE + 9, 1);
    else for (int j = 0; j < nr; j++) {
        int need = slayer_need(k, rk[j]), have = r->kills[k][rk[j]], done = ach_has(k * NRANK + rk[j]);
        snprintf(line, sizeof line, "%-3s%3d/%d", rk[j] == RK_GILDED ? "G" : RANK_ROMAN[rk[j]], have < 999 ? have : 999, need);
        int cx = x0 + (j & 1) * 92, cy = 118 + (j / 2) * 9;
        gfx_text8(f, cx, cy, line, done ? (uint8_t)(R_GOLD + 12) : (rk[j] == G.best_rank ? (uint8_t)C_WHITE : (uint8_t)(R_STONE + 11)), 1);
    }
    /* what hurts it, learnt from three kills with each spell */
    for (int i = 10; i < W - 10; i++) gfx_px(f, i, 147, R_STONE + 5);
    gfx_text8(f, 14, 151, "WHAT HURTS IT", R_GOLD + 10, 1);
    if (spirit) { gfx_text8(f, 14, 161, "NOTHING YOU CARRY TOUCHES IT", R_STONE + 11, 1); return; }
    char more[80] = "MORE:", less[80] = "LESS:", same[80] = "AS USUAL:", untried[80] = "NOT YET LEARNT:";
    int nm = 0, nl = 0, ns = 0, nu = 0;
    for (int sp = 0; sp < AT_BFS; sp++) {
        char *dst; int *cnt;
        float wv = KINDS[k].weak[sp];
        if (r->spell_kills[k][sp] < 3) { dst = untried; cnt = &nu; }
        else if (wv > 1.01f) { dst = more; cnt = &nm; }
        else if (wv < 0.99f) { dst = less; cnt = &nl; }
        else { dst = same; cnt = &ns; }
        size_t len = strlen(dst);
        snprintf(dst + len, sizeof more - len, " %s", ATK_SHORT[sp]);
        (*cnt)++;
    }
    int y = 161;
    if (nm) { gfx_text8(f, 14, y, more, R_MOSS + 13, 1); y += 9; }
    if (nl) { gfx_text8(f, 14, y, less, R_BLOOD + 13, 1); y += 9; }
    if (ns && y < 186) { gfx_text8(f, 14, y, same, R_STONE + 12, 1); y += 9; }
    if (nu && y < 186) text_wrap(f, 14, y, untried, 36, R_STONE + 8, (195 - y) / 9);
}

/* the L menu: a small plate over the crypt, a wisp of each kind that can be cast now and its price */
static void render_wispmenu(Frame *f) {
    static const char *const KEYW[NKEYS] = { "THE GOLD KEY", "THE SILVER KEY", "THE CRIMSON KEY" };
    WispChoice c[WISP_CHOICES];
    int n = wisp_choices(c), sel = clampi(G.ui_sel, 0, n - 1);
    dim_view(f);
    int pw = 296, ph = 48 + n * 13 + (RUN.wisp_auto ? 26 : 16), px = (W - pw) / 2, py = 24;
    panel(f, px, py, pw, ph);
    gfx_text16(f, px + 10, py + 6, "WISPS", R_GOLD + 12, 1);
    char sp[24], line[40];
    spirit_text(sp, sizeof sp);
    snprintf(line, sizeof line, "SPIRIT %s", sp);
    gfx_text8(f, px + pw - 10 - gfx_text_width(line, 1), py + 10, line, R_GLOW + 12, 1);
    gfx_text8(f, px + 10, py + 24, "W/S CHOOSE  ENTER CASTS  ESC CLOSES", R_STONE + 9, 1);
    for (int r = 0; r < n; r++) {
        int y = py + 40 + r * 13, ok = c[r].cost <= 0.0f || spirit() + 1e-4f >= c[r].cost;
        if (r == sel) gfx_rect(f, px + 6, y - 3, pw - 12, 13, R_IRON + 3);
        Wisp probe;
        probe.kind = c[r].kind; probe.color = c[r].color;
        wisp_ramp = wisp_ramp_of(&probe);                          /* a little wisp of its colour */
        float t = (float)G.t * 9.0f + r;
        for (int pass = 0; pass < 2; pass++) wisp_ball(f, px + 16, y + 3.5f, 3.2f + 0.3f * sinf(t), 0.8f, pass, 0.0f);
        const char *what = c[r].kind == WK_EXIT ? "THE WAY OUT" : (c[r].kind == WK_KEY ? KEYW[c[r].color] : "WHAT THE ALTAR HIDES");
        gfx_text8(f, px + 28, y, what, r == sel ? (uint8_t)C_WHITE : (uint8_t)(R_STONE + 12), 1);
        const char *cost = c[r].cost <= 0.0f ? "FREE" : "1 SOUL";
        gfx_text8(f, px + pw - 12 - gfx_text_width(cost, 1), y, cost, ok ? (uint8_t)(R_GOLD + 11) : (uint8_t)(R_BLOOD + 13), 1);
    }
    int ty = py + 44 + n * 13 + 4;
    for (int i = px + 10; i < px + pw - 10; i++) gfx_px(f, i, ty - 5, R_STONE + 5);
    gfx_text8(f, px + 10, ty, RUN.wisp_auto ? "[X]" : "[ ]", RUN.wisp_auto ? (uint8_t)(R_GOLD + 12) : (uint8_t)(R_STONE + 10), 1);
    gfx_text8(f, px + 38, ty, "L ALWAYS CASTS THIS  (SPACE)", R_STONE + 12, 1);
    if (RUN.wisp_auto) gfx_text8(f, px + 38, ty + 10, "HOLD L FOR THIS LIST", R_STONE + 9, 1);
}

/* the altar's menu: its gifts, a little fire of each one's colour, the price, and a note on the one chosen */
static void render_altarmenu(Frame *f) {
    static const int RAMP[3] = { R_PURPLE, R_GLOW, R_GOLD };
    AltarRow r[3];
    altar_rows(r);
    int sel = clampi(G.ui_sel, 0, 2);
    dim_view(f);
    int pw = 296, ph = 112, px = (W - pw) / 2, py = 24;
    panel(f, px, py, pw, ph);
    gfx_text16(f, px + 10, py + 6, "THE BLACK ALTAR", R_GOLD + 12, 1);
    char sp[24], line[40];
    spirit_text(sp, sizeof sp);
    snprintf(line, sizeof line, "SPIRIT %s", sp);
    gfx_text8(f, px + pw - 10 - gfx_text_width(line, 1), py + 10, line, R_GLOW + 12, 1);
    gfx_text8(f, px + 10, py + 24, "W/S CHOOSE  ENTER TAKES  ESC CLOSES", R_STONE + 9, 1);
    for (int i = 0; i < 3; i++) {
        int y = py + 40 + i * 13, pay = r[i].price <= 0 || spirit() + 1e-4f >= (float)r[i].price;
        if (i == sel) gfx_rect(f, px + 6, y - 3, pw - 12, 13, R_IRON + 3);
        float t = (float)G.t * 9.0f + i;
        if (i == 1) { wisp_ramp = R_GOLD + 4; wisp_ball(f, px + 16, y + 3.5f, 3.4f, 0.6f, 0, 0.0f); }   /* the blessing's halo */
        wisp_ramp = RAMP[i];
        for (int pass = 0; pass < 2; pass++) wisp_ball(f, px + 16, y + 3.5f, (r[i].ok ? 3.2f : 2.2f) + 0.3f * sinf(t), 0.8f, pass, 0.0f);
        uint8_t c = !r[i].ok ? (uint8_t)(R_STONE + 8) : (i == sel ? (uint8_t)C_WHITE : (uint8_t)(R_STONE + 12));
        gfx_text8(f, px + 28, y, r[i].what, c, 1);
        if (r[i].cost[0])
            gfx_text8(f, px + pw - 12 - gfx_text_width(r[i].cost, 1), y, r[i].cost, pay ? (uint8_t)(R_GOLD + 11) : (uint8_t)(R_BLOOD + 13), 1);
    }
    int ty = py + 40 + 3 * 13 + 6;
    for (int i = px + 10; i < px + pw - 10; i++) gfx_px(f, i, ty - 4, R_STONE + 5);
    gfx_text8(f, px + 10, ty + 2, r[sel].note, r[sel].ok ? (uint8_t)(R_STONE + 12) : (uint8_t)(R_STONE + 10), 1);
    if (G.blessed) gfx_text8(f, px + 10, ty + 14, "YOUR EXIT WISPS ARE BLESSED", R_GOLD + 10, 1);
}

/* the shrine's screen: the nine slots on the left, its actions (or a question) on the right with a note
 * under them, and the runes you hold along the bottom */
static void render_shrinemenu(Frame *f) {
    dim_view(f);
    int px = 6, py = 6, pw = W - 12, ph = H - 12;
    panel(f, px, py, pw, ph);
    gfx_text16(f, px + 10, py + 6, "THE RUNE SHRINE", R_GOLD + 12, 1);
    char line[96];
    snprintf(line, sizeof line, "CURSED COINS %d", RUN.coins);
    int lw = gfx_text_width(line, 1);
    gfx_text8(f, px + pw - 10 - lw, py + 10, line, R_GOLD + 11, 1);
    draw_coin(f, px + pw - 21 - lw, py + 10);
    int rank = RUN.shrine, ramp = rank ? QUAL_RAMP[rank - 1] : R_STONE;
    if (!rank) gfx_text8(f, px + 10, py + 26, "SANCTIFIED: IT GIVES NOTHING", R_STONE + 11, 1);
    else { snprintf(line, sizeof line, "CURSED: RUNES UP TO %s", QUAL_NAME[rank - 1]); gfx_text8(f, px + 10, py + 26, line, ramp_at(ramp, 12), 1); }
    /* the slots */
    int slots = G.sh_mode == SM_TAKE || G.sh_mode == SM_LETGO;
    for (int i = 0; i < SHRINE_SLOTS; i++) {
        int x = px + 8 + i % 3 * 36, y = py + 40 + i / 3 * 34, k = RUN.slot_def[i] - 1, q = RUN.slot_q[i];
        int fl = G.sh_flash[i] > 0.0f && ((int)(G.sh_flash[i] * 16.0f) & 1);
        gfx_rect(f, x, y, 32, 30, fl ? (uint8_t)(R_STONE + 7) : (uint8_t)(R_STONE + 2));
        uint8_t rim = slots && i == G.sh_slot ? (uint8_t)(G.sh_arm == i ? R_BLOOD + 13 : R_GOLD + 12) : (fl ? (uint8_t)C_WHITE : (uint8_t)(R_STONE + 6));
        for (int j = 0; j < 32; j++) { gfx_px(f, x + j, y, rim); gfx_px(f, x + j, y + 29, rim); }
        for (int j = 0; j < 30; j++) { gfx_px(f, x, y + j, rim); gfx_px(f, x + 31, y + j, rim); }
        if (k < 0) continue;
        int r = QUAL_RAMP[q - 1];
        draw_glyph(f, x + 11, y + 4, k, 2, ramp_at(r, 13));
        gfx_text8(f, x + (32 - gfx_text_width(Q_TAG[q - 1], 1)) / 2, y + 20, Q_TAG[q - 1], ramp_at(r, 10), 1);
    }
    /* the actions, or a question */
    int rx = px + 124, rr = px + pw - 10, ry = py + 41, ny;
    char note[160] = "";
    if (G.sh_mode == SM_FULL || G.sh_mode == SM_LEAVE) {
        static const char *const FULL[3] = { "LET GO OF SOME FIRST", "PULL ANYWAY", "NOT NOW" };
        static const char *const LEAVE[2] = { "CHOOSE A RUNE", "CLOSE FOR NOW" };
        int full = G.sh_mode == SM_FULL, n = full ? 3 : 2;
        if (full) snprintf(line, sizeof line, "THIS PULL MAY BRING %d, AND %d SLOT%s EMPTY. WHAT DOESN'T FIT FADES.",
                           pull_most(G.sh_coins), shrine_count(0), shrine_count(0) == 1 ? " IS" : "S ARE");
        else snprintf(line, sizeof line, "%d TAKE%s LEFT. UNUSED, %s LOST WHEN YOU LEAVE THE ROOM.", G.sh_takes, G.sh_takes == 1 ? "" : "S", G.sh_takes == 1 ? "IT IS" : "THEY ARE");
        int nl = text_wrap(f, rx, ry, line, 22, R_STONE + 13, 4);
        for (int i = 0; i < n; i++) {
            int y = ry + nl * 9 + 6 + i * 13;
            if (i == G.ui_sel) gfx_rect(f, rx - 4, y - 3, rr - rx + 8, 13, R_IRON + 3);
            gfx_text8(f, rx, y, full ? FULL[i] : LEAVE[i], i == G.ui_sel ? (uint8_t)C_WHITE : (uint8_t)(R_STONE + 12), 1);
        }
        ny = ry + nl * 9 + 10 + n * 13;
    } else {
        int n = shrine_rows();
        for (int i = 0; i < n; i++) {
            int y = ry + i * 11, sel = !slots && i == G.ui_sel, ok = 1;
            const char *what = "";
            char val[32] = "";
            if (!rank) { what = i ? "LEAVE" : "CURSE IT"; if (!i) { snprintf(val, sizeof val, "1 COIN"); ok = RUN.coins >= 1; } }
            else switch (i) {
            case SR_PULL: what = "PULL"; snprintf(val, sizeof val, "< %d COIN%s >", G.sh_coins, G.sh_coins == 1 ? "" : "S"); ok = G.sh_coins + fav_coins(G.sh_fav) <= RUN.coins; break;
            case SR_FAVOUR:
                what = "FAVOUR";
                if (!RUN.attuned) { snprintf(val, sizeof val, "NONE ATTUNED"); ok = 0; }
                else if (!G.sh_fav) snprintf(val, sizeof val, "< NONE >");
                else snprintf(val, sizeof val, "< %s +%d >", DEF_SHORT[fav_kind(G.sh_fav)], fav_coins(G.sh_fav));
                break;
            case SR_RAISE:
                what = "RAISE";
                if (rank >= NQUAL) { snprintf(val, sizeof val, "AT ITS HIGHEST"); ok = 0; }
                else { snprintf(val, sizeof val, "%s: %d", QUAL_NAME[rank], SHRINE_COST[rank]); ok = RUN.coins >= SHRINE_COST[rank]; }
                break;
            case SR_TAKE: what = "TAKE A RUNE"; snprintf(val, sizeof val, "%d LEFT", G.sh_takes); ok = G.sh_takes > 0 && shrine_count(1); break;
            case SR_LETGO: what = "LET GO OF ONE"; ok = shrine_count(1) > 0; break;
            default: what = "LEAVE"; break;
            }
            if (sel) gfx_rect(f, rx - 4, y - 2, rr - rx + 8, 11, R_IRON + 3);
            gfx_text8(f, rx, y, what, sel ? (uint8_t)C_WHITE : (uint8_t)(R_STONE + 12), 1);
            if (val[0]) gfx_text8(f, rr - gfx_text_width(val, 1), y, val, ok ? (uint8_t)(R_GOLD + 11) : (uint8_t)(R_STONE + 9), 1);
        }
        ny = ry + n * 11 + 3;
        if (slots) {                                               /* the rune in the slot chosen */
            int k = RUN.slot_def[G.sh_slot] - 1, q = RUN.slot_q[G.sh_slot];
            if (k < 0) snprintf(note, sizeof note, "AN EMPTY SLOT");
            else if (RUN.gear.def[k] > 0) snprintf(note, sizeof note, "%s %s. YOU HOLD %s.", QUAL_NAME[q - 1], DEF_NAME[k], QUAL_NAME[RUN.gear.def[k] - 1]);
            else snprintf(note, sizeof note, "%s %s. YOU HAVE NONE.", QUAL_NAME[q - 1], DEF_NAME[k]);
        } else if (!rank) snprintf(note, sizeof note, RUN.coins ? "ONE CURSED COIN WAKES IT, AND THEN IT GIVES RUNES." : "CURSED COINS LIE DEEP IN THE SECRETS.");
        else switch (G.ui_sel) {
        case SR_PULL: { int lo = G.sh_coins == 3 ? 3 : G.sh_coins, hi = pull_most(G.sh_coins);
            snprintf(note, sizeof note, "SHOWS %d TO %d RUNES. ONE TAKE A PULL.", lo, hi); break; }
        case SR_FAVOUR: snprintf(note, sizeof note, RUN.attuned ? "EACH COIN, ADDED TO THE PULL, MAKES IT A THIRD LIKELIER." : "IT LEARNS A KIND WHEN YOU HOLD ONE AT RARE."); break;
        case SR_RAISE: snprintf(note, sizeof note, "RAISED, IT GIVES BETTER RUNES."); break;
        case SR_TAKE: snprintf(note, sizeof note, "UNUSED TAKES ARE LOST WHEN YOU LEAVE THE ROOM."); break;
        case SR_LETGO: snprintf(note, sizeof note, "FREES A SLOT. THE RUNE IS GONE."); break;
        default: snprintf(note, sizeof note, "THE SLOTS KEEP THEIR RUNES FOR NEXT TIME."); break;
        }
    }
    if (G.sh_note_t > 0.0f && G.sh_mode != SM_FULL && G.sh_mode != SM_LEAVE) text_wrap(f, rx, ny + 2, G.sh_note, 22, R_GOLD + 11, 3);
    else if (note[0]) text_wrap(f, rx, ny + 2, note, 22, R_STONE + 11, 3);
    /* what you hold, under its own line */
    int y = py + 143, x = px + 10;
    for (int i = px + 8; i < px + pw - 8; i++) gfx_px(f, i, y - 2, R_STONE + 5);
    gfx_text8(f, x, y + 1, "YOURS", R_GOLD + 10, 1);
    y += 11;
    int any = 0;
    for (int k = 0; k < NDEF; k++) {
        int q = RUN.gear.def[k];
        if (q <= 0) continue;
        any = 1;
        int w = gfx_text_width(DEF_SHORT[k], 1), wt = gfx_text_width(Q_TAG[q - 1], 1);
        if (x + w + 5 + wt > px + pw - 10) { x = px + 10; y += 9; }
        gfx_text8(f, x, y, DEF_SHORT[k], ramp_at(QUAL_RAMP[q - 1], 12), 1);
        gfx_text8(f, x + w + 5, y, Q_TAG[q - 1], ramp_at(QUAL_RAMP[q - 1], 9), 1);
        x += w + 5 + wt + 9;
    }
    if (!any) gfx_text8(f, x, y, "NO RUNES YET", R_STONE + 9, 1);
    const char *hint = slots ? (G.sh_mode == SM_TAKE ? "WASD A SLOT  ENTER TAKES  ESC BACK" : "WASD A SLOT  ENTER LETS GO  ESC BACK")
                     : (G.sh_mode == SM_PULL ? "THE RUNES ARE COMING..." : "W/S CHOOSE  A/D CHANGE  ESC LEAVES");
    gfx_text8(f, px + 10, py + ph - 12, hint, R_STONE + 9, 1);
}

static void render_ui(Frame *f) {
    if (G.ui == UI_SHRINE) { render_shrinemenu(f); return; }
    if (G.ui == UI_WISPS) { render_wispmenu(f); return; }
    if (G.ui == UI_ALTAR) { render_altarmenu(f); return; }
    if (G.ui == UI_INVENTORY) render_inventory(f);
    else if (G.ui == UI_QUIT) render_quit(f);
    else if (G.ui == UI_GRAVES) render_graves(f);
    else if (G.ui == UI_BESTIARY) render_bestiary(f);
    else if (G.ui == UI_LETGO) render_letgo(f);
}

static void crawl_render(Frame *f) {
    f->w = W; f->h = H;
    memcpy(f->pal, crawl_pal, sizeof crawl_pal);
    f->fade = G.fade;
    if (G.tally) { render_tally(f); if (G.player && G.ui) render_ui(f); render_toasts(f); con_render(f); return; }
    g_camfog = fog_at(G.px, G.py);
    float ang = G.ang + G.kick;
    float dx = cosf(ang), dy = sinf(ang);
    float plx = -dy * PLANE_LEN, ply = dx * PLANE_LEN;
    float horizon = H / 2.0f + G.hurt * 3.0f;
    float camz = G.camz;
    G.camz -= 0.03f * G.hurt;
    fm_any = hz_marks();
    render_floor(f, dx, dy, plx, ply, horizon);
    render_walls(f, dx, dy, plx, ply, horizon);
    render_sprites(f, dx, dy, horizon);
    render_beams(f, dx, dy, horizon);
    render_wisps(f, dx, dy, horizon);
    G.camz = camz;
    if (G.web_hp > 0.0f && !G.dead && !G.shop) render_web(f);
    if (G.shop) render_shop_tags(f, dx, dy, horizon);
    if (!G.dead) { render_wards(f); render_hand(f); }
    if (G.shop) render_shop_card(f);
    if (G.shop || G.player) render_statusbar(f);                    /* a person gets the Wolfenstein bar, */
    if (G.player && !G.shop && G.bar <= 0.0f) render_readout(f);   /* and the small one while it is away */
    if (!G.shop && !G.player) { render_readout(f); render_face_popup(f); }
    if (!G.shop) { render_bossbar(f); render_card(f); }
    render_messages(f);
    if (G.player && G.ui) render_ui(f);
    render_toasts(f);
    tint(f);
    con_render(f);
}

const Scene scene_crawl = { "CATACOMB", W, H, SND_PROG_CRAWL, crawl_enter, crawl_update, crawl_render,
                            crawl_control, crawl_status, crawl_input, crawl_save, crawl_load };
