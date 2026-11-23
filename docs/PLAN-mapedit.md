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

## 3. Part 2: the game's side (built)

- **The level table.** `Drawn` carries `level`, the level it is laid on the first time down, and `drawn_level()` looks it up in
  place of "levels 1 and 2". The tutorial's rows say 1 and 2, so they lie where they did.
- **Spawn points.** `DrawnSpawn { float x, y; int kind, chance, echance, elem, rank; }`, `spawn` and `nspawn` on `Drawn`;
  `drawn_spawns()` after `drawn_dead()`. A point whose kind the owner chose comes whenever its chance falls, budget or not; a point
  left to the budget (`kind` -1) takes `random_type()` and comes only while `budget_of` (times 1.4 corrupted, half on a boss's level)
  can pay for it at rank I, less what is already set (`set_spent`). 48 heads at most. Rank -1 is `spawn_mob`'s own roll. With no
  spawn points no dice are thrown, so a level without them lies as before.
- **Resistance.** `Mob.dsp`, `dsp_ech`, `dsp_el` (creatures are never saved, so saves are untouched); `elem_roll` takes the point's
  chance and element in place of the rank's and the run's favourites.
- **The tutorial's alone.** `DRAWN_TUT` 2: the first two drawings are the tutorial's. Their lessons (`tu_due`, `tu_trig`,
  `tu_cleared` by slot) and their rule that the dead take every element alike (`weak_el`, T6 a) stay theirs.
- **Ice and the gate on any drawn level.** The checks that read `g_drawn == 2` for the ice and the gate (`ice_at`, `hazards.c`,
  `draw/floor.c`, `abilities.c`, `draw/walls.c`) read any drawn level; on the tutorial that is the same test, level 1 having neither.
  The ice coming back when the map is taken stays level 2's lesson (`pockets.c`).
- What a drawn level still does without, as the tutorial's did: hazards' sites, puzzles, trials, places to mend laid by the generator,
  things that want a spell laid by it, spectres, the lull's watch. Each is the owner's to ask for.

## 4. Later

The in-game editor (decision 7). The stamps' altar, brazier, loot and torch marks on drawn levels.

## 5. Hand-off log

**Session 1 (23 November 2026).** Part 1 built. The round trip is exact for both tutorial levels (every row, thing, room, vault and
seed). Both pass the checks; the dream finishes both (level 1 in 220 steps, level 2 in 249, its secret's four treasures missed until
the secret is pressed). The budget panel shows the tutorial levels hold more dead than the generator's budget (11 against 4.3 on level
1): a fact about the drawings. Tested in Playwright's Chromium (no page errors; no sideways scroll at 400 px) and by `test.js`. Not yet
used by the owner. Part 2 not begun.

**Session 1, part 2 (23 November 2026).** Part 2 built. Proof that nothing else moved: the dream's log (`CRAWL_SEED` 1 to 8,
`CRAWL_RUNID=7`, `apl crawl 3000`, levels 1 to 7, 8 or 9) against the build before, line for line: the same in all eight but the line
that prints a time taken (`PORTRAITS ... kept in`). The seeds fell in pairs that gave the same log (2 and 3, 4 and 5, 6 and 7), so it
is five runs that differ, not eight. Saves are untouched: creatures are not saved, and nothing else saved changed. The spawn points
proved on a test level 3 drawn in the editor and pasted in as its export says (in a worktree, never committed): three of four points
filled (the 0% one never), the rank III skeleton set to frost born to frost, the dead's cost 3.40 against level 3's 7.50, the dream
through to level 4 on seeds 1 to 3; `make asan` on it with `ASAN_OPTIONS=detect_leaks=0`, seeds 1 and 4, 400 s: clean. The test
level found a hole in the editor's checks: the exit's front must be on a lane, or the dream can never leave; that is now an error
(and a place to mend's a warning). Not played by hand; nothing run under Wine; the Windows and browser builds not compiled here.
