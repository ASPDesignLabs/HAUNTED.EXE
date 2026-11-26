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
| `tools/mapedit/build.py` | Puts `core.js` and the data into the page: `python3 -I tools/mapedit/build.py` writes `build/mapedit/haunted-world-editor.html`, the file published as the artifact (*HAUNTED.EXE World Editor*, renamed from *Catacomb Map Editor* at the owner's word) |
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

## 3b. Part 3: PLAY (built)

The owner asked to test a level without a round trip through a build: "Can this be done in the artifact?" A throwaway artifact of the
browser build showed it runs there, with sound and fonts (the owner, 25 November 2026). So the editor carries the game.

- **The game reads the export.** `game/drawn_text.c`: `crawl_drawn_text` reads the text EXPORT C gives (the arrays and the row), the
  same text that is pasted into `DRAWN[]`, into `g_dtext`. Names (`DT_*`, `MOB_*`, `RT_*`, `TA_*` ...) are looked up in a table the
  compiler fills, so a value is never wrong; a name not in it, a count past the game's limits or an index out of range is refused, with
  the line. The drawing read takes the place of whatever is drawn for its level (`drawn_level` gives `NDRAWN + 1`, `drawn()` its
  struct), and a new run begins on its level (`pockets.c`). Nothing is saved of it.
- **The doors.** The browser: `web_drawn_buf`, `web_drawn_cap`, `web_drawn_load` (`web_main.c`); `haunted.js` reads `window.HVS_LEVEL`
  before the game starts, `HVS_PREFIX` keeps its storage apart, `HVS_FORCE` its settings, and `HVS_WANT_LEVEL` holds the boot till the
  level has come; none is set on the play page, which stays as it was. The harness: `HOST_DRAWN=file`; `drawncheck` (docs/HARNESS.md).
- **The editor.** PLAY refuses while a check is an error; else it puts the game (`play/`, the browser build that `make web` leaves in
  `build/web`, copied by `build.py` with a few lines added to its page) over the editor in a frame and hands it the export by message.
  BACK TO THE EDITOR closes it. Its saves are under `haunted.exe/world-editor/`, apart from anything else in that browser.

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

**Session 3 (24 November 2026), the editor.** Spots: a marker with a tag, kept for good; a thing or a spawn point may be laid at one of
the spots of a tag, and a thing that may appear in a room goes to a spot standing in it when there is one. Fire and ice: each joined
patch of burning floor or ice is a field, chosen by clicking it, with a name, live or not at the start, its harm (fire), the chance of
its status, and whether it may kill; triggers turn fields on and off by name, and the dream walk dims a field while it is down. Three
more zooms (40, 48, 64 px a cell) with faint quarter lines, and the view keeps its middle as it zooms. The view follows the dream as it
walks (*Follow the dream*, remembered in the browser). A selected torch can no longer be dragged (dragging one moved a room). Tested by
`test.js` and in Playwright's Chromium on the full test level; not used by the owner. The game's side of all of it is with another
agent, to docs/PLAN-events.md section 2b.

**Session 4 (25 November 2026).** Renamed *HAUNTED.EXE World Editor* at the owner's word (the title, the heading, the built file
`build/mapedit/haunted-world-editor.html`, backup files `haunted-level-*.json`; older backups still load). The checks follow the game's
changes in docs/PLAN-events.md's session 4: a place to mend may face a secret room, 120 torches, a gate an event works told to be a
barred door. The owner's next ask: a PLAY button that runs the game's browser build in the artifact on the map being drawn (option A),
after a Windows play test of their level 3 (option B, done first).

**Session 4, part 3 (25 November 2026): PLAY.** Built as 3b says. Proof: the tutorial's two levels, exported by the editor and read
back, are `DRAWN[0]` and `DRAWN[1]` field for field (`drawncheck`), and the owner's Storage, exported and read, is the playtest branch's
compiled `DRAWN[2]` field for field; the dream on Storage read from text (`HOST_DRAWN`) against the dream on it compiled in (the
playtest branch), seeds 1 and 3, 420 s: line for line the same but the time-taken line; the dream's logs without it (seeds 1 and 4,
1,500 s) the same as `68876dd`'s. The export cut short at every 97th character (87 cuts) and a few odd texts: each refused with its line
and reason, none crashed, under `make asan`. In Playwright's Chromium, the editor served locally: the owner's saved map (two gates) is
refused with the checks; with barred doors, PLAY opens the game, its page names the level, it boots and the dream plays; no page errors.
The browser build compiled here with Emscripten 6.0.13 (CI's). Not yet used by the owner.

**Session 4, later (25 November 2026): rooms by run; hazards in the editor.** The room types are listed under their runs (`ROOM_RUNS`,
`ROOM_ANY` in `core.js`, as `pick_rtype` lays them); `test.js` holds every type to one group. Hazards (docs/PLAN-events.md, 2c): a
palette group headed with the level's run, its kinds as the game has them (`extract.py` reads `HZ_FIXED`, `HZ_ARISE`, `HZ_SAPPER` and
`HZ_ELEM`); each placed at its anchor (a wall for those set in one), its cells painted in order (*PAINT ITS CELLS*), placed or a zone, a
clock, a box walked into, live at the start, may kill, and harm, status and warning each "the game's own" or a number; triggers' *A
hazard wakes* and *A hazard stops*; drawn in its element's colour, the anchor as a warning sign, the box dashed; the checks (stone under
it, no cells, no box, never goes off, another run's, two of one name, the limits). Until the game's side lands, PLAY offers to play a
level without its hazards (`PLAY_HAZARDS`). Tested by `test.js` (the export, its checks, `fixtures/hazards.c`) and in Playwright's
Chromium (placing, painting, the pane, the events' list, PLAY's offer); not used by the owner.
The game's side of hazards merged (docs/PLAN-events.md, its log): PLAY lays them now (`PLAY_HAZARDS`), the browser build made again
with them; a level with hazards boots in it in Playwright's Chromium, no page errors.

**Session 4, later still (25 November 2026): layers and rolls** (docs/PLAN-lights.md). *LAYERS* in the map's bar: lights, triggers
(every trigger's box and what it watches, faint; the one being edited strong), traps (hazards, fire and ice), the dream's path, things,
rooms; each hidden layer is neither drawn nor chosen by a click; *ALL ON*, *ALL OFF*; remembered in the browser. *ROLL CREATURES* and
*ROLL GOLD* on the budget tab: five layouts over the whole level, each on its own seed (`rolls`, `rollCreatures`, `rollGold` in
`core.js`): the creatures by `random_type`'s weights (`typeWeights`, a kind once it has come), to what is left of the budget ("Count what
is placed", or the whole budget), eight cells' walk or more from the start, a cell between each, mostly in rooms, none in a vault; the
gold as `place_items` lays it (a little in most rooms, more from level 4; coins and chalices along the halls; the hoards behind each
coloured door). One chosen shows on the map; kept, they are ordinary things. One of the dead becomes a spawn point and back with a
button. Tested by `test.js` and in Playwright's Chromium on the owner's Storage; not used by the owner.

**Session 4: the lights' game side merged** (docs/PLAN-lights.md, its log). PLAY lays lights by hand (`PLAY_LIGHTS`); *THE GENERATOR'S
LIGHTS* asks the game (the browser build in `play/`, loaded once in the editor's page, no `web_start`) where its generator would hang and
stand lights on the drawing (`web_drawn_load`, `web_drawn_lights`) and gives them to the owner as lights of their own (`gen`); the styles'
order is read from the game (`extract.py`'s `lights`, held to the editor's by `test.js`). In Playwright's Chromium on the owner's
Storage: the five it lists are the harness's `drawnlights` five, cell for cell; PLAY boots with them; no page errors.

**The owner's Map 3, the Compass (26 November 2026, on `claude/eloquent-johnson-sjuioa`).** The owner's level drawn as *Storage* is now
`DRAWN[2]` in `game/drawn.c`, laid on level 3 in every run: its export word for word (`NDRAWN` 3), kept as
`tools/mapedit/fixtures/drawn3_compass.c`. `build/host_debug drawncheck` reads that export against the compiled drawing: the same, field for
field. The dream on it (`CRAWL_LEVEL=3`, seeds 1 to 8, 900 s): level 4 reached on all eight, the compass taken on all eight. Level 3 is no
longer the generator's, so the dream's logs differ from the build before's from level 3 on: that is the drawing, not a fault. Harness only;
not yet played by hand in the game's own build (the owner has played it through PLAY). The playtest branch stays unmerged as it was.
