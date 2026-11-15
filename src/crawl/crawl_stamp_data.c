/* Stamps' data (docs/PLAN-stamps.md, track B): the owner's ten secret rooms and ten joiners as drawn, and the smaller versions that
 * sit beside them, one StampDef each, in the order drawn. The glyphs are in crawl_stamp.h. Every stamp is stored once; the loader
 * makes the eight turns and mirrors. A secret's box takes in its entrance (E) and doorway (D) cells. The owner's drawings are in
 * media/sheets/stamps/; the sheet tool (tools/stampsheet.c) draws these and checks them. Pure data: nothing here is used by the
 * game until the owner has approved the sheet.
 *
 * Counts per level and each stamp's steps, weights and runs are defaults, the owner's to change. */
#include "crawl_stamp.h"

const StampDef STAMP_DEFS[] = {
    /* ---- the owner's secret rooms (owner-secret-areas.png) ---- */
    /* S1: 3x3 floor; entrances top and left, doorway at the bottom right */
    { "secret_box", ST_SECRET, 4, 5, 0, 9, 6, 0,
    "##E#\n"
    "#...\n"
    "E...\n"
    "#...\n"
    "###D" },
    /* S2: 4x3 floor; entrances top and left, doorway on the right */
    { "secret_wide", ST_SECRET, 6, 4, 0, 9, 6, 0,
    "##E###\n"
    "#....#\n"
    "E....#\n"
    "#....D" },
    /* S3: 7x2 floor; entrances top and left, doorway at the far end */
    { "secret_strip", ST_SECRET, 9, 3, 0, 9, 6, 0,
    "##E######\n"
    "E.......#\n"
    "#.......D" },
    /* S4: L: a 2-wide leg of 3 and a foot of 4x2; doorway at the foot's end */
    { "secret_l", ST_SECRET, 6, 6, 0, 9, 4, 0,
    "#E####\n"
    "E..###\n"
    "#..###\n"
    "#..###\n"
    "#....D\n"
    "#....#" },
    /* S5: L turned: a 4x2 bar and a 2-wide leg of 3; doorway at the leg's foot */
    { "secret_l_turned", ST_SECRET, 5, 7, 0, 9, 4, 0,
    "#E###\n"
    "E....\n"
    "#....\n"
    "###..\n"
    "###..\n"
    "###..\n"
    "###D#" },
    /* S6: arrowhead, stepped; doorway at the point */
    { "secret_arrow", ST_SECRET, 6, 7, 0, 9, 4, 0,
    "#E####\n"
    "E..###\n"
    "#...##\n"
    "#....D\n"
    "#....#\n"
    "#...##\n"
    "#..###" },
    /* S7: C: the doorway is in the notch, on the inner face of the lower arm */
    { "secret_c", ST_SECRET, 6, 7, 2, 9, 3, 0,
    "#E####\n"
    "E.....\n"
    "#.....\n"
    "####..\n"
    "##D#..\n"
    "#.....\n"
    "#....." },
    /* S8: spiral with an inner stub; doorway at the far foot */
    { "secret_spiral", ST_SECRET, 6, 7, 2, 9, 3, 0,
    "#E####\n"
    "E.....\n"
    "#..#..\n"
    "#..#..\n"
    "####..\n"
    "#.....\n"
    "D....." },
    /* S9: J: a 1-wide leg of 4 and a foot of 4x2; doorway at the foot's west end */
    { "secret_j", ST_SECRET, 5, 7, 2, 9, 3, 0,
    "####E\n"
    "####.\n"
    "####.\n"
    "####.\n"
    "####.\n"
    "D....\n"
    "#...." },
    /* S10: 1-wide approach of 9, then a 6x2 foot with a step; doorway on the raised part */
    { "secret_long", ST_SECRET, 6, 12, 4, 9, 2, 0,
    "E#####\n"
    ".#####\n"
    ".#####\n"
    ".#####\n"
    ".#####\n"
    ".#####\n"
    ".#####\n"
    ".#####\n"
    ".###D#\n"
    ".##...\n"
    "......\n"
    "......" },

    /* ---- joiners (owner-joiners.png): ports N and S must connect, W and E may be walled off, unless said ---- */
    /* J1, a cruciform monument; 16 drawn, regridded to 19 (stem 1 wide, crossbar 5) */
    { "joiner_cross_19", ST_JOINER, 19, 19, 2, 9, 3, 0,
    "########PPP########\n"
    "########.#.########\n"
    "######...#...######\n"
    "######...#...######\n"
    "####.....#.....####\n"
    "####.....#.....####\n"
    "##.......#.......##\n"
    "##.......#.......##\n"
    "p........#........p\n"
    "p......#####......p\n"
    "p........#........p\n"
    "##.......#.......##\n"
    "##.......#.......##\n"
    "####.....#.....####\n"
    "####.....#.....####\n"
    "######...#...######\n"
    "######...#...######\n"
    "########.#.########\n"
    "########PPP########" },
    /* J1 at 11 (crossbar 3) */
    { "joiner_cross_11", ST_JOINER, 11, 11, 0, 9, 5, 0,
    "####PPP####\n"
    "####.#.####\n"
    "##...#...##\n"
    "##...#...##\n"
    "p....#....p\n"
    "p...###...p\n"
    "p....#....p\n"
    "##...#...##\n"
    "##...#...##\n"
    "####.#.####\n"
    "####PPP####" },
    /* J2, a ring road round a cross; 16 drawn, regridded to 19 */
    { "joiner_ring_19", ST_JOINER, 19, 19, 2, 9, 3, 0,
    "########PPP########\n"
    "########...########\n"
    "######.......######\n"
    "######.......######\n"
    "####...........####\n"
    "####...#####...####\n"
    "##.....#####.....##\n"
    "##...#########...##\n"
    "p....#########....p\n"
    "p....#########....p\n"
    "p....#########....p\n"
    "##...#########...##\n"
    "##.....#####.....##\n"
    "####...#####...####\n"
    "####...........####\n"
    "######.......######\n"
    "######.......######\n"
    "########...########\n"
    "########PPP########" },
    /* J2 at 11 */
    { "joiner_ring_11", ST_JOINER, 11, 11, 0, 9, 5, 0,
    "####PPP####\n"
    "####...####\n"
    "##.......##\n"
    "##..###..##\n"
    "p..#####..p\n"
    "p..#####..p\n"
    "p..#####..p\n"
    "##..###..##\n"
    "##.......##\n"
    "####...####\n"
    "####PPP####" },
    /* J3, four L pillars with corner pockets; 16 drawn, regridded to 19 (aisles 3 wide) */
    { "joiner_pillars_19", ST_JOINER, 19, 19, 3, 9, 3, 0,
    "########PPP########\n"
    "########...########\n"
    "######.......######\n"
    "######.......######\n"
    "####...#...#...####\n"
    "####...#...#...####\n"
    "##.....#...#.....##\n"
    "##..####...####..##\n"
    "p.................p\n"
    "p.................p\n"
    "p.................p\n"
    "##..####...####..##\n"
    "##.....#...#.....##\n"
    "####...#...#...####\n"
    "####...#...#...####\n"
    "######.......######\n"
    "######.......######\n"
    "########...########\n"
    "########PPP########" },
    /* J3 at 11 (aisles 1 wide, pillars of 3 cells) */
    { "joiner_pillars_11", ST_JOINER, 11, 11, 0, 9, 5, 0,
    "####PPP####\n"
    "####...####\n"
    "##.......##\n"
    "##..#.#..##\n"
    "p..##.##..p\n"
    "p.........p\n"
    "p..##.##..p\n"
    "##..#.#..##\n"
    "##.......##\n"
    "####...####\n"
    "####PPP####" },
    /* J4, a bend joining two halls; 12 drawn, trimmed to 11 by the centre column; ports at the feet */
    { "joiner_chevron_11", ST_JOINER, 11, 7, 0, 9, 5, 0,
    "####...####\n"
    "###.....###\n"
    "##.......##\n"
    "#.........#\n"
    ".....#.....\n"
    "....###....\n"
    "#PP#####PP#" },
    /* J5, a transept hub with a small cross core; 16 drawn, regridded to 19 */
    { "joiner_transept_19", ST_JOINER, 19, 19, 2, 9, 3, 0,
    "########PPP########\n"
    "#######.....#######\n"
    "#######.....#######\n"
    "#######.....#######\n"
    "#######.....#######\n"
    "#####.........#####\n"
    "#####.........#####\n"
    "#.......###.......#\n"
    "p......#####......p\n"
    "p......#####......p\n"
    "p......#####......p\n"
    "#.......###.......#\n"
    "#####.........#####\n"
    "#####.........#####\n"
    "#######.....#######\n"
    "#######.....#######\n"
    "#######.....#######\n"
    "#######.....#######\n"
    "########PPP########" },
    /* J5 at 11 */
    { "joiner_transept_11", ST_JOINER, 11, 11, 0, 9, 5, 0,
    "####PPP####\n"
    "####...####\n"
    "####...####\n"
    "##.......##\n"
    "p....#....p\n"
    "p...###...p\n"
    "p....#....p\n"
    "##.......##\n"
    "####...####\n"
    "####...####\n"
    "####PPP####" },
    /* J6, a thin-armed star; 16 drawn, regridded to 19 (arms 3 wide) */
    { "joiner_star_19", ST_JOINER, 19, 19, 2, 9, 3, 0,
    "########PPP########\n"
    "########...########\n"
    "########...########\n"
    "########...########\n"
    "########...########\n"
    "#####.........#####\n"
    "#####.........#####\n"
    "#####...###...#####\n"
    "p......#####......p\n"
    "p......#####......p\n"
    "p......#####......p\n"
    "#####...###...#####\n"
    "#####.........#####\n"
    "#####.........#####\n"
    "########...########\n"
    "########...########\n"
    "########...########\n"
    "########...########\n"
    "########PPP########" },
    /* J6 at 11 (arms 1 wide) */
    { "joiner_star_11", ST_JOINER, 11, 11, 0, 9, 5, 0,
    "#####P#####\n"
    "#####.#####\n"
    "#####.#####\n"
    "###.....###\n"
    "###..#..###\n"
    "p...###...p\n"
    "###..#..###\n"
    "###.....###\n"
    "#####.#####\n"
    "#####.#####\n"
    "#####P#####" },
    /* J7, a T-junction; 22x7 drawn, regridded to 19x7 (stub 3 wide); ports at the wingtips and the stub */
    { "joiner_tee_19", ST_JOINER, 19, 7, 1, 9, 4, 0,
    "########ppp########\n"
    "P......#...#......P\n"
    "P......#...#......P\n"
    "####...........####\n"
    "#####.........#####\n"
    "######.......######\n"
    "#######.....#######" },
    /* J8, a fork; 14x10 drawn, regridded to 11x11; ports at the feet and the apex */
    { "joiner_fork_11", ST_JOINER, 11, 11, 0, 9, 5, 0,
    "####ppp####\n"
    "####...####\n"
    "##.......##\n"
    "##.......##\n"
    "...........\n"
    "...........\n"
    "...#####...\n"
    "...#####...\n"
    "...#####...\n"
    "...#####...\n"
    "PPP#####PPP" },
    /* J9, the great nave; 35x10 drawn, one row added (11 tall) so its end ports sit on a block centre; at most one a level */
    { "joiner_nave_35", ST_JOINER, 35, 11, 5, 9, 1, 0,
    "................ppp................\n"
    ".#########..##########..##########.\n"
    "...................................\n"
    "............#........#.............\n"
    "P#..........#........#...........#P\n"
    "P#..........#........#...........#P\n"
    "P#..........#........#...........#P\n"
    "............#........#.............\n"
    "...................................\n"
    ".#########..##########..##########.\n"
    "................ppp................" },
    /* J10, a processional gallery; 3x16 drawn, trimmed to 15 */
    { "joiner_gallery_15", ST_JOINER, 3, 15, 0, 9, 4, 0,
    "PPP\n"
    "...\n"
    "...\n"
    "...\n"
    "...\n"
    "...\n"
    "...\n"
    "...\n"
    "...\n"
    "...\n"
    "...\n"
    "...\n"
    "...\n"
    "...\n"
    "PPP" },
    /* ---- ours, for the owner to approve (docs/PLAN-stamps.md, B3): more joiners ---- */
    /* ours: ladder, two halls joined by two 1-wide rungs */
    { "joiner_ladder_15", ST_JOINER, 15, 11, 1, 9, 4, 0,
    "PPP#########PPP\n"
    "...#########...\n"
    "...#########...\n"
    "...............\n"
    "...#########...\n"
    "...#########...\n"
    "...#########...\n"
    "...............\n"
    "...#########...\n"
    "...#########...\n"
    "ppp#########ppp" },
    /* ours: horseshoe arc, a bend round solid stone; both feet connect */
    { "joiner_horseshoe_15", ST_JOINER, 15, 11, 0, 9, 4, 0,
    "###.........###\n"
    "#.............#\n"
    "...............\n"
    "...#########...\n"
    "...#########...\n"
    "...#########...\n"
    "...#########...\n"
    "...#########...\n"
    "...#########...\n"
    "...#########...\n"
    "PPP#########PPP" },
    /* ours: hourglass, two chambers and a 1-wide neck (for a boss, a vault or a trial) */
    { "joiner_hourglass_11", ST_JOINER, 11, 11, 0, 9, 4, 0,
    "##..PPP..##\n"
    "...........\n"
    "...........\n"
    "#.........#\n"
    "#####.#####\n"
    "#####.#####\n"
    "#####.#####\n"
    "#.........#\n"
    "...........\n"
    "...........\n"
    "##..PPP..##" },
    /* ours: dogleg, a 3-wide diagonal joining halls a block-width apart */
    { "joiner_dogleg_15", ST_JOINER, 15, 15, 0, 9, 4, 0,
    "PPP############\n"
    "...############\n"
    "#...###########\n"
    "##...##########\n"
    "###...#########\n"
    "####...########\n"
    "#####...#######\n"
    "######...######\n"
    "#######...#####\n"
    "########...####\n"
    "#########...###\n"
    "##########...##\n"
    "###########...#\n"
    "############...\n"
    "############PPP" },
    /* ours: broken ring, a ring road pinched to 1 wide at two corners, with four spokes */
    { "joiner_broken_ring_19", ST_JOINER, 19, 19, 2, 9, 3, 0,
    "########PPP########\n"
    "########...########\n"
    "########...########\n"
    "########...########\n"
    "####...........####\n"
    "####........##.####\n"
    "####........##.####\n"
    "####...#####...####\n"
    "p......#####......p\n"
    "p......#####......p\n"
    "p......#####......p\n"
    "####...#####...####\n"
    "####.##........####\n"
    "####.##........####\n"
    "####...........####\n"
    "########...########\n"
    "########...########\n"
    "########...########\n"
    "########PPP########" },
    /* ours: pinwheel, four blades turning the way in to a core room */
    { "joiner_pinwheel_11", ST_JOINER, 11, 11, 0, 9, 4, 0,
    "####PPP####\n"
    "####...####\n"
    "##....#..##\n"
    "##....#..##\n"
    "p.###.#...p\n"
    "p.........p\n"
    "p...#.###.p\n"
    "##..#....##\n"
    "##..#....##\n"
    "####...####\n"
    "####PPP####" },
    /* ours: trident, three prongs from one wall */
    { "joiner_trident_11", ST_JOINER, 11, 11, 0, 9, 4, 0,
    "ppp#PPP#ppp\n"
    "...#...#...\n"
    "...#...#...\n"
    "...#...#...\n"
    "...#...#...\n"
    "...#...#...\n"
    "...#...#...\n"
    "...........\n"
    "...........\n"
    "...........\n"
    "....ppp...." },
    /* ours: small octagon with a pillar; two ports off its axis connect */
    { "joiner_octagon_7", ST_JOINER, 7, 7, 0, 9, 5, 0,
    "#PP...#\n"
    "p......\n"
    "p......\n"
    "...#...\n"
    "......p\n"
    "......p\n"
    "#...PP#" },
    /* ours: small octagon with a central pillar */
    { "joiner_octagon_11", ST_JOINER, 11, 11, 0, 9, 5, 0,
    "###.PPP.###\n"
    "#.........#\n"
    "...........\n"
    "...........\n"
    "p...###...p\n"
    "p...###...p\n"
    "p...###...p\n"
    "...........\n"
    "...........\n"
    "#.........#\n"
    "###.PPP.###" },
};

const int NSTAMP_DEFS = (int)(sizeof STAMP_DEFS / sizeof STAMP_DEFS[0]);
