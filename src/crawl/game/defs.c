/* A part of scene_crawl.c (docs/PLAN-tidy.md): it is #included there in its place and never compiled on its own.
 * Sizes and paces; the kinds of cell, creature, item, missile and harm; cells, doors, lights; the creature (Mob); the contracts between combat's two tracks; packs and ambushes; KINDS and the ranks' tables. */

/* a thing of paper or ink inside a menu keeps its own letters */
#define UI_PLAIN_BEGIN void (*hk8_)(Frame *, int, int, uint8_t, int, int) = gfx_hook8, (*hk16_)(Frame *, int, int, uint8_t, int, int) = gfx_hook16; gfx_hook8 = 0; gfx_hook16 = 0
#define UI_PLAIN_END gfx_hook8 = hk8_; gfx_hook16 = hk16_
static int ui_marble_lv = -1;                                      /* the level the menus' marble was made for */

#define W 320
#define H 200
#define MAPN 45                             /* 11 blocks of 3x3 cells and the walls between: 4 * 11 + 1 */
#define LMR 4                               /* light-map texels per cell */
#define LMN (MAPN * LMR)
#define PLANE_LEN 0.75f
#define FOCAL ((W / 2) / PLANE_LEN)
#define CRUISE 2.1f                         /* cells per second */
#define SPRINT_BASE 2.5f                    /* seconds of sprint everyone has (docs/PLAN-ai.md, 24); SPRINT adds to it */
#define WALK 3.0f                           /* cells a second, for a person at the keys */
#define BODY 0.24f                          /* the caster's girth, against walls */

enum { K_FLOOR, K_WALL, K_DOOR, K_PILLAR, K_PROP };   /* K_PROP: low and solid (a grave, a fire pit): walked round, seen and cast over */
enum { TH_CORRIDOR, TH_OSSUARY, TH_CRYPT, TH_BRICK, TH_MOSS, TH_COUNT };
enum { PS_MOVE, PS_TURN, PS_DOOR, PS_AIM, PS_CAST, PS_UNAIM, PS_HOLD, PS_EXIT, PS_SHOP, PS_PLAYER, PS_SETTLE, PS_PUSH, PS_ALTAR, PS_USE, PS_MEND,
       PS_LOOK, PS_PROBE, PS_GUARD };   /* the dream's counters (1.0.0b15 step 10): a glance back, a spell at a door, a corner held */
enum { MOB_SKEL, MOB_GHOST, MOB_BAT, MOB_WSKULL, MOB_SHIELD, MOB_SPIDER,
       MOB_SHAMBLER, MOB_DROWNED, MOB_HOUND, MOB_WRAITH, MOB_HUNTER, MOB_GHOUL, MOB_KNIGHT, MOB_BROOD, MOB_LING, MOB_SAC,
       MOB_SHADE, MOB_IMP, MOB_DEACON, MOB_GARGOYLE,
       MOB_WHIPPER, MOB_SEXTON, MOB_LIGHTLESS,
       MOB_RAT };   /* kept in saves (the record): new kinds go at the end (REC_KINDS 24 at most: the charnel rat took the last) */
enum { MS_WALK, MS_DIE, MS_RECOIL, MS_FLEE };   /* the last two: wraiths only */
enum { IT_COINS, IT_CHALICE, IT_GEMS, IT_CROWN, IT_CHEST, IT_KEY, IT_POTION, IT_URN, IT_RING, IT_BONE, IT_SCRIP, IT_MAP };   /* IT_SCRIP: color is its boss;
                                                                      IT_MAP the tutorial's (docs/PLAN-tutorial.md 7.4) */
static const int ITEM_VALUE[] = { 10, 25, 50, 100, 250, 0, 0, 0, 0, 0, 0 };
enum { MIS_BONE, MIS_FIRE, MIS_EMBER, MIS_TURNED, MIS_WEB };   /* what the dead throw: bones, a wall skull's fireball, an imp's
                                                                    ember; a spell of yours the Ill-Fated Knight's Mirror Rune
                                                                    turned; the Wandering Spider's glob of web */
enum { DM_PIERCE = 1, DM_ARC = 2, DM_SLOW = 4, DM_TICK = 8 };   /* how a hit lands: through shields, round them, chilling; poison or
                                                                    burning wearing on it (no hit landed, not made your target) */
#define DM_ATK(a) (((a) + 1) << 8)          /* and which spell it is, for the creature's weakness to it */
enum { HURT_BLOW, HURT_SPELL, HURT_POISON, HURT_STONE, HURT_VOID,   /* STONE: no dodging it; VOID: nothing resists it */
       HURT_ELEM };                         /* a hazard's stroke (1.0.0b13): no dodging it, resisted by its element alone */
/* SPELL (docs/PLAN-combat.md, section 3; it was HURT_MAGIC): dodged, cut by its element's own resist (harm_next; Fire if none
 * is named) and by half the armour */
#define HURT_MAGIC HURT_SPELL                /* the old name, for anything written before it changed */
/* What last hurt the dweller, for the stone if it was the end of them: a creature (its kind and rank), a
 * boss, a boss's hazard, or what a creature left behind (its poison, an imp's fire, a drowned one's pool),
 * or your own spell turned back by the knight's mirror */
enum { BL_NONE, BL_MOB, BL_BOSS, BL_HAZARD, BL_POISON, BL_FIRE, BL_POOL, BL_MIRROR };
typedef struct { uint8_t what, type, rank; } Blame;
#define NKEYS 3                             /* gold, silver, crimson */
#define SHOT_REFLECT NATK                   /* a shot kind: something the Mirror Veil sent back */
#define SHOT_WEB (NATK + 1)                 /* and your Tangled Web's glob */
#define SHOT_ORB (NATK + 2)                 /* the ultimates' (docs/PLAN-b11.md, section 9-10): Amperage's orb, */
#define SHOT_LANCE (NATK + 3)               /* Winter's Lance, */
#define SHOT_BALLISTA (NATK + 4)            /* the Ballista's heavy spear */
#define SHOT_SPLINTER (NATK + 5)            /* and a splinter, thrown when the bones are gone (1.0.0b16) */
#define NSHOTKIND (NATK + 6)
#define NPED 7                              /* the merchant's pedestals, at most */
enum { ALTAR_DORMANT, ALTAR_AWAKE, ALTAR_READY, ALTAR_SPENT };   /* a Black Altar: asleep, seeking, gold, given */
enum { AW_NONE, AW_SEEK, AW_HOME, AW_REST };   /* its wisp: not woken, at a secret's door, on its way home, resting over it */
#define ALTAR_SOULS 10                      /* what waking one costs */
#define ALTAR_BLESS 25                      /* its blessing on your exit wisps, once a level */
#define BLESS_REACH 4.0f                    /* how near a blessed wisp the undead turn */

typedef struct { uint8_t kind, tex, theme, visits; int16_t region, door; uint8_t secret, lamp;
                 uint8_t face[4], fover[4], fflip, ftorch, flo, cei, por; } Cell;   /* por: 1 + a portrait of the slain hung on one of its faces */   /* secret: 1 a hidden closet, 2 its cracked wall; lamp: a pocket's ceiling panel, 1 + its light, 255 dead */
/* On a drawn run (docs/PLAN-b10.md) a wall's faces have their own looks: face[d] is the texture of its face
 * toward (DX[d], DY[d]) (0: tex, as before), fover what stands on it, fflip bit d mirrored, ftorch the face a
 * torch is on, + 1. A floor cell's flo and cei are its floor and ceiling (0: plain); fflip bits 5-7 say how
 * its floor is turned (1, 2 mirrored, 4 across), bit 4 that it lies as laid, not turned at random. */
typedef struct { int x, y, along_x; float open, idle; int opening, locked, color, was; } Door;   /* locked: 1 locked, 2 unlocked;
                                                                      was: opening last frame (its sound and its noise) */
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
    float webbed, webmax, claw;                                    /* held in your Tangled Web: what is left of it, all of it, a claw at it */
    int ally; float ally_t, acool;                                 /* raised by your Grave Call: how long it has left; its next blow at
                                                                      the other side (an ally's at them, theirs at an ally) */
    int rank0;                                                     /* in a pocket: 1 + the rank it would have had on the level */
    float pois_t, pois_dps, pois_acc;                              /* poisoned (docs/PLAN-b11.md: the Blight Flask): time left, harm a second */
    float burn_t, burn_dps, burn_acc;                              /* alight: time left, harm a second (Firestarter, Firewall) */
    float pin, chill_t; int chill_n;                               /* Impale: pinned; Deep Chill: shards on it since chill_t */
    int tri, lure; float lx, ly;                                   /* a trial's (1.0.0b13): 1, or 2 one that goes for its things; drawn
                                                                      to (lx, ly) while lure is set */
    int aware, wx, wy, wn, hx, hy;                                 /* how the dead think (1.0.0b15, docs/PLAN-ai.md): MA_*; a search's
                                                                      next place and whether it has one; its post (0: none yet) */
    float aw_t, kx, ky, kh, look, edgy, wait;                      /* time in the state; where it means to look and the way you were
                                                                      going; its head turned from its facing; listening harder a while;
                                                                      a pause to look round */
    int role, route, wp; float pf;                                 /* step 2: RL_* (0 not yet given); a patrol's route and the
                                                                      waypoint it makes for; a post's facing */
    int pack, engage, hold, sleep, fetch; float flee, fetch_t;     /* step 3: 1 + its pack; one of those in the fight now; holding off
                                                                      (1 the fight is full, 2 waiting its pack's turn); a roost's bat asleep
                                                                      (1) or dropping (2); a broken pack's hound gone for help (1 + whom);
                                                                      scattering; how long it has looked for help */
    int called, scav; float call_t, scx, scy;                      /* step 4: a caller has called (or is about to, call_t); a ghoul
                                                                      gone to where something died, (scx, scy), to feed */
    int hunter; float shy;                                         /* step 5: one of a hunt (1 waiting, 2 on your trail, 3 the watch,
                                                                      going to your last noise); a shade lit and looked at, fading back */
    int amb, bait_st, bthrew; float bait_t;                        /* step 6: 1 + its ambush (a tooth, or with bait_st B_* its bait);
                                                                      the straggler's one throw; how long it has shown itself, or lost you */
    int flank, feint, cover, cvx, cvy, fell, dwx, dwy;             /* step 7: taking its pack's other way; 1 this wind-up is a feint, 2
                                                                      the last was; 1 gone behind a corner between throws, to (cvx, cvy);
                                                                      1 + the one of its pack it falls back to (-1: it has); the doorway
                                                                      it waits beside */
    float dodge_t, dodge_cd, door_t, lsx, lsy, lst, fall_t;        /* a step aside from a spell, and the wait before another; waiting
                                                                      beside a doorway you hold (below 0: tired of it); where and when it
                                                                      last saw you; how long it has been falling back */
    int set;                                                       /* step 8: the set piece it came with (SET_*) */
    int cov_n; float cov_t;                                        /* its covers running, and when the last was: three, then it stays out */
    int wild, rings; float ring_t, ring_cd, ice_t, seen_t, blocked;         /* step 9: a hound whose huntsman fell; the Sexton's rings since he saw
                                                                      you, his bell ringing (and how hard), the wait to ring hard again,
                                                                      frozen; when he last saw you; how long its way somewhere has been
                                                                      stood in */
    int wset, ward[2]; float wdmg[2], wring;                       /* 1.0.0b16: warded (docs/PLAN-play.md section 6): whether it has been
                                                                      given its ward yet; 1 + each element it is warded against; the
                                                                      damage of that element taken (a fifth of its health cracks it); the
                                                                      dull ring of a warded hit */
    /* docs/PLAN-combat.md, two tracks on one branch (its section 17): each adds fields only above its own end mark */
    int elem_on;                                                   /* the attack under way carries its element (section 8): set by C,
                                                                      read by D's eyes, which flare in its colour */
    float elem_t;                                                  /* an attack's element decided (C3): how long it holds, wind-up and
                                                                      a moment after; 0 not decided */
    int c_flags;                                                   /* contract (C4): DR_* flags C asks it drawn with while an attack of
                                                                      its is under way; the creatures' part ORs them in */
    int c4, c4n; float c4_t, c4_cd, c4a, c4x, c4y;                 /* C4's attacks: which is under way (C4_*), bones thrown; its clock,
                                                                      the wait before the next, its aim and where it began */
    int pyre;                                                      /* Pyre (1.0.0b21): it has passed its burning on, once */
    /* the caster's track (C) ends */
    /* the dead's track (D): */
    int elem;                                                      /* 1 + the element it was born to (section 7.4), 0 none: set by D,
                                                                      read by C's attacks */
    int wseen;                                                     /* its rank's strengths (ward[]) seen struck, a bit each (section
                                                                      7.3, B11): only these are shown */
    float stun_t, unb_t, chl_t, stun_cd, frz_cd;                   /* its statuses (section 5): stunned, Unbound; since the last
                                                                      chilling; the waits before a stun or a freeze can land again */
    int chl_n, frz_was;                                            /* Chilled stacks; frozen last frame (thawing takes the stacks) */
    float corner_t;                                                /* the Wraith: how long it has been driven back into a corner it
                                                                      can't leave (wraith_cornered) */
    float bld_t, bld_dps, bld_acc;                                 /* D7, Barbed: its bleed's time left, harm a second, harm owed */
    float stb_t; int stb_n;                                        /* D7, Static Build: since lightning last struck it, the strikes */
    /* the dead's track (D) ends */
    int feat; float feat_t;                                        /* the portraits' feats (docs/PLAN-portraits.md, 5 B): what it has done
                                                                      that spoils its kind's, or the moment that makes it (FT_*); when a
                                                                      gargoyle woke */
    float told_t;                                                  /* a charnel rat told of you by a mouse (docs/PLAN-mice.md): rearing */
} Mob;
enum { FT_THREW = 1, FT_DROPPED = 2, FT_HATCHED = 4, FT_HORN = 8, FT_RUNG = 16, FT_SHED = 32, FT_BROKE = 64, FT_NOW = 128 };
/* What the two tracks share (docs/PLAN-combat.md, section 17.3). Change any of it only as that section says. The statuses, the
 * same seven on the caster and on the dead (sections 4-5), and the one each element imparts */
enum { AF_FIRE, AF_CHILL, AF_FROZEN, AF_STUN, AF_POISON, AF_UNBOUND, AF_DOWN, NAF };
static inline int af_of(int el) {
    static const int8_t AF[NELEM] = { AF_FIRE, AF_CHILL, AF_STUN, AF_POISON, AF_UNBOUND, AF_DOWN };
    return el >= 0 && el < NELEM ? AF[el] : -1;
}
/* the dead's track fills these in; the caster's track's attacks call them (an Unbound creature has no element, Bone Thrower,
 * charge or call; a stunned one no blow, throw or call) */
static inline int mob_stunned(const Mob *m) { return m->stun_t > 0.0f; }   /* filled in by D3 */
static inline int mob_unbound(const Mob *m) { return m->unb_t > 0.0f; }
/* 1.0.0b20 (section 17.3): the Unholy Confessions (C5, section 10). conf_rank(cf): the whole ranks confessed, 0 none, 1-5 for I-V,
 * past 5 by overlevels. conf_value(cf, which): one of its numbers as the caster has it now, the class's weight in it (10.6), moved
 * a point at a time (10.1); 0 while nothing is confessed, and those marked "from I" 0 till rank I is whole. Shares are fractions
 * (0.30 is 30%), the rest in their stat's own units. C5 fills them; D5 reads them */
enum { CF_RESTORE, CF_AGILITY, CF_SPIRIT, CF_CONST, CF_SURVIVAL, CF_TOUGH, NCONF };
enum { CV_HEAL, CV_HEAL_SOULS, CV_PHYSIC };                        /* Restoration: the spell's health a second, a share of the most
                                                                      (from I); its souls a second (from I); Physic % */
enum { CV_HARD, CV_DODGE, CV_MOVE, CV_QUIET };                     /* Agility: Knocked Down, Stunned and Frozen cut (a share off the
                                                                      chance and the length); Dodge, Movement, Quiet % */
enum { CV_SOULS, CV_UNBOUND, CV_HASTE, CV_THRIFT, CV_EFFECT };     /* Spirituality: souls from kills (a share more); Unbound cut; Spell
                                                                      Haste %, Spirit Cost % less, Spell Effect % */
enum { CV_RECOVER, CV_SPRINT, CV_HEALTH, CV_LASTBLOW };            /* Constitution: the sprint's recovery (a share quicker), its length
                                                                      (s), health; a killing blow survived (a chance, from I) */
enum { CV_SOFT, CV_TREASURE, CV_LUCK };                            /* Survival: every other status cut; Treasure %, Luck % */
enum { CV_ARMOUR, CV_ARMOUR_WORN, CV_CRACKS };                     /* Toughness: armour; a share more of what is worn; how far a crack
                                                                      is seen, in cells (from I) */
static int conf_rank(int cf);
static float conf_value(int cf, int which);
/* D5's (sections 9, 10.4), called by C: the class's own sheet into the caster's totals (own[] in caster_sync's units, per ST_*),
 * what of it no stat holds (KX_*), and a killing blow survived (1 if it was: hurt_ex is about to let the caster die, not to a
 * seep). Stubs that do nothing till D5 fills them */
enum { KX_AF_CUT, KX_SOULS };                                      /* every status's chance, a share off; souls from kills, a share more */
static void class_own(float *own);
static float class_extra(int what);
static int blow_survived(int kind);
static float class_cap(int st);                                    /* 1.0.0b20, the owner's caps by class (docs/BRIEF-caps.md): how far
                                                                      the class lifts stat st's cap, in its units; D's, a stub till then */
static float gear_cut(int k);                                      /* 1.0.0b21 (D6, section 12): the share what is worn takes off status k's
                                                                      chance and length on the caster (Steadiness on Knocked Down, Stunned and
                                                                      Frozen; the Headsman's Block on Knocked Down), added to Agility's under its
                                                                      cap in conf_cut; D's, a stub till then */
static void abil_short(int ab, float a, char *buf, int n);         /* and an ability's few words on Tab's caster page (D's) */
/* 1.0.0b21 (section 17.3): the spells' augments (section 11; C6 the frame and Fire's, D7 the other five lines'). Each line has three
 * steps with a pair at each, one of which the caster chooses at the Lampstand: its opening, its ability, its ultimate (AUG_OPEN,
 * AUG_ABIL, AUG_ULT). aug_of(e, t): which of the pair is chosen at step t of line e (0 none, 1 the first, 2 the second);
 * aug_rank(e, t): its rank, 0-3, from the line's credits; aug_k(e, t, w): 0 unless w is the one chosen, else 0.5, 0.75 or 1 by its
 * rank (A8: the guide's numbers are rank III's). C6 fills them. The hooks that hand each line its say are after Shot */
enum { AUG_OPEN, AUG_ABIL, AUG_ULT, NAUGT };
enum { AUGQ_CHANCE, AUGQ_HARM };                                   /* aug_af's two questions: a status's chance, its harm */
static int aug_of(int e, int t);
static int aug_rank(int e, int t);
static inline float aug_k(int e, int t, int w);
/* a pack (docs/PLAN-ai.md, sections 5-6): its kind (PA_*), its members, the one that leads it, how many it began with; its
 * state (PAS_*), the one calling and how long the call has to run; when one of it last saw the caster, and where; its
 * turns: those engaged now, their health as their turn began, the one relieved last, and when the turn changes */
typedef struct {
    int kind, state, leader, n0, caller, broken, last;
    uint64_t mem;
    float call_t, seen_t, kx, ky, turn_t;
    int on[2]; float on_hp[2];
    float chill_t;                                                 /* step 8: the Wraith's wail heard; it keeps to its posts a while */
    int ward[2], wset, elem;                                       /* 1.0.0b16: the ward its members share (1 + the element), once given;
                                                                      1.0.0b19: the element they were born to (1 + it, -1 none, 0 not yet) */
    int hunted, hurts0, done;                                      /* 1.0.0b18: it has hunted you; your hurts as it set on you; its end
                                                                      counted */
} Pack;
#define MAXPACKS 12
enum { PACK_NONE, PACK_FILE, PACK_HOUNDS, PACK_ROOST, PACK_WAKE, PACK_SCAV, PACK_SPIDERS, PACK_IMPS, PACK_SENTINELS, PACK_BAND, PACK_WHIP };
/* the dead's ways, as a person meets them (1.0.0b18): named once a run of ten, and a page each in the Codex */
enum { WY_FILE, WY_HOUNDS, WY_ROOST, WY_WAKE, WY_SCAV, WY_CALLER, WY_HUNT, WY_AMBUSH, WY_WATCH, WY_NEST, WY_BAND, WY_IMPS, WY_SENTINELS,
       WY_WHIP, WY_BELL, WY_CHILL, WY_GATHER, NWAY };
enum { DEED_AMB, DEED_SHAKEN, DEED_HEADS, DEED_HUSH, DEED_FILE };   /* the record's deeds against them */
/* an ambush (docs/PLAN-ai.md, section 12): its kill room, the way in (outside, inside), the far side; its teeth; whether sprung (1;
 * 2 the teeth out searching, 3 come hunting); how often refused; 1 + its bait; 1 + a torch snuffed by its door */
typedef struct { int room, ox, oy, ix, iy, fx, fy, teeth, sprung, teases, bait, snuff, deaths0, done, eyes; } Ambush;   /* 1.0.0b18: the
                                                                      deaths on the level as it sprang; its end counted */
enum { B_NONE, B_WAIT, B_SHOW, B_LURE, B_HOME, B_GILD };
enum { PAS_CALM, PAS_CALL, PAS_HUNT };
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
    float hp, speed, scale, rad, weak[NELEM];
    int harm;                               /* what it does to you, a bit an element (H_*); a bat's Soul Sap is by rank */
} Kind;
#define H_FIRE (1 << EL_FIRE)
#define H_FROST (1 << EL_FROST)
#define H_POISON (1 << EL_POISON)
#define H_SPIRIT (1 << EL_SPIRIT)
#define H_PHYS (1 << EL_PHYS)
static const Kind KINDS[] = {
    /* what each element does to it, in the enum's order (fire, frost, lightning, poison, spirit, physical), then its harm */
    [MOB_SKEL] = { "SKELETON", "SKELETONS", "THE ORDER'S DEAD, STILL AT THEIR POSTS. THEY THROW WHAT THEY ARE MADE OF.",
        SP_SKEL_A, SP_SKEL_B, SP_SKEL_SA, SP_SKEL_SB, PF_BONE, 1, RK_V, 1, KF_UNDEAD | KF_SOLID | KF_PAGE | KF_DOORS, SFX_BONES, SP_SKEL_D3,
        20, 0.9f, 1.0f, 0.42f,                          { 1.0f, 1.0f, 1.0f, 0.5f, 1.0f, 1.5f }, H_PHYS },
    [MOB_GHOST] = { "SPECTRE", "SPECTRES", "A CHILL WITH A FACE. IT ONLY WANTS YOU TO KNOW IT IS THERE.",
        SP_GHOST_A, SP_GHOST_B, SP_GHOST_SA, SP_GHOST_SB, -1, 1, RK_I, 0, KF_SPIRIT | KF_PAGE | KF_UNCOUNTED, SFX_GHOST_DIE, -1,
        20, 0.7f, 1.35f, 0.42f,                         { 0 }, 0 },   /* only Spirit Mastery's Spirit reaches it (weak_el) */
    [MOB_BAT] = { "BAT", "BATS", "THEY COME FOR THE FACE, AND BITE WHEN THEY HAVE IT. DEEP DOWN THEY ARE THE COLOUR OF EMBERS.",
        SP_BAT_A, SP_BAT_B, SP_BAT_SA, SP_BAT_SB, PF_WING, 1, RK_IV, 0, KF_SOLID | KF_PAGE, SFX_BAT, -1,
        10, 1.7f, 1.0f, 0.30f,                          { 0.5f, 1.0f, 1.0f, 1.5f, 1.0f, 1.0f }, H_PHYS },
    [MOB_WSKULL] = { "WALL SKULL", "WALL SKULLS", "SET IN THE STONE TO WATCH THE HALLS. WALK UP TO ONE AND MEET ITS EYE, AND IT WAKES.",
        SP_WSKULL1, SP_WSKULL1, SP_WSKULL_S, SP_WSKULL_S, PF_BONE, 3, RK_V, 0, KF_PAGE, SFX_SHATTER, -1,
        1, 0.0f, 0.62f, 0.38f,                          { 1.0f, 1.0f, 1.0f, 0.5f, 1.0f, 1.0f }, H_FIRE },
    [MOB_SHIELD] = { "SHIELD-BEARER", "SHIELD-BEARERS", "A SKELETON THAT REMEMBERS ITS DRILL. GO ROUND THE SHIELD, OR THROUGH IT.",
        SP_SHIELD_A, SP_SHIELD_B, SP_SHIELD_SA, SP_SHIELD_SB, PF_BONE, 4, RK_V, 1, KF_UNDEAD | KF_SOLID | KF_PAGE | KF_DOORS | KF_MELEE, SFX_BONES, SP_SKEL_D3,
        30, 1.0f, 1.0f, 0.42f,                          { 1.0f, 1.0f, 1.5f, 1.0f, 1.0f, 0.5f }, H_PHYS },
    [MOB_SPIDER] = { "SPIDER", "SPIDERS", "IT WAITS IN A CORNER WEB, AND ALL YOU SEE ARE ITS EYES. THE DEEP ONES ARE PALE.",
        SP_SPIDER_A, SP_SPIDER_B, SP_SPIDER_SA, SP_SPIDER_SB, PF_CHITIN, 6, RK_V, 0, KF_PAGE, SFX_SKITTER, -1,
        15, 0.0f, 0.42f, 0.38f,                         { 1.5f, 0.5f, 1.0f, 1.0f, 1.0f, 1.0f }, H_POISON },
    [MOB_SHAMBLER] = { "SHAMBLER", "SHAMBLERS", "THE BURIED, UP AGAIN: SLOW, AND HUNGRY, AND IT WILL HOLD ON TO YOU. FIRE TAKES IT WELL.",
        SP_SHAMB_A, SP_SHAMB_B, SP_SHAMB_SA, SP_SHAMB_SB, PF_FLESH, 2, RK_V, 1, KF_UNDEAD | KF_SOLID | KF_PAGE | KF_DOORS | KF_MELEE, SFX_GROAN, SP_SHAMB_D2,
        32, 0.6f, 1.0f, 0.42f,                          { 1.5f, 0.5f, 1.0f, 1.0f, 1.0f, 1.0f }, H_PHYS },
    [MOB_DROWNED] = { "DROWNED", "DROWNED", "SWOLLEN WITH THE WATER IT DIED IN, WHICH DROWNS FIRE AND CARRIES LIGHTNING. IT BURSTS.",
        SP_DROWN_A, SP_DROWN_B, SP_DROWN_SA, SP_DROWN_SB, PF_DROWN, 7, RK_V, 1, KF_UNDEAD | KF_SOLID | KF_PAGE | KF_MELEE, SFX_SHATTER, -1,
        46, 0.55f, 1.1f, 0.46f,                         { 0.5f, 1.0f, 1.5f, 1.0f, 1.0f, 1.0f }, H_PHYS | H_POISON },
    [MOB_HOUND] = { "BONE HOUND", "BONE HOUNDS", "A DOG'S BONES THAT STILL KNOW THEIR WORK. THEY HUNT IN PAIRS, BITE, AND RUN.",
        SP_HOUND_A, SP_HOUND_B, SP_HOUND_SA, SP_HOUND_SB, PF_BONE, 8, RK_V, 0, KF_UNDEAD | KF_SOLID | KF_PAGE | KF_MELEE, SFX_BONES, SP_HOUND_D,
        14, 2.0f, 0.75f, 0.34f,                         { 1.0f, 1.5f, 1.0f, 1.0f, 1.0f, 0.5f }, H_PHYS },
    [MOB_WRAITH] = { "WRAITH", "WRAITHS", "IT WALKS THE HALLS AS YOU DO, AND IT DOES NOT TIRE. FIRE AND LIGHT DRIVE IT BACK, A WHILE.",
        SP_WRAITH_A, SP_WRAITH_B, SP_WRAITH_SA, SP_WRAITH_SB, -1, 9, RK_I, 0, KF_SPIRIT | KF_SOLID | KF_UNIQUE | KF_PAGE | KF_DOORS | KF_MELEE | KF_UNCOUNTED, SFX_WAIL, -1,
        1e9f, 0.75f, 1.6f, 0.5f,                        { 0 }, H_FROST },
    [MOB_HUNTER] = { "HUNTING SPIDER", "HUNTING SPIDERS", "IT DOES NOT WAIT IN A WEB. IT WALKS, ON THE FLOOR OR OVERHEAD, AND IT COMES FOR YOU.",
        SP_HUNTER_A, SP_HUNTER_B, SP_HUNTER_SA, SP_HUNTER_SB, PF_HIDE, 11, RK_V, 0, KF_PAGE | KF_MELEE, SFX_SKITTER, SP_HUNTER_D,
        22, 1.6f, 0.6f, 0.34f,                          { 1.0f, 1.5f, 0.5f, 1.0f, 1.0f, 1.0f }, H_PHYS | H_POISON },
    [MOB_GHOUL] = { "GHOUL", "GHOULS", "IT EATS WHAT THE OTHERS LEAVE. CATCH IT FEEDING AND IT IS OPEN.",
        SP_GHOUL_A, SP_GHOUL_B, SP_GHOUL_SA, SP_GHOUL_SB, PF_FLESH, 12, RK_V, 1, KF_UNDEAD | KF_SOLID | KF_PAGE | KF_MELEE, SFX_GROAN, SP_GHOUL_D,
        30, 1.5f, 0.85f, 0.40f,                         { 1.0f, 1.0f, 1.0f, 0.5f, 1.5f, 1.0f }, H_PHYS },
    [MOB_KNIGHT] = { "SKELETON KNIGHT", "SKELETON KNIGHTS", "PLATE OVER BONE. HIT IT TILL THE ARMOUR FALLS AWAY; IT GETS QUICKER AS IT DOES.",
        SP_KNIGHT_A, SP_KNIGHT_B, SP_KNIGHT_SA, SP_KNIGHT_SB, PF_ARMOUR, 13, RK_V, 1, KF_UNDEAD | KF_SOLID | KF_PAGE | KF_DOORS | KF_MELEE, SFX_SHATTER, SP_KNIGHT_D,
        60, 0.8f, 1.2f, 0.46f,                          { 1.0f, 1.0f, 1.5f, 1.0f, 1.0f, 0.5f }, H_PHYS },
    [MOB_BROOD] = { "BROOD MOTHER", "BROOD MOTHERS", "SHE LAYS AS SHE GOES. KILL HER AND THE SACS DIE WITH HER.",
        SP_BROOD_A, SP_BROOD_B, SP_BROOD_SA, SP_BROOD_SB, PF_CHITIN, 14, RK_V, 0, KF_SOLID | KF_PAGE, SFX_SKITTER, SP_BROOD_D,
        70, 0.6f, 1.0f, 0.5f,                           { 1.0f, 0.5f, 1.0f, 1.5f, 1.0f, 1.0f }, H_PHYS },
    [MOB_LING] = { "SPIDERLING", "SPIDERLINGS", "HER CHILDREN. ONE BITE EACH, AND THERE ARE ALWAYS MORE.",
        SP_LING_A, SP_LING_B, SP_LING_A, SP_LING_B, PF_CHITIN, 14, RK_V, 0, KF_PAGE | KF_MELEE, SFX_SKITTER, -1,
        4, 2.2f, 0.22f, 0.24f,                          { 1.0f, 0.5f, 1.0f, 1.5f, 1.0f, 1.0f }, H_PHYS },
    [MOB_SAC] = { "EGG SAC", "EGG SACS", "",
        SP_SAC_A, SP_SAC_B, SP_SAC_A, SP_SAC_B, -1, 14, RK_I, 0, KF_UNCOUNTED, SFX_SHATTER, SP_SAC_OPEN,
        6, 0.0f, 0.5f, 0.30f,                           { 1.0f, 0.5f, 1.0f, 1.5f, 0.0f, 1.0f }, 0 },
    [MOB_SHADE] = { "SHADE", "SHADES", "A SHAPE OF DARK THAT SHOWS ONLY IN LIGHT. ITS TOUCH TAKES SOULS.",
        SP_SHADE_A, SP_SHADE_B, SP_SHADE_SA, SP_SHADE_SB, -1, 16, RK_I, 0, KF_PAGE | KF_MELEE, SFX_GHOST_DIE, -1,
        30, 1.1f, 1.1f, 0.40f,                          { 1.0f, 1.0f, 1.0f, 0.5f, 1.5f, 1.0f }, H_SPIRIT },
    [MOB_IMP] = { "EMBER IMP", "EMBER IMPS", "BORN OF THE MAGMA, AND IT THROWS IT. COLD IS WHAT IT FEARS.",
        SP_IMP_A, SP_IMP_B, SP_IMP_SA, SP_IMP_SB, PF_CLAY, 17, RK_V, 0, KF_SOLID | KF_PAGE, SFX_SHATTER, SP_IMP_D,
        18, 1.4f, 0.55f, 0.30f,                         { 0.5f, 1.5f, 1.0f, 1.0f, 1.0f, 1.0f }, H_FIRE },
    [MOB_DEACON] = { "DEACON", "DEACONS", "A PRIEST OF THE DEAD, AND THEY GET UP WHEN HE ASKS. KILL HIM FIRST.",
        SP_DEACON_A, SP_DEACON_B, SP_DEACON_SA, SP_DEACON_SB, PF_ROBE, 18, RK_V, 1, KF_UNDEAD | KF_SOLID | KF_PAGE | KF_DOORS, SFX_BONES, SP_DEACON_D,
        40, 0.9f, 1.05f, 0.42f,                         { 1.5f, 1.0f, 1.0f, 1.0f, 0.5f, 1.0f }, 0 },
    [MOB_GARGOYLE] = { "GARGOYLE", "GARGOYLES", "IT IS ONLY A STATUE UNTIL YOU PASS. LIGHTNING RUNS OFF IT; A SPEAR CHIPS IT.",
        SP_GARG_A, SP_GARG_B, SP_GARG_SA, SP_GARG_SB, PF_STONE, 19, RK_V, 0, KF_PAGE | KF_MELEE, SFX_SHATTER, SP_GARG_D,
        45, 2.2f, 0.9f, 0.42f,                          { 1.0f, 1.0f, 0.5f, 1.0f, 1.0f, 1.5f }, H_PHYS },
    /* the new kinds made for the minds' roles (1.0.0b15, docs/PLAN-ai.md section 22) */
    [MOB_WHIPPER] = { "WHIPPER-IN", "WHIPPERS-IN", "THE ORDER'S HUNTSMAN. HIS HORN SETS HIS HOUNDS ON YOU; FELL HIM AND THEY RUN WILD.",
        SP_WHIP_A, SP_WHIP_B, SP_WHIP_SA, SP_WHIP_SB, PF_BONE, 22, RK_V, 1, KF_UNDEAD | KF_SOLID | KF_PAGE | KF_DOORS, SFX_BONES, SP_WHIP_D,
        36, 1.0f, 1.15f, 0.42f,                         { 1.0f, 1.0f, 0.5f, 1.0f, 1.5f, 1.0f }, H_PHYS },
    [MOB_SEXTON] = { "SEXTON", "SEXTONS", "HE RINGS AT EACH TURN OF HIS ROUND, AND HARD WHEN HE SEES YOU. FROST STOPS THE BELL.",
        SP_SEXTON_A, SP_SEXTON_B, SP_SEXTON_SA, SP_SEXTON_SB, PF_HIDE, 26, RK_V, 1, KF_UNDEAD | KF_SOLID | KF_PAGE | KF_DOORS, SFX_GROAN, SP_SEXTON_D,
        30, 0.8f, 1.0f, 0.42f,                          { 1.0f, 1.0f, 1.0f, 1.5f, 0.5f, 1.0f }, 0 },
    [MOB_LIGHTLESS] = { "LIGHTLESS", "LIGHTLESS", "BLACK GLASS, NO EYES. IT FEELS LIGHT AND COMES TO IT, SILENT. SHUTTER THE LANTERN.",
        SP_LIGHT_A, SP_LIGHT_B, SP_LIGHT_SA, SP_LIGHT_SB, PF_CHITIN, 32, RK_V, 1, KF_SOLID | KF_PAGE | KF_DOORS | KF_MELEE, SFX_SHATTER, SP_LIGHT_D,
        50, 1.3f, 1.2f, 0.40f,                          { 1.0f, 1.0f, 1.0f, 1.0f, 0.5f, 1.5f }, H_PHYS },
    /* the mice grown (docs/PLAN-mice.md): living, so no Dread or Grave Call; no doors, as the mice could open none */
    [MOB_RAT] = { "CHARNEL RAT", "CHARNEL RATS", "THE MICE AT THE TORCHES, GROWN ON WHAT THE CRYPT FEEDS THEM. A SMALL ONE FETCHES THEM.",
        SP_RAT_A, SP_RAT_B, SP_RAT_SA, SP_RAT_SB, PF_HIDE, MOUSE_DEBUT, RK_V, 1, KF_SOLID | KF_PAGE | KF_MELEE, SFX_RAT_SQUEAL, SP_RAT_D,
        16, 1.8f, 0.62f, 0.32f,                         { 1.5f, 1.0f, 1.0f, 0.5f, 1.0f, 1.0f }, H_PHYS },
};
_Static_assert(MOB_RAT < 24, "the record keeps 24 kinds (REC_KINDS), and the charnel rat is the last of them");
#define NKINDS ((int)(sizeof KINDS / sizeof KINDS[0]))
#define KIND_IS(m, f) (KINDS[(m)->type].flags & (f))
/* the ladder: a rank means about the same strength at any depth; depth shows in which ranks come */
static const float RANK_HP[NRANK] = { 1.0f, 1.6f, 2.4f, 3.5f, 5.0f, 3.0f };
static const float RANK_DMG[NRANK] = { 1.0f, 1.15f, 1.3f, 1.5f, 1.7f, 1.2f };
static const float RANK_SPEED[NRANK] = { 1.0f, 1.05f, 1.1f, 1.15f, 1.15f, 1.0f };
static const float RANK_SOULS[NRANK] = { 0.5f, 1.0f, 1.5f, 2.0f, 3.0f, 3.0f };   /* a kill's, as a mote (1.0.0b11: about half) */
static const float RANK_SIZE[NRANK] = { 1.0f, 1.04f, 1.08f, 1.12f, 1.16f, 1.08f };   /* a second cue, for eyes that mix up the colours */
static const char *const RANK_WORD[NRANK] = { "", "ROTTEN ", "COLD ", "BLOODY ", "BLACK ", "GILDED " };

