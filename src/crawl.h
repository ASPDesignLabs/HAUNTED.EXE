/* Shared data for the CATACOMB raycaster: palette, light colormap, wall
 * textures and sprites.  All of it is generated procedurally at startup. */
#ifndef CRAWL_H
#define CRAWL_H

#include "core.h"

#define TS 64                       /* texture and sprite size */
#define NLIGHT 32                   /* light levels in the colormap; level 24 = unlit base colour */
#define LIGHT_UNIT 24.0f

/* Palette layout: 16-entry ramps, dark to light. */
enum {
    R_STONE = 1, R_WARM = 17, R_BONE = 33, R_BRICK = 49, R_MOSS = 65, R_WOOD = 81,
    R_IRON = 97, R_SKIN = 113, R_GHOST = 129, R_PURPLE = 145, R_BLOOD = 161,
    R_FLOOR = 177, R_CEIL = 193,
    R_FLAME = 209,                  /* full-bright: fire */
    R_GLOW = 225,                   /* full-bright: magic */
    C_WHITE = 241,
    R_GOLD = 242,                   /* 13 entries, 242..254: treasure and the key */
    C_CLEAR = 255                   /* transparent in sprites */
};
#define NGOLD 13
#define IS_FULLBRIGHT(c) ((c) >= R_FLAME && (c) <= C_WHITE)

enum {
    T_STONE, T_RUBBLE, T_SKULLS, T_BRICK, T_MOSS, T_NICHE, T_BANNER,
    T_TORCH0, T_TORCH1, T_TORCH2, T_DOOR, T_JAMB, T_LOCKED,
    T_EXIT0, T_EXIT1, T_EXIT_OPEN,                /* the way out: runes shimmering, then thrown open */
    T_LOCKED_S, T_LOCKED_C,                       /* the silver and crimson vault doors */
    T_SIGN,                                       /* the merchant's sign */
    T_PASSAGE,                                    /* a secret's doorway into its pockets: frames going away into light */
    /* the Backrooms (1.0.0b6): drawn in R_FLOOR and R_CEIL, which a pocket recolours (crawl_art_theme) */
    T_PAPER, T_PAPER_DAMP, T_PAPER_OUTLET,        /* the yellow wallpaper; a leak from above; a socket */
    T_BDOOR,                                      /* a doorway between pockets: painted trim, another yellow room */
    T_VISPORT0, T_VISPORT1, T_VISPORT2, T_VISPORT3,   /* the chain's end: a doorway onto doorways, without end */
    T_CARPET,                                     /* the floor: damp carpet */
    T_CEILTILE, T_CEILSTAIN, T_CEILLAMP, T_CEILDEAD,  /* the ceiling: tiles, a stained one, a tube panel lit and dead */
    /* Severance (1.0.0b6): R_FLOOR a muted green, R_CEIL a cool white, inside its pockets */
    T_WHITE, T_WHITE_SIGN, T_WHITE_ART,           /* white walls; a placard with an arrow; a calm picture */
    T_SDOOR,                                      /* a doorway between pockets: a treatment door, wired glass, a push bar */
    T_SCARPET,                                    /* the floor: close office carpet */
    T_SCEIL, T_SCEILLAMP,                         /* the ceiling: smooth panels, and panels lit */
    /* the bosses' arenas (1.0.0b7) */
    T_GATE,                                       /* a portcullis in an arch: the adds come through */
    T_FONT, T_FONT_DRY,                           /* a basin set in the wall, a draught in it; and drunk dry */
    T_GRATE,                                      /* a drain low in the wall: where the bile comes from */
    T_PORTAL0, T_PORTAL1, T_PORTAL2, T_PORTAL3,   /* the Accursed Portal: an arch filled with a slow red-violet swirl */
    T_SEALED,                                     /* a chain's doorway shut for the level: dark frames under a cracked red seal */
    T_REPOSITORY,                                 /* the Wretched Repository: iron doors shut in niches, three to a wall */
    /* the runs' hazards (1.0.0b13, docs/PLAN-b13.md section 5): drawn on plain stone, set in the run's own */
    T_HZ_SLAB, T_HZ_SLAB_OPEN,                    /* the Embalmers' Breath: a burial slab; cracked, a seam of light round it */
    T_HZ_COLD,                                    /* the Cold Vault: a crack furred with rime */
    T_HZ_SLUICE, T_HZ_SLUICE_OPEN,                /* a sluice gate shut; and thrown up on the surge */
    T_HZ_SAINT, T_HZ_SAINT_LIT,                   /* the Weeping Saint in her alcove; her eyes alight as she drinks */
    T_HZ_SEAM, T_HZ_SEAM_LIT,                     /* the Lodestone Seam, green veins in the rock; and crackling */
    T_HZ_FISSURE0, T_HZ_FISSURE1,                 /* Fissure Breath: a crack in the glass rock, glowing; then white-hot */
    T_HZ_MIRROR, T_HZ_MIRROR_SEEN,                /* the Black Mirror; and your reflection come into it */
    T_WINGDOOR,                                   /* a puzzle's sealed wing (1.0.0b13): a great door, its seal carved over it */
    /* each run's own (docs/PLAN-b10.md), made again for its run by crawl_art_run: the materials, two looks
     * each, on one grid of courses; blends from one material to the next, left to right; what stands on a
     * wall, drawn over its material (C_CLEAR lets the material show); floors, ceilings and the doors */
    T_MAT,
    T_BLEND = T_MAT + 10,                         /* NMAT x NVAR */
    T_FEAT = T_BLEND + 6,                         /* NBLEND */
    T_FLOORS = T_FEAT + 8,                        /* NFEAT */
    T_CEILS = T_FLOORS + 5,                       /* NFLOORS */
    T_RDOOR = T_CEILS + 3,                        /* NCEILS */
    T_RJAMB, T_RLOCKED, T_RLOCKED_S, T_RLOCKED_C,
    T_TRDOOR,                                     /* a trial's door (1.0.0b13): the run's own, its skull and crossed bones */
    T_REST,                                       /* a place to mend (1.0.0b15, docs/PLAN-ai.md, 28): the run's own, set in its stone */
    T_GATE_SEAL,                                  /* the tutorial's one-way gate (docs/PLAN-tutorial.md 7.3): its south face, a plate for the seal */
    T_ICE,                                        /* its ice field: a sheet laid over the floor, as a mark (C_CLEAR nowhere) */
    NTEX
};
#define NMAT 5
#define NVAR 2
#define NBLEND 6
#define NFEAT 8
#define NFLOORS 5
#define NCEILS 3
#define T_MATV(m, v) (T_MAT + (m) * NVAR + (v))
/* run I's slots, by name: its materials, what stands on its walls (the torch's three frames from F1_TORCH),
 * its floors and its ceilings */
enum { M1_ASHLAR, M1_RUBBLE, M1_PLASTER, M1_BEDROCK, M1_BONES };
enum { F1_NICHE, F1_BANNER, F1_SLAB, F1_SHRINE, F1_TABLET, F1_TORCH };
enum { FL1_FLAGS, FL1_CHAPTER, FL1_SETTS, FL1_EARTH, FL1_BROKEN };
enum { CE1_RIB, CE1_VAULT, CE1_ROCK };
/* run II's: the same slots, in the same roles (floor 1 lies as laid, floor 4 is floor 0 gone with the rot,
 * ceiling 0 the rooms', 1 the halls') */
enum { M2_SLATE, M2_LINING, M2_SILT, M2_FLOW, M2_BONES };
enum { F2_NICHE, F2_SAINT, F2_CULVERT, F2_HANDS, F2_WEB, F2_LANTERN };
enum { FL2_FLAGS, FL2_SILT, FL2_SLATE, FL2_WATER, FL2_PUDDLES };
enum { CE2_VAULT, CE2_ROCK, CE2_DRIP };
/* run III's */
enum { M3_GRANITE, M3_FACE, M3_TIMBER, M3_BASALT, M3_FILL };
enum { F3_ORE, F3_FOSSIL, F3_FRAME, F3_CHUTE, F3_MARKS, F3_LAMP };
enum { FL3_GRAVEL, FL3_RAILS, FL3_EARTH, FL3_CHARNEL, FL3_SCREE };
enum { CE3_LAGGING, CE3_ROCK, CE3_ROOTS };
/* run IV's (the torch's three frames are the magma fissure's) */
enum { M4_OBSIDIAN, M4_BASALT, M4_ASH, M4_CRYSTAL, M4_BONES };
enum { F4_GEODE, F4_SEAM, F4_GOLD, F4_FUSED, F4_FRIEZE, F4_FISSURE };
enum { FL4_GLASS, FL4_THRONE, FL4_ASH, FL4_CINDER, FL4_SHARDS };
enum { CE4_CRYSTAL, CE4_ROCK, CE4_SOOT };

enum {
    SP_BONES1, SP_BONES2, SP_SKULLPILE, SP_COBWEB, SP_CHAINS, SP_URN, SP_PILLAR, SP_CANDLES,
    SP_SKEL_A, SP_SKEL_B, SP_SKEL_D1, SP_SKEL_D2, SP_SKEL_D3,
    SP_GHOST_A, SP_GHOST_B, SP_GHOST_D1, SP_GHOST_D2,
    SP_BAT_A, SP_BAT_B, SP_BAT_D,
    SP_FIRE_A, SP_FIRE_B, SP_BOOM1, SP_BOOM2, SP_BOOM3,
    SP_SKEL_T,                                  /* skeleton winding up a throw */
    SP_BONEFLY0, SP_BONEFLY1, SP_BONEFLY2, SP_BONEFLY3,
    SP_COINS_A, SP_COINS_B, SP_CHALICE, SP_GEMS, SP_CROWN, SP_CHEST,
    SP_KEY0, SP_KEY1, SP_KEY2, SP_KEY3, SP_POTION,
    SP_KEYS0, SP_KEYS1, SP_KEYS2, SP_KEYS3,     /* the silver key, turning */
    SP_KEYC0, SP_KEYC1, SP_KEYC2, SP_KEYC3,     /* the crimson key */
    SP_SHIELD_A, SP_SHIELD_B, SP_SHIELD_S,      /* shield-bearing skeleton: walking, swinging */
    SP_SHIELD_XA, SP_SHIELD_XB, SP_SHIELD_XS,   /* ... its shield broken */
    SP_WSKULL0, SP_WSKULL1, SP_WSKULL2, SP_WSKULL_D,   /* the wall skull: asleep, charging, blazing, falling */
    SP_SPIDER_A, SP_SPIDER_B, SP_SPIDER_BITE, SP_SPIDER_D,
    SP_SKULLFIRE_A, SP_SKULLFIRE_B,             /* a wall skull's fireball */
    SP_FROST, SP_SPEAR, SP_HELL_A, SP_HELL_B, SP_WISP, SP_SPARK, SP_BURN_A, SP_BURN_B,
    SP_SOULURN, SP_RINGITEM,                    /* in the key rooms */
    SP_PEDESTAL, SP_ITEM_TOME, SP_ITEM_RUNE, SP_ITEM_ROBE, SP_ITEM_HOOD, SP_ITEM_RING,   /* the merchant's wares; R_PURPLE parts take the item's colour */
    SP_ITEM_BRACER, SP_ITEM_BOOTS, SP_ITEM_AMULET, SP_ITEM_BAG,
    SP_ITEM_BONE,                               /* a bone on the floor, for the spear */
    SP_BALTAR,                                  /* a Black Altar: black stone, carved, a bowl for its fire */
    SP_BFLAME_V0, SP_BFLAME_V1,                 /* ... its fire, violet while it seeks */
    SP_BFLAME_G0, SP_BFLAME_G1,                 /* ... and gold when every secret is found */
    SP_HOOKS,                                   /* chains from the ceiling ending in meat hooks */
    SP_GIBBET, SP_GIBBET_SKEL,                  /* a hanging cage, empty or with its prisoner */
    SP_CHANDELIER_A, SP_CHANDELIER_B,           /* a ring of candles hung on chains, flickering */
    SP_ROOTS, SP_WEBSHEET,                      /* roots through the ceiling; a sheet of web */
    SP_TOMB_ROUND, SP_TOMB_CROSS, SP_TOMB_OBELISK, SP_TOMB_BROKEN,
    SP_FIREPIT_A, SP_FIREPIT_B, SP_FIREPIT_C,   /* a ring of stones, burning */
    SP_TOTEM_A, SP_TOTEM_B,                     /* a pole of skulls with candles, flickering */
    /* the bestiary (docs/PLAN-bestiary.md): drawn, not yet in play */
    SP_SHAMB_A, SP_SHAMB_B, SP_SHAMB_GRAB, SP_SHAMB_RISE1, SP_SHAMB_RISE2, SP_SHAMB_D1, SP_SHAMB_D2,
    SP_DROWN_A, SP_DROWN_B, SP_DROWN_GRAB, SP_DROWN_RISE, SP_DROWN_BURST, SP_DROWN_POOL,
    SP_GHOUL_A, SP_GHOUL_B, SP_GHOUL_LEAP, SP_GHOUL_FEED, SP_GHOUL_D,
    SP_HOUND_A, SP_HOUND_B, SP_HOUND_BITE, SP_HOUND_D,
    SP_KNIGHT_A, SP_KNIGHT_B, SP_KNIGHT_W, SP_KNIGHT_S,     /* walking, winding up, striking: in full plate, */
    SP_KNIGHT2_A, SP_KNIGHT2_B, SP_KNIGHT2_W, SP_KNIGHT2_S, /* ... with half of it hacked off, */
    SP_KNIGHT3_A, SP_KNIGHT3_B, SP_KNIGHT3_W, SP_KNIGHT3_S, /* ... and down to the breastplate */
    SP_KNIGHT_D,
    SP_DEACON_A, SP_DEACON_B, SP_DEACON_CHANT, SP_DEACON_D,
    SP_HUNTER_A, SP_HUNTER_B, SP_HUNTER_LEAP, SP_HUNTER_D,   /* drawn right way up: the renderer flips it on the ceiling */
    SP_BROOD_A, SP_BROOD_B, SP_BROOD_LAY, SP_BROOD_D,
    SP_SAC_A, SP_SAC_B, SP_SAC_OPEN,                         /* an egg sac on the wall: still, stirring, split */
    SP_LING_A, SP_LING_B,                                    /* spiderlings, drawn small */
    SP_WRAITH_A, SP_WRAITH_B, SP_WRAITH_REACH, SP_WRAITH_BACK,   /* the Wraith: gliding, reaching, driven back */
    SP_SHADE_A, SP_SHADE_B, SP_SHADE_TOUCH, SP_SHADE_D,
    SP_IMP_A, SP_IMP_B, SP_IMP_THROW, SP_IMP_D, SP_EMBER_A, SP_EMBER_B,
    SP_GARG_PERCH, SP_GARG_A, SP_GARG_B, SP_GARG_SWOOP, SP_GARG_D,   /* the perch is also the statue in an alcove */
    /* the dressing brought in as you go down (docs/PLAN-bestiary.md, section 8): drawn, not yet placed */
    SP_CANDLESTAND_A, SP_CANDLESTAND_B,         /* I: a hall's light before the totems come */
    SP_BROKEN_ARMS,                             /* I: a split shield and a broken sword */
    SP_MOUND, SP_PIT,                           /* I: a grave's earth freshly turned; and opened */
    SP_PUDDLE_A, SP_PUDDLE_B,                   /* II: still water; and bubbling */
    SP_STALAG,                                  /* II: flowstone */
    SP_LANTERN_A, SP_LANTERN_B,                 /* II: hung on a chain, lit */
    SP_BRAZIER_A, SP_BRAZIER_B, SP_BRAZIER_C,
    SP_COLLAR,                                  /* II: a chain and a spiked collar among gnawed bones */
    SP_ICICLES,                                 /* II: where the Wraith has passed */
    SP_BELL,                                    /* II: a bell hung from a beam */
    SP_CARCASS,                                 /* III: picked to the bone */
    SP_ARMOURSTAND, SP_HELM,                    /* III: plate on a stand; a great helm fallen */
    SP_TIMBER,                                  /* III: a mine prop */
    SP_MINELAMP_A, SP_MINELAMP_B,
    SP_CART,                                    /* III: a mine cart of ore */
    SP_FUNGUS,
    SP_CRYSTAL_A, SP_CRYSTAL_B,                 /* IV: glowing, as they pulse */
    SP_MAGMA_A, SP_MAGMA_B, SP_MAGMA_C,         /* IV: a vent of molten rock */
    SP_OBSIDIAN,
    SP_ALTAR_A, SP_ALTAR_B,                     /* IV: black candles on a draped stone */
    SP_DOUBLE,                                  /* an Exquisite Blink's double: the caster, hooded, in violet light */
    SP_SHRINE_S, SP_SHRINE_C,                   /* the Rune Shrine: sanctified (pale, cold), and cursed (soot, cracks) */
    SP_SHRINE_G0, SP_SHRINE_G1,                 /* ... its cursed carvings and candles, at full brightness; R_PURPLE takes its rank's colour */
    SP_SRUNE0, SP_SRUNE1, SP_SRUNE2, SP_SRUNE3, SP_SRUNE4, SP_SRUNE5, SP_SRUNE6, SP_SRUNE7, SP_SRUNE8,   /* a rune circling it, by kind */
    SP_SRUNE_X,                                 /* ... the one it shows before it is attuned to any */
    SP_CCOIN,                                   /* a Cursed Coin */
    SP_DESKS,                                   /* Severance: four desks behind low partitions, old terminals on them */
    SP_COOLER, SP_COOLER_LOW,                   /* ... the water cooler, full, and after its one drink */
    SP_SCRIP,                                   /* a boss's Named Odious Scrip: a curl of parchment, a black seal */
    /* the bosses' hazards (1.0.0b7) */
    SP_GRIT0, SP_GRIT1,                         /* grit trickling from the ceiling where a rock will fall */
    SP_ROCK, SP_RUBBLE,                         /* the rock, falling; and what it leaves, sinking into dust */
    SP_ERUPT0, SP_ERUPT1,                       /* fire bursting from the cracks, at full brightness */
    SP_VOIDCOL,                                 /* the void's column of purple dark, drawn see-through */
    SP_HANDS,                                   /* hands of the dark rising from the floor */
    SP_KNIGHT_P0, SP_KNIGHT_P1, SP_KNIGHT_P2,   /* the Ill-Fated Knight plants its sword, in each state of its plate */
    SP_BSPIDER_A, SP_BSPIDER_B, SP_BSPIDER_LEAP, /* the Wandering Spider: walking, and its leap (the rest at the end) */
    SP_BRAZIER_OUT,                             /* a brazier the Wraith has put out: the cold bowl and a little ash */
    SP_HEXTOTEM,                                /* the Hexed Totem: a black pole of skulls, hooks for the bosses' scrip */
    SP_TOMB_FRESH,                              /* the newest stone in the merchant's plot: clean-cut, the earth still turned */
    SP_LECTERN,                                 /* the Corpus Compendium: a tome open on a lectern of black oak */
    SP_MAPWARE,                                 /* the merchant's map of the crypt: a scroll half unrolled, floating (1.0.0b9) */
    SP_FLASK,                                   /* the Blight Flask in flight: a stoppered flask of green venom (1.0.0b11) */
    SP_LAMPSTAND,                               /* the Lampstand: six iron bowls on a black stand, the Alignment's (1.0.0b11) */
    SP_ORB_A, SP_ORB_B,                         /* Amperage's orb: a slow ball of lightning, threads crackling off it */
    SP_LANCE,                                   /* Winter's Lance going: a long star of ice, frost streaming off */
    SP_BALLISTA,                                /* the Ballista's spear, point first: a heavy bone, its head bound in iron */
    SP_FSPIRIT_A, SP_FSPIRIT_B,                 /* Firewall's flame spirits, two hollow eyes in each */
    SP_RIME,                                    /* the Lance's rime on the floor, feathering out */
    SP_MOTE,                                    /* a soul's mote where the dead fell: a pale flame with a face in it */
    SP_DBELL, SP_DBELL_C,                       /* the Dead Bell in its frame: cracked bronze; corrupted, black, the crack alight */
    SP_FONT, SP_FONT_C,                         /* the Weeping Font: a basin under a weeping face, dry; cursed, brimming red */
    /* the runs' hazards (1.0.0b13, docs/PLAN-b13.md section 5) */
    SP_CENSER,                                  /* the Swinging Censer: an iron bowl on its chain, smoking */
    SP_SMOKE, SP_VAPOUR, SP_GAS,                /* a puff of smoke; the embalmers' grey-green breath; silt gas and firedamp, sallow */
    SP_VOTIVE,                                  /* a votive rack gone over, its cups spilling */
    SP_RELIQ, SP_RELIQ_O,                       /* the Hungry Reliquary on its plinth; and its lid up, violet inside */
    SP_SNARE,                                   /* a web across a doorway, all but unseen */
    SP_SPIKES,                                  /* Shatterglass: spikes of black glass stood up from the floor */
    SP_ASHFALL,                                 /* ash coming down */
    /* puzzles' things (1.0.0b13, docs/PLAN-b13.md sections 6 and 7) */
    SP_PZ_SKULL,                                /* a skull to carry, its brow scratched with a mark */
    SP_PZ_PLINTH,                               /* a low stone plinth that takes a thing */
    SP_PZ_DIAL0, SP_PZ_LEVER0 = SP_PZ_DIAL0 + 8,   /* a dial on a post in its eight turns; */
    SP_PZ_LEVER1, SP_PZ_CANDLE0, SP_PZ_CANDLE1, /* a lever up and down; a candle dark and lit */
    SP_PZ_TABLET,                               /* a carved tablet on its stand */
    SP_PZ_NICHE,                                /* a niche cut in the wall, a mark over it (the Ossuary Ledger) */
    SP_PZ_EFFIGY0, SP_PZ_EFFIGY3 = SP_PZ_EFFIGY0 + 3,   /* a knight on his tomb, his head turned away, to your right, to you, to your left */
    SP_PZ_PAN, SP_PZ_SCALE0, SP_PZ_SCALE2 = SP_PZ_SCALE0 + 2,   /* a pan on its stand; the great scale, left down, level, right down */
    SP_PZ_CASKET,                               /* a small reliquary, beads on its lid */
    SP_PZ_BANNER,                               /* the Order's banner on its pole */
    SP_PZ_CISTERN0, SP_PZ_SAINT = SP_PZ_CISTERN0 + 9,   /* a cistern in the wall, its water from dry to full; a saint, her hands out empty */
    SP_PZ_EMBLEM,                               /* an emblem to give her, its sign on it */
    SP_PZ_SPOUT, SP_PZ_MILL0, SP_PZ_MILL1,      /* the culverts' spout, and the mill wheel turning */
    SP_PZ_IDOL, SP_PZ_LOCK,                     /* the gold veins' idol, and the door's lock */
    SP_PZ_WHEEL,                                /* a sluice wheel, a saint's sign on its hub */
    SP_PZ_BOARD, SP_PZ_BOX, SP_PZ_SACK,         /* the Hollows': a chalk board; the hoist's counterweight box; a sack of ore */
    SP_PZ_HOOK, SP_PZ_TAG,                      /* a hook on the tally board; a miner's tag */
    SP_PZ_SOCKET, SP_PZ_FOSSIL,                 /* a socket in the rock; a fossil's bone */
    SP_PZ_STONE, SP_PZ_LAMP0, SP_PZ_LAMP1,      /* a standing stone; a miner's lamp, dark and lit */
    SP_PZ_PRISM0, SP_PZ_GEODE = SP_PZ_PRISM0 + 4,   /* the Deep's: a prism, its face away, to your right, to you, to your left; */
    SP_PZ_GEODE1, SP_PZ_THRONE, SP_PZ_STATUE,   /* a geode, dark and lit; the throne; a statue of the dead court */
    SP_PZ_VENT0, SP_PZ_VENT1,                   /* a vent, shut and open */
    SP_PZ_SLAB, SP_PZ_NUMSTONE,                 /* a square of the grid; a numbered stone */
    SP_PZ_LETTER0, SP_PZ_LETTER_END = SP_PZ_LETTER0 + 26,   /* the letters, A to Z, for the dials */
    SP_MARK0, SP_PIP1 = SP_MARK0 + 16,          /* the marks carved on things (PZ_MARKS in crawl_art.c), */
    SP_PZ_LAST = SP_PIP1 + 9,                   /* and one to nine pips */
    SP_HIDE0,                                   /* where the hidden message lies (1.0.0b13): a candle-box, a drowned */
    SP_BSHRINE = SP_HIDE0 + 4,                  /* satchel, a miner's boot, a broken geode, a run each; the Black Shrine, */
    SP_BSHRINE_E0, SP_BSHRINE_E1,               /* and its skull's eyes, faint and flaring (drawn at full brightness) */
    SP_TRLIGHT0,                                /* by a trial's door (1.0.0b13), a run each: a red candle on a stand, a */
    SP_TR_BIER = SP_TRLIGHT0 + 4,               /* drowned lantern, a red lamp, a brazier of red glass; a bier of stone */
    SP_TR_PROP0, SP_TR_PROP1,                   /* a loose prop under the roof, and braced */
    SP_TR_BRAZ0, SP_TR_BRAZ1,                   /* a brazier, out and burning (the Wraith's, as a trial's thing) */
    /* side views, walking, facing right (drawn mirrored for left): the bestiary turns them about */
    SP_SKEL_SA, SP_SKEL_SB, SP_SHIELD_SA, SP_SHIELD_SB, SP_BAT_SA, SP_BAT_SB, SP_SPIDER_SA, SP_SPIDER_SB,
    SP_GHOST_SA, SP_GHOST_SB, SP_WSKULL_S,
    SP_SHAMB_SA, SP_SHAMB_SB, SP_DROWN_SA, SP_DROWN_SB, SP_HOUND_SA, SP_HOUND_SB, SP_WRAITH_SA, SP_WRAITH_SB,
    SP_HUNTER_SA, SP_HUNTER_SB, SP_GHOUL_SA, SP_GHOUL_SB, SP_KNIGHT_SA, SP_KNIGHT_SB, SP_BROOD_SA, SP_BROOD_SB,
    SP_SHADE_SA, SP_SHADE_SB, SP_IMP_SA, SP_IMP_SB, SP_DEACON_SA, SP_DEACON_SB, SP_GARG_SA, SP_GARG_SB,
    /* the bosses' own (docs/InitialArtPass.md, part three) */
    SP_BKNIGHT_A, SP_BKNIGHT_B, SP_BKNIGHT_W, SP_BKNIGHT_S,     /* the Ill-Fated Knight, a fallen paladin: in full plate, */
    SP_BKNIGHT2_A, SP_BKNIGHT2_B, SP_BKNIGHT2_W, SP_BKNIGHT2_S, /* ... half of it hacked off, */
    SP_BKNIGHT3_A, SP_BKNIGHT3_B, SP_BKNIGHT3_W, SP_BKNIGHT3_S, /* ... the breastplate cracked and the helm gone */
    SP_BKNIGHT_P0, SP_BKNIGHT_P1, SP_BKNIGHT_P2,                /* planting its sword, in each state */
    SP_BSPIDER_REAR, SP_BSPIDER_D,                              /* the Wandering Spider, an old matriarch: rearing to spit, dead */
    SP_BDEACON_A, SP_BDEACON_B, SP_BDEACON_CHANT, SP_BDEACON_D, /* the Black Deacon, a profane bishop: walking, chanting, fallen */
    /* the props' other looks (docs/InitialArtPass.md section 14; crawl_alt) */
    SP_BONES1_2, SP_BONES1_3, SP_BONES2_2, SP_SKULLPILE_2, SP_SKULLPILE_3, SP_TOTEM2_A, SP_TOTEM2_B,
    SP_CANDLES_2, SP_CANDLES_3,
    SP_TOMB_ROUND_2, SP_TOMB_CROSS_2, SP_TOMB_OBELISK_2, SP_TOMB_BROKEN_2,
    SP_CHAINS_2, SP_ROOTS_2, SP_COBWEB_2, SP_WEBSHEET_2,   /* what hangs, their other looks */
    SP_PILLAR_2, SP_URN_2, SP_URN_3,            /* stone and floor, their other looks */
    /* the three new kinds (1.0.0b15, docs/PLAN-ai.md section 22) */
    SP_WHIP_A, SP_WHIP_B, SP_WHIP_HORN, SP_WHIP_CRACK, SP_WHIP_D,     /* the Whipper-In: walking, the horn, the whip's crack, fallen */
    SP_SEXTON_A, SP_SEXTON_B, SP_SEXTON_RING_A, SP_SEXTON_RING_B, SP_SEXTON_ICE, SP_SEXTON_D,   /* the Sexton: walking, ringing (up, down), the bell frozen, fallen */
    SP_LIGHT_A, SP_LIGHT_B, SP_LIGHT_STRIKE, SP_LIGHT_LISTEN, SP_LIGHT_D,   /* the Lightless: walking, the strike, listening, shattered */
    SP_WHIP_SA, SP_WHIP_SB, SP_SEXTON_SA, SP_SEXTON_SB, SP_LIGHT_SA, SP_LIGHT_SB,   /* their side views, walking, facing right */
    /* things that want a spell (1.0.0b16, docs/PLAN-play.md section 4), each waiting and done */
    SP_MK_PRICKET0, SP_MK_PRICKET1, SP_MK_COFFER0, SP_MK_COFFER1, SP_MK_CAGE0, SP_MK_CAGE1, SP_MK_RACK0, SP_MK_RACK1,
    SP_MK_URN0, SP_MK_URN1, SP_MK_COIL0, SP_MK_COIL1, SP_MK_VINE0, SP_MK_VINE1, SP_MK_LANTERN0, SP_MK_LANTERN1,
    SP_MK_BARRICADE0, SP_MK_BARRICADE1,
    /* step 6's (section 4.3, 4.6): a corpse-light, a rolling skull (two frames each), the heat-and-shatter seal (cold, hot,
     * broken), a bell (still, ringing), a winch, a hearth, a steam vent, run III's ore cart and lodestone, run II's ice */
    SP_MK_CLIGHT0, SP_MK_CLIGHT1, SP_MK_RSKULL0, SP_MK_RSKULL1, SP_MK_HSEAL0, SP_MK_HSEAL1, SP_MK_HSEAL2,
    SP_MK_BELL0, SP_MK_BELL1, SP_MK_WINCH0, SP_MK_WINCH1, SP_MK_HEARTH0, SP_MK_HEARTH1, SP_MK_VENT0, SP_MK_VENT1,
    SP_MK_CART0, SP_MK_CART1, SP_MK_LODE0, SP_MK_LODE1, SP_MK_ICE,
    /* step 8's (sections 4.4, 4.5): the tablet and its plinths, the turning reliquary (front, turning, open), the ward-stone,
     * the relay's lamp (dead, lit, frozen); lamp oil, loose masonry, the poison font, the Order's drum (each before and after) */
    SP_MK_TABLET, SP_MK_PLINTH0, SP_MK_PLINTH1, SP_MK_RELIQ0, SP_MK_RELIQ1, SP_MK_RELIQ2, SP_MK_WSTONE0, SP_MK_WSTONE1,
    SP_MK_RELAY0, SP_MK_RELAY1, SP_MK_RELAY2, SP_MK_CASK0, SP_MK_CASK1, SP_MK_PILLAR0, SP_MK_PILLAR1,
    SP_MK_PFONT0, SP_MK_PFONT1, SP_MK_DRUM0, SP_MK_DRUM1,
    /* 1.0.0b18.1 (docs/PLAN-b18-1.md, sections 5 and 6): the bell puzzles' four bells, still and swung (I silver on a yoke,
     * II bronze and waisted, III iron and wide-mouthed, IV the great bell in black iron); run II's dead bell; the soul-lamp
     * in each run's look, three a run: out, and lit in two frames (SP_WLAMP0 + 3 * run + state) */
    SP_PBELL0, SP_PBELL1, SP_PBELL2, SP_PBELL3, SP_PBELL0R, SP_PBELL1R, SP_PBELL2R, SP_PBELL3R, SP_BELL_DEAD,
    SP_WLAMP0, SP_WLAMP_LAST = SP_WLAMP0 + 11,
    /* 1.0.0b18, the minds shown (docs/PLAN-ai.md Part H): backs (walking), the roost asleep and stirring, the ambushes' signs */
    SP_SKEL_BKA, SP_SKEL_BKB, SP_SHIELD_BKA, SP_SHIELD_BKB, SP_BAT_ROOST, SP_BAT_ROOST2,
    SP_SIGN_RAKE, SP_SIGN_DRAG, SP_SIGN_SCREE, SP_SIGN_CINDER, SP_SIGN_EYES,
    SP_KNIGHT_BKA, SP_KNIGHT_BKB, SP_KNIGHT2_BKA, SP_KNIGHT2_BKB, SP_KNIGHT3_BKA, SP_KNIGHT3_BKB,   /* the knight's backs, by its plate */
    SP_WHIP_BKA, SP_WHIP_BKB, SP_DEACON_BKA, SP_DEACON_BKB, SP_SEXTON_BKA, SP_SEXTON_BKB,
    SP_SHAMB_BKA, SP_SHAMB_BKB, SP_DROWN_BKA, SP_DROWN_BKB, SP_GHOUL_BKA, SP_GHOUL_BKB, SP_HOUND_BKA, SP_HOUND_BKB, SP_IMP_BKA, SP_IMP_BKB, SP_GARG_BKA, SP_GARG_BKB,
    SP_HUNTER_BKA, SP_HUNTER_BKB, SP_BROOD_BKA, SP_BROOD_BKB,
    /* docs/PLAN-combat.md, two tracks on one branch (its section 17): each adds only above its own end mark */
    SP_SHIELD_RAISE, SP_SHIELD_RUSH,            /* C4: the shield-bearer's charge: the stamp, the shield up; behind it, at a run */
    SP_DROWN_SWELL, SP_DROWN_SPEW,              /* C4: the Drowned swelling, then spewing */
    SP_SPEW_A, SP_SPEW_B, SP_DUST_A, SP_DUST_B, /* C4: a gout of the spew; the dust of a stamp */
    /* the caster's track (C) ends */
    /* the dead's track (D): */
    /* the dead's track (D) ends */
    /* the tutorial (docs/PLAN-tutorial.md, section 7): level 2's rimed seal, the switch that holds the ice (rimed, thawed, spent),
     * and the map lying on the floor */
    SP_MK_RSEAL0, SP_MK_RSEAL1, SP_MK_RSEAL2, SP_MAPFLOOR,
    NSPR
};

#define HAND_W 160
#define HAND_H 144
#define CAT_W 128
#define CAT_H 128
#define FACE_W 32
#define FACE_H 40
#define FACE_FRAMES 30

/* Ranks: one ladder of colours for every family, strongest last (docs/PLAN-bestiary.md). A rank
 * redraws a creature's body ramp in another as it is drawn; shading stays inside the new ramp. */
enum { RK_I, RK_II, RK_III, RK_IV, RK_V, RK_GILDED, NRANK };    /* own colour, green, blue, red, black, gold */
enum {                                          /* which ramp a creature's body is drawn in */
    PF_BONE,                                    /* R_BONE: skeletons, shield-bearers, hounds, wall skulls */
    PF_FLESH,                                   /* R_WARM: shamblers, ghouls */
    PF_CHITIN,                                  /* R_IRON: web spiders, the brood; black already, so rank V goes pale */
    PF_HIDE,                                    /* R_WOOD: hunting spiders */
    PF_WING,                                    /* R_PURPLE: bats, which stop at rank IV */
    PF_CLAY,                                    /* R_BRICK: ember imps */
    PF_STONE,                                   /* R_STONE: gargoyles */
    PF_ARMOUR,                                  /* R_IRON: a knight's plate (its blade is R_GHOST, and stays steel); rank V darkens it */
    PF_ROBE,                                    /* R_PURPLE: the deacon's robe */
    PF_DROWN,                                   /* R_GHOST: the Drowned's waxen skin (docs/InitialArtPass.md: never the walls' stone) */
    NPF
};
extern uint8_t crawl_rank_lut[NPF][NRANK][256];

#define NALT 4
extern int16_t crawl_alt[NSPR][NALT];          /* a prop's looks: [0] itself, then others, -1 after; [1] -1: one look */
extern uint8_t crawl_pal[256][3];               /* 6-bit DAC values */
extern uint8_t crawl_cmap[NLIGHT][256];         /* colour at a light level */
extern uint8_t crawl_tex[NTEX][TS * TS];        /* column-major: [x * TS + y] */
extern uint8_t crawl_spr[NSPR][TS * TS];        /* column-major, C_CLEAR = transparent */
extern const uint8_t crawl_hand_img[HAND_H * HAND_W];  /* the caster's hand, row-major, C_CLEAR = transparent (hand_img.c) */
enum { HF_REST, HF_IDLE1, HF_IDLE2, HF_GATHER, HF_CLAW, HF_LOOSE, HF_AFTER, NHF };   /* the hand's frames: at rest, idling, a cast */
extern const int8_t crawl_hand_shift[NHF][2];      /* where each frame is drawn, against the hand at rest */
/* frame k of the hand from src (and its labels, lab: 0 none), into dst and dlab; tip: where a spell forms in it */
void crawl_hand_frame(int k, const uint8_t *src, const uint8_t *lab, uint8_t *dst, uint8_t *dlab, int tip[2]);
extern int8_t crawl_glint[NSPR][2];             /* brightest gold pixel of a sprite, for twinkles; -1 if none */
/* each sprite's foot (1.0.0b18.1, docs/PLAN-b18-1.md section 8): how wide the lowest eighth of its drawn rows is, in texels,
 * measured at the end of crawl_art_level, after the props are drawn; a prop's is its widest frame or look (crawl_prop_foot),
 * and it takes the whole of its cell, the half along its wall or the quarter in its corner by crawl_foot_part */
extern uint8_t crawl_foot[NSPR];
enum { FOOT_WHOLE, FOOT_HALF, FOOT_QUARTER };
#define FOOT_WHOLE_W 45                         /* 0.7 of a cell, and up */
#define FOOT_HALF_W 29                          /* 0.45 */
int crawl_prop_foot(int spr, int nfr);
int crawl_foot_part(int w);
/* where a creature's eyes are on its sprite (docs/InitialArtPass.md section 5), recorded as the sprite is drawn (cv_eye),
 * for the pass that puts them on the screen whatever the light and fog: n eyes (0 none), at x, y in the sprite */
typedef struct { int8_t n, x[2], y[2]; } EyeAt;
extern EyeAt crawl_eyes[NSPR];
/* each creature's dressing by rank (docs/InitialArtPass.md section 6): a layer over its sprite, C_CLEAR where the body
 * shows, drawn in its own stuff (cloth, iron, frost, blood, gold) and left alone by the rank's tint; ix -1 none */
#define NDRESS 768
extern uint8_t crawl_dress[NDRESS][TS * TS];            /* column-major, as the sprites */
extern int16_t crawl_dress_ix[NSPR][NRANK];
/* the merchant (cat_img.c): the owner's cat draped over a chest, with what the game needs to animate it */
extern const uint8_t crawl_cat_img[CAT_H * CAT_W];
extern const uint8_t crawl_cat_eye[2][4], crawl_cat_eyecol[5], crawl_cat_lid[2], crawl_cat_ear[2][4], crawl_cat_tail[2];
/* the caster's face for the status bar (face_img.c): frame = stage * 4 + {ahead, left, right, grimace}; 28 grin, 29 dead */
extern const uint8_t crawl_face_img[FACE_FRAMES][FACE_H * FACE_W];
extern const uint8_t crawl_face_eyes[FACE_FRAMES][2][2];

/* The descent: four runs of ten levels, a set each, worn down slowly towards the last (the boss's
 * level), then the four again, deeper (docs/PLAN-b7.md). */
#define RUN_LEN 10
#define DESCENT_LEN (4 * RUN_LEN)
static inline int crawl_run(int level) { return (level - 1) / RUN_LEN; }     /* 0 run I, 1 run II ..., counting every time down */
static inline int crawl_set(int level) { return crawl_run(level) % 4; }     /* 0 the Crypts, 1 the Catacombs, 2 the Hollows, 3 the Deep */
static inline int crawl_step(int level) { return (level - 1) % RUN_LEN; }   /* 0..9 within the run; 9 is its boss's */
static inline int crawl_drawn(int level) { return crawl_set(level) <= 3; }  /* a run whose own look is drawn (docs/PLAN-b10.md): all four now; kept so one can be held back while it is redrawn */
static inline int crawl_boss(int level) { return crawl_step(level) == RUN_LEN - 1; }
static inline int crawl_loop(int level) { return (level - 1) / DESCENT_LEN; }   /* 0 the first time down */
static inline float crawl_decay(int level) { return (float)crawl_step(level) / (float)(RUN_LEN - 1); }   /* rot and ruin: fresh at a run's start */
static inline float crawl_depth(int level) {                                /* how far down: darker, fewer torches */
    float d = (float)(crawl_set(level) + 4 * crawl_loop(level)) / 3.0f;
    return d < 1.0f ? d : 1.0f;
}

extern const char *const RUNE_GLYPH[9][7];      /* the runes' marks, 5 by 7, by DF_* (the hand's tattoos, the shrine) */
extern const char *const ELEM_GLYPH[6][7];      /* the elements' glyphs, 7 by 7, by EL_*: '#' the colour, 'o' bright (1.0.0b11) */
enum { MK_WAIT, MK_HIT, MK_WRONG, MK_DONE };      /* a marked thing's mark (1.0.0b16): waiting, just struck, the wrong spell, done */
void crawl_mark(uint8_t *buf, int w, int h, int cx, int cy, int el, int state, int pips, int lit_n, float t);
uint8_t crawl_ice_at(float wx, float wy, float k, uint8_t under);   /* the tutorial's ice over a floor texel, k of the way laid */   /* row-major, at full brightness */
extern const char *const RUNE_MARK[9][11];      /* the runes' marks for their sockets, 11 by 11, by DF_* */
extern const char *const CLASS_MARK[6][11];     /* the classes' marks, 11 by 11, by the class's element */
extern const char *const PZ_MARKS[16][7];       /* the marks carved on puzzles' things, 7 by 7 (1.0.0b13): mitre, helm, tonsure, crown,
                                                   key, wheel, arrows, tower, lamb, lamp, eye, hand, picks, moon, star, skull */
extern const signed char LAMP_BOWL[6][2];       /* where the Lampstand's bowls sit on its sprite, by EL_* */
int crawl_lamp_flame(int dx, int dy, int rank, float t, int seed);   /* a bowl's flame: its ramp's step there (0 none) */
void crawl_art_init(void);                      /* builds everything once */
void crawl_art_level(int level);                /* that level's palette, ruin and rot (call after init) */
enum { PT_NONE, PT_BACKROOMS, PT_SEVERANCE };   /* a pocket's theme */
void crawl_art_theme(int theme, int level);     /* the palette inside a pocket of that theme, or the level's again */

/* Each run's look (docs/PLAN-b10.md): what its materials, blends, features, floors and ceilings are, by
 * slot. A blend joins two materials, the first on its left and in front (the second lies behind it). */
typedef struct {
    const char *name;
    const char *mat[NMAT], *feat[NFEAT], *floor[NFLOORS], *ceil[NCEILS];
    signed char blend[NBLEND][2];
} RunLook;
extern const RunLook crawl_looks[4];
void crawl_art_run(int level);                  /* that run's own textures and keys, at the level's rot (after crawl_art_level) */
void crawl_art_stone(int level, uint8_t *wall, uint8_t *floor);   /* a run's main stone and first floor alone (docs/PLAN-portraits.md) */

/* A document's page (1.0.0b13, docs/PLAN-b13.md section 6a), a run each: a leaf of the Order's vellum, a letter's paper,
 * a scrap of sacking, beaten gold leaf. Drawn into pix (stride across) at x0, y0, w by h, left as it was where it is torn
 * away (run + 4: a slip, without the vellum's square for a first letter); returns the colour to write on it in.
 * crawl_doc_ink finishes what was written on it: the letter's ink runs, the gold's scratches catch the light.
 * crawl_doc_paper says whether a colour is the page's own, for the writer. */
unsigned char crawl_doc_page(unsigned char *pix, int stride, int x0, int y0, int w, int h, int run, unsigned seed);
void crawl_doc_ink(unsigned char *pix, int stride, int x0, int y0, int w, int h, int run, unsigned char ink, unsigned seed);
int crawl_doc_paper(int run, unsigned char c);
/* a trial's hourglass (1.0.0b13, section 8), 21 by 33, into pix (stride across) at x0, y0: sand the share still above
 * (1 full, 0 run out), state 0 running, 1 broken (won), 2 cracked and empty (lost); t a clock for the falling grains */
void crawl_hourglass(unsigned char *pix, int stride, int x0, int y0, float sand, int state, float t);

#endif
