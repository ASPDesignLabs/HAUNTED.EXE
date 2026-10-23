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

#define W 320
#define H 200
#define MAPN 45                             /* 11 blocks of 3x3 cells and the walls between: 4 * 11 + 1 */
#define LMR 4                               /* light-map texels per cell */
#define LMN (MAPN * LMR)
#define PLANE_LEN 0.75f
#define FOCAL ((W / 2) / PLANE_LEN)
#define CRUISE 2.1f                         /* cells per second */
#define WALK 3.0f                           /* cells a second, for a person at the keys */

enum { K_FLOOR, K_WALL, K_DOOR, K_PILLAR, K_PROP };   /* K_PROP: low and solid (a grave, a fire pit): walked round, seen and cast over */
enum { TH_CORRIDOR, TH_OSSUARY, TH_CRYPT, TH_BRICK, TH_MOSS, TH_COUNT };
enum { PS_MOVE, PS_TURN, PS_DOOR, PS_AIM, PS_CAST, PS_UNAIM, PS_HOLD, PS_EXIT, PS_SHOP, PS_PLAYER, PS_SETTLE, PS_PUSH };
enum { MOB_SKEL, MOB_GHOST, MOB_BAT, MOB_WSKULL, MOB_SHIELD, MOB_SPIDER,
       MOB_SHAMBLER, MOB_DROWNED, MOB_HOUND, MOB_WRAITH, MOB_HUNTER, MOB_GHOUL, MOB_KNIGHT, MOB_BROOD, MOB_LING, MOB_SAC,
       MOB_SHADE, MOB_IMP, MOB_DEACON, MOB_GARGOYLE };   /* kept in saves (the record): new kinds go at the end */
enum { MS_WALK, MS_DIE, MS_RECOIL, MS_FLEE };   /* the last two: wraiths only */
enum { IT_COINS, IT_CHALICE, IT_GEMS, IT_CROWN, IT_CHEST, IT_KEY, IT_POTION, IT_URN, IT_RING };
static const int ITEM_VALUE[] = { 10, 25, 50, 100, 250, 0, 0, 0, 0 };
enum { MIS_BONE, MIS_FIRE, MIS_EMBER };     /* what the dead throw: bones, a wall skull's fireball, an imp's ember */
enum { DM_PIERCE = 1, DM_ARC = 2, DM_SLOW = 4 };   /* how a hit lands: through shields, round them, chilling */
#define DM_ATK(a) (((a) + 1) << 8)          /* and which spell it is, for the creature's weakness to it */
enum { HURT_BLOW, HURT_MAGIC, HURT_POISON };
#define NKEYS 3                             /* gold, silver, crimson */
#define SHOT_REFLECT NATK                   /* a shot kind: something the Mirror Veil sent back */

typedef struct { uint8_t kind, tex, theme, visits; int16_t region, door; uint8_t secret; } Cell;   /* secret: 1 a hidden closet, 2 its cracked wall */
typedef struct { int x, y, along_x; float open, idle; int opening, locked, color; } Door;   /* locked: 1 locked, 2 unlocked */
typedef struct { float x, y, power, radius, phase; int cx, cy, first, count; } Light;
typedef struct { float x, y, phase; int spr, nfr, flags; } Deco;   /* nfr: frames to cycle through */
enum { DF_SWAY = 1 };                                                /* hanging: swings a little, from the top */
typedef struct {
    int type, state, alive; float x, y, z, t, anim, speed; int cx, cy, tx, ty;
    int seen, attacking, hits, misses; float cool, throw_t, contact, orbit, stagger, ignore;   /* skeleton throws, bat bites */
    float fade, st;                                                /* wraiths: how far gone, time in state */
    float hp, maxhp, shield, face;                                 /* face: where a shield-bearer is looking */
    float frozen, feared, slowed, swing;                           /* frost nova, dread, frost shard; a sword raised */
    int sub, guard; float ax, ay, bx, by, sub_t;                   /* wall skulls and spiders: a phase, an anchor, a way back */
    int rank, home, hunts, last_atk, deco, raised;                 /* its rank (RK_*); the room it keeps to (-1: none); whether it may
                                                                      hunt now; the spell that last hurt it; the grave or puddle it
                                                                      rises from, or the bones a deacon calls; how many it has raised */
    float alert, dmgk, lost;                                       /* how long it will go on hunting; its rank's weight of blow; how
                                                                      long since it saw the caster */
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

typedef struct { int alive, kind, target; float x, y, z, vx, vy, life, dmg; uint32_t pierced; } Shot;
typedef struct { int alive, kind; float x, y, vx, vy, life, spin, dmg, life0; } Bone;   /* life0: an ember's whole flight */
typedef struct { int alive, type, cx, cy, seen, color, lx, ly, secret; float x, y, phase; } Item;   /* lx, ly: the lane cell it is taken from */
typedef struct { int alive; float x, y, t, r; int big; } Boom;
typedef struct { int alive, kind; float x0, y0, z0, x1, y1, z1, t; } Beam;   /* lightning and soul drain, for a moment */
typedef struct { int alive; float x, y, t, dps; int foe; } Burn;           /* hellfire on the floor; foe: an ember's, which burns the caster */
typedef struct { int alive; float x, y, t; } Pool;                         /* what a drowned one leaves: foul, a few seconds */
#define MAXPOOLS 4
typedef struct { int x, y, w, h, region, theme; } Room;
typedef struct { int fx, fy, d, sx, sy, tex, state, rolled; } Secret;   /* stand at f, press d: the wall at s slides back */
#define MAXSECRETS 3
#define SLIDE_CELLS 3                       /* how far a secret wall slides: right through its closet */
#define WISP_SOULS 10                       /* souls taken on a level before the wisp comes to the hand */
#define WISP_N 9                            /* links in its tail, from the hand to its body */
#define WISP_SPARKS 18

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
#define FACE_T 2.6f

static const int DX[4] = { 1, 0, -1, 0 }, DY[4] = { 0, 1, 0, -1 };
static const uint8_t BAYER[4][4] = { {0, 8, 2, 10}, {12, 4, 14, 6}, {3, 11, 1, 9}, {15, 7, 13, 5} };

static const char *const LEVEL_NAMES[20] = {                       /* five to a set; the last of each is its boss's */
    "THE OSSUARY", "HALL OF TORCHES", "CRYPT OF THE NAMELESS", "THE BONE GALLERY", "THE WARDEN'S HALL",
    "THE DROWNED VAULT", "CATACOMB OF SAINTS", "THE WEEPING STAIR", "CISTERN OF BONES", "THE ABBESS BELOW",
    "THE CHARNEL MINE", "WARRENS OF THE DEAD", "THE FOSSIL ROAD", "THE BROKEN SEAM", "THE COLOSSUS DEEP",
    "THE GLITTERING DARK", "VEINS OF THE EARTH", "THE EMBER ROOT", "THE BLACK GLASS", "THRONE OF THE HOLLOW KING",
};
/* the level's name; past the twentieth they come round again with a numeral: THE OSSUARY II */
static void level_name(int level, char *buf, int n) {
    static const char *const NUM[] = { "", " II", " III", " IV", " V", " VI", " VII", " VIII", " IX", " X" };
    int loop = crawl_loop(level);
    snprintf(buf, (size_t)n, "%s%s", LEVEL_NAMES[(level - 1) % 20], loop < 10 ? NUM[loop] : " X");
}

/* A run's record: what it has slain, met, found and cleared, and the achievements earned. It is kept
 * in the save with the run (version 2), so each save has its own. Its sizes are fixed so saves keep
 * their shape as the bestiary grows. */
#define REC_KINDS 24
#define ACH_BITS 256
typedef struct {
    uint16_t kills[REC_KINDS][NRANK];
    uint8_t spell_kills[REC_KINDS][NATK];   /* by which spell, to 255: three, and its effect is known */
    uint8_t seen[REC_KINDS];                /* the ranks of each kind met */
    uint16_t secrets, descents;             /* secrets found; times all the way down (20, 40, 60 ...) */
    uint32_t cleared;                       /* levels 1 to 20 cleared, a bit each */
    uint8_t got[ACH_BITS / 8];              /* achievements earned, a bit each (see ach_bit) */
} Record;
#define REC_BYTES (REC_KINDS * NRANK * 2 + REC_KINDS * NATK + REC_KINDS + 2 + 2 + 4 + ACH_BITS / 8)

/* The run: what carries from level to level, through death and across scene switches */
static struct {
    int active, level, hp, treasure, souls;
    float soul_frac;
    Gear gear;
    int goal;                               /* gold it is saving towards (0: not saving) */
    int shop_next;                          /* the merchant comes first when the level is (re)entered */
    int loaded;                             /* just read from a save: nothing in G to carry over */
    Record rec;
} RUN;

static struct {
    uint32_t rs;
    double t, level_t;
    int level;
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
    int wraith_done; float wraith_try;      /* the Wraith has come this level; when to look for a place for it */
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
    float ward_hp, ward_t, mirror_t, slip_t, reap_t, mend_t, mend_rate;
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
    Secret secrets[MAXSECRETS]; int nsecrets, secrets_found;
    int slide, seek_secret; float slide_t;  /* the wall sliding back, and how far; a secret the autopilot means to press */
    int16_t route[MAPN][MAPN];              /* steps to where the wisp leads, every way round; -1 where none */
    int route_gx, route_gy;                 /* where that is: the exit's doorstep, or a key it needs first */
    int wisp_on; float wisp_a, wisp_dir;    /* the will-o'-wisp: come yet, how bright, which way it leans (world) */
    float wtx[WISP_N], wty[WISP_N];         /* its tail on the screen, from the hand out to its body */
    float wbvx, wbvy, wcx, wcy, wcvx, wcvy; /* its body's drift; the bend in its tail, which lags */
    float wscale, wspark_t;                 /* its size as it leans near or far; the next spark */
    struct { float x, y, vx, vy, life, max; } wsparks[WISP_SPARKS];
    int tally;                              /* the level-complete screen */
    double tally_t, clear_time;
    int tally_shown[5], tally_done;           /* one a row: time, kills, treasure, souls, secrets */
    float hurt, gold, readout, kick, heartbeat, chill;              /* chill: the cold flash when a wraith shrieks */
    int dead;
    int retries;                        /* deaths on this level so far: the crypt goes a little easier each time */
    double dead_t;

    /* the merchant's antechamber, before each level */
    int shop, shop_step, shop_pick, shop_bought, shop_look, shop_gold, shop_saving, shop_heal;
    Offer offers[NPOOL];
    int offer_gone[NPOOL];
    double shop_t;
    float sa0, sa1, sdur, sx0, sy0, sx1, sy1, smove, sparkle;    /* turning and walking: from, to, how long */
    float cat_blink, cat_slow, cat_ear, cat_trill, cat_look, purr_t, cat_next_blink, cat_next_ear, cat_tail_k;
    int cat_ear_side;

    /* a person at the controls */
    int player, ui, ui_sel, quit_line, status_ret;
    int graves_mode, graves_arm, graves_msg;    /* the save slots: carving (0) or rising (1); a second Enter's slot; why */
    int best_page, best_rank, best_scroll;      /* the bestiary: which creature (or the achievements), which rank, how far down */
    float ui_t;                                 /* a clock that runs while a menu holds the crypt still */
    float pc_t, pc_len, grab_t, hint_t, msg_t;     /* their cast; reaching for a ware; the hints; a message */
    int pc_fired;
    char msg[64];
    Gear inv0;                              /* the gear as it was when the inventory opened */
    int inv_page;                           /* the wardrobe (0) or the caster's numbers and spellbook (1) */

    float fade, card;
    int ending;
    double end_t;
    float mood_t;
} G;
static float zbuf[W];
enum { UI_NONE, UI_INVENTORY, UI_QUIT, UI_GRAVES, UI_BESTIARY };   /* the menus a person can open */
static Input IN;                            /* their keys and mouse, this step */

static uint8_t ramp_at(int ramp, int k) { return (uint8_t)(ramp + clampi(k, 0, ramp == R_GOLD ? 12 : 15)); }   /* the gold ramp is short */
static uint8_t qual_text(int q) { return q == Q_COMMON ? (uint8_t)C_WHITE : ramp_at(QUAL_RAMP[q], 13); }   /* white, then green, blue, purple, gold */
static int rnd(int n) { return (int)(rng_next(&G.rs) % (uint32_t)n); }
static float decay(void) { return crawl_decay(G.level); }        /* rot and ruin: 0 as a set begins, 1 on its last level */
static float depth(void) { return crawl_depth(G.level); }         /* how far down: 0 in the Crypts, 1 from the Deep on */
/* a creature's blow: a little more each level, more by its rank, less after each death on the level */
static float mob_damage(const Mob *m) {
    float d = (4.0f + 0.8f * (float)G.level) * m->dmgk;
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
    static Cell solid = { K_WALL, T_STONE, 0, 0, -1, -1, 0 };
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
    return d->locked != 1 || G.has_key[d->color];
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
typedef struct { uint8_t kind, theme, w, lo, trunk; int8_t room; int16_t region; } MCell;
static MCell MM[MG][MG];
static int mroom[12][4];                                         /* the rooms on the plan: x, y, w, h */
static int mstart(int m) { return (m & 1) ? 2 * m - 1 : 2 * m; }  /* the first cell of plan row/column m */
static MCell *mc(int x, int y) {
    static MCell solid = { K_WALL, 0, 0, 0, 0, -1, -1 };
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
                if (!mopen(x, y) || MM[y][x].room >= 0 || mdeg(x, y) > 1) continue;
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

/* Secrets: a block left solid beside a hall or room becomes a closet of loot behind a wall with
 * cracks in it, which slides back when pressed. One on the first two levels, two from level 3,
 * three from level 6. They never lead anywhere but the closet. */
static void place_secrets(void) {
    G.nsecrets = G.secrets_found = 0; G.slide = -1;
    int want = G.level >= 6 ? 3 : (G.level >= 3 ? 2 : 1);
    static int cand[MG * MG * 4];
    int nc = 0;
    for (int my = 1; my < MG; my += 2)
        for (int mx = 1; mx < MG; mx += 2) {
            if (mopen(mx, my)) continue;
            for (int d = 0; d < 4; d++) {                          /* d: from the closet towards where it is pressed */
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
    for (int i = 0; i < nc && G.nsecrets < want; i++) {
        int b = cand[i] / 4, d = cand[i] % 4, mx = b % MG, my = b / MG;
        int cx = mx + DX[d], cy = my + DY[d], sx = mstart(cx) + (cx & 1), sy = mstart(cy) + (cy & 1);
        int clash = MM[my][mx].trunk;                              /* trunk marks a closet already taken */
        for (int k = 0; k < G.nsecrets; k++) clash |= abs(G.secrets[k].sx - sx) + abs(G.secrets[k].sy - sy) < 8;
        if (clash) continue;
        MM[my][mx].trunk = 1;
        for (int y = mstart(my); y < mstart(my) + 3; y++)
            for (int x = mstart(mx); x < mstart(mx) + 3; x++) { carve(x, y, -2, TH_CORRIDOR); G.map[y][x].secret = 1; }
        G.map[sy][sx].secret = 2;
        G.secrets[G.nsecrets++] = (Secret){ sx + DX[d], sy + DY[d], (d + 2) & 3, sx, sy, 0, 0, 0 };
    }
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

static void gen_level(void) {
    memset(G.map, 0, sizeof G.map);
    for (int y = 0; y < MAPN; y++)
        for (int x = 0; x < MAPN; x++) { G.map[y][x].kind = K_WALL; G.map[y][x].region = -1; G.map[y][x].door = -1; }
    memset(MM, 0, sizeof MM);
    for (int y = 0; y < MG; y++) for (int x = 0; x < MG; x++) { MM[y][x].kind = K_WALL; MM[y][x].region = -1; MM[y][x].room = -1; }
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
    for (int y = 1; y < MG - 1; y += 2)
        for (int x = 1; x < MG - 1; x += 2)
            if (MM[y][x].kind == K_WALL && nreg < 1000) mz_grow(x, y, nreg++);

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

static int item_at(int x, int y) {
    for (int i = 0; i < G.nitems; i++) if (G.items[i].alive && G.items[i].cx == x && G.items[i].cy == y) return 1;
    return 0;
}

static int free_floor(int x, int y) {
    if (cell(x, y)->kind != K_FLOOR || cell(x, y)->secret || G.bulky[y][x] || item_at(x, y) || lane_dist(x, y) > 1) return 0;   /* within reach of a lane */
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
    /* and in each secret closet, off to the sides at the back where it glints past the wall as it goes:
     * treasure, and often something to heal with. None of it counts towards the level's treasure, so a
     * secret is pure gain */
    static const int LOOT[6] = { IT_CHEST, IT_CROWN, IT_GEMS, IT_GEMS, IT_CROWN, IT_CHALICE };
    for (int k = 0; k < G.nsecrets; k++) {
        const Secret *sc = &G.secrets[k];
        int px = DY[sc->d], py = DX[sc->d], spots[4][2], ns = 0;  /* beside the last two cells of the closet */
        for (int depth = 2; depth <= 3; depth++)
            for (int side = -1; side <= 1; side += 2) { spots[ns][0] = sc->sx + DX[sc->d] * depth + px * side; spots[ns][1] = sc->sy + DY[sc->d] * depth + py * side; ns++; }
        for (int i = ns - 1; i > 0; i--) { int j = rnd(i + 1); int t0 = spots[i][0], t1 = spots[i][1]; spots[i][0] = spots[j][0]; spots[i][1] = spots[j][1]; spots[j][0] = t0; spots[j][1] = t1; }
        int n = 2 + (G.level >= 4) + (rnd(3) == 0);
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
        if (G.map[y][x].kind != K_FLOOR || G.map[y][x].secret || G.bulky[y][x] || vault_of_room(x, y) >= 0 || abs(x - sx) + abs(y - sy) < 6) continue;
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
            if (G.map[y][x].kind != K_FLOOR || G.map[y][x].secret || !block_corner(x, y) || lane(x, y) || by_way(x, y) || occupied(x, y, NULL)) continue;
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
    float t = (float)G.t + L->phase;
    return 0.80f + 0.12f * sinf(t * 9.1f) * sinf(t * 5.3f + 1.0f) + 0.08f * sinf(t * 23.0f + L->phase * 3.0f);
}

static void build_lightmap(void) {
    float ambient = G.shop ? 0.06f : 0.06f - 0.012f * depth();    /* the dark itself deepens, set by set */
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
    if (G.wisp_a > 0.0f && !G.shop)                                 /* and the wisp's, four cells round */
        light_dynamic_ex(G.px, G.py, 0.6f * G.wisp_a * (0.93f + 0.07f * sinf((float)G.t * 7.0f)), 4.0f, 1);
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

/* which rank, by the set's mix (PLAN-bestiary, section 4): the set's newest rung comes in on its third
 * level; each time round again the mix climbs a rung, up to black */
static int pick_rank(int type) {
    if (KINDS[type].family < 0) return RK_I;
#ifdef CRAWL_DEBUG
    if (getenv("CRAWL_RANK")) return clampi(atoi(getenv("CRAWL_RANK")), 0, NRANK - 1);
#endif
    static const int MIX[4][5] = { { 85, 15, 0, 0, 0 }, { 55, 35, 10, 0, 0 }, { 20, 40, 30, 10, 0 }, { 0, 25, 35, 30, 10 } };
    int set = crawl_set(G.level), step = crawl_step(G.level), loop = crawl_loop(G.level), share[5];
    if (type == MOB_BAT && set == 3) return RK_IV;                 /* the Deep's bats: embers, all of them */
    for (int i = 0; i < 5; i++) share[i] = MIX[set][i];
    if (step < 2 && loop == 0) { share[set] += share[set + 1]; share[set + 1] = 0; }
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
 * give way to the Wraith after the Catacombs, and bats grow rare in the Deep */
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
        if (d >= dmin && d <= dmax && G.map[y][x].kind == K_FLOOR && !occupied(x, y, NULL)
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

static void pop_readout(void) { G.readout = READOUT_T; }

#ifdef CRAWL_DEBUG
static int hits_bone, hits_bat, throws, shrieks, hits_fire, hits_sword, bites, casts[NATK], defs[NDEF], bfs_kills, dodges, wskull_wakes, spider_drops,
           grabs, hound_bites, wraith_touches, wraith_drives, leaps, knight_blows, brood_bites, ling_bites, sacs_laid,
           shade_touches, embers, ember_hits, ember_burns, ember_scorch, raised, garg_wakes;
#define DBG_COUNT(v) ((v)++, fprintf(stderr, "EVENT %s t=%.2f\n", #v, G.t))
#else
#define DBG_COUNT(v) ((void)0)
#endif

static int in_combat(void);

/* The caster takes a blow. Shadow robes may make it miss, a ward soaks it up, armor blunts it.
 * Returns what got through. (sx, sy): where it came from, for the portrait's flinch. */
static int hurt_ex(float dmg, int kind, float sx, float sy) {
    if (G.dead || G.ending || G.exiting || G.tally || G.shop || G.level_t < 1.5) return 0;
    const Gear *g = &RUN.gear;
    if (kind != HURT_POISON) {
        if (rndf() < gear_dodge(g)) { snd_sfx_ex(SFX_WHOOSH, 0.5f, 0.5f); DBG_COUNT(dodges); return 0; }   /* it passed through a shadow */
        if (kind == HURT_MAGIC) dmg *= 1.0f - gear_magic_resist(g);
        dmg -= kind == HURT_MAGIC ? gear_armor(g) * 0.5f : (float)gear_armor(g);
        if (dmg < 1.0f) dmg = 1.0f;
        if (G.ward_hp > 0.0f) {                                    /* the bone ward takes it */
            float ab = fminf(G.ward_hp, dmg);
            G.ward_hp -= ab; dmg -= ab;
            sfx_at(SFX_BONES, sx, sy, 0.5f);
            if (G.ward_hp <= 0.0f) { G.ward_t = 0.0f; snd_sfx_ex(SFX_SHATTER, 0.8f, 0.5f); }
            if (dmg < 0.5f) return 0;
        }
    }
    int d = (int)(dmg + 0.5f);
    if (d < 1) d = 1;
    G.hp -= d;
    if (kind != HURT_POISON) {
        G.hurt = 1.0f;
        G.kick = (rnd(2) ? 1.0f : -1.0f) * 0.07f;
        float lat = (sx - G.px) * -sinf(G.ang) + (sy - G.py) * cosf(G.ang);
        G.face_side = fabsf(lat) < 0.3f ? 0 : (lat < 0.0f ? -1 : 1);
        G.face_t = FACE_T;
    } else G.hurt = fmaxf(G.hurt, 0.25f);
    pop_readout();
    if (G.hp > 0) { if (kind != HURT_POISON) snd_sfx(SFX_HURT); return d; }
    G.hp = 0; G.dead = 1; G.dead_t = 0.0; G.retries++;
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT died level %d t=%.1f\n", G.level, G.level_t);
#endif
    G.hand = 0.0f; G.spd = 0.0f; G.face_t = 1e9f;
    snd_sfx(SFX_DEATH);
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
                     kind == MIS_FIRE ? 2.0f * mob_damage(m) : (kind == MIS_EMBER ? 0.8f * mob_damage(m) : mob_damage(m)), 0.0f };
        if (kind == MIS_EMBER) b->life = b->life0 = fminf(4.0f, hypotf(tx - m->x, ty - m->y) / sp);   /* lobbed to where the caster will be */
        sfx_at(kind == MIS_BONE ? SFX_THROW : SFX_CAST, m->x, m->y, kind == MIS_EMBER ? 0.7f : 1.0f);
        return;
    }
}
static void throw_bone(const Mob *m) { throw_missile(m, MIS_BONE); }

static void explode(float x, float y);
static Shot *new_shot(int kind, float x, float y, float a, float speed, float dmg);

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
                if (b->kind == MIS_FIRE) explode(b->x, b->y); else if (b->kind == MIS_EMBER) ember_land(b); else sfx_at(SFX_BONES, b->x, b->y, 0.4f);
                break;
            }
            b->x = nx; b->y = ny;
            float d = hypotf(b->x - G.px, b->y - G.py);
            if (G.mirror_t > 0.0f && d < 0.7f && !G.dead) {           /* the Mirror Veil sends it back */
                b->alive = 0;
                Shot *sh = new_shot(SHOT_REFLECT, b->x, b->y, atan2f(-b->vy, -b->vx), b->kind == MIS_FIRE ? 6.0f : 7.0f, b->dmg * 1.5f);
                if (sh) sh->z = (float)b->kind;
                snd_sfx_ex(SFX_SHIMMER, 0.35f, 0.5f);
                break;
            }
            if (!G.dead && d < 0.36f) {
                b->alive = 0;
                if (b->kind == MIS_FIRE) { hurt_ex(b->dmg, HURT_MAGIC, b->x - b->vx, b->y - b->vy); explode(b->x, b->y); DBG_COUNT(hits_fire); }
                else if (b->kind == MIS_EMBER) { hurt_ex(b->dmg, HURT_MAGIC, b->x - b->vx, b->y - b->vy); ember_land(b); DBG_COUNT(ember_hits); }
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
    if (whole > 0) { RUN.souls += whole; RUN.soul_frac -= (float)whole; G.level_souls += whole; snd_sfx_ex(SFX_GHOST_TICK, 0.5f, 0.5f); }
}

/* ---- the record and its achievements (PLAN-bestiary, section 11): the autopilot earns them too ---- */

enum { ACH_SECRET0 = REC_KINDS * NRANK, ACH_LEVEL0 = ACH_SECRET0 + 16, ACH_DESCENT0 = ACH_LEVEL0 + 20 };   /* bits: slayers by kind and rank, then the rest */
#define NSECRET_STEPS 7
static const int SECRET_STEP[NSECRET_STEPS] = { 1, 5, 10, 20, 30, 40, 50 };
static const char *const SECRET_NAME[NSECRET_STEPS] = { "A HOLLOW KNOCK", "LISTENING WALLS", "THE STONE GIVES WAY", "NOTHING STAYS HIDDEN",
                                                        "THE BUILDERS' TRICKS", "EVERY LAST CRACK", "KEEPER OF SECRETS" };
#define MAX_DESCENT 20                                              /* level 400: as far as anyone will go */
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
    if (TOASTS.n >= MAXTOASTS) return;
    Toast *t = &TOASTS.q[TOASTS.n++];
    t->spr = spr; t->family = family; t->rank = rank; t->progress = progress;
    snprintf(t->title, sizeof t->title, "%s", title); snprintf(t->desc, sizeof t->desc, "%s", desc);
    if (TOASTS.n == 1) TOASTS.t = -1.0f;                           /* its chime as it comes in */
}
static void update_toasts(float dt) {
    if (!TOASTS.n) return;
    if (TOASTS.t < 0.0f) { TOASTS.t = 0.0f; snd_sfx_ex(TOASTS.q[0].progress ? SFX_TICK : SFX_KEYGET, TOASTS.q[0].progress ? 0.6f : 0.9f, 0.5f); }
    TOASTS.t += dt;
    if (TOASTS.t >= toast_len(&TOASTS.q[0])) {
        memmove(TOASTS.q, TOASTS.q + 1, sizeof TOASTS.q[0] * (size_t)(TOASTS.n - 1));
        if (--TOASTS.n) TOASTS.t = -1.0f;
    }
}

static int ach_has(int bit) { return RUN.rec.got[bit >> 3] >> (bit & 7) & 1; }
static void ach_earn(int bit, int spr, int family, int rank, const char *title, const char *desc) {
    if (ach_has(bit)) return;
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
static void rec_seen(const Mob *m) { if (m->type < REC_KINDS) RUN.rec.seen[m->type] |= (uint8_t)(1 << m->rank); }
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
static void rec_cleared(void) {
    Record *r = &RUN.rec;
    char t[48], d[32];
    if (G.level <= 20) {
        r->cleared |= 1u << (G.level - 1);
        level_name(G.level, t, sizeof t);
        snprintf(d, sizeof d, "LEVEL %d CLEARED", G.level);
        ach_earn(ACH_LEVEL0 + G.level - 1, SP_CHALICE, -1, 0, t, d);
    }
    if (G.level % 20 == 0) {
        int k = G.level / 20;
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
    if (!KIND_IS(m, KF_UNCOUNTED)) { G.kills++; rec_kill(m); }
    sfx_at(KINDS[m->type].sfx_die, m->x, m->y, 1.0f);
    if (m->type == MOB_SPIDER && m->z > 0.3f) m->sub = 9;           /* it falls from the web */
    if (m->type == MOB_DROWNED) { add_pool(m->x, m->y); sfx_at(SFX_EXPLODE, m->x, m->y, 0.6f); }   /* it bursts */
    if (m->type == MOB_BROOD) {                                    /* her sacs die with the last of the mothers */
        int others = 0;
        for (int i = 0; i < MAXMOBS; i++) others += G.mobs[i].alive && G.mobs[i].state == MS_WALK && G.mobs[i].type == MOB_BROOD && &G.mobs[i] != m;
        if (!others) for (int i = 0; i < MAXMOBS; i++) if (G.mobs[i].alive && G.mobs[i].type == MOB_SAC && G.mobs[i].state == MS_WALK) { G.mobs[i].state = MS_DIE; G.mobs[i].t = 0.0f; }
    }
    gain_souls((float)RANK_SOULS[m->rank]);
    /* what it leaves: the gilded always something good, the higher ranks now and then coin or a potion.
     * Like a closet's loot it isn't counted in the level's treasure, and it glints a little */
    int drop = -1;
    if (m->rank == RK_GILDED) drop = rnd(3) ? IT_CROWN : IT_CHEST;
    else if (m->rank >= RK_IV && rnd(10) == 0) drop = IT_POTION;
    else if (m->rank >= RK_II && rndf() < 0.08f * (float)m->rank) drop = m->rank >= RK_IV ? IT_GEMS : IT_COINS;
    if (drop >= 0 && G.map[m->cy][m->cx].kind == K_FLOOR) {
        Item *it = add_item(m->cx, m->cy, drop);
        if (it) {
            it->secret = 1; it->seen = 1;
            if (drop < IT_KEY) { G.items_total--; G.value_total -= treasure_value(drop); }
        }
    }
}

/* the knight's plate comes off as it is hurt: 0 whole, 1 half, 2 down to the breastplate */
static int knight_stage(const Mob *m) { return m->hp > m->maxhp * 0.66f ? 0 : (m->hp > m->maxhp * 0.33f ? 1 : 2); }

/* How much a spell bites a creature: its weakness to the spell, and each element worn added in by how
 * much that element hurts it (fire as Fire Bolt does, frost as Frost Shard, lightning as Chain
 * Lightning). A knight down to its breastplate has nothing left to keep fire off. */
static float weak_to(const Mob *m, int atk) {
    if (m->type == MOB_KNIGHT && atk == AT_FIRE && knight_stage(m) == 2) return 1.0f;
    return KINDS[m->type].weak[atk];
}
static float hit_mult(const Mob *m, int atk) {
    const Gear *g = &RUN.gear;
    float k = weak_to(m, atk);
    if (atk == AT_BFS) return k;
    return k + gear_total(g, ST_FIRE) / 100.0f * weak_to(m, AT_FIRE) + gear_total(g, ST_FROST) / 100.0f * weak_to(m, AT_FROST)
             + gear_total(g, ST_SHOCK) / 100.0f * weak_to(m, AT_CHAIN);
}
static float slow_factor(void) {                                   /* slowed, the caster walks at 60%; resisted, less slowly */
    return G.slow_t > 0.0f ? 1.0f - 0.4f * (1.0f - 0.5f * gear_slow_resist(&RUN.gear)) : 1.0f;
}
static float spell_effect(void) { return 1.0f + gear_total(&RUN.gear, ST_EFFECT) / 100.0f; }   /* all a spell does besides damage */

/* A hit on a creature, from (fx, fy) (shields only stop what comes from in front). Returns 1 if it died. */
static int damage_mob(Mob *m, float dmg, int flags, float fx, float fy, float slow) {
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
    if (m->type == MOB_GHOUL && m->sub == 4) { dmg *= 2.0f; m->sub = 0; }   /* caught feeding */
    if (m->type == MOB_GARGOYLE) garg_wake(m);                     /* it was never a statue */
    if (m->type == MOB_HUNTER && m->sub == 0) { m->sub = 1; m->sub_t = 0.0f; }   /* hit, it drops from the ceiling */
    m->misses = 0;
    m->alert = fmaxf(m->alert, 12.0f);                             /* hurt, it knows where you are */
    if (m->rank == RK_GILDED && m->hp > m->maxhp * 0.5f && m->hp - dmg <= m->maxhp * 0.5f) m->feared = 6.0f;   /* the gilded run, with their gold */
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
    return c->kind == K_DOOR && G.doors[c->door].locked != 1;
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
    if (G.wisp_a > 0.5f && pd < 3.0f && m->sub != 1) wraith_back(m);   /* the wisp's light, held up close */
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
    float dax = G.px - m->ax, day = G.py - m->ay, da = sqrtf(dax * dax + day * day) + 1e-4f;
    float reach = fminf(da - 0.45f, 0.75f);                         /* how far out from the corner it drops */
    switch (m->sub) {
    case 0:
        m->x = m->ax; m->y = m->ay; m->z = 0.62f;
        if (!G.dead && m->cool <= 0.0f && da < 2.0f && line_clear(m->ax, m->ay, G.px, G.py)) { m->sub = 1; m->sub_t = 0.0f; sfx_at(SFX_SKITTER, m->x, m->y, 1.0f); DBG_COUNT(spider_drops); }
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
    for (int k = 0; k < 3; k++) {                                   /* it hatches */
        Mob *l = spawn_mob(m->cx, m->cy, MOB_LING);
        if (!l) break;
        l->x = m->x + (rndf() - 0.5f) * 0.6f; l->y = m->y + (rndf() - 0.5f) * 0.6f; l->alert = 30.0f;
        set_rank(l, m->rank < RK_V ? m->rank : RK_V);              /* their mother's colour */
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

static void update_mob(Mob *m, float dt) {
    if (m->state == MS_DIE && m->sub == 77) { if ((m->t += dt) >= 0.9f) m->alive = 0; return; }   /* gone into the BFS9000 */
    if (m->type == MOB_GHOST) { update_ghost(m, dt); return; }
    if (m->type == MOB_WSKULL) { update_wskull(m, dt); return; }
    if (m->type == MOB_SPIDER) { update_spider(m, dt); return; }
    if (m->type == MOB_WRAITH) { update_wraith(m, dt); return; }
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
    if (m->feared > 0.0f) m->feared -= dt;
    if (m->stagger > 0.0f) { m->stagger -= dt; return; }
    int keeps_off = m->type == MOB_IMP || m->type == MOB_DEACON;   /* they fight from further off, and back away */
    int afraid = (undead && m->feared > 0.0f) || (keeps_off && sees && pd0 < 2.8f);
    if (m->type == MOB_SKEL && !G.dead && !afraid) {               /* once it has seen you, it keeps throwing */
        int los = sees && G.level_t > 1.5;
        if (los && !m->seen) { m->seen = 1; m->cool = (0.45f + 0.35f * rndf()) * throw_interval() / 1.5f; }
        if (m->throw_t > 0.0f) {
            m->throw_t -= dt;
            if (m->throw_t <= 0.0f) { throw_bone(m); m->cool = throw_interval() * (1.0f + (rndf() - 0.5f) * 0.27f); DBG_COUNT(throws); }
            return;                                                /* stands still to throw */
        }
        if (m->seen && los && (m->cool -= dt) <= 0.0f) { m->throw_t = 0.35f; return; }
    }
    if (m->type == MOB_SHIELD && !G.dead && !afraid) {             /* closes in behind its shield and strikes */
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
    if ((m->type == MOB_SHAMBLER || m->type == MOB_DROWNED) && !G.dead && !afraid) {   /* it lurches in and takes hold */
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
    if (m->type == MOB_HOUND && !G.dead && !afraid) {               /* in, a bite, and away */
        m->cool -= dt;
        if (m->swing > 0.0f) {
            m->swing -= dt;
            if (m->swing <= 0.0f) {
                m->swing = 0.0f; m->cool = 1.3f; m->feared = 0.8f;
                sfx_at(SFX_BONES, m->x, m->y, 0.7f);
                if (pd0 < 1.3f && hurt_ex(mob_damage(m) * 0.8f, HURT_BLOW, m->x, m->y) > 0) { thorns(m); DBG_COUNT(hound_bites); }
            }
            return;
        }
        if (sees && pd0 < 1.15f && m->cool <= 0.0f) { m->swing = 0.25f; return; }
    }
    if (m->type == MOB_HUNTER && !G.dead) {                        /* along the ceiling, down a thread, and a leap */
        if (update_leap(m, dt)) return;
        m->cool -= dt;
        if (m->sub == 0) {                                         /* overhead, upside down */
            m->z = 1.0f - KINDS[MOB_HUNTER].scale * RANK_SIZE[m->rank];
            if (sees && pd0 < 2.6f && G.level_t > 1.5) { m->sub = 1; m->sub_t = 0.0f; sfx_at(SFX_SKITTER, m->x, m->y, 1.3f); }
        } else if (m->sub == 1) {                                  /* it drops */
            m->sub_t += dt;
            m->z = lerpf(1.0f - KINDS[MOB_HUNTER].scale, 0.0f, fminf(1.0f, m->sub_t / 0.4f));
            if (m->sub_t >= 0.4f) { m->sub = 2; m->z = 0.0f; m->alert = 12.0f; }
            return;
        } else if (sees && pd0 < 2.4f && pd0 > 0.9f && m->cool <= 0.0f) { start_leap(m); return; }
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
        if (!afraid && sees && pd0 < 2.6f && pd0 > 0.9f && m->cool <= 0.0f) { start_leap(m); return; }
        if (m->alert <= 0.0f && m->hp < m->maxhp && (m->contact -= dt) <= 0.0f) {   /* bones near? it stops to eat */
            m->contact = 2.0f;
            for (int i = 0; i < G.ndecos; i++) {
                int sp = G.decos[i].spr;
                if ((sp == SP_BONES1 || sp == SP_BONES2 || sp == SP_SKEL_D3 || sp == SP_SKULLPILE || sp == SP_CARCASS)
                    && fabsf(G.decos[i].x - m->x) < 1.2f && fabsf(G.decos[i].y - m->y) < 1.2f) { m->sub = 4; m->sub_t = 4.0f; break; }
            }
        }
    }
    if (m->type == MOB_KNIGHT && !G.dead && !afraid) {              /* a long wind-up, and a blow that tells */
        int st = knight_stage(m);
        if (st != m->sub) { m->sub = st; sfx_at(SFX_SHATTER, m->x, m->y, 1.1f); m->speed = KINDS[MOB_KNIGHT].speed * RANK_SPEED[m->rank] * (1.0f + 0.25f * st); }
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
        if (sees && pd0 < 1.4f && m->cool <= 0.0f) {
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
        if (pd0 < 0.8f && m->cool <= 0.0f) {
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
        if (pd0 < 0.95f && m->cool <= 0.0f) { m->swing = 0.5f; sfx_at(SFX_GHOST_TICK, m->x, m->y, 0.8f); return; }
        if (pd0 < 6.0f && shade_hidden(m) && rnd(100) == 0) sfx_at(SFX_STEP, m->x, m->y, 0.45f);   /* footsteps, and no one there */
    }
    if (m->type == MOB_IMP && !G.dead) {                           /* it keeps off, and lobs embers */
        if (m->throw_t > 0.0f) {
            if ((m->throw_t -= dt) <= 0.0f) { throw_missile(m, MIS_EMBER); m->cool = 2.4f + rndf(); DBG_COUNT(embers); }
            return;
        }
        m->cool -= dt;
        if (sees && pd0 > 1.5f && pd0 < 8.0f && m->cool <= 0.0f && G.level_t > 1.5) { m->throw_t = 0.4f; return; }
    }
    if (m->type == MOB_DEACON && !G.dead) {                        /* it keeps back, and calls the fallen up again */
        if (m->swing > 0.0f) {                                     /* chanting over the bones */
            if ((m->swing -= dt) > 0.0f) return;
            m->swing = 0.0f; m->contact = 6.0f;
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
                    sfx_at(SFX_BONES, r->x, r->y, 1.1f);
                    DBG_COUNT(raised);
                }
            }
            return;
        }
        if (m->alert > 0.0f && m->raised < 4 && (m->contact -= dt) <= 0.0f) {
            m->contact = 1.0f;                                     /* it looks again in a moment */
            for (int i = 0; i < G.ndecos; i++) {
                const Deco *dc = &G.decos[i];
                if (dc->spr != SP_SKEL_D3 && dc->spr != SP_BONES2) continue;
                if (hypotf(dc->x - m->x, dc->y - m->y) > 6.0f || !line_clear(m->x, m->y, dc->x, dc->y) || occupied((int)dc->x, (int)dc->y, NULL)) continue;
                m->swing = DEACON_CHANT; m->deco = i; m->ax = dc->x; m->ay = dc->y;
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
        if (!G.dead && sees && pd0 < 1.2f && m->cool <= 0.0f) { m->swing = 0.3f; return; }
        if (!G.dead && sees && pd0 > 1.4f && pd0 < 3.2f && m->cool <= 0.0f) { start_leap(m); return; }
    }
    if (m->type == MOB_BAT && !G.dead) {                           /* bats come for your face */
        int los = pd0 < 5.0f && sees;
        if (los) m->attacking = 1;
        if (m->attacking && pd0 > 7.0f) { m->attacking = 0; m->tx = m->cx; m->ty = m->cy; }
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
    int chase = pd < (KIND_IS(m, KF_MELEE) ? 9.0f : 7.0f) && pd > keep && line_clear(m->x, m->y, G.px, G.py);
    int here = hunt[m->cy][m->cx], opens = KIND_IS(m, KF_DOORS) != 0;   /* the dead open doors; bats and hounds wait behind them */
    int hunting = !afraid && !chase && !G.dead && m->alert > 0.0f && m->hunts && here > 0 && pd > keep;
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
        if (!chase && !afraid && m->home >= 0 && c->region != m->home) continue;   /* keeps to its own room till it knows */
        float s = rndf();
        if (afraid) s += hypotf(nx + 0.5f - G.px, ny + 0.5f - G.py) * 2.0f;
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
    } else if (best >= 0 && (afraid || !(pd <= keep))) { m->tx = m->cx + DX[best]; m->ty = m->cy + DY[best]; }
    if (m->type == MOB_BAT && pd < 6.0f && rnd(40) == 0) sfx_at(SFX_BAT, m->x, m->y, 0.6f);
}

/* ---- the caster's spells ---- */

static int wisp_field[MAPN][MAPN];                                  /* steps to the wisps' target, for going round corners */
static void wisp_bfs(int tx, int ty) {
    static int qx[MAPN * MAPN], qy[MAPN * MAPN];
    for (int y = 0; y < MAPN; y++) for (int x = 0; x < MAPN; x++) wisp_field[y][x] = -1;
    int qh = 0, qt = 0;
    wisp_field[ty][tx] = 0; qx[qt] = tx; qy[qt] = ty; qt++;
    while (qh < qt) {
        int x = qx[qh], y = qy[qh]; qh++;
        if (wisp_field[y][x] > 14) continue;
        for (int d = 0; d < 4; d++) {
            int nx = x + DX[d], ny = y + DY[d];
            if (blocks_light(nx, ny) || cell(nx, ny)->kind == K_PILLAR || wisp_field[ny][nx] >= 0) continue;
            wisp_field[ny][nx] = wisp_field[y][x] + 1; qx[qt] = nx; qy[qt] = ny; qt++;
        }
    }
}

static Shot *new_shot(int kind, float x, float y, float a, float speed, float dmg) {
    for (int i = 0; i < MAXSHOTS; i++) {
        Shot *s = &G.shots[i];
        if (s->alive) continue;
        *s = (Shot){ 1, kind, -1, x, y, 0.0f, cosf(a) * speed, sinf(a) * speed, kind == AT_SPEAR ? 1.0f : 1.6f, dmg, 0 };
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
static void chain_lightning(int t, float dmg) {
    uint32_t hit = 0;
    Mob *m = &G.mobs[t];
    float hx = G.px + cosf(G.ang) * 0.35f, hy = G.py + sinf(G.ang) * 0.35f;
    add_beam(AT_CHAIN, hx, hy, 0.42f, m->x, m->y, m->z + 0.5f);
    hit |= 1u << t;
    damage_mob(m, dmg, DM_ARC | DM_ATK(AT_CHAIN), G.px, G.py, 0.0f);
    int tier = spell_tier(AT_CHAIN), hops = 2 + (tier >= 4) + (tier >= 7);
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
        if (!m->alive || m->state != MS_WALK || KIND_IS(m, KF_SPIRIT)) continue;
        if (hypotf(m->x - G.px, m->y - G.py) > 14.0f || !line_clear(G.px, G.py, m->x, m->y)) continue;
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
static void cast_spell(int sp, int t) {
    const Gear *g = &RUN.gear;
    Mob *m = t >= 0 ? &G.mobs[t] : &G.mobs[0];
    int have = t >= 0 && m->alive && m->state == MS_WALK;
    float tx = have ? m->x : G.px + cosf(G.ang) * 4, ty = have ? m->y : G.py + sinf(G.ang) * 4;
    float a = atan2f(ty - G.py, tx - G.px), dmg = spell_damage(g, sp);
    if (G.player && sp != AT_CHAIN && sp != AT_DRAIN) a = G.ang;   /* a person's shots go where they look */
    float hx = G.px + cosf(a) * 0.35f, hy = G.py + sinf(a) * 0.35f;
    int bat = have && m->type == MOB_BAT && !G.player;
    alert_near(8, 8.0f);                                           /* a spell is heard down the halls */
#ifdef CRAWL_DEBUG
    casts[sp]++;
    fprintf(stderr, "EVENT cast_%s t=%.2f\n", ATK_NAME[sp], G.t);
#endif
    switch (sp) {
    case AT_CHAIN: if (have) chain_lightning(t, dmg); else fizzle(sp); return;
    case AT_DRAIN:
        if (have) {
            add_beam(AT_DRAIN, hx, hy, 0.42f, m->x, m->y, m->z + 0.5f);
            float before = m->hp;
            damage_mob(m, dmg, DM_ARC | DM_ATK(AT_DRAIN), G.px, G.py, 0.0f);
            float dealt = before - fmaxf(m->hp, 0.0f);
            G.hp = clampi(G.hp + (int)(dealt * fminf(0.8f, 0.5f + 0.05f * spell_tier(AT_DRAIN)) * spell_effect() + 0.5f), 0, G.max_hp);
            pop_readout();
            snd_sfx_ex(SFX_WHISPER, 0.8f, 0.5f);
        } else fizzle(sp);
        return;
    case AT_BFS: bfs9000(); return;
    case AT_WISPS: {
        int n = 3 + (spell_tier(AT_WISPS) >= 3) + (spell_tier(AT_WISPS) >= 6);
        if (have) wisp_bfs(m->cx, m->cy);
        for (int i = 0; i < n; i++) {
            Shot *s = new_shot(AT_WISPS, hx, hy, a + (i - (n - 1) * 0.5f) * 0.45f, 4.5f, dmg);
            if (s) { s->target = have ? t : -1; s->life = 3.0f; }
        }
        snd_sfx_ex(SFX_SHIMMER, 0.6f, 0.5f);
        return;
    }
    default: {
        float speed = sp == AT_SPEAR ? 12.0f : (sp == AT_SPARKS ? 11.0f : (sp == AT_HELL ? 6.0f : (sp == AT_FROST ? 9.0f : 8.0f)));
        if (bat) a += (rndf() - 0.5f) * (sp == AT_SPARKS || sp == AT_FROST ? 0.3f : 0.6f);   /* bats are hard to hit */
        new_shot(sp, hx, hy, a, speed, dmg);
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
        int n = 1 + (int)(sqrtf(s->vx * s->vx + s->vy * s->vy) * dt / 0.08f);
        for (int k = 0; k < n && s->alive; k++) {
            float nx = s->x + s->vx * dt / n, ny = s->y + s->vy * dt / n;
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
            for (int mi = 0; mi < MAXMOBS; mi++) {
                Mob *mb = &G.mobs[mi];
                if (!mb->alive || mb->state != MS_WALK || mb->type == MOB_GHOST) continue;   /* fire passes through wraiths */
                if (s->kind == AT_SPEAR && (s->pierced >> mi & 1)) continue;
                float hr = KINDS[mb->type].rad;
                if (hypotf(mb->x - s->x, mb->y - s->y) >= hr) continue;
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
                        if (!G.burns[b].alive) { G.burns[b] = (Burn){ 1, s->x, s->y, 0.0f, (4.0f + spell_tier(AT_HELL)) * spell_effect(), 0 }; break; }
                    break;
                case AT_FIRE:
                    s->alive = 0; explode(s->x, s->y);
                    damage_mob(mb, s->dmg, DM_ATK(AT_FIRE), fx, fy, 0.0f);
                    if (spell_tier(AT_FIRE) >= 2) splash(s->x, s->y, 0.75f, s->dmg * 0.5f, mb, AT_FIRE);
                    break;
                case AT_FROST:
                    s->alive = 0; sfx_at(SFX_FROST, s->x, s->y, 0.8f);
                    damage_mob(mb, s->dmg, DM_SLOW | DM_ATK(AT_FROST), fx, fy, (2.5f + 0.25f * spell_tier(AT_FROST)) * spell_effect());
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

/* Blink: two or three steps to wherever is furthest from trouble, out of sight if it can */
static void blink(void) {
    static int dist[MAPN][MAPN], qx[MAPN * MAPN], qy[MAPN * MAPN];
    int range = 2 + RUN.gear.def[DF_BLINK] / 3;
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
    int t = RUN.gear.def[k];
    if (t <= 0) return;
    G.def_cd[k] = def_cooldown(t);
    G.rune_glow[k] = 1.4f; G.rune_lift = 1.4f;
    snd_sfx_ex(SFX_RUNE, 0.9f, 0.5f);
#ifdef CRAWL_DEBUG
    defs[k]++;
    fprintf(stderr, "EVENT defense %s t=%.2f hp=%d\n", DEF_NAME[k], G.t, G.hp);
#endif
    float eff = spell_effect();                                    /* the runes' strength */
    switch (k) {
    case DF_WARD: G.ward_hp = (10.0f + 5.0f * t) * eff; G.ward_t = 12.0f; break;
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
            if (m->alive && m->state == MS_WALK && (KIND_IS(m, KF_UNDEAD) || m->type == MOB_WSKULL || m->type == MOB_WRAITH)
                && hypotf(m->x - G.px, m->y - G.py) < 5.0f) m->feared = (3.0f + 0.5f * t) * eff;
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
            if (G.def_wait[k] <= 0.0f && G.def_cd[k] <= 0.0f && !G.dead) cast_defense(k);
            continue;
        }
        if (G.def_cd[k] > 0.0f || G.def_hush[k] > 0.0f || G.dead) continue;
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
    if (G.ward_t > 0.0f && (G.ward_t -= dt) <= 0.0f) G.ward_hp = 0.0f;
    G.mirror_t = fmaxf(0.0f, G.mirror_t - dt);
    G.slip_t = fmaxf(0.0f, G.slip_t - dt);
    G.reap_t = fmaxf(0.0f, G.reap_t - dt);
    for (int k = 0; k < NDEF; k++) G.rune_glow[k] = fmaxf(0.0f, G.rune_glow[k] - dt);
    G.rune_lift = fmaxf(0.0f, G.rune_lift - dt);
    if (G.nova_t > 0.0f && (G.nova_t += dt) > 0.6f) G.nova_t = 0.0f;
    if (G.dread_t > 0.0f && (G.dread_t += dt) > 0.8f) G.dread_t = 0.0f;
    float heal = 0.0f;
    if (G.mend_t > 0.0f) { G.mend_t -= dt; heal += G.mend_rate * dt; }
    heal += gear_regen(&RUN.gear) * dt;
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
/* the wisp's map: how many steps from each cell to (gx,gy), doors locked or not */
static void build_route(int gx, int gy) {
    static int q[MAPN * MAPN];
    memset(G.route, 0xff, sizeof G.route);
    G.route_gx = gx; G.route_gy = gy;
    if (gx < 0) return;
    int qh = 0, qt = 0;
    G.route[gy][gx] = 0; q[qt++] = gy * MAPN + gx;
    while (qh < qt) {
        int x = q[qh] % MAPN, y = q[qh] / MAPN; qh++;
        for (int d = 0; d < 4; d++) {
            int nx = x + DX[d], ny = y + DY[d];
            if (nx < 0 || ny < 0 || nx >= MAPN || ny >= MAPN || G.route[ny][nx] >= 0 || !is_open(nx, ny)) continue;
            G.route[ny][nx] = (int16_t)(G.route[y][x] + 1); q[qt++] = ny * MAPN + nx;
        }
    }
}

static void open_secret(int i) {
    Secret *sc = &G.secrets[i];
    if (sc->state || G.slide >= 0) return;
    sc->state = 1; G.slide = i; G.slide_t = 0.0f;
    G.map[sc->sy][sc->sx].kind = K_FLOOR;                          /* the block is drawn by itself while it moves */
    int nl = G.nlights;
    light_static(sc->sx + 0.5f + DX[sc->d] * 1.5f, sc->sy + 0.5f + DY[sc->d] * 1.5f, 0.75f, 3.4f);   /* the hoard's own glow */
    if (G.nlights > nl) G.lights[nl].phase = -1000.0f;             /* steady, not a flame */
    G.secrets_found++;
    rec_secret();
    say("A SECRET!   %d OF %d", G.secrets_found, G.nsecrets);
    sfx_at(SFX_PUSHWALL, sc->sx + 0.5f, sc->sy + 0.5f, 1.0f);
    snd_sfx_ex(SFX_SECRET, 1.0f, 0.5f);
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT secret %d of %d t=%.2f by %s\n", G.secrets_found, G.nsecrets, G.t, G.player ? "player" : "autopilot");
#endif
}
static void update_slide(float dt) {
    if (G.slide < 0) return;
    Secret *sc = &G.secrets[G.slide];
    G.slide_t += dt / 0.78f;
    if (G.slide_t < SLIDE_CELLS) return;
    int ex = sc->sx + DX[sc->d] * SLIDE_CELLS, ey = sc->sy + DY[sc->d] * SLIDE_CELLS;   /* it comes to rest at the back */
    G.map[ey][ex].kind = K_WALL; G.map[ey][ex].tex = (uint8_t)sc->tex;
    for (int k = 0; k <= SLIDE_CELLS; k++)
        for (int side = -1; side <= 1; side++) {
            int x = sc->sx + DX[sc->d] * k + DY[sc->d] * side, y = sc->sy + DY[sc->d] * k + DX[sc->d] * side;
            if (x >= 0 && y >= 0 && x < MAPN && y < MAPN) G.map[y][x].secret = 0;
        }
    sc->state = 2; G.slide = -1;
    G.route_gx = -1;                                               /* the closet is part of the crypt now: map the ways again */
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
    if (it->type == IT_POTION) return G.hp < G.max_hp * 4 / 5;      /* saved for when it is needed */
    return 1;
}

/* something worth walking to from (x,y): a vault door once its key is held, else treasure it has seen */
static int pick_goal(int x, int y, int *gx, int *gy) {
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
                if (it->type == IT_KEY) { if (d < 0 || d > 22 || !key_calls(it)) continue; d -= 8; }   /* a vault's worth: worth the walk */
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
static int decide(int x, int y, int h) {
    int gx, gy;
    if (pick_goal(x, y, &gx, &gy)) {
        bfs_from(gx, gy);
        int gbest = -1, gs = 1 << 30;
        for (int d = 0; d < 4; d++) {
            int nx = x + DX[d], ny = y + DY[d];
            if (!apass(nx, ny) || bfs_dist[ny][nx] < 0) continue;
            int sc = bfs_dist[ny][nx] * 4 + (d == h ? 0 : 1) + (d == ((h + 2) & 3) ? 2 : 0) + (G.bulky[ny][nx] ? 5 : 0);
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
    if (G.leaving && !G.exiting && G.cx == G.exit_fx && G.cy == G.exit_fy) {   /* the way out: face it, open it, go */
        G.exiting = 1; G.exit_t = 0.0f; G.u = 1.0f;
        begin_turn(heading_angle(G.exit_dir)); G.adur = 0.4f;
        G.state = PS_EXIT; G.spd = 0.0f; G.hand = 0.0f;
        return 1;
    }
    if (G.map[G.cy][G.cx].visits < 250) {
        if (G.map[G.cy][G.cx].visits == 0) G.explored++;
        G.map[G.cy][G.cx].visits++;
    }
    for (int y = G.cy - 1; y <= G.cy + 1; y++)                     /* the lanes beside it are seen in passing */
        for (int x = G.cx - 1; x <= G.cx + 1; x++)
            if (apass(x, y) && G.map[y][x].visits == 0) { G.map[y][x].visits = 1; G.explored++; }
    for (int i = 0; i < G.nsecrets && G.seek_secret < 0 && !G.leaving; i++) {   /* the cracks catch its eye, a third of the time */
        Secret *sc = &G.secrets[i];
        if (sc->state || sc->rolled || abs(G.cx - sc->fx) + abs(G.cy - sc->fy) > 1) continue;
        sc->rolled = 1;
        if (rndf() < 0.35f) G.seek_secret = i;
    }
    if (G.seek_secret >= 0) {                                      /* one step across to stand before it */
        const Secret *sc = &G.secrets[G.seek_secret];
        for (int d = 0; d < 4; d++) if (G.cx + DX[d] == sc->fx && G.cy + DY[d] == sc->fy) G.next_h = d;
    }
    return 0;
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
                 || (m->type == MOB_WRAITH && m->sub == 0 && dist < 7.0f) || (KIND_IS(m, KF_MELEE) && m->alert > 0.0f && dist < 4.0f)) score = dist + 1.5f;
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
    if (m->type == MOB_WRAITH) return g->atk[AT_HELL] >= 0 ? AT_HELL : AT_FIRE;   /* only fire turns it */
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
    for (int sp = 0; sp < AT_BFS; sp++) {
        if (g->atk[sp] < 0) continue;
        owned[no++] = sp;
        float v = spell_damage(g, sp) / spell_cycle(g, sp);
        if (sp == AT_WISPS) v *= (float)(3 + (g->atk[sp] >= 3) + (g->atk[sp] >= 6));
        if (m->type == MOB_WSKULL) v = 1.0f / spell_cycle(g, sp);   /* any hit will do: the quickest */
        switch (sp) {
        case AT_SPEAR: if (shielded) v *= 2.2f; if (near) v *= 1.3f; break;
        case AT_CHAIN: v *= 1.0f + 0.6f * (float)(near < 3 ? near : 3); if (shielded) v *= 1.8f; break;
        case AT_HELL: v *= 1.0f + 0.8f * packed; break;
        case AT_DRAIN: if (hpf < 0.45f) v *= 2.2f; break;
        case AT_FROST: if (m->type == MOB_BAT || m->type == MOB_SPIDER || m->type == MOB_SHIELD) v *= 1.2f; break;   /* slowing what closes in */
        default: break;
        }
        if (m->type != MOB_WSKULL) v *= hit_mult(m, sp);           /* what it is weak to, the elements worn too */
        if (sp == AT_WISPS) v *= 0.75f;                            /* not every wisp gets there */
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

static void update_player(float dt) {
    G.cooldown -= dt;
    G.calm += dt;
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
            if (G.cooldown <= 0.0f) { aim_at(blocker); break; }
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
            if (arrive() || at_secret()) break;
            if (G.next_h != G.h) {
                begin_turn(heading_angle(G.next_h));
                G.state = PS_TURN;
            } else start_move();
        }
        if (G.cooldown <= 0.0f && G.state == PS_MOVE) {
            int t = find_target();
            if (t >= 0) aim_at(t);
        }
        break;
    }
    case PS_TURN: {
        G.st += dt;
        if (G.u < 1.0f) {                                         /* still finishing the step into the corner */
            advance(fmaxf(G.spd, CRUISE * 0.4f) * dt);
            if (G.u >= 1.0f && (arrive() || at_secret())) break;
        } else G.spd *= 0.85f;
        float k = smoothstepf(0.0f, 1.0f, G.st / G.adur);
        G.ang = lerpf(G.ang0, G.ang1, k);
        if (G.st >= G.adur) {
            if (G.u < 1.0f) { advance(1.0f - G.u); if (arrive() || at_secret()) break; }
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
    G.ward_hp = G.ward_t = G.mirror_t = G.slip_t = G.reap_t = G.mend_t = G.mend_rate = 0.0f;
    memset(G.def_cd, 0, sizeof G.def_cd); memset(G.def_wait, 0, sizeof G.def_wait);
    memset(G.def_hush, 0, sizeof G.def_hush); memset(G.rune_glow, 0, sizeof G.rune_glow);
    G.bfs_t = 0.0f; G.face_t = G.grin_t = 0.0f;
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
                if (c->secret) ch = c->secret == 2 ? '?' : ':';
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

static void start_level(void) {
    memset(G.has_key, 0, sizeof G.has_key);
    G.shop = 0;
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
    place_exit(G.cx, G.cy);
    G.route_gx = -1;
    G.level_treasure = 0; G.level_souls = 0; G.dead = 0; G.dead_t = 0.0;
    G.wisp_on = 0; G.wisp_a = 0.0f;                                /* the wisp is earned afresh on every level */
#ifdef CRAWL_DEBUG
    if (getenv("CRAWL_WISP")) G.level_souls = WISP_SOULS;
#endif
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
    hunt_cx = -1;
    if (G.level >= 7 && rnd(3) == 0) {                             /* now and then one of them is gilded, and carries treasure */
        int pick[MAXMOBS], np = 0;
        for (int i = 0; i < MAXMOBS; i++) {
            const Mob *m = &G.mobs[i];
            if (m->alive && !m->guard && KINDS[m->type].gild) pick[np++] = i;
        }
        if (np) set_rank(&G.mobs[pick[rnd(np)]], RK_GILDED);
    }
#ifdef CRAWL_DEBUG
    { int nt[NKINDS] = { 0 }, nr[NRANK] = { 0 }; for (int i = 0; i < MAXMOBS; i++) if (G.mobs[i].alive) { nt[G.mobs[i].type]++; nr[G.mobs[i].rank]++; }
      fprintf(stderr, "LEVEL %d mobs:", G.level);
      for (int k = 0; k < NKINDS; k++) if (nt[k]) fprintf(stderr, " %s %d", KINDS[k].name, nt[k]);
      fprintf(stderr, "; ranks %d %d %d %d %d gilded %d\n", nr[0], nr[1], nr[2], nr[3], nr[4], nr[5]); }
#endif
    G.level_t = 0.0; G.card = 0.0f; G.ending = 0;
    G.readout = READOUT_T + 1.5f;                                  /* show where things stand as the level opens */
    if (G.player) human_begin();
#ifdef CRAWL_DEBUG
    if (getenv("CRAWL_MAP")) debug_map(0);
#endif
}

#ifdef CRAWL_DEBUG
/* for testing: CRAWL_GEAR=n gives every spell and rune at tier n, and an Uncommon piece of level 2n
 * in every slot (about what tier n used to wear); CRAWL_GOLD, CRAWL_SOULS */
static void debug_gear(void) {
    if (getenv("CRAWL_GEAR")) {
        int t = atoi(getenv("CRAWL_GEAR"));
        uint32_t rs = 0x9e3779b9u ^ (uint32_t)t;
        static const int KIND_OF[NSLOT] = { IK_WRIST, IK_WRIST, IK_BOOTS, IK_NECK, IK_HOOD, IK_ROBE, IK_RING };
        for (int i = 0; i < NATK; i++) RUN.gear.atk[i] = (int8_t)t;
        for (int i = 0; i < NDEF; i++) RUN.gear.def[i] = (int8_t)t;
        for (int s = 0; s < NSLOT && t > 0; s++) RUN.gear.worn[s] = piece_roll(KIND_OF[s], 2 * t, Q_UNCOMMON, &rs);
    }
    if (getenv("CRAWL_BAG")) {                                     /* a bag of that quality, all but one place filled */
        int q = clampi(atoi(getenv("CRAWL_BAG")), 0, NQUAL - 1);
        uint32_t rs = 0x85ebca6bu ^ (uint32_t)q;
        RUN.gear.bag_size = (uint8_t)bag_capacity(q); RUN.gear.bag_quality = (uint8_t)q;
        for (int i = 0; i + 1 < RUN.gear.bag_size; i++) RUN.gear.bag[i] = piece_roll(IK_WRIST + (int)(rs % 6), 3 + i * 2, i % NQUAL, &rs);
    }
    if (getenv("CRAWL_GOLD")) RUN.treasure = atoi(getenv("CRAWL_GOLD"));
    if (getenv("CRAWL_SOULS")) RUN.souls = atoi(getenv("CRAWL_SOULS"));
}
#endif

/* A piece comes to the caster, bought or found: worn if it beats what is on, otherwise into the bag;
 * what has nowhere to go is lost. A caption says which. (A person will choose at the merchant.) */
static void take_piece(const Offer *o) {
    const Piece *p = &o->piece;
    char a[48], b[48];
    piece_name(p, a, b, sizeof a);
    int slot = piece_slot(&RUN.gear, p->kind);
    const char *was = slot >= 0 ? KIND_NAME[RUN.gear.worn[slot].kind] : "";
    int old = G.max_hp;
    switch (gear_take(&RUN.gear, o)) {
    case TAKE_WORN: say("NOW WEARING %s", b); break;
    case TAKE_WORN_OLD_BAGGED: say("%s ON; THE OLD %s IN THE BAG", KIND_NAME[p->kind], was); break;
    case TAKE_WORN_OLD_LOST: say("%s ON; THE OLD %s LOST", KIND_NAME[p->kind], was); break;
    case TAKE_BAGGED: say("INTO THE BAG: %s", b); break;
    case TAKE_LOST: say("NO ROOM FOR IT: %s LEFT", KIND_NAME[p->kind]); break;
    case TAKE_BAG: say("A %s: ROOM FOR %d", bag_name(RUN.gear.bag_size), RUN.gear.bag_size); break;
    default: break;
    }
    G.max_hp = gear_max_hp(&RUN.gear);
    if (G.max_hp > old) G.hp += G.max_hp - old;                    /* new strength comes with it */
    if (G.hp > G.max_hp) G.hp = G.max_hp;
}

/* The run carries on across scene switches: the level, HP, gold, souls and gear are kept, and the
 * current level is laid out afresh (without a second visit to the merchant). */
static void crawl_enter(uint32_t seed) {
    crawl_art_init();
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
    switch (it->type) {
    case IT_KEY:
        G.has_key[it->color] = 1; snd_sfx(SFX_KEYGET); G.gold = 1.0f; G.grin_t = 1.6f;
#ifdef CRAWL_DEBUG
        bfs_from(G.doors[G.vault_door[it->color]].x, G.doors[G.vault_door[it->color]].y);
        fprintf(stderr, "EVENT key%d t=%.2f path=%d\n", it->color, G.t, bfs_dist[G.cy][G.cx]);
#endif
        break;
    case IT_POTION: G.hp = G.hp + 25 > G.max_hp ? G.max_hp : G.hp + 25; snd_sfx(SFX_POTION); break;
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
        take_piece(&o);
        snd_sfx(SFX_KEYGET); G.gold = 1.0f; G.grin_t = 1.6f;
        break;
    }
    default: {
        int v = treasure_value(it->type);
        RUN.treasure += v; G.level_treasure += v;
        if (!it->secret) { G.value_got += v; G.items_got++; }
        snd_sfx_ex(SFX_COIN, it->type >= IT_CROWN ? 1.2f : 0.9f, 0.5f);
        G.gold = it->type >= IT_CROWN ? 1.0f : 0.6f;
        if (it->type >= IT_CROWN) G.grin_t = 1.6f;
        break;
    }
    }
    pop_readout();
}

static void update_items(void) {
    for (int i = 0; i < G.nitems; i++) {
        Item *it = &G.items[i];
        if (!it->alive) continue;
        float d = hypotf(it->x - G.px, it->y - G.py);
        if (!it->seen && d < (it->type == IT_KEY ? 13.0f : 8.5f) && line_clear(G.px, G.py, it->x, it->y)) it->seen = 1;
        if (d < (G.player ? 0.5f : 1.5f) && G.level_t > 1.0 && !G.ending) collect(it);   /* the autopilot reaches out */
    }
}

/* the tally counts up row by row: time, kills, treasure, souls, secrets */
#define TALLY_ROWS 5
static const float TALLY_ROW0 = 1.6f, TALLY_ROW_GAP = 1.0f, TALLY_COUNT = 0.75f, TALLY_END = 9.0f;
static int tally_value(int row, float t, int final) {
    float k = smoothstepf(0.0f, 1.0f, (t - TALLY_ROW0 - row * TALLY_ROW_GAP) / TALLY_COUNT);
    return (int)floorf(final * k);
}
static void update_tally(double dt) {
    float pt = (float)G.tally_t;
    G.tally_t += dt;
    float t = (float)G.tally_t;
    int finals[TALLY_ROWS] = { (int)G.clear_time, G.kills, G.items_got, G.level_souls, G.secrets_found };
    for (int r = 0; r < TALLY_ROWS; r++) {
        int v = tally_value(r, t, finals[r]);
        if (v != G.tally_shown[r]) { G.tally_shown[r] = v; snd_sfx_ex(SFX_TICK, 0.7f, 0.5f); }
        float done = TALLY_ROW0 + r * TALLY_ROW_GAP + TALLY_COUNT;
        if (pt < done && t >= done) snd_sfx_ex(r == 2 ? SFX_COIN : (r == 4 && G.nsecrets && G.secrets_found == G.nsecrets ? SFX_SECRET : SFX_ENTER), 1.0f, 0.5f);
    }
    float perfect = TALLY_ROW0 + TALLY_ROWS * TALLY_ROW_GAP + 0.2f;
    if (pt < perfect && t >= perfect && G.items_total > 0 && G.items_got == G.items_total) snd_sfx_ex(SFX_KEYGET, 1.0f, 0.5f);
    if (t >= TALLY_END) { G.level++; RUN.level = G.level; RUN.hp = G.hp; begin_shop(); }
}

static void update_shop(double dt);

static void update_ui(void);
static void open_ui(int which);
static void update_human(float dt);
/* how far up the hand is: a cast, a rune shown off, or the wisp held up to see by */
#define WISP_AX 64.0f                       /* where the wisp's tail is bound, on the back of the hand */
#define WISP_AY 55.0f
static float hand_raise(void) {
    float lift = G.rune_lift > 0.0f ? 0.8f * smoothstepf(1.4f, 1.25f, G.rune_lift) * smoothstepf(0.0f, 0.3f, G.rune_lift) : 0.0f;
    float hand = fmaxf(G.hand, lift);
    if (G.wisp_a > 0.0f && !G.shop) hand = fmaxf(hand, 0.62f * smoothstepf(0.0f, 0.6f, G.wisp_a));
    return hand;
}
/* the hand rises from the bottom right, reaching up and over, Catacomb 3-D / Doom style */
static void hand_origin(float hand, float *ox, float *oy) {
    float walk = G.player || G.wisp_a > 0.0f ? clampf(G.spd / WALK, 0.0f, 1.0f) : 0.0f;
    *ox = 170.0f + (int)(2.0f * sinf((float)G.t * 3.0f) + 5.0f * walk * sinf(G.bobph * 0.5f));
    *oy = H - (int)(hand * 130.0f) + (int)(2.0f * sinf((float)G.t * 6.0f) + 5.0f * walk * fabsf(sinf(G.bobph * 0.5f))) - (G.shop || G.player ? 40 : 0);
}

/* where the wisp leads: the exit, or first the key to the vault the exit is in, and to the vault that
 * key is in, if it comes to that */
static void wisp_goal(int *gx, int *gy) {
    *gx = G.exit_fx; *gy = G.exit_fy;
    int k = G.exit_in_vault;
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

/* which way the wisp leans: along the way, as far down it as there is a clear walk from here */
static int wisp_heading(float *out) {
    int x = (int)G.px, y = (int)G.py, gx, gy;
    wisp_goal(&gx, &gy);
    if (gx < 0 || x < 0 || y < 0 || x >= MAPN || y >= MAPN) return 0;
    if (gx != G.route_gx || gy != G.route_gy) build_route(gx, gy);
    int to_exit = gx == G.exit_fx && gy == G.exit_fy;
    float tx = to_exit ? G.exit_wx + 0.5f : gx + 0.5f, ty = to_exit ? G.exit_wy + 0.5f : gy + 0.5f;   /* there: the door, or the key */
    if (G.route[y][x] < 0) { *out = atan2f(ty - G.py, tx - G.px); return 1; }
    for (int k = 0; k < 7 && G.route[y][x] > 0; k++) {
        int bx = -1, by = -1;
        float best = 1e9f;
        for (int d = 0; d < 4; d++) {
            int nx = x + DX[d], ny = y + DY[d];
            if (nx < 0 || ny < 0 || nx >= MAPN || ny >= MAPN || G.route[ny][nx] != G.route[y][x] - 1) continue;
            float e = hypotf((float)(nx - gx), (float)(ny - gy));  /* of equal ways, the straightest */
            if (e < best) { best = e; bx = nx; by = ny; }
        }
        if (bx < 0 || (k > 0 && !walk_clear(G.px, G.py, bx + 0.5f, by + 0.5f))) break;
        x = bx; y = by; tx = x + 0.5f; ty = y + 0.5f;
    }
    *out = atan2f(ty - G.py, tx - G.px);
    return 1;
}

/* Ten souls taken on a level and a will-o'-wisp comes to rest on the back of the hand. Its body leans
 * the way out, held to the hand by its tail, and it lights the crypt a little way round. */
static void update_wisp(float dt) {
    int want = !G.shop && !G.dead && !G.exiting && G.level_souls >= WISP_SOULS && G.exit_fx >= 0;
    float hd, ox, oy, t = (float)G.t;
    if (want && !G.wisp_on) {                                      /* it comes: a spark on the hand that grows */
        G.wisp_on = 1;
        say("A WISP KINDLES. IT KNOWS THE WAY OUT");
        snd_sfx_ex(SFX_SHIMMER, 0.7f, 0.5f);
        if (wisp_heading(&hd)) G.wisp_dir = hd;
        hand_origin(hand_raise(), &ox, &oy);
        for (int i = 0; i < WISP_N; i++) { G.wtx[i] = ox + WISP_AX; G.wty[i] = oy + WISP_AY; }
        G.wcx = ox + WISP_AX; G.wcy = oy + WISP_AY;
        G.wbvx = G.wbvy = G.wcvx = G.wcvy = 0.0f; G.wscale = 1.0f;
        memset(G.wsparks, 0, sizeof G.wsparks);
    }
    G.wisp_a = want ? fminf(1.0f, G.wisp_a + dt / 1.4f) : fmaxf(0.0f, G.wisp_a - dt / 0.6f);
    for (int i = 0; i < WISP_SPARKS; i++) {                        /* sparks shed from it, rising and going out */
        if (G.wsparks[i].life <= 0.0f) continue;
        G.wsparks[i].life -= dt;
        G.wsparks[i].x += G.wsparks[i].vx * dt; G.wsparks[i].y += G.wsparks[i].vy * dt;
        G.wsparks[i].vx *= expf(-dt * 2.0f);
    }
    if (G.wisp_a <= 0.0f || dt <= 0.0f) return;
    if (wisp_heading(&hd)) G.wisp_dir = wrap_pi(G.wisp_dir + wrap_pi(hd - G.wisp_dir) * fminf(1.0f, dt * 4.0f));
    hand_origin(hand_raise(), &ox, &oy);
    float ax = ox + WISP_AX, ay = oy + WISP_AY, e = smoothstepf(0.0f, 1.0f, G.wisp_a);
    float rel = wrap_pi(G.wisp_dir - G.ang), fw = cosf(rel), sd = sinf(rel);
    /* a little 3-D of its own: the way ahead is up and further off, behind is lower and nearer */
    float tx = ax + e * (30.0f * sd + 3.0f * sinf(t * 1.7f)),
          ty = ay - e * (27.0f + 13.0f * fw + 2.5f * sinf(t * 2.3f + 1.0f));
    G.wscale += (1.0f - 0.22f * fw - G.wscale) * fminf(1.0f, dt * 5.0f);
    float *bx = &G.wtx[WISP_N - 1], *by = &G.wty[WISP_N - 1];
    G.wbvx += ((tx - *bx) * 70.0f - G.wbvx * 11.0f) * dt;          /* it drifts after where it means to be */
    G.wbvy += ((ty - *by) * 70.0f - G.wbvy * 11.0f) * dt;
    *bx += G.wbvx * dt; *by += G.wbvy * dt;
    /* the tail: a chain from the hand to the body, slack and bowed when the body is close, taut when it
     * pulls away; its bend swings after the body, and a ripple runs along it */
    float vx = *bx - ax, vy = *by - ay, len = fmaxf(hypotf(vx, vy), 0.01f), nx = -vy / len, ny = vx / len;
    if (ny > 0.3f || (fabsf(ny) <= 0.3f && nx > 0.0f)) { nx = -nx; ny = -ny; }
    float bow = 0.45f * sqrtf(fmaxf(0.0f, 50.0f * 50.0f - len * len)) * e;
    float cx = ax + vx * 0.5f + nx * bow, cy = ay + vy * 0.5f + ny * bow - 3.0f * e;
    G.wcvx += ((cx - G.wcx) * 40.0f - G.wcvx * 7.0f) * dt;
    G.wcvy += ((cy - G.wcy) * 40.0f - G.wcvy * 7.0f) * dt;
    G.wcx += G.wcvx * dt; G.wcy += G.wcvy * dt;
    G.wtx[0] = ax; G.wty[0] = ay;
    for (int i = 1; i < WISP_N - 1; i++) {
        float u = (float)i / (WISP_N - 1), a0 = (1 - u) * (1 - u), a1 = 2 * u * (1 - u), a2 = u * u;
        float rip = 2.2f * e * sinf(t * 7.0f - i * 1.1f) * sinf(u * PI_F);
        G.wtx[i] = a0 * ax + a1 * G.wcx + a2 * *bx + nx * rip;
        G.wty[i] = a0 * ay + a1 * G.wcy + a2 * *by + ny * rip;
    }
    if ((G.wspark_t -= dt) <= 0.0f) {
        G.wspark_t = 0.05f + 0.05f * rndf();
        for (int i = 0; i < WISP_SPARKS; i++) {
            if (G.wsparks[i].life > 0.0f) continue;
            float m = 0.45f + 0.4f * rndf();
            G.wsparks[i].x = *bx + (rndf() - 0.5f) * 9.0f * G.wscale; G.wsparks[i].y = *by + (rndf() - 0.5f) * 6.0f;
            G.wsparks[i].vx = (rndf() - 0.5f) * 20.0f + G.wbvx * 0.3f; G.wsparks[i].vy = -12.0f - 16.0f * rndf() + G.wbvy * 0.3f;
            G.wsparks[i].life = G.wsparks[i].max = m;
            break;
        }
    }
}

static void crawl_update(double dt) {
    float fdt = (float)dt;
    G.msg_t = fmaxf(0.0f, G.msg_t - fdt); G.hint_t = fmaxf(0.0f, G.hint_t - fdt);
    G.ui_t += fdt;
    update_toasts(fdt);
    if (G.player && G.ui && !G.shop) { update_ui(); return; }     /* a menu is up: the crypt holds its breath */
    if (G.player && (G.tally || G.dead) && IN.hit[KEY_ESC]) { open_ui(UI_QUIT); return; }
    if (G.player && (G.tally || G.dead) && IN.hit[KEY_B]) { open_ui(UI_BESTIARY); return; }
    G.t += dt; G.level_t += dt; G.card += fdt;
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
    static int meet_level = -1;                                    /* one of a kind, ahead, once the level's card has gone */
    if (getenv("CRAWL_MEET") && meet_level != G.level && G.level_t > 5.0 && !G.dead) {
        int k = clampi(atoi(getenv("CRAWL_MEET")), 0, NKINDS - 1), far = 0;
        for (int d = 1; d <= 7; d++) {                             /* the farthest open cell in view, up to seven */
            int mx = (int)(G.px + cosf(G.ang) * d), my = (int)(G.py + sinf(G.ang) * d);
            if (G.map[my][mx].kind != K_FLOOR || !line_clear(G.px, G.py, mx + 0.5f, my + 0.5f)) break;
            if (d >= 3 && !occupied(mx, my, NULL)) far = d;
        }
        if (far) {
            Mob *m = spawn_mob((int)(G.px + cosf(G.ang) * far), (int)(G.py + sinf(G.ang) * far), k);
            if (m && k == MOB_HUNTER) m->sub = 0;
            meet_level = G.level;
        }
    }
#endif
    for (int i = 0; i < MAXMOBS; i++) if (G.mobs[i].alive) update_mob(&G.mobs[i], fdt);
    update_pools(fdt);
    /* the Wraith comes, once, after about four minutes on a level, from the far end of it */
    float wraith_at = 240.0f;
#ifdef CRAWL_DEBUG
    if (getenv("CRAWL_WRAITH")) wraith_at = (float)atof(getenv("CRAWL_WRAITH"));
#endif
    if (!G.wraith_done && (G.level >= KINDS[MOB_WRAITH].debut || crawl_loop(G.level) > 0) && G.level_t > wraith_at && !G.tally && !G.exiting
        && (G.wraith_try -= fdt) <= 0.0f) {
        int x, y;
        G.wraith_try = 5.0f;
        if (pick_cell_at_depth(G.cx, G.cy, 20, 400, 1, &x, &y)) {
            Mob *m = spawn_mob(x, y, MOB_WRAITH);
            if (m) { m->alert = 1e9f; G.wraith_done = 1; sfx_at(SFX_WAIL, x + 0.5f, y + 0.5f, 3.0f); }
#ifdef CRAWL_DEBUG
            fprintf(stderr, "EVENT wraith comes t=%.1f\n", G.level_t);
#endif
        }
    }
    update_shots(fdt);
    update_bones(fdt);
    update_doors(fdt);
    update_slide(fdt);
    update_wisp(fdt);
    if (!G.dead) { update_items(); update_status(fdt); if (G.level_t > 1.0) update_defense(fdt); }
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
    if (G.calm > (G.player ? 35.0f : 11.0f)) {                     /* a person clears a level for good, mostly */
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
#ifdef CRAWL_DEBUG
        fprintf(stderr, "EVENT leaving t=%.2f in_vault=%d got=%.2f\n", G.t, G.exit_in_vault, got);
#endif
    }
    if (!G.tally && !G.dead && !G.player && (G.exit_fx < 0 || G.level_t > 600.0)) {   /* no exit, or hopelessly lost: call it done */
        G.tally = 1; G.tally_t = 0.0; G.clear_time = G.level_t; G.retries = 0; snd_sfx_ex(SFX_FANFARE, 1.0f, 0.5f);
        rec_cleared();
    }
    float fout = 1.0f;
    if (G.dead) {                                                  /* down to the floor, a red haze, then the level again */
        G.dead_t += dt;
        G.camz = lerpf(0.5f, 0.1f, smoothstepf(0.0f, 0.9f, (float)G.dead_t));
        fout = 1.0f - smoothstepf(3.8f, 4.8f, (float)G.dead_t);
        if (G.dead_t >= 5.0) { RUN.treasure -= G.level_treasure; G.hp = gear_max_hp(&RUN.gear); start_level(); }
    }
    float fin = smoothstepf(0.0f, 1.2f, (float)G.level_t);
    if (G.exiting) fout = 1.0f - smoothstepf(1.6f, 2.4f, G.exit_t);  /* into the light */
    G.fade = fin * fout;
    build_lightmap();
}

/* ================================================================== */
/* The merchant's antechamber, before each level: the owner's cat on a chest, three wares on
 * pedestals (an attack spell, a defensive rune, something to wear), one to be had for the gold in
 * hand. The first is free. Then out through the east door and into the level. */
/* ================================================================== */

#define CAT_X 12.62f
#define CAT_Y 15.45f
#define CAT_S 1.05f
static const float PED_X[NPOOL] = { 13.75f, 15.5f, 17.25f }, PED_Y[NPOOL] = { 13.75f, 13.55f, 13.75f };
enum { SH_GREET, SH_LOOK, SH_THINK, SH_GO, SH_BUY, SH_THANK, SH_LEAVE };
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
    G.nsecrets = G.secrets_found = 0; G.slide = G.seek_secret = -1;
    for (int k = 0; k < NKEYS; k++) G.vault_room[k] = G.vault_door[k] = -1;
    memset(G.bulky, 0, sizeof G.bulky); memset(G.fog, 0, sizeof G.fog);
    for (int y = 13; y <= 17; y++) for (int x = 12; x <= 18; x++) carve(x, y, 0, TH_BRICK);
    G.rooms[G.nrooms++] = (Room){ 12, 13, 7, 5, 0, TH_BRICK };
    for (int x = 19; x <= 24; x++) carve(x, 15, 1, TH_CORRIDOR);
    G.map[15][19].kind = K_DOOR; G.map[15][19].door = 0;
    G.doors[0] = (Door){ 19, 15, 1, 0.0f, 0.0f, 0, 0, 0 }; G.ndoors = 1;
    G.map[12][15].tex = T_SIGN;                                    /* the merchant's sign behind the wares */
    G.map[12][13].tex = G.map[12][17].tex = T_TORCH0;
    light_static(13.5f, 12.92f, 1.35f, 4.6f); light_static(17.5f, 12.92f, 1.35f, 4.6f);
    G.map[17][11].tex = T_TORCH0; light_static(12.08f, 17.5f, 1.2f, 4.2f);   /* by the cat */
    G.map[12][14].tex = G.map[12][16].tex = T_BRICK;
    add_deco(12.4f, 13.4f, SP_CANDLES); light_static(12.45f, 13.45f, 0.9f, 3.0f);
    add_deco(18.55f, 13.45f, SP_URN); add_deco(18.55f, 17.55f, SP_URN);
    for (int i = 0; i < NPOOL; i++) add_deco(PED_X[i], PED_Y[i], SP_PEDESTAL);
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
    float v[NPOOL];
    int best = -1, dear = -1;
    float bs = 0.0f;
    for (int i = 0; i < NPOOL; i++) {
        v[i] = gear_value(g, &G.offers[i], G.level);
        if (v[i] <= 0.0f || G.offers[i].price > RUN.treasure) continue;
        float s = v[i] / sqrtf(fmaxf(1.0f, (float)G.offers[i].price) / 100.0f + 1.0f) * (0.8f + 0.4f * rndf());
        if (s > bs) { bs = s; best = i; }
        if (dear < 0 || G.offers[i].price > G.offers[dear].price) dear = i;
    }
    G.shop_saving = 0;
    if (G.level == 1) return best;                                 /* the first is free: take the best */
    #define SPELL(o) ((o).kind == GK_ATK || (o).kind == GK_DEF)
    if (RUN.goal > 0) {                                            /* saving: a spell worth the wait now? */
        int sp = -1;
        for (int i = 0; i < NPOOL; i++)
            if (SPELL(G.offers[i]) && G.offers[i].price <= RUN.treasure && v[i] > 0.5f && (sp < 0 || v[i] > v[sp])) sp = i;
        if (sp >= 0) { RUN.goal = 0; return sp; }
        if (RUN.treasure < RUN.goal && rndf() < 0.7f) { G.shop_saving = 1; return -1; }
        RUN.goal = 0;
    }
    float next = gear_haul(G.level) * 0.6f;                        /* what the coming level should bring, cautiously */
    for (int i = 0; i < NPOOL; i++) {                              /* a spell it can't quite afford: put the gold by */
        if (!SPELL(G.offers[i]) || G.offers[i].price <= RUN.treasure || v[i] <= (best >= 0 ? v[best] : 0.3f)) continue;
        float p = G.offers[i].price <= RUN.treasure + next ? 0.4f : 0.15f;
        if (best >= 0 && SPELL(G.offers[best])) p *= 0.3f;         /* a spell in hand already */
        if (rndf() < p) { RUN.goal = G.offers[i].price; G.shop_saving = 1; return -1; }
    }
    #undef SPELL
    if (dear >= 0 && rndf() < 0.12f) return dear;                  /* splurge */
    return best;
}

static void begin_shop(void) {
    G.shop = 1; G.state = PS_SHOP;
    crawl_art_level(1);                                            /* the antechamber looks the same every time */
    gen_shop();
    memset(G.mobs, 0, sizeof G.mobs); memset(G.shots, 0, sizeof G.shots); memset(G.booms, 0, sizeof G.booms);
    memset(G.bones, 0, sizeof G.bones); memset(G.beams, 0, sizeof G.beams); memset(G.burns, 0, sizeof G.burns);
    reset_caster_state();
    if (G.hp <= 0) G.hp = G.max_hp;
    G.tally = 0; G.dead = 0; G.exiting = G.leaving = 0; G.readout = 0.0f; G.card = 99.0f;
    G.hurt = G.gold = G.kick = G.chill = 0.0f;
    G.px = 14.35f; G.py = 15.95f; G.cx = 14; G.cy = 15;            /* close by the merchant */
    G.ang = face_of(CAT_X, CAT_Y);
    G.camz = 0.5f; G.hand = 0.0f; G.spd = 0.0f; G.bob = 0.0f;
    for (int p = 0; p < NPOOL; p++) {
        G.offers[p] = gear_offer(p, G.level, &RUN.gear, &G.rs);
        if (G.level == 1) G.offers[p].price = 0;
        G.offer_gone[p] = 0;
    }
    G.shop_step = SH_GREET; G.shop_t = 0.0; G.shop_pick = -1; G.shop_bought = 0; G.shop_look = -1;
    G.shop_gold = RUN.treasure; G.shop_saving = 0; G.sparkle = 0.0f;
    G.shop_heal = G.hp + (G.max_hp - G.hp) * 4 / 10;               /* a rest by the purring cat */
    G.cat_blink = G.cat_slow = G.cat_ear = G.cat_trill = 0.0f; G.purr_t = 0.4f;
    G.cat_next_blink = 2.5f + 2.0f * rndf(); G.cat_next_ear = 3.0f + 3.0f * rndf(); G.cat_tail_k = 0.0f;
    G.sdur = 0.0f; G.smove = 0.0f;
    G.level_t = 0.0;
#ifdef CRAWL_DEBUG
    fprintf(stderr, "SHOP level %d gold %d souls %d:", G.level, RUN.treasure, RUN.souls);
    for (int p = 0; p < NPOOL; p++) { char nm[80]; gear_name(&G.offers[p], nm, sizeof nm); fprintf(stderr, " [%s $%d%s]", nm, G.offers[p].price, G.offers[p].rare ? " RARE" : ""); }
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
    G.shop_pick = i; G.shop_bought = 1; G.offer_gone[i] = 1; G.sparkle = 0.8f;
    RUN.treasure -= o->price;
    if (o->kind == GK_PIECE) take_piece(o);
    else gear_take(&RUN.gear, o);
    if (o->price > 0) snd_sfx_ex(SFX_COIN, 1.1f, 0.5f);
    snd_sfx_ex(SFX_SHIMMER, 0.8f, 0.5f);
    if (o->kind == GK_DEF) snd_sfx_ex(SFX_RUNE, 0.8f, 0.5f);
    G.grin_t = 2.5f;
    sfx_at(SFX_TRILL, CAT_X, CAT_Y, 1.3f); G.cat_trill = 0.5f; G.cat_slow = 1.6f; G.cat_tail_k = 1.0f;
#ifdef CRAWL_DEBUG
    char nm[80]; gear_name(o, nm, sizeof nm);
    fprintf(stderr, "EVENT bought %s for %d (left %d) t=%.2f\n", nm, o->price, RUN.treasure, G.t);
#endif
}

static void shop_human(float dt);
static void update_ui(void);

static void update_shop(double dt) {
    float fdt = (float)dt;
    G.shop_t += dt;
    float st = (float)G.shop_t;
    shop_cat(fdt);
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
        if (st >= 2.4f) { G.shop_step = SH_LOOK; G.shop_look = 0; G.shop_t = 0.0; shop_turn(face_of(PED_X[0], PED_Y[0]), 0.7f); }
        break;
    case SH_LOOK: {                                                /* each ware in turn */
        float hold = 0.7f + 1.0f + 0.5f * (float)(hash2(G.level, G.shop_look) % 100) / 100.0f;
        if (st >= hold) {
            if (++G.shop_look >= NPOOL) { G.shop_look = -1; G.shop_step = SH_THINK; G.shop_t = 0.0; shop_turn(face_of(15.5f, 13.0f), 0.5f); }
            else { G.shop_t = 0.0; shop_turn(face_of(PED_X[G.shop_look], PED_Y[G.shop_look]), 0.45f); }
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
                shop_walk(PED_X[i] + (15.5f - PED_X[i]) * 0.25f, PED_Y[i] + 1.0f, 1.6f);
                G.shop_step = SH_GO;
            } else { G.shop_step = SH_THANK; shop_turn(face_of(CAT_X, CAT_Y), 0.7f); }
#ifdef CRAWL_DEBUG
            fprintf(stderr, "EVENT shop_pick %d saving=%d goal=%d t=%.2f\n", G.shop_pick, G.shop_saving, RUN.goal, G.t);
#endif
        }
        break;
    case SH_GO:
        if (st >= G.sdur + 0.15f) { G.shop_step = SH_BUY; G.shop_t = 0.0; shop_turn(face_of(PED_X[G.shop_pick], PED_Y[G.shop_pick]), 0.3f); }
        break;
    case SH_BUY: {                                                 /* reach out and take it */
        G.hand = st < 0.35f ? st / 0.35f : (st > 1.05f ? fmaxf(0.0f, 1.0f - (st - 1.05f) / 0.3f) : 1.0f);
        if (!G.shop_bought && st >= 0.5f) shop_purchase(G.shop_pick);
        if (st >= 1.5f) { G.hand = 0.0f; G.shop_step = SH_THANK; G.shop_t = 0.0; shop_turn(face_of(CAT_X, CAT_Y), 0.7f); }
        break;
    }
    case SH_THANK:                                                 /* a look back at the merchant */
        if (!G.shop_bought && st > 0.9f && st - fdt <= 0.9f) G.cat_slow = 1.4f;
        if (st >= 2.1f) { G.shop_step = SH_LEAVE; G.shop_t = 0.0; shop_walk(18.2f, 15.5f, 1.8f); }
        break;
    default: {                                                     /* out through the door */
        Door *d = &G.doors[0];
        if (hypotf(d->x + 0.5f - G.px, d->y + 0.5f - G.py) < 2.2f && d->open < 1.0f) d->opening = 1;
        if (st >= G.sdur && G.sx1 < 20.0f) { G.shop_t = 0.0; shop_walk(22.5f, 15.5f, 1.8f); }
        break;
    }
    }
    if (G.px > 20.4f) {                                            /* through the door and into the level */
        G.fade = 1.0f - smoothstepf(20.4f, 21.6f, G.px);
        if (G.px >= 21.6f) { start_level(); return; }
    }
    update_doors(fdt);
    if (G.px <= 20.4f) G.fade = smoothstepf(0.0f, 0.7f, (float)G.level_t);
    G.mood_t -= fdt;
    if (G.mood_t <= 0.0f) { snd_set_mood(0.6f); G.mood_t = 0.5f; }
    build_lightmap();
}

/* ================================================================== */
/* A person at the controls (hold H for three seconds)                 */
/* ================================================================== */

#define MOUSE_TURN 0.0015f                  /* radians per mouse count */
#define KEY_TURN 2.6f                       /* radians a second with the arrow keys */
#define BODY 0.24f                          /* the caster's girth, against walls */
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
        if (m->type == MOB_SAC || shade_hidden(m)) continue;       /* a shade shows only in light */
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
    if (d->locked == 1) {
        if (!G.has_key[d->color]) { say("THE %s DOOR IS LOCKED", KEY_COLOR[d->color]); return 0; }
        d->locked = 2; G.has_key[d->color] = 0; G.unlock_t = G.level_t; snd_sfx(SFX_UNLOCK); pop_readout();
    }
    if (!d->opening && d->open < 0.95f) { d->opening = 1; sfx_at(SFX_DOOR, d->x + 0.5f, d->y + 0.5f, 1.0f); }
    return 1;
}

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
    else if (G.exit_fx >= 0 && cx == G.exit_wx && cy == G.exit_wy && (int)G.px == G.exit_fx && (int)G.py == G.exit_fy) human_exit();
}

/* E or Space: the door or exit just ahead */
static int human_use(void) {
    int cx = (int)floorf(G.px + cosf(G.ang) * 0.8f), cy = (int)floorf(G.py + sinf(G.ang) * 0.8f);
    Cell *c = cell(cx, cy);
    if (c->kind == K_DOOR) { open_door(&G.doors[c->door]); return 1; }
    if (c->kind == K_WALL && c->secret == 2)                       /* a cracked wall: it gives */
        for (int i = 0; i < G.nsecrets; i++) if (G.secrets[i].sx == cx && G.secrets[i].sy == cy && !G.secrets[i].state) { open_secret(i); G.grab_t = 1.0f; return 1; }
    if (!G.shop && G.exit_fx >= 0 && cx == G.exit_wx && cy == G.exit_wy && hypotf(G.exit_fx + 0.5f - G.px, G.exit_fy + 0.5f - G.py) < 1.0f) { human_exit(); return 1; }
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
        for (int i = 0; i < NPOOL; i++) push_from(PED_X[i], PED_Y[i], 0.5f);
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
    if (which == UI_INVENTORY) { G.ui_sel = 0; G.inv_page = 0; G.inv0 = RUN.gear; snd_sfx_ex(SFX_KEY, 0.8f, 0.5f); }
    else if (which == UI_BESTIARY) { best_default_rank(); snd_sfx_ex(SFX_KEY, 0.8f, 0.5f); }
    else { G.ui_sel = 1; G.quit_line++; snd_sfx_ex(SFX_KNOCK, 0.6f, 0.5f); }
}

/* the hand and the spell in it, the way a person casts: click (or Ctrl) and it goes where you look */
static void human_cast(float dt) {
    G.cooldown -= dt;
    int trig = IN.down[KEY_MOUSE1] || IN.down[KEY_CTRL];
    if (trig && G.pc_t < 0.0f && G.cooldown <= 0.0f && !G.dead) {
        if (G.spell == AT_BFS && RUN.souls < bfs_cost(RUN.gear.atk[AT_BFS])) {
            if (IN.hit[KEY_MOUSE1] || IN.hit[KEY_CTRL]) { say("THE BFS9000 NEEDS %d SOULS", bfs_cost(RUN.gear.atk[AT_BFS])); snd_sfx_ex(SFX_ZAP, 0.4f, 0.5f); }
        } else {
            float cyc = spell_cycle(&RUN.gear, G.spell);
            G.pc_t = 0.0f; G.pc_fired = 0; G.pc_len = clampf(cyc * 0.35f, 0.22f, 0.5f); G.cooldown = cyc;
        }
    }
    float rest = G.shop ? 0.45f : 0.62f, h = rest;                 /* lower in the merchant's room, to see the wares */
    if (G.pc_t >= 0.0f) {
        G.pc_t += dt;
        float L = G.pc_len;
        if (!G.pc_fired && G.pc_t >= 0.35f * L) { G.pc_fired = 1; cast_spell(G.spell, aim_target()); G.calm = 0.0f; }
        h = G.pc_t < 0.3f * L ? lerpf(rest, 1.0f, G.pc_t / (0.3f * L)) : (G.pc_t < 0.7f * L ? 1.0f : lerpf(1.0f, rest, (G.pc_t - 0.7f * L) / (0.3f * L)));
        if (G.pc_t >= L) G.pc_t = -1.0f;
    }
    if (G.grab_t > 0.0f) { G.grab_t -= dt; h = fmaxf(h, rest + (1.0f - rest) * sinf(PI_F * clampf(1.0f - G.grab_t, 0.0f, 1.0f))); }
    G.hand = h;
}

static void update_human(float dt) {
    G.calm += dt;
#ifdef CRAWL_DEBUG
    if (getenv("CRAWL_FOLLOWWISP") && G.wisp_a > 0.5f) {           /* testing: walk wherever the wisp leans */
        G.ang = wrap_pi(G.ang + wrap_pi(G.wisp_dir - G.ang) * fminf(1.0f, dt * 8.0f));
        IN.down[KEY_W] = 1;
        IN.hit[KEY_E] = fmodf((float)G.t, 0.5f) < dt;              /* doors in the way */
        if (fmodf((float)G.t, 2.0f) < dt)
            fprintf(stderr, "FOLLOW t=%.1f at (%.2f,%.2f) route=%d goal=(%d,%d) dir=%.2f ang=%.2f\n", G.t, G.px, G.py,
                    G.route[(int)G.py][(int)G.px], G.route_gx, G.route_gy, G.wisp_dir, G.ang);
        if ((int)G.px == G.exit_fx && (int)G.py == G.exit_fy) { G.ang = heading_angle(G.exit_dir); IN.hit[KEY_E] = 1; }
        G.hp = G.max_hp;
    }
#endif
    spell_keys();
    if (IN.hit[KEY_TAB]) { open_ui(UI_INVENTORY); return; }
    if (IN.hit[KEY_B]) { open_ui(UI_BESTIARY); return; }
    if (IN.hit[KEY_ESC]) { open_ui(UI_QUIT); return; }
    human_walk(dt);
    if (IN.hit[KEY_E] || IN.hit[KEY_SPACE]) human_use();
    if (G.exiting) return;
    human_cast(dt);
}

/* in the merchant's room: walk up to a ware to see it, E, Space or a click to buy it */
static void shop_human(float dt) {
    spell_keys();
    if (IN.hit[KEY_TAB]) { open_ui(UI_INVENTORY); return; }
    if (IN.hit[KEY_B]) { open_ui(UI_BESTIARY); return; }
    if (IN.hit[KEY_ESC]) { open_ui(UI_QUIT); return; }
    human_walk(dt);
    G.shop_look = -1;
    float bd = 3.2f;
    for (int i = 0; i < NPOOL; i++) {
        if (G.offer_gone[i]) continue;
        float d = hypotf(PED_X[i] - G.px, PED_Y[i] - G.py);
        if (d < bd && fabsf(wrap_pi(atan2f(PED_Y[i] - G.py, PED_X[i] - G.px) - G.ang)) < 0.3f + 0.25f / d) { bd = d; G.shop_look = i; }
    }
    if (IN.hit[KEY_E] || IN.hit[KEY_SPACE] || IN.hit[KEY_MOUSE1]) {
        int i = G.shop_look;
        if (i >= 0) {
            if (G.shop_bought) say("ONE WARE A VISIT");
            else if (G.offers[i].price > RUN.treasure) { say("YOU CAN'T AFFORD THAT"); G.cat_ear = 0.32f; G.cat_ear_side = rnd(2); }
            else { shop_purchase(i); G.grab_t = 1.0f; }
        } else if (!human_use() && hypotf(G.px - CAT_X, G.py - CAT_Y) < 1.8f) { G.cat_slow = 1.4f; sfx_at(SFX_TRILL, CAT_X, CAT_Y, 0.9f); }   /* a pat */
    }
    human_cast(dt);
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
        if (G.shop_bought) { G.shop_step = SH_LEAVE; G.shop_t = 0.0; shop_walk(18.2f, 15.5f, 1.8f); }
        else { G.shop_step = SH_THINK; G.shop_t = 0.0; shop_turn(face_of(15.5f, 13.0f), 0.5f); }
        return;
    }
    if (G.exiting || G.tally || G.dead) return;
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

/* ---- the scene's side of the bargain with the app ---- */

static void crawl_input(const Input *in, double dt) { (void)dt; IN = *in; }

static void crawl_control(int what) {
    if (what == CTRL_PAUSE) { if (G.player && !G.ui && G.status_ret != PLAY_QUIT) open_ui(UI_QUIT); return; }
    G.player = 1; G.status_ret = PLAY_PLAYER; G.ui = UI_NONE;
    if (!G.quit_line) G.quit_line = 1 + rnd(97);                    /* the crypt's arguments, from somewhere new each time */
    if (G.shop) { G.sdur = 0.0f; G.shop_look = -1; }
    else if (!G.exiting && !G.tally && !G.dead) human_begin();
    G.hint_t = 6.0f;
    snd_sfx_ex(SFX_RUNE, 1.0f, 0.5f); snd_sfx_ex(SFX_WHOOSH, 0.6f, 0.5f);
    say("THE CONTROLS ARE YOURS");
#ifdef CRAWL_DEBUG
    if (getenv("CRAWL_TPEXIT") && !G.shop && G.exit_fx >= 0) {     /* testing: straight to the exit, facing it */
        G.px = G.exit_fx + 0.5f; G.py = G.exit_fy + 0.5f; G.ang = heading_angle(G.exit_dir); human_begin();
    }
    if (getenv("CRAWL_TPSECRET") && !G.shop && G.nsecrets) {      /* testing: three steps back from a secret, facing it */
        const Secret *sc = &G.secrets[atoi(getenv("CRAWL_TPSECRET")) % G.nsecrets];
        G.px = sc->fx + 0.5f - DX[sc->d] * 2.5f; G.py = sc->fy + 0.5f - DY[sc->d] * 2.5f; G.ang = heading_angle(sc->d); human_begin();
    }
    fprintf(stderr, "EVENT control t=%.2f at (%.2f,%.2f) ang %.3f shop %d\n", G.t, G.px, G.py, G.ang, G.shop);
#endif
}

static int crawl_status(void) { return G.player ? G.status_ret : PLAY_AUTO; }

/* What a save keeps: the level to start from, health, gold, souls and the gear, and since version 2
 * the run's record (kills, secrets, levels cleared, achievements). Not where you stood: you come back
 * to the start of the level, so its gold and souls are taken back out. Version 3 packs the gear field
 * by field (what is worn, the bag, the bones, a ware held at the merchant's) and keeps souls in
 * tenths. Versions 1 and 2 still load: their spells and runes stay, their robe, hood and best ring
 * become Common pieces of their tier, and version 1 has an empty record. */
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
static void gear_unpack(const uint8_t *p, Gear *g) {
    static const int SLOT_IK[NSLOT] = { IK_WRIST, IK_WRIST, IK_BOOTS, IK_NECK, IK_HOOD, IK_ROBE, IK_RING };
    gear_reset(g);
    for (int i = 0; i < NATK; i++) g->atk[i] = (int8_t)clampi((int8_t)*p++, -1, 15);
    if (g->atk[AT_FIRE] < 0) g->atk[AT_FIRE] = 0;
    for (int i = 0; i < NDEF; i++) g->def[i] = (int8_t)clampi((int8_t)*p++, 0, 15);
    for (int s = 0; s < NSLOT; s++, p += PIECE_BYTES) {
        g->worn[s] = piece_unpack(p);
        if (g->worn[s].kind != SLOT_IK[s]) memset(&g->worn[s], 0, sizeof g->worn[s]);   /* only what goes there */
    }
    g->bag_size = (uint8_t)clampi(p[0], 0, BAG_MAX); g->bag_quality = (uint8_t)clampi(p[1], 0, NQUAL - 1); p += 2;
    for (int i = 0; i < BAG_MAX; i++, p += PIECE_BYTES) if (i < g->bag_size) g->bag[i] = piece_unpack(p);
    g->bones = (int16_t)clampi(get16(p), 0, 999); p += 2;
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
    for (int i = 0; i < NDEF; i++) g->def[i] = (int8_t)clampi((int8_t)p[NATK + i], 0, 15);
    int robe = (int8_t)p[18], robe_t = (int8_t)p[19], hood = (int8_t)p[20], hood_t = (int8_t)p[21];
    if (robe > 0 && robe < 6 && robe_t > 0) g->worn[SL_ROBE] = piece_make(IK_ROBE, robe_t, ROBE_STAT[robe], &rs);
    if (hood > 0 && hood < 5 && hood_t > 0) g->worn[SL_HOOD] = piece_make(IK_HOOD, hood_t, HOOD_STAT[hood], &rs);
    int best = 0;
    for (int f = 0; f < 4; f++) if ((int8_t)p[22 + f] >= 0 && (int8_t)p[26 + f] > best) best = (int8_t)p[26 + f];
    if (best > 0) g->worn[SL_RING] = piece_make(IK_RING, best, ST_DAMAGE, &rs);
}
/* the record, little-endian, field by field */
static void rec_pack(uint8_t *p, const Record *r) {
    for (int k = 0; k < REC_KINDS; k++) for (int i = 0; i < NRANK; i++, p += 2) put16(p, r->kills[k][i]);
    memcpy(p, r->spell_kills, sizeof r->spell_kills); p += sizeof r->spell_kills;
    memcpy(p, r->seen, sizeof r->seen); p += sizeof r->seen;
    put16(p, r->secrets); put16(p + 2, r->descents); put32(p + 4, (int32_t)r->cleared); p += 8;
    memcpy(p, r->got, sizeof r->got);
}
static void rec_unpack(const uint8_t *p, Record *r) {
    for (int k = 0; k < REC_KINDS; k++) for (int i = 0; i < NRANK; i++, p += 2) r->kills[k][i] = (uint16_t)get16(p);
    memcpy(r->spell_kills, p, sizeof r->spell_kills); p += sizeof r->spell_kills;
    memcpy(r->seen, p, sizeof r->seen); p += sizeof r->seen;
    r->secrets = (uint16_t)get16(p); r->descents = (uint16_t)get16(p + 2); r->cleared = (uint32_t)get32(p + 4); p += 8;
    memcpy(r->got, p, sizeof r->got);
}
static int save_version(const uint8_t *buf, int len) {             /* 1, 2 or 3 if it is a save we can read, else 0 */
    if (len < SAVE_V1 || (uint32_t)get32(buf) != SAVE_MAGIC) return 0;
    int v = get32(buf + 4);
    if (v == 1 || (v == 2 && len >= SAVE_V1 + REC_BYTES) || (v == 3 && len >= SAVE_V3 + REC_BYTES)) return v;
    return 0;
}
static int save_end(int v) { return (v >= 3 ? SAVE_V3 : SAVE_V1) + (v >= 2 ? REC_BYTES : 0); }   /* a grave's time follows */

static int crawl_save(uint8_t *buf, int cap) {
    int n = SAVE_V3 + REC_BYTES;
    if (cap < n) return 0;
    int level = G.level, hp = G.dead || G.hp <= 0 ? gear_max_hp(&RUN.gear) : G.hp, gold = RUN.treasure, souls = RUN.souls, shop = 0;
    if (G.tally || G.exiting) { level = G.level + 1; shop = 1; }   /* the level is won */
    else if (G.shop) shop = !G.shop_bought;
    else { gold -= G.level_treasure; souls -= G.level_souls; }
    put32(buf, (int32_t)SAVE_MAGIC);
    put32(buf + 4, 3);                                             /* version */
    put32(buf + 8, level); put32(buf + 12, hp); put32(buf + 16, gold); put32(buf + 20, souls * 10); put32(buf + 24, shop);
    gear_pack(buf + 28, &RUN.gear);
    rec_pack(buf + SAVE_V3, &RUN.rec);
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
    if (v >= 3) { gear_unpack(buf + 28, &RUN.gear); RUN.souls /= 10; }
    else gear_from_old(buf + 28, &RUN.gear, (uint32_t)RUN.level * 2654435761u);
    RUN.hp = clampi(get32(buf + 12), 1, gear_max_hp(&RUN.gear));
    if (RUN.treasure < 0) RUN.treasure = 0;
    if (RUN.souls < 0) RUN.souls = 0;
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
    if (G.shop) { G.sdur = 0.0f; G.shop_look = -1; }
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

static void update_ui(void) {
    if (G.ui == UI_INVENTORY) update_inventory();
    else if (G.ui == UI_GRAVES) update_graves();
    else if (G.ui == UI_BESTIARY) update_bestiary();
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

static void render_floor(Frame *f, float dx, float dy, float plx, float ply, float horizon) {
    float lx = dx - plx, ly = dy - ply, rx = dx + plx, ry = dy + ply;
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
            if (floor_row) {
                int cx = (int)floorf(wx), cy = (int)floorf(wy);
                float fx = wx - cx, fy = wy - cy;
                base = (uint8_t)(R_FLOOR + 8 + ((cx * 7 + cy * 13) & 1));
                if (grout && (fx < 0.035f || fy < 0.035f)) base = R_FLOOR + 5;
                if (G.nova_t > 0.0f) {                             /* the frost nova racing out over the floor */
                    float r = 2.2f * sqrtf(G.nova_t / 0.6f), d = hypotf(wx - G.px, wy - G.py);
                    if (d < r && (fabsf(d - r) < 0.22f || BAYER[y & 3][x & 3] < (int)(10.0f * (1.0f - G.nova_t / 0.6f)))) {
                        row[x] = (uint8_t)(R_GLOW + clampi((int)(15.0f - fabsf(d - r) * 30.0f), 5, 15));
                        continue;
                    }
                }
            } else base = R_CEIL + 8;
            float L = lm_at(wx, wy) * a;
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
                if (tex == T_EXIT0) tex = G.exit_open > 0.3f ? T_EXIT_OPEN : T_EXIT0 + ((int)(G.t * 2.5) & 1);
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
typedef struct { float x, y, z, depth; int spr; float alpha, twinkle, scale; int remap, flags; float sway; const uint8_t *lut; } Draw;
enum { DR_EYES = 1, DR_IMPLODE = 2, DR_BRIGHT = 4, DR_ICE = 8, DR_CAT = 16, DR_SWAY = 32, DR_FLIPV = 64 };   /* FLIPV: upside down, on the ceiling */

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
        return m->sub == 3 ? SP_HUNTER_LEAP : (((int)(m->t * 10.0f)) & 1 ? SP_HUNTER_B : SP_HUNTER_A);
    case MOB_GHOUL:
        if (m->state == MS_DIE) return SP_GHOUL_D;
        return m->sub == 3 ? SP_GHOUL_LEAP : (m->sub == 4 ? SP_GHOUL_FEED : (fr ? SP_GHOUL_B : SP_GHOUL_A));
    case MOB_KNIGHT: {
        if (m->state == MS_DIE) return SP_KNIGHT_D;
        int base = SP_KNIGHT_A + knight_stage(m) * 4;
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

static void render_sprites(Frame *f, float dx, float dy, float horizon) {
    static Draw list[MAXDECO + MAXMOBS + MAXSHOTS + MAXBOOMS + MAXBONES + MAXITEMS + MAXBURNS + MAXPOOLS];
    int n = 0;
    float rx = -dy, ry = dx;
    #define ADD(X, Y, Z, S, GH) do { float vx_ = (X) - G.px, vy_ = (Y) - G.py, dp_ = vx_ * dx + vy_ * dy; \
        if (dp_ > 0.12f && dp_ < 24.0f) list[n++] = (Draw){ X, Y, Z, dp_, S, (GH) > 0.0f ? (GH) : 1.0f, 0.0f, 1.0f, 0, 0, 0.0f, NULL }; } while (0)
    for (int i = 0; i < G.ndecos; i++) {
        const Deco *dc = &G.decos[i];
        static const float FPS[4] = { 0.0f, 0.0f, 3.0f, 9.0f };    /* by frame count: candles flicker, fire roars */
        if (dc->spr < 0) continue;                                 /* gone: risen, or never a statue */
        int spr = dc->spr + (dc->nfr > 1 ? (int)((float)G.t * FPS[dc->nfr & 3] + dc->phase) % dc->nfr : 0), n0 = n;
        ADD(dc->x, dc->y, 0.0f, spr, 0);
        if (n > n0 && (dc->flags & DF_SWAY)) {                     /* a slow swing, and a shiver on top of it */
            list[n - 1].flags = DR_SWAY;
            list[n - 1].sway = 2.6f * sinf((float)G.t * 0.9f + dc->phase) + 0.6f * sinf((float)G.t * 2.3f + dc->phase * 1.7f);
        }
    }
    for (int i = 0; i < MAXMOBS; i++) {
        Mob *m = &G.mobs[i];
        if (!m->alive || buried(m)) continue;
        float sc;
        int s = mob_sprite(m, &sc), n0 = n, statue = m->type == MOB_GARGOYLE && m->sub == 0;
        float gh = m->type == MOB_GHOST ? (m->state == MS_RECOIL ? 0.9f : 0.72f * (1.0f - m->fade) + 0.01f) : (m->type == MOB_SHADE ? shade_alpha(m) : 0.0f);
        if (m->type == MOB_SHADE && gh < 0.04f) continue;
        ADD(m->x, m->y, m->z, s, gh);
        if (n == n0) continue;
        Draw *d = &list[n - 1];
        d->scale = statue ? 1.0f : sc * RANK_SIZE[m->rank];       /* a statue like the others: its rank doesn't show yet */
        if (m->rank != RK_I && KINDS[m->type].family >= 0 && !statue) d->lut = crawl_rank_lut[KINDS[m->type].family][m->rank];
        if (m->type == MOB_SPIDER && m->state == MS_WALK && m->sub == 0) d->flags = DR_EYES;
        if (m->type == MOB_HUNTER && m->state == MS_WALK && m->sub == 0) d->flags = DR_FLIPV;
        if (m->frozen > 0.0f && m->state == MS_WALK) d->flags |= DR_ICE;   /* only its eyes, in the dark of the web */
        if (m->state == MS_DIE && m->sub == 77) {                  /* drawn in to a point */
            float k = smoothstepf(0.1f, 0.75f, m->t);
            d->z += sc * 0.5f * k; d->scale = sc * (1.0f - k) + 0.02f; d->flags = DR_IMPLODE;
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
        } else ADD(b->x, b->y, 0.04f, SP_BONEFLY0 + (((int)b->spin) & 3), 0);
    }
    for (int i = 0; i < G.nitems; i++) {
        const Item *it = &G.items[i];
        if (!it->alive) continue;
        static const int SPR[] = { SP_COINS_A, SP_CHALICE, SP_GEMS, SP_CROWN, SP_CHEST, SP_KEY0, SP_POTION, SP_SOULURN, SP_RINGITEM };
        float ph = (float)G.t + it->phase, z = 0.0f;
        int sp = SPR[it->type];
        if (it->type == IT_COINS && ((int)(ph * 2.5f) & 1)) sp = SP_COINS_B;
        if (it->type == IT_KEY) { sp = key_sprite(it->color, ph); z = 0.14f + 0.04f * sinf(ph * 2.5f); }
        int n0 = n;
        ADD(it->x, it->y, z, sp, 0);
        if (n > n0 && it->type != IT_KEY) { float u = ph * 0.6f - floorf(ph * 0.6f); list[n - 1].twinkle = u < 0.1f ? sinf(u / 0.1f * PI_F) : 0.0f; }
    }
    if (G.shop) {
        build_cat();
        int n0 = n;
        ADD(CAT_X, CAT_Y, 0.0f, SP_PEDESTAL, 0);
        if (n > n0) { list[n - 1].scale = CAT_S; list[n - 1].flags = DR_CAT; }
        for (int i = 0; i < NPOOL; i++) {
            if (G.offer_gone[i]) continue;
            const Offer *o = &G.offers[i];
            int sp = offer_sprite(o);
            float sc = 0.42f, z = 0.55f + 0.03f * sinf((float)G.t * 2.0f + i * 2.1f) - (TS - 1 - spr_bottom(sp)) / (float)TS * sc;
            n0 = n;
            ADD(PED_X[i], PED_Y[i], z, sp, 0);
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
        for (int x = x0; x < x1; x++) {
            if (d->depth >= zbuf[x]) continue;
            int tx = clampi((int)((x + 0.5f - left) * step), 0, TS - 1);
            const uint8_t *col = &img[tx * TS];
            float ty = (y0 + 0.5f - top) * step;
            for (int y = y0; y < y1; y++, ty += step) {
                int tyi = clampi((int)ty, 0, TS - 1);
                uint8_t c = col[(d->flags & DR_FLIPV) ? TS - 1 - tyi : tyi];
                if (c == C_CLEAR) continue;
                if (d->alpha < 1.0f && d->alpha * 16.0f <= BAYER[(y + (int)(G.t * 20.0)) & 3][x & 3] + 0.5f) continue;
                if (d->lut) c = d->lut[c];
                if (d->remap && c >= R_PURPLE && c < R_PURPLE + 16) c = ramp_at(d->remap, c - R_PURPLE);
                if (d->flags & DR_IMPLODE) { f->pix[y * W + x] = c == 0 ? 0 : (uint8_t)(R_GLOW + clampi(6 + (int)((c & 15) * 0.6f), 6, 15)); continue; }
                if ((d->flags & DR_ICE) && c != 0 && ((x + y) & 1)) {    /* rimed with frost */
                    f->pix[y * W + x] = fogged((uint8_t)(R_GLOW + clampi(2 + (int)(((c - 1) & 15) * 0.5f + light * 4.0f), 2, 12)), fog, light, x, y);
                    continue;
                }
                f->pix[y * W + x] = (d->flags & DR_BRIGHT) ? c : fogged(shade(c, light, x, y), fog, light, x, y);
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
    for (int i = 0; i < NPOOL; i++) {
        float sx, sy, dep;
        if (!project(PED_X[i], PED_Y[i], i == G.shop_pick && G.sparkle > 0.0f ? 0.78f : 1.04f, dx, dy, horizon, &sx, &sy, &dep)) continue;
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
        int w = gfx_text_width(tag, 1), tx = x - w / 2, ty = (int)sy - 10;
        if (ty < 0 || ty > H - 60) continue;
        uint8_t col = o->price <= RUN.treasure ? (uint8_t)(R_GOLD + 12) : (uint8_t)(R_BLOOD + 11);
        gfx_text8(f, tx + 1, ty + 1, tag, 0, 1);
        gfx_text8(f, tx, ty, tag, col, 1);
        if (o->rare && ((int)(G.t * 3.0) & 1)) { int rw = gfx_text_width("RARE", 1); gfx_text8(f, x - rw / 2 + 1, ty - 9, "RARE", 0, 1); gfx_text8(f, x - rw / 2, ty - 10, "RARE", C_WHITE, 1); }
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
static void render_statusbar(Frame *f) {
    int y0 = H - 44;
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
        case 3: snprintf(num, sizeof num, "%d", RUN.souls); col = R_GLOW + 12; break;
        default: {                                                 /* the spell in hand, by name and tier */
            int sp = clampi(G.spell, 0, NATK - 1), t = RUN.gear.atk[sp];
            uint8_t sc = ramp_at(gear_color(GK_ATK, sp), 13);
            gfx_text8(f, P[i].x + (P[i].w - gfx_text_width(ATK_SHORT[sp], 1)) / 2, y0 + 19, ATK_SHORT[sp], sc, 1);
            if (sp == AT_BFS) snprintf(num, sizeof num, "-%d", bfs_cost(t));
            else { char nm[40]; gear_item_name(GK_ATK, sp, t, nm, sizeof nm); const char *r = strrchr(nm, ' '); snprintf(num, sizeof num, "%s", t > 0 && r ? r + 1 : ""); }
            gfx_text8(f, P[i].x + (P[i].w - gfx_text_width(num, 1)) / 2, y0 + 30, num, R_IRON + 13, 1);
            continue;
        }
        }
        gfx_text16(f, P[i].x + (P[i].w - gfx_text_width(num, 1)) / 2, y0 + 19, num, col, 1);
    }
    bevel(f, 304, y0 + 4, 12, 37, R_IRON + 2);                     /* the keys in hand */
    for (int c = 0; c < NKEYS; c++) if (G.has_key[c]) key_icon(f, 306, y0 + 9 + c * 10, c);
    bevel(f, 162, y0 + 1, 40, 43, 0);
    draw_face(f, 166, y0 + 3, face_sub());
}

/* 8x8 type wrapped at spaces to cols a line, lh px apart; returns the lines it took (no frame: only counts) */
static int text_wrap_ex(Frame *f, int x, int y, const char *s, int cols, int col, int maxlines, int lh) {
    int lines = 0;
    while (*s && lines < maxlines) {
        int n = (int)strlen(s), cut = n;
        if (n > cols) { cut = cols; while (cut > 0 && s[cut] != ' ') cut--; if (!cut) cut = cols; }
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
static void render_shop_card(Frame *f) {
    int i = G.shop_step == SH_LOOK || G.shop_step == SH_GO || (G.shop_step == SH_BUY && !G.shop_bought) ? G.shop_look : -1;
    if (G.shop_step == SH_LOOK && G.shop_t < 0.35) i = -1;
    if (G.player) i = G.shop_look;
    int y0 = H - 44 - 46;
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
        ph = 34 + 9 * text_wrap_ex(NULL, 0, 0, stats, (bw - 16) / 8, 0, 4, 9);
    }
    y0 = H - 44 - 4 - ph;
    gfx_rect(f, bx, y0, bw, ph, 0);
    for (int x = 0; x < bw; x++) { gfx_px(f, bx + x, y0, R_STONE + 10); gfx_px(f, bx + x, y0 + ph - 1, R_STONE + 6); }
    for (int y = 0; y < ph; y++) { gfx_px(f, bx, y0 + y, R_STONE + 10); gfx_px(f, bx + bw - 1, y0 + y, R_STONE + 6); }
    if (o->kind == GK_PIECE) {
        int col = qual_text(p->quality);
        gfx_text8(f, bx + 8, y0 + 5, KIND_NAME[p->kind], R_STONE + 11, 1);
        gfx_text8(f, bx + 8, y0 + 14, l1, col, 1);
        gfx_text8(f, bx + 8, y0 + 23, l2, col, 1);
        text_wrap_ex(f, bx + 8, y0 + 33, stats, (bw - 16) / 8, R_STONE + 13, 4, 9);
        gfx_text16(f, bx + bw - 8 - pw, y0 + 4, price, o->price <= RUN.treasure ? R_GOLD + 12 : R_BLOOD + 12, 1);
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
    if (G.mirror_t > 0.0f) {                                       /* a silvered ripple at the edges of sight */
        float k = fminf(1.0f, G.mirror_t * 2.0f);
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
static const char *const RUNE_GLYPH[NDEF][7] = {
    { "#.#.#", "#.#.#", ".###.", "..#..", "..#..", "..#..", "..#.." },   /* algiz: the ward */
    { "##...", "#.#..", "#..#.", "#.#..", "##...", "#.#..", "#..#." },   /* raido: the journey */
    { "#...#", "##.##", "#.#.#", "#...#", "#...#", "#...#", "#...#" },   /* mannaz: the mirror */
    { "##...", "#.#..", "#.#..", "##...", "#.#..", "#.#..", "##..." },   /* berkano: healing */
    { "#....", "##...", "#.#..", "#....", "#....", "#....", "#...." },   /* laguz: water, cleansing */
    { "#.#.#", ".###.", "#####", ".###.", "#.#.#", "..#..", "..#.." },   /* frost, a star */
    { "#....", "##...", "#.#..", "#..#.", "#.#..", "##...", "#...." },   /* thurisaz: the thorn, dread */
    { "#...#", "##.##", "#.#.#", "#...#", "#.#.#", "##.##", "#...#" },   /* dagaz: the turning of time */
    { "###..", "#.#..", "#..#.", "#.#..", "###..", "#.#..", "#..#." },   /* perthro: lots, fate, souls */
};

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
    float wa = G.wisp_a * 0.8f, wbx = G.wtx[WISP_N - 1], wby = G.wty[WISP_N - 1];   /* the wisp lights the skin under it */
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
            if (wa > 0.0f) { float k = fmaxf(0.0f, 1.0f - hypotf(sx - wbx, sy - wby) / 52.0f); lit += wa * k * k; }
            f->pix[sy * W + sx] = shade(c, lit, sx, sy);
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
static void wisp_ball(Frame *f, float cx, float cy, float r, float a, int pass) {
    float reach = pass ? 1.0f : 2.0f;
    int x0 = (int)floorf(cx - r * reach), x1 = (int)ceilf(cx + r * reach), y0 = (int)floorf(cy - r * reach), y1 = (int)ceilf(cy + r * reach);
    for (int y = y0 < 0 ? 0 : y0; y <= y1 && y < H; y++)
        for (int x = x0 < 0 ? 0 : x0; x <= x1 && x < W; x++) {
            float d = hypotf(x + 0.5f - cx, y + 0.5f - cy) / r;
            if (pass) {
                if (d >= 1.0f) continue;
                f->pix[y * W + x] = d < 0.42f && r > 1.6f ? C_WHITE : (uint8_t)(R_GLOW + 15 - (int)(d * 6.0f));
            } else if (d >= 1.0f && d < 2.0f) {
                float k = (2.0f - d) * a;
                if (k * 16.0f > BAYER[y & 3][x & 3] + 0.5f) f->pix[y * W + x] = (uint8_t)(R_GLOW + 6 + (int)(k * 4.0f));
            }
        }
}

/* the will-o'-wisp on the back of the hand: bound there by its tail, its body leaning the way out */
static void render_wisp(Frame *f) {
    if (G.wisp_a <= 0.0f || G.shop) return;
    float e = smoothstepf(0.0f, 1.0f, G.wisp_a), t = (float)G.t, s = G.wscale * (0.35f + 0.65f * e);
    float bx = G.wtx[WISP_N - 1], by = G.wty[WISP_N - 1];
    float flick = 1.0f + 0.09f * sinf(t * 17.0f) + 0.06f * sinf(t * 29.0f + 1.3f);
    /* its flame streams back along the tail */
    float tdx = G.wtx[WISP_N - 2] - bx, tdy = G.wty[WISP_N - 2] - by, tl = fmaxf(hypotf(tdx, tdy), 0.01f);
    tdx /= tl; tdy /= tl;
    for (int pass = 0; pass < 2; pass++) {
        for (int i = 1; i < WISP_N - 1; i++) {                     /* the tail's links, a pulse running out along them */
            float u = (float)i / (WISP_N - 1), pulse = 0.5f + 0.5f * sinf(t * 9.0f - i * 0.9f);
            if (pass) {                                            /* the thread between links */
                int n = (int)(hypotf(G.wtx[i] - G.wtx[i - 1], G.wty[i] - G.wty[i - 1]) * 1.5f) + 1;
                for (int k = 0; k <= n; k++) {
                    int x = (int)lerpf(G.wtx[i - 1], G.wtx[i], (float)k / n), y = (int)lerpf(G.wty[i - 1], G.wty[i], (float)k / n);
                    if (x >= 0 && y >= 0 && x < W && y < H) f->pix[y * W + x] = (uint8_t)(R_GLOW + 7 + (int)(pulse * 3.0f * e));
                }
            }
            wisp_ball(f, G.wtx[i], G.wty[i], (0.9f + 2.2f * u * u) * s * (0.85f + 0.3f * pulse), 0.55f * e, pass);
        }
        for (int k = 4; k >= 1; k--) {                             /* tongues of it, licking back */
            float w = sinf(t * 11.0f + k * 2.1f) * 0.9f * k;
            wisp_ball(f, bx + (tdx * 3.4f * k - tdy * w) * s, by + (tdy * 3.4f * k + tdx * w) * s, (5.6f - 1.0f * k) * s * flick, 0.75f * e, pass);
        }
        for (int k = 1; k <= 2; k++) {                             /* and flickering up off its crown */
            float ph = t * (2.3f + 0.7f * k) + k * 1.7f, u = ph - floorf(ph);
            wisp_ball(f, bx + sinf(ph * 5.0f) * 1.5f * s, by - (4.0f + 7.0f * u) * s, (2.4f - 1.6f * u) * s, 0.6f * e * (1.0f - u), pass);
        }
        wisp_ball(f, bx, by, 6.2f * s * flick, e, pass);
    }
    float ax = G.wtx[0], ay = G.wty[0];                             /* where it is bound: a mark on the skin */
    for (int y = -3; y <= 3; y++)
        for (int x = -3; x <= 3; x++) {
            int px = (int)ax + x, py = (int)ay + y, m = abs(x) + abs(y);
            if (px < 0 || py < 0 || px >= W || py >= H || m > 2 || (m == 2 && x && y)) continue;
            f->pix[py * W + px] = (uint8_t)(R_GLOW + (m == 0 ? 15 : m == 1 ? 10 + (int)(3.0f * e) : 8));
        }
    for (int i = 0; i < WISP_SPARKS; i++) {                        /* sparks going out */
        float k = G.wsparks[i].life / fmaxf(G.wsparks[i].max, 0.01f);
        int x = (int)G.wsparks[i].x, y = (int)G.wsparks[i].y;
        if (G.wsparks[i].life <= 0.0f || x < 0 || y < 0 || x >= W || y >= H) continue;
        f->pix[y * W + x] = k > 0.7f ? C_WHITE : (uint8_t)(R_GLOW + 6 + (int)(k * 10.0f));
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
    if (G.card < 0.3f || G.card > 3.6f) return;
    char name[48];
    snprintf(line, sizeof line, "LEVEL %d", G.level);
    level_name(G.level, name, sizeof name);
    int bw = gfx_text_width(name, 1) + 32;
    card_box(f, bw > 210 ? bw : 210, 64, 68);
    card_text(f, 80, line, C_WHITE);
    card_text(f, 104, name, R_FLAME + 11);
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
    static const char *LABEL[TALLY_ROWS] = { "TIME", "KILLS", "TREASURE", "SOULS", "SECRETS" };
    int totals[TALLY_ROWS] = { 0, G.kills_total, G.items_total, 0, G.nsecrets };
    for (int r = 0; r < TALLY_ROWS; r++) {
        if (t < TALLY_ROW0 + r * TALLY_ROW_GAP - 0.3f) continue;
        int y = 82 + r * 16, v = G.tally_shown[r];
        gfx_text16(f, 52, y, LABEL[r], C_WHITE, 1);
        if (r == 0) snprintf(line, sizeof line, "%d:%02d", v / 60, v % 60);
        else if (r == 3) snprintf(line, sizeof line, "+%d", v);
        else snprintf(line, sizeof line, "%d / %d", v, totals[r]);
        int done = t >= TALLY_ROW0 + r * TALLY_ROW_GAP + TALLY_COUNT;
        gfx_text16(f, 268 - gfx_text_width(line, 1), y, line, done ? R_GOLD + 12 : R_GOLD + 8, 1);
        for (int x = 52 + gfx_text_width(LABEL[r], 1) + 6; x < 262 - gfx_text_width(line, 1); x += 4) gfx_px(f, x, y + 12, R_STONE + 7);   /* leader dots */
    }
    if (t > TALLY_ROW0 + TALLY_ROWS * TALLY_ROW_GAP + 0.2f) {
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
    snprintf(souls, sizeof souls, "%d", RUN.souls);
    int nk = 0;
    for (int c = 0; c < NKEYS; c++) nk += G.has_key[c] > 0;
    int show_souls = RUN.souls > 0 || RUN.gear.atk[AT_BFS] >= 0;
    int gx = 84, sx = gx + 10 + gfx_text_width(gold, 1) + 8;
    int kx = show_souls ? sx + 12 + gfx_text_width(souls, 1) + 8 : sx;
    int pw = kx + nk * 11 + 4, ph = 18, px = 6, py = H - 4 - (int)(k * (ph + 4)) + 4;
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
            int w = gfx_text_width(G.msg, 1), x = (W - w) / 2, y = toast_showing() ? 50 : 14;   /* below a pop-up */
            gfx_rect(f, x - 5, y, w + 10, 14, 0);
            gfx_text8(f, x, y + 3, G.msg, R_GOLD + 11, 1);
        }
    }
    if (G.hint_t > 0.0f && G.player && !G.ui) {
        static const char *const HINT[3] = { "WASD WALK  MOUSE TURN  CLICK CASTS", "1-9 SPELLS  E OPENS  TAB INVENTORY", "B BESTIARY  ESC LEAVE OR SLEEPWALK" };
        for (int i = 0; i < 3; i++) {
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

/* Tab, the second page: the caster's numbers (what abilities add in gold beside them) and the spellbook */
static void render_caster(Frame *f) {
    static const char *const SHORT[NSTAT] = { "ARMOR", "DODGE", "HEALTH", "RESTORES", "POISON WARD", "BURN WARD", "SLOW WARD",
                                              "MOVEMENT", "QUIET", "SPELL HASTE", "SPIRIT COST", "DAMAGE", "FIRE", "FROST",
                                              "LIGHTNING", "EFFECT" };
    const Gear *g = &RUN.gear, *g0 = &G.inv0;
    gfx_text16(f, 14, 9, "THE CASTER", R_GOLD + 12, 1);
    gfx_text8(f, 14, 26, "A/D PAGE  TAB CLOSE", R_STONE + 9, 1);
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
    static const char *const DEF_SHORT[NDEF] = { "WARD", "BLINK", "MIRROR", "MEND", "PURIFY", "NOVA", "DREAD", "SLIP", "REAP" };
    int any = 0;
    for (int k = 0; k < NDEF; k++) {
        if (g->def[k] <= 0) continue;
        any = 1;
        int w = gfx_text_width(DEF_SHORT[k], 1);
        if (x + w > W - 10) { x = 66; y += 9; }
        if (y > H - 14) break;
        gfx_text8(f, x, y, DEF_SHORT[k], G.def_cd[k] > 0.0f ? (uint8_t)(R_STONE + 8) : (uint8_t)(R_GLOW + 11), 1);
        x += w + 8;
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

static int toast_showing(void) { return TOASTS.n > 0 && TOASTS.t >= 0.0f; }

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
    const char *head = t->progress ? "PROGRESS" : "ACHIEVEMENT";
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
    for (int lv = 1; lv <= 20 && n < max; lv++) {
        AchLine *l = &L[n++];
        l->done = ach_has(ACH_LEVEL0 + lv - 1); l->known = 1;
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
        snprintf(l->prog, sizeof l->prog, "L%d", k * 20);
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

static void render_ui(Frame *f) {
    if (G.ui == UI_INVENTORY) render_inventory(f);
    else if (G.ui == UI_QUIT) render_quit(f);
    else if (G.ui == UI_GRAVES) render_graves(f);
    else if (G.ui == UI_BESTIARY) render_bestiary(f);
}

static void crawl_render(Frame *f) {
    f->w = W; f->h = H;
    memcpy(f->pal, crawl_pal, sizeof crawl_pal);
    f->fade = G.fade;
    if (G.tally) { render_tally(f); if (G.player && G.ui) render_ui(f); render_toasts(f); return; }
    g_camfog = fog_at(G.px, G.py);
    float ang = G.ang + G.kick;
    float dx = cosf(ang), dy = sinf(ang);
    float plx = -dy * PLANE_LEN, ply = dx * PLANE_LEN;
    float horizon = H / 2.0f + G.hurt * 3.0f;
    float camz = G.camz;
    G.camz -= 0.03f * G.hurt;
    render_floor(f, dx, dy, plx, ply, horizon);
    render_walls(f, dx, dy, plx, ply, horizon);
    render_sprites(f, dx, dy, horizon);
    render_beams(f, dx, dy, horizon);
    G.camz = camz;
    if (G.shop) render_shop_tags(f, dx, dy, horizon);
    if (!G.dead) { render_wards(f); render_hand(f); render_wisp(f); }
    if (G.shop) render_shop_card(f);
    if (G.shop || G.player) render_statusbar(f);                    /* a person gets the Wolfenstein bar */
    else { render_readout(f); render_face_popup(f); }
    if (!G.shop) render_card(f);
    render_messages(f);
    if (G.player && G.ui) render_ui(f);
    render_toasts(f);
    tint(f);
}

const Scene scene_crawl = { "CATACOMB", W, H, SND_PROG_CRAWL, crawl_enter, crawl_update, crawl_render,
                            crawl_control, crawl_status, crawl_input, crawl_save, crawl_load };
