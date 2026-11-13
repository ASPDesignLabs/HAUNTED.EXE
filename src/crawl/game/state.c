/* A part of scene_crawl.c (docs/PLAN-tidy.md): it is #included there in its place and never compiled on its own.
 * The bosses' arenas and BOSSES; hazards, things, puzzles and trials as structs; shots, secrets, items; the levels' names; the record, a stone, the Alignment's lines; RUN; the console's and the ledger's state; G. */

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
enum { HZ_ROCK, HZ_FIRE, HZ_BILE, HZ_VOID, HZ_ARC, HZ_GRASP,
       /* the runs' own (1.0.0b13, docs/PLAN-b13.md section 5): the Crypts', the Catacombs', the Hollows', the Deep's. Saved
          as a bit each (RUN.omens): new ones at the end */
       HZ_CENSER, HZ_BREATH, HZ_VOTIVE, HZ_COLD, HZ_RELIQ, HZ_SLUICE, HZ_GALV, HZ_SNARE, HZ_SILT, HZ_SAINT,
       HZ_TIMBER, HZ_DAMP, HZ_CART, HZ_LODE, HZ_TITHE, HZ_FISSURE, HZ_ASH, HZ_CRYSTAL, HZ_GLASS, HZ_DCOLD, HZ_MIRROR, NHZ };
#define MAXHAZ 48
#define MAXSITE 12                          /* the fixed ones on a level (1.0.0b13) */
#define HZ_CELLS 24
#define MAXOUT 6                            /* a boss's outlets for its bile */
typedef struct {
    int alive, kind, omen, hit, n, src, wx, wy, wtex;   /* src: the outlet it pours from; wx, wy, wtex: an omen's grate and the wall it was */
    float x, y, r, ang, t, warn, on, off, share;
    int8_t cx[HZ_CELLS], cy[HZ_CELLS]; uint8_t cd[HZ_CELLS];
    float pre;                                                     /* Premonition's second, before its own warning */
    int site; uint64_t hitmask;                                    /* the fixed one it is a turn of (-1: none); the creatures it has struck */
} Hazard;
/* a fixed hazard (1.0.0b13): where it stands (a wall cell and the point on its face, for what is set in a wall), its
 * cells, its clock and state, its turn now (a hazard), whether it is drinking from you and what it has taken */
typedef struct {
    int kind, x, y, d, wx, wy, n, state, haz, drink;
    float t, wait, k, took, hit_t, ax, ay;
    int8_t cx[HZ_CELLS], cy[HZ_CELLS]; uint8_t cd[HZ_CELLS];
} HzSite;
/* a thing a puzzle lays out (1.0.0b13, docs/PLAN-b13.md section 6): what it is (TR_*), drawn as a sprite (one frame a state
 * when it has frames) with a mark carved on it, or on the floor; an item sits in a place (in), and was taken from one (home)
 * or from the floor (hx, hy); a place holds an item so high (hold); its value (a mark, a weight, a note) and what it wants */
enum { TR_ITEM, TR_PLACE, TR_TURN, TR_LEVER, TR_LIGHT, TR_STRIKE, TR_PLATE, TR_READ };
enum { TU_TAKE, TU_SET, TU_TURN, TU_PULL, TU_LIGHT, TU_STRIKE, TU_STEP, TU_READ };   /* how a thing was touched */
#define MAXTHING 64
/* the puzzles (1.0.0b13, docs/PLAN-b13.md section 7): the Crypts', the Catacombs', the Hollows', the Deep's, and two of any run */
enum { PZ_LEDGER, PZ_KNELL, PZ_EFFIGY, PZ_BALANCE, PZ_VOTIVES, PZ_URNS, PZ_HOURS,
       PZ_CISTERNS, PZ_SAINTS, PZ_CULVERTS, PZ_FERRY, PZ_SLUICELOCK, PZ_DBELLS,
       PZ_POINTS, PZ_HOIST, PZ_TALLY, PZ_FOSSIL, PZ_RING, PZ_LAMPS,
       PZ_PRISMS, PZ_COURT, PZ_GLASS, PZ_CRYSTALS, PZ_VEINS, PZ_VENTS,
       PZ_SQUARE, PZ_LETTERS, NPZK };
#define WINGED 4                            /* a Door's lock: the sealed wing's, until its puzzle is solved */
#define TRIALED 5                           /* a Door's lock: a trial's, while it runs */
#define GATED 6                             /* a Door's lock: a winch's or a hearth's gate, while it is down (1.0.0b16) */
/* the trials (1.0.0b13, docs/PLAN-b13.md part D): the Crypts', the Catacombs', the Hollows', the Deep's */
enum { TRI_OSSUARY, TRI_VIGIL, TRI_RITES, TRI_WAGER, TRI_TOLL, TRI_WATER, TRI_BROOD, TRI_SLUICES, TRI_LAMPS,
       TRI_CAVEIN, TRI_FEAST, TRI_RITE, TRI_FIREDAMP, TRI_MOLTEN, TRI_DARK, TRI_COURT, TRI_SINGING, NTRI };
enum { TRS_SHUT, TRS_ON, TRS_WON, TRS_LOST };   /* not yet gone into; running; won; its sand run out */
#define TRI_TIME 60.0f
#define TRI_IN 1.5f                         /* how far past its door's middle a trial begins, the body clear of the door
                                               (1.0.0b18.1: one cell in, it had shut on a person half in the doorway) */
#define TRI_REWARD 100                      /* an Item's color: a won trial's hoard or draught (1.0.0b18.1) */
#define TOLL_T 9.0f                         /* the Toll's great bell, this often */
typedef struct {
    uint8_t role, nstate, state, wall, on, frames, cap, facing, look, tri, slow;   /* cap: a place that piles more than one; facing: drawn by
                                                                     which way it points from where you stand */
    int16_t spr, mark, ramp;
    int8_t in, home, below;                                        /* below: in a pile, the item under it */
    int cx, cy, val, want, grp;
    float x, y, z, hold, scale, t, hx, hy, mz;
    char name[40], text[160];
} Thing;
static int boss_sealed(void);
static int boss_fighting(void);
static int in_arena(float x, float y);
static void boss_spawn(void);
static void update_boss(float dt);
static int font_drink(int i);
static Mob *boss_mob(void);
static void hz_clear(void);

typedef struct { int alive, kind, target; float x, y, z, vx, vy, life, dmg; uint64_t pierced; int tier; float arc; uint16_t mkhit; float scale; } Shot;   /* tier: the rank
                                                                                 it was cast at; arc: a lob's whole flight (the Blight Flask), 0 straight;
                                                                                 scale (1.0.0b21): its size, 0 as drawn (a held bolt grown, an augment's) */
/* The augments' hooks (17.3, 1.0.0b21): called where a line's spell is cast, flies, strikes, burns and kills, each handing the line
 * to its own track's handler (Fire's the caster's track's; the other five lines' D7's, augd_* in the dead's track's marked block).
 * With no augment chosen they change nothing, so a line without one plays as before.
 *   aug_cost(sp)               what the line's augments add to the spell's spirit, a share (0.3 is 30% more; A7: they add up)
 *   aug_dmg(sp)                a multiplier on the spell's damage as it is cast (cast_spell, cast_charged)
 *   aug_cast(sp, s, t)         a shot just made by the line's spell (s NULL for lightning and the drain, after they strike; t the
 *                              target or -1); it may change the shot (dmg, speed, life, scale) and make more
 *   aug_hit(m, atk, flags, d)  a hit in damage_mob, after its weaknesses and before its health; it may change *d
 *   aug_af(m, atk, q)          a multiplier on the status the hit rolls in el_status: AUGQ_CHANCE its chance, AUGQ_HARM its harm
 *   aug_kill(m, atk, flags)    a creature killed by the line (flags as damage_mob had them: DM_TICK a burn's or poison's tick);
 *                              the caster's track banks the line's credit here too
 *   aug_tick(m, dt)            each creature, each step, from mob_afflict (a burning that spreads, a bleed)
 *   aug_fly(s, dt)             each shot in flight, from update_shots (a trail)
 * The same with the line first (augd_*, e the line) are the dead's track's for its five; augd_built(e) says which of its lines'
 * augments are built, so the Lampstand offers them */
static float aug_cost(int sp);
static float aug_dmg(int sp);
static void aug_cast(int sp, Shot *s, int t);
static void aug_hit(Mob *m, int atk, int flags, float *dmg);
static float aug_af(const Mob *m, int atk, int q);
static void aug_kill(Mob *m, int atk, int flags);
static void aug_tick(Mob *m, float dt);
static void aug_fly(Shot *s, float dt);
static float augd_cost(int e, int sp);
static float augd_dmg(int e, int sp);
static void augd_cast(int e, int sp, Shot *s, int t);
static void augd_hit(int e, Mob *m, int atk, int flags, float *dmg);
static float augd_af(int e, const Mob *m, int atk, int q);
static void augd_kill(int e, Mob *m, int atk, int flags);
static void augd_tick(Mob *m, float dt);
static void augd_fly(int e, Shot *s, float dt);
static int augd_built(int e);
__attribute__((unused)) static const char *augd_name(int e, int t, int w);   /* D's (D7's contract, 9 November): the name of
                                                                      augment w (1, 2) at step t of line e, one of the dead's track's
                                                                      five; "" for Fire or none (unused till C6's Lampstand reads it) */
__attribute__((unused)) static void augd_does(int e, int t, int w, float k, char *buf, int n);   /* and what it does at a rank's
                                                                      share k (0.5, 0.75, 1; 0 as at rank III), its numbers in it */
static float dk(int e, int t, int w);                              /* D7's own, not the contract: augment w at step t of line e, its
                                                                      rank's share (aug_k, or CRAWL_AUGD in the debug build) */
typedef struct { int alive, kind; float x, y, vx, vy, life, spin, dmg, life0; int from, atk; Blame by; int el1; } Bone;   /* life0: an ember's whole flight; from: its thrower;
                                                                                                    atk: a turned spell's kind;
                                                                                                    el1: 1 + its element (PLAN-combat) */
typedef struct { int alive, type, cx, cy, seen, color, lx, ly, secret, gen; float x, y, phase; int value; } Item;   /* lx, ly: the lane cell it is taken from;
                                                                                                   gen: 1 + its place among a pocket's things (0: not a pocket's) */
typedef struct { int alive; float x, y, t, r; int big; } Boom;           /* big 2: the Blight Flask's green splash */
typedef struct { int alive, kind; float x0, y0, z0, x1, y1, z1, t; } Beam;   /* lightning and soul drain, for a moment */
typedef struct { int alive; float x, y, t, dps; int foe; float life; } Burn;   /* fire on the floor; foe 1: an ember's, which burns the caster;
                                                                                 2: Firestarter's, which sets creatures alight */
typedef struct { int alive; float x0, y0, x1, y1, t, dmg; uint64_t hit; } Trail;   /* Ionization: lightning's trail, hanging */
typedef struct { int alive; float x, y, r, t, life, dose, slow; int rime; } Venom;   /* Seep: a pool of venom where a flask burst;
                                                                                 rime: Winter's Lance's frost on the floor */
typedef struct { int alive; float x, y, t; int feat; } Pool;              /* what a drowned one leaves: foul, a few seconds; feat: it
                                                                              hasn't reached you yet (the portraits') */
#define MAXPOOLS 4
typedef struct { int deco, type, rank; float t; } Remains;         /* what something you slew left: for Grave Call to raise */
#define MAXREMAINS 12
typedef struct { int x, y, w, h, region, theme, rtype; } Room;   /* rtype: its run's room type, RT_* */
/* stand at f, press d: the wall at s slides len cells back into its room (x0..x1, y0..y1). links: the chain
 * it begins (1, a room alone); door: the doorway at the back of its room into the pockets (x -1: none) */
typedef struct { int fx, fy, d, sx, sy, tex, state, rolled, len, links, door_x, door_y, x0, y0, x1, y1; uint32_t seed; int sealed; } Secret;   /* seed: its pockets'; sealed: a trial failed in them */
#define MAXSECRETS 6
#define SLIDE_TIME 2.3f                     /* how long a secret wall takes to slide right through its room */
#define MAXWISPS 2                          /* the one you summon, and the Black Altar's */
#define WISP_MOTES 192                      /* the trail a wisp leaves hanging in the air */
#define WISP_MOTE_T 12.0f                   /* how long a mote of it hangs there */
#define WISP_PACE (1.5f * WALK)             /* how fast it goes on ahead */
#define WISP_COST 1.0f                      /* a soul a wisp */
enum { WK_EXIT, WK_KEY, WK_ALTAR, WK_MEND };   /* where a wisp leads: the way out, a key, the altar's secrets; a place to mend
                                               (the lessons', docs/PLAN-tutorial.md 8.5) */
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
#define MAXMOBS 64                          /* 1.0.0b15: room for what is raised, hatched and called (docs/PLAN-ai.md, 27) */
#define MAXSHOTS 16
#define MAXBOOMS 16                         /* doubled in 1.0.0b15: the dream's runs filled eight (docs/PLAN-ai.md, 27) */
#define MAXBONES 10                         /* the dead's missiles: kept at ten, part of the balance (their throws refused when full) */
#define MAXITEMS 96
#define MAXBEAMS 8
#define MAXBURNS 40
#define MAXTRAILS 12
#define MAXVENOM 40
#define MAXMOTES 24
typedef struct { int alive; float x, y, t, souls, fly; } SoulMote;   /* a soul where it fell: six seconds, taken within 1.2 cells;
                                                                      fly: Shepherd's (D7), coming to you so many cells a second */
#define MOTE_LIFE 6.0f
#define MOTE_REACH 1.2f
enum { CXM_SAP = 1 };                       /* RUN.cx_more: what else the Codex has met (1.0.0b11): Soul Sap */
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
#define POR_GEMS 8                          /* a portrait's gems (docs/PLAN-portraits.md): its ranks I to V and gilded (RK_*), its
                                               feat (POR_FEAT), one spare */
#define POR_FEAT 6
_Static_assert(POR_FEAT == PORTRAIT_FEAT, "the feat's gem is the art's");
enum { POR_CLEARED = 1, POR_WOKEN = 2 };   /* her room pays the portraits' gold this life: a level cleared in it, or woken into it */
#define ACH_BITS 512                        /* 256 until version 6 */
typedef struct {
    uint16_t kills[REC_KINDS][NRANK];
    uint8_t el_kills[REC_KINDS][NELEM];     /* slain by each element, to 255 (version 7; by spell before) */
    uint8_t el_hits[REC_KINDS][NELEM];      /* its hits landed on each kind, to 255: five kills or twelve hits, and known */
    uint8_t harms[REC_KINDS][NELEM];        /* times each kind has hurt you with each element: three, and known */
    uint8_t seen[REC_KINDS];                /* the ranks of each kind met */
    uint16_t secrets, descents;             /* secrets found; times all the way down (40, 80, 120 ...) */
    uint64_t cleared;                       /* levels 1 to 40 cleared, a bit each */
    uint8_t got[ACH_BITS / 8];              /* achievements earned, a bit each (see ach_bit) */
    struct { uint8_t met; uint16_t slain, tries, best, scrip; } boss[BOSS_REC];   /* version 6: each boss met, slain, tried, its
                                                                                 best time in seconds (0: none yet), scrip won */
} Record;
#define REC_SPELLS 9                        /* the spells the record counted kills by until version 7 (NATK then) */
#define REC_BYTES (REC_KINDS * NRANK * 2 + REC_KINDS * REC_SPELLS + REC_KINDS + 2 + 2 + 4 + 256 / 8)   /* as packed since version 2 */
#define REC_ELEM_BYTES (3 * REC_KINDS * NELEM)   /* version 7's LG_ELEM: the record by element */
#define REC_EXTRA (4 + (ACH_BITS - 256) / 8 + BOSS_REC * 9)   /* version 6: levels 33 to 40 cleared, the achievements past the 256th, the bosses */

/* A stone in the merchant's grave plot: a person's death, when, where and by what, and what was
 * carried (docs/PLAN-b7.md, section 1). The last ten are kept, newest first. */
#define STONES 10
typedef struct {
    int64_t when;
    int level, gold, souls, coins;
    Blame by;
    int8_t atk[NATK], def[NDEF];            /* the lines' ranks in their spells' places (1.0.0b11), the runes */
    Piece worn[NSLOT], bag[BAG_MAX];
    uint8_t bag_size, bag_quality;
    int lined, cls; uint8_t path[NELEM];    /* written since the Alignment: the class, each line's path */
} Stone;

/* A line of the Alignment (docs/PLAN-b11.md, sections 8-10): its opening, Efficiency, Insight, its ability, then
 * one of two paths of three nodes, the last its ultimate */
enum { ND_OPEN, ND_EFF, ND_INS, ND_ABIL, ND_A1, ND_A2, ND_AULT, ND_B1, ND_B2, ND_BULT, NNODE };

/* What points buy (docs/PLAN-b9.md, section 5): a line each; Resist buys every resist at once */
enum { LN_HEALTH, LN_DAMAGE, LN_RESIST, LN_POISON, LN_BURN, LN_SLOW, LN_HASTE, LN_DODGE, LN_TREASURE, LN_MOVE, LN_LUCK, LN_ATTUNE,
       LN_SPIRIT, LN_LIGHT, LN_SPRINT, LN_PHYSIC, NLINE };   /* saved by number: a new line goes at the end (Resist Lightning,
                                                                1.0.0b13; Sprint and Physic, 1.0.0b15) */
#define NLINE_B9 12                         /* the lines a page held before Resist Spirit's */
/* the cornerstones (section 6): Second Wind, Premonition, Cartographer, Lantern of the Dead, Grave Sense */
enum { CS_WIND, CS_OMEN, CS_CARTO, CS_LANTERN, CS_SENSE, NCS };

/* The run: what carries from level to level, through death and across scene switches */
#define COMP_MAX 12                         /* the Compendium keeps so many of each kind at each quality */
#define REPO_MAX 12                         /* the Repository's places, at the most */
#define SHRINE_SLOTS 9
#define SHRINE_X 15.5f
#define SHRINE_Y 17.5f                      /* the Rune Shrine, in the merchant's room against the south wall */
#define TOTEM_X 4.5f
#define TOTEM_Y 12.5f                       /* the Hexed Totem, in the back room's corner */
#define LECTERN_X 6
#define LECTERN_Y 15                        /* the Corpus Compendium, in the back room's middle */
#define PORTAL_X 16
#define PORTAL_Y 27                         /* the Accursed Portal, in the grave plot's far wall */
#define BELL_X 23
#define BELL_Y 5                            /* the Dead Bell, in the east half of the room north of hers (1.0.0b11) */
#define FONT_X 17
#define FONT_Y 5                            /* the Weeping Font, in its west half */
#define LAMP_X 20
#define LAMP_Y 3                            /* the Lampstand (1.0.0b11), against the far wall of the room north of hers */
static const int8_t STONE_AT[10][2] = { { 20, 22 }, { 18, 22 }, { 16, 22 }, { 14, 22 }, { 12, 22 },   /* the plot's stones, */
                                        { 20, 24 }, { 18, 24 }, { 16, 24 }, { 14, 24 }, { 12, 24 } };   /* the newest nearest the arch */
#define MAXMK 12
enum { MKK_PRICKET, MKK_RACK, MKK_COFFER, MKK_CAGE, MKK_URN, MKK_COIL, MKK_VINE, MKK_LANTERN, MKK_BARRICADE,
       /* step 6 (section 4.3, from level 5, and the runs' own, 4.6) */
       MKK_CLIGHT, MKK_RSKULL, MKK_HSEAL, MKK_BELL, MKK_WINCH, MKK_HEARTH, MKK_VENT, MKK_WEB, MKK_CART, MKK_LODE, MKK_PRISM, MKK_GEODE,
       /* step 8: sequences (section 4.4, from level 13) and things for fights (4.5, from 5) */
       MKK_TABLET, MKK_PLINTH, MKK_RELIQ, MKK_WSTONE, MKK_RELAY, MKK_CASK, MKK_PILLAR, MKK_PFONT, MKK_DRUM, MKK_WATER, MKK_ICEF, MKK_DAMP,
       MKK_RSEAL, MKK_GSEAL,                                    /* the tutorial's level 2 (docs/PLAN-tutorial.md 7.2, 7.3): any spell */
       NMKK };
typedef struct {
    int kind, el, pips, got, done, deco, tried, said;   /* said: 1 named, 2 its want told, 4 first answered; done 2: lost */
    int st, grp, door, times;               /* st: its state (hot, rolling, up, iced); grp: 1 + a bell's first; door: a gate's */
    int els[3];                             /* step 8: a tablet's order, a reliquary's faces; an ice floor's first decoration */
    float x, y, hit_t, wrong_t, done_t, t, hx, hy, vx, vy;   /* t: its clock; hx, hy where it keeps to; vx, vy the way it goes */
} Marked;
typedef struct { int s0, s1, el, pips, sx, sy, dx, dy; const char *name, *done; int tier, runs, own; } MkKind;
static const MkKind MKK[NMKK] = {                                   /* el -1: any line held; sx, sy the seal on the sprite, dx, dy once done;
                                                                       tier 2 from level 5; runs, own: a bit a run (I 1, II 2, III 4, IV 8) */
    { SP_MK_PRICKET0, SP_MK_PRICKET1, EL_FIRE, 1, 32, -6, 32, -6, "A COLD PRICKET", "THE PRICKET BURNS", 1, 15, 1 },
    { SP_MK_RACK0, SP_MK_RACK1, EL_FIRE, 3, 32, 22, 32, 22, "A VOTIVE RACK", "THE CANDLES BURN", 1, 15, 1 },
    { SP_MK_COFFER0, SP_MK_COFFER1, -1, 3, 32, 44, 32, 50, "A BOUND COFFER", "THE CHAINS FALL AWAY", 1, 15, 1 },
    { SP_MK_CAGE0, SP_MK_CAGE1, -1, 1, 32, 8, 32, 46, "A HANGING CAGE", "THE CAGE COMES DOWN", 1, 15, 1 },
    { SP_MK_URN0, SP_MK_URN1, EL_FROST, 2, 32, 14, 32, 14, "THE KEEPER'S URN", "THE COALS GO OUT", 1, 15, 2 },
    { SP_MK_COIL0, SP_MK_COIL1, EL_LIGHT, 2, 32, 58, 32, 58, "A GALVANIC COIL", "THE COIL WAKES", 1, 15, 4 },
    { SP_MK_VINE0, SP_MK_VINE1, EL_POISON, 2, 32, 30, 32, 30, "A KNOT OF ROOTS", "THE ROOTS WITHER", 1, 15, 2 },
    { SP_MK_LANTERN0, SP_MK_LANTERN1, EL_SPIRIT, 1, 38, 48, 38, 48, "A BOUND SOUL", "THE SOUL GOES FREE", 1, 15, 1 },
    { SP_MK_BARRICADE0, SP_MK_BARRICADE1, EL_PHYS, 3, 32, 30, 32, 52, "A BONE BARRICADE", "THE BARRICADE COMES DOWN", 1, 15, 1 },
    { SP_MK_CLIGHT0, SP_MK_CLIGHT0, -1, 1, 32, 14, 32, 14, "A CORPSE-LIGHT", "THE LIGHT BREAKS", 2, 15, 0 },
    { SP_MK_RSKULL0, SP_MK_RSKULL0, -1, 1, 32, 40, 32, 40, "A ROLLING SKULL", "THE SKULL BREAKS", 2, 15, 0 },
    { SP_MK_HSEAL0, SP_MK_HSEAL2, EL_FIRE, 1, 32, 24, 32, 24, "A STONE SEAL", "THE SEAL SHATTERS", 2, 15, 8 },
    { SP_MK_BELL0, SP_MK_BELL1, -1, 1, 32, 40, 32, 40, "THREE BELLS", "THE BELLS RING TOGETHER", 2, 15, 1 },
    { SP_MK_WINCH0, SP_MK_WINCH1, EL_LIGHT, 1, 32, 50, 32, 50, "A PORTCULLIS WINCH", "THE GATE RISES", 2, 15, 3 },
    { SP_MK_HEARTH0, SP_MK_HEARTH1, EL_FIRE, 4, 32, 22, 32, 22, "A DYING HEARTH", "THE DOOR IS UP", 2, 15, 1 },
    { SP_MK_VENT0, SP_MK_VENT1, EL_FROST, 1, 26, 50, 26, 50, "A STEAM VENT", "THE VENT ICES OVER", 2, 15, 2 },
    { SP_WEBSHEET, SP_WEBSHEET, EL_FIRE, 2, 32, 30, 32, 30, "A WEBBED NICHE", "THE WEB BURNS AWAY", 2, 2, 2 },
    { SP_MK_CART0, SP_MK_CART1, -1, 1, 32, 20, 32, 20, "AN ORE CART", "ITS WHEEL BREAKS", 2, 4, 4 },
    { SP_MK_LODE0, SP_MK_LODE1, EL_LIGHT, 3, 32, 18, 32, 18, "A LODESTONE", "THE LODESTONE IS CHARGED", 2, 4, 4 },
    { SP_PZ_PRISM0 + 2, SP_PZ_PRISM0 + 2, EL_LIGHT, 3, 32, 16, 32, 16, "A PRISM", "THE PRISM BURNS", 2, 8, 8 },
    { SP_PZ_GEODE, SP_PZ_GEODE1, EL_PHYS, 2, 32, 24, 32, 24, "A GEODE", "THE GEODE CRACKS", 2, 8, 8 },
    /* tier 3 the sequences, from level 13; tier 4 for fights, placed in rooms by place_fights; 9 never chosen alone */
    { SP_MK_TABLET, SP_MK_TABLET, -1, 3, 32, 28, 32, 28, "A CARVED TABLET", "THE ORDER IS KEPT", 3, 15, 0 },
    { SP_MK_PLINTH0, SP_MK_PLINTH1, -1, 1, 32, 16, 32, 16, "A PLINTH", "", 9, 15, 0 },
    { SP_MK_RELIQ0, SP_MK_RELIQ2, -1, 3, 32, 32, 32, 32, "A TURNING RELIQUARY", "THE RELIQUARY OPENS", 3, 15, 0 },
    { SP_MK_WSTONE0, SP_MK_WSTONE1, -1, 5, 32, 30, 32, 30, "A WARD-STONE", "THE WARD BREAKS: THE DOOR IS UP", 3, 15, 0 },
    { SP_MK_RELAY0, SP_MK_RELAY1, EL_FIRE, 1, 32, 21, 32, 21, "A DEAD LAMP", "THE LAMPS BURN", 3, 15, 1 },
    { SP_MK_CASK0, SP_MK_CASK1, EL_FIRE, 1, 32, 22, 32, 50, "LAMP OIL", "THE OIL BURNS", 4, 15, 0 },
    { SP_MK_PILLAR0, SP_MK_PILLAR1, EL_PHYS, 2, 32, 24, 32, 46, "LOOSE MASONRY", "THE COLUMN COMES DOWN", 4, 15, 0 },
    { SP_MK_PFONT0, SP_MK_PFONT1, EL_POISON, 1, 32, 14, 32, 14, "A DRY FONT", "THE FONT BRIMS WITH BILE", 4, 15, 0 },
    { SP_MK_DRUM0, SP_MK_DRUM1, EL_SPIRIT, 1, 32, 34, 32, 34, "THE ORDER'S DRUM", "THE DRUM IS SILENCED", 4, 15, 1 },
    { -1, -1, EL_LIGHT, 1, 32, 50, 32, 50, "STANDING WATER", "THE WATER CRACKLES", 4, 2, 2 },
    { -1, -1, EL_FROST, 1, 32, 50, 32, 50, "A COLD FLOOR", "THE FLOOR RIMES OVER", 4, 15, 0 },
    { -1, -1, EL_FIRE, 1, 32, 40, 32, 40, "FIREDAMP", "THE FIREDAMP GOES UP", 4, 4, 4 },
    /* the tutorial's, laid only by its drawing (tier 9): the switch that holds the ice, and the one-way gate's seal */
    { SP_MK_RSEAL0, SP_MK_RSEAL1, -1, 3, 32, 32, 32, 32, "A RIMED SEAL", "THE ICE DRAINS AWAY", 9, 0, 0 },
    { -1, -1, -1, 1, 32, 34, 32, 34, "THE GATE'S SEAL", "THE GATE RISES", 9, 0, 0 },
};
#define MK_FROM 5                           /* section 4.3's things from this level of a run */
#define BELL_RING 6.5f                      /* a bell rings this long: all three at once to answer them (a spell comes round
                                               every two seconds or so, so the plan's two seconds were the spear's alone) */
#define GATE_UP 8.0f                        /* the winch's portcullis stays up so long */
#define HSEAL_HOT 4.0f                      /* the heated seal's glow */
#define VENT_CYCLE 5.0f                     /* the steam vent: quiet, a hiss (VENT_WARN), then scalding (VENT_ON) */
#define VENT_WARN 2.7f
#define VENT_ON 3.5f
#define VENT_ICE 15.0f                      /* iced so long */
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
    int deed_pz, deed_tri, deed_lost;       /* 1.0.0b13's record: puzzles solved, trials won and lost; the kinds of each, */
    uint32_t pz_kinds, tri_kinds;           /* a bit each (saved in LG_DEEDS, kept through death) */
    uint8_t abil[BOSS_REC];                 /* each boss's ability for the caster, from the Hexed Totem: its rank, 0-3 */
    int abil_sel;                           /* the one the right mouse uses (a boss's number) */
    int run_top;                            /* the furthest run opened at the Accursed Portal: its boss's level over ten */
    int chal_from;                          /* in a challenge: the level whose merchant's room it was begun from */
    uint32_t run_id, signs;                 /* the ledger's: whose run this is (born at a fresh start, kept through
                                               death), and how many times its page has been written */
    Stone stones[STONES]; int nstones;      /* the grave plot, newest first */
    int peds;                               /* the merchant's pedestals, two to seven, the rest bought from her */
    Piece comp[NIK][NQUAL][COMP_MAX];       /* the Corpus Compendium: each piece as first worn, by kind and quality, */
    uint8_t ncomp[NIK][NQUAL];
    Piece repo[REPO_MAX];                   /* the Wretched Repository: what lies in each place, */
    int repo_n, repo_gold, repo_coins;      /* the places bought (two at first, the first untaxed), its gold and coins */
    uint32_t cx_abil, cx_rune, cx_exq; uint64_t cx_stat;   /* the Codex: what is known, a bit each (hazards are RUN.omens) */
    uint32_t cx_more;                       /* and what else has been met (1.0.0b11): CXM_* bits */
    /* the caster's own (docs/PLAN-b9.md): the merchant's map, kept through death; and, gone with a death, the
       Level of Atonement claimed, the experience earned since the last death (all of it, not only what is
       over), the points not yet spent and the steps bought in each line, the overlevels chosen in each, the
       cornerstones held (a bit each, CS_*), Second Wind spent, and the run it last came back on */
    int has_map;
    int lantern_off;                        /* the Lantern of the Dead shuttered (1.0.0b15, docs/PLAN-ai.md, 26): flag 2 beside the map's */
    uint8_t al[NELEM][NNODE];               /* the Alignment (docs/PLAN-b11.md, section 8): the points in each line's nodes */
    int al_points;                          /* Alignment Points not spent: one a Level of Atonement */
    int cls, cls_pts, cls_spent;            /* the class (docs/PLAN-b11.md, section 11): 1 + its element, 0 not yet asked;
                                               the Witchfinder's points for Fire, and those of them spent there */
    uint8_t al_gift;                        /* lines opened by a class given up: a bit each, their opening never paid */
    int gifts;                              /* the Lampstand's gifts taken this life (1.0.0b16, docs/PLAN-play.md section 3), */
    uint8_t gifted;                         /* and the lines they opened, a bit each: their opening never paid, nor lost to unbinding */
    int bell_cursed, font_cursed;           /* the Dead Bell corrupted, the Weeping Font cursed: a coin each, once, kept through death */
    int bell_next, bell_secrets, font_next; /* the level the bell has corrupted (and its secrets shown), the font sweetened; 0 none */
    int replay_from;                        /* a level replayed through the Portal: the level whose room it was begun from; 0 none */
    int latch_said;                         /* the bat's caption, once a life */
    int old_spells;                         /* read from a page older than the Alignment: said once, as she greets you */
    int atone, xp, points;
    uint16_t steps[NLINE]; uint8_t over[NLINE];
    uint8_t conf[NCONF], conf_over[NCONF];  /* 1.0.0b20: the points in each Confession (150 makes rank V), its overlevels past V */
    uint8_t aug[NELEM][NAUGT], aug_ord[NELEM][NAUGT];   /* 1.0.0b21, the augments (section 11): which of each step's pair is chosen
                                               (0 none, 1, 2) and in what order (1 the first chosen), its credits, and each line's bank of
                                               credits not yet spent (LG_AUG; a death that costs and an unbinding take them) */
    uint16_t aug_cr[NELEM][NAUGT], aug_bank[NELEM];
    int ap_overs, old_lines;                /* overlevels whose Alignment Point an old line already gave (paid back as overlevels to
                                               choose again); the old lines paid back as points, still to be said (not saved) */
    int corner, wind_spent, wind_run;
    unsigned amb_runs;                      /* the runs of ten whose first ambush has been sprung (not saved: named again after a load) */
    int perfect;                            /* the level just cleared was perfect: her hall plays the alley cat till one isn't (not saved) */
    uint32_t ways, way_said; int way_run;   /* 1.0.0b18, the minds shown: the dead's ways met (WY_*, a bit each), those named on this run
                                               of ten, and which run that is (LG_WAYS, kept through death) */
    int deed_amb, deed_shaken, deed_heads, deed_hush, deed_file;   /* ambushes come through, hunts shaken off, packs broken by their
                                               leader's fall, callers silenced before they called, files of the Order beaten untouched */
    int rune_sel, mouse_rune;               /* the rune RB raises, and whether the right mouse is the rune's or the gift's (not saved) */
    int splint_used; float splint_t;        /* splinters thrown and not yet grown back, and the growing (1.0.0b16; not saved:
                                               a load starts with all eight) */
    /* the portraits of the slain (docs/PLAN-portraits.md; LG_PORTRAIT): kept through death, the run each gem was unlocked in
       (1 + its set, 0 not yet) and the tapestries drawn back (a bit a kind); for this life, each portrait's gems whose gold is
       taken (a bit a gem), each boss's (a bit a deed), and whether her room pays yet (POR_CLEARED, POR_WOKEN) */
    uint8_t por_run[REC_KINDS][POR_GEMS];
    uint32_t por_drawn;
    uint8_t por_taken[REC_KINDS], por_btaken[BOSS_REC];
    int por_open;
    uint16_t por_feat[REC_KINDS];           /* kept through death: each kind's feat's deeds; the Spectre's laid to rest, the Wraith's
                                               times driven back (step 5; the spiderling's and the imp's are the record's) */
} RUN;
static int lantern_held(void) { return RUN.corner >> CS_LANTERN & 1; }   /* the Lantern of the Dead (docs/PLAN-ai.md, 26) */
static int lantern_lit(void) { return lantern_held() && !RUN.lantern_off; }

/* The caster's boss abilities (docs/PLAN-b7.md, section 8): one a boss, unlocked at the Hexed Totem with a
 * scrip of that boss and raised with more, to rank III. Swift Dodge (the knight's): tap a direction twice
 * and dart that way. Tangled Web (the spider's), Grave Call (the Deacon's) and Wraithwalk (the Wraith's)
 * are on the right mouse, the readied one, and Q readies the next. */
#define ABIL_TOP 3
static const char *const BA_NAME[NBOSS] = { "SWIFT DODGE", "TANGLED WEB", "GRAVE CALL", "WRAITHWALK" };
static const float BA_WAIT[NBOSS][ABIL_TOP] = { { 15, 7, 3 }, { 12, 9, 6 }, { 20, 20, 20 }, { 25, 18, 12 } };
static float ab_wait(int b) { return RUN.abil[b] ? BA_WAIT[b][clampi(RUN.abil[b], 1, ABIL_TOP) - 1] : 0.0f; }
static int ab_on_mouse(int b) { return b >= BOSS_SPIDER && b < NBOSS && RUN.abil[b] > 0; }
static int ab_readied(void) {                                      /* the right mouse's, or -1 */
    if (ab_on_mouse(RUN.abil_sel)) return RUN.abil_sel;
    for (int b = BOSS_SPIDER; b < NBOSS; b++) if (ab_on_mouse(b)) return b;
    return -1;
}

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

/* The merchant's ledger (docs/PLAN-b7.md, sections 2 and 3): three pages, written by signing with her, by
 * a death and by her claim, and by nothing else. What lasts only while the program runs: whether a person
 * has taken the run over (from then on it is theirs; until then it is the screensaver's dream, and nothing
 * it does is kept), and the page it was opened from or last signed onto (0: unsigned, a run the progress
 * holds itself). A page signed in her room keeps the visit too, so that opening it sets out the same wares. */
typedef struct {
    int level, ped_n, left, bought, takes, held;
    uint8_t gone[NPED], back[NPED];
    Offer offers[NPED];
} Visit;
static struct {
    int read, yours, page;
    int kept;                               /* the run came from the ledger as the program started: a person's, so the */
                                            /* dream makes none of its lasting choices for it (what it was, its lines) */
    int has_visit; Visit visit;             /* the page's visit, set out when its room is entered */
    int marked, claim_due;                  /* this run's page is marked in danger; one opened marked: her claim is due */
    float calm_t;                           /* how long the danger has been over, counting down to the mark's clearing */
    int lent;                               /* 1 + a line lent to the dream for a run with no class (an old page it may not
                                               choose for): never saved, ended by a class taken */
} LEDGER;

static struct {
    uint32_t rs;
    double t, level_t;
    int level, ptheme, vis_x, vis_y, vis_deco;                     /* a pocket's look (PT_*), PT_NONE on the level; its visportal or water cooler */
    int faced, look;                                                /* walls by face and floors textured: a drawn run's look, or her hall's; look: whose (crawl_set) */
    Cell map[MAPN][MAPN];
    Room rooms[12]; int nrooms;
    Door doors[MAXDOORS]; int ndoors;
    Light lights[MAXLIGHTS]; int nlights;
    uint16_t lc_idx[MAXLC]; float lc_w[MAXLC]; int nlc;
    float lm[LMN * LMN];
    Deco decos[MAXDECO]; int ndecos;
    uint8_t bulky[MAPN][MAPN];          /* cells holding something big to walk around */
    uint8_t foot[MAPN][MAPN];               /* a prop's footprint (1.0.0b18.1): the quarters of a K_PROP cell it takes, 1 -x -y, 2 +x -y,
                                               4 -x +y, 8 +x +y; 0 the whole cell, as every prop before */
    float fog[MAPN][MAPN];                  /* fog banks, 0..1, thicker the deeper the level */
    uint8_t mapped[MAPN][MAPN];             /* the map: 1 its shape known, 2 been near it (docs/PLAN-b9.md); 4 seen, 8 walked (1.0.0b11) */
    Mob mobs[MAXMOBS];
    Shot shots[MAXSHOTS];
    Boom booms[MAXBOOMS];
    Bone bones[MAXBONES];
    Beam beams[MAXBEAMS];
    Burn burns[MAXBURNS];
    Trail trails[MAXTRAILS];
    Venom venoms[MAXVENOM];
    SoulMote motes[MAXMOTES];               /* souls left where the dead fell (docs/PLAN-b11.md, section 12) */
    float vslip_t; int full_said;           /* souls slipping from a full vessel, a pale wisp off the hand; said once a level */
    Pool pools[MAXPOOLS];
    Remains remains[MAXREMAINS]; int nremains;                     /* the newest remains, round and round */
    int wraith_done, wraith_now; float wraith_try;
    int spec_n; float spec_t;               /* spectres still to come this level (docs/PLAN-b11.md, SP2), and when the next drifts in */   /* the Wraith has come this level (or is called now); when to look for a place for it */
    int pilgrim;                            /* the feet follow the wisp: 1 by a code, till a key is touched; 2 the test player */
    Item items[MAXITEMS]; int nitems, items_total, items_got, value_total, value_got;
    int nvaults, vault_room[NKEYS], vault_door[NKEYS];   /* one locked room per key colour, in order */
    int ncells, explored;
    float lit_here, glow_here, flare_t;     /* the light at the caster but its own glow; a cast's flare (docs/PLAN-ai.md, S2) */
    int lost_rg[8], lost_n;                 /* the rooms where a search lost the caster this level, newest first (section 30) */
    int nroute; struct { int n, x[5], y[5]; } route[24];  /* patrols' rounds (docs/PLAN-ai.md, section 9) */
    int npacks; Pack packs[MAXPACKS];       /* packs (section 5) */
    int gathered;                           /* step 8: from 39, what you passed has gone to the exit's room */
    unsigned lairs;                         /* rooms a pack may lie in, given a second way in (D2), a bit each */
    float scent[MAPN][MAPN];                /* the caster's trail: when it was last in each cell, by the level's clock (section 11) */
    float still_t, noise_x, noise_y, noise_t;   /* how long the caster has stood; its last loud noise, and when */
    int namb; Ambush amb[2];                /* the level's ambushes (section 12) */
    int nmk; Marked mk[MAXMK];              /* things that want a spell (1.0.0b16, docs/PLAN-play.md section 4) */
    int ward_said;                          /* the elements a ward has been named in, a bit each, this level (section 6) */
    float lull_t, lull_cd, lull_look;       /* step 9: a person's walking with nothing happening; the wait before another
                                               offering; the next look about for a creature */
    int lull_items, lull_links;             /* what had been found when the lull was last reset */
    int hurts, hunt_shaken;                 /* 1.0.0b18: the caster's hurts on the level so far; the hunt shaken off */
    int probe_mk, probe_sp, probe_n; float probe_last;   /* the dream at one: 1 + which, the spell, casts so far, the last */
    int hunt_kind, hunt_on, hunt_said, hunt_nx, hunt_ny, hunt_whip; float hunt_at, hunt_heard;   /* the level's hunt: HK_*, let loose, its
                                                                      nearer tell given; where it picks up the trail; when; the last noise
                                                                      the watch went to */
    int nrest, rest_x[2], rest_y[2], rest_fx[2], rest_fy[2], rest_on, rest_goal, rest_said;   /* the places to mend (section 28): the
                                                                      wall, the floor before it; 1 + the one in use; the dream's goal */
    float rest_acc, rest_beat;

    /* player */
    float px, py, ang, spd, bob, bobph, camz;
    int cx, cy, h, tx, ty, next_h, state, fired, target, spell;
    float u, st, ang0, ang1, adur, hand, cooldown, calm, cast_len, cast_wind;   /* cast_wind: the wind-up, when it goes */
    int resume_state;
    float blocked;                      /* seconds spent waiting on a creature */

    float pvx, pvy;                     /* the caster's velocity, for skeletons leading their throws */
    int hp, max_hp, level_treasure, level_souls, level_xp, has_key[NKEYS];
    double unlock_t;

    /* what is on the caster: poison, slow, and the defensive spells running */
    float poison_t, poison_dps, poison_acc, slow_t, purify_t, regen_acc;
    float af[NAF], af0[NAF], af_k, af_shake;                       /* PLAN-combat section 4: each status's time left on the caster and
                                                                      what it began at (its glyph's edge), how far a knock-down has the
                                                                      view down (0 up, 1 on the floor), a grapple's shove */
    int af_n, af_by;                                               /* Chilled's stacks; 1 + whoever knocked you down */
    float af_lock[NAF], af_hardt;                                  /* S1: each hard control's wait before it may land again, and
                                                                      a second after any when none may */
    float af_burn, af_acc, af_cold, af_cold_t;                     /* On Fire's harm still to come and its odd points; standing in
                                                                      the cold, towards the next stack, and when it was last felt */
    Blame blame, poison_by, af_fire_by;  /* what last hurt the caster, what poisoned it, and what set it alight */
    float ward_hp, ward_t, mirror_t, slip_t, reap_t, mend_t, mend_rate, mirror_flash;   /* mirror_flash: Mirrored just sent one back */
    float mirror_true;                      /* the Mirror raised by hand: its first moment sends back twice as hard, homing (1.0.0b16) */
    float ward_shards, decoy_x, decoy_y, decoy_t;   /* Exquisite runes: the Bone Ward's burst when it breaks; Blink's double */
    float oss_t[10];                    /* Ossified (1.0.0b11): a point of armour for each spear that struck, six seconds each */
    float spr, spr_wait, spr_show;      /* the sprint (1.0.0b15, docs/PLAN-ai.md, 24): seconds left, the wait before it fills, the meter's fade */
    int spr_set, spr_on, spr_dry;       /* the meter filled for the level; sprinting now; run dry, Shift not yet let go */
    float whisper_t, commune_t, commune_show, commune_len, commune_r;   /* Treasure Hunter's whisper; Commune's clock and showing */
    int altar_x, altar_y, altar_state, altar_seek;   /* the Black Altar (x -1: none); the secret its wisp seeks */
    int altar_wisp, altar_inside;                    /* where its wisp is (AW_*); whether you have been into the secret it shows */
    int blessed;                                     /* the altar's blessing: exit wisps turn the undead, this level */
    float def_cd[NDEF], def_wait[NDEF], def_hush[NDEF], rune_glow[NDEF];   /* cooldown; a cast on its way; a missed cue; glow on the hand */
    float rs_since[NDEF], rs_a[NDEF];   /* the runes' sockets (docs/PLAN-b11.md, section 3): since it fired; how far up it has slid */
    uint8_t rs_on[NDEF], rs_slot[NDEF]; /* going round since it fired; its place in the row + 1 (0: none) */
    float bfs_t;                        /* the BFS9000's implosion, playing */
    float nova_t, dread_t, rune_lift;   /* rune_lift: the hand comes up to show a rune burning */              /* the frost nova bursting; Dread's shadow going out */
    float face_t, grin_t; int face_side;    /* the portrait popping in, the grin, which way it flinched */

    /* the way out: a door set in a wall, opened from the floor cell in front of it */
    int exit_wx, exit_wy, exit_fx, exit_fy, exit_dir, exit_in_vault, exit_seen;
    int leaving, exiting;                   /* heading for the exit; stepping through it */
    float exit_open, exit_t;
    int kills, kills_total;
    int rooms_in;                           /* the level's rooms set foot in, a bit each, for their experience */
    int look_mob; float look_t;             /* a person's target: the creature looked at or last hit, how long it stays so */
    int spawned[NKINDS];                    /* how many of each kind have come on this level: a newcomer comes alone */
    uint8_t named[NKINDS];                  /* the ranks of each kind the caption has named on this level */
    Secret secrets[MAXSECRETS]; int nsecrets, secrets_found, nlinks, links_found;   /* rooms behind cracked walls; every link of every chain */
    int trials_failed;                  /* chains sealed for the level after a death in them */
    float knock_t;                      /* a knock on a sealed doorway, not too often */
    int slide, seek_secret; float slide_t;  /* the wall sliding back, and how far; a secret the autopilot means to press */
    int use_x, use_y, use_kind;             /* the autopilot in a chain: a doorway or visportal it means to use (USE_*) */
    Wisp wisps[MAXWISPS];                   /* will-o'-wisps in the crypt: [0] summoned, [1] the altar's */
    int tally;                              /* the level-complete screen */
    double tally_t, clear_time;
    int tally_shown[7], tally_done;           /* one a row: time, kills, treasure, souls, secrets, experience, and a boss's fight */
    float hurt, gold, readout, kick, heartbeat, wind_t, chill;      /* chill: the cold flash when a wraith shrieks; wind_t: a Second Wind */
    int dead;
    int retries;                        /* deaths on this level so far: the crypt goes a little easier each time */
    double dead_t;
    Blame killer;                       /* what it was, at the fall */
    int dcost, dcard_done;              /* a death that costs (a person's run, docs/PLAN-b7.md section 1); its card let go */
    float dcard_t;                      /* how long the card has shown */
    char dlost[64];                     /* what went with it, in a line */
    int claim_card; float claim_t;      /* her card, after a marked page was opened: what she took */
    char claim_text[280];

    /* the merchant's antechamber, before each level */
    int shop, shop_step, shop_pick, shop_bought, shop_look, shop_gold, shop_saving, shop_heal;
    Offer offers[NPED];
    int offer_gone[NPED], offer_back[NPED]; /* offer_back: held last visit, back as it was */
    float ped_x[NPED], ped_y[NPED];         /* the pedestals, in an arc round the back wall */
    int ped_deco[NPED];                     /* and their stones among the decorations */
    int ped_n, shop_left, shop_confirm, shop_letgo, shop_held, shop_buy;   /* pedestals; wares still to be had; a ware awaiting a
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
    int tot_look, tot_arm, tot_col;         /* the Hexed Totem: looked at; a sale awaiting a second Enter; raise (0) or sell (1) */
    int lamp_look;                          /* the Lampstand looked at (1.0.0b11) */
    int por_look;                           /* the Accursed Portal, looked at */
    int pt_look, pt_sel;                    /* a portrait of the slain looked at (1 + which), and the one its card shows */
    int comp_look, comp_page, comp_arm;     /* the Corpus Compendium: looked at; its page; a purchase awaiting a second Enter */
    int comp_tried;                         /* the autopilot has looked in it this visit */
    int auto_chg; float auto_hold;          /* the autopilot charging an ultimate (or holding Soul Tether, so long) (1.0.0b11) */
    int hall_step;                          /* its errand to the bell and the font: 0 none, then which leg of it */
    int rep_look, rep_mode, rep_place, rep_amt;   /* the Repository: looked at; choosing what goes in (1) and where; how much gold */
    int cx_page;                            /* the Codex's page open */
    int mer_look;                           /* the merchant herself, close enough for E */
    int map_look;                           /* the map floating beside her, looked at */
    int stone_look, stone_sel;              /* a stone in the plot looked at (-1 none); the one whose page is open */

    /* a person at the controls */
    int player, ui, ui_sel, quit_line, status_ret;
    int quit_arm;                           /* Esc's BEGIN A NEW RUN, asked once already */
    int led_mode, led_arm, led_msg;             /* the ledger: signing (0) or opening (1); a second Enter's page; why */
    int best_page, best_rank, best_scroll;      /* the bestiary: which creature (or the achievements), which rank, how far down */
    int best_ways;                              /* 1.0.0b18: its ways shown in place of its lore (Space) */
    int best_feat;                              /* its feat looked at, past its ranks (the portraits', step 5) */
    float ui_t;                                 /* a clock that runs while a menu holds the crypt still */
    float pc_t, pc_len, grab_t, hint_t, msg_t;     /* their cast; reaching for a ware; the hints; a message */
    float pc_wind;                          /* their cast's wind-up: it goes this far in (docs/PLAN-b11.md, section 7) */
    int chg_on, chg_sp; float chg_t;        /* a spell held to charge (section 7, K2): which, and how long; on: still held */
    int tether; float tether_tick;          /* Soul Tether held on a creature (1 + its number; 0 none), its next draining */
    float lay_x[MAXSHOTS], lay_y[MAXSHOTS];  /* where each shot last laid Ionization's trail or the Lance's rime */
    float fwall_t, fwalk_t, fwalk_cd, fwall_dps;   /* Firewall's spirits circling, and their burn; Flamewalker's armour, and its wait */
    float bar, bar_t, peek, lift;           /* the status bar shown (0..1), time before it goes; H held; the readout raised */
    float hand_base, ready_t;               /* a person's hand: how far up between casts, and how long it stays */
    float l_t; int l_down;                  /* L held this long (a tap casts, a hold opens the menu, or with the Lantern of
                                               the Dead shutters it) */
    float l_tap;                            /* with the lantern and L casting at a tap: a first tap waiting for a second */
    float gift_glow; int gift_said; double gift_bump;   /* the Lampstand flaring as a gift is taken; its caption, once a visit */
    int pc_fired, spirit_said, splint_said; /* spirit_said: the caption for a weak cast, once a level; splint_said a splinter's */
    int click_spent;                        /* the click went on a ware or the cat: no cast until it is let go */
    char msg[64];
    Gear inv0;                              /* the gear as it was when the inventory opened */
    int inv_page;                           /* the wardrobe (0), the caster's numbers and spellbook (1), the cornerstones, the Alignment,
                                               the elements (1.0.0b11) */
    int al_el, al_node, al_arm;             /* the Alignment's screen: the line, the node, a buy or unbinding awaiting a second Enter */
    int vow_sel, vow_arm, vow_change;       /* her question: the class chosen, awaiting a second Enter, another past (changing) */
    int bell_look, font_look, h2_obj, h2_arm;   /* the Dead Bell or the Weeping Font looked at; which one's menu is up, armed */
    int bell_deco, font_deco;               /* and their decorations, to turn when they are cursed */
    int corrupt, cshown, fonted, font_drunk;    /* this level corrupted, its secrets shown, its experience sweetened; drunk this visit */
    int replay, port_lvl;                   /* this level a replay; the Portal's level chosen to replay */
    int latch;                              /* Soul Sap (docs/PLAN-b11.md, section 6): 1 + the bat latched on your face */
    float latch_t, shake, sap_acc, shake_ang;   /* how long it has held; the shaking done; the drain gathering; the view's last turn */
    int shake_dir, strafe_dir; float shake_last, strafe_last, slip_k;   /* the way the view and the feet went last, and when */
    float map_cx, map_cy, map_hold;         /* the map: the cell at its middle, a direction held this long */
    int map_on; float map_pan_t;            /* the map up as you walk (docs/PLAN-b11.md, section 1); since it was last panned */
    int asc_buy[NLINE], asc_arm;            /* the caster's screen: steps bought since it opened (they may go back); a buy past a cap awaiting a second press */
    int cs_sel, cs_arm;                     /* the cornerstones' screen: the one looked at; one awaiting a second Enter */
    /* the boss abilities: each one's wait; Swift Dodge's dart (its clock and its way) and the direction last
       tapped and how long ago; Wraithwalk's while; which sockets were ready, for the tick when one comes round */
    float ab_cd[NBOSS], dash_t, dash_vx, dash_vy, tap_t, wwalk_t;
    int tap_key, ab_ready;

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
    /* the runs' hazards (1.0.0b13, docs/PLAN-b13.md part B): the fixed ones placed with the level; the next arising one;
       the harm each element is wearing into you and which hazard it was; a web holding you fast */
    HzSite site[MAXSITE]; int nsite;
    float arise_t, hz_el_acc[NELEM], snare_t;
    uint8_t hz_el_by[NELEM];
    /* puzzles' things (1.0.0b13, section 6): what is laid out, what is carried (1 + it), what was read last and how long it shows */
    Thing thing[MAXTHING]; int nthing, carry;
    float read_t; char read_text[160];
    /* the level's puzzle (part C): its kind (-1 none), the wing's room and door, the exit in it, solved, wrong tries and the
       whisper given; the cell before the door; an order to follow and how far it has come; its own numbers; clocks; the
       answer (the hidden message's) and the nudge; the thing the dream means to touch */
    int pz, pz_room, pz_door, pz_exit, pz_solved, pz_tries, pz_hinted, pz_wx, pz_wy;
    int pz_seq[16], pz_nseq, pz_step, pz_a[32], use_thing, pz_stepx, pz_stepy;   /* pz_step*: a stone the dream steps to, off the lanes */
    float pz_t, pz_wait;
    char pz_answer[200], pz_hint[100];
    /* the hidden message (section 6a): the decoration it lies in (-1 none), found; a search under way (the decoration, -1
       none, and its second) and those searched; the Black Shrine (its cell, -1 none, and decoration), found, its showing
       left, its eyes' flare, its menu asking twice */
    int msg_deco, msg_found, srch_deco, nsearched, bsh_x, bsh_y, bsh_deco, bsh_found, bsh_arm;
    int16_t searched[32];
    float srch_t, bsh_show, bsh_flare;
    /* the level's trial (part D): its kind (-1 none), room, door (index), the cell before it outside and the lane cell two
       steps in; its state (TRS_*); the boon (1) or the curse (-1) for the rest of the level; the things it brought (from
       tri_n0); its own numbers and clocks; the sand left and how long since it ended; a slow thing under the hand (1 +
       it) and its second; the room's faces while it is turned */
    int tri, tri_room, tri_door, tri_wx, tri_wy, tri_ix, tri_iy, tri_state, tri_boon, tri_n0, tri_a[16], work_i, tri_nface;
    float tri_t, tri_end, tri_f[8], work_t, tri_hold;
    int rk_fresh;                                                  /* the reckoning (section 10a) just opened */
    int16_t tri_fx[160], tri_fy[160]; uint8_t tri_fd[160], tri_face[160], tri_fover[160], tri_flo[256], tri_floored;
    /* the dream's counters (1.0.0b15 step 10, docs/PLAN-ai.md Part G): a sound behind it to glance back at (pending, where, when
       last); a corner to fight from (held, the cell, how long, when it may take one again); the ambushes whose sign it has read
       and those it has cast into (a bit each) */
    int glance, corner_on, corner_x, corner_y, amb_read, amb_probed;
    float glance_x, glance_y, glance_last, guard_t, corner_cd;
    /* docs/PLAN-combat.md section 17: each track adds fields only above its own end mark. What is on the caster and must go
       into a pocket with it belongs in carry_caster's spans instead (the caster's track: by poison_t) */
    int cf_buy[NCONF], cf_arm, cf_repn;                            /* 1.0.0b20, the Confessions' screen: points confessed since it
                                                                      opened (they may go back); an overlevel awaiting its second
                                                                      press (100 + the Confession), -1 none; a key held, the points
                                                                      it has given and when it went down (-1 not held) */
    float cf_rep0;
    int al_aug, al_augs;                                           /* 1.0.0b21, the Lampstand: the pair of an augment's step shown (1 + the
                                                                      step, 0 none), and which of the two the eye is on */
    /* the caster's track (C) ends */
    /* the dead's track (D): */
    int lb_spent;                                                  /* 1.0.0b20: Constitution's killing blow lived this level (D5, */
    double lb_at;                                                  /* 10.4), and when: its breath is drawn as Second Wind's */
    int rst_sel, rst_on;                                           /* Restoration (10.3): in hand in place of the spell; channelling */
    float rst_t, rst_acc, rst_k, rst_snd, rst_spent;               /* the press held, health owed, the glow, a breath's sound, souls */
    int rst_healed;                                                /* spent and health given this channel */
    int d7_lg;                                                     /* D7: Lingering Hunger's 1 + the creature the beam holds, its time */
    float d7_lg_t, d7_lg_dps, d7_lg_acc, d7_lg_beam;               /* left, harm a second, harm owed, the beam's next drawing */
    float d7_drained, d7_zap, d7_spear, d7_shep;                   /* the drain's last hit, the bolt's, the spear's; Shepherd's pace for */
    int d7_splint;                                                 /* the motes a kill now leaves; a splinter striking */
    /* the dead's track (D) ends */
} G;
enum { BP_NONE, BP_PLANT, BP_RAISE, BP_STRIKE,      /* the knight's: sword planted with a shout, raised, swept; */
       BP_LITANY };                                 /* the Deacon's censer raised, the green light growing */
static float zbuf[W];
enum { MM_SIGN, MM_ASCEND, MM_OFFER, MM_VOW, MM_PED, MM_WIND, MM_PAT, MM_LEAVE, NMM };   /* the merchant's menu */
static const int PED_PRICE[NPED + 1] = { 0, 0, 0, 500, 1250, 3125, 7815, 19530 };   /* the third pedestal to the seventh */
#define NINV 6                              /* Tab's pages */
enum { UI_NONE, UI_INVENTORY, UI_QUIT, UI_LEDGER, UI_BESTIARY, UI_LETGO, UI_WISPS, UI_ALTAR, UI_SHRINE, UI_TOTEM, UI_PORTAL, UI_MERCHANT, UI_STONE, UI_COMP, UI_REPO, UI_CODEX, UI_ASCEND, UI_STONES, UI_ALIGN, UI_CLASS, UI_HALL2, UI_BSHRINE, UI_RECKON, UI_PAD,
       UI_PORTRAIT, UI_BUY, UI_TUTOR };                                      /* the menus a person can open; a portrait of the slain's card; a ware's buying */
