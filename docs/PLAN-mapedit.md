# PLAN-mapedit: a map editor for drawn levels

The owner asked for a tile editor to draw CATACOMB's levels by hand: "The generator is neat and all but I think it is time for some
personal direction." It is an artifact on claude.ai, so it opens anywhere, and it writes the drawn levels' own letters
(`src/crawl/game/drawn.c`), so a map is the level cell for cell.

## 1. What the owner settled

1. **The lattice by default.** The editor snaps stamps to the generator's blocks (a block's first cell at 4n + 1) and gives the halls'
   middle lanes (x or y = 2 mod 4) on its own. It can be turned off for a free drawing like the tutorial's, whose lanes are painted.
2. **Into the game by pasting into `drawn.c`.** The export is a `DRAWNn_MAP`, `_THING` and `_ROOM` block and its row in `DRAWN[]`.
3. **The dream's walk in the editor is a copy of its rules**, not the game's code: it shows what can be reached and in what order;
   a run in the harness stays the proof.
4. **Spawn points are filled by the budget.** The owner sets the point and, when wanted, the creature, its chance, its rank and its
   resistance.
5. **"Resistance" is the chance of being born to an element**, the owner confirmed (asked as "the ward chance"). Wards were replaced
   in 1.0.0b19: `elem_mode()` is on, `ward_give` returns before a ward is rolled, and a creature is born to an element by rank
   (`elem_roll`: II 15%, III 25%, IV 35%, V 45%), each run favouring two. The editor sets that chance and the element.
6. **The owner picks the level a map is for.** Each map carries its level number; part 2 makes `drawn_level()` a table.
7. **An in-game editor is wanted later**, once this one has been tried.
8. A retro look, like the game's mode 13h, was asked for as bonus points: the editor is cut in the game's carved stone and gold
   capitals, with pixel tiles.

## 2. Part 1: the editor (built)

| File | What |
|---|---|
| `tools/mapedit/extract.py` | Reads the game's source into `data.json`: the 40 stamps, the 24 kinds (name, first level, top rank, cost), the budget table, gold values, room types, elements, and both tutorial levels |
| `tools/mapedit/core.js` | The logic, no drawing: the map, stamps laid and turned, letters both ways, the checks, the budgets, the dream's walk |
| `tools/mapedit/editor.html` | The page |
| `tools/mapedit/build.py` | Puts `core.js` and the data into the page: `python3 -I tools/mapedit/build.py` writes `build/mapedit/catacomb-map-editor.html`, the file published as the artifact |
| `tools/mapedit/test.js` | `node tools/mapedit/test.js`: both tutorial levels in and out again must match `drawn.c` exactly; then their checks and the dream |

The map is the whole 45 x 45 (`MAPN`); the export trims it to the box of what is not stone, which gives `w`, `h`, `ox` and `oy`.
Stamps stay stamps (movable, turnable) until baked into the map. A secret stamp's floor is the secret room (`q`), its chosen entrance
the cracked wall (`c`), its other entrances and its doorway stone. Things sit at the middle of a quarter cell. A gold thing on a
secret room's cell exports as `DT_SGOLD`.

The checks come from the code they protect: `gen_drawn`, `drawn_walls`, `drawn_items`, `stamp_valid`, `place_creatures`'s `set_clear`
and `place_stamp_spawns`'s eight steps, and the limits (`DFIRE_CELLS` 16, `ICE_CELLS` 24, 12 rooms, 2 places to mend, 96 items,
48 heads). One secret a drawn level: `gen_drawn` takes every `q` as one box.

Storage: the artifact's `db`, read and written at `admin` (the owner, and editors if ever invited), `maps/<id>` and a save point at
`maps/<id>/versions/<time>` on every save; a draft in the browser after every change; backup files through `downloads`.

## 3. Part 2: the game's side (not begun)

- `DrawnSpawn { float x, y; int kind, chance, echance, elem, rank; }` and `const DrawnSpawn *spawn; int nspawn;` on `Drawn`; spent from
  the level's budget (`budget_of`) as `place_stamp_spawns` does, kind -1 the budget's choice, rank -1 the level's usual.
- Let a spawn point's creature be born to an element on a drawn level: today `weak_el` returns 1 for every element there (T6 a).
- `drawn_level()` as a table of level numbers to slots, in place of "levels 1 and 2 of the first descent".
- Proof that nothing else moved: the dream's log on generated levels line for line against the build before, and the saves.

## 4. Later

The in-game editor (decision 7). The stamps' altar, brazier, loot and torch marks on drawn levels.

## 5. Hand-off log

**Session 1 (23 November 2026).** Part 1 built. The round trip is exact for both tutorial levels (every row, thing, room, vault and
seed). Both pass the checks; the dream finishes both (level 1 in 220 steps, level 2 in 249, its secret's four treasures missed until
the secret is pressed). The budget panel shows the tutorial levels hold more dead than the generator's budget (11 against 4.3 on level
1): a fact about the drawings. Tested in Playwright's Chromium (no page errors; no sideways scroll at 400 px) and by `test.js`. Not yet
used by the owner. Part 2 not begun.
