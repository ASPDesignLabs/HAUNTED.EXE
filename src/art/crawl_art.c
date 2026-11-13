/* Procedural art for the CATACOMB raycaster: palette, light colormap,
 * 64x64 wall textures, sprites and the caster's hand. */
#include "crawl.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

uint8_t crawl_pal[256][3];
uint8_t crawl_cmap[NLIGHT][256];
uint8_t crawl_tex[NTEX][TS * TS];
uint8_t crawl_spr[NSPR][TS * TS];
int8_t crawl_glint[NSPR][2];

/* From here the file is cut into parts (docs/PLAN-tidy.md), each #included in the place it was written in. It is still one
 * translation unit: a part sees everything in the parts above it, and none is compiled on its own. The parts are in
 * parts/. This list is the map: to find a thing, read down it. */

/* The palette's ramps, the light colormap, and noise (hsh, vnoise, fbm) */
#include "parts/palette.c"

/* Wall textures: stone, rubble, brick, moss, the skull wall, niches, banners, torches, doors and locks, gates, the ice, the Portal, fonts, the sign, a secret's passage, the sealed doorway, the Repository */
#include "parts/walls.c"

/* The pockets' walls, floors and ceilings: the Backrooms and Severance; the exit */
#include "parts/pockets.c"

/* The sprite canvas (cv_*), the master skull and its stamps; the caster's hand in motion (crawl_hand_frame) */
#include "parts/canvas.c"

/* Flame; the skeleton and the shield-bearer; the dressing a rank puts on a creature (kit_*); their backs; the wall skull; the web spider */
#include "parts/skeleton.c"

/* Spells and what they leave (bolts, shards, the spear, wisps, the flask, the orb, the lance, rime, motes, burns); the pedestal and the items; the altar's fire; the Rune Shrine and its runes; the Cursed Coin */
#include "parts/spells.c"

/* The runs' hazards: what stands in the wall, and their sprites (grit, the censer, votives, snares, spikes, ash) */
#include "parts/hazards.c"

/* The puzzles' things; the things a message hides among; the Black Shrine */
#include "parts/puzzles.c"

/* A document's page and its ink; the bosses' hazards (rock, rubble, the eruption, the void, the hands); the scrip; the Hexed Totem */
#include "parts/documents.c"

/* Things that want a spell: run I's and one for each line, the moving and timed ones, sequences and things for fights; crawl_mark (the seal and its motes) */
#include "parts/marked.c"

/* Her hall's things: the Lampstand, the Dead Bell, the Weeping Font, the lectern; the tutorial's rimed seal and its map; the map on sale; Severance's desks and cooler; the caster's double */
#include "parts/hall.c"

/* The spectre and the bat; fire, the blast, a bone in flight; gems and the key; a chain, the tombstone */
#include "parts/first_kinds.c"

/* The bestiary begins: rot and soil; the shambler and the Drowned; the shield-bearer's charge, the spew, dust */
#include "parts/risen.c"

/* The ghoul, the bone hound, the skeleton knight, the deacon, with their backs and dressing */
#include "parts/hunters.c"

/* The Ill-Fated Knight and the Black Deacon */
#include "parts/bosses.c"

/* The Whipper-In, the Sexton and the Lightless, with their side views; the bestiary's bones */
#include "parts/newkinds.c"

/* The hunting spider; the Wandering Spider; the brood mother, her sacs and spiderlings */
#include "parts/spiders.c"

/* The Wraith, the shade, the ember imp and its ember, the gargoyle */
#include "parts/wraith.c"

/* The dressing that comes in as you go down: carcasses, armour stands, timber, mine lamps, carts, fungus, crystal, magma, obsidian, the altar */
#include "parts/dressing.c"

/* Side views: each creature walking in profile; the glints found */
#include "parts/sides.c"

/* Ranks' colours (build_rank_luts); each level's age: decay_tex, level_palette; a pocket's theme (crawl_art_theme) */
#include "parts/ageing.c"

/* Props: bones, litter, heaps and skull stacks; the totem post; candles, prickets, the corona, hearths, braziers, lanterns */
#include "parts/props_bones.c"

/* Props: graves and what is carved on them; chains, manacles, the hook beam, the gibbet, the bronze bell */
#include "parts/props_graves.c"

/* The bell puzzles' four bells, run II's dead bell, the soul-lamp */
#include "parts/props_bells.c"

/* Props: hanging roots, webs and cocoons, ice, the gothic pillar, flowstone, the altar stone, urns, broken arms, the spiked collar; the roost asleep */
#include "parts/props_growth.c"

/* The ambushes' signs; the pool; treasure (coins, the chalice, gems, the crown, the chest), the phial, the soul urn, the ring; props_level; footprints measured; crawl_art_level */
#include "parts/treasure.c"

/* Each run's own look begins: the course grid; the Crypts' materials, blends, what stands on their walls, floors, ceilings, doors and keys */
#include "parts/run_crypts.c"

/* The Catacombs' materials, what stands on their walls, floors and ceilings, doors, keys and treasure */
#include "parts/run_catacombs.c"

/* The Hollows' materials, what stands on their walls, floors and ceilings, doors, keys and treasure */
#include "parts/run_hollows.c"

/* The Deep's materials, what stands on its walls, floors, ceilings, doors, keys and treasure */
#include "parts/run_deep.c"

/* Each run's set made (crypts_run, catacombs_run, hollows_run, deep_run); the specials on the run's stone; the trials' doors and things; the places to mend; crawl_art_run, crawl_art_stone, crawl_art_init */
#include "parts/runs.c"
